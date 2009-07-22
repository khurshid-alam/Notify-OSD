
SUBDIRS = icons

servicedir=$(datadir)/dbus-1/services
service_DATA=org.freedesktop.Notifications.service

EXTRA_DIST=org.freedesktop.Notifications.service.in

org.freedesktop.Notifications.service: org.freedesktop.Notifications.service.in
	sed "s,@LIBEXECDIR@,$(libexecdir)," $< > $@

CLEANFILES = org.freedesktop.Notifications.service
