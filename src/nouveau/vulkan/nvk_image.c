   /*
   * Copyright © 2022 Collabora Ltd. and Red Hat Inc.
   * SPDX-License-Identifier: MIT
   */
   #include "nvk_image.h"
      #include "nvk_device.h"
   #include "nvk_device_memory.h"
   #include "nvk_entrypoints.h"
   #include "nvk_format.h"
   #include "nvk_physical_device.h"
      #include "nil_format.h"
   #include "vulkan/util/vk_format.h"
      #include "clb097.h"
   #include "clb197.h"
      static VkFormatFeatureFlags2
   nvk_get_image_plane_format_features(struct nvk_physical_device *pdev,
         {
               if (tiling != VK_IMAGE_TILING_OPTIMAL)
            enum pipe_format p_format = vk_format_to_pipe_format(vk_format);
   if (p_format == PIPE_FORMAT_NONE)
            if (!nil_format_supports_texturing(&pdev->info, p_format))
            /* You can't tile a non-power-of-two */
   if (!util_is_power_of_two_nonzero(util_format_get_blocksize(p_format)))
            features |= VK_FORMAT_FEATURE_2_TRANSFER_SRC_BIT;
   features |= VK_FORMAT_FEATURE_2_TRANSFER_DST_BIT;
   features |= VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_BIT;
            if (nil_format_supports_filtering(&pdev->info, p_format)) {
      features |= VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
   if (pdev->info.cls_eng3d >= MAXWELL_B)
               /* TODO: VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_DEPTH_COMPARISON_BIT */
   if (vk_format_has_depth(vk_format)) {
                  if (nil_format_supports_color_targets(&pdev->info, p_format)) {
      features |= VK_FORMAT_FEATURE_2_COLOR_ATTACHMENT_BIT;
   if (nil_format_supports_blending(&pdev->info, p_format))
                     if (vk_format_is_depth_or_stencil(vk_format)) {
      if (!nil_format_supports_depth_stencil(&pdev->info, p_format))
                        if (nil_format_supports_storage(&pdev->info, p_format)) {
      features |= VK_FORMAT_FEATURE_2_STORAGE_IMAGE_BIT |
         if (pdev->info.cls_eng3d >= MAXWELL_A)
               if (p_format == PIPE_FORMAT_R32_UINT || p_format == PIPE_FORMAT_R32_SINT)
               }
      VkFormatFeatureFlags2
   nvk_get_image_format_features(struct nvk_physical_device *pdev,
         {
      const struct vk_format_ycbcr_info *ycbcr_info =
         if (ycbcr_info == NULL)
            /* For multi-plane, we get the feature flags of each plane separately,
   * then take their intersection as the overall format feature flags
   */
   VkFormatFeatureFlags2 features = ~0ull;
   bool cosited_chroma = false;
   for (uint8_t plane = 0; plane < ycbcr_info->n_planes; plane++) {
      const struct vk_format_ycbcr_plane *plane_info = &ycbcr_info->planes[plane];
   features &= nvk_get_image_plane_format_features(pdev, plane_info->format,
         if (plane_info->denominator_scales[0] > 1 ||
      plane_info->denominator_scales[1] > 1)
   }
   if (features == 0)
            /* Uh... We really should be able to sample from YCbCr */
   assert(features & VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_BIT);
            /* These aren't allowed for YCbCr formats */
   features &= ~(VK_FORMAT_FEATURE_2_BLIT_SRC_BIT |
               VK_FORMAT_FEATURE_2_BLIT_DST_BIT |
            /* This is supported on all YCbCr formats */
            if (ycbcr_info->n_planes > 1) {
      /* DISJOINT_BIT implies that each plane has its own separate binding,
   * while SEPARATE_RECONSTRUCTION_FILTER_BIT implies that luma and chroma
   * each have their own, separate filters, so these two bits make sense
   * for multi-planar formats only.
   *
   * For MIDPOINT_CHROMA_SAMPLES_BIT, NVIDIA HW on single-plane interleaved
   * YCbCr defaults to COSITED_EVEN, which is inaccurate and fails tests.
   * This can be fixed with a NIR tweak but for now, we only enable this bit
   * for multi-plane formats. See Issue #9525 on the mesa/main tracker.
   */
   features |= VK_FORMAT_FEATURE_DISJOINT_BIT |
                     if (cosited_chroma)
               }
      static VkFormatFeatureFlags2KHR
   vk_image_usage_to_format_features(VkImageUsageFlagBits usage_flag)
   {
      assert(util_bitcount(usage_flag) == 1);
   switch (usage_flag) {
   case VK_IMAGE_USAGE_TRANSFER_SRC_BIT:
      return VK_FORMAT_FEATURE_2_TRANSFER_SRC_BIT_KHR |
      case VK_IMAGE_USAGE_TRANSFER_DST_BIT:
      return VK_FORMAT_FEATURE_2_TRANSFER_DST_BIT_KHR |
      case VK_IMAGE_USAGE_SAMPLED_BIT:
         case VK_IMAGE_USAGE_STORAGE_BIT:
         case VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT:
         case VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT:
         default:
            }
      uint32_t
   nvk_image_max_dimension(const struct nv_device_info *info,
         {
      switch (image_type) {
   case VK_IMAGE_TYPE_1D:
   case VK_IMAGE_TYPE_2D:
         case VK_IMAGE_TYPE_3D:
         default:
            }
      VKAPI_ATTR VkResult VKAPI_CALL
   nvk_GetPhysicalDeviceImageFormatProperties2(
      VkPhysicalDevice physicalDevice,
   const VkPhysicalDeviceImageFormatInfo2 *pImageFormatInfo,
      {
               const VkPhysicalDeviceExternalImageFormatInfo *external_info =
      vk_find_struct_const(pImageFormatInfo->pNext,
         /* Initialize to zero in case we return VK_ERROR_FORMAT_NOT_SUPPORTED */
   memset(&pImageFormatProperties->imageFormatProperties, 0,
            const struct vk_format_ycbcr_info *ycbcr_info =
            /* For the purposes of these checks, we don't care about all the extra
   * YCbCr features and we just want the accumulation of features available
   * to all planes of the given format.
   */
   VkFormatFeatureFlags2 features;
   if (ycbcr_info == NULL) {
      features = nvk_get_image_plane_format_features(
      } else {
      features = ~0ull;
   assert(ycbcr_info->n_planes > 0);
   for (uint8_t plane = 0; plane < ycbcr_info->n_planes; plane++) {
      const VkFormat plane_format = ycbcr_info->planes[plane].format;
   features &= nvk_get_image_plane_format_features(
         }
   if (features == 0)
            if (vk_format_is_compressed(pImageFormatInfo->format) &&
      pImageFormatInfo->type != VK_IMAGE_TYPE_2D)
         if (ycbcr_info && pImageFormatInfo->type != VK_IMAGE_TYPE_2D)
            const uint32_t max_dim =
         VkExtent3D maxExtent;
   uint32_t maxArraySize;
   switch (pImageFormatInfo->type) {
   case VK_IMAGE_TYPE_1D:
      maxExtent = (VkExtent3D) { max_dim, 1, 1 };
   maxArraySize = 2048;
      case VK_IMAGE_TYPE_2D:
      maxExtent = (VkExtent3D) { max_dim, max_dim, 1 };
   maxArraySize = 2048;
      case VK_IMAGE_TYPE_3D:
      maxExtent = (VkExtent3D) { max_dim, max_dim, max_dim };
   maxArraySize = 1;
      default:
                  assert(util_is_power_of_two_nonzero(max_dim));
   uint32_t maxMipLevels = util_logbase2(max_dim) + 1;
   if (ycbcr_info != NULL)
            VkSampleCountFlags sampleCounts = VK_SAMPLE_COUNT_1_BIT;
   if (pImageFormatInfo->tiling == VK_IMAGE_TILING_OPTIMAL &&
      pImageFormatInfo->type == VK_IMAGE_TYPE_2D &&
   ycbcr_info == NULL &&
   (features & (VK_FORMAT_FEATURE_2_COLOR_ATTACHMENT_BIT |
         !(pImageFormatInfo->flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) &&
   !(pImageFormatInfo->usage & VK_IMAGE_USAGE_STORAGE_BIT)) {
   sampleCounts = VK_SAMPLE_COUNT_1_BIT |
                           /* From the Vulkan 1.2.199 spec:
   *
   *    "VK_IMAGE_CREATE_EXTENDED_USAGE_BIT specifies that the image can be
   *    created with usage flags that are not supported for the format the
   *    image is created with but are supported for at least one format a
   *    VkImageView created from the image can have."
   *
   * If VK_IMAGE_CREATE_EXTENDED_USAGE_BIT is set, views can be created with
   * different usage than the image so we can't always filter on usage.
   * There is one exception to this below for storage.
   */
   const VkImageUsageFlags image_usage = pImageFormatInfo->usage;
   VkImageUsageFlags view_usage = image_usage;
   if (pImageFormatInfo->flags & VK_IMAGE_CREATE_EXTENDED_USAGE_BIT)
            u_foreach_bit(b, view_usage) {
      VkFormatFeatureFlags2KHR usage_features =
         if (usage_features && !(features & usage_features))
               const VkExternalMemoryProperties *ext_mem_props = NULL;
   if (external_info != NULL && external_info->handleType != 0) {
      bool tiling_has_explicit_layout;
   switch (pImageFormatInfo->tiling) {
   case VK_IMAGE_TILING_LINEAR:
   case VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT:
      tiling_has_explicit_layout = true;
      case VK_IMAGE_TILING_OPTIMAL:
      tiling_has_explicit_layout = false;
      default:
                  switch (external_info->handleType) {
   case VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT:
      /* No special restrictions */
   if (tiling_has_explicit_layout) {
      /* With an explicit memory layout, we don't care which type of
   * fd the image belongs too. Both OPAQUE_FD and DMA_BUF are
   * interchangeable here.
   */
      } else {
                     case VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT:
      if (!tiling_has_explicit_layout) {
      return vk_errorf(pdev, VK_ERROR_FORMAT_NOT_SUPPORTED,
                  }
               default:
      /* From the Vulkan 1.3.256 spec:
   *
   *    "If handleType is not compatible with the [parameters] in
   *    VkPhysicalDeviceImageFormatInfo2, then
   *    vkGetPhysicalDeviceImageFormatProperties2 returns
   *    VK_ERROR_FORMAT_NOT_SUPPORTED."
   */
   return vk_errorf(pdev, VK_ERROR_FORMAT_NOT_SUPPORTED,
                        const unsigned plane_count =
            /* From the Vulkan 1.3.259 spec, VkImageCreateInfo:
   *
   *    VUID-VkImageCreateInfo-imageCreateFormatFeatures-02260
   *
   *    "If format is a multi-planar format, and if imageCreateFormatFeatures
   *    (as defined in Image Creation Limits) does not contain
   *    VK_FORMAT_FEATURE_DISJOINT_BIT, then flags must not contain
   *    VK_IMAGE_CREATE_DISJOINT_BIT"
   *
   * This is satisfied trivially because we support DISJOINT on all
   * multi-plane formats.  Also,
   *
   *    VUID-VkImageCreateInfo-format-01577
   *
   *    "If format is not a multi-planar format, and flags does not include
   *    VK_IMAGE_CREATE_ALIAS_BIT, flags must not contain
   *    VK_IMAGE_CREATE_DISJOINT_BIT"
   */
   if (plane_count == 1 &&
      !(pImageFormatInfo->flags & VK_IMAGE_CREATE_ALIAS_BIT) &&
   (pImageFormatInfo->flags & VK_IMAGE_CREATE_DISJOINT_BIT))
         pImageFormatProperties->imageFormatProperties = (VkImageFormatProperties) {
      .maxExtent = maxExtent,
   .maxMipLevels = maxMipLevels,
   .maxArrayLayers = maxArraySize,
   .sampleCounts = sampleCounts,
               vk_foreach_struct(s, pImageFormatProperties->pNext) {
      switch (s->sType) {
   case VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES: {
      VkExternalImageFormatProperties *p = (void *)s;
   /* From the Vulkan 1.3.256 spec:
   *
   *    "If handleType is 0, vkGetPhysicalDeviceImageFormatProperties2
   *    will behave as if VkPhysicalDeviceExternalImageFormatInfo was
   *    not present, and VkExternalImageFormatProperties will be
   *    ignored."
   *
   * This is true if and only if ext_mem_props == NULL
   */
   if (ext_mem_props != NULL)
            }
   case VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_IMAGE_FORMAT_PROPERTIES: {
      VkSamplerYcbcrConversionImageFormatProperties *ycbcr_props = (void *) s;
   ycbcr_props->combinedImageSamplerDescriptorCount = plane_count;
      }
   default:
      nvk_debug_ignored_stype(s->sType);
                     }
      VKAPI_ATTR void VKAPI_CALL
   nvk_GetPhysicalDeviceSparseImageFormatProperties2(
      VkPhysicalDevice physicalDevice,
   const VkPhysicalDeviceSparseImageFormatInfo2* pFormatInfo,
   uint32_t *pPropertyCount,
      {
      /* Sparse images are not yet supported. */
      }
      static enum nil_image_dim
   vk_image_type_to_nil_dim(VkImageType type)
   {
      switch (type) {
   case VK_IMAGE_TYPE_1D:  return NIL_IMAGE_DIM_1D;
   case VK_IMAGE_TYPE_2D:  return NIL_IMAGE_DIM_2D;
   case VK_IMAGE_TYPE_3D:  return NIL_IMAGE_DIM_3D;
   default:
            }
      static VkResult
   nvk_image_init(struct nvk_device *dev,
               {
               if ((image->vk.usage & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            image->vk.samples > 1) {
   image->vk.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
               if (image->vk.usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
         if (image->vk.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
            enum nil_image_usage_flags usage = 0; /* TODO */
   if (pCreateInfo->tiling == VK_IMAGE_TILING_LINEAR)
         if (pCreateInfo->flags & VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT)
         if (pCreateInfo->flags & VK_IMAGE_CREATE_2D_VIEW_COMPATIBLE_BIT_EXT)
            /* We treat 3D storage images as 2D arrays.  One day, we may wire up actual
   * 3D storage image support but baseArrayLayer gets tricky.
   */
   if (image->vk.usage & VK_IMAGE_USAGE_STORAGE_BIT)
            /* In order to be able to clear 3D depth/stencil images, we need to bind
   * them as 2D arrays.  Fortunately, 3D depth/stencil shouldn't be common.
   */
   if ((image->vk.aspects & (VK_IMAGE_ASPECT_DEPTH_BIT |
            pCreateInfo->imageType == VK_IMAGE_TYPE_3D)
         image->plane_count = vk_format_get_plane_count(pCreateInfo->format);
   image->disjoint = image->plane_count > 1 &&
            const struct vk_format_ycbcr_info *ycbcr_info =
         for (uint8_t plane = 0; plane < image->plane_count; plane++) {
      VkFormat format = ycbcr_info ?
         const uint8_t width_scale = ycbcr_info ?
         const uint8_t height_scale = ycbcr_info ?
         struct nil_image_init_info nil_info = {
      .dim = vk_image_type_to_nil_dim(pCreateInfo->imageType),
   .format = vk_format_to_pipe_format(format),
   .extent_px = {
      .w = pCreateInfo->extent.width / width_scale,
   .h = pCreateInfo->extent.height / height_scale,
   .d = pCreateInfo->extent.depth,
      },
   .levels = pCreateInfo->mipLevels,
   .samples = pCreateInfo->samples,
               ASSERTED bool ok = nil_image_init(&nvk_device_physical(dev)->info,
                     if (image->vk.format == VK_FORMAT_D32_SFLOAT_S8_UINT) {
      struct nil_image_init_info stencil_nil_info = {
      .dim = vk_image_type_to_nil_dim(pCreateInfo->imageType),
   .format = PIPE_FORMAT_R32_UINT,
   .extent_px = {
      .w = pCreateInfo->extent.width,
   .h = pCreateInfo->extent.height,
   .d = pCreateInfo->extent.depth,
      },
   .levels = pCreateInfo->mipLevels,
   .samples = pCreateInfo->samples,
               ASSERTED bool ok = nil_image_init(&nvk_device_physical(dev)->info,
                              }
      static VkResult
   nvk_image_plane_alloc_vma(struct nvk_device *dev,
               {
      const bool sparse_bound =
         const bool sparse_resident =
                  if (sparse_bound || plane->nil.pte_kind) {
      plane->vma_size_B = plane->nil.size_B;
   plane->addr = nouveau_ws_alloc_vma(dev->ws_dev, plane->vma_size_B,
                        }
         static void
   nvk_image_plane_finish(struct nvk_device *dev,
                     {
      if (plane->vma_size_B) {
      const bool sparse_resident =
            nouveau_ws_bo_unbind_vma(dev->ws_dev, plane->addr, plane->vma_size_B);
   nouveau_ws_free_vma(dev->ws_dev, plane->addr, plane->vma_size_B,
         }
      static void
   nvk_image_finish(struct nvk_device *dev, struct nvk_image *image,
         {
      for (uint8_t plane = 0; plane < image->plane_count; plane++) {
      nvk_image_plane_finish(dev, &image->planes[plane],
               if (image->stencil_copy_temp.nil.size_B > 0) {
      nvk_image_plane_finish(dev, &image->stencil_copy_temp,
                  }
      VKAPI_ATTR VkResult VKAPI_CALL
   nvk_CreateImage(VkDevice device,
                     {
      VK_FROM_HANDLE(nvk_device, dev, device);
   struct nvk_image *image;
            image = vk_zalloc2(&dev->vk.alloc, pAllocator, sizeof(*image), 8,
         if (!image)
            result = nvk_image_init(dev, image, pCreateInfo);
   if (result != VK_SUCCESS) {
      vk_free2(&dev->vk.alloc, pAllocator, image);
               for (uint8_t plane = 0; plane < image->plane_count; plane++) {
      result = nvk_image_plane_alloc_vma(dev, &image->planes[plane],
         if (result != VK_SUCCESS) {
      nvk_image_finish(dev, image, pAllocator);
   vk_free2(&dev->vk.alloc, pAllocator, image);
                  if (image->stencil_copy_temp.nil.size_B > 0) {
      result = nvk_image_plane_alloc_vma(dev, &image->stencil_copy_temp,
         if (result != VK_SUCCESS) {
      nvk_image_finish(dev, image, pAllocator);
   vk_free2(&dev->vk.alloc, pAllocator, image);
                              }
      VKAPI_ATTR void VKAPI_CALL
   nvk_DestroyImage(VkDevice device,
               {
      VK_FROM_HANDLE(nvk_device, dev, device);
            if (!image)
            nvk_image_finish(dev, image, pAllocator);
      }
      static void
   nvk_image_plane_add_req(struct nvk_image_plane *plane,
         {
      assert(util_is_power_of_two_or_zero64(*align_B));
            *align_B = MAX2(*align_B, plane->nil.align_B);
   *size_B = align64(*size_B, plane->nil.align_B);
      }
      static void
   nvk_get_image_memory_requirements(struct nvk_device *dev,
                     {
                        uint64_t size_B = 0;
   uint32_t align_B = 0;
   if (image->disjoint) {
      uint8_t plane = nvk_image_aspects_to_plane(image, aspects);
      } else {
      for (unsigned plane = 0; plane < image->plane_count; plane++)
               assert(image->vk.external_handle_types == 0 || image->plane_count == 1);
   bool needs_dedicated =
      image->vk.external_handle_types != 0 &&
         if (image->stencil_copy_temp.nil.size_B > 0)
            pMemoryRequirements->memoryRequirements.memoryTypeBits = memory_types;
   pMemoryRequirements->memoryRequirements.alignment = align_B;
            vk_foreach_struct_const(ext, pMemoryRequirements->pNext) {
      switch (ext->sType) {
   case VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS: {
      VkMemoryDedicatedRequirements *dedicated = (void *)ext;
   dedicated->prefersDedicatedAllocation = needs_dedicated;
   dedicated->requiresDedicatedAllocation = needs_dedicated;
      }
   default:
      nvk_debug_ignored_stype(ext->sType);
            }
      VKAPI_ATTR void VKAPI_CALL
   nvk_GetImageMemoryRequirements2(VkDevice device,
               {
      VK_FROM_HANDLE(nvk_device, dev, device);
            const VkImagePlaneMemoryRequirementsInfo *plane_info =
         const VkImageAspectFlags aspects =
            nvk_get_image_memory_requirements(dev, image, aspects,
      }
      VKAPI_ATTR void VKAPI_CALL 
   nvk_GetDeviceImageMemoryRequirements(VkDevice device,
               {
      VK_FROM_HANDLE(nvk_device, dev, device);
   ASSERTED VkResult result;
            result = nvk_image_init(dev, &image, pInfo->pCreateInfo);
            const VkImageAspectFlags aspects =
            nvk_get_image_memory_requirements(dev, &image, aspects,
               }
      VKAPI_ATTR void VKAPI_CALL
   nvk_GetImageSparseMemoryRequirements2(VkDevice device,
                     {
      /* We dont support sparse images yet, this is a stub to get KHR_get_memory_requirements2 */
      }
      VKAPI_ATTR void VKAPI_CALL
   nvk_GetDeviceImageSparseMemoryRequirements(VkDevice device,
                     {
      /* Sparse images are not supported so this is just a stub for now. */
      }
      static void
   nvk_get_image_subresource_layout(UNUSED struct nvk_device *dev,
                     {
               const uint8_t p = nvk_image_aspects_to_plane(image, isr->aspectMask);
            pLayout->subresourceLayout = (VkSubresourceLayout) {
      .offset = nil_image_level_layer_offset_B(&plane->nil, isr->mipLevel,
         .size = nil_image_level_size_B(&plane->nil, isr->mipLevel),
   .rowPitch = plane->nil.levels[isr->mipLevel].row_stride_B,
   .arrayPitch = plane->nil.array_stride_B,
         }
      VKAPI_ATTR void VKAPI_CALL
   nvk_GetImageSubresourceLayout2KHR(VkDevice device,
                     {
      VK_FROM_HANDLE(nvk_device, dev, device);
               }
      VKAPI_ATTR void VKAPI_CALL
   nvk_GetDeviceImageSubresourceLayoutKHR(
      VkDevice device,
   const VkDeviceImageSubresourceInfoKHR *pInfo,
      {
      VK_FROM_HANDLE(nvk_device, dev, device);
   ASSERTED VkResult result;
            result = nvk_image_init(dev, &image, pInfo->pCreateInfo);
                        }
      static void
   nvk_image_plane_bind(struct nvk_device *dev,
                     {
               if (plane->vma_size_B) {
      if (mem != NULL) {
      nouveau_ws_bo_bind_vma(dev->ws_dev,
                        mem->bo,
   } else {
      nouveau_ws_bo_unbind_vma(dev->ws_dev,
               } else {
      assert(plane->nil.pte_kind == 0);
                  }
      VKAPI_ATTR VkResult VKAPI_CALL
   nvk_BindImageMemory2(VkDevice device,
               {
      VK_FROM_HANDLE(nvk_device, dev, device);
   for (uint32_t i = 0; i < bindInfoCount; ++i) {
      VK_FROM_HANDLE(nvk_device_memory, mem, pBindInfos[i].memory);
            uint64_t offset_B = pBindInfos[i].memoryOffset;
   if (image->disjoint) {
      const VkBindImagePlaneMemoryInfo *plane_info =
         uint8_t plane = nvk_image_aspects_to_plane(image, plane_info->planeAspect);
      } else {
      for (unsigned plane = 0; plane < image->plane_count; plane++) {
                     if (image->stencil_copy_temp.nil.size_B > 0)
                  }
