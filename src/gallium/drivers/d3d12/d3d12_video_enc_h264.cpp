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
   #include "d3d12_video_enc_h264.h"
   #include "util/u_video.h"
   #include "d3d12_screen.h"
   #include "d3d12_format.h"
      #include <cmath>
      void
   d3d12_video_encoder_update_current_rate_control_h264(struct d3d12_video_encoder *pD3D12Enc,
         {
               pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc = {};
   pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_FrameRate.Numerator =
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_FrameRate.Denominator =
                  switch (picture->rate_ctrl[0].rate_ctrl_method) {
      case PIPE_H2645_ENC_RATE_CONTROL_METHOD_VARIABLE_SKIP:
   case PIPE_H2645_ENC_RATE_CONTROL_METHOD_VARIABLE:
   {
      pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Mode = D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_VBR;
   pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_VBR.TargetAvgBitRate =
                        if (D3D12_VIDEO_ENC_CBR_FORCE_VBV_EQUAL_BITRATE) {
      debug_printf("[d3d12_video_encoder_h264] d3d12_video_encoder_update_current_rate_control_h264 D3D12_VIDEO_ENC_CBR_FORCE_VBV_EQUAL_BITRATE environment variable is set, "
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags |=
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_CBR.VBVCapacity =
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_CBR.InitialVBVFullness =
      } else if (picture->rate_ctrl[0].app_requested_hrd_buffer) {
      debug_printf("[d3d12_video_encoder_h264] d3d12_video_encoder_update_current_rate_control_h264 HRD required by app,"
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags |=
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_VBR.VBVCapacity =
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_VBR.InitialVBVFullness =
               if (picture->rate_ctrl[0].max_au_size > 0) {
      pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags |=
                        debug_printf(
      "[d3d12_video_encoder_h264] d3d12_video_encoder_update_current_rate_control_h264 "
                  if (picture->rate_ctrl[0].app_requested_qp_range) {
      debug_printf(
      "[d3d12_video_encoder_h264] d3d12_video_encoder_update_current_rate_control_h264 "
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
      picture->rate_ctrl[0].peak_bitrate;
      #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
            if (D3D12_VIDEO_ENC_CBR_FORCE_VBV_EQUAL_BITRATE) {
      debug_printf("[d3d12_video_encoder_h264] d3d12_video_encoder_update_current_rate_control_h264 D3D12_VIDEO_ENC_CBR_FORCE_VBV_EQUAL_BITRATE environment variable is set, "
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags |=
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags |=
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_QVBR1.VBVCapacity =
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_QVBR1.InitialVBVFullness =
      } else if (picture->rate_ctrl[0].app_requested_hrd_buffer) {
      debug_printf("[d3d12_video_encoder_h264] d3d12_video_encoder_update_current_rate_control_h264 HRD required by app,"
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags |=
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags |=
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_QVBR1.VBVCapacity =
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_QVBR1.InitialVBVFullness =
   #endif
         if (picture->rate_ctrl[0].max_au_size > 0) {
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags |=
                        debug_printf(
      "[d3d12_video_encoder_h264] d3d12_video_encoder_update_current_rate_control_h264 "
                  if (picture->rate_ctrl[0].app_requested_qp_range) {
      debug_printf(
      "[d3d12_video_encoder_h264] d3d12_video_encoder_update_current_rate_control_h264 "
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
      debug_printf("[d3d12_video_encoder_h264] d3d12_video_encoder_update_current_rate_control_h264 D3D12_VIDEO_ENC_CBR_FORCE_VBV_EQUAL_BITRATE environment variable is set, "
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags |=
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_CBR.VBVCapacity =
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_CBR.InitialVBVFullness =
      } else if (picture->rate_ctrl[0].app_requested_hrd_buffer) {
      debug_printf("[d3d12_video_encoder_h264] d3d12_video_encoder_update_current_rate_control_h264 HRD required by app,"
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags |=
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_CBR.VBVCapacity =
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_CBR.InitialVBVFullness =
               if (picture->rate_ctrl[0].max_au_size > 0) {
      pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags |=
                        debug_printf(
      "[d3d12_video_encoder_h264] d3d12_video_encoder_update_current_rate_control_h264 "
                  if (picture->rate_ctrl[0].app_requested_qp_range) {
      debug_printf(
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
      debug_printf("[d3d12_video_encoder_h264] d3d12_video_encoder_update_current_rate_control_h264 invalid RC "
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Mode = D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_CQP;
   pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_CQP
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_CQP
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_CQP
                  if (memcmp(&previousConfig,
            &pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc,
         }
      void
   d3d12_video_encoder_update_current_frame_pic_params_info_h264(struct d3d12_video_encoder *pD3D12Enc,
                           {
      struct pipe_h264_enc_picture_desc *h264Pic = (struct pipe_h264_enc_picture_desc *) picture;
   d3d12_video_bitstream_builder_h264 *pH264BitstreamBuilder =
                           picParams.pH264PicData->pic_parameter_set_id = pH264BitstreamBuilder->get_active_pps_id();
   picParams.pH264PicData->idr_pic_id = h264Pic->idr_pic_id;
   picParams.pH264PicData->FrameType = d3d12_video_encoder_convert_frame_type_h264(h264Pic->picture_type);
   picParams.pH264PicData->PictureOrderCountNumber = h264Pic->pic_order_cnt;
            picParams.pH264PicData->List0ReferenceFramesCount = 0;
   picParams.pH264PicData->pList0ReferenceFrames = nullptr;
   picParams.pH264PicData->List1ReferenceFramesCount = 0;
            if (picParams.pH264PicData->FrameType == D3D12_VIDEO_ENCODER_FRAME_TYPE_H264_P_FRAME) {
      picParams.pH264PicData->List0ReferenceFramesCount = h264Pic->num_ref_idx_l0_active_minus1 + 1;
      } else if (picParams.pH264PicData->FrameType == D3D12_VIDEO_ENCODER_FRAME_TYPE_H264_B_FRAME) {
      picParams.pH264PicData->List0ReferenceFramesCount = h264Pic->num_ref_idx_l0_active_minus1 + 1;
   picParams.pH264PicData->pList0ReferenceFrames = h264Pic->ref_idx_l0_list;
   picParams.pH264PicData->List1ReferenceFramesCount = h264Pic->num_ref_idx_l1_active_minus1 + 1;
         }
      D3D12_VIDEO_ENCODER_FRAME_TYPE_H264
   d3d12_video_encoder_convert_frame_type_h264(enum pipe_h2645_enc_picture_type picType)
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
   d3d12_video_encoder_negotiate_current_h264_slices_configuration(struct d3d12_video_encoder *pD3D12Enc,
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
      int64_t curSlice = picture->slices_descriptors[sliceIdx].num_macroblocks;
   int64_t prevSlice = picture->slices_descriptors[sliceIdx - 1].num_macroblocks;
               uint32_t mbPerScanline =
                  if (!bUniformSizeSlices &&
      d3d12_video_encoder_check_subregion_mode_support(
                  if (D3D12_VIDEO_ENC_FALLBACK_SLICE_CONFIG) {   // Check if fallback mode is enabled, or we should just fail
            // Not supported to have custom slice sizes in D3D12 Video Encode fallback to uniform multi-slice
   debug_printf(
      "[d3d12_video_encoder_h264] WARNING: Requested slice control mode is not supported: All slices must "
   "have the same number of macroblocks. Falling back to encoding uniform %d slices per frame.\n",
      requestedSlicesMode =
         requestedSlicesConfig.NumberOfSlicesPerFrame = picture->num_slice_descriptors;
   debug_printf("[d3d12_video_encoder_h264] Using multi slice encoding mode: "
                  } else {
      debug_printf("[d3d12_video_encoder_h264] Requested slice control mode is not supported: All slices must "
                     } else if (bUniformSizeSlices && bSliceAligned &&
                           // Number of macroblocks per slice is aligned to a scanline width, in which case we can
   // use D3D12_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE_UNIFORM_PARTITIONING_ROWS_PER_SUBREGION
   requestedSlicesMode = D3D12_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE_UNIFORM_PARTITIONING_ROWS_PER_SUBREGION;
   requestedSlicesConfig.NumberOfRowsPerSlice = (picture->slices_descriptors[0].num_macroblocks / mbPerScanline);
   debug_printf("[d3d12_video_encoder_h264] Using multi slice encoding mode: "
                  } else if (bUniformSizeSlices &&
            d3d12_video_encoder_check_subregion_mode_support(
      pD3D12Enc,
   requestedSlicesMode =
         requestedSlicesConfig.NumberOfSlicesPerFrame = picture->num_slice_descriptors;
   debug_printf("[d3d12_video_encoder_h264] Using multi slice encoding mode: "
               } else if (D3D12_VIDEO_ENC_FALLBACK_SLICE_CONFIG) {   // Check if fallback mode is enabled, or we should just fail
            // Fallback to single slice encoding (assigned by default when initializing variables requestedSlicesMode,
   // requestedSlicesConfig)
   debug_printf(
      "[d3d12_video_encoder_h264] WARNING: Slice mode for %d slices with bUniformSizeSlices: %d - bSliceAligned "
   "%d not supported by the D3D12 driver, falling back to encoding a single slice per frame.\n",
   picture->num_slice_descriptors,
   bUniformSizeSlices,
   } else {
      debug_printf("[d3d12_video_encoder_h264] Requested slice control mode is not supported: All slices must "
                     } else {
      debug_printf("[d3d12_video_encoder_h264] Requested slice control mode is full frame. m_SlicesPartition_H264.NumberOfSlicesPerFrame = %d - m_encoderSliceConfigMode = %d \n",
               if (!d3d12_video_encoder_compare_slice_config_h264_hevc(
         pD3D12Enc->m_currentEncodeConfig.m_encoderSliceConfigMode,
   pD3D12Enc->m_currentEncodeConfig.m_encoderSliceConfigDesc.m_SlicesPartition_H264,
   requestedSlicesMode,
                  pD3D12Enc->m_currentEncodeConfig.m_encoderSliceConfigDesc.m_SlicesPartition_H264 = requestedSlicesConfig;
               }
      D3D12_VIDEO_ENCODER_MOTION_ESTIMATION_PRECISION_MODE
   d3d12_video_encoder_convert_h264_motion_configuration(struct d3d12_video_encoder *pD3D12Enc,
         {
         }
      D3D12_VIDEO_ENCODER_LEVELS_H264
   d3d12_video_encoder_convert_level_h264(uint32_t h264SpecLevel)
   {
      switch (h264SpecLevel) {
      case 10:
   {
         } break;
   case 11:
   {
         } break;
   case 12:
   {
         } break;
   case 13:
   {
         } break;
   case 20:
   {
         } break;
   case 21:
   {
         } break;
   case 22:
   {
         } break;
   case 30:
   {
         } break;
   case 31:
   {
         } break;
   case 32:
   {
         } break;
   case 40:
   {
         } break;
   case 41:
   {
         } break;
   case 42:
   {
         } break;
   case 50:
   {
         } break;
   case 51:
   {
         } break;
   case 52:
   {
         } break;
   case 60:
   {
         } break;
   case 61:
   {
         } break;
   case 62:
   {
         } break;
   default:
   {
               }
      void
   d3d12_video_encoder_convert_from_d3d12_level_h264(D3D12_VIDEO_ENCODER_LEVELS_H264 level12,
               {
      specLevel = 0;
            switch (level12) {
      case D3D12_VIDEO_ENCODER_LEVELS_H264_1:
   {
         } break;
   case D3D12_VIDEO_ENCODER_LEVELS_H264_1b:
   {
      specLevel = 11;
      } break;
   case D3D12_VIDEO_ENCODER_LEVELS_H264_11:
   {
         } break;
   case D3D12_VIDEO_ENCODER_LEVELS_H264_12:
   {
         } break;
   case D3D12_VIDEO_ENCODER_LEVELS_H264_13:
   {
         } break;
   case D3D12_VIDEO_ENCODER_LEVELS_H264_2:
   {
         } break;
   case D3D12_VIDEO_ENCODER_LEVELS_H264_21:
   {
         } break;
   case D3D12_VIDEO_ENCODER_LEVELS_H264_22:
   {
         } break;
   case D3D12_VIDEO_ENCODER_LEVELS_H264_3:
   {
         } break;
   case D3D12_VIDEO_ENCODER_LEVELS_H264_31:
   {
         } break;
   case D3D12_VIDEO_ENCODER_LEVELS_H264_32:
   {
         } break;
   case D3D12_VIDEO_ENCODER_LEVELS_H264_4:
   {
         } break;
   case D3D12_VIDEO_ENCODER_LEVELS_H264_41:
   {
         } break;
   case D3D12_VIDEO_ENCODER_LEVELS_H264_42:
   {
         } break;
   case D3D12_VIDEO_ENCODER_LEVELS_H264_5:
   {
         } break;
   case D3D12_VIDEO_ENCODER_LEVELS_H264_51:
   {
         } break;
   case D3D12_VIDEO_ENCODER_LEVELS_H264_52:
   {
         } break;
   case D3D12_VIDEO_ENCODER_LEVELS_H264_6:
   {
         } break;
   case D3D12_VIDEO_ENCODER_LEVELS_H264_61:
   {
         } break;
   case D3D12_VIDEO_ENCODER_LEVELS_H264_62:
   {
         } break;
   default:
   {
               }
      bool
   d3d12_video_encoder_update_h264_gop_configuration(struct d3d12_video_encoder *pD3D12Enc,
         {
      // Only update GOP when it begins
   if (picture->gop_cnt == 1) {
      uint32_t GOPCoeff = picture->i_remain;
   uint32_t GOPLength = picture->gop_size / GOPCoeff;
            if (picture->seq.pic_order_cnt_type == 1u) {
      debug_printf("[d3d12_video_encoder_h264] Upper layer is requesting pic_order_cnt_type %d but D3D12 Video "
                           const uint32_t max_pic_order_cnt_lsb = 2 * GOPLength;
   const uint32_t max_max_frame_num = GOPLength;
   double log2_max_frame_num_minus4 = std::max(0.0, std::ceil(std::log2(max_max_frame_num)) - 4);
   double log2_max_pic_order_cnt_lsb_minus4 = std::max(0.0, std::ceil(std::log2(max_pic_order_cnt_lsb)) - 4);
   assert(log2_max_frame_num_minus4 < UCHAR_MAX);
   assert(log2_max_pic_order_cnt_lsb_minus4 < UCHAR_MAX);
            // Set dirty flag if m_H264GroupOfPictures changed
   auto previousGOPConfig = pD3D12Enc->m_currentEncodeConfig.m_encoderGOPConfigDesc.m_H264GroupOfPictures;
   pD3D12Enc->m_currentEncodeConfig.m_encoderGOPConfigDesc.m_H264GroupOfPictures = {
      GOPLength,
   PPicturePeriod,
   static_cast<uint8_t>(picture->seq.pic_order_cnt_type),
   static_cast<uint8_t>(log2_max_frame_num_minus4),
               if (memcmp(&previousGOPConfig,
            &pD3D12Enc->m_currentEncodeConfig.m_encoderGOPConfigDesc.m_H264GroupOfPictures,
         }
      }
      D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_H264
   d3d12_video_encoder_convert_h264_codec_configuration(struct d3d12_video_encoder *pD3D12Enc,
               {
      is_supported = true;
   D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_H264 config = {
      D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_H264_FLAG_NONE,
   D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_H264_DIRECT_MODES_DISABLED,
               if (picture->pic_ctrl.enc_cabac_enable) {
                  pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_H264CodecCaps =
   {
      D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT_H264_FLAG_NONE,
               D3D12_FEATURE_DATA_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT capCodecConfigData = { };
   capCodecConfigData.NodeIndex = pD3D12Enc->m_NodeIndex;
   capCodecConfigData.Codec = D3D12_VIDEO_ENCODER_CODEC_H264;
   D3D12_VIDEO_ENCODER_PROFILE_H264 prof = d3d12_video_encoder_convert_profile_to_d3d12_enc_profile_h264(pD3D12Enc->base.profile);
   capCodecConfigData.Profile.pH264Profile = &prof;
   capCodecConfigData.Profile.DataSize = sizeof(prof);
   capCodecConfigData.CodecSupportLimits.pH264Support = &pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_H264CodecCaps;
            if(FAILED(pD3D12Enc->m_spD3D12VideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT, &capCodecConfigData, sizeof(capCodecConfigData))
         {
         debug_printf("D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION arguments not supported - DisableDeblockingFilterConfig (value %d) "
            "not allowed by DisableDeblockingFilterSupportedModes 0x%x cap reporting.",
      is_supported = false;
            if(((config.ConfigurationFlags & D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_H264_FLAG_ENABLE_CABAC_ENCODING) != 0)
         {
      debug_printf("D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION arguments are not supported - CABAC encoding mode not supported."
      " Ignoring the request for this feature flag on this encode session");
   // Disable it and keep going with a warning
               }
      bool
   d3d12_video_encoder_update_current_encoder_config_state_h264(struct d3d12_video_encoder *pD3D12Enc,
               {
               // Reset reconfig dirty flags
   pD3D12Enc->m_currentEncodeConfig.m_ConfigDirtyFlags = d3d12_video_encoder_config_dirty_flag_none;
   // Reset sequence changes flags
            // Set codec
   if (pD3D12Enc->m_currentEncodeConfig.m_encoderCodecDesc != D3D12_VIDEO_ENCODER_CODEC_H264) {
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
   if (h264Pic->seq.enc_frame_cropping_flag) {
      pD3D12Enc->m_currentEncodeConfig.m_FrameCroppingCodecConfig.left = h264Pic->seq.enc_frame_crop_left_offset;
   pD3D12Enc->m_currentEncodeConfig.m_FrameCroppingCodecConfig.right = h264Pic->seq.enc_frame_crop_right_offset;
   pD3D12Enc->m_currentEncodeConfig.m_FrameCroppingCodecConfig.top = h264Pic->seq.enc_frame_crop_top_offset;
   pD3D12Enc->m_currentEncodeConfig.m_FrameCroppingCodecConfig.bottom =
      } else {
      memset(&pD3D12Enc->m_currentEncodeConfig.m_FrameCroppingCodecConfig,
                     // Set profile
   auto targetProfile = d3d12_video_encoder_convert_profile_to_d3d12_enc_profile_h264(pD3D12Enc->base.profile);
   if (pD3D12Enc->m_currentEncodeConfig.m_encoderProfileDesc.m_H264Profile != targetProfile) {
         }
            // Set level
   auto targetLevel = d3d12_video_encoder_convert_level_h264(pD3D12Enc->base.level);
   if (pD3D12Enc->m_currentEncodeConfig.m_encoderLevelDesc.m_H264LevelSetting != targetLevel) {
         }
            // Set codec config
   bool is_supported = false;
   auto targetCodecConfig = d3d12_video_encoder_convert_h264_codec_configuration(pD3D12Enc, h264Pic, is_supported);
   if (!is_supported) {
                  if (memcmp(&pD3D12Enc->m_currentEncodeConfig.m_encoderCodecSpecificConfigDesc.m_H264Config,
            &targetCodecConfig,
      }
            // Set rate control
            // Set slices config
   if(!d3d12_video_encoder_negotiate_current_h264_slices_configuration(pD3D12Enc, h264Pic)) {
      debug_printf("d3d12_video_encoder_negotiate_current_h264_slices_configuration failed!\n");
               // Set GOP config
   if(!d3d12_video_encoder_update_h264_gop_configuration(pD3D12Enc, h264Pic)) {
      debug_printf("d3d12_video_encoder_update_h264_gop_configuration failed!\n");
               // m_currentEncodeConfig.m_encoderPicParamsDesc pic params are set in d3d12_video_encoder_reconfigure_encoder_objects
            // Set motion estimation config
   auto targetMotionLimit = d3d12_video_encoder_convert_h264_motion_configuration(pD3D12Enc, h264Pic);
   if (pD3D12Enc->m_currentEncodeConfig.m_encoderMotionPrecisionLimit != targetMotionLimit) {
      pD3D12Enc->m_currentEncodeConfig.m_ConfigDirtyFlags |=
      }
            ///
   /// Check for video encode support detailed capabilities
            // Will call for d3d12 driver support based on the initial requested features, then
   // try to fallback if any of them is not supported and return the negotiated d3d12 settings
   D3D12_FEATURE_DATA_VIDEO_ENCODER_SUPPORT1 capEncoderSupportData1 = {};
   if (!d3d12_video_encoder_negotiate_requested_features_and_d3d12_driver_caps(pD3D12Enc, capEncoderSupportData1)) {
      debug_printf("[d3d12_video_encoder_h264] After negotiating caps, D3D12_FEATURE_VIDEO_ENCODER_SUPPORT1 "
                  "arguments are not supported - "
                  ///
   // Calculate current settings based on the returned values from the caps query
   //
   pD3D12Enc->m_currentEncodeCapabilities.m_MaxSlicesInOutput =
      d3d12_video_encoder_calculate_max_slices_count_in_output(
      pD3D12Enc->m_currentEncodeConfig.m_encoderSliceConfigMode,
   &pD3D12Enc->m_currentEncodeConfig.m_encoderSliceConfigDesc.m_SlicesPartition_H264,
   pD3D12Enc->m_currentEncodeCapabilities.m_currentResolutionSupportCaps.MaxSubregionsNumber,
            //
   // Validate caps support returned values against current settings
   //
   if (pD3D12Enc->m_currentEncodeConfig.m_encoderProfileDesc.m_H264Profile !=
      pD3D12Enc->m_currentEncodeCapabilities.m_encoderSuggestedProfileDesc.m_H264Profile) {
   debug_printf("[d3d12_video_encoder_h264] Warning: Requested D3D12_VIDEO_ENCODER_PROFILE_H264 by upper layer: %d "
                           if (pD3D12Enc->m_currentEncodeConfig.m_encoderLevelDesc.m_H264LevelSetting !=
      pD3D12Enc->m_currentEncodeCapabilities.m_encoderLevelSuggestedDesc.m_H264LevelSetting) {
   debug_printf("[d3d12_video_encoder_h264] Warning: Requested D3D12_VIDEO_ENCODER_LEVELS_H264 by upper layer: %d "
                           if (pD3D12Enc->m_currentEncodeCapabilities.m_MaxSlicesInOutput >
      pD3D12Enc->m_currentEncodeCapabilities.m_currentResolutionSupportCaps.MaxSubregionsNumber) {
   debug_printf("[d3d12_video_encoder_h264] Desired number of subregions is not supported (higher than max "
            }
      }
      D3D12_VIDEO_ENCODER_PROFILE_H264
   d3d12_video_encoder_convert_profile_to_d3d12_enc_profile_h264(enum pipe_video_profile profile)
   {
      switch (profile) {
      case PIPE_VIDEO_PROFILE_MPEG4_AVC_CONSTRAINED_BASELINE:
   case PIPE_VIDEO_PROFILE_MPEG4_AVC_BASELINE:
   case PIPE_VIDEO_PROFILE_MPEG4_AVC_MAIN:
   {
            } break;
   case PIPE_VIDEO_PROFILE_MPEG4_AVC_HIGH:
   {
         } break;
   case PIPE_VIDEO_PROFILE_MPEG4_AVC_HIGH10:
   {
         } break;
   default:
   {
               }
      D3D12_VIDEO_ENCODER_CODEC
   d3d12_video_encoder_convert_codec_to_d3d12_enc_codec(enum pipe_video_profile profile)
   {
      switch (u_reduce_video_profile(profile)) {
      case PIPE_VIDEO_FORMAT_MPEG4_AVC:
   {
         } break;
   case PIPE_VIDEO_FORMAT_HEVC:
   {
         #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
         case PIPE_VIDEO_FORMAT_AV1:
   {
         #endif
         case PIPE_VIDEO_FORMAT_MPEG12:
   case PIPE_VIDEO_FORMAT_MPEG4:
   case PIPE_VIDEO_FORMAT_VC1:
   case PIPE_VIDEO_FORMAT_JPEG:
   case PIPE_VIDEO_FORMAT_VP9:
   case PIPE_VIDEO_FORMAT_UNKNOWN:
   default:
   {
               }
      bool
   d3d12_video_encoder_compare_slice_config_h264_hevc(
      D3D12_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE targetMode,
   D3D12_VIDEO_ENCODER_PICTURE_CONTROL_SUBREGIONS_LAYOUT_DATA_SLICES targetConfig,
   D3D12_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE otherMode,
      {
      return (targetMode == otherMode) &&
         (memcmp(&targetConfig,
      }
      uint32_t
   d3d12_video_encoder_build_codec_headers_h264(struct d3d12_video_encoder *pD3D12Enc)
   {
      D3D12_VIDEO_ENCODER_PICTURE_CONTROL_CODEC_DATA currentPicParams =
            auto profDesc = d3d12_video_encoder_get_current_profile_desc(pD3D12Enc);
   auto levelDesc = d3d12_video_encoder_get_current_level_desc(pD3D12Enc);
   auto codecConfigDesc = d3d12_video_encoder_get_current_codec_config_desc(pD3D12Enc);
            size_t writtenSPSBytesCount = 0;
   bool isFirstFrame = (pD3D12Enc->m_fenceValue == 1);
   bool writeNewSPS = isFirstFrame                                         // on first frame
                  d3d12_video_bitstream_builder_h264 *pH264BitstreamBuilder =
                           if (writeNewSPS) {
      // For every new SPS for reconfiguration, increase the active_sps_id
   if (!isFirstFrame) {
      active_seq_parameter_set_id++;
      }
   pH264BitstreamBuilder->build_sps(*profDesc.pH264Profile,
                                    *levelDesc.pH264LevelSetting,
   pD3D12Enc->m_currentEncodeConfig.m_encodeFormatInfo.Format,
   *codecConfigDesc.pH264Config,
   pD3D12Enc->m_currentEncodeConfig.m_encoderGOPConfigDesc.m_H264GroupOfPictures,
   active_seq_parameter_set_id,
            size_t writtenPPSBytesCount = 0;
   pH264BitstreamBuilder->build_pps(*profDesc.pH264Profile,
                                    *codecConfigDesc.pH264Config,
         std::vector<uint8_t>& active_pps = pH264BitstreamBuilder->get_active_pps();
   if ( (writtenPPSBytesCount != active_pps.size()) ||
            active_pps = pD3D12Enc->m_StagingHeadersBuffer;
   pD3D12Enc->m_BitstreamHeadersBuffer.resize(writtenSPSBytesCount + writtenPPSBytesCount);
      } else {
      writtenPPSBytesCount = 0;
               // Shrink buffer to fit the headers
   if (pD3D12Enc->m_BitstreamHeadersBuffer.size() > (writtenPPSBytesCount + writtenSPSBytesCount)) {
                     }
