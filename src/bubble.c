/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
**      10        20        30        40        50        60        70        80
**
** project:
**    alsdorf
**
** file:
**    bubble.c
**
** author(s):
**    Mirco "MacSlow" Mueller <mirco.mueller@canonical.com>
**    David Barth <david.barth@canonical.com>
**
** copyright (C) Canonical, oct. 2008
**
*******************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <pixman.h>
#include <math.h>

#include "bubble.h"
#include "defaults.h"
#include "stack.h"

G_DEFINE_TYPE (Bubble, bubble, G_TYPE_OBJECT);

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BUBBLE_TYPE, BubblePrivate))

struct _BubblePrivate {
	GtkWidget* widget;
	gchar*     title;
	gchar*     message_body;
	guint      id;
	GdkPixbuf* icon_pixbuf;
	gboolean   visible;
	guint      timer_id;
	guint      timeout;
	gboolean   mouse_over;
	gint       start_y;
	gint       end_y;
	gint       delta_y;
	gdouble    inc_factor;
	gint       value; /* "empty": -1, valid range: 0 - 100 */
};

enum
{
	TIMED_OUT,
	LAST_SIGNAL
};

enum
{
	R = 0,
	G,
	B,
	A
};

/* FIXME: this is in class Defaults already, but not yet hooked up so for the
 * moment we use the macros here, these values reflect the visual-guideline
 * for jaunty notifications */
#define TEXT_TITLE_COLOR_R 1.0f
#define TEXT_TITLE_COLOR_G 1.0f
#define TEXT_TITLE_COLOR_B 1.0f
#define TEXT_TITLE_COLOR_A 1.0f

#define TEXT_BODY_COLOR_R  0.91f
#define TEXT_BODY_COLOR_G  0.91f
#define TEXT_BODY_COLOR_B  0.91f
#define TEXT_BODY_COLOR_A  1.0f

#define BUBBLE_BG_COLOR_R  0.07f
#define BUBBLE_BG_COLOR_G  0.07f
#define BUBBLE_BG_COLOR_B  0.07f
#define BUBBLE_BG_COLOR_A  0.8f

#define INDICATOR_UNLIT_R  1.0f
#define INDICATOR_UNLIT_G  1.0f
#define INDICATOR_UNLIT_B  1.0f
#define INDICATOR_UNLIT_A  0.3f

#define INDICATOR_LIT_R    1.0f
#define INDICATOR_LIT_G    1.0f
#define INDICATOR_LIT_B    1.0f
#define INDICATOR_LIT_A    1.0f

/*-- private functions  --------------------------------------------------------------*/

static guint g_bubble_signals[LAST_SIGNAL] = { 0 };
gint         g_pointer[2];

static void
draw_round_rect (cairo_t* cr,
		 gdouble   aspect,         /* aspect-ratio            */
		 gdouble   x,              /* top-left corner         */
		 gdouble   y,              /* top-left corner         */
		 gdouble   corner_radius,  /* "size" of the corners   */
		 gdouble   width,          /* width of the rectangle  */
		 gdouble   height          /* height of the rectangle */)
{
	gdouble radius = corner_radius / aspect;

	/* top-left, right of the corner */
	cairo_move_to (cr, x + radius, y);

	/* top-right, left of the corner */
	cairo_line_to (cr,
		       x + width - radius,
		       y);

	/* top-right, below the corner */
	cairo_arc (cr,
                   x + width - radius,
                   y + radius,
                   radius,
                   -90.0f * G_PI / 180.0f,
                   0.0f * G_PI / 180.0f);

	/* bottom-right, above the corner */
	cairo_line_to (cr,
		       x + width,
		       y + height - radius);

	/* bottom-right, left of the corner */
	cairo_arc (cr,
                   x + width - radius,
                   y + height - radius,
                   radius,
                   0.0f * G_PI / 180.0f,
                   90.0f * G_PI / 180.0f);

	/* bottom-left, right of the corner */
	cairo_line_to (cr,
		       x + radius,
		       y + height);

	/* bottom-left, above the corner */
	cairo_arc (cr,
                   x + radius,
                   y + height - radius,
                   radius,
                   90.0f * G_PI / 180.0f,
                   180.0f * G_PI / 180.0f);

	/* top-left, below the corner */
	cairo_line_to (cr,
		       x,
		       y + radius);

	/* top-left, right of the corner */
	cairo_arc (cr,
                   x + radius,
                   y + radius,
                   radius,
                   180.0f * G_PI / 180.0f,
                   270.0f * G_PI / 180.0f);
}

static
void
screen_changed_handler (GtkWidget* window,
			GdkScreen* old_screen,
			gpointer   data)
{                       
	GdkScreen*   screen   = gtk_widget_get_screen (window);
	GdkColormap* colormap = gdk_screen_get_rgba_colormap (screen);
      
	if (!colormap)
		colormap = gdk_screen_get_rgb_colormap (screen);

	gtk_widget_set_colormap (window, colormap);
}

static
void
update_input_shape (GtkWidget* window,
		    gint       width,
		    gint       height)
{
	GdkBitmap* mask = NULL;
	cairo_t*   cr   = NULL;

	mask = (GdkBitmap*) gdk_pixmap_new (NULL, width, height, 1);
	if (mask)
	{
		cr = gdk_cairo_create (mask);
		if (cairo_status (cr) == CAIRO_STATUS_SUCCESS)
		{
			cairo_scale (cr, 1.0f, 1.0f);
			cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
			cairo_paint (cr);
			cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
			cairo_set_source_rgb (cr, 1.0f, 1.0f, 1.0f);

			/* just draw something */
			draw_round_rect (cr,
					 1.0f,
					 0.0f, 0.0f,
					 10.0f,
					 10.0f, 10.0f);

			cairo_fill (cr);

			cairo_destroy (cr);

			gtk_widget_input_shape_combine_mask (window,
							     NULL,
							     0,
							     0);
			gtk_widget_input_shape_combine_mask (window,
							     mask,
							     0,
							     0);
		}

		g_object_unref ((gpointer) mask);
	}
}

static pixman_fixed_t*
create_gaussian_blur_kernel (gint    radius,
                             gdouble sigma,
                             gint*   length)
{
	const gdouble   scale2 = 2.0f * sigma * sigma;
	const gdouble   scale1 = 1.0f / (G_PI * scale2);
	const gint      size = 2 * radius + 1;
	const gint      n_params = size * size;
	pixman_fixed_t* params;
	gdouble*        tmp;
	gdouble         sum;
	gint            x;
	gint            y;
	gint            i;

        tmp = g_newa (double, n_params);

        /* caluclate gaussian kernel in floating point format */
        for (i = 0, sum = 0, x = -radius; x <= radius; ++x) {
                for (y = -radius; y <= radius; ++y, ++i) {
                        const gdouble u = x * x;
                        const gdouble v = y * y;

                        tmp[i] = scale1 * exp (-(u+v)/scale2);

                        sum += tmp[i];
                }
        }

        /* normalize gaussian kernel and convert to fixed point format */
        params = g_new (pixman_fixed_t, n_params + 2);

        params[0] = pixman_int_to_fixed (size);
        params[1] = pixman_int_to_fixed (size);

        for (i = 0; i < n_params; ++i)
                params[2 + i] = pixman_double_to_fixed (tmp[i] / sum);

        if (length)
                *length = n_params + 2;

        return params;
}

static cairo_surface_t*
blur_image_surface (cairo_surface_t* surface,
                    gint             radius,
                    gdouble          sigma /* pass 0.0f for auto-calculation */) 
{
        static cairo_user_data_key_t data_key;
        pixman_fixed_t*              params = NULL;
        gint                         n_params;
        pixman_image_t*              src;
	pixman_image_t*              dst;
        gint                         w;
        gint                         h;
        gint                         s;
        gpointer                     p;
	gdouble                      radiusf;

        if (cairo_surface_get_type (surface) != CAIRO_SURFACE_TYPE_IMAGE)
		return NULL;

	radiusf = fabs (radius) + 1.0f;
	if (sigma == 0.0f)
		sigma = sqrt (-(radiusf * radiusf) / (2.0f * log (1.0f / 255.0f)));

        w = cairo_image_surface_get_width (surface);
        h = cairo_image_surface_get_height (surface);
        s = cairo_image_surface_get_stride (surface);

        /* create pixman image for cairo image surface */
        p = cairo_image_surface_get_data (surface);
        src = pixman_image_create_bits (PIXMAN_a8r8g8b8, w, h, p, s);

        /* attach gaussian kernel to pixman image */
        params = create_gaussian_blur_kernel (radius, sigma, &n_params);
        pixman_image_set_filter (src,
				 PIXMAN_FILTER_CONVOLUTION,
				 params,
				 n_params);
        g_free (params);

        /* render blured image to new pixman image */
        p = g_malloc0 (s * h);
        dst = pixman_image_create_bits (PIXMAN_a8r8g8b8, w, h, p, s);
        pixman_image_composite (PIXMAN_OP_SRC,
				src,
				NULL,
				dst,
				0,
				0,
				0,
				0,
				0,
				0,
				w,
				h);
        pixman_image_unref (src);

        /* create new cairo image for blured pixman image */
        surface = cairo_image_surface_create_for_data (p,
						       CAIRO_FORMAT_ARGB32,
						       w,
						       h,
						       s);
        cairo_surface_set_user_data (surface, &data_key, p, g_free);
        pixman_image_unref (dst);

        return surface;
}

void
draw_shadow (cairo_t* cr,
	     gdouble  width,
	     gdouble  height,
	     gint     shadow_radius)
{
	cairo_surface_t* tmp_surface = NULL;
	cairo_surface_t* new_surface = NULL;
	cairo_pattern_t* pattern     = NULL;
	cairo_t*         cr_surf     = NULL;
	cairo_matrix_t   matrix;

	tmp_surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
						  4 * shadow_radius,
						  4 * shadow_radius);
	if (cairo_surface_status (tmp_surface) != CAIRO_STATUS_SUCCESS)
		return;

	cr_surf = cairo_create (tmp_surface);
	if (cairo_status (cr_surf) != CAIRO_STATUS_SUCCESS)
	{
		cairo_surface_destroy (tmp_surface);
		return;
	}

	cairo_scale (cr_surf, 1.0f, 1.0f);
	cairo_set_operator (cr_surf, CAIRO_OPERATOR_CLEAR);
	cairo_paint (cr_surf);
	cairo_set_operator (cr_surf, CAIRO_OPERATOR_OVER);
	cairo_set_source_rgba (cr_surf, 0.0f, 0.0f, 0.0f, 0.85f);
	cairo_arc (cr_surf,
		   2 * shadow_radius,
		   2 * shadow_radius,
		   1.25f * shadow_radius,
		   0.0f,
		   360.0f * (G_PI / 180.f));
	cairo_fill (cr_surf);
	cairo_destroy (cr_surf);
	tmp_surface = blur_image_surface (tmp_surface, shadow_radius, 0.0f);
	new_surface = cairo_image_surface_create_for_data (
			cairo_image_surface_get_data (tmp_surface),
			cairo_image_surface_get_format (tmp_surface),
			cairo_image_surface_get_width (tmp_surface) / 2,
			cairo_image_surface_get_height (tmp_surface) / 2,
			cairo_image_surface_get_stride (tmp_surface));
	pattern = cairo_pattern_create_for_surface (new_surface);
	if (cairo_pattern_status (pattern) != CAIRO_STATUS_SUCCESS)
	{
		cairo_surface_destroy (tmp_surface);
		cairo_surface_destroy (new_surface);
		return;
	}

	/* top left */
	cairo_pattern_set_extend (pattern, CAIRO_EXTEND_PAD);
	cairo_set_source (cr, pattern);
	cairo_rectangle (cr,
			 0.0f,
			 0.0f,
			 width - 2 * shadow_radius,
			 2 * shadow_radius);
	cairo_fill (cr);

	/* bottom left */
	cairo_matrix_init_rotate (&matrix, (G_PI / 180.0f) * 90.0f);
	cairo_matrix_translate (&matrix, 0.0f, -height);
	cairo_pattern_set_matrix (pattern, &matrix);
	cairo_rectangle (cr,
			 0.0f,
			 2 * shadow_radius,
			 2 * shadow_radius,
			 height - 2 * shadow_radius);
	cairo_fill (cr);

	/* top right */
	cairo_matrix_init_rotate (&matrix, (G_PI / 180.0f) * -90.0f);
	cairo_matrix_translate (&matrix, -width, 0.0f);
	cairo_pattern_set_matrix (pattern, &matrix);
	cairo_rectangle (cr,
			 width - 2 * shadow_radius,
			 0.0f,
			 2 * shadow_radius,
			 height - 2 * shadow_radius);
	cairo_fill (cr);

	/* bottom right */
	cairo_matrix_init_rotate (&matrix, (G_PI / 180.0f) * 180.0f);
	cairo_matrix_translate (&matrix, -width, -height);
	cairo_pattern_set_matrix (pattern, &matrix);
	cairo_rectangle (cr,
			 2 * shadow_radius,
			 height - 2 * shadow_radius,
			 width - 2 * shadow_radius,
			 2 * shadow_radius);
	cairo_fill (cr);

	/* clean up */
	cairo_pattern_destroy (pattern);
	cairo_surface_destroy (tmp_surface);
	cairo_surface_destroy (new_surface);
}

void
draw_value_indicator (cairo_t* cr,
		      gint     value,   /* value to render                 */
		      gint     start_x, /* top                             */
		      gint     start_y, /* left                            */
		      gint     width,   /* width of surrounding rect       */
		      gint     height,  /* height of surrounding rect      */
		      gint     bars,    /* how may bars to use for display */
		      gdouble* lit,     /* lit-color as gdouble[4]         */
		      gdouble* unlit    /* unlit-color as gdouble[4]       */)
{
	gint    step;
	gdouble x = (gdouble) start_x;
	gdouble y = (gdouble) start_y;
	gdouble w = (gdouble) width;
	gdouble h = (gdouble) height;
	gdouble radius;           /* corner-radius of a bar         */
	gdouble x_gap;            /* gap between two bars           */
	gdouble y_start = 0.275f; /* normalized height of first bar */
	gdouble x_step;           /* width of a bar                 */
	gdouble y_step;           /* increment-step for bar-height  */
	gint    step_value;

	/* sanity checks */
	if (bars < 0 || lit == NULL || unlit == NULL)
		return;

	/* reference rect for positioning-control */
	/*cairo_set_source_rgb (cr, 1.0f, 0.0f, 0.0f);
	cairo_rectangle (cr, x, y, w, h);
	cairo_stroke (cr);*/

	step_value = 100.0f / (gdouble) bars;
	x_gap = w * 0.3f / (gdouble) (bars - 1);
	x_step = w * 0.7f / (gdouble) bars;
	radius = 0.3f * x_step;
	y_step = (h - (h * y_start)) / (bars - 1);

	for (step = 0; step < bars; step++)
	{
		if (step * step_value >= value)
		{
			cairo_set_source_rgba (cr,
					       unlit[R],
					       unlit[G],
					       unlit[B],
					       unlit[A]);
		}
		else
		{
			cairo_set_source_rgba (cr,
					       lit[R],
					       lit[G],
					       lit[B],
					       lit[A]);
		}

		draw_round_rect (cr,
				 1.0f,
				 x + (x_step + x_gap) * (gdouble) step,
				 h + y - y_step * (gdouble) step - y_start * h,
				 radius,
				 x_step,
				 y_start * h + y_step * (gdouble) step);
		cairo_fill (cr);
	}
}

static
gboolean
expose_handler (GtkWidget*      window,
		GdkEventExpose* event,
		gpointer        data)
{
	Bubble*  bubble;
	cairo_t* cr;
	gdouble  width       = (gdouble) window->allocation.width;
	gdouble  height      = (gdouble) window->allocation.height;
	gdouble  margin_gap;
	gdouble  left_margin;
	gdouble  top_margin;

	bubble = (Bubble*) G_OBJECT (data);

	margin_gap   = (gdouble) defaults_get_margin_size (bubble->defaults);
	left_margin  = (gdouble) defaults_get_bubble_shadow_size (bubble->defaults);
	top_margin   = (gdouble) defaults_get_bubble_shadow_size (bubble->defaults);
	left_margin += (gdouble) defaults_get_margin_size (bubble->defaults);
	top_margin  += (gdouble) defaults_get_margin_size (bubble->defaults);

	cr = gdk_cairo_create (window->window);

        /* clear and render drop-shadow and bubble-background */
	cairo_scale (cr, 1.0f, 1.0f);
	cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint (cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	draw_shadow (cr,
		     width,
		     height,
		     defaults_get_bubble_shadow_size (bubble->defaults));
	cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
	draw_round_rect (cr,
			 1.0f,
			 (gdouble) defaults_get_bubble_shadow_size (bubble->defaults),
			 (gdouble) defaults_get_bubble_shadow_size (bubble->defaults),
			 (gdouble) defaults_get_bubble_corner_radius (bubble->defaults),
			 (gdouble) defaults_get_bubble_width (bubble->defaults),
			 (gdouble) defaults_get_bubble_min_height (bubble->defaults));
	cairo_fill (cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	cairo_set_source_rgba (cr,
			       BUBBLE_BG_COLOR_R,
			       BUBBLE_BG_COLOR_G,
			       BUBBLE_BG_COLOR_B,
			       gtk_window_get_opacity (GTK_WINDOW (window)));
	draw_round_rect (cr,
			 1.0f,
			 (gdouble) defaults_get_bubble_shadow_size (bubble->defaults),
			 (gdouble) defaults_get_bubble_shadow_size (bubble->defaults),
			 (gdouble) defaults_get_bubble_corner_radius (bubble->defaults),
			 (gdouble) defaults_get_bubble_width (bubble->defaults),
			 (gdouble) defaults_get_bubble_min_height (bubble->defaults));
	cairo_fill (cr);

	/* render icon */
	if (GET_PRIVATE (bubble)->icon_pixbuf)
	{
		gdk_cairo_set_source_pixbuf (
					cr,
					GET_PRIVATE (bubble)->icon_pixbuf,
					left_margin,
					left_margin);
		cairo_paint (cr);

		left_margin += (gdouble) defaults_get_icon_size (bubble->defaults);
		left_margin += (gdouble) defaults_get_margin_size (bubble->defaults);
	}

	/* render title */
	if (GET_PRIVATE (bubble)->title)
	{
		PangoFontDescription* desc   = NULL;
		PangoLayout*          layout = NULL;
		PangoRectangle        ink_rect;

		layout = pango_cairo_create_layout (cr);
		desc = pango_font_description_new ();
		pango_font_description_set_absolute_size (desc,
							  defaults_get_text_title_size (bubble->defaults) *
							  PANGO_SCALE);
		pango_font_description_set_family_static (desc, defaults_get_text_font_face (bubble->defaults));
		pango_font_description_set_weight (desc, defaults_get_text_title_weight (bubble->defaults));
		pango_font_description_set_style (desc, PANGO_STYLE_NORMAL);
		pango_layout_set_wrap (layout, PANGO_WRAP_WORD);
		pango_layout_set_font_description (layout, desc);
		pango_font_description_free (desc);

		pango_layout_set_width (layout,
					(defaults_get_bubble_width (bubble->defaults) - left_margin - margin_gap) *
					PANGO_SCALE);
		pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);

		/* print and layout string (pango-wise) */
		pango_layout_set_text (layout, GET_PRIVATE (bubble)->title, -1);

		pango_layout_get_extents (layout, NULL, &ink_rect);

		/* If no summary/message_body is present,
		 * and assuming there is an icon,
		 * center/align title in the middle of the bubble
		 */ 	
		if ((GET_PRIVATE (bubble)->message_body == '\0') &&
		    (GET_PRIVATE (bubble)->icon_pixbuf != NULL))
		{
			cairo_move_to (cr,
				       defaults_get_icon_size (bubble->defaults) +
				       (defaults_get_bubble_width (bubble->defaults) -
					defaults_get_icon_size (bubble->defaults) -
					ink_rect.width / PANGO_SCALE) / 2,
				       (defaults_get_bubble_min_height (bubble->defaults) -
					ink_rect.height / PANGO_SCALE) / 2);
		}
		else
		{
			cairo_move_to (cr, left_margin, top_margin);
		}

		/* draw pango-text as path to our cairo-context */
		pango_cairo_layout_path (cr, layout);

		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
		cairo_set_source_rgba (cr,
				       TEXT_TITLE_COLOR_R,
				       TEXT_TITLE_COLOR_G,
				       TEXT_TITLE_COLOR_B,
				       TEXT_TITLE_COLOR_A);
		cairo_fill (cr);
		g_object_unref (layout);

		top_margin += (gdouble) ink_rect.height / PANGO_SCALE;
	}

	/* render body-message */
	if (GET_PRIVATE (bubble)->message_body)
	{
		PangoFontDescription* desc   = NULL;
		PangoLayout*          layout = NULL;

		layout = pango_cairo_create_layout (cr);
		desc = pango_font_description_new ();
		pango_font_description_set_absolute_size (desc,
							  defaults_get_text_body_size (bubble->defaults) *
							  PANGO_SCALE);
		pango_font_description_set_family_static (desc,
							  defaults_get_text_font_face (bubble->defaults));
		pango_font_description_set_weight (desc,
						   defaults_get_text_body_weight (bubble->defaults));
		pango_font_description_set_style (desc, PANGO_STYLE_NORMAL);
		pango_layout_set_wrap (layout, PANGO_WRAP_WORD);
		pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);
		pango_layout_set_font_description (layout, desc);
		pango_font_description_free (desc);
		pango_layout_set_width (layout,
					(defaults_get_bubble_width (bubble->defaults) -
					 left_margin -
					 margin_gap) *
					PANGO_SCALE);
		pango_layout_set_height (layout,
					 (defaults_get_bubble_min_height (bubble->defaults) -
					  top_margin -
					  margin_gap) *
					 PANGO_SCALE);

		/* print and layout string (pango-wise) */
		/*pango_layout_set_text (layout,
				       GET_PRIVATE (bubble)->message_body,
				       -1);*/
		pango_layout_set_markup (layout,
					 GET_PRIVATE (bubble)->message_body,
					 -1);

		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
		cairo_move_to (cr, left_margin, top_margin);

		/* draw pango-text as path to our cairo-context */
		pango_cairo_layout_path (cr, layout);

		cairo_set_source_rgba (cr,
				       TEXT_BODY_COLOR_R,
				       TEXT_BODY_COLOR_G,
				       TEXT_BODY_COLOR_B,
				       TEXT_BODY_COLOR_A);
		cairo_fill (cr);
		g_object_unref (layout);
	}

	/* render value */
	if (GET_PRIVATE (bubble)->value >= 0 &&
	    GET_PRIVATE (bubble)->value <= 100)
	{
		gdouble lit[4]   = {INDICATOR_LIT_R,
				    INDICATOR_LIT_G,
				    INDICATOR_LIT_B,
				    INDICATOR_LIT_A};
		gdouble unlit[4] = {INDICATOR_UNLIT_R,
				    INDICATOR_UNLIT_G,
				    INDICATOR_UNLIT_B,
				    INDICATOR_UNLIT_A};

		draw_value_indicator (
			cr,
			GET_PRIVATE (bubble)->value,
			defaults_get_bubble_shadow_size (bubble->defaults) +
			2 * defaults_get_margin_size (bubble->defaults) +
			defaults_get_icon_size (bubble->defaults),
			defaults_get_bubble_shadow_size (bubble->defaults) +
			defaults_get_margin_size (bubble->defaults),
			defaults_get_bubble_width (bubble->defaults) -
			3 * defaults_get_margin_size (bubble->defaults) -
			defaults_get_icon_size (bubble->defaults),
			defaults_get_icon_size (bubble->defaults),
			13,
			lit,
			unlit);
	}

	cairo_destroy (cr);

	return TRUE;
}

static
gboolean
redraw_handler (Bubble* bubble)
{
	gdouble    opacity;
	GtkWindow* window;

	if (!bubble)
		return FALSE;

	if (!bubble_is_visible (bubble))
		return FALSE;

	window = GTK_WINDOW (GET_PRIVATE(bubble)->widget);

	if (!GTK_IS_WINDOW (window))
		return FALSE;

	opacity = gtk_window_get_opacity (window);

	if (!bubble_is_mouse_over (bubble) && opacity < 0.95f)
		opacity += 0.05f;

	if (bubble_is_mouse_over (bubble) && opacity > 0.1f)
		opacity -= 0.05f;

	gtk_window_set_opacity (window, opacity);

	return TRUE;
}

static
GdkPixbuf*
load_icon (const gchar* filename,
	   gint         icon_size)
{
	GdkPixbuf*    pixbuf = NULL;
	GtkIconTheme*  theme = NULL;
	GtkIconInfo*    info = NULL;
	GError*        error = NULL;
	GFile*          file = NULL;
	
	/* sanity check */
	g_return_val_if_fail (filename, NULL);

	/* Images referenced must always be local files. */
	if (!strncmp (filename, "file://", 7))
		filename += 7;

	file = g_file_new_for_path (filename);
	if (g_file_query_exists (file, NULL))
	/* Implementation note: blocking I/O; could be cancellable though */
	{
		/* load image into pixbuf */
		pixbuf = gdk_pixbuf_new_from_file_at_scale (filename,
							    icon_size,
							    icon_size,
							    TRUE,
							    &error);
	} else {
		/* TODO: rewrite, check for SVG support, raise apport notification for low-res icons */

		theme = gtk_icon_theme_get_default ();
		info  = gtk_icon_theme_lookup_icon (theme, filename,
						    icon_size,
						    GTK_ICON_LOOKUP_USE_BUILTIN);
		g_return_val_if_fail (info, NULL);

		const gchar *f = gtk_icon_info_get_filename (info);
		if (f == NULL ||
		    !g_str_has_suffix (f, ".svg"))
			g_warning ("icon '%s' not available in SVG", filename);

		pixbuf = gtk_icon_theme_load_icon (theme, filename,
						   icon_size,
						   GTK_ICON_LOOKUP_USE_BUILTIN,
						   NULL);
		
		gtk_icon_info_free (info);
	}

	return pixbuf;
}

static
gboolean
pointer_update (Bubble* bubble)
{
	gint       pointer_rel_x;
	gint       pointer_rel_y;
	gint       pointer_abs_x;
	gint       pointer_abs_y;
	gint       win_x;
	gint       win_y;
	gint       width;
	gint       height;
	GtkWidget* window;

	if (!bubble)
		return FALSE;

	if (!bubble_is_visible (bubble))
		return FALSE;

	window = GET_PRIVATE(bubble)->widget;

	if (!GTK_IS_WINDOW (window))
		return FALSE;

	if (GTK_WIDGET_REALIZED (window))
	{
		gtk_widget_get_pointer (window, &pointer_rel_x, &pointer_rel_y);
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
	}

	return TRUE;
}


/*-- internal API ------------------------------------------------------------*/

static void
bubble_dispose (GObject* gobject)
{
	/* chain up to the parent class */
	G_OBJECT_CLASS (bubble_parent_class)->dispose (gobject);
}

static void
bubble_finalize (GObject* gobject)
{
	if (GTK_IS_WIDGET (BUBBLE (gobject)->priv->widget))
	{
		gtk_widget_destroy (GTK_WIDGET (BUBBLE (gobject)->priv->widget));
		BUBBLE (gobject)->priv->widget = NULL;
	}

    	if (GET_PRIVATE (gobject)->title)
	{
		g_free ((gpointer) GET_PRIVATE (gobject)->title);
		GET_PRIVATE (gobject)->title = NULL;
	}

    	if (GET_PRIVATE (gobject)->message_body)
	{
		g_free ((gpointer) GET_PRIVATE (gobject)->message_body);
		GET_PRIVATE (gobject)->message_body = NULL;
	}

	if (GET_PRIVATE (gobject)->icon_pixbuf)
	{
		g_object_unref (GET_PRIVATE (gobject)->icon_pixbuf);
		GET_PRIVATE (gobject)->icon_pixbuf = NULL;
	}

	/* chain up to the parent class */
	G_OBJECT_CLASS (bubble_parent_class)->finalize (gobject);
}

static void
bubble_init (Bubble* self)
{
	/* If you need specific construction properties to complete
	** initialization, delay initialization completion until the
	** property is set. */

	BubblePrivate *priv;

	self->priv = priv      = GET_PRIVATE (self);
	priv->title            = NULL;
	priv->message_body     = NULL;
	priv->visible          = FALSE;
	priv->icon_pixbuf      = NULL;
	priv->value            = -1;
}

static void
bubble_get_property (GObject*    gobject,
		     guint       prop,
		     GValue*     value,
		     GParamSpec* spec)
{
	Bubble* bubble;

	bubble = BUBBLE (gobject);

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

	g_type_class_add_private (klass, sizeof (BubblePrivate));

	gobject_class->dispose      = bubble_dispose;
	gobject_class->finalize     = bubble_finalize;
	gobject_class->get_property = bubble_get_property;

	g_bubble_signals[TIMED_OUT] = g_signal_new ("timed-out",
						    G_OBJECT_CLASS_TYPE (gobject_class),
						    G_SIGNAL_RUN_LAST,
						    G_STRUCT_OFFSET (BubbleClass,
								     timed_out),
						    NULL,
						    NULL,
						    g_cclosure_marshal_VOID__VOID,
						    G_TYPE_NONE,
						    0);
}

/*-- public API --------------------------------------------------------------*/

Bubble*
bubble_new (Defaults* defaults)
{
	Bubble*         this              = NULL;
	GtkWidget*      window            = NULL;
	guint           draw_handler_id   = 0;
	guint           pointer_update_id = 0;

	this = g_object_new (BUBBLE_TYPE, NULL);
	if (!this)
		return NULL;

	this->defaults = defaults;

	GET_PRIVATE (this)->widget = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	window = GET_PRIVATE (this)->widget;
	if (!window)
		return NULL;

	g_object_set_data (G_OBJECT(window), "bubble", (gpointer) &this);

	gtk_window_set_type_hint (GTK_WINDOW (window),
				  GDK_WINDOW_TYPE_HINT_DOCK);

	gtk_widget_add_events (window,
			       GDK_POINTER_MOTION_MASK |
			       GDK_BUTTON_PRESS_MASK |
			       GDK_BUTTON_RELEASE_MASK);

	/* hook up input-event handlers to window */
	g_signal_connect (G_OBJECT (window),
			  "screen-changed",
			  G_CALLBACK (screen_changed_handler),
			  NULL);

	gtk_window_move (GTK_WINDOW (window), 0, 0);

	/* make sure the window opens with a RGBA-visual */
	screen_changed_handler (window, NULL, NULL);
	gtk_widget_realize (window);
	gdk_window_set_back_pixmap (window->window, NULL, FALSE);

	/* hook up window-event handlers to window */
	g_signal_connect (G_OBJECT (window),
			  "expose-event",
			  G_CALLBACK (expose_handler),
			  this);       

	/*  "clear" input-mask, set title/icon/attributes */
	update_input_shape (window, 1, 1);
	gtk_widget_set_app_paintable (window, TRUE);
	gtk_window_set_title (GTK_WINDOW (window), "notification");
	gtk_window_set_decorated (GTK_WINDOW (window), FALSE);
	gtk_window_set_keep_above (GTK_WINDOW (window), TRUE);
	gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
	gtk_window_set_accept_focus (GTK_WINDOW (window), FALSE);
	gtk_window_set_opacity (GTK_WINDOW (window), 0.95f);

	this->priv = GET_PRIVATE (this);
	GET_PRIVATE(this)->widget        = window;
	GET_PRIVATE(this)->title         = g_strdup("GTK+ Notification");
	GET_PRIVATE(this)->message_body  = g_strdup("Courtesy of the new Canonical notification sub-system");
	GET_PRIVATE(this)->visible       = FALSE;
	GET_PRIVATE(this)->timeout       = 5;
	GET_PRIVATE(this)->mouse_over    = FALSE;
	GET_PRIVATE(this)->start_y       = 0;
	GET_PRIVATE(this)->end_y         = 0;
	GET_PRIVATE(this)->inc_factor    = 0.0f;
	GET_PRIVATE(this)->delta_y       = 0;

	/* FIXME: do nasty busy-polling rendering in the drawing-area */
	draw_handler_id = g_timeout_add (1000/60,
					 (GSourceFunc) redraw_handler,
					 this);

	/* FIXME: read out current mouse-pointer position every 1/10 second */
        pointer_update_id = g_timeout_add (100,
					   (GSourceFunc) pointer_update,
					   this);

	return this;
}

void
bubble_del (Bubble* self)
{
	if (!self || !IS_BUBBLE (self))
		return;

	g_object_unref (self);
}

void
bubble_set_title (Bubble*      self,
		  const gchar* title)
{
	if (!self || !IS_BUBBLE (self))
		return;

    	if (GET_PRIVATE (self)->title)
		g_free ((gpointer) GET_PRIVATE (self)->title);

	GET_PRIVATE (self)->title = g_strdup (title);
}

void
bubble_set_message_body (Bubble*      self,
			 const gchar* body)
{
	if (!self || !IS_BUBBLE (self))
		return;

    	if (GET_PRIVATE (self)->message_body)
		g_free ((gpointer) GET_PRIVATE (self)->message_body);

	GET_PRIVATE (self)->message_body = g_strdup (body);
}

void
bubble_set_icon (Bubble*      self,
		 const gchar* filename)
{
 	if (!self || !IS_BUBBLE (self))
		return;

	if (GET_PRIVATE (self)->icon_pixbuf)
		g_object_unref (GET_PRIVATE (self)->icon_pixbuf);

	GET_PRIVATE (self)->icon_pixbuf = load_icon (
						filename,
						defaults_get_icon_size (self->defaults));
}

void
bubble_set_icon_from_pixbuf (Bubble*    self,
			     GdkPixbuf* pixbuf)
{
	GdkPixbuf *scaled;
	int height, width;

 	if (!self || !IS_BUBBLE (self) || !pixbuf)
		return;

	if (GET_PRIVATE (self)->icon_pixbuf)
		g_object_unref (GET_PRIVATE (self)->icon_pixbuf);

	height = gdk_pixbuf_get_height (pixbuf);
	width = gdk_pixbuf_get_width (pixbuf);

	if (width != defaults_get_icon_size (self->defaults))
	{
		if (width != height)
			g_warning ("non-square pixmap");
		/* TODO: improve scaling for non-square pixmaps */

		scaled = gdk_pixbuf_scale_simple (pixbuf,
						  defaults_get_icon_size (self->defaults),
						  defaults_get_icon_size (self->defaults),
						  GDK_INTERP_HYPER);
						  /*GDK_INTERP_BILINEAR);*/
		pixbuf = scaled;
	}

	GET_PRIVATE (self)->icon_pixbuf = pixbuf;
}

void
bubble_set_value (Bubble* self,
		  gint    value)
{
	if (!self || !IS_BUBBLE (self))
		return;

	GET_PRIVATE (self)->value = value;
}

void
bubble_set_size (Bubble* self,
		 gint    width,
		 gint    height)
{
	if (!self || !IS_BUBBLE (self))
		return;

	gtk_widget_set_size_request (GET_PRIVATE(self)->widget, width, height);
}

void
bubble_set_timeout (Bubble* self,
		    guint   timeout)
{
	if (!self || !IS_BUBBLE (self))
		return;

	GET_PRIVATE (self)->timeout = timeout;
}

/* a timeout of 0 doesn't make much sense now does it, thus 0 indicates an
** error */
guint
bubble_get_timeout (Bubble* self)
{
	if (!self || !IS_BUBBLE (self))
		return 0;

	return GET_PRIVATE(self)->timeout;
}

void
bubble_set_timer_id (Bubble* self,
		     guint   timer_id)
{
	if (!self || !IS_BUBBLE (self))
		return;

	GET_PRIVATE(self)->timer_id = timer_id;
}

/* a valid GLib timer-id is always > 0, thus 0 indicates an error */
guint
bubble_get_timer_id (Bubble* self)
{
	if (!self || !IS_BUBBLE (self))
		return 0;

	return GET_PRIVATE(self)->timer_id;
}

void
bubble_set_mouse_over (Bubble*  self,
		       gboolean flag)
{
	if (!self || !IS_BUBBLE (self))
		return;

	GET_PRIVATE(self)->mouse_over = flag;
}

gboolean
bubble_is_mouse_over (Bubble* self)
{
	if (!self || !IS_BUBBLE (self))
		return FALSE;

	return GET_PRIVATE(self)->mouse_over;
}

void
bubble_move (Bubble* self,
	     gint    x,
	     gint    y)
{
	if (!self || !IS_BUBBLE (self))
		return;

	gtk_window_move (GTK_WINDOW (GET_PRIVATE (self)->widget), x, y);
}

void
bubble_show (Bubble* self)
{
	if (!self || !IS_BUBBLE (self))
		return;

	GET_PRIVATE (self)->visible = TRUE;
	gtk_widget_show_all (GET_PRIVATE (self)->widget);

	/* and now let the timer tick... we use g_timeout_add_seconds() here
	** because for the bubble timeouts microsecond-precise resolution is not
	** needed, furthermore this also allows glib more room for optimizations
	** and improve system-power-usage to be more efficient */
	bubble_set_timer_id (self,
			     g_timeout_add_seconds (bubble_get_timeout (self),
						    (GSourceFunc) bubble_timed_out,
						    self));
}

/* mostly called when we change the content of the bubble
   and need to force a redraw
*/
void
bubble_refresh (Bubble* self)
{
	/* force a redraw */
	gtk_widget_queue_draw (GET_PRIVATE (self)->widget);
}

static
gboolean
do_slide_bubble (Bubble* self)
{
	gint x = 0;
	gint y = 0;

	/* sanity check */
	if (!self || !IS_BUBBLE (self))
		return FALSE;

	/* where is the bubble at the moment? */
	gtk_window_get_position (GTK_WINDOW (GET_PRIVATE (self)->widget),
				 &x,
				 &y);

	/* check if we arrived at the destination ... or overshot */
	if (GET_PRIVATE (self)->delta_y > 0)
	{
		/* stop the callback/animation */
		if (y >= GET_PRIVATE (self)->end_y)
			return FALSE;
	}
	else
	{
		/* stop the callback/animation */
		if (y <= GET_PRIVATE (self)->end_y)
			return FALSE;
	}

	/* move the bubble to the new position */
	bubble_move (self,
		     x,
		     GET_PRIVATE (self)->start_y +
		     GET_PRIVATE (self)->delta_y *
		     sin (GET_PRIVATE (self)->inc_factor) * G_PI / 2.0f);

	/* "advance" the increase-factor */
	GET_PRIVATE (self)->inc_factor += 1.0f / 30.0f;

	/* keep going */
	return TRUE;
}

void
bubble_slide_to (Bubble* self,
		 gint    end_x,
		 gint    end_y)
{
	guint timer_id = 0;
	gint  start_x  = 0;
	gint  start_y  = 0;

	if (!self || !IS_BUBBLE (self))
		return;

	/* determine direction to take */
	gtk_window_get_position (GTK_WINDOW (GET_PRIVATE (self)->widget),
				 &start_x,
				 &start_y);

	/* don't do anything if we got the same target/end y-position */
	if (end_y == start_y)
		return;

	GET_PRIVATE (self)->inc_factor = 0.0f;
	GET_PRIVATE (self)->start_y    = start_y;
	GET_PRIVATE (self)->end_y      = end_y;
	GET_PRIVATE (self)->delta_y    = end_y - start_y;

	/* and now let the timer tick... */
	timer_id = g_timeout_add (1000/60,
				  (GSourceFunc) do_slide_bubble,
				  self);
}

gboolean
bubble_timed_out (Bubble* self)
{
	if (!self || !IS_BUBBLE (self))
		return FALSE;

	bubble_set_timeout (self, 0);
	bubble_hide (self);

	g_signal_emit (self, g_bubble_signals[TIMED_OUT], 0);

	return FALSE;
}

gboolean
bubble_hide (Bubble* self)
{
	if (!self || !IS_BUBBLE (self) || !bubble_is_visible (self))
		return FALSE;

	GET_PRIVATE (self)->visible = FALSE;
	gtk_widget_hide (GET_PRIVATE (self)->widget);

	return FALSE; /* this also instructs the timer to stop */
}

void
bubble_set_id (Bubble* self,
	       guint   id)
{
	if (!self || !IS_BUBBLE (self))
		return;

	GET_PRIVATE (self)->id = id;
}

guint
bubble_get_id (Bubble* self)
{
	if (!self || !IS_BUBBLE (self))
		return 0;

	return GET_PRIVATE (self)->id;
}

gboolean
bubble_is_visible (Bubble* self)
{
	if (!self || !IS_BUBBLE (self))
		return FALSE;

	return GET_PRIVATE (self)->visible;
}

void
bubble_reset_timeout (Bubble* self)
{
	guint timer_id;

	if (!self || !IS_BUBBLE (self))
		return;

	timer_id = bubble_get_timer_id (self);
	if (timer_id <= 0)
		return;

	if (g_source_remove (timer_id))
	{
		bubble_set_timer_id (
			self,
			g_timeout_add_seconds (bubble_get_timeout (self),
					       (GSourceFunc) bubble_timed_out,
					       self));
	}
}

void
bubble_get_position (Bubble* self,
		     gint*   x,
		     gint*   y)
{
	if (!self || !IS_BUBBLE (self))
		return;

	gtk_window_get_position (GTK_WINDOW (GET_PRIVATE (self)->widget),
				 x, y);
}


gint
bubble_get_height (Bubble *self)
{

	gint width;
	gint height;

	if (!self || !IS_BUBBLE (self))
		return 0;

	gtk_window_get_size (GTK_WINDOW (GET_PRIVATE (self)->widget),
			     &width,
			     &height);

	return height;
}
