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
	bubble_move (bubble, 30, 30);
	bubble_set_size (bubble, 300, 100);
	bubble_set_title(bubble, title);
	bubble_set_message_body(bubble, message);
	bubble_set_icon (bubble, "./icons/avatar.svg");
	bubble_show (bubble);
}

static PyObject *
stack_push_notification_py (PyObject *self,
			    PyObject *args)
{
	char *title = NULL;
	char *message = NULL;

	if (!PyArg_ParseTuple(args, "ss", &title, &message))
		return NULL;
	stack_push_notification(title, message);

	return Py_BuildValue("");
}


static PyMethodDef notification_methods[] = {
	{"push", stack_push_notification_py, METH_VARARGS, "Insert a new message in the notification stack"},
	{NULL,		NULL}		/* sentinel */
};

void
stack_init_python_interface (void)
{
	PyImport_AddModule("notifications");
	Py_InitModule("notifications", notification_methods);
}
