////////////////////////////////////////////////////////////////////////////////
//3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
//      10        20        30        40        50        60        70        80
//
// notify-osd
//
// notification.c - notification object storing attributes like title- and body-
//                  text, value, icon, id, sender-pid etc.
//
// Copyright 2009 Canonical Ltd.
//
// Authors:
//    Mirco "MacSlow" Mueller <mirco.mueller@canonical.com>
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License version 3, as published
// by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranties of
// MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
// PURPOSE.  See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////////////

#include "notification.h"

#define RETURN_GCHAR(n, string) \
	if (!n)\
		return NULL;\
	if (!n->priv)\
		return NULL;\
	if (!n->priv->string)\
		return NULL;\
	if (!n->priv->string->str)\
		return NULL;\
	return n->priv->string->str;

#define SET_GCHAR(n, string) \
	if (!n)\
		return;\
	if (!n->priv)\
		return;\
	_set_text (&n->priv->string, string);

struct _notification_private_t
{
	gint       id;                  // unique notification-id
	GString*   title;               // summary-text strdup'ed from setter
	GString*   body;                // body-text strdup'ed from setter
	gint       value;               // range: 0..100, special: -1, 101
	GString*   icon_themename;      // e.g. "notification-message-email"
	GString*   icon_filename;       // e.g. "/usr/share/icons/icon.png"
	GdkPixbuf* icon_pixbuf;         // from setter memcpy'ed pixbuf 
	gint       onscreen_time;       // time on-screen in ms
	GString*   sender_name;         // app-name, strdup'ed from setter
	gint       sender_pid;          // pid of sending application
	GTimeVal   reception_timestamp; // timestamp of  reception
};

// a little utility function to help avoid code-duplication
void
_set_text (GString** string,
	   gchar*    text)
{
	// sanity checks
	if (!text)
		return;

	// see if we already have a GString set
	if (*string)
	{
		GString* new_string = g_string_new (text);

		// is the new text different from the stored one
		if (g_string_equal (*string, new_string))
		{
			// if not, free new one and leave everything untouched
			g_string_free (new_string, TRUE);
			return;
		}
		else
		{
			// if it is, delete old and store new one
			g_string_free (*string, TRUE);
			*string = new_string;
		}
	}
	else
		*string = g_string_new (text);
}

notification_t*
notification_new ()
{
	notification_private_t* priv = NULL;
	notification_t*         n = NULL;

	priv = g_new0 (notification_private_t, 1);
	if (!priv)
		return NULL;

	n = g_new0 (notification_t, 1);
	if (!n)
		return NULL;

	n->priv = priv;

	return n;
}

void
notification_destroy (notification_t* n)
{
	// sanity checks
	if (!n)
		return;

	if (!n->priv)
	{
		g_free ((gpointer) n);
		return;
	}

	// free any allocated string of pixbuf
	if (n->priv->title)
	{
		g_string_free (n->priv->title, TRUE);
		n->priv->title = NULL;
	}

	if (n->priv->body)
	{
		g_string_free (n->priv->body, TRUE);
		n->priv->body = NULL;
	}

	if (n->priv->icon_themename)
	{
		g_string_free (n->priv->icon_themename, TRUE);
		n->priv->icon_themename = NULL;
	}

	if (n->priv->icon_filename)
	{
		g_string_free (n->priv->icon_filename, TRUE);
		n->priv->icon_filename = NULL;
	}

	if (n->priv->icon_pixbuf)
	{
		g_object_unref ((gpointer) n->priv->icon_pixbuf);
		n->priv->icon_pixbuf = NULL;
	}

	if (n->priv->sender_name)
	{
		g_string_free (n->priv->sender_name, TRUE);
		n->priv->sender_name = NULL;
	}

	// get rid of the main allocated structs
	g_free ((gpointer) n->priv);
	n->priv = NULL;
	g_free ((gpointer) n);
	n = NULL;
}

gint
notification_get_id (notification_t* n)
{
	// sanity checks
	if (!n)
		return -1;

	if (!n->priv)
		return -1;

	return n->priv->id;
}

void
notification_set_id (notification_t* n,
		     gint            id)
{
	// sanity checks
	if (!n)
		return;

	if (!n->priv)
		return;

	// an id is not allowed to be negative
	if (id < 0)
		return;

	n->priv->id = id;
}

gchar*
notification_get_title (notification_t* n)
{
	RETURN_GCHAR (n, title)
}

void
notification_set_title (notification_t* n,
			gchar*          title)
{
	SET_GCHAR (n, title)
}

gchar*
notification_get_body (notification_t* n)
{
	RETURN_GCHAR (n, body)
}

void
notification_set_body (notification_t* n,
		       gchar*          body)
{
	SET_GCHAR (n, body)
}

// the allowed range for stored values is -1..101, thus a return-value of -2
// indicates an error on behalf of the caller
gint
notification_get_value (notification_t* n)
{
	// sanity checks
	if (!n)
		return -2;

	if (!n->priv)
		return -2;

	return n->priv->value;
}

void
notification_set_value (notification_t* n,
			gint            value)
{
	// sanity checks
	if (!n)
		return;

	if (!n->priv)
		return;

	// don't store any values outside of allowed range -1..101
	if (value < NOTIFICATION_VALUE_MIN_ALLOWED)
	{
		n->priv->value = NOTIFICATION_VALUE_MIN_ALLOWED;
		return;
	}

	if (value > NOTIFICATION_VALUE_MAX_ALLOWED)
	{
		n->priv->value = NOTIFICATION_VALUE_MAX_ALLOWED;
		return;
	}

	n->priv->value = value;
}

gchar*
notification_get_icon_themename (notification_t* n)
{
	RETURN_GCHAR (n, icon_themename)
}

void
notification_set_icon_themename (notification_t* n,
				 gchar*          icon_themename)
{
	SET_GCHAR (n, icon_themename)
}

gchar*
notification_get_icon_filename (notification_t* n)
{
	RETURN_GCHAR (n, icon_filename)
}

void
notification_set_icon_filename (notification_t* n,
				gchar*          icon_filename)
{
	SET_GCHAR (n, icon_filename)
}

GdkPixbuf*
notification_get_icon_pixbuf (notification_t* n)
{
	// sanity checks
	if (!n)
		return NULL;

	if (!n->priv)
		return NULL;

	// see if we actually have an icon_pixbuf set
	if (!n->priv->icon_pixbuf)
		return NULL;

	return n->priv->icon_pixbuf;
}

void
notification_set_icon_pixbuf (notification_t* n,
			      GdkPixbuf*      icon_pixbuf)
{
	// sanity checks
	if (!n)
		return;

	if (!n->priv)
		return;

	if (!icon_pixbuf)
		return;

	// free any previous stored pixbuf
	if (n->priv->icon_pixbuf)
		g_object_unref (n->priv->icon_pixbuf);

	// create a new/private copy of the supplied pixbuf
	n->priv->icon_pixbuf = gdk_pixbuf_copy (icon_pixbuf);
}

// a return-value of -1 indicates an error on behalf of the caller, a
// return-value of 0 would indicate that a notification has not been displayed
// yet
gint
notification_get_onscreen_time (notification_t* n)
{
	// sanity checks
	if (!n)
		return -1;

	if (!n->priv)
		return -1;

	return n->priv->onscreen_time;
}

void
notification_set_onscreen_time (notification_t* n,
				gint            onscreen_time)
{
	// sanity checks
	if (!n)
		return;

	if (!n->priv)
		return;

	// see if the caller is really stupid and passes a negative time
	if (onscreen_time < 0)
		return;

	// onscreen-time can only increase not decrease
	if (n->priv->onscreen_time > onscreen_time)
		return;

	// you made it upto here, congratulations... let's store the new value
	n->priv->onscreen_time = onscreen_time;
}

gchar*
notification_get_sender_name (notification_t* n)
{
	RETURN_GCHAR (n, sender_name)
}

void
notification_set_sender_name (notification_t* n,
			      gchar*          sender_name)
{
	SET_GCHAR (n, sender_name)
}

// a return-value of -1 indicates an error on behalf of the caller, PIDs are
// never negative
gint
notification_get_sender_pid (notification_t* n)
{
	// sanity checks
	if (!n)
		return -1;

	if (!n->priv)
		return -1;

	return n->priv->sender_pid;
}

void
notification_set_sender_pid (notification_t* n,
			     gint            sender_pid)
{
	// sanity checks
	if (!n)
		return;

	if (!n->priv)
		return;

	// it's hardly possible we'll get a notification from init, but anyway
	if (sender_pid < 1)
		return;

	n->priv->sender_pid = sender_pid;
}

// the caller only gets a pointer to the timestamp
GTimeVal*
notification_get_reception_timestamp (notification_t* n)
{
	// sanity checks
	if (!n)
		return NULL;

	if (!n->priv)
		return NULL;

	return &n->priv->reception_timestamp;
}

void
notification_set_reception_timestamp (notification_t* n,
				      GTimeVal*       reception_timestamp)
{
	// sanity checks
	if (!n)
		return;

	if (!n->priv)
		return;

	// being overly cautious again
	if (!reception_timestamp)
		return;

	// don't store older timestamp over newer one
	if (n->priv->reception_timestamp.tv_sec > reception_timestamp->tv_sec)
		return;

	n->priv->reception_timestamp.tv_sec  = reception_timestamp->tv_sec;
	n->priv->reception_timestamp.tv_usec = reception_timestamp->tv_usec;
}

