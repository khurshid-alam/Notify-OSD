/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
**      10        20        30        40        50        60        70        80
**
** notify-osd
**
** test-dnd.c - test the do-not-disturb mode code
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

#include <unistd.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "dnd.h"

#define TEST_DBUS_NAME "org.freedesktop.Notificationstest"

static
void
test_dnd_screensaver (void)
{
	gboolean test = dnd_is_screensaver_inhibited();

	if (test)
		g_debug ("screensaver is inhibited");

	test = dnd_is_screensaver_active();

	if (test)
		g_debug ("screensaver is active");
}

static
gboolean
check_fullscreen (GMainLoop *loop)
{
	g_assert (dnd_has_one_fullscreen_window());
	g_main_loop_quit (loop);
	return FALSE;
}

static
gboolean
check_no_fullscreen (GMainLoop *loop)
{
	g_assert (!dnd_has_one_fullscreen_window());
	g_main_loop_quit (loop);
	return FALSE;
}

static
void
test_dnd_fullscreen (void)
{
	g_assert (!dnd_has_one_fullscreen_window());

	GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_fullscreen (GTK_WINDOW (window));
	gtk_widget_show_now (window);

	GMainLoop* loop = g_main_loop_new (NULL, FALSE);
	g_timeout_add (2000, (GSourceFunc) check_fullscreen, loop);
	g_main_loop_run (loop);

	gtk_widget_destroy (window);
	g_timeout_add (2000, (GSourceFunc) check_no_fullscreen, loop);
	g_main_loop_run (loop);
}

GTestSuite *
test_dnd_create_test_suite (void)
{
	GTestSuite *ts = NULL;

	ts = g_test_create_suite ("dnd");
#define TC(x) g_test_create_case(#x, 0, NULL, NULL, x, NULL)
	g_test_suite_add (ts, TC(test_dnd_screensaver));
	g_test_suite_add (ts, TC(test_dnd_fullscreen));

	return ts;
}
