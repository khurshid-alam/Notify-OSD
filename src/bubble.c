////////////////////////////////////////////////////////////////////////////////
//3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
//      10        20        30        40        50        60        70        80
//
// notify-osd
//
// bubble.c - implements all the rendering of a notification bubble
//
// Copyright 2009 Canonical Ltd.
//
// Authors:
//    Mirco "MacSlow" Mueller <mirco.mueller@canonical.com>
//    David Barth <david.barth@canonical.com>
//
// Contributor(s):
//    Frederic "fredp" Peters <fpeters@gnome.org> (icon-only fix, rev. 204)
//    Eitan Isaacson <eitan@ascender.com> (ATK interface for a11y, rev. 351)
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License version 3, as published
// by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranties of
// MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
// PURPOSE.  See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdlib.h>
#include <X11/Xatom.h>
#include <glib.h>
#include <gtk/gtk.h>
//#include <gdk/gdkx.h>
#include <pixman.h>
#include <math.h>

#include <egg/egg-hack.h>
#include <egg/egg-alpha.h>

#include "bubble.h"
#include "defaults.h"
#include "stack.h"
#include "dbus.h"
#include "util.h"
#include "bubble-window.h"
#include "raico-blur.h"
#include "tile.h"

struct _BubblePrivate {
	BubbleLayout     layout;
	GtkWidget*       widget;
	gboolean         visible;
	guint            timer_id;
	gboolean         mouse_over;
	gfloat           distance;
	gchar*           synchronous;
	gboolean         composited;
	EggAlpha*        alpha;
	EggTimeline*     timeline;
	guint            draw_handler_id;
	guint            pointer_update_id;
	tile_t*          tile_background_part;
	tile_t*          tile_background;
	tile_t*          tile_icon;
	tile_t*          tile_title;
	tile_t*          tile_body;
	tile_t*          tile_indicator;
	gint             title_width;
	gint             title_height;
	gint             body_width;
	gint             body_height;
	gboolean         append;
	gboolean         icon_only;
	gint             future_height;
	gboolean         prevent_fade;

	// these will be replaced by class Notification later on
	GString*         title;
	GString*         message_body;
	gboolean         title_needs_refresh;
	gboolean         message_body_needs_refresh;
	guint            id;
	GdkPixbuf*       icon_pixbuf;
	gint             value; // "empty": -2, valid range: -1..101, -1/101 trigger "over/undershoot"-effect
	gchar*           sender;
	guint            timeout;
	guint            urgency;
	//notification_t* notification;

	// used to prevent unneeded updates of the tile-cache, for append-,
	// update or replace-cases, needs to move into class Notification
	gchar *          old_icon_filename;
};

G_DEFINE_TYPE_WITH_PRIVATE (Bubble, bubble, G_TYPE_OBJECT);

enum
{
	TIMED_OUT,
	VALUE_CHANGED,
	MESSAGE_BODY_DELETED,
	MESSAGE_BODY_INSERTED,
	LAST_SIGNAL
};

enum
{
	R = 0,
	G,
	B,
	A
};

typedef struct _NotifyHSVColor NotifyHSVColor;

struct _NotifyHSVColor {
	gdouble hue;
	gdouble saturation;
	gdouble value;
};

// FIXME: this is in class Defaults already, but not yet hooked up so for the
// moment we use the macros here, these values reflect the visual-guideline
// for jaunty notifications
#define TEXT_TITLE_COLOR_R 1.0f
#define TEXT_TITLE_COLOR_G 1.0f
#define TEXT_TITLE_COLOR_B 1.0f
#define TEXT_TITLE_COLOR_A 1.0f

#define TEXT_BODY_COLOR_R  0.91f
#define TEXT_BODY_COLOR_G  0.91f
#define TEXT_BODY_COLOR_B  0.91f
#define TEXT_BODY_COLOR_A  1.0f

#define TEXT_SHADOW_COLOR_R 0.0f
#define TEXT_SHADOW_COLOR_G 0.0f 
#define TEXT_SHADOW_COLOR_B 0.0f 
#define TEXT_SHADOW_COLOR_A 1.0f 

#define BUBBLE_BG_COLOR_R  0.15f
#define BUBBLE_BG_COLOR_G  0.15f
#define BUBBLE_BG_COLOR_B  0.15f
#define BUBBLE_BG_COLOR_A  0.9f

#define INDICATOR_UNLIT_R  1.0f
#define INDICATOR_UNLIT_G  1.0f
#define INDICATOR_UNLIT_B  1.0f
#define INDICATOR_UNLIT_A  0.3f

#define INDICATOR_LIT_R    1.0f
#define INDICATOR_LIT_G    1.0f
#define INDICATOR_LIT_B    1.0f
#define INDICATOR_LIT_A    1.0f

#define FPS                 60
#define PROXIMITY_THRESHOLD 40
#define WINDOW_MIN_OPACITY  0.4f
#define WINDOW_MAX_OPACITY  1.0f

// text drop-shadow should _never_ be bigger than content blur-radius!!!
#define BUBBLE_CONTENT_BLUR_RADIUS 4
#define TEXT_DROP_SHADOW_SIZE      2

//-- private functions ---------------------------------------------------------

static guint g_bubble_signals[LAST_SIGNAL] = { 0 };
gint         g_pointer[2];

static cairo_surface_t *
bubble_create_image_surface (Bubble*        self,
			     cairo_format_t format,
			     gint           width,
			     gint           height)
{
	cairo_surface_t *surface;
	gint scale;

	scale = gtk_widget_get_scale_factor (self->priv->widget);

	surface = cairo_image_surface_create (format, scale * width, scale * height);
	cairo_surface_set_device_scale (surface, scale, scale);

	return surface;
}

static void
draw_round_rect (cairo_t* cr,
		 gdouble  aspect,        // aspect-ratio
		 gdouble  x,             // top-left corner
		 gdouble  y,             // top-left corner
		 gdouble  corner_radius, // "size" of the corners
		 gdouble  width,         // width of the rectangle
		 gdouble  height)        // height of the rectangle
{
	gdouble radius = corner_radius / aspect;

	// top-left, right of the corner
	cairo_move_to (cr, x + radius, y);

	// top-right, left of the corner
	cairo_line_to (cr,
		       x + width - radius,
		       y);

	// top-right, below the corner
	cairo_arc (cr,
                   x + width - radius,
                   y + radius,
                   radius,
                   -90.0f * G_PI / 180.0f,
                   0.0f * G_PI / 180.0f);

	// bottom-right, above the corner
	cairo_line_to (cr,
		       x + width,
		       y + height - radius);

	// bottom-right, left of the corner
	cairo_arc (cr,
                   x + width - radius,
                   y + height - radius,
                   radius,
                   0.0f * G_PI / 180.0f,
                   90.0f * G_PI / 180.0f);

	// bottom-left, right of the corner
	cairo_line_to (cr,
		       x + radius,
		       y + height);

	// bottom-left, above the corner
	cairo_arc (cr,
                   x + radius,
                   y + height - radius,
                   radius,
                   90.0f * G_PI / 180.0f,
                   180.0f * G_PI / 180.0f);

	// top-left, below the corner
	cairo_line_to (cr,
		       x,
		       y + radius);

	// top-left, right of the corner
	cairo_arc (cr,
                   x + radius,
                   y + radius,
                   radius,
                   180.0f * G_PI / 180.0f,
                   270.0f * G_PI / 180.0f);
}

// color-, alpha-, radius-, width-, height- and gradient-values were determined
// by very close obvervation of a SVG-mockup from the design-team
static void
_draw_value_indicator (cairo_t* cr,
		       gint     value,             // value to render: 0 - 100
		       gint     start_x,           // top of surrounding rect
		       gint     start_y,           // left of surrounding rect
		       gint     width,             // width of surrounding rect
		       gint     height,            // height of surrounding rect
		       gint     outline_thickness) // outline-thickness
{
	gdouble          outline_radius;
	gdouble          outline_width;
	gdouble          outline_height;
	gdouble          bar_radius;
	gdouble          bar_width;
	//gdouble          bar_height;
	cairo_pattern_t* gradient;

	outline_radius = outline_thickness;
	outline_width  = width;
	outline_height = height;

	// draw bar-background
	cairo_set_line_width (cr, outline_thickness);
	cairo_set_source_rgba (cr, 0.0f, 0.0f, 0.0f, 0.5f);
	draw_round_rect (cr,
			 1.0f,
			 start_x,
			 start_y,
			 outline_radius,
			 outline_width,
			 outline_height);
	cairo_fill (cr);
	gradient = cairo_pattern_create_linear (0.0f,
						start_y + outline_thickness,
						0.0f,
						start_y +
						outline_height -
	                                        2 * outline_thickness);
	cairo_pattern_add_color_stop_rgba (gradient,
					   0.0f,
					   0.866f,
					   0.866f,
					   0.866f,
					   0.3f);
	cairo_pattern_add_color_stop_rgba (gradient,
					   0.2f,
					   0.827f,
					   0.827f,
					   0.827f,
					   0.3f);
	cairo_pattern_add_color_stop_rgba (gradient,
					   0.3f,
					   0.772f,
					   0.772f,
					   0.772f,
					   0.3f);
	cairo_pattern_add_color_stop_rgba (gradient,
					   1.0f,
					   0.623f,
					   0.623f,
					   0.623f,
					   0.3f);
	cairo_set_source (cr, gradient);
	draw_round_rect (cr,
			 1.0f,
			 start_x + outline_thickness,
			 start_y + outline_thickness,
			 outline_radius,
			 outline_width - 2 * outline_thickness,
			 outline_height - 2 * outline_thickness);
	cairo_fill (cr);
	cairo_pattern_destroy (gradient);

	bar_radius = outline_radius;
	bar_width  = outline_width - 2 * outline_radius;
	//bar_height = outline_height - outline_radius;

	// draw value-bar
	if (value > 0)
	{
		gint corrected_value = value;

		if (corrected_value > 100)
			corrected_value = 100;

		draw_round_rect (cr,
				 1.0f,
				 start_x + outline_thickness,
				 start_y + outline_thickness,
				 bar_radius,
				 bar_width / 100.0f * (gdouble) corrected_value,
				 outline_height - 2 * outline_thickness);

		gradient = cairo_pattern_create_linear (0.0f,
							start_y +
							outline_thickness,
		                                        0.0f,
							start_y +
							outline_height -
							2 * outline_thickness);
		cairo_pattern_add_color_stop_rgba (gradient,
						   0.0f,
						   0.9f,
						   0.9f,
						   0.9f,
						   1.0f);
		cairo_pattern_add_color_stop_rgba (gradient,
						   0.75f,
						   0.5f,
						   0.5f,
						   0.5f,
						   1.0f);
		cairo_pattern_add_color_stop_rgba (gradient,
						   1.0f,
						   0.4f,
						   0.4f,
						   0.4f,
						   1.0f);

		cairo_set_source (cr, gradient);
		cairo_fill (cr);
		cairo_pattern_destroy (gradient);
	}
}

void
_draw_shadow (Bubble*  self,
	      cairo_t* cr,
	      gdouble  width,
	      gdouble  height,
	      gint     shadow_radius,
	      gint     corner_radius)
{
	cairo_surface_t* tmp_surface     = NULL;
	cairo_surface_t* new_surface     = NULL;
	cairo_pattern_t* pattern         = NULL;
	cairo_t*         cr_surf         = NULL;
	cairo_matrix_t   matrix;
	raico_blur_t*    blur            = NULL;
	double           x_scale;
	double           y_scale;

	tmp_surface = bubble_create_image_surface (self,
						   CAIRO_FORMAT_ARGB32,
						   4 * shadow_radius,
						   4 * shadow_radius);
	if (cairo_surface_status (tmp_surface) != CAIRO_STATUS_SUCCESS) {
		if (tmp_surface)
			cairo_surface_destroy (tmp_surface);
		return;
	}

	cr_surf = cairo_create (tmp_surface);
	if (cairo_status (cr_surf) != CAIRO_STATUS_SUCCESS)
	{
		cairo_surface_destroy (tmp_surface);
		if (cr_surf)
			cairo_destroy (cr_surf);
		return;
	}

	cairo_scale (cr_surf, 1.0f, 1.0f);
	cairo_set_operator (cr_surf, CAIRO_OPERATOR_CLEAR);
	cairo_paint (cr_surf);
	cairo_set_operator (cr_surf, CAIRO_OPERATOR_OVER);
	cairo_set_source_rgba (cr_surf, 0.0f, 0.0f, 0.0f, 1.0f);
	cairo_arc (cr_surf,
		   2 * shadow_radius,
		   2 * shadow_radius,
		   1.75f * corner_radius,
		   0.0f,
		   360.0f * (G_PI / 180.f));
	cairo_fill (cr_surf);
	cairo_destroy (cr_surf);

	// create and setup blur
	blur = raico_blur_create (RAICO_BLUR_QUALITY_HIGH);
	raico_blur_set_radius (blur, shadow_radius);

	// now blur it
	raico_blur_apply (blur, tmp_surface);

	// blur no longer needed
	raico_blur_destroy (blur);

	new_surface = cairo_image_surface_create_for_data (
			cairo_image_surface_get_data (tmp_surface),
			cairo_image_surface_get_format (tmp_surface),
			cairo_image_surface_get_width (tmp_surface) / 2,
			cairo_image_surface_get_height (tmp_surface) / 2,
			cairo_image_surface_get_stride (tmp_surface));
	cairo_surface_get_device_scale (tmp_surface, &x_scale, &y_scale);
	cairo_surface_set_device_scale (new_surface, x_scale, y_scale);

	pattern = cairo_pattern_create_for_surface (new_surface);
	if (cairo_pattern_status (pattern) != CAIRO_STATUS_SUCCESS)
	{
		cairo_surface_destroy (tmp_surface);
		cairo_surface_destroy (new_surface);
		if (pattern)
			cairo_pattern_destroy (pattern);
		return;
	}

	// top left
	cairo_pattern_set_extend (pattern, CAIRO_EXTEND_PAD);
	cairo_set_source (cr, pattern);
	cairo_rectangle (cr,
			 0.0f,
			 0.0f,
			 width - 2 * shadow_radius,
			 2 * shadow_radius);
	cairo_fill (cr);

	// bottom left
	cairo_matrix_init_scale (&matrix, 1.0f, -1.0f);
	cairo_matrix_translate (&matrix, 0.0f, -height);
	cairo_pattern_set_matrix (pattern, &matrix);
	cairo_rectangle (cr,
			 0.0f,
			 2 * shadow_radius,
			 2 * shadow_radius,
			 height - 2 * shadow_radius);
	cairo_fill (cr);

	// top right
	cairo_matrix_init_scale (&matrix, -1.0f, 1.0f);
	cairo_matrix_translate (&matrix, -width, 0.0f);
	cairo_pattern_set_matrix (pattern, &matrix);
	cairo_rectangle (cr,
			 width - 2 * shadow_radius,
			 0.0f,
			 2 * shadow_radius,
			 height - 2 * shadow_radius);
	cairo_fill (cr);

	// bottom right
	cairo_matrix_init_scale (&matrix, -1.0f, -1.0f);
	cairo_matrix_translate (&matrix, -width, -height);
	cairo_pattern_set_matrix (pattern, &matrix);
	cairo_rectangle (cr,
			 2 * shadow_radius,
			 height - 2 * shadow_radius,
			 width - 2 * shadow_radius,
			 2 * shadow_radius);
	cairo_fill (cr);

	// clean up
	cairo_pattern_destroy (pattern);
	cairo_surface_destroy (tmp_surface);
	cairo_surface_destroy (new_surface);
}

static gdouble
get_shadow_size (Bubble *bubble)
{
	BubblePrivate* priv = bubble->priv;
	Defaults*      d    = bubble->defaults;
	return defaults_get_bubble_shadow_size (d, priv->composited);
}

static gdouble
get_corner_radius (Bubble *bubble)
{
	BubblePrivate* priv = bubble->priv;
	Defaults*      d    = bubble->defaults;
	return defaults_get_bubble_corner_radius (d, priv->composited);
}

static void
_draw_layout_grid (cairo_t* cr,
		  Bubble*  bubble)
{
	Defaults* d = bubble->defaults;

	if (!cr)
		return;

	cairo_set_line_width (cr, 1.0f);
	cairo_set_source_rgba (cr, 1.0f, 0.5f, 0.25f, 1.0f);

	// all vertical grid lines
	cairo_move_to (cr,
		       0.5f + EM2PIXELS (get_shadow_size (bubble), d),
		       0.5f + EM2PIXELS (get_shadow_size (bubble), d));
	cairo_line_to (cr,
		       0.5f + EM2PIXELS (get_shadow_size (bubble), d),
		       0.5f + (gdouble) bubble_get_height (bubble) -
		       EM2PIXELS (get_shadow_size (bubble), d));
	cairo_move_to (cr,
		       0.5f + EM2PIXELS (get_shadow_size (bubble), d) +
		       EM2PIXELS (defaults_get_margin_size (d), d),
		       0.5f + EM2PIXELS (get_shadow_size (bubble), d));
	cairo_line_to (cr,
		       0.5f + EM2PIXELS (get_shadow_size (bubble), d) +
		       EM2PIXELS (defaults_get_margin_size (d), d),
		       0.5f + (gdouble) bubble_get_height (bubble) -
		       EM2PIXELS (get_shadow_size (bubble), d));
	cairo_move_to (cr,
		       0.5f + EM2PIXELS (get_shadow_size (bubble), d) +
		       EM2PIXELS (defaults_get_margin_size (d), d) +
		       EM2PIXELS (defaults_get_icon_size (d), d),
		       0.5f + EM2PIXELS (get_shadow_size (bubble), d));
	cairo_line_to (cr,
		       0.5f + EM2PIXELS (get_shadow_size (bubble), d) +
		       EM2PIXELS (defaults_get_margin_size (d), d) +
		       EM2PIXELS (defaults_get_icon_size (d), d),
		       0.5f + (gdouble) bubble_get_height (bubble) -
		       EM2PIXELS (get_shadow_size (bubble), d));
	cairo_move_to (cr,
		       0.5f + EM2PIXELS (get_shadow_size (bubble), d) +
		       EM2PIXELS (2 * defaults_get_margin_size (d), d) +
		       EM2PIXELS (defaults_get_icon_size (d), d),
		       0.5f + EM2PIXELS (get_shadow_size (bubble), d));
	cairo_line_to (cr,
		       0.5f + EM2PIXELS (get_shadow_size (bubble), d) +
		       EM2PIXELS (2 * defaults_get_margin_size (d), d) +
		       EM2PIXELS (defaults_get_icon_size (d), d),
		       0.5f + (gdouble) bubble_get_height (bubble) -
		       EM2PIXELS (get_shadow_size (bubble), d));
	cairo_move_to (cr,
		       0.5f + EM2PIXELS (get_shadow_size (bubble), d) +
		       EM2PIXELS (defaults_get_bubble_width (d), d) -
		       EM2PIXELS (defaults_get_margin_size (d), d),
		       0.5f + EM2PIXELS (get_shadow_size (bubble), d));
	cairo_line_to (cr,
		       0.5f + EM2PIXELS (get_shadow_size (bubble), d) +
		       EM2PIXELS (defaults_get_bubble_width (d), d) -
		       EM2PIXELS (defaults_get_margin_size (d), d),
		       0.5f + (gdouble) bubble_get_height (bubble) -
		       EM2PIXELS (get_shadow_size (bubble), d));
	cairo_move_to (cr,
		       0.5f + EM2PIXELS (get_shadow_size (bubble), d) +
		       EM2PIXELS (defaults_get_bubble_width (d), d),
		       0.5f + EM2PIXELS (get_shadow_size (bubble), d));
	cairo_line_to (cr,
		       0.5f + EM2PIXELS (get_shadow_size (bubble), d) +
		       EM2PIXELS (defaults_get_bubble_width (d), d),
		       0.5f + (gdouble) bubble_get_height (bubble) -
		       EM2PIXELS (get_shadow_size (bubble), d));

	// all horizontal grid lines
	cairo_move_to (cr,
		       0.5f + EM2PIXELS (get_shadow_size (bubble), d),
		       0.5f + EM2PIXELS (get_shadow_size (bubble), d));
	cairo_line_to (cr,
		       0.5f + EM2PIXELS (get_shadow_size (bubble), d) +
		       EM2PIXELS (defaults_get_bubble_width (d), d),
		       0.5f + EM2PIXELS (get_shadow_size (bubble), d));
	cairo_move_to (cr,
		       0.5f + EM2PIXELS (get_shadow_size (bubble), d),
		       0.5f + EM2PIXELS (get_shadow_size (bubble), d) +
		       EM2PIXELS (defaults_get_margin_size (d), d));
	cairo_line_to (cr,
		       0.5f + EM2PIXELS (get_shadow_size (bubble), d) +
		       EM2PIXELS (defaults_get_bubble_width (d), d),
		       0.5f + EM2PIXELS (get_shadow_size (bubble), d) +
		       EM2PIXELS (defaults_get_margin_size (d), d));
	cairo_move_to (cr,
		       0.5f + EM2PIXELS (get_shadow_size (bubble), d),
		       0.5f + EM2PIXELS (get_shadow_size (bubble), d) +
		       EM2PIXELS (defaults_get_margin_size (d), d) +
		       EM2PIXELS (defaults_get_icon_size (d), d));
	cairo_line_to (cr,
		       0.5f + EM2PIXELS (get_shadow_size (bubble), d) +
		       EM2PIXELS (defaults_get_bubble_width (d), d),
		       0.5f + EM2PIXELS (get_shadow_size (bubble), d) +
		       EM2PIXELS (defaults_get_margin_size (d), d) +
		       EM2PIXELS (defaults_get_icon_size (d), d));
	cairo_move_to (cr,
		       0.5f + EM2PIXELS (get_shadow_size (bubble), d),
		       0.5f + (gdouble) bubble_get_height (bubble) -
		       EM2PIXELS (get_shadow_size (bubble), d) -
		       EM2PIXELS (defaults_get_margin_size (d), d));
	cairo_line_to (cr,
		       0.5f + EM2PIXELS (get_shadow_size (bubble), d) +
		       EM2PIXELS (defaults_get_bubble_width (d), d),
		       0.5f + (gdouble) bubble_get_height (bubble) -
		       EM2PIXELS (get_shadow_size (bubble), d) -
		       EM2PIXELS (defaults_get_margin_size (d), d));
	cairo_move_to (cr,
		       0.5f + EM2PIXELS (get_shadow_size (bubble), d),
		       0.5f + (gdouble) bubble_get_height (bubble) -
		       EM2PIXELS (get_shadow_size (bubble), d));
	cairo_line_to (cr,
		       0.5f + EM2PIXELS (get_shadow_size (bubble), d) +
		       EM2PIXELS (defaults_get_bubble_width (d), d),
		       0.5f + (gdouble) bubble_get_height (bubble) -
		       EM2PIXELS (get_shadow_size (bubble), d));

	cairo_stroke (cr);
}

void
_refresh_background (Bubble* self)
{
	BubblePrivate*   priv       = self->priv;
	Defaults*        d          = self->defaults;
	cairo_t*         cr         = NULL;
	cairo_surface_t* scratch    = NULL;
	cairo_surface_t* clone      = NULL;
	cairo_surface_t* normal     = NULL;
	cairo_surface_t* blurred    = NULL;
	raico_blur_t*    blur       = NULL;
	gint             width;
	gint             height;
	gint             scratch_shadow_size;

	bubble_get_size (self, &width, &height);

	// create temp. scratch surface for top-left shadow/background part
	if (priv->composited)
	{
		scratch_shadow_size = EM2PIXELS (get_shadow_size (self), d);
		scratch = bubble_create_image_surface (
			self,
			CAIRO_FORMAT_ARGB32,
			3 * scratch_shadow_size,
			3 * scratch_shadow_size);
	}
	else
	{
		// We must have at least some width to this scratch surface.
		scratch_shadow_size = 1;
		scratch = bubble_create_image_surface (
			self,
			CAIRO_FORMAT_RGB24,
			3 * scratch_shadow_size,
			3 * scratch_shadow_size);
	}

	g_return_if_fail (scratch);

	if (cairo_surface_status (scratch) != CAIRO_STATUS_SUCCESS)
	{
		if (scratch)
			cairo_surface_destroy (scratch);

		return;
	}

	// create drawing context for that temp. scratch surface
	cr = cairo_create (scratch);
	if (cairo_status (cr) != CAIRO_STATUS_SUCCESS)
	{
		cairo_surface_destroy (scratch);

		if (cr)
			cairo_destroy (cr);

		return;
	}

	// clear, render top-left part of shadow/background in scratch-surface
    cairo_scale (cr, 1.0f, 1.0f);
	cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint (cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	GdkRGBA color;
	gchar* color_string = NULL;
    color_string = defaults_get_bubble_bg_color (d);
	gdk_rgba_parse (&color, color_string);
    g_free (color_string);

	// Apply color tweaks
	NotifyHSVColor hsv_color;
	gtk_rgb_to_hsv (color.red, color.green, color.blue,
	                &hsv_color.hue, &hsv_color.saturation, &hsv_color.value);
	hsv_color.saturation *= (2.0f - hsv_color.saturation);
	hsv_color.value = MIN (hsv_color.value, 0.4f);
	gtk_hsv_to_rgb (hsv_color.hue, hsv_color.saturation, hsv_color.value,
	                &color.red, &color.green, &color.blue);

	if (priv->composited)
	{
		_draw_shadow (
			self,
			cr,
			width,
			height,
			EM2PIXELS (get_shadow_size (self), d),
			EM2PIXELS (get_corner_radius (self), d));
		cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
		draw_round_rect (
			cr,
			1.0f,
			EM2PIXELS (get_shadow_size (self), d),
			EM2PIXELS (get_shadow_size (self), d),
			EM2PIXELS (get_corner_radius (self), d),
			EM2PIXELS (defaults_get_bubble_width (d), d),
			(gdouble) bubble_get_height (self) -
			2.0f * EM2PIXELS (get_shadow_size (self), d));
		cairo_fill (cr);
		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
		cairo_set_source_rgba (cr,
				       color.red,
				       color.green,
				       color.blue,
				       BUBBLE_BG_COLOR_A);
	}
	else
		cairo_set_source_rgb (cr,
				       color.red,
				       color.green,
				       color.blue);

	draw_round_rect (
		cr,
		1.0f,
		EM2PIXELS (get_shadow_size (self), d),
		EM2PIXELS (get_shadow_size (self), d),
		EM2PIXELS (get_corner_radius (self), d),
		EM2PIXELS (defaults_get_bubble_width (d), d),
		(gdouble) bubble_get_height (self) -
		2.0f * EM2PIXELS (get_shadow_size (self), d));
	cairo_fill (cr);
	cairo_destroy (cr);

	// create temp. clone of scratch surface
	clone = copy_surface (scratch);

	// create normal surface from that surface-clone
	normal = copy_surface (clone);

	// now blur the surface-clone
	blur = raico_blur_create (RAICO_BLUR_QUALITY_LOW);
	raico_blur_set_radius (blur, BUBBLE_CONTENT_BLUR_RADIUS);
	raico_blur_apply (blur, clone);
	raico_blur_destroy (blur);

	// create blurred version from that blurred surface-clone 
	blurred = copy_surface (clone);
	cairo_surface_destroy (clone);

	// finally create tile with top-left shadow/background part
	if (priv->tile_background_part)
		tile_destroy (priv->tile_background_part);
	priv->tile_background_part = tile_new_for_padding (normal, blurred,
							   3 * scratch_shadow_size,
							   3 * scratch_shadow_size);
	cairo_surface_destroy (normal);
	cairo_surface_destroy (blurred);

	// create surface(s) for full shadow/background tile
	if (priv->composited)
	{
		// we need two RGBA-surfaces
		normal = bubble_create_image_surface (self,
						      CAIRO_FORMAT_ARGB32,
						      width,
						      height);
		if (cairo_surface_status (normal) != CAIRO_STATUS_SUCCESS)
		{
			cairo_surface_destroy (scratch);

			if (normal)
				cairo_surface_destroy (normal);

			return;
		}

		blurred = bubble_create_image_surface (self,
						       CAIRO_FORMAT_ARGB32,
						       width,
						       height);
		if (cairo_surface_status (blurred) != CAIRO_STATUS_SUCCESS)
		{
			cairo_surface_destroy (normal);
			cairo_surface_destroy (scratch);

			if (blurred)
				cairo_surface_destroy (blurred);

			return;
		}
	}
	else
	{
		// we need only one RGB-surface
		normal = bubble_create_image_surface (self,
						      CAIRO_FORMAT_RGB24,
						      width,
						      height);
		if (cairo_surface_status (normal) != CAIRO_STATUS_SUCCESS)
		{
			cairo_surface_destroy (scratch);

			if (normal)
				cairo_surface_destroy (normal);

			return;
		}
	}

	// use tile for top-left background-part to fill the full bg-surface
	if (priv->composited)
	{
		// create context for blurred surface
		cr = cairo_create (blurred);
		if (cairo_status (cr) != CAIRO_STATUS_SUCCESS)
		{
			cairo_surface_destroy (normal);
			cairo_surface_destroy (blurred);
			cairo_surface_destroy (scratch);

			if (cr)
				cairo_destroy (cr);

			return;
		}

		// clear blurred surface
	    	cairo_scale (cr, 1.0f, 1.0f);
		cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
		cairo_paint (cr);
		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

		// fill with blurred state of background-part tile
		tile_paint_with_padding (priv->tile_background_part,
					 cr,
					 0.0f,
					 0.0f,
					 width,
					 height,
					 0.0f,
					 1.0f);

		// get rid of context for blurred surface
		cairo_destroy (cr);
	}

	// create context for normal surface
	cr = cairo_create (normal);
	if (cairo_status (cr) != CAIRO_STATUS_SUCCESS)
	{
		cairo_surface_destroy (normal);
		cairo_surface_destroy (scratch);

		if (priv->composited)
			cairo_surface_destroy (blurred);

		return;
	}

	// clear normal surface
    	cairo_scale (cr, 1.0f, 1.0f);
	cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint (cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	// fill with normal state of background-part tile
	tile_paint_with_padding (priv->tile_background_part,
				 cr,
				 0.0f,
				 0.0f,
				 width,
				 height,
				 1.0f,
				 0.0f);

	// get rid of context for normal surface
	cairo_destroy (cr);

	// finally create tile for full background
	if (priv->tile_background)
		tile_destroy (priv->tile_background);
	if (priv->composited)
		priv->tile_background = tile_new_for_padding (normal, blurred, width, height);
	else
		priv->tile_background = tile_new_for_padding (normal, normal, width, height);

	// clean up
	if (priv->composited)
		cairo_surface_destroy (blurred);

	cairo_surface_destroy (normal);
	cairo_surface_destroy (scratch);
}

void
_refresh_icon (Bubble* self)
{
	BubblePrivate*   priv         = self->priv;
	Defaults*        d            = self->defaults;
	cairo_surface_t* normal       = NULL;
	cairo_surface_t* icon_surface = NULL;
	cairo_t*         cr           = NULL;

	if (!priv->icon_pixbuf)
		return;

	// create temp. scratch surface
	normal = bubble_create_image_surface (
			self,
			CAIRO_FORMAT_ARGB32,
			EM2PIXELS (defaults_get_icon_size (d), d) +
			2 * BUBBLE_CONTENT_BLUR_RADIUS,
			EM2PIXELS (defaults_get_icon_size (d), d) +
			2 * BUBBLE_CONTENT_BLUR_RADIUS);
	if (cairo_surface_status (normal) != CAIRO_STATUS_SUCCESS)
		return;

	// create context for normal surface
	cr = cairo_create (normal);
	if (cairo_status (cr) != CAIRO_STATUS_SUCCESS)
	{
		cairo_surface_destroy (normal);
		return;
	}

	// clear normal surface
    	cairo_scale (cr, 1.0f, 1.0f);
	cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint (cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	// render icon into normal surface
	icon_surface = gdk_cairo_surface_create_from_pixbuf (priv->icon_pixbuf, 0,
							     gtk_widget_get_window (priv->widget));
	cairo_set_source_surface (cr,
				  icon_surface,
				  BUBBLE_CONTENT_BLUR_RADIUS,
				  BUBBLE_CONTENT_BLUR_RADIUS);
	cairo_paint (cr);

	// create the surface/blur-cache from the normal surface
	if (priv->tile_icon)
		tile_destroy (priv->tile_icon);
	priv->tile_icon = tile_new (normal, BUBBLE_CONTENT_BLUR_RADIUS/2);

	// clean up
	cairo_destroy (cr);
	cairo_surface_destroy (icon_surface);
	cairo_surface_destroy (normal);
}

void
_refresh_title (Bubble* self)
{
	BubblePrivate*        priv           = self->priv;
	Defaults*             d              = self->defaults;
	cairo_surface_t*      normal         = NULL;
	cairo_t*              cr             = NULL;
	PangoFontDescription* desc           = NULL;
	PangoLayout*          layout         = NULL;
	raico_blur_t*         blur           = NULL;
	gchar*                text_font_face = NULL;

	// create temp. scratch surface
	normal = bubble_create_image_surface (
			self,
			CAIRO_FORMAT_ARGB32,
			priv->title_width + 2 * BUBBLE_CONTENT_BLUR_RADIUS,
			priv->title_height + 2 * BUBBLE_CONTENT_BLUR_RADIUS);
	if (cairo_surface_status (normal) != CAIRO_STATUS_SUCCESS)
		return;

	// create context for normal surface
	cr = cairo_create (normal);
	if (cairo_status (cr) != CAIRO_STATUS_SUCCESS)
	{
		cairo_surface_destroy (normal);
		return;
	}

	// clear normal surface
    	cairo_scale (cr, 1.0f, 1.0f);
	cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint (cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	// create pango desc/layout
	layout = pango_cairo_create_layout (cr);
	text_font_face = defaults_get_text_font_face (d);
	desc = pango_font_description_from_string (text_font_face);
	g_free ((gpointer) text_font_face);

	pango_font_description_set_size (desc,
					 defaults_get_system_font_size (d) *
					 defaults_get_text_title_size (d) *
					 PANGO_SCALE);

	pango_font_description_set_weight (desc,
					   defaults_get_text_title_weight (d));
	pango_layout_set_font_description (layout, desc);
	pango_font_description_free (desc);

	pango_layout_set_wrap (layout, PANGO_WRAP_WORD_CHAR);
	pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);
	pango_layout_set_width (layout, priv->title_width * PANGO_SCALE);
	pango_layout_set_height (layout, priv->title_height * PANGO_SCALE);

	// print and layout string (pango-wise)
	pango_layout_set_text (layout, priv->title->str, priv->title->len);

	// make sure system-wide font-options like hinting, antialiasing etc.
	// are taken into account
	pango_cairo_context_set_font_options (
		pango_layout_get_context (layout),
		gdk_screen_get_font_options (
			gtk_widget_get_screen (priv->widget)));
	pango_cairo_context_set_resolution  (pango_layout_get_context (layout),
					     defaults_get_screen_dpi (d));
	pango_layout_context_changed (layout);

	// draw text for drop-shadow and ...
	cairo_move_to (cr,
	               BUBBLE_CONTENT_BLUR_RADIUS,
	               BUBBLE_CONTENT_BLUR_RADIUS);
	cairo_set_source_rgba (cr,
			       TEXT_SHADOW_COLOR_R,
			       TEXT_SHADOW_COLOR_G,
			       TEXT_SHADOW_COLOR_B,
			       TEXT_SHADOW_COLOR_A);
	pango_cairo_show_layout (cr, layout);

	// ... blur it
	blur = raico_blur_create (RAICO_BLUR_QUALITY_LOW);
	raico_blur_set_radius (blur, TEXT_DROP_SHADOW_SIZE);
	raico_blur_apply (blur, normal);
	raico_blur_destroy (blur);

	// now draw normal (non-blurred) text over drop-shadow
	cairo_set_source_rgba (cr,
			       TEXT_TITLE_COLOR_R,
			       TEXT_TITLE_COLOR_G,
			       TEXT_TITLE_COLOR_B,
			       TEXT_TITLE_COLOR_A);
	pango_cairo_show_layout (cr, layout);

	g_object_unref (layout);

	// create the surface/blur-cache from the normal surface
	if (priv->tile_title)
		tile_destroy (priv->tile_title);
	priv->tile_title = tile_new (normal, BUBBLE_CONTENT_BLUR_RADIUS);

	// clean up
	cairo_destroy (cr);
	cairo_surface_destroy (normal);

	// reset refresh-flag
	priv->title_needs_refresh = FALSE;
}

void
_refresh_body (Bubble* self)
{
	BubblePrivate*        priv           = self->priv;
	Defaults*             d              = self->defaults;
	cairo_surface_t*      normal         = NULL;
	cairo_t*              cr             = NULL;
	PangoFontDescription* desc           = NULL;
	PangoLayout*          layout         = NULL;
	raico_blur_t*         blur           = NULL;
	gchar*                text_font_face = NULL;

	// create temp. scratch surface
	normal = bubble_create_image_surface (
			self,
			CAIRO_FORMAT_ARGB32,
			priv->body_width + 2 * BUBBLE_CONTENT_BLUR_RADIUS,
			priv->body_height + 2 * BUBBLE_CONTENT_BLUR_RADIUS);
	if (cairo_surface_status (normal) != CAIRO_STATUS_SUCCESS)
		return;

	// create context for normal surface
	cr = cairo_create (normal);
	if (cairo_status (cr) != CAIRO_STATUS_SUCCESS)
	{
		cairo_surface_destroy (normal);
		return;
	}

	// clear normal surface
    	cairo_scale (cr, 1.0f, 1.0f);
	cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint (cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	// create pango desc/layout
	layout = pango_cairo_create_layout (cr);
	text_font_face = defaults_get_text_font_face (d);
	desc = pango_font_description_from_string (text_font_face);
	g_free ((gpointer) text_font_face);

	pango_font_description_set_size (desc,
					 defaults_get_system_font_size (d) *
					 defaults_get_text_body_size (d) *
					 PANGO_SCALE);

	pango_font_description_set_weight (desc,
					   defaults_get_text_body_weight (d));
	pango_layout_set_font_description (layout, desc);
	pango_font_description_free (desc);

	pango_layout_set_wrap (layout, PANGO_WRAP_WORD_CHAR);
	pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);
	pango_layout_set_width (layout, priv->body_width * PANGO_SCALE);
	pango_layout_set_height (layout, priv->body_height * PANGO_SCALE);

	// print and layout string (pango-wise)
	pango_layout_set_text (layout,
	                       priv->message_body->str,
	                       priv->message_body->len);

	// make sure system-wide font-options like hinting, antialiasing etc.
	// are taken into account
	pango_cairo_context_set_font_options (
		pango_layout_get_context (layout),
		gdk_screen_get_font_options (
			gtk_widget_get_screen (priv->widget)));
	pango_cairo_context_set_resolution  (pango_layout_get_context (layout),
					     defaults_get_screen_dpi (d));
	pango_layout_context_changed (layout);

	// draw text for drop-shadow and ...
	cairo_move_to (cr,
	               BUBBLE_CONTENT_BLUR_RADIUS,
	               BUBBLE_CONTENT_BLUR_RADIUS);
	cairo_set_source_rgba (cr,
			       TEXT_SHADOW_COLOR_R,
			       TEXT_SHADOW_COLOR_G,
			       TEXT_SHADOW_COLOR_B,
			       TEXT_SHADOW_COLOR_A);
	pango_cairo_show_layout (cr, layout);

	// ... blur it
	blur = raico_blur_create (RAICO_BLUR_QUALITY_LOW);
	raico_blur_set_radius (blur, TEXT_DROP_SHADOW_SIZE);
	raico_blur_apply (blur, normal);
	raico_blur_destroy (blur);

	// now draw normal (non-blurred) text over drop-shadow
	cairo_set_source_rgba (cr,
			       TEXT_BODY_COLOR_R,
			       TEXT_BODY_COLOR_G,
			       TEXT_BODY_COLOR_B,
			       TEXT_BODY_COLOR_A);
	pango_cairo_show_layout (cr, layout);

	g_object_unref (layout);

	// create the surface/blur-cache from the normal surface
	if (priv->tile_body)
		tile_destroy (priv->tile_body);
	priv->tile_body = tile_new (normal, BUBBLE_CONTENT_BLUR_RADIUS);

	// clean up
	cairo_destroy (cr);
	cairo_surface_destroy (normal);

	// reset refresh-flag
	priv->message_body_needs_refresh = FALSE;
}

void
_refresh_indicator (Bubble* self)
{
	BubblePrivate*   priv   = self->priv;
	Defaults*        d      = self->defaults;
	cairo_surface_t* normal = NULL;
	cairo_t*         cr     = NULL;

	// create temp. scratch surface
	normal = bubble_create_image_surface (
			self,
			CAIRO_FORMAT_ARGB32,
			EM2PIXELS (defaults_get_bubble_width (d), d) -
			3 * EM2PIXELS (defaults_get_margin_size (d), d) -
			EM2PIXELS (defaults_get_icon_size (d), d)
			+ 2 * BUBBLE_CONTENT_BLUR_RADIUS,
			EM2PIXELS (defaults_get_icon_size (d), d) / 5.0f
			+ 2 * BUBBLE_CONTENT_BLUR_RADIUS);
	if (cairo_surface_status (normal) != CAIRO_STATUS_SUCCESS)
		return;

	// create context for normal surface
	cr = cairo_create (normal);
	if (cairo_status (cr) != CAIRO_STATUS_SUCCESS)
	{
		cairo_surface_destroy (normal);
		return;
	}

	// clear normal surface
    	cairo_scale (cr, 1.0f, 1.0f);
	cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint (cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	_draw_value_indicator (
		cr,
		priv->value,
		BUBBLE_CONTENT_BLUR_RADIUS,
		BUBBLE_CONTENT_BLUR_RADIUS,
		EM2PIXELS (defaults_get_bubble_width (d), d) -
		3 * EM2PIXELS (defaults_get_margin_size (d), d) -
		EM2PIXELS (defaults_get_icon_size (d), d),
		EM2PIXELS (defaults_get_gauge_size (d), d),
		EM2PIXELS (defaults_get_gauge_outline_width (d), d));

	// create the surface/blur-cache from the normal surface
	if (priv->tile_indicator)
		tile_destroy (priv->tile_indicator);
	priv->tile_indicator = tile_new (normal, BUBBLE_CONTENT_BLUR_RADIUS/2);

	// clean up
	cairo_destroy (cr);
	cairo_surface_destroy (normal);
}

void
_render_background (Bubble*  self,
                    cairo_t* cr,
		    gdouble  alpha_normal,
		    gdouble  alpha_blur)
{
	Defaults* d = self->defaults;

	tile_paint (self->priv->tile_background,
		    cr,
		    0.0f,
		    0.0f,
		    alpha_normal,
		    alpha_blur);

	// layout-grid and urgency-indication bar
	if (g_getenv ("DEBUG"))
	{
		// for debugging layout and positioning issues
		_draw_layout_grid (cr, self);

		switch (bubble_get_urgency (self))
		{
			// low urgency-bar is painted blue
			case 0:
				cairo_set_source_rgb (cr, 0.25f, 0.5f, 1.0f);
			break;

			// normal urgency-bar is painted green
			case 1:
				cairo_set_source_rgb (cr, 0.0f, 1.0f, 0.0f);
			break;

			// urgent urgency-bar is painted red
			case 2:
				cairo_set_source_rgb (cr, 1.0f, 0.0f, 0.0f);
			break;

			default:
			break;
		}

		draw_round_rect (
			cr,
			1.0f,
			EM2PIXELS (get_shadow_size (self), d) + 2.0f,
			EM2PIXELS (get_shadow_size (self), d) + 2.0f,
			EM2PIXELS (get_corner_radius (self), d) - 2.0f,
			EM2PIXELS (defaults_get_bubble_width (d), d) - 4.0f,
			2.0f * EM2PIXELS (get_shadow_size (self), d) - 2.0f);
		cairo_fill (cr);

		cairo_set_source_rgb (cr, 0.0f, 0.0f, 0.0f);
		cairo_set_font_size (
			cr,
			EM2PIXELS (defaults_get_text_body_size  (d), d));
		cairo_move_to (
			cr,
			EM2PIXELS (defaults_get_text_body_size  (d), d) +
			EM2PIXELS (get_shadow_size (self), d) +
			2.0f,
			EM2PIXELS (defaults_get_text_body_size  (d), d) +
			EM2PIXELS (get_shadow_size (self), d) +
			2.0f +
			((2.0f * EM2PIXELS (get_shadow_size (self), d) - 2.0f) -
			EM2PIXELS (defaults_get_text_body_size  (d), d)) / 2);

		switch (bubble_get_urgency (self))
		{
			case 0:
				cairo_show_text (
					cr,
					"low - report incorrect urgency?");
			break;

			case 1:
				cairo_show_text (
					cr,
					"normal - report incorrect urgency?");
			break;

			case 2:
				cairo_show_text (
					cr,
					"urgent - report incorrect urgency?");
			break;

			default:
			break;
		}
	}
}

void
_render_icon (Bubble*  self,
	      cairo_t* cr,
	      gint     x,
	      gint     y,
	      gdouble  alpha_normal,
	      gdouble  alpha_blur)
{
	BubblePrivate*   priv = self->priv;
	cairo_pattern_t* pattern;

	tile_paint (priv->tile_icon,
		    cr,
		    x,
		    y,
		    alpha_normal,
		    alpha_blur);

	if (priv->alpha)
	{
		cairo_push_group (cr);
		tile_paint (priv->tile_icon,
			    cr,
			    x,
			    y,
			    0.0f,
			    (gfloat) egg_alpha_get_alpha (priv->alpha) /
			    (gfloat) EGG_ALPHA_MAX_ALPHA);
		pattern = cairo_pop_group (cr);
		if (priv->value == -1)
			cairo_set_source_rgba (cr, 0.0f, 0.0f, 0.0f, 1.0f);
		if (priv->value == 101)
			cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 1.0f);
		cairo_mask (cr, pattern);
		cairo_pattern_destroy (pattern);
	}
}

void
_render_title (Bubble*  self,
	       cairo_t* cr,
	       gint     x,
	       gint     y,
	       gdouble  alpha_normal,
	       gdouble  alpha_blur)
{
	tile_paint (self->priv->tile_title,
		    cr,
		    x,
		    y,
		    alpha_normal,
		    alpha_blur);
}

void
_render_body (Bubble*  self,
	      cairo_t* cr,
	      gint     x,
	      gint     y,
	      gdouble  alpha_normal,
	      gdouble  alpha_blur)
{
	tile_paint (self->priv->tile_body,
		    cr,
		    x,
		    y,
		    alpha_normal,
		    alpha_blur);
}

void
_render_indicator (Bubble*  self,
		   cairo_t* cr,
		   gint     x,
		   gint     y,
		   gdouble  alpha_normal,
		   gdouble  alpha_blur)
{
	BubblePrivate*   priv = self->priv;
	cairo_pattern_t* pattern;

	tile_paint (priv->tile_indicator,
		    cr,
		    x,
		    y,
		    alpha_normal,
		    alpha_blur);

	if (priv->alpha)
	{
		cairo_push_group (cr);
		tile_paint (priv->tile_indicator,
			    cr,
			    x,
			    y,
			    0.0f,
			    (gfloat) egg_alpha_get_alpha (priv->alpha) /
			    (gfloat) EGG_ALPHA_MAX_ALPHA);
		pattern = cairo_pop_group (cr);
		if (priv->value == -1)
			cairo_set_source_rgba (cr, 0.0f, 0.0f, 0.0f, 1.0f);
		if (priv->value == 101)
			cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 1.0f);
		cairo_mask (cr, pattern);
		cairo_pattern_destroy (pattern);
	}
}

void
_render_layout (Bubble*  self,
		cairo_t* cr,
		gdouble  alpha_normal,
		gdouble  alpha_blur)
{
	Defaults* d           = self->defaults;
	gint      shadow      = EM2PIXELS (get_shadow_size (self), d);
	gint      icon_half   = EM2PIXELS (defaults_get_icon_size (d), d) / 2;
	gint      width_half  = EM2PIXELS (defaults_get_bubble_width (d), d) / 2;
	gint      height_half = EM2PIXELS (defaults_get_bubble_min_height (d), d) / 2;
	gint      gauge_half  = EM2PIXELS (defaults_get_gauge_size (d), d) / 2;
	gint      margin      = EM2PIXELS (defaults_get_margin_size (d), d);

	switch (bubble_get_layout (self))
	{
		case LAYOUT_ICON_ONLY:
			_render_icon (self,
				      cr,
				      shadow + width_half - icon_half - BUBBLE_CONTENT_BLUR_RADIUS,
				      shadow + height_half - icon_half - BUBBLE_CONTENT_BLUR_RADIUS,
				      alpha_normal,
				      alpha_blur);
		break;

		case LAYOUT_ICON_INDICATOR:
			_render_icon (self,
				      cr,
				      shadow + margin - BUBBLE_CONTENT_BLUR_RADIUS,
				      shadow + margin - BUBBLE_CONTENT_BLUR_RADIUS,
				      alpha_normal,
				      alpha_blur);
			_render_indicator (self,
					   cr,
					   shadow + 2 * margin + 2 * icon_half - BUBBLE_CONTENT_BLUR_RADIUS,
					   shadow + margin + icon_half - gauge_half - BUBBLE_CONTENT_BLUR_RADIUS,
					   alpha_normal,
					   alpha_blur);
		break;

		case LAYOUT_ICON_TITLE:
			_render_icon (self,
				      cr,
				      shadow + margin - BUBBLE_CONTENT_BLUR_RADIUS,
				      shadow + margin - BUBBLE_CONTENT_BLUR_RADIUS,
				      alpha_normal,
				      alpha_blur);
			_render_title (self,
				       cr,
				       shadow + 2 * margin + 2 * icon_half - BUBBLE_CONTENT_BLUR_RADIUS,
				       shadow + margin + icon_half - self->priv->title_height / 2 - BUBBLE_CONTENT_BLUR_RADIUS,
				       alpha_normal,
				       alpha_blur);
		break;

		case LAYOUT_ICON_TITLE_BODY:
			_render_icon (self,
				      cr,
				      shadow + margin - BUBBLE_CONTENT_BLUR_RADIUS,
				      shadow + margin - BUBBLE_CONTENT_BLUR_RADIUS,
				      alpha_normal,
				      alpha_blur);
			_render_title (self,
				       cr,
				       shadow + 2 * margin + 2 * icon_half - BUBBLE_CONTENT_BLUR_RADIUS,
				       shadow + margin - BUBBLE_CONTENT_BLUR_RADIUS,
				       alpha_normal,
				       alpha_blur);
			_render_body (self,
				      cr,
				      shadow + 2 * margin + 2 * icon_half - BUBBLE_CONTENT_BLUR_RADIUS,
				      shadow + margin + self->priv->title_height - BUBBLE_CONTENT_BLUR_RADIUS,
				      alpha_normal,
				      alpha_blur);
		break;

		case LAYOUT_TITLE_BODY:
			_render_title (self,
				       cr,
				       shadow + margin - BUBBLE_CONTENT_BLUR_RADIUS,
				       shadow + margin - BUBBLE_CONTENT_BLUR_RADIUS,
				       alpha_normal,
				       alpha_blur);
			_render_body (self,
				      cr,
				      shadow + margin - BUBBLE_CONTENT_BLUR_RADIUS,
				      shadow + margin + self->priv->title_height - BUBBLE_CONTENT_BLUR_RADIUS,
				      alpha_normal,
				      alpha_blur);
		break;

		case LAYOUT_TITLE_ONLY:
			_render_title (self,
				       cr,
				       shadow + margin - BUBBLE_CONTENT_BLUR_RADIUS,
				       shadow + margin - BUBBLE_CONTENT_BLUR_RADIUS,
				       alpha_normal,
				       alpha_blur);
		break;

		case LAYOUT_NONE:
			// should be intercepted by stack_notify_handler()
			g_warning ("WARNING: No layout defined!!!\n");
		break;
	}
}

// the behind-bubble blur only works with the enabled/working compiz-plugin blur
// by setting the hint _COMPIZ_WM_WINDOW_BLUR on the bubble-window
void
_set_bg_blur (GtkWidget* window,
	      gboolean   set_blur,
	      gint       shadow_size)
{
	glong data[8];
	GtkAllocation a;
	GdkWindow *gdkwindow;

	// sanity check
	if (!window)
		return;

	gtk_widget_get_allocation (window, &a);
	gdkwindow = gtk_widget_get_window (window);

	// this is meant to tell the blur-plugin what and how to blur, somehow
	// the y-coords are interpreted as being CenterGravity, I wonder why
	data[0] = 2;                             // threshold
	data[1] = 0;                             // filter
	data[2] = NorthWestGravity;              // gravity of top-left
	data[3] = shadow_size;                   // x-coord of top-left
	data[4] = (-a.height / 2) + shadow_size; // y-coord of top-left
	data[5] = NorthWestGravity;              // gravity of bottom-right
	data[6] = a.width - shadow_size;         // bottom-right x-coord
	data[7] = (a.height / 2) - shadow_size;  // bottom-right y-coord

	if (set_blur)
	{
		XChangeProperty (GDK_WINDOW_XDISPLAY (gdkwindow),
				 GDK_WINDOW_XID (gdkwindow),
				 XInternAtom (GDK_WINDOW_XDISPLAY (gdkwindow),
					      "_COMPIZ_WM_WINDOW_BLUR",
					      FALSE),
				 XA_INTEGER,
				 32,
				 PropModeReplace,
				 (guchar *) data,
				 8);
	}
	else
	{
		XDeleteProperty (GDK_WINDOW_XDISPLAY (gdkwindow),
				 GDK_WINDOW_XID (gdkwindow),
				 XInternAtom (GDK_WINDOW_XDISPLAY (gdkwindow),
					      "_COMPIZ_WM_WINDOW_BLUR",
					      FALSE));
	}
}

static
void
screen_changed_handler (GtkWindow* window,
						GdkScreen* old_screen,
						gpointer   data)
{
	GdkScreen* screen = gtk_widget_get_screen (GTK_WIDGET (window));
	GdkVisual* visual = gdk_screen_get_rgba_visual (screen);

	if (!visual)
		visual = gdk_screen_get_system_visual (screen);

	gtk_widget_set_visual (GTK_WIDGET (window), visual);
}

static
void
update_input_shape (GtkWidget* window)
{
	cairo_region_t*             region = NULL;
	const cairo_rectangle_int_t rect   = {0, 0, 1, 1};

	// sanity check
	if (!window)
		return;

	// set an 1x1 input-region to allow click-through 
	region = cairo_region_create_rectangle (&rect);
	if (cairo_region_status (region) == CAIRO_STATUS_SUCCESS)
	{
		gtk_widget_input_shape_combine_region (window, NULL);
		gtk_widget_input_shape_combine_region (window, region);
	}
	cairo_region_destroy (region);
}

static void
update_shape (Bubble* self)
{
	gint           width;
	gint           height;
	BubblePrivate* priv;

	// sanity test
	if (!self || !IS_BUBBLE (self))
		return;

	priv = self->priv;

	// do we actually need a shape-mask at all?
	if (priv->composited)
	{
		gtk_widget_shape_combine_region (priv->widget, NULL);
		return;
	}

	// we're not-composited, so deal with mouse-over differently
	if (bubble_is_mouse_over (self))
	{
		cairo_region_t* region = NULL;

		region = cairo_region_create ();
		gdk_window_shape_combine_region (gtk_widget_get_window (priv->widget),
						 region,
						 0,
						 0);
		cairo_region_destroy (region);
	}
	else
	{
		gtk_widget_get_size_request (priv->widget, &width, &height);
		const cairo_rectangle_int_t rects[]   = {{2, 0, width - 4, height},
												 {1, 1, width - 2, height - 2},
												 {0, 2, width, height - 4}};
		cairo_region_t*             region = NULL;

		region = cairo_region_create_rectangles (rects, 3);
		if (cairo_region_status (region) == CAIRO_STATUS_SUCCESS)
		{
			gtk_widget_shape_combine_region (priv->widget, NULL);
			gtk_widget_shape_combine_region (priv->widget, region);
		}
	}
}

static void
composited_changed_handler (GtkWidget* window,
			    gpointer   data)
{
	Bubble*        bubble;
	BubblePrivate* priv;

	bubble = BUBBLE (data);
	priv = bubble->priv;

	priv->composited = gdk_screen_is_composited (gtk_widget_get_screen (window));

	update_shape (bubble);
}

static gboolean
bubble_draw (GtkWidget* widget,
             cairo_t*   cr,
             gpointer   data)
{
	Bubble*        bubble;
	Defaults*      d;
	BubblePrivate* priv;

	bubble = (Bubble*) G_OBJECT (data);

	d    = bubble->defaults;
	priv = bubble->priv;

        // clear bubble-background
	cairo_scale (cr, 1.0f, 1.0f);
	cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint (cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	if (priv->prevent_fade || !priv->composited)
	{
	        // render drop-shadow and bubble-background
		_render_background (bubble, cr, 1.0f, 0.0f);

		// render content of bubble depending on layout
		_render_layout (bubble, cr, 1.0f, 0.0f);
	}
	else
	{
	        // render drop-shadow and bubble-background
		_render_background (bubble,
		                    cr,
		                    priv->distance,
		                    1.0f - priv->distance);
    
		// render content of bubble depending on layout
		_render_layout (bubble,
		                cr,
		                priv->distance,
		                1.0f - priv->distance);
	}

	_set_bg_blur (widget,
		      TRUE,
		      EM2PIXELS (get_shadow_size (bubble), d));

	return TRUE;
}

static gboolean
redraw_handler (Bubble* bubble)
{
	GtkWindow*     window;
	BubblePrivate* priv;

	if (!bubble)
		return FALSE;

	if (!bubble_is_visible (bubble))
		return FALSE;

	priv = bubble->priv;

	if (!priv->composited)
		return TRUE;

	window = GTK_WINDOW (priv->widget);

	if (!GTK_IS_WINDOW (window))
		return FALSE;

	if (priv->alpha == NULL)
	{
		if (priv->distance < 1.0f && !priv->prevent_fade)
		{
			gtk_widget_set_opacity (priv->widget,
			                        WINDOW_MIN_OPACITY +
			                        priv->distance *
			                        (WINDOW_MAX_OPACITY -
			                         WINDOW_MIN_OPACITY));
			bubble_refresh (bubble);
		}
		else
			gtk_widget_set_opacity (priv->widget, WINDOW_MAX_OPACITY);
	}

	return TRUE;
}

static
gboolean
pointer_update (Bubble* bubble)
{
	gint           pointer_rel_x;
	gint           pointer_rel_y;
	gint           pointer_abs_x;
	gint           pointer_abs_y;
	gint           win_x;
	gint           win_y;
	gint           width;
	gint           height;
	GtkWidget*     window;
	BubblePrivate* priv;

	if (!bubble)
		return FALSE;

	if (!bubble_is_visible (bubble))
		return FALSE;

	priv   = bubble->priv;
	window = priv->widget;

	if (!GTK_IS_WINDOW (window))
		return FALSE;

	if (gtk_widget_get_realized (window))
	{
		GdkDeviceManager *device_manager;
		GdkDevice        *device;
		gint              distance_x;
		gint              distance_y;

		device_manager = gdk_display_get_device_manager (gtk_widget_get_display (window));
		device = gdk_device_manager_get_client_pointer (device_manager);
		gdk_window_get_device_position (gtk_widget_get_window (window),
		                                device,
		                                &pointer_rel_x,
		                                &pointer_rel_y,
		                                NULL);

		gtk_window_get_position (GTK_WINDOW (window), &win_x, &win_y);
		pointer_abs_x = win_x + pointer_rel_x;
		pointer_abs_y = win_y + pointer_rel_y;
		gtk_window_get_size (GTK_WINDOW (window), &width, &height);
		if (pointer_abs_x >= win_x &&
		    pointer_abs_x <= win_x + width &&
		    pointer_abs_y >= win_y &&
		    pointer_abs_y <= win_y + height)
		{
			bubble_set_mouse_over (bubble, TRUE);
		}
		else
		{
			bubble_set_mouse_over (bubble, FALSE);
		}

		if (pointer_abs_x >= win_x &&
		    pointer_abs_x <= win_x + width)
			distance_x = 0;
		else
		{
			if (pointer_abs_x < win_x)
				distance_x = abs (pointer_rel_x);

			if (pointer_abs_x > win_x + width)
				distance_x = abs (pointer_rel_x - width);
		}

		if (pointer_abs_y >= win_y &&
		    pointer_abs_y <= win_y + height)
			distance_y = 0;
		else
		{
			if (pointer_abs_y < win_y)
				distance_y = abs (pointer_rel_y);

			if (pointer_abs_y > win_y + height)
				distance_y = abs (pointer_rel_y - height);
		}

		priv->distance = sqrt (distance_x * distance_x +
				       distance_y * distance_y) /
				       (double) PROXIMITY_THRESHOLD;

		// mark mouse-pointer having left bubble and proximity-area
		// after inital show-up of bubble
		if (priv->prevent_fade && priv->distance > 1.0f)
			priv->prevent_fade = FALSE;
	}

	return TRUE;
}

//-- internal API --------------------------------------------------------------

static void
bubble_dispose (GObject* gobject)
{
	Bubble*        bubble;
	BubblePrivate* priv;

	if (!gobject || !IS_BUBBLE (gobject))
		return;

	bubble = BUBBLE (gobject);
	priv = bubble->priv;

	if (GTK_IS_WIDGET (priv->widget))
	{
		gtk_widget_destroy (GTK_WIDGET (priv->widget));
		priv->widget = NULL;
	}

    	if (priv->title)
	{
		g_string_free ((gpointer) priv->title,
			       TRUE);
		priv->title = NULL;
	}

    	if (priv->message_body)
	{
		g_string_free ((gpointer) priv->message_body,
			       TRUE);
		priv->message_body = NULL;
	}

    	if (priv->synchronous)
	{
		g_free ((gpointer) priv->synchronous);
		priv->synchronous = NULL;
	}

    	if (priv->sender)
	{
		g_free ((gpointer) priv->sender);
		priv->sender = NULL;
	}

	if (priv->icon_pixbuf)
	{
		g_object_unref (priv->icon_pixbuf);
		priv->icon_pixbuf = NULL;
	}

	if (priv->alpha)
	{
		g_object_unref (priv->alpha);
		priv->alpha = NULL;
	}

	if (priv->timeline)
	{
		g_object_unref (priv->timeline);
		priv->timeline = NULL;
	}

	if (priv->pointer_update_id)
	{
		g_source_remove (priv->pointer_update_id);
		priv->pointer_update_id = 0;
	}

	if (priv->draw_handler_id)
	{
		g_source_remove (priv->draw_handler_id);
		priv->draw_handler_id = 0;
	}

	if (priv->timer_id)
	{
		g_source_remove (priv->timer_id);
		priv->timer_id = 0;
	}

	if (priv->tile_background_part)
	{
		tile_destroy (priv->tile_background_part);
		priv->tile_background_part = NULL;
	}

	if (priv->tile_background)
	{
		tile_destroy (priv->tile_background);
		priv->tile_background = NULL;
	}

	if (priv->tile_icon)
	{
		tile_destroy (priv->tile_icon);
		priv->tile_icon = NULL;
	}

	if (priv->tile_title)
	{
		tile_destroy (priv->tile_title);
		priv->tile_title = NULL;
	}

	if (priv->tile_body)
	{
		tile_destroy (priv->tile_body);
		priv->tile_body = NULL;
	}

	if (priv->tile_indicator)
	{
		tile_destroy (priv->tile_indicator);
		priv->tile_indicator = NULL;
	}

	g_clear_pointer (&priv->old_icon_filename, g_free);

	// chain up to the parent class
	G_OBJECT_CLASS (bubble_parent_class)->dispose (gobject);
}

static void
bubble_finalize (GObject* gobject)
{
	// chain up to the parent class
	G_OBJECT_CLASS (bubble_parent_class)->finalize (gobject);
}

static void
bubble_init (Bubble* self)
{
	// If you need specific construction properties to complete
	// initialization, delay initialization completion until the
	// property is set.

	BubblePrivate *priv;

	self->priv = priv                = bubble_get_instance_private (self);
	priv->layout                     = LAYOUT_NONE;
	priv->title                      = NULL;
	priv->message_body               = NULL;
	priv->title_needs_refresh        = FALSE;
	priv->message_body_needs_refresh = FALSE;
	priv->visible                    = FALSE;
	priv->icon_pixbuf                = NULL;
	priv->value                      = -2;
	priv->synchronous                = NULL;
	priv->sender                     = NULL;
	priv->draw_handler_id            = 0;
	priv->pointer_update_id          = 0;
}

static void
bubble_get_property (GObject*    gobject,
		     guint       prop,
		     GValue*     value,
		     GParamSpec* spec)
{
	switch (prop)
	{
		default :
			G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop, spec);
		break;
	}
}

static void
bubble_class_init (BubbleClass* klass)
{
	GObjectClass* gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->dispose      = bubble_dispose;
	gobject_class->finalize     = bubble_finalize;
	gobject_class->get_property = bubble_get_property;

	g_bubble_signals[TIMED_OUT] = g_signal_new (
		"timed-out",
		G_OBJECT_CLASS_TYPE (gobject_class),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (BubbleClass, timed_out),
		NULL,
		NULL,
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE,
		0);

	g_bubble_signals[VALUE_CHANGED] = g_signal_new (
		"value-changed",
		G_OBJECT_CLASS_TYPE (gobject_class),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (BubbleClass, value_changed),
		NULL,
		NULL,
		g_cclosure_marshal_VOID__INT,
		G_TYPE_NONE,
		1,
        G_TYPE_INT);

    g_bubble_signals[MESSAGE_BODY_DELETED] = g_signal_new (
		"message-body-deleted",
		G_OBJECT_CLASS_TYPE (gobject_class),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (BubbleClass, message_body_deleted),
		NULL,
		NULL,
		g_cclosure_marshal_VOID__STRING,
		G_TYPE_NONE,
		1,
        G_TYPE_STRING);

    g_bubble_signals[MESSAGE_BODY_INSERTED] = g_signal_new (
		"message-body-inserted",
		G_OBJECT_CLASS_TYPE (gobject_class),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (BubbleClass, message_body_inserted),
		NULL,
		NULL,
		g_cclosure_marshal_VOID__STRING,
		G_TYPE_NONE,
		1,
        G_TYPE_STRING);
}

//-- public API ----------------------------------------------------------------

Bubble*
bubble_new (Defaults* defaults)
{
	Bubble*        this        = NULL;
	GtkWidget*     window      = NULL;
	BubblePrivate* priv;
	GdkRGBA        transparent = {0.0, 0.0, 0.0, 0.0};

	this = g_object_new (BUBBLE_TYPE, NULL);
	if (!this)
		return NULL;

	this->defaults = defaults;
	priv = this->priv;

	priv->widget = bubble_window_new();
	window = priv->widget;
	if (!window)
		return NULL;

	g_object_set_data (G_OBJECT(window), "bubble", (gpointer) this);

	gtk_window_set_type_hint (GTK_WINDOW (window),
				  GDK_WINDOW_TYPE_HINT_NOTIFICATION);
	gtk_window_set_skip_pager_hint (GTK_WINDOW (window), TRUE);
	gtk_window_set_skip_taskbar_hint (GTK_WINDOW (window), TRUE);
	gtk_window_stick (GTK_WINDOW (window));

	gtk_widget_add_events (window,
			       GDK_POINTER_MOTION_MASK |
			       GDK_BUTTON_PRESS_MASK |
			       GDK_BUTTON_RELEASE_MASK);

	// hook up input/event handlers to window
	g_signal_connect (G_OBJECT (window),
			  "screen-changed",
			  G_CALLBACK (screen_changed_handler),
			  NULL);
	g_signal_connect (G_OBJECT (window),
			  "composited-changed",
			  G_CALLBACK (composited_changed_handler),
			  this);

	gtk_window_move (GTK_WINDOW (window), 0, 0);

	// make sure the window opens with a RGBA-visual
	screen_changed_handler (GTK_WINDOW (window), NULL, NULL);
	gtk_widget_realize (window);
	gdk_window_set_background_rgba (gtk_widget_get_window (window),
									&transparent);

	// hook up window-event handlers to window
	g_signal_connect (window,
	                  "draw",
	                  G_CALLBACK (bubble_draw),
	                  this);

	// "clear" input-mask, set title/icon/attributes
	gtk_widget_set_app_paintable (window, TRUE);
	gtk_window_set_title (GTK_WINDOW (window), "notify-osd");
	gtk_window_set_decorated (GTK_WINDOW (window), FALSE);
	gtk_window_set_keep_above (GTK_WINDOW (window), TRUE);
	gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
	gtk_window_set_focus_on_map (GTK_WINDOW (window), FALSE);
	gtk_window_set_accept_focus (GTK_WINDOW (window), FALSE);
	gtk_widget_set_opacity (window, 0.0f);

	// TODO: fold some of that back into bubble_init
	this->priv = this->priv;
	this->priv->layout                     = LAYOUT_NONE;
	this->priv->widget                     = window;
	this->priv->title                      = g_string_new ("");
	this->priv->message_body               = g_string_new ("");
	this->priv->title_needs_refresh        = FALSE;
	this->priv->message_body_needs_refresh = FALSE;
	this->priv->icon_pixbuf                = NULL;
	this->priv->value                      = -2;
	this->priv->visible                    = FALSE;
	this->priv->timeout                    = 5000;
	this->priv->mouse_over                 = FALSE;
	this->priv->distance                   = 1.0f;
	this->priv->composited                 = gdk_screen_is_composited (
						gtk_widget_get_screen (window));
	this->priv->alpha                      = NULL;
	this->priv->timeline                   = NULL;
	this->priv->title_width                = 0;
	this->priv->title_height               = 0;
	this->priv->body_width                 = 0;
	this->priv->body_height                = 0;
	this->priv->append                     = FALSE;
	this->priv->icon_only                  = FALSE;
	this->priv->tile_background_part       = NULL;
	this->priv->tile_background            = NULL;
	this->priv->tile_icon                  = NULL;
	this->priv->tile_title                 = NULL;
	this->priv->tile_body                  = NULL;
	this->priv->tile_indicator             = NULL;
	this->priv->prevent_fade               = FALSE;

	update_input_shape (window);

	return this;
}

gchar*
bubble_get_synchronous (Bubble* self)
{
	g_return_val_if_fail (IS_BUBBLE (self), NULL);

	return self->priv->synchronous;
}

gchar*
bubble_get_sender (Bubble* self)
{
	g_return_val_if_fail (IS_BUBBLE (self), NULL);

	return self->priv->sender;
}

void
bubble_set_title (Bubble*      self,
		  const gchar* title)
{
	gchar*         text;
	BubblePrivate* priv;

	if (!self || !IS_BUBBLE (self))
		return;

	priv = self->priv;

	// convert any newline to space
	text = newline_to_space (title);

	if (priv->title)
	{
		if (g_strcmp0 (priv->title->str, text))
			priv->title_needs_refresh = TRUE;

		g_string_free (priv->title, TRUE);
	}

	priv->title = g_string_new (text);

	g_object_notify (
		G_OBJECT (gtk_widget_get_accessible (self->priv->widget)), 
		"accessible-name");

	g_free (text);
}

const gchar*
bubble_get_title (Bubble* self)
{
	if (!self || !IS_BUBBLE (self))
		return NULL;

	return self->priv->title->str;
}

void
bubble_set_message_body (Bubble*      self,
			 const gchar* body)
{
	gchar*         text;
	BubblePrivate* priv;

	if (!self || !IS_BUBBLE (self))
		return;

	priv = self->priv;

	// filter out any HTML/markup if possible
	text = filter_text (body);

	if (priv->message_body)
		if (g_strcmp0 (priv->message_body->str, text))
			priv->message_body_needs_refresh = TRUE;

	if (priv->message_body && priv->message_body->len != 0)
	{
		g_signal_emit (self,
			       g_bubble_signals[MESSAGE_BODY_DELETED], 
			       0,
			       priv->message_body->str);
		g_string_free (priv->message_body, TRUE);
	}

	priv->message_body = g_string_new (text);

	g_signal_emit (self, g_bubble_signals[MESSAGE_BODY_INSERTED], 0, text);
	g_object_notify (G_OBJECT (gtk_widget_get_accessible (priv->widget)), 
					 "accessible-description");

	g_free (text);
}

const gchar*
bubble_get_message_body (Bubble* self)
{
	if (!self || !IS_BUBBLE (self))
		return NULL;

	return self->priv->message_body->str;
}

void
bubble_set_icon (Bubble*      self,
		 const gchar* name)
{
	BubblePrivate *priv;
	gint           scale;
	gint	       icon_size;

	g_return_if_fail (self != NULL);
	g_return_if_fail (name != NULL);

	priv = self->priv;
	scale = gtk_widget_get_scale_factor (priv->widget);
	icon_size = EM2PIXELS (defaults_get_icon_size (self->defaults), self->defaults);

	// check if an app tries to set the same file as icon again, this check
	// avoids superfluous regeneration of the tile/blur-cache for the icon,
	// thus it improves performance in update- and append-cases
	if (!g_strcmp0 (priv->old_icon_filename, name))
		return;

	g_clear_object (&priv->icon_pixbuf);
	g_clear_pointer (&priv->old_icon_filename, g_free);

	if (g_str_has_prefix (name, "file://"))
	{
		gchar *filename;
		GError *error = NULL;

		filename = g_filename_from_uri (name, NULL, &error);
		if (filename == NULL)
		{
			g_printerr ("%s is not a valid file uri: %s", name, error->message);
			g_error_free (error);
			return;
		}

		priv->icon_pixbuf = gdk_pixbuf_new_from_file_at_scale (filename, scale * icon_size, scale * icon_size, TRUE, NULL);

		g_free (filename);
	}
	else
	{
		GError *error = NULL;
		GdkPixbuf *buffer;
		GtkIconTheme *theme;
		GtkIconLookupFlags flags;
		gchar *fallback_name;

		theme = gtk_icon_theme_get_default ();
		flags = GTK_ICON_LOOKUP_FORCE_SVG | GTK_ICON_LOOKUP_GENERIC_FALLBACK | GTK_ICON_LOOKUP_FORCE_SIZE;

		fallback_name = g_strconcat ("notification-", name, NULL);
		buffer = gtk_icon_theme_load_icon_for_scale (theme, fallback_name, icon_size, scale, flags, &error);
		g_free (fallback_name);

		if (buffer == NULL && g_error_matches (error, GTK_ICON_THEME_ERROR, GTK_ICON_THEME_NOT_FOUND)) {
			g_clear_error (&error);
			buffer = gtk_icon_theme_load_icon_for_scale (theme, name, icon_size, scale, flags, &error);
		}

		if (buffer == NULL)
		{
			g_print ("Unable to load icon '%s': %s", name, error->message);
			g_error_free (error);
			return;
		}

		/* copy and unref buffer so on an icon-theme change old
		** icons are not kept in memory due to dangling
		** references, this also makes sure we do not need to
		** connect to GtkWidget::style-set signal for the
		** GdkPixbuf we get from gtk_icon_theme_load_icon() */
		priv->icon_pixbuf = gdk_pixbuf_copy (buffer);
		g_object_unref (buffer);
	}

	// store the new icon-basename
	priv->old_icon_filename = g_strdup (name);

	_refresh_icon (self);
}

static GdkPixbuf *
scale_pixbuf (const GdkPixbuf *pixbuf, gint size)
{
	GdkPixbuf *scaled_icon;
	GdkPixbuf *new_icon;
	gint w, h, dest_x, dest_y, new_width, new_height, max_edge;

	dest_x = dest_y = 0;

	w = gdk_pixbuf_get_width (pixbuf);
	h = gdk_pixbuf_get_height (pixbuf);

	max_edge = MAX (w, h);

	// temporarily cast to float so we don't end up missing fractional parts
	// from the division, especially nasty for 0.something :)
	// e.g.: 99 / 100 = 0 but 99.0 / 100.0 = 0.99
	new_width = size * ((gfloat) w / (gfloat) max_edge);
	new_height =  size * ((gfloat) h / (gfloat) max_edge);

	/* Scale the pixbuf down, preserving the aspect ratio */
	scaled_icon = gdk_pixbuf_scale_simple (pixbuf,
					       new_width,
					       new_height,
					       GDK_INTERP_BILINEAR);

	if (w == h)
		return scaled_icon;

	/* Create a square pixbuf with an alpha channel */
	new_icon = gdk_pixbuf_new (gdk_pixbuf_get_colorspace (scaled_icon),
				   TRUE,
				   gdk_pixbuf_get_bits_per_sample (scaled_icon),
				   size, size);

	/* Clear the pixbuf so it is transparent */
	gdk_pixbuf_fill (new_icon, 0x00000000);

	/* Center the rectangular pixbuf inside the transparent square */
	if (new_width > new_height)
		dest_y = (new_width - new_height) / 2;
	else
		dest_x = (new_height - new_width) / 2;

	/* Copy the rectangular pixbuf into the new pixbuf at a centered position */
	gdk_pixbuf_copy_area (scaled_icon,
			      0, 0,
			      gdk_pixbuf_get_width (scaled_icon),
			      gdk_pixbuf_get_height (scaled_icon),
			      new_icon,
			      dest_x, dest_y);
	g_object_unref (scaled_icon);

	return new_icon;
}

void
bubble_set_icon_from_pixbuf (Bubble*    self,
			     GdkPixbuf* pixbuf)
{
	GdkPixbuf*     scaled;
	gint           height;
	gint           width;
	Defaults*      d;
	BubblePrivate* priv;

 	if (!self || !IS_BUBBLE (self) || !pixbuf)
		return;

	priv = self->priv;

	// "reset" the stored the icon-filename, fixes LP: #451086
	g_clear_pointer (&priv->old_icon_filename, g_free);

	if (priv->icon_pixbuf)
	{
		g_object_unref (priv->icon_pixbuf);
		priv->icon_pixbuf = NULL;
	}

	height = gdk_pixbuf_get_height (pixbuf);
	width = gdk_pixbuf_get_width (pixbuf);

	d = self->defaults;

	if (width != defaults_get_icon_size (d) ||
            height != defaults_get_icon_size (d))
	{
		scaled = scale_pixbuf (pixbuf, EM2PIXELS (defaults_get_icon_size (d), d));
		g_object_unref (pixbuf);
		pixbuf = scaled;
	}

	priv->icon_pixbuf = pixbuf;

	_refresh_icon (self);
}

GdkPixbuf*
bubble_get_icon_pixbuf (Bubble *self)
{
	g_return_val_if_fail (IS_BUBBLE (self), NULL);

	return self->priv->icon_pixbuf;
}

void
bubble_set_value (Bubble* self,
		  gint    value)
{
	BubblePrivate* priv;

	if (!self || !IS_BUBBLE (self))
		return;

	priv = self->priv;

	// only really cause a refresh (blur, recreating tile-/blur-cache) if
	// a different value has been set, this helps improve performance when
	// a user e.g. presses and holds down keys for Volume-Up/Down or
	// Brightness-Up/Down and apps like gnome-settings-daemon or
	// gnome-power-daemon fire continues notification-updates due to the
	// key-repeat events
	if (priv->value != value)
	{
		priv->value = value;
		g_signal_emit (self, g_bubble_signals[VALUE_CHANGED], 0, value);
		_refresh_indicator (self);
	}
}

gint
bubble_get_value (Bubble* self)
{
	if (!self || !IS_BUBBLE (self))
		return -2;

	return self->priv->value;
}

void
bubble_set_size (Bubble* self,
		 gint    width,
		 gint    height)
{
	if (!self || !IS_BUBBLE (self))
		return;

	gtk_widget_set_size_request (self->priv->widget, width, height);
}

void
bubble_get_size (Bubble* self,
		 gint*   width,
		 gint*   height)
{
	if (!self || !IS_BUBBLE (self))
		return;

	gtk_widget_get_size_request (self->priv->widget, width, height);
}

void
bubble_set_timeout (Bubble* self,
		    guint   timeout)
{
	if (!self || !IS_BUBBLE (self))
		return;

	self->priv->timeout = timeout;
}

/* a timeout of 0 doesn't make much sense now does it, thus 0 indicates an
** error */
guint
bubble_get_timeout (Bubble* self)
{
	if (!self || !IS_BUBBLE (self))
		return 0;

	return self->priv->timeout;
}

void
bubble_set_timer_id (Bubble* self,
		     guint   timer_id)
{
	if (!self || !IS_BUBBLE (self))
		return;

	self->priv->timer_id = timer_id;
}

/* a valid GLib timer-id is always > 0, thus 0 indicates an error */
guint
bubble_get_timer_id (Bubble* self)
{
	if (!self || !IS_BUBBLE (self))
		return 0;

	return self->priv->timer_id;
}

void
bubble_set_mouse_over (Bubble*  self,
		       gboolean flag)
{
	BubblePrivate* priv;

	if (!self || !IS_BUBBLE (self))
		return;

	priv = self->priv;

	/* did anything change? */
	if (priv->mouse_over != flag)
	{
		priv->mouse_over = flag;
		update_shape (self);

		if (flag) {
			g_debug("mouse entered bubble, supressing timeout");
			bubble_clear_timer(self);
		} else {
			g_debug("mouse left bubble, continuing timeout");
			bubble_set_timeout(self, 3000);
			bubble_start_timer(self, TRUE);
		}
        
	}

}

gboolean
bubble_is_mouse_over (Bubble* self)
{
	BubblePrivate* priv;

	if (!self || !IS_BUBBLE (self))
		return FALSE;

	priv = self->priv;

	if (priv->prevent_fade)
		return FALSE;

	return priv->mouse_over;
}

void
bubble_move (Bubble* self,
	     gint    x,
	     gint    y)
{
	if (!self || !IS_BUBBLE (self))
		return;

	gtk_window_move (GTK_WINDOW (self->priv->widget), x, y);
}

static void
glow_completed_cb (EggTimeline *timeline,
		   Bubble *bubble)
{
	BubblePrivate* priv;

	g_return_if_fail (IS_BUBBLE (bubble));

	priv = bubble->priv;

	/* get rid of the alpha, so that the mouse-over algorithm notices */
	if (priv->alpha)
	{
		g_object_unref (priv->alpha);
		priv->alpha = NULL;
	}

	if (priv->timeline)
	{
		g_object_unref (priv->timeline);
		priv->timeline = NULL;
	}
}

static void
glow_cb (EggTimeline *timeline,
	 gint frame_no,
	 Bubble *bubble)
{
	g_return_if_fail (IS_BUBBLE (bubble));

	bubble_refresh (bubble);
}

static void
bubble_start_glow_effect (Bubble *self,
			  guint   msecs)
{
	EggTimeline*   timeline;
	BubblePrivate* priv;

	g_return_if_fail (IS_BUBBLE (self));

	priv = self->priv;

	timeline = egg_timeline_new_for_duration (msecs);
	egg_timeline_set_speed (timeline, FPS);
	priv->timeline = timeline;

	if (priv->alpha != NULL)
	{
		g_object_unref (priv->alpha);
		priv->alpha = NULL;
	}

	priv->alpha = egg_alpha_new_full (timeline,
					  EGG_ALPHA_RAMP_DEC,
					  NULL,
					  NULL);

	g_signal_connect (G_OBJECT (timeline),
			  "completed",
			  G_CALLBACK (glow_completed_cb),
			  self);
	g_signal_connect (G_OBJECT (timeline),
			  "new-frame",
			  G_CALLBACK (glow_cb),
			  self);

	egg_timeline_start (timeline);
}

void
bubble_show (Bubble* self)
{
	BubblePrivate* priv;

	if (!self || !IS_BUBBLE (self))
		return;

	priv = self->priv;

	priv->visible = TRUE;
	gtk_widget_show_all (priv->widget);

	// check if mouse-pointer is over bubble (and proximity-area) initially
	pointer_update (self);
	if (priv->distance <= 1.0f)
		priv->prevent_fade = TRUE;
	else
		priv->prevent_fade = FALSE;

	// FIXME: do nasty busy-polling rendering in the drawing-area
	priv->draw_handler_id = g_timeout_add (1000/FPS,
					       (GSourceFunc) redraw_handler,
					       self);

	// FIXME: read out current mouse-pointer position every 1/25 second
        priv->pointer_update_id = g_timeout_add (1000/FPS,
						 (GSourceFunc) pointer_update,
						 self);
}

/* mostly called when we change the content of the bubble
   and need to force a redraw
*/
void
bubble_refresh (Bubble* self)
{
	if (!self || !IS_BUBBLE (self))
		return;

	/* force a redraw */
	gtk_widget_queue_draw (self->priv->widget);
}

static inline gboolean
bubble_is_composited (Bubble *bubble)
{
	/* no g_return_if_fail(), the caller should have already
	   checked that */
	return gtk_widget_is_composited (bubble->priv->widget);
}

static inline GtkWindow*
bubble_get_window (Bubble *bubble)
{
	return GTK_WINDOW (bubble->priv->widget);
}

static void
fade_cb (EggTimeline *timeline,
	 gint frame_no,
	 Bubble *bubble)
{
	float opacity;

	g_return_if_fail (IS_BUBBLE (bubble));

	opacity = (float)egg_alpha_get_alpha (bubble->priv->alpha)
		/ (float)EGG_ALPHA_MAX_ALPHA
		* WINDOW_MAX_OPACITY;

	if (bubble_is_mouse_over (bubble))
		gtk_widget_set_opacity (bubble->priv->widget,
		                        WINDOW_MIN_OPACITY);
	else
		gtk_widget_set_opacity (bubble->priv->widget, opacity);
}

static void
fade_out_completed_cb (EggTimeline *timeline,
		       Bubble *bubble)
{
	g_return_if_fail (IS_BUBBLE (bubble));

	bubble_hide (bubble);

	dbus_send_close_signal (bubble_get_sender (bubble),
				bubble_get_id (bubble),
				1);
	g_signal_emit (bubble, g_bubble_signals[TIMED_OUT], 0);
}


static void
fade_in_completed_cb (EggTimeline* timeline,
		      Bubble*      bubble)
{
	BubblePrivate* priv;

	g_return_if_fail (IS_BUBBLE (bubble));

	priv = bubble->priv;

	/* get rid of the alpha, so that the mouse-over algorithm notices */
	if (priv->alpha)
	{
		g_object_unref (priv->alpha);
		priv->alpha = NULL;
	}

	if (priv->timeline)
	{
		g_object_unref (priv->timeline);
		priv->timeline = NULL;
	}

	if (bubble_is_mouse_over (bubble))
		gtk_widget_set_opacity (bubble->priv->widget,
		                        WINDOW_MIN_OPACITY);
	else
		gtk_widget_set_opacity (bubble->priv->widget,
		                        WINDOW_MAX_OPACITY);

	bubble_start_timer (bubble, TRUE);
}

void
bubble_fade_in (Bubble* self,
		guint   msecs)
{
	EggTimeline*   timeline;
	BubblePrivate* priv;

	g_return_if_fail (IS_BUBBLE (self));

	priv = self->priv;

	if (!bubble_is_composited (self)
	    || msecs == 0)
	{
		bubble_show (self);
		bubble_start_timer (self, TRUE);
		return;
	}

	timeline = egg_timeline_new_for_duration (msecs);
	egg_timeline_set_speed (timeline, FPS);

	if (priv->alpha != NULL)
	{
		g_object_unref (priv->alpha);
		priv->alpha = NULL;
	}

	priv->alpha = egg_alpha_new_full (timeline,
					  EGG_ALPHA_RAMP_INC,
					  NULL,
					  NULL);

	g_signal_connect (G_OBJECT (timeline),
			  "completed",
			  G_CALLBACK (fade_in_completed_cb),
			  self);

	g_signal_connect (G_OBJECT (timeline),
			  "new-frame",
			  G_CALLBACK (fade_cb),
			  self);

	egg_timeline_start (timeline);

	gtk_widget_set_opacity (self->priv->widget, 0.0f);

	bubble_show (self);
}

void
bubble_fade_out (Bubble* self,
		 guint   msecs)
{
	EggTimeline*   timeline;
	BubblePrivate* priv;

	g_return_if_fail (IS_BUBBLE (self));

	priv = self->priv;

	timeline = egg_timeline_new_for_duration (msecs);
	egg_timeline_set_speed (timeline, FPS);
	priv->timeline = timeline;

	if (priv->alpha != NULL)
	{
		g_object_unref (priv->alpha);
		priv->alpha = NULL;
	}

	priv->alpha = egg_alpha_new_full (timeline,
					  EGG_ALPHA_RAMP_DEC,
					  NULL,
					  NULL);

	g_signal_connect (G_OBJECT (timeline),
			  "completed",
			  G_CALLBACK (fade_out_completed_cb),
			  self);
	g_signal_connect (G_OBJECT (timeline),
			  "new-frame",
			  G_CALLBACK (fade_cb),
			  self);

	egg_timeline_start (timeline);
}

gboolean
bubble_timed_out (Bubble* self)
{
	g_return_val_if_fail (IS_BUBBLE (self), FALSE);

	/* This function always returns FALSE, which removes the timer
	 * source. Unset priv->timer_id so that we don't call
	 * g_source_remove() on it later. */
	bubble_set_timer_id (self, 0);

	if (self->priv->composited)
	{
		bubble_fade_out (self, 300);
		return FALSE;
	}

	bubble_hide (self);

	dbus_send_close_signal (bubble_get_sender (self),
				bubble_get_id (self),
				1);
	g_signal_emit (self, g_bubble_signals[TIMED_OUT], 0);

	return FALSE;
}


void
bubble_hide (Bubble* self)
{
	BubblePrivate* priv;

	if (!self || !IS_BUBBLE (self) || !bubble_is_visible (self))
		return;

	priv = self->priv;

	priv->visible = FALSE;
	gtk_widget_hide (priv->widget);

	priv->timeout = 0;

	if (priv->timeline)
	{
		egg_timeline_stop (priv->timeline);
		g_object_unref (priv->timeline);
		priv->timeline = NULL;
	}

	if (priv->alpha)
	{
		g_object_unref (priv->alpha);
		priv->alpha = NULL;
	}
}

void
bubble_set_id (Bubble* self,
	       guint   id)
{
	if (!self || !IS_BUBBLE (self))
		return;

	self->priv->id = id;
}

guint
bubble_get_id (Bubble* self)
{
	if (!self || !IS_BUBBLE (self))
		return 0;

	return self->priv->id;
}

gboolean
bubble_is_visible (Bubble* self)
{
	if (!self || !IS_BUBBLE (self))
		return FALSE;

	return self->priv->visible;
}

void
bubble_start_timer (Bubble*  self,
		    gboolean trigger)
{
	guint          timer_id;
	BubblePrivate* priv;

	if (!self || !IS_BUBBLE (self))
		return;

	priv = self->priv;

	timer_id = bubble_get_timer_id (self);
	if (timer_id > 0)
		g_source_remove (timer_id);

	/* and now let the timer tick... we use g_timeout_add_seconds() here
	** because for the bubble timeouts microsecond-precise resolution is not
	** needed, furthermore this also allows glib more room for optimizations
	** and improve system-power-usage to be more efficient */
	bubble_set_timer_id (
		self,
		g_timeout_add_seconds (bubble_get_timeout (self) / 1000,
				       (GSourceFunc) bubble_timed_out,
				       self));

	/* if the bubble is displaying a value that is out of bounds
	   trigger a dim/glow animation */
	if (trigger)
		if (priv->value == -1 || priv->value == 101)
			bubble_start_glow_effect (self, 500);
}

void
bubble_clear_timer (Bubble* self) 
{
	guint timer_id;

	if (!self || !IS_BUBBLE (self))
		return;
    
	timer_id = self->priv->timer_id;

	if (timer_id > 0) {
		g_source_remove (timer_id);
		bubble_set_timer_id(self, 0);
	}
}

void
bubble_get_position (Bubble* self,
		     gint*   x,
		     gint*   y)
{
	if (!self || !IS_BUBBLE (self))
		return;

	gtk_window_get_position (GTK_WINDOW (self->priv->widget),
				 x, y);
}

gint
bubble_get_height (Bubble *self)
{
	gint width;
	gint height;

	if (!self || !IS_BUBBLE (self))
		return 0;

	gtk_window_get_size (GTK_WINDOW (self->priv->widget),
			     &width,
			     &height);

	return height;
}

gint
bubble_get_future_height (Bubble *self)
{
	if (!self || !IS_BUBBLE (self))
		return 0;

	return self->priv->future_height;
}

gint
_calc_title_height (Bubble* self,
		    gint    title_width /* requested text-width in pixels */)
{
	Defaults*             d;
	cairo_surface_t*      surface;
	cairo_t*              cr;
	PangoFontDescription* desc    = NULL;
	PangoLayout*          layout  = NULL;
	PangoRectangle        log_rect = {0, 0, 0, 0};
	gint                  title_height;
	BubblePrivate*        priv;
	gchar*                text_font_face = NULL;

	if (!self || !IS_BUBBLE (self))
		return 0;

	d    = self->defaults;
	priv = self->priv;

	surface = bubble_create_image_surface (self, CAIRO_FORMAT_A1, 1, 1);
	if (cairo_surface_status (surface) != CAIRO_STATUS_SUCCESS) {
		if (surface)
			cairo_surface_destroy (surface);
		return 0;
	}

	cr = cairo_create (surface);
	cairo_surface_destroy (surface);
	if (cairo_status (cr) != CAIRO_STATUS_SUCCESS) {
		if (cr)
			cairo_destroy (cr);
		return 0;
	}

	layout = pango_cairo_create_layout (cr);
	text_font_face = defaults_get_text_font_face (d);
	desc = pango_font_description_from_string (text_font_face);
	g_free ((gpointer) text_font_face);

	// make sure system-wide font-options like hinting, antialiasing etc.
	// are taken into account
	pango_cairo_context_set_font_options (
		pango_layout_get_context (layout),
		gdk_screen_get_font_options (
			gtk_widget_get_screen (priv->widget)));
	pango_cairo_context_set_resolution  (pango_layout_get_context (layout),
					     defaults_get_screen_dpi (d));
	pango_layout_context_changed (layout);

	pango_font_description_set_size (desc,
					 defaults_get_system_font_size (d) *
					 defaults_get_text_title_size (d) *
					 PANGO_SCALE);

	pango_font_description_set_weight (
		desc,
		defaults_get_text_title_weight (d));

	pango_layout_set_font_description (layout, desc);
	pango_font_description_free (desc);

	pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);
	pango_layout_set_wrap (layout, PANGO_WRAP_WORD_CHAR);
	pango_layout_set_width (layout, title_width * PANGO_SCALE);
	pango_layout_set_height (layout, -3);

	pango_layout_set_text (layout, priv->title->str, priv->title->len);

	pango_layout_get_extents (layout, NULL, &log_rect);
	title_height = PANGO_PIXELS (log_rect.height);
	g_object_unref (layout);
	cairo_destroy (cr);

	return title_height;
}

gint
_calc_body_height (Bubble* self,
		   gint    body_width /* requested text-width in pixels */)
{
	Defaults*             d;
	cairo_t*              cr;
	PangoFontDescription* desc    = NULL;
	PangoLayout*          layout  = NULL;
	PangoRectangle        log_rect;
	gint                  body_height;
	BubblePrivate*        priv;
	gchar*                text_font_face = NULL;

	if (!self || !IS_BUBBLE (self))
		return 0;

	d    = self->defaults;
	priv = self->priv;

	cr = gdk_cairo_create (gtk_widget_get_window (priv->widget));
	if (cairo_status (cr) != CAIRO_STATUS_SUCCESS) {
		if (cr)
			cairo_destroy (cr);
		return 0;
	}

	layout = pango_cairo_create_layout (cr);
	text_font_face = defaults_get_text_font_face (d);
	desc = pango_font_description_from_string (text_font_face);
	g_free ((gpointer) text_font_face);

	// make sure system-wide font-options like hinting, antialiasing etc.
	// are taken into account
	pango_cairo_context_set_font_options (
		pango_layout_get_context (layout),
		gdk_screen_get_font_options (
			gtk_widget_get_screen (priv->widget)));
	pango_cairo_context_set_resolution  (pango_layout_get_context (layout),
					     defaults_get_screen_dpi (d));
	pango_layout_context_changed (layout);

	pango_font_description_set_size (desc,
					 defaults_get_system_font_size (d) *
					 defaults_get_text_body_size (d) *
					 PANGO_SCALE);

	pango_font_description_set_weight (
		desc,
		defaults_get_text_body_weight (d));

	pango_layout_set_font_description (layout, desc);

	pango_layout_set_wrap (layout, PANGO_WRAP_WORD_CHAR);
	pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_MIDDLE);
	pango_layout_set_width (layout, body_width * PANGO_SCALE);
	pango_layout_set_height (layout, -10);

	pango_layout_set_text (layout,
			       priv->message_body->str,
			       priv->message_body->len);

	/* enforce the 10 line-limit, usually only triggered by appended
	** body-message text due to the newline-characters added with each
	** append */
	if (pango_layout_get_line_count (layout) > 10)
	{
		/* get it down to 9 lines, because we add our own ...-line */
		while (pango_layout_get_line_count (layout) > 9)
		{
			GString* string = NULL;

			/* cut leading chunk of text up to the first newline */
			string = g_string_new (g_strstr_len (priv->message_body->str,
							     priv->message_body->len,
							     "\n"));

			/* also cut the first newline itself */
			string = g_string_erase (string, 0, 1);

			/* copy that stripped text back to the body-message */
			g_string_assign (priv->message_body, string->str);

			/* set the new body-message text to the pango-layout */
			pango_layout_set_text (layout,
					       priv->message_body->str,
					       priv->message_body->len);

			/* clean up */
			g_string_free (string, TRUE);
		}

		/* add our own ellipsize-line */
		g_string_prepend (priv->message_body, "...\n");

		/* set final body-message text to the pango-layout again */
		pango_layout_set_text (layout,
				       priv->message_body->str,
				       priv->message_body->len);
	}

	pango_layout_get_extents (layout, NULL, &log_rect);
	body_height = PANGO_PIXELS (log_rect.height);

	pango_font_description_free (desc);
	g_object_unref (layout);
	cairo_destroy (cr);

	return body_height;
}

void
bubble_recalc_size (Bubble *self)
{
	gint           old_bubble_width  = 0;
	gint           old_bubble_height = 0;
	gint           new_bubble_width  = 0;
	gint           new_bubble_height = 0;
	Defaults*      d;
	BubblePrivate* priv;

	if (!self || !IS_BUBBLE (self))
		return;

	d    = self->defaults;
	priv = self->priv;

	/* FIXME: a quick fix to rescale an icon (e.g. user changed font-size or
	** DPI while a bubble is displayed, thus bubble is re-rendered and the
	** icon needs to adapt to the new size) */
	if (priv->icon_pixbuf)
	{
		GdkPixbuf *pixbuf;
		pixbuf = gdk_pixbuf_scale_simple (
					priv->icon_pixbuf,
					EM2PIXELS (defaults_get_icon_size (d), d),
					EM2PIXELS (defaults_get_icon_size (d), d),
					GDK_INTERP_BILINEAR);
		g_object_unref (priv->icon_pixbuf);
		priv->icon_pixbuf = pixbuf;
	}

	bubble_determine_layout (self);

	new_bubble_width =
		EM2PIXELS (defaults_get_bubble_width (d), d) +
		2 * EM2PIXELS (get_shadow_size (self), d);

	switch (priv->layout)
	{
		case LAYOUT_ICON_ONLY:
		case LAYOUT_ICON_INDICATOR:
		case LAYOUT_ICON_TITLE:
			priv->title_width =
				EM2PIXELS (defaults_get_bubble_width (d), d) -
				3 * EM2PIXELS (defaults_get_margin_size (d), d) -
				EM2PIXELS (defaults_get_icon_size (d), d);

			priv->title_height = _calc_title_height (
					self,
					self->priv->title_width);

			new_bubble_height =
				EM2PIXELS (defaults_get_bubble_min_height (d), d) +
				2.0f * EM2PIXELS (get_shadow_size (self), d);
		break;

		case LAYOUT_ICON_TITLE_BODY:
		{
			gdouble available_height = 0.0f;
			gdouble bubble_height    = 0.0f;

			priv->title_width =
				EM2PIXELS (defaults_get_bubble_width (d), d) -
				3 * EM2PIXELS (defaults_get_margin_size (d), d) -
				EM2PIXELS (defaults_get_icon_size (d), d);

			priv->title_height = _calc_title_height (
					self,
					self->priv->title_width);

			priv->body_width =
				EM2PIXELS (defaults_get_bubble_width (d), d) -
				3 * EM2PIXELS (defaults_get_margin_size (d), d) -
				EM2PIXELS (defaults_get_icon_size (d), d);

			priv->body_height = _calc_body_height (
					self,
					priv->body_width);

			available_height = PIXELS2EM (defaults_get_desktop_height (d), d) -
					   defaults_get_desktop_bottom_gap (d) -
					   defaults_get_bubble_min_height (d) -
					   2.0f * defaults_get_bubble_vert_gap (d);

			bubble_height =
				PIXELS2EM (priv->title_height, d) +
				PIXELS2EM (priv->body_height, d) +
				2.0f * defaults_get_margin_size (d);

			if (bubble_height >= available_height)
			{
				new_bubble_height = EM2PIXELS (available_height, d);
			}
			else
			{
				if (priv->body_height +
				    priv->title_height <
				    EM2PIXELS (defaults_get_icon_size (d), d))
				{
					new_bubble_height =
						EM2PIXELS (defaults_get_icon_size (d), d) +
						2.0f * EM2PIXELS (defaults_get_margin_size (d), d) +
						2.0f * EM2PIXELS (get_shadow_size (self), d);
				}
				else
				{
					new_bubble_height =
						priv->body_height +
						priv->title_height +
						2.0f * EM2PIXELS (defaults_get_margin_size (d), d) +
						2.0f * EM2PIXELS (get_shadow_size (self), d);
				}
			}
		}
		break;

		case LAYOUT_TITLE_BODY:
		{
			gdouble available_height = 0.0f;
			gdouble bubble_height    = 0.0f;

			priv->title_width =
				EM2PIXELS (defaults_get_bubble_width (d), d) -
				2 * EM2PIXELS (defaults_get_margin_size (d), d);

			priv->title_height = _calc_title_height (
					self,
					priv->title_width);

			priv->body_width =
				EM2PIXELS (defaults_get_bubble_width (d), d) -
				2 * EM2PIXELS (defaults_get_margin_size (d), d);

			priv->body_height = _calc_body_height (
					self,
					priv->body_width);

			available_height = PIXELS2EM (defaults_get_desktop_height (d), d) -
					   defaults_get_desktop_bottom_gap (d) -
					   defaults_get_bubble_min_height (d) -
					   2.0f * defaults_get_bubble_vert_gap (d);

			bubble_height =
				PIXELS2EM (priv->title_height, d) +
				PIXELS2EM (priv->body_height, d) +
				2.0f * defaults_get_margin_size (d);

			if (bubble_height >= available_height)
			{
				new_bubble_height = EM2PIXELS (available_height, d);
			}
			else
			{
				new_bubble_height =
					priv->body_height +
					priv->title_height +
					2.0f * EM2PIXELS (defaults_get_margin_size (d), d) +
					2.0f * EM2PIXELS (get_shadow_size (self), d);
			}
		}
		break;

		case LAYOUT_TITLE_ONLY:
		{
			priv->title_width =
				EM2PIXELS (defaults_get_bubble_width (d), d) -
				2 * EM2PIXELS (defaults_get_margin_size (d), d);

			priv->title_height = _calc_title_height (
					self,
					priv->title_width);

			new_bubble_height = priv->title_height +
				2.0f * EM2PIXELS (defaults_get_margin_size (d), d) +
				2.0f * EM2PIXELS (get_shadow_size (self), d);
		}
		break;

		case LAYOUT_NONE:
			g_warning ("bubble_recalc_size(): WARNING, no layout!!!\n");
		break;
	}
	priv->future_height = new_bubble_height;

	bubble_get_size (self, &old_bubble_width, &old_bubble_height);
	bubble_set_size (self, new_bubble_width, new_bubble_height);

	// don't refresh tile/blur-caches for background, title or body if the
	// size or contents have not changed, this improves performance for the
	// update- and replace-cases
	if (old_bubble_width != new_bubble_width ||
	    old_bubble_height != new_bubble_height)
		_refresh_background (self);

	if (priv->title_needs_refresh)
		_refresh_title (self);

	if (priv->message_body_needs_refresh)
		_refresh_body (self);

	update_shape (self);
}

void
bubble_set_synchronous (Bubble *self,
			const gchar *sync)
{
	BubblePrivate* priv;

	g_return_if_fail (IS_BUBBLE (self));

	priv = self->priv;

	if (priv->synchronous != NULL)
		g_free (priv->synchronous);

	priv->synchronous = g_strdup (sync);
}

void
bubble_set_sender (Bubble *self,
		   const gchar *sender)
{
	BubblePrivate* priv;

	g_return_if_fail (IS_BUBBLE (self));

	priv = self->priv;

	if (priv->sender != NULL)
		g_free (priv->sender);

	priv->sender = g_strdup (sender);
}

gboolean
bubble_is_synchronous (Bubble *self)
{
	if (!self || !IS_BUBBLE (self))
		return FALSE;

	return (self->priv->synchronous != NULL);
}

gboolean
bubble_is_urgent (Bubble *self)
{
	g_return_val_if_fail (IS_BUBBLE (self), FALSE);

	return (self->priv->urgency == 2);
}

guint
bubble_get_urgency (Bubble *self)
{
	g_return_val_if_fail (IS_BUBBLE (self), 0);

	return self->priv->urgency;
}

void
bubble_set_urgency (Bubble *self,
		   guint  urgency)
{
	g_return_if_fail (IS_BUBBLE (self));

	self->priv->urgency = urgency;
}

void
bubble_determine_layout (Bubble* self)
{
	BubblePrivate* priv;

	/* sanity test */
	if (!self || !IS_BUBBLE (self))
		return;

	priv = self->priv;

	/* set a sane default */
	priv->layout = LAYOUT_NONE;

	/* icon-only layout-case, e.g. eject */
	if (priv->icon_only && priv->icon_pixbuf != NULL)
	{
		priv->layout = LAYOUT_ICON_ONLY;
		return;
	}

	/* icon + indicator layout-case, e.g. volume */
	if ((priv->icon_pixbuf       != NULL) &&
	    (priv->title->len        != 0) &&
	    (priv->message_body->len == 0) &&
	    (priv->value             >= -1))
	{
		priv->layout = LAYOUT_ICON_INDICATOR;
		return;
	}

	/* icon + title layout-case, e.g. "Wifi signal lost" */
	if ((priv->icon_pixbuf       != NULL) &&
	    (priv->title->len        != 0) &&
	    (priv->message_body->len == 0) &&
	    (priv->value             == -2))
	{
		priv->layout = LAYOUT_ICON_TITLE;
		return;
	}

	/* icon/avatar + title + body/message layout-case, e.g. IM-message */
	if ((priv->icon_pixbuf       != NULL) &&
	    (priv->title->len        != 0) &&
	    (priv->message_body->len != 0) &&
	    (priv->value             == -2))
	{
		priv->layout = LAYOUT_ICON_TITLE_BODY;
		return;
	}

	/* title + body/message layout-case, e.g. IM-message without avatar */
	if ((priv->icon_pixbuf       == NULL) &&
	    (priv->title->len        != 0) &&
	    (priv->message_body->len != 0) &&
	    (priv->value             == -2))
	{
		priv->layout = LAYOUT_TITLE_BODY;
		return;
	}

	/* title-only layout-case, use discouraged but needs to be supported */
	if ((priv->icon_pixbuf       == NULL) &&
	    (priv->title->len        != 0) &&
	    (priv->message_body->len == 0) &&
	    (priv->value             == -2))
	{
		priv->layout = LAYOUT_TITLE_ONLY;
		return;
	}

	return;
}

BubbleLayout
bubble_get_layout (Bubble* self)
{
	if (!self || !IS_BUBBLE (self))
		return LAYOUT_NONE;

	return self->priv->layout;
}

void
bubble_set_icon_only (Bubble*  self,
		      gboolean allowed)
{
	if (!self || !IS_BUBBLE (self))
		return;

	self->priv->icon_only = allowed;
}

void
bubble_set_append (Bubble*  self,
		   gboolean allowed)
{
	if (!self || !IS_BUBBLE (self))
		return;

	self->priv->append = allowed;
}


gboolean
bubble_is_append_allowed (Bubble* self)
{
	if (!self || !IS_BUBBLE (self))
		return FALSE;

	return self->priv->append;
}

void
bubble_append_message_body (Bubble*      self,
			    const gchar* append_body)
{
	gboolean       result = FALSE;
	gchar*         text   = NULL;
	GError*        error  = NULL;
	BubblePrivate* priv   = NULL;

	if (!self || !IS_BUBBLE (self) || !append_body)
		return;

	priv = self->priv;

	// filter out any HTML/markup if possible
    	result = pango_parse_markup (append_body,
				     -1,
				     0,    // no accel-marker needed
				     NULL, // no PangoAttr needed
				     &text,
				     NULL, // no accel-marker-return needed
				     &error);
	if (error && !result)
	{
		g_warning ("%s(): Got error \"%s\"\n",
			   G_STRFUNC,
			   error->message);
		g_error_free (error);
		error = NULL;

		if (text)
			g_free (text);
	}

	if (text)
	{
		// append text to current message-body
		g_string_append (priv->message_body, text);
		priv->message_body_needs_refresh = TRUE;

		g_signal_emit (self,
			       g_bubble_signals[MESSAGE_BODY_INSERTED],
			       0,
			       text);

		g_object_notify (
			G_OBJECT (gtk_widget_get_accessible (priv->widget)),
				  "accessible-description");

		g_free (text);
	}
}

void
bubble_sync_with (Bubble *self,
		  Bubble *other)
{
	g_return_if_fail (IS_BUBBLE (self) && IS_BUBBLE (other));

	bubble_set_timeout (self,
			    bubble_get_timeout (other));
	bubble_start_timer (self, FALSE);
	bubble_start_timer (other, FALSE);
}
