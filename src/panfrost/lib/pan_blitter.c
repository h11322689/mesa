   /*
   * Copyright (C) 2020-2021 Collabora, Ltd.
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
   *   Alyssa Rosenzweig <alyssa.rosenzweig@collabora.com>
   *   Boris Brezillon <boris.brezillon@collabora.com>
   */
      #include "pan_blitter.h"
   #include <math.h>
   #include <stdio.h>
   #include "compiler/nir/nir_builder.h"
   #include "util/u_math.h"
   #include "pan_blend.h"
   #include "pan_cs.h"
   #include "pan_encoder.h"
   #include "pan_pool.h"
   #include "pan_scoreboard.h"
   #include "pan_shader.h"
   #include "pan_texture.h"
      #if PAN_ARCH >= 6
   /* On Midgard, the native blit infrastructure (via MFBD preloads) is broken or
   * missing in many cases. We instead use software paths as fallbacks to
   * implement blits, which are done as TILER jobs. No vertex shader is
   * necessary since we can supply screen-space coordinates directly.
   *
   * This is primarily designed as a fallback for preloads but could be extended
   * for other clears/blits if needed in the future. */
      static enum mali_register_file_format
   blit_type_to_reg_fmt(nir_alu_type in)
   {
      switch (in) {
   case nir_type_float32:
         case nir_type_int32:
         case nir_type_uint32:
         default:
            }
   #endif
      struct pan_blit_surface {
      gl_frag_result loc              : 4;
   nir_alu_type type               : 8;
   enum mali_texture_dimension dim : 2;
   bool array                      : 1;
   unsigned src_samples            : 5;
      };
      struct pan_blit_shader_key {
         };
      struct pan_blit_shader_data {
      struct pan_blit_shader_key key;
   struct pan_shader_info info;
   mali_ptr address;
   unsigned blend_ret_offsets[8];
      };
      struct pan_blit_blend_shader_key {
      enum pipe_format format;
   nir_alu_type type;
   unsigned rt         : 3;
   unsigned nr_samples : 5;
      };
      struct pan_blit_blend_shader_data {
      struct pan_blit_blend_shader_key key;
      };
      struct pan_blit_rsd_key {
      struct {
      enum pipe_format format;
   nir_alu_type type               : 8;
   unsigned src_samples            : 5;
   unsigned dst_samples            : 5;
   enum mali_texture_dimension dim : 2;
         };
      struct pan_blit_rsd_data {
      struct pan_blit_rsd_key key;
      };
      #if PAN_ARCH >= 5
   static void
   pan_blitter_emit_blend(const struct panfrost_device *dev, unsigned rt,
                     {
               pan_pack(out, BLEND, cfg) {
      if (!iview) {
   #if PAN_ARCH >= 6
         #endif
                        cfg.round_to_fb_precision = true;
      #if PAN_ARCH >= 6
         #endif
            if (!blend_shader) {
      cfg.equation.rgb.a = MALI_BLEND_OPERAND_A_SRC;
   cfg.equation.rgb.b = MALI_BLEND_OPERAND_B_SRC;
   cfg.equation.rgb.c = MALI_BLEND_OPERAND_C_ZERO;
   cfg.equation.alpha.a = MALI_BLEND_OPERAND_A_SRC;
   cfg.equation.alpha.b = MALI_BLEND_OPERAND_B_SRC;
         #if PAN_ARCH >= 6
                     cfg.internal.fixed_function.num_comps = 4;
   cfg.internal.fixed_function.conversion.memory_format =
                     #endif
         #if PAN_ARCH <= 5
               #endif
               }
   #endif
      struct pan_blitter_views {
      unsigned rt_count;
   const struct pan_image_view *src_rts[8];
   const struct pan_image_view *dst_rts[8];
   const struct pan_image_view *src_z;
   const struct pan_image_view *dst_z;
   const struct pan_image_view *src_s;
      };
      static bool
   pan_blitter_is_ms(struct pan_blitter_views *views)
   {
      for (unsigned i = 0; i < views->rt_count; i++) {
      if (views->dst_rts[i]) {
      if (pan_image_view_get_nr_samples(views->dst_rts[i]) > 1)
                  if (views->dst_z && pan_image_view_get_nr_samples(views->dst_z) > 1)
            if (views->dst_s && pan_image_view_get_nr_samples(views->dst_s) > 1)
               }
      #if PAN_ARCH >= 5
   static void
   pan_blitter_emit_blends(const struct panfrost_device *dev,
                     {
      for (unsigned i = 0; i < MAX2(views->rt_count, 1); ++i) {
      void *dest = out + pan_size(BLEND) * i;
   const struct pan_image_view *rt_view = views->dst_rts[i];
                  }
   #endif
      #if PAN_ARCH <= 7
   static void
   pan_blitter_emit_rsd(const struct panfrost_device *dev,
                     {
      UNUSED bool zs = (views->dst_z || views->dst_s);
            pan_pack(out, RENDERER_STATE, cfg) {
      assert(blit_shader->address);
            cfg.multisample_misc.sample_mask = 0xFFFF;
   cfg.multisample_misc.multisample_enable = ms;
   cfg.multisample_misc.evaluate_per_sample = ms;
   cfg.multisample_misc.depth_write_mask = views->dst_z != NULL;
            cfg.stencil_mask_misc.stencil_enable = views->dst_s != NULL;
   cfg.stencil_mask_misc.stencil_mask_front = 0xFF;
   cfg.stencil_mask_misc.stencil_mask_back = 0xFF;
   cfg.stencil_front.compare_function = MALI_FUNC_ALWAYS;
   cfg.stencil_front.stencil_fail = MALI_STENCIL_OP_REPLACE;
   cfg.stencil_front.depth_fail = MALI_STENCIL_OP_REPLACE;
   cfg.stencil_front.depth_pass = MALI_STENCIL_OP_REPLACE;
   cfg.stencil_front.mask = 0xFF;
      #if PAN_ARCH >= 6
         if (zs) {
      /* Writing Z/S requires late updates */
   cfg.properties.zs_update_operation = MALI_PIXEL_KILL_FORCE_LATE;
      } else {
      /* Skipping ATEST requires forcing Z/S */
   cfg.properties.zs_update_operation = MALI_PIXEL_KILL_STRONG_EARLY;
               /* However, while shaders writing Z/S can normally be killed, on v6
   * for frame shaders it can cause GPU timeouts, so only allow colour
   * blit shaders to be killed. */
            if (PAN_ARCH == 6)
   #else
            mali_ptr blend_shader =
      blend_shaders
               cfg.properties.work_register_count = 4;
   cfg.properties.force_early_z = !zs;
            #if PAN_ARCH == 5
         #else
         cfg.blend_shader = blend_shader;
   cfg.stencil_mask_misc.write_enable = true;
   cfg.stencil_mask_misc.dither_disable = true;
   cfg.multisample_misc.blend_shader = !!blend_shader;
   cfg.blend_shader = blend_shader;
   if (!cfg.multisample_misc.blend_shader) {
      cfg.blend_equation.rgb.a = MALI_BLEND_OPERAND_A_SRC;
   cfg.blend_equation.rgb.b = MALI_BLEND_OPERAND_B_SRC;
   cfg.blend_equation.rgb.c = MALI_BLEND_OPERAND_C_ZERO;
   cfg.blend_equation.alpha.a = MALI_BLEND_OPERAND_A_SRC;
   cfg.blend_equation.alpha.b = MALI_BLEND_OPERAND_B_SRC;
                  if (views->dst_rts[0] != NULL) {
      cfg.stencil_mask_misc.srgb =
               #endif
   #endif
            #if PAN_ARCH >= 5
      pan_blitter_emit_blends(dev, blit_shader, views, blend_shaders,
      #endif
   }
   #endif
      #if PAN_ARCH <= 5
   static void
   pan_blitter_get_blend_shaders(struct panfrost_device *dev, unsigned rt_count,
                     {
      if (!rt_count)
            struct pan_blend_state blend_state = {
                  for (unsigned i = 0; i < rt_count; i++) {
      if (!rts[i] || panfrost_blendable_formats_v7[rts[i]->format].internal)
            struct pan_blit_blend_shader_key key = {
      .format = rts[i]->format,
   .rt = i,
   .nr_samples = pan_image_view_get_nr_samples(rts[i]),
               pthread_mutex_lock(&dev->blitter.shaders.lock);
   struct hash_entry *he =
         struct pan_blit_blend_shader_data *blend_shader = he ? he->data : NULL;
   if (blend_shader) {
      blend_shaders[i] = blend_shader->address;
   pthread_mutex_unlock(&dev->blitter.shaders.lock);
               blend_shader =
                  blend_state.rts[i] = (struct pan_blend_rt_state){
      .format = rts[i]->format,
   .nr_samples = pan_image_view_get_nr_samples(rts[i]),
   .equation =
      {
      .blend_enable = false,
               pthread_mutex_lock(&dev->blend_shaders.lock);
   struct pan_blend_shader_variant *b = GENX(pan_blend_get_shader_locked)(
      dev, &blend_state, blit_shader->blend_types[i],
               assert(b->work_reg_count <= 4);
   struct panfrost_ptr bin =
                  blend_shader->address = bin.gpu | b->first_tag;
   pthread_mutex_unlock(&dev->blend_shaders.lock);
   _mesa_hash_table_insert(dev->blitter.shaders.blend, &blend_shader->key,
         pthread_mutex_unlock(&dev->blitter.shaders.lock);
         }
   #endif
      /*
   * Early Mali GPUs did not respect sampler LOD clamps or bias, so the Midgard
   * compiler inserts lowering code with a load_sampler_lod_parameters_pan sysval
   * that we need to lower. Our samplers do not use LOD clamps or bias, so we
   * lower to the identity settings and let constant folding get rid of the
   * unnecessary lowering.
   */
   static bool
   lower_sampler_parameters(nir_builder *b, nir_intrinsic_instr *intr,
         {
      if (intr->intrinsic != nir_intrinsic_load_sampler_lod_parameters_pan)
            const nir_const_value constants[4] = {
      nir_const_value_for_float(0.0f, 32),     /* min_lod */
   nir_const_value_for_float(INFINITY, 32), /* max_lod */
               b->cursor = nir_after_instr(&intr->instr);
   nir_def_rewrite_uses(&intr->def, nir_build_imm(b, 3, 32, constants));
      }
      static const struct pan_blit_shader_data *
   pan_blitter_get_blit_shader(struct panfrost_device *dev,
         {
      pthread_mutex_lock(&dev->blitter.shaders.lock);
   struct hash_entry *he =
                  if (shader)
            unsigned coord_comps = 0;
   unsigned sig_offset = 0;
   char sig[256];
   bool first = true;
   for (unsigned i = 0; i < ARRAY_SIZE(key->surfaces); i++) {
      const char *type_str, *dim_str;
   if (key->surfaces[i].type == nir_type_invalid)
            switch (key->surfaces[i].type) {
   case nir_type_float32:
      type_str = "float";
      case nir_type_uint32:
      type_str = "uint";
      case nir_type_int32:
      type_str = "int";
      default:
                  switch (key->surfaces[i].dim) {
   case MALI_TEXTURE_DIMENSION_CUBE:
      dim_str = "cube";
      case MALI_TEXTURE_DIMENSION_1D:
      dim_str = "1D";
      case MALI_TEXTURE_DIMENSION_2D:
      dim_str = "2D";
      case MALI_TEXTURE_DIMENSION_3D:
      dim_str = "3D";
      default:
                  coord_comps = MAX2(coord_comps, (key->surfaces[i].dim ?: 3) +
                  if (sig_offset >= sizeof(sig))
            sig_offset +=
      snprintf(sig + sig_offset, sizeof(sig) - sig_offset,
            "%s[%s;%s;%s%s;src_samples=%d,dst_samples=%d]",
               nir_builder b = nir_builder_init_simple_shader(
      MESA_SHADER_FRAGMENT, GENX(pan_shader_get_compiler_options)(),
      nir_variable *coord_var = nir_variable_create(
      b.shader, nir_var_shader_in,
                        unsigned active_count = 0;
   for (unsigned i = 0; i < ARRAY_SIZE(key->surfaces); i++) {
      if (key->surfaces[i].type == nir_type_invalid)
            /* Resolve operations only work for N -> 1 samples. */
   assert(key->surfaces[i].dst_samples == 1 ||
            static const char *out_names[] = {
                  unsigned ncomps = key->surfaces[i].loc >= FRAG_RESULT_DATA0 ? 4 : 1;
   enum glsl_base_type type =
         nir_variable *out = nir_variable_create(b.shader, nir_var_shader_out,
               out->data.location = key->surfaces[i].loc;
            bool resolve =
         bool ms = key->surfaces[i].src_samples > 1;
            switch (key->surfaces[i].dim) {
   case MALI_TEXTURE_DIMENSION_1D:
      sampler_dim = GLSL_SAMPLER_DIM_1D;
      case MALI_TEXTURE_DIMENSION_2D:
      sampler_dim = ms ? GLSL_SAMPLER_DIM_MS : GLSL_SAMPLER_DIM_2D;
      case MALI_TEXTURE_DIMENSION_3D:
      sampler_dim = GLSL_SAMPLER_DIM_3D;
      case MALI_TEXTURE_DIMENSION_CUBE:
      sampler_dim = GLSL_SAMPLER_DIM_CUBE;
                        if (resolve) {
      /* When resolving a float type, we need to calculate
   * the average of all samples. For integer resolve, GL
   * and Vulkan say that one sample should be chosen
   * without telling which. Let's just pick the first one
   * in that case.
   */
   nir_alu_type base_type =
                                          tex->op = nir_texop_txf_ms;
   tex->dest_type = key->surfaces[i].type;
   tex->texture_index = active_count;
                  tex->src[0] =
                                 tex->src[2] =
                                    if (base_type == nir_type_float)
      } else {
               tex->dest_type = key->surfaces[i].type;
   tex->texture_index = active_count;
                                    tex->src[0] =
                                 tex->src[2] =
                        tex->src[0] = nir_tex_src_for_ssa(nir_tex_src_coord, coord);
               nir_def_init(&tex->instr, &tex->def, 4, 32);
   nir_builder_instr_insert(&b, &tex->instr);
                        if (key->surfaces[i].loc >= FRAG_RESULT_DATA0) {
         } else {
      unsigned c = key->surfaces[i].loc == FRAG_RESULT_STENCIL ? 1 : 0;
      }
               struct panfrost_compile_inputs inputs = {
      .gpu_id = dev->gpu_id,
   .is_blit = true,
      };
                                       for (unsigned i = 0; i < active_count; ++i)
                     if (PAN_ARCH == 4) {
      NIR_PASS_V(b.shader, nir_shader_intrinsics_pass, lower_sampler_parameters,
                        shader->key = *key;
   shader->address =
      pan_pool_upload_aligned(dev->blitter.shaders.pool, binary.data,
         util_dynarray_fini(&binary);
         #if PAN_ARCH >= 6
      for (unsigned i = 0; i < ARRAY_SIZE(shader->blend_ret_offsets); i++) {
      shader->blend_ret_offsets[i] =
               #endif
               out:
      pthread_mutex_unlock(&dev->blitter.shaders.lock);
      }
      static struct pan_blit_shader_key
   pan_blitter_get_key(struct pan_blitter_views *views)
   {
               if (views->src_z) {
      assert(views->dst_z);
   key.surfaces[0].loc = FRAG_RESULT_DEPTH;
   key.surfaces[0].type = nir_type_float32;
   key.surfaces[0].src_samples = pan_image_view_get_nr_samples(views->src_z);
   key.surfaces[0].dst_samples = pan_image_view_get_nr_samples(views->dst_z);
   key.surfaces[0].dim = views->src_z->dim;
   key.surfaces[0].array =
               if (views->src_s) {
      assert(views->dst_s);
   key.surfaces[1].loc = FRAG_RESULT_STENCIL;
   key.surfaces[1].type = nir_type_uint32;
   key.surfaces[1].src_samples = pan_image_view_get_nr_samples(views->src_s);
   key.surfaces[1].dst_samples = pan_image_view_get_nr_samples(views->dst_s);
   key.surfaces[1].dim = views->src_s->dim;
   key.surfaces[1].array =
               for (unsigned i = 0; i < views->rt_count; i++) {
      if (!views->src_rts[i])
            assert(views->dst_rts[i]);
   key.surfaces[i].loc = FRAG_RESULT_DATA0 + i;
   key.surfaces[i].type =
      util_format_is_pure_uint(views->src_rts[i]->format) ? nir_type_uint32
   : util_format_is_pure_sint(views->src_rts[i]->format)
      ? nir_type_int32
   key.surfaces[i].src_samples =
         key.surfaces[i].dst_samples =
         key.surfaces[i].dim = views->src_rts[i]->dim;
   key.surfaces[i].array =
                  }
      #if PAN_ARCH <= 7
   static mali_ptr
   pan_blitter_get_rsd(struct panfrost_device *dev,
         {
                                 if (views->src_z) {
      assert(views->dst_z);
   rsd_key.z.format = views->dst_z->format;
   rsd_key.z.type = blit_key.surfaces[0].type;
   rsd_key.z.src_samples = blit_key.surfaces[0].src_samples;
   rsd_key.z.dst_samples = blit_key.surfaces[0].dst_samples;
   rsd_key.z.dim = blit_key.surfaces[0].dim;
               if (views->src_s) {
      assert(views->dst_s);
   rsd_key.s.format = views->dst_s->format;
   rsd_key.s.type = blit_key.surfaces[1].type;
   rsd_key.s.src_samples = blit_key.surfaces[1].src_samples;
   rsd_key.s.dst_samples = blit_key.surfaces[1].dst_samples;
   rsd_key.s.dim = blit_key.surfaces[1].dim;
               for (unsigned i = 0; i < views->rt_count; i++) {
      if (!views->src_rts[i])
            assert(views->dst_rts[i]);
   rsd_key.rts[i].format = views->dst_rts[i]->format;
   rsd_key.rts[i].type = blit_key.surfaces[i].type;
   rsd_key.rts[i].src_samples = blit_key.surfaces[i].src_samples;
   rsd_key.rts[i].dst_samples = blit_key.surfaces[i].dst_samples;
   rsd_key.rts[i].dim = blit_key.surfaces[i].dim;
               pthread_mutex_lock(&dev->blitter.rsds.lock);
   struct hash_entry *he =
         struct pan_blit_rsd_data *rsd = he ? he->data : NULL;
   if (rsd)
            rsd = rzalloc(dev->blitter.rsds.rsds, struct pan_blit_rsd_data);
            unsigned bd_count = PAN_ARCH >= 5 ? MAX2(views->rt_count, 1) : 0;
   struct panfrost_ptr rsd_ptr = pan_pool_alloc_desc_aggregate(
      dev->blitter.rsds.pool, PAN_DESC(RENDERER_STATE),
                  const struct pan_blit_shader_data *blit_shader =
         #if PAN_ARCH <= 5
      pan_blitter_get_blend_shaders(dev, views->rt_count, views->dst_rts,
      #endif
         pan_blitter_emit_rsd(dev, blit_shader, views, blend_shaders, rsd_ptr.cpu);
   rsd->address = rsd_ptr.gpu;
         out:
      pthread_mutex_unlock(&dev->blitter.rsds.lock);
      }
      static mali_ptr
   pan_blit_get_rsd(struct panfrost_device *dev,
               {
      const struct util_format_description *desc =
                     if (util_format_has_depth(desc)) {
      views.src_z = &src_views[0];
               if (src_views[1].format) {
      views.src_s = &src_views[1];
      } else if (util_format_has_stencil(desc)) {
      views.src_s = &src_views[0];
               if (!views.src_z && !views.src_s) {
      views.rt_count = 1;
   views.src_rts[0] = src_views;
                  }
   #endif
      static struct pan_blitter_views
   pan_preload_get_views(const struct pan_fb_info *fb, bool zs,
         {
               if (zs) {
      if (fb->zs.preload.z)
            if (fb->zs.preload.s) {
                     switch (view->format) {
   case PIPE_FORMAT_Z24_UNORM_S8_UINT:
      fmt = PIPE_FORMAT_X24S8_UINT;
      case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
      fmt = PIPE_FORMAT_X32_S8X24_UINT;
      default:
      fmt = view->format;
               if (fmt != view->format) {
      *patched_s = *view;
   patched_s->format = fmt;
      } else {
               } else {
      for (unsigned i = 0; i < fb->rt_count; i++) {
      if (fb->rts[i].preload) {
      views.src_rts[i] = fb->rts[i].view;
                                 }
      static bool
   pan_preload_needed(const struct pan_fb_info *fb, bool zs)
   {
      if (zs) {
      if (fb->zs.preload.z || fb->zs.preload.s)
      } else {
      for (unsigned i = 0; i < fb->rt_count; i++) {
      if (fb->rts[i].preload)
                     }
      static mali_ptr
   pan_blitter_emit_varying(struct pan_pool *pool)
   {
               pan_pack(varying.cpu, ATTRIBUTE, cfg) {
      cfg.buffer_index = 0;
   cfg.offset_enable = PAN_ARCH <= 5;
      #if PAN_ARCH >= 9
         cfg.attribute_type = MALI_ATTRIBUTE_TYPE_1D;
   cfg.table = PAN_TABLE_ATTRIBUTE_BUFFER;
   cfg.frequency = MALI_ATTRIBUTE_FREQUENCY_VERTEX;
   #endif
                  }
      static mali_ptr
   pan_blitter_emit_varying_buffer(struct pan_pool *pool, mali_ptr coordinates)
   {
   #if PAN_ARCH >= 9
               pan_pack(varying_buffer.cpu, BUFFER, cfg) {
      cfg.address = coordinates;
         #else
      /* Bifrost needs an empty desc to mark end of prefetching */
            struct panfrost_ptr varying_buffer = pan_pool_alloc_desc_array(
            pan_pack(varying_buffer.cpu, ATTRIBUTE_BUFFER, cfg) {
      cfg.pointer = coordinates;
   cfg.stride = 4 * sizeof(float);
               if (padding_buffer) {
      pan_pack(varying_buffer.cpu + pan_size(ATTRIBUTE_BUFFER),
               #endif
            }
      static mali_ptr
   pan_blitter_emit_sampler(struct pan_pool *pool, bool nearest_filter)
   {
               pan_pack(sampler.cpu, SAMPLER, cfg) {
      cfg.seamless_cube_map = false;
   cfg.normalized_coordinates = false;
   cfg.minify_nearest = nearest_filter;
                  }
      static mali_ptr
   pan_blitter_emit_textures(struct pan_pool *pool, unsigned tex_count,
         {
   #if PAN_ARCH >= 6
      struct panfrost_ptr textures =
            for (unsigned i = 0; i < tex_count; i++) {
      void *texture = textures.cpu + (pan_size(TEXTURE) * i);
   size_t payload_size =
         struct panfrost_ptr surfaces =
                           #else
               for (unsigned i = 0; i < tex_count; i++) {
      size_t sz = pan_size(TEXTURE) +
         struct panfrost_ptr texture =
         struct panfrost_ptr surfaces = {
      .cpu = texture.cpu + pan_size(TEXTURE),
               GENX(panfrost_new_texture)(pool->dev, views[i], texture.cpu, &surfaces);
               return pan_pool_upload_aligned(pool, textures, tex_count * sizeof(mali_ptr),
      #endif
   }
      static mali_ptr
   pan_preload_emit_textures(struct pan_pool *pool, const struct pan_fb_info *fb,
         {
      const struct pan_image_view *views[8];
   struct pan_image_view patched_s_view;
            if (zs) {
      if (fb->zs.preload.z)
            if (fb->zs.preload.s) {
                     switch (view->format) {
   case PIPE_FORMAT_Z24_UNORM_S8_UINT:
      fmt = PIPE_FORMAT_X24S8_UINT;
      case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
      fmt = PIPE_FORMAT_X32_S8X24_UINT;
      default:
      fmt = view->format;
               if (fmt != view->format) {
      patched_s_view = *view;
   patched_s_view.format = fmt;
      }
         } else {
      for (unsigned i = 0; i < fb->rt_count; i++) {
      if (fb->rts[i].preload)
                              }
      #if PAN_ARCH >= 8
   /* TODO: cache */
   static mali_ptr
   pan_blitter_emit_zs(struct pan_pool *pool, bool z, bool s)
   {
               pan_pack(zsd.cpu, DEPTH_STENCIL, cfg) {
      cfg.depth_function = MALI_FUNC_ALWAYS;
            if (z)
            cfg.stencil_test_enable = s;
            cfg.front_compare_function = MALI_FUNC_ALWAYS;
   cfg.front_stencil_fail = MALI_STENCIL_OP_REPLACE;
   cfg.front_depth_fail = MALI_STENCIL_OP_REPLACE;
   cfg.front_depth_pass = MALI_STENCIL_OP_REPLACE;
   cfg.front_write_mask = 0xFF;
            cfg.back_compare_function = MALI_FUNC_ALWAYS;
   cfg.back_stencil_fail = MALI_STENCIL_OP_REPLACE;
   cfg.back_depth_fail = MALI_STENCIL_OP_REPLACE;
   cfg.back_depth_pass = MALI_STENCIL_OP_REPLACE;
   cfg.back_write_mask = 0xFF;
                           }
   #else
   static mali_ptr
   pan_blitter_emit_viewport(struct pan_pool *pool, uint16_t minx, uint16_t miny,
         {
               pan_pack(vp.cpu, VIEWPORT, cfg) {
      cfg.scissor_minimum_x = minx;
   cfg.scissor_minimum_y = miny;
   cfg.scissor_maximum_x = maxx;
                  }
   #endif
      static void
   pan_preload_emit_dcd(struct pan_pool *pool, struct pan_fb_info *fb, bool zs,
               {
      unsigned tex_count = 0;
   mali_ptr textures = pan_preload_emit_textures(pool, fb, zs, &tex_count);
   mali_ptr samplers = pan_blitter_emit_sampler(pool, true);
   mali_ptr varyings = pan_blitter_emit_varying(pool);
   mali_ptr varying_buffers =
            /* Tiles updated by blit shaders are still considered clean (separate
   * for colour and Z/S), allowing us to suppress unnecessary writeback
   */
            /* Image view used when patching stencil formats for combined
   * depth/stencil preloads.
   */
                  #if PAN_ARCH <= 7
      pan_pack(out, DRAW, cfg) {
               if (PAN_ARCH == 4) {
      maxx = fb->width - 1;
      } else {
      /* Align on 32x32 tiles */
   minx = fb->extent.minx & ~31;
   miny = fb->extent.miny & ~31;
   maxx = MIN2(ALIGN_POT(fb->extent.maxx + 1, 32), fb->width) - 1;
               cfg.thread_storage = tsd;
            cfg.position = coordinates;
            cfg.varyings = varyings;
   cfg.varying_buffers = varying_buffers;
   cfg.textures = textures;
      #if PAN_ARCH >= 6
         #endif
         #else
      struct panfrost_ptr T;
            /* Although individual resources need only 16 byte alignment, the
   * resource table as a whole must be 64-byte aligned.
   */
   T = pan_pool_alloc_aligned(pool, nr_tables * pan_size(RESOURCE), 64);
            panfrost_make_resource_table(T, PAN_TABLE_TEXTURE, textures, tex_count);
   panfrost_make_resource_table(T, PAN_TABLE_SAMPLER, samplers, 1);
   panfrost_make_resource_table(T, PAN_TABLE_ATTRIBUTE, varyings, 1);
   panfrost_make_resource_table(T, PAN_TABLE_ATTRIBUTE_BUFFER, varying_buffers,
            struct pan_blit_shader_key key = pan_blitter_get_key(&views);
   const struct pan_blit_shader_data *blit_shader =
            bool z = fb->zs.preload.z;
   bool s = fb->zs.preload.s;
            struct panfrost_ptr spd = pan_pool_alloc_desc(pool, SHADER_PROGRAM);
   pan_pack(spd.cpu, SHADER_PROGRAM, cfg) {
      cfg.stage = MALI_SHADER_STAGE_FRAGMENT;
   cfg.primary_shader = true;
   cfg.register_allocation = MALI_SHADER_REGISTER_ALLOCATION_32_PER_THREAD;
   cfg.binary = blit_shader->address;
               unsigned bd_count = views.rt_count;
            if (!zs) {
                  pan_pack(out, DRAW, cfg) {
      if (zs) {
      /* ZS_EMIT requires late update/kill */
   cfg.zs_update_operation = MALI_PIXEL_KILL_FORCE_LATE;
   cfg.pixel_kill_operation = MALI_PIXEL_KILL_FORCE_LATE;
      } else {
      /* Skipping ATEST requires forcing Z/S */
                  cfg.blend = blend.gpu;
   cfg.blend_count = bd_count;
               cfg.allow_forward_pixel_to_kill = !zs;
   cfg.allow_forward_pixel_to_be_killed = true;
   cfg.depth_stencil = pan_blitter_emit_zs(pool, z, s);
   cfg.sample_mask = 0xFFFF;
   cfg.multisample_enable = ms;
   cfg.evaluate_per_sample = ms;
   cfg.maximum_z = 1.0;
   cfg.clean_fragment_write = clean_fragment_write;
   cfg.shader.resources = T.gpu | nr_tables;
   cfg.shader.shader = spd.gpu;
         #endif
   }
      #if PAN_ARCH <= 7
   static void *
   pan_blit_emit_tiler_job(struct pan_pool *pool,
               {
               pan_section_pack(job->cpu, TILER_JOB, PRIMITIVE, cfg) {
      cfg.draw_mode = MALI_DRAW_MODE_TRIANGLE_STRIP;
   cfg.index_count = 4;
               pan_section_pack(job->cpu, TILER_JOB, PRIMITIVE_SIZE, cfg) {
                  void *invoc = pan_section_ptr(job->cpu, TILER_JOB, INVOCATION);
         #if PAN_ARCH >= 6
      pan_section_pack(job->cpu, TILER_JOB, PADDING, cfg)
         pan_section_pack(job->cpu, TILER_JOB, TILER, cfg) {
            #endif
         panfrost_add_job(pool, scoreboard, MALI_JOB_TYPE_TILER, false, false, 0, 0,
            }
   #endif
      #if PAN_ARCH >= 6
   static void
   pan_preload_fb_alloc_pre_post_dcds(struct pan_pool *desc_pool,
         {
      if (fb->bifrost.pre_post.dcds.gpu)
               }
      static void
   pan_preload_emit_pre_frame_dcd(struct pan_pool *desc_pool,
               {
      unsigned dcd_idx = zs ? 1 : 0;
   pan_preload_fb_alloc_pre_post_dcds(desc_pool, fb);
   assert(fb->bifrost.pre_post.dcds.cpu);
            /* We only use crc_rt to determine whether to force writes for updating
   * the CRCs, so use a conservative tile size (16x16).
   */
                     /* If CRC data is currently invalid and this batch will make it valid,
   * write even clean tiles to make sure CRC data is updated. */
   if (crc_rt >= 0) {
      bool *valid = fb->rts[crc_rt].crc_valid;
   bool full = !fb->extent.minx && !fb->extent.miny &&
                  if (full && !(*valid))
               pan_preload_emit_dcd(desc_pool, fb, zs, coords, tsd, dcd, always_write);
   if (zs) {
      enum pipe_format fmt = fb->zs.view.zs
                        /* If we're dealing with a combined ZS resource and only one
   * component is cleared, we need to reload the whole surface
   * because the zs_clean_pixel_write_enable flag is set in that
   * case.
   */
   if (util_format_is_depth_and_stencil(fmt) &&
                  /* We could use INTERSECT on Bifrost v7 too, but
   * EARLY_ZS_ALWAYS has the advantage of reloading the ZS tile
   * buffer one or more tiles ahead, making ZS data immediately
   * available for any ZS tests taking place in other shaders.
   * Thing's haven't been benchmarked to determine what's
   * preferable (saving bandwidth vs having ZS preloaded
   * earlier), so let's leave it like that for now.
   */
   fb->bifrost.pre_post.modes[dcd_idx] =
      desc_pool->dev->arch > 6
         : always ? MALI_PRE_POST_FRAME_SHADER_MODE_ALWAYS
   } else {
      fb->bifrost.pre_post.modes[dcd_idx] =
      always_write ? MALI_PRE_POST_FRAME_SHADER_MODE_ALWAYS
      }
   #else
   static struct panfrost_ptr
   pan_preload_emit_tiler_job(struct pan_pool *desc_pool,
                     {
               pan_preload_emit_dcd(desc_pool, fb, zs, coords, tsd,
            pan_section_pack(job.cpu, TILER_JOB, PRIMITIVE, cfg) {
      cfg.draw_mode = MALI_DRAW_MODE_TRIANGLE_STRIP;
   cfg.index_count = 4;
               pan_section_pack(job.cpu, TILER_JOB, PRIMITIVE_SIZE, cfg) {
                  void *invoc = pan_section_ptr(job.cpu, TILER_JOB, INVOCATION);
            panfrost_add_job(desc_pool, scoreboard, MALI_JOB_TYPE_TILER, false, false, 0,
            }
   #endif
      static struct panfrost_ptr
   pan_preload_fb_part(struct pan_pool *pool, struct pan_scoreboard *scoreboard,
               {
            #if PAN_ARCH >= 6
         #else
         #endif
         }
      unsigned
   GENX(pan_preload_fb)(struct pan_pool *pool, struct pan_scoreboard *scoreboard,
               {
      bool preload_zs = pan_preload_needed(fb, true);
   bool preload_rts = pan_preload_needed(fb, false);
            if (!preload_zs && !preload_rts)
            float rect[] = {
      0.0, 0.0,        0.0, 1.0, fb->width, 0.0,        0.0, 1.0,
                        unsigned njobs = 0;
   if (preload_zs) {
      struct panfrost_ptr job =
         if (jobs && job.cpu)
               if (preload_rts) {
      struct panfrost_ptr job =
         if (jobs && job.cpu)
                  }
      #if PAN_ARCH <= 7
   void
   GENX(pan_blit_ctx_init)(struct panfrost_device *dev,
                     {
               struct pan_image_view sviews[2] = {
      {
      .format = info->src.planes[0].format,
   .planes =
      {
      info->src.planes[0].image,
   info->src.planes[1].image,
         .dim =
      info->src.planes[0].image->layout.dim == MALI_TEXTURE_DIMENSION_CUBE
      ? MALI_TEXTURE_DIMENSION_2D
   .first_level = info->src.level,
   .last_level = info->src.level,
   .first_layer = info->src.start.layer,
   .last_layer = info->src.end.layer,
   .swizzle =
      {
      PIPE_SWIZZLE_X,
   PIPE_SWIZZLE_Y,
   PIPE_SWIZZLE_Z,
                  struct pan_image_view dview = {
      .format = info->dst.planes[0].format,
   .planes =
      {
      info->dst.planes[0].image,
   info->dst.planes[1].image,
         .dim = info->dst.planes[0].image->layout.dim == MALI_TEXTURE_DIMENSION_1D
               .first_level = info->dst.level,
   .last_level = info->dst.level,
   .first_layer = info->dst.start.layer,
   .last_layer = info->dst.start.layer,
   .swizzle =
      {
      PIPE_SWIZZLE_X,
   PIPE_SWIZZLE_Y,
   PIPE_SWIZZLE_Z,
               ctx->src.start.x = info->src.start.x;
   ctx->src.start.y = info->src.start.y;
   ctx->src.end.x = info->src.end.x;
   ctx->src.end.y = info->src.end.y;
            if (info->dst.planes[0].image->layout.dim == MALI_TEXTURE_DIMENSION_3D) {
      unsigned max_z =
            ctx->z_scale = (float)(info->src.end.z - info->src.start.z) /
         assert(info->dst.start.z != info->dst.end.z);
   if (info->dst.start.z > info->dst.end.z) {
      ctx->dst.cur_layer = info->dst.start.z - 1;
      } else {
      ctx->dst.cur_layer = info->dst.start.z;
      }
   ctx->dst.cur_layer = MIN2(MAX2(ctx->dst.cur_layer, 0), max_z);
   ctx->dst.last_layer = MIN2(MAX2(ctx->dst.last_layer, 0), max_z);
      } else {
      unsigned max_layer = info->dst.planes[0].image->layout.array_size - 1;
   ctx->dst.layer_offset = info->dst.start.layer;
   ctx->dst.cur_layer = info->dst.start.layer;
   ctx->dst.last_layer = MIN2(info->dst.end.layer, max_layer);
               if (sviews[0].dim == MALI_TEXTURE_DIMENSION_3D) {
      if (info->src.start.z < info->src.end.z)
         else
      } else {
                  /* Split depth and stencil */
   if (util_format_is_depth_and_stencil(sviews[0].format)) {
      sviews[1] = sviews[0];
   sviews[0].format = util_format_get_depth_only(sviews[0].format);
      } else if (info->src.planes[1].format) {
      sviews[1] = sviews[0];
   sviews[1].format = info->src.planes[1].format;
                                          unsigned dst_w =
         unsigned dst_h =
         unsigned maxx = MIN2(MAX2(info->dst.start.x, info->dst.end.x), dst_w - 1);
   unsigned maxy = MIN2(MAX2(info->dst.start.y, info->dst.end.y), dst_h - 1);
   unsigned minx = MAX2(MIN3(info->dst.start.x, info->dst.end.x, maxx), 0);
            if (info->scissor.enable) {
      minx = MAX2(minx, info->scissor.minx);
   miny = MAX2(miny, info->scissor.miny);
   maxx = MIN2(maxx, info->scissor.maxx);
               const struct pan_image_view *sview_ptrs[] = {&sviews[0], &sviews[1]};
            ctx->textures = pan_blitter_emit_textures(blit_pool, nviews, sview_ptrs);
                     float dst_rect[] = {
      info->dst.start.x, info->dst.start.y, 0.0, 1.0,
   info->dst.end.x,   info->dst.start.y, 0.0, 1.0,
   info->dst.start.x, info->dst.end.y,   0.0, 1.0,
               ctx->position =
      }
      struct panfrost_ptr
   GENX(pan_blit)(struct pan_blit_context *ctx, struct pan_pool *pool,
         {
      if (ctx->dst.cur_layer < 0 ||
      (ctx->dst.last_layer >= ctx->dst.layer_offset &&
   ctx->dst.cur_layer > ctx->dst.last_layer) ||
   (ctx->dst.last_layer < ctx->dst.layer_offset &&
   ctx->dst.cur_layer < ctx->dst.last_layer))
         int32_t layer = ctx->dst.cur_layer - ctx->dst.layer_offset;
   float src_z;
   if (ctx->src.dim == MALI_TEXTURE_DIMENSION_3D)
         else
            float src_rect[] = {
      ctx->src.start.x, ctx->src.start.y, src_z, 1.0,
   ctx->src.end.x,   ctx->src.start.y, src_z, 1.0,
   ctx->src.start.x, ctx->src.end.y,   src_z, 1.0,
               mali_ptr src_coords =
            struct panfrost_ptr job = {0};
            pan_pack(dcd, DRAW, cfg) {
      cfg.thread_storage = tsd;
            cfg.position = ctx->position;
   cfg.varyings = pan_blitter_emit_varying(pool);
   cfg.varying_buffers = pan_blitter_emit_varying_buffer(pool, src_coords);
   cfg.viewport = ctx->vpd;
   cfg.textures = ctx->textures;
                  }
   #endif
      static uint32_t
   pan_blit_shader_key_hash(const void *key)
   {
         }
      static bool
   pan_blit_shader_key_equal(const void *a, const void *b)
   {
         }
      static uint32_t
   pan_blit_blend_shader_key_hash(const void *key)
   {
         }
      static bool
   pan_blit_blend_shader_key_equal(const void *a, const void *b)
   {
         }
      static uint32_t
   pan_blit_rsd_key_hash(const void *key)
   {
         }
      static bool
   pan_blit_rsd_key_equal(const void *a, const void *b)
   {
         }
      static void
   pan_blitter_prefill_blit_shader_cache(struct panfrost_device *dev)
   {
      static const struct pan_blit_shader_key prefill[] = {
      {
      .surfaces[0] =
      {
      .loc = FRAG_RESULT_DEPTH,
   .type = nir_type_float32,
   .dim = MALI_TEXTURE_DIMENSION_2D,
   .src_samples = 1,
      },
   {
      .surfaces[1] =
      {
      .loc = FRAG_RESULT_STENCIL,
   .type = nir_type_uint32,
   .dim = MALI_TEXTURE_DIMENSION_2D,
   .src_samples = 1,
      },
   {
      .surfaces[0] =
      {
      .loc = FRAG_RESULT_DATA0,
   .type = nir_type_float32,
   .dim = MALI_TEXTURE_DIMENSION_2D,
   .src_samples = 1,
                  for (unsigned i = 0; i < ARRAY_SIZE(prefill); i++)
      }
      void
   GENX(pan_blitter_init)(struct panfrost_device *dev, struct pan_pool *bin_pool,
         {
      dev->blitter.shaders.blit = _mesa_hash_table_create(
         dev->blitter.shaders.blend = _mesa_hash_table_create(
         dev->blitter.shaders.pool = bin_pool;
   pthread_mutex_init(&dev->blitter.shaders.lock, NULL);
            dev->blitter.rsds.pool = desc_pool;
   dev->blitter.rsds.rsds = _mesa_hash_table_create(NULL, pan_blit_rsd_key_hash,
            }
      void
   GENX(pan_blitter_cleanup)(struct panfrost_device *dev)
   {
      _mesa_hash_table_destroy(dev->blitter.shaders.blit, NULL);
   _mesa_hash_table_destroy(dev->blitter.shaders.blend, NULL);
   pthread_mutex_destroy(&dev->blitter.shaders.lock);
   _mesa_hash_table_destroy(dev->blitter.rsds.rsds, NULL);
      }
