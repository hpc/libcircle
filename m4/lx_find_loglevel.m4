AC_DEFUN([X_AC_LIBCIRCLE_LOGGING], [
  AC_MSG_CHECKING([for libcircle log level])
  AC_ARG_WITH(
    [loglevel],
    AS_HELP_STRING(--with-loglevel=N, set the libcircle logging level),
    [ if test `expr "$withval" : '[[0-9]]*$'` -gt 0; then
        loglevel="$withval"
        x_ac_libcircle_loglevel=yes
      fi
    ]
  )
  if test "$x_ac_libcircle_loglevel" = yes; then
    if test $loglevel -gt 5; then
      AC_MSG_ERROR([Invalid log level $loglevel])
    fi
    AC_DEFINE_UNQUOTED(LIBCIRCLE_LOGLEVEL, [$loglevel], [Define the libcircle logging level])
  fi
])
