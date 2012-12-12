#include <ctype.h>
//#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "options.h"
#include "hash.h"
#include "tokens.h"

#define IDENT_LEN 512
#define TYPE_LEN 65536
#define TYPE2_LEN 1024
#define MAX_ENTRY 100
#define MAX_BRACKETS 50

HashTab ignore_ident;
HashTab_list scopes;
int prev_token = -1;

char dummy_char1 = 'T', dummy_char2 = 'N';
char *is_type = &dummy_char1, *is_nothing = &dummy_char2;
void conditional_free (char *s)
{
    if (s != is_type && s != is_nothing)
        free (s);
}

extern int column, line;
extern char yytext[], last_yytext[];

#define warn(string) fprintf (stderr, "[%d.%d] Warning: %s\n", line, column, string)
#define start_warn(string) fprintf (stderr, "[%d-%d.%d] Warning: %s\n", start_line, line, column, string)

void lappend (char *, char *);
int read_until_inbalance (char *);
void replace (void);
extern void flush_out (void);
int lex (void);

int main (void)
{
   /* Parser */
   int brace_level = 0, paren_level = 0, start_paren_level, start_brace_level;
   int star_level;
   int token;
   enum {NONE, SPECIFIERS, ENTRY, INITIALIZER} state = SPECIFIERS;
   char seen_type = 0, seen_name = 0, eat_next_rparen = 0;

   /* Temps */
   char typename[IDENT_LEN];
   char mods[256];

   /* Memory */
   char is_static = 0, is_auto = 0, is_extern = 0, is_const = 0, is_typedef = 0;
   char type[TYPE_LEN] = "";
   int nent = 1;
   struct
   {
      char name[IDENT_LEN], type2[TYPE2_LEN];
      int nstar, nparens, nbrackets;
      char is_const;
   } ent[MAX_ENTRY];
#define initialize_ent(index) \
  { \
    strcpy (ent[index].name, "<no name!>"); \
    ent[index].type2[0] = '\0'; \
    ent[index].nstar = ent[index].nparens = ent[index].nbrackets = 0; \
    ent[index].is_const = 0; \
    /* star_level isn't in ent but needs to be initialized for each entry */ \
    star_level = -1; \
  }
   initialize_ent (0);

   ignore_ident = hash_new ();
   hash_add (ignore_ident, "_iob", (void *) 1);
   hash_add (ignore_ident, "__iob", (void *) 1);
   hash_add (ignore_ident, "errno", (void *) 1);

   scopes = hashlist_new ();
   hashlist_push (scopes, hash_new ());

   printf ("typedef struct { void *key; void *oneval; } MPII_KeyOne;\n");
   printf ("extern void *MPII_Get_global (MPII_KeyOne *, int, void *);\n");

   while (1)
   {
      token = lex ();
      if (token <= 0)
          break;
      switch (state)
      {
         /* We switch to this state when we want to ignore the rest of the
          * section (e.g., statement), i.e. until we hit a semicolon, brace,
          * or right parenthesis (provided it's in a reasonable spot).  We
          * still substitute references to converted variables, of course.
          */
         case NONE: state_NONE: state = NONE;
            switch (token)
            {
               case SEMI:
                  if (paren_level > 0)          /* for loops */
                     goto not_a_start;
                  break;
               case LBRACE:
                  brace_level++;
                  hashlist_push (scopes, hash_new ());
                  break;
               case RBRACE:
                  if (brace_level-- == 0)
                  {
                     warn ("} without matching { -- ignoring");
                     brace_level = 0;
                  }
                  hashlist_destroy_one (scopes, conditional_free);
                  break;
               case LPAREN:
                  paren_level++;
                  goto not_a_start;
                  /* Do not consider this a start.  Perhaps wrong in C++
                   * (can you do for (static i = 0; ...)?  Hope not!) but
                   * otherwise we'll catch type casts which is silly.
                   */
               case RPAREN:
                  if (paren_level-- == 0)
                  {
                     warn (") without matching ( -- ignoring");
                     paren_level = 0;
                  }
                  break;
               case IDENTIFIER:
                  if (prev_token != PTR_OP && prev_token != DOT)
                      replace ();
                  goto not_a_start;
               default:
                  goto not_a_start;
            }
            state = SPECIFIERS;
            is_static = is_auto = is_extern = is_const = is_typedef = 0;
            type[0] = '\0';
            seen_type = seen_name = 0;
            nent = 1;
            initialize_ent (0);
            start_paren_level = paren_level;
            start_brace_level = brace_level;
not_a_start:
            break;

         /* In this state, we gobble up type specifiers (typedef, extern,
          * static, auto, register, and const keywords and types themselves).
          * We enter it before we've seen any, but it's now possible that we
          * can see one (e.g., after a semicolon or brace).
          */
         case SPECIFIERS: state_SPECIFIERS: state = SPECIFIERS;
            switch (token)
            {
               case TYPEDEF:
                  is_typedef = 1;
                  break;
               case EXTERN:
                  is_extern = 1;
                  break;
               case STATIC:
                  if (is_auto)
                     warn ("static after auto -- assuming static");
                  is_static = 1;
                  break;
               case AUTO:
                  if (is_static)
                     warn ("auto after static -- assuming auto");
                  is_auto = 1;
                  break;
               case REGISTER:
                  break;
               case CONST: case VOLATILE:
                  /* Consider a volatile variable constant in the sense that
                   * making it thread-specific data is nuts, unless it is
                   * just the thing being pointed to that is const or volatile.
                   */
                  lappend (type, yytext);
                  is_const = 1;
                  break;
               case VOID: case CHAR: case SHORT: case INT: case LONG:
               case FLOAT: case DOUBLE:
                  lappend (type, yytext);
                  seen_type = 1;
                  break;
               case SIGNED: case UNSIGNED:
                  lappend (type, yytext);
                  break;
               case IDENTIFIER:
                  if (hashlist_find (scopes, yytext) == is_type)
                  {
                    lappend (type, yytext);
                    if (seen_type && !is_typedef)
                      warn ("Multiple types for single declaration");
                    else
                      seen_type = 1;
                  }
                  /* Assignment or function call */
                  else if (!seen_type && brace_level > 0)
                    goto state_NONE;
                  /* Variable name in declaration, or name of function that
                   * didn't have a return type specified (defaulting to int).
                   */
                  else
                    goto state_ENTRY;
                  break;
               case MULT_OP: case LPAREN:
                  if (!seen_type)       /* Executable statement */
                     goto state_NONE;
                  goto state_ENTRY;
               case STRUCT: case UNION: case ENUM:
               {
                  lappend (type, yytext);

                  if (seen_type == 1 && !is_typedef)
                     warn ("Multiple types for single declaration");
                  else
                     seen_type = 1;
                  if ((token = lex ()) == IDENTIFIER)
                  {
                     strcpy (typename, yytext);
                     token = lex ();
                  }
                  else
                     typename[0] = '\0';
                  if (token == LBRACE)
                  {
                     int start_line = line;
                     lappend (type, yytext);
                     if (read_until_inbalance (type) != RBRACE)
                     {
                        start_warn ("Got lost looking for } ending struct/union/enum -- skipping declaration");
                        state = NONE;
                     }

#ifndef PURE_C
                     /* Make the struct/union/enum type a bonified type.
                      * This is not consistent with C (you can actually have
                      * a variable with the same name), but it is with
                      * C++ (e.g., class).
                      */
                     if (typename[0] != '\0')
                        hashlist_add (scopes, typename, is_type);
#endif
                  }
                  else
                  {
                      lappend (type, typename);
                      goto state_SPECIFIERS;
                  }
                  break;
               }
               default:
                  goto state_NONE;
            }
            break;

         /* In this state, we eat up the non-type-specifier parts of a
          * declaration.  This includes asteriks (for pointers), stuff in
          * parens (functions), and the variable names (there may be several,
          * separated by commas).
          */
         case ENTRY: state_ENTRY: state = ENTRY;
            switch (token)
            {
               case COMMA:
                  seen_name = 0;
                  initialize_ent (nent);
                  nent++;
                  break;
               case MULT_OP:    /* pointer */
                  lappend (ent[nent-1].type2, yytext);
                  if (seen_name)
                    warn ("`*' after the variable name");
                  if (paren_level > star_level)
                    star_level = paren_level;
                  /* Incrementing nstar and hence marking this as non-constant
                   * here is perhaps not the most accurate, but it works.
                   * (Concerning functions and placement of const and such...
                   * Actually, I'm not sure why nstar is used in the check
                   * for const, since is_const is reset around here.  Oh well,
                   * it's safer as it is now so I'll leave it.
                   */
                  ent[nent-1].nstar++;
                  ent[nent-1].is_const = 0;
                  while ((token = lex ()) == CONST || token == VOLATILE)
                  {
                    ent[nent-1].is_const = 1;
                    lappend (ent[nent-1].type2, yytext);
                  }
                  goto state_ENTRY;
               case EQUALS:
                  state = INITIALIZER;
                  break;
               case SEMI:
               {
                 int i;
                 if (is_typedef)
                   for (i = 0; i < nent; i++)
                     hashlist_add (scopes, ent[i].name, is_type);
                 else
                 {
                   flush_out ();

                   for (i = 0; i < nent; i++)
                                         /* Account for: */
                     if ((brace_level == 0 || is_static) &&
                                         /* Locals */
                         !(ent[i].nparens > 0) &&
                                         /* Function prototypes */
                         !(is_const && ent[i].nstar <= 0) &&
                                         /* Constant (not pointer to one) */
                         !ent[i].is_const &&
                                         /* Constant pointer */
                         !hash_find (ignore_ident, ent[i].name))
                                         /* Forbidden identifiers like errno */
                     {
                       char *s = (char *) malloc (sizeof (char) *
                         (3 * strlen (ent[i].name) + 37 +
                          strlen (type) + strlen (ent[i].type2)));
                       char indirect = (ent[i].nbrackets > 0 ? ' ' : '*');
                       if (s == NULL)
                       {
                         perror ("g2tsd");
                         exit (1);
                       }
                       sprintf (s, "(%c((%s %s%c)MPII_Get_global(&_K_%s,_L_%s,&%s)))",
                                indirect, type, ent[i].type2, indirect,
                                ent[i].name, ent[i].name, ent[i].name);
                       hashlist_add (scopes, ent[i].name, s);
                       
                       mods[0] = '\0';
                       if (is_static)
                         strcat (mods, "static ");
                       if (is_extern)
                         printf (" extern MPII_KeyOne _K_%s;extern unsigned _L_%s;",
                                 ent[i].name, ent[i].name);
                       else
                         printf (" %sMPII_KeyOne _K_%s={(void*)0,(void*)0};%sunsigned _L_%s=sizeof(%s);", mods, ent[i].name, mods, ent[i].name, ent[i].name);
                       /* DO NOT free (s); s is used in the hashtable */
                     }
                     else
                       /* If we are overriding this variable from a
                        * different scope, define it as nothing to
                        * (if needed) undefine it as a global variable.
                        */
                       hashlist_add (scopes, ent[i].name, is_nothing);
                 }

                 /* Initialize and go to SPECIFIERS state. */
                 goto state_NONE;
               }
               case TYPEDEF: case EXTERN: case STATIC: case AUTO:
               case REGISTER: case CONST: case VOLATILE: case VOID: case CHAR:
               case SHORT: case INT: case LONG: case FLOAT: case DOUBLE:
               case SIGNED: case UNSIGNED: case STRUCT: case UNION: case ENUM:
arg_declaration:
               {
                 int start_line = line;
                 /* This must be a declaration of an argument in old
                  * K&R style C.  We will never have to convert such an
                  * argument since it is allocated on the stack.  Hence,
                  * we can completely ignore everything before the next
                  * left brace (which marks the start of the function).
                  */
                 while ((token = lex ()) != LBRACE)
                   if (token == 0)
                   {
                     start_warn ("Hit end of file while reading K&R declaration; was looking for { -- aborting");
                     exit (1);
                   }
                 goto state_ENTRY; /* this actually goes to state_NONE */
               }
               case IDENTIFIER:
                  if (hashlist_find (scopes, yytext) == is_type)
                    /* Old K&R-style function argument declaration */
                    goto arg_declaration;
                  if (seen_name)
                  {
                    warn ("Two names for a variable; using latter one");
                    fprintf (stderr, "    Name was `%s', now `%s'\n",
                             ent[nent-1].name, yytext);
                  }
                  else
                  {
                    int len = strlen (ent[nent-1].type2);
                    if (len > 0 && ent[nent-1].type2[len-1] == '(')
                    {
                      eat_next_rparen = 1;
                      ent[nent-1].type2[len-1] = '\0';
                    }
                    else
                      eat_next_rparen = 0;
                  }
                  seen_name = 1;
                  strcpy (ent[nent-1].name, yytext);
                  break;
               case LPAREN:
                  lappend (ent[nent-1].type2, yytext);
                  if (seen_name)
                  {
                     int start_line = line;
                     if (read_until_inbalance (ent[nent-1].type2) != RPAREN)
                     {
                        start_warn ("Got lost looking for ) ending parameter list -- skipping declaration");
                        state = NONE;
                     }
                     /* If star_level > paren_level, this is a pointer to a
                      * function, not a function.  Need convincing?  Consider
                      * the following examples.
                      *   void *f ();              --function
                      *   void (*f) ();            --pointer to function
                      *   void (*signal (...)) ... --function
                      */
                     if (star_level <= paren_level)
                       ent[nent-1].nparens++;
                  }
                  else
                     paren_level++;
                  break;
               case RPAREN:
                  if (eat_next_rparen)
                      eat_next_rparen = 0;
                  else
                      lappend (ent[nent-1].type2, yytext);
                  if (paren_level-- == 0)
                  {
                     warn (") without matching ( -- ignoring");
                     paren_level = 0;
                  }
                  /* Here we catch things like:
                   *    typedef void (*foo) (int i);
                   *                         ^^^^^*
                   * This would only happen if ( was a character like ; and {.
                   */
                  if (paren_level < start_paren_level)
                     state = NONE;
                  break;
               case LBRACKET:
               {
                  int start_line = line;
                  lappend (ent[nent-1].type2, "*");
                  if (read_until_inbalance (NULL) != RBRACKET)
                  {
                     start_warn ("Got lost looking for ] ending array dimension -- skipping declaration");
                     state = NONE;
                  }
                  ent[nent-1].nbrackets++;
                  break;
               }
               default:
                  goto state_NONE;
            }
            break;

         /* Here we eat the initializer of a variable.  This is basically
          * a temporary state until we return back to state ENTRY with a
          * comma or semicolon (which are interpreted by state ENTRY, not us).
          */
         case INITIALIZER: state_INITIALIZER: state = INITIALIZER;
            switch (token)
            {
               case LBRACE:
                  brace_level++;
                  break;
               case RBRACE:
                  if (brace_level-- == 0)
                  {
                     warn ("} without matching { -- ignoring");
                     brace_level = 0;
                  }
                  break;
               case LPAREN:
                  paren_level++;
                  break;
               case RPAREN:
                  if (paren_level-- == 0)
                  {
                     warn (") without matching ( -- ignoring");
                     paren_level = 0;
                  }
                  break;
               case COMMA: case SEMI:
                  if (paren_level == start_paren_level &&
                      brace_level == start_brace_level)
                     goto state_ENTRY;
                  break;
            }
            break;
      }
   }

   flush_out ();
   /*fprintf (stderr, "Done!\n");*/
   return 0;
}

void replace (void)
{
    char *info = hashlist_find (scopes, yytext);
    if (info != NULL && info != is_type && info != is_nothing)
    {
        last_yytext[0] = '\0';
        printf ("%s", info);
    }
}

/* Equivalent to "lappend $s $add" in Tcl, or the following in Python:
 *     if len(s) == 0:
 *         s = add
 *     else:
 *         s = s + " " + add
 * or the following in C:
 *     if (*s == '\0')
 *         strcpy (s, add);
 *     else
 *         strcat (strcat (s, " "), add);
 * but about twice as efficient in the second case.
 * Also checks for out-of-bounds condition (according to TYPE_LEN).
 */
void lappend (char *s, char *add)
{
    if (s == NULL)
        return;
    if (*s == '\0')
        strcpy (s, add);
    else
    {
        int cnt = 0;
        while (*s != '\0')
        {
            cnt++;
            s++;
        }
        *(s++) = ' ';
        while (*add != '\0')
        {
            cnt++;
            if (cnt >= TYPE2_LEN)
            {
                fprintf (stderr, "Type too long; increase TYPE_LEN\n");
                exit (1);
            }
            *(s++) = *(add++);
        }
        *s = '\0';
    }
}

int read_until_inbalance (char *addto)
{
   int parens = 0, brackets = 0, braces = 0, done = 0;
   int token;

   while (1)
   {
      token = lex ();
      if (token <= 0)
          return -1;
      lappend (addto, yytext);
      switch (token)
      {
         case LPAREN: parens++; break;
         case RPAREN: if (parens-- <= 0) done = 1; break;
         case LBRACKET: brackets++; break;
         case RBRACKET: if (brackets-- <= 0) done = 1; break;
         case LBRACE: braces++; break;
         case RBRACE: if (braces-- <= 0) done = 1; break;
      }
      if (done)
          return token;
   }
}

int lex (void)
{
    static int last_token = -1;
    prev_token = last_token;
    return (last_token = yylex ());
}

