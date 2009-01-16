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

static void
stack_finalize (GObject* gobject)
{
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
	self->feedback_bubble = NULL;
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

static
void
delete_entry (gpointer data,
	      gpointer user_data)
{
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

static gint
stack_get_height (Stack* self)
{
	GList*  list         = NULL;
	Bubble* bubble       = NULL;
	gint    stack_height = 0;

	/* sanity check */
	g_return_val_if_fail (self != NULL, 0);

	for (list = g_list_first (self->list);
	     list != NULL;
	     list = g_list_next (list))
	{
		bubble = (Bubble*) list->data;
		stack_height += defaults_get_bubble_gap (self->defaults) +
				bubble_get_height (bubble);
	}

	stack_height -= defaults_get_bubble_gap (self->defaults);

	return stack_height;
}

static void
stack_pop_last_bubble (Stack* self)
{
	GList* entry;

	/* sanity check */
	g_return_if_fail (self != NULL);

	/* find last (remember fifo) entry in stack-list */
	entry = g_list_first (self->list);
	if (!entry)
		return;

	/* close/hide/fade-out bubble */
	bubble_hide ((Bubble*) entry->data);
	bubble_del ((Bubble*) entry->data);

	/* find entry in list corresponding to id and remove it */
	self->list = g_list_delete_link (self->list, entry);
}

static void
stack_layout (Stack* self)
{
	GList*  list   = NULL;
	Bubble* bubble = NULL;
	gint    y      = 0;
	gint    x      = 0;

	/* sanity check */
	g_return_if_fail (self != NULL);

	/* position the top left corner of the stack  */
	y  =  defaults_get_desktop_top (self->defaults);
	y  += defaults_get_bubble_gap (self->defaults);
	x  =  (gtk_widget_get_default_direction () == GTK_TEXT_DIR_LTR) ?
		defaults_get_desktop_right (self->defaults) -
		defaults_get_bubble_gap (self->defaults) -
		defaults_get_bubble_width (self->defaults)
		:
		defaults_get_bubble_gap (self->defaults)
		;
 
	/* consider the special case of the feedback synchronous bubble */
	if (self->feedback_bubble)
	{
		y += bubble_get_height (self->feedback_bubble);
		y += defaults_get_bubble_gap (self->defaults);
	}

	/* 1. check if we need to expire bubbles early because the feedback
	**    bubble needs room */
	while (stack_get_height (self) > 
	       defaults_get_stack_height (self->defaults))
 	{
		stack_pop_last_bubble (self);
 	}

	/* 2. walk through the list of the current bubbles on the stack
	      and compute the new position for the bubbles
	      as long as there is room available on the stack
	 */
	for (list = g_list_last (self->list);
	     list != NULL;
	     list = g_list_previous (list))
	{
		bubble = (Bubble*) list->data;

		/* set/update the bubble attributes */
		bubble_set_size (bubble,
				 defaults_get_bubble_width (self->defaults),
				 defaults_get_bubble_height (self->defaults));

		if (y == defaults_get_desktop_top (self->defaults) +
			 defaults_get_bubble_gap (self->defaults))
		{
			bubble_move (bubble, x, y);
		}
		else
		{
			bubble_slide_to (bubble, x, y);
		}

		bubble_show (bubble);
		y += bubble_get_height (bubble)
		     + defaults_get_bubble_gap (self->defaults);
	}
	/* TODO: consider the case of old ids for refreshed bubbles; they
 	 * may be expired early, even if they still have a large lifespan
 	 * left */
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

	g_list_foreach (self->list, delete_entry, NULL);
	g_object_unref (self->defaults);
	g_object_unref (self);
}

void
timed_out_handler (Bubble* bubble,
		   Stack*  stack)
{
	stack_pop_bubble_by_id (stack, bubble_get_id (bubble));
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
		bubble_reset_timeout (bubble);
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
stack_show_feedback_bubble (Stack* self,
			    Bubble* bubble)
{
	/* sanity check */
	if (!self)
		return;

	if (self->feedback_bubble != NULL)
	{
		bubble_del(self->feedback_bubble);
	}

	/* add bubble/id to stack */
	self->feedback_bubble = bubble;

	/* recalculate layout of current stack, will open new bubble */
	stack_layout (self);
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
	bubble_del (bubble);

	/* find entry in list corresponding to id and remove it */
	self->list = g_list_delete_link (self->list,
					 find_entry_by_id (self, id));

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
					      dbus_g_type_get_collection ("GArray", G_TYPE_UCHAR),
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
	if (!bubble)
	{
		bubble = bubble_new ();
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

	if (*icon == '\0')
	{
		data = (GValue*) g_hash_table_lookup (hints, "icon_data");
		pixbuf = process_dbus_icon_data (data);
		bubble_set_icon_from_pixbuf (bubble, pixbuf);
	} else
		bubble_set_icon (bubble, icon);

	if (timeout || actions != NULL)
		apport_report (app_name, summary, actions, timeout);

	/* push the bubble in the stack */
	stack_push_bubble(self, bubble);

	/* update the layout of the stack;
	   this will also open the new bubble
	*/
	stack_layout (self);

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

	bubble_timed_out (bubble);

	return TRUE;
}

gboolean
stack_get_capabilities (Stack*   self,
			gchar*** out_caps)
{
	*out_caps      = g_malloc0 (3 * sizeof(char *));

	(*out_caps)[0] = g_strdup ("body");
	(*out_caps)[1] = g_strdup ("icon-static");
	(*out_caps)[2] = NULL;

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
