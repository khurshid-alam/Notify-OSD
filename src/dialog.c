/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
**      10        20        30        40        50        60        70        80
**
** notify-osd
**
** dialog.c - fallback to display when a notification is not spec-compliant
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

/* Note: this file is actually #include'd as a part of bubble.c
   but kept separate because this is not really a true "bubble".
 */

static void
handle_close (GtkWidget *dialog,
	      guint response_id,
	      gpointer user_data)
{
	Bubble *bubble = g_object_get_data (G_OBJECT (dialog),
					   "_bubble");

	GET_PRIVATE (bubble)->visible = FALSE;

	dbus_send_close_signal (bubble_get_sender (bubble),
				bubble_get_id (bubble),
				2);

	gtk_widget_destroy (GTK_WIDGET(dialog));
}

static void
handle_response (GtkWidget *button,
		 GdkEventButton *event,
		 GtkDialog *dialog)
{
	gchar *action = g_object_get_data (G_OBJECT (button),
					   "_libnotify_action");
	Bubble *bubble = g_object_get_data (G_OBJECT (dialog),
					   "_bubble");

	GET_PRIVATE (bubble)->visible = FALSE;

	/* send a "click" signal... <sigh> */
	dbus_send_action_signal (bubble_get_sender (bubble),
				 bubble_get_id (bubble),
				 action);

	dbus_send_close_signal (bubble_get_sender (bubble),
				bubble_get_id (bubble),
				3);

	gtk_widget_destroy (GTK_WIDGET(dialog));
}


static void
add_pathological_action_buttons (GtkWidget *dialog,
				 gchar **actions)
{
	int i;

	for (i = 0; actions[i] != NULL; i += 2)
	{
		gchar *label = actions[i + 1];

		if (label == NULL)
		{
			g_warning ("missing label for action \"%s\""
				   "; discarding the action",
				   actions[i]);
			break;
		}

		if (g_strcmp0 (actions[i], "default") == 0)
			break;

		GtkWidget *button =
			gtk_dialog_add_button (GTK_DIALOG (dialog),
					       label,
					       i/2);
		g_object_set_data_full (G_OBJECT (button),
					"_libnotify_action",
					g_strdup (actions[i]),
					g_free);
		g_signal_connect (G_OBJECT (button),
				  "button-release-event",
				  G_CALLBACK (handle_response),
				  dialog);
	}
}

GObject*
bubble_show_dialog (Bubble *bubble,
		    const char *app_name,
		    gchar **actions)
{
	GtkWidget*     dialog;
	GtkWidget*     hbox;
	GtkWidget*     vbox;
	GtkWidget*     title;
	GtkWidget*     body;
	GtkWidget*     image;
	Defaults*      d = bubble->defaults;
	gchar*         esc;
	gchar*         txt;
	guint          gap = EM2PIXELS (defaults_get_margin_size (d), d);
	BubblePrivate* priv;

	dialog = gtk_dialog_new ();

	hbox = g_object_new (GTK_TYPE_HBOX,
			     "spacing", gap,
			     NULL);

	priv = GET_PRIVATE (bubble);

	image = gtk_image_new_from_pixbuf (bubble_get_icon_pixbuf (bubble));
	gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);
	gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);

	vbox = g_object_new (GTK_TYPE_VBOX,
			     NULL);

	title = gtk_label_new (NULL);
	esc = g_markup_escape_text (priv->title->str, -1);
	txt = g_strdup_printf ("<b>%s</b>", esc);
	gtk_label_set_markup (GTK_LABEL (title), txt);
	gtk_label_set_line_wrap (GTK_LABEL (title), TRUE);
	g_free (esc); g_free (txt);

	body = gtk_label_new (NULL);
	gtk_label_set_markup (GTK_LABEL (body), priv->message_body->str);
	gtk_label_set_line_wrap (GTK_LABEL (body), TRUE);

	gtk_misc_set_alignment (GTK_MISC (title), 0.0, 0.0);
	gtk_box_pack_start (GTK_BOX (vbox), title, TRUE, TRUE, 0);

	gtk_misc_set_alignment (GTK_MISC (body), 0.0, 0.0);
	gtk_box_pack_start (GTK_BOX (vbox), body, TRUE, TRUE, 0);

	gtk_container_add (GTK_CONTAINER (hbox), vbox);

	gtk_container_add (GTK_CONTAINER (
				   gtk_dialog_get_content_area (
					   GTK_DIALOG (dialog))),
			   hbox);

	gtk_container_set_border_width (GTK_CONTAINER (dialog), 2);

	gtk_window_set_default_size (GTK_WINDOW (dialog),
				     EM2PIXELS (defaults_get_bubble_width (d) * 1.2f, d),
				     -1);
	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
	gtk_window_set_title (GTK_WINDOW (dialog), app_name);
	gtk_window_set_urgency_hint (GTK_WINDOW (dialog), TRUE);
				     /* bubble_is_urgent (bubble)); */

	/* is it a bad notification with actions? */
	if (actions[0] != NULL)
		add_pathological_action_buttons (dialog, actions);

	GtkButton *cancel = GTK_BUTTON (
		gtk_dialog_add_button (GTK_DIALOG (dialog),
				       GTK_STOCK_CANCEL,
				       GTK_RESPONSE_CANCEL));
	g_signal_connect_swapped (G_OBJECT (cancel),
				  "button-release-event",
				  G_CALLBACK (handle_close),
				  dialog);

	g_signal_connect (G_OBJECT (dialog),
			  "response",
			  G_CALLBACK (handle_close),
			  dialog);
		
		
	GtkButton *ok = GTK_BUTTON (
		gtk_dialog_add_button (GTK_DIALOG (dialog),
				       GTK_STOCK_OK,
				       GTK_RESPONSE_OK));
	g_object_set_data_full (G_OBJECT (ok),
				"_libnotify_action",
				g_strdup ("default"),
				g_free);
	g_signal_connect (G_OBJECT (ok),
			  "button-release-event",
			  G_CALLBACK (handle_response),
			  dialog);

	g_object_set_data (G_OBJECT (dialog),
			   "_bubble",
			   bubble);

	g_signal_connect (G_OBJECT (dialog),
			  "button-release-event",
			  G_CALLBACK (handle_response),
			  dialog);

	gtk_window_set_focus (GTK_WINDOW (dialog),
			      NULL);
	gtk_window_set_default (GTK_WINDOW (dialog),
				NULL);

	gtk_widget_show_all (dialog);

	/* pretend its visible, so that the purge algorithm
	   does not try to unreference the bubble */
	priv->visible = TRUE;

	return G_OBJECT (dialog);
}
