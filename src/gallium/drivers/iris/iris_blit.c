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
   #include "pipe/p_defines.h"
   #include "pipe/p_state.h"
   #include "pipe/p_context.h"
   #include "pipe/p_screen.h"
   #include "util/format/u_format.h"
   #include "util/u_inlines.h"
   #include "util/u_surface.h"
   #include "util/ralloc.h"
   #include "intel/blorp/blorp.h"
   #include "iris_context.h"
   #include "iris_resource.h"
   #include "iris_screen.h"
      /**
   * Helper function for handling mirror image blits.
   *
   * If coord0 > coord1, swap them and return "true" (mirrored).
   */
   static bool
   apply_mirror(float *coord0, float *coord1)
   {
      if (*coord0 > *coord1) {
      float tmp = *coord0;
   *coord0 = *coord1;
   *coord1 = tmp;
      }
      }
      /**
   * Compute the number of pixels to clip for each side of a rect
   *
   * \param x0 The rect's left coordinate
   * \param y0 The rect's bottom coordinate
   * \param x1 The rect's right coordinate
   * \param y1 The rect's top coordinate
   * \param min_x The clipping region's left coordinate
   * \param min_y The clipping region's bottom coordinate
   * \param max_x The clipping region's right coordinate
   * \param max_y The clipping region's top coordinate
   * \param clipped_x0 The number of pixels to clip from the left side
   * \param clipped_y0 The number of pixels to clip from the bottom side
   * \param clipped_x1 The number of pixels to clip from the right side
   * \param clipped_y1 The number of pixels to clip from the top side
   *
   * \return false if we clip everything away, true otherwise
   */
   static inline bool
   compute_pixels_clipped(float x0, float y0, float x1, float y1,
                     {
      /* If we are going to clip everything away, stop. */
   if (!(min_x <= max_x &&
         min_y <= max_y &&
   x0 <= max_x &&
   y0 <= max_y &&
   min_x <= x1 &&
   min_y <= y1 &&
   x0 <= x1 &&
                  if (x0 < min_x)
         else
         if (max_x < x1)
         else
            if (y0 < min_y)
         else
         if (max_y < y1)
         else
               }
      /**
   * Clips a coordinate (left, right, top or bottom) for the src or dst rect
   * (whichever requires the largest clip) and adjusts the coordinate
   * for the other rect accordingly.
   *
   * \param mirror true if mirroring is required
   * \param src the source rect coordinate (for example src_x0)
   * \param dst0 the dst rect coordinate (for example dst_x0)
   * \param dst1 the opposite dst rect coordinate (for example dst_x1)
   * \param clipped_dst0 number of pixels to clip from the dst coordinate
   * \param clipped_dst1 number of pixels to clip from the opposite dst coordinate
   * \param scale the src vs dst scale involved for that coordinate
   * \param is_left_or_bottom true if we are clipping the left or bottom sides
   *        of the rect.
   */
   static void
   clip_coordinates(bool mirror,
                  float *src, float *dst0, float *dst1,
   float clipped_dst0,
      {
      /* When clipping we need to add or subtract pixels from the original
   * coordinates depending on whether we are acting on the left/bottom
   * or right/top sides of the rect respectively. We assume we have to
   * add them in the code below, and multiply by -1 when we should
   * subtract.
   */
            if (!mirror) {
      *dst0 += clipped_dst0 * mult;
      } else {
      *dst1 -= clipped_dst1 * mult;
         }
      /**
   * Apply a scissor rectangle to blit coordinates.
   *
   * Returns true if the blit was entirely scissored away.
   */
   static bool
   apply_blit_scissor(const struct pipe_scissor_state *scissor,
                     float *src_x0, float *src_y0,
   float *src_x1, float *src_y1,
   {
               /* Compute number of pixels to scissor away. */
   if (!compute_pixels_clipped(*dst_x0, *dst_y0, *dst_x1, *dst_y1,
                                             /* When clipping any of the two rects we need to adjust the coordinates
   * in the other rect considering the scaling factor involved.  To obtain
   * the best precision we want to make sure that we only clip once per
   * side to avoid accumulating errors due to the scaling adjustment.
   *
   * For example, if src_x0 and dst_x0 need both to be clipped we want to
   * avoid the situation where we clip src_x0 first, then adjust dst_x0
   * accordingly but then we realize that the resulting dst_x0 still needs
   * to be clipped, so we clip dst_x0 and adjust src_x0 again.  Because we are
   * applying scaling factors to adjust the coordinates in each clipping
   * pass we lose some precision and that can affect the results of the
   * blorp blit operation slightly.  What we want to do here is detect the
   * rect that we should clip first for each side so that when we adjust
   * the other rect we ensure the resulting coordinate does not need to be
   * clipped again.
   *
   * The code below implements this by comparing the number of pixels that
   * we need to clip for each side of both rects considering the scales
   * involved.  For example, clip_src_x0 represents the number of pixels
   * to be clipped for the src rect's left side, so if clip_src_x0 = 5,
   * clip_dst_x0 = 4 and scale_x = 2 it means that we are clipping more
   * from the dst rect so we should clip dst_x0 only and adjust src_x0.
   * This is because clipping 4 pixels in the dst is equivalent to
   * clipping 4 * 2 = 8 > 5 in the src.
            if (*src_x0 == *src_x1 || *src_y0 == *src_y1
      || *dst_x0 == *dst_x1 || *dst_y0 == *dst_y1)
         float scale_x = (float) (*src_x1 - *src_x0) / (*dst_x1 - *dst_x0);
            /* Clip left side */
   clip_coordinates(mirror_x, src_x0, dst_x0, dst_x1,
            /* Clip right side */
   clip_coordinates(mirror_x, src_x1, dst_x1, dst_x0,
            /* Clip bottom side */
   clip_coordinates(mirror_y, src_y0, dst_y0, dst_y1,
            /* Clip top side */
   clip_coordinates(mirror_y, src_y1, dst_y1, dst_y0,
            /* Check for invalid bounds
   * Can't blit for 0-dimensions
   */
   return *src_x0 == *src_x1 || *src_y0 == *src_y1
      }
      void
   iris_blorp_surf_for_resource(struct isl_device *isl_dev,
                                 {
      struct iris_resource *res = (void *) p_res;
            *surf = (struct blorp_surf) {
      .surf = &res->surf,
   .addr = (struct blorp_address) {
      .buffer = res->bo,
   .offset = res->offset,
   .reloc_flags = is_dest ? IRIS_BLORP_RELOC_FLAGS_EXEC_OBJECT_WRITE : 0,
   .mocs = iris_mocs(res->bo, isl_dev,
                  },
               if (aux_usage != ISL_AUX_USAGE_NONE) {
      surf->aux_surf = &res->aux.surf;
   surf->aux_addr = (struct blorp_address) {
      .buffer = res->aux.bo,
   .offset = res->aux.offset,
   .reloc_flags = is_dest ? IRIS_BLORP_RELOC_FLAGS_EXEC_OBJECT_WRITE : 0,
   .mocs = iris_mocs(res->bo, isl_dev, 0),
   .local_hint = devinfo->has_flat_ccs ||
      };
   surf->clear_color = res->aux.clear_color;
   surf->clear_color_addr = (struct blorp_address) {
      .buffer = res->aux.clear_color_bo,
   .offset = res->aux.clear_color_offset,
   .reloc_flags = 0,
   .mocs = iris_mocs(res->aux.clear_color_bo, isl_dev, 0),
   .local_hint = devinfo->has_flat_ccs ||
            }
      static bool
   is_astc(enum isl_format format)
   {
         }
      static void
   tex_cache_flush_hack(struct iris_batch *batch,
               {
               /* The WaSamplerCacheFlushBetweenRedescribedSurfaceReads workaround says:
   *
   *    "Currently Sampler assumes that a surface would not have two
   *     different format associate with it.  It will not properly cache
   *     the different views in the MT cache, causing a data corruption."
   *
   * We may need to handle this for texture views in general someday, but
   * for now we handle it here, as it hurts copies and blits particularly
   * badly because they ofter reinterpret formats.
   *
   * If the BO hasn't been referenced yet this batch, we assume that the
   * texture cache doesn't contain any relevant data nor need flushing.
   *
   * Icelake (Gfx11+) claims to fix this issue, but seems to still have
   * issues with ASTC formats.
   */
   bool need_flush = devinfo->ver >= 11 ?
               if (!need_flush)
            const char *reason =
            iris_emit_pipe_control_flush(batch, reason, PIPE_CONTROL_CS_STALL);
   iris_emit_pipe_control_flush(batch, reason,
      }
      static struct iris_resource *
   iris_resource_for_aspect(struct pipe_resource *p_res, unsigned pipe_mask)
   {
      if (pipe_mask == PIPE_MASK_S) {
      struct iris_resource *junk, *s_res;
   iris_get_depth_stencil_resources(p_res, &junk, &s_res);
      } else {
            }
      static enum pipe_format
   pipe_format_for_aspect(enum pipe_format format, unsigned pipe_mask)
   {
      if (pipe_mask == PIPE_MASK_S) {
         } else if (pipe_mask == PIPE_MASK_Z) {
         } else {
            }
      static bool
   clear_color_is_fully_zero(const struct iris_resource *res)
   {
      return !res->aux.clear_color_unknown &&
         res->aux.clear_color.u32[0] == 0 &&
   res->aux.clear_color.u32[1] == 0 &&
      }
      /**
   * The pipe->blit() driver hook.
   *
   * This performs a blit between two surfaces, which copies data but may
   * also perform format conversion, scaling, flipping, and so on.
   */
   static void
   iris_blit(struct pipe_context *ctx, const struct pipe_blit_info *info)
   {
      struct iris_context *ice = (void *) ctx;
   struct iris_screen *screen = (struct iris_screen *)ctx->screen;
   const struct intel_device_info *devinfo = screen->devinfo;
   struct iris_batch *batch = &ice->batches[IRIS_BATCH_RENDER];
            /* We don't support color masking. */
   assert((info->mask & PIPE_MASK_RGBA) == PIPE_MASK_RGBA ||
            if (info->render_condition_enable) {
      if (ice->state.predicate == IRIS_PREDICATE_STATE_DONT_RENDER)
            if (ice->state.predicate == IRIS_PREDICATE_STATE_USE_BIT)
               float src_x0 = info->src.box.x;
   float src_x1 = info->src.box.x + info->src.box.width;
   float src_y0 = info->src.box.y;
   float src_y1 = info->src.box.y + info->src.box.height;
   float dst_x0 = info->dst.box.x;
   float dst_x1 = info->dst.box.x + info->dst.box.width;
   float dst_y0 = info->dst.box.y;
   float dst_y1 = info->dst.box.y + info->dst.box.height;
   bool mirror_x = apply_mirror(&src_x0, &src_x1);
   bool mirror_y = apply_mirror(&src_y0, &src_y1);
            if (info->scissor_enable) {
      bool noop = apply_blit_scissor(&info->scissor,
                     if (noop)
               /* Do DRI PRIME blits on the hardware blitter on Gfx12+ */
   if (devinfo->ver >= 12 &&
      (info->dst.resource->bind & PIPE_BIND_PRIME_BLIT_DST)) {
   assert(!info->render_condition_enable);
   assert(util_can_blit_via_copy_region(info, false, false));
   iris_copy_region(&ice->blorp, &ice->batches[IRIS_BATCH_BLITTER],
                  info->dst.resource, info->dst.level,
                  if (abs(info->dst.box.width) == abs(info->src.box.width) &&
      abs(info->dst.box.height) == abs(info->src.box.height)) {
   if (info->src.resource->nr_samples > 1 &&
      info->dst.resource->nr_samples <= 1) {
   /* The OpenGL ES 3.2 specification, section 16.2.1, says:
   *
   *    "If the read framebuffer is multisampled (its effective
   *     value of SAMPLE_BUFFERS is one) and the draw framebuffer
   *     is not (its value of SAMPLE_BUFFERS is zero), the samples
   *     corresponding to each pixel location in the source are
   *     converted to a single sample before being written to the
   *     destination.  The filter parameter is ignored.  If the
   *     source formats are integer types or stencil values, a
   *     single sample’s value is selected for each pixel.  If the
   *     source formats are floating-point or normalized types,
   *     the sample values for each pixel are resolved in an
   *     implementation-dependent manner.  If the source formats
   *     are depth values, sample values are resolved in an
   *     implementation-dependent manner where the result will be
   *     between the minimum and maximum depth values in the pixel."
   *
   * When selecting a single sample, we always choose sample 0.
   */
   if (util_format_is_depth_or_stencil(info->src.format) ||
      util_format_is_pure_integer(info->src.format)) {
      } else {
            } else {
      /* The OpenGL 4.6 specification, section 18.3.1, says:
   *
   *    "If the source and destination dimensions are identical,
   *     no filtering is applied."
   *
   * Using BLORP_FILTER_NONE will also handle the upsample case by
   * replicating the one value in the source to all values in the
   * destination.
   */
         } else if (info->filter == PIPE_TEX_FILTER_LINEAR) {
         } else {
                  struct blorp_batch blorp_batch;
                     /* There is no interpolation to the pixel center during rendering, so
   * add the 0.5 offset ourselves here.
   */
   float depth_center_offset = 0;
   if (info->src.resource->target == PIPE_TEXTURE_3D)
            /* Perform a blit for each aspect requested by the caller. PIPE_MASK_R is
   * used to represent the color aspect. */
   unsigned aspect_mask = info->mask & (PIPE_MASK_R | PIPE_MASK_ZS);
   while (aspect_mask) {
               struct iris_resource *src_res =
         struct iris_resource *dst_res =
            enum pipe_format src_pfmt =
         enum pipe_format dst_pfmt =
            struct iris_format_info src_fmt =
         enum isl_aux_usage src_aux_usage =
                  iris_resource_prepare_texture(ice, src_res, src_fmt.fmt,
               iris_emit_buffer_barrier_for(batch, src_res->bo,
            struct iris_format_info dst_fmt =
      iris_format_for_usage(devinfo, dst_pfmt,
      enum isl_aux_usage dst_aux_usage =
                  iris_resource_prepare_render(ice, dst_res, dst_fmt.fmt, info->dst.level,
               iris_emit_buffer_barrier_for(batch, dst_res->bo,
            struct blorp_surf src_surf, dst_surf;
   iris_blorp_surf_for_resource(&screen->isl_dev,  &src_surf,
               iris_blorp_surf_for_resource(&screen->isl_dev, &dst_surf,
                  if (iris_batch_references(batch, src_res->bo))
            if (dst_res->base.b.target == PIPE_BUFFER) {
      util_range_add(&dst_res->base.b, &dst_res->valid_buffer_range,
               for (int slice = 0; slice < info->dst.box.depth; slice++) {
      unsigned dst_z = info->dst.box.z + slice;
                                 blorp_blit(&blorp_batch,
            &src_surf, info->src.level, src_z,
   src_fmt.fmt, src_fmt.swizzle,
   &dst_surf, info->dst.level, dst_z,
   dst_fmt.fmt, dst_fmt.swizzle,
                                    iris_resource_finish_render(ice, dst_res, info->dst.level,
                                 }
      static enum isl_aux_usage
   copy_region_aux_usage(struct iris_context *ice,
                        const struct iris_batch *batch,
      {
      struct iris_screen *screen = (void *) ice->ctx.screen;
            if (batch->name == IRIS_BATCH_RENDER) {
      if (is_dest) {
      return iris_resource_render_aux_usage(ice, res, view_format, level,
      } else {
      return iris_resource_texture_aux_usage(ice, res, view_format, level,
         } else {
                           /* We only blit to images created with PIPE_BIND_PRIME_BLIT_DST.
   * These are only created with dri3_alloc_render_buffer. That
   * function also makes them linear, so they lack compression.
   *
   * Note: this code block could be substituted with the one below, but
   * this setup clarifies that no additional handling of FCV is
   * necessary.
   */
   assert(res->base.b.bind & PIPE_BIND_PRIME_BLIT_DST);
                     } else {
               }
      static void
   prepare_copy_region(struct iris_context *ice,
                     const struct iris_batch *batch,
   struct iris_resource *res,
   enum isl_format view_format,
   uint32_t level,
   {
      if (batch->name == IRIS_BATCH_RENDER) {
      if (is_dest) {
      iris_resource_prepare_render(ice, res, view_format, level,
      } else {
      iris_resource_prepare_texture(ice, res, view_format, level, 1,
         } else {
      assert(batch->name == IRIS_BATCH_BLITTER);
   iris_resource_prepare_access(ice, res, level, 1,
                     }
      /**
   * Perform a GPU-based raw memory copy between compatible view classes.
   *
   * Does not perform any flushing - the new data may still be left in the
   * render cache, and old data may remain in other caches.
   *
   * Wraps blorp_copy() and blorp_buffer_copy().
   */
   void
   iris_copy_region(struct blorp_context *blorp,
                  struct iris_batch *batch,
   struct pipe_resource *dst,
   unsigned dst_level,
   unsigned dstx, unsigned dsty, unsigned dstz,
      {
      struct blorp_batch blorp_batch;
   struct iris_context *ice = blorp->driver_ctx;
   struct iris_screen *screen = (void *) ice->ctx.screen;
   struct iris_resource *src_res = (void *) src;
            enum iris_domain write_domain =
      batch->name == IRIS_BATCH_BLITTER ? IRIS_DOMAIN_OTHER_WRITE
         enum isl_format src_fmt, dst_fmt;
   blorp_copy_get_formats(&screen->isl_dev, &src_res->surf, &dst_res->surf,
            enum isl_aux_usage src_aux_usage =
         enum isl_aux_usage dst_aux_usage =
            if (iris_batch_references(batch, src_res->bo))
            if (dst->target == PIPE_BUFFER)
                              if (dst->target == PIPE_BUFFER && src->target == PIPE_BUFFER) {
      struct blorp_address src_addr = {
      .buffer = src_res->bo, .offset = src_res->offset + src_box->x,
   .mocs = iris_mocs(src_res->bo, &screen->isl_dev,
            };
   struct blorp_address dst_addr = {
      .buffer = dst_res->bo, .offset = dst_res->offset + dstx,
   .reloc_flags = IRIS_BLORP_RELOC_FLAGS_EXEC_OBJECT_WRITE,
   .mocs = iris_mocs(dst_res->bo, &screen->isl_dev,
                     iris_emit_buffer_barrier_for(batch, src_res->bo,
                           iris_batch_sync_region_start(batch);
   blorp_buffer_copy(&blorp_batch, src_addr, dst_addr, src_box->width);
      } else {
               prepare_copy_region(ice, batch, src_res, src_fmt, src_level,
         prepare_copy_region(ice, batch, dst_res, dst_fmt, dst_level,
            iris_emit_buffer_barrier_for(batch, src_res->bo,
                  struct blorp_surf src_surf, dst_surf;
   iris_blorp_surf_for_resource(&screen->isl_dev, &src_surf,
         iris_blorp_surf_for_resource(&screen->isl_dev, &dst_surf,
            for (int slice = 0; slice < src_box->depth; slice++) {
               iris_batch_sync_region_start(batch);
   blorp_copy(&blorp_batch, &src_surf, src_level, src_box->z + slice,
            &dst_surf, dst_level, dstz + slice,
                  iris_resource_finish_write(ice, dst_res, dst_level, dstz,
                           }
      /**
   * The pipe->resource_copy_region() driver hook.
   *
   * This implements ARB_copy_image semantics - a raw memory copy between
   * compatible view classes.
   */
   static void
   iris_resource_copy_region(struct pipe_context *ctx,
                           struct pipe_resource *p_dst,
   unsigned dst_level,
   {
      struct iris_context *ice = (void *) ctx;
            iris_copy_region(&ice->blorp, batch, p_dst, dst_level, dstx, dsty, dstz,
            if (util_format_is_depth_and_stencil(p_dst->format) &&
      util_format_has_stencil(util_format_description(p_src->format))) {
   struct iris_resource *junk, *s_src_res, *s_dst_res;
   iris_get_depth_stencil_resources(p_src, &junk, &s_src_res);
            iris_copy_region(&ice->blorp, batch, &s_dst_res->base.b, dst_level, dstx,
                  }
      void
   iris_init_blit_functions(struct pipe_context *ctx)
   {
      ctx->blit = iris_blit;
      }
