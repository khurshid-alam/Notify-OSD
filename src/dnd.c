/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
**      10        20        30        40        50        60        70        80
**
** notify-osd
**
** dnd.c - implements the "do not disturb"-mode, e.g. screensaver, presentations
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

#include <X11/Xproto.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <gdk/gdkx.h>

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

#include "dbus.h"


/* Portions from notification-daemon
 * (C) 2006 Christian Hammond <chipx86@chipx86.com>
 * (C) 2005 John (J5) Palmieri <johnp@redhat.com>
 * (GPLv2 or later version)
 */

/* Portions from gnome-screensaver-command
 * (C) 2004-2006 William Jon McCann <mccann@jhu.edu>
 * (GPLv2 or later version)
 */

static gboolean
is_screensaver_active ()
{
	GdkDisplay *display = gdk_display_get_default ();

	Atom type;
	int format;
	unsigned long nitems, bytes_after;
	unsigned char *temp_data;
	gboolean active = FALSE;
	Atom XA_BLANK = gdk_x11_get_xatom_by_name_for_display(display, "BLANK");
	Atom XA_LOCK = gdk_x11_get_xatom_by_name_for_display(display, "LOCK");

	/* Check for a screensaver first. */
	if (XGetWindowProperty(
		GDK_DISPLAY_XDISPLAY(display),
		GDK_ROOT_WINDOW(),
		gdk_x11_get_xatom_by_name_for_display(display, "_SCREENSAVER_STATUS"),
		0, G_MAXLONG, False, XA_INTEGER, &type, &format, &nitems,
		&bytes_after, &temp_data) == Success &&
		type && temp_data != NULL)
	{
		CARD32 *data = (CARD32 *)temp_data;

		active = (type == XA_INTEGER && nitems >= 3 &&
				  (time_t)data[1] > (time_t)666000000L &&
				  (data[0] == XA_BLANK || data[0] == XA_LOCK));
	}

	if (temp_data != NULL)
		free (temp_data);

	return active;
}

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#define GS_SERVICE   "org.gnome.ScreenSaver"
#define GS_PATH      "/org/gnome/ScreenSaver"
#define GS_INTERFACE "org.gnome.ScreenSaver"

static DBusMessage *
screensaver_send_message_void (DBusConnection *connection,
                               const char     *name,
                               gboolean        expect_reply)
{
        DBusMessage *message;
        DBusMessage *reply;
        DBusError    error;

        g_return_val_if_fail (connection != NULL, NULL);
        g_return_val_if_fail (name != NULL, NULL);

        dbus_error_init (&error);

        message = dbus_message_new_method_call (GS_SERVICE, GS_PATH, GS_INTERFACE, name);
        if (message == NULL) {
                g_warning ("Couldn't allocate the dbus message");
                return NULL;
        }

        if (! expect_reply) {
                if (!dbus_connection_send (connection, message, NULL))
                        g_warning ("could not send message");
                reply = NULL;
        } else {
                reply = dbus_connection_send_with_reply_and_block (connection,
                                                                   message,
                                                                   -1, &error);
                if (dbus_error_is_set (&error)) {
                        g_warning ("%s raised:\n %s\n\n", error.name, error.message);
                        reply = NULL;
                }
        }
        dbus_connection_flush (connection);

        dbus_message_unref (message);
        dbus_error_free (&error);

        return reply;
}

static char **
get_string_from_iter (DBusMessageIter *iter,
                      int             *num_elements)
{
        int    count;
        char **buffer;

        count = 0;
        buffer = (char **)malloc (sizeof (char *) * 8);

        if (buffer == NULL) {
                goto oom;
        }

        buffer[0] = NULL;
        while (dbus_message_iter_get_arg_type (iter) == DBUS_TYPE_STRING) {
                const char *value;
                char       *str;

                if ((count % 8) == 0 && count != 0) {
                        buffer = realloc (buffer, sizeof (char *) * (count + 8));
                        if (buffer == NULL) {
                                goto oom;
                        }
                }

                dbus_message_iter_get_basic (iter, &value);
                str = g_strdup (value);
                if (str == NULL) {
                        goto oom;
                }

                buffer[count] = str;

                dbus_message_iter_next (iter);
                count++;
        }

        if ((count % 8) == 0) {
                buffer = realloc (buffer, sizeof (char *) * (count + 1));
                if (buffer == NULL) {
                        goto oom;
                }
        }

        buffer[count] = NULL;
        if (num_elements != NULL) {
                *num_elements = count;
        }
        return buffer;

oom:
        g_debug ("%s %d : error allocating memory\n", __FILE__, __LINE__);
        return NULL;

}

static gboolean
is_screensaver_inhibited ()
{
        DBusMessage    *reply;
        DBusMessageIter iter;
	DBusMessageIter array;
	gboolean        res = FALSE;

	DBusConnection *connection = dbus_get_connection ();
	reply = screensaver_send_message_void (connection, "GetInhibitors", TRUE);
	if (! reply) {
		g_debug ("Did not receive a reply from the screensaver.");
		return FALSE;
	}
	
	dbus_message_iter_init (reply, &iter);
	dbus_message_iter_recurse (&iter, &array);
	
	if (dbus_message_iter_get_arg_type (&array) == DBUS_TYPE_INVALID) {
	} else {
		char **inhibitors;
		int    num = 0;
		
		g_debug ("The screensaver is being inhibited");
		inhibitors = get_string_from_iter (&array, &num);
		g_strfreev (inhibitors);

		res = TRUE;
	}
	
	dbus_message_unref (reply);

	return res;
}

static gboolean
is_online_presence_dnd ()
{
	/* TODO: ask FUSA if we're in DND mode */

	return FALSE;
}

/* Tries to determine whether the user is in "do not disturb" mode */
gboolean
dnd_dont_disturb_user (void)
{
	return (is_online_presence_dnd()
		|| is_screensaver_active()
		|| is_screensaver_inhibited()
		);
}
