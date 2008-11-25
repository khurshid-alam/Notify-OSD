/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
**      10        20        30        40        50        60        70        80
**
** project:
**    alsdorf
**
** file:
**    observer.h
**
** author(s):
**    Mirco "MacSlow" Mueller <mirco.mueller@canonical.com>
**    David Barth <david.barth@canonical.com>
**
** copyright (C) Canonical, oct. 2008
**
*******************************************************************************/

#ifndef __OBSERVER_H
#define __OBSERVER_H

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define OBSERVER_TYPE             (observer_get_type ())
#define OBSERVER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), OBSERVER_TYPE, Observer))
#define OBSERVER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), OBSERVER_TYPE, ObserverClass))
#define IS_OBSERVER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OBSERVER_TYPE))
#define IS_OBSERVER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), OBSERVER_TYPE))
#define OBSERVER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), OBSERVER_TYPE, ObserverClass))

typedef struct _Observer      Observer;
typedef struct _ObserverClass ObserverClass;

/* instance structure */
struct _Observer
{
	GObject parent;

	/* private */
	GtkWidget* window;
	gint       timeout_frequency;
	guint      timeout_id;
	gint       pointer_x;
	gint       pointer_y;
};

/* class structure */
struct _ObserverClass
{
	GObjectClass parent;
};

GType Observer_get_type (void);

Observer*
observer_new (void);

void
observer_del (Observer* self);

gint
observer_get_x (Observer* self);

gint
observer_get_y (Observer* self);

G_END_DECLS

#endif /* __OBSERVER_H */
