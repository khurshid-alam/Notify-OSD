noinst_PROGRAMS = test-modules

test_modules_SOURCES =		\
	../src/bubble.c		\
	../src/defaults.c	\
	../src/observer.c	\
	../src/stack.c		\
	test-modules-main.c     \
	test-bubble.c		\
	test-defaults.c		\
	test-observer.c		\
	test-stack.c

test_modules_CFLAGS =		\
	-Wall -Werror -std=c99  \
	$(GLIB_CFLAGS)		\
	$(GTK_CFLAGS)		\
	-I$(top_srcdir)/src

test_modules_LDADD =		\
	$(GLIB_LIBS)		\
	$(GTK_LIBS)

test:
	gtester -o=test-modules.xml ./test-modules

test-report-html: test
	gtester-report test-modules.xml >test-modules.html

test-report-display: test-report-html
	epiphany test-modules.html

