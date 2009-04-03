/*******************************************************************************
 **3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
 **      10        20        30        40        50        60        70        80
 **
 ** notify-osd
 **
 ** util.c - all sorts of helper functions
 **
 ** Copyright 2009 Canonical Ltd.
 **
 ** Authors:
 **    Cody Russell <cody.russell@canonical.com>
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

#include <glib.h>

gchar*
filter_text (const gchar *text)
{
	gchar *ret;
	gchar *text1;
	GRegex *regex;

	regex = g_regex_new ("&(?!(amp;|lt;|gt;|quot;|apos;))", 0, 0, NULL);
	text1 = g_regex_replace (regex, text, -1, 0, "&amp;", 0, NULL);
	g_regex_unref (regex);

	regex = g_regex_new ("<(.|\n)*?>", 0, 0, NULL);
	ret = g_regex_replace (regex, text1, -1, 0, "", 0, NULL);
	g_regex_unref (regex);

	g_free (text1);

	return ret;
}

