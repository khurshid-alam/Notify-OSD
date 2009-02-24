/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
**      10        20        30        40        50        60        70        80
**
** Codename "alsdorf"
**
** bubble-accessible.h - implements an accessible bubble object
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

#ifndef __BUBBLE_ACCESSIBLE_H__
#define __BUBBLE_ACCESSIBLE_H__

#include <gtk/gtk.h>
#include <atk/atk.h>
#include "bubble.h"
#include "bubble-accessible.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define BUBBLE_TYPE_ACCESSIBLE                     (bubble_accessible_get_type ())
#define BUBBLE_ACCESSIBLE(obj)                     (G_TYPE_CHECK_INSTANCE_CAST ((obj), BUBBLE_TYPE_ACCESSIBLE, BubbleAccessible))
#define BUBBLE_ACCESSIBLE_CLASS(klass)             (G_TYPE_CHECK_CLASS_CAST ((klass), BUBBLE_TYPE_ACCESSIBLE, BubbleAccessibleClass))
#define IS_BUBBLE_ACCESSIBLE(obj)                  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BUBBLE_TYPE_ACCESSIBLE))
#define IS_BUBBLE_ACCESSIBLE_CLASS(klass)          (G_TYPE_CHECK_CLASS_TYPE ((klass), BubbleAccessible))
#define BUBBLE_ACCESSIBLE_GET_CLASS(obj)           (G_TYPE_INSTANCE_GET_CLASS ((obj), BUBBLE_TYPE_ACCESSIBLE, BubbleAccessibleClass)) 

typedef struct _BubbleAccessible BubbleAccessible;
typedef struct _BubbleAccessibleClass BubbleAccessibleClass;

struct _BubbleAccessible
{
  AtkGObjectAccessible parent;

  int index;
};

struct _BubbleAccessibleClass 
{
  AtkGObjectAccessibleClass parent_class;
};

GType bubble_accessible_get_type (void) G_GNUC_CONST;

AtkObject* bubble_accessible_new (GObject *obj); 

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __BUBBLE_ACCESSIBLE_H__ */
