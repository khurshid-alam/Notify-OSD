/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
**      10        20        30        40        50        60        70        80
**
** project:
**    alsdorf
**
** file:
**    dnd.c
**
** author(s):
**    Mirco "MacSlow" Mueller <mirco.mueller@canonical.com>
**    David Barth <david.barth@canonical.com>
**
** copyright (C) Canonical, 2009
**
*******************************************************************************/

#include <glib-object.h>

static gboolean
is_screensaver_active ()
{
	return FALSE;
}

static gboolean
is_screen_locked ()
{
	return FALSE;
}

static gboolean
is_online_presence_dnd ()
{
	return FALSE;
}

/* Tries to determine whether the user is in "do not disturb" mode */
gboolean
dnd_dont_disturb_user (void)
{
	return (is_online_presence_dnd()
		|| is_screensaver_active()
		|| is_screen_locked());
}
