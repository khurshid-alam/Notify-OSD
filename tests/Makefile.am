noinst_PROGRAMS = test-modules \
		  test-timeline \
		  test-timeline-dup-frames \
		  test-timeline-interpolate \
		  test-timeline-rewind \
		  test-timeline-smoothness \
		  test-alpha-animation

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
	$(GTK_CFLAGS)		\
	$(DBUS_CFLAGS)		\
	-I$(top_srcdir)/src

test_modules_LDADD =		\
	$(GLIB_LIBS)		\
	$(DBUS_LIBS)		\
	$(GTK_LIBS)		\
	-lnotify

CLUTTER_MODULES = \
	../clutter/clutter-fixed.c		\
	../clutter/clutter-units.c		\
	../clutter/clutter-timeline.c		\
	../clutter/clutter-timeout-pool.c	\
	../clutter/clutter-alpha.c	\
	../clutter/clutter-hack.c

AM_CFLAGS = \
	-Wall -Werror -std=c99  \
	$(GLIB_CFLAGS)		\
	-I$(top_srcdir)/src -I$(top_srcdir)/clutter

LDADD = $(GLIB_LIBS)

test_timeline_SOURCES = ../clutter/test-timeline.c \
	$(CLUTTER_MODULES)
test_timeline_dup_frames_SOURCES = ../clutter/test-timeline-dup-frames.c \
	$(CLUTTER_MODULES)
test_timeline_interpolate_SOURCES = ../clutter/test-timeline-interpolate.c \
	$(CLUTTER_MODULES)
test_timeline_rewind_SOURCES = ../clutter/test-timeline-rewind.c \
	$(CLUTTER_MODULES)
test_timeline_smoothness_SOURCES = ../clutter/test-timeline-smoothness.c \
	$(CLUTTER_MODULES)

test_alpha_animation_SOURCES = test-alpha-animation.c \
	$(CLUTTER_MODULES)

test: test-modules
	gtester -o=test-modules.xml -k ./test-modules

i18n: test-modules
	gtester --verbose -p=/i18n -k ./test-modules

test-report-html: test
	gtester-report test-modules.xml >test-modules.html

test-report-display: test-report-html
	gnome-open test-modules.html
