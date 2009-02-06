/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
**      10        20        30        40        50        60        70        80
**
** project:
**    alsdorf
**
** file:
**    test-synchronous.c
**
** author(s):
**    Mirco "MacSlow" Mueller <mirco.mueller@canonical.com>
**    David Barth <david.barth@canonical.com>
**
** copyright (C) Canonical, oct. 2008
**
*******************************************************************************/

#include <unistd.h>
#include <glib.h>
#include <libnotify/notify.h>
#include "dbus.h"


static void
send_normal (const gchar *message)
{
        NotifyNotification *n;
	n = notify_notification_new ("Test notification",
				     g_strdup (message),
				     "", NULL);
	notify_notification_show (n, NULL);
	g_object_unref(G_OBJECT(n));
}

static void
send_synchronous (gint value)
{
        static NotifyNotification *n = NULL;

	if (n == NULL)
		n = notify_notification_new (" ",
					     "",
					     "/home/dbarth/devel/alsdorf/icons/volume.svg",
					     NULL);
	else
		notify_notification_update (n,
					    " ",
					    "",
					    "/home/dbarth/devel/alsdorf/icons/volume.svg");
		
	notify_notification_set_hint_int32(n, "value", value);
	notify_notification_set_hint_string(n, "synchronous", "volume");

	notify_notification_show (n, NULL);
	// g_object_unref(G_OBJECT(n));
}

static void
test_synchronous_layout (void)
{

        notify_init (__FILE__);

	send_normal ("Hey, what about this restaurant? http://www.blafasel.org"
		     ""
		     "Would you go from your place by train or should I pick you up from work? What do you think?"
		);
	sleep (1);
	send_synchronous (75);
	sleep (1);
	send_normal ("Hey, what about this restaurant? http://www.blafasel.org"
		     ""
		     "Would you go from your place by train or should I pick you up from work? What do you think?"
		     "etc. etc. etc. etc. etc. etc etc. etc. etc. etc. etc. etc. etc. etc. etc. etc. etc. etc."
		);
	sleep (1);
	send_synchronous (75);
	sleep (1);
	send_synchronous (80);
	sleep (1);
	send_synchronous (85);
}

GTestSuite *
test_synchronous_create_test_suite (void)
{
	GTestSuite *ts = NULL;

	ts = g_test_create_suite ("synchronous");

	g_test_suite_add(ts,
			 g_test_create_case ("synchronous layout",
					     0,
					     NULL,
					     NULL,
					     test_synchronous_layout,
					     NULL)
		);

	return ts;
}
