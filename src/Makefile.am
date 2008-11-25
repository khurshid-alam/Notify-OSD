NULL =

INCLUDES =			\
	-I.			\
	-I$(srcdir)		\
	$(PYTHON_INCLUDES)	\
	$(NULL)

bin_PROGRAMS = \
	alsdorf			\
	$(NULL)

alsdorf_sources =		\
	bubble.c		\
	defaults.c		\
	main.c			\
	observer.c		\
	stack-display.c		\
	stack.c			\
	$(NULL)

alsdorf_headers =		\
	bubble.h		\
	defaults.h		\
	observer.h		\
	stack-display.h		\
	stack.h			\
	$(NULL)

alsdorf_SOURCES =		\
	$(alsdorf_sources)	\
	$(alsdorf_headers)	\
	$(NULL)

alsdorf_LDADD =			\
	$(X_LIBS)		\
	$(PYTHON_LIBS)		\
	$(GTK_LIBS) 		\
	$(NULL)

alsdorf_CFLAGS = 		\
	-Wall -Werror -std=c99	\
	$(GTK_CFLAGS) 		\
	$(NULL)

alsdorf_LDFLAGS = 		\
	$(PYTHON_LIBS_DIR)	\
	-Xlinker -export-dynamic -Wl,-O1 -Wl,-Bsymbolic-functions \
	$(NULL)

# this comes from distutils.sysconfig.get_config_var('LINKFORSHARED')


EXTRA_DIST =			\
	$(NULL)

CLEANFILES = 			\
	$(NULL)

dist-hook:
	cd $(distdir) ; rm -f $(CLEANFILES)
