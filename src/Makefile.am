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
	dnd.c			\
	apport.c		\
	../egg/egg-fixed.c		\
	../egg/egg-units.c		\
	../egg/egg-timeline.c		\
	../egg/egg-timeout-pool.c	\
	../egg/egg-alpha.c		\
	../egg/egg-hack.c		\
	$(NULL)

alsdorf_headers =		\
	bubble.h		\
	defaults.h		\
	observer.h		\
	stack.h			\
	stack-glue.h		\
	dbus.h			\
	dnd.h			\
	apport.h		\
	../egg/egg-fixed.h		\
	../egg/egg-units.h		\
	../egg/egg-timeline.h		\
	../egg/egg-timeout-pool.h	\
	../egg/egg-alpha.h		\
	../egg/egg-debug.h		\
	../egg/egg-hack.h		\
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
