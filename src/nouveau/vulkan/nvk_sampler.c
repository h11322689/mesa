   /*
   * Copyright © 2022 Collabora Ltd. and Red Hat Inc.
   * SPDX-License-Identifier: MIT
   */
   #include "nvk_sampler.h"
      #include "nvk_device.h"
   #include "nvk_entrypoints.h"
   #include "nvk_physical_device.h"
      #include "vk_format.h"
   #include "vk_sampler.h"
      #include "nouveau_context.h"
      #include "util/bitpack_helpers.h"
   #include "util/format/format_utils.h"
   #include "util/format_srgb.h"
      #include "cla097.h"
   #include "clb197.h"
   #include "cl9097tex.h"
   #include "cla097tex.h"
   #include "clb197tex.h"
   #include "drf.h"
      ALWAYS_INLINE static void
   __set_u32(uint32_t *o, uint32_t v, unsigned lo, unsigned hi)
   {
      assert(lo <= hi && hi < 32);
      }
      #define FIXED_FRAC_BITS 8
      ALWAYS_INLINE static void
   __set_ufixed(uint32_t *o, float v, unsigned lo, unsigned hi)
   {
      assert(lo <= hi && hi < 32);
      }
      ALWAYS_INLINE static void
   __set_sfixed(uint32_t *o, float v, unsigned lo, unsigned hi)
   {
      assert(lo <= hi && hi < 32);
      }
      ALWAYS_INLINE static void
   __set_bool(uint32_t *o, bool b, unsigned lo, unsigned hi)
   {
      assert(lo == hi && hi < 32);
      }
      #define MW(x) x
      #define SAMP_SET_U(o, NV, i, FIELD, val) \
      __set_u32(&(o)[i], (val), DRF_LO(NV##_TEXSAMP##i##_##FIELD),\
         #define SAMP_SET_UF(o, NV, i, FIELD, val) \
      __set_ufixed(&(o)[i], (val), DRF_LO(NV##_TEXSAMP##i##_##FIELD),\
         #define SAMP_SET_SF(o, NV, i, FIELD, val) \
      __set_sfixed(&(o)[i], (val), DRF_LO(NV##_TEXSAMP##i##_##FIELD),\
         #define SAMP_SET_B(o, NV, i, FIELD, b) \
      __set_bool(&(o)[i], (b), DRF_LO(NV##_TEXSAMP##i##_##FIELD),\
         #define SAMP_SET_E(o, NV, i, FIELD, E) \
            static inline uint32_t
   vk_to_9097_address_mode(VkSamplerAddressMode addr_mode)
   {
   #define MODE(VK, NV) \
      [VK_SAMPLER_ADDRESS_MODE_##VK] = NV9097_TEXSAMP0_ADDRESS_U_##NV
   static const uint8_t vk_to_9097[] = {
      MODE(REPEAT,               WRAP),
   MODE(MIRRORED_REPEAT,      MIRROR),
   MODE(CLAMP_TO_EDGE,        CLAMP_TO_EDGE),
   MODE(CLAMP_TO_BORDER,      BORDER),
         #undef MODE
         assert(addr_mode < ARRAY_SIZE(vk_to_9097));
      }
      static uint32_t
   vk_to_9097_texsamp_compare_op(VkCompareOp op)
   {
   #define OP(VK, NV) \
      [VK_COMPARE_OP_##VK] = NV9097_TEXSAMP0_DEPTH_COMPARE_FUNC_##NV
   ASSERTED static const uint8_t vk_to_9097[] = {
      OP(NEVER,            ZC_NEVER),
   OP(LESS,             ZC_LESS),
   OP(EQUAL,            ZC_EQUAL),
   OP(LESS_OR_EQUAL,    ZC_LEQUAL),
   OP(GREATER,          ZC_GREATER),
   OP(NOT_EQUAL,        ZC_NOTEQUAL),
   OP(GREATER_OR_EQUAL, ZC_GEQUAL),
         #undef OP
         assert(op < ARRAY_SIZE(vk_to_9097));
               }
      static uint32_t
   vk_to_9097_max_anisotropy(float max_anisotropy)
   {
      if (max_anisotropy >= 16)
            if (max_anisotropy >= 12)
            uint32_t aniso_u32 = MAX2(0.0f, max_anisotropy);
      }
      static uint32_t
   vk_to_9097_trilin_opt(float max_anisotropy)
   {
      /* No idea if we want this but matching nouveau */
   if (max_anisotropy >= 12)
            if (max_anisotropy >= 4)
            if (max_anisotropy >= 2)
               }
      static void
   nvk_sampler_fill_header(const struct nvk_physical_device *pdev,
                     {
      SAMP_SET_U(samp, NV9097, 0, ADDRESS_U,
         SAMP_SET_U(samp, NV9097, 0, ADDRESS_V,
         SAMP_SET_U(samp, NV9097, 0, ADDRESS_P,
            if (info->compareEnable) {
      SAMP_SET_B(samp, NV9097, 0, DEPTH_COMPARE, true);
   SAMP_SET_U(samp, NV9097, 0, DEPTH_COMPARE_FUNC,
               SAMP_SET_B(samp, NV9097, 0, S_R_G_B_CONVERSION, true);
   SAMP_SET_E(samp, NV9097, 0, FONT_FILTER_WIDTH, SIZE_2);
            if (info->anisotropyEnable) {
      SAMP_SET_U(samp, NV9097, 0, MAX_ANISOTROPY,
               switch (info->magFilter) {
   case VK_FILTER_NEAREST:
      SAMP_SET_E(samp, NV9097, 1, MAG_FILTER, MAG_POINT);
      case VK_FILTER_LINEAR:
      SAMP_SET_E(samp, NV9097, 1, MAG_FILTER, MAG_LINEAR);
      default:
                  switch (info->minFilter) {
   case VK_FILTER_NEAREST:
      SAMP_SET_E(samp, NV9097, 1, MIN_FILTER, MIN_POINT);
      case VK_FILTER_LINEAR:
      if (info->anisotropyEnable)
         else
            default:
                  switch (info->mipmapMode) {
   case VK_SAMPLER_MIPMAP_MODE_NEAREST:
      SAMP_SET_E(samp, NV9097, 1, MIP_FILTER, MIP_POINT);
      case VK_SAMPLER_MIPMAP_MODE_LINEAR:
      SAMP_SET_E(samp, NV9097, 1, MIP_FILTER, MIP_LINEAR);
      default:
                  assert(pdev->info.cls_eng3d >= KEPLER_A);
   if (info->flags & VK_SAMPLER_CREATE_NON_SEAMLESS_CUBE_MAP_BIT_EXT) {
         } else {
                  if (pdev->info.cls_eng3d >= MAXWELL_B) {
      switch (vk_sampler->reduction_mode) {
   case VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE:
      SAMP_SET_E(samp, NVB197, 1, REDUCTION_FILTER, RED_NONE);
      case VK_SAMPLER_REDUCTION_MODE_MIN:
      SAMP_SET_E(samp, NVB197, 1, REDUCTION_FILTER, RED_MINIMUM);
      case VK_SAMPLER_REDUCTION_MODE_MAX:
      SAMP_SET_E(samp, NVB197, 1, REDUCTION_FILTER, RED_MAXIMUM);
      default:
                              assert(pdev->info.cls_eng3d >= KEPLER_A);
   if (info->unnormalizedCoordinates) {
      SAMP_SET_E(samp, NVA097, 1, FLOAT_COORD_NORMALIZATION,
      } else {
      SAMP_SET_E(samp, NVA097, 1, FLOAT_COORD_NORMALIZATION,
      }
   SAMP_SET_U(samp, NV9097, 1, TRILIN_OPT,
            SAMP_SET_UF(samp, NV9097, 2, MIN_LOD_CLAMP, info->minLod);
            VkClearColorValue bc = vk_sampler->border_color_value;
            const VkSamplerBorderColorComponentMappingCreateInfoEXT *swiz_info =
      vk_find_struct_const(info->pNext,
      if (swiz_info) {
      if (swiz_info->srgb) {
      for (uint32_t i = 0; i < 3; i++)
               const bool is_int = vk_border_color_is_int(info->borderColor);
            for (uint32_t i = 0; i < 3; i++)
      } else {
      /* Otherwise, we can assume no swizzle or that the border color is
   * transparent black or opaque white and there's nothing to do but
   * convert the (unswizzled) border color to sRGB.
   */
   for (unsigned i = 0; i < 3; i++)
               SAMP_SET_U(samp, NV9097, 2, S_R_G_B_BORDER_COLOR_R, bc_srgb[0]);
   SAMP_SET_U(samp, NV9097, 3, S_R_G_B_BORDER_COLOR_G, bc_srgb[1]);
            SAMP_SET_U(samp, NV9097, 4, BORDER_COLOR_R, bc.uint32[0]);
   SAMP_SET_U(samp, NV9097, 5, BORDER_COLOR_G, bc.uint32[1]);
   SAMP_SET_U(samp, NV9097, 6, BORDER_COLOR_B, bc.uint32[2]);
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   nvk_CreateSampler(VkDevice device,
                     {
      VK_FROM_HANDLE(nvk_device, dev, device);
   struct nvk_sampler *sampler;
            sampler = vk_sampler_create(&dev->vk, pCreateInfo,
         if (!sampler)
            uint32_t samp[8] = {};
   sampler->plane_count = 1;
   nvk_sampler_fill_header(dev->pdev, pCreateInfo, &sampler->vk, samp);
   result = nvk_descriptor_table_add(dev, &dev->samplers,
               if (result != VK_SUCCESS) {
      vk_sampler_destroy(&dev->vk, pAllocator, &sampler->vk);
               /* In order to support CONVERSION_SEPARATE_RECONSTRUCTION_FILTER_BIT, we
   * need multiple sampler planes: at minimum we will need one for luminance
   * (the default), and one for chroma.  Each sampler plane needs its own
   * sampler table entry.  However, sampler table entries are very rare on
   * NVIDIA; we only have 4096 entries for the whole VkDevice, and each plane
   * would burn one of those. So we make sure to allocate only the minimum
   * amount that we actually need (i.e., either 1 or 2), and then just copy
   * the last sampler plane out as far as we need to fill the number of image
   * planes.
            if (sampler->vk.ycbcr_conversion) {
      const VkFilter chroma_filter =
         if (pCreateInfo->magFilter != chroma_filter ||
      pCreateInfo->minFilter != chroma_filter) {
   VkSamplerCreateInfo plane2_info = *pCreateInfo;
                  memset(samp, 0, sizeof(samp));
   sampler->plane_count = 2;
   nvk_sampler_fill_header(dev->pdev, &plane2_info, &sampler->vk, samp);
   result = nvk_descriptor_table_add(dev, &dev->samplers,
               if (result != VK_SUCCESS) {
      nvk_descriptor_table_remove(dev, &dev->samplers,
         vk_sampler_destroy(&dev->vk, pAllocator, &sampler->vk);
                                 }
      VKAPI_ATTR void VKAPI_CALL
   nvk_DestroySampler(VkDevice device,
               {
      VK_FROM_HANDLE(nvk_device, dev, device);
            if (!sampler)
            for (uint8_t plane = 0; plane < sampler->plane_count; plane++) {
      nvk_descriptor_table_remove(dev, &dev->samplers,
                  }
