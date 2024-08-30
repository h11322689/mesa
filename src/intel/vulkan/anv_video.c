   /*
   * Copyright © 2021 Red Hat
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
      #include "vk_video/vulkan_video_codecs_common.h"
      VkResult
   anv_CreateVideoSessionKHR(VkDevice _device,
                     {
               struct anv_video_session *vid =
         if (!vid)
                     VkResult result = vk_video_session_init(&device->vk,
               if (result != VK_SUCCESS) {
      vk_free2(&device->vk.alloc, pAllocator, vid);
               *pVideoSession = anv_video_session_to_handle(vid);
      }
      void
   anv_DestroyVideoSessionKHR(VkDevice _device,
               {
      ANV_FROM_HANDLE(anv_device, device, _device);
   ANV_FROM_HANDLE(anv_video_session, vid, _session);
   if (!_session)
            vk_object_base_finish(&vid->vk.base);
      }
      VkResult
   anv_CreateVideoSessionParametersKHR(VkDevice _device,
                     {
      ANV_FROM_HANDLE(anv_device, device, _device);
   ANV_FROM_HANDLE(anv_video_session, vid, pCreateInfo->videoSession);
   ANV_FROM_HANDLE(anv_video_session_params, templ, pCreateInfo->videoSessionParametersTemplate);
   struct anv_video_session_params *params =
         if (!params)
            VkResult result = vk_video_session_parameters_init(&device->vk,
                           if (result != VK_SUCCESS) {
      vk_free2(&device->vk.alloc, pAllocator, params);
               *pVideoSessionParameters = anv_video_session_params_to_handle(params);
      }
      void
   anv_DestroyVideoSessionParametersKHR(VkDevice _device,
               {
      ANV_FROM_HANDLE(anv_device, device, _device);
   ANV_FROM_HANDLE(anv_video_session_params, params, _params);
   if (!_params)
         vk_video_session_parameters_finish(&device->vk, &params->vk);
      }
      VkResult
   anv_GetPhysicalDeviceVideoCapabilitiesKHR(VkPhysicalDevice physicalDevice,
               {
               pCapabilities->minBitstreamBufferOffsetAlignment = 32;
   pCapabilities->minBitstreamBufferSizeAlignment = 32;
   pCapabilities->maxCodedExtent.width = 4096;
   pCapabilities->maxCodedExtent.height = 4096;
            struct VkVideoDecodeCapabilitiesKHR *dec_caps = (struct VkVideoDecodeCapabilitiesKHR *)
         if (dec_caps)
            /* H264 allows different luma and chroma bit depths */
   if (pVideoProfile->lumaBitDepth != pVideoProfile->chromaBitDepth)
            if (pVideoProfile->chromaSubsampling != VK_VIDEO_CHROMA_SUBSAMPLING_420_BIT_KHR)
            switch (pVideoProfile->videoCodecOperation) {
   case VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR: {
      struct VkVideoDecodeH264CapabilitiesKHR *ext = (struct VkVideoDecodeH264CapabilitiesKHR *)
            if (pVideoProfile->lumaBitDepth != VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR)
            pCapabilities->maxDpbSlots = 17;
   pCapabilities->maxActiveReferencePictures = ANV_VIDEO_H264_MAX_NUM_REF_FRAME;
   pCapabilities->pictureAccessGranularity.width = ANV_MB_WIDTH;
   pCapabilities->pictureAccessGranularity.height = ANV_MB_HEIGHT;
   pCapabilities->minCodedExtent.width = ANV_MB_WIDTH;
            ext->fieldOffsetGranularity.x = 0;
   ext->fieldOffsetGranularity.y = 0;
   ext->maxLevelIdc = STD_VIDEO_H264_LEVEL_IDC_5_1;
   strcpy(pCapabilities->stdHeaderVersion.extensionName, VK_STD_VULKAN_VIDEO_CODEC_H264_DECODE_EXTENSION_NAME);
   pCapabilities->stdHeaderVersion.specVersion = VK_STD_VULKAN_VIDEO_CODEC_H264_DECODE_SPEC_VERSION;
      }
   case VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR: {
      struct VkVideoDecodeH265CapabilitiesKHR *ext = (struct VkVideoDecodeH265CapabilitiesKHR *)
            const struct VkVideoDecodeH265ProfileInfoKHR *h265_profile =
                  /* No hardware supports the scc extension profile */
   if (h265_profile->stdProfileIdc != STD_VIDEO_H265_PROFILE_IDC_MAIN &&
      h265_profile->stdProfileIdc != STD_VIDEO_H265_PROFILE_IDC_MAIN_10 &&
   h265_profile->stdProfileIdc != STD_VIDEO_H265_PROFILE_IDC_MAIN_STILL_PICTURE &&
               /* Skylake only supports the main profile */
   if (h265_profile->stdProfileIdc != STD_VIDEO_H265_PROFILE_IDC_MAIN &&
      h265_profile->stdProfileIdc != STD_VIDEO_H265_PROFILE_IDC_MAIN_STILL_PICTURE &&
               /* Gfx10 and under don't support the range extension profile */
   if (h265_profile->stdProfileIdc != STD_VIDEO_H265_PROFILE_IDC_MAIN &&
      h265_profile->stdProfileIdc != STD_VIDEO_H265_PROFILE_IDC_MAIN_10 &&
   h265_profile->stdProfileIdc != STD_VIDEO_H265_PROFILE_IDC_MAIN_STILL_PICTURE &&
               if (pVideoProfile->lumaBitDepth != VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR &&
                  pCapabilities->pictureAccessGranularity.width = ANV_MAX_H265_CTB_SIZE;
   pCapabilities->pictureAccessGranularity.height = ANV_MAX_H265_CTB_SIZE;
   pCapabilities->minCodedExtent.width = ANV_MAX_H265_CTB_SIZE;
   pCapabilities->minCodedExtent.height = ANV_MAX_H265_CTB_SIZE;
   pCapabilities->maxDpbSlots = ANV_VIDEO_H265_MAX_NUM_REF_FRAME;
                     strcpy(pCapabilities->stdHeaderVersion.extensionName, VK_STD_VULKAN_VIDEO_CODEC_H265_DECODE_EXTENSION_NAME);
   pCapabilities->stdHeaderVersion.specVersion = VK_STD_VULKAN_VIDEO_CODEC_H265_DECODE_SPEC_VERSION;
      }
   default:
         }
      }
      VkResult
   anv_GetPhysicalDeviceVideoFormatPropertiesKHR(VkPhysicalDevice physicalDevice,
                     {
      VK_OUTARRAY_MAKE_TYPED(VkVideoFormatPropertiesKHR, out,
                  bool need_10bit = false;
   const struct VkVideoProfileListInfoKHR *prof_list = (struct VkVideoProfileListInfoKHR *)
            if (prof_list) {
      for (unsigned i = 0; i < prof_list->profileCount; i++) {
      const VkVideoProfileInfoKHR *profile = &prof_list->pProfiles[i];
   if (profile->lumaBitDepth & VK_VIDEO_COMPONENT_BIT_DEPTH_10_BIT_KHR ||
      profile->chromaBitDepth & VK_VIDEO_COMPONENT_BIT_DEPTH_10_BIT_KHR)
               vk_outarray_append_typed(VkVideoFormatPropertiesKHR, &out, p) {
      p->format = VK_FORMAT_G8_B8R8_2PLANE_420_UNORM;
   p->imageType = VK_IMAGE_TYPE_2D;
   p->imageTiling = VK_IMAGE_TILING_OPTIMAL;
               if (need_10bit) {
      vk_outarray_append_typed(VkVideoFormatPropertiesKHR, &out, p) {
      p->format = VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16;
   p->imageType = VK_IMAGE_TYPE_2D;
   p->imageTiling = VK_IMAGE_TILING_OPTIMAL;
                     }
      static uint64_t
   get_h264_video_mem_size(struct anv_video_session *vid, uint32_t mem_idx)
   {
      uint32_t width_in_mb =
            switch (mem_idx) {
   case ANV_VID_MEM_H264_INTRA_ROW_STORE:
         case ANV_VID_MEM_H264_DEBLOCK_FILTER_ROW_STORE:
         case ANV_VID_MEM_H264_BSD_MPC_ROW_SCRATCH:
         case ANV_VID_MEM_H264_MPR_ROW_SCRATCH:
         default:
            }
      static uint64_t
   get_h265_video_mem_size(struct anv_video_session *vid, uint32_t mem_idx)
   {
      uint32_t bit_shift =
            /* TODO. these sizes can be determined dynamically depending on ctb sizes of each slice. */
   uint32_t width_in_ctb =
         uint32_t height_in_ctb =
                  switch (mem_idx) {
   case ANV_VID_MEM_H265_DEBLOCK_FILTER_ROW_STORE_LINE:
   case ANV_VID_MEM_H265_DEBLOCK_FILTER_ROW_STORE_TILE_LINE:
      size = align(vid->vk.max_coded.width, 32) >> bit_shift;
      case ANV_VID_MEM_H265_DEBLOCK_FILTER_ROW_STORE_TILE_COLUMN:
      size = align(vid->vk.max_coded.height + 6 * height_in_ctb, 32) >> bit_shift;
      case ANV_VID_MEM_H265_METADATA_LINE:
      size = (((vid->vk.max_coded.width + 15) >> 4) * 188 + width_in_ctb * 9 + 1023) >> 9;
      case ANV_VID_MEM_H265_METADATA_TILE_LINE:
      size = (((vid->vk.max_coded.width + 15) >> 4) * 172 + width_in_ctb * 9 + 1023) >> 9;
      case ANV_VID_MEM_H265_METADATA_TILE_COLUMN:
      size = (((vid->vk.max_coded.height + 15) >> 4) * 176 + height_in_ctb * 89 + 1023) >> 9;
      case ANV_VID_MEM_H265_SAO_LINE:
      size = align((vid->vk.max_coded.width >> 1) + width_in_ctb * 3, 16) >> bit_shift;
      case ANV_VID_MEM_H265_SAO_TILE_LINE:
      size = align((vid->vk.max_coded.width >> 1) + width_in_ctb * 6, 16) >> bit_shift;
      case ANV_VID_MEM_H265_SAO_TILE_COLUMN:
      size = align((vid->vk.max_coded.height >> 1) + height_in_ctb * 6, 16) >> bit_shift;
      default:
                     }
      static void
   get_h264_video_session_mem_reqs(struct anv_video_session *vid,
                     {
      VK_OUTARRAY_MAKE_TYPED(VkVideoSessionMemoryRequirementsKHR,
                        for (unsigned i = 0; i < ANV_VIDEO_MEM_REQS_H264; i++) {
      uint32_t bind_index = ANV_VID_MEM_H264_INTRA_ROW_STORE + i;
            vk_outarray_append_typed(VkVideoSessionMemoryRequirementsKHR, &out, p) {
      p->memoryBindIndex = bind_index;
   p->memoryRequirements.size = size;
   p->memoryRequirements.alignment = 4096;
            }
      static void
   get_h265_video_session_mem_reqs(struct anv_video_session *vid,
                     {
      VK_OUTARRAY_MAKE_TYPED(VkVideoSessionMemoryRequirementsKHR,
                        for (unsigned i = 0; i < ANV_VIDEO_MEM_REQS_H265; i++) {
      uint32_t bind_index =
                  vk_outarray_append_typed(VkVideoSessionMemoryRequirementsKHR, &out, p) {
      p->memoryBindIndex = bind_index;
   p->memoryRequirements.size = size;
   p->memoryRequirements.alignment = 4096;
            }
      VkResult
   anv_GetVideoSessionMemoryRequirementsKHR(VkDevice _device,
                     {
      ANV_FROM_HANDLE(anv_device, device, _device);
            uint32_t memory_types = (1ull << device->physical->memory.type_count) - 1;
   switch (vid->vk.op) {
   case VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR:
      get_h264_video_session_mem_reqs(vid,
                        case VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR:
      get_h265_video_session_mem_reqs(vid,
                        default:
                     }
      VkResult
   anv_UpdateVideoSessionParametersKHR(VkDevice _device,
               {
      ANV_FROM_HANDLE(anv_video_session_params, params, _params);
      }
      static void
   copy_bind(struct anv_vid_mem *dst,
         {
      dst->mem = anv_device_memory_from_handle(src->memory);
   dst->offset = src->memoryOffset;
      }
      VkResult
   anv_BindVideoSessionMemoryKHR(VkDevice _device,
                     {
               switch (vid->vk.op) {
   case VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR:
   case VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR:
      for (unsigned i = 0; i < bind_mem_count; i++) {
         }
      default:
         }
      }
