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
      #include <assert.h>
   #include <stdbool.h>
      #include "anv_private.h"
   #include "anv_measure.h"
   #include "vk_render_pass.h"
   #include "vk_util.h"
      #include "common/intel_aux_map.h"
   #include "genxml/gen_macros.h"
   #include "genxml/genX_pack.h"
   #include "genxml/genX_rt_pack.h"
   #include "common/intel_genX_state.h"
      #include "ds/intel_tracepoints.h"
      /* We reserve :
   *    - GPR 14 for secondary command buffer returns
   *    - GPR 15 for conditional rendering
   */
   #define MI_BUILDER_NUM_ALLOC_GPRS 14
   #define __gen_get_batch_dwords anv_batch_emit_dwords
   #define __gen_address_offset anv_address_add
   #define __gen_get_batch_address(b, a) anv_batch_address(b, a)
   #include "common/mi_builder.h"
      #include "genX_cmd_draw_helpers.h"
      static void genX(flush_pipeline_select)(struct anv_cmd_buffer *cmd_buffer,
            static enum anv_pipe_bits
   convert_pc_to_bits(struct GENX(PIPE_CONTROL) *pc) {
      enum anv_pipe_bits bits = 0;
   bits |= (pc->DepthCacheFlushEnable) ?  ANV_PIPE_DEPTH_CACHE_FLUSH_BIT : 0;
      #if GFX_VERx10 >= 125
         #endif
   #if GFX_VER >= 12
      bits |= (pc->TileCacheFlushEnable) ?  ANV_PIPE_TILE_CACHE_FLUSH_BIT : 0;
      #endif
      bits |= (pc->RenderTargetCacheFlushEnable) ?  ANV_PIPE_RENDER_TARGET_CACHE_FLUSH_BIT : 0;
   bits |= (pc->VFCacheInvalidationEnable) ?  ANV_PIPE_VF_CACHE_INVALIDATE_BIT : 0;
   bits |= (pc->StateCacheInvalidationEnable) ?  ANV_PIPE_STATE_CACHE_INVALIDATE_BIT : 0;
   bits |= (pc->ConstantCacheInvalidationEnable) ?  ANV_PIPE_CONSTANT_CACHE_INVALIDATE_BIT : 0;
   bits |= (pc->TextureCacheInvalidationEnable) ?  ANV_PIPE_TEXTURE_CACHE_INVALIDATE_BIT : 0;
   bits |= (pc->InstructionCacheInvalidateEnable) ?  ANV_PIPE_INSTRUCTION_CACHE_INVALIDATE_BIT : 0;
   bits |= (pc->StallAtPixelScoreboard) ?  ANV_PIPE_STALL_AT_SCOREBOARD_BIT : 0;
   bits |= (pc->DepthStallEnable) ?  ANV_PIPE_DEPTH_STALL_BIT : 0;
      #if GFX_VERx10 == 125
      bits |= (pc->UntypedDataPortCacheFlushEnable) ? ANV_PIPE_UNTYPED_DATAPORT_CACHE_FLUSH_BIT : 0;
      #endif
         }
      #define anv_debug_dump_pc(pc, reason) \
      if (INTEL_DEBUG(DEBUG_PIPE_CONTROL)) { \
      fputs("pc: emit PC=( ", stdout); \
   anv_dump_pipe_bits(convert_pc_to_bits(&(pc)), stdout);   \
            ALWAYS_INLINE static void
   genX(emit_dummy_post_sync_op)(struct anv_cmd_buffer *cmd_buffer,
         {
      genX(batch_emit_dummy_post_sync_op)(&cmd_buffer->batch, cmd_buffer->device,
            }
      void
   genX(cmd_buffer_emit_state_base_address)(struct anv_cmd_buffer *cmd_buffer)
   {
      struct anv_device *device = cmd_buffer->device;
            /* If we are emitting a new state base address we probably need to re-emit
   * binding tables.
   */
         #if GFX_VERx10 >= 125
      genx_batch_emit_pipe_control(&cmd_buffer->batch,
               anv_batch_emit(
      &cmd_buffer->batch, GENX(3DSTATE_BINDING_TABLE_POOL_ALLOC), btpa) {
   btpa.BindingTablePoolBaseAddress =
         btpa.BindingTablePoolBufferSize = device->physical->va.binding_table_pool.size / 4096;
         #else /* GFX_VERx10 < 125 */
      /* Emit a render target cache flush.
   *
   * This isn't documented anywhere in the PRM.  However, it seems to be
   * necessary prior to changing the surface state base address.  Without
   * this, we get GPU hangs when using multi-level command buffers which
   * clear depth, reset state base address, and then go render stuff.
   */
   genx_batch_emit_pipe_control
      #if GFX_VER >= 12
         #else
         #endif
         ANV_PIPE_RENDER_TARGET_CACHE_FLUSH_BIT |
      #if INTEL_NEEDS_WA_1607854226
      /* Wa_1607854226:
   *
   *  Workaround the non pipelined state not applying in MEDIA/GPGPU pipeline
   *  mode by putting the pipeline temporarily in 3D mode.
   */
   uint32_t gfx12_wa_pipeline = cmd_buffer->state.current_pipeline;
      #endif
         anv_batch_emit(&cmd_buffer->batch, GENX(STATE_BASE_ADDRESS), sba) {
      sba.GeneralStateBaseAddress = (struct anv_address) { NULL, 0 };
   sba.GeneralStateMOCS = mocs;
                     sba.SurfaceStateBaseAddress =
         sba.SurfaceStateMOCS = mocs;
            sba.DynamicStateBaseAddress =
         sba.DynamicStateMOCS = mocs;
            sba.IndirectObjectBaseAddress = (struct anv_address) { NULL, 0 };
   sba.IndirectObjectMOCS = mocs;
            sba.InstructionBaseAddress =
         sba.InstructionMOCS = mocs;
            sba.GeneralStateBufferSize       = 0xfffff;
   sba.IndirectObjectBufferSize     = 0xfffff;
   sba.DynamicStateBufferSize       = device->physical->va.dynamic_state_pool.size / 4096;
   sba.InstructionBufferSize        = device->physical->va.instruction_state_pool.size / 4096;
   sba.GeneralStateBufferSizeModifyEnable    = true;
   sba.IndirectObjectBufferSizeModifyEnable  = true;
   sba.DynamicStateBufferSizeModifyEnable    = true;
            #if GFX_VERx10 >= 125
            /* Bindless Surface State & Bindless Sampler State are aligned to the
   * same heap
   */
   sba.BindlessSurfaceStateBaseAddress =
      sba.BindlessSamplerStateBaseAddress =
   (struct anv_address) { .offset =
      sba.BindlessSurfaceStateSize =
      (device->physical->va.binding_table_pool.size +
   device->physical->va.internal_surface_state_pool.size +
      sba.BindlessSamplerStateBufferSize =
      (device->physical->va.binding_table_pool.size +
   device->physical->va.internal_surface_state_pool.size +
      sba.BindlessSurfaceStateMOCS = sba.BindlessSamplerStateMOCS = mocs;
      #else
         #endif
         } else {
      sba.BindlessSurfaceStateBaseAddress =
      (struct anv_address) { .offset =
      };
   sba.BindlessSurfaceStateSize =
            #if GFX_VER >= 11
            sba.BindlessSamplerStateBaseAddress = (struct anv_address) { NULL, 0 };
   sba.BindlessSamplerStateMOCS = mocs;
      #endif
            #if GFX_VERx10 >= 125
         #endif
            #if INTEL_NEEDS_WA_1607854226
      /* Wa_1607854226:
   *
   *  Put the pipeline back into its current mode.
   */
   if (gfx12_wa_pipeline != UINT32_MAX)
      #endif
      #endif /* GFX_VERx10 < 125 */
         /* After re-setting the surface state base address, we have to do some
   * cache flushing so that the sampler engine will pick up the new
   * SURFACE_STATE objects and binding tables. From the Broadwell PRM,
   * Shared Function > 3D Sampler > State > State Caching (page 96):
   *
   *    Coherency with system memory in the state cache, like the texture
   *    cache is handled partially by software. It is expected that the
   *    command stream or shader will issue Cache Flush operation or
   *    Cache_Flush sampler message to ensure that the L1 cache remains
   *    coherent with system memory.
   *
   *    [...]
   *
   *    Whenever the value of the Dynamic_State_Base_Addr,
   *    Surface_State_Base_Addr are altered, the L1 state cache must be
   *    invalidated to ensure the new surface or sampler state is fetched
   *    from system memory.
   *
   * The PIPE_CONTROL command has a "State Cache Invalidation Enable" bit
   * which, according the PIPE_CONTROL instruction documentation in the
   * Broadwell PRM:
   *
   *    Setting this bit is independent of any other bit in this packet.
   *    This bit controls the invalidation of the L1 and L2 state caches
   *    at the top of the pipe i.e. at the parsing time.
   *
   * Unfortunately, experimentation seems to indicate that state cache
   * invalidation through a PIPE_CONTROL does nothing whatsoever in
   * regards to surface state and binding tables.  In stead, it seems that
   * invalidating the texture cache is what is actually needed.
   *
   * XXX:  As far as we have been able to determine through
   * experimentation, shows that flush the texture cache appears to be
   * sufficient.  The theory here is that all of the sampling/rendering
   * units cache the binding table in the texture cache.  However, we have
   * yet to be able to actually confirm this.
   *
   * Wa_14013910100:
   *
   *  "DG2 128/256/512-A/B: S/W must program STATE_BASE_ADDRESS command twice
   *   or program pipe control with Instruction cache invalidate post
   *   STATE_BASE_ADDRESS command"
   */
   enum anv_pipe_bits bits =
      ANV_PIPE_TEXTURE_CACHE_INVALIDATE_BIT |
   #if GFX_VERx10 == 125
         #endif
            #if GFX_VER >= 9 && GFX_VER <= 11
         /* From the SKL PRM, Vol. 2a, "PIPE_CONTROL",
   *
   *    "Workaround : “CS Stall” bit in PIPE_CONTROL command must be
   *     always set for GPGPU workloads when “Texture Cache Invalidation
   *     Enable” bit is set".
   *
   * Workaround stopped appearing in TGL PRMs.
   */
   if (cmd_buffer->state.current_pipeline == GPGPU)
   #endif
      genx_batch_emit_pipe_control(&cmd_buffer->batch, cmd_buffer->device->info,
      }
      static void
   add_surface_reloc(struct anv_cmd_buffer *cmd_buffer,
         {
      VkResult result = anv_reloc_list_add_bo(&cmd_buffer->surface_relocs,
            if (unlikely(result != VK_SUCCESS))
      }
      static void
   add_surface_state_relocs(struct anv_cmd_buffer *cmd_buffer,
         {
      assert(!anv_address_is_null(state->address));
            if (!anv_address_is_null(state->aux_address)) {
      VkResult result =
      anv_reloc_list_add_bo(&cmd_buffer->surface_relocs,
      if (result != VK_SUCCESS)
               if (!anv_address_is_null(state->clear_address)) {
      VkResult result =
      anv_reloc_list_add_bo(&cmd_buffer->surface_relocs,
      if (result != VK_SUCCESS)
         }
      /* Transitions a HiZ-enabled depth buffer from one layout to another. Unless
   * the initial layout is undefined, the HiZ buffer and depth buffer will
   * represent the same data at the end of this operation.
   */
   static void
   transition_depth_buffer(struct anv_cmd_buffer *cmd_buffer,
                           const struct anv_image *image,
   {
      const uint32_t depth_plane =
         if (image->planes[depth_plane].aux_usage == ISL_AUX_USAGE_NONE)
            /* If will_full_fast_clear is set, the caller promises to fast-clear the
   * largest portion of the specified range as it can.  For depth images,
   * that means the entire image because we don't support multi-LOD HiZ.
   */
   assert(image->planes[0].primary_surface.isl.levels == 1);
   if (will_full_fast_clear)
            const enum isl_aux_state initial_state =
      anv_layout_to_aux_state(cmd_buffer->device->info, image,
                  const enum isl_aux_state final_state =
      anv_layout_to_aux_state(cmd_buffer->device->info, image,
                     const bool initial_depth_valid =
         const bool initial_hiz_valid =
         const bool final_needs_depth =
         const bool final_needs_hiz =
            /* Getting into the pass-through state for Depth is tricky and involves
   * both a resolve and an ambiguate.  We don't handle that state right now
   * as anv_layout_to_aux_state never returns it.
   */
            if (final_needs_depth && !initial_depth_valid) {
      assert(initial_hiz_valid);
   anv_image_hiz_op(cmd_buffer, image, VK_IMAGE_ASPECT_DEPTH_BIT,
      } else if (final_needs_hiz && !initial_hiz_valid) {
      assert(initial_depth_valid);
   anv_image_hiz_op(cmd_buffer, image, VK_IMAGE_ASPECT_DEPTH_BIT,
               /* Additional tile cache flush for MTL:
   *
   * https://gitlab.freedesktop.org/mesa/mesa/-/issues/10420
   * https://gitlab.freedesktop.org/mesa/mesa/-/issues/10530
   */
   if (intel_device_info_is_mtl(cmd_buffer->device->info) &&
      image->planes[depth_plane].aux_usage == ISL_AUX_USAGE_HIZ_CCS &&
   final_needs_depth && !initial_depth_valid) {
   anv_add_pending_pipe_bits(cmd_buffer,
               }
      /* Transitions a HiZ-enabled depth buffer from one layout to another. Unless
   * the initial layout is undefined, the HiZ buffer and depth buffer will
   * represent the same data at the end of this operation.
   */
   static void
   transition_stencil_buffer(struct anv_cmd_buffer *cmd_buffer,
                           const struct anv_image *image,
   uint32_t base_level, uint32_t level_count,
   {
   #if GFX_VER == 12
      const uint32_t plane =
         if (image->planes[plane].aux_usage == ISL_AUX_USAGE_NONE)
            if ((initial_layout == VK_IMAGE_LAYOUT_UNDEFINED ||
      initial_layout == VK_IMAGE_LAYOUT_PREINITIALIZED) &&
   cmd_buffer->device->info->has_aux_map) {
   /* If will_full_fast_clear is set, the caller promises to fast-clear the
   * largest portion of the specified range as it can.
   */
   if (will_full_fast_clear)
            for (uint32_t l = 0; l < level_count; l++) {
      const uint32_t level = base_level + l;
   const VkRect2D clear_rect = {
      .offset.x = 0,
   .offset.y = 0,
   .extent.width = u_minify(image->vk.extent.width, level),
               uint32_t aux_layers =
                        /* From Bspec's 3DSTATE_STENCIL_BUFFER_BODY > Stencil Compression
   * Enable:
   *
   *    "When enabled, Stencil Buffer needs to be initialized via
   *    stencil clear (HZ_OP) before any renderpass."
   */
   anv_image_hiz_clear(cmd_buffer, image, VK_IMAGE_ASPECT_STENCIL_BIT,
                        /* Additional tile cache flush for MTL:
   *
   * https://gitlab.freedesktop.org/mesa/mesa/-/issues/10420
   * https://gitlab.freedesktop.org/mesa/mesa/-/issues/10530
   */
   if (intel_device_info_is_mtl(cmd_buffer->device->info)) {
      anv_add_pending_pipe_bits(cmd_buffer,
               #endif
   }
      #define MI_PREDICATE_SRC0    0x2400
   #define MI_PREDICATE_SRC1    0x2408
   #define MI_PREDICATE_RESULT  0x2418
      static void
   set_image_compressed_bit(struct anv_cmd_buffer *cmd_buffer,
                           const struct anv_image *image,
   {
               /* We only have compression tracking for CCS_E */
   if (!isl_aux_usage_has_ccs_e(image->planes[plane].aux_usage))
            for (uint32_t a = 0; a < layer_count; a++) {
      uint32_t layer = base_layer + a;
   anv_batch_emit(&cmd_buffer->batch, GENX(MI_STORE_DATA_IMM), sdi) {
      sdi.Address = anv_image_get_compression_state_addr(cmd_buffer->device,
                              /* FCV_CCS_E images are automatically fast cleared to default value at
   * render time. In order to account for this, anv should set the the
   * appropriate fast clear state for level0/layer0.
   *
   * At the moment, tracking the fast clear state for higher levels/layers is
   * neither supported, nor do we enter a situation where it is a concern.
   */
   if (image->planes[plane].aux_usage == ISL_AUX_USAGE_FCV_CCS_E &&
      base_layer == 0 && level == 0) {
   anv_batch_emit(&cmd_buffer->batch, GENX(MI_STORE_DATA_IMM), sdi) {
      sdi.Address = anv_image_get_fast_clear_type_addr(cmd_buffer->device,
                  }
      static void
   set_image_fast_clear_state(struct anv_cmd_buffer *cmd_buffer,
                     {
      anv_batch_emit(&cmd_buffer->batch, GENX(MI_STORE_DATA_IMM), sdi) {
      sdi.Address = anv_image_get_fast_clear_type_addr(cmd_buffer->device,
                     /* Whenever we have fast-clear, we consider that slice to be compressed.
   * This makes building predicates much easier.
   */
   if (fast_clear != ANV_FAST_CLEAR_NONE)
      }
      /* This is only really practical on haswell and above because it requires
   * MI math in order to get it correct.
   */
   static void
   anv_cmd_compute_resolve_predicate(struct anv_cmd_buffer *cmd_buffer,
                                 {
      struct anv_address addr = anv_image_get_fast_clear_type_addr(cmd_buffer->device,
         struct mi_builder b;
   mi_builder_init(&b, cmd_buffer->device->info, &cmd_buffer->batch);
   const uint32_t mocs = anv_mocs_for_address(cmd_buffer->device, &addr);
                     if (resolve_op == ISL_AUX_OP_FULL_RESOLVE) {
      /* In this case, we're doing a full resolve which means we want the
   * resolve to happen if any compression (including fast-clears) is
   * present.
   *
   * In order to simplify the logic a bit, we make the assumption that,
   * if the first slice has been fast-cleared, it is also marked as
   * compressed.  See also set_image_fast_clear_state.
   */
   const struct mi_value compression_state =
      mi_mem32(anv_image_get_compression_state_addr(cmd_buffer->device,
            mi_store(&b, mi_reg64(MI_PREDICATE_SRC0), compression_state);
            if (level == 0 && array_layer == 0) {
      /* If the predicate is true, we want to write 0 to the fast clear type
   * and, if it's false, leave it alone.  We can do this by writing
   *
   * clear_type = clear_type & ~predicate;
   */
   struct mi_value new_fast_clear_type =
      mi_iand(&b, fast_clear_type,
            } else if (level == 0 && array_layer == 0) {
      /* In this case, we are doing a partial resolve to get rid of fast-clear
   * colors.  We don't care about the compression state but we do care
   * about how much fast clear is allowed by the final layout.
   */
   assert(resolve_op == ISL_AUX_OP_PARTIAL_RESOLVE);
            /* We need to compute (fast_clear_supported < image->fast_clear) */
   struct mi_value pred =
                  /* If the predicate is true, we want to write 0 to the fast clear type
   * and, if it's false, leave it alone.  We can do this by writing
   *
   * clear_type = clear_type & ~predicate;
   */
   struct mi_value new_fast_clear_type =
            } else {
      /* In this case, we're trying to do a partial resolve on a slice that
   * doesn't have clear color.  There's nothing to do.
   */
   assert(resolve_op == ISL_AUX_OP_PARTIAL_RESOLVE);
               /* Set src1 to 0 and use a != condition */
            anv_batch_emit(&cmd_buffer->batch, GENX(MI_PREDICATE), mip) {
      mip.LoadOperation    = LOAD_LOADINV;
   mip.CombineOperation = COMBINE_SET;
         }
      static void
   anv_cmd_predicated_ccs_resolve(struct anv_cmd_buffer *cmd_buffer,
                                 const struct anv_image *image,
   enum isl_format format,
   {
               anv_cmd_compute_resolve_predicate(cmd_buffer, image,
                  /* CCS_D only supports full resolves and BLORP will assert on us if we try
   * to do a partial resolve on a CCS_D surface.
   */
   if (resolve_op == ISL_AUX_OP_PARTIAL_RESOLVE &&
      image->planes[plane].aux_usage == ISL_AUX_USAGE_CCS_D)
         anv_image_ccs_op(cmd_buffer, image, format, swizzle, aspect,
      }
      static void
   anv_cmd_predicated_mcs_resolve(struct anv_cmd_buffer *cmd_buffer,
                                 const struct anv_image *image,
   enum isl_format format,
   {
      assert(aspect == VK_IMAGE_ASPECT_COLOR_BIT);
            anv_cmd_compute_resolve_predicate(cmd_buffer, image,
                  anv_image_mcs_op(cmd_buffer, image, format, swizzle, aspect,
      }
      void
   genX(cmd_buffer_mark_image_written)(struct anv_cmd_buffer *cmd_buffer,
                                       {
      /* The aspect must be exactly one of the image aspects. */
            /* Filter out aux usages that don't have any compression tracking.
   * Note: We only have compression tracking for CCS_E images, but it's
   * possible for a CCS_E enabled image to have a subresource with a different
   * aux usage.
   */
   if (!isl_aux_usage_has_compression(aux_usage))
            set_image_compressed_bit(cmd_buffer, image, aspect,
      }
      static void
   init_fast_clear_color(struct anv_cmd_buffer *cmd_buffer,
               {
      assert(cmd_buffer && image);
            /* Initialize the struct fields that are accessed for fast clears so that
   * the HW restrictions on the field values are satisfied.
   *
   * On generations that do not support indirect clear color natively, we
   * can just skip initializing the values, because they will be set by
   * BLORP before actually doing the fast clear.
   *
   * For newer generations, we may not be able to skip initialization.
   * Testing shows that writing to CLEAR_COLOR causes corruption if
   * the surface is currently being used. So, care must be taken here.
   * There are two cases that we consider:
   *
   *    1. For CCS_E without FCV, we can skip initializing the color-related
   *       fields, just like on the older platforms. Also, DWORDS 6 and 7
   *       are marked MBZ (or have a usable field on gfx11), but we can skip
   *       initializing them because in practice these fields need other
   *       state to be programmed for their values to matter.
   *
   *    2. When the FCV optimization is enabled, we must initialize the
   *       color-related fields. Otherwise, the engine might reference their
   *       uninitialized contents before we fill them for a manual fast clear
   *       with BLORP. Although the surface may be in use, no synchronization
   *       is needed before initialization. The only possible clear color we
   *       support in this mode is 0.
      #if GFX_VER == 12
               if (image->planes[plane].aux_usage == ISL_AUX_USAGE_FCV_CCS_E) {
      assert(!image->planes[plane].can_non_zero_fast_clear);
            unsigned num_dwords = 6;
   struct anv_address addr =
            for (unsigned i = 0; i < num_dwords; i++) {
      anv_batch_emit(&cmd_buffer->batch, GENX(MI_STORE_DATA_IMM), sdi) {
      sdi.Address = addr;
   sdi.Address.offset += i * 4;
   sdi.ImmediateData = 0;
               #endif
   }
      /* Copy the fast-clear value dword(s) between a surface state object and an
   * image's fast clear state buffer.
   */
   void
   genX(load_image_clear_color)(struct anv_cmd_buffer *cmd_buffer,
               {
   #if GFX_VER < 10
      assert(cmd_buffer && image);
            struct anv_address ss_clear_addr =
      anv_state_pool_state_address(
      &cmd_buffer->device->internal_surface_state_pool,
   (struct anv_state) {
      .offset = surface_state.offset +
      const struct anv_address entry_addr =
      anv_image_get_clear_color_addr(cmd_buffer->device, image,
               struct mi_builder b;
                     /* Updating a surface state object may require that the state cache be
   * invalidated. From the SKL PRM, Shared Functions -> State -> State
   * Caching:
   *
   *    Whenever the RENDER_SURFACE_STATE object in memory pointed to by
   *    the Binding Table Pointer (BTP) and Binding Table Index (BTI) is
   *    modified [...], the L1 state cache must be invalidated to ensure
   *    the new surface or sampler state is fetched from system memory.
   *
   * In testing, SKL doesn't actually seem to need this, but HSW does.
   */
   anv_add_pending_pipe_bits(cmd_buffer,
            #endif
   }
      void
   genX(set_fast_clear_state)(struct anv_cmd_buffer *cmd_buffer,
                     {
      if (isl_color_value_is_zero(clear_color, format)) {
      /* This image has the auxiliary buffer enabled. We can mark the
   * subresource as not needing a resolve because the clear color
   * will match what's in every RENDER_SURFACE_STATE object when
   * it's being used for sampling.
   */
   set_image_fast_clear_state(cmd_buffer, image,
            } else {
      set_image_fast_clear_state(cmd_buffer, image,
               }
      /**
   * @brief Transitions a color buffer from one layout to another.
   *
   * See section 6.1.1. Image Layout Transitions of the Vulkan 1.0.50 spec for
   * more information.
   *
   * @param level_count VK_REMAINING_MIP_LEVELS isn't supported.
   * @param layer_count VK_REMAINING_ARRAY_LAYERS isn't supported. For 3D images,
   *                    this represents the maximum layers to transition at each
   *                    specified miplevel.
   */
   static void
   transition_color_buffer(struct anv_cmd_buffer *cmd_buffer,
                           const struct anv_image *image,
   VkImageAspectFlagBits aspect,
   const uint32_t base_level, uint32_t level_count,
   uint32_t base_layer, uint32_t layer_count,
   VkImageLayout initial_layout,
   {
      struct anv_device *device = cmd_buffer->device;
   const struct intel_device_info *devinfo = device->info;
   /* Validate the inputs. */
   assert(cmd_buffer);
   assert(image && image->vk.aspects & VK_IMAGE_ASPECT_ANY_COLOR_BIT_ANV);
   /* These values aren't supported for simplicity's sake. */
   assert(level_count != VK_REMAINING_MIP_LEVELS &&
         /* Ensure the subresource range is valid. */
   UNUSED uint64_t last_level_num = base_level + level_count;
   const uint32_t max_depth = u_minify(image->vk.extent.depth, base_level);
   UNUSED const uint32_t image_layers = MAX2(image->vk.array_layers, max_depth);
   assert((uint64_t)base_layer + layer_count  <= image_layers);
   assert(last_level_num <= image->vk.mip_levels);
   /* If there is a layout transfer, the final layout cannot be undefined or
   * preinitialized (VUID-VkImageMemoryBarrier-newLayout-01198).
   */
   assert(initial_layout == final_layout ||
         (final_layout != VK_IMAGE_LAYOUT_UNDEFINED &&
   const struct isl_drm_modifier_info *isl_mod_info =
      image->vk.tiling == VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT
   ? isl_drm_modifier_get_info(image->vk.drm_format_mod)
         const bool src_queue_external =
      src_queue_family == VK_QUEUE_FAMILY_FOREIGN_EXT ||
         const bool dst_queue_external =
      dst_queue_family == VK_QUEUE_FAMILY_FOREIGN_EXT ||
         /* If the queues are external, consider the first queue family flags
   * (should be the most capable)
   */
   const VkQueueFlagBits src_queue_flags =
      device->physical->queue.families[
      (src_queue_external || src_queue_family == VK_QUEUE_FAMILY_IGNORED) ?
   const VkQueueFlagBits dst_queue_flags =
      device->physical->queue.families[
               /* Simultaneous acquire and release on external queues is illegal. */
            /* Ownership transition on an external queue requires special action if the
   * image has a DRM format modifier because we store image data in
   * a driver-private bo which is inaccessible to the external queue.
   */
   const bool private_binding_acquire =
      src_queue_external &&
   anv_image_is_externally_shared(image) &&
         const bool private_binding_release =
      dst_queue_external &&
   anv_image_is_externally_shared(image) &&
         if (initial_layout == final_layout &&
      !private_binding_acquire && !private_binding_release) {
   /* No work is needed. */
               /**
   * Section 7.7.4 of the Vulkan 1.3.260 spec says:
   *
   *    If the transfer is via an image memory barrier, and an image layout
   *    transition is desired, then the values of oldLayout and newLayout in the
   *    release operation's memory barrier must be equal to values of oldLayout
   *    and newLayout in the acquire operation's memory barrier. Although the
   *    image layout transition is submitted twice, it will only be executed
   *    once. A layout transition specified in this way happens-after the
   *    release operation and happens-before the acquire operation.
   *
   * Because we know that we get match transition on each queue, we choose to
   * only do the work on one queue type : RENDER. In the cases where we do
   * transitions between COMPUTE & TRANSFER, we should have matching
   * aux/fast_clear value which would trigger no work in the code below.
   */
   if (!(src_queue_external || dst_queue_external) &&
      src_queue_family != VK_QUEUE_FAMILY_IGNORED &&
   dst_queue_family != VK_QUEUE_FAMILY_IGNORED &&
   src_queue_family != dst_queue_family) {
   enum intel_engine_class src_engine =
         if (src_engine != INTEL_ENGINE_CLASS_RENDER)
                        if (base_layer >= anv_image_aux_layers(image, aspect, base_level))
            enum isl_aux_usage initial_aux_usage =
      anv_layout_to_aux_usage(devinfo, image, aspect, 0,
      enum isl_aux_usage final_aux_usage =
      anv_layout_to_aux_usage(devinfo, image, aspect, 0,
      enum anv_fast_clear_type initial_fast_clear =
      anv_layout_to_fast_clear_type(devinfo, image, aspect, initial_layout,
      enum anv_fast_clear_type final_fast_clear =
      anv_layout_to_fast_clear_type(devinfo, image, aspect, final_layout,
         /* We must override the anv_layout_to_* functions because they are unaware
   * of acquire/release direction.
   */
   if (private_binding_acquire) {
      initial_aux_usage = isl_drm_modifier_has_aux(isl_mod_info->modifier) ?
         initial_fast_clear = isl_mod_info->supports_clear_color ?
      } else if (private_binding_release) {
      final_aux_usage = isl_drm_modifier_has_aux(isl_mod_info->modifier) ?
         final_fast_clear = isl_mod_info->supports_clear_color ?
                        /* The following layouts are equivalent for non-linear images. */
   const bool initial_layout_undefined =
      initial_layout == VK_IMAGE_LAYOUT_UNDEFINED ||
         bool must_init_fast_clear_state = false;
            if (initial_layout_undefined) {
      /* The subresource may have been aliased and populated with arbitrary
   * data.
   */
            if (image->planes[plane].aux_usage == ISL_AUX_USAGE_MCS ||
                     } else {
               /* We can start using the CCS immediately without ambiguating. The
   * two conditions that enable this are:
   *
   * 1) The device treats all possible CCS values as legal. In other
   *    words, we can't confuse the hardware with random bits in the
   *    CCS.
   *
   * 2) We enable compression on all writable image layouts. The CCS
   *    will receive all writes and will therefore always be in sync
   *    with the main surface.
   *
   *    If we were to disable compression on some writable layouts, the
   *    CCS could get out of sync with the main surface and the app
   *    could lose the data it wrote previously. For example, this
   *    could happen if an app: transitions from UNDEFINED w/o
   *    ambiguating -> renders with AUX_NONE -> samples with AUX_CCS.
   *
   * The second condition is asserted below, but could be moved
   * elsewhere for more coverage (we're only checking transitions from
   * an undefined layout).
   */
                           } else if (private_binding_acquire) {
      /* The fast clear state lives in a driver-private bo, and therefore the
   * external/foreign queue is unaware of it.
   *
   * If this is the first time we are accessing the image, then the fast
   * clear state is uninitialized.
   *
   * If this is NOT the first time we are accessing the image, then the fast
   * clear state may still be valid and correct due to the resolve during
   * our most recent ownership release.  However, we do not track the aux
   * state with MI stores, and therefore must assume the worst-case: that
   * this is the first time we are accessing the image.
   */
   assert(image->planes[plane].fast_clear_memory_range.binding ==
                  if (anv_image_get_aux_memory_range(image, plane)->binding ==
      ANV_IMAGE_MEMORY_BINDING_PRIVATE) {
   /* The aux surface, like the fast clear state, lives in
   * a driver-private bo.  We must initialize the aux surface for the
   * same reasons we must initialize the fast clear state.
   */
      } else {
      /* The aux surface, unlike the fast clear state, lives in
   * application-visible VkDeviceMemory and is shared with the
   * external/foreign queue. Therefore, when we acquire ownership of the
   * image with a defined VkImageLayout, the aux surface is valid and has
   * the aux state required by the modifier.
   */
                  if (must_init_fast_clear_state) {
      if (base_level == 0 && base_layer == 0) {
      set_image_fast_clear_state(cmd_buffer, image, aspect,
      }
               if (must_init_aux_surface) {
               /* Initialize the aux buffers to enable correct rendering.  In order to
   * ensure that things such as storage images work correctly, aux buffers
   * need to be initialized to valid data.
   *
   * Having an aux buffer with invalid data is a problem for two reasons:
   *
   *  1) Having an invalid value in the buffer can confuse the hardware.
   *     For instance, with CCS_E on SKL, a two-bit CCS value of 2 is
   *     invalid and leads to the hardware doing strange things.  It
   *     doesn't hang as far as we can tell but rendering corruption can
   *     occur.
   *
   *  2) If this transition is into the GENERAL layout and we then use the
   *     image as a storage image, then we must have the aux buffer in the
   *     pass-through state so that, if we then go to texture from the
   *     image, we get the results of our storage image writes and not the
   *     fast clear color or other random data.
   *
   * For CCS both of the problems above are real demonstrable issues.  In
   * that case, the only thing we can do is to perform an ambiguate to
   * transition the aux surface into the pass-through state.
   *
   * For MCS, (2) is never an issue because we don't support multisampled
   * storage images.  In theory, issue (1) is a problem with MCS but we've
   * never seen it in the wild.  For 4x and 16x, all bit patterns could,
   * in theory, be interpreted as something but we don't know that all bit
   * patterns are actually valid.  For 2x and 8x, you could easily end up
   * with the MCS referring to an invalid plane because not all bits of
   * the MCS value are actually used.  Even though we've never seen issues
   * in the wild, it's best to play it safe and initialize the MCS.  We
   * could use a fast-clear for MCS because we only ever touch from render
   * and texture (no image load store). However, due to WA 14013111325,
   * we choose to ambiguate MCS as well.
   */
   if (image->vk.samples == 1) {
                        uint32_t aux_layers = anv_image_aux_layers(image, aspect, level);
   if (base_layer >= aux_layers)
                        /* If will_full_fast_clear is set, the caller promises to
   * fast-clear the largest portion of the specified range as it can.
   * For color images, that means only the first LOD and array slice.
   */
   if (level == 0 && base_layer == 0 && will_full_fast_clear) {
      base_layer++;
   level_layer_count--;
                     anv_image_ccs_op(cmd_buffer, image,
                              set_image_compressed_bit(cmd_buffer, image, aspect, level,
         } else {
      /* If will_full_fast_clear is set, the caller promises to fast-clear
   * the largest portion of the specified range as it can.
   */
                  assert(base_level == 0 && level_count == 1);
   anv_image_mcs_op(cmd_buffer, image,
                  image->planes[plane].primary_surface.isl.format,
   }
               /* The current code assumes that there is no mixing of CCS_E and CCS_D.
   * We can handle transitions between CCS_D/E to and from NONE.  What we
   * don't yet handle is switching between CCS_E and CCS_D within a given
   * image.  Doing so in a performant way requires more detailed aux state
   * tracking such as what is done in i965.  For now, just assume that we
   * only have one type of compression.
   */
   assert(initial_aux_usage == ISL_AUX_USAGE_NONE ||
                  /* If initial aux usage is NONE, there is nothing to resolve */
   if (initial_aux_usage == ISL_AUX_USAGE_NONE)
                     /* If the initial layout supports more fast clear than the final layout
   * then we need at least a partial resolve.
   */
   if (final_fast_clear < initial_fast_clear) {
      /* Partial resolves will actually only occur on layer 0/level 0. This
   * is generally okay because anv only allows explicit fast clears to
   * the first subresource.
   *
   * The situation is a bit different with FCV_CCS_E. With that aux
   * usage, implicit fast clears can occur on any layer and level.
   * anv doesn't track fast clear states for more than the first
   * subresource, so we need to assert that a layout transition doesn't
   * attempt to partial resolve the other subresources.
   *
   * At the moment, we don't enter such a situation, and partial resolves
   * for higher level/layer resources shouldn't be a concern.
   */
   if (image->planes[plane].aux_usage == ISL_AUX_USAGE_FCV_CCS_E) {
      assert(base_level == 0 && level_count == 1 &&
      }
               if (isl_aux_usage_has_ccs_e(initial_aux_usage) &&
      !isl_aux_usage_has_ccs_e(final_aux_usage))
         if (resolve_op == ISL_AUX_OP_NONE)
            /* Perform a resolve to synchronize data between the main and aux buffer.
   * Before we begin, we must satisfy the cache flushing requirement specified
   * in the Sky Lake PRM Vol. 7, "MCS Buffer for Render Target(s)":
   *
   *    Any transition from any value in {Clear, Render, Resolve} to a
   *    different value in {Clear, Render, Resolve} requires end of pipe
   *    synchronization.
   *
   * We perform a flush of the write cache before and after the clear and
   * resolve operations to meet this requirement.
   *
   * Unlike other drawing, fast clear operations are not properly
   * synchronized. The first PIPE_CONTROL here likely ensures that the
   * contents of the previous render or clear hit the render target before we
   * resolve and the second likely ensures that the resolve is complete before
   * we do any more rendering or clearing.
   */
   anv_add_pending_pipe_bits(cmd_buffer,
                        for (uint32_t l = 0; l < level_count; l++) {
               uint32_t aux_layers = anv_image_aux_layers(image, aspect, level);
   if (base_layer >= aux_layers)
         uint32_t level_layer_count =
            for (uint32_t a = 0; a < level_layer_count; a++) {
               /* If will_full_fast_clear is set, the caller promises to fast-clear
   * the largest portion of the specified range as it can.  For color
   * images, that means only the first LOD and array slice.
   */
                  if (image->vk.samples == 1) {
      anv_cmd_predicated_ccs_resolve(cmd_buffer, image,
                        } else {
      /* We only support fast-clear on the first layer so partial
   * resolves should not be used on other layers as they will use
   * the clear color stored in memory that is only valid for layer0.
   */
   if (resolve_op == ISL_AUX_OP_PARTIAL_RESOLVE &&
                  anv_cmd_predicated_mcs_resolve(cmd_buffer, image,
                                       anv_add_pending_pipe_bits(cmd_buffer,
                  }
      static MUST_CHECK VkResult
   anv_cmd_buffer_init_attachments(struct anv_cmd_buffer *cmd_buffer,
         {
               /* Reserve one for the NULL state. */
   unsigned num_states = 1 + color_att_count;
   const struct isl_device *isl_dev = &cmd_buffer->device->isl_dev;
   const uint32_t ss_stride = align(isl_dev->ss.size, isl_dev->ss.align);
   gfx->att_states =
      anv_state_stream_alloc(&cmd_buffer->surface_state_stream,
      if (gfx->att_states.map == NULL) {
      return anv_batch_set_error(&cmd_buffer->batch,
               struct anv_state next_state = gfx->att_states;
            gfx->null_surface_state = next_state;
   next_state.offset += ss_stride;
            gfx->color_att_count = color_att_count;
   for (uint32_t i = 0; i < color_att_count; i++) {
      gfx->color_att[i] = (struct anv_attachment) {
         };
   next_state.offset += ss_stride;
      }
   gfx->depth_att = (struct anv_attachment) { };
               }
      static void
   anv_cmd_buffer_reset_rendering(struct anv_cmd_buffer *cmd_buffer)
   {
               gfx->render_area = (VkRect2D) { };
   gfx->layer_count = 0;
            gfx->color_att_count = 0;
   gfx->depth_att = (struct anv_attachment) { };
   gfx->stencil_att = (struct anv_attachment) { };
      }
      /**
   * Program the hardware to use the specified L3 configuration.
   */
   void
   genX(cmd_buffer_config_l3)(struct anv_cmd_buffer *cmd_buffer,
         {
      assert(cfg || GFX_VER >= 12);
   if (cfg == cmd_buffer->state.current_l3_config)
         #if GFX_VER >= 11
      /* On Gfx11+ we use only one config, so verify it remains the same and skip
   * the stalling programming entirely.
   */
      #else
      if (INTEL_DEBUG(DEBUG_L3)) {
      mesa_logd("L3 config transition: ");
               /* According to the hardware docs, the L3 partitioning can only be changed
   * while the pipeline is completely drained and the caches are flushed,
   * which involves a first PIPE_CONTROL flush which stalls the pipeline...
   */
   genx_batch_emit_pipe_control(&cmd_buffer->batch, cmd_buffer->device->info,
                  /* ...followed by a second pipelined PIPE_CONTROL that initiates
   * invalidation of the relevant caches.  Note that because RO invalidation
   * happens at the top of the pipeline (i.e. right away as the PIPE_CONTROL
   * command is processed by the CS) we cannot combine it with the previous
   * stalling flush as the hardware documentation suggests, because that
   * would cause the CS to stall on previous rendering *after* RO
   * invalidation and wouldn't prevent the RO caches from being polluted by
   * concurrent rendering before the stall completes.  This intentionally
   * doesn't implement the SKL+ hardware workaround suggesting to enable CS
   * stall on PIPE_CONTROLs with the texture cache invalidation bit set for
   * GPGPU workloads because the previous and subsequent PIPE_CONTROLs
   * already guarantee that there is no concurrent GPGPU kernel execution
   * (see SKL HSD 2132585).
   */
   genx_batch_emit_pipe_control(&cmd_buffer->batch, cmd_buffer->device->info,
                              /* Now send a third stalling flush to make sure that invalidation is
   * complete when the L3 configuration registers are modified.
   */
   genx_batch_emit_pipe_control(&cmd_buffer->batch, cmd_buffer->device->info,
                     #endif /* GFX_VER >= 11 */
         }
      ALWAYS_INLINE enum anv_pipe_bits
   genX(emit_apply_pipe_flushes)(struct anv_batch *batch,
                           {
   #if GFX_VER >= 12
      /* From the TGL PRM, Volume 2a, "PIPE_CONTROL":
   *
   *     "SW must follow below programming restrictions when programming
   *      PIPE_CONTROL command [for ComputeCS]:
   *      ...
   *      Following bits must not be set when programmed for ComputeCS:
   *      - "Render Target Cache Flush Enable", "Depth Cache Flush Enable"
   *         and "Tile Cache Flush Enable"
   *      - "Depth Stall Enable", Stall at Pixel Scoreboard and
   *         "PSD Sync Enable".
   *      - "OVR Tile 0 Flush", "TBIMR Force Batch Closure",
   *         "AMFS Flush Enable", "VF Cache Invalidation Enable" and
   *         "Global Snapshot Count Reset"."
   *
   * XXX: According to spec this should not be a concern for a regular
   * RCS in GPGPU mode, but during testing it was found that at least
   * "VF Cache Invalidation Enable" bit is ignored in such case.
   * This can cause us to miss some important invalidations
   * (e.g. from CmdPipelineBarriers) and have incoherent data.
   *
   * There is also a Wa_1606932921 "RCS is not waking up fixed function clock
   * when specific 3d related bits are programmed in pipecontrol in
   * compute mode" that suggests us not to use "RT Cache Flush" in GPGPU mode.
   *
   * The other bits are not confirmed to cause problems, but included here
   * just to be safe, as they're also not really relevant in the GPGPU mode,
   * and having them doesn't seem to cause any regressions.
   *
   * So if we're currently in GPGPU mode, we hide some bits from
   * this flush, and will flush them only when we'll be able to.
   * Similar thing with GPGPU-only bits.
   */
   enum anv_pipe_bits defer_bits = bits &
               #endif
         /*
   * From Sandybridge PRM, volume 2, "1.7.2 End-of-Pipe Synchronization":
   *
   *    Write synchronization is a special case of end-of-pipe
   *    synchronization that requires that the render cache and/or depth
   *    related caches are flushed to memory, where the data will become
   *    globally visible. This type of synchronization is required prior to
   *    SW (CPU) actually reading the result data from memory, or initiating
   *    an operation that will use as a read surface (such as a texture
   *    surface) a previous render target and/or depth/stencil buffer
   *
   *
   * From Haswell PRM, volume 2, part 1, "End-of-Pipe Synchronization":
   *
   *    Exercising the write cache flush bits (Render Target Cache Flush
   *    Enable, Depth Cache Flush Enable, DC Flush) in PIPE_CONTROL only
   *    ensures the write caches are flushed and doesn't guarantee the data
   *    is globally visible.
   *
   *    SW can track the completion of the end-of-pipe-synchronization by
   *    using "Notify Enable" and "PostSync Operation - Write Immediate
   *    Data" in the PIPE_CONTROL command.
   *
   * In other words, flushes are pipelined while invalidations are handled
   * immediately.  Therefore, if we're flushing anything then we need to
   * schedule an end-of-pipe sync before any invalidations can happen.
   */
   if (bits & ANV_PIPE_FLUSH_BITS)
               /* HSD 1209978178: docs say that before programming the aux table:
   *
   *    "Driver must ensure that the engine is IDLE but ensure it doesn't
   *    add extra flushes in the case it knows that the engine is already
   *    IDLE."
   *
   * HSD 22012751911: SW Programming sequence when issuing aux invalidation:
   *
   *    "Render target Cache Flush + L3 Fabric Flush + State Invalidation + CS Stall"
   *
   * Notice we don't set the L3 Fabric Flush here, because we have
   * ANV_PIPE_END_OF_PIPE_SYNC_BIT which inserts a CS stall. The
   * PIPE_CONTROL::L3 Fabric Flush documentation says :
   *
   *    "L3 Fabric Flush will ensure all the pending transactions in the L3
   *     Fabric are flushed to global observation point. HW does implicit L3
   *     Fabric Flush on all stalling flushes (both explicit and implicit)
   *     and on PIPECONTROL having Post Sync Operation enabled."
   *
   * Therefore setting L3 Fabric Flush here would be redundant.
   */
   if (GFX_VER == 12 && (bits & ANV_PIPE_AUX_TABLE_INVALIDATE_BIT)) {
      if (current_pipeline == GPGPU) {
      bits |= (ANV_PIPE_NEEDS_END_OF_PIPE_SYNC_BIT |
            } else if (current_pipeline == _3D) {
      bits |= (ANV_PIPE_NEEDS_END_OF_PIPE_SYNC_BIT |
            ANV_PIPE_RENDER_TARGET_CACHE_FLUSH_BIT |
               /* If we're going to do an invalidate and we have a pending end-of-pipe
   * sync that has yet to be resolved, we do the end-of-pipe sync now.
   */
   if ((bits & ANV_PIPE_INVALIDATE_BITS) &&
      (bits & ANV_PIPE_NEEDS_END_OF_PIPE_SYNC_BIT)) {
   bits |= ANV_PIPE_END_OF_PIPE_SYNC_BIT;
            if (INTEL_DEBUG(DEBUG_PIPE_CONTROL) && bits) {
      fputs("pc: add ", stderr);
   anv_dump_pipe_bits(ANV_PIPE_END_OF_PIPE_SYNC_BIT, stdout);
                  /* Project: SKL / Argument: LRI Post Sync Operation [23]
   *
   * "PIPECONTROL command with “Command Streamer Stall Enable” must be
   *  programmed prior to programming a PIPECONTROL command with "LRI
   *  Post Sync Operation" in GPGPU mode of operation (i.e when
   *  PIPELINE_SELECT command is set to GPGPU mode of operation)."
   *
   * The same text exists a few rows below for Post Sync Op.
   */
   if (bits & ANV_PIPE_POST_SYNC_BIT) {
      if (GFX_VER == 9 && current_pipeline == GPGPU)
                     if (bits & (ANV_PIPE_FLUSH_BITS | ANV_PIPE_STALL_BITS |
            enum anv_pipe_bits flush_bits =
            #if GFX_VERx10 >= 125
         if (current_pipeline != GPGPU) {
      if (flush_bits & ANV_PIPE_HDC_PIPELINE_FLUSH_BIT)
      } else {
      if (flush_bits & (ANV_PIPE_HDC_PIPELINE_FLUSH_BIT |
                     /* BSpec 47112: PIPE_CONTROL::Untyped Data-Port Cache Flush:
   *
   *    "'HDC Pipeline Flush' bit must be set for this bit to take
   *     effect."
   */
   if (flush_bits & ANV_PIPE_UNTYPED_DATAPORT_CACHE_FLUSH_BIT)
   #endif
      #if GFX_VER < 12
         if (flush_bits & ANV_PIPE_HDC_PIPELINE_FLUSH_BIT)
   #endif
            uint32_t sync_op = NoWrite;
            /* From Sandybridge PRM, volume 2, "1.7.3.1 Writing a Value to Memory":
   *
   *    "The most common action to perform upon reaching a
   *    synchronization point is to write a value out to memory. An
   *    immediate value (included with the synchronization command) may
   *    be written."
   *
   *
   * From Broadwell PRM, volume 7, "End-of-Pipe Synchronization":
   *
   *    "In case the data flushed out by the render engine is to be
   *    read back in to the render engine in coherent manner, then the
   *    render engine has to wait for the fence completion before
   *    accessing the flushed data. This can be achieved by following
   *    means on various products: PIPE_CONTROL command with CS Stall
   *    and the required write caches flushed with Post-Sync-Operation
   *    as Write Immediate Data.
   *
   *    Example:
   *       - Workload-1 (3D/GPGPU/MEDIA)
   *       - PIPE_CONTROL (CS Stall, Post-Sync-Operation Write
   *         Immediate Data, Required Write Cache Flush bits set)
   *       - Workload-2 (Can use the data produce or output by
   *         Workload-1)
   */
   if (flush_bits & ANV_PIPE_END_OF_PIPE_SYNC_BIT) {
      flush_bits |= ANV_PIPE_CS_STALL_BIT;
   sync_op = WriteImmediateData;
               /* Flush PC. */
   genx_batch_emit_pipe_control_write(batch, device->info, sync_op, addr,
            /* If the caller wants to know what flushes have been emitted,
   * provide the bits based off the PIPE_CONTROL programmed bits.
   */
   if (emitted_flush_bits != NULL)
            bits &= ~(ANV_PIPE_FLUSH_BITS | ANV_PIPE_STALL_BITS |
               if (bits & ANV_PIPE_INVALIDATE_BITS) {
      /* From the SKL PRM, Vol. 2a, "PIPE_CONTROL",
   *
   *    "If the VF Cache Invalidation Enable is set to a 1 in a
   *    PIPE_CONTROL, a separate Null PIPE_CONTROL, all bitfields sets to
   *    0, with the VF Cache Invalidation Enable set to 0 needs to be sent
   *    prior to the PIPE_CONTROL with VF Cache Invalidation Enable set to
   *    a 1."
   *
   * This appears to hang Broadwell, so we restrict it to just gfx9.
   */
   if (GFX_VER == 9 && (bits & ANV_PIPE_VF_CACHE_INVALIDATE_BIT))
      #if GFX_VER >= 9 && GFX_VER <= 11
         /* From the SKL PRM, Vol. 2a, "PIPE_CONTROL",
   *
   *    "Workaround : “CS Stall” bit in PIPE_CONTROL command must be
   *     always set for GPGPU workloads when “Texture Cache
   *     Invalidation Enable” bit is set".
   *
   * Workaround stopped appearing in TGL PRMs.
   */
   if (current_pipeline == GPGPU &&
         #endif
            uint32_t sync_op = NoWrite;
            /* From the SKL PRM, Vol. 2a, "PIPE_CONTROL",
   *
   *    "When VF Cache Invalidate is set “Post Sync Operation” must be
   *    enabled to “Write Immediate Data” or “Write PS Depth Count” or
   *    “Write Timestamp”.
   */
   if (GFX_VER == 9 && (bits & ANV_PIPE_VF_CACHE_INVALIDATE_BIT)) {
      sync_op = WriteImmediateData;
               /* Invalidate PC. */
   genx_batch_emit_pipe_control_write(batch, device->info, sync_op, addr,
      #if GFX_VER == 12
         if ((bits & ANV_PIPE_AUX_TABLE_INVALIDATE_BIT) && device->info->has_aux_map) {
      uint64_t register_addr =
      current_pipeline == GPGPU ? GENX(COMPCS0_CCS_AUX_INV_num) :
      anv_batch_emit(batch, GENX(MI_LOAD_REGISTER_IMM), lri) {
      lri.RegisterOffset = register_addr;
      }
   /* HSD 22012751911: SW Programming sequence when issuing aux invalidation:
   *
   *    "Poll Aux Invalidation bit once the invalidation is set
   *     (Register 4208 bit 0)"
   */
   anv_batch_emit(batch, GENX(MI_SEMAPHORE_WAIT), sem) {
      sem.CompareOperation = COMPARE_SAD_EQUAL_SDD;
   sem.WaitMode = PollingMode;
   sem.RegisterPollMode = true;
   sem.SemaphoreDataDword = 0x0;
   sem.SemaphoreAddress =
         #else
         #endif
                     #if GFX_VER >= 12
         #endif
            }
      ALWAYS_INLINE void
   genX(cmd_buffer_apply_pipe_flushes)(struct anv_cmd_buffer *cmd_buffer)
   {
   #if INTEL_NEEDS_WA_1508744258
      /* If we're changing the state of the RHWO optimization, we need to have
   * sb_stall+cs_stall.
   */
   const bool rhwo_opt_change =
      cmd_buffer->state.rhwo_optimization_enabled !=
      if (rhwo_opt_change) {
      anv_add_pending_pipe_bits(cmd_buffer,
                     #endif
                  if (unlikely(cmd_buffer->device->physical->always_flush_cache))
         else if (bits == 0)
            if (anv_cmd_buffer_is_blitter_queue(cmd_buffer))
            const bool trace_flush =
      (bits & (ANV_PIPE_FLUSH_BITS |
            ANV_PIPE_STALL_BITS |
   if (trace_flush)
            if (GFX_VER == 9 &&
      (bits & ANV_PIPE_CS_STALL_BIT) &&
   (bits & ANV_PIPE_VF_CACHE_INVALIDATE_BIT)) {
   /* If we are doing a VF cache invalidate AND a CS stall (it must be
   * both) then we can reset our vertex cache tracking.
   */
   memset(cmd_buffer->state.gfx.vb_dirty_ranges, 0,
         memset(&cmd_buffer->state.gfx.ib_dirty_range, 0,
                  enum anv_pipe_bits emitted_bits = 0;
   cmd_buffer->state.pending_pipe_bits =
      genX(emit_apply_pipe_flushes)(&cmd_buffer->batch,
                              #if INTEL_NEEDS_WA_1508744258
      if (rhwo_opt_change) {
      anv_batch_write_reg(&cmd_buffer->batch, GENX(COMMON_SLICE_CHICKEN1), c1) {
      c1.RCCRHWOOptimizationDisable =
            }
   cmd_buffer->state.rhwo_optimization_enabled =
         #endif
         if (trace_flush) {
      trace_intel_end_stall(&cmd_buffer->trace,
               }
      static void
   cmd_buffer_alloc_gfx_push_constants(struct anv_cmd_buffer *cmd_buffer)
   {
      VkShaderStageFlags stages =
            /* In order to avoid thrash, we assume that vertex and fragment stages
   * always exist.  In the rare case where one is missing *and* the other
   * uses push concstants, this may be suboptimal.  However, avoiding stalls
   * seems more important.
   */
   stages |= VK_SHADER_STAGE_FRAGMENT_BIT;
   if (anv_pipeline_is_primitive(cmd_buffer->state.gfx.pipeline))
            if (stages == cmd_buffer->state.gfx.push_constant_stages)
                     const struct intel_device_info *devinfo = cmd_buffer->device->info;
   if (anv_pipeline_is_mesh(cmd_buffer->state.gfx.pipeline))
         else
            const unsigned num_stages =
                  /* Broadwell+ and Haswell gt3 require that the push constant sizes be in
   * units of 2KB.  Incidentally, these are the same platforms that have
   * 32KB worth of push constant space.
   */
   if (push_constant_kb == 32)
            uint32_t kb_used = 0;
   for (int i = MESA_SHADER_VERTEX; i < MESA_SHADER_FRAGMENT; i++) {
      const unsigned push_size = (stages & (1 << i)) ? size_per_stage : 0;
   anv_batch_emit(&cmd_buffer->batch,
            alloc._3DCommandSubOpcode  = 18 + i;
   alloc.ConstantBufferOffset = (push_size > 0) ? kb_used : 0;
      }
               anv_batch_emit(&cmd_buffer->batch,
            alloc.ConstantBufferOffset = kb_used;
            #if GFX_VERx10 == 125
      /* DG2: Wa_22011440098
   * MTL: Wa_18022330953
   *
   * In 3D mode, after programming push constant alloc command immediately
   * program push constant command(ZERO length) without any commit between
   * them.
   */
   anv_batch_emit(&cmd_buffer->batch, GENX(3DSTATE_CONSTANT_ALL), c) {
      /* Update empty push constants for all stages (bitmask = 11111b) */
   c.ShaderUpdateEnable = 0x1f;
         #endif
                  /* From the BDW PRM for 3DSTATE_PUSH_CONSTANT_ALLOC_VS:
   *
   *    "The 3DSTATE_CONSTANT_VS must be reprogrammed prior to
   *    the next 3DPRIMITIVE command after programming the
   *    3DSTATE_PUSH_CONSTANT_ALLOC_VS"
   *
   * Since 3DSTATE_PUSH_CONSTANT_ALLOC_VS is programmed as part of
   * pipeline setup, we need to dirty push constants.
   */
      }
      static inline struct anv_state
   emit_dynamic_buffer_binding_table_entry(struct anv_cmd_buffer *cmd_buffer,
                     {
      /* Compute the offset within the buffer */
   uint32_t dynamic_offset =
      pipe_state->dynamic_offsets[
      uint64_t offset = desc->offset + dynamic_offset;
   /* Clamp to the buffer size */
   offset = MIN2(offset, desc->buffer->vk.size);
   /* Clamp the range to the buffer size */
            /* Align the range for consistency */
   if (desc->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
            struct anv_address address =
            struct anv_state surface_state = anv_cmd_buffer_alloc_surface_state(cmd_buffer);
   enum isl_format format =
      anv_isl_format_for_descriptor_type(cmd_buffer->device,
         isl_surf_usage_flags_t usage =
      desc->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ?
   ISL_SURF_USAGE_CONSTANT_BUFFER_BIT :
         anv_fill_buffer_surface_state(cmd_buffer->device,
                           }
      static uint32_t
   emit_indirect_descriptor_binding_table_entry(struct anv_cmd_buffer *cmd_buffer,
                     {
      struct anv_device *device = cmd_buffer->device;
            /* Relative offset in the STATE_BASE_ADDRESS::SurfaceStateBaseAddress heap.
   * Depending on where the descriptor surface state is allocated, they can
   * either come from device->internal_surface_state_pool or
   * device->bindless_surface_state_pool.
   */
   switch (desc->type) {
   case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: {
      if (desc->image_view) {
      const struct anv_surface_state *sstate =
      (desc->layout == VK_IMAGE_LAYOUT_GENERAL) ?
   &desc->image_view->planes[binding->plane].general_sampler :
      surface_state = desc->image_view->use_surface_state_stream ?
      sstate->state :
         } else {
         }
               case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: {
      if (desc->image_view) {
      const struct anv_surface_state *sstate =
         surface_state = desc->image_view->use_surface_state_stream ?
      sstate->state :
         } else {
      surface_state =
      }
               case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
      if (desc->set_buffer_view) {
      surface_state = desc->set_buffer_view->general.state;
      } else {
         }
         case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
      if (desc->buffer_view) {
      surface_state = anv_bindless_state_for_binding_table(
      device,
         } else {
         }
         case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: {
      if (desc->buffer) {
      surface_state =
      emit_dynamic_buffer_binding_table_entry(cmd_buffer, pipe_state,
   } else {
         }
               case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
      if (desc->buffer_view) {
      surface_state = anv_bindless_state_for_binding_table(
            } else {
         }
         default:
                  assert(surface_state.map);
      }
      static uint32_t
   emit_direct_descriptor_binding_table_entry(struct anv_cmd_buffer *cmd_buffer,
                           {
      struct anv_device *device = cmd_buffer->device;
            /* Relative offset in the STATE_BASE_ADDRESS::SurfaceStateBaseAddress heap.
   * Depending on where the descriptor surface state is allocated, they can
   * either come from device->internal_surface_state_pool or
   * device->bindless_surface_state_pool.
   */
   switch (desc->type) {
   case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
   case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
   case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
      desc_offset = set->desc_offset + binding->set_offset;
         case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: {
      struct anv_state state = desc->buffer ?
      emit_dynamic_buffer_binding_table_entry(cmd_buffer, pipe_state,
            desc_offset = state.offset;
               default:
                     }
      static VkResult
   emit_binding_table(struct anv_cmd_buffer *cmd_buffer,
                     {
               struct anv_pipeline_bind_map *map = &shader->bind_map;
   if (map->surface_count == 0) {
      *bt_state = (struct anv_state) { 0, };
               *bt_state = anv_cmd_buffer_alloc_binding_table(cmd_buffer,
                        if (bt_state->map == NULL)
            for (uint32_t s = 0; s < map->surface_count; s++) {
                        switch (binding->set) {
   case ANV_DESCRIPTOR_SET_NULL:
                  case ANV_DESCRIPTOR_SET_COLOR_ATTACHMENTS:
      /* Color attachment binding */
   assert(shader->stage == MESA_SHADER_FRAGMENT);
   if (binding->index < cmd_buffer->state.gfx.color_att_count) {
      const struct anv_attachment *att =
            } else {
         }
   assert(surface_state.map);
               case ANV_DESCRIPTOR_SET_NUM_WORK_GROUPS: {
                                    const enum isl_format format =
      anv_isl_format_for_descriptor_type(cmd_buffer->device,
      anv_fill_buffer_surface_state(cmd_buffer->device, surface_state.map,
                              assert(surface_state.map);
   bt_map[s] = surface_state.offset + state_offset;
               case ANV_DESCRIPTOR_SET_DESCRIPTORS: {
                     /* If the shader doesn't access the set buffer, just put the null
   * surface.
   */
   if (set->is_push && !shader->push_desc_info.used_set_buffer) {
      bt_map[s] = 0;
               /* This is a descriptor set buffer so the set index is actually
   * given by binding->binding.  (Yes, that's confusing.)
   */
   assert(set->desc_mem.alloc_size);
   assert(set->desc_surface_state.alloc_size);
   bt_map[s] = set->desc_surface_state.offset + state_offset;
   add_surface_reloc(cmd_buffer, anv_descriptor_set_address(set));
               default: {
      assert(binding->set < MAX_SETS);
                  if (binding->index >= set->descriptor_count) {
      /* From the Vulkan spec section entitled "DescriptorSet and
   * Binding Assignment":
   *
   *    "If the array is runtime-sized, then array elements greater
   *    than or equal to the size of that binding in the bound
   *    descriptor set must not be used."
   *
   * Unfortunately, the compiler isn't smart enough to figure out
   * when a dynamic binding isn't used so it may grab the whole
   * array and stick it in the binding table.  In this case, it's
   * safe to just skip those bindings that are OOB.
   */
   assert(binding->index < set->layout->descriptor_count);
               /* For push descriptor, if the binding is fully promoted to push
   * constants, just reference the null surface in the binding table.
   * It's unused and we didn't allocate/pack a surface state for it .
   */
   if (set->is_push) {
                     if (shader->push_desc_info.fully_promoted_ubo_descriptors & BITFIELD_BIT(desc_idx)) {
      surface_state =
                        const struct anv_descriptor *desc = &set->descriptors[binding->index];
   if (desc->type == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR ||
      desc->type == VK_DESCRIPTOR_TYPE_SAMPLER) {
   /* Nothing for us to do here */
               const struct anv_pipeline *pipeline = pipe_state->pipeline;
   uint32_t surface_state_offset;
   if (pipeline->layout.type == ANV_PIPELINE_DESCRIPTOR_SET_LAYOUT_TYPE_INDIRECT) {
      surface_state_offset =
      emit_indirect_descriptor_binding_table_entry(cmd_buffer,
         } else {
      surface_state_offset =
                     bt_map[s] = surface_state_offset + state_offset;
      }
                  }
      static VkResult
   emit_samplers(struct anv_cmd_buffer *cmd_buffer,
               struct anv_cmd_pipeline_state *pipe_state,
   {
      struct anv_pipeline_bind_map *map = &shader->bind_map;
   if (map->sampler_count == 0) {
      *state = (struct anv_state) { 0, };
               uint32_t size = map->sampler_count * 16;
            if (state->map == NULL)
            for (uint32_t s = 0; s < map->sampler_count; s++) {
      struct anv_pipeline_binding *binding = &map->sampler_to_descriptor[s];
   const struct anv_descriptor *desc =
            if (desc->type != VK_DESCRIPTOR_TYPE_SAMPLER &&
                           /* This can happen if we have an unfilled slot since TYPE_SAMPLER
   * happens to be zero.
   */
   if (sampler == NULL)
            memcpy(state->map + (s * 16),
                  }
      static uint32_t
   flush_descriptor_sets(struct anv_cmd_buffer *cmd_buffer,
                           {
               VkResult result = VK_SUCCESS;
   for (uint32_t i = 0; i < num_shaders; i++) {
      if (!shaders[i])
            gl_shader_stage stage = shaders[i]->stage;
   VkShaderStageFlags vk_stage = mesa_to_vk_shader_stage(stage);
   if ((vk_stage & dirty) == 0)
            assert(stage < ARRAY_SIZE(cmd_buffer->state.samplers));
   result = emit_samplers(cmd_buffer, pipe_state, shaders[i],
         if (result != VK_SUCCESS)
            assert(stage < ARRAY_SIZE(cmd_buffer->state.binding_tables));
   result = emit_binding_table(cmd_buffer, pipe_state, shaders[i],
         if (result != VK_SUCCESS)
                        if (result != VK_SUCCESS) {
               result = anv_cmd_buffer_new_binding_table_block(cmd_buffer);
   if (result != VK_SUCCESS)
            /* Re-emit state base addresses so we get the new surface state base
   * address before we start emitting binding tables etc.
   */
            /* Re-emit all active binding tables */
            for (uint32_t i = 0; i < num_shaders; i++) {
                              result = emit_samplers(cmd_buffer, pipe_state, shaders[i],
         if (result != VK_SUCCESS) {
      anv_batch_set_error(&cmd_buffer->batch, result);
      }
   result = emit_binding_table(cmd_buffer, pipe_state, shaders[i],
         if (result != VK_SUCCESS) {
      anv_batch_set_error(&cmd_buffer->batch, result);
                                 }
      /* This functions generates surface states used by a pipeline for push
   * descriptors. This is delayed to the draw/dispatch time to avoid allocation
   * and surface state generation when a pipeline is not going to use the
   * binding table to access any push descriptor data.
   */
   static void
   flush_push_descriptor_set(struct anv_cmd_buffer *cmd_buffer,
               {
      assert(pipeline->use_push_descriptor &&
            struct anv_descriptor_set *set =
         while (set->generate_surface_states) {
      int desc_idx = u_bit_scan(&set->generate_surface_states);
   struct anv_descriptor *desc = &set->descriptors[desc_idx];
            if (bview != NULL) {
      bview->general.state =
         anv_descriptor_write_surface_state(cmd_buffer->device, desc,
                  if (pipeline->use_push_descriptor_buffer) {
      struct anv_descriptor_set_layout *layout = set->layout;
   enum isl_format format =
                  set->desc_surface_state = anv_cmd_buffer_alloc_surface_state(cmd_buffer);
   anv_fill_buffer_surface_state(cmd_buffer->device,
                                 }
      static void
   cmd_buffer_emit_descriptor_pointers(struct anv_cmd_buffer *cmd_buffer,
         {
      static const uint32_t sampler_state_opcodes[] = {
      [MESA_SHADER_VERTEX]                      = 43,
   [MESA_SHADER_TESS_CTRL]                   = 44, /* HS */
   [MESA_SHADER_TESS_EVAL]                   = 45, /* DS */
   [MESA_SHADER_GEOMETRY]                    = 46,
               static const uint32_t binding_table_opcodes[] = {
      [MESA_SHADER_VERTEX]                      = 38,
   [MESA_SHADER_TESS_CTRL]                   = 39,
   [MESA_SHADER_TESS_EVAL]                   = 40,
   [MESA_SHADER_GEOMETRY]                    = 41,
               anv_foreach_stage(s, stages) {
               if (cmd_buffer->state.samplers[s].alloc_size > 0) {
      anv_batch_emit(&cmd_buffer->batch,
            ssp._3DCommandSubOpcode = sampler_state_opcodes[s];
                  /* Always emit binding table pointers if we're asked to, since on SKL
   * this is what flushes push constants. */
   anv_batch_emit(&cmd_buffer->batch,
            btp._3DCommandSubOpcode = binding_table_opcodes[s];
            }
      static struct anv_address
   get_push_range_address(struct anv_cmd_buffer *cmd_buffer,
               {
      struct anv_cmd_graphics_state *gfx_state = &cmd_buffer->state.gfx;
   switch (range->set) {
   case ANV_DESCRIPTOR_SET_DESCRIPTORS: {
      /* This is a descriptor set buffer so the set index is
   * actually given by binding->binding.  (Yes, that's
   * confusing.)
   */
   struct anv_descriptor_set *set =
                     case ANV_DESCRIPTOR_SET_PUSH_CONSTANTS: {
      if (gfx_state->base.push_constants_state.alloc_size == 0) {
      gfx_state->base.push_constants_state =
      }
   return anv_state_pool_state_address(
      &cmd_buffer->device->dynamic_state_pool,
            default: {
      assert(range->set < MAX_SETS);
   struct anv_descriptor_set *set =
         const struct anv_descriptor *desc =
            if (desc->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
      if (desc->buffer) {
      return anv_address_add(desc->buffer->address,
         } else {
      assert(desc->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
   if (desc->buffer) {
      const struct anv_cmd_pipeline_state *pipe_state = &gfx_state->base;
   uint32_t dynamic_offset =
      pipe_state->dynamic_offsets[
      return anv_address_add(desc->buffer->address,
                  /* For NULL UBOs, we just return an address in the workaround BO.  We do
   * writes to it for workarounds but always at the bottom.  The higher
   * bytes should be all zeros.
   */
   assert(range->length * 32 <= 2048);
   return (struct anv_address) {
      .bo = cmd_buffer->device->workaround_bo,
         }
      }
         /** Returns the size in bytes of the bound buffer
   *
   * The range is relative to the start of the buffer, not the start of the
   * range.  The returned range may be smaller than
   *
   *    (range->start + range->length) * 32;
   */
   static uint32_t
   get_push_range_bound_size(struct anv_cmd_buffer *cmd_buffer,
               {
      assert(shader->stage != MESA_SHADER_COMPUTE);
   const struct anv_cmd_graphics_state *gfx_state = &cmd_buffer->state.gfx;
   switch (range->set) {
   case ANV_DESCRIPTOR_SET_DESCRIPTORS: {
      struct anv_descriptor_set *set =
         assert(range->start * 32 < set->desc_mem.alloc_size);
   assert((range->start + range->length) * 32 <= set->desc_mem.alloc_size);
               case ANV_DESCRIPTOR_SET_PUSH_CONSTANTS:
            default: {
      assert(range->set < MAX_SETS);
   struct anv_descriptor_set *set =
         const struct anv_descriptor *desc =
            if (desc->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
      /* Here we promote a UBO to a binding table entry so that we can avoid a layer of indirection.
         */
                                    } else {
                     assert(desc->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
   /* Compute the offset within the buffer */
   const struct anv_cmd_pipeline_state *pipe_state = &gfx_state->base;
   uint32_t dynamic_offset =
      pipe_state->dynamic_offsets[
      uint64_t offset = desc->offset + dynamic_offset;
   /* Clamp to the buffer size */
   offset = MIN2(offset, desc->buffer->vk.size);
                                       }
      }
      static void
   cmd_buffer_emit_push_constant(struct anv_cmd_buffer *cmd_buffer,
                     {
      const struct anv_cmd_graphics_state *gfx_state = &cmd_buffer->state.gfx;
            static const uint32_t push_constant_opcodes[] = {
      [MESA_SHADER_VERTEX]                      = 21,
   [MESA_SHADER_TESS_CTRL]                   = 25, /* HS */
   [MESA_SHADER_TESS_EVAL]                   = 26, /* DS */
   [MESA_SHADER_GEOMETRY]                    = 22,
                                 anv_batch_emit(&cmd_buffer->batch, GENX(3DSTATE_CONSTANT_VS), c) {
               /* Set MOCS.
   *
   * We only have one MOCS field for the whole packet, not one per
   * buffer.  We could go out of our way here to walk over all of
   * the buffers and see if any of them are used externally and use
   * the external MOCS.  However, the notion that someone would use
   * the same bit of memory for both scanout and a UBO is nuts.
   *
   * Let's not bother and assume it's all internal.
   */
            if (anv_pipeline_has_stage(pipeline, stage)) {
                     /* The Skylake PRM contains the following restriction:
   *
   *    "The driver must ensure The following case does not occur
   *     without a flush to the 3D engine: 3DSTATE_CONSTANT_* with
   *     buffer 3 read length equal to zero committed followed by a
   *     3DSTATE_CONSTANT_* with buffer 0 read length not equal to
   *     zero committed."
   *
   * To avoid this, we program the buffers in the highest slots.
   * This way, slot 0 is only used if slot 3 is also used.
   */
   assert(buffer_count <= 4);
   const unsigned shift = 4 - buffer_count;
                                    c.ConstantBody.ReadLength[i + shift] = range->length;
   c.ConstantBody.Buffer[i + shift] =
               }
      #if GFX_VER >= 12
   static void
   cmd_buffer_emit_push_constant_all(struct anv_cmd_buffer *cmd_buffer,
                     {
      if (buffer_count == 0) {
      anv_batch_emit(&cmd_buffer->batch, GENX(3DSTATE_CONSTANT_ALL), c) {
      c.ShaderUpdateEnable = shader_mask;
      }
               const struct anv_cmd_graphics_state *gfx_state = &cmd_buffer->state.gfx;
                     const struct anv_pipeline_bind_map *bind_map =
            uint32_t *dw;
   const uint32_t buffer_mask = (1 << buffer_count) - 1;
            dw = anv_batch_emitn(&cmd_buffer->batch, num_dwords,
                              for (int i = 0; i < buffer_count; i++) {
      const struct anv_push_range *range = &bind_map->push_ranges[i];
   GENX(3DSTATE_CONSTANT_ALL_DATA_pack)(
      &cmd_buffer->batch, dw + 2 + i * 2,
   &(struct GENX(3DSTATE_CONSTANT_ALL_DATA)) {
      .PointerToConstantBuffer =
               }
   #endif
      static void
   cmd_buffer_flush_gfx_push_constants(struct anv_cmd_buffer *cmd_buffer,
         {
      VkShaderStageFlags flushed = 0;
   struct anv_cmd_graphics_state *gfx_state = &cmd_buffer->state.gfx;
         #if GFX_VER >= 12
         #endif
         /* Compute robust pushed register access mask for each stage. */
   anv_foreach_stage(stage, dirty_stages) {
      if (!anv_pipeline_has_stage(pipeline, stage))
            const struct anv_shader_bin *shader = pipeline->base.shaders[stage];
   if (shader->prog_data->zero_push_reg) {
                     push->push_reg_mask[stage] = 0;
   /* Start of the current range in the shader, relative to the start of
   * push constants in the shader.
   */
   unsigned range_start_reg = 0;
   for (unsigned i = 0; i < 4; i++) {
      const struct anv_push_range *range = &bind_map->push_ranges[i];
                  unsigned bound_size =
         if (bound_size >= range->start * 32) {
      unsigned bound_regs =
      MIN2(DIV_ROUND_UP(bound_size, 32) - range->start,
      assert(range_start_reg + bound_regs <= 64);
                                                      /* Resets the push constant state so that we allocate a new one if
   * needed.
   */
            anv_foreach_stage(stage, dirty_stages) {
      unsigned buffer_count = 0;
   flushed |= mesa_to_vk_shader_stage(stage);
            struct anv_address buffers[4] = {};
   if (anv_pipeline_has_stage(pipeline, stage)) {
                     /* We have to gather buffer addresses as a second step because the
   * loop above puts data into the push constant area and the call to
   * get_push_range_address is what locks our push constants and copies
   * them into the actual GPU buffer.  If we did the two loops at the
   * same time, we'd risk only having some of the sizes in the push
   * constant buffer when we did the copy.
   */
   for (unsigned i = 0; i < 4; i++) {
      const struct anv_push_range *range = &bind_map->push_ranges[i];
                  buffers[i] = get_push_range_address(cmd_buffer, shader, range);
   max_push_range = MAX2(max_push_range, range->length);
               /* We have at most 4 buffers but they should be tightly packed */
   for (unsigned i = buffer_count; i < 4; i++)
         #if GFX_VER >= 12
         /* If this stage doesn't have any push constants, emit it later in a
   * single CONSTANT_ALL packet.
   */
   if (buffer_count == 0) {
      nobuffer_stages |= 1 << stage;
               /* The Constant Buffer Read Length field from 3DSTATE_CONSTANT_ALL
   * contains only 5 bits, so we can only use it for buffers smaller than
   * 32.
   *
   * According to Wa_16011448509, Gfx12.0 misinterprets some address bits
   * in 3DSTATE_CONSTANT_ALL.  It should still be safe to use the command
   * for disabling stages, where all address bits are zero.  However, we
   * can't safely use it for general buffers with arbitrary addresses.
   * Just fall back to the individual 3DSTATE_CONSTANT_XS commands in that
   * case.
   */
   if (max_push_range < 32 && GFX_VERx10 > 120) {
      cmd_buffer_emit_push_constant_all(cmd_buffer, 1 << stage,
            #endif
                     #if GFX_VER >= 12
      if (nobuffer_stages)
      /* Wa_16011448509: all address bits are zero */
   #endif
            }
      #if GFX_VERx10 >= 125
   static void
   cmd_buffer_flush_mesh_inline_data(struct anv_cmd_buffer *cmd_buffer,
         {
      struct anv_cmd_graphics_state *gfx_state = &cmd_buffer->state.gfx;
            if (dirty_stages & VK_SHADER_STAGE_TASK_BIT_EXT &&
               const struct anv_shader_bin *shader = pipeline->base.shaders[MESA_SHADER_TASK];
            anv_batch_emit(&cmd_buffer->batch, GENX(3DSTATE_TASK_SHADER_DATA), data) {
      const struct anv_push_range *range = &bind_map->push_ranges[0];
   if (range->length > 0) {
                     uint64_t addr = anv_address_physical(buffer);
                  memcpy(&data.InlineData[BRW_TASK_MESH_PUSH_CONSTANTS_START_DW],
                           if (dirty_stages & VK_SHADER_STAGE_MESH_BIT_EXT &&
               const struct anv_shader_bin *shader = pipeline->base.shaders[MESA_SHADER_MESH];
            anv_batch_emit(&cmd_buffer->batch, GENX(3DSTATE_MESH_SHADER_DATA), data) {
      const struct anv_push_range *range = &bind_map->push_ranges[0];
   if (range->length > 0) {
                     uint64_t addr = anv_address_physical(buffer);
                  memcpy(&data.InlineData[BRW_TASK_MESH_PUSH_CONSTANTS_START_DW],
                              }
   #endif
      ALWAYS_INLINE void
   genX(batch_emit_pipe_control)(struct anv_batch *batch,
                     {
      genX(batch_emit_pipe_control_write)(batch,
                                    }
      ALWAYS_INLINE void
   genX(batch_emit_pipe_control_write)(struct anv_batch *batch,
                                       {
            #if INTEL_NEEDS_WA_1409600907
      /* Wa_1409600907: "PIPE_CONTROL with Depth Stall Enable bit must
   * be set with any PIPE_CONTROL with Depth Flush Enable bit set.
   */
   if (bits & ANV_PIPE_DEPTH_CACHE_FLUSH_BIT)
      #endif
            #if GFX_VERx10 >= 125
         pipe.UntypedDataPortCacheFlushEnable =
         #endif
   #if GFX_VER >= 12
         #endif
   #if GFX_VER > 11
         #endif
         pipe.DepthCacheFlushEnable = bits & ANV_PIPE_DEPTH_CACHE_FLUSH_BIT;
   pipe.DCFlushEnable = bits & ANV_PIPE_DATA_CACHE_FLUSH_BIT;
   pipe.RenderTargetCacheFlushEnable =
               #if GFX_VERx10 >= 125
         #endif
         pipe.CommandStreamerStallEnable = bits & ANV_PIPE_CS_STALL_BIT;
            pipe.StateCacheInvalidationEnable =
         pipe.ConstantCacheInvalidationEnable =
   #if GFX_VER >= 12
         /* Invalidates the L3 cache part in which index & vertex data is loaded
   * when VERTEX_BUFFER_STATE::L3BypassDisable is set.
   */
   pipe.L3ReadOnlyCacheInvalidationEnable =
   #endif
         pipe.VFCacheInvalidationEnable =
         pipe.TextureCacheInvalidationEnable =
         pipe.InstructionCacheInvalidateEnable =
            pipe.PostSyncOperation = post_sync_op;
   pipe.Address = address;
   pipe.DestinationAddressType = DAT_PPGTT;
                  }
      /* Set preemption on/off. */
   void
   genX(batch_set_preemption)(struct anv_batch *batch,
               {
   #if GFX_VERx10 >= 120
      anv_batch_write_reg(batch, GENX(CS_CHICKEN1), cc1) {
      cc1.DisablePreemptionandHighPriorityPausingdueto3DPRIMITIVECommand = !value;
               /* Wa_16013994831 - we need to insert CS_STALL and 250 noops. */
            for (unsigned i = 0; i < 250; i++)
      #endif
   }
      void
   genX(cmd_buffer_set_preemption)(struct anv_cmd_buffer *cmd_buffer, bool value)
   {
   #if GFX_VERx10 >= 120
      if (cmd_buffer->state.gfx.object_preemption == value)
            genX(batch_set_preemption)(&cmd_buffer->batch, cmd_buffer->device->info,
            #endif
   }
      ALWAYS_INLINE static void
   genX(emit_hs)(struct anv_cmd_buffer *cmd_buffer)
   {
      struct anv_graphics_pipeline *pipeline = cmd_buffer->state.gfx.pipeline;
   if (!anv_pipeline_has_stage(pipeline, MESA_SHADER_TESS_EVAL))
               }
      ALWAYS_INLINE static void
   genX(emit_ds)(struct anv_cmd_buffer *cmd_buffer)
   {
   #if GFX_VERx10 >= 125
      /* Wa_14019750404:
   *   In any 3D enabled context, just before any Tessellation enabled draw
   *   call (3D Primitive), re-send the last programmed 3DSTATE_DS again.
   *   This will make sure that the 3DSTATE_INT generated just before the
   *   draw call will have TDS dirty which will make sure TDS will launch the
   *   state thread before the draw call.
   *
   * This fixes a hang resulting from running anything using tessellation
   * after a switch away from the mesh pipeline.
   * We don't need to track said switch, as it matters at the HW level, and
   * can be triggered even across processes, so we apply the Wa at all times.
   *
   * FIXME: Use INTEL_NEEDS_WA_14019750404 once the tool picks it up.
   */
   struct anv_graphics_pipeline *pipeline = cmd_buffer->state.gfx.pipeline;
   if (!anv_pipeline_has_stage(pipeline, MESA_SHADER_TESS_EVAL))
               #endif
   }
      ALWAYS_INLINE static void
   genX(cmd_buffer_flush_gfx_state)(struct anv_cmd_buffer *cmd_buffer)
   {
      struct anv_graphics_pipeline *pipeline = cmd_buffer->state.gfx.pipeline;
   const struct vk_dynamic_graphics_state *dyn =
                                                      /* Wa_14015814527
   *
   * Apply task URB workaround when switching from task to primitive.
   */
   if (cmd_buffer->state.gfx.dirty & ANV_CMD_DIRTY_PIPELINE) {
      if (anv_pipeline_is_primitive(pipeline)) {
         } else if (anv_pipeline_has_stage(cmd_buffer->state.gfx.pipeline,
                           /* Apply any pending pipeline flushes we may have.  We want to apply them
   * now because, if any of those flushes are for things like push constants,
   * the GPU will read the state at weird times.
   */
            /* Check what vertex buffers have been rebound against the set of bindings
   * being used by the current set of vertex attributes.
   */
   uint32_t vb_emit = cmd_buffer->state.gfx.vb_dirty & dyn->vi->bindings_valid;
   /* If the pipeline changed, the we have to consider all the valid bindings. */
   if ((cmd_buffer->state.gfx.dirty & ANV_CMD_DIRTY_PIPELINE) ||
      BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_VI_BINDING_STRIDES))
         if (vb_emit) {
      const uint32_t num_buffers = __builtin_popcount(vb_emit);
            p = anv_batch_emitn(&cmd_buffer->batch, num_dwords,
         uint32_t i = 0;
   u_foreach_bit(vb, vb_emit) {
                     struct GENX(VERTEX_BUFFER_STATE) state;
   if (buffer) {
                                       .MOCS = anv_mocs(cmd_buffer->device, buffer->address.bo,
         .AddressModifyEnable = true,
      #if GFX_VER >= 12
         #endif
                           } else {
      state = (struct GENX(VERTEX_BUFFER_STATE)) {
      .VertexBufferIndex = vb,
   .NullVertexBuffer = true,
   .MOCS = anv_mocs(cmd_buffer->device, NULL,
         #if GFX_VER == 9
            genX(cmd_buffer_set_binding_for_gfx8_vb_flush)(cmd_buffer, vb,
      #endif
               GENX(VERTEX_BUFFER_STATE_pack)(&cmd_buffer->batch, &p[1 + i * 4], &state);
                           /* If patch control points value is changed, let's just update the push
   * constant data. If the current pipeline also use this, we need to reemit
   * the 3DSTATE_CONSTANT packet.
   */
   struct anv_push_constants *push = &cmd_buffer->state.gfx.base.push_constants;
   if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_TS_PATCH_CONTROL_POINTS) &&
      push->gfx.tcs_input_vertices != dyn->ts.patch_control_points) {
   push->gfx.tcs_input_vertices = dyn->ts.patch_control_points;
   if (pipeline->dynamic_patch_control_points)
               const bool any_dynamic_state_dirty =
         uint32_t descriptors_dirty = cmd_buffer->state.descriptors_dirty &
            const uint32_t push_descriptor_dirty =
      cmd_buffer->state.push_descriptors_dirty &
      if (push_descriptor_dirty) {
      flush_push_descriptor_set(cmd_buffer,
               descriptors_dirty |= push_descriptor_dirty;
               /* Wa_1306463417, Wa_16011107343 - Send HS state for every primitive. */
   if (cmd_buffer->state.gfx.dirty & ANV_CMD_DIRTY_PIPELINE ||
      (INTEL_NEEDS_WA_1306463417 || INTEL_NEEDS_WA_16011107343)) {
               if (!cmd_buffer->state.gfx.dirty && !descriptors_dirty &&
      !any_dynamic_state_dirty &&
   ((cmd_buffer->state.push_constants_dirty &
      (VK_SHADER_STAGE_ALL_GRAPHICS |
   VK_SHADER_STAGE_TASK_BIT_EXT |
            if (cmd_buffer->state.gfx.dirty & ANV_CMD_DIRTY_XFB_ENABLE) {
      /* Wa_16011411144:
   *
   * SW must insert a PIPE_CONTROL cmd before and after the
   * 3dstate_so_buffer_index_0/1/2/3 states to ensure so_buffer_index_*
   * state is not combined with other state changes.
   */
   if (intel_needs_workaround(cmd_buffer->device->info, 16011411144)) {
      anv_add_pending_pipe_bits(cmd_buffer,
                           /* We don't need any per-buffer dirty tracking because you're not
   * allowed to bind different XFB buffers while XFB is enabled.
   */
   for (unsigned idx = 0; idx < MAX_XFB_BUFFERS; idx++) {
         #if GFX_VER < 12
         #else
               #endif
                  if (cmd_buffer->state.xfb_enabled && xfb->buffer && xfb->size != 0) {
      sob.MOCS = anv_mocs(cmd_buffer->device, xfb->buffer->address.bo,
         sob.SurfaceBaseAddress = anv_address_add(xfb->buffer->address,
         sob.SOBufferEnable = true;
   sob.StreamOffsetWriteEnable = false;
   /* Size is in DWords - 1 */
      } else {
                        if (intel_needs_workaround(cmd_buffer->device->info, 16011411144)) {
      /* Wa_16011411144: also CS_STALL after touching SO_BUFFER change */
   anv_add_pending_pipe_bits(cmd_buffer,
                  } else if (GFX_VER >= 10) {
      /* CNL and later require a CS stall after 3DSTATE_SO_BUFFER */
   anv_add_pending_pipe_bits(cmd_buffer,
                        /* Flush the runtime state into the HW state tracking */
   if (cmd_buffer->state.gfx.dirty || any_dynamic_state_dirty)
            /* Flush the HW state into the commmand buffer */
   if (!BITSET_IS_EMPTY(cmd_buffer->state.gfx.dyn_state.dirty))
            /* If the pipeline changed, we may need to re-allocate push constant space
   * in the URB.
   */
   if (cmd_buffer->state.gfx.dirty & ANV_CMD_DIRTY_PIPELINE) {
               /* Also add the relocations (scratch buffers) */
   VkResult result = anv_reloc_list_append(cmd_buffer->batch.relocs,
         if (result != VK_SUCCESS) {
      anv_batch_set_error(&cmd_buffer->batch, result);
                  /* Render targets live in the same binding table as fragment descriptors */
   if (cmd_buffer->state.gfx.dirty & ANV_CMD_DIRTY_RENDER_TARGETS)
            /* We emit the binding tables and sampler tables first, then emit push
   * constants and then finally emit binding table and sampler table
   * pointers.  It has to happen in this order, since emitting the binding
   * tables may change the push constants (in case of storage images). After
   * emitting push constants, on SKL+ we have to emit the corresponding
   * 3DSTATE_BINDING_TABLE_POINTER_* for the push constants to take effect.
   */
   uint32_t dirty = 0;
   if (descriptors_dirty) {
      dirty = flush_descriptor_sets(cmd_buffer,
                                       if (dirty || cmd_buffer->state.push_constants_dirty) {
      /* Because we're pushing UBOs, we have to push whenever either
   * descriptors or push constants is dirty.
   */
   dirty |= cmd_buffer->state.push_constants_dirty &
         cmd_buffer_flush_gfx_push_constants(cmd_buffer,
   #if GFX_VERx10 >= 125
         cmd_buffer_flush_mesh_inline_data(
         #endif
               if (dirty & VK_SHADER_STAGE_ALL_GRAPHICS) {
      cmd_buffer_emit_descriptor_pointers(cmd_buffer,
               /* When we're done, there is no more dirty gfx state. */
      }
      #include "genX_cmd_draw_generated_indirect.h"
      ALWAYS_INLINE static bool
   anv_use_generated_draws(const struct anv_cmd_buffer *cmd_buffer, uint32_t count)
   {
               /* Limit generated draws to pipelines without HS stage. This makes things
   * simpler for implementing Wa_1306463417, Wa_16011107343.
   */
   if ((INTEL_NEEDS_WA_1306463417 || INTEL_NEEDS_WA_16011107343) &&
      anv_pipeline_has_stage(cmd_buffer->state.gfx.pipeline,
                     return device->physical->generated_indirect_draws &&
      }
      VkResult
   genX(BeginCommandBuffer)(
      VkCommandBuffer                             commandBuffer,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
            /* If this is the first vkBeginCommandBuffer, we must *initialize* the
   * command buffer's state. Otherwise, we must *reset* its state. In both
   * cases we reset it.
   *
   * From the Vulkan 1.0 spec:
   *
   *    If a command buffer is in the executable state and the command buffer
   *    was allocated from a command pool with the
   *    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT flag set, then
   *    vkBeginCommandBuffer implicitly resets the command buffer, behaving
   *    as if vkResetCommandBuffer had been called with
   *    VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT not set. It then puts
   *    the command buffer in the recording state.
   */
   anv_cmd_buffer_reset(&cmd_buffer->vk, 0);
                     /* VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT must be ignored for
   * primary level command buffers.
   *
   * From the Vulkan 1.0 spec:
   *
   *    VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT specifies that a
   *    secondary command buffer is considered to be entirely inside a render
   *    pass. If this is a primary command buffer, then this bit is ignored.
   */
   if (cmd_buffer->vk.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY)
         #if GFX_VER >= 12
      /* Reenable prefetching at the beginning of secondary command buffers. We
   * do this so that the return instruction edition is not prefetched before
   * completion.
   */
   if (cmd_buffer->vk.level == VK_COMMAND_BUFFER_LEVEL_SECONDARY) {
      anv_batch_emit(&cmd_buffer->batch, GENX(MI_ARB_CHECK), arb) {
      arb.PreParserDisableMask = true;
            #endif
                  if (anv_cmd_buffer_is_video_queue(cmd_buffer) ||
      anv_cmd_buffer_is_blitter_queue(cmd_buffer))
                  /* We sometimes store vertex data in the dynamic state buffer for blorp
   * operations and our dynamic state stream may re-use data from previous
   * command buffers.  In order to prevent stale cache data, we flush the VF
   * cache.  We could do this on every blorp call but that's not really
   * needed as all of the data will get written by the CPU prior to the GPU
   * executing anything.  The chances are fairly high that they will use
   * blorp at least once per primary command buffer so it shouldn't be
   * wasted.
   *
   * There is also a workaround on gfx8 which requires us to invalidate the
   * VF cache occasionally.  It's easier if we can assume we start with a
   * fresh cache (See also genX(cmd_buffer_set_binding_for_gfx8_vb_flush).)
   */
   anv_add_pending_pipe_bits(cmd_buffer,
                  /* Re-emit the aux table register in every command buffer.  This way we're
   * ensured that we have the table even if this command buffer doesn't
   * initialize any images.
   */
   if (cmd_buffer->device->info->has_aux_map) {
      anv_add_pending_pipe_bits(cmd_buffer,
                     /* We send an "Indirect State Pointers Disable" packet at
   * EndCommandBuffer, so all push constant packets are ignored during a
   * context restore. Documentation says after that command, we need to
   * emit push constants again before any rendering operation. So we
   * flag them dirty here to make sure they get emitted.
   */
            if (cmd_buffer->usage_flags &
      VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT) {
            char gcbiar_data[VK_GCBIARR_DATA_SIZE(MAX_RTS)];
   const VkRenderingInfo *resume_info =
      vk_get_command_buffer_inheritance_as_rendering_resume(cmd_buffer->vk.level,
            if (resume_info != NULL) {
         } else {
      const VkCommandBufferInheritanceRenderingInfo *inheritance_info =
      vk_get_command_buffer_inheritance_rendering_info(cmd_buffer->vk.level,
               gfx->rendering_flags = inheritance_info->flags;
   gfx->render_area = (VkRect2D) { };
   gfx->layer_count = 0;
                  uint32_t color_att_count = inheritance_info->colorAttachmentCount;
   result = anv_cmd_buffer_init_attachments(cmd_buffer, color_att_count);
                  for (uint32_t i = 0; i < color_att_count; i++) {
      gfx->color_att[i].vk_format =
      }
   gfx->depth_att.vk_format =
                                 cmd_buffer->state.gfx.dirty |= ANV_CMD_DIRTY_RENDER_AREA |
                  /* Emit the sample pattern at the beginning of the batch because the
   * default locations emitted at the device initialization might have been
   * changed by a previous command buffer.
   *
   * Do not change that when we're continuing a previous renderpass.
   */
   if (cmd_buffer->device->vk.enabled_extensions.EXT_sample_locations &&
      !(cmd_buffer->usage_flags & VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT))
         if (cmd_buffer->vk.level == VK_COMMAND_BUFFER_LEVEL_SECONDARY) {
      const VkCommandBufferInheritanceConditionalRenderingInfoEXT *conditional_rendering_info =
            /* If secondary buffer supports conditional rendering
   * we should emit commands as if conditional rendering is enabled.
   */
   cmd_buffer->state.conditional_render_enabled =
            if (pBeginInfo->pInheritanceInfo->occlusionQueryEnable) {
      cmd_buffer->state.gfx.n_occlusion_queries = 1;
                     }
      /* From the PRM, Volume 2a:
   *
   *    "Indirect State Pointers Disable
   *
   *    At the completion of the post-sync operation associated with this pipe
   *    control packet, the indirect state pointers in the hardware are
   *    considered invalid; the indirect pointers are not saved in the context.
   *    If any new indirect state commands are executed in the command stream
   *    while the pipe control is pending, the new indirect state commands are
   *    preserved.
   *
   *    [DevIVB+]: Using Invalidate State Pointer (ISP) only inhibits context
   *    restoring of Push Constant (3DSTATE_CONSTANT_*) commands. Push Constant
   *    commands are only considered as Indirect State Pointers. Once ISP is
   *    issued in a context, SW must initialize by programming push constant
   *    commands for all the shaders (at least to zero length) before attempting
   *    any rendering operation for the same context."
   *
   * 3DSTATE_CONSTANT_* packets are restored during a context restore,
   * even though they point to a BO that has been already unreferenced at
   * the end of the previous batch buffer. This has been fine so far since
   * we are protected by these scratch page (every address not covered by
   * a BO should be pointing to the scratch page). But on CNL, it is
   * causing a GPU hang during context restore at the 3DSTATE_CONSTANT_*
   * instruction.
   *
   * The flag "Indirect State Pointers Disable" in PIPE_CONTROL tells the
   * hardware to ignore previous 3DSTATE_CONSTANT_* packets during a
   * context restore, so the mentioned hang doesn't happen. However,
   * software must program push constant commands for all stages prior to
   * rendering anything. So we flag them dirty in BeginCommandBuffer.
   *
   * Finally, we also make sure to stall at pixel scoreboard to make sure the
   * constants have been loaded into the EUs prior to disable the push constants
   * so that it doesn't hang a previous 3DPRIMITIVE.
   */
   static void
   emit_isp_disable(struct anv_cmd_buffer *cmd_buffer)
   {
      genx_batch_emit_pipe_control(&cmd_buffer->batch,
                     anv_batch_emit(&cmd_buffer->batch, GENX(PIPE_CONTROL), pc) {
         pc.IndirectStatePointersDisable = true;
   pc.CommandStreamerStallEnable = true;
      }
      static VkResult
   end_command_buffer(struct anv_cmd_buffer *cmd_buffer)
   {
      if (anv_batch_has_error(&cmd_buffer->batch))
                     if (anv_cmd_buffer_is_video_queue(cmd_buffer) ||
      anv_cmd_buffer_is_blitter_queue(cmd_buffer)) {
   trace_intel_end_cmd_buffer(&cmd_buffer->trace, cmd_buffer->vk.level);
   anv_cmd_buffer_end_batch_buffer(cmd_buffer);
               /* Flush query clears using blorp so that secondary query writes do not
   * race with the clear.
   */
   if (cmd_buffer->state.queries.clear_bits) {
      anv_add_pending_pipe_bits(cmd_buffer,
                              /* Turn on object level preemption if it is disabled to have it in known
   * state at the beginning of new command buffer.
   */
   if (!cmd_buffer->state.gfx.object_preemption)
            /* We want every command buffer to start with the PMA fix in a known state,
   * so we disable it at the end of the command buffer.
   */
            /* Wa_14015814527
   *
   * Apply task URB workaround in the end of primary or secondary cmd_buffer.
   */
                                                   }
      VkResult
   genX(EndCommandBuffer)(
         {
               VkResult status = end_command_buffer(cmd_buffer);
   if (status != VK_SUCCESS)
            /* If there is MSAA access over the compute/transfer queue, we can use the
   * companion RCS command buffer and end it properly.
   */
   if (cmd_buffer->companion_rcs_cmd_buffer) {
      assert(anv_cmd_buffer_is_compute_queue(cmd_buffer) ||
                        }
      static void
   cmd_buffer_emit_copy_ts_buffer(struct u_trace_context *utctx,
                           {
      struct anv_memcpy_state *memcpy_state = cmdstream;
   struct anv_address from_addr = (struct anv_address) {
         struct anv_address to_addr = (struct anv_address) {
            genX(emit_so_memcpy)(memcpy_state, to_addr, from_addr,
      }
      void
   genX(CmdExecuteCommands)(
      VkCommandBuffer                             commandBuffer,
   uint32_t                                    commandBufferCount,
      {
               if (anv_batch_has_error(&container->batch))
            /* The secondary command buffers will assume that the PMA fix is disabled
   * when they begin executing.  Make sure this is true.
   */
            /* Turn on preemption in case it was toggled off. */
   if (!container->state.gfx.object_preemption)
            /* Wa_14015814527
   *
   * Apply task URB workaround before secondary cmd buffers.
   */
            /* Flush query clears using blorp so that secondary query writes do not
   * race with the clear.
   */
   if (container->state.queries.clear_bits) {
      anv_add_pending_pipe_bits(container,
                     /* The secondary command buffer doesn't know which textures etc. have been
   * flushed prior to their execution.  Apply those flushes now.
   */
                     for (uint32_t i = 0; i < commandBufferCount; i++) {
               assert(secondary->vk.level == VK_COMMAND_BUFFER_LEVEL_SECONDARY);
            if (secondary->state.conditional_render_enabled) {
      if (!container->state.conditional_render_enabled) {
      /* Secondary buffer is constructed as if it will be executed
   * with conditional rendering, we should satisfy this dependency
   * regardless of conditional rendering being enabled in container.
   */
   struct mi_builder b;
   mi_builder_init(&b, container->device->info, &container->batch);
   mi_store(&b, mi_reg64(ANV_PREDICATE_RESULT_REG),
                  if (secondary->usage_flags &
      VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT) {
   /* If we're continuing a render pass from the container, we need to
   * copy the surface states for the current subpass into the storage
   * we allocated for them in BeginCommandBuffer.
   */
   struct anv_state src_state = container->state.gfx.att_states;
                  genX(cmd_buffer_so_memcpy)(
      container,
   anv_state_pool_state_address(&container->device->internal_surface_state_pool,
         anv_state_pool_state_address(&container->device->internal_surface_state_pool,
                           /* Add secondary buffer's RCS command buffer to container buffer's RCS
   * command buffer for execution if secondary RCS is valid.
   */
   if (secondary->companion_rcs_cmd_buffer != NULL) {
      if (container->companion_rcs_cmd_buffer == NULL) {
      VkResult result = anv_create_companion_rcs_command_buffer(container);
   if (result != VK_SUCCESS) {
      anv_batch_set_error(&container->batch, result);
         }
   anv_cmd_buffer_add_secondary(container->companion_rcs_cmd_buffer,
               assert(secondary->perf_query_pool == NULL || container->perf_query_pool == NULL ||
         if (secondary->perf_query_pool)
      #if INTEL_NEEDS_WA_1808121037
         if (secondary->state.depth_reg_mode != ANV_DEPTH_REG_MODE_UNKNOWN)
   #endif
               /* The secondary isn't counted in our VF cache tracking so we need to
   * invalidate the whole thing.
   */
   if (GFX_VER == 9) {
      anv_add_pending_pipe_bits(container,
                     /* The secondary may have selected a different pipeline (3D or compute) and
   * may have changed the current L3$ configuration.  Reset our tracking
   * variables to invalid values to ensure that we re-emit these in the case
   * where we do any draws or compute dispatches from the container after the
   * secondary has returned.
   */
   container->state.current_pipeline = UINT32_MAX;
   container->state.current_l3_config = NULL;
   container->state.current_hash_scale = 0;
   container->state.gfx.push_constant_stages = 0;
   container->state.gfx.ds_write_state = false;
   memcpy(container->state.gfx.dyn_state.dirty,
                  /* Each of the secondary command buffers will use its own state base
   * address.  We need to re-emit state base address for the container after
   * all of the secondaries are done.
   *
   * TODO: Maybe we want to make this a dirty bit to avoid extra state base
   * address calls?
   */
            /* Copy of utrace timestamp buffers from secondary into container */
   struct anv_device *device = container->device;
   if (u_trace_enabled(&device->ds.trace_context)) {
               struct anv_memcpy_state memcpy_state;
   genX(emit_so_memcpy_init)(&memcpy_state, device, &container->batch);
   uint32_t num_traces = 0;
   for (uint32_t i = 0; i < commandBufferCount; i++) {
               num_traces += secondary->trace.num_traces;
   u_trace_clone_append(u_trace_begin_iterator(&secondary->trace),
                        }
                     /* Memcpy is done using the 3D pipeline. */
         }
      static inline enum anv_pipe_bits
   anv_pipe_flush_bits_for_access_flags(struct anv_cmd_buffer *cmd_buffer,
         {
               u_foreach_bit64(b, flags) {
      switch ((VkAccessFlags2)BITFIELD64_BIT(b)) {
   case VK_ACCESS_2_SHADER_WRITE_BIT:
   case VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT:
   case VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR:
      /* We're transitioning a buffer that was previously used as write
   * destination through the data port. To make its content available
   * to future operations, flush the hdc pipeline.
   */
   pipe_bits |= ANV_PIPE_HDC_PIPELINE_FLUSH_BIT;
   pipe_bits |= ANV_PIPE_UNTYPED_DATAPORT_CACHE_FLUSH_BIT;
      case VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT:
      /* We're transitioning a buffer that was previously used as render
   * target. To make its content available to future operations, flush
   * the render target cache.
   */
   pipe_bits |= ANV_PIPE_RENDER_TARGET_CACHE_FLUSH_BIT;
      case VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT:
      /* We're transitioning a buffer that was previously used as depth
   * buffer. To make its content available to future operations, flush
   * the depth cache.
   */
   pipe_bits |= ANV_PIPE_DEPTH_CACHE_FLUSH_BIT;
      case VK_ACCESS_2_TRANSFER_WRITE_BIT:
      /* We're transitioning a buffer that was previously used as a
   * transfer write destination. Generic write operations include color
   * & depth operations as well as buffer operations like :
   *     - vkCmdClearColorImage()
   *     - vkCmdClearDepthStencilImage()
   *     - vkCmdBlitImage()
   *     - vkCmdCopy*(), vkCmdUpdate*(), vkCmdFill*()
   *
   * Most of these operations are implemented using Blorp which writes
   * through the render target cache or the depth cache on the graphics
   * queue. On the compute queue, the writes are done through the data
   * port.
   */
   if (anv_cmd_buffer_is_compute_queue(cmd_buffer)) {
      pipe_bits |= ANV_PIPE_HDC_PIPELINE_FLUSH_BIT;
      } else {
      pipe_bits |= ANV_PIPE_RENDER_TARGET_CACHE_FLUSH_BIT;
      }
      case VK_ACCESS_2_MEMORY_WRITE_BIT:
      /* We're transitioning a buffer for generic write operations. Flush
   * all the caches.
   */
   pipe_bits |= ANV_PIPE_FLUSH_BITS;
      case VK_ACCESS_2_HOST_WRITE_BIT:
      /* We're transitioning a buffer for access by CPU. Invalidate
   * all the caches. Since data and tile caches don't have invalidate,
   * we are forced to flush those as well.
   */
   pipe_bits |= ANV_PIPE_FLUSH_BITS;
   pipe_bits |= ANV_PIPE_INVALIDATE_BITS;
      case VK_ACCESS_2_TRANSFORM_FEEDBACK_WRITE_BIT_EXT:
   case VK_ACCESS_2_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT:
      /* We're transitioning a buffer written either from VS stage or from
   * the command streamer (see CmdEndTransformFeedbackEXT), we just
   * need to stall the CS.
   *
   * Streamout writes apparently bypassing L3, in order to make them
   * visible to the destination, we need to invalidate the other
   * caches.
   */
   pipe_bits |= ANV_PIPE_CS_STALL_BIT | ANV_PIPE_INVALIDATE_BITS;
      default:
                        }
      static inline enum anv_pipe_bits
   anv_pipe_invalidate_bits_for_access_flags(struct anv_cmd_buffer *cmd_buffer,
         {
      struct anv_device *device = cmd_buffer->device;
            u_foreach_bit64(b, flags) {
      switch ((VkAccessFlags2)BITFIELD64_BIT(b)) {
   case VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT:
      /* Indirect draw commands take a buffer as input that we're going to
   * read from the command streamer to load some of the HW registers
   * (see genX_cmd_buffer.c:load_indirect_parameters). This requires a
   * command streamer stall so that all the cache flushes have
   * completed before the command streamer loads from memory.
   */
   pipe_bits |=  ANV_PIPE_CS_STALL_BIT;
   /* Indirect draw commands also set gl_BaseVertex & gl_BaseIndex
   * through a vertex buffer, so invalidate that cache.
   */
   pipe_bits |= ANV_PIPE_VF_CACHE_INVALIDATE_BIT;
   /* For CmdDipatchIndirect, we also load gl_NumWorkGroups through a
   * UBO from the buffer, so we need to invalidate constant cache.
   */
   pipe_bits |= ANV_PIPE_CONSTANT_CACHE_INVALIDATE_BIT;
   pipe_bits |= ANV_PIPE_DATA_CACHE_FLUSH_BIT;
   /* Tile cache flush needed For CmdDipatchIndirect since command
   * streamer and vertex fetch aren't L3 coherent.
   */
   pipe_bits |= ANV_PIPE_TILE_CACHE_FLUSH_BIT;
      case VK_ACCESS_2_INDEX_READ_BIT:
   case VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT:
      /* We transitioning a buffer to be used for as input for vkCmdDraw*
   * commands, so we invalidate the VF cache to make sure there is no
   * stale data when we start rendering.
   */
   pipe_bits |= ANV_PIPE_VF_CACHE_INVALIDATE_BIT;
      case VK_ACCESS_2_UNIFORM_READ_BIT:
   case VK_ACCESS_2_SHADER_BINDING_TABLE_READ_BIT_KHR:
      /* We transitioning a buffer to be used as uniform data. Because
   * uniform is accessed through the data port & sampler, we need to
   * invalidate the texture cache (sampler) & constant cache (data
   * port) to avoid stale data.
   */
   pipe_bits |= ANV_PIPE_CONSTANT_CACHE_INVALIDATE_BIT;
   if (device->physical->compiler->indirect_ubos_use_sampler) {
         } else {
      pipe_bits |= ANV_PIPE_HDC_PIPELINE_FLUSH_BIT;
      }
      case VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT:
   case VK_ACCESS_2_TRANSFER_READ_BIT:
   case VK_ACCESS_2_SHADER_SAMPLED_READ_BIT:
      /* Transitioning a buffer to be read through the sampler, so
   * invalidate the texture cache, we don't want any stale data.
   */
   pipe_bits |= ANV_PIPE_TEXTURE_CACHE_INVALIDATE_BIT;
      case VK_ACCESS_2_SHADER_READ_BIT:
      /* Same as VK_ACCESS_2_UNIFORM_READ_BIT and
   * VK_ACCESS_2_SHADER_SAMPLED_READ_BIT cases above
   */
   pipe_bits |= ANV_PIPE_CONSTANT_CACHE_INVALIDATE_BIT |
         if (!device->physical->compiler->indirect_ubos_use_sampler) {
      pipe_bits |= ANV_PIPE_HDC_PIPELINE_FLUSH_BIT;
      }
      case VK_ACCESS_2_MEMORY_READ_BIT:
      /* Transitioning a buffer for generic read, invalidate all the
   * caches.
   */
   pipe_bits |= ANV_PIPE_INVALIDATE_BITS;
      case VK_ACCESS_2_MEMORY_WRITE_BIT:
      /* Generic write, make sure all previously written things land in
   * memory.
   */
   pipe_bits |= ANV_PIPE_FLUSH_BITS;
      case VK_ACCESS_2_CONDITIONAL_RENDERING_READ_BIT_EXT:
   case VK_ACCESS_2_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT:
      /* Transitioning a buffer for conditional rendering or transform
   * feedback. We'll load the content of this buffer into HW registers
   * using the command streamer, so we need to stall the command
   * streamer , so we need to stall the command streamer to make sure
   * any in-flight flush operations have completed.
   */
   pipe_bits |= ANV_PIPE_CS_STALL_BIT;
   pipe_bits |= ANV_PIPE_TILE_CACHE_FLUSH_BIT;
   pipe_bits |= ANV_PIPE_DATA_CACHE_FLUSH_BIT;
      case VK_ACCESS_2_HOST_READ_BIT:
      /* We're transitioning a buffer that was written by CPU.  Flush
   * all the caches.
   */
   pipe_bits |= ANV_PIPE_FLUSH_BITS;
      case VK_ACCESS_2_TRANSFORM_FEEDBACK_WRITE_BIT_EXT:
      /* We're transitioning a buffer to be written by the streamout fixed
   * function. This one is apparently not L3 coherent, so we need a
   * tile cache flush to make sure any previous write is not going to
   * create WaW hazards.
   */
   pipe_bits |= ANV_PIPE_TILE_CACHE_FLUSH_BIT;
      case VK_ACCESS_2_SHADER_STORAGE_READ_BIT:
   /* VK_ACCESS_2_SHADER_STORAGE_READ_BIT specifies read access to a
   * storage buffer, physical storage buffer, storage texel buffer, or
   * storage image in any shader pipeline stage.
   *
   * Any storage buffers or images written to must be invalidated and
   * flushed before the shader can access them.
   *
   * Both HDC & Untyped flushes also do invalidation. This is why we use
   * this here on Gfx12+.
   *
   * Gfx11 and prior don't have HDC. Only Data cache flush is available
   * and it only operates on the written cache lines.
   */
   if (device->info->ver >= 12) {
      pipe_bits |= ANV_PIPE_UNTYPED_DATAPORT_CACHE_FLUSH_BIT;
      }
   break;
   default:
                        }
      static inline bool
   stage_is_shader(const VkPipelineStageFlags2 stage)
   {
      return (stage & (VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
                  VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT |
   VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT |
   VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT |
   VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
   VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT |
   VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT |
   VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT |
   }
      static inline bool
   stage_is_transfer(const VkPipelineStageFlags2 stage)
   {
      return (stage & (VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT |
      }
      static inline bool
   stage_is_video(const VkPipelineStageFlags2 stage)
   {
         #ifdef VK_ENABLE_BETA_EXTENSIONS
         #endif
         }
      static inline bool
   mask_is_shader_write(const VkAccessFlags2 access)
   {
      return (access & (VK_ACCESS_2_SHADER_WRITE_BIT |
            }
      static inline bool
   mask_is_write(const VkAccessFlags2 access)
   {
      return access & (VK_ACCESS_2_SHADER_WRITE_BIT |
                  VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT |
   VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
   VK_ACCESS_2_TRANSFER_WRITE_BIT |
   VK_ACCESS_2_HOST_WRITE_BIT |
   #ifdef VK_ENABLE_BETA_EXTENSIONS
         #endif
                     VK_ACCESS_2_TRANSFORM_FEEDBACK_WRITE_BIT_EXT |
   VK_ACCESS_2_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT |
   VK_ACCESS_2_COMMAND_PREPROCESS_WRITE_BIT_NV |
   }
      static void
   cmd_buffer_barrier_video(struct anv_cmd_buffer *cmd_buffer,
         {
               bool flush_llc = false;
   bool flush_ccs = false;
   for (uint32_t i = 0; i < dep_info->imageMemoryBarrierCount; i++) {
      const VkImageMemoryBarrier2 *img_barrier =
            ANV_FROM_HANDLE(anv_image, image, img_barrier->image);
            /* If srcQueueFamilyIndex is not equal to dstQueueFamilyIndex, this
   * memory barrier defines a queue family ownership transfer.
   */
   if (img_barrier->srcQueueFamilyIndex != img_barrier->dstQueueFamilyIndex)
            VkImageAspectFlags img_aspects =
         anv_foreach_image_aspect_bit(aspect_bit, image, img_aspects) {
      const uint32_t plane =
         if (isl_aux_usage_has_ccs(image->planes[plane].aux_usage)) {
                        for (uint32_t i = 0; i < dep_info->bufferMemoryBarrierCount; i++) {
      /* Flush the cache if something is written by the video operations and
   * used by any other stages except video encode/decode stages or if
   * srcQueueFamilyIndex is not equal to dstQueueFamilyIndex, this memory
   * barrier defines a queue family ownership transfer.
   */
   if ((stage_is_video(dep_info->pBufferMemoryBarriers[i].srcStageMask) &&
      mask_is_write(dep_info->pBufferMemoryBarriers[i].srcAccessMask) &&
   !stage_is_video(dep_info->pBufferMemoryBarriers[i].dstStageMask)) ||
   (dep_info->pBufferMemoryBarriers[i].srcQueueFamilyIndex !=
   dep_info->pBufferMemoryBarriers[i].dstQueueFamilyIndex)) {
   flush_llc = true;
                  for (uint32_t i = 0; i < dep_info->memoryBarrierCount; i++) {
      /* Flush the cache if something is written by the video operations and
   * used by any other stages except video encode/decode stage.
   */
   if (stage_is_video(dep_info->pMemoryBarriers[i].srcStageMask) &&
      mask_is_write(dep_info->pMemoryBarriers[i].srcAccessMask) &&
   !stage_is_video(dep_info->pMemoryBarriers[i].dstStageMask)) {
   flush_llc = true;
                  if (flush_ccs || flush_llc) {
      #if GFX_VERx10 >= 125
         #endif
   #if GFX_VER >= 12
            /* Using this bit on Gfx9 triggers a GPU hang.
   * This is undocumented behavior. Gfx12 seems fine.
   * TODO: check Gfx11
      #endif
               }
      static void
   cmd_buffer_barrier_blitter(struct anv_cmd_buffer *cmd_buffer,
         {
   #if GFX_VERx10 >= 125
               /* The blitter requires an MI_FLUSH_DW command when a buffer transitions
   * from being a destination to a source.
   */
   bool flush_llc = false;
   bool flush_ccs = false;
   for (uint32_t i = 0; i < dep_info->imageMemoryBarrierCount; i++) {
      const VkImageMemoryBarrier2 *img_barrier =
            ANV_FROM_HANDLE(anv_image, image, img_barrier->image);
            /* If srcQueueFamilyIndex is not equal to dstQueueFamilyIndex, this
   * memory barrier defines a queue family transfer operation.
   */
   if (img_barrier->srcQueueFamilyIndex != img_barrier->dstQueueFamilyIndex)
            /* Flush cache if transfer command reads the output of the previous
   * transfer command, ideally we should just wait for the completion but
   * for now just flush the cache to make the data visible.
   */
   if ((img_barrier->oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL ||
            (img_barrier->newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL ||
   img_barrier->newLayout == VK_IMAGE_LAYOUT_GENERAL)) {
               VkImageAspectFlags img_aspects =
         anv_foreach_image_aspect_bit(aspect_bit, image, img_aspects) {
      const uint32_t plane =
         if (isl_aux_usage_has_ccs(image->planes[plane].aux_usage)) {
                        for (uint32_t i = 0; i < dep_info->bufferMemoryBarrierCount; i++) {
      /* Flush the cache if something is written by the transfer command and
   * used by any other stages except transfer stage or if
   * srcQueueFamilyIndex is not equal to dstQueueFamilyIndex, this memory
   * barrier defines a queue family transfer operation.
   */
   if ((stage_is_transfer(dep_info->pBufferMemoryBarriers[i].srcStageMask) &&
      mask_is_write(dep_info->pBufferMemoryBarriers[i].srcAccessMask)) ||
   (dep_info->pBufferMemoryBarriers[i].srcQueueFamilyIndex !=
   dep_info->pBufferMemoryBarriers[i].dstQueueFamilyIndex)) {
   flush_llc = true;
                  for (uint32_t i = 0; i < dep_info->memoryBarrierCount; i++) {
      /* Flush the cache if something is written by the transfer command and
   * used by any other stages except transfer stage.
   */
   if (stage_is_transfer(dep_info->pMemoryBarriers[i].srcStageMask) &&
      mask_is_write(dep_info->pMemoryBarriers[i].srcAccessMask)) {
   flush_llc = true;
                  if (flush_ccs || flush_llc) {
      anv_batch_emit(&cmd_buffer->batch, GENX(MI_FLUSH_DW), fd) {
      fd.FlushCCS = flush_ccs;
            #endif
   }
      static void
   cmd_buffer_barrier(struct anv_cmd_buffer *cmd_buffer,
               {
      if (anv_cmd_buffer_is_video_queue(cmd_buffer)) {
      cmd_buffer_barrier_video(cmd_buffer, dep_info);
               if (anv_cmd_buffer_is_blitter_queue(cmd_buffer)) {
      cmd_buffer_barrier_blitter(cmd_buffer, dep_info);
                        /* XXX: Right now, we're really dumb and just flush whatever categories
   * the app asks for.  One of these days we may make this a bit better
   * but right now that's all the hardware allows for in most areas.
   */
   VkAccessFlags2 src_flags = 0;
                     for (uint32_t i = 0; i < dep_info->memoryBarrierCount; i++) {
      src_flags |= dep_info->pMemoryBarriers[i].srcAccessMask;
            /* Shader writes to buffers that could then be written by a transfer
   * command (including queries).
   */
   if (stage_is_shader(dep_info->pMemoryBarriers[i].srcStageMask) &&
      mask_is_shader_write(dep_info->pMemoryBarriers[i].srcAccessMask) &&
   stage_is_transfer(dep_info->pMemoryBarriers[i].dstStageMask)) {
   cmd_buffer->state.queries.buffer_write_bits |=
               /* There's no way of knowing if this memory barrier is related to sparse
   * buffers! This is pretty horrible.
   */
   if (device->using_sparse && mask_is_write(src_flags))
               for (uint32_t i = 0; i < dep_info->bufferMemoryBarrierCount; i++) {
      const VkBufferMemoryBarrier2 *buf_barrier =
                  src_flags |= buf_barrier->srcAccessMask;
            /* Shader writes to buffers that could then be written by a transfer
   * command (including queries).
   */
   if (stage_is_shader(buf_barrier->srcStageMask) &&
      mask_is_shader_write(buf_barrier->srcAccessMask) &&
   stage_is_transfer(buf_barrier->dstStageMask)) {
   cmd_buffer->state.queries.buffer_write_bits |=
               if (anv_buffer_is_sparse(buffer) && mask_is_write(src_flags))
               for (uint32_t i = 0; i < dep_info->imageMemoryBarrierCount; i++) {
      const VkImageMemoryBarrier2 *img_barrier =
            src_flags |= img_barrier->srcAccessMask;
            ANV_FROM_HANDLE(anv_image, image, img_barrier->image);
            uint32_t base_layer, layer_count;
   if (image->vk.image_type == VK_IMAGE_TYPE_3D) {
      base_layer = 0;
      } else {
      base_layer = range->baseArrayLayer;
      }
   const uint32_t level_count =
            if (range->aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT) {
      transition_depth_buffer(cmd_buffer, image,
                                 if (range->aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT) {
      transition_stencil_buffer(cmd_buffer, image,
                                       if (range->aspectMask & VK_IMAGE_ASPECT_ANY_COLOR_BIT_ANV) {
      VkImageAspectFlags color_aspects =
         anv_foreach_image_aspect_bit(aspect_bit, image, color_aspects) {
      transition_color_buffer(cmd_buffer, image, 1UL << aspect_bit,
                           range->baseMipLevel, level_count,
   base_layer, layer_count,
                  /* Mark image as compressed if the destination layout has untracked
   * writes to the aux surface.
   */
   VkImageAspectFlags aspects =
         anv_foreach_image_aspect_bit(aspect_bit, image, aspects) {
      VkImageAspectFlagBits aspect = 1UL << aspect_bit;
   if (anv_layout_has_untracked_aux_writes(
         device->info,
   image, aspect,
   img_barrier->newLayout,
      for (uint32_t l = 0; l < level_count; l++) {
      set_image_compressed_bit(cmd_buffer, image, aspect,
                                 if (anv_image_is_sparse(image) && mask_is_write(src_flags))
               enum anv_pipe_bits bits =
      anv_pipe_flush_bits_for_access_flags(cmd_buffer, src_flags) |
         /* Our HW implementation of the sparse feature lives in the GAM unit
   * (interface between all the GPU caches and external memory). As a result
   * writes to NULL bound images & buffers that should be ignored are
   * actually still visible in the caches. The only way for us to get correct
   * NULL bound regions to return 0s is to evict the caches to force the
   * caches to be repopulated with 0s.
   */
   if (apply_sparse_flushes)
            if (dst_flags & VK_ACCESS_INDIRECT_COMMAND_READ_BIT)
               }
      void genX(CmdPipelineBarrier2)(
      VkCommandBuffer                             commandBuffer,
      {
                  }
      void
   genX(batch_emit_breakpoint)(struct anv_batch *batch,
               {
      /* Update draw call count once */
   uint32_t draw_count = emit_before_draw ?
                  if (((draw_count == intel_debug_bkp_before_draw_count &&
      emit_before_draw) ||
   (draw_count == intel_debug_bkp_after_draw_count &&
   !emit_before_draw))) {
   struct anv_address wait_addr =
                  anv_batch_emit(batch, GENX(MI_SEMAPHORE_WAIT), sem) {
      sem.WaitMode            = PollingMode;
   sem.CompareOperation    = COMPARE_SAD_EQUAL_SDD;
   sem.SemaphoreDataDword  = 0x1;
            }
      #if GFX_VER >= 11
   #define _3DPRIMITIVE_DIRECT GENX(3DPRIMITIVE_EXTENDED)
   #else
   #define _3DPRIMITIVE_DIRECT GENX(3DPRIMITIVE)
   #endif
      void genX(CmdDraw)(
      VkCommandBuffer                             commandBuffer,
   uint32_t                                    vertexCount,
   uint32_t                                    instanceCount,
   uint32_t                                    firstVertex,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
            if (anv_batch_has_error(&cmd_buffer->batch))
            const uint32_t count =
         anv_measure_snapshot(cmd_buffer,
                        /* Select pipeline here to allow
   * cmd_buffer_emit_vertex_constants_and_flush() without flushing before
   * cmd_buffer_flush_gfx_state().
   */
            if (cmd_buffer->state.conditional_render_enabled)
         #if GFX_VER < 11
      cmd_buffer_emit_vertex_constants_and_flush(cmd_buffer,
                  #endif
         genX(cmd_buffer_flush_gfx_state)(cmd_buffer);
                     anv_batch_emit(&cmd_buffer->batch, _3DPRIMITIVE_DIRECT, prim) {
      prim.PredicateEnable          = cmd_buffer->state.conditional_render_enabled;
   prim.VertexAccessType         = SEQUENTIAL;
   prim.VertexCountPerInstance   = vertexCount;
   prim.StartVertexLocation      = firstVertex;
   prim.InstanceCount            = instanceCount *
         prim.StartInstanceLocation    = firstInstance;
   #if GFX_VER >= 11
         prim.ExtendedParametersPresent = true;
   prim.ExtendedParameter0       = firstVertex;
   prim.ExtendedParameter1       = firstInstance;
   #endif
                     #if GFX_VERx10 == 125
         #endif
                     }
      void genX(CmdDrawMultiEXT)(
      VkCommandBuffer                             commandBuffer,
   uint32_t                                    drawCount,
   const VkMultiDrawInfoEXT                   *pVertexInfo,
   uint32_t                                    instanceCount,
   uint32_t                                    firstInstance,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
            if (anv_batch_has_error(&cmd_buffer->batch))
                     if (cmd_buffer->state.conditional_render_enabled)
               #if GFX_VER < 11
      vk_foreach_multi_draw(draw, i, pVertexInfo, drawCount, stride) {
      cmd_buffer_emit_vertex_constants_and_flush(cmd_buffer,
                        const uint32_t count =
         anv_measure_snapshot(cmd_buffer,
                                 anv_batch_emit(&cmd_buffer->batch, GENX(3DPRIMITIVE), prim) {
      prim.PredicateEnable          = cmd_buffer->state.conditional_render_enabled;
   prim.VertexAccessType         = SEQUENTIAL;
   prim.VertexCountPerInstance   = draw->vertexCount;
   prim.StartVertexLocation      = draw->firstVertex;
   prim.InstanceCount            = instanceCount *
         prim.StartInstanceLocation    = firstInstance;
               genX(emit_breakpoint)(&cmd_buffer->batch, cmd_buffer->device, false);
         #else
                  /* Wa_1306463417, Wa_16011107343 - Send HS state for every primitive,
   * first one was handled by cmd_buffer_flush_gfx_state.
   */
   if (i && (INTEL_NEEDS_WA_1306463417 || INTEL_NEEDS_WA_16011107343))
                  const uint32_t count = draw->vertexCount * instanceCount;
   anv_measure_snapshot(cmd_buffer,
                                 anv_batch_emit(&cmd_buffer->batch, _3DPRIMITIVE_DIRECT, prim) {
      prim.PredicateEnable          = cmd_buffer->state.conditional_render_enabled;
   prim.VertexAccessType         = SEQUENTIAL;
   prim.VertexCountPerInstance   = draw->vertexCount;
   prim.StartVertexLocation      = draw->firstVertex;
   prim.InstanceCount            = instanceCount;
   prim.StartInstanceLocation    = firstInstance;
   prim.BaseVertexLocation       = 0;
   prim.ExtendedParametersPresent = true;
   prim.ExtendedParameter0       = draw->firstVertex;
   prim.ExtendedParameter1       = firstInstance;
               genX(emit_breakpoint)(&cmd_buffer->batch, cmd_buffer->device, false);
         #endif
      #if GFX_VERx10 == 125
      genX(emit_dummy_post_sync_op)(cmd_buffer,
            #endif
         }
      void genX(CmdDrawIndexed)(
      VkCommandBuffer                             commandBuffer,
   uint32_t                                    indexCount,
   uint32_t                                    instanceCount,
   uint32_t                                    firstIndex,
   int32_t                                     vertexOffset,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
            if (anv_batch_has_error(&cmd_buffer->batch))
            const uint32_t count =
         anv_measure_snapshot(cmd_buffer,
                              /* Select pipeline here to allow
   * cmd_buffer_emit_vertex_constants_and_flush() without flushing before
   * cmd_buffer_flush_gfx_state().
   */
            if (cmd_buffer->state.conditional_render_enabled)
         #if GFX_VER < 11
      const struct brw_vs_prog_data *vs_prog_data = get_vs_prog_data(pipeline);
   cmd_buffer_emit_vertex_constants_and_flush(cmd_buffer, vs_prog_data,
            #endif
         genX(cmd_buffer_flush_gfx_state)(cmd_buffer);
            anv_batch_emit(&cmd_buffer->batch, _3DPRIMITIVE_DIRECT, prim) {
      prim.PredicateEnable          = cmd_buffer->state.conditional_render_enabled;
   prim.VertexAccessType         = RANDOM;
   prim.VertexCountPerInstance   = indexCount;
   prim.StartVertexLocation      = firstIndex;
   prim.InstanceCount            = instanceCount *
         prim.StartInstanceLocation    = firstInstance;
   #if GFX_VER >= 11
         prim.ExtendedParametersPresent = true;
   prim.ExtendedParameter0       = vertexOffset;
   prim.ExtendedParameter1       = firstInstance;
   #endif
                     #if GFX_VERx10 == 125
         #endif
                     }
      void genX(CmdDrawMultiIndexedEXT)(
      VkCommandBuffer                             commandBuffer,
   uint32_t                                    drawCount,
   const VkMultiDrawIndexedInfoEXT            *pIndexInfo,
   uint32_t                                    instanceCount,
   uint32_t                                    firstInstance,
   uint32_t                                    stride,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
            if (anv_batch_has_error(&cmd_buffer->batch))
                     if (cmd_buffer->state.conditional_render_enabled)
               #if GFX_VER < 11
      const struct brw_vs_prog_data *vs_prog_data = get_vs_prog_data(pipeline);
   if (pVertexOffset) {
      if (vs_prog_data->uses_drawid) {
      bool emitted = true;
   if (vs_prog_data->uses_firstvertex ||
      vs_prog_data->uses_baseinstance) {
   emit_base_vertex_instance(cmd_buffer, *pVertexOffset, firstInstance);
      }
   vk_foreach_multi_draw_indexed(draw, i, pIndexInfo, drawCount, stride) {
      if (vs_prog_data->uses_drawid) {
      emit_draw_index(cmd_buffer, i);
      }
   /* Emitting draw index or vertex index BOs may result in needing
   * additional VF cache flushes.
   */
                  const uint32_t count =
         anv_measure_snapshot(cmd_buffer,
                     trace_intel_begin_draw_indexed_multi(&cmd_buffer->trace);
                  anv_batch_emit(&cmd_buffer->batch, GENX(3DPRIMITIVE), prim) {
      prim.PredicateEnable          = cmd_buffer->state.conditional_render_enabled;
   prim.VertexAccessType         = RANDOM;
   prim.VertexCountPerInstance   = draw->indexCount;
   prim.StartVertexLocation      = draw->firstIndex;
   prim.InstanceCount            = instanceCount *
         prim.StartInstanceLocation    = firstInstance;
      }
   genX(emit_breakpoint)(&cmd_buffer->batch, cmd_buffer->device,
         trace_intel_end_draw_indexed_multi(&cmd_buffer->trace, count);
         } else {
      if (vs_prog_data->uses_firstvertex ||
      vs_prog_data->uses_baseinstance) {
   emit_base_vertex_instance(cmd_buffer, *pVertexOffset, firstInstance);
   /* Emitting draw index or vertex index BOs may result in needing
   * additional VF cache flushes.
   */
      }
   vk_foreach_multi_draw_indexed(draw, i, pIndexInfo, drawCount, stride) {
      const uint32_t count =
         anv_measure_snapshot(cmd_buffer,
                     trace_intel_begin_draw_indexed_multi(&cmd_buffer->trace);
                  anv_batch_emit(&cmd_buffer->batch, GENX(3DPRIMITIVE), prim) {
      prim.PredicateEnable          = cmd_buffer->state.conditional_render_enabled;
   prim.VertexAccessType         = RANDOM;
   prim.VertexCountPerInstance   = draw->indexCount;
   prim.StartVertexLocation      = draw->firstIndex;
   prim.InstanceCount            = instanceCount *
         prim.StartInstanceLocation    = firstInstance;
      }
   genX(emit_breakpoint)(&cmd_buffer->batch, cmd_buffer->device,
                  } else {
      vk_foreach_multi_draw_indexed(draw, i, pIndexInfo, drawCount, stride) {
      cmd_buffer_emit_vertex_constants_and_flush(cmd_buffer, vs_prog_data,
                  const uint32_t count =
         anv_measure_snapshot(cmd_buffer,
                                    anv_batch_emit(&cmd_buffer->batch, GENX(3DPRIMITIVE), prim) {
      prim.PredicateEnable          = cmd_buffer->state.conditional_render_enabled;
   prim.VertexAccessType         = RANDOM;
   prim.VertexCountPerInstance   = draw->indexCount;
   prim.StartVertexLocation      = draw->firstIndex;
   prim.InstanceCount            = instanceCount *
         prim.StartInstanceLocation    = firstInstance;
      }
   genX(emit_breakpoint)(&cmd_buffer->batch, cmd_buffer->device, false);
            #else
                  /* Wa_1306463417, Wa_16011107343 - Send HS state for every primitive,
   * first one was handled by cmd_buffer_flush_gfx_state.
   */
   if (i && (INTEL_NEEDS_WA_1306463417 || INTEL_NEEDS_WA_16011107343))
                  const uint32_t count =
         anv_measure_snapshot(cmd_buffer,
                     trace_intel_begin_draw_indexed_multi(&cmd_buffer->trace);
            anv_batch_emit(&cmd_buffer->batch, GENX(3DPRIMITIVE_EXTENDED), prim) {
      prim.PredicateEnable          = cmd_buffer->state.conditional_render_enabled;
   prim.VertexAccessType         = RANDOM;
   prim.VertexCountPerInstance   = draw->indexCount;
   prim.StartVertexLocation      = draw->firstIndex;
   prim.InstanceCount            = instanceCount *
         prim.StartInstanceLocation    = firstInstance;
   prim.BaseVertexLocation       = pVertexOffset ? *pVertexOffset : draw->vertexOffset;
   prim.ExtendedParametersPresent = true;
   prim.ExtendedParameter0       = pVertexOffset ? *pVertexOffset : draw->vertexOffset;
   prim.ExtendedParameter1       = firstInstance;
      }
   genX(emit_breakpoint)(&cmd_buffer->batch, cmd_buffer->device, false);
         #endif
      #if GFX_VERx10 == 125
      genX(emit_dummy_post_sync_op)(cmd_buffer,
            #endif
         }
      /* Auto-Draw / Indirect Registers */
   #define GFX7_3DPRIM_END_OFFSET          0x2420
   #define GFX7_3DPRIM_START_VERTEX        0x2430
   #define GFX7_3DPRIM_VERTEX_COUNT        0x2434
   #define GFX7_3DPRIM_INSTANCE_COUNT      0x2438
   #define GFX7_3DPRIM_START_INSTANCE      0x243C
   #define GFX7_3DPRIM_BASE_VERTEX         0x2440
      /* On Gen11+, we have three custom "extended parameters" which we can use to
   * provide extra system-generated values to shaders.  Our assignment of these
   * is arbitrary; we choose to assign them as follows:
   *
   *    gl_BaseVertex = XP0
   *    gl_BaseInstance = XP1
   *    gl_DrawID = XP2
   *
   * For gl_BaseInstance, we never actually have to set up the value because we
   * can just program 3DSTATE_VF_SGVS_2 to load it implicitly.  We can also do
   * that for gl_BaseVertex but it does the wrong thing for indexed draws.
   */
   #define GEN11_3DPRIM_XP0                0x2690
   #define GEN11_3DPRIM_XP1                0x2694
   #define GEN11_3DPRIM_XP2                0x2698
   #define GEN11_3DPRIM_XP_BASE_VERTEX     GEN11_3DPRIM_XP0
   #define GEN11_3DPRIM_XP_BASE_INSTANCE   GEN11_3DPRIM_XP1
   #define GEN11_3DPRIM_XP_DRAW_ID         GEN11_3DPRIM_XP2
      void genX(CmdDrawIndirectByteCountEXT)(
      VkCommandBuffer                             commandBuffer,
   uint32_t                                    instanceCount,
   uint32_t                                    firstInstance,
   VkBuffer                                    counterBuffer,
   VkDeviceSize                                counterBufferOffset,
   uint32_t                                    counterOffset,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   ANV_FROM_HANDLE(anv_buffer, counter_buffer, counterBuffer);
            /* firstVertex is always zero for this draw function */
            if (anv_batch_has_error(&cmd_buffer->batch))
            anv_measure_snapshot(cmd_buffer,
                              /* Select pipeline here to allow
   * cmd_buffer_emit_vertex_constants_and_flush() without flushing before
   * emit_base_vertex_instance() & emit_draw_index().
   */
            if (cmd_buffer->state.conditional_render_enabled)
         #if GFX_VER < 11
      const struct brw_vs_prog_data *vs_prog_data = get_vs_prog_data(pipeline);
   if (vs_prog_data->uses_firstvertex ||
      vs_prog_data->uses_baseinstance)
      if (vs_prog_data->uses_drawid)
      #endif
                  struct mi_builder b;
   mi_builder_init(&b, cmd_buffer->device->info, &cmd_buffer->batch);
   const uint32_t mocs = anv_mocs_for_address(cmd_buffer->device, &counter_buffer->address);
   mi_builder_set_mocs(&b, mocs);
   struct mi_value count =
      mi_mem32(anv_address_add(counter_buffer->address,
      if (counterOffset)
         count = mi_udiv32_imm(&b, count, vertexStride);
            mi_store(&b, mi_reg32(GFX7_3DPRIM_START_VERTEX), mi_imm(firstVertex));
   mi_store(&b, mi_reg32(GFX7_3DPRIM_INSTANCE_COUNT),
         mi_store(&b, mi_reg32(GFX7_3DPRIM_START_INSTANCE), mi_imm(firstInstance));
         #if GFX_VER >= 11
      mi_store(&b, mi_reg32(GEN11_3DPRIM_XP_BASE_VERTEX),
         /* GEN11_3DPRIM_XP_BASE_INSTANCE is implicit */
      #endif
         genX(emit_breakpoint)(&cmd_buffer->batch, cmd_buffer->device, true);
   anv_batch_emit(&cmd_buffer->batch, _3DPRIMITIVE_DIRECT, prim) {
      prim.IndirectParameterEnable  = true;
   prim.PredicateEnable          = cmd_buffer->state.conditional_render_enabled;
   #if GFX_VER >= 11
         #endif
                  #if GFX_VERx10 == 125
         #endif
                  trace_intel_end_draw_indirect_byte_count(&cmd_buffer->trace,
      }
      static void
   load_indirect_parameters(struct anv_cmd_buffer *cmd_buffer,
                     {
               struct mi_builder b;
   mi_builder_init(&b, cmd_buffer->device->info, &cmd_buffer->batch);
   const uint32_t mocs = anv_mocs_for_address(cmd_buffer->device, &addr);
            mi_store(&b, mi_reg32(GFX7_3DPRIM_VERTEX_COUNT),
            struct mi_value instance_count = mi_mem32(anv_address_add(addr, 4));
   if (pipeline->instance_multiplier > 1) {
      instance_count = mi_imul_imm(&b, instance_count,
      }
            mi_store(&b, mi_reg32(GFX7_3DPRIM_START_VERTEX),
            if (indexed) {
      mi_store(&b, mi_reg32(GFX7_3DPRIM_BASE_VERTEX),
         mi_store(&b, mi_reg32(GFX7_3DPRIM_START_INSTANCE),
   #if GFX_VER >= 11
         mi_store(&b, mi_reg32(GEN11_3DPRIM_XP_BASE_VERTEX),
         #endif
      } else {
      mi_store(&b, mi_reg32(GFX7_3DPRIM_START_INSTANCE),
         #if GFX_VER >= 11
         mi_store(&b, mi_reg32(GEN11_3DPRIM_XP_BASE_VERTEX),
         #endif
            #if GFX_VER >= 11
      mi_store(&b, mi_reg32(GEN11_3DPRIM_XP_DRAW_ID),
      #endif
   }
      static void
   emit_indirect_draws(struct anv_cmd_buffer *cmd_buffer,
                     struct anv_address indirect_data_addr,
   {
   #if GFX_VER < 11
      struct anv_graphics_pipeline *pipeline = cmd_buffer->state.gfx.pipeline;
      #endif
                  if (cmd_buffer->state.conditional_render_enabled)
            uint32_t offset = 0;
   for (uint32_t i = 0; i < draw_count; i++) {
         #if GFX_VER < 11
                  /* With sequential draws, we're dealing with the VkDrawIndirectCommand
   * structure data. We want to load VkDrawIndirectCommand::firstVertex at
   * offset 8 in the structure.
   *
   * With indexed draws, we're dealing with VkDrawIndexedIndirectCommand.
   * We want the VkDrawIndirectCommand::vertexOffset field at offset 12 in
   * the structure.
   */
   if (vs_prog_data->uses_firstvertex ||
      vs_prog_data->uses_baseinstance) {
   emit_base_vertex_instance_bo(cmd_buffer,
      }
   if (vs_prog_data->uses_drawid)
   #endif
            /* Emitting draw index or vertex index BOs may result in needing
   * additional VF cache flushes.
   */
                     /* Wa_1306463417, Wa_16011107343 - Send HS state for every primitive,
   * first one was handled by cmd_buffer_flush_gfx_state.
   */
   if (i && (INTEL_NEEDS_WA_1306463417 || INTEL_NEEDS_WA_16011107343))
                  genX(emit_breakpoint)(&cmd_buffer->batch, cmd_buffer->device, true);
   anv_batch_emit(&cmd_buffer->batch, _3DPRIMITIVE_DIRECT, prim) {
      prim.IndirectParameterEnable  = true;
      #if GFX_VER >= 11
         #endif
                  #if GFX_VERx10 == 125
         #endif
            update_dirty_vbs_for_gfx8_vb_flush(cmd_buffer,
                  }
      void genX(CmdDrawIndirect)(
      VkCommandBuffer                             commandBuffer,
   VkBuffer                                    _buffer,
   VkDeviceSize                                offset,
   uint32_t                                    drawCount,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
            if (anv_batch_has_error(&cmd_buffer->batch))
            anv_measure_snapshot(cmd_buffer,
                              if (anv_use_generated_draws(cmd_buffer, drawCount)) {
      genX(cmd_buffer_emit_indirect_generated_draws)(
      cmd_buffer,
   anv_address_add(buffer->address, offset),
   MAX2(stride, sizeof(VkDrawIndirectCommand)),
   ANV_NULL_ADDRESS /* count_addr */,
   drawCount,
   } else {
      emit_indirect_draws(cmd_buffer,
                        }
      void genX(CmdDrawIndexedIndirect)(
      VkCommandBuffer                             commandBuffer,
   VkBuffer                                    _buffer,
   VkDeviceSize                                offset,
   uint32_t                                    drawCount,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
            if (anv_batch_has_error(&cmd_buffer->batch))
            anv_measure_snapshot(cmd_buffer,
                              if (anv_use_generated_draws(cmd_buffer, drawCount)) {
      genX(cmd_buffer_emit_indirect_generated_draws)(
      cmd_buffer,
   anv_address_add(buffer->address, offset),
   MAX2(stride, sizeof(VkDrawIndexedIndirectCommand)),
   ANV_NULL_ADDRESS /* count_addr */,
   drawCount,
   } else {
      emit_indirect_draws(cmd_buffer,
                        }
      static struct mi_value
   prepare_for_draw_count_predicate(struct anv_cmd_buffer *cmd_buffer,
               {
               if (cmd_buffer->state.conditional_render_enabled) {
      ret = mi_new_gpr(b);
      } else {
      /* Upload the current draw count from the draw parameters buffer to
   * MI_PREDICATE_SRC0.
   */
   mi_store(b, mi_reg64(MI_PREDICATE_SRC0), mi_mem32(count_address));
                  }
      static void
   emit_draw_count_predicate(struct anv_cmd_buffer *cmd_buffer,
               {
      /* Upload the index of the current primitive to MI_PREDICATE_SRC1. */
            if (draw_index == 0) {
      anv_batch_emit(&cmd_buffer->batch, GENX(MI_PREDICATE), mip) {
      mip.LoadOperation    = LOAD_LOADINV;
   mip.CombineOperation = COMBINE_SET;
         } else {
      /* While draw_index < draw_count the predicate's result will be
   *  (draw_index == draw_count) ^ TRUE = TRUE
   * When draw_index == draw_count the result is
   *  (TRUE) ^ TRUE = FALSE
   * After this all results will be:
   *  (FALSE) ^ FALSE = FALSE
   */
   anv_batch_emit(&cmd_buffer->batch, GENX(MI_PREDICATE), mip) {
      mip.LoadOperation    = LOAD_LOAD;
   mip.CombineOperation = COMBINE_XOR;
            }
      static void
   emit_draw_count_predicate_with_conditional_render(
                           {
      struct mi_value pred = mi_ult(b, mi_imm(draw_index), max);
               }
      static void
   emit_draw_count_predicate_cond(struct anv_cmd_buffer *cmd_buffer,
                     {
      if (cmd_buffer->state.conditional_render_enabled) {
      emit_draw_count_predicate_with_conditional_render(
      } else {
            }
      static void
   emit_indirect_count_draws(struct anv_cmd_buffer *cmd_buffer,
                           struct anv_address indirect_data_addr,
   {
   #if GFX_VER < 11
      struct anv_graphics_pipeline *pipeline = cmd_buffer->state.gfx.pipeline;
      #endif
                  struct mi_builder b;
   mi_builder_init(&b, cmd_buffer->device->info, &cmd_buffer->batch);
   const uint32_t mocs = anv_mocs_for_address(cmd_buffer->device, &draw_count_addr);
   mi_builder_set_mocs(&b, mocs);
   struct mi_value max =
            for (uint32_t i = 0; i < max_draw_count; i++) {
      struct anv_address draw =
               #if GFX_VER < 11
         if (vs_prog_data->uses_firstvertex ||
      vs_prog_data->uses_baseinstance) {
   emit_base_vertex_instance_bo(cmd_buffer,
      }
   if (vs_prog_data->uses_drawid)
            /* Emitting draw index or vertex index BOs may result in needing
   * additional VF cache flushes.
   */
   #endif
                     /* Wa_1306463417, Wa_16011107343 - Send HS state for every primitive,
   * first one was handled by cmd_buffer_flush_gfx_state.
   */
   if (i && (INTEL_NEEDS_WA_1306463417 || INTEL_NEEDS_WA_16011107343))
                  genX(emit_breakpoint)(&cmd_buffer->batch, cmd_buffer->device, true);
   anv_batch_emit(&cmd_buffer->batch, _3DPRIMITIVE_DIRECT, prim) {
      prim.IndirectParameterEnable  = true;
      #if GFX_VER >= 11
         #endif
                  #if GFX_VERx10 == 125
         #endif
                           }
      void genX(CmdDrawIndirectCount)(
      VkCommandBuffer                             commandBuffer,
   VkBuffer                                    _buffer,
   VkDeviceSize                                offset,
   VkBuffer                                    _countBuffer,
   VkDeviceSize                                countBufferOffset,
   uint32_t                                    maxDrawCount,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   ANV_FROM_HANDLE(anv_buffer, buffer, _buffer);
            if (anv_batch_has_error(&cmd_buffer->batch))
            anv_measure_snapshot(cmd_buffer,
                              struct anv_address indirect_data_address =
         struct anv_address count_address =
                  if (anv_use_generated_draws(cmd_buffer, maxDrawCount)) {
      genX(cmd_buffer_emit_indirect_generated_draws)(
      cmd_buffer,
   indirect_data_address,
   stride,
   count_address,
   maxDrawCount,
   } else {
      emit_indirect_count_draws(cmd_buffer,
                                          }
      void genX(CmdDrawIndexedIndirectCount)(
      VkCommandBuffer                             commandBuffer,
   VkBuffer                                    _buffer,
   VkDeviceSize                                offset,
   VkBuffer                                    _countBuffer,
   VkDeviceSize                                countBufferOffset,
   uint32_t                                    maxDrawCount,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   ANV_FROM_HANDLE(anv_buffer, buffer, _buffer);
            if (anv_batch_has_error(&cmd_buffer->batch))
            anv_measure_snapshot(cmd_buffer,
                              struct anv_address indirect_data_address =
         struct anv_address count_address =
                  if (anv_use_generated_draws(cmd_buffer, maxDrawCount)) {
      genX(cmd_buffer_emit_indirect_generated_draws)(
      cmd_buffer,
   indirect_data_address,
   stride,
   count_address,
   maxDrawCount,
   } else {
      emit_indirect_count_draws(cmd_buffer,
                                             }
      void genX(CmdBeginTransformFeedbackEXT)(
      VkCommandBuffer                             commandBuffer,
   uint32_t                                    firstCounterBuffer,
   uint32_t                                    counterBufferCount,
   const VkBuffer*                             pCounterBuffers,
      {
               assert(firstCounterBuffer < MAX_XFB_BUFFERS);
   assert(counterBufferCount <= MAX_XFB_BUFFERS);
                     /* From the SKL PRM Vol. 2c, SO_WRITE_OFFSET:
   *
   *    "Ssoftware must ensure that no HW stream output operations can be in
   *    process or otherwise pending at the point that the MI_LOAD/STORE
   *    commands are processed. This will likely require a pipeline flush."
   */
   anv_add_pending_pipe_bits(cmd_buffer,
                        for (uint32_t idx = 0; idx < MAX_XFB_BUFFERS; idx++) {
      /* If we have a counter buffer, this is a resume so we need to load the
   * value into the streamout offset register.  Otherwise, this is a begin
   * and we need to reset it to zero.
   */
   if (pCounterBuffers &&
      idx >= firstCounterBuffer &&
   idx - firstCounterBuffer < counterBufferCount &&
   pCounterBuffers[idx - firstCounterBuffer] != VK_NULL_HANDLE) {
   uint32_t cb_idx = idx - firstCounterBuffer;
   ANV_FROM_HANDLE(anv_buffer, counter_buffer, pCounterBuffers[cb_idx]);
                  anv_batch_emit(&cmd_buffer->batch, GENX(MI_LOAD_REGISTER_MEM), lrm) {
      lrm.RegisterAddress  = GENX(SO_WRITE_OFFSET0_num) + idx * 4;
   lrm.MemoryAddress    = anv_address_add(counter_buffer->address,
         } else {
      anv_batch_emit(&cmd_buffer->batch, GENX(MI_LOAD_REGISTER_IMM), lri) {
      lri.RegisterOffset   = GENX(SO_WRITE_OFFSET0_num) + idx * 4;
                     cmd_buffer->state.xfb_enabled = true;
      }
      void genX(CmdEndTransformFeedbackEXT)(
      VkCommandBuffer                             commandBuffer,
   uint32_t                                    firstCounterBuffer,
   uint32_t                                    counterBufferCount,
   const VkBuffer*                             pCounterBuffers,
      {
               assert(firstCounterBuffer < MAX_XFB_BUFFERS);
   assert(counterBufferCount <= MAX_XFB_BUFFERS);
            /* From the SKL PRM Vol. 2c, SO_WRITE_OFFSET:
   *
   *    "Ssoftware must ensure that no HW stream output operations can be in
   *    process or otherwise pending at the point that the MI_LOAD/STORE
   *    commands are processed. This will likely require a pipeline flush."
   */
   anv_add_pending_pipe_bits(cmd_buffer,
                        for (uint32_t cb_idx = 0; cb_idx < counterBufferCount; cb_idx++) {
               /* If we have a counter buffer, this is a resume so we need to load the
   * value into the streamout offset register.  Otherwise, this is a begin
   * and we need to reset it to zero.
   */
   if (pCounterBuffers &&
      cb_idx < counterBufferCount &&
   pCounterBuffers[cb_idx] != VK_NULL_HANDLE) {
   ANV_FROM_HANDLE(anv_buffer, counter_buffer, pCounterBuffers[cb_idx]);
                  anv_batch_emit(&cmd_buffer->batch, GENX(MI_STORE_REGISTER_MEM), srm) {
      srm.MemoryAddress    = anv_address_add(counter_buffer->address,
                                    cmd_buffer->state.xfb_enabled = false;
      }
      #if GFX_VERx10 >= 125
      void
   genX(CmdDrawMeshTasksEXT)(
         VkCommandBuffer commandBuffer,
   uint32_t x,
   uint32_t y,
   {
               if (anv_batch_has_error(&cmd_buffer->batch))
            anv_measure_snapshot(cmd_buffer,
                           /* TODO(mesh): Check if this is not emitting more packets than we need. */
            if (cmd_buffer->state.conditional_render_enabled)
            anv_batch_emit(&cmd_buffer->batch, GENX(3DMESH_3D), m) {
      m.PredicateEnable = cmd_buffer->state.conditional_render_enabled;
   m.ThreadGroupCountX = x;
   m.ThreadGroupCountY = y;
                  }
      #define GFX125_3DMESH_TG_COUNT 0x26F0
   #define GFX10_3DPRIM_XP(n) (0x2690 + (n) * 4) /* n = { 0, 1, 2 } */
      static void
   mesh_load_indirect_parameters_3dmesh_3d(struct anv_cmd_buffer *cmd_buffer,
                           {
      const size_t groupCountXOff = offsetof(VkDrawMeshTasksIndirectCommandEXT, groupCountX);
   const size_t groupCountYOff = offsetof(VkDrawMeshTasksIndirectCommandEXT, groupCountY);
            mi_store(b, mi_reg32(GFX125_3DMESH_TG_COUNT),
            mi_store(b, mi_reg32(GFX10_3DPRIM_XP(1)),
            mi_store(b, mi_reg32(GFX10_3DPRIM_XP(2)),
            if (emit_xp0)
      }
      static void
   emit_indirect_3dmesh_3d(struct anv_batch *batch,
               {
      uint32_t len = GENX(3DMESH_3D_length) + uses_drawid;
   uint32_t *dw = anv_batch_emitn(batch, len, GENX(3DMESH_3D),
                     if (uses_drawid)
      }
      void
   genX(CmdDrawMeshTasksIndirectEXT)(
      VkCommandBuffer                             commandBuffer,
   VkBuffer                                    _buffer,
   VkDeviceSize                                offset,
   uint32_t                                    drawCount,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   ANV_FROM_HANDLE(anv_buffer, buffer, _buffer);
   struct anv_graphics_pipeline *pipeline = cmd_buffer->state.gfx.pipeline;
   const struct brw_task_prog_data *task_prog_data = get_task_prog_data(pipeline);
   const struct brw_mesh_prog_data *mesh_prog_data = get_mesh_prog_data(pipeline);
            if (anv_batch_has_error(&cmd_buffer->batch))
            anv_measure_snapshot(cmd_buffer,
                                    if (cmd_state->conditional_render_enabled)
            bool uses_drawid = (task_prog_data && task_prog_data->uses_drawid) ||
         struct mi_builder b;
            for (uint32_t i = 0; i < drawCount; i++) {
                        emit_indirect_3dmesh_3d(&cmd_buffer->batch,
                           }
      void
   genX(CmdDrawMeshTasksIndirectCountEXT)(
      VkCommandBuffer                             commandBuffer,
   VkBuffer                                    _buffer,
   VkDeviceSize                                offset,
   VkBuffer                                    _countBuffer,
   VkDeviceSize                                countBufferOffset,
   uint32_t                                    maxDrawCount,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   ANV_FROM_HANDLE(anv_buffer, buffer, _buffer);
   ANV_FROM_HANDLE(anv_buffer, count_buffer, _countBuffer);
   struct anv_graphics_pipeline *pipeline = cmd_buffer->state.gfx.pipeline;
   const struct brw_task_prog_data *task_prog_data = get_task_prog_data(pipeline);
            if (anv_batch_has_error(&cmd_buffer->batch))
            anv_measure_snapshot(cmd_buffer,
                                    bool uses_drawid = (task_prog_data && task_prog_data->uses_drawid) ||
            struct mi_builder b;
   mi_builder_init(&b, cmd_buffer->device->info, &cmd_buffer->batch);
   const uint32_t mocs = anv_mocs_for_address(cmd_buffer->device, &count_buffer->address);
            struct mi_value max =
         prepare_for_draw_count_predicate(
            for (uint32_t i = 0; i < maxDrawCount; i++) {
                                                         }
      #endif /* GFX_VERx10 >= 125 */
      void
   genX(cmd_buffer_ensure_cfe_state)(struct anv_cmd_buffer *cmd_buffer,
         {
   #if GFX_VERx10 >= 125
                        if (total_scratch <= comp_state->scratch_size)
            const struct intel_device_info *devinfo = cmd_buffer->device->info;
   anv_batch_emit(&cmd_buffer->batch, GENX(CFE_STATE), cfe) {
      cfe.MaximumNumberofThreads =
            uint32_t scratch_surf = 0xffffffff;
   if (total_scratch > 0) {
      struct anv_bo *scratch_bo =
         anv_scratch_pool_alloc(cmd_buffer->device,
               anv_reloc_list_add_bo(cmd_buffer->batch.relocs,
         scratch_surf =
      anv_scratch_pool_get_surf(cmd_buffer->device,
                                       #else
         #endif
   }
      static void
   genX(cmd_buffer_flush_compute_state)(struct anv_cmd_buffer *cmd_buffer)
   {
      struct anv_cmd_compute_state *comp_state = &cmd_buffer->state.compute;
   struct anv_compute_pipeline *pipeline = comp_state->pipeline;
                                       /* Apply any pending pipeline flushes we may have.  We want to apply them
   * now because, if any of those flushes are for things like push constants,
   * the GPU will read the state at weird times.
   */
               #if GFX_VERx10 < 125
         /* From the Sky Lake PRM Vol 2a, MEDIA_VFE_STATE:
   *
   *    "A stalling PIPE_CONTROL is required before MEDIA_VFE_STATE unless
   *    the only bits that are changed are scoreboard related: Scoreboard
   *    Enable, Scoreboard Type, Scoreboard Mask, Scoreboard * Delta. For
   *    these scoreboard related states, a MEDIA_STATE_FLUSH is
   *    sufficient."
   */
   anv_add_pending_pipe_bits(cmd_buffer,
               #endif
               #if GFX_VERx10 >= 125
         const struct brw_cs_prog_data *prog_data = get_cs_prog_data(pipeline);
   #endif
            /* The workgroup size of the pipeline affects our push constant layout
   * so flag push constants as dirty if we change the pipeline.
   */
               const uint32_t push_descriptor_dirty =
      cmd_buffer->state.push_descriptors_dirty &
      if (push_descriptor_dirty) {
      flush_push_descriptor_set(cmd_buffer,
               cmd_buffer->state.descriptors_dirty |= push_descriptor_dirty;
               if ((cmd_buffer->state.descriptors_dirty & VK_SHADER_STAGE_COMPUTE_BIT) ||
      cmd_buffer->state.compute.pipeline_dirty) {
   flush_descriptor_sets(cmd_buffer,
                        #if GFX_VERx10 < 125
         uint32_t iface_desc_data_dw[GENX(INTERFACE_DESCRIPTOR_DATA_length)];
   struct GENX(INTERFACE_DESCRIPTOR_DATA) desc = {
      .BindingTablePointer =
         .SamplerStatePointer =
      };
            struct anv_state state =
      anv_cmd_buffer_merge_dynamic(cmd_buffer, iface_desc_data_dw,
                     uint32_t size = GENX(INTERFACE_DESCRIPTOR_DATA_length) * sizeof(uint32_t);
   anv_batch_emit(&cmd_buffer->batch,
            mid.InterfaceDescriptorTotalLength        = size;
      #endif
               if (cmd_buffer->state.push_constants_dirty & VK_SHADER_STAGE_COMPUTE_BIT) {
      comp_state->push_data =
      #if GFX_VERx10 < 125
         if (comp_state->push_data.alloc_size) {
      anv_batch_emit(&cmd_buffer->batch, GENX(MEDIA_CURBE_LOAD), curbe) {
      curbe.CURBETotalDataLength    = comp_state->push_data.alloc_size;
         #endif
                                    }
      static void
   anv_cmd_buffer_push_base_group_id(struct anv_cmd_buffer *cmd_buffer,
                     {
      if (anv_batch_has_error(&cmd_buffer->batch))
            struct anv_push_constants *push =
         if (push->cs.base_work_group_id[0] != baseGroupX ||
      push->cs.base_work_group_id[1] != baseGroupY ||
   push->cs.base_work_group_id[2] != baseGroupZ) {
   push->cs.base_work_group_id[0] = baseGroupX;
   push->cs.base_work_group_id[1] = baseGroupY;
                  }
      #if GFX_VERx10 >= 125
      static inline void
   emit_compute_walker(struct anv_cmd_buffer *cmd_buffer,
                     const struct anv_compute_pipeline *pipeline, bool indirect,
   {
      const struct anv_cmd_compute_state *comp_state = &cmd_buffer->state.compute;
   const struct anv_shader_bin *cs_bin = pipeline->cs;
            const struct intel_device_info *devinfo = pipeline->base.device->info;
   const struct brw_cs_dispatch_info dispatch =
            cmd_buffer->last_compute_walker =
      anv_batch_emitn(
      &cmd_buffer->batch,
   GENX(COMPUTE_WALKER_length),
   GENX(COMPUTE_WALKER),
   .IndirectParameterEnable        = indirect,
   .PredicateEnable                = predicate,
   .SIMDSize                       = dispatch.simd_size / 16,
   .IndirectDataStartAddress       = comp_state->push_data.offset,
   .IndirectDataLength             = comp_state->push_data.alloc_size,
   .LocalXMaximum                  = prog_data->local_size[0] - 1,
   .LocalYMaximum                  = prog_data->local_size[1] - 1,
   .LocalZMaximum                  = prog_data->local_size[2] - 1,
   .ThreadGroupIDXDimension        = groupCountX,
   .ThreadGroupIDYDimension        = groupCountY,
   .ThreadGroupIDZDimension        = groupCountZ,
   .ExecutionMask                  = dispatch.right_mask,
   .PostSync                       = {
                  .InterfaceDescriptor            = (struct GENX(INTERFACE_DESCRIPTOR_DATA)) {
      .KernelStartPointer                = cs_bin->kernel.offset,
   .SamplerStatePointer               = cmd_buffer->state.samplers[
         .BindingTablePointer               = cmd_buffer->state.binding_tables[
         /* Typically set to 0 to avoid prefetching on every thread dispatch. */
   .BindingTableEntryCount            = devinfo->verx10 == 125 ?
         .NumberofThreadsinGPGPUThreadGroup = dispatch.threads,
   .SharedLocalMemorySize             = encode_slm_size(
         .PreferredSLMAllocationSize        = preferred_slm_allocation_size(devinfo),
   }
      #else /* #if GFX_VERx10 >= 125 */
      static inline void
   emit_gpgpu_walker(struct anv_cmd_buffer *cmd_buffer,
                     const struct anv_compute_pipeline *pipeline, bool indirect,
   {
               const struct intel_device_info *devinfo = pipeline->base.device->info;
   const struct brw_cs_dispatch_info dispatch =
            anv_batch_emit(&cmd_buffer->batch, GENX(GPGPU_WALKER), ggw) {
      ggw.IndirectParameterEnable      = indirect;
   ggw.PredicateEnable              = predicate;
   ggw.SIMDSize                     = dispatch.simd_size / 16;
   ggw.ThreadDepthCounterMaximum    = 0;
   ggw.ThreadHeightCounterMaximum   = 0;
   ggw.ThreadWidthCounterMaximum    = dispatch.threads - 1;
   ggw.ThreadGroupIDXDimension      = groupCountX;
   ggw.ThreadGroupIDYDimension      = groupCountY;
   ggw.ThreadGroupIDZDimension      = groupCountZ;
   ggw.RightExecutionMask           = dispatch.right_mask;
                  }
      #endif /* #if GFX_VERx10 >= 125 */
      static inline void
   emit_cs_walker(struct anv_cmd_buffer *cmd_buffer,
                  const struct anv_compute_pipeline *pipeline, bool indirect,
      {
   #if GFX_VERx10 >= 125
      emit_compute_walker(cmd_buffer, pipeline, indirect, prog_data, groupCountX,
      #else
      emit_gpgpu_walker(cmd_buffer, pipeline, indirect, prog_data, groupCountX,
      #endif
   }
      void genX(CmdDispatchBase)(
      VkCommandBuffer                             commandBuffer,
   uint32_t                                    baseGroupX,
   uint32_t                                    baseGroupY,
   uint32_t                                    baseGroupZ,
   uint32_t                                    groupCountX,
   uint32_t                                    groupCountY,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   struct anv_compute_pipeline *pipeline = cmd_buffer->state.compute.pipeline;
            anv_cmd_buffer_push_base_group_id(cmd_buffer, baseGroupX,
            if (anv_batch_has_error(&cmd_buffer->batch))
            anv_measure_snapshot(cmd_buffer,
                        INTEL_SNAPSHOT_COMPUTE,
                  if (prog_data->uses_num_work_groups) {
      struct anv_state state =
         uint32_t *sizes = state.map;
   sizes[0] = groupCountX;
   sizes[1] = groupCountY;
   sizes[2] = groupCountZ;
   cmd_buffer->state.compute.num_workgroups =
                  /* The num_workgroups buffer goes in the binding table */
                        if (cmd_buffer->state.conditional_render_enabled)
            emit_cs_walker(cmd_buffer, pipeline, false, prog_data, groupCountX,
            trace_intel_end_compute(&cmd_buffer->trace,
      }
      #define GPGPU_DISPATCHDIMX 0x2500
   #define GPGPU_DISPATCHDIMY 0x2504
   #define GPGPU_DISPATCHDIMZ 0x2508
      void genX(CmdDispatchIndirect)(
      VkCommandBuffer                             commandBuffer,
   VkBuffer                                    _buffer,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   ANV_FROM_HANDLE(anv_buffer, buffer, _buffer);
   struct anv_compute_pipeline *pipeline = cmd_buffer->state.compute.pipeline;
   const struct brw_cs_prog_data *prog_data = get_cs_prog_data(pipeline);
   struct anv_address addr = anv_address_add(buffer->address, offset);
                     anv_measure_snapshot(cmd_buffer,
                              if (prog_data->uses_num_work_groups) {
               /* The num_workgroups buffer goes in the binding table */
                        struct mi_builder b;
            struct mi_value size_x = mi_mem32(anv_address_add(addr, 0));
   struct mi_value size_y = mi_mem32(anv_address_add(addr, 4));
            mi_store(&b, mi_reg32(GPGPU_DISPATCHDIMX), size_x);
   mi_store(&b, mi_reg32(GPGPU_DISPATCHDIMY), size_y);
            if (cmd_buffer->state.conditional_render_enabled)
                        }
      struct anv_state
   genX(cmd_buffer_ray_query_globals)(struct anv_cmd_buffer *cmd_buffer)
   {
   #if GFX_VERx10 >= 125
               struct anv_state state =
      anv_cmd_buffer_alloc_dynamic_state(cmd_buffer,
            struct brw_rt_scratch_layout layout;
   uint32_t stack_ids_per_dss = 2048; /* TODO: can we use a lower value in
               brw_rt_compute_scratch_layout(&layout, device->info,
            const struct GENX(RT_DISPATCH_GLOBALS) rtdg = {
      .MemBaseAddress = (struct anv_address) {
      /* The ray query HW computes offsets from the top of the buffer, so
   * let the address at the end of the buffer.
   */
   .bo = device->ray_query_bo,
      },
   .AsyncRTStackSize = layout.ray_stack_stride / 64,
   .NumDSSRTStacks = layout.stack_ids_per_dss,
   .MaxBVHLevels = BRW_RT_MAX_BVH_LEVELS,
   .Flags = RT_DEPTH_TEST_LESS_EQUAL,
   .ResumeShaderTable = (struct anv_address) {
            };
               #else
         #endif
   }
      #if GFX_VERx10 >= 125
   void
   genX(cmd_buffer_dispatch_kernel)(struct anv_cmd_buffer *cmd_buffer,
                           {
      const struct intel_device_info *devinfo = cmd_buffer->device->info;
   const struct brw_cs_prog_data *cs_prog_data =
                     if (anv_cmd_buffer_is_render_queue(cmd_buffer))
            /* Apply any pending pipeline flushes we may have.  We want to apply them
   * now because, if any of those flushes are for things like push constants,
   * the GPU will read the state at weird times.
   */
            uint32_t indirect_data_size = sizeof(struct brw_kernel_sysvals);
   indirect_data_size += kernel->bin->bind_map.kernel_args_size;
   indirect_data_size = ALIGN(indirect_data_size, 64);
   struct anv_state indirect_data =
      anv_state_stream_alloc(&cmd_buffer->general_state_stream,
               struct brw_kernel_sysvals sysvals = {};
   if (global_size != NULL) {
      for (unsigned i = 0; i < 3; i++)
      } else {
      struct mi_builder b;
            struct anv_address sysvals_addr = {
      .bo = NULL, /* General state buffer is always 0. */
               mi_store(&b, mi_mem32(anv_address_add(sysvals_addr, 0)),
         mi_store(&b, mi_mem32(anv_address_add(sysvals_addr, 4)),
         mi_store(&b, mi_mem32(anv_address_add(sysvals_addr, 8)),
                        void *args_map = indirect_data.map + sizeof(sysvals);
   for (unsigned i = 0; i < kernel->bin->bind_map.kernel_arg_count; i++) {
      struct brw_kernel_arg_desc *arg_desc =
         assert(i < arg_count);
   const struct anv_kernel_arg *arg = &args[i];
   if (arg->is_ptr) {
         } else {
      assert(arg_desc->size <= sizeof(arg->u64));
                  struct brw_cs_dispatch_info dispatch =
            anv_batch_emit(&cmd_buffer->batch, GENX(COMPUTE_WALKER), cw) {
      cw.PredicateEnable                = false;
   cw.SIMDSize                       = dispatch.simd_size / 16;
   cw.IndirectDataStartAddress       = indirect_data.offset;
   cw.IndirectDataLength             = indirect_data.alloc_size;
   cw.LocalXMaximum                  = cs_prog_data->local_size[0] - 1;
   cw.LocalYMaximum                  = cs_prog_data->local_size[1] - 1;
   cw.LocalZMaximum                  = cs_prog_data->local_size[2] - 1;
   cw.ExecutionMask                  = dispatch.right_mask;
            if (global_size != NULL) {
      cw.ThreadGroupIDXDimension     = global_size[0];
   cw.ThreadGroupIDYDimension     = global_size[1];
      } else {
                  struct GENX(INTERFACE_DESCRIPTOR_DATA) idd = {
      .KernelStartPointer = kernel->bin->kernel.offset,
   .NumberofThreadsinGPGPUThreadGroup = dispatch.threads,
   .SharedLocalMemorySize =
            };
               /* We just blew away the compute pipeline state */
      }
      static void
   calc_local_trace_size(uint8_t local_shift[3], const uint32_t global[3])
   {
      unsigned total_shift = 0;
            bool progress;
   do {
      progress = false;
   for (unsigned i = 0; i < 3; i++) {
      assert(global[i] > 0);
   if ((1 << local_shift[i]) < global[i]) {
      progress = true;
   local_shift[i]++;
               if (total_shift == 3)
                  /* Assign whatever's left to x */
      }
      static struct GENX(RT_SHADER_TABLE)
   vk_sdar_to_shader_table(const VkStridedDeviceAddressRegionKHR *region)
   {
      return (struct GENX(RT_SHADER_TABLE)) {
      .BaseAddress = anv_address_from_u64(region->deviceAddress),
         }
      struct trace_params {
      /* If is_sbt_indirect, use indirect_sbts_addr to build RT_DISPATCH_GLOBALS
   * with mi_builder.
   */
   bool is_sbt_indirect;
   const VkStridedDeviceAddressRegionKHR *raygen_sbt;
   const VkStridedDeviceAddressRegionKHR *miss_sbt;
   const VkStridedDeviceAddressRegionKHR *hit_sbt;
            /* A pointer to a VkTraceRaysIndirectCommand2KHR structure */
            /* If is_indirect, use launch_size_addr to program the dispatch size. */
   bool is_launch_size_indirect;
            /* A pointer a uint32_t[3] */
      };
      static struct anv_state
   cmd_buffer_emit_rt_dispatch_globals(struct anv_cmd_buffer *cmd_buffer,
         {
      assert(!params->is_sbt_indirect);
   assert(params->miss_sbt != NULL);
   assert(params->hit_sbt != NULL);
                     struct anv_state rtdg_state =
      anv_cmd_buffer_alloc_dynamic_state(cmd_buffer,
                     struct GENX(RT_DISPATCH_GLOBALS) rtdg = {
      .MemBaseAddress     = (struct anv_address) {
      .bo = rt->scratch.bo,
      },
   .CallStackHandler   = anv_shader_bin_get_bsr(
         .AsyncRTStackSize   = rt->scratch.layout.ray_stack_stride / 64,
   .NumDSSRTStacks     = rt->scratch.layout.stack_ids_per_dss,
   .MaxBVHLevels       = BRW_RT_MAX_BVH_LEVELS,
   .Flags              = RT_DEPTH_TEST_LESS_EQUAL,
   .HitGroupTable      = vk_sdar_to_shader_table(params->hit_sbt),
   .MissGroupTable     = vk_sdar_to_shader_table(params->miss_sbt),
   .SWStackSize        = rt->scratch.layout.sw_stack_size / 64,
   .LaunchWidth        = params->launch_size[0],
   .LaunchHeight       = params->launch_size[1],
   .LaunchDepth        = params->launch_size[2],
      };
               }
      static struct mi_value
   mi_build_sbt_entry(struct mi_builder *b,
               {
      return mi_ior(b,
               mi_iand(b, mi_mem64(anv_address_from_u64(addr_field_addr)),
      }
      static struct anv_state
   cmd_buffer_emit_rt_dispatch_globals_indirect(struct anv_cmd_buffer *cmd_buffer,
         {
               struct anv_state rtdg_state =
      anv_cmd_buffer_alloc_dynamic_state(cmd_buffer,
                     struct GENX(RT_DISPATCH_GLOBALS) rtdg = {
      .MemBaseAddress     = (struct anv_address) {
      .bo = rt->scratch.bo,
      },
   .CallStackHandler   = anv_shader_bin_get_bsr(
         .AsyncRTStackSize   = rt->scratch.layout.ray_stack_stride / 64,
   .NumDSSRTStacks     = rt->scratch.layout.stack_ids_per_dss,
   .MaxBVHLevels       = BRW_RT_MAX_BVH_LEVELS,
   .Flags              = RT_DEPTH_TEST_LESS_EQUAL,
      };
            struct anv_address rtdg_addr =
      anv_state_pool_state_address(
               struct mi_builder b;
   mi_builder_init(&b, cmd_buffer->device->info, &cmd_buffer->batch);
   const uint32_t mocs = anv_mocs_for_address(cmd_buffer->device, &rtdg_addr);
            /* Fill the MissGroupTable, HitGroupTable & CallableGroupTable fields of
   * RT_DISPATCH_GLOBALS using the mi_builder.
   */
   mi_store(&b,
            mi_mem64(
      anv_address_add(
      rtdg_addr,
   mi_build_sbt_entry(&b,
                     params->indirect_sbts_addr +
      mi_store(&b,
            mi_mem64(
      anv_address_add(
      rtdg_addr,
   mi_build_sbt_entry(&b,
                     params->indirect_sbts_addr +
      mi_store(&b,
            mi_mem64(
      anv_address_add(
      rtdg_addr,
   mi_build_sbt_entry(&b,
                     params->indirect_sbts_addr +
            }
      static void
   cmd_buffer_trace_rays(struct anv_cmd_buffer *cmd_buffer,
         {
      struct anv_device *device = cmd_buffer->device;
   struct anv_cmd_ray_tracing_state *rt = &cmd_buffer->state.rt;
            if (anv_batch_has_error(&cmd_buffer->batch))
            /* If we have a known degenerate launch size, just bail */
   if (!params->is_launch_size_indirect &&
      (params->launch_size[0] == 0 ||
   params->launch_size[1] == 0 ||
   params->launch_size[2] == 0))
                  genX(cmd_buffer_config_l3)(cmd_buffer, pipeline->base.l3_config);
                              /* Add these to the reloc list as they're internal buffers that don't
   * actually have relocs to pick them up manually.
   *
   * TODO(RT): This is a bit of a hack
   */
   anv_reloc_list_add_bo(cmd_buffer->batch.relocs,
         anv_reloc_list_add_bo(cmd_buffer->batch.relocs,
            /* Allocate and set up our RT_DISPATCH_GLOBALS */
   struct anv_state rtdg_state =
      params->is_sbt_indirect ?
   cmd_buffer_emit_rt_dispatch_globals_indirect(cmd_buffer, params) :
         assert(rtdg_state.alloc_size >= (BRW_RT_PUSH_CONST_OFFSET +
         assert(GENX(RT_DISPATCH_GLOBALS_length) * 4 <= BRW_RT_PUSH_CONST_OFFSET);
   /* Push constants go after the RT_DISPATCH_GLOBALS */
   memcpy(rtdg_state.map + BRW_RT_PUSH_CONST_OFFSET,
                  struct anv_address rtdg_addr =
      anv_state_pool_state_address(&cmd_buffer->device->dynamic_state_pool,
         uint8_t local_size_log2[3];
   uint32_t global_size[3] = {};
   if (params->is_launch_size_indirect) {
      /* Pick a local size that's probably ok.  We assume most TraceRays calls
   * will use a two-dimensional dispatch size.  Worst case, our initial
   * dispatch will be a little slower than it has to be.
   */
   local_size_log2[0] = 2;
   local_size_log2[1] = 1;
            struct mi_builder b;
   mi_builder_init(&b, cmd_buffer->device->info, &cmd_buffer->batch);
   const uint32_t mocs = anv_mocs_for_address(cmd_buffer->device, &rtdg_addr);
            struct mi_value launch_size[3] = {
      mi_mem32(anv_address_from_u64(params->launch_size_addr + 0)),
   mi_mem32(anv_address_from_u64(params->launch_size_addr + 4)),
               /* Store the original launch size into RT_DISPATCH_GLOBALS */
   mi_store(&b, mi_mem32(anv_address_add(rtdg_addr,
               mi_store(&b, mi_mem32(anv_address_add(rtdg_addr,
               mi_store(&b, mi_mem32(anv_address_add(rtdg_addr,
                  /* Compute the global dispatch size */
   for (unsigned i = 0; i < 3; i++) {
                     /* global_size = DIV_ROUND_UP(launch_size, local_size)
   *
   * Fortunately for us MI_ALU math is 64-bit and , mi_ushr32_imm
   * has the semantics of shifting the enture 64-bit value and taking
   * the bottom 32 so we don't have to worry about roll-over.
   */
   uint32_t local_size = 1 << local_size_log2[i];
   launch_size[i] = mi_iadd(&b, launch_size[i],
         launch_size[i] = mi_ushr32_imm(&b, launch_size[i],
               mi_store(&b, mi_reg32(GPGPU_DISPATCHDIMX), launch_size[0]);
   mi_store(&b, mi_reg32(GPGPU_DISPATCHDIMY), launch_size[1]);
      } else {
               for (unsigned i = 0; i < 3; i++) {
      /* We have to be a bit careful here because DIV_ROUND_UP adds to the
   * numerator value may overflow.  Cast to uint64_t to avoid this.
   */
   uint32_t local_size = 1 << local_size_log2[i];
               #if GFX_VERx10 == 125
      /* Wa_14014427904 - We need additional invalidate/flush when
   * emitting NP state commands with ATS-M in compute mode.
   */
   if (intel_device_info_is_atsm(device->info) &&
      cmd_buffer->queue_family->engine_class == INTEL_ENGINE_CLASS_COMPUTE) {
   genx_batch_emit_pipe_control(&cmd_buffer->batch,
                              cmd_buffer->device->info,
   ANV_PIPE_CS_STALL_BIT |
   ANV_PIPE_STATE_CACHE_INVALIDATE_BIT |
      #endif
         anv_batch_emit(&cmd_buffer->batch, GENX(3DSTATE_BTD), btd) {
      /* TODO: This is the timeout after which the bucketed thread dispatcher
   *       will kick off a wave of threads. We go with the lowest value
   *       for now. It could be tweaked on a per application basis
   *       (drirc).
   */
   btd.DispatchTimeoutCounter = _64clocks;
   /* BSpec 43851: "This field must be programmed to 6h i.e. memory backed
   *               buffer must be 128KB."
   */
   btd.PerDSSMemoryBackedBufferSize = 6;
   btd.MemoryBackedBufferBasePointer = (struct anv_address) { .bo = device->btd_fifo_bo };
   if (pipeline->base.scratch_size > 0) {
      struct anv_bo *scratch_bo =
      anv_scratch_pool_alloc(device,
                  anv_reloc_list_add_bo(cmd_buffer->batch.relocs,
         uint32_t scratch_surf =
      anv_scratch_pool_get_surf(cmd_buffer->device,
                                    const struct brw_cs_prog_data *cs_prog_data =
         struct brw_cs_dispatch_info dispatch =
            anv_batch_emit(&cmd_buffer->batch, GENX(COMPUTE_WALKER), cw) {
      cw.IndirectParameterEnable        = params->is_launch_size_indirect;
   cw.PredicateEnable                = cmd_buffer->state.conditional_render_enabled;
   cw.SIMDSize                       = dispatch.simd_size / 16;
   cw.LocalXMaximum                  = (1 << local_size_log2[0]) - 1;
   cw.LocalYMaximum                  = (1 << local_size_log2[1]) - 1;
   cw.LocalZMaximum                  = (1 << local_size_log2[2]) - 1;
   cw.ThreadGroupIDXDimension        = global_size[0];
   cw.ThreadGroupIDYDimension        = global_size[1];
   cw.ThreadGroupIDZDimension        = global_size[2];
   cw.ExecutionMask                  = 0xff;
   cw.EmitInlineParameter            = true;
            const gl_shader_stage s = MESA_SHADER_RAYGEN;
   struct anv_device *device = cmd_buffer->device;
   struct anv_state *surfaces = &cmd_buffer->state.binding_tables[s];
   struct anv_state *samplers = &cmd_buffer->state.samplers[s];
   cw.InterfaceDescriptor = (struct GENX(INTERFACE_DESCRIPTOR_DATA)) {
      .KernelStartPointer = device->rt_trampoline->kernel.offset,
   .SamplerStatePointer = samplers->offset,
   /* i965: DIV_ROUND_UP(CLAMP(stage_state->sampler_count, 0, 16), 4), */
   .SamplerCount = 0,
   .BindingTablePointer = surfaces->offset,
   .NumberofThreadsinGPGPUThreadGroup = 1,
               struct brw_rt_raygen_trampoline_params trampoline_params = {
      .rt_disp_globals_addr = anv_address_physical(rtdg_addr),
   .raygen_bsr_addr =
      params->is_sbt_indirect ?
   (params->indirect_sbts_addr +
   offsetof(VkTraceRaysIndirectCommand2KHR,
            .is_indirect = params->is_sbt_indirect,
   .local_group_size_log2 = {
      local_size_log2[0],
   local_size_log2[1],
         };
   STATIC_ASSERT(sizeof(trampoline_params) == 32);
               trace_intel_end_rays(&cmd_buffer->trace,
                  }
      void
   genX(CmdTraceRaysKHR)(
      VkCommandBuffer                             commandBuffer,
   const VkStridedDeviceAddressRegionKHR*      pRaygenShaderBindingTable,
   const VkStridedDeviceAddressRegionKHR*      pMissShaderBindingTable,
   const VkStridedDeviceAddressRegionKHR*      pHitShaderBindingTable,
   const VkStridedDeviceAddressRegionKHR*      pCallableShaderBindingTable,
   uint32_t                                    width,
   uint32_t                                    height,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   struct trace_params params = {
      .is_sbt_indirect         = false,
   .raygen_sbt              = pRaygenShaderBindingTable,
   .miss_sbt                = pMissShaderBindingTable,
   .hit_sbt                 = pHitShaderBindingTable,
   .callable_sbt            = pCallableShaderBindingTable,
   .is_launch_size_indirect = false,
   .launch_size             = {
      width,
   height,
                     }
      void
   genX(CmdTraceRaysIndirectKHR)(
      VkCommandBuffer                             commandBuffer,
   const VkStridedDeviceAddressRegionKHR*      pRaygenShaderBindingTable,
   const VkStridedDeviceAddressRegionKHR*      pMissShaderBindingTable,
   const VkStridedDeviceAddressRegionKHR*      pHitShaderBindingTable,
   const VkStridedDeviceAddressRegionKHR*      pCallableShaderBindingTable,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   struct trace_params params = {
      .is_sbt_indirect         = false,
   .raygen_sbt              = pRaygenShaderBindingTable,
   .miss_sbt                = pMissShaderBindingTable,
   .hit_sbt                 = pHitShaderBindingTable,
   .callable_sbt            = pCallableShaderBindingTable,
   .is_launch_size_indirect = true,
                  }
      void
   genX(CmdTraceRaysIndirect2KHR)(
      VkCommandBuffer                             commandBuffer,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   struct trace_params params = {
      .is_sbt_indirect         = true,
   .indirect_sbts_addr      = indirectDeviceAddress,
   .is_launch_size_indirect = true,
   .launch_size_addr        = indirectDeviceAddress +
                  }
      #endif /* GFX_VERx10 >= 125 */
      /* Only emit PIPELINE_SELECT, for the whole mode switch and flushing use
   * flush_pipeline_select()
   */
   void
   genX(emit_pipeline_select)(struct anv_batch *batch, uint32_t pipeline)
   {
      anv_batch_emit(batch, GENX(PIPELINE_SELECT), ps) {
      ps.MaskBits = GFX_VER >= 12 ? 0x13 : 3;
   ps.MediaSamplerDOPClockGateEnable = GFX_VER >= 12;
         }
      static void
   genX(flush_pipeline_select)(struct anv_cmd_buffer *cmd_buffer,
         {
               if (cmd_buffer->state.current_pipeline == pipeline)
         #if GFX_VER == 9
      /* From the Broadwell PRM, Volume 2a: Instructions, PIPELINE_SELECT:
   *
   *   Software must clear the COLOR_CALC_STATE Valid field in
   *   3DSTATE_CC_STATE_POINTERS command prior to send a PIPELINE_SELECT
   *   with Pipeline Select set to GPGPU.
   *
   * The internal hardware docs recommend the same workaround for Gfx9
   * hardware too.
   */
   if (pipeline == GPGPU)
      #endif
      #if GFX_VERx10 == 120
      /* Undocumented workaround to force the re-emission of
   * MEDIA_INTERFACE_DESCRIPTOR_LOAD when switching from 3D to Compute
   * pipeline without rebinding a pipeline :
   *    vkCmdBindPipeline(COMPUTE, cs_pipeline);
   *    vkCmdDispatch(...);
   *    vkCmdBindPipeline(GRAPHICS, gfx_pipeline);
   *    vkCmdDraw(...);
   *    vkCmdDispatch(...);
   */
   if (pipeline == _3D)
      #endif
      #if GFX_VERx10 < 125
      /* We apparently cannot flush the tile cache (color/depth) from the GPGPU
   * pipeline. That means query clears will not be visible to query
   * copy/write. So we need to flush it before going to GPGPU mode.
   */
   if (cmd_buffer->state.current_pipeline == _3D &&
      cmd_buffer->state.queries.clear_bits) {
   anv_add_pending_pipe_bits(cmd_buffer,
               #endif
         /* Flush and invalidate bits done needed prior PIPELINE_SELECT. */
         #if GFX_VER >= 12
      /* From Tigerlake PRM, Volume 2a, PIPELINE_SELECT:
   *
   *   "Software must ensure Render Cache, Depth Cache and HDC Pipeline flush
   *   are flushed through a stalling PIPE_CONTROL command prior to
   *   programming of PIPELINE_SELECT command transitioning Pipeline Select
   *   from 3D to GPGPU/Media.
   *   Software must ensure HDC Pipeline flush and Generic Media State Clear
   *   is issued through a stalling PIPE_CONTROL command prior to programming
   *   of PIPELINE_SELECT command transitioning Pipeline Select from
   *   GPGPU/Media to 3D."
   *
   * Note: Issuing PIPE_CONTROL_MEDIA_STATE_CLEAR causes GPU hangs, probably
   * because PIPE was not in MEDIA mode?!
   */
            if (cmd_buffer->state.current_pipeline == _3D) {
      bits |= ANV_PIPE_RENDER_TARGET_CACHE_FLUSH_BIT |
      } else {
            #else
      /* From "BXML » GT » MI » vol1a GPU Overview » [Instruction]
   * PIPELINE_SELECT [DevBWR+]":
   *
   *   Project: DEVSNB+
   *
   *   Software must ensure all the write caches are flushed through a
   *   stalling PIPE_CONTROL command followed by another PIPE_CONTROL
   *   command to invalidate read only caches prior to programming
   *   MI_PIPELINE_SELECT command to change the Pipeline Select Mode.
   *
   * Note the cmd_buffer_apply_pipe_flushes will split this into two
   * PIPE_CONTROLs.
   */
   bits |= ANV_PIPE_RENDER_TARGET_CACHE_FLUSH_BIT |
         ANV_PIPE_DEPTH_CACHE_FLUSH_BIT |
   ANV_PIPE_HDC_PIPELINE_FLUSH_BIT |
   ANV_PIPE_CS_STALL_BIT |
   ANV_PIPE_TEXTURE_CACHE_INVALIDATE_BIT |
   ANV_PIPE_CONSTANT_CACHE_INVALIDATE_BIT |
   ANV_PIPE_STATE_CACHE_INVALIDATE_BIT |
      #endif
         /* Wa_16013063087 -  State Cache Invalidate must be issued prior to
   * PIPELINE_SELECT when switching from 3D to Compute.
   *
   * SW must do this by programming of PIPECONTROL with “CS Stall” followed by
   * a PIPECONTROL with State Cache Invalidate bit set.
   *
   */
   if (cmd_buffer->state.current_pipeline == _3D && pipeline == GPGPU &&
      intel_needs_workaround(cmd_buffer->device->info, 16013063087))
         anv_add_pending_pipe_bits(cmd_buffer, bits, "flush/invalidate PIPELINE_SELECT");
         #if GFX_VER == 9
      if (pipeline == _3D) {
      /* There is a mid-object preemption workaround which requires you to
   * re-emit MEDIA_VFE_STATE after switching from GPGPU to 3D.  However,
   * even without preemption, we have issues with geometry flickering when
   * GPGPU and 3D are back-to-back and this seems to fix it.  We don't
   * really know why.
   *
   * Also, from the Sky Lake PRM Vol 2a, MEDIA_VFE_STATE:
   *
   *    "A stalling PIPE_CONTROL is required before MEDIA_VFE_STATE unless
   *    the only bits that are changed are scoreboard related ..."
   *
   * This is satisfied by applying pre-PIPELINE_SELECT pipe flushes above.
   */
   anv_batch_emit(&cmd_buffer->batch, GENX(MEDIA_VFE_STATE), vfe) {
      vfe.MaximumNumberofThreads =
         vfe.NumberofURBEntries     = 2;
               /* We just emitted a dummy MEDIA_VFE_STATE so now that packet is
   * invalid. Set the compute pipeline to dirty to force a re-emit of the
   * pipeline in case we get back-to-back dispatch calls with the same
   * pipeline and a PIPELINE_SELECT in between.
   */
         #endif
               #if GFX_VER == 9
      if (devinfo->platform == INTEL_PLATFORM_GLK) {
      /* Project: DevGLK
   *
   * "This chicken bit works around a hardware issue with barrier logic
   *  encountered when switching between GPGPU and 3D pipelines.  To
   *  workaround the issue, this mode bit should be set after a pipeline
   *  is selected."
   */
   anv_batch_write_reg(&cmd_buffer->batch, GENX(SLICE_COMMON_ECO_CHICKEN1), scec1) {
      scec1.GLKBarrierMode = pipeline == GPGPU ? GLK_BARRIER_MODE_GPGPU
                  #endif
            }
      void
   genX(flush_pipeline_select_3d)(struct anv_cmd_buffer *cmd_buffer)
   {
         }
      void
   genX(flush_pipeline_select_gpgpu)(struct anv_cmd_buffer *cmd_buffer)
   {
         }
      void
   genX(cmd_buffer_emit_gfx12_depth_wa)(struct anv_cmd_buffer *cmd_buffer,
         {
   #if INTEL_NEEDS_WA_1808121037
      const bool is_d16_1x_msaa = surf->format == ISL_FORMAT_R16_UNORM &&
            switch (cmd_buffer->state.depth_reg_mode) {
   case ANV_DEPTH_REG_MODE_HW_DEFAULT:
      if (!is_d16_1x_msaa)
            case ANV_DEPTH_REG_MODE_D16_1X_MSAA:
      if (is_d16_1x_msaa)
            case ANV_DEPTH_REG_MODE_UNKNOWN:
                  /* We'll change some CHICKEN registers depending on the depth surface
   * format. Do a depth flush and stall so the pipeline is not using these
   * settings while we change the registers.
   */
   anv_add_pending_pipe_bits(cmd_buffer,
                                    /* Wa_1808121037
   *
   * To avoid sporadic corruptions “Set 0x7010[9] when Depth Buffer
   * Surface Format is D16_UNORM , surface type is not NULL & 1X_MSAA”.
   */
   anv_batch_write_reg(&cmd_buffer->batch, GENX(COMMON_SLICE_CHICKEN1), reg) {
      reg.HIZPlaneOptimizationdisablebit = is_d16_1x_msaa;
               cmd_buffer->state.depth_reg_mode =
      is_d16_1x_msaa ? ANV_DEPTH_REG_MODE_D16_1X_MSAA :
   #endif
   }
      #if GFX_VER == 9
   /* From the Skylake PRM, 3DSTATE_VERTEX_BUFFERS:
   *
   *    "The VF cache needs to be invalidated before binding and then using
   *    Vertex Buffers that overlap with any previously bound Vertex Buffer
   *    (at a 64B granularity) since the last invalidation.  A VF cache
   *    invalidate is performed by setting the "VF Cache Invalidation Enable"
   *    bit in PIPE_CONTROL."
   *
   * This is implemented by carefully tracking all vertex and index buffer
   * bindings and flushing if the cache ever ends up with a range in the cache
   * that would exceed 4 GiB.  This is implemented in three parts:
   *
   *    1. genX(cmd_buffer_set_binding_for_gfx8_vb_flush)() which must be called
   *       every time a 3DSTATE_VERTEX_BUFFER packet is emitted and informs the
   *       tracking code of the new binding.  If this new binding would cause
   *       the cache to have a too-large range on the next draw call, a pipeline
   *       stall and VF cache invalidate are added to pending_pipeline_bits.
   *
   *    2. genX(cmd_buffer_apply_pipe_flushes)() resets the cache tracking to
   *       empty whenever we emit a VF invalidate.
   *
   *    3. genX(cmd_buffer_update_dirty_vbs_for_gfx8_vb_flush)() must be called
   *       after every 3DPRIMITIVE and copies the bound range into the dirty
   *       range for each used buffer.  This has to be a separate step because
   *       we don't always re-bind all buffers and so 1. can't know which
   *       buffers are actually bound.
   */
   void
   genX(cmd_buffer_set_binding_for_gfx8_vb_flush)(struct anv_cmd_buffer *cmd_buffer,
                     {
      if (GFX_VER > 9)
            struct anv_vb_cache_range *bound, *dirty;
   if (vb_index == -1) {
      bound = &cmd_buffer->state.gfx.ib_bound_range;
      } else {
      assert(vb_index >= 0);
   assert(vb_index < ARRAY_SIZE(cmd_buffer->state.gfx.vb_bound_ranges));
   assert(vb_index < ARRAY_SIZE(cmd_buffer->state.gfx.vb_dirty_ranges));
   bound = &cmd_buffer->state.gfx.vb_bound_ranges[vb_index];
               if (anv_gfx8_9_vb_cache_range_needs_workaround(bound, dirty,
                  anv_add_pending_pipe_bits(cmd_buffer,
                     }
      void
   genX(cmd_buffer_update_dirty_vbs_for_gfx8_vb_flush)(struct anv_cmd_buffer *cmd_buffer,
               {
      if (access_type == RANDOM) {
      /* We have an index buffer */
   struct anv_vb_cache_range *bound = &cmd_buffer->state.gfx.ib_bound_range;
                        uint64_t mask = vb_used;
   while (mask) {
      int i = u_bit_scan64(&mask);
   assert(i >= 0);
   assert(i < ARRAY_SIZE(cmd_buffer->state.gfx.vb_bound_ranges));
            struct anv_vb_cache_range *bound, *dirty;
   bound = &cmd_buffer->state.gfx.vb_bound_ranges[i];
                  }
   #endif /* GFX_VER == 9 */
      /**
   * Update the pixel hashing modes that determine the balancing of PS threads
   * across subslices and slices.
   *
   * \param width Width bound of the rendering area (already scaled down if \p
   *              scale is greater than 1).
   * \param height Height bound of the rendering area (already scaled down if \p
   *               scale is greater than 1).
   * \param scale The number of framebuffer samples that could potentially be
   *              affected by an individual channel of the PS thread.  This is
   *              typically one for single-sampled rendering, but for operations
   *              like CCS resolves and fast clears a single PS invocation may
   *              update a huge number of pixels, in which case a finer
   *              balancing is desirable in order to maximally utilize the
   *              bandwidth available.  UINT_MAX can be used as shorthand for
   *              "finest hashing mode available".
   */
   void
   genX(cmd_buffer_emit_hashing_mode)(struct anv_cmd_buffer *cmd_buffer,
               {
   #if GFX_VER == 9
      const struct intel_device_info *devinfo = cmd_buffer->device->info;
   const unsigned slice_hashing[] = {
      /* Because all Gfx9 platforms with more than one slice require
   * three-way subslice hashing, a single "normal" 16x16 slice hashing
   * block is guaranteed to suffer from substantial imbalance, with one
   * subslice receiving twice as much work as the other two in the
   * slice.
   *
   * The performance impact of that would be particularly severe when
   * three-way hashing is also in use for slice balancing (which is the
   * case for all Gfx9 GT4 platforms), because one of the slices
   * receives one every three 16x16 blocks in either direction, which
   * is roughly the periodicity of the underlying subslice imbalance
   * pattern ("roughly" because in reality the hardware's
   * implementation of three-way hashing doesn't do exact modulo 3
   * arithmetic, which somewhat decreases the magnitude of this effect
   * in practice).  This leads to a systematic subslice imbalance
   * within that slice regardless of the size of the primitive.  The
   * 32x32 hashing mode guarantees that the subslice imbalance within a
   * single slice hashing block is minimal, largely eliminating this
   * effect.
   */
   _32x32,
   /* Finest slice hashing mode available. */
      };
   const unsigned subslice_hashing[] = {
      /* 16x16 would provide a slight cache locality benefit especially
   * visible in the sampler L1 cache efficiency of low-bandwidth
   * non-LLC platforms, but it comes at the cost of greater subslice
   * imbalance for primitives of dimensions approximately intermediate
   * between 16x4 and 16x16.
   */
   _16x4,
   /* Finest subslice hashing mode available. */
      };
   /* Dimensions of the smallest hashing block of a given hashing mode.  If
   * the rendering area is smaller than this there can't possibly be any
   * benefit from switching to this mode, so we optimize out the
   * transition.
   */
   const unsigned min_size[][2] = {
         { 16, 4 },
   };
            if (cmd_buffer->state.current_hash_scale != scale &&
      (width > min_size[idx][0] || height > min_size[idx][1])) {
   anv_add_pending_pipe_bits(cmd_buffer,
                              anv_batch_write_reg(&cmd_buffer->batch, GENX(GT_MODE), gt) {
      gt.SliceHashing = (devinfo->num_slices > 1 ? slice_hashing[idx] : 0);
   gt.SliceHashingMask = (devinfo->num_slices > 1 ? -1 : 0);
   gt.SubsliceHashing = subslice_hashing[idx];
                     #endif
   }
      static void
   cmd_buffer_emit_depth_stencil(struct anv_cmd_buffer *cmd_buffer)
   {
      struct anv_device *device = cmd_buffer->device;
            uint32_t *dw = anv_batch_emit_dwords(&cmd_buffer->batch,
         if (dw == NULL)
            struct isl_view isl_view = {};
   struct isl_depth_stencil_hiz_emit_info info = {
      .view = &isl_view,
               if (gfx->depth_att.iview != NULL) {
         } else if (gfx->stencil_att.iview != NULL) {
                  if (gfx->view_mask) {
      assert(isl_view.array_len == 0 ||
            } else {
      assert(isl_view.array_len == 0 ||
                     if (gfx->depth_att.iview != NULL) {
      const struct anv_image_view *iview = gfx->depth_att.iview;
            const uint32_t depth_plane =
         const struct anv_surface *depth_surface =
         const struct anv_address depth_address =
                     info.depth_surf = &depth_surface->isl;
   info.depth_address = anv_address_physical(depth_address);
   info.mocs =
            info.hiz_usage = gfx->depth_att.aux_usage;
   if (info.hiz_usage != ISL_AUX_USAGE_NONE) {
               const struct anv_surface *hiz_surface =
                                                               if (gfx->stencil_att.iview != NULL) {
      const struct anv_image_view *iview = gfx->stencil_att.iview;
            const uint32_t stencil_plane =
         const struct anv_surface *stencil_surface =
         const struct anv_address stencil_address =
                              info.stencil_aux_usage = image->planes[stencil_plane].aux_usage;
   info.stencil_address = anv_address_physical(stencil_address);
   info.mocs =
                        /* Wa_14016712196:
   * Emit depth flush after state that sends implicit depth flush.
   */
   if (intel_needs_workaround(cmd_buffer->device->info, 14016712196)) {
      genx_batch_emit_pipe_control(&cmd_buffer->batch,
                     if (info.depth_surf)
            if (GFX_VER >= 11) {
      cmd_buffer->state.pending_pipe_bits |= ANV_PIPE_POST_SYNC_BIT;
            if (intel_needs_workaround(cmd_buffer->device->info, 1408224581) ||
      intel_needs_workaround(cmd_buffer->device->info, 14014097488)) {
   /* Wa_1408224581
   *
   * Workaround: Gfx12LP Astep only An additional pipe control with
   * post-sync = store dword operation would be required.( w/a is to
   * have an additional pipe control after the stencil state whenever
   * the surface state bits of this state is changing).
   *
   * This also seems sufficient to handle Wa_14014097488.
   */
   genx_batch_emit_pipe_control_write
      (&cmd_buffer->batch, cmd_buffer->device->info, WriteImmediateData,
      }
      }
      static void
   cmd_buffer_emit_cps_control_buffer(struct anv_cmd_buffer *cmd_buffer,
         {
   #if GFX_VERx10 >= 125
               if (!device->vk.enabled_extensions.KHR_fragment_shading_rate)
            uint32_t *dw = anv_batch_emit_dwords(&cmd_buffer->batch,
         if (dw == NULL)
                     if (fsr_iview) {
                        struct anv_address addr =
            info.view = &fsr_iview->planes[0].isl;
   info.surf = &fsr_iview->image->planes[0].primary_surface.isl;
   info.address = anv_address_physical(addr);
   info.mocs =
      anv_mocs(device, fsr_iview->image->bindings[0].address.bo,
                     /* Wa_14016712196:
   * Emit depth flush after state that sends implicit depth flush.
   */
   if (intel_needs_workaround(cmd_buffer->device->info, 14016712196)) {
      genx_batch_emit_pipe_control(&cmd_buffer->batch,
               #endif /* GFX_VERx10 >= 125 */
   }
      static VkImageLayout
   attachment_initial_layout(const VkRenderingAttachmentInfo *att)
   {
      const VkRenderingAttachmentInitialLayoutInfoMESA *layout_info =
      vk_find_struct_const(att->pNext,
      if (layout_info != NULL)
               }
      void genX(CmdBeginRendering)(
      VkCommandBuffer                             commandBuffer,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   struct anv_cmd_graphics_state *gfx = &cmd_buffer->state.gfx;
            if (!anv_cmd_buffer_is_render_queue(cmd_buffer)) {
      assert(!"Trying to start a render pass on non-render queue!");
   anv_batch_set_error(&cmd_buffer->batch, VK_ERROR_UNKNOWN);
               anv_measure_beginrenderpass(cmd_buffer);
            gfx->rendering_flags = pRenderingInfo->flags;
   gfx->view_mask = pRenderingInfo->viewMask;
   gfx->layer_count = pRenderingInfo->layerCount;
            if (gfx->render_area.offset.x != pRenderingInfo->renderArea.offset.x ||
      gfx->render_area.offset.y != pRenderingInfo->renderArea.offset.y ||
   gfx->render_area.extent.width != pRenderingInfo->renderArea.extent.width ||
   gfx->render_area.extent.height != pRenderingInfo->renderArea.extent.height) {
   gfx->render_area = pRenderingInfo->renderArea;
               const bool is_multiview = gfx->view_mask != 0;
   const VkRect2D render_area = gfx->render_area;
   const uint32_t layers =
            /* The framebuffer size is at least large enough to contain the render
   * area.  Because a zero renderArea is possible, we MAX with 1.
   */
   struct isl_extent3d fb_size = {
      .w = MAX2(1, render_area.offset.x + render_area.extent.width),
   .h = MAX2(1, render_area.offset.y + render_area.extent.height),
               const uint32_t color_att_count = pRenderingInfo->colorAttachmentCount;
   result = anv_cmd_buffer_init_attachments(cmd_buffer, color_att_count);
   if (result != VK_SUCCESS)
                     for (uint32_t i = 0; i < gfx->color_att_count; i++) {
      if (pRenderingInfo->pColorAttachments[i].imageView == VK_NULL_HANDLE)
            const VkRenderingAttachmentInfo *att =
         ANV_FROM_HANDLE(anv_image_view, iview, att->imageView);
            assert(render_area.offset.x + render_area.extent.width <=
         assert(render_area.offset.y + render_area.extent.height <=
                  fb_size.w = MAX2(fb_size.w, iview->vk.extent.width);
            assert(gfx->samples == 0 || gfx->samples == iview->vk.image->samples);
            enum isl_aux_usage aux_usage =
      anv_layout_to_aux_usage(cmd_buffer->device->info,
                                          if (att->loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR &&
      !(gfx->rendering_flags & VK_RENDERING_RESUMING_BIT)) {
   const union isl_color_value clear_color =
                  /* We only support fast-clears on the first layer */
   const bool fast_clear =
      (!is_multiview || (gfx->view_mask & 1)) &&
   anv_can_fast_clear_color_view(cmd_buffer->device, iview,
                     if (att->imageLayout != initial_layout) {
      assert(render_area.offset.x == 0 && render_area.offset.y == 0 &&
         render_area.extent.width == iview->vk.extent.width &&
   if (is_multiview) {
      u_foreach_bit(view, gfx->view_mask) {
      transition_color_buffer(cmd_buffer, iview->image,
                           VK_IMAGE_ASPECT_COLOR_BIT,
   iview->vk.base_mip_level, 1,
   iview->vk.base_array_layer + view,
         } else {
      transition_color_buffer(cmd_buffer, iview->image,
                           VK_IMAGE_ASPECT_COLOR_BIT,
   iview->vk.base_mip_level, 1,
   iview->vk.base_array_layer,
                  uint32_t clear_view_mask = pRenderingInfo->viewMask;
   uint32_t base_clear_layer = iview->vk.base_array_layer;
   uint32_t clear_layer_count = gfx->layer_count;
   if (fast_clear) {
      /* We only support fast-clears on the first layer */
                           if (iview->image->vk.samples == 1) {
      anv_image_ccs_op(cmd_buffer, iview->image,
                  iview->planes[0].isl.format,
   iview->planes[0].isl.swizzle,
   VK_IMAGE_ASPECT_COLOR_BIT,
   } else {
      anv_image_mcs_op(cmd_buffer, iview->image,
                  iview->planes[0].isl.format,
   iview->planes[0].isl.swizzle,
   VK_IMAGE_ASPECT_COLOR_BIT,
   }
   clear_view_mask &= ~1u;
                  genX(set_fast_clear_state)(cmd_buffer, iview->image,
                     if (is_multiview) {
      u_foreach_bit(view, clear_view_mask) {
      anv_image_clear_color(cmd_buffer, iview->image,
                        VK_IMAGE_ASPECT_COLOR_BIT,
   aux_usage,
   iview->planes[0].isl.format,
      } else {
      anv_image_clear_color(cmd_buffer, iview->image,
                        VK_IMAGE_ASPECT_COLOR_BIT,
   aux_usage,
   iview->planes[0].isl.format,
      } else {
      /* If not LOAD_OP_CLEAR, we shouldn't have a layout transition. */
               gfx->color_att[i].vk_format = iview->vk.format;
   gfx->color_att[i].iview = iview;
   gfx->color_att[i].layout = att->imageLayout;
            struct isl_view isl_view = iview->planes[0].isl;
   if (pRenderingInfo->viewMask) {
      assert(isl_view.array_len >= util_last_bit(pRenderingInfo->viewMask));
      } else {
      assert(isl_view.array_len >= pRenderingInfo->layerCount);
               anv_image_fill_surface_state(cmd_buffer->device,
                              iview->image,
                        if (GFX_VER < 10 &&
      (att->loadOp == VK_ATTACHMENT_LOAD_OP_LOAD ||
   (gfx->rendering_flags & VK_RENDERING_RESUMING_BIT)) &&
   iview->image->planes[0].aux_usage != ISL_AUX_USAGE_NONE &&
   iview->planes[0].isl.base_level == 0 &&
   iview->planes[0].isl.base_array_layer == 0) {
   genX(load_image_clear_color)(cmd_buffer,
                     if (att->resolveMode != VK_RESOLVE_MODE_NONE) {
      gfx->color_att[i].resolve_mode = att->resolveMode;
   gfx->color_att[i].resolve_iview =
                                 const struct anv_image_view *fsr_iview = NULL;
   const VkRenderingFragmentShadingRateAttachmentInfoKHR *fsr_att =
      vk_find_struct_const(pRenderingInfo->pNext,
      if (fsr_att != NULL && fsr_att->imageView != VK_NULL_HANDLE) {
      fsr_iview = anv_image_view_from_handle(fsr_att->imageView);
               const struct anv_image_view *ds_iview = NULL;
   const VkRenderingAttachmentInfo *d_att = pRenderingInfo->pDepthAttachment;
   const VkRenderingAttachmentInfo *s_att = pRenderingInfo->pStencilAttachment;
   if ((d_att != NULL && d_att->imageView != VK_NULL_HANDLE) ||
      (s_att != NULL && s_att->imageView != VK_NULL_HANDLE)) {
   const struct anv_image_view *d_iview = NULL, *s_iview = NULL;
   VkImageLayout depth_layout = VK_IMAGE_LAYOUT_UNDEFINED;
   VkImageLayout stencil_layout = VK_IMAGE_LAYOUT_UNDEFINED;
   VkImageLayout initial_depth_layout = VK_IMAGE_LAYOUT_UNDEFINED;
   VkImageLayout initial_stencil_layout = VK_IMAGE_LAYOUT_UNDEFINED;
   enum isl_aux_usage depth_aux_usage = ISL_AUX_USAGE_NONE;
   enum isl_aux_usage stencil_aux_usage = ISL_AUX_USAGE_NONE;
   float depth_clear_value = 0;
            if (d_att != NULL && d_att->imageView != VK_NULL_HANDLE) {
      d_iview = anv_image_view_from_handle(d_att->imageView);
   initial_depth_layout = attachment_initial_layout(d_att);
   depth_layout = d_att->imageLayout;
   depth_aux_usage =
      anv_layout_to_aux_usage(cmd_buffer->device->info,
                                          if (s_att != NULL && s_att->imageView != VK_NULL_HANDLE) {
      s_iview = anv_image_view_from_handle(s_att->imageView);
   initial_stencil_layout = attachment_initial_layout(s_att);
   stencil_layout = s_att->imageLayout;
   stencil_aux_usage =
      anv_layout_to_aux_usage(cmd_buffer->device->info,
                                          assert(s_iview == NULL || d_iview == NULL || s_iview == d_iview);
   ds_iview = d_iview != NULL ? d_iview : s_iview;
            assert(render_area.offset.x + render_area.extent.width <=
         assert(render_area.offset.y + render_area.extent.height <=
                  fb_size.w = MAX2(fb_size.w, ds_iview->vk.extent.width);
            assert(gfx->samples == 0 || gfx->samples == ds_iview->vk.image->samples);
            VkImageAspectFlags clear_aspects = 0;
   if (d_iview != NULL && d_att->loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR &&
      !(gfx->rendering_flags & VK_RENDERING_RESUMING_BIT))
      if (s_iview != NULL && s_att->loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR &&
                  if (clear_aspects != 0) {
      const bool hiz_clear =
      anv_can_hiz_clear_ds_view(cmd_buffer->device, d_iview,
                           if (depth_layout != initial_depth_layout) {
      assert(render_area.offset.x == 0 && render_area.offset.y == 0 &&
                  if (is_multiview) {
      u_foreach_bit(view, gfx->view_mask) {
      transition_depth_buffer(cmd_buffer, d_iview->image,
                           } else {
      transition_depth_buffer(cmd_buffer, d_iview->image,
                                    if (stencil_layout != initial_stencil_layout) {
      assert(render_area.offset.x == 0 && render_area.offset.y == 0 &&
                  if (is_multiview) {
      u_foreach_bit(view, gfx->view_mask) {
      transition_stencil_buffer(cmd_buffer, s_iview->image,
                           s_iview->vk.base_mip_level, 1,
         } else {
      transition_stencil_buffer(cmd_buffer, s_iview->image,
                           s_iview->vk.base_mip_level, 1,
                  if (is_multiview) {
      uint32_t clear_view_mask = pRenderingInfo->viewMask;
                                    if (hiz_clear) {
      anv_image_hiz_clear(cmd_buffer, ds_iview->image,
                        } else {
      anv_image_clear_depth_stencil(cmd_buffer, ds_iview->image,
                                          } else {
      uint32_t level = ds_iview->vk.base_mip_level;
                  if (hiz_clear) {
      anv_image_hiz_clear(cmd_buffer, ds_iview->image,
                        } else {
      anv_image_clear_depth_stencil(cmd_buffer, ds_iview->image,
                                          } else {
      /* If not LOAD_OP_CLEAR, we shouldn't have a layout transition. */
   assert(depth_layout == initial_depth_layout);
               if (d_iview != NULL) {
      gfx->depth_att.vk_format = d_iview->vk.format;
   gfx->depth_att.iview = d_iview;
   gfx->depth_att.layout = depth_layout;
   gfx->depth_att.aux_usage = depth_aux_usage;
   if (d_att != NULL && d_att->resolveMode != VK_RESOLVE_MODE_NONE) {
      assert(d_att->resolveImageView != VK_NULL_HANDLE);
   gfx->depth_att.resolve_mode = d_att->resolveMode;
   gfx->depth_att.resolve_iview =
                        if (s_iview != NULL) {
      gfx->stencil_att.vk_format = s_iview->vk.format;
   gfx->stencil_att.iview = s_iview;
   gfx->stencil_att.layout = stencil_layout;
   gfx->stencil_att.aux_usage = stencil_aux_usage;
   if (s_att->resolveMode != VK_RESOLVE_MODE_NONE) {
      assert(s_att->resolveImageView != VK_NULL_HANDLE);
   gfx->stencil_att.resolve_mode = s_att->resolveMode;
   gfx->stencil_att.resolve_iview =
                           /* Finally, now that we know the right size, set up the null surface */
   assert(util_bitcount(gfx->samples) <= 1);
   isl_null_fill_state(&cmd_buffer->device->isl_dev,
                  for (uint32_t i = 0; i < gfx->color_att_count; i++) {
      if (pRenderingInfo->pColorAttachments[i].imageView != VK_NULL_HANDLE)
            isl_null_fill_state(&cmd_buffer->device->isl_dev,
                                       /* It is possible to start a render pass with an old pipeline.  Because the
   * render pass and subpass index are both baked into the pipeline, this is
   * highly unlikely.  In order to do so, it requires that you have a render
   * pass with a single subpass and that you use that render pass twice
   * back-to-back and use the same pipeline at the start of the second render
   * pass as at the end of the first.  In order to avoid unpredictable issues
   * with this edge case, we just dirty the pipeline at the start of every
   * subpass.
   */
         #if GFX_VER >= 11
      bool has_color_att = false;
   for (uint32_t i = 0; i < gfx->color_att_count; i++) {
      if (pRenderingInfo->pColorAttachments[i].imageView != VK_NULL_HANDLE) {
      has_color_att = true;
         }
   if (has_color_att) {
      /* The PIPE_CONTROL command description says:
   *
   *    "Whenever a Binding Table Index (BTI) used by a Render Target Message
   *     points to a different RENDER_SURFACE_STATE, SW must issue a Render
   *     Target Cache Flush by enabling this bit. When render target flush
   *     is set due to new association of BTI, PS Scoreboard Stall bit must
   *     be set in this packet."
   *
   * We assume that a new BeginRendering is always changing the RTs, which
   * may not be true and cause excessive flushing.  We can trivially skip it
   * in the case that there are no RTs (depth-only rendering), though.
   */
   anv_add_pending_pipe_bits(cmd_buffer,
                     #endif
                     }
      static void
   cmd_buffer_mark_attachment_written(struct anv_cmd_buffer *cmd_buffer,
               {
      struct anv_cmd_graphics_state *gfx = &cmd_buffer->state.gfx;
            if (iview == NULL)
            if (gfx->view_mask == 0) {
      genX(cmd_buffer_mark_image_written)(cmd_buffer, iview->image,
                        } else {
      uint32_t res_view_mask = gfx->view_mask;
   while (res_view_mask) {
                              genX(cmd_buffer_mark_image_written)(cmd_buffer, iview->image,
                  }
      static enum blorp_filter
   vk_to_blorp_resolve_mode(VkResolveModeFlagBits vk_mode)
   {
      switch (vk_mode) {
   case VK_RESOLVE_MODE_SAMPLE_ZERO_BIT:
         case VK_RESOLVE_MODE_AVERAGE_BIT:
         case VK_RESOLVE_MODE_MIN_BIT:
         case VK_RESOLVE_MODE_MAX_BIT:
         default:
            }
      static void
   cmd_buffer_resolve_msaa_attachment(struct anv_cmd_buffer *cmd_buffer,
                     {
      struct anv_cmd_graphics_state *gfx = &cmd_buffer->state.gfx;
   const struct anv_image_view *src_iview = att->iview;
            enum isl_aux_usage src_aux_usage =
      anv_layout_to_aux_usage(cmd_buffer->device->info,
                           enum isl_aux_usage dst_aux_usage =
      anv_layout_to_aux_usage(cmd_buffer->device->info,
                                    const VkRect2D render_area = gfx->render_area;
   if (gfx->view_mask == 0) {
      anv_image_msaa_resolve(cmd_buffer,
                        src_iview->image, src_aux_usage,
   src_iview->planes[0].isl.base_level,
   src_iview->planes[0].isl.base_array_layer,
   dst_iview->image, dst_aux_usage,
   dst_iview->planes[0].isl.base_level,
   dst_iview->planes[0].isl.base_array_layer,
   aspect,
   render_area.offset.x, render_area.offset.y,
   } else {
      uint32_t res_view_mask = gfx->view_mask;
   while (res_view_mask) {
               anv_image_msaa_resolve(cmd_buffer,
                        src_iview->image, src_aux_usage,
   src_iview->planes[0].isl.base_level,
   src_iview->planes[0].isl.base_array_layer + i,
   dst_iview->image, dst_aux_usage,
   dst_iview->planes[0].isl.base_level,
   dst_iview->planes[0].isl.base_array_layer + i,
   aspect,
   render_area.offset.x, render_area.offset.y,
         }
      void genX(CmdEndRendering)(
         {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
            if (anv_batch_has_error(&cmd_buffer->batch))
            const bool is_multiview = gfx->view_mask != 0;
   const uint32_t layers =
            bool has_color_resolve = false;
   for (uint32_t i = 0; i < gfx->color_att_count; i++) {
      cmd_buffer_mark_attachment_written(cmd_buffer, &gfx->color_att[i],
            /* Stash this off for later */
   if (gfx->color_att[i].resolve_mode != VK_RESOLVE_MODE_NONE &&
      !(gfx->rendering_flags & VK_RENDERING_SUSPENDING_BIT))
            cmd_buffer_mark_attachment_written(cmd_buffer, &gfx->depth_att,
            cmd_buffer_mark_attachment_written(cmd_buffer, &gfx->stencil_att,
            if (has_color_resolve) {
      /* We are about to do some MSAA resolves.  We need to flush so that the
   * result of writes to the MSAA color attachments show up in the sampler
   * when we blit to the single-sampled resolve target.
   */
   anv_add_pending_pipe_bits(cmd_buffer,
                           if (gfx->depth_att.resolve_mode != VK_RESOLVE_MODE_NONE ||
      gfx->stencil_att.resolve_mode != VK_RESOLVE_MODE_NONE) {
   /* We are about to do some MSAA resolves.  We need to flush so that the
   * result of writes to the MSAA depth attachments show up in the sampler
   * when we blit to the single-sampled resolve target.
   */
   anv_add_pending_pipe_bits(cmd_buffer,
                           for (uint32_t i = 0; i < gfx->color_att_count; i++) {
      const struct anv_attachment *att = &gfx->color_att[i];
   if (att->resolve_mode == VK_RESOLVE_MODE_NONE ||
                  cmd_buffer_resolve_msaa_attachment(cmd_buffer, att, att->layout,
               if (gfx->depth_att.resolve_mode != VK_RESOLVE_MODE_NONE &&
      !(gfx->rendering_flags & VK_RENDERING_SUSPENDING_BIT)) {
            /* MSAA resolves sample from the source attachment.  Transition the
   * depth attachment first to get rid of any HiZ that we may not be
   * able to handle.
   */
   transition_depth_buffer(cmd_buffer, src_iview->image,
                                    cmd_buffer_resolve_msaa_attachment(cmd_buffer, &gfx->depth_att,
                  /* Transition the source back to the original layout.  This seems a bit
   * inefficient but, since HiZ resolves aren't destructive, going from
   * less HiZ to more is generally a no-op.
   */
   transition_depth_buffer(cmd_buffer, src_iview->image,
                                       if (gfx->stencil_att.resolve_mode != VK_RESOLVE_MODE_NONE &&
      !(gfx->rendering_flags & VK_RENDERING_SUSPENDING_BIT)) {
   cmd_buffer_resolve_msaa_attachment(cmd_buffer, &gfx->stencil_att,
                        trace_intel_end_render_pass(&cmd_buffer->trace,
                                 }
      void
   genX(cmd_emit_conditional_render_predicate)(struct anv_cmd_buffer *cmd_buffer)
   {
      struct mi_builder b;
            mi_store(&b, mi_reg64(MI_PREDICATE_SRC0),
                  anv_batch_emit(&cmd_buffer->batch, GENX(MI_PREDICATE), mip) {
      mip.LoadOperation    = LOAD_LOADINV;
   mip.CombineOperation = COMBINE_SET;
         }
      void genX(CmdBeginConditionalRenderingEXT)(
      VkCommandBuffer                             commandBuffer,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   ANV_FROM_HANDLE(anv_buffer, buffer, pConditionalRenderingBegin->buffer);
   struct anv_cmd_state *cmd_state = &cmd_buffer->state;
   struct anv_address value_address =
            const bool isInverted = pConditionalRenderingBegin->flags &
                              struct mi_builder b;
   mi_builder_init(&b, cmd_buffer->device->info, &cmd_buffer->batch);
   const uint32_t mocs = anv_mocs_for_address(cmd_buffer->device, &value_address);
            /* Section 19.4 of the Vulkan 1.1.85 spec says:
   *
   *    If the value of the predicate in buffer memory changes
   *    while conditional rendering is active, the rendering commands
   *    may be discarded in an implementation-dependent way.
   *    Some implementations may latch the value of the predicate
   *    upon beginning conditional rendering while others
   *    may read it before every rendering command.
   *
   * So it's perfectly fine to read a value from the buffer once.
   */
            /* Precompute predicate result, it is necessary to support secondary
   * command buffers since it is unknown if conditional rendering is
   * inverted when populating them.
   */
   mi_store(&b, mi_reg64(ANV_PREDICATE_RESULT_REG),
            }
      void genX(CmdEndConditionalRenderingEXT)(
   VkCommandBuffer                             commandBuffer)
   {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
               }
      /* Set of stage bits for which are pipelined, i.e. they get queued
   * by the command streamer for later execution.
   */
   #define ANV_PIPELINE_STAGE_PIPELINED_BITS \
      ~(VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT | \
   VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT | \
   VK_PIPELINE_STAGE_2_HOST_BIT | \
         void genX(CmdSetEvent2)(
      VkCommandBuffer                             commandBuffer,
   VkEvent                                     _event,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
            if (anv_cmd_buffer_is_video_queue(cmd_buffer)) {
      anv_batch_emit(&cmd_buffer->batch, GENX(MI_FLUSH_DW), flush) {
      flush.PostSyncOperation = WriteImmediateData;
   flush.Address = anv_state_pool_state_address(
      &cmd_buffer->device->dynamic_state_pool,
         }
                        for (uint32_t i = 0; i < pDependencyInfo->memoryBarrierCount; i++)
         for (uint32_t i = 0; i < pDependencyInfo->bufferMemoryBarrierCount; i++)
         for (uint32_t i = 0; i < pDependencyInfo->imageMemoryBarrierCount; i++)
            cmd_buffer->state.pending_pipe_bits |= ANV_PIPE_POST_SYNC_BIT;
            enum anv_pipe_bits pc_bits = 0;
   if (src_stages & ANV_PIPELINE_STAGE_PIPELINED_BITS) {
      pc_bits |= ANV_PIPE_STALL_AT_SCOREBOARD_BIT;
   }
         genx_batch_emit_pipe_control_write
      (&cmd_buffer->batch, cmd_buffer->device->info, WriteImmediateData,
   anv_state_pool_state_address(&cmd_buffer->device->dynamic_state_pool,
         }
      void genX(CmdResetEvent2)(
      VkCommandBuffer                             commandBuffer,
   VkEvent                                     _event,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
            if (anv_cmd_buffer_is_video_queue(cmd_buffer)) {
      anv_batch_emit(&cmd_buffer->batch, GENX(MI_FLUSH_DW), flush) {
      flush.PostSyncOperation = WriteImmediateData;
   flush.Address = anv_state_pool_state_address(
      &cmd_buffer->device->dynamic_state_pool,
         }
               cmd_buffer->state.pending_pipe_bits |= ANV_PIPE_POST_SYNC_BIT;
            enum anv_pipe_bits pc_bits = 0;
   if (stageMask & ANV_PIPELINE_STAGE_PIPELINED_BITS) {
      pc_bits |= ANV_PIPE_STALL_AT_SCOREBOARD_BIT;
               genx_batch_emit_pipe_control_write
      (&cmd_buffer->batch, cmd_buffer->device->info, WriteImmediateData,
   anv_state_pool_state_address(&cmd_buffer->device->dynamic_state_pool,
         VK_EVENT_RESET,
   }
      void genX(CmdWaitEvents2)(
      VkCommandBuffer                             commandBuffer,
   uint32_t                                    eventCount,
   const VkEvent*                              pEvents,
      {
               for (uint32_t i = 0; i < eventCount; i++) {
               anv_batch_emit(&cmd_buffer->batch, GENX(MI_SEMAPHORE_WAIT), sem) {
      sem.WaitMode            = PollingMode;
   sem.CompareOperation    = COMPARE_SAD_EQUAL_SDD;
   sem.SemaphoreDataDword  = VK_EVENT_SET;
   sem.SemaphoreAddress    = anv_state_pool_state_address(
      &cmd_buffer->device->dynamic_state_pool,
                  }
      static uint32_t vk_to_intel_index_type(VkIndexType type)
   {
      switch (type) {
   case VK_INDEX_TYPE_UINT8_EXT:
         case VK_INDEX_TYPE_UINT16:
         case VK_INDEX_TYPE_UINT32:
         default:
            }
      static uint32_t restart_index_for_type(VkIndexType type)
   {
      switch (type) {
   case VK_INDEX_TYPE_UINT8_EXT:
         case VK_INDEX_TYPE_UINT16:
         case VK_INDEX_TYPE_UINT32:
         default:
            }
      void genX(CmdBindIndexBuffer2KHR)(
      VkCommandBuffer                             commandBuffer,
   VkBuffer                                    _buffer,
   VkDeviceSize                                offset,
   VkDeviceSize                                size,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
            uint32_t restart_index = restart_index_for_type(indexType);
   if (cmd_buffer->state.gfx.restart_index != restart_index) {
      cmd_buffer->state.gfx.restart_index = restart_index;
               uint32_t index_type = vk_to_intel_index_type(indexType);
   if (cmd_buffer->state.gfx.index_buffer != buffer ||
      cmd_buffer->state.gfx.index_type != index_type ||
   cmd_buffer->state.gfx.index_offset != offset) {
   cmd_buffer->state.gfx.index_buffer = buffer;
   cmd_buffer->state.gfx.index_type = vk_to_intel_index_type(indexType);
   cmd_buffer->state.gfx.index_offset = offset;
   cmd_buffer->state.gfx.index_size = vk_buffer_range(&buffer->vk, offset, size);
         }
      VkResult genX(CmdSetPerformanceOverrideINTEL)(
      VkCommandBuffer                             commandBuffer,
      {
               switch (pOverrideInfo->type) {
   case VK_PERFORMANCE_OVERRIDE_TYPE_NULL_HARDWARE_INTEL: {
      anv_batch_write_reg(&cmd_buffer->batch, GENX(CS_DEBUG_MODE2), csdm2) {
      csdm2._3DRenderingInstructionDisable = pOverrideInfo->enable;
   csdm2.MediaInstructionDisable = pOverrideInfo->enable;
   csdm2._3DRenderingInstructionDisableMask = true;
      }
               case VK_PERFORMANCE_OVERRIDE_TYPE_FLUSH_GPU_CACHES_INTEL:
      if (pOverrideInfo->enable) {
      /* FLUSH ALL THE THINGS! As requested by the MDAPI team. */
   anv_add_pending_pipe_bits(cmd_buffer,
                        }
         default:
                     }
      VkResult genX(CmdSetPerformanceStreamMarkerINTEL)(
      VkCommandBuffer                             commandBuffer,
      {
                  }
      #define TIMESTAMP 0x2358
      void genX(cmd_emit_timestamp)(struct anv_batch *batch,
                              /* Make sure ANV_TIMESTAMP_CAPTURE_AT_CS_STALL and
   * ANV_TIMESTAMP_REWRITE_COMPUTE_WALKER capture type are not set for
   * transfer queue.
   */
   if ((batch->engine_class == INTEL_ENGINE_CLASS_COPY) ||
      (batch->engine_class == INTEL_ENGINE_CLASS_VIDEO)) {
   assert(type != ANV_TIMESTAMP_CAPTURE_AT_CS_STALL &&
               switch (type) {
   case ANV_TIMESTAMP_CAPTURE_TOP_OF_PIPE: {
      struct mi_builder b;
   mi_builder_init(&b, device->info, batch);
   mi_store(&b, mi_mem64(addr), mi_reg64(TIMESTAMP));
               case ANV_TIMESTAMP_CAPTURE_END_OF_PIPE: {
      if ((batch->engine_class == INTEL_ENGINE_CLASS_COPY) ||
      (batch->engine_class == INTEL_ENGINE_CLASS_VIDEO)) {
   anv_batch_emit(batch, GENX(MI_FLUSH_DW), fd) {
      fd.PostSyncOperation = WriteTimestamp;
         } else {
      genx_batch_emit_pipe_control_write(batch, device->info,
      }
               case ANV_TIMESTAMP_CAPTURE_AT_CS_STALL:
      genx_batch_emit_pipe_control_write
      (batch, device->info, WriteTimestamp, addr, 0,
         #if GFX_VERx10 >= 125
      case ANV_TIMESTAMP_REWRITE_COMPUTE_WALKER: {
               GENX(COMPUTE_WALKER_pack)(batch, dwords, &(struct GENX(COMPUTE_WALKER)) {
         .PostSync = (struct GENX(POSTSYNC_DATA)) {
      .Operation = WriteTimestamp,
   .DestinationAddress = addr,
               for (uint32_t i = 0; i < ARRAY_SIZE(dwords); i++)
               #endif
         default:
            }
      void genX(batch_emit_secondary_call)(struct anv_batch *batch,
               {
      /* Emit a write to change the return address of the secondary */
   uint64_t *write_return_addr =
      anv_batch_emitn(batch,
         #if GFX_VER >= 12
         #endif
                  #if GFX_VER >= 12
      /* Disable prefetcher before jumping into a secondary */
   anv_batch_emit(batch, GENX(MI_ARB_CHECK), arb) {
      arb.PreParserDisableMask = true;
         #endif
         /* Jump into the secondary */
   anv_batch_emit(batch, GENX(MI_BATCH_BUFFER_START), bbs) {
      bbs.AddressSpaceIndicator = ASI_PPGTT;
   bbs.SecondLevelBatchBuffer = Firstlevelbatch;
               /* Replace the return address written by the MI_STORE_DATA_IMM above with
   * the primary's current batch address (immediately after the jump).
   */
   *write_return_addr =
      }
      void *
   genX(batch_emit_return)(struct anv_batch *batch)
   {
      return anv_batch_emitn(batch,
                        }
      void
   genX(batch_emit_dummy_post_sync_op)(struct anv_batch *batch,
                     {
      if (!intel_needs_workaround(device->info, 22014412737))
            if ((primitive_topology == _3DPRIM_POINTLIST ||
      primitive_topology == _3DPRIM_LINELIST ||
   primitive_topology == _3DPRIM_LINESTRIP ||
   primitive_topology == _3DPRIM_LINELIST_ADJ ||
   primitive_topology == _3DPRIM_LINESTRIP_ADJ ||
   primitive_topology == _3DPRIM_LINELOOP ||
   primitive_topology == _3DPRIM_POINTLIST_BF ||
   primitive_topology == _3DPRIM_LINESTRIP_CONT ||
   primitive_topology == _3DPRIM_LINESTRIP_BF ||
   primitive_topology == _3DPRIM_LINESTRIP_CONT_BF) &&
   (vertex_count == 1 || vertex_count == 2)) {
   genx_batch_emit_pipe_control_write
                  }
      struct anv_state
   genX(cmd_buffer_begin_companion_rcs_syncpoint)(
         {
   #if GFX_VERx10 >= 125
      const struct intel_device_info *info = cmd_buffer->device->info;
   struct anv_state syncpoint =
         struct anv_address xcs_wait_addr =
      anv_state_pool_state_address(&cmd_buffer->device->dynamic_state_pool,
               /* Reset the sync point */
                     /* On CCS:
   *    - flush all caches & invalidate
   *    - unblock RCS
   *    - wait on RCS to complete
   *    - clear the value we waited on
            if (anv_cmd_buffer_is_compute_queue(cmd_buffer)) {
      anv_add_pending_pipe_bits(cmd_buffer, ANV_PIPE_FLUSH_BITS |
                        } else if (anv_cmd_buffer_is_blitter_queue(cmd_buffer)) {
      anv_batch_emit(&cmd_buffer->batch, GENX(MI_FLUSH_DW), fd) {
                     {
      mi_builder_init(&b, info, &cmd_buffer->batch);
   mi_store(&b, mi_mem32(rcs_wait_addr), mi_imm(0x1));
   anv_batch_emit(&cmd_buffer->batch, GENX(MI_SEMAPHORE_WAIT), sem) {
      sem.WaitMode            = PollingMode;
   sem.CompareOperation    = COMPARE_SAD_EQUAL_SDD;
   sem.SemaphoreDataDword  = 0x1;
      }
   /* Make sure to reset the semaphore in case the command buffer is run
   * multiple times.
   */
               /* On RCS:
   *    - wait on CCS signal
   *    - clear the value we waited on
   */
   {
      mi_builder_init(&b, info, &cmd_buffer->companion_rcs_cmd_buffer->batch);
   anv_batch_emit(&cmd_buffer->companion_rcs_cmd_buffer->batch,
                  sem.WaitMode            = PollingMode;
   sem.CompareOperation    = COMPARE_SAD_EQUAL_SDD;
   sem.SemaphoreDataDword  = 0x1;
      }
   /* Make sure to reset the semaphore in case the command buffer is run
   * multiple times.
   */
                  #else
         #endif
   }
      void
   genX(cmd_buffer_end_companion_rcs_syncpoint)(struct anv_cmd_buffer *cmd_buffer,
         {
   #if GFX_VERx10 >= 125
      struct anv_address xcs_wait_addr =
      anv_state_pool_state_address(&cmd_buffer->device->dynamic_state_pool,
                  /* On RCS:
   *    - flush all caches & invalidate
   *    - unblock the CCS
   */
   anv_add_pending_pipe_bits(cmd_buffer->companion_rcs_cmd_buffer,
                                    mi_builder_init(&b, cmd_buffer->device->info,
            #else
         #endif
   }
