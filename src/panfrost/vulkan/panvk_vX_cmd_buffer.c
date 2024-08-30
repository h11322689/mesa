   /*
   * Copyright © 2021 Collabora Ltd.
   *
   * Derived from tu_cmd_buffer.c which is:
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
      #include "genxml/gen_macros.h"
      #include "panvk_cs.h"
   #include "panvk_private.h"
      #include "pan_blitter.h"
   #include "pan_cs.h"
   #include "pan_encoder.h"
      #include "util/rounding.h"
   #include "util/u_pack_color.h"
   #include "vk_format.h"
      static uint32_t
   panvk_debug_adjust_bo_flags(const struct panvk_device *device,
         {
               if (debug_flags & PANVK_DEBUG_DUMP)
               }
      static void
   panvk_cmd_prepare_fragment_job(struct panvk_cmd_buffer *cmdbuf)
   {
      const struct pan_fb_info *fbinfo = &cmdbuf->state.fb.info;
   struct panvk_batch *batch = cmdbuf->state.batch;
   struct panfrost_ptr job_ptr =
            GENX(pan_emit_fragment_job)
   (fbinfo, batch->fb.desc.gpu, job_ptr.cpu), batch->fragment_job = job_ptr.gpu;
      }
      void
   panvk_per_arch(cmd_close_batch)(struct panvk_cmd_buffer *cmdbuf)
   {
               if (!batch)
                              bool clear = fbinfo->zs.clear.z | fbinfo->zs.clear.s;
   for (unsigned i = 0; i < fbinfo->rt_count; i++)
            if (!clear && !batch->scoreboard.first_job) {
      if (util_dynarray_num_elements(&batch->event_ops,
            /* Content-less batch, let's drop it */
      } else {
      /* Batch has no jobs but is needed for synchronization, let's add a
   * NULL job so the SUBMIT ioctl doesn't choke on it.
   */
   struct panfrost_ptr ptr =
         util_dynarray_append(&batch->jobs, void *, ptr.cpu);
   panfrost_add_job(&cmdbuf->desc_pool.base, &batch->scoreboard,
            }
   cmdbuf->state.batch = NULL;
                                 if (batch->scoreboard.first_tiler) {
      struct panfrost_ptr preload_jobs[2];
   unsigned num_preload_jobs = GENX(pan_preload_fb)(
      &cmdbuf->desc_pool.base, &batch->scoreboard, &cmdbuf->state.fb.info,
      for (unsigned i = 0; i < num_preload_jobs; i++)
               if (batch->tlsinfo.tls.size) {
      unsigned size = panfrost_get_total_stack_size(
         batch->tlsinfo.tls.ptr =
               if (batch->tlsinfo.wls.size) {
      assert(batch->wls_total_size);
   batch->tlsinfo.wls.ptr =
      pan_pool_alloc_aligned(&cmdbuf->tls_pool.base, batch->wls_total_size,
                  if (batch->tls.cpu)
            if (batch->fb.desc.cpu) {
      batch->fb.desc.gpu |=
                                 }
      void
   panvk_per_arch(CmdNextSubpass2)(VkCommandBuffer commandBuffer,
               {
                        cmdbuf->state.subpass++;
   panvk_cmd_fb_info_set_subpass(cmdbuf);
      }
      void
   panvk_per_arch(CmdNextSubpass)(VkCommandBuffer cmd, VkSubpassContents contents)
   {
      VkSubpassBeginInfo binfo = {.sType = VK_STRUCTURE_TYPE_SUBPASS_BEGIN_INFO,
         VkSubpassEndInfo einfo = {
                     }
      void
   panvk_per_arch(cmd_alloc_fb_desc)(struct panvk_cmd_buffer *cmdbuf)
   {
               if (batch->fb.desc.gpu)
            const struct pan_fb_info *fbinfo = &cmdbuf->state.fb.info;
            batch->fb.info = cmdbuf->state.framebuffer;
   batch->fb.desc = pan_pool_alloc_desc_aggregate(
      &cmdbuf->desc_pool.base, PAN_DESC(FRAMEBUFFER),
   PAN_DESC_ARRAY(has_zs_ext ? 1 : 0, ZS_CRC_EXTENSION),
         memset(&cmdbuf->state.fb.info.bifrost.pre_post.dcds, 0,
      }
      void
   panvk_per_arch(cmd_alloc_tls_desc)(struct panvk_cmd_buffer *cmdbuf, bool gfx)
   {
               assert(batch);
   if (!batch->tls.gpu) {
            }
      static void
   panvk_cmd_prepare_draw_sysvals(
      struct panvk_cmd_buffer *cmdbuf,
   struct panvk_cmd_bind_point_state *bind_point_state,
      {
               unsigned base_vertex = draw->index_size ? draw->vertex_offset : 0;
   if (sysvals->first_vertex != draw->offset_start ||
      sysvals->base_vertex != base_vertex ||
   sysvals->base_instance != draw->first_instance) {
   sysvals->first_vertex = draw->offset_start;
   sysvals->base_vertex = base_vertex;
   sysvals->base_instance = draw->first_instance;
               if (cmdbuf->state.dirty & PANVK_DYNAMIC_BLEND_CONSTANTS) {
      memcpy(&sysvals->blend_constants, cmdbuf->state.blend.constants,
                     if (cmdbuf->state.dirty & PANVK_DYNAMIC_VIEWPORT) {
      panvk_sysval_upload_viewport_scale(&cmdbuf->state.viewport,
         panvk_sysval_upload_viewport_offset(&cmdbuf->state.viewport,
               }
      static void
   panvk_cmd_prepare_sysvals(struct panvk_cmd_buffer *cmdbuf,
         {
               if (desc_state->sysvals_ptr)
            struct panfrost_ptr sysvals = pan_pool_alloc_aligned(
         memcpy(sysvals.cpu, &desc_state->sysvals, sizeof(desc_state->sysvals));
      }
      static void
   panvk_cmd_prepare_push_constants(
      struct panvk_cmd_buffer *cmdbuf,
      {
      struct panvk_descriptor_state *desc_state = &bind_point_state->desc_state;
            if (!pipeline->layout->push_constants.size || desc_state->push_constants)
            struct panfrost_ptr push_constants = pan_pool_alloc_aligned(
      &cmdbuf->desc_pool.base,
         memcpy(push_constants.cpu, cmdbuf->push_constants,
            }
      static void
   panvk_cmd_prepare_ubos(struct panvk_cmd_buffer *cmdbuf,
         {
      struct panvk_descriptor_state *desc_state = &bind_point_state->desc_state;
            if (!pipeline->num_ubos || desc_state->ubos)
            panvk_cmd_prepare_sysvals(cmdbuf, bind_point_state);
            struct panfrost_ptr ubos = pan_pool_alloc_desc_array(
                        }
      static void
   panvk_cmd_prepare_textures(struct panvk_cmd_buffer *cmdbuf,
         {
      struct panvk_descriptor_state *desc_state = &bind_point_state->desc_state;
   const struct panvk_pipeline *pipeline = bind_point_state->pipeline;
            if (!num_textures || desc_state->textures)
            struct panfrost_ptr textures = pan_pool_alloc_aligned(
      &cmdbuf->desc_pool.base, num_textures * pan_size(TEXTURE),
                  for (unsigned i = 0; i < ARRAY_SIZE(desc_state->sets); i++) {
      if (!desc_state->sets[i])
            memcpy(texture, desc_state->sets[i]->textures,
                           }
      static void
   panvk_cmd_prepare_samplers(struct panvk_cmd_buffer *cmdbuf,
         {
      struct panvk_descriptor_state *desc_state = &bind_point_state->desc_state;
   const struct panvk_pipeline *pipeline = bind_point_state->pipeline;
            if (!num_samplers || desc_state->samplers)
            struct panfrost_ptr samplers =
                     /* Prepare the dummy sampler */
   pan_pack(sampler, SAMPLER, cfg) {
      cfg.seamless_cube_map = false;
   cfg.magnify_nearest = true;
   cfg.minify_nearest = true;
                        for (unsigned i = 0; i < ARRAY_SIZE(desc_state->sets); i++) {
      if (!desc_state->sets[i])
            memcpy(sampler, desc_state->sets[i]->samplers,
                           }
      static void
   panvk_draw_prepare_fs_rsd(struct panvk_cmd_buffer *cmdbuf,
         {
      const struct panvk_pipeline *pipeline =
            if (!pipeline->fs.dynamic_rsd) {
      draw->fs_rsd = pipeline->rsds[MESA_SHADER_FRAGMENT];
               if (!cmdbuf->state.fs_rsd) {
      struct panfrost_ptr rsd = pan_pool_alloc_desc_aggregate(
                  struct mali_renderer_state_packed rsd_dyn;
   struct mali_renderer_state_packed *rsd_templ =
                     panvk_per_arch(emit_dyn_fs_rsd)(pipeline, &cmdbuf->state, &rsd_dyn);
   pan_merge(rsd_dyn, (*rsd_templ), RENDERER_STATE);
            void *bd = rsd.cpu + pan_size(RENDERER_STATE);
   for (unsigned i = 0; i < pipeline->blend.state.rt_count; i++) {
      if (pipeline->blend.constant[i].index != (uint8_t)~0) {
      struct mali_blend_packed bd_dyn;
                  STATIC_ASSERT(sizeof(pipeline->blend.bd_template[0]) >=
         panvk_per_arch(emit_blend_constant)(cmdbuf->device, pipeline, i,
               pan_merge(bd_dyn, (*bd_templ), BLEND);
      }
                              }
      void
   panvk_per_arch(cmd_get_tiler_context)(struct panvk_cmd_buffer *cmdbuf,
         {
               if (batch->tiler.descs.cpu)
            batch->tiler.descs = pan_pool_alloc_desc_aggregate(
         STATIC_ASSERT(sizeof(batch->tiler.templ) >=
            struct panfrost_ptr desc = {
      .gpu = batch->tiler.descs.gpu,
               panvk_per_arch(emit_tiler_context)(cmdbuf->device, width, height, &desc);
   memcpy(batch->tiler.descs.cpu, batch->tiler.templ,
            }
      void
   panvk_per_arch(cmd_prepare_tiler_context)(struct panvk_cmd_buffer *cmdbuf)
   {
                  }
      static void
   panvk_draw_prepare_tiler_context(struct panvk_cmd_buffer *cmdbuf,
         {
               panvk_per_arch(cmd_prepare_tiler_context)(cmdbuf);
      }
      static void
   panvk_draw_prepare_varyings(struct panvk_cmd_buffer *cmdbuf,
         {
      const struct panvk_pipeline *pipeline =
                  panvk_varyings_alloc(varyings, &cmdbuf->varying_pool.base,
            unsigned buf_count = panvk_varyings_buf_count(varyings);
   struct panfrost_ptr bufs = pan_pool_alloc_desc_array(
                     /* We need an empty entry to stop prefetching on Bifrost */
   memset(bufs.cpu + (pan_size(ATTRIBUTE_BUFFER) * buf_count), 0,
            if (BITSET_TEST(varyings->active, VARYING_SLOT_POS)) {
      draw->position =
      varyings->buf[varyings->varying[VARYING_SLOT_POS].buf].address +
            if (pipeline->ia.writes_point_size) {
      draw->psiz =
      varyings->buf[varyings->varying[VARYING_SLOT_PSIZ].buf].address +
   } else if (pipeline->ia.topology == MALI_DRAW_MODE_LINES ||
            pipeline->ia.topology == MALI_DRAW_MODE_LINE_STRIP ||
   draw->line_width = pipeline->dynamic_state_mask & PANVK_DYNAMIC_LINE_WIDTH
            } else {
         }
            for (unsigned s = 0; s < MESA_SHADER_STAGES; s++) {
      if (!varyings->stage[s].count)
            struct panfrost_ptr attribs = pan_pool_alloc_desc_array(
            panvk_per_arch(emit_varyings)(cmdbuf->device, varyings, s, attribs.cpu);
         }
      static void
   panvk_fill_non_vs_attribs(struct panvk_cmd_buffer *cmdbuf,
               {
      struct panvk_descriptor_state *desc_state = &bind_point_state->desc_state;
            for (unsigned s = 0; s < pipeline->layout->vk.set_count; s++) {
               if (!set)
            const struct panvk_descriptor_set_layout *layout = set->layout;
   unsigned img_idx = pipeline->layout->sets[s].img_offset;
   unsigned offset = img_idx * pan_size(ATTRIBUTE_BUFFER) * 2;
                     offset = img_idx * pan_size(ATTRIBUTE);
   for (unsigned i = 0; i < layout->num_imgs; i++) {
      pan_pack(attribs + offset, ATTRIBUTE, cfg) {
      cfg.buffer_index = first_buf + (img_idx + i) * 2;
      }
            }
      static void
   panvk_prepare_non_vs_attribs(struct panvk_cmd_buffer *cmdbuf,
         {
      struct panvk_descriptor_state *desc_state = &bind_point_state->desc_state;
            if (desc_state->non_vs_attribs || !pipeline->img_access_mask)
            unsigned attrib_count = pipeline->layout->num_imgs;
   unsigned attrib_buf_count = (pipeline->layout->num_imgs * 2);
   struct panfrost_ptr bufs = pan_pool_alloc_desc_array(
         struct panfrost_ptr attribs = pan_pool_alloc_desc_array(
            panvk_fill_non_vs_attribs(cmdbuf, bind_point_state, bufs.cpu, attribs.cpu,
            desc_state->non_vs_attrib_bufs = bufs.gpu;
      }
      static void
   panvk_draw_prepare_vs_attribs(struct panvk_cmd_buffer *cmdbuf,
         {
      struct panvk_cmd_bind_point_state *bind_point_state =
         struct panvk_descriptor_state *desc_state = &bind_point_state->desc_state;
   const struct panvk_pipeline *pipeline = bind_point_state->pipeline;
   unsigned num_imgs =
      pipeline->img_access_mask & BITFIELD_BIT(MESA_SHADER_VERTEX)
      ? pipeline->layout->num_imgs
            if (desc_state->vs_attribs || !attrib_count)
            if (!pipeline->attribs.buf_count) {
      panvk_prepare_non_vs_attribs(cmdbuf, bind_point_state);
   desc_state->vs_attrib_bufs = desc_state->non_vs_attrib_bufs;
   desc_state->vs_attribs = desc_state->non_vs_attribs;
               unsigned attrib_buf_count = pipeline->attribs.buf_count * 2;
   struct panfrost_ptr bufs = pan_pool_alloc_desc_array(
         struct panfrost_ptr attribs = pan_pool_alloc_desc_array(
            panvk_per_arch(emit_attrib_bufs)(&pipeline->attribs, cmdbuf->state.vb.bufs,
         panvk_per_arch(emit_attribs)(cmdbuf->device, draw, &pipeline->attribs,
                  if (attrib_count > pipeline->attribs.buf_count) {
      unsigned bufs_offset =
         unsigned attribs_offset =
            panvk_fill_non_vs_attribs(
      cmdbuf, bind_point_state, bufs.cpu + bufs_offset,
            /* A NULL entry is needed to stop prefecting on Bifrost */
   memset(bufs.cpu + (pan_size(ATTRIBUTE_BUFFER) * attrib_buf_count), 0,
            desc_state->vs_attrib_bufs = bufs.gpu;
      }
      static void
   panvk_draw_prepare_attributes(struct panvk_cmd_buffer *cmdbuf,
         {
      struct panvk_cmd_bind_point_state *bind_point_state =
         struct panvk_descriptor_state *desc_state = &bind_point_state->desc_state;
            for (unsigned i = 0; i < ARRAY_SIZE(draw->stages); i++) {
      if (i == MESA_SHADER_VERTEX) {
      panvk_draw_prepare_vs_attribs(cmdbuf, draw);
   draw->stages[i].attributes = desc_state->vs_attribs;
      } else if (pipeline->img_access_mask & BITFIELD_BIT(i)) {
      panvk_prepare_non_vs_attribs(cmdbuf, bind_point_state);
   draw->stages[i].attributes = desc_state->non_vs_attribs;
            }
      static void
   panvk_draw_prepare_viewport(struct panvk_cmd_buffer *cmdbuf,
         {
      const struct panvk_pipeline *pipeline =
            if (pipeline->vpd) {
         } else if (cmdbuf->state.vpd) {
         } else {
      struct panfrost_ptr vp =
            const VkViewport *viewport =
      pipeline->dynamic_state_mask & PANVK_DYNAMIC_VIEWPORT
      ? &cmdbuf->state.viewport
   const VkRect2D *scissor =
      pipeline->dynamic_state_mask & PANVK_DYNAMIC_SCISSOR
               panvk_per_arch(emit_viewport)(viewport, scissor, vp.cpu);
         }
      static void
   panvk_draw_prepare_vertex_job(struct panvk_cmd_buffer *cmdbuf,
         {
      const struct panvk_pipeline *pipeline =
         struct panvk_batch *batch = cmdbuf->state.batch;
   struct panfrost_ptr ptr =
            util_dynarray_append(&batch->jobs, void *, ptr.cpu);
   draw->jobs.vertex = ptr;
      }
      static void
   panvk_draw_prepare_tiler_job(struct panvk_cmd_buffer *cmdbuf,
         {
      const struct panvk_pipeline *pipeline =
         struct panvk_batch *batch = cmdbuf->state.batch;
   struct panfrost_ptr ptr =
            util_dynarray_append(&batch->jobs, void *, ptr.cpu);
   draw->jobs.tiler = ptr;
      }
      static void
   panvk_cmd_draw(struct panvk_cmd_buffer *cmdbuf, struct panvk_draw_info *draw)
   {
      struct panvk_batch *batch = cmdbuf->state.batch;
   struct panvk_cmd_bind_point_state *bind_point_state =
         const struct panvk_pipeline *pipeline =
            /* There are only 16 bits in the descriptor for the job ID, make sure all
   * the 3 (2 in Bifrost) jobs in this draw are in the same batch.
   */
   if (batch->scoreboard.job_index >= (UINT16_MAX - 3)) {
      panvk_per_arch(cmd_close_batch)(cmdbuf);
   panvk_cmd_preload_fb_after_batch_split(cmdbuf);
               if (pipeline->rast.enable)
                     panvk_cmd_prepare_draw_sysvals(cmdbuf, bind_point_state, draw);
   panvk_cmd_prepare_ubos(cmdbuf, bind_point_state);
   panvk_cmd_prepare_textures(cmdbuf, bind_point_state);
            /* TODO: indexed draws */
   struct panvk_descriptor_state *desc_state =
            draw->tls = batch->tls.gpu;
   draw->fb = batch->fb.desc.gpu;
   draw->ubos = desc_state->ubos;
   draw->textures = desc_state->textures;
            STATIC_ASSERT(sizeof(draw->invocation) >=
         panfrost_pack_work_groups_compute(
      (struct mali_invocation_packed *)&draw->invocation, 1, draw->vertex_range,
         panvk_draw_prepare_fs_rsd(cmdbuf, draw);
   panvk_draw_prepare_varyings(cmdbuf, draw);
   panvk_draw_prepare_attributes(cmdbuf, draw);
   panvk_draw_prepare_viewport(cmdbuf, draw);
   panvk_draw_prepare_tiler_context(cmdbuf, draw);
   panvk_draw_prepare_vertex_job(cmdbuf, draw);
   panvk_draw_prepare_tiler_job(cmdbuf, draw);
   batch->tlsinfo.tls.size = MAX2(pipeline->tls_size, batch->tlsinfo.tls.size);
            unsigned vjob_id = panfrost_add_job(
      &cmdbuf->desc_pool.base, &batch->scoreboard, MALI_JOB_TYPE_VERTEX, false,
         if (pipeline->rast.enable) {
      panfrost_add_job(&cmdbuf->desc_pool.base, &batch->scoreboard,
                     /* Clear the dirty flags all at once */
      }
      void
   panvk_per_arch(CmdDraw)(VkCommandBuffer commandBuffer, uint32_t vertexCount,
               {
               if (instanceCount == 0 || vertexCount == 0)
            struct panvk_draw_info draw = {
      .first_vertex = firstVertex,
   .vertex_count = vertexCount,
   .vertex_range = vertexCount,
   .first_instance = firstInstance,
   .instance_count = instanceCount,
   .padded_vertex_count = instanceCount > 1
                              }
      static void
   panvk_index_minmax_search(struct panvk_cmd_buffer *cmdbuf, uint32_t start,
               {
      void *ptr = cmdbuf->state.ib.buffer->bo->ptr.cpu +
            fprintf(
      stderr,
         assert(cmdbuf->state.ib.buffer);
   assert(cmdbuf->state.ib.buffer->bo);
                     /* TODO: Use panfrost_minmax_cache */
   /* TODO: Read full cacheline of data to mitigate the uncached
   * mapping slowness.
   */
      #define MINMAX_SEARCH_CASE(sz)                                                 \
      case sz: {                                                                  \
      uint##sz##_t *indices = ptr;                                             \
   *min = UINT##sz##_MAX;                                                   \
   for (uint32_t i = 0; i < count; i++) {                                   \
      if (restart && indices[i + start] == UINT##sz##_MAX)                  \
         *min = MIN2(indices[i + start], *min);                                \
      }                                                                        \
      }
      MINMAX_SEARCH_CASE(32)
   MINMAX_SEARCH_CASE(16)
   #undef MINMAX_SEARCH_CASE
      default:
            }
      void
   panvk_per_arch(CmdDrawIndexed)(VkCommandBuffer commandBuffer,
                     {
      VK_FROM_HANDLE(panvk_cmd_buffer, cmdbuf, commandBuffer);
            if (instanceCount == 0 || indexCount == 0)
            const struct panvk_pipeline *pipeline =
                  panvk_index_minmax_search(cmdbuf, firstIndex, indexCount, primitive_restart,
            unsigned vertex_range = max_vertex - min_vertex + 1;
   struct panvk_draw_info draw = {
      .index_size = cmdbuf->state.ib.index_size,
   .first_index = firstIndex,
   .index_count = indexCount,
   .vertex_offset = vertexOffset,
   .first_instance = firstInstance,
   .instance_count = instanceCount,
   .vertex_range = vertex_range,
   .vertex_count = indexCount + abs(vertexOffset),
   .padded_vertex_count = instanceCount > 1
               .offset_start = min_vertex + vertexOffset,
   .indices = panvk_buffer_gpu_ptr(cmdbuf->state.ib.buffer,
                        }
      VkResult
   panvk_per_arch(EndCommandBuffer)(VkCommandBuffer commandBuffer)
   {
                           }
      void
   panvk_per_arch(CmdEndRenderPass2)(VkCommandBuffer commandBuffer,
         {
               panvk_per_arch(cmd_close_batch)(cmdbuf);
   vk_free(&cmdbuf->vk.pool->alloc, cmdbuf->state.clear);
   cmdbuf->state.batch = NULL;
   cmdbuf->state.pass = NULL;
   cmdbuf->state.subpass = NULL;
   cmdbuf->state.framebuffer = NULL;
      }
      void
   panvk_per_arch(CmdEndRenderPass)(VkCommandBuffer cmd)
   {
      VkSubpassEndInfo einfo = {
                     }
      void
   panvk_per_arch(CmdPipelineBarrier2)(VkCommandBuffer commandBuffer,
         {
               /* Caches are flushed/invalidated at batch boundaries for now, nothing to do
   * for memory barriers assuming we implement barriers with the creation of a
   * new batch.
   * FIXME: We can probably do better with a CacheFlush job that has the
   * barrier flag set to true.
   */
   if (cmdbuf->state.batch) {
      panvk_per_arch(cmd_close_batch)(cmdbuf);
   panvk_cmd_preload_fb_after_batch_split(cmdbuf);
         }
      static void
   panvk_add_set_event_operation(struct panvk_cmd_buffer *cmdbuf,
               {
      struct panvk_event_op op = {
      .type = type,
               if (cmdbuf->state.batch == NULL) {
      /* No open batch, let's create a new one so this operation happens in
   * the right order.
   */
   panvk_cmd_open_batch(cmdbuf);
   util_dynarray_append(&cmdbuf->state.batch->event_ops,
            } else {
      /* Let's close the current batch so the operation executes before any
   * future commands.
   */
   util_dynarray_append(&cmdbuf->state.batch->event_ops,
         panvk_per_arch(cmd_close_batch)(cmdbuf);
   panvk_cmd_preload_fb_after_batch_split(cmdbuf);
         }
      static void
   panvk_add_wait_event_operation(struct panvk_cmd_buffer *cmdbuf,
         {
      struct panvk_event_op op = {
      .type = PANVK_EVENT_OP_WAIT,
               if (cmdbuf->state.batch == NULL) {
      /* No open batch, let's create a new one and have it wait for this event. */
   panvk_cmd_open_batch(cmdbuf);
   util_dynarray_append(&cmdbuf->state.batch->event_ops,
      } else {
      /* Let's close the current batch so any future commands wait on the
   * event signal operation.
   */
   if (cmdbuf->state.batch->fragment_job ||
      cmdbuf->state.batch->scoreboard.first_job) {
   panvk_per_arch(cmd_close_batch)(cmdbuf);
   panvk_cmd_preload_fb_after_batch_split(cmdbuf);
      }
   util_dynarray_append(&cmdbuf->state.batch->event_ops,
         }
      void
   panvk_per_arch(CmdSetEvent2)(VkCommandBuffer commandBuffer, VkEvent _event,
         {
      VK_FROM_HANDLE(panvk_cmd_buffer, cmdbuf, commandBuffer);
            /* vkCmdSetEvent cannot be called inside a render pass */
               }
      void
   panvk_per_arch(CmdResetEvent2)(VkCommandBuffer commandBuffer, VkEvent _event,
         {
      VK_FROM_HANDLE(panvk_cmd_buffer, cmdbuf, commandBuffer);
            /* vkCmdResetEvent cannot be called inside a render pass */
               }
      void
   panvk_per_arch(CmdWaitEvents2)(VkCommandBuffer commandBuffer,
               {
                        for (uint32_t i = 0; i < eventCount; i++) {
      VK_FROM_HANDLE(panvk_event, event, pEvents[i]);
         }
      static void
   panvk_reset_cmdbuf(struct vk_command_buffer *vk_cmdbuf,
         {
      struct panvk_cmd_buffer *cmdbuf =
                     list_for_each_entry_safe(struct panvk_batch, batch, &cmdbuf->batches, node) {
      list_del(&batch->node);
   util_dynarray_fini(&batch->jobs);
                        panvk_pool_reset(&cmdbuf->desc_pool);
   panvk_pool_reset(&cmdbuf->tls_pool);
            for (unsigned i = 0; i < MAX_BIND_POINTS; i++)
      memset(&cmdbuf->bind_points[i].desc_state.sets, 0,
   }
      static void
   panvk_destroy_cmdbuf(struct vk_command_buffer *vk_cmdbuf)
   {
      struct panvk_cmd_buffer *cmdbuf =
                  list_for_each_entry_safe(struct panvk_batch, batch, &cmdbuf->batches, node) {
      list_del(&batch->node);
   util_dynarray_fini(&batch->jobs);
                        panvk_pool_cleanup(&cmdbuf->desc_pool);
   panvk_pool_cleanup(&cmdbuf->tls_pool);
   panvk_pool_cleanup(&cmdbuf->varying_pool);
   vk_command_buffer_finish(&cmdbuf->vk);
      }
      static VkResult
   panvk_create_cmdbuf(struct vk_command_pool *vk_pool,
         {
      struct panvk_device *device =
         struct panvk_cmd_pool *pool =
                  cmdbuf = vk_zalloc(&device->vk.alloc, sizeof(*cmdbuf), 8,
         if (!cmdbuf)
            VkResult result = vk_command_buffer_init(&pool->vk, &cmdbuf->vk,
         if (result != VK_SUCCESS) {
      vk_free(&device->vk.alloc, cmdbuf);
                        panvk_pool_init(&cmdbuf->desc_pool, &device->physical_device->pdev,
               panvk_pool_init(&cmdbuf->tls_pool, &device->physical_device->pdev,
                     panvk_pool_init(&cmdbuf->varying_pool, &device->physical_device->pdev,
                     list_inithead(&cmdbuf->batches);
   *cmdbuf_out = &cmdbuf->vk;
      }
      const struct vk_command_buffer_ops panvk_per_arch(cmd_buffer_ops) = {
      .create = panvk_create_cmdbuf,
   .reset = panvk_reset_cmdbuf,
      };
      VkResult
   panvk_per_arch(BeginCommandBuffer)(VkCommandBuffer commandBuffer,
         {
                                    }
      void
   panvk_per_arch(DestroyCommandPool)(VkDevice _device, VkCommandPool commandPool,
         {
      VK_FROM_HANDLE(panvk_device, device, _device);
                     panvk_bo_pool_cleanup(&pool->desc_bo_pool);
   panvk_bo_pool_cleanup(&pool->varying_bo_pool);
               }
      void
   panvk_per_arch(CmdDispatch)(VkCommandBuffer commandBuffer, uint32_t x,
         {
      VK_FROM_HANDLE(panvk_cmd_buffer, cmdbuf, commandBuffer);
   const struct panfrost_device *pdev = &cmdbuf->device->physical_device->pdev;
   struct panvk_dispatch_info dispatch = {
                  panvk_per_arch(cmd_close_batch)(cmdbuf);
            struct panvk_cmd_bind_point_state *bind_point_state =
         struct panvk_descriptor_state *desc_state = &bind_point_state->desc_state;
   const struct panvk_pipeline *pipeline = bind_point_state->pipeline;
   struct panfrost_ptr job =
            struct panvk_sysvals *sysvals = &desc_state->sysvals;
   sysvals->num_work_groups.u32[0] = x;
   sysvals->num_work_groups.u32[1] = y;
   sysvals->num_work_groups.u32[2] = z;
   sysvals->local_group_size.u32[0] = pipeline->cs.local_size.x;
   sysvals->local_group_size.u32[1] = pipeline->cs.local_size.y;
   sysvals->local_group_size.u32[2] = pipeline->cs.local_size.z;
            panvk_per_arch(cmd_alloc_tls_desc)(cmdbuf, false);
            panvk_prepare_non_vs_attribs(cmdbuf, bind_point_state);
   dispatch.attributes = desc_state->non_vs_attribs;
            panvk_cmd_prepare_ubos(cmdbuf, bind_point_state);
            panvk_cmd_prepare_textures(cmdbuf, bind_point_state);
            panvk_cmd_prepare_samplers(cmdbuf, bind_point_state);
            panvk_per_arch(emit_compute_job)(pipeline, &dispatch, job.cpu);
   panfrost_add_job(&cmdbuf->desc_pool.base, &batch->scoreboard,
            batch->tlsinfo.tls.size = pipeline->tls_size;
   batch->tlsinfo.wls.size = pipeline->wls_size;
   if (batch->tlsinfo.wls.size) {
      batch->wls_total_size =
               panvk_per_arch(cmd_close_batch)(cmdbuf);
      }
