   /**************************************************************************
   *
   * Copyright 2016 Advanced Micro Devices, Inc.
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
      #include "pipe/p_video_codec.h"
   #include "util/u_memory.h"
   #include "util/u_video.h"
   #include "util/vl_rbsp.h"
      #include "entrypoint.h"
   #include "vid_dec.h"
      #define DPB_MAX_SIZE 32
   #define MAX_NUM_REF_PICS 16
      enum {
      NAL_UNIT_TYPE_TRAIL_N = 0,
   NAL_UNIT_TYPE_TRAIL_R = 1,
   NAL_UNIT_TYPE_TSA_N = 2,
   NAL_UNIT_TYPE_TSA_R = 3,
   NAL_UNIT_TYPE_STSA_N = 4,
   NAL_UNIT_TYPE_STSA_R = 5,
   NAL_UNIT_TYPE_RADL_N = 6,
   NAL_UNIT_TYPE_RADL_R = 7,
   NAL_UNIT_TYPE_RASL_N = 8,
   NAL_UNIT_TYPE_RASL_R = 9,
   NAL_UNIT_TYPE_BLA_W_LP = 16,
   NAL_UNIT_TYPE_BLA_W_RADL = 17,
   NAL_UNIT_TYPE_BLA_N_LP =  18,
   NAL_UNIT_TYPE_IDR_W_RADL = 19,
   NAL_UNIT_TYPE_IDR_N_LP = 20,
   NAL_UNIT_TYPE_CRA = 21,
   NAL_UNIT_TYPE_SPS = 33,
      };
      static const uint8_t Default_8x8_Intra[64] = {
      16, 16, 16, 16, 17, 18, 21, 24,
   16, 16, 16, 16, 17, 19, 22, 25,
   16, 16, 17, 18, 20, 22, 25, 29,
   16, 16, 18, 21, 24, 27, 31, 36,
   17, 17, 20, 24, 30, 35, 41, 47,
   18, 19, 22, 27, 35, 44, 54, 65,
   21, 22, 25, 31, 41, 54, 70, 88,
      };
      static const uint8_t Default_8x8_Inter[64] = {
      16, 16, 16, 16, 17, 18, 20, 24,
   16, 16, 16, 17, 18, 20, 24, 25,
   16, 16, 17, 18, 20, 24, 25, 28,
   16, 17, 18, 20, 24, 25, 28, 33,
   17, 18, 20, 24, 25, 28, 33, 41,
   18, 20, 24, 25, 28, 33, 41, 54,
   20, 24, 25, 28, 33, 41, 54, 71,
      };
      struct dpb_list {
      struct list_head list;
   struct pipe_video_buffer *buffer;
   OMX_TICKS timestamp;
      };
      struct ref_pic_set {
   unsigned  num_pics;
   unsigned  num_neg_pics;
   unsigned  num_pos_pics;
   unsigned  num_delta_poc;
   int  delta_poc[MAX_NUM_REF_PICS];
   bool used[MAX_NUM_REF_PICS];
   };
      static bool is_idr_picture(unsigned nal_unit_type)
   {
      return (nal_unit_type == NAL_UNIT_TYPE_IDR_W_RADL ||
      }
      /* broken link access picture */
   static bool is_bla_picture(unsigned nal_unit_type)
   {
      return (nal_unit_type == NAL_UNIT_TYPE_BLA_W_LP ||
            }
      /* random access point picture */
   static bool is_rap_picture(unsigned nal_unit_type)
   {
      return (nal_unit_type >= NAL_UNIT_TYPE_BLA_W_LP &&
      }
      static bool is_slice_picture(unsigned nal_unit_type)
   {
      return (nal_unit_type <= NAL_UNIT_TYPE_RASL_R ||
      }
      static void set_poc(vid_dec_PrivateType *priv,
         {
               if (priv->codec_data.h265.temporal_id == 0 &&
      (nal_unit_type == NAL_UNIT_TYPE_TRAIL_R ||
   nal_unit_type == NAL_UNIT_TYPE_TSA_R ||
   nal_unit_type == NAL_UNIT_TYPE_STSA_R ||
   is_rap_picture(nal_unit_type)))
   }
      static unsigned get_poc(vid_dec_PrivateType *priv)
   {
         }
      static void profile_tier(struct vl_rbsp *rbsp)
   {
               /* general_profile_space */
            /* general_tier_flag */
            /* general_profile_idc */
            /* general_profile_compatibility_flag */
   for(i = 0; i < 32; ++i)
            /* general_progressive_source_flag */
            /* general_interlaced_source_flag */
            /* general_non_packed_constraint_flag */
            /* general_frame_only_constraint_flag */
            /* general_reserved_zero_44bits */
   vl_rbsp_u(rbsp, 16);
   vl_rbsp_u(rbsp, 16);
      }
      static unsigned profile_tier_level(struct vl_rbsp *rbsp,
         {
      bool sub_layer_profile_present_flag[6];
   bool sub_layer_level_present_flag[6];
   unsigned level_idc;
                     /* general_level_idc */
            for (i = 0; i < max_sublayers_minus1; ++i) {
      sub_layer_profile_present_flag[i] = vl_rbsp_u(rbsp, 1);
               if (max_sublayers_minus1 > 0)
      for (i = max_sublayers_minus1; i < 8; ++i)
               for (i = 0; i < max_sublayers_minus1; ++i) {
      if (sub_layer_profile_present_flag[i])
            if (sub_layer_level_present_flag[i])
      /* sub_layer_level_idc */
               }
      static void scaling_list_data(vid_dec_PrivateType *priv,
         {
      unsigned size_id, matrix_id;
   unsigned scaling_list_len[4] = { 16, 64, 64, 64 };
   uint8_t scaling_list4x4[6][64] = {  };
            uint8_t (*scaling_list_data[4])[6][64] = {
      (uint8_t (*)[6][64])scaling_list4x4,
   (uint8_t (*)[6][64])sps->ScalingList8x8,
   (uint8_t (*)[6][64])sps->ScalingList16x16,
      };
   uint8_t (*scaling_list_dc_coeff[2])[6] = {
      (uint8_t (*)[6])sps->ScalingListDCCoeff16x16,
                           for (matrix_id = 0; matrix_id < ((size_id == 3) ? 2 : 6); ++matrix_id) {
               if (!scaling_list_pred_mode_flag) {
                     if (matrix_id != matrix_id_with_delta) {
      memcpy((*scaling_list_data[size_id])[matrix_id],
         (*scaling_list_data[size_id])[matrix_id_with_delta],
   if (size_id > 1)
      (*scaling_list_dc_coeff[size_id - 2])[matrix_id] =
                     if (size_id == 0)
         else {
      if (size_id < 3)
         else
         memcpy((*scaling_list_data[size_id])[matrix_id], d,
      }
   if (size_id > 1)
         } else {
                     if (size_id > 1) {
      /* scaling_list_dc_coef_minus8 */
                     for (i = 0; i < coef_num; ++i) {
      /* scaling_list_delta_coef */
   next_coef = (next_coef + vl_rbsp_se(rbsp) + 256) % 256;
                        for (i = 0; i < 6; ++i)
               }
      static void st_ref_pic_set(vid_dec_PrivateType *priv, struct vl_rbsp *rbsp,
               {
      bool inter_rps_pred_flag;
   unsigned delta_idx_minus1;
   int delta_poc;
                     if (inter_rps_pred_flag) {
      struct ref_pic_set *ref_rps;
   unsigned sign, abs;
   int delta_rps;
   bool used;
            if (idx == sps->num_short_term_ref_pic_sets)
         else
            ref_rps = (struct ref_pic_set *)
            /* delta_rps_sign */
   sign = vl_rbsp_u(rbsp, 1);
   /* abs_delta_rps_minus1 */
   abs = vl_rbsp_ue(rbsp);
            rps->num_neg_pics = 0;
   rps->num_pos_pics = 0;
            for(i = 0 ; i <= ref_rps->num_pics; ++i) {
      /* used_by_curr_pic_flag */
   if (!vl_rbsp_u(rbsp, 1))
      /* use_delta_flag */
      else {
      delta_poc = delta_rps +
         rps->delta_poc[rps->num_pics] = delta_poc;
   rps->used[rps->num_pics] = true;
   if (delta_poc < 0)
         else
                                 /* sort delta poc */
   for (i = 1; i < rps->num_pics; ++i) {
      delta_poc = rps->delta_poc[i];
   used = rps->used[i];
   for (j = i - 1; j >= 0; j--) {
      if (delta_poc < rps->delta_poc[j]) {
      rps->delta_poc[j + 1] = rps->delta_poc[j];
   rps->used[j + 1] = rps->used[j];
   rps->delta_poc[j] = delta_poc;
                     for (i = 0 , j = rps->num_neg_pics - 1;
      i < rps->num_neg_pics >> 1; i++, j--) {
   delta_poc = rps->delta_poc[i];
   used = rps->used[i];
   rps->delta_poc[i] = rps->delta_poc[j];
   rps->used[i] = rps->used[j];
   rps->delta_poc[j] = delta_poc;
         } else {
      /* num_negative_pics */
   rps->num_neg_pics = vl_rbsp_ue(rbsp);
   /* num_positive_pics */
   rps->num_pos_pics = vl_rbsp_ue(rbsp);
            delta_poc = 0;
   for(i = 0 ; i < rps->num_neg_pics; ++i) {
      /* delta_poc_s0_minus1 */
   delta_poc -= (vl_rbsp_ue(rbsp) + 1);
   rps->delta_poc[i] = delta_poc;
   /* used_by_curr_pic_s0_flag */
               delta_poc = 0;
   for(i = rps->num_neg_pics; i < rps->num_pics; ++i) {
      /* delta_poc_s1_minus1 */
   delta_poc += (vl_rbsp_ue(rbsp) + 1);
   rps->delta_poc[i] = delta_poc;
   /* used_by_curr_pic_s1_flag */
            }
      static struct pipe_h265_sps *seq_parameter_set_id(vid_dec_PrivateType *priv,
         {
               if (id >= ARRAY_SIZE(priv->codec_data.h265.sps))
               }
      static void seq_parameter_set(vid_dec_PrivateType *priv, struct vl_rbsp *rbsp)
   {
      struct pipe_h265_sps *sps;
   int sps_max_sub_layers_minus1;
            /* sps_video_parameter_set_id */
            /* sps_max_sub_layers_minus1 */
                     /* sps_temporal_id_nesting_flag */
            priv->codec_data.h265.level_idc =
            sps = seq_parameter_set_id(priv, rbsp);
   if (!sps)
                              if (sps->chroma_format_idc == 3)
            priv->codec_data.h265.pic_width_in_luma_samples =
            priv->codec_data.h265.pic_height_in_luma_samples =
            /* conformance_window_flag */
   if (vl_rbsp_u(rbsp, 1)) {
      /* conf_win_left_offset */
   vl_rbsp_ue(rbsp);
   /* conf_win_right_offset */
   vl_rbsp_ue(rbsp);
   /* conf_win_top_offset */
   vl_rbsp_ue(rbsp);
   /* conf_win_bottom_offset */
               sps->bit_depth_luma_minus8 = vl_rbsp_ue(rbsp);
   sps->bit_depth_chroma_minus8 = vl_rbsp_ue(rbsp);
            /* sps_sub_layer_ordering_info_present_flag */
   i  = vl_rbsp_u(rbsp, 1) ? 0 : sps_max_sub_layers_minus1;
   for (; i <= sps_max_sub_layers_minus1; ++i) {
      sps->sps_max_dec_pic_buffering_minus1 = vl_rbsp_ue(rbsp);
   /* sps_max_num_reorder_pics */
   vl_rbsp_ue(rbsp);
   /* sps_max_latency_increase_plus */
               sps->log2_min_luma_coding_block_size_minus3 = vl_rbsp_ue(rbsp);
   sps->log2_diff_max_min_luma_coding_block_size = vl_rbsp_ue(rbsp);
   sps->log2_min_transform_block_size_minus2 = vl_rbsp_ue(rbsp);
   sps->log2_diff_max_min_transform_block_size = vl_rbsp_ue(rbsp);
   sps->max_transform_hierarchy_depth_inter = vl_rbsp_ue(rbsp);
            sps->scaling_list_enabled_flag = vl_rbsp_u(rbsp, 1);
   if (sps->scaling_list_enabled_flag)
      /* sps_scaling_list_data_present_flag */
   if (vl_rbsp_u(rbsp, 1))
         sps->amp_enabled_flag = vl_rbsp_u(rbsp, 1);
   sps->sample_adaptive_offset_enabled_flag = vl_rbsp_u(rbsp, 1);
   sps->pcm_enabled_flag = vl_rbsp_u(rbsp, 1);
   if (sps->pcm_enabled_flag) {
      sps->pcm_sample_bit_depth_luma_minus1 = vl_rbsp_u(rbsp, 4);
   sps->pcm_sample_bit_depth_chroma_minus1 = vl_rbsp_u(rbsp, 4);
   sps->log2_min_pcm_luma_coding_block_size_minus3 = vl_rbsp_ue(rbsp);
   sps->log2_diff_max_min_pcm_luma_coding_block_size = vl_rbsp_ue(rbsp);
                        for (i = 0; i < sps->num_short_term_ref_pic_sets; ++i) {
               rps = (struct ref_pic_set *)
                     sps->long_term_ref_pics_present_flag = vl_rbsp_u(rbsp, 1);
   if (sps->long_term_ref_pics_present_flag) {
      sps->num_long_term_ref_pics_sps = vl_rbsp_ue(rbsp);
   for (i = 0; i < sps->num_long_term_ref_pics_sps; ++i) {
      /* lt_ref_pic_poc_lsb_sps */
   vl_rbsp_u(rbsp, sps->log2_max_pic_order_cnt_lsb_minus4 + 4);
   /* used_by_curr_pic_lt_sps_flag */
                  sps->sps_temporal_mvp_enabled_flag = vl_rbsp_u(rbsp, 1);
      }
      static struct pipe_h265_pps *pic_parameter_set_id(vid_dec_PrivateType *priv,
         {
               if (id >= ARRAY_SIZE(priv->codec_data.h265.pps))
               }
      static void picture_parameter_set(vid_dec_PrivateType *priv,
         {
      struct pipe_h265_sps *sps;
   struct pipe_h265_pps *pps;
            pps = pic_parameter_set_id(priv, rbsp);
   if (!pps)
            memset(pps, 0, sizeof(*pps));
   sps = pps->sps = seq_parameter_set_id(priv, rbsp);
   if (!sps)
            pps->dependent_slice_segments_enabled_flag = vl_rbsp_u(rbsp, 1);
   pps->output_flag_present_flag = vl_rbsp_u(rbsp, 1);
   pps->num_extra_slice_header_bits = vl_rbsp_u(rbsp, 3);
   pps->sign_data_hiding_enabled_flag = vl_rbsp_u(rbsp, 1);
            pps->num_ref_idx_l0_default_active_minus1 = vl_rbsp_ue(rbsp);
   pps->num_ref_idx_l1_default_active_minus1 = vl_rbsp_ue(rbsp);
   pps->init_qp_minus26 = vl_rbsp_se(rbsp);
   pps->constrained_intra_pred_flag = vl_rbsp_u(rbsp, 1);
            pps->cu_qp_delta_enabled_flag = vl_rbsp_u(rbsp, 1);
   if (pps->cu_qp_delta_enabled_flag)
            pps->pps_cb_qp_offset = vl_rbsp_se(rbsp);
   pps->pps_cr_qp_offset = vl_rbsp_se(rbsp);
            pps->weighted_pred_flag = vl_rbsp_u(rbsp, 1);
            pps->transquant_bypass_enabled_flag = vl_rbsp_u(rbsp, 1);
   pps->tiles_enabled_flag = vl_rbsp_u(rbsp, 1);
            if (pps->tiles_enabled_flag) {
      pps->num_tile_columns_minus1 = vl_rbsp_ue(rbsp);
            pps->uniform_spacing_flag = vl_rbsp_u(rbsp, 1);
   if (!pps->uniform_spacing_flag) {
                     for (i = 0; i < pps->num_tile_rows_minus1; ++i)
               if (!pps->num_tile_columns_minus1 || !pps->num_tile_rows_minus1)
                        pps->deblocking_filter_control_present_flag = vl_rbsp_u(rbsp, 1);
   if (pps->deblocking_filter_control_present_flag) {
      pps->deblocking_filter_override_enabled_flag = vl_rbsp_u(rbsp, 1);
   pps->pps_deblocking_filter_disabled_flag = vl_rbsp_u(rbsp, 1);
   if (!pps->pps_deblocking_filter_disabled_flag) {
      pps->pps_beta_offset_div2 = vl_rbsp_se(rbsp);
                  if (vl_vlc_bits_left(&rbsp->nal) == 0)
            /* pps_scaling_list_data_present_flag */
   if (vl_rbsp_u(rbsp, 1))
            pps->lists_modification_present_flag = vl_rbsp_u(rbsp, 1);
   pps->log2_parallel_merge_level_minus2 = vl_rbsp_ue(rbsp);
      }
      static void vid_dec_h265_BeginFrame(vid_dec_PrivateType *priv)
   {
      if (priv->frame_started)
            if (!priv->codec) {
      struct pipe_video_codec templat = {};
   omx_base_video_PortType *port = (omx_base_video_PortType *)
            templat.profile = priv->profile;
   templat.entrypoint = PIPE_VIDEO_ENTRYPOINT_BITSTREAM;
   templat.chroma_format = PIPE_VIDEO_CHROMA_FORMAT_420;
   templat.expect_chunked_decode = false;
   templat.width = priv->codec_data.h265.pic_width_in_luma_samples;
   templat.height = priv->codec_data.h265.pic_height_in_luma_samples;
   templat.level =  priv->codec_data.h265.level_idc;
            /* disable transcode tunnel if video size is different from coded size */
   if (priv->codec_data.h265.pic_width_in_luma_samples !=
      port->sPortParam.format.video.nFrameWidth ||
   priv->codec_data.h265.pic_height_in_luma_samples !=
   port->sPortParam.format.video.nFrameHeight)
                     if (priv->first_buf_in_frame)
                  priv->codec->begin_frame(priv->codec, priv->target, &priv->picture.base);
      }
      static struct pipe_video_buffer *vid_dec_h265_Flush(vid_dec_PrivateType *priv,
         {
      struct dpb_list *entry, *result = NULL;
            /* search for the lowest poc and break on zeros */
               if (result && entry->poc == 0)
            if (!result || entry->poc < result->poc)
               if (!result)
            buf = result->buffer;
   if (timestamp)
            --priv->codec_data.h265.dpb_num;
   list_del(&result->list);
               }
      static void vid_dec_h265_EndFrame(vid_dec_PrivateType *priv)
   {
      struct dpb_list *entry = NULL;
   struct pipe_video_buffer *tmp;
   struct ref_pic_set *rps;
   int i;
            if (!priv->frame_started)
            priv->picture.h265.NumPocStCurrBefore = 0;
   priv->picture.h265.NumPocStCurrAfter = 0;
   memset(priv->picture.h265.RefPicSetStCurrBefore, 0, 8);
   memset(priv->picture.h265.RefPicSetStCurrAfter, 0, 8);
   for (i = 0; i < MAX_NUM_REF_PICS; ++i) {
      priv->picture.h265.ref[i] = NULL;
                        if (rps) {
               priv->picture.h265.NumDeltaPocsOfRefRpsIdx = rps->num_delta_poc;
   for (i = 0; i < rps->num_pics; ++i) {
                     LIST_FOR_EACH_ENTRY(entry, &priv->codec_data.h265.dpb_list, list) {
      if (entry->poc == priv->picture.h265.PicOrderCntVal[i]) {
                     if (rps->used[i]) {
      if (i < rps->num_neg_pics) {
      priv->picture.h265.NumPocStCurrBefore++;
      } else {
      priv->picture.h265.NumPocStCurrAfter++;
                        priv->codec->end_frame(priv->codec, priv->target, &priv->picture.base);
            /* add the decoded picture to the dpb list */
   entry = CALLOC_STRUCT(dpb_list);
   if (!entry)
            priv->first_buf_in_frame = true;
   entry->buffer = priv->target;
   entry->timestamp = priv->timestamp;
            list_addtail(&entry->list, &priv->codec_data.h265.dpb_list);
   ++priv->codec_data.h265.dpb_num;
            if (priv->codec_data.h265.dpb_num <= DPB_MAX_SIZE)
            tmp = priv->in_buffers[0]->pInputPortPrivate;
   priv->in_buffers[0]->pInputPortPrivate = vid_dec_h265_Flush(priv, &timestamp);
   priv->in_buffers[0]->nTimeStamp = timestamp;
   priv->target = tmp;
   priv->frame_finished = priv->in_buffers[0]->pInputPortPrivate != NULL;
   if (priv->frame_finished &&
      (priv->in_buffers[0]->nFlags & OMX_BUFFERFLAG_EOS))
   }
      static void slice_header(vid_dec_PrivateType *priv, struct vl_rbsp *rbsp,
         {
      struct pipe_h265_pps *pps;
   struct pipe_h265_sps *sps;
   bool first_slice_segment_in_pic_flag;
   bool dependent_slice_segment_flag = false;
   struct ref_pic_set *rps;
   unsigned poc_lsb, poc_msb, slice_prev_poc;
   unsigned max_poc_lsb, prev_poc_lsb, prev_poc_msb;
   unsigned num_st_rps;
            if (priv->picture.h265.IDRPicFlag != is_idr_picture(nal_unit_type))
                              if (is_rap_picture(nal_unit_type))
      /* no_output_of_prior_pics_flag */
         pps = pic_parameter_set_id(priv, rbsp);
   if (!pps)
            sps = pps->sps;
   if (!sps)
            if (pps != priv->picture.h265.pps)
                     if (priv->picture.h265.RAPPicFlag != is_rap_picture(nal_unit_type))
         priv->picture.h265.RAPPicFlag = is_rap_picture(nal_unit_type);
   priv->picture.h265.IntraPicFlag = is_rap_picture(nal_unit_type);
            if (priv->picture.h265.CurrRpsIdx != num_st_rps)
                  if (!first_slice_segment_in_pic_flag) {
      int size, num;
            if (pps->dependent_slice_segments_enabled_flag)
            size = 1 << (sps->log2_min_luma_coding_block_size_minus3 + 3 +
            num = ((sps->pic_width_in_luma_samples + size - 1) / size) *
            while (num > (1 << bits_slice_segment_address))
            /* slice_segment_address */
               if (dependent_slice_segment_flag)
            for (i = 0; i < pps->num_extra_slice_header_bits; ++i)
      /* slice_reserved_flag */
         /* slice_type */
            if (pps->output_flag_present_flag)
      /* pic output flag */
         if (sps->separate_colour_plane_flag)
      /* colour_plane_id */
         if (is_idr_picture(nal_unit_type)) {
      set_poc(priv, nal_unit_type, 0);
               /* slice_pic_order_cnt_lsb */
   poc_lsb =
            slice_prev_poc = (int)priv->codec_data.h265.slice_prev_poc;
            prev_poc_lsb = slice_prev_poc & (max_poc_lsb - 1);
            if ((poc_lsb < prev_poc_lsb) &&
      ((prev_poc_lsb - poc_lsb ) >= (max_poc_lsb / 2)))
         else if ((poc_lsb > prev_poc_lsb ) &&
                  else
            if (is_bla_picture(nal_unit_type))
            if (get_poc(priv) != poc_msb + poc_lsb)
                     /* short_term_ref_pic_set_sps_flag */
   if (!vl_rbsp_u(rbsp, 1)) {
      rps = (struct ref_pic_set *)
               } else if (num_st_rps > 1) {
      int num_bits = 0;
            while ((1 << num_bits) < num_st_rps)
            if (num_bits > 0)
      /* short_term_ref_pic_set_idx */
      else
            rps = (struct ref_pic_set *)
      } else
      rps = (struct ref_pic_set *)
         if (is_bla_picture(nal_unit_type)) {
      rps->num_neg_pics = 0;
   rps->num_pos_pics = 0;
                           }
      static void vid_dec_h265_Decode(vid_dec_PrivateType *priv,
               {
      unsigned nal_unit_type;
   unsigned nuh_layer_id;
            if (!vl_vlc_search_byte(vlc, vl_vlc_bits_left(vlc) - min_bits_left, 0x00))
            if (vl_vlc_peekbits(vlc, 24) != 0x000001) {
      vl_vlc_eatbits(vlc, 8);
               if (priv->slice) {
               priv->codec->decode_bitstream(priv->codec, priv->target,
                                    /* forbidden_zero_bit */
            if (vl_vlc_valid_bits(vlc) < 15)
                     /* nuh_layer_id */
            /* nuh_temporal_id_plus1 */
   nuh_temporal_id_plus1 = vl_vlc_get_uimsbf(vlc, 3);
            if (!is_slice_picture(nal_unit_type))
            if (nal_unit_type == NAL_UNIT_TYPE_SPS) {
               vl_rbsp_init(&rbsp, vlc, ~0, /* emulation_bytes */ true);
         } else if (nal_unit_type == NAL_UNIT_TYPE_PPS) {
               vl_rbsp_init(&rbsp, vlc, ~0, /* emulation_bytes */ true);
         } else if (is_slice_picture(nal_unit_type)) {
      unsigned bits = vl_vlc_valid_bits(vlc);
   unsigned bytes = bits / 8 + 5;
   struct vl_rbsp rbsp;
   uint8_t buf[9];
   const void *ptr = buf;
            buf[0] = 0x0;
   buf[1] = 0x0;
   buf[2] = 0x1;
   buf[3] = nal_unit_type << 1 | nuh_layer_id >> 5;
   buf[4] = nuh_layer_id << 3 | nuh_temporal_id_plus1;
   for (i = 5; i < bytes; ++i)
            priv->bytes_left = (vl_vlc_bits_left(vlc) - bits) / 8;
            vl_rbsp_init(&rbsp, vlc, 128, /* emulation_bytes */ true);
                     priv->codec->decode_bitstream(priv->codec, priv->target,
                     /* resync to byte boundary */
      }
      void vid_dec_h265_Init(vid_dec_PrivateType *priv)
   {
               list_inithead(&priv->codec_data.h265.dpb_list);
   priv->codec_data.h265.ref_pic_set_list = (struct ref_pic_set *)
            priv->Decode = vid_dec_h265_Decode;
   priv->EndFrame = vid_dec_h265_EndFrame;
   priv->Flush = vid_dec_h265_Flush;
      }
