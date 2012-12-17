TESTS = check_queue check_checkpoint
check_PROGRAMS = check_queue check_checkpoint

check_queue_SOURCES = check_queue.c $(top_builddir)/libcircle.h
check_queue_CFLAGS = -I$(top_builddir)/libcircle/ @CHECK_CFLAGS@
check_queue_LDADD = $(top_builddir)/libcircle.la @CHECK_LIBS@

check_checkpoint_SOURCES = check_checkpoint.c $(top_builddir)/libcircle.h
check_checkpoint_CFLAGS = -I$(top_builddir)/libcircle/ @CHECK_CFLAGS@
check_checkpoint_LDADD = $(top_builddir)/libcircle.la @CHECK_LIBS@
