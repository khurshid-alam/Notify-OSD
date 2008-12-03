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
#include "Python.h"

#include "bubble.h"

void
stack_push_notification (gchar* title,
	                 gchar* message)
{
	Bubble* bubble = NULL;

	bubble = bubble_new ();
	bubble_set_title(bubble, title);
	bubble_set_message_body(bubble, message);
        // bubble_set_icon (bubble, "./icons/avatar.svg");
	bubble_set_icon (bubble, "./icons/chat.svg");
	bubble_show (bubble);
}

