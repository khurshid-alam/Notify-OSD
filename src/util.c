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
 **    Mirco "MacSlow" Mueller <mirco.mueller@canonical.com>
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

#include <string.h>
#include <glib.h>
#include <pango/pango.h>
#include <cairo.h>

#define CHARACTER_LT_REGEX            "&(lt;|#60;|#x3c;)"
#define CHARACTER_GT_REGEX            "&(gt;|#62;|#x3e;)"
#define CHARACTER_AMP_REGEX           "&(amp;|#38;|#x26;)"
#define CHARACTER_APOS_REGEX          "&apos;"
#define CHARACTER_QUOT_REGEX          "&quot;"

#define TAG_MATCH_REGEX     "<(b|i|u|big|a|img|span|s|sub|small|tt)\\b[^>]*>(.*?)</\\1>|<(img|span|a)[^>]/>|<(img)[^>]*>"
#define TAG_REPLACE_REGEX   "<(b|i|u|big|a|img|span|s|sub|small|tt)\\b[^>]*>|</(b|i|u|big|a|img|span|s|sub|small|tt)>"


static gchar*
strip_html (const gchar *text, const gchar *match_regex, const gchar* replace_regex)
{
	GRegex   *regex;
	gchar    *ret;
	gboolean  match = FALSE;
	GMatchInfo *info = NULL;

	regex = g_regex_new (match_regex, G_REGEX_DOTALL | G_REGEX_OPTIMIZE, 0, NULL);
	match = g_regex_match (regex, text, 0, &info);
	g_regex_unref (regex);

	if (match) {
		regex = g_regex_new (replace_regex, G_REGEX_DOTALL | G_REGEX_OPTIMIZE, 0, NULL);
		ret = g_regex_replace (regex, text, -1, 0, "", 0, NULL);
		g_regex_unref (regex);
	} else {
		ret = g_strdup (text);
	}

	if (info)
		g_match_info_free (info);

	return ret;
}

static gchar*
replace_markup (const gchar *text, const gchar *match_regex, const gchar *replace_text)
{
	GRegex *regex;
	gchar  *ret;

	regex = g_regex_new (match_regex, G_REGEX_DOTALL | G_REGEX_OPTIMIZE, 0, NULL);
	ret = g_regex_replace (regex, text, -1, 0, replace_text, 0, NULL);
	g_regex_unref (regex);

	return ret;
}

gchar*
filter_text (const gchar *text)
{
	gchar *text1;
	gchar *text2;
	gchar *text3;
	gchar *text4;
	gchar *text5;
	gchar *text6;

	text1 = strip_html (text, TAG_MATCH_REGEX, TAG_REPLACE_REGEX);

	text2 = replace_markup (text1, CHARACTER_AMP_REGEX, "&");
	g_free (text1);

	text3 = replace_markup (text2, CHARACTER_LT_REGEX, "<");
	g_free (text2);

	text4 = replace_markup (text3, CHARACTER_GT_REGEX, ">");
	g_free (text3);

	text5 = replace_markup (text4, CHARACTER_APOS_REGEX, "'");
	g_free (text4);

	text6 = replace_markup (text5, CHARACTER_QUOT_REGEX, "\"");
	g_free (text5);

	return text6;
}

cairo_surface_t*
copy_surface (cairo_surface_t* orig)
{
	cairo_surface_t* copy       = NULL;
	guchar*          pixels_src = NULL;
	guchar*          pixels_cpy = NULL;
	cairo_format_t   format;
	gint             width;
	gint             height;
	gint             stride;

	pixels_src = cairo_image_surface_get_data (orig);
	if (!pixels_src)
		return NULL;

	format = cairo_image_surface_get_format (orig);
	width  = cairo_image_surface_get_width (orig);
	height = cairo_image_surface_get_height (orig);
	stride = cairo_image_surface_get_stride (orig);

	pixels_cpy = g_malloc0 (stride * height);
	if (!pixels_cpy)
		return NULL;

	memcpy ((void*) pixels_cpy, (void*) pixels_src, height * stride);

	copy = cairo_image_surface_create_for_data (pixels_cpy,
						    format,
						    width,
						    height,
						    stride);

	return copy;
}
