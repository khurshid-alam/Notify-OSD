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
	PROP_RECEPTION_TIMESTAMP,
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
	GTimeVal   reception_timestamp; // timestamp of  reception
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
		priv->body;
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
	G_OBJECT_CLASS (defaults_parent_class)->dispose (gobject);
}

static void
notification_finalize (GObject* gobject)
{
	// gee, I wish I knew the difference between foobar_dispose() and
	// foobar_finalize()

	// chain up to the parent class
	G_OBJECT_CLASS (timings_parent_class)->finalize (gobject);
}

static void
notification_init (Notification* n)
{
	NotificationPrivate* priv;

	// sanity checks
	g_assert (n);
	priv = GET_PRIVATE (n);
	g_assert (priv);

	priv->id                          = -1;
	priv->title                       = NULL;
	priv->body                        = NULL;
	priv->value                       = -2;
	priv->icon_themename              = NULL;
	priv->icon_filename               = NULL;
	priv->icon_pixbuf                 = NULL;
	priv->onscreen_time               = 0;
	priv->sender_name                 = NULL;
	priv->sender_pid                  = -1;
	priv->reception_timestamp.tv_sec  = 0;
	priv->reception_timestamp.tv_usec = 0;
	priv->urgency                     = NOTIFICATION_URGENCY_NONE;
}

static void
notification_get_property (GObject*    gobject,
			   guint       prop,
			   GValue*     value,
			   GParamSpec* spec)
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

	switch (prop)
	{
		case PROP_ID:
			g_value_set_int (value, priv->id);
		break;

		case PROP_TITLE:
			g_value_set_string (value, priv->title->str);
		break;

		case PROP_BODY:
			g_value_set_string (value, priv->body->str);
		break;

		case PROP_VALUE:
			g_value_set_int (value, priv->value);
		break;

		case PROP_ICON_THEMENAME:
			g_value_set_string (value, priv->icon_themename->str);
		break;

		case PROP_ICON_FILENAME:
			g_value_set_string (value, priv->icon_filename->str);
		break;

		case PROP_ICON_PIXBUF:
			// GdkPixbuf*
		break;

		case PROP_ONSCREEN_TIME:
			g_value_set_int (value, priv->onscreen_time);
		break;

		case PROP_SENDER_NAME:
			g_value_set_string (value, priv->sender_name->str);
		break;

		case PROP_SENDER_PID:
			g_value_set_int (value, priv->sender_pid);
		break;

		case PROP_RECEPTION_TIMESTAMP:
		break;

		case PROP_URGENCY:
			g_value_set_int (value, priv->urgency);
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
	Notification*        n;
	NotificationPrivate* priv;

	// sanity checks
	g_assert (gobject);
	n = NOTIFICATION (gobject);
	g_assert (n);
	g_assert (IS_NOTIFICATION (n));
	priv = GET_PRIVATE (n);
	g_assert (priv);

	switch (prop)
	{
		case PROP_ID:
			priv->id = g_value_get_int (value);
		break;

		case PROP_TITLE:
			if (priv->title != NULL)
			{
				g_string_free (priv->title, TRUE);
				priv->title = NULL
			}

			priv->title = g_string_new (g_value_get_string (value));
		break;

		case PROP_BODY:
			if (priv->body != NULL)
			{
				g_string_free (priv->body, TRUE);
				priv->body = NULL
			}

			priv->body = g_string_new (g_value_get_string (value));
		break;

		case PROP_VALUE:
			priv->value = g_value_get_int (value);
		break;

		case PROP_ICON_THEMENAME:
			if (priv->icon_themename != NULL)
			{
				g_string_free (priv->icon_themename, TRUE);
				priv->icon_themename = NULL
			}

			priv->icon_themename = g_string_new (
						g_value_get_string (value));
		break;

		case PROP_ICON_FILENAME:
			if (priv->icon_filename != NULL)
			{
				g_string_free (priv->icon_filename, TRUE);
				priv->icon_filename = NULL
			}

			priv->icon_filename = g_string_new (
						g_value_get_string (value));
		break;

		case PROP_ICON_PIXBUF:
			// GdkPixbuf*
		break;

		case PROP_ONSCREEN_TIME:
			priv->onscreen_time = g_value_get_int (value);
		break;

		case PROP_SENDER_NAME:
			if (priv->sender_name != NULL)
			{
				g_string_free (priv->sender_name, TRUE);
				priv->sender_name = NULL
			}

			priv->sender_name = g_string_new (
						g_value_get_string (value));
		break;

		case PROP_SENDER_PID:
			priv->sender_pid = g_value_get_int (value);
		break;

		case PROP_RECEPTION_TIMESTAMP:
			// GTimeVal
		break;

		case PROP_URGENCY:
			priv->urgency = g_value_get_int (value);
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

}

//-- public functions ----------------------------------------------------------

Notification*
notification_new ()
{
}

gint
notification_get_id (Notification* n)
{
}

void
notification_set_id (Notification* n,
		     gint          id)
{
}

gchar*
notification_get_title (Notification* n)
{
}

void
notification_set_title (Notification* n,
			gchar*        title)
{
}

gchar*
notification_get_body (Notification* n)
{
}

void
notification_set_body (Notification* n,
		       gchar*        body)
{
}

gint
notification_get_value (Notification* n)
{
}

void
notification_set_value (Notification* n,
			gint          value)
{
}

gchar*
notification_get_icon_themename (Notification* n)
{
}

void
notification_set_icon_themename (Notification* n,
				 gchar*        icon_themename)
{
}

gchar*
notification_get_icon_filename (Notification* n)
{
}

void
notification_set_icon_filename (Notification* n,
				gchar*        icon_filename)
{
}

GdkPixbuf*
notification_get_icon_pixbuf (Notification* n)
{
}

void
notification_set_icon_pixbuf (Notification* n,
			      GdkPixbuf*    icon_pixbuf)
{
}

gint
notification_get_onscreen_time (Notification* n)
{
}

void
notification_set_onscreen_time (Notification* n,
				gint          onscreen_time)
{
}

gchar*
notification_get_sender_name (Notification* n)
{
}

void
notification_set_sender_name (Notification* n,
			      gchar*        sender_name)
{
}

gint
notification_get_sender_pid (Notification* n)
{
}

void
notification_set_sender_pid (Notification* n,
			     gint          sender_pid)
{
}

GTimeVal*
notification_get_reception_timestamp (Notification* n)
{
}

void
notification_set_reception_timestamp (Notification* n,
				      GTimeVal*     reception_timestamp)
{
}

gint
notification_get_urgency (Notification* n)
{
}

void
notification_set_urgency (Notification* n,
			  Urgency       urgency)
{
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
		g_string_free (n->priv->title, TRUE);

	if (n->priv->body)
		g_string_free (n->priv->body, TRUE);

	if (n->priv->icon_themename)
		g_string_free (n->priv->icon_themename, TRUE);

	if (n->priv->icon_filename)
		g_string_free (n->priv->icon_filename, TRUE);

	if (n->priv->icon_pixbuf)
		g_object_unref ((gpointer) n->priv->icon_pixbuf);

	if (n->priv->sender_name)
		g_string_free (n->priv->sender_name, TRUE);

	// get rid of the main allocated structs
	g_free ((gpointer) n->priv);
	g_free ((gpointer) n);
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

	if (n->priv->reception_timestamp.tv_sec == reception_timestamp->tv_sec)
		if (n->priv->reception_timestamp.tv_usec >
		    reception_timestamp->tv_usec)
			return;

	// new timestamp certainly more current that stored one
	n->priv->reception_timestamp.tv_sec  = reception_timestamp->tv_sec;
	n->priv->reception_timestamp.tv_usec = reception_timestamp->tv_usec;
}

// a return-value of -1 indicates an error on behalf of the caller, urgency can
// only be low = 0, normal = 1 or high = 2
gint
notification_get_urgency (notification_t* n)
{
	// sanity checks
	if (!n)
		return -1;

	if (!n->priv)
		return -1;

	return n->priv->urgency;
}

void
notification_set_urgency (notification_t* n,
			  Urgency         urgency)
{
	// sanity checks
	if (!n)
		return;

	if (!n->priv)
		return;

	if (urgency != URGENCY_LOW    &&
	    urgency != URGENCY_NORMAL &&
	    urgency != URGENCY_HIGH)
		return;

	n->priv->urgency = urgency;
}

