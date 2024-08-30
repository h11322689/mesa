   /*
   * Copyright 2019 Google LLC
   * SPDX-License-Identifier: MIT
   *
   * based in part on anv and radv which are:
   * Copyright © 2015 Intel Corporation
   * Copyright © 2016 Red Hat.
   * Copyright © 2016 Bas Nieuwenhuizen
   */
      #include "vn_command_buffer.h"
      #include "venus-protocol/vn_protocol_driver_command_buffer.h"
   #include "venus-protocol/vn_protocol_driver_command_pool.h"
      #include "vn_descriptor_set.h"
   #include "vn_device.h"
   #include "vn_image.h"
   #include "vn_query_pool.h"
   #include "vn_render_pass.h"
      static void
   vn_cmd_submit(struct vn_command_buffer *cmd);
      #define VN_CMD_ENQUEUE(cmd_name, commandBuffer, ...)                         \
      do {                                                                      \
      struct vn_command_buffer *_cmd =                                       \
         size_t _cmd_size = vn_sizeof_##cmd_name(commandBuffer, ##__VA_ARGS__); \
         if (vn_cs_encoder_reserve(&_cmd->cs, _cmd_size))                       \
         else                                                                   \
      _cmd->state = VN_COMMAND_BUFFER_STATE_INVALID;                      \
      if (VN_PERF(NO_CMD_BATCHING))                                          \
            static bool
   vn_image_memory_barrier_has_present_src(
         {
      for (uint32_t i = 0; i < count; i++) {
      if (img_barriers[i].oldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR ||
      img_barriers[i].newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
   }
      }
      static bool
   vn_dependency_info_has_present_src(uint32_t dep_count,
         {
      for (uint32_t i = 0; i < dep_count; i++) {
      for (uint32_t j = 0; j < dep_infos[i].imageMemoryBarrierCount; j++) {
      const VkImageMemoryBarrier2 *b =
         if (b->oldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR ||
      b->newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
                        }
      static void *
   vn_cmd_get_tmp_data(struct vn_command_buffer *cmd, size_t size)
   {
      struct vn_command_pool *pool = cmd->pool;
   /* avoid shrinking in case of non efficient reallocation implementation */
   if (size > pool->tmp.size) {
      void *data =
      vk_realloc(&pool->allocator, pool->tmp.data, size, VN_DEFAULT_ALIGN,
      if (!data)
            pool->tmp.data = data;
                  }
      static inline VkImageMemoryBarrier *
   vn_cmd_get_image_memory_barriers(struct vn_command_buffer *cmd,
         {
         }
      /* About VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, the spec says
   *
   *    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR must only be used for presenting a
   *    presentable image for display. A swapchain's image must be transitioned
   *    to this layout before calling vkQueuePresentKHR, and must be
   *    transitioned away from this layout after calling vkAcquireNextImageKHR.
   *
   * That allows us to treat the layout internally as
   *
   *  - VK_IMAGE_LAYOUT_GENERAL
   *  - VK_QUEUE_FAMILY_FOREIGN_EXT has the ownership, if the image is not a
   *    prime blit source
   *
   * while staying performant.
   *
   * About queue family ownerships, the spec says
   *
   *    A queue family can take ownership of an image subresource or buffer
   *    range of a resource created with VK_SHARING_MODE_EXCLUSIVE, without an
   *    ownership transfer, in the same way as for a resource that was just
   *    created; however, taking ownership in this way has the effect that the
   *    contents of the image subresource or buffer range are undefined.
   *
   * It is unclear if that is applicable to external resources, which supposedly
   * have the same semantics
   *
   *    Binding a resource to a memory object shared between multiple Vulkan
   *    instances or other APIs does not change the ownership of the underlying
   *    memory. The first entity to access the resource implicitly acquires
   *    ownership. Accessing a resource backed by memory that is owned by a
   *    particular instance or API has the same semantics as accessing a
   *    VK_SHARING_MODE_EXCLUSIVE resource[...]
   *
   * We should get the spec clarified, or get rid of this completely broken code
   * (TODO).
   *
   * Assuming a queue family can acquire the ownership implicitly when the
   * contents are not needed, we do not need to worry about
   * VK_IMAGE_LAYOUT_UNDEFINED.  We can use VK_IMAGE_LAYOUT_PRESENT_SRC_KHR as
   * the sole signal to trigger queue family ownership transfers.
   *
   * When the image has VK_SHARING_MODE_CONCURRENT, we can, and are required to,
   * use VK_QUEUE_FAMILY_IGNORED as the other queue family whether we are
   * transitioning to or from VK_IMAGE_LAYOUT_PRESENT_SRC_KHR.
   *
   * When the image has VK_SHARING_MODE_EXCLUSIVE, we have to work out who the
   * other queue family is.  It is easier when the barrier does not also define
   * a queue family ownership transfer (i.e., srcQueueFamilyIndex equals to
   * dstQueueFamilyIndex).  The other queue family must be the queue family the
   * command buffer was allocated for.
   *
   * When the barrier also defines a queue family ownership transfer, it is
   * submitted both to the source queue family to release the ownership and to
   * the destination queue family to acquire the ownership.  Depending on
   * whether the barrier transitions to or from VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
   * we are only interested in the ownership release or acquire respectively and
   * should be careful to avoid double releases/acquires.
   *
   * I haven't followed all transition paths mentally to verify the correctness.
   * I likely also violate some VUs or miss some cases below.  They are
   * hopefully fixable and are left as TODOs.
   */
   static void
   vn_cmd_fix_image_memory_barrier(const struct vn_command_buffer *cmd,
               {
                        /* no fix needed */
   if (out_barrier->oldLayout != VK_IMAGE_LAYOUT_PRESENT_SRC_KHR &&
      out_barrier->newLayout != VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
                  if (VN_PRESENT_SRC_INTERNAL_LAYOUT == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
            /* prime blit src or no layout transition */
   if (img->wsi.is_prime_blit_src ||
      out_barrier->oldLayout == out_barrier->newLayout) {
   if (out_barrier->oldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
         if (out_barrier->newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
                     if (out_barrier->oldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
               /* no availability operation needed */
            const uint32_t dst_qfi = out_barrier->dstQueueFamilyIndex;
   if (img->sharing_mode == VK_SHARING_MODE_CONCURRENT) {
      out_barrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_FOREIGN_EXT;
      } else if (dst_qfi == out_barrier->srcQueueFamilyIndex ||
            out_barrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_FOREIGN_EXT;
      } else {
      /* The barrier also defines a queue family ownership transfer, and
   * this is the one that gets submitted to the source queue family to
   * release the ownership.  Skip both the transfer and the transition.
   */
   out_barrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   out_barrier->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
         } else {
               /* no visibility operation needed */
            const uint32_t src_qfi = out_barrier->srcQueueFamilyIndex;
   if (img->sharing_mode == VK_SHARING_MODE_CONCURRENT) {
      out_barrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      } else if (src_qfi == out_barrier->dstQueueFamilyIndex ||
            out_barrier->srcQueueFamilyIndex = cmd->pool->queue_family_index;
      } else {
      /* The barrier also defines a queue family ownership transfer, and
   * this is the one that gets submitted to the destination queue
   * family to acquire the ownership.  Skip both the transfer and the
   * transition.
   */
   out_barrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   out_barrier->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            }
      /** See vn_cmd_fix_image_memory_barrier(). */
   static void
   vn_cmd_fix_image_memory_barrier2(const struct vn_command_buffer *cmd,
         {
      if (VN_PRESENT_SRC_INTERNAL_LAYOUT == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
            if (b->oldLayout != VK_IMAGE_LAYOUT_PRESENT_SRC_KHR &&
      b->newLayout != VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
         const struct vn_image *img = vn_image_from_handle(b->image);
            if (img->wsi.is_prime_blit_src || b->oldLayout == b->newLayout) {
      if (b->oldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
         if (b->newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
                     if (b->oldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
               /* no availability operation needed */
   b->srcStageMask = 0;
            if (img->sharing_mode == VK_SHARING_MODE_CONCURRENT) {
      b->srcQueueFamilyIndex = VK_QUEUE_FAMILY_FOREIGN_EXT;
      } else if (b->dstQueueFamilyIndex == b->srcQueueFamilyIndex ||
            b->srcQueueFamilyIndex = VK_QUEUE_FAMILY_FOREIGN_EXT;
      } else {
      /* The barrier also defines a queue family ownership transfer, and
   * this is the one that gets submitted to the source queue family to
   * release the ownership.  Skip both the transfer and the transition.
   */
   b->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   b->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
         } else {
               /* no visibility operation needed */
   b->dstStageMask = 0;
            if (img->sharing_mode == VK_SHARING_MODE_CONCURRENT) {
      b->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      } else if (b->srcQueueFamilyIndex == b->dstQueueFamilyIndex ||
            b->srcQueueFamilyIndex = cmd->pool->queue_family_index;
      } else {
      /* The barrier also defines a queue family ownership transfer, and
   * this is the one that gets submitted to the destination queue
   * family to acquire the ownership.  Skip both the transfer and the
   * transition.
   */
   b->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   b->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            }
      static const VkImageMemoryBarrier *
   vn_cmd_wait_events_fix_image_memory_barriers(
      struct vn_command_buffer *cmd,
   const VkImageMemoryBarrier *src_barriers,
   uint32_t count,
      {
               if (cmd->builder.in_render_pass ||
      !vn_image_memory_barrier_has_present_src(src_barriers, count))
         VkImageMemoryBarrier *img_barriers =
         if (!img_barriers) {
      cmd->state = VN_COMMAND_BUFFER_STATE_INVALID;
               /* vkCmdWaitEvents cannot be used for queue family ownership transfers.
   * Nothing appears to be said about the submission order of image memory
   * barriers in the same array.  We take the liberty to move queue family
   * ownership transfers to the tail.
   */
   VkImageMemoryBarrier *transfer_barriers = img_barriers + count;
   uint32_t transfer_count = 0;
   uint32_t valid_count = 0;
   for (uint32_t i = 0; i < count; i++) {
      VkImageMemoryBarrier *img_barrier = &img_barriers[valid_count];
            if (VN_PRESENT_SRC_INTERNAL_LAYOUT == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
      valid_count++;
               if (img_barrier->srcQueueFamilyIndex ==
      img_barrier->dstQueueFamilyIndex) {
      } else {
                     assert(valid_count + transfer_count == count);
   if (transfer_count) {
      /* copy back to the tail */
   memcpy(&img_barriers[valid_count], transfer_barriers,
                        }
      static const VkImageMemoryBarrier *
   vn_cmd_pipeline_barrier_fix_image_memory_barriers(
      struct vn_command_buffer *cmd,
   const VkImageMemoryBarrier *src_barriers,
      {
      if (cmd->builder.in_render_pass ||
      !vn_image_memory_barrier_has_present_src(src_barriers, count))
         VkImageMemoryBarrier *img_barriers =
         if (!img_barriers) {
      cmd->state = VN_COMMAND_BUFFER_STATE_INVALID;
               for (uint32_t i = 0; i < count; i++) {
      vn_cmd_fix_image_memory_barrier(cmd, &src_barriers[i],
                  }
      static const VkDependencyInfo *
   vn_cmd_fix_dependency_infos(struct vn_command_buffer *cmd,
               {
      if (cmd->builder.in_render_pass ||
      !vn_dependency_info_has_present_src(dep_count, dep_infos))
         uint32_t total_barrier_count = 0;
   for (uint32_t i = 0; i < dep_count; i++)
            size_t tmp_size = dep_count * sizeof(VkDependencyInfo) +
         void *tmp = vn_cmd_get_tmp_data(cmd, tmp_size);
   if (!tmp) {
      cmd->state = VN_COMMAND_BUFFER_STATE_INVALID;
               VkDependencyInfo *new_dep_infos = tmp;
   tmp += dep_count * sizeof(VkDependencyInfo);
            for (uint32_t i = 0; i < dep_count; i++) {
               VkImageMemoryBarrier2 *new_barriers = tmp;
            memcpy(new_barriers, dep_infos[i].pImageMemoryBarriers,
                  for (uint32_t j = 0; j < barrier_count; j++) {
                        }
      static void
   vn_cmd_encode_memory_barriers(struct vn_command_buffer *cmd,
                                 VkPipelineStageFlags src_stage_mask,
   {
               VN_CMD_ENQUEUE(vkCmdPipelineBarrier, cmd_handle, src_stage_mask,
            }
      static void
   vn_present_src_attachment_to_image_memory_barrier(
      const struct vn_image *img,
   const struct vn_present_src_attachment *att,
   VkImageMemoryBarrier *img_barrier,
      {
      *img_barrier = (VkImageMemoryBarrier)
   {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
   .srcAccessMask = att->src_access_mask,
   .dstAccessMask = att->dst_access_mask,
   .oldLayout = acquire ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
         .newLayout = acquire ? VN_PRESENT_SRC_INTERNAL_LAYOUT
         .image = vn_image_to_handle((struct vn_image *)img),
   .subresourceRange = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
   .levelCount = 1,
            }
      static void
   vn_cmd_transfer_present_src_images(
      struct vn_command_buffer *cmd,
   bool acquire,
   const struct vn_image *const *images,
   const struct vn_present_src_attachment *atts,
      {
      VkImageMemoryBarrier *img_barriers =
         if (!img_barriers) {
      cmd->state = VN_COMMAND_BUFFER_STATE_INVALID;
               VkPipelineStageFlags src_stage_mask = 0;
   VkPipelineStageFlags dst_stage_mask = 0;
   for (uint32_t i = 0; i < count; i++) {
      src_stage_mask |= atts[i].src_stage_mask;
            vn_present_src_attachment_to_image_memory_barrier(
         vn_cmd_fix_image_memory_barrier(cmd, &img_barriers[i],
               if (VN_PRESENT_SRC_INTERNAL_LAYOUT == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
            vn_cmd_encode_memory_barriers(cmd, src_stage_mask, dst_stage_mask, 0, NULL,
      }
      struct vn_feedback_query_batch *
   vn_cmd_query_batch_alloc(struct vn_command_pool *pool,
                           {
      struct vn_feedback_query_batch *batch;
   if (list_is_empty(&pool->free_query_batches)) {
      batch = vk_alloc(&pool->allocator, sizeof(*batch), VN_DEFAULT_ALIGN,
         if (!batch)
      } else {
      batch = list_first_entry(&pool->free_query_batches,
                     batch->query_pool = query_pool;
   batch->query = query;
   batch->query_count = query_count;
               }
      static inline void
   vn_cmd_merge_batched_query_feedback(struct vn_command_buffer *primary_cmd,
         {
      list_for_each_entry_safe(struct vn_feedback_query_batch, secondary_batch,
               struct vn_feedback_query_batch *batch = vn_cmd_query_batch_alloc(
      primary_cmd->pool, secondary_batch->query_pool,
               if (!batch) {
      primary_cmd->state = VN_COMMAND_BUFFER_STATE_INVALID;
                     }
      static void
   vn_cmd_begin_render_pass(struct vn_command_buffer *cmd,
                     {
      assert(begin_info);
            cmd->builder.render_pass = pass;
   cmd->builder.in_render_pass = true;
   cmd->builder.subpass_index = 0;
            if (!pass->present_count)
            /* find fb attachments */
   const VkImageView *views;
   ASSERTED uint32_t view_count;
   if (fb->image_view_count) {
      views = fb->image_views;
      } else {
      const VkRenderPassAttachmentBeginInfo *imageless_info =
      vk_find_struct_const(begin_info->pNext,
      assert(imageless_info);
   views = imageless_info->pAttachments;
               const struct vn_image **images =
      vk_alloc(&cmd->pool->allocator, sizeof(*images) * pass->present_count,
      if (!images) {
      cmd->state = VN_COMMAND_BUFFER_STATE_INVALID;
               for (uint32_t i = 0; i < pass->present_count; i++) {
      const uint32_t index = pass->present_attachments[i].index;
   assert(index < view_count);
               if (pass->present_acquire_count) {
      vn_cmd_transfer_present_src_images(cmd, true, images,
                        }
      static void
   vn_cmd_end_render_pass(struct vn_command_buffer *cmd)
   {
      const struct vn_render_pass *pass = cmd->builder.render_pass;
            cmd->builder.render_pass = NULL;
   cmd->builder.present_src_images = NULL;
   cmd->builder.in_render_pass = false;
   cmd->builder.subpass_index = 0;
            if (!pass->present_count || !images)
            if (pass->present_release_count) {
      vn_cmd_transfer_present_src_images(
      cmd, false, images + pass->present_acquire_count,
               }
      static inline void
   vn_cmd_next_subpass(struct vn_command_buffer *cmd)
   {
      cmd->builder.view_mask = vn_render_pass_get_subpass_view_mask(
      }
      static inline void
   vn_cmd_begin_rendering(struct vn_command_buffer *cmd,
         {
      cmd->builder.in_render_pass = true;
      }
      static inline void
   vn_cmd_end_rendering(struct vn_command_buffer *cmd)
   {
      cmd->builder.in_render_pass = false;
      }
      /* command pool commands */
      VkResult
   vn_CreateCommandPool(VkDevice device,
                     {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   const VkAllocationCallbacks *alloc =
            struct vn_command_pool *pool =
      vk_zalloc(alloc, sizeof(*pool), VN_DEFAULT_ALIGN,
      if (!pool)
                     pool->allocator = *alloc;
   pool->device = dev;
   pool->queue_family_index = pCreateInfo->queueFamilyIndex;
   list_inithead(&pool->command_buffers);
            VkCommandPool pool_handle = vn_command_pool_to_handle(pool);
   vn_async_vkCreateCommandPool(dev->instance, device, pCreateInfo, NULL,
                        }
      static inline void
   vn_recycle_query_feedback_cmd(struct vn_command_buffer *cmd)
   {
      vn_ResetCommandBuffer(
         list_add(&cmd->linked_query_feedback_cmd->head,
            }
      void
   vn_DestroyCommandPool(VkDevice device,
               {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   struct vn_command_pool *pool = vn_command_pool_from_handle(commandPool);
            if (!pool)
                     /* We must emit vkDestroyCommandPool before freeing the command buffers in
   * pool->command_buffers.  Otherwise, another thread might reuse their
   * object ids while they still refer to the command buffers in the
   * renderer.
   */
            list_for_each_entry_safe(struct vn_command_buffer, cmd,
            vn_cs_encoder_fini(&cmd->cs);
            if (cmd->builder.present_src_images)
            list_for_each_entry_safe(struct vn_feedback_query_batch, batch,
                  if (cmd->linked_query_feedback_cmd)
                        list_for_each_entry_safe(struct vn_feedback_query_batch, batch,
                  if (pool->tmp.data)
            vn_object_base_fini(&pool->base);
      }
      static void
   vn_cmd_reset(struct vn_command_buffer *cmd)
   {
               cmd->state = VN_COMMAND_BUFFER_STATE_INITIAL;
            if (cmd->builder.present_src_images)
            list_for_each_entry_safe(struct vn_feedback_query_batch, batch,
                  if (cmd->linked_query_feedback_cmd)
                        }
      VkResult
   vn_ResetCommandPool(VkDevice device,
               {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
            list_for_each_entry_safe(struct vn_command_buffer, cmd,
                              }
      void
   vn_TrimCommandPool(VkDevice device,
               {
      VN_TRACE_FUNC();
               }
      /* command buffer commands */
      VkResult
   vn_AllocateCommandBuffers(VkDevice device,
               {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   struct vn_command_pool *pool =
                  for (uint32_t i = 0; i < pAllocateInfo->commandBufferCount; i++) {
      struct vn_command_buffer *cmd =
      vk_zalloc(alloc, sizeof(*cmd), VN_DEFAULT_ALIGN,
      if (!cmd) {
      for (uint32_t j = 0; j < i; j++) {
      cmd = vn_command_buffer_from_handle(pCommandBuffers[j]);
   vn_cs_encoder_fini(&cmd->cs);
   list_del(&cmd->head);
   vn_object_base_fini(&cmd->base);
      }
   memset(pCommandBuffers, 0,
                     vn_object_base_init(&cmd->base, VK_OBJECT_TYPE_COMMAND_BUFFER,
         cmd->pool = pool;
   cmd->level = pAllocateInfo->level;
   cmd->state = VN_COMMAND_BUFFER_STATE_INITIAL;
   vn_cs_encoder_init(&cmd->cs, dev->instance,
                              VkCommandBuffer cmd_handle = vn_command_buffer_to_handle(cmd);
               vn_async_vkAllocateCommandBuffers(dev->instance, device, pAllocateInfo,
               }
      void
   vn_FreeCommandBuffers(VkDevice device,
                     {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   struct vn_command_pool *pool = vn_command_pool_from_handle(commandPool);
            vn_async_vkFreeCommandBuffers(dev->instance, device, commandPool,
            for (uint32_t i = 0; i < commandBufferCount; i++) {
      struct vn_command_buffer *cmd =
            if (!cmd)
            vn_cs_encoder_fini(&cmd->cs);
            if (cmd->builder.present_src_images)
            list_for_each_entry_safe(struct vn_feedback_query_batch, batch,
                  if (cmd->linked_query_feedback_cmd)
            vn_object_base_fini(&cmd->base);
         }
      VkResult
   vn_ResetCommandBuffer(VkCommandBuffer commandBuffer,
         {
      VN_TRACE_FUNC();
   struct vn_command_buffer *cmd =
                                       }
      struct vn_command_buffer_begin_info {
      VkCommandBufferBeginInfo begin;
   VkCommandBufferInheritanceInfo inheritance;
            bool has_inherited_pass;
      };
      static const VkCommandBufferBeginInfo *
   vn_fix_command_buffer_begin_info(struct vn_command_buffer *cmd,
               {
               if (!begin_info->pInheritanceInfo)
            const bool is_cmd_secondary =
         const bool has_continue =
         const bool has_renderpass =
      is_cmd_secondary &&
         /* Per spec 1.3.255: "VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT
   * specifies that a secondary command buffer is considered to be
   * entirely inside a render pass. If this is a primary command buffer,
   * then this bit is ignored."
   */
            /* Can early-return if dynamic rendering is used and no structures need to
   * be dropped from the pNext chain of VkCommandBufferInheritanceInfo.
   */
   if (is_cmd_secondary && has_continue && !has_renderpass)
                     if (!is_cmd_secondary) {
      local->begin.pInheritanceInfo = NULL;
               local->inheritance = *begin_info->pInheritanceInfo;
            if (!has_continue) {
      local->inheritance.framebuffer = VK_NULL_HANDLE;
   local->inheritance.renderPass = VK_NULL_HANDLE;
      } else {
      /* With early-returns above, it must be an inherited pass. */
               /* Per spec, about VkCommandBufferInheritanceRenderingInfo:
   *
   * If VkCommandBufferInheritanceInfo::renderPass is not VK_NULL_HANDLE, or
   * VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT is not specified in
   * VkCommandBufferBeginInfo::flags, parameters of this structure are
   * ignored.
   */
   VkBaseOutStructure *head = NULL;
   VkBaseOutStructure *tail = NULL;
   vk_foreach_struct_const(src, local->inheritance.pNext) {
      void *pnext = NULL;
   switch (src->sType) {
   case VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_CONDITIONAL_RENDERING_INFO_EXT:
      memcpy(
      &local->conditional_rendering, src,
      pnext = &local->conditional_rendering;
      case VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDERING_INFO:
   default:
                  if (pnext) {
      if (!head)
                              }
               }
      VkResult
   vn_BeginCommandBuffer(VkCommandBuffer commandBuffer,
         {
      VN_TRACE_FUNC();
   struct vn_command_buffer *cmd =
         struct vn_instance *instance = cmd->pool->device->instance;
            /* reset regardless of VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT */
            struct vn_command_buffer_begin_info local_begin_info;
   pBeginInfo =
            cmd_size = vn_sizeof_vkBeginCommandBuffer(commandBuffer, pBeginInfo);
   if (!vn_cs_encoder_reserve(&cmd->cs, cmd_size)) {
      cmd->state = VN_COMMAND_BUFFER_STATE_INVALID;
      }
   cmd->builder.is_simultaneous =
                              const VkCommandBufferInheritanceInfo *inheritance_info =
            if (inheritance_info) {
               if (local_begin_info.has_inherited_pass) {
      /* Store the viewMask from the inherited render pass subpass for
   * query feedback.
   */
   cmd->builder.view_mask = vn_render_pass_get_subpass_view_mask(
      vn_render_pass_from_handle(inheritance_info->renderPass),
   } else {
      /* Store the viewMask from the
   * VkCommandBufferInheritanceRenderingInfo.
   */
   const VkCommandBufferInheritanceRenderingInfo
      *inheritance_rendering_info = vk_find_struct_const(
      inheritance_info->pNext,
   if (inheritance_rendering_info)
                     }
      static void
   vn_cmd_submit(struct vn_command_buffer *cmd)
   {
               if (cmd->state != VN_COMMAND_BUFFER_STATE_RECORDING)
            vn_cs_encoder_commit(&cmd->cs);
   if (vn_cs_encoder_get_fatal(&cmd->cs)) {
      cmd->state = VN_COMMAND_BUFFER_STATE_INVALID;
   vn_cs_encoder_reset(&cmd->cs);
               if (vn_instance_ring_submit(instance, &cmd->cs) != VK_SUCCESS) {
      cmd->state = VN_COMMAND_BUFFER_STATE_INVALID;
               vn_cs_encoder_reset(&cmd->cs);
      }
      static inline void
   vn_cmd_count_draw_and_submit_on_batch_limit(struct vn_command_buffer *cmd)
   {
      if (++cmd->draw_cmd_batched >= vn_env.draw_cmd_batch_limit)
      }
      VkResult
   vn_EndCommandBuffer(VkCommandBuffer commandBuffer)
   {
      VN_TRACE_FUNC();
   struct vn_command_buffer *cmd =
         struct vn_instance *instance = cmd->pool->device->instance;
            if (cmd->state != VN_COMMAND_BUFFER_STATE_RECORDING)
            cmd_size = vn_sizeof_vkEndCommandBuffer(commandBuffer);
   if (!vn_cs_encoder_reserve(&cmd->cs, cmd_size)) {
      cmd->state = VN_COMMAND_BUFFER_STATE_INVALID;
                        vn_cmd_submit(cmd);
   if (cmd->state == VN_COMMAND_BUFFER_STATE_INVALID)
                        }
      void
   vn_CmdBindPipeline(VkCommandBuffer commandBuffer,
               {
      VN_CMD_ENQUEUE(vkCmdBindPipeline, commandBuffer, pipelineBindPoint,
      }
      void
   vn_CmdSetViewport(VkCommandBuffer commandBuffer,
                     {
      VN_CMD_ENQUEUE(vkCmdSetViewport, commandBuffer, firstViewport,
      }
      void
   vn_CmdSetScissor(VkCommandBuffer commandBuffer,
                     {
      VN_CMD_ENQUEUE(vkCmdSetScissor, commandBuffer, firstScissor, scissorCount,
      }
      void
   vn_CmdSetLineWidth(VkCommandBuffer commandBuffer, float lineWidth)
   {
         }
      void
   vn_CmdSetDepthBias(VkCommandBuffer commandBuffer,
                     {
      VN_CMD_ENQUEUE(vkCmdSetDepthBias, commandBuffer, depthBiasConstantFactor,
      }
      void
   vn_CmdSetBlendConstants(VkCommandBuffer commandBuffer,
         {
         }
      void
   vn_CmdSetDepthBounds(VkCommandBuffer commandBuffer,
               {
      VN_CMD_ENQUEUE(vkCmdSetDepthBounds, commandBuffer, minDepthBounds,
      }
      void
   vn_CmdSetStencilCompareMask(VkCommandBuffer commandBuffer,
               {
      VN_CMD_ENQUEUE(vkCmdSetStencilCompareMask, commandBuffer, faceMask,
      }
      void
   vn_CmdSetStencilWriteMask(VkCommandBuffer commandBuffer,
               {
      VN_CMD_ENQUEUE(vkCmdSetStencilWriteMask, commandBuffer, faceMask,
      }
      void
   vn_CmdSetStencilReference(VkCommandBuffer commandBuffer,
               {
      VN_CMD_ENQUEUE(vkCmdSetStencilReference, commandBuffer, faceMask,
      }
      void
   vn_CmdBindDescriptorSets(VkCommandBuffer commandBuffer,
                           VkPipelineBindPoint pipelineBindPoint,
   VkPipelineLayout layout,
   uint32_t firstSet,
   {
      VN_CMD_ENQUEUE(vkCmdBindDescriptorSets, commandBuffer, pipelineBindPoint,
            }
      void
   vn_CmdBindIndexBuffer(VkCommandBuffer commandBuffer,
                     {
      VN_CMD_ENQUEUE(vkCmdBindIndexBuffer, commandBuffer, buffer, offset,
      }
      void
   vn_CmdBindVertexBuffers(VkCommandBuffer commandBuffer,
                           {
      VN_CMD_ENQUEUE(vkCmdBindVertexBuffers, commandBuffer, firstBinding,
      }
      void
   vn_CmdDraw(VkCommandBuffer commandBuffer,
            uint32_t vertexCount,
   uint32_t instanceCount,
      {
      VN_CMD_ENQUEUE(vkCmdDraw, commandBuffer, vertexCount, instanceCount,
            vn_cmd_count_draw_and_submit_on_batch_limit(
      }
      void
   vn_CmdBeginRendering(VkCommandBuffer commandBuffer,
         {
      vn_cmd_begin_rendering(vn_command_buffer_from_handle(commandBuffer),
               }
      void
   vn_CmdEndRendering(VkCommandBuffer commandBuffer)
   {
                  }
      void
   vn_CmdDrawIndexed(VkCommandBuffer commandBuffer,
                     uint32_t indexCount,
   uint32_t instanceCount,
   {
      VN_CMD_ENQUEUE(vkCmdDrawIndexed, commandBuffer, indexCount, instanceCount,
            vn_cmd_count_draw_and_submit_on_batch_limit(
      }
      void
   vn_CmdDrawIndirect(VkCommandBuffer commandBuffer,
                     VkBuffer buffer,
   {
      VN_CMD_ENQUEUE(vkCmdDrawIndirect, commandBuffer, buffer, offset, drawCount,
            vn_cmd_count_draw_and_submit_on_batch_limit(
      }
      void
   vn_CmdDrawIndexedIndirect(VkCommandBuffer commandBuffer,
                           {
      VN_CMD_ENQUEUE(vkCmdDrawIndexedIndirect, commandBuffer, buffer, offset,
            vn_cmd_count_draw_and_submit_on_batch_limit(
      }
      void
   vn_CmdDrawIndirectCount(VkCommandBuffer commandBuffer,
                           VkBuffer buffer,
   VkDeviceSize offset,
   {
      VN_CMD_ENQUEUE(vkCmdDrawIndirectCount, commandBuffer, buffer, offset,
            vn_cmd_count_draw_and_submit_on_batch_limit(
      }
      void
   vn_CmdDrawIndexedIndirectCount(VkCommandBuffer commandBuffer,
                                 VkBuffer buffer,
   {
      VN_CMD_ENQUEUE(vkCmdDrawIndexedIndirectCount, commandBuffer, buffer,
                  vn_cmd_count_draw_and_submit_on_batch_limit(
      }
      void
   vn_CmdDispatch(VkCommandBuffer commandBuffer,
                     {
      VN_CMD_ENQUEUE(vkCmdDispatch, commandBuffer, groupCountX, groupCountY,
      }
      void
   vn_CmdDispatchIndirect(VkCommandBuffer commandBuffer,
               {
         }
      void
   vn_CmdCopyBuffer(VkCommandBuffer commandBuffer,
                  VkBuffer srcBuffer,
      {
      VN_CMD_ENQUEUE(vkCmdCopyBuffer, commandBuffer, srcBuffer, dstBuffer,
      }
      void
   vn_CmdCopyBuffer2(VkCommandBuffer commandBuffer,
         {
         }
      void
   vn_CmdCopyImage(VkCommandBuffer commandBuffer,
                  VkImage srcImage,
   VkImageLayout srcImageLayout,
   VkImage dstImage,
      {
      VN_CMD_ENQUEUE(vkCmdCopyImage, commandBuffer, srcImage, srcImageLayout,
      }
      void
   vn_CmdCopyImage2(VkCommandBuffer commandBuffer,
         {
         }
      void
   vn_CmdBlitImage(VkCommandBuffer commandBuffer,
                  VkImage srcImage,
   VkImageLayout srcImageLayout,
   VkImage dstImage,
   VkImageLayout dstImageLayout,
      {
      VN_CMD_ENQUEUE(vkCmdBlitImage, commandBuffer, srcImage, srcImageLayout,
      }
      void
   vn_CmdBlitImage2(VkCommandBuffer commandBuffer,
         {
         }
      void
   vn_CmdCopyBufferToImage(VkCommandBuffer commandBuffer,
                           VkBuffer srcBuffer,
   {
      VN_CMD_ENQUEUE(vkCmdCopyBufferToImage, commandBuffer, srcBuffer, dstImage,
      }
      void
   vn_CmdCopyBufferToImage2(
      VkCommandBuffer commandBuffer,
      {
      VN_CMD_ENQUEUE(vkCmdCopyBufferToImage2, commandBuffer,
      }
      static bool
   vn_needs_prime_blit(VkImage src_image, VkImageLayout src_image_layout)
   {
      if (src_image_layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR &&
               /* sanity check */
   ASSERTED const struct vn_image *img = vn_image_from_handle(src_image);
   assert(img->wsi.is_wsi && img->wsi.is_prime_blit_src);
                  }
      static void
   vn_transition_prime_layout(struct vn_command_buffer *cmd, VkBuffer dst_buffer)
   {
      const VkBufferMemoryBarrier buf_barrier = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
   .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
   .srcQueueFamilyIndex = cmd->pool->queue_family_index,
   .dstQueueFamilyIndex = VK_QUEUE_FAMILY_FOREIGN_EXT,
   .buffer = dst_buffer,
      };
   vn_cmd_encode_memory_barriers(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
            }
      void
   vn_CmdCopyImageToBuffer(VkCommandBuffer commandBuffer,
                           VkImage srcImage,
   {
      struct vn_command_buffer *cmd =
            bool prime_blit = vn_needs_prime_blit(srcImage, srcImageLayout);
   if (prime_blit)
            VN_CMD_ENQUEUE(vkCmdCopyImageToBuffer, commandBuffer, srcImage,
            if (prime_blit)
      }
      void
   vn_CmdCopyImageToBuffer2(
      VkCommandBuffer commandBuffer,
      {
      struct vn_command_buffer *cmd =
                  bool prime_blit =
         if (prime_blit)
                     if (prime_blit)
      }
      void
   vn_CmdUpdateBuffer(VkCommandBuffer commandBuffer,
                     VkBuffer dstBuffer,
   {
      VN_CMD_ENQUEUE(vkCmdUpdateBuffer, commandBuffer, dstBuffer, dstOffset,
      }
      void
   vn_CmdFillBuffer(VkCommandBuffer commandBuffer,
                  VkBuffer dstBuffer,
      {
      VN_CMD_ENQUEUE(vkCmdFillBuffer, commandBuffer, dstBuffer, dstOffset, size,
      }
      void
   vn_CmdClearColorImage(VkCommandBuffer commandBuffer,
                        VkImage image,
      {
      VN_CMD_ENQUEUE(vkCmdClearColorImage, commandBuffer, image, imageLayout,
      }
      void
   vn_CmdClearDepthStencilImage(VkCommandBuffer commandBuffer,
                                 {
      VN_CMD_ENQUEUE(vkCmdClearDepthStencilImage, commandBuffer, image,
      }
      void
   vn_CmdClearAttachments(VkCommandBuffer commandBuffer,
                           {
      VN_CMD_ENQUEUE(vkCmdClearAttachments, commandBuffer, attachmentCount,
      }
      void
   vn_CmdResolveImage(VkCommandBuffer commandBuffer,
                     VkImage srcImage,
   VkImageLayout srcImageLayout,
   VkImage dstImage,
   {
      VN_CMD_ENQUEUE(vkCmdResolveImage, commandBuffer, srcImage, srcImageLayout,
      }
      void
   vn_CmdResolveImage2(VkCommandBuffer commandBuffer,
         {
         }
      void
   vn_CmdSetEvent(VkCommandBuffer commandBuffer,
               {
               vn_feedback_event_cmd_record(commandBuffer, event, stageMask, VK_EVENT_SET,
      }
      static VkPipelineStageFlags2
   vn_dependency_info_collect_src_stage_mask(const VkDependencyInfo *dep_info)
   {
               for (uint32_t i = 0; i < dep_info->memoryBarrierCount; i++)
            for (uint32_t i = 0; i < dep_info->bufferMemoryBarrierCount; i++)
            for (uint32_t i = 0; i < dep_info->imageMemoryBarrierCount; i++)
               }
      void
   vn_CmdSetEvent2(VkCommandBuffer commandBuffer,
                  {
      struct vn_command_buffer *cmd =
                              VkPipelineStageFlags2 src_stage_mask =
            vn_feedback_event_cmd_record(commandBuffer, event, src_stage_mask,
      }
      void
   vn_CmdResetEvent(VkCommandBuffer commandBuffer,
               {
               vn_feedback_event_cmd_record(commandBuffer, event, stageMask,
      }
      void
   vn_CmdResetEvent2(VkCommandBuffer commandBuffer,
               {
      VN_CMD_ENQUEUE(vkCmdResetEvent2, commandBuffer, event, stageMask);
   vn_feedback_event_cmd_record(commandBuffer, event, stageMask,
      }
      void
   vn_CmdWaitEvents(VkCommandBuffer commandBuffer,
                  uint32_t eventCount,
   const VkEvent *pEvents,
   VkPipelineStageFlags srcStageMask,
   VkPipelineStageFlags dstStageMask,
   uint32_t memoryBarrierCount,
   const VkMemoryBarrier *pMemoryBarriers,
   uint32_t bufferMemoryBarrierCount,
      {
      struct vn_command_buffer *cmd =
                  pImageMemoryBarriers = vn_cmd_wait_events_fix_image_memory_barriers(
                  VN_CMD_ENQUEUE(vkCmdWaitEvents, commandBuffer, eventCount, pEvents,
                  srcStageMask, dstStageMask, memoryBarrierCount,
         if (transfer_count) {
      pImageMemoryBarriers += imageMemoryBarrierCount;
   vn_cmd_encode_memory_barriers(cmd, srcStageMask, dstStageMask, 0, NULL,
         }
      void
   vn_CmdWaitEvents2(VkCommandBuffer commandBuffer,
                     {
      struct vn_command_buffer *cmd =
            pDependencyInfos =
            VN_CMD_ENQUEUE(vkCmdWaitEvents2, commandBuffer, eventCount, pEvents,
      }
      void
   vn_CmdPipelineBarrier(VkCommandBuffer commandBuffer,
                        VkPipelineStageFlags srcStageMask,
   VkPipelineStageFlags dstStageMask,
   VkDependencyFlags dependencyFlags,
   uint32_t memoryBarrierCount,
   const VkMemoryBarrier *pMemoryBarriers,
      {
      struct vn_command_buffer *cmd =
            pImageMemoryBarriers = vn_cmd_pipeline_barrier_fix_image_memory_barriers(
            VN_CMD_ENQUEUE(vkCmdPipelineBarrier, commandBuffer, srcStageMask,
                  dstStageMask, dependencyFlags, memoryBarrierCount,
   }
      void
   vn_CmdPipelineBarrier2(VkCommandBuffer commandBuffer,
         {
      struct vn_command_buffer *cmd =
                        }
      void
   vn_CmdBeginQuery(VkCommandBuffer commandBuffer,
                     {
         }
      static inline void
   vn_cmd_add_query_feedback(VkCommandBuffer cmd_handle,
               {
      struct vn_command_buffer *cmd = vn_command_buffer_from_handle(cmd_handle);
            if (!query_pool->feedback)
            /* Per 1.3.255 spec "If queries are used while executing a render pass
   * instance that has multiview enabled, the query uses N consecutive
   * query indices in the query pool (starting at query) where N is the
   * number of bits set in the view mask in the subpass the query is used
   * in."
   */
   uint32_t query_count =
      (cmd->builder.in_render_pass && cmd->builder.view_mask)
               struct vn_feedback_query_batch *batch = vn_cmd_query_batch_alloc(
         if (!batch) {
      cmd->state = VN_COMMAND_BUFFER_STATE_INVALID;
                  }
      static inline void
   vn_cmd_add_query_reset_feedback(VkCommandBuffer cmd_handle,
                     {
      struct vn_command_buffer *cmd = vn_command_buffer_from_handle(cmd_handle);
            if (!query_pool->feedback)
            struct vn_feedback_query_batch *batch = vn_cmd_query_batch_alloc(
         if (!batch)
               }
      void
   vn_CmdEndQuery(VkCommandBuffer commandBuffer,
               {
                  }
      void
   vn_CmdResetQueryPool(VkCommandBuffer commandBuffer,
                     {
      VN_CMD_ENQUEUE(vkCmdResetQueryPool, commandBuffer, queryPool, firstQuery,
            vn_cmd_add_query_reset_feedback(commandBuffer, queryPool, firstQuery,
      }
      void
   vn_CmdWriteTimestamp(VkCommandBuffer commandBuffer,
                     {
      VN_CMD_ENQUEUE(vkCmdWriteTimestamp, commandBuffer, pipelineStage,
               }
      void
   vn_CmdWriteTimestamp2(VkCommandBuffer commandBuffer,
                     {
      VN_CMD_ENQUEUE(vkCmdWriteTimestamp2, commandBuffer, stage, queryPool,
               }
      void
   vn_CmdCopyQueryPoolResults(VkCommandBuffer commandBuffer,
                              VkQueryPool queryPool,
   uint32_t firstQuery,
      {
      VN_CMD_ENQUEUE(vkCmdCopyQueryPoolResults, commandBuffer, queryPool,
            }
      void
   vn_CmdPushConstants(VkCommandBuffer commandBuffer,
                     VkPipelineLayout layout,
   VkShaderStageFlags stageFlags,
   {
      VN_CMD_ENQUEUE(vkCmdPushConstants, commandBuffer, layout, stageFlags,
      }
      void
   vn_CmdBeginRenderPass(VkCommandBuffer commandBuffer,
               {
      struct vn_command_buffer *cmd =
            vn_cmd_begin_render_pass(
      cmd, vn_render_pass_from_handle(pRenderPassBegin->renderPass),
   vn_framebuffer_from_handle(pRenderPassBegin->framebuffer),
         VN_CMD_ENQUEUE(vkCmdBeginRenderPass, commandBuffer, pRenderPassBegin,
      }
      void
   vn_CmdNextSubpass(VkCommandBuffer commandBuffer, VkSubpassContents contents)
   {
                  }
      void
   vn_CmdEndRenderPass(VkCommandBuffer commandBuffer)
   {
                  }
      void
   vn_CmdBeginRenderPass2(VkCommandBuffer commandBuffer,
               {
      struct vn_command_buffer *cmd =
            vn_cmd_begin_render_pass(
      cmd, vn_render_pass_from_handle(pRenderPassBegin->renderPass),
   vn_framebuffer_from_handle(pRenderPassBegin->framebuffer),
         VN_CMD_ENQUEUE(vkCmdBeginRenderPass2, commandBuffer, pRenderPassBegin,
      }
      void
   vn_CmdNextSubpass2(VkCommandBuffer commandBuffer,
               {
               VN_CMD_ENQUEUE(vkCmdNextSubpass2, commandBuffer, pSubpassBeginInfo,
      }
      void
   vn_CmdEndRenderPass2(VkCommandBuffer commandBuffer,
         {
                  }
      void
   vn_CmdExecuteCommands(VkCommandBuffer commandBuffer,
               {
      VN_CMD_ENQUEUE(vkCmdExecuteCommands, commandBuffer, commandBufferCount,
            struct vn_command_buffer *primary_cmd =
         for (uint32_t i = 0; i < commandBufferCount; i++) {
      struct vn_command_buffer *secondary_cmd =
               }
      void
   vn_CmdSetDeviceMask(VkCommandBuffer commandBuffer, uint32_t deviceMask)
   {
         }
      void
   vn_CmdDispatchBase(VkCommandBuffer commandBuffer,
                     uint32_t baseGroupX,
   uint32_t baseGroupY,
   uint32_t baseGroupZ,
   {
      VN_CMD_ENQUEUE(vkCmdDispatchBase, commandBuffer, baseGroupX, baseGroupY,
      }
      void
   vn_CmdSetLineStippleEXT(VkCommandBuffer commandBuffer,
               {
      VN_CMD_ENQUEUE(vkCmdSetLineStippleEXT, commandBuffer, lineStippleFactor,
      }
      void
   vn_CmdBeginQueryIndexedEXT(VkCommandBuffer commandBuffer,
                           {
      VN_CMD_ENQUEUE(vkCmdBeginQueryIndexedEXT, commandBuffer, queryPool, query,
      }
      void
   vn_CmdEndQueryIndexedEXT(VkCommandBuffer commandBuffer,
                     {
      VN_CMD_ENQUEUE(vkCmdEndQueryIndexedEXT, commandBuffer, queryPool, query,
               }
      void
   vn_CmdBindTransformFeedbackBuffersEXT(VkCommandBuffer commandBuffer,
                                 {
      VN_CMD_ENQUEUE(vkCmdBindTransformFeedbackBuffersEXT, commandBuffer,
      }
      void
   vn_CmdBeginTransformFeedbackEXT(VkCommandBuffer commandBuffer,
                           {
      VN_CMD_ENQUEUE(vkCmdBeginTransformFeedbackEXT, commandBuffer,
            }
      void
   vn_CmdEndTransformFeedbackEXT(VkCommandBuffer commandBuffer,
                           {
      VN_CMD_ENQUEUE(vkCmdEndTransformFeedbackEXT, commandBuffer,
            }
      void
   vn_CmdDrawIndirectByteCountEXT(VkCommandBuffer commandBuffer,
                                 uint32_t instanceCount,
   {
      VN_CMD_ENQUEUE(vkCmdDrawIndirectByteCountEXT, commandBuffer, instanceCount,
                  vn_cmd_count_draw_and_submit_on_batch_limit(
      }
      void
   vn_CmdBindVertexBuffers2(VkCommandBuffer commandBuffer,
                           uint32_t firstBinding,
   uint32_t bindingCount,
   {
      VN_CMD_ENQUEUE(vkCmdBindVertexBuffers2, commandBuffer, firstBinding,
      }
      void
   vn_CmdSetCullMode(VkCommandBuffer commandBuffer, VkCullModeFlags cullMode)
   {
         }
      void
   vn_CmdSetDepthBoundsTestEnable(VkCommandBuffer commandBuffer,
         {
      VN_CMD_ENQUEUE(vkCmdSetDepthBoundsTestEnable, commandBuffer,
      }
      void
   vn_CmdSetDepthCompareOp(VkCommandBuffer commandBuffer,
         {
         }
      void
   vn_CmdSetDepthTestEnable(VkCommandBuffer commandBuffer,
         {
         }
      void
   vn_CmdSetDepthWriteEnable(VkCommandBuffer commandBuffer,
         {
         }
      void
   vn_CmdSetFrontFace(VkCommandBuffer commandBuffer, VkFrontFace frontFace)
   {
         }
      void
   vn_CmdSetPrimitiveTopology(VkCommandBuffer commandBuffer,
         {
      VN_CMD_ENQUEUE(vkCmdSetPrimitiveTopology, commandBuffer,
      }
      void
   vn_CmdSetScissorWithCount(VkCommandBuffer commandBuffer,
               {
      VN_CMD_ENQUEUE(vkCmdSetScissorWithCount, commandBuffer, scissorCount,
      }
      void
   vn_CmdSetStencilOp(VkCommandBuffer commandBuffer,
                     VkStencilFaceFlags faceMask,
   VkStencilOp failOp,
   {
      VN_CMD_ENQUEUE(vkCmdSetStencilOp, commandBuffer, faceMask, failOp, passOp,
      }
      void
   vn_CmdSetStencilTestEnable(VkCommandBuffer commandBuffer,
         {
      VN_CMD_ENQUEUE(vkCmdSetStencilTestEnable, commandBuffer,
      }
      void
   vn_CmdSetViewportWithCount(VkCommandBuffer commandBuffer,
               {
      VN_CMD_ENQUEUE(vkCmdSetViewportWithCount, commandBuffer, viewportCount,
      }
      void
   vn_CmdSetDepthBiasEnable(VkCommandBuffer commandBuffer,
         {
         }
      void
   vn_CmdSetLogicOpEXT(VkCommandBuffer commandBuffer, VkLogicOp logicOp)
   {
         }
      void
   vn_CmdSetColorWriteEnableEXT(VkCommandBuffer commandBuffer,
               {
      VN_CMD_ENQUEUE(vkCmdSetColorWriteEnableEXT, commandBuffer, attachmentCount,
      }
      void
   vn_CmdSetPatchControlPointsEXT(VkCommandBuffer commandBuffer,
         {
      VN_CMD_ENQUEUE(vkCmdSetPatchControlPointsEXT, commandBuffer,
      }
      void
   vn_CmdSetPrimitiveRestartEnable(VkCommandBuffer commandBuffer,
         {
      VN_CMD_ENQUEUE(vkCmdSetPrimitiveRestartEnable, commandBuffer,
      }
      void
   vn_CmdSetRasterizerDiscardEnable(VkCommandBuffer commandBuffer,
         {
      VN_CMD_ENQUEUE(vkCmdSetRasterizerDiscardEnable, commandBuffer,
      }
      void
   vn_CmdBeginConditionalRenderingEXT(
      VkCommandBuffer commandBuffer,
      {
      VN_CMD_ENQUEUE(vkCmdBeginConditionalRenderingEXT, commandBuffer,
      }
      void
   vn_CmdEndConditionalRenderingEXT(VkCommandBuffer commandBuffer)
   {
         }
      void
   vn_CmdDrawMultiEXT(VkCommandBuffer commandBuffer,
                     uint32_t drawCount,
   const VkMultiDrawInfoEXT *pVertexInfo,
   {
      VN_CMD_ENQUEUE(vkCmdDrawMultiEXT, commandBuffer, drawCount, pVertexInfo,
            vn_cmd_count_draw_and_submit_on_batch_limit(
      }
      void
   vn_CmdDrawMultiIndexedEXT(VkCommandBuffer commandBuffer,
                           uint32_t drawCount,
   const VkMultiDrawIndexedInfoEXT *pIndexInfo,
   {
      VN_CMD_ENQUEUE(vkCmdDrawMultiIndexedEXT, commandBuffer, drawCount,
                  vn_cmd_count_draw_and_submit_on_batch_limit(
      }
      void
   vn_CmdPushDescriptorSetKHR(VkCommandBuffer commandBuffer,
                                 {
      if (vn_should_sanitize_descriptor_set_writes(descriptorWriteCount,
            struct vn_command_buffer *cmd =
         struct vn_update_descriptor_sets *update =
      vn_update_descriptor_sets_parse_writes(
      descriptorWriteCount, pDescriptorWrites, &cmd->pool->allocator,
   if (!update) {
      cmd->state = VN_COMMAND_BUFFER_STATE_INVALID;
               VN_CMD_ENQUEUE(vkCmdPushDescriptorSetKHR, commandBuffer,
                     } else {
      VN_CMD_ENQUEUE(vkCmdPushDescriptorSetKHR, commandBuffer,
               }
      void
   vn_CmdPushDescriptorSetWithTemplateKHR(
      VkCommandBuffer commandBuffer,
   VkDescriptorUpdateTemplate descriptorUpdateTemplate,
   VkPipelineLayout layout,
   uint32_t set,
      {
      struct vn_descriptor_update_template *templ =
                     struct vn_update_descriptor_sets *update =
      vn_update_descriptor_set_with_template_locked(templ, VK_NULL_HANDLE,
      VN_CMD_ENQUEUE(vkCmdPushDescriptorSetKHR, commandBuffer,
                     }
      void
   vn_CmdSetVertexInputEXT(
      VkCommandBuffer commandBuffer,
   uint32_t vertexBindingDescriptionCount,
   const VkVertexInputBindingDescription2EXT *pVertexBindingDescriptions,
   uint32_t vertexAttributeDescriptionCount,
      {
      VN_CMD_ENQUEUE(vkCmdSetVertexInputEXT, commandBuffer,
                  }
