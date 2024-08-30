   /*
   * Copyright © 2019 Raspberry Pi Ltd
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
      #include "v3dv_private.h"
      static uint32_t
   num_subpass_attachments(const VkSubpassDescription2 *desc)
   {
      return desc->inputAttachmentCount +
         desc->colorAttachmentCount +
      }
      static void
   set_try_tlb_resolve(struct v3dv_device *device,
         {
      const struct v3dv_format *format = v3dv_X(device, get_format)(att->desc.format);
      }
      static void
   pass_find_subpass_range_for_attachments(struct v3dv_device *device,
         {
      for (uint32_t i = 0; i < pass->attachment_count; i++) {
      pass->attachments[i].first_subpass = pass->subpass_count - 1;
   pass->attachments[i].last_subpass = 0;
   if (pass->multiview_enabled) {
      for (uint32_t j = 0; j < MAX_MULTIVIEW_VIEW_COUNT; j++) {
      pass->attachments[i].views[j].first_subpass = pass->subpass_count - 1;
                     for (uint32_t i = 0; i < pass->subpass_count; i++) {
               for (uint32_t j = 0; j < subpass->color_count; j++) {
      uint32_t attachment_idx = subpass->color_attachments[j].attachment;
                                 if (i < att->first_subpass)
                        uint32_t view_mask = subpass->view_mask;
   while (view_mask) {
      uint32_t view_index = u_bit_scan(&view_mask);
   if (i < att->views[view_index].first_subpass)
         if (i > att->views[view_index].last_subpass)
               if (subpass->resolve_attachments &&
      subpass->resolve_attachments[j].attachment != VK_ATTACHMENT_UNUSED) {
                  uint32_t ds_attachment_idx = subpass->ds_attachment.attachment;
   if (ds_attachment_idx != VK_ATTACHMENT_UNUSED) {
      if (i < pass->attachments[ds_attachment_idx].first_subpass)
                        if (subpass->ds_resolve_attachment.attachment != VK_ATTACHMENT_UNUSED)
               for (uint32_t j = 0; j < subpass->input_count; j++) {
      uint32_t input_attachment_idx = subpass->input_attachments[j].attachment;
   if (input_attachment_idx == VK_ATTACHMENT_UNUSED)
         if (i < pass->attachments[input_attachment_idx].first_subpass)
         if (i > pass->attachments[input_attachment_idx].last_subpass)
               if (subpass->resolve_attachments) {
      for (uint32_t j = 0; j < subpass->color_count; j++) {
      uint32_t attachment_idx = subpass->resolve_attachments[j].attachment;
   if (attachment_idx == VK_ATTACHMENT_UNUSED)
         if (i < pass->attachments[attachment_idx].first_subpass)
         if (i > pass->attachments[attachment_idx].last_subpass)
               }
         VKAPI_ATTR VkResult VKAPI_CALL
   v3dv_CreateRenderPass2(VkDevice _device,
                     {
      V3DV_FROM_HANDLE(v3dv_device, device, _device);
                     /* From the VK_KHR_multiview spec:
   *
   *   When a subpass uses a non-zero view mask, multiview functionality is
   *   considered to be enabled. Multiview is all-or-nothing for a render
   *   pass - that is, either all subpasses must have a non-zero view mask
   *   (though some subpasses may have only one view) or all must be zero.
   */
   bool multiview_enabled = pCreateInfo->subpassCount &&
            size_t size = sizeof(*pass);
   size_t subpasses_offset = size;
   size += pCreateInfo->subpassCount * sizeof(pass->subpasses[0]);
   size_t attachments_offset = size;
            pass = vk_object_zalloc(&device->vk, pAllocator, size,
         if (pass == NULL)
            pass->multiview_enabled = multiview_enabled;
   pass->attachment_count = pCreateInfo->attachmentCount;
   pass->attachments = (void *) pass + attachments_offset;
   pass->subpass_count = pCreateInfo->subpassCount;
            for (uint32_t i = 0; i < pCreateInfo->attachmentCount; i++)
            uint32_t subpass_attachment_count = 0;
   for (uint32_t i = 0; i < pCreateInfo->subpassCount; i++) {
      const VkSubpassDescription2 *desc = &pCreateInfo->pSubpasses[i];
               if (subpass_attachment_count) {
      const size_t subpass_attachment_bytes =
         pass->subpass_attachments =
      vk_alloc2(&device->vk.alloc, pAllocator, subpass_attachment_bytes, 8,
      if (pass->subpass_attachments == NULL) {
      vk_object_free(&device->vk, pAllocator, pass);
         } else {
                  struct v3dv_subpass_attachment *p = pass->subpass_attachments;
   for (uint32_t i = 0; i < pCreateInfo->subpassCount; i++) {
      const VkSubpassDescription2 *desc = &pCreateInfo->pSubpasses[i];
            subpass->input_count = desc->inputAttachmentCount;
   subpass->color_count = desc->colorAttachmentCount;
            if (desc->inputAttachmentCount > 0) {
                     for (uint32_t j = 0; j < desc->inputAttachmentCount; j++) {
      subpass->input_attachments[j] = (struct v3dv_subpass_attachment) {
      .attachment = desc->pInputAttachments[j].attachment,
                     if (desc->colorAttachmentCount > 0) {
                     for (uint32_t j = 0; j < desc->colorAttachmentCount; j++) {
      subpass->color_attachments[j] = (struct v3dv_subpass_attachment) {
      .attachment = desc->pColorAttachments[j].attachment,
                     if (desc->pResolveAttachments) {
                     for (uint32_t j = 0; j < desc->colorAttachmentCount; j++) {
      subpass->resolve_attachments[j] = (struct v3dv_subpass_attachment) {
      .attachment = desc->pResolveAttachments[j].attachment,
                     if (desc->pDepthStencilAttachment) {
      subpass->ds_attachment = (struct v3dv_subpass_attachment) {
      .attachment = desc->pDepthStencilAttachment->attachment,
               /* GFXH-1461: if depth is cleared but stencil is loaded (or vice versa),
   * the clear might get lost. If a subpass has this then we can't emit
   * the clear using the TLB and we have to do it as a draw call. This
   * issue is fixed since V3D 4.3.18.
   *
   * FIXME: separate stencil.
   */
   if (device->devinfo.ver == 42 &&
      subpass->ds_attachment.attachment != VK_ATTACHMENT_UNUSED) {
   struct v3dv_render_pass_attachment *att =
         if (att->desc.format == VK_FORMAT_D24_UNORM_S8_UINT) {
      if (att->desc.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR &&
      att->desc.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_LOAD) {
      } else if (att->desc.loadOp == VK_ATTACHMENT_LOAD_OP_LOAD &&
                              /* VK_KHR_depth_stencil_resolve */
   const VkSubpassDescriptionDepthStencilResolve *resolve_desc =
         const VkAttachmentReference2 *resolve_att =
      resolve_desc && resolve_desc->pDepthStencilResolveAttachment &&
   resolve_desc->pDepthStencilResolveAttachment->attachment != VK_ATTACHMENT_UNUSED ?
      if (resolve_att) {
      subpass->ds_resolve_attachment = (struct v3dv_subpass_attachment) {
      .attachment = resolve_att->attachment,
      };
   assert(resolve_desc->depthResolveMode == VK_RESOLVE_MODE_SAMPLE_ZERO_BIT ||
         subpass->resolve_depth =
      resolve_desc->depthResolveMode != VK_RESOLVE_MODE_NONE &&
      subpass->resolve_stencil =
      resolve_desc->stencilResolveMode != VK_RESOLVE_MODE_NONE &&
   } else {
      subpass->ds_resolve_attachment.attachment = VK_ATTACHMENT_UNUSED;
   subpass->resolve_depth = false;
         } else {
      subpass->ds_attachment.attachment = VK_ATTACHMENT_UNUSED;
   subpass->ds_resolve_attachment.attachment = VK_ATTACHMENT_UNUSED;
   subpass->resolve_depth = false;
                                                }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_DestroyRenderPass(VkDevice _device,
               {
      V3DV_FROM_HANDLE(v3dv_device, device, _device);
            if (!_pass)
            vk_free2(&device->vk.alloc, pAllocator, pass->subpass_attachments);
      }
      static void
   subpass_get_granularity(struct v3dv_device *device,
                     {
      /* Granularity is defined by the tile size */
   assert(subpass_idx < pass->subpass_count);
   struct v3dv_subpass *subpass = &pass->subpasses[subpass_idx];
            bool msaa = false;
   uint32_t max_internal_bpp = 0;
   uint32_t total_color_bpp = 0;
   for (uint32_t i = 0; i < color_count; i++) {
      uint32_t attachment_idx = subpass->color_attachments[i].attachment;
   if (attachment_idx == VK_ATTACHMENT_UNUSED)
         const VkAttachmentDescription2 *desc =
         const struct v3dv_format *format = v3dv_X(device, get_format)(desc->format);
   uint32_t internal_type, internal_bpp;
   /* We don't support rendering to YCbCr images */
   assert(format->plane_count == 1);
   v3dv_X(device, get_internal_type_bpp_for_output_format)
            max_internal_bpp = MAX2(max_internal_bpp, internal_bpp);
            if (desc->samples > VK_SAMPLE_COUNT_1_BIT)
               /* If requested, double-buffer may or may not be enabled depending on
   * heuristics so we choose a conservative granularity here, with it disabled.
   */
   uint32_t width, height;
   v3d_choose_tile_size(&device->devinfo, color_count,
               *granularity = (VkExtent2D) {
      .width = width,
         }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_GetRenderAreaGranularity(VkDevice _device,
               {
      V3DV_FROM_HANDLE(v3dv_render_pass, pass, renderPass);
            *pGranularity = (VkExtent2D) {
      .width = 64,
               for (uint32_t i = 0; i < pass->subpass_count; i++) {
      VkExtent2D sg;
   subpass_get_granularity(device, pass, i, &sg);
   pGranularity->width = MIN2(pGranularity->width, sg.width);
         }
      /* Checks whether the render area rectangle covers a region that is aligned to
   * tile boundaries. This means that we are writing to all pixels covered by
   * all tiles in that area (except for pixels on edge tiles that are outside
   * the framebuffer dimensions).
   *
   * When our framebuffer is aligned to tile boundaries we know we are writing
   * valid data to all all pixels in each tile and we can apply certain
   * optimizations, like avoiding tile loads, since we know that none of the
   * original pixel values in each tile for that area need to be preserved.
   * We also use this to decide if we can use TLB clears, as these clear whole
   * tiles so we can't use them if the render area is not aligned.
   *
   * Note that when an image is created it will possibly include padding blocks
   * depending on its tiling layout. When the framebuffer dimensions are not
   * aligned to tile boundaries then edge tiles are only partially covered by the
   * framebuffer pixels, but tile stores still seem to store full tiles
   * writing to the padded sections. This is important when the framebuffer
   * is aliasing a smaller section of a larger image, as in that case the edge
   * tiles of the framebuffer would overwrite valid pixels in the larger image.
   * In that case, we can't flag the area as being aligned.
   */
   bool
   v3dv_subpass_area_is_tile_aligned(struct v3dv_device *device,
                           {
               VkExtent2D granularity;
            return area->offset.x % granularity.width == 0 &&
         area->offset.y % granularity.height == 0 &&
   (area->extent.width % granularity.width == 0 ||
   (fb->has_edge_padding &&
   area->offset.x + area->extent.width >= fb->width)) &&
   (area->extent.height % granularity.height == 0 ||
      }
