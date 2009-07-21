/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
**      10        20        30        40        50        60        70        80
**
** notify-osd
**
** stack.c - manages the stack/queue of incoming notifications
**
** Copyright 2009 Canonical Ltd.
**
** Authors:
**    Mirco "MacSlow" Mueller <mirco.mueller@canonical.com>
**    David Barth <david.barth@canonical.com>
**
** Contributor(s):
**    Abhishek Mukherjee <abhishek.mukher.g@gmail.com> (append fixes, rev. 280)
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

#include <assert.h>
#include "dbus.h"
#include <dbus/dbus-glib-lowlevel.h>
#include <glib-object.h>
#include "stack.h"
#include "bubble.h"
#include "apport.h"
#include "dnd.h"
#include "log.h"

G_DEFINE_TYPE (Stack, stack, G_TYPE_OBJECT);

/* fwd declaration */
void close_handler (GObject* n, Stack*  stack);

/*-- internal API ------------------------------------------------------------*/

static void
stack_dispose (GObject* gobject)
{
	/* chain up to the parent class */
	G_OBJECT_CLASS (stack_parent_class)->dispose (gobject);
}

static
void
disconnect_bubble (gpointer data,
	      gpointer user_data)
{
	Bubble* bubble = BUBBLE(data);
	Stack* stack = STACK(user_data);
	g_signal_handlers_disconnect_by_func (G_OBJECT(bubble), G_CALLBACK (close_handler), stack);
}


static void
stack_finalize (GObject* gobject)
{
	if (STACK(gobject)->list != NULL)
		g_list_foreach (STACK(gobject)->list, disconnect_bubble, gobject);
	if (STACK(gobject)->defaults != NULL)
		g_object_unref (STACK(gobject)->defaults);
	if (STACK(gobject)->observer != NULL)
		g_object_unref (STACK(gobject)->observer);

	/* chain up to the parent class */
	G_OBJECT_CLASS (stack_parent_class)->finalize (gobject);
}

static void
stack_init (Stack* self)
{
	/* If you need specific construction properties to complete
	** initialization, delay initialization completion until the
	** property is set. */

	self->list = NULL;
}

static void
stack_get_property (GObject*    gobject,
		    guint       prop,
		    GValue*     value,
		    GParamSpec* spec)
{
	Stack* stack;

	stack = STACK (gobject);

	switch (prop)
	{
		default :
			G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop, spec);
		break;
	}
}

#include "stack-glue.h"

static void
stack_class_init (StackClass* klass)
{
	GObjectClass* gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->dispose      = stack_dispose;
	gobject_class->finalize     = stack_finalize;
	gobject_class->get_property = stack_get_property;

	dbus_g_object_type_install_info (G_TYPE_FROM_CLASS (klass),
					 &dbus_glib_stack_object_info);
}

gint
compare_id (gconstpointer a,
	    gconstpointer b)
{
	gint  result;
	guint id_1;
	guint id_2;

	if (!a || !b)
		return -1;

	if (!IS_BUBBLE (a))
		return -1;

	id_1 = bubble_get_id ((Bubble*) a);
	id_2 = *((guint*) b);

	if (id_1 < id_2)
		result = -1;

	if (id_1 == id_2)
		result = 0;

	if (id_1 > id_2)
		result = 1;

	return result;
}

static gint
compare_append (gconstpointer a,
	        gconstpointer b)
{
	const gchar* str_1;
	const gchar* str_2;
	gint cmp;

	if (!a || !b)
		return -1;

	if (!IS_BUBBLE (a))
		return -1;
	if (!IS_BUBBLE (b))
		return 1;

	if (!bubble_is_append_allowed((Bubble*) a))
		return -1;
	if (!bubble_is_append_allowed((Bubble*) b))
		return 1;

	str_1 = bubble_get_title ((Bubble*) a);
	str_2 = bubble_get_title ((Bubble*) b);

	cmp = g_strcmp0(str_1, str_2);
	if (cmp < 0)
		return -1;
	if (cmp > 0)
		return 1;

	str_1 = bubble_get_sender ((Bubble*) a);
	str_2 = bubble_get_sender ((Bubble*) b);
	return g_strcmp0(str_1, str_2);
}

GList*
find_entry_by_id (Stack* self,
		  guint  id)
{
	GList* entry;

	/* sanity check */
	if (!self)
		return NULL;

	entry = g_list_find_custom (self->list,
				    (gconstpointer) &id,
				    compare_id);
	if (!entry)
		return NULL;

	return entry;
}

static Bubble*
find_bubble_by_id (Stack* self,
		   guint  id)
{
	GList* entry;

	/* sanity check */
	if (!self)
		return NULL;

	entry = g_list_find_custom (self->list,
				    (gconstpointer) &id,
				    compare_id);
	if (!entry)
		return NULL;

	return (Bubble*) entry->data;
}

static Bubble*
find_bubble_for_append(Stack* self,
		       Bubble *bubble)
{
	GList* entry;

	/* sanity check */
	if (!self)
		return NULL;

	entry = g_list_find_custom(self->list,
				   (gconstpointer) bubble,
				   compare_append);

	if (!entry)
		return NULL;

	return (Bubble*) entry->data;
}


static void
stack_purge_old_bubbles (Stack* self)
{
	Bubble* bubble = NULL;
	GList*    list = NULL;

	g_return_if_fail (self != NULL);

	for (list = g_list_first (self->list);
	     list != NULL;)
	{
		bubble = (Bubble*) list->data;
		
		if (! IS_BUBBLE (bubble))
		{
			self->list = g_list_delete_link (self->list, list);
			list = self->list;
		} else if (! bubble_is_visible (bubble) &&
			   (bubble_get_timeout (bubble) == 0))
		{
			self->list = g_list_delete_link (self->list, list);
			list = self->list;
			g_object_unref (bubble);
		} else {
			list = g_list_next (list);
		}
	}
}

static void
_trigger_bubble_redraw (gpointer data,
			gpointer user_data)
{
	Bubble* bubble;

	if (!data)
		return;

	bubble = BUBBLE (data);
	if (!IS_BUBBLE (bubble))
		return;

	bubble_recalc_size (bubble);
	bubble_refresh (bubble);
}

static void
value_changed_handler (Defaults* defaults,
		       Stack*    stack)
{
	if (stack->list != NULL)
		g_list_foreach (stack->list, _trigger_bubble_redraw, NULL);
}

/* this is in dialog.c */
gboolean dialog_check_actions_and_timeout (gchar **actions, gint timeout);

static Bubble *sync_bubble = NULL;

#include "display.c"

/*-- public API --------------------------------------------------------------*/

Stack*
stack_new (Defaults* defaults,
	   Observer* observer)
{
	Stack* this;

	if (!defaults || !observer)
		return NULL;

	this = g_object_new (STACK_TYPE, NULL);
	if (!this)
		return NULL;

	this->defaults = defaults;
	this->observer = observer;
	this->list     = NULL;
	this->next_id  = 1;

	/* hook up handler to act on changes of defaults/settings */
	g_signal_connect (G_OBJECT (defaults),
			  "value-changed",
			  G_CALLBACK (value_changed_handler),
			  this);

	return this;
}

void
stack_del (Stack* self)
{
	if (!self)
		return;

	g_object_unref (self);
}

void
close_handler (GObject *n,
	       Stack*  stack)
{
	/* TODO: use weak-refs to dispose the bubble.
	   Meanwhile, do nothing here to avoid segfaults
	   and rely on the stack_purge_old_bubbles() call
	   later on in the thread.
	*/

	if (n != NULL)
	{
		if (IS_BUBBLE (n)
		    && bubble_is_synchronous (BUBBLE (n)))
		{
			g_object_unref (n);
			sync_bubble = NULL;
		} else {
			/* Fix for a tricky race condition
			   where a bubble fades out in sync
			   with a synchronous bubble: the symc.
			   one is still considered visible while
			   the normal one has triggered this signal.
			   This ensures the display slot of the
			   sync. bubble is recycled, and no gap is
			   left on the screen */
			sync_bubble = NULL;
		}

		stack_layout (stack);
	}

	return;
}

/* since notification-ids are unsigned integers the first id index is 1, 0 is
** used to indicate an error */
guint
stack_push_bubble (Stack*  self,
		   Bubble* bubble)
{
	guint notification_id = -1;

	/* sanity check */
	if (!self || !IS_BUBBLE (bubble))
		return -1;

	/* check if this is just an update */
	if (find_bubble_by_id (self, bubble_get_id (bubble)))
	{
		bubble_start_timer (bubble);
		bubble_refresh (bubble);

		/* resync the synchronous bubble if it's at the top */
		if (sync_bubble != NULL
		    && bubble_is_visible (sync_bubble))
			if (stack_is_at_top_corner (self, sync_bubble))
				bubble_sync_with (sync_bubble, bubble);

		return bubble_get_id (bubble);
	}

	/* add bubble/id to stack */
	notification_id = self->next_id++;
	bubble_set_id (bubble, notification_id);
	self->list = g_list_append (self->list, (gpointer) bubble);

	g_signal_connect (G_OBJECT (bubble),
			  "timed-out",
			  G_CALLBACK (close_handler),
			  self);

	/* return current/new id to caller (usually our DBus-dispatcher) */
	return notification_id;
}

void
stack_pop_bubble_by_id (Stack* self,
			guint  id)
{
	Bubble* bubble;

	/* sanity check */
	if (!self)
		return;

	/* find bubble corresponding to id */
	bubble = find_bubble_by_id (self, id);
	if (!bubble)
		return;

	/* close/hide/fade-out bubble */
	bubble_hide (bubble);

	/* find entry in list corresponding to id and remove it */
	self->list = g_list_delete_link (self->list,
					 find_entry_by_id (self, id));
	g_object_unref (bubble);

	/* immediately refresh the layout of the stack */
	stack_layout (self);
}

static GdkPixbuf*
process_dbus_icon_data (GValue *data)
{
	GType dbus_icon_t;
	GArray *pixels;
	int width, height, rowstride, bits_per_sample, n_channels, size;
	gboolean has_alpha;
	guchar *copy;
	GdkPixbuf *pixbuf = NULL;

	g_return_val_if_fail (data != NULL, NULL);

	dbus_icon_t = dbus_g_type_get_struct ("GValueArray",
					      G_TYPE_INT,
					      G_TYPE_INT,
					      G_TYPE_INT,
					      G_TYPE_BOOLEAN,
					      G_TYPE_INT,
					      G_TYPE_INT,
					      dbus_g_type_get_collection ("GArray",
									  G_TYPE_UCHAR),
					      G_TYPE_INVALID);
	
	if (G_VALUE_HOLDS (data, dbus_icon_t))
	{
		dbus_g_type_struct_get (data,
					0, &width,
					1, &height,
					2, &rowstride,
					3, &has_alpha,
					4, &bits_per_sample,
					5, &n_channels,
					6, &pixels,
					G_MAXUINT);

		size = (height - 1) * rowstride + width *
			((n_channels * bits_per_sample + 7) / 8);
		copy = (guchar *) g_memdup (pixels->data, size);
		pixbuf = gdk_pixbuf_new_from_data(copy, GDK_COLORSPACE_RGB,
						  has_alpha,
						  bits_per_sample,
						  width, height,
						  rowstride,
						  (GdkPixbufDestroyNotify)g_free,
						  NULL);
	}

	return pixbuf;
}

gboolean
stack_notify_handler (Stack*                 self,
		      const gchar*           app_name,
		      guint                  id,
		      const gchar*           icon,
		      const gchar*           summary,
		      const gchar*           body,
		      gchar**                actions,
		      GHashTable*            hints,
		      gint                   timeout,
		      DBusGMethodInvocation* context)
{
	Bubble*    bubble     = NULL;
	Bubble*    app_bubble = NULL;
	GValue*      data     = NULL;
	GValue*    compat     = NULL;
	GdkPixbuf* pixbuf     = NULL;
	gboolean   new_bubble = FALSE;

        /* check if a bubble exists with same id */
	bubble = find_bubble_by_id (self, id);
	if (bubble == NULL)
	{
		gchar *sender;
		new_bubble = TRUE;
		bubble = bubble_new (self->defaults);
		sender = dbus_g_method_get_sender (context);
		bubble_set_sender (bubble, sender);
		g_free (sender);
	}

	if (new_bubble && hints)
	{
		data   = (GValue*) g_hash_table_lookup (hints, "x-canonical-append");
		compat = (GValue*) g_hash_table_lookup (hints, "append");
		if (G_VALUE_HOLDS_STRING (data) || G_VALUE_HOLDS_STRING (compat))
			bubble_set_append (bubble, TRUE);
		else
			bubble_set_append (bubble, FALSE);
	}

	if (summary)
		bubble_set_title (bubble, summary);
	if (body)
		bubble_set_message_body (bubble, body);


	if (new_bubble && bubble_is_append_allowed(bubble)) {
		app_bubble = find_bubble_for_append(self, bubble);

		/* Appending to an old bubble */
		if (app_bubble != NULL) {
			g_object_unref(bubble);
			bubble = app_bubble;
			if (body) {
				bubble_append_message_body (bubble, "\n");
				bubble_append_message_body (bubble, body);
			}
		}
	}

	if (hints)
	{
		data   = (GValue*) g_hash_table_lookup (hints, "x-canonical-private-synchronous");
		compat = (GValue*) g_hash_table_lookup (hints, "synchronous");
		if (G_VALUE_HOLDS_STRING (data) || G_VALUE_HOLDS_STRING (compat))
		{
			if (sync_bubble != NULL
			    && IS_BUBBLE (sync_bubble))
			{
				g_object_unref (bubble);
				bubble = sync_bubble;
			}

			if (G_VALUE_HOLDS_STRING (data))
				bubble_set_synchronous (bubble, g_value_get_string (data));

			if (G_VALUE_HOLDS_STRING (compat))
				bubble_set_synchronous (bubble, g_value_get_string (compat));
		}
	}

	if (hints)
	{
		data = (GValue*) g_hash_table_lookup (hints, "value");
		if (G_VALUE_HOLDS_INT (data))
			bubble_set_value (bubble, g_value_get_int (data));
	}

	if (hints)
	{
		data = (GValue*) g_hash_table_lookup (hints, "urgency");
		if (G_VALUE_HOLDS_UCHAR (data))
			bubble_set_urgency (bubble,
					   g_value_get_uchar (data));
		/* Note: urgency was defined as an enum: LOW, NORMAL, CRITICAL
		   So, 2 means CRITICAL
		*/
	}

	if (hints)
	{
		data   = (GValue*) g_hash_table_lookup (hints, "x-canonical-private-icon-only");
		compat = (GValue*) g_hash_table_lookup (hints, "icon-only");
		if (G_VALUE_HOLDS_STRING (data) || G_VALUE_HOLDS_STRING (compat))
			bubble_set_icon_only (bubble, TRUE);
		else
			bubble_set_icon_only (bubble, FALSE);
	}

	if (hints)
	{
		if ((data = (GValue*) g_hash_table_lookup (hints, "image_data")))
		{
			g_debug("Using image_data hint\n");
			pixbuf = process_dbus_icon_data (data);
			bubble_set_icon_from_pixbuf (bubble, pixbuf);
		}
		else if ((data = (GValue*) g_hash_table_lookup (hints, "image_path")))
		{
			g_debug("Using image_path hint\n");
			if (G_VALUE_HOLDS_STRING (data))
				bubble_set_icon (bubble, g_value_get_string(data));
			else
				g_warning ("image_path hint is not a string\n");
		}
		else if (icon && *icon != '\0')
		{
			g_debug("Using icon parameter\n");
			bubble_set_icon (bubble, icon);
		}
		else if ((data = (GValue*) g_hash_table_lookup (hints, "icon_data")))
		{
			g_debug("Using deprecated icon_data hint\n");
			pixbuf = process_dbus_icon_data (data);
			bubble_set_icon_from_pixbuf (bubble, pixbuf);
		}
	}

	log_bubble_debug (bubble, app_name,
			  (*icon == '\0' && data != NULL) ?
			  "..." : icon);

	gboolean turn_into_dialog = dialog_check_actions_and_timeout (actions, timeout);

	if (turn_into_dialog)
		/* || bubble_is_urgent (bubble)) */
	{
		/* TODO: apport_report (app_name, summary, actions, timeout); */

		GObject *dialog = G_OBJECT (
			bubble_show_dialog (bubble, app_name, actions));
		g_signal_connect (dialog,
				  "destroy-event",
				  G_CALLBACK (close_handler),
				  self);

		dbus_g_method_return (context, bubble_get_id (bubble));

		return TRUE;
	}

	bubble_determine_layout (bubble);

	bubble_recalc_size (bubble);

	if (bubble_is_synchronous (bubble))
	{
		stack_display_sync_bubble (self, bubble);

	} else {
		stack_push_bubble (self, bubble);

		if (! new_bubble && bubble_is_append_allowed (bubble))
			log_bubble (bubble, app_name, "appended");
		else if (! new_bubble)
			log_bubble (bubble, app_name, "replaced");
		else
			log_bubble (bubble, app_name, "");

		/* make sure the sync. bubble is positioned correctly
		   even for the append case
		*/
		if (sync_bubble != NULL
		    && bubble_is_visible (sync_bubble))
			stack_display_position_sync_bubble (self, sync_bubble);

		/* update the layout of the stack;
		 * this will also open the new bubble */
		stack_layout (self);
	}

	dbus_g_method_return (context, bubble_get_id (bubble));

	return TRUE;
}

gboolean
stack_close_notification_handler (Stack*   self,
				  guint    id,
				  GError** error)
{
	Bubble *bubble = find_bubble_by_id (self, id);

	/* exit but pretend it's ok, for applications
	   that call us after an action button was clicked */
	if (bubble == NULL)
		return TRUE;

	dbus_send_close_signal (bubble_get_sender (bubble),
				bubble_get_id (bubble),
				3);
	bubble_hide (bubble);
	g_object_unref (bubble);

	stack_layout (self);

	return TRUE;
}

gboolean
stack_get_capabilities (Stack*   self,
			gchar*** out_caps)
{
	*out_caps = g_malloc0 (13 * sizeof(char *));

	(*out_caps)[0]  = g_strdup ("body");
	(*out_caps)[1]  = g_strdup ("body-markup");
	(*out_caps)[2]  = g_strdup ("icon-static");
	(*out_caps)[3]  = g_strdup ("image/svg+xml");
	(*out_caps)[4]  = g_strdup ("x-canonical-private-synchronous");
	(*out_caps)[5]  = g_strdup ("x-canonical-append");
	(*out_caps)[6]  = g_strdup ("x-canonical-private-icon-only");
	(*out_caps)[7]  = g_strdup ("x-canonical-truncation");

	/* a temp. compatibility-check for the transition time to allow apps a
	** grace-period to catch up with the capability- and hint-name-changes
	** introduced with notify-osd rev. 224 */
	(*out_caps)[8]  = g_strdup ("private-synchronous");
	(*out_caps)[9]  = g_strdup ("append");
	(*out_caps)[10] = g_strdup ("private-icon-only");
	(*out_caps)[11] = g_strdup ("truncation");

	(*out_caps)[12] = NULL;

	return TRUE;
}

gboolean
stack_get_server_information (Stack*  self,
			      gchar** out_name,
			      gchar** out_vendor,
			      gchar** out_version,
			      gchar** out_spec_ver)
{
	*out_name     = g_strdup ("notify-osd");
	*out_vendor   = g_strdup ("Canonical Ltd");
	*out_version  = g_strdup ("1.0");
	*out_spec_ver = g_strdup ("0.10");

	return TRUE;
}
