# Check for autoconf version.
AC_PREREQ([2.68])

AC_INIT([libcircle], [0.0.1], [github-hpc@lanl.gov])

AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_AUX_DIR(auxdir)
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE(libcircle, [0.0.1])
AC_CONFIG_HEADERS([config.h])

# Checks for programs
AC_PROG_CC
AM_PROG_CC_C_O

# Needed for tests
AM_PATH_CHECK

LT_INIT
AC_SUBST(LIBTOOL_DEPS)

# Checks for headers.
AC_CHECK_HEADERS([stdlib.h string.h])

# Checks for typedefs, structs, and compiler options.
AC_TYPE_SIZE_T

# Checks for library functs.
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC

# Check for MPI
LX_FIND_MPI
test "x$have_C_mpi" = xyes || \
    AC_MSG_ERROR([You should check to see if MPI is setup properly.])
AM_CONDITIONAL(HAVE_MPI, [test "x$have_C_mpi" = xyes])

# Check for debug mode.
AC_MSG_CHECKING([turn on debug mode with verbose output])
AC_ARG_ENABLE([debug],
              [AS_HELP_STRING([--enable-debug],
                              [Turn on debug mode with verbose output])])
test "x$enable_debug" = xyes || enable_debug=no
AC_MSG_RESULT($enable_debug)
AM_CONDITIONAL(DEBUG, [test x$enable_debug = xyes])

echo
echo "========================================================"
echo "==        libcircle: final build configuration        =="
echo "========================================================"
echo "External Library Support:"
echo "  MPI ............................................ $have_C_mpi"
echo
echo "Build Options:"
echo "  Debug enabled .................................. $enable_debug"
echo "========================================================"
echo

AC_CONFIG_FILES([Makefile \
                 src/Makefile \
                 tests/Makefile])

AC_OUTPUT
