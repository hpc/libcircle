AC_DEFUN([X_AC_LIBCIRCLE_CHECK], [
  AC_MSG_CHECKING([for libcircle unit tests])
  AC_ARG_ENABLE(
    [tests],
    AS_HELP_STRING(--enable-tests, enable the unit tests),
    [AS_IF([test "x$enable_tests" != "xno"],
        [ PKG_CHECK_MODULES([CHECK], [check >= 0.9.4])
          x_ac_libcircle_check=yes
        ],
	[
           x_ac_libcircle_check=no
	]
    )],
    [
      x_ac_libcircle_check=no
    ]
  )
])
