   /*
   * Copyright © 2021 Collabora Ltd.
   *
   * Derived from tu_pass.c which is:
   * Copyright © 2016 Red Hat.
   * Copyright © 2016 Bas Nieuwenhuizen
   * Copyright © 2015 Intel Corporation
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
   #include "panvk_private.h"
      #include "vk_format.h"
   #include "vk_util.h"
      VkResult
   panvk_CreateRenderPass2(VkDevice _device,
                     {
      VK_FROM_HANDLE(panvk_device, device, _device);
   struct panvk_render_pass *pass;
   size_t size;
   size_t attachments_offset;
                     size = sizeof(*pass);
   size += pCreateInfo->subpassCount * sizeof(pass->subpasses[0]);
   attachments_offset = size;
            pass = vk_object_zalloc(&device->vk, pAllocator, size,
         if (pass == NULL)
            pass->attachment_count = pCreateInfo->attachmentCount;
   pass->subpass_count = pCreateInfo->subpassCount;
            vk_foreach_struct_const(ext, pCreateInfo->pNext) {
      switch (ext->sType) {
   case VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO:
      multiview_info = (VkRenderPassMultiviewCreateInfo *)ext;
      default:
                     for (uint32_t i = 0; i < pCreateInfo->attachmentCount; i++) {
               att->format =
         att->samples = pCreateInfo->pAttachments[i].samples;
   att->load_op = pCreateInfo->pAttachments[i].loadOp;
   att->stencil_load_op = pCreateInfo->pAttachments[i].stencilLoadOp;
   att->initial_layout = pCreateInfo->pAttachments[i].initialLayout;
   att->final_layout = pCreateInfo->pAttachments[i].finalLayout;
   att->store_op = pCreateInfo->pAttachments[i].storeOp;
   att->stencil_store_op = pCreateInfo->pAttachments[i].stencilStoreOp;
               uint32_t subpass_attachment_count = 0;
   struct panvk_subpass_attachment *p;
   for (uint32_t i = 0; i < pCreateInfo->subpassCount; i++) {
               subpass_attachment_count +=
      desc->inputAttachmentCount + desc->colorAttachmentCount +
   (desc->pResolveAttachments ? desc->colorAttachmentCount : 0) +
            if (subpass_attachment_count) {
      pass->subpass_attachments = vk_alloc2(
      &device->vk.alloc, pAllocator,
   subpass_attachment_count * sizeof(struct panvk_subpass_attachment), 8,
      if (pass->subpass_attachments == NULL) {
      vk_object_free(&device->vk, pAllocator, pass);
                  p = pass->subpass_attachments;
   for (uint32_t i = 0; i < pCreateInfo->subpassCount; i++) {
      const VkSubpassDescription2 *desc = &pCreateInfo->pSubpasses[i];
            subpass->input_count = desc->inputAttachmentCount;
   subpass->color_count = desc->colorAttachmentCount;
   if (multiview_info)
            if (desc->inputAttachmentCount > 0) {
                     for (uint32_t j = 0; j < desc->inputAttachmentCount; j++) {
      subpass->input_attachments[j] = (struct panvk_subpass_attachment){
      .idx = desc->pInputAttachments[j].attachment,
      };
   if (desc->pInputAttachments[j].attachment != VK_ATTACHMENT_UNUSED)
      pass->attachments[desc->pInputAttachments[j].attachment]
               if (desc->colorAttachmentCount > 0) {
                                       subpass->color_attachments[j] = (struct panvk_subpass_attachment){
                        if (idx != VK_ATTACHMENT_UNUSED) {
      pass->attachments[idx].view_mask |= subpass->view_mask;
   if (pass->attachments[idx].first_used_in_subpass == ~0) {
      pass->attachments[idx].first_used_in_subpass = i;
   if (pass->attachments[idx].load_op ==
      VK_ATTACHMENT_LOAD_OP_CLEAR)
      else if (pass->attachments[idx].load_op ==
            } else {
                           if (desc->pResolveAttachments) {
                                       subpass->resolve_attachments[j] = (struct panvk_subpass_attachment){
                        if (idx != VK_ATTACHMENT_UNUSED)
                  unsigned idx = desc->pDepthStencilAttachment
               subpass->zs_attachment.idx = idx;
   if (idx != VK_ATTACHMENT_UNUSED) {
                     if (pass->attachments[idx].first_used_in_subpass == ~0) {
      pass->attachments[idx].first_used_in_subpass = i;
   if (pass->attachments[idx].load_op == VK_ATTACHMENT_LOAD_OP_CLEAR)
         else if (pass->attachments[idx].load_op ==
            } else {
                        *pRenderPass = panvk_render_pass_to_handle(pass);
      }
      void
   panvk_DestroyRenderPass(VkDevice _device, VkRenderPass _pass,
         {
      VK_FROM_HANDLE(panvk_device, device, _device);
            if (!pass)
            vk_free2(&device->vk.alloc, pAllocator, pass->subpass_attachments);
      }
      void
   panvk_GetRenderAreaGranularity(VkDevice _device, VkRenderPass renderPass,
         {
      /* TODO: Return the actual tile size for the render pass? */
      }
