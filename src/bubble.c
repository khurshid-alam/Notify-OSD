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
#include <X11/Xatom.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <pixman.h>
#include <math.h>

#include <clutter/clutter-hack.h>
#include <clutter/clutter-alpha.h>

#include "bubble.h"
#include "defaults.h"
#include "stack.h"

G_DEFINE_TYPE (Bubble, bubble, G_TYPE_OBJECT);

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BUBBLE_TYPE, BubblePrivate))

typedef enum
{
	LAYOUT_NONE = 0,
	LAYOUT_ICON_INDICATOR,
	LAYOUT_ICON_TITLE,
	LAYOUT_ICON_TITLE_BODY,
	LAYOUT_TITLE_BODY
} BubbleLayout;

struct _BubblePrivate {
	BubbleLayout layout;
	GtkWidget*   widget;
	GString*     title;
	GString*     message_body;
	guint        id;
	GdkPixbuf*   icon_pixbuf;
	gboolean     visible;
	guint        timer_id;
	guint        timeout;
	gboolean     mouse_over;
	gint         start_y;
	gint         end_y;
	gint         delta_y;
	gdouble      inc_factor;
	gint         value; /* "empty": -1, valid range: 0 - 100 */
	gchar*       synchronous;
	gboolean     urgent;
	gboolean     composited;
	ClutterAlpha *alpha;
	ClutterTimeline *timeline;
	guint        draw_handler_id;
	guint        pointer_update_id;
	cairo_surface_t* blurred_content;
	cairo_surface_t* blurred_bubble;
	gint             title_width;
	gint             title_height;
	gint             body_width;
	gint             body_height;
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
bubble_determine_layout (Bubble* self)
{
	/* sanity test */
	if (!self || !IS_BUBBLE (self))
		return;

	/* set a sane default */
	GET_PRIVATE (self)->layout = LAYOUT_NONE;

	/* icon + indicator layout-case, e.g. volume */
	if ((GET_PRIVATE (self)->icon_pixbuf       != NULL) &&
	    (GET_PRIVATE (self)->title->len        != 0) &&
	    (GET_PRIVATE (self)->message_body->len == 0) &&
	    (GET_PRIVATE (self)->value             >= 0))
	{
		GET_PRIVATE (self)->layout = LAYOUT_ICON_INDICATOR;
		return;
	}

	/* icon + title layout-case, e.g. "Wifi signal lost" */
	if ((GET_PRIVATE (self)->icon_pixbuf       != NULL) &&
	    (GET_PRIVATE (self)->title->len        != 0) &&
	    (GET_PRIVATE (self)->message_body->len == 0) &&
	    (GET_PRIVATE (self)->value             == -1))
	{
		GET_PRIVATE (self)->layout = LAYOUT_ICON_TITLE;
		return;	    
	}

	/* icon/avatar + title + body/message layout-case, e.g. IM-message */
	if ((GET_PRIVATE (self)->icon_pixbuf       != NULL) &&
	    (GET_PRIVATE (self)->title->len        != 0) &&
	    (GET_PRIVATE (self)->message_body->len != 0) &&
	    (GET_PRIVATE (self)->value             == -1))
	{
		GET_PRIVATE (self)->layout = LAYOUT_ICON_TITLE_BODY;
		return;
	}

	/* title + body/message layout-case, e.g. IM-message without avatar */
	if ((GET_PRIVATE (self)->icon_pixbuf       == NULL) &&
	    (GET_PRIVATE (self)->title->len        != 0) &&
	    (GET_PRIVATE (self)->message_body->len != 0) &&
	    (GET_PRIVATE (self)->value             == -1))
	{
		GET_PRIVATE (self)->layout = LAYOUT_TITLE_BODY;
		return;
	}
}

BubbleLayout
bubble_get_layout (Bubble* self)
{
	if (!self || !IS_BUBBLE (self))
		return LAYOUT_NONE;

	return GET_PRIVATE (self)->layout;
}

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

#if 0
static void
draw_layout_grid (cairo_t* cr,
		  Bubble*  bubble)
{
	Defaults* d = bubble->defaults;

	if (!cr)
		return;

	cairo_set_line_width (cr, 1.0f);
	cairo_set_source_rgba (cr, 0.0f, 1.0f, 0.0f, 0.5f);

	/* all vertical grid lines */
	cairo_move_to (cr,
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d),
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d));
	cairo_line_to (cr,
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d),
		       (gdouble) bubble_get_height (bubble) -
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d));
	cairo_move_to (cr,
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d) +
		       EM2PIXELS (defaults_get_margin_size (d), d),
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d));
	cairo_line_to (cr,
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d) +
		       EM2PIXELS (defaults_get_margin_size (d), d),
		       (gdouble) bubble_get_height (bubble) -
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d));
	cairo_move_to (cr,
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d) +
		       EM2PIXELS (defaults_get_margin_size (d), d) +
		       EM2PIXELS (defaults_get_icon_size (d), d),
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d));
	cairo_line_to (cr,
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d) +
		       EM2PIXELS (defaults_get_margin_size (d), d) +
		       EM2PIXELS (defaults_get_icon_size (d), d),
		       (gdouble) bubble_get_height (bubble) -
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d));
	cairo_move_to (cr,
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d) +
		       EM2PIXELS (2 * defaults_get_margin_size (d), d) +
		       EM2PIXELS (defaults_get_icon_size (d), d),
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d));
	cairo_line_to (cr,
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d) +
		       EM2PIXELS (2 * defaults_get_margin_size (d), d) +
		       EM2PIXELS (defaults_get_icon_size (d), d),
		       (gdouble) bubble_get_height (bubble) -
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d));
	cairo_move_to (cr,
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d) +
		       EM2PIXELS (defaults_get_bubble_width (d), d) -
		       EM2PIXELS (defaults_get_margin_size (d), d),
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d));
	cairo_line_to (cr,
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d) +
		       EM2PIXELS (defaults_get_bubble_width (d), d) -
		       EM2PIXELS (defaults_get_margin_size (d), d),
		       (gdouble) bubble_get_height (bubble) -
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d));
	cairo_move_to (cr,
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d) +
		       EM2PIXELS (defaults_get_bubble_width (d), d),
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d));
	cairo_line_to (cr,
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d) +
		       EM2PIXELS (defaults_get_bubble_width (d), d),
		       (gdouble) bubble_get_height (bubble) -
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d));

	/* all horizontal grid lines */
	cairo_move_to (cr,
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d),
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d));
	cairo_line_to (cr,
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d) +
		       EM2PIXELS (defaults_get_bubble_width (d), d),
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d));
	cairo_move_to (cr,
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d),
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d) +
		       EM2PIXELS (defaults_get_margin_size (d), d));
	cairo_line_to (cr,
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d) +
		       EM2PIXELS (defaults_get_bubble_width (d), d),
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d) +
		       EM2PIXELS (defaults_get_margin_size (d), d));
	cairo_move_to (cr,
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d),
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d) +
		       EM2PIXELS (defaults_get_margin_size (d), d) +
		       EM2PIXELS (defaults_get_icon_size (d), d));
	cairo_line_to (cr,
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d) +
		       EM2PIXELS (defaults_get_bubble_width (d), d),
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d) +
		       EM2PIXELS (defaults_get_margin_size (d), d) +
		       EM2PIXELS (defaults_get_icon_size (d), d));
	cairo_move_to (cr,
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d),
		       (gdouble) bubble_get_height (bubble) -
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d) -
		       EM2PIXELS (defaults_get_margin_size (d), d));
	cairo_line_to (cr,
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d) +
		       EM2PIXELS (defaults_get_bubble_width (d), d),
		       (gdouble) bubble_get_height (bubble) -
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d) -
		       EM2PIXELS (defaults_get_margin_size (d), d));
	cairo_move_to (cr,
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d),
		       (gdouble) bubble_get_height (bubble) -
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d));
	cairo_line_to (cr,
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d) +
		       EM2PIXELS (defaults_get_bubble_width (d), d),
		       (gdouble) bubble_get_height (bubble) -
		       EM2PIXELS (defaults_get_bubble_shadow_size (d), d));

	cairo_stroke (cr);
}
#endif

static void
draw_value_indicator (cairo_t* cr,
		      gint     value,   /* value to render: 0 - 100        */
		      gint     start_x, /* top of surrounding rect         */
		      gint     start_y, /* left of surrounding rect        */
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

static void
_render_icon_indicator (Bubble*  self,
			cairo_t* cr)
{
	Defaults*        d = self->defaults;
	cairo_t*         glow_cr;
	cairo_surface_t* glow_surface;
	cairo_surface_t* tmp;
	cairo_status_t   status;
	cairo_pattern_t* pattern;
	gint             bars = 13;
	gdouble          lit[4]   = {INDICATOR_LIT_R,
				     INDICATOR_LIT_G,
				     INDICATOR_LIT_B,
				     INDICATOR_LIT_A};
	gdouble          unlit[4] = {INDICATOR_UNLIT_R,
				     INDICATOR_UNLIT_G,
				     INDICATOR_UNLIT_B,
				     INDICATOR_UNLIT_A};
	gint             blur_radius = 10;

	/* create "scratch-pad" surface */
	glow_surface = cairo_image_surface_create (
			CAIRO_FORMAT_ARGB32,
			EM2PIXELS (defaults_get_bubble_width (d), d) -
			2 * EM2PIXELS (defaults_get_margin_size (d), d) +
			2 * blur_radius,
			EM2PIXELS (defaults_get_icon_size (d), d) +
			2 * blur_radius);
	status = cairo_surface_status (glow_surface);
	if (status != CAIRO_STATUS_SUCCESS)
		return;

	/* create context for scratch-pad surface */
	glow_cr = cairo_create (glow_surface);
	status = cairo_status (glow_cr);
	if (status != CAIRO_STATUS_SUCCESS)
	{
		cairo_surface_destroy (glow_surface);
		return;
	}

	/* clear context of scratch-pad surface */
	cairo_scale (glow_cr, 1.0f, 1.0f);
	cairo_set_operator (glow_cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint (glow_cr);

	cairo_set_operator (glow_cr, CAIRO_OPERATOR_OVER);

	/* render icon to scratch-pad context */
	gdk_cairo_set_source_pixbuf (glow_cr,
				     GET_PRIVATE (self)->icon_pixbuf,
				     blur_radius,
				     blur_radius);
	cairo_paint (glow_cr);

	/* render value-bar(s) to scratch-pad context */
	draw_value_indicator (
		glow_cr,
		GET_PRIVATE (self)->value,
		EM2PIXELS (defaults_get_margin_size (d), d) +
		EM2PIXELS (defaults_get_icon_size (d), d) +
		blur_radius,
		blur_radius,
		EM2PIXELS (defaults_get_bubble_width (d), d) -
		3 * EM2PIXELS (defaults_get_margin_size (d), d) -
		EM2PIXELS (defaults_get_icon_size (d), d),
		EM2PIXELS (defaults_get_icon_size (d), d),
		bars,
		lit,
		unlit);

	/* "blit" scratch-pad context to context of bubble */
	cairo_set_source_surface (cr,
				  glow_surface,
				  EM2PIXELS (defaults_get_bubble_shadow_size (d), d) +
				  EM2PIXELS (defaults_get_margin_size (d), d) -
				  blur_radius,
				  EM2PIXELS (defaults_get_bubble_shadow_size (d), d) +
				  EM2PIXELS (defaults_get_margin_size (d), d) -
				  blur_radius);
	cairo_paint (cr);

	switch (GET_PRIVATE (self)->value)
	{
		/* "undershoot" effect */
		case 0:
			/* abuse blur to create a mask of scratch-pad surface */
			tmp = blur_image_surface (glow_surface,
						  0.0f,
						  0.0f);

			/* clear scratch-pad context */
			cairo_set_operator (glow_cr, CAIRO_OPERATOR_CLEAR);
			cairo_paint (glow_cr);

			/* create mask-pattern from scratch-pad surface */
			cairo_set_operator (glow_cr, CAIRO_OPERATOR_OVER);
			cairo_push_group (glow_cr);
			cairo_set_source_surface (glow_cr, tmp, 0.0f, 0.0f);
			cairo_paint (glow_cr);
			pattern = cairo_pop_group (glow_cr);

			/* paint semi transparent black through mask-pattern */
			cairo_set_source_rgba (glow_cr, 0.0f, 0.0f, 0.0f, 0.65f);
			cairo_mask (glow_cr, pattern);
			cairo_pattern_destroy (pattern);

			/* finally "blit" scratch-pad onto context of bubble */
			cairo_set_source_surface (cr,
						  glow_surface,
						  EM2PIXELS (defaults_get_bubble_shadow_size (d), d) +
						  EM2PIXELS (defaults_get_margin_size (d), d) -
						  blur_radius,
						  EM2PIXELS (defaults_get_bubble_shadow_size (d), d) +
						  EM2PIXELS (defaults_get_margin_size (d), d) -
						  blur_radius);
			cairo_paint_with_alpha (cr, 0.8f);
		break;

		/* "overshoot" effect */
		case 100:
			/* blur the scratch-pad surface */
			tmp = blur_image_surface (glow_surface,
						  blur_radius,
						  0.0f);

			/* clear scratch-pad context */
			cairo_set_operator (glow_cr, CAIRO_OPERATOR_CLEAR);
			cairo_paint (glow_cr);

			/* create mask-pattern from blurred scratch-pad
			 * surface */
			cairo_set_operator (glow_cr, CAIRO_OPERATOR_OVER);
			cairo_push_group (glow_cr);
			cairo_set_source_surface (glow_cr, tmp, 0.0f, 0.0f);
			cairo_paint (glow_cr);
			pattern = cairo_pop_group (glow_cr);

			/* paint fully opaque white "through" blurred
			 * mask-pattern */
			cairo_set_source_rgba (glow_cr, 1.0f, 1.0f, 1.0f, 1.0f);
			cairo_mask (glow_cr, pattern);
			cairo_pattern_destroy (pattern);

			/* finally "blit" scratch-pad onto context of bubble */
			cairo_set_source_surface (cr,
						  glow_surface,
						  EM2PIXELS (defaults_get_bubble_shadow_size (d), d) +
						  EM2PIXELS (defaults_get_margin_size (d), d) -
						  blur_radius,
						  EM2PIXELS (defaults_get_bubble_shadow_size (d), d) +
						  EM2PIXELS (defaults_get_margin_size (d), d) -
						  blur_radius);
			cairo_paint_with_alpha (cr, 1.0f);
		break;

		/* normal effect-less rendering */
		default:
			/* do nothing */
		break;
	}

	/* clean up */
	cairo_destroy (glow_cr);
	cairo_surface_destroy (glow_surface);
	cairo_surface_destroy (tmp);
}

static void
_render_icon_title (Bubble*  self,
		    cairo_t* cr)
{
	Defaults*             d = self->defaults;
	gint                  margin_gap;
	gint                  top_margin;
	gint                  left_margin;
	PangoFontDescription* desc   = NULL;
	PangoLayout*          layout = NULL;
	PangoRectangle        ink_rect;
	PangoRectangle        log_rect;

	margin_gap   = EM2PIXELS (defaults_get_margin_size (d), d);
	top_margin   = EM2PIXELS (defaults_get_bubble_shadow_size (d), d);
	left_margin = EM2PIXELS (defaults_get_bubble_shadow_size (d), d) +
		      EM2PIXELS (defaults_get_margin_size (d), d);

	/* render icon */
	gdk_cairo_set_source_pixbuf (cr,
				     GET_PRIVATE (self)->icon_pixbuf,
				     left_margin,
				     left_margin);
	cairo_paint (cr);

	left_margin += EM2PIXELS (defaults_get_icon_size (d), d);
	left_margin += EM2PIXELS (defaults_get_margin_size (d), d);

	/* render title */
	layout = pango_cairo_create_layout (cr);
	desc = pango_font_description_new ();

	pango_font_description_set_size (desc,
					 EM2PIXELS (defaults_get_text_title_size (d), d) *
					 PANGO_SCALE);
	pango_font_description_set_family_static (desc, defaults_get_text_font_face (d));
	pango_font_description_set_weight (desc, defaults_get_text_title_weight (d));
	pango_font_description_set_style (desc, PANGO_STYLE_NORMAL);
	pango_layout_set_wrap (layout, PANGO_WRAP_WORD);
	pango_layout_set_font_description (layout, desc);
	pango_font_description_free (desc);

	pango_layout_set_width (layout,
				(EM2PIXELS (defaults_get_bubble_width (d), d) -
				 left_margin -
				 margin_gap) *
				PANGO_SCALE);

	pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);

	/* print and layout string (pango-wise) */
	pango_layout_set_text (layout,
			       GET_PRIVATE (self)->title->str,
			       GET_PRIVATE (self)->title->len);

	pango_layout_get_extents (layout, &ink_rect, &log_rect);

	top_margin = (bubble_get_height (self) / 2) -
		     (ink_rect.height / PANGO_SCALE) +
		     0.25f * (ink_rect.height / PANGO_SCALE);

	cairo_move_to (cr, left_margin, top_margin);

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
}

static void
_render_icon_title_body (Bubble*  self,
			 cairo_t* cr)
{
	Defaults*             d      = self->defaults;
	PangoFontDescription* desc   = NULL;
	PangoLayout*          layout = NULL;
	PangoRectangle        ink_rect;
	PangoRectangle        log_rect;
	gint                  margin_gap;
	gint                  top_margin;
	gint                  left_margin;

	margin_gap  = EM2PIXELS (defaults_get_margin_size (d), d);
	top_margin  = EM2PIXELS (defaults_get_bubble_shadow_size (d), d);
	left_margin = EM2PIXELS (defaults_get_bubble_shadow_size (d), d) +
		      EM2PIXELS (defaults_get_margin_size (d), d);

	/* render icon */
	gdk_cairo_set_source_pixbuf (cr,
				     GET_PRIVATE (self)->icon_pixbuf,
				     left_margin,
				     left_margin);
	cairo_paint (cr);

	left_margin += EM2PIXELS (defaults_get_icon_size (d), d);
	left_margin += EM2PIXELS (defaults_get_margin_size (d), d);

	/* render title */
	layout = pango_cairo_create_layout (cr);
	desc = pango_font_description_new ();

	pango_font_description_set_size (desc,
					 EM2PIXELS (defaults_get_text_title_size (d), d) *
					 PANGO_SCALE);
	pango_font_description_set_family_static (desc, defaults_get_text_font_face (d));
	pango_font_description_set_weight (desc, defaults_get_text_title_weight (d));
	pango_font_description_set_style (desc, PANGO_STYLE_NORMAL);
	pango_layout_set_font_description (layout, desc);
	pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);
	pango_font_description_free (desc);

	/* print and layout string (pango-wise) */
	pango_layout_set_text (layout,
			       GET_PRIVATE (self)->title->str,
			       GET_PRIVATE (self)->title->len);

	pango_layout_set_width (layout,
				GET_PRIVATE (self)->title_width * PANGO_SCALE);

	pango_layout_get_extents (layout, &ink_rect, &log_rect);

	top_margin += margin_gap;

	cairo_move_to (cr, left_margin, top_margin);

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

	top_margin += log_rect.height / PANGO_SCALE;

	/* render body-message */
	layout = pango_cairo_create_layout (cr);
	desc = pango_font_description_new ();
	pango_font_description_set_size (desc,
					 EM2PIXELS (defaults_get_text_body_size (d), d) *
					 PANGO_SCALE);
	pango_font_description_set_family_static (desc,
						  defaults_get_text_font_face (d));
	pango_font_description_set_weight (desc,
					   defaults_get_text_body_weight (d));
	pango_font_description_set_style (desc, PANGO_STYLE_NORMAL);
	pango_layout_set_wrap (layout, PANGO_WRAP_WORD);
	pango_layout_set_font_description (layout, desc);
	pango_font_description_free (desc);
	pango_layout_set_width (layout,
				GET_PRIVATE (self)->body_width * PANGO_SCALE);

	/* print and layout string (pango-wise) */
	pango_layout_set_text (layout,
			       GET_PRIVATE (self)->message_body->str,
			       GET_PRIVATE (self)->message_body->len);

	pango_layout_get_extents (layout, &ink_rect, &log_rect);

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

static void
_render_title_body (Bubble*  self,
		    cairo_t* cr)
{
	Defaults*             d      = self->defaults;
	PangoFontDescription* desc   = NULL;
	PangoLayout*          layout = NULL;
	PangoRectangle        ink_rect;
	PangoRectangle        log_rect;
	gint                  margin_gap;
	gint                  top_margin;
	gint                  left_margin;

	margin_gap  = EM2PIXELS (defaults_get_margin_size (d), d);
	top_margin  = EM2PIXELS (defaults_get_bubble_shadow_size (d), d);
	left_margin = EM2PIXELS (defaults_get_bubble_shadow_size (d), d) +
		      EM2PIXELS (defaults_get_margin_size (d), d);

	/* render title */
	layout = pango_cairo_create_layout (cr);
	desc = pango_font_description_new ();

	pango_font_description_set_size (desc,
					 EM2PIXELS (defaults_get_text_title_size (d), d) *
					 PANGO_SCALE);
	pango_font_description_set_family_static (desc, defaults_get_text_font_face (d));
	pango_font_description_set_weight (desc, defaults_get_text_title_weight (d));
	pango_font_description_set_style (desc, PANGO_STYLE_NORMAL);
	pango_layout_set_wrap (layout, PANGO_WRAP_WORD);
	pango_layout_set_font_description (layout, desc);
	pango_font_description_free (desc);

	pango_layout_set_width (layout,
				GET_PRIVATE (self)->title_width * PANGO_SCALE);

	pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);

	/* print and layout string (pango-wise) */
	pango_layout_set_text (layout,
			       GET_PRIVATE (self)->title->str,
			       GET_PRIVATE (self)->title->len);

	pango_layout_get_extents (layout, &ink_rect, &log_rect);

	top_margin += margin_gap;

	cairo_move_to (cr, left_margin, top_margin);

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

	top_margin += (gdouble) log_rect.height / PANGO_SCALE;

	/* render body-message */
	layout = pango_cairo_create_layout (cr);
	desc = pango_font_description_new ();
	pango_font_description_set_size (desc,
					 EM2PIXELS (defaults_get_text_body_size (d), d) *
					 PANGO_SCALE);
	pango_font_description_set_family_static (desc,
						  defaults_get_text_font_face (d));
	pango_font_description_set_weight (desc,
					   defaults_get_text_body_weight (d));
	pango_font_description_set_style (desc, PANGO_STYLE_NORMAL);
	pango_layout_set_wrap (layout, PANGO_WRAP_WORD);
	pango_layout_set_font_description (layout, desc);
	pango_font_description_free (desc);
	pango_layout_set_width (layout,
				GET_PRIVATE (self)->body_width * PANGO_SCALE);

	/* print and layout string (pango-wise) */
	pango_layout_set_text (layout,
			       GET_PRIVATE (self)->message_body->str,
			       GET_PRIVATE (self)->message_body->len);

	pango_layout_get_extents (layout, &ink_rect, &log_rect);

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

static void
update_shape (Bubble* self)
{
	GdkBitmap* mask = NULL;
	cairo_t*   cr   = NULL;
	gint       width;
	gint       height;
	Defaults*  d;

	/* sanity test */
	if (!self || !IS_BUBBLE (self))
		return;

	d = self->defaults;

	/* do we actually need a shape-mask at all? */
	if (GET_PRIVATE (self)->composited)
	{
		gtk_widget_shape_combine_mask (GET_PRIVATE (self)->widget,
					       NULL,
					       0,
					       0);
		return;
	}

	/* guess we need one */
	width = GET_PRIVATE (self)->widget->allocation.width;
	height = GET_PRIVATE (self)->widget->allocation.height;
	mask = (GdkBitmap*) gdk_pixmap_new (NULL, width, height, 1);
	if (mask)
	{
		/* create context from mask/pixmap */
		cr = gdk_cairo_create (mask);
		if (cairo_status (cr) == CAIRO_STATUS_SUCCESS)
		{
			/* clear mask/context */
			cairo_scale (cr, 1.0f, 1.0f);
			cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
			cairo_paint (cr);

			/* draw rounded rectangle shape/mask */
			cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
			cairo_set_source_rgb (cr, 1.0f, 1.0f, 1.0f);
			draw_round_rect (cr,
					 1.0f,
					 EM2PIXELS (defaults_get_bubble_shadow_size (d), d),
					 EM2PIXELS (defaults_get_bubble_shadow_size (d), d),
					 EM2PIXELS (defaults_get_bubble_corner_radius (d), d),
					 EM2PIXELS (defaults_get_bubble_width (d), d),
					 (gdouble) bubble_get_height (self) -
					 2.0f * EM2PIXELS (defaults_get_bubble_shadow_size (d), d));
			cairo_fill (cr);
			cairo_destroy (cr);

			/* remove any current shape-mask */
			gtk_widget_shape_combine_mask (GET_PRIVATE (self)->widget,
						       NULL,
						       0,
						       0);

			/* set new shape-mask */
			gtk_widget_shape_combine_mask (GET_PRIVATE (self)->widget,
						       mask,
						       0,
						       0);
		}

		g_object_unref ((gpointer) mask);
	}
}

static void
composited_changed_handler (GtkWidget* window,
			    gpointer   data)
{
	Bubble* bubble;

	bubble = (Bubble*) G_OBJECT (data);

	GET_PRIVATE (bubble)->composited = gdk_screen_is_composited (
						gtk_widget_get_screen (window));

	update_shape (bubble);
}

void
draw_shadow (cairo_t* cr,
	     gdouble  width,
	     gdouble  height,
	     gint     shadow_radius,
	     gint     corner_radius)
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
	cairo_set_source_rgba (cr_surf, 0.0f, 0.0f, 0.0f, 0.75f);
	cairo_arc (cr_surf,
		   2 * shadow_radius,
		   2 * shadow_radius,
		   1.75f * corner_radius,
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

static void
_render_background (cairo_t*  cr,
		    Defaults* d,
		    gint      width,
		    gint      height,
		    Bubble*   bubble)
{
        /* clear and render drop-shadow and bubble-background */
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	if (GET_PRIVATE (bubble)->composited)
	{
		draw_shadow (cr,
			     width,
			     height,
			     EM2PIXELS (defaults_get_bubble_shadow_size (d),
					d),
			     EM2PIXELS (defaults_get_bubble_corner_radius (d),
					d));
		cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
		draw_round_rect (cr,
				 1.0f,
				 EM2PIXELS (defaults_get_bubble_shadow_size (d),
					    d),
				 EM2PIXELS (defaults_get_bubble_shadow_size (d),
					    d),
				 EM2PIXELS (defaults_get_bubble_corner_radius (d),
					    d),
				 EM2PIXELS (defaults_get_bubble_width (d),
					    d),
				 (gdouble) bubble_get_height (bubble) -
				 2.0f * EM2PIXELS (defaults_get_bubble_shadow_size (d),
						   d));
		cairo_fill (cr);
	}

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	if (GET_PRIVATE (bubble)->composited)
	{
		cairo_set_source_rgba (cr,
				       BUBBLE_BG_COLOR_R,
				       BUBBLE_BG_COLOR_G,
				       BUBBLE_BG_COLOR_B,
				       BUBBLE_BG_COLOR_A);
	}
	else
	{
		cairo_set_source_rgb (cr,
				      BUBBLE_BG_COLOR_R,
				      BUBBLE_BG_COLOR_G,
				      BUBBLE_BG_COLOR_B);
	}

	draw_round_rect (cr,
			 1.0f,
			 EM2PIXELS (defaults_get_bubble_shadow_size (d),
				    d),
			 EM2PIXELS (defaults_get_bubble_shadow_size (d),
				    d),
			 EM2PIXELS (defaults_get_bubble_corner_radius (d),
				    d),
			 EM2PIXELS (defaults_get_bubble_width (d),
				    d),
			 (gdouble) bubble_get_height (bubble) -
			 2.0f * EM2PIXELS (defaults_get_bubble_shadow_size (d),
					   d));
	cairo_fill (cr);
}

static
gboolean
expose_handler (GtkWidget*      window,
		GdkEventExpose* event,
		gpointer        data)
{
	Bubble*   bubble;
	cairo_t*  cr;
	gdouble   width  = (gdouble) window->allocation.width;
	gdouble   height = (gdouble) window->allocation.height;
	Defaults* d;

	bubble = (Bubble*) G_OBJECT (data);

	d = bubble->defaults;

	cr = gdk_cairo_create (window->window);

        /* clear bubble-background */
	cairo_scale (cr, 1.0f, 1.0f);
	cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint (cr);

        /* render drop-shadow and bubble-background */
	_render_background (cr, d, width, height, bubble);

	bubble_determine_layout (bubble);
	switch (bubble_get_layout (bubble))
	{
		case LAYOUT_ICON_INDICATOR:
			_render_icon_indicator (bubble, cr);
		break;

		case LAYOUT_ICON_TITLE:
			_render_icon_title (bubble, cr);
		break;

		case LAYOUT_ICON_TITLE_BODY:
			_render_icon_title_body (bubble, cr);
		break;

		case LAYOUT_TITLE_BODY:
			_render_title_body (bubble, cr);
		break;

		case LAYOUT_NONE:
			g_warning ("WARNING: No layout defined!!!\n");
		break;
	}

	/* draw_layout_grid (cr, bubble); */

	cairo_destroy (cr);

	return TRUE;
}

static
gboolean
redraw_handler (Bubble* bubble)
{
	GtkWindow* window;

	if (!bubble)
		return FALSE;

	if (!bubble_is_visible (bubble))
		return FALSE;

	if (!GET_PRIVATE (bubble)->composited)
		return TRUE;

	window = GTK_WINDOW (GET_PRIVATE(bubble)->widget);

	if (!GTK_IS_WINDOW (window))
		return FALSE;

	/* new mouse-over behaviour */	    
	if (bubble_is_mouse_over (bubble))
		gtk_window_set_opacity (window, 0.1f);
	else if (GET_PRIVATE(bubble)->alpha == NULL)
		gtk_window_set_opacity (window, 0.95f);

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
	if (GET_PRIVATE(gobject)->synchronous)
		g_debug ("** sync. bubble %p finalized", gobject);
	else
		g_debug ("** bubble %p finalized", gobject);

	cairo_status_t status;

	if (GTK_IS_WIDGET (BUBBLE (gobject)->priv->widget))
	{
		gtk_widget_destroy (GTK_WIDGET (BUBBLE (gobject)->priv->widget));
		BUBBLE (gobject)->priv->widget = NULL;
	}

    	if (GET_PRIVATE (gobject)->title)
	{
		g_string_free ((gpointer) GET_PRIVATE (gobject)->title,
			       TRUE);
		GET_PRIVATE (gobject)->title = NULL;
	}

    	if (GET_PRIVATE (gobject)->message_body)
	{
		g_string_free ((gpointer) GET_PRIVATE (gobject)->message_body,
			       TRUE);
		GET_PRIVATE (gobject)->message_body = NULL;
	}

	if (GET_PRIVATE (gobject)->icon_pixbuf)
	{
		g_object_unref (GET_PRIVATE (gobject)->icon_pixbuf);
		GET_PRIVATE (gobject)->icon_pixbuf = NULL;
	}

	if (GET_PRIVATE (gobject)->alpha)
	{
		g_object_unref (GET_PRIVATE (gobject)->alpha);
		GET_PRIVATE (gobject)->alpha = NULL;
	}

	if (GET_PRIVATE (gobject)->timeline)
	{
		g_object_unref (GET_PRIVATE (gobject)->timeline);
		GET_PRIVATE (gobject)->timeline = NULL;
	}

	if (GET_PRIVATE (gobject)->pointer_update_id)
	{
		g_source_remove (GET_PRIVATE (gobject)->pointer_update_id);
		GET_PRIVATE (gobject)->pointer_update_id = 0;
	}

	if (GET_PRIVATE (gobject)->draw_handler_id)
	{
		g_source_remove (GET_PRIVATE (gobject)->draw_handler_id);
		GET_PRIVATE (gobject)->draw_handler_id = 0;
	}

	if (GET_PRIVATE (gobject)->timer_id)
	{
		g_source_remove (GET_PRIVATE (gobject)->timer_id);
		GET_PRIVATE (gobject)->timer_id = 0;
	}

	if (GET_PRIVATE (gobject)->blurred_content)
	{
		status = cairo_surface_status (GET_PRIVATE (gobject)->blurred_content);
		if (status == CAIRO_STATUS_SUCCESS)
		{
			cairo_surface_destroy (GET_PRIVATE (gobject)->blurred_content);
			GET_PRIVATE (gobject)->blurred_content = NULL;
		}
	}

	if (GET_PRIVATE (gobject)->blurred_bubble)
	{
		status = cairo_surface_status (GET_PRIVATE (gobject)->blurred_bubble);
		if (status == CAIRO_STATUS_SUCCESS)
		{
			cairo_surface_destroy (GET_PRIVATE (gobject)->blurred_bubble);
			GET_PRIVATE (gobject)->blurred_bubble = NULL;
		}
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
	priv->layout           = LAYOUT_NONE;
	priv->title            = NULL;
	priv->message_body     = NULL;
	priv->visible          = FALSE;
	priv->icon_pixbuf      = NULL;
	priv->value            = -1;
	priv->synchronous      = NULL;
	priv->draw_handler_id  = 0;
	priv->pointer_update_id= 0;
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

/* the behind-bubble blur only works with the enabled/working compiz-plugin blur
 * by setting the hint _COMPIZ_WM_WINDOW_BLUR on the bubble-window, thanks to
 * the opacity-threshold of the blur-plugin we might not need to unset it for
 * fade-on-hover case */
// static
 void
_set_bg_blur (GtkWidget* window,
	      gboolean   set_blur)
{
	glong data[] = {2, /* threshold */
			0  /* filter    */};

	/* sanity check */
	if (!window)
		return;

	if (set_blur)
	{
		XChangeProperty (GDK_WINDOW_XDISPLAY (window->window),
				 GDK_WINDOW_XID (window->window),
				 XInternAtom (GDK_WINDOW_XDISPLAY (window->window),
					      "_COMPIZ_WM_WINDOW_BLUR",
					      FALSE),
				 XA_INTEGER,
				 32,
				 PropModeReplace,
				 (guchar *) data,
				 2);
	}
	else
	{
		/* FIXME: not sure yet if we really need to unset it, because
		 * the compiz blur-plugin has a opacity-threshold for applying
		 * the blur */
	}
}

/*-- public API --------------------------------------------------------------*/

Bubble*
bubble_new (Defaults* defaults)
{
	Bubble*    this   = NULL;
	GtkWidget* window = NULL;

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
	/* TODO: fix the default settings of compiz to avoid fade and other
	   effects to apply to our windows when we add this hint:
	   | GDK_WINDOW_TYPE_HINT_NOTIFICATION);
	*/

	gtk_widget_add_events (window,
			       GDK_POINTER_MOTION_MASK |
			       GDK_BUTTON_PRESS_MASK |
			       GDK_BUTTON_RELEASE_MASK);

	/* hook up input/event handlers to window */
	g_signal_connect (G_OBJECT (window),
			  "screen-changed",
			  G_CALLBACK (screen_changed_handler),
			  NULL);
	g_signal_connect (G_OBJECT (window),
			  "composited-changed",
			  G_CALLBACK (composited_changed_handler),
			  this);

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
	gtk_widget_set_app_paintable (window, TRUE);
	gtk_window_set_title (GTK_WINDOW (window), "notification");
	gtk_window_set_decorated (GTK_WINDOW (window), FALSE);
	gtk_window_set_keep_above (GTK_WINDOW (window), TRUE);
	gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
	gtk_window_set_accept_focus (GTK_WINDOW (window), FALSE);
	gtk_window_set_opacity (GTK_WINDOW (window), 0.0f);

	/* TODO: fold some of that back into bubble_init */
	this->priv = GET_PRIVATE (this);
	GET_PRIVATE(this)->layout        = LAYOUT_NONE;
	GET_PRIVATE(this)->widget        = window;
	GET_PRIVATE(this)->title         = g_string_new ("");
	GET_PRIVATE(this)->message_body  = g_string_new ("");
	GET_PRIVATE(this)->icon_pixbuf   = NULL;
	GET_PRIVATE(this)->value         = -1;
	GET_PRIVATE(this)->visible       = FALSE;
	GET_PRIVATE(this)->timeout       = 2;
	GET_PRIVATE(this)->mouse_over    = FALSE;
	GET_PRIVATE(this)->start_y       = 0;
	GET_PRIVATE(this)->end_y         = 0;
	GET_PRIVATE(this)->inc_factor    = 0.0f;
	GET_PRIVATE(this)->delta_y       = 0;
	GET_PRIVATE(this)->composited    = gdk_screen_is_composited (
						gtk_widget_get_screen (window));
	GET_PRIVATE(this)->alpha         = NULL;
	GET_PRIVATE(this)->timeline      = NULL;
	GET_PRIVATE(this)->blurred_content = NULL;
	GET_PRIVATE(this)->blurred_bubble  = NULL;
	GET_PRIVATE(this)->title_width     = 0;
	GET_PRIVATE(this)->title_height    = 0;
	GET_PRIVATE(this)->body_width      = 0;
	GET_PRIVATE(this)->body_height     = 0;

	update_shape (this);
	update_input_shape (window, 1, 1);

	_set_bg_blur (window, TRUE);

	return this;
}

gchar*
bubble_get_synchronous (Bubble* self)
{
	g_return_val_if_fail (IS_BUBBLE (self), NULL);

	return GET_PRIVATE (self)->synchronous;
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

	if (GET_PRIVATE (self)->title->len != 0)
		g_string_free (GET_PRIVATE (self)->title, TRUE);

	GET_PRIVATE (self)->title = g_string_new (title);
}

void
bubble_set_message_body (Bubble*      self,
			 const gchar* body)
{
	gboolean result;
	gchar*   text;
	GError*  error = NULL;

	if (!self || !IS_BUBBLE (self))
		return;

	if (GET_PRIVATE (self)->message_body->len != 0)
		g_string_free (GET_PRIVATE (self)->message_body, TRUE);

	/* filter out any HTML/markup if possible */
    	result = pango_parse_markup (body,
				     -1,
				     0,    /* no accel-marker needed */
				     NULL, /* no PangoAttr needed */
				     &text,
				     NULL, /* no accel-marker-return needed */
				     &error);

	GET_PRIVATE (self)->message_body = g_string_new (text);
	g_free ((gpointer) text);
}

void
bubble_set_icon (Bubble*      self,
		 const gchar* filename)
{
	Defaults* d;

 	if (!self || !IS_BUBBLE (self))
		return;

	if (GET_PRIVATE (self)->icon_pixbuf)
	{
		g_object_unref (GET_PRIVATE (self)->icon_pixbuf);
		GET_PRIVATE (self)->icon_pixbuf = NULL;
	}

	d = self->defaults;
	GET_PRIVATE (self)->icon_pixbuf = load_icon (
						filename,
						EM2PIXELS (defaults_get_icon_size (d), d));
}

void
bubble_set_icon_from_pixbuf (Bubble*    self,
			     GdkPixbuf* pixbuf)
{
	GdkPixbuf* scaled;
	gint       height;
	gint       width;
	Defaults*  d;

 	if (!self || !IS_BUBBLE (self) || !pixbuf)
		return;

	if (GET_PRIVATE (self)->icon_pixbuf)
	{
		g_object_unref (GET_PRIVATE (self)->icon_pixbuf);
		GET_PRIVATE (self)->icon_pixbuf = NULL;
	}

	height = gdk_pixbuf_get_height (pixbuf);
	width = gdk_pixbuf_get_width (pixbuf);

	d = self->defaults;

	if (width != defaults_get_icon_size (d))
	{
		if (width != height)
			g_warning ("non-square pixmap");
		/* TODO: improve scaling for non-square pixmaps */

		scaled = gdk_pixbuf_scale_simple (pixbuf,
						  EM2PIXELS (defaults_get_icon_size (d), d),
						  EM2PIXELS (defaults_get_icon_size (d), d),
						  GDK_INTERP_HYPER);
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

	/* FIXME: do nasty busy-polling rendering in the drawing-area */
	GET_PRIVATE (self)->draw_handler_id
		= g_timeout_add (1000/60,
				 (GSourceFunc) redraw_handler,
				 self);

	/* FIXME: read out current mouse-pointer position every 1/10 second */

        GET_PRIVATE (self)->pointer_update_id
		= g_timeout_add (100,
				 (GSourceFunc) pointer_update,
				 self);
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


static inline gboolean
bubble_is_composited (Bubble *bubble)
{
	/* no g_return_if_fail(), the caller should have already
	   checked that */
	return gtk_widget_is_composited (GET_PRIVATE (bubble)->widget);
}

static inline GtkWindow*
bubble_get_window (Bubble *bubble)
{
	return GTK_WINDOW (GET_PRIVATE (bubble)->widget);
}

static void
fade_cb (ClutterTimeline *timeline,
	 gint frame_no,
	 Bubble *bubble)
{
	float opacity;

	g_return_if_fail (IS_BUBBLE (bubble));

	opacity = (float)clutter_alpha_get_alpha (GET_PRIVATE (bubble)->alpha)
		/ (float)CLUTTER_ALPHA_MAX_ALPHA
		* 0.95f;

	gtk_window_set_opacity (bubble_get_window (bubble), opacity);
}

static void
fade_out_completed_cb (ClutterTimeline *timeline,
		       Bubble *bubble)
{
	g_return_if_fail (IS_BUBBLE (bubble));

	bubble_hide (bubble);

	g_signal_emit (bubble, g_bubble_signals[TIMED_OUT], 0);	
}


static void
fade_in_completed_cb (ClutterTimeline *timeline,
		      Bubble *bubble)
{
	g_return_if_fail (IS_BUBBLE (bubble));

	/* get rid of the alpha, so that the mouse-over algorithm notices */
	if (GET_PRIVATE (bubble)->alpha)
	{
		g_object_unref (GET_PRIVATE (bubble)->alpha);
		GET_PRIVATE (bubble)->alpha = NULL;
	}

	if (GET_PRIVATE (bubble)->timeline)
	{
		g_object_unref (GET_PRIVATE (bubble)->timeline);
		GET_PRIVATE (bubble)->timeline = NULL;
	}

	gtk_window_set_opacity (bubble_get_window (bubble), 0.95f);

	bubble_start_timer (bubble);
}

void
bubble_fade_in (Bubble *self,
		guint   msecs)
{
	ClutterTimeline *timeline;

	g_return_if_fail (IS_BUBBLE (self));

	if (!bubble_is_composited (self))
	{
		bubble_show (self);
		bubble_start_timer (self);
		return;
	}

	timeline = clutter_timeline_new_for_duration (msecs);

	if (GET_PRIVATE (self)->alpha != NULL)
	{
		g_object_unref (GET_PRIVATE (self)->alpha);
		GET_PRIVATE (self)->alpha = NULL;
	}

	GET_PRIVATE (self)->alpha =
		clutter_alpha_new_full (timeline,
					CLUTTER_ALPHA_RAMP_INC,
					NULL,
					NULL);
	g_object_unref (timeline);

	g_signal_connect (G_OBJECT (timeline),
			  "completed",
			  G_CALLBACK (fade_in_completed_cb),
			  self);

	g_signal_connect (G_OBJECT (timeline),
			  "new-frame",
			  G_CALLBACK (fade_cb),
			  self);

	clutter_timeline_start (timeline);

	gtk_window_set_opacity (bubble_get_window (self), 0.0);
	bubble_show (self);	
}

void
bubble_fade_out (Bubble *self,
		 guint   msecs)
{
	ClutterTimeline *timeline;

	g_return_if_fail (IS_BUBBLE (self));

	timeline = clutter_timeline_new_for_duration (msecs);
	GET_PRIVATE (self)->timeline = timeline;

	if (GET_PRIVATE (self)->alpha != NULL)
	{
		g_object_unref (GET_PRIVATE (self)->alpha);
		GET_PRIVATE (self)->alpha = NULL;
	}

	GET_PRIVATE (self)->alpha =
		clutter_alpha_new_full (timeline,
					CLUTTER_ALPHA_RAMP_DEC,
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

	clutter_timeline_start (timeline);
}

gboolean
bubble_timed_out (Bubble* self)
{
	g_return_val_if_fail (IS_BUBBLE (self), FALSE);

	bubble_set_timeout (self, 0);

	if (! GET_PRIVATE (self)->composited)
	{
		bubble_hide (self);
		return FALSE;
	}

	bubble_fade_out (self, 300);

	return FALSE;
}


gboolean
bubble_hide (Bubble* self)
{
	if (!self || !IS_BUBBLE (self) || !bubble_is_visible (self))
		return FALSE;

	GET_PRIVATE (self)->visible = FALSE;
	gtk_widget_hide (GET_PRIVATE (self)->widget);

	if (GET_PRIVATE (self)->timeline)
	{
		clutter_timeline_stop (GET_PRIVATE (self)->timeline);
		g_object_unref (GET_PRIVATE (self)->timeline);
		GET_PRIVATE (self)->timeline = NULL;
	}

	if (GET_PRIVATE (self)->alpha)
	{
		g_object_unref (GET_PRIVATE (self)->alpha);
		GET_PRIVATE (self)->alpha = NULL;
	}

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
bubble_start_timer (Bubble* self)
{
	guint timer_id;

	if (!self || !IS_BUBBLE (self))
		return;

	timer_id = bubble_get_timer_id (self);
	if (timer_id > 0)
		g_source_remove (timer_id);

	/* and now let the timer tick... we use g_timeout_add_seconds() here
	** because for the bubble timeouts microsecond-precise resolution is not
	** needed, furthermore this also allows glib more room for optimizations
	** and improve system-power-usage to be more efficient */
	bubble_set_timer_id (
		self,
		g_timeout_add_seconds (bubble_get_timeout (self),
				       (GSourceFunc) bubble_timed_out,
				       self));
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

	if (!self || !IS_BUBBLE (self))
		return 0;

	d = self->defaults;

	surface = cairo_image_surface_create (CAIRO_FORMAT_A1, 1, 1);
	if (cairo_surface_status (surface) != CAIRO_STATUS_SUCCESS)
		return 0;

	cr = cairo_create (surface);
	cairo_surface_destroy (surface);
	if (cairo_status (cr) != CAIRO_STATUS_SUCCESS)
		return 0;

	layout = pango_cairo_create_layout (cr);
	desc = pango_font_description_new ();
	pango_font_description_set_size (
		desc,
		EM2PIXELS (defaults_get_text_title_size (d), d) *
		PANGO_SCALE);

	pango_font_description_set_family_static (
		desc,
		defaults_get_text_font_face (d));

	pango_font_description_set_weight (
		desc,
		defaults_get_text_title_weight (d));

	pango_font_description_set_style (desc, PANGO_STYLE_NORMAL);
	pango_layout_set_wrap (layout, PANGO_WRAP_WORD);
	pango_layout_set_font_description (layout, desc);
	pango_font_description_free (desc);

	pango_layout_set_width (layout, title_width);
	pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);

	pango_layout_set_text (
		layout,
		GET_PRIVATE (self)->title->str,
		GET_PRIVATE (self)->title->len);

	pango_layout_get_extents (layout, NULL, &log_rect);
	title_height = log_rect.height / PANGO_SCALE;
	g_object_unref (layout);
	cairo_destroy (cr);

	return title_height;
}

gint
_calc_body_height (Bubble* self,
		   gint    body_width /* requested text-width in pixels */)
{
	Defaults*             d;
	cairo_surface_t*      surface;
	cairo_t*              cr;
	PangoFontDescription* desc    = NULL;
	PangoLayout*          layout  = NULL;
	PangoRectangle        log_rect;
	gint                  body_height;
	gint                  h;

	if (!self || !IS_BUBBLE (self))
		return 0;

	d = self->defaults;

	surface = cairo_image_surface_create (CAIRO_FORMAT_A1, 1, 1);
	if (cairo_surface_status (surface) != CAIRO_STATUS_SUCCESS)
		return 0;

	/*cr = cairo_create (surface);*/
	cr = gdk_cairo_create (GET_PRIVATE (self)->widget->window);
	cairo_surface_destroy (surface);
	if (cairo_status (cr) != CAIRO_STATUS_SUCCESS)
		return 0;

	layout = pango_cairo_create_layout (cr);
	desc = pango_font_description_new ();
	pango_font_description_set_size (
		desc,
		EM2PIXELS (defaults_get_text_body_size (d), d) *
		PANGO_SCALE);

	pango_font_description_set_family_static (
		desc,
		defaults_get_text_font_face (d));

	pango_font_description_set_weight (
		desc,
		defaults_get_text_body_weight (d));

	pango_font_description_set_style (desc, PANGO_STYLE_NORMAL);
	pango_layout_set_wrap (layout, PANGO_WRAP_WORD);
	pango_layout_set_font_description (layout, desc);

	pango_layout_set_text (
		layout,
		GET_PRIVATE (self)->message_body->str,
		GET_PRIVATE (self)->message_body->len);
	pango_layout_set_width (layout, body_width * PANGO_SCALE);

	pango_layout_get_extents (layout, NULL, &log_rect);
	body_height = PANGO_PIXELS (log_rect.height);

	pango_layout_get_size (layout, NULL, &h);
	pango_layout_get_line_count (layout),

	pango_font_description_free (desc);
	g_object_unref (layout);
	cairo_destroy (cr);

	return body_height;
}

void
bubble_recalc_size (Bubble *self)
{
	gint      new_bubble_width  = 0;
	gint      new_bubble_height = 0;
	Defaults* d;

	if (!self || !IS_BUBBLE (self))
		return;

	d = self->defaults;

	bubble_determine_layout (self);

	new_bubble_width =
		EM2PIXELS (defaults_get_bubble_width (d), d) +
		2 * EM2PIXELS (defaults_get_bubble_shadow_size (d), d);

	switch (GET_PRIVATE (self)->layout)
	{
		case LAYOUT_ICON_INDICATOR:
		case LAYOUT_ICON_TITLE:
			new_bubble_height =
				EM2PIXELS (defaults_get_bubble_min_height (d), d) +
				2.0f * EM2PIXELS (defaults_get_bubble_shadow_size (d), d);
		break;

		case LAYOUT_ICON_TITLE_BODY:
		{
			gdouble available_height = 0.0f;
			gdouble bubble_height    = 0.0f;

			GET_PRIVATE (self)->title_width =
				EM2PIXELS (defaults_get_bubble_width (d), d) -
				3 * EM2PIXELS (defaults_get_margin_size (d), d) -
				EM2PIXELS (defaults_get_icon_size (d), d);

			GET_PRIVATE (self)->title_height = _calc_title_height (
					self,
					GET_PRIVATE (self)->title_width);

			GET_PRIVATE (self)->body_width =
				EM2PIXELS (defaults_get_bubble_width (d), d) -
				3 * EM2PIXELS (defaults_get_margin_size (d), d) -
				EM2PIXELS (defaults_get_icon_size (d), d);

			GET_PRIVATE (self)->body_height = _calc_body_height (
					self,
					GET_PRIVATE(self)->body_width);

			available_height = PIXELS2EM (defaults_get_desktop_height (d), d) -
					   defaults_get_desktop_bottom_gap (d) -
					   defaults_get_bubble_min_height (d) -
					   2.0f * defaults_get_bubble_vert_gap (d);

			bubble_height =
				PIXELS2EM (GET_PRIVATE (self)->title_height, d) +
				PIXELS2EM (GET_PRIVATE (self)->body_height, d) +
				2.0f * defaults_get_margin_size (d);

			if (bubble_height >= available_height)
			{
				new_bubble_height = EM2PIXELS (available_height, d);
			}
			else
			{
				new_bubble_height =
					GET_PRIVATE (self)->body_height +
					GET_PRIVATE (self)->title_height +
					2.0f * EM2PIXELS (defaults_get_margin_size (d), d) +
					2.0f * EM2PIXELS (defaults_get_bubble_shadow_size (d), d);
			}
		}
		break;

		case LAYOUT_TITLE_BODY:
		{
			gdouble available_height = 0.0f;
			gdouble bubble_height    = 0.0f;

			GET_PRIVATE (self)->title_width =
				EM2PIXELS (defaults_get_bubble_width (d), d) -
				2 * EM2PIXELS (defaults_get_margin_size (d), d);

			GET_PRIVATE (self)->title_height = _calc_title_height (
					self,
					GET_PRIVATE (self)->title_width);

			GET_PRIVATE (self)->body_width = 
				EM2PIXELS (defaults_get_bubble_width (d), d) -
				2 * EM2PIXELS (defaults_get_margin_size (d), d);

			GET_PRIVATE (self)->body_height = _calc_body_height (
					self,
					GET_PRIVATE (self)->body_width);

			available_height = PIXELS2EM (defaults_get_desktop_height (d), d) -
					   defaults_get_desktop_bottom_gap (d) -
					   defaults_get_bubble_min_height (d) -
					   2.0f * defaults_get_bubble_vert_gap (d);

			bubble_height =
				PIXELS2EM (GET_PRIVATE (self)->title_height, d) +
				PIXELS2EM (GET_PRIVATE (self)->body_height, d) +
				2.0f * defaults_get_margin_size (d);

			if (bubble_height >= available_height)
			{
				new_bubble_height = EM2PIXELS (available_height, d);
			}
			else
			{
				new_bubble_height =
					GET_PRIVATE (self)->body_height +
					GET_PRIVATE (self)->title_height +
					2.0f * EM2PIXELS (defaults_get_margin_size (d), d) +
					2.0f * EM2PIXELS (defaults_get_bubble_shadow_size (d), d);
			}
		}
		break;

		case LAYOUT_NONE:
			g_warning ("bubble_recalc_size(): WARNING, no layout!!!\n");
		break;
	}
	bubble_set_size (self, new_bubble_width, new_bubble_height);
}

void
bubble_set_synchronous (Bubble *self,
			const gchar *sync)
{
	g_return_if_fail (IS_BUBBLE (self));

	GET_PRIVATE (self)->synchronous = g_strdup (sync);
}

gboolean
bubble_is_synchronous (Bubble *self)
{
	if (!self || !IS_BUBBLE (self))
		return FALSE;

	return (GET_PRIVATE (self)->synchronous != NULL);
}

gboolean
bubble_is_urgent (Bubble *self)
{
	g_return_val_if_fail (IS_BUBBLE (self), FALSE);

	return GET_PRIVATE (self)->urgent;
}

void
bubble_set_urgent (Bubble *self,
		   gboolean urgent)
{
	g_return_if_fail (IS_BUBBLE (self));

	GET_PRIVATE (self)->urgent = urgent;
}
