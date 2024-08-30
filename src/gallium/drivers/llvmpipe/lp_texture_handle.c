   /*
   * Copyright © 2023 Valve Corporation
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "lp_context.h"
   #include "lp_texture_handle.h"
   #include "lp_screen.h"
      #include "gallivm/lp_bld_const.h"
   #include "gallivm/lp_bld_debug.h"
   #include "gallivm/lp_bld_nir.h"
      #include "nir.h"
   #include "nir_builder.h"
      #include "pipe/p_context.h"
   #include "pipe/p_screen.h"
   #include "util/mesa-sha1.h"
      static const char *image_function_base_hash = "8ca89d7a4ab5830be6a1ba1140844081235b01164a8fce8316ca6a2f81f1a899";
   static const char *sample_function_base_hash = "0789b032c4a1ddba086e07496fe2a992b1ee08f78c0884a2923564b1ed52b9cc";
   static const char *size_function_base_hash = "6d249ab9c1106c68b87ec9fdb5ade28368171d27f221c687f32ae1544231d2fe";
      static void
   llvmpipe_register_texture(struct llvmpipe_context *ctx, struct lp_static_texture_state *state, bool sampled);
      static void
   llvmpipe_register_sampler(struct llvmpipe_context *ctx, struct lp_static_sampler_state *state);
      static uint64_t
   llvmpipe_create_texture_handle(struct pipe_context *pctx, struct pipe_sampler_view *view, const struct pipe_sampler_state *sampler)
   {
      struct llvmpipe_context *ctx = llvmpipe_context(pctx);
                     if (view) {
      struct lp_static_texture_state state;
            /* Trade a bit of performance for potentially less sampler/texture combinations. */
   state.pot_width = false;
   state.pot_height = false;
                     bool found = false;
   for (uint32_t i = 0; i < matrix->texture_count; i++) {
      if (!memcmp(&matrix->textures[i]->state, &state, sizeof(struct lp_static_texture_state))) {
      handle->functions = matrix->textures[i];
   found = true;
         }
               if (sampler) {
      struct lp_static_sampler_state state;
                     bool found = false;
   for (uint32_t i = 0; i < matrix->sampler_count; i++) {
      if (!memcmp(matrix->samplers + i, &state, sizeof(struct lp_static_sampler_state))) {
      handle->sampler_index = i;
   found = true;
         }
                  }
      static void
   llvmpipe_delete_texture_handle(struct pipe_context *pctx, uint64_t _handle)
   {
      if (!_handle)
                     struct lp_texture_functions *functions = handle->functions;
   if (functions) {
      assert(functions->ref_count);
                  }
      static uint64_t
   llvmpipe_create_image_handle(struct pipe_context *pctx, const struct pipe_image_view *view)
   {
      struct llvmpipe_context *ctx = llvmpipe_context(pctx);
                     struct lp_static_texture_state state;
            /* Trade a bit of performance for potentially less sampler/texture combinations. */
   state.pot_width = false;
   state.pot_height = false;
            if (view->u.tex.first_layer == view->u.tex.last_layer) {
      if (state.target == PIPE_TEXTURE_1D_ARRAY)
         else if (state.target == PIPE_TEXTURE_2D_ARRAY || state.target == PIPE_TEXTURE_3D)
         else if (state.target == PIPE_TEXTURE_CUBE_ARRAY)
                        bool found = false;
   for (uint32_t i = 0; i < matrix->texture_count; i++) {
      if (!memcmp(&matrix->textures[i]->state, &state, sizeof(struct lp_static_texture_state))) {
      handle->functions = matrix->textures[i];
   found = true;
         }
               }
      static void
   llvmpipe_delete_image_handle(struct pipe_context *pctx, uint64_t handle)
   {
         }
      void
   llvmpipe_init_sampler_matrix(struct llvmpipe_context *ctx)
   {
      ctx->pipe.create_texture_handle = llvmpipe_create_texture_handle;
   ctx->pipe.delete_texture_handle = llvmpipe_delete_texture_handle;
   ctx->pipe.create_image_handle = llvmpipe_create_image_handle;
               }
      void
   llvmpipe_sampler_matrix_destroy(struct llvmpipe_context *ctx)
   {
                        for (uint32_t texture_index = 0; texture_index < matrix->texture_count; texture_index++) {
               uint32_t sampler_count = texture->sampler_count;
   if (texture->state.format == PIPE_FORMAT_NONE)
            for (uint32_t sampler_index = 0; sampler_index < sampler_count; sampler_index++)
            free(texture->sample_functions);
   free(texture->fetch_functions);
   free(texture->image_functions);
      }
            util_dynarray_foreach (&ctx->sampler_matrix.gallivms, struct gallivm_state *, gallivm)
               }
      static void *
   compile_function(struct llvmpipe_context *ctx, struct gallivm_state *gallivm, LLVMValueRef function,
               {
      gallivm_verify_function(gallivm, function);
                     if (needs_caching)
                                 }
      static void *
   compile_image_function(struct llvmpipe_context *ctx, struct lp_static_texture_state *texture, uint32_t op)
   {
      const struct util_format_description *desc = util_format_description(texture->format);
   if (desc->colorspace != UTIL_FORMAT_COLORSPACE_ZS && !lp_storage_render_image_format_supported(texture->format))
            bool ms = op >= LP_TOTAL_IMAGE_OP_COUNT / 2;
   if (ms)
                     params.img_op = op;
   if (op >= LP_IMG_OP_COUNT - 1) {
      params.img_op = LP_IMG_ATOMIC;
      } else if (op != LP_IMG_LOAD && op != LP_IMG_STORE) {
                  /* Loads need to support a wider range of formats for input attachments. */
   if (params.img_op != LP_IMG_LOAD)
      if (texture->format != PIPE_FORMAT_NONE && !lp_storage_image_format_supported(texture->format))
         uint8_t cache_key[SHA1_DIGEST_LENGTH];
   struct mesa_sha1 hash_ctx;
   _mesa_sha1_init(&hash_ctx);
   _mesa_sha1_update(&hash_ctx, image_function_base_hash, strlen(image_function_base_hash));
   _mesa_sha1_update(&hash_ctx, texture, sizeof(*texture));
   _mesa_sha1_update(&hash_ctx, &op, sizeof(op));
   _mesa_sha1_update(&hash_ctx, &ms, sizeof(ms));
            struct lp_cached_code cached = { 0 };
   lp_disk_cache_find_shader(llvmpipe_screen(ctx->pipe.screen), &cached, cache_key);
                     struct lp_image_static_state state = {
         };
            struct lp_type type;
   memset(&type, 0, sizeof type);
   type.floating = true;      /* floating point values */
   type.sign = true;          /* values are signed */
   type.norm = false;         /* values are not limited to [0,1] or [-1,1] */
   type.width = 32;           /* 32-bit float */
            struct lp_compute_shader_variant cs = { .gallivm = gallivm };
            params.type = type;
   params.target = texture->target;
   params.resources_type = cs.jit_resources_type;
            LLVMTypeRef function_type = lp_build_image_function_type(gallivm, &params, ms);
   if (!function_type) {
      free(image_soa);
   gallivm_destroy(gallivm);
                                          if (params.img_op != LP_IMG_LOAD)
            LLVMValueRef coords[3];
   params.coords = coords;
   for (uint32_t i = 0; i < 3; i++)
            if (ms)
            if (params.img_op != LP_IMG_LOAD)
      for (uint32_t i = 0; i < 4; i++)
         if (params.img_op == LP_IMG_ATOMIC_CAS)
      for (uint32_t i = 0; i < 4; i++)
         LLVMBuilderRef old_builder = gallivm->builder;
   LLVMBasicBlockRef block = LLVMAppendBasicBlockInContext(gallivm->context, function, "entry");
   gallivm->builder = LLVMCreateBuilderInContext(gallivm->context);
            LLVMValueRef outdata[4] = { 0 };
            for (uint32_t i = 1; i < 4; i++)
      if (!outdata[i])
         if (params.img_op != LP_IMG_STORE)
         else
            LLVMDisposeBuilder(gallivm->builder);
                        }
      static void *
   compile_sample_function(struct llvmpipe_context *ctx, struct lp_static_texture_state *texture,
         {
               bool supported = true;
   if (texture->format != PIPE_FORMAT_NONE) {
      enum lp_sampler_op_type op_type = (sample_key & LP_SAMPLER_OP_TYPE_MASK) >> LP_SAMPLER_OP_TYPE_SHIFT;
   if (op_type != LP_SAMPLER_OP_LODQ)
                  /* Skip integer formats which would cause a type mismatch in the compare function. */
   const struct util_format_description *desc = util_format_description(texture->format);
   struct lp_type texel_type = {
      .floating = true,
   .width = 32,
      };
   texel_type = lp_build_texel_type(texel_type, desc);
   if ((sample_key & LP_SAMPLER_SHADOW) && !texel_type.floating)
            if (texture_dims(texture->target) != 2 && op_type == LP_SAMPLER_OP_GATHER)
            if (op_type != LP_SAMPLER_OP_FETCH) {
      if (!sampler->normalized_coords) {
      if (texture->target != PIPE_TEXTURE_1D && texture->target != PIPE_TEXTURE_2D &&
                  if (!texture->level_zero_only)
                  if (util_format_is_pure_integer(texture->format) &&
      (sampler->min_img_filter == PIPE_TEX_FILTER_LINEAR ||
   sampler->min_mip_filter == PIPE_TEX_MIPFILTER_LINEAR ||
               if (sampler->aniso) {
                     if (util_format_is_pure_integer(texture->format))
               if (util_format_get_num_planes(texture->format) > 1)
            uint32_t bind = op_type == LP_SAMPLER_OP_FETCH ? PIPE_BIND_CONSTANT_BUFFER : PIPE_BIND_SAMPLER_VIEW;
   if (!ctx->pipe.screen->is_format_supported(ctx->pipe.screen, texture->format, texture->target, 0, 0, bind))
               uint8_t cache_key[SHA1_DIGEST_LENGTH];
   struct mesa_sha1 hash_ctx;
   _mesa_sha1_init(&hash_ctx);
   _mesa_sha1_update(&hash_ctx, sample_function_base_hash, strlen(sample_function_base_hash));
   _mesa_sha1_update(&hash_ctx, texture, sizeof(*texture));
   _mesa_sha1_update(&hash_ctx, sampler, sizeof(*sampler));
   _mesa_sha1_update(&hash_ctx, &sample_key, sizeof(sample_key));
            struct lp_cached_code cached = { 0 };
   lp_disk_cache_find_shader(llvmpipe_screen(ctx->pipe.screen), &cached, cache_key);
                     struct lp_sampler_static_state state = {
      .texture_state = *texture,
      };
            struct lp_type type;
   memset(&type, 0, sizeof type);
   type.floating = true;      /* floating point values */
   type.sign = true;          /* values are signed */
   type.norm = false;         /* values are not limited to [0,1] or [-1,1] */
   type.width = 32;           /* 32-bit float */
            struct lp_compute_shader_variant cs = { .gallivm = gallivm };
            LLVMTypeRef function_type = lp_build_sample_function_type(gallivm, sample_key);
                     gallivm->texture_descriptor = LLVMGetParam(function, arg_index++);
                     LLVMValueRef coords[5];
   for (unsigned i = 0; i < 4; i++)
            if (sample_key & LP_SAMPLER_SHADOW)
         else
            LLVMValueRef ms_index = NULL;
   if (sample_key & LP_SAMPLER_FETCH_MS)
            LLVMValueRef offsets[3] = { 0 };
   if (sample_key & LP_SAMPLER_OFFSETS)
      for (unsigned i = 0; i < 3; i++)
         LLVMValueRef lod = NULL;
   if (lod_control == LP_SAMPLER_LOD_BIAS || lod_control == LP_SAMPLER_LOD_EXPLICIT)
            LLVMBuilderRef old_builder = gallivm->builder;
   LLVMBasicBlockRef block = LLVMAppendBasicBlockInContext(gallivm->context, function, "entry");
   gallivm->builder = LLVMCreateBuilderInContext(gallivm->context);
            LLVMValueRef texel_out[4] = { 0 };
   if (supported) {
      lp_build_sample_soa_code(gallivm, texture, sampler, lp_build_sampler_soa_dynamic_state(sampler_soa),
            } else {
                           LLVMDisposeBuilder(gallivm->builder);
                        }
      static void *
   compile_size_function(struct llvmpipe_context *ctx, struct lp_static_texture_state *texture, bool samples)
   {
      uint8_t cache_key[SHA1_DIGEST_LENGTH];
   struct mesa_sha1 hash_ctx;
   _mesa_sha1_init(&hash_ctx);
   _mesa_sha1_update(&hash_ctx, size_function_base_hash, strlen(size_function_base_hash));
   _mesa_sha1_update(&hash_ctx, texture, sizeof(*texture));
   _mesa_sha1_update(&hash_ctx, &samples, sizeof(samples));
            struct lp_cached_code cached = { 0 };
   lp_disk_cache_find_shader(llvmpipe_screen(ctx->pipe.screen), &cached, cache_key);
                     struct lp_sampler_static_state state = {
         };
            struct lp_type type;
   memset(&type, 0, sizeof type);
   type.floating = true;      /* floating point values */
   type.sign = true;          /* values are signed */
   type.norm = false;         /* values are not limited to [0,1] or [-1,1] */
   type.width = 32;           /* 32-bit float */
            struct lp_compute_shader_variant cs = { .gallivm = gallivm };
            struct lp_sampler_size_query_params params = {
      .int_type = lp_int_type(type),
   .target = texture->target,
   .resources_type = cs.jit_resources_type,
   .is_sviewinfo = true,
               if (params.target == PIPE_TEXTURE_1D)
         else if (params.target == PIPE_TEXTURE_2D)
         else if (params.target == PIPE_TEXTURE_CUBE)
            LLVMTypeRef function_type = lp_build_size_function_type(gallivm, &params);
                              if (!samples)
            LLVMBuilderRef old_builder = gallivm->builder;
   LLVMBasicBlockRef block = LLVMAppendBasicBlockInContext(gallivm->context, function, "entry");
   gallivm->builder = LLVMCreateBuilderInContext(gallivm->context);
            LLVMValueRef out_sizes[4] = { 0 };
   params.sizes_out = out_sizes;
            for (uint32_t i = 0; i < 4; i++)
      if (!out_sizes[i])
                  LLVMDisposeBuilder(gallivm->builder);
                        }
      static void
   compile_sample_functions(struct llvmpipe_context *ctx, struct lp_static_texture_state *texture,
         {
      void **functions;
   if (*dst) {
         } else {
      functions = calloc(LP_SAMPLE_KEY_COUNT, sizeof(void *));
                        struct lp_static_sampler_state dummy_sampler = { 0 };
   if (!sampler)
            struct lp_sampler_matrix *matrix = &ctx->sampler_matrix;
   for (uint32_t sample_key = 0; sample_key < LP_SAMPLE_KEY_COUNT; sample_key++) {
      if (!matrix->sampler_keys[sample_key])
            enum lp_sampler_op_type op_type = (sample_key & LP_SAMPLER_OP_TYPE_MASK) >> LP_SAMPLER_OP_TYPE_SHIFT;
   if (has_sampler && op_type == LP_SAMPLER_OP_FETCH)
            if (!functions[sample_key])
         }
      static void
   llvmpipe_register_texture(struct llvmpipe_context *ctx, struct lp_static_texture_state *state, bool sampled)
   {
               bool packed = true;
   uint32_t dst_index = matrix->texture_count;
   for (uint32_t i = 0; i < matrix->texture_count; i++) {
      if (memcmp(&matrix->textures[i]->state, state, sizeof(struct lp_static_texture_state)))
                     uint32_t prev_ref_count = matrix->textures[i]->ref_count++;
   if (has_functions && prev_ref_count)
            packed = false;
   dst_index = i;
               struct lp_texture_functions *entry;
   if (packed) {
      matrix->texture_count++;
            entry = calloc(1, sizeof(struct lp_texture_functions));
            entry->ref_count = 1;
   entry->state = *state;
      } else {
                  if (sampled)
         else
            if (entry->sampled) {
      if (entry->sample_functions) {
      entry->sample_functions = realloc(entry->sample_functions, matrix->sampler_count * sizeof(void **));
      } else {
         }
            if (state->format == PIPE_FORMAT_NONE) {
      if (matrix->sampler_count)
         for (uint32_t i = 1; i < matrix->sampler_count; i++)
      } else {
      for (uint32_t i = 0; i < matrix->sampler_count; i++)
                        if (!entry->size_function)
            if (!entry->samples_function)
               if (entry->storage) {
      uint32_t image_op;
   BITSET_FOREACH_SET (image_op, matrix->image_ops, LP_TOTAL_IMAGE_OP_COUNT)
      if (!entry->image_functions[image_op])
      }
      static void
   llvmpipe_register_sampler(struct llvmpipe_context *ctx, struct lp_static_sampler_state *state)
   {
      struct lp_sampler_matrix *matrix = &ctx->sampler_matrix;
   for (uint32_t i = 0; i < matrix->sampler_count; i++)
      if (!memcmp(matrix->samplers + i, state, sizeof(struct lp_static_sampler_state)))
         matrix->sampler_count++;
                     for (uint32_t i = 0; i < matrix->texture_count; i++) {
      struct lp_texture_functions *texture = matrix->textures[i];
   if (!texture->ref_count || !texture->sampled)
            texture->sampler_count = matrix->sampler_count;
                     if (texture->state.format == PIPE_FORMAT_NONE)  {
      if (matrix->sampler_count == 1) {
      *dst = NULL;
      } else {
                              *dst = NULL;
         }
      static void
   register_sample_key(struct llvmpipe_context *ctx, uint32_t sample_key)
   {
               uint32_t prev_ref_count = matrix->sampler_keys[sample_key]++;
   if (prev_ref_count)
            for (uint32_t texture_index = 0; texture_index < matrix->texture_count; texture_index++) {
      struct lp_texture_functions *texture = matrix->textures[texture_index];
   if (!texture->ref_count || !texture->sampled)
            enum lp_sampler_op_type op_type = (sample_key & LP_SAMPLER_OP_TYPE_MASK) >> LP_SAMPLER_OP_TYPE_SHIFT;
   if (op_type == LP_SAMPLER_OP_FETCH) {
      if (!texture->fetch_functions[sample_key]) {
      struct lp_static_sampler_state dummy_sampler = { 0 };
      }
               if (texture->state.format == PIPE_FORMAT_NONE) {
      if (matrix->sampler_count && !texture->sample_functions[0][sample_key]) {
      struct lp_static_sampler_state dummy_sampler = { 0 };
      }
               for (uint32_t sampler_index = 0; sampler_index < matrix->sampler_count; sampler_index++) {
      if (!texture->sample_functions[sampler_index][sample_key]) {
      texture->sample_functions[sampler_index][sample_key] = compile_sample_function(
               }
      static void
   unregister_sample_key(struct llvmpipe_context *ctx, uint32_t sample_key)
   {
               assert(matrix->sampler_keys[sample_key]);
      }
      static void
   register_image_op(struct llvmpipe_context *ctx, uint32_t op)
   {
      struct lp_sampler_matrix *matrix = &ctx->sampler_matrix;
   if (BITSET_TEST(matrix->image_ops, op))
                     for (uint32_t texture_index = 0; texture_index < matrix->texture_count; texture_index++) {
      struct lp_texture_functions *texture = matrix->textures[texture_index];
   if (texture->ref_count && texture->storage)
         }
      struct register_shader_state {
      struct llvmpipe_context *ctx;
      };
      static bool
   register_instr(nir_builder *b, nir_instr *instr, void *_state)
   {
               if (instr->type == nir_instr_type_tex) {
      nir_tex_instr *tex = nir_instr_as_tex(instr);
            if (state->unregister)
         else
      } else if (instr->type == nir_instr_type_intrinsic) {
               struct lp_img_params params;
            if (params.img_op == -1)
            uint32_t op = params.img_op;
   if (op == LP_IMG_ATOMIC_CAS)
         else if (op == LP_IMG_ATOMIC)
            if (nir_intrinsic_image_dim(intrin) == GLSL_SAMPLER_DIM_MS ||
                                 }
      void
   llvmpipe_register_shader(struct pipe_context *ctx, const struct pipe_shader_state *shader, bool unregister)
   {
      if (shader->type != PIPE_SHADER_IR_NIR)
            struct register_shader_state state = {
      .ctx = llvmpipe_context(ctx),
      };
      }
