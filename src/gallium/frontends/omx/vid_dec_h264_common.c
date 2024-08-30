   /**************************************************************************
   *
   * Copyright 2013 Advanced Micro Devices, Inc.
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
      #if ENABLE_ST_OMX_TIZONIA
   #include <tizkernel.h>
   #endif
      #include "util/u_memory.h"
      #include "vid_dec_h264_common.h"
      static void vid_dec_h264_BeginFrame(vid_dec_PrivateType *priv)
   {
               if (priv->frame_started)
            if (!priv->codec) {
      struct pipe_video_codec templat = {};
   templat.profile = priv->profile;
   templat.entrypoint = PIPE_VIDEO_ENTRYPOINT_BITSTREAM;
   templat.chroma_format = PIPE_VIDEO_CHROMA_FORMAT_420;
   templat.max_references = priv->picture.h264.num_ref_frames;
   #if ENABLE_ST_OMX_BELLAGIO
         omx_base_video_PortType *port;
   port = (omx_base_video_PortType *)priv->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
   templat.width = port->sPortParam.format.video.nFrameWidth;
   #else
         templat.width = priv->out_port_def_.format.video.nFrameWidth;
   #endif
                                       if (priv->first_buf_in_frame)
                           priv->picture.h264.slice_count = 0;
   priv->codec->begin_frame(priv->codec, priv->target, &priv->picture.base);
      }
      struct pipe_video_buffer *vid_dec_h264_Flush(vid_dec_PrivateType *priv,
         {
      struct dpb_list *entry, *result = NULL;
            /* search for the lowest poc and break on zeros */
               if (result && entry->poc == 0)
            if (!result || entry->poc < result->poc)
               if (!result)
            buf = result->buffer;
   if (timestamp)
            --priv->codec_data.h264.dpb_num;
   list_del(&result->list);
               }
      void vid_dec_h264_EndFrame(vid_dec_PrivateType *priv)
   {
      struct dpb_list *entry;
   struct pipe_video_buffer *tmp;
   bool top_field_first;
            if (!priv->frame_started)
            priv->codec->end_frame(priv->codec, priv->target, &priv->picture.base);
            // TODO: implement frame number handling
   priv->picture.h264.frame_num_list[0] = priv->picture.h264.frame_num;
   priv->picture.h264.field_order_cnt_list[0][0] = priv->picture.h264.frame_num;
                     if (priv->picture.h264.field_pic_flag && priv->picture.h264.bottom_field_flag != top_field_first)
            /* add the decoded picture to the dpb list */
   entry = CALLOC_STRUCT(dpb_list);
   if (!entry)
            priv->first_buf_in_frame = true;
   entry->buffer = priv->target;
   entry->timestamp = priv->timestamp;
   entry->poc = MIN2(priv->picture.h264.field_order_cnt[0], priv->picture.h264.field_order_cnt[1]);
   list_addtail(&entry->list, &priv->codec_data.h264.dpb_list);
   ++priv->codec_data.h264.dpb_num;
   priv->target = NULL;
            if (priv->codec_data.h264.dpb_num <= DPB_MAX_SIZE)
            tmp = priv->in_buffers[0]->pInputPortPrivate;
   priv->in_buffers[0]->pInputPortPrivate = vid_dec_h264_Flush(priv, &timestamp);
   priv->in_buffers[0]->nTimeStamp = timestamp;
   priv->target = tmp;
      }
      static void vui_parameters(struct vl_rbsp *rbsp)
   {
         }
      static void scaling_list(struct vl_rbsp *rbsp, uint8_t *scalingList, unsigned sizeOfScalingList,
         {
      unsigned lastScale = 8, nextScale = 8;
   const int *list;
            /* (pic|seq)_scaling_list_present_flag[i] */
   if (!vl_rbsp_u(rbsp, 1)) {
      if (fallbackList)
                     list = (sizeOfScalingList == 16) ? vl_zscan_normal_16 : vl_zscan_normal;
               if (nextScale != 0) {
      signed delta_scale = vl_rbsp_se(rbsp);
   nextScale = (lastScale + delta_scale + 256) % 256;
   if (i == 0 && nextScale == 0) {
      memcpy(scalingList, defaultList, sizeOfScalingList);
         }
   scalingList[list[i]] = nextScale == 0 ? lastScale : nextScale;
         }
      static struct pipe_h264_sps *seq_parameter_set_id(vid_dec_PrivateType *priv, struct vl_rbsp *rbsp)
   {
      unsigned id = vl_rbsp_ue(rbsp);
   if (id >= ARRAY_SIZE(priv->codec_data.h264.sps))
               }
      static void seq_parameter_set(vid_dec_PrivateType *priv, struct vl_rbsp *rbsp)
   {
      struct pipe_h264_sps *sps;
   unsigned profile_idc, level_idc;
            /* Sequence parameter set */
            /* constraint_set0_flag */
            /* constraint_set1_flag */
            /* constraint_set2_flag */
            /* constraint_set3_flag */
            /* constraint_set4_flag */
            /* constraint_set5_flag */
            /* reserved_zero_2bits */
            /* level_idc */
            sps = seq_parameter_set_id(priv, rbsp);
   if (!sps)
            memset(sps, 0, sizeof(*sps));
   memset(sps->ScalingList4x4, 16, sizeof(sps->ScalingList4x4));
                     if (profile_idc == 100 || profile_idc == 110 || profile_idc == 122 || profile_idc == 244 ||
      profile_idc == 44 || profile_idc == 83 || profile_idc == 86 || profile_idc == 118 ||
                     if (sps->chroma_format_idc == 3)
                              /* qpprime_y_zero_transform_bypass_flag */
            sps->seq_scaling_matrix_present_flag = vl_rbsp_u(rbsp, 1);
               scaling_list(rbsp, sps->ScalingList4x4[0], 16, Default_4x4_Intra, Default_4x4_Intra);
   scaling_list(rbsp, sps->ScalingList4x4[1], 16, Default_4x4_Intra, sps->ScalingList4x4[0]);
   scaling_list(rbsp, sps->ScalingList4x4[2], 16, Default_4x4_Intra, sps->ScalingList4x4[1]);
   scaling_list(rbsp, sps->ScalingList4x4[3], 16, Default_4x4_Inter, Default_4x4_Inter);
                  scaling_list(rbsp, sps->ScalingList8x8[0], 64, Default_8x8_Intra, Default_8x8_Intra);
   scaling_list(rbsp, sps->ScalingList8x8[1], 64, Default_8x8_Inter, Default_8x8_Inter);
   if (sps->chroma_format_idc == 3) {
      scaling_list(rbsp, sps->ScalingList8x8[2], 64, Default_8x8_Intra, sps->ScalingList8x8[0]);
   scaling_list(rbsp, sps->ScalingList8x8[3], 64, Default_8x8_Inter, sps->ScalingList8x8[1]);
   scaling_list(rbsp, sps->ScalingList8x8[4], 64, Default_8x8_Intra, sps->ScalingList8x8[2]);
            } else if (profile_idc == 183)
         else
                              if (sps->pic_order_cnt_type == 0)
         else if (sps->pic_order_cnt_type == 1) {
                                          for (i = 0; i < sps->num_ref_frames_in_pic_order_cnt_cycle; ++i)
                        /* gaps_in_frame_num_value_allowed_flag */
            /* pic_width_in_mbs_minus1 */
   ASSERTED int pic_width_in_samplesl = (vl_rbsp_ue(rbsp) + 1) * 16;
            /* pic_height_in_map_units_minus1 */
   ASSERTED int pic_height_in_map_units = vl_rbsp_ue(rbsp) + 1;
            sps->frame_mbs_only_flag = vl_rbsp_u(rbsp, 1);
   if (!sps->frame_mbs_only_flag)
                        #if ENABLE_ST_OMX_TIZONIA
               int frame_height_in_mbs = (2 - sps->frame_mbs_only_flag) * pic_height_in_map_units;
   int pic_height_in_mbs = frame_height_in_mbs / ( 1 + priv->picture.h264.field_pic_flag );
   int pic_height_in_samplesl = pic_height_in_mbs * 16;
               /* frame_cropping_flag */
   if (vl_rbsp_u(rbsp, 1)) {
      unsigned frame_crop_left_offset = vl_rbsp_ue(rbsp);
   unsigned frame_crop_right_offset = vl_rbsp_ue(rbsp);
   unsigned frame_crop_top_offset = vl_rbsp_ue(rbsp);
            priv->stream_info.width -= (frame_crop_left_offset + frame_crop_right_offset) * 2;
         #else
      /* frame_cropping_flag */
   if (vl_rbsp_u(rbsp, 1)) {
      /* frame_crop_left_offset */
            /* frame_crop_right_offset */
            /* frame_crop_top_offset */
            /* frame_crop_bottom_offset */
         #endif
         /* vui_parameters_present_flag */
   if (vl_rbsp_u(rbsp, 1))
      }
      static struct pipe_h264_pps *pic_parameter_set_id(vid_dec_PrivateType *priv, struct vl_rbsp *rbsp)
   {
      unsigned id = vl_rbsp_ue(rbsp);
   if (id >= ARRAY_SIZE(priv->codec_data.h264.pps))
               }
      static void picture_parameter_set(vid_dec_PrivateType *priv, struct vl_rbsp *rbsp)
   {
      struct pipe_h264_sps *sps;
   struct pipe_h264_pps *pps;
            pps = pic_parameter_set_id(priv, rbsp);
   if (!pps)
                     sps = pps->sps = seq_parameter_set_id(priv, rbsp);
   if (!sps)
            memcpy(pps->ScalingList4x4, sps->ScalingList4x4, sizeof(pps->ScalingList4x4));
                              pps->num_slice_groups_minus1 = vl_rbsp_ue(rbsp);
   if (pps->num_slice_groups_minus1 > 0) {
                           for (i = 0; i <= pps->num_slice_groups_minus1; ++i)
                           for (i = 0; i <= pps->num_slice_groups_minus1; ++i) {
                     /* bottom_right[i] */
                                                                           for (i = 0; i <= pic_size_in_map_units_minus1; ++i)
      /* slice_group_id[i] */
                                                                                                         if (vl_rbsp_more_data(rbsp)) {
               /* pic_scaling_matrix_present_flag */
               scaling_list(rbsp, pps->ScalingList4x4[0], 16, Default_4x4_Intra,
         scaling_list(rbsp, pps->ScalingList4x4[1], 16, Default_4x4_Intra, pps->ScalingList4x4[0]);
   scaling_list(rbsp, pps->ScalingList4x4[2], 16, Default_4x4_Intra, pps->ScalingList4x4[1]);
   scaling_list(rbsp, pps->ScalingList4x4[3], 16, Default_4x4_Inter,
                        if (pps->transform_8x8_mode_flag) {
      scaling_list(rbsp, pps->ScalingList8x8[0], 64, Default_8x8_Intra,
         scaling_list(rbsp, pps->ScalingList8x8[1], 64, Default_8x8_Inter,
         if (sps->chroma_format_idc == 3) {
      scaling_list(rbsp, pps->ScalingList8x8[2], 64, Default_8x8_Intra, pps->ScalingList8x8[0]);
   scaling_list(rbsp, pps->ScalingList8x8[3], 64, Default_8x8_Inter, pps->ScalingList8x8[1]);
   scaling_list(rbsp, pps->ScalingList8x8[4], 64, Default_8x8_Intra, pps->ScalingList8x8[2]);
                           }
      static void ref_pic_list_mvc_modification(vid_dec_PrivateType *priv, struct vl_rbsp *rbsp)
   {
      // TODO
      }
      static void ref_pic_list_modification(vid_dec_PrivateType *priv, struct vl_rbsp *rbsp,
         {
               if (slice_type != 2 && slice_type != 4) {
      /* ref_pic_list_modification_flag_l0 */
   if (vl_rbsp_u(rbsp, 1)) {
      do {
      modification_of_pic_nums_idc = vl_rbsp_ue(rbsp);
   if (modification_of_pic_nums_idc == 0 ||
      modification_of_pic_nums_idc == 1)
   /* abs_diff_pic_num_minus1 */
      else if (modification_of_pic_nums_idc == 2)
      /* long_term_pic_num */
                  if (slice_type == 1) {
      /* ref_pic_list_modification_flag_l1 */
   if (vl_rbsp_u(rbsp, 1)) {
      do {
      modification_of_pic_nums_idc = vl_rbsp_ue(rbsp);
   if (modification_of_pic_nums_idc == 0 ||
      modification_of_pic_nums_idc == 1)
   /* abs_diff_pic_num_minus1 */
      else if (modification_of_pic_nums_idc == 2)
      /* long_term_pic_num */
            }
      static void pred_weight_table(vid_dec_PrivateType *priv, struct vl_rbsp *rbsp,
         {
      unsigned ChromaArrayType = sps->separate_colour_plane_flag ? 0 : sps->chroma_format_idc;
            /* luma_log2_weight_denom */
            if (ChromaArrayType != 0)
      /* chroma_log2_weight_denom */
         for (i = 0; i <= priv->picture.h264.num_ref_idx_l0_active_minus1; ++i) {
      /* luma_weight_l0_flag */
   if (vl_rbsp_u(rbsp, 1)) {
      /* luma_weight_l0[i] */
   vl_rbsp_se(rbsp);
   /* luma_offset_l0[i] */
      }
   if (ChromaArrayType != 0) {
      /* chroma_weight_l0_flag */
   if (vl_rbsp_u(rbsp, 1)) {
      for (j = 0; j < 2; ++j) {
      /* chroma_weight_l0[i][j] */
   vl_rbsp_se(rbsp);
   /* chroma_offset_l0[i][j] */
                        if (slice_type == 1) {
      for (i = 0; i <= priv->picture.h264.num_ref_idx_l1_active_minus1; ++i) {
      /* luma_weight_l1_flag */
   if (vl_rbsp_u(rbsp, 1)) {
      /* luma_weight_l1[i] */
   vl_rbsp_se(rbsp);
   /* luma_offset_l1[i] */
      }
   if (ChromaArrayType != 0) {
      /* chroma_weight_l1_flag */
   if (vl_rbsp_u(rbsp, 1)) {
      for (j = 0; j < 2; ++j) {
      /* chroma_weight_l1[i][j] */
   vl_rbsp_se(rbsp);
   /* chroma_offset_l1[i][j] */
                     }
      static void dec_ref_pic_marking(vid_dec_PrivateType *priv, struct vl_rbsp *rbsp,
         {
               if (IdrPicFlag) {
      /* no_output_of_prior_pics_flag */
   vl_rbsp_u(rbsp, 1);
   /* long_term_reference_flag */
      } else {
      /* adaptive_ref_pic_marking_mode_flag */
   if (vl_rbsp_u(rbsp, 1)) {
                        if (memory_management_control_operation == 1 ||
                        if (memory_management_control_operation == 2)
                  if (memory_management_control_operation == 3 ||
                        if (memory_management_control_operation == 4)
      /* max_long_term_frame_idx_plus1 */
            }
      static void slice_header(vid_dec_PrivateType *priv, struct vl_rbsp *rbsp,
         {
      enum pipe_h264_slice_type slice_type;
   struct pipe_h264_pps *pps;
   struct pipe_h264_sps *sps;
   unsigned frame_num, prevFrameNum;
            if (IdrPicFlag != priv->codec_data.h264.IdrPicFlag)
                     /* first_mb_in_slice */
                     /* get picture parameter set */
   pps = pic_parameter_set_id(priv, rbsp);
   if (!pps)
            /* get sequence parameter set */
   sps = pps->sps;
   if (!sps)
            if (pps != priv->picture.h264.pps)
                     if (sps->separate_colour_plane_flag == 1 )
      /* colour_plane_id */
         /* frame number handling */
            if (frame_num != priv->picture.h264.frame_num)
            prevFrameNum = priv->picture.h264.frame_num;
            priv->picture.h264.field_pic_flag = 0;
            if (!sps->frame_mbs_only_flag) {
               if (!field_pic_flag && field_pic_flag != priv->picture.h264.field_pic_flag)
                     if (priv->picture.h264.field_pic_flag) {
                                             if (IdrPicFlag) {
      /* set idr_pic_id */
            if (idr_pic_id != priv->codec_data.h264.idr_pic_id)
                        if (sps->pic_order_cnt_type == 0) {
      /* pic_order_cnt_lsb */
   unsigned log2_max_pic_order_cnt_lsb = sps->log2_max_pic_order_cnt_lsb_minus4 + 4;
   unsigned max_pic_order_cnt_lsb = 1 << log2_max_pic_order_cnt_lsb;
   int pic_order_cnt_lsb = vl_rbsp_u(rbsp, log2_max_pic_order_cnt_lsb);
            if (pic_order_cnt_lsb != priv->codec_data.h264.pic_order_cnt_lsb)
            if (IdrPicFlag) {
      priv->codec_data.h264.pic_order_cnt_msb = 0;
               if ((pic_order_cnt_lsb < priv->codec_data.h264.pic_order_cnt_lsb) &&
                  else if ((pic_order_cnt_lsb > priv->codec_data.h264.pic_order_cnt_lsb) &&
                  else
            priv->codec_data.h264.pic_order_cnt_msb = pic_order_cnt_msb;
            if (pps->bottom_field_pic_order_in_frame_present_flag && !priv->picture.h264.field_pic_flag) {
                                                if (!priv->picture.h264.field_pic_flag) {
      priv->picture.h264.field_order_cnt[0] = pic_order_cnt_msb + pic_order_cnt_lsb;
   priv->picture.h264.field_order_cnt[1] = priv->picture.h264.field_order_cnt [0] +
      } else if (!priv->picture.h264.bottom_field_flag)
         else
         } else if (sps->pic_order_cnt_type == 1) {
      /* delta_pic_order_cnt[0] */
   unsigned MaxFrameNum = 1 << (sps->log2_max_frame_num_minus4 + 4);
            if (!sps->delta_pic_order_always_zero_flag) {
                                                if (pps->bottom_field_pic_order_in_frame_present_flag && !priv->picture.h264.field_pic_flag) {
                                                   if (IdrPicFlag)
         else if (prevFrameNum > frame_num)
         else
                     if (sps->num_ref_frames_in_pic_order_cnt_cycle != 0)
         else
            if (nal_ref_idc == 0 && absFrameNum > 0)
            if (absFrameNum > 0) {
      unsigned picOrderCntCycleCnt = (absFrameNum - 1) / sps->num_ref_frames_in_pic_order_cnt_cycle;
   unsigned frameNumInPicOrderCntCycle = (absFrameNum - 1) % sps->num_ref_frames_in_pic_order_cnt_cycle;
                                 expectedPicOrderCnt = picOrderCntCycleCnt * ExpectedDeltaPerPicOrderCntCycle;
               } else
            if (nal_ref_idc == 0)
            if (!priv->picture.h264.field_pic_flag) {
      priv->picture.h264.field_order_cnt[0] = expectedPicOrderCnt + priv->codec_data.h264.delta_pic_order_cnt[0];
               } else if (!priv->picture.h264.bottom_field_flag)
         else
               } else if (sps->pic_order_cnt_type == 2) {
      unsigned MaxFrameNum = 1 << (sps->log2_max_frame_num_minus4 + 4);
            if (IdrPicFlag)
         else if (prevFrameNum > frame_num)
         else
                     if (IdrPicFlag)
         else if (nal_ref_idc == 0)
         else
            if (!priv->picture.h264.field_pic_flag) {
                  } else if (!priv->picture.h264.bottom_field_flag)
         else
               if (pps->redundant_pic_cnt_present_flag)
      /* redundant_pic_cnt */
         if (slice_type == PIPE_H264_SLICE_TYPE_B)
      /* direct_spatial_mv_pred_flag */
         priv->picture.h264.num_ref_idx_l0_active_minus1 = pps->num_ref_idx_l0_default_active_minus1;
            if (slice_type == PIPE_H264_SLICE_TYPE_P ||
      slice_type == PIPE_H264_SLICE_TYPE_SP ||
            /* num_ref_idx_active_override_flag */
   if (vl_rbsp_u(rbsp, 1)) {
               if (slice_type == PIPE_H264_SLICE_TYPE_B)
                  if (nal_unit_type == 20 || nal_unit_type == 21)
         else
            if ((pps->weighted_pred_flag && (slice_type == PIPE_H264_SLICE_TYPE_P || slice_type == PIPE_H264_SLICE_TYPE_SP)) ||
      (pps->weighted_bipred_idc == 1 && slice_type == PIPE_H264_SLICE_TYPE_B))
         if (nal_ref_idc != 0)
            if (pps->entropy_coding_mode_flag && slice_type != PIPE_H264_SLICE_TYPE_I && slice_type != PIPE_H264_SLICE_TYPE_SI)
      /* cabac_init_idc */
         /* slice_qp_delta */
            if (slice_type == PIPE_H264_SLICE_TYPE_SP || slice_type == PIPE_H264_SLICE_TYPE_SI) {
      if (slice_type == PIPE_H264_SLICE_TYPE_SP)
                  /*slice_qs_delta */
               if (pps->deblocking_filter_control_present_flag) {
               if (disable_deblocking_filter_idc != 1) {
                     /* slice_beta_offset_div2 */
                  if (pps->num_slice_groups_minus1 > 0 && pps->slice_group_map_type >= 3 && pps->slice_group_map_type <= 5)
      /* slice_group_change_cycle */
   }
      #if ENABLE_ST_OMX_TIZONIA
   static OMX_ERRORTYPE update_port_parameters(vid_dec_PrivateType* priv) {
      OMX_VIDEO_PORTDEFINITIONTYPE * p_def = NULL;   /* Output port info */
   h264d_stream_info_t * i_def = NULL; /* Info read from stream */
                     p_def = &(priv->out_port_def_.format.video);
            /* Handle dynamic resolution change */
   if ((p_def->nFrameWidth == i_def->width) && p_def->nFrameHeight == i_def->height)
            p_def->nFrameWidth = i_def->width;
   p_def->nFrameHeight = i_def->height;
   p_def->nStride = i_def->width;
            err = tiz_krn_SetParameter_internal(tiz_get_krn(handleOf(priv)), handleOf(priv),
         if (err == OMX_ErrorNone) {
               /* Set desired buffer size that will be used when allocating input buffers */
            /* Get a locally copy of port def. Useful for the early return above */
   tiz_check_omx(tiz_api_GetParameter(tiz_get_krn(handleOf(priv)), handleOf(priv),
            tiz_srv_issue_event((OMX_PTR) priv, OMX_EventPortSettingsChanged,
                              }
   #endif
      void vid_dec_h264_Decode(vid_dec_PrivateType *priv, struct vl_vlc *vlc, unsigned min_bits_left)
   {
               if (!vl_vlc_search_byte(vlc, vl_vlc_bits_left(vlc) - min_bits_left, 0x00))
            if (vl_vlc_peekbits(vlc, 24) != 0x000001) {
      vl_vlc_eatbits(vlc, 8);
               if (priv->slice) {
      unsigned bytes = priv->bytes_left - (vl_vlc_bits_left(vlc) / 8);
   ++priv->picture.h264.slice_count;
   priv->codec->decode_bitstream(priv->codec, priv->target, &priv->picture.base,
                              /* forbidden_zero_bit */
                     if (nal_ref_idc != priv->codec_data.h264.nal_ref_idc &&
      (nal_ref_idc * priv->codec_data.h264.nal_ref_idc) == 0)
                           if (nal_unit_type != 1 && nal_unit_type != 5)
            if (nal_unit_type == 7) {
      struct vl_rbsp rbsp;
   vl_rbsp_init(&rbsp, vlc, ~0, /* emulation_bytes */ true);
   #if ENABLE_ST_OMX_TIZONIA
         #endif
         } else if (nal_unit_type == 8) {
      struct vl_rbsp rbsp;
   vl_rbsp_init(&rbsp, vlc, ~0, /* emulation_bytes */ true);
         } else if (nal_unit_type == 1 || nal_unit_type == 5) {
      /* Coded slice of a non-IDR or IDR picture */
   unsigned bits = vl_vlc_valid_bits(vlc);
   unsigned bytes = bits / 8 + 4;
   struct vl_rbsp rbsp;
   uint8_t buf[8];
   const void *ptr = buf;
            buf[0] = 0x0;
   buf[1] = 0x0;
   buf[2] = 0x1;
   buf[3] = (nal_ref_idc << 5) | nal_unit_type;
   for (i = 4; i < bytes; ++i)
            priv->bytes_left = (vl_vlc_bits_left(vlc) - bits) / 8;
            vl_rbsp_init(&rbsp, vlc, 128, /* emulation_bytes */ true);
                     ++priv->picture.h264.slice_count;
   priv->codec->decode_bitstream(priv->codec, priv->target, &priv->picture.base,
               /* resync to byte boundary */
      }
      void vid_dec_FreeInputPortPrivate(OMX_BUFFERHEADERTYPE *buf)
   {
      struct pipe_video_buffer *vbuf = buf->pInputPortPrivate;
   if (!vbuf)
            vbuf->destroy(vbuf);
      }
      void vid_dec_FrameDecoded_common(vid_dec_PrivateType* priv, OMX_BUFFERHEADERTYPE* input,
         {
   #if ENABLE_ST_OMX_BELLAGIO
         #else
         #endif
                  #if ENABLE_ST_OMX_BELLAGIO
         #else
         #endif
         if (timestamp != OMX_VID_DEC_AVC_TIMESTAMP_INVALID)
               if (input->pInputPortPrivate) {
      if (output->pInputPortPrivate && !priv->disable_tunnel) {
               tmp = output->pOutputPortPrivate;
   vbuf = input->pInputPortPrivate;
   if (vbuf->interlaced) {
      /* re-allocate the progressive buffer */
      #if ENABLE_ST_OMX_BELLAGIO
               omx_base_video_PortType *port;
   #else
               #endif
         #if ENABLE_ST_OMX_BELLAGIO
               #else
               #endif
               templat.buffer_format = PIPE_FORMAT_NV12;
                  /* convert the interlaced to the progressive */
   src_rect.x0 = dst_rect.x0 = 0;
   src_rect.x1 = dst_rect.x1 = templat.width;
                  vl_compositor_yuv_deint_full(&priv->cstate, &priv->compositor,
                  /* set the progrssive buffer for next round */
   vbuf->destroy(vbuf);
      }
   output->pOutputPortPrivate = input->pInputPortPrivate;
      } else {
         }
   output->nFilledLen = output->nAllocLen;
               if (eos && input->pInputPortPrivate)
         else
      }
