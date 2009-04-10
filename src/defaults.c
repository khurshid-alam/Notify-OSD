/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
**      10        20        30        40        50        60        70        80
**
** notify-osd
**
** defaults.c - a singelton providing all default values for sizes, colors etc. 
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

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <gconf/gconf-client.h>
#include <libwnck/application.h>
#include <libwnck/class-group.h>
#include <libwnck/workspace.h>

#include "defaults.h"

G_DEFINE_TYPE (Defaults, defaults, G_TYPE_OBJECT);

enum
{
	PROP_DUMMY = 0,
	PROP_DESKTOP_WIDTH,
	PROP_DESKTOP_HEIGHT,
	PROP_DESKTOP_TOP,
	PROP_DESKTOP_BOTTOM,
	PROP_DESKTOP_LEFT,
	PROP_DESKTOP_RIGHT,
	PROP_DESKTOP_BOTTOM_GAP,
	PROP_STACK_HEIGHT,
	PROP_BUBBLE_VERT_GAP,
	PROP_BUBBLE_HORZ_GAP,
	PROP_BUBBLE_WIDTH,
	PROP_BUBBLE_MIN_HEIGHT,
	PROP_BUBBLE_MAX_HEIGHT,
	PROP_BUBBLE_SHADOW_SIZE,
	PROP_BUBBLE_SHADOW_COLOR,
	PROP_BUBBLE_BG_COLOR,
	PROP_BUBBLE_BG_OPACITY,
	PROP_BUBBLE_HOVER_OPACITY,
	PROP_BUBBLE_CORNER_RADIUS,
	PROP_CONTENT_SHADOW_SIZE,
	PROP_CONTENT_SHADOW_COLOR,
	PROP_MARGIN_SIZE,
	PROP_ICON_SIZE,
	PROP_FADE_IN_TIMEOUT,
	PROP_FADE_OUT_TIMEOUT,
	PROP_ON_SCREEN_TIMEOUT,
	PROP_TEXT_FONT_FACE,
	PROP_TEXT_TITLE_COLOR,
	PROP_TEXT_TITLE_WEIGHT,
	PROP_TEXT_TITLE_SIZE,
	PROP_TEXT_BODY_COLOR,
	PROP_TEXT_BODY_WEIGHT,
	PROP_TEXT_BODY_SIZE,
	PROP_PIXELS_PER_EM
};

enum
{
	VALUE_CHANGED,
	LAST_SIGNAL
};

enum
{
	R = 0,
	G,
	B
};

/* taking hints from Pango here, Qt looks a bit different*/
enum
{
	TEXT_WEIGHT_LIGHT  = 300, /* QFont::Light  25 */
	TEXT_WEIGHT_NORMAL = 400, /* QFont::Normal 50 */
	TEXT_WEIGHT_BOLD   = 700  /* QFont::Bold   75 */
};

/* these values are interpreted as em-measurements and do comply to the 
 * visual guide for jaunty-notifications */
#define DEFAULT_DESKTOP_BOTTOM_GAP    6.0f
#define DEFAULT_BUBBLE_WIDTH         23.0f
#define DEFAULT_BUBBLE_MIN_HEIGHT     5.0f
#define DEFAULT_BUBBLE_MAX_HEIGHT    12.2f
#define DEFAULT_BUBBLE_VERT_GAP       0.5f
#define DEFAULT_BUBBLE_HORZ_GAP       0.5f
#define DEFAULT_BUBBLE_SHADOW_SIZE    0.7f
#define DEFAULT_BUBBLE_SHADOW_COLOR  "#000000"
#define DEFAULT_BUBBLE_BG_COLOR      "#131313"
#define DEFAULT_BUBBLE_BG_OPACITY    "#cc"
#define DEFAULT_BUBBLE_HOVER_OPACITY "#66"
#define DEFAULT_BUBBLE_CORNER_RADIUS 0.375f
#define DEFAULT_CONTENT_SHADOW_SIZE  0.125f
#define DEFAULT_CONTENT_SHADOW_COLOR "#000000"
#define DEFAULT_MARGIN_SIZE          1.4f
#define DEFAULT_ICON_SIZE            4.0f
#define DEFAULT_TEXT_FONT_FACE       "Sans"
#define DEFAULT_TEXT_TITLE_COLOR     "#ffffff"
#define DEFAULT_TEXT_TITLE_WEIGHT    TEXT_WEIGHT_BOLD
#define DEFAULT_TEXT_TITLE_SIZE      0.95f
#define DEFAULT_TEXT_BODY_COLOR      "#eaeaea"
#define DEFAULT_TEXT_BODY_WEIGHT     TEXT_WEIGHT_NORMAL
#define DEFAULT_TEXT_BODY_SIZE       0.8f
#define DEFAULT_PIXELS_PER_EM        10

/* these values are interpreted as milliseconds-measurements and do comply to
 * the visual guide for jaunty-notifications */
#define DEFAULT_FADE_IN_TIMEOUT      250
#define DEFAULT_FADE_OUT_TIMEOUT     1000
#define DEFAULT_ON_SCREEN_TIMEOUT    5000

/* GConf-keys to watch */
#define GCONF_UI_FONT_NAME        "/desktop/gnome/interface/font_name"
#define GCONF_FONT_ANTIALIAS      "/desktop/gnome/font_rendering/antialiasing"
#define GCONF_FONT_DPI            "/desktop/gnome/font_rendering/dpi"
#define GCONF_FONT_HINTING        "/desktop/gnome/font_rendering/hinting"
#define GCONF_FONT_SUBPIXEL_ORDER "/desktop/gnome/font_rendering/rgba_order"

/* GConf-trees to watch */
#define GCONF_UI_TREE             "/desktop/gnome/interface"
#define GCONF_FONT_TREE           "/desktop/gnome/font_rendering"

/* notify-osd settings */
#define GCONF_MULTIHEAD_MODE "/apps/notify-osd/multihead_mode"

static guint g_defaults_signals[LAST_SIGNAL] = { 0 };

/*-- internal API ------------------------------------------------------------*/

static void
_get_font_size_dpi (Defaults* self)
{
	GString*   string        = NULL;
	GError*    error         = NULL;
	GScanner*  scanner       = NULL;
	GTokenType token         = G_TOKEN_NONE;
	gint       points        = 0;
	GString*   font_face     = NULL;
	gdouble    dpi           = 0.0f;
	gint       pixels_per_em = 0;

	if (!IS_DEFAULTS (self))
		return;

	/* determine current system font-name/size */
	error = NULL;
	string = g_string_new (gconf_client_get_string (self->context,
							GCONF_UI_FONT_NAME,
							&error));
	if (error)
	{
		/* if something went wrong, assume "Sans 10" and continue */
		string = g_string_assign (string, "Sans 10");
	}

	/* extract font-family-name and font-size */
	scanner = g_scanner_new (NULL);
	if (scanner)
	{
		g_scanner_input_text (scanner, string->str, string->len);
		for (token = g_scanner_get_next_token (scanner);
		     token != G_TOKEN_EOF;
		     token = g_scanner_get_next_token (scanner))
		{
			switch (token)
			{
				case G_TOKEN_INT:
					points = (gint) scanner->value.v_int;
				break;

				case G_TOKEN_IDENTIFIER:
					if (!font_face)
						font_face = g_string_new (scanner->value.v_string);
					else
					{
						g_string_append (font_face,
								 " ");
						g_string_append (font_face,
								 scanner->value.v_string);
					}
				break;

				default:
				break;
			}
		}
		g_scanner_destroy (scanner);
	}

	/* clean up */
	if (string != NULL)
		g_string_free (string, TRUE);

	/* update stored font-face name and clean up */
	if (font_face != NULL)
	{
		g_object_set (self, "text-font-face", font_face->str, NULL);
		g_string_free (font_face, TRUE);
	}

	/* determine current system DPI-setting */
	error = NULL;
	dpi = gconf_client_get_float (self->context, GCONF_FONT_DPI, &error);
	if (error)
	{
		/* if something went wrong, assume 72 DPI and continue */
		dpi = 72.0f;
	}

	/* update stored DPI-value */
	pixels_per_em = (gint) ((gdouble) points * 1.0f / 72.0f * dpi);
	g_object_set (self, "pixels-per-em", (gint) pixels_per_em, NULL);
}

static void
_font_changed (GConfClient* client,
	       guint        cnxn_id,
	       GConfEntry*  entry,
               gpointer     data)
{
	Defaults* defaults;

	if (!data)
		return;

	defaults = (Defaults*) data;
	if (!IS_DEFAULTS (defaults))
		return;

    	/* grab system-wide font-face/size and DPI */
	_get_font_size_dpi (defaults);

	g_signal_emit (defaults, g_defaults_signals[VALUE_CHANGED], 0);
}

static void
_antialias_changed (GConfClient* client,
		    guint        cnxn_id,
		    GConfEntry*  entry,
		    gpointer     data)
{
	Defaults* defaults;

	if (!data)
		return;

	defaults = (Defaults*) data;
	if (!IS_DEFAULTS (defaults))
		return;

	/* just triggering a redraw by emitting the "value-changed" signal is
	** enough in this case, no need to update any stored values */
	g_signal_emit (defaults, g_defaults_signals[VALUE_CHANGED], 0);
}

static void
_dpi_changed (GConfClient* client,
	      guint        cnxn_id,
	      GConfEntry*  entry,
	      gpointer     data)
{
	Defaults* defaults;

	if (!data)
		return;

	defaults = (Defaults*) data;
	if (!IS_DEFAULTS (defaults))
		return;

    	/* grab system-wide font-face/size and DPI */
	_get_font_size_dpi (defaults);

	g_signal_emit (defaults, g_defaults_signals[VALUE_CHANGED], 0);
}

static void
_hinting_changed (GConfClient* client,
		  guint        cnxn_id,
		  GConfEntry*  entry,
		  gpointer     data)
{
	Defaults* defaults;

	if (!data)
		return;

	defaults = (Defaults*) data;
	if (!IS_DEFAULTS (defaults))
		return;

	/* just triggering a redraw by emitting the "value-changed" signal is
	** enough in this case, no need to update any stored values */
	g_signal_emit (defaults, g_defaults_signals[VALUE_CHANGED], 0);
}

static void
_subpixel_order_changed (GConfClient* client,
			 guint        cnxn_id,
			 GConfEntry*  entry,
			 gpointer     data)
{
	Defaults* defaults;

	if (!data)
		return;

	defaults = (Defaults*) data;
	if (!IS_DEFAULTS (defaults))
		return;

	/* just triggering a redraw by emitting the "value-changed" signal is
	** enough in this case, no need to update any stored values */
	g_signal_emit (defaults, g_defaults_signals[VALUE_CHANGED], 0);
}

static gdouble
_get_average_char_width (Defaults* self)
{
	cairo_surface_t*      surface;
	cairo_t*              cr;
	PangoFontDescription* desc;
	PangoLayout*          layout;
	PangoContext*         context;
	PangoLanguage*        language;
	PangoFontMetrics*     metrics;
	gint                  char_width;

	if (!self || !IS_DEFAULTS (self))
		return 0;

	surface = cairo_image_surface_create (CAIRO_FORMAT_A1, 1, 1);
	if (cairo_surface_status (surface) != CAIRO_STATUS_SUCCESS)
		return 0;

	cr = cairo_create (surface);
	cairo_surface_destroy (surface);
	if (cairo_status (cr) != CAIRO_STATUS_SUCCESS)
		return 0;

	layout = pango_cairo_create_layout (cr);
	desc = pango_font_description_new ();
	pango_font_description_set_size (
		desc,
		EM2PIXELS (defaults_get_text_title_size (self), self) *
		PANGO_SCALE);

	pango_font_description_set_family_static (
		desc,
		defaults_get_text_font_face (self));

	pango_font_description_set_weight (
		desc,
		defaults_get_text_title_weight (self));

	pango_font_description_set_style (desc, PANGO_STYLE_NORMAL);
	pango_layout_set_wrap (layout, PANGO_WRAP_WORD);
	pango_layout_set_font_description (layout, desc);

	context  = pango_layout_get_context (layout); /* no need to unref */
	language = pango_language_get_default ();     /* no need to unref */
	metrics  = pango_context_get_metrics (context, desc, language);
	char_width = pango_font_metrics_get_approximate_char_width (metrics);

	/* clean up */
	pango_font_metrics_unref (metrics);
	pango_font_description_free (desc);
	g_object_unref (layout);
	cairo_destroy (cr);

	return PIXELS2EM (char_width / PANGO_SCALE, self);
}

void
defaults_refresh_screen_dimension_properties (Defaults *self)
{
	Atom         real_type;
	gint         result;
	gint         real_format;
	gulong       items_read;
	gulong       items_left;
	glong*       coords;
	Atom         workarea_atom;

	g_return_if_fail ((self != NULL) && IS_DEFAULTS (self));

	/* get real desktop-area without the panels */
	workarea_atom = gdk_x11_get_xatom_by_name ("_NET_WORKAREA");
  
	gdk_error_trap_push ();
	result = XGetWindowProperty (GDK_DISPLAY (),
				     GDK_ROOT_WINDOW (),
				     workarea_atom,
				     0L,
				     4L,
				     False,
				     XA_CARDINAL,
				     &real_type,
				     &real_format,
				     &items_read,
				     &items_left,
				     (guchar **) (void*) &coords);
	gdk_flush ();
	gdk_error_trap_pop ();

	if (result == Success && items_read)
	{
		g_object_set (self,
			      "desktop-width",
			      (gint) coords[2],
			      NULL);
		g_object_set (self,
			      "desktop-height",
			      (gint) coords[3],
			      NULL);
		g_object_set (self,
			      "desktop-top",
			      (gint) coords[1],
			      NULL);
		g_object_set (self,
			      "desktop-bottom",
			      (gint) coords[3],
			      NULL);
		g_object_set (self,
			      "desktop-left",
			      (gint) coords[0],
			      NULL);
		g_object_set (self,
			      "desktop-right",
			      (gint) coords[2],
			      NULL);
		/* FIXME: use new upper and lower threshold/limits for stack */
		/*g_object_set (self,
			      "stack-height",
			      (gint) coords[3] / 2,
			      NULL);*/
		XFree (coords);
	}
}

static void
defaults_constructed (GObject* gobject)
{
	Defaults*    self;
	gdouble      margin_size;
	gdouble      icon_size;
	gdouble      bubble_height;
	gdouble      new_bubble_height;
	gdouble      bubble_width;
	gdouble      new_bubble_width;
	gdouble      average_char_width;

	self = DEFAULTS (gobject);

	defaults_refresh_screen_dimension_properties (self);

	/* grab system-wide font-face/size and DPI */
	_get_font_size_dpi (self);

	/* correct the default min. bubble-height, according to the icon-size */
	g_object_get (self,
		      "margin-size",
		      &margin_size,
		      NULL);
	g_object_get (self,
		      "icon-size",
		      &icon_size,
		      NULL);
	g_object_get (self,
		      "bubble-min-height",
		      &bubble_height,
		      NULL);

#if 0
	/* try to register the non-standard size for the gtk_icon_theme_lookup
	   calls to work */
	gtk_icon_size_register ("52x52",
				pixels_per_em * icon_size,
				pixels_per_em * icon_size);
#endif

	new_bubble_height = 2.0f * margin_size + icon_size;

	if (new_bubble_height > bubble_height)
	{
		g_object_set (self,
			      "bubble-min-height",
			      new_bubble_height,
			      NULL);
	}

	/* correct the default bubble-width depending on the average width of a 
	 * character rendered in the default system-font at the default
	 * font-size,
	 * as default layout, we'll take the icon+title+body+message case, thus
	 * seen from left to right we use:
	 *
	 *      margin + icon_size + margin + 20 * avg_char_width + margin
	 */
	g_object_get (self,
		      "bubble-width",
		      &bubble_width,
		      NULL);
	average_char_width = _get_average_char_width (self);

	new_bubble_width = 3.0f * margin_size +
			   icon_size +
			   20.0f * average_char_width;
	if (new_bubble_width > bubble_width)
	{
		g_object_set (self,
			      "bubble-width",
			      new_bubble_width,
			      NULL);
	}

	/* FIXME: calling this here causes a segfault */
	/* chain up to the parent class */
	/*G_OBJECT_CLASS (defaults_parent_class)->constructed (gobject);*/
}

static void
defaults_dispose (GObject* gobject)
{
	Defaults* defaults;

	defaults = DEFAULTS (gobject);

	gconf_client_notify_remove (defaults->context, defaults->notifier[0]);
	gconf_client_notify_remove (defaults->context, defaults->notifier[1]);
	gconf_client_notify_remove (defaults->context, defaults->notifier[2]);
	gconf_client_notify_remove (defaults->context, defaults->notifier[3]);
	gconf_client_notify_remove (defaults->context, defaults->notifier[4]);
	gconf_client_remove_dir (defaults->context, GCONF_UI_TREE, NULL);
	gconf_client_remove_dir (defaults->context, GCONF_FONT_TREE, NULL);
	g_object_unref (defaults->context);

	/* chain up to the parent class */
	G_OBJECT_CLASS (defaults_parent_class)->dispose (gobject);
}

static void
defaults_finalize (GObject* gobject)
{
	/* chain up to the parent class */
	G_OBJECT_CLASS (defaults_parent_class)->finalize (gobject);
}

static void
defaults_init (Defaults* self)
{
	GError* error;

	/* "connect" to the whole gconf-thing */
	self->context = gconf_client_get_default ();
	if (!self->context)
	{
		g_warning ("Could not get GConf client-context");
		return;
	}

	/* register watching all relevant GNOME UI-settings */
	error = NULL;
	gconf_client_add_dir (self->context,
			      GCONF_UI_TREE,
			      GCONF_CLIENT_PRELOAD_NONE,
			      &error);
	if (error)
	{
		g_object_unref (self->context);
		g_warning ("%s\n", error->message);
		return;
	}

	/* register watching all font-settings */
	error = NULL;
	gconf_client_add_dir (self->context,
			      GCONF_FONT_TREE,
			      GCONF_CLIENT_PRELOAD_NONE,
			      &error);
	if (error)
	{
		gconf_client_remove_dir (self->context, GCONF_UI_TREE, NULL);
		g_object_unref (self->context);
		g_warning ("%s\n", error->message);
		return;
	}

	/* hook up notifier for font-name/size changes */
	error = NULL;
	self->notifier[0] = gconf_client_notify_add (self->context,
						     GCONF_UI_FONT_NAME,
						     _font_changed,
						     (gpointer) self,
						     NULL,
						     &error);
	if (error)
	{
		gconf_client_remove_dir (self->context, GCONF_UI_TREE, NULL);
		gconf_client_remove_dir (self->context, GCONF_FONT_TREE, NULL);
		g_object_unref (self->context);
		g_warning ("%s\n", error->message);
		return;
	}

	/* hook up notifier for antialiasing changes */
	error = NULL;
	self->notifier[1] = gconf_client_notify_add (self->context,
						     GCONF_FONT_ANTIALIAS,
						     _antialias_changed,
						     (gpointer) self,
						     NULL,
						     &error);
	if (error)
	{
		gconf_client_notify_remove (self->context, self->notifier[0]);
		gconf_client_remove_dir (self->context, GCONF_UI_TREE, NULL);
		gconf_client_remove_dir (self->context, GCONF_FONT_TREE, NULL);
		g_object_unref (self->context);
		g_warning ("%s\n", error->message);
		return;
	}

	/* hook up notifier for DPI changes */
	error = NULL;
	self->notifier[2] = gconf_client_notify_add (self->context,
						     GCONF_FONT_DPI,
						     _dpi_changed,
						     (gpointer) self,
						     NULL,
						     &error);
	if (error)
	{
		gconf_client_notify_remove (self->context, self->notifier[0]);
		gconf_client_notify_remove (self->context, self->notifier[1]);
		gconf_client_remove_dir (self->context, GCONF_UI_TREE, NULL);
		gconf_client_remove_dir (self->context, GCONF_FONT_TREE, NULL);
		g_object_unref (self->context);
		g_warning ("%s\n", error->message);
		return;
	}
	
	/* hook up notifier for hinting changes */
	error = NULL;
	self->notifier[3] = gconf_client_notify_add (self->context,
						     GCONF_FONT_HINTING,
						     _hinting_changed,
						     (gpointer) self,
						     NULL,
						     &error);
	if (error)
	{
		gconf_client_notify_remove (self->context, self->notifier[0]);
		gconf_client_notify_remove (self->context, self->notifier[1]);
		gconf_client_notify_remove (self->context, self->notifier[2]);
		gconf_client_remove_dir (self->context, GCONF_UI_TREE, NULL);
		gconf_client_remove_dir (self->context, GCONF_FONT_TREE, NULL);
		g_object_unref (self->context);
		g_warning ("%s\n", error->message);
		return;
	}

	/* hook up notifier for subpixel-order changes */
	error = NULL;
	self->notifier[4] = gconf_client_notify_add (self->context,
						     GCONF_FONT_SUBPIXEL_ORDER,
						     _subpixel_order_changed,
						     (gpointer) self,
						     NULL,
						     &error);
	if (error)
	{
		gconf_client_notify_remove (self->context, self->notifier[0]);
		gconf_client_notify_remove (self->context, self->notifier[1]);
		gconf_client_notify_remove (self->context, self->notifier[2]);
		gconf_client_notify_remove (self->context, self->notifier[3]);
		gconf_client_remove_dir (self->context, GCONF_UI_TREE, NULL);
		gconf_client_remove_dir (self->context, GCONF_FONT_TREE, NULL);
		g_object_unref (self->context);
		g_warning ("%s\n", error->message);
		return;
	}
}

static void
defaults_get_property (GObject*    gobject,
		       guint       prop,
		       GValue*     value,
		       GParamSpec* spec)
{
	Defaults* defaults;

	defaults = DEFAULTS (gobject);

	switch (prop)
	{
		case PROP_DESKTOP_WIDTH:
			g_value_set_int (value, defaults->desktop_width);
		break;

		case PROP_DESKTOP_HEIGHT:
			g_value_set_int (value, defaults->desktop_height);
		break;

		case PROP_DESKTOP_TOP:
			g_value_set_int (value, defaults->desktop_top);
		break;

		case PROP_DESKTOP_BOTTOM:
			g_value_set_int (value, defaults->desktop_bottom);
		break;

		case PROP_DESKTOP_LEFT:
			g_value_set_int (value, defaults->desktop_left);
		break;

		case PROP_DESKTOP_RIGHT:
			g_value_set_int (value, defaults->desktop_right);
		break;

		case PROP_DESKTOP_BOTTOM_GAP:
			g_value_set_double (value, defaults->desktop_bottom_gap);
		break;

		case PROP_STACK_HEIGHT:
			g_value_set_double (value, defaults->stack_height);
		break;

		case PROP_BUBBLE_VERT_GAP:
			g_value_set_double (value, defaults->bubble_vert_gap);
		break;

		case PROP_BUBBLE_HORZ_GAP:
			g_value_set_double (value, defaults->bubble_horz_gap);
		break;

		case PROP_BUBBLE_WIDTH:
			g_value_set_double (value, defaults->bubble_width);
		break;

		case PROP_BUBBLE_MIN_HEIGHT:
			g_value_set_double (value, defaults->bubble_min_height);
		break;

		case PROP_BUBBLE_MAX_HEIGHT:
			g_value_set_double (value, defaults->bubble_max_height);
		break;

		case PROP_BUBBLE_SHADOW_SIZE:
			g_value_set_double (value, defaults->bubble_shadow_size);
		break;

		case PROP_BUBBLE_SHADOW_COLOR:
			g_value_set_string (value,
					    defaults->bubble_shadow_color->str);
		break;

		case PROP_BUBBLE_BG_COLOR:
			g_value_set_string (value,
					    defaults->bubble_bg_color->str);
		break;

		case PROP_BUBBLE_BG_OPACITY:
			g_value_set_string (value,
					    defaults->bubble_bg_opacity->str);
		break;

		case PROP_BUBBLE_HOVER_OPACITY:
			g_value_set_string (value,
					    defaults->bubble_hover_opacity->str);
		break;

		case PROP_BUBBLE_CORNER_RADIUS:
			g_value_set_double (value, defaults->bubble_corner_radius);
		break;

		case PROP_CONTENT_SHADOW_SIZE:
			g_value_set_double (value, defaults->content_shadow_size);
		break;

		case PROP_CONTENT_SHADOW_COLOR:
			g_value_set_string (value,
					    defaults->content_shadow_color->str);
		break;

		case PROP_MARGIN_SIZE:
			g_value_set_double (value, defaults->margin_size);
		break;

		case PROP_ICON_SIZE:
			g_value_set_double (value, defaults->icon_size);
		break;

		case PROP_FADE_IN_TIMEOUT:
			g_value_set_int (value, defaults->fade_in_timeout);
		break;

		case PROP_FADE_OUT_TIMEOUT:
			g_value_set_int (value, defaults->fade_out_timeout);
		break;

		case PROP_ON_SCREEN_TIMEOUT:
			g_value_set_int (value, defaults->on_screen_timeout);
		break;

		case PROP_TEXT_FONT_FACE:
			g_value_set_string (value,
					    defaults->text_font_face->str);
		break;

		case PROP_TEXT_TITLE_COLOR:
			g_value_set_string (value,
					    defaults->text_title_color->str);
		break;

		case PROP_TEXT_TITLE_WEIGHT:
			g_value_set_int (value, defaults->text_title_weight);
		break;

		case PROP_TEXT_TITLE_SIZE:
			g_value_set_double (value, defaults->text_title_size);
		break;

		case PROP_TEXT_BODY_COLOR:
			g_value_set_string (value,
					    defaults->text_body_color->str);
		break;

		case PROP_TEXT_BODY_WEIGHT:
			g_value_set_int (value, defaults->text_body_weight);
		break;

		case PROP_TEXT_BODY_SIZE:
			g_value_set_double (value, defaults->text_body_size);
		break;

		case PROP_PIXELS_PER_EM:
			g_value_set_int (value, defaults->pixels_per_em);
		break;

		default :
			G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop, spec);
		break;
	}
}

static void
defaults_set_property (GObject*      gobject,
		       guint         prop,
		       const GValue* value,
		       GParamSpec*   spec)
{
	Defaults* defaults;

	defaults = DEFAULTS (gobject);

	switch (prop)
	{
		case PROP_DESKTOP_WIDTH:
			defaults->desktop_width = g_value_get_int (value);
		break;

		case PROP_DESKTOP_HEIGHT:
			defaults->desktop_height = g_value_get_int (value);
		break;

		case PROP_DESKTOP_TOP:
			defaults->desktop_top = g_value_get_int (value);
		break;

		case PROP_DESKTOP_BOTTOM:
			defaults->desktop_bottom = g_value_get_int (value);
		break;

		case PROP_DESKTOP_LEFT:
			defaults->desktop_left = g_value_get_int (value);
		break;

		case PROP_DESKTOP_RIGHT:
			defaults->desktop_right = g_value_get_int (value);
		break;

		case PROP_DESKTOP_BOTTOM_GAP:
			defaults->desktop_bottom_gap = g_value_get_double (value);
		break;

		case PROP_STACK_HEIGHT:
			defaults->stack_height = g_value_get_double (value);
		break;

		case PROP_BUBBLE_WIDTH:
			defaults->bubble_width = g_value_get_double (value);
		break;

		case PROP_BUBBLE_VERT_GAP:
			defaults->bubble_vert_gap = g_value_get_double (value);
		break;

		case PROP_BUBBLE_HORZ_GAP:
			defaults->bubble_horz_gap = g_value_get_double (value);
		break;

		case PROP_BUBBLE_MIN_HEIGHT:
			defaults->bubble_min_height = g_value_get_double (value);
		break;

		case PROP_BUBBLE_MAX_HEIGHT:
			defaults->bubble_max_height = g_value_get_double (value);
		break;

		case PROP_BUBBLE_SHADOW_SIZE:
			defaults->bubble_shadow_size = g_value_get_double (value);
		break;

		case PROP_BUBBLE_SHADOW_COLOR:
			if (defaults->bubble_shadow_color != NULL)
			{
				g_string_free (defaults->bubble_shadow_color,
					       TRUE);
			}
			defaults->bubble_shadow_color = g_string_new (
				g_value_get_string (value));
		break;

		case PROP_BUBBLE_BG_COLOR:
			if (defaults->bubble_bg_color != NULL)
			{
				g_string_free (defaults->bubble_bg_color, TRUE);
			}
			defaults->bubble_bg_color = g_string_new (
				g_value_get_string (value));
		break;

		case PROP_BUBBLE_BG_OPACITY:
			if (defaults->bubble_bg_opacity != NULL)
			{
				g_string_free (defaults->bubble_bg_opacity,
					       TRUE);
			}
			defaults->bubble_bg_opacity = g_string_new (
				g_value_get_string (value));
		break;

		case PROP_BUBBLE_HOVER_OPACITY:
			if (defaults->bubble_hover_opacity != NULL)
			{
				g_string_free (defaults->bubble_hover_opacity,
					       TRUE);
			}
			defaults->bubble_hover_opacity = g_string_new (
				g_value_get_string (value));
		break;

		case PROP_BUBBLE_CORNER_RADIUS:
			defaults->bubble_corner_radius = g_value_get_double (value);
		break;

		case PROP_CONTENT_SHADOW_SIZE:
			defaults->content_shadow_size = g_value_get_double (value);
		break;

		case PROP_CONTENT_SHADOW_COLOR:
			if (defaults->content_shadow_color != NULL)
			{
				g_string_free (defaults->content_shadow_color,
					       TRUE);
			}
			defaults->content_shadow_color = g_string_new (
				g_value_get_string (value));
		break;

		case PROP_MARGIN_SIZE:
			defaults->margin_size = g_value_get_double (value);
		break;

		case PROP_ICON_SIZE:
			defaults->icon_size = g_value_get_double (value);
		break;

		case PROP_FADE_IN_TIMEOUT:
			defaults->fade_in_timeout = g_value_get_int (value);
		break;

		case PROP_FADE_OUT_TIMEOUT:
			defaults->fade_out_timeout = g_value_get_int (value);
		break;

		case PROP_ON_SCREEN_TIMEOUT:
			defaults->on_screen_timeout = g_value_get_int (value);
		break;

		case PROP_TEXT_FONT_FACE:
			if (defaults->text_font_face != NULL)
			{
				g_string_free (defaults->text_font_face,
					       TRUE);
			}
			defaults->text_font_face = g_string_new (
				g_value_get_string (value));
		break;

		case PROP_TEXT_TITLE_COLOR:
			if (defaults->text_title_color != NULL)
			{
				g_string_free (defaults->text_title_color,
					       TRUE);
			}
			defaults->text_title_color = g_string_new (
				g_value_get_string (value));
		break;

		case PROP_TEXT_TITLE_WEIGHT:
			defaults->text_title_weight = g_value_get_int (value);
		break;

		case PROP_TEXT_TITLE_SIZE:
			defaults->text_title_size = g_value_get_double (value);
		break;

		case PROP_TEXT_BODY_COLOR:
			if (defaults->text_body_color != NULL)
			{
				g_string_free (defaults->text_body_color, TRUE);
			}
			defaults->text_body_color = g_string_new (
				g_value_get_string (value));
		break;

		case PROP_TEXT_BODY_WEIGHT:
			defaults->text_body_weight = g_value_get_int (value);
		break;

		case PROP_TEXT_BODY_SIZE:
			defaults->text_body_size = g_value_get_double (value);
		break;

		case PROP_PIXELS_PER_EM:
			defaults->pixels_per_em = g_value_get_int (value);
		break;

		default :
			G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop, spec);
		break;
	}
}

static void
defaults_class_init (DefaultsClass* klass)
{
	GObjectClass* gobject_class = G_OBJECT_CLASS (klass);

	GdkScreen*    screen = gdk_screen_get_default ();
	GParamSpec*   property_desktop_width;
	GParamSpec*   property_desktop_height;
	GParamSpec*   property_desktop_top;
	GParamSpec*   property_desktop_bottom;
	GParamSpec*   property_desktop_left;
	GParamSpec*   property_desktop_right;
	GParamSpec*   property_desktop_bottom_gap;
	GParamSpec*   property_stack_height;
	GParamSpec*   property_bubble_vert_gap;
	GParamSpec*   property_bubble_horz_gap;
	GParamSpec*   property_bubble_width;
	GParamSpec*   property_bubble_min_height;
	GParamSpec*   property_bubble_max_height;
	GParamSpec*   property_bubble_shadow_size;
	GParamSpec*   property_bubble_shadow_color;
	GParamSpec*   property_bubble_bg_color;
	GParamSpec*   property_bubble_bg_opacity;
	GParamSpec*   property_bubble_hover_opacity;
	GParamSpec*   property_bubble_corner_radius;
	GParamSpec*   property_content_shadow_size;
	GParamSpec*   property_content_shadow_color;
	GParamSpec*   property_margin_size;
	GParamSpec*   property_icon_size;
	GParamSpec*   property_fade_in_timeout;
	GParamSpec*   property_fade_out_timeout;
	GParamSpec*   property_on_screen_timeout;
	GParamSpec*   property_text_font_face;
	GParamSpec*   property_text_title_color;
	GParamSpec*   property_text_title_weight;
	GParamSpec*   property_text_title_size;
	GParamSpec*   property_text_body_color;
	GParamSpec*   property_text_body_weight;
	GParamSpec*   property_text_body_size;
	GParamSpec*   property_pixels_per_em;

	gobject_class->constructed  = defaults_constructed;
	gobject_class->dispose      = defaults_dispose;
	gobject_class->finalize     = defaults_finalize;
	gobject_class->get_property = defaults_get_property;
	gobject_class->set_property = defaults_set_property;

	g_defaults_signals[VALUE_CHANGED] = g_signal_new (
		"value-changed",
		G_OBJECT_CLASS_TYPE (gobject_class),
		G_SIGNAL_RUN_LAST,
		G_STRUCT_OFFSET (DefaultsClass, value_changed),
		NULL,
		NULL,
		g_cclosure_marshal_VOID__VOID,
		G_TYPE_NONE,
		0);

	property_desktop_width = g_param_spec_int (
				"desktop-width",
				"desktop-width",
				"Width of desktop in pixels",
				0,
				4096,
				gdk_screen_get_width (screen),
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_DESKTOP_WIDTH,
					 property_desktop_width);

	property_desktop_height = g_param_spec_int (
				"desktop-height",
				"desktop-height",
				"Height of desktop in pixels",
				0,
				4096,
				gdk_screen_get_height (screen),
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_DESKTOP_HEIGHT,
					 property_desktop_height);

	property_desktop_top = g_param_spec_int (
				"desktop-top",
				"desktop-top",
				"Top of desktop in pixels",
				0,
				4096,
				0,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_DESKTOP_TOP,
					 property_desktop_top);

	property_desktop_bottom = g_param_spec_int (
				"desktop-bottom",
				"desktop-bottom",
				"Bottom of desktop in pixels",
				0,
				4096,
				4096,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_DESKTOP_BOTTOM,
					 property_desktop_bottom);

	property_desktop_left = g_param_spec_int (
				"desktop-left",
				"desktop-left",
				"Left of desktop in pixels",
				0,
				4096,
				0,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_DESKTOP_LEFT,
					 property_desktop_left);

	property_desktop_right = g_param_spec_int (
				"desktop-right",
				"desktop-right",
				"Right of desktop in pixels",
				0,
				4096,
				4096,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_DESKTOP_RIGHT,
					 property_desktop_right);

	property_desktop_bottom_gap = g_param_spec_double (
				"desktop-bottom-gap",
				"desktop-bottom-gap",
				"Bottom gap on the desktop in em",
				0.0f,
				16.0f,
				DEFAULT_DESKTOP_BOTTOM_GAP,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_DESKTOP_BOTTOM_GAP,
					 property_desktop_bottom_gap);

	property_stack_height = g_param_spec_double (
				"stack-height",
				"stack-height",
				"Maximum allowed height of stack (in em)",
				0.0f,
				256.0f,
				50.0f,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_STACK_HEIGHT,
					 property_stack_height);

    	property_bubble_vert_gap = g_param_spec_double (
				"bubble-vert-gap",
				"bubble-vert-gap",
				"Vert. gap between bubble and workarea edge (in em)",
				0.0f,
				10.0f,
				DEFAULT_BUBBLE_VERT_GAP,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_BUBBLE_VERT_GAP,
					 property_bubble_vert_gap);

    	property_bubble_horz_gap = g_param_spec_double (
				"bubble-horz-gap",
				"bubble-horz-gap",
				"Horz. gap between bubble and workarea edge (in em)",
				0.0f,
				10.0f,
				DEFAULT_BUBBLE_HORZ_GAP,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_BUBBLE_HORZ_GAP,
					 property_bubble_horz_gap);

	property_bubble_width = g_param_spec_double (
				"bubble-width",
				"bubble-width",
				"Width of bubble (in em)",
				0.0f,
				256.0f,
				DEFAULT_BUBBLE_WIDTH,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_BUBBLE_WIDTH,
					 property_bubble_width);

	property_bubble_min_height = g_param_spec_double (
				"bubble-min-height",
				"bubble-min-height",
				"Min. height of bubble (in em)",
				0.0f,
				256.0f,
				DEFAULT_BUBBLE_MIN_HEIGHT,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_BUBBLE_MIN_HEIGHT,
					 property_bubble_min_height);

	property_bubble_max_height = g_param_spec_double (
				"bubble-max-height",
				"bubble-max-height",
				"Max. height of bubble (in em)",
				0.0f,
				256.0f,
				DEFAULT_BUBBLE_MAX_HEIGHT,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_BUBBLE_MAX_HEIGHT,
					 property_bubble_max_height);

	property_bubble_shadow_size = g_param_spec_double (
				"bubble-shadow-size",
				"bubble-shadow-size",
				"Size (in em) of bubble drop-shadow",
				0.0f,
				32.0f,
				DEFAULT_BUBBLE_SHADOW_SIZE,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_BUBBLE_SHADOW_SIZE,
					 property_bubble_shadow_size);

	property_bubble_shadow_color = g_param_spec_string (
				"bubble-shadow-color",
				"bubble-shadow-color",
				"Color of bubble drop-shadow",
				DEFAULT_BUBBLE_SHADOW_COLOR,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_BUBBLE_SHADOW_COLOR,
					 property_bubble_shadow_color);

	property_bubble_bg_color = g_param_spec_string (
				"bubble-bg-color",
				"bubble-bg-color",
				"Color of bubble-background",
				DEFAULT_BUBBLE_BG_COLOR,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_BUBBLE_BG_COLOR,
					 property_bubble_bg_color);

	property_bubble_bg_opacity = g_param_spec_string (
				"bubble-bg-opacity",
				"bubble-bg-opacity",
				"Opacity of bubble-background",
				DEFAULT_BUBBLE_BG_OPACITY,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_BUBBLE_BG_OPACITY,
					 property_bubble_bg_opacity);

	property_bubble_hover_opacity = g_param_spec_string (
				"bubble-hover-opacity",
				"bubble-hover-opacity",
				"Opacity of bubble in mouse-over case",
				DEFAULT_BUBBLE_HOVER_OPACITY,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_BUBBLE_HOVER_OPACITY,
					 property_bubble_hover_opacity);

	property_bubble_corner_radius = g_param_spec_double (
				"bubble-corner-radius",
				"bubble-corner-radius",
				"Corner-radius of bubble (in em)",
				0.0f,
				16.0f,
				DEFAULT_BUBBLE_CORNER_RADIUS,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_BUBBLE_CORNER_RADIUS,
					 property_bubble_corner_radius);

	property_content_shadow_size = g_param_spec_double (
				"content-shadow-size",
				"content-shadow-size",
				"Size (in em) of icon/text drop-shadow",
				0.0f,
				8.0f,
				DEFAULT_CONTENT_SHADOW_SIZE,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_CONTENT_SHADOW_SIZE,
					 property_content_shadow_size);

	property_content_shadow_color = g_param_spec_string (
				"content-shadow-color",
				"content-shadow-color",
				"Color of icon/text drop-shadow",
				DEFAULT_CONTENT_SHADOW_COLOR,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_CONTENT_SHADOW_COLOR,
					 property_content_shadow_color);

	property_margin_size = g_param_spec_double (
				"margin-size",
				"margin-size",
				"Size (in em) of margin",
				0.0f,
				32.0f,
				DEFAULT_MARGIN_SIZE,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_MARGIN_SIZE,
					 property_margin_size);

	property_icon_size = g_param_spec_double (
				"icon-size",
				"icon-size",
				"Size (in em) of icon/avatar",
				0.0f,
				64.0f,
				DEFAULT_ICON_SIZE,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_ICON_SIZE,
					 property_icon_size);

	property_fade_in_timeout = g_param_spec_int (
				"fade-in-timeout",
				"fade-in-timeout",
				"Timeout for bubble fade-in in milliseconds",
				0,
				10000,
				DEFAULT_FADE_IN_TIMEOUT,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_FADE_IN_TIMEOUT,
					 property_fade_in_timeout);

	property_fade_out_timeout = g_param_spec_int (
				"fade-out-timeout",
				"fade-out-timeout",
				"Timeout for bubble fade-out in milliseconds",
				0,
				10000,
				DEFAULT_FADE_OUT_TIMEOUT,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_FADE_OUT_TIMEOUT,
					 property_fade_out_timeout);

	property_on_screen_timeout = g_param_spec_int (
				"on-screen-timeout",
				"on-screen-timeout",
				"Timeout for bubble on screen in milliseconds",
				0,
				10000,
				DEFAULT_ON_SCREEN_TIMEOUT,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_ON_SCREEN_TIMEOUT,
					 property_on_screen_timeout);

	property_text_font_face = g_param_spec_string (
				"text-font-face",
				"text-font-face",
				"Font-face to use of any rendered text",
				DEFAULT_TEXT_FONT_FACE,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_TEXT_FONT_FACE,
					 property_text_font_face);

	property_text_title_color = g_param_spec_string (
				"text-title-color",
				"text-title-color",
				"Color to use for content title-text",
				DEFAULT_TEXT_TITLE_COLOR,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_TEXT_TITLE_COLOR,
					 property_text_title_color);

	property_text_title_weight = g_param_spec_int (
				"text-title-weight",
				"text-title-weight",
				"Weight to use for content title-text",
				0,
				1000,
				DEFAULT_TEXT_TITLE_WEIGHT,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_TEXT_TITLE_WEIGHT,
					 property_text_title_weight);

	property_text_title_size = g_param_spec_double (
				"text-title-size",
				"text-title-size",
				"Size (in em) of font to use for content title-text",
				0.0f,
				32.0f,
				DEFAULT_TEXT_TITLE_SIZE,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_TEXT_TITLE_SIZE,
					 property_text_title_size);

	property_text_body_color = g_param_spec_string (
				"text-body-color",
				"text-body-color",
				"Color to use for content body-text",
				DEFAULT_TEXT_BODY_COLOR,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_TEXT_BODY_COLOR,
					 property_text_body_color);

	property_text_body_weight = g_param_spec_int (
				"text-body-weight",
				"text-body-weight",
				"Weight to use for content body-text",
				0,
				1000,
				DEFAULT_TEXT_BODY_WEIGHT,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_TEXT_BODY_WEIGHT,
					 property_text_body_weight);

	property_text_body_size = g_param_spec_double (
				"text-body-size",
				"text-body-size",
				"Size (in em) of font to use for content body-text",
				0.0f,
				32.0f,
				DEFAULT_TEXT_BODY_SIZE,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_TEXT_BODY_SIZE,
					 property_text_body_size);

	property_pixels_per_em = g_param_spec_int (
				"pixels-per-em",
				"pixels-per-em",
				"Number of pixels for one em-unit",
				1,
				100,
				DEFAULT_PIXELS_PER_EM,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_PIXELS_PER_EM,
					 property_pixels_per_em);
}

/*-- public API --------------------------------------------------------------*/

Defaults*
defaults_new (void)
{
	Defaults* this = g_object_new (DEFAULTS_TYPE, NULL);

	return this;
}

void
defaults_del (Defaults* self)
{
	g_object_unref (self);
}

gint
defaults_get_desktop_width (Defaults* self)
{
	gint width;

	if (!self || !IS_DEFAULTS (self))
		return 0;

	g_object_get (self, "desktop-width", &width, NULL);

	return width;
}

gint
defaults_get_desktop_height (Defaults* self)
{
	gint height;

	if (!self || !IS_DEFAULTS (self))
		return 0;

	g_object_get (self, "desktop-height", &height, NULL);

	return height;
}

gint
defaults_get_desktop_top (Defaults* self)
{
	gint top_edge;

	if (!self || !IS_DEFAULTS (self))
		return 0;

	g_object_get (self, "desktop-top", &top_edge, NULL);

	return top_edge;
}

gint
defaults_get_desktop_bottom (Defaults* self)
{
	gint bottom_edge;

	if (!self || !IS_DEFAULTS (self))
		return 0;

	g_object_get (self, "desktop-bottom", &bottom_edge, NULL);

	return bottom_edge;
}

gint
defaults_get_desktop_left (Defaults* self)
{
	gint left_edge;

	if (!self || !IS_DEFAULTS (self))
		return 0;

	g_object_get (self, "desktop-left", &left_edge, NULL);

	return left_edge;
}

gint
defaults_get_desktop_right (Defaults* self)
{
	gint right_edge;

	if (!self || !IS_DEFAULTS (self))
		return 0;

	g_object_get (self, "desktop-right", &right_edge, NULL);

	return right_edge;
}

gdouble
defaults_get_desktop_bottom_gap (Defaults* self)
{
	gdouble bottom_gap;

	if (!self || !IS_DEFAULTS (self))
		return 0.0f;

	g_object_get (self, "desktop-bottom-gap", &bottom_gap, NULL);

	return bottom_gap;
}

gdouble
defaults_get_stack_height (Defaults* self)
{
	gdouble stack_height;

	if (!self || !IS_DEFAULTS (self))
		return 0.0f;

	g_object_get (self, "stack-height", &stack_height, NULL);

	return stack_height;
}

gdouble
defaults_get_bubble_gap (Defaults* self)
{
	gdouble gap;

	if (!self || !IS_DEFAULTS (self))
		return 0.0f;

	g_object_get (self, "bubble-gap", &gap, NULL);

	return gap;
}

gdouble
defaults_get_bubble_width (Defaults* self)
{
	gdouble width;

	if (!self || !IS_DEFAULTS (self))
		return 0.0f;

	g_object_get (self, "bubble-width", &width, NULL);

	return width;
}

gdouble
defaults_get_bubble_min_height (Defaults* self)
{
	gdouble bubble_min_height;

	if (!self || !IS_DEFAULTS (self))
		return 0.0f;

	g_object_get (self, "bubble-min-height", &bubble_min_height, NULL);

	return bubble_min_height;
}

gdouble
defaults_get_bubble_max_height (Defaults* self)
{
	gdouble bubble_max_height;

	if (!self || !IS_DEFAULTS (self))
		return 0.0f;

	g_object_get (self, "bubble-max-height", &bubble_max_height, NULL);

	return bubble_max_height;
}

gdouble
defaults_get_bubble_vert_gap (Defaults* self)
{
	gdouble bubble_vert_gap;

	if (!self || !IS_DEFAULTS (self))
		return 0.0f;

	g_object_get (self, "bubble-vert-gap", &bubble_vert_gap, NULL);

	return bubble_vert_gap;
}

gdouble
defaults_get_bubble_horz_gap (Defaults* self)
{
	gdouble bubble_horz_gap;

	if (!self || !IS_DEFAULTS (self))
		return 0.0f;

	g_object_get (self, "bubble-horz-gap", &bubble_horz_gap, NULL);

	return bubble_horz_gap;
}

gdouble
defaults_get_bubble_height (Defaults* self)
{
	gdouble height;

	if (!self || !IS_DEFAULTS (self))
		return 0.0f;

	g_object_get (self, "bubble-height", &height, NULL);

	return height;
}

gdouble
defaults_get_bubble_shadow_size (Defaults* self)
{
	gdouble bubble_shadow_size;

	if (!self || !IS_DEFAULTS (self))
		return 0.0f;

	g_object_get (self, "bubble-shadow-size", &bubble_shadow_size, NULL);

	return bubble_shadow_size;
}

gchar*
defaults_get_bubble_shadow_color (Defaults* self)
{
	gchar* bubble_shadow_color = NULL;

	if (!self || !IS_DEFAULTS (self))
		return NULL;

	g_object_get (self,
		      "bubble-shadow-color",
		      &bubble_shadow_color,
		      NULL);

	return bubble_shadow_color;
}

gchar*
defaults_get_bubble_bg_color (Defaults* self)
{
	gchar* bubble_bg_color = NULL;

	if (!self || !IS_DEFAULTS (self))
		return NULL;

	g_object_get (self,
		      "bubble-bg-color",
		      &bubble_bg_color,
		      NULL);

	return bubble_bg_color;
}

gchar*
defaults_get_bubble_bg_opacity (Defaults* self)
{
	gchar* bubble_bg_opacity = NULL;

	if (!self || !IS_DEFAULTS (self))
		return NULL;

	g_object_get (self,
		      "bubble-bg-opacity",
		      &bubble_bg_opacity,
		      NULL);

	return bubble_bg_opacity;
}

gchar*
defaults_get_bubble_hover_opacity (Defaults* self)
{
	gchar* bubble_hover_opacity = NULL;

	if (!self || !IS_DEFAULTS (self))
		return NULL;

	g_object_get (self,
		      "bubble-hover-opacity",
		      &bubble_hover_opacity,
		      NULL);

	return bubble_hover_opacity;
}

gdouble
defaults_get_bubble_corner_radius (Defaults* self)
{
	gdouble bubble_corner_radius;

	if (!self || !IS_DEFAULTS (self))
		return 0.0f;

	g_object_get (self,
		      "bubble-corner-radius",
		      &bubble_corner_radius,
		      NULL);

	return bubble_corner_radius;
}

gdouble
defaults_get_content_shadow_size (Defaults* self)
{
	gdouble content_shadow_size;

	if (!self || !IS_DEFAULTS (self))
		return 0.0f;

	g_object_get (self, "content-shadow-size", &content_shadow_size, NULL);

	return content_shadow_size;
}

gchar*
defaults_get_content_shadow_color (Defaults* self)
{
	gchar* content_shadow_color = NULL;

	if (!self || !IS_DEFAULTS (self))
		return NULL;

	g_object_get (self,
		      "content-shadow-color",
		      &content_shadow_color,
		      NULL);

	return content_shadow_color;
}

gdouble
defaults_get_margin_size (Defaults* self)
{
	gdouble margin_size;

	if (!self || !IS_DEFAULTS (self))
		return 0.0f;

	g_object_get (self, "margin-size", &margin_size, NULL);

	return margin_size;
}

gdouble
defaults_get_icon_size (Defaults* self)
{
	gdouble icon_size;

	if (!self || !IS_DEFAULTS (self))
		return 0.0f;

	g_object_get (self, "icon-size", &icon_size, NULL);

	return icon_size;
}

gint
defaults_get_fade_in_timeout (Defaults* self)
{
	gint fade_in_timeout;

	if (!self || !IS_DEFAULTS (self))
		return 0;

	g_object_get (self, "fade-in-timeout", &fade_in_timeout, NULL);

	return fade_in_timeout;
}

gint
defaults_get_fade_out_timeout (Defaults* self)
{
	gint fade_out_timeout;

	if (!self || !IS_DEFAULTS (self))
		return 0;

	g_object_get (self, "fade-out-timeout", &fade_out_timeout, NULL);

	return fade_out_timeout;
}

gint
defaults_get_on_screen_timeout (Defaults* self)
{
	gint on_screen_timeout;

	if (!self || !IS_DEFAULTS (self))
		return 0;

	g_object_get (self, "on-screen-timeout", &on_screen_timeout, NULL);

	return on_screen_timeout;
}

gchar*
defaults_get_text_font_face (Defaults* self)
{
	gchar* text_font_face = NULL;

	if (!self || !IS_DEFAULTS (self))
		return NULL;

	g_object_get (self, "text-font-face", &text_font_face, NULL);

	return text_font_face;
}

gchar*
defaults_get_text_title_color (Defaults* self)
{
	gchar* text_title_color = NULL;

	if (!self || !IS_DEFAULTS (self))
		return NULL;

	g_object_get (self, "text-title-color", &text_title_color, NULL);

	return text_title_color;
}

gint
defaults_get_text_title_weight (Defaults* self)
{
	gint text_title_weight;

	if (!self || !IS_DEFAULTS (self))
		return 0;

	g_object_get (self, "text-title-weight", &text_title_weight, NULL);

	return text_title_weight;
}


gdouble
defaults_get_text_title_size (Defaults* self)
{
	gdouble text_title_size;

	if (!self || !IS_DEFAULTS (self))
		return 0.0f;

	g_object_get (self, "text-title-size", &text_title_size, NULL);

	return text_title_size;
}


gchar*
defaults_get_text_body_color (Defaults* self)
{
	gchar* text_body_color = NULL;

	if (!self || !IS_DEFAULTS (self))
		return NULL;

	g_object_get (self, "text-body-color", &text_body_color, NULL);

	return text_body_color;
}

gint
defaults_get_text_body_weight (Defaults* self)
{
	gint text_body_weight;

	if (!self || !IS_DEFAULTS (self))
		return 0;

	g_object_get (self, "text-body-weight", &text_body_weight, NULL);

	return text_body_weight;
}

gdouble
defaults_get_text_body_size (Defaults* self)
{
	gdouble text_body_size;

	if (!self || !IS_DEFAULTS (self))
		return 0.0f;

	g_object_get (self, "text-body-size", &text_body_size, NULL);

	return text_body_size;
}

/* we use the normal font-height in pixels ("pixels-per-em") as the measurement
 * for 1 em, thus it should _not_ be called before defaults_constructed() */
gint
defaults_get_pixel_per_em (Defaults* self)
{
	if (!self || !IS_DEFAULTS (self))
		return 0;

	gint pixels_per_em;

	g_object_get (self, "pixels-per-em", &pixels_per_em, NULL);

	return pixels_per_em;
}

static gboolean
defaults_multihead_does_focus_follow (Defaults *self)
{
	GError *error = NULL;
	gboolean mode = FALSE;

	g_return_val_if_fail (self != NULL && IS_DEFAULTS (self), FALSE);

	gchar *mode_str = gconf_client_get_string (self->context,
						   GCONF_MULTIHEAD_MODE,
						   &error);
	if (mode_str != NULL)
	{
		if (! g_strcmp0 (mode_str, "focus-follow"))
			mode = TRUE;
	} else if (error != NULL)
		g_warning ("error getting multihead mode: %s\n",
			   error->message);
	
	return mode;
}

static gboolean
_window_look_for_top_panel_attributes (GdkWindow *win)
{
	XClassHint class_hints = {0, 0};
	gboolean is_panel = FALSE;
	GdkRectangle frame;
	int result;

	if (win == NULL) return FALSE;

	gdk_error_trap_push ();

	result = XGetClassHint (GDK_DISPLAY (),
				GDK_WINDOW_XWINDOW (win),
				&class_hints);

	if (! result || class_hints.res_class == NULL)
		goto failed;

	if (g_strcmp0 (class_hints.res_name, "gnome-panel"))
		goto failed;

	/* discard dialog windows like panel properties or the applet directory... */
	if (gdk_window_get_type_hint (win)
	    != GDK_WINDOW_TYPE_HINT_DOCK)
		goto failed;

	/* select only the top panel */
	gdk_window_get_frame_extents (win, &frame);
	if (frame.x != 0 || frame.y != 0)
		goto failed;

	if (frame.width < frame.height)
		goto failed;
			
	is_panel = TRUE;

failed:
	if (class_hints.res_class)
		XFree (class_hints.res_class);
	if (class_hints.res_name)
		XFree (class_hints.res_name);

	gdk_error_trap_pop ();

	return is_panel;
}

static GdkWindow*
get_panel_window (void)
{
	GdkWindow *panel_window = NULL;
	GList     *window;
	GList     *iter;
	
	window = gdk_screen_get_window_stack (gdk_screen_get_default ());

	for (iter = g_list_first (window);
	     iter != NULL;
	     iter = g_list_next (iter))
	{
		if (_window_look_for_top_panel_attributes (iter->data))
		{
			panel_window = iter->data;
			break;
		}
	}
	
	g_list_free (window);

	return panel_window;
}

void
defaults_get_top_corner (Defaults *self, gint *x, gint *y)
{
	GdkRectangle rect;
	GdkRectangle panel_rect       = {0, 0, 0, 0};
	GdkScreen*   screen           = NULL;
	GdkWindow*   active_window    = NULL;
	GdkWindow*   panel_window     = NULL;
	gint         mx;
	gint         my;
	gint         monitor          = 0;
	gint         panel_monitor    = 0;
	gint         aw_monitor;
	gboolean     has_panel_window = FALSE;

	g_return_if_fail (self != NULL && IS_DEFAULTS (self));

	gdk_display_get_pointer (gdk_display_get_default (),
				 &screen,
	                         &mx,
	                         &my,
	                         NULL);

	panel_window = get_panel_window ();

	if (panel_window != NULL)
	{
		gdk_window_get_frame_extents (panel_window, &panel_rect);
		panel_monitor = gdk_screen_get_monitor_at_window (screen,
		                                                  panel_window);
		monitor = panel_monitor;
		g_debug ("found panel (%d,%d) - %dx%d on monitor %d",
			 panel_rect.x,
		         panel_rect.y,
			 panel_rect.width,
		         panel_rect.height,
		         monitor);

		has_panel_window  = TRUE;
	}

	if (defaults_multihead_does_focus_follow (self))
	{
		g_debug ("multi_head_focus_follow mode");
		monitor = gdk_screen_get_monitor_at_point (screen, mx, my);
		active_window = gdk_screen_get_active_window (screen);
		if (active_window != NULL)
		{
			aw_monitor = gdk_screen_get_monitor_at_window (
					screen,
			                active_window);

			if (monitor != aw_monitor)
			{
				g_debug ("choosing the monitor with the active"
				         " window, not the one with the mouse"
				         " cursor");
			}

			monitor = aw_monitor;

			g_object_unref (active_window);
		}
	}

	gdk_screen_get_monitor_geometry (screen, monitor, &rect);
	g_debug ("selecting monitor %d at (%d,%d) - %dx%d",
		 monitor,
	         rect.x,
	         rect.y,
	         rect.width,
	         rect.height);

	/* Position the top left corner of the stack. */
	if (has_panel_window &&
	    panel_monitor == monitor)
	{
		/* position the corner on the selected monitor */
		rect.y += panel_rect.y + panel_rect.height;
	} else if (! has_panel_window)
	{
		g_debug ("no panel detetected; using workarea fallback");

		defaults_refresh_screen_dimension_properties (self);

		/* workarea rectangle */
		g_object_get (self, "desktop-left", &rect.x, NULL);
		g_object_get (self, "desktop-top", &rect.y, NULL);
		g_object_get (self, "desktop-width", &rect.width, NULL);
		g_object_get (self, "desktop-height", &rect.height, NULL);
 	}

	*y   = rect.y;
	*y  += EM2PIXELS (defaults_get_bubble_vert_gap (self), self)
	       - EM2PIXELS (defaults_get_bubble_shadow_size (self), self);

	if (gtk_widget_get_default_direction () == GTK_TEXT_DIR_LTR)
	{
		*x = rect.x + rect.width;
		*x -= EM2PIXELS (defaults_get_bubble_shadow_size (self), self)
			+ EM2PIXELS (defaults_get_bubble_horz_gap (self), self)
			+ EM2PIXELS (defaults_get_bubble_width (self), self);
	} else {
		*x = rect.x
			- EM2PIXELS (defaults_get_bubble_shadow_size (self), self)
			+ EM2PIXELS (defaults_get_bubble_horz_gap (self), self);
	}

	g_debug ("top corner at: %d, %d", *x, *y);
}
