   /*
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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "anv_private.h"
      #include "common/intel_aux_map.h"
   #include "common/intel_sample_positions.h"
   #include "common/intel_pixel_hash.h"
   #include "genxml/gen_macros.h"
   #include "genxml/genX_pack.h"
      #include "vk_standard_sample_locations.h"
      #if GFX_VERx10 == 125 && ANV_SUPPORT_RT
   #include "grl/genX_grl.h"
   #endif
      #include "vk_util.h"
   #include "vk_format.h"
      static void
   genX(emit_slice_hashing_state)(struct anv_device *device,
         {
   #if GFX_VER == 11
      /* Gfx11 hardware has two pixel pipes at most. */
   for (unsigned i = 2; i < ARRAY_SIZE(device->info->ppipe_subslices); i++)
            if (device->info->ppipe_subslices[0] == device->info->ppipe_subslices[1])
            if (!device->slice_hash.alloc_size) {
      unsigned size = GENX(SLICE_HASH_TABLE_length) * 4;
   device->slice_hash =
            const bool flip = device->info->ppipe_subslices[0] <
         struct GENX(SLICE_HASH_TABLE) table;
                        anv_batch_emit(batch, GENX(3DSTATE_SLICE_TABLE_STATE_POINTERS), ptr) {
      ptr.SliceHashStatePointerValid = true;
               anv_batch_emit(batch, GENX(3DSTATE_3D_MODE), mode) {
            #elif GFX_VERx10 == 120
      /* For each n calculate ppipes_of[n], equal to the number of pixel pipes
   * present with n active dual subslices.
   */
            for (unsigned n = 0; n < ARRAY_SIZE(ppipes_of); n++) {
      for (unsigned p = 0; p < 3; p++)
               /* Gfx12 has three pixel pipes. */
   for (unsigned p = 3; p < ARRAY_SIZE(device->info->ppipe_subslices); p++)
            if (ppipes_of[2] == 3 || ppipes_of[0] == 2) {
      /* All three pixel pipes have the maximum number of active dual
   * subslices, or there is only one active pixel pipe: Nothing to do.
   */
               anv_batch_emit(batch, GENX(3DSTATE_SUBSLICE_HASH_TABLE), p) {
               if (ppipes_of[2] == 2 && ppipes_of[0] == 1)
         else if (ppipes_of[2] == 1 && ppipes_of[1] == 1 && ppipes_of[0] == 1)
            if (ppipes_of[2] == 2 && ppipes_of[1] == 1)
         else if (ppipes_of[2] == 2 && ppipes_of[0] == 1)
         else if (ppipes_of[2] == 1 && ppipes_of[1] == 1 && ppipes_of[0] == 1)
         else
               anv_batch_emit(batch, GENX(3DSTATE_3D_MODE), p) {
      p.SubsliceHashingTableEnable = true;
         #elif GFX_VERx10 == 125
      uint32_t ppipe_mask = 0;
   for (unsigned p = 0; p < ARRAY_SIZE(device->info->ppipe_subslices); p++) {
      if (device->info->ppipe_subslices[p])
      }
            if (!device->slice_hash.alloc_size) {
      unsigned size = GENX(SLICE_HASH_TABLE_length) * 4;
   device->slice_hash =
                     /* Note that the hardware expects an array with 7 tables, each
   * table is intended to specify the pixel pipe hashing behavior
   * for every possible slice count between 2 and 8, however that
   * doesn't actually work, among other reasons due to hardware
   * bugs that will cause the GPU to erroneously access the table
   * at the wrong index in some cases, so in practice all 7 tables
   * need to be initialized to the same value.
   */
   for (unsigned i = 0; i < 7; i++)
                        anv_batch_emit(batch, GENX(3DSTATE_SLICE_TABLE_STATE_POINTERS), ptr) {
      ptr.SliceHashStatePointerValid = true;
               /* TODO: Figure out FCV support for other platforms
   * Testing indicates that FCV is broken on MTL, but works fine on DG2.
   * Let's disable FCV on MTL for now till we figure out what's wrong.
   *
   * Alternatively, it can be toggled off via drirc option 'anv_disable_fcv'.
   *
   * Ref: https://gitlab.freedesktop.org/mesa/mesa/-/issues/9987
   */
   anv_batch_emit(batch, GENX(3DSTATE_3D_MODE), mode) {
      mode.SliceHashingTableEnable = true;
   mode.SliceHashingTableEnableMask = true;
   mode.CrossSliceHashingMode = (util_bitcount(ppipe_mask) > 1 ?
   hashing32x32 : NormalMode);
   mode.CrossSliceHashingModeMask = -1;
   mode.FastClearOptimizationEnable = !device->physical->disable_fcv;
         #endif
   }
      static void
   init_common_queue_state(struct anv_queue *queue, struct anv_batch *batch)
   {
            #if GFX_VER >= 11
      /* Starting with GFX version 11, SLM is no longer part of the L3$ config
   * so it never changes throughout the lifetime of the VkDevice.
   */
   const struct intel_l3_config *cfg = intel_get_default_l3_config(device->info);
   genX(emit_l3_config)(batch, device, cfg);
      #endif
      #if GFX_VERx10 == 125
      /* Even though L3 partial write merging is supposed to be enabled
   * by default on Gfx12.5 according to the hardware spec, i915
   * appears to accidentally clear the enables during context
   * initialization, so make sure to enable them here since partial
   * write merging has a large impact on rendering performance.
   */
   anv_batch_write_reg(batch, GENX(L3SQCREG5), reg) {
      reg.L3CachePartialWriteMergeTimerInitialValue = 0x7f;
   reg.CompressiblePartialWriteMergeEnable = true;
   reg.CoherentPartialWriteMergeEnable = true;
         #endif
         /* Emit STATE_BASE_ADDRESS on Gfx12+ because we set a default CPS_STATE and
   * those are relative to STATE_BASE_ADDRESS::DynamicStateBaseAddress.
      #if GFX_VER >= 12
      #if GFX_VERx10 >= 125
      /* Wa_14016407139:
   *
   * "On Surface state base address modification, for 3D workloads, SW must
   *  always program PIPE_CONTROL either with CS Stall or PS sync stall. In
   *  both the cases set Render Target Cache Flush Enable".
   */
   genx_batch_emit_pipe_control(batch, device->info,
            #endif
         /* GEN:BUG:1607854226:
   *
   *  Non-pipelined state has issues with not applying in MEDIA/GPGPU mode.
   *  Fortunately, we always start the context off in 3D mode.
   */
   uint32_t mocs = device->isl_dev.mocs.internal;
   anv_batch_emit(batch, GENX(STATE_BASE_ADDRESS), sba) {
      sba.GeneralStateBaseAddress = (struct anv_address) { NULL, 0 };
   sba.GeneralStateBufferSize  = 0xfffff;
   sba.GeneralStateMOCS = mocs;
   sba.GeneralStateBaseAddressModifyEnable = true;
                     sba.SurfaceStateBaseAddress =
      (struct anv_address) { .offset =
      };
   sba.SurfaceStateMOCS = mocs;
            sba.DynamicStateBaseAddress =
      (struct anv_address) { .offset =
      };
   sba.DynamicStateBufferSize = device->physical->va.dynamic_state_pool.size / 4096;
   sba.DynamicStateMOCS = mocs;
   sba.DynamicStateBaseAddressModifyEnable = true;
            sba.IndirectObjectBaseAddress = (struct anv_address) { NULL, 0 };
   sba.IndirectObjectBufferSize = 0xfffff;
   sba.IndirectObjectMOCS = mocs;
   sba.IndirectObjectBaseAddressModifyEnable = true;
            sba.InstructionBaseAddress =
      (struct anv_address) { .offset =
      };
   sba.InstructionBufferSize = device->physical->va.instruction_state_pool.size / 4096;
   sba.InstructionMOCS = mocs;
   sba.InstructionBaseAddressModifyEnable = true;
            if (device->physical->indirect_descriptors) {
      sba.BindlessSurfaceStateBaseAddress =
      (struct anv_address) { .offset =
      };
   sba.BindlessSurfaceStateSize =
                        sba.BindlessSamplerStateBaseAddress = (struct anv_address) { NULL, 0 };
   sba.BindlessSamplerStateMOCS = mocs;
   sba.BindlessSamplerStateBaseAddressModifyEnable = true;
      } else {
      /* Bindless Surface State & Bindless Sampler State are aligned to the
   * same heap
   */
   sba.BindlessSurfaceStateBaseAddress =
      sba.BindlessSamplerStateBaseAddress =
      sba.BindlessSurfaceStateSize =
      (device->physical->va.binding_table_pool.size +
   device->physical->va.internal_surface_state_pool.size +
      sba.BindlessSamplerStateBufferSize =
      (device->physical->va.binding_table_pool.size +
   device->physical->va.internal_surface_state_pool.size +
      sba.BindlessSurfaceStateMOCS = sba.BindlessSamplerStateMOCS = mocs;
   sba.BindlessSurfaceStateBaseAddressModifyEnable =
         #if GFX_VERx10 >= 125
         #endif
         #endif
      #if GFX_VERx10 >= 125
      if (ANV_SUPPORT_RT && device->info->has_ray_tracing) {
      anv_batch_emit(batch, GENX(3DSTATE_BTD), btd) {
      /* TODO: This is the timeout after which the bucketed thread
   *       dispatcher will kick off a wave of threads. We go with the
   *       lowest value for now. It could be tweaked on a per
   *       application basis (drirc).
   */
   btd.DispatchTimeoutCounter = _64clocks;
   /* BSpec 43851: "This field must be programmed to 6h i.e. memory
   *               backed buffer must be 128KB."
   */
   btd.PerDSSMemoryBackedBufferSize = 6;
   btd.MemoryBackedBufferBasePointer = (struct anv_address) {
      /* This batch doesn't have a reloc list so we can't use the BO
   * here.  We just use the address directly.
   */
               #endif
   }
      static VkResult
   init_render_queue_state(struct anv_queue *queue, bool is_companion_rcs_batch)
   {
      struct anv_device *device = queue->device;
   UNUSED const struct intel_device_info *devinfo = queue->device->info;
   uint32_t cmds[128];
   struct anv_batch batch = {
      .start = cmds,
   .next = cmds,
               struct GENX(VERTEX_ELEMENT_STATE) empty_ve = {
      .Valid = true,
   .Component0Control = VFCOMP_STORE_0,
   .Component1Control = VFCOMP_STORE_0,
   .Component2Control = VFCOMP_STORE_0,
      };
                  #if GFX_VER == 9
      anv_batch_write_reg(&batch, GENX(CACHE_MODE_1), cm1) {
      cm1.FloatBlendOptimizationEnable = true;
   cm1.FloatBlendOptimizationEnableMask = true;
   cm1.MSCRAWHazardAvoidanceBit = true;
   cm1.MSCRAWHazardAvoidanceBitMask = true;
   cm1.PartialResolveDisableInVC = true;
         #endif
                  anv_batch_emit(&batch, GENX(3DSTATE_DRAWING_RECTANGLE), rect) {
      rect.ClippedDrawingRectangleYMin = 0;
   rect.ClippedDrawingRectangleXMin = 0;
   rect.ClippedDrawingRectangleYMax = UINT16_MAX;
   rect.ClippedDrawingRectangleXMax = UINT16_MAX;
   rect.DrawingRectangleOriginY = 0;
                        /* SKL PRMs, Volume 2a: Command Reference: Instructions: 3DSTATE_WM_HZ_OP:
   *
   *   "3DSTATE_RASTER if used must be programmed prior to using this
   *    packet."
   *
   * Emit this before 3DSTATE_WM_HZ_OP below.
   */
   anv_batch_emit(&batch, GENX(3DSTATE_RASTER), rast) {
                  /* SKL PRMs, Volume 2a: Command Reference: Instructions: 3DSTATE_WM_HZ_OP:
   *
   *    "3DSTATE_MULTISAMPLE packet must be used prior to this packet to
   *     change the Number of Multisamples. This packet must not be used to
   *     change Number of Multisamples in a rendering sequence."
   *
   * Emit this before 3DSTATE_WM_HZ_OP below.
   */
            /* The BDW+ docs describe how to use the 3DSTATE_WM_HZ_OP instruction in the
   * section titled, "Optimized Depth Buffer Clear and/or Stencil Buffer
   * Clear." It mentions that the packet overrides GPU state for the clear
   * operation and needs to be reset to 0s to clear the overrides. Depending
   * on the kernel, we may not get a context with the state for this packet
   * zeroed. Do it ourselves just in case. We've observed this to prevent a
   * number of GPU hangs on ICL.
   */
                  #if GFX_VER == 11
      /* The default behavior of bit 5 "Headerless Message for Pre-emptable
   * Contexts" in SAMPLER MODE register is set to 0, which means
   * headerless sampler messages are not allowed for pre-emptable
   * contexts. Set the bit 5 to 1 to allow them.
   */
   anv_batch_write_reg(&batch, GENX(SAMPLER_MODE), sm) {
      sm.HeaderlessMessageforPreemptableContexts = true;
               /* Bit 1 "Enabled Texel Offset Precision Fix" must be set in
   * HALF_SLICE_CHICKEN7 register.
   */
   anv_batch_write_reg(&batch, GENX(HALF_SLICE_CHICKEN7), hsc7) {
      hsc7.EnabledTexelOffsetPrecisionFix = true;
               anv_batch_write_reg(&batch, GENX(TCCNTLREG), tcc) {
      tcc.L3DataPartialWriteMergingEnable = true;
   tcc.ColorZPartialWriteMergingEnable = true;
   tcc.URBPartialWriteMergingEnable = true;
         #endif
            #if GFX_VER >= 11
      /* hardware specification recommends disabling repacking for
   * the compatibility with decompression mechanism in display controller.
   */
   if (device->info->disable_ccs_repack) {
      anv_batch_write_reg(&batch, GENX(CACHE_MODE_0), cm0) {
      cm0.DisableRepackingforCompression = true;
                  /* an unknown issue is causing vs push constants to become
   * corrupted during object-level preemption. For now, restrict
   * to command buffer level preemption to avoid rendering
   * corruption.
   */
   anv_batch_write_reg(&batch, GENX(CS_CHICKEN1), cc1) {
      cc1.ReplayMode = MidcmdbufferPreemption;
      #if GFX_VERx10 == 120
         cc1.DisablePreemptionandHighPriorityPausingdueto3DPRIMITIVECommand = true;
   #endif
            #if INTEL_NEEDS_WA_1806527549
      /* Wa_1806527549 says to disable the following HiZ optimization when the
   * depth buffer is D16_UNORM. We've found the WA to help with more depth
   * buffer configurations however, so we always disable it just to be safe.
   */
   anv_batch_write_reg(&batch, GENX(HIZ_CHICKEN), reg) {
      reg.HZDepthTestLEGEOptimizationDisable = true;
         #endif
      #if GFX_VER == 12
      anv_batch_write_reg(&batch, GENX(FF_MODE2), reg) {
      /* On Alchemist, the FF_MODE2 docs for the GS timer say:
   *
   *    "The timer value must be set to 224."
   *
   * and Wa_16011163337 indicates this is the case for all Gfx12 parts,
   * and that this is necessary to avoid hanging the HS/DS units.  It
   * also clarifies that 224 is literally 0xE0 in the bits, not 7*32=224.
   *
   * The HS timer docs also have the same quote for Alchemist.  I am
   * unaware of a reason it needs to be set to 224 on Tigerlake, but
   * we do so for consistency if nothing else.
   *
   * For the TDS timer value, the docs say:
   *
   *    "For best performance, a value of 4 should be programmed."
   *
   * i915 also sets it this way on Tigerlake due to workarounds.
   *
   * The default VS timer appears to be 0, so we leave it at that.
   */
   reg.GSTimerValue  = 224;
   reg.HSTimerValue  = 224;
   reg.TDSTimerValue = 4;
         #endif
      #if INTEL_NEEDS_WA_1508744258
      /*    Disable RHWO by setting 0x7010[14] by default except during resolve
   *    pass.
   *
   * We implement global disabling of the optimization here and we toggle it
   * in anv_image_ccs_op().
   */
   anv_batch_write_reg(&batch, GENX(COMMON_SLICE_CHICKEN1), c1) {
      c1.RCCRHWOOptimizationDisable = true;
         #endif
      #if GFX_VERx10 < 125
   #define AA_LINE_QUALITY_REG GENX(3D_CHICKEN3)
   #else
   #define AA_LINE_QUALITY_REG GENX(CHICKEN_RASTER_1)
   #endif
         /* Enable the new line drawing algorithm that produces higher quality
   * lines.
   */
   anv_batch_write_reg(&batch, AA_LINE_QUALITY_REG, c3) {
      c3.AALineQualityFix = true;
         #endif
      #if GFX_VER == 12
      if (device->info->has_aux_map) {
      uint64_t aux_base_addr = intel_aux_map_get_base(device->aux_map_ctx);
   assert(aux_base_addr % (32 * 1024) == 0);
   anv_batch_emit(&batch, GENX(MI_LOAD_REGISTER_IMM), lri) {
      lri.RegisterOffset = GENX(GFX_AUX_TABLE_BASE_ADDR_num);
      }
   anv_batch_emit(&batch, GENX(MI_LOAD_REGISTER_IMM), lri) {
      lri.RegisterOffset = GENX(GFX_AUX_TABLE_BASE_ADDR_num) + 4;
            #endif
         /* Set the "CONSTANT_BUFFER Address Offset Disable" bit, so
   * 3DSTATE_CONSTANT_XS buffer 0 is an absolute address.
   *
   * This is only safe on kernels with context isolation support.
   */
   assert(device->physical->info.has_context_isolation);
   anv_batch_write_reg(&batch, GENX(CS_DEBUG_MODE2), csdm2) {
      csdm2.CONSTANT_BUFFERAddressOffsetDisable = true;
                        /* Because 3DSTATE_CPS::CoarsePixelShadingStateArrayPointer is relative to
   * the dynamic state base address we need to emit this instruction after
   * STATE_BASE_ADDRESS in init_common_queue_state().
      #if GFX_VER == 11
         #elif GFX_VER >= 12
      anv_batch_emit(&batch, GENX(3DSTATE_CPS_POINTERS), cps) {
      assert(device->cps_states.alloc_size != 0);
   /* Offset 0 is the disabled state */
   cps.CoarsePixelShadingStateArrayPointer =
         #endif
      #if GFX_VERx10 >= 125
      anv_batch_emit(&batch, GENX(STATE_COMPUTE_MODE), zero);
   anv_batch_emit(&batch, GENX(3DSTATE_MESH_CONTROL), zero);
   anv_batch_emit(&batch, GENX(3DSTATE_TASK_CONTROL), zero);
   genx_batch_emit_pipe_control_write(&batch, device->info, NoWrite,
                     genX(emit_pipeline_select)(&batch, GPGPU);
   anv_batch_emit(&batch, GENX(CFE_STATE), cfe) {
      cfe.MaximumNumberofThreads =
      }
   genx_batch_emit_pipe_control_write(&batch, device->info, NoWrite,
                        #endif
                              }
      static VkResult
   init_compute_queue_state(struct anv_queue *queue)
   {
      UNUSED const struct intel_device_info *devinfo = queue->device->info;
   uint32_t cmds[64];
   struct anv_batch batch = {
      .start = cmds,
   .next = cmds,
                     #if GFX_VER == 12
      if (queue->device->info->has_aux_map) {
      uint64_t aux_base_addr =
         assert(aux_base_addr % (32 * 1024) == 0);
   anv_batch_emit(&batch, GENX(MI_LOAD_REGISTER_IMM), lri) {
      lri.RegisterOffset = GENX(COMPCS0_AUX_TABLE_BASE_ADDR_num);
      }
   anv_batch_emit(&batch, GENX(MI_LOAD_REGISTER_IMM), lri) {
      lri.RegisterOffset = GENX(COMPCS0_AUX_TABLE_BASE_ADDR_num) + 4;
            #else
         #endif
         /* Wa_14015782607 - Issue pipe control with HDC_flush and
   * untyped cache flush set to 1 when CCS has NP state update with
   * STATE_COMPUTE_MODE.
   */
   if (intel_needs_workaround(devinfo, 14015782607) &&
      queue->family->engine_class == INTEL_ENGINE_CLASS_COMPUTE) {
   genx_batch_emit_pipe_control(&batch, devinfo,
                        #if GFX_VERx10 >= 125
      /* Wa_14014427904/22013045878 - We need additional invalidate/flush when
   * emitting NP state commands with ATS-M in compute mode.
   */
   if (intel_device_info_is_atsm(devinfo) &&
      queue->family->engine_class == INTEL_ENGINE_CLASS_COMPUTE) {
   genx_batch_emit_pipe_control
      (&batch, devinfo,
   ANV_PIPE_CS_STALL_BIT |
   ANV_PIPE_STATE_CACHE_INVALIDATE_BIT |
   ANV_PIPE_CONSTANT_CACHE_INVALIDATE_BIT |
   ANV_PIPE_UNTYPED_DATAPORT_CACHE_FLUSH_BIT |
   ANV_PIPE_TEXTURE_CACHE_INVALIDATE_BIT |
   ANV_PIPE_INSTRUCTION_CACHE_INVALIDATE_BIT |
            anv_batch_emit(&batch, GENX(STATE_COMPUTE_MODE), cm) {
      cm.PixelAsyncComputeThreadLimit = 4;
         #endif
               #if GFX_VERx10 >= 125
      anv_batch_emit(&batch, GENX(CFE_STATE), cfe) {
      cfe.MaximumNumberofThreads =
         #endif
                           return anv_queue_submit_simple_batch(queue, &batch,
      }
      void
   genX(init_physical_device_state)(ASSERTED struct anv_physical_device *pdevice)
   {
         #if GFX_VERx10 == 125 && ANV_SUPPORT_RT
      genX(grl_load_rt_uuid)(pdevice->rt_uuid);
      #endif
            }
      VkResult
   genX(init_device_state)(struct anv_device *device)
   {
               device->slice_hash = (struct anv_state) { 0 };
   for (uint32_t i = 0; i < device->queue_count; i++) {
      struct anv_queue *queue = &device->queues[i];
   switch (queue->family->engine_class) {
   case INTEL_ENGINE_CLASS_RENDER:
      res = init_render_queue_state(queue, false /* is_companion_rcs_batch */);
      case INTEL_ENGINE_CLASS_COMPUTE: {
      res = init_compute_queue_state(queue);
                  /**
   * Execute RCS init batch by default on the companion RCS command buffer in
   * order to support MSAA copy/clear operations on compute queue.
   */
   res = init_render_queue_state(queue, true /* is_companion_rcs_batch */);
      }
   case INTEL_ENGINE_CLASS_VIDEO:
      res = VK_SUCCESS;
      case INTEL_ENGINE_CLASS_COPY:
      /**
   * Execute RCS init batch by default on the companion RCS command buffer in
   * order to support MSAA copy/clear operations on copy queue.
   */
   res = init_render_queue_state(queue, true /* is_companion_rcs_batch */);
      default:
      res = vk_error(device, VK_ERROR_INITIALIZATION_FAILED);
      }
   if (res != VK_SUCCESS)
                  }
      #if GFX_VERx10 >= 125
   #define maybe_for_each_shading_rate_op(name) \
      for (VkFragmentShadingRateCombinerOpKHR name = VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR; \
      name <= VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MUL_KHR; \
   #elif GFX_VER >= 12
   #define maybe_for_each_shading_rate_op(name)
   #endif
      /* Rather than reemitting the CPS_STATE structure everything those changes and
   * for as many viewports as needed, we can just prepare all possible cases and
   * just pick the right offset from the prepacked states when needed.
   */
   void
   genX(init_cps_device_state)(struct anv_device *device)
   {
   #if GFX_VER >= 12
               /* Disabled CPS mode */
   for (uint32_t __v = 0; __v < MAX_VIEWPORTS; __v++) {
      /* ICL PRMs, Volume 2d: Command Reference: Structures: 3DSTATE_CPS_BODY:
   *
   *   "It is an INVALID configuration to set the CPS mode other than
   *    CPS_MODE_NONE and request per-sample dispatch in 3DSTATE_PS_EXTRA.
   *    Such configuration should be disallowed at the API level, and
   *    rendering results are undefined."
   *
   * Since we select this state when per coarse pixel is disabled and that
   * includes when per-sample dispatch is enabled, we need to ensure this
   * is set to NONE.
   */
   struct GENX(CPS_STATE) cps_state = {
                  GENX(CPS_STATE_pack)(NULL, cps_state_ptr, &cps_state);
               maybe_for_each_shading_rate_op(op0) {
      maybe_for_each_shading_rate_op(op1) {
      for (uint32_t x = 1; x <= 4; x *= 2) {
      for (uint32_t y = 1; y <= 4; y *= 2) {
      struct GENX(CPS_STATE) cps_state = {
      .CoarsePixelShadingMode = CPS_MODE_CONSTANT,
      #if GFX_VERx10 >= 125
                  static const uint32_t combiner_ops[] = {
      [VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR]    = PASSTHROUGH,
   [VK_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE_KHR] = OVERRIDE,
   [VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MIN_KHR]     = HIGH_QUALITY,
                  #endif /* GFX_VERx10 >= 125 */
                     for (uint32_t __v = 0; __v < MAX_VIEWPORTS; __v++) {
      GENX(CPS_STATE_pack)(NULL, cps_state_ptr, &cps_state);
                     #endif /* GFX_VER >= 12 */
   }
      void
   genX(emit_l3_config)(struct anv_batch *batch,
               {
            #if GFX_VER >= 12
   #define L3_ALLOCATION_REG GENX(L3ALLOC)
   #define L3_ALLOCATION_REG_num GENX(L3ALLOC_num)
   #else
   #define L3_ALLOCATION_REG GENX(L3CNTLREG)
   #define L3_ALLOCATION_REG_num GENX(L3CNTLREG_num)
   #endif
         anv_batch_write_reg(batch, L3_ALLOCATION_REG, l3cr) {
      #if GFX_VER >= 12
         #else
         #endif
         #if GFX_VER < 11
         #endif
   #if INTEL_NEEDS_WA_1406697149
            /* Wa_1406697149: Bit 9 "Error Detection Behavior Control" must be
   * set in L3CNTLREG register. The default setting of the bit is not
   * the desirable behavior.
   */
      #endif /* INTEL_NEEDS_WA_1406697149 */
            assert(cfg->n[INTEL_L3P_IS] == 0);
   assert(cfg->n[INTEL_L3P_C] == 0);
   assert(cfg->n[INTEL_L3P_T] == 0);
   l3cr.URBAllocation = cfg->n[INTEL_L3P_URB];
   l3cr.ROAllocation = cfg->n[INTEL_L3P_RO];
   l3cr.DCAllocation = cfg->n[INTEL_L3P_DC];
            }
      void
   genX(emit_sample_pattern)(struct anv_batch *batch,
         {
      assert(sl == NULL || sl->grid_size.width == 1);
            /* See the Vulkan 1.0 spec Table 24.1 "Standard sample locations" and
   * VkPhysicalDeviceFeatures::standardSampleLocations.
   */
   anv_batch_emit(batch, GENX(3DSTATE_SAMPLE_PATTERN), sp) {
      /* The Skylake PRM Vol. 2a "3DSTATE_SAMPLE_PATTERN" says:
   *
   *    "When programming the sample offsets (for NUMSAMPLES_4 or _8
   *    and MSRASTMODE_xxx_PATTERN), the order of the samples 0 to 3
   *    (or 7 for 8X, or 15 for 16X) must have monotonically increasing
   *    distance from the pixel center. This is required to get the
   *    correct centroid computation in the device."
   *
   * However, the Vulkan spec seems to require that the the samples occur
   * in the order provided through the API. The standard sample patterns
   * have the above property that they have monotonically increasing
   * distances from the center but client-provided ones do not. As long as
   * this only affects centroid calculations as the docs say, we should be
   * ok because OpenGL and Vulkan only require that the centroid be some
   * lit sample and that it's the same for all samples in a pixel; they
   * have no requirement that it be the one closest to center.
   */
   for (uint32_t i = 1; i <= 16; i *= 2) {
      switch (i) {
   case VK_SAMPLE_COUNT_1_BIT:
      if (sl && sl->per_pixel == i) {
         } else {
         }
      case VK_SAMPLE_COUNT_2_BIT:
      if (sl && sl->per_pixel == i) {
         } else {
         }
      case VK_SAMPLE_COUNT_4_BIT:
      if (sl && sl->per_pixel == i) {
         } else {
         }
      case VK_SAMPLE_COUNT_8_BIT:
      if (sl && sl->per_pixel == i) {
         } else {
         }
      case VK_SAMPLE_COUNT_16_BIT:
      if (sl && sl->per_pixel == i) {
         } else {
         }
      default:
                  }
      static uint32_t
   vk_to_intel_tex_filter(VkFilter filter, bool anisotropyEnable)
   {
      switch (filter) {
   default:
         case VK_FILTER_NEAREST:
         case VK_FILTER_LINEAR:
            }
      static uint32_t
   vk_to_intel_max_anisotropy(float ratio)
   {
         }
      static const uint32_t vk_to_intel_mipmap_mode[] = {
      [VK_SAMPLER_MIPMAP_MODE_NEAREST]          = MIPFILTER_NEAREST,
      };
      static const uint32_t vk_to_intel_tex_address[] = {
      [VK_SAMPLER_ADDRESS_MODE_REPEAT]          = TCM_WRAP,
   [VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT] = TCM_MIRROR,
   [VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE]   = TCM_CLAMP,
   [VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE] = TCM_MIRROR_ONCE,
      };
      /* Vulkan specifies the result of shadow comparisons as:
   *     1     if   ref <op> texel,
   *     0     otherwise.
   *
   * The hardware does:
   *     0     if texel <op> ref,
   *     1     otherwise.
   *
   * So, these look a bit strange because there's both a negation
   * and swapping of the arguments involved.
   */
   static const uint32_t vk_to_intel_shadow_compare_op[] = {
      [VK_COMPARE_OP_NEVER]                        = PREFILTEROP_ALWAYS,
   [VK_COMPARE_OP_LESS]                         = PREFILTEROP_LEQUAL,
   [VK_COMPARE_OP_EQUAL]                        = PREFILTEROP_NOTEQUAL,
   [VK_COMPARE_OP_LESS_OR_EQUAL]                = PREFILTEROP_LESS,
   [VK_COMPARE_OP_GREATER]                      = PREFILTEROP_GEQUAL,
   [VK_COMPARE_OP_NOT_EQUAL]                    = PREFILTEROP_EQUAL,
   [VK_COMPARE_OP_GREATER_OR_EQUAL]             = PREFILTEROP_GREATER,
      };
      static const uint32_t vk_to_intel_sampler_reduction_mode[] = {
      [VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE] = STD_FILTER,
   [VK_SAMPLER_REDUCTION_MODE_MIN]              = MINIMUM,
      };
      VkResult genX(CreateSampler)(
      VkDevice                                    _device,
   const VkSamplerCreateInfo*                  pCreateInfo,
   const VkAllocationCallbacks*                pAllocator,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
            sampler = vk_sampler_create(&device->vk, pCreateInfo,
         if (!sampler)
            const struct vk_format_ycbcr_info *ycbcr_info =
      sampler->vk.format != VK_FORMAT_UNDEFINED ?
                        uint32_t border_color_stride = 64;
   uint32_t border_color_offset;
   if (sampler->vk.border_color <= VK_BORDER_COLOR_INT_OPAQUE_WHITE) {
      border_color_offset = device->border_colors.offset +
            } else {
      assert(vk_border_color_is_custom(sampler->vk.border_color));
   sampler->custom_border_color =
                  union isl_color_value color = { .u32 = {
      sampler->vk.border_color_value.uint32[0],
   sampler->vk.border_color_value.uint32[1],
   sampler->vk.border_color_value.uint32[2],
               const struct anv_format *format_desc =
                  if (format_desc && format_desc->n_planes == 1 &&
                     assert(!isl_format_has_int_channel(fmt_plane->isl_format));
                           /* If we have bindless, allocate enough samplers.  We allocate 32 bytes
   * for each sampler instead of 16 bytes because we want all bindless
   * samplers to be 32-byte aligned so we don't have to use indirect
   * sampler messages on them.
   */
   sampler->bindless_state =
      anv_state_pool_alloc(&device->dynamic_state_pool,
         const bool seamless_cube =
            for (unsigned p = 0; p < sampler->n_planes; p++) {
      const bool plane_has_chroma =
         const VkFilter min_filter =
      plane_has_chroma ? sampler->vk.ycbcr_conversion->state.chroma_filter :
      const VkFilter mag_filter =
      plane_has_chroma ? sampler->vk.ycbcr_conversion->state.chroma_filter :
      const bool force_addr_rounding =
         const bool enable_min_filter_addr_rounding =
         const bool enable_mag_filter_addr_rounding =
         /* From Broadwell PRM, SAMPLER_STATE:
   *   "Mip Mode Filter must be set to MIPFILTER_NONE for Planar YUV surfaces."
   */
   enum isl_format plane0_isl_format = sampler->vk.ycbcr_conversion ?
      anv_get_format(sampler->vk.format)->planes[0].isl_format :
      const bool isl_format_is_planar_yuv =
      plane0_isl_format != ISL_FORMAT_UNSUPPORTED &&
               const uint32_t mip_filter_mode =
                  struct GENX(SAMPLER_STATE) sampler_state = {
            #if GFX_VER >= 11
         #endif
                        .MipModeFilter = mip_filter_mode,
   .MagModeFilter = vk_to_intel_tex_filter(mag_filter, pCreateInfo->anisotropyEnable),
   .MinModeFilter = vk_to_intel_tex_filter(min_filter, pCreateInfo->anisotropyEnable),
   .TextureLODBias = CLAMP(pCreateInfo->mipLodBias, -16, 15.996),
   .AnisotropicAlgorithm =
         .MinLOD = CLAMP(pCreateInfo->minLod, 0, 14),
   .MaxLOD = CLAMP(pCreateInfo->maxLod, 0, 14),
   .ChromaKeyEnable = 0,
   .ChromaKeyIndex = 0,
   .ChromaKeyMode = 0,
   .ShadowFunction =
      vk_to_intel_shadow_compare_op[pCreateInfo->compareEnable ?
                                 .MaximumAnisotropy = vk_to_intel_max_anisotropy(pCreateInfo->maxAnisotropy),
   .RAddressMinFilterRoundingEnable = enable_min_filter_addr_rounding,
   .RAddressMagFilterRoundingEnable = enable_mag_filter_addr_rounding,
   .VAddressMinFilterRoundingEnable = enable_min_filter_addr_rounding,
   .VAddressMagFilterRoundingEnable = enable_mag_filter_addr_rounding,
   .UAddressMinFilterRoundingEnable = enable_min_filter_addr_rounding,
   .UAddressMagFilterRoundingEnable = enable_mag_filter_addr_rounding,
   .TrilinearFilterQuality = 0,
   .NonnormalizedCoordinateEnable = pCreateInfo->unnormalizedCoordinates,
   .TCXAddressControlMode = vk_to_intel_tex_address[pCreateInfo->addressModeU],
                  .ReductionType =
         .ReductionTypeEnable =
                        if (sampler->bindless_state.map) {
      memcpy(sampler->bindless_state.map + p * 32,
                              }
      /* Wa_14015814527
   *
   * Check if task shader was utilized within cmd_buffer, if so
   * commit empty URB states and null prim.
   */
   void
   genX(apply_task_urb_workaround)(struct anv_cmd_buffer *cmd_buffer)
   {
   #if GFX_VERx10 >= 125
               if (!intel_needs_workaround(devinfo, 16014390852))
            if (cmd_buffer->state.current_pipeline != _3D ||
      !cmd_buffer->state.gfx.used_task_shader)
                  /* Wa_14015821291 mentions that WA below is not required if we have
   * a pipeline flush going on. It will get flushed during
   * cmd_buffer_flush_state before draw.
   */
   if ((cmd_buffer->state.pending_pipe_bits & ANV_PIPE_CS_STALL_BIT))
            for (int i = 0; i <= MESA_SHADER_GEOMETRY; i++) {
      anv_batch_emit(&cmd_buffer->batch, GENX(3DSTATE_URB_VS), urb) {
                     anv_batch_emit(&cmd_buffer->batch, GENX(3DSTATE_URB_ALLOC_MESH), zero);
            /* Issue 'nullprim' to commit the state. */
   genx_batch_emit_pipe_control_write
      (&cmd_buffer->batch, cmd_buffer->device->info,
   #endif
   }
