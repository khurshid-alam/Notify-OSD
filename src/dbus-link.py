#!/usr/bin/env python

import dbus
import gobject
import dbus.service
from dbus.mainloop.glib import DBusGMainLoop

# get access to internal C routines
import notifications

main_loop = DBusGMainLoop()
session_bus = dbus.SessionBus(mainloop=main_loop)

DBUS_NAME = "com.canonical.notifications"
DBUS_PATH = "/com/canonical/notifications"

class NotificationsObject(dbus.service.Object):
    def __init__(self):
        global session_bus
        bus_name = dbus.service.BusName(DBUS_NAME, bus=session_bus)
        dbus.service.Object.__init__(self, bus_name, DBUS_PATH)

#    @dbus.service.method(dbus_interface=DBUS_NAME,
#                         in_signature='susssasa{sv}i',
#                         out_signature='u')
#    def notify(self, app_name, replaces_id, app_icon, summary, body, actions, hints, expire_timeout):
#        print "Received notification from %s", app_name
#        notifications.push(summary, body)

    @dbus.service.method(dbus_interface=DBUS_NAME,
                         in_signature='ss',
                         out_signature='')
    def notify(self, summary, body):
        print "Received notification {\"%s\", \"%s\"}" % (summary, body)
        notifications.push(summary, body)

server = NotificationsObject()
