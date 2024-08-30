   /*
   * Copyright (c) 2017-2019 Lima Project
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sub license,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   *
   */
      #include "util/u_memory.h"
   #include "util/ralloc.h"
   #include "util/u_debug.h"
      #include "tgsi/tgsi_dump.h"
   #include "compiler/nir/nir.h"
   #include "compiler/nir/nir_serialize.h"
   #include "nir/tgsi_to_nir.h"
   #include "nir_legacy.h"
      #include "pipe/p_state.h"
      #include "lima_screen.h"
   #include "lima_context.h"
   #include "lima_job.h"
   #include "lima_program.h"
   #include "lima_bo.h"
   #include "lima_disk_cache.h"
      #include "ir/lima_ir.h"
      static const nir_shader_compiler_options vs_nir_options = {
      .lower_ffma16 = true,
   .lower_ffma32 = true,
   .lower_ffma64 = true,
   .lower_fpow = true,
   .lower_ffract = true,
   .lower_fdiv = true,
   .lower_fmod = true,
   .lower_fsqrt = true,
   .lower_flrp32 = true,
   .lower_flrp64 = true,
   /* could be implemented by clamp */
   .lower_fsat = true,
   .lower_bitops = true,
   .lower_sincos = true,
   .lower_fceil = true,
   .lower_insert_byte = true,
   .lower_insert_word = true,
   .force_indirect_unrolling = nir_var_all,
   .force_indirect_unrolling_sampler = true,
   .lower_varying_from_uniform = true,
      };
      static const nir_shader_compiler_options fs_nir_options = {
      .lower_ffma16 = true,
   .lower_ffma32 = true,
   .lower_ffma64 = true,
   .lower_fpow = true,
   .lower_fdiv = true,
   .lower_fmod = true,
   .lower_flrp32 = true,
   .lower_flrp64 = true,
   .lower_fsign = true,
   .lower_fdot = true,
   .lower_fdph = true,
   .lower_insert_byte = true,
   .lower_insert_word = true,
   .lower_bitops = true,
   .lower_vector_cmp = true,
   .force_indirect_unrolling = (nir_var_shader_out | nir_var_function_temp),
   .force_indirect_unrolling_sampler = true,
   .lower_varying_from_uniform = true,
      };
      const void *
   lima_program_get_compiler_options(enum pipe_shader_type shader)
   {
      switch (shader) {
   case PIPE_SHADER_VERTEX:
         case PIPE_SHADER_FRAGMENT:
         default:
            }
      static int
   type_size(const struct glsl_type *type, bool bindless)
   {
         }
      void
   lima_program_optimize_vs_nir(struct nir_shader *s)
   {
               NIR_PASS_V(s, nir_lower_viewport_transform);
   NIR_PASS_V(s, nir_lower_point_size, 1.0f, 100.0f);
   NIR_PASS_V(s, nir_lower_io,
         NIR_PASS_V(s, nir_lower_load_const_to_scalar);
   NIR_PASS_V(s, lima_nir_lower_uniform_to_scalar);
   NIR_PASS_V(s, nir_lower_io_to_scalar,
            do {
               NIR_PASS_V(s, nir_lower_vars_to_ssa);
   NIR_PASS(progress, s, nir_lower_alu_to_scalar, NULL, NULL);
   NIR_PASS(progress, s, nir_lower_phis_to_scalar, false);
   NIR_PASS(progress, s, nir_copy_prop);
   NIR_PASS(progress, s, nir_opt_remove_phis);
   NIR_PASS(progress, s, nir_opt_dce);
   NIR_PASS(progress, s, nir_opt_dead_cf);
   NIR_PASS(progress, s, nir_opt_cse);
   NIR_PASS(progress, s, nir_opt_peephole_select, 8, true, true);
   NIR_PASS(progress, s, nir_opt_algebraic);
   NIR_PASS(progress, s, lima_nir_lower_ftrunc);
   NIR_PASS(progress, s, nir_opt_constant_folding);
   NIR_PASS(progress, s, nir_opt_undef);
   NIR_PASS(progress, s, nir_lower_undef_to_zero);
   NIR_PASS(progress, s, nir_opt_loop_unroll);
               NIR_PASS_V(s, nir_lower_int_to_float);
   /* int_to_float pass generates ftrunc, so lower it */
   NIR_PASS(progress, s, lima_nir_lower_ftrunc);
            NIR_PASS_V(s, nir_copy_prop);
   NIR_PASS_V(s, nir_opt_dce);
   NIR_PASS_V(s, lima_nir_split_loads);
   NIR_PASS_V(s, nir_convert_from_ssa, true);
   NIR_PASS_V(s, nir_opt_dce);
   NIR_PASS_V(s, nir_remove_dead_variables, nir_var_function_temp, NULL);
      }
      static bool
   lima_alu_to_scalar_filter_cb(const nir_instr *instr, const void *data)
   {
      if (instr->type != nir_instr_type_alu)
            nir_alu_instr *alu = nir_instr_as_alu(instr);
   switch (alu->op) {
   case nir_op_frcp:
   /* nir_op_idiv is lowered to frcp by lower_int_to_floats which
   * will be run later, so lower idiv here
   */
   case nir_op_idiv:
   case nir_op_frsq:
   case nir_op_flog2:
   case nir_op_fexp2:
   case nir_op_fsqrt:
   case nir_op_fsin:
   case nir_op_fcos:
         default:
                  /* nir vec4 fcsel assumes that each component of the condition will be
   * used to select the same component from the two options, but Utgard PP
   * has only 1 component condition. If all condition components are not the
   * same we need to lower it to scalar.
   */
   switch (alu->op) {
   case nir_op_bcsel:
   case nir_op_fcsel:
         default:
                                    for (int i = 1; i < num_components; i++)
      if (alu->src[0].swizzle[i] != swizzle)
            }
      static bool
   lima_vec_to_regs_filter_cb(const nir_instr *instr, unsigned writemask,
         {
      assert(writemask > 0);
   if (util_bitcount(writemask) == 1)
               }
      void
   lima_program_optimize_fs_nir(struct nir_shader *s,
         {
               NIR_PASS_V(s, nir_lower_fragcoord_wtrans);
   NIR_PASS_V(s, nir_lower_io,
         NIR_PASS_V(s, nir_lower_tex, tex_options);
            do {
      progress = false;
               do {
               NIR_PASS_V(s, nir_lower_vars_to_ssa);
   NIR_PASS(progress, s, nir_lower_alu_to_scalar, lima_alu_to_scalar_filter_cb, NULL);
   NIR_PASS(progress, s, nir_copy_prop);
   NIR_PASS(progress, s, nir_opt_remove_phis);
   NIR_PASS(progress, s, nir_opt_dce);
   NIR_PASS(progress, s, nir_opt_dead_cf);
   NIR_PASS(progress, s, nir_opt_cse);
   NIR_PASS(progress, s, nir_opt_peephole_select, 8, true, true);
   NIR_PASS(progress, s, nir_opt_algebraic);
   NIR_PASS(progress, s, nir_opt_constant_folding);
   NIR_PASS(progress, s, nir_opt_undef);
   NIR_PASS(progress, s, nir_opt_loop_unroll);
               NIR_PASS_V(s, nir_lower_int_to_float);
            /* Some ops must be lowered after being converted from int ops,
   * so re-run nir_opt_algebraic after int lowering. */
   do {
      progress = false;
               /* Must be run after optimization loop */
            NIR_PASS_V(s, nir_copy_prop);
            NIR_PASS_V(s, nir_convert_from_ssa, true);
            NIR_PASS_V(s, nir_move_vec_src_uses_to_dest, false);
                     NIR_PASS_V(s, lima_nir_duplicate_load_uniforms);
   NIR_PASS_V(s, lima_nir_duplicate_load_inputs);
                        }
      static bool
   lima_fs_compile_shader(struct lima_context *ctx,
                     {
      struct lima_screen *screen = lima_screen(ctx->base.screen);
            struct nir_lower_tex_options tex_options = {
      .swizzle_result = ~0u,
               for (int i = 0; i < ARRAY_SIZE(key->tex); i++) {
      for (int j = 0; j < 4; j++)
                        if (lima_debug & LIMA_DEBUG_PP)
            if (!ppir_compile_nir(fs, nir, screen->pp_ra, &ctx->base.debug)) {
      ralloc_free(nir);
               fs->state.uses_discard = nir->info.fs.uses_discard;
               }
      static bool
   lima_fs_upload_shader(struct lima_context *ctx,
         {
               fs->bo = lima_bo_create(screen, fs->state.shader_size, 0);
   if (!fs->bo) {
      fprintf(stderr, "lima: create fs shader bo fail\n");
                           }
      static struct lima_fs_compiled_shader *
   lima_get_compiled_fs(struct lima_context *ctx,
               {
      struct lima_screen *screen = lima_screen(ctx->base.screen);
   struct hash_table *ht;
            ht = ctx->fs_cache;
            struct hash_entry *entry = _mesa_hash_table_search(ht, key);
   if (entry)
            /* Not on memory cache, try disk cache */
   struct lima_fs_compiled_shader *fs =
            if (!fs) {
      /* Not on disk cache, compile and insert into disk cache*/
   fs = rzalloc(NULL, struct lima_fs_compiled_shader);
   if (!fs)
            if (!lima_fs_compile_shader(ctx, key, ufs, fs))
                        if (!lima_fs_upload_shader(ctx, fs))
            ralloc_free(fs->shader);
            /* Insert into memory cache */
   struct lima_key *dup_key;
   dup_key = rzalloc_size(fs, key_size);
   memcpy(dup_key, key, key_size);
                  err:
      ralloc_free(fs);
      }
      static void *
   lima_create_fs_state(struct pipe_context *pctx,
         {
      struct lima_context *ctx = lima_context(pctx);
            if (!so)
            nir_shader *nir;
   if (cso->type == PIPE_SHADER_IR_NIR)
      /* The backend takes ownership of the NIR shader on state
   * creation. */
      else {
                           so->base.type = PIPE_SHADER_IR_NIR;
            /* Serialize the NIR to a binary blob that we can hash for the disk
   * cache.  Drop unnecessary information (like variable names)
   * so the serialized NIR is smaller, and also to let us detect more
   * isomorphic shaders when hashing, increasing cache hits.
   */
   struct blob blob;
   blob_init(&blob);
   nir_serialize(&blob, nir, true);
   _mesa_sha1_compute(blob.data, blob.size, so->nir_sha1);
            if (lima_debug & LIMA_DEBUG_PRECOMPILE) {
      /* Trigger initial compilation with default settings */
   struct lima_fs_key key;
   memset(&key, 0, sizeof(key));
   memcpy(key.nir_sha1, so->nir_sha1, sizeof(so->nir_sha1));
   for (int i = 0; i < ARRAY_SIZE(key.tex); i++) {
      for (int j = 0; j < 4; j++)
      }
                  }
      static void
   lima_bind_fs_state(struct pipe_context *pctx, void *hwcso)
   {
               ctx->uncomp_fs = hwcso;
      }
      static void
   lima_delete_fs_state(struct pipe_context *pctx, void *hwcso)
   {
      struct lima_context *ctx = lima_context(pctx);
            hash_table_foreach(ctx->fs_cache, entry) {
      const struct lima_fs_key *key = entry->key;
   if (!memcmp(key->nir_sha1, so->nir_sha1, sizeof(so->nir_sha1))) {
      struct lima_fs_compiled_shader *fs = entry->data;
   _mesa_hash_table_remove(ctx->fs_cache, entry);
                                                ralloc_free(so->base.ir.nir);
      }
      static bool
   lima_vs_compile_shader(struct lima_context *ctx,
                     {
                        if (lima_debug & LIMA_DEBUG_GP)
            if (!gpir_compile_nir(vs, nir, &ctx->base.debug)) {
      ralloc_free(nir);
                           }
      static bool
   lima_vs_upload_shader(struct lima_context *ctx,
         {
      struct lima_screen *screen = lima_screen(ctx->base.screen);
   vs->bo = lima_bo_create(screen, vs->state.shader_size, 0);
   if (!vs->bo) {
      fprintf(stderr, "lima: create vs shader bo fail\n");
                           }
      static struct lima_vs_compiled_shader *
   lima_get_compiled_vs(struct lima_context *ctx,
               {
      struct lima_screen *screen = lima_screen(ctx->base.screen);
   struct hash_table *ht;
            ht = ctx->vs_cache;
            struct hash_entry *entry = _mesa_hash_table_search(ht, key);
   if (entry)
            /* Not on memory cache, try disk cache */
   struct lima_vs_compiled_shader *vs =
            if (!vs) {
      /* Not on disk cache, compile and insert into disk cache */
   vs = rzalloc(NULL, struct lima_vs_compiled_shader);
   if (!vs)
         if (!lima_vs_compile_shader(ctx, key, uvs, vs))
                        if (!lima_vs_upload_shader(ctx, vs))
            ralloc_free(vs->shader);
            struct lima_key *dup_key;
   dup_key = rzalloc_size(vs, key_size);
   memcpy(dup_key, key, key_size);
                  err:
      ralloc_free(vs);
      }
      bool
   lima_update_vs_state(struct lima_context *ctx)
   {
      if (!(ctx->dirty & LIMA_CONTEXT_DIRTY_UNCOMPILED_VS)) {
                  struct lima_vs_key local_key;
   struct lima_vs_key *key = &local_key;
   memset(key, 0, sizeof(*key));
   memcpy(key->nir_sha1, ctx->uncomp_vs->nir_sha1,
            struct lima_vs_compiled_shader *old_vs = ctx->vs;
   struct lima_vs_compiled_shader *vs = lima_get_compiled_vs(ctx,
               if (!vs)
                     if (ctx->vs != old_vs)
               }
      bool
   lima_update_fs_state(struct lima_context *ctx)
   {
      if (!(ctx->dirty & (LIMA_CONTEXT_DIRTY_UNCOMPILED_FS |
                        struct lima_texture_stateobj *lima_tex = &ctx->tex_stateobj;
   struct lima_fs_key local_key;
   struct lima_fs_key *key = &local_key;
   memset(key, 0, sizeof(*key));
   memcpy(key->nir_sha1, ctx->uncomp_fs->nir_sha1,
            uint8_t identity[4] = { PIPE_SWIZZLE_X, PIPE_SWIZZLE_Y,
         for (int i = 0; i < lima_tex->num_textures; i++) {
      struct lima_sampler_view *sampler = lima_sampler_view(lima_tex->textures[i]);
   if (!sampler) {
      memcpy(key->tex[i].swizzle, identity, 4);
      }
   for (int j = 0; j < 4; j++)
               /* Fill rest with identity swizzle */
   for (int i = lima_tex->num_textures; i < ARRAY_SIZE(key->tex); i++)
                     struct lima_fs_compiled_shader *fs = lima_get_compiled_fs(ctx,
               if (!fs)
                     if (ctx->fs != old_fs)
               }
      static void *
   lima_create_vs_state(struct pipe_context *pctx,
         {
      struct lima_context *ctx = lima_context(pctx);
            if (!so)
            nir_shader *nir;
   if (cso->type == PIPE_SHADER_IR_NIR)
      /* The backend takes ownership of the NIR shader on state
   * creation. */
      else {
                           so->base.type = PIPE_SHADER_IR_NIR;
            /* Serialize the NIR to a binary blob that we can hash for the disk
   * cache.  Drop unnecessary information (like variable names)
   * so the serialized NIR is smaller, and also to let us detect more
   * isomorphic shaders when hashing, increasing cache hits.
   */
   struct blob blob;
   blob_init(&blob);
   nir_serialize(&blob, nir, true);
   _mesa_sha1_compute(blob.data, blob.size, so->nir_sha1);
            if (lima_debug & LIMA_DEBUG_PRECOMPILE) {
      /* Trigger initial compilation with default settings */
   struct lima_vs_key key;
   memset(&key, 0, sizeof(key));
   memcpy(key.nir_sha1, so->nir_sha1, sizeof(so->nir_sha1));
                  }
      static void
   lima_bind_vs_state(struct pipe_context *pctx, void *hwcso)
   {
               ctx->uncomp_vs = hwcso;
      }
      static void
   lima_delete_vs_state(struct pipe_context *pctx, void *hwcso)
   {
      struct lima_context *ctx = lima_context(pctx);
            hash_table_foreach(ctx->vs_cache, entry) {
      const struct lima_vs_key *key = entry->key;
   if (!memcmp(key->nir_sha1, so->nir_sha1, sizeof(so->nir_sha1))) {
      struct lima_vs_compiled_shader *vs = entry->data;
   _mesa_hash_table_remove(ctx->vs_cache, entry);
                                                ralloc_free(so->base.ir.nir);
      }
      static uint32_t
   lima_fs_cache_hash(const void *key)
   {
         }
      static uint32_t
   lima_vs_cache_hash(const void *key)
   {
         }
      static bool
   lima_fs_cache_compare(const void *key1, const void *key2)
   {
         }
      static bool
   lima_vs_cache_compare(const void *key1, const void *key2)
   {
         }
      void
   lima_program_init(struct lima_context *ctx)
   {
      ctx->base.create_fs_state = lima_create_fs_state;
   ctx->base.bind_fs_state = lima_bind_fs_state;
            ctx->base.create_vs_state = lima_create_vs_state;
   ctx->base.bind_vs_state = lima_bind_vs_state;
            ctx->fs_cache = _mesa_hash_table_create(ctx, lima_fs_cache_hash,
         ctx->vs_cache = _mesa_hash_table_create(ctx, lima_vs_cache_hash,
      }
      void
   lima_program_fini(struct lima_context *ctx)
   {
      hash_table_foreach(ctx->vs_cache, entry) {
      struct lima_vs_compiled_shader *vs = entry->data;
   if (vs->bo)
         ralloc_free(vs);
               hash_table_foreach(ctx->fs_cache, entry) {
      struct lima_fs_compiled_shader *fs = entry->data;
   if (fs->bo)
         ralloc_free(fs);
         }
