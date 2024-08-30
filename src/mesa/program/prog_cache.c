   /**************************************************************************
   *
   * Copyright 2003 VMware, Inc.
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
         #include "util/glheader.h"
   #include "main/shader_types.h"
      #include "main/shaderobj.h"
   #include "program/prog_cache.h"
   #include "program/program.h"
   #include "util/u_memory.h"
         struct cache_item
   {
      GLuint hash;
   unsigned keysize;
   void *key;
   struct gl_program *program;
      };
      struct gl_program_cache
   {
      struct cache_item **items;
   struct cache_item *last;
      };
            /**
   * Compute hash index from state key.
   */
   static GLuint
   hash_key(const void *key, GLuint key_size)
   {
      const GLuint *ikey = (const GLuint *) key;
                     /* Make a slightly better attempt at a hash function:
   */
   for (i = 0; i < key_size / sizeof(*ikey); i++)
   {
      hash += ikey[i];
   hash += (hash << 10);
                  }
         /**
   * Rebuild/expand the hash table to accommodate more entries
   */
   static void
   rehash(struct gl_program_cache *cache)
   {
      struct cache_item **items;
   struct cache_item *c, *next;
                     size = cache->size * 3;
   items = malloc(size * sizeof(*items));
            for (i = 0; i < cache->size; i++)
      next = c->next;
   c->next = items[c->hash % size];
   items[c->hash % size] = c;
               free(cache->items);
   cache->items = items;
      }
         static void
   clear_cache(struct gl_context *ctx, struct gl_program_cache *cache,
         {
      struct cache_item *c, *next;
                     for (i = 0; i < cache->size; i++) {
      next = c->next;
   free(c->key);
   if (shader) {
      _mesa_reference_shader_program(ctx,
      (struct gl_shader_program **)&c->program,
   } else {
         }
   FREE(c);
         }
                     }
            struct gl_program_cache *
   _mesa_new_program_cache(void)
   {
      struct gl_program_cache *cache = CALLOC_STRUCT(gl_program_cache);
   if (cache) {
      cache->size = 17;
   cache->items =
         if (!cache->items) {
      FREE(cache);
         }
      }
         void
   _mesa_delete_program_cache(struct gl_context *ctx, struct gl_program_cache *cache)
   {
      clear_cache(ctx, cache, GL_FALSE);
   free(cache->items);
      }
         struct gl_program *
   _mesa_search_program_cache(struct gl_program_cache *cache,
         {
      if (cache->last &&
      cache->last->keysize == keysize &&
   memcmp(cache->last->key, key, keysize) == 0) {
      }
   else {
      const GLuint hash = hash_key(key, keysize);
            for (c = cache->items[hash % cache->size]; c; c = c->next) {
      if (c->hash == hash &&
                     cache->last = c;
                        }
         void
   _mesa_program_cache_insert(struct gl_context *ctx,
                     {
      const GLuint hash = hash_key(key, keysize);
                     c->key = malloc(keysize);
   memcpy(c->key, key, keysize);
                     if (cache->n_items > cache->size * 1.5) {
      rehash(cache);
         clear_cache(ctx, cache, GL_FALSE);
               cache->n_items++;
   c->next = cache->items[hash % cache->size];
      }
