   /**************************************************************************
   *
   * Copyright 2007 VMware, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      /*
   * Authors:
   *   Zack Rusin <zackr@vmware.com>
   */
      #include "util/u_debug.h"
   #include "util/u_memory.h"
   #include "util/bitscan.h"
      #include "cso_hash.h"
      #ifndef MAX
   #define MAX(a, b) ((a > b) ? (a) : (b))
   #endif
      static const int MinNumBits = 4;
      static const unsigned char prime_deltas[] = {
      0,  0,  1,  3,  1,  5,  3,  3,  1,  9,  7,  5,  3,  9, 25,  3,
      };
         static int
   primeForNumBits(int numBits)
   {
         }
         /*
   * Returns the smallest integer n such that
   * primeForNumBits(n) >= hint.
   */
   static int
   countBits(int hint)
   {
               if (numBits >= (int)sizeof(prime_deltas)) {
         } else if (primeForNumBits(numBits) < hint) {
         }
      }
         static struct cso_node *
   cso_hash_create_node(struct cso_hash *hash,
               {
               if (!node)
            node->key = akey;
            node->next = *anextNode;
   *anextNode = node;
   ++hash->size;
      }
         static void
   cso_data_rehash(struct cso_hash *hash, int hint)
   {
      if (hint < 0) {
      hint = countBits(-hint);
   if (hint < MinNumBits)
         hash->userNumBits = (short)hint;
   while (primeForNumBits(hint) < (hash->size >> 1))
      } else if (hint < MinNumBits) {
                  if (hash->numBits != hint) {
      struct cso_node *e = (struct cso_node *)hash;
   struct cso_node **oldBuckets = hash->buckets;
            hash->numBits = (short)hint;
   hash->numBuckets = primeForNumBits(hint);
   hash->buckets = MALLOC(sizeof(struct cso_node*) * hash->numBuckets);
   for (int i = 0; i < hash->numBuckets; ++i)
            for (int i = 0; i < oldNumBuckets; ++i) {
      struct cso_node *firstNode = oldBuckets[i];
   while (firstNode != e) {
      unsigned h = firstNode->key;
   struct cso_node *lastNode = firstNode;
                                 afterLastNode = lastNode->next;
   beforeFirstNode = &hash->buckets[h % hash->numBuckets];
   while (*beforeFirstNode != e)
         lastNode->next = *beforeFirstNode;
   *beforeFirstNode = firstNode;
         }
         }
         static void
   cso_data_might_grow(struct cso_hash *hash)
   {
      if (hash->size >= hash->numBuckets)
      }
         static void
   cso_data_has_shrunk(struct cso_hash *hash)
   {
      if (hash->size <= (hash->numBuckets >> 3) &&
      hash->numBits > hash->userNumBits) {
   int max = MAX(hash->numBits-2, hash->userNumBits);
         }
         static struct cso_node *
   cso_data_first_node(struct cso_hash *hash)
   {
      struct cso_node *e = (struct cso_node *)hash;
   struct cso_node **bucket = hash->buckets;
            while (n--) {
      if (*bucket != e)
            }
      }
         struct cso_hash_iter
   cso_hash_insert(struct cso_hash *hash, unsigned key, void *data)
   {
               struct cso_node **nextNode = cso_hash_find_node(hash, key);
   struct cso_node *node = cso_hash_create_node(hash, key, data, nextNode);
   if (!node) {
      struct cso_hash_iter null_iter = {hash, NULL};
               struct cso_hash_iter iter = {hash, node};
      }
         void
   cso_hash_init(struct cso_hash *hash)
   {
      hash->fakeNext = NULL;
   hash->buckets = NULL;
   hash->size = 0;
   hash->userNumBits = (short)MinNumBits;
   hash->numBits = 0;
   hash->numBuckets = 0;
      }
         void
   cso_hash_deinit(struct cso_hash *hash)
   {
      struct cso_node *e_for_x = hash->end;
   struct cso_node **bucket = hash->buckets;
            while (n--) {
      struct cso_node *cur = *bucket++;
   while (cur != e_for_x) {
      struct cso_node *next = cur->next;
   FREE(cur);
         }
      }
         unsigned
   cso_hash_iter_key(struct cso_hash_iter iter)
   {
      if (!iter.node || iter.hash->end == iter.node)
            }
         struct cso_node *
   cso_hash_data_next(struct cso_node *node)
   {
      union {
      struct cso_node *next;
   struct cso_node *e;
               a.next = node->next;
   if (!a.next) {
      debug_printf("iterating beyond the last element\n");
      }
   if (a.next->next)
            int start = (node->key % a.d->numBuckets) + 1;
   struct cso_node **bucket = a.d->buckets + start;
   int n = a.d->numBuckets - start;
   while (n--) {
      if (*bucket != a.e)
            }
      }
         void *
   cso_hash_take(struct cso_hash *hash, unsigned akey)
   {
               if (*node != hash->end) {
      void *t = (*node)->value;
   struct cso_node *next = (*node)->next;
   FREE(*node);
   *node = next;
   --hash->size;
   cso_data_has_shrunk(hash);
      }
      }
         struct cso_hash_iter
   cso_hash_first_node(struct cso_hash *hash)
   {
      struct cso_hash_iter iter = {hash, cso_data_first_node(hash)};
      }
         int
   cso_hash_size(const struct cso_hash *hash)
   {
         }
         struct cso_hash_iter
   cso_hash_erase(struct cso_hash *hash, struct cso_hash_iter iter)
   {
      struct cso_hash_iter ret = iter;
   struct cso_node *node = iter.node;
            if (node == hash->end)
            ret = cso_hash_iter_next(ret);
   node_ptr = &hash->buckets[node->key % hash->numBuckets];
   while (*node_ptr != node)
         *node_ptr = node->next;
   FREE(node);
   --hash->size;
      }
         bool
   cso_hash_contains(struct cso_hash *hash, unsigned key)
   {
      struct cso_node **node = cso_hash_find_node(hash, key);
      }
