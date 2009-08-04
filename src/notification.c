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

G_DEFINE_TYPE (Notification, notification, G_TYPE_OBJECT);

enum
{
	PROP_DUMMY = 0,
	PROP_ID,
	PROP_TITLE,
	PROP_BODY,
	PROP_VALUE,
	PROP_ICON_THEMENAME,
	PROP_ICON_FILENAME,
	PROP_ICON_PIXBUF,
	PROP_ONSCREEN_TIME,
	PROP_SENDER_NAME,
	PROP_SENDER_PID,
	PROP_TIMESTAMP,
	PROP_URGENCY
};

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), NOTIFICATION_TYPE, NotificationPrivate))

struct _NotificationPrivate {
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
	GTimeVal   timestamp;           // timestamp of  reception
	Urgency    urgency;             // urgency-level: low, normal, high
};

//-- private functions ---------------------------------------------------------

//-- internal functions --------------------------------------------------------

// this is what gets called if one does g_object_unref(bla)
static void
notification_dispose (GObject* gobject)
{
	Notification*        n;
	NotificationPrivate* priv;

	// sanity checks
	g_assert (gobject);
	n = NOTIFICATION (gobject);
	g_assert (n);
	g_assert (IS_NOTIFICATION (n));
	priv = GET_PRIVATE (n);
	g_assert (priv);

	// free any allocated resources
	if (priv->title)
	{
		g_string_free (priv->title, TRUE);
		priv->title = NULL;
	}

	if (priv->body)
	{
		g_string_free (priv->body, TRUE);
		priv->body = NULL;
	}

	if (priv->icon_themename)
	{
		g_string_free (priv->icon_themename, TRUE);
		priv->icon_themename = NULL;
	}

	if (priv->icon_filename)
	{
		g_string_free (priv->icon_filename, TRUE);
		priv->icon_filename = NULL;
	}

	if (priv->icon_pixbuf)
	{
		g_object_unref (priv->icon_pixbuf);
	}

	if (priv->sender_name)
	{
		g_string_free (priv->sender_name, TRUE);
		priv->sender_name = NULL;
	}

	//GTimeVal reception_timestamp;

	// chain up to the parent class
	G_OBJECT_CLASS (notification_parent_class)->dispose (gobject);
}

static void
notification_finalize (GObject* gobject)
{
	// gee, I wish I knew the difference between foobar_dispose() and
	// foobar_finalize()

	// chain up to the parent class
	G_OBJECT_CLASS (notification_parent_class)->finalize (gobject);
}

static void
notification_init (Notification* n)
{
	NotificationPrivate* priv;

	// sanity checks
	g_assert (n);
	priv = GET_PRIVATE (n);
	g_assert (priv);

	priv->id                = -1;
	priv->title             = NULL;
	priv->body              = NULL;
	priv->value             = -2;
	priv->icon_themename    = NULL;
	priv->icon_filename     = NULL;
	priv->icon_pixbuf       = NULL;
	priv->onscreen_time     = 0;
	priv->sender_name       = NULL;
	priv->sender_pid        = -1;
	priv->timestamp.tv_sec  = 0;
	priv->timestamp.tv_usec = 0;
	priv->urgency           = URGENCY_NONE;
}

static void
notification_get_property (GObject*    gobject,
			   guint       prop,
			   GValue*     value,
			   GParamSpec* spec)
{
	Notification* n = NOTIFICATION (gobject);

	// sanity checks are done in public getters

	switch (prop)
	{
		case PROP_ID:
			g_value_set_int (value, notification_get_id (n));
		break;

		case PROP_TITLE:
			g_value_set_string (value, notification_get_title (n));
		break;

		case PROP_BODY:
			g_value_set_string (value, notification_get_body (n));
		break;

		case PROP_VALUE:
			g_value_set_int (value, notification_get_value (n));
		break;

		case PROP_ICON_THEMENAME:
			g_value_set_string (value,
					    notification_get_icon_themename (n));
		break;

		case PROP_ICON_FILENAME:
			g_value_set_string (value,
					    notification_get_icon_filename (n));
		break;

		case PROP_ICON_PIXBUF:
			g_value_set_pointer (value,
					     notification_get_icon_pixbuf (n));
		break;

		case PROP_ONSCREEN_TIME:
			g_value_set_int (value,
					 notification_get_onscreen_time (n));
		break;

		case PROP_SENDER_NAME:
			g_value_set_string (value,
					    notification_get_sender_name (n));
		break;

		case PROP_SENDER_PID:
			g_value_set_int (value,
					 notification_get_sender_pid (n));
		break;

		case PROP_TIMESTAMP:
			g_value_set_pointer (value,
					     notification_get_timestamp (n));
		break;

		case PROP_URGENCY:
			g_value_set_int (value,
					 notification_get_urgency (n));
		break;

		default :
			G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop, spec);
		break;
	}
}

static void
notification_set_property (GObject*      gobject,
			   guint         prop,
			   const GValue* value,
			   GParamSpec*   spec)
{
	Notification* n = NOTIFICATION (gobject);

	// sanity checks are done in the public setters

	switch (prop)
	{
		case PROP_ID:
			notification_set_id (n, g_value_get_int (value));
		break;

		case PROP_TITLE:
			notification_set_title (n, g_value_get_string (value));
		break;

		case PROP_BODY:
			notification_set_body (n, g_value_get_string (value));
		break;

		case PROP_VALUE:
			notification_set_value (n, g_value_get_int (value));
		break;

		case PROP_ICON_THEMENAME:
			notification_set_icon_themename (
				n,
				g_value_get_string (value));
		break;

		case PROP_ICON_FILENAME:
			notification_set_icon_filename (
				n,
				g_value_get_string (value));
		break;

		case PROP_ICON_PIXBUF:
			notification_set_icon_pixbuf (
				n,
				g_value_get_pointer (value));
		break;

		case PROP_ONSCREEN_TIME:
			notification_set_onscreen_time (
				n,
				g_value_get_int (value));
		break;

		case PROP_SENDER_NAME:
			notification_set_sender_name (
				n,
				g_value_get_string (value));
		break;

		case PROP_SENDER_PID:
			notification_set_sender_pid (
				n,
				g_value_get_int (value));
		break;

		case PROP_TIMESTAMP:
			notification_set_timestamp (
				n,
				g_value_get_pointer (value));
		break;

		case PROP_URGENCY:
			notification_set_urgency (
				n,
				g_value_get_int (value));
		break;

		default :
			G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop, spec);
		break;
	}
}

static void
notification_class_init (NotificationClass* klass)
{
	GObjectClass* gobject_class = G_OBJECT_CLASS (klass);
	GParamSpec*   property_id;
	GParamSpec*   property_title;
	GParamSpec*   property_body;
	GParamSpec*   property_value;
	GParamSpec*   property_icon_themename;
	GParamSpec*   property_icon_filename;
	GParamSpec*   property_icon_pixbuf;
	GParamSpec*   property_onscreen_time;
	GParamSpec*   property_sender_name;
	GParamSpec*   property_sender_pid;
	GParamSpec*   property_timestamp;
	GParamSpec*   property_urgency;

	g_type_class_add_private (klass, sizeof (NotificationPrivate));

	gobject_class->dispose      = notification_dispose;
	gobject_class->finalize     = notification_finalize;
	gobject_class->get_property = notification_get_property;
	gobject_class->set_property = notification_set_property;

	property_id = g_param_spec_int ("id",
					"id",
					"unique notification id",
					G_MININT,
					G_MAXINT,
					G_MININT,
					G_PARAM_CONSTRUCT |
					G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_ID,
					 property_id);

	property_title = g_param_spec_string ("title",
					      "title",
					      "title-text of a notification",
					      "",
					      G_PARAM_CONSTRUCT |
					      G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_TITLE,
					 property_title);

	property_body = g_param_spec_string ("body",
					     "body",
					     "body-text of a notification",
					     "",
					     G_PARAM_CONSTRUCT |
					     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_BODY,
					 property_body);

	property_value = g_param_spec_int ("value",
					   "value",
					   "value between -1..101 to render",
					   -2,
					   101,
					   -2,
					   G_PARAM_CONSTRUCT |
					   G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_VALUE,
					 property_value);

	property_icon_themename = g_param_spec_string (
					"icon-themename",
					"icon-themename",
					"theme-name of icon to use",
					"",
					G_PARAM_CONSTRUCT |
					G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_ICON_THEMENAME,
					 property_icon_themename);

	property_icon_filename = g_param_spec_string (
					"icon-filename",
					"icon-filename",
					"file-name of icon to use",
					"",
					G_PARAM_CONSTRUCT |
					G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_ICON_FILENAME,
					 property_icon_filename);

	property_icon_pixbuf = g_param_spec_pointer ("icon-pixbuf",
						     "icon-pixbuf",
						     "pixbuf of icon to use",
						     G_PARAM_CONSTRUCT |
						     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_ICON_PIXBUF,
					 property_icon_pixbuf);

	property_onscreen_time = g_param_spec_int ("onscreen-time",
						   "onscreen-time",
						   "time on screen sofar",
						   0,
						   15000,
						   0,
						   G_PARAM_CONSTRUCT |
						   G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_ONSCREEN_TIME,
					 property_onscreen_time);

	property_sender_name = g_param_spec_string (
					"sender-name",
					"sender-name",
					"name of sending application",
					"",
					G_PARAM_CONSTRUCT |
					G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_SENDER_NAME,
					 property_sender_name);

	property_sender_pid = g_param_spec_int (
					"sender-pid",
					"sender-pid",
					"process ID of sending application",
					G_MININT,
					G_MAXINT,
					G_MININT,
					G_PARAM_CONSTRUCT |
					G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_SENDER_PID,
					 property_sender_pid);

	property_timestamp = g_param_spec_pointer ("timestamp",
						   "timestamp",
						   "timestamp of reception",
					 	   G_PARAM_CONSTRUCT |
						   G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_TIMESTAMP,
					 property_timestamp);

	property_urgency = g_param_spec_int ("urgency",
					     "urgency",
					     "urgency-level of notification",
					     URGENCY_LOW,
					     URGENCY_NONE,
					     URGENCY_NONE,
					     G_PARAM_CONSTRUCT |
					     G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_URGENCY,
					 property_urgency);
}

//-- public functions ----------------------------------------------------------

Notification*
notification_new ()
{
	return g_object_new (NOTIFICATION_TYPE, NULL);
}

gint
notification_get_id (Notification* n)
{
	g_return_val_if_fail (IS_NOTIFICATION (n), -1);

	return GET_PRIVATE (n)->id;
}

void
notification_set_id (Notification* n,
		     gint          id)
{
	g_assert (IS_NOTIFICATION (n));

	GET_PRIVATE (n)->id = id;
}

gchar*
notification_get_title (Notification* n)
{
	NotificationPrivate* priv;

	g_return_val_if_fail (IS_NOTIFICATION (n), NULL);

	priv = GET_PRIVATE (n);

	if (!priv->title)
		return NULL;

	return GET_PRIVATE (n)->title->str;
}

void
notification_set_title (Notification* n,
			const gchar*  title)
{
	NotificationPrivate* priv;

	g_assert (IS_NOTIFICATION (n));
	g_assert (title);

	priv = GET_PRIVATE (n);

	if (priv->title)
		g_string_free (priv->title, TRUE);

	priv->title = g_string_new (title);	
}

gchar*
notification_get_body (Notification* n)
{
	NotificationPrivate* priv;

	g_return_val_if_fail (IS_NOTIFICATION (n), NULL);

	priv = GET_PRIVATE (n);

	if (!priv->body)
		return NULL;

	return GET_PRIVATE (n)->body->str;
}

void
notification_set_body (Notification* n,
		       const gchar*  body)
{
	NotificationPrivate* priv;

	g_assert (IS_NOTIFICATION (n));
	g_assert (body);

	priv = GET_PRIVATE (n);

	if (priv->body)
		g_string_free (priv->body, TRUE);

	priv->body = g_string_new (body);
}

// the allowed range for stored values is -1..101, thus a return-value of -2
// indicates an error on behalf of the caller
gint
notification_get_value (Notification* n)
{
	g_return_val_if_fail (IS_NOTIFICATION (n), -2);

	return GET_PRIVATE (n)->value;
}

void
notification_set_value (Notification* n,
			gint          value)
{
	NotificationPrivate* priv;

	g_assert (IS_NOTIFICATION (n));

	priv = GET_PRIVATE (n);

	// don't store any values outside of allowed range -1..101
	if (value < NOTIFICATION_VALUE_MIN_ALLOWED)
	{
		priv->value = NOTIFICATION_VALUE_MIN_ALLOWED;
		return;
	}

	if (value > NOTIFICATION_VALUE_MAX_ALLOWED)
	{
		priv->value = NOTIFICATION_VALUE_MAX_ALLOWED;
		return;
	}

	priv->value = value;
}

gchar*
notification_get_icon_themename (Notification* n)
{
	NotificationPrivate* priv;

	g_return_val_if_fail (IS_NOTIFICATION (n), NULL);

	priv = GET_PRIVATE (n);

	if (!priv->icon_themename)
		return NULL;

	return GET_PRIVATE (n)->icon_themename->str;
}

void
notification_set_icon_themename (Notification* n,
				 const gchar*  icon_themename)
{
	NotificationPrivate* priv;

	g_assert (IS_NOTIFICATION (n));
	g_assert (icon_themename);

	priv = GET_PRIVATE (n);

	if (priv->icon_themename)
		g_string_free (priv->icon_themename, TRUE);

	priv->icon_themename = g_string_new (icon_themename);
}

gchar*
notification_get_icon_filename (Notification* n)
{
	NotificationPrivate* priv;

	g_return_val_if_fail (IS_NOTIFICATION (n), NULL);

	priv = GET_PRIVATE (n);

	if (!priv->icon_filename)
		return NULL;

	return GET_PRIVATE (n)->icon_filename->str;
}

void
notification_set_icon_filename (Notification* n,
				const gchar*  icon_filename)
{
	NotificationPrivate* priv;

	g_assert (IS_NOTIFICATION (n));
	g_assert (icon_filename);

	priv = GET_PRIVATE (n);

	if (priv->icon_filename)
		g_string_free (priv->icon_filename, TRUE);

	priv->icon_filename = g_string_new (icon_filename);
}

GdkPixbuf*
notification_get_icon_pixbuf (Notification* n)
{
	NotificationPrivate* priv;

	g_return_val_if_fail (IS_NOTIFICATION (n), NULL);

	priv = GET_PRIVATE (n);

	if (!priv->icon_pixbuf)
		return NULL;

	return GET_PRIVATE (n)->icon_pixbuf;
}

void
notification_set_icon_pixbuf (Notification*    n,
			      const GdkPixbuf* icon_pixbuf)
{
	NotificationPrivate* priv;

	g_assert (IS_NOTIFICATION (n));
	g_assert (icon_pixbuf);

	priv = GET_PRIVATE (n);

	// free any previous stored pixbuf
	if (priv->icon_pixbuf)
		g_object_unref (priv->icon_pixbuf);

	// create a new/private copy of the supplied pixbuf
	priv->icon_pixbuf = gdk_pixbuf_copy (icon_pixbuf);
}

// a return-value of -1 indicates an error on behalf of the caller, a
// return-value of 0 would indicate that a notification has not been displayed
// yet
gint
notification_get_onscreen_time (Notification* n)
{
	g_return_val_if_fail (IS_NOTIFICATION (n), -1);

	return GET_PRIVATE (n)->onscreen_time;
}

void
notification_set_onscreen_time (Notification* n,
				gint          onscreen_time)
{
	NotificationPrivate* priv;

	g_assert (IS_NOTIFICATION (n));

	priv = GET_PRIVATE (n);

	// see if the caller is really stupid and passes a negative time
	if (onscreen_time < 0)
		return;

	// onscreen-time can only increase not decrease
	if (priv->onscreen_time > onscreen_time)
		return;

	// you made it upto here, congratulations... let's store the new value
	priv->onscreen_time = onscreen_time;
}

gchar*
notification_get_sender_name (Notification* n)
{
	NotificationPrivate* priv;

	g_return_val_if_fail (IS_NOTIFICATION (n), NULL);

	priv = GET_PRIVATE (n);

	if (!priv->sender_name)
		return NULL;

	return GET_PRIVATE (n)->sender_name->str;
}

void
notification_set_sender_name (Notification* n,
			      const gchar*  sender_name)
{
	NotificationPrivate* priv;

	g_assert (IS_NOTIFICATION (n));
	g_assert (sender_name);

	priv = GET_PRIVATE (n);

	if (priv->sender_name)
		g_string_free (priv->sender_name, TRUE);

	priv->sender_name = g_string_new (sender_name);
}

// a return-value of -1 indicates an error on behalf of the caller, PIDs are
// never negative
gint
notification_get_sender_pid (Notification* n)
{
	g_return_val_if_fail (IS_NOTIFICATION (n), -1);

	return GET_PRIVATE (n)->sender_pid;
}

void
notification_set_sender_pid (Notification* n,
			     gint          sender_pid)
{
	g_assert (IS_NOTIFICATION (n));

	// it's hardly possible we'll get a notification from init, but anyway
	if (sender_pid <= 1)
		return;

	GET_PRIVATE (n)->sender_pid = sender_pid;
}

GTimeVal*
notification_get_timestamp (Notification* n)
{
	NotificationPrivate* priv;

	g_return_val_if_fail (IS_NOTIFICATION (n), NULL);

	priv = GET_PRIVATE (n);

	return &priv->timestamp;
}

void
notification_set_timestamp (Notification*   n,
			    const GTimeVal* timestamp)
{
	NotificationPrivate* priv;

	g_assert (IS_NOTIFICATION (n));
	g_assert (timestamp);

	priv = GET_PRIVATE (n);

	// don't store older timestamp over newer one
	if (priv->timestamp.tv_sec > timestamp->tv_sec)
		return;

	if (priv->timestamp.tv_sec == timestamp->tv_sec)
		if (priv->timestamp.tv_usec > timestamp->tv_usec)
			return;

	// new timestamp certainly more current that stored one
	priv->timestamp.tv_sec  = timestamp->tv_sec;
	priv->timestamp.tv_usec = timestamp->tv_usec;
}

gint
notification_get_urgency (Notification* n)
{
	g_return_val_if_fail (IS_NOTIFICATION (n), URGENCY_NONE);

	return GET_PRIVATE (n)->urgency;
}

void
notification_set_urgency (Notification* n,
			  Urgency       urgency)
{
	g_assert (IS_NOTIFICATION (n));

	// don't store any values outside of allowed range -1..101
	if (urgency != URGENCY_LOW    &&
	    urgency != URGENCY_NORMAL &&
	    urgency != URGENCY_HIGH)
		return;

	GET_PRIVATE (n)->urgency = urgency;
}

//------------------------------------------------------------------------------

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
