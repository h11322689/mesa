   /*
   * Copyright © 2021 Collabora Ltd.
   *
   * Derived from tu_formats.c which is:
   * Copyright © 2016 Red Hat.
   * Copyright © 2016 Bas Nieuwenhuizen
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
      #include "panvk_private.h"
      #include "panfrost/lib/pan_texture.h"
   #include "util/format_r11g11b10f.h"
   #include "util/format_srgb.h"
   #include "util/half_float.h"
   #include "vulkan/util/vk_format.h"
   #include "vk_format.h"
   #include "vk_util.h"
      static void
   get_format_properties(struct panvk_physical_device *physical_device,
         {
      struct panfrost_device *pdev = &physical_device->pdev;
   VkFormatFeatureFlags tex = 0, buffer = 0;
   enum pipe_format pfmt = vk_format_to_pipe_format(format);
            if (!pfmt || !fmt.hw)
            /* 3byte formats are not supported by the buffer <-> image copy helpers. */
   if (util_format_get_blocksize(pfmt) == 3)
            /* We don't support compressed formats yet: this is causing trouble when
   * doing a vkCmdCopyImage() between a compressed and a non-compressed format
   * on a tiled/AFBC resource.
   */
   if (util_format_is_compressed(pfmt))
            buffer |=
            if (fmt.bind & PAN_BIND_VERTEX_BUFFER)
            if (fmt.bind & PAN_BIND_SAMPLER_VIEW) {
      tex |= VK_FORMAT_FEATURE_TRANSFER_SRC_BIT |
         VK_FORMAT_FEATURE_TRANSFER_DST_BIT |
   VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT |
            /* Integer formats only support nearest filtering */
   if (!util_format_is_scaled(pfmt) && !util_format_is_pure_integer(pfmt))
                                 /* SNORM rendering isn't working yet, disable */
   if (fmt.bind & PAN_BIND_RENDER_TARGET && !util_format_is_snorm(pfmt)) {
      tex |= VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT |
            tex |= VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;
            /* Can always blend via blend shaders */
               if (fmt.bind & PAN_BIND_DEPTH_STENCIL)
         end:
      out_properties->linearTilingFeatures = tex;
   out_properties->optimalTilingFeatures = tex;
      }
      void
   panvk_GetPhysicalDeviceFormatProperties(VkPhysicalDevice physicalDevice,
               {
                  }
      void
   panvk_GetPhysicalDeviceFormatProperties2(VkPhysicalDevice physicalDevice,
               {
               get_format_properties(physical_device, format,
            VkDrmFormatModifierPropertiesListEXT *list = vk_find_struct(
         if (list) {
      VK_OUTARRAY_MAKE_TYPED(VkDrmFormatModifierPropertiesEXT, out,
                  vk_outarray_append_typed(VkDrmFormatModifierProperties2EXT, &out,
         {
      mod_props->drmFormatModifier = DRM_FORMAT_MOD_LINEAR;
            }
      static VkResult
   get_image_format_properties(struct panvk_physical_device *physical_device,
                     {
      VkFormatProperties format_props;
   VkFormatFeatureFlags format_feature_flags;
   VkExtent3D maxExtent;
   uint32_t maxMipLevels;
   uint32_t maxArraySize;
   VkSampleCountFlags sampleCounts = VK_SAMPLE_COUNT_1_BIT;
                     switch (info->tiling) {
   case VK_IMAGE_TILING_LINEAR:
      format_feature_flags = format_props.linearTilingFeatures;
         case VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT:
      /* The only difference between optimal and linear is currently whether
   * depth/stencil attachments are allowed on depth/stencil formats.
   * There's no reason to allow importing depth/stencil textures, so just
   * disallow it and then this annoying edge case goes away.
   *
   * TODO: If anyone cares, we could enable this by looking at the
   * modifier and checking if it's LINEAR or not.
   */
   if (util_format_is_depth_or_stencil(format))
            assert(format_props.optimalTilingFeatures ==
            case VK_IMAGE_TILING_OPTIMAL:
      format_feature_flags = format_props.optimalTilingFeatures;
      default:
                  if (format_feature_flags == 0)
            if (info->type != VK_IMAGE_TYPE_2D &&
      util_format_is_depth_or_stencil(format))
         switch (info->type) {
   default:
         case VK_IMAGE_TYPE_1D:
      maxExtent.width = 16384;
   maxExtent.height = 1;
   maxExtent.depth = 1;
   maxMipLevels = 15; /* log2(maxWidth) + 1 */
   maxArraySize = 2048;
      case VK_IMAGE_TYPE_2D:
      maxExtent.width = 16384;
   maxExtent.height = 16384;
   maxExtent.depth = 1;
   maxMipLevels = 15; /* log2(maxWidth) + 1 */
   maxArraySize = 2048;
      case VK_IMAGE_TYPE_3D:
      maxExtent.width = 2048;
   maxExtent.height = 2048;
   maxExtent.depth = 2048;
   maxMipLevels = 12; /* log2(maxWidth) + 1 */
   maxArraySize = 1;
               if (info->tiling == VK_IMAGE_TILING_OPTIMAL &&
      info->type == VK_IMAGE_TYPE_2D &&
   (format_feature_flags &
   (VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT |
         !(info->flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) &&
   !(info->usage & VK_IMAGE_USAGE_STORAGE_BIT)) {
               if (info->usage & VK_IMAGE_USAGE_SAMPLED_BIT) {
      if (!(format_feature_flags & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) {
                     if (info->usage & VK_IMAGE_USAGE_STORAGE_BIT) {
      if (!(format_feature_flags & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT)) {
                     if (info->usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
      if (!(format_feature_flags & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT)) {
                     if (info->usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
      if (!(format_feature_flags &
                           *pImageFormatProperties = (VkImageFormatProperties){
      .maxExtent = maxExtent,
   .maxMipLevels = maxMipLevels,
   .maxArrayLayers = maxArraySize,
            /* FINISHME: Accurately calculate
   * VkImageFormatProperties::maxResourceSize.
   */
               if (p_feature_flags)
               unsupported:
      *pImageFormatProperties = (VkImageFormatProperties){
      .maxExtent = {0, 0, 0},
   .maxMipLevels = 0,
   .maxArrayLayers = 0,
   .sampleCounts = 0,
                  }
      VkResult
   panvk_GetPhysicalDeviceImageFormatProperties(
      VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type,
   VkImageTiling tiling, VkImageUsageFlags usage,
   VkImageCreateFlags createFlags,
      {
               const VkPhysicalDeviceImageFormatInfo2 info = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2,
   .pNext = NULL,
   .format = format,
   .type = type,
   .tiling = tiling,
   .usage = usage,
               return get_image_format_properties(physical_device, &info,
      }
      static VkResult
   panvk_get_external_image_format_properties(
      const struct panvk_physical_device *physical_device,
   const VkPhysicalDeviceImageFormatInfo2 *pImageFormatInfo,
   VkExternalMemoryHandleTypeFlagBits handleType,
      {
      VkExternalMemoryFeatureFlagBits flags = 0;
   VkExternalMemoryHandleTypeFlags export_flags = 0;
            /* From the Vulkan 1.1.98 spec:
   *
   *    If handleType is not compatible with the format, type, tiling,
   *    usage, and flags specified in VkPhysicalDeviceImageFormatInfo2,
   *    then vkGetPhysicalDeviceImageFormatProperties2 returns
   *    VK_ERROR_FORMAT_NOT_SUPPORTED.
   */
   switch (handleType) {
   case VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT:
   case VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT:
      switch (pImageFormatInfo->type) {
   case VK_IMAGE_TYPE_2D:
      flags = VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT |
         VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT |
   compat_flags = export_flags =
      VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT |
         default:
      return vk_errorf(
      physical_device, VK_ERROR_FORMAT_NOT_SUPPORTED,
   "VkExternalMemoryTypeFlagBits(0x%x) unsupported for VkImageType(%d)",
   }
      case VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT:
      flags = VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT;
   compat_flags = VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT;
      default:
      return vk_errorf(physical_device, VK_ERROR_FORMAT_NOT_SUPPORTED,
                     *external_properties = (VkExternalMemoryProperties){
      .externalMemoryFeatures = flags,
   .exportFromImportedHandleTypes = export_flags,
                  }
      VkResult
   panvk_GetPhysicalDeviceImageFormatProperties2(
      VkPhysicalDevice physicalDevice,
   const VkPhysicalDeviceImageFormatInfo2 *base_info,
      {
      VK_FROM_HANDLE(panvk_physical_device, physical_device, physicalDevice);
   const VkPhysicalDeviceExternalImageFormatInfo *external_info = NULL;
   const VkPhysicalDeviceImageViewImageFormatInfoEXT *image_view_info = NULL;
   VkExternalImageFormatProperties *external_props = NULL;
   VkFilterCubicImageViewImageFormatPropertiesEXT *cubic_props = NULL;
   VkFormatFeatureFlags format_feature_flags;
   VkSamplerYcbcrConversionImageFormatProperties *ycbcr_props = NULL;
            result = get_image_format_properties(physical_device, base_info,
               if (result != VK_SUCCESS)
            /* Extract input structs */
   vk_foreach_struct_const(s, base_info->pNext) {
      switch (s->sType) {
   case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO:
      external_info = (const void *)s;
      case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_VIEW_IMAGE_FORMAT_INFO_EXT:
      image_view_info = (const void *)s;
      default:
                     /* Extract output structs */
   vk_foreach_struct(s, base_props->pNext) {
      switch (s->sType) {
   case VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES:
      external_props = (void *)s;
      case VK_STRUCTURE_TYPE_FILTER_CUBIC_IMAGE_VIEW_IMAGE_FORMAT_PROPERTIES_EXT:
      cubic_props = (void *)s;
      case VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_IMAGE_FORMAT_PROPERTIES:
      ycbcr_props = (void *)s;
      default:
                     /* From the Vulkan 1.0.42 spec:
   *
   *    If handleType is 0, vkGetPhysicalDeviceImageFormatProperties2 will
   *    behave as if VkPhysicalDeviceExternalImageFormatInfo was not
   *    present and VkExternalImageFormatProperties will be ignored.
   */
   if (external_info && external_info->handleType != 0) {
      result = panvk_get_external_image_format_properties(
      physical_device, base_info, external_info->handleType,
      if (result != VK_SUCCESS)
               if (cubic_props) {
      /* note: blob only allows cubic filtering for 2D and 2D array views
   * its likely we can enable it for 1D and CUBE, needs testing however
   */
   if ((image_view_info->imageViewType == VK_IMAGE_VIEW_TYPE_2D ||
      image_view_info->imageViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY) &&
   (format_feature_flags &
   VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_CUBIC_BIT_EXT)) {
   cubic_props->filterCubic = true;
      } else {
      cubic_props->filterCubic = false;
                  if (ycbcr_props)
                  fail:
      if (result == VK_ERROR_FORMAT_NOT_SUPPORTED) {
      /* From the Vulkan 1.0.42 spec:
   *
   *    If the combination of parameters to
   *    vkGetPhysicalDeviceImageFormatProperties2 is not supported by
   *    the implementation for use in vkCreateImage, then all members of
   *    imageFormatProperties will be filled with zero.
   */
                  }
      void
   panvk_GetPhysicalDeviceSparseImageFormatProperties(
      VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type,
   VkSampleCountFlagBits samples, VkImageUsageFlags usage, VkImageTiling tiling,
      {
      /* Sparse images are not yet supported. */
      }
      void
   panvk_GetPhysicalDeviceSparseImageFormatProperties2(
      VkPhysicalDevice physicalDevice,
   const VkPhysicalDeviceSparseImageFormatInfo2 *pFormatInfo,
      {
      /* Sparse images are not yet supported. */
      }
      void
   panvk_GetPhysicalDeviceExternalBufferProperties(
      VkPhysicalDevice physicalDevice,
   const VkPhysicalDeviceExternalBufferInfo *pExternalBufferInfo,
      {
         }
