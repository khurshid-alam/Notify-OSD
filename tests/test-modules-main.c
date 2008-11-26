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

#include "test-bubble.h"
#include "test-defaults.h"
#include "test-observer.h"
#include "test-stack.h"

int
main (int    argc,
      char** argv)
{
	gint result;

	g_test_init (&argc, &argv, NULL);
	gtk_init (&argc, &argv);

	/* test module bubble */
	g_test_add_func ("/bubble/test new",
			 test_bubble_new);

	/* test module defaults */
	g_test_add_func ("/defaults/test new",
			 test_defaults_new);
	g_test_add_func ("/defaults/test del",
			 test_defaults_del);
	g_test_add_func ("/defaults/test initialized",
			 test_defaults_initialized);
	g_test_add_func ("/defaults/test get_desktop_width",
			 test_defaults_get_desktop_width);
	g_test_add_func ("/defaults/test get_desktop_height",
			 test_defaults_get_desktop_height);
	g_test_add_func ("/defaults/test get_bubble_width",
			 test_defaults_get_bubble_width);
	g_test_add_func ("/defaults/test get_bubble_height",
			 test_defaults_get_bubble_height);
	g_test_add_func ("/defaults/test get_bubble_opacity",
			 test_defaults_get_bubble_opacity);
	g_test_add_func ("/defaults/test get_bubble_color",
			 test_defaults_get_bubble_color);
	g_test_add_func ("/defaults/test get_font_dpi",
			 test_defaults_get_font_dpi);
	g_test_add_func ("/defaults/test get_font_size",
			 test_defaults_get_font_size);
	g_test_add_func ("/defaults/test get_font_face",
			 test_defaults_get_font_face);

	/* test module observer */
	g_test_add_func ("/observer/test new",
			 test_observer_new);
	g_test_add_func ("/observer/test del",
			 test_observer_del);
	g_test_add_func ("/observer/test get_x",
			 test_observer_get_x);
	g_test_add_func ("/observer/test get_y",
			 test_observer_get_y);

	/* test module stack */
	g_test_add_func ("/stack/test new",
			 test_stack_new);
	g_test_add_func ("/stack/test del",
			 test_stack_del);
	g_test_add_func ("/stack/test get_next_id",
			 test_stack_get_next_id);

	result = g_test_run ();

	return result;
}

