include $(top_srcdir)/common.mk

SUBDIRS = extern tests doc

AUTOMAKE_OPTIONS = subdir-objects foreign
ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

lib_LTLIBRARIES = libcircle.la

if USE_BUILTIN_TOMPI
    libcircle_la_LIBADD = $(top_srcdir)/extern/tompi/libmpi.la
    AM_CFLAGS = -I$(top_srcdir)/extern/tompi/include
else
    libcircle_la_LIBADD = $(MPI_CLDFLAGS)
    AM_CFLAGS += $(MPI_CFLAGS)
endif

libcircle_la_LDFLAGS = -version-info $(LIBCIRCLE_SO_VERSION)
libcircle_la_SOURCES = libcircle/lib.c    \
                       libcircle/queue.c  \
                       libcircle/worker.c \
                       libcircle/token.c  

libcircle_includedir = $(includedir)/
nobase_libcircle_include_HEADERS = libcircle.h

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
