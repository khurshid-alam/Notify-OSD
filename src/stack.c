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

static void
stack_class_init (StackClass* klass)
{
	GObjectClass* gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->dispose      = stack_dispose;
	gobject_class->finalize     = stack_finalize;
	gobject_class->get_property = stack_get_property;
}

void
delete_entry (gpointer data,
	      gpointer user_data)
{
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

void
layout (Stack* self)
{
	/* sanity check */
	if (!self)
		return;
}

/*-- public API --------------------------------------------------------------*/

Stack*
stack_new (Defaults* defaults)
{
	Stack* this;

	if (!defaults)
		return NULL;

	this = g_object_new (STACK_TYPE, NULL);
	if (!this)
		return NULL;

	this->defaults = defaults;
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
stack_push (Stack* self,
	    guint  id,
	    gchar* title,
	    gchar* body,
	    gchar* icon)
{
	guint   notification_id = id;
	Bubble* bubble          = NULL;
	Entry*  entry           = NULL;

	/* sanity check */
	if (!self)
		return 0;

	/* check if there is an existing bubble */
	if (notification_id != 0)
	{
		bubble = find_bubble_by_id (self, notification_id);
		bubble_set_title (bubble, title);
		bubble_set_message_body (bubble, body);
		bubble_set_icon (bubble, icon);
		bubble_reset_timeout (bubble);
	}
	else
	{
		/* grab id for new bubble */
		notification_id = self->next_id++;

		/* create and setup bubble */
		bubble = bubble_new ();
		bubble_set_title (bubble, title);
		bubble_set_message_body (bubble, body);
		bubble_set_icon (bubble, icon);
		bubble_set_id (bubble, notification_id);

		/* add bubble/id to stack */
		entry = (Entry*) g_malloc0 (sizeof (Entry));
		entry->notification_id = notification_id;
		entry->bubble = bubble;
		self->list = g_list_append (self->list, (gpointer) entry);
	}

	/* recalculate layout of current stack, will open new bubble */
	layout (self);

	/* return current/new id to caller (usually our DBus-dispatcher) */
	return notification_id;
}

void
stack_pop (Stack* self,
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
