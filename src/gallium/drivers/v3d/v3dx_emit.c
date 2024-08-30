   /*
   * Copyright © 2014-2017 Broadcom
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
      #include "util/format/u_format.h"
   #include "util/half_float.h"
   #include "v3d_context.h"
   #include "broadcom/common/v3d_macros.h"
   #include "broadcom/cle/v3dx_pack.h"
   #include "broadcom/common/v3d_util.h"
   #include "broadcom/compiler/v3d_compiler.h"
      static uint8_t
   v3d_factor(enum pipe_blendfactor factor, bool dst_alpha_one)
   {
         /* We may get a bad blendfactor when blending is disabled. */
   if (factor == 0)
            switch (factor) {
   case PIPE_BLENDFACTOR_ZERO:
         case PIPE_BLENDFACTOR_ONE:
         case PIPE_BLENDFACTOR_SRC_COLOR:
         case PIPE_BLENDFACTOR_INV_SRC_COLOR:
         case PIPE_BLENDFACTOR_DST_COLOR:
         case PIPE_BLENDFACTOR_INV_DST_COLOR:
         case PIPE_BLENDFACTOR_SRC_ALPHA:
         case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
         case PIPE_BLENDFACTOR_DST_ALPHA:
            return (dst_alpha_one ?
      case PIPE_BLENDFACTOR_INV_DST_ALPHA:
            return (dst_alpha_one ?
      case PIPE_BLENDFACTOR_CONST_COLOR:
         case PIPE_BLENDFACTOR_INV_CONST_COLOR:
         case PIPE_BLENDFACTOR_CONST_ALPHA:
         case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
         case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
            return (dst_alpha_one ?
      default:
         }
      #if V3D_VERSION < 40
   static inline uint16_t
   swizzled_border_color(const struct v3d_device_info *devinfo,
                     {
         const struct util_format_description *desc =
                  /* If we're doing swizzling in the sampler, then only rearrange the
      * border color for the mismatch between the V3D texture format and
   * the PIPE_FORMAT, since GL_ARB_texture_swizzle will be handled by
   * the sampler's swizzle.
   *
   * For swizzling in the shader, we don't do any pre-swizzling of the
   * border color.
      if (v3d_get_tex_return_size(devinfo, sview->base.format) != 32)
            switch (swiz) {
   case PIPE_SWIZZLE_0:
         case PIPE_SWIZZLE_1:
         default:
         }
      static void
   emit_one_texture(struct v3d_context *v3d, struct v3d_texture_stateobj *stage_tex,
         {
         struct v3d_job *job = v3d->job;
   struct pipe_sampler_state *psampler = stage_tex->samplers[i];
   struct v3d_sampler_state *sampler = v3d_sampler_state(psampler);
   struct pipe_sampler_view *psview = stage_tex->textures[i];
   struct v3d_sampler_view *sview = v3d_sampler_view(psview);
   struct pipe_resource *prsc = psview->texture;
   struct v3d_resource *rsc = v3d_resource(prsc);
            stage_tex->texture_state[i].offset =
            v3d_cl_ensure_space(&job->indirect,
      v3d_bo_set_reference(&stage_tex->texture_state[i].bo,
                     struct V3D33_TEXTURE_SHADER_STATE unpacked = {
            /* XXX */
   .border_color_red = swizzled_border_color(devinfo, psampler,
         .border_color_green = swizzled_border_color(devinfo, psampler,
         .border_color_blue = swizzled_border_color(devinfo, psampler,
                        /* In the normal texturing path, the LOD gets clamped between
   * min/max, and the base_level field (set in the sampler view
   * from first_level) only decides where the min/mag switch
   * happens, so we need to use the LOD clamps to keep us
   * between min and max.
   *
   * For txf, the LOD clamp is still used, despite GL not
   * wanting that.  We will need to have a separate
   * TEXTURE_SHADER_STATE that ignores psview->min/max_lod to
   * support txf properly.
   */
   .min_level_of_detail = MIN2(psview->u.tex.first_level +
               .max_level_of_detail = MIN2(psview->u.tex.first_level +
                                             /* Set up the sampler swizzle if we're doing 16-bit sampling.  For
      * 32-bit, we leave swizzling up to the shader compiler.
   *
   * Note: Contrary to the docs, the swizzle still applies even if the
   * return size is 32.  It's just that you probably want to swizzle in
   * the shader, because you need the Y/Z/W channels to be defined.
      if (return_size == 32) {
            unpacked.swizzle_r = v3d_translate_pipe_swizzle(PIPE_SWIZZLE_X);
   unpacked.swizzle_g = v3d_translate_pipe_swizzle(PIPE_SWIZZLE_Y);
      } else {
            unpacked.swizzle_r = v3d_translate_pipe_swizzle(sview->swizzle[0]);
   unpacked.swizzle_g = v3d_translate_pipe_swizzle(sview->swizzle[1]);
               int min_img_filter = psampler->min_img_filter;
   int min_mip_filter = psampler->min_mip_filter;
            if (return_size == 32) {
            min_mip_filter = PIPE_TEX_MIPFILTER_NEAREST;
               bool min_nearest = min_img_filter == PIPE_TEX_FILTER_NEAREST;
   switch (min_mip_filter) {
   case PIPE_TEX_MIPFILTER_NONE:
               case PIPE_TEX_MIPFILTER_NEAREST:
               case PIPE_TEX_MIPFILTER_LINEAR:
            unpacked.filter += min_nearest ? 4 : 8;
               if (mag_img_filter == PIPE_TEX_FILTER_NEAREST)
            if (psampler->max_anisotropy > 8)
         else if (psampler->max_anisotropy > 4)
         else if (psampler->max_anisotropy > 2)
         else if (psampler->max_anisotropy)
            uint8_t packed[cl_packet_length(TEXTURE_SHADER_STATE)];
            for (int i = 0; i < ARRAY_SIZE(packed); i++)
            /* TMU indirect structs need to be 32b aligned. */
   v3d_cl_ensure_space(&job->indirect, ARRAY_SIZE(packed), 32);
   }
      static void
   emit_textures(struct v3d_context *v3d, struct v3d_texture_stateobj *stage_tex)
   {
         for (int i = 0; i < stage_tex->num_textures; i++) {
               }
   #endif /* V3D_VERSION < 40 */
      static uint32_t
   translate_colormask(struct v3d_context *v3d, uint32_t colormask, int rt)
   {
         if (v3d->swap_color_rb & (1 << rt)) {
            colormask = ((colormask & (2 | 8)) |
               }
      static void
   emit_rt_blend(struct v3d_context *v3d, struct v3d_job *job,
               {
            #if V3D_VERSION >= 40
         /* We don't need to emit blend state for disabled RTs. */
   if (!rtblend->blend_enable)
   #endif
            #if V3D_VERSION >= 40
         #else
         #endif
                     config.color_blend_mode = rtblend->rgb_func;
   config.color_blend_dst_factor =
                                    config.alpha_blend_mode = rtblend->alpha_func;
   config.alpha_blend_dst_factor =
               config.alpha_blend_src_factor =
      }
      static void
   emit_flat_shade_flags(struct v3d_job *job,
                           {
         cl_emit(&job->bcl, FLAT_SHADE_FLAGS, flags) {
            flags.varying_offset_v0 = varying_offset;
   flags.flat_shade_flags_for_varyings_v024 = varyings;
   flags.action_for_flat_shade_flags_of_lower_numbered_varyings =
            }
      #if V3D_VERSION >= 40
   static void
   emit_noperspective_flags(struct v3d_job *job,
                           {
         cl_emit(&job->bcl, NON_PERSPECTIVE_FLAGS, flags) {
            flags.varying_offset_v0 = varying_offset;
   flags.non_perspective_flags_for_varyings_v024 = varyings;
   flags.action_for_non_perspective_flags_of_lower_numbered_varyings =
            }
      static void
   emit_centroid_flags(struct v3d_job *job,
                     int varying_offset,
   {
         cl_emit(&job->bcl, CENTROID_FLAGS, flags) {
            flags.varying_offset_v0 = varying_offset;
   flags.centroid_flags_for_varyings_v024 = varyings;
   flags.action_for_centroid_flags_of_lower_numbered_varyings =
            }
   #endif /* V3D_VERSION >= 40 */
      static bool
   emit_varying_flags(struct v3d_job *job, uint32_t *flags,
                     void (*flag_emit_callback)(struct v3d_job *job,
         {
         struct v3d_context *v3d = job->v3d;
            for (int i = 0; i < ARRAY_SIZE(v3d->prog.fs->prog_data.fs->flat_shade_flags); i++) {
                           if (emitted_any) {
            flag_emit_callback(job, i, flags[i],
      } else if (i == 0) {
            flag_emit_callback(job, i, flags[i],
      } else {
            flag_emit_callback(job, i, flags[i],
                  }
      static inline struct v3d_uncompiled_shader *
   get_tf_shader(struct v3d_context *v3d)
   {
         if (v3d->prog.bind_gs)
         else
   }
      void
   v3dX(emit_state)(struct pipe_context *pctx)
   {
         struct v3d_context *v3d = v3d_context(pctx);
   struct v3d_job *job = v3d->job;
            if (v3d->dirty & (V3D_DIRTY_SCISSOR | V3D_DIRTY_VIEWPORT |
                  float *vpscale = v3d->viewport.scale;
   float *vptranslate = v3d->viewport.translate;
   float vp_minx = -fabsf(vpscale[0]) + vptranslate[0];
                        /* Clip to the scissor if it's enabled, but still clip to the
   * drawable regardless since that controls where the binner
   * tries to put things.
   *
   * Additionally, always clip the rendering to the viewport,
   * since the hardware does guardband clipping, meaning
   * primitives would rasterize outside of the view volume.
   */
   uint32_t minx, miny, maxx, maxy;
   if (!v3d->rasterizer->base.scissor) {
            minx = MAX2(vp_minx, 0);
   miny = MAX2(vp_miny, 0);
      } else {
            minx = MAX2(vp_minx, v3d->scissor.minx);
                     cl_emit(&job->bcl, CLIP_WINDOW, clip) {
            clip.clip_window_left_pixel_coordinate = minx;
   clip.clip_window_bottom_pixel_coordinate = miny;
   if (maxx > minx && maxy > miny) {
         clip.clip_window_width_in_pixels = maxx - minx;
   } else if (V3D_VERSION < 41) {
         /* The HW won't entirely clip out when scissor
      * w/h is 0.  Just treat it the same as
   * rasterizer discard.
                        job->draw_min_x = MIN2(job->draw_min_x, minx);
                        if (!v3d->rasterizer->base.scissor) {
         } else if (!job->scissor.disabled &&
                  if (job->scissor.count < MAX_JOB_SCISSORS) {
         job->scissor.rects[job->scissor.count].min_x =
         job->scissor.rects[job->scissor.count].min_y =
         job->scissor.rects[job->scissor.count].max_x =
         job->scissor.rects[job->scissor.count].max_y =
         } else {
                  if (v3d->dirty & (V3D_DIRTY_RASTERIZER |
                     V3D_DIRTY_ZSA |
      cl_emit(&job->bcl, CFG_BITS, config) {
            config.enable_forward_facing_primitive =
         !rasterizer_discard &&
   !(v3d->rasterizer->base.cull_face &
   config.enable_reverse_facing_primitive =
         !rasterizer_discard &&
   !(v3d->rasterizer->base.cull_face &
   /* This seems backwards, but it's what gets the
                                             /* V3D follows GL behavior where the sample mask only
   * applies when MSAA is enabled.  Gallium has sample
   * mask apply anyway, and the MSAA blit shaders will
   * set sample mask without explicitly setting
   * rasterizer oversample.  Just force it on here,
   * since the blit shaders are the only way to have
   * !multisample && samplemask != 0xf.
                                                #if V3D_VERSION <= 42
               #endif
                     #if V3D_VERSION <= 42
               #endif
                                                                           /* Use nicer line caps when line smoothing is
   * enabled
   */
      #if V3D_VERSION >= 71
                           /* The following follows the logic implemented in v3dv
   * plus the definition of depth_clip_near/far and
   * depth_clamp.
   *
   * Note: some extensions are not supported by v3d
   * (like ARB_depth_clamp) that would affect this, but
   * the values on rasterizer are taking that into
   * account.
   #endif
                        if (v3d->dirty & V3D_DIRTY_RASTERIZER &&
         v3d->rasterizer->base.offset_tri) {
      if (v3d->screen->devinfo.ver <= 42 &&
      job->zsbuf &&
   job->zsbuf->format == PIPE_FORMAT_Z16_UNORM) {
         cl_emit_prepacked_sized(&job->bcl,
      } else {
            cl_emit_prepacked_sized(&job->bcl,
            if (v3d->dirty & V3D_DIRTY_RASTERIZER) {
                                 cl_emit(&job->bcl, LINE_WIDTH, line_width) {
               #if V3D_VERSION <= 42
                  cl_emit(&job->bcl, CLIPPER_XY_SCALING, clip) {
            clip.viewport_half_width_in_1_256th_of_pixel =
   #endif
   #if V3D_VERSION >= 71
                  cl_emit(&job->bcl, CLIPPER_XY_SCALING, clip) {
            clip.viewport_half_width_in_1_64th_of_pixel =
   #endif
                        cl_emit(&job->bcl, CLIPPER_Z_SCALE_AND_OFFSET, clip) {
            clip.viewport_z_offset_zc_to_zs =
            }
   cl_emit(&job->bcl, CLIPPER_Z_MIN_MAX_CLIPPING_PLANES, clip) {
            float z1 = (v3d->viewport.translate[2] -
         float z2 = (v3d->viewport.translate[2] +
            #if V3D_VERSION < 41
                           #else
                                                      /* The fine coordinates must be unsigned, but coarse
   * can be signed.
   */
   if (unlikely(vp_fine_x < 0)) {
                                    if (unlikely(vp_fine_y < 0)) {
                                    #endif
                        if (v3d->dirty & V3D_DIRTY_BLEND) {
            #if V3D_VERSION >= 40
                     #endif
                              const uint32_t max_rts =
         if (blend->base.independent_blend_enable) {
         for (int i = 0; i < max_rts; i++)
               } else if (v3d->blend_dst_alpha_one &&
               /* Even if we don't have independent per-RT
      * blending, we may have a combination of RT
   * formats were some RTs have an alpha channel
   * and others don't. Since this affects how
   * blending is performed, we also need to emit
   * independent blend configurations in this
   * case: one for RTs with alpha and one for
   * RTs without.
      emit_rt_blend(v3d, job, &blend->base, 0,
                     emit_rt_blend(v3d, job, &blend->base, 0,
               } else {
         emit_rt_blend(v3d, job, &blend->base, 0,
            if (v3d->dirty & V3D_DIRTY_BLEND) {
                     const uint32_t max_rts =
         cl_emit(&job->bcl, COLOR_WRITE_MASKS, mask) {
                                                /* GFXH-1431: On V3D 3.x, writing BLEND_CONFIG resets the constant
      * color.
      if (v3d->dirty & V3D_DIRTY_BLEND_COLOR ||
         (V3D_VERSION < 41 && (v3d->dirty & V3D_DIRTY_BLEND))) {
      cl_emit(&job->bcl, BLEND_CONSTANT_COLOR, color) {
            color.red_f16 = (v3d->swap_color_rb ?
               color.green_f16 = v3d->blend_color.hf[1];
   color.blue_f16 = (v3d->swap_color_rb ?
                  if (v3d->dirty & (V3D_DIRTY_ZSA | V3D_DIRTY_STENCIL_REF)) {
                           if (front->enabled) {
            cl_emit_with_prepacked(&job->bcl, STENCIL_CFG,
                           if (back->enabled) {
            cl_emit_with_prepacked(&job->bcl, STENCIL_CFG,
                  #if V3D_VERSION < 40
         /* Pre-4.x, we have texture state that depends on both the sampler and
      * the view, so we merge them together at draw time.
      if (v3d->dirty & V3D_DIRTY_FRAGTEX)
            if (v3d->dirty & V3D_DIRTY_GEOMTEX)
            if (v3d->dirty & V3D_DIRTY_VERTTEX)
   #endif
            if (v3d->dirty & V3D_DIRTY_FLAT_SHADE_FLAGS) {
            if (!emit_varying_flags(job,
                     #if V3D_VERSION >= 40
         if (v3d->dirty & V3D_DIRTY_NOPERSPECTIVE_FLAGS) {
            if (!emit_varying_flags(job,
                           if (v3d->dirty & V3D_DIRTY_CENTROID_FLAGS) {
            if (!emit_varying_flags(job,
                  #endif
            /* Set up the transform feedback data specs (which VPM entries to
      * output to which buffers).
      if (v3d->dirty & (V3D_DIRTY_STREAMOUT |
                        struct v3d_streamout_stateobj *so = &v3d->streamout;
   if (so->num_targets) {
            bool psiz_per_vertex = (v3d->prim_mode == MESA_PRIM_POINTS &&
         struct v3d_uncompiled_shader *tf_shader =
      #if V3D_VERSION >= 40
                                          cl_emit(&job->bcl, TRANSFORM_FEEDBACK_SPECS, tfe) {
   #else /* V3D_VERSION < 40 */
                           cl_emit(&job->bcl, TRANSFORM_FEEDBACK_ENABLE, tfe) {
         #endif /* V3D_VERSION < 40 */
                           #if V3D_VERSION >= 40
                     #endif /* V3D_VERSION >= 40 */
                        /* Set up the transform feedback buffers. */
   if (v3d->dirty & V3D_DIRTY_STREAMOUT) {
            struct v3d_uncompiled_shader *tf_shader = get_tf_shader(v3d);
   struct v3d_streamout_stateobj *so = &v3d->streamout;
   for (int i = 0; i < so->num_targets; i++) {
            struct pipe_stream_output_target *target =
         struct v3d_resource *rsc = target ?
            #if V3D_VERSION >= 40
                                          cl_emit(&job->bcl, TRANSFORM_FEEDBACK_BUFFER, output) {
         output.buffer_address =
               #else /* V3D_VERSION < 40 */
                           cl_emit(&job->bcl, TRANSFORM_FEEDBACK_OUTPUT_ADDRESS, output) {
         if (target) {
         #endif /* V3D_VERSION < 40 */
                           if (target) {
         v3d_job_add_tf_write_resource(v3d->job,
            if (v3d->dirty & V3D_DIRTY_OQ) {
            cl_emit(&job->bcl, OCCLUSION_QUERY_COUNTER, counter) {
            if (v3d->active_queries && v3d->current_oq) {
      #if V3D_VERSION >= 40
         if (v3d->dirty & V3D_DIRTY_SAMPLE_STATE) {
            cl_emit(&job->bcl, SAMPLE_STATE, state) {
            /* Note: SampleCoverage was handled at the
   * frontend level by converting to sample_mask.
   */
   #endif
   }
