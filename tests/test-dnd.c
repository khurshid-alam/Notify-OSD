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

GTestSuite *
test_dnd_create_test_suite (void)
{
	GTestSuite *ts = NULL;
	GTestCase  *tc = NULL;

	ts = g_test_create_suite ("dnd");
	tc = g_test_create_case ("detect screensaver",
				 0,
				 NULL,
				 NULL,
				 test_dnd_screensaver,
				 NULL);
	g_test_suite_add (ts, tc);

	return ts;
}
