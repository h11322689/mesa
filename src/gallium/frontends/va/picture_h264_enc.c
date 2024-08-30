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
      enum H264NALUnitType {
      H264_NAL_SPS        = 7,
      };
      VAStatus
   vlVaHandleVAEncPictureParameterBufferTypeH264(vlVaDriver *drv, vlVaContext *context, vlVaBuffer *buf)
   {
      VAEncPictureParameterBufferH264 *h264;
            h264 = buf->data;
   if (h264->pic_fields.bits.idr_pic_flag == 1)
         context->desc.h264enc.not_referenced = !h264->pic_fields.bits.reference_pic_flag;
   context->desc.h264enc.pic_order_cnt = h264->CurrPic.TopFieldOrderCnt;
   context->desc.h264enc.is_ltr = h264->CurrPic.flags & VA_PICTURE_H264_LONG_TERM_REFERENCE;
   if (context->desc.h264enc.is_ltr)
         if (context->desc.h264enc.gop_cnt == 0)
         else if (context->desc.h264enc.frame_num == 1)
                     coded_buf = handle_table_get(drv->htab, h264->coded_buf);
   if (!coded_buf)
            if (!coded_buf->derived_surface.resource)
      coded_buf->derived_surface.resource = pipe_buffer_create(drv->pipe->screen, PIPE_BIND_VERTEX_BUFFER,
               if (context->desc.h264enc.is_ltr)
      _mesa_hash_table_insert(context->desc.h264enc.frame_idx,
      UINT_TO_PTR(h264->CurrPic.picture_id + 1),
   else
      _mesa_hash_table_insert(context->desc.h264enc.frame_idx,
               if (h264->pic_fields.bits.idr_pic_flag == 1)
         else
            /* Initialize slice descriptors for this picture */
   context->desc.h264enc.num_slice_descriptors = 0;
            context->desc.h264enc.quant_i_frames = h264->pic_init_qp;
   context->desc.h264enc.quant_b_frames = h264->pic_init_qp;
   context->desc.h264enc.quant_p_frames = h264->pic_init_qp;
   context->desc.h264enc.gop_cnt++;
   if (context->desc.h264enc.gop_cnt == context->desc.h264enc.gop_size)
            context->desc.h264enc.pic_ctrl.enc_cabac_enable = h264->pic_fields.bits.entropy_coding_mode_flag;
   context->desc.h264enc.num_ref_idx_l0_active_minus1 = h264->num_ref_idx_l0_active_minus1;
   context->desc.h264enc.num_ref_idx_l1_active_minus1 = h264->num_ref_idx_l1_active_minus1;
   context->desc.h264enc.pic_ctrl.deblocking_filter_control_present_flag
         context->desc.h264enc.pic_ctrl.redundant_pic_cnt_present_flag
         context->desc.h264enc.pic_ctrl.chroma_qp_index_offset = h264->chroma_qp_index_offset;
   context->desc.h264enc.pic_ctrl.second_chroma_qp_index_offset
               }
      VAStatus
   vlVaHandleVAEncSliceParameterBufferTypeH264(vlVaDriver *drv, vlVaContext *context, vlVaBuffer *buf)
   {
               h264 = buf->data;
   memset(&context->desc.h264enc.ref_idx_l0_list, VA_INVALID_ID, sizeof(context->desc.h264enc.ref_idx_l0_list));
            if(h264->num_ref_idx_active_override_flag) {
      context->desc.h264enc.num_ref_idx_l0_active_minus1 = h264->num_ref_idx_l0_active_minus1;
               for (int i = 0; i < 32; i++) {
      if (h264->RefPicList0[i].picture_id != VA_INVALID_ID) {
            context->desc.h264enc.ref_idx_l0_list[i] = PTR_TO_UINT(util_hash_table_get(context->desc.h264enc.frame_idx,
            }
   if (h264->RefPicList1[i].picture_id != VA_INVALID_ID && h264->slice_type == 1) {
         context->desc.h264enc.ref_idx_l1_list[i] = PTR_TO_UINT(util_hash_table_get(context->desc.h264enc.frame_idx,
         context->desc.h264enc.l1_is_long_term[i] = h264->RefPicList1[i].flags &
               /**
   *  VAEncSliceParameterBufferH264.slice_type
   *  Slice type.
   *  Range: 0..2, 5..7, i.e. no switching slices.
   */
   struct h264_slice_descriptor slice_descriptor;
   memset(&slice_descriptor, 0, sizeof(slice_descriptor));
   slice_descriptor.macroblock_address = h264->macroblock_address;
            if ((h264->slice_type == 1) || (h264->slice_type == 6)) {
      context->desc.h264enc.picture_type = PIPE_H2645_ENC_PICTURE_TYPE_B;
      } else if ((h264->slice_type == 0) || (h264->slice_type == 5)) {
      context->desc.h264enc.picture_type = PIPE_H2645_ENC_PICTURE_TYPE_P;
      } else if ((h264->slice_type == 2) || (h264->slice_type == 7)) {
      if (context->desc.h264enc.picture_type == PIPE_H2645_ENC_PICTURE_TYPE_IDR) {
      if (slice_descriptor.macroblock_address == 0) {
      /* Increment it only for the first slice of the IDR frame */
      }
      } else {
      context->desc.h264enc.picture_type = PIPE_H2645_ENC_PICTURE_TYPE_I;
         } else {
                  context->desc.h264enc.pic_ctrl.enc_cabac_init_idc = h264->cabac_init_idc;
   context->desc.h264enc.dbk.disable_deblocking_filter_idc = h264->disable_deblocking_filter_idc;
   context->desc.h264enc.dbk.alpha_c0_offset_div2 = h264->slice_alpha_c0_offset_div2;
            /* Handle the slice control parameters */
   if (context->desc.h264enc.num_slice_descriptors < ARRAY_SIZE(context->desc.h264enc.slices_descriptors)) {
         } else {
                     }
      VAStatus
   vlVaHandleVAEncSequenceParameterBufferTypeH264(vlVaDriver *drv, vlVaContext *context, vlVaBuffer *buf)
   {
               VAEncSequenceParameterBufferH264 *h264 = (VAEncSequenceParameterBufferH264 *)buf->data;
   if (!context->decoder) {
      context->templat.max_references = h264->max_num_ref_frames;
   context->templat.level = h264->level_idc;
   context->decoder = drv->pipe->create_video_codec(drv->pipe, &context->templat);
   if (!context->decoder)
            getEncParamPresetH264(context);
   context->desc.h264enc.rate_ctrl[0].vbv_buffer_size = 20000000;
   context->desc.h264enc.rate_ctrl[0].vbv_buf_lv = 48;
   context->desc.h264enc.rate_ctrl[0].fill_data_enable = 1;
   context->desc.h264enc.rate_ctrl[0].enforce_hrd = 1;
   context->desc.h264enc.rate_ctrl[0].max_qp = 51;
   context->desc.h264enc.rate_ctrl[0].min_qp = 0;
               context->desc.h264enc.intra_idr_period =
         context->gop_coeff = ((1024 + context->desc.h264enc.intra_idr_period - 1) /
         if (context->gop_coeff > VL_VA_ENC_GOP_COEFF)
         context->desc.h264enc.gop_size = context->desc.h264enc.intra_idr_period * context->gop_coeff;
   context->desc.h264enc.seq.pic_order_cnt_type = h264->seq_fields.bits.pic_order_cnt_type;
   context->desc.h264enc.seq.vui_parameters_present_flag = h264->vui_parameters_present_flag;
   if (h264->vui_parameters_present_flag) {
      context->desc.h264enc.seq.vui_flags.aspect_ratio_info_present_flag =
         context->desc.h264enc.seq.aspect_ratio_idc = h264->aspect_ratio_idc;
   context->desc.h264enc.seq.sar_width = h264->sar_width;
   context->desc.h264enc.seq.sar_height = h264->sar_height;
   context->desc.h264enc.seq.vui_flags.timing_info_present_flag =
         num_units_in_tick = h264->num_units_in_tick;
      } else
            if (!context->desc.h264enc.seq.vui_flags.timing_info_present_flag) {
      /* if not present, set default value */
   num_units_in_tick = PIPE_DEFAULT_FRAME_RATE_DEN;
               context->desc.h264enc.seq.num_units_in_tick = num_units_in_tick;
   context->desc.h264enc.seq.time_scale = time_scale;
   context->desc.h264enc.rate_ctrl[0].frame_rate_num = time_scale / 2;
            if (h264->frame_cropping_flag) {
      context->desc.h264enc.seq.enc_frame_cropping_flag = h264->frame_cropping_flag;
   context->desc.h264enc.seq.enc_frame_crop_left_offset = h264->frame_crop_left_offset;
   context->desc.h264enc.seq.enc_frame_crop_right_offset = h264->frame_crop_right_offset;
   context->desc.h264enc.seq.enc_frame_crop_top_offset = h264->frame_crop_top_offset;
      }
      }
      VAStatus
   vlVaHandleVAEncMiscParameterTypeRateControlH264(vlVaContext *context, VAEncMiscParameterBuffer *misc)
   {
      unsigned temporal_id;
            temporal_id = context->desc.h264enc.rate_ctrl[0].rate_ctrl_method !=
                        if (context->desc.h264enc.rate_ctrl[0].rate_ctrl_method ==
      PIPE_H2645_ENC_RATE_CONTROL_METHOD_CONSTANT)
   context->desc.h264enc.rate_ctrl[temporal_id].target_bitrate =
      else
      context->desc.h264enc.rate_ctrl[temporal_id].target_bitrate =
         if (context->desc.h264enc.seq.num_temporal_layers > 0 &&
      temporal_id >= context->desc.h264enc.seq.num_temporal_layers)
         context->desc.h264enc.rate_ctrl[temporal_id].fill_data_enable = !(rc->rc_flags.bits.disable_bit_stuffing);
   /* context->desc.h264enc.rate_ctrl[temporal_id].skip_frame_enable = !(rc->rc_flags.bits.disable_frame_skip); */
   context->desc.h264enc.rate_ctrl[temporal_id].skip_frame_enable = 0;
            if ((context->desc.h264enc.rate_ctrl[0].rate_ctrl_method == PIPE_H2645_ENC_RATE_CONTROL_METHOD_CONSTANT) ||
      (context->desc.h264enc.rate_ctrl[0].rate_ctrl_method == PIPE_H2645_ENC_RATE_CONTROL_METHOD_CONSTANT_SKIP))
   context->desc.h264enc.rate_ctrl[temporal_id].vbv_buffer_size =
      else if (context->desc.h264enc.rate_ctrl[temporal_id].target_bitrate < 2000000)
      context->desc.h264enc.rate_ctrl[temporal_id].vbv_buffer_size =
      else
      context->desc.h264enc.rate_ctrl[temporal_id].vbv_buffer_size =
         context->desc.h264enc.rate_ctrl[temporal_id].max_qp = rc->max_qp;
   context->desc.h264enc.rate_ctrl[temporal_id].min_qp = rc->min_qp;
   /* Distinguishes from the default params set for these values in other
                  if (context->desc.h264enc.rate_ctrl[0].rate_ctrl_method ==
      PIPE_H2645_ENC_RATE_CONTROL_METHOD_QUALITY_VARIABLE)
   context->desc.h264enc.rate_ctrl[temporal_id].vbr_quality_factor =
            }
      VAStatus
   vlVaHandleVAEncMiscParameterTypeFrameRateH264(vlVaContext *context, VAEncMiscParameterBuffer *misc)
   {
      unsigned temporal_id;
            temporal_id = context->desc.h264enc.rate_ctrl[0].rate_ctrl_method !=
                        if (context->desc.h264enc.seq.num_temporal_layers > 0 &&
      temporal_id >= context->desc.h264enc.seq.num_temporal_layers)
         if (fr->framerate & 0xffff0000) {
      context->desc.h264enc.rate_ctrl[temporal_id].frame_rate_num = fr->framerate       & 0xffff;
      } else {
      context->desc.h264enc.rate_ctrl[temporal_id].frame_rate_num = fr->framerate;
                  }
      static void parseEncHrdParamsH264(struct vl_rbsp *rbsp)
   {
               cpb_cnt_minus1 = vl_rbsp_ue(rbsp);
   vl_rbsp_u(rbsp, 4); /* bit_rate_scale */
   vl_rbsp_u(rbsp, 4); /* cpb_size_scale */
   for (i = 0; i <= cpb_cnt_minus1; ++i) {
      vl_rbsp_ue(rbsp); /* bit_rate_value_minus1[i] */
   vl_rbsp_ue(rbsp); /* cpb_size_value_minus1[i] */
      }
   vl_rbsp_u(rbsp, 5); /* initial_cpb_removal_delay_length_minus1 */
   vl_rbsp_u(rbsp, 5); /* cpb_removal_delay_length_minus1 */
   vl_rbsp_u(rbsp, 5); /* dpb_output_delay_length_minus1 */
      }
      static void parseEncSpsParamsH264(vlVaContext *context, struct vl_rbsp *rbsp)
   {
      unsigned i, profile_idc, num_ref_frames_in_pic_order_cnt_cycle;
                     vl_rbsp_u(rbsp, 8); /* constraint_set_flags */
                     if (profile_idc == 100 || profile_idc == 110 ||
      profile_idc == 122 || profile_idc == 244 || profile_idc == 44 ||
   profile_idc == 83 || profile_idc == 86 || profile_idc == 118 ||
   profile_idc == 128 || profile_idc == 138 || profile_idc == 139 ||
            if (vl_rbsp_ue(rbsp) == 3) /* chroma_format_idc */
            vl_rbsp_ue(rbsp); /* bit_depth_luma_minus8 */
   vl_rbsp_ue(rbsp); /* bit_depth_chroma_minus8 */
            if (vl_rbsp_u(rbsp, 1)) /* seq_scaling_matrix_present_flag */
               vl_rbsp_ue(rbsp); /* log2_max_frame_num_minus4 */
            if (context->desc.h264enc.seq.pic_order_cnt_type == 0)
         else if (context->desc.h264enc.seq.pic_order_cnt_type == 1) {
      vl_rbsp_u(rbsp, 1); /* delta_pic_order_always_zero_flag */
   vl_rbsp_se(rbsp); /* offset_for_non_ref_pic */
   vl_rbsp_se(rbsp); /* offset_for_top_to_bottom_field */
   num_ref_frames_in_pic_order_cnt_cycle = vl_rbsp_ue(rbsp);
   for (i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; ++i)
               vl_rbsp_ue(rbsp); /* max_num_ref_frames */
   vl_rbsp_u(rbsp, 1); /* gaps_in_frame_num_value_allowed_flag */
   vl_rbsp_ue(rbsp); /* pic_width_in_mbs_minus1 */
   vl_rbsp_ue(rbsp); /* pic_height_in_map_units_minus1 */
   if (!vl_rbsp_u(rbsp, 1)) /* frame_mbs_only_flag */
            vl_rbsp_u(rbsp, 1); /* direct_8x8_inference_flag */
   if (vl_rbsp_u(rbsp, 1)) { /* frame_cropping_flag */
      vl_rbsp_ue(rbsp); /* frame_crop_left_offset */
   vl_rbsp_ue(rbsp); /* frame_crop_right_offset */
   vl_rbsp_ue(rbsp); /* frame_crop_top_offset */
               context->desc.h264enc.seq.vui_parameters_present_flag = vl_rbsp_u(rbsp, 1);
   if (context->desc.h264enc.seq.vui_parameters_present_flag) {
      context->desc.h264enc.seq.vui_flags.aspect_ratio_info_present_flag = vl_rbsp_u(rbsp, 1);
   if (context->desc.h264enc.seq.vui_flags.aspect_ratio_info_present_flag) {
      if (vl_rbsp_u(rbsp, 8) == 255) { /* aspect_ratio_idc == Extended_SAR */
      vl_rbsp_u(rbsp, 16); /* sar_width */
                  if (vl_rbsp_u(rbsp, 1)) /* overscan_info_present_flag */
            context->desc.h264enc.seq.vui_flags.video_signal_type_present_flag = vl_rbsp_u(rbsp, 1);
   if (context->desc.h264enc.seq.vui_flags.video_signal_type_present_flag) {
      context->desc.h264enc.seq.video_format = vl_rbsp_u(rbsp, 3);
   context->desc.h264enc.seq.video_full_range_flag = vl_rbsp_u(rbsp, 1);
   context->desc.h264enc.seq.vui_flags.colour_description_present_flag = vl_rbsp_u(rbsp, 1);
   if (context->desc.h264enc.seq.vui_flags.colour_description_present_flag) {
      context->desc.h264enc.seq.colour_primaries = vl_rbsp_u(rbsp, 8);
   context->desc.h264enc.seq.transfer_characteristics = vl_rbsp_u(rbsp, 8);
                  context->desc.h264enc.seq.vui_flags.chroma_loc_info_present_flag = vl_rbsp_u(rbsp, 1);
   if (context->desc.h264enc.seq.vui_flags.chroma_loc_info_present_flag) {
      context->desc.h264enc.seq.chroma_sample_loc_type_top_field = vl_rbsp_ue(rbsp);
               if (vl_rbsp_u(rbsp, 1)) { /* timing_info_present_flag */
      vl_rbsp_u(rbsp, 32); /* num_units_in_tick */
   vl_rbsp_u(rbsp, 32); /* time_scale */
               nal_hrd_parameters_present_flag = vl_rbsp_u(rbsp, 1);
   if (nal_hrd_parameters_present_flag)
            vcl_hrd_parameters_present_flag = vl_rbsp_u(rbsp, 1);
   if (vcl_hrd_parameters_present_flag)
            if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag)
                     if (vl_rbsp_u(rbsp, 1)) { /* bitstream_restriction_flag */
      vl_rbsp_u(rbsp, 1); /* motion_vectors_over_pic_boundaries_flag */
   vl_rbsp_ue(rbsp); /* max_bytes_per_pic_denom */
   vl_rbsp_ue(rbsp); /* max_bits_per_mb_denom */
   vl_rbsp_ue(rbsp); /* log2_max_mv_length_horizontal */
   vl_rbsp_ue(rbsp); /* log2_max_mv_length_vertical */
   context->desc.h264enc.seq.max_num_reorder_frames = vl_rbsp_ue(rbsp);
            }
      VAStatus
   vlVaHandleVAEncPackedHeaderDataBufferTypeH264(vlVaContext *context, vlVaBuffer *buf)
   {
      struct vl_vlc vlc = {0};
            while (vl_vlc_bits_left(&vlc) > 0) {
      /* search the first 64 bytes for a startcode */
   for (int i = 0; i < 64 && vl_vlc_bits_left(&vlc) >= 24; ++i) {
      if (vl_vlc_peekbits(&vlc, 24) == 0x000001)
         vl_vlc_eatbits(&vlc, 8);
      }
            if (vl_vlc_valid_bits(&vlc) < 15)
            vl_vlc_eatbits(&vlc, 3);
            struct vl_rbsp rbsp;
            switch(nal_unit_type) {
   case H264_NAL_SPS:
      parseEncSpsParamsH264(context, &rbsp);
      case H264_NAL_PPS:
   default:
                  if (!context->packed_header_emulation_bytes)
                  }
      VAStatus
   vlVaHandleVAEncMiscParameterTypeTemporalLayerH264(vlVaContext *context, VAEncMiscParameterBuffer *misc)
   {
                           }
      VAStatus
   vlVaHandleVAEncMiscParameterTypeQualityLevelH264(vlVaContext *context, VAEncMiscParameterBuffer *misc)
   {
      VAEncMiscParameterBufferQualityLevel *ql = (VAEncMiscParameterBufferQualityLevel *)misc->data;
   vlVaHandleVAEncMiscParameterTypeQualityLevel(&context->desc.h264enc.quality_modes,
               }
      VAStatus
   vlVaHandleVAEncMiscParameterTypeMaxFrameSizeH264(vlVaContext *context, VAEncMiscParameterBuffer *misc)
   {
      VAEncMiscParameterBufferMaxFrameSize *ms = (VAEncMiscParameterBufferMaxFrameSize *)misc->data;
   context->desc.h264enc.rate_ctrl[0].max_au_size = ms->max_frame_size;
      }
      VAStatus
   vlVaHandleVAEncMiscParameterTypeHRDH264(vlVaContext *context, VAEncMiscParameterBuffer *misc)
   {
               if (ms->buffer_size) {
      context->desc.h264enc.rate_ctrl[0].vbv_buffer_size = ms->buffer_size;
   context->desc.h264enc.rate_ctrl[0].vbv_buf_lv = (ms->initial_buffer_fullness << 6 ) / ms->buffer_size;
   context->desc.h264enc.rate_ctrl[0].vbv_buf_initial_size = ms->initial_buffer_fullness;
   /* Distinguishes from the default params set for these values in other
   functions and app specific params passed down via HRD buffer */
                  }
      void getEncParamPresetH264(vlVaContext *context)
   {
      //rate control
   if (context->desc.h264enc.rate_ctrl[0].frame_rate_num == 0 ||
      context->desc.h264enc.rate_ctrl[0].frame_rate_den == 0) {
      context->desc.h264enc.rate_ctrl[0].frame_rate_num = 30;
   }
   context->desc.h264enc.rate_ctrl[0].target_bits_picture =
      context->desc.h264enc.rate_ctrl[0].target_bitrate *
   ((float)context->desc.h264enc.rate_ctrl[0].frame_rate_den /
      context->desc.h264enc.rate_ctrl[0].peak_bits_picture_integer =
      context->desc.h264enc.rate_ctrl[0].peak_bitrate *
   ((float)context->desc.h264enc.rate_ctrl[0].frame_rate_den /
            }
