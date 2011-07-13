
SUBDIRS = icons

servicedir=$(datadir)/dbus-1/services
service_DATA=org.freedesktop.Notifications.service

gsettings_SCHEMAS=com.canonical.NotifyOSD.gschema.xml
@GSETTINGS_RULES@

EXTRA_DIST=org.freedesktop.Notifications.service.in \
	   com.canonical.NotifyOSD.gschema.xml

org.freedesktop.Notifications.service: org.freedesktop.Notifications.service.in
	sed "s,@LIBEXECDIR@,$(libexecdir)," $< > $@

CLEANFILES = org.freedesktop.Notifications.service
