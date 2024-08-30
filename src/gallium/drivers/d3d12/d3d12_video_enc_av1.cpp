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
   #include "d3d12_video_enc_av1.h"
   #include "util/u_video.h"
   #include "d3d12_resource.h"
   #include "d3d12_screen.h"
   #include "d3d12_format.h"
   #include <cmath>
      void
   d3d12_video_encoder_update_current_rate_control_av1(struct d3d12_video_encoder *pD3D12Enc,
         {
               pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc = {};
   pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_FrameRate.Numerator = picture->rc[0].frame_rate_num;
   pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_FrameRate.Denominator = picture->rc[0].frame_rate_den;
            switch (picture->rc[0].rate_ctrl_method) {
      case PIPE_H2645_ENC_RATE_CONTROL_METHOD_VARIABLE_SKIP:
   case PIPE_H2645_ENC_RATE_CONTROL_METHOD_VARIABLE:
   {
      pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Mode = D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_VBR;
   pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_VBR.TargetAvgBitRate =
                        if (D3D12_VIDEO_ENC_CBR_FORCE_VBV_EQUAL_BITRATE) {
      debug_printf(
      "[d3d12_video_encoder_av1] d3d12_video_encoder_update_current_rate_control_av1 "
   "D3D12_VIDEO_ENC_CBR_FORCE_VBV_EQUAL_BITRATE environment variable is set, "
   ", forcing VBV Size = VBV Initial Capacity = Target Bitrate = %" PRIu64 " (bits)\n",
      pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags |=
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_CBR.VBVCapacity =
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_CBR.InitialVBVFullness =
      } else if (picture->rc[0].app_requested_hrd_buffer) {
      debug_printf(
      "[d3d12_video_encoder_av1] d3d12_video_encoder_update_current_rate_control_av1 HRD required by app,"
   " setting VBV Size = %d (bits) - VBV Initial Capacity %d (bits)\n",
   picture->rc[0].vbv_buffer_size,
      pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags |=
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_VBR.VBVCapacity =
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_VBR.InitialVBVFullness =
               if (picture->rc[0].max_au_size > 0) {
      pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags |=
                        debug_printf(
      "[d3d12_video_encoder_av1] d3d12_video_encoder_update_current_rate_control_av1 "
                  if (picture->rc[0].app_requested_qp_range) {
      debug_printf("[d3d12_video_encoder_av1] d3d12_video_encoder_update_current_rate_control_av1 "
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
               #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
            if (D3D12_VIDEO_ENC_CBR_FORCE_VBV_EQUAL_BITRATE) {
      debug_printf("[d3d12_video_encoder_av1] d3d12_video_encoder_update_current_rate_control_av1 "
               "D3D12_VIDEO_ENC_CBR_FORCE_VBV_EQUAL_BITRATE environment variable is set, "
   ", forcing VBV Size = VBV Initial Capacity = Target Bitrate = %" PRIu64 " (bits)\n",
   pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags |=
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags |=
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_QVBR1.VBVCapacity =
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_QVBR1.InitialVBVFullness =
      } else if (picture->rc[0].app_requested_hrd_buffer) {
      debug_printf(
      "[d3d12_video_encoder_av1] d3d12_video_encoder_update_current_rate_control_av1 HRD required by app,"
   " setting VBV Size = %d (bits) - VBV Initial Capacity %d (bits)\n",
   picture->rc[0].vbv_buffer_size,
      pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags |=
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags |=
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_QVBR1.VBVCapacity =
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_QVBR1.InitialVBVFullness =
   #endif
            if (picture->rc[0].max_au_size > 0) {
      pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags |=
                        debug_printf(
      "[d3d12_video_encoder_av1] d3d12_video_encoder_update_current_rate_control_av1 "
                  if (picture->rc[0].app_requested_qp_range) {
      debug_printf("[d3d12_video_encoder_av1] d3d12_video_encoder_update_current_rate_control_av1 "
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
      debug_printf(
      "[d3d12_video_encoder_av1] d3d12_video_encoder_update_current_rate_control_av1 "
   "D3D12_VIDEO_ENC_CBR_FORCE_VBV_EQUAL_BITRATE environment variable is set, "
   ", forcing VBV Size = VBV Initial Capacity = Target Bitrate = %" PRIu64 " (bits)\n",
      pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags |=
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_CBR.VBVCapacity =
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_CBR.InitialVBVFullness =
      } else if (picture->rc[0].app_requested_hrd_buffer) {
      debug_printf(
      "[d3d12_video_encoder_av1] d3d12_video_encoder_update_current_rate_control_av1 HRD required by app,"
   " setting VBV Size = %d (bits) - VBV Initial Capacity %d (bits)\n",
   picture->rc[0].vbv_buffer_size,
      pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags |=
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_CBR.VBVCapacity =
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_CBR.InitialVBVFullness =
               if (picture->rc[0].max_au_size > 0) {
      pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags |=
                        debug_printf(
      "[d3d12_video_encoder_av1] d3d12_video_encoder_update_current_rate_control_av1 "
                  if (picture->rc[0].app_requested_qp_range) {
      debug_printf("[d3d12_video_encoder_av1] d3d12_video_encoder_update_current_rate_control_av1 "
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
      .ConstantQP_InterPredictedFrame_PrevRefOnly =
      pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_CQP
         #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
            if (picture->quality_modes.level > 0) {
      pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags |=
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
      debug_printf("[d3d12_video_encoder_av1] d3d12_video_encoder_update_current_rate_control_av1 invalid RC "
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Mode = D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_CQP;
   pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_CQP
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_CQP
         pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Config.m_Configuration_CQP
                  if (memcmp(&previousConfig,
            &pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc,
         }
      //
   // Returns AV1 extra size on top of the usual base metadata layout size
   //
   size_t
   d3d12_video_encoder_calculate_metadata_resolved_buffer_size_av1(uint32_t maxSliceNumber)
   {
      return sizeof(D3D12_VIDEO_ENCODER_AV1_PICTURE_CONTROL_SUBREGIONS_LAYOUT_DATA_TILES) +
      }
      void
   d3d12_video_encoder_convert_d3d12_to_spec_level_av1(D3D12_VIDEO_ENCODER_AV1_LEVELS level12, uint32_t &specLevel)
   {
      // Enum matches values as in seq_level_idx
      }
      void
   d3d12_video_encoder_convert_d3d12_to_spec_tier_av1(D3D12_VIDEO_ENCODER_AV1_TIER tier12, uint32_t &specTier)
   {
      // Enum matches values as in seq_level_idx
      }
      void
   d3d12_video_encoder_convert_spec_to_d3d12_level_av1(uint32_t specLevel, D3D12_VIDEO_ENCODER_AV1_LEVELS &level12)
   {
      // Enum matches values as in seq_level_idx
      }
      void
   d3d12_video_encoder_convert_spec_to_d3d12_tier_av1(uint32_t specTier, D3D12_VIDEO_ENCODER_AV1_TIER &tier12)
   {
      // Enum matches values as in seq_tier
      }
      uint32_t
   d3d12_video_encoder_convert_d3d12_profile_to_spec_profile_av1(D3D12_VIDEO_ENCODER_AV1_PROFILE profile)
   {
      switch (profile) {
      case D3D12_VIDEO_ENCODER_AV1_PROFILE_MAIN:
   {
         } break;
   default:
   {
               }
      D3D12_VIDEO_ENCODER_AV1_PROFILE
   d3d12_video_encoder_convert_profile_to_d3d12_enc_profile_av1(enum pipe_video_profile profile)
   {
      switch (profile) {
      case PIPE_VIDEO_PROFILE_AV1_MAIN:
   {
         } break;
   default:
   {
               }
      bool
   d3d12_video_encoder_update_av1_gop_configuration(struct d3d12_video_encoder *pD3D12Enc,
         {
      static_assert((unsigned) D3D12_VIDEO_ENCODER_AV1_FRAME_TYPE_KEY_FRAME == (unsigned) PIPE_AV1_ENC_FRAME_TYPE_KEY);
   static_assert((unsigned) D3D12_VIDEO_ENCODER_AV1_FRAME_TYPE_INTER_FRAME == (unsigned) PIPE_AV1_ENC_FRAME_TYPE_INTER);
   static_assert((unsigned) D3D12_VIDEO_ENCODER_AV1_FRAME_TYPE_INTRA_ONLY_FRAME ==
         static_assert((unsigned) D3D12_VIDEO_ENCODER_AV1_FRAME_TYPE_SWITCH_FRAME ==
            // Only update GOP when it begins
   // This triggers DPB/encoder/heap re-creation, so only check on IDR when a GOP might change
   if ((picture->frame_type == PIPE_AV1_ENC_FRAME_TYPE_INTRA_ONLY) ||
      (picture->frame_type == PIPE_AV1_ENC_FRAME_TYPE_KEY)) {
   uint32_t GOPLength = picture->seq.intra_period;
            // Set dirty flag if m_AV1SequenceStructure changed
   auto previousGOPConfig = pD3D12Enc->m_currentEncodeConfig.m_encoderGOPConfigDesc.m_AV1SequenceStructure;
   pD3D12Enc->m_currentEncodeConfig.m_encoderGOPConfigDesc.m_AV1SequenceStructure = {
      GOPLength,
               if (memcmp(&previousGOPConfig,
            &pD3D12Enc->m_currentEncodeConfig.m_encoderGOPConfigDesc.m_AV1SequenceStructure,
         }
      }
      D3D12_VIDEO_ENCODER_MOTION_ESTIMATION_PRECISION_MODE
   d3d12_video_encoder_convert_av1_motion_configuration(struct d3d12_video_encoder *pD3D12Enc,
         {
         }
      bool
   d3d12_video_encoder_compare_tile_config_av1(
      D3D12_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE currentTilesMode,
   D3D12_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE requestedTilesMode,
   D3D12_VIDEO_ENCODER_AV1_PICTURE_CONTROL_SUBREGIONS_LAYOUT_DATA_TILES currentTilePartition,
      {
      if (currentTilesMode != requestedTilesMode)
            if (memcmp(&currentTilePartition,
            &requestedTilePartition,
            }
      ///
   /// Tries to configurate the encoder using the requested tile configuration
   ///
   bool
   d3d12_video_encoder_negotiate_current_av1_tiles_configuration(struct d3d12_video_encoder *pD3D12Enc,
         {
      D3D12_VIDEO_ENCODER_AV1_PICTURE_CONTROL_SUBREGIONS_LAYOUT_DATA_TILES tilePartition = {};
   tilePartition.RowCount = pAV1Pic->tile_rows;
   tilePartition.ColCount = pAV1Pic->tile_cols;
            // VA-API interface has 63 entries for tile cols/rows. When 64 requested,
            // Copy the tile col sizes (up to 63 defined in VA-API interface array sizes)
   size_t accum_cols_sb = 0;
   uint8_t src_cols_count = MIN2(63, pAV1Pic->tile_cols);
   for (uint8_t i = 0; i < src_cols_count; i++) {
      tilePartition.ColWidths[i] = pAV1Pic->width_in_sbs_minus_1[i] + 1;
               // If there are 64 cols, calculate the last one manually as the difference
   // between frame width in sb minus the accumulated tiles sb sizes
   if (pAV1Pic->tile_cols == 64)
            // Copy the tile row sizes (up to 63 defined in VA-API interface array sizes)
   size_t accum_rows_sb = 0;
   uint8_t src_rows_count = MIN2(63, pAV1Pic->tile_rows);
   for (uint8_t i = 0; i < src_rows_count; i++) {
      tilePartition.RowHeights[i] = pAV1Pic->height_in_sbs_minus_1[i] + 1;
               // If there are 64 rows, calculate the last one manually as the difference
   // between frame height in sb minus the accumulated tiles sb sizes
   if (pAV1Pic->tile_rows == 64)
            // Iterate the tiles and see if they're uniformly partitioned to decide
   // which D3D12 tile mode to use
   // Ignore the last row and last col width
   bool tilesUniform = !D3D12_VIDEO_FORCE_TILE_MODE && util_is_power_of_two_or_zero(tilePartition.RowCount) &&
         // Iterate again now that the 63/64 edge case has been handled above.
   for (uint8_t i = 1; tilesUniform && (i < tilePartition.RowCount - 1) /* Ignore last row */; i++)
            for (uint8_t i = 1; tilesUniform && (i < tilePartition.ColCount - 1) /* Ignore last col */; i++)
            D3D12_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE requestedTilesMode =
      tilesUniform ? D3D12_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE_UNIFORM_GRID_PARTITION :
         assert(pAV1Pic->num_tile_groups <= 128);   // ARRAY_SIZE(TilesGroups)
   pD3D12Enc->m_currentEncodeConfig.m_encoderSliceConfigDesc.m_TilesConfig_AV1.TilesGroupsCount =
         for (uint8_t i = 0; i < pAV1Pic->num_tile_groups; i++) {
      pD3D12Enc->m_currentEncodeConfig.m_encoderSliceConfigDesc.m_TilesConfig_AV1.TilesGroups[i].tg_start =
         pD3D12Enc->m_currentEncodeConfig.m_encoderSliceConfigDesc.m_TilesConfig_AV1.TilesGroups[i].tg_end =
               if (!d3d12_video_encoder_compare_tile_config_av1(
         pD3D12Enc->m_currentEncodeConfig.m_encoderSliceConfigMode,
   requestedTilesMode,
   pD3D12Enc->m_currentEncodeConfig.m_encoderSliceConfigDesc.m_TilesConfig_AV1.TilesPartition,
                  // Update the encoder state with the tile config
   pD3D12Enc->m_currentEncodeConfig.m_encoderSliceConfigMode = requestedTilesMode;
            D3D12_FEATURE_DATA_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_CONFIG capDataTilesSupport = {};
   capDataTilesSupport.NodeIndex = pD3D12Enc->m_NodeIndex;
   capDataTilesSupport.Codec = D3D12_VIDEO_ENCODER_CODEC_AV1;
   capDataTilesSupport.Profile.DataSize = sizeof(pD3D12Enc->m_currentEncodeConfig.m_encoderProfileDesc.m_AV1Profile);
   capDataTilesSupport.Profile.pAV1Profile = &pD3D12Enc->m_currentEncodeConfig.m_encoderProfileDesc.m_AV1Profile;
   capDataTilesSupport.Level.DataSize = sizeof(pD3D12Enc->m_currentEncodeConfig.m_encoderLevelDesc.m_AV1LevelSetting);
   capDataTilesSupport.Level.pAV1LevelSetting = &pD3D12Enc->m_currentEncodeConfig.m_encoderLevelDesc.m_AV1LevelSetting;
   capDataTilesSupport.FrameResolution.Width = pAV1Pic->frame_width;
   capDataTilesSupport.FrameResolution.Height = pAV1Pic->frame_height;
   capDataTilesSupport.SubregionMode = requestedTilesMode;
   capDataTilesSupport.CodecSupport.DataSize =
         capDataTilesSupport.CodecSupport.pAV1Support =
         pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1TileCaps.Use128SuperBlocks = false;
   // return units in 64x64 default size
   pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1TileCaps.TilesConfiguration =
         HRESULT hr =
      pD3D12Enc->m_spD3D12VideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_CONFIG,
            if (FAILED(hr) || !capDataTilesSupport.IsSupported) {
      debug_printf("D3D12_FEATURE_VIDEO_ENCODER_SUBREGION_TILES_SUPPORT HR (0x%x) error or IsSupported: (%d).\n",
                              }
      D3D12_VIDEO_ENCODER_AV1_CODEC_CONFIGURATION
   d3d12_video_encoder_convert_av1_codec_configuration(struct d3d12_video_encoder *pD3D12Enc,
               {
      is_supported = true;
   D3D12_VIDEO_ENCODER_AV1_CODEC_CONFIGURATION config = {
      // D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAGS FeatureFlags;
   D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_NONE,
   // UINT OrderHintBitsMinus1;
               //
   // Query AV1 caps and store in m_currentEncodeCapabilities
            D3D12_FEATURE_DATA_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT capCodecConfigData = {};
   capCodecConfigData.NodeIndex = pD3D12Enc->m_NodeIndex;
   capCodecConfigData.Codec = D3D12_VIDEO_ENCODER_CODEC_AV1;
   D3D12_VIDEO_ENCODER_AV1_PROFILE prof =
         capCodecConfigData.Profile.pAV1Profile = &prof;
   capCodecConfigData.Profile.DataSize = sizeof(prof);
   capCodecConfigData.CodecSupportLimits.pAV1Support =
         capCodecConfigData.CodecSupportLimits.DataSize =
            if (FAILED(
         pD3D12Enc->m_spD3D12VideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT,
            !capCodecConfigData.IsSupported) {
   debug_printf("D3D12_FEATURE_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT arguments not supported.\n");
   is_supported = false;
                        if (pAV1Pic->seq.seq_bits.use_128x128_superblock)
         if (pAV1Pic->seq.seq_bits.enable_filter_intra)
         if (pAV1Pic->seq.seq_bits.enable_intra_edge_filter)
         if (pAV1Pic->seq.seq_bits.enable_interintra_compound)
         if (pAV1Pic->seq.seq_bits.enable_masked_compound)
         if (pAV1Pic->seq.seq_bits.enable_warped_motion)
         if (pAV1Pic->seq.seq_bits.enable_dual_filter)
         if (pAV1Pic->seq.seq_bits.enable_order_hint)
         if (pAV1Pic->seq.seq_bits.enable_jnt_comp)
         if (pAV1Pic->seq.seq_bits.enable_ref_frame_mvs)
         if (pAV1Pic->seq.seq_bits.enable_superres)
         if (pAV1Pic->seq.seq_bits.enable_cdef)
         if (pAV1Pic->seq.seq_bits.enable_restoration)
                     if ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.SupportedFeatureFlags &
      D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_FORCED_INTEGER_MOTION_VECTORS) != 0)   // seq_force_integer_mv
         if ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.SupportedFeatureFlags &
      D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_PALETTE_ENCODING) != 0)
         if ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.SupportedFeatureFlags &
      D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_INTRA_BLOCK_COPY) != 0)
         if ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.SupportedFeatureFlags &
      D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_LOOP_FILTER_DELTAS) != 0)
         if ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.SupportedFeatureFlags &
      D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_QUANTIZATION_DELTAS) != 0)
         if ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.SupportedFeatureFlags &
      D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_QUANTIZATION_MATRIX) != 0)
         if ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.SupportedFeatureFlags &
      D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_REDUCED_TX_SET) != 0)
         if ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.SupportedFeatureFlags &
      D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_MOTION_MODE_SWITCHABLE) != 0)
         if ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.SupportedFeatureFlags &
      D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_ALLOW_HIGH_PRECISION_MV) != 0)
         //
   // Add any missing mandatory/required features we didn't enable before
   //
   if ((config.FeatureFlags &
      pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.RequiredFeatureFlags) !=
   pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.RequiredFeatureFlags) {
   debug_printf(
                  if (((config.FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_128x128_SUPERBLOCK) == 0) &&
      ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.RequiredFeatureFlags &
         pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.RequiredNotRequestedFeatureFlags |=
         debug_printf(
      "[d3d12_video_encoder_convert_av1_codec_configuration] Adding "
   "D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_128x128_SUPERBLOCK required by caps but not selected already in "
            if (((config.FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_FILTER_INTRA) == 0) &&
      ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.RequiredFeatureFlags &
         pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.RequiredNotRequestedFeatureFlags |=
         debug_printf(
      "[d3d12_video_encoder_convert_av1_codec_configuration] "
   "D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_FILTER_INTRA Adding required by caps but not selected already in "
            if (((config.FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_INTRA_EDGE_FILTER) == 0) &&
      ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.RequiredFeatureFlags &
         pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.RequiredNotRequestedFeatureFlags |=
         debug_printf(
      "[d3d12_video_encoder_convert_av1_codec_configuration] Adding "
   "D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_INTRA_EDGE_FILTER required by caps but not selected already in "
            if (((config.FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_INTERINTRA_COMPOUND) == 0) &&
      ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.RequiredFeatureFlags &
         pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.RequiredNotRequestedFeatureFlags |=
         debug_printf(
      "[d3d12_video_encoder_convert_av1_codec_configuration] Adding "
   "D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_INTERINTRA_COMPOUND required by caps but not selected already in "
            if (((config.FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_MASKED_COMPOUND) == 0) &&
      ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.RequiredFeatureFlags &
         pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.RequiredNotRequestedFeatureFlags |=
         debug_printf(
      "[d3d12_video_encoder_convert_av1_codec_configuration] Adding "
   "D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_MASKED_COMPOUND required by caps but not selected already in "
            if (((config.FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_WARPED_MOTION) == 0) &&
      ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.RequiredFeatureFlags &
         pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.RequiredNotRequestedFeatureFlags |=
         debug_printf(
      "[d3d12_video_encoder_convert_av1_codec_configuration] "
   "D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_WARPED_MOTION Adding required by caps but not selected already in "
            if (((config.FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_DUAL_FILTER) == 0) &&
      ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.RequiredFeatureFlags &
         pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.RequiredNotRequestedFeatureFlags |=
         debug_printf("[d3d12_video_encoder_convert_av1_codec_configuration] == Adding required by caps but not "
                     if (((config.FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_JNT_COMP) == 0) &&
      ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.RequiredFeatureFlags &
         pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.RequiredNotRequestedFeatureFlags |=
         debug_printf("[d3d12_video_encoder_convert_av1_codec_configuration] 0 Adding required by caps but not "
                     if (((config.FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_FORCED_INTEGER_MOTION_VECTORS) == 0) &&
      ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.RequiredFeatureFlags &
         pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.RequiredNotRequestedFeatureFlags |=
         debug_printf(
      "[d3d12_video_encoder_convert_av1_codec_configuration] Adding required by "
   "D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_FORCED_INTEGER_MOTION_VECTORS caps but not selected already in "
            if (((config.FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_SUPER_RESOLUTION) == 0) &&
      ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.RequiredFeatureFlags &
         pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.RequiredNotRequestedFeatureFlags |=
         debug_printf(
      "[d3d12_video_encoder_convert_av1_codec_configuration] Adding "
   "D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_SUPER_RESOLUTION required by caps but not selected already in "
            if (((config.FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_LOOP_RESTORATION_FILTER) == 0) &&
      ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.RequiredFeatureFlags &
         pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.RequiredNotRequestedFeatureFlags |=
         debug_printf(
      "[d3d12_video_encoder_convert_av1_codec_configuration] Adding "
   "D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_LOOP_RESTORATION_FILTER required by caps but not selected already in "
            if (((config.FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_PALETTE_ENCODING) == 0) &&
      ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.RequiredFeatureFlags &
         pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.RequiredNotRequestedFeatureFlags |=
         debug_printf(
      "[d3d12_video_encoder_convert_av1_codec_configuration] Adding "
   "D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_PALETTE_ENCODING required by caps but not selected already in "
            if (((config.FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_CDEF_FILTERING) == 0) &&
      ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.RequiredFeatureFlags &
         pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.RequiredNotRequestedFeatureFlags |=
         debug_printf(
      "[d3d12_video_encoder_convert_av1_codec_configuration] "
   "D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_CDEF_FILTERING Adding required by caps but not selected already in "
            if (((config.FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_INTRA_BLOCK_COPY) == 0) &&
      ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.RequiredFeatureFlags &
         pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.RequiredNotRequestedFeatureFlags |=
         debug_printf(
      "[d3d12_video_encoder_convert_av1_codec_configuration] Adding "
   "D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_INTRA_BLOCK_COPY required by caps but not selected already in "
            if (((config.FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_FRAME_REFERENCE_MOTION_VECTORS) == 0) &&
      ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.RequiredFeatureFlags &
         pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.RequiredNotRequestedFeatureFlags |=
         debug_printf(
      "[d3d12_video_encoder_convert_av1_codec_configuration] Adding required by "
   "D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_FRAME_REFERENCE_MOTION_VECTORS caps but not selected already in "
            if (((config.FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_ORDER_HINT_TOOLS) == 0) &&
      ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.RequiredFeatureFlags &
         pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.RequiredNotRequestedFeatureFlags |=
         debug_printf(
      "[d3d12_video_encoder_convert_av1_codec_configuration] Adding "
   "D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_ORDER_HINT_TOOLS required by caps but not selected already in "
            if (((config.FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_AUTO_SEGMENTATION) == 0) &&
      ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.RequiredFeatureFlags &
         pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.RequiredNotRequestedFeatureFlags |=
         debug_printf(
      "[d3d12_video_encoder_convert_av1_codec_configuration] Adding "
   "D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_AUTO_SEGMENTATION required by caps but not selected already in "
            if (((config.FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_CUSTOM_SEGMENTATION) == 0) &&
      ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.RequiredFeatureFlags &
         pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.RequiredNotRequestedFeatureFlags |=
         debug_printf(
      "[d3d12_video_encoder_convert_av1_codec_configuration] Adding "
   "D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_CUSTOM_SEGMENTATION required by caps but not selected already in "
            if (((config.FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_LOOP_FILTER_DELTAS) == 0) &&
      ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.RequiredFeatureFlags &
         pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.RequiredNotRequestedFeatureFlags |=
         debug_printf(
      "[d3d12_video_encoder_convert_av1_codec_configuration] Adding "
   "D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_LOOP_FILTER_DELTAS required by caps but not selected already in "
            if (((config.FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_QUANTIZATION_DELTAS) == 0) &&
      ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.RequiredFeatureFlags &
         pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.RequiredNotRequestedFeatureFlags |=
         debug_printf(
      "[d3d12_video_encoder_convert_av1_codec_configuration] Adding "
   "D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_QUANTIZATION_DELTAS required by caps but not selected already in "
            if (((config.FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_QUANTIZATION_MATRIX) == 0) &&
      ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.RequiredFeatureFlags &
         pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.RequiredNotRequestedFeatureFlags |=
         debug_printf(
      "[d3d12_video_encoder_convert_av1_codec_configuration] Adding "
   "D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_QUANTIZATION_MATRIX required by caps but not selected already in "
            if (((config.FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_REDUCED_TX_SET) == 0) &&
      ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.RequiredFeatureFlags &
         pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.RequiredNotRequestedFeatureFlags |=
         debug_printf(
      "[d3d12_video_encoder_convert_av1_codec_configuration] "
   "D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_REDUCED_TX_SET Adding required by caps but not selected already in "
            if (((config.FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_MOTION_MODE_SWITCHABLE) == 0) &&
      ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.RequiredFeatureFlags &
         pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.RequiredNotRequestedFeatureFlags |=
         debug_printf(
      "[d3d12_video_encoder_convert_av1_codec_configuration] Adding "
   "D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_MOTION_MODE_SWITCHABLE required by caps but not selected already in "
            if (((config.FeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_ALLOW_HIGH_PRECISION_MV) == 0) &&
      ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.RequiredFeatureFlags &
         pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.RequiredNotRequestedFeatureFlags |=
         debug_printf(
      "[d3d12_video_encoder_convert_av1_codec_configuration] Adding "
   "D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_ALLOW_HIGH_PRECISION_MV required by caps but not selected already in "
            // Enable all required flags previously not selected
   config.FeatureFlags |=
               // Check config.FeatureFlags against SupportedFeatureFlags and assign is_supported
   if ((config.FeatureFlags &
      pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.SupportedFeatureFlags) !=
   config.FeatureFlags) {
   debug_printf(
      "[d3d12_video_encoder_convert_av1_codec_configuration] AV1 Configuration flags requested 0x%x not supported "
   "by "
   "m_AV1CodecCaps.SupportedFeatureFlags 0x%x\n",
                              }
      bool
   d3d12_video_encoder_update_current_encoder_config_state_av1(struct d3d12_video_encoder *pD3D12Enc,
               {
               // Reset reconfig dirty flags
   pD3D12Enc->m_currentEncodeConfig.m_ConfigDirtyFlags = d3d12_video_encoder_config_dirty_flag_none;
   // Reset sequence changes flags
            // Set codec
   if (pD3D12Enc->m_currentEncodeConfig.m_encoderCodecDesc != D3D12_VIDEO_ENCODER_CODEC_AV1) {
         }
            // Set input format
   DXGI_FORMAT targetFmt = d3d12_get_format(srcTexture->buffer_format);
   if (pD3D12Enc->m_currentEncodeConfig.m_encodeFormatInfo.Format != targetFmt) {
                  pD3D12Enc->m_currentEncodeConfig.m_encodeFormatInfo = {};
   pD3D12Enc->m_currentEncodeConfig.m_encodeFormatInfo.Format = targetFmt;
   HRESULT hr =
      pD3D12Enc->m_pD3D12Screen->dev->CheckFeatureSupport(D3D12_FEATURE_FORMAT_INFO,
            if (FAILED(hr)) {
      debug_printf("CheckFeatureSupport failed with HR 0x%x\n", hr);
               // Set resolution (ie. frame_size)
   if ((pD3D12Enc->m_currentEncodeConfig.m_currentResolution.Width != srcTexture->width) ||
      (pD3D12Enc->m_currentEncodeConfig.m_currentResolution.Height != srcTexture->height)) {
      }
   pD3D12Enc->m_currentEncodeConfig.m_currentResolution.Width = srcTexture->width;
            // render_size
   pD3D12Enc->m_currentEncodeConfig.m_FrameCroppingCodecConfig.right = av1Pic->frame_width;
            // Set profile
   auto targetProfile = d3d12_video_encoder_convert_profile_to_d3d12_enc_profile_av1(pD3D12Enc->base.profile);
   if (pD3D12Enc->m_currentEncodeConfig.m_encoderProfileDesc.m_AV1Profile != targetProfile) {
         }
            // Set level and tier
   D3D12_VIDEO_ENCODER_AV1_LEVELS targetLevel = {};
   D3D12_VIDEO_ENCODER_AV1_TIER targetTier = {};
   d3d12_video_encoder_convert_spec_to_d3d12_level_av1(av1Pic->seq.level, targetLevel);
            if ((pD3D12Enc->m_currentEncodeConfig.m_encoderLevelDesc.m_AV1LevelSetting.Level != targetLevel) ||
      (pD3D12Enc->m_currentEncodeConfig.m_encoderLevelDesc.m_AV1LevelSetting.Tier != targetTier)) {
      }
   pD3D12Enc->m_currentEncodeConfig.m_encoderLevelDesc.m_AV1LevelSetting.Tier = targetTier;
            //
   // Validate caps support returned values against current settings
   //
   if (pD3D12Enc->m_currentEncodeConfig.m_encoderProfileDesc.m_AV1Profile !=
      pD3D12Enc->m_currentEncodeCapabilities.m_encoderSuggestedProfileDesc.m_AV1Profile) {
   debug_printf("[d3d12_video_encoder_av1] Warning: Requested D3D12_VIDEO_ENCODER_PROFILE_AV1 by upper layer: %d "
                           if ((pD3D12Enc->m_currentEncodeConfig.m_encoderLevelDesc.m_AV1LevelSetting.Level !=
      pD3D12Enc->m_currentEncodeCapabilities.m_encoderLevelSuggestedDesc.m_AV1LevelSetting.Level) ||
   (pD3D12Enc->m_currentEncodeConfig.m_encoderLevelDesc.m_AV1LevelSetting.Tier !=
   pD3D12Enc->m_currentEncodeCapabilities.m_encoderLevelSuggestedDesc.m_AV1LevelSetting.Tier)) {
   debug_printf(
      "[d3d12_video_encoder_av1] Warning: Requested D3D12_VIDEO_ENCODER_LEVELS_AV1 by upper layer: level %d tier %d "
   "mismatches UMD suggested D3D12_VIDEO_ENCODER_LEVELS_AV1: level %d tier %d\n",
   pD3D12Enc->m_currentEncodeConfig.m_encoderLevelDesc.m_AV1LevelSetting.Level,
   pD3D12Enc->m_currentEncodeConfig.m_encoderLevelDesc.m_AV1LevelSetting.Tier,
   pD3D12Enc->m_currentEncodeCapabilities.m_encoderLevelSuggestedDesc.m_AV1LevelSetting.Level,
            // Set codec config
   bool is_supported = false;
   auto targetCodecConfig = d3d12_video_encoder_convert_av1_codec_configuration(pD3D12Enc, av1Pic, is_supported);
   if (!is_supported) {
                  if (memcmp(&pD3D12Enc->m_currentEncodeConfig.m_encoderCodecSpecificConfigDesc.m_AV1Config,
            &targetCodecConfig,
      }
            // Set rate control
            // Set tiles config
   if (!d3d12_video_encoder_negotiate_current_av1_tiles_configuration(pD3D12Enc, av1Pic)) {
      debug_printf("d3d12_video_encoder_negotiate_current_av1_tiles_configuration failed!\n");
               // Set GOP config
   if (!d3d12_video_encoder_update_av1_gop_configuration(pD3D12Enc, av1Pic)) {
      debug_printf("d3d12_video_encoder_update_av1_gop_configuration failed!\n");
               // m_currentEncodeConfig.m_encoderPicParamsDesc pic params are set in d3d12_video_encoder_reconfigure_encoder_objects
            // Set motion estimation config
   auto targetMotionLimit = d3d12_video_encoder_convert_av1_motion_configuration(pD3D12Enc, av1Pic);
   if (pD3D12Enc->m_currentEncodeConfig.m_encoderMotionPrecisionLimit != targetMotionLimit) {
      pD3D12Enc->m_currentEncodeConfig.m_ConfigDirtyFlags |=
      }
            // Will call for d3d12 driver support based on the initial requested (non codec specific) features, then
   // try to fallback if any of them is not supported and return the negotiated d3d12 settings
   D3D12_FEATURE_DATA_VIDEO_ENCODER_SUPPORT1 capEncoderSupportData1 = {};
   if (!d3d12_video_encoder_negotiate_requested_features_and_d3d12_driver_caps(pD3D12Enc, capEncoderSupportData1)) {
      debug_printf("[d3d12_video_encoder_av1] After negotiating caps, D3D12_FEATURE_VIDEO_ENCODER_SUPPORT1 "
               "arguments are not supported - "
   "ValidationFlags: 0x%x - SupportFlags: 0x%x\n",
               pD3D12Enc->m_currentEncodeCapabilities.m_MaxSlicesInOutput = (av1Pic->tile_cols * av1Pic->tile_rows);
   if (pD3D12Enc->m_currentEncodeCapabilities.m_MaxSlicesInOutput >
      pD3D12Enc->m_currentEncodeCapabilities.m_currentResolutionSupportCaps.MaxSubregionsNumber) {
   debug_printf("[d3d12_video_encoder_av1] Desired number of subregions is not supported (higher than max "
                        }
      /*
   * Called at begin_frame record time
   */
   void
   d3d12_video_encoder_update_current_frame_pic_params_info_av1(struct d3d12_video_encoder *pD3D12Enc,
                           {
               // Output param bUsedAsReference
            // D3D12_VIDEO_ENCODER_AV1_PICTURE_CONTROL_FLAGS Flags;
            if (pAV1Pic->error_resilient_mode)
            if (pAV1Pic->disable_cdf_update)
            if (pAV1Pic->palette_mode_enable)
            // Override if required feature
   if ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.RequiredFeatureFlags &
      D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_PALETTE_ENCODING) != 0) {
   debug_printf("[d3d12_video_encoder_update_current_frame_pic_params_info_av1] Overriding required feature "
                     if (pAV1Pic->skip_mode_present)
            if ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.RequiredFeatureFlags &
      D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_SKIP_MODE_PRESENT) != 0) {
   debug_printf("[d3d12_video_encoder_update_current_frame_pic_params_info_av1] Overriding required feature "
                     if (pAV1Pic->use_ref_frame_mvs)
            // Override if required feature
   if ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.RequiredFeatureFlags &
      D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_FRAME_REFERENCE_MOTION_VECTORS) != 0) {
   debug_printf("[d3d12_video_encoder_update_current_frame_pic_params_info_av1] Overriding required feature "
                     // No pipe flag for force_integer_mv (D3D12_VIDEO_ENCODER_AV1_PICTURE_CONTROL_FLAG_FORCE_INTEGER_MOTION_VECTORS)
   // choose default based on required/supported underlying codec flags
   if ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.RequiredFeatureFlags &
      D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_FORCED_INTEGER_MOTION_VECTORS) != 0) {
   debug_printf("[d3d12_video_encoder_update_current_frame_pic_params_info_av1] Overriding required feature "
                     if (pAV1Pic->allow_intrabc)
            // Override if required feature
   if ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.RequiredFeatureFlags &
      D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_INTRA_BLOCK_COPY) != 0) {
   debug_printf("[d3d12_video_encoder_update_current_frame_pic_params_info_av1] Overriding required feature "
                     if (pAV1Pic->use_superres)
            if (pAV1Pic->disable_frame_end_update_cdf)
            // No pipe flag for D3D12_VIDEO_ENCODER_AV1_PICTURE_CONTROL_FLAG_ENABLE_FRAME_SEGMENTATION_AUTO
   // choose default based on required/supported underlying codec flags
   if ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.RequiredFeatureFlags &
      D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_AUTO_SEGMENTATION) != 0) {
   debug_printf("[d3d12_video_encoder_update_current_frame_pic_params_info_av1] Overriding required feature "
                     // No pipe flag for D3D12_VIDEO_ENCODER_AV1_PICTURE_CONTROL_FLAG_ENABLE_FRAME_SEGMENTATION_CUSTOM
   // choose default based on required/supported underlying codec flags
   if ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.RequiredFeatureFlags &
      D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_CUSTOM_SEGMENTATION) != 0) {
   debug_printf("[d3d12_video_encoder_update_current_frame_pic_params_info_av1] Overriding required feature "
         picParams.pAV1PicData->Flags |= D3D12_VIDEO_ENCODER_AV1_PICTURE_CONTROL_FLAG_ENABLE_FRAME_SEGMENTATION_CUSTOM;
               // No pipe flag for allow_warped_motion (D3D12_VIDEO_ENCODER_AV1_PICTURE_CONTROL_FLAG_ENABLE_WARPED_MOTION)
   // choose default based on required/supported underlying codec flags
   if ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.RequiredFeatureFlags &
      D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_WARPED_MOTION) != 0) {
   debug_printf("[d3d12_video_encoder_update_current_frame_pic_params_info_av1] Overriding required feature "
                     // Only enable if supported (there is no PIPE/VA cap flag for reduced_tx_set)
   if ((pAV1Pic->reduced_tx_set) &&
      (pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.SupportedFeatureFlags &
   D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_REDUCED_TX_SET) != 0) {
               // Override if required feature
   if ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.RequiredFeatureFlags &
      D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_REDUCED_TX_SET) != 0) {
   debug_printf("[d3d12_video_encoder_update_current_frame_pic_params_info_av1] Overriding required feature "
                     // Only enable if supported
   if ((pAV1Pic->allow_high_precision_mv) &&
      (pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.SupportedFeatureFlags &
   D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_ALLOW_HIGH_PRECISION_MV) != 0) {
               // Override if required feature
   if ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.RequiredFeatureFlags &
      D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_ALLOW_HIGH_PRECISION_MV) != 0) {
   debug_printf("[d3d12_video_encoder_update_current_frame_pic_params_info_av1] Overriding required feature "
                     // No pipe flag for is_motion_mode_switchable (D3D12_VIDEO_ENCODER_AV1_PICTURE_CONTROL_FLAG_MOTION_MODE_SWITCHABLE)
   // choose default based on required/supported underlying codec flags
   if ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.RequiredFeatureFlags &
      D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_MOTION_MODE_SWITCHABLE) != 0) {
   debug_printf("[d3d12_video_encoder_update_current_frame_pic_params_info_av1] Overriding required feature "
                     // D3D12_VIDEO_ENCODER_AV1_FRAME_TYPE FrameType;
   // AV1 spec matches w/D3D12 enum definition
            if (picParams.pAV1PicData->FrameType == D3D12_VIDEO_ENCODER_AV1_FRAME_TYPE_KEY_FRAME)
         if (picParams.pAV1PicData->FrameType == D3D12_VIDEO_ENCODER_AV1_FRAME_TYPE_INTER_FRAME)
         if (picParams.pAV1PicData->FrameType == D3D12_VIDEO_ENCODER_AV1_FRAME_TYPE_INTRA_ONLY_FRAME)
         if (picParams.pAV1PicData->FrameType == D3D12_VIDEO_ENCODER_AV1_FRAME_TYPE_SWITCH_FRAME)
            // D3D12_VIDEO_ENCODER_AV1_COMP_PREDICTION_TYPE CompoundPredictionType;
   picParams.pAV1PicData->CompoundPredictionType = (pAV1Pic->compound_reference_mode == 0) ?
                  // D3D12_VIDEO_ENCODER_AV1_INTERPOLATION_FILTERS InterpolationFilter;
   // AV1 spec matches w/D3D12 enum definition
   picParams.pAV1PicData->InterpolationFilter =
            // Workaround for apps sending interpolation_filter values not supported even when reporting
   // them in pipe_av1_enc_cap_features_ext1.interpolation_filter. If D3D12 driver doesn't support
   // requested InterpolationFilter, fallback to the first supported by D3D12 driver
   if ( (pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.SupportedInterpolationFilters &
      (1 << picParams.pAV1PicData->InterpolationFilter)) == 0 ) { /* See definition of D3D12_VIDEO_ENCODER_AV1_INTERPOLATION_FILTERS_FLAGS */
   debug_printf("[d3d12_video_encoder_update_current_frame_pic_params_info_av1] Requested interpolation_filter"
               for(uint8_t i = D3D12_VIDEO_ENCODER_AV1_INTERPOLATION_FILTERS_EIGHTTAP; i <= D3D12_VIDEO_ENCODER_AV1_INTERPOLATION_FILTERS_SWITCHABLE; i++) {
      if ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.SupportedInterpolationFilters &
      (1 << i)) /* See definition of D3D12_VIDEO_ENCODER_AV1_INTERPOLATION_FILTERS_FLAGS */ != 0) {
   picParams.pAV1PicData->InterpolationFilter = static_cast<D3D12_VIDEO_ENCODER_AV1_INTERPOLATION_FILTERS>(i);
                     // D3D12_VIDEO_ENCODER_AV1_RESTORATION_CONFIG FrameRestorationConfig;
            picParams.pAV1PicData->FrameRestorationConfig.FrameRestorationType[0] =
         picParams.pAV1PicData->FrameRestorationConfig.FrameRestorationType[1] =
         picParams.pAV1PicData->FrameRestorationConfig.FrameRestorationType[2] =
            if (picParams.pAV1PicData->FrameRestorationConfig.FrameRestorationType[0] !=
      D3D12_VIDEO_ENCODER_AV1_RESTORATION_TYPE_DISABLED) {
   picParams.pAV1PicData->FrameRestorationConfig.LoopRestorationPixelSize[0] =
               if (picParams.pAV1PicData->FrameRestorationConfig.FrameRestorationType[1] !=
      D3D12_VIDEO_ENCODER_AV1_RESTORATION_TYPE_DISABLED) {
   picParams.pAV1PicData->FrameRestorationConfig.LoopRestorationPixelSize[1] =
      d3d12_video_encoder_looprestorationsize_uint_to_d3d12_av1(
            if (picParams.pAV1PicData->FrameRestorationConfig.FrameRestorationType[2] !=
      D3D12_VIDEO_ENCODER_AV1_RESTORATION_TYPE_DISABLED) {
   picParams.pAV1PicData->FrameRestorationConfig.LoopRestorationPixelSize[2] =
      d3d12_video_encoder_looprestorationsize_uint_to_d3d12_av1(
            // D3D12_VIDEO_ENCODER_AV1_TX_MODE TxMode;
   // AV1 spec matches w/D3D12 enum definition
            // Workaround for mismatch between VAAPI/D3D12 and TxMode support for all/some frame types
   // If D3D12 driver doesn't support requested TxMode, fallback to the first supported by D3D12
   // driver for the requested frame type
   if ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.SupportedTxModes[picParams.pAV1PicData->FrameType] &
      (1 << picParams.pAV1PicData->TxMode)) == 0) /* See definition of D3D12_VIDEO_ENCODER_AV1_TX_MODE_FLAGS */ {
   debug_printf("[d3d12_video_encoder_update_current_frame_pic_params_info_av1] Requested tx_mode not supported"
         for(uint8_t i = D3D12_VIDEO_ENCODER_AV1_TX_MODE_ONLY4x4; i <= D3D12_VIDEO_ENCODER_AV1_TX_MODE_SELECT; i++) {
      if ((pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1CodecCaps.SupportedTxModes[picParams.pAV1PicData->FrameType] &
      (1 << i)) /* See definition of D3D12_VIDEO_ENCODER_AV1_TX_MODE_FLAGS */ != 0) {
   picParams.pAV1PicData->TxMode = static_cast<D3D12_VIDEO_ENCODER_AV1_TX_MODE>(i);
                     // UINT SuperResDenominator;
            // UINT OrderHint;
            // UINT PictureIndex - Substract the last_key_frame_num to make it modulo KEY frame
            // UINT TemporalLayerIndexPlus1;
   assert(pAV1Pic->temporal_id == pAV1Pic->tg_obu_header.temporal_id);
            // UINT SpatialLayerIndexPlus1;
            //
   // Reference Pictures
   //
   {
      for (uint8_t i = 0; i < ARRAY_SIZE(picParams.pAV1PicData->ReferenceIndices); i++) {
                  bool FrameIsIntra = (picParams.pAV1PicData->FrameType == D3D12_VIDEO_ENCODER_AV1_FRAME_TYPE_INTRA_ONLY_FRAME ||
         if (FrameIsIntra)
         else
         debug_printf("App requested primary_ref_frame: %" PRIu32 "\n", pAV1Pic->primary_ref_frame);
               // D3D12_VIDEO_ENCODER_CODEC_AV1_LOOP_FILTER_CONFIG LoopFilter;
   picParams.pAV1PicData->LoopFilter.LoopFilterLevel[0] = pAV1Pic->loop_filter.filter_level[0];
   picParams.pAV1PicData->LoopFilter.LoopFilterLevel[1] = pAV1Pic->loop_filter.filter_level[1];
   picParams.pAV1PicData->LoopFilter.LoopFilterLevelU = pAV1Pic->loop_filter.filter_level_u;
   picParams.pAV1PicData->LoopFilter.LoopFilterLevelV = pAV1Pic->loop_filter.filter_level_v;
   picParams.pAV1PicData->LoopFilter.LoopFilterSharpnessLevel = pAV1Pic->loop_filter.sharpness_level;
   picParams.pAV1PicData->LoopFilter.LoopFilterDeltaEnabled = pAV1Pic->loop_filter.mode_ref_delta_enabled;
   picParams.pAV1PicData->LoopFilter.UpdateRefDelta = pAV1Pic->loop_filter.mode_ref_delta_update;
   if (picParams.pAV1PicData->LoopFilter.UpdateRefDelta) {
      for (uint8_t i = 0; i < ARRAY_SIZE(picParams.pAV1PicData->LoopFilter.RefDeltas); i++) {
            }
   picParams.pAV1PicData->LoopFilter.UpdateModeDelta = pAV1Pic->loop_filter.mode_ref_delta_update;
   if (picParams.pAV1PicData->LoopFilter.UpdateModeDelta) {
      for (uint8_t i = 0; i < ARRAY_SIZE(picParams.pAV1PicData->LoopFilter.ModeDeltas); i++) {
                     // D3D12_VIDEO_ENCODER_CODEC_AV1_LOOP_FILTER_DELTA_CONFIG LoopFilterDelta;
   picParams.pAV1PicData->LoopFilterDelta.DeltaLFMulti = pAV1Pic->loop_filter.delta_lf_multi;
   picParams.pAV1PicData->LoopFilterDelta.DeltaLFPresent = pAV1Pic->loop_filter.delta_lf_present;
            // D3D12_VIDEO_ENCODER_CODEC_AV1_QUANTIZATION_CONFIG Quantization;
   picParams.pAV1PicData->Quantization.BaseQIndex = pAV1Pic->quantization.base_qindex;
   picParams.pAV1PicData->Quantization.YDCDeltaQ = pAV1Pic->quantization.y_dc_delta_q;
   picParams.pAV1PicData->Quantization.UDCDeltaQ = pAV1Pic->quantization.u_dc_delta_q;
   picParams.pAV1PicData->Quantization.UACDeltaQ = pAV1Pic->quantization.u_ac_delta_q;
   picParams.pAV1PicData->Quantization.VDCDeltaQ = pAV1Pic->quantization.v_dc_delta_q;
   picParams.pAV1PicData->Quantization.VACDeltaQ = pAV1Pic->quantization.v_ac_delta_q;
   picParams.pAV1PicData->Quantization.UsingQMatrix = pAV1Pic->quantization.using_qmatrix;
   picParams.pAV1PicData->Quantization.QMY = pAV1Pic->quantization.qm_y;
   picParams.pAV1PicData->Quantization.QMU = pAV1Pic->quantization.qm_u;
            // D3D12_VIDEO_ENCODER_CODEC_AV1_QUANTIZATION_DELTA_CONFIG QuantizationDelta;
   picParams.pAV1PicData->QuantizationDelta.DeltaQPresent = pAV1Pic->quantization.delta_q_present;
            // D3D12_VIDEO_ENCODER_AV1_CDEF_CONFIG CDEF;
   picParams.pAV1PicData->CDEF.CdefBits = pAV1Pic->cdef.cdef_bits;
   picParams.pAV1PicData->CDEF.CdefDampingMinus3 = pAV1Pic->cdef.cdef_damping_minus_3;
   for (uint32_t i = 0; i < 8; i++) {
      picParams.pAV1PicData->CDEF.CdefYPriStrength[i] = (pAV1Pic->cdef.cdef_y_strengths[i] >> 2);
   picParams.pAV1PicData->CDEF.CdefYSecStrength[i] = (pAV1Pic->cdef.cdef_y_strengths[i] & 0x03);
   picParams.pAV1PicData->CDEF.CdefUVPriStrength[i] = (pAV1Pic->cdef.cdef_uv_strengths[i] >> 2);
               //
   // Set values for mandatory but not requested features (ie. RequiredFeature flags reported by driver not requested
   // by pipe)
            // For the ones that are more trivial (ie. 128x128 SB) than enabling their flag, and have more parameters to be
            // These are a trivial flag enabling
   //  D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_128x128_SUPERBLOCK
   //  D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_FILTER_INTRA
   //  D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_INTRA_EDGE_FILTER
   //  D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_INTERINTRA_COMPOUND
   //  D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_MASKED_COMPOUND
   //  D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_WARPED_MOTION
   //  D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_DUAL_FILTER
   //  D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_JNT_COMP
   //  D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_FORCED_INTEGER_MOTION_VECTORS
   //  D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_SUPER_RESOLUTION
   //  D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_PALETTE_ENCODING
   //  D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_INTRA_BLOCK_COPY
   //  D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_FRAME_REFERENCE_MOTION_VECTORS
   //  D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_ORDER_HINT_TOOLS
   //  D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_REDUCED_TX_SET
   //  D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_MOTION_MODE_SWITCHABLE
            // If driver requires these, can use post encode values to enforce specific values
   // D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_CDEF_FILTERING
   // D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_AUTO_SEGMENTATION // Only useful with post encode values as driver
   // controls D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_CUSTOM_SEGMENTATION // There are more Segmentation caps in
   // D3D12_VIDEO_ENCODER_AV1_CODEC_CONFIGURATION_SUPPORT for these one too
   // D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_LOOP_FILTER_DELTAS
   // D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_QUANTIZATION_DELTAS
            // If driver requires this one, use driver restoration caps to set default values
   // D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_LOOP_RESTORATION_FILTER
   if (pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps.RequiredNotRequestedFeatureFlags &
      D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_LOOP_RESTORATION_FILTER) {
   debug_printf(
      "[d3d12_video_encoder_update_current_frame_pic_params_info_av1] Adding default params for "
               // Set Y, U, V luma plane restoration params
   for (uint32_t planeIdx = 0; planeIdx < 3; planeIdx++) {
      // Let's see which filters and with which sizes are supported for planeIdx
   bool foundSupportedFilter = false;
   for (uint32_t filterIdx = 0; ((filterIdx < 3) && !foundSupportedFilter); filterIdx++) {
      auto curFilterSupport = pD3D12Enc->m_currentEncodeCapabilities.m_encoderCodecSpecificConfigCaps
         for (uint32_t curFilterSize = D3D12_VIDEO_ENCODER_AV1_RESTORATION_TILESIZE_32x32;
                     // Check if there's support for curFilter on CurPlane and choose the first supported restoration size
   //  If yes, set restoration params for planeIdx
   if (curFilterSupport &
      (1 << (curFilterSize - 1))) { /* Converts D3D12_VIDEO_ENCODER_AV1_RESTORATION_TILESIZE definition
         {
      foundSupportedFilter = true;
                              picParams.pAV1PicData->FrameRestorationConfig.FrameRestorationType[planeIdx] = curFilter;
   picParams.pAV1PicData->FrameRestorationConfig.LoopRestorationPixelSize[planeIdx] =
      static_cast<D3D12_VIDEO_ENCODER_AV1_RESTORATION_TILESIZE>(
                           // Save state snapshot from record time to resolve headers at get_feedback time
   uint64_t current_metadata_slot = (pD3D12Enc->m_fenceValue % D3D12_VIDEO_ENC_METADATA_BUFFERS_COUNT);
   pD3D12Enc->m_spEncodedFrameMetadata[current_metadata_slot].m_associatedEncodeCapabilities =
         pD3D12Enc->m_spEncodedFrameMetadata[current_metadata_slot].m_associatedEncodeConfig =
         pD3D12Enc->m_spEncodedFrameMetadata[current_metadata_slot].m_CodecSpecificData.AV1HeadersInfo.enable_frame_obu =
         pD3D12Enc->m_spEncodedFrameMetadata[current_metadata_slot].m_CodecSpecificData.AV1HeadersInfo.obu_has_size_field =
         // Disabling for now as the libva spec does not allow these but some apps send down anyway. It's possible in the future 
   // the libva spec may be retro-fitted to allow this given existing apps in the wild doing it.
   // pD3D12Enc->m_spEncodedFrameMetadata[current_metadata_slot]
      }
      void
   fill_av1_seq_header(EncodedBitstreamResolvedMetadata &associatedMetadata, av1_seq_header_t *seq_header)
   {
      // Set all zero by default
            seq_header->seq_profile = d3d12_video_encoder_convert_d3d12_profile_to_spec_profile_av1(
         // seq_header->still_picture; // coded in bitstream by default as 0
   // seq_header->reduced_still_picture_header; // coded in bitstream by default as 0
   // seq_header->timing_info_present_flag; // coded in bitstream by default as 0
   // seq_header->decoder_model_info_present_flag; // coded in bitstream by default as 0
   // seq_header->operating_points_cnt_minus_1; // memset default as 0
   // seq_header->operating_point_idc[32]; // memset default as 0
   d3d12_video_encoder_convert_d3d12_to_spec_level_av1(
      associatedMetadata.m_associatedEncodeConfig.m_encoderLevelDesc.m_AV1LevelSetting.Level,
      d3d12_video_encoder_convert_d3d12_to_spec_tier_av1(
      associatedMetadata.m_associatedEncodeConfig.m_encoderLevelDesc.m_AV1LevelSetting.Tier,
      // seq_header->decoder_model_present_for_this_op[32]; // memset default as 0
   // seq_header->initial_display_delay_present_flag; // coded in bitstream by default as 0
   // seq_header->initial_display_delay_minus_1[32];
   // seq_header->initial_display_delay_present_for_this_op[32]
   // seq_header->frame_width_bits; // coded in bitstream by default as 16
            // frame_size (comes from input texture size)
   seq_header->max_frame_width = associatedMetadata.m_associatedEncodeConfig.m_currentResolution.Width;
            // seq_header->frame_id_numbers_present_flag; // coded in bitstream by default as 0
   seq_header->use_128x128_superblock =
      ((associatedMetadata.m_associatedEncodeConfig.m_encoderCodecSpecificConfigDesc.m_AV1Config.FeatureFlags &
   D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_128x128_SUPERBLOCK) != 0) ?
      1 :
   seq_header->enable_filter_intra =
      ((associatedMetadata.m_associatedEncodeConfig.m_encoderCodecSpecificConfigDesc.m_AV1Config.FeatureFlags &
   D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_FILTER_INTRA) != 0) ?
      1 :
   seq_header->enable_intra_edge_filter =
      ((associatedMetadata.m_associatedEncodeConfig.m_encoderCodecSpecificConfigDesc.m_AV1Config.FeatureFlags &
   D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_INTRA_EDGE_FILTER) != 0) ?
      1 :
   seq_header->enable_interintra_compound =
      ((associatedMetadata.m_associatedEncodeConfig.m_encoderCodecSpecificConfigDesc.m_AV1Config.FeatureFlags &
   D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_INTERINTRA_COMPOUND) != 0) ?
      1 :
   seq_header->enable_masked_compound =
      ((associatedMetadata.m_associatedEncodeConfig.m_encoderCodecSpecificConfigDesc.m_AV1Config.FeatureFlags &
   D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_MASKED_COMPOUND) != 0) ?
      1 :
   seq_header->enable_warped_motion =
      ((associatedMetadata.m_associatedEncodeConfig.m_encoderCodecSpecificConfigDesc.m_AV1Config.FeatureFlags &
   D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_WARPED_MOTION) != 0) ?
      1 :
   seq_header->enable_dual_filter =
      ((associatedMetadata.m_associatedEncodeConfig.m_encoderCodecSpecificConfigDesc.m_AV1Config.FeatureFlags &
   D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_DUAL_FILTER) != 0) ?
      1 :
   seq_header->enable_order_hint =
      ((associatedMetadata.m_associatedEncodeConfig.m_encoderCodecSpecificConfigDesc.m_AV1Config.FeatureFlags &
   D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_ORDER_HINT_TOOLS) != 0) ?
      1 :
   seq_header->enable_jnt_comp =
      ((associatedMetadata.m_associatedEncodeConfig.m_encoderCodecSpecificConfigDesc.m_AV1Config.FeatureFlags &
   D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_JNT_COMP) != 0) ?
      1 :
   seq_header->enable_ref_frame_mvs =
      ((associatedMetadata.m_associatedEncodeConfig.m_encoderCodecSpecificConfigDesc.m_AV1Config.FeatureFlags &
   D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_FRAME_REFERENCE_MOTION_VECTORS) != 0) ?
      1 :
   seq_header->seq_choose_screen_content_tools = 0;   // coded in bitstream by default as 0
   seq_header->seq_force_screen_content_tools = 0;    // coded in bitstream by default as 0
   seq_header->seq_choose_integer_mv = 0;             // coded in bitstream by default as 0
   seq_header->seq_force_integer_mv = 0;              // coded in bitstream by default as 0
   seq_header->order_hint_bits_minus1 =
         seq_header->enable_superres =
      ((associatedMetadata.m_associatedEncodeConfig.m_encoderCodecSpecificConfigDesc.m_AV1Config.FeatureFlags &
   D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_SUPER_RESOLUTION) != 0) ?
      1 :
   seq_header->enable_cdef =
      ((associatedMetadata.m_associatedEncodeConfig.m_encoderCodecSpecificConfigDesc.m_AV1Config.FeatureFlags &
   D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_CDEF_FILTERING) != 0) ?
      1 :
   seq_header->enable_restoration =
      ((associatedMetadata.m_associatedEncodeConfig.m_encoderCodecSpecificConfigDesc.m_AV1Config.FeatureFlags &
   D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_LOOP_RESTORATION_FILTER) != 0) ?
               seq_header->color_config.bit_depth = associatedMetadata.m_associatedEncodeConfig.m_encodeFormatInfo.Format;
   // seq_header->color_config.mono_chrome; // coded in bitstream by default as 0
   // seq_header->color_config.color_description_present_flag; // memset default as 0
   // seq_header->color_config.color_primaries; // memset default as 0
   // seq_header->color_config.transfer_characteristics; // memset default as 0
   // seq_header->color_config.matrix_coefficients; // memset default as 0
   // seq_header->color_config.color_range; // memset default as 0
   seq_header->color_config.chroma_sample_position = 0;   // CSP_UNKNOWN
   seq_header->color_config.subsampling_x = 1;            // DX12 4:2:0 only
   seq_header->color_config.subsampling_y = 1;            // DX12 4:2:0 only
               }
      void
   fill_av1_pic_header(EncodedBitstreamResolvedMetadata &associatedMetadata,
                     av1_pic_header_t *pic_header,
   {
      // Set all zero by default
            {
               debug_printf("[d3d12_video_enc_av1] Parsing driver post encode "
            debug_printf("Post encode quantization_params.BaseQIndex: %" PRIu64 "\n",
         debug_printf("Post encode quantization_params.YDCDeltaQ: %" PRId64 "\n",
         debug_printf("Post encode quantization_params.UDCDeltaQ: %" PRId64 "\n",
         debug_printf("Post encode quantization_params.UACDeltaQ: %" PRId64 "\n",
         debug_printf("Post encode quantization_params.VDCDeltaQ: %" PRId64 "\n",
         debug_printf("Post encode quantization_params.VACDeltaQ: %" PRId64 "\n",
         debug_printf("Post encode quantization_params.UsingQMatrix: %" PRIu64 "\n",
         debug_printf("Post encode quantization_params.QMY: %" PRIu64 "\n", pic_header->quantization_params.QMY);
   debug_printf("Post encode quantization_params.QMU: %" PRIu64 "\n", pic_header->quantization_params.QMU);
               pic_header->segmentation_enabled = 0;
   if ((associatedMetadata.m_associatedEncodeConfig.m_encoderPicParamsDesc.m_AV1PicData.Flags &
               // The segmentation info comes from the driver
   pic_header->segmentation_config = pParsedPostEncodeValues->SegmentationConfig;
               {
      debug_printf("[d3d12_video_enc_av1] Parsing driver post encode "
                  debug_printf("Post encode delta_q_params.DeltaQPresent: %" PRIu64 "\n", pic_header->delta_q_params.DeltaQPresent);
               {
      debug_printf("[d3d12_video_enc_av1] Parsing driver post encode "
                  debug_printf("Post encode delta_lf_params.DeltaLFMulti: %" PRIu64 "\n", pic_header->delta_lf_params.DeltaLFMulti);
   debug_printf("Post encode delta_lf_params.DeltaLFPresent: %" PRIu64 "\n",
                     {
      debug_printf("[d3d12_video_enc_av1] Parsing driver post encode "
                  debug_printf("Post encode loop_filter_params.LoopFilterDeltaEnabled: %" PRIu64 "\n",
         debug_printf("Post encode loop_filter_params.LoopFilterLevel[0]: %" PRIu64 "\n",
         debug_printf("Post encode loop_filter_params.LoopFilterLevel[1]: %" PRIu64 "\n",
         debug_printf("Post encode loop_filter_params.LoopFilterLevelU: %" PRIu64 "\n",
         debug_printf("Post encode loop_filter_params.LoopFilterLevelV: %" PRIu64 "\n",
         debug_printf("Post encode loop_filter_params.LoopFilterSharpnessLevel: %" PRIu64 "\n",
         debug_printf("Post encode loop_filter_params.ModeDeltas[0]: %" PRIu64 "\n",
         debug_printf("Post encode loop_filter_params.ModeDeltas[1]: %" PRIu64 "\n",
         for (uint8_t i = 0; i < 8; i++) {
      debug_printf("Post encode loop_filter_params.RefDeltas[%d]: %" PRIu64 "\n",
            }
   debug_printf("Post encode loop_filter_params.UpdateModeDelta: %" PRIu64 "\n",
         debug_printf("Post encode loop_filter_params.UpdateRefDelta: %" PRIu64 "\n",
               {
      debug_printf("[d3d12_video_enc_av1] Parsing driver post encode "
                           debug_printf("Post encode cdef_params.CdefDampingMinus3: %" PRIu64 "\n",
            for (uint8_t i = 0; i < 8; i++) {
      debug_printf("Post encode cdef_params.CdefYPriStrength[%d]: %" PRIu64 "\n",
                  debug_printf("Post encode cdef_params.CdefUVPriStrength[%d]: %" PRIu64 "\n",
                  debug_printf("Post encode cdef_params.CdefYSecStrength[%d]: %" PRIu64 "\n",
                  debug_printf("Post encode cdef_params.CdefUVSecStrength[%d]: %" PRIu64 "\n",
                        {
      debug_printf("[d3d12_video_enc_av1] Parsing driver post encode "
         pic_header->reference_select = (pParsedPostEncodeValues->CompoundPredictionType ==
                                 // if ((pevFlags & D3D12_VIDEO_ENCODER_AV1_POST_ENCODE_VALUES_FLAG_CONTEXT_UPDATE_TILE_ID) != 0)
   // We just copy the whole structure, the driver must copy it even if the pev flag is not set
            {
      debug_printf("[d3d12_video_enc_av1] Parsing driver post encode "
         pic_header->primary_ref_frame = static_cast<uint32_t>(pParsedPostEncodeValues->PrimaryRefFrame);
               debug_printf("Post encode tile_info.tile_partition.ContextUpdateTileId: %" PRIu64 "\n",
         debug_printf("Post encode tile_info.tile_partition.ColCount: %" PRIu64 "\n",
         debug_printf("Post encode tile_info.tile_partition.RowCount: %" PRIu64 "\n",
            assert(pic_header->tile_info.tile_partition.ColCount < 64);
   for (uint8_t i = 0; i < pic_header->tile_info.tile_partition.ColCount; i++) {
      debug_printf("Post encode tile_info.tile_partition.ColWidths[%d]: %" PRIu64 "\n",
                     assert(pic_header->tile_info.tile_partition.RowCount < 64);
   for (uint8_t i = 0; i < pic_header->tile_info.tile_partition.RowCount; i++) {
      debug_printf("Post encode tile_info.tile_partition.RowHeights[%d]: %" PRIu64 "\n",
                     pic_header->show_existing_frame = 0;
                     pic_header->frame_type = associatedMetadata.m_associatedEncodeConfig.m_encoderPicParamsDesc.m_AV1PicData.FrameType;
   {
      UINT EncodeOrderInGop =
                  UINT ShowOrderInGop =
                              pic_header->showable_frame =   // this will always be showable for P/B frames, even when using show_frame for B
            associatedMetadata.m_associatedEncodeConfig.m_encoderPicParamsDesc.m_AV1PicData.FrameType !=
      pic_header->error_resilient_mode =
      ((associatedMetadata.m_associatedEncodeConfig.m_encoderPicParamsDesc.m_AV1PicData.Flags &
      pic_header->disable_cdf_update =
      ((associatedMetadata.m_associatedEncodeConfig.m_encoderPicParamsDesc.m_AV1PicData.Flags &
         pic_header->allow_screen_content_tools =
      ((associatedMetadata.m_associatedEncodeConfig.m_encoderPicParamsDesc.m_AV1PicData.Flags &
      pic_header->force_integer_mv =
      ((associatedMetadata.m_associatedEncodeConfig.m_encoderPicParamsDesc.m_AV1PicData.Flags &
         // frame_size_override_flag is not coded + default as 1 for SWITCH_FRAME and explicitly coded otherwise
   if (pic_header->frame_type == D3D12_VIDEO_ENCODER_AV1_FRAME_TYPE_SWITCH_FRAME)
         else
                     pic_header->refresh_frame_flags =
            // frame_size (comes from input texture size)
   pic_header->FrameWidth = associatedMetadata.m_associatedEncodeConfig.m_currentResolution.Width;
   pic_header->UpscaledWidth = associatedMetadata.m_associatedEncodeConfig.m_currentResolution.Width;
            // render_size (comes from AV1 pipe picparams pic)
   pic_header->RenderWidth = associatedMetadata.m_associatedEncodeConfig.m_FrameCroppingCodecConfig.right;
            bool use_128x128_superblock =
      ((associatedMetadata.m_associatedEncodeConfig.m_encoderCodecSpecificConfigDesc.m_AV1Config.FeatureFlags &
   D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_128x128_SUPERBLOCK) != 0) ?
      1 :
   unsigned MiCols = 2 * ((pic_header->FrameWidth + 7) >> 3);
   unsigned MiRows = 2 * ((pic_header->FrameHeight + 7) >> 3);
   pic_header->frame_width_sb = use_128x128_superblock ? ((MiCols + 31) >> 5) : ((MiCols + 15) >> 4);
            pic_header->use_superres = ((associatedMetadata.m_associatedEncodeConfig.m_encoderPicParamsDesc.m_AV1PicData.Flags &
         pic_header->SuperresDenom =
            pic_header->allow_intrabc = ((associatedMetadata.m_associatedEncodeConfig.m_encoderPicParamsDesc.m_AV1PicData.Flags &
            for (unsigned i = 0; i < ARRAY_SIZE(associatedMetadata.m_associatedEncodeConfig.m_encoderPicParamsDesc.m_AV1PicData
            i++) {
   pic_header->ref_order_hint[i] = associatedMetadata.m_associatedEncodeConfig.m_encoderPicParamsDesc.m_AV1PicData
                     for (uint8_t i = 0; i < ARRAY_SIZE(pParsedPostEncodeValues->ReferenceIndices); i++)
            pic_header->allow_high_precision_mv =
      ((associatedMetadata.m_associatedEncodeConfig.m_encoderPicParamsDesc.m_AV1PicData.Flags &
      pic_header->interpolation_filter =
         pic_header->is_motion_mode_switchable =
      ((associatedMetadata.m_associatedEncodeConfig.m_encoderPicParamsDesc.m_AV1PicData.Flags &
      pic_header->use_ref_frame_mvs =
      ((associatedMetadata.m_associatedEncodeConfig.m_encoderPicParamsDesc.m_AV1PicData.Flags &
      pic_header->disable_frame_end_update_cdf =
      ((associatedMetadata.m_associatedEncodeConfig.m_encoderPicParamsDesc.m_AV1PicData.Flags &
         pic_header->tile_info.tile_mode = associatedMetadata.m_associatedEncodeConfig.m_encoderSliceConfigMode;
   pic_header->tile_info.tile_support_caps =
            pic_header->tile_info.uniform_tile_spacing_flag = 0;
   if ((pic_header->tile_info.tile_mode == D3D12_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE_UNIFORM_GRID_PARTITION) ||
      (pic_header->tile_info.tile_mode == D3D12_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE_FULL_FRAME))
      else if (pic_header->tile_info.tile_mode ==
               else
            // lr_params
   for (uint32_t i = 0; i < 3; i++)
      pic_header->lr_params.lr_type[i] = associatedMetadata.m_associatedEncodeConfig.m_encoderPicParamsDesc.m_AV1PicData
      bool lr_enabled =
         if (lr_enabled) {
      uint8_t luma_shift_total = log2(d3d12_video_encoder_looprestorationsize_d3d12_to_uint_av1(
                     pic_header->lr_params.lr_unit_shift = (luma_shift_total > 0) ? 1 : 0;
   pic_header->lr_params.lr_unit_extra_shift = (luma_shift_total > 1) ? 1 : 0;
   assert(associatedMetadata.m_associatedEncodeConfig.m_encoderPicParamsDesc.m_AV1PicData.FrameRestorationConfig
                  if (associatedMetadata.m_associatedEncodeConfig.m_encoderPicParamsDesc.m_AV1PicData.FrameRestorationConfig
            pic_header->lr_params.lr_uv_shift = log2(d3d12_video_encoder_looprestorationsize_d3d12_to_uint_av1(
                  } else {
                     pic_header->TxMode = associatedMetadata.m_associatedEncodeConfig.m_encoderPicParamsDesc.m_AV1PicData.TxMode;
   pic_header->skip_mode_present =
      ((associatedMetadata.m_associatedEncodeConfig.m_encoderPicParamsDesc.m_AV1PicData.Flags &
      pic_header->allow_warped_motion =
      ((associatedMetadata.m_associatedEncodeConfig.m_encoderPicParamsDesc.m_AV1PicData.Flags &
      pic_header->reduced_tx_set =
      ((associatedMetadata.m_associatedEncodeConfig.m_encoderPicParamsDesc.m_AV1PicData.Flags &
         }
      D3D12_VIDEO_ENCODER_AV1_RESTORATION_TILESIZE
   d3d12_video_encoder_looprestorationsize_uint_to_d3d12_av1(uint32_t pixel_size)
   {
      switch (pixel_size) {
      case 32:
   {
         } break;
   case 64:
   {
         } break;
   case 128:
   {
         } break;
   case 256:
   {
         } break;
   default:
   {
               }
      unsigned
   d3d12_video_encoder_looprestorationsize_d3d12_to_uint_av1(D3D12_VIDEO_ENCODER_AV1_RESTORATION_TILESIZE d3d12_type)
   {
      switch (d3d12_type) {
      case D3D12_VIDEO_ENCODER_AV1_RESTORATION_TILESIZE_32x32:
   {
         } break;
   case D3D12_VIDEO_ENCODER_AV1_RESTORATION_TILESIZE_64x64:
   {
         } break;
   case D3D12_VIDEO_ENCODER_AV1_RESTORATION_TILESIZE_128x128:
   {
         } break;
   case D3D12_VIDEO_ENCODER_AV1_RESTORATION_TILESIZE_256x256:
   {
         } break;
   default:
   {
               }
      /*
   * Called at get_feedback time FOR A PREVIOUSLY RECORDED AND EXECUTED FRAME
   */
   unsigned
   d3d12_video_encoder_build_post_encode_codec_bitstream_av1(struct d3d12_video_encoder *pD3D12Enc,
               {
               ID3D12Resource *pResolvedMetadataBuffer = associatedMetadata.spBuffer.Get();
            struct d3d12_screen *pD3D12Screen = (struct d3d12_screen *) pD3D12Enc->m_pD3D12Screen;
   pipe_resource *pPipeResolvedMetadataBuffer =
         assert(resourceMetadataSize < INT_MAX);
   struct pipe_box box = {
      0,                                        // x
   0,                                        // y
   0,                                        // z
   static_cast<int>(resourceMetadataSize),   // width
   1,                                        // height
      };
   struct pipe_transfer *mapTransferMetadata;
   uint8_t *pMetadataBufferSrc =
      reinterpret_cast<uint8_t *>(pD3D12Enc->base.context->buffer_map(pD3D12Enc->base.context,
                                          D3D12_VIDEO_ENCODER_OUTPUT_METADATA *pParsedMetadata =
                  D3D12_VIDEO_ENCODER_FRAME_SUBREGION_METADATA *pFrameSubregionMetadata =
         pMetadataBufferSrc +=
            D3D12_VIDEO_ENCODER_AV1_PICTURE_CONTROL_SUBREGIONS_LAYOUT_DATA_TILES *pParsedTilePartitions =
                  D3D12_VIDEO_ENCODER_AV1_POST_ENCODE_VALUES *pParsedPostEncodeValues =
                  debug_printf("[d3d12_video_enc_av1] D3D12_VIDEO_ENCODER_OUTPUT_METADATA.EncodeErrorFlags: %" PRIx64 " \n",
         debug_printf("[d3d12_video_enc_av1] D3D12_VIDEO_ENCODER_OUTPUT_METADATA.EncodeStats.AverageQP: %" PRIu64 " \n",
         debug_printf("[d3d12_video_enc_av1] D3D12_VIDEO_ENCODER_OUTPUT_METADATA.EncodeStats.IntraCodingUnitsCount: %" PRIu64
               debug_printf("[d3d12_video_enc_av1] D3D12_VIDEO_ENCODER_OUTPUT_METADATA.EncodeStats.InterCodingUnitsCount: %" PRIu64
               debug_printf("[d3d12_video_enc_av1] D3D12_VIDEO_ENCODER_OUTPUT_METADATA.EncodeStats.SkipCodingUnitsCount: %" PRIu64
               debug_printf("[d3d12_video_enc_av1] "
               debug_printf("[d3d12_video_enc_av1] "
               debug_printf("[d3d12_video_enc_av1] D3D12_VIDEO_ENCODER_OUTPUT_METADATA.EncodedBitstreamWrittenBytesCount: %" PRIu64
               debug_printf("[d3d12_video_enc_av1] D3D12_VIDEO_ENCODER_OUTPUT_METADATA.WrittenSubregionsCount: %" PRIu64 " \n",
            for (uint8_t i = 0; i < pParsedMetadata->WrittenSubregionsCount; i++) {
      debug_printf("[d3d12_video_enc_av1] D3D12_VIDEO_ENCODER_FRAME_SUBREGION_METADATA[%d].bHeaderSize: %" PRIu64 " \n",
               debug_printf("[d3d12_video_enc_av1] D3D12_VIDEO_ENCODER_FRAME_SUBREGION_METADATA[%d].bStartOffset: %" PRIu64
               " \n",
   debug_printf("[d3d12_video_enc_av1] D3D12_VIDEO_ENCODER_FRAME_SUBREGION_METADATA[%d].bSize: %" PRIu64 " \n",
                     if (pParsedMetadata->EncodeErrorFlags != D3D12_VIDEO_ENCODER_ENCODE_ERROR_FLAG_NO_ERROR) {
      debug_printf("[d3d12_video_enc_av1] Encode GPU command for fence %" PRIu64 " failed - EncodeErrorFlags: %" PRIu64
               "\n",
   assert(false);
               if (pParsedMetadata->EncodedBitstreamWrittenBytesCount == 0u) {
      debug_printf("[d3d12_video_enc_av1] Encode GPU command for fence %" PRIu64
               assert(false);
                        av1_seq_header_t seqHdr = {};
   fill_av1_seq_header(associatedMetadata, &seqHdr);
   av1_pic_header_t picHdr = {};
            bool bNeedSeqUpdate = false;
   bool diff_uv_delta_from_pev = (picHdr.quantization_params.VDCDeltaQ != picHdr.quantization_params.UDCDeltaQ) ||
                  // Make sure the Seq header allows diff_uv_delta from pev
   if (diff_uv_delta_from_pev && !seqHdr.color_config.separate_uv_delta_q) {
      seqHdr.color_config.separate_uv_delta_q = 1;
               d3d12_video_bitstream_builder_av1 *pAV1BitstreamBuilder =
                  size_t writtenTemporalDelimBytes = 0;
   if (picHdr.show_frame && associatedMetadata.m_CodecSpecificData.AV1HeadersInfo.temporal_delim_rendered) {
      pAV1BitstreamBuilder->write_temporal_delimiter_obu(
      pD3D12Enc->m_BitstreamHeadersBuffer,
   pD3D12Enc->m_BitstreamHeadersBuffer.begin(),   // placingPositionStart
      );
   assert(pD3D12Enc->m_BitstreamHeadersBuffer.size() == writtenTemporalDelimBytes);
               size_t writtenSequenceBytes = 0;
            // on first frame or on resolution change or if we changed seq hdr values with post encode values
   bool writeNewSeqHeader = isFirstFrame || bNeedSeqUpdate ||
                  if (writeNewSeqHeader) {
      pAV1BitstreamBuilder->write_sequence_header(
      &seqHdr,
   pD3D12Enc->m_BitstreamHeadersBuffer,
   pD3D12Enc->m_BitstreamHeadersBuffer.begin() + writtenTemporalDelimBytes,   // placingPositionStart
      );
   assert(pD3D12Enc->m_BitstreamHeadersBuffer.size() == (writtenSequenceBytes + writtenTemporalDelimBytes));
               // Only supported bitstream format is with obu_size for now.
            size_t writtenFrameBytes = 0;
            pipe_resource *src_driver_bitstream =
                  size_t comp_bitstream_offset = 0;
   if (associatedMetadata.m_CodecSpecificData.AV1HeadersInfo.enable_frame_obu) {
               assert(associatedMetadata.m_associatedEncodeConfig.m_encoderSliceConfigDesc.m_TilesConfig_AV1.TilesGroupsCount ==
            // write_frame_header writes OBU_FRAME except tile_obu_group, but included obu_size for tile_group_obu as
   // calculated below
   size_t tile_group_obu_size = 0;
   size_t decode_tile_elements_size = 0;
   pAV1BitstreamBuilder->calculate_tile_group_obu_size(
      pParsedMetadata,
   pFrameSubregionMetadata,
   associatedMetadata.m_associatedEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1TileCaps
               associatedMetadata.m_associatedEncodeConfig.m_encoderSliceConfigDesc.m_TilesConfig_AV1.TilesPartition,
   associatedMetadata.m_associatedEncodeConfig.m_encoderSliceConfigDesc.m_TilesConfig_AV1.TilesGroups[0],
               pAV1BitstreamBuilder->write_frame_header(
      &seqHdr,
   &picHdr,
   OBU_FRAME,
   tile_group_obu_size,   // We need it to code obu_size of OBU_FRAME open_bitstream_unit()
   pD3D12Enc->m_BitstreamHeadersBuffer,
   pD3D12Enc->m_BitstreamHeadersBuffer.begin() + writtenSequenceBytes +
                              assert(pD3D12Enc->m_BitstreamHeadersBuffer.size() ==
            debug_printf("Uploading %" PRIu64
                        // Upload headers to the finalized compressed bitstream buffer
   // Note: The buffer_subdata is queued in pD3D12Enc->base.context but doesn't execute immediately
   pD3D12Enc->base.context->buffer_subdata(pD3D12Enc->base.context,                   // context
                                    comp_bitstream_offset = pD3D12Enc->m_BitstreamHeadersBuffer.size();
            upload_tile_group_obu(
      pD3D12Enc,
   tile_group_obu_size,
   decode_tile_elements_size,
   associatedMetadata.m_StagingBitstreamConstruction,
   0,   // staging_bitstream_buffer_offset,
   src_driver_bitstream,
   associatedMetadata.comp_bit_destination,
   comp_bitstream_offset,
   pFrameSubregionMetadata,
   associatedMetadata.m_associatedEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1TileCaps
               associatedMetadata.m_associatedEncodeConfig.m_encoderSliceConfigDesc.m_TilesConfig_AV1.TilesPartition,
               writtenTileBytes += tile_group_obu_size;
            // Flush batch with upload work and wait on this CPU thread for GPU work completion
   {
      struct pipe_fence_handle *pUploadGPUCompletionFence = NULL;
   pD3D12Enc->base.context->flush(pD3D12Enc->base.context,
               assert(pUploadGPUCompletionFence);
   debug_printf(
      "[d3d12_video_encoder] d3d12_video_encoder_build_post_encode_codec_bitstream_av1 - Waiting on GPU "
               pD3D12Enc->m_pD3D12Screen->base.fence_finish(&pD3D12Enc->m_pD3D12Screen->base,
                     pD3D12Enc->m_pD3D12Screen->base.fence_reference(&pD3D12Enc->m_pD3D12Screen->base,
               } else {
               // Build open_bitstream_unit for OBU_FRAME_HEADER
   pAV1BitstreamBuilder->write_frame_header(&seqHdr,
                                                               assert(pD3D12Enc->m_BitstreamHeadersBuffer.size() ==
            debug_printf("Uploading %" PRIu64 " bytes from OBU headers to comp_bit_destination %p at offset 0\n",
                  // Upload headers to the finalized compressed bitstream buffer
   // Note: The buffer_subdata is queued in pD3D12Enc->base.context but doesn't execute immediately
   pD3D12Enc->base.context->buffer_subdata(pD3D12Enc->base.context,                   // context
                                    comp_bitstream_offset = pD3D12Enc->m_BitstreamHeadersBuffer.size();
            for (int tg_idx = 0;
      tg_idx <
   associatedMetadata.m_associatedEncodeConfig.m_encoderSliceConfigDesc.m_TilesConfig_AV1.TilesGroupsCount;
   tg_idx++) {
                  debug_printf("Uploading tile group %d to comp_bit_destination %p at offset %" PRIu64 "\n",
                        size_t tile_group_obu_size = 0;
   size_t decode_tile_elements_size = 0;
   pAV1BitstreamBuilder->calculate_tile_group_obu_size(
      pParsedMetadata,
   pFrameSubregionMetadata,
   associatedMetadata.m_associatedEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1TileCaps
               associatedMetadata.m_associatedEncodeConfig.m_encoderSliceConfigDesc.m_TilesConfig_AV1.TilesPartition,
   currentTg,
               size_t writtenTileObuPrefixBytes = 0;
   pAV1BitstreamBuilder->write_obu_tile_group_header(
      tile_group_obu_size,   // tile_group_obu() size to pack OBU_TILE_GROUP obu_size element
   associatedMetadata.m_StagingBitstreamConstruction,   // Source CPU buffer cannot be overwritten until
         associatedMetadata.m_StagingBitstreamConstruction.begin() +
               debug_printf("Written %" PRIu64 " bytes for OBU_TILE_GROUP open_bitstream_unit() prefix with obu_header() and "
                                       // Note: The buffer_subdata is queued in pD3D12Enc->base.context but doesn't execute immediately
   pD3D12Enc->base.context->buffer_subdata(
      pD3D12Enc->base.context,                   // context
   associatedMetadata.comp_bit_destination,   // comp. bitstream
   PIPE_MAP_WRITE,                            // usage PIPE_MAP_x
   comp_bitstream_offset,                     // offset
               debug_printf("Uploading %" PRIu64 " bytes for OBU_TILE_GROUP open_bitstream_unit() prefix with obu_header() "
               "and obu_size: %" PRIu64 " to comp_bit_destination %p at offset %" PRIu64 "\n",
                                    size_t written_bytes_to_staging_bitstream_buffer = 0;
   upload_tile_group_obu(
      pD3D12Enc,
   tile_group_obu_size,
   decode_tile_elements_size,
   associatedMetadata.m_StagingBitstreamConstruction,
   staging_bitstream_buffer_offset,
   src_driver_bitstream,
   associatedMetadata.comp_bit_destination,
   comp_bitstream_offset,
   pFrameSubregionMetadata,
   associatedMetadata.m_associatedEncodeCapabilities.m_encoderCodecSpecificConfigCaps.m_AV1TileCaps
               associatedMetadata.m_associatedEncodeConfig.m_encoderSliceConfigDesc.m_TilesConfig_AV1.TilesPartition,
               staging_bitstream_buffer_offset += written_bytes_to_staging_bitstream_buffer;
                  // Flush batch with upload work and wait on this CPU thread for GPU work completion
   // for this case we have to do it within the loop, so the CUP arrays that are reused
   // on each loop are completely done being read by the GPU before we modify them.
   {
      struct pipe_fence_handle *pUploadGPUCompletionFence = NULL;
   pD3D12Enc->base.context->flush(pD3D12Enc->base.context,
               assert(pUploadGPUCompletionFence);
   debug_printf(
                        pD3D12Enc->m_pD3D12Screen->base.fence_finish(&pD3D12Enc->m_pD3D12Screen->base,
                     pD3D12Enc->m_pD3D12Screen->base.fence_reference(&pD3D12Enc->m_pD3D12Screen->base,
                           size_t extra_show_existing_frame_payload_bytes = 0;
               // When writing in the bitstream a frame that will be accessed as reference by a frame we did not
   // write to the bitstream yet, let's store them for future show_existing_frame mechanism
   pD3D12Enc->m_currentEncodeConfig.m_encoderCodecSpecificStateDescAV1.pendingShowableFrames.push_back(
                     // Check if any of the pending "to be showed later" frames is present in the current frame references
            for (auto pendingFrameIt =
                           // Check if current frame references uses this pendingFrameIt
   int cur_ref_idx_matching_pending = -1;
   for (unsigned i = 0; i < ARRAY_SIZE(picHdr.ref_frame_idx) /*NUM_REF_FRAMES*/; i++) {
      if (
      // Is a valid reference
   (associatedMetadata.m_associatedEncodeConfig.m_encoderPicParamsDesc.m_AV1PicData
      .ReferenceFramesReconPictureDescriptors[picHdr.ref_frame_idx[i]]
      // And matches the pending frame PictureIndex
                        // Store the reference index
   cur_ref_idx_matching_pending = picHdr.ref_frame_idx[i];
                  // If we found a reference of the current frame using pendingFrameIt,
   // - Issue the show_existing_frame header
   // - Remove it from the pending list
                        size_t writtenTemporalDelimBytes = 0;
   if (D3D12_VIDEO_AV1_INSERT_SHOW_EXISTING_FRAME_HEADER) {
      pAV1BitstreamBuilder->write_temporal_delimiter_obu(
      pD3D12Enc->m_BitstreamHeadersBuffer,
   pD3D12Enc->m_BitstreamHeadersBuffer.begin() + staging_buf_offset,   // placingPositionStart
                                                size_t writtenShowExistingFrameBytes = 0;
   av1_pic_header_t showExistingPicHdr = {};
                  if (D3D12_VIDEO_AV1_INSERT_SHOW_EXISTING_FRAME_HEADER) {
      pAV1BitstreamBuilder->write_frame_header(
      NULL,   // No seq header necessary for show_existing_frame
   &showExistingPicHdr,
   OBU_FRAME_HEADER,
   0,   // No tile info for OBU_FRAME_HEADER
   pD3D12Enc->m_BitstreamHeadersBuffer,
   pD3D12Enc->m_BitstreamHeadersBuffer.begin() + staging_buf_offset +
               }
                                                // Upload headers to the finalized compressed bitstream buffer
   // Note: The buffer_subdata is queued in pD3D12Enc->base.context but doesn't execute immediately
   pD3D12Enc->base.context->buffer_subdata(pD3D12Enc->base.context,                   // context
                                                   debug_printf("Written show_existing_frame OBU_FRAME header for previous frame with PictureIndex %d (Used "
                              // Remove it from the list of pending frames
   pendingFrameIt =
      pD3D12Enc->m_currentEncodeConfig.m_encoderCodecSpecificStateDescAV1.pendingShowableFrames.erase(
   } else {
                     // Flush again if uploaded show_existing_frame header(s) to bitstream.
   if (extra_show_existing_frame_payload_bytes) {
      struct pipe_fence_handle *pUploadGPUCompletionFence = NULL;
   pD3D12Enc->base.context->flush(pD3D12Enc->base.context,
               assert(pUploadGPUCompletionFence);
   debug_printf("[d3d12_video_encoder] [show_existing_frame extra headers upload] "
                        pD3D12Enc->m_pD3D12Screen->base.fence_finish(&pD3D12Enc->m_pD3D12Screen->base,
                     pD3D12Enc->m_pD3D12Screen->base.fence_reference(&pD3D12Enc->m_pD3D12Screen->base,
                        // d3d12_resource_from_resource calls AddRef to it so this should only be deleting
   // the pipe_resource wrapping object and not the underlying spStagingBitstream
   pipe_resource_reference(&src_driver_bitstream, NULL);
            // Unmap the metadata buffer tmp storage
   pipe_buffer_unmap(pD3D12Enc->base.context, mapTransferMetadata);
            assert((writtenSequenceBytes + writtenTemporalDelimBytes + writtenFrameBytes +
         return static_cast<unsigned int>(writtenSequenceBytes + writtenTemporalDelimBytes + writtenFrameBytes +
      }
      void
   upload_tile_group_obu(struct d3d12_video_encoder *pD3D12Enc,
                        size_t tile_group_obu_size,
   size_t decode_tile_elements_size,
   std::vector<uint8_t> &staging_bitstream_buffer,
   size_t staging_bitstream_buffer_offset,
   pipe_resource *src_driver_bitstream,
   pipe_resource *comp_bit_destination,
   size_t comp_bit_destination_offset,
   const D3D12_VIDEO_ENCODER_FRAME_SUBREGION_METADATA *pFrameSubregionMetadata,
      {
      debug_printf("[Tile group start %d to end %d] Writing to comp_bit_destination %p starts at offset %" PRIu64 "\n",
               tileGroup.tg_start,
            debug_printf("[Tile group start %d to end %d] Using staging_bitstream_buffer %p at offset %" PRIu64
               " to write the tile_obu_group() prefix syntax: tile_start_and_end_present_flag, tg_start, tg_end and "
   "the tile_size_minus1\n",
   tileGroup.tg_start,
            // Reserve space upfront in the scratch storage
            size_t tile_obu_prefix_size = tile_group_obu_size - decode_tile_elements_size;
   if (staging_bitstream_buffer.size() < (staging_bitstream_buffer_offset + tile_obu_prefix_size))
            d3d12_video_encoder_bitstream bitstream_tile_group_obu;
   bitstream_tile_group_obu.setup_bitstream(staging_bitstream_buffer.size(),
                  uint8_t NumTiles = TilesPartition.ColCount * TilesPartition.RowCount;
   bool tile_start_and_end_present_flag = !(tileGroup.tg_start == 0 && (tileGroup.tg_end == (NumTiles - 1)));
   if (NumTiles > 1)
      bitstream_tile_group_obu.put_bits(1,
         if (!(NumTiles == 1 || !tile_start_and_end_present_flag)) {
      uint8_t tileBits = log2(TilesPartition.ColCount) + log2(TilesPartition.RowCount);
   bitstream_tile_group_obu.put_bits(tileBits, tileGroup.tg_start);   // tg_start	   f(tileBits)
                                          debug_printf("[Tile group start %d to end %d] Written %" PRIu64
               " bitstream_tile_group_obu_bytes at staging_bitstream_buffer %p at offset %" PRIu64
   " for tile_obu_group() prefix syntax: tile_start_and_end_present_flag, tg_start, tg_end\n",
   tileGroup.tg_start,
   tileGroup.tg_end,
               // Save this to compare the final written destination byte size against the expected tile_group_obu_size
   // at the end of the function
            // Upload first part of the header to compressed bitstream destination
   // Note: The buffer_subdata is queued in pD3D12Enc->base.context but doesn't execute immediately
   if (bitstream_tile_group_obu_bytes > 0) {
      pD3D12Enc->base.context->buffer_subdata(
      pD3D12Enc->base.context,                                              // context
   comp_bit_destination,                                                 // comp. bitstream
   PIPE_MAP_WRITE,                                                       // usage PIPE_MAP_x
   comp_bit_destination_offset,                                          // offset
               debug_printf("[Tile group start %d to end %d]  Uploading %" PRIu64 " bytes"
               " for tile_obu_group() prefix syntax: tile_start_and_end_present_flag, tg_start, tg_end"
   " from staging_bitstream_buffer %p at offset %" PRIu64
   " to comp_bit_destination %p at offset %" PRIu64 "\n",
   tileGroup.tg_start,
   tileGroup.tg_end,
   static_cast<uint64_t>(bitstream_tile_group_obu_bytes),
   staging_bitstream_buffer.data(),
            comp_bit_destination_offset += bitstream_tile_group_obu_bytes;
               size_t src_offset = 0;
   for (UINT64 TileIdx = tileGroup.tg_start; TileIdx <= tileGroup.tg_end; TileIdx++) {
      size_t tile_size = pFrameSubregionMetadata[TileIdx].bSize - pFrameSubregionMetadata[TileIdx].bStartOffset;
   // The i-th tile is read from the src_buffer[offset] with offset = [sum j = (0, (i-1)){ tile[j].bSize }] +
   // tile[i].bStartOffset
   size_t src_buf_tile_position = src_offset + pFrameSubregionMetadata[TileIdx].bStartOffset;
            // tile_size_minus_1	not coded for last tile
   if ((TileIdx != tileGroup.tg_end)) {
      bitstream_tile_group_obu.put_le_bytes(TileSizeBytes,   // tile_size_minus_1	le(TileSizeBytes)
                  debug_printf("[Tile group start %d to end %d] [TileIdx %" PRIu64 "] Written %" PRIu64
               " written_bytes_to_staging_bitstream_buffer at staging_bitstream_buffer %p at offset %" PRIu64
   " for the tile_size_minus1 syntax\n",
   tileGroup.tg_start,
   tileGroup.tg_end,
                  // Upload current tile_size_minus_1
   // Note: The buffer_subdata is queued in pD3D12Enc->base.context but doesn't execute immediately
   pD3D12Enc->base.context->buffer_subdata(pD3D12Enc->base.context,       // context
                                                debug_printf("[Tile group start %d to end %d] [TileIdx %" PRIu64 "] Uploading %" PRIu64 " bytes"
               " for tile_obu_group() prefix syntax: tile_size_minus_1"
   " from staging_bitstream_buffer %p at offset %" PRIu64
   " to comp_bit_destination %p at offset %" PRIu64 "\n",
   tileGroup.tg_start,
   tileGroup.tg_end,
   TileIdx,
   static_cast<uint64_t>(TileSizeBytes),
                  comp_bit_destination_offset += TileSizeBytes;
                        struct pipe_box src_box = {
      static_cast<int>(src_buf_tile_position),   // x
   0,                                         // y
   0,                                         // z
   static_cast<int>(tile_size),               // width
   1,                                         // height
               pD3D12Enc->base.context->resource_copy_region(pD3D12Enc->base.context,       // ctx
                                                      debug_printf("[Tile group start %d to end %d] [TileIdx %" PRIu64 "] Written %" PRIu64
               " tile_size bytes data from src_driver_bitstream %p at offset %" PRIu64
   " into comp_bit_destination %p at offset %" PRIu64 "\n",
   tileGroup.tg_start,
   tileGroup.tg_end,
   TileIdx,
   static_cast<uint64_t>(tile_size),
   src_driver_bitstream,
                        // Make sure we wrote the expected bytes that match the obu_size elements
   // in the OBUs wrapping this uploaded tile_group_obu
      }
      void
   d3d12_video_encoder_store_current_picture_references_av1(d3d12_video_encoder *pD3D12Enc, uint64_t current_metadata_slot)
   {
      // Update DX12 picparams for post execution (get_feedback) after d3d12_video_encoder_references_manager_av1
   // changes
   pD3D12Enc->m_spEncodedFrameMetadata[current_metadata_slot].m_associatedEncodeConfig.m_encoderPicParamsDesc =
      }
