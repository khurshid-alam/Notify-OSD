/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
**      10        20        30        40        50        60        70        80
**
** project:
**    alsdorf
**
** file:
**    test-stack.c
**
** author(s):
**    Mirco "MacSlow" Mueller <mirco.mueller@canonical.com>
**    David Barth <david.barth@canonical.com>
**
** copyright (C) Canonical, oct. 2008
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

	g_object_unref (G_OBJECT (defaults));
	g_object_unref (G_OBJECT (observer));
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

	return ts;
}

