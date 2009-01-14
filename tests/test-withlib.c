/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
**      10        20        30        40        50        60        70        80
**
** project:
**    alsdorf
**
** file:
**    test-withlib.c
**
** author(s):
**    Mirco "MacSlow" Mueller <mirco.mueller@canonical.com>
**    David Barth <david.barth@canonical.com>
**
** copyright (C) Canonical, 2009
**
*******************************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <libnotify/notify.h>
#include "dbus.h"

/* #define DBUS_NAME "org.freedesktop.Notifications" */

static
gboolean
stop_main_loop (GMainLoop *loop)
{
	g_main_loop_quit (loop);

	return FALSE;
}

static void
test_withlib_get_server_information (void)
{
	gchar *name = NULL, *vendor = NULL, *version = NULL, *specver = NULL;
	gboolean ret = FALSE;

        notify_init (__FILE__);
	ret = notify_get_server_info (&name, &vendor, &version, &specver);
	
	g_assert (ret);
	g_assert (g_strrstr (name, "alsdorf"));
	g_assert (g_strrstr (specver, "1.0"));
}

static void
test_withlib_show_notification (void)
{
        NotifyNotification *n;

        notify_init (__FILE__);

	n = notify_notification_new ("Test",
				     "You should see a normal notification",
				     "", NULL);
	notify_notification_show (n, NULL);
	sleep (3);

	g_object_unref(G_OBJECT(n));
}

static void
test_withlib_update_notification (void)
{
        NotifyNotification *n;
	gboolean res = FALSE;

        notify_init (__FILE__);

	n = notify_notification_new ("Test",
				     "New notification",
				     "", NULL);
	res = notify_notification_show (n, NULL);
	g_assert (res);
	sleep (3);

	res = notify_notification_update (n, "Test (updated)",
				    "The message body has been updated...",
				    "");
	g_assert (res);

	res = notify_notification_show (n, NULL);
	g_assert (res);
	sleep (3);

	g_object_unref(G_OBJECT(n));
}

static void
test_withlib_pass_icon_data (void)
{
        NotifyNotification *n;
	GdkPixbuf* pixbuf;
        GMainLoop* loop;

	n = notify_notification_new ("Image Test",
				     "You should see an image",
				     "", NULL);
	pixbuf = gdk_pixbuf_new_from_file_at_scale ("../icons/avatar.png",
						    64, 64, TRUE, NULL);
	notify_notification_set_icon_from_pixbuf (n, pixbuf);
	notify_notification_show (n, NULL);
	
	loop = g_main_loop_new(NULL, FALSE);
        g_timeout_add (2000, (GSourceFunc) stop_main_loop, loop);
        g_main_loop_run (loop);

	g_object_unref(G_OBJECT(n));
}

GTestSuite *
test_withlib_create_test_suite (void)
{
	GTestSuite *ts = NULL;

	ts = g_test_create_suite ("libnotify");

	g_test_suite_add(ts,
			 g_test_create_case ("can get server info",
					     0,
					     NULL,
					     NULL,
					     test_withlib_get_server_information,
					     NULL)
		);
	g_test_suite_add(ts,
			 g_test_create_case ("can show normal notification",
					     0,
					     NULL,
					     NULL,
					     test_withlib_show_notification,
					     NULL)
		);
	g_test_suite_add(ts,
			 g_test_create_case ("can update notification",
					     0,
					     NULL,
					     NULL,
					     test_withlib_update_notification,
					     NULL)
		);
	g_test_suite_add(ts,
			 g_test_create_case ("can pass icon data on the wire",
					     0,
					     NULL,
					     NULL,
					     test_withlib_pass_icon_data,
					     NULL)
		);

	return ts;
}
