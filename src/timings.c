////////////////////////////////////////////////////////////////////////////////
//3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
//      10        20        30        40        50        60        70        80
//
// notify-osd
//
// timings.c - timings object handling duration and max. on-screen time
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

#include <math.h>

#include "timings.h"

struct _timings_private_t
{
	GTimer*     on_screen_timer;
	GTimer*     duration_timer;
	GTimer*     paused_timer;
	guint       timeout_id;
	guint       max_timeout_id;
	guint       scheduled_duration; // value interpreted as milliseconds
	guint       max_duration;       // value interpreted as milliseconds
	gboolean    is_paused;
	GSourceFunc close_cb;
};

guint
_ms_elapsed (GTimer* timer)
{
	gulong  microseconds;
	gulong  milliseconds;
	gdouble duration;
	gdouble seconds;

	// sanity check
	g_assert (timer != NULL);

	// get elapsed time
	duration = g_timer_elapsed (timer, &microseconds);

	// convert result to milliseconds ...
	milliseconds = microseconds / 1000;
	modf (duration, &seconds);

	// ... and return it
	return seconds * 1000 + milliseconds;
}

timings_t*
timings_new (guint       scheduled_duration,
	     guint       max_duration,
	     GSourceFunc close_cb,
	     GSourceFunc force_close_cb)
{
	timings_private_t* priv = NULL;
	timings_t*         t    = NULL;

	// sanity checks
	g_assert (scheduled_duration <= max_duration);
	g_assert (close_cb);
	g_assert (force_close_cb);

	// create private struct
	priv = g_new0 (timings_private_t, 1);
	if (!priv)
		return NULL;

	// create public struct
	t = g_new0 (timings_t, 1);
	if (!t)
	{
		g_free ((gpointer) priv);
		return NULL;
	}

	// fill private struct
	priv->on_screen_timer    = g_timer_new ();
	priv->duration_timer     = g_timer_new ();
	priv->paused_timer       = g_timer_new ();
	g_timer_stop (priv->paused_timer);
	priv->timeout_id         = g_timeout_add (scheduled_duration,
						  close_cb,
						  NULL);
	priv->max_timeout_id     = g_timeout_add (max_duration,
						  force_close_cb,
						  NULL);
	priv->scheduled_duration = scheduled_duration;
	priv->max_duration       = max_duration;
	priv->is_paused          = FALSE;
	priv->close_cb           = close_cb;

	// hook up private struct to public struct
	t->priv = priv;

	return t;
}

void
timings_destroy (timings_t* t)
{
	// sanity checks
	g_assert (t != NULL);
	g_assert (t->priv != NULL);

	if (g_getenv ("DEBUG"))
	{
		g_print ("on-screen time: %d seconds, %d ms.\n",
			 _ms_elapsed (t->priv->on_screen_timer) / 1000,
			 _ms_elapsed (t->priv->on_screen_timer) % 1000);
		g_print ("paused time   : %d seconds, %d ms.\n",
			 _ms_elapsed (t->priv->paused_timer) / 1000,
			 _ms_elapsed (t->priv->paused_timer) % 1000);
		g_print ("unpaused time : %d seconds, %d ms.\n",
			 _ms_elapsed (t->priv->duration_timer) / 1000,
			 _ms_elapsed (t->priv->duration_timer) % 1000);
		g_print ("scheduled time: %d seconds, %d ms.\n",
			 t->priv->scheduled_duration / 1000,
			 t->priv->scheduled_duration % 1000);
	} 

	// free any allocated resources
	if (t->priv->on_screen_timer)
		g_timer_destroy (t->priv->on_screen_timer);

	if (t->priv->duration_timer)
		g_timer_destroy (t->priv->duration_timer);

	if (t->priv->paused_timer)
		g_timer_destroy (t->priv->paused_timer);

	if (t->priv->timeout_id != 0)
		g_source_remove (t->priv->timeout_id);

	if (t->priv->max_timeout_id != 0)
		g_source_remove (t->priv->max_timeout_id);

	// get rid of the main allocated structs
	g_free ((gpointer) t->priv);
	g_free ((gpointer) t);
}

void
timings_extend_by_ms (timings_t* t,
		      guint      extension)
{
	gboolean removed_successfully;
	guint    on_screen_time; // value interpreted as milliseconds

	// sanity checks
	g_assert (t != NULL);

	// you never know how stupid the caller may be
	if (extension == 0)
		return;

	// if paused only update scheduled duration and return
	if (t->priv->is_paused)
	{
		if (t->priv->scheduled_duration + extension >
		    t->priv->max_duration)
			t->priv->scheduled_duration = t->priv->max_duration;
		else
			t->priv->scheduled_duration += extension;

		return;
	}

	// try to get rid of old timeout
	removed_successfully = g_source_remove (t->priv->timeout_id);
	g_assert (removed_successfully);

	// ensure we don't overshoot limit with the on-screen time
	on_screen_time = _ms_elapsed (t->priv->duration_timer);
	if (t->priv->scheduled_duration + extension > t->priv->max_duration)
	{
		extension = t->priv->max_duration - on_screen_time;
		t->priv->scheduled_duration = t->priv->max_duration;
	}
	else
	{
		t->priv->scheduled_duration += extension;
		extension = t->priv->scheduled_duration - on_screen_time;
	}

	// add new timeout
	t->priv->timeout_id = g_timeout_add (extension,
					     t->priv->close_cb,
					     NULL);
}

void
timings_pause (timings_t* t)
{
	gboolean removed_successfully;

	// sanity checks
	g_assert (t != NULL);

	// don't continue if we are already paused
	if (t->priv->is_paused)
	{
		if (g_getenv ("DEBUG"))
			g_print ("*** WARNING: Already paused!\n");

		return;
	}

	// make paused-timer tick again, hold duration-timer and update flag
	g_timer_continue (t->priv->paused_timer);
	g_timer_stop (t->priv->duration_timer);
	t->priv->is_paused = TRUE;

	// try to get rid of old timeout
	removed_successfully = g_source_remove (t->priv->timeout_id);
	g_assert (removed_successfully);
}

void
timings_continue (timings_t* t)
{
	guint extension;

	// sanity checks
	g_assert (t != NULL);

	// don't continue if we are not paused
	if (!t->priv->is_paused)
	{
		if (g_getenv ("DEBUG"))
			g_print ("*** WARNING: Already running!\n");

		return;
	}

	// make duration-timer tick again, hold paused-timer and update flag
	g_timer_continue (t->priv->duration_timer);
	g_timer_stop (t->priv->paused_timer);
	t->priv->is_paused = FALSE;

	// put new timeout in place
	extension = t->priv->scheduled_duration -
		    _ms_elapsed (t->priv->duration_timer);
	t->priv->timeout_id = g_timeout_add (extension,
					     t->priv->close_cb,
					     NULL);
	g_assert (t->priv->timeout_id != 0);
}

