   /*
   * Copyright © 2023 Valve Corporation
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
   * 
   * Authors:
   *    Mike Blumenkrantz <michael.blumenkrantz@gmail.com>
   */
      #include "zink_batch.h"
   #include "zink_context.h"
   #include "zink_descriptors.h"
   #include "zink_resource.h"
   #include "zink_screen.h"
         static VkAccessFlags
   access_src_flags(VkImageLayout layout)
   {
      switch (layout) {
   case VK_IMAGE_LAYOUT_UNDEFINED:
            case VK_IMAGE_LAYOUT_GENERAL:
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
   case VK_IMAGE_LAYOUT_ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT:
         case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
   case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            case VK_IMAGE_LAYOUT_PREINITIALIZED:
            case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            default:
            }
      static VkAccessFlags
   access_dst_flags(VkImageLayout layout)
   {
      switch (layout) {
   case VK_IMAGE_LAYOUT_UNDEFINED:
            case VK_IMAGE_LAYOUT_GENERAL:
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
   case VK_IMAGE_LAYOUT_ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT:
         case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            default:
            }
      static VkPipelineStageFlags
   pipeline_dst_stage(VkImageLayout layout)
   {
      switch (layout) {
   case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
         case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
         case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            case VK_IMAGE_LAYOUT_GENERAL:
            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
   case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
            default:
            }
      #define ALL_READ_ACCESS_FLAGS \
      (VK_ACCESS_INDIRECT_COMMAND_READ_BIT | \
   VK_ACCESS_INDEX_READ_BIT | \
   VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | \
   VK_ACCESS_UNIFORM_READ_BIT | \
   VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | \
   VK_ACCESS_SHADER_READ_BIT | \
   VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | \
   VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | \
   VK_ACCESS_TRANSFER_READ_BIT |\
   VK_ACCESS_HOST_READ_BIT |\
   VK_ACCESS_MEMORY_READ_BIT |\
   VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT |\
   VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT |\
   VK_ACCESS_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT |\
   VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR |\
   VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR |\
   VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT |\
   VK_ACCESS_COMMAND_PREPROCESS_READ_BIT_NV |\
   VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR |\
            bool
   zink_resource_access_is_write(VkAccessFlags flags)
   {
         }
      bool
   zink_resource_image_needs_barrier(struct zink_resource *res, VkImageLayout new_layout, VkAccessFlags flags, VkPipelineStageFlags pipeline)
   {
      if (!pipeline)
         if (!flags)
         return res->layout != new_layout || (res->obj->access_stage & pipeline) != pipeline ||
         (res->obj->access & flags) != flags ||
      }
      void
   zink_resource_image_barrier_init(VkImageMemoryBarrier *imb, struct zink_resource *res, VkImageLayout new_layout, VkAccessFlags flags, VkPipelineStageFlags pipeline)
   {
      if (!pipeline)
         if (!flags)
            VkImageSubresourceRange isr = {
      res->aspect,
   0, VK_REMAINING_MIP_LEVELS,
      };
   *imb = VkImageMemoryBarrier {
      VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
   NULL,
   res->obj->access ? res->obj->access : access_src_flags(res->layout),
   flags,
   res->layout,
   new_layout,
   VK_QUEUE_FAMILY_IGNORED,
   VK_QUEUE_FAMILY_IGNORED,
   res->obj->image,
         }
      void
   zink_resource_image_barrier2_init(VkImageMemoryBarrier2 *imb, struct zink_resource *res, VkImageLayout new_layout, VkAccessFlags flags, VkPipelineStageFlags pipeline)
   {
      if (!pipeline)
         if (!flags)
            VkImageSubresourceRange isr = {
      res->aspect,
   0, VK_REMAINING_MIP_LEVELS,
      };
   *imb = VkImageMemoryBarrier2 {
      VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
   NULL,
   res->obj->access_stage ? res->obj->access_stage : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
   res->obj->access ? res->obj->access : access_src_flags(res->layout),
   pipeline,
   flags,
   res->layout,
   new_layout,
   VK_QUEUE_FAMILY_IGNORED,
   VK_QUEUE_FAMILY_IGNORED,
   res->obj->image,
         }
      static inline bool
   is_shader_pipline_stage(VkPipelineStageFlags pipeline)
   {
         }
      static void
   resource_check_defer_buffer_barrier(struct zink_context *ctx, struct zink_resource *res, VkPipelineStageFlags pipeline)
   {
      assert(res->obj->is_buffer);
   if (res->bind_count[0] - res->so_bind_count > 0) {
      if ((res->vbo_bind_mask && !(pipeline & VK_PIPELINE_STAGE_VERTEX_INPUT_BIT)) ||
      (util_bitcount(res->vbo_bind_mask) != res->bind_count[0] && !is_shader_pipline_stage(pipeline)))
   /* gfx rebind */
   }
   if (res->bind_count[1] && !(pipeline & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT))
      /* compute rebind */
   }
      static inline bool
   unordered_res_exec(const struct zink_context *ctx, const struct zink_resource *res, bool is_write)
   {
      /* if all usage is unordered, keep unordered */
   if (res->obj->unordered_read && res->obj->unordered_write)
         /* if testing write access but have any ordered read access, cannot promote */
   if (is_write && zink_batch_usage_matches(res->obj->bo->reads.u, ctx->batch.state) && !res->obj->unordered_read)
         /* if write access is unordered or nonexistent, always promote */
      }
      VkCommandBuffer
   zink_get_cmdbuf(struct zink_context *ctx, struct zink_resource *src, struct zink_resource *dst)
   {
      bool unordered_exec = (zink_debug & ZINK_DEBUG_NOREORDER) == 0;
   /* TODO: figure out how to link up unordered layout -> ordered layout and delete these two conditionals */
   if (src && !src->obj->is_buffer) {
      if (zink_resource_usage_is_unflushed(src) && !src->obj->unordered_read && !src->obj->unordered_write)
      }
   if (dst && !dst->obj->is_buffer) {
      if (zink_resource_usage_is_unflushed(dst) && !dst->obj->unordered_read && !dst->obj->unordered_write)
      }
   if (src && unordered_exec)
         if (dst && unordered_exec)
         if (src)
         if (dst)
         if (!unordered_exec || ctx->unordered_blitting)
         if (unordered_exec) {
      ctx->batch.state->has_barriers = true;
   ctx->batch.has_work = true;
      }
      }
      static void
   resource_check_defer_image_barrier(struct zink_context *ctx, struct zink_resource *res, VkImageLayout layout, VkPipelineStageFlags pipeline)
   {
      assert(!res->obj->is_buffer);
            bool is_compute = pipeline == VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
   /* if this is a non-shader barrier and there are binds, always queue a shader barrier */
   bool is_shader = is_shader_pipline_stage(pipeline);
   if ((is_shader || !res->bind_count[is_compute]) &&
      /* if no layout change is needed between gfx and compute, do nothing */
   !res->bind_count[!is_compute] && (!is_compute || !res->fb_bind_count))
         if (res->bind_count[!is_compute] && is_shader) {
      /* if the layout is the same between gfx and compute, do nothing */
   if (layout == zink_descriptor_util_image_layout_eval(ctx, res, !is_compute))
      }
   /* queue a layout change if a layout change will be needed */
   if (res->bind_count[!is_compute])
         /* also queue a layout change if this is a non-shader layout */
   if (res->bind_count[is_compute] && !is_shader)
      }
      template <bool HAS_SYNC2>
   void
   zink_resource_image_barrier(struct zink_context *ctx, struct zink_resource *res, VkImageLayout new_layout, VkAccessFlags flags, VkPipelineStageFlags pipeline)
   {
      if (!pipeline)
         if (!flags)
            if (!res->obj->needs_zs_evaluate && !zink_resource_image_needs_barrier(res, new_layout, flags, pipeline) &&
      (res->queue == zink_screen(ctx->base.screen)->gfx_queue || res->queue == VK_QUEUE_FAMILY_IGNORED))
      bool is_write = zink_resource_access_is_write(flags);
   enum zink_resource_access rw = is_write ? ZINK_RESOURCE_ACCESS_RW : ZINK_RESOURCE_ACCESS_WRITE;
   bool completed = zink_resource_usage_check_completion_fast(zink_screen(ctx->base.screen), res, rw);
   bool usage_matches = !completed && zink_resource_usage_matches(res, ctx->batch.state);
   VkCommandBuffer cmdbuf;
   if (!usage_matches) {
      res->obj->unordered_write = true;
   if (is_write || zink_resource_usage_check_completion_fast(zink_screen(ctx->base.screen), res, ZINK_RESOURCE_ACCESS_RW))
      }
   /* if current batch usage exists with ordered non-transfer access, never promote
   * this avoids layout dsync
   */
   if (zink_resource_usage_matches(res, ctx->batch.state) && !ctx->unordered_blitting &&
      (!res->obj->unordered_read || !res->obj->unordered_write)) {
   cmdbuf = ctx->batch.state->cmdbuf;
   res->obj->unordered_write = false;
   res->obj->unordered_read = false;
   /* it's impossible to detect this from the caller
   * there should be no valid case where this barrier can occur inside a renderpass
   */
      } else {
      cmdbuf = is_write ? zink_get_cmdbuf(ctx, NULL, res) : zink_get_cmdbuf(ctx, res, NULL);
   /* force subsequent barriers to be ordered to avoid layout desync */
   if (cmdbuf != ctx->batch.state->barrier_cmdbuf) {
      res->obj->unordered_write = false;
         }
   assert(new_layout);
   bool marker = zink_cmd_debug_marker_begin(ctx, cmdbuf, "image_barrier(%s->%s)", vk_ImageLayout_to_str(res->layout), vk_ImageLayout_to_str(new_layout));
   bool queue_import = false;
   if (HAS_SYNC2) {
      VkImageMemoryBarrier2 imb;
   zink_resource_image_barrier2_init(&imb, res, new_layout, flags, pipeline);
   if (!res->obj->access_stage || completed)
         if (res->obj->needs_zs_evaluate)
         res->obj->needs_zs_evaluate = false;
   if (res->queue != zink_screen(ctx->base.screen)->gfx_queue && res->queue != VK_QUEUE_FAMILY_IGNORED) {
      imb.srcQueueFamilyIndex = res->queue;
   imb.dstQueueFamilyIndex = zink_screen(ctx->base.screen)->gfx_queue;
   res->queue = VK_QUEUE_FAMILY_IGNORED;
      }
   VkDependencyInfo dep = {
      VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
   NULL,
   0,
   0,
   NULL,
   0,
   NULL,
   1,
      };
      } else {
      VkImageMemoryBarrier imb;
   zink_resource_image_barrier_init(&imb, res, new_layout, flags, pipeline);
   if (!res->obj->access_stage || completed)
         if (res->obj->needs_zs_evaluate)
         res->obj->needs_zs_evaluate = false;
   if (res->queue != zink_screen(ctx->base.screen)->gfx_queue && res->queue != VK_QUEUE_FAMILY_IGNORED) {
      imb.srcQueueFamilyIndex = res->queue;
   imb.dstQueueFamilyIndex = zink_screen(ctx->base.screen)->gfx_queue;
   res->queue = VK_QUEUE_FAMILY_IGNORED;
      }
   VKCTX(CmdPipelineBarrier)(
      cmdbuf,
   res->obj->access_stage ? res->obj->access_stage : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
   pipeline,
   0,
   0, NULL,
   0, NULL,
         }
                     if (is_write)
            res->obj->access = flags;
   res->obj->access_stage = pipeline;
   res->layout = new_layout;
   if (res->obj->dt) {
      struct kopper_displaytarget *cdt = res->obj->dt;
   if (cdt->swapchain->num_acquires && res->obj->dt_idx != UINT32_MAX) {
            } else if (res->obj->exportable) {
      struct pipe_resource *pres = NULL;
   bool found = false;
   _mesa_set_search_or_add(&ctx->batch.state->dmabuf_exports, res, &found);
   if (!found) {
            }
   if (new_layout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
         if (res->obj->exportable && queue_import) {
      for (; res; res = zink_resource(res->base.b.next)) {
      VkSemaphore sem = zink_screen_export_dmabuf_semaphore(zink_screen(ctx->base.screen), res);
   if (sem)
            }
      bool
   zink_check_unordered_transfer_access(struct zink_resource *res, unsigned level, const struct pipe_box *box)
   {
      /* always barrier against previous non-transfer writes */
   bool non_transfer_write = res->obj->last_write && res->obj->last_write != VK_ACCESS_TRANSFER_WRITE_BIT;
   /* must barrier if clobbering a previous write */
   bool transfer_clobber = res->obj->last_write == VK_ACCESS_TRANSFER_WRITE_BIT && zink_resource_copy_box_intersects(res, level, box);
      }
      bool
   zink_check_valid_buffer_src_access(struct zink_context *ctx, struct zink_resource *res, unsigned offset, unsigned size)
   {
         }
      void
   zink_resource_image_transfer_dst_barrier(struct zink_context *ctx, struct zink_resource *res, unsigned level, const struct pipe_box *box)
   {
      if (res->obj->copies_need_reset)
         /* skip TRANSFER_DST barrier if no intersection from previous copies */
   if (res->layout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL ||
      zink_screen(ctx->base.screen)->driver_workarounds.broken_cache_semantics ||
   zink_check_unordered_transfer_access(res, level, box)) {
      } else {
      res->obj->access = VK_ACCESS_TRANSFER_WRITE_BIT;
   res->obj->last_write = VK_ACCESS_TRANSFER_WRITE_BIT;
      }
      }
      bool
   zink_resource_buffer_transfer_dst_barrier(struct zink_context *ctx, struct zink_resource *res, unsigned offset, unsigned size)
   {
      if (res->obj->copies_need_reset)
         bool unordered = true;
   struct pipe_box box = {(int)offset, 0, 0, (int)size, 0, 0};
   bool can_unordered_write = unordered_res_exec(ctx, res, true);
   /* must barrier if something read the valid buffer range */
   bool valid_read = (res->obj->access || res->obj->unordered_access) &&
         if (valid_read || zink_screen(ctx->base.screen)->driver_workarounds.broken_cache_semantics || zink_check_unordered_transfer_access(res, 0, &box)) {
      zink_screen(ctx->base.screen)->buffer_barrier(ctx, res, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
      } else {
      res->obj->unordered_access = VK_ACCESS_TRANSFER_WRITE_BIT;
   res->obj->last_write = VK_ACCESS_TRANSFER_WRITE_BIT;
            ctx->batch.state->unordered_write_access |= VK_ACCESS_TRANSFER_WRITE_BIT;
   ctx->batch.state->unordered_write_stages |= VK_PIPELINE_STAGE_TRANSFER_BIT;
   if (!zink_resource_usage_matches(res, ctx->batch.state)) {
      res->obj->access = VK_ACCESS_TRANSFER_WRITE_BIT;
   res->obj->access_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
         }
   zink_resource_copy_box_add(ctx, res, 0, &box);
   /* this return value implies that the caller could do an unordered op on this resource */
      }
      VkPipelineStageFlags
   zink_pipeline_flags_from_stage(VkShaderStageFlagBits stage)
   {
      switch (stage) {
   case VK_SHADER_STAGE_VERTEX_BIT:
         case VK_SHADER_STAGE_FRAGMENT_BIT:
         case VK_SHADER_STAGE_GEOMETRY_BIT:
         case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
         case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
         case VK_SHADER_STAGE_COMPUTE_BIT:
         default:
            }
      ALWAYS_INLINE static VkPipelineStageFlags
   pipeline_access_stage(VkAccessFlags flags)
   {
      if (flags & (VK_ACCESS_UNIFORM_READ_BIT |
                  return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
         VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT |
   VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT |
   VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT |
         }
      ALWAYS_INLINE static bool
   buffer_needs_barrier(struct zink_resource *res, VkAccessFlags flags, VkPipelineStageFlags pipeline, bool unordered)
   {
      return zink_resource_access_is_write(unordered ? res->obj->unordered_access : res->obj->access) ||
         zink_resource_access_is_write(flags) ||
      }
      template <bool HAS_SYNC2>
   void
   zink_resource_buffer_barrier(struct zink_context *ctx, struct zink_resource *res, VkAccessFlags flags, VkPipelineStageFlags pipeline)
   {
      if (!pipeline)
            bool is_write = zink_resource_access_is_write(flags);
   enum zink_resource_access rw = is_write ? ZINK_RESOURCE_ACCESS_RW : ZINK_RESOURCE_ACCESS_WRITE;
   bool completed = zink_resource_usage_check_completion_fast(zink_screen(ctx->base.screen), res, rw);
   bool usage_matches = !completed && zink_resource_usage_matches(res, ctx->batch.state);
   if (!usage_matches) {
      res->obj->unordered_write = true;
   if (is_write || zink_resource_usage_check_completion_fast(zink_screen(ctx->base.screen), res, ZINK_RESOURCE_ACCESS_RW))
      }
   bool unordered_usage_matches = res->obj->unordered_access && usage_matches;
   bool unordered = unordered_res_exec(ctx, res, is_write);
   if (!buffer_needs_barrier(res, flags, pipeline, unordered))
         if (completed) {
      /* reset access on complete */
   res->obj->access = VK_ACCESS_NONE;
   res->obj->access_stage = VK_PIPELINE_STAGE_NONE;
      } else if (unordered && unordered_usage_matches && res->obj->ordered_access_is_copied) {
      /* always reset propagated access to avoid weirdness */
   res->obj->access = VK_ACCESS_NONE;
      } else if (!unordered && !unordered_usage_matches) {
      /* reset unordered access on first ordered barrier */
   res->obj->unordered_access = VK_ACCESS_NONE;
      }
   if (!usage_matches) {
      /* reset unordered on first new cmdbuf barrier */
   res->obj->unordered_access = VK_ACCESS_NONE;
   res->obj->unordered_access_stage = VK_PIPELINE_STAGE_NONE;
      }
   /* unordered barriers can be skipped when:
   * - there is no current-batch unordered access AND previous batch usage is not write access
   * - there is current-batch unordered access AND the unordered access is not write access
   */
   bool can_skip_unordered = !unordered ? false : !zink_resource_access_is_write(!unordered_usage_matches ? res->obj->access : res->obj->unordered_access);
   /* ordered barriers can be skipped if both:
   * - there is no current access
   * - there is no current-batch unordered access
   */
   bool can_skip_ordered = unordered ? false : (!res->obj->access && !unordered_usage_matches);
   if (zink_debug & ZINK_DEBUG_NOREORDER)
            if (!can_skip_unordered && !can_skip_ordered) {
      VkCommandBuffer cmdbuf = is_write ? zink_get_cmdbuf(ctx, NULL, res) : zink_get_cmdbuf(ctx, res, NULL);
   bool marker = false;
   if (unlikely(zink_tracing)) {
      char buf[4096];
   zink_string_vkflags_unroll(buf, sizeof(buf), flags, (zink_vkflags_func)vk_AccessFlagBits_to_str);
               VkPipelineStageFlags stages = res->obj->access_stage ? res->obj->access_stage : pipeline_access_stage(res->obj->access);;
   if (HAS_SYNC2) {
      VkMemoryBarrier2 bmb;
   bmb.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
   bmb.pNext = NULL;
   if (unordered) {
      bmb.srcStageMask = usage_matches ? res->obj->unordered_access_stage : stages;
      } else {
      bmb.srcStageMask = stages;
      }
   bmb.dstStageMask = pipeline;
   bmb.dstAccessMask = flags;
   VkDependencyInfo dep = {
      VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
   NULL,
   0,
   1,
   &bmb,
   0,
   NULL,
   0,
      };
      } else {
      VkMemoryBarrier bmb;
   bmb.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
   bmb.pNext = NULL;
   if (unordered) {
      stages = usage_matches ? res->obj->unordered_access_stage : stages;
      } else {
         }
   VKCTX(CmdPipelineBarrier)(
      cmdbuf,
   stages,
   pipeline,
   0,
   1, &bmb,
   0, NULL,
                                       if (is_write)
         if (unordered) {
      /* these should get automatically emitted during submission */
   res->obj->unordered_access = flags;
   res->obj->unordered_access_stage = pipeline;
   if (is_write) {
      ctx->batch.state->unordered_write_access |= flags;
         }
   if (!unordered || !usage_matches || res->obj->ordered_access_is_copied) {
      res->obj->access = flags;
   res->obj->access_stage = pipeline;
      }
   if (pipeline != VK_PIPELINE_STAGE_TRANSFER_BIT && is_write)
      }
      void
   zink_synchronization_init(struct zink_screen *screen)
   {
      if (screen->info.have_vulkan13 || screen->info.have_KHR_synchronization2) {
      screen->buffer_barrier = zink_resource_buffer_barrier<true>;
      } else {
      screen->buffer_barrier = zink_resource_buffer_barrier<false>;
         }
