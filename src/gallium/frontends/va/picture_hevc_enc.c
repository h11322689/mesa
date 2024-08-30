   /**************************************************************************
   *
   * Copyright 2018 Advanced Micro Devices, Inc.
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
      #include "util/u_handle_table.h"
   #include "util/u_video.h"
   #include "va_private.h"
      #include "util/vl_rbsp.h"
      enum HEVCNALUnitType {
      HEVC_NAL_VPS        = 32,
   HEVC_NAL_SPS        = 33,
      };
      VAStatus
   vlVaHandleVAEncPictureParameterBufferTypeHEVC(vlVaDriver *drv, vlVaContext *context, vlVaBuffer *buf)
   {
      VAEncPictureParameterBufferHEVC *h265;
   vlVaBuffer *coded_buf;
            h265 = buf->data;
   context->desc.h265enc.decoded_curr_pic = h265->decoded_curr_pic.picture_id;
            for (i = 0; i < 15; i++)
            context->desc.h265enc.pic_order_cnt = h265->decoded_curr_pic.pic_order_cnt;
   coded_buf = handle_table_get(drv->htab, h265->coded_buf);
   if (!coded_buf)
            if (!coded_buf->derived_surface.resource)
      coded_buf->derived_surface.resource = pipe_buffer_create(drv->pipe->screen, PIPE_BIND_VERTEX_BUFFER,
         context->coded_buf = coded_buf;
   context->desc.h265enc.pic.log2_parallel_merge_level_minus2 = h265->log2_parallel_merge_level_minus2;
   context->desc.h265enc.pic.nal_unit_type = h265->nal_unit_type;
   context->desc.h265enc.rc.quant_i_frames = h265->pic_init_qp;
   context->desc.h265enc.rc.quant_p_frames = h265->pic_init_qp;
            switch(h265->pic_fields.bits.coding_type) {
   case 1:
      if (h265->pic_fields.bits.idr_pic_flag)
         else
            case 2:
      context->desc.h265enc.picture_type = PIPE_H2645_ENC_PICTURE_TYPE_P;
      case 3:
   case 4:
   case 5:
      context->desc.h265enc.picture_type = PIPE_H2645_ENC_PICTURE_TYPE_B;
               context->desc.h265enc.pic.constrained_intra_pred_flag = h265->pic_fields.bits.constrained_intra_pred_flag;
   context->desc.h265enc.pic.pps_loop_filter_across_slices_enabled_flag = h265->pic_fields.bits.pps_loop_filter_across_slices_enabled_flag;
            _mesa_hash_table_insert(context->desc.h265enc.frame_idx,
                  /* Initialize slice descriptors for this picture */
   context->desc.h265enc.num_slice_descriptors = 0;
            context->desc.h265enc.num_ref_idx_l0_active_minus1 = h265->num_ref_idx_l0_default_active_minus1;
               }
      VAStatus
   vlVaHandleVAEncSliceParameterBufferTypeHEVC(vlVaDriver *drv, vlVaContext *context, vlVaBuffer *buf)
   {
               h265 = buf->data;
   memset(&context->desc.h265enc.ref_idx_l0_list, VA_INVALID_ID, sizeof(context->desc.h265enc.ref_idx_l0_list));
            if (h265->slice_fields.bits.num_ref_idx_active_override_flag) {
      context->desc.h265enc.num_ref_idx_l0_active_minus1 = h265->num_ref_idx_l0_active_minus1;
               for (int i = 0; i < 15; i++) {
      if (h265->ref_pic_list0[i].picture_id != VA_INVALID_ID) {
      context->desc.h265enc.ref_idx_l0_list[i] = PTR_TO_UINT(util_hash_table_get(context->desc.h265enc.frame_idx,
      }
   if (h265->ref_pic_list1[i].picture_id != VA_INVALID_ID && h265->slice_type == PIPE_H265_SLICE_TYPE_B) {
      context->desc.h265enc.ref_idx_l1_list[i] = PTR_TO_UINT(util_hash_table_get(context->desc.h265enc.frame_idx,
                  context->desc.h265enc.slice.max_num_merge_cand = h265->max_num_merge_cand;
   context->desc.h265enc.slice.slice_cb_qp_offset = h265->slice_cb_qp_offset;
   context->desc.h265enc.slice.slice_cr_qp_offset = h265->slice_cr_qp_offset;
   context->desc.h265enc.slice.slice_beta_offset_div2 = h265->slice_beta_offset_div2;
   context->desc.h265enc.slice.slice_tc_offset_div2 = h265->slice_tc_offset_div2;
   context->desc.h265enc.slice.cabac_init_flag = h265->slice_fields.bits.cabac_init_flag;
   context->desc.h265enc.slice.slice_deblocking_filter_disabled_flag = h265->slice_fields.bits.slice_deblocking_filter_disabled_flag;
            /* Handle the slice control parameters */
   struct h265_slice_descriptor slice_descriptor;
   memset(&slice_descriptor, 0, sizeof(slice_descriptor));
   slice_descriptor.slice_segment_address = h265->slice_segment_address;
   slice_descriptor.num_ctu_in_slice = h265->num_ctu_in_slice;
   slice_descriptor.slice_type = h265->slice_type;
            if (context->desc.h265enc.num_slice_descriptors < ARRAY_SIZE(context->desc.h265enc.slices_descriptors))
         else
               }
      VAStatus
   vlVaHandleVAEncSequenceParameterBufferTypeHEVC(vlVaDriver *drv, vlVaContext *context, vlVaBuffer *buf)
   {
      VAEncSequenceParameterBufferHEVC *h265 = (VAEncSequenceParameterBufferHEVC *)buf->data;
            if (!context->decoder) {
      context->templat.max_references = PIPE_H265_MAX_REFERENCES;
   context->templat.level = h265->general_level_idc;
            if (!context->decoder)
            getEncParamPresetH265(context);
   context->desc.h265enc.rc.vbv_buffer_size = 20000000;
   context->desc.h265enc.rc.vbv_buf_lv = 48;
   context->desc.h265enc.rc.fill_data_enable = 1;
   context->desc.h265enc.rc.enforce_hrd = 1;
   context->desc.h265enc.rc.max_qp = 51;
               context->desc.h265enc.seq.general_profile_idc = h265->general_profile_idc;
   context->desc.h265enc.seq.general_level_idc = h265->general_level_idc;
   context->desc.h265enc.seq.general_tier_flag = h265->general_tier_flag;
   context->desc.h265enc.seq.intra_period = h265->intra_period;
   context->desc.h265enc.seq.ip_period = h265->ip_period;
   context->desc.h265enc.seq.pic_width_in_luma_samples = h265->pic_width_in_luma_samples;
   context->desc.h265enc.seq.pic_height_in_luma_samples = h265->pic_height_in_luma_samples;
   context->desc.h265enc.seq.chroma_format_idc = h265->seq_fields.bits.chroma_format_idc;
   context->desc.h265enc.seq.bit_depth_luma_minus8 = h265->seq_fields.bits.bit_depth_luma_minus8;
   context->desc.h265enc.seq.bit_depth_chroma_minus8 = h265->seq_fields.bits.bit_depth_chroma_minus8;
   context->desc.h265enc.seq.strong_intra_smoothing_enabled_flag = h265->seq_fields.bits.strong_intra_smoothing_enabled_flag;
   context->desc.h265enc.seq.amp_enabled_flag = h265->seq_fields.bits.amp_enabled_flag;
   context->desc.h265enc.seq.sample_adaptive_offset_enabled_flag = h265->seq_fields.bits.sample_adaptive_offset_enabled_flag;
   context->desc.h265enc.seq.pcm_enabled_flag = h265->seq_fields.bits.pcm_enabled_flag;
   context->desc.h265enc.seq.sps_temporal_mvp_enabled_flag = h265->seq_fields.bits.sps_temporal_mvp_enabled_flag;
   context->desc.h265enc.seq.log2_min_luma_coding_block_size_minus3 = h265->log2_min_luma_coding_block_size_minus3;
   context->desc.h265enc.seq.log2_diff_max_min_luma_coding_block_size = h265->log2_diff_max_min_luma_coding_block_size;
   context->desc.h265enc.seq.log2_min_transform_block_size_minus2 = h265->log2_min_transform_block_size_minus2;
   context->desc.h265enc.seq.log2_diff_max_min_transform_block_size = h265->log2_diff_max_min_transform_block_size;
   context->desc.h265enc.seq.max_transform_hierarchy_depth_inter = h265->max_transform_hierarchy_depth_inter;
            context->desc.h265enc.seq.vui_parameters_present_flag = h265->vui_parameters_present_flag;
   if (h265->vui_parameters_present_flag) {
      context->desc.h265enc.seq.vui_flags.aspect_ratio_info_present_flag =
      h265->vui_fields.bits.aspect_ratio_info_present_flag;
      context->desc.h265enc.seq.sar_width = h265->sar_width;
            context->desc.h265enc.seq.vui_flags.timing_info_present_flag =
         num_units_in_tick = h265->vui_num_units_in_tick;
      } else
            if (!context->desc.h265enc.seq.vui_flags.timing_info_present_flag) {
      /* if not present, set default value */
   num_units_in_tick = PIPE_DEFAULT_FRAME_RATE_DEN;
               context->desc.h265enc.seq.num_units_in_tick = num_units_in_tick;
   context->desc.h265enc.seq.time_scale = time_scale;
   context->desc.h265enc.rc.frame_rate_num = time_scale;
               }
      VAStatus
   vlVaHandleVAEncMiscParameterTypeRateControlHEVC(vlVaContext *context, VAEncMiscParameterBuffer *misc)
   {
               if (context->desc.h265enc.rc.rate_ctrl_method ==
               else
         context->desc.h265enc.rc.peak_bitrate = rc->bits_per_second;
   if (context->desc.h265enc.rc.target_bitrate < 2000000)
         else
            context->desc.h265enc.rc.fill_data_enable = !(rc->rc_flags.bits.disable_bit_stuffing);
   /* context->desc.h265enc.rc.skip_frame_enable = !(rc->rc_flags.bits.disable_frame_skip); */
   context->desc.h265enc.rc.skip_frame_enable = 0;
   context->desc.h265enc.rc.max_qp = rc->max_qp;
   context->desc.h265enc.rc.min_qp = rc->min_qp;
   /* Distinguishes from the default params set for these values in other
                  if (context->desc.h265enc.rc.rate_ctrl_method ==
      PIPE_H2645_ENC_RATE_CONTROL_METHOD_QUALITY_VARIABLE)
   context->desc.h265enc.rc.vbr_quality_factor =
            }
      VAStatus
   vlVaHandleVAEncMiscParameterTypeFrameRateHEVC(vlVaContext *context, VAEncMiscParameterBuffer *misc)
   {
               if (fr->framerate & 0xffff0000) {
      context->desc.h265enc.rc.frame_rate_num = fr->framerate       & 0xffff;
      } else {
      context->desc.h265enc.rc.frame_rate_num = fr->framerate;
                  }
      VAStatus
   vlVaHandleVAEncMiscParameterTypeQualityLevelHEVC(vlVaContext *context, VAEncMiscParameterBuffer *misc)
   {
      VAEncMiscParameterBufferQualityLevel *ql = (VAEncMiscParameterBufferQualityLevel *)misc->data;
   vlVaHandleVAEncMiscParameterTypeQualityLevel(&context->desc.h265enc.quality_modes,
               }
      static void profile_tier(struct vl_rbsp *rbsp)
   {
      vl_rbsp_u(rbsp, 2); /* general_profile_space */
   vl_rbsp_u(rbsp, 1); /* general_tier_flag */
            /* general_profile_compatibility_flag */
   for(int i = 0; i < 32; ++i)
            vl_rbsp_u(rbsp, 1); /* general_progressive_source_flag */
   vl_rbsp_u(rbsp, 1); /* general_interlaced_source_flag */
   vl_rbsp_u(rbsp, 1); /* general_non_packed_constraint_flag */
            /* general_reserved_zero_44bits */
   vl_rbsp_u(rbsp, 16);
   vl_rbsp_u(rbsp, 16);
      }
      static unsigned profile_tier_level(struct vl_rbsp *rbsp,
         {
      bool sub_layer_profile_present_flag[6];
   bool sub_layer_level_present_flag[6];
   unsigned level_idc;
            profile_tier(rbsp);
            for (i = 0; i < max_sublayers_minus1; ++i) {
      sub_layer_profile_present_flag[i] = vl_rbsp_u(rbsp, 1);
               if (max_sublayers_minus1 > 0)
      for (i = max_sublayers_minus1; i < 8; ++i)
         for (i = 0; i < max_sublayers_minus1; ++i) {
      if (sub_layer_profile_present_flag[i])
            if (sub_layer_level_present_flag[i])
                  }
      static void parseEncSpsParamsH265(vlVaContext *context, struct vl_rbsp *rbsp)
   {
      int sps_max_sub_layers_minus1;
            vl_rbsp_u(rbsp, 4);     /* sps_video_parameter_set_id */
   sps_max_sub_layers_minus1 = vl_rbsp_u(rbsp, 3);
            /* level_idc */
            vl_rbsp_ue(rbsp);       /* id */
   context->desc.h265enc.seq.chroma_format_idc = vl_rbsp_ue(rbsp);
   if (context->desc.h265enc.seq.chroma_format_idc == 3)
            context->desc.h265enc.seq.pic_width_in_luma_samples = vl_rbsp_ue(rbsp);
            /* conformance_window_flag - used for cropping */
   context->desc.h265enc.seq.conformance_window_flag = vl_rbsp_u(rbsp, 1);
   if (context->desc.h265enc.seq.conformance_window_flag) {
      context->desc.h265enc.seq.conf_win_left_offset = vl_rbsp_ue(rbsp);
   context->desc.h265enc.seq.conf_win_right_offset = vl_rbsp_ue(rbsp);
   context->desc.h265enc.seq.conf_win_top_offset = vl_rbsp_ue(rbsp);
               vl_rbsp_ue(rbsp); /* bit_depth_luma_minus8 */
   vl_rbsp_ue(rbsp); /* bit_depth_chroma_minus8 */
            sps_sub_layer_ordering_info_present_flag = vl_rbsp_u(rbsp, 1);
   i = sps_sub_layer_ordering_info_present_flag ? 0 : sps_max_sub_layers_minus1;
   for (; i <= sps_max_sub_layers_minus1; ++i) {
      vl_rbsp_ue(rbsp); /* sps_max_dec_pic_buffering_minus1[i] */
   vl_rbsp_ue(rbsp); /* sps_max_num_reorder_pics[i] */
               vl_rbsp_ue(rbsp); /* log2_min_luma_coding_block_size_minus3 */
   vl_rbsp_ue(rbsp); /* log2_diff_max_min_luma_coding_block_size */
   vl_rbsp_ue(rbsp); /* log2_min_luma_transform_block_size_minus2 */
   vl_rbsp_ue(rbsp); /* log2_diff_max_min_luma_transform_block_size */
   vl_rbsp_ue(rbsp); /* max_transform_hierarchy_depth_inter */
            if (vl_rbsp_u(rbsp, 1)) /* scaling_list_enabled_flag */
            vl_rbsp_u(rbsp, 1); /* amp_enabled_flag */
   vl_rbsp_u(rbsp, 1); /* sample_adaptive_offset_enabled_flag */
   if (vl_rbsp_u(rbsp, 1)) { /* pcm_enabled_flag */
      vl_rbsp_u(rbsp, 4); /* pcm_sample_bit_depth_luma_minus1 */
   vl_rbsp_u(rbsp, 4); /* pcm_sample_bit_depth_chroma_minus1 */
   vl_rbsp_ue(rbsp); /* log2_min_pcm_luma_coding_block_size_minus3 */
   vl_rbsp_ue(rbsp); /* log2_diff_max_min_pcm_luma_coding_block_size */
               if (vl_rbsp_ue(rbsp)) /* num_short_term_ref_pic_sets */
            if (vl_rbsp_u(rbsp, 1)) /* long_term_ref_pics_present_flag */
            vl_rbsp_u(rbsp, 1); /* sps_temporal_mvp_enabled_flag */
            context->desc.h265enc.seq.vui_parameters_present_flag = vl_rbsp_u(rbsp, 1);
   if (context->desc.h265enc.seq.vui_parameters_present_flag) {
      context->desc.h265enc.seq.vui_flags.aspect_ratio_info_present_flag = vl_rbsp_u(rbsp, 1);
   if (context->desc.h265enc.seq.vui_flags.aspect_ratio_info_present_flag) {
      if (vl_rbsp_u(rbsp, 8) == 255) { /* aspect_ratio_idc == Extended_SAR */
      vl_rbsp_u(rbsp, 16); /* sar_width */
                  if (vl_rbsp_u(rbsp, 1)) /* overscan_info_present_flag */
            context->desc.h265enc.seq.vui_flags.video_signal_type_present_flag = vl_rbsp_u(rbsp, 1);
   if (context->desc.h265enc.seq.vui_flags.video_signal_type_present_flag) {
      context->desc.h265enc.seq.video_format = vl_rbsp_u(rbsp, 3);
   context->desc.h265enc.seq.video_full_range_flag = vl_rbsp_u(rbsp, 1);
   context->desc.h265enc.seq.vui_flags.colour_description_present_flag = vl_rbsp_u(rbsp, 1);
   if (context->desc.h265enc.seq.vui_flags.colour_description_present_flag) {
      context->desc.h265enc.seq.colour_primaries = vl_rbsp_u(rbsp, 8);
   context->desc.h265enc.seq.transfer_characteristics = vl_rbsp_u(rbsp, 8);
                  context->desc.h265enc.seq.vui_flags.chroma_loc_info_present_flag = vl_rbsp_u(rbsp, 1);
   if (context->desc.h265enc.seq.vui_flags.chroma_loc_info_present_flag) {
      context->desc.h265enc.seq.chroma_sample_loc_type_top_field = vl_rbsp_ue(rbsp);
            }
      VAStatus
   vlVaHandleVAEncPackedHeaderDataBufferTypeHEVC(vlVaContext *context, vlVaBuffer *buf)
   {
      struct vl_vlc vlc = {0};
            while (vl_vlc_bits_left(&vlc) > 0) {
      /* search the first 64 bytes for a startcode */
   for (int i = 0; i < 64 && vl_vlc_bits_left(&vlc) >= 24; ++i) {
      if (vl_vlc_peekbits(&vlc, 24) == 0x000001)
         vl_vlc_eatbits(&vlc, 8);
      }
            if (vl_vlc_valid_bits(&vlc) < 15)
            vl_vlc_eatbits(&vlc, 1);
   unsigned nal_unit_type = vl_vlc_get_uimsbf(&vlc, 6);
   vl_vlc_eatbits(&vlc, 6);
            struct vl_rbsp rbsp;
            switch(nal_unit_type) {
   case HEVC_NAL_SPS:
      parseEncSpsParamsH265(context, &rbsp);
      case HEVC_NAL_VPS:
   case HEVC_NAL_PPS:
   default:
                  if (!context->packed_header_emulation_bytes)
                  }
      VAStatus
   vlVaHandleVAEncMiscParameterTypeMaxFrameSizeHEVC(vlVaContext *context, VAEncMiscParameterBuffer *misc)
   {
      VAEncMiscParameterBufferMaxFrameSize *ms = (VAEncMiscParameterBufferMaxFrameSize *)misc->data;
   context->desc.h265enc.rc.max_au_size = ms->max_frame_size;
      }
      VAStatus
   vlVaHandleVAEncMiscParameterTypeHRDHEVC(vlVaContext *context, VAEncMiscParameterBuffer *misc)
   {
               if (ms->buffer_size) {
      context->desc.h265enc.rc.vbv_buffer_size = ms->buffer_size;
   context->desc.h265enc.rc.vbv_buf_lv = (ms->initial_buffer_fullness << 6 ) / ms->buffer_size;
   context->desc.h265enc.rc.vbv_buf_initial_size = ms->initial_buffer_fullness;
   /* Distinguishes from the default params set for these values in other
                        }
      void getEncParamPresetH265(vlVaContext *context)
   {
      //rate control
   if (context->desc.h265enc.rc.frame_rate_num == 0 ||
      context->desc.h265enc.rc.frame_rate_den == 0) {
   context->desc.h265enc.rc.frame_rate_num = 30;
      }
   context->desc.h265enc.rc.target_bits_picture =
      context->desc.h265enc.rc.target_bitrate *
   ((float)context->desc.h265enc.rc.frame_rate_den /
      context->desc.h265enc.rc.peak_bits_picture_integer =
      context->desc.h265enc.rc.peak_bitrate *
   ((float)context->desc.h265enc.rc.frame_rate_den /
            }
