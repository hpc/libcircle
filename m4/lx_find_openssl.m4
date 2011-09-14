AC_DEFUN([X_AC_CHECK_FOR_OPENSSL], [
  AC_CHECK_LIB([ssl], [SSL_library_init], [have_openssl='yes'], [AC_MSG_FAILURE([could not find OpenSSL])])
])
