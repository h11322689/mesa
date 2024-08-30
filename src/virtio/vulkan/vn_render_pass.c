   /*
   * Copyright 2019 Google LLC
   * SPDX-License-Identifier: MIT
   *
   * based in part on anv and radv which are:
   * Copyright © 2015 Intel Corporation
   * Copyright © 2016 Red Hat.
   * Copyright © 2016 Bas Nieuwenhuizen
   */
      #include "vn_render_pass.h"
      #include "venus-protocol/vn_protocol_driver_framebuffer.h"
   #include "venus-protocol/vn_protocol_driver_render_pass.h"
   #include "vk_format.h"
      #include "vn_device.h"
   #include "vn_image.h"
      #define COUNT_PRESENT_SRC(atts, att_count, initial_count, final_count)       \
      do {                                                                      \
      *initial_count = 0;                                                    \
   *final_count = 0;                                                      \
   for (uint32_t i = 0; i < att_count; i++) {                             \
      if (atts[i].initialLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)       \
         if (atts[i].finalLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)         \
               #define REPLACE_PRESENT_SRC(pass, atts, att_count, out_atts)                 \
      do {                                                                      \
      struct vn_present_src_attachment *_acquire_atts =                      \
         struct vn_present_src_attachment *_release_atts =                      \
      pass->present_release_attachments;                                  \
      memcpy(out_atts, atts, sizeof(*atts) * att_count);                     \
   for (uint32_t i = 0; i < att_count; i++) {                             \
      if (out_atts[i].initialLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) { \
      out_atts[i].initialLayout = VN_PRESENT_SRC_INTERNAL_LAYOUT;      \
   _acquire_atts->index = i;                                        \
      }                                                                   \
   if (out_atts[i].finalLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {   \
      out_atts[i].finalLayout = VN_PRESENT_SRC_INTERNAL_LAYOUT;        \
   _release_atts->index = i;                                        \
                  #define INIT_SUBPASSES(_pass, _pCreateInfo)                                  \
      do {                                                                      \
      for (uint32_t i = 0; i < _pCreateInfo->subpassCount; i++) {            \
      __auto_type subpass_desc = &_pCreateInfo->pSubpasses[i];            \
   struct vn_subpass *subpass = &_pass->subpasses[i];                  \
         for (uint32_t j = 0; j < subpass_desc->colorAttachmentCount; j++) { \
      if (subpass_desc->pColorAttachments[j].attachment !=             \
      VK_ATTACHMENT_UNUSED) {                                      \
   subpass->attachment_aspects |= VK_IMAGE_ASPECT_COLOR_BIT;     \
         }                                                                   \
         if (subpass_desc->pDepthStencilAttachment &&                        \
      subpass_desc->pDepthStencilAttachment->attachment !=            \
         uint32_t att =                                                   \
         subpass->attachment_aspects |=                                   \
                  static void
   vn_render_pass_count_present_src(const VkRenderPassCreateInfo *create_info,
               {
      COUNT_PRESENT_SRC(create_info->pAttachments, create_info->attachmentCount,
      }
      static void
   vn_render_pass_count_present_src2(const VkRenderPassCreateInfo2 *create_info,
               {
      COUNT_PRESENT_SRC(create_info->pAttachments, create_info->attachmentCount,
      }
      static void
   vn_render_pass_replace_present_src(struct vn_render_pass *pass,
               {
      REPLACE_PRESENT_SRC(pass, create_info->pAttachments,
      }
      static void
   vn_render_pass_replace_present_src2(struct vn_render_pass *pass,
               {
      REPLACE_PRESENT_SRC(pass, create_info->pAttachments,
      }
      static void
   vn_render_pass_setup_present_src_barriers(struct vn_render_pass *pass)
   {
               for (uint32_t i = 0; i < pass->present_acquire_count; i++) {
      struct vn_present_src_attachment *att =
            att->src_stage_mask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
   att->src_access_mask = 0;
   att->dst_stage_mask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
   att->dst_access_mask =
               for (uint32_t i = 0; i < pass->present_release_count; i++) {
      struct vn_present_src_attachment *att =
            att->src_stage_mask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
   att->src_access_mask = VK_ACCESS_MEMORY_WRITE_BIT;
   att->dst_stage_mask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
         }
      static struct vn_render_pass *
   vn_render_pass_create(struct vn_device *dev,
                           {
      uint32_t present_count = present_acquire_count + present_release_count;
   struct vn_render_pass *pass;
   struct vn_present_src_attachment *present_atts;
            VK_MULTIALLOC(ma);
   vk_multialloc_add(&ma, &pass, __typeof__(*pass), 1);
   vk_multialloc_add(&ma, &present_atts, __typeof__(*present_atts),
                  if (!vk_multialloc_zalloc(&ma, alloc, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT))
                     pass->present_count = present_count;
   pass->present_acquire_count = present_acquire_count;
   pass->present_release_count = present_release_count;
            /* For each array pointer, set it only if its count != 0. This allows code
   * elsewhere to intuitively use either condition, `foo_atts == NULL` or
   * `foo_count != 0`.
   */
   if (present_count)
         if (present_acquire_count)
         if (present_release_count)
      pass->present_release_attachments =
      if (subpass_count)
               }
      /* render pass commands */
      VkResult
   vn_CreateRenderPass(VkDevice device,
                     {
      struct vn_device *dev = vn_device_from_handle(device);
   const VkAllocationCallbacks *alloc =
            uint32_t acquire_count;
   uint32_t release_count;
   vn_render_pass_count_present_src(pCreateInfo, &acquire_count,
            struct vn_render_pass *pass = vn_render_pass_create(
         if (!pass)
                     VkRenderPassCreateInfo local_pass_info;
   if (pass->present_count) {
      VkAttachmentDescription *temp_atts =
      vk_alloc(alloc, sizeof(*temp_atts) * pCreateInfo->attachmentCount,
      if (!temp_atts) {
      vk_free(alloc, pass);
               vn_render_pass_replace_present_src(pass, pCreateInfo, temp_atts);
            local_pass_info = *pCreateInfo;
   local_pass_info.pAttachments = temp_atts;
               const struct VkRenderPassMultiviewCreateInfo *multiview_info =
      vk_find_struct_const(pCreateInfo->pNext,
         /* Store the viewMask of each subpass for query feedback */
   if (multiview_info) {
      for (uint32_t i = 0; i < multiview_info->subpassCount; i++)
               VkRenderPass pass_handle = vn_render_pass_to_handle(pass);
   vn_async_vkCreateRenderPass(dev->instance, device, pCreateInfo, NULL,
            if (pCreateInfo == &local_pass_info)
                        }
      VkResult
   vn_CreateRenderPass2(VkDevice device,
                     {
      struct vn_device *dev = vn_device_from_handle(device);
   const VkAllocationCallbacks *alloc =
            uint32_t acquire_count;
   uint32_t release_count;
   vn_render_pass_count_present_src2(pCreateInfo, &acquire_count,
            struct vn_render_pass *pass = vn_render_pass_create(
         if (!pass)
                     VkRenderPassCreateInfo2 local_pass_info;
   if (pass->present_count) {
      VkAttachmentDescription2 *temp_atts =
      vk_alloc(alloc, sizeof(*temp_atts) * pCreateInfo->attachmentCount,
      if (!temp_atts) {
      vk_free(alloc, pass);
               vn_render_pass_replace_present_src2(pass, pCreateInfo, temp_atts);
            local_pass_info = *pCreateInfo;
   local_pass_info.pAttachments = temp_atts;
               /* Store the viewMask of each subpass for query feedback */
   for (uint32_t i = 0; i < pCreateInfo->subpassCount; i++)
            VkRenderPass pass_handle = vn_render_pass_to_handle(pass);
   vn_async_vkCreateRenderPass2(dev->instance, device, pCreateInfo, NULL,
            if (pCreateInfo == &local_pass_info)
                        }
      void
   vn_DestroyRenderPass(VkDevice device,
               {
      struct vn_device *dev = vn_device_from_handle(device);
   struct vn_render_pass *pass = vn_render_pass_from_handle(renderPass);
   const VkAllocationCallbacks *alloc =
            if (!pass)
                     vn_object_base_fini(&pass->base);
      }
      void
   vn_GetRenderAreaGranularity(VkDevice device,
               {
      struct vn_device *dev = vn_device_from_handle(device);
            if (!pass->granularity.width) {
      vn_call_vkGetRenderAreaGranularity(dev->instance, device, renderPass,
                  }
      /* framebuffer commands */
      VkResult
   vn_CreateFramebuffer(VkDevice device,
                     {
      struct vn_device *dev = vn_device_from_handle(device);
   const VkAllocationCallbacks *alloc =
            /* Two render passes differ only in attachment image layouts are considered
   * compatible.  We must not use pCreateInfo->renderPass here.
   */
   const bool imageless =
                  struct vn_framebuffer *fb =
      vk_zalloc(alloc, sizeof(*fb) + sizeof(*fb->image_views) * view_count,
      if (!fb)
                     fb->image_view_count = view_count;
   memcpy(fb->image_views, pCreateInfo->pAttachments,
            VkFramebuffer fb_handle = vn_framebuffer_to_handle(fb);
   vn_async_vkCreateFramebuffer(dev->instance, device, pCreateInfo, NULL,
                        }
      void
   vn_DestroyFramebuffer(VkDevice device,
               {
      struct vn_device *dev = vn_device_from_handle(device);
   struct vn_framebuffer *fb = vn_framebuffer_from_handle(framebuffer);
   const VkAllocationCallbacks *alloc =
            if (!fb)
                     vn_object_base_fini(&fb->base);
      }
