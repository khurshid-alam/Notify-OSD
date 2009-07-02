////////////////////////////////////////////////////////////////////////////////
//3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
//      10        20        30        40        50        60        70        80
//
// notify-osd
//
// tile-test.c - exercises the tile-API doing surface- and blur-caching
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

#include <cairo.h>
#include <pango/pangocairo.h>

#include "tile.h"

#define WIDTH  500
#define HEIGHT 500

#define TILE_WIDTH  250
#define TILE_HEIGHT 100

#define BLUR_RADIUS 2

cairo_surface_t*
render_text_to_surface (gchar* text,
			gint   width,
			gint   height,
			guint  blur_radius)
{
	cairo_surface_t*      surface;
	cairo_t*              cr;
	PangoFontDescription* desc;
	PangoLayout*          layout;
	PangoRectangle        ink_rect;
	PangoRectangle        log_rect;

	// sanity check
	if (!text      ||
	    width <= 0 ||
	    height <= 0)
		return NULL;

	// create surface
	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					      width,
					      height);
	if (cairo_surface_status (surface) != CAIRO_STATUS_SUCCESS)
		return NULL;

	// create context
	cr = cairo_create (surface);
	if (cairo_status (cr) != CAIRO_STATUS_SUCCESS)
	{
		cairo_surface_destroy (surface);
		return NULL;
	}

	// clear context
	cairo_scale (cr, 1.0f, 1.0f);
	cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint (cr);

	//
	layout = pango_cairo_create_layout (cr);
	desc = pango_font_description_new ();

	pango_font_description_set_size (desc, 15 * PANGO_SCALE);
	pango_font_description_set_family_static (desc, "Candara");
	pango_font_description_set_weight (desc, PANGO_WEIGHT_NORMAL);
	pango_font_description_set_style (desc, PANGO_STYLE_NORMAL);
	pango_layout_set_wrap (layout, PANGO_WRAP_WORD);
	pango_layout_set_font_description (layout, desc);
	pango_font_description_free (desc);
	pango_layout_set_width (layout,
				(width - 2 * blur_radius) * PANGO_SCALE);
	pango_layout_set_height (layout,
				 (height - 2 * blur_radius) * PANGO_SCALE);
	pango_layout_set_alignment (layout, PANGO_ALIGN_CENTER);
	pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);

        // print and layout string (pango-wise)
        pango_layout_set_text (layout, text, -1);
        pango_layout_get_extents (layout, &ink_rect, &log_rect);

	cairo_move_to (cr, (gdouble) blur_radius, (gdouble) blur_radius);

	// draw pango-text as path to our cairo-context
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	cairo_set_source_rgba (cr, 1.0f, 1.0f, 1.0f, 1.0f);
	pango_cairo_show_layout (cr, layout);

	// clean up
	g_object_unref (layout);
	cairo_destroy (cr);

	return surface;
}

int 
main (int    argc,
      char** argv)
{
	cairo_surface_t* tile_surface = NULL;
	cairo_surface_t* surface = NULL;
	cairo_t*         cr      = NULL;
	tile_t*          tile    = NULL;
	cairo_pattern_t* pattern = NULL;

	// create and setup image-surface and context
	tile_surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
						   TILE_WIDTH,
						   TILE_HEIGHT);
	if (cairo_surface_status (tile_surface) != CAIRO_STATUS_SUCCESS)
	{
		g_debug ("Could not create tile-surface!");
		return 1;
	}

	cr = cairo_create (tile_surface);
	if (cairo_status (cr) != CAIRO_STATUS_SUCCESS)
	{
		cairo_surface_destroy (tile_surface);
		g_debug ("Could not create context for tile-surface!");
		return 2;
	}

	// draw something onto the tile-surface
	tile_surface = render_text_to_surface (
				"Polyfon zwitschernd aßen Mäxchens"
				" Vögel Rüben, Joghurt und Quark.\0",
				TILE_WIDTH,
				TILE_HEIGHT,
				BLUR_RADIUS);

	// create and setup tile from that with a blur-radius of 5px
	tile = tile_new (tile_surface, BLUR_RADIUS);
	cairo_surface_destroy (tile_surface);
	cairo_destroy (cr);

	// create and setup result-surface and context
	surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					      WIDTH,
					      HEIGHT);
	if (cairo_surface_status (surface) != CAIRO_STATUS_SUCCESS)
	{
		tile_destroy (tile);
		g_debug ("Could not create result-surface!");
		return 3;
	}

	cr = cairo_create (surface);
	if (cairo_status (cr) != CAIRO_STATUS_SUCCESS)
	{
		tile_destroy (tile);
		cairo_surface_destroy (surface);
		g_debug ("Could not create context for result-surface!");
		return 4;
	}

	// use the tile for drawing onto result-surface
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	cairo_set_source_rgba (cr, 0.75f, 0.75f, 0.75f, 1.0f);
	cairo_paint (cr);

	// colored drop-shadow trick
	cairo_push_group (cr);
	tile_paint (tile, cr, 10.0f, 10.0f, 0.0f, 1.0f);
	pattern = cairo_pop_group (cr);
	cairo_set_source_rgba (cr, 0.0f, 0.0f, 0.0f, 0.5f);
	cairo_mask (cr, pattern);
	cairo_pattern_destroy (pattern);

	// draw the normal tile-state over drop-shadow
	tile_paint (tile, cr, 10.0f, 10.0f, 1.0f, 0.0f);

	// just draw the normal tile-state
	tile_paint (tile, cr, 10.0f, 110.0f, 1.0f, 0.0f);

	// just draw the blurred tile-state
	tile_paint (tile, cr, 10.0f, 210.0f, 0.0f, 1.0f);

	// save surface to a PNG-file
	cairo_surface_write_to_png (surface, "./tile-result.png");
	g_print ("See file tile-result.png in current directory for output.\n");

	// clean up
	cairo_destroy (cr);
	cairo_surface_destroy (surface);
	tile_destroy (tile);

	return 0;
}

