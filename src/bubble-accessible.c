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

#include <gtk/gtk.h>
#include <errno.h>
#include <unistd.h>
#include "bubble-accessible.h"

static void        bubble_accessible_class_init          (BubbleAccessibleClass *klass);
static const char* bubble_accessible_get_name            (AtkObject                    *obj);
static const char* bubble_accessible_get_description     (AtkObject                    *obj);
static int         bubble_accessible_get_index_in_parent (AtkObject                    *obj);

GType
bubble_accessible_get_type (void)
{
  static GType type = 0;

  if (!type)
    {
      const GTypeInfo tinfo = 
      {
        sizeof (BubbleAccessibleClass),
        (GBaseInitFunc) NULL, /* base init */
        (GBaseFinalizeFunc) NULL, /* base finalize */
        (GClassInitFunc) bubble_accessible_class_init,
        (GClassFinalizeFunc) NULL, /* class finalize */
        NULL, /* class data */
        sizeof (BubbleAccessible), /* instance size*/
        0, /* nb preallocs */
        (GInstanceInitFunc) NULL, /* instance init */
        NULL /* value table */
      };

      type = g_type_register_static (ATK_TYPE_GOBJECT_ACCESSIBLE, "BubbleAccessible", &tinfo, 0);
  }
  return type;
}


static void
bubble_accessible_class_init (BubbleAccessibleClass *klass)
{
  AtkObjectClass *class = ATK_OBJECT_CLASS (klass);

  class->get_name = bubble_accessible_get_name;
  class->get_description = bubble_accessible_get_description;
  class->get_index_in_parent = bubble_accessible_get_index_in_parent;
}

AtkObject*
bubble_accessible_new (GObject *obj)
{
  GObject *object;
  AtkObject *atk_object;

  g_return_val_if_fail (IS_BUBBLE (obj), NULL);

  object = g_object_new (BUBBLE_TYPE_ACCESSIBLE, NULL);
  atk_object = ATK_OBJECT (object);
  atk_object_initialize (atk_object, obj);

  g_return_val_if_fail (ATK_IS_OBJECT (atk_object), NULL);

  BUBBLE_ACCESSIBLE (atk_object)->index = 0;
  
  return atk_object;
}

static const char*
bubble_accessible_get_name (AtkObject *obj)
{
  return "Awesome bubble!";
}

static const char*
bubble_accessible_get_description (AtkObject *obj)
{
  g_return_val_if_fail (IS_BUBBLE_ACCESSIBLE (obj), NULL);

  if (obj->description != NULL)
    {
      return obj->description;
    }
  else
    return NULL;
}

static gint
bubble_accessible_get_index_in_parent (AtkObject *obj)
{
  g_return_val_if_fail (IS_BUBBLE_ACCESSIBLE (obj), -1);

  return BUBBLE_ACCESSIBLE (obj)->index;
}
