AC_INIT([LibCircle], [0.1], [github-hpc@lanl.gov], [libcircle],
        [http://github.com/hpc/libcircle])
AC_PREREQ([2.59])
AM_INIT_AUTOMAKE([1.10 -Wall no-define])

AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_HEADERS([config.h libcircleconfig.h])
AC_PROG_CXX
LT_INIT([disable-static])

AC_SUBST([LIBCIRCLE_SO_VERSION], [0:0:0])
AC_SUBST([LIBCIRCLE_API_VERSION], [0.1])

AC_CONFIG_FILES([Makefile
                 libcircle-${LIBCIRCLE_API_VERSION}.pc:libcircle.pc.in])
AC_OUTPUT
