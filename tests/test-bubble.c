/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
**      10        20        30        40        50        60        70        80
**
** Codename "alsdorf"
**
** test-bubble.c - implements unit-tests for rendering/displaying a bubble
**
** Copyright 2009 Canonical Ltd.
**
** Authors:
**    Mirco "MacSlow" Mueller <mirco.mueller@canonical.com>
**    David Barth <david.barth@canonical.com>
**
** This program is free software: you can redistribute it and/or modify it
** under the terms of the GNU General Public License version 3, as published
** by the Free Software Foundation.
**
** This program is distributed in the hope that it will be useful, but
** WITHOUT ANY WARRANTY; without even the implied warranties of
** MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
** PURPOSE.  See the GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License along
** with this program.  If not, see <http://www.gnu.org/licenses/>.
**
*******************************************************************************/

#include <unistd.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "bubble.h"


static
gboolean
stop_main_loop (GMainLoop *loop)
{
	g_main_loop_quit (loop);

	return FALSE;
}

static
void
test_bubble_slide (void)
{
        GMainLoop *loop;
	Bubble* bubble;
	gint x         = 0;
	gint y         = 0;
	Defaults* defaults;

	defaults = defaults_new ();
	bubble = bubble_new (defaults);
	g_assert (bubble != NULL);

	bubble_move (bubble, 0, 0);
	bubble_slide_to (bubble, 100, 200);

	/* let the main loop run to have the slide being performed */
        loop = g_main_loop_new(NULL, FALSE);
        g_timeout_add(2000, (GSourceFunc)stop_main_loop, loop);
        g_main_loop_run(loop);

	/* check position */
	bubble_get_position (bubble, &x, &y);

	g_assert_cmpint (x, ==, 100);

	g_object_unref (bubble);
	g_object_unref (defaults);
}


static
void
test_bubble_new (void)
{
	Bubble* bubble;
	Defaults* defaults;

	defaults = defaults_new ();
	bubble = bubble_new (defaults);

	g_assert (bubble != NULL);

	g_object_unref (bubble);
	g_object_unref (defaults);
}

static
void
test_bubble_del (void)
{
	Bubble* bubble;
	Defaults* defaults;

	defaults = defaults_new ();
	bubble = bubble_new (defaults);

	g_assert (bubble != NULL);

	g_object_unref (bubble);
	g_object_unref (defaults);
}

static
void
test_bubble_set_attributes (void)
{
	Bubble*    bubble;
        GMainLoop* loop;
	Defaults* defaults;

	defaults = defaults_new ();
	bubble = bubble_new (defaults);
	bubble_set_icon (bubble, "../icons/chat.svg");
	bubble_set_title (bubble, "Unit Testing");
	bubble_set_message_body (bubble, "Long text that is hopefully wrapped");
	bubble_set_size (bubble, 300, 100);
	bubble_move (bubble, 30, 30);
	bubble_show (bubble);

	/* let the main loop run to have the slide being performed */
        loop = g_main_loop_new (NULL, FALSE);
        g_timeout_add (2000, (GSourceFunc) stop_main_loop, loop);
        g_main_loop_run (loop);

	g_object_unref (bubble);
	g_object_unref (defaults);
}




GTestSuite *
test_bubble_create_test_suite (void)
{
	GTestSuite *ts = NULL;
	GTestCase  *tc = NULL;

	ts = g_test_create_suite ("bubble");
	tc = g_test_create_case ("can create a new bubble",
				 0,
				 NULL,
				 NULL,
				 test_bubble_new,
				 NULL);
	g_test_suite_add (ts, tc);

	g_test_suite_add(ts,
			 g_test_create_case ("can delete a bubble",
					     0,
					     NULL,
					     NULL,
					     test_bubble_del,
					     NULL)
		);

	g_test_suite_add(ts,
			 g_test_create_case ("can slide a bubble to a new position",
					     0,
					     NULL,
					     NULL,
					     test_bubble_slide,
					     NULL)
		);

	g_test_suite_add (ts,
			  g_test_create_case ("can set bubble attributes",
					      0,
					      NULL,
					      NULL,
					      test_bubble_set_attributes,
					      NULL));

	return ts;
}
