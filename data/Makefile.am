servicedir=/usr/share/dbus-1/services
service_DATA=org.freedesktop.Notifications.service

org.freedesktop.Notifications.service: org.freedesktop.Notifications.service.in
	sed "s_@LIBEXECDIR@_$(libexecdir)_" $< > $@

SUBDIRS = icons
