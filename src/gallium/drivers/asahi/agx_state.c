   /*
   * Copyright 2021 Alyssa Rosenzweig
   * Copyright 2019-2020 Collabora, Ltd.
   * Copyright 2014-2017 Broadcom
   * Copyright 2010 Red Hat Inc.
   * SPDX-License-Identifier: MIT
   */
   #include "agx_state.h"
   #include <errno.h>
   #include <stdio.h>
   #include "asahi/compiler/agx_compile.h"
   #include "asahi/layout/layout.h"
   #include "asahi/lib/agx_formats.h"
   #include "asahi/lib/agx_helpers.h"
   #include "asahi/lib/agx_pack.h"
   #include "asahi/lib/agx_ppp.h"
   #include "asahi/lib/agx_usc.h"
   #include "compiler/nir/nir.h"
   #include "compiler/nir/nir_serialize.h"
   #include "compiler/shader_enums.h"
   #include "gallium/auxiliary/nir/tgsi_to_nir.h"
   #include "gallium/auxiliary/tgsi/tgsi_from_mesa.h"
   #include "gallium/auxiliary/util/u_blend.h"
   #include "gallium/auxiliary/util/u_draw.h"
   #include "gallium/auxiliary/util/u_framebuffer.h"
   #include "gallium/auxiliary/util/u_helpers.h"
   #include "gallium/auxiliary/util/u_prim_restart.h"
   #include "gallium/auxiliary/util/u_viewport.h"
   #include "pipe/p_context.h"
   #include "pipe/p_defines.h"
   #include "pipe/p_screen.h"
   #include "pipe/p_state.h"
   #include "util/blob.h"
   #include "util/compiler.h"
   #include "util/format/u_format.h"
   #include "util/format_srgb.h"
   #include "util/half_float.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "util/u_prim.h"
   #include "util/u_resource.h"
   #include "util/u_transfer.h"
   #include "util/u_upload_mgr.h"
   #include "agx_device.h"
   #include "agx_disk_cache.h"
   #include "agx_tilebuffer.h"
   #include "pool.h"
      void
   agx_legalize_compression(struct agx_context *ctx, struct agx_resource *rsrc,
         {
      /* If the resource isn't compressed, we can reinterpret */
   if (rsrc->layout.tiling != AIL_TILING_TWIDDLED_COMPRESSED)
            /* Normalize due to Gallium shenanigans */
   if (format == PIPE_FORMAT_Z24_UNORM_S8_UINT ||
      format == PIPE_FORMAT_Z24X8_UNORM)
         /* The physical format */
            /* sRGB vs linear are always compatible */
   storage = util_format_linear(storage);
            /* If no reinterpretation happens, we don't have to decompress */
   if (storage == format)
            /* Otherwise, decompress. TODO: Reverse-engineer which formats are compatible
   * and don't need decompression. There are some vague hints in the Metal
   * documentation:
   *
   * https://developer.apple.com/documentation/metal/mtltextureusage/mtltextureusagepixelformatview?language=objc
   */
      }
      static void
   agx_set_shader_images(struct pipe_context *pctx, enum pipe_shader_type shader,
                     {
      struct agx_context *ctx = agx_context(pctx);
            /* Unbind start_slot...start_slot+count */
   if (!iviews) {
      for (int i = start_slot;
      i < start_slot + count + unbind_num_trailing_slots; i++) {
               ctx->stage[shader].image_mask &= ~(((1ull << count) - 1) << start_slot);
               /* Bind start_slot...start_slot+count */
   for (int i = 0; i < count; i++) {
               if (image->resource)
         else
            if (!image->resource) {
      util_copy_image_view(&ctx->stage[shader].images[start_slot + i], NULL);
               /* Images writeable with pixel granularity are incompatible with
   * compression. Decompress if necessary.
   */
   struct agx_resource *rsrc = agx_resource(image->resource);
   if (!rsrc->layout.writeable_image &&
                           /* Readable images may be compressed but are still subject to format
   * reinterpretation rules.
   */
            if (image->shader_access & PIPE_IMAGE_ACCESS_WRITE)
            /* FIXME: Decompress here once we have texture compression */
               /* Unbind start_slot+count...start_slot+count+unbind_num_trailing_slots */
   for (int i = 0; i < unbind_num_trailing_slots; i++) {
      ctx->stage[shader].image_mask &= ~BITFIELD_BIT(start_slot + count + i);
   util_copy_image_view(&ctx->stage[shader].images[start_slot + count + i],
         }
      static void
   agx_set_shader_buffers(struct pipe_context *pctx, enum pipe_shader_type shader,
                     {
               util_set_shader_buffers_mask(ctx->stage[shader].ssbo,
                     }
      static void
   agx_set_blend_color(struct pipe_context *pctx,
         {
               if (state)
               }
      static void *
   agx_create_blend_state(struct pipe_context *ctx,
         {
               so->alpha_to_coverage = state->alpha_to_coverage;
            if (state->logicop_enable) {
      so->logicop_enable = true;
               for (unsigned i = 0; i < PIPE_MAX_COLOR_BUFS; ++i) {
      unsigned rti = state->independent_blend_enable ? i : 0;
            if (state->logicop_enable) {
         } else if (!rt.blend_enable) {
      static const nir_lower_blend_channel replace = {
      .func = PIPE_BLEND_ADD,
   .src_factor = PIPE_BLENDFACTOR_ONE,
               so->rt[i].rgb = replace;
      } else {
      so->rt[i].rgb.func = rt.rgb_func;
                  so->rt[i].alpha.func = rt.alpha_func;
                                       if (rt.colormask)
                  }
      static void
   agx_bind_blend_state(struct pipe_context *pctx, void *cso)
   {
      struct agx_context *ctx = agx_context(pctx);
   ctx->blend = cso;
      }
      static const enum agx_stencil_op agx_stencil_ops[PIPE_STENCIL_OP_INVERT + 1] = {
      [PIPE_STENCIL_OP_KEEP] = AGX_STENCIL_OP_KEEP,
   [PIPE_STENCIL_OP_ZERO] = AGX_STENCIL_OP_ZERO,
   [PIPE_STENCIL_OP_REPLACE] = AGX_STENCIL_OP_REPLACE,
   [PIPE_STENCIL_OP_INCR] = AGX_STENCIL_OP_INCR_SAT,
   [PIPE_STENCIL_OP_DECR] = AGX_STENCIL_OP_DECR_SAT,
   [PIPE_STENCIL_OP_INCR_WRAP] = AGX_STENCIL_OP_INCR_WRAP,
   [PIPE_STENCIL_OP_DECR_WRAP] = AGX_STENCIL_OP_DECR_WRAP,
      };
      static void
   agx_pack_stencil(struct agx_fragment_stencil_packed *out,
         {
      if (st.enabled) {
      agx_pack(out, FRAGMENT_STENCIL, cfg) {
      cfg.compare = (enum agx_zs_func)st.func;
                  cfg.depth_pass = agx_stencil_ops[st.zpass_op];
   cfg.depth_fail = agx_stencil_ops[st.zfail_op];
         } else {
      agx_pack(out, FRAGMENT_STENCIL, cfg) {
      cfg.compare = AGX_ZS_FUNC_ALWAYS;
                  cfg.depth_pass = AGX_STENCIL_OP_KEEP;
   cfg.depth_fail = AGX_STENCIL_OP_KEEP;
            }
      static void *
   agx_create_zsa_state(struct pipe_context *ctx,
         {
      struct agx_zsa *so = CALLOC_STRUCT(agx_zsa);
                     /* Handle the enable flag */
   enum pipe_compare_func depth_func =
            /* Z func can otherwise be used as-is */
   STATIC_ASSERT((enum agx_zs_func)PIPE_FUNC_NEVER == AGX_ZS_FUNC_NEVER);
   STATIC_ASSERT((enum agx_zs_func)PIPE_FUNC_LESS == AGX_ZS_FUNC_LESS);
   STATIC_ASSERT((enum agx_zs_func)PIPE_FUNC_EQUAL == AGX_ZS_FUNC_EQUAL);
   STATIC_ASSERT((enum agx_zs_func)PIPE_FUNC_LEQUAL == AGX_ZS_FUNC_LEQUAL);
   STATIC_ASSERT((enum agx_zs_func)PIPE_FUNC_GREATER == AGX_ZS_FUNC_GREATER);
   STATIC_ASSERT((enum agx_zs_func)PIPE_FUNC_NOTEQUAL == AGX_ZS_FUNC_NOT_EQUAL);
   STATIC_ASSERT((enum agx_zs_func)PIPE_FUNC_GEQUAL == AGX_ZS_FUNC_GEQUAL);
            agx_pack(&so->depth, FRAGMENT_FACE, cfg) {
      cfg.depth_function = (enum agx_zs_func)depth_func;
                        if (state->stencil[1].enabled) {
         } else {
      /* One sided stencil */
               if (depth_func != PIPE_FUNC_NEVER && depth_func != PIPE_FUNC_ALWAYS)
            if (state->depth_writemask) {
      so->load |= PIPE_CLEAR_DEPTH;
               if (state->stencil[0].enabled) {
      so->load |= PIPE_CLEAR_STENCIL; /* TODO: Optimize */
                  }
      static void
   agx_bind_zsa_state(struct pipe_context *pctx, void *cso)
   {
      struct agx_context *ctx = agx_context(pctx);
   ctx->zs = cso;
      }
      static enum agx_polygon_mode
   agx_translate_polygon_mode(unsigned mode)
   {
      switch (mode) {
   case PIPE_POLYGON_MODE_FILL:
         case PIPE_POLYGON_MODE_POINT:
         case PIPE_POLYGON_MODE_LINE:
         default:
            }
      static void *
   agx_create_rs_state(struct pipe_context *ctx,
         {
      struct agx_rasterizer *so = CALLOC_STRUCT(agx_rasterizer);
            agx_pack(so->cull, CULL, cfg) {
      cfg.cull_front = cso->cull_face & PIPE_FACE_FRONT;
   cfg.cull_back = cso->cull_face & PIPE_FACE_BACK;
   cfg.front_face_ccw = cso->front_ccw;
   cfg.depth_clip = cso->depth_clip_near;
   cfg.depth_clamp = !cso->depth_clip_near;
   cfg.flat_shading_vertex =
                     /* Two-sided polygon mode doesn't seem to work on G13. Apple's OpenGL
   * implementation lowers to multiple draws with culling. Warn.
   */
   if (unlikely(cso->fill_front != cso->fill_back)) {
      agx_msg("Warning: Two-sided fill modes are unsupported, "
               so->polygon_mode = agx_translate_polygon_mode(cso->fill_front);
               }
      static void
   agx_bind_rasterizer_state(struct pipe_context *pctx, void *cso)
   {
      struct agx_context *ctx = agx_context(pctx);
                     /* Check if scissor or depth bias state has changed, since scissor/depth bias
   * enable is part of the rasterizer state but everything else needed for
   * scissors and depth bias is part of the scissor/depth bias arrays */
   bool scissor_zbias_changed =
      base_cso_changed || (ctx->rast->base.scissor != so->base.scissor) ||
                  if (scissor_zbias_changed)
            if (base_cso_changed ||
      (ctx->rast->base.sprite_coord_mode != so->base.sprite_coord_mode))
            }
      static enum agx_wrap
   agx_wrap_from_pipe(enum pipe_tex_wrap in)
   {
      switch (in) {
   case PIPE_TEX_WRAP_REPEAT:
         case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
         case PIPE_TEX_WRAP_MIRROR_REPEAT:
         case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
         case PIPE_TEX_WRAP_CLAMP:
         case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
         default:
            }
      static enum agx_mip_filter
   agx_mip_filter_from_pipe(enum pipe_tex_mipfilter in)
   {
      switch (in) {
   case PIPE_TEX_MIPFILTER_NEAREST:
         case PIPE_TEX_MIPFILTER_LINEAR:
         case PIPE_TEX_MIPFILTER_NONE:
                     }
      static const enum agx_compare_func agx_compare_funcs[PIPE_FUNC_ALWAYS + 1] = {
      [PIPE_FUNC_NEVER] = AGX_COMPARE_FUNC_NEVER,
   [PIPE_FUNC_LESS] = AGX_COMPARE_FUNC_LESS,
   [PIPE_FUNC_EQUAL] = AGX_COMPARE_FUNC_EQUAL,
   [PIPE_FUNC_LEQUAL] = AGX_COMPARE_FUNC_LEQUAL,
   [PIPE_FUNC_GREATER] = AGX_COMPARE_FUNC_GREATER,
   [PIPE_FUNC_NOTEQUAL] = AGX_COMPARE_FUNC_NOT_EQUAL,
   [PIPE_FUNC_GEQUAL] = AGX_COMPARE_FUNC_GEQUAL,
      };
      static enum pipe_format
   fixup_border_zs(enum pipe_format orig, union pipe_color_union *c)
   {
      switch (orig) {
   case PIPE_FORMAT_Z24_UNORM_S8_UINT:
   case PIPE_FORMAT_Z24X8_UNORM:
      /* Z24 is internally promoted to Z32F via transfer_helper. These formats
   * are normalized so should get clamped, but Z32F does not get clamped, so
   * we clamp here.
   */
   c->f[0] = SATURATE(c->f[0]);
         case PIPE_FORMAT_X24S8_UINT:
   case PIPE_FORMAT_X32_S8X24_UINT:
      /* Separate stencil is internally promoted */
         default:
            }
      static void *
   agx_create_sampler_state(struct pipe_context *pctx,
         {
      struct agx_sampler_state *so = CALLOC_STRUCT(agx_sampler_state);
            /* We report a max texture LOD bias of 16, so clamp appropriately */
   float lod_bias = CLAMP(state->lod_bias, -16.0, 16.0);
            agx_pack(&so->desc, SAMPLER, cfg) {
      cfg.minimum_lod = state->min_lod;
   cfg.maximum_lod = state->max_lod;
   cfg.maximum_anisotropy =
         cfg.magnify_linear = (state->mag_img_filter == PIPE_TEX_FILTER_LINEAR);
   cfg.minify_linear = (state->min_img_filter == PIPE_TEX_FILTER_LINEAR);
   cfg.mip_filter = agx_mip_filter_from_pipe(state->min_mip_filter);
   cfg.wrap_s = agx_wrap_from_pipe(state->wrap_s);
   cfg.wrap_t = agx_wrap_from_pipe(state->wrap_t);
   cfg.wrap_r = agx_wrap_from_pipe(state->wrap_r);
   cfg.pixel_coordinates = state->unnormalized_coords;
   cfg.compare_func = agx_compare_funcs[state->compare_func];
   cfg.compare_enable = state->compare_mode == PIPE_TEX_COMPARE_R_TO_TEXTURE;
            if (state->border_color_format != PIPE_FORMAT_NONE) {
      /* TODO: Optimize to use compact descriptors for black/white borders */
   so->uses_custom_border = true;
                  if (so->uses_custom_border) {
      union pipe_color_union border = state->border_color;
   enum pipe_format format =
                           }
      static void
   agx_delete_sampler_state(struct pipe_context *ctx, void *state)
   {
      struct agx_sampler_state *so = state;
      }
      static void
   agx_bind_sampler_states(struct pipe_context *pctx, enum pipe_shader_type shader,
         {
                        for (unsigned i = 0; i < count; i++) {
      unsigned p = start + i;
   ctx->stage[shader].samplers[p] = states ? states[i] : NULL;
   if (ctx->stage[shader].samplers[p])
         else
               ctx->stage[shader].sampler_count =
            /* Recalculate whether we need custom borders */
            u_foreach_bit(i, ctx->stage[shader].valid_samplers) {
      if (ctx->stage[shader].samplers[i]->uses_custom_border)
         }
      static enum agx_texture_dimension
   agx_translate_tex_dim(enum pipe_texture_target dim, unsigned samples)
   {
               switch (dim) {
   case PIPE_BUFFER:
   case PIPE_TEXTURE_1D:
      /* Lowered to 2D */
   assert(samples == 1);
         case PIPE_TEXTURE_RECT:
   case PIPE_TEXTURE_2D:
      return samples > 1 ? AGX_TEXTURE_DIMENSION_2D_MULTISAMPLED
         case PIPE_TEXTURE_1D_ARRAY:
      assert(samples == 1);
   /* Lowered to 2D */
      case PIPE_TEXTURE_2D_ARRAY:
      return samples > 1 ? AGX_TEXTURE_DIMENSION_2D_ARRAY_MULTISAMPLED
         case PIPE_TEXTURE_3D:
      assert(samples == 1);
         case PIPE_TEXTURE_CUBE:
      assert(samples == 1);
         case PIPE_TEXTURE_CUBE_ARRAY:
      assert(samples == 1);
         default:
            }
      static enum agx_sample_count
   agx_translate_sample_count(unsigned samples)
   {
      switch (samples) {
   case 2:
         case 4:
         default:
            }
      static void
   agx_pack_texture(void *out, struct agx_resource *rsrc,
               {
                        uint8_t format_swizzle[4] = {
      desc->swizzle[0],
   desc->swizzle[1],
   desc->swizzle[2],
               if (util_format_is_depth_or_stencil(format)) {
      assert(!util_format_is_depth_and_stencil(format) &&
            /* Broadcast depth and stencil */
   format_swizzle[0] = 0;
   format_swizzle[1] = 0;
   format_swizzle[2] = 0;
               /* We only have a single swizzle for the user swizzle and the format fixup,
   * so compose them now. */
   uint8_t out_swizzle[4];
   uint8_t view_swizzle[4] = {state->swizzle_r, state->swizzle_g,
                     unsigned first_layer =
            /* Pack the descriptor into GPU memory */
   agx_pack(out, TEXTURE, cfg) {
      cfg.dimension = agx_translate_tex_dim(state->target,
         cfg.layout = agx_translate_layout(rsrc->layout.tiling);
   cfg.channels = agx_pixel_format[format].channels;
   cfg.type = agx_pixel_format[format].type;
   cfg.swizzle_r = agx_channel_from_pipe(out_swizzle[0]);
   cfg.swizzle_g = agx_channel_from_pipe(out_swizzle[1]);
   cfg.swizzle_b = agx_channel_from_pipe(out_swizzle[2]);
            if (state->target == PIPE_BUFFER) {
                     /* Use a 2D texture to increase the maximum size */
   cfg.width = 1024;
                  /* Stash the actual size in an unused part of the texture descriptor,
   * which we'll read later to implement txs.
   */
      } else {
      cfg.width = rsrc->base.width0;
   cfg.height = rsrc->base.height0;
   cfg.first_level = state->u.tex.first_level;
               cfg.srgb = (desc->colorspace == UTIL_FORMAT_COLORSPACE_SRGB);
   cfg.unk_mipmapped = rsrc->mipmapped;
            if (ail_is_compressed(&rsrc->layout)) {
      cfg.compressed_1 = true;
               if (include_bo) {
                              if (ail_is_compressed(&rsrc->layout)) {
      cfg.acceleration_buffer =
      agx_map_texture_gpu(rsrc, 0) + rsrc->layout.metadata_offset_B +
               if (state->target == PIPE_TEXTURE_3D) {
         } else if (state->target == PIPE_BUFFER) {
         } else {
                     if ((state->target == PIPE_TEXTURE_CUBE) ||
                  if (rsrc->layout.tiling == AIL_TILING_LINEAR &&
                     cfg.depth_linear = layers;
   cfg.layer_stride_linear = (rsrc->layout.layer_stride_B - 0x80);
      } else {
      assert((rsrc->layout.tiling != AIL_TILING_LINEAR) || (layers == 1));
                  if (rsrc->base.nr_samples > 1)
            if (state->target == PIPE_BUFFER) {
         } else if (rsrc->layout.tiling == AIL_TILING_LINEAR) {
         } else {
                              }
      static struct pipe_sampler_view *
   agx_create_sampler_view(struct pipe_context *pctx,
               {
      struct agx_resource *rsrc = agx_resource(orig_texture);
            if (!so)
            struct pipe_resource *texture = orig_texture;
                     /* Separate stencil always used on G13, so we need to fix up for Z32S8 */
   if (util_format_has_stencil(desc) && rsrc->separate_stencil) {
      if (util_format_has_depth(desc)) {
      /* Reinterpret as the depth-only part */
      } else {
      /* Use the stencil-only-part */
   rsrc = rsrc->separate_stencil;
   texture = &rsrc->base;
                           /* Save off the resource that we actually use, with the stencil fixed up */
   so->rsrc = rsrc;
            so->base = *state;
   so->base.texture = NULL;
   pipe_resource_reference(&so->base.texture, orig_texture);
   pipe_reference_init(&so->base.reference, 1);
   so->base.context = pctx;
      }
      static void
   agx_set_sampler_views(struct pipe_context *pctx, enum pipe_shader_type shader,
                     {
      struct agx_context *ctx = agx_context(pctx);
   unsigned new_nr = 0;
                     if (!views)
            for (i = 0; i < count; ++i) {
      if (views[i])
            if (take_ownership) {
      pipe_sampler_view_reference(
            } else {
      pipe_sampler_view_reference(
      (struct pipe_sampler_view **)&ctx->stage[shader].textures[i],
               for (; i < ctx->stage[shader].texture_count; i++) {
      pipe_sampler_view_reference(
      }
   ctx->stage[shader].texture_count = new_nr;
      }
      static void
   agx_sampler_view_destroy(struct pipe_context *ctx,
         {
      struct agx_sampler_view *view = (struct agx_sampler_view *)pview;
   pipe_resource_reference(&view->base.texture, NULL);
      }
      static struct pipe_surface *
   agx_create_surface(struct pipe_context *ctx, struct pipe_resource *texture,
         {
      agx_legalize_compression(agx_context(ctx), agx_resource(texture),
                     if (!surface)
                     pipe_reference_init(&surface->reference, 1);
                     surface->context = ctx;
   surface->format = surf_tmpl->format;
   surface->nr_samples = surf_tmpl->nr_samples;
   surface->width = u_minify(texture->width0, level);
   surface->height = u_minify(texture->height0, level);
   surface->texture = texture;
   surface->u.tex.first_layer = surf_tmpl->u.tex.first_layer;
   surface->u.tex.last_layer = surf_tmpl->u.tex.last_layer;
               }
      static void
   agx_set_clip_state(struct pipe_context *ctx,
         {
   }
      static void
   agx_set_polygon_stipple(struct pipe_context *ctx,
         {
   }
      static void
   agx_set_sample_mask(struct pipe_context *pipe, unsigned sample_mask)
   {
               /* Optimization: At most MSAA 4x supported, so normalize to avoid pointless
   * dirtying switching between e.g. 0xFFFF and 0xFFFFFFFF masks.
   */
            if (ctx->sample_mask != new_mask) {
      ctx->sample_mask = new_mask;
         }
      static void
   agx_set_scissor_states(struct pipe_context *pctx, unsigned start_slot,
               {
               assert(start_slot == 0 && "no geometry shaders");
            ctx->scissor = *scissor;
      }
      static void
   agx_set_stencil_ref(struct pipe_context *pctx,
         {
      struct agx_context *ctx = agx_context(pctx);
   ctx->stencil_ref = state;
      }
      static void
   agx_set_viewport_states(struct pipe_context *pctx, unsigned start_slot,
               {
               assert(start_slot == 0 && "no geometry shaders");
            ctx->dirty |= AGX_DIRTY_VIEWPORT;
      }
      static void
   agx_get_scissor_extents(const struct pipe_viewport_state *vp,
                     {
      float trans_x = vp->translate[0], trans_y = vp->translate[1];
            /* Calculate the extent of the viewport. Note if a particular dimension of
   * the viewport is an odd number of pixels, both the translate and the scale
   * will have a fractional part of 0.5, so adding and subtracting them yields
   * an integer. Therefore we don't need to round explicitly */
   *minx = CLAMP((int)(trans_x - abs_scale_x), 0, fb->width);
   *miny = CLAMP((int)(trans_y - abs_scale_y), 0, fb->height);
   *maxx = CLAMP((int)(trans_x + abs_scale_x), 0, fb->width);
            if (ss) {
      *minx = MAX2(ss->minx, *minx);
   *miny = MAX2(ss->miny, *miny);
   *maxx = MIN2(ss->maxx, *maxx);
         }
      static void
   agx_upload_viewport_scissor(struct agx_pool *pool, struct agx_batch *batch,
               {
                                 float minz, maxz;
            /* Allocate a new scissor descriptor */
   unsigned index = batch->scissor.size / AGX_SCISSOR_LENGTH;
            agx_pack(ptr, SCISSOR, cfg) {
      cfg.min_x = minx;
   cfg.min_y = miny;
   cfg.min_z = minz;
   cfg.max_x = maxx;
   cfg.max_y = maxy;
               /* Upload state */
   struct agx_ppp_update ppp =
      agx_new_ppp_update(pool, (struct AGX_PPP_HEADER){
                           agx_ppp_push(&ppp, DEPTH_BIAS_SCISSOR, cfg) {
               /* Use the current depth bias, we allocate linearly */
   unsigned count = batch->depth_bias.size / AGX_DEPTH_BIAS_LENGTH;
               agx_ppp_push(&ppp, REGION_CLIP, cfg) {
      cfg.enable = true;
   cfg.min_x = minx / 32;
   cfg.min_y = miny / 32;
   cfg.max_x = DIV_ROUND_UP(maxx, 32);
               agx_ppp_push(&ppp, VIEWPORT, cfg) {
      cfg.translate_x = vp->translate[0];
   cfg.translate_y = vp->translate[1];
   cfg.translate_z = vp->translate[2];
   cfg.scale_x = vp->scale[0];
   cfg.scale_y = vp->scale[1];
                  }
      static void
   agx_upload_depth_bias(struct agx_batch *batch,
         {
      void *ptr =
            agx_pack(ptr, DEPTH_BIAS, cfg) {
      cfg.depth_bias = rast->offset_units;
   cfg.slope_scale = rast->offset_scale;
         }
      /* A framebuffer state can be reused across batches, so it doesn't make sense
   * to add surfaces to the BO list here. Instead we added them when flushing.
   */
      static void
   agx_set_framebuffer_state(struct pipe_context *pctx,
         {
               if (!state)
            util_copy_framebuffer_state(&ctx->framebuffer, state);
   ctx->batch = NULL;
      }
      /*
   * To write out render targets, each render target surface is bound as a
   * writable shader image, written with the end-of-tile program. This helper
   * constructs the internal pipe_image_view used.
   */
   static struct pipe_image_view
   image_view_for_surface(struct pipe_surface *surf)
   {
      return (struct pipe_image_view){
      .resource = surf->texture,
   .format = surf->format,
   .access = PIPE_IMAGE_ACCESS_READ_WRITE,
   .shader_access = PIPE_IMAGE_ACCESS_READ_WRITE,
   .u.tex.single_layer_view =
         .u.tex.first_layer = surf->u.tex.first_layer,
   .u.tex.last_layer = surf->u.tex.last_layer,
         }
      /* Similarly, to read render targets, surfaces are bound as textures */
   static struct pipe_sampler_view
   sampler_view_for_surface(struct pipe_surface *surf)
   {
               return (struct pipe_sampler_view){
      /* To reduce shader variants, we always use a 2D texture. For reloads of
   * arrays and cube maps, we map a single layer as a 2D image.
   */
   .target = layered ? PIPE_TEXTURE_2D_ARRAY : PIPE_TEXTURE_2D,
   .swizzle_r = PIPE_SWIZZLE_X,
   .swizzle_g = PIPE_SWIZZLE_Y,
   .swizzle_b = PIPE_SWIZZLE_Z,
   .swizzle_a = PIPE_SWIZZLE_W,
   .u.tex =
      {
      .first_layer = surf->u.tex.first_layer,
   .last_layer = surf->u.tex.last_layer,
   .first_level = surf->u.tex.level,
         }
      static void
   agx_pack_image_atomic_data(void *packed, struct pipe_image_view *view)
   {
               if (tex->base.target == PIPE_BUFFER) {
      agx_pack(packed, PBE_BUFFER_SOFTWARE, cfg) {
            } else if (tex->layout.writeable_image) {
      unsigned level = view->u.tex.level;
            agx_pack(packed, ATOMIC_SOFTWARE, cfg) {
      cfg.base =
                           if (tex->layout.tiling == AIL_TILING_TWIDDLED) {
      struct ail_tile tile_size = tex->layout.tilesize_el[level];
                                 cfg.layer_stride_pixels = DIV_ROUND_UP(
               }
      static bool
   target_is_array(enum pipe_texture_target target)
   {
      switch (target) {
   case PIPE_TEXTURE_3D:
   case PIPE_TEXTURE_CUBE:
   case PIPE_TEXTURE_1D_ARRAY:
   case PIPE_TEXTURE_2D_ARRAY:
   case PIPE_TEXTURE_CUBE_ARRAY:
         default:
            }
      static void
   agx_batch_upload_pbe(struct agx_batch *batch, struct agx_pbe_packed *out,
               {
      struct agx_resource *tex = agx_resource(view->resource);
   const struct util_format_description *desc =
         enum pipe_texture_target target = tex->base.target;
            if (!is_buffer && view->u.tex.single_layer_view)
            /* To reduce shader variants, spilled layered render targets are accessed as
   * 2D Arrays regardless of the actual target, so force in that case.
   */
   if (arrays_as_2d && target_is_array(target))
            unsigned level = is_buffer ? 0 : view->u.tex.level;
            agx_pack(out, PBE, cfg) {
      cfg.dimension =
         cfg.layout = agx_translate_layout(tex->layout.tiling);
   cfg.channels = agx_pixel_format[view->format].channels;
   cfg.type = agx_pixel_format[view->format].type;
            assert(desc->nr_channels >= 1 && desc->nr_channels <= 4);
            if (desc->nr_channels >= 2)
            if (desc->nr_channels >= 3)
            if (desc->nr_channels >= 4)
            cfg.buffer = agx_map_texture_gpu(tex, layer);
            if (is_buffer) {
                                    /* Use a 2D texture to increase the maximum size */
   cfg.width = 1024;
   cfg.height = DIV_ROUND_UP(size_el, cfg.width);
   cfg.level = 0;
   cfg.stride = (cfg.width * util_format_get_blocksize(view->format)) - 4;
   cfg.layers = 1;
      } else if (util_res_sample_count(&tex->base) > 1 && !block_access) {
      /* Multisampled images are bound like buffer textures, with
   * addressing arithmetic to determine the texel to write.
   *
   * Note that the end-of-tile program uses real multisample images with
   * image_write_block instructions.
   */
                  cfg.dimension = AGX_TEXTURE_DIMENSION_2D;
   cfg.layout = AGX_LAYOUT_LINEAR;
   cfg.width = 1024;
   cfg.height = DIV_ROUND_UP(size_px, cfg.width);
   cfg.stride = (cfg.width * blocksize_B) - 4;
                  cfg.buffer += tex->layout.level_offsets_B[level];
      } else {
      cfg.width = view->resource->width0;
                           if (tex->layout.tiling == AIL_TILING_LINEAR &&
                     cfg.depth_linear = layers;
   cfg.layer_stride_linear = (tex->layout.layer_stride_B - 0x80);
      } else {
      assert((tex->layout.tiling != AIL_TILING_LINEAR) || (layers == 1));
               if (tex->layout.tiling == AIL_TILING_LINEAR) {
      cfg.stride = ail_get_linear_stride_B(&tex->layout, level) - 4;
      } else {
      cfg.page_aligned_layers = tex->layout.page_aligned_layers;
               if (tex->base.nr_samples > 1)
               if (ail_is_compressed(&tex->layout)) {
                     cfg.acceleration_buffer =
      agx_map_texture_gpu(tex, 0) + tex->layout.metadata_offset_B +
            /* When the descriptor isn't extended architecturally, we can use the last
   * 8 bytes as a sideband. We use it to provide metadata for image atomics.
   */
   if (!cfg.extended) {
      bool compact =
                  if (compact) {
                     STATIC_ASSERT(sizeof(cfg.software_defined) == 8);
      } else {
                     agx_pack_image_atomic_data(desc.cpu, view);
               }
      /* Likewise constant buffers, textures, and samplers are handled in a common
   * per-draw path, with dirty tracking to reduce the costs involved.
   */
      static void
   agx_set_constant_buffer(struct pipe_context *pctx, enum pipe_shader_type shader,
               {
      struct agx_context *ctx = agx_context(pctx);
   struct agx_stage *s = &ctx->stage[shader];
                     /* Upload user buffer immediately */
   if (constants->user_buffer && !constants->buffer) {
      u_upload_data(ctx->base.const_uploader, 0, constants->buffer_size, 64,
                              if (cb)
         else
               }
      static void
   agx_surface_destroy(struct pipe_context *ctx, struct pipe_surface *surface)
   {
      pipe_resource_reference(&surface->texture, NULL);
      }
      static void
   agx_delete_state(struct pipe_context *ctx, void *state)
   {
         }
      /* BOs added to the batch in the uniform upload path */
      static void
   agx_set_vertex_buffers(struct pipe_context *pctx, unsigned count,
               {
               util_set_vertex_buffers_mask(ctx->vertex_buffers, &ctx->vb_mask, buffers,
                     }
      static void *
   agx_create_vertex_elements(struct pipe_context *ctx, unsigned count,
         {
               struct agx_attribute *attribs = calloc(sizeof(*attribs), AGX_MAX_ATTRIBS);
   for (unsigned i = 0; i < count; ++i) {
               const struct util_format_description *desc =
         unsigned chan_size = desc->channel[0].size / 8;
            attribs[i] = (struct agx_attribute){
      .buf = ve.vertex_buffer_index,
   .src_offset = ve.src_offset,
   .stride = ve.src_stride,
   .format = ve.src_format,
                     }
      static void
   agx_bind_vertex_elements_state(struct pipe_context *pctx, void *cso)
   {
      struct agx_context *ctx = agx_context(pctx);
   ctx->attributes = cso;
      }
      static uint32_t
   asahi_vs_shader_key_hash(const void *key)
   {
         }
      static bool
   asahi_vs_shader_key_equal(const void *a, const void *b)
   {
         }
      static uint32_t
   asahi_fs_shader_key_hash(const void *key)
   {
         }
      static bool
   asahi_fs_shader_key_equal(const void *a, const void *b)
   {
         }
      /* No compute variants */
   static uint32_t
   asahi_cs_shader_key_hash(const void *key)
   {
         }
      static bool
   asahi_cs_shader_key_equal(const void *a, const void *b)
   {
         }
      static unsigned
   agx_find_linked_slot(struct agx_varyings_vs *vs, struct agx_varyings_fs *fs,
         {
      assert(offset < 4);
            if (slot == VARYING_SLOT_POS) {
      if (offset == 3) {
         } else if (offset == 2) {
      assert(fs->reads_z);
      } else {
                              /* If the layer is read but not written, its value will be ignored by the
   * agx_nir_predicate_layer_id lowering, so read garbage.
   */
   if (vs_index >= vs->nr_index && slot == VARYING_SLOT_LAYER)
            assert(vs_index >= 4 && "gl_Position should have been the first 4 slots");
   assert(vs_index < vs->nr_index &&
         assert((vs_index < vs->base_index_fp16) ==
                           if (fs->reads_z)
         else
      }
      static unsigned
   agx_num_general_outputs(struct agx_varyings_vs *vs)
   {
      unsigned nr_vs = vs->nr_index;
            assert(nr_vs >= 4 && "gl_Position must be written");
   if (writes_psiz)
               }
      static uint32_t
   agx_link_varyings_vs_fs(struct agx_pool *pool, struct agx_varyings_vs *vs,
         {
      /* If there are no bindings, there's nothing to emit */
   if (fs->nr_bindings == 0)
            size_t linkage_size =
            void *tmp = alloca(linkage_size);
   struct agx_cf_binding_header_packed *header = tmp;
                     agx_pack(header, CF_BINDING_HEADER, cfg) {
      cfg.number_of_32_bit_slots = nr_slots;
               for (unsigned i = 0; i < fs->nr_bindings; ++i) {
      agx_pack(bindings + i, CF_BINDING, cfg) {
      cfg.base_coefficient_register = fs->bindings[i].cf_base;
                  cfg.shade_model = fs->bindings[i].smooth ? AGX_SHADE_MODEL_GOURAUD
                        if (fs->bindings[i].slot == VARYING_SLOT_PNTC) {
      assert(fs->bindings[i].offset == 0);
      } else {
                     assert(cfg.base_slot + cfg.components <= nr_slots &&
               if (fs->bindings[i].slot == VARYING_SLOT_POS) {
      if (fs->bindings[i].offset == 2)
         else
               assert(cfg.base_coefficient_register + cfg.components <= fs->nr_cf &&
                  struct agx_ptr ptr = agx_pool_alloc_aligned(pool, (3 * linkage_size), 256);
            /* I don't understand why the data structures are repeated thrice */
   for (unsigned i = 0; i < 3; ++i) {
      memcpy(((uint8_t *)ptr.cpu) + (i * linkage_size), (uint8_t *)tmp,
                  }
      /* Does not take ownership of key. Clones if necessary. */
   static struct agx_compiled_shader *
   agx_compile_variant(struct agx_device *dev, struct agx_uncompiled_shader *so,
               {
      struct agx_compiled_shader *compiled = CALLOC_STRUCT(agx_compiled_shader);
   struct util_dynarray binary;
            struct blob_reader reader;
   blob_reader_init(&reader, so->serialized_nir.data, so->serialized_nir.size);
            /* This can happen at inopportune times and cause jank, log it */
   perf_debug(dev, "Compiling shader variant #%u",
                     if (nir->info.stage == MESA_SHADER_VERTEX) {
                        if (key->xfb.active && nir->xfb_info != NULL)
      } else if (nir->info.stage == MESA_SHADER_FRAGMENT) {
               struct agx_tilebuffer_layout tib = agx_build_tilebuffer_layout(
            if (dev->debug & AGX_DBG_SMALLTILE)
            nir_lower_blend_options opts = {
      .scalar_blend_const = true,
   .logicop_enable = key->blend.logicop_enable,
               static_assert(ARRAY_SIZE(opts.format) == PIPE_MAX_COLOR_BUFS,
            for (unsigned i = 0; i < PIPE_MAX_COLOR_BUFS; ++i)
                     /* It's more efficient to use masked stores (with
   * agx_nir_lower_tilebuffer) than to emulate colour masking with
   * nir_lower_blend.
   */
            for (unsigned i = 0; i < PIPE_MAX_COLOR_BUFS; ++i) {
      /* TODO: Flakes some dEQPs, seems to invoke UB. Revisit later.
   * dEQP-GLES2.functional.fragment_ops.interaction.basic_shader.77
   * dEQP-GLES2.functional.fragment_ops.interaction.basic_shader.98
   */
   if (0 /* agx_tilebuffer_supports_mask(&tib, i) */) {
      colormasks[i] = key->blend.rt[i].colormask;
      } else {
                  /* If not all bound RTs are fully written to, we need to force
   * translucent pass type. agx_nir_lower_tilebuffer will take
   * care of this for its own colormasks input.
   */
   unsigned comps = util_format_get_nr_components(key->rt_formats[i]);
   if (i < key->nr_cbufs &&
      (opts.rt[i].colormask & BITFIELD_MASK(comps)) !=
                  /* Clip plane lowering creates discard instructions, so run that before
   * lowering discards. Note: this introduces extra loads from the clip
   * plane outputs, but they use smooth interpolation so it does not affect
   * the flat/linear masks that get propagated back to the VS.
   */
   if (key->clip_plane_enable) {
                  /* Discards must be lowering before lowering MSAA to handle discards */
            /* Alpha-to-coverage must be lowered before alpha-to-one */
   if (key->blend.alpha_to_coverage)
            /* Alpha-to-one must be lowered before blending */
   if (key->blend.alpha_to_one)
                     /* XXX: don't replicate this all over the driver */
   unsigned rt_spill_base = BITSET_LAST_BIT(nir->info.textures_used) +
         unsigned rt_spill = rt_spill_base;
   NIR_PASS_V(nir, agx_nir_lower_tilebuffer, &tib, colormasks, &rt_spill,
            /* If anything spilled, we have bindless texture */
            NIR_PASS_V(nir, agx_nir_lower_sample_intrinsics);
   NIR_PASS_V(nir, agx_nir_lower_monolithic_msaa,
            &(struct agx_msaa_state){
               if (key->sprite_coord_enable) {
      NIR_PASS_V(nir, nir_lower_texcoord_replace_late,
                                 struct agx_shader_key base_key = {
      .needs_g13x_coherency = (dev->params.gpu_generation == 13 &&
                     if (nir->info.stage == MESA_SHADER_FRAGMENT)
            if (nir->info.stage == MESA_SHADER_VERTEX) {
      base_key.vs.outputs_flat_shaded = key_->vs.outputs_flat_shaded;
               NIR_PASS_V(nir, agx_nir_lower_sysvals);
   NIR_PASS_V(nir, agx_nir_layout_uniforms, so->internal_bindless, compiled,
                     /* reads_tib => Translucent pass type */
            /* Could be optimized to use non-translucent pass types with the appropriate
   * HSR configuration, but that mechanism is not yet understood. Warn that
   * we're leaving perf on the table when used.
   */
   if (force_translucent)
            if (binary.size) {
      compiled->bo = agx_bo_create(dev, binary.size,
                        ralloc_free(nir);
               }
      static struct agx_compiled_shader *
   agx_get_shader_variant(struct agx_screen *screen,
                     {
      struct agx_compiled_shader *compiled =
            if (!compiled) {
      compiled = agx_compile_variant(&screen->dev, so, debug, key);
               /* key may be destroyed after we return, so clone it before using it as a
   * hash table key. The clone is logically owned by the hash table.
   */
   union asahi_shader_key *cloned_key =
            if (so->type == PIPE_SHADER_FRAGMENT) {
         } else if (so->type == PIPE_SHADER_VERTEX) {
         } else {
      assert(gl_shader_stage_is_compute(so->type));
                           }
      static void
   agx_shader_initialize(struct agx_device *dev, struct agx_uncompiled_shader *so,
         {
      if (nir->info.stage == MESA_SHADER_KERNEL)
                     nir_lower_robust_access_options robustness = {
      /* Images accessed through the texture or PBE hardware are robust, so we
   * don't set lower_image. However, buffer images and image atomics are
   * lowered so require robustness lowering.
   */
   .lower_buffer_image = true,
               /* We need to lower robustness before bindings, since robustness lowering
   * affects the bindings used.
   */
            /* Similarly, we need to do early texture lowering before bindings */
            /* We need to lower binding tables before calling agx_preprocess_nir, since
   * that does texture lowering that needs to know the binding model.
   */
            if (nir->info.stage == MESA_SHADER_FRAGMENT) {
      /* Lower to maximum colour buffers, the excess stores will get cleaned up
   * by tilebuffer lowering so they won't become real shader code. However,
   * that depends on the shader key which we don't have at this point.
   */
               bool allow_mediump = !(dev->debug & AGX_DBG_NO16);
            blob_init(&so->serialized_nir);
   nir_serialize(&so->serialized_nir, nir, true);
   _mesa_sha1_compute(so->serialized_nir.data, so->serialized_nir.size,
               }
      static void *
   agx_create_shader_state(struct pipe_context *pctx,
         {
      struct agx_context *ctx = agx_context(pctx);
   struct agx_uncompiled_shader *so =
                  if (!so)
                     nir_shader *nir = cso->type == PIPE_SHADER_IR_NIR
                  if (nir->info.stage == MESA_SHADER_VERTEX) {
      so->variants = _mesa_hash_table_create(so, asahi_vs_shader_key_hash,
      } else {
      so->variants = _mesa_hash_table_create(so, asahi_fs_shader_key_hash,
                        /* We're done with the NIR, throw it away */
   ralloc_free(nir);
            /* For shader-db, precompile a shader with a default key. This could be
   * improved but hopefully this is acceptable for now.
   */
   if (dev->debug & AGX_DBG_PRECOMPILE) {
               switch (so->type) {
   case PIPE_SHADER_VERTEX: {
      key.vs.vbuf.count = AGX_MAX_VBUFS;
   for (unsigned i = 0; i < AGX_MAX_VBUFS; ++i) {
      key.vs.vbuf.attributes[i] = (struct agx_attribute){
      .buf = i,
   .stride = 16,
                     }
   case PIPE_SHADER_FRAGMENT:
      key.fs.nr_cbufs = 1;
   key.fs.nr_samples = 1;
   for (unsigned i = 0; i < key.fs.nr_cbufs; ++i) {
                     const nir_lower_blend_channel replace = {
      .func = PIPE_BLEND_ADD,
                     key.fs.blend.rt[i].rgb = replace;
      }
      default:
                                 }
      static void *
   agx_create_compute_state(struct pipe_context *pctx,
         {
      struct agx_context *ctx = agx_context(pctx);
   struct agx_device *dev = agx_device(pctx->screen);
   struct agx_uncompiled_shader *so =
            if (!so)
            so->variants = _mesa_hash_table_create(so, asahi_cs_shader_key_hash,
                     assert(cso->ir_type == PIPE_SHADER_IR_NIR && "TGSI kernels unsupported");
            agx_shader_initialize(dev, so, nir, ctx->support_lod_bias);
            /* We're done with the NIR, throw it away */
   ralloc_free(nir);
      }
      static void
   agx_get_compute_state_info(struct pipe_context *pctx, void *cso,
         {
      union asahi_shader_key key = {0};
   struct agx_compiled_shader *so =
            info->max_threads =
         info->private_memory = 0;
   info->preferred_simd_size = 32;
            /* HACK: Clamp max_threads to what we advertise. When we fix the CAP
   * situation around block sizes, we can drop this.
   */
      }
      /* Does not take ownership of key. Clones if necessary. */
   static bool
   agx_update_shader(struct agx_context *ctx, struct agx_compiled_shader **out,
         {
      struct agx_uncompiled_shader *so = ctx->stage[stage].shader;
                     if (he) {
      if ((*out) == he->data)
            *out = he->data;
               struct agx_screen *screen = agx_screen(ctx->base.screen);
   *out = agx_get_shader_variant(screen, so, &ctx->base.debug, key);
      }
      static bool
   agx_update_vs(struct agx_context *ctx)
   {
      /* Only proceed if the shader or anything the key depends on changes
   *
   * vb_mask, attributes, vertex_buffers: VERTEX
   * streamout.active: XFB
   * outputs_{flat,linear}_shaded: FS_PROG
   */
   if (!(ctx->dirty & (AGX_DIRTY_VS_PROG | AGX_DIRTY_VERTEX | AGX_DIRTY_XFB |
                  struct asahi_vs_shader_key key = {
      .vbuf.count = util_last_bit(ctx->vb_mask),
   .xfb = ctx->streamout.key,
   .outputs_flat_shaded =
         .outputs_linear_shaded =
               memcpy(key.vbuf.attributes, ctx->attributes,
            return agx_update_shader(ctx, &ctx->vs, PIPE_SHADER_VERTEX,
      }
      static bool
   agx_update_fs(struct agx_batch *batch)
   {
               /* Only proceed if the shader or anything the key depends on changes
   *
   * batch->key: implicitly dirties everything, no explicit check
   * rast: RS
   * blend: BLEND
   * sample_mask: SAMPLE_MASK
   */
   if (!(ctx->dirty & (AGX_DIRTY_FS_PROG | AGX_DIRTY_RS | AGX_DIRTY_BLEND |
                  unsigned nr_samples = util_framebuffer_get_num_samples(&batch->key);
            struct asahi_fs_shader_key key = {
      .nr_cbufs = batch->key.nr_cbufs,
   .clip_plane_enable = ctx->rast->base.clip_plane_enable,
   .nr_samples = nr_samples,
   .layered = util_framebuffer_get_num_layers(&batch->key) > 1,
            /* Only lower sample mask if at least one sample is masked out */
   .api_sample_mask =
               if (batch->reduced_prim == MESA_PRIM_POINTS)
            for (unsigned i = 0; i < key.nr_cbufs; ++i) {
                                    /* Normalize key */
   if (!key.multisample)
            return agx_update_shader(ctx, &ctx->fs, PIPE_SHADER_FRAGMENT,
      }
      static void
   agx_bind_shader_state(struct pipe_context *pctx, void *cso)
   {
      if (!cso)
            struct agx_context *ctx = agx_context(pctx);
            if (so->type == PIPE_SHADER_VERTEX)
         else if (so->type == PIPE_SHADER_FRAGMENT)
               }
      static void
   agx_delete_compiled_shader(struct hash_entry *ent)
   {
      struct agx_compiled_shader *so = ent->data;
   agx_bo_unreference(so->bo);
      }
      static void
   agx_delete_shader_state(struct pipe_context *ctx, void *cso)
   {
      struct agx_uncompiled_shader *so = cso;
   _mesa_hash_table_destroy(so->variants, agx_delete_compiled_shader);
   blob_finish(&so->serialized_nir);
      }
      static unsigned
   sampler_count(struct agx_context *ctx, struct agx_compiled_shader *cs,
         {
               if (cs->info.uses_txf)
               }
      static inline enum agx_sampler_states
   translate_sampler_state_count(struct agx_context *ctx,
               {
      return agx_translate_sampler_state_count(sampler_count(ctx, cs, stage),
      }
      /*
   * Despite having both a layout *and* a flag that I only see Metal use with null
   * textures, AGX doesn't seem to have "real" null textures. Instead we need to
   * bind an arbitrary address and throw away the results to read all 0's.
   * Accordingly, the caller must pass some address that lives at least as long as
   * the texture descriptor itself.
   */
   static void
   agx_set_null_texture(struct agx_texture_packed *tex, uint64_t valid_address)
   {
      agx_pack(tex, TEXTURE, cfg) {
      cfg.layout = AGX_LAYOUT_NULL;
   cfg.channels = AGX_CHANNELS_R8;
   cfg.type = AGX_TEXTURE_TYPE_UNORM /* don't care */;
   cfg.swizzle_r = AGX_CHANNEL_0;
   cfg.swizzle_g = AGX_CHANNEL_0;
   cfg.swizzle_b = AGX_CHANNEL_0;
   cfg.swizzle_a = AGX_CHANNEL_0;
   cfg.address = valid_address;
         }
      static uint32_t
   agx_nr_tex_descriptors_without_spilled_rts(const struct agx_compiled_shader *cs)
   {
      /* 2 descriptors per image, 1 descriptor per texture */
      }
      static uint32_t
   agx_nr_tex_descriptors(struct agx_batch *batch, enum pipe_shader_type stage,
         {
               /* We add on texture/PBE descriptors for spilled render targets */
   bool spilled_rt = stage == PIPE_SHADER_FRAGMENT &&
         if (spilled_rt)
               }
      /*
   * For spilled render targets, upload a texture/PBE pair for each surface to
   * allow loading/storing to the render target from the shader.
   */
   static void
   agx_upload_spilled_rt_descriptors(struct agx_texture_packed *out,
         {
      for (unsigned rt = 0; rt < batch->key.nr_cbufs; ++rt) {
      struct agx_texture_packed *texture = out + (2 * rt);
            struct pipe_surface *surf = batch->key.cbufs[rt];
   if (!surf)
            struct agx_resource *rsrc = agx_resource(surf->texture);
   struct pipe_image_view view = image_view_for_surface(surf);
            agx_pack_texture(texture, rsrc, surf->format, &sampler_view, true);
         }
      static void
   agx_upload_textures(struct agx_batch *batch, struct agx_compiled_shader *cs,
         {
      struct agx_context *ctx = batch->ctx;
   unsigned nr_textures = cs->info.nr_bindful_textures;
   unsigned nr_active_textures = ctx->stage[stage].texture_count;
   unsigned nr_tex_descriptors = agx_nr_tex_descriptors(batch, stage, cs);
            struct agx_ptr T_tex = agx_pool_alloc_aligned(
                     for (unsigned i = 0; i < MIN2(nr_textures, nr_active_textures); ++i) {
               if (tex == NULL) {
      agx_set_null_texture(&textures[i], T_tex.gpu);
               struct agx_resource *rsrc = tex->rsrc;
            unsigned first_layer =
            /* Without the address */
            /* Just the address */
   struct agx_texture_packed texture2;
   agx_pack(&texture2, TEXTURE, cfg) {
                              if (ail_is_compressed(&rsrc->layout)) {
      cfg.acceleration_buffer =
      agx_map_texture_gpu(rsrc, 0) + rsrc->layout.metadata_offset_B +
               agx_merge(texture, texture2, TEXTURE);
               for (unsigned i = nr_active_textures; i < nr_textures; ++i)
            for (unsigned i = 0; i < nr_images; ++i) {
      if (!(ctx->stage[stage].image_mask & BITFIELD_BIT(i))) {
      /* TODO: Null images */
               struct pipe_image_view *view = &ctx->stage[stage].images[i];
            /* Image descriptors come in pairs after the textures */
   struct agx_texture_packed *texture =
                           struct pipe_sampler_view sampler_view = util_image_to_sampler_view(view);
   agx_pack_texture(texture, agx_resource(view->resource), view->format,
                     if (stage == PIPE_SHADER_FRAGMENT &&
               struct agx_texture_packed *out =
                              batch->texture_count[stage] = nr_tex_descriptors;
      }
      static void
   agx_upload_samplers(struct agx_batch *batch, struct agx_compiled_shader *cs,
         {
      struct agx_context *ctx = batch->ctx;
   unsigned nr_samplers = sampler_count(ctx, cs, stage);
            size_t sampler_length =
            struct agx_ptr T =
            uint8_t *out_sampler = T.cpu;
   for (unsigned i = 0; i < nr_samplers; ++i) {
      struct agx_sampler_state *sampler = ctx->stage[stage].samplers[i];
            if (cs->info.uses_txf && i == cs->info.txf_sampler) {
      agx_pack(out, SAMPLER, cfg) {
                     /* Out-of-bounds reads must return 0 */
   cfg.wrap_s = AGX_WRAP_CLAMP_TO_BORDER;
   cfg.wrap_t = AGX_WRAP_CLAMP_TO_BORDER;
   cfg.wrap_r = AGX_WRAP_CLAMP_TO_BORDER;
         } else if (sampler) {
               if (custom_borders) {
      memcpy(out_sampler + AGX_SAMPLER_LENGTH, &sampler->border,
      } else {
            } else {
                              batch->sampler_count[stage] = nr_samplers;
      }
      static void
   agx_update_descriptors(struct agx_batch *batch, struct agx_compiled_shader *cs,
         {
               if (ctx->stage[stage].dirty & AGX_STAGE_DIRTY_IMAGE)
            if (ctx->stage[stage].dirty & AGX_STAGE_DIRTY_SAMPLER)
            if (ctx->stage[stage].dirty) {
      batch->uniforms.tables[AGX_SYSVAL_STAGE(stage)] =
         }
      static uint32_t
   agx_build_pipeline(struct agx_batch *batch, struct agx_compiled_shader *cs,
         {
      struct agx_context *ctx = batch->ctx;
   struct agx_usc_builder b =
            if (batch->texture_count[stage]) {
      agx_usc_pack(&b, TEXTURE, cfg) {
      cfg.start = 0;
   cfg.count =
                        if (batch->sampler_count[stage]) {
      agx_usc_pack(&b, SAMPLER, cfg) {
      cfg.start = 0;
   cfg.count = batch->sampler_count[stage];
                  for (unsigned i = 0; i < cs->push_range_count; ++i) {
      agx_usc_uniform(
      &b, cs->push[i].uniform, cs->push[i].length,
            if (stage == PIPE_SHADER_FRAGMENT) {
         } else if (stage == PIPE_SHADER_COMPUTE) {
               agx_usc_pack(&b, SHARED, cfg) {
      cfg.layout = AGX_SHARED_LAYOUT_VERTEX_COMPUTE;
   cfg.bytes_per_threadgroup = size > 0 ? size : 65536;
         } else {
                  agx_usc_pack(&b, SHADER, cfg) {
      if (stage == PIPE_SHADER_FRAGMENT)
            cfg.code = cs->bo->ptr.gpu + cs->info.main_offset;
               agx_usc_pack(&b, REGISTERS, cfg) {
      cfg.register_count = cs->info.nr_gprs;
               if (stage == PIPE_SHADER_FRAGMENT) {
      agx_usc_pack(&b, FRAGMENT_PROPERTIES, cfg) {
      bool writes_sample_mask = ctx->fs->info.writes_sample_mask;
   cfg.early_z_testing = !writes_sample_mask;
   cfg.unk_4 = 0x2;
                  if (cs->info.has_preamble) {
      agx_usc_pack(&b, PRESHADER, cfg) {
            } else {
      agx_usc_pack(&b, NO_PRESHADER, cfg)
                  }
      uint64_t
   agx_build_meta(struct agx_batch *batch, bool store, bool partial_render)
   {
               /* Construct the key */
            for (unsigned rt = 0; rt < PIPE_MAX_COLOR_BUFS; ++rt) {
               if (surf == NULL)
            if (store) {
      /* TODO: Suppress stores to discarded render targets */
      } else if (batch->tilebuffer_layout.spilled[rt] && partial_render) {
      /* Partial render programs exist only to store/load the tilebuffer to
   * main memory. When render targets are already spilled to main memory,
   * there's nothing to do.
   */
      } else {
      struct agx_resource *rsrc = agx_resource(surf->texture);
   bool valid = agx_resource_valid(rsrc, surf->u.tex.level);
                                 /* The background program used for partial renders must always load
   * whatever was stored in the mid-frame end-of-tile program.
                  key.op[rt] = load    ? AGX_META_OP_LOAD
                        /* Begin building the pipeline */
   struct agx_usc_builder b =
            bool needs_sampler = false;
            for (unsigned rt = 0; rt < PIPE_MAX_COLOR_BUFS; ++rt) {
      if (key.op[rt] == AGX_META_OP_LOAD) {
      /* Each reloaded render target is textured */
   struct agx_ptr texture =
                                                agx_usc_pack(&b, TEXTURE, cfg) {
      cfg.start = rt;
   cfg.count = 1;
                  } else if (key.op[rt] == AGX_META_OP_CLEAR) {
      assert(batch->uploaded_clear_color[rt] && "set when cleared");
   agx_usc_uniform(&b, 4 + (8 * rt), 8, batch->uploaded_clear_color[rt]);
      } else if (key.op[rt] == AGX_META_OP_STORE) {
      struct pipe_image_view view =
                                                agx_usc_pack(&b, TEXTURE, cfg) {
      cfg.start = rt;
   cfg.count = 1;
                     if (agx_tilebuffer_spills(&batch->tilebuffer_layout) && !partial_render &&
      !store) {
   /* Upload texture/PBE descriptors for each render target so we can clear
   * spilled render targets.
   */
   struct agx_ptr descs = agx_pool_alloc_aligned(
                  agx_usc_pack(&b, TEXTURE, cfg) {
      cfg.start = 0;
   cfg.count = 2 * batch->key.nr_cbufs;
               /* Bind the base as u0_u1 for bindless access */
   agx_usc_uniform(&b, 0, 4,
                     /* All render targets share a sampler */
   if (needs_sampler) {
      struct agx_ptr sampler =
            agx_pack(sampler.cpu, SAMPLER, cfg) {
      cfg.magnify_linear = true;
   cfg.minify_linear = false;
   cfg.mip_filter = AGX_MIP_FILTER_NONE;
   cfg.wrap_s = AGX_WRAP_CLAMP_TO_EDGE;
   cfg.wrap_t = AGX_WRAP_CLAMP_TO_EDGE;
   cfg.wrap_r = AGX_WRAP_CLAMP_TO_EDGE;
   cfg.pixel_coordinates = true;
               agx_usc_pack(&b, SAMPLER, cfg) {
      cfg.start = 0;
   cfg.count = 1;
                           /* Get the shader */
   key.reserved_preamble = uniforms;
   struct agx_meta_shader *shader = agx_get_meta_shader(&ctx->meta, &key);
            agx_usc_pack(&b, SHADER, cfg) {
      cfg.code = shader->ptr;
               agx_usc_pack(&b, REGISTERS, cfg)
            if (shader->info.has_preamble) {
      agx_usc_pack(&b, PRESHADER, cfg) {
            } else {
      agx_usc_pack(&b, NO_PRESHADER, cfg)
                  }
      /*
   * Return the standard sample positions, packed into a 32-bit word with fixed
   * point nibbles for each x/y component of the (at most 4) samples. This is
   * suitable for programming the PPP_MULTISAMPLECTL control register.
   */
   static uint32_t
   agx_default_sample_positions(unsigned nr_samples)
   {
      switch (nr_samples) {
   case 1:
         case 2:
         case 4:
         default:
            }
      void
   agx_batch_init_state(struct agx_batch *batch)
   {
      if (batch->initialized)
            if (batch->key.width == AGX_COMPUTE_BATCH_WIDTH) {
      batch->initialized = true;
               /* Emit state on the batch that we don't change and so don't dirty track */
   uint8_t *out = batch->encoder_current;
   struct agx_ppp_update ppp =
      agx_new_ppp_update(&batch->pool, (struct AGX_PPP_HEADER){
                                       /* clang-format off */
   agx_ppp_push(&ppp, W_CLAMP, cfg) cfg.w_clamp = 1e-10;
   agx_ppp_push(&ppp, CULL_2, cfg);
   agx_ppp_push(&ppp, FRAGMENT_OCCLUSION_QUERY_2, cfg);
   agx_ppp_push(&ppp, OUTPUT_UNKNOWN, cfg);
   agx_ppp_push(&ppp, VARYING_2, cfg);
            agx_ppp_fini(&out, &ppp);
            /* Mark it as initialized now, since agx_batch_writes() will check this. */
            /* Choose a tilebuffer layout given the framebuffer key */
   enum pipe_format formats[PIPE_MAX_COLOR_BUFS] = {0};
   for (unsigned i = 0; i < batch->key.nr_cbufs; ++i) {
      struct pipe_surface *surf = batch->key.cbufs[i];
   if (surf)
               batch->tilebuffer_layout = agx_build_tilebuffer_layout(
      formats, batch->key.nr_cbufs,
   util_framebuffer_get_num_samples(&batch->key),
         if (agx_device(batch->ctx->base.screen)->debug & AGX_DBG_SMALLTILE)
            /* If the layout spilled render targets, we need to decompress those render
   * targets to ensure we can write to them.
   */
   if (agx_tilebuffer_spills(&batch->tilebuffer_layout)) {
      for (unsigned i = 0; i < batch->key.nr_cbufs; ++i) {
                     struct pipe_surface *surf = batch->key.cbufs[i];
                  struct agx_resource *rsrc = agx_resource(surf->texture);
                  /* Decompress if we can and shadow if we can't. */
   if (rsrc->base.bind & PIPE_BIND_SHARED)
         else
                  if (batch->key.zsbuf) {
      struct agx_resource *rsrc = agx_resource(batch->key.zsbuf->texture);
            if (rsrc->separate_stencil)
               for (unsigned i = 0; i < batch->key.nr_cbufs; ++i) {
      if (batch->key.cbufs[i])
               /* Set up standard sample positions */
   batch->uniforms.ppp_multisamplectl =
      }
      static enum agx_object_type
   agx_point_object_type(struct agx_rasterizer *rast)
   {
      return (rast->base.sprite_coord_mode == PIPE_SPRITE_COORD_UPPER_LEFT)
            }
      #define MAX_PPP_UPDATES 2
      static uint8_t *
   agx_encode_state(struct agx_batch *batch, uint8_t *out, bool is_lines,
         {
      struct agx_context *ctx = batch->ctx;
   struct agx_rasterizer *rast = ctx->rast;
         #define IS_DIRTY(ST) !!(ctx->dirty & AGX_DIRTY_##ST)
         agx_update_descriptors(batch, ctx->vs, PIPE_SHADER_VERTEX);
            if (IS_DIRTY(VERTEX)) {
                  if (IS_DIRTY(BLEND_COLOR)) {
      memcpy(batch->uniforms.blend_constant, &ctx->blend_color,
               if (IS_DIRTY(VS) || IS_DIRTY(FS) || IS_DIRTY(VERTEX) ||
                           if (IS_DIRTY(VS)) {
      agx_pack(out, VDM_STATE, cfg) {
      cfg.vertex_shader_word_0_present = true;
   cfg.vertex_shader_word_1_present = true;
   cfg.vertex_outputs_present = true;
      }
            agx_pack(out, VDM_STATE_VERTEX_SHADER_WORD_0, cfg) {
      cfg.uniform_register_count = ctx->vs->info.push_count;
   cfg.preshader_register_count = ctx->vs->info.nr_preamble_gprs;
   cfg.texture_state_register_count =
         cfg.sampler_state_register_count =
      }
            agx_pack(out, VDM_STATE_VERTEX_SHADER_WORD_1, cfg) {
      cfg.pipeline =
      }
            agx_pack(out, VDM_STATE_VERTEX_OUTPUTS, cfg) {
      cfg.output_count_1 = ctx->vs->info.varyings.vs.nr_index;
      }
            agx_pack(out, VDM_STATE_VERTEX_UNKNOWN, cfg) {
      cfg.flat_shading_control = ctx->rast->base.flatshade_first
                  }
            /* Pad up to a multiple of 8 bytes */
   memset(out, 0, 4);
               struct agx_pool *pool = &batch->pool;
            if ((ctx->dirty & AGX_DIRTY_RS) && ctx->rast->base.offset_tri) {
      agx_upload_depth_bias(batch, &ctx->rast->base);
               if (ctx->dirty & (AGX_DIRTY_VIEWPORT | AGX_DIRTY_SCISSOR_ZBIAS)) {
      agx_upload_viewport_scissor(
      pool, batch, &out, &ctx->viewport,
                     if (IS_DIRTY(VS_PROG) || IS_DIRTY(FS_PROG) || IS_DIRTY(RS)) {
      batch->varyings = agx_link_varyings_vs_fs(
                  varyings_dirty = true;
               bool object_type_dirty =
            bool fragment_face_dirty =
            enum agx_object_type object_type = is_points  ? agx_point_object_type(rast)
                  struct AGX_PPP_HEADER dirty = {
      .fragment_control =
         .fragment_control_2 = IS_DIRTY(PRIM) || IS_DIRTY(FS_PROG) || IS_DIRTY(RS),
   .fragment_front_face = fragment_face_dirty,
   .fragment_front_face_2 = object_type_dirty || IS_DIRTY(FS_PROG),
   .fragment_front_stencil = IS_DIRTY(ZS),
   .fragment_back_face = fragment_face_dirty,
   .fragment_back_face_2 = object_type_dirty || IS_DIRTY(FS_PROG),
   .fragment_back_stencil = IS_DIRTY(ZS),
   .output_select = IS_DIRTY(VS_PROG) || IS_DIRTY(FS_PROG),
   .varying_counts_32 = IS_DIRTY(VS_PROG),
   .varying_counts_16 = IS_DIRTY(VS_PROG),
   .cull = IS_DIRTY(RS),
   .fragment_shader =
         .occlusion_query = IS_DIRTY(QUERY),
                        if (dirty.fragment_control) {
      agx_ppp_push(&ppp, FRAGMENT_CONTROL, cfg) {
      if (ctx->active_queries && ctx->occlusion_query) {
      if (ctx->occlusion_query->type == PIPE_QUERY_OCCLUSION_COUNTER)
         else
               cfg.stencil_test_enable = ctx->zs->base.stencil[0].enabled;
                  /* Always enable scissoring so we may scissor to the viewport (TODO:
   * optimize this out if the viewport is the default and the app does
   * not use the scissor test)
   */
                  if (dirty.fragment_control_2) {
      agx_ppp_push(&ppp, FRAGMENT_CONTROL, cfg) {
      /* This avoids broken derivatives along primitive edges */
   cfg.disable_tri_merging =
         cfg.tag_write_disable = ctx->fs->info.tag_write_disable ||
                        if (dirty.fragment_front_face) {
      struct agx_fragment_face_packed front_face;
   agx_pack(&front_face, FRAGMENT_FACE, cfg) {
      cfg.stencil_reference = ctx->stencil_ref.ref_value[0];
   cfg.line_width = rast->line_width;
                                    if (dirty.fragment_front_face_2)
            if (dirty.fragment_front_stencil) {
      agx_ppp_push_packed(&ppp, ctx->zs->front_stencil.opaque,
               if (dirty.fragment_back_face) {
               agx_pack(&back_face, FRAGMENT_FACE, cfg) {
      bool twosided = ctx->zs->base.stencil[1].enabled;
   cfg.stencil_reference = ctx->stencil_ref.ref_value[twosided ? 1 : 0];
   cfg.line_width = rast->line_width;
               back_face.opaque[0] |= ctx->zs->depth.opaque[0];
               if (dirty.fragment_back_face_2)
            if (dirty.fragment_back_stencil)
            if (dirty.output_select) {
      agx_ppp_push(&ppp, OUTPUT_SELECT, cfg) {
      cfg.varyings = !!fs->info.varyings.fs.nr_bindings;
   cfg.point_size = vs->info.writes_psiz;
   cfg.viewport_target = vs->info.writes_layer_viewport;
   cfg.render_target = vs->info.writes_layer_viewport;
                           if (dirty.varying_counts_32) {
      agx_ppp_push(&ppp, VARYING_COUNTS, cfg) {
      cfg.smooth = vs->info.varyings.vs.num_32_smooth;
   cfg.flat = vs->info.varyings.vs.num_32_flat;
               agx_ppp_push(&ppp, VARYING_COUNTS, cfg) {
      cfg.smooth = vs->info.varyings.vs.num_16_smooth;
   cfg.flat = vs->info.varyings.vs.num_16_flat;
                  if (dirty.cull)
            if (dirty.fragment_shader) {
               agx_ppp_push(&ppp, FRAGMENT_SHADER, cfg) {
      cfg.pipeline =
         cfg.uniform_register_count = ctx->fs->info.push_count;
   cfg.preshader_register_count = ctx->fs->info.nr_preamble_gprs;
   cfg.texture_state_register_count =
         cfg.sampler_state_register_count =
                        /* XXX: This is probably wrong */
                  if (dirty.occlusion_query) {
      agx_ppp_push(&ppp, FRAGMENT_OCCLUSION_QUERY, cfg) {
      if (ctx->active_queries && ctx->occlusion_query) {
         } else {
                        if (dirty.output_size) {
      agx_ppp_push(&ppp, OUTPUT_SIZE, cfg)
               agx_ppp_fini(&out, &ppp);
         #undef IS_DIRTY
         assert(ppp_updates <= MAX_PPP_UPDATES);
      }
      static enum agx_primitive
   agx_primitive_for_pipe(enum mesa_prim mode)
   {
      switch (mode) {
   case MESA_PRIM_POINTS:
         case MESA_PRIM_LINES:
         case MESA_PRIM_LINE_STRIP:
         case MESA_PRIM_LINE_LOOP:
         case MESA_PRIM_TRIANGLES:
         case MESA_PRIM_TRIANGLE_STRIP:
         case MESA_PRIM_TRIANGLE_FAN:
         case MESA_PRIM_QUADS:
         case MESA_PRIM_QUAD_STRIP:
         default:
            }
      static uint64_t
   agx_index_buffer_rsrc_ptr(struct agx_batch *batch,
         {
               struct agx_resource *rsrc = agx_resource(info->index.resource);
            *extent = ALIGN_POT(util_resource_size(&rsrc->base), 4);
      }
      static uint64_t
   agx_index_buffer_direct_ptr(struct agx_batch *batch,
               {
      off_t offset = draw->start * info->index_size;
            if (!info->has_user_indices) {
               *extent = ALIGN_POT(MIN2(*extent - offset, max_extent), 4);
      } else {
               return agx_pool_upload_aligned(&batch->pool,
               }
      static bool
   agx_scissor_culls_everything(struct agx_context *ctx)
   {
      unsigned minx, miny, maxx, maxy;
   agx_get_scissor_extents(&ctx->viewport,
                     }
      static void
   agx_ensure_cmdbuf_has_space(struct agx_batch *batch, size_t space)
   {
      /* Assert that we have space for a link tag */
   assert((batch->encoder_current + AGX_VDM_STREAM_LINK_LENGTH) <=
                  /* Always leave room for a link tag, in case we run out of space later,
   * plus padding because VDM apparently overreads?
   *
   * 0x200 is not enough. 0x400 seems to work. 0x800 for safety.
   */
            /* If there is room in the command buffer, we're done */
   if (likely((batch->encoder_end - batch->encoder_current) >= space))
            /* Otherwise, we need to allocate a new command buffer. We use memory owned
   * by the batch to simplify lifetime management for the BO.
   */
   size_t size = 65536;
            /* Jump from the old command buffer to the new command buffer */
   agx_pack(batch->encoder_current, VDM_STREAM_LINK, cfg) {
      cfg.target_lo = T.gpu & BITFIELD_MASK(32);
               /* Swap out the command buffer */
   batch->encoder_current = T.cpu;
      }
      static void
   agx_draw_vbo(struct pipe_context *pctx, const struct pipe_draw_info *info,
               unsigned drawid_offset,
   {
               if (unlikely(!agx_render_condition_check(ctx)))
            if (num_draws > 1) {
      util_draw_multi(pctx, info, drawid_offset, indirect, draws, num_draws);
               if (indirect && indirect->count_from_stream_output) {
      agx_draw_vbo_from_xfb(pctx, info, drawid_offset, indirect);
               bool has_xfb_info = ctx->stage[PIPE_SHADER_VERTEX].shader->has_xfb_info;
   bool uses_xfb = has_xfb_info && ctx->streamout.num_targets;
            if (indirect && (uses_prims_generated || uses_xfb)) {
      perf_debug_ctx(ctx, "Emulating indirect draw due to XFB");
   util_draw_indirect(pctx, info, indirect);
               if (uses_xfb && info->primitive_restart) {
      perf_debug_ctx(ctx, "Emulating primitive restart due to XFB");
   util_draw_vbo_without_prim_restart(pctx, info, drawid_offset, indirect,
                     if (!ctx->streamout.key.active && uses_prims_generated) {
                  struct agx_batch *batch = agx_get_batch(ctx);
   unsigned idx_size = info->index_size;
   uint64_t ib = 0;
            if (idx_size) {
      if (indirect != NULL)
         else
               if (uses_xfb)
         #ifndef NDEBUG
      if (unlikely(agx_device(pctx->screen)->debug & AGX_DBG_DIRTY))
      #endif
         if (agx_scissor_culls_everything(ctx))
            /* We don't support side effects in vertex stages (only used internally for
   * transform feedback lowering), so this is trivial.
   */
   if (ctx->rast->base.rasterizer_discard && !ctx->streamout.key.active)
                     /* Dirty track the reduced prim: lines vs points vs triangles */
   enum mesa_prim reduced_prim = u_reduced_prim(info->mode);
   if (reduced_prim != batch->reduced_prim)
                  /* Update batch masks based on current state */
   if (ctx->dirty & AGX_DIRTY_BLEND) {
      /* TODO: Any point to tracking load? */
   batch->draw |= ctx->blend->store;
               if (ctx->dirty & AGX_DIRTY_ZS) {
      batch->load |= ctx->zs->load;
   batch->draw |= ctx->zs->store;
               if (agx_update_vs(ctx)) {
      ctx->dirty |= AGX_DIRTY_VS | AGX_DIRTY_VS_PROG;
      } else if (ctx->stage[PIPE_SHADER_VERTEX].dirty ||
                        struct agx_compiled_shader *vs = ctx->vs;
            if (agx_update_fs(batch)) {
      ctx->dirty |= AGX_DIRTY_FS | AGX_DIRTY_FS_PROG;
      } else if (ctx->stage[PIPE_SHADER_FRAGMENT].dirty ||
                        agx_batch_add_bo(batch, ctx->vs->bo);
            /* When we approach the end of a command buffer, cycle it out for a new one.
   * We only need to do this once per draw as long as we conservatively
   * estimate the maximum bytes of VDM commands that this draw will emit.
   */
   agx_ensure_cmdbuf_has_space(
      batch,
   (AGX_VDM_STATE_LENGTH * 2) + (AGX_PPP_STATE_LENGTH * MAX_PPP_UPDATES) +
      AGX_VDM_STATE_RESTART_INDEX_LENGTH +
   AGX_VDM_STATE_VERTEX_SHADER_WORD_0_LENGTH +
   AGX_VDM_STATE_VERTEX_SHADER_WORD_1_LENGTH +
   AGX_VDM_STATE_VERTEX_OUTPUTS_LENGTH +
   AGX_VDM_STATE_VERTEX_UNKNOWN_LENGTH + 4 /* padding */ +
   ((!batch->any_draws) ? AGX_VDM_BARRIER_LENGTH : 0) +
   AGX_INDEX_LIST_LENGTH + AGX_INDEX_LIST_BUFFER_LO_LENGTH +
            uint8_t *out = agx_encode_state(batch, batch->encoder_current,
                  enum agx_primitive prim = agx_primitive_for_pipe(info->mode);
   if (idx_size) {
      agx_pack(out, VDM_STATE, cfg)
                  agx_pack(out, VDM_STATE_RESTART_INDEX, cfg) {
         }
               if (!batch->any_draws) {
      agx_pack(out, VDM_BARRIER, cfg) {
         }
                        agx_pack(out, INDEX_LIST, cfg) {
      cfg.primitive = prim;
            if (indirect != NULL) {
         } else {
      cfg.index_count_present = true;
               if (idx_size) {
      cfg.restart_enable = info->primitive_restart;
   cfg.index_buffer_hi = (ib >> 32);
   cfg.index_size = agx_translate_index_size(idx_size);
   cfg.index_buffer_present = true;
         }
            if (idx_size) {
      agx_pack(out, INDEX_LIST_BUFFER_LO, cfg) {
         }
               if (!indirect) {
      agx_pack(out, INDEX_LIST_COUNT, cfg)
                     agx_pack(out, INDEX_LIST_INSTANCES, cfg)
                  if (indirect) {
      struct agx_resource *indirect_rsrc = agx_resource(indirect->buffer);
            agx_pack(out, INDEX_LIST_INDIRECT_BUFFER, cfg) {
      cfg.address_hi = address >> 32;
      }
      } else {
      agx_pack(out, INDEX_LIST_START, cfg) {
         }
               if (idx_size) {
      agx_pack(out, INDEX_LIST_BUFFER_SIZE, cfg) {
         }
               /* Insert a memory barrier after transform feedback so the result may be
   * consumed by a subsequent vertex shader.
   */
   if (ctx->streamout.key.active) {
      struct agx_device *dev = agx_device(pctx->screen);
   agx_pack(out, VDM_BARRIER, cfg) {
      cfg.unk_5 = true;
   cfg.unk_6 = true;
   cfg.unk_8 = true;
   cfg.unk_11 = true;
   cfg.unk_20 = true;
   if (dev->params.num_clusters_total > 1) {
      cfg.unk_24 = true;
   if (dev->params.gpu_generation == 13) {
      cfg.unk_4 = true;
                                 batch->encoder_current = out;
   assert((batch->encoder_current + AGX_VDM_STREAM_LINK_LENGTH) <=
                                          /* The scissor/zbias arrays are indexed with 16-bit integers, imposigin a
   * maximum of UINT16_MAX descriptors. Flush if the next draw would overflow
   */
   if (unlikely((batch->scissor.size / AGX_SCISSOR_LENGTH) >= UINT16_MAX) ||
      (batch->depth_bias.size / AGX_DEPTH_BIAS_LENGTH) >= UINT16_MAX) {
      } else if (unlikely(batch->draws > 100000)) {
      /* Mostly so drawoverhead doesn't OOM */
         }
      static void
   agx_texture_barrier(struct pipe_context *pipe, unsigned flags)
   {
               /* Framebuffer fetch is coherent, so barriers are a no-op. */
   if (flags == PIPE_TEXTURE_BARRIER_FRAMEBUFFER)
               }
      static void
   agx_launch_grid(struct pipe_context *pipe, const struct pipe_grid_info *info)
   {
      struct agx_context *ctx = agx_context(pipe);
   struct agx_batch *batch = agx_get_compute_batch(ctx);
                     /* Consider compute launches as "draws" for the purposes of sanity
   * checking batch state.
   */
            /* To implement load_num_workgroups, the number of workgroups needs to be
   * available in GPU memory. This is either the indirect buffer, or just a
   * buffer we upload ourselves if not indirect.
   */
   if (info->indirect) {
      struct agx_resource *indirect = agx_resource(info->indirect);
            batch->uniforms.tables[AGX_SYSVAL_TABLE_GRID] =
      } else {
      static_assert(sizeof(info->grid) == 12,
            batch->uniforms.tables[AGX_SYSVAL_TABLE_GRID] = agx_pool_upload_aligned(
               util_dynarray_foreach(&ctx->global_buffers, struct pipe_resource *, res) {
      if (!*res)
            struct agx_resource *buffer = agx_resource(*res);
               struct agx_uncompiled_shader *uncompiled =
            /* There is exactly one variant, get it */
   struct agx_compiled_shader *cs =
                     agx_update_descriptors(batch, cs, PIPE_SHADER_COMPUTE);
            /* TODO: Ensure space if we allow multiple kernels in a batch */
            agx_pack(out, CDM_HEADER, cfg) {
      if (info->indirect)
         else
            cfg.uniform_register_count = cs->info.push_count;
   cfg.preshader_register_count = cs->info.nr_preamble_gprs;
   cfg.texture_state_register_count =
         cfg.sampler_state_register_count =
         cfg.pipeline = agx_build_pipeline(batch, cs, PIPE_SHADER_COMPUTE,
      }
            /* Added in G14X */
   if (dev->params.gpu_generation >= 14 && dev->params.num_clusters_total > 1) {
      agx_pack(out, CDM_UNK_G14X, cfg)
                     if (info->indirect) {
      agx_pack(out, CDM_INDIRECT, cfg) {
      cfg.address_hi = batch->uniforms.tables[AGX_SYSVAL_TABLE_GRID] >> 32;
   cfg.address_lo =
      }
      } else {
      agx_pack(out, CDM_GLOBAL_SIZE, cfg) {
      cfg.x = info->grid[0] * info->block[0];
   cfg.y = info->grid[1] * info->block[1];
      }
               agx_pack(out, CDM_LOCAL_SIZE, cfg) {
      cfg.x = info->block[0];
   cfg.y = info->block[1];
      }
            agx_pack(out, CDM_LAUNCH, cfg)
                  batch->encoder_current = out;
   assert(batch->encoder_current <= batch->encoder_end &&
                  /* TODO: Allow multiple kernels in a batch? */
   agx_flush_batch_for_reason(ctx, batch, "Compute kernel serialization");
      }
      static void
   agx_set_global_binding(struct pipe_context *pipe, unsigned first,
               {
      struct agx_context *ctx = agx_context(pipe);
   unsigned old_size =
            if (old_size < first + count) {
      /* we are screwed no matter what */
   if (!util_dynarray_grow(&ctx->global_buffers, *resources,
                  for (unsigned i = old_size; i < first + count; i++)
      *util_dynarray_element(&ctx->global_buffers, struct pipe_resource *,
            for (unsigned i = 0; i < count; ++i) {
      struct pipe_resource **res = util_dynarray_element(
         if (resources && resources[i]) {
               /* The handle points to uint32_t, but space is allocated for 64
   * bits. We need to respect the offset passed in. This interface
   * is so bad.
   */
                  memcpy(&addr, handles[i], sizeof(addr));
   addr += rsrc->bo->ptr.gpu;
      } else {
               }
      void agx_init_state_functions(struct pipe_context *ctx);
      void
   agx_init_state_functions(struct pipe_context *ctx)
   {
      ctx->create_blend_state = agx_create_blend_state;
   ctx->create_depth_stencil_alpha_state = agx_create_zsa_state;
   ctx->create_fs_state = agx_create_shader_state;
   ctx->create_rasterizer_state = agx_create_rs_state;
   ctx->create_sampler_state = agx_create_sampler_state;
   ctx->create_sampler_view = agx_create_sampler_view;
   ctx->create_surface = agx_create_surface;
   ctx->create_vertex_elements_state = agx_create_vertex_elements;
   ctx->create_vs_state = agx_create_shader_state;
   ctx->create_compute_state = agx_create_compute_state;
   ctx->bind_blend_state = agx_bind_blend_state;
   ctx->bind_depth_stencil_alpha_state = agx_bind_zsa_state;
   ctx->bind_sampler_states = agx_bind_sampler_states;
   ctx->bind_fs_state = agx_bind_shader_state;
   ctx->bind_rasterizer_state = agx_bind_rasterizer_state;
   ctx->bind_vertex_elements_state = agx_bind_vertex_elements_state;
   ctx->bind_vs_state = agx_bind_shader_state;
   ctx->bind_compute_state = agx_bind_shader_state;
   ctx->delete_blend_state = agx_delete_state;
   ctx->delete_depth_stencil_alpha_state = agx_delete_state;
   ctx->delete_fs_state = agx_delete_shader_state;
   ctx->delete_compute_state = agx_delete_shader_state;
   ctx->delete_rasterizer_state = agx_delete_state;
   ctx->delete_sampler_state = agx_delete_sampler_state;
   ctx->delete_vertex_elements_state = agx_delete_state;
   ctx->delete_vs_state = agx_delete_shader_state;
   ctx->set_blend_color = agx_set_blend_color;
   ctx->set_clip_state = agx_set_clip_state;
   ctx->set_constant_buffer = agx_set_constant_buffer;
   ctx->set_shader_buffers = agx_set_shader_buffers;
   ctx->set_shader_images = agx_set_shader_images;
   ctx->set_sampler_views = agx_set_sampler_views;
   ctx->set_framebuffer_state = agx_set_framebuffer_state;
   ctx->set_polygon_stipple = agx_set_polygon_stipple;
   ctx->set_sample_mask = agx_set_sample_mask;
   ctx->set_scissor_states = agx_set_scissor_states;
   ctx->set_stencil_ref = agx_set_stencil_ref;
   ctx->set_vertex_buffers = agx_set_vertex_buffers;
   ctx->set_viewport_states = agx_set_viewport_states;
   ctx->sampler_view_destroy = agx_sampler_view_destroy;
   ctx->surface_destroy = agx_surface_destroy;
   ctx->draw_vbo = agx_draw_vbo;
   ctx->launch_grid = agx_launch_grid;
   ctx->set_global_binding = agx_set_global_binding;
   ctx->texture_barrier = agx_texture_barrier;
      }
