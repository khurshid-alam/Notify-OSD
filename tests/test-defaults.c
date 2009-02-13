/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
**      10        20        30        40        50        60        70        80
**
** Codename "alsdorf"
**
** test-defaults.c - implements unit-tests for defaults class
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
