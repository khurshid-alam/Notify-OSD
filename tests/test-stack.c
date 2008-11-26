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

void
test_stack_new ()
{
	Stack* stack = NULL;

	stack = stack_new ();
	g_assert (stack != NULL);
	stack_del (stack);
}

void
test_stack_del ()
{
	Stack* stack = NULL;

	stack = stack_new ();
	stack_del (stack);
	/*g_assert (stack == NULL);*/
}

void
test_stack_get_next_id ()
{
	Stack* stack = NULL;
	guint  id;

	stack = stack_new ();
	id = stack_get_next_id (stack);
	g_assert_cmpint (id, ==, 1);
	id = stack_get_next_id (stack);
	g_assert_cmpint (id, ==, 2);
	id = stack_get_next_id (stack);
	g_assert_cmpint (id, ==, 3);
	stack_del (stack);
}

