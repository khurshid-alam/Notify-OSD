/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
**      10        20        30        40        50        60        70        80
**
** Codename "alsdorf"
**
** test-text-filtering.c - unit-tests for text filtering
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

#include "util.h"

typedef struct {
	const gchar *before;
	const gchar *expected;
} TextComparisons;

static void
test_text_filter ()
{
	static const TextComparisons tests[] = {
		{ "<a href=\"http://www.ubuntu.com/\">Ubuntu</a>", "Ubuntu"           },
		{ "Peace &amp; Love",                              "Peace &amp; Love" },
		{ "War & Peace",                                   "War &amp; Peace"  },
		{ "7 > 3",                                         "7 > 3"            },
		{ "7 &gt; 3",                                      "7 &gt; 3"         },
		{ NULL, NULL }
	};

	for (int i = 0; tests[i].before != NULL; i++) {
		char *filtered = filter_text (tests[i].before);
		g_assert_cmpstr (filtered, ==, tests[i].expected);
		g_free (filtered);
	}
}

GTestSuite *
test_filtering_create_test_suite (void)
{
	GTestSuite *ts = NULL;

	ts = g_test_create_suite ("text-filter");

#define TC(x) g_test_create_case(#x, 0, NULL, NULL, x, NULL)

	g_test_suite_add(ts, TC(test_text_filter));

	return ts;
}
