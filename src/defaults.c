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
	PROP_BUBBLE_GAP,
	PROP_BUBBLE_WIDTH,
	PROP_BUBBLE_HEIGHT,
	PROP_BUBBLE_OPACITY,
	PROP_BUBBLE_RED,
	PROP_BUBBLE_GREEN,
	PROP_BUBBLE_BLUE,
	PROP_FONT_DPI,
	PROP_FONT_SIZE,
	PROP_FONT_FACE
};

enum
{
	R = 0,
	G,
	B
};

/*-- internal API ------------------------------------------------------------*/

static void
defaults_constructed (GObject* gobject)
{
	Atom      real_type;
	gint      result;
	gint      real_format;
	gulong    items_read;
	gulong    items_left;
	glong*    coords;
	Atom      workarea_atom;
	Defaults* self;

	self = DEFAULTS (gobject);

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
		g_object_set (self,
			      "bubble-width",
			      226 + 20, /* HACK: drop-shadow */
			      NULL);
		g_object_set (self,
			      "bubble-height",
			      64 + 20, /* HACK: drop-shadow */
			      NULL);
		XFree (coords);
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

		case PROP_STACK_HEIGHT:
			g_value_set_int (value, defaults->stack_height);
		break;

		case PROP_BUBBLE_GAP:
			g_value_set_int (value, defaults->bubble_gap);
		break;

		case PROP_BUBBLE_WIDTH:
			g_value_set_int (value, defaults->bubble_width);
		break;

		case PROP_BUBBLE_HEIGHT:
			g_value_set_int (value, defaults->bubble_height);
		break;

		case PROP_BUBBLE_OPACITY:
			g_value_set_double (value, defaults->bubble_opacity);
		break;

		case PROP_BUBBLE_RED:
			g_value_set_double (value, defaults->bubble_color[R]);
		break;

		case PROP_BUBBLE_GREEN:
			g_value_set_double (value, defaults->bubble_color[G]);
		break;

		case PROP_BUBBLE_BLUE:
			g_value_set_double (value, defaults->bubble_color[B]);
		break;

		case PROP_FONT_DPI:
			g_value_set_double (value, defaults->font_dpi);
		break;

		case PROP_FONT_SIZE:
			g_value_set_double (value, defaults->font_size);
		break;

		case PROP_FONT_FACE:
			g_value_set_string (value, defaults->font_face->str);
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

		case PROP_BUBBLE_GAP:
			defaults->bubble_gap = g_value_get_int (value);
		break;

		case PROP_BUBBLE_HEIGHT:
			defaults->bubble_height = g_value_get_int (value);
		break;

		case PROP_BUBBLE_OPACITY:
			defaults->bubble_opacity = g_value_get_double (value);
		break;

		case PROP_BUBBLE_RED:
			defaults->bubble_color[R] = g_value_get_double (value);
		break;

		case PROP_BUBBLE_GREEN:
			defaults->bubble_color[G] = g_value_get_double (value);
		break;

		case PROP_BUBBLE_BLUE:
			defaults->bubble_color[B] = g_value_get_double (value);
		break;

		case PROP_FONT_DPI:
			defaults->font_dpi = g_value_get_double (value);
		break;

		case PROP_FONT_SIZE:
			defaults->font_size = g_value_get_double (value);
		break;

		case PROP_FONT_FACE:
			defaults->font_face = g_string_new (g_value_get_string
								(value));
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

	GdkScreen*    screen        = gdk_screen_get_default ();
	GParamSpec*   property_desktop_width;
	GParamSpec*   property_desktop_height;
	GParamSpec*   property_desktop_top;
	GParamSpec*   property_desktop_bottom;
	GParamSpec*   property_desktop_left;
	GParamSpec*   property_desktop_right;
	GParamSpec*   property_stack_height;
	GParamSpec*   property_bubble_gap;
	GParamSpec*   property_bubble_width;
	GParamSpec*   property_bubble_height;
	GParamSpec*   property_bubble_opacity;
	GParamSpec*   property_bubble_red;
	GParamSpec*   property_bubble_green;
	GParamSpec*   property_bubble_blue;
	GParamSpec*   property_font_dpi;
	GParamSpec*   property_font_size;
	GParamSpec*   property_font_face;

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

    	property_bubble_gap = g_param_spec_int (
				"bubble-gap",
				"bubble-gap",
				"Gap between bubbles and the workarea edges",
				0,
				50,
				7,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_BUBBLE_GAP,
					 property_bubble_gap);

	property_bubble_width = g_param_spec_int (
				"bubble-width",
				"bubble-width",
				"Width of bubble in pixels",
				0,
				1024,
				226,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_BUBBLE_WIDTH,
					 property_bubble_width);

	property_bubble_height = g_param_spec_int (
				"bubble-height",
				"bubble-height",
				"Height of bubble in pixels",
				50,
				300,
				64,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_BUBBLE_HEIGHT,
					 property_bubble_height);

	property_bubble_opacity = g_param_spec_double (
				"bubble-opacity",
				"bubble-opacity",
				"Opacity of bubble",
				0.1f,
				1.0f,
				0.70f,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_BUBBLE_OPACITY,
					 property_bubble_opacity);

	property_bubble_red = g_param_spec_double (
				"bubble-color-red",
				"bubble-color-red",
				"Red-component of bubble color",
				0.0f,
				1.0f,
				0.15f,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_BUBBLE_RED,
					 property_bubble_red);

	property_bubble_green = g_param_spec_double (
				"bubble-color-green",
				"bubble-color-green",
				"Green-component of bubble color",
				0.0f,
				1.0f,
				0.15f,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_BUBBLE_GREEN,
					 property_bubble_green);

	property_bubble_blue = g_param_spec_double (
				"bubble-color-blue",
				"bubble-color-blue",
				"Blue-component of bubble color",
				0.0f,
				1.0f,
				0.15f,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_BUBBLE_BLUE,
					 property_bubble_blue);

	property_font_dpi = g_param_spec_double (
				"font-dpi",
				"font-dpi",
				"DPI to use for font-display",
				72.0f,
				300.0f,
				72.0f,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_FONT_DPI,
					 property_font_dpi);

	property_font_size = g_param_spec_double (
				"font-size",
				"font-size",
				"Size/height of the font",
				5.0f,
				144.0f,
				10.0f,
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_FONT_SIZE,
					 property_font_size);

	property_font_face = g_param_spec_string (
				"font-face",
				"font-face",
				"Face of the font",
				"DejaVu Sans",
				G_PARAM_CONSTRUCT |
				G_PARAM_READWRITE);
	g_object_class_install_property (gobject_class,
					 PROP_FONT_FACE,
					 property_font_face);
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
defaults_get_bubble_height (Defaults* self)
{
	gint height;

	g_object_get (self, "bubble-height", &height, NULL);

	return height;
}

gdouble
defaults_get_bubble_opacity (Defaults* self)
{
	gdouble opacity;

	g_object_get (self, "bubble-opacity", &opacity, NULL);

	return opacity;
}

void
defaults_get_bubble_color (Defaults* self,
			   gdouble*  red,
			   gdouble*  green,
			   gdouble*  blue)
{
	if (!red || !green || !blue)
		return;

	g_object_get (self, "bubble-color-red", red, NULL);
	g_object_get (self, "bubble-color-green", green, NULL);
	g_object_get (self, "bubble-color-blue", blue, NULL);
}

gdouble
defaults_get_font_dpi (Defaults* self)
{
	gdouble dpi;

	g_object_get (self, "font-dpi", &dpi, NULL);

	return dpi;
}

gdouble
defaults_get_font_size (Defaults* self)
{
	gdouble size;

	g_object_get (self, "font-size", &size, NULL);

	return size;
}

gchar*
defaults_get_font_face (Defaults* self)
{
	gchar* face = NULL;

	g_object_get (self, "font-face", &face, NULL);

	return face;
}

