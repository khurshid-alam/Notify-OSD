////////////////////////////////////////////////////////////////////////////////
//3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
//      10        20        30        40        50        60        70        80
//
// notify-osd
//
// timings.h - timings object handling duration and max. on-screen time
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

#ifndef _TIMINGS_H
#define _TIMINGS_H

#include <glib.h>

typedef struct _timings_private_t timings_private_t;

typedef struct _timings_t
{
	timings_private_t* priv;
} timings_t;

timings_t*
timings_new (guint       scheduled_duration,
	     guint       max_duration,
	     GSourceFunc close_cb,
	     GSourceFunc force_close_cb);

void
timings_destroy (timings_t* t);

void
timings_extend_by_ms (timings_t* t,
		      guint      extension);

void
timings_pause (timings_t* t);

void
timings_continue (timings_t* t);

#endif // _TIMINGS_H

