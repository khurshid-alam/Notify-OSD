/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
**      10        20        30        40        50        60        70        80
**
** project:
**    alsdorf
**
** file:
**    observer.c
**
** author(s):
**    Mirco "MacSlow" Mueller <mirco.mueller@canonical.com>
**    David Barth <david.barth@canonical.com>
**
** copyright (C) Canonical, oct. 2008
**
*******************************************************************************/

#include <gtk/gtk.h>

#include "observer.h"

G_DEFINE_TYPE (Observer, observer, G_TYPE_OBJECT);

enum
{
	PROP_DUMMY = 0,
	PROP_X,
	PROP_Y
};

/*-- internal API ------------------------------------------------------------*/

static void
observer_dispose (GObject* gobject)
{
	/* chain up to the parent class */
	G_OBJECT_CLASS (observer_parent_class)->dispose (gobject);
}

static void
observer_finalize (GObject* gobject)
{
	/* chain up to the parent class */
	G_OBJECT_CLASS (observer_parent_class)->finalize (gobject);
}

static void
observer_init (Observer* self)
{
	/* here we assume gtk_init() has been issued before by the caller */
	self->window            = NULL;
	self->timeout_frequency = 0;
	self->timeout_id        = 0;
	self->pointer_x         = 0;
	self->pointer_y         = 0;

	/* If you need specific construction properties to complete
	** initialization, delay initialization completion until the
	** property is set. */
}

static void
observer_get_property (GObject*    gobject,
		       guint       prop,
		       GValue*     value,
		       GParamSpec* spec)
{
	Observer* observer;

	observer = OBSERVER (gobject);

	switch (prop)
	{
		case PROP_X:
			g_value_set_int (value, observer->pointer_x);
		break;

		case PROP_Y:
			g_value_set_int (value, observer->pointer_y);
		break;

		default :
			G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop, spec);
		break;
	}
}

static void
observer_class_init (ObserverClass* klass)
{
	GObjectClass* gobject_class = G_OBJECT_CLASS (klass);
	GParamSpec*   property_x;
	GParamSpec*   property_y;

	gobject_class->dispose      = observer_dispose;
	gobject_class->finalize     = observer_finalize;
	gobject_class->get_property = observer_get_property;

	property_x = g_param_spec_int (
				"pointer-x",
				"pointer-x",
				"X-coord. of mouse pointer",
				0,
				4096,
				0,
				G_PARAM_CONSTRUCT |
				G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
					 PROP_X,
					 property_x);

	property_y = g_param_spec_int (
				"pointer-y",
				"pointer-y",
				"Y-coord. of mouse pointer",
				0,
				4096,
				0,
				G_PARAM_CONSTRUCT |
				G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
					 PROP_Y,
					 property_y);
}

/*-- public API --------------------------------------------------------------*/

Observer*
observer_new (void)
{
	Observer* this = g_object_new (OBSERVER_TYPE, NULL);

	return this;
}

void
observer_del (Observer* self)
{
	g_object_unref (self);
}

gint
observer_get_x (Observer* self)
{
	gint x;

	g_object_get (self, "pointer-x", &x, NULL);

	return x;
}

gint
observer_get_y (Observer* self)
{
	gint y;

	g_object_get (self, "pointer-y", &y, NULL);

	return y;
}
