/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
**      10        20        30        40        50        60        70        80
**
** project:
**    alsdorf
**
** file:
**    stack.c
**
** author(s):
**    Mirco "MacSlow" Mueller <mirco.mueller@canonical.com>
**    David Barth <david.barth@canonical.com>
**
** copyright (C) Canonical, oct. 2008
**
*******************************************************************************/

#include <assert.h>
#include "stack.h"
#include "bubble.h"
#include "apport.h"

G_DEFINE_TYPE (Stack, stack, G_TYPE_OBJECT);

/*-- internal API ------------------------------------------------------------*/

static void
stack_dispose (GObject* gobject)
{
	/* chain up to the parent class */
	G_OBJECT_CLASS (stack_parent_class)->dispose (gobject);
}

static
void
delete_entry (gpointer data,
	      gpointer user_data)
{
}


static void
stack_finalize (GObject* gobject)
{
	if (STACK(gobject)->list != NULL)
		g_list_foreach (STACK(gobject)->list, delete_entry, NULL);
	if (STACK(gobject)->defaults != NULL)
		g_object_unref (STACK(gobject)->defaults);
	if (STACK(gobject)->observer != NULL)
		g_object_unref (STACK(gobject)->observer);

	/* chain up to the parent class */
	G_OBJECT_CLASS (stack_parent_class)->finalize (gobject);
}

static void
stack_init (Stack* self)
{
	/* If you need specific construction properties to complete
	** initialization, delay initialization completion until the
	** property is set. */

	self->list = NULL;
}

static void
stack_get_property (GObject*    gobject,
		    guint       prop,
		    GValue*     value,
		    GParamSpec* spec)
{
	Stack* stack;

	stack = STACK (gobject);

	switch (prop)
	{
		default :
			G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop, spec);
		break;
	}
}

#include "stack-glue.h"

static void
stack_class_init (StackClass* klass)
{
	GObjectClass* gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->dispose      = stack_dispose;
	gobject_class->finalize     = stack_finalize;
	gobject_class->get_property = stack_get_property;

	dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (klass),
					 &dbus_glib_stack_object_info);
}

gint
compare_id (gconstpointer a,
	    gconstpointer b)
{
	gint  result;
	guint id_1;
	guint id_2;

	if (!a || !b)
		return -1;

	if (!IS_BUBBLE (a))
		return -1;

	id_1 = bubble_get_id ((Bubble*) a);
	id_2 = *((guint*) b);

	if (id_1 < id_2)
		result = -1;

	if (id_1 == id_2)
		result = 0;

	if (id_1 > id_2)
		result = 1;

	return result;
}

GList*
find_entry_by_id (Stack* self,
		  guint  id)
{
	GList* entry;

	/* sanity check */
	if (!self)
		return NULL;

	entry = g_list_find_custom (self->list,
				    (gconstpointer) &id,
				    compare_id);
	if (!entry)
		return NULL;

	return entry;
}

static Bubble*
find_bubble_by_id (Stack* self,
		   guint  id)
{
	GList* entry;

	/* sanity check */
	if (!self)
		return NULL;

	entry = g_list_find_custom (self->list,
				    (gconstpointer) &id,
				    compare_id);
	if (!entry)
		return NULL;

	return (Bubble*) entry->data;
}

static void
stack_purge_old_bubbles (Stack* self)
{
	Bubble* bubble = NULL;
	GList*    list = NULL;

	g_return_if_fail (self != NULL);

	for (list = g_list_first (self->list);
	     list != NULL;)
	{
		bubble = (Bubble*) list->data;
		
		if (! IS_BUBBLE (bubble))
		{
			self->list = g_list_delete_link (self->list, list);
			list = self->list;
		} else if (! bubble_is_visible (bubble) &&
			   (bubble_get_timeout (bubble) == 0))
		{
			self->list = g_list_delete_link (self->list, list);
			list = self->list;
			g_object_unref (bubble);
		} else {
			list = g_list_next (list);
		}
	}
}

/* fwd declaration */
void timed_out_handler (Bubble* bubble, Stack*  stack);

static Bubble *sync_bubble = NULL;

static void
stack_display_sync_bubble (Stack *self, Bubble *bubble)
{
	gint      y      = 0;
	gint      x      = 0;
	Defaults* d;

	g_return_if_fail (IS_STACK (self));
	g_return_if_fail (IS_BUBBLE (bubble));

	/* is the notification reusing the current bubble? */
	if (sync_bubble == bubble)
	{
		bubble_start_timer (bubble);
		bubble_refresh (bubble);
		return;
	}

	/* Position the bubble at the top left corner of the display. */
	d = self->defaults;
	y  =  defaults_get_desktop_top (d);
	y  -= EM2PIXELS (defaults_get_bubble_shadow_size (d), d);
	y  += EM2PIXELS (defaults_get_bubble_vert_gap (d), d);
	x  =  (gtk_widget_get_default_direction () == GTK_TEXT_DIR_LTR) ?
		(defaults_get_desktop_right (d) -
		 EM2PIXELS (defaults_get_bubble_shadow_size (d), d) -
		 EM2PIXELS (defaults_get_bubble_horz_gap (d), d) -
		 EM2PIXELS (defaults_get_bubble_width (d), d))
		:
		(defaults_get_desktop_left (d) -
		 EM2PIXELS (defaults_get_bubble_shadow_size (d), d) +
		 EM2PIXELS (defaults_get_bubble_horz_gap (d), d))
		;

	bubble_move (bubble, x, y);
	bubble_set_timeout (bubble, 2); /* Warning: in *seconds*! */

	bubble_fade_in (bubble, 100);

	sync_bubble = bubble;

	g_signal_connect (G_OBJECT (bubble),
			  "timed-out",
			  G_CALLBACK (timed_out_handler),
			  self);
}

static void
stack_resync_bubble_on_display (Stack *self)
{
	GList*    list   = NULL;
	Bubble*   bubble = NULL;

	g_return_if_fail (IS_STACK (self));

	/* find the bubble on display */
	for (list = g_list_first (self->list);
	     list != NULL;
	     list = g_list_next (list))
	{
		bubble = (Bubble*) list->data;

		if (bubble_is_visible (bubble)){
			bubble_start_timer (bubble);
			bubble_refresh (bubble);
			return;
		}
	}
}

static void
stack_layout (Stack* self)
{
	Bubble*   next_to_display = NULL;
	Bubble*   urgent_bubble   = NULL;
	GList*    list   = NULL;
	Bubble*   bubble = NULL;
	gint      y      = 0;
	gint      x      = 0;
	gboolean  reset  = 0;
	Defaults* d;

	g_return_if_fail (self != NULL);

	stack_purge_old_bubbles (self);

	/* pickup the next bubble to display */
	for (list = g_list_first (self->list);
	     list != NULL;
	     list = g_list_next (list))
	{
		bubble = (Bubble*) list->data;

		/* sync. bubbles have already been taken care of */
		if (bubble_is_synchronous (bubble))
		    continue;

		/* if there is already one bubble on display
		   we don't have room for another one */
		if (bubble_is_visible (bubble)){
			/* but if a synchronous notification is on
			   screen, synchronize both timers */
			if (reset) {
				bubble_start_timer (bubble);
				bubble_refresh (bubble);
			}
			return;
		}

		if (bubble_is_urgent (bubble))
		{
			/* pick-up the /first/ urgent bubble
			   in the queue (FIFO) */
			if (urgent_bubble == NULL) {
				urgent_bubble = bubble;
				next_to_display = bubble;
			}
		} else if (next_to_display == NULL) {
			next_to_display = bubble;
		}
	}

	if (next_to_display == NULL)
		/* this actually happens when we're called for a synchronous
		   bubble or after a bubble timed out, but there where no other
		   notifications waiting in the queue */
		return;

	/* Position the top left corner of the stack. */
	d = self->defaults;
	y  =  defaults_get_desktop_top (d);
	y  += 2 * EM2PIXELS (defaults_get_bubble_vert_gap (d), d);
	y  += EM2PIXELS (defaults_get_bubble_min_height (d), d);

	x  =  (gtk_widget_get_default_direction () == GTK_TEXT_DIR_LTR) ?
		(defaults_get_desktop_right (d) -
		 EM2PIXELS (defaults_get_bubble_shadow_size (d), d) -
		 EM2PIXELS (defaults_get_bubble_horz_gap (d), d) -
		 EM2PIXELS (defaults_get_bubble_width (d), d))
		:
		(defaults_get_desktop_left (d) -
		 EM2PIXELS (defaults_get_bubble_shadow_size (d), d) +
		 EM2PIXELS (defaults_get_bubble_horz_gap (d), d))
		;

	bubble_move (next_to_display, x, y);

	/* TODO: adjust timings for bubbles that appear in a serie of bubbles */
	if (urgent_bubble != NULL)
		bubble_fade_in (urgent_bubble, 100);
	else 
		bubble_fade_in (next_to_display, 200);
}


/*-- public API --------------------------------------------------------------*/

Stack*
stack_new (Defaults* defaults,
	   Observer* observer)
{
	Stack* this;

	if (!defaults || !observer)
		return NULL;

	this = g_object_new (STACK_TYPE, NULL);
	if (!this)
		return NULL;

	this->defaults = defaults;
	this->observer = observer;
	this->list     = NULL;
	this->next_id  = 1;

	return this;
}

void
stack_del (Stack* self)
{
	if (!self)
		return;

	g_object_unref (self);
}

void
timed_out_handler (Bubble* bubble,
		   Stack*  stack)
{
	if (bubble != NULL
	    && IS_BUBBLE (bubble)
	    && bubble_is_synchronous (bubble))
		g_object_unref (bubble);
	else 
		stack_layout (stack);

	/* TODO: use weak-refs to dispose the bubble.
	   Meanwhile, do nothing here to avoid segfaults
	   and rely on the stack_purge_old_bubbles() call
	   later on in the thread.
	*/

	return;
}

/* since notification-ids are unsigned integers the first id index is 1, 0 is
** used to indicate an error */
guint
stack_push_bubble (Stack*  self,
		   Bubble* bubble)
{
	guint notification_id = -1;

	/* sanity check */
	if (!self || !IS_BUBBLE (bubble))
		return -1;

	/* check if this is just an update */
	if (find_bubble_by_id (self, bubble_get_id (bubble)))		
	{
		bubble_start_timer (bubble);
		bubble_refresh (bubble);

		return bubble_get_id (bubble);
	}

	/* add bubble/id to stack */
	notification_id = self->next_id++;
	bubble_set_id (bubble, notification_id);
	self->list = g_list_append (self->list, (gpointer) bubble);

	g_signal_connect (G_OBJECT (bubble),
			  "timed-out",
			  G_CALLBACK (timed_out_handler),
			  self);

	/* return current/new id to caller (usually our DBus-dispatcher) */
	return notification_id;
}

void
stack_pop_bubble_by_id (Stack* self,
			guint  id)
{
	Bubble* bubble;

	/* sanity check */
	if (!self)
		return;

	/* find bubble corresponding to id */
	bubble = find_bubble_by_id (self, id);
	if (!bubble)
		return;

	/* close/hide/fade-out bubble */
	bubble_hide (bubble);

	/* find entry in list corresponding to id and remove it */
	self->list = g_list_delete_link (self->list,
					 find_entry_by_id (self, id));
	g_object_unref (bubble);

	/* immediately refresh the layout of the stack */
	stack_layout (self);
}

static GdkPixbuf*
process_dbus_icon_data (GValue *data)
{
	GType dbus_icon_t;
	GArray *pixels;
	int width, height, rowstride, bits_per_sample, n_channels, size;
	gboolean has_alpha;
	guchar *copy;
	GdkPixbuf *pixbuf = NULL;

	g_return_val_if_fail (data != NULL, NULL);

	dbus_icon_t = dbus_g_type_get_struct ("GValueArray",
					      G_TYPE_INT,
					      G_TYPE_INT,
					      G_TYPE_INT,
					      G_TYPE_BOOLEAN,
					      G_TYPE_INT,
					      G_TYPE_INT,
					      dbus_g_type_get_collection ("GArray",
									  G_TYPE_UCHAR),
					      G_TYPE_INVALID);
	
	if (G_VALUE_HOLDS (data, dbus_icon_t))
	{
		dbus_g_type_struct_get (data,
					0, &width,
					1, &height,
					2, &rowstride,
					3, &has_alpha,
					4, &bits_per_sample,
					5, &n_channels,
					6, &pixels,
					G_MAXUINT);

		size = (height - 1) * rowstride + width *
			((n_channels * bits_per_sample + 7) / 8);
		copy = (guchar *) g_memdup (pixels->data, size);
		pixbuf = gdk_pixbuf_new_from_data(copy, GDK_COLORSPACE_RGB,
						  has_alpha,
						  bits_per_sample,
						  width, height,
						  rowstride,
						  (GdkPixbufDestroyNotify)g_free,
						  NULL);
	}

	return pixbuf;
}

gboolean
stack_notify_handler (Stack*                 self,
		      const gchar*           app_name,
		      guint                  id,
		      const gchar*           icon,
		      const gchar*           summary,
		      const gchar*           body,
		      gchar**                actions,
		      GHashTable*            hints,
		      gint                   timeout,
		      DBusGMethodInvocation* context)
{
	Bubble*    bubble = NULL;
	GValue*      data = NULL;
	GdkPixbuf* pixbuf = NULL;

        /* check if a bubble exists with same id */
	bubble = find_bubble_by_id (self, id);
	if (bubble == NULL)
		bubble = bubble_new (self->defaults);
	
	if (hints)
	{
		data = (GValue*) g_hash_table_lookup (hints, "synchronous");
		if (G_VALUE_HOLDS_STRING (data))
		{
			if (sync_bubble != NULL
			    && IS_BUBBLE (sync_bubble))
			{
				g_object_unref (bubble);
				bubble = sync_bubble;
			}
			bubble_set_synchronous (bubble, g_value_get_string (data));
		}
	}

	if (hints)
	{
		data = (GValue*) g_hash_table_lookup (hints, "value");
		if (G_VALUE_HOLDS_INT (data))
			bubble_set_value (bubble, g_value_get_int (data));

	}

	if (summary)
		bubble_set_title (bubble, summary);
	if (body)
		bubble_set_message_body (bubble, body);

	if (hints)
	{
		data = (GValue*) g_hash_table_lookup (hints, "icon_data");
		if (*icon == '\0' && data != NULL)
		{
			pixbuf = process_dbus_icon_data (data);
			bubble_set_icon_from_pixbuf (bubble, pixbuf);
		} else
			bubble_set_icon (bubble, icon);
	}

	if (timeout || actions != NULL)
		apport_report (app_name, summary, actions, timeout);

	bubble_determine_layout (bubble);

	bubble_recalc_size (bubble);

	if (bubble_is_synchronous (bubble))
	{
		stack_display_sync_bubble (self, bubble);
		stack_resync_bubble_on_display (self);
	} else {
		stack_push_bubble (self, bubble);
		/* update the layout of the stack;
		 * this will also open the new bubble */
		stack_layout (self);
	}

	dbus_g_method_return (context, bubble_get_id (bubble));

	return TRUE;
}

gboolean
stack_close_notification_handler (Stack*   self,
				  guint    id,
				  GError** error)
{
	Bubble *bubble = find_bubble_by_id (self, id);
	g_return_val_if_fail (bubble != NULL, FALSE);

	bubble_hide (bubble);
	g_object_unref (bubble);

	stack_layout (self);

	return TRUE;
}

gboolean
stack_get_capabilities (Stack*   self,
			gchar*** out_caps)
{
	*out_caps = g_malloc0 (4 * sizeof(char *));

	(*out_caps)[0] = g_strdup ("body");
	(*out_caps)[1] = g_strdup ("icon-static");
	(*out_caps)[2] = g_strdup ("body-markup");
	(*out_caps)[3] = NULL;

	return TRUE;
}

gboolean
stack_get_server_information (Stack*  self,
			      gchar** out_name,
			      gchar** out_vendor,
			      gchar** out_version,
			      gchar** out_spec_ver)
{
	*out_name     = g_strdup ("alsdorf (Canonical's Notification Daemon");
	*out_vendor   = g_strdup ("Canonical Ltd");
	*out_version  = g_strdup ("0.1");
	*out_spec_ver = g_strdup ("1.0-with-twists");

	return TRUE;
}
