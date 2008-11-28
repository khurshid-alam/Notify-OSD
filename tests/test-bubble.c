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

#include <glib.h>
#include <gtk/gtk.h>

#include "bubble.h"

Bubble*
bubble_new (void);

void
bubble_set_title (Bubble* self,
		  char *title);

void
bubble_set_message_body (Bubble* self,
			 char *body);

void
bubble_set_size(Bubble* self,
	      gint width,
	      gint height);

void
bubble_move (Bubble* self,
	     gint x,
	     gint y);

void
bubble_display (Bubble* self);

void
bubble_hide (Bubble* self);

void
bubble_del (Bubble* self);

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
	tc = g_test_create_case ("bubble_new",
				 0,
				 NULL,
				 NULL,
				 test_bubble_new,
				 NULL);
	g_test_suite_add(ts, tc);

	return ts;
}
