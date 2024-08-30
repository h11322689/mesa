   /*
   * Copyright © 2017 Intel Corporation
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included
   * in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      #include <stdio.h>
   #include <errno.h>
   #include "pipe/p_defines.h"
   #include "pipe/p_state.h"
   #include "pipe/p_context.h"
   #include "pipe/p_screen.h"
   #include "util/u_inlines.h"
   #include "util/u_surface.h"
   #include "util/format/u_format.h"
   #include "util/u_upload_mgr.h"
   #include "util/ralloc.h"
   #include "crocus_context.h"
   #include "crocus_resource.h"
   #include "crocus_screen.h"
   #include "intel/compiler/brw_compiler.h"
   #include "util/format_srgb.h"
      static bool
   crocus_is_color_fast_clear_compatible(struct crocus_context *ice,
               {
      if (isl_format_has_int_channel(format)) {
      perf_debug(&ice->dbg, "Integer fast clear not enabled for %s",
                     for (int i = 0; i < 4; i++) {
      if (!isl_format_has_color_component(format, i)) {
                  if (color.f32[i] != 0.0f && color.f32[i] != 1.0f) {
                        }
      static bool
   can_fast_clear_color(struct crocus_context *ice,
                        struct pipe_resource *p_res,
   unsigned level,
   const struct pipe_box *box,
      {
               if (INTEL_DEBUG(DEBUG_NO_FAST_CLEAR))
            if (!isl_aux_usage_has_fast_clears(res->aux.usage))
            /* Check for partial clear */
   if (box->x > 0 || box->y > 0 ||
      box->width < u_minify(p_res->width0, level) ||
   box->height < u_minify(p_res->height0, level)) {
               /* Avoid conditional fast clears to maintain correct tracking of the aux
   * state (see iris_resource_finish_write for more info). Note that partial
   * fast clears (if they existed) would not pose a problem with conditional
   * rendering.
   */
   if (render_condition_enabled &&
      ice->state.predicate == CROCUS_PREDICATE_STATE_USE_BIT) {
               /* We store clear colors as floats or uints as needed.  If there are
   * texture views in play, the formats will not properly be respected
   * during resolves because the resolve operations only know about the
   * resource and not the renderbuffer.
   */
   if (!crocus_render_formats_color_compatible(render_format, res->surf.format,
                        /* XXX: if (irb->mt->supports_fast_clear)
   * see intel_miptree_create_for_dri_image()
            if (!crocus_is_color_fast_clear_compatible(ice, format, color))
               }
      static union isl_color_value
   convert_fast_clear_color(struct crocus_context *ice,
                     {
      union isl_color_value override_color = color;
            const enum pipe_format format = p_res->format;
   const struct util_format_description *desc =
                  if (util_format_is_intensity(format) ||
      util_format_is_luminance(format) ||
   util_format_is_luminance_alpha(format)) {
   override_color.u32[1] = override_color.u32[0];
   override_color.u32[2] = override_color.u32[0];
   if (util_format_is_intensity(format))
      } else {
      for (int chan = 0; chan < 3; chan++) {
      if (!(colormask & (1 << chan)))
                  if (util_format_is_unorm(format)) {
      for (int i = 0; i < 4; i++)
      } else if (util_format_is_snorm(format)) {
      for (int i = 0; i < 4; i++)
      } else if (util_format_is_pure_uint(format)) {
      for (int i = 0; i < 4; i++) {
      unsigned bits = util_format_get_component_bits(
         if (bits < 32) {
      uint32_t max = (1u << bits) - 1;
            } else if (util_format_is_pure_sint(format)) {
      for (int i = 0; i < 4; i++) {
      unsigned bits = util_format_get_component_bits(
         if (bits < 32) {
      int32_t max = (1 << (bits - 1)) - 1;
   int32_t min = -(1 << (bits - 1));
            } else if (format == PIPE_FORMAT_R11G11B10_FLOAT ||
            /* these packed float formats only store unsigned values */
   for (int i = 0; i < 4; i++)
               if (!(colormask & 1 << 3)) {
      if (util_format_is_pure_integer(format))
         else
               /* Handle linear to SRGB conversion */
   if (isl_format_is_srgb(render_format)) {
      for (int i = 0; i < 3; i++) {
      override_color.f32[i] =
                     }
      static void
   fast_clear_color(struct crocus_context *ice,
                  struct crocus_resource *res,
   unsigned level,
   const struct pipe_box *box,
      {
      struct crocus_batch *batch = &ice->batches[CROCUS_BATCH_RENDER];
   struct crocus_screen *screen = batch->screen;
                     bool color_changed = !!memcmp(&res->aux.clear_color, &color,
            if (color_changed) {
      /* If we are clearing to a new clear value, we need to resolve fast
   * clears from other levels/layers first, since we can't have different
   * levels/layers with different fast clear colors.
   */
   for (unsigned res_lvl = 0; res_lvl < res->surf.levels; res_lvl++) {
      const unsigned level_layers =
         for (unsigned layer = 0; layer < level_layers; layer++) {
      if (res_lvl == level &&
      layer >= box->z &&
   layer < box->z + box->depth) {
                                    if (aux_state != ISL_AUX_STATE_CLEAR &&
      aux_state != ISL_AUX_STATE_PARTIAL_CLEAR &&
   aux_state != ISL_AUX_STATE_COMPRESSED_CLEAR) {
                     /* If we got here, then the level may have fast-clear bits that use
   * the old clear value.  We need to do a color resolve to get rid
   * of their use of the clear color before we can change it.
   * Fortunately, few applications ever change their clear color at
   * different levels/layers, so this shouldn't happen often.
   */
   crocus_resource_prepare_access(ice, res,
                     perf_debug(&ice->dbg,
            "Resolving resource (%p) level %d, layer %d: color changing from "
   "(%0.2f, %0.2f, %0.2f, %0.2f) to "
   "(%0.2f, %0.2f, %0.2f, %0.2f)\n",
   res, res_lvl, layer,
   res->aux.clear_color.f32[0],
   res->aux.clear_color.f32[1],
   res->aux.clear_color.f32[2],
                           /* If the buffer is already in ISL_AUX_STATE_CLEAR, and the color hasn't
   * changed, the clear is redundant and can be skipped.
   */
   const enum isl_aux_state aux_state =
         if (!color_changed && box->depth == 1 && aux_state == ISL_AUX_STATE_CLEAR)
            /* Ivybrigde PRM Vol 2, Part 1, "11.7 MCS Buffer for Render Target(s)":
   *
   *    "Any transition from any value in {Clear, Render, Resolve} to a
   *    different value in {Clear, Render, Resolve} requires end of pipe
   *    synchronization."
   *
   * In other words, fast clear ops are not properly synchronized with
   * other drawing.  We need to use a PIPE_CONTROL to ensure that the
   * contents of the previous draw hit the render target before we resolve
   * and again afterwards to ensure that the resolve is complete before we
   * do any more regular drawing.
   */
   crocus_emit_end_of_pipe_sync(batch,
                  /* If we reach this point, we need to fast clear to change the state to
   * ISL_AUX_STATE_CLEAR, or to update the fast clear color (or both).
   */
            struct blorp_batch blorp_batch;
            struct blorp_surf surf;
   crocus_blorp_surf_for_resource(&screen->vtbl, &batch->screen->isl_dev, &surf,
            /* In newer gens (> 9), the hardware will do a linear -> sRGB conversion of
   * the clear color during the fast clear, if the surface format is of sRGB
   * type. We use the linear version of the surface format here to prevent
   * that from happening, since we already do our own linear -> sRGB
   * conversion in convert_fast_clear_color().
   */
   blorp_fast_clear(&blorp_batch, &surf, isl_format_srgb_to_linear(format),
                  ISL_SWIZZLE_IDENTITY,
      blorp_batch_finish(&blorp_batch);
   crocus_emit_end_of_pipe_sync(batch,
                  crocus_resource_set_aux_state(ice, res, level, box->z,
         ice->state.stage_dirty |= CROCUS_ALL_STAGE_DIRTY_BINDINGS;
      }
      static void
   clear_color(struct crocus_context *ice,
               struct pipe_resource *p_res,
   unsigned level,
   const struct pipe_box *box,
   bool render_condition_enabled,
   enum isl_format format,
   {
      struct crocus_resource *res = (void *) p_res;
   struct crocus_batch *batch = &ice->batches[CROCUS_BATCH_RENDER];
   struct crocus_screen *screen = batch->screen;
   const struct intel_device_info *devinfo = &batch->screen->devinfo;
            if (render_condition_enabled) {
      if (!crocus_check_conditional_render(ice))
            if (ice->state.predicate == CROCUS_PREDICATE_STATE_USE_BIT)
               if (p_res->target == PIPE_BUFFER)
                     bool can_fast_clear = can_fast_clear_color(ice, p_res, level, box,
               if (can_fast_clear) {
      fast_clear_color(ice, res, level, box, format, color,
                     enum isl_aux_usage aux_usage =
            crocus_resource_prepare_render(ice, res, level,
            struct blorp_surf surf;
   crocus_blorp_surf_for_resource(&screen->vtbl, &batch->screen->isl_dev, &surf,
            struct blorp_batch blorp_batch;
            if (!isl_format_supports_rendering(devinfo, format) &&
      isl_format_is_rgbx(format))
         blorp_clear(&blorp_batch, &surf, format, swizzle,
                        blorp_batch_finish(&blorp_batch);
   crocus_flush_and_dirty_for_history(ice, batch, res,
                  crocus_resource_finish_render(ice, res, level,
      }
      static bool
   can_fast_clear_depth(struct crocus_context *ice,
                        struct crocus_resource *res,
      {
      struct pipe_resource *p_res = (void *) res;
   struct pipe_context *ctx = (void *) ice;
   struct crocus_screen *screen = (void *) ctx->screen;
            if (devinfo->ver < 6)
            if (INTEL_DEBUG(DEBUG_NO_FAST_CLEAR))
            /* Check for partial clears */
   if (box->x > 0 || box->y > 0 ||
      box->width < u_minify(p_res->width0, level) ||
   box->height < u_minify(p_res->height0, level)) {
               /* Avoid conditional fast clears to maintain correct tracking of the aux
   * state (see iris_resource_finish_write for more info). Note that partial
   * fast clears would not pose a problem with conditional rendering.
   */
   if (render_condition_enabled &&
      ice->state.predicate == CROCUS_PREDICATE_STATE_USE_BIT) {
               if (!crocus_resource_level_has_hiz(res, level))
            if (res->base.b.format == PIPE_FORMAT_Z16_UNORM) {
      /* From the Sandy Bridge PRM, volume 2 part 1, page 314:
   *
   *     "[DevSNB+]: Several cases exist where Depth Buffer Clear cannot be
   *      enabled (the legacy method of clearing must be performed):
   *
   *      - DevSNB{W/A}]: When depth buffer format is D16_UNORM and the
   *        width of the map (LOD0) is not multiple of 16, fast clear
   *        optimization must be disabled.
   */
   if (devinfo->ver == 6 &&
      (u_minify(res->surf.phys_level0_sa.width,
         }
      }
      static void
   fast_clear_depth(struct crocus_context *ice,
                  struct crocus_resource *res,
      {
                        /* If we're clearing to a new clear value, then we need to resolve any clear
   * flags out of the HiZ buffer into the real depth buffer.
   */
   if (res->aux.clear_color.f32[0] != depth) {
      for (unsigned res_level = 0; res_level < res->surf.levels; res_level++) {
                     const unsigned level_layers =
         for (unsigned layer = 0; layer < level_layers; layer++) {
      if (res_level == level &&
      layer >= box->z &&
   layer < box->z + box->depth) {
                                    if (aux_state != ISL_AUX_STATE_CLEAR &&
      aux_state != ISL_AUX_STATE_COMPRESSED_CLEAR) {
                     /* If we got here, then the level may have fast-clear bits that
   * use the old clear value.  We need to do a depth resolve to get
   * rid of their use of the clear value before we can change it.
   * Fortunately, few applications ever change their depth clear
   * value so this shouldn't happen often.
   */
   crocus_hiz_exec(ice, batch, res, res_level, layer, 1,
         crocus_resource_set_aux_state(ice, res, res_level, layer, 1,
         }
   const union isl_color_value clear_value = { .f32 = {depth, } };
   crocus_resource_set_clear_color(ice, res, clear_value);
               for (unsigned l = 0; l < box->depth; l++) {
      enum isl_aux_state aux_state =
      crocus_resource_level_has_hiz(res, level) ?
   crocus_resource_get_aux_state(res, level, box->z + l) :
      if (update_clear_depth || aux_state != ISL_AUX_STATE_CLEAR) {
      if (aux_state == ISL_AUX_STATE_CLEAR) {
      perf_debug(&ice->dbg, "Performing HiZ clear just to update the "
      }
   crocus_hiz_exec(ice, batch, res, level,
                        crocus_resource_set_aux_state(ice, res, level, box->z, box->depth,
            }
      static void
   clear_depth_stencil(struct crocus_context *ice,
                     struct pipe_resource *p_res,
   unsigned level,
   const struct pipe_box *box,
   bool render_condition_enabled,
   bool clear_depth,
   {
      struct crocus_resource *res = (void *) p_res;
   struct crocus_batch *batch = &ice->batches[CROCUS_BATCH_RENDER];
   struct crocus_screen *screen = batch->screen;
            if (render_condition_enabled) {
      if (!crocus_check_conditional_render(ice))
            if (ice->state.predicate == CROCUS_PREDICATE_STATE_USE_BIT)
                        struct crocus_resource *z_res;
   struct crocus_resource *stencil_res;
   struct blorp_surf z_surf;
            crocus_get_depth_stencil_resources(&batch->screen->devinfo, p_res, &z_res, &stencil_res);
   if (z_res && clear_depth &&
      can_fast_clear_depth(ice, z_res, level, box, render_condition_enabled,
         fast_clear_depth(ice, z_res, level, box, depth);
   crocus_flush_and_dirty_for_history(ice, batch, res, 0,
         clear_depth = false;
               /* At this point, we might have fast cleared the depth buffer. So if there's
   * no stencil clear pending, return early.
   */
   if (!(clear_depth || (clear_stencil && stencil_res))) {
                  if (clear_depth && z_res) {
      const enum isl_aux_usage aux_usage =
      crocus_resource_render_aux_usage(ice, z_res, level, z_res->surf.format,
      crocus_resource_prepare_render(ice, z_res, level, box->z, box->depth,
         crocus_blorp_surf_for_resource(&screen->vtbl, &batch->screen->isl_dev,
                     struct blorp_batch blorp_batch;
            uint8_t stencil_mask = clear_stencil && stencil_res ? 0xff : 0;
   if (stencil_mask) {
      crocus_resource_prepare_access(ice, stencil_res, level, 1, box->z,
         crocus_blorp_surf_for_resource(&screen->vtbl, &batch->screen->isl_dev,
                     blorp_clear_depth_stencil(&blorp_batch, &z_surf, &stencil_surf,
                           level, box->z, box->depth,
            blorp_batch_finish(&blorp_batch);
   crocus_flush_and_dirty_for_history(ice, batch, res, 0,
            if (clear_depth && z_res) {
      crocus_resource_finish_render(ice, z_res, level,
               if (stencil_mask) {
      crocus_resource_finish_write(ice, stencil_res, level, box->z, box->depth,
         }
      /**
   * The pipe->clear() driver hook.
   *
   * This clears buffers attached to the current draw framebuffer.
   */
   static void
   crocus_clear(struct pipe_context *ctx,
               unsigned buffers,
   const struct pipe_scissor_state *scissor_state,
   const union pipe_color_union *p_color,
   {
      struct crocus_context *ice = (void *) ctx;
   struct pipe_framebuffer_state *cso_fb = &ice->state.framebuffer;
   struct crocus_screen *screen = (void *) ctx->screen;
   const struct intel_device_info *devinfo = &screen->devinfo;
            struct pipe_box box = {
      .width = cso_fb->width,
               if (scissor_state) {
      box.x = scissor_state->minx;
   box.y = scissor_state->miny;
   box.width = MIN2(box.width, scissor_state->maxx - scissor_state->minx);
               if (buffers & PIPE_CLEAR_DEPTHSTENCIL) {
      if (devinfo->ver < 6) {
      crocus_blitter_begin(ice, CROCUS_SAVE_FRAGMENT_STATE, true);
   util_blitter_clear(ice->blitter, cso_fb->width, cso_fb->height,
            } else {
      struct pipe_surface *psurf = cso_fb->zsbuf;
                  clear_depth_stencil(ice, psurf->texture, psurf->u.tex.level, &box, true,
                  }
               if (buffers & PIPE_CLEAR_COLOR) {
      /* pipe_color_union and isl_color_value are interchangeable */
            for (unsigned i = 0; i < cso_fb->nr_cbufs; i++) {
      if (buffers & (PIPE_CLEAR_COLOR0 << i)) {
      struct pipe_surface *psurf = cso_fb->cbufs[i];
   struct crocus_surface *isurf = (void *) psurf;
                  clear_color(ice, psurf->texture, psurf->u.tex.level, &box,
                     }
      /**
   * The pipe->clear_texture() driver hook.
   *
   * This clears the given texture resource.
   */
   static void
   crocus_clear_texture(struct pipe_context *ctx,
                           {
      struct crocus_context *ice = (void *) ctx;
   struct crocus_screen *screen = (void *) ctx->screen;
            if (devinfo->ver < 6) {
      u_default_clear_texture(ctx, p_res, level, box, data);
               if (util_format_is_depth_or_stencil(p_res->format)) {
      const struct util_format_unpack_description *fmt_unpack =
            float depth = 0.0;
            if (fmt_unpack->unpack_z_float)
            if (fmt_unpack->unpack_s_8uint)
            clear_depth_stencil(ice, p_res, level, box, true, true, true,
      } else {
      union isl_color_value color;
   struct crocus_resource *res = (void *) p_res;
            if (!isl_format_supports_rendering(devinfo, format)) {
      const struct isl_format_layout *fmtl = isl_format_get_layout(format);
   // XXX: actually just get_copy_format_for_bpb from BLORP
   // XXX: don't cut and paste this
   switch (fmtl->bpb) {
   case 8:   format = ISL_FORMAT_R8_UINT;           break;
   case 16:  format = ISL_FORMAT_R8G8_UINT;         break;
   case 24:  format = ISL_FORMAT_R8G8B8_UINT;       break;
   case 32:  format = ISL_FORMAT_R8G8B8A8_UINT;     break;
   case 48:  format = ISL_FORMAT_R16G16B16_UINT;    break;
   case 64:  format = ISL_FORMAT_R16G16B16A16_UINT; break;
   case 96:  format = ISL_FORMAT_R32G32B32_UINT;    break;
   case 128: format = ISL_FORMAT_R32G32B32A32_UINT; break;
   default:
                  /* No aux surfaces for non-renderable surfaces */
                        clear_color(ice, p_res, level, box, true, format,
         }
      /**
   * The pipe->clear_render_target() driver hook.
   *
   * This clears the given render target surface.
   */
   static void
   crocus_clear_render_target(struct pipe_context *ctx,
                                 {
      struct crocus_context *ice = (void *) ctx;
   struct crocus_surface *isurf = (void *) psurf;
   struct pipe_box box = {
      .x = dst_x,
   .y = dst_y,
   .z = psurf->u.tex.first_layer,
   .width = width,
   .height = height,
               /* pipe_color_union and isl_color_value are interchangeable */
            clear_color(ice, psurf->texture, psurf->u.tex.level, &box,
            }
      /**
   * The pipe->clear_depth_stencil() driver hook.
   *
   * This clears the given depth/stencil surface.
   */
   static void
   crocus_clear_depth_stencil(struct pipe_context *ctx,
                              struct pipe_surface *psurf,
   unsigned flags,
      {
         #if 0
      struct crocus_context *ice = (void *) ctx;
   struct pipe_box box = {
      .x = dst_x,
   .y = dst_y,
   .z = psurf->u.tex.first_layer,
   .width = width,
   .height = height,
      };
                     crocus_blitter_begin(ice, CROCUS_SAVE_FRAGMENT_STATE);
   util_blitter_clear(ice->blitter, width, height,
      #if 0
      clear_depth_stencil(ice, psurf->texture, psurf->u.tex.level, &box,
                  #endif
   #endif
   }
      void
   crocus_init_clear_functions(struct pipe_context *ctx)
   {
      ctx->clear = crocus_clear;
   ctx->clear_texture = crocus_clear_texture;
   ctx->clear_render_target = crocus_clear_render_target;
      }
