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
#include "bubble-accessible-factory.h"
#include "bubble-accessible.h"

G_DEFINE_TYPE (BubbleAccessibleFactory,
               bubble_accessible_factory, ATK_TYPE_OBJECT_FACTORY);

static AtkObject* bubble_accessible_factory_create_accessible (GObject *obj);

static GType bubble_accessible_factory_get_accessible_type (void);

static void
bubble_accessible_factory_class_init (BubbleAccessibleFactoryClass *klass)
{
  AtkObjectFactoryClass *class = ATK_OBJECT_FACTORY_CLASS (klass);

  class->create_accessible = bubble_accessible_factory_create_accessible;
  class->get_accessible_type = bubble_accessible_factory_get_accessible_type;
}

static void
bubble_accessible_factory_init (BubbleAccessibleFactory *factory)
{
}

AtkObjectFactory*
bubble_accessible_factory_new (void)
{
  GObject *factory;
  factory = g_object_new (BUBBLE_TYPE_ACCESSIBLE_FACTORY, NULL);
  return ATK_OBJECT_FACTORY (factory);
}

static AtkObject*
bubble_accessible_factory_create_accessible (GObject *obj)
{
  return bubble_accessible_new (obj);
}

static GType
bubble_accessible_factory_get_accessible_type (void)
{
  return BUBBLE_TYPE_ACCESSIBLE;
}
