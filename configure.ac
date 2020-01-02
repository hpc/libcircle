AC_INIT([LibCircle],[0.3.0],[github-hpc@lanl.gov],[libcircle],[http://github.com/hpc/libcircle])

# Keep artifacts in directories as much as possible.
AC_CONFIG_AUX_DIR(auxdir)
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([-Wall no-define])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_CONFIG_HEADERS([config.h])

# Check for compilers.
AC_PROG_CC
AM_PROG_CC_C_O

# Check for pkg-config
X_AC_LIBCIRCLE_PKG_CONFIG

# Needed for tests
X_AC_LIBCIRCLE_CHECK
AM_CONDITIONAL([HAVE_CHECK], [test x$x_ac_libcircle_check = xyes])

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

# Define the library version for libtool -version-info.
#
# https://autotools.io/libtool/version.html
# To set the version of the library, libtool provides the -version-info parameter,
# which accepts three numbers, separated by colons, that are called respectively,
# current, revision and age.
# The rules of thumb, when dealing with these values are:
# - Always increase the revision value.
# - Increase the current value whenever an interface has been added, removed or changed.
# - Increase the age value only if the changes made to the ABI are backward compatible.
AC_SUBST([LIBCIRCLE_SO_VERSION], [3:1:1])

# Use Semantic Versioning for API
#
# https://semver.org
# Given a version number MAJOR.MINOR.PATCH, increment the:
# - MAJOR version when you make incompatible API changes,
# - MINOR version when you add functionality in a backwards compatible manner, and
# - PATCH version when you make backwards compatible bug fixes.
# Additional labels for pre-release and build metadata are available as
# extensions to the MAJOR.MINOR.PATCH format.
AC_SUBST([LIBCIRCLE_API_VERSION], [0.3.0])

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
