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

#include "bubble.h"

G_DEFINE_TYPE (Bubble, bubble, G_TYPE_OBJECT);

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BUBBLE_TYPE, BubblePrivate))

struct _BubblePrivate {
	GtkWidget *widget;

	char *title;
	char *message_body;
};


/*-- private functions  --------------------------------------------------------------*/

double   g_alpha   = 0.95f;
gboolean g_entered = FALSE;
gboolean g_left    = FALSE;
gint     g_pointer[2];

static
void
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
	Bubble* bubble;
	cairo_t* cr;

	bubble = (Bubble *)G_OBJECT(data);

	cr = gdk_cairo_create (window->window);

	cairo_scale (cr, 1.0f, 1.0f);
	cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint (cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	cairo_set_source_rgba (cr, 0.05f, 0.05f, 0.05f, g_alpha);

	draw_round_rect (cr,
			 1.0f,
			 0.0f, 0.0f,
			 10.0f,
			 (double) window->allocation.width,
			 (double) window->allocation.height);

	cairo_fill (cr);

	cairo_select_font_face (cr,
				"DejaVu Sans",
				CAIRO_FONT_SLANT_NORMAL,
				CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size (cr, 24.0f);
	cairo_move_to (cr, 50.0f, 50.0f);
	cairo_text_path (cr, GET_PRIVATE(bubble)->title);
	cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 0.5f);
	cairo_fill (cr);

	cairo_destroy (cr);

	return TRUE;
}

static
gboolean
redraw_handler (GtkWidget* window)
{
	if (g_left && g_alpha < 0.95f)
		g_alpha += 0.05f;

	if (g_entered && g_alpha > 0.1f)
		g_alpha -= 0.05f;

	gtk_window_set_opacity (GTK_WINDOW (window), g_alpha);

	return TRUE;
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
	GError*         error             = NULL;
	guint           draw_handler_id   = 0;
	guint           pointer_update_id = 0;
	gint            x                 = 30; /* dummy defaults */
	gint            y                 = 30;
	gint            width             = 100;
	gint            height            = 50;


	this = g_object_new (BUBBLE_TYPE, NULL);
	if (!this)
		return NULL;
	
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	if (!window)
		return NULL;

	g_object_set_data (G_OBJECT(window), "bubble", (gpointer)&this);

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

	/* do nasty busy-polling rendering in the drawing-area */
	draw_handler_id = g_timeout_add (1000/60,
					 (GSourceFunc) redraw_handler,
					 window);

	/* read out current mouse-pointer position every 1/10 second */
        pointer_update_id = g_timeout_add (100,
					   (GSourceFunc) pointer_update,
					   window);

	/*  "clear" input-mask, set title/icon/attributes */
	update_input_shape (window, 1, 1);
	gtk_widget_set_app_paintable (window, TRUE);
	gtk_window_set_title (GTK_WINDOW (window), "notification-test-cairo");
	gtk_window_set_icon_from_file (GTK_WINDOW (window),
				       "cairo-logo.png",
				       &error);
	gtk_window_set_decorated (GTK_WINDOW (window), FALSE);
	gtk_window_set_keep_above (GTK_WINDOW (window), TRUE);
	gtk_window_set_resizable (GTK_WINDOW (window), FALSE);

	GET_PRIVATE(this)->widget           = window;
	GET_PRIVATE(this)->title            = g_strdup("GTK+ Notification");
	GET_PRIVATE(this)->message_body     = g_strdup("Courtesy of the new Canonical notification sub-system");

	return this;
}


void
bubble_set_title (Bubble* self,
		  char *title)
{
	GET_PRIVATE(self)->title = g_strdup(title);
}

void
bubble_set_message_body (Bubble* self,
			 char *body)
{
	GET_PRIVATE(self)->message_body = g_strdup(body);
}

void
bubble_set_size(Bubble* self,
	      gint width,
	      gint height)
{
	gtk_widget_set_size_request (GET_PRIVATE(self)->widget, width, height);
}

void
bubble_move (Bubble* self,
	     gint x,
	     gint y)
{
	gtk_window_move (GTK_WINDOW(GET_PRIVATE(self)->widget), x, y);
}

void
bubble_display (Bubble* self)
{
	gtk_widget_show_all (GET_PRIVATE(self)->widget);
}

void
bubble_hide (Bubble* self)
{
	gtk_widget_hide (GET_PRIVATE(self)->widget);
}

void
bubble_del (Bubble* self)
{
	g_object_unref (self);
	/* TODO: dispose of the struct members (widget, etc.) */
}

