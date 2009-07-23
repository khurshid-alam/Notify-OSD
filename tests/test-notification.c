////////////////////////////////////////////////////////////////////////////////
//3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
//      10        20        30        40        50        60        70        80
//
// notify-osd
//
// test-notification.c - implements unit-tests for exercising API of abstract
//                       notification object
//
// Copyright 2009 Canonical Ltd.
//
// Authors:
//    Mirco "MacSlow" Mueller <mirco.mueller@canonical.com>
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License version 3, as published
// by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranties of
// MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
// PURPOSE.  See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////////////

#include "notification.h"

static void
test_notification_new (void)
{
	notification_t* n = NULL;

	// create new object
	n = notification_new ();

	// test validity of main notification object
	g_assert (n != NULL);

	// test validity of initialized non-pointer and zeroed values
	g_assert_cmpint (notification_get_id (n), ==, 0);
	g_assert_cmpint (notification_get_value (n), ==, 0);
	g_assert_cmpint (notification_get_onscreen_time (n), ==, 0);
	g_assert_cmpint (notification_get_sender_pid (n), ==, 0);

	// clean up
	notification_destroy (n);
}

static void
test_notification_destroy (void)
{
	notification_t* n = NULL;

	// create new object
	n = notification_new ();

	// test validity of main notification object
	g_assert (n != NULL);

	// clean up
	notification_destroy (n);

	// verify destruction notification object
	g_assert_cmpint (notification_get_id (n), ==, -1);
}

static void
test_notification_setget_id (void)
{
	notification_t* n = NULL;

	// create new object
	n = notification_new ();

	// if nothing has been set yet it should return 0
	g_assert_cmpint (notification_get_id (n), ==, 0);

	// a negative id should not be stored
	notification_set_id (n, -1);
	g_assert_cmpint (notification_get_id (n), >=, 0);

	// smallest possible id is 0
	notification_set_id (n, 0);
	g_assert_cmpint (notification_get_id (n), ==, 0);

	// largest possible id is G_MAXINT
	notification_set_id (n, G_MAXINT);
	g_assert_cmpint (notification_get_id (n), ==, G_MAXINT);

	// clean up
	notification_destroy (n);

	// after destruction setting an id should not crash and it should
	// yield -1
	notification_set_id (n, 42);
	g_assert_cmpint (notification_get_id (n), ==, -1);
}

static void
test_notification_setget_title (void)
{
	notification_t* n = NULL;

	// create new object
	n = notification_new ();

	// if no title has been set yet it should return NULL
	g_assert (notification_get_title (n) == NULL);

	// set an initial title-text and verify it
	notification_set_title (n, "Some title text");
	g_assert_cmpstr (notification_get_title (n), ==, "Some title text");

	// set a new title-text and verify it
	notification_set_title (n, "The new summary");
	g_assert_cmpstr (notification_get_title (n), !=, "Some title text");
	g_assert_cmpstr (notification_get_title (n), ==, "The new summary");

	// clean up
	notification_destroy (n);

	// after destruction setting a title should not crash and it should
	// yield NULL
	notification_set_title (n, "Unsettable title");
	g_assert (notification_get_title (n) == NULL);
}

static void
test_notification_setget_body (void)
{
	notification_t* n = NULL;

	// create new object
	n = notification_new ();

	// if no body has been set yet it should return NULL
	g_assert (notification_get_body (n) == NULL);

	// set an initial body-text and verify it
	notification_set_body (n, "Example body text");
	g_assert_cmpstr (notification_get_body (n), ==, "Example body text");

	// set a new body-text and verify it
	notification_set_body (n, "Some new body text");
	g_assert_cmpstr (notification_get_body (n), !=, "Example body text");
	g_assert_cmpstr (notification_get_body (n), ==, "Some new body text");

	// clean up
	notification_destroy (n);

	// after destruction setting a body should not crash and it should
	// yield NULL
	notification_set_body (n, "You'll not get this body-text");
	g_assert (notification_get_body (n) == NULL);
}

static void
test_notification_setget_value (void)
{
	notification_t* n = NULL;

	// create new object
	n = notification_new ();

	// if no value has been set yet it should return 0
	g_assert_cmpint (notification_get_value (n), ==, 0);

	// set an initial value and verify it
	notification_set_value (n, 25);
	g_assert_cmpint (notification_get_value (n), ==, 25);

	// set a new value and verify it
	notification_set_value (n, 45);
	g_assert_cmpint (notification_get_value (n), !=, 25);
	g_assert_cmpint (notification_get_value (n), ==, 45);

	// test allowed range
	notification_set_value (n, NOTIFICATION_VALUE_MAX_ALLOWED + 1);
	g_assert_cmpint (notification_get_value (n),
			 ==,
			 NOTIFICATION_VALUE_MAX_ALLOWED);
	notification_set_value (n, NOTIFICATION_VALUE_MIN_ALLOWED - 1);
	g_assert_cmpint (notification_get_value (n),
			 ==,
			 NOTIFICATION_VALUE_MIN_ALLOWED);

	// clean up
	notification_destroy (n);

	// after destruction setting a value should not crash and it should
	// yield -2
	notification_set_value (n, 50);
	g_assert_cmpint (notification_get_value (n), ==, -2);
}

static void
test_notification_setget_icon_themename (void)
{
	notification_t* n = NULL;

	// create new object
	n = notification_new ();

	// if no icon-themename has been set yet it should return NULL
	g_assert (notification_get_icon_themename (n) == NULL);

	// set an initial icon-themename and verify it
	notification_set_icon_themename (n, "notification-message-im");
	g_assert_cmpstr (notification_get_icon_themename (n),
			 ==,
			 "notification-message-im");

	// set a new icon-themename and verify it
	notification_set_icon_themename (n, "notification-device-usb");
	g_assert_cmpstr (notification_get_icon_themename (n),
			 !=,
			 "notification-message-im");
	g_assert_cmpstr (notification_get_icon_themename (n),
			 ==,
			 "notification-device-usb");

	// clean up
	notification_destroy (n);

	// after destruction setting an icon-themename should not crash and it
	// should yield NULL
	notification_set_icon_themename (n, "notification-printer");
	g_assert (notification_get_icon_themename (n) == NULL);
}

static void
test_notification_setget_icon_filename (void)
{
	notification_t* n = NULL;

	// create new object
	n = notification_new ();

	// if no icon-filename has been set yet it should return NULL
	g_assert (notification_get_icon_filename (n) == NULL);

	// set an initial icon-filename and verify it
	notification_set_icon_filename (n, "/usr/share/icon/photo.png");
	g_assert_cmpstr (notification_get_icon_filename (n),
			 ==,
			 "/usr/share/icon/photo.png");

	// set a new icon-filename and verify it
	notification_set_icon_filename (n, "/tmp/drawing.svg");
	g_assert_cmpstr (notification_get_icon_filename (n),
			 !=,
			 "/usr/share/icon/photo.png");
	g_assert_cmpstr (notification_get_icon_filename (n),
			 ==,
			 "/tmp/drawing.svg");

	// passing an invalid/NULL pointer should not change the stored
	// icon-filename 
	notification_set_icon_filename (n, NULL);
	g_assert_cmpstr (notification_get_icon_filename (n),
			 ==,
			 "/tmp/drawing.svg");

	// pass an empty (but not NULL) strings
	notification_set_icon_filename (n, "");
	g_assert_cmpstr (notification_get_icon_filename (n),
			 ==,
			 "");
	notification_set_icon_filename (n, "\0");
	g_assert_cmpstr (notification_get_icon_filename (n),
			 ==,
			 "\0");

	// clean up
	notification_destroy (n);

	// after destruction setting an icon-filename should not crash and it
	// should yield NULL
	notification_set_icon_filename (n, "");
	g_assert (notification_get_icon_filename (n) == NULL);
}

static void
test_notification_setget_icon_pixbuf (void)
{
	notification_t* n      = NULL;
	GdkPixbuf*      pixbuf = NULL;

	// create new object
	n = notification_new ();

	// if no icon-pixbuf has been set yet it should return NULL
	g_assert (notification_get_icon_pixbuf (n) == NULL);

	// create pixbuf for testing
	pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, 100, 100);

	// set an initial icon-pixbuf and verify it
	notification_set_icon_pixbuf (n, pixbuf);
	g_assert (notification_get_icon_pixbuf (n) != NULL);

	// passing an invalid/NULL pointer should not change the stored
	// icon-pixbuf
	notification_set_icon_pixbuf (n, NULL);
	g_assert (notification_get_icon_pixbuf (n) != NULL);

	// clean up
	notification_destroy (n);

	// after destruction setting an icon-pixbuf should not crash and it
	// should yield NULL
	notification_set_icon_pixbuf (n, pixbuf);
	g_assert (notification_get_icon_pixbuf (n) == NULL);

	// more clean up
	g_object_unref (pixbuf);
}

static void
test_notification_setget_onscreen_time (void)
{
	notification_t* n = NULL;

	// create new object
	n = notification_new ();

	// if no onscreen-time has been set yet it should return 0
	g_assert_cmpint (notification_get_onscreen_time (n), ==, 0);

	// setting a negative onscreen-time should fail
	notification_set_onscreen_time (n, -1);
	g_assert_cmpint (notification_get_onscreen_time (n), ==, 0);

	// set an positive onscreen-time and verify it
	notification_set_onscreen_time (n, 1000);
	g_assert_cmpint (notification_get_onscreen_time (n), ==, 1000);

	// set a new onscreen-time and verify it
	notification_set_onscreen_time (n, 5000);
	g_assert_cmpint (notification_get_onscreen_time (n), !=, 1000);
	g_assert_cmpint (notification_get_onscreen_time (n), ==, 5000);

	// setting a new onscreen-time smaller than the currently stored one
	// should fail
	notification_set_onscreen_time (n, 4000);
	g_assert_cmpint (notification_get_onscreen_time (n), ==, 5000);

	// clean up
	notification_destroy (n);

	// after destruction setting a value should not crash and it should
	// yield -1
	notification_set_onscreen_time (n, 500);
	g_assert_cmpint (notification_get_onscreen_time (n), ==, -1);
}

static void
test_notification_setget_sender_name (void)
{
	notification_t* n = NULL;

	// create new object
	n = notification_new ();

	// if no sender-name has been set yet it should return NULL
	g_assert (notification_get_sender_name (n) == NULL);

	// set an initial sender-name and verify it
	notification_set_sender_name (n, "evolution");
	g_assert_cmpstr (notification_get_sender_name (n), ==, "evolution");

	// set a new sender-name and verify it
	notification_set_sender_name (n, "pidgin");
	g_assert_cmpstr (notification_get_sender_name (n), !=, "evolution");
	g_assert_cmpstr (notification_get_sender_name (n), ==, "pidgin");

	// passing an invalid/NULL pointer should not change the stored
	// sender-name 
	notification_set_sender_name (n, NULL);
	g_assert_cmpstr (notification_get_sender_name (n), ==, "pidgin");

	// pass an empty (but not NULL) strings
	notification_set_sender_name (n, "");
	g_assert_cmpstr (notification_get_sender_name (n), ==, "");
	notification_set_sender_name (n, "\0");
	g_assert_cmpstr (notification_get_sender_name (n), ==, "\0");

	// clean up
	notification_destroy (n);

	// after destruction setting an icon-filename should not crash and it
	// should yield NULL
	notification_set_sender_name (n, "banshee");
	g_assert (notification_get_sender_name (n) == NULL);
}

static void
test_notification_setget_sender_pid (void)
{
	notification_t* n = NULL;

	// create new object
	n = notification_new ();

	// if nothing has been set yet it should return 0
	g_assert_cmpint (notification_get_sender_pid (n), ==, 0);

	// a negative pid makes no sense and should therefore not be stored
	notification_set_sender_pid (n, -1);
	g_assert_cmpint (notification_get_sender_pid (n), ==, 0);

	// smallest possible pid is 1
	notification_set_sender_pid (n, 1);
	g_assert_cmpint (notification_get_sender_pid (n), ==, 1);

	// largest possible id is G_MAXINT
	notification_set_sender_pid (n, G_MAXINT);
	g_assert_cmpint (notification_get_sender_pid (n), ==, G_MAXINT);

	// a pid of 0 would mean something before the init process is sending us
	// a notification, leave the stored pid untouched
	notification_set_sender_pid (n, 0);
	g_assert_cmpint (notification_get_sender_pid (n), ==, G_MAXINT);

	// clean up
	notification_destroy (n);

	// after destruction setting a pid should not crash and it should
	// yield -1
	notification_set_sender_pid (n, 42);
	g_assert_cmpint (notification_get_sender_pid (n), ==, -1);
}

static void
test_notification_setget_reception_timestamp (void)
{
	notification_t* n     = NULL;
	GTimeVal*       tvptr = NULL;
	GTimeVal        tv_old;
	GTimeVal        tv_new;

	// create new object
	n = notification_new ();

	// if no reception-time has been set yet it should return 0/0
	tvptr = notification_get_reception_timestamp (n);
	g_assert_cmpint (tvptr->tv_sec, ==, 0);
	g_assert_cmpint (tvptr->tv_usec, ==, 0);

	// ehm... well, get current time
	g_get_current_time (&tv_old);

	// store current time as reception-time and verify it
	notification_set_reception_timestamp (n, &tv_old);
	tvptr = notification_get_reception_timestamp (n);
	g_assert_cmpint (tvptr->tv_sec, ==, tv_old.tv_sec);
	g_assert_cmpint (tvptr->tv_usec, ==, tv_old.tv_usec);

	// get current time
	g_get_current_time (&tv_new);

	// trying to store an older timestamp over a newer one should fail
	notification_set_reception_timestamp (n, &tv_new);
	notification_set_reception_timestamp (n, &tv_old);
	tvptr = notification_get_reception_timestamp (n);
	g_assert_cmpint (tvptr->tv_sec, !=, tv_old.tv_sec);
	g_assert_cmpint (tvptr->tv_usec, !=, tv_old.tv_usec);
	g_assert_cmpint (tvptr->tv_sec, ==, tv_new.tv_sec);
	g_assert_cmpint (tvptr->tv_usec, ==, tv_new.tv_usec);

	// clean up
	notification_destroy (n);

	// after destruction setting a reception-time should not crash and it
	// should yield NULL
	notification_set_reception_timestamp (n, &tv_new);
	g_assert (notification_get_reception_timestamp (n) == NULL);
}

GTestSuite *
test_notification_create_test_suite (void)
{
	GTestSuite *ts = NULL;
	GTestCase  *tc = NULL;

	ts = g_test_create_suite ("notification");
	tc = g_test_create_case ("can create",
				 0,
				 NULL,
				 NULL,
				 test_notification_new,
				 NULL);
	g_test_suite_add (ts, tc);

	g_test_suite_add (
		ts,
		g_test_create_case (
			"can destroy",
			0,
			NULL,
			NULL,
			test_notification_destroy,
			NULL));

	g_test_suite_add (
		ts,
		g_test_create_case (
			"can set|get id",
			0,
			NULL,
			NULL,
			test_notification_setget_id,
			NULL));

	g_test_suite_add (
		ts,
		g_test_create_case (
			"can set|get title",
			0,
			NULL,
			NULL,
			test_notification_setget_title,
			NULL));

	g_test_suite_add (
		ts,
		g_test_create_case (
			"can set|get body",
			0,
			NULL,
			NULL,
			test_notification_setget_body,
			NULL));

	g_test_suite_add (
		ts,
		g_test_create_case (
			"can set|get value",
			0,
			NULL,
			NULL,
			test_notification_setget_value,
			NULL));

	g_test_suite_add (
		ts,
		g_test_create_case (
			"can set|get icon-themename",
			0,
			NULL,
			NULL,
			test_notification_setget_icon_themename,
			NULL));

	g_test_suite_add (
		ts,
		g_test_create_case (
			"can set|get icon-filename",
			0,
			NULL,
			NULL,
			test_notification_setget_icon_filename,
			NULL));

	g_test_suite_add (
		ts,
		g_test_create_case (
			"can set|get icon-pixbuf",
			0,
			NULL,
			NULL,
			test_notification_setget_icon_pixbuf,
			NULL));

	g_test_suite_add (
		ts,
		g_test_create_case (
			"can set|get onscreen-time",
			0,
			NULL,
			NULL,
			test_notification_setget_onscreen_time,
			NULL));

	g_test_suite_add (
		ts,
		g_test_create_case (
			"can set|get sender-name",
			0,
			NULL,
			NULL,
			test_notification_setget_sender_name,
			NULL));

	g_test_suite_add (
		ts,
		g_test_create_case (
			"can set|get sender-pid",
			0,
			NULL,
			NULL,
			test_notification_setget_sender_pid,
			NULL));

	g_test_suite_add (
		ts,
		g_test_create_case (
			"can set|get reception-timestamp",
			0,
			NULL,
			NULL,
			test_notification_setget_reception_timestamp,
			NULL));

	return ts;
}

