/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * bubble-window.c
 * Copyright (C) Eitan Isaacson 2009 <eitan@ascender.com>
 * 
 */

#ifndef _BUBBLE_WINDOW_H_
#define _BUBBLE_WINDOW_H_

#include <glib-object.h>
#include <gtk/gtkwindow.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define BUBBLE_TYPE_WINDOW             (bubble_window_get_type ())
#define BUBBLE_WINDOW(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BUBBLE_TYPE_WINDOW, BubbleWindow))
#define BUBBLE_WINDOW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BUBBLE_TYPE_WINDOW, BubbleWindowClass))
#define BUBBLE_IS_WINDOW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BUBBLE_TYPE_WINDOW))
#define BUBBLE_IS_WINDOW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BUBBLE_TYPE_WINDOW))
#define BUBBLE_WINDOW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BUBBLE_TYPE_WINDOW, BubbleWindowClass))

typedef struct _BubbleWindowClass BubbleWindowClass;
typedef struct _BubbleWindow BubbleWindow;

struct _BubbleWindowClass
{
	GtkWindowClass parent_class;
};

struct _BubbleWindow
{
	GtkWindow parent_instance;
};

GType bubble_window_get_type (void) G_GNUC_CONST;

GtkWidget *bubble_window_new (void);

G_END_DECLS

#endif /* _BUBBLE_WINDOW_H_ */
