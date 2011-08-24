TESTS = check_libcircle
check_PROGRAMS = check_libcircle
check_libcircle_SOURCES = check_libcircle.c $(top_builddir)/src/libcircle.h
check_libcircle_CFLAGS = -I$(top_builddir)/src/ @CHECK_CFLAGS@
check_libcircle_LDADD = $(top_builddir)/src/libcircle-0.0.1.la @CHECK_LIBS@
