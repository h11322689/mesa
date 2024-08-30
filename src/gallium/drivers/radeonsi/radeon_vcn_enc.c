   /**************************************************************************
   *
   * Copyright 2017 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   *
   **************************************************************************/
      #include "radeon_vcn_enc.h"
   #include "ac_vcn_enc_av1_default_cdf.h"
      #include "pipe/p_video_codec.h"
   #include "radeon_video.h"
   #include "radeonsi/si_pipe.h"
   #include "util/u_memory.h"
   #include "util/u_video.h"
   #include "vl/vl_video_buffer.h"
      #include <stdio.h>
      static const unsigned index_to_shifts[4] = {24, 16, 8, 0};
      /* set quality modes from the input */
   static void radeon_vcn_enc_quality_modes(struct radeon_encoder *enc,
         {
               p->preset_mode = in->preset_mode > RENCODE_PRESET_MODE_HIGH_QUALITY
                  if (u_reduce_video_profile(enc->base.profile) != PIPE_VIDEO_FORMAT_AV1 &&
      p->preset_mode == RENCODE_PRESET_MODE_HIGH_QUALITY)
         p->pre_encode_mode = in->pre_encode_mode ? RENCODE_PREENCODE_MODE_4X
            }
      /* to process invalid frame rate */
   static void radeon_vcn_enc_invalid_frame_rate(uint32_t *den, uint32_t *num)
   {
      if (*den == 0 || *num == 0) {
      *den = 1;
         }
      static uint32_t radeon_vcn_per_frame_integer(uint32_t bitrate, uint32_t den, uint32_t num)
   {
                  }
      static uint32_t radeon_vcn_per_frame_frac(uint32_t bitrate, uint32_t den, uint32_t num)
   {
      uint64_t rate_den = (uint64_t)bitrate * (uint64_t)den;
               }
      static void radeon_vcn_enc_h264_get_cropping_param(struct radeon_encoder *enc,
         {
      if (pic->seq.enc_frame_cropping_flag) {
      enc->enc_pic.crop_left = pic->seq.enc_frame_crop_left_offset;
   enc->enc_pic.crop_right = pic->seq.enc_frame_crop_right_offset;
   enc->enc_pic.crop_top = pic->seq.enc_frame_crop_top_offset;
      } else {
      enc->enc_pic.crop_left = 0;
   enc->enc_pic.crop_right = (align(enc->base.width, 16) - enc->base.width) / 2;
   enc->enc_pic.crop_top = 0;
         }
      static void radeon_vcn_enc_h264_get_dbk_param(struct radeon_encoder *enc,
         {
      enc->enc_pic.h264_deblock.disable_deblocking_filter_idc =
         enc->enc_pic.h264_deblock.alpha_c0_offset_div2 = pic->dbk.alpha_c0_offset_div2;
   enc->enc_pic.h264_deblock.beta_offset_div2 = pic->dbk.beta_offset_div2;
   enc->enc_pic.h264_deblock.cb_qp_offset = pic->pic_ctrl.chroma_qp_index_offset;
      }
      static void radeon_vcn_enc_h264_get_spec_misc_param(struct radeon_encoder *enc,
         {
      enc->enc_pic.spec_misc.profile_idc = u_get_h264_profile_idc(enc->base.profile);
   if (enc->enc_pic.spec_misc.profile_idc >= PIPE_VIDEO_PROFILE_MPEG4_AVC_MAIN &&
               else
            enc->enc_pic.spec_misc.cabac_init_idc = enc->enc_pic.spec_misc.cabac_enable ?
         enc->enc_pic.spec_misc.deblocking_filter_control_present_flag =
         enc->enc_pic.spec_misc.redundant_pic_cnt_present_flag =
            }
      static void radeon_vcn_enc_h264_get_rc_param(struct radeon_encoder *enc,
         {
               enc->enc_pic.num_temporal_layers = pic->seq.num_temporal_layers ? pic->seq.num_temporal_layers : 1;
   enc->enc_pic.temporal_id = 0;
   for (int i = 0; i < enc->enc_pic.num_temporal_layers; i++) {
      enc->enc_pic.rc_layer_init[i].target_bit_rate = pic->rate_ctrl[i].target_bitrate;
   enc->enc_pic.rc_layer_init[i].peak_bit_rate = pic->rate_ctrl[i].peak_bitrate;
   frame_rate_den = pic->rate_ctrl[i].frame_rate_den;
   frame_rate_num = pic->rate_ctrl[i].frame_rate_num;
   radeon_vcn_enc_invalid_frame_rate(&frame_rate_den, &frame_rate_num);
   enc->enc_pic.rc_layer_init[i].frame_rate_den = frame_rate_den;
   enc->enc_pic.rc_layer_init[i].frame_rate_num = frame_rate_num;
   enc->enc_pic.rc_layer_init[i].vbv_buffer_size = pic->rate_ctrl[i].vbv_buffer_size;
   enc->enc_pic.rc_layer_init[i].avg_target_bits_per_picture =
      radeon_vcn_per_frame_integer(pic->rate_ctrl[i].target_bitrate,
            enc->enc_pic.rc_layer_init[i].peak_bits_per_picture_integer =
      radeon_vcn_per_frame_integer(pic->rate_ctrl[i].peak_bitrate,
            enc->enc_pic.rc_layer_init[i].peak_bits_per_picture_fractional =
      radeon_vcn_per_frame_frac(pic->rate_ctrl[i].peak_bitrate,
         }
   enc->enc_pic.rc_session_init.vbv_buffer_level = pic->rate_ctrl[0].vbv_buf_lv;
   enc->enc_pic.rc_per_pic.qp = pic->quant_i_frames;
   enc->enc_pic.rc_per_pic.min_qp_app = pic->rate_ctrl[0].min_qp;
   enc->enc_pic.rc_per_pic.max_qp_app = pic->rate_ctrl[0].max_qp ?
         enc->enc_pic.rc_per_pic.enabled_filler_data = pic->rate_ctrl[0].fill_data_enable;
   enc->enc_pic.rc_per_pic.skip_frame_enable = pic->rate_ctrl[0].skip_frame_enable;
            switch (pic->rate_ctrl[0].rate_ctrl_method) {
      case PIPE_H2645_ENC_RATE_CONTROL_METHOD_DISABLE:
      enc->enc_pic.rc_session_init.rate_control_method = RENCODE_RATE_CONTROL_METHOD_NONE;
      case PIPE_H2645_ENC_RATE_CONTROL_METHOD_CONSTANT_SKIP:
   case PIPE_H2645_ENC_RATE_CONTROL_METHOD_CONSTANT:
      enc->enc_pic.rc_session_init.rate_control_method = RENCODE_RATE_CONTROL_METHOD_CBR;
      case PIPE_H2645_ENC_RATE_CONTROL_METHOD_VARIABLE_SKIP:
   case PIPE_H2645_ENC_RATE_CONTROL_METHOD_VARIABLE:
      enc->enc_pic.rc_session_init.rate_control_method =
            default:
      }
      }
      static void radeon_vcn_enc_h264_get_vui_param(struct radeon_encoder *enc,
         {
      enc->enc_pic.vui_info.vui_parameters_present_flag =
         enc->enc_pic.vui_info.flags.aspect_ratio_info_present_flag =
         enc->enc_pic.vui_info.flags.timing_info_present_flag =
         enc->enc_pic.vui_info.flags.video_signal_type_present_flag =
         enc->enc_pic.vui_info.flags.colour_description_present_flag =
         enc->enc_pic.vui_info.flags.chroma_loc_info_present_flag =
         enc->enc_pic.vui_info.aspect_ratio_idc = pic->seq.aspect_ratio_idc;
   enc->enc_pic.vui_info.sar_width = pic->seq.sar_width;
   enc->enc_pic.vui_info.sar_height = pic->seq.sar_height;
   enc->enc_pic.vui_info.num_units_in_tick = pic->seq.num_units_in_tick;
   enc->enc_pic.vui_info.time_scale = pic->seq.time_scale;
   enc->enc_pic.vui_info.video_format = pic->seq.video_format;
   enc->enc_pic.vui_info.video_full_range_flag = pic->seq.video_full_range_flag;
   enc->enc_pic.vui_info.colour_primaries = pic->seq.colour_primaries;
   enc->enc_pic.vui_info.transfer_characteristics = pic->seq.transfer_characteristics;
   enc->enc_pic.vui_info.matrix_coefficients = pic->seq.matrix_coefficients;
   enc->enc_pic.vui_info.chroma_sample_loc_type_top_field =
         enc->enc_pic.vui_info.chroma_sample_loc_type_bottom_field =
            }
      /* only checking the first slice to get num of mbs in slice to
   * determine the number of slices in this frame, only fixed MB mode
   * is supported now, the last slice in frame could have less number of
   * MBs.
   */
   static void radeon_vcn_enc_h264_get_slice_ctrl_param(struct radeon_encoder *enc,
         {
               width_in_mb = PIPE_ALIGN_IN_BLOCK_SIZE(enc->base.width, PIPE_H264_MB_SIZE);
            if (pic->slices_descriptors[0].num_macroblocks >= width_in_mb * height_in_mb ||
      pic->slices_descriptors[0].num_macroblocks == 0)
      else
               }
      static void radeon_vcn_enc_get_output_format_param(struct radeon_encoder *enc, bool full_range)
   {
      switch (enc->enc_pic.bit_depth_luma_minus8) {
   case 2: /* 10 bits */
      enc->enc_pic.enc_output_format.output_color_volume = RENCODE_COLOR_VOLUME_G22_BT709;
   enc->enc_pic.enc_output_format.output_color_range = full_range ?
         enc->enc_pic.enc_output_format.output_chroma_location = RENCODE_CHROMA_LOCATION_INTERSTITIAL;
   enc->enc_pic.enc_output_format.output_color_bit_depth = RENCODE_COLOR_BIT_DEPTH_10_BIT;
      default: /* 8 bits */
      enc->enc_pic.enc_output_format.output_color_volume = RENCODE_COLOR_VOLUME_G22_BT709;
   enc->enc_pic.enc_output_format.output_color_range = full_range ?
         enc->enc_pic.enc_output_format.output_chroma_location = RENCODE_CHROMA_LOCATION_INTERSTITIAL;
   enc->enc_pic.enc_output_format.output_color_bit_depth = RENCODE_COLOR_BIT_DEPTH_8_BIT;
         }
      static void radeon_vcn_enc_get_input_format_param(struct radeon_encoder *enc,
         {
      switch (pic_base->input_format) {
   case PIPE_FORMAT_P010:
      enc->enc_pic.enc_input_format.input_color_bit_depth = RENCODE_COLOR_BIT_DEPTH_10_BIT;
   enc->enc_pic.enc_input_format.input_color_packing_format = RENCODE_COLOR_PACKING_FORMAT_P010;
   enc->enc_pic.enc_input_format.input_chroma_subsampling = RENCODE_CHROMA_SUBSAMPLING_4_2_0;
   enc->enc_pic.enc_input_format.input_color_space = RENCODE_COLOR_SPACE_YUV;
      case PIPE_FORMAT_B8G8R8A8_UNORM:
   case PIPE_FORMAT_B8G8R8X8_UNORM:
      enc->enc_pic.enc_input_format.input_color_bit_depth = RENCODE_COLOR_BIT_DEPTH_8_BIT;
   enc->enc_pic.enc_input_format.input_chroma_subsampling = RENCODE_CHROMA_SUBSAMPLING_4_4_4;
   enc->enc_pic.enc_input_format.input_color_packing_format = RENCODE_COLOR_PACKING_FORMAT_A8R8G8B8;
   enc->enc_pic.enc_input_format.input_color_space = RENCODE_COLOR_SPACE_RGB;
      case PIPE_FORMAT_R8G8B8A8_UNORM:
   case PIPE_FORMAT_R8G8B8X8_UNORM:
      enc->enc_pic.enc_input_format.input_color_bit_depth = RENCODE_COLOR_BIT_DEPTH_8_BIT;
   enc->enc_pic.enc_input_format.input_chroma_subsampling = RENCODE_CHROMA_SUBSAMPLING_4_4_4;
   enc->enc_pic.enc_input_format.input_color_packing_format = RENCODE_COLOR_PACKING_FORMAT_A8B8G8R8;
   enc->enc_pic.enc_input_format.input_color_space = RENCODE_COLOR_SPACE_RGB;
      case PIPE_FORMAT_NV12: /* FALL THROUGH */
   default:
      enc->enc_pic.enc_input_format.input_color_bit_depth = RENCODE_COLOR_BIT_DEPTH_8_BIT;
   enc->enc_pic.enc_input_format.input_color_packing_format = RENCODE_COLOR_PACKING_FORMAT_NV12;
   enc->enc_pic.enc_input_format.input_chroma_subsampling = RENCODE_CHROMA_SUBSAMPLING_4_2_0;
   enc->enc_pic.enc_input_format.input_color_space = RENCODE_COLOR_SPACE_YUV;
            enc->enc_pic.enc_input_format.input_color_volume = RENCODE_COLOR_VOLUME_G22_BT709;
   enc->enc_pic.enc_input_format.input_color_range = pic_base->input_full_range ?
      RENCODE_COLOR_RANGE_FULL : RENCODE_COLOR_RANGE_STUDIO;
      }
      static void radeon_vcn_enc_h264_get_param(struct radeon_encoder *enc,
         {
      enc->enc_pic.picture_type = pic->picture_type;
   enc->enc_pic.bit_depth_luma_minus8 = 0;
   enc->enc_pic.bit_depth_chroma_minus8 = 0;
   radeon_vcn_enc_quality_modes(enc, &pic->quality_modes);
   enc->enc_pic.frame_num = pic->frame_num;
   enc->enc_pic.pic_order_cnt = pic->pic_order_cnt;
   enc->enc_pic.pic_order_cnt_type = pic->seq.pic_order_cnt_type;
   enc->enc_pic.ref_idx_l0 = pic->ref_idx_l0_list[0];
   enc->enc_pic.ref_idx_l0_is_ltr = pic->l0_is_long_term[0];
   enc->enc_pic.ref_idx_l1 = pic->ref_idx_l1_list[0];
   enc->enc_pic.ref_idx_l1_is_ltr = pic->l1_is_long_term[0];
   enc->enc_pic.not_referenced = pic->not_referenced;
   enc->enc_pic.is_idr = (pic->picture_type == PIPE_H2645_ENC_PICTURE_TYPE_IDR);
   enc->enc_pic.is_ltr = pic->is_ltr;
   enc->enc_pic.ltr_idx = pic->is_ltr ? pic->ltr_index : 0;
   radeon_vcn_enc_h264_get_cropping_param(enc, pic);
   radeon_vcn_enc_h264_get_dbk_param(enc, pic);
   radeon_vcn_enc_h264_get_rc_param(enc, pic);
   radeon_vcn_enc_h264_get_spec_misc_param(enc, pic);
   radeon_vcn_enc_h264_get_vui_param(enc, pic);
   radeon_vcn_enc_h264_get_slice_ctrl_param(enc, pic);
   radeon_vcn_enc_get_input_format_param(enc, &pic->base);
      }
      static void radeon_vcn_enc_hevc_get_cropping_param(struct radeon_encoder *enc,
         {
      if (pic->seq.conformance_window_flag) {
      enc->enc_pic.crop_left = pic->seq.conf_win_left_offset;
   enc->enc_pic.crop_right = pic->seq.conf_win_right_offset;
   enc->enc_pic.crop_top = pic->seq.conf_win_top_offset;
      } else {
      enc->enc_pic.crop_left = 0;
   enc->enc_pic.crop_right = (align(enc->base.width, 16) - enc->base.width) / 2;
   enc->enc_pic.crop_top = 0;
         }
      static void radeon_vcn_enc_hevc_get_dbk_param(struct radeon_encoder *enc,
         {
      enc->enc_pic.hevc_deblock.loop_filter_across_slices_enabled =
         enc->enc_pic.hevc_deblock.deblocking_filter_disabled =
         enc->enc_pic.hevc_deblock.beta_offset_div2 = pic->slice.slice_beta_offset_div2;
   enc->enc_pic.hevc_deblock.tc_offset_div2 = pic->slice.slice_tc_offset_div2;
   enc->enc_pic.hevc_deblock.cb_qp_offset = pic->slice.slice_cb_qp_offset;
      }
      static void radeon_vcn_enc_hevc_get_spec_misc_param(struct radeon_encoder *enc,
         {
      enc->enc_pic.hevc_spec_misc.log2_min_luma_coding_block_size_minus3 =
         enc->enc_pic.hevc_spec_misc.amp_disabled = !pic->seq.amp_enabled_flag;
   enc->enc_pic.hevc_spec_misc.strong_intra_smoothing_enabled =
         enc->enc_pic.hevc_spec_misc.constrained_intra_pred_flag =
         enc->enc_pic.hevc_spec_misc.cabac_init_flag = pic->slice.cabac_init_flag;
   enc->enc_pic.hevc_spec_misc.half_pel_enabled = 1;
      }
      static void radeon_vcn_enc_hevc_get_rc_param(struct radeon_encoder *enc,
         {
               enc->enc_pic.rc_layer_init[0].target_bit_rate = pic->rc.target_bitrate;
   enc->enc_pic.rc_layer_init[0].peak_bit_rate = pic->rc.peak_bitrate;
   frame_rate_den = pic->rc.frame_rate_den;
   frame_rate_num = pic->rc.frame_rate_num;
   radeon_vcn_enc_invalid_frame_rate(&frame_rate_den, &frame_rate_num);
   enc->enc_pic.rc_layer_init[0].frame_rate_den = frame_rate_den;
   enc->enc_pic.rc_layer_init[0].frame_rate_num = frame_rate_num;
   enc->enc_pic.rc_layer_init[0].vbv_buffer_size = pic->rc.vbv_buffer_size;
   enc->enc_pic.rc_layer_init[0].avg_target_bits_per_picture =
      radeon_vcn_per_frame_integer(pic->rc.target_bitrate,
            enc->enc_pic.rc_layer_init[0].peak_bits_per_picture_integer =
      radeon_vcn_per_frame_integer(pic->rc.peak_bitrate,
            enc->enc_pic.rc_layer_init[0].peak_bits_per_picture_fractional =
      radeon_vcn_per_frame_frac(pic->rc.peak_bitrate,
            enc->enc_pic.rc_session_init.vbv_buffer_level = pic->rc.vbv_buf_lv;
   enc->enc_pic.rc_per_pic.qp = pic->rc.quant_i_frames;
   enc->enc_pic.rc_per_pic.min_qp_app = pic->rc.min_qp;
   enc->enc_pic.rc_per_pic.max_qp_app = pic->rc.max_qp ? pic->rc.max_qp : 51;
   enc->enc_pic.rc_per_pic.enabled_filler_data = pic->rc.fill_data_enable;
   enc->enc_pic.rc_per_pic.skip_frame_enable = pic->rc.skip_frame_enable;
   enc->enc_pic.rc_per_pic.enforce_hrd = pic->rc.enforce_hrd;
   switch (pic->rc.rate_ctrl_method) {
      case PIPE_H2645_ENC_RATE_CONTROL_METHOD_DISABLE:
      enc->enc_pic.rc_session_init.rate_control_method = RENCODE_RATE_CONTROL_METHOD_NONE;
      case PIPE_H2645_ENC_RATE_CONTROL_METHOD_CONSTANT_SKIP:
   case PIPE_H2645_ENC_RATE_CONTROL_METHOD_CONSTANT:
      enc->enc_pic.rc_session_init.rate_control_method = RENCODE_RATE_CONTROL_METHOD_CBR;
      case PIPE_H2645_ENC_RATE_CONTROL_METHOD_VARIABLE_SKIP:
   case PIPE_H2645_ENC_RATE_CONTROL_METHOD_VARIABLE:
      enc->enc_pic.rc_session_init.rate_control_method =
            default:
      }
      }
      static void radeon_vcn_enc_hevc_get_vui_param(struct radeon_encoder *enc,
         {
      enc->enc_pic.vui_info.vui_parameters_present_flag = pic->seq.vui_parameters_present_flag;
   enc->enc_pic.vui_info.flags.aspect_ratio_info_present_flag =
         enc->enc_pic.vui_info.flags.timing_info_present_flag =
         enc->enc_pic.vui_info.flags.video_signal_type_present_flag =
         enc->enc_pic.vui_info.flags.colour_description_present_flag =
         enc->enc_pic.vui_info.flags.chroma_loc_info_present_flag =
         enc->enc_pic.vui_info.aspect_ratio_idc = pic->seq.aspect_ratio_idc;
   enc->enc_pic.vui_info.sar_width = pic->seq.sar_width;
   enc->enc_pic.vui_info.sar_height = pic->seq.sar_height;
   enc->enc_pic.vui_info.num_units_in_tick = pic->seq.num_units_in_tick;
   enc->enc_pic.vui_info.time_scale = pic->seq.time_scale;
   enc->enc_pic.vui_info.video_format = pic->seq.video_format;
   enc->enc_pic.vui_info.video_full_range_flag = pic->seq.video_full_range_flag;
   enc->enc_pic.vui_info.colour_primaries = pic->seq.colour_primaries;
   enc->enc_pic.vui_info.transfer_characteristics = pic->seq.transfer_characteristics;
   enc->enc_pic.vui_info.matrix_coefficients = pic->seq.matrix_coefficients;
   enc->enc_pic.vui_info.chroma_sample_loc_type_top_field =
         enc->enc_pic.vui_info.chroma_sample_loc_type_bottom_field =
      }
      /* only checking the first slice to get num of ctbs in slice to
   * determine the number of slices in this frame, only fixed CTB mode
   * is supported now, the last slice in frame could have less number of
   * ctbs.
   */
   static void radeon_vcn_enc_hevc_get_slice_ctrl_param(struct radeon_encoder *enc,
         {
               width_in_ctb = PIPE_ALIGN_IN_BLOCK_SIZE(pic->seq.pic_width_in_luma_samples,
         height_in_ctb = PIPE_ALIGN_IN_BLOCK_SIZE(pic->seq.pic_height_in_luma_samples,
            if (pic->slices_descriptors[0].num_ctu_in_slice >= width_in_ctb * height_in_ctb ||
      pic->slices_descriptors[0].num_ctu_in_slice == 0)
      else
            enc->enc_pic.hevc_slice_ctrl.fixed_ctbs_per_slice.num_ctbs_per_slice =
            enc->enc_pic.hevc_slice_ctrl.fixed_ctbs_per_slice.num_ctbs_per_slice_segment =
      }
      static void radeon_vcn_enc_hevc_get_param(struct radeon_encoder *enc,
         {
      enc->enc_pic.picture_type = pic->picture_type;
   enc->enc_pic.frame_num = pic->frame_num;
   radeon_vcn_enc_quality_modes(enc, &pic->quality_modes);
   enc->enc_pic.pic_order_cnt_type = pic->pic_order_cnt_type;
   enc->enc_pic.ref_idx_l0 = pic->ref_idx_l0_list[0];
   enc->enc_pic.ref_idx_l1 = pic->ref_idx_l1_list[0];
   enc->enc_pic.not_referenced = pic->not_referenced;
   enc->enc_pic.is_idr = (pic->picture_type == PIPE_H2645_ENC_PICTURE_TYPE_IDR);
   radeon_vcn_enc_hevc_get_cropping_param(enc, pic);
   enc->enc_pic.general_tier_flag = pic->seq.general_tier_flag;
   enc->enc_pic.general_profile_idc = pic->seq.general_profile_idc;
   enc->enc_pic.general_level_idc = pic->seq.general_level_idc;
   /* use fixed value for max_poc until new feature added */
   enc->enc_pic.max_poc = 16;
   enc->enc_pic.log2_max_poc = 4;
   enc->enc_pic.num_temporal_layers = 1;
   enc->enc_pic.pic_order_cnt = pic->pic_order_cnt % enc->enc_pic.max_poc;
   enc->enc_pic.chroma_format_idc = pic->seq.chroma_format_idc;
   enc->enc_pic.pic_width_in_luma_samples = pic->seq.pic_width_in_luma_samples;
   enc->enc_pic.pic_height_in_luma_samples = pic->seq.pic_height_in_luma_samples;
   enc->enc_pic.log2_diff_max_min_luma_coding_block_size =
         enc->enc_pic.log2_min_transform_block_size_minus2 =
         enc->enc_pic.log2_diff_max_min_transform_block_size =
            /* To fix incorrect hardcoded values set by player
   * log2_diff_max_min_luma_coding_block_size = log2(64) - (log2_min_luma_coding_block_size_minus3 + 3)
   * max_transform_hierarchy_depth_inter = log2_diff_max_min_luma_coding_block_size + 1
   * max_transform_hierarchy_depth_intra = log2_diff_max_min_luma_coding_block_size + 1
   */
   enc->enc_pic.max_transform_hierarchy_depth_inter =
         enc->enc_pic.max_transform_hierarchy_depth_intra =
            enc->enc_pic.log2_parallel_merge_level_minus2 = pic->pic.log2_parallel_merge_level_minus2;
   enc->enc_pic.bit_depth_luma_minus8 = pic->seq.bit_depth_luma_minus8;
   enc->enc_pic.bit_depth_chroma_minus8 = pic->seq.bit_depth_chroma_minus8;
   enc->enc_pic.nal_unit_type = pic->pic.nal_unit_type;
   enc->enc_pic.max_num_merge_cand = pic->slice.max_num_merge_cand;
   enc->enc_pic.sample_adaptive_offset_enabled_flag =
         enc->enc_pic.pcm_enabled_flag = pic->seq.pcm_enabled_flag;
   enc->enc_pic.sps_temporal_mvp_enabled_flag = pic->seq.sps_temporal_mvp_enabled_flag;
   radeon_vcn_enc_hevc_get_spec_misc_param(enc, pic);
   radeon_vcn_enc_hevc_get_dbk_param(enc, pic);
   radeon_vcn_enc_hevc_get_rc_param(enc, pic);
   radeon_vcn_enc_hevc_get_vui_param(enc, pic);
   radeon_vcn_enc_hevc_get_slice_ctrl_param(enc, pic);
   radeon_vcn_enc_get_input_format_param(enc, &pic->base);
      }
      static void radeon_vcn_enc_av1_get_spec_misc_param(struct radeon_encoder *enc,
         {
      enc->enc_pic.av1_spec_misc.cdef_mode = pic->seq.seq_bits.enable_cdef;
   enc->enc_pic.av1_spec_misc.disable_cdf_update = pic->disable_cdf_update;
   enc->enc_pic.av1_spec_misc.disable_frame_end_update_cdf = pic->disable_frame_end_update_cdf;
   enc->enc_pic.av1_spec_misc.num_tiles_per_picture = pic->num_tiles_in_pic;
            if (enc->enc_pic.disable_screen_content_tools) {
      enc->enc_pic.force_integer_mv  = 0;
               if (enc->enc_pic.force_integer_mv)
         else
      }
      static void radeon_vcn_enc_av1_timing_info(struct radeon_encoder *enc,
         {
      if (pic->seq.seq_bits.timing_info_present_flag)
   {
      enc->enc_pic.av1_timing_info.num_units_in_display_tick =
         enc->enc_pic.av1_timing_info.time_scale = pic->seq.time_scale;
   enc->enc_pic.av1_timing_info.num_tick_per_picture_minus1 =
         }
      static void radeon_vcn_enc_av1_color_description(struct radeon_encoder *enc,
         {
      if (pic->seq.seq_bits.color_description_present_flag)
   {
      enc->enc_pic.av1_color_description.color_primaries = pic->seq.color_config.color_primaries;
   enc->enc_pic.av1_color_description.transfer_characteristics = pic->seq.color_config.transfer_characteristics;
      }
   enc->enc_pic.av1_color_description.color_range = pic->seq.color_config.color_range;
      }
      static void radeon_vcn_enc_av1_get_rc_param(struct radeon_encoder *enc,
         {
               for (int i = 0; i < ARRAY_SIZE(enc->enc_pic.rc_layer_init); i++) {
      enc->enc_pic.rc_layer_init[i].target_bit_rate = pic->rc[i].target_bitrate;
   enc->enc_pic.rc_layer_init[i].peak_bit_rate = pic->rc[i].peak_bitrate;
   frame_rate_den = pic->rc[i].frame_rate_den;
   frame_rate_num = pic->rc[i].frame_rate_num;
   radeon_vcn_enc_invalid_frame_rate(&frame_rate_den, &frame_rate_num);
   enc->enc_pic.rc_layer_init[i].frame_rate_den = frame_rate_den;
   enc->enc_pic.rc_layer_init[i].frame_rate_num = frame_rate_num;
   enc->enc_pic.rc_layer_init[i].vbv_buffer_size = pic->rc[i].vbv_buffer_size;
   enc->enc_pic.rc_layer_init[i].avg_target_bits_per_picture =
      radeon_vcn_per_frame_integer(pic->rc[i].target_bitrate,
            enc->enc_pic.rc_layer_init[i].peak_bits_per_picture_integer =
      radeon_vcn_per_frame_integer(pic->rc[i].peak_bitrate,
            enc->enc_pic.rc_layer_init[i].peak_bits_per_picture_fractional =
      radeon_vcn_per_frame_frac(pic->rc[i].peak_bitrate,
         }
   enc->enc_pic.rc_session_init.vbv_buffer_level = pic->rc[0].vbv_buf_lv;
   enc->enc_pic.rc_per_pic.qp = pic->rc[0].qp;
   enc->enc_pic.rc_per_pic.min_qp_app = pic->rc[0].min_qp ? pic->rc[0].min_qp : 1;
   enc->enc_pic.rc_per_pic.max_qp_app = pic->rc[0].max_qp ? pic->rc[0].max_qp : 255;
   enc->enc_pic.rc_per_pic.enabled_filler_data = pic->rc[0].fill_data_enable;
   enc->enc_pic.rc_per_pic.skip_frame_enable = pic->rc[0].skip_frame_enable;
   enc->enc_pic.rc_per_pic.enforce_hrd = pic->rc[0].enforce_hrd;
   switch (pic->rc[0].rate_ctrl_method) {
      case PIPE_H2645_ENC_RATE_CONTROL_METHOD_DISABLE:
      enc->enc_pic.rc_session_init.rate_control_method = RENCODE_RATE_CONTROL_METHOD_NONE;
      case PIPE_H2645_ENC_RATE_CONTROL_METHOD_CONSTANT_SKIP:
   case PIPE_H2645_ENC_RATE_CONTROL_METHOD_CONSTANT:
      enc->enc_pic.rc_session_init.rate_control_method = RENCODE_RATE_CONTROL_METHOD_CBR;
      case PIPE_H2645_ENC_RATE_CONTROL_METHOD_VARIABLE_SKIP:
   case PIPE_H2645_ENC_RATE_CONTROL_METHOD_VARIABLE:
      enc->enc_pic.rc_session_init.rate_control_method =
            default:
      }
      }
      static void radeon_vcn_enc_av1_get_param(struct radeon_encoder *enc,
         {
      struct radeon_enc_pic *enc_pic = &enc->enc_pic;
   enc_pic->frame_type = pic->frame_type;
   enc_pic->frame_num = pic->frame_num;
   enc_pic->bit_depth_luma_minus8 = enc_pic->bit_depth_chroma_minus8 =
         enc_pic->pic_width_in_luma_samples = pic->seq.pic_width_in_luma_samples;
   enc_pic->pic_height_in_luma_samples = pic->seq.pic_height_in_luma_samples;
   enc_pic->general_profile_idc = pic->seq.profile;
   enc_pic->general_level_idc = pic->seq.level;
            enc_pic->num_temporal_layers =
                  /* 1, 2 layer needs 1 reference, and 3, 4 layer needs 2 references */
   enc->base.max_references = (enc_pic->num_temporal_layers + 1) / 2;
   radeon_vcn_enc_quality_modes(enc, &pic->quality_modes);
   enc_pic->frame_id_numbers_present = pic->seq.seq_bits.frame_id_number_present_flag;
   enc_pic->enable_error_resilient_mode = pic->error_resilient_mode;
   enc_pic->force_integer_mv = pic->force_integer_mv;
   enc_pic->enable_order_hint = pic->seq.seq_bits.enable_order_hint;
   enc_pic->order_hint_bits = pic->seq.order_hint_bits;
   enc_pic->enable_render_size = pic->enable_render_size;
   enc_pic->render_width = pic->render_width;
   enc_pic->render_height = pic->render_height;
   enc_pic->enable_color_description = pic->seq.seq_bits.color_description_present_flag;
   enc_pic->timing_info_present = pic->seq.seq_bits.timing_info_present_flag;
   enc_pic->timing_info_equal_picture_interval = pic->seq.seq_bits.equal_picture_interval;
   enc_pic->disable_screen_content_tools = !pic->allow_screen_content_tools;
   enc_pic->is_obu_frame = pic->enable_frame_obu;
            radeon_vcn_enc_av1_get_spec_misc_param(enc, pic);
   radeon_vcn_enc_av1_timing_info(enc, pic);
   radeon_vcn_enc_av1_color_description(enc, pic);
   radeon_vcn_enc_av1_get_rc_param(enc, pic);
   radeon_vcn_enc_get_input_format_param(enc, &pic->base);
      }
      static void radeon_vcn_enc_get_param(struct radeon_encoder *enc, struct pipe_picture_desc *picture)
   {
      if (u_reduce_video_profile(picture->profile) == PIPE_VIDEO_FORMAT_MPEG4_AVC)
         else if (u_reduce_video_profile(picture->profile) == PIPE_VIDEO_FORMAT_HEVC)
         else if (u_reduce_video_profile(picture->profile) == PIPE_VIDEO_FORMAT_AV1)
      }
      static void flush(struct radeon_encoder *enc)
   {
         }
      static void radeon_enc_flush(struct pipe_video_codec *encoder)
   {
      struct radeon_encoder *enc = (struct radeon_encoder *)encoder;
      }
      static void radeon_enc_cs_flush(void *ctx, unsigned flags, struct pipe_fence_handle **fence)
   {
         }
      /* configure reconstructed picture offset */
   static void radeon_enc_rec_offset(rvcn_enc_reconstructed_picture_t *recon,
                           {
      if (offset) {
      recon->luma_offset = *offset;
   *offset += luma_size;
   recon->chroma_offset = *offset;
   *offset += chroma_size;
   if (is_av1) {
      recon->av1.av1_cdf_frame_context_offset = *offset;
   *offset += RENCODE_AV1_FRAME_CONTEXT_CDF_TABLE_SIZE;
   recon->av1.av1_cdef_algorithm_context_offset = *offset;
         } else {
      recon->luma_offset = 0;
   recon->chroma_offset = 0;
   recon->av1.av1_cdf_frame_context_offset = 0;
         }
      static int setup_cdf(struct radeon_encoder *enc)
   {
               if (!enc->cdf ||
         !si_vid_create_buffer(enc->screen,
                  RVID_ERR("Can't create CDF buffer.\n");
               p_cdf = enc->ws->buffer_map(enc->ws,
                     if (!p_cdf)
            memcpy(p_cdf, rvcn_av1_cdf_default_table, VCN_ENC_AV1_DEFAULT_CDF_SIZE);
                  error:
         }
      static int setup_dpb(struct radeon_encoder *enc)
   {
      bool is_h264 = u_reduce_video_profile(enc->base.profile)
         bool is_av1 = u_reduce_video_profile(enc->base.profile)
         uint32_t rec_alignment = is_h264 ? 16 : 64;
   uint32_t aligned_width = align(enc->base.width, rec_alignment);
   uint32_t aligned_height = align(enc->base.height, rec_alignment);
   uint32_t pitch = align(aligned_width, enc->alignment);
   uint32_t num_reconstructed_pictures = enc->base.max_references + 1;
   uint32_t luma_size, chroma_size, offset;
   struct radeon_enc_pic *enc_pic = &enc->enc_pic;
   int i;
            luma_size = align(pitch * aligned_dpb_height , enc->alignment);
   chroma_size = align(luma_size / 2 , enc->alignment);
   if (enc_pic->bit_depth_luma_minus8 || enc_pic->bit_depth_chroma_minus8) {
      luma_size *= 2;
                        enc_pic->ctx_buf.rec_luma_pitch   = pitch;
   enc_pic->ctx_buf.rec_chroma_pitch = pitch;
   enc_pic->ctx_buf.pre_encode_picture_luma_pitch   = pitch;
            offset = 0;
   if (enc_pic->quality_modes.pre_encode_mode) {
      uint32_t pre_size  = DIV_ROUND_UP((aligned_width >> 2), rec_alignment) *
         uint32_t full_size = DIV_ROUND_UP(aligned_width, rec_alignment) *
         pre_size  = align(pre_size, 4);
            enc_pic->ctx_buf.two_pass_search_center_map_offset = offset;
   if (is_h264 && !enc_pic->spec_misc.b_picture_enabled)
         else if (!is_h264)
      } else
            if (is_av1) {
      enc_pic->ctx_buf.av1.av1_sdb_intermedidate_context_offset = offset;
               for (i = 0; i < num_reconstructed_pictures; i++) {
      radeon_enc_rec_offset(&enc_pic->ctx_buf.reconstructed_pictures[i],
            if (enc_pic->quality_modes.pre_encode_mode)
      radeon_enc_rec_offset(&enc_pic->ctx_buf.pre_encode_reconstructed_pictures[i],
            for (; i < RENCODE_MAX_NUM_RECONSTRUCTED_PICTURES; i++) {
      radeon_enc_rec_offset(&enc_pic->ctx_buf.reconstructed_pictures[i],
         if (enc_pic->quality_modes.pre_encode_mode)
      radeon_enc_rec_offset(&enc_pic->ctx_buf.pre_encode_reconstructed_pictures[i],
            if (enc_pic->quality_modes.pre_encode_mode) {
      enc_pic->ctx_buf.pre_encode_input_picture.rgb.red_offset = offset;
   offset += luma_size;
   enc_pic->ctx_buf.pre_encode_input_picture.rgb.green_offset = offset;
   offset += luma_size;
   enc_pic->ctx_buf.pre_encode_input_picture.rgb.blue_offset = offset;
               enc_pic->ctx_buf.num_reconstructed_pictures = num_reconstructed_pictures;
            if (enc_pic->spec_misc.b_picture_enabled) {
      enc_pic->ctx_buf.colloc_buffer_offset = offset;
      } else
                        }
      static void radeon_enc_begin_frame(struct pipe_video_codec *encoder,
               {
      struct radeon_encoder *enc = (struct radeon_encoder *)encoder;
   struct vl_video_buffer *vid_buf = (struct vl_video_buffer *)source;
            if (u_reduce_video_profile(enc->base.profile) == PIPE_VIDEO_FORMAT_MPEG4_AVC) {
      struct pipe_h264_enc_picture_desc *pic = (struct pipe_h264_enc_picture_desc *)picture;
   enc->need_rate_control =
      (enc->enc_pic.rc_layer_init[0].target_bit_rate != pic->rate_ctrl[0].target_bitrate) ||
   (enc->enc_pic.rc_layer_init[0].frame_rate_num != pic->rate_ctrl[0].frame_rate_num) ||
   } else if (u_reduce_video_profile(picture->profile) == PIPE_VIDEO_FORMAT_HEVC) {
      struct pipe_h265_enc_picture_desc *pic = (struct pipe_h265_enc_picture_desc *)picture;
   enc->need_rate_control =
      (enc->enc_pic.rc_layer_init[0].target_bit_rate != pic->rc.target_bitrate) ||
   (enc->enc_pic.rc_layer_init[0].frame_rate_num != pic->rc.frame_rate_num) ||
   } else if (u_reduce_video_profile(picture->profile) == PIPE_VIDEO_FORMAT_AV1) {
      struct pipe_av1_enc_picture_desc *pic = (struct pipe_av1_enc_picture_desc *)picture;
   enc->need_rate_control =
      (enc->enc_pic.rc_layer_init[0].target_bit_rate != pic->rc[0].target_bitrate) ||
               if (!enc->cdf) {
      enc->cdf = CALLOC_STRUCT(rvid_buffer);
   if (setup_cdf(enc)) {
      RVID_ERR("Can't create cdf buffer.\n");
                     radeon_vcn_enc_get_param(enc, picture);
   if (!enc->dpb) {
      enc->dpb = CALLOC_STRUCT(rvid_buffer);
   setup_dpb(enc);
   if (!enc->dpb ||
      !si_vid_create_buffer(enc->screen, enc->dpb, enc->dpb_size, PIPE_USAGE_DEFAULT)) {
   RVID_ERR("Can't create DPB buffer.\n");
                  if (source->buffer_format == PIPE_FORMAT_NV12 ||
      source->buffer_format == PIPE_FORMAT_P010 ||
   source->buffer_format == PIPE_FORMAT_P016) {
   enc->get_buffer(vid_buf->resources[0], &enc->handle, &enc->luma);
      }
   else {
      enc->get_buffer(vid_buf->resources[0], &enc->handle, &enc->luma);
                        if (!enc->stream_handle) {
      struct rvid_buffer fb;
   enc->stream_handle = si_vid_alloc_stream_handle();
   enc->si = CALLOC_STRUCT(rvid_buffer);
   if (!enc->si ||
      !enc->stream_handle ||
   !si_vid_create_buffer(enc->screen, enc->si, 128 * 1024, PIPE_USAGE_STAGING)) {
   RVID_ERR("Can't create session buffer.\n");
      }
   si_vid_create_buffer(enc->screen, &fb, 4096, PIPE_USAGE_STAGING);
   enc->fb = &fb;
   enc->begin(enc);
   flush(enc);
   si_vid_destroy_buffer(&fb);
                     error:
      RADEON_ENC_DESTROY_VIDEO_BUFFER(enc->dpb);
   RADEON_ENC_DESTROY_VIDEO_BUFFER(enc->si);
      }
      static void radeon_enc_encode_bitstream(struct pipe_video_codec *encoder,
               {
      struct radeon_encoder *enc = (struct radeon_encoder *)encoder;
            enc->get_buffer(destination, &enc->bs_handle, NULL);
                     if (!si_vid_create_buffer(enc->screen, enc->fb, 4096, PIPE_USAGE_STAGING)) {
      RVID_ERR("Can't create feedback buffer.\n");
               if (vid_buf->base.statistics_data) {
      enc->get_buffer(vid_buf->base.statistics_data, &enc->stats, NULL);
   if (enc->stats->size < sizeof(rvcn_encode_stats_type_0_t)) {
      RVID_ERR("Encoder statistics output buffer is too small.\n");
      }
      }
   else
            enc->need_feedback = true;
      }
      static void radeon_enc_end_frame(struct pipe_video_codec *encoder, struct pipe_video_buffer *source,
         {
      struct radeon_encoder *enc = (struct radeon_encoder *)encoder;
      }
      static void radeon_enc_destroy(struct pipe_video_codec *encoder)
   {
               if (enc->stream_handle) {
      struct rvid_buffer fb;
   enc->need_feedback = false;
   si_vid_create_buffer(enc->screen, &fb, 512, PIPE_USAGE_STAGING);
   enc->fb = &fb;
   enc->destroy(enc);
   flush(enc);
   RADEON_ENC_DESTROY_VIDEO_BUFFER(enc->si);
               RADEON_ENC_DESTROY_VIDEO_BUFFER(enc->dpb);
   RADEON_ENC_DESTROY_VIDEO_BUFFER(enc->cdf);
   enc->ws->cs_destroy(&enc->cs);
   if (enc->ectx)
               }
      static void radeon_enc_get_feedback(struct pipe_video_codec *encoder, void *feedback,
         {
      struct radeon_encoder *enc = (struct radeon_encoder *)encoder;
            if (size) {
      uint32_t *ptr = enc->ws->buffer_map(enc->ws, fb->res->buf, &enc->cs,
         if (ptr[1])
         else
                     si_vid_destroy_buffer(fb);
      }
      static void radeon_enc_destroy_fence(struct pipe_video_codec *encoder,
         {
                  }
      struct pipe_video_codec *radeon_create_encoder(struct pipe_context *context,
                     {
      struct si_screen *sscreen = (struct si_screen *)context->screen;
   struct si_context *sctx = (struct si_context *)context;
                     if (!enc)
            if (sctx->vcn_has_ctx) {
      enc->ectx = pipe_create_multimedia_context(context->screen);
   if (!enc->ectx)
               enc->alignment = 256;
   enc->base = *templ;
   enc->base.context = (sctx->vcn_has_ctx)? enc->ectx : context;
   enc->base.destroy = radeon_enc_destroy;
   enc->base.begin_frame = radeon_enc_begin_frame;
   enc->base.encode_bitstream = radeon_enc_encode_bitstream;
   enc->base.end_frame = radeon_enc_end_frame;
   enc->base.flush = radeon_enc_flush;
   enc->base.get_feedback = radeon_enc_get_feedback;
   enc->base.destroy_fence = radeon_enc_destroy_fence;
   enc->get_buffer = get_buffer;
   enc->bits_in_shifter = 0;
   enc->screen = context->screen;
            if (!ws->cs_create(&enc->cs,
      (sctx->vcn_has_ctx) ? ((struct si_context *)enc->ectx)->ctx : sctx->ctx,
   AMD_IP_VCN_ENC, radeon_enc_cs_flush, enc)) {
   RVID_ERR("Can't get command submission context.\n");
               if (sscreen->info.vcn_ip_version >= VCN_4_0_0)
         else if (sscreen->info.vcn_ip_version >= VCN_3_0_0)
         else if (sscreen->info.vcn_ip_version >= VCN_2_0_0)
         else
                  error:
      enc->ws->cs_destroy(&enc->cs);
   FREE(enc);
      }
      void radeon_enc_add_buffer(struct radeon_encoder *enc, struct pb_buffer *buf,
         {
      enc->ws->cs_add_buffer(&enc->cs, buf, usage | RADEON_USAGE_SYNCHRONIZED, domain);
   uint64_t addr;
   addr = enc->ws->buffer_get_virtual_address(buf);
   addr = addr + offset;
   RADEON_ENC_CS(addr >> 32);
      }
      void radeon_enc_set_emulation_prevention(struct radeon_encoder *enc, bool set)
   {
      if (set != enc->emulation_prevention) {
      enc->emulation_prevention = set;
         }
      void radeon_enc_output_one_byte(struct radeon_encoder *enc, unsigned char byte)
   {
      if (enc->byte_index == 0)
         enc->cs.current.buf[enc->cs.current.cdw] |=
                  if (enc->byte_index >= 4) {
      enc->byte_index = 0;
         }
      void radeon_enc_emulation_prevention(struct radeon_encoder *enc, unsigned char byte)
   {
      if (enc->emulation_prevention) {
      if ((enc->num_zeros >= 2) && ((byte == 0x00) || (byte == 0x01) ||
      (byte == 0x02) || (byte == 0x03))) {
   radeon_enc_output_one_byte(enc, 0x03);
   enc->bits_output += 8;
      }
         }
      void radeon_enc_code_fixed_bits(struct radeon_encoder *enc, unsigned int value,
         {
      unsigned int bits_to_pack = 0;
            while (num_bits > 0) {
      unsigned int value_to_pack = value & (0xffffffff >> (32 - num_bits));
   bits_to_pack =
            if (bits_to_pack < num_bits)
            enc->shifter |= value_to_pack << (32 - enc->bits_in_shifter - bits_to_pack);
   num_bits -= bits_to_pack;
            while (enc->bits_in_shifter >= 8) {
      unsigned char output_byte = (unsigned char)(enc->shifter >> 24);
   enc->shifter <<= 8;
   radeon_enc_emulation_prevention(enc, output_byte);
   radeon_enc_output_one_byte(enc, output_byte);
   enc->bits_in_shifter -= 8;
            }
      void radeon_enc_code_uvlc(struct radeon_encoder *enc, unsigned int value)
   {
      uint32_t num_bits = 0;
   uint64_t value_plus1 = (uint64_t)value + 1;
            while ((uint64_t)1 << num_bits <= value_plus1)
            num_leading_zeros = num_bits - 1;
   radeon_enc_code_fixed_bits(enc, 0, num_leading_zeros);
   radeon_enc_code_fixed_bits(enc, 1, 1);
      }
      void radeon_enc_code_leb128(uint8_t *buf, uint32_t value,
         {
      uint8_t leb128_byte = 0;
            do {
      leb128_byte = (value & 0x7f);
   value >>= 7;
   if (num_bytes > 1)
            *(buf + i) = leb128_byte;
   num_bytes--;
         }
      void radeon_enc_reset(struct radeon_encoder *enc)
   {
      enc->emulation_prevention = false;
   enc->shifter = 0;
   enc->bits_in_shifter = 0;
   enc->bits_output = 0;
   enc->num_zeros = 0;
   enc->byte_index = 0;
      }
      void radeon_enc_byte_align(struct radeon_encoder *enc)
   {
               if (num_padding_zeros > 0)
      }
      void radeon_enc_flush_headers(struct radeon_encoder *enc)
   {
      if (enc->bits_in_shifter != 0) {
      unsigned char output_byte = (unsigned char)(enc->shifter >> 24);
   radeon_enc_emulation_prevention(enc, output_byte);
   radeon_enc_output_one_byte(enc, output_byte);
   enc->bits_output += enc->bits_in_shifter;
   enc->shifter = 0;
   enc->bits_in_shifter = 0;
               if (enc->byte_index > 0) {
      enc->cs.current.cdw++;
         }
      void radeon_enc_code_ue(struct radeon_encoder *enc, unsigned int value)
   {
      int x = -1;
   unsigned int ue_code = value + 1;
            while (value) {
      value = (value >> 1);
               unsigned int ue_length = (x << 1) + 1;
      }
      void radeon_enc_code_se(struct radeon_encoder *enc, int value)
   {
               if (value != 0)
               }
      /* dummy function for re-using the same pipeline */
   void radeon_enc_dummy(struct radeon_encoder *enc) {}
      /* this function has to be in pair with AV1 header copy instruction type at the end */
   static void radeon_enc_av1_bs_copy_end(struct radeon_encoder *enc, uint32_t bits)
   {
      assert(bits > 0);
   /* it must be dword aligned at the end */
   *enc->enc_pic.copy_start = DIV_ROUND_UP(bits, 32) * 4 + 12;
      }
      /* av1 bitstream instruction type */
   void radeon_enc_av1_bs_instruction_type(struct radeon_encoder *enc,
               {
               if (enc->bits_output)
            enc->enc_pic.copy_start = &enc->cs.current.buf[enc->cs.current.cdw++];
            if (inst != RENCODE_HEADER_INSTRUCTION_COPY) {
      *enc->enc_pic.copy_start = 8;
   if (inst == RENCODE_AV1_BITSTREAM_INSTRUCTION_OBU_START) {
      *enc->enc_pic.copy_start += 4;
         } else
               }
      uint32_t radeon_enc_value_bits(uint32_t value)
   {
               while (value > 1) {
      i++;
                  }
