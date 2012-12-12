/* Routines for manipulating hash tables */

//#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash.h"

#define myerror(str) do { fprintf (stderr, "%s", str); exit (1); } while (0)
#define PUBLIC

/* For linear traversals. */
static int llist_pos;
static Node *llist_ptr;
static HashTab llist_tab;
static HashTab_node *llist_tablist;

/* Hashes a key. */
PUBLIC int hash_func (char *name)
{
   int i, sum = 0, len = strlen (name);

   for (i = 0; i < len; i++)
      sum += name[i];

   return sum % BIGPRIME;
}

/* Returns a new hash table. */
PUBLIC HashTab hash_new (void)
{
   HashTab tab;
   int i;

   tab = (HashTab) malloc (sizeof (HashTab_real));
   if (tab == NULL)
      myerror ("hash_new: Out of memory!");

   for (i = 0; i < BIGPRIME; i++)
      tab->info[i] = NULL;

   return tab;
}

/* Adds an element to a hash table.  Assumes that it does not already exist.
 * Note that the data is not duplicated, since it is not known how!
 */
PUBLIC void hash_add (HashTab tab, char *name, void *data)
{
   int hval = hash_func (name);
   Node *p = (Node *) malloc (sizeof (Node));

   if (p == NULL)
      myerror ("hash_add: Out of memory!");

   p->name = strdup (name);
   p->data = data;
   p->next = tab->info[hval];
   tab->info[hval] = p;
}

/* Removes an element from a hash table, returning its associated data (or
 * NULL if the entry did not exist).
 */
PUBLIC void *hash_remove (HashTab tab, char *name)
{
   int hval = hash_func (name);
   Node *p, *q;

   for (p = tab->info[hval], q = NULL; p != NULL; q = p, p = p->next)
      if (!strcmp (p->name, name))
         break;

   if (p != NULL)
   {
      void *data;

      if (q == NULL)
         tab->info[hval] = p->next;
      else
         q->next = p->next;

      data = p->data;
      free (p->name);
      free (p);
      return data;
   }

   return NULL;
}

/* Searches for an entry in a hash table, returning the associated data or
 * NULL if it wasn't found.
 */
PUBLIC void *hash_find (HashTab tab, char *name)
{
   int hval = hash_func (name);
   Node *p;

   for (p = tab->info[hval]; p != NULL; p = p->next)
      if (!strcmp (p->name, name))
         break;

   if (p != NULL)
      return p->data;
   else
      return NULL;
}

/* Resets the linear traversal of a hash table. */
PUBLIC void hash_llist_reset (HashTab tab)
{
   llist_pos = -1;
   llist_ptr = NULL;
   llist_tab = tab;
}

/* Returns the next data field in the linear traversal of the hash table.
 * Entries cannot be removed or deleted between this call and the last
 * hash_llist_reset() call.
 *
 * This function is NOT THREAD SAFE and is not local to a hash table.
 *
 * Returns NULL at the end.
 */
PUBLIC void *hash_llist (void)
{
   void *data;

   while (llist_ptr == NULL)
   {
      if (++llist_pos >= BIGPRIME)
         return NULL;
      llist_ptr = llist_tab->info[llist_pos];
   }
   
   data = llist_ptr->data;

   llist_ptr = llist_ptr->next;

   return data;
}

/* Frees a hash table, destroying each element.  f is not called unless the
 * data is non-NULL.
 */
PUBLIC void hash_destroy (HashTab tab, Deleter f)
{
   int i;
   Node *p, *q;

   for (i = 0; i < BIGPRIME; i++)
      for (p = tab->info[i]; p != NULL; p = q)
      {
         q = p->next;
         free (p->name);
         if (p->data != NULL && f != NULL)
            f (p->data);
         free (p);
      }
   free (tab);
}

/* Returns a new list of hash tables. */
PUBLIC HashTab_list hashlist_new (void)
{
   HashTab_list list = (HashTab_list) malloc (sizeof (HashTab_node *));

   if (list == NULL)
      myerror ("hashlist_new: Out of memory!");

   *list = NULL;
   return list;
}

/* Pushes a hash table onto a list. */
PUBLIC void hashlist_push (HashTab_list list, HashTab tab)
{
   HashTab_node *p = (HashTab_node *) malloc (sizeof (HashTab_node));

   if (p == NULL)
      myerror ("hashlist_push: Out of memory!");

   p->tab = tab;
   p->next = *list;
   *list = p;
}

/* Pops a hash table off a list. */
PUBLIC HashTab hashlist_pop (HashTab_list list)
{
   if (*list != NULL)
   {
      HashTab tab = (*list)->tab;
      *list = (*list)->next;
      return tab;
   }
   else
      return NULL;
}

/* Pops and destroys a hash table off a list. */
PUBLIC void hashlist_destroy_one (HashTab_list list, Deleter f)
{
   if (*list != NULL)
   {
      HashTab tab = (*list)->tab;
      *list = (*list)->next;
      hash_destroy (tab, f);
   }
}

/* Destroys an entire hash table list. */
PUBLIC void hashlist_destroy (HashTab_list list, Deleter f)
{
   while (*list != NULL)
      hashlist_destroy_one (list, f);
}

/* Returns the top hash table in a list. */
PUBLIC HashTab hashlist_top (HashTab_list list)
{
   return (*list)->tab;
}

/* Adds an element to the top hash table.  Assumes that it does not already
 * exist.  Note that the data is not duplicated, since it is not known how!
 */
PUBLIC void hashlist_add (HashTab_list tab, char *name, void *data)
{
   if (*tab == NULL)
      myerror ("hashlist_add: Attempt to add to empty list of hash tables.");
   hash_add ((*tab)->tab, name, data);
}

/* Searches for an entry in several hash tables, returning the associated data
 * or NULL if it wasn't found.
 */
PUBLIC void *hashlist_find (HashTab_list tab, char *name)
{
   HashTab_node *p;
   void *result;

   for (p = *tab; p != NULL; p = p->next)
      if ((result = hash_find (p->tab, name)) != NULL)
         return result;

   return NULL;
}

/* Resets the linear traversal of a hash-table list. */
PUBLIC void hashlist_llist_reset (HashTab_list tablist)
{
   llist_tablist = *tablist;
   hash_llist_reset (llist_tablist->tab);
}

/* Returns the next data field in the linear traversal of the hash table.
 * Restrictions of hash_llist() apply here too.
 */
PUBLIC void *hashlist_llist (void)
{
   void *data;

   /* Allow returning NULL multiple times. */
   if (llist_tablist == NULL)
      return NULL;

   while ((data = hash_llist ()) == NULL)
   {
      llist_tablist = llist_tablist->next;
      if (llist_tablist == NULL)
         return NULL;
      hash_llist_reset (llist_tablist->tab);
   }

   return data;
}

