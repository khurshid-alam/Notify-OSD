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

static
void
test_stack_new ()
{
	Stack*    stack = NULL;
	Defaults* defaults = defaults_new ();
	Observer* observer = observer_new ();

	stack = stack_new (defaults, observer);
	g_assert (stack != NULL);
	stack_del (stack);
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
	/*g_assert (stack == NULL);*/
}

GTestSuite *
test_stack_create_test_suite (void)
{
	GTestSuite *ts = NULL;

	ts = g_test_create_suite (__FILE__);

#define TC(x) g_test_create_case(#x, 0, NULL, NULL, x, NULL)

	g_test_suite_add(ts, TC(test_stack_new));
	g_test_suite_add(ts, TC(test_stack_del));

	return ts;
}

