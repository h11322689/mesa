   /**************************************************************************
   *
   * Copyright 2012-2021 VMware, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   **************************************************************************/
      /*
   * Draw.h --
   *    Functions that render 3D primitives.
   */
         #include "Draw.h"
   #include "State.h"
   #include "Shader.h"
      #include "Debug.h"
      #include "util/u_draw.h"
   #include "util/u_memory.h"
      static unsigned
   ClampedUAdd(unsigned a,
         {
      unsigned c = a + b;
   if (c < a) {
         }
      }
         /* stride is required in order to set the element data */
   static void
   update_velems(Device *pDevice)
   {
      if (!pDevice->velems_changed)
            if(pDevice->element_layout) {
      struct cso_velems_state *state = &pDevice->element_layout->state;
   for (unsigned i = 0; i < state->count; i++)
                        }
      /*
   * We have to resolve the stream output state for empty geometry shaders.
   * In particular we've remapped the output indices when translating the
   * shaders so now the register_index variables in the stream output
   * state are incorrect and we need to remap them back to the correct
   * state.
   */
   static void
   ResolveState(Device *pDevice)
   {
      if (pDevice->bound_empty_gs && pDevice->bound_vs &&
      pDevice->bound_vs->state.tokens) {
   Shader *gs = pDevice->bound_empty_gs;
   Shader *vs = pDevice->bound_vs;
   bool remapped = false;
   struct pipe_context *pipe = pDevice->pipe;
   if (!gs->output_resolved) {
      for (unsigned i = 0; i < gs->state.stream_output.num_outputs; ++i) {
      unsigned mapping =
         if (mapping != gs->state.stream_output.output[i].register_index) {
      gs->state.stream_output.output[i].register_index = mapping;
         }
   if (remapped) {
      pipe->delete_gs_state(pipe, gs->handle);
      }
      }
      }
      }
         static struct pipe_resource *
   create_null_index_buffer(struct pipe_context *ctx, uint num_indices,
               {
      unsigned buf_size = num_indices * sizeof(unsigned);
   unsigned *buf = (unsigned*)MALLOC(buf_size);
                     ibuf = pipe_buffer_create_with_data(ctx,
                     *index_size = 4;
   *restart_index = 0xffffffff;
                        }
      /*
   * ----------------------------------------------------------------------
   *
   * Draw --
   *
   *    The Draw function draws nonindexed primitives.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   Draw(D3D10DDI_HDEVICE hDevice,   // IN
      UINT VertexCount,           // IN
      {
                                 assert(pDevice->primitive < MESA_PRIM_COUNT);
   util_draw_arrays(pDevice->pipe,
                  }
         /*
   * ----------------------------------------------------------------------
   *
   * DrawIndexed --
   *
   *    The DrawIndexed function draws indexed primitives.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   DrawIndexed(D3D10DDI_HDEVICE hDevice,  // IN
               UINT IndexCount,           // IN
   {
               Device *pDevice = CastDevice(hDevice);
   struct pipe_draw_info info;
   struct pipe_draw_start_count_bias draw;
   struct pipe_resource *null_ib = NULL;
   unsigned restart_index = pDevice->restart_index;
   unsigned index_size = pDevice->index_size;
                     /* XXX I don't think draw still needs this? */
   if (!pDevice->index_buffer) {
      null_ib =
      create_null_index_buffer(pDevice->pipe,
                           util_draw_init_info(&info);
   info.index_size = index_size;
   info.mode = pDevice->primitive;
   draw.start = ClampedUAdd(StartIndexLocation, ib_offset / index_size);
   draw.count = IndexCount;
   info.index.resource = null_ib ? null_ib : pDevice->index_buffer;
   draw.index_bias = BaseVertexLocation;
   info.primitive_restart = true;
                     if (null_ib) {
            }
         /*
   * ----------------------------------------------------------------------
   *
   * DrawInstanced --
   *
   *    The DrawInstanced function draws particular instances
   *    of nonindexed primitives.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   DrawInstanced(D3D10DDI_HDEVICE hDevice,      // IN
               UINT VertexCountPerInstance,   // IN
   UINT InstanceCount,            // IN
   {
                        if (!InstanceCount) {
                           assert(pDevice->primitive < MESA_PRIM_COUNT);
   util_draw_arrays_instanced(pDevice->pipe,
                              }
         /*
   * ----------------------------------------------------------------------
   *
   * DrawIndexedInstanced --
   *
   *    The DrawIndexedInstanced function draws particular
   *    instances of indexed primitives.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   DrawIndexedInstanced(D3D10DDI_HDEVICE hDevice,   // IN
                        UINT IndexCountPerInstance, // IN
      {
               Device *pDevice = CastDevice(hDevice);
   struct pipe_draw_info info;
   struct pipe_draw_start_count_bias draw;
   struct pipe_resource *null_ib = NULL;
   unsigned restart_index = pDevice->restart_index;
   unsigned index_size = pDevice->index_size;
                     if (!InstanceCount) {
                  /* XXX I don't think draw still needs this? */
   if (!pDevice->index_buffer) {
      null_ib =
      create_null_index_buffer(pDevice->pipe,
                           util_draw_init_info(&info);
   info.index_size = index_size;
   info.mode = pDevice->primitive;
   draw.start = ClampedUAdd(StartIndexLocation, ib_offset / index_size);
   draw.count = IndexCountPerInstance;
   info.index.resource = null_ib ? null_ib : pDevice->index_buffer;
   draw.index_bias = BaseVertexLocation;
   info.start_instance = StartInstanceLocation;
   info.instance_count = InstanceCount;
   info.primitive_restart = true;
                     if (null_ib) {
            }
         /*
   * ----------------------------------------------------------------------
   *
   * DrawAuto --
   *
   *    The DrawAuto function works similarly to the Draw function,
   *    except DrawAuto is used for the special case where vertex
   *    data is written through the stream-output unit and then
   *    recycled as a vertex buffer. The driver determines the number
   *    of primitives, in part, by how much data was written to the
   *    buffer through stream output.
   *
   * ----------------------------------------------------------------------
   */
      void APIENTRY
   DrawAuto(D3D10DDI_HDEVICE hDevice)  // IN
   {
               Device *pDevice = CastDevice(hDevice);
   struct pipe_draw_info info;
               if (!pDevice->draw_so_target) {
      LOG_UNSUPPORTED("DrawAuto without a set source buffer!");
                                 util_draw_init_info(&info);
   info.mode = pDevice->primitive;
   memset(&indirect, 0, sizeof indirect);
               }
