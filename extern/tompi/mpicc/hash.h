#ifndef __HASH_H__
#define __HASH_H__

#define BIGPRIME 1009

typedef struct Node_STRUCT {
   char *name;
   void *data;
   struct Node_STRUCT *next;
} Node;

typedef struct {
   Node *info[BIGPRIME];
} HashTab_real;

typedef HashTab_real *HashTab;

typedef struct HashTab_node_STRUCT {
   HashTab tab;
   struct HashTab_node_STRUCT *next;
} HashTab_node;

typedef HashTab_node **HashTab_list;

typedef void (*Deleter) (void *);


int hash_func (char *name);
HashTab hash_new (void);
void hash_add (HashTab tab, char *name, void *data);
void *hash_remove (HashTab tab, char *name);
void *hash_find (HashTab tab, char *name);
void hash_llist_reset (HashTab tab);
void *hash_llist (void);
void hash_destroy (HashTab tab, Deleter f);
HashTab_list hashlist_new (void);
void hashlist_push (HashTab_list list, HashTab tab);
HashTab hashlist_pop (HashTab_list list);
void hashlist_destroy_one (HashTab_list list, Deleter f);
void hashlist_destroy (HashTab_list list, Deleter f);
HashTab hashlist_top (HashTab_list list);
void hashlist_add (HashTab_list tab, char *name, void *data);
void *hashlist_find (HashTab_list tab, char *name);
void hashlist_llist_reset (HashTab_list tablist);
void *hashlist_llist (void);

#endif
