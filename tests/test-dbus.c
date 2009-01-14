/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
**      10        20        30        40        50        60        70        80
**
** project:
**    alsdorf
**
** file:
**    test-dbus.c
**
** author(s):
**    Mirco "MacSlow" Mueller <mirco.mueller@canonical.com>
**    David Barth <david.barth@canonical.com>
**
** copyright (C) Canonical, oct. 2008
**
*******************************************************************************/

#include <stdlib.h>
#include "dbus.h"
#include "stack.h"

#define DBUS_NAME "org.freedesktop.Notificationstest"

static
void
test_dbus_instance (void)
{
	DBusGConnection* connection = NULL;

	connection = dbus_create_service_instance (DBUS_NAME);
	g_assert (connection != NULL);
}

static
void
test_dbus_collision (void)
{
	DBusGConnection* connection = NULL;

	/* HACK: as we did not destroy the instance after the first test above,
	   this second creation should fail */

	if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDOUT |
                               G_TEST_TRAP_SILENCE_STDERR))
        {
		connection = dbus_create_service_instance (DBUS_NAME);
		exit (0); /* should never be triggered */
        }
	g_test_trap_assert_failed();
}


static void
test_dbus_get_capabilities (void)
{
	char **caps = NULL;
	gboolean ret = FALSE;

	ret = stack_get_capabilities (NULL, &caps);

	g_assert (ret);
	g_assert (!g_strcmp0 (caps[0], "body"));

	int i = 0;
	while (caps[i] != NULL)
	{
		g_assert (!g_strrstr (caps[i++], "actions"));
	}
}

static void
test_dbus_get_server_information (void)
{
	gchar *name = NULL, *vendor = NULL, *version = NULL, *specver = NULL;
	gboolean ret = FALSE;

	ret = stack_get_server_information (NULL, &name, &vendor,
					    &version, &specver);
	g_assert (ret);
	g_assert (g_strrstr (name, "alsdorf"));
	g_assert (g_strrstr (specver, "1.0"));
}

GTestSuite *
test_dbus_create_test_suite (void)
{
	GTestSuite *ts = NULL;

	ts = g_test_create_suite ("dbus");

	g_test_suite_add(ts,
			 g_test_create_case ("can create an instance on the bus",
					     0,
					     NULL,
					     NULL,
					     test_dbus_instance,
					     NULL)
		);

	g_test_suite_add(ts,
			 g_test_create_case ("refuse to have multiple instance on the bus",
					     0,
					     NULL,
					     NULL,
					     test_dbus_collision,
					     NULL)
		);

	g_test_suite_add(ts,
			 g_test_create_case ("can get server capabilities",
					     0,
					     NULL,
					     NULL,
					     test_dbus_get_capabilities,
					     NULL)
		);

	g_test_suite_add(ts,
			 g_test_create_case ("can get server info",
					     0,
					     NULL,
					     NULL,
					     test_dbus_get_server_information,
					     NULL)
		);
	return ts;
}
