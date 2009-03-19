/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
**      10        20        30        40        50        60        70        80
**
** notify-osd
**
** dnd.c - implements the "do not disturb"-mode, e.g. gsmgr, presentations
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

#include "config.h"
#include <stdlib.h>

#include <glib.h>
#include <glib-object.h>

#include <dbus/dbus-glib.h>

#include <X11/Xproto.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <gdk/gdkx.h>

#include "dbus.h"

static DBusGProxy *gsmgr = NULL;

gboolean
dnd_is_xscreensaver_active ()
{
	GdkDisplay *display = gdk_display_get_default ();

	Atom XA_BLANK =
		gdk_x11_get_xatom_by_name_for_display(display, "BLANK");
	Atom XA_LOCK =
		gdk_x11_get_xatom_by_name_for_display(display, "BLANK");
	Atom XA_SCREENSAVER_STATUS =
		gdk_x11_get_xatom_by_name_for_display(display,
						      "_SCREENSAVER_STATUS");
	gboolean active = FALSE;

	Atom type;
	int format;
	int status;
	unsigned long nitems, bytesafter;
	CARD32 *data = 0;

	Display *dpy = gdk_x11_get_default_xdisplay ();
	Window   win = gdk_x11_get_default_root_xwindow ();

	status = XGetWindowProperty (dpy, win,
				     XA_SCREENSAVER_STATUS,
				     0, 999, False, XA_INTEGER,
				     &type, &format, &nitems, &bytesafter,
				     (unsigned char **) &data);
	if (status == Success
	    && type == XA_INTEGER
	    && nitems >= 3
	    && data != NULL)
	{
		active = (data[0] == XA_BLANK || data[0] == XA_LOCK)
			&& ((time_t)data[1] > (time_t)666000000L);

		g_debug ("Screensaver is currently active");
	}

	if (data != NULL)
		free (data);

	return active;
}

static DBusGProxy*
get_screensaver_proxy (void)
{
	if (gsmgr == NULL)
	{
		DBusGConnection *connection = dbus_get_connection ();
		gsmgr = dbus_g_proxy_new_for_name (connection,
						   "org.gnome.ScreenSaver",
						   "/org/gnome/ScreenSaver",
						   "org.gnome.ScreenSaver");
	}

	return gsmgr;
}	

gboolean
dnd_is_screensaver_inhibited ()
{
	GError  *error;
	gboolean inhibited = FALSE;
	char **list;

	if (! get_screensaver_proxy ())
		return FALSE;

	if (dbus_g_proxy_call_with_timeout (
		    gsmgr, "GetInhibitors", 2000, &error,
		    G_TYPE_INVALID,
		    G_TYPE_STRV, &list,
		    G_TYPE_INVALID))
	{
		/* if the list is not empty, the screensaver is inhibited */
		if (*list)
		{
			inhibited = TRUE;
			g_debug ("Screensaver has been inhibited");
		}
		g_strfreev (list);
	}

	return inhibited;
}

gboolean
dnd_is_screensaver_active ()
{
	GError  *error;
	gboolean active = FALSE;;

	if (! get_screensaver_proxy ())
		return FALSE;

	dbus_g_proxy_call_with_timeout (
		gsmgr, "GetActive", 2000, &error,
		G_TYPE_INVALID,
		G_TYPE_BOOLEAN, &active,
		G_TYPE_INVALID);

	if (active)
		g_debug ("Gnome screensaver is active");

	return active;
}

static gboolean
dnd_is_online_presence_dnd ()
{
	/* TODO: ask FUSA if we're in DND mode */

	return FALSE;
}

/* Tries to determine whether the user is in "do not disturb" mode */
gboolean
dnd_dont_disturb_user (void)
{
	return (dnd_is_online_presence_dnd()
		|| dnd_is_xscreensaver_active()
		|| dnd_is_screensaver_active()
		|| dnd_is_screensaver_inhibited()
		);
}
