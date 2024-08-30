   /*
   * Copyright © 2021 Red Hat
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
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "vk_video.h"
   #include "vk_util.h"
   #include "vk_log.h"
   #include "vk_alloc.h"
   #include "vk_device.h"
   #include "util/vl_rbsp.h"
      VkResult
   vk_video_session_init(struct vk_device *device,
               {
               vid->op = create_info->pVideoProfile->videoCodecOperation;
   vid->max_coded = create_info->maxCodedExtent;
   vid->picture_format = create_info->pictureFormat;
   vid->ref_format = create_info->referencePictureFormat;
   vid->max_dpb_slots = create_info->maxDpbSlots;
            switch (vid->op) {
   case VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR: {
      const struct VkVideoDecodeH264ProfileInfoKHR *h264_profile =
      vk_find_struct_const(create_info->pVideoProfile->pNext,
      vid->h264.profile_idc = h264_profile->stdProfileIdc;
      }
   case VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR: {
      const struct VkVideoDecodeH265ProfileInfoKHR *h265_profile =
      vk_find_struct_const(create_info->pVideoProfile->pNext,
      vid->h265.profile_idc = h265_profile->stdProfileIdc;
      }
   default:
                     }
      #define FIND(PARAMSET, SS, SET, ID)                                     \
      static PARAMSET *find_##SS##_##SET(const struct vk_video_session_parameters *params, uint32_t id) { \
      for (unsigned i = 0; i < params->SS.SET##_count; i++) {           \
      if (params->SS.SET[i].ID == id)                                \
      }                                                                 \
      }                                                                    \
         static void add_##SS##_##SET(struct vk_video_session_parameters *params, \
            PARAMSET *set = find_##SS##_##SET(params, new_set->ID);           \
   if (noreplace)                                                 \
                     } else                                                            \
      }                                                                    \
         static VkResult update_##SS##_##SET(struct vk_video_session_parameters *params, \
            if (params->SS.SET##_count + count >= params->SS.max_##SET##_count) \
         typed_memcpy(&params->SS.SET[params->SS.SET##_count], updates, count); \
   params->SS.SET##_count += count;                                  \
            FIND(StdVideoH264SequenceParameterSet, h264_dec, std_sps, seq_parameter_set_id)
   FIND(StdVideoH264PictureParameterSet, h264_dec, std_pps, pic_parameter_set_id)
   FIND(StdVideoH265VideoParameterSet, h265_dec, std_vps, vps_video_parameter_set_id)
   FIND(StdVideoH265SequenceParameterSet, h265_dec, std_sps, sps_seq_parameter_set_id)
   FIND(StdVideoH265PictureParameterSet, h265_dec, std_pps, pps_pic_parameter_set_id)
      static void
   init_add_h264_session_parameters(struct vk_video_session_parameters *params,
               {
               if (h264_add) {
      for (i = 0; i < h264_add->stdSPSCount; i++) {
            }
   if (templ) {
      for (i = 0; i < templ->h264_dec.std_sps_count; i++) {
                     if (h264_add) {
      for (i = 0; i < h264_add->stdPPSCount; i++) {
            }
   if (templ) {
      for (i = 0; i < templ->h264_dec.std_pps_count; i++) {
               }
      static void
   init_add_h265_session_parameters(struct vk_video_session_parameters *params,
               {
               if (h265_add) {
      for (i = 0; i < h265_add->stdVPSCount; i++) {
            }
   if (templ) {
      for (i = 0; i < templ->h265_dec.std_vps_count; i++) {
            }
   if (h265_add) {
      for (i = 0; i < h265_add->stdSPSCount; i++) {
            }
   if (templ) {
      for (i = 0; i < templ->h265_dec.std_sps_count; i++) {
                     if (h265_add) {
      for (i = 0; i < h265_add->stdPPSCount; i++) {
            }
   if (templ) {
      for (i = 0; i < templ->h265_dec.std_pps_count; i++) {
               }
      VkResult
   vk_video_session_parameters_init(struct vk_device *device,
                           {
      memset(params, 0, sizeof(*params));
                     switch (vid->op) {
   case VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR: {
      const struct VkVideoDecodeH264SessionParametersCreateInfoKHR *h264_create =
            params->h264_dec.max_std_sps_count = h264_create->maxStdSPSCount;
            uint32_t sps_size = params->h264_dec.max_std_sps_count * sizeof(StdVideoH264SequenceParameterSet);
            params->h264_dec.std_sps = vk_alloc(&device->alloc, sps_size, 8, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
   params->h264_dec.std_pps = vk_alloc(&device->alloc, pps_size, 8, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
   if (!params->h264_dec.std_sps || !params->h264_dec.std_pps) {
      vk_free(&device->alloc, params->h264_dec.std_sps);
   vk_free(&device->alloc, params->h264_dec.std_pps);
               init_add_h264_session_parameters(params, h264_create->pParametersAddInfo, templ);
      }
   case VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR: {
      const struct VkVideoDecodeH265SessionParametersCreateInfoKHR *h265_create =
            params->h265_dec.max_std_vps_count = h265_create->maxStdVPSCount;
   params->h265_dec.max_std_sps_count = h265_create->maxStdSPSCount;
            uint32_t vps_size = params->h265_dec.max_std_vps_count * sizeof(StdVideoH265VideoParameterSet);
   uint32_t sps_size = params->h265_dec.max_std_sps_count * sizeof(StdVideoH265SequenceParameterSet);
            params->h265_dec.std_vps = vk_alloc(&device->alloc, vps_size, 8, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
   params->h265_dec.std_sps = vk_alloc(&device->alloc, sps_size, 8, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
   params->h265_dec.std_pps = vk_alloc(&device->alloc, pps_size, 8, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
   if (!params->h265_dec.std_sps || !params->h265_dec.std_pps || !params->h265_dec.std_vps) {
      vk_free(&device->alloc, params->h265_dec.std_vps);
   vk_free(&device->alloc, params->h265_dec.std_sps);
   vk_free(&device->alloc, params->h265_dec.std_pps);
               init_add_h265_session_parameters(params, h265_create->pParametersAddInfo, templ);
      }
   default:
      unreachable("Unsupported video codec operation");
      }
      }
      void
   vk_video_session_parameters_finish(struct vk_device *device,
         {
      switch (params->op) {
   case VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR:
      vk_free(&device->alloc, params->h264_dec.std_sps);
   vk_free(&device->alloc, params->h264_dec.std_pps);
      case VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR:
      vk_free(&device->alloc, params->h265_dec.std_vps);
   vk_free(&device->alloc, params->h265_dec.std_sps);
   vk_free(&device->alloc, params->h265_dec.std_pps);
      default:
         }
      }
      static VkResult
   update_sps(struct vk_video_session_parameters *params,
         {
      if (params->h264_dec.std_sps_count + count >= params->h264_dec.max_std_sps_count)
            typed_memcpy(&params->h264_dec.std_sps[params->h264_dec.std_sps_count], adds, count);
   params->h264_dec.std_sps_count += count;
      }
      static VkResult
   update_h264_session_parameters(struct vk_video_session_parameters *params,
         {
               result = update_h264_dec_std_sps(params, h264_add->stdSPSCount, h264_add->pStdSPSs);
   if (result != VK_SUCCESS)
            result = update_h264_dec_std_pps(params, h264_add->stdPPSCount, h264_add->pStdPPSs);
      }
      static VkResult
   update_h265_session_parameters(struct vk_video_session_parameters *params,
         {
      VkResult result = VK_SUCCESS;
   result = update_h265_dec_std_vps(params, h265_add->stdVPSCount, h265_add->pStdVPSs);
   if (result != VK_SUCCESS)
            result = update_h265_dec_std_sps(params, h265_add->stdSPSCount, h265_add->pStdSPSs);
   if (result != VK_SUCCESS)
            result = update_h265_dec_std_pps(params, h265_add->stdPPSCount, h265_add->pStdPPSs);
      }
      VkResult
   vk_video_session_parameters_update(struct vk_video_session_parameters *params,
         {
      /* 39.6.5. Decoder Parameter Sets -
   * "The provided H.264 SPS/PPS parameters must be within the limits specified during decoder
   * creation for the decoder specified in VkVideoSessionParametersCreateInfoKHR."
            /*
   * There is no need to deduplicate here.
   * videoSessionParameters must not already contain a StdVideoH264PictureParameterSet entry with
   * both seq_parameter_set_id and pic_parameter_set_id matching any of the elements of
   * VkVideoDecodeH264SessionParametersAddInfoKHR::pStdPPS
   */
            switch (params->op) {
   case VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR: {
      const struct VkVideoDecodeH264SessionParametersAddInfoKHR *h264_add =
            }
   case VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR: {
      const struct VkVideoDecodeH265SessionParametersAddInfoKHR *h265_add =
               }
   default:
         }
      }
      const uint8_t h264_scaling_list_default_4x4_intra[] =
   {
      /* Table 7-3 - Default_4x4_Intra */
      };
      const uint8_t h264_scaling_list_default_4x4_inter[] =
   {
      /* Table 7-3 - Default_4x4_Inter */
      };
      const uint8_t h264_scaling_list_default_8x8_intra[] =
   {
      /* Table 7-4 - Default_8x8_Intra */
   6,  10, 10, 13, 11, 13, 16, 16, 16, 16, 18, 18, 18, 18, 18, 23,
   23, 23, 23, 23, 23, 25, 25, 25, 25, 25, 25, 25, 27, 27, 27, 27,
   27, 27, 27, 27, 29, 29, 29, 29, 29, 29, 29, 31, 31, 31, 31, 31,
      };
      const uint8_t h264_scaling_list_default_8x8_inter[] =
   {
      /* Table 7-4 - Default_8x8_Inter */
   9 , 13, 13, 15, 13, 15, 17, 17, 17, 17, 19, 19, 19, 19, 19, 21,
   21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 24, 24, 24, 24,
   24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 25, 27, 27, 27, 27, 27,
      };
      void
   vk_video_derive_h264_scaling_list(const StdVideoH264SequenceParameterSet *sps,
               {
               /* derive SPS scaling list first, because PPS may depend on it in fall-back
   * rule B */
   if (sps->flags.seq_scaling_matrix_present_flag)
   {
      for (int i = 0; i < STD_VIDEO_H264_SCALING_LIST_4X4_NUM_LISTS; i++)
   {
      if (sps->pScalingLists->scaling_list_present_mask & (1 << i))
      memcpy(temp.ScalingList4x4[i],
            else /* fall-back rule A */
   {
      if (i == 0)
      memcpy(temp.ScalingList4x4[i],
            else if (i == 3)
      memcpy(temp.ScalingList4x4[i],
            else
      memcpy(temp.ScalingList4x4[i],
                     for (int j = 0; j < STD_VIDEO_H264_SCALING_LIST_8X8_NUM_LISTS; j++)
   {
      int i = j + STD_VIDEO_H264_SCALING_LIST_4X4_NUM_LISTS;
   if (sps->pScalingLists->scaling_list_present_mask & (1 << i))
      memcpy(temp.ScalingList8x8[j], pps->pScalingLists->ScalingList8x8[j],
      else /* fall-back rule A */
   {
      if (i == 6)
      memcpy(temp.ScalingList8x8[j],
            else if (i == 7)
      memcpy(temp.ScalingList8x8[j],
            else
      memcpy(temp.ScalingList8x8[j], temp.ScalingList8x8[j - 2],
         }
   else
   {
      memset(temp.ScalingList4x4, 0x10,
         STD_VIDEO_H264_SCALING_LIST_4X4_NUM_LISTS *
   memset(temp.ScalingList8x8, 0x10,
                     if (pps->flags.pic_scaling_matrix_present_flag)
   {
      for (int i = 0; i < STD_VIDEO_H264_SCALING_LIST_4X4_NUM_LISTS; i++)
   {
      if (pps->pScalingLists->scaling_list_present_mask & (1 << i))
      memcpy(list->ScalingList4x4[i], pps->pScalingLists->ScalingList4x4[i],
      else if (sps->flags.seq_scaling_matrix_present_flag) /* fall-back rule B */
   {
      if (i == 0 || i == 3)
      memcpy(list->ScalingList4x4[i], temp.ScalingList4x4[i],
      else
      memcpy(list->ScalingList4x4[i], list->ScalingList4x4[i - 1],
   }
   else /* fall-back rule A */
   {
      if (i == 0)
      memcpy(list->ScalingList4x4[i],
            else if (i == 3)
      memcpy(list->ScalingList4x4[i],
            else
      memcpy(list->ScalingList4x4[i],
                     for (int j = 0; j < STD_VIDEO_H264_SCALING_LIST_8X8_NUM_LISTS; j++)
   {
      int i = j + STD_VIDEO_H264_SCALING_LIST_4X4_NUM_LISTS;
   if (pps->pScalingLists->scaling_list_present_mask & (1 << i))
      memcpy(list->ScalingList8x8[j], pps->pScalingLists->ScalingList8x8[j],
      else if (sps->flags.seq_scaling_matrix_present_flag) /* fall-back rule B */
   {
      if (i == 6 || i == 7)
      memcpy(list->ScalingList8x8[j], temp.ScalingList8x8[j],
      else
      memcpy(list->ScalingList8x8[j], list->ScalingList8x8[j - 2],
   }
   else /* fall-back rule A */
   {
      if (i == 6)
      memcpy(list->ScalingList8x8[j],
            else if (i == 7)
      memcpy(list->ScalingList8x8[j],
            else
      memcpy(list->ScalingList8x8[j], list->ScalingList8x8[j - 2],
         }
   else
   {
      memcpy(list->ScalingList4x4, temp.ScalingList4x4,
         STD_VIDEO_H264_SCALING_LIST_4X4_NUM_LISTS *
   memcpy(list->ScalingList8x8, temp.ScalingList8x8,
               }
      const StdVideoH264SequenceParameterSet *
   vk_video_find_h264_dec_std_sps(const struct vk_video_session_parameters *params,
         {
         }
      const StdVideoH264PictureParameterSet *
   vk_video_find_h264_dec_std_pps(const struct vk_video_session_parameters *params,
         {
         }
      const StdVideoH265VideoParameterSet *
   vk_video_find_h265_dec_std_vps(const struct vk_video_session_parameters *params,
         {
         }
      const StdVideoH265SequenceParameterSet *
   vk_video_find_h265_dec_std_sps(const struct vk_video_session_parameters *params,
         {
         }
      const StdVideoH265PictureParameterSet *
   vk_video_find_h265_dec_std_pps(const struct vk_video_session_parameters *params,
         {
         }
      int
   vk_video_h265_poc_by_slot(const struct VkVideoDecodeInfoKHR *frame_info, int slot)
   {
      for (unsigned i = 0; i < frame_info->referenceSlotCount; i++) {
      const VkVideoDecodeH265DpbSlotInfoKHR *dpb_slot_info =
         if (frame_info->pReferenceSlots[i].slotIndex == slot)
                           }
      void
   vk_fill_video_h265_reference_info(const VkVideoDecodeInfoKHR *frame_info,
                     {
      uint8_t list_cnt = slice_params->slice_type == STD_VIDEO_H265_SLICE_TYPE_B ? 2 : 1;
   uint8_t list_idx;
            for (list_idx = 0; list_idx < list_cnt; list_idx++) {
      /* The order is
   *  L0: Short term current before set - Short term current after set - long term current
   *  L1: Short term current after set - short term current before set - long term current
   */
   const uint8_t *rps[3] = {
      list_idx ? pic->pStdPictureInfo->RefPicSetStCurrAfter : pic->pStdPictureInfo->RefPicSetStCurrBefore,
   list_idx ? pic->pStdPictureInfo->RefPicSetStCurrBefore : pic->pStdPictureInfo->RefPicSetStCurrAfter,
               uint8_t ref_idx = 0;
   for (i = 0; i < 3; i++) {
               for (j = 0; (cur_rps[j] != 0xff) && ((j + ref_idx) < 8); j++) {
      ref_slots[list_idx][j + ref_idx].slot_index = cur_rps[j];
      }
               /* TODO: should handle cases where rpl_modification_flag is true. */
         }
      static void
   h265_pred_weight_table(struct vk_video_h265_slice_params *params,
                     {
      unsigned chroma_array_type = sps->flags.separate_colour_plane_flag ? 0 : sps->chroma_format_idc;
                              if (chroma_array_type != 0) {
      params->chroma_log2_weight_denom = params->luma_log2_weight_denom + vl_rbsp_se(rbsp);
               for (i = 0; i < params->num_ref_idx_l0_active; ++i) {
      params->luma_weight_l0_flag[i] = vl_rbsp_u(rbsp, 1);
   if (!params->luma_weight_l0_flag[i]) {
      params->luma_weight_l0[i] = 1 << params->luma_log2_weight_denom;
                  for (i = 0; i < params->num_ref_idx_l0_active; ++i) {
      if (chroma_array_type == 0) {
         } else {
                     for (i = 0; i < params->num_ref_idx_l0_active; ++i) {
      if (params->luma_weight_l0_flag[i]) {
      params->delta_luma_weight_l0[i] = vl_rbsp_se(rbsp);
   params->luma_weight_l0[i] = (1 << params->luma_log2_weight_denom) + params->delta_luma_weight_l0[i];
               if (params->chroma_weight_l0_flag[i]) {
      for (j = 0; j < 2; j++) {
                     params->chroma_weight_l0[i][j] =
         params->chroma_offset_l0[i][j] = CLAMP(params->delta_chroma_offset_l0[i][j] -
         } else {
      for (j = 0; j < 2; j++) {
      params->chroma_weight_l0[i][j] = 1 << params->chroma_log2_weight_denom;
                     if (slice_type == STD_VIDEO_H265_SLICE_TYPE_B) {
      for (i = 0; i < params->num_ref_idx_l1_active; ++i) {
      params->luma_weight_l1_flag[i] = vl_rbsp_u(rbsp, 1);
   if (!params->luma_weight_l1_flag[i]) {
      params->luma_weight_l1[i] = 1 << params->luma_log2_weight_denom;
                  for (i = 0; i < params->num_ref_idx_l1_active; ++i) {
      if (chroma_array_type == 0) {
         } else {
                     for (i = 0; i < params->num_ref_idx_l1_active; ++i) {
      if (params->luma_weight_l1_flag[i]) {
      params->delta_luma_weight_l1[i] = vl_rbsp_se(rbsp);
   params->luma_weight_l1[i] =
                     if (params->chroma_weight_l1_flag[i]) {
      for (j = 0; j < 2; j++) {
                     params->chroma_weight_l1[i][j] =
         params->chroma_offset_l1[i][j] = CLAMP(params->delta_chroma_offset_l1[i][j] -
         } else {
      for (j = 0; j < 2; j++) {
      params->chroma_weight_l1[i][j] = 1 << params->chroma_log2_weight_denom;
                  }
      void
   vk_video_parse_h265_slice_header(const struct VkVideoDecodeInfoKHR *frame_info,
                                       {
      struct vl_vlc vlc;
   const void *slice_headers[1] = { slice_data };
                              /* forbidden_zero_bit */
            if (vl_vlc_valid_bits(&vlc) < 15)
            vl_vlc_get_uimsbf(&vlc, 6); /* nal_unit_type */
   vl_vlc_get_uimsbf(&vlc, 6); /* nuh_layer_id */
            struct vl_rbsp rbsp;
                     params->slice_size = slice_size;
            /* no_output_of_prior_pics_flag */
   if (pic_info->pStdPictureInfo->flags.IrapPicFlag)
            /* pps id */
            if (!params->first_slice_segment_in_pic_flag) {
      int size, num;
            if (pps->flags.dependent_slice_segments_enabled_flag)
            size = 1 << (sps->log2_min_luma_coding_block_size_minus3 + 3 +
            num = ((sps->pic_width_in_luma_samples + size - 1) / size) *
            while (num > (1 << bits_slice_segment_address))
            /* slice_segment_address */
               if (params->dependent_slice_segment)
            for (unsigned i = 0; i < pps->num_extra_slice_header_bits; ++i)
      /* slice_reserved_flag */
         /* slice_type */
            if (pps->flags.output_flag_present_flag)
      /* pic output flag */
         if (sps->flags.separate_colour_plane_flag)
      /* colour_plane_id */
         if (!pic_info->pStdPictureInfo->flags.IdrPicFlag) {
      /* slice_pic_order_cnt_lsb */
   params->pic_order_cnt_lsb =
            /* short_term_ref_pic_set_sps_flag */
   if (!vl_rbsp_u(&rbsp, 1)) {
                              if (rps_predict) {
      /* delta_idx */
   vl_rbsp_ue(&rbsp);
   /* delta_rps_sign */
   vl_rbsp_u(&rbsp, 1);
                  for (int i = 0 ; i <= pic_info->pStdPictureInfo->NumDeltaPocsOfRefRpsIdx; i++) {
      uint8_t used = vl_rbsp_u(&rbsp, 1);
   if (!used)
         } else {
      /* num_negative_pics */
   unsigned num_neg_pics = vl_rbsp_ue(&rbsp);
                  for(unsigned i = 0 ; i < num_neg_pics; ++i) {
      /* delta_poc_s0_minus1 */
   vl_rbsp_ue(&rbsp);
                     for(unsigned i = 0; i < num_pos_pics; ++i) {
      /* delta_poc_s1_minus1 */
   vl_rbsp_ue(&rbsp);
   /* used_by_curr_pic_s0_flag */
               } else {
               int numbits = util_logbase2_ceil(num_st_rps);
   if (numbits > 0)
      /* short_term_ref_pic_set_idx */
            if (sps->flags.long_term_ref_pics_present_flag) {
                                             for (unsigned i = 0; i < num_refs; i++) {
      if (i < num_lt_sps) {
      if (sps->num_long_term_ref_pics_sps > 1)
      /* lt_idx_sps */
   vl_rbsp_u(&rbsp,
   } else {
      /* poc_lsb_lt */
   vl_rbsp_u(&rbsp, sps->log2_max_pic_order_cnt_lsb_minus4 + 4);
                     /* poc_msb_present */
   if (vl_rbsp_u(&rbsp, 1)) {
      /* delta_poc_msb_cycle_lt */
                     if (sps->flags.sps_temporal_mvp_enabled_flag)
               if (sps->flags.sample_adaptive_offset_enabled_flag) {
      params->sao_luma_flag = vl_rbsp_u(&rbsp, 1);
   if (sps->chroma_format_idc)
                                             if (params->slice_type == STD_VIDEO_H265_SLICE_TYPE_B)
         else
            /* num_ref_idx_active_override_flag */
   if (vl_rbsp_u(&rbsp, 1)) {
      params->num_ref_idx_l0_active = vl_rbsp_ue(&rbsp) + 1;
   if (params->slice_type == STD_VIDEO_H265_SLICE_TYPE_B)
               if (pps->flags.lists_modification_present_flag) {
      params->rpl_modification_flag[0] = vl_rbsp_u(&rbsp, 1);
   if (params->rpl_modification_flag[0]) {
      for (int i = 0; i < params->num_ref_idx_l0_active; i++) {
      /* list_entry_l0 */
   vl_rbsp_u(&rbsp,
                  if (params->slice_type == STD_VIDEO_H265_SLICE_TYPE_B) {
      params->rpl_modification_flag[1] = vl_rbsp_u(&rbsp, 1);
   if (params->rpl_modification_flag[1]) {
      for (int i = 0; i < params->num_ref_idx_l1_active; i++) {
      /* list_entry_l1 */
   vl_rbsp_u(&rbsp,
                        if (params->slice_type == STD_VIDEO_H265_SLICE_TYPE_B)
            if (pps->flags.cabac_init_present_flag)
                  if (params->temporal_mvp_enable) {
                     if (params->collocated_list == 0) {
      if (params->num_ref_idx_l0_active > 1)
      }  else if (params->collocated_list == 1) {
      if (params->num_ref_idx_l1_active > 1)
                  if ((pps->flags.weighted_pred_flag && params->slice_type == STD_VIDEO_H265_SLICE_TYPE_P) ||
                                             if (pps->flags.pps_slice_chroma_qp_offsets_present_flag) {
      params->slice_cb_qp_offset = vl_rbsp_se(&rbsp);
               if (pps->flags.chroma_qp_offset_list_enabled_flag)
      /* cu_chroma_qp_offset_enabled_flag */
         if (pps->flags.deblocking_filter_control_present_flag) {
      if (pps->flags.deblocking_filter_override_enabled_flag) {
      /* deblocking_filter_override_flag */
                     if (!params->disable_deblocking_filter_idc) {
      params->beta_offset_div2 = vl_rbsp_se(&rbsp);
         } else {
      params->disable_deblocking_filter_idc =
                     if (pps->flags.pps_loop_filter_across_slices_enabled_flag &&
         (params->sao_luma_flag || params->sao_chroma_flag ||
            if (pps->flags.tiles_enabled_flag || pps->flags.entropy_coding_sync_enabled_flag) {
               if (num_entry_points_offsets > 0) {
      unsigned offset_len = vl_rbsp_ue(&rbsp) + 1;
   for (unsigned i = 0; i < num_entry_points_offsets; i++) {
      /* entry_point_offset_minus1 */
                     if (pps->flags.pps_extension_present_flag) {
      unsigned length = vl_rbsp_ue(&rbsp);
   for (unsigned i = 0; i < length; i++)
      /* slice_reserved_undetermined_flag */
            unsigned header_bits =
            }
      void
   vk_video_get_profile_alignments(const VkVideoProfileListInfoKHR *profile_list,
         {
      uint32_t width_align = 1, height_align = 1;
   for (unsigned i = 0; i < profile_list->profileCount; i++) {
      if (profile_list->pProfiles[i].videoCodecOperation == VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR) {
      width_align = MAX2(width_align, VK_VIDEO_H264_MACROBLOCK_WIDTH);
      }
   if (profile_list->pProfiles[i].videoCodecOperation == VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR) {
      width_align = MAX2(width_align, VK_VIDEO_H265_CTU_MAX_WIDTH);
         }
   *width_align_out = width_align;
      }
