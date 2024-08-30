   /**************************************************************************
   *
   * Copyright 2017 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   *
   **************************************************************************/
      #include "radeon_vcn_dec.h"
      #include "pipe/p_video_codec.h"
   #include "radeonsi/si_pipe.h"
   #include "util/u_memory.h"
   #include "util/u_video.h"
   #include "vl/vl_mpeg12_decoder.h"
   #include "vl/vl_probs_table.h"
   #include "pspdecryptionparam.h"
      #include <assert.h>
   #include <stdio.h>
      #include "ac_vcn_av1_default.h"
      #define FB_BUFFER_OFFSET             0x1000
   #define FB_BUFFER_SIZE               2048
   #define IT_SCALING_TABLE_SIZE        992
   #define VP9_PROBS_TABLE_SIZE         (RDECODE_VP9_PROBS_DATA_SIZE + 256)
      #define NUM_MPEG2_REFS 6
   #define NUM_H264_REFS  17
   #define NUM_VC1_REFS   5
   #define NUM_VP9_REFS   8
   #define NUM_AV1_REFS   8
   #define NUM_AV1_REFS_PER_FRAME 7
      static unsigned calc_dpb_size(struct radeon_decoder *dec);
   static unsigned calc_ctx_size_h264_perf(struct radeon_decoder *dec);
   static unsigned calc_ctx_size_h265_main(struct radeon_decoder *dec);
   static unsigned calc_ctx_size_h265_main10(struct radeon_decoder *dec,
            static void radeon_dec_destroy_associated_data(void *data)
   {
         }
      static void get_current_pic_index(struct radeon_decoder *dec,
               {
      for (int i = 0; i < ARRAY_SIZE(dec->render_pic_list); ++i) {
      if (dec->render_pic_list[i] && dec->render_pic_list[i] == target) {
      if (target->codec != NULL)
         else {
      *curr_pic_idx = i;
   vl_video_buffer_set_associated_data(target, &dec->base, (void *)(uintptr_t)i,
      }
      } else if (!dec->render_pic_list[i]) {
      dec->render_pic_list[i] = target;
   *curr_pic_idx = i;
   vl_video_buffer_set_associated_data(target, &dec->base, (void *)(uintptr_t)i,
                  }
      static rvcn_dec_message_avc_t get_h264_msg(struct radeon_decoder *dec,
               {
      rvcn_dec_message_avc_t result;
            memset(&result, 0, sizeof(result));
   switch (pic->base.profile) {
   case PIPE_VIDEO_PROFILE_MPEG4_AVC_BASELINE:
   case PIPE_VIDEO_PROFILE_MPEG4_AVC_CONSTRAINED_BASELINE:
      result.profile = RDECODE_H264_PROFILE_BASELINE;
         case PIPE_VIDEO_PROFILE_MPEG4_AVC_MAIN:
      result.profile = RDECODE_H264_PROFILE_MAIN;
         case PIPE_VIDEO_PROFILE_MPEG4_AVC_HIGH:
      result.profile = RDECODE_H264_PROFILE_HIGH;
         default:
      assert(0);
                        result.sps_info_flags = 0;
   result.sps_info_flags |= pic->pps->sps->direct_8x8_inference_flag << 0;
   result.sps_info_flags |= pic->pps->sps->mb_adaptive_frame_field_flag << 1;
   result.sps_info_flags |= pic->pps->sps->frame_mbs_only_flag << 2;
   result.sps_info_flags |= pic->pps->sps->delta_pic_order_always_zero_flag << 3;
   result.sps_info_flags |= ((dec->dpb_type == DPB_DYNAMIC_TIER_2) ? 0 : 1)
            result.bit_depth_luma_minus8 = pic->pps->sps->bit_depth_luma_minus8;
   result.bit_depth_chroma_minus8 = pic->pps->sps->bit_depth_chroma_minus8;
   result.log2_max_frame_num_minus4 = pic->pps->sps->log2_max_frame_num_minus4;
   result.pic_order_cnt_type = pic->pps->sps->pic_order_cnt_type;
            switch (dec->base.chroma_format) {
   case PIPE_VIDEO_CHROMA_FORMAT_NONE:
         case PIPE_VIDEO_CHROMA_FORMAT_400:
      result.chroma_format = 0;
      case PIPE_VIDEO_CHROMA_FORMAT_420:
      result.chroma_format = 1;
      case PIPE_VIDEO_CHROMA_FORMAT_422:
      result.chroma_format = 2;
      case PIPE_VIDEO_CHROMA_FORMAT_444:
      result.chroma_format = 3;
               result.pps_info_flags = 0;
   result.pps_info_flags |= pic->pps->transform_8x8_mode_flag << 0;
   result.pps_info_flags |= pic->pps->redundant_pic_cnt_present_flag << 1;
   result.pps_info_flags |= pic->pps->constrained_intra_pred_flag << 2;
   result.pps_info_flags |= pic->pps->deblocking_filter_control_present_flag << 3;
   result.pps_info_flags |= pic->pps->weighted_bipred_idc << 4;
   result.pps_info_flags |= pic->pps->weighted_pred_flag << 6;
   result.pps_info_flags |= pic->pps->bottom_field_pic_order_in_frame_present_flag << 7;
            result.num_slice_groups_minus1 = pic->pps->num_slice_groups_minus1;
   result.slice_group_map_type = pic->pps->slice_group_map_type;
   result.slice_group_change_rate_minus1 = pic->pps->slice_group_change_rate_minus1;
   result.pic_init_qp_minus26 = pic->pps->pic_init_qp_minus26;
   result.chroma_qp_index_offset = pic->pps->chroma_qp_index_offset;
            memcpy(result.scaling_list_4x4, pic->pps->ScalingList4x4, 6 * 16);
            memcpy(dec->it, result.scaling_list_4x4, 6 * 16);
                     result.num_ref_idx_l0_active_minus1 = pic->num_ref_idx_l0_active_minus1;
            result.frame_num = pic->frame_num;
   memcpy(result.frame_num_list, pic->frame_num_list, 4 * 16);
   result.curr_field_order_cnt_list[0] = pic->field_order_cnt[0];
   result.curr_field_order_cnt_list[1] = pic->field_order_cnt[1];
   memcpy(result.field_order_cnt_list, pic->field_order_cnt_list, 4 * 16 * 2);
   result.non_existing_frame_flags = 0;
            if (dec->dpb_type != DPB_DYNAMIC_TIER_2) {
      result.decoded_pic_idx = pic->frame_num;
               for (i = 0; i < ARRAY_SIZE(dec->render_pic_list); i++) {
      for (j = 0; (pic->ref[j] != NULL) && (j < ARRAY_SIZE(dec->render_pic_list)); j++) {
      if (dec->render_pic_list[i] == pic->ref[j])
         if (j == ARRAY_SIZE(dec->render_pic_list) - 1)
         else if (pic->ref[j + 1] == NULL)
         }
   for (i = 0; i < ARRAY_SIZE(dec->render_pic_list); ++i) {
      if (dec->render_pic_list[i] && dec->render_pic_list[i] == target) {
      if (target->codec != NULL){
      result.decoded_pic_idx =
      } else {
      result.decoded_pic_idx = i;
   vl_video_buffer_set_associated_data(target, &dec->base, (void *)(uintptr_t)i,
      }
         }
   if (i == ARRAY_SIZE(dec->render_pic_list)) {
      for (i = 0; i < ARRAY_SIZE(dec->render_pic_list); ++i) {
      if (!dec->render_pic_list[i]) {
      dec->render_pic_list[i] = target;
   result.decoded_pic_idx = i;
   vl_video_buffer_set_associated_data(target, &dec->base, (void *)(uintptr_t)i,
                  }
   for (i = 0; i < ARRAY_SIZE(result.ref_frame_list); i++) {
      result.ref_frame_list[i] = pic->ref[i] ?
            if (result.ref_frame_list[i] != 0xff) {
      if (pic->top_is_reference[i])
                                                for (j = 0; j < ARRAY_SIZE(dec->h264_valid_ref_num); j++) {
      if ((dec->h264_valid_ref_num[j] != (unsigned)-1)
                     for (k = 0; k < ARRAY_SIZE(dec->h264_valid_poc_num); k++) {
      if ((dec->h264_valid_poc_num[k] != (unsigned)-1)
         && ((dec->h264_valid_poc_num[k] == result.field_order_cnt_list[i][0])
         }
   if (result.ref_frame_list[i] != 0xff && (j == ARRAY_SIZE(dec->h264_valid_ref_num))
            result.non_existing_frame_flags |= 1 << i;
   result.curr_pic_ref_frame_num--;
                  /* if reference picture exists, however no reference picture found at the end
         if (result.used_for_reference_flags && (result.curr_pic_ref_frame_num == 0)) {
      for (i = 0; i < ARRAY_SIZE(result.ref_frame_list); i++) {
      result.ref_frame_list[i] = pic->ref[i] ?
         if (result.ref_frame_list[i] != 0xff) {
      result.curr_pic_ref_frame_num++;
   result.non_existing_frame_flags &= ~(1 << i);
                     for (i = 0; i < ARRAY_SIZE(result.ref_frame_list); i++) {
      if (result.ref_frame_list[i] != 0xff) {
      dec->h264_valid_ref_num[i]         = result.frame_num_list[i];
   dec->h264_valid_poc_num[2 * i]     = pic->top_is_reference[i] ?
         dec->h264_valid_poc_num[2 * i + 1] = pic->bottom_is_reference[i] ?
      } else {
      dec->h264_valid_ref_num[i]         =
   dec->h264_valid_poc_num[2 * i]     =
                  dec->h264_valid_ref_num[ARRAY_SIZE(dec->h264_valid_ref_num) - 1] = result.frame_num;
   dec->h264_valid_poc_num[ARRAY_SIZE(dec->h264_valid_poc_num) - 2] =
               dec->h264_valid_poc_num[ARRAY_SIZE(dec->h264_valid_poc_num) - 1] =
                  if (dec->dpb_type == DPB_DYNAMIC_TIER_2) {
      dec->ref_codec.bts = CODEC_8_BITS;
   dec->ref_codec.index = result.decoded_pic_idx;
   dec->ref_codec.ref_size = 16;
   memset(dec->ref_codec.ref_list, 0xff, sizeof(dec->ref_codec.ref_list));
            end:
         }
      static rvcn_dec_message_hevc_t get_h265_msg(struct radeon_decoder *dec,
               {
      rvcn_dec_message_hevc_t result;
            memset(&result, 0, sizeof(result));
   result.sps_info_flags = 0;
   result.sps_info_flags |= pic->pps->sps->scaling_list_enabled_flag << 0;
   result.sps_info_flags |= pic->pps->sps->amp_enabled_flag << 1;
   result.sps_info_flags |= pic->pps->sps->sample_adaptive_offset_enabled_flag << 2;
   result.sps_info_flags |= pic->pps->sps->pcm_enabled_flag << 3;
   result.sps_info_flags |= pic->pps->sps->pcm_loop_filter_disabled_flag << 4;
   result.sps_info_flags |= pic->pps->sps->long_term_ref_pics_present_flag << 5;
   result.sps_info_flags |= pic->pps->sps->sps_temporal_mvp_enabled_flag << 6;
   result.sps_info_flags |= pic->pps->sps->strong_intra_smoothing_enabled_flag << 7;
   result.sps_info_flags |= pic->pps->sps->separate_colour_plane_flag << 8;
   if (((struct si_screen *)dec->screen)->info.family == CHIP_CARRIZO)
         if (pic->UseRefPicList == true)
         if (pic->UseStRpsBits == true && pic->pps->st_rps_bits != 0) {
      result.sps_info_flags |= 1 << 11;
   }
         result.chroma_format = pic->pps->sps->chroma_format_idc;
   result.bit_depth_luma_minus8 = pic->pps->sps->bit_depth_luma_minus8;
   result.bit_depth_chroma_minus8 = pic->pps->sps->bit_depth_chroma_minus8;
   result.log2_max_pic_order_cnt_lsb_minus4 = pic->pps->sps->log2_max_pic_order_cnt_lsb_minus4;
   result.sps_max_dec_pic_buffering_minus1 = pic->pps->sps->sps_max_dec_pic_buffering_minus1;
   result.log2_min_luma_coding_block_size_minus3 =
         result.log2_diff_max_min_luma_coding_block_size =
         result.log2_min_transform_block_size_minus2 =
         result.log2_diff_max_min_transform_block_size =
         result.max_transform_hierarchy_depth_inter = pic->pps->sps->max_transform_hierarchy_depth_inter;
   result.max_transform_hierarchy_depth_intra = pic->pps->sps->max_transform_hierarchy_depth_intra;
   result.pcm_sample_bit_depth_luma_minus1 = pic->pps->sps->pcm_sample_bit_depth_luma_minus1;
   result.pcm_sample_bit_depth_chroma_minus1 = pic->pps->sps->pcm_sample_bit_depth_chroma_minus1;
   result.log2_min_pcm_luma_coding_block_size_minus3 =
         result.log2_diff_max_min_pcm_luma_coding_block_size =
                  result.pps_info_flags = 0;
   result.pps_info_flags |= pic->pps->dependent_slice_segments_enabled_flag << 0;
   result.pps_info_flags |= pic->pps->output_flag_present_flag << 1;
   result.pps_info_flags |= pic->pps->sign_data_hiding_enabled_flag << 2;
   result.pps_info_flags |= pic->pps->cabac_init_present_flag << 3;
   result.pps_info_flags |= pic->pps->constrained_intra_pred_flag << 4;
   result.pps_info_flags |= pic->pps->transform_skip_enabled_flag << 5;
   result.pps_info_flags |= pic->pps->cu_qp_delta_enabled_flag << 6;
   result.pps_info_flags |= pic->pps->pps_slice_chroma_qp_offsets_present_flag << 7;
   result.pps_info_flags |= pic->pps->weighted_pred_flag << 8;
   result.pps_info_flags |= pic->pps->weighted_bipred_flag << 9;
   result.pps_info_flags |= pic->pps->transquant_bypass_enabled_flag << 10;
   result.pps_info_flags |= pic->pps->tiles_enabled_flag << 11;
   result.pps_info_flags |= pic->pps->entropy_coding_sync_enabled_flag << 12;
   result.pps_info_flags |= pic->pps->uniform_spacing_flag << 13;
   result.pps_info_flags |= pic->pps->loop_filter_across_tiles_enabled_flag << 14;
   result.pps_info_flags |= pic->pps->pps_loop_filter_across_slices_enabled_flag << 15;
   result.pps_info_flags |= pic->pps->deblocking_filter_override_enabled_flag << 16;
   result.pps_info_flags |= pic->pps->pps_deblocking_filter_disabled_flag << 17;
   result.pps_info_flags |= pic->pps->lists_modification_present_flag << 18;
            result.num_extra_slice_header_bits = pic->pps->num_extra_slice_header_bits;
   result.num_long_term_ref_pic_sps = pic->pps->sps->num_long_term_ref_pics_sps;
   result.num_ref_idx_l0_default_active_minus1 = pic->pps->num_ref_idx_l0_default_active_minus1;
   result.num_ref_idx_l1_default_active_minus1 = pic->pps->num_ref_idx_l1_default_active_minus1;
   result.pps_cb_qp_offset = pic->pps->pps_cb_qp_offset;
   result.pps_cr_qp_offset = pic->pps->pps_cr_qp_offset;
   result.pps_beta_offset_div2 = pic->pps->pps_beta_offset_div2;
   result.pps_tc_offset_div2 = pic->pps->pps_tc_offset_div2;
   result.diff_cu_qp_delta_depth = pic->pps->diff_cu_qp_delta_depth;
   result.num_tile_columns_minus1 = pic->pps->num_tile_columns_minus1;
   result.num_tile_rows_minus1 = pic->pps->num_tile_rows_minus1;
   result.log2_parallel_merge_level_minus2 = pic->pps->log2_parallel_merge_level_minus2;
            for (i = 0; i < 19; ++i)
            for (i = 0; i < 21; ++i)
            result.num_delta_pocs_ref_rps_idx = pic->NumDeltaPocsOfRefRpsIdx;
            for (i = 0; i < ARRAY_SIZE(dec->render_pic_list); i++) {
      for (j = 0;
      (pic->ref[j] != NULL) && (j < ARRAY_SIZE(dec->render_pic_list));
   j++) {
   if (dec->render_pic_list[i] == pic->ref[j])
         if (j == ARRAY_SIZE(dec->render_pic_list) - 1)
         else if (pic->ref[j + 1] == NULL)
         }
   for (i = 0; i < ARRAY_SIZE(dec->render_pic_list); i++) {
      if (dec->render_pic_list[i] == NULL) {
      dec->render_pic_list[i] = target;
   result.curr_idx = i;
                  vl_video_buffer_set_associated_data(target, &dec->base, (void *)(uintptr_t)result.curr_idx,
            for (i = 0; i < 16; ++i) {
      struct pipe_video_buffer *ref = pic->ref[i];
                     if (ref)
         else
                     for (i = 0; i < 8; ++i) {
      result.ref_pic_set_st_curr_before[i] = 0xFF;
   result.ref_pic_set_st_curr_after[i] = 0xFF;
               for (i = 0; i < pic->NumPocStCurrBefore; ++i)
            for (i = 0; i < pic->NumPocStCurrAfter; ++i)
            for (i = 0; i < pic->NumPocLtCurr; ++i)
            for (i = 0; i < 6; ++i)
            for (i = 0; i < 2; ++i)
            memcpy(dec->it, pic->pps->sps->ScalingList4x4, 6 * 16);
   memcpy(dec->it + 96, pic->pps->sps->ScalingList8x8, 6 * 64);
   memcpy(dec->it + 480, pic->pps->sps->ScalingList16x16, 6 * 64);
            for (i = 0; i < 2; i++) {
      for (j = 0; j < 15; j++)
               if (pic->base.profile == PIPE_VIDEO_PROFILE_HEVC_MAIN_10) {
      if (target->buffer_format == PIPE_FORMAT_P010 || target->buffer_format == PIPE_FORMAT_P016) {
      result.p010_mode = 1;
      } else {
      result.p010_mode = 0;
   result.luma_10to8 = 5;
   result.chroma_10to8 = 5;
   result.hevc_reserved[0] = 4; /* sclr_luma10to8 */
                  if (dec->dpb_type == DPB_DYNAMIC_TIER_2) {
      dec->ref_codec.bts = (pic->base.profile == PIPE_VIDEO_PROFILE_HEVC_MAIN_10) ?
         dec->ref_codec.index = result.curr_idx;
   dec->ref_codec.ref_size = 15;
   memset(dec->ref_codec.ref_list, 0x7f, sizeof(dec->ref_codec.ref_list));
      }
      }
      static void fill_probs_table(void *ptr)
   {
               memcpy(&probs->coef_probs[0], default_coef_probs_4x4, sizeof(default_coef_probs_4x4));
   memcpy(&probs->coef_probs[1], default_coef_probs_8x8, sizeof(default_coef_probs_8x8));
   memcpy(&probs->coef_probs[2], default_coef_probs_16x16, sizeof(default_coef_probs_16x16));
   memcpy(&probs->coef_probs[3], default_coef_probs_32x32, sizeof(default_coef_probs_32x32));
   memcpy(probs->y_mode_prob, default_if_y_probs, sizeof(default_if_y_probs));
   memcpy(probs->uv_mode_prob, default_if_uv_probs, sizeof(default_if_uv_probs));
   memcpy(probs->single_ref_prob, default_single_ref_p, sizeof(default_single_ref_p));
   memcpy(probs->switchable_interp_prob, default_switchable_interp_prob,
         memcpy(probs->partition_prob, default_partition_probs, sizeof(default_partition_probs));
   memcpy(probs->inter_mode_probs, default_inter_mode_probs, sizeof(default_inter_mode_probs));
   memcpy(probs->mbskip_probs, default_skip_probs, sizeof(default_skip_probs));
   memcpy(probs->intra_inter_prob, default_intra_inter_p, sizeof(default_intra_inter_p));
   memcpy(probs->comp_inter_prob, default_comp_inter_p, sizeof(default_comp_inter_p));
   memcpy(probs->comp_ref_prob, default_comp_ref_p, sizeof(default_comp_ref_p));
   memcpy(probs->tx_probs_32x32, default_tx_probs_32x32, sizeof(default_tx_probs_32x32));
   memcpy(probs->tx_probs_16x16, default_tx_probs_16x16, sizeof(default_tx_probs_16x16));
   memcpy(probs->tx_probs_8x8, default_tx_probs_8x8, sizeof(default_tx_probs_8x8));
   memcpy(probs->mv_joints, default_nmv_joints, sizeof(default_nmv_joints));
   memcpy(&probs->mv_comps[0], default_nmv_components, sizeof(default_nmv_components));
      }
      static rvcn_dec_message_vp9_t get_vp9_msg(struct radeon_decoder *dec,
               {
      rvcn_dec_message_vp9_t result;
                     /* segment table */
            if (pic->picture_parameter.pic_fields.segmentation_enabled) {
      for (i = 0; i < 8; ++i) {
      prbs->seg.feature_data[i] =
      (pic->slice_parameter.seg_param[i].alt_quant & 0xffff) |
   ((pic->slice_parameter.seg_param[i].alt_lf & 0xff) << 16) |
      prbs->seg.feature_mask[i] =
      (pic->slice_parameter.seg_param[i].alt_quant_enabled << 0) |
   (pic->slice_parameter.seg_param[i].alt_lf_enabled << 1) |
   (pic->slice_parameter.seg_param[i].segment_flags.segment_reference_enabled << 2) |
            for (i = 0; i < 7; ++i)
            for (i = 0; i < 3; ++i)
               } else
            result.frame_header_flags = (pic->picture_parameter.pic_fields.frame_type
                  result.frame_header_flags |= (pic->picture_parameter.pic_fields.error_resilient_mode
                  result.frame_header_flags |= (pic->picture_parameter.pic_fields.intra_only
                  result.frame_header_flags |= (pic->picture_parameter.pic_fields.allow_high_precision_mv
                  result.frame_header_flags |= (pic->picture_parameter.pic_fields.frame_parallel_decoding_mode
                  result.frame_header_flags |= (pic->picture_parameter.pic_fields.refresh_frame_context
                  result.frame_header_flags |= (pic->picture_parameter.pic_fields.segmentation_enabled
                  result.frame_header_flags |= (pic->picture_parameter.pic_fields.segmentation_update_map
                  result.frame_header_flags |= (pic->picture_parameter.pic_fields.segmentation_temporal_update
                  result.frame_header_flags |= (pic->picture_parameter.mode_ref_delta_enabled
                  result.frame_header_flags |= (pic->picture_parameter.mode_ref_delta_update
                  result.frame_header_flags |=
      ((dec->show_frame && !pic->picture_parameter.pic_fields.error_resilient_mode &&
   dec->last_width == dec->base.width && dec->last_height == dec->base.height)
   << RDECODE_FRAME_HDR_INFO_VP9_USE_PREV_IN_FIND_MV_REFS_SHIFT) &
               result.frame_header_flags |=  (1 << RDECODE_FRAME_HDR_INFO_VP9_USE_UNCOMPRESSED_HEADER_SHIFT) &
                     result.frame_context_idx = pic->picture_parameter.pic_fields.frame_context_idx;
            result.filter_level = pic->picture_parameter.filter_level;
            for (i = 0; i < 8; ++i)
            if (pic->picture_parameter.pic_fields.lossless_flag) {
      result.base_qindex = 0;
   result.y_dc_delta_q = 0;
   result.uv_ac_delta_q = 0;
      } else {
      result.base_qindex = pic->picture_parameter.base_qindex;
   result.y_dc_delta_q = pic->picture_parameter.y_dc_delta_q;
   result.uv_ac_delta_q = pic->picture_parameter.uv_ac_delta_q;
               result.log2_tile_cols = pic->picture_parameter.log2_tile_columns;
   result.log2_tile_rows = pic->picture_parameter.log2_tile_rows;
   result.chroma_format = 1;
   result.bit_depth_luma_minus8 = result.bit_depth_chroma_minus8 =
            result.vp9_frame_size = align(dec->bs_size, 128);
   result.uncompressed_header_size = pic->picture_parameter.frame_header_length_in_bytes;
                     //clear the dec->render list if it is not used as a reference
   for (i = 0; i < ARRAY_SIZE(dec->render_pic_list); i++) {
      if (dec->render_pic_list[i]) {
      for (j=0;j<8;j++) {
      if (dec->render_pic_list[i] == pic->ref[j])
      }
   if (j == 8)
                           for (i = 0; i < 8; i++) {
      result.ref_frame_map[i] =
      (pic->ref[i]) ? (uintptr_t)vl_video_buffer_get_associated_data(pic->ref[i], &dec->base)
            result.frame_refs[0] = result.ref_frame_map[pic->picture_parameter.pic_fields.last_ref_frame];
   result.ref_frame_sign_bias[0] = pic->picture_parameter.pic_fields.last_ref_frame_sign_bias;
   result.frame_refs[1] = result.ref_frame_map[pic->picture_parameter.pic_fields.golden_ref_frame];
   result.ref_frame_sign_bias[1] = pic->picture_parameter.pic_fields.golden_ref_frame_sign_bias;
   result.frame_refs[2] = result.ref_frame_map[pic->picture_parameter.pic_fields.alt_ref_frame];
            if (pic->base.profile == PIPE_VIDEO_PROFILE_VP9_PROFILE2) {
      if (target->buffer_format == PIPE_FORMAT_P010 || target->buffer_format == PIPE_FORMAT_P016) {
      result.p010_mode = 1;
      } else {
      result.p010_mode = 0;
   result.luma_10to8 = 1;
                  if (dec->dpb_type == DPB_DYNAMIC_TIER_2) {
      dec->ref_codec.bts = (pic->base.profile == PIPE_VIDEO_PROFILE_VP9_PROFILE2) ?
         dec->ref_codec.index = result.curr_pic_idx;
   dec->ref_codec.ref_size = 8;
   memset(dec->ref_codec.ref_list, 0x7f, sizeof(dec->ref_codec.ref_list));
               dec->last_width = dec->base.width;
               }
      static void set_drm_keys(rvcn_dec_message_drm_t *drm, DECRYPT_PARAMETERS *decrypted)
   {
      int cbc = decrypted->u.s.cbc;
   int ctr = decrypted->u.s.ctr;
   int id = decrypted->u.s.drm_id;
   int ekc = 1;
   int data1 = 1;
            drm->drm_cmd = 0;
                     if (cbc || ctr) {
      drm->drm_cntl = 0 << DRM_CNTL_BYPASS_SHIFT;
            if (ctr)
         else if (cbc)
            drm->drm_cmd |= 1 << DRM_CMD_GEN_MASK_SHIFT;
   drm->drm_cmd |= ekc << DRM_CMD_UNWRAP_KEY_SHIFT;
   drm->drm_cmd |= 0 << DRM_CMD_OFFSET_SHIFT;
   drm->drm_cmd |= data2 << DRM_CMD_CNT_DATA_SHIFT;
   drm->drm_cmd |= data1 << DRM_CMD_CNT_KEY_SHIFT;
   drm->drm_cmd |= ekc << DRM_CMD_KEY_SHIFT;
            if (ekc)
         if (data1)
         if (data2)
               }
      static int32_t rvcn_av1_film_grain_random_number(unsigned short *seed, int32_t bits)
   {
      unsigned short bit;
            bit = ((value >> 0) ^ (value >> 1) ^ (value >> 3) ^ (value >> 12)) & 1;
   value = (value >> 1) | (bit << 15);
               }
      static void rvcn_av1_film_grain_init_scaling(uint8_t scaling_points[][2],
               {
      int32_t i, x, delta_x, delta_y;
            if (num == 0)
            for ( i = 0; i < scaling_points[0][0]; i++ )
            for ( i = 0; i < num - 1; i++ ) {
      delta_y = scaling_points[i + 1][1] - scaling_points[i][1];
                     for ( x = 0; x < delta_x; x++ )
      scaling_lut[scaling_points[i][0] + x] =
            for ( i = scaling_points[num - 1][0]; i < 256; i++ )
      }
      static void rvcn_av1_init_film_grain_buffer(rvcn_dec_film_grain_params_t *fg_params,
         {
      const int32_t luma_block_size_y = 73;
   const int32_t luma_block_size_x = 82;
   const int32_t chroma_block_size_y = 38;
   const int32_t chroma_block_size_x = 44;
   const int32_t gauss_bits = 11;
   int32_t filt_luma_grain_block[luma_block_size_y][luma_block_size_x];
   int32_t filt_cb_grain_block[chroma_block_size_y][chroma_block_size_x];
   int32_t filt_cr_grain_block[chroma_block_size_y][chroma_block_size_x];
   int32_t chroma_subsamp_y = 1;
   int32_t chroma_subsamp_x = 1;
   unsigned short seed = fg_params->random_seed;
   int32_t ar_coeff_lag = fg_params->ar_coeff_lag;
   int32_t bit_depth = fg_params->bit_depth_minus_8 + 8;
   short grain_center = 128 << (bit_depth - 8);
   short grain_min = 0 - grain_center;
   short grain_max = (256 << (bit_depth - 8)) - 1 - grain_center;
   int32_t shift = 12 - bit_depth + fg_params->grain_scale_shift;
   short luma_grain_block_tmp[64][80];
   short cb_grain_block_tmp[32][40];
   short cr_grain_block_tmp[32][40];
   short *align_ptr, *align_ptr0, *align_ptr1;
   int32_t x, y, g, i, j, c, c0, c1, delta_row, delta_col;
            /* generate luma grain block */
   memset(filt_luma_grain_block, 0, sizeof(filt_luma_grain_block));
   for ( y = 0; y < luma_block_size_y; y++ ) {
      for ( x = 0; x < luma_block_size_x; x++ ) {
      g = 0;
   if (fg_params->num_y_points > 0) {
      r = rvcn_av1_film_grain_random_number(&seed, gauss_bits);
      }
                  for ( y = 3; y < luma_block_size_y; y++ ) {
      for ( x = 3; x < luma_block_size_x - 3; x++ ) {
      s = 0;
   pos = 0;
   for (delta_row = -ar_coeff_lag; delta_row <= 0; delta_row++) {
      for (delta_col = -ar_coeff_lag; delta_col <= ar_coeff_lag; delta_col++) {
      if (delta_row == 0 && delta_col == 0)
         c = fg_params->ar_coeffs_y[pos];
   s += filt_luma_grain_block[y + delta_row][x + delta_col] * c;
         }
   filt_luma_grain_block[y][x] =
      AV1_CLAMP(filt_luma_grain_block[y][x]
                     /* generate chroma grain block */
   memset(filt_cb_grain_block, 0, sizeof(filt_cb_grain_block));
   shift = 12 - bit_depth + fg_params->grain_scale_shift;
   seed = fg_params->random_seed ^ 0xb524;
   for (y = 0; y < chroma_block_size_y; y++) {
      for (x = 0; x < chroma_block_size_x; x++) {
      g = 0;
   if (fg_params->num_cb_points || fg_params->chroma_scaling_from_luma) {
      r = rvcn_av1_film_grain_random_number(&seed, gauss_bits);
      }
                  memset(filt_cr_grain_block, 0, sizeof(filt_cr_grain_block));
   seed = fg_params->random_seed ^ 0x49d8;
   for (y = 0; y < chroma_block_size_y; y++) {
      for (x = 0; x < chroma_block_size_x; x++) {
      g = 0;
   if (fg_params->num_cr_points || fg_params->chroma_scaling_from_luma) {
      r = rvcn_av1_film_grain_random_number(&seed, gauss_bits);
      }
                  for (y = 3; y < chroma_block_size_y; y++) {
      for (x = 3; x < chroma_block_size_x - 3; x++) {
      s0 = 0, s1 = 0, pos = 0;
   for (delta_row = -ar_coeff_lag; delta_row <= 0; delta_row++) {
      for (delta_col = -ar_coeff_lag; delta_col <= ar_coeff_lag; delta_col++) {
      c0 = fg_params->ar_coeffs_cb[pos];
   c1 = fg_params->ar_coeffs_cr[pos];
   if (delta_row == 0 && delta_col == 0) {
      if (fg_params->num_y_points > 0) {
      int luma = 0;
   int luma_x = ((x - 3) << chroma_subsamp_x) + 3;
                              luma = ROUND_POWER_OF_TWO(luma, chroma_subsamp_x + chroma_subsamp_y);
   s0 += luma * c0;
      }
      }
   s0 += filt_cb_grain_block[y + delta_row][x + delta_col] * c0;
   s1 += filt_cr_grain_block[y + delta_row][x + delta_col] * c1;
         }
   filt_cb_grain_block[y][x] = AV1_CLAMP(filt_cb_grain_block[y][x] +
               filt_cr_grain_block[y][x] = AV1_CLAMP(filt_cr_grain_block[y][x] +
                        for ( i = 9; i < luma_block_size_y; i++ )
      for ( j = 9; j < luma_block_size_x; j++ )
         for ( i = 6; i < chroma_block_size_y; i++ )
      for ( j = 6; j < chroma_block_size_x; j++ ) {
      cb_grain_block_tmp[i - 6][j - 6] = filt_cb_grain_block[i][j];
            align_ptr = &fg_buf->luma_grain_block[0][0];
   for ( i = 0; i < 64; i++ ) {
      for ( j = 0; j < 80; j++)
            if (((i + 1) % 4) == 0)
               align_ptr0 = &fg_buf->cb_grain_block[0][0];
   align_ptr1 = &fg_buf->cr_grain_block[0][0];
   for ( i = 0; i < 32; i++) {
      for ( j = 0; j < 40; j++) {
      *align_ptr0++ = cb_grain_block_tmp[i][j];
      }
   if (((i + 1) % 8) == 0) {
      align_ptr0 += 64;
                  memset(fg_buf->scaling_lut_y, 0, sizeof(fg_buf->scaling_lut_y));
   rvcn_av1_film_grain_init_scaling(fg_params->scaling_points_y, fg_params->num_y_points,
         if (fg_params->chroma_scaling_from_luma) {
      memcpy(fg_buf->scaling_lut_cb, fg_buf->scaling_lut_y, sizeof(fg_buf->scaling_lut_y));
      } else {
      memset(fg_buf->scaling_lut_cb, 0, sizeof(fg_buf->scaling_lut_cb));
   memset(fg_buf->scaling_lut_cr, 0, sizeof(fg_buf->scaling_lut_cr));
   rvcn_av1_film_grain_init_scaling(fg_params->scaling_points_cb, fg_params->num_cb_points,
         rvcn_av1_film_grain_init_scaling(fg_params->scaling_points_cr, fg_params->num_cr_points,
         }
      static void rvcn_dec_av1_film_grain_surface(struct pipe_video_buffer **target,
         {
      if (!pic->picture_parameter.film_grain_info.film_grain_info_fields.apply_grain ||
      !pic->film_grain_target)
            }
      static rvcn_dec_message_av1_t get_av1_msg(struct radeon_decoder *dec,
               {
      rvcn_dec_message_av1_t result;
   unsigned i, j;
                     result.frame_header_flags = (pic->picture_parameter.pic_info_fields.show_frame
                  result.frame_header_flags |= (pic->picture_parameter.pic_info_fields.disable_cdf_update
                  result.frame_header_flags |= ((!pic->picture_parameter.pic_info_fields.disable_frame_end_update_cdf)
                  result.frame_header_flags |= ((pic->picture_parameter.pic_info_fields.frame_type ==
                  result.frame_header_flags |= (pic->picture_parameter.pic_info_fields.allow_intrabc
                  result.frame_header_flags |= (pic->picture_parameter.pic_info_fields.allow_high_precision_mv
                  result.frame_header_flags |= (pic->picture_parameter.seq_info_fields.mono_chrome
                  result.frame_header_flags |= (pic->picture_parameter.mode_control_fields.skip_mode_present
                  result.frame_header_flags |= (((pic->picture_parameter.qmatrix_fields.qm_y == 0xf) ? 0 : 1)
                  result.frame_header_flags |= (pic->picture_parameter.seq_info_fields.enable_filter_intra
                  result.frame_header_flags |= (pic->picture_parameter.seq_info_fields.enable_intra_edge_filter
                  result.frame_header_flags |= (pic->picture_parameter.seq_info_fields.enable_interintra_compound
                  result.frame_header_flags |= (pic->picture_parameter.seq_info_fields.enable_masked_compound
                  result.frame_header_flags |= (pic->picture_parameter.pic_info_fields.allow_warped_motion
                  result.frame_header_flags |= (pic->picture_parameter.seq_info_fields.enable_dual_filter
                  result.frame_header_flags |= (pic->picture_parameter.seq_info_fields.enable_order_hint
                  result.frame_header_flags |= (pic->picture_parameter.seq_info_fields.enable_jnt_comp
                  result.frame_header_flags |= (pic->picture_parameter.pic_info_fields.use_ref_frame_mvs
                  result.frame_header_flags |= (pic->picture_parameter.pic_info_fields.allow_screen_content_tools
                  result.frame_header_flags |= (pic->picture_parameter.pic_info_fields.force_integer_mv
                  result.frame_header_flags |= (pic->picture_parameter.loop_filter_info_fields.mode_ref_delta_enabled
                  result.frame_header_flags |= (pic->picture_parameter.loop_filter_info_fields.mode_ref_delta_update
                  result.frame_header_flags |= (pic->picture_parameter.mode_control_fields.delta_q_present_flag
                  result.frame_header_flags |= (pic->picture_parameter.mode_control_fields.delta_lf_present_flag
                  result.frame_header_flags |= (pic->picture_parameter.mode_control_fields.reduced_tx_set_used
                  result.frame_header_flags |= (pic->picture_parameter.seg_info.segment_info_fields.enabled
                  result.frame_header_flags |= (pic->picture_parameter.seg_info.segment_info_fields.update_map
                  result.frame_header_flags |= (pic->picture_parameter.seg_info.segment_info_fields.temporal_update
                  result.frame_header_flags |= (pic->picture_parameter.mode_control_fields.delta_lf_multi
                  result.frame_header_flags |= (pic->picture_parameter.pic_info_fields.is_motion_mode_switchable
                  result.frame_header_flags |= ((!pic->picture_parameter.refresh_frame_flags)
                  result.frame_header_flags |= ((!pic->picture_parameter.seq_info_fields.ref_frame_mvs)
                  result.current_frame_id = pic->picture_parameter.current_frame_id;
            result.profile = pic->picture_parameter.profile;
   result.is_annexb = 0;
   result.frame_type = pic->picture_parameter.pic_info_fields.frame_type;
                     result.sb_size = pic->picture_parameter.seq_info_fields.use_128x128_superblock;
   result.interp_filter = pic->picture_parameter.interp_filter;
   for (i = 0; i < 2; ++i)
         result.filter_level_u = pic->picture_parameter.filter_level_u;
   result.filter_level_v = pic->picture_parameter.filter_level_v;
   result.sharpness_level = pic->picture_parameter.loop_filter_info_fields.sharpness_level;
   for (i = 0; i < 8; ++i)
         for (i = 0; i < 2; ++i)
         result.base_qindex = pic->picture_parameter.base_qindex;
   result.y_dc_delta_q = pic->picture_parameter.y_dc_delta_q;
   result.u_dc_delta_q = pic->picture_parameter.u_dc_delta_q;
   result.v_dc_delta_q = pic->picture_parameter.v_dc_delta_q;
   result.u_ac_delta_q = pic->picture_parameter.u_ac_delta_q;
   result.v_ac_delta_q = pic->picture_parameter.v_ac_delta_q;
   result.qm_y = pic->picture_parameter.qmatrix_fields.qm_y | 0xf0;
   result.qm_u = pic->picture_parameter.qmatrix_fields.qm_u | 0xf0;
   result.qm_v = pic->picture_parameter.qmatrix_fields.qm_v | 0xf0;
   result.delta_q_res = 1 << pic->picture_parameter.mode_control_fields.log2_delta_q_res;
            result.tile_cols = pic->picture_parameter.tile_cols;
   result.tile_rows = pic->picture_parameter.tile_rows;
   result.tx_mode = pic->picture_parameter.mode_control_fields.tx_mode;
   result.reference_mode = (pic->picture_parameter.mode_control_fields.reference_select == 1) ? 2 : 0;
   result.chroma_format = pic->picture_parameter.seq_info_fields.mono_chrome ? 0 : 1;
   result.tile_size_bytes = 0xff;
   result.context_update_tile_id = pic->picture_parameter.context_update_tile_id;
   for (i = 0; i < 65; ++i) {
      result.tile_col_start_sb[i] = pic->picture_parameter.tile_col_start_sb[i];
      }
   result.max_width = pic->picture_parameter.max_width;
   result.max_height = pic->picture_parameter.max_height;
   if (pic->picture_parameter.pic_info_fields.use_superres) {
      result.width = (pic->picture_parameter.frame_width * 8 + pic->picture_parameter.superres_scale_denominator / 2) /
            } else {
      result.width = pic->picture_parameter.frame_width;
      }
   result.height = pic->picture_parameter.frame_height;
   result.superres_upscaled_width = pic->picture_parameter.frame_width;
            for (i = 0; i < NUM_AV1_REFS; ++i) {
      result.ref_frame_map[i] =
      (pic->ref[i]) ? (uintptr_t)vl_video_buffer_get_associated_data(pic->ref[i], &dec->base)
   }
   for (i = 0; i < NUM_AV1_REFS_PER_FRAME; ++i)
                     for (i = 0; i < 8; ++i) {
      for (j = 0; j < 8; ++j)
            }
   memcpy(dec->probs, &pic->picture_parameter.seg_info.feature_data, 128);
            result.cdef_damping = pic->picture_parameter.cdef_damping_minus_3 + 3;
   result.cdef_bits = pic->picture_parameter.cdef_bits;
   for (i = 0; i < 8; ++i) {
      result.cdef_strengths[i] = pic->picture_parameter.cdef_y_strengths[i];
      }
   result.frame_restoration_type[0] = pic->picture_parameter.loop_restoration_fields.yframe_restoration_type;
   result.frame_restoration_type[1] = pic->picture_parameter.loop_restoration_fields.cbframe_restoration_type;
   result.frame_restoration_type[2] = pic->picture_parameter.loop_restoration_fields.crframe_restoration_type;
   for (i = 0; i < 3; ++i) {
      int log2_num = 0;
   int unit_size = pic->picture_parameter.lr_unit_size[i];
   if (unit_size) {
      while (unit_size >>= 1)
            } else {
                     if (pic->picture_parameter.bit_depth_idx) {
      if (target->buffer_format == PIPE_FORMAT_P010 || target->buffer_format == PIPE_FORMAT_P016) {
      result.p010_mode = 1;
      } else {
      result.luma_10to8 = 1;
                  result.preskip_segid = 0;
   result.last_active_segid = 0;
   for (i = 0; i < 8; i++) {
      for (j = 0; j < 8; j++) {
      if (pic->picture_parameter.seg_info.feature_mask[i] & (1 << j)) {
      result.last_active_segid = i;
   if (j >= 5)
                     result.seg_lossless_flag = 0;
   for (i = 0; i < 8; ++i) {
      int av1_get_qindex, qindex;
   int segfeature_active = pic->picture_parameter.seg_info.feature_mask[i] & (1 << 0);
   if (segfeature_active) {
      int seg_qindex = pic->picture_parameter.base_qindex +
            } else {
         }
   qindex = pic->picture_parameter.seg_info.segment_info_fields.enabled ?
               result.seg_lossless_flag |= (((qindex == 0) && result.y_dc_delta_q == 0 &&
                     rvcn_dec_film_grain_params_t* fg_params = &result.film_grain;
   fg_params->apply_grain = pic->picture_parameter.film_grain_info.film_grain_info_fields.apply_grain;
   if (fg_params->apply_grain) {
               fg_params->random_seed = pic->picture_parameter.film_grain_info.grain_seed;
   fg_params->grain_scale_shift =
         fg_params->scaling_shift =
         fg_params->chroma_scaling_from_luma =
         fg_params->num_y_points = pic->picture_parameter.film_grain_info.num_y_points;
   fg_params->num_cb_points = pic->picture_parameter.film_grain_info.num_cb_points;
   fg_params->num_cr_points = pic->picture_parameter.film_grain_info.num_cr_points;
   fg_params->cb_mult = pic->picture_parameter.film_grain_info.cb_mult;
   fg_params->cb_luma_mult = pic->picture_parameter.film_grain_info.cb_luma_mult;
   fg_params->cb_offset = pic->picture_parameter.film_grain_info.cb_offset;
   fg_params->cr_mult = pic->picture_parameter.film_grain_info.cr_mult;
   fg_params->cr_luma_mult = pic->picture_parameter.film_grain_info.cr_luma_mult;
   fg_params->cr_offset = pic->picture_parameter.film_grain_info.cr_offset;
            for (i = 0; i < fg_params->num_y_points; ++i) {
      fg_params->scaling_points_y[i][0] = pic->picture_parameter.film_grain_info.point_y_value[i];
      }
   for (i = 0; i < fg_params->num_cb_points; ++i) {
      fg_params->scaling_points_cb[i][0] = pic->picture_parameter.film_grain_info.point_cb_value[i];
      }
   for (i = 0; i < fg_params->num_cr_points; ++i) {
      fg_params->scaling_points_cr[i][0] = pic->picture_parameter.film_grain_info.point_cr_value[i];
               fg_params->ar_coeff_lag = pic->picture_parameter.film_grain_info.film_grain_info_fields.ar_coeff_lag;
   fg_params->ar_coeff_shift =
            for (i = 0; i < 24; ++i)
            for (i = 0; i < 25; ++i) {
      fg_params->ar_coeffs_cb[i] = pic->picture_parameter.film_grain_info.ar_coeffs_cb[i];
               fg_params->overlap_flag = pic->picture_parameter.film_grain_info.film_grain_info_fields.overlap_flag;
   fg_params->clip_to_restricted_range =
                        result.uncompressed_header_size = 0;
   for (i = 0; i < 7; ++i) {
      result.global_motion[i + 1].wmtype = (rvcn_dec_transformation_type_e)pic->picture_parameter.wm[i].wmtype;
   for (j = 0; j < 6; ++j)
      }
   for (i = 0; i < tile_count && i < 256; ++i) {
      result.tile_info[i].offset = pic->slice_parameter.slice_data_offset[i];
               if (dec->dpb_type == DPB_DYNAMIC_TIER_2) {
      dec->ref_codec.bts = pic->picture_parameter.bit_depth_idx ? CODEC_10_BITS : CODEC_8_BITS;
   dec->ref_codec.index = result.curr_pic_idx;
   dec->ref_codec.ref_size = 8;
   memset(dec->ref_codec.ref_list, 0x7f, sizeof(dec->ref_codec.ref_list));
                  }
      static void rvcn_init_mode_probs(void *prob)
   {
      rvcn_av1_frame_context_t * fc = (rvcn_av1_frame_context_t*)prob;
            memcpy(fc->palette_y_size_cdf, default_palette_y_size_cdf, sizeof(default_palette_y_size_cdf));
   memcpy(fc->palette_uv_size_cdf, default_palette_uv_size_cdf, sizeof(default_palette_uv_size_cdf));
   memcpy(fc->palette_y_color_index_cdf, default_palette_y_color_index_cdf, sizeof(default_palette_y_color_index_cdf));
   memcpy(fc->palette_uv_color_index_cdf, default_palette_uv_color_index_cdf, sizeof(default_palette_uv_color_index_cdf));
   memcpy(fc->kf_y_cdf, default_kf_y_mode_cdf, sizeof(default_kf_y_mode_cdf));
   memcpy(fc->angle_delta_cdf, default_angle_delta_cdf, sizeof(default_angle_delta_cdf));
   memcpy(fc->comp_inter_cdf, default_comp_inter_cdf, sizeof(default_comp_inter_cdf));
   memcpy(fc->comp_ref_type_cdf, default_comp_ref_type_cdf,sizeof(default_comp_ref_type_cdf));
   memcpy(fc->uni_comp_ref_cdf, default_uni_comp_ref_cdf, sizeof(default_uni_comp_ref_cdf));
   memcpy(fc->palette_y_mode_cdf, default_palette_y_mode_cdf, sizeof(default_palette_y_mode_cdf));
   memcpy(fc->palette_uv_mode_cdf, default_palette_uv_mode_cdf, sizeof(default_palette_uv_mode_cdf));
   memcpy(fc->comp_ref_cdf, default_comp_ref_cdf, sizeof(default_comp_ref_cdf));
   memcpy(fc->comp_bwdref_cdf, default_comp_bwdref_cdf, sizeof(default_comp_bwdref_cdf));
   memcpy(fc->single_ref_cdf, default_single_ref_cdf, sizeof(default_single_ref_cdf));
   memcpy(fc->txfm_partition_cdf, default_txfm_partition_cdf, sizeof(default_txfm_partition_cdf));
   memcpy(fc->compound_index_cdf, default_compound_idx_cdfs, sizeof(default_compound_idx_cdfs));
   memcpy(fc->comp_group_idx_cdf, default_comp_group_idx_cdfs, sizeof(default_comp_group_idx_cdfs));
   memcpy(fc->newmv_cdf, default_newmv_cdf, sizeof(default_newmv_cdf));
   memcpy(fc->zeromv_cdf, default_zeromv_cdf, sizeof(default_zeromv_cdf));
   memcpy(fc->refmv_cdf, default_refmv_cdf, sizeof(default_refmv_cdf));
   memcpy(fc->drl_cdf, default_drl_cdf, sizeof(default_drl_cdf));
   memcpy(fc->motion_mode_cdf, default_motion_mode_cdf, sizeof(default_motion_mode_cdf));
   memcpy(fc->obmc_cdf, default_obmc_cdf, sizeof(default_obmc_cdf));
   memcpy(fc->inter_compound_mode_cdf, default_inter_compound_mode_cdf, sizeof(default_inter_compound_mode_cdf));
   memcpy(fc->compound_type_cdf, default_compound_type_cdf, sizeof(default_compound_type_cdf));
   memcpy(fc->wedge_idx_cdf, default_wedge_idx_cdf, sizeof(default_wedge_idx_cdf));
   memcpy(fc->interintra_cdf, default_interintra_cdf, sizeof(default_interintra_cdf));
   memcpy(fc->wedge_interintra_cdf, default_wedge_interintra_cdf, sizeof(default_wedge_interintra_cdf));
   memcpy(fc->interintra_mode_cdf, default_interintra_mode_cdf, sizeof(default_interintra_mode_cdf));
   memcpy(fc->pred_cdf, default_segment_pred_cdf, sizeof(default_segment_pred_cdf));
   memcpy(fc->switchable_restore_cdf, default_switchable_restore_cdf, sizeof(default_switchable_restore_cdf));
   memcpy(fc->wiener_restore_cdf, default_wiener_restore_cdf, sizeof(default_wiener_restore_cdf));
   memcpy(fc->sgrproj_restore_cdf, default_sgrproj_restore_cdf, sizeof(default_sgrproj_restore_cdf));
   memcpy(fc->y_mode_cdf, default_if_y_mode_cdf, sizeof(default_if_y_mode_cdf));
   memcpy(fc->uv_mode_cdf, default_uv_mode_cdf, sizeof(default_uv_mode_cdf));
   memcpy(fc->switchable_interp_cdf, default_switchable_interp_cdf, sizeof(default_switchable_interp_cdf));
   memcpy(fc->partition_cdf, default_partition_cdf, sizeof(default_partition_cdf));
   memcpy(fc->intra_ext_tx_cdf, default_intra_ext_tx_cdf, sizeof(default_intra_ext_tx_cdf));
   memcpy(fc->inter_ext_tx_cdf, default_inter_ext_tx_cdf, sizeof(default_inter_ext_tx_cdf));
   memcpy(fc->skip_cdfs, default_skip_cdfs, sizeof(default_skip_cdfs));
   memcpy(fc->intra_inter_cdf, default_intra_inter_cdf, sizeof(default_intra_inter_cdf));
   memcpy(fc->tree_cdf, default_seg_tree_cdf, sizeof(default_seg_tree_cdf));
   for (i = 0; i < SPATIAL_PREDICTION_PROBS; ++i)
         memcpy(fc->tx_size_cdf, default_tx_size_cdf, sizeof(default_tx_size_cdf));
   memcpy(fc->delta_q_cdf, default_delta_q_cdf, sizeof(default_delta_q_cdf));
   memcpy(fc->skip_mode_cdfs, default_skip_mode_cdfs, sizeof(default_skip_mode_cdfs));
   memcpy(fc->delta_lf_cdf, default_delta_lf_cdf, sizeof(default_delta_lf_cdf));
   memcpy(fc->delta_lf_multi_cdf, default_delta_lf_multi_cdf, sizeof(default_delta_lf_multi_cdf));
   memcpy(fc->cfl_sign_cdf, default_cfl_sign_cdf, sizeof(default_cfl_sign_cdf));
   memcpy(fc->cfl_alpha_cdf, default_cfl_alpha_cdf, sizeof(default_cfl_alpha_cdf));
   memcpy(fc->filter_intra_cdfs, default_filter_intra_cdfs, sizeof(default_filter_intra_cdfs));
   memcpy(fc->filter_intra_mode_cdf, default_filter_intra_mode_cdf, sizeof(default_filter_intra_mode_cdf));
      }
      static void rvcn_vcn4_init_mode_probs(void *prob)
   {
      rvcn_av1_vcn4_frame_context_t * fc = (rvcn_av1_vcn4_frame_context_t*)prob;
            memcpy(fc->palette_y_size_cdf, default_palette_y_size_cdf, sizeof(default_palette_y_size_cdf));
   memcpy(fc->palette_uv_size_cdf, default_palette_uv_size_cdf, sizeof(default_palette_uv_size_cdf));
   memcpy(fc->palette_y_color_index_cdf, default_palette_y_color_index_cdf, sizeof(default_palette_y_color_index_cdf));
   memcpy(fc->palette_uv_color_index_cdf, default_palette_uv_color_index_cdf, sizeof(default_palette_uv_color_index_cdf));
   memcpy(fc->kf_y_cdf, default_kf_y_mode_cdf, sizeof(default_kf_y_mode_cdf));
   memcpy(fc->angle_delta_cdf, default_angle_delta_cdf, sizeof(default_angle_delta_cdf));
   memcpy(fc->comp_inter_cdf, default_comp_inter_cdf, sizeof(default_comp_inter_cdf));
   memcpy(fc->comp_ref_type_cdf, default_comp_ref_type_cdf,sizeof(default_comp_ref_type_cdf));
   memcpy(fc->uni_comp_ref_cdf, default_uni_comp_ref_cdf, sizeof(default_uni_comp_ref_cdf));
   memcpy(fc->palette_y_mode_cdf, default_palette_y_mode_cdf, sizeof(default_palette_y_mode_cdf));
   memcpy(fc->palette_uv_mode_cdf, default_palette_uv_mode_cdf, sizeof(default_palette_uv_mode_cdf));
   memcpy(fc->comp_ref_cdf, default_comp_ref_cdf, sizeof(default_comp_ref_cdf));
   memcpy(fc->comp_bwdref_cdf, default_comp_bwdref_cdf, sizeof(default_comp_bwdref_cdf));
   memcpy(fc->single_ref_cdf, default_single_ref_cdf, sizeof(default_single_ref_cdf));
   memcpy(fc->txfm_partition_cdf, default_txfm_partition_cdf, sizeof(default_txfm_partition_cdf));
   memcpy(fc->compound_index_cdf, default_compound_idx_cdfs, sizeof(default_compound_idx_cdfs));
   memcpy(fc->comp_group_idx_cdf, default_comp_group_idx_cdfs, sizeof(default_comp_group_idx_cdfs));
   memcpy(fc->newmv_cdf, default_newmv_cdf, sizeof(default_newmv_cdf));
   memcpy(fc->zeromv_cdf, default_zeromv_cdf, sizeof(default_zeromv_cdf));
   memcpy(fc->refmv_cdf, default_refmv_cdf, sizeof(default_refmv_cdf));
   memcpy(fc->drl_cdf, default_drl_cdf, sizeof(default_drl_cdf));
   memcpy(fc->motion_mode_cdf, default_motion_mode_cdf, sizeof(default_motion_mode_cdf));
   memcpy(fc->obmc_cdf, default_obmc_cdf, sizeof(default_obmc_cdf));
   memcpy(fc->inter_compound_mode_cdf, default_inter_compound_mode_cdf, sizeof(default_inter_compound_mode_cdf));
   memcpy(fc->compound_type_cdf, default_compound_type_cdf, sizeof(default_compound_type_cdf));
   memcpy(fc->wedge_idx_cdf, default_wedge_idx_cdf, sizeof(default_wedge_idx_cdf));
   memcpy(fc->interintra_cdf, default_interintra_cdf, sizeof(default_interintra_cdf));
   memcpy(fc->wedge_interintra_cdf, default_wedge_interintra_cdf, sizeof(default_wedge_interintra_cdf));
   memcpy(fc->interintra_mode_cdf, default_interintra_mode_cdf, sizeof(default_interintra_mode_cdf));
   memcpy(fc->pred_cdf, default_segment_pred_cdf, sizeof(default_segment_pred_cdf));
   memcpy(fc->switchable_restore_cdf, default_switchable_restore_cdf, sizeof(default_switchable_restore_cdf));
   memcpy(fc->wiener_restore_cdf, default_wiener_restore_cdf, sizeof(default_wiener_restore_cdf));
   memcpy(fc->sgrproj_restore_cdf, default_sgrproj_restore_cdf, sizeof(default_sgrproj_restore_cdf));
   memcpy(fc->y_mode_cdf, default_if_y_mode_cdf, sizeof(default_if_y_mode_cdf));
   memcpy(fc->uv_mode_cdf, default_uv_mode_cdf, sizeof(default_uv_mode_cdf));
   memcpy(fc->switchable_interp_cdf, default_switchable_interp_cdf, sizeof(default_switchable_interp_cdf));
   memcpy(fc->partition_cdf, default_partition_cdf, sizeof(default_partition_cdf));
   memcpy(fc->intra_ext_tx_cdf, &default_intra_ext_tx_cdf[1], sizeof(default_intra_ext_tx_cdf[1]) * 2);
   memcpy(fc->inter_ext_tx_cdf, &default_inter_ext_tx_cdf[1], sizeof(default_inter_ext_tx_cdf[1]) * 3);
   memcpy(fc->skip_cdfs, default_skip_cdfs, sizeof(default_skip_cdfs));
   memcpy(fc->intra_inter_cdf, default_intra_inter_cdf, sizeof(default_intra_inter_cdf));
   memcpy(fc->tree_cdf, default_seg_tree_cdf, sizeof(default_seg_tree_cdf));
   for (i = 0; i < SPATIAL_PREDICTION_PROBS; ++i)
         memcpy(fc->tx_size_cdf, default_tx_size_cdf, sizeof(default_tx_size_cdf));
   memcpy(fc->delta_q_cdf, default_delta_q_cdf, sizeof(default_delta_q_cdf));
   memcpy(fc->skip_mode_cdfs, default_skip_mode_cdfs, sizeof(default_skip_mode_cdfs));
   memcpy(fc->delta_lf_cdf, default_delta_lf_cdf, sizeof(default_delta_lf_cdf));
   memcpy(fc->delta_lf_multi_cdf, default_delta_lf_multi_cdf, sizeof(default_delta_lf_multi_cdf));
   memcpy(fc->cfl_sign_cdf, default_cfl_sign_cdf, sizeof(default_cfl_sign_cdf));
   memcpy(fc->cfl_alpha_cdf, default_cfl_alpha_cdf, sizeof(default_cfl_alpha_cdf));
   memcpy(fc->filter_intra_cdfs, default_filter_intra_cdfs, sizeof(default_filter_intra_cdfs));
   memcpy(fc->filter_intra_mode_cdf, default_filter_intra_mode_cdf, sizeof(default_filter_intra_mode_cdf));
      }
      static void rvcn_av1_init_mv_probs(void *prob)
   {
               memcpy(fc->nmvc_joints_cdf, default_nmv_context.joints_cdf, sizeof(default_nmv_context.joints_cdf));
   memcpy(fc->nmvc_0_bits_cdf, default_nmv_context.comps[0].bits_cdf, sizeof(default_nmv_context.comps[0].bits_cdf));
   memcpy(fc->nmvc_0_class0_cdf, default_nmv_context.comps[0].class0_cdf, sizeof(default_nmv_context.comps[0].class0_cdf));
   memcpy(fc->nmvc_0_class0_fp_cdf, default_nmv_context.comps[0].class0_fp_cdf, sizeof(default_nmv_context.comps[0].class0_fp_cdf));
   memcpy(fc->nmvc_0_class0_hp_cdf, default_nmv_context.comps[0].class0_hp_cdf, sizeof(default_nmv_context.comps[0].class0_hp_cdf));
   memcpy(fc->nmvc_0_classes_cdf, default_nmv_context.comps[0].classes_cdf, sizeof(default_nmv_context.comps[0].classes_cdf));
   memcpy(fc->nmvc_0_fp_cdf, default_nmv_context.comps[0].fp_cdf, sizeof(default_nmv_context.comps[0].fp_cdf));
   memcpy(fc->nmvc_0_hp_cdf, default_nmv_context.comps[0].hp_cdf, sizeof(default_nmv_context.comps[0].hp_cdf));
   memcpy(fc->nmvc_0_sign_cdf, default_nmv_context.comps[0].sign_cdf, sizeof(default_nmv_context.comps[0].sign_cdf));
   memcpy(fc->nmvc_1_bits_cdf, default_nmv_context.comps[1].bits_cdf, sizeof(default_nmv_context.comps[1].bits_cdf));
   memcpy(fc->nmvc_1_class0_cdf, default_nmv_context.comps[1].class0_cdf, sizeof(default_nmv_context.comps[1].class0_cdf));
   memcpy(fc->nmvc_1_class0_fp_cdf, default_nmv_context.comps[1].class0_fp_cdf, sizeof(default_nmv_context.comps[1].class0_fp_cdf));
   memcpy(fc->nmvc_1_class0_hp_cdf, default_nmv_context.comps[1].class0_hp_cdf, sizeof(default_nmv_context.comps[1].class0_hp_cdf));
   memcpy(fc->nmvc_1_classes_cdf, default_nmv_context.comps[1].classes_cdf, sizeof(default_nmv_context.comps[1].classes_cdf));
   memcpy(fc->nmvc_1_fp_cdf, default_nmv_context.comps[1].fp_cdf, sizeof(default_nmv_context.comps[1].fp_cdf));
   memcpy(fc->nmvc_1_hp_cdf, default_nmv_context.comps[1].hp_cdf, sizeof(default_nmv_context.comps[1].hp_cdf));
   memcpy(fc->nmvc_1_sign_cdf, default_nmv_context.comps[1].sign_cdf, sizeof(default_nmv_context.comps[1].sign_cdf));
   memcpy(fc->ndvc_joints_cdf, default_nmv_context.joints_cdf, sizeof(default_nmv_context.joints_cdf));
   memcpy(fc->ndvc_0_bits_cdf, default_nmv_context.comps[0].bits_cdf, sizeof(default_nmv_context.comps[0].bits_cdf));
   memcpy(fc->ndvc_0_class0_cdf, default_nmv_context.comps[0].class0_cdf, sizeof(default_nmv_context.comps[0].class0_cdf));
   memcpy(fc->ndvc_0_class0_fp_cdf, default_nmv_context.comps[0].class0_fp_cdf, sizeof(default_nmv_context.comps[0].class0_fp_cdf));
   memcpy(fc->ndvc_0_class0_hp_cdf, default_nmv_context.comps[0].class0_hp_cdf, sizeof(default_nmv_context.comps[0].class0_hp_cdf));
   memcpy(fc->ndvc_0_classes_cdf, default_nmv_context.comps[0].classes_cdf, sizeof(default_nmv_context.comps[0].classes_cdf));
   memcpy(fc->ndvc_0_fp_cdf, default_nmv_context.comps[0].fp_cdf, sizeof(default_nmv_context.comps[0].fp_cdf));
   memcpy(fc->ndvc_0_hp_cdf, default_nmv_context.comps[0].hp_cdf, sizeof(default_nmv_context.comps[0].hp_cdf));
   memcpy(fc->ndvc_0_sign_cdf, default_nmv_context.comps[0].sign_cdf, sizeof(default_nmv_context.comps[0].sign_cdf));
   memcpy(fc->ndvc_1_bits_cdf, default_nmv_context.comps[1].bits_cdf, sizeof(default_nmv_context.comps[1].bits_cdf));
   memcpy(fc->ndvc_1_class0_cdf, default_nmv_context.comps[1].class0_cdf, sizeof(default_nmv_context.comps[1].class0_cdf));
   memcpy(fc->ndvc_1_class0_fp_cdf, default_nmv_context.comps[1].class0_fp_cdf, sizeof(default_nmv_context.comps[1].class0_fp_cdf));
   memcpy(fc->ndvc_1_class0_hp_cdf, default_nmv_context.comps[1].class0_hp_cdf, sizeof(default_nmv_context.comps[1].class0_hp_cdf));
   memcpy(fc->ndvc_1_classes_cdf, default_nmv_context.comps[1].classes_cdf, sizeof(default_nmv_context.comps[1].classes_cdf));
   memcpy(fc->ndvc_1_fp_cdf, default_nmv_context.comps[1].fp_cdf, sizeof(default_nmv_context.comps[1].fp_cdf));
   memcpy(fc->ndvc_1_hp_cdf, default_nmv_context.comps[1].hp_cdf, sizeof(default_nmv_context.comps[1].hp_cdf));
      }
      static void rvcn_vcn4_av1_init_mv_probs(void *prob)
   {
               memcpy(fc->nmvc_joints_cdf, default_nmv_context.joints_cdf, sizeof(default_nmv_context.joints_cdf));
   memcpy(fc->nmvc_0_bits_cdf, default_nmv_context.comps[0].bits_cdf, sizeof(default_nmv_context.comps[0].bits_cdf));
   memcpy(fc->nmvc_0_class0_cdf, default_nmv_context.comps[0].class0_cdf, sizeof(default_nmv_context.comps[0].class0_cdf));
   memcpy(fc->nmvc_0_class0_fp_cdf, default_nmv_context.comps[0].class0_fp_cdf, sizeof(default_nmv_context.comps[0].class0_fp_cdf));
   memcpy(fc->nmvc_0_class0_hp_cdf, default_nmv_context.comps[0].class0_hp_cdf, sizeof(default_nmv_context.comps[0].class0_hp_cdf));
   memcpy(fc->nmvc_0_classes_cdf, default_nmv_context.comps[0].classes_cdf, sizeof(default_nmv_context.comps[0].classes_cdf));
   memcpy(fc->nmvc_0_fp_cdf, default_nmv_context.comps[0].fp_cdf, sizeof(default_nmv_context.comps[0].fp_cdf));
   memcpy(fc->nmvc_0_hp_cdf, default_nmv_context.comps[0].hp_cdf, sizeof(default_nmv_context.comps[0].hp_cdf));
   memcpy(fc->nmvc_0_sign_cdf, default_nmv_context.comps[0].sign_cdf, sizeof(default_nmv_context.comps[0].sign_cdf));
   memcpy(fc->nmvc_1_bits_cdf, default_nmv_context.comps[1].bits_cdf, sizeof(default_nmv_context.comps[1].bits_cdf));
   memcpy(fc->nmvc_1_class0_cdf, default_nmv_context.comps[1].class0_cdf, sizeof(default_nmv_context.comps[1].class0_cdf));
   memcpy(fc->nmvc_1_class0_fp_cdf, default_nmv_context.comps[1].class0_fp_cdf, sizeof(default_nmv_context.comps[1].class0_fp_cdf));
   memcpy(fc->nmvc_1_class0_hp_cdf, default_nmv_context.comps[1].class0_hp_cdf, sizeof(default_nmv_context.comps[1].class0_hp_cdf));
   memcpy(fc->nmvc_1_classes_cdf, default_nmv_context.comps[1].classes_cdf, sizeof(default_nmv_context.comps[1].classes_cdf));
   memcpy(fc->nmvc_1_fp_cdf, default_nmv_context.comps[1].fp_cdf, sizeof(default_nmv_context.comps[1].fp_cdf));
   memcpy(fc->nmvc_1_hp_cdf, default_nmv_context.comps[1].hp_cdf, sizeof(default_nmv_context.comps[1].hp_cdf));
   memcpy(fc->nmvc_1_sign_cdf, default_nmv_context.comps[1].sign_cdf, sizeof(default_nmv_context.comps[1].sign_cdf));
   memcpy(fc->ndvc_joints_cdf, default_nmv_context.joints_cdf, sizeof(default_nmv_context.joints_cdf));
   memcpy(fc->ndvc_0_bits_cdf, default_nmv_context.comps[0].bits_cdf, sizeof(default_nmv_context.comps[0].bits_cdf));
   memcpy(fc->ndvc_0_class0_cdf, default_nmv_context.comps[0].class0_cdf, sizeof(default_nmv_context.comps[0].class0_cdf));
   memcpy(fc->ndvc_0_class0_fp_cdf, default_nmv_context.comps[0].class0_fp_cdf, sizeof(default_nmv_context.comps[0].class0_fp_cdf));
   memcpy(fc->ndvc_0_class0_hp_cdf, default_nmv_context.comps[0].class0_hp_cdf, sizeof(default_nmv_context.comps[0].class0_hp_cdf));
   memcpy(fc->ndvc_0_classes_cdf, default_nmv_context.comps[0].classes_cdf, sizeof(default_nmv_context.comps[0].classes_cdf));
   memcpy(fc->ndvc_0_fp_cdf, default_nmv_context.comps[0].fp_cdf, sizeof(default_nmv_context.comps[0].fp_cdf));
   memcpy(fc->ndvc_0_hp_cdf, default_nmv_context.comps[0].hp_cdf, sizeof(default_nmv_context.comps[0].hp_cdf));
   memcpy(fc->ndvc_0_sign_cdf, default_nmv_context.comps[0].sign_cdf, sizeof(default_nmv_context.comps[0].sign_cdf));
   memcpy(fc->ndvc_1_bits_cdf, default_nmv_context.comps[1].bits_cdf, sizeof(default_nmv_context.comps[1].bits_cdf));
   memcpy(fc->ndvc_1_class0_cdf, default_nmv_context.comps[1].class0_cdf, sizeof(default_nmv_context.comps[1].class0_cdf));
   memcpy(fc->ndvc_1_class0_fp_cdf, default_nmv_context.comps[1].class0_fp_cdf, sizeof(default_nmv_context.comps[1].class0_fp_cdf));
   memcpy(fc->ndvc_1_class0_hp_cdf, default_nmv_context.comps[1].class0_hp_cdf, sizeof(default_nmv_context.comps[1].class0_hp_cdf));
   memcpy(fc->ndvc_1_classes_cdf, default_nmv_context.comps[1].classes_cdf, sizeof(default_nmv_context.comps[1].classes_cdf));
   memcpy(fc->ndvc_1_fp_cdf, default_nmv_context.comps[1].fp_cdf, sizeof(default_nmv_context.comps[1].fp_cdf));
   memcpy(fc->ndvc_1_hp_cdf, default_nmv_context.comps[1].hp_cdf, sizeof(default_nmv_context.comps[1].hp_cdf));
      }
      static void rvcn_av1_default_coef_probs(void *prob, int index)
   {
               memcpy(fc->txb_skip_cdf, av1_default_txb_skip_cdfs[index], sizeof(av1_default_txb_skip_cdfs[index]));
   memcpy(fc->eob_extra_cdf, av1_default_eob_extra_cdfs[index], sizeof(av1_default_eob_extra_cdfs[index]));
   memcpy(fc->dc_sign_cdf, av1_default_dc_sign_cdfs[index], sizeof(av1_default_dc_sign_cdfs[index]));
   memcpy(fc->coeff_br_cdf, av1_default_coeff_lps_multi_cdfs[index], sizeof(av1_default_coeff_lps_multi_cdfs[index]));
   memcpy(fc->coeff_base_cdf, av1_default_coeff_base_multi_cdfs[index], sizeof(av1_default_coeff_base_multi_cdfs[index]));
   memcpy(fc->coeff_base_eob_cdf, av1_default_coeff_base_eob_multi_cdfs[index], sizeof(av1_default_coeff_base_eob_multi_cdfs[index]));
   memcpy(fc->eob_flag_cdf16, av1_default_eob_multi16_cdfs[index], sizeof(av1_default_eob_multi16_cdfs[index]));
   memcpy(fc->eob_flag_cdf32, av1_default_eob_multi32_cdfs[index], sizeof(av1_default_eob_multi32_cdfs[index]));
   memcpy(fc->eob_flag_cdf64, av1_default_eob_multi64_cdfs[index], sizeof(av1_default_eob_multi64_cdfs[index]));
   memcpy(fc->eob_flag_cdf128, av1_default_eob_multi128_cdfs[index], sizeof(av1_default_eob_multi128_cdfs[index]));
   memcpy(fc->eob_flag_cdf256, av1_default_eob_multi256_cdfs[index], sizeof(av1_default_eob_multi256_cdfs[index]));
   memcpy(fc->eob_flag_cdf512, av1_default_eob_multi512_cdfs[index], sizeof(av1_default_eob_multi512_cdfs[index]));
      }
      static void rvcn_vcn4_av1_default_coef_probs(void *prob, int index)
   {
      rvcn_av1_vcn4_frame_context_t *fc = (rvcn_av1_vcn4_frame_context_t*)prob;
   void *p;
   int i, j;
                     p = (void *)fc->eob_extra_cdf;
   size = sizeof(av1_default_eob_extra_cdfs[0][0][0][0]) * EOB_COEF_CONTEXTS_VCN4;
   for (i = 0; i < AV1_TX_SIZES; i++) {
      for ( j = 0; j < AV1_PLANE_TYPES; j++) {
      memcpy(p, &av1_default_eob_extra_cdfs[index][i][j][3], size);
                  memcpy(fc->dc_sign_cdf, av1_default_dc_sign_cdfs[index], sizeof(av1_default_dc_sign_cdfs[index]));
   memcpy(fc->coeff_br_cdf, av1_default_coeff_lps_multi_cdfs[index], sizeof(av1_default_coeff_lps_multi_cdfs[index]));
   memcpy(fc->coeff_base_cdf, av1_default_coeff_base_multi_cdfs[index], sizeof(av1_default_coeff_base_multi_cdfs[index]));
   memcpy(fc->coeff_base_eob_cdf, av1_default_coeff_base_eob_multi_cdfs[index], sizeof(av1_default_coeff_base_eob_multi_cdfs[index]));
   memcpy(fc->eob_flag_cdf16, av1_default_eob_multi16_cdfs[index], sizeof(av1_default_eob_multi16_cdfs[index]));
   memcpy(fc->eob_flag_cdf32, av1_default_eob_multi32_cdfs[index], sizeof(av1_default_eob_multi32_cdfs[index]));
   memcpy(fc->eob_flag_cdf64, av1_default_eob_multi64_cdfs[index], sizeof(av1_default_eob_multi64_cdfs[index]));
   memcpy(fc->eob_flag_cdf128, av1_default_eob_multi128_cdfs[index], sizeof(av1_default_eob_multi128_cdfs[index]));
   memcpy(fc->eob_flag_cdf256, av1_default_eob_multi256_cdfs[index], sizeof(av1_default_eob_multi256_cdfs[index]));
   memcpy(fc->eob_flag_cdf512, av1_default_eob_multi512_cdfs[index], sizeof(av1_default_eob_multi512_cdfs[index]));
      }
      static unsigned calc_ctx_size_h265_main(struct radeon_decoder *dec)
   {
      unsigned width = align(dec->base.width, VL_MACROBLOCK_WIDTH);
                     if (dec->base.width * dec->base.height >= 4096 * 2000)
         else
            width = align(width, 16);
   height = align(height, 16);
      }
      static unsigned calc_ctx_size_h265_main10(struct radeon_decoder *dec,
         {
      unsigned log2_ctb_size, width_in_ctb, height_in_ctb, num_16x16_block_per_ctb;
   unsigned context_buffer_size_per_ctb_row, cm_buffer_size, max_mb_address, db_left_tile_pxl_size;
            unsigned width = align(dec->base.width, VL_MACROBLOCK_WIDTH);
   unsigned height = align(dec->base.height, VL_MACROBLOCK_HEIGHT);
   unsigned coeff_10bit =
                     if (dec->base.width * dec->base.height >= 4096 * 2000)
         else
            log2_ctb_size = pic->pps->sps->log2_min_luma_coding_block_size_minus3 + 3 +
            width_in_ctb = (width + ((1 << log2_ctb_size) - 1)) >> log2_ctb_size;
            num_16x16_block_per_ctb = ((1 << log2_ctb_size) >> 4) * ((1 << log2_ctb_size) >> 4);
   context_buffer_size_per_ctb_row = align(width_in_ctb * num_16x16_block_per_ctb * 16, 256);
            cm_buffer_size = max_references * context_buffer_size_per_ctb_row * height_in_ctb;
               }
      static rvcn_dec_message_vc1_t get_vc1_msg(struct pipe_vc1_picture_desc *pic)
   {
               memset(&result, 0, sizeof(result));
   switch (pic->base.profile) {
   case PIPE_VIDEO_PROFILE_VC1_SIMPLE:
      result.profile = RDECODE_VC1_PROFILE_SIMPLE;
   result.level = 1;
         case PIPE_VIDEO_PROFILE_VC1_MAIN:
      result.profile = RDECODE_VC1_PROFILE_MAIN;
   result.level = 2;
         case PIPE_VIDEO_PROFILE_VC1_ADVANCED:
      result.profile = RDECODE_VC1_PROFILE_ADVANCED;
   result.level = 4;
         default:
                  result.sps_info_flags |= pic->postprocflag << 7;
   result.sps_info_flags |= pic->pulldown << 6;
   result.sps_info_flags |= pic->interlace << 5;
   result.sps_info_flags |= pic->tfcntrflag << 4;
   result.sps_info_flags |= pic->finterpflag << 3;
            result.pps_info_flags |= pic->range_mapy_flag << 31;
   result.pps_info_flags |= pic->range_mapy << 28;
   result.pps_info_flags |= pic->range_mapuv_flag << 27;
   result.pps_info_flags |= pic->range_mapuv << 24;
   result.pps_info_flags |= pic->multires << 21;
   result.pps_info_flags |= pic->maxbframes << 16;
   result.pps_info_flags |= pic->overlap << 11;
   result.pps_info_flags |= pic->quantizer << 9;
   result.pps_info_flags |= pic->panscan_flag << 7;
   result.pps_info_flags |= pic->refdist_flag << 6;
            if (pic->base.profile != PIPE_VIDEO_PROFILE_VC1_SIMPLE) {
      result.pps_info_flags |= pic->syncmarker << 20;
   result.pps_info_flags |= pic->rangered << 19;
   result.pps_info_flags |= pic->loopfilter << 5;
   result.pps_info_flags |= pic->fastuvmc << 4;
   result.pps_info_flags |= pic->extended_mv << 3;
   result.pps_info_flags |= pic->extended_dmv << 8;
                           }
      static uint32_t get_ref_pic_idx(struct radeon_decoder *dec, struct pipe_video_buffer *ref)
   {
      uint32_t min = MAX2(dec->frame_number, NUM_MPEG2_REFS) - NUM_MPEG2_REFS;
   uint32_t max = MAX2(dec->frame_number, 1) - 1;
            /* seems to be the most sane fallback */
   if (!ref)
            /* get the frame number from the associated data */
            /* limit the frame number to a valid range */
      }
      static rvcn_dec_message_mpeg2_vld_t get_mpeg2_msg(struct radeon_decoder *dec,
         {
      const int *zscan = pic->alternate_scan ? vl_zscan_alternate : vl_zscan_normal;
   rvcn_dec_message_mpeg2_vld_t result;
            memset(&result, 0, sizeof(result));
            result.forward_ref_pic_idx = get_ref_pic_idx(dec, pic->ref[0]);
            if (pic->intra_matrix) {
      result.load_intra_quantiser_matrix = 1;
   for (i = 0; i < 64; ++i) {
            }
   if (pic->non_intra_matrix) {
      result.load_nonintra_quantiser_matrix = 1;
   for (i = 0; i < 64; ++i) {
                     result.profile_and_level_indication = 0;
            result.picture_coding_type = pic->picture_coding_type;
   result.f_code[0][0] = pic->f_code[0][0] + 1;
   result.f_code[0][1] = pic->f_code[0][1] + 1;
   result.f_code[1][0] = pic->f_code[1][0] + 1;
   result.f_code[1][1] = pic->f_code[1][1] + 1;
   result.intra_dc_precision = pic->intra_dc_precision;
   result.pic_structure = pic->picture_structure;
   result.top_field_first = pic->top_field_first;
   result.frame_pred_frame_dct = pic->frame_pred_frame_dct;
   result.concealment_motion_vectors = pic->concealment_motion_vectors;
   result.q_scale_type = pic->q_scale_type;
   result.intra_vlc_format = pic->intra_vlc_format;
               }
      static rvcn_dec_message_mpeg4_asp_vld_t get_mpeg4_msg(struct radeon_decoder *dec,
         {
      rvcn_dec_message_mpeg4_asp_vld_t result;
            memset(&result, 0, sizeof(result));
            result.forward_ref_pic_idx = get_ref_pic_idx(dec, pic->ref[0]);
            result.variant_type = 0;
            result.video_object_layer_verid = 0x5;
            result.video_object_layer_width = dec->base.width;
                     result.short_video_header = pic->short_video_header;
   result.interlaced = pic->interlaced;
   result.load_intra_quant_mat = 1;
   result.load_nonintra_quant_mat = 1;
   result.quarter_sample = pic->quarter_sample;
   result.complexity_estimation_disable = 1;
   result.resync_marker_disable = pic->resync_marker_disable;
   result.newpred_enable = 0;
                     for (i = 0; i < 64; ++i) {
      result.intra_quant_mat[i] = pic->intra_matrix[vl_zscan_normal[i]];
                  }
      static void rvcn_dec_message_create(struct radeon_decoder *dec)
   {
      rvcn_dec_message_header_t *header = dec->msg;
   rvcn_dec_message_create_t *create = dec->msg + sizeof(rvcn_dec_message_header_t);
            memset(dec->msg, 0, sizes);
   header->header_size = sizeof(rvcn_dec_message_header_t);
   header->total_size = sizes;
   header->num_buffers = 1;
   header->msg_type = RDECODE_MSG_CREATE;
   header->stream_handle = dec->stream_handle;
            header->index[0].message_id = RDECODE_MESSAGE_CREATE;
   header->index[0].offset = sizeof(rvcn_dec_message_header_t);
   header->index[0].size = sizeof(rvcn_dec_message_create_t);
            create->stream_type = dec->stream_type;
   create->session_flags = 0;
   create->width_in_samples = dec->base.width;
      }
      static unsigned rvcn_dec_dynamic_dpb_t2_message(struct radeon_decoder *dec, rvcn_dec_message_decode_t *decode,
         {
      struct rvcn_dec_dynamic_dpb_t2 *dpb = NULL, *dummy = NULL;
   unsigned width, height, size;
   uint64_t addr;
            width = align(decode->width_in_samples, dec->db_alignment);
   height = align(decode->height_in_samples, dec->db_alignment);
   size = align((width * height * 3) / 2, 256);
   if (dec->ref_codec.bts == CODEC_10_BITS)
            list_for_each_entry_safe(struct rvcn_dec_dynamic_dpb_t2, d, &dec->dpb_ref_list, list) {
      for (i = 0; i < dec->ref_codec.ref_size; ++i) {
      if (((dec->ref_codec.ref_list[i] & 0x7f) != 0x7f) && (d->index == (dec->ref_codec.ref_list[i] & 0x7f))) {
                     addr = dec->ws->buffer_get_virtual_address(d->dpb.res->buf);
   if (!addr && dummy) {
      RVID_ERR("Ref list from application is incorrect, using dummy buffer instead.\n");
      }
   dynamic_dpb_t2->dpbAddrLo[i] = addr;
   dynamic_dpb_t2->dpbAddrHi[i] = addr >> 32;
   ++dynamic_dpb_t2->dpbArraySize;
         }
   if (i == dec->ref_codec.ref_size) {
      if (d->dpb.res->b.b.width0 * d->dpb.res->b.b.height0 != size) {
      list_del(&d->list);
      } else {
                        list_for_each_entry_safe(struct rvcn_dec_dynamic_dpb_t2, d, &dec->dpb_ref_list, list) {
      if (d->dpb.res->b.b.width0 * d->dpb.res->b.b.height0 == size && d->index == dec->ref_codec.index) {
      dpb = d;
                  if (!dpb) {
      list_for_each_entry_safe(struct rvcn_dec_dynamic_dpb_t2, d, &dec->dpb_ref_list, list) {
      if (d->index == 0x7f) {
      d->index = dec->ref_codec.index;
   dpb = d;
                     list_for_each_entry_safe(struct rvcn_dec_dynamic_dpb_t2, d, &dec->dpb_unref_list, list) {
      if (dec->prev_fence)
         list_del(&d->list);
   si_vid_destroy_buffer(&d->dpb);
               if (!dpb) {
      bool r;
   dpb = CALLOC_STRUCT(rvcn_dec_dynamic_dpb_t2);
   if (!dpb)
         dpb->index = dec->ref_codec.index;
   if (encrypted)
         else
                  if (!r) {
      RVID_ERR("Can't allocated dpb buffer.\n");
   FREE(dpb);
      }
               dec->ws->cs_add_buffer(&dec->cs, dpb->dpb.res->buf,
         addr = dec->ws->buffer_get_virtual_address(dpb->dpb.res->buf);
   dynamic_dpb_t2->dpbCurrLo = addr;
            decode->decode_flags = 1;
   dynamic_dpb_t2->dpbConfigFlags = 0;
   dynamic_dpb_t2->dpbLumaPitch = align(decode->width_in_samples, dec->db_alignment);
   dynamic_dpb_t2->dpbLumaAlignedHeight = align(decode->height_in_samples, dec->db_alignment);
   dynamic_dpb_t2->dpbLumaAlignedSize = dynamic_dpb_t2->dpbLumaPitch *
         dynamic_dpb_t2->dpbChromaPitch = dynamic_dpb_t2->dpbLumaPitch >> 1;
   dynamic_dpb_t2->dpbChromaAlignedHeight = dynamic_dpb_t2->dpbLumaAlignedHeight >> 1;
   dynamic_dpb_t2->dpbChromaAlignedSize = dynamic_dpb_t2->dpbChromaPitch *
            if (dec->ref_codec.bts == CODEC_10_BITS) {
      dynamic_dpb_t2->dpbLumaAlignedSize = dynamic_dpb_t2->dpbLumaAlignedSize * 3 / 2;
                  }
      static struct pb_buffer *rvcn_dec_message_decode(struct radeon_decoder *dec,
               {
      DECRYPT_PARAMETERS *decrypt = (DECRYPT_PARAMETERS *)picture->decrypt_key;
   bool encrypted = picture->protected_playback;
   struct si_texture *luma;
   struct si_texture *chroma;
   struct pipe_video_buffer *out_surf = target;
   ASSERTED struct si_screen *sscreen = (struct si_screen *)dec->screen;
   rvcn_dec_message_header_t *header;
   rvcn_dec_message_index_t *index_codec;
   rvcn_dec_message_index_t *index_drm = NULL;
   rvcn_dec_message_index_t *index_dynamic_dpb = NULL;
   rvcn_dec_message_decode_t *decode;
   unsigned sizes = 0, offset_decode, offset_codec;
   unsigned offset_drm = 0, offset_dynamic_dpb = 0;
   void *codec;
   rvcn_dec_message_drm_t *drm = NULL;
   rvcn_dec_message_dynamic_dpb_t *dynamic_dpb = NULL;
   rvcn_dec_message_dynamic_dpb_t2_t *dynamic_dpb_t2 = NULL;
   bool dpb_resize = false;
   header = dec->msg;
            index_codec = (void*)header + sizes;
            if (encrypted) {
      index_drm = (void*)header + sizes;
               if (dec->dpb_type >= DPB_DYNAMIC_TIER_1) {
      index_dynamic_dpb = (void*)header + sizes;
               offset_decode = sizes;
   decode = (void*)header + sizes;
            if (encrypted) {
      offset_drm = sizes;
   drm = (void*)header + sizes;
               if (dec->dpb_type >= DPB_DYNAMIC_TIER_1) {
      offset_dynamic_dpb = sizes;
   if (dec->dpb_type == DPB_DYNAMIC_TIER_1) {
      dynamic_dpb = (void*)header + sizes;
      }
   else if (dec->dpb_type == DPB_DYNAMIC_TIER_2) {
      dynamic_dpb_t2 = (void*)header + sizes;
                  offset_codec = sizes;
            memset(dec->msg, 0, sizes);
   header->header_size = sizeof(rvcn_dec_message_header_t);
   header->total_size = sizes;
   header->msg_type = RDECODE_MSG_DECODE;
   header->stream_handle = dec->stream_handle;
            header->index[0].message_id = RDECODE_MESSAGE_DECODE;
   header->index[0].offset = offset_decode;
   header->index[0].size = sizeof(rvcn_dec_message_decode_t);
   header->index[0].filled = 0;
            index_codec->offset = offset_codec;
   index_codec->size = sizeof(rvcn_dec_message_avc_t);
   index_codec->filled = 0;
            if (encrypted) {
      index_drm->message_id = RDECODE_MESSAGE_DRM;
   index_drm->offset = offset_drm;
   index_drm->size = sizeof(rvcn_dec_message_drm_t);
   index_drm->filled = 0;
               if (dec->dpb_type >= DPB_DYNAMIC_TIER_1) {
      index_dynamic_dpb->message_id = RDECODE_MESSAGE_DYNAMIC_DPB;
   index_dynamic_dpb->offset = offset_dynamic_dpb;
   index_dynamic_dpb->filled = 0;
   ++header->num_buffers;
   if (dec->dpb_type == DPB_DYNAMIC_TIER_1)
         else if (dec->dpb_type == DPB_DYNAMIC_TIER_2)
               decode->stream_type = dec->stream_type;
   decode->decode_flags = 0;
   decode->width_in_samples = dec->base.width;
                     if (dec->dpb_type != DPB_DYNAMIC_TIER_2) {
      bool r;
   if (!dec->dpb.res && dec->dpb_size) {
      if (encrypted) {
         } else {
         }
   assert(encrypted == (bool)(dec->dpb.res->flags & RADEON_FLAG_ENCRYPTED));
   if (!r) {
      RVID_ERR("Can't allocated dpb.\n");
      }
      } else if (dec->dpb_type == DPB_DYNAMIC_TIER_1 && dec->dpb.res &&
                     buf_offset_info.num_units = (NUM_VP9_REFS + 1);
   buf_offset_info.old_offset = (align(dec->max_width, dec->db_alignment) *
                        dec->dpb_size = calc_dpb_size(dec);
   r = si_vid_resize_buffer(dec->screen, &dec->cs, &dec->dpb, dec->dpb_size, &buf_offset_info);
   if (!r) {
      }
            dec->max_width = dec->base.width;
   dec->max_height = dec->base.height;
                  if (!dec->ctx.res) {
      enum pipe_video_format fmt = u_reduce_video_profile(picture->profile);
   if (dec->stream_type == RDECODE_CODEC_H264_PERF) {
      unsigned ctx_size = calc_ctx_size_h264_perf(dec);
   bool r;
   if (encrypted && dec->tmz_ctx) {
         } else {
                        if (!r) {
      RVID_ERR("Can't allocated context buffer.\n");
      }
      } else if (fmt == PIPE_VIDEO_FORMAT_VP9) {
      unsigned ctx_size;
                                 if (((struct si_screen *)dec->screen)->info.vcn_ip_version >= VCN_2_0_0) {
      /* SRE collocated context data */
   ctx_size += 32 * 2 * 128 * 68;
   /* SMP collocated context data */
   ctx_size += 9 * 64 * 2 * 128 * 68;
   /* SDB left tile pixel */
      } else {
      ctx_size += 32 * 2 * 64 * 64;
   ctx_size += 9 * 64 * 2 * 64 * 64;
                              if (encrypted && dec->tmz_ctx) {
         } else {
         }
   if (!r) {
      RVID_ERR("Can't allocated context buffer.\n");
                     /* ctx needs probs table */
   ptr = dec->ws->buffer_map(dec->ws, dec->ctx.res->buf, &dec->cs,
         fill_probs_table(ptr);
   dec->ws->buffer_unmap(dec->ws, dec->ctx.res->buf);
      } else if (fmt == PIPE_VIDEO_FORMAT_HEVC) {
      unsigned ctx_size;
   bool r;
   if (dec->base.profile == PIPE_VIDEO_PROFILE_HEVC_MAIN_10)
                        if (encrypted && dec->tmz_ctx) {
         } else {
         }
   if (!r) {
      RVID_ERR("Can't allocated context buffer.\n");
      }
         }
   if (encrypted != dec->ws->cs_is_secure(&dec->cs)) {
                  if (dec->stream_type == RDECODE_CODEC_AV1)
            luma   = (struct si_texture *)((struct vl_video_buffer *)out_surf)->resources[0];
                     /* When texture being created, the bo will be created with total size of planes,
   * and all planes point to the same buffer */
   assert(si_resource(((struct vl_video_buffer *)out_surf)->resources[0])->buf->size ==
                     decode->sct_size = 0;
            decode->sw_ctxt_size = RDECODE_SESSION_CONTEXT_SIZE;
            if ((((struct si_screen*)dec->screen)->info.vcn_ip_version >= VCN_3_0_0) &&
      (dec->stream_type == RDECODE_CODEC_VP9 || dec->stream_type == RDECODE_CODEC_AV1 ||
   dec->base.profile == PIPE_VIDEO_PROFILE_HEVC_MAIN_10))
         decode->db_surf_tile_config = 0;
            decode->dt_pitch = luma->surface.u.gfx9.surf_pitch * luma->surface.blk_w;
            if (luma->surface.meta_offset) {
      RVID_ERR("DCC surfaces not supported.\n");
               decode->dt_tiling_mode = 0;
   decode->dt_swizzle_mode = luma->surface.u.gfx9.swizzle_mode;
   decode->dt_array_mode = dec->addr_gfx_mode;
   decode->dt_field_mode = ((struct vl_video_buffer *)out_surf)->base.interlaced;
   decode->dt_surf_tile_config = 0;
            decode->dt_luma_top_offset = luma->surface.u.gfx9.surf_offset;
   decode->dt_chroma_top_offset = chroma->surface.u.gfx9.surf_offset;
   if (decode->dt_field_mode) {
      decode->dt_luma_bottom_offset =
         decode->dt_chroma_bottom_offset =
      } else {
      decode->dt_luma_bottom_offset = decode->dt_luma_top_offset;
      }
   if (dec->stream_type == RDECODE_CODEC_AV1)
            if (encrypted) {
      assert(sscreen->info.has_tmz_support);
               if (dec->dpb_type == DPB_DYNAMIC_TIER_1) {
      decode->decode_flags |= (RDECODE_FLAGS_USE_DYNAMIC_DPB_MASK | RDECODE_FLAGS_USE_PAL_MASK);
   // Add decode flag for RESIZE_DPB ,when we do resize
   if (dpb_resize == true)
            dynamic_dpb->dpbArraySize = NUM_VP9_REFS + 1;
   dynamic_dpb->dpbLumaPitch = align(dec->max_width, dec->db_alignment);
   dynamic_dpb->dpbLumaAlignedHeight = align(dec->max_height, dec->db_alignment);
   dynamic_dpb->dpbLumaAlignedSize =
         dynamic_dpb->dpbChromaPitch = dynamic_dpb->dpbLumaPitch >> 1;
   dynamic_dpb->dpbChromaAlignedHeight = dynamic_dpb->dpbLumaAlignedHeight >> 1;
   dynamic_dpb->dpbChromaAlignedSize =
                  if (dec->base.profile == PIPE_VIDEO_PROFILE_VP9_PROFILE2) {
      dynamic_dpb->dpbLumaAlignedSize = dynamic_dpb->dpbLumaAlignedSize * 3 / 2;
                  switch (u_reduce_video_profile(picture->profile)) {
   case PIPE_VIDEO_FORMAT_MPEG4_AVC: {
      rvcn_dec_message_avc_t avc = get_h264_msg(dec, target, (struct pipe_h264_picture_desc *)picture);
   memcpy(codec, (void *)&avc, sizeof(rvcn_dec_message_avc_t));
   index_codec->message_id = RDECODE_MESSAGE_AVC;
      }
   case PIPE_VIDEO_FORMAT_HEVC: {
      rvcn_dec_message_hevc_t hevc =
            memcpy(codec, (void *)&hevc, sizeof(rvcn_dec_message_hevc_t));
   index_codec->message_id = RDECODE_MESSAGE_HEVC;
      }
   case PIPE_VIDEO_FORMAT_VC1: {
               memcpy(codec, (void *)&vc1, sizeof(rvcn_dec_message_vc1_t));
   if ((picture->profile == PIPE_VIDEO_PROFILE_VC1_SIMPLE) ||
      (picture->profile == PIPE_VIDEO_PROFILE_VC1_MAIN)) {
   decode->width_in_samples = align(decode->width_in_samples, 16) / 16;
      }
   index_codec->message_id = RDECODE_MESSAGE_VC1;
      }
   case PIPE_VIDEO_FORMAT_MPEG12: {
      rvcn_dec_message_mpeg2_vld_t mpeg2 =
            memcpy(codec, (void *)&mpeg2, sizeof(rvcn_dec_message_mpeg2_vld_t));
   index_codec->message_id = RDECODE_MESSAGE_MPEG2_VLD;
      }
   case PIPE_VIDEO_FORMAT_MPEG4: {
      rvcn_dec_message_mpeg4_asp_vld_t mpeg4 =
            memcpy(codec, (void *)&mpeg4, sizeof(rvcn_dec_message_mpeg4_asp_vld_t));
   index_codec->message_id = RDECODE_MESSAGE_MPEG4_ASP_VLD;
      }
   case PIPE_VIDEO_FORMAT_VP9: {
      rvcn_dec_message_vp9_t vp9 =
            memcpy(codec, (void *)&vp9, sizeof(rvcn_dec_message_vp9_t));
   index_codec->message_id = RDECODE_MESSAGE_VP9;
      }
   case PIPE_VIDEO_FORMAT_AV1: {
      rvcn_dec_message_av1_t av1 =
            memcpy(codec, (void *)&av1, sizeof(rvcn_dec_message_av1_t));
            if (dec->ctx.res == NULL) {
      unsigned frame_ctxt_size = dec->av1_version == RDECODE_AV1_VER_0
                  unsigned ctx_size = (9 + 4) * frame_ctxt_size + 9 * 64 * 34 * 512 + 9 * 64 * 34 * 256 * 5;
   int num_64x64_CTB_8k = 68;
   int num_128x128_CTB_8k = 34;
   int sdb_pitch_64x64 = align(32 * num_64x64_CTB_8k, 256) * 2;
   int sdb_pitch_128x128 = align(32 * num_128x128_CTB_8k, 256) * 2;
   int sdb_lf_size_ctb_64x64 = sdb_pitch_64x64 * (align(1728, 64) / 64);
   int sdb_lf_size_ctb_128x128 = sdb_pitch_128x128 * (align(3008, 64) / 64);
   int sdb_superres_size_ctb_64x64 = sdb_pitch_64x64 * (align(3232, 64) / 64);
   int sdb_superres_size_ctb_128x128 = sdb_pitch_128x128 * (align(6208, 64) / 64);
   int sdb_output_size_ctb_64x64 = sdb_pitch_64x64 * (align(1312, 64) / 64);
   int sdb_output_size_ctb_128x128 = sdb_pitch_128x128 * (align(2336, 64) / 64);
   int sdb_fg_avg_luma_size_ctb_64x64 = sdb_pitch_64x64 * (align(384, 64) / 64);
   int sdb_fg_avg_luma_size_ctb_128x128 = sdb_pitch_128x128 * (align(640, 64) / 64);
   uint8_t *ptr;
                  if (dec->av1_version == RDECODE_AV1_VER_0) {
      prob.init_mode_probs = rvcn_init_mode_probs;
   prob.init_mv_probs = rvcn_av1_init_mv_probs;
      } else {
      prob.init_mode_probs = rvcn_vcn4_init_mode_probs;
   prob.init_mv_probs = rvcn_vcn4_av1_init_mv_probs;
               ctx_size += (MAX2(sdb_lf_size_ctb_64x64, sdb_lf_size_ctb_128x128) +
                        if (!si_vid_create_buffer(dec->screen, &dec->ctx, ctx_size, PIPE_USAGE_DEFAULT))
                           for (i = 0; i < 4; ++i) {
      prob.init_mode_probs((void*)(ptr + i * frame_ctxt_size));
   prob.init_mv_probs((void*)(ptr + i * frame_ctxt_size));
      }
                  }
   default:
      assert(0);
               if (dec->ctx.res)
            if (dec->dpb_type == DPB_DYNAMIC_TIER_2)
      if (rvcn_dec_dynamic_dpb_t2_message(dec, decode, dynamic_dpb_t2, encrypted))
            }
      static void rvcn_dec_message_destroy(struct radeon_decoder *dec)
   {
               memset(dec->msg, 0, sizeof(rvcn_dec_message_header_t));
   header->header_size = sizeof(rvcn_dec_message_header_t);
   header->total_size = sizeof(rvcn_dec_message_header_t) - sizeof(rvcn_dec_message_index_t);
   header->num_buffers = 0;
   header->msg_type = RDECODE_MSG_DESTROY;
   header->stream_handle = dec->stream_handle;
      }
      static void rvcn_dec_message_feedback(struct radeon_decoder *dec)
   {
               header->header_size = sizeof(rvcn_dec_feedback_header_t);
   header->total_size = sizeof(rvcn_dec_feedback_header_t);
      }
      static void rvcn_dec_sq_tail(struct radeon_decoder *dec)
   {
      if (dec->vcn_dec_sw_ring == false)
               }
   /* flush IB to the hardware */
   static int flush(struct radeon_decoder *dec, unsigned flags,
                        }
      /* add a new set register command to the IB */
   static void set_reg(struct radeon_decoder *dec, unsigned reg, uint32_t val)
   {
      radeon_emit(&dec->cs, RDECODE_PKT0(reg >> 2, 0));
      }
      /* send a command to the VCPU through the GPCOM registers */
   static void send_cmd(struct radeon_decoder *dec, unsigned cmd, struct pb_buffer *buf, uint32_t off,
         {
               dec->ws->cs_add_buffer(&dec->cs, buf, usage | RADEON_USAGE_SYNCHRONIZED, domain);
   addr = dec->ws->buffer_get_virtual_address(buf);
            if (dec->vcn_dec_sw_ring == false) {
      set_reg(dec, dec->reg.data0, addr);
   set_reg(dec, dec->reg.data1, addr >> 32);
   set_reg(dec, dec->reg.cmd, cmd << 1);
               if (!dec->cs.current.cdw) {
      rvcn_sq_header(&dec->cs, &dec->sq, false);
   rvcn_decode_ib_package_t *ib_header =
            ib_header->package_size = sizeof(struct rvcn_decode_buffer_s) +
         dec->cs.current.cdw++;
   ib_header->package_type = (RDECODE_IB_PARAM_DECODE_BUFFER);
            dec->decode_buffer =
            dec->cs.current.cdw += sizeof(struct rvcn_decode_buffer_s) / 4;
               switch(cmd) {
      case RDECODE_CMD_MSG_BUFFER:
         dec->decode_buffer->valid_buf_flag |= RDECODE_CMDBUF_FLAGS_MSG_BUFFER;
   dec->decode_buffer->msg_buffer_address_hi = (addr >> 32);
         case RDECODE_CMD_DPB_BUFFER:
         dec->decode_buffer->valid_buf_flag |= (RDECODE_CMDBUF_FLAGS_DPB_BUFFER);
   dec->decode_buffer->dpb_buffer_address_hi = (addr >> 32);
         case RDECODE_CMD_DECODING_TARGET_BUFFER:
         dec->decode_buffer->valid_buf_flag |= (RDECODE_CMDBUF_FLAGS_DECODING_TARGET_BUFFER);
   dec->decode_buffer->target_buffer_address_hi = (addr >> 32);
         case RDECODE_CMD_FEEDBACK_BUFFER:
         dec->decode_buffer->valid_buf_flag |= (RDECODE_CMDBUF_FLAGS_FEEDBACK_BUFFER);
   dec->decode_buffer->feedback_buffer_address_hi = (addr >> 32);
         case RDECODE_CMD_PROB_TBL_BUFFER:
         dec->decode_buffer->valid_buf_flag |= (RDECODE_CMDBUF_FLAGS_PROB_TBL_BUFFER);
   dec->decode_buffer->prob_tbl_buffer_address_hi = (addr >> 32);
         case RDECODE_CMD_SESSION_CONTEXT_BUFFER:
         dec->decode_buffer->valid_buf_flag |= (RDECODE_CMDBUF_FLAGS_SESSION_CONTEXT_BUFFER);
   dec->decode_buffer->session_contex_buffer_address_hi = (addr >> 32);
         case RDECODE_CMD_BITSTREAM_BUFFER:
         dec->decode_buffer->valid_buf_flag |= (RDECODE_CMDBUF_FLAGS_BITSTREAM_BUFFER);
   dec->decode_buffer->bitstream_buffer_address_hi = (addr >> 32);
         case RDECODE_CMD_IT_SCALING_TABLE_BUFFER:
         dec->decode_buffer->valid_buf_flag |= (RDECODE_CMDBUF_FLAGS_IT_SCALING_BUFFER);
   dec->decode_buffer->it_sclr_table_buffer_address_hi = (addr >> 32);
         case RDECODE_CMD_CONTEXT_BUFFER:
         dec->decode_buffer->valid_buf_flag |= (RDECODE_CMDBUF_FLAGS_CONTEXT_BUFFER);
   dec->decode_buffer->context_buffer_address_hi = (addr >> 32);
         default:
         }
      /* do the codec needs an IT buffer ?*/
   static bool have_it(struct radeon_decoder *dec)
   {
         }
      /* do the codec needs an probs buffer? */
   static bool have_probs(struct radeon_decoder *dec)
   {
         }
      /* map the next available message/feedback/itscaling buffer */
   static void map_msg_fb_it_probs_buf(struct radeon_decoder *dec)
   {
      struct rvid_buffer *buf;
            /* grab the current message/feedback buffer */
            /* and map it for CPU access */
   ptr =
            /* calc buffer offsets */
            dec->fb = (uint32_t *)(ptr + FB_BUFFER_OFFSET);
   if (have_it(dec))
         else if (have_probs(dec))
      }
      /* unmap and send a message command to the VCPU */
   static void send_msg_buf(struct radeon_decoder *dec)
   {
               /* ignore the request if message/feedback buffer isn't mapped */
   if (!dec->msg || !dec->fb)
            /* grab the current message buffer */
            /* unmap the buffer */
   dec->ws->buffer_unmap(dec->ws, buf->res->buf);
   dec->bs_ptr = NULL;
   dec->msg = NULL;
   dec->fb = NULL;
   dec->it = NULL;
            if (dec->sessionctx.res)
      send_cmd(dec, RDECODE_CMD_SESSION_CONTEXT_BUFFER, dec->sessionctx.res->buf, 0,
         /* and send it to the hardware */
      }
      /* cycle to the next set of buffers */
   static void next_buffer(struct radeon_decoder *dec)
   {
      ++dec->cur_buffer;
      }
      static unsigned calc_ctx_size_h264_perf(struct radeon_decoder *dec)
   {
      unsigned width_in_mb, height_in_mb, ctx_size;
   unsigned width = align(dec->base.width, VL_MACROBLOCK_WIDTH);
                     // picture width & height in 16 pixel units
   width_in_mb = width / VL_MACROBLOCK_WIDTH;
            unsigned fs_in_mb = width_in_mb * height_in_mb;
   unsigned num_dpb_buffer;
   switch (dec->base.level) {
   case 30:
      num_dpb_buffer = 8100 / fs_in_mb;
      case 31:
      num_dpb_buffer = 18000 / fs_in_mb;
      case 32:
      num_dpb_buffer = 20480 / fs_in_mb;
      case 41:
      num_dpb_buffer = 32768 / fs_in_mb;
      case 42:
      num_dpb_buffer = 34816 / fs_in_mb;
      case 50:
      num_dpb_buffer = 110400 / fs_in_mb;
      case 51:
      num_dpb_buffer = 184320 / fs_in_mb;
      default:
      num_dpb_buffer = 184320 / fs_in_mb;
      }
   num_dpb_buffer++;
   max_references = MAX2(MIN2(NUM_H264_REFS, num_dpb_buffer), max_references);
               }
      /* calculate size of reference picture buffer */
   static unsigned calc_dpb_size(struct radeon_decoder *dec)
   {
               // always align them to MB size for dpb calculation
   unsigned width = align(dec->base.width, VL_MACROBLOCK_WIDTH);
            // always one more for currently decoded picture
            // aligned size of a single frame
   image_size = align(width, 32) * height;
   image_size += image_size / 2;
            // picture width & height in 16 pixel units
   width_in_mb = width / VL_MACROBLOCK_WIDTH;
            switch (u_reduce_video_profile(dec->base.profile)) {
   case PIPE_VIDEO_FORMAT_MPEG4_AVC: {
      unsigned fs_in_mb = width_in_mb * height_in_mb;
            switch (dec->base.level) {
   case 30:
      num_dpb_buffer = 8100 / fs_in_mb;
      case 31:
      num_dpb_buffer = 18000 / fs_in_mb;
      case 32:
      num_dpb_buffer = 20480 / fs_in_mb;
      case 41:
      num_dpb_buffer = 32768 / fs_in_mb;
      case 42:
      num_dpb_buffer = 34816 / fs_in_mb;
      case 50:
      num_dpb_buffer = 110400 / fs_in_mb;
      case 51:
      num_dpb_buffer = 184320 / fs_in_mb;
      default:
      num_dpb_buffer = 184320 / fs_in_mb;
      }
   num_dpb_buffer++;
   max_references = MAX2(MIN2(NUM_H264_REFS, num_dpb_buffer), max_references);
   dpb_size = image_size * max_references;
               case PIPE_VIDEO_FORMAT_HEVC:
      if (dec->base.width * dec->base.height >= 4096 * 2000)
         else
            width = align(width, 16);
   height = align(height, 16);
   if (dec->base.profile == PIPE_VIDEO_PROFILE_HEVC_MAIN_10)
         else
               case PIPE_VIDEO_FORMAT_VC1:
      // the firmware seems to always assume a minimum of ref frames
            // reference picture buffer
            // CONTEXT_BUFFER
            // IT surface buffer
            // DB surface buffer
            // BP
   dpb_size += align(MAX2(width_in_mb, height_in_mb) * 7 * 16, 64);
         case PIPE_VIDEO_FORMAT_MPEG12:
      // reference picture buffer, must be big enough for all frames
   dpb_size = image_size * NUM_MPEG2_REFS;
         case PIPE_VIDEO_FORMAT_MPEG4:
      // reference picture buffer
            // CM
            // IT surface buffer
            dpb_size = MAX2(dpb_size, 30 * 1024 * 1024);
         case PIPE_VIDEO_FORMAT_VP9:
               if (dec->dpb_type == DPB_MAX_RES)
      dpb_size = (((struct si_screen *)dec->screen)->info.vcn_ip_version >= VCN_2_0_0)
      ? (8192 * 4320 * 3 / 2) * max_references
   else
                  if (dec->base.profile == PIPE_VIDEO_PROFILE_VP9_PROFILE2)
               case PIPE_VIDEO_FORMAT_AV1:
      max_references = MAX2(max_references, 9);
   dpb_size = 8192 * 4320 * 3 / 2 * max_references * 3 / 2;
         case PIPE_VIDEO_FORMAT_JPEG:
      dpb_size = 0;
         default:
      // something is missing here
            // at least use a sane default value
   dpb_size = 32 * 1024 * 1024;
      }
      }
      /**
   * destroy this video decoder
   */
   static void radeon_dec_destroy(struct pipe_video_codec *decoder)
   {
      struct radeon_decoder *dec = (struct radeon_decoder *)decoder;
                     if (dec->stream_type != RDECODE_CODEC_JPEG) {
      map_msg_fb_it_probs_buf(dec);
   rvcn_dec_message_destroy(dec);
   send_msg_buf(dec);
   flush(dec, 0, &dec->destroy_fence);
   dec->ws->fence_wait(dec->ws, dec->destroy_fence, PIPE_DEFAULT_DECODER_FEEDBACK_TIMEOUT_NS);
               dec->ws->fence_reference(&dec->prev_fence, NULL);
            if (dec->stream_type == RDECODE_CODEC_JPEG) {
      for (i = 0; i < dec->njctx; i++) {
      dec->ws->cs_destroy(&dec->jcs[i]);
                  if (dec->msg_fb_it_probs_buffers && dec->bs_buffers) {
      for (i = 0; i < dec->num_dec_bufs; ++i) {
         si_vid_destroy_buffer(&dec->msg_fb_it_probs_buffers[i]);
   }
   FREE(dec->msg_fb_it_probs_buffers);
      }
            if (dec->dpb_type != DPB_DYNAMIC_TIER_2) {
         } else {
      list_for_each_entry_safe(struct rvcn_dec_dynamic_dpb_t2, d, &dec->dpb_ref_list, list) {
      list_del(&d->list);
   si_vid_destroy_buffer(&d->dpb);
         }
   si_vid_destroy_buffer(&dec->ctx);
            FREE(dec->jcs);
   FREE(dec->jctx);
      }
      /**
   * start decoding of a new frame
   */
   static void radeon_dec_begin_frame(struct pipe_video_codec *decoder,
               {
      struct radeon_decoder *dec = (struct radeon_decoder *)decoder;
                     frame = ++dec->frame_number;
   if (dec->stream_type != RDECODE_CODEC_VP9 && dec->stream_type != RDECODE_CODEC_AV1
            vl_video_buffer_set_associated_data(target, decoder, (void *)frame,
         dec->bs_size = 0;
   dec->bs_ptr = dec->ws->buffer_map(dec->ws, dec->bs_buffers[dec->cur_buffer].res->buf, &dec->cs,
      }
      /**
   * decode a macroblock
   */
   static void radeon_dec_decode_macroblock(struct pipe_video_codec *decoder,
                           {
      /* not supported (yet) */
      }
      /**
   * decode a bitstream
   */
   static void radeon_dec_decode_bitstream(struct pipe_video_codec *decoder,
                     {
      struct radeon_decoder *dec = (struct radeon_decoder *)decoder;
                     if (!dec->bs_ptr)
            if (dec->bs_size && dec->stream_type == RDECODE_CODEC_AV1)
            unsigned long total_bs_size = dec->bs_size;
   for (i = 0; i < num_buffers; ++i)
                     if (total_bs_size > buf->res->buf->size) {
      dec->ws->buffer_unmap(dec->ws, buf->res->buf);
   dec->bs_ptr = NULL;
   if (!si_vid_resize_buffer(dec->screen, &dec->cs, buf, total_bs_size, NULL)) {
      RVID_ERR("Can't resize bitstream buffer!");
               dec->bs_ptr = dec->ws->buffer_map(dec->ws, buf->res->buf, &dec->cs,
         if (!dec->bs_ptr)
                        for (i = 0; i < num_buffers; ++i) {
      memcpy(dec->bs_ptr, buffers[i], sizes[i]);
   dec->bs_size += sizes[i];
         }
      /**
   * send cmd for vcn dec
   */
   void send_cmd_dec(struct radeon_decoder *dec, struct pipe_video_buffer *target,
         {
      struct pb_buffer *dt;
            msg_fb_it_probs_buf = &dec->msg_fb_it_probs_buffers[dec->cur_buffer];
            memset(dec->bs_ptr, 0, align(dec->bs_size, 128) - dec->bs_size);
   dec->ws->buffer_unmap(dec->ws, bs_buf->res->buf);
            map_msg_fb_it_probs_buf(dec);
   dt = rvcn_dec_message_decode(dec, target, picture);
   rvcn_dec_message_feedback(dec);
            if (dec->dpb_type != DPB_DYNAMIC_TIER_2)
      send_cmd(dec, RDECODE_CMD_DPB_BUFFER, dec->dpb.res->buf, 0, RADEON_USAGE_READWRITE,
      if (dec->ctx.res)
      send_cmd(dec, RDECODE_CMD_CONTEXT_BUFFER, dec->ctx.res->buf, 0, RADEON_USAGE_READWRITE,
      send_cmd(dec, RDECODE_CMD_BITSTREAM_BUFFER, bs_buf->res->buf, 0, RADEON_USAGE_READ,
         send_cmd(dec, RDECODE_CMD_DECODING_TARGET_BUFFER, dt, 0, RADEON_USAGE_WRITE, RADEON_DOMAIN_VRAM);
   send_cmd(dec, RDECODE_CMD_FEEDBACK_BUFFER, msg_fb_it_probs_buf->res->buf, FB_BUFFER_OFFSET,
         if (have_it(dec))
      send_cmd(dec, RDECODE_CMD_IT_SCALING_TABLE_BUFFER, msg_fb_it_probs_buf->res->buf,
      else if (have_probs(dec))
      send_cmd(dec, RDECODE_CMD_PROB_TBL_BUFFER, msg_fb_it_probs_buf->res->buf,
         if (dec->vcn_dec_sw_ring == false)
      }
      /**
   * end decoding of the current frame
   */
   static void radeon_dec_end_frame(struct pipe_video_codec *decoder, struct pipe_video_buffer *target,
         {
                        if (!dec->bs_ptr)
            dec->send_cmd(dec, target, picture);
   flush(dec, PIPE_FLUSH_ASYNC, picture->fence);
   if (picture->fence)
            }
      /**
   * end decoding of the current jpeg frame
   */
   static void radeon_dec_jpeg_end_frame(struct pipe_video_codec *decoder, struct pipe_video_buffer *target,
         {
      struct radeon_decoder *dec = (struct radeon_decoder *)decoder;
                     if (!dec->bs_ptr)
            dec->jpg.crop_x = ROUND_DOWN_TO(pic->picture_parameter.crop_x, VL_MACROBLOCK_WIDTH);
   dec->jpg.crop_y = ROUND_DOWN_TO(pic->picture_parameter.crop_y, VL_MACROBLOCK_HEIGHT);
   dec->jpg.crop_width = align(pic->picture_parameter.crop_width, VL_MACROBLOCK_WIDTH);
   dec->jpg.crop_height = align(pic->picture_parameter.crop_height, VL_MACROBLOCK_HEIGHT);
   if (dec->jpg.crop_x + dec->jpg.crop_width > pic->picture_parameter.picture_width)
         if (dec->jpg.crop_y + dec->jpg.crop_height > pic->picture_parameter.picture_height)
         dec->send_cmd(dec, target, picture);
   dec->ws->cs_flush(&dec->jcs[dec->cb_idx], PIPE_FLUSH_ASYNC, NULL);
   next_buffer(dec);
      }
      /**
   * flush any outstanding command buffers to the hardware
   */
   static void radeon_dec_flush(struct pipe_video_codec *decoder)
   {
   }
      static int radeon_dec_get_decoder_fence(struct pipe_video_codec *decoder,
                                 }
      static void radeon_dec_destroy_fence(struct pipe_video_codec *decoder,
         {
                  }
      /**
   * update render list when target buffer got updated, use the existing
   * index and update the new buffer to associate with it.
   */
   static void radeon_dec_update_render_list(struct pipe_video_codec *decoder,
               {
      struct radeon_decoder *dec = (struct radeon_decoder *)decoder;
            vl_video_buffer_set_associated_data(updated, decoder, index,
         for (int i = 0; i < ARRAY_SIZE(dec->render_pic_list); ++i) {
      if (dec->render_pic_list[i] == old) {
      dec->render_pic_list[i] = updated;
            }
   /**
   * create and HW decoder
   */
   struct pipe_video_codec *radeon_create_decoder(struct pipe_context *context,
         {
      struct si_context *sctx = (struct si_context *)context;
   struct radeon_winsys *ws = sctx->ws;
   unsigned width = templ->width, height = templ->height;
   unsigned bs_buf_size, stream_type = 0, ring = AMD_IP_VCN_DEC;
   struct radeon_decoder *dec;
            switch (u_reduce_video_profile(templ->profile)) {
   case PIPE_VIDEO_FORMAT_MPEG12:
      if (templ->entrypoint > PIPE_VIDEO_ENTRYPOINT_BITSTREAM)
         stream_type = RDECODE_CODEC_MPEG2_VLD;
      case PIPE_VIDEO_FORMAT_MPEG4:
      width = align(width, VL_MACROBLOCK_WIDTH);
   height = align(height, VL_MACROBLOCK_HEIGHT);
   stream_type = RDECODE_CODEC_MPEG4;
      case PIPE_VIDEO_FORMAT_VC1:
      stream_type = RDECODE_CODEC_VC1;
      case PIPE_VIDEO_FORMAT_MPEG4_AVC:
      width = align(width, VL_MACROBLOCK_WIDTH);
   height = align(height, VL_MACROBLOCK_HEIGHT);
   stream_type = RDECODE_CODEC_H264_PERF;
      case PIPE_VIDEO_FORMAT_HEVC:
      stream_type = RDECODE_CODEC_H265;
      case PIPE_VIDEO_FORMAT_VP9:
      stream_type = RDECODE_CODEC_VP9;
      case PIPE_VIDEO_FORMAT_AV1:
      stream_type = RDECODE_CODEC_AV1;
      case PIPE_VIDEO_FORMAT_JPEG:
      stream_type = RDECODE_CODEC_JPEG;
   ring = AMD_IP_VCN_JPEG;
      default:
      assert(0);
                        if (!dec)
            dec->base = *templ;
   dec->base.context = context;
   dec->base.width = width;
   dec->base.height = height;
   dec->max_width = width;
   dec->max_height = height;
   dec->base.destroy = radeon_dec_destroy;
   dec->base.begin_frame = radeon_dec_begin_frame;
   dec->base.decode_macroblock = radeon_dec_decode_macroblock;
   dec->base.decode_bitstream = radeon_dec_decode_bitstream;
   dec->base.end_frame = radeon_dec_end_frame;
   dec->base.flush = radeon_dec_flush;
   dec->base.get_decoder_fence = radeon_dec_get_decoder_fence;
   dec->base.destroy_fence = radeon_dec_destroy_fence;
            dec->stream_type = stream_type;
   dec->stream_handle = si_vid_alloc_stream_handle();
   dec->screen = context->screen;
            if (u_reduce_video_profile(templ->profile) != PIPE_VIDEO_FORMAT_JPEG &&
      (sctx->vcn_ip_ver >= VCN_4_0_0)) {
   dec->vcn_dec_sw_ring = true;
               dec->sq.ib_total_size_in_dw = NULL;
            if (!ws->cs_create(&dec->cs, sctx->ctx, ring, NULL, NULL)) {
      RVID_ERR("Can't get command submission context.\n");
                           if (sctx->vcn_ip_ver == VCN_2_5_0 || sctx->vcn_ip_ver == VCN_2_6_0)
         else if (sctx->vcn_ip_ver == VCN_4_0_3)
         else
            dec->jctx = (struct radeon_winsys_ctx **) CALLOC(dec->njctx,
         dec->jcs = (struct radeon_cmdbuf *) CALLOC(dec->njctx, sizeof(struct radeon_cmdbuf));
   if(!dec->jctx || !dec->jcs)
         for (i = 0; i < dec->njctx; i++) {
   /* Initialize the context handle and the command stream. */
      dec->jctx[i] = dec->ws->ctx_create(dec->ws, RADEON_CTX_PRIORITY_MEDIUM,
         if (!sctx->ctx)
         if (!dec->ws->cs_create(&dec->jcs[i], dec->jctx[i], ring, NULL, NULL)) {
      RVID_ERR("Can't get additional command submission context for mJPEG.\n");
         }
   dec->base.end_frame = radeon_dec_jpeg_end_frame;
               for (i = 0; i < ARRAY_SIZE(dec->render_pic_list); i++)
            if ((sctx->vcn_ip_ver >= VCN_3_0_0) && (stream_type == RDECODE_CODEC_H264_PERF)) {
      for (i = 0; i < ARRAY_SIZE(dec->h264_valid_ref_num); i++)
         for (i = 0; i < ARRAY_SIZE(dec->h264_valid_poc_num); i++)
               if (dec->stream_type == RDECODE_CODEC_JPEG) {
      if (sctx->vcn_ip_ver == VCN_4_0_3)
         else
      } else
            bs_buf_size = align(width * height / 32, 128);
   dec->msg_fb_it_probs_buffers = (struct rvid_buffer *) CALLOC(dec->num_dec_bufs, sizeof(struct rvid_buffer));
   dec->bs_buffers = (struct rvid_buffer *) CALLOC(dec->num_dec_bufs, sizeof(struct rvid_buffer));
   if(!dec->msg_fb_it_probs_buffers || !dec->bs_buffers)
            for (i = 0; i < dec->num_dec_bufs; ++i) {
      unsigned msg_fb_it_probs_size = FB_BUFFER_OFFSET + FB_BUFFER_SIZE;
   if (have_it(dec))
         else if (have_probs(dec))
      msg_fb_it_probs_size += (dec->stream_type == RDECODE_CODEC_VP9) ?
            /* use vram to improve performance, workaround an unknown bug */
   if (!si_vid_create_buffer(dec->screen, &dec->msg_fb_it_probs_buffers[i], msg_fb_it_probs_size,
            RVID_ERR("Can't allocated message buffers.\n");
               if (!si_vid_create_buffer(dec->screen, &dec->bs_buffers[i], bs_buf_size,
            RVID_ERR("Can't allocated bitstream buffers.\n");
               si_vid_clear_buffer(context, &dec->msg_fb_it_probs_buffers[i]);
            if (have_probs(dec) && dec->stream_type == RDECODE_CODEC_VP9) {
                     buf = &dec->msg_fb_it_probs_buffers[i];
   ptr = dec->ws->buffer_map(dec->ws, buf->res->buf, &dec->cs,
         ptr += FB_BUFFER_OFFSET + FB_BUFFER_SIZE;
   fill_probs_table(ptr);
   dec->ws->buffer_unmap(dec->ws, buf->res->buf);
                  if ((sctx->vcn_ip_ver >= VCN_3_0_0) &&
         (stream_type == RDECODE_CODEC_VP9 ||
      ((stream_type == RDECODE_CODEC_H265) && templ->expect_chunked_decode) ||
   ((stream_type == RDECODE_CODEC_H264_PERF) && templ->expect_chunked_decode)))
      else if (sctx->vcn_ip_ver <= VCN_2_6_0 && stream_type == RDECODE_CODEC_VP9)
         else
            dec->db_alignment = (((struct si_screen *)dec->screen)->info.vcn_ip_version >= VCN_2_0_0 &&
                                 if (!si_vid_create_buffer(dec->screen, &dec->sessionctx, RDECODE_SESSION_CONTEXT_SIZE,
            RVID_ERR("Can't allocated session ctx.\n");
      }
            dec->addr_gfx_mode = RDECODE_ARRAY_MODE_LINEAR;
            switch (sctx->vcn_ip_ver) {
   case VCN_1_0_0:
   case VCN_1_0_1:
      dec->reg.data0 = RDECODE_VCN1_GPCOM_VCPU_DATA0;
   dec->reg.data1 = RDECODE_VCN1_GPCOM_VCPU_DATA1;
   dec->reg.cmd = RDECODE_VCN1_GPCOM_VCPU_CMD;
   dec->reg.cntl = RDECODE_VCN1_ENGINE_CNTL;
   dec->jpg_reg.version = RDECODE_JPEG_REG_VER_V1;
      case VCN_2_0_0:
   case VCN_2_0_2:
   case VCN_2_0_3:
   case VCN_2_2_0:
      dec->reg.data0 = RDECODE_VCN2_GPCOM_VCPU_DATA0;
   dec->reg.data1 = RDECODE_VCN2_GPCOM_VCPU_DATA1;
   dec->reg.cmd = RDECODE_VCN2_GPCOM_VCPU_CMD;
   dec->reg.cntl = RDECODE_VCN2_ENGINE_CNTL;
   dec->jpg_reg.version = RDECODE_JPEG_REG_VER_V2;
      case VCN_2_5_0:
   case VCN_2_6_0:
   case VCN_3_0_0:
   case VCN_3_0_2:
   case VCN_3_0_16:
   case VCN_3_0_33:
   case VCN_3_1_1:
   case VCN_3_1_2:
      dec->reg.data0 = RDECODE_VCN2_5_GPCOM_VCPU_DATA0;
   dec->reg.data1 = RDECODE_VCN2_5_GPCOM_VCPU_DATA1;
   dec->reg.cmd = RDECODE_VCN2_5_GPCOM_VCPU_CMD;
   dec->reg.cntl = RDECODE_VCN2_5_ENGINE_CNTL;
   dec->jpg_reg.version = RDECODE_JPEG_REG_VER_V2;
      case VCN_4_0_3:
      dec->jpg_reg.version = RDECODE_JPEG_REG_VER_V3;
   dec->addr_gfx_mode = RDECODE_ARRAY_MODE_ADDRLIB_SEL_GFX9;
   dec->av1_version = RDECODE_AV1_VER_1;
      case VCN_4_0_0:
   case VCN_4_0_2:
   case VCN_4_0_4:
   case VCN_4_0_5:
      dec->jpg_reg.version = RDECODE_JPEG_REG_VER_V2;
   dec->addr_gfx_mode = RDECODE_ARRAY_MODE_ADDRLIB_SEL_GFX11;
   dec->av1_version = RDECODE_AV1_VER_1;
      default:
      RVID_ERR("VCN is not supported.\n");
               if (dec->stream_type != RDECODE_CODEC_JPEG) {
      map_msg_fb_it_probs_buf(dec);
   rvcn_dec_message_create(dec);
   send_msg_buf(dec);
   r = flush(dec, 0, NULL);
   if (r)
      } else if (dec->jpg_reg.version != RDECODE_JPEG_REG_VER_V1) {
      dec->jpg_reg.jrbc_ib_cond_rd_timer = vcnipUVD_JRBC_IB_COND_RD_TIMER;
   dec->jpg_reg.jrbc_ib_ref_data = vcnipUVD_JRBC_IB_REF_DATA;
   dec->jpg_reg.jpeg_rb_base = vcnipUVD_JPEG_RB_BASE;
   dec->jpg_reg.jpeg_rb_size = vcnipUVD_JPEG_RB_SIZE;
   dec->jpg_reg.jpeg_rb_wptr = vcnipUVD_JPEG_RB_WPTR;
   dec->jpg_reg.jpeg_int_en = vcnipUVD_JPEG_INT_EN;
   dec->jpg_reg.jpeg_cntl = vcnipUVD_JPEG_CNTL;
   dec->jpg_reg.jpeg_rb_rptr = vcnipUVD_JPEG_RB_RPTR;
   if (dec->jpg_reg.version == RDECODE_JPEG_REG_VER_V2) {
      dec->jpg_reg.jpeg_dec_soft_rst = vcnipUVD_JPEG_DEC_SOFT_RST;
   dec->jpg_reg.lmi_jpeg_read_64bit_bar_high = vcnipUVD_LMI_JPEG_READ_64BIT_BAR_HIGH;
   dec->jpg_reg.lmi_jpeg_read_64bit_bar_low = vcnipUVD_LMI_JPEG_READ_64BIT_BAR_LOW;
   dec->jpg_reg.jpeg_pitch = vcnipUVD_JPEG_PITCH;
   dec->jpg_reg.jpeg_uv_pitch = vcnipUVD_JPEG_UV_PITCH;
   dec->jpg_reg.dec_addr_mode = vcnipJPEG_DEC_ADDR_MODE;
   dec->jpg_reg.dec_y_gfx10_tiling_surface = vcnipJPEG_DEC_Y_GFX10_TILING_SURFACE;
   dec->jpg_reg.dec_uv_gfx10_tiling_surface = vcnipJPEG_DEC_UV_GFX10_TILING_SURFACE;
   dec->jpg_reg.lmi_jpeg_write_64bit_bar_high = vcnipUVD_LMI_JPEG_WRITE_64BIT_BAR_HIGH;
   dec->jpg_reg.lmi_jpeg_write_64bit_bar_low = vcnipUVD_LMI_JPEG_WRITE_64BIT_BAR_LOW;
   dec->jpg_reg.jpeg_tier_cntl2 = vcnipUVD_JPEG_TIER_CNTL2;
   dec->jpg_reg.jpeg_outbuf_cntl = vcnipUVD_JPEG_OUTBUF_CNTL;
   dec->jpg_reg.jpeg_outbuf_rptr = vcnipUVD_JPEG_OUTBUF_RPTR;
   dec->jpg_reg.jpeg_outbuf_wptr = vcnipUVD_JPEG_OUTBUF_WPTR;
   dec->jpg_reg.jpeg_index = vcnipUVD_JPEG_INDEX;
      } else {
      dec->jpg_reg.jpeg_dec_soft_rst = vcnipUVD_JPEG_DEC_SOFT_RST_1;
   dec->jpg_reg.lmi_jpeg_read_64bit_bar_high = vcnipUVD_LMI_JPEG_READ_64BIT_BAR_HIGH_1;
   dec->jpg_reg.lmi_jpeg_read_64bit_bar_low = vcnipUVD_LMI_JPEG_READ_64BIT_BAR_LOW_1;
   dec->jpg_reg.jpeg_pitch = vcnipUVD_JPEG_PITCH_1;
   dec->jpg_reg.jpeg_uv_pitch = vcnipUVD_JPEG_UV_PITCH_1;
   dec->jpg_reg.dec_addr_mode = vcnipJPEG_DEC_ADDR_MODE_1;
   dec->jpg_reg.dec_y_gfx10_tiling_surface = vcnipJPEG_DEC_Y_GFX10_TILING_SURFACE_1;
   dec->jpg_reg.dec_uv_gfx10_tiling_surface = vcnipJPEG_DEC_UV_GFX10_TILING_SURFACE_1;
   dec->jpg_reg.lmi_jpeg_write_64bit_bar_high = vcnipUVD_LMI_JPEG_WRITE_64BIT_BAR_HIGH_1;
   dec->jpg_reg.lmi_jpeg_write_64bit_bar_low = vcnipUVD_LMI_JPEG_WRITE_64BIT_BAR_LOW_1;
   dec->jpg_reg.jpeg_tier_cntl2 = vcnipUVD_JPEG_TIER_CNTL2_1;
   dec->jpg_reg.jpeg_outbuf_cntl = vcnipUVD_JPEG_OUTBUF_CNTL_1;
   dec->jpg_reg.jpeg_outbuf_rptr = vcnipUVD_JPEG_OUTBUF_RPTR_1;
   dec->jpg_reg.jpeg_outbuf_wptr = vcnipUVD_JPEG_OUTBUF_WPTR_1;
   dec->jpg_reg.jpeg_luma_base0_0 = vcnipUVD_JPEG_LUMA_BASE0_0;
   dec->jpg_reg.jpeg_chroma_base0_0 = vcnipUVD_JPEG_CHROMA_BASE0_0;
                           if (stream_type == RDECODE_CODEC_JPEG)
         else
               if (dec->dpb_type == DPB_DYNAMIC_TIER_2) {
      list_inithead(&dec->dpb_ref_list);
                              error:
               if (dec->stream_type == RDECODE_CODEC_JPEG) {
      for (i = 0; i < dec->njctx; i++) {
      dec->ws->cs_destroy(&dec->jcs[i]);
                  if (dec->msg_fb_it_probs_buffers && dec->bs_buffers) {
      for (i = 0; i < dec->num_dec_bufs; ++i) {
         si_vid_destroy_buffer(&dec->msg_fb_it_probs_buffers[i]);
   }
   FREE(dec->msg_fb_it_probs_buffers);
               if (dec->dpb_type != DPB_DYNAMIC_TIER_2)
         si_vid_destroy_buffer(&dec->ctx);
         err:
      if (dec->jcs)
         if (dec->jctx)
                     }
