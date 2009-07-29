////////////////////////////////////////////////////////////////////////////////
//3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
//      10        20        30        40        50        60        70        80
//
// notify-osd
//
// test-timings.c - implements unit-tests for exercising API of timings object
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

#include <unistd.h>

#include "timings.h"

#define MAX_TIME_LIMIT   15000
#define INITIAL_DURATION  1000
#define EXTENSION         1000

gboolean
_close_cb (gpointer data)
{
	g_print ("coming from _close_cb()\n");

	return FALSE;
}

gboolean
_force_close_cb (gpointer data)
{
	g_print ("coming from _force_close_cb()\n");

	return FALSE;
}

static void
test_timings_new (void)
{
	timings_t* t = NULL;

	// create new object
	t = timings_new (INITIAL_DURATION,
			 MAX_TIME_LIMIT,
			 _close_cb,
			 _force_close_cb);

	// wait a bit
	sleep (INITIAL_DURATION / 1000 + 1);

	// test validity of main notification object
	g_assert (t != NULL);

	// clean up
	timings_destroy (t);
	t = NULL;
}

static void
test_timings_destroy (void)
{
	timings_t* t = NULL;

	// create new object
	t = timings_new (INITIAL_DURATION,
			 MAX_TIME_LIMIT,
			 _close_cb,
			 _force_close_cb);

	// test validity of main notification object
	g_assert (t != NULL);

	// wait a bit
	sleep (INITIAL_DURATION / 1000 + 1);

	// clean up
	timings_destroy (t);
	t = NULL;
}

static void
test_timings_extend (void)
{
	timings_t* t = NULL;

	// create new object
	t = timings_new (INITIAL_DURATION,
			 MAX_TIME_LIMIT,
			 _close_cb,
			 _force_close_cb);

	// test validity of main notification object
	g_assert (t != NULL);

	// try to extend by 3 seconds
	timings_extend_by_ms (t, EXTENSION);

	// wait a bit
	sleep ((INITIAL_DURATION + EXTENSION)/ 1000 + 1);

	// clean up
	timings_destroy (t);
	t = NULL;

	// trying to extend an invalid timing object should not crash
	timings_extend_by_ms (t, EXTENSION);
}

static void
test_timings_pause (void)
{
	timings_t* t = NULL;

	// create new object
	t = timings_new (INITIAL_DURATION,
			 MAX_TIME_LIMIT,
			 _close_cb,
			 _force_close_cb);

	// test validity of main notification object
	g_assert (t != NULL);

	// try to pause for 2 seconds
	timings_pause (t);
	sleep (2);
	timings_continue (t);

	// wait a bit
	sleep (INITIAL_DURATION / 1000 + 1);

	// clean up
	timings_destroy (t);
	t = NULL;

	// trying to pause an invalid timing object should not crash
	timings_pause (t);
}

static void
test_timings_continue (void)
{
	timings_t* t = NULL;

	// create new object
	t = timings_new (INITIAL_DURATION,
			 MAX_TIME_LIMIT,
			 _close_cb,
			 _force_close_cb);

	// test validity of main notification object
	g_assert (t != NULL);

	// try to pause for 2 seconds
	timings_pause (t);
	sleep (2);
	timings_continue (t);

	// wait a bit
	sleep (INITIAL_DURATION / 1000 + 1);

	// clean up
	timings_destroy (t);
	t = NULL;

	// trying to continue an invalid timing object should not crash
	timings_continue (t);
}

static void
test_timings_intercept_pause (void)
{
	timings_t* t = NULL;

	// create new object
	t = timings_new (INITIAL_DURATION,
			 MAX_TIME_LIMIT,
			 _close_cb,
			 _force_close_cb);

	// test validity of main notification object
	g_assert (t != NULL);

	// try to pause twice should result in message on stdout
	timings_pause (t);
	sleep (2);
	timings_pause (t);
	timings_continue (t);

	// wait a bit
	sleep (INITIAL_DURATION / 1000 + 1);

	// clean up
	timings_destroy (t);
	t = NULL;
}

static void
test_timings_intercept_continue (void)
{
	timings_t* t = NULL;

	// create new object
	t = timings_new (INITIAL_DURATION,
			 MAX_TIME_LIMIT,
			 _close_cb,
			 _force_close_cb);

	// test validity of main notification object
	g_assert (t != NULL);

	// try to continue twice should result in message on stdout
	timings_pause (t);
	sleep (2);
	timings_continue (t);
	timings_continue (t);

	// wait a bit
	sleep (INITIAL_DURATION / 1000 + 1);

	// clean up
	timings_destroy (t);
	t = NULL;
}

GTestSuite *
test_timings_create_test_suite (void)
{
	GTestSuite *ts = NULL;
	GTestCase  *tc = NULL;

	ts = g_test_create_suite ("timings");
	tc = g_test_create_case ("can create",
				 0,
				 NULL,
				 NULL,
				 test_timings_new,
				 NULL);
	g_test_suite_add (ts, tc);

	g_test_suite_add (ts,
			  g_test_create_case ("can destroy",
					      0,
					      NULL,
					      NULL,
					      test_timings_destroy,
					      NULL));

	g_test_suite_add (ts,
			  g_test_create_case ("can extend",
					      0,
					      NULL,
					      NULL,
					      test_timings_extend,
					      NULL));

	g_test_suite_add (ts,
			  g_test_create_case ("can pause",
					      0,
					      NULL,
					      NULL,
					      test_timings_pause,
					      NULL));

	g_test_suite_add (ts,
			  g_test_create_case ("can continue",
					      0,
					      NULL,
					      NULL,
					      test_timings_continue,
					      NULL));

	g_test_suite_add (ts,
			  g_test_create_case ("can intercept pause if paused",
					      0,
					      NULL,
					      NULL,
					      test_timings_intercept_pause,
					      NULL));

	g_test_suite_add (ts,
			  g_test_create_case (
			  	"can intercept continue if running",
				0,
				NULL,
				NULL,
				test_timings_intercept_continue,
				NULL));

	return ts;
}

