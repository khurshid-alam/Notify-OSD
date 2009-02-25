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

static void        bubble_window_accessible_init            (BubbleWindowAccessible*      object);
static void        bubble_window_accessible_finalize        (GObject*                     object);
static void        bubble_window_accessible_class_init      (BubbleWindowAccessibleClass* klass);
static const char* bubble_window_accessible_get_name        (AtkObject*                   obj);
static const char* bubble_window_accessible_get_description (AtkObject*                   obj);

static void* bubble_window_accessible_parent_class;

GType
bubble_window_accessible_get_type (void)
{
    static GType type = 0;
    
    if (!type) 
    {
        GTypeInfo tinfo = 
        {
            sizeof (BubbleWindowAccessibleClass),
            (GBaseInitFunc) bubble_window_accessible_init, /* base init */
            (GBaseFinalizeFunc) bubble_window_accessible_finalize, /* base finalize */
            (GClassInitFunc) bubble_window_accessible_class_init, /* class init */
            (GClassFinalizeFunc) NULL, /* class finalize */
            NULL, /* class data */
            sizeof (BubbleWindowAccessible), /* instance size */
            0, /* nb preallocs */
            NULL, /* instance init */
            NULL /* value table */
        };
                
        /*
         * Figure out the size of the class and instance
         * we are deriving from
         */
        AtkObjectFactory *factory;
        GType derived_type;
        GTypeQuery query;
        GType derived_atk_type;
        
        derived_type = g_type_parent (BUBBLE_TYPE_WINDOW);  
        
        factory = atk_registry_get_factory (atk_get_default_registry (),
                                            derived_type);
        derived_atk_type = atk_object_factory_get_accessible_type (factory);
        
        g_type_query (derived_atk_type, &query);
        tinfo.class_size = query.class_size;
        tinfo.instance_size = query.instance_size;
        
        type = g_type_register_static (derived_atk_type,
                                       "BubbleWindowAccessible", &tinfo, 0);
        
    }
    
    return type;
}


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
    
    bubble_window_accessible_parent_class = g_type_class_peek_parent (klass);
    
    class->get_name = bubble_window_accessible_get_name;
    class->get_description = bubble_window_accessible_get_description;

	object_class->finalize = bubble_window_accessible_finalize;
}

AtkObject*
bubble_window_accessible_new (GtkWidget *widget)
{
	GObject *object;
	AtkObject *aobj;
	GtkAccessible *gtk_accessible;
	
	object = g_object_new (BUBBLE_WINDOW_TYPE_ACCESSIBLE, NULL);
	
	aobj = ATK_OBJECT (object);
	
	gtk_accessible = GTK_ACCESSIBLE (aobj);
	gtk_accessible->widget = widget;
	
	atk_object_initialize (aobj, widget);
	//aobj->role = ATK_ROLE_PANEL;
	
	return aobj;
}

static const char* 
bubble_window_accessible_get_name (AtkObject* obj)
{
    GtkAccessible *accessible;
 	Bubble *bubble;
 
 	g_return_val_if_fail (BUBBLE_WINDOW_IS_ACCESSIBLE (obj), "");
 	
 	accessible = GTK_ACCESSIBLE (obj);
    
    if (accessible->widget == NULL)
        return "";
 	
 	bubble = g_object_get_data (G_OBJECT(accessible->widget), "bubble");

    g_return_val_if_fail (IS_BUBBLE (bubble), "");
    
    return bubble_get_title(bubble);
}

static const char*
bubble_window_accessible_get_description (AtkObject* obj)
{
    GtkAccessible *accessible;
 	Bubble *bubble;
 
 	g_return_val_if_fail (BUBBLE_WINDOW_IS_ACCESSIBLE (obj), "");
 	
 	accessible = GTK_ACCESSIBLE (obj);
    
    if (accessible->widget == NULL)
        return "";
 	
 	bubble = g_object_get_data (G_OBJECT(accessible->widget), "bubble");

    g_return_val_if_fail (IS_BUBBLE (bubble), "");
    
    return bubble_get_message_body(bubble);
}
