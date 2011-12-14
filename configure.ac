AC_INIT([LibCircle],[0.1],[github-hpc@lanl.gov],[libcircle],[http://github.com/hpc/libcircle])

# Keep artifacts in directories as much as possible.
AC_CONFIG_AUX_DIR(auxdir)
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([-Wall no-define])
AC_CONFIG_HEADERS([config.h])

# Check for compilers.
AC_PROG_CC
AM_PROG_CC_C_O

# Needed for tests
X_AC_LIBCIRCLE_CHECK

# Needed for docs
X_AC_LIBCIRCLE_DOXYGEN

# Pull in libtool. We need to support ancient autotools, so don't use LT_INIT.
#LT_INIT
AC_PROG_LIBTOOL

# Checks for typedefs, structs, and compiler options.
AC_TYPE_SIZE_T
AC_C_INLINE

# Checks for library functs.
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC

# Define the library version.
AC_SUBST([LIBCIRCLE_SO_VERSION], [0:0:0])
AC_SUBST([LIBCIRCLE_API_VERSION], [0.1])

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
echo "  Unit tests ..................................... $x_ac_libcircle_check"
echo "  Doxygen ........................................ $x_ac_libcircle_doxygen"
echo "========================================================"
echo

AC_OUTPUT([Makefile      \
          tests/Makefile \
          doc/Makefile   \
          libcircle.pc:libcircle.pc.in])
