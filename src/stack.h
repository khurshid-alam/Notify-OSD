/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
**      10        20        30        40        50        60        70        80
**
** project:
**    alsdorf
**
** file:
**    stack.h
**
** author(s):
**    Mirco "MacSlow" Mueller <mirco.mueller@canonical.com>
**    David Barth <david.barth@canonical.com>
**
** copyright (C) Canonical, oct. 2008
**
*******************************************************************************/

#ifndef __STACK_H
#define __STACK_H

#include <glib-object.h>

G_BEGIN_DECLS

#define STACK_TYPE             (stack_get_type ())
#define STACK(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), STACK_TYPE, Stack))
#define STACK_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), STACK_TYPE, StackClass))
#define IS_STACK(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), STACK_TYPE))
#define IS_STACK_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), STACK_TYPE))
#define STACK_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), STACK_TYPE, StackClass))

typedef struct _Stack      Stack;
typedef struct _StackClass StackClass;

/* instance structure */
struct _Stack
{
	GObject parent;

	/* private */
	GList* synchronous;
	GList* asynchronous;
	guint  next_id;
	GList* timers;
};

/* class structure */
struct _StackClass
{
	GObjectClass parent;
};

GType stack_get_type (void);

Stack*
stack_new (void);

void
stack_del (Stack* self);

guint
stack_get_next_id (Stack* self);

void
stack_add_async (Stack* self,
		 guint  id);

void
stack_remove_async (Stack* self,
		    guint  id);

void
stack_add_sync (Stack* self,
		guint  id);

void
stack_remove_sync (Stack* self,
		   guint  id);

G_END_DECLS

#endif /* __STACK_H */
