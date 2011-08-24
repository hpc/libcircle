TESTS = check_queue
check_PROGRAMS = check_queue

check_queue_SOURCES = check_queue.c $(top_builddir)/src/libcircle.h
check_queue_CFLAGS = -I$(top_builddir)/src/ @CHECK_CFLAGS@
check_queue_LDADD = $(top_builddir)/src/libcircle-0.0.1.la @CHECK_LIBS@
