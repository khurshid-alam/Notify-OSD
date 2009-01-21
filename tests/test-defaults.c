/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
**      10        20        30        40        50        60        70        80
**
** project:
**    alsdorf
**
** file:
**    test-defaults.c
**
** author(s):
**    Mirco "MacSlow" Mueller <mirco.mueller@canonical.com>
**    David Barth <david.barth@canonical.com>
**
** copyright (C) Canonical, oct. 2008
**
*******************************************************************************/

#include <glib.h>

#include "defaults.h"

static
void
test_defaults_new ()
{
	Defaults* defaults = NULL;

	defaults = defaults_new ();
	g_assert (defaults != NULL);
	defaults_del (defaults);
}

static
void
test_defaults_del ()
{
	Defaults* defaults = NULL;

	defaults = defaults_new ();
	defaults_del (defaults);
	/*g_assert (defaults == NULL);*/
}

static
void
test_defaults_get_desktop_width ()
{
	Defaults* defaults = NULL;

	defaults = defaults_new ();
	g_assert_cmpint (defaults_get_desktop_width (defaults), <=, 4096);
	g_assert_cmpint (defaults_get_desktop_width (defaults), >=, 640);
	defaults_del (defaults);
}

static
void
test_defaults_get_desktop_height ()
{
	Defaults* defaults = NULL;

	defaults = defaults_new ();
	g_assert_cmpint (defaults_get_desktop_height (defaults), <=, 4096);
	g_assert_cmpint (defaults_get_desktop_height (defaults), >=, 600);
	defaults_del (defaults);
}

static
void
test_defaults_get_desktop_top ()
{
	Defaults* defaults = NULL;

	defaults = defaults_new ();
	g_assert_cmpint (defaults_get_desktop_top (defaults), <=, 4096);
	g_assert_cmpint (defaults_get_desktop_top (defaults), >=, 0);
	defaults_del (defaults);
}

static
void
test_defaults_get_desktop_bottom ()
{
	Defaults* defaults = NULL;

	defaults = defaults_new ();
	g_assert_cmpint (defaults_get_desktop_bottom (defaults), <=, 4096);
	g_assert_cmpint (defaults_get_desktop_bottom (defaults), >=, 0);
	defaults_del (defaults);
}

static
void
test_defaults_get_desktop_left ()
{
	Defaults* defaults = NULL;

	defaults = defaults_new ();
	g_assert_cmpint (defaults_get_desktop_left (defaults), <=, 4096);
	g_assert_cmpint (defaults_get_desktop_left (defaults), >=, 0);
	defaults_del (defaults);
}

static
void
test_defaults_get_desktop_right ()
{
	Defaults* defaults = NULL;

	defaults = defaults_new ();
	g_assert_cmpint (defaults_get_desktop_right (defaults), <=, 4096);
	g_assert_cmpint (defaults_get_desktop_right (defaults), >=, 0);
	defaults_del (defaults);
}

static
void
test_defaults_get_stack_height ()
{
	Defaults* defaults = NULL;

	defaults = defaults_new ();
	g_assert_cmpint (defaults_get_stack_height (defaults), <=, 4096);
	g_assert_cmpint (defaults_get_stack_height (defaults), >=, 0);
	defaults_del (defaults);
}

static
void
test_defaults_get_bubble_width ()
{
	Defaults* defaults = NULL;

	defaults = defaults_new ();
	g_assert_cmpint (defaults_get_bubble_width (defaults), <=, 1024);
	g_assert_cmpint (defaults_get_bubble_width (defaults), >=, 200);
	defaults_del (defaults);
}

GTestSuite *
test_defaults_create_test_suite (void)
{
	GTestSuite *ts = NULL;

	ts = g_test_create_suite (__FILE__);

#define TC(x) g_test_create_case(#x, 0, NULL, NULL, x, NULL)

	g_test_suite_add(ts, TC(test_defaults_new));
	g_test_suite_add(ts, TC(test_defaults_del));
	g_test_suite_add(ts, TC(test_defaults_get_desktop_width));
	g_test_suite_add(ts, TC(test_defaults_get_desktop_height));
	g_test_suite_add(ts, TC(test_defaults_get_desktop_top));
	g_test_suite_add(ts, TC(test_defaults_get_desktop_bottom));
	g_test_suite_add(ts, TC(test_defaults_get_desktop_left));
	g_test_suite_add(ts, TC(test_defaults_get_desktop_right));
	g_test_suite_add(ts, TC(test_defaults_get_stack_height));
	g_test_suite_add(ts, TC(test_defaults_get_bubble_width));

	return ts;
}
