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
#include <gdk/gdkkeysyms.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include "defaults.h"
#include "stack.h"
#include "observer.h"
#include "stack-display.h"

#define DBUS_PATH "/org/freedesktop/Notifications"

int
main (int    argc,
      char** argv)
{
	Defaults*        defaults   = NULL;
	Stack*           stack      = NULL;
	Observer*        observer   = NULL;
	DBusGConnection* connection = NULL;
	DBusGProxy*      proxy      = NULL;
	guint            request_name_result;
	GError*          error      = NULL;

	gtk_init (&argc, &argv);

	dbus_g_thread_init ();

	connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
	if (error)
	{
		g_print ("%s", error->message);
		g_error_free (error);
	}

	proxy = dbus_g_proxy_new_for_name (connection,
					   "org.freedesktop.DBus",
					   "/org/freedesktop/Dbus",
					   "org.freedesktop.DBus");
	if (!dbus_g_proxy_call (proxy,
				"RequestName",
				&error,
				G_TYPE_STRING, "org.freedesktop.Notifications",
				G_TYPE_UINT, 0,
				G_TYPE_INVALID,
				G_TYPE_UINT, &request_name_result,
				G_TYPE_INVALID))
	{
		g_error ("Could not aquire name: %s", error->message);
	}

	defaults = defaults_new ();
	observer = observer_new ();
	stack = stack_new (defaults, observer);
	dbus_g_connection_register_g_object (connection,
					     DBUS_PATH,
					     G_OBJECT (stack));

	gtk_main ();

	return 0;
}
