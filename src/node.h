/**
 *    author:     UncP
 *    date:    2018-08-19
 *    license:    BSD-3
**/

/**
 *   B+ tree node is k-v storage unit & internal index unit
 *
 *   layout of a node in bytes:
 *       type    level    prefix        id         keys        offset     next node    first child
 *     |   1   |   1   |     2     |     4     |     4     |     4     |      8      |      8      |
 *     |        prefix data        |                          kv paris                             |
 *     |                                     kv pairs                                              |
 *     |                                     kv pairs                                              |
 *     |                         kv pairs                              |            index          |
 *
 *
 *   layout of kv pair:
 *        key len                           ptr
 *     |     1     |        key        |     8     |
 *
 *   if node is a leaf node, ptr represents the pointer to the value or value itself
 *   if node is a internal node, ptr represents the pointer to the child nodes
 *
**/

#ifndef _node_h_
#define _node_h_

#include <stdint.h>

// node type
#define Root   0
#define Branch 1
#define Leaf   2
#define Batch  3

// op type
#define Read  0
#define Write 1

// do not fucking change it
typedef uint64_t val_t;
#define value_bytes sizeof(val_t)

#define set_val(ptr, val) ((*(val_t *)(ptr)) = (val))

// you can change uint8_t to uint16_t so that bigger keys are supported,
// but key length byte will take more space, also you need to update
// `node_min_size` below to at least 128kb
typedef uint8_t  len_t;
#define key_byte sizeof(len_t)

#define max_key_size ((uint32_t)((len_t)~((uint64_t)0)))

// you can change uint16_t to uint32_t so that bigger size nodes are supported,
// but index will take more space
typedef uint16_t index_t;
#define index_byte sizeof(index_t)

#define node_min_size  (((uint32_t)1) << 12) //  4kb
#define node_max_size  (((uint32_t)1) << 16) // 64kb
                                             // if you set `index_t` to uint32_t,
                                             // the node_max_size can be up to 4gb

typedef struct node
{
  uint32_t    type:8;   // Root or Branch or Leaf
  uint32_t   level:8;
  uint32_t     pre:16;  // prefix length
  uint32_t     id;
  uint32_t     keys;    // number of keys
  uint32_t     off;     // current data offset
  struct node *next;    // pointer to the right child
  struct node *first;   // pointer to the first child if level > 0, otherwise NULL
  char         data[0]; // to palce the prefix & the index & all the k-v pairs
}node;

void set_node_size(uint32_t size);
void set_batch_size(uint32_t size);
uint32_t get_batch_size();
int compare_key(const void *key1, uint32_t len1, const void *key2, uint32_t len2);

node* new_node(uint8_t type, uint8_t level);
void free_node(node *n);
void free_btree_node(node *n);
node* node_descend(node *n, const void *key, uint32_t len);
int node_insert(node *n, const void *key, uint32_t len, const void *val);
void* node_search(node *n, const void *key, uint32_t len);
void node_split(node *old, node *new, char *pkey, uint32_t *plen);

/**
 *   batch is a wrapper for node with some difference, key may be duplicated
 *
 *   layout of kv pair in batch:
 *            op       key len                           ptr
 *      |     1     |     1     |        key        |     8     |
**/
// TODO: different size for node and batch, batch size can be much larger than node size
typedef node batch;

batch* new_batch();
void free_batch(batch *b);
void batch_clear(batch *b);
int batch_add_write(batch *b, const void *key, uint32_t len, const void *val);
int batch_add_read(batch *b, const void *key, uint32_t len);
int batch_read_at(batch *b, uint32_t idx, uint32_t *op, void **key, uint32_t *len, void **val);

#define max_descend_depth 7 // should be enough levels for a b+ tree

// the root to leaf descending path of one kv
typedef struct path {
  uint32_t  id;                       // id of the kv in the batch
  uint32_t  depth;                    // node number in this path
  node     *nodes[max_descend_depth]; // nodes[0] is root
}path;

void path_clear(path *p);
void path_set_kv_id(path *p, uint32_t id);
uint32_t path_get_kv_id(path *p);
void path_push_node(path *p, node *n);
node* path_get_node_at_level(path *p, uint32_t level);

typedef struct fence
{
  path     *pth;                   // path that this fence belongs to
  uint32_t  len;                   // key length
  char      key[max_key_size + 1]; // key data, +1 for alignment
  node     *ptr;                   // new node pointer
}fence;

#ifdef Test

uint32_t get_node_size();
void node_print(node *n, int detail);
void batch_print(batch *b, int detail);
void node_validate(node *n);
void batch_validate(batch *n);
void btree_node_validate(node *n);

#define likely(x)   (__builtin_expect(!!(x), 1))
#define unlikely(x) (__builtin_expect(!!(x), 0))

#endif /* Test */

#endif /* _node_h_ */