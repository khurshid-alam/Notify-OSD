/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
**      10        20        30        40        50        60        70        80
**
** project:
**    alsdorf
**
** file:
**    dbus.c
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
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

DBusGConnection*
dbus_create_service_instance (char *service_name)
{
	DBusGConnection* connection = NULL;
	DBusGProxy*      proxy      = NULL;
	guint            request_name_result;
	GError*          error      = NULL;

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
				G_TYPE_STRING, service_name,
				G_TYPE_UINT, 0,
				G_TYPE_INVALID,
				G_TYPE_UINT, &request_name_result,
				G_TYPE_INVALID))
	{
		g_warning ("Could not acquire name: %s", error->message);
		return NULL;
	}

	if (request_name_result != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
	{
		g_warning ("Another instance has already registered %s", service_name);
		return NULL;
	}

	return connection;
}
