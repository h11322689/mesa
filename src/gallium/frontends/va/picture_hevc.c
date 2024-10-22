   /**************************************************************************
   *
   * Copyright 2014 Advanced Micro Devices, Inc.
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
      #include "util/macros.h"
   #include "vl/vl_zscan.h"
   #include "va_private.h"
      void vlVaHandlePictureParameterBufferHEVC(vlVaDriver *drv, vlVaContext *context, vlVaBuffer *buf)
   {
      VAPictureParameterBufferHEVC *hevc = buf->data;
            assert(buf->size >= sizeof(VAPictureParameterBufferHEVC) && buf->num_elements == 1);
   context->desc.h265.pps->sps->chroma_format_idc = hevc->pic_fields.bits.chroma_format_idc;
   context->desc.h265.pps->sps->separate_colour_plane_flag =
         context->desc.h265.pps->sps->no_pic_reordering_flag =
         context->desc.h265.pps->sps->no_bi_pred_flag =
         context->desc.h265.pps->sps->pic_width_in_luma_samples = hevc->pic_width_in_luma_samples;
   context->desc.h265.pps->sps->pic_height_in_luma_samples = hevc->pic_height_in_luma_samples;
   context->desc.h265.pps->sps->bit_depth_luma_minus8 = hevc->bit_depth_luma_minus8;
   context->desc.h265.pps->sps->bit_depth_chroma_minus8 = hevc->bit_depth_chroma_minus8;
   context->desc.h265.pps->sps->log2_max_pic_order_cnt_lsb_minus4 =
         context->desc.h265.pps->sps->sps_max_dec_pic_buffering_minus1 =
         context->desc.h265.pps->sps->log2_min_luma_coding_block_size_minus3 =
         context->desc.h265.pps->sps->log2_diff_max_min_luma_coding_block_size =
         context->desc.h265.pps->sps->log2_min_transform_block_size_minus2 =
         context->desc.h265.pps->sps->log2_diff_max_min_transform_block_size =
         context->desc.h265.pps->sps->max_transform_hierarchy_depth_inter =
         context->desc.h265.pps->sps->max_transform_hierarchy_depth_intra =
         context->desc.h265.pps->sps->scaling_list_enabled_flag =
         context->desc.h265.pps->sps->amp_enabled_flag = hevc->pic_fields.bits.amp_enabled_flag;
   context->desc.h265.pps->sps->sample_adaptive_offset_enabled_flag =
         context->desc.h265.pps->sps->pcm_enabled_flag = hevc->pic_fields.bits.pcm_enabled_flag;
   if (hevc->pic_fields.bits.pcm_enabled_flag == 1) {
      context->desc.h265.pps->sps->pcm_sample_bit_depth_luma_minus1 =
         context->desc.h265.pps->sps->pcm_sample_bit_depth_chroma_minus1 =
         context->desc.h265.pps->sps->log2_min_pcm_luma_coding_block_size_minus3 =
         context->desc.h265.pps->sps->log2_diff_max_min_pcm_luma_coding_block_size =
         context->desc.h265.pps->sps->pcm_loop_filter_disabled_flag =
      }
   context->desc.h265.pps->sps->num_short_term_ref_pic_sets = hevc->num_short_term_ref_pic_sets;
   context->desc.h265.pps->sps->long_term_ref_pics_present_flag =
         context->desc.h265.pps->sps->num_long_term_ref_pics_sps = hevc->num_long_term_ref_pic_sps;
   context->desc.h265.pps->sps->sps_temporal_mvp_enabled_flag =
         context->desc.h265.pps->sps->strong_intra_smoothing_enabled_flag =
            context->desc.h265.pps->dependent_slice_segments_enabled_flag =
         context->desc.h265.pps->output_flag_present_flag =
         context->desc.h265.pps->num_extra_slice_header_bits = hevc->num_extra_slice_header_bits;
   context->desc.h265.pps->sign_data_hiding_enabled_flag =
         context->desc.h265.pps->cabac_init_present_flag =
         context->desc.h265.pps->num_ref_idx_l0_default_active_minus1 =
         context->desc.h265.pps->num_ref_idx_l1_default_active_minus1 =
         context->desc.h265.pps->init_qp_minus26 = hevc->init_qp_minus26;
   context->desc.h265.pps->constrained_intra_pred_flag =
         context->desc.h265.pps->transform_skip_enabled_flag =
         context->desc.h265.pps->cu_qp_delta_enabled_flag =
         context->desc.h265.pps->diff_cu_qp_delta_depth = hevc->diff_cu_qp_delta_depth;
   context->desc.h265.pps->pps_cb_qp_offset = hevc->pps_cb_qp_offset;
   context->desc.h265.pps->pps_cr_qp_offset = hevc->pps_cr_qp_offset;
   context->desc.h265.pps->pps_slice_chroma_qp_offsets_present_flag =
         context->desc.h265.pps->weighted_pred_flag = hevc->pic_fields.bits.weighted_pred_flag;
   context->desc.h265.pps->weighted_bipred_flag = hevc->pic_fields.bits.weighted_bipred_flag;
   context->desc.h265.pps->transquant_bypass_enabled_flag =
         context->desc.h265.pps->tiles_enabled_flag = hevc->pic_fields.bits.tiles_enabled_flag;
   context->desc.h265.pps->entropy_coding_sync_enabled_flag =
         if (hevc->pic_fields.bits.tiles_enabled_flag == 1) {
      context->desc.h265.pps->num_tile_columns_minus1 = hevc->num_tile_columns_minus1;
   context->desc.h265.pps->num_tile_rows_minus1 = hevc->num_tile_rows_minus1;
   for (i = 0 ; i < 19 ; i++)
         for (i = 0 ; i < 21 ; i++)
         context->desc.h265.pps->loop_filter_across_tiles_enabled_flag =
      }
   context->desc.h265.pps->pps_loop_filter_across_slices_enabled_flag =
         context->desc.h265.pps->deblocking_filter_override_enabled_flag =
         context->desc.h265.pps->pps_deblocking_filter_disabled_flag =
         context->desc.h265.pps->pps_beta_offset_div2 = hevc->pps_beta_offset_div2;
   context->desc.h265.pps->pps_tc_offset_div2 = hevc->pps_tc_offset_div2;
   context->desc.h265.pps->lists_modification_present_flag =
         context->desc.h265.pps->log2_parallel_merge_level_minus2 =
         context->desc.h265.pps->slice_segment_header_extension_present_flag =
            context->desc.h265.IDRPicFlag = hevc->slice_parsing_fields.bits.IdrPicFlag;
   context->desc.h265.RAPPicFlag = hevc->slice_parsing_fields.bits.RapPicFlag;
                     for (i = 0 ; i < 8 ; i++) {
      context->desc.h265.RefPicSetStCurrBefore[i] = 0xFF;
   context->desc.h265.RefPicSetStCurrAfter[i] = 0xFF;
      }
   context->desc.h265.NumPocStCurrBefore = 0;
   context->desc.h265.NumPocStCurrAfter = 0;
   context->desc.h265.NumPocLtCurr = 0;
   unsigned int iBefore = 0;
   unsigned int iAfter = 0;
   unsigned int iCurr = 0;
   for (i = 0 ; i < 15 ; i++) {
                        if ((hevc->ReferenceFrames[i].flags & VA_PICTURE_HEVC_RPS_ST_CURR_BEFORE) && (iBefore < 8)) {
      context->desc.h265.RefPicSetStCurrBefore[iBefore++] = i;
      }
   if ((hevc->ReferenceFrames[i].flags & VA_PICTURE_HEVC_RPS_ST_CURR_AFTER) && (iAfter < 8)) {
      context->desc.h265.RefPicSetStCurrAfter[iAfter++] = i;
      }
   if ((hevc->ReferenceFrames[i].flags & VA_PICTURE_HEVC_RPS_LT_CURR) && (iCurr < 8)) {
      context->desc.h265.RefPicSetLtCurr[iCurr++] = i;
      }
      }
   context->desc.h265.pps->st_rps_bits = hevc->st_rps_bits;
            context->desc.h265.slice_parameter.slice_count = 0;
   context->desc.h265.slice_parameter.slice_info_present = false;
   memset(context->desc.h265.slice_parameter.slice_data_flag, 0, sizeof(context->desc.h265.slice_parameter.slice_data_flag));
   memset(context->desc.h265.slice_parameter.slice_data_offset, 0, sizeof(context->desc.h265.slice_parameter.slice_data_offset));
      }
      void vlVaHandleIQMatrixBufferHEVC(vlVaContext *context, vlVaBuffer *buf)
   {
      VAIQMatrixBufferHEVC *h265 = buf->data;
                     for (i = 0; i < 6; i++) {
      for (j = 0; j < 16; j++)
                  for (j = 0; j < 64; j++) {
      context->desc.h265.pps->sps->ScalingList8x8[i][j] =
                        if (i < 2)
      context->desc.h265.pps->sps->ScalingList32x32[i][j] =
            context->desc.h265.pps->sps->ScalingListDCCoeff16x16[i] =
         if (i < 2)
      context->desc.h265.pps->sps->ScalingListDCCoeff32x32[i] =
      }
      void vlVaHandleSliceParameterBufferHEVC(vlVaContext *context, vlVaBuffer *buf)
   {
                        switch(h265->LongSliceFlags.fields.slice_type) {
   /* Depending on slice_type, only update relevant reference */
   case 0: /* HEVC_SLICE_B */
      for (int j = 0 ; j < 15 ; j++)
            case 1: /* HEVC_SLICE_P */
      for (int j = 0 ; j < 15 ; j++)
            default:
         }
            ASSERTED const size_t max_pipe_hevc_slices = ARRAY_SIZE(context->desc.h265.slice_parameter.slice_data_offset);
            context->desc.h265.slice_parameter.slice_info_present = true;
   context->desc.h265.slice_parameter.slice_data_size[context->desc.h265.slice_parameter.slice_count] = h265->slice_data_size;
            switch (h265->slice_data_flag) {
   case VA_SLICE_DATA_FLAG_ALL:
      context->desc.h265.slice_parameter.slice_data_flag[context->desc.h265.slice_parameter.slice_count] = PIPE_SLICE_BUFFER_PLACEMENT_TYPE_WHOLE;
      case VA_SLICE_DATA_FLAG_BEGIN:
      context->desc.h265.slice_parameter.slice_data_flag[context->desc.h265.slice_parameter.slice_count] = PIPE_SLICE_BUFFER_PLACEMENT_TYPE_BEGIN;
      case VA_SLICE_DATA_FLAG_MIDDLE:
      context->desc.h265.slice_parameter.slice_data_flag[context->desc.h265.slice_parameter.slice_count] = PIPE_SLICE_BUFFER_PLACEMENT_TYPE_MIDDLE;
      case VA_SLICE_DATA_FLAG_END:
      context->desc.h265.slice_parameter.slice_data_flag[context->desc.h265.slice_parameter.slice_count] = PIPE_SLICE_BUFFER_PLACEMENT_TYPE_END;
      default:
                  /* assert(buf->num_elements == 1) above; */
      }
