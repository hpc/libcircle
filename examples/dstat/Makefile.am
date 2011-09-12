bin_PROGRAMS = dstat
dstat_SOURCES = vfprintstatf.c dstat.c
dstat_LDADD = \
    $(MPI_CLDFLAGS)                      \
    $(top_srcdir)/src/libcircle-0.0.1.la \
    $(top_srcdir)/examples/dstat/hiredis/libhiredis.a

SUBDIRS = hiredis
INCLUDES = \
    $(MPI_CFLAGS)                          \
    -I$(top_srcdir)/src                    \
    -I$(top_srcdir)/examples/dstat         \
    -I$(top_srcdir)/examples/dstat/hiredis
