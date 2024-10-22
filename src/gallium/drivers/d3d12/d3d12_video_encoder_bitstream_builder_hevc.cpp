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
      #include "d3d12_video_encoder_bitstream_builder_hevc.h"
   #include <cmath>
      static uint8_t
   convert_profile12_to_stdprofile(D3D12_VIDEO_ENCODER_PROFILE_HEVC profile)
   {
      // Main is 1, Main10 is 2, one more than the D3D12 enum definition
      }
      void
   d3d12_video_bitstream_builder_hevc::init_profile_tier_level(HEVCProfileTierLevel *ptl,
                     {
               ptl->general_profile_space = 0;     // must be 0
   ptl->general_tier_flag = isHighTier ? 1 : 0;
            memset(ptl->general_profile_compatibility_flag, 0, sizeof(ptl->general_profile_compatibility_flag));
            ptl->general_progressive_source_flag = 1; // yes
   ptl->general_interlaced_source_flag = 0;  // no
   ptl->general_non_packed_constraint_flag = 1; // no frame packing arrangement SEI messages
   ptl->general_frame_only_constraint_flag = 1;
      }
      void
   d3d12_video_encoder_convert_from_d3d12_level_hevc(D3D12_VIDEO_ENCODER_LEVELS_HEVC level12,
         {
      specLevel = 3u;
   switch(level12)
   {
      case D3D12_VIDEO_ENCODER_LEVELS_HEVC_1:
   {
         } break;
   case D3D12_VIDEO_ENCODER_LEVELS_HEVC_2:
   {
         } break;
   case D3D12_VIDEO_ENCODER_LEVELS_HEVC_21:
   {
         } break;
   case D3D12_VIDEO_ENCODER_LEVELS_HEVC_3:
   {
         } break;
   case D3D12_VIDEO_ENCODER_LEVELS_HEVC_31:
   {
         } break;
   case D3D12_VIDEO_ENCODER_LEVELS_HEVC_4:
   {
         } break;
   case D3D12_VIDEO_ENCODER_LEVELS_HEVC_41:
   {
         } break;
   case D3D12_VIDEO_ENCODER_LEVELS_HEVC_5:
   {
         } break;
   case D3D12_VIDEO_ENCODER_LEVELS_HEVC_51:
   {
         } break;
   case D3D12_VIDEO_ENCODER_LEVELS_HEVC_52:
   {
         } break;
   case D3D12_VIDEO_ENCODER_LEVELS_HEVC_6 :
   {
         } break;
   case D3D12_VIDEO_ENCODER_LEVELS_HEVC_61 :
   {
         } break;
   case D3D12_VIDEO_ENCODER_LEVELS_HEVC_62 :
   {
         } break;
   default:
   {
               }
      D3D12_VIDEO_ENCODER_LEVELS_HEVC
   d3d12_video_encoder_convert_level_hevc(uint32_t hevcSpecLevel)
   {
      hevcSpecLevel = hevcSpecLevel / 3u;
   switch(hevcSpecLevel)
   {
      case 10:
   {
         } break;
   case 20:
   {
         } break;
   case 21:
   {
         } break;
   case 30:
   {
         } break;
   case 31:
   {
         } break;
   case 40:
   {
         } break;
   case 41:
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
      uint8_t
   d3d12_video_encoder_convert_12cusize_to_pixel_size_hevc(const D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_CUSIZE& cuSize)
   {
      switch(cuSize)
   {
      case D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_CUSIZE_8x8:
   {
         } break;
   case D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_CUSIZE_16x16:
   {
         } break;
   case D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_CUSIZE_32x32:
   {
         } break;
   case D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_CUSIZE_64x64:
   {
         } break;
   default:
   {
         unreachable(L"Not a supported cu size");
         }
      D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_CUSIZE
   d3d12_video_encoder_convert_pixel_size_hevc_to_12cusize(const uint32_t& cuSize)
   {
      switch(cuSize)
   {
      case 8u:
   {
         } break;
   case 16u:
   {
         } break;
   case 32u:
   {
         } break;
   case 64u:
   {
         } break;
   default:
   {
               }
      uint8_t 
   d3d12_video_encoder_convert_12tusize_to_pixel_size_hevc(const D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_TUSIZE& TUSize)
   {
      switch(TUSize)
   {
      case D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_TUSIZE_4x4:
   {
         } break;
   case D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_TUSIZE_8x8:
   {
         } break;
   case D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_TUSIZE_16x16:
   {
         } break;
   case D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_TUSIZE_32x32:
   {
         } break;        
   default:
   {
               }
      D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_TUSIZE 
   d3d12_video_encoder_convert_pixel_size_hevc_to_12tusize(const uint32_t& TUSize)
   {
      switch(TUSize)
   {
      case 4u:
   {
         } break;
   case 8u:
   {
         } break;
   case 16u:
   {
         } break;
   case 32u:
   {
         } break;        
   default:
   {
               }
      void
   d3d12_video_bitstream_builder_hevc::build_vps(const D3D12_VIDEO_ENCODER_PROFILE_HEVC& profile,
            const D3D12_VIDEO_ENCODER_LEVEL_TIER_CONSTRAINTS_HEVC& level,
   const DXGI_FORMAT inputFmt,
   uint8_t maxRefFrames,
   bool gopHasBFrames,
   uint8_t vps_video_parameter_set_id,
   std::vector<BYTE> &headerBitstream,
   std::vector<BYTE>::iterator placingPositionStart,
      {
      uint8_t HEVCProfileIdc = convert_profile12_to_stdprofile(profile);
   uint32_t HEVCLevelIdc = 0u;
   d3d12_video_encoder_convert_from_d3d12_level_hevc(level.Level, HEVCLevelIdc);
            memset(&m_latest_vps, 0, sizeof(HevcVideoParameterSet));
   m_latest_vps.nalu = {
         // forbidden_zero_bit 
   0u,
   // nal_unit_type 
   HEVC_NALU_VPS_NUT,
   // nuh_layer_id 
   0u,
   // nuh_temporal_id_plus1 
            m_latest_vps.vps_video_parameter_set_id = vps_video_parameter_set_id,
   m_latest_vps.vps_reserved_three_2bits = 3u;
   m_latest_vps.vps_max_layers_minus1 = 0u;
   m_latest_vps.vps_max_sub_layers_minus1 = 0u;
   m_latest_vps.vps_temporal_id_nesting_flag = 1u;
   m_latest_vps.vps_reserved_0xffff_16bits = 0xFFFF;
   init_profile_tier_level(&m_latest_vps.ptl, HEVCProfileIdc, HEVCLevelIdc, isHighTier);
   m_latest_vps.vps_sub_layer_ordering_info_present_flag = 0u;
   for (int i = (m_latest_vps.vps_sub_layer_ordering_info_present_flag ? 0 : m_latest_vps.vps_max_sub_layers_minus1); i <= m_latest_vps.vps_max_sub_layers_minus1; i++) {
      m_latest_vps.vps_max_dec_pic_buffering_minus1[i] = (maxRefFrames/*previous reference frames*/ + 1 /*additional current frame recon pic*/) - 1/**minus1 for header*/;        
   m_latest_vps.vps_max_num_reorder_pics[i] = gopHasBFrames ? m_latest_vps.vps_max_dec_pic_buffering_minus1[i] : 0;
               // Print built VPS structure
   debug_printf("[HEVCBitstreamBuilder] HevcVideoParameterSet Structure generated before writing to bitstream:\n");
                     if(pVPSStruct != nullptr)
   {
            }
      void
   d3d12_video_bitstream_builder_hevc::build_sps(const HevcVideoParameterSet& parentVPS,
            uint8_t seq_parameter_set_id,
   const D3D12_VIDEO_ENCODER_PICTURE_RESOLUTION_DESC& encodeResolution,
   const D3D12_BOX& crop_window_upper_layer,
   const UINT picDimensionMultipleRequirement,
   const DXGI_FORMAT& inputFmt,
   const D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC& codecConfig,
   const D3D12_VIDEO_ENCODER_SEQUENCE_GOP_STRUCTURE_HEVC& hevcGOP,    
   std::vector<BYTE> &headerBitstream,
   std::vector<BYTE>::iterator placingPositionStart,
      {
               // In case is 420 10 bits
   if(inputFmt == DXGI_FORMAT_P010)
   {
      m_latest_sps.bit_depth_luma_minus8 = 2;
               uint8_t minCuSize = d3d12_video_encoder_convert_12cusize_to_pixel_size_hevc(codecConfig.MinLumaCodingUnitSize);
   uint8_t maxCuSize = d3d12_video_encoder_convert_12cusize_to_pixel_size_hevc(codecConfig.MaxLumaCodingUnitSize);
   uint8_t minTuSize = d3d12_video_encoder_convert_12tusize_to_pixel_size_hevc(codecConfig.MinLumaTransformUnitSize);
            m_latest_sps.nalu.nal_unit_type = HEVC_NALU_SPS_NUT;
            m_latest_sps.sps_seq_parameter_set_id = seq_parameter_set_id;
   m_latest_sps.sps_max_sub_layers_minus1 = parentVPS.vps_max_sub_layers_minus1;
            // inherit PTL from parentVPS fully
                     // Codec spec dictates pic_width/height_in_luma_samples must be divisible by minCuSize but HW might have higher req pow 2 multiples
            // upper layer passes the viewport, can calculate the difference between it and pic_width_in_luma_samples
   D3D12_VIDEO_ENCODER_PICTURE_RESOLUTION_DESC viewport = { };
   viewport.Width = crop_window_upper_layer.front /* passes height */ - ((crop_window_upper_layer.left + crop_window_upper_layer.right) << 1);
            m_latest_sps.pic_width_in_luma_samples = ALIGN(encodeResolution.Width, picDimensionMultipleRequirement);
   m_latest_sps.pic_height_in_luma_samples = ALIGN(encodeResolution.Height, picDimensionMultipleRequirement);
   m_latest_sps.conf_win_right_offset = (m_latest_sps.pic_width_in_luma_samples - viewport.Width) >> 1;
   m_latest_sps.conf_win_bottom_offset = (m_latest_sps.pic_height_in_luma_samples - viewport.Height) >> 1;
            m_latest_sps.log2_max_pic_order_cnt_lsb_minus4 = hevcGOP.log2_max_pic_order_cnt_lsb_minus4;
            m_latest_sps.sps_sub_layer_ordering_info_present_flag = parentVPS.vps_sub_layer_ordering_info_present_flag;
   for (int i = (m_latest_sps.sps_sub_layer_ordering_info_present_flag ? 0 : m_latest_sps.sps_max_sub_layers_minus1); i <= m_latest_sps.sps_max_sub_layers_minus1; i++) {
      m_latest_sps.sps_max_dec_pic_buffering_minus1[i] = parentVPS.vps_max_dec_pic_buffering_minus1[i];
   m_latest_sps.sps_max_num_reorder_pics[i] = parentVPS.vps_max_num_reorder_pics[i];
               m_latest_sps.log2_min_luma_coding_block_size_minus3 = static_cast<uint8_t>(std::log2(minCuSize) - 3);
   m_latest_sps.log2_diff_max_min_luma_coding_block_size = static_cast<uint8_t>(std::log2(maxCuSize) - std::log2(minCuSize));
   m_latest_sps.log2_min_transform_block_size_minus2 = static_cast<uint8_t>(std::log2(minTuSize) - 2);
            m_latest_sps.max_transform_hierarchy_depth_inter = codecConfig.max_transform_hierarchy_depth_inter;
            m_latest_sps.scaling_list_enabled_flag = 0;
   m_latest_sps.amp_enabled_flag = ((codecConfig.ConfigurationFlags & D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_FLAG_USE_ASYMETRIC_MOTION_PARTITION) != 0) ? 1u : 0u;
   m_latest_sps.sample_adaptive_offset_enabled_flag = ((codecConfig.ConfigurationFlags & D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_FLAG_ENABLE_SAO_FILTER) != 0) ? 1u : 0u;
                     m_latest_sps.long_term_ref_pics_present_flag = ((codecConfig.ConfigurationFlags & D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_FLAG_ENABLE_LONG_TERM_REFERENCES) != 0) ? 1u : 0u;
            m_latest_sps.sps_temporal_mvp_enabled_flag = 0;
            m_latest_sps.vui_parameters_present_flag = 0;
            // Print built SPS structure
   debug_printf("[HEVCBitstreamBuilder] HevcSeqParameterSet Structure generated before writing to bitstream:\n");
   print_sps(m_latest_sps);
               if(outputSPS != nullptr)
   {
            }
      void
   d3d12_video_bitstream_builder_hevc::build_pps(const HevcSeqParameterSet& parentSPS,
            uint8_t pic_parameter_set_id,
   const D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC& codecConfig,
   const D3D12_VIDEO_ENCODER_PICTURE_CONTROL_CODEC_DATA_HEVC& pictureControl,
   std::vector<BYTE> &headerBitstream,
   std::vector<BYTE>::iterator placingPositionStart,
      {
               m_latest_pps.nalu.nal_unit_type = HEVC_NALU_PPS_NUT;
            m_latest_pps.pps_pic_parameter_set_id = pic_parameter_set_id;
                     m_latest_pps.num_ref_idx_lx_default_active_minus1[0] = static_cast<uint8_t>(std::max(static_cast<INT>(pictureControl.List0ReferenceFramesCount) - 1, 0));
            m_latest_pps.num_tile_columns_minus1 = 0u; // no tiling in D3D12
   m_latest_pps.num_tile_rows_minus1 = 0u; // no tiling in D3D12
   m_latest_pps.tiles_enabled_flag = 0u; // no tiling in D3D12
            m_latest_pps.lists_modification_present_flag = 0;
            m_latest_pps.deblocking_filter_control_present_flag = 1;
   m_latest_pps.deblocking_filter_override_enabled_flag = 0;
   m_latest_pps.pps_deblocking_filter_disabled_flag = 0;
   m_latest_pps.pps_scaling_list_data_present_flag = 0;
   m_latest_pps.pps_beta_offset_div2 = 0;
   m_latest_pps.pps_tc_offset_div2 = 0;
   m_latest_pps.pps_loop_filter_across_slices_enabled_flag = ((codecConfig.ConfigurationFlags & D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_FLAG_DISABLE_LOOP_FILTER_ACROSS_SLICES) != 0) ? 0 : 1;
   m_latest_pps.transform_skip_enabled_flag = ((codecConfig.ConfigurationFlags & D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_FLAG_ENABLE_TRANSFORM_SKIPPING) != 0) ? 1 : 0;
   m_latest_pps.constrained_intra_pred_flag = ((codecConfig.ConfigurationFlags & D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_FLAG_USE_CONSTRAINED_INTRAPREDICTION) != 0) ? 1 : 0;
   m_latest_pps.cabac_init_present_flag = 1;
   m_latest_pps.pps_slice_chroma_qp_offsets_present_flag = 1;
            // Print built PPS structure
   debug_printf("[HEVCBitstreamBuilder] HevcPicParameterSet Structure generated before writing to bitstream:\n");
                     if(outputPPS != nullptr)
   {
            }
      void
   d3d12_video_bitstream_builder_hevc::write_end_of_stream_nalu(std::vector<uint8_t> &         headerBitstream,
               {
         }
   void
   d3d12_video_bitstream_builder_hevc::write_end_of_sequence_nalu(std::vector<uint8_t> &         headerBitstream,
               {
         }
      void
   d3d12_video_bitstream_builder_hevc::print_vps(const HevcVideoParameterSet& VPS)
   {
               debug_printf("vps_video_parameter_set_id: %d\n", VPS.vps_video_parameter_set_id);
   debug_printf("vps_reserved_three_2bits: %d\n", VPS.vps_reserved_three_2bits);
   debug_printf("vps_max_layers_minus1: %d\n", VPS.vps_max_layers_minus1);
   debug_printf("vps_max_sub_layers_minus1: %d\n", VPS.vps_max_sub_layers_minus1);
            debug_printf("general_profile_space: %d\n", VPS.ptl.general_profile_space);
   debug_printf("general_tier_flag: %d\n", VPS.ptl.general_tier_flag);
   debug_printf("general_profile_idc: %d\n", VPS.ptl.general_profile_idc);
   debug_printf("general_progressive_source_flag: %d\n", VPS.ptl.general_progressive_source_flag);
   debug_printf("general_interlaced_source_flag: %d\n", VPS.ptl.general_interlaced_source_flag);
   debug_printf("general_non_packed_constraint_flag: %d\n", VPS.ptl.general_non_packed_constraint_flag);
   debug_printf("general_frame_only_constraint_flag: %d\n", VPS.ptl.general_frame_only_constraint_flag);
            debug_printf("vps_sub_layer_ordering_info_present_flag: %d\n", VPS.vps_sub_layer_ordering_info_present_flag);
   debug_printf("vps_max_dec_pic_buffering_minus1[%d]: %d\n", (0u), VPS.vps_max_dec_pic_buffering_minus1[0u]);
   debug_printf("vps_max_num_reorder_pics[%d]: %d\n", (0u), VPS.vps_max_num_reorder_pics[0u]);
   debug_printf("vps_max_latency_increase_plus1[%d]: %d\n", (0u), VPS.vps_max_latency_increase_plus1[0u]);
   debug_printf("vps_max_layer_id: %d\n", VPS.vps_max_layer_id);
   debug_printf("vps_num_layer_sets_minus1: %d\n", VPS.vps_num_layer_sets_minus1);
   debug_printf("vps_timing_info_present_flag: %d\n", VPS.vps_timing_info_present_flag);
   debug_printf("vps_num_units_in_tick: %d\n", VPS.vps_num_units_in_tick);
   debug_printf("vps_time_scale: %d\n", VPS.vps_time_scale);
   debug_printf("vps_poc_proportional_to_timing_flag: %d\n", VPS.vps_poc_proportional_to_timing_flag);
   debug_printf("vps_num_ticks_poc_diff_one_minus1: %d\n", VPS.vps_num_ticks_poc_diff_one_minus1);
   debug_printf("vps_num_hrd_parameters: %d\n", VPS.vps_num_hrd_parameters);
   debug_printf("vps_extension_flag: %d\n", VPS.vps_extension_flag);
               }
   void
   d3d12_video_bitstream_builder_hevc::print_sps(const HevcSeqParameterSet& SPS)
   {
               debug_printf("sps_video_parameter_set_id: %d\n", SPS.sps_video_parameter_set_id);
   debug_printf("sps_max_sub_layers_minus1: %d\n", SPS.sps_max_sub_layers_minus1);
   debug_printf("sps_temporal_id_nesting_flag: %d\n", SPS.sps_temporal_id_nesting_flag);
      debug_printf("general_profile_space: %d\n", SPS.ptl.general_profile_space);
   debug_printf("general_tier_flag: %d\n", SPS.ptl.general_tier_flag);
   debug_printf("general_profile_idc: %d\n", SPS.ptl.general_profile_idc);
   debug_printf("general_progressive_source_flag: %d\n", SPS.ptl.general_progressive_source_flag);
   debug_printf("general_interlaced_source_flag: %d\n", SPS.ptl.general_interlaced_source_flag);
   debug_printf("general_non_packed_constraint_flag: %d\n", SPS.ptl.general_non_packed_constraint_flag);
   debug_printf("general_frame_only_constraint_flag: %d\n", SPS.ptl.general_frame_only_constraint_flag);
            debug_printf("sps_seq_parameter_set_id: %d\n", SPS.sps_seq_parameter_set_id);
   debug_printf("chroma_format_idc: %d\n", SPS.chroma_format_idc);
   debug_printf("separate_colour_plane_flag: %d\n", SPS.separate_colour_plane_flag);
   debug_printf("pic_width_in_luma_samples: %d\n", SPS.pic_width_in_luma_samples);
   debug_printf("pic_height_in_luma_samples: %d\n", SPS.pic_height_in_luma_samples);
   debug_printf("conformance_window_flag: %d\n", SPS.conformance_window_flag);
   debug_printf("conf_win_left_offset: %d\n", SPS.conf_win_left_offset);
   debug_printf("conf_win_right_offset: %d\n", SPS.conf_win_right_offset);
   debug_printf("conf_win_top_offset: %d\n", SPS.conf_win_top_offset);
   debug_printf("conf_win_bottom_offset: %d\n", SPS.conf_win_bottom_offset);
   debug_printf("bit_depth_luma_minus8: %d\n", SPS.bit_depth_luma_minus8);
   debug_printf("bit_depth_chroma_minus8: %d\n", SPS.bit_depth_chroma_minus8);
   debug_printf("log2_max_pic_order_cnt_lsb_minus4: %d\n", SPS.log2_max_pic_order_cnt_lsb_minus4);
   debug_printf("maxPicOrderCntLsb: %d\n", SPS.maxPicOrderCntLsb);
            debug_printf("sps_max_dec_pic_buffering_minus1[%d]: %d\n", (0u), SPS.sps_max_dec_pic_buffering_minus1[0u]);
   debug_printf("sps_max_num_reorder_pics[%d]: %d\n", (0u), SPS.sps_max_num_reorder_pics[0u]);
   debug_printf("sps_max_latency_increase_plus1[%d]: %d\n", (0u), SPS.sps_max_latency_increase_plus1[0u]);
      debug_printf("log2_min_luma_coding_block_size_minus3: %d\n", SPS.log2_min_luma_coding_block_size_minus3);
   debug_printf("log2_diff_max_min_luma_coding_block_size: %d\n", SPS.log2_diff_max_min_luma_coding_block_size);
   debug_printf("log2_min_transform_block_size_minus2: %d\n", SPS.log2_min_transform_block_size_minus2);
   debug_printf("log2_diff_max_min_transform_block_size: %d\n", SPS.log2_diff_max_min_transform_block_size);
   debug_printf("max_transform_hierarchy_depth_inter: %d\n", SPS.max_transform_hierarchy_depth_inter);
   debug_printf("max_transform_hierarchy_depth_intra: %d\n", SPS.max_transform_hierarchy_depth_intra);
   debug_printf("scaling_list_enabled_flag: %d\n", SPS.scaling_list_enabled_flag);
   debug_printf("sps_scaling_list_data_present_flag: %d\n", SPS.sps_scaling_list_data_present_flag);
   debug_printf("amp_enabled_flag: %d\n", SPS.amp_enabled_flag);
   debug_printf("sample_adaptive_offset_enabled_flag: %d\n", SPS.sample_adaptive_offset_enabled_flag);
   debug_printf("pcm_enabled_flag: %d\n", SPS.pcm_enabled_flag);
   debug_printf("pcm_sample_bit_depth_luma_minus1: %d\n", SPS.pcm_sample_bit_depth_luma_minus1);
   debug_printf("pcm_sample_bit_depth_chroma_minus1: %d\n", SPS.pcm_sample_bit_depth_chroma_minus1);
   debug_printf("log2_min_pcm_luma_coding_block_size_minus3: %d\n", SPS.log2_min_pcm_luma_coding_block_size_minus3);
   debug_printf("log2_diff_max_min_pcm_luma_coding_block_size: %d\n", SPS.log2_diff_max_min_pcm_luma_coding_block_size);
   debug_printf("pcm_loop_filter_disabled_flag: %d\n", SPS.pcm_loop_filter_disabled_flag);
   debug_printf("num_short_term_ref_pic_sets: %d\n", SPS.num_short_term_ref_pic_sets);
      for(UINT idx = 0; idx < SPS.num_short_term_ref_pic_sets ; idx++)
   {        
         }
      debug_printf("long_term_ref_pics_present_flag: %d\n", SPS.long_term_ref_pics_present_flag);
   debug_printf("num_long_term_ref_pics_sps: %d\n", SPS.num_long_term_ref_pics_sps);
      for(UINT idx = 0; idx < SPS.num_long_term_ref_pics_sps ; idx++)
   {
      debug_printf("lt_ref_pic_poc_lsb_sps[%d]: %d\n", idx, SPS.lt_ref_pic_poc_lsb_sps[idx]);
      }
      debug_printf("sps_temporal_mvp_enabled_flag: %d\n", SPS.sps_temporal_mvp_enabled_flag);
   debug_printf("strong_intra_smoothing_enabled_flag: %d\n", SPS.strong_intra_smoothing_enabled_flag);
   debug_printf("vui_parameters_present_flag: %d\n", SPS.vui_parameters_present_flag);
   debug_printf("sps_extension_flag: %d\n", SPS.sps_extension_flag);
                  }
   void
   d3d12_video_bitstream_builder_hevc::print_pps(const HevcPicParameterSet& PPS)
   {
      debug_printf("--------------------------------------\nHevcPicParameterSet values below:\n");
   debug_printf("pps_pic_parameter_set_id: %d\n", PPS.pps_pic_parameter_set_id);
   debug_printf("pps_seq_parameter_set_id: %d\n", PPS.pps_seq_parameter_set_id);
   debug_printf("dependent_slice_segments_enabled_flag: %d\n", PPS.dependent_slice_segments_enabled_flag);
   debug_printf("output_flag_present_flag: %d\n", PPS.output_flag_present_flag);
   debug_printf("num_extra_slice_header_bits: %d\n", PPS.num_extra_slice_header_bits);
   debug_printf("sign_data_hiding_enabled_flag: %d\n", PPS.sign_data_hiding_enabled_flag);
   debug_printf("cabac_init_present_flag: %d\n", PPS.cabac_init_present_flag);
   debug_printf("num_ref_idx_l0_default_active_minus1: %d\n", PPS.num_ref_idx_lx_default_active_minus1[0]);
   debug_printf("num_ref_idx_l1_default_active_minus1: %d\n", PPS.num_ref_idx_lx_default_active_minus1[1]);
   debug_printf("init_qp_minus26: %d\n", PPS.init_qp_minus26);
   debug_printf("constrained_intra_pred_flag: %d\n", PPS.constrained_intra_pred_flag);
   debug_printf("transform_skip_enabled_flag: %d\n", PPS.transform_skip_enabled_flag);
   debug_printf("cu_qp_delta_enabled_flag: %d\n", PPS.cu_qp_delta_enabled_flag);
   debug_printf("diff_cu_qp_delta_depth: %d\n", PPS.diff_cu_qp_delta_depth);
   debug_printf("pps_cb_qp_offset: %d\n", PPS.pps_cb_qp_offset);
   debug_printf("pps_cr_qp_offset: %d\n", PPS.pps_cr_qp_offset);
   debug_printf("pps_slice_chroma_qp_offsets_present_flag: %d\n", PPS.pps_slice_chroma_qp_offsets_present_flag);
   debug_printf("weighted_pred_flag: %d\n", PPS.weighted_pred_flag);
   debug_printf("weighted_bipred_flag: %d\n", PPS.weighted_bipred_flag);
   debug_printf("transquant_bypass_enabled_flag: %d\n", PPS.transquant_bypass_enabled_flag);
   debug_printf("tiles_enabled_flag: %d\n", PPS.tiles_enabled_flag);
   debug_printf("entropy_coding_sync_enabled_flag: %d\n", PPS.entropy_coding_sync_enabled_flag);
   debug_printf("num_tile_columns_minus1: %d\n", PPS.num_tile_columns_minus1);
   debug_printf("num_tile_rows_minus1: %d\n", PPS.num_tile_rows_minus1);
   debug_printf("uniform_spacing_flag: %d\n", PPS.uniform_spacing_flag);
   debug_printf("column_width_minus1[0]: %d\n", PPS.column_width_minus1[0]); // no tiles in D3D12)
   debug_printf("row_height_minus1[0]: %d\n", PPS.row_height_minus1[0]); // no tiles in D3D12)
   debug_printf("loop_filter_across_tiles_enabled_flag: %d\n", PPS.loop_filter_across_tiles_enabled_flag);
   debug_printf("pps_loop_filter_across_slices_enabled_flag: %d\n", PPS.pps_loop_filter_across_slices_enabled_flag);
   debug_printf("deblocking_filter_control_present_flag: %d\n", PPS.deblocking_filter_control_present_flag);
   debug_printf("deblocking_filter_override_enabled_flag: %d\n", PPS.deblocking_filter_override_enabled_flag);
   debug_printf("pps_deblocking_filter_disabled_flag: %d\n", PPS.pps_deblocking_filter_disabled_flag);
   debug_printf("pps_beta_offset_div2: %d\n", PPS.pps_beta_offset_div2);
   debug_printf("pps_tc_offset_div2: %d\n", PPS.pps_tc_offset_div2);
   debug_printf("pps_scaling_list_data_present_flag: %d\n", PPS.pps_scaling_list_data_present_flag);
   debug_printf("lists_modification_present_flag: %d\n", PPS.lists_modification_present_flag);
   debug_printf("log2_parallel_merge_level_minus2: %d\n", PPS.log2_parallel_merge_level_minus2);
   debug_printf("slice_segment_header_extension_present_flag: %d\n", PPS.slice_segment_header_extension_present_flag);
   debug_printf("pps_extension_flag: %d\n", PPS.pps_extension_flag);
   debug_printf("pps_extension_data_flag: %d\n", PPS.pps_extension_data_flag);
      }
      void
   d3d12_video_bitstream_builder_hevc::print_rps(const HevcSeqParameterSet* sps, UINT stRpsIdx)
   {
                                 if(rps->inter_ref_pic_set_prediction_flag)
   {
      debug_printf("delta_idx_minus1: %d\n", rps->delta_idx_minus1);                
   debug_printf("delta_rps_sign: %d\n", rps->delta_rps_sign);
   debug_printf("abs_delta_rps_minus1: %d\n", rps->abs_delta_rps_minus1);
   debug_printf("num_negative_pics: %d\n", rps->num_negative_pics);        
            int32_t RefRpsIdx = stRpsIdx - 1 - rps->delta_idx_minus1;
   const HEVCReferencePictureSet* rpsRef = &(sps->rpsShortTerm[RefRpsIdx]);
   auto numberOfPictures = rpsRef->num_negative_pics + rpsRef->num_positive_pics;
   for (uint8_t j = 0; j <= numberOfPictures; j++) {
      debug_printf("used_by_curr_pic_flag[%d]: %d\n", j, rps->used_by_curr_pic_flag[j]);
   if (!rps->used_by_curr_pic_flag[j]) {
               }
   else
   {
      debug_printf("num_negative_pics: %d\n", rps->num_negative_pics);        
   for (uint8_t i = 0; i < rps->num_negative_pics; i++) {
      debug_printf("delta_poc_s0_minus1[%d]: %d\n", i, rps->delta_poc_s0_minus1[i]);
               debug_printf("num_positive_pics: %d\n", rps->num_positive_pics);
   for (int32_t i = 0; i < rps->num_positive_pics; i++) {
      debug_printf("delta_poc_s1_minus1[%d]: %d\n", i, rps->delta_poc_s1_minus1[i]);
                     }
