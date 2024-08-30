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
   #include <string.h>
   #include <unistd.h>
   #include <fcntl.h>
      #include "anv_private.h"
      #include "common/intel_aux_map.h"
   #include "common/intel_sample_positions.h"
   #include "common/intel_pixel_hash.h"
   #include "genxml/gen_macros.h"
   #include "genxml/genX_pack.h"
      #include "vk_standard_sample_locations.h"
   #include "vk_util.h"
   #include "vk_format.h"
      static VkResult
   init_render_queue_state(struct anv_queue *queue)
   {
      struct anv_device *device = queue->device;
   uint32_t cmds[128];
   struct anv_batch batch = {
      .start = cmds,
   .next = cmds,
               anv_batch_emit(&batch, GENX(PIPELINE_SELECT), ps) {
                           anv_batch_emit(&batch, GENX(3DSTATE_DRAWING_RECTANGLE), rect) {
      rect.ClippedDrawingRectangleYMin = 0;
   rect.ClippedDrawingRectangleXMin = 0;
   rect.ClippedDrawingRectangleYMax = UINT16_MAX;
   rect.ClippedDrawingRectangleXMax = UINT16_MAX;
   rect.DrawingRectangleOriginY = 0;
            #if GFX_VER >= 8
                        /* The BDW+ docs describe how to use the 3DSTATE_WM_HZ_OP instruction in the
   * section titled, "Optimized Depth Buffer Clear and/or Stencil Buffer
   * Clear." It mentions that the packet overrides GPU state for the clear
   * operation and needs to be reset to 0s to clear the overrides. Depending
   * on the kernel, we may not get a context with the state for this packet
   * zeroed. Do it ourselves just in case. We've observed this to prevent a
   * number of GPU hangs on ICL.
   */
      #endif
         /* Set the "CONSTANT_BUFFER Address Offset Disable" bit, so
   * 3DSTATE_CONSTANT_XS buffer 0 is an absolute address.
   *
   * This is only safe on kernels with context isolation support.
   */
      #if GFX_VER == 8
         anv_batch_write_reg(&batch, GENX(INSTPM), instpm) {
      instpm.CONSTANT_BUFFERAddressOffsetDisable = true;
      #endif
                                    }
      void
   genX(init_physical_device_state)(ASSERTED struct anv_physical_device *pdevice)
   {
         }
      VkResult
   genX(init_device_state)(struct anv_device *device)
   {
               device->slice_hash = (struct anv_state) { 0 };
   for (uint32_t i = 0; i < device->queue_count; i++) {
      struct anv_queue *queue = &device->queues[i];
   switch (queue->family->engine_class) {
   case INTEL_ENGINE_CLASS_RENDER:
      res = init_render_queue_state(queue);
      default:
      res = vk_error(device, VK_ERROR_INITIALIZATION_FAILED);
      }
   if (res != VK_SUCCESS)
                  }
      void
   genX(emit_l3_config)(struct anv_batch *batch,
               {
            #if GFX_VER >= 8
      #define L3_ALLOCATION_REG GENX(L3CNTLREG)
   #define L3_ALLOCATION_REG_num GENX(L3CNTLREG_num)
         anv_batch_write_reg(batch, L3_ALLOCATION_REG, l3cr) {
      if (cfg == NULL) {
         } else {
      l3cr.SLMEnable = cfg->n[INTEL_L3P_SLM];
   assert(cfg->n[INTEL_L3P_IS] == 0);
   assert(cfg->n[INTEL_L3P_C] == 0);
   assert(cfg->n[INTEL_L3P_T] == 0);
   l3cr.URBAllocation = cfg->n[INTEL_L3P_URB];
   l3cr.ROAllocation = cfg->n[INTEL_L3P_RO];
   l3cr.DCAllocation = cfg->n[INTEL_L3P_DC];
               #else /* GFX_VER < 8 */
         const bool has_dc = cfg->n[INTEL_L3P_DC] || cfg->n[INTEL_L3P_ALL];
   const bool has_is = cfg->n[INTEL_L3P_IS] || cfg->n[INTEL_L3P_RO] ||
         const bool has_c = cfg->n[INTEL_L3P_C] || cfg->n[INTEL_L3P_RO] ||
         const bool has_t = cfg->n[INTEL_L3P_T] || cfg->n[INTEL_L3P_RO] ||
                     /* When enabled SLM only uses a portion of the L3 on half of the banks,
   * the matching space on the remaining banks has to be allocated to a
   * client (URB for all validated configurations) set to the
   * lower-bandwidth 2-bank address hashing mode.
   */
   const bool urb_low_bw = cfg->n[INTEL_L3P_SLM] && devinfo->platform != INTEL_PLATFORM_BYT;
            /* Minimum number of ways that can be allocated to the URB. */
   const unsigned n0_urb = devinfo->platform == INTEL_PLATFORM_BYT ? 32 : 0;
            anv_batch_write_reg(batch, GENX(L3SQCREG1), l3sqc) {
      l3sqc.ConvertDC_UC = !has_dc;
   l3sqc.ConvertIS_UC = !has_is;
   l3sqc.ConvertC_UC = !has_c;
   #if GFX_VERx10 == 75
         #else
         l3sqc.L3SQGeneralPriorityCreditInitialization =
   #endif
                     anv_batch_write_reg(batch, GENX(L3CNTLREG2), l3cr2) {
      l3cr2.SLMEnable = cfg->n[INTEL_L3P_SLM];
   l3cr2.URBLowBandwidth = urb_low_bw;
   #if !GFX_VERx10 == 75
         #endif
         l3cr2.ROAllocation = cfg->n[INTEL_L3P_RO];
               anv_batch_write_reg(batch, GENX(L3CNTLREG3), l3cr3) {
      l3cr3.ISAllocation = cfg->n[INTEL_L3P_IS];
   l3cr3.ISLowBandwidth = 0;
   l3cr3.CAllocation = cfg->n[INTEL_L3P_C];
   l3cr3.CLowBandwidth = 0;
   l3cr3.TAllocation = cfg->n[INTEL_L3P_T];
            #if GFX_VERx10 == 75
      if (device->physical->cmd_parser_version >= 4) {
      /* Enable L3 atomics on HSW if we have a DC partition, otherwise keep
   * them disabled to avoid crashing the system hard.
   */
   anv_batch_write_reg(batch, GENX(SCRATCH1), s1) {
         }
   anv_batch_write_reg(batch, GENX(CHICKEN3), c3) {
      c3.L3AtomicDisableMask = true;
            #endif /* GFX_VERx10 == 75 */
      #endif /* GFX_VER < 8 */
   }
      void
   genX(emit_multisample)(struct anv_batch *batch, uint32_t samples,
         {
      if (sl != NULL) {
      assert(sl->per_pixel == samples);
   assert(sl->grid_size.width == 1);
      } else {
                  anv_batch_emit(batch, GENX(3DSTATE_MULTISAMPLE), ms) {
               #if GFX_VER < 8
         switch (samples) {
   case 1:
      INTEL_SAMPLE_POS_1X_ARRAY(ms.Sample, sl->locations);
      case 2:
      INTEL_SAMPLE_POS_2X_ARRAY(ms.Sample, sl->locations);
      case 4:
      INTEL_SAMPLE_POS_4X_ARRAY(ms.Sample, sl->locations);
      case 8:
      INTEL_SAMPLE_POS_8X_ARRAY(ms.Sample, sl->locations);
      default:
         #endif
         }
      #if GFX_VER >= 8
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
   for (uint32_t i = 1; i <= (GFX_VER >= 9 ? 16 : 8); i *= 2) {
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
         #if GFX_VER >= 9
            case VK_SAMPLE_COUNT_16_BIT:
      if (sl && sl->per_pixel == i) {
         } else {
         #endif
            default:
                  }
   #endif
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
      VkResult genX(CreateSampler)(
      VkDevice                                    _device,
   const VkSamplerCreateInfo*                  pCreateInfo,
   const VkAllocationCallbacks*                pAllocator,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
                     sampler = vk_object_zalloc(&device->vk, pAllocator, sizeof(*sampler),
         if (!sampler)
                     uint32_t border_color_stride = GFX_VERx10 == 75 ? 512 : 64;
   uint32_t border_color_offset;
   ASSERTED bool has_custom_color = false;
   if (pCreateInfo->borderColor <= VK_BORDER_COLOR_INT_OPAQUE_WHITE) {
      border_color_offset = device->border_colors.offset +
            } else {
      assert(GFX_VER >= 8);
   sampler->custom_border_color =
                     const struct vk_format_ycbcr_info *ycbcr_info = NULL;
   vk_foreach_struct_const(ext, pCreateInfo->pNext) {
      switch (ext->sType) {
   case VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO: {
      VkSamplerYcbcrConversionInfo *pSamplerConversion =
                        /* Ignore conversion for non-YUV formats. This fulfills a requirement
   * for clients that want to utilize same code path for images with
   * external formats (VK_FORMAT_UNDEFINED) and "regular" RGBA images
   * where format is known.
   */
                  ycbcr_info = vk_format_get_ycbcr_info(conversion->state.format);
                  sampler->n_planes = ycbcr_info->n_planes;
   sampler->conversion = conversion;
      }
   case VK_STRUCTURE_TYPE_SAMPLER_CUSTOM_BORDER_COLOR_CREATE_INFO_EXT: {
      VkSamplerCustomBorderColorCreateInfoEXT *custom_border_color =
                        union isl_color_value color = { .u32 = {
      custom_border_color->customBorderColor.uint32[0],
   custom_border_color->customBorderColor.uint32[1],
   custom_border_color->customBorderColor.uint32[2],
               const struct anv_format *format_desc =
                  /* For formats with a swizzle, it does not carry over to the sampler
   * for border colors, so we need to do the swizzle ourselves here.
   */
   if (format_desc && format_desc->n_planes == 1 &&
                     assert(!isl_format_has_int_channel(fmt_plane->isl_format));
               memcpy(sampler->custom_border_color.map, color.u32, sizeof(color));
   has_custom_color = true;
      }
   case VK_STRUCTURE_TYPE_SAMPLER_BORDER_COLOR_COMPONENT_MAPPING_CREATE_INFO_EXT:
         default:
      anv_debug_ignored_stype(ext->sType);
                           if (device->physical->has_bindless_samplers) {
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
         const VkFilter mag_filter =
         const bool enable_min_filter_addr_rounding = min_filter != VK_FILTER_NEAREST;
   const bool enable_mag_filter_addr_rounding = mag_filter != VK_FILTER_NEAREST;
   /* From Broadwell PRM, SAMPLER_STATE:
   *   "Mip Mode Filter must be set to MIPFILTER_NONE for Planar YUV surfaces."
   */
   enum isl_format plane0_isl_format = sampler->conversion ?
      anv_get_format(sampler->conversion->state.format)->planes[0].isl_format :
      const bool isl_format_is_planar_yuv =
      plane0_isl_format != ISL_FORMAT_UNSUPPORTED &&
               const uint32_t mip_filter_mode =
                  struct GENX(SAMPLER_STATE) sampler_state = {
            #if GFX_VER >= 8
         #else
         #endif
      #if GFX_VER == 8
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
               #if GFX_VER >= 8
         #endif
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
   .TCYAddressControlMode = vk_to_intel_tex_address[pCreateInfo->addressModeV],
                        if (sampler->bindless_state.map) {
      memcpy(sampler->bindless_state.map + p * 32,
                              }
