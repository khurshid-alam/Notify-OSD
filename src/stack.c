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

#include "stack.h"
#include "bubble.h"
#include "apport.h"

G_DEFINE_TYPE (Stack, stack, G_TYPE_OBJECT);

/* a convenience-structure for the handling of entries in the stack-list */
typedef struct _Entry
{
	guint   notification_id;
	Bubble* bubble;
} Entry;

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

GList*
find_entry_by_id (Stack* self,
		  guint  id)
{
	GList* entry = NULL;

	/* sanity check */
	if (!self)
		return NULL;

	return entry;
}

Bubble*
find_bubble_by_id (Stack* self,
		   guint  id)
{
	Bubble* bubble = NULL;

	/* sanity check */
	if (!self)
		return NULL;

	return bubble;
}


static
guint
bubble_get_height (Bubble *bubble)
{
	return 30;
}

void
layout (Stack* self)
{
	GList*  list = NULL;
	guint height = 0;
	guint      y = 0;
	guint   hreq = 0;

	y  = defaults_get_desktop_top (self->defaults);
	y += defaults_get_bubble_gap (self->defaults);

	/* sanity check */
	if (!self)
		return;


	/* consider the special case of the feedback synchronous bubble */
	if (self->feedback_bubble)
	{
		y += bubble_get_height (self->feedback_bubble);
		y += defaults_get_bubble_gap (self->defaults);
	}

	/* 1. check if we need to expire bubbles early because the feedback bubble needs room */
	for (list = self->list; list != NULL; list = g_list_next (self->list))
		hreq += defaults_get_bubble_gap (self->defaults) +
			bubble_get_height (((Entry *)(list->data))->bubble);

	if (hreq > defaults_get_desktop_height (self->defaults))
	{
		/* FIXME */
	}

	/* 2. walk through the list of the current bubbles on the stack
	      by order of arrival, ie by id,
	      !!!!! except when the ids loop! *** NEED A SPECIFIC TEST HERE ***
	      and compute the new position for the bubbles
	      as long as there is room available on the stack
	 */
	for (list = self->list; list != NULL; list = g_list_next (self->list))
	{
		height += defaults_get_bubble_gap (self->defaults);
	}

	/* 3. now that we have computed the future position for each bubble
	      ask every bubble to slide to its new position
	 */	
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

/* since notification-ids are unsigned integers the first id index is 1, 0 is
** used to indicate an error */
guint
stack_push_bubble (Stack* self,
		   Bubble* bubble)
{
	Entry*  entry           = NULL;
	guint notification_id   = -1;

	/* sanity check */
	if (!self)
		return -1;

	notification_id = self->next_id++;

	/* add bubble/id to stack */
	bubble_set_id (bubble, notification_id);

	entry = (Entry*) g_malloc0 (sizeof (Entry));
	entry->notification_id = notification_id;
	entry->bubble = bubble;
	self->list = g_list_append (self->list, (gpointer) entry);

	/* recalculate layout of current stack, will open new bubble */
	layout (self);
	bubble_move (bubble,
		     defaults_get_desktop_right (self->defaults) -
		     defaults_get_bubble_gap (self->defaults) -
		     defaults_get_bubble_width (self->defaults),
		     defaults_get_desktop_top (self->defaults) +
		     defaults_get_bubble_gap (self->defaults));

	bubble_show (bubble); // temp.; easier to do here

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
	layout (self);
	bubble_show (bubble); // temp. easier to do here now
}



/* dbarth: turned static, because it is the stack's business to manage its list of bubbles
 */
#if 0 // FIXME FIXME FIXME (-Wall)
static 
void
stack_pop_bubble_by_id (Stack* self,
			guint  id)
{
	Bubble* bubble;
	GList*  entry;

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
	entry = find_entry_by_id (self, id);
	self->list = g_list_delete_link (self->list, entry);

	/* recalculate layout of current stack */
	layout (self);
}
#endif

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
	Bubble* bubble = NULL;
	GValue* data   = NULL;

	/* check if a bubble exists with same id */
	bubble = find_bubble_by_id (self, id);
	if (!bubble)
	{
		bubble = bubble_new ();
		bubble_set_id (bubble, self->next_id++);
	}

	/* set/update the bubble attributes */
	bubble_set_size (bubble,
			 defaults_get_bubble_width (self->defaults),
			 defaults_get_bubble_height (self->defaults));

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
	if (icon)
		bubble_set_icon (bubble, icon);

	if (timeout || actions != NULL)
		apport_report (app_name, summary, actions, timeout);

	/* push the bubble and try to display it */
	stack_push_bubble(self, bubble);

	dbus_g_method_return (context, bubble_get_id (bubble));

	return TRUE;
}

gboolean
stack_close_notification_handler (Stack*   self,
				  guint    id,
				  GError** error)
{
	g_print ("stack_close_notification_handler() called, %s\n", G_STRLOC);

	return TRUE;
}

gboolean
stack_get_capabilities (Stack*   self,
			gchar*** out_caps)
{
	g_print ("stack_get_capabilities() called, %s\n", G_STRLOC);

	return TRUE;
}

gboolean
stack_get_server_information (Stack*  self,
			      gchar** out_name,
			      gchar** out_vendor,
			      gchar** out_version,
			      gchar** out_spec_ver)
{
	g_print ("stack_get_server_information() called, %s\n", G_STRLOC);

	return TRUE;
}
