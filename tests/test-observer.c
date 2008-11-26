/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
**      10        20        30        40        50        60        70        80
**
** project:
**    alsdorf
**
** file:
**    test-observer.c
**
** author(s):
**    Mirco "MacSlow" Mueller <mirco.mueller@canonical.com>
**    David Barth <david.barth@canonical.com>
**
** copyright (C) Canonical, oct. 2008
**
*******************************************************************************/

#include <glib.h>

#include "observer.h"

void
test_observer_new ()
{
	Observer* observer = NULL;

	observer = observer_new ();
	g_assert (observer != NULL);
	observer_del (observer);
}

void
test_observer_del ()
{
	Observer* observer = NULL;

	observer = observer_new ();
	observer_del (observer);
	/*g_assert (observer == NULL);*/
}

void
test_observer_get_x ()
{
	Observer* observer = NULL;

	observer = observer_new ();
	g_assert_cmpint (observer_get_x (observer), <=, 4096);
	g_assert_cmpint (observer_get_x (observer), >=, 0);
	observer_del (observer);
}

void
test_observer_get_y ()
{
	Observer* observer = NULL;

	observer = observer_new ();
	g_assert_cmpint (observer_get_y (observer), <=, 4096);
	g_assert_cmpint (observer_get_y (observer), >=, 0);
	observer_del (observer);
}

