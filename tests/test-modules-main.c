/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
**      10        20        30        40        50        60        70        80
**
** project:
**    alsdorf
**
** file:
**    test-modules-main.c
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

/* Declare entries located in the individual test modules
   (avoids a superflous .h file)
*/
GTestSuite *test_bubble_create_test_suite (void);
GTestSuite *test_defaults_create_test_suite (void);
GTestSuite *test_observer_create_test_suite (void);
GTestSuite *test_stack_create_test_suite (void);


int
main (int    argc,
      char** argv)
{
	gint result;

	g_test_init (&argc, &argv, NULL);
	gtk_init (&argc, &argv);

	GTestSuite *suite = NULL;

	suite = g_test_get_root ();

	g_test_suite_add_suite (suite, test_bubble_create_test_suite ());
	g_test_suite_add_suite (suite, test_defaults_create_test_suite ());
	g_test_suite_add_suite (suite, test_observer_create_test_suite ());
	g_test_suite_add_suite (suite, test_stack_create_test_suite ());
	
	result = g_test_run ();

	return result;
}

