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
// Notes:
//    to see timings working in a more obvious way start it with DEBUG=1 set in
//    the environment
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

#define MAX_TIME_LIMIT    3000
#define INITIAL_DURATION  1000
#define EXTENSION         1000

gboolean
_stop_main_loop (GMainLoop *loop)
{
	g_main_loop_quit (loop);

	return FALSE;
}

gboolean
_trigger_pause (gpointer data)
{
	Timings* t = NULL;

	g_assert (data);
	t = TIMINGS (data);

	timings_pause (t);

	return FALSE;
}

gboolean
_trigger_continue (gpointer data)
{
	Timings* t = NULL;

	g_assert (data);
	t = TIMINGS (data);

	timings_continue (t);

	return FALSE;
}

static void
test_timings_new (void)
{
	Timings* t = NULL;

	// create new object
	t = timings_new (INITIAL_DURATION, MAX_TIME_LIMIT);

	// test validity of main notification object
	g_assert (t != NULL);

	// clean up
	timings_destroy (t);
	t = NULL;
}

static void
test_timings_destroy (void)
{
	Timings* t = NULL;

	// create new object
	t = timings_new (INITIAL_DURATION, MAX_TIME_LIMIT);

	// test validity of main notification object
	g_assert (t != NULL);

	// clean up
	timings_destroy (t);
	t = NULL;
}

static void
test_timings_extend (void)
{
	Timings*   t    = NULL;
	GMainLoop* loop = NULL;

	// create new object
	t = timings_new (INITIAL_DURATION, MAX_TIME_LIMIT);

	// test validity of main notification object
	g_assert (t != NULL);

	// verify we're fool-proof
	g_assert (!timings_extend (t, 0));

	// try to extend by 1 second
	g_assert (timings_extend (t, EXTENSION));

	// let the main loop run
	loop = g_main_loop_new (NULL, FALSE);
	g_timeout_add (INITIAL_DURATION + 2 * EXTENSION,
		       (GSourceFunc) _stop_main_loop,
		       loop);
	g_main_loop_run (loop);

	// clean up
	timings_destroy (t);
	t = NULL;
}

static void
test_timings_pause (void)
{
	Timings*   t    = NULL;
	GMainLoop* loop = NULL;

	// create new object
	t = timings_new (INITIAL_DURATION, MAX_TIME_LIMIT);

	// test validity of main notification object
	g_assert (t != NULL);

	// trigger pause after .5 second
	g_timeout_add (500,
		       (GSourceFunc) _trigger_pause,
		       (gpointer) t);

	// let the main loop run
	loop = g_main_loop_new (NULL, FALSE);
	g_timeout_add (INITIAL_DURATION + EXTENSION,
		       (GSourceFunc) _stop_main_loop,
		       loop);
	g_main_loop_run (loop);

	// clean up
	timings_destroy (t);
	t = NULL;
}

static void
test_timings_continue (void)
{
	Timings*   t    = NULL;
	GMainLoop* loop = NULL;

	// create new object
	t = timings_new (INITIAL_DURATION, MAX_TIME_LIMIT);

	// test validity of main notification object
	g_assert (t != NULL);

	// trigger pause after .5 second
	g_timeout_add (500,
		       (GSourceFunc) _trigger_pause,
		       (gpointer) t);

	// trigger continue after .75 seconds
	g_timeout_add (750,
		       (GSourceFunc) _trigger_continue,
		       (gpointer) t);

	// let the main loop run
	loop = g_main_loop_new (NULL, FALSE);
	g_timeout_add (INITIAL_DURATION + EXTENSION,
		       (GSourceFunc) _stop_main_loop,
		       loop);
	g_main_loop_run (loop);

	// clean up
	timings_destroy (t);
	t = NULL;
}

static void
test_timings_intercept_pause (void)
{
	Timings*   t    = NULL;
	GMainLoop* loop = NULL;

	// create new object
	t = timings_new (INITIAL_DURATION, MAX_TIME_LIMIT);

	// test validity of main notification object
	g_assert (t != NULL);

	// trigger pause after .5 second
	g_timeout_add (500,
		       (GSourceFunc) _trigger_pause,
		       (gpointer) t);

	// trigger 2nd pause after .75 seconds
	g_timeout_add (750,
		       (GSourceFunc) _trigger_pause,
		       (gpointer) t);

	// let the main loop run
	loop = g_main_loop_new (NULL, FALSE);
	g_timeout_add (INITIAL_DURATION + EXTENSION,
		       (GSourceFunc) _stop_main_loop,
		       loop);
	g_main_loop_run (loop);

	// clean up
	timings_destroy (t);
	t = NULL;
}

static void
test_timings_intercept_continue (void)
{
	Timings*   t    = NULL;
	GMainLoop* loop = NULL;

	// create new object
	t = timings_new (INITIAL_DURATION, MAX_TIME_LIMIT);

	// test validity of main notification object
	g_assert (t != NULL);

	// trigger 1st continue after .5 second
	g_timeout_add (500,
		       (GSourceFunc) _trigger_continue,
		       (gpointer) t);

	// trigger 2nd continue after .75 seconds
	g_timeout_add (750,
		       (GSourceFunc) _trigger_continue,
		       (gpointer) t);

	// let the main loop run
	loop = g_main_loop_new (NULL, FALSE);
	g_timeout_add (INITIAL_DURATION + EXTENSION,
		       (GSourceFunc) _stop_main_loop,
		       loop);
	g_main_loop_run (loop);

	// clean up
	timings_destroy (t);
	t = NULL;
}

static void
test_timings_normal_callback (void)
{
	Timings*   t    = NULL;
	GMainLoop* loop = NULL;

	// create new object
	t = timings_new (INITIAL_DURATION, MAX_TIME_LIMIT);

	// let the main loop run
	loop = g_main_loop_new (NULL, FALSE);
	g_timeout_add (MAX_TIME_LIMIT,
		       (GSourceFunc) _stop_main_loop,
		       loop);
	g_main_loop_run (loop);

	// clean up
	timings_destroy (t);
	t = NULL;
}

static void
test_timings_force_callback (void)
{
	Timings*   t    = NULL;
	GMainLoop* loop = NULL;

	// create new object
	t = timings_new (INITIAL_DURATION, MAX_TIME_LIMIT);

	// trigger pause after .5 seconds
	g_timeout_add (500,
		       (GSourceFunc) _trigger_pause,
		       (gpointer) t);

	// let the main loop run
	loop = g_main_loop_new (NULL, FALSE);
	g_timeout_add (MAX_TIME_LIMIT + EXTENSION,
		       (GSourceFunc) _stop_main_loop,
		       loop);
	g_main_loop_run (loop);

	// clean up
	timings_destroy (t);
	t = NULL;
}

GTestSuite*
test_timings_create_test_suite (void)
{
	GTestSuite* ts = NULL;

	ts = g_test_create_suite ("timings");
	g_test_suite_add (ts,
			  g_test_create_case ("can create",
					      0,
					      NULL,
					      NULL,
					      test_timings_new,
					      NULL));

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

	g_test_suite_add (ts,
			  g_test_create_case (
			  	"let normal-callback kick in",
				0,
				NULL,
				NULL,
				test_timings_normal_callback,
				NULL));

	g_test_suite_add (ts,
			  g_test_create_case (
			  	"let force-callback kick in",
				0,
				NULL,
				NULL,
				test_timings_force_callback,
				NULL));

	return ts;
}

