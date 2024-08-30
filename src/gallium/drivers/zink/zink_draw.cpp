   #include "zink_batch.h"
   #include "zink_compiler.h"
   #include "zink_context.h"
   #include "zink_descriptors.h"
   #include "zink_program.h"
   #include "zink_program_state.hpp"
   #include "zink_query.h"
   #include "zink_resource.h"
   #include "zink_screen.h"
   #include "zink_state.h"
   #include "zink_surface.h"
   #include "zink_inlines.h"
      #include "util/hash_table.h"
   #include "util/u_cpu_detect.h"
   #include "util/u_debug.h"
   #include "util/u_helpers.h"
   #include "util/u_inlines.h"
   #include "util/u_prim.h"
   #include "util/u_prim_restart.h"
      static void
   zink_emit_xfb_counter_barrier(struct zink_context *ctx)
   {
      for (unsigned i = 0; i < ctx->num_so_targets; i++) {
      struct zink_so_target *t = zink_so_target(ctx->so_targets[i]);
   if (!t)
         struct zink_resource *res = zink_resource(t->counter_buffer);
   VkAccessFlags access = VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT;
   VkPipelineStageFlags stage = VK_PIPELINE_STAGE_TRANSFORM_FEEDBACK_BIT_EXT;
   if (t->counter_buffer_valid) {
      /* Between the pause and resume there needs to be a memory barrier for the counter buffers
   * with a source access of VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT
   * at pipeline stage VK_PIPELINE_STAGE_TRANSFORM_FEEDBACK_BIT_EXT
   * to a destination access of VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT
   * at pipeline stage VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT.
   *
   * - from VK_EXT_transform_feedback spec
   */
   access |= VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT;
      }
   zink_screen(ctx->base.screen)->buffer_barrier(ctx, res, access, stage);
   if (!ctx->unordered_blitting)
         }
      static void
   zink_emit_stream_output_targets(struct pipe_context *pctx)
   {
      struct zink_context *ctx = zink_context(pctx);
   struct zink_batch *batch = &ctx->batch;
   VkBuffer buffers[PIPE_MAX_SO_BUFFERS] = {0};
   VkDeviceSize buffer_offsets[PIPE_MAX_SO_BUFFERS] = {0};
            for (unsigned i = 0; i < ctx->num_so_targets; i++) {
      struct zink_so_target *t = (struct zink_so_target *)ctx->so_targets[i];
   if (!t) {
      /* no need to reference this or anything */
   buffers[i] = zink_resource(ctx->dummy_xfb_buffer)->obj->buffer;
   buffer_offsets[i] = 0;
   buffer_sizes[i] = sizeof(uint8_t);
      }
   struct zink_resource *res = zink_resource(t->base.buffer);
   if (!res->so_valid)
      /* resource has been rebound */
      buffers[i] = res->obj->buffer;
   zink_batch_reference_resource_rw(batch, res, true);
   buffer_offsets[i] = t->base.buffer_offset;
   buffer_sizes[i] = t->base.buffer_size;
   res->so_valid = true;
   if (!ctx->unordered_blitting) {
      res->obj->unordered_read = res->obj->unordered_write = false;
   res->obj->access = VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT;
      }
   util_range_add(t->base.buffer, &res->valid_buffer_range, t->base.buffer_offset,
               VKCTX(CmdBindTransformFeedbackBuffersEXT)(batch->state->cmdbuf, 0, ctx->num_so_targets,
                  }
      ALWAYS_INLINE static void
   check_buffer_barrier(struct zink_context *ctx, struct pipe_resource *pres, VkAccessFlags flags, VkPipelineStageFlags pipeline)
   {
      struct zink_resource *res = zink_resource(pres);
   zink_screen(ctx->base.screen)->buffer_barrier(ctx, res, flags, pipeline);
   if (!ctx->unordered_blitting)
      }
      ALWAYS_INLINE static void
   barrier_draw_buffers(struct zink_context *ctx, const struct pipe_draw_info *dinfo,
         {
      if (index_buffer)
         if (dindirect && dindirect->buffer) {
      check_buffer_barrier(ctx, dindirect->buffer,
         if (dindirect->indirect_draw_count)
      check_buffer_barrier(ctx, dindirect->indirect_draw_count,
      }
      static void
   bind_vertex_buffers_dgc(struct zink_context *ctx)
   {
               ctx->vertex_buffers_dirty = false;
   if (!elems->hw_state.num_bindings)
         for (unsigned i = 0; i < elems->hw_state.num_bindings; i++) {
      struct pipe_vertex_buffer *vb = ctx->vertex_buffers + ctx->element_state->hw_state.binding_map[i];
   assert(vb);
   VkBindVertexBufferIndirectCommandNV *ptr;
   VkIndirectCommandsLayoutTokenNV *token = zink_dgc_add_token(ctx, VK_INDIRECT_COMMANDS_TOKEN_TYPE_VERTEX_BUFFER_NV, (void**)&ptr);
   token->vertexBindingUnit = ctx->element_state->hw_state.binding_map[i];
   if (vb->buffer.resource) {
      struct zink_resource *res = zink_resource(vb->buffer.resource);
   assert(res->obj->bda);
   ptr->bufferAddress = res->obj->bda + vb->buffer_offset;
   ptr->size = res->base.b.width0;
      } else {
      ptr->bufferAddress = 0;
   ptr->size = 0;
            }
      template <zink_dynamic_state DYNAMIC_STATE>
   static void
   zink_bind_vertex_buffers(struct zink_batch *batch, struct zink_context *ctx)
   {
      VkBuffer buffers[PIPE_MAX_ATTRIBS];
   VkDeviceSize buffer_offsets[PIPE_MAX_ATTRIBS];
   struct zink_vertex_elements_state *elems = ctx->element_state;
            for (unsigned i = 0; i < elems->hw_state.num_bindings; i++) {
      struct pipe_vertex_buffer *vb = ctx->vertex_buffers + elems->hw_state.binding_map[i];
   assert(vb);
   if (vb->buffer.resource) {
      struct zink_resource *res = zink_resource(vb->buffer.resource);
   assert(res->obj->buffer);
   buffers[i] = res->obj->buffer;
      } else {
      buffers[i] = zink_resource(ctx->dummy_vertex_buffer)->obj->buffer;
                  if (DYNAMIC_STATE != ZINK_NO_DYNAMIC_STATE &&
      DYNAMIC_STATE != ZINK_DYNAMIC_VERTEX_INPUT2 &&
   DYNAMIC_STATE != ZINK_DYNAMIC_VERTEX_INPUT) {
   if (elems->hw_state.num_bindings)
      VKCTX(CmdBindVertexBuffers2EXT)(batch->state->cmdbuf, 0,
         } else if (elems->hw_state.num_bindings)
      VKSCR(CmdBindVertexBuffers)(batch->state->cmdbuf, 0,
               if (DYNAMIC_STATE == ZINK_DYNAMIC_VERTEX_INPUT2 || DYNAMIC_STATE == ZINK_DYNAMIC_VERTEX_INPUT)
      VKCTX(CmdSetVertexInputEXT)(batch->state->cmdbuf,
                  }
      ALWAYS_INLINE static void
   update_drawid(struct zink_context *ctx, unsigned draw_id)
   {
      VKCTX(CmdPushConstants)(ctx->batch.state->cmdbuf, ctx->curr_program->base.layout, VK_SHADER_STAGE_ALL_GRAPHICS,
            }
      static void
   update_drawid_dgc(struct zink_context *ctx, unsigned draw_id)
   {
      uint32_t *ptr;
   VkIndirectCommandsLayoutTokenNV *token = zink_dgc_add_token(ctx, VK_INDIRECT_COMMANDS_TOKEN_TYPE_PUSH_CONSTANT_NV, (void**)&ptr);
   token->pushconstantOffset = offsetof(struct zink_gfx_push_constant, draw_id);
   token->pushconstantSize = sizeof(unsigned);
      }
      ALWAYS_INLINE static void
   draw_indexed_dgc_need_index_buffer_unref(struct zink_context *ctx,
                  const struct pipe_draw_info *dinfo,
   const struct pipe_draw_start_count_bias *draws,
      {
      if (dinfo->increment_draw_id && needs_drawid) {
      for (unsigned i = 0; i < num_draws; i++) {
      update_drawid_dgc(ctx, draw_id);
   VkDrawIndexedIndirectCommand *ptr, cmd = {
         };
   zink_dgc_add_token(ctx, VK_INDIRECT_COMMANDS_TOKEN_TYPE_DRAW_INDEXED_NV, (void**)&ptr);
   *ptr = cmd;
         } else {
      if (needs_drawid)
         for (unsigned i = 0; i < num_draws; i++) {
      VkDrawIndexedIndirectCommand *ptr, cmd = {
         };
   zink_dgc_add_token(ctx, VK_INDIRECT_COMMANDS_TOKEN_TYPE_DRAW_INDEXED_NV, (void**)&ptr);
            }
      ALWAYS_INLINE static void
   draw_indexed_need_index_buffer_unref(struct zink_context *ctx,
               const struct pipe_draw_info *dinfo,
   const struct pipe_draw_start_count_bias *draws,
   unsigned num_draws,
   {
      VkCommandBuffer cmdbuf = ctx->batch.state->cmdbuf;
   if (dinfo->increment_draw_id && needs_drawid) {
      for (unsigned i = 0; i < num_draws; i++) {
      update_drawid(ctx, draw_id);
   VKCTX(CmdDrawIndexed)(cmdbuf,
      draws[i].count, dinfo->instance_count,
            } else {
      if (needs_drawid)
         for (unsigned i = 0; i < num_draws; i++)
      VKCTX(CmdDrawIndexed)(cmdbuf,
               }
      ALWAYS_INLINE static void
   draw_indexed_dgc(struct zink_context *ctx,
                  const struct pipe_draw_info *dinfo,
   const struct pipe_draw_start_count_bias *draws,
      {
      if (dinfo->increment_draw_id && needs_drawid) {
      for (unsigned i = 0; i < num_draws; i++) {
      update_drawid_dgc(ctx, draw_id);
   VkDrawIndexedIndirectCommand *ptr, cmd = {
         };
   zink_dgc_add_token(ctx, VK_INDIRECT_COMMANDS_TOKEN_TYPE_DRAW_INDEXED_NV, (void**)&ptr);
   *ptr = cmd;
         } else {
      if (needs_drawid)
         for (unsigned i = 0; i < num_draws; i++) {
      VkDrawIndexedIndirectCommand *ptr, cmd = {
         };
   zink_dgc_add_token(ctx, VK_INDIRECT_COMMANDS_TOKEN_TYPE_DRAW_INDEXED_NV, (void**)&ptr);
            }
      template <zink_multidraw HAS_MULTIDRAW>
   ALWAYS_INLINE static void
   draw_indexed(struct zink_context *ctx,
               const struct pipe_draw_info *dinfo,
   const struct pipe_draw_start_count_bias *draws,
   unsigned num_draws,
   {
      VkCommandBuffer cmdbuf = ctx->batch.state->cmdbuf;
   if (dinfo->increment_draw_id && needs_drawid) {
      for (unsigned i = 0; i < num_draws; i++) {
      update_drawid(ctx, draw_id);
   VKCTX(CmdDrawIndexed)(cmdbuf,
      draws[i].count, dinfo->instance_count,
            } else {
      if (needs_drawid)
         if (HAS_MULTIDRAW) {
      VKCTX(CmdDrawMultiIndexedEXT)(cmdbuf, num_draws, (const VkMultiDrawIndexedInfoEXT*)draws,
                  } else {
      for (unsigned i = 0; i < num_draws; i++)
      VKCTX(CmdDrawIndexed)(cmdbuf,
               }
      ALWAYS_INLINE static void
   draw_dgc(struct zink_context *ctx,
            const struct pipe_draw_info *dinfo,
   const struct pipe_draw_start_count_bias *draws,
   unsigned num_draws,
      {
      if (dinfo->increment_draw_id && needs_drawid) {
      for (unsigned i = 0; i < num_draws; i++) {
      update_drawid_dgc(ctx, draw_id);
   VkDrawIndirectCommand *ptr, cmd = {
         };
   zink_dgc_add_token(ctx, VK_INDIRECT_COMMANDS_TOKEN_TYPE_DRAW_NV, (void**)&ptr);
   *ptr = cmd;
         } else {
      if (needs_drawid)
         for (unsigned i = 0; i < num_draws; i++) {
      VkDrawIndirectCommand *ptr, cmd = {
         };
   zink_dgc_add_token(ctx, VK_INDIRECT_COMMANDS_TOKEN_TYPE_DRAW_NV, (void**)&ptr);
            }
      template <zink_multidraw HAS_MULTIDRAW>
   ALWAYS_INLINE static void
   draw(struct zink_context *ctx,
      const struct pipe_draw_info *dinfo,
   const struct pipe_draw_start_count_bias *draws,
   unsigned num_draws,
   unsigned draw_id,
      {
      VkCommandBuffer cmdbuf = ctx->batch.state->cmdbuf;
   if (dinfo->increment_draw_id && needs_drawid) {
      for (unsigned i = 0; i < num_draws; i++) {
      update_drawid(ctx, draw_id);
   VKCTX(CmdDraw)(cmdbuf, draws[i].count, dinfo->instance_count, draws[i].start, dinfo->start_instance);
         } else {
      if (needs_drawid)
         if (HAS_MULTIDRAW)
      VKCTX(CmdDrawMultiEXT)(cmdbuf, num_draws, (const VkMultiDrawInfoEXT*)draws,
            else {
                        }
      template <zink_dynamic_state DYNAMIC_STATE, bool BATCH_CHANGED>
   static bool
   update_gfx_pipeline(struct zink_context *ctx, struct zink_batch_state *bs, enum mesa_prim mode, bool can_dgc)
   {
      VkPipeline prev_pipeline = ctx->gfx_pipeline_state.pipeline;
   const struct zink_screen *screen = zink_screen(ctx->base.screen);
   bool shaders_changed = ctx->gfx_dirty || ctx->dirty_gfx_stages;
   if (screen->optimal_keys && !ctx->is_generated_gs_bound)
         else
         bool pipeline_changed = false;
   VkPipeline pipeline = VK_NULL_HANDLE;
   if (!ctx->curr_program->base.uses_shobj) {
      if (screen->info.have_EXT_graphics_pipeline_library)
         else
      }
   if (pipeline) {
      pipeline_changed = prev_pipeline != pipeline;
   if (BATCH_CHANGED || pipeline_changed || ctx->shobj_draw) {
      ctx->dgc.last_prog = ctx->curr_program;
   if (unlikely(can_dgc && screen->info.nv_dgc_props.maxGraphicsShaderGroupCount == 1)) {
      VkBindShaderGroupIndirectCommandNV *ptr;
   zink_dgc_add_token(ctx, VK_INDIRECT_COMMANDS_TOKEN_TYPE_SHADER_GROUP_NV, (void**)&ptr);
   util_dynarray_append(&ctx->dgc.pipelines, VkPipeline, pipeline);
   /* zero-indexed -> base + group + num_pipelines-1 = base + num_pipelines */
      } else {
            }
      } else {
      if (BATCH_CHANGED || shaders_changed || !ctx->shobj_draw) {
      VkShaderStageFlagBits stages[] = {
      VK_SHADER_STAGE_VERTEX_BIT,
   VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
   VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
   VK_SHADER_STAGE_GEOMETRY_BIT,
      };
   /* always rebind all stages */
   VKCTX(CmdBindShadersEXT)(bs->cmdbuf, ZINK_GFX_SHADER_COUNT, stages, ctx->curr_program->objects);
   VKCTX(CmdSetDepthBiasEnable)(bs->cmdbuf, VK_TRUE);
   VKCTX(CmdSetTessellationDomainOriginEXT)(bs->cmdbuf, VK_TESSELLATION_DOMAIN_ORIGIN_LOWER_LEFT);
      }
      }
      }
      static enum mesa_prim
   zink_prim_type(const struct zink_context *ctx,
         {
      if (ctx->gfx_pipeline_state.shader_rast_prim != MESA_PRIM_COUNT)
               }
      static enum mesa_prim
   zink_rast_prim(const struct zink_context *ctx,
         {
      enum mesa_prim prim_type = zink_prim_type(ctx, dinfo);
            if (prim_type == MESA_PRIM_TRIANGLES &&
      ctx->rast_state->base.fill_front != PIPE_POLYGON_MODE_FILL) {
   switch(ctx->rast_state->base.fill_front) {
   case PIPE_POLYGON_MODE_POINT:
         case PIPE_POLYGON_MODE_LINE:
         default:
                        }
      template <zink_multidraw HAS_MULTIDRAW, zink_dynamic_state DYNAMIC_STATE, bool BATCH_CHANGED, bool DRAW_STATE>
   void
   zink_draw(struct pipe_context *pctx,
            const struct pipe_draw_info *dinfo,
   unsigned drawid_offset,
   const struct pipe_draw_indirect_info *dindirect,
   const struct pipe_draw_start_count_bias *draws,
   unsigned num_draws,
      {
      if (!dindirect && (!draws[0].count || !dinfo->instance_count))
            struct zink_context *ctx = zink_context(pctx);
   struct zink_screen *screen = zink_screen(pctx->screen);
   struct zink_rasterizer_state *rast_state = ctx->rast_state;
   struct zink_depth_stencil_alpha_state *dsa_state = ctx->dsa_state;
   struct zink_batch *batch = &ctx->batch;
   struct zink_so_target *so_target =
      dindirect && dindirect->count_from_stream_output ?
      VkBuffer counter_buffers[PIPE_MAX_SO_BUFFERS];
   VkDeviceSize counter_buffer_offsets[PIPE_MAX_SO_BUFFERS];
   bool need_index_buffer_unref = false;
   bool mode_changed = ctx->gfx_pipeline_state.gfx_prim_mode != dinfo->mode;
   bool reads_drawid = ctx->shader_reads_drawid;
   bool reads_basevertex = ctx->shader_reads_basevertex;
   unsigned work_count = ctx->batch.work_count;
            if (ctx->memory_barrier && !ctx->blitting)
            if (unlikely(ctx->buffer_rebind_counter < screen->buffer_rebind_counter && !ctx->blitting)) {
      ctx->buffer_rebind_counter = screen->buffer_rebind_counter;
               if (unlikely(ctx->image_rebind_counter < screen->image_rebind_counter && !ctx->blitting)) {
      ctx->image_rebind_counter = screen->image_rebind_counter;
               if (mode_changed)
            unsigned index_offset = 0;
   unsigned index_size = dinfo->index_size;
   struct pipe_resource *index_buffer = NULL;
   if (index_size > 0) {
      if (dinfo->has_user_indices) {
      if (!util_upload_index_buffer(pctx, dinfo, &draws[0], &index_buffer, &index_offset, 4)) {
      debug_printf("util_upload_index_buffer() failed\n");
      }
   /* this will have extra refs from tc */
   if (screen->threaded)
         else
      } else {
      index_buffer = dinfo->index.resource;
      }
   assert(index_size <= 4 && index_size != 3);
                        bool have_streamout = !!ctx->num_so_targets;
   if (have_streamout) {
      zink_emit_xfb_counter_barrier(ctx);
   if (ctx->dirty_so_targets) {
      /* have to loop here and below because barriers must be emitted out of renderpass,
   * but xfb buffers can't be bound before the renderpass is active to avoid
   * breaking from recursion
   */
   for (unsigned i = 0; i < ctx->num_so_targets; i++) {
      struct zink_so_target *t = (struct zink_so_target *)ctx->so_targets[i];
   if (t) {
      struct zink_resource *res = zink_resource(t->base.buffer);
   zink_screen(ctx->base.screen)->buffer_barrier(ctx, res,
         if (!ctx->unordered_blitting)
                        barrier_draw_buffers(ctx, dinfo, dindirect, index_buffer);
   /* this may re-emit draw buffer barriers, but such synchronization is harmless */
   if (!ctx->blitting)
            bool can_dgc = false;
   if (unlikely(zink_debug & ZINK_DEBUG_DGC))
            /* ensure synchronization between doing streamout with counter buffer
   * and using counter buffer for indirect draw
   */
   if (so_target && so_target->counter_buffer_valid) {
      struct zink_resource *res = zink_resource(so_target->counter_buffer);
   zink_screen(ctx->base.screen)->buffer_barrier(ctx, res,
               if (!ctx->unordered_blitting)
                        if (unlikely(zink_debug & ZINK_DEBUG_SYNC)) {
      zink_batch_no_rp(ctx);
   VkMemoryBarrier mb;
   mb.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
   mb.pNext = NULL;
   mb.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
   mb.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
   VKSCR(CmdPipelineBarrier)(ctx->batch.state->cmdbuf,
                           zink_batch_rp(ctx);
   /* check dead swapchain */
   if (unlikely(!ctx->batch.in_rp))
            if (BATCH_CHANGED)
            /* these must be after renderpass start to avoid issues with recursion */
   bool drawid_broken = false;
   if (reads_drawid && (!dindirect || !dindirect->buffer))
      drawid_broken = (drawid_offset != 0 ||
            if (drawid_broken != zink_get_last_vertex_key(ctx)->push_drawid)
            bool rast_prim_changed = false;
   bool prim_changed = false;
   bool rast_state_changed = ctx->rast_state_changed;
   if (mode_changed || ctx->gfx_pipeline_state.modules_changed ||
      rast_state_changed) {
   enum mesa_prim rast_prim = zink_rast_prim(ctx, dinfo);
   if (rast_prim != ctx->gfx_pipeline_state.rast_prim) {
      bool points_changed =
                           static bool rect_warned = false;
   if (DYNAMIC_STATE >= ZINK_DYNAMIC_STATE3 && rast_prim == MESA_PRIM_LINES && !rect_warned && 
      (VkLineRasterizationModeEXT)rast_state->hw_state.line_mode == VK_LINE_RASTERIZATION_MODE_RECTANGULAR_EXT) {
   if (screen->info.line_rast_feats.rectangularLines)
         else
                              if (points_changed && ctx->rast_state->base.point_quad_rasterization)
         }
            if ((mode_changed || prim_changed || rast_state_changed || ctx->gfx_pipeline_state.modules_changed)) {
                  if (index_size) {
      const VkIndexType index_type[3] = {
      VK_INDEX_TYPE_UINT8_EXT,
   VK_INDEX_TYPE_UINT16,
      };
   struct zink_resource *res = zink_resource(index_buffer);
   if (unlikely(can_dgc)) {
      VkBindIndexBufferIndirectCommandNV *ptr;
   zink_dgc_add_token(ctx, VK_INDIRECT_COMMANDS_TOKEN_TYPE_INDEX_BUFFER_NV, (void**)&ptr);
   ptr->bufferAddress = res->obj->bda + index_offset;
   ptr->size = res->base.b.width0;
      } else {
            }
   if (DYNAMIC_STATE < ZINK_DYNAMIC_STATE2) {
      if (ctx->gfx_pipeline_state.dyn_state2.primitive_restart != dinfo->primitive_restart)
                     if (have_streamout && ctx->dirty_so_targets)
                     if (BATCH_CHANGED || ctx->vp_state_changed || (DYNAMIC_STATE == ZINK_NO_DYNAMIC_STATE && pipeline_changed)) {
      VkViewport viewports[PIPE_MAX_VIEWPORTS];
   for (unsigned i = 0; i < ctx->vp_state.num_viewports; i++) {
      VkViewport viewport = {
      ctx->vp_state.viewport_states[i].translate[0] - ctx->vp_state.viewport_states[i].scale[0],
   ctx->vp_state.viewport_states[i].translate[1] - ctx->vp_state.viewport_states[i].scale[1],
   MAX2(ctx->vp_state.viewport_states[i].scale[0] * 2, 1),
   ctx->vp_state.viewport_states[i].scale[1] * 2,
   CLAMP(ctx->rast_state->base.clip_halfz ?
         ctx->vp_state.viewport_states[i].translate[2] :
   ctx->vp_state.viewport_states[i].translate[2] - ctx->vp_state.viewport_states[i].scale[2],
   CLAMP(ctx->vp_state.viewport_states[i].translate[2] + ctx->vp_state.viewport_states[i].scale[2],
      };
   if (!ctx->rast_state->base.half_pixel_center) {
      /* magic constant value from dxvk */
   float cf = 0.5f - (1.0f / 128.0f);
   viewport.x += cf;
   if (viewport.height < 0)
         else
      }
      }
   if (DYNAMIC_STATE != ZINK_NO_DYNAMIC_STATE)
         else
      }
   if (BATCH_CHANGED || ctx->scissor_changed || ctx->vp_state_changed || (DYNAMIC_STATE == ZINK_NO_DYNAMIC_STATE && pipeline_changed)) {
      VkRect2D scissors[PIPE_MAX_VIEWPORTS];
   if (ctx->rast_state->base.scissor) {
      for (unsigned i = 0; i < ctx->vp_state.num_viewports; i++) {
      scissors[i].offset.x = ctx->vp_state.scissor_states[i].minx;
   scissors[i].offset.y = ctx->vp_state.scissor_states[i].miny;
   scissors[i].extent.width = ctx->vp_state.scissor_states[i].maxx - ctx->vp_state.scissor_states[i].minx;
         } else {
      for (unsigned i = 0; i < ctx->vp_state.num_viewports; i++) {
      scissors[i].offset.x = 0;
   scissors[i].offset.y = 0;
   scissors[i].extent.width = ctx->fb_state.width;
         }
   if (DYNAMIC_STATE != ZINK_NO_DYNAMIC_STATE)
         else
      }
   ctx->vp_state_changed = false;
            if (BATCH_CHANGED || ctx->stencil_ref_changed) {
      VKCTX(CmdSetStencilReference)(batch->state->cmdbuf, VK_STENCIL_FACE_FRONT_BIT,
         VKCTX(CmdSetStencilReference)(batch->state->cmdbuf, VK_STENCIL_FACE_BACK_BIT,
                     if (DYNAMIC_STATE != ZINK_NO_DYNAMIC_STATE && (BATCH_CHANGED || ctx->dsa_state_changed)) {
      VKCTX(CmdSetDepthBoundsTestEnableEXT)(batch->state->cmdbuf, dsa_state->hw_state.depth_bounds_test);
   if (dsa_state->hw_state.depth_bounds_test)
      VKCTX(CmdSetDepthBounds)(batch->state->cmdbuf,
            VKCTX(CmdSetDepthTestEnableEXT)(batch->state->cmdbuf, dsa_state->hw_state.depth_test);
   VKCTX(CmdSetDepthCompareOpEXT)(batch->state->cmdbuf, dsa_state->hw_state.depth_compare_op);
   VKCTX(CmdSetDepthWriteEnableEXT)(batch->state->cmdbuf, dsa_state->hw_state.depth_write);
   VKCTX(CmdSetStencilTestEnableEXT)(batch->state->cmdbuf, dsa_state->hw_state.stencil_test);
   if (dsa_state->hw_state.stencil_test) {
      VKCTX(CmdSetStencilOpEXT)(batch->state->cmdbuf, VK_STENCIL_FACE_FRONT_BIT,
                           VKCTX(CmdSetStencilOpEXT)(batch->state->cmdbuf, VK_STENCIL_FACE_BACK_BIT,
                           if (dsa_state->base.stencil[1].enabled) {
      VKCTX(CmdSetStencilWriteMask)(batch->state->cmdbuf, VK_STENCIL_FACE_FRONT_BIT, dsa_state->hw_state.stencil_front.writeMask);
   VKCTX(CmdSetStencilWriteMask)(batch->state->cmdbuf, VK_STENCIL_FACE_BACK_BIT, dsa_state->hw_state.stencil_back.writeMask);
   VKCTX(CmdSetStencilCompareMask)(batch->state->cmdbuf, VK_STENCIL_FACE_FRONT_BIT, dsa_state->hw_state.stencil_front.compareMask);
      } else {
      VKCTX(CmdSetStencilWriteMask)(batch->state->cmdbuf, VK_STENCIL_FACE_FRONT_AND_BACK, dsa_state->hw_state.stencil_front.writeMask);
         } else {
      VKCTX(CmdSetStencilWriteMask)(batch->state->cmdbuf, VK_STENCIL_FACE_FRONT_AND_BACK, dsa_state->hw_state.stencil_front.writeMask);
   VKCTX(CmdSetStencilCompareMask)(batch->state->cmdbuf, VK_STENCIL_FACE_FRONT_AND_BACK, dsa_state->hw_state.stencil_front.compareMask);
         }
            if (BATCH_CHANGED || rast_state_changed) {
      if (DYNAMIC_STATE != ZINK_NO_DYNAMIC_STATE) {
      VKCTX(CmdSetFrontFaceEXT)(batch->state->cmdbuf, (VkFrontFace)ctx->gfx_pipeline_state.dyn_state1.front_face);
               if (DYNAMIC_STATE >= ZINK_DYNAMIC_STATE3) {
      if (ctx->ds3_states & BITFIELD_BIT(ZINK_DS3_RAST_STIPPLE))
         if (ctx->ds3_states & BITFIELD_BIT(ZINK_DS3_RAST_CLIP))
         if (ctx->ds3_states & BITFIELD_BIT(ZINK_DS3_RAST_CLAMP))
         if (ctx->ds3_states & BITFIELD_BIT(ZINK_DS3_RAST_POLYGON))
         if (ctx->ds3_states & BITFIELD_BIT(ZINK_DS3_RAST_HALFZ))
         if (ctx->ds3_states & BITFIELD_BIT(ZINK_DS3_RAST_PV))
      VKCTX(CmdSetProvokingVertexModeEXT)(batch->state->cmdbuf,
                  if (ctx->ds3_states & BITFIELD_BIT(ZINK_DS3_RAST_CLIP))
         if (ctx->ds3_states & BITFIELD_BIT(ZINK_DS3_RAST_STIPPLE_ON))
         }
   if ((BATCH_CHANGED || ctx->sample_mask_changed) && screen->have_full_ds3) {
      VKCTX(CmdSetRasterizationSamplesEXT)(batch->state->cmdbuf, (VkSampleCountFlagBits)(ctx->gfx_pipeline_state.rast_samples + 1));
   VKCTX(CmdSetSampleMaskEXT)(batch->state->cmdbuf, (VkSampleCountFlagBits)(ctx->gfx_pipeline_state.rast_samples + 1), &ctx->gfx_pipeline_state.sample_mask);
      }
   if ((BATCH_CHANGED || ctx->blend_state_changed)) {
      if (ctx->gfx_pipeline_state.blend_state) {
      if (ctx->ds3_states & BITFIELD_BIT(ZINK_DS3_BLEND_A2C))
      VKCTX(CmdSetAlphaToCoverageEnableEXT)(batch->state->cmdbuf, ctx->gfx_pipeline_state.blend_state->alpha_to_coverage &&
      if (ctx->ds3_states & BITFIELD_BIT(ZINK_DS3_BLEND_A21))
         if (ctx->fb_state.nr_cbufs) {
      if (ctx->ds3_states & BITFIELD_BIT(ZINK_DS3_BLEND_ON))
         if (ctx->ds3_states & BITFIELD_BIT(ZINK_DS3_BLEND_WRITE))
         if (ctx->ds3_states & BITFIELD_BIT(ZINK_DS3_BLEND_EQ))
      }
   if (ctx->ds3_states & BITFIELD_BIT(ZINK_DS3_BLEND_LOGIC_ON))
         if (ctx->ds3_states & BITFIELD_BIT(ZINK_DS3_BLEND_LOGIC))
         }
            if (BATCH_CHANGED ||
      /* only re-emit on non-batch change when actually drawing lines */
   ((ctx->line_width_changed || rast_prim_changed) && ctx->gfx_pipeline_state.rast_prim == MESA_PRIM_LINES)) {
   VKCTX(CmdSetLineWidth)(batch->state->cmdbuf, rast_state->line_width);
               if (BATCH_CHANGED || mode_changed ||
      ctx->gfx_pipeline_state.modules_changed ||
   rast_state_changed) {
   bool depth_bias =
                  if (depth_bias) {
      if (rast_state->base.offset_units_unscaled) {
         } else {
            } else {
            }
            if (DYNAMIC_STATE != ZINK_NO_DYNAMIC_STATE) {
      if (ctx->sample_locations_changed) {
      VkSampleLocationsInfoEXT loc;
   zink_init_vk_sample_locations(ctx, &loc);
      }
               if (BATCH_CHANGED || ctx->blend_state_changed) {
         }
            if (!DRAW_STATE) {
      if (BATCH_CHANGED || ctx->vertex_buffers_dirty) {
      if (unlikely(can_dgc))
         else if (DYNAMIC_STATE == ZINK_DYNAMIC_VERTEX_INPUT || ctx->gfx_pipeline_state.uses_dynamic_stride)
         else
                  if (BATCH_CHANGED) {
      ctx->pipeline_changed[0] = false;
               if (DYNAMIC_STATE != ZINK_NO_DYNAMIC_STATE && (BATCH_CHANGED || mode_changed))
            if (DYNAMIC_STATE >= ZINK_DYNAMIC_STATE2 && (BATCH_CHANGED || ctx->primitive_restart != dinfo->primitive_restart)) {
      VKCTX(CmdSetPrimitiveRestartEnableEXT)(batch->state->cmdbuf, dinfo->primitive_restart);
               if (DYNAMIC_STATE >= ZINK_DYNAMIC_STATE2 && (BATCH_CHANGED || ctx->rasterizer_discard_changed)) {
      VKCTX(CmdSetRasterizerDiscardEnableEXT)(batch->state->cmdbuf, ctx->gfx_pipeline_state.dyn_state2.rasterizer_discard);
               if (zink_program_has_descriptors(&ctx->curr_program->base))
            if (ctx->di.any_bindless_dirty &&
      /* some apps (d3dretrace) call MakeTextureHandleResidentARB randomly */
   zink_program_has_descriptors(&ctx->curr_program->base) &&
   ctx->curr_program->base.dd.bindless)
         if (reads_basevertex) {
      unsigned draw_mode_is_indexed = index_size > 0;
   if (unlikely(can_dgc)) {
      uint32_t *ptr;
   VkIndirectCommandsLayoutTokenNV *token = zink_dgc_add_token(ctx, VK_INDIRECT_COMMANDS_TOKEN_TYPE_PUSH_CONSTANT_NV, (void**)&ptr);
   token->pushconstantOffset = offsetof(struct zink_gfx_push_constant, draw_mode_is_indexed);
   token->pushconstantSize = sizeof(unsigned);
      } else {
      VKCTX(CmdPushConstants)(batch->state->cmdbuf, ctx->curr_program->base.layout, VK_SHADER_STAGE_ALL_GRAPHICS,
               }
   if (ctx->curr_program->shaders[MESA_SHADER_TESS_CTRL] &&
      ctx->curr_program->shaders[MESA_SHADER_TESS_CTRL]->non_fs.is_generated) {
   if (unlikely(can_dgc)) {
      float *ptr;
   VkIndirectCommandsLayoutTokenNV *token = zink_dgc_add_token(ctx, VK_INDIRECT_COMMANDS_TOKEN_TYPE_PUSH_CONSTANT_NV, (void**)&ptr);
   token->pushconstantOffset = offsetof(struct zink_gfx_push_constant, default_inner_level);
   token->pushconstantSize = sizeof(float) * 6;
      } else {
      VKCTX(CmdPushConstants)(batch->state->cmdbuf, ctx->curr_program->base.layout, VK_SHADER_STAGE_ALL_GRAPHICS,
                        if (!screen->optimal_keys) {
      if (zink_get_fs_key(ctx)->lower_line_stipple ||
                                                   float viewport_scale[2] = {
      ctx->vp_state.viewport_states[0].scale[0],
      };
   VKCTX(CmdPushConstants)(batch->state->cmdbuf,
                              uint32_t stipple = ctx->rast_state->base.line_stipple_pattern;
   stipple |= ctx->rast_state->base.line_stipple_factor << 16;
   VKCTX(CmdPushConstants)(batch->state->cmdbuf,
                              if (ctx->gfx_pipeline_state.shader_keys.key[MESA_SHADER_FRAGMENT].key.fs.lower_line_smooth) {
      float line_width = ctx->rast_state->base.line_width;
   VKCTX(CmdPushConstants)(batch->state->cmdbuf,
                                       if (have_streamout) {
      for (unsigned i = 0; i < ctx->num_so_targets; i++) {
      struct zink_so_target *t = zink_so_target(ctx->so_targets[i]);
   counter_buffers[i] = VK_NULL_HANDLE;
   if (t) {
      struct zink_resource *res = zink_resource(t->counter_buffer);
   t->stride = ctx->last_vertex_stage->sinfo.stride[i];
   zink_batch_reference_resource_rw(batch, res, true);
   if (!ctx->unordered_blitting)
         if (t->counter_buffer_valid) {
      counter_buffers[i] = res->obj->buffer;
            }
               bool marker = false;
   if (unlikely(zink_tracing)) {
      VkViewport viewport = {
      ctx->vp_state.viewport_states[0].translate[0] - ctx->vp_state.viewport_states[0].scale[0],
   ctx->vp_state.viewport_states[0].translate[1] - ctx->vp_state.viewport_states[0].scale[1],
   MAX2(ctx->vp_state.viewport_states[0].scale[0] * 2, 1),
   ctx->vp_state.viewport_states[0].scale[1] * 2,
   CLAMP(ctx->rast_state->base.clip_halfz ?
         ctx->vp_state.viewport_states[0].translate[2] :
   ctx->vp_state.viewport_states[0].translate[2] - ctx->vp_state.viewport_states[0].scale[2],
   CLAMP(ctx->vp_state.viewport_states[0].translate[2] + ctx->vp_state.viewport_states[0].scale[2],
      };
   if (ctx->blitting) {
      bool is_zs = util_format_is_depth_or_stencil(ctx->sampler_views[MESA_SHADER_FRAGMENT][0]->format);
   marker = zink_cmd_debug_marker_begin(ctx, VK_NULL_HANDLE, "u_blitter(%s->%s, %dx%d)",
                  } else {
      marker = zink_cmd_debug_marker_begin(ctx, VK_NULL_HANDLE, "draw(%u cbufs|%s, %dx%d)",
                              bool needs_drawid = reads_drawid && zink_get_last_vertex_key(ctx)->push_drawid;
   work_count += num_draws;
   if (index_size > 0) {
      if (dindirect && dindirect->buffer) {
      assert(num_draws == 1);
   if (needs_drawid)
         struct zink_resource *indirect = zink_resource(dindirect->buffer);
   zink_batch_reference_resource_rw(batch, indirect, false);
   if (dindirect->indirect_draw_count) {
      struct zink_resource *indirect_draw_count = zink_resource(dindirect->indirect_draw_count);
   zink_batch_reference_resource_rw(batch, indirect_draw_count, false);
   VKCTX(CmdDrawIndexedIndirectCount)(batch->state->cmdbuf, indirect->obj->buffer, dindirect->offset,
            } else
      } else {
      if (unlikely(can_dgc)) {
      if (need_index_buffer_unref)
         else
      } else if (need_index_buffer_unref) {
         } else {
               } else {
      if (so_target && screen->info.tf_props.transformFeedbackDraw) {
      /* GTF-GL46.gtf40.GL3Tests.transform_feedback2.transform_feedback2_api attempts a bogus xfb
   * draw using a streamout target that has no data
   * to avoid hanging the gpu, reject any such draws
   */
   if (so_target->counter_buffer_valid) {
      if (needs_drawid)
         zink_batch_reference_resource_rw(batch, zink_resource(so_target->base.buffer), false);
   zink_batch_reference_resource_rw(batch, zink_resource(so_target->counter_buffer), true);
   VKCTX(CmdDrawIndirectByteCountEXT)(batch->state->cmdbuf, dinfo->instance_count, dinfo->start_instance,
               } else if (dindirect && dindirect->buffer) {
      assert(num_draws == 1);
   if (needs_drawid)
         struct zink_resource *indirect = zink_resource(dindirect->buffer);
   zink_batch_reference_resource_rw(batch, indirect, false);
   if (dindirect->indirect_draw_count) {
      struct zink_resource *indirect_draw_count = zink_resource(dindirect->indirect_draw_count);
   zink_batch_reference_resource_rw(batch, indirect_draw_count, false);
   VKCTX(CmdDrawIndirectCount)(batch->state->cmdbuf, indirect->obj->buffer, dindirect->offset,
            } else
      } else {
      if (unlikely(can_dgc))
         else
                  if (unlikely(zink_tracing))
            ctx->dgc.valid = can_dgc;
   if (have_streamout) {
      for (unsigned i = 0; i < ctx->num_so_targets; i++) {
      struct zink_so_target *t = zink_so_target(ctx->so_targets[i]);
   if (t) {
      counter_buffers[i] = zink_resource(t->counter_buffer)->obj->buffer;
   counter_buffer_offsets[i] = t->counter_buffer_offset;
         }
               batch->has_work = true;
   batch->last_was_compute = false;
   ctx->batch.work_count = work_count;
   /* flush if there's >100k draws */
   if (!ctx->unordered_blitting && (unlikely(work_count >= 30000) || ctx->oom_flush))
      }
      template <zink_multidraw HAS_MULTIDRAW, zink_dynamic_state DYNAMIC_STATE, bool BATCH_CHANGED>
   static void
   zink_draw_vbo(struct pipe_context *pctx,
               const struct pipe_draw_info *info,
   unsigned drawid_offset,
   const struct pipe_draw_indirect_info *indirect,
   {
         }
      template <util_popcnt HAS_POPCNT>
   static void
   zink_vertex_state_mask(struct zink_context *ctx, struct pipe_vertex_state *vstate, uint32_t partial_velem_mask)
   {
      struct zink_vertex_state *zstate = (struct zink_vertex_state *)vstate;
            if (partial_velem_mask == vstate->input.full_velem_mask) {
      VKCTX(CmdSetVertexInputEXT)(cmdbuf,
                           VkVertexInputAttributeDescription2EXT dynattribs[PIPE_MAX_ATTRIBS];
   unsigned num_attribs = 0;
   u_foreach_bit(elem, vstate->input.full_velem_mask & partial_velem_mask) {
      unsigned idx = util_bitcount_fast<HAS_POPCNT>(vstate->input.full_velem_mask & BITFIELD_MASK(elem));
   dynattribs[num_attribs] = zstate->velems.hw_state.dynattribs[idx];
   dynattribs[num_attribs].location = num_attribs;
               VKCTX(CmdSetVertexInputEXT)(cmdbuf,
            }
      template <util_popcnt HAS_POPCNT>
   static void
   zink_bind_vertex_state(struct zink_context *ctx, struct pipe_vertex_state *vstate, uint32_t partial_velem_mask)
   {
      struct zink_vertex_state *zstate = (struct zink_vertex_state *)vstate;
   VkCommandBuffer cmdbuf = ctx->batch.state->cmdbuf;
   if (!vstate->input.vbuffer.buffer.resource)
                     struct zink_resource *res = zink_resource(vstate->input.vbuffer.buffer.resource);
   zink_batch_resource_usage_set(&ctx->batch, res, false, true);
   VkDeviceSize offset = vstate->input.vbuffer.buffer_offset;
   if (unlikely(zink_debug & ZINK_DEBUG_DGC)) {
      VkBindVertexBufferIndirectCommandNV *ptr;
   VkIndirectCommandsLayoutTokenNV *token = zink_dgc_add_token(ctx, VK_INDIRECT_COMMANDS_TOKEN_TYPE_VERTEX_BUFFER_NV, (void**)&ptr);
   token->vertexBindingUnit = 0;
   token->vertexDynamicStride = VK_FALSE;
   ptr->bufferAddress = res->obj->bda + offset;
   ptr->size = res->base.b.width0;
      } else {
      VKCTX(CmdBindVertexBuffers)(cmdbuf, 0,
               }
      template <zink_multidraw HAS_MULTIDRAW, zink_dynamic_state DYNAMIC_STATE, util_popcnt HAS_POPCNT, bool BATCH_CHANGED>
   static void
   zink_draw_vertex_state(struct pipe_context *pctx,
                        struct pipe_vertex_state *vstate,
      {
               dinfo.mode = info.mode;
   dinfo.index_size = 4;
   dinfo.instance_count = 1;
   dinfo.index.resource = vstate->input.indexbuf;
   struct zink_context *ctx = zink_context(pctx);
   struct zink_resource *res = zink_resource(vstate->input.vbuffer.buffer.resource);
   zink_screen(ctx->base.screen)->buffer_barrier(ctx, res, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
         if (!ctx->unordered_blitting)
                  zink_draw<HAS_MULTIDRAW, DYNAMIC_STATE, BATCH_CHANGED, true>(pctx, &dinfo, 0, NULL, draws, num_draws, vstate, partial_velem_mask);
   /* ensure ctx->vertex_buffers gets rebound on next non-vstate draw */
            if (info.take_vertex_state_ownership)
      }
      template <bool BATCH_CHANGED>
   static void
   zink_launch_grid(struct pipe_context *pctx, const struct pipe_grid_info *info)
   {
      struct zink_context *ctx = zink_context(pctx);
   struct zink_screen *screen = zink_screen(pctx->screen);
            if (ctx->render_condition_active)
            if (info->indirect) {
      /*
      VK_ACCESS_INDIRECT_COMMAND_READ_BIT specifies read access to indirect command data read as
                     */
               zink_update_barriers(ctx, true, NULL, info->indirect, NULL);
   if (ctx->memory_barrier)
            if (unlikely(zink_debug & ZINK_DEBUG_SYNC)) {
      zink_batch_no_rp(ctx);
   VkMemoryBarrier mb;
   mb.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
   mb.pNext = NULL;
   mb.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
   mb.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
   VKSCR(CmdPipelineBarrier)(ctx->batch.state->cmdbuf,
                           zink_program_update_compute_pipeline_state(ctx, ctx->curr_compute, info);
            if (BATCH_CHANGED) {
         }
   if (ctx->compute_dirty) {
      /* update inlinable constants */
   zink_update_compute_program(ctx);
               VkPipeline pipeline = zink_get_compute_pipeline(screen, ctx->curr_compute,
            if (prev_pipeline != pipeline || BATCH_CHANGED)
         if (BATCH_CHANGED) {
      ctx->pipeline_changed[1] = false;
               if (zink_program_has_descriptors(&ctx->curr_compute->base))
         if (ctx->di.any_bindless_dirty && ctx->curr_compute->base.dd.bindless)
            batch->work_count++;
   zink_batch_no_rp(ctx);
   if (!ctx->queries_disabled)
         if (info->indirect) {
      VKCTX(CmdDispatchIndirect)(batch->state->cmdbuf, zink_resource(info->indirect)->obj->buffer, info->indirect_offset);
      } else
         batch->has_work = true;
   batch->last_was_compute = true;
   /* flush if there's >100k computes */
   if (!ctx->unordered_blitting && (unlikely(ctx->batch.work_count >= 30000) || ctx->oom_flush))
      }
      template <zink_multidraw HAS_MULTIDRAW, zink_dynamic_state DYNAMIC_STATE, bool BATCH_CHANGED>
   static void
   init_batch_changed_functions(struct zink_context *ctx, pipe_draw_vbo_func draw_vbo_array[2][6][2], pipe_draw_vertex_state_func draw_state_array[2][6][2][2])
   {
      draw_vbo_array[HAS_MULTIDRAW][DYNAMIC_STATE][BATCH_CHANGED] = zink_draw_vbo<HAS_MULTIDRAW, DYNAMIC_STATE, BATCH_CHANGED>;
   draw_state_array[HAS_MULTIDRAW][DYNAMIC_STATE][0][BATCH_CHANGED] = zink_draw_vertex_state<HAS_MULTIDRAW, DYNAMIC_STATE, POPCNT_NO, BATCH_CHANGED>;
      }
      template <zink_multidraw HAS_MULTIDRAW, zink_dynamic_state DYNAMIC_STATE>
   static void
   init_dynamic_state_functions(struct zink_context *ctx, pipe_draw_vbo_func draw_vbo_array[2][6][2], pipe_draw_vertex_state_func draw_state_array[2][6][2][2])
   {
      init_batch_changed_functions<HAS_MULTIDRAW, DYNAMIC_STATE, false>(ctx, draw_vbo_array, draw_state_array);
      }
      template <zink_multidraw HAS_MULTIDRAW>
   static void
   init_multidraw_functions(struct zink_context *ctx, pipe_draw_vbo_func draw_vbo_array[2][6][2], pipe_draw_vertex_state_func draw_state_array[2][6][2][2])
   {
      init_dynamic_state_functions<HAS_MULTIDRAW, ZINK_NO_DYNAMIC_STATE>(ctx, draw_vbo_array, draw_state_array);
   init_dynamic_state_functions<HAS_MULTIDRAW, ZINK_DYNAMIC_STATE>(ctx, draw_vbo_array, draw_state_array);
   init_dynamic_state_functions<HAS_MULTIDRAW, ZINK_DYNAMIC_STATE2>(ctx, draw_vbo_array, draw_state_array);
   init_dynamic_state_functions<HAS_MULTIDRAW, ZINK_DYNAMIC_VERTEX_INPUT2>(ctx, draw_vbo_array, draw_state_array);
   init_dynamic_state_functions<HAS_MULTIDRAW, ZINK_DYNAMIC_STATE3>(ctx, draw_vbo_array, draw_state_array);
      }
      static void
   init_all_draw_functions(struct zink_context *ctx, pipe_draw_vbo_func draw_vbo_array[2][6][2], pipe_draw_vertex_state_func draw_state_array[2][6][2][2])
   {
      init_multidraw_functions<ZINK_NO_MULTIDRAW>(ctx, draw_vbo_array, draw_state_array);
      }
      template <bool BATCH_CHANGED>
   static void
   init_grid_batch_changed_functions(struct zink_context *ctx)
   {
         }
      static void
   init_all_grid_functions(struct zink_context *ctx)
   {
      init_grid_batch_changed_functions<false>(ctx);
      }
      static void
   zink_invalid_draw_vbo(struct pipe_context *pipe,
                        const struct pipe_draw_info *dinfo,
      {
         }
      static void
   zink_invalid_draw_vertex_state(struct pipe_context *pipe,
                                 {
         }
      static void
   zink_invalid_launch_grid(struct pipe_context *pctx, const struct pipe_grid_info *info)
   {
         }
      #define STAGE_BASE 0
   #define STAGE_BASE_GS (BITFIELD_BIT(MESA_SHADER_GEOMETRY) >> 1)
   #define STAGE_BASE_TES (BITFIELD_BIT(MESA_SHADER_TESS_EVAL) >> 1)
   #define STAGE_BASE_TES_GS ((BITFIELD_BIT(MESA_SHADER_TESS_EVAL) | BITFIELD_BIT(MESA_SHADER_GEOMETRY)) >> 1)
   #define STAGE_BASE_TCS_TES ((BITFIELD_BIT(MESA_SHADER_TESS_CTRL) | BITFIELD_BIT(MESA_SHADER_TESS_EVAL)) >> 1)
   #define STAGE_BASE_TCS_TES_GS ((BITFIELD_BIT(MESA_SHADER_TESS_CTRL) | BITFIELD_BIT(MESA_SHADER_TESS_EVAL) | BITFIELD_BIT(MESA_SHADER_GEOMETRY)) >> 1)
      template <unsigned STAGE_MASK>
   static uint32_t
   hash_gfx_program(const void *key)
   {
      const struct zink_shader **shaders = (const struct zink_shader**)key;
   uint32_t base_hash = shaders[MESA_SHADER_VERTEX]->hash ^ shaders[MESA_SHADER_FRAGMENT]->hash;
   if (STAGE_MASK == STAGE_BASE) //VS+FS
         if (STAGE_MASK == STAGE_BASE_GS) //VS+GS+FS
         /*VS+TCS+FS isn't a thing */
   /*VS+TCS+GS+FS isn't a thing */
   if (STAGE_MASK == STAGE_BASE_TES) //VS+TES+FS
         if (STAGE_MASK == STAGE_BASE_TES_GS) //VS+TES+GS+FS
         if (STAGE_MASK == STAGE_BASE_TCS_TES) //VS+TCS+TES+FS
            /* all stages */
      }
      template <unsigned STAGE_MASK>
   static bool
   equals_gfx_program(const void *a, const void *b)
   {
      const void **sa = (const void**)a;
   const void **sb = (const void**)b;
   STATIC_ASSERT(MESA_SHADER_VERTEX == 0);
   STATIC_ASSERT(MESA_SHADER_TESS_CTRL == 1);
   STATIC_ASSERT(MESA_SHADER_TESS_EVAL == 2);
   STATIC_ASSERT(MESA_SHADER_GEOMETRY == 3);
   STATIC_ASSERT(MESA_SHADER_FRAGMENT == 4);
   if (STAGE_MASK == STAGE_BASE) //VS+FS
      return sa[MESA_SHADER_VERTEX] == sb[MESA_SHADER_VERTEX] &&
      if (STAGE_MASK == STAGE_BASE_GS) //VS+GS+FS
      return sa[MESA_SHADER_VERTEX] == sb[MESA_SHADER_VERTEX] &&
      /*VS+TCS+FS isn't a thing */
   /*VS+TCS+GS+FS isn't a thing */
   if (STAGE_MASK == STAGE_BASE_TES) //VS+TES+FS
      return sa[MESA_SHADER_VERTEX] == sb[MESA_SHADER_VERTEX] &&
            if (STAGE_MASK == STAGE_BASE_TES_GS) //VS+TES+GS+FS
      return sa[MESA_SHADER_VERTEX] == sb[MESA_SHADER_VERTEX] &&
      if (STAGE_MASK == STAGE_BASE_TCS_TES) //VS+TCS+TES+FS
      return !memcmp(sa, sb, sizeof(void*) * 3) &&
         /* all stages */
      }
      extern "C"
   void
   zink_init_draw_functions(struct zink_context *ctx, struct zink_screen *screen)
   {
      pipe_draw_vbo_func draw_vbo_array[2][6] //multidraw, zink_dynamic_state
         pipe_draw_vertex_state_func draw_state_array[2][6] //multidraw, zink_dynamic_state
         zink_dynamic_state dynamic;
   if (screen->info.have_EXT_extended_dynamic_state) {
      if (screen->info.have_EXT_extended_dynamic_state2) {
      if (screen->info.have_EXT_extended_dynamic_state3) {
      if (screen->info.have_EXT_vertex_input_dynamic_state)
         else
      } else {
      if (screen->info.have_EXT_vertex_input_dynamic_state)
         else
         } else {
            } else {
         }
   init_all_draw_functions(ctx, draw_vbo_array, draw_state_array);
   memcpy(ctx->draw_vbo, &draw_vbo_array[screen->info.have_EXT_multi_draw]
               memcpy(ctx->draw_state, &draw_state_array[screen->info.have_EXT_multi_draw]
                  /* Bind a fake draw_vbo, so that draw_vbo isn't NULL, which would skip
   * initialization of callbacks in upper layers (such as u_threaded_context).
   */
   ctx->base.draw_vbo = zink_invalid_draw_vbo;
            _mesa_hash_table_init(&ctx->program_cache[0], ctx, hash_gfx_program<0>, equals_gfx_program<0>);
   _mesa_hash_table_init(&ctx->program_cache[1], ctx, hash_gfx_program<1>, equals_gfx_program<1>);
   _mesa_hash_table_init(&ctx->program_cache[2], ctx, hash_gfx_program<2>, equals_gfx_program<2>);
   _mesa_hash_table_init(&ctx->program_cache[3], ctx, hash_gfx_program<3>, equals_gfx_program<3>);
   _mesa_hash_table_init(&ctx->program_cache[4], ctx, hash_gfx_program<4>, equals_gfx_program<4>);
   _mesa_hash_table_init(&ctx->program_cache[5], ctx, hash_gfx_program<5>, equals_gfx_program<5>);
   _mesa_hash_table_init(&ctx->program_cache[6], ctx, hash_gfx_program<6>, equals_gfx_program<6>);
   _mesa_hash_table_init(&ctx->program_cache[7], ctx, hash_gfx_program<7>, equals_gfx_program<7>);
   for (unsigned i = 0; i < ARRAY_SIZE(ctx->program_lock); i++)
      }
      void
   zink_init_grid_functions(struct zink_context *ctx)
   {
      init_all_grid_functions(ctx);
   /* Bind a fake launch_grid, so that draw_vbo isn't NULL, which would skip
   * initialization of callbacks in upper layers (such as u_threaded_context).
   */
      }
      void
   zink_init_screen_pipeline_libs(struct zink_screen *screen)
   {
      _mesa_set_init(&screen->pipeline_libs[0], screen, hash_gfx_program<0>, equals_gfx_program<0>);
   _mesa_set_init(&screen->pipeline_libs[1], screen, hash_gfx_program<1>, equals_gfx_program<1>);
   _mesa_set_init(&screen->pipeline_libs[2], screen, hash_gfx_program<2>, equals_gfx_program<2>);
   _mesa_set_init(&screen->pipeline_libs[3], screen, hash_gfx_program<3>, equals_gfx_program<3>);
   _mesa_set_init(&screen->pipeline_libs[4], screen, hash_gfx_program<4>, equals_gfx_program<4>);
   _mesa_set_init(&screen->pipeline_libs[5], screen, hash_gfx_program<5>, equals_gfx_program<5>);
   _mesa_set_init(&screen->pipeline_libs[6], screen, hash_gfx_program<6>, equals_gfx_program<6>);
   _mesa_set_init(&screen->pipeline_libs[7], screen, hash_gfx_program<7>, equals_gfx_program<7>);
   for (unsigned i = 0; i < ARRAY_SIZE(screen->pipeline_libs_lock); i++)
      }
