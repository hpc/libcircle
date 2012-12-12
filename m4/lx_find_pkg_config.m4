AC_DEFUN([X_AC_LIBCIRCLE_PKG_CONFIG], [
  AC_CHECK_PROG(HAVE_PKG_CONFIG, pkg-config, yes, no)
  if test "x$HAVE_PKG_CONFIG" = "xno"
  then
    AC_MSG_ERROR([*** Libcircle requires the pkg-config command to be available!])
  fi
  m4_ifdef([PKG_CHECK_MODULES], [], [
    AC_MSG_ERROR([*** Libcircle requires the pkg-config macros to be available!])
  ])
])
