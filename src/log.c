/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
**      10        20        30        40        50        60        70        80
**
** notify-osd
**
** log.h - log utils
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

#include "config.h"
#include <sys/time.h>
#include <time.h>
#include <stdio.h>

#include "bubble.h"

static FILE *logfile;

void
log_init (void)
{
	const char *homedir = g_getenv ("HOME");
	if (!homedir)
		homedir = g_get_home_dir ();

	char *filename =
		g_strdup_printf ("%s/.cache/notify-osd.log", homedir);

	logfile = fopen (filename, "w");

	g_free (filename);
}

static
char*
log_create_timestamp (void)
{
	struct timeval tv;
	struct tm     *tm;

	/* FIXME: deal with tz offsets */
	gettimeofday (&tv, NULL);
	tm = localtime (&tv.tv_sec);

	return g_strdup_printf ("%.4d-%.2d-%.2dT%.2d:%.2d:%.2d%.1s%.2d:%.2d",
				tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
				tm->tm_hour, tm->tm_min, tm->tm_sec,
				"-", 0, 0);
}

void
log_bubble (Bubble *bubble, const char *app_name, const char *option)
{
	g_return_if_fail (IS_BUBBLE (bubble));

	char *ts = log_create_timestamp ();

	if (option)
		fprintf (logfile, "[%s, %s %s] %s\n",
			 ts, app_name, option,
			 bubble_get_title (bubble));
	else
		fprintf (logfile, "[%s, %s] %s\n",
			 ts, app_name,
			 bubble_get_title (bubble));

	fprintf (logfile, "%s\n\n",
		 bubble_get_message_body (bubble));

	fflush (logfile);

	g_free (ts);
}
