/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
**      10        20        30        40        50        60        70        80
**
** notify-osd
**
** main.c - pulling it all together
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

#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "defaults.h"
#include "stack.h"
#include "observer.h"
#include "dbus.h"
#include "log.h"

#define ICONS_DIR  (DATADIR G_DIR_SEPARATOR_S "notify-osd" G_DIR_SEPARATOR_S "icons")

int
main (int    argc,
      char** argv)
{
	Defaults*        defaults   = NULL;
	Stack*           stack      = NULL;
	Observer*        observer   = NULL;
	DBusGConnection* connection = NULL;

	g_thread_init (NULL);
	dbus_g_thread_init ();
	log_init ();

	gtk_init (&argc, &argv);

	/* Init some theme/icon stuff */
	gtk_icon_theme_append_search_path(gtk_icon_theme_get_default(),
	                                  ICONS_DIR);

	defaults = defaults_new ();
	observer = observer_new ();
	stack = stack_new (defaults, observer);

	connection = dbus_create_service_instance (DBUS_NAME);
	if (connection == NULL)
	{
		g_warning ("Could not register instance");
		stack_del (stack);
		return 0;
	}

	dbus_g_connection_register_g_object (connection,
					     DBUS_PATH,
					     G_OBJECT (stack));

	gtk_main ();

	stack_del (stack);

	return 0;
}
