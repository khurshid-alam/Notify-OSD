////////////////////////////////////////////////////////////////////////////////
//3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
//      10        20        30        40        50        60        70        80
//
// notify-osd
//
// notification.h - notification object storing attributes like title- and body-
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

#ifndef _NOTIFICATION_H
#define _NOTIFICATION_H

#define NOTIFICATION_VALUE_MIN_ALLOWED  -1
#define NOTIFICATION_VALUE_MAX_ALLOWED 101

#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

typedef enum
{
	URGENCY_LOW = 0,
	URGENCY_NORMAL,
	URGENCY_HIGH
} Urgency;

typedef struct _notification_private_t notification_private_t;

typedef struct _notification_t
{
	notification_private_t* priv;
} notification_t;

notification_t*
notification_new ();

void
notification_destroy (notification_t* n);

gint
notification_get_id (notification_t* n);

void
notification_set_id (notification_t* n,
		     gint            id);

gchar*
notification_get_title (notification_t* n);

void
notification_set_title (notification_t* n,
			gchar*          title);

gchar*
notification_get_body (notification_t* n);

void
notification_set_body (notification_t* n,
		       gchar*          body);

gint
notification_get_value (notification_t* n);

void
notification_set_value (notification_t* n,
			gint            value);

gchar*
notification_get_icon_themename (notification_t* n);

void
notification_set_icon_themename (notification_t* n,
				 gchar*          icon_themename);

gchar*
notification_get_icon_filename (notification_t* n);

void
notification_set_icon_filename (notification_t* n,
				gchar*          icon_filename);

GdkPixbuf*
notification_get_icon_pixbuf (notification_t* n);

void
notification_set_icon_pixbuf (notification_t* n,
			      GdkPixbuf*      icon_pixbuf);

gint
notification_get_onscreen_time (notification_t* n);

void
notification_set_onscreen_time (notification_t* n,
				gint            onscreen_time);

gchar*
notification_get_sender_name (notification_t* n);

void
notification_set_sender_name (notification_t* n,
			      gchar*          sender_name);

gint
notification_get_sender_pid (notification_t* n);

void
notification_set_sender_pid (notification_t* n,
			     gint            sender_pid);

GTimeVal*
notification_get_reception_timestamp (notification_t* n);

void
notification_set_reception_timestamp (notification_t* n,
				      GTimeVal*       reception_timestamp);

gint
notification_get_urgency (notification_t* n);

void
notification_set_urgency (notification_t* n,
			  Urgency         urgency);

#endif // _NOTIFICATION_H

