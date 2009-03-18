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
	../src/dnd.c		\
	../src/apport.c		\
	../src/util.c		\
	$(EGG_MODULES)	\
	test-modules-main.c     \
	test-apport.c		\
	test-dbus.c		\
	test-withlib.c		\
	test-bubble.c		\
	test-defaults.c		\
	test-observer.c		\
	test-i18n.c		\
	test-synchronous.c	\
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

EGG_MODULES = \
	../egg/egg-fixed.c		\
	../egg/egg-units.c		\
	../egg/egg-timeline.c		\
	../egg/egg-timeout-pool.c	\
	../egg/egg-alpha.c	\
	../egg/egg-hack.c

AM_CFLAGS = \
	-Wall -Werror -std=c99  \
	$(GLIB_CFLAGS)		\
	-I$(top_srcdir)/src -I$(top_srcdir)/egg

LDADD = $(GLIB_LIBS)

test_timeline_SOURCES = ../egg/test-timeline.c \
	$(EGG_MODULES)
test_timeline_dup_frames_SOURCES = ../egg/test-timeline-dup-frames.c \
	$(EGG_MODULES)
test_timeline_interpolate_SOURCES = ../egg/test-timeline-interpolate.c \
	$(EGG_MODULES)
test_timeline_rewind_SOURCES = ../egg/test-timeline-rewind.c \
	$(EGG_MODULES)
test_timeline_smoothness_SOURCES = ../egg/test-timeline-smoothness.c \
	$(EGG_MODULES)

test_alpha_animation_SOURCES = test-alpha-animation.c \
	$(EGG_MODULES)

test: test-modules
	gtester -o=test-modules.xml -k ./test-modules

i18n: test-modules
	gtester --verbose -p=/i18n -k ./test-modules

test-report-html: test
	gtester-report test-modules.xml >test-modules.html

test-report-display: test-report-html
	gnome-open test-modules.html
