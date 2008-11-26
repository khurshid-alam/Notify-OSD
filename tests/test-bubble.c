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

void
test_bubble_new ()
{
	GtkWidget* bubble;

	bubble = bubble_new (20, 20, 200, 50);

	g_assert (bubble != NULL);

	g_object_unref (bubble);
}

