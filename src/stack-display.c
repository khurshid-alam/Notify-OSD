/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
**      10        20        30        40        50        60        70        80
**
** project:
**    alsdorf
**
** file:
**    stack-display.c
**
** author(s):
**    Mirco "MacSlow" Mueller <mirco.mueller@canonical.com>
**    David Barth <david.barth@canonical.com>
**
** copyright (C) Canonical, oct. 2008
**
*******************************************************************************/

#include <gtk/gtk.h>

void
stack_push_notification (char *title,
	                 char *message)
{
	GtkWidget*      window            = NULL;

	window = bubble_new (30, 30, 300, 100);
	gtk_widget_show_all (window);
}
