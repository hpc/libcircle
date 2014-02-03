AC_DEFUN([X_AC_LIBCIRCLE_DOXYGEN], [
  AC_MSG_CHECKING([for libcircle doxygen])
  AC_ARG_ENABLE(
    [doxygen],
    AS_HELP_STRING(--enable-doxygen, enable doxygen),
    [AS_IF([test "x$enable_doxygen" != "xno"],
        [ AC_CHECK_PROGS([DOXYGEN], [doxygen])
          x_ac_libcircle_doxygen=yes
        ],
        [
          x_ac_libcircle_doxygen=no
        ]
    )],
    [
      x_ac_libcircle_doxygen=no
    ]
  )
  AM_CONDITIONAL([HAVE_DOXYGEN],
    [test -n "$DOXYGEN"])
  if test [HAVE_DOXYGEN]; then 
    AC_OUTPUT([doc/Doxyfile])
  fi
])
