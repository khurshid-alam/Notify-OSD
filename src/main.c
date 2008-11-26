/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
**      10        20        30        40        50        60        70        80
**
** project:
**    alsdorf
**
** file:
**    main.c
**
** author(s):
**    Mirco "MacSlow" Mueller <mirco.mueller@canonical.com>
**    David Barth <david.barth@canonical.com>
**
** copyright (C) Canonical, oct. 2008
**
*******************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

int 
main (int    argc,
      char** argv)
{
	gint            x                 = 0;
	gint            y                 = 0;
	gint            width             = 0;
	gint            height            = 0;
	GOptionContext* option_context    = NULL;
	GOptionEntry    options[]         = {{"xposition",
					      'x',
					      0,
					      G_OPTION_ARG_INT,
					      &x,
					      "x-pos of the top-left corner",
					      "X"},
					     {"yposition",
					      'y',
					      0,
					      G_OPTION_ARG_INT,
					      &y,
					      "y-pos of the top-left corner",
					      "Y"},
					     {"width",
					      'w',
					      0,
					      G_OPTION_ARG_INT,
					      &width,
					      "open window with this width",
					      "WIDTH"},
					     {"height",
					      'h',
					      0,
					      G_OPTION_ARG_INT,
					      &height,
					      "open window with this height",
					      "HEIGHT"},
					     {NULL}};

	gtk_init (&argc, &argv);

	python_init(argc, argv);

	/* handle commandline options */
	x      = 30;
	y      = 30;
	width  = 300;
	height = 100;
	option_context = g_option_context_new ("- prototype for some new tech");
	g_option_context_add_main_entries (option_context,
					   options,
					   "notification test");
	g_option_context_parse (option_context, &argc, &argv, NULL);
	g_option_context_free (option_context);

	python_load_script("dbus-link.py");

	gtk_main ();

	python_exit();

	return 0;
}

