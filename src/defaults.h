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
	gint     bubble_vert_gap;
	gint     bubble_horz_gap;
	gint     bubble_width;
	gint     bubble_min_height;
	gint     bubble_max_height;
	gint     bubble_shadow_size;
	GString* bubble_shadow_color;
	GString* bubble_bg_color;
	GString* bubble_bg_opacity;
	GString* bubble_hover_opacity;
	gint     bubble_corner_radius;
	gint     content_shadow_size;
	GString* content_shadow_color;
	gint     margin_size;
	gint     icon_size;
	gint     fade_in_timeout;
	gint     fade_out_timeout;
	gint     on_screen_timeout;
	GString* text_font_face;
	GString* text_title_color;
	gint     text_title_weight;
	gint     text_title_size;
	GString* text_body_color;
	gint     text_body_weight;
	gint     text_body_size;
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
defaults_get_bubble_width (Defaults* self);

gint
defaults_get_bubble_min_height (Defaults* self);

gint
defaults_get_bubble_max_height (Defaults* self);

gint
defaults_get_bubble_vert_gap (Defaults* self);

gint
defaults_get_bubble_horz_gap (Defaults* self);

gint
defaults_get_bubble_shadow_size (Defaults* self);

gchar*
defaults_get_bubble_shadow_color (Defaults* self);

gchar*
defaults_get_bubble_bg_color (Defaults* self);

gchar*
defaults_get_bubble_bg_opacity (Defaults* self);

gchar*
defaults_get_bubble_hover_opacity (Defaults* self);

gint
defaults_get_bubble_corner_radius (Defaults* self);

gint
defaults_get_content_shadow_size (Defaults* self);

gchar*
defaults_get_content_shadow_color (Defaults* self);

gint
defaults_get_margin_size (Defaults* self);

gint
defaults_get_icon_size (Defaults* self);

gint
defaults_get_fade_in_timeout (Defaults* self);

gint
defaults_get_fade_out_timeout (Defaults* self);

gint
defaults_get_on_screen_timeout (Defaults* self);

gchar*
defaults_get_text_font_face (Defaults* self);

gchar*
defaults_get_text_title_color (Defaults* self);

gint
defaults_get_text_title_weight (Defaults* self);

gint
defaults_get_text_title_size (Defaults* self);

gchar*
defaults_get_text_body_color (Defaults* self);

gint
defaults_get_text_body_weight (Defaults* self);

gint
defaults_get_text_body_size (Defaults* self);

G_END_DECLS

#endif /* __DEFAULTS_H */
