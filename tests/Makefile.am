noinst_PROGRAMS = test-modules \
		  test-timeline \
		  test-timeline-dup-frames \
		  test-timeline-interpolate \
		  test-timeline-rewind \
		  test-timeline-smoothness \
		  test-alpha-animation \
		  test-raico \
		  test-tile \
		  test-grow-bubble \
		  test-scroll-text

check_PROGRAMS = test-modules
TESTS = test-modules

GCOV_CFLAGS = -fprofile-arcs -ftest-coverage

test_modules_SOURCES =		                   \
	../src/bubble.c		                   \
	../src/defaults.c	                   \
	../src/observer.c	                   \
	../src/stack.c		                   \
	../src/dbus.c		                   \
	../src/dnd.c		                   \
	../src/apport.c		                   \
	../src/util.c				   \
	../src/stack-blur.c			   \
	../src/exponential-blur.c		   \
	../src/gaussian-blur.c			   \
	../src/raico-blur.c			   \
	../src/tile.c				   \
	../src/bubble-window.c                     \
	../src/bubble-window-accessible.c          \
	../src/bubble-window-accessible-factory.c  \
	../src/log.c		\
	$(EGG_MODULES)		\
	test-modules-main.c     \
	test-apport.c		\
	test-dbus.c		\
	test-withlib.c		\
	test-bubble.c		\
	test-defaults.c		\
	test-observer.c		\
	test-i18n.c		\
	test-synchronous.c	\
	test-dnd.c		\
	test-stack.c		\
	test-text-filtering.c

test_modules_CFLAGS =		\
	$(GCOV_CFLAGS) \
	-Wall -Werror -std=c99  \
	$(GLIB_CFLAGS)		\
	$(GTK_CFLAGS)		\
	-DWNCK_I_KNOW_THIS_IS_UNSTABLE \
	$(WNCK_CFLAGS)		\
	$(DBUS_CFLAGS)		\
	$(LIBNOTIFY_CFLAGS)	\
	-I$(top_srcdir)/src

test_modules_LDADD =		\
	$(GLIB_LIBS)		\
	$(WNCK_LIBS)		\
	$(DBUS_LIBS)		\
	$(LIBNOTIFY_LIBS)	\
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
	$(GCOV_CFLAGS) \
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

RAICO_MODULES = \
	../src/stack-blur.c \
	../src/exponential-blur.c \
	../src/gaussian-blur.c \
	../src/raico-blur.c

test_raico_SOURCES = \
	$(RAICO_MODULES) \
	test-raico.c

test_raico_CFLAGS = \
	-Wall -Werror -O0 -ggdb -std=c99  \
	$(GTK_CFLAGS) \
	-I$(top_srcdir)/src

test_raico_LDADD = \
	$(GTK_LIBS)

TILE_MODULES = \
	$(RAICO_MODULES) \
	../src/util.c \
	../src/tile.c

test_tile_SOURCES = \
	$(TILE_MODULES) \
	test-tile.c

test_tile_CFLAGS = \
	-Wall -Werror -O0 -ggdb -std=c99  \
	$(GTK_CFLAGS) \
	-I$(top_srcdir)/src

test_tile_LDADD = \
	$(GTK_LIBS)

GROW_BUBBLE_MODULES = \
	$(RAICO_MODULES) \
	../src/tile.c

test_grow_bubble_SOURCES = \
	$(TILE_MODULES) \
	test-grow-bubble.c

test_grow_bubble_CFLAGS = \
	-Wall -Werror -O0 -ggdb -std=c99  \
	$(GTK_CFLAGS) \
	-I$(top_srcdir)/src

test_grow_bubble_LDADD = \
	$(GTK_LIBS)

SCROLL_TEXT_MODULES = \
	$(RAICO_MODULES) \
	../src/tile.c

test_scroll_text_SOURCES = \
	$(TILE_MODULES) \
	test-scroll-text.c

test_scroll_text_CFLAGS = \
	-Wall -Werror -O0 -ggdb -std=c99  \
	$(GTK_CFLAGS) \
	-I$(top_srcdir)/src

test_scroll_text_LDADD = \
	$(GTK_LIBS)

test: test-modules
	gtester -o=test-modules.xml -k ./test-modules

i18n: test-modules
	gtester --verbose -p=/i18n -k ./test-modules

test-report-html: test
	gtester-report test-modules.xml >test-modules.html

test-report-display: test-report-html
	gnome-open test-modules.html
