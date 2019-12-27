include $(top_srcdir)/common.mk

SUBDIRS = doc

# add in unit tests if we found libcheck
if HAVE_CHECK
  SUBDIRS += tests
endif

AUTOMAKE_OPTIONS = subdir-objects foreign
ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

lib_LTLIBRARIES = libcircle.la

libcircle_la_LIBADD = $(MPI_CLDFLAGS)
libcircle_la_LDFLAGS = -version-info $(LIBCIRCLE_SO_VERSION)
libcircle_la_SOURCES = \
        libcircle/lib.c \
        libcircle/queue.c \
        libcircle/token.c \
        libcircle/worker.c

libcircle_includedir = $(includedir)/
#nobase_libcircle_include_HEADERS = libcircle.h

# headers to be installed in /include
include_HEADERS = \
        libcircle/libcircle.h

# headers that should be part of distribution
# but not installed in /include
noinst_HEADERS = \
        libcircle/log.h \
        libcircle/lib.h \
        libcircle/queue.h \
        libcircle/token.h \
        libcircle/worker.h

AM_CFLAGS += $(MPI_CFLAGS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libcircle.pc

dist_noinst_SCRIPTS = autogen.sh

MAINTAINERCLEANFILES =                                  \
        aclocal.m4 config.guess config.xml              \
        config.h.in config.sub configure install-sh     \
        ltconfig ltmain.sh missing mkinstalldirs        \
        stamp-h.in

distclean-local:
	-(cd $(top_srcdir) && rm -rf autom4te*.cache autoscan.*)
	-(cd $(top_srcdir) && rm -rf $(PACKAGE)-* libcircle.pc)

mrproper: distclean-local clean
	-(cd $(top_srcdir) && rm -rf autom4te.cache config.h config.log)
	-(cd $(top_srcdir) && rm -rf config.status libtool stamp-h1)
	-(cd $(top_srcdir) && rm -rf auxdir)
	-(cd $(top_srcdir) && rm -f configure configure.ac~)
	-(cd $(top_srcdir) && rm -f aclocal.m4 config.h.in config.h.in~)
	-(cd $(top_srcdir)/m4 && rm -f libtool.m4 ltoptions.m4)
	-(cd $(top_srcdir)/m4 && rm -f ltsugar.m4 ltversion.m4)
	-(cd $(top_srcdir)/m4 && rm -f lt~obsolete.m4)
	-(cd $(top_srcdir)/doc && rm -f Doxyfile)
	-(cd $(top_srcdir)/tests && rm -f circle*.txt)
	-find $(top_srcdir)/ -name "Makefile" -exec rm {} \;
	-find $(top_srcdir)/ -name "Makefile.in" -exec rm {} \;
	-find $(top_srcdir)/ -name ".dirstamp" -exec rm {} \;
	-find $(top_srcdir) -depth -name ".deps" -exec rm -rf {} \;
