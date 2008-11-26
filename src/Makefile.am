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
	main.c			\
	stack-display.c		\
	bubble.c		\
	python.c		\
	$(NULL)

alsdorf_headers =		\
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
