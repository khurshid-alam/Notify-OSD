/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
**      10        20        30        40        50        60        70        80
**
** notify-osd
**
** test-stack.c - unit-tests for stack class
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

#include "stack.h"
#include "defaults.h"
#include "bubble.h"

static
void
test_stack_new ()
{
	Stack*    stack = NULL;
	Defaults* defaults = defaults_new ();
	Observer* observer = observer_new ();

	stack = stack_new (defaults, observer);
	g_assert (stack != NULL);
}

static
void
test_stack_del ()
{
	Stack*    stack = NULL;
	Defaults* defaults = defaults_new ();
	Observer* observer = observer_new ();

	stack = stack_new (defaults, observer);
	stack_del (stack);
}

static
void
test_stack_push ()
{
	Stack*       stack = NULL;
	Defaults* defaults = defaults_new ();
	Observer* observer = observer_new ();
	Bubble*     bubble = bubble_new (defaults);
	guint           id = -1;
	guint  replaced_id = -1;

	stack = stack_new (defaults, observer);
	id = stack_push_bubble (stack, bubble);

	g_assert (id > 0);

	replaced_id = stack_push_bubble (stack, bubble);

	g_assert (replaced_id == id);

	g_object_unref (G_OBJECT (stack));
}

static void
test_stack_placement ()
{
	Stack*       stack = NULL;
	Defaults* defaults = defaults_new ();
	Observer* observer = observer_new ();

	stack = stack_new (defaults, observer);

	// upon creation the placement method should not be unset
	g_assert_cmpint (stack_get_placement (stack), !=, PLACEMENT_NONE);

	// currently the default value should be the new placement method
	g_assert_cmpint (stack_get_placement (stack), ==, PLACEMENT_NEW);

	// check if we can pass "crap" to the call without causing a crash
	g_assert_cmpint (stack_get_placement (NULL), ==, PLACEMENT_NONE);

	g_object_unref (G_OBJECT (stack));
}

static void
test_stack_slots ()
{
	Stack*    stack = NULL;
	Defaults* defaults = defaults_new ();
	Observer* observer = observer_new ();
	gint      x;
	gint      y;

	stack = stack_new (defaults, observer);

	// check if stack_is_slot_vacant() can take "crap" without crashing
	g_assert_cmpint (stack_is_slot_vacant (NULL, SLOT_TOP), ==, FALSE);
	g_assert_cmpint (stack_is_slot_vacant (stack, 832), ==, FALSE);
	g_assert_cmpint (stack_is_slot_vacant (NULL, 4321), ==, FALSE);

	// check if stack_get_slot_position can take "crap" without crashing
	stack_get_slot_position (NULL, SLOT_TOP, &x, &y);
	g_assert_cmpint (x, ==, -1);
	g_assert_cmpint (y, ==, -1);
	stack_get_slot_position (stack, 4711, &x, &y);
	g_assert_cmpint (x, ==, -1);
	g_assert_cmpint (y, ==, -1);
	stack_get_slot_position (NULL, 42, NULL, NULL);

	// initially both slots should be empty
	g_assert_cmpint (stack_is_slot_vacant (stack, SLOT_TOP), ==, TRUE);
	g_assert_cmpint (stack_is_slot_vacant (stack, SLOT_BOTTOM), ==, TRUE);

	g_object_unref (G_OBJECT (stack));
}

GTestSuite *
test_stack_create_test_suite (void)
{
	GTestSuite *ts = NULL;

	ts = g_test_create_suite ("stack");

#define TC(x) g_test_create_case(#x, 0, NULL, NULL, x, NULL)

	g_test_suite_add(ts, TC(test_stack_new));
	g_test_suite_add(ts, TC(test_stack_del));
	g_test_suite_add(ts, TC(test_stack_push));
	g_test_suite_add(ts, TC(test_stack_placement));
	g_test_suite_add(ts, TC(test_stack_slots));

	return ts;
}
