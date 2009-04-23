noinst_PROGRAMS = icon-only		\
		  sync-icon-only	\
		  icon-summary-body	\
		  icon-summary		\
		  icon-value		\
		  summary-body		\
		  summary-only		\
		  append-hint-example	\
		  update-notifications

example_SOURCES = example-util.c	\
		  icon-only.c		\
		  sync-icon-only.c	\
		  icon-summary-body.c	\
		  icon-summary.c	\
		  icon-value.c		\
		  summary-body.c	\
		  summary-only.c	\
		  append-hint-example.c	\
		  update-notifications.c

example_CFLAGS = -O0		\
		 -ggdb		\
		 -Wall		\
		 -Werror	\
		 `pkg-config --cflags libnotify glib-2.0`

gcc -O0 -ggdb -Wall -Werror `pkg-config --cflags --libs libnotify glib-2.0` example-util.c icon-only.c -o icon-only
gcc -O0 -ggdb -Wall -Werror `pkg-config --cflags --libs libnotify glib-2.0` example-util.c sync-icon-only.c -o sync-icon-only
gcc -O0 -ggdb -Wall -Werror `pkg-config --cflags --libs libnotify glib-2.0` example-util.c icon-summary-body.c -o icon-summary-body
gcc -O0 -ggdb -Wall -Werror `pkg-config --cflags --libs libnotify glib-2.0` example-util.c icon-summary.c -o icon-summary
gcc -O0 -ggdb -Wall -Werror `pkg-config --cflags --libs libnotify glib-2.0` example-util.c icon-value.c -o icon-value
gcc -O0 -ggdb -Wall -Werror `pkg-config --cflags --libs libnotify glib-2.0` example-util.c summary-body.c -o summary-body
gcc -O0 -ggdb -Wall -Werror `pkg-config --cflags --libs libnotify glib-2.0` example-util.c summary-only.c -o summary-only
gcc -O0 -ggdb -Wall -Werror `pkg-config --cflags --libs libnotify glib-2.0` example-util.c append-hint-example.c -o append-hint-example
gcc -O0 -ggdb -Wall -Werror `pkg-config --cflags --libs libnotify glib-2.0` example-util.c update-notifications.c -o update-notifications

notify_osddir = $(libexecdir)

notify_osd_PROGRAMS = \
	notify-osd		\
	$(NULL)

notify_osd_sources =		\
	bubble.c		\
	defaults.c		\
	main.c			\
	observer.c		\
	stack.c			\
	dbus.c			\
	dnd.c			\
	apport.c		\
	log.c			\
	util.c			\
	bubble-window.c		\
	bubble-window-accessible.c \
	bubble-window-accessible-factory.c \
	../egg/egg-fixed.c		\
	../egg/egg-units.c		\
	../egg/egg-timeline.c		\
	../egg/egg-timeout-pool.c	\
	../egg/egg-alpha.c		\
	../egg/egg-hack.c		\
	$(NULL)

notify_osd_headers =		\
	bubble.h		\
	defaults.h		\
	observer.h		\
	stack.h			\
	stack-glue.h		\
	dbus.h			\
	dnd.h			\
	log.h			\
	apport.h		\
	util.h			\
	bubble-window.h \
	bubble-window-accessible.h \
	bubble-window-accessible-factory.h \
	../egg/egg-fixed.h		\
	../egg/egg-units.h		\
	../egg/egg-timeline.h		\
	../egg/egg-timeout-pool.h	\
	../egg/egg-alpha.h		\
	../egg/egg-debug.h		\
	../egg/egg-hack.h		\
	$(NULL)

notify_osd_SOURCES =		\
	$(notify_osd_sources)	\
	$(notify_osd_headers)	\
	$(NULL)

notify_osd_LDADD =			\
	$(X_LIBS)		\
	$(GLIB_LIBS) 		\
	$(GCONF_LIBS) 		\
	$(GTK_LIBS) 		\
	$(NOTIFY_OSD_LIBS)	\
	$(DBUS_LIBS)     	\
	$(WNCK_LIBS)     	\
	$(NULL)

notify_osd_CFLAGS = 		\
	-DDATADIR=\""$(datadir)"\" \
	$(GTK_CFLAGS) 		\
	$(NOTIFY_OSD_CFLAGS)	\
	$(GLIB_CFLAGS) 		\
	$(GCONF_CFLAGS)		\
	$(DBUS_CFLAGS)		\
	$(WNCK_CFLAGS)		\
	$(NULL)

notify_osd_LDFLAGS = 		\
	-Xlinker -export-dynamic -Wl,-O1 -Wl,-Bsymbolic-functions \
	$(NULL)

# this comes from distutils.sysconfig.get_config_var('LINKFORSHARED')

stack-glue.h: notify-osd.xml Makefile
	$(LIBTOOL) --mode=execute $(DBUS_GLIB_BIN)/dbus-binding-tool --prefix=stack --mode=glib-server --output=$@ $<

BUILT_SOURCES =			\
	stack-glue.h

EXTRA_DIST =			\
	notify-osd.xml		\
	dialog.c		\
	display.c		\
	$(NULL)

CLEANFILES = 			\
	$(NULL)

dist-hook:
	cd $(distdir) ; rm -f $(CLEANFILES)
