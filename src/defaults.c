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
	PROP_TEXT_BODY_SIZE
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

/* these values are interpreted as pixel-measurements and do comply to the 
 * visual guide for jaunty-notifications */
#define DEFAULT_BUBBLE_WIDTH         226
#define DEFAULT_BUBBLE_MIN_HEIGHT    64
#define DEFAULT_BUBBLE_MAX_HEIGHT    200
#define DEFAULT_BUBBLE_VERT_GAP      7
#define DEFAULT_BUBBLE_HORZ_GAP      5
#define DEFAULT_BUBBLE_SHADOW_SIZE   8
#define DEFAULT_BUBBLE_SHADOW_COLOR  "#000000"
#define DEFAULT_BUBBLE_BG_COLOR      "#131313"
#define DEFAULT_BUBBLE_BG_OPACITY    "#cc"
#define DEFAULT_BUBBLE_HOVER_OPACITY "#66"
#define DEFAULT_BUBBLE_CORNER_RADIUS 6
#define DEFAULT_CONTENT_SHADOW_SIZE  2
#define DEFAULT_CONTENT_SHADOW_COLOR "#000000"
#define DEFAULT_MARGIN_SIZE          16
#define DEFAULT_ICON_SIZE            32
#define DEFAULT_TEXT_FONT_FACE       "Sans"
#define DEFAULT_TEXT_TITLE_COLOR     "#ffffff"
#define DEFAULT_TEXT_TITLE_WEIGHT    TEXT_WEIGHT_BOLD
#define DEFAULT_TEXT_TITLE_SIZE      120
#define DEFAULT_TEXT_BODY_COLOR      "#eaeaea"
#define DEFAULT_TEXT_BODY_WEIGHT     TEXT_WEIGHT_NORMAL
#define DEFAULT_TEXT_BODY_SIZE       100

/* these values are interpreted as milliseconds-measurements and do comply to
 * the visual guide for jaunty-notifications */
#define DEFAULT_FADE_IN_TIMEOUT      250
#define DEFAULT_FADE_OUT_TIMEOUT     1000
#define DEFAULT_ON_SCREEN_TIMEOUT    5000

#define GCONF_FONT_KEY "/desktop/gnome/interface/font_name"

/*-- internal API ------------------------------------------------------------*/

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
	GConfClient* context   = NULL;
	GString*     string    = NULL;
	GError*      error     = NULL;
	GString*     font_face = NULL;
	gint         font_size = 0;

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
		g_object_set (self,
			      "stack-height",
			      (gint) coords[3] / 2,
			      NULL);
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
						font_size = (gint) scanner->value.v_int;
					break;

					case G_TOKEN_IDENTIFIER:
						if (!font_face)
							font_face = g_string_new (scanner->value.v_string);
						else
						{
							g_string_append (font_face, " ");
							g_string_append (font_face, scanner->value.v_string);
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
		g_object_set (self,
			      "text-title-size",
			      (gint) (1.2f * font_size),
			      NULL);
		g_object_set (self,
			      "text-body-size",
			      font_size,
			      NULL);

		if (font_face != NULL)
			g_string_free (font_face, TRUE);
	}

	g_object_unref (context);

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

		case PROP_STACK_HEIGHT:
			g_value_set_int (value, defaults->stack_height);
		break;

		case PROP_BUBBLE_VERT_GAP:
			g_value_set_int (value, defaults->bubble_vert_gap);
		break;

		case PROP_BUBBLE_HORZ_GAP:
			g_value_set_int (value, defaults->bubble_horz_gap);
		break;

		case PROP_BUBBLE_WIDTH:
			g_value_set_int (value, defaults->bubble_width);
		break;

		case PROP_BUBBLE_MIN_HEIGHT:
			g_value_set_int (value, defaults->bubble_min_height);
		break;

		case PROP_BUBBLE_MAX_HEIGHT:
			g_value_set_int (value, defaults->bubble_max_height);
		break;

		case PROP_BUBBLE_SHADOW_SIZE:
			g_value_set_int (value, defaults->bubble_shadow_size);
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
			g_value_set_int (value, defaults->bubble_corner_radius);
		break;

		case PROP_CONTENT_SHADOW_SIZE:
			g_value_set_int (value, defaults->content_shadow_size);
		break;

		case PROP_CONTENT_SHADOW_COLOR:
			g_value_set_string (value,
					    defaults->content_shadow_color->str);
		break;

		case PROP_MARGIN_SIZE:
			g_value_set_int (value, defaults->margin_size);
		break;

		case PROP_ICON_SIZE:
			g_value_set_int (value, defaults->icon_size);
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
			g_value_set_int (value, defaults->text_title_size);
		break;

		case PROP_TEXT_BODY_COLOR:
			g_value_set_string (value,
					    defaults->text_body_color->str);
		break;

		case PROP_TEXT_BODY_WEIGHT:
			g_value_set_int (value, defaults->text_body_weight);
		break;

		case PROP_TEXT_BODY_SIZE:
			g_value_set_int (value, defaults->text_body_size);
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

		case PROP_STACK_HEIGHT:
			defaults->stack_height = g_value_get_int (value);
		break;

		case PROP_BUBBLE_WIDTH:
			defaults->bubble_width = g_value_get_int (value);
		break;

		case PROP_BUBBLE_VERT_GAP:
			defaults->bubble_vert_gap = g_value_get_int (value);
		break;

		case PROP_BUBBLE_HORZ_GAP:
			defaults->bubble_horz_gap = g_value_get_int (value);
		break;

		case PROP_BUBBLE_MIN_HEIGHT:
			defaults->bubble_min_height = g_value_get_int (value);
		break;

		case PROP_BUBBLE_MAX_HEIGHT:
			defaults->bubble_max_height = g_value_get_int (value);
		break;

		case PROP_BUBBLE_SHADOW_SIZE:
			defaults->bubble_shadow_size = g_value_get_int (value);
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
			defaults->bubble_corner_radius = g_value_get_int (value);
		break;

		case PROP_CONTENT_SHADOW_SIZE:
			defaults->content_shadow_size = g_value_get_int (value);
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
			defaults->margin_size = g_value_get_int (value);
		break;

		case PROP_ICON_SIZE:
			defaults->icon_size = g_value_get_int (value);
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
			defaults->text_title_size = g_value_get_int (value);
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
			defaults->text_body_size = g_value_get_int (value);
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

	property_stack_height = g_param_spec_int (
				"stack-height",
				"stack-height",
				"Maximum allowed height of stack",
				0,
				4096,
				2048,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_STACK_HEIGHT,
					 property_stack_height);

    	property_bubble_vert_gap = g_param_spec_int (
				"bubble-vert-gap",
				"bubble-vert-gap",
				"Vert. gap between bubble and workarea edge",
				0,
				1024,
				DEFAULT_BUBBLE_VERT_GAP,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_BUBBLE_VERT_GAP,
					 property_bubble_vert_gap);

    	property_bubble_horz_gap = g_param_spec_int (
				"bubble-horz-gap",
				"bubble-horz-gap",
				"Horz. gap between bubble and workarea edge",
				0,
				1024,
				DEFAULT_BUBBLE_HORZ_GAP,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_BUBBLE_HORZ_GAP,
					 property_bubble_horz_gap);

	property_bubble_width = g_param_spec_int (
				"bubble-width",
				"bubble-width",
				"Width of bubble in pixels",
				0,
				1024,
				DEFAULT_BUBBLE_WIDTH,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_BUBBLE_WIDTH,
					 property_bubble_width);

	property_bubble_min_height = g_param_spec_int (
				"bubble-min-height",
				"bubble-min-height",
				"Min. height of bubble in pixels",
				0,
				1024,
				DEFAULT_BUBBLE_MIN_HEIGHT,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_BUBBLE_MIN_HEIGHT,
					 property_bubble_min_height);

	property_bubble_max_height = g_param_spec_int (
				"bubble-max-height",
				"bubble-max-height",
				"Max. height of bubble in pixels",
				0,
				1024,
				DEFAULT_BUBBLE_MAX_HEIGHT,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_BUBBLE_MAX_HEIGHT,
					 property_bubble_max_height);

	property_bubble_shadow_size = g_param_spec_int (
				"bubble-shadow-size",
				"bubble-shadow-size",
				"Size of bubble drop-shadow in pixels",
				0,
				1024,
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

	property_bubble_corner_radius = g_param_spec_int (
				"bubble-corner-radius",
				"bubble-corner-radius",
				"Corner-radius of bubble in pixels",
				0,
				1024,
				DEFAULT_BUBBLE_CORNER_RADIUS,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_BUBBLE_CORNER_RADIUS,
					 property_bubble_corner_radius);

	property_content_shadow_size = g_param_spec_int (
				"content-shadow-size",
				"content-shadow-size",
				"Size of icon/text drop-shadow in pixels",
				0,
				1024,
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

	property_margin_size = g_param_spec_int (
				"margin-size",
				"margin-size",
				"Size of margin in pixels",
				0,
				1024,
				DEFAULT_MARGIN_SIZE,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_MARGIN_SIZE,
					 property_margin_size);

	property_icon_size = g_param_spec_int (
				"icon-size",
				"icon-size",
				"Size of icon/avatar in pixels",
				0,
				1024,
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

	property_text_title_size = g_param_spec_int (
				"text-title-size",
				"text-title-size",
				"Size of font to use for content title-text",
				0,
				1000,
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

	property_text_body_size = g_param_spec_int (
				"text-body-size",
				"text-body-size",
				"Size of font to use for content body-text",
				0,
				1000,
				DEFAULT_TEXT_BODY_SIZE,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_TEXT_BODY_SIZE,
					 property_text_body_size);
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

	g_object_get (self, "desktop-width", &width, NULL);

	return width;
}

gint
defaults_get_desktop_height (Defaults* self)
{
	gint height;

	g_object_get (self, "desktop-height", &height, NULL);

	return height;
}

gint
defaults_get_desktop_top (Defaults* self)
{
	gint top_edge;

	g_object_get (self, "desktop-top", &top_edge, NULL);

	return top_edge;
}

gint
defaults_get_desktop_bottom (Defaults* self)
{
	gint bottom_edge;

	g_object_get (self, "desktop-bottom", &bottom_edge, NULL);

	return bottom_edge;
}

gint
defaults_get_desktop_left (Defaults* self)
{
	gint left_edge;

	g_object_get (self, "desktop-left", &left_edge, NULL);

	return left_edge;
}

gint
defaults_get_desktop_right (Defaults* self)
{
	gint right_edge;

	g_object_get (self, "desktop-right", &right_edge, NULL);

	return right_edge;
}

gint
defaults_get_stack_height (Defaults* self)
{
	gint stack_height;

	g_object_get (self, "stack-height", &stack_height, NULL);

	return stack_height;
}

gint
defaults_get_bubble_gap (Defaults* self)
{
	gint gap;

	g_object_get (self, "bubble-gap", &gap, NULL);

	return gap;
}

gint
defaults_get_bubble_width (Defaults* self)
{
	gint width;

	g_object_get (self, "bubble-width", &width, NULL);

	return width;
}

gint
defaults_get_bubble_min_height (Defaults* self)
{
	gint bubble_min_height;

	g_object_get (self, "bubble-min-height", &bubble_min_height, NULL);

	return bubble_min_height;
}

gint
defaults_get_bubble_max_height (Defaults* self)
{
	gint bubble_max_height;

	g_object_get (self, "bubble-max-height", &bubble_max_height, NULL);

	return bubble_max_height;
}

gint
defaults_get_bubble_vert_gap (Defaults* self)
{
	gint bubble_vert_gap;

	g_object_get (self, "bubble-vert-gap", &bubble_vert_gap, NULL);

	return bubble_vert_gap;
}

gint
defaults_get_bubble_horz_gap (Defaults* self)
{
	gint bubble_horz_gap;

	g_object_get (self, "bubble-horz-gap", &bubble_horz_gap, NULL);

	return bubble_horz_gap;
}

gint
defaults_get_bubble_height (Defaults* self)
{
	gint height;

	g_object_get (self, "bubble-height", &height, NULL);

	return height;
}

gint
defaults_get_bubble_shadow_size (Defaults* self)
{
	gint bubble_shadow_size;

	g_object_get (self, "bubble-shadow-size", &bubble_shadow_size, NULL);

	return bubble_shadow_size;
}

gchar*
defaults_get_bubble_shadow_color (Defaults* self)
{
	gchar* bubble_shadow_color = NULL;

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

	g_object_get (self,
		      "bubble-hover-opacity",
		      &bubble_hover_opacity,
		      NULL);

	return bubble_hover_opacity;
}

gint
defaults_get_bubble_corner_radius (Defaults* self)
{
	gint bubble_corner_radius;

	g_object_get (self,
		      "bubble-corner-radius",
		      &bubble_corner_radius,
		      NULL);

	return bubble_corner_radius;
}

gint
defaults_get_content_shadow_size (Defaults* self)
{
	gint content_shadow_size;

	g_object_get (self, "content-shadow-size", &content_shadow_size, NULL);

	return content_shadow_size;
}

gchar*
defaults_get_content_shadow_color (Defaults* self)
{
	gchar* content_shadow_color = NULL;

	g_object_get (self,
		      "content-shadow-color",
		      &content_shadow_color,
		      NULL);

	return content_shadow_color;
}

gint
defaults_get_margin_size (Defaults* self)
{
	gint margin_size;

	g_object_get (self, "margin-size", &margin_size, NULL);

	return margin_size;
}

gint
defaults_get_icon_size (Defaults* self)
{
	gint icon_size;

	g_object_get (self, "icon-size", &icon_size, NULL);

	return icon_size;
}

gint
defaults_get_fade_in_timeout (Defaults* self)
{
	gint fade_in_timeout;

	g_object_get (self, "fade-in-timeout", &fade_in_timeout, NULL);

	return fade_in_timeout;
}

gint
defaults_get_fade_out_timeout (Defaults* self)
{
	gint fade_out_timeout;

	g_object_get (self, "fade-out-timeout", &fade_out_timeout, NULL);

	return fade_out_timeout;
}

gint
defaults_get_on_screen_timeout (Defaults* self)
{
	gint on_screen_timeout;

	g_object_get (self, "on-screen-timeout", &on_screen_timeout, NULL);

	return on_screen_timeout;
}

gchar*
defaults_get_text_font_face (Defaults* self)
{
	gchar* text_font_face = NULL;

	g_object_get (self, "text-font-face", &text_font_face, NULL);

	return text_font_face;
}

gchar*
defaults_get_text_title_color (Defaults* self)
{
	gchar* text_title_color = NULL;

	g_object_get (self, "text-title-color", &text_title_color, NULL);

	return text_title_color;
}

gint
defaults_get_text_title_weight (Defaults* self)
{
	gint text_title_weight;

	g_object_get (self, "text-title-weight", &text_title_weight, NULL);

	return text_title_weight;
}


gint
defaults_get_text_title_size (Defaults* self)
{
	gint text_title_size;

	g_object_get (self, "text-title-size", &text_title_size, NULL);

	return text_title_size;
}


gchar*
defaults_get_text_body_color (Defaults* self)
{
	gchar* text_body_color = NULL;

	g_object_get (self, "text-body-color", &text_body_color, NULL);

	return text_body_color;
}

gint
defaults_get_text_body_weight (Defaults* self)
{
	gint text_body_weight;

	g_object_get (self, "text-body-weight", &text_body_weight, NULL);

	return text_body_weight;
}


gint
defaults_get_text_body_size (Defaults* self)
{
	gint text_body_size;

	g_object_get (self, "text-body-size", &text_body_size, NULL);

	return text_body_size;
}
