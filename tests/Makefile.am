noinst_PROGRAMS = test-modules

check_PROGRAMS = test-modules
TESTS = test-modules

test_modules_SOURCES =		\
	../src/bubble.c		\
	../src/defaults.c	\
	../src/observer.c	\
	../src/stack.c		\
	../src/dbus.c		\
	../src/apport.c		\
	test-modules-main.c     \
	test-apport.c		\
	test-dbus.c		\
	test-withlib.c		\
	test-bubble.c		\
	test-defaults.c		\
	test-observer.c		\
	test-i18n.c		\
	test-stack.c

test_modules_CFLAGS =		\
	-Wall -Werror -std=c99  \
	$(GLIB_CFLAGS)		\
	$(LIBRSVG_CFLAGS)	\
	$(GTK_CFLAGS)		\
	$(DBUS_CFLAGS)		\
	-I$(top_srcdir)/src

test_modules_LDADD =		\
	$(GLIB_LIBS)		\
	$(LIBRSVG_LIBS)		\
	$(DBUS_LIBS)		\
	$(GTK_LIBS)		\
	-lnotify

test: test-modules
	gtester -o=test-modules.xml -k ./test-modules

i18n: test-modules
	gtester --verbose -p=/i18n -k ./test-modules

test-report-html: test
	gtester-report test-modules.xml >test-modules.html

test-report-display: test-report-html
	gnome-open test-modules.html

