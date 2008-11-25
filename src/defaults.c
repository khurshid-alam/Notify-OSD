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

#include <gdk/gdk.h>

#include "defaults.h"

G_DEFINE_TYPE (Defaults, defaults, G_TYPE_OBJECT);

enum
{
	PROP_DUMMY = 0,
	PROP_INITIALIZED,
	PROP_DESKTOP_WIDTH,
	PROP_DESKTOP_HEIGHT,
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
	GdkScreen* screen = gdk_screen_get_default ();

	/* here we assume gtk_init() has been issued before by the caller */
	self->initialized     = TRUE;
	self->desktop_width   = gdk_screen_get_width (screen);
	self->desktop_height  = gdk_screen_get_height (screen);
	self->bubble_width    = self->desktop_width / 5;
	self->bubble_height   = self->desktop_height / 10;
	self->bubble_opacity  = 0.85f;
	self->bubble_color[R] = 0.15f;
	self->bubble_color[G] = 0.15f;
	self->bubble_color[B] = 0.15f;
	self->font_dpi        = 72.0f;
	self->font_size       = 10.0f;
	self->font_face       = g_string_new ("DejaVu Sans");

	/* If you need specific construction properties to complete
	** initialization, delay initialization completion until the
	** property is set. */
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
		case PROP_INITIALIZED:
			g_value_set_boolean (value, defaults->initialized);
		break;

		case PROP_DESKTOP_WIDTH:
			g_value_set_int (value, defaults->desktop_width);
		break;

		case PROP_DESKTOP_HEIGHT:
			g_value_set_int (value, defaults->desktop_height);
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
defaults_class_init (DefaultsClass* klass)
{
	GObjectClass* gobject_class = G_OBJECT_CLASS (klass);
	GParamSpec*   property_initialized;
	GParamSpec*   property_desktop_width;
	GParamSpec*   property_desktop_height;
	GParamSpec*   property_bubble_width;
	GParamSpec*   property_bubble_height;
	GParamSpec*   property_bubble_opacity;
	GParamSpec*   property_bubble_red;
	GParamSpec*   property_bubble_green;
	GParamSpec*   property_bubble_blue;
	GParamSpec*   property_font_dpi;
	GParamSpec*   property_font_size;
	GParamSpec*   property_font_face;

	gobject_class->dispose      = defaults_dispose;
	gobject_class->finalize     = defaults_finalize;
	gobject_class->get_property = defaults_get_property;

	property_initialized = g_param_spec_boolean (
				"initialized",
				"initialized",
				"Defaults already initialized?",
				FALSE,
				G_PARAM_CONSTRUCT |
				G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
					 PROP_INITIALIZED,
					 property_initialized);

	property_desktop_width = g_param_spec_int (
				"desktop-width",
				"desktop-width",
				"Width of desktop in pixels",
				640,
				4096,
				800,
				G_PARAM_CONSTRUCT |
				G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
					 PROP_DESKTOP_WIDTH,
					 property_desktop_width);

	property_desktop_height = g_param_spec_int (
				"desktop-height",
				"desktop-height",
				"Height of desktop in pixels",
				600,
				4096,
				600,
				G_PARAM_CONSTRUCT |
				G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
					 PROP_DESKTOP_HEIGHT,
					 property_desktop_height);

	property_bubble_width = g_param_spec_int (
				"bubble-width",
				"bubble-width",
				"Width of bubble in pixels",
				200,
				1024,
				300,
				G_PARAM_CONSTRUCT |
				G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
					 PROP_BUBBLE_WIDTH,
					 property_bubble_width);

	property_bubble_height = g_param_spec_int (
				"bubble-height",
				"bubble-height",
				"Height of bubble in pixels",
				50,
				300,
				100,
				G_PARAM_CONSTRUCT |
				G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
					 PROP_BUBBLE_HEIGHT,
					 property_bubble_height);

	property_bubble_opacity = g_param_spec_double (
				"bubble-opacity",
				"bubble-opacity",
				"Opacity of bubble",
				0.1f,
				1.0f,
				0.75f,
				G_PARAM_CONSTRUCT |
				G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
					 PROP_BUBBLE_OPACITY,
					 property_bubble_opacity);

	property_bubble_red = g_param_spec_double (
				"bubble-color-red",
				"bubble-color-red",
				"Red-component of bubble color",
				0.0f,
				1.0f,
				0.1f,
				G_PARAM_CONSTRUCT |
				G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
					 PROP_BUBBLE_RED,
					 property_bubble_red);

	property_bubble_green = g_param_spec_double (
				"bubble-color-green",
				"bubble-color-green",
				"Green-component of bubble color",
				0.0f,
				1.0f,
				0.1f,
				G_PARAM_CONSTRUCT |
				G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
					 PROP_BUBBLE_GREEN,
					 property_bubble_green);

	property_bubble_blue = g_param_spec_double (
				"bubble-color-blue",
				"bubble-color-blue",
				"Blue-component of bubble color",
				0.0f,
				1.0f,
				0.1f,
				G_PARAM_CONSTRUCT |
				G_PARAM_READABLE);
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
				G_PARAM_READABLE);
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
				G_PARAM_READABLE);
	g_object_class_install_property (gobject_class,
					 PROP_FONT_SIZE,
					 property_font_size);

	property_font_face = g_param_spec_string (
				"font-face",
				"font-face",
				"Face of the font",
				"DejaVu Sans",
				G_PARAM_CONSTRUCT |
				G_PARAM_READABLE);
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

gboolean
defaults_initialized (Defaults* self)
{
	gboolean flag;

	g_object_get (self, "initialized", &flag, NULL);

	return flag;
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

	g_object_get (self, "font-face", face, NULL);

	return face;
}
