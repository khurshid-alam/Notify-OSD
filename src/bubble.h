/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
**      10        20        30        40        50        60        70        80
**
** project:
**    alsdorf
**
** file:
**    bubble.h
**
** author(s):
**    Mirco "MacSlow" Mueller <mirco.mueller@canonical.com>
**    David Barth <david.barth@canonical.com>
**
** copyright (C) Canonical, oct. 2008
**
*******************************************************************************/
#ifndef __BUBBLE_H
#define __BUBBLE_H

#include <glib-object.h>

G_BEGIN_DECLS

#define BUBBLE_TYPE             (bubble_get_type ())
#define BUBBLE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BUBBLE_TYPE, Bubble))
#define BUBBLE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BUBBLE_TYPE, BubbleClass))
#define IS_BUBBLE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BUBBLE_TYPE))
#define IS_BUBBLE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BUBBLE_TYPE))
#define BUBBLE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BUBBLE_TYPE, BubbleClass))

typedef struct _Bubble      Bubble;
typedef struct _BubbleClass BubbleClass;
typedef struct _BubblePrivate BubblePrivate;


/* instance structure */
struct _Bubble
{
	GObject parent;

	/*< private >*/
	BubblePrivate *priv;
};

/* class structure */
struct _BubbleClass
{
	GObjectClass parent;
};

GType bubble_get_type (void);

Bubble*
bubble_new (void);

void
bubble_set_title (Bubble* self,
		  char *title);

void
bubble_set_message_body (Bubble* self,
			 char *body);

void
bubble_set_size(Bubble* self,
	      gint width,
	      gint height);

void
bubble_move (Bubble* self,
	     gint x,
	     gint y);

void
bubble_display (Bubble* self);

void
bubble_hide (Bubble* self);

void
bubble_del (Bubble* self);

G_END_DECLS

#endif /* __BUBBLE_H */
