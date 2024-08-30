   /*
   * Copyright (C) 2023 Amazon.com, Inc. or its affiliates.
   * Copyright (C) 2018 Alyssa Rosenzweig
   * Copyright (C) 2020 Collabora Ltd.
   * Copyright © 2017 Intel Corporation
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
   */
      #include "gallium/auxiliary/util/u_blend.h"
   #include "pipe/p_defines.h"
   #include "pipe/p_state.h"
   #include "util/macros.h"
   #include "util/u_draw.h"
   #include "util/u_helpers.h"
   #include "util/u_memory.h"
   #include "util/u_prim.h"
   #include "util/u_sample_positions.h"
   #include "util/u_vbuf.h"
   #include "util/u_viewport.h"
      #include "genxml/gen_macros.h"
      #include "pan_afbc_cso.h"
   #include "pan_blend.h"
   #include "pan_blitter.h"
   #include "pan_bo.h"
   #include "pan_context.h"
   #include "pan_indirect_dispatch.h"
   #include "pan_job.h"
   #include "pan_pool.h"
   #include "pan_shader.h"
   #include "pan_texture.h"
   #include "pan_util.h"
      #define PAN_GPU_INDIRECTS (PAN_ARCH == 7)
      struct panfrost_rasterizer {
            #if PAN_ARCH <= 7
      /* Partially packed RSD words */
   struct mali_multisample_misc_packed multisample;
      #endif
   };
      struct panfrost_zsa_state {
               /* Is any depth, stencil, or alpha testing enabled? */
            /* Does the depth and stencil tests always pass? This ignores write
   * masks, we are only interested in whether pixels may be killed.
   */
            /* Are depth or stencil writes possible? */
         #if PAN_ARCH <= 7
      /* Prepacked words from the RSD */
   struct mali_multisample_misc_packed rsd_depth;
   struct mali_stencil_mask_misc_packed rsd_stencil;
      #else
      /* Depth/stencil descriptor template */
      #endif
   };
      struct panfrost_sampler_state {
      struct pipe_sampler_state base;
      };
      /* Misnomer: Sampler view corresponds to textures, not samplers */
      struct panfrost_sampler_view {
      struct pipe_sampler_view base;
   struct panfrost_pool_ref state;
   struct mali_texture_packed bifrost_descriptor;
   mali_ptr texture_bo;
            /* Pool used to allocate the descriptor. If NULL, defaults to the global
   * descriptor pool. Can be set for short lived descriptors, useful for
   * shader images on Valhall.
   */
      };
      struct panfrost_vertex_state {
      unsigned num_elements;
   struct pipe_vertex_element pipe[PIPE_MAX_ATTRIBS];
         #if PAN_ARCH >= 9
      /* Packed attribute descriptors */
      #else
      /* buffers corresponds to attribute buffer, element_buffers corresponds
   * to an index in buffers for each vertex element */
   struct pan_vertex_buffer buffers[PIPE_MAX_ATTRIBS];
   unsigned element_buffer[PIPE_MAX_ATTRIBS];
               #endif
   };
      /* Statically assert that PIPE_* enums match the hardware enums.
   * (As long as they match, we don't need to translate them.)
   */
   static_assert((int)PIPE_FUNC_NEVER == MALI_FUNC_NEVER, "must match");
   static_assert((int)PIPE_FUNC_LESS == MALI_FUNC_LESS, "must match");
   static_assert((int)PIPE_FUNC_EQUAL == MALI_FUNC_EQUAL, "must match");
   static_assert((int)PIPE_FUNC_LEQUAL == MALI_FUNC_LEQUAL, "must match");
   static_assert((int)PIPE_FUNC_GREATER == MALI_FUNC_GREATER, "must match");
   static_assert((int)PIPE_FUNC_NOTEQUAL == MALI_FUNC_NOT_EQUAL, "must match");
   static_assert((int)PIPE_FUNC_GEQUAL == MALI_FUNC_GEQUAL, "must match");
   static_assert((int)PIPE_FUNC_ALWAYS == MALI_FUNC_ALWAYS, "must match");
      static inline enum mali_sample_pattern
   panfrost_sample_pattern(unsigned samples)
   {
      switch (samples) {
   case 1:
         case 4:
         case 8:
         case 16:
         default:
            }
      static unsigned
   translate_tex_wrap(enum pipe_tex_wrap w, bool using_nearest)
   {
      /* CLAMP is only supported on Midgard, where it is broken for nearest
   * filtering. Use CLAMP_TO_EDGE in that case.
            switch (w) {
   case PIPE_TEX_WRAP_REPEAT:
         case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
         case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
         case PIPE_TEX_WRAP_MIRROR_REPEAT:
         case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
         case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
         #if PAN_ARCH <= 5
      case PIPE_TEX_WRAP_CLAMP:
      return using_nearest ? MALI_WRAP_MODE_CLAMP_TO_EDGE
      case PIPE_TEX_WRAP_MIRROR_CLAMP:
      return using_nearest ? MALI_WRAP_MODE_MIRRORED_CLAMP_TO_EDGE
   #endif
         default:
            }
      /* The hardware compares in the wrong order order, so we have to flip before
   * encoding. Yes, really. */
      static enum mali_func
   panfrost_sampler_compare_func(const struct pipe_sampler_state *cso)
   {
      return !cso->compare_mode
            }
      static enum mali_mipmap_mode
   pan_pipe_to_mipmode(enum pipe_tex_mipfilter f)
   {
      switch (f) {
   case PIPE_TEX_MIPFILTER_NEAREST:
         case PIPE_TEX_MIPFILTER_LINEAR:
      #if PAN_ARCH >= 6
      case PIPE_TEX_MIPFILTER_NONE:
      #else
      case PIPE_TEX_MIPFILTER_NONE:
      #endif
      default:
            }
      static void *
   panfrost_create_sampler_state(struct pipe_context *pctx,
         {
      struct panfrost_sampler_state *so = CALLOC_STRUCT(panfrost_sampler_state);
         #if PAN_ARCH == 7
      /* On v7, pan_texture.c composes the API swizzle with a bijective
   * swizzle derived from the format, to allow more formats than the
   * hardware otherwise supports. When packing border colours, we need to
   * undo this bijection, by swizzling with its inverse.
   */
   unsigned mali_format = panfrost_pipe_format_v7[cso->border_color_format].hw;
            unsigned char inverted_swizzle[4];
   panfrost_invert_swizzle(GENX(pan_decompose_swizzle)(order).post,
            util_format_apply_color_swizzle(&so->base.border_color, &cso->border_color,
            #endif
                  pan_pack(&so->hw, SAMPLER, cfg) {
      cfg.magnify_nearest = cso->mag_img_filter == PIPE_TEX_FILTER_NEAREST;
            cfg.normalized_coordinates = !cso->unnormalized_coords;
   cfg.lod_bias = cso->lod_bias;
   cfg.minimum_lod = cso->min_lod;
            cfg.wrap_mode_s = translate_tex_wrap(cso->wrap_s, using_nearest);
   cfg.wrap_mode_t = translate_tex_wrap(cso->wrap_t, using_nearest);
            cfg.mipmap_mode = pan_pipe_to_mipmode(cso->min_mip_filter);
   cfg.compare_function = panfrost_sampler_compare_func(cso);
            cfg.border_color_r = so->base.border_color.ui[0];
   cfg.border_color_g = so->base.border_color.ui[1];
   cfg.border_color_b = so->base.border_color.ui[2];
      #if PAN_ARCH >= 6
         if (cso->max_anisotropy > 1) {
      cfg.maximum_anisotropy = cso->max_anisotropy;
      #else
         /* Emulate disabled mipmapping by clamping the LOD as tight as
   * possible (from 0 to epsilon = 1/256) */
   if (cso->min_mip_filter == PIPE_TEX_MIPFILTER_NONE)
   #endif
                  }
      static bool
   panfrost_fs_required(struct panfrost_compiled_shader *fs,
                     {
      /* If we generally have side effects. This inclues use of discard,
   * which can affect the results of an occlusion query. */
   if (fs->info.fs.sidefx)
            /* Using an empty FS requires early-z to be enabled, but alpha test
   * needs it disabled. Alpha test is only native on Midgard, so only
   * check there.
   */
   if (PAN_ARCH <= 5 && zsa->base.alpha_func != PIPE_FUNC_ALWAYS)
            /* If colour is written we need to execute */
   for (unsigned i = 0; i < state->nr_cbufs; ++i) {
      if (state->cbufs[i] && blend->info[i].enabled)
               /* If depth is written and not implied we need to execute.
   * TODO: Predicate on Z/S writes being enabled */
      }
      /* Get pointers to the blend shaders bound to each active render target. Used
   * to emit the blend descriptors, as well as the fragment renderer state
   * descriptor.
   */
   static void
   panfrost_get_blend_shaders(struct panfrost_batch *batch,
         {
      unsigned shader_offset = 0;
            for (unsigned c = 0; c < batch->key.nr_cbufs; ++c) {
      if (batch->key.cbufs[c]) {
      blend_shaders[c] =
                  if (shader_bo)
      }
      #if PAN_ARCH >= 5
   UNUSED static uint16_t
   pack_blend_constant(enum pipe_format format, float cons)
   {
      const struct util_format_description *format_desc =
                     for (unsigned i = 0; i < format_desc->nr_channels; i++)
            uint16_t unorm = (cons * ((1 << chan_size) - 1));
      }
      /*
   * Determine whether to set the respective overdraw alpha flag.
   *
   * The overdraw alpha=1 flag should be set when alpha=1 implies full overdraw,
   * equivalently, all enabled render targets have alpha_one_store set. Likewise,
   * overdraw alpha=0 should be set when alpha=0 implies no overdraw,
   * equivalently, all enabled render targets have alpha_zero_nop set.
   */
   #if PAN_ARCH >= 6
   static bool
   panfrost_overdraw_alpha(const struct panfrost_context *ctx, bool zero)
   {
               for (unsigned i = 0; i < ctx->pipe_framebuffer.nr_cbufs; ++i) {
               bool enabled = ctx->pipe_framebuffer.cbufs[i] && !info.enabled;
            if (enabled && !flag)
                  }
   #endif
      static void
   panfrost_emit_blend(struct panfrost_batch *batch, void *rts,
         {
      unsigned rt_count = batch->key.nr_cbufs;
   struct panfrost_context *ctx = batch->ctx;
   const struct panfrost_blend_state *so = ctx->blend;
            /* Always have at least one render target for depth-only passes */
   for (unsigned i = 0; i < MAX2(rt_count, 1); ++i) {
               /* Disable blending for unbacked render targets */
   if (rt_count == 0 || !batch->key.cbufs[i] || !so->info[i].enabled) {
         #if PAN_ARCH >= 6
         #endif
                                 struct pan_blend_info info = so->info[i];
   enum pipe_format format = batch->key.cbufs[i]->format;
   float cons =
            /* Word 0: Flags and constant */
   pan_pack(packed, BLEND, cfg) {
      cfg.srgb = util_format_is_srgb(format);
   cfg.load_destination = info.load_dest;
      #if PAN_ARCH >= 6
               #else
                     if (blend_shaders[i])
            #endif
                  if (!blend_shaders[i]) {
      /* Word 1: Blend Equation */
   STATIC_ASSERT(pan_size(BLEND_EQUATION) == 4);
         #if PAN_ARCH >= 6
         const struct panfrost_device *dev = pan_device(ctx->base.screen);
            /* Words 2 and 3: Internal blend */
   if (blend_shaders[i]) {
      /* The blend shader's address needs to be at
   * the same top 32 bit as the fragment shader.
   * TODO: Ensure that's always the case.
   */
                  pan_pack(&packed->opaque[2], INTERNAL_BLEND, cfg) {
         #if PAN_ARCH <= 7
                        #endif
               } else {
      pan_pack(&packed->opaque[2], INTERNAL_BLEND, cfg) {
                     /* If we want the conversion to work properly,
   * num_comps must be set to 4
   */
   cfg.fixed_function.num_comps = 4;
   cfg.fixed_function.conversion.memory_format =
      #if PAN_ARCH <= 7
               if (!info.opaque) {
                        if (fs->info.fs.untyped_color_outputs) {
      cfg.fixed_function.conversion.register_format = GENX(
      } else {
         #endif
               #endif
         }
   #endif
      static inline bool
   pan_allow_forward_pixel_to_kill(struct panfrost_context *ctx,
         {
      /* Track if any colour buffer is reused across draws, either
   * from reading it directly, or from failing to write it
   */
   unsigned rt_mask = ctx->fb_rt_mask;
   uint64_t rt_written = (fs->info.outputs_written >> FRAG_RESULT_DATA0) &
         bool blend_reads_dest = (ctx->blend->load_dest_mask & rt_mask);
            return fs->info.fs.can_fpk && !(rt_mask & ~rt_written) &&
      }
      static mali_ptr
   panfrost_emit_compute_shader_meta(struct panfrost_batch *batch,
         {
               panfrost_batch_add_bo(batch, ss->bin.bo, PIPE_SHADER_VERTEX);
               }
      #if PAN_ARCH <= 7
   /* Construct a partial RSD corresponding to no executed fragment shader, and
   * merge with the existing partial RSD. */
      static void
   pan_merge_empty_fs(struct mali_renderer_state_packed *rsd)
   {
                  #if PAN_ARCH >= 6
         cfg.properties.shader_modifies_coverage = true;
   cfg.properties.allow_forward_pixel_to_kill = true;
   cfg.properties.allow_forward_pixel_to_be_killed = true;
            /* Alpha isn't written so these are vacuous */
   cfg.multisample_misc.overdraw_alpha0 = true;
   #else
         cfg.shader.shader = 0x1;
   cfg.properties.work_register_count = 1;
   cfg.properties.depth_source = MALI_DEPTH_SOURCE_FIXED_FUNCTION;
   #endif
                  }
      static void
   panfrost_prepare_fs_state(struct panfrost_context *ctx, mali_ptr *blend_shaders,
         {
      struct pipe_rasterizer_state *rast = &ctx->rasterizer->base;
   const struct panfrost_zsa_state *zsa = ctx->depth_stencil;
   struct panfrost_compiled_shader *fs = ctx->prog[PIPE_SHADER_FRAGMENT];
   struct panfrost_blend_state *so = ctx->blend;
   bool alpha_to_coverage = ctx->blend->base.alpha_to_coverage;
                              for (unsigned c = 0; c < rt_count; ++c)
                     pan_pack(rsd, RENDERER_STATE, cfg) {
      #if PAN_ARCH >= 6
            struct pan_earlyzs_state earlyzs = pan_earlyzs_get(
      fs->earlyzs, ctx->depth_stencil->writes_zs || has_oq,
                                 #else
            cfg.properties.force_early_z =
                  /* TODO: Reduce this limit? */
   if (has_blend_shader)
      cfg.properties.work_register_count =
                     /* Hardware quirks around early-zs forcing without a
                  cfg.properties.shader_reads_tilebuffer =
            #endif
            #if PAN_ARCH == 4
         if (rt_count > 0) {
      cfg.multisample_misc.load_destination = so->info[0].load_dest;
   cfg.multisample_misc.blend_shader = (blend_shaders[0] != 0);
   cfg.stencil_mask_misc.write_enable = so->info[0].enabled;
   cfg.stencil_mask_misc.srgb =
                        if (blend_shaders[0]) {
         } else {
      cfg.blend_constant = pan_blend_get_constant(
         } else {
      /* If there is no colour buffer, leaving fields default is
   * fine, except for blending which is nonnullable */
   cfg.blend_equation.color_mask = 0xf;
   cfg.blend_equation.rgb.a = MALI_BLEND_OPERAND_A_SRC;
   cfg.blend_equation.rgb.b = MALI_BLEND_OPERAND_B_SRC;
   cfg.blend_equation.rgb.c = MALI_BLEND_OPERAND_C_ZERO;
   cfg.blend_equation.alpha.a = MALI_BLEND_OPERAND_A_SRC;
   cfg.blend_equation.alpha.b = MALI_BLEND_OPERAND_B_SRC;
      #elif PAN_ARCH == 5
         /* Workaround */
   #endif
                        #if PAN_ARCH >= 6
         /* MSAA blend shaders need to pass their sample ID to
   * LD_TILE/ST_TILE, so we must preload it. Additionally, we
   * need per-sample shading for the blend shader, accomplished
            if (msaa && has_blend_shader) {
      cfg.multisample_misc.evaluate_per_sample = true;
               /* Bifrost does not have native point sprites. Point sprites are
   * lowered in the driver to gl_PointCoord reads. This field
   * actually controls the orientation of gl_PointCoord. Both
   * orientations are controlled with sprite_coord_mode in
   * Gallium.
   */
   cfg.properties.point_sprite_coord_origin_max_y =
            cfg.multisample_misc.overdraw_alpha0 = panfrost_overdraw_alpha(ctx, 0);
   #endif
            cfg.stencil_mask_misc.alpha_to_coverage = alpha_to_coverage;
   cfg.depth_units = rast->offset_units * 2.0f;
   cfg.depth_factor = rast->offset_scale;
            bool back_enab = zsa->base.stencil[1].enabled;
   cfg.stencil_front.reference_value = ctx->stencil_ref.ref_value[0];
   cfg.stencil_back.reference_value =
      #if PAN_ARCH <= 5
         /* v6+ fits register preload here, no alpha testing */
   #endif
         }
      static void
   panfrost_emit_frag_shader(struct panfrost_context *ctx,
               {
      const struct panfrost_zsa_state *zsa = ctx->depth_stencil;
   const struct panfrost_rasterizer *rast = ctx->rasterizer;
            /* We need to merge several several partial renderer state descriptors,
   * so stage to temporary storage rather than reading back write-combine
   * memory, which will trash performance. */
   struct mali_renderer_state_packed rsd;
         #if PAN_ARCH == 4
      if (ctx->pipe_framebuffer.nr_cbufs > 0 && !blend_shaders[0]) {
      /* Word 14: SFBD Blend Equation */
   STATIC_ASSERT(pan_size(BLEND_EQUATION) == 4);
         #endif
         /* Merge with CSO state and upload */
   if (panfrost_fs_required(fs, ctx->blend, &ctx->pipe_framebuffer, zsa)) {
      struct mali_renderer_state_packed *partial_rsd =
         STATIC_ASSERT(sizeof(fs->partial_rsd) == sizeof(*partial_rsd));
      } else {
                  /* Word 8, 9 Misc state */
                     /* Word 10, 11 Stencil Front and Back */
   rsd.opaque[10] |= zsa->stencil_front.opaque[0];
               }
      static mali_ptr
   panfrost_emit_frag_shader_meta(struct panfrost_batch *batch)
   {
      struct panfrost_context *ctx = batch->ctx;
                           #if PAN_ARCH == 4
         #else
               xfer =
      pan_pool_alloc_desc_aggregate(&batch->pool.base, PAN_DESC(RENDERER_STATE),
   #endif
         mali_ptr blend_shaders[PIPE_MAX_COLOR_BUFS] = {0};
            panfrost_emit_frag_shader(ctx, (struct mali_renderer_state_packed *)xfer.cpu,
         #if PAN_ARCH >= 5
      panfrost_emit_blend(batch, xfer.cpu + pan_size(RENDERER_STATE),
      #endif
            }
   #endif
      static mali_ptr
   panfrost_emit_viewport(struct panfrost_batch *batch)
   {
      struct panfrost_context *ctx = batch->ctx;
   const struct pipe_viewport_state *vp = &ctx->pipe_viewport;
   const struct pipe_scissor_state *ss = &ctx->scissor;
            /* Derive min/max from translate/scale. Note since |x| >= 0 by
   * definition, we have that -|x| <= |x| hence translate - |scale| <=
   * translate + |scale|, so the ordering is correct here. */
   float vp_minx = vp->translate[0] - fabsf(vp->scale[0]);
   float vp_maxx = vp->translate[0] + fabsf(vp->scale[0]);
   float vp_miny = vp->translate[1] - fabsf(vp->scale[1]);
            float minz, maxz;
            /* Scissor to the intersection of viewport and to the scissor, clamped
            unsigned minx = MIN2(batch->key.width, MAX2((int)vp_minx, 0));
   unsigned maxx = MIN2(batch->key.width, MAX2((int)vp_maxx, 0));
   unsigned miny = MIN2(batch->key.height, MAX2((int)vp_miny, 0));
            if (ss && rast->scissor) {
      minx = MAX2(ss->minx, minx);
   miny = MAX2(ss->miny, miny);
   maxx = MIN2(ss->maxx, maxx);
               /* Set the range to [1, 1) so max values don't wrap round */
   if (maxx == 0 || maxy == 0)
            panfrost_batch_union_scissor(batch, minx, miny, maxx, maxy);
            /* [minx, maxx) and [miny, maxy) are exclusive ranges in the hardware */
   maxx--;
            batch->minimum_z = rast->depth_clip_near ? minz : -INFINITY;
         #if PAN_ARCH <= 7
               pan_pack(T.cpu, VIEWPORT, cfg) {
      cfg.scissor_minimum_x = minx;
   cfg.scissor_minimum_y = miny;
   cfg.scissor_maximum_x = maxx;
            cfg.minimum_z = batch->minimum_z;
                  #else
      pan_pack(&batch->scissor, SCISSOR, cfg) {
      cfg.scissor_minimum_x = minx;
   cfg.scissor_minimum_y = miny;
   cfg.scissor_maximum_x = maxx;
                  #endif
   }
      #if PAN_ARCH >= 9
   /**
   * Emit a Valhall depth/stencil descriptor at draw-time. The bulk of the
   * descriptor corresponds to a pipe_depth_stencil_alpha CSO and is packed at
   * CSO create time. However, the stencil reference values and shader
   * interactions are dynamic state. Pack only the dynamic state here and OR
   * together.
   */
   static mali_ptr
   panfrost_emit_depth_stencil(struct panfrost_batch *batch)
   {
      struct panfrost_context *ctx = batch->ctx;
   const struct panfrost_zsa_state *zsa = ctx->depth_stencil;
   struct panfrost_rasterizer *rast = ctx->rasterizer;
   struct panfrost_compiled_shader *fs = ctx->prog[PIPE_SHADER_FRAGMENT];
            struct panfrost_ptr T =
                  pan_pack(&dynamic, DEPTH_STENCIL, cfg) {
      cfg.front_reference_value = ctx->stencil_ref.ref_value[0];
            cfg.stencil_from_shader = fs->info.fs.writes_stencil;
            cfg.depth_bias_enable = rast->base.offset_tri;
   cfg.depth_units = rast->base.offset_units * 2.0f;
   cfg.depth_factor = rast->base.offset_scale;
               pan_merge(dynamic, zsa->desc, DEPTH_STENCIL);
               }
      /**
   * Emit Valhall blend descriptor at draw-time. The descriptor itself is shared
   * with Bifrost, but the container data structure is simplified.
   */
   static mali_ptr
   panfrost_emit_blend_valhall(struct panfrost_batch *batch)
   {
               struct panfrost_ptr T =
            mali_ptr blend_shaders[PIPE_MAX_COLOR_BUFS] = {0};
                     /* Precalculate for the per-draw path */
            for (unsigned i = 0; i < rt_count; ++i)
                        }
      /**
   * Emit Valhall buffer descriptors for bound vertex buffers at draw-time.
   */
   static mali_ptr
   panfrost_emit_vertex_buffers(struct panfrost_batch *batch)
   {
      struct panfrost_context *ctx = batch->ctx;
   unsigned buffer_count = util_last_bit(ctx->vb_mask);
   struct panfrost_ptr T =
                  u_foreach_bit(i, ctx->vb_mask) {
      struct pipe_vertex_buffer vb = ctx->vertex_buffers[i];
   struct pipe_resource *prsrc = vb.buffer.resource;
   struct panfrost_resource *rsrc = pan_resource(prsrc);
                     pan_pack(buffers + i, BUFFER, cfg) {
                                 }
      static mali_ptr
   panfrost_emit_vertex_data(struct panfrost_batch *batch)
   {
      struct panfrost_context *ctx = batch->ctx;
            return pan_pool_upload_aligned(&batch->pool.base, vtx->attributes,
            }
      static void panfrost_update_sampler_view(struct panfrost_sampler_view *view,
            static mali_ptr
   panfrost_emit_images(struct panfrost_batch *batch, enum pipe_shader_type stage)
   {
      struct panfrost_context *ctx = batch->ctx;
            struct panfrost_ptr T =
                     for (int i = 0; i < last_bit; ++i) {
               if (!(ctx->image_mask[stage] & BITFIELD_BIT(i))) {
      memset(&out[i], 0, sizeof(out[i]));
               /* Construct a synthetic sampler view so we can use our usual
   * sampler view code for the actual descriptor packing.
   *
   * Use the batch pool for a transient allocation, rather than
   * allocating a long-lived descriptor.
   */
   struct panfrost_sampler_view view = {
      .base = util_image_to_sampler_view(image),
               /* If we specify a cube map, the hardware internally treat it as
   * a 2D array. Since cube maps as images can confuse our common
   * texturing code, explicitly use a 2D array.
   *
   * Similar concerns apply to 3D textures.
   */
   if (view.base.target == PIPE_BUFFER)
         else
            panfrost_update_sampler_view(&view, &ctx->base);
                           }
   #endif
      static mali_ptr
   panfrost_map_constant_buffer_gpu(struct panfrost_batch *batch,
                     {
      struct pipe_constant_buffer *cb = &buf->cb[index];
            if (rsrc) {
               /* Alignment gauranteed by
   * PIPE_CAP_CONSTANT_BUFFER_OFFSET_ALIGNMENT */
      } else if (cb->user_buffer) {
      return pan_pool_upload_aligned(&batch->pool.base,
            } else {
            }
      struct sysval_uniform {
      union {
      float f[4];
   int32_t i[4];
   uint32_t u[4];
         };
      static void
   panfrost_upload_viewport_scale_sysval(struct panfrost_batch *batch,
         {
      struct panfrost_context *ctx = batch->ctx;
            uniform->f[0] = vp->scale[0];
   uniform->f[1] = vp->scale[1];
      }
      static void
   panfrost_upload_viewport_offset_sysval(struct panfrost_batch *batch,
         {
      struct panfrost_context *ctx = batch->ctx;
            uniform->f[0] = vp->translate[0];
   uniform->f[1] = vp->translate[1];
      }
      static void
   panfrost_upload_txs_sysval(struct panfrost_batch *batch,
               {
      struct panfrost_context *ctx = batch->ctx;
   unsigned texidx = PAN_SYSVAL_ID_TO_TXS_TEX_IDX(sysvalid);
   unsigned dim = PAN_SYSVAL_ID_TO_TXS_DIM(sysvalid);
   bool is_array = PAN_SYSVAL_ID_TO_TXS_IS_ARRAY(sysvalid);
                     if (tex->target == PIPE_BUFFER) {
      assert(dim == 1);
   uniform->i[0] = tex->u.buf.size / util_format_get_blocksize(tex->format);
                        if (dim > 1)
            if (dim > 2)
            if (is_array) {
               /* Internally, we store the number of 2D images (faces * array
   * size). Externally, we report the array size in terms of
   * complete cubes. So divide by the # of faces per cube.
   */
   if (tex->target == PIPE_TEXTURE_CUBE_ARRAY)
                  }
      static void
   panfrost_upload_image_size_sysval(struct panfrost_batch *batch,
                     {
      struct panfrost_context *ctx = batch->ctx;
   unsigned idx = PAN_SYSVAL_ID_TO_TXS_TEX_IDX(sysvalid);
   unsigned dim = PAN_SYSVAL_ID_TO_TXS_DIM(sysvalid);
                              if (image->resource->target == PIPE_BUFFER) {
      unsigned blocksize = util_format_get_blocksize(image->format);
   uniform->i[0] = image->resource->width0 / blocksize;
                        if (dim > 1)
            if (dim > 2)
            if (is_array)
      }
      static void
   panfrost_upload_ssbo_sysval(struct panfrost_batch *batch,
               {
               assert(ctx->ssbo_mask[st] & (1 << ssbo_id));
            /* Compute address */
   struct panfrost_resource *rsrc = pan_resource(sb.buffer);
                     util_range_add(&rsrc->base, &rsrc->valid_buffer_range, sb.buffer_offset,
            /* Upload address and size as sysval */
   uniform->du[0] = bo->ptr.gpu + sb.buffer_offset;
      }
      static void
   panfrost_upload_sampler_sysval(struct panfrost_batch *batch,
               {
      struct panfrost_context *ctx = batch->ctx;
            uniform->f[0] = sampl->min_lod;
   uniform->f[1] = sampl->max_lod;
            /* Even without any errata, Midgard represents "no mipmapping" as
   * fixing the LOD with the clamps; keep behaviour consistent. c.f.
   * panfrost_create_sampler_state which also explains our choice of
            if (sampl->min_mip_filter == PIPE_TEX_MIPFILTER_NONE)
      }
      static void
   panfrost_upload_num_work_groups_sysval(struct panfrost_batch *batch,
         {
               uniform->u[0] = ctx->compute_grid->grid[0];
   uniform->u[1] = ctx->compute_grid->grid[1];
      }
      static void
   panfrost_upload_local_group_size_sysval(struct panfrost_batch *batch,
         {
               uniform->u[0] = ctx->compute_grid->block[0];
   uniform->u[1] = ctx->compute_grid->block[1];
      }
      static void
   panfrost_upload_work_dim_sysval(struct panfrost_batch *batch,
         {
                  }
      /* Sample positions are pushed in a Bifrost specific format on Bifrost. On
   * Midgard, we emulate the Bifrost path with some extra arithmetic in the
   * shader, to keep the code as unified as possible. */
      static void
   panfrost_upload_sample_positions_sysval(struct panfrost_batch *batch,
         {
      struct panfrost_context *ctx = batch->ctx;
            unsigned samples = util_framebuffer_get_num_samples(&batch->key);
   uniform->du[0] =
      }
      static void
   panfrost_upload_multisampled_sysval(struct panfrost_batch *batch,
         {
      unsigned samples = util_framebuffer_get_num_samples(&batch->key);
      }
      #if PAN_ARCH >= 6
   static void
   panfrost_upload_rt_conversion_sysval(struct panfrost_batch *batch,
               {
      struct panfrost_context *ctx = batch->ctx;
   struct panfrost_device *dev = pan_device(ctx->base.screen);
   unsigned rt = size_and_rt & 0xF;
            if (rt < batch->key.nr_cbufs && batch->key.cbufs[rt]) {
      enum pipe_format format = batch->key.cbufs[rt]->format;
   uniform->u[0] =
      } else {
      pan_pack(&uniform->u[0], INTERNAL_CONVERSION, cfg)
         }
   #endif
      static unsigned
   panfrost_xfb_offset(unsigned stride, struct pipe_stream_output_target *target)
   {
         }
      static void
   panfrost_upload_sysvals(struct panfrost_batch *batch, void *ptr_cpu,
               {
               for (unsigned i = 0; i < ss->sysvals.sysval_count; ++i) {
               switch (PAN_SYSVAL_TYPE(sysval)) {
   case PAN_SYSVAL_VIEWPORT_SCALE:
      panfrost_upload_viewport_scale_sysval(batch, &uniforms[i]);
      case PAN_SYSVAL_VIEWPORT_OFFSET:
      panfrost_upload_viewport_offset_sysval(batch, &uniforms[i]);
      case PAN_SYSVAL_TEXTURE_SIZE:
      panfrost_upload_txs_sysval(batch, st, PAN_SYSVAL_ID(sysval),
            case PAN_SYSVAL_SSBO:
      panfrost_upload_ssbo_sysval(batch, st, PAN_SYSVAL_ID(sysval),
               case PAN_SYSVAL_XFB: {
      unsigned buf = PAN_SYSVAL_ID(sysval);
   struct panfrost_compiled_shader *vs =
                        struct pipe_stream_output_target *target = NULL;
                  if (!target) {
      /* Memory sink */
   uniforms[i].du[0] = 0x8ull << 60;
                                                      uniforms[i].du[0] = rsrc->image.data.bo->ptr.gpu + offset;
               case PAN_SYSVAL_NUM_VERTICES:
                  case PAN_SYSVAL_NUM_WORK_GROUPS:
      for (unsigned j = 0; j < 3; j++) {
      batch->num_wg_sysval[j] =
      }
   panfrost_upload_num_work_groups_sysval(batch, &uniforms[i]);
      case PAN_SYSVAL_LOCAL_GROUP_SIZE:
      panfrost_upload_local_group_size_sysval(batch, &uniforms[i]);
      case PAN_SYSVAL_WORK_DIM:
      panfrost_upload_work_dim_sysval(batch, &uniforms[i]);
      case PAN_SYSVAL_SAMPLER:
      panfrost_upload_sampler_sysval(batch, st, PAN_SYSVAL_ID(sysval),
            case PAN_SYSVAL_IMAGE_SIZE:
      panfrost_upload_image_size_sysval(batch, st, PAN_SYSVAL_ID(sysval),
            case PAN_SYSVAL_SAMPLE_POSITIONS:
      panfrost_upload_sample_positions_sysval(batch, &uniforms[i]);
      case PAN_SYSVAL_MULTISAMPLED:
         #if PAN_ARCH >= 6
         case PAN_SYSVAL_RT_CONVERSION:
      panfrost_upload_rt_conversion_sysval(batch, PAN_SYSVAL_ID(sysval),
      #endif
         case PAN_SYSVAL_VERTEX_INSTANCE_OFFSETS:
      uniforms[i].u[0] = batch->ctx->offset_start;
   uniforms[i].u[1] = batch->ctx->base_vertex;
   uniforms[i].u[2] = batch->ctx->base_instance;
      case PAN_SYSVAL_DRAWID:
      uniforms[i].u[0] = batch->ctx->drawid;
      default:
               }
      static const void *
   panfrost_map_constant_buffer_cpu(struct panfrost_context *ctx,
               {
      struct pipe_constant_buffer *cb = &buf->cb[index];
            if (rsrc) {
      panfrost_bo_mmap(rsrc->image.data.bo);
   panfrost_flush_writer(ctx, rsrc, "CPU constant buffer mapping");
               } else if (cb->user_buffer) {
         } else
      }
      /* Emit a single UBO record. On Valhall, UBOs are dumb buffers and are
   * implemented with buffer descriptors in the resource table, sized in terms of
   * bytes. On Bifrost and older, UBOs have special uniform buffer data
   * structure, sized in terms of entries.
   */
   static void
   panfrost_emit_ubo(void *base, unsigned index, mali_ptr address, size_t size)
   {
   #if PAN_ARCH >= 9
               pan_pack(out + index, BUFFER, cfg) {
      cfg.size = size;
         #else
               /* Issue (57) for the ARB_uniform_buffer_object spec says that
   * the buffer can be larger than the uniform data inside it,
            pan_pack(out + index, UNIFORM_BUFFER, cfg) {
      cfg.entries = MIN2(DIV_ROUND_UP(size, 16), 1 << 12);
         #endif
   }
      static mali_ptr
   panfrost_emit_const_buf(struct panfrost_batch *batch,
               {
      struct panfrost_context *ctx = batch->ctx;
   struct panfrost_constant_buffer *buf = &ctx->constant_buffer[stage];
            if (!ss)
            /* Allocate room for the sysval and the uniforms */
   size_t sys_size = sizeof(float) * 4 * ss->sysvals.sysval_count;
   struct panfrost_ptr transfer =
            /* Upload sysvals requested by the shader */
   uint8_t *sysvals = alloca(sys_size);
   panfrost_upload_sysvals(batch, sysvals, transfer.gpu, ss, stage);
            /* Next up, attach UBOs. UBO count includes gaps but no sysval UBO */
   struct panfrost_compiled_shader *shader = ctx->prog[stage];
   unsigned ubo_count = shader->info.ubo_count - (sys_size ? 1 : 0);
   unsigned sysval_ubo = sys_size ? ubo_count : ~0;
         #if PAN_ARCH >= 9
         #else
      ubos = pan_pool_alloc_desc_array(&batch->pool.base, ubo_count + 1,
      #endif
         if (buffer_count)
                     if (sys_size)
                     u_foreach_bit(ubo, ss->info.ubo_mask & buf->enabled_mask) {
      size_t usz = buf->cb[ubo].buffer_size;
            if (usz > 0) {
                              if (pushed_words)
            if (ss->info.push.count == 0)
            /* Copy push constants required by the shader */
   struct panfrost_ptr push_transfer =
            uint32_t *push_cpu = (uint32_t *)push_transfer.cpu;
            for (unsigned i = 0; i < ss->info.push.count; ++i) {
               if (src.ubo == sysval_ubo) {
      unsigned sysval_idx = src.offset / 16;
   unsigned sysval_comp = (src.offset % 16) / 4;
   unsigned sysval_type =
                  if (sysval_type == PAN_SYSVAL_NUM_WORK_GROUPS)
      }
   /* Map the UBO, this should be cheap. For some buffers this may
   * read from write-combine memory which is slow, though :-(
   */
   const void *mapped_ubo =
      (src.ubo == sysval_ubo)
               /* TODO: Is there any benefit to combining ranges */
                  }
      /*
   * Choose the number of WLS instances to allocate. This must be a power-of-two.
   * The number of WLS instances limits the number of concurrent tasks on a given
   * shader core, setting to the (rounded) total number of tasks avoids any
   * throttling. Smaller values save memory at the expense of possible throttling.
   *
   * With indirect dispatch, we don't know at launch-time how many tasks will be
   * needed, so we use a conservative value that's unlikely to cause slowdown in
   * practice without wasting too much memory.
   */
   static unsigned
   panfrost_choose_wls_instance_count(const struct pipe_grid_info *grid)
   {
      if (grid->indirect) {
      /* May need tuning in the future, conservative guess */
      } else {
      return util_next_power_of_two(grid->grid[0]) *
               }
      static mali_ptr
   panfrost_emit_shared_memory(struct panfrost_batch *batch,
         {
      struct panfrost_context *ctx = batch->ctx;
   struct panfrost_device *dev = pan_device(ctx->base.screen);
   struct panfrost_compiled_shader *ss = ctx->prog[PIPE_SHADER_COMPUTE];
   struct panfrost_ptr t =
            struct pan_tls_info info = {
      .tls.size = ss->info.tls_size,
   .wls.size = ss->info.wls_size + grid->variable_shared_mem,
               if (ss->info.tls_size) {
      struct panfrost_bo *bo = panfrost_batch_get_scratchpad(
                     if (info.wls.size) {
      unsigned size = pan_wls_adjust_size(info.wls.size) * info.wls.instances *
                                 GENX(pan_emit_tls)(&info, t.cpu);
      }
      #if PAN_ARCH <= 5
   static mali_ptr
   panfrost_get_tex_desc(struct panfrost_batch *batch, enum pipe_shader_type st,
         {
      if (!view)
            struct pipe_sampler_view *pview = &view->base;
            panfrost_batch_read_rsrc(batch, rsrc, st);
               }
   #endif
      static void
   panfrost_create_sampler_view_bo(struct panfrost_sampler_view *so,
               {
      struct panfrost_device *device = pan_device(pctx->screen);
   struct panfrost_context *ctx = pan_context(pctx);
   struct panfrost_resource *prsrc = (struct panfrost_resource *)texture;
   enum pipe_format format = so->base.format;
            /* Format to access the stencil/depth portion of a Z32_S8 texture */
   if (format == PIPE_FORMAT_X32_S8X24_UINT) {
      assert(prsrc->separate_stencil);
   texture = &prsrc->separate_stencil->base;
   prsrc = (struct panfrost_resource *)texture;
      } else if (format == PIPE_FORMAT_Z32_FLOAT_S8X24_UINT) {
                  so->texture_bo = prsrc->image.data.bo->ptr.gpu;
                     assert(texture->nr_samples <= 1 || so->base.target == PIPE_TEXTURE_2D ||
            enum mali_texture_dimension type =
                     unsigned first_level = is_buffer ? 0 : so->base.u.tex.first_level;
   unsigned last_level = is_buffer ? 0 : so->base.u.tex.last_level;
   unsigned first_layer = is_buffer ? 0 : so->base.u.tex.first_layer;
   unsigned last_layer = is_buffer ? 0 : so->base.u.tex.last_layer;
   unsigned buf_offset = is_buffer ? so->base.u.buf.offset : 0;
   unsigned buf_size =
            if (so->base.target == PIPE_TEXTURE_3D) {
      first_layer /= prsrc->image.layout.depth;
   last_layer /= prsrc->image.layout.depth;
               struct pan_image_view iview = {
      .format = format,
   .dim = type,
   .first_level = first_level,
   .last_level = last_level,
   .first_layer = first_layer,
   .last_layer = last_layer,
   .swizzle =
      {
      so->base.swizzle_r,
   so->base.swizzle_g,
   so->base.swizzle_b,
         .planes = {NULL},
   .buf.offset = buf_offset,
                        unsigned size = (PAN_ARCH <= 5 ? pan_size(TEXTURE) : 0) +
            struct panfrost_pool *pool = so->pool ?: &ctx->descs;
   struct panfrost_ptr payload = pan_pool_alloc_aligned(&pool->base, size, 64);
                     if (PAN_ARCH <= 5) {
      payload.cpu += pan_size(TEXTURE);
                  }
      static void
   panfrost_update_sampler_view(struct panfrost_sampler_view *view,
         {
      struct panfrost_resource *rsrc = pan_resource(view->base.texture);
   if (view->texture_bo != rsrc->image.data.bo->ptr.gpu ||
      view->modifier != rsrc->image.layout.modifier) {
   panfrost_bo_unreference(view->state.bo);
         }
      #if PAN_ARCH >= 6
   static void
   panfrost_emit_null_texture(struct mali_texture_packed *out)
      {
      /* Annoyingly, an all zero texture descriptor is not valid and will raise
   * a DATA_INVALID_FAULT if you try to texture it, instead of returning
   * 0000s! Fill in with sometthing that will behave robustly.
   */
   pan_pack(out, TEXTURE, cfg) {
      cfg.dimension = MALI_TEXTURE_DIMENSION_2D;
   cfg.width = 1;
   cfg.height = 1;
   cfg.depth = 1;
   cfg.array_size = 1;
   cfg.format = PAN_ARCH >= 7 ? panfrost_pipe_format_v7[PIPE_FORMAT_NONE].hw
   #if PAN_ARCH <= 7
         #endif
         }
   #endif
      static mali_ptr
   panfrost_emit_texture_descriptors(struct panfrost_batch *batch,
         {
               unsigned actual_count = ctx->sampler_view_count[stage];
   unsigned needed_count = ctx->prog[stage]->info.texture_count;
            if (!alloc_count)
         #if PAN_ARCH >= 6
      struct panfrost_ptr T =
                  for (int i = 0; i < actual_count; ++i) {
               if (!view) {
      panfrost_emit_null_texture(&out[i]);
               struct pipe_sampler_view *pview = &view->base;
            panfrost_update_sampler_view(view, &ctx->base);
            panfrost_batch_read_rsrc(batch, rsrc, stage);
               for (int i = actual_count; i < needed_count; ++i)
               #else
               for (int i = 0; i < actual_count; ++i) {
               if (!view) {
      trampolines[i] = 0;
                                    for (int i = actual_count; i < needed_count; ++i)
            return pan_pool_upload_aligned(&batch->pool.base, trampolines,
            #endif
   }
      static mali_ptr
   panfrost_upload_wa_sampler(struct panfrost_batch *batch)
   {
      struct panfrost_ptr T = pan_pool_alloc_desc(&batch->pool.base, SAMPLER);
   pan_pack(T.cpu, SAMPLER, cfg)
            }
      static mali_ptr
   panfrost_emit_sampler_descriptors(struct panfrost_batch *batch,
         {
               /* We always need at least 1 sampler for txf to work */
   if (!ctx->sampler_count[stage])
            struct panfrost_ptr T = pan_pool_alloc_desc_array(
                  for (unsigned i = 0; i < ctx->sampler_count[stage]; ++i) {
                              }
      #if PAN_ARCH <= 7
   /* Packs all image attribute descs and attribute buffer descs.
   * `first_image_buf_index` must be the index of the first image attribute buffer
   * descriptor.
   */
   static void
   emit_image_attribs(struct panfrost_context *ctx, enum pipe_shader_type shader,
         {
      struct panfrost_device *dev = pan_device(ctx->base.screen);
            for (unsigned i = 0; i < last_bit; ++i) {
               pan_pack(attribs + i, ATTRIBUTE, cfg) {
      /* Continuation record means 2 buffers per image */
   cfg.buffer_index = first_buf + (i * 2);
   cfg.offset_enable = (PAN_ARCH <= 5);
            }
      static enum mali_attribute_type
   pan_modifier_to_attr_type(uint64_t modifier)
   {
      switch (modifier) {
   case DRM_FORMAT_MOD_LINEAR:
         case DRM_FORMAT_MOD_ARM_16X16_BLOCK_U_INTERLEAVED:
         default:
            }
      static void
   emit_image_bufs(struct panfrost_batch *batch, enum pipe_shader_type shader,
               {
      struct panfrost_context *ctx = batch->ctx;
            for (unsigned i = 0; i < last_bit; ++i) {
               if (!(ctx->image_mask[shader] & (1 << i)) ||
      !(image->shader_access & PIPE_IMAGE_ACCESS_READ_WRITE)) {
   /* Unused image bindings */
   pan_pack(bufs + (i * 2), ATTRIBUTE_BUFFER, cfg)
         pan_pack(bufs + (i * 2) + 1, ATTRIBUTE_BUFFER, cfg)
                              /* TODO: MSAA */
            bool is_3d = rsrc->base.target == PIPE_TEXTURE_3D;
            unsigned offset = is_buffer ? image->u.buf.offset
                                       pan_pack(bufs + (i * 2), ATTRIBUTE_BUFFER, cfg) {
      cfg.type = pan_modifier_to_attr_type(rsrc->image.layout.modifier);
   cfg.pointer = rsrc->image.data.bo->ptr.gpu + offset;
   cfg.stride = util_format_get_blocksize(image->format);
               if (is_buffer) {
      pan_pack(bufs + (i * 2) + 1, ATTRIBUTE_BUFFER_CONTINUATION_3D, cfg) {
      cfg.s_dimension =
                                 pan_pack(bufs + (i * 2) + 1, ATTRIBUTE_BUFFER_CONTINUATION_3D, cfg) {
               cfg.s_dimension = u_minify(rsrc->base.width0, level);
   cfg.t_dimension = u_minify(rsrc->base.height0, level);
   cfg.r_dimension =
                           if (rsrc->base.target != PIPE_TEXTURE_2D) {
      cfg.slice_stride =
               }
      static mali_ptr
   panfrost_emit_image_attribs(struct panfrost_batch *batch, mali_ptr *buffers,
         {
      struct panfrost_context *ctx = batch->ctx;
            if (!shader->info.attribute_count) {
      *buffers = 0;
               /* Images always need a MALI_ATTRIBUTE_BUFFER_CONTINUATION_3D */
   unsigned attr_count = shader->info.attribute_count;
            struct panfrost_ptr bufs =
            struct panfrost_ptr attribs =
            emit_image_attribs(ctx, type, attribs.cpu, 0);
               #if PAN_ARCH >= 6
      pan_pack(bufs.cpu + ((buf_count - 1) * pan_size(ATTRIBUTE_BUFFER)),
            #endif
         *buffers = bufs.gpu;
      }
      static mali_ptr
   panfrost_emit_vertex_data(struct panfrost_batch *batch, mali_ptr *buffers)
   {
      struct panfrost_context *ctx = batch->ctx;
   struct panfrost_vertex_state *so = ctx->vertex;
   struct panfrost_compiled_shader *vs = ctx->prog[PIPE_SHADER_VERTEX];
   bool instanced = ctx->instance_count > 1;
   uint32_t image_mask = ctx->image_mask[PIPE_SHADER_VERTEX];
            /* Worst case: everything is NPOT, which is only possible if instancing
   * is enabled. Otherwise single record is gauranteed.
   * Also, we allocate more memory than what's needed here if either instancing
   * is enabled or images are present, this can be improved. */
   unsigned bufs_per_attrib = (instanced || nr_images > 0) ? 2 : 1;
   unsigned nr_bufs =
                     struct panfrost_compiled_shader *xfb =
            if (xfb)
         #if PAN_ARCH <= 5
      /* Midgard needs vertexid/instanceid handled specially */
            if (special_vbufs)
      #endif
         if (!nr_bufs) {
      *buffers = 0;
               struct panfrost_ptr S =
         struct panfrost_ptr T =
            struct mali_attribute_buffer_packed *bufs =
                     unsigned attrib_to_buffer[PIPE_MAX_ATTRIBS] = {0};
            for (unsigned i = 0; i < so->nr_bufs; ++i) {
      unsigned vbi = so->buffers[i].vbi;
   unsigned divisor = so->buffers[i].divisor;
            if (!(ctx->vb_mask & (1 << vbi)))
            struct pipe_vertex_buffer *buf = &ctx->vertex_buffers[vbi];
            rsrc = pan_resource(buf->buffer.resource);
   if (!rsrc)
                     /* Mask off lower bits, see offset fixup below */
   mali_ptr raw_addr = rsrc->image.data.bo->ptr.gpu + buf->buffer_offset;
            /* Since we advanced the base pointer, we shrink the buffer
   * size, but add the offset we subtracted */
   unsigned size =
            /* When there is a divisor, the hardware-level divisor is
   * the product of the instance divisor and the padded count */
   unsigned stride = so->strides[vbi];
            if (ctx->instance_count <= 1) {
      /* Per-instance would be every attribute equal */
                  pan_pack(bufs + k, ATTRIBUTE_BUFFER, cfg) {
      cfg.pointer = addr;
   cfg.stride = stride;
         } else if (!divisor) {
      pan_pack(bufs + k, ATTRIBUTE_BUFFER, cfg) {
      cfg.type = MALI_ATTRIBUTE_TYPE_1D_MODULUS;
   cfg.pointer = addr;
   cfg.stride = stride;
   cfg.size = size;
         } else if (util_is_power_of_two_or_zero(hw_divisor)) {
      pan_pack(bufs + k, ATTRIBUTE_BUFFER, cfg) {
      cfg.type = MALI_ATTRIBUTE_TYPE_1D_POT_DIVISOR;
   cfg.pointer = addr;
   cfg.stride = stride;
   cfg.size = size;
            } else {
                              /* Records with continuations must be aligned */
                  pan_pack(bufs + k, ATTRIBUTE_BUFFER, cfg) {
      cfg.type = MALI_ATTRIBUTE_TYPE_1D_NPOT_DIVISOR;
   cfg.pointer = addr;
                  cfg.divisor_r = shift;
               pan_pack(bufs + k + 1, ATTRIBUTE_BUFFER_CONTINUATION_NPOT, cfg) {
      cfg.divisor_numerator = magic_divisor;
                                    #if PAN_ARCH <= 5
      /* Add special gl_VertexID/gl_InstanceID buffers */
   if (special_vbufs) {
               pan_pack(out + PAN_VERTEX_ID, ATTRIBUTE, cfg) {
      cfg.buffer_index = k++;
               panfrost_instance_id(ctx->padded_count, &bufs[k],
            pan_pack(out + PAN_INSTANCE_ID, ATTRIBUTE, cfg) {
      cfg.buffer_index = k++;
            #endif
         if (nr_images) {
      k = ALIGN_POT(k, 2);
   emit_image_attribs(ctx, PIPE_SHADER_VERTEX, out + so->num_elements, k);
   emit_image_bufs(batch, PIPE_SHADER_VERTEX, bufs + k, k);
            #if PAN_ARCH >= 6
      /* We need an empty attrib buf to stop the prefetching on Bifrost */
   pan_pack(&bufs[k], ATTRIBUTE_BUFFER, cfg)
      #endif
         /* Attribute addresses require 64-byte alignment, so let:
   *
   *      base' = base & ~63 = base - (base & 63)
   *      offset' = offset + (base & 63)
   *
   * Since base' + offset' = base + offset, these are equivalent
   * addressing modes and now base is 64 aligned.
            /* While these are usually equal, they are not required to be. In some
   * cases, u_blitter passes too high a value for num_elements.
   */
            for (unsigned i = 0; i < vs->info.attributes_read_count; ++i) {
      unsigned vbi = so->pipe[i].vertex_buffer_index;
            /* BOs are aligned; just fixup for buffer_offset */
   signed src_offset = so->pipe[i].src_offset;
            /* Base instance offset */
   if (ctx->base_instance && so->pipe[i].instance_divisor) {
      src_offset += (ctx->base_instance * so->pipe[i].src_stride) /
               /* Also, somewhat obscurely per-instance data needs to be
            if (so->pipe[i].instance_divisor && ctx->instance_count > 1)
            pan_pack(out + i, ATTRIBUTE, cfg) {
      cfg.buffer_index = attrib_to_buffer[so->element_buffer[i]];
   cfg.format = so->formats[i];
                  *buffers = S.gpu;
      }
      static mali_ptr
   panfrost_emit_varyings(struct panfrost_batch *batch,
               {
      unsigned size = stride * count;
   mali_ptr ptr =
            pan_pack(slot, ATTRIBUTE_BUFFER, cfg) {
      cfg.stride = stride;
   cfg.size = size;
                  }
      /* Given a varying, figure out which index it corresponds to */
      static inline unsigned
   pan_varying_index(unsigned present, enum pan_special_varying v)
   {
         }
      /* Determines which varying buffers are required */
      static inline unsigned
   pan_varying_present(const struct panfrost_device *dev,
               {
      /* At the moment we always emit general and position buffers. Not
            unsigned present =
                     if (producer->vs.writes_point_size)
         #if PAN_ARCH <= 5
      /* On Midgard, these exist as real varyings. Later architectures use
            if (consumer->fs.reads_point_coord)
            if (consumer->fs.reads_face)
            if (consumer->fs.reads_frag_coord)
                     for (unsigned i = 0; i < consumer->varyings.input_count; i++) {
               if (util_varying_is_point_coord(loc, point_coord_mask))
         #endif
            }
      /* Emitters for varying records */
      static void
   pan_emit_vary(const struct panfrost_device *dev,
               {
      pan_pack(out, ATTRIBUTE, cfg) {
      cfg.buffer_index = buffer_index;
   cfg.offset_enable = (PAN_ARCH <= 5);
   cfg.format = format;
         }
      /* Special records */
      /* clang-format off */
   static const struct {
      unsigned components;
      } pan_varying_formats[PAN_VARY_MAX] = {
      [PAN_VARY_POSITION]  = { 4, MALI_SNAP_4   },
   [PAN_VARY_PSIZ]      = { 1, MALI_R16F     },
   [PAN_VARY_PNTCOORD]  = { 4, MALI_RGBA32F  },
   [PAN_VARY_FACE]      = { 1, MALI_R32I     },
      };
   /* clang-format on */
      static mali_pixel_format
   pan_special_format(const struct panfrost_device *dev,
         {
      assert(buf < PAN_VARY_MAX);
         #if PAN_ARCH <= 6
      unsigned nr = pan_varying_formats[buf].components;
      #endif
            }
      static void
   pan_emit_vary_special(const struct panfrost_device *dev,
               {
      pan_emit_vary(dev, out, pan_varying_index(present, buf),
      }
      /* Negative indicates a varying is not found */
      static signed
   pan_find_vary(const struct pan_shader_varying *vary, unsigned vary_count,
         {
      for (unsigned i = 0; i < vary_count; ++i) {
      if (vary[i].location == loc)
                  }
      /* Assign varying locations for the general buffer. Returns the calculated
   * per-vertex stride, and outputs offsets into the passed array. Negative
   * offset indicates a varying is not used. */
      static unsigned
   pan_assign_varyings(const struct panfrost_device *dev,
               {
      unsigned producer_count = producer->varyings.output_count;
            const struct pan_shader_varying *producer_vars = producer->varyings.output;
                     for (unsigned i = 0; i < producer_count; ++i) {
      signed loc = pan_find_vary(consumer_vars, consumer_count,
         enum pipe_format format =
            if (format != PIPE_FORMAT_NONE) {
      offsets[i] = stride;
      } else {
                        }
      /* Emitter for a single varying (attribute) descriptor */
      static void
   panfrost_emit_varying(const struct panfrost_device *dev,
                        struct mali_attribute_packed *out,
      {
      /* Note: varying.format != pipe_format in some obscure cases due to a
   * limitation of the NIR linker. This should be fixed in the future to
   * eliminate the additional lookups. See:
   * dEQP-GLES3.functional.shaders.conditionals.if.sequence_statements_vertex
   */
   gl_varying_slot loc = varying.location;
            if (util_varying_is_point_coord(loc, point_sprite_mask)) {
         } else if (loc == VARYING_SLOT_POS) {
         } else if (loc == VARYING_SLOT_PSIZ) {
         } else if (loc == VARYING_SLOT_FACE) {
         } else if (offset < 0) {
         } else {
      STATIC_ASSERT(PAN_VARY_GENERAL == 0);
         }
      /* Links varyings and uploads ATTRIBUTE descriptors. Can execute at link time,
   * rather than draw time (under good conditions). */
      static void
   panfrost_emit_varying_descs(struct panfrost_pool *pool,
                     {
      struct panfrost_device *dev = pool->base.dev;
   unsigned producer_count = producer->info.varyings.output_count;
            /* Offsets within the general varying buffer, indexed by location */
   signed offsets[PAN_MAX_VARYINGS];
   assert(producer_count <= ARRAY_SIZE(offsets));
            /* Allocate enough descriptors for both shader stages */
   struct panfrost_ptr T = pan_pool_alloc_desc_array(
            /* Take a reference if we're being put on the CSO */
   if (!pool->owned) {
      out->bo = pool->transient_bo;
               struct mali_attribute_packed *descs = T.cpu;
   out->producer = producer_count ? T.gpu : 0;
   out->consumer =
            /* Lay out the varyings. Must use producer to lay out, in order to
   * respect transform feedback precisions. */
   out->present = pan_varying_present(dev, &producer->info, &consumer->info,
            out->stride =
            for (unsigned i = 0; i < producer_count; ++i) {
      signed j = pan_find_vary(consumer->info.varyings.input,
                  enum pipe_format format = (j >= 0)
                  panfrost_emit_varying(dev, descs + i, producer->info.varyings.output[i],
                     for (unsigned i = 0; i < consumer_count; ++i) {
      signed j = pan_find_vary(producer->info.varyings.output,
                           panfrost_emit_varying(
      dev, descs + producer_count + i, consumer->info.varyings.input[i],
   consumer->info.varyings.input[i].format, out->present,
      }
      #if PAN_ARCH <= 5
   static void
   pan_emit_special_input(struct mali_attribute_buffer_packed *out,
               {
      if (present & BITFIELD_BIT(v)) {
               pan_pack(out + idx, ATTRIBUTE_BUFFER, cfg) {
      cfg.special = special;
            }
   #endif
      static void
   panfrost_emit_varying_descriptor(struct panfrost_batch *batch,
                           {
      struct panfrost_context *ctx = batch->ctx;
   struct panfrost_compiled_shader *vs = ctx->prog[PIPE_SHADER_VERTEX];
                  #if PAN_ARCH <= 5
               /* Point sprites are lowered on Bifrost and newer */
   if (point_coord_replace)
      #endif
         /* In good conditions, we only need to link varyings once */
   bool prelink =
            /* Try to reduce copies */
   struct pan_linkage _linkage;
            /* Emit ATTRIBUTE descriptors if needed */
   if (!prelink || vs->linkage.bo == NULL) {
                           unsigned present = linkage->present, stride = linkage->stride;
   unsigned count = util_bitcount(present);
   struct panfrost_ptr T =
         struct mali_attribute_buffer_packed *varyings =
            if (buffer_count)
         #if PAN_ARCH >= 6
      /* Suppress prefetch on Bifrost */
      #endif
         if (stride) {
      panfrost_emit_varyings(
      batch, &varyings[pan_varying_index(present, PAN_VARY_GENERAL)], stride,
   } else {
      /* The indirect draw code reads the stride field, make sure
   * that it is initialised */
   memset(varyings + pan_varying_index(present, PAN_VARY_GENERAL), 0,
               /* fp32 vec4 gl_Position */
   *position = panfrost_emit_varyings(
      batch, &varyings[pan_varying_index(present, PAN_VARY_POSITION)],
         if (present & BITFIELD_BIT(PAN_VARY_PSIZ)) {
      *psiz = panfrost_emit_varyings(
      batch, &varyings[pan_varying_index(present, PAN_VARY_PSIZ)], 2,
         #if PAN_ARCH <= 5
      pan_emit_special_input(
      varyings, present, PAN_VARY_PNTCOORD,
   (rast->sprite_coord_mode == PIPE_SPRITE_COORD_LOWER_LEFT)
      ? MALI_ATTRIBUTE_SPECIAL_POINT_COORD_MAX_Y
   pan_emit_special_input(varyings, present, PAN_VARY_FACE,
         pan_emit_special_input(varyings, present, PAN_VARY_FRAGCOORD,
      #endif
         *buffers = T.gpu;
   *vs_attribs = linkage->producer;
      }
      /*
   * Emit jobs required for the rasterization pipeline. If there are side effects
   * from the vertex shader, these are handled ahead-of-time with a compute
   * shader. This function should not be called if rasterization is skipped.
   */
   static void
   panfrost_emit_vertex_tiler_jobs(struct panfrost_batch *batch,
               {
      unsigned vertex = panfrost_add_job(&batch->pool.base, &batch->scoreboard,
                  panfrost_add_job(&batch->pool.base, &batch->scoreboard, MALI_JOB_TYPE_TILER,
      }
   #endif
      static void
   emit_tls(struct panfrost_batch *batch)
   {
               /* Emitted with the FB descriptor on Midgard. */
   if (PAN_ARCH <= 5 && batch->framebuffer.gpu)
            struct panfrost_bo *tls_bo =
      batch->stack_size ? panfrost_batch_get_scratchpad(
                  struct pan_tls_info tls = {
      .tls =
      {
      .ptr = tls_bo ? tls_bo->ptr.gpu : 0,
               assert(batch->tls.cpu);
      }
      static void
   emit_fbd(struct panfrost_batch *batch, const struct pan_fb_info *fb)
   {
      struct panfrost_device *dev = pan_device(batch->ctx->base.screen);
   struct panfrost_bo *tls_bo =
      batch->stack_size ? panfrost_batch_get_scratchpad(
                  struct pan_tls_info tls = {
      .tls =
      {
      .ptr = tls_bo ? tls_bo->ptr.gpu : 0,
               batch->framebuffer.gpu |= GENX(pan_emit_fbd)(
      }
      /* Mark a surface as written */
      static void
   panfrost_initialize_surface(struct panfrost_batch *batch,
         {
      if (surf) {
      struct panfrost_resource *rsrc = pan_resource(surf->texture);
         }
      /* Generate a fragment job. This should be called once per frame. (Usually,
   * this corresponds to eglSwapBuffers or one of glFlush, glFinish)
   */
   static mali_ptr
   emit_fragment_job(struct panfrost_batch *batch, const struct pan_fb_info *pfb)
   {
      /* Mark the affected buffers as initialized, since we're writing to it.
                     for (unsigned i = 0; i < fb->nr_cbufs; ++i)
                     /* The passed tile coords can be out of range in some cases, so we need
   * to clamp them to the framebuffer size to avoid a TILE_RANGE_FAULT.
   * Theoretically we also need to clamp the coordinates positive, but we
   * avoid that edge case as all four values are unsigned. Also,
   * theoretically we could clamp the minima, but if that has to happen
   * the asserts would fail anyway (since the maxima would get clamped
   * and then be smaller than the minima). An edge case of sorts occurs
   * when no scissors are added to draw, so by default min=~0 and max=0.
   * But that can't happen if any actual drawing occurs (beyond a
            batch->maxx = MIN2(batch->maxx, fb->width);
            /* Rendering region must be at least 1x1; otherwise, there is nothing
            assert(batch->maxx > batch->minx);
            struct panfrost_ptr transfer =
                        }
      #define DEFINE_CASE(c)                                                         \
      case MESA_PRIM_##c:                                                         \
         static uint8_t
   pan_draw_mode(enum mesa_prim mode)
   {
      switch (mode) {
      DEFINE_CASE(POINTS);
   DEFINE_CASE(LINES);
   DEFINE_CASE(LINE_LOOP);
   DEFINE_CASE(LINE_STRIP);
   DEFINE_CASE(TRIANGLES);
   DEFINE_CASE(TRIANGLE_STRIP);
   DEFINE_CASE(TRIANGLE_FAN);
   DEFINE_CASE(QUADS);
   #if PAN_ARCH <= 6
         #endif
         default:
            }
      #undef DEFINE_CASE
      /* Count generated primitives (when there is no geom/tess shaders) for
   * transform feedback */
      static void
   panfrost_statistics_record(struct panfrost_context *ctx,
               {
      if (!ctx->active_queries)
            uint32_t prims = u_prims_for_vertices(info->mode, draw->count);
            if (!ctx->streamout.num_targets)
            ctx->tf_prims_generated += prims;
      }
      static void
   panfrost_update_streamout_offsets(struct panfrost_context *ctx)
   {
      unsigned count =
            for (unsigned i = 0; i < ctx->streamout.num_targets; ++i) {
      if (!ctx->streamout.targets[i])
                  }
      static inline enum mali_index_type
   panfrost_translate_index_size(unsigned size)
   {
      STATIC_ASSERT(MALI_INDEX_TYPE_NONE == 0);
   STATIC_ASSERT(MALI_INDEX_TYPE_UINT8 == 1);
               }
      #if PAN_ARCH <= 7
   static inline void
   pan_emit_draw_descs(struct panfrost_batch *batch, struct MALI_DRAW *d,
         {
      d->offset_start = batch->ctx->offset_start;
   d->instance_size =
            d->uniform_buffers = batch->uniform_buffers[st];
   d->push_uniforms = batch->push_uniforms[st];
   d->textures = batch->textures[st];
      }
      static void
   panfrost_draw_emit_vertex_section(struct panfrost_batch *batch,
                     {
      pan_pack(section, DRAW, cfg) {
      cfg.state = batch->rsd[PIPE_SHADER_VERTEX];
   cfg.attributes = attribs;
   cfg.attribute_buffers = attrib_bufs;
   cfg.varyings = vs_vary;
   cfg.varying_buffers = vs_vary ? varyings : 0;
   cfg.thread_storage = batch->tls.gpu;
         }
      static void
   panfrost_draw_emit_vertex(struct panfrost_batch *batch,
                           {
      void *section = pan_section_ptr(job, COMPUTE_JOB, INVOCATION);
            pan_section_pack(job, COMPUTE_JOB, PARAMETERS, cfg) {
                  section = pan_section_ptr(job, COMPUTE_JOB, DRAW);
   panfrost_draw_emit_vertex_section(batch, vs_vary, varyings, attribs,
         #if PAN_ARCH == 4
      pan_section_pack(job, COMPUTE_JOB, COMPUTE_PADDING, cfg)
      #endif
   }
   #endif
      static void
   panfrost_emit_primitive_size(struct panfrost_context *ctx, bool points,
         {
               pan_pack(prim_size, PRIMITIVE_SIZE, cfg) {
      if (panfrost_writes_point_size(ctx)) {
         } else {
               }
      static bool
   panfrost_is_implicit_prim_restart(const struct pipe_draw_info *info)
   {
      /* As a reminder primitive_restart should always be checked before any
         return info->primitive_restart &&
      }
      /* On Bifrost and older, the Renderer State Descriptor aggregates many pieces of
   * 3D state. In particular, it groups the fragment shader descriptor with
   * depth/stencil, blend, polygon offset, and multisampling state. These pieces
   * of state are dirty tracked independently for the benefit of newer GPUs that
   * separate the descriptors. FRAGMENT_RSD_DIRTY_MASK contains the list of 3D
   * dirty flags that trigger re-emits of the fragment RSD.
   *
   * Obscurely, occlusion queries are included. Occlusion query state is nominally
   * specified in the draw call descriptor, but must be considered when determing
   * early-Z state which is part of the RSD.
   */
   #define FRAGMENT_RSD_DIRTY_MASK                                                \
      (PAN_DIRTY_ZS | PAN_DIRTY_BLEND | PAN_DIRTY_MSAA | PAN_DIRTY_RASTERIZER |   \
         static inline void
   panfrost_update_shader_state(struct panfrost_batch *batch,
         {
      struct panfrost_context *ctx = batch->ctx;
            bool frag = (st == PIPE_SHADER_FRAGMENT);
   unsigned dirty_3d = ctx->dirty;
            if (dirty & (PAN_DIRTY_STAGE_TEXTURE | PAN_DIRTY_STAGE_SHADER)) {
                  if (dirty & PAN_DIRTY_STAGE_SAMPLER) {
                  /* On Bifrost and older, the fragment shader descriptor is fused
   * together with the renderer state; the combined renderer state
   * descriptor is emitted below. Otherwise, the shader descriptor is
   * standalone and is emitted here.
   */
   if ((dirty & PAN_DIRTY_STAGE_SHADER) && !((PAN_ARCH <= 7) && frag)) {
               #if PAN_ARCH >= 9
      if (dirty & PAN_DIRTY_STAGE_IMAGE) {
      batch->images[st] =
         #endif
         if ((dirty & ss->dirty_shader) || (dirty_3d & ss->dirty_3d)) {
      batch->uniform_buffers[st] = panfrost_emit_const_buf(
      batch, st, &batch->nr_uniform_buffers[st], &batch->push_uniforms[st],
         #if PAN_ARCH <= 7
      /* On Bifrost and older, if the fragment shader changes OR any renderer
   * state specified with the fragment shader, the whole renderer state
   * descriptor is dirtied and must be reemited.
   */
   if (frag && ((dirty & PAN_DIRTY_STAGE_SHADER) ||
                           if (frag && (dirty & PAN_DIRTY_STAGE_IMAGE)) {
      batch->attribs[st] =
         #endif
   }
      static inline void
   panfrost_update_state_3d(struct panfrost_batch *batch)
   {
      struct panfrost_context *ctx = batch->ctx;
            if (dirty & PAN_DIRTY_TLS_SIZE)
            if (dirty & PAN_DIRTY_BLEND)
            if (dirty & PAN_DIRTY_ZS)
         #if PAN_ARCH >= 9
      if ((dirty & (PAN_DIRTY_ZS | PAN_DIRTY_RASTERIZER)) ||
      (ctx->dirty_shader[PIPE_SHADER_FRAGMENT] & PAN_DIRTY_STAGE_SHADER))
         if (dirty & PAN_DIRTY_BLEND)
            if (dirty & PAN_DIRTY_VERTEX) {
               batch->attrib_bufs[PIPE_SHADER_VERTEX] =
         #endif
   }
      #if PAN_ARCH >= 6
   static mali_ptr
   panfrost_batch_get_bifrost_tiler(struct panfrost_batch *batch,
         {
               if (!vertex_count)
            if (batch->tiler_ctx.bifrost)
                                       t = pan_pool_alloc_desc(&batch->pool.base, TILER_CONTEXT);
   GENX(pan_emit_tiler_ctx)
   (dev, batch->key.width, batch->key.height,
   util_framebuffer_get_num_samples(&batch->key),
            batch->tiler_ctx.bifrost = t.gpu;
      }
   #endif
      /* Packs a primitive descriptor, mostly common between Midgard/Bifrost tiler
   * jobs and Valhall IDVS jobs
   */
   static void
   panfrost_emit_primitive(struct panfrost_context *ctx,
                     {
               bool lines =
      (info->mode == MESA_PRIM_LINES || info->mode == MESA_PRIM_LINE_LOOP ||
         pan_pack(out, PRIMITIVE, cfg) {
      cfg.draw_mode = pan_draw_mode(info->mode);
   if (panfrost_writes_point_size(ctx))
      #if PAN_ARCH <= 8
         /* For line primitives, PRIMITIVE.first_provoking_vertex must
   * be set to true and the provoking vertex is selected with
   * DRAW.flat_shading_vertex.
   */
   if (lines)
         else
            if (panfrost_is_implicit_prim_restart(info)) {
         } else if (info->primitive_restart) {
      cfg.primitive_restart = MALI_PRIMITIVE_RESTART_EXPLICIT;
               #else
                  cfg.allow_rotating_primitives =
                  /* Non-fixed restart indices should have been lowered */
   #endif
            cfg.index_count = draw->count;
            if (PAN_ARCH >= 9) {
      /* Base vertex offset on Valhall is used for both
   * indexed and non-indexed draws, in a simple way for
   * either. Handle both cases.
   */
   if (cfg.index_type)
                        /* Indices are moved outside the primitive descriptor
   * on Valhall, so we don't need to set that here
      } else if (cfg.index_type) {
      #if PAN_ARCH <= 7
         #endif
            #if PAN_ARCH >= 6
         #endif
         }
      #if PAN_ARCH >= 9
   static mali_ptr
   panfrost_emit_resources(struct panfrost_batch *batch,
         {
      struct panfrost_context *ctx = batch->ctx;
   struct panfrost_ptr T;
            /* Although individual resources need only 16 byte alignment, the
   * resource table as a whole must be 64-byte aligned.
   */
   T = pan_pool_alloc_aligned(&batch->pool.base, nr_tables * pan_size(RESOURCE),
                  panfrost_make_resource_table(T, PAN_TABLE_UBO, batch->uniform_buffers[stage],
            panfrost_make_resource_table(T, PAN_TABLE_TEXTURE, batch->textures[stage],
            /* We always need at least 1 sampler for txf to work */
   panfrost_make_resource_table(T, PAN_TABLE_SAMPLER, batch->samplers[stage],
            panfrost_make_resource_table(T, PAN_TABLE_IMAGE, batch->images[stage],
            if (stage == PIPE_SHADER_VERTEX) {
      panfrost_make_resource_table(T, PAN_TABLE_ATTRIBUTE,
                  panfrost_make_resource_table(T, PAN_TABLE_ATTRIBUTE_BUFFER,
                        }
      static void
   panfrost_emit_shader(struct panfrost_batch *batch,
                     {
      cfg->resources = panfrost_emit_resources(batch, stage);
   cfg->thread_storage = thread_storage;
            /* Each entry of FAU is 64-bits */
   cfg->fau = batch->push_uniforms[stage];
      }
   #endif
      static void
   panfrost_emit_draw(void *out, struct panfrost_batch *batch, bool fs_required,
               {
      struct panfrost_context *ctx = batch->ctx;
   struct pipe_rasterizer_state *rast = &ctx->rasterizer->base;
            pan_pack(out, DRAW, cfg) {
      /*
   * From the Gallium documentation,
   * pipe_rasterizer_state::cull_face "indicates which faces of
   * polygons to cull". Points and lines are not considered
   * polygons and should be drawn even if all faces are culled.
   * The hardware does not take primitive type into account when
   * culling, so we need to do that check ourselves.
   */
   cfg.cull_front_face = polygon && (rast->cull_face & PIPE_FACE_FRONT);
   cfg.cull_back_face = polygon && (rast->cull_face & PIPE_FACE_BACK);
            if (ctx->occlusion_query && ctx->active_queries) {
      if (ctx->occlusion_query->type == PIPE_QUERY_OCCLUSION_COUNTER)
                        struct panfrost_resource *rsrc =
         cfg.occlusion = rsrc->image.data.bo->ptr.gpu;
         #if PAN_ARCH >= 9
                  cfg.multisample_enable = rast->multisample;
            /* Use per-sample shading if required by API Also use it when a
   * blend shader is used with multisampling, as this is handled
   * by a single ST_TILE in the blend shader with the current
   * sample ID, requiring per-sample shading.
   */
   cfg.evaluate_per_sample =
                                    cfg.minimum_z = batch->minimum_z;
                     if (fs_required) {
               struct pan_earlyzs_state earlyzs = pan_earlyzs_get(
      fs->earlyzs, ctx->depth_stencil->writes_zs || has_oq,
                              cfg.allow_forward_pixel_to_kill =
                  /* Mask of render targets that may be written. A render
   * target may be written if the fragment shader writes
   * to it AND it actually exists. If the render target
   * doesn't actually exist, the blend descriptor will be
   * OFF so it may be omitted from the mask.
   *
   * Only set when there is a fragment shader, since
   * otherwise no colour updates are possible.
   */
                  /* Also use per-sample shading if required by the shader
                  /* Unlike Bifrost, alpha-to-coverage must be included in
   * this identically-named flag. Confusing, isn't it?
   */
   cfg.shader_modifies_coverage = fs->info.fs.writes_coverage ||
                  /* Blend descriptors are only accessed by a BLEND
   * instruction on Valhall. It follows that if the
   * fragment shader is omitted, we may also emit the
   * blend descriptors.
   */
   cfg.blend = batch->blend;
                                 panfrost_emit_shader(batch, &cfg.shader, PIPE_SHADER_FRAGMENT,
      } else {
      /* These operations need to be FORCE to benefit from the
   * depth-only pass optimizations.
   */
                  /* No shader and no blend => no shader or blend
   * reasons to disable FPK. The only FPK-related state
   * not covered is alpha-to-coverage which we don't set
   * without blend.
                                 /* Alpha isn't written so these are vacuous */
   cfg.overdraw_alpha0 = true;
      #else
         cfg.position = pos;
   cfg.state = batch->rsd[PIPE_SHADER_FRAGMENT];
   cfg.attributes = batch->attribs[PIPE_SHADER_FRAGMENT];
   cfg.attribute_buffers = batch->attrib_bufs[PIPE_SHADER_FRAGMENT];
   cfg.viewport = batch->viewport;
   cfg.varyings = fs_vary;
   cfg.varying_buffers = fs_vary ? varyings : 0;
            /* For all primitives but lines DRAW.flat_shading_vertex must
   * be set to 0 and the provoking vertex is selected with the
   * PRIMITIVE.first_provoking_vertex field.
   */
   if (prim == MESA_PRIM_LINES) {
      /* The logic is inverted across arches. */
               #endif
         }
      #if PAN_ARCH >= 9
   static void
   panfrost_emit_malloc_vertex(struct panfrost_batch *batch,
                     {
      struct panfrost_context *ctx = batch->ctx;
   struct panfrost_compiled_shader *vs = ctx->prog[PIPE_SHADER_VERTEX];
            bool fs_required = panfrost_fs_required(
            /* Varying shaders only feed data to the fragment shader, so if we omit
   * the fragment shader, we should omit the varying shader too.
   */
            panfrost_emit_primitive(ctx, info, draw, 0, secondary_shader,
            pan_section_pack(job, MALLOC_VERTEX_JOB, INSTANCE_COUNT, cfg) {
                  pan_section_pack(job, MALLOC_VERTEX_JOB, ALLOCATION, cfg) {
      if (secondary_shader) {
      unsigned v = vs->info.varyings.output_count;
   unsigned f = fs->info.varyings.input_count;
   unsigned slots = MAX2(v, f);
                  /* Assumes 16 byte slots. We could do better. */
   cfg.vertex_packet_stride = size + 16;
      } else {
      /* Hardware requirement for "no varyings" */
   cfg.vertex_packet_stride = 16;
                  pan_section_pack(job, MALLOC_VERTEX_JOB, TILER, cfg) {
                  STATIC_ASSERT(sizeof(batch->scissor) == pan_size(SCISSOR));
   memcpy(pan_section_ptr(job, MALLOC_VERTEX_JOB, SCISSOR), &batch->scissor,
            panfrost_emit_primitive_size(
      ctx, info->mode == MESA_PRIM_POINTS, 0,
         pan_section_pack(job, MALLOC_VERTEX_JOB, INDICES, cfg) {
                  panfrost_emit_draw(pan_section_ptr(job, MALLOC_VERTEX_JOB, DRAW), batch,
            pan_section_pack(job, MALLOC_VERTEX_JOB, POSITION, cfg) {
      /* IDVS/points vertex shader */
            /* IDVS/triangle vertex shader */
   if (vs_ptr && info->mode != MESA_PRIM_POINTS)
            panfrost_emit_shader(batch, &cfg, PIPE_SHADER_VERTEX, vs_ptr,
               pan_section_pack(job, MALLOC_VERTEX_JOB, VARYING, cfg) {
      /* If a varying shader is used, we configure it with the same
   * state as the position shader for backwards compatible
   * behaviour with Bifrost. This could be optimized.
   */
   if (!secondary_shader)
            mali_ptr ptr =
            panfrost_emit_shader(batch, &cfg, PIPE_SHADER_VERTEX, ptr,
         }
   #endif
      #if PAN_ARCH <= 7
   static void
   panfrost_draw_emit_tiler(struct panfrost_batch *batch,
                           const struct pipe_draw_info *info,
   {
               void *section = pan_section_ptr(job, TILER_JOB, INVOCATION);
            panfrost_emit_primitive(ctx, info, draw, indices, secondary_shader,
            void *prim_size = pan_section_ptr(job, TILER_JOB, PRIMITIVE_SIZE);
         #if PAN_ARCH >= 6
      pan_section_pack(job, TILER_JOB, TILER, cfg) {
                  pan_section_pack(job, TILER_JOB, PADDING, cfg)
      #endif
         panfrost_emit_draw(pan_section_ptr(job, TILER_JOB, DRAW), batch, true, prim,
               }
   #endif
      static void
   panfrost_launch_xfb(struct panfrost_batch *batch,
               {
                        /* Nothing to do */
   if (batch->ctx->streamout.num_targets == 0)
            /* TODO: XFB with index buffers */
   // assert(info->index_size == 0);
            if (count == 0)
                     struct panfrost_uncompiled_shader *vs_uncompiled =
                           mali_ptr saved_rsd = batch->rsd[PIPE_SHADER_VERTEX];
   mali_ptr saved_ubo = batch->uniform_buffers[PIPE_SHADER_VERTEX];
   mali_ptr saved_push = batch->push_uniforms[PIPE_SHADER_VERTEX];
   unsigned saved_nr_push_uniforms =
            ctx->uncompiled[PIPE_SHADER_VERTEX] = NULL; /* should not be read */
   ctx->prog[PIPE_SHADER_VERTEX] = vs_uncompiled->xfb;
   batch->rsd[PIPE_SHADER_VERTEX] =
            batch->uniform_buffers[PIPE_SHADER_VERTEX] =
      panfrost_emit_const_buf(batch, PIPE_SHADER_VERTEX, NULL,
            #if PAN_ARCH >= 9
      pan_section_pack(t.cpu, COMPUTE_JOB, PAYLOAD, cfg) {
      cfg.workgroup_size_x = 1;
   cfg.workgroup_size_y = 1;
            cfg.workgroup_count_x = count;
   cfg.workgroup_count_y = info->instance_count;
            panfrost_emit_shader(batch, &cfg.compute, PIPE_SHADER_VERTEX,
            /* TODO: Indexing. Also, this is a legacy feature... */
            /* Transform feedback shaders do not use barriers or shared
   * memory, so we may merge workgroups.
   */
   cfg.allow_merging_workgroups = true;
   cfg.task_increment = 1;
         #else
               panfrost_pack_work_groups_compute(&invocation, 1, count,
                  panfrost_draw_emit_vertex(batch, info, &invocation, 0, 0, attribs,
      #endif
         #if PAN_ARCH <= 5
         #endif
      panfrost_add_job(&batch->pool.base, &batch->scoreboard, job_type, true,
            ctx->uncompiled[PIPE_SHADER_VERTEX] = vs_uncompiled;
   ctx->prog[PIPE_SHADER_VERTEX] = vs;
   batch->rsd[PIPE_SHADER_VERTEX] = saved_rsd;
   batch->uniform_buffers[PIPE_SHADER_VERTEX] = saved_ubo;
   batch->push_uniforms[PIPE_SHADER_VERTEX] = saved_push;
      }
      /*
   * Increase the vertex count on the batch using a saturating add, and hope the
   * compiler can use the machine instruction here...
   */
   static inline void
   panfrost_increase_vertex_count(struct panfrost_batch *batch, uint32_t increment)
   {
               if (sum >= batch->tiler_ctx.vertex_count)
         else
      }
      static void
   panfrost_direct_draw(struct panfrost_batch *batch,
               {
      if (!draw->count || !info->instance_count)
                     /* If we change whether we're drawing points, or whether point sprites
   * are enabled (specified in the rasterizer), we may need to rebind
   * shaders accordingly. This implicitly covers the case of rebinding
   * framebuffers, because all dirty flags are set there.
   */
   if ((ctx->dirty & PAN_DIRTY_RASTERIZER) ||
      ((ctx->active_prim == MESA_PRIM_POINTS) ^
            ctx->active_prim = info->mode;
               /* Take into account a negative bias */
   ctx->vertex_count =
         ctx->instance_count = info->instance_count;
   ctx->base_vertex = info->index_size ? draw->index_bias : 0;
   ctx->base_instance = info->start_instance;
   ctx->active_prim = info->mode;
                     bool idvs = vs->info.vs.idvs;
                        #if PAN_ARCH >= 9
         #elif PAN_ARCH >= 6
         #else
         #endif
      } else {
      vertex = pan_pool_alloc_desc(&batch->pool.base, COMPUTE_JOB);
                        unsigned min_index = 0, max_index = 0;
            if (info->index_size && PAN_ARCH >= 9) {
               /* Use index count to estimate vertex count */
      } else if (info->index_size) {
      indices = panfrost_get_index_buffer_bounded(batch, info, draw, &min_index,
            /* Use the corresponding values */
   vertex_count = max_index - min_index + 1;
   ctx->offset_start = min_index + draw->index_bias;
      } else {
      ctx->offset_start = draw->start;
               if (info->instance_count > 1) {
               /* Index-Driven Vertex Shading requires different instances to
   * have different cache lines for position results. Each vertex
   * position is 16 bytes and the Mali cache line is 64 bytes, so
   * the instance count must be aligned to 4 vertices.
   */
   if (idvs)
               } else
                  #if PAN_ARCH <= 7
      struct mali_invocation_packed invocation;
   if (info->instance_count > 1) {
      panfrost_pack_work_groups_compute(&invocation, 1, vertex_count,
            } else {
      pan_pack(&invocation, INVOCATION, cfg) {
      cfg.invocations = vertex_count - 1;
   cfg.size_y_shift = 0;
   cfg.size_z_shift = 0;
   cfg.workgroups_x_shift = 0;
   cfg.workgroups_y_shift = 0;
   cfg.workgroups_z_shift = 32;
                  /* Emit all sort of descriptors. */
            panfrost_emit_varying_descriptor(
      batch, ctx->padded_count * ctx->instance_count, &vs_vary, &fs_vary,
         mali_ptr attribs, attrib_bufs;
      #endif
         panfrost_update_state_3d(batch);
   panfrost_update_shader_state(batch, PIPE_SHADER_VERTEX);
   panfrost_update_shader_state(batch, PIPE_SHADER_FRAGMENT);
               #if PAN_ARCH >= 9
         #endif
                     /* Increment transform feedback offsets */
            /* Any side effects must be handled by the XFB shader, so we only need
   * to run vertex shaders if we need rasterization.
   */
   if (panfrost_batch_skip_rasterization(batch))
         #if PAN_ARCH >= 9
               panfrost_emit_malloc_vertex(batch, info, draw, indices, secondary_shader,
            panfrost_add_job(&batch->pool.base, &batch->scoreboard,
            #else
      /* Fire off the draw itself */
   panfrost_draw_emit_tiler(batch, info, draw, &invocation, indices, fs_vary,
            #if PAN_ARCH >= 6
         panfrost_draw_emit_vertex_section(
                  panfrost_add_job(&batch->pool.base, &batch->scoreboard,
         #endif
      } else {
      panfrost_draw_emit_vertex(batch, info, &invocation, vs_vary, varyings,
               #endif
   }
      static bool
   panfrost_compatible_batch_state(struct panfrost_batch *batch, bool points)
   {
      /* Only applies on Valhall */
   if (PAN_ARCH < 9)
            struct panfrost_context *ctx = batch->ctx;
            bool coord = (rast->sprite_coord_mode == PIPE_SPRITE_COORD_LOWER_LEFT);
            /* gl_PointCoord orientation only matters when drawing points, but
   * provoking vertex doesn't matter for points.
   */
   if (points)
         else
      }
      static void
   panfrost_draw_vbo(struct pipe_context *pipe, const struct pipe_draw_info *info,
                     unsigned drawid_offset,
   {
      struct panfrost_context *ctx = pan_context(pipe);
            if (!panfrost_render_condition_check(ctx))
                     /* Emulate indirect draws on JM */
   if (indirect && indirect->buffer) {
      assert(num_draws == 1);
   util_draw_indirect(pipe, info, indirect);
   perf_debug(dev, "Emulating indirect draw on the CPU");
               /* Do some common setup */
            /* Don't add too many jobs to a single batch. Hardware has a hard limit
   * of 65536 jobs, but we choose a smaller soft limit (arbitrary) to
   * avoid the risk of timeouts. This might not be a good idea. */
   if (unlikely(batch->scoreboard.job_index > 10000))
                     if (unlikely(!panfrost_compatible_batch_state(batch, points))) {
               ASSERTED bool succ = panfrost_compatible_batch_state(batch, points);
               /* panfrost_batch_skip_rasterization reads
   * batch->scissor_culls_everything, which is set by
   * panfrost_emit_viewport, so call that first.
   */
   if (ctx->dirty & (PAN_DIRTY_VIEWPORT | PAN_DIRTY_SCISSOR))
            /* Mark everything dirty when debugging */
   if (unlikely(dev->debug & PAN_DBG_DIRTY))
            /* Conservatively assume draw parameters always change */
            struct pipe_draw_info tmp_info = *info;
            for (unsigned i = 0; i < num_draws; i++) {
               if (tmp_info.increment_draw_id) {
      ctx->dirty |= PAN_DIRTY_DRAWID;
            }
      /* Launch grid is the compute equivalent of draw_vbo, so in this routine, we
   * construct the COMPUTE job and some of its payload.
   */
      static void
   panfrost_launch_grid_on_batch(struct pipe_context *pipe,
               {
               if (info->indirect && !PAN_GPU_INDIRECTS) {
      struct pipe_transfer *transfer;
   uint32_t *params =
                  struct pipe_grid_info direct = *info;
   direct.indirect = NULL;
   direct.grid[0] = params[0];
   direct.grid[1] = params[1];
   direct.grid[2] = params[2];
            if (params[0] && params[1] && params[2])
                                                            if (info->indirect)
            /* Conservatively assume workgroup size changes every launch */
                  #if PAN_ARCH <= 7
      panfrost_pack_work_groups_compute(
      pan_section_ptr(t.cpu, COMPUTE_JOB, INVOCATION), num_wg[0], num_wg[1],
   num_wg[2], info->block[0], info->block[1], info->block[2], false,
         pan_section_pack(t.cpu, COMPUTE_JOB, PARAMETERS, cfg) {
      cfg.job_task_split = util_logbase2_ceil(info->block[0] + 1) +
                     pan_section_pack(t.cpu, COMPUTE_JOB, DRAW, cfg) {
      cfg.state = batch->rsd[PIPE_SHADER_COMPUTE];
   cfg.attributes = panfrost_emit_image_attribs(
         cfg.thread_storage = panfrost_emit_shared_memory(batch, info);
   cfg.uniform_buffers = batch->uniform_buffers[PIPE_SHADER_COMPUTE];
   cfg.push_uniforms = batch->push_uniforms[PIPE_SHADER_COMPUTE];
   cfg.textures = batch->textures[PIPE_SHADER_COMPUTE];
            #if PAN_ARCH == 4
      pan_section_pack(t.cpu, COMPUTE_JOB, COMPUTE_PADDING, cfg)
      #endif
   #else
               pan_section_pack(t.cpu, COMPUTE_JOB, PAYLOAD, cfg) {
      cfg.workgroup_size_x = info->block[0];
   cfg.workgroup_size_y = info->block[1];
            cfg.workgroup_count_x = num_wg[0];
   cfg.workgroup_count_y = num_wg[1];
            panfrost_emit_shader(batch, &cfg.compute, PIPE_SHADER_COMPUTE,
                  /* Workgroups may be merged if the shader does not use barriers
   * or shared memory. This condition is checked against the
   * static shared_size at compile-time. We need to check the
   * variable shared size at launch_grid time, because the
   * compiler doesn't know about that.
   */
   cfg.allow_merging_workgroups = cs->info.cs.allow_merging_workgroups &&
            cfg.task_increment = 1;
         #endif
            #if PAN_GPU_INDIRECTS
      if (info->indirect) {
      struct pan_indirect_dispatch_info indirect = {
      .job = t.gpu,
   .indirect_dim = pan_resource(info->indirect)->image.data.bo->ptr.gpu +
         .num_wg_sysval =
      {
      batch->num_wg_sysval[0],
   batch->num_wg_sysval[1],
               indirect_dep = GENX(pan_indirect_dispatch_emit)(
         #endif
         panfrost_add_job(&batch->pool.base, &batch->scoreboard,
            }
      static void
   panfrost_launch_grid(struct pipe_context *pipe,
         {
               /* XXX - shouldn't be necessary with working memory barriers. Affected
   * test: KHR-GLES31.core.compute_shader.pipeline-post-xfb */
            struct panfrost_batch *batch = panfrost_get_batch_for_fbo(ctx);
               }
      #define AFBC_BLOCK_ALIGN 16
      static void
   panfrost_launch_afbc_shader(struct panfrost_batch *batch, void *cso,
               {
      struct pipe_context *pctx = &batch->ctx->base;
   void *saved_cso = NULL;
   struct pipe_constant_buffer saved_const = {};
   struct pipe_grid_info grid = {
      .block[0] = 1,
   .block[1] = 1,
   .block[2] = 1,
   .grid[0] = nr_blocks,
   .grid[1] = 1,
               struct panfrost_constant_buffer *pbuf =
         saved_cso = batch->ctx->uncompiled[PIPE_SHADER_COMPUTE];
            pctx->bind_compute_state(pctx, cso);
                     pctx->bind_compute_state(pctx, saved_cso);
      }
      #define LAUNCH_AFBC_SHADER(name, batch, rsrc, consts, nr_blocks)               \
      struct pan_afbc_shader_data *shaders =                                      \
         struct pipe_constant_buffer constant_buffer = {                             \
      .buffer_size = sizeof(consts),                                           \
      panfrost_launch_afbc_shader(batch, shaders->name##_cso, &constant_buffer,   \
         static void
   panfrost_afbc_size(struct panfrost_batch *batch, struct panfrost_resource *src,
               {
      struct pan_image_slice_layout *slice = &src->image.layout.slices[level];
   struct panfrost_afbc_size_info consts = {
      .src =
                     panfrost_batch_read_rsrc(batch, src, PIPE_SHADER_COMPUTE);
               }
      static void
   panfrost_afbc_pack(struct panfrost_batch *batch, struct panfrost_resource *src,
                     struct panfrost_bo *dst,
   {
      struct pan_image_slice_layout *src_slice = &src->image.layout.slices[level];
   struct panfrost_afbc_pack_info consts = {
      .src = src->image.data.bo->ptr.gpu + src->image.data.offset +
         .dst = dst->ptr.gpu + dst_slice->offset,
   .metadata = metadata->ptr.gpu + metadata_offset,
   .header_size = dst_slice->afbc.header_size,
   .src_stride = src_slice->afbc.stride,
               panfrost_batch_write_rsrc(batch, src, PIPE_SHADER_COMPUTE);
   panfrost_batch_write_bo(batch, dst, PIPE_SHADER_COMPUTE);
               }
      static void *
   panfrost_create_rasterizer_state(struct pipe_context *pctx,
         {
                     #if PAN_ARCH <= 7
      pan_pack(&so->multisample, MULTISAMPLE_MISC, cfg) {
      cfg.multisample_enable = cso->multisample;
   cfg.fixed_function_near_discard = cso->depth_clip_near;
   cfg.fixed_function_far_discard = cso->depth_clip_far;
               pan_pack(&so->stencil_misc, STENCIL_MASK_MISC, cfg) {
      cfg.front_facing_depth_bias = cso->offset_tri;
   cfg.back_facing_depth_bias = cso->offset_tri;
         #endif
            }
      #if PAN_ARCH >= 9
   /*
   * Given a pipe_vertex_element, pack the corresponding Valhall attribute
   * descriptor. This function is called at CSO create time.
   */
   static void
   panfrost_pack_attribute(struct panfrost_device *dev,
               {
      pan_pack(out, ATTRIBUTE, cfg) {
      cfg.table = PAN_TABLE_ATTRIBUTE_BUFFER;
   cfg.frequency = (el.instance_divisor > 0)
               cfg.format = dev->formats[el.src_format].hw;
   cfg.offset = el.src_offset;
   cfg.buffer_index = el.vertex_buffer_index;
            if (el.instance_divisor == 0) {
      /* Per-vertex */
   cfg.attribute_type = MALI_ATTRIBUTE_TYPE_1D;
   cfg.frequency = MALI_ATTRIBUTE_FREQUENCY_VERTEX;
      } else if (util_is_power_of_two_or_zero(el.instance_divisor)) {
      /* Per-instance, POT divisor */
   cfg.attribute_type = MALI_ATTRIBUTE_TYPE_1D_POT_DIVISOR;
   cfg.frequency = MALI_ATTRIBUTE_FREQUENCY_INSTANCE;
      } else {
      /* Per-instance, NPOT divisor */
                  cfg.divisor_d = panfrost_compute_magic_divisor(
            }
   #endif
      static void *
   panfrost_create_vertex_elements_state(struct pipe_context *pctx,
               {
      struct panfrost_vertex_state *so = CALLOC_STRUCT(panfrost_vertex_state);
            so->num_elements = num_elements;
            for (unsigned i = 0; i < num_elements; ++i)
      #if PAN_ARCH >= 9
      for (unsigned i = 0; i < num_elements; ++i)
      #else
      /* Assign attribute buffers corresponding to the vertex buffers, keyed
   * for a particular divisor since that's how instancing works on Mali */
   for (unsigned i = 0; i < num_elements; ++i) {
      so->element_buffer[i] = pan_assign_vertex_buffer(
      so->buffers, &so->nr_bufs, elements[i].vertex_buffer_index,
            for (int i = 0; i < num_elements; ++i) {
      enum pipe_format fmt = elements[i].src_format;
                        /* Let's also prepare vertex builtins */
   so->formats[PAN_VERTEX_ID] = dev->formats[PIPE_FORMAT_R32_UINT].hw;
      #endif
            }
      static inline unsigned
   pan_pipe_to_stencil_op(enum pipe_stencil_op in)
   {
      switch (in) {
   case PIPE_STENCIL_OP_KEEP:
         case PIPE_STENCIL_OP_ZERO:
         case PIPE_STENCIL_OP_REPLACE:
         case PIPE_STENCIL_OP_INCR:
         case PIPE_STENCIL_OP_DECR:
         case PIPE_STENCIL_OP_INCR_WRAP:
         case PIPE_STENCIL_OP_DECR_WRAP:
         case PIPE_STENCIL_OP_INVERT:
         default:
            }
      #if PAN_ARCH <= 7
   static inline void
   pan_pipe_to_stencil(const struct pipe_stencil_state *in,
         {
      pan_pack(out, STENCIL, s) {
      s.mask = in->valuemask;
   s.compare_function = (enum mali_func)in->func;
   s.stencil_fail = pan_pipe_to_stencil_op(in->fail_op);
   s.depth_fail = pan_pipe_to_stencil_op(in->zfail_op);
         }
   #endif
      static bool
   pipe_zs_always_passes(const struct pipe_depth_stencil_alpha_state *zsa)
   {
      if (zsa->depth_enabled && zsa->depth_func != PIPE_FUNC_ALWAYS)
            if (zsa->stencil[0].enabled && zsa->stencil[0].func != PIPE_FUNC_ALWAYS)
            if (zsa->stencil[1].enabled && zsa->stencil[1].func != PIPE_FUNC_ALWAYS)
               }
      static void *
   panfrost_create_depth_stencil_state(
         {
      struct panfrost_zsa_state *so = CALLOC_STRUCT(panfrost_zsa_state);
            const struct pipe_stencil_state front = zsa->stencil[0];
   const struct pipe_stencil_state back =
            enum mali_func depth_func =
            /* Normalize (there's no separate enable) */
   if (PAN_ARCH <= 5 && !zsa->alpha_enabled)
         #if PAN_ARCH <= 7
      /* Prepack relevant parts of the Renderer State Descriptor. They will
   * be ORed in at draw-time */
   pan_pack(&so->rsd_depth, MULTISAMPLE_MISC, cfg) {
      cfg.depth_function = depth_func;
               pan_pack(&so->rsd_stencil, STENCIL_MASK_MISC, cfg) {
      cfg.stencil_enable = front.enabled;
   cfg.stencil_mask_front = front.writemask;
      #if PAN_ARCH <= 5
         #endif
               /* Stencil tests have their own words in the RSD */
   pan_pipe_to_stencil(&front, &so->stencil_front);
      #else
      pan_pack(&so->desc, DEPTH_STENCIL, cfg) {
      cfg.front_compare_function = (enum mali_func)front.func;
   cfg.front_stencil_fail = pan_pipe_to_stencil_op(front.fail_op);
   cfg.front_depth_fail = pan_pipe_to_stencil_op(front.zfail_op);
            cfg.back_compare_function = (enum mali_func)back.func;
   cfg.back_stencil_fail = pan_pipe_to_stencil_op(back.fail_op);
   cfg.back_depth_fail = pan_pipe_to_stencil_op(back.zfail_op);
            cfg.stencil_test_enable = front.enabled;
   cfg.front_write_mask = front.writemask;
   cfg.back_write_mask = back.writemask;
   cfg.front_value_mask = front.valuemask;
            cfg.depth_write_enable = zsa->depth_writemask;
         #endif
         so->enabled = zsa->stencil[0].enabled ||
            so->zs_always_passes = pipe_zs_always_passes(zsa);
            /* TODO: Bounds test should be easy */
               }
      static struct pipe_sampler_view *
   panfrost_create_sampler_view(struct pipe_context *pctx,
               {
      struct panfrost_context *ctx = pan_context(pctx);
   struct panfrost_sampler_view *so =
            pan_legalize_afbc_format(ctx, pan_resource(texture), template->format, false,
                     so->base = *template;
   so->base.texture = texture;
   so->base.reference.count = 1;
                        }
      /* A given Gallium blend state can be encoded to the hardware in numerous,
   * dramatically divergent ways due to the interactions of blending with
   * framebuffer formats. Conceptually, there are two modes:
   *
   * - Fixed-function blending (for suitable framebuffer formats, suitable blend
   *   state, and suitable blend constant)
   *
   * - Blend shaders (for everything else)
   *
   * A given Gallium blend configuration will compile to exactly one
   * fixed-function blend state, if it compiles to any, although the constant
   * will vary across runs as that is tracked outside of the Gallium CSO.
   *
   * However, that same blend configuration will compile to many different blend
   * shaders, depending on the framebuffer formats active. The rationale is that
   * blend shaders override not just fixed-function blending but also
   * fixed-function format conversion, so blend shaders are keyed to a particular
   * framebuffer format. As an example, the tilebuffer format is identical for
   * RG16F and RG16UI -- both are simply 32-bit raw pixels -- so both require
   * blend shaders.
   *
   * All of this state is encapsulated in the panfrost_blend_state struct
   * (our subclass of pipe_blend_state).
   */
      /* Create a blend CSO. Essentially, try to compile a fixed-function
   * expression and initialize blend shaders */
      static void *
   panfrost_create_blend_state(struct pipe_context *pipe,
         {
      struct panfrost_blend_state *so = CALLOC_STRUCT(panfrost_blend_state);
            so->pan.logicop_enable = blend->logicop_enable;
   so->pan.logicop_func = blend->logicop_func;
            for (unsigned c = 0; c < so->pan.rt_count; ++c) {
      unsigned g = blend->independent_blend_enable ? c : 0;
   const struct pipe_rt_blend_state pipe = blend->rt[g];
            equation.color_mask = pipe.colormask;
            if (pipe.blend_enable) {
      equation.rgb_func = pipe.rgb_func;
   equation.rgb_src_factor = pipe.rgb_src_factor;
   equation.rgb_dst_factor = pipe.rgb_dst_factor;
   equation.alpha_func = pipe.alpha_func;
   equation.alpha_src_factor = pipe.alpha_src_factor;
               /* Determine some common properties */
   unsigned constant_mask = pan_blend_constant_mask(equation);
   const bool supports_2src = pan_blend_supports_2src(PAN_ARCH);
   so->info[c] = (struct pan_blend_info){
      .enabled = (equation.color_mask != 0) &&
                                             /* Could this possibly be fixed-function? */
   .fixed_function =
      !blend->logicop_enable &&
               .alpha_zero_nop = pan_blend_alpha_zero_nop(equation),
                        /* Bifrost needs to know if any render target loads its
   * destination in the hot draw path, so precompute this */
   if (so->info[c].load_dest)
            /* Bifrost needs to know if any render target loads its
   * destination in the hot draw path, so precompute this */
   if (so->info[c].enabled)
            /* Converting equations to Mali style is expensive, do it at
   * CSO create time instead of draw-time */
   if (so->info[c].fixed_function) {
                        }
      #if PAN_ARCH >= 9
   static enum mali_flush_to_zero_mode
   panfrost_ftz_mode(struct pan_shader_info *info)
   {
      if (info->ftz_fp32) {
      if (info->ftz_fp16)
         else
      } else {
      /* We don't have a "flush FP16, preserve FP32" mode, but APIs
   * should not be able to generate that.
   */
   assert(!info->ftz_fp16 && !info->ftz_fp32);
         }
   #endif
      static void
   prepare_shader(struct panfrost_compiled_shader *state,
         {
   #if PAN_ARCH <= 7
               if (upload) {
      struct panfrost_ptr ptr =
            state->state = panfrost_pool_take_ref(pool, ptr.gpu);
               pan_pack(out, RENDERER_STATE, cfg) {
            #else
               /* The address in the shader program descriptor must be non-null, but
   * the entire shader program descriptor may be omitted.
   *
   * See dEQP-GLES31.functional.compute.basic.empty
   */
   if (!state->bin.gpu)
            bool vs = (state->info.stage == MESA_SHADER_VERTEX);
            unsigned nr_variants = secondary_enable ? 3 : vs ? 2 : 1;
   struct panfrost_ptr ptr =
                     /* Generic, or IDVS/points */
   pan_pack(ptr.cpu, SHADER_PROGRAM, cfg) {
      cfg.stage = pan_shader_stage(&state->info);
   cfg.primary_shader = true;
   cfg.register_allocation =
         cfg.binary = state->bin.gpu;
   cfg.preload.r48_r63 = (state->info.preload >> 48);
            if (cfg.stage == MALI_SHADER_STAGE_FRAGMENT)
               if (!vs)
            /* IDVS/triangles */
   pan_pack(ptr.cpu + pan_size(SHADER_PROGRAM), SHADER_PROGRAM, cfg) {
      cfg.stage = pan_shader_stage(&state->info);
   cfg.primary_shader = true;
   cfg.register_allocation =
         cfg.binary = state->bin.gpu + state->info.vs.no_psiz_offset;
   cfg.preload.r48_r63 = (state->info.preload >> 48);
               if (!secondary_enable)
            pan_pack(ptr.cpu + (pan_size(SHADER_PROGRAM) * 2), SHADER_PROGRAM, cfg) {
               cfg.stage = pan_shader_stage(&state->info);
   cfg.primary_shader = false;
   cfg.register_allocation = pan_register_allocation(work_count);
   cfg.binary = state->bin.gpu + state->info.vs.secondary_offset;
   cfg.preload.r48_r63 = (state->info.vs.secondary_preload >> 48);
         #endif
   }
      static void
   screen_destroy(struct pipe_screen *pscreen)
   {
      struct panfrost_device *dev = pan_device(pscreen);
      #if PAN_GPU_INDIRECTS
         #endif
   }
      static void
   preload(struct panfrost_batch *batch, struct pan_fb_info *fb)
   {
      GENX(pan_preload_fb)
   (&batch->pool.base, &batch->scoreboard, fb, batch->tls.gpu,
      }
      static void
   init_batch(struct panfrost_batch *batch)
   {
      /* Reserve the framebuffer and local storage descriptors */
      #if PAN_ARCH == 4
         #else
         pan_pool_alloc_desc_aggregate(
         #endif
      #if PAN_ARCH >= 6
         #else
      /* On Midgard, the TLS is embedded in the FB descriptor */
         #if PAN_ARCH == 5
               pan_pack(ptr.opaque, FRAMEBUFFER_POINTER, cfg) {
      cfg.pointer = batch->framebuffer.gpu;
                  #endif
   #endif
   }
      static void
   panfrost_sampler_view_destroy(struct pipe_context *pctx,
         {
               pipe_resource_reference(&pview->texture, NULL);
   panfrost_bo_unreference(view->state.bo);
      }
      static void
   context_init(struct pipe_context *pipe)
   {
      pipe->draw_vbo = panfrost_draw_vbo;
            pipe->create_vertex_elements_state = panfrost_create_vertex_elements_state;
   pipe->create_rasterizer_state = panfrost_create_rasterizer_state;
   pipe->create_depth_stencil_alpha_state = panfrost_create_depth_stencil_state;
   pipe->create_sampler_view = panfrost_create_sampler_view;
   pipe->sampler_view_destroy = panfrost_sampler_view_destroy;
   pipe->create_sampler_state = panfrost_create_sampler_state;
               }
      #if PAN_ARCH <= 5
      /* Returns the polygon list's GPU address if available, or otherwise allocates
   * the polygon list.  It's perfectly fast to use allocate/free BO directly,
   * since we'll hit the BO cache and this is one-per-batch anyway. */
      static mali_ptr
   batch_get_polygon_list(struct panfrost_batch *batch)
   {
               if (!batch->tiler_ctx.midgard.polygon_list) {
      bool has_draws = batch->scoreboard.first_tiler != NULL;
   unsigned size = panfrost_tiler_get_polygon_list_size(
                  /* Create the BO as invisible if we can. If there are no draws,
   * we need to write the polygon list manually because there's
   * no WRITE_VALUE job in the chain
   */
   bool init_polygon_list = !has_draws;
   batch->tiler_ctx.midgard.polygon_list = panfrost_batch_create_bo(
      batch, size, init_polygon_list ? 0 : PAN_BO_INVISIBLE,
      panfrost_batch_add_bo(batch, batch->tiler_ctx.midgard.polygon_list,
            if (init_polygon_list && dev->model->quirks.no_hierarchical_tiling) {
      assert(batch->tiler_ctx.midgard.polygon_list->ptr.cpu);
   uint32_t *polygon_list_body =
                  /* Magic for Mali T720 */
      } else if (init_polygon_list) {
      assert(batch->tiler_ctx.midgard.polygon_list->ptr.cpu);
   uint32_t *header = batch->tiler_ctx.midgard.polygon_list->ptr.cpu;
                              }
   #endif
      static void
   init_polygon_list(struct panfrost_batch *batch)
   {
   #if PAN_ARCH <= 5
      mali_ptr polygon_list = batch_get_polygon_list(batch);
   panfrost_scoreboard_initialize_tiler(&batch->pool.base, &batch->scoreboard,
      #endif
   }
      void
   GENX(panfrost_cmdstream_screen_init)(struct panfrost_screen *screen)
   {
               screen->vtbl.prepare_shader = prepare_shader;
   screen->vtbl.emit_tls = emit_tls;
   screen->vtbl.emit_fbd = emit_fbd;
   screen->vtbl.emit_fragment_job = emit_fragment_job;
   screen->vtbl.screen_destroy = screen_destroy;
   screen->vtbl.preload = preload;
   screen->vtbl.context_init = context_init;
   screen->vtbl.init_batch = init_batch;
   screen->vtbl.get_blend_shader = GENX(pan_blend_get_shader_locked);
   screen->vtbl.init_polygon_list = init_polygon_list;
   screen->vtbl.get_compiler_options = GENX(pan_shader_get_compiler_options);
   screen->vtbl.compile_shader = GENX(pan_shader_compile);
   screen->vtbl.afbc_size = panfrost_afbc_size;
            GENX(pan_blitter_init)
      }
