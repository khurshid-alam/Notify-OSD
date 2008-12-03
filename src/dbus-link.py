#!/usr/bin/env python

import dbus
import gobject
import dbus.service
from dbus.mainloop.glib import DBusGMainLoop

# get access to internal C routines
import notifications

main_loop = DBusGMainLoop()
session_bus = dbus.SessionBus(mainloop=main_loop)

DBUS_NAME = "org.freedesktop.Notifications"
DBUS_PATH = "/org/freedesktop/Notifications"

class NotificationsObject(dbus.service.Object):
    def __init__(self):
        global session_bus
        bus_name = dbus.service.BusName(DBUS_NAME, bus=session_bus)
        dbus.service.Object.__init__(self, bus_name, DBUS_PATH)
        # FIXME: check error, like another program already owning the name...

    @dbus.service.method(dbus_interface=DBUS_NAME,
                         in_signature='susssasa{sv}i',
                         out_signature='u')
    def Notify(self, app_name, replaces_id, app_icon,
               summary, body, actions, hints, expire_timeout):
        print "Received notification from %s", app_name
        notifications.push(summary, body)
        # FIXME: return the real notification ID
        return 1

    @dbus.service.method(dbus_interface=DBUS_NAME,
                         out_signature='ssss')
    def GetServerInformation(self):
        print "GetServerInformation"
        #      return_name, return_vendor, return_version, return_spec_version
        return "alsdorf",   "Canonical",   "0.1",          "2.0"

    @dbus.service.method(dbus_interface=DBUS_NAME,
                         out_signature='as')
    def GetCapabilities(self):
        print "GetCapabilities"
        return [ "dummy", "dummy" ]
        # return [ return_caps ]

    @dbus.service.method(dbus_interface=DBUS_NAME,
                         in_signature='u')
    def CloseNotification(self, id):
        print "CloseNotification(%s)" % id

server = NotificationsObject()
