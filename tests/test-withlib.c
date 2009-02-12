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
test_withlib_get_server_caps (void)
{
	GList *cap, *caps = NULL;
	gboolean test = FALSE;

        notify_init (__FILE__);

	caps = notify_get_server_caps ();

	g_assert (caps);

	for (cap = g_list_first (caps);
	     cap != NULL;
	     cap = g_list_next (cap))
	{
		if (! g_strcmp0 (cap->data, "canonical-private-1"))
			test = TRUE;
	}

	g_assert (test);
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
	sleep (1);

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

static void
test_withlib_priority (void)
{
        NotifyNotification *n1, *n2, *n3;
        GMainLoop* loop;

	n1 = notify_notification_new ("Normal Notification",
				      "This is the 2nd notification you should see",
				      "", NULL);
	notify_notification_show (n1, NULL);
	n3 = notify_notification_new ("Synchronous Notification",
				      "You should always see this notification",
				      "", NULL);
	notify_notification_show (n3, NULL);
	n2 = notify_notification_new ("Urgent Notification",
				      "This is the 1st notification you should see",
				      "", NULL);
	notify_notification_show (n2, NULL);
	
	loop = g_main_loop_new(NULL, FALSE);
        g_timeout_add (8000, (GSourceFunc) stop_main_loop, loop);
        g_main_loop_run (loop);

	g_object_unref(G_OBJECT(n1));
	g_object_unref(G_OBJECT(n2));
	g_object_unref(G_OBJECT(n3));
}

static void
test_withlib_close_notification (void)
{
        NotifyNotification *n;
        GMainLoop* loop;
	gboolean res = FALSE;

	n = notify_notification_new ("Test Title",
				     "This notification will be closed prematurely...",
				     "", NULL);
	notify_notification_show (n, NULL);
	
	loop = g_main_loop_new(NULL, FALSE);
        g_timeout_add (2000, (GSourceFunc) stop_main_loop, loop);
        g_main_loop_run (loop);

	res = notify_notification_close (n, NULL);
	g_assert (res);

        g_timeout_add (2000, (GSourceFunc) stop_main_loop, loop);
        g_main_loop_run (loop);

	g_object_unref(G_OBJECT(n));
}

static void
test_withlib_append_hint (void)
{
        NotifyNotification *n;
	gboolean res = FALSE;

        notify_init (__FILE__);

	/* init notification, supply first line of body-text */
	n = notify_notification_new ("Test (append-hint)",
				     "The quick brown fox jumps over the lazy dog.",
				     "./icons/avatar.png",
				     NULL);
	res = notify_notification_show (n, NULL);
	g_assert (res);
	sleep (2);

	/* append second part of body-text */
	res = notify_notification_update (n,
					  " ",
					  "Polyfon zwitschernd aßen Mäxchens Vögel Rüben, Joghurt und Quark. (first append)",
					  NULL);
	notify_notification_set_hint_string (n, "append", "allowed");
	g_assert (res);
	res = notify_notification_show (n, NULL);
	g_assert (res);
 	sleep (2);

	/* append third part of body-text */
	res = notify_notification_update (n,
					  " ",
					  "Съешь ещё этих мягких французских булок, да выпей чаю. (last append)",
					  NULL);
	notify_notification_set_hint_string (n, "append", "allowed");
	g_assert (res);
	res = notify_notification_show (n, NULL);
	g_assert (res);
 	sleep (2);

	g_object_unref(G_OBJECT(n));
}

static void
test_withlib_swallow_markup (void)
{
        NotifyNotification *n;
	gboolean res = FALSE;

        notify_init (__FILE__);

	n = notify_notification_new ("Swallow markup test",
				     "This text is hopefully neither <b>bold</b>, <i>italic</i> nor <u>underlined</u>.\n\nA little math-notation:\n\n\ta &gt; b &lt; c = 0",
				     "./icons/avatar.png",
				     NULL);
	res = notify_notification_show (n, NULL);
	g_assert (res);
	sleep (2);

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
			 g_test_create_case ("can get private server cap",
					     0,
					     NULL,
					     NULL,
					     test_withlib_get_server_caps,
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
	g_test_suite_add(ts,
			 g_test_create_case ("can close a notification",
					     0,
					     NULL,
					     NULL,
					     test_withlib_close_notification,
					     NULL)
		);
	g_test_suite_add(ts,
			 g_test_create_case ("honors priority level",
					     0,
					     NULL,
					     NULL,
					     test_withlib_priority,
					     NULL)
		);
	g_test_suite_add(ts,
			 g_test_create_case ("supports append-hint",
					     0,
					     NULL,
					     NULL,
					     test_withlib_append_hint,
					     NULL)
		);
	g_test_suite_add(ts,
			 g_test_create_case ("swallows markup",
					     0,
					     NULL,
					     NULL,
					     test_withlib_swallow_markup,
					     NULL)
		);

	return ts;
}
