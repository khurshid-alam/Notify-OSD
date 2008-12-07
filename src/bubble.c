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
#include <librsvg/rsvg.h>
#include <librsvg/rsvg-cairo.h>

#include "bubble.h"
#include "stack.h"

G_DEFINE_TYPE (Bubble, bubble, G_TYPE_OBJECT);

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BUBBLE_TYPE, BubblePrivate))

struct _BubblePrivate {
	GtkWidget*       widget;
	gchar*           title;
	gchar*           message_body;
	guint            id;
	GdkPixbuf*       icon_pixbuf;
	gboolean         visible;
	guint            timeout;
	gint             sliding_to_x;
	gint             sliding_to_y;
	gint             inc_x;
	gint             inc_y;
	gint             value; /* "empty": -1, valid range: 0 - 100 */
};

/*-- private functions  --------------------------------------------------------------*/

double   g_alpha   = 0.95f;
gboolean g_entered = FALSE;
gboolean g_left    = FALSE;
gint     g_pointer[2];

static void
draw_round_rect (cairo_t* cr,
		 double   aspect,         /* aspect-ratio            */
		 double   x,              /* top-left corner         */
		 double   y,              /* top-left corner         */
		 double   corner_radius,  /* "size" of the corners   */
		 double   width,          /* width of the rectangle  */
		 double   height          /* height of the rectangle */)
{
	double radius = corner_radius / aspect;

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
gboolean
delete_handler (GtkWidget* window,
		GdkEvent*  event,
		gpointer   data)
{
	gtk_main_quit ();
	return TRUE;
}

static
gboolean
key_press_handler (GtkWidget*   window,
		   GdkEventKey* event,
		   gpointer     data)
{
	if (event->type   == GDK_KEY_PRESS &&
	    (event->keyval == GDK_q || event->keyval == GDK_Escape))
	{
		delete_handler (window, NULL, NULL);
	}

	return TRUE;
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

static
gboolean
expose_handler (GtkWidget*      window,
		GdkEventExpose* event,
		gpointer        data)
{
	Bubble*  bubble;
	cairo_t* cr;

	bubble = (Bubble*) G_OBJECT (data);

	cr = gdk_cairo_create (window->window);

        /* clear and render bubble-background */
	cairo_scale (cr, 1.0f, 1.0f);
	cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint (cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	cairo_set_source_rgba (cr, 0.05f, 0.05f, 0.05f, g_alpha);
	draw_round_rect (cr,
			 1.0f,
			 0.0f, 0.0f,
			 10.0f,
			 (gdouble) window->allocation.width,
			 (gdouble) window->allocation.height);
	cairo_fill (cr);
	cairo_set_source_rgba (cr, 0.15f, 0.15f, 0.15f, g_alpha);
	draw_round_rect (cr,
			 1.0f,
			 1.0f, 1.0f,
			 9.5f,
			 (gdouble) window->allocation.width - 2.0f,
			 (gdouble) window->allocation.height / 2.0f);
	cairo_fill (cr);

	/* render title */
	if (GET_PRIVATE (bubble)->title)
	{
		PangoFontDescription* desc   = NULL;
		PangoLayout*          layout = NULL;

		layout = pango_cairo_create_layout (cr);
		desc = pango_font_description_new ();
		pango_font_description_set_absolute_size (desc, 16 * PANGO_SCALE);
		pango_font_description_set_family_static (desc, "Candara");
		pango_font_description_set_weight (desc, PANGO_WEIGHT_BOLD);
		pango_font_description_set_style (desc, PANGO_STYLE_NORMAL);
		pango_layout_set_wrap (layout, PANGO_WRAP_WORD);
		pango_layout_set_font_description (layout, desc);
		pango_font_description_free (desc);
		pango_layout_set_width (layout, 210 * PANGO_SCALE);

		/* print and layout string (pango-wise) */
		pango_layout_set_text (layout, GET_PRIVATE (bubble)->title, -1);

		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
		/*cairo_set_line_width (cr, 0.0025f);*/
		cairo_move_to (cr, 80.0f, 15.0f);

		/* draw pango-text as path to our cairo-context */
		pango_cairo_layout_path (cr, layout);

		cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 0.9f);
		cairo_fill (cr);
		g_object_unref (layout);
	}

	/* render body-message */
	if (GET_PRIVATE (bubble)->message_body)
	{
		PangoFontDescription* desc   = NULL;
		PangoLayout*          layout = NULL;

		layout = pango_cairo_create_layout (cr);
		desc = pango_font_description_new ();
		pango_font_description_set_absolute_size (desc, 12 * PANGO_SCALE);
		pango_font_description_set_family_static (desc, "Candara");
		pango_font_description_set_weight (desc, PANGO_WEIGHT_NORMAL);
		pango_font_description_set_style (desc, PANGO_STYLE_NORMAL);
		pango_layout_set_wrap (layout, PANGO_WRAP_WORD);
		pango_layout_set_font_description (layout, desc);
		pango_font_description_free (desc);
		pango_layout_set_width (layout, 200 * PANGO_SCALE);

		/* print and layout string (pango-wise) */
		pango_layout_set_text (layout,
				       GET_PRIVATE (bubble)->message_body,
				       -1);

		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
		/*cairo_set_line_width (cr, 0.0025f);*/
		cairo_move_to (cr, 80.0f, 35.0f);

		/* draw pango-text as path to our cairo-context */
		pango_cairo_layout_path (cr, layout);
		cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 0.5f);
		cairo_fill (cr);
		g_object_unref (layout);
	}

	/* render icon */
	if (GET_PRIVATE (bubble)->icon_pixbuf)
	{
		gdk_cairo_set_source_pixbuf (cr,
					     GET_PRIVATE (bubble)->icon_pixbuf,
					     15.0f,
					     20.0f);
		cairo_paint (cr);
	}

	/* render value */
	if (GET_PRIVATE (bubble)->value >= 0 &&
	    GET_PRIVATE (bubble)->value <= 100)
	{
		gint    step;
		gdouble x        = (gdouble) window->allocation.width / 3.5f;
		gdouble y        = (gdouble) window->allocation.height / 1.5f;
		gdouble w        = (gdouble) window->allocation.width / 2.5f;
		gdouble h        = (gdouble) window->allocation.height / 2.0f;
		gdouble radius   = 3.0f;
		gdouble x_gap    = 3.0f;
		gdouble y_start  = 0.1f;
		gdouble x_step;
		gdouble y_step;

		x_step = (w - (8 * x_gap))/ 9;
		y_step = (h - (h * y_start)) / 9;

		for (step = 1; step < 10; step++)
		{
			if (step * 10 >= GET_PRIVATE (bubble)->value)
			{
				cairo_set_source_rgba (cr,
						       0.3f,
						       0.3f,
						       0.3f,
						       0.5f);
			}
			else
			{
				cairo_set_source_rgb (cr, 1.0f, 0.5f, 0.25f);
			}

			draw_round_rect (cr,
					 1.0f,
					 x + (x_step + x_gap) * (gdouble) step,
					 y - y_step * (gdouble) step,
					 radius,
					 x_step,
					 y_start * h + y_step * (gdouble) step);
			cairo_fill (cr);
		}
	}

	cairo_destroy (cr);

	return TRUE;
}

static
gboolean
redraw_handler (GtkWidget* window)
{
	if (!GTK_IS_WINDOW (window))
		return FALSE;

	if (g_left && g_alpha < 0.95f)
		g_alpha += 0.05f;

	if (g_entered && g_alpha > 0.1f)
		g_alpha -= 0.05f;

	gtk_window_set_opacity (GTK_WINDOW (window), g_alpha);

	return TRUE;
}

static
GdkPixbuf*
load_bitmap_icon (const gchar* filename)
{
	/*cairo_surface_t*  surf   = NULL;*/
	gint              width  = BUBBLE_ICON_WIDTH;
	gint              height = BUBBLE_ICON_HEIGHT;
	GError*           error  = NULL;
	GdkPixbuf*        pixbuf = NULL;

	/* sanity check */
	if (!filename)
		return NULL;

	/* load image into pixbuf */
	pixbuf = gdk_pixbuf_new_from_file_at_scale (filename,
						    width,
						    height,
						    TRUE,
						    &error);
	if (!pixbuf)
		return NULL;

	return pixbuf;
}

static
gboolean
pointer_update (GtkWidget* window)
{
	gint pointer_rel_x;
	gint pointer_rel_y;
	gint pointer_abs_x;
	gint pointer_abs_y;
	gint win_x;
	gint win_y;
	gint width;
	gint height;

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
			g_entered = TRUE;
			g_left    = FALSE;
		}
		else
		{
			g_entered = FALSE;
			g_left    = TRUE;
		}
	}

	return TRUE;
}

/*static
void
calculate_default_size (guint *width,
			guint *height)
{
	* FIXME: Mirco, please implement the cairo code here *
	*width  = 250;
	*height = 100;
}*/

/*static
void
calculate_default_position (guint *x,
			    guint *y)
{
	* FIXME: Mirco, please implement the cairo code here *
	*x = 1400 - 250 - 10;
	*y = 30;
}*/



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
}

/*-- public API --------------------------------------------------------------*/

Bubble*
bubble_new (void)
{
	Bubble*         this              = NULL;
	GtkWidget*      window            = NULL;
	guint           draw_handler_id   = 0;
	guint           pointer_update_id = 0;
	gint            x                 = 0;
	gint            y                 = 0;
	gint            width             = 0;
	gint            height            = 0;

	this = g_object_new (BUBBLE_TYPE, NULL);
	if (!this)
		return NULL;

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
			  "delete-event",
			  G_CALLBACK (delete_handler),
			  NULL);
	g_signal_connect (G_OBJECT (window),
			  "key-press-event",
			  G_CALLBACK (key_press_handler),
			  NULL);
	g_signal_connect (G_OBJECT (window),
			  "screen-changed",
			  G_CALLBACK (screen_changed_handler),
			  NULL);

	/* size and position the window depending on the screen resolution */
	/*calculate_default_size (&width, &height);
	calculate_default_position (&x, &y);*/
	gtk_widget_set_size_request (window, width, height);
	
	gtk_window_move (GTK_WINDOW (window), x, y);

	/* make sure the window opens with a RGBA-visual */
	screen_changed_handler (window, NULL, NULL);
	gtk_widget_realize (window);
	gdk_window_set_back_pixmap (window->window, NULL, FALSE);

	/* hook up window-event handlers to window */
	g_signal_connect (G_OBJECT (window),
			  "expose-event",
			  G_CALLBACK (expose_handler),
			  this);

	/* FIXME: do nasty busy-polling rendering in the drawing-area */
	draw_handler_id = g_timeout_add (1000/60,
					 (GSourceFunc) redraw_handler,
					 window);

	/* FIXME: read out current mouse-pointer position every 1/10 second */
        pointer_update_id = g_timeout_add (100,
					   (GSourceFunc) pointer_update,
					   window);
       

	/*  "clear" input-mask, set title/icon/attributes */
	update_input_shape (window, 1, 1);
	gtk_widget_set_app_paintable (window, TRUE);
	gtk_window_set_title (GTK_WINDOW (window), "notification-test-cairo");
	gtk_window_set_decorated (GTK_WINDOW (window), FALSE);
	gtk_window_set_keep_above (GTK_WINDOW (window), TRUE);
	gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
	gtk_window_set_accept_focus (GTK_WINDOW (window), FALSE);
	
	this->priv = GET_PRIVATE (this);
	GET_PRIVATE(this)->widget       = window;
	GET_PRIVATE(this)->title        = g_strdup("GTK+ Notification");
	GET_PRIVATE(this)->message_body = g_strdup("Courtesy of the new Canonical notification sub-system");
	GET_PRIVATE(this)->visible      = FALSE;
	GET_PRIVATE(this)->timeout      = 10*1000; /* 10s */
	GET_PRIVATE(this)->sliding_to_x = -1;
	GET_PRIVATE(this)->sliding_to_y = -1;

	return this;
}


void
bubble_set_title (Bubble*      self,
		  const gchar* title)
{
	if (!self)
		return;

	GET_PRIVATE (self)->title = g_strdup (title);
}

void
bubble_set_message_body (Bubble*      self,
			 const gchar* body)
{
	if (!self)
		return;

	GET_PRIVATE (self)->message_body = g_strdup (body);
}

void
bubble_set_icon (Bubble*      self,
		 const gchar* filename)
{
	if (!self)
		return;

	if (GET_PRIVATE (self)->icon_pixbuf)
		g_object_unref (GET_PRIVATE (self)->icon_pixbuf);

	GET_PRIVATE (self)->icon_pixbuf = load_bitmap_icon (filename);
}

void
bubble_set_value (Bubble* self,
		  gint    value)
{
	if (!self)
		return;

	GET_PRIVATE (self)->value = value;
}

void
bubble_set_size (Bubble* self,
		 gint    width,
		 gint    height)
{
	if (!self)
		return;

	gtk_widget_set_size_request (GET_PRIVATE(self)->widget, width, height);
}

void
bubble_move (Bubble* self,
	     gint    x,
	     gint    y)
{
	if (!self)
		return;

	gtk_window_move (GTK_WINDOW (GET_PRIVATE (self)->widget), x, y);
}

void
bubble_show (Bubble* self)
{
	/* TODO: move that into the Bubble gobject */
	guint timer_id = 0;

	if (!self)
		return;

	GET_PRIVATE (self)->visible = TRUE;
	gtk_widget_show_all (GET_PRIVATE (self)->widget);

	/* and now let the timer tick... */
	timer_id = g_timeout_add (GET_PRIVATE (self)->timeout,
				  (GSourceFunc) bubble_hide,
				  self);
}

static
gboolean
do_slide_bubble (Bubble *self)
{
	gint x         = 0;
	gint y         = 0;

	if (!self) return FALSE;

	gtk_window_get_position (GTK_WINDOW (GET_PRIVATE (self)->widget),
				 &x, &y);

	/* check if we arrived at the destination */
	if ((x == GET_PRIVATE (self)->sliding_to_x) &&
	    (y == GET_PRIVATE (self)->sliding_to_y))
		return FALSE;

#if 0
	g_printf ("moving from (%d, %d) to (%d, %d) by (%d, %d)\n",
		  x, y,
		  GET_PRIVATE (self)->sliding_to_x,
		  GET_PRIVATE (self)->sliding_to_y,
		  GET_PRIVATE (self)->inc_x,
		  GET_PRIVATE (self)->inc_y);
#endif
	bubble_move(self,
		    x + GET_PRIVATE (self)->inc_x,
		    y + GET_PRIVATE (self)->inc_y);

	return TRUE; /* keep going */
}

void
bubble_slide_to (Bubble* self,
		 gint    x,
		 gint    y)
{
	guint timer_id = 0;
	gint delta_x   = 0;
	gint delta_y   = 0;
	gint inc_x   = 0;
	gint inc_y   = 0;

	if (!self) return;

	GET_PRIVATE (self)->sliding_to_x = x;
	GET_PRIVATE (self)->sliding_to_y = y;

	/* determine direction to take */
	gtk_window_get_position (GTK_WINDOW (GET_PRIVATE (self)->widget),
				 &x, &y);

	delta_x = GET_PRIVATE (self)->sliding_to_x - x;
	delta_y = GET_PRIVATE (self)->sliding_to_y - y;
	inc_x   = (GET_PRIVATE (self)->sliding_to_x - x) /
		abs( GET_PRIVATE (self)->sliding_to_x -x);
	inc_y   = (GET_PRIVATE (self)->sliding_to_y - y) /
		abs( GET_PRIVATE (self)->sliding_to_y -y);
	/* TODO: put some sin() here to get a smoother effect */
	inc_x   *= delta_x / 5;
	inc_y   *= delta_y / 5;

	GET_PRIVATE (self)->inc_x = inc_x;
	GET_PRIVATE (self)->inc_y = inc_y;

	/* and now let the timer tick... */
	timer_id = g_timeout_add (100,
				  (GSourceFunc) do_slide_bubble,
				  self);
}

gboolean
bubble_hide (Bubble* self)
{
	if (!self) return FALSE;

	GET_PRIVATE (self)->visible = FALSE;
	gtk_widget_hide (GET_PRIVATE (self)->widget);

	return FALSE; /* this also instructs the timer to stop */
}

void
bubble_set_id (Bubble* self,
	       guint   id)
{
	if (!self)
		return;

	GET_PRIVATE (self)->id = id;
}

guint
bubble_get_id (Bubble* self)
{
	if (!self)
		return 0;

	return GET_PRIVATE (self)->id;
}

gboolean
bubble_is_visible (Bubble* self)
{
	if (!self)
		return FALSE;

	return GET_PRIVATE (self)->visible;
}

void
bubble_reset_timeout (Bubble* self)
{
	if (!self)
		return;
}

void
bubble_get_position (Bubble* self,
		     gint*   x,
		     gint*   y)
{
	gtk_window_get_position (GTK_WINDOW (GET_PRIVATE (self)->widget),
				 x, y);
}
