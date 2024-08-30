   /*
   * Copyright © 2016 Intel Corporation
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
      #include "anv_private.h"
      #include "genxml/gen_macros.h"
   #include "genxml/genX_pack.h"
      #include "common/intel_l3_config.h"
      /**
   * This file implements some lightweight memcpy/memset operations on the GPU
   * using a vertex buffer and streamout.
   */
      /**
   * Returns the greatest common divisor of a and b that is a power of two.
   */
   static uint64_t
   gcd_pow2_u64(uint64_t a, uint64_t b)
   {
               unsigned a_log2 = ffsll(a) - 1;
            /* If either a or b is 0, then a_log2 or b_log2 will be UINT_MAX in which
   * case, the MIN2() will take the other one.  If both are 0 then we will
   * hit the assert above.
   */
      }
      static void
   emit_common_so_memcpy(struct anv_batch *batch, struct anv_device *device,
         {
   #if GFX_VER >= 8
      anv_batch_emit(batch, GENX(3DSTATE_VF_INSTANCING), vfi) {
      vfi.InstancingEnable = false;
      }
      #endif
         /* Disable all shader stages */
   anv_batch_emit(batch, GENX(3DSTATE_VS), vs);
   anv_batch_emit(batch, GENX(3DSTATE_HS), hs);
   anv_batch_emit(batch, GENX(3DSTATE_TE), te);
   anv_batch_emit(batch, GENX(3DSTATE_DS), DS);
   anv_batch_emit(batch, GENX(3DSTATE_GS), gs);
            anv_batch_emit(batch, GENX(3DSTATE_SBE), sbe) {
      sbe.VertexURBEntryReadOffset = 1;
   sbe.NumberofSFOutputAttributes = 1;
   #if GFX_VER >= 8
         sbe.ForceVertexURBEntryReadLength = true;
   #endif
               /* Emit URB setup.  We tell it that the VS is active because we want it to
   * allocate space for the VS.  Even though one isn't run, we need VUEs to
   * store the data that VF is going to pass to SOL.
   */
            genX(emit_urb_setup)(device, batch, l3_config,
         #if GFX_VER >= 8
      anv_batch_emit(batch, GENX(3DSTATE_VF_TOPOLOGY), topo) {
            #endif
         anv_batch_emit(batch, GENX(3DSTATE_VF_STATISTICS), vf) {
            }
      static void
   emit_so_memcpy(struct anv_batch *batch, struct anv_device *device,
               {
      /* The maximum copy block size is 4 32-bit components at a time. */
   assert(size % 4 == 0);
            enum isl_format format;
   switch (bs) {
   case 4:  format = ISL_FORMAT_R32_UINT;          break;
   case 8:  format = ISL_FORMAT_R32G32_UINT;       break;
   case 16: format = ISL_FORMAT_R32G32B32A32_UINT; break;
   default:
                  uint32_t *dw;
   dw = anv_batch_emitn(batch, 5, GENX(3DSTATE_VERTEX_BUFFERS));
   GENX(VERTEX_BUFFER_STATE_pack)(batch, dw + 1,
      &(struct GENX(VERTEX_BUFFER_STATE)) {
      .VertexBufferIndex = 32, /* Reserved for this */
   .AddressModifyEnable = true,
   .BufferStartingAddress = src,
      #if (GFX_VER >= 8)
         #else
         #endif
               dw = anv_batch_emitn(batch, 3, GENX(3DSTATE_VERTEX_ELEMENTS));
   GENX(VERTEX_ELEMENT_STATE_pack)(batch, dw + 1,
      &(struct GENX(VERTEX_ELEMENT_STATE)) {
      .VertexBufferIndex = 32,
   .Valid = true,
   .SourceElementFormat = format,
   .SourceElementOffset = 0,
   .Component0Control = (bs >= 4) ? VFCOMP_STORE_SRC : VFCOMP_STORE_0,
   .Component1Control = (bs >= 8) ? VFCOMP_STORE_SRC : VFCOMP_STORE_0,
   .Component2Control = (bs >= 12) ? VFCOMP_STORE_SRC : VFCOMP_STORE_0,
               anv_batch_emit(batch, GENX(3DSTATE_SO_BUFFER), sob) {
      sob.SOBufferIndex = 0;
   sob.MOCS = anv_mocs(device, dst.bo, ISL_SURF_USAGE_STREAM_OUT_BIT),
      #if GFX_VER >= 8
         sob.SOBufferEnable = true;
   #else
         sob.SurfacePitch = bs;
   #endif
      #if GFX_VER >= 8
         /* As SOL writes out data, it updates the SO_WRITE_OFFSET registers with
   * the end position of the stream.  We need to reset this value to 0 at
   * the beginning of the run or else SOL will start at the offset from
   * the previous draw.
   */
   sob.StreamOffsetWriteEnable = true;
   #endif
            #if GFX_VER <= 7
      /* The hardware can do this for us on BDW+ (see above) */
   anv_batch_emit(batch, GENX(MI_LOAD_REGISTER_IMM), load) {
      load.RegisterOffset = GENX(SO_WRITE_OFFSET0_num);
         #endif
         dw = anv_batch_emitn(batch, 5, GENX(3DSTATE_SO_DECL_LIST),
               GENX(SO_DECL_ENTRY_pack)(batch, dw + 3,
      &(struct GENX(SO_DECL_ENTRY)) {
      .Stream0Decl = {
      .OutputBufferSlot = 0,
   .RegisterIndex = 0,
               anv_batch_emit(batch, GENX(3DSTATE_STREAMOUT), so) {
      so.SOFunctionEnable = true;
   so.RenderingDisable = true;
   so.Stream0VertexReadOffset = 0;
   #if GFX_VER >= 8
         #else
         #endif
               anv_batch_emit(batch, GENX(3DPRIMITIVE), prim) {
      prim.VertexAccessType         = SEQUENTIAL;
   prim.PrimitiveTopologyType    = _3DPRIM_POINTLIST;
   prim.VertexCountPerInstance   = size / bs;
   prim.StartVertexLocation      = 0;
   prim.InstanceCount            = 1;
   prim.StartInstanceLocation    = 0;
         }
      void
   genX(emit_so_memcpy_init)(struct anv_memcpy_state *state,
               {
               state->batch = batch;
            const struct intel_l3_config *cfg = intel_get_default_l3_config(device->info);
            anv_batch_emit(batch, GENX(PIPELINE_SELECT), ps) {
                     }
      void
   genX(emit_so_memcpy_fini)(struct anv_memcpy_state *state)
   {
      genX(emit_apply_pipe_flushes)(state->batch, state->device, _3D,
                     if ((state->batch->next - state->batch->start) & 4)
      }
      void
   genX(emit_so_memcpy)(struct anv_memcpy_state *state,
               {
      if (GFX_VER >= 8 && !anv_use_relocations(state->device->physical) &&
      anv_gfx8_9_vb_cache_range_needs_workaround(&state->vb_bound,
               genX(emit_apply_pipe_flushes)(state->batch, state->device, _3D,
                              }
      void
   genX(cmd_buffer_so_memcpy)(struct anv_cmd_buffer *cmd_buffer,
               {
      if (size == 0)
            if (!cmd_buffer->state.current_l3_config) {
      const struct intel_l3_config *cfg =
                     genX(cmd_buffer_set_binding_for_gfx8_vb_flush)(cmd_buffer, 32, src, size);
                     emit_common_so_memcpy(&cmd_buffer->batch, cmd_buffer->device,
                  genX(cmd_buffer_update_dirty_vbs_for_gfx8_vb_flush)(cmd_buffer, SEQUENTIAL,
            /* Invalidate pipeline & raster discard since we touch
   * 3DSTATE_STREAMOUT.
   */
   cmd_buffer->state.gfx.dirty |= ANV_CMD_DIRTY_PIPELINE;
   BITSET_SET(cmd_buffer->vk.dynamic_graphics_state.dirty,
      }
