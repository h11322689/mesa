   /*
   * Copyright © Microsoft Corporation
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
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "d3d12_video_enc.h"
   #include "d3d12_video_enc_hevc.h"
   #include "util/u_video.h"
   #include "d3d12_screen.h"
   #include "d3d12_format.h"
      #include <cmath>
      void
   d3d12_video_encoder_update_current_rate_control_hevc(struct d3d12_video_encoder *pD3D12Enc,
         {
               pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc = {};
   pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_FrameRate.Numerator =
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_FrameRate.Denominator =
                  switch (picture->rc.rate_ctrl_method) {
      case PIPE_H2645_ENC_RATE_CONTROL_METHOD_VARIABLE_SKIP:
   case PIPE_H2645_ENC_RATE_CONTROL_METHOD_VARIABLE:
   {
      pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Mode = D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_VBR;
   pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_VBR.TargetAvgBitRate =
                        if (D3D12_VIDEO_ENC_CBR_FORCE_VBV_EQUAL_BITRATE) {
      debug_printf("[d3d12_video_encoder_hevc] d3d12_video_encoder_update_current_rate_control_hevc D3D12_VIDEO_ENC_CBR_FORCE_VBV_EQUAL_BITRATE environment variable is set, "
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags |=
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_CBR.VBVCapacity =
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_CBR.InitialVBVFullness =
      } else if (picture->rc.app_requested_hrd_buffer) {
      debug_printf("[d3d12_video_encoder_hevc] d3d12_video_encoder_update_current_rate_control_hevc HRD required by app,"
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags |=
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_VBR.VBVCapacity =
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_VBR.InitialVBVFullness =
               if (picture->rc.max_au_size > 0) {
      pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags |=
                        debug_printf(
      "[d3d12_video_encoder_hevc] d3d12_video_encoder_update_current_rate_control_hevc "
                  if (picture->rc.app_requested_qp_range) {
      debug_printf(
      "[d3d12_video_encoder_hevc] d3d12_video_encoder_update_current_rate_control_hevc "
   "Upper layer requested explicit MinQP: %d MaxQP: %d\n",
      pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags |=
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_VBR.MinQP =
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_VBR.MaxQP =
      #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
            if (picture->quality_modes.level > 0) {
      pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags |=
                        // Convert between D3D12 definition and PIPE definition
   // D3D12: QualityVsSpeed must be in the range [0, D3D12_FEATURE_DATA_VIDEO_ENCODER_SUPPORT1.MaxQualityVsSpeed]
   // The lower the value, the fastest the encode operation
   // PIPE: The quality level range can be queried through the VAConfigAttribEncQualityRange attribute. 
   // A lower value means higher quality, and a value of 1 represents the highest quality. 
                  pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_VBR1.QualityVsSpeed =
   #endif
            } break;
   case PIPE_H2645_ENC_RATE_CONTROL_METHOD_QUALITY_VARIABLE:
   {
      pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Mode = D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_QVBR;
   pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_QVBR.TargetAvgBitRate =
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_QVBR.PeakBitRate =
      picture->rc.peak_bitrate;
   #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
            if (D3D12_VIDEO_ENC_CBR_FORCE_VBV_EQUAL_BITRATE) {
      debug_printf("[d3d12_video_encoder_hevc] d3d12_video_encoder_update_current_rate_control_hevc D3D12_VIDEO_ENC_CBR_FORCE_VBV_EQUAL_BITRATE environment variable is set, "
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags |=
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags |=
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_QVBR1.VBVCapacity =
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_QVBR1.InitialVBVFullness =
      } else if (picture->rc.app_requested_hrd_buffer) {
      debug_printf("[d3d12_video_encoder_hevc] d3d12_video_encoder_update_current_rate_control_hevc HRD required by app,"
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags |=
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags |=
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_QVBR1.VBVCapacity =
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_QVBR1.InitialVBVFullness =
   #endif
            if (picture->rc.max_au_size > 0) {
      pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags |=
                        debug_printf(
      "[d3d12_video_encoder_hevc] d3d12_video_encoder_update_current_rate_control_hevc "
                  if (picture->rc.app_requested_qp_range) {
      debug_printf(
      "[d3d12_video_encoder_hevc] d3d12_video_encoder_update_current_rate_control_hevc "
   "Upper layer requested explicit MinQP: %d MaxQP: %d\n",
      pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags |=
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_QVBR.MinQP =
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_QVBR.MaxQP =
      #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
            if (picture->quality_modes.level > 0) {
      pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags |=
                        // Convert between D3D12 definition and PIPE definition
   // D3D12: QualityVsSpeed must be in the range [0, D3D12_FEATURE_DATA_VIDEO_ENCODER_SUPPORT1.MaxQualityVsSpeed]
   // The lower the value, the fastest the encode operation
   // PIPE: The quality level range can be queried through the VAConfigAttribEncQualityRange attribute. 
   // A lower value means higher quality, and a value of 1 represents the highest quality. 
                  pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_QVBR1.QualityVsSpeed =
   #endif
            } break;
   case PIPE_H2645_ENC_RATE_CONTROL_METHOD_CONSTANT_SKIP:
   case PIPE_H2645_ENC_RATE_CONTROL_METHOD_CONSTANT:
   {
      pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Mode = D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_CBR;
                  /* For CBR mode, to guarantee bitrate of generated stream complies with
   * target bitrate (e.g. no over +/-10%), vbv_buffer_size and initial capacity should be same
   * as target bitrate. Controlled by OS env var D3D12_VIDEO_ENC_CBR_FORCE_VBV_EQUAL_BITRATE
   */
   if (D3D12_VIDEO_ENC_CBR_FORCE_VBV_EQUAL_BITRATE) {
      debug_printf("[d3d12_video_encoder_hevc] d3d12_video_encoder_update_current_rate_control_hevc D3D12_VIDEO_ENC_CBR_FORCE_VBV_EQUAL_BITRATE environment variable is set, "
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags |=
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_CBR.VBVCapacity =
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_CBR.InitialVBVFullness =
      } else if (picture->rc.app_requested_hrd_buffer) {
      debug_printf("[d3d12_video_encoder_hevc] d3d12_video_encoder_update_current_rate_control_hevc HRD required by app,"
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags |=
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_CBR.VBVCapacity =
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_CBR.InitialVBVFullness =
               if (picture->rc.max_au_size > 0) {
      pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags |=
                        debug_printf(
      "[d3d12_video_encoder_hevc] d3d12_video_encoder_update_current_rate_control_hevc "
                  if (picture->rc.app_requested_qp_range) {
      debug_printf(
      "[d3d12_video_encoder_hevc] d3d12_video_encoder_update_current_rate_control_hevc "
   "Upper layer requested explicit MinQP: %d MaxQP: %d\n",
      pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags |=
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_CBR.MinQP =
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_CBR.MaxQP =
      #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
            if (picture->quality_modes.level > 0) {
      pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags |=
                        // Convert between D3D12 definition and PIPE definition
   // D3D12: QualityVsSpeed must be in the range [0, D3D12_FEATURE_DATA_VIDEO_ENCODER_SUPPORT1.MaxQualityVsSpeed]
   // The lower the value, the fastest the encode operation
   // PIPE: The quality level range can be queried through the VAConfigAttribEncQualityRange attribute. 
   // A lower value means higher quality, and a value of 1 represents the highest quality. 
                  pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_CBR1.QualityVsSpeed =
   #endif
            } break;
   case PIPE_H2645_ENC_RATE_CONTROL_METHOD_DISABLE:
   {
      pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Mode = D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_CQP;
   pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_CQP
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_CQP
               #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
            if (picture->quality_modes.level > 0) {
      pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags |=
                        // Convert between D3D12 definition and PIPE definition
   // D3D12: QualityVsSpeed must be in the range [0, D3D12_FEATURE_DATA_VIDEO_ENCODER_SUPPORT1.MaxQualityVsSpeed]
   // The lower the value, the fastest the encode operation
   // PIPE: The quality level range can be queried through the VAConfigAttribEncQualityRange attribute. 
   // A lower value means higher quality, and a value of 1 represents the highest quality. 
                  pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_CQP1.QualityVsSpeed =
   #endif
         } break;
   default:
   {
      debug_printf("[d3d12_video_encoder_hevc] d3d12_video_encoder_update_current_rate_control_hevc invalid RC "
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Mode = D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_CQP;
   pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_CQP
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_CQP
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_CQP
                  if (memcmp(&previousConfig,
            &pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc,
         }
      void
   d3d12_video_encoder_update_current_frame_pic_params_info_hevc(struct d3d12_video_encoder *pD3D12Enc,
                           {
      struct pipe_h265_enc_picture_desc *hevcPic = (struct pipe_h265_enc_picture_desc *) picture;
   d3d12_video_bitstream_builder_hevc *pHEVCBitstreamBuilder =
                           picParams.pHEVCPicData->slice_pic_parameter_set_id = pHEVCBitstreamBuilder->get_active_pps_id();
   picParams.pHEVCPicData->FrameType = d3d12_video_encoder_convert_frame_type_hevc(hevcPic->picture_type);
            picParams.pHEVCPicData->List0ReferenceFramesCount = 0;
   picParams.pHEVCPicData->pList0ReferenceFrames = nullptr;
   picParams.pHEVCPicData->List1ReferenceFramesCount = 0;
            if (picParams.pHEVCPicData->FrameType == D3D12_VIDEO_ENCODER_FRAME_TYPE_HEVC_P_FRAME) {
      picParams.pHEVCPicData->List0ReferenceFramesCount = hevcPic->num_ref_idx_l0_active_minus1 + 1;
      } else if (picParams.pHEVCPicData->FrameType == D3D12_VIDEO_ENCODER_FRAME_TYPE_HEVC_B_FRAME) {
      picParams.pHEVCPicData->List0ReferenceFramesCount = hevcPic->num_ref_idx_l0_active_minus1 + 1;
   picParams.pHEVCPicData->pList0ReferenceFrames = hevcPic->ref_idx_l0_list;
   picParams.pHEVCPicData->List1ReferenceFramesCount = hevcPic->num_ref_idx_l1_active_minus1 + 1;
               if ((pD3D12Enc->m_currentEncodeConfig.m_encoderCodecSpecificConfigDesc.m_HEVCConfig.ConfigurationFlags 
      & D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_FLAG_ALLOW_REQUEST_INTRA_CONSTRAINED_SLICES) != 0)
   }
      D3D12_VIDEO_ENCODER_FRAME_TYPE_HEVC
   d3d12_video_encoder_convert_frame_type_hevc(enum pipe_h2645_enc_picture_type picType)
   {
      switch (picType) {
      case PIPE_H2645_ENC_PICTURE_TYPE_P:
   {
         } break;
   case PIPE_H2645_ENC_PICTURE_TYPE_B:
   {
         } break;
   case PIPE_H2645_ENC_PICTURE_TYPE_I:
   {
         } break;
   case PIPE_H2645_ENC_PICTURE_TYPE_IDR:
   {
         } break;
   default:
   {
               }
      ///
   /// Tries to configurate the encoder using the requested slice configuration
   /// or falls back to single slice encoding.
   ///
   bool
   d3d12_video_encoder_negotiate_current_hevc_slices_configuration(struct d3d12_video_encoder *pD3D12Enc,
         {
      ///
   /// Initialize single slice by default
   ///
   D3D12_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE requestedSlicesMode =
         D3D12_VIDEO_ENCODER_PICTURE_CONTROL_SUBREGIONS_LAYOUT_DATA_SLICES requestedSlicesConfig = {};
            ///
   /// Try to see if can accomodate for multi-slice request by user
   ///
   if (picture->num_slice_descriptors > 1) {
      /* Last slice can be less for rounding frame size and leave some error for mb rounding */
   bool bUniformSizeSlices = true;
   const double rounding_delta = 1.0;
   for (uint32_t sliceIdx = 1; (sliceIdx < picture->num_slice_descriptors - 1) && bUniformSizeSlices; sliceIdx++) {
      int64_t curSlice = picture->slices_descriptors[sliceIdx].num_ctu_in_slice;
   int64_t prevSlice = picture->slices_descriptors[sliceIdx - 1].num_ctu_in_slice;
               uint32_t subregion_block_pixel_size = pD3D12Enc->m_currentEncodeCapabilities.m_currentResolutionSupportCaps.SubregionBlockPixelsSize;
   uint32_t num_subregions_per_scanline =
            /* m_currentResolutionSupportCaps.SubregionBlockPixelsSize can be a multiple of MinCUSize to accomodate for HW requirements 
      So, if the allowed subregion (slice) pixel size partition is bigger (a multiple) than the CTU size, we have to adjust
               /* This assert should always be true according to the spec
         */
   uint8_t minCUSize = d3d12_video_encoder_convert_12cusize_to_pixel_size_hevc(pD3D12Enc->m_currentEncodeConfig.m_encoderCodecSpecificConfigDesc.m_HEVCConfig.MinLumaCodingUnitSize);
   assert((pD3D12Enc->m_currentEncodeCapabilities.m_currentResolutionSupportCaps.SubregionBlockPixelsSize 
            uint32_t subregionsize_to_ctu_factor = pD3D12Enc->m_currentEncodeCapabilities.m_currentResolutionSupportCaps.SubregionBlockPixelsSize / 
         uint32_t num_subregions_per_slice = picture->slices_descriptors[0].num_ctu_in_slice
                           if (!bUniformSizeSlices &&
      d3d12_video_encoder_check_subregion_mode_support(
                  if (D3D12_VIDEO_ENC_FALLBACK_SLICE_CONFIG) {   // Check if fallback mode is enabled, or we should just fail
            // Not supported to have custom slice sizes in D3D12 Video Encode fallback to uniform multi-slice
   debug_printf(
      "[d3d12_video_encoder_hevc] WARNING: Requested slice control mode is not supported: All slices must "
   "have the same number of macroblocks. Falling back to encoding uniform %d slices per frame.\n",
      requestedSlicesMode =
         requestedSlicesConfig.NumberOfSlicesPerFrame = picture->num_slice_descriptors;
   debug_printf("[d3d12_video_encoder_hevc] Using multi slice encoding mode: "
                  } else {
      debug_printf("[d3d12_video_encoder_hevc] Requested slice control mode is not supported: All slices must "
                     } else if (bUniformSizeSlices && bSliceAligned &&
                           // Number of subregion block per slice is aligned to a scanline width, in which case we can
   // use D3D12_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE_UNIFORM_PARTITIONING_ROWS_PER_SUBREGION
   requestedSlicesMode = D3D12_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE_UNIFORM_PARTITIONING_ROWS_PER_SUBREGION;
   requestedSlicesConfig.NumberOfRowsPerSlice = (num_subregions_per_slice / num_subregions_per_scanline);
   debug_printf("[d3d12_video_encoder_hevc] Using multi slice encoding mode: "
                  "D3D12_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE_UNIFORM_PARTITIONING_ROWS_PER_SUBREGION with "
   } else if (bUniformSizeSlices &&
            d3d12_video_encoder_check_subregion_mode_support(
      pD3D12Enc,
   requestedSlicesMode =
         requestedSlicesConfig.NumberOfSlicesPerFrame = picture->num_slice_descriptors;
   debug_printf("[d3d12_video_encoder_hevc] Using multi slice encoding mode: "
               } else if (D3D12_VIDEO_ENC_FALLBACK_SLICE_CONFIG) {   // Check if fallback mode is enabled, or we should just fail
            // Fallback to single slice encoding (assigned by default when initializing variables requestedSlicesMode,
   // requestedSlicesConfig)
   debug_printf(
      "[d3d12_video_encoder_hevc] WARNING: Slice mode for %d slices with bUniformSizeSlices: %d - bSliceAligned "
   "%d not supported by the D3D12 driver, falling back to encoding a single slice per frame.\n",
   picture->num_slice_descriptors,
   bUniformSizeSlices,
   } else {
      debug_printf("[d3d12_video_encoder_hevc] Requested slice control mode is not supported: All slices must "
                              if (!d3d12_video_encoder_isequal_slice_config_hevc(
         pD3D12Enc->m_currentEncodeConfig.m_encoderSliceConfigMode,
   pD3D12Enc->m_currentEncodeConfig.m_encoderSliceConfigDesc.m_SlicesPartition_HEVC,
   requestedSlicesMode,
                  pD3D12Enc->m_currentEncodeConfig.m_encoderSliceConfigDesc.m_SlicesPartition_HEVC = requestedSlicesConfig;
               }
      D3D12_VIDEO_ENCODER_MOTION_ESTIMATION_PRECISION_MODE
   d3d12_video_encoder_convert_hevc_motion_configuration(struct d3d12_video_encoder *pD3D12Enc,
         {
         }
      bool
   d3d12_video_encoder_update_hevc_gop_configuration(struct d3d12_video_encoder *pD3D12Enc,
         {
      // Only update GOP when it begins
   // This triggers DPB/encoder/heap re-creation, so only check on IDR when a GOP might change
   if ((picture->picture_type == PIPE_H2645_ENC_PICTURE_TYPE_IDR)
      || (picture->picture_type == PIPE_H2645_ENC_PICTURE_TYPE_I)) {
   uint32_t GOPLength = picture->seq.intra_period;
            const uint32_t max_pic_order_cnt_lsb = MAX2(16, util_next_power_of_two(GOPLength));
   double log2_max_pic_order_cnt_lsb_minus4 = std::max(0.0, std::ceil(std::log2(max_pic_order_cnt_lsb)) - 4);
            // Set dirty flag if m_HEVCGroupOfPictures changed
   auto previousGOPConfig = pD3D12Enc->m_currentEncodeConfig.m_encoderGOPConfigDesc.m_HEVCGroupOfPictures;
   pD3D12Enc->m_currentEncodeConfig.m_encoderGOPConfigDesc.m_HEVCGroupOfPictures = {
      GOPLength,
   PPicturePeriod,
               if (memcmp(&previousGOPConfig,
            &pD3D12Enc->m_currentEncodeConfig.m_encoderGOPConfigDesc.m_HEVCGroupOfPictures,
         }
      }
      D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC
   d3d12_video_encoder_convert_hevc_codec_configuration(struct d3d12_video_encoder *pD3D12Enc,
               {
      is_supported = true;
   uint32_t min_cu_size = (1 << (picture->seq.log2_min_luma_coding_block_size_minus3 + 3));
            uint32_t min_tu_size = (1 << (picture->seq.log2_min_transform_block_size_minus2 + 2));
            D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC config = {
      D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_FLAG_NONE,
   d3d12_video_encoder_convert_pixel_size_hevc_to_12cusize(min_cu_size),
   d3d12_video_encoder_convert_pixel_size_hevc_to_12cusize(max_cu_size),
   d3d12_video_encoder_convert_pixel_size_hevc_to_12tusize(min_tu_size),
   d3d12_video_encoder_convert_pixel_size_hevc_to_12tusize(max_tu_size),
   picture->seq.max_transform_hierarchy_depth_inter,
               pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_HEVCCodecCaps = 
   {
         D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT_HEVC_FLAG_NONE,
   config.MinLumaCodingUnitSize,
   config.MaxLumaCodingUnitSize,
   config.MinLumaTransformUnitSize,
   config.MaxLumaTransformUnitSize,
   config.max_transform_hierarchy_depth_inter,
            D3D12_FEATURE_DATA_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT capCodecConfigData = { };
   capCodecConfigData.NodeIndex = pD3D12Enc->m_NodeIndex;
   capCodecConfigData.Codec = D3D12_VIDEO_ENCODER_CODEC_HEVC;
   D3D12_VIDEO_ENCODER_PROFILE_HEVC prof = d3d12_video_encoder_convert_profile_to_d3d12_enc_profile_hevc(pD3D12Enc->base.profile);
   capCodecConfigData.Profile.pHEVCProfile = &prof;
   capCodecConfigData.Profile.DataSize = sizeof(prof);
   capCodecConfigData.CodecSupportLimits.pHEVCSupport = &pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_HEVCCodecCaps;
            if(FAILED(pD3D12Enc->m_spD3D12VideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT, &capCodecConfigData, sizeof(capCodecConfigData)))
         {
                  // Workaround for https://github.com/intel/libva/issues/641
   if ( !capCodecConfigData.IsSupported
      && ((picture->seq.max_transform_hierarchy_depth_inter == 0)
      {
      // Try and see if the values where 4 and overflowed in the 2 bit fields
   capCodecConfigData.CodecSupportLimits.pHEVCSupport->max_transform_hierarchy_depth_inter =
                        // Call the caps check again
   if(SUCCEEDED(pD3D12Enc->m_spD3D12VideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT, &capCodecConfigData, sizeof(capCodecConfigData)))
         {
      // If this was the case, then update the config return variable with the overriden values too
   is_supported = true;
   config.max_transform_hierarchy_depth_inter =
         config.max_transform_hierarchy_depth_intra =
                  if (!is_supported) {
      debug_printf("D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION arguments are not supported - "
      "Call to CheckFeatureCaps (D3D12_FEATURE_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT, ...) returned failure "
   "or not supported for Codec HEVC -  MinLumaSize %d - MaxLumaSize %d -  MinTransformSize %d - "
   "MaxTransformSize %d - Depth_inter %d - Depth intra %d\n",
   config.MinLumaCodingUnitSize,
   config.MaxLumaCodingUnitSize,
   config.MinLumaTransformUnitSize,
                                 if (picture->seq.amp_enabled_flag)
            if (picture->seq.sample_adaptive_offset_enabled_flag)
            if (picture->pic.pps_loop_filter_across_slices_enabled_flag)
            if (picture->pic.transform_skip_enabled_flag)
            if (picture->pic.constrained_intra_pred_flag)
            if(((config.ConfigurationFlags & D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_FLAG_DISABLE_LOOP_FILTER_ACROSS_SLICES) != 0)
         {
         debug_printf("D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION arguments are not supported - Disable deblocking across slice boundary mode not supported."
   " Ignoring the request for this feature flag on this encode session\n");
   // Disable it and keep going with a warning
            if(((config.ConfigurationFlags & D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_FLAG_ALLOW_REQUEST_INTRA_CONSTRAINED_SLICES) != 0)
         {
         debug_printf("D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION arguments are not supported - Intra slice constrained mode not supported."
   " Ignoring the request for this feature flag on this encode session\n");
   // Disable it and keep going with a warning
            if(((config.ConfigurationFlags & D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_FLAG_ENABLE_SAO_FILTER) != 0)
         {
         debug_printf("D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION arguments are not supported - SAO Filter mode not supported."
   " Ignoring the request for this feature flag on this encode session\n");
   // Disable it and keep going with a warning
               if(((config.ConfigurationFlags & D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_FLAG_USE_ASYMETRIC_MOTION_PARTITION) != 0)
         {
         debug_printf("D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION arguments are not supported - Asymetric motion partition not supported."
   " Ignoring the request for this feature flag on this encode session\n");
   // Disable it and keep going with a warning
            if(((config.ConfigurationFlags & D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_FLAG_USE_ASYMETRIC_MOTION_PARTITION) == 0)
         {
         debug_printf("D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION arguments are not supported - Asymetric motion partition is required to be set."
   " Enabling this HW required feature flag on this encode session\n");
   // HW doesn't support otherwise, so set it
            if(((config.ConfigurationFlags & D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_FLAG_ENABLE_TRANSFORM_SKIPPING) != 0)
         {
         debug_printf("D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION arguments are not supported - Allow transform skipping is not supported."
   " Ignoring the request for this feature flag on this encode session\n");
   // Disable it and keep going with a warning
            if(((config.ConfigurationFlags & D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_FLAG_USE_CONSTRAINED_INTRAPREDICTION) != 0)
         {
         debug_printf("D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION arguments are not supported - Constrained intra-prediction use is not supported."
   " Ignoring the request for this feature flag on this encode session\n");
   // Disable it and keep going with a warning
               }
      bool
   d3d12_video_encoder_update_current_encoder_config_state_hevc(struct d3d12_video_encoder *pD3D12Enc,
               {
               // Reset reconfig dirty flags
   pD3D12Enc->m_currentEncodeConfig.m_ConfigDirtyFlags = d3d12_video_encoder_config_dirty_flag_none;
   // Reset sequence changes flags
            // Set codec
   if (pD3D12Enc->m_currentEncodeConfig.m_encoderCodecDesc != D3D12_VIDEO_ENCODER_CODEC_HEVC) {
         }
            // Set input format
   DXGI_FORMAT targetFmt = d3d12_convert_pipe_video_profile_to_dxgi_format(pD3D12Enc->base.profile);
   if (pD3D12Enc->m_currentEncodeConfig.m_encodeFormatInfo.Format != targetFmt) {
                  pD3D12Enc->m_currentEncodeConfig.m_encodeFormatInfo = {};
   pD3D12Enc->m_currentEncodeConfig.m_encodeFormatInfo.Format = targetFmt;
   HRESULT hr = pD3D12Enc->m_pD3D12Screen->dev->CheckFeatureSupport(D3D12_FEATURE_FORMAT_INFO,
               if (FAILED(hr)) {
      debug_printf("CheckFeatureSupport failed with HR %x\n", hr);
               // Set resolution
   if ((pD3D12Enc->m_currentEncodeConfig.m_currentResolution.Width != srcTexture->width) ||
      (pD3D12Enc->m_currentEncodeConfig.m_currentResolution.Height != srcTexture->height)) {
      }
   pD3D12Enc->m_currentEncodeConfig.m_currentResolution.Width = srcTexture->width;
            // Set resolution codec dimensions (ie. cropping)
   memset(&pD3D12Enc->m_currentEncodeConfig.m_FrameCroppingCodecConfig, 0,
         pD3D12Enc->m_currentEncodeConfig.m_FrameCroppingCodecConfig.front = hevcPic->seq.pic_width_in_luma_samples;
   pD3D12Enc->m_currentEncodeConfig.m_FrameCroppingCodecConfig.back = hevcPic->seq.pic_height_in_luma_samples;
   if (hevcPic->seq.conformance_window_flag) {
      pD3D12Enc->m_currentEncodeConfig.m_FrameCroppingCodecConfig.left = hevcPic->seq.conf_win_left_offset;
   pD3D12Enc->m_currentEncodeConfig.m_FrameCroppingCodecConfig.right = hevcPic->seq.conf_win_right_offset;
   pD3D12Enc->m_currentEncodeConfig.m_FrameCroppingCodecConfig.top = hevcPic->seq.conf_win_top_offset;
      }
   // Set profile
   auto targetProfile = d3d12_video_encoder_convert_profile_to_d3d12_enc_profile_hevc(pD3D12Enc->base.profile);
   if (pD3D12Enc->m_currentEncodeConfig.m_encoderProfileDesc.m_HEVCProfile != targetProfile) {
         }
            // Set level
   auto targetLevel = d3d12_video_encoder_convert_level_hevc(hevcPic->seq.general_level_idc);
   auto targetTier = (hevcPic->seq.general_tier_flag == 0) ? D3D12_VIDEO_ENCODER_TIER_HEVC_MAIN : D3D12_VIDEO_ENCODER_TIER_HEVC_HIGH;
   if ( (pD3D12Enc->m_currentEncodeConfig.m_encoderLevelDesc.m_HEVCLevelSetting.Level != targetLevel)
      || (pD3D12Enc->m_currentEncodeConfig.m_encoderLevelDesc.m_HEVCLevelSetting.Tier != targetTier)) {
      }
   pD3D12Enc->m_currentEncodeConfig.m_encoderLevelDesc.m_HEVCLevelSetting.Tier = targetTier;
            // Set codec config
   bool is_supported = true;
   auto targetCodecConfig = d3d12_video_encoder_convert_hevc_codec_configuration(pD3D12Enc, hevcPic, is_supported);
   if(!is_supported) {
                  if (memcmp(&pD3D12Enc->m_currentEncodeConfig.m_encoderCodecSpecificConfigDesc.m_HEVCConfig,
            &targetCodecConfig,
      }
            // Set rate control
            ///
   /// Check for video encode support detailed capabilities
            // Will call for d3d12 driver support based on the initial requested features, then
   // try to fallback if any of them is not supported and return the negotiated d3d12 settings
   D3D12_FEATURE_DATA_VIDEO_ENCODER_SUPPORT1 capEncoderSupportData1 = {};
   if (!d3d12_video_encoder_negotiate_requested_features_and_d3d12_driver_caps(pD3D12Enc, capEncoderSupportData1)) {
      debug_printf("[d3d12_video_encoder_hevc] After negotiating caps, D3D12_FEATURE_VIDEO_ENCODER_SUPPORT1 "
                  "arguments are not supported - "
                  ///
   // Calculate current settings based on the returned values from the caps query
   //
   pD3D12Enc->m_currentEncodeCapabilities.m_MaxSlicesInOutput =
      d3d12_video_encoder_calculate_max_slices_count_in_output(
      pD3D12Enc->m_currentEncodeConfig.m_encoderSliceConfigMode,
   &pD3D12Enc->m_currentEncodeConfig.m_encoderSliceConfigDesc.m_SlicesPartition_HEVC,
   pD3D12Enc->m_currentEncodeCapabilities.m_currentResolutionSupportCaps.MaxSubregionsNumber,
            // Set slices config
   if(!d3d12_video_encoder_negotiate_current_hevc_slices_configuration(pD3D12Enc, hevcPic)) {
      debug_printf("d3d12_video_encoder_negotiate_current_hevc_slices_configuration failed!\n");
               // Set GOP config
   if(!d3d12_video_encoder_update_hevc_gop_configuration(pD3D12Enc, hevcPic)) {
      debug_printf("d3d12_video_encoder_update_hevc_gop_configuration failed!\n");
               // m_currentEncodeConfig.m_encoderPicParamsDesc pic params are set in d3d12_video_encoder_reconfigure_encoder_objects
            // Set motion estimation config
   auto targetMotionLimit = d3d12_video_encoder_convert_hevc_motion_configuration(pD3D12Enc, hevcPic);
   if (pD3D12Enc->m_currentEncodeConfig.m_encoderMotionPrecisionLimit != targetMotionLimit) {
      pD3D12Enc->m_currentEncodeConfig.m_ConfigDirtyFlags |=
      }
            //
   // Validate caps support returned values against current settings
   //
   if (pD3D12Enc->m_currentEncodeConfig.m_encoderProfileDesc.m_HEVCProfile !=
      pD3D12Enc->m_currentEncodeCapabilities.m_encoderSuggestedProfileDesc.m_HEVCProfile) {
   debug_printf("[d3d12_video_encoder_hevc] Warning: Requested D3D12_VIDEO_ENCODER_PROFILE_HEVC by upper layer: %d "
                           if (pD3D12Enc->m_currentEncodeConfig.m_encoderLevelDesc.m_HEVCLevelSetting.Tier !=
      pD3D12Enc->m_currentEncodeCapabilities.m_encoderLevelSuggestedDesc.m_HEVCLevelSetting.Tier) {
   debug_printf("[d3d12_video_encoder_hevc] Warning: Requested D3D12_VIDEO_ENCODER_LEVELS_HEVC.Tier by upper layer: %d "
                           if (pD3D12Enc->m_currentEncodeConfig.m_encoderLevelDesc.m_HEVCLevelSetting.Level !=
      pD3D12Enc->m_currentEncodeCapabilities.m_encoderLevelSuggestedDesc.m_HEVCLevelSetting.Level) {
   debug_printf("[d3d12_video_encoder_hevc] Warning: Requested D3D12_VIDEO_ENCODER_LEVELS_HEVC.Level by upper layer: %d "
                           if (pD3D12Enc->m_currentEncodeCapabilities.m_MaxSlicesInOutput >
      pD3D12Enc->m_currentEncodeCapabilities.m_currentResolutionSupportCaps.MaxSubregionsNumber) {
   debug_printf("[d3d12_video_encoder_hevc] Desired number of subregions is not supported (higher than max "
            }
      }
      D3D12_VIDEO_ENCODER_PROFILE_HEVC
   d3d12_video_encoder_convert_profile_to_d3d12_enc_profile_hevc(enum pipe_video_profile profile)
   {
      switch (profile) {
      case PIPE_VIDEO_PROFILE_HEVC_MAIN:
   {
            } break;
   case PIPE_VIDEO_PROFILE_HEVC_MAIN_10:
   {
         } break;
   default:
   {
               }
      bool
   d3d12_video_encoder_isequal_slice_config_hevc(
      D3D12_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE targetMode,
   D3D12_VIDEO_ENCODER_PICTURE_CONTROL_SUBREGIONS_LAYOUT_DATA_SLICES targetConfig,
   D3D12_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE otherMode,
      {
      return (targetMode == otherMode) &&
         (memcmp(&targetConfig,
      }
      uint32_t
   d3d12_video_encoder_build_codec_headers_hevc(struct d3d12_video_encoder *pD3D12Enc)
   {
      D3D12_VIDEO_ENCODER_PICTURE_CONTROL_CODEC_DATA currentPicParams =
            auto profDesc = d3d12_video_encoder_get_current_profile_desc(pD3D12Enc);
   auto levelDesc = d3d12_video_encoder_get_current_level_desc(pD3D12Enc);
   auto codecConfigDesc = d3d12_video_encoder_get_current_codec_config_desc(pD3D12Enc);
            size_t writtenSPSBytesCount = 0;
   size_t writtenVPSBytesCount = 0;
   bool isFirstFrame = (pD3D12Enc->m_fenceValue == 1);
   bool writeNewSPS = isFirstFrame                                         // on first frame
                  d3d12_video_bitstream_builder_hevc *pHEVCBitstreamBuilder =
                  uint32_t active_seq_parameter_set_id = pHEVCBitstreamBuilder->get_active_sps_id();
   uint32_t active_video_parameter_set_id = pHEVCBitstreamBuilder->get_active_vps_id();
      bool writeNewVPS = isFirstFrame;
      if (writeNewVPS) {
      bool gopHasBFrames = (pD3D12Enc->m_currentEncodeConfig.m_encoderGOPConfigDesc.m_HEVCGroupOfPictures.PPicturePeriod > 1);
   pHEVCBitstreamBuilder->build_vps(*profDesc.pHEVCProfile,
                                    *levelDesc.pHEVCLevelSetting,
   pD3D12Enc->m_currentEncodeConfig.m_encodeFormatInfo.Format,
            if (writeNewSPS) {
      // For every new SPS for reconfiguration, increase the active_sps_id
   if (!isFirstFrame) {
      active_seq_parameter_set_id++;
               pHEVCBitstreamBuilder->build_sps(
                        pHEVCBitstreamBuilder->get_latest_vps(),
   active_seq_parameter_set_id,
   pD3D12Enc->m_currentEncodeConfig.m_currentResolution,
   pD3D12Enc->m_currentEncodeConfig.m_FrameCroppingCodecConfig,
   pD3D12Enc->m_currentEncodeCapabilities.m_currentResolutionSupportCaps.SubregionBlockPixelsSize,
   pD3D12Enc->m_currentEncodeConfig.m_encodeFormatInfo.Format,
   *codecConfigDesc.pHEVCConfig,
                     pHEVCBitstreamBuilder->build_pps(pHEVCBitstreamBuilder->get_latest_sps(),
                                          std::vector<uint8_t>& active_pps = pHEVCBitstreamBuilder->get_active_pps();
   if ( (writtenPPSBytesCount != active_pps.size()) ||
            active_pps = pD3D12Enc->m_StagingHeadersBuffer;
   pD3D12Enc->m_BitstreamHeadersBuffer.resize(writtenSPSBytesCount + writtenVPSBytesCount + writtenPPSBytesCount);
      } else {
      writtenPPSBytesCount = 0;
               // Shrink buffer to fit the headers
   if (pD3D12Enc->m_BitstreamHeadersBuffer.size() > (writtenPPSBytesCount + writtenSPSBytesCount + writtenVPSBytesCount)) {
                     }
