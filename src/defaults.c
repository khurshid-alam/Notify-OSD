/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
**      10        20        30        40        50        60        70        80
**
** project:
**    alsdorf
**
** file:
**    defaults.c
**
** author(s):
**    Mirco "MacSlow" Mueller <mirco.mueller@canonical.com>
**    David Barth <david.barth@canonical.com>
**
** copyright (C) Canonical, oct. 2008
**
*******************************************************************************/

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gconf/gconf-client.h>

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
#define DEFAULT_BUBBLE_WIDTH         18.0f
#define DEFAULT_BUBBLE_MIN_HEIGHT     5.0f
#define DEFAULT_BUBBLE_MAX_HEIGHT    12.2f
#define DEFAULT_BUBBLE_VERT_GAP       0.5f
#define DEFAULT_BUBBLE_HORZ_GAP       0.5f
#define DEFAULT_BUBBLE_SHADOW_SIZE    0.5f
#define DEFAULT_BUBBLE_SHADOW_COLOR  "#000000"
#define DEFAULT_BUBBLE_BG_COLOR      "#131313"
#define DEFAULT_BUBBLE_BG_OPACITY    "#cc"
#define DEFAULT_BUBBLE_HOVER_OPACITY "#66"
#define DEFAULT_BUBBLE_CORNER_RADIUS 0.375f
#define DEFAULT_CONTENT_SHADOW_SIZE  0.125f
#define DEFAULT_CONTENT_SHADOW_COLOR "#000000"
#define DEFAULT_MARGIN_SIZE          1.0f
#define DEFAULT_ICON_SIZE            3.0f
#define DEFAULT_TEXT_FONT_FACE       "Sans"
#define DEFAULT_TEXT_TITLE_COLOR     "#ffffff"
#define DEFAULT_TEXT_TITLE_WEIGHT    TEXT_WEIGHT_BOLD
#define DEFAULT_TEXT_TITLE_SIZE      1.2f
#define DEFAULT_TEXT_BODY_COLOR      "#eaeaea"
#define DEFAULT_TEXT_BODY_WEIGHT     TEXT_WEIGHT_NORMAL
#define DEFAULT_TEXT_BODY_SIZE       1.0f
#define DEFAULT_PIXELS_PER_EM        10

/* these values are interpreted as milliseconds-measurements and do comply to
 * the visual guide for jaunty-notifications */
#define DEFAULT_FADE_IN_TIMEOUT      250
#define DEFAULT_FADE_OUT_TIMEOUT     1000
#define DEFAULT_ON_SCREEN_TIMEOUT    5000

#define GCONF_FONT_KEY "/desktop/gnome/interface/font_name"
#define GCONF_DPI_KEY "/desktop/gnome/font_rendering/dpi"

/*-- internal API ------------------------------------------------------------*/

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

static void
defaults_constructed (GObject* gobject)
{
	Atom         real_type;
	gint         result;
	gint         real_format;
	gulong       items_read;
	gulong       items_left;
	glong*       coords;
	Atom         workarea_atom;
	Defaults*    self;
	GConfClient* context       = NULL;
	GString*     string        = NULL;
	gdouble      dpi           = 0.0f;
	GError*      error         = NULL;
	GString*     font_face     = NULL;
	gint         points        = 0;
	gint         pixels_per_em = 0;
	gdouble      margin_size;
	gdouble      icon_size;
	gdouble      bubble_height;
	gdouble      new_bubble_height;
	gdouble      bubble_width;
	gdouble      new_bubble_width;
	gdouble      average_char_width;

	self = DEFAULTS (gobject);

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

	/* grab default font-face and size from gconf */
	context = gconf_client_get_default ();
	if (!context)
		return;

	string = g_string_new (gconf_client_get_string (context,
							GCONF_FONT_KEY,
							&error));

	if (error)
	{
		g_object_unref (context);
		return;
	}
	else
	{
		GScanner*  scanner = NULL;
		GTokenType token   = G_TOKEN_NONE;

		/* extract font-family-name and font-size */
		scanner = g_scanner_new (NULL);
		if (scanner)
		{
			g_scanner_input_text (scanner,
					      string->str,
					      string->len);
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

		if (string != NULL)
			g_string_free (string, TRUE);

		g_object_set (self,
			      "text-font-face",
			      font_face->str,
			      NULL);

		if (font_face != NULL)
			g_string_free (font_face, TRUE);
	}

	dpi = gconf_client_get_float (context, GCONF_DPI_KEY, &error);
	if (error)
	{
		g_object_unref (context);
		return;
	}
	else
	{
		pixels_per_em = (gint) ((gdouble) points *
					1.0f / 72.0f * dpi);
		g_debug ("pixels-per-em: %d\n", pixels_per_em);
		g_object_set (self,
			      "pixels-per-em",
			      pixels_per_em,
			      NULL);
	}

	g_object_unref (context);

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
