////////////////////////////////////////////////////////////////////////////////
//3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
//      10        20        30        40        50        60        70        80
//
// notify-osd
//
// tile.c - implements public API surface/blur cache
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

#include <string.h>

#include "tile.h"
#include "raico-blur.h"

struct _tile_private_t
{
	cairo_surface_t* normal;
	cairo_surface_t* blurred;
	guint            blur_radius;
};

cairo_surface_t*
_copy_surface (cairo_surface_t* orig)
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

tile_t*
tile_new (cairo_surface_t* source, guint blur_radius)
{
	tile_private_t* priv = NULL;
	tile_t*         tile = NULL;
	raico_blur_t*   blur = NULL;

	if (blur_radius < 1)
		return NULL;

	priv = g_new0 (tile_private_t, 1);
	if (!priv)
		return NULL;

	tile = g_new0 (tile_t, 1);
	if (!tile)
		return NULL;

	tile->priv = priv;

	tile->priv->normal      = _copy_surface (source);
	tile->priv->blurred     = _copy_surface (source);
	tile->priv->blur_radius = blur_radius;

	blur = raico_blur_create (RAICO_BLUR_QUALITY_LOW);
	raico_blur_set_radius (blur, blur_radius);
	raico_blur_apply (blur, tile->priv->blurred);
	raico_blur_destroy (blur);

	return tile;
}

void
tile_destroy (tile_t* tile)
{
	if (!tile)
		return;

	//cairo_surface_write_to_png (tile->priv->normal, "./tile-normal.png");
	//cairo_surface_write_to_png (tile->priv->blurred, "./tile-blurred.png");

	cairo_surface_destroy (tile->priv->normal);
	cairo_surface_destroy (tile->priv->blurred);

	g_free ((gpointer) tile->priv);
	g_free ((gpointer) tile);
}

// assumes x and y to be actual pixel-measurement values
void
tile_paint (tile_t*  tile,
	    cairo_t* cr,
	    gdouble  x,
	    gdouble  y,
	    gdouble  normal_alpha,
	    gdouble  blurred_alpha)
{
	if (!tile)
		return;

	if (cairo_status (cr) != CAIRO_STATUS_SUCCESS)
		return;

	if (normal_alpha > 0.0f)
	{
		cairo_set_source_surface (cr, tile->priv->normal, x, y);
		cairo_paint_with_alpha (cr, normal_alpha);
	}

	if (blurred_alpha > 0.0f)
	{
		cairo_set_source_surface (cr, tile->priv->blurred, x, y);
		cairo_paint_with_alpha (cr, blurred_alpha);
	}

}

