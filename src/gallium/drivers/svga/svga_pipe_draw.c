   /**********************************************************
   * Copyright 2008-2023 VMware, Inc.  All rights reserved.
   *
   * Permission is hereby granted, free of charge, to any person
   * obtaining a copy of this software and associated documentation
   * files (the "Software"), to deal in the Software without
   * restriction, including without limitation the rights to use, copy,
   * modify, merge, publish, distribute, sublicense, and/or sell copies
   * of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be
   * included in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   **********************************************************/
         #include "util/u_draw.h"
   #include "util/format/u_format.h"
   #include "util/u_helpers.h"
   #include "util/u_inlines.h"
   #include "util/u_prim.h"
   #include "util/u_prim_restart.h"
      #include "svga_context.h"
   #include "svga_draw_private.h"
   #include "svga_screen.h"
   #include "svga_draw.h"
   #include "svga_shader.h"
   #include "svga_surface.h"
   #include "svga_swtnl.h"
   #include "svga_debug.h"
   #include "svga_resource_buffer.h"
         static enum pipe_error
   retry_draw_range_elements(struct svga_context *svga,
                     {
                        SVGA_STATS_TIME_POP(svga_sws(svga));
      }
         static enum pipe_error
   retry_draw_arrays( struct svga_context *svga,
                     {
                        SVGA_RETRY_OOM(svga, ret, svga_hwtnl_draw_arrays(svga->hwtnl, prim, start,
                     SVGA_STATS_TIME_POP(svga_sws(svga));
      }
         /**
   * Auto draw (get vertex count from a transform feedback result).
   */
   static enum pipe_error
   retry_draw_auto(struct svga_context *svga,
               {
      assert(svga_have_sm5(svga));
   assert(indirect->count_from_stream_output);
   assert(info->instance_count == 1);
   /* SO drawing implies core profile and none of these prim types */
   assert(info->mode != MESA_PRIM_QUADS &&
                  if (info->mode == MESA_PRIM_LINE_LOOP) {
      /* XXX need to do a fallback */
   assert(!"draw auto fallback not supported yet");
      }
   else {
      SVGA3dPrimitiveRange range;
            range.primType = svga_translate_prim(info->mode, 12, &hw_count,
         range.primitiveCount = 0;
   range.indexArray.surfaceId = SVGA3D_INVALID_ID;
   range.indexArray.offset = 0;
   range.indexArray.stride = 0;
   range.indexWidth = 0;
            SVGA_RETRY(svga, svga_hwtnl_prim
            (svga->hwtnl, &range,
      0,    /* vertex count comes from SO buffer */
   0,    /* don't know min index */
   ~0u,  /* don't know max index */
   NULL, /* no index buffer */
   0,    /* start instance */
                  }
         /**
   * Indirect draw (get vertex count, start index, etc. from a buffer object.
   */
   static enum pipe_error
   retry_draw_indirect(struct svga_context *svga,
               {
      assert(svga_have_sm5(svga));
   assert(indirect && indirect->buffer);
   /* indirect drawing implies core profile and none of these prim types */
   assert(info->mode != MESA_PRIM_QUADS &&
                  if (info->mode == MESA_PRIM_LINE_LOOP) {
      /* need to do a fallback */
   util_draw_indirect(&svga->pipe, info, indirect);
      }
   else {
      SVGA3dPrimitiveRange range;
            range.primType = svga_translate_prim(info->mode, 12, &hw_count,
         range.primitiveCount = 0;  /* specified in indirect buffer */
   range.indexArray.surfaceId = SVGA3D_INVALID_ID;
   range.indexArray.offset = 0;
   range.indexArray.stride = 0;
   range.indexWidth = info->index_size;
            SVGA_RETRY(svga, svga_hwtnl_prim
            (svga->hwtnl, &range,
      0,   /* vertex count is in indirect buffer */
   0,   /* don't know min index */
   ~0u, /* don't know max index */
   info->index.resource,
   info->start_instance,
                  }
         /**
   * Determine if we need to implement primitive restart with a fallback
   * path which breaks the original primitive into sub-primitive at the
   * restart indexes.
   */
   static bool
   need_fallback_prim_restart(const struct svga_context *svga,
         {
      if (info->primitive_restart && info->index_size) {
      if (!svga_have_vgpu10(svga))
         else if (!svga->state.sw.need_swtnl) {
      if (info->index_size == 1)
         else if (info->index_size == 2)
         else
                     }
         /**
   * A helper function to return the vertex count from the primitive count
   * returned from the stream output statistics query for the specified stream.
   */
   static unsigned
   get_vcount_from_stream_output(struct svga_context *svga,
               {
      unsigned primcount;
   primcount = svga_get_primcount_from_stream_output(svga, stream);
      }
         static void
   svga_draw_vbo(struct pipe_context *pipe, const struct pipe_draw_info *info,
               unsigned drawid_offset,
   const struct pipe_draw_indirect_info *indirect,
   {
      if (num_draws > 1) {
      util_draw_multi(pipe, info, drawid_offset, indirect, draws, num_draws);
               if (!indirect && (!draws[0].count || !info->instance_count))
            struct svga_context *svga = svga_context(pipe);
   enum mesa_prim reduced_prim = u_reduced_prim(info->mode);
   unsigned count = draws[0].count;
   enum pipe_error ret = 0;
                              if (u_reduced_prim(info->mode) == MESA_PRIM_TRIANGLES &&
      svga->curr.rast->templ.cull_face == PIPE_FACE_FRONT_AND_BACK)
         if (svga->curr.reduced_prim != reduced_prim) {
      svga->curr.reduced_prim = reduced_prim;
               /* We need to adjust the vertexID in the vertex shader since SV_VertexID
   * always start from 0 for DrawArrays and does not include baseVertex for
   * DrawIndexed.
   */
   unsigned index_bias = info->index_size ? draws->index_bias : 0;
   if (svga->curr.vertex_id_bias != (draws[0].start + index_bias)) {
      svga->curr.vertex_id_bias = draws[0].start + index_bias;
               if (svga->curr.vertices_per_patch != svga->patch_vertices) {
               /* If input patch size changes, we need to notifiy the TCS
   * code to reevaluate the shader variant since the
   * vertices per patch count is a constant in the control
   * point count declaration.
   */
   if (svga->curr.tcs || svga->curr.tes)
               if (need_fallback_prim_restart(svga, info)) {
      enum pipe_error r;
   r = util_draw_vbo_without_prim_restart(pipe, info, drawid_offset, indirect, &draws[0]);
   assert(r == PIPE_OK);
   (void) r;
               if (!indirect && !u_trim_pipe_prim(info->mode, &count))
                              if (svga->state.sw.need_swtnl) {
      svga->hud.num_fallbacks++;  /* for SVGA_QUERY_NUM_FALLBACKS */
   if (!needed_swtnl) {
      /*
   * We're switching from HW to SW TNL.  SW TNL will require mapping all
   * currently bound vertex buffers, some of which may already be
   * referenced in the current command buffer as result of previous HW
   * TNL. So flush now, to prevent the context to flush while a referred
                              /* Avoid leaking the previous hwtnl bias to swtnl */
   svga_hwtnl_set_index_bias(svga->hwtnl, 0);
      }
   else {
      if (!svga_update_state_retry(svga, SVGA_STATE_HW_DRAW)) {
      static const char *msg = "State update failed, skipping draw call";
   debug_printf("%s\n", msg);
   util_debug_message(&svga->debug.callback, INFO, "%s", msg);
      }
                     /** determine if flatshade is to be used after svga_update_state()
   *  in case the fragment shader is changed.
   */
   svga_hwtnl_set_flatshade(svga->hwtnl,
                        if (indirect && indirect->count_from_stream_output) {
                     /* If the vertex count is from the stream output of a non-zero stream
   * or the draw info specifies instancing, we will need a workaround
   * since the draw_auto command does not support stream instancing.
   * The workaround requires querying the vertex count from the
   * stream output statistics query for the specified stream and then
                  /* Check the stream index of the specified stream output target */
   for (unsigned i = 0; i < ARRAY_SIZE(svga->so_targets); i++) {
      if (svga->vcount_so_targets[i] == indirect->count_from_stream_output) {
      stream = (svga->vcount_buffer_stream >> (i * 4)) & 0xf;
         }
   if (info->instance_count > 1 || stream > 0) {
                     if (indirect && indirect->count_from_stream_output && count == 0) {
         }
   else if (indirect && indirect->buffer) {
         }
   else if (info->index_size) {
         }
   else {
      ret = retry_draw_arrays(svga, info->mode, draws[0].start, count,
                        /*
   * Mark currently bound target surfaces as dirty after draw is completed.
   */
            /* XXX: Silence warnings, do something sensible here? */
            if (SVGA_DEBUG & DEBUG_FLUSH) {
      svga_hwtnl_flush_retry(svga);
            done:
         }
         void
   svga_init_draw_functions(struct svga_context *svga)
   {
         }
