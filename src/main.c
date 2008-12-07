/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
**      10        20        30        40        50        60        70        80
**
** project:
**    alsdorf
**
** file:
**    main.c
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
#include <gtk/gtk.h>

#include "defaults.h"
#include "stack.h"
#include "observer.h"
#include "dbus.h"

#define DBUS_PATH "/org/freedesktop/Notifications"
#define DBUS_NAME "org.freedesktop.Notifications"


int
main (int    argc,
      char** argv)
{
	Defaults*        defaults   = NULL;
	Stack*           stack      = NULL;
	Observer*        observer   = NULL;
	DBusGConnection* connection = NULL;

	g_thread_init (NULL);
	dbus_g_thread_init ();

	gtk_init (&argc, &argv);

	defaults = defaults_new ();
	observer = observer_new ();
	stack = stack_new (defaults, observer);

	connection = dbus_create_service_instance (DBUS_NAME);
	if (connection == NULL)
		g_error ("Could not register instance");

	dbus_g_connection_register_g_object (connection,
					     DBUS_PATH,
					     G_OBJECT (stack));

	gtk_main ();

	return 0;
}
