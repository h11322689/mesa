   /*
   * Copyright © 2019 Red Hat.
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
      #include "lvp_private.h"
   #include "util/detect.h"
   #include "pipe/p_defines.h"
   #include "util/format/u_format.h"
   #include "util/u_math.h"
   #include "vk_util.h"
   #include "vk_enum_defines.h"
      static bool lvp_is_filter_minmax_format_supported(VkFormat format)
   {
      /* From the Vulkan spec 1.1.71:
   *
   * "The following formats must support the
   *  VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_MINMAX_BIT feature with
   *  VK_IMAGE_TILING_OPTIMAL, if they support
   *  VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT."
   */
   /* TODO: enable more formats. */
   switch (format) {
   case VK_FORMAT_R8_UNORM:
   case VK_FORMAT_R8_SNORM:
   case VK_FORMAT_R16_UNORM:
   case VK_FORMAT_R16_SNORM:
   case VK_FORMAT_R16_SFLOAT:
   case VK_FORMAT_R32_SFLOAT:
   case VK_FORMAT_D16_UNORM:
   case VK_FORMAT_X8_D24_UNORM_PACK32:
   case VK_FORMAT_D32_SFLOAT:
   case VK_FORMAT_D16_UNORM_S8_UINT:
   case VK_FORMAT_D24_UNORM_S8_UINT:
   case VK_FORMAT_D32_SFLOAT_S8_UINT:
         default:
            }
         static void
   lvp_physical_device_get_format_properties(struct lvp_physical_device *physical_device,
               {
      const enum pipe_format pformat = lvp_vk_format_to_pipe_format(format);
   struct pipe_screen *pscreen = physical_device->pscreen;
            if (pformat == PIPE_FORMAT_NONE) {
   out_properties->linearTilingFeatures = 0;
   out_properties->optimalTilingFeatures = 0;
   out_properties->bufferFeatures = 0;
   return;
            if (pscreen->is_format_supported(pscreen, pformat, PIPE_TEXTURE_2D, 0, 0,
            out_properties->linearTilingFeatures = 0;
   out_properties->optimalTilingFeatures =
      (VK_FORMAT_FEATURE_2_DEPTH_STENCIL_ATTACHMENT_BIT |
   VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_BIT |
   VK_FORMAT_FEATURE_2_TRANSFER_SRC_BIT |
   VK_FORMAT_FEATURE_2_TRANSFER_DST_BIT |
   VK_FORMAT_FEATURE_2_BLIT_SRC_BIT | VK_FORMAT_FEATURE_2_BLIT_DST_BIT |
               if (lvp_is_filter_minmax_format_supported(format))
      out_properties->optimalTilingFeatures |=
      out_properties->bufferFeatures = 0;
               if (util_format_is_compressed(pformat)) {
      if (pscreen->is_format_supported(pscreen, pformat, PIPE_TEXTURE_2D, 0, 0,
            features |= (VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_BIT |
               VK_FORMAT_FEATURE_2_BLIT_SRC_BIT |
   VK_FORMAT_FEATURE_2_TRANSFER_SRC_BIT |
      }
   out_properties->linearTilingFeatures = features;
   out_properties->optimalTilingFeatures = features;
   out_properties->bufferFeatures = buffer_features;
               if (!util_format_is_srgb(pformat) &&
      pscreen->is_format_supported(pscreen, pformat, PIPE_BUFFER, 0, 0,
                     if (pscreen->is_format_supported(pscreen, pformat, PIPE_BUFFER, 0, 0,
                        if (pscreen->is_format_supported(pscreen, pformat, PIPE_BUFFER, 0, 0,
            buffer_features |= VK_FORMAT_FEATURE_2_STORAGE_TEXEL_BUFFER_BIT |
                     if (pscreen->is_format_supported(pscreen, pformat, PIPE_TEXTURE_2D, 0, 0,
            features |= VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_BIT;
   if (util_format_has_depth(util_format_description(pformat)))
         if (!util_format_is_pure_integer(pformat))
         if (lvp_is_filter_minmax_format_supported(format))
         const struct vk_format_ycbcr_info *ycbcr_info =
         if (ycbcr_info) {
      if (ycbcr_info->n_planes > 1)
                        for (uint8_t plane = 0; plane < ycbcr_info->n_planes; plane++) {
      const struct vk_format_ycbcr_plane *plane_info = &ycbcr_info->planes[plane];
   if (plane_info->denominator_scales[0] > 1 ||
                     /* The subsampled formats have no support for linear filters. */
   const struct util_format_description *desc = util_format_description(pformat);
   if (desc->layout != UTIL_FORMAT_LAYOUT_SUBSAMPLED)
                  if (pscreen->is_format_supported(pscreen, pformat, PIPE_TEXTURE_2D, 0, 0,
            features |= VK_FORMAT_FEATURE_2_COLOR_ATTACHMENT_BIT;
   /* SNORM blending on llvmpipe fails CTS - disable for now */
   if (!util_format_is_snorm(pformat) &&
      !util_format_is_pure_integer(pformat))
            if (pscreen->is_format_supported(pscreen, pformat, PIPE_TEXTURE_2D, 0, 0,
            features |= VK_FORMAT_FEATURE_2_STORAGE_IMAGE_BIT |
                     if (pformat == PIPE_FORMAT_R32_UINT ||
      pformat == PIPE_FORMAT_R32_SINT ||
   pformat == PIPE_FORMAT_R32_FLOAT) {
   features |= VK_FORMAT_FEATURE_2_STORAGE_IMAGE_ATOMIC_BIT;
      } else if (pformat == PIPE_FORMAT_R11G11B10_FLOAT ||
                        if (features && buffer_features != VK_FORMAT_FEATURE_2_VERTEX_BUFFER_BIT) {
      features |= (VK_FORMAT_FEATURE_2_TRANSFER_SRC_BIT |
      }
   if (pformat == PIPE_FORMAT_B5G6R5_UNORM) {
      features |= (VK_FORMAT_FEATURE_2_BLIT_SRC_BIT |
      }
   if ((pformat != PIPE_FORMAT_R9G9B9E5_FLOAT) &&
      util_format_get_nr_components(pformat) != 3 &&
   !util_format_is_subsampled_422(pformat) &&
   !util_format_is_yuv(pformat) &&
   pformat != PIPE_FORMAT_R10G10B10A2_SNORM &&
   pformat != PIPE_FORMAT_B10G10R10A2_SNORM &&
   pformat != PIPE_FORMAT_B10G10R10A2_UNORM) {
   features |= (VK_FORMAT_FEATURE_2_BLIT_SRC_BIT |
               out_properties->linearTilingFeatures = features;
   out_properties->optimalTilingFeatures = features;
   out_properties->bufferFeatures = buffer_features;
   if (out_properties->linearTilingFeatures)
         if (out_properties->optimalTilingFeatures)
      }
         VKAPI_ATTR void VKAPI_CALL lvp_GetPhysicalDeviceFormatProperties2(
         VkPhysicalDevice                            physicalDevice,
   VkFormat                                    format,
   {
               VkFormatProperties3 format_props;
   lvp_physical_device_get_format_properties(physical_device,
               pFormatProperties->formatProperties.linearTilingFeatures = vk_format_features2_to_features(format_props.linearTilingFeatures);
   pFormatProperties->formatProperties.optimalTilingFeatures = vk_format_features2_to_features(format_props.optimalTilingFeatures);
   pFormatProperties->formatProperties.bufferFeatures = vk_format_features2_to_features(format_props.bufferFeatures);
   VkFormatProperties3 *prop3 = (void*)vk_find_struct_const(pFormatProperties->pNext, FORMAT_PROPERTIES_3);
   if (prop3) {
      prop3->linearTilingFeatures = format_props.linearTilingFeatures;
   prop3->optimalTilingFeatures = format_props.optimalTilingFeatures;
      }
   VkSubpassResolvePerformanceQueryEXT *perf = (void*)vk_find_struct_const(pFormatProperties->pNext, SUBPASS_RESOLVE_PERFORMANCE_QUERY_EXT);
   if (perf)
      }
      static VkResult lvp_get_image_format_properties(struct lvp_physical_device *physical_device,
               {
      VkFormatProperties3 format_props;
   VkFormatFeatureFlags2 format_feature_flags;
   VkExtent3D maxExtent;
   uint32_t maxMipLevels;
   uint32_t maxArraySize;
   VkSampleCountFlags sampleCounts = VK_SAMPLE_COUNT_1_BIT;
   enum pipe_format pformat = lvp_vk_format_to_pipe_format(info->format);
   lvp_physical_device_get_format_properties(physical_device, info->format,
         if (info->tiling == VK_IMAGE_TILING_LINEAR) {
         } else if (info->tiling == VK_IMAGE_TILING_OPTIMAL) {
         } else {
                  if (format_feature_flags == 0)
            uint32_t max_2d_ext = physical_device->pscreen->get_param(physical_device->pscreen, PIPE_CAP_MAX_TEXTURE_2D_SIZE);
   uint32_t max_layers = physical_device->pscreen->get_param(physical_device->pscreen, PIPE_CAP_MAX_TEXTURE_ARRAY_LAYERS);
   switch (info->type) {
   default:
         case VK_IMAGE_TYPE_1D:
      if (util_format_is_compressed(pformat))
            maxExtent.width = max_2d_ext;
   maxExtent.height = 1;
   maxExtent.depth = 1;
   maxMipLevels = util_logbase2(max_2d_ext) + 1;
   maxArraySize = max_layers;
      case VK_IMAGE_TYPE_2D:
      maxExtent.width = max_2d_ext;
   maxExtent.height = max_2d_ext;
   maxExtent.depth = 1;
   maxMipLevels = util_logbase2(max_2d_ext) + 1;
   maxArraySize = max_layers;
   if (info->tiling == VK_IMAGE_TILING_OPTIMAL &&
      !(info->flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) &&
   !util_format_is_compressed(pformat) &&
   (format_feature_flags & (VK_FORMAT_FEATURE_2_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_2_DEPTH_STENCIL_ATTACHMENT_BIT)))
         case VK_IMAGE_TYPE_3D:
      maxExtent.width = max_2d_ext;
   maxExtent.height = max_2d_ext;
   maxExtent.depth = (1 << physical_device->pscreen->get_param(physical_device->pscreen, PIPE_CAP_MAX_TEXTURE_3D_LEVELS));
   maxMipLevels = util_logbase2(max_2d_ext) + 1;
   maxArraySize = 1;
               if (info->flags & VK_IMAGE_CREATE_EXTENDED_USAGE_BIT)
            if (info->usage & VK_IMAGE_USAGE_SAMPLED_BIT) {
      if (!(format_feature_flags & VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_BIT)) {
                     if (info->usage & VK_IMAGE_USAGE_STORAGE_BIT) {
      if (!(format_feature_flags & VK_FORMAT_FEATURE_2_STORAGE_IMAGE_BIT)) {
                     if (info->usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
      if (!(format_feature_flags & VK_FORMAT_FEATURE_2_COLOR_ATTACHMENT_BIT)) {
                     if (info->usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
      if (!(format_feature_flags & VK_FORMAT_FEATURE_2_DEPTH_STENCIL_ATTACHMENT_BIT)) {
                     if (info->usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
      if (!(format_feature_flags & VK_FORMAT_FEATURE_2_TRANSFER_SRC_BIT)) {
                     if (info->usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
      if (!(format_feature_flags & VK_FORMAT_FEATURE_2_TRANSFER_DST_BIT)) {
                     if (info->usage & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT) {
      if (!(format_feature_flags & (VK_FORMAT_FEATURE_2_COLOR_ATTACHMENT_BIT |
                        skip_checks:
      *pImageFormatProperties = (VkImageFormatProperties) {
      .maxExtent = maxExtent,
   .maxMipLevels = maxMipLevels,
   .maxArrayLayers = maxArraySize,
            /* FINISHME: Accurately calculate
   * VkImageFormatProperties::maxResourceSize.
   */
      };
      unsupported:
      *pImageFormatProperties = (VkImageFormatProperties) {
      .maxExtent = { 0, 0, 0 },
   .maxMipLevels = 0,
   .maxArrayLayers = 0,
   .sampleCounts = 0,
                  }
      VKAPI_ATTR VkResult VKAPI_CALL lvp_GetPhysicalDeviceImageFormatProperties2(
         VkPhysicalDevice                            physicalDevice,
   const VkPhysicalDeviceImageFormatInfo2     *base_info,
   {
      LVP_FROM_HANDLE(lvp_physical_device, physical_device, physicalDevice);
   const VkPhysicalDeviceExternalImageFormatInfo *external_info = NULL;
   VkExternalImageFormatProperties *external_props = NULL;
   VkSamplerYcbcrConversionImageFormatProperties *ycbcr_props = NULL;
   VkHostImageCopyDevicePerformanceQueryEXT *hic;
   VkResult result;
   result = lvp_get_image_format_properties(physical_device, base_info,
         if (result != VK_SUCCESS)
            vk_foreach_struct_const(s, base_info->pNext) {
      switch (s->sType) {
   case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO:
      external_info = (const void *) s;
      default:
                     /* Extract output structs */
   vk_foreach_struct(s, base_props->pNext) {
      switch (s->sType) {
   case VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES:
      external_props = (void *) s;
      case VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_IMAGE_FORMAT_PROPERTIES:
      ycbcr_props = (void *) s;
      case VK_STRUCTURE_TYPE_HOST_IMAGE_COPY_DEVICE_PERFORMANCE_QUERY_EXT:
      hic = (void*)s;
   hic->optimalDeviceAccess = VK_TRUE;
   hic->identicalMemoryLayout = VK_TRUE;
      default:
                     if (external_info && external_info->handleType != 0) {
      VkExternalMemoryFeatureFlagBits flags = 0;
   VkExternalMemoryHandleTypeFlags export_flags = 0;
            #ifdef PIPE_MEMORY_FD
         case VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT:
      flags = VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT | VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT;
   export_flags = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
      #endif
         case VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT:
      flags = VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT;
   compat_flags = VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT;
      default:
         }
   external_props->externalMemoryProperties = (VkExternalMemoryProperties) {
      .externalMemoryFeatures = flags,
   .exportFromImportedHandleTypes = export_flags,
         }
   if (ycbcr_props)
            }
      VKAPI_ATTR void VKAPI_CALL lvp_GetPhysicalDeviceSparseImageFormatProperties(
      VkPhysicalDevice                            physicalDevice,
   VkFormat                                    format,
   VkImageType                                 type,
   VkSampleCountFlagBits                       samples,
   VkImageUsageFlags                           usage,
   VkImageTiling                               tiling,
   uint32_t*                                   pNumProperties,
      {
      /* Sparse images are not yet supported. */
      }
      VKAPI_ATTR void VKAPI_CALL lvp_GetPhysicalDeviceSparseImageFormatProperties2(
         VkPhysicalDevice                            physicalDevice,
   const VkPhysicalDeviceSparseImageFormatInfo2 *pFormatInfo,
   uint32_t                                   *pPropertyCount,
   {
         /* Sparse images are not yet supported. */
   }
      VKAPI_ATTR void VKAPI_CALL lvp_GetPhysicalDeviceExternalBufferProperties(
      VkPhysicalDevice                            physicalDevice,
   const VkPhysicalDeviceExternalBufferInfo    *pExternalBufferInfo,
      {
      VkExternalMemoryFeatureFlagBits flags = 0;
   VkExternalMemoryHandleTypeFlags export_flags = 0;
   VkExternalMemoryHandleTypeFlags compat_flags = 0;
      #ifdef PIPE_MEMORY_FD
      case VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT:
      flags = VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT | VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT;
   export_flags = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
   compat_flags = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
   #endif
      case VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT:
      flags = VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT;
   compat_flags = VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT;
      default:
                  pExternalBufferProperties->externalMemoryProperties = (VkExternalMemoryProperties) {
      .externalMemoryFeatures = flags,
   .exportFromImportedHandleTypes = export_flags,
         }
