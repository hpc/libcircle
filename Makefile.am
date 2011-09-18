## LibCircle

AUTOMAKE_OPTIONS = subdir-objects foreign
ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

lib_LTLIBRARIES = libcircle-@LIBCIRCLE_API_VERSION@.la

libcircle_@LIBCIRCLE_API_VERSION@_la_LDFLAGS = -version-info $(libcircle_SO_VERSION)
libcircle_@LIBCIRCLE_API_VERSION@_la_SOURCES = libcircle/lib.c    \
                                                libcircle/queue.c  \
                                                libcircle/worker.c \
                                                libcircle/token.c

libcircle_includedir = $(includedir)/libcircle-$(LIBCIRCLE_API_VERSION)
nobase_libcircle_include_HEADERS = libcircle.h

libcircle_libincludedir = $(libdir)/libcircle-$(LIBCIRCLE_API_VERSION)/include
nodist_libcircle_libinclude_HEADERS = libcircleconfig.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libcircle-$(LIBCIRCLE_API_VERSION).pc

dist_noinst_SCRIPTS = autogen.sh

MAINTAINERCLEANFILES =                                  \
        aclocal.m4 config.guess config.xml              \
        config.h.in config.sub configure install-sh     \
        ltconfig ltmain.sh missing mkinstalldirs        \
        stamp-h.in

distclean-local:
	-(cd $(top_srcdir) && rm -rf autom4te*.cache autoscan.*)
	-(cd $(top_srcdir) && rm -rf $(PACKAGE)-*)

mrproper: distclean-local clean
	-(cd $(top_srcdir) && rm -rf autom4te.cache config.h config.log)
	-(cd $(top_srcdir) && rm -rf config.status libtool stamp-h1)
	-(cd $(top_srcdir)/auxdir && rm -rf mkinstalldirs)
	-find $(top_srcdir)/ -name "Makefile" -exec rm {} \;
	-find $(top_srcdir) -depth -name ".deps" -exec rm -rf {} \;
