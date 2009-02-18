/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
**      10        20        30        40        50        60        70        80
**
** notify-osd
**
** display.c - manages the display of notifications waiting in the stack/queue
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

/* This is actually part of stack.c, but moved here because it manages
   the display of notifications. This is also in preparation for some
   refactoring, where we'll create:
 *  - a distinct (non-graphical) notification.c module
 *  - a distinct display.c module that takes care of placing bubbles
 *    on the screen
 */
  

static Bubble*
stack_find_bubble_on_display (Stack *self)
{
	GList*    list   = NULL;
	Bubble*   bubble = NULL;

	g_assert (IS_STACK (self));

	/* find the bubble on display */
	for (list = g_list_first (self->list);
	     list != NULL;
	     list = g_list_next (list))
	{
		bubble = (Bubble*) list->data;

		if (bubble_is_visible (bubble))
			return bubble;
	}

	return NULL;
}

static void
stack_get_top_corner (Stack *self, gint *x, gint *y)
{
	Defaults* d;

	g_assert (IS_STACK (self));

	/* Position the top left corner of the stack. */
	d = self->defaults;
	*y  =  defaults_get_desktop_top (d);
	*y  += EM2PIXELS (defaults_get_bubble_vert_gap (d), d)
	       - EM2PIXELS (defaults_get_bubble_shadow_size (d), d);

	*x  =  (gtk_widget_get_default_direction () == GTK_TEXT_DIR_LTR) ?
		(defaults_get_desktop_right (d) -
		 EM2PIXELS (defaults_get_bubble_shadow_size (d), d) -
		 EM2PIXELS (defaults_get_bubble_horz_gap (d), d) -
		 EM2PIXELS (defaults_get_bubble_width (d), d))
		:
		(defaults_get_desktop_left (d) -
		 EM2PIXELS (defaults_get_bubble_shadow_size (d), d) +
		 EM2PIXELS (defaults_get_bubble_horz_gap (d), d))
		;
}

static gboolean
stack_is_at_top_corner (Stack *self, Bubble *bubble)
{
	gint x, y1, y2;

	stack_get_top_corner (self, &x, &y1);
	bubble_get_position (bubble, &x, &y2);

	return y1 == y2;
}

static void
stack_display_sync_bubble (Stack *self, Bubble *bubble)
{
	Defaults* d;
	gint      y      = 0;
	gint      x      = 0;

	g_return_if_fail (IS_STACK (self));
	g_return_if_fail (IS_BUBBLE (bubble));

	bubble_set_timeout (bubble, 2000);

	Bubble *other = stack_find_bubble_on_display (self);
	if (other != NULL)
	{
		/* synchronize the sync bubble with 
		   the timeout of the bubble at the bottom */
		if (stack_is_at_top_corner (self, bubble))
			bubble_sync_with (bubble, other);
		else 
			bubble_sync_with (bubble, other);
		
		bubble_refresh (other);
	}

	/* is the notification reusing the current bubble? */
	if (sync_bubble == bubble)
	{
		bubble_start_timer (bubble);
		bubble_refresh (bubble);
		return;
	}

	stack_get_top_corner (self, &x, &y);

	Bubble *async = stack_find_bubble_on_display (self);
	if (async != NULL)
	{
		d = self->defaults;
		y += bubble_get_height (async);
		y += EM2PIXELS (defaults_get_bubble_vert_gap (d), d)
		     - 2 * EM2PIXELS (defaults_get_bubble_shadow_size (d), d);
	}

	bubble_move (bubble, x, y);

	bubble_fade_in (bubble, 100);

	sync_bubble = bubble;

	g_signal_connect (G_OBJECT (bubble),
			  "timed-out",
			  G_CALLBACK (close_handler),
			  self);
}

static Bubble*
stack_select_next_to_display (Stack *self)
{
	Bubble*   next_to_display = NULL;
	GList*    list   = NULL;
	Bubble*   bubble = NULL;

	/* pickup the next bubble to display */
	for (list = g_list_first (self->list);
	     list != NULL;
	     list = g_list_next (list))
	{
		bubble = (Bubble*) list->data;

		/* sync. bubbles have already been taken care of */
		if (bubble_is_synchronous (bubble))
		{
			g_critical ("synchronous notifications are managed separately");
			continue;
		}

		/* if there is already one bubble on display
		   we don't have room for another one */
		if (bubble_is_visible (bubble))
			return NULL;

		if (bubble_is_urgent (bubble))
		{
			/* pick-up the /first/ urgent bubble
			   in the queue (FIFO) */
			return bubble;

		}

		if (next_to_display == NULL)
			next_to_display = bubble;

		/* loop, in case there are urgent bubbles waiting higher up
		   in the stack */
	}

	return next_to_display;
}

static void
stack_layout (Stack* self)
{
	Bubble*   bubble = NULL;
	Defaults* d;
	gint      y      = 0;
	gint      x      = 0;

	g_return_if_fail (self != NULL);

	stack_purge_old_bubbles (self);

	bubble = stack_select_next_to_display (self);
	if (bubble == NULL)
		/* this actually happens when we're called for a synchronous
		   bubble or after a bubble timed out, but there where no other
		   notifications waiting in the queue */
		return;

	if (dnd_dont_disturb_user ()
	    && bubble_is_urgent (bubble))
		return;

	bubble_set_timeout (bubble,
			    defaults_get_on_screen_timeout (self->defaults));

	stack_get_top_corner (self, &x, &y);

	if (sync_bubble != NULL
	    && bubble_is_visible (sync_bubble))
	{
		d = self->defaults;
		y += bubble_get_height (sync_bubble);
		y += EM2PIXELS (defaults_get_bubble_vert_gap (d), d)
		     - 2 * EM2PIXELS (defaults_get_bubble_shadow_size (d), d);

		/* synchronize the sync bubble with 
		   the timeout of the bubble at the bottom */
		if (stack_is_at_top_corner (self, sync_bubble))
			bubble_sync_with (sync_bubble, bubble);
	}

	bubble_move (bubble, x, y);

	/* TODO: adjust timings for bubbles that appear in a serie of bubbles */
	if (bubble_is_urgent (bubble))
		bubble_fade_in (bubble, 100);
	else 
		bubble_fade_in (bubble, 200);
}
