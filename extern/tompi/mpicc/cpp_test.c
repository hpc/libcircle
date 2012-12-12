int main (void) {
#ifdef LANGUAGE_C
   printf ("LANGUAGE_C ");
#endif
#ifdef __LANGUAGE_C__
   printf ("__LANGUAGE_C__ ");
#endif
#ifdef unix
   printf ("unix ");
#endif
#ifdef __unix__
   printf ("__unix__ ");
#endif
#ifdef __osf__
   printf ("__osf__ ");
#endif
#ifdef __alpha
   printf ("__alpha ");
#endif
#ifdef SYSTYPE_BSD
   printf ("SYSTYPE_BSD ");
#endif
#ifdef _SYSTYPE_BSD
   printf ("_SYSTYPE_BSD ");
#endif
#ifdef LANGUAGE_ASSEMBLY
   printf ("LANGUAGE_ASSEMBLY ");
#endif
#ifdef __LANGUAGE_ASSEMBLY__
   printf ("__LANGUAGE_ASSEMBLY__ ");
#endif
#ifdef _XOPEN_SOURCE
   printf ("_XOPEN_SOURCE ");
#endif
#ifdef _POSIX_SOURCE
   printf ("_POSIX_SOURCE ");
#endif
#ifdef _ANSI_C_SOURCE
   printf ("_ANSI_C_SOURCE ");
#endif
   return 0;
}
