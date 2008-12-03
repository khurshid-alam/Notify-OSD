/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
**      10        20        30        40        50        60        70        80
**
** project:
**    alsdorf
**
** file:
**    test-bubble.c
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

	bubble = bubble_new ();
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
}


static
void
test_bubble_new (void)
{
	Bubble* bubble;

	bubble = bubble_new ();

	g_assert (bubble != NULL);

	/* I've no idea why unref'ing bubble here causes a SIGABRT */
	/*g_object_unref (bubble);*/
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
	g_test_suite_add(ts, tc);

	g_test_suite_add(ts,
			 g_test_create_case ("can slide a bubble to a new position",
					     0,
					     NULL,
					     NULL,
					     test_bubble_slide,
					     NULL)
		);

	return ts;
}
