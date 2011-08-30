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
PKG_CHECK_MODULES([CHECK], [check >= 0.9.4])

LT_INIT
AC_SUBST(LIBTOOL_DEPS)

# Checks for headers.
AC_CHECK_HEADERS([stdlib.h string.h])

# Check for the logging level.
X_AC_LIBCIRCLE_LOGGING

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

echo
echo "========================================================"
echo "==        libcircle: final build configuration        =="
echo "========================================================"
echo "External Library Support:"
echo "  MPI ............................................ $have_C_mpi"
echo
echo "Build Options:"
echo "  Logging level .................................. $logginglevel"
echo "========================================================"
echo

AC_CONFIG_FILES([Makefile \
                 src/Makefile \
                 tests/Makefile \
                 examples/Makefile \
                 examples/dstat/Makefile \
                 examples/dcopy/Makefile])

AC_OUTPUT
