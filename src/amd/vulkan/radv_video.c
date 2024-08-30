   /**************************************************************************
   *
   * Copyright 2017 Advanced Micro Devices, Inc.
   * Copyright 2021 Red Hat Inc.
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
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   * IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
   #include "radv_private.h"
      #ifndef _WIN32
   #include "drm-uapi/amdgpu_drm.h"
   #endif
      #include "util/vl_zscan_data.h"
   #include "vk_video/vulkan_video_codecs_common.h"
   #include "ac_uvd_dec.h"
   #include "ac_vcn_dec.h"
      #include "radv_cs.h"
   #include "radv_debug.h"
      #define NUM_H264_REFS                17
   #define NUM_H265_REFS                8
   #define FB_BUFFER_OFFSET             0x1000
   #define FB_BUFFER_SIZE               2048
   #define FB_BUFFER_SIZE_TONGA         (2048 * 64)
   #define IT_SCALING_TABLE_SIZE        992
   #define RDECODE_SESSION_CONTEXT_SIZE (128 * 1024)
      /* Not 100% sure this isn't too much but works */
   #define VID_DEFAULT_ALIGNMENT 256
      static bool
   radv_enable_tier2(struct radv_physical_device *pdevice)
   {
      if (pdevice->rad_info.family >= CHIP_NAVI21 && !(pdevice->instance->debug_flags & RADV_DEBUG_VIDEO_ARRAY_PATH))
            }
      static uint32_t
   radv_video_get_db_alignment(struct radv_physical_device *pdevice, int width, bool is_h265_main_10)
   {
      if (pdevice->rad_info.family >= CHIP_RENOIR && width > 32 && is_h265_main_10)
            }
      static bool
   radv_vid_buffer_upload_alloc(struct radv_cmd_buffer *cmd_buffer, unsigned size, unsigned *out_offset, void **ptr)
   {
         }
      /* vcn unified queue (sq) ib header */
   static void
   radv_vcn_sq_header(struct radeon_cmdbuf *cs, struct rvcn_sq_var *sq, bool enc)
   {
      /* vcn ib signature */
   radeon_emit(cs, RADEON_VCN_SIGNATURE_SIZE);
   radeon_emit(cs, RADEON_VCN_SIGNATURE);
   sq->ib_checksum = &cs->buf[cs->cdw];
   radeon_emit(cs, 0);
   sq->ib_total_size_in_dw = &cs->buf[cs->cdw];
            /* vcn ib engine info */
   radeon_emit(cs, RADEON_VCN_ENGINE_INFO_SIZE);
   radeon_emit(cs, RADEON_VCN_ENGINE_INFO);
   radeon_emit(cs, enc ? RADEON_VCN_ENGINE_TYPE_ENCODE : RADEON_VCN_ENGINE_TYPE_DECODE);
      }
      static void
   radv_vcn_sq_tail(struct radeon_cmdbuf *cs, struct rvcn_sq_var *sq)
   {
      uint32_t *end;
   uint32_t size_in_dw;
            if (sq->ib_checksum == NULL || sq->ib_total_size_in_dw == NULL)
            end = &cs->buf[cs->cdw];
   size_in_dw = end - sq->ib_total_size_in_dw - 1;
   *sq->ib_total_size_in_dw = size_in_dw;
            for (int i = 0; i < size_in_dw; i++)
               }
      static void
   radv_vcn_sq_start(struct radv_cmd_buffer *cmd_buffer)
   {
      radeon_check_space(cmd_buffer->device->ws, cmd_buffer->cs, 256);
   radv_vcn_sq_header(cmd_buffer->cs, &cmd_buffer->video.sq, false);
   rvcn_decode_ib_package_t *ib_header = (rvcn_decode_ib_package_t *)&(cmd_buffer->cs->buf[cmd_buffer->cs->cdw]);
   ib_header->package_size = sizeof(struct rvcn_decode_buffer_s) + sizeof(struct rvcn_decode_ib_package_s);
   cmd_buffer->cs->cdw++;
   ib_header->package_type = (RDECODE_IB_PARAM_DECODE_BUFFER);
   cmd_buffer->cs->cdw++;
   cmd_buffer->video.decode_buffer = (rvcn_decode_buffer_t *)&(cmd_buffer->cs->buf[cmd_buffer->cs->cdw]);
   cmd_buffer->cs->cdw += sizeof(struct rvcn_decode_buffer_s) / 4;
      }
      /* generate an stream handle */
   static unsigned
   si_vid_alloc_stream_handle(struct radv_physical_device *pdevice)
   {
               stream_handle ^= ++pdevice->stream_handle_counter;
      }
      void
   radv_init_physical_device_decoder(struct radv_physical_device *pdevice)
   {
      if (pdevice->rad_info.family >= CHIP_NAVI31 || pdevice->rad_info.family == CHIP_GFX940)
         else if (radv_has_uvd(pdevice))
         else
            pdevice->stream_handle_counter = 0;
                              switch (pdevice->rad_info.family) {
   case CHIP_VEGA10:
   case CHIP_VEGA12:
   case CHIP_VEGA20:
      pdevice->vid_dec_reg.data0 = RUVD_GPCOM_VCPU_DATA0_SOC15;
   pdevice->vid_dec_reg.data1 = RUVD_GPCOM_VCPU_DATA1_SOC15;
   pdevice->vid_dec_reg.cmd = RUVD_GPCOM_VCPU_CMD_SOC15;
   pdevice->vid_dec_reg.cntl = RUVD_ENGINE_CNTL_SOC15;
      case CHIP_RAVEN:
   case CHIP_RAVEN2:
      pdevice->vid_dec_reg.data0 = RDECODE_VCN1_GPCOM_VCPU_DATA0;
   pdevice->vid_dec_reg.data1 = RDECODE_VCN1_GPCOM_VCPU_DATA1;
   pdevice->vid_dec_reg.cmd = RDECODE_VCN1_GPCOM_VCPU_CMD;
   pdevice->vid_dec_reg.cntl = RDECODE_VCN1_ENGINE_CNTL;
      case CHIP_NAVI10:
   case CHIP_NAVI12:
   case CHIP_NAVI14:
   case CHIP_RENOIR:
      pdevice->vid_dec_reg.data0 = RDECODE_VCN2_GPCOM_VCPU_DATA0;
   pdevice->vid_dec_reg.data1 = RDECODE_VCN2_GPCOM_VCPU_DATA1;
   pdevice->vid_dec_reg.cmd = RDECODE_VCN2_GPCOM_VCPU_CMD;
   pdevice->vid_dec_reg.cntl = RDECODE_VCN2_ENGINE_CNTL;
      case CHIP_MI100:
   case CHIP_MI200:
   case CHIP_NAVI21:
   case CHIP_NAVI22:
   case CHIP_NAVI23:
   case CHIP_NAVI24:
   case CHIP_VANGOGH:
   case CHIP_REMBRANDT:
   case CHIP_RAPHAEL_MENDOCINO:
      pdevice->vid_dec_reg.data0 = RDECODE_VCN2_5_GPCOM_VCPU_DATA0;
   pdevice->vid_dec_reg.data1 = RDECODE_VCN2_5_GPCOM_VCPU_DATA1;
   pdevice->vid_dec_reg.cmd = RDECODE_VCN2_5_GPCOM_VCPU_CMD;
   pdevice->vid_dec_reg.cntl = RDECODE_VCN2_5_ENGINE_CNTL;
      case CHIP_GFX940:
      pdevice->vid_addr_gfx_mode = RDECODE_ARRAY_MODE_ADDRLIB_SEL_GFX9;
      case CHIP_NAVI31:
   case CHIP_NAVI32:
   case CHIP_NAVI33:
   case CHIP_GFX1103_R1:
   case CHIP_GFX1103_R2:
   case CHIP_GFX1150:
      pdevice->vid_addr_gfx_mode = RDECODE_ARRAY_MODE_ADDRLIB_SEL_GFX11;
      default:
      if (radv_has_uvd(pdevice)) {
      pdevice->vid_dec_reg.data0 = RUVD_GPCOM_VCPU_DATA0;
   pdevice->vid_dec_reg.data1 = RUVD_GPCOM_VCPU_DATA1;
   pdevice->vid_dec_reg.cmd = RUVD_GPCOM_VCPU_CMD;
      }
         }
      static bool
   have_it(struct radv_video_session *vid)
   {
         }
      static unsigned
   calc_ctx_size_h264_perf(struct radv_video_session *vid)
   {
      unsigned width_in_mb, height_in_mb, ctx_size;
   unsigned width = align(vid->vk.max_coded.width, VL_MACROBLOCK_WIDTH);
                     // picture width & height in 16 pixel units
   width_in_mb = width / VL_MACROBLOCK_WIDTH;
                        }
      static unsigned
   calc_ctx_size_h265_main(struct radv_video_session *vid)
   {
      unsigned width = align(vid->vk.max_coded.width, VL_MACROBLOCK_WIDTH);
                     if (vid->vk.max_coded.width * vid->vk.max_coded.height >= 4096 * 2000)
         else
            width = align(width, 16);
   height = align(height, 16);
      }
      static unsigned
   calc_ctx_size_h265_main10(struct radv_video_session *vid)
   {
      unsigned log2_ctb_size, width_in_ctb, height_in_ctb, num_16x16_block_per_ctb;
   unsigned context_buffer_size_per_ctb_row, cm_buffer_size, max_mb_address, db_left_tile_pxl_size;
            unsigned width = align(vid->vk.max_coded.width, VL_MACROBLOCK_WIDTH);
   unsigned height = align(vid->vk.max_coded.height, VL_MACROBLOCK_HEIGHT);
                     if (vid->vk.max_coded.width * vid->vk.max_coded.height >= 4096 * 2000)
         else
            /* 64x64 is the maximum ctb size. */
            width_in_ctb = (width + ((1 << log2_ctb_size) - 1)) >> log2_ctb_size;
            num_16x16_block_per_ctb = ((1 << log2_ctb_size) >> 4) * ((1 << log2_ctb_size) >> 4);
   context_buffer_size_per_ctb_row = align(width_in_ctb * num_16x16_block_per_ctb * 16, 256);
            cm_buffer_size = max_references * context_buffer_size_per_ctb_row * height_in_ctb;
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_CreateVideoSessionKHR(VkDevice _device, const VkVideoSessionCreateInfoKHR *pCreateInfo,
         {
               struct radv_video_session *vid =
         if (!vid)
                     VkResult result = vk_video_session_init(&device->vk, &vid->vk, pCreateInfo);
   if (result != VK_SUCCESS) {
      vk_free2(&device->vk.alloc, pAllocator, vid);
               vid->interlaced = false;
            switch (vid->vk.op) {
   case VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR:
      vid->stream_type = RDECODE_CODEC_H264_PERF;
   if (radv_enable_tier2(device->physical_device))
            case VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR:
      vid->stream_type = RDECODE_CODEC_H265;
   if (radv_enable_tier2(device->physical_device))
            default:
                  vid->stream_handle = si_vid_alloc_stream_handle(device->physical_device);
   vid->dbg_frame_cnt = 0;
   vid->db_alignment = radv_video_get_db_alignment(
      device->physical_device, vid->vk.max_coded.width,
         *pVideoSession = radv_video_session_to_handle(vid);
      }
      VKAPI_ATTR void VKAPI_CALL
   radv_DestroyVideoSessionKHR(VkDevice _device, VkVideoSessionKHR _session, const VkAllocationCallbacks *pAllocator)
   {
      RADV_FROM_HANDLE(radv_device, device, _device);
   RADV_FROM_HANDLE(radv_video_session, vid, _session);
   if (!_session)
            vk_object_base_finish(&vid->vk.base);
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_CreateVideoSessionParametersKHR(VkDevice _device, const VkVideoSessionParametersCreateInfoKHR *pCreateInfo,
               {
      RADV_FROM_HANDLE(radv_device, device, _device);
   RADV_FROM_HANDLE(radv_video_session, vid, pCreateInfo->videoSession);
   RADV_FROM_HANDLE(radv_video_session_params, templ, pCreateInfo->videoSessionParametersTemplate);
   struct radv_video_session_params *params =
         if (!params)
            VkResult result =
         if (result != VK_SUCCESS) {
      vk_free2(&device->vk.alloc, pAllocator, params);
               *pVideoSessionParameters = radv_video_session_params_to_handle(params);
      }
      VKAPI_ATTR void VKAPI_CALL
   radv_DestroyVideoSessionParametersKHR(VkDevice _device, VkVideoSessionParametersKHR _params,
         {
      RADV_FROM_HANDLE(radv_device, device, _device);
            vk_video_session_parameters_finish(&device->vk, &params->vk);
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_GetPhysicalDeviceVideoCapabilitiesKHR(VkPhysicalDevice physicalDevice, const VkVideoProfileInfoKHR *pVideoProfile,
         {
      RADV_FROM_HANDLE(radv_physical_device, pdevice, physicalDevice);
               #ifndef _WIN32
      case VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR:
      cap = &pdevice->rad_info.dec_caps.codec_info[AMDGPU_INFO_VIDEO_CAPS_CODEC_IDX_MPEG4_AVC];
      case VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR:
      cap = &pdevice->rad_info.dec_caps.codec_info[AMDGPU_INFO_VIDEO_CAPS_CODEC_IDX_HEVC];
   #endif
      default:
                  if (cap && !cap->valid)
            pCapabilities->flags = 0;
   pCapabilities->minBitstreamBufferOffsetAlignment = 128;
   pCapabilities->minBitstreamBufferSizeAlignment = 128;
   pCapabilities->pictureAccessGranularity.width = VL_MACROBLOCK_WIDTH;
   pCapabilities->pictureAccessGranularity.height = VL_MACROBLOCK_HEIGHT;
   pCapabilities->minCodedExtent.width = VL_MACROBLOCK_WIDTH;
            struct VkVideoDecodeCapabilitiesKHR *dec_caps =
         if (dec_caps)
            /* H264 allows different luma and chroma bit depths */
   if (pVideoProfile->lumaBitDepth != pVideoProfile->chromaBitDepth)
            if (pVideoProfile->chromaSubsampling != VK_VIDEO_CHROMA_SUBSAMPLING_420_BIT_KHR)
            switch (pVideoProfile->videoCodecOperation) {
   case VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR: {
      struct VkVideoDecodeH264CapabilitiesKHR *ext = (struct VkVideoDecodeH264CapabilitiesKHR *)vk_find_struct(
            const struct VkVideoDecodeH264ProfileInfoKHR *h264_profile =
            if (h264_profile->stdProfileIdc != STD_VIDEO_H264_PROFILE_IDC_BASELINE &&
      h264_profile->stdProfileIdc != STD_VIDEO_H264_PROFILE_IDC_MAIN &&
               if (pVideoProfile->lumaBitDepth != VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR)
            pCapabilities->maxDpbSlots = NUM_H264_REFS;
            /* for h264 on navi21+ separate dpb images should work */
   if (radv_enable_tier2(pdevice))
         ext->fieldOffsetGranularity.x = 0;
   ext->fieldOffsetGranularity.y = 0;
   ext->maxLevelIdc = STD_VIDEO_H264_LEVEL_IDC_5_1;
   strcpy(pCapabilities->stdHeaderVersion.extensionName, VK_STD_VULKAN_VIDEO_CODEC_H264_DECODE_EXTENSION_NAME);
   pCapabilities->stdHeaderVersion.specVersion = VK_STD_VULKAN_VIDEO_CODEC_H264_DECODE_SPEC_VERSION;
      }
   case VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR: {
      struct VkVideoDecodeH265CapabilitiesKHR *ext = (struct VkVideoDecodeH265CapabilitiesKHR *)vk_find_struct(
            const struct VkVideoDecodeH265ProfileInfoKHR *h265_profile =
            if (h265_profile->stdProfileIdc != STD_VIDEO_H265_PROFILE_IDC_MAIN &&
      h265_profile->stdProfileIdc != STD_VIDEO_H265_PROFILE_IDC_MAIN_10 &&
               if (pVideoProfile->lumaBitDepth != VK_VIDEO_COMPONENT_BIT_DEPTH_8_BIT_KHR &&
                  pCapabilities->maxDpbSlots = NUM_H264_REFS;
   pCapabilities->maxActiveReferencePictures = NUM_H265_REFS;
   /* for h265 on navi21+ separate dpb images should work */
   if (radv_enable_tier2(pdevice))
         ext->maxLevelIdc = STD_VIDEO_H265_LEVEL_IDC_5_1;
   strcpy(pCapabilities->stdHeaderVersion.extensionName, VK_STD_VULKAN_VIDEO_CODEC_H265_DECODE_EXTENSION_NAME);
   pCapabilities->stdHeaderVersion.specVersion = VK_STD_VULKAN_VIDEO_CODEC_H265_DECODE_SPEC_VERSION;
      }
   default:
         }
   if (cap) {
      pCapabilities->maxCodedExtent.width = cap->max_width;
      } else {
      switch (pVideoProfile->videoCodecOperation) {
   case VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR:
      pCapabilities->maxCodedExtent.width = (pdevice->rad_info.family < CHIP_TONGA) ? 2048 : 4096;
   pCapabilities->maxCodedExtent.height = (pdevice->rad_info.family < CHIP_TONGA) ? 1152 : 4096;
      case VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR:
      pCapabilities->maxCodedExtent.width =
         pCapabilities->maxCodedExtent.height =
            default:
                        }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_GetPhysicalDeviceVideoFormatPropertiesKHR(VkPhysicalDevice physicalDevice,
                     {
      /* radv requires separate allocates for DPB and decode video. */
   if ((pVideoFormatInfo->imageUsage &
      (VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR | VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR)) ==
   (VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR | VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR))
                  bool need_8bit = true;
   bool need_10bit = false;
   const struct VkVideoProfileListInfoKHR *prof_list =
         if (prof_list) {
      for (unsigned i = 0; i < prof_list->profileCount; i++) {
      const VkVideoProfileInfoKHR *profile = &prof_list->pProfiles[i];
   if (profile->lumaBitDepth & VK_VIDEO_COMPONENT_BIT_DEPTH_10_BIT_KHR)
                  if (need_10bit) {
      vk_outarray_append_typed(VkVideoFormatPropertiesKHR, &out, p)
   {
      p->format = VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16;
   p->imageType = VK_IMAGE_TYPE_2D;
   p->imageTiling = VK_IMAGE_TILING_OPTIMAL;
               if (pVideoFormatInfo->imageUsage & (VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR))
               if (need_8bit) {
      vk_outarray_append_typed(VkVideoFormatPropertiesKHR, &out, p)
   {
      p->format = VK_FORMAT_G8_B8R8_2PLANE_420_UNORM;
   p->imageType = VK_IMAGE_TYPE_2D;
   p->imageTiling = VK_IMAGE_TILING_OPTIMAL;
                     }
      #define RADV_BIND_SESSION_CTX 0
   #define RADV_BIND_DECODER_CTX 1
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_GetVideoSessionMemoryRequirementsKHR(VkDevice _device, VkVideoSessionKHR videoSession,
               {
      RADV_FROM_HANDLE(radv_device, device, _device);
   RADV_FROM_HANDLE(radv_video_session, vid, videoSession);
                     /* 1 buffer for session context */
   if (device->physical_device->rad_info.family >= CHIP_POLARIS10) {
      vk_outarray_append_typed(VkVideoSessionMemoryRequirementsKHR, &out, m)
   {
      m->memoryBindIndex = RADV_BIND_SESSION_CTX;
   m->memoryRequirements.size = RDECODE_SESSION_CONTEXT_SIZE;
   m->memoryRequirements.alignment = 0;
                  if (vid->stream_type == RDECODE_CODEC_H264_PERF && device->physical_device->rad_info.family >= CHIP_POLARIS10) {
      vk_outarray_append_typed(VkVideoSessionMemoryRequirementsKHR, &out, m)
   {
      m->memoryBindIndex = RADV_BIND_DECODER_CTX;
   m->memoryRequirements.size = align(calc_ctx_size_h264_perf(vid), 4096);
   m->memoryRequirements.alignment = 0;
         }
   if (vid->stream_type == RDECODE_CODEC_H265) {
               if (vid->vk.h265.profile_idc == STD_VIDEO_H265_PROFILE_IDC_MAIN_10)
         else
         vk_outarray_append_typed(VkVideoSessionMemoryRequirementsKHR, &out, m)
   {
      m->memoryBindIndex = RADV_BIND_DECODER_CTX;
   m->memoryRequirements.size = align(ctx_size, 4096);
   m->memoryRequirements.alignment = 0;
         }
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_UpdateVideoSessionParametersKHR(VkDevice _device, VkVideoSessionParametersKHR videoSessionParameters,
         {
                  }
      static void
   copy_bind(struct radv_vid_mem *dst, const VkBindVideoSessionMemoryInfoKHR *src)
   {
      dst->mem = radv_device_memory_from_handle(src->memory);
   dst->offset = src->memoryOffset;
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_BindVideoSessionMemoryKHR(VkDevice _device, VkVideoSessionKHR videoSession, uint32_t videoSessionBindMemoryCount,
         {
               for (unsigned i = 0; i < videoSessionBindMemoryCount; i++) {
      switch (pBindSessionMemoryInfos[i].memoryBindIndex) {
   case RADV_BIND_SESSION_CTX:
      copy_bind(&vid->sessionctx, &pBindSessionMemoryInfos[i]);
      case RADV_BIND_DECODER_CTX:
      copy_bind(&vid->ctx, &pBindSessionMemoryInfos[i]);
      default:
      assert(0);
         }
      }
      /* add a new set register command to the IB */
   static void
   set_reg(struct radv_cmd_buffer *cmd_buffer, unsigned reg, uint32_t val)
   {
      struct radeon_cmdbuf *cs = cmd_buffer->cs;
   radeon_emit(cs, RDECODE_PKT0(reg >> 2, 0));
      }
      static void
   send_cmd(struct radv_cmd_buffer *cmd_buffer, unsigned cmd, struct radeon_winsys_bo *bo, uint32_t offset)
   {
      struct radv_physical_device *pdev = cmd_buffer->device->physical_device;
            radv_cs_add_buffer(cmd_buffer->device->ws, cmd_buffer->cs, bo);
   addr = radv_buffer_get_va(bo);
            if (cmd_buffer->device->physical_device->vid_decode_ip != AMD_IP_VCN_UNIFIED) {
      radeon_check_space(cmd_buffer->device->ws, cmd_buffer->cs, 6);
   set_reg(cmd_buffer, pdev->vid_dec_reg.data0, addr);
   set_reg(cmd_buffer, pdev->vid_dec_reg.data1, addr >> 32);
   set_reg(cmd_buffer, pdev->vid_dec_reg.cmd, cmd << 1);
      }
   switch (cmd) {
   case RDECODE_CMD_MSG_BUFFER:
      cmd_buffer->video.decode_buffer->valid_buf_flag |= RDECODE_CMDBUF_FLAGS_MSG_BUFFER;
   cmd_buffer->video.decode_buffer->msg_buffer_address_hi = (addr >> 32);
   cmd_buffer->video.decode_buffer->msg_buffer_address_lo = (addr);
      case RDECODE_CMD_DPB_BUFFER:
      cmd_buffer->video.decode_buffer->valid_buf_flag |= (RDECODE_CMDBUF_FLAGS_DPB_BUFFER);
   cmd_buffer->video.decode_buffer->dpb_buffer_address_hi = (addr >> 32);
   cmd_buffer->video.decode_buffer->dpb_buffer_address_lo = (addr);
      case RDECODE_CMD_DECODING_TARGET_BUFFER:
      cmd_buffer->video.decode_buffer->valid_buf_flag |= (RDECODE_CMDBUF_FLAGS_DECODING_TARGET_BUFFER);
   cmd_buffer->video.decode_buffer->target_buffer_address_hi = (addr >> 32);
   cmd_buffer->video.decode_buffer->target_buffer_address_lo = (addr);
      case RDECODE_CMD_FEEDBACK_BUFFER:
      cmd_buffer->video.decode_buffer->valid_buf_flag |= (RDECODE_CMDBUF_FLAGS_FEEDBACK_BUFFER);
   cmd_buffer->video.decode_buffer->feedback_buffer_address_hi = (addr >> 32);
   cmd_buffer->video.decode_buffer->feedback_buffer_address_lo = (addr);
      case RDECODE_CMD_PROB_TBL_BUFFER:
      cmd_buffer->video.decode_buffer->valid_buf_flag |= (RDECODE_CMDBUF_FLAGS_PROB_TBL_BUFFER);
   cmd_buffer->video.decode_buffer->prob_tbl_buffer_address_hi = (addr >> 32);
   cmd_buffer->video.decode_buffer->prob_tbl_buffer_address_lo = (addr);
      case RDECODE_CMD_SESSION_CONTEXT_BUFFER:
      cmd_buffer->video.decode_buffer->valid_buf_flag |= (RDECODE_CMDBUF_FLAGS_SESSION_CONTEXT_BUFFER);
   cmd_buffer->video.decode_buffer->session_contex_buffer_address_hi = (addr >> 32);
   cmd_buffer->video.decode_buffer->session_contex_buffer_address_lo = (addr);
      case RDECODE_CMD_BITSTREAM_BUFFER:
      cmd_buffer->video.decode_buffer->valid_buf_flag |= (RDECODE_CMDBUF_FLAGS_BITSTREAM_BUFFER);
   cmd_buffer->video.decode_buffer->bitstream_buffer_address_hi = (addr >> 32);
   cmd_buffer->video.decode_buffer->bitstream_buffer_address_lo = (addr);
      case RDECODE_CMD_IT_SCALING_TABLE_BUFFER:
      cmd_buffer->video.decode_buffer->valid_buf_flag |= (RDECODE_CMDBUF_FLAGS_IT_SCALING_BUFFER);
   cmd_buffer->video.decode_buffer->it_sclr_table_buffer_address_hi = (addr >> 32);
   cmd_buffer->video.decode_buffer->it_sclr_table_buffer_address_lo = (addr);
      case RDECODE_CMD_CONTEXT_BUFFER:
      cmd_buffer->video.decode_buffer->valid_buf_flag |= (RDECODE_CMDBUF_FLAGS_CONTEXT_BUFFER);
   cmd_buffer->video.decode_buffer->context_buffer_address_hi = (addr >> 32);
   cmd_buffer->video.decode_buffer->context_buffer_address_lo = (addr);
      default:
            }
      static void
   rvcn_dec_message_create(struct radv_video_session *vid, void *ptr, uint32_t size)
   {
      rvcn_dec_message_header_t *header = ptr;
            memset(ptr, 0, size);
   header->header_size = sizeof(rvcn_dec_message_header_t);
   header->total_size = size;
   header->num_buffers = 1;
   header->msg_type = RDECODE_MSG_CREATE;
   header->stream_handle = vid->stream_handle;
            header->index[0].message_id = RDECODE_MESSAGE_CREATE;
   header->index[0].offset = sizeof(rvcn_dec_message_header_t);
   header->index[0].size = sizeof(rvcn_dec_message_create_t);
            create->stream_type = vid->stream_type;
   create->session_flags = 0;
   create->width_in_samples = vid->vk.max_coded.width;
      }
      static void
   rvcn_dec_message_feedback(void *ptr)
   {
               header->header_size = sizeof(rvcn_dec_feedback_header_t);
   header->total_size = sizeof(rvcn_dec_feedback_header_t);
      }
      static const uint8_t h264_levels[] = {10, 11, 12, 13, 20, 21, 22, 30, 31, 32, 40, 41, 42, 50, 51, 52, 60, 61, 62};
   static uint8_t
   get_h264_level(StdVideoH264LevelIdc level)
   {
      assert(level <= STD_VIDEO_H264_LEVEL_IDC_6_2);
      }
      static void
   update_h264_scaling(unsigned char scaling_list_4x4[6][16], unsigned char scaling_list_8x8[2][64],
         {
      for (int i = 0; i < STD_VIDEO_H264_SCALING_LIST_4X4_NUM_LISTS; i++) {
      for (int j = 0; j < STD_VIDEO_H264_SCALING_LIST_4X4_NUM_ELEMENTS; j++)
               for (int i = 0; i < 2; i++) {
      for (int j = 0; j < STD_VIDEO_H264_SCALING_LIST_8X8_NUM_ELEMENTS; j++)
         }
      static rvcn_dec_message_avc_t
   get_h264_msg(struct radv_video_session *vid, struct radv_video_session_params *params,
               {
      rvcn_dec_message_avc_t result;
   const struct VkVideoDecodeH264PictureInfoKHR *h264_pic_info =
                              assert(params->vk.h264_dec.std_sps_count > 0);
   const StdVideoH264SequenceParameterSet *sps =
         switch (sps->profile_idc) {
   case STD_VIDEO_H264_PROFILE_IDC_BASELINE:
      result.profile = RDECODE_H264_PROFILE_BASELINE;
      case STD_VIDEO_H264_PROFILE_IDC_MAIN:
      result.profile = RDECODE_H264_PROFILE_MAIN;
      case STD_VIDEO_H264_PROFILE_IDC_HIGH:
      result.profile = RDECODE_H264_PROFILE_HIGH;
      default:
      fprintf(stderr, "UNSUPPORTED CODEC %d\n", sps->profile_idc);
   result.profile = RDECODE_H264_PROFILE_MAIN;
               *width_in_samples = (sps->pic_width_in_mbs_minus1 + 1) * 16;
   *height_in_samples = (sps->pic_height_in_map_units_minus1 + 1) * 16;
   if (!sps->flags.frame_mbs_only_flag)
                           result.sps_info_flags |= sps->flags.direct_8x8_inference_flag << 0;
   result.sps_info_flags |= sps->flags.mb_adaptive_frame_field_flag << 1;
   result.sps_info_flags |= sps->flags.frame_mbs_only_flag << 2;
   result.sps_info_flags |= sps->flags.delta_pic_order_always_zero_flag << 3;
   if (vid->dpb_type != DPB_DYNAMIC_TIER_2)
            result.bit_depth_luma_minus8 = sps->bit_depth_luma_minus8;
   result.bit_depth_chroma_minus8 = sps->bit_depth_chroma_minus8;
   result.log2_max_frame_num_minus4 = sps->log2_max_frame_num_minus4;
   result.pic_order_cnt_type = sps->pic_order_cnt_type;
                     const StdVideoH264PictureParameterSet *pps =
         result.pps_info_flags = 0;
   result.pps_info_flags |= pps->flags.transform_8x8_mode_flag << 0;
   result.pps_info_flags |= pps->flags.redundant_pic_cnt_present_flag << 1;
   result.pps_info_flags |= pps->flags.constrained_intra_pred_flag << 2;
   result.pps_info_flags |= pps->flags.deblocking_filter_control_present_flag << 3;
   result.pps_info_flags |= pps->weighted_bipred_idc << 4;
   result.pps_info_flags |= pps->flags.weighted_pred_flag << 6;
   result.pps_info_flags |= pps->flags.bottom_field_pic_order_in_frame_present_flag << 7;
            result.pic_init_qp_minus26 = pps->pic_init_qp_minus26;
   result.chroma_qp_index_offset = pps->chroma_qp_index_offset;
            StdVideoH264ScalingLists scaling_lists;
   vk_video_derive_h264_scaling_list(sps, pps, &scaling_lists);
            memset(it_ptr, 0, IT_SCALING_TABLE_SIZE);
   memcpy(it_ptr, result.scaling_list_4x4, 6 * 16);
            result.num_ref_idx_l0_active_minus1 = pps->num_ref_idx_l0_default_active_minus1;
            result.curr_field_order_cnt_list[0] = h264_pic_info->pStdPictureInfo->PicOrderCnt[0];
                     result.num_ref_frames = sps->max_num_ref_frames;
   result.non_existing_frame_flags = 0;
            memset(result.ref_frame_list, 0xff, sizeof(unsigned char) * 16);
   memset(result.frame_num_list, 0, sizeof(unsigned int) * 16);
   for (unsigned i = 0; i < frame_info->referenceSlotCount; i++) {
      int idx = frame_info->pReferenceSlots[i].slotIndex;
   const struct VkVideoDecodeH264DpbSlotInfoKHR *dpb_slot =
            result.frame_num_list[i] = dpb_slot->pStdReferenceInfo->FrameNum;
   result.field_order_cnt_list[i][0] = dpb_slot->pStdReferenceInfo->PicOrderCnt[0];
                     if (dpb_slot->pStdReferenceInfo->flags.top_field_flag)
         if (dpb_slot->pStdReferenceInfo->flags.bottom_field_flag)
            if (!dpb_slot->pStdReferenceInfo->flags.top_field_flag && !dpb_slot->pStdReferenceInfo->flags.bottom_field_flag)
            if (dpb_slot->pStdReferenceInfo->flags.used_for_long_term_reference)
         if (dpb_slot->pStdReferenceInfo->flags.is_non_existing)
      }
   result.curr_pic_ref_frame_num = frame_info->referenceSlotCount;
               }
      static void
   update_h265_scaling(void *it_ptr, const StdVideoH265ScalingLists *scaling_lists)
   {
      if (scaling_lists) {
      memcpy(it_ptr, scaling_lists->ScalingList4x4,
         memcpy((char *)it_ptr + 96, scaling_lists->ScalingList8x8,
         memcpy((char *)it_ptr + 480, scaling_lists->ScalingList16x16,
         memcpy((char *)it_ptr + 864, scaling_lists->ScalingList32x32,
      } else {
      memset(it_ptr, 0, STD_VIDEO_H265_SCALING_LIST_4X4_NUM_LISTS * STD_VIDEO_H265_SCALING_LIST_4X4_NUM_ELEMENTS);
   memset((char *)it_ptr + 96, 0,
         memset((char *)it_ptr + 480, 0,
         memset((char *)it_ptr + 864, 0,
         }
      static rvcn_dec_message_hevc_t
   get_h265_msg(struct radv_device *device, struct radv_video_session *vid, struct radv_video_session_params *params,
         {
      rvcn_dec_message_hevc_t result;
   int i, j;
   const struct VkVideoDecodeH265PictureInfoKHR *h265_pic_info =
                  const StdVideoH265SequenceParameterSet *sps =
         const StdVideoH265PictureParameterSet *pps =
            result.sps_info_flags = 0;
   result.sps_info_flags |= sps->flags.scaling_list_enabled_flag << 0;
   result.sps_info_flags |= sps->flags.amp_enabled_flag << 1;
   result.sps_info_flags |= sps->flags.sample_adaptive_offset_enabled_flag << 2;
   result.sps_info_flags |= sps->flags.pcm_enabled_flag << 3;
   result.sps_info_flags |= sps->flags.pcm_loop_filter_disabled_flag << 4;
   result.sps_info_flags |= sps->flags.long_term_ref_pics_present_flag << 5;
   result.sps_info_flags |= sps->flags.sps_temporal_mvp_enabled_flag << 6;
   result.sps_info_flags |= sps->flags.strong_intra_smoothing_enabled_flag << 7;
            if (device->physical_device->rad_info.family == CHIP_CARRIZO)
            if (!h265_pic_info->pStdPictureInfo->flags.short_term_ref_pic_set_sps_flag) {
         }
            result.chroma_format = sps->chroma_format_idc;
   result.bit_depth_luma_minus8 = sps->bit_depth_luma_minus8;
   result.bit_depth_chroma_minus8 = sps->bit_depth_chroma_minus8;
   result.log2_max_pic_order_cnt_lsb_minus4 = sps->log2_max_pic_order_cnt_lsb_minus4;
   result.sps_max_dec_pic_buffering_minus1 =
         result.log2_min_luma_coding_block_size_minus3 = sps->log2_min_luma_coding_block_size_minus3;
   result.log2_diff_max_min_luma_coding_block_size = sps->log2_diff_max_min_luma_coding_block_size;
   result.log2_min_transform_block_size_minus2 = sps->log2_min_luma_transform_block_size_minus2;
   result.log2_diff_max_min_transform_block_size = sps->log2_diff_max_min_luma_transform_block_size;
   result.max_transform_hierarchy_depth_inter = sps->max_transform_hierarchy_depth_inter;
   result.max_transform_hierarchy_depth_intra = sps->max_transform_hierarchy_depth_intra;
   if (sps->flags.pcm_enabled_flag) {
      result.pcm_sample_bit_depth_luma_minus1 = sps->pcm_sample_bit_depth_luma_minus1;
   result.pcm_sample_bit_depth_chroma_minus1 = sps->pcm_sample_bit_depth_chroma_minus1;
   result.log2_min_pcm_luma_coding_block_size_minus3 = sps->log2_min_pcm_luma_coding_block_size_minus3;
      }
            result.pps_info_flags = 0;
   result.pps_info_flags |= pps->flags.dependent_slice_segments_enabled_flag << 0;
   result.pps_info_flags |= pps->flags.output_flag_present_flag << 1;
   result.pps_info_flags |= pps->flags.sign_data_hiding_enabled_flag << 2;
   result.pps_info_flags |= pps->flags.cabac_init_present_flag << 3;
   result.pps_info_flags |= pps->flags.constrained_intra_pred_flag << 4;
   result.pps_info_flags |= pps->flags.transform_skip_enabled_flag << 5;
   result.pps_info_flags |= pps->flags.cu_qp_delta_enabled_flag << 6;
   result.pps_info_flags |= pps->flags.pps_slice_chroma_qp_offsets_present_flag << 7;
   result.pps_info_flags |= pps->flags.weighted_pred_flag << 8;
   result.pps_info_flags |= pps->flags.weighted_bipred_flag << 9;
   result.pps_info_flags |= pps->flags.transquant_bypass_enabled_flag << 10;
   result.pps_info_flags |= pps->flags.tiles_enabled_flag << 11;
   result.pps_info_flags |= pps->flags.entropy_coding_sync_enabled_flag << 12;
   result.pps_info_flags |= pps->flags.uniform_spacing_flag << 13;
   result.pps_info_flags |= pps->flags.loop_filter_across_tiles_enabled_flag << 14;
   result.pps_info_flags |= pps->flags.pps_loop_filter_across_slices_enabled_flag << 15;
   result.pps_info_flags |= pps->flags.deblocking_filter_override_enabled_flag << 16;
   result.pps_info_flags |= pps->flags.pps_deblocking_filter_disabled_flag << 17;
   result.pps_info_flags |= pps->flags.lists_modification_present_flag << 18;
            result.num_extra_slice_header_bits = pps->num_extra_slice_header_bits;
   result.num_long_term_ref_pic_sps = sps->num_long_term_ref_pics_sps;
   result.num_ref_idx_l0_default_active_minus1 = pps->num_ref_idx_l0_default_active_minus1;
   result.num_ref_idx_l1_default_active_minus1 = pps->num_ref_idx_l1_default_active_minus1;
   result.pps_cb_qp_offset = pps->pps_cb_qp_offset;
   result.pps_cr_qp_offset = pps->pps_cr_qp_offset;
   result.pps_beta_offset_div2 = pps->pps_beta_offset_div2;
   result.pps_tc_offset_div2 = pps->pps_tc_offset_div2;
   result.diff_cu_qp_delta_depth = pps->diff_cu_qp_delta_depth;
   result.num_tile_columns_minus1 = pps->num_tile_columns_minus1;
   result.num_tile_rows_minus1 = pps->num_tile_rows_minus1;
   result.log2_parallel_merge_level_minus2 = pps->log2_parallel_merge_level_minus2;
            for (i = 0; i < 19; ++i)
            for (i = 0; i < 21; ++i)
            result.num_delta_pocs_ref_rps_idx = h265_pic_info->pStdPictureInfo->NumDeltaPocsOfRefRpsIdx;
            uint8_t idxs[16];
   memset(result.poc_list, 0, 16 * sizeof(int));
   memset(result.ref_pic_list, 0x7f, 16);
   memset(idxs, 0xff, 16);
   for (i = 0; i < frame_info->referenceSlotCount; i++) {
      const struct VkVideoDecodeH265DpbSlotInfoKHR *dpb_slot =
         int idx = frame_info->pReferenceSlots[i].slotIndex;
   result.poc_list[i] = dpb_slot->pStdReferenceInfo->PicOrderCntVal;
   result.ref_pic_list[i] = idx;
      }
         #define IDXS(x) ((x) == 0xff ? 0xff : idxs[(x)])
      for (i = 0; i < 8; ++i)
            for (i = 0; i < 8; ++i)
            for (i = 0; i < 8; ++i)
            const StdVideoH265ScalingLists *scaling_lists = NULL;
   if (pps->flags.pps_scaling_list_data_present_flag)
         else if (sps->flags.sps_scaling_list_data_present_flag)
                     if (scaling_lists) {
      for (i = 0; i < STD_VIDEO_H265_SCALING_LIST_16X16_NUM_LISTS; ++i)
            for (i = 0; i < STD_VIDEO_H265_SCALING_LIST_32X32_NUM_LISTS; ++i)
               for (i = 0; i < 2; i++) {
      for (j = 0; j < 15; j++)
               if (vid->vk.h265.profile_idc == STD_VIDEO_H265_PROFILE_IDC_MAIN_10) {
      if (vid->vk.picture_format == VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16) {
      result.p010_mode = 1;
      } else {
      result.p010_mode = 0;
   result.luma_10to8 = 5;
   result.chroma_10to8 = 5;
   result.hevc_reserved[0] = 4; /* sclr_luma10to8 */
                     }
      static bool
   rvcn_dec_message_decode(struct radv_cmd_buffer *cmd_buffer, struct radv_video_session *vid,
               {
      struct radv_device *device = cmd_buffer->device;
   rvcn_dec_message_header_t *header;
   rvcn_dec_message_index_t *index_codec;
   rvcn_dec_message_decode_t *decode;
   rvcn_dec_message_index_t *index_dynamic_dpb = NULL;
   rvcn_dec_message_dynamic_dpb_t2_t *dynamic_dpb_t2 = NULL;
   void *codec;
   unsigned sizes = 0, offset_decode, offset_codec, offset_dynamic_dpb;
   struct radv_image_view *dst_iv = radv_image_view_from_handle(frame_info->dstPictureResource.imageViewBinding);
   struct radv_image *img = dst_iv->image;
   struct radv_image_plane *luma = &img->planes[0];
   struct radv_image_plane *chroma = &img->planes[1];
   struct radv_image_view *dpb_iv =
                  header = ptr;
            index_codec = (void *)((char *)header + sizes);
            if (vid->dpb_type == DPB_DYNAMIC_TIER_2) {
      index_dynamic_dpb = (void *)((char *)header + sizes);
               offset_decode = sizes;
   decode = (void *)((char *)header + sizes);
            if (vid->dpb_type == DPB_DYNAMIC_TIER_2) {
      offset_dynamic_dpb = sizes;
   dynamic_dpb_t2 = (void *)((char *)header + sizes);
               offset_codec = sizes;
                     header->header_size = sizeof(rvcn_dec_message_header_t);
   header->total_size = sizes;
   header->msg_type = RDECODE_MSG_DECODE;
   header->stream_handle = vid->stream_handle;
            header->index[0].message_id = RDECODE_MESSAGE_DECODE;
   header->index[0].offset = offset_decode;
   header->index[0].size = sizeof(rvcn_dec_message_decode_t);
   header->index[0].filled = 0;
            index_codec->offset = offset_codec;
   index_codec->size = sizeof(rvcn_dec_message_avc_t);
   index_codec->filled = 0;
            if (vid->dpb_type == DPB_DYNAMIC_TIER_2) {
      index_dynamic_dpb->message_id = RDECODE_MESSAGE_DYNAMIC_DPB;
   index_dynamic_dpb->offset = offset_dynamic_dpb;
   index_dynamic_dpb->filled = 0;
   ++header->num_buffers;
               decode->stream_type = vid->stream_type;
   decode->decode_flags = 0;
   decode->width_in_samples = frame_info->dstPictureResource.codedExtent.width;
                              decode->dt_size = dst_iv->image->planes[0].surface.total_size + dst_iv->image->planes[1].surface.total_size;
   decode->sct_size = 0;
                     decode->db_pitch = dpb->planes[0].surface.u.gfx9.surf_pitch;
   decode->db_aligned_height = dpb->planes[0].surface.u.gfx9.surf_height;
   decode->db_swizzle_mode = dpb->planes[0].surface.u.gfx9.swizzle_mode;
            decode->dt_pitch = luma->surface.u.gfx9.surf_pitch * luma->surface.blk_w;
            if (luma->surface.meta_offset) {
      fprintf(stderr, "DCC SURFACES NOT SUPPORTED.\n");
               decode->dt_tiling_mode = 0;
   decode->dt_swizzle_mode = luma->surface.u.gfx9.swizzle_mode;
   decode->dt_array_mode = device->physical_device->vid_addr_gfx_mode;
   decode->dt_field_mode = vid->interlaced ? 1 : 0;
   decode->dt_surf_tile_config = 0;
            decode->dt_luma_top_offset = luma->surface.u.gfx9.surf_offset;
            if (decode->dt_field_mode) {
      decode->dt_luma_bottom_offset = luma->surface.u.gfx9.surf_offset + luma->surface.u.gfx9.surf_slice_size;
      } else {
      decode->dt_luma_bottom_offset = decode->dt_luma_top_offset;
               *slice_offset = 0;
   switch (vid->vk.op) {
   case VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR: {
      rvcn_dec_message_avc_t avc = get_h264_msg(vid, params, frame_info, slice_offset, &decode->width_in_samples,
         memcpy(codec, (void *)&avc, sizeof(rvcn_dec_message_avc_t));
   index_codec->message_id = RDECODE_MESSAGE_AVC;
      }
   case VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR: {
      rvcn_dec_message_hevc_t hevc = get_h265_msg(device, vid, params, frame_info, it_ptr);
   memcpy(codec, (void *)&hevc, sizeof(rvcn_dec_message_hevc_t));
   index_codec->message_id = RDECODE_MESSAGE_HEVC;
      }
   default:
                           if (vid->dpb_type != DPB_DYNAMIC_TIER_2)
            uint64_t addr;
   for (int i = 0; i < frame_info->referenceSlotCount; i++) {
      struct radv_image_view *f_dpb_iv =
                  radv_cs_add_buffer(cmd_buffer->device->ws, cmd_buffer->cs, dpb_img->bindings[0].bo);
            dynamic_dpb_t2->dpbAddrLo[i] = addr;
   dynamic_dpb_t2->dpbAddrHi[i] = addr >> 32;
               radv_cs_add_buffer(cmd_buffer->device->ws, cmd_buffer->cs, dpb->bindings[0].bo);
            dynamic_dpb_t2->dpbCurrLo = addr;
            decode->decode_flags = 1;
   dynamic_dpb_t2->dpbConfigFlags = 0;
   dynamic_dpb_t2->dpbLumaPitch = luma->surface.u.gfx9.surf_pitch;
   dynamic_dpb_t2->dpbLumaAlignedHeight = luma->surface.u.gfx9.surf_height;
            dynamic_dpb_t2->dpbChromaPitch = chroma->surface.u.gfx9.surf_pitch;
   dynamic_dpb_t2->dpbChromaAlignedHeight = chroma->surface.u.gfx9.surf_height;
               }
      static struct ruvd_h264
   get_uvd_h264_msg(struct radv_video_session *vid, struct radv_video_session_params *params,
               {
      struct ruvd_h264 result;
   const struct VkVideoDecodeH264PictureInfoKHR *h264_pic_info =
                              const StdVideoH264SequenceParameterSet *sps =
         switch (sps->profile_idc) {
   case STD_VIDEO_H264_PROFILE_IDC_BASELINE:
      result.profile = RUVD_H264_PROFILE_BASELINE;
      case STD_VIDEO_H264_PROFILE_IDC_MAIN:
      result.profile = RUVD_H264_PROFILE_MAIN;
      case STD_VIDEO_H264_PROFILE_IDC_HIGH:
      result.profile = RUVD_H264_PROFILE_HIGH;
      default:
      fprintf(stderr, "UNSUPPORTED CODEC %d\n", sps->profile_idc);
   result.profile = RUVD_H264_PROFILE_MAIN;
               *width_in_samples = (sps->pic_width_in_mbs_minus1 + 1) * 16;
   *height_in_samples = (sps->pic_height_in_map_units_minus1 + 1) * 16;
   if (!sps->flags.frame_mbs_only_flag)
                           result.sps_info_flags |= sps->flags.direct_8x8_inference_flag << 0;
   result.sps_info_flags |= sps->flags.mb_adaptive_frame_field_flag << 1;
   result.sps_info_flags |= sps->flags.frame_mbs_only_flag << 2;
   result.sps_info_flags |= sps->flags.delta_pic_order_always_zero_flag << 3;
            result.bit_depth_luma_minus8 = sps->bit_depth_luma_minus8;
   result.bit_depth_chroma_minus8 = sps->bit_depth_chroma_minus8;
   result.log2_max_frame_num_minus4 = sps->log2_max_frame_num_minus4;
   result.pic_order_cnt_type = sps->pic_order_cnt_type;
                     const StdVideoH264PictureParameterSet *pps =
         result.pps_info_flags = 0;
   result.pps_info_flags |= pps->flags.transform_8x8_mode_flag << 0;
   result.pps_info_flags |= pps->flags.redundant_pic_cnt_present_flag << 1;
   result.pps_info_flags |= pps->flags.constrained_intra_pred_flag << 2;
   result.pps_info_flags |= pps->flags.deblocking_filter_control_present_flag << 3;
   result.pps_info_flags |= pps->weighted_bipred_idc << 4;
   result.pps_info_flags |= pps->flags.weighted_pred_flag << 6;
   result.pps_info_flags |= pps->flags.bottom_field_pic_order_in_frame_present_flag << 7;
            result.pic_init_qp_minus26 = pps->pic_init_qp_minus26;
   result.chroma_qp_index_offset = pps->chroma_qp_index_offset;
            StdVideoH264ScalingLists scaling_lists;
   vk_video_derive_h264_scaling_list(sps, pps, &scaling_lists);
            memset(it_ptr, 0, IT_SCALING_TABLE_SIZE);
   memcpy(it_ptr, result.scaling_list_4x4, 6 * 16);
            result.num_ref_idx_l0_active_minus1 = pps->num_ref_idx_l0_default_active_minus1;
            result.curr_field_order_cnt_list[0] = h264_pic_info->pStdPictureInfo->PicOrderCnt[0];
                     result.num_ref_frames = sps->max_num_ref_frames;
   memset(result.ref_frame_list, 0xff, sizeof(unsigned char) * 16);
   memset(result.frame_num_list, 0, sizeof(unsigned int) * 16);
   for (unsigned i = 0; i < frame_info->referenceSlotCount; i++) {
      int idx = frame_info->pReferenceSlots[i].slotIndex;
   const struct VkVideoDecodeH264DpbSlotInfoKHR *dpb_slot =
            result.frame_num_list[i] = dpb_slot->pStdReferenceInfo->FrameNum;
   result.field_order_cnt_list[i][0] = dpb_slot->pStdReferenceInfo->PicOrderCnt[0];
                     if (dpb_slot->pStdReferenceInfo->flags.used_for_long_term_reference)
      }
   result.curr_pic_ref_frame_num = frame_info->referenceSlotCount;
               }
      static struct ruvd_h265
   get_uvd_h265_msg(struct radv_device *device, struct radv_video_session *vid, struct radv_video_session_params *params,
         {
      struct ruvd_h265 result;
   int i, j;
   const struct VkVideoDecodeH265PictureInfoKHR *h265_pic_info =
                     const StdVideoH265SequenceParameterSet *sps =
         const StdVideoH265PictureParameterSet *pps =
            result.sps_info_flags = 0;
   result.sps_info_flags |= sps->flags.scaling_list_enabled_flag << 0;
   result.sps_info_flags |= sps->flags.amp_enabled_flag << 1;
   result.sps_info_flags |= sps->flags.sample_adaptive_offset_enabled_flag << 2;
   result.sps_info_flags |= sps->flags.pcm_enabled_flag << 3;
   result.sps_info_flags |= sps->flags.pcm_loop_filter_disabled_flag << 4;
   result.sps_info_flags |= sps->flags.long_term_ref_pics_present_flag << 5;
   result.sps_info_flags |= sps->flags.sps_temporal_mvp_enabled_flag << 6;
   result.sps_info_flags |= sps->flags.strong_intra_smoothing_enabled_flag << 7;
            if (device->physical_device->rad_info.family == CHIP_CARRIZO)
            result.chroma_format = sps->chroma_format_idc;
   result.bit_depth_luma_minus8 = sps->bit_depth_luma_minus8;
   result.bit_depth_chroma_minus8 = sps->bit_depth_chroma_minus8;
   result.log2_max_pic_order_cnt_lsb_minus4 = sps->log2_max_pic_order_cnt_lsb_minus4;
   result.sps_max_dec_pic_buffering_minus1 =
         result.log2_min_luma_coding_block_size_minus3 = sps->log2_min_luma_coding_block_size_minus3;
   result.log2_diff_max_min_luma_coding_block_size = sps->log2_diff_max_min_luma_coding_block_size;
   result.log2_min_transform_block_size_minus2 = sps->log2_min_luma_transform_block_size_minus2;
   result.log2_diff_max_min_transform_block_size = sps->log2_diff_max_min_luma_transform_block_size;
   result.max_transform_hierarchy_depth_inter = sps->max_transform_hierarchy_depth_inter;
   result.max_transform_hierarchy_depth_intra = sps->max_transform_hierarchy_depth_intra;
   if (sps->flags.pcm_enabled_flag) {
      result.pcm_sample_bit_depth_luma_minus1 = sps->pcm_sample_bit_depth_luma_minus1;
   result.pcm_sample_bit_depth_chroma_minus1 = sps->pcm_sample_bit_depth_chroma_minus1;
   result.log2_min_pcm_luma_coding_block_size_minus3 = sps->log2_min_pcm_luma_coding_block_size_minus3;
      }
            result.pps_info_flags = 0;
   result.pps_info_flags |= pps->flags.dependent_slice_segments_enabled_flag << 0;
   result.pps_info_flags |= pps->flags.output_flag_present_flag << 1;
   result.pps_info_flags |= pps->flags.sign_data_hiding_enabled_flag << 2;
   result.pps_info_flags |= pps->flags.cabac_init_present_flag << 3;
   result.pps_info_flags |= pps->flags.constrained_intra_pred_flag << 4;
   result.pps_info_flags |= pps->flags.transform_skip_enabled_flag << 5;
   result.pps_info_flags |= pps->flags.cu_qp_delta_enabled_flag << 6;
   result.pps_info_flags |= pps->flags.pps_slice_chroma_qp_offsets_present_flag << 7;
   result.pps_info_flags |= pps->flags.weighted_pred_flag << 8;
   result.pps_info_flags |= pps->flags.weighted_bipred_flag << 9;
   result.pps_info_flags |= pps->flags.transquant_bypass_enabled_flag << 10;
   result.pps_info_flags |= pps->flags.tiles_enabled_flag << 11;
   result.pps_info_flags |= pps->flags.entropy_coding_sync_enabled_flag << 12;
   result.pps_info_flags |= pps->flags.uniform_spacing_flag << 13;
   result.pps_info_flags |= pps->flags.loop_filter_across_tiles_enabled_flag << 14;
   result.pps_info_flags |= pps->flags.pps_loop_filter_across_slices_enabled_flag << 15;
   result.pps_info_flags |= pps->flags.deblocking_filter_override_enabled_flag << 16;
   result.pps_info_flags |= pps->flags.pps_deblocking_filter_disabled_flag << 17;
   result.pps_info_flags |= pps->flags.lists_modification_present_flag << 18;
            result.num_extra_slice_header_bits = pps->num_extra_slice_header_bits;
   result.num_long_term_ref_pic_sps = sps->num_long_term_ref_pics_sps;
   result.num_ref_idx_l0_default_active_minus1 = pps->num_ref_idx_l0_default_active_minus1;
   result.num_ref_idx_l1_default_active_minus1 = pps->num_ref_idx_l1_default_active_minus1;
   result.pps_cb_qp_offset = pps->pps_cb_qp_offset;
   result.pps_cr_qp_offset = pps->pps_cr_qp_offset;
   result.pps_beta_offset_div2 = pps->pps_beta_offset_div2;
   result.pps_tc_offset_div2 = pps->pps_tc_offset_div2;
   result.diff_cu_qp_delta_depth = pps->diff_cu_qp_delta_depth;
   result.num_tile_columns_minus1 = pps->num_tile_columns_minus1;
   result.num_tile_rows_minus1 = pps->num_tile_rows_minus1;
   result.log2_parallel_merge_level_minus2 = pps->log2_parallel_merge_level_minus2;
            for (i = 0; i < 19; ++i)
            for (i = 0; i < 21; ++i)
            result.num_delta_pocs_ref_rps_idx = h265_pic_info->pStdPictureInfo->NumDeltaPocsOfRefRpsIdx;
            uint8_t idxs[16];
   memset(result.poc_list, 0, 16 * sizeof(int));
   memset(result.ref_pic_list, 0x7f, 16);
   memset(idxs, 0xff, 16);
   for (i = 0; i < frame_info->referenceSlotCount; i++) {
      const struct VkVideoDecodeH265DpbSlotInfoKHR *dpb_slot =
         int idx = frame_info->pReferenceSlots[i].slotIndex;
   result.poc_list[i] = dpb_slot->pStdReferenceInfo->PicOrderCntVal;
   result.ref_pic_list[i] = idx;
      }
         #define IDXS(x) ((x) == 0xff ? 0xff : idxs[(x)])
      for (i = 0; i < 8; ++i)
            for (i = 0; i < 8; ++i)
            for (i = 0; i < 8; ++i)
            const StdVideoH265ScalingLists *scaling_lists = NULL;
   if (pps->flags.pps_scaling_list_data_present_flag)
         else if (sps->flags.sps_scaling_list_data_present_flag)
            update_h265_scaling(it_ptr, scaling_lists);
   if (scaling_lists) {
      for (i = 0; i < STD_VIDEO_H265_SCALING_LIST_16X16_NUM_LISTS; ++i)
            for (i = 0; i < STD_VIDEO_H265_SCALING_LIST_32X32_NUM_LISTS; ++i)
               for (i = 0; i < 2; i++) {
      for (j = 0; j < 15; j++)
               if (vid->vk.h265.profile_idc == STD_VIDEO_H265_PROFILE_IDC_MAIN_10) {
      if (vid->vk.picture_format == VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16) {
      result.p010_mode = 1;
      } else {
      result.p010_mode = 0;
   result.luma_10to8 = 5;
   result.chroma_10to8 = 5;
   result.sclr_luma10to8 = 4;
                     }
      static unsigned
   texture_offset_legacy(struct radeon_surf *surface, unsigned layer)
   {
      return (uint64_t)surface->u.legacy.level[0].offset_256B * 256 +
      }
      static bool
   ruvd_dec_message_decode(struct radv_device *device, struct radv_video_session *vid,
               {
      struct ruvd_msg *msg = ptr;
   struct radv_image_view *dst_iv = radv_image_view_from_handle(frame_info->dstPictureResource.imageViewBinding);
   struct radv_image *img = dst_iv->image;
   struct radv_image_plane *luma = &img->planes[0];
   struct radv_image_plane *chroma = &img->planes[1];
   struct radv_image_view *dpb_iv =
                  memset(msg, 0, sizeof(struct ruvd_msg));
   msg->size = sizeof(*msg);
   msg->msg_type = RUVD_MSG_DECODE;
   msg->stream_handle = vid->stream_handle;
            msg->body.decode.stream_type = vid->stream_type;
   msg->body.decode.decode_flags = 0x1;
   msg->body.decode.width_in_samples = frame_info->dstPictureResource.codedExtent.width;
            msg->body.decode.dpb_size = (vid->dpb_type != DPB_DYNAMIC_TIER_2) ? dpb->size : 0;
   msg->body.decode.bsd_size = frame_info->srcBufferRange;
            if (vid->stream_type == RUVD_CODEC_H264_PERF && device->physical_device->rad_info.family >= CHIP_POLARIS10)
            *slice_offset = 0;
   switch (vid->vk.op) {
   case VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR: {
      msg->body.decode.codec.h264 =
      get_uvd_h264_msg(vid, params, frame_info, slice_offset, &msg->body.decode.width_in_samples,
         }
   case VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR: {
               if (vid->ctx.mem)
            }
   default:
                           if (device->physical_device->rad_info.gfx_level >= GFX9) {
      msg->body.decode.dt_pitch = luma->surface.u.gfx9.surf_pitch * luma->surface.blk_w;
   msg->body.decode.dt_tiling_mode = RUVD_TILE_LINEAR;
   msg->body.decode.dt_array_mode = RUVD_ARRAY_MODE_LINEAR;
   msg->body.decode.dt_luma_top_offset = luma->surface.u.gfx9.surf_offset;
   msg->body.decode.dt_chroma_top_offset = chroma->surface.u.gfx9.surf_offset;
   if (msg->body.decode.dt_field_mode) {
      msg->body.decode.dt_luma_bottom_offset =
         msg->body.decode.dt_chroma_bottom_offset =
      } else {
      msg->body.decode.dt_luma_bottom_offset = msg->body.decode.dt_luma_top_offset;
      }
      } else {
      msg->body.decode.dt_pitch = luma->surface.u.legacy.level[0].nblk_x * luma->surface.blk_w;
   switch (luma->surface.u.legacy.level[0].mode) {
   case RADEON_SURF_MODE_LINEAR_ALIGNED:
      msg->body.decode.dt_tiling_mode = RUVD_TILE_LINEAR;
   msg->body.decode.dt_array_mode = RUVD_ARRAY_MODE_LINEAR;
      case RADEON_SURF_MODE_1D:
      msg->body.decode.dt_tiling_mode = RUVD_TILE_8X8;
   msg->body.decode.dt_array_mode = RUVD_ARRAY_MODE_1D_THIN;
      case RADEON_SURF_MODE_2D:
      msg->body.decode.dt_tiling_mode = RUVD_TILE_8X8;
   msg->body.decode.dt_array_mode = RUVD_ARRAY_MODE_2D_THIN;
      default:
      assert(0);
               msg->body.decode.dt_luma_top_offset = texture_offset_legacy(&luma->surface, 0);
   if (chroma)
         if (msg->body.decode.dt_field_mode) {
      msg->body.decode.dt_luma_bottom_offset = texture_offset_legacy(&luma->surface, 1);
   if (chroma)
      } else {
      msg->body.decode.dt_luma_bottom_offset = msg->body.decode.dt_luma_top_offset;
               if (chroma) {
      assert(luma->surface.u.legacy.bankw == chroma->surface.u.legacy.bankw);
   assert(luma->surface.u.legacy.bankh == chroma->surface.u.legacy.bankh);
               msg->body.decode.dt_surf_tile_config |= RUVD_BANK_WIDTH(util_logbase2(luma->surface.u.legacy.bankw));
   msg->body.decode.dt_surf_tile_config |= RUVD_BANK_HEIGHT(util_logbase2(luma->surface.u.legacy.bankh));
   msg->body.decode.dt_surf_tile_config |=
               if (device->physical_device->rad_info.family >= CHIP_STONEY)
            msg->body.decode.db_surf_tile_config = msg->body.decode.dt_surf_tile_config;
               }
      static void
   ruvd_dec_message_create(struct radv_video_session *vid, void *ptr)
   {
               memset(ptr, 0, sizeof(*msg));
   msg->size = sizeof(*msg);
   msg->msg_type = RUVD_MSG_CREATE;
   msg->stream_handle = vid->stream_handle;
   msg->body.create.stream_type = vid->stream_type;
   msg->body.create.width_in_samples = vid->vk.max_coded.width;
      }
      VKAPI_ATTR void VKAPI_CALL
   radv_CmdBeginVideoCodingKHR(VkCommandBuffer commandBuffer, const VkVideoBeginCodingInfoKHR *pBeginInfo)
   {
      RADV_FROM_HANDLE(radv_cmd_buffer, cmd_buffer, commandBuffer);
   RADV_FROM_HANDLE(radv_video_session, vid, pBeginInfo->videoSession);
            cmd_buffer->video.vid = vid;
      }
      static void
   radv_vcn_cmd_reset(struct radv_cmd_buffer *cmd_buffer)
   {
      struct radv_video_session *vid = cmd_buffer->video.vid;
            void *ptr;
   uint32_t out_offset;
            if (cmd_buffer->device->physical_device->vid_decode_ip == AMD_IP_VCN_UNIFIED)
            rvcn_dec_message_create(vid, ptr, size);
   send_cmd(cmd_buffer, RDECODE_CMD_SESSION_CONTEXT_BUFFER, vid->sessionctx.mem->bo, vid->sessionctx.offset);
   send_cmd(cmd_buffer, RDECODE_CMD_MSG_BUFFER, cmd_buffer->upload.upload_bo, out_offset);
            if (cmd_buffer->device->physical_device->vid_decode_ip != AMD_IP_VCN_UNIFIED) {
      radeon_check_space(cmd_buffer->device->ws, cmd_buffer->cs, 8);
   for (unsigned i = 0; i < 8; i++)
      } else
      }
      static void
   radv_uvd_cmd_reset(struct radv_cmd_buffer *cmd_buffer)
   {
      struct radv_video_session *vid = cmd_buffer->video.vid;
   uint32_t size = sizeof(struct ruvd_msg);
   void *ptr;
   uint32_t out_offset;
            ruvd_dec_message_create(vid, ptr);
   if (vid->sessionctx.mem)
                  /* pad out the IB to the 16 dword boundary - otherwise the fw seems to be unhappy */
   int padsize = vid->sessionctx.mem ? 4 : 6;
   radeon_check_space(cmd_buffer->device->ws, cmd_buffer->cs, padsize);
   for (unsigned i = 0; i < padsize; i++)
      }
      VKAPI_ATTR void VKAPI_CALL
   radv_CmdControlVideoCodingKHR(VkCommandBuffer commandBuffer, const VkVideoCodingControlInfoKHR *pCodingControlInfo)
   {
      RADV_FROM_HANDLE(radv_cmd_buffer, cmd_buffer, commandBuffer);
   if (pCodingControlInfo->flags & VK_VIDEO_CODING_CONTROL_RESET_BIT_KHR) {
      if (radv_has_uvd(cmd_buffer->device->physical_device))
         else
         }
      VKAPI_ATTR void VKAPI_CALL
   radv_CmdEndVideoCodingKHR(VkCommandBuffer commandBuffer, const VkVideoEndCodingInfoKHR *pEndCodingInfo)
   {
   }
      static void
   radv_uvd_decode_video(struct radv_cmd_buffer *cmd_buffer, const VkVideoDecodeInfoKHR *frame_info)
   {
      RADV_FROM_HANDLE(radv_buffer, src_buffer, frame_info->srcBuffer);
   struct radv_video_session *vid = cmd_buffer->video.vid;
   struct radv_video_session_params *params = cmd_buffer->video.params;
   unsigned size = sizeof(struct ruvd_msg);
   void *ptr, *fb_ptr, *it_ptr = NULL;
   uint32_t out_offset, fb_offset, it_offset = 0;
   struct radeon_winsys_bo *msg_bo, *fb_bo, *it_bo = NULL;
   unsigned fb_size =
            radv_vid_buffer_upload_alloc(cmd_buffer, fb_size, &fb_offset, &fb_ptr);
   fb_bo = cmd_buffer->upload.upload_bo;
   if (have_it(vid)) {
      radv_vid_buffer_upload_alloc(cmd_buffer, IT_SCALING_TABLE_SIZE, &it_offset, &it_ptr);
               radv_vid_buffer_upload_alloc(cmd_buffer, size, &out_offset, &ptr);
            uint32_t slice_offset;
   ruvd_dec_message_decode(cmd_buffer->device, vid, params, ptr, it_ptr, &slice_offset, frame_info);
   rvcn_dec_message_feedback(fb_ptr);
   if (vid->sessionctx.mem)
                  if (vid->dpb_type != DPB_DYNAMIC_TIER_2) {
      struct radv_image_view *dpb_iv =
         struct radv_image *dpb = dpb_iv->image;
               if (vid->ctx.mem)
            send_cmd(cmd_buffer, RDECODE_CMD_BITSTREAM_BUFFER, src_buffer->bo,
            struct radv_image_view *dst_iv = radv_image_view_from_handle(frame_info->dstPictureResource.imageViewBinding);
   struct radv_image *img = dst_iv->image;
   send_cmd(cmd_buffer, RDECODE_CMD_DECODING_TARGET_BUFFER, img->bindings[0].bo, img->bindings[0].offset);
   send_cmd(cmd_buffer, RDECODE_CMD_FEEDBACK_BUFFER, fb_bo, fb_offset);
   if (have_it(vid))
            radeon_check_space(cmd_buffer->device->ws, cmd_buffer->cs, 2);
      }
      static void
   radv_vcn_decode_video(struct radv_cmd_buffer *cmd_buffer, const VkVideoDecodeInfoKHR *frame_info)
   {
      RADV_FROM_HANDLE(radv_buffer, src_buffer, frame_info->srcBuffer);
   struct radv_video_session *vid = cmd_buffer->video.vid;
   struct radv_video_session_params *params = cmd_buffer->video.params;
   unsigned size = 0;
   void *ptr, *fb_ptr, *it_ptr = NULL;
   uint32_t out_offset, fb_offset, it_offset = 0;
            size += sizeof(rvcn_dec_message_header_t); /* header */
   size += sizeof(rvcn_dec_message_index_t);  /* codec */
   if (vid->dpb_type == DPB_DYNAMIC_TIER_2) {
      size += sizeof(rvcn_dec_message_index_t);
      }
   size += sizeof(rvcn_dec_message_decode_t); /* decode */
   switch (vid->vk.op) {
   case VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR:
      size += sizeof(rvcn_dec_message_avc_t);
      case VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR:
      size += sizeof(rvcn_dec_message_hevc_t);
      default:
                  radv_vid_buffer_upload_alloc(cmd_buffer, FB_BUFFER_SIZE, &fb_offset, &fb_ptr);
   fb_bo = cmd_buffer->upload.upload_bo;
   if (have_it(vid)) {
      radv_vid_buffer_upload_alloc(cmd_buffer, IT_SCALING_TABLE_SIZE, &it_offset, &it_ptr);
               radv_vid_buffer_upload_alloc(cmd_buffer, size, &out_offset, &ptr);
            if (cmd_buffer->device->physical_device->vid_decode_ip == AMD_IP_VCN_UNIFIED)
            uint32_t slice_offset;
   rvcn_dec_message_decode(cmd_buffer, vid, params, ptr, it_ptr, &slice_offset, frame_info);
   rvcn_dec_message_feedback(fb_ptr);
   send_cmd(cmd_buffer, RDECODE_CMD_SESSION_CONTEXT_BUFFER, vid->sessionctx.mem->bo, vid->sessionctx.offset);
            if (vid->dpb_type != DPB_DYNAMIC_TIER_2) {
      struct radv_image_view *dpb_iv =
         struct radv_image *dpb = dpb_iv->image;
               if (vid->ctx.mem)
            send_cmd(cmd_buffer, RDECODE_CMD_BITSTREAM_BUFFER, src_buffer->bo,
            struct radv_image_view *dst_iv = radv_image_view_from_handle(frame_info->dstPictureResource.imageViewBinding);
   struct radv_image *img = dst_iv->image;
   send_cmd(cmd_buffer, RDECODE_CMD_DECODING_TARGET_BUFFER, img->bindings[0].bo, img->bindings[0].offset);
   send_cmd(cmd_buffer, RDECODE_CMD_FEEDBACK_BUFFER, fb_bo, fb_offset);
   if (have_it(vid))
            if (cmd_buffer->device->physical_device->vid_decode_ip != AMD_IP_VCN_UNIFIED) {
      radeon_check_space(cmd_buffer->device->ws, cmd_buffer->cs, 2);
      } else
      }
      VKAPI_ATTR void VKAPI_CALL
   radv_CmdDecodeVideoKHR(VkCommandBuffer commandBuffer, const VkVideoDecodeInfoKHR *frame_info)
   {
               if (radv_has_uvd(cmd_buffer->device->physical_device))
         else
      }
      void
   radv_video_get_profile_alignments(struct radv_physical_device *pdevice, const VkVideoProfileListInfoKHR *profile_list,
         {
      vk_video_get_profile_alignments(profile_list, width_align_out, height_align_out);
   bool is_h265_main_10 = false;
   for (unsigned i = 0; i < profile_list->profileCount; i++) {
      if (profile_list->pProfiles[i].videoCodecOperation == VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR) {
      const struct VkVideoDecodeH265ProfileInfoKHR *h265_profile =
         if (h265_profile->stdProfileIdc == STD_VIDEO_H265_PROFILE_IDC_MAIN_10)
                  uint32_t db_alignment = radv_video_get_db_alignment(pdevice, 64, is_h265_main_10);
   *width_align_out = MAX2(*width_align_out, db_alignment);
      }
