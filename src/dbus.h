/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789
**      10        20        30        40        50        60        70        80
**
** project:
**    alsdorf
**
** file:
**    dbus.h
**
** author(s):
**    Mirco "MacSlow" Mueller <mirco.mueller@canonical.com>
**    David Barth <david.barth@canonical.com>
**
** copyright (C) Canonical, oct. 2008
**
*******************************************************************************/

#ifndef __DBUS_H
#define __DBUS_H

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

DBusGConnection*
dbus_create_service_instance ();

G_END_DECLS

#endif /* __DEFAULTS_H */
