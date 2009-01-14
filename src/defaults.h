/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
**      10        20        30        40        50        60        70        80
**
** project:
**    alsdorf
**
** file:
**    defaults.h
**
** author(s):
**    Mirco "MacSlow" Mueller <mirco.mueller@canonical.com>
**    David Barth <david.barth@canonical.com>
**
** copyright (C) Canonical, oct. 2008
**
*******************************************************************************/

#ifndef __DEFAULTS_H
#define __DEFAULTS_H

#include <glib-object.h>

G_BEGIN_DECLS

#define DEFAULTS_TYPE             (defaults_get_type ())
#define DEFAULTS(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), DEFAULTS_TYPE, Defaults))
#define DEFAULTS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), DEFAULTS_TYPE, DefaultsClass))
#define IS_DEFAULTS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DEFAULTS_TYPE))
#define IS_DEFAULTS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), DEFAULTS_TYPE))
#define DEFAULTS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), DEFAULTS_TYPE, DefaultsClass))

typedef struct _Defaults      Defaults;
typedef struct _DefaultsClass DefaultsClass;

/* instance structure */
struct _Defaults
{
	GObject parent;

	/* private */
	gint     desktop_width;
	gint     desktop_height;
	gint     desktop_top;
	gint     desktop_bottom;
	gint     desktop_left;
	gint     desktop_right;
	gint     stack_height;
	gint     bubble_gap;
	gint     bubble_width;
	gint     bubble_height;
	gdouble  bubble_opacity;
	gdouble  bubble_color[3];
	gdouble  font_dpi;
	gdouble  font_size;
	GString* font_face;
};

/* class structure */
struct _DefaultsClass
{
	GObjectClass parent;
};

GType defaults_get_type (void);

Defaults*
defaults_new (void);

void
defaults_del (Defaults* self);

gint
defaults_get_desktop_width (Defaults* self);

gint
defaults_get_desktop_height (Defaults* self);

gint
defaults_get_desktop_top (Defaults* self);

gint
defaults_get_desktop_bottom (Defaults* self);

gint
defaults_get_desktop_left (Defaults* self);

gint
defaults_get_desktop_right (Defaults* self);

gint
defaults_get_stack_height (Defaults* self);

gint
defaults_get_bubble_gap (Defaults* self);

gint
defaults_get_bubble_width (Defaults* self);

gint
defaults_get_bubble_height (Defaults* self);

gdouble
defaults_get_bubble_opacity (Defaults* self);

void
defaults_get_bubble_color (Defaults* self,
			   gdouble*  red,
			   gdouble*  green,
			   gdouble*  blue);

gdouble
defaults_get_font_dpi (Defaults* self);

gdouble
defaults_get_font_size (Defaults* self);

gchar*
defaults_get_font_face (Defaults* self);

G_END_DECLS

#endif /* __DEFAULTS_H */
