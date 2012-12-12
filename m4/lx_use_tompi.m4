AC_DEFUN([X_AC_LIBCIRCLE_TOMPI], [
  AC_MSG_CHECKING([for threads instead of MPI])
  AC_ARG_ENABLE(
    [threads],
    AS_HELP_STRING(--enable-threads, use threads instead of MPI),
    [
      x_ac_libcircle_tompi=yes
    ],
    [
      x_ac_libcircle_tompi=no
    ]
  )
])
