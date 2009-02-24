/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
**      10        20        30        40        50        60        70        80
**
** Codename "alsdorf"
**
** bubble-accessible-factory.c - implements an accessible object factory
**
** Copyright 2009 Canonical Ltd.
**
** Authors:
**    Eitan Isaacson <eitan@ascender.com>
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

#include "bubble-window-accessible.h"
#include "bubble.h"



G_DEFINE_TYPE (BubbleWindowAccessible, bubble_window_accessible, GTK_TYPE_ACCESSIBLE);

static int         bubble_window_accessible_get_n_children (AtkObject *obj);
static AtkObject*  bubble_window_accessible_ref_child      (AtkObject *obj,
															int        i);

static void
bubble_window_accessible_init (BubbleWindowAccessible *object)
{
	/* TODO: Add initialization code here */
}

static void
bubble_window_accessible_finalize (GObject *object)
{
	/* TODO: Add deinitalization code here */

	G_OBJECT_CLASS (bubble_window_accessible_parent_class)->finalize (object);
}

static void
bubble_window_accessible_class_init (BubbleWindowAccessibleClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

	class->get_n_children = bubble_window_accessible_get_n_children;
	class->ref_child = bubble_window_accessible_ref_child;
	
	object_class->finalize = bubble_window_accessible_finalize;
}

AtkObject*
bubble_window_accessible_new (GtkWidget *widget)
{
	GObject *object;
	AtkObject *aobj_pager;
	GtkAccessible *gtk_accessible;
	
	object = g_object_new (BUBBLE_WINDOW_TYPE_ACCESSIBLE, NULL);
	
	aobj_pager = ATK_OBJECT (object);
	
	gtk_accessible = GTK_ACCESSIBLE (aobj_pager);
	gtk_accessible->widget = widget;
	
	atk_object_initialize (aobj_pager, widget);
	//aobj_pager->role = ATK_ROLE_PANEL;
	
	return aobj_pager;
}

static int 
bubble_window_accessible_get_n_children (AtkObject *obj)
{
	GtkAccessible *accessible;
	GtkWidget *widget;
	gpointer bubble;
	
	g_return_val_if_fail (BUBBLE_WINDOW_IS_ACCESSIBLE (obj), 0);
	
	accessible = GTK_ACCESSIBLE (obj);
	widget = accessible->widget;
	
	bubble = g_object_get_data (G_OBJECT(widget), "bubble");
	
	if (bubble != NULL)
		return 1;

	return 0;
}

static AtkObject*
bubble_window_accessible_ref_child (AtkObject *obj,
									int        i)
{
	GtkAccessible *accessible;
	AtkObject *bubble_accessible;
	GtkWidget *widget;
	Bubble *bubble;
	AtkRegistry *default_registry;   
	AtkObjectFactory *factory;

	g_return_val_if_fail (BUBBLE_WINDOW_IS_ACCESSIBLE (obj), 0);
	
	accessible = GTK_ACCESSIBLE (obj);
	widget = accessible->widget;
	
	bubble = g_object_get_data (G_OBJECT(widget), "bubble");
	
	default_registry = atk_get_default_registry ();
	factory = atk_registry_get_factory (default_registry, BUBBLE_TYPE);
	
	printf("Bubble: %p %d\n", bubble, IS_BUBBLE(bubble));

    bubble_accessible = atk_gobject_accessible_for_object (G_OBJECT (bubble));

	if (atk_object_get_parent (bubble_accessible) == NULL)
		atk_object_set_parent (bubble_accessible, obj); 
															 
	g_object_ref (G_OBJECT (bubble_accessible));
	
	return bubble_accessible;

}
