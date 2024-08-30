   /**************************************************************************
   *
   * Copyright 2010-2021 VMware, Inc.
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
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   **************************************************************************/
         #include "util/detect.h"
      #include "util/u_math.h"
   #include "util/u_cpu_detect.h"
   #include "util/u_pack_color.h"
   #include "util/u_rect.h"
   #include "util/u_sse.h"
      #include "lp_jit.h"
   #include "lp_rast.h"
   #include "lp_debug.h"
   #include "lp_state_fs.h"
   #include "lp_linear_priv.h"
         #if DETECT_ARCH_SSE
         /* For debugging (LP_DEBUG=linear), shade areas of run-time fallback
   * purple.  Keep blending active so we can see more of what's going
   * on.
   */
   static bool
   linear_fallback(const struct lp_rast_state *state,
                  unsigned x, unsigned y,
      {
      unsigned col = 0x808000ff;
            for (y = 0; y < height; y++) {
      for (i = 0; i < 64; i++) {
                        }
         /*
   * Run our configurable linear shader pipeline:
   * x,y is the surface position of the linear region, width, height is the size.
   * Return TRUE for success, FALSE otherwise.
   */
   static bool
   lp_fs_linear_run(const struct lp_rast_state *state,
                  unsigned x, unsigned y,
   unsigned width, unsigned height,
   const float (*a0)[4],
   const float (*dadx)[4],
      {
      const struct lp_fragment_shader_variant *variant = state->variant;
   const struct lp_tgsi_info *info = &variant->shader->info;
   const struct lp_fragment_shader_variant_key *key = &variant->key;
   bool rgba_order = (key->cbuf_format[0] == PIPE_FORMAT_R8G8B8A8_UNORM ||
                           /* Require constant w in these rectangles:
   */
   if (dadx[0][3] != 0.0f ||
      dady[0][3] != 0.0f) {
   if (LP_DEBUG & DEBUG_LINEAR2)
                     /* XXX: Per statechange:
   */
            for (int i = 0; i < nr_consts; i++){
      float val = state->jit_resources.constants[0].f[i];
   if (val < 0.0f || val > 1.0f) {
      if (LP_DEBUG & DEBUG_LINEAR2)
            }
               struct lp_jit_linear_context jit;
            if (!rgba_order) {
      jit.blend_color =
      state->jit_context.u8_blend_color[32] +
   (state->jit_context.u8_blend_color[16] << 8) +
   (state->jit_context.u8_blend_color[0] << 16) +
   } else {
      jit.blend_color =
      (state->jit_context.u8_blend_color[32] << 24) +
   (state->jit_context.u8_blend_color[16] << 16) +
   (state->jit_context.u8_blend_color[0] << 8) +
                     /* XXX: Per primitive:
   */
   struct lp_linear_interp interp[LP_MAX_LINEAR_INPUTS];
   const float oow = 1.0f / a0[0][3];
   unsigned input_mask = variant->linear_input_mask;
   while (input_mask) {
      int i = u_bit_scan(&input_mask);
   unsigned usage_mask = info->base.input_usage_mask[i];
   bool perspective =
         info->base.input_interpolate[i] == TGSI_INTERPOLATE_PERSPECTIVE ||
   (info->base.input_interpolate[i] == TGSI_INTERPOLATE_COLOR &&
   if (!lp_linear_init_interp(&interp[i],
                              x, y, width, height,
   usage_mask,
   perspective,
   if (LP_DEBUG & DEBUG_LINEAR2)
                                 /* XXX: Per primitive: Initialize linear or nearest samplers:
   */
   struct lp_linear_sampler samp[LP_MAX_LINEAR_TEXTURES];
   const int nr_tex = info->num_texs;
   for (int i = 0; i < nr_tex; i++) {
      const struct lp_tgsi_texture_info *tex_info = &info->tex[i];
   const unsigned tex_unit = tex_info->texture_unit;
   const unsigned samp_unit = tex_info->sampler_unit;
   //const unsigned fs_s_input = tex_info->coord[0].u.index;
            // xxx investigate why these fail in deqp-vk
   //assert(variant->linear_input_mask & (1 << fs_s_input));
            /* XXX: some texture coordinates are linear!
   */
   //boolean perspective = (info->base.input_interpolate[i] ==
            if (!lp_linear_init_sampler(&samp[i], tex_info,
               lp_fs_variant_key_sampler_idx(&variant->key, samp_unit),
      if (LP_DEBUG & DEBUG_LINEAR2)
                                 /* JIT function already does blending */
   jit.color0 = color + x * 4 + y * stride;
            for (unsigned iy = 0; iy < height; iy++) {
      jit_func(&jit, 0, 0, width);  // x=0, y=0
                     fail:
      /* Visually distinguish this from other fallbacks:
   */
   if (LP_DEBUG & DEBUG_LINEAR) {
                     }
         static void
   check_linear_interp_mask_a(struct lp_fragment_shader_variant *variant)
   {
      const struct lp_tgsi_info *info = &variant->shader->info;
            struct lp_linear_sampler samp[LP_MAX_LINEAR_TEXTURES];
   struct lp_linear_interp interp[LP_MAX_LINEAR_INPUTS];
   uint8_t constants[LP_MAX_LINEAR_CONSTANTS][4];
            const int nr_inputs = info->base.file_max[TGSI_FILE_INPUT]+1;
                              for (int i = 0; i < nr_tex; i++) {
      lp_linear_init_noop_sampler(&samp[i]);
               for (int i = 0; i < nr_inputs; i++) {
      lp_linear_init_noop_interp(&interp[i]);
                                 /* Find out which interpolators were called, and store this as a
   * mask:
   */
   for (int i = 0; i < nr_inputs; i++) {
            }
         /* Until the above is working, look at texture information and guess
   * that any input used as a texture coordinate is not used for
   * anything else.
   */
   static void
   check_linear_interp_mask_b(struct lp_fragment_shader_variant *variant)
   {
      const struct lp_tgsi_info *info = &variant->shader->info;
   int nr_inputs = info->base.file_max[TGSI_FILE_INPUT]+1;
   int nr_tex = info->num_texs;
   unsigned tex_mask = 0;
                     for (i = 0; i < nr_tex; i++) {
      const struct lp_tgsi_texture_info *tex_info = &info->tex[i];
   const struct lp_tgsi_channel_info *schan = &tex_info->coord[0];
   const struct lp_tgsi_channel_info *tchan = &tex_info->coord[1];
   tex_mask |= 1 << schan->u.index;
                  }
         void
   lp_linear_check_variant(struct lp_fragment_shader_variant *variant)
   {
      const struct lp_fragment_shader_variant_key *key = &variant->key;
   const struct lp_fragment_shader *shader = variant->shader;
            if (info->base.file_max[TGSI_FILE_CONSTANT] >= LP_MAX_LINEAR_CONSTANTS ||
      info->base.file_max[TGSI_FILE_INPUT] >= LP_MAX_LINEAR_INPUTS) {
   if (LP_DEBUG & DEBUG_LINEAR)
                     /* If we have a fastpath which implements the entire variant, use
   * that.
   */
   if (lp_linear_check_fastpath(variant)) {
                  /* Otherwise, can we build up a spanline-based linear path for this
   * variant?
            /* Check static sampler state.
   */
   for (unsigned i = 0; i < info->num_texs; i++) {
      const struct lp_tgsi_texture_info *tex_info = &info->tex[i];
            /* XXX: Relax this once setup premultiplies by oow:
   */
   if (info->base.input_interpolate[unit] != TGSI_INTERPOLATE_PERSPECTIVE) {
      if (LP_DEBUG & DEBUG_LINEAR)
                     struct lp_sampler_static_state *samp =
         if (!lp_linear_check_sampler(samp, tex_info)) {
      if (LP_DEBUG & DEBUG_LINEAR)
                        /* Check shader.  May not have been jitted.
   */
   if (variant->linear_function == NULL) {
      if (LP_DEBUG & DEBUG_LINEAR)
                     /* Hook in the catchall shader runner:
   */
            /* Figure out which inputs we don't need to interpolate (because
   * they are only used as texture coordinates).  This is important
   * as we can cope with texture coordinates which exceed 1.0, but
   * cannot do so for regular inputs.
   */
   if (1)
         else
               if (0) {
      lp_debug_fs_variant(variant);
                     fail:
      if (LP_DEBUG & DEBUG_LINEAR) {
      lp_debug_fs_variant(variant);
         }
         #else
   void
   lp_linear_check_variant(struct lp_fragment_shader_variant *variant)
   {
   }
   #endif
