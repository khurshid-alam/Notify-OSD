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

#include "defaults.h"
#include "bubble.h"

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
	Defaults* defaults;
	GList*    list;
	guint     next_id;
};

/* class structure */
struct _StackClass
{
	GObjectClass parent;
};

GType stack_get_type (void);

Stack*
stack_new (Defaults* defaults);

void
stack_del (Stack* self);

guint
stack_push (Stack* self,
	    guint  id,
	    gchar* title,
	    gchar* body,
	    gchar* icon);

void
stack_pop (Stack* self,
	   guint  id);

G_END_DECLS

#endif /* __STACK_H */
