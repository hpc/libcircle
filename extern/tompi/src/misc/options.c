/* Support for environment and command-line options */

#include <stdlib.h>
#include "mpii.h"

typedef struct
{
  char *env, *arg;
  int *val;
  enum {VAR_POS_INT, VAR_BOOL, VAR_FUNC} kind;
} Option;

/* Some local options */

static Option vars[] = {
  {"TOMPI_NTHREAD", "-nthread", &MPII_nthread, VAR_POS_INT},
  {"TOMPI_THREADINFO", "-threadinfo", (int *) MPII_Thread_info, VAR_FUNC},
  {NULL, NULL, NULL, VAR_POS_INT}
};

static void set_option (Option *o, char *val, char *where, char *what)
{
  int ival;
  switch (o->kind)
  {
    case VAR_POS_INT:
      ival = atoi (val);
      if (ival > 0)
        *(o->val) = ival;
      else
      {
        fprintf (stderr, "TOMPI warning: %s %s ignored,\n",
                 where, what);
        fprintf (stderr, "  since value `%s' is not a positive integer.", val);
      }
      break;
    case VAR_BOOL:
      *(o->val) = 1;
      break;
    case VAR_FUNC:
      ((void (*) (char *)) o->val) (val);
      break;
  }
}

/* Examines any relevant environment variables.  Must be called when in
 * single-thread mode.
 */
static void read_environment ()
{
  Option *o;

  for (o = vars; o->env; o++)
  {
    char *val;
    if ((val = (char *) getenv (o->env)))
      set_option (o, val, "Environment variable", o->env);
  }
}

/* Examines and removes any relevant command-line arguments (given by argc
 * and argv).  Must be called when in single-thread mode.
 */
static void read_arguments (int *argc, char ***argv)
{
  Option *o;
  int arg, future_arg;
  static char desc[] = "Command-line argument";

  if (argc == NULL || argv == NULL)
    return;

  for (arg = 1; arg < *argc; arg++)
    for (o = vars; o->env; o++)
      if (!strcmp ((*argv)[arg], o->arg))
      {
        int nused = 1;
        if (o->kind == VAR_BOOL || o->kind == VAR_FUNC)    /* no arguments */
          set_option (o, NULL, desc, o->arg);
        else if ((arg+1) < *argc)
        {
          nused++;
          set_option (o, (*argv)[arg+1], desc, o->arg);
        }
        else
        {
          fprintf (stderr, "TOMPI warning: %s %s ignored,\n", desc, o->arg);
          fprintf (stderr, "  since it has no corresponding value (it was the last argument).\n");
        }
        *argc -= nused;
        for (future_arg = arg; future_arg < *argc; future_arg++)
          (*argv)[future_arg] = (*argv)[future_arg+nused];
        arg--;
        break;
      }
}

PRIVATE void MPII_Read_options (int *argc, char ***argv)
{
  read_environment ();
  read_arguments (argc, argv);
}
