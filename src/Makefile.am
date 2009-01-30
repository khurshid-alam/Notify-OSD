NULL =

INCLUDES =			\
	-I.			\
	-I$(srcdir)		\
	$(NULL)

alsdorfdir = $(libexecdir)

alsdorf_PROGRAMS = \
	alsdorf			\
	$(NULL)

alsdorf_sources =		\
	bubble.c		\
	defaults.c		\
	main.c			\
	observer.c		\
	stack.c			\
	dbus.c			\
	apport.c		\
	../clutter/clutter-fixed.c		\
	../clutter/clutter-units.c		\
	../clutter/clutter-timeline.c		\
	../clutter/clutter-timeout-pool.c	\
	../clutter/clutter-alpha.c		\
	../clutter/clutter-hack.c		\
	$(NULL)

alsdorf_headers =		\
	bubble.h		\
	defaults.h		\
	observer.h		\
	stack.h			\
	stack-glue.h		\
	dbus.h			\
	apport.h		\
	../clutter/clutter-fixed.h		\
	../clutter/clutter-units.h		\
	../clutter/clutter-timeline.h		\
	../clutter/clutter-timeout-pool.h	\
	../clutter/clutter-alpha.h		\
	../clutter/clutter-debug.h		\
	../clutter/clutter-hack.h		\
	$(NULL)

alsdorf_SOURCES =		\
	$(alsdorf_sources)	\
	$(alsdorf_headers)	\
	$(NULL)

alsdorf_LDADD =			\
	$(X_LIBS)		\
	$(GLIB_LIBS) 		\
	$(GCONF_LIBS) 		\
	$(GTK_LIBS) 		\
	$(DBUS_LIBS)     	\
	$(NULL)

alsdorf_CFLAGS = 		\
	-Wall -Werror -std=c99	\
	$(GTK_CFLAGS) 		\
	$(GCONF_CFLAGS)		\
	$(DBUS_CFLAGS)		\
	$(NULL)

alsdorf_LDFLAGS = 		\
	-Xlinker -export-dynamic -Wl,-O1 -Wl,-Bsymbolic-functions \
	$(NULL)

# this comes from distutils.sysconfig.get_config_var('LINKFORSHARED')

stack-glue.h: alsdorf.xml Makefile
	$(LIBTOOL) --mode=execute $(DBUS_GLIB_BIN)/dbus-binding-tool --prefix=stack --mode=glib-server --output=$@ $<

BUILT_SOURCES =			\
	stack-glue.h

EXTRA_DIST =			\
	alsdorf.xml		\
	$(NULL)

CLEANFILES = 			\
	$(NULL)

dist-hook:
	cd $(distdir) ; rm -f $(CLEANFILES)
