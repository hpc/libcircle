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

# Define the library version.
# https://autotools.io/libtool/version.html
AC_SUBST([LIBCIRCLE_SO_VERSION], [3:1:1])

# semantic versioning on API
# https://semver.org
AC_SUBST([LIBCIRCLE_API_VERSION], [0.3.0])

# Allow user to switch to tree-based termination
AC_ARG_ENABLE([treeterm],
  AS_HELP_STRING(--enable-treeterm, [switch to tree-based termination for improved scalability]),
  [case "${enableval}" in
   yes) AC_DEFINE(USE_TREETERM, 1, [tree-based termination for improved scalability])
        x_ac_libcircle_treeterm=yes ;;
   no)  x_ac_libcircle_treeterm=no ;;
   *) AC_MSG_ERROR([bad value ${enableval} for --enable-treeterm]) ;;
   esac],
  [x_ac_libcircle_treeterm=no]
)

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
echo "  Tree termination ............................... $x_ac_libcircle_treeterm"
echo "========================================================"
echo

AC_OUTPUT([Makefile      \
          tests/Makefile \
          doc/Makefile   \
          libcircle.pc:libcircle.pc.in])
