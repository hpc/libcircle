TESTS = check_queue
check_PROGRAMS = check_queue

check_queue_SOURCES = check_queue.c $(top_builddir)/libcircle.h
check_queue_CFLAGS = -I$(top_builddir)/libcircle/ @CHECK_CFLAGS@
check_queue_LDADD = $(top_builddir)/libcircle-@LIBCIRCLE_API_VERSION@.la @CHECK_LIBS@
