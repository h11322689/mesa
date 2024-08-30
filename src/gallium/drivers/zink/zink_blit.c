   #include "zink_clear.h"
   #include "zink_context.h"
   #include "zink_format.h"
   #include "zink_inlines.h"
   #include "zink_kopper.h"
   #include "zink_helpers.h"
   #include "zink_query.h"
   #include "zink_resource.h"
   #include "zink_screen.h"
      #include "util/u_blitter.h"
   #include "util/u_rect.h"
   #include "util/u_surface.h"
   #include "util/format/u_format.h"
      static void
   apply_dst_clears(struct zink_context *ctx, const struct pipe_blit_info *info, bool discard_only)
   {
      if (info->scissor_enable) {
      struct u_rect rect = { info->scissor.minx, info->scissor.maxx,
            } else
      }
      static bool
   blit_resolve(struct zink_context *ctx, const struct pipe_blit_info *info, bool *needs_present_readback)
   {
      if (util_format_get_mask(info->dst.format) != info->mask ||
      util_format_get_mask(info->src.format) != info->mask ||
   util_format_is_depth_or_stencil(info->dst.format) ||
   info->scissor_enable ||
   info->alpha_blend)
         if (info->src.box.width < 0 ||
      info->dst.box.width < 0 ||
   info->src.box.height < 0 ||
   info->dst.box.height < 0 ||
   info->src.box.depth < 0 ||
   info->dst.box.depth < 0)
      /* vulkan resolves can't downscale */
   if (info->src.box.width > info->dst.box.width ||
      info->src.box.height > info->dst.box.height ||
   info->src.box.depth > info->dst.box.depth)
         if (info->render_condition_enable &&
      ctx->render_condition_active)
         struct zink_resource *src = zink_resource(info->src.resource);
   struct zink_resource *use_src = src;
            struct zink_screen *screen = zink_screen(ctx->base.screen);
   /* aliased/swizzled formats need u_blitter */
   if (src->format != zink_get_format(screen, info->src.format) ||
      dst->format != zink_get_format(screen, info->dst.format))
      if (src->format != dst->format)
               apply_dst_clears(ctx, info, false);
            if (src->obj->dt)
            struct zink_batch *batch = &ctx->batch;
   zink_resource_setup_transfer_layouts(ctx, use_src, dst);
   VkCommandBuffer cmdbuf = *needs_present_readback ?
               if (cmdbuf == ctx->batch.state->cmdbuf)
         zink_batch_reference_resource_rw(batch, use_src, false);
            bool marker = zink_cmd_debug_marker_begin(ctx, cmdbuf, "blit_resolve(%s->%s, %dx%d->%dx%d)",
                                    region.srcSubresource.aspectMask = src->aspect;
   region.srcSubresource.mipLevel = info->src.level;
   region.srcOffset.x = info->src.box.x;
            if (src->base.b.array_size > 1) {
      region.srcOffset.z = 0;
   region.srcSubresource.baseArrayLayer = info->src.box.z;
      } else {
      assert(info->src.box.depth == 1);
   region.srcOffset.z = info->src.box.z;
   region.srcSubresource.baseArrayLayer = 0;
               region.dstSubresource.aspectMask = dst->aspect;
   region.dstSubresource.mipLevel = info->dst.level;
   region.dstOffset.x = info->dst.box.x;
            if (dst->base.b.array_size > 1) {
      region.dstOffset.z = 0;
   region.dstSubresource.baseArrayLayer = info->dst.box.z;
      } else {
      assert(info->dst.box.depth == 1);
   region.dstOffset.z = info->dst.box.z;
   region.dstSubresource.baseArrayLayer = 0;
               region.extent.width = info->dst.box.width;
   region.extent.height = info->dst.box.height;
   region.extent.depth = info->dst.box.depth;
   if (region.srcOffset.x + region.extent.width >= u_minify(src->base.b.width0, region.srcSubresource.mipLevel))
         if (region.dstOffset.x + region.extent.width >= u_minify(dst->base.b.width0, region.dstSubresource.mipLevel))
         if (region.srcOffset.y + region.extent.height >= u_minify(src->base.b.height0, region.srcSubresource.mipLevel))
         if (region.dstOffset.y + region.extent.height >= u_minify(dst->base.b.height0, region.dstSubresource.mipLevel))
         if (region.srcOffset.z + region.extent.depth >= u_minify(src->base.b.depth0, region.srcSubresource.mipLevel))
         if (region.dstOffset.z + region.extent.depth >= u_minify(dst->base.b.depth0, region.dstSubresource.mipLevel))
         VKCTX(CmdResolveImage)(cmdbuf, use_src->obj->image, src->layout,
                           }
      static bool
   blit_native(struct zink_context *ctx, const struct pipe_blit_info *info, bool *needs_present_readback)
   {
      if (util_format_get_mask(info->dst.format) != info->mask ||
      util_format_get_mask(info->src.format) != info->mask ||
   info->scissor_enable ||
   info->alpha_blend)
         if (info->render_condition_enable &&
      ctx->render_condition_active)
         if (util_format_is_depth_or_stencil(info->dst.format) &&
      (info->dst.format != info->src.format || info->filter == PIPE_TEX_FILTER_LINEAR))
         /* vkCmdBlitImage must not be used for multisampled source or destination images. */
   if (info->src.resource->nr_samples > 1 || info->dst.resource->nr_samples > 1)
            struct zink_resource *src = zink_resource(info->src.resource);
   struct zink_resource *use_src = src;
            struct zink_screen *screen = zink_screen(ctx->base.screen);
   if (src->format != zink_get_format(screen, info->src.format) ||
      dst->format != zink_get_format(screen, info->dst.format))
      if (src->format != VK_FORMAT_A8_UNORM_KHR && zink_format_is_emulated_alpha(info->src.format))
            if (!(src->obj->vkfeats & VK_FORMAT_FEATURE_BLIT_SRC_BIT) ||
      !(dst->obj->vkfeats & VK_FORMAT_FEATURE_BLIT_DST_BIT))
         if ((util_format_is_pure_sint(info->src.format) !=
      util_format_is_pure_sint(info->dst.format)) ||
   (util_format_is_pure_uint(info->src.format) !=
   util_format_is_pure_uint(info->dst.format)))
         if (info->filter == PIPE_TEX_FILTER_LINEAR &&
      !(src->obj->vkfeats & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
            VkImageBlit region = {0};
   region.srcSubresource.aspectMask = src->aspect;
   region.srcSubresource.mipLevel = info->src.level;
   region.srcOffsets[0].x = info->src.box.x;
   region.srcOffsets[0].y = info->src.box.y;
   region.srcOffsets[1].x = info->src.box.x + info->src.box.width;
            enum pipe_texture_target src_target = src->base.b.target;
   if (src->need_2D)
         switch (src_target) {
   case PIPE_TEXTURE_CUBE:
   case PIPE_TEXTURE_CUBE_ARRAY:
   case PIPE_TEXTURE_2D_ARRAY:
   case PIPE_TEXTURE_1D_ARRAY:
      /* these use layer */
   region.srcSubresource.baseArrayLayer = info->src.box.z;
   /* VUID-vkCmdBlitImage-srcImage-00240 */
   if (region.srcSubresource.baseArrayLayer && dst->base.b.target == PIPE_TEXTURE_3D)
         region.srcSubresource.layerCount = info->src.box.depth;
   region.srcOffsets[0].z = 0;
   region.srcOffsets[1].z = 1;
      case PIPE_TEXTURE_3D:
      /* this uses depth */
   region.srcSubresource.baseArrayLayer = 0;
   region.srcSubresource.layerCount = 1;
   region.srcOffsets[0].z = info->src.box.z;
   region.srcOffsets[1].z = info->src.box.z + info->src.box.depth;
      default:
      /* these must only copy one layer */
   region.srcSubresource.baseArrayLayer = 0;
   region.srcSubresource.layerCount = 1;
   region.srcOffsets[0].z = 0;
               region.dstSubresource.aspectMask = dst->aspect;
   region.dstSubresource.mipLevel = info->dst.level;
   region.dstOffsets[0].x = info->dst.box.x;
   region.dstOffsets[0].y = info->dst.box.y;
   region.dstOffsets[1].x = info->dst.box.x + info->dst.box.width;
   region.dstOffsets[1].y = info->dst.box.y + info->dst.box.height;
   assert(region.dstOffsets[0].x != region.dstOffsets[1].x);
            enum pipe_texture_target dst_target = dst->base.b.target;
   if (dst->need_2D)
         switch (dst_target) {
   case PIPE_TEXTURE_CUBE:
   case PIPE_TEXTURE_CUBE_ARRAY:
   case PIPE_TEXTURE_2D_ARRAY:
   case PIPE_TEXTURE_1D_ARRAY:
      /* these use layer */
   region.dstSubresource.baseArrayLayer = info->dst.box.z;
   /* VUID-vkCmdBlitImage-srcImage-00240 */
   if (region.dstSubresource.baseArrayLayer && src->base.b.target == PIPE_TEXTURE_3D)
         region.dstSubresource.layerCount = info->dst.box.depth;
   region.dstOffsets[0].z = 0;
   region.dstOffsets[1].z = 1;
      case PIPE_TEXTURE_3D:
      /* this uses depth */
   region.dstSubresource.baseArrayLayer = 0;
   region.dstSubresource.layerCount = 1;
   region.dstOffsets[0].z = info->dst.box.z;
   region.dstOffsets[1].z = info->dst.box.z + info->dst.box.depth;
      default:
      /* these must only copy one layer */
   region.dstSubresource.baseArrayLayer = 0;
   region.dstSubresource.layerCount = 1;
   region.dstOffsets[0].z = 0;
      }
            apply_dst_clears(ctx, info, false);
            if (src->obj->dt)
            struct zink_batch *batch = &ctx->batch;
   zink_resource_setup_transfer_layouts(ctx, use_src, dst);
   VkCommandBuffer cmdbuf = *needs_present_readback ?
               if (cmdbuf == ctx->batch.state->cmdbuf)
         zink_batch_reference_resource_rw(batch, use_src, false);
            bool marker = zink_cmd_debug_marker_begin(ctx, cmdbuf, "blit_native(%s->%s, %dx%d->%dx%d)",
                              VKCTX(CmdBlitImage)(cmdbuf, use_src->obj->image, src->layout,
                                    }
      static bool
   try_copy_region(struct pipe_context *pctx, const struct pipe_blit_info *info)
   {
      struct zink_context *ctx = zink_context(pctx);
   struct zink_resource *src = zink_resource(info->src.resource);
   struct zink_resource *dst = zink_resource(info->dst.resource);
   /* if we're copying between resources with matching aspects then we can probably just copy_region */
   if (src->aspect != dst->aspect)
                  if (src->aspect & VK_IMAGE_ASPECT_STENCIL_BIT &&
      new_info.render_condition_enable &&
   !ctx->render_condition_active)
            }
      void
   zink_blit(struct pipe_context *pctx,
         {
      struct zink_context *ctx = zink_context(pctx);
   const struct util_format_description *src_desc = util_format_description(info->src.format);
            struct zink_resource *src = zink_resource(info->src.resource);
   struct zink_resource *use_src = src;
   struct zink_resource *dst = zink_resource(info->dst.resource);
   bool needs_present_readback = false;
   if (zink_is_swapchain(dst)) {
      if (!zink_kopper_acquire(ctx, dst, UINT64_MAX))
               if (src_desc == dst_desc ||
      src_desc->nr_channels != 4 || src_desc->layout != UTIL_FORMAT_LAYOUT_PLAIN ||
   (src_desc->nr_channels == 4 && src_desc->channel[3].type != UTIL_FORMAT_TYPE_VOID)) {
   /* we can't blit RGBX -> RGBA formats directly since they're emulated
   * so we have to use sampler views
   */
   if (info->src.resource->nr_samples > 1 &&
      info->dst.resource->nr_samples <= 1) {
   if (blit_resolve(ctx, info, &needs_present_readback))
      } else {
      if (try_copy_region(pctx, info))
         if (blit_native(ctx, info, &needs_present_readback))
                        bool stencil_blit = false;
   if (!util_blitter_is_blit_supported(ctx->blitter, info)) {
      if (util_format_is_depth_or_stencil(info->src.resource->format)) {
      struct pipe_blit_info depth_blit = *info;
   depth_blit.mask = PIPE_MASK_Z;
   stencil_blit = util_blitter_is_blit_supported(ctx->blitter, &depth_blit);
   if (stencil_blit) {
      zink_blit_begin(ctx, ZINK_BLIT_SAVE_FB | ZINK_BLIT_SAVE_FS | ZINK_BLIT_SAVE_TEXTURES);
         }
   if (!stencil_blit) {
      mesa_loge("ZINK: blit unsupported %s -> %s",
         util_format_short_name(info->src.resource->format),
                  if (src->obj->dt) {
      zink_fb_clears_apply_region(ctx, info->src.resource, zink_rect_from_box(&info->src.box));
               /* this is discard_only because we're about to start a renderpass that will
   * flush all pending clears anyway
   */
   apply_dst_clears(ctx, info, true);
   zink_fb_clears_apply_region(ctx, info->src.resource, zink_rect_from_box(&info->src.box));
   unsigned rp_clears_enabled = ctx->rp_clears_enabled;
   unsigned clears_enabled = ctx->clears_enabled;
   if (!dst->fb_bind_count) {
      /* avoid applying clears from fb unbind by storing and re-setting them after the blit */
   ctx->rp_clears_enabled = 0;
      } else {
      unsigned bit;
   /* convert to PIPE_CLEAR_XYZ */
   if (dst->fb_binds & BITFIELD_BIT(PIPE_MAX_COLOR_BUFS))
         else
         rp_clears_enabled &= ~bit;
   clears_enabled &= ~bit;
   ctx->rp_clears_enabled &= bit;
               /* this will draw a full-resource quad, so ignore existing data */
   bool whole = util_blit_covers_whole_resource(info);
   if (whole)
            zink_flush_dgc_if_enabled(ctx);
   ctx->unordered_blitting = !(info->render_condition_enable && ctx->render_condition_active) &&
                     VkCommandBuffer cmdbuf = ctx->batch.state->cmdbuf;
   VkPipeline pipeline = ctx->gfx_pipeline_state.pipeline;
   bool in_rp = ctx->batch.in_rp;
   uint64_t tc_data = ctx->dynamic_fb.tc_info.data;
   bool queries_disabled = ctx->queries_disabled;
   bool rp_changed = ctx->rp_changed || (!ctx->fb_state.zsbuf && util_format_is_depth_or_stencil(info->dst.format));
   unsigned ds3_states = ctx->ds3_states;
   bool rp_tc_info_updated = ctx->rp_tc_info_updated;
   if (ctx->unordered_blitting) {
      /* for unordered blit, swap the unordered cmdbuf for the main one for the whole op to avoid conditional hell */
   ctx->batch.state->cmdbuf = ctx->batch.state->barrier_cmdbuf;
   ctx->batch.in_rp = false;
   ctx->rp_changed = true;
   ctx->queries_disabled = true;
   ctx->batch.state->has_barriers = true;
   ctx->pipeline_changed[0] = true;
   zink_reset_ds3_states(ctx);
      }
   zink_blit_begin(ctx, ZINK_BLIT_SAVE_FB | ZINK_BLIT_SAVE_FS | ZINK_BLIT_SAVE_TEXTURES);
   if (zink_format_needs_mutable(info->src.format, info->src.resource->format))
         if (zink_format_needs_mutable(info->dst.format, info->dst.resource->format))
         zink_blit_barriers(ctx, use_src, dst, whole);
            if (stencil_blit) {
      struct pipe_surface *dst_view, dst_templ;
   util_blitter_default_dst_texture(&dst_templ, info->dst.resource, info->dst.level, info->dst.box.z);
            util_blitter_clear_depth_stencil(ctx->blitter, dst_view, PIPE_CLEAR_STENCIL,
               zink_blit_begin(ctx, ZINK_BLIT_SAVE_FB | ZINK_BLIT_SAVE_FS | ZINK_BLIT_SAVE_TEXTURES | ZINK_BLIT_SAVE_FS_CONST_BUF);
   util_blitter_stencil_fallback(ctx->blitter,
                                 info->dst.resource,
               } else {
      struct pipe_blit_info new_info = *info;
   new_info.src.resource = &use_src->base.b;
      }
   ctx->blitting = false;
   ctx->rp_clears_enabled = rp_clears_enabled;
   ctx->clears_enabled = clears_enabled;
   if (ctx->unordered_blitting) {
      zink_batch_no_rp(ctx);
   ctx->batch.in_rp = in_rp;
   ctx->gfx_pipeline_state.rp_state = zink_update_rendering_info(ctx);
   ctx->rp_changed = rp_changed;
   ctx->rp_tc_info_updated |= rp_tc_info_updated;
   ctx->queries_disabled = queries_disabled;
   ctx->dynamic_fb.tc_info.data = tc_data;
   ctx->batch.state->cmdbuf = cmdbuf;
   ctx->gfx_pipeline_state.pipeline = pipeline;
   ctx->pipeline_changed[0] = true;
   ctx->ds3_states = ds3_states;
      }
      end:
      if (needs_present_readback) {
      src->obj->unordered_read = false;
   dst->obj->unordered_write = false;
         }
      /* similar to radeonsi */
   void
   zink_blit_begin(struct zink_context *ctx, enum zink_blit_flags flags)
   {
      util_blitter_save_vertex_elements(ctx->blitter, ctx->element_state);
            util_blitter_save_vertex_buffer_slot(ctx->blitter, ctx->vertex_buffers);
   util_blitter_save_vertex_shader(ctx->blitter, ctx->gfx_stages[MESA_SHADER_VERTEX]);
   util_blitter_save_tessctrl_shader(ctx->blitter, ctx->gfx_stages[MESA_SHADER_TESS_CTRL]);
   util_blitter_save_tesseval_shader(ctx->blitter, ctx->gfx_stages[MESA_SHADER_TESS_EVAL]);
   util_blitter_save_geometry_shader(ctx->blitter, ctx->gfx_stages[MESA_SHADER_GEOMETRY]);
   util_blitter_save_rasterizer(ctx->blitter, ctx->rast_state);
            if (flags & ZINK_BLIT_SAVE_FS_CONST_BUF)
            if (flags & ZINK_BLIT_SAVE_FS) {
      util_blitter_save_blend(ctx->blitter, ctx->gfx_pipeline_state.blend_state);
   util_blitter_save_depth_stencil_alpha(ctx->blitter, ctx->dsa_state);
   util_blitter_save_stencil_ref(ctx->blitter, &ctx->stencil_ref);
   util_blitter_save_sample_mask(ctx->blitter, ctx->gfx_pipeline_state.sample_mask, ctx->gfx_pipeline_state.min_samples + 1);
   util_blitter_save_scissor(ctx->blitter, ctx->vp_state.scissor_states);
                        if (flags & ZINK_BLIT_SAVE_FB)
               if (flags & ZINK_BLIT_SAVE_TEXTURES) {
      util_blitter_save_fragment_sampler_states(ctx->blitter,
               util_blitter_save_fragment_sampler_views(ctx->blitter,
                     if (flags & ZINK_BLIT_NO_COND_RENDER && ctx->render_condition_active)
      }
      void
   zink_blit_barriers(struct zink_context *ctx, struct zink_resource *src, struct zink_resource *dst, bool whole_dst)
   {
      struct zink_screen *screen = zink_screen(ctx->base.screen);
   if (src && zink_is_swapchain(src)) {
      if (!zink_kopper_acquire(ctx, src, UINT64_MAX))
      } else if (dst && zink_is_swapchain(dst)) {
      if (!zink_kopper_acquire(ctx, dst, UINT64_MAX))
               VkAccessFlagBits flags;
   VkPipelineStageFlagBits pipeline;
   if (util_format_is_depth_or_stencil(dst->base.b.format)) {
      flags = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
   if (!whole_dst)
            } else {
      flags = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
   if (!whole_dst)
            }
   if (src == dst) {
      VkImageLayout layout = zink_screen(ctx->base.screen)->info.have_EXT_attachment_feedback_loop_layout ?
                  } else {
      if (src) {
      VkImageLayout layout = util_format_is_depth_or_stencil(src->base.b.format) &&
                     screen->image_barrier(ctx, src, layout,
         if (!ctx->unordered_blitting)
      }
   VkImageLayout layout = util_format_is_depth_or_stencil(dst->base.b.format) ?
                  }
   if (!ctx->unordered_blitting)
      }
      bool
   zink_blit_region_fills(struct u_rect region, unsigned width, unsigned height)
   {
      struct u_rect intersect = {0, width, 0, height};
   struct u_rect r = {
      MIN2(region.x0, region.x1),
   MAX2(region.x0, region.x1),
   MIN2(region.y0, region.y1),
               if (!u_rect_test_intersection(&r, &intersect))
      /* is this even a thing? */
         u_rect_find_intersection(&r, &intersect);
   if (intersect.x0 != 0 || intersect.y0 != 0 ||
      intersect.x1 != width || intersect.y1 != height)
            }
      bool
   zink_blit_region_covers(struct u_rect region, struct u_rect covers)
   {
      struct u_rect r = {
      MIN2(region.x0, region.x1),
   MAX2(region.x0, region.x1),
   MIN2(region.y0, region.y1),
      };
   struct u_rect c = {
      MIN2(covers.x0, covers.x1),
   MAX2(covers.x0, covers.x1),
   MIN2(covers.y0, covers.y1),
      };
   struct u_rect intersect;
   if (!u_rect_test_intersection(&r, &c))
            u_rect_union(&intersect, &r, &c);
   return intersect.x0 == c.x0 && intersect.y0 == c.y0 &&
      }
