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
      /* Authors:  Zack Rusin <zackr@vmware.com>
   */
      #include "util/u_debug.h"
      #include "util/u_memory.h"
      #include "cso_cache.h"
   #include "cso_hash.h"
         /* Default delete callback. It can also be used by custom callbacks. */
   void
   cso_delete_state(struct pipe_context *pipe, void *state,
         {
      switch (type) {
   case CSO_BLEND:
      pipe->delete_blend_state(pipe, ((struct cso_blend*)state)->data);
      case CSO_SAMPLER:
      pipe->delete_sampler_state(pipe, ((struct cso_sampler*)state)->data);
      case CSO_DEPTH_STENCIL_ALPHA:
      pipe->delete_depth_stencil_alpha_state(pipe,
            case CSO_RASTERIZER:
      pipe->delete_rasterizer_state(pipe, ((struct cso_rasterizer*)state)->data);
      case CSO_VELEMENTS:
      pipe->delete_vertex_elements_state(pipe, ((struct cso_velements*)state)->data);
      default:
                     }
         static inline void
   sanitize_hash(struct cso_cache *sc,
               struct cso_hash *hash,
   {
      if (sc->sanitize_cb)
      }
         static inline void
   sanitize_cb(struct cso_hash *hash, enum cso_cache_type type,
         {
               /* if we're approach the maximum size, remove fourth of the entries
   * otherwise every subsequent call will go through the same */
   int hash_size = cso_hash_size(hash);
   int max_entries = (max_size > hash_size) ? max_size : hash_size;
   int to_remove =  (max_size < max_entries) * max_entries/4;
   if (hash_size > max_size)
         while (to_remove) {
      /*remove elements until we're good */
   /*fixme: currently we pick the nodes to remove at random*/
   struct cso_hash_iter iter = cso_hash_first_node(hash);
   void  *cso = cso_hash_take(hash, cso_hash_iter_key(iter));
   cache->delete_cso(cache->delete_cso_ctx, cso, type);
         }
         struct cso_hash_iter
   cso_insert_state(struct cso_cache *sc,
               {
      struct cso_hash *hash = &sc->hashes[type];
   sanitize_hash(sc, hash, type, sc->max_size);
      }
         void *
   cso_hash_find_data_from_template(struct cso_hash *hash,
                     {
      struct cso_hash_iter iter = cso_hash_find(hash, hash_key);
   while (!cso_hash_iter_is_null(iter)) {
      void *iter_data = cso_hash_iter_data(iter);
   if (!memcmp(iter_data, templ, size)) {
      /* We found a match */
      }
      }
      }
         void
   cso_cache_init(struct cso_cache *sc, struct pipe_context *pipe)
   {
               sc->max_size = 4096;
   for (int i = 0; i < CSO_CACHE_MAX; i++)
            sc->sanitize_cb = sanitize_cb;
   sc->sanitize_data = sc;
   sc->delete_cso = (cso_delete_cso_callback)cso_delete_state;
      }
         static void
   cso_delete_all(struct cso_cache *sc, enum cso_cache_type type)
   {
      struct cso_hash *hash = &sc->hashes[type];
   struct cso_hash_iter iter = cso_hash_first_node(hash);
   while (!cso_hash_iter_is_null(iter)) {
      void *state = cso_hash_iter_data(iter);
   iter = cso_hash_iter_next(iter);
   if (state) {
               }
         void
   cso_cache_delete(struct cso_cache *sc)
   {
      /* delete driver data */
   cso_delete_all(sc, CSO_BLEND);
   cso_delete_all(sc, CSO_DEPTH_STENCIL_ALPHA);
   cso_delete_all(sc, CSO_RASTERIZER);
   cso_delete_all(sc, CSO_SAMPLER);
            for (int i = 0; i < CSO_CACHE_MAX; i++)
      }
         void
   cso_set_maximum_cache_size(struct cso_cache *sc, int number)
   {
               for (int i = 0; i < CSO_CACHE_MAX; i++)
      }
         void
   cso_cache_set_sanitize_callback(struct cso_cache *sc,
               {
      sc->sanitize_cb = cb;
      }
         void
   cso_cache_set_delete_cso_callback(struct cso_cache *sc,
               {
      sc->delete_cso = delete_cso;
      }
