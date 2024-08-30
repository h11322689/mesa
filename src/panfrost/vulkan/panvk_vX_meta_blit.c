   /*
   * Copyright © 2021 Collabora Ltd.
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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      #include "gen_macros.h"
      #include "pan_blitter.h"
      #include "panvk_private.h"
      static void
   panvk_meta_blit(struct panvk_cmd_buffer *cmdbuf,
         {
      struct panfrost_device *pdev = &cmdbuf->device->physical_device->pdev;
   struct pan_fb_info *fbinfo = &cmdbuf->state.fb.info;
   struct pan_blit_context ctx;
   struct pan_image_view views[2] = {
      {
      .format = blitinfo->dst.planes[0].format,
   .dim = MALI_TEXTURE_DIMENSION_2D,
   .planes =
      {
      blitinfo->dst.planes[0].image,
   blitinfo->dst.planes[1].image,
         .nr_samples = blitinfo->dst.planes[0].image->layout.nr_samples,
   .first_level = blitinfo->dst.level,
   .last_level = blitinfo->dst.level,
   .swizzle = {PIPE_SWIZZLE_X, PIPE_SWIZZLE_Y, PIPE_SWIZZLE_Z,
                  *fbinfo = (struct pan_fb_info){
      .width = u_minify(blitinfo->dst.planes[0].image->layout.width,
         .height = u_minify(blitinfo->dst.planes[0].image->layout.height,
         .extent =
      {
      .minx = MAX2(MIN2(blitinfo->dst.start.x, blitinfo->dst.end.x), 0),
   .miny = MAX2(MIN2(blitinfo->dst.start.y, blitinfo->dst.end.y), 0),
   .maxx = MAX2(blitinfo->dst.start.x, blitinfo->dst.end.x),
                     fbinfo->extent.maxx = MIN2(fbinfo->extent.maxx, fbinfo->width - 1);
                     const struct util_format_description *fdesc =
            if (util_format_has_depth(fdesc)) {
      /* We want the image format here, otherwise we might lose one of the
   * component.
   */
   views[0].format = blitinfo->dst.planes[0].image->layout.format;
   fbinfo->zs.view.zs = &views[0];
   fbinfo->zs.preload.z = true;
      } else if (util_format_has_stencil(fdesc)) {
      fbinfo->zs.view.s = &views[0];
      } else {
      fbinfo->rt_count = 1;
   fbinfo->rts[0].view = &views[0];
   fbinfo->rts[0].preload = true;
   cmdbuf->state.fb.crc_valid[0] = false;
               if (blitinfo->dst.planes[1].format != PIPE_FORMAT_NONE) {
      /* TODO: don't force preloads of dst resources if unneeded */
   views[1].format = blitinfo->dst.planes[1].format;
   views[1].dim = MALI_TEXTURE_DIMENSION_2D;
   views[1].planes[0] = blitinfo->dst.planes[1].image;
   views[1].nr_samples = blitinfo->dst.planes[1].image->layout.nr_samples;
   views[1].first_level = blitinfo->dst.level;
   views[1].last_level = blitinfo->dst.level;
   views[1].swizzle[0] = PIPE_SWIZZLE_X;
   views[1].swizzle[1] = PIPE_SWIZZLE_Y;
   views[1].swizzle[2] = PIPE_SWIZZLE_Z;
   views[1].swizzle[3] = PIPE_SWIZZLE_W;
                        GENX(pan_blit_ctx_init)(pdev, blitinfo, &cmdbuf->desc_pool.base, &ctx);
   do {
      if (ctx.dst.cur_layer < 0)
            struct panvk_batch *batch = panvk_cmd_open_batch(cmdbuf);
            views[0].first_layer = views[0].last_layer = ctx.dst.cur_layer;
   views[1].first_layer = views[1].last_layer = views[0].first_layer;
   batch->blit.src = blitinfo->src.planes[0].image->data.bo;
   batch->blit.dst = blitinfo->dst.planes[0].image->data.bo;
   panvk_per_arch(cmd_alloc_tls_desc)(cmdbuf, true);
   panvk_per_arch(cmd_alloc_fb_desc)(cmdbuf);
            tsd = batch->tls.gpu;
            struct panfrost_ptr job = GENX(pan_blit)(&ctx, &cmdbuf->desc_pool.base,
         util_dynarray_append(&batch->jobs, void *, job.cpu);
         }
      void
   panvk_per_arch(CmdBlitImage2)(VkCommandBuffer commandBuffer,
         {
      VK_FROM_HANDLE(panvk_cmd_buffer, cmdbuf, commandBuffer);
   VK_FROM_HANDLE(panvk_image, src, pBlitImageInfo->srcImage);
            for (unsigned i = 0; i < pBlitImageInfo->regionCount; i++) {
      const VkImageBlit2 *region = &pBlitImageInfo->pRegions[i];
   struct pan_blit_info info = {
      .src =
      {
      .planes[0].image = &src->pimage,
   .planes[0].format = src->pimage.layout.format,
   .level = region->srcSubresource.mipLevel,
   .start =
      {
      region->srcOffsets[0].x,
   region->srcOffsets[0].y,
   region->srcOffsets[0].z,
         .end =
      {
      region->srcOffsets[1].x,
   region->srcOffsets[1].y,
   region->srcOffsets[1].z,
   region->srcSubresource.baseArrayLayer +
         .dst =
      {
      .planes[0].image = &dst->pimage,
   .planes[0].format = dst->pimage.layout.format,
   .level = region->dstSubresource.mipLevel,
   .start =
      {
      region->dstOffsets[0].x,
   region->dstOffsets[0].y,
   region->dstOffsets[0].z,
         .end =
      {
      region->dstOffsets[1].x,
   region->dstOffsets[1].y,
   region->dstOffsets[1].z,
   region->dstSubresource.baseArrayLayer +
                     if (region->srcSubresource.aspectMask == VK_IMAGE_ASPECT_STENCIL_BIT)
      info.src.planes[0].format =
      else if (region->srcSubresource.aspectMask == VK_IMAGE_ASPECT_DEPTH_BIT)
                  if (region->dstSubresource.aspectMask == VK_IMAGE_ASPECT_STENCIL_BIT)
      info.dst.planes[0].format =
      else if (region->dstSubresource.aspectMask == VK_IMAGE_ASPECT_DEPTH_BIT)
                        }
      void
   panvk_per_arch(CmdResolveImage2)(VkCommandBuffer commandBuffer,
         {
         }
      void
   panvk_per_arch(meta_blit_init)(struct panvk_physical_device *dev)
   {
      panvk_pool_init(&dev->meta.blitter.bin_pool, &dev->pdev, NULL,
               panvk_pool_init(&dev->meta.blitter.desc_pool, &dev->pdev, NULL, 0, 16 * 1024,
         pan_blend_shaders_init(&dev->pdev);
   GENX(pan_blitter_init)
   (&dev->pdev, &dev->meta.blitter.bin_pool.base,
      }
      void
   panvk_per_arch(meta_blit_cleanup)(struct panvk_physical_device *dev)
   {
      GENX(pan_blitter_cleanup)(&dev->pdev);
   pan_blend_shaders_cleanup(&dev->pdev);
   panvk_pool_cleanup(&dev->meta.blitter.desc_pool);
      }
