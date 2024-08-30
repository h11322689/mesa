   /*
   * Copyright (C) 2015 Rob Clark <robclark@freedesktop.org>
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   * Authors:
   *    Rob Clark <robclark@freedesktop.org>
   */
      #include "util/hash_table.h"
   #include "util/ralloc.h"
   #define XXH_INLINE_ALL
   #include "util/xxhash.h"
      #include "ir3_cache.h"
   #include "ir3_gallium.h"
      static uint32_t
   key_hash(const void *_key)
   {
      const struct ir3_cache_key *key = _key;
      }
      static bool
   key_equals(const void *_a, const void *_b)
   {
      const struct ir3_cache_key *a = _a;
   const struct ir3_cache_key *b = _b;
   // TODO we could optimize the key shader-variant key comparison by not
   // ignoring has_per_samp.. not really sure if that helps..
      }
      struct ir3_cache {
      /* cache mapping gallium/etc shader state-objs + shader-key to backend
   * specific state-object
   */
            const struct ir3_cache_funcs *funcs;
      };
      struct ir3_cache *
   ir3_cache_create(const struct ir3_cache_funcs *funcs, void *data)
   {
               cache->ht = _mesa_hash_table_create(cache, key_hash, key_equals);
   cache->funcs = funcs;
               }
      void
   ir3_cache_destroy(struct ir3_cache *cache)
   {
      if (!cache)
            /* _mesa_hash_table_destroy is so *almost* useful.. */
   hash_table_foreach (cache->ht, entry) {
                     }
      struct ir3_program_state *
   ir3_cache_lookup(struct ir3_cache *cache, const struct ir3_cache_key *key,
         {
      uint32_t hash = key_hash(key);
   struct hash_entry *entry =
            if (entry) {
                           if (key->hs)
            struct ir3_shader *shaders[MESA_SHADER_STAGES] = {
      [MESA_SHADER_VERTEX] = ir3_get_shader(key->vs),
   [MESA_SHADER_TESS_CTRL] = ir3_get_shader(key->hs),
   [MESA_SHADER_TESS_EVAL] = ir3_get_shader(key->ds),
   [MESA_SHADER_GEOMETRY] = ir3_get_shader(key->gs),
               if (shaders[MESA_SHADER_TESS_EVAL] && !shaders[MESA_SHADER_TESS_CTRL]) {
      struct ir3_shader *vs = shaders[MESA_SHADER_VERTEX];
   struct ir3_shader *hs =
                     const struct ir3_shader_variant *variants[MESA_SHADER_STAGES];
            for (gl_shader_stage stage = MESA_SHADER_VERTEX; stage < MESA_SHADER_STAGES;
      stage++) {
   if (shaders[stage]) {
      variants[stage] =
         if (!variants[stage])
      } else {
                     struct ir3_compiler *compiler = shaders[MESA_SHADER_VERTEX]->compiler;
   uint32_t safe_constlens = ir3_trim_constlen(variants, compiler);
            for (gl_shader_stage stage = MESA_SHADER_VERTEX; stage < MESA_SHADER_STAGES;
      stage++) {
   if (safe_constlens & (1 << stage)) {
      variants[stage] =
         if (!variants[stage])
                           if (ir3_has_binning_vs(&key->key)) {
      /* starting with a6xx, the same const state is used for binning and draw
   * passes, so the binning pass VS variant needs to match the main VS
   */
   shader_key.safe_constlen = (compiler->gen >= 6) &&
         bs =
         if (!bs)
      } else {
                  struct ir3_program_state *state = cache->funcs->create_state(
      cache->data, bs, variants[MESA_SHADER_VERTEX],
   variants[MESA_SHADER_TESS_CTRL], variants[MESA_SHADER_TESS_EVAL],
   variants[MESA_SHADER_GEOMETRY], variants[MESA_SHADER_FRAGMENT],
               /* NOTE: uses copy of key in state obj, because pointer passed by caller
   * is probably on the stack
   */
               }
      /* call when an API level state object is destroyed, to invalidate
   * cache entries which reference that state object.
   */
   void
   ir3_cache_invalidate(struct ir3_cache *cache, void *stobj)
   {
      if (!cache)
            hash_table_foreach (cache->ht, entry) {
      const struct ir3_cache_key *key = entry->key;
   if ((key->fs == stobj) || (key->vs == stobj) || (key->ds == stobj) ||
      (key->hs == stobj) || (key->gs == stobj)) {
   cache->funcs->destroy_state(cache->data, entry->data);
   _mesa_hash_table_remove(cache->ht, entry);
            }
