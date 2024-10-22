   /**************************************************************************
   *
   * Copyright 2011 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   *
   **************************************************************************/
      #include "radeon_uvd.h"
      #include "pipe/p_video_codec.h"
   #include "radeon_video.h"
   #include "radeonsi/si_pipe.h"
   #include "util/u_memory.h"
   #include "util/u_video.h"
   #include "vl/vl_defines.h"
   #include "vl/vl_mpeg12_decoder.h"
   #include <sys/types.h>
      #include <assert.h>
   #include <errno.h>
   #include <stdio.h>
   #include <unistd.h>
      #define NUM_BUFFERS 4
      #define NUM_MPEG2_REFS 6
   #define NUM_H264_REFS  17
   #define NUM_VC1_REFS   5
      #define FB_BUFFER_OFFSET         0x1000
   #define FB_BUFFER_SIZE           2048
   #define FB_BUFFER_SIZE_TONGA     (2048 * 64)
   #define IT_SCALING_TABLE_SIZE    992
   #define UVD_SESSION_CONTEXT_SIZE (128 * 1024)
      /* UVD decoder representation */
   struct ruvd_decoder {
                        unsigned stream_handle;
   unsigned stream_type;
            struct pipe_screen *screen;
   struct radeon_winsys *ws;
                     struct rvid_buffer msg_fb_it_buffers[NUM_BUFFERS];
   struct ruvd_msg *msg;
   uint32_t *fb;
   unsigned fb_size;
            struct rvid_buffer bs_buffers[NUM_BUFFERS];
   void *bs_ptr;
            struct rvid_buffer dpb;
   bool use_legacy;
   struct rvid_buffer ctx;
   struct rvid_buffer sessionctx;
   struct {
      unsigned data0;
   unsigned data1;
   unsigned cmd;
                  };
      /* flush IB to the hardware */
   static int flush(struct ruvd_decoder *dec, unsigned flags, struct pipe_fence_handle **fence)
   {
         }
      static int ruvd_dec_get_decoder_fence(struct pipe_video_codec *decoder,
                  struct ruvd_decoder *dec = (struct ruvd_decoder *)decoder;
      }
      /* add a new set register command to the IB */
   static void set_reg(struct ruvd_decoder *dec, unsigned reg, uint32_t val)
   {
      radeon_emit(&dec->cs, RUVD_PKT0(reg >> 2, 0));
      }
      /* send a command to the VCPU through the GPCOM registers */
   static void send_cmd(struct ruvd_decoder *dec, unsigned cmd, struct pb_buffer *buf, uint32_t off,
         {
               reloc_idx = dec->ws->cs_add_buffer(&dec->cs, buf, usage | RADEON_USAGE_SYNCHRONIZED, domain);
   if (!dec->use_legacy) {
      uint64_t addr;
   addr = dec->ws->buffer_get_virtual_address(buf);
   addr = addr + off;
   set_reg(dec, dec->reg.data0, addr);
      } else {
      off += dec->ws->buffer_get_reloc_offset(buf);
   set_reg(dec, RUVD_GPCOM_VCPU_DATA0, off);
      }
      }
      /* do the codec needs an IT buffer ?*/
   static bool have_it(struct ruvd_decoder *dec)
   {
         }
      /* map the next available message/feedback/itscaling buffer */
   static void map_msg_fb_it_buf(struct ruvd_decoder *dec)
   {
      struct rvid_buffer *buf;
            /* grab the current message/feedback buffer */
            /* and map it for CPU access */
   ptr =
            /* calc buffer offsets */
   dec->msg = (struct ruvd_msg *)ptr;
            dec->fb = (uint32_t *)(ptr + FB_BUFFER_OFFSET);
   if (have_it(dec))
      }
      /* unmap and send a message command to the VCPU */
   static void send_msg_buf(struct ruvd_decoder *dec)
   {
               /* ignore the request if message/feedback buffer isn't mapped */
   if (!dec->msg || !dec->fb)
            /* grab the current message buffer */
            /* unmap the buffer */
   dec->ws->buffer_unmap(dec->ws, buf->res->buf);
   dec->msg = NULL;
   dec->fb = NULL;
            if (dec->sessionctx.res)
      send_cmd(dec, RUVD_CMD_SESSION_CONTEXT_BUFFER, dec->sessionctx.res->buf, 0,
         /* and send it to the hardware */
      }
      /* cycle to the next set of buffers */
   static void next_buffer(struct ruvd_decoder *dec)
   {
      ++dec->cur_buffer;
      }
      /* convert the profile into something UVD understands */
   static uint32_t profile2stream_type(struct ruvd_decoder *dec, unsigned family)
   {
      switch (u_reduce_video_profile(dec->base.profile)) {
   case PIPE_VIDEO_FORMAT_MPEG4_AVC:
            case PIPE_VIDEO_FORMAT_VC1:
            case PIPE_VIDEO_FORMAT_MPEG12:
            case PIPE_VIDEO_FORMAT_MPEG4:
            case PIPE_VIDEO_FORMAT_HEVC:
            case PIPE_VIDEO_FORMAT_JPEG:
            default:
      assert(0);
         }
      static unsigned calc_ctx_size_h264_perf(struct ruvd_decoder *dec)
   {
      unsigned width_in_mb, height_in_mb, ctx_size;
   unsigned width = align(dec->base.width, VL_MACROBLOCK_WIDTH);
                     // picture width & height in 16 pixel units
   width_in_mb = width / VL_MACROBLOCK_WIDTH;
            if (!dec->use_legacy) {
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
      } else {
      // the firmware seems to always assume a minimum of ref frames
   max_references = MAX2(NUM_H264_REFS, max_references);
   // macroblock context buffer
                  }
      static unsigned calc_ctx_size_h265_main(struct ruvd_decoder *dec)
   {
      unsigned width = align(dec->base.width, VL_MACROBLOCK_WIDTH);
                     if (dec->base.width * dec->base.height >= 4096 * 2000)
         else
            width = align(width, 16);
   height = align(height, 16);
      }
      static unsigned calc_ctx_size_h265_main10(struct ruvd_decoder *dec,
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
      static unsigned get_db_pitch_alignment(struct ruvd_decoder *dec)
   {
      if (((struct si_screen *)dec->screen)->info.family < CHIP_VEGA10)
         else
      }
      /* calculate size of reference picture buffer */
   static unsigned calc_dpb_size(struct ruvd_decoder *dec)
   {
               // always align them to MB size for dpb calculation
   unsigned width = align(dec->base.width, VL_MACROBLOCK_WIDTH);
            // always one more for currently decoded picture
            // aligned size of a single frame
   image_size = align(width, get_db_pitch_alignment(dec)) * height;
   image_size += image_size / 2;
            // picture width & height in 16 pixel units
   width_in_mb = width / VL_MACROBLOCK_WIDTH;
            switch (u_reduce_video_profile(dec->base.profile)) {
   case PIPE_VIDEO_FORMAT_MPEG4_AVC: {
      if (!dec->use_legacy) {
                     if (dec->stream_type == RUVD_CODEC_H264_PERF)
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
   if ((dec->stream_type != RUVD_CODEC_H264_PERF) ||
      (((struct si_screen *)dec->screen)->info.family < CHIP_POLARIS10)) {
   dpb_size += max_references * align(width_in_mb * height_in_mb * 192, alignment);
         } else {
      // the firmware seems to always assume a minimum of ref frames
   max_references = MAX2(NUM_H264_REFS, max_references);
   // reference picture buffer
   dpb_size = image_size * max_references;
   if ((dec->stream_type != RUVD_CODEC_H264_PERF) ||
      (((struct si_screen *)dec->screen)->info.family < CHIP_POLARIS10)) {
   // macroblock context buffer
   dpb_size += width_in_mb * height_in_mb * max_references * 192;
   // IT surface buffer
         }
               case PIPE_VIDEO_FORMAT_HEVC:
      if (dec->base.width * dec->base.height >= 4096 * 2000)
         else
            width = align(width, 16);
   height = align(height, 16);
   if (dec->base.profile == PIPE_VIDEO_PROFILE_HEVC_MAIN_10)
      dpb_size = align((align(width, get_db_pitch_alignment(dec)) * height * 9) / 4, 256) *
      else
      dpb_size = align((align(width, get_db_pitch_alignment(dec)) * height * 3) / 2, 256) *
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
         case PIPE_VIDEO_FORMAT_JPEG:
      dpb_size = 0;
         default:
      // something is missing here
            // at least use a sane default value
   dpb_size = 32 * 1024 * 1024;
      }
      }
      /* free associated data in the video buffer callback */
   static void ruvd_destroy_associated_data(void *data)
   {
         }
      /* get h264 specific message bits */
   static struct ruvd_h264 get_h264_msg(struct ruvd_decoder *dec, struct pipe_h264_picture_desc *pic)
   {
               memset(&result, 0, sizeof(result));
   switch (pic->base.profile) {
   case PIPE_VIDEO_PROFILE_MPEG4_AVC_BASELINE:
   case PIPE_VIDEO_PROFILE_MPEG4_AVC_CONSTRAINED_BASELINE:
      result.profile = RUVD_H264_PROFILE_BASELINE;
         case PIPE_VIDEO_PROFILE_MPEG4_AVC_MAIN:
      result.profile = RUVD_H264_PROFILE_MAIN;
         case PIPE_VIDEO_PROFILE_MPEG4_AVC_HIGH:
      result.profile = RUVD_H264_PROFILE_HIGH;
         default:
      assert(0);
                        result.sps_info_flags = 0;
   result.sps_info_flags |= pic->pps->sps->direct_8x8_inference_flag << 0;
   result.sps_info_flags |= pic->pps->sps->mb_adaptive_frame_field_flag << 1;
   result.sps_info_flags |= pic->pps->sps->frame_mbs_only_flag << 2;
            result.bit_depth_luma_minus8 = pic->pps->sps->bit_depth_luma_minus8;
   result.bit_depth_chroma_minus8 = pic->pps->sps->bit_depth_chroma_minus8;
   result.log2_max_frame_num_minus4 = pic->pps->sps->log2_max_frame_num_minus4;
   result.pic_order_cnt_type = pic->pps->sps->pic_order_cnt_type;
            switch (dec->base.chroma_format) {
   case PIPE_VIDEO_CHROMA_FORMAT_NONE:
      /* TODO: assert? */
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
            if (dec->stream_type == RUVD_CODEC_H264_PERF) {
      memcpy(dec->it, result.scaling_list_4x4, 6 * 16);
                        result.num_ref_idx_l0_active_minus1 = pic->num_ref_idx_l0_active_minus1;
            result.frame_num = pic->frame_num;
   memcpy(result.frame_num_list, pic->frame_num_list, 4 * 16);
   result.curr_field_order_cnt_list[0] = pic->field_order_cnt[0];
   result.curr_field_order_cnt_list[1] = pic->field_order_cnt[1];
                        }
      /* get h265 specific message bits */
   static struct ruvd_h265 get_h265_msg(struct ruvd_decoder *dec, struct pipe_video_buffer *target,
         {
      struct ruvd_h265 result;
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
   result.pps_info_flags |= pic->pps->slice_segment_header_extension_present_flag << 19;
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
            for (i = 0; i < 16; i++) {
      for (j = 0; (pic->ref[j] != NULL) && (j < 16); j++) {
      if (dec->render_pic_list[i] == pic->ref[j])
         if (j == 15)
         else if (pic->ref[j + 1] == NULL)
         }
   for (i = 0; i < 16; i++) {
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
      result.luma_10to8 = 5;
   result.chroma_10to8 = 5;
   result.sclr_luma10to8 = 4;
                  /* TODO
   result.highestTid;
            IDRPicFlag;
   RAPPicFlag;
   NumPocTotalCurr;
   NumShortTermPictureSliceHeaderBits;
            IsLongTerm[16];
               }
      /* get vc1 specific message bits */
   static struct ruvd_vc1 get_vc1_msg(struct pipe_vc1_picture_desc *pic)
   {
                        switch (pic->base.profile) {
   case PIPE_VIDEO_PROFILE_VC1_SIMPLE:
      result.profile = RUVD_VC1_PROFILE_SIMPLE;
   result.level = 1;
         case PIPE_VIDEO_PROFILE_VC1_MAIN:
      result.profile = RUVD_VC1_PROFILE_MAIN;
   result.level = 2;
         case PIPE_VIDEO_PROFILE_VC1_ADVANCED:
      result.profile = RUVD_VC1_PROFILE_ADVANCED;
   result.level = 4;
         default:
                  /* fields common for all profiles */
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
            /* some fields only apply to main/advanced profile */
   if (pic->base.profile != PIPE_VIDEO_PROFILE_VC1_SIMPLE) {
      result.pps_info_flags |= pic->syncmarker << 20;
   result.pps_info_flags |= pic->rangered << 19;
   result.pps_info_flags |= pic->loopfilter << 5;
   result.pps_info_flags |= pic->fastuvmc << 4;
   result.pps_info_flags |= pic->extended_mv << 3;
   result.pps_info_flags |= pic->extended_dmv << 8;
                     #if 0
   //(((unsigned int)(pPicParams->advance.reserved1)) << SPS_INFO_VC1_RESERVED_SHIFT)
   uint32_t  slice_count
   uint8_t   picture_type
   uint8_t   frame_coding_mode
   uint8_t   deblockEnable
   uint8_t   pquant
   #endif
            }
      /* extract the frame number from a referenced video buffer */
   static uint32_t get_ref_pic_idx(struct ruvd_decoder *dec, struct pipe_video_buffer *ref)
   {
      uint32_t min = MAX2(dec->frame_number, NUM_MPEG2_REFS) - NUM_MPEG2_REFS;
   uint32_t max = MAX2(dec->frame_number, 1) - 1;
            /* seems to be the most sane fallback */
   if (!ref)
            /* get the frame number from the associated data */
            /* limit the frame number to a valid range */
      }
      /* get mpeg2 specific msg bits */
   static struct ruvd_mpeg2 get_mpeg2_msg(struct ruvd_decoder *dec,
         {
      const int *zscan = pic->alternate_scan ? vl_zscan_alternate : vl_zscan_normal;
   struct ruvd_mpeg2 result;
            memset(&result, 0, sizeof(result));
   result.decoded_pic_idx = dec->frame_number;
   for (i = 0; i < 2; ++i)
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
      /* get mpeg4 specific msg bits */
   static struct ruvd_mpeg4 get_mpeg4_msg(struct ruvd_decoder *dec,
         {
      struct ruvd_mpeg4 result;
            memset(&result, 0, sizeof(result));
   result.decoded_pic_idx = dec->frame_number;
   for (i = 0; i < 2; ++i)
            result.variant_type = 0;
            result.video_object_layer_verid = 0x5; // advanced simple
            result.video_object_layer_width = dec->base.width;
                     result.flags |= pic->short_video_header << 0;
   // result.flags |= obmc_disable << 1;
   result.flags |= pic->interlaced << 2;
   result.flags |= 1 << 3; // load_intra_quant_mat
   result.flags |= 1 << 4; // load_nonintra_quant_mat
   result.flags |= pic->quarter_sample << 5;
   result.flags |= 1 << 6; // complexity_estimation_disable
   result.flags |= pic->resync_marker_disable << 7;
   // result.flags |= data_partitioned << 8;
   // result.flags |= reversible_vlc << 9;
   result.flags |= 0 << 10; // newpred_enable
   result.flags |= 0 << 11; // reduced_resolution_vop_enable
   // result.flags |= scalability << 12;
   // result.flags |= is_object_layer_identifier << 13;
   // result.flags |= fixed_vop_rate << 14;
                     for (i = 0; i < 64; ++i) {
      result.intra_quant_mat[i] = pic->intra_matrix[vl_zscan_normal[i]];
               /*
   int32_t    trd [2]
   int32_t    trb [2]
   uint8_t    vop_coding_type
   uint8_t    vop_fcode_forward
   uint8_t    vop_fcode_backward
   uint8_t    rounding_control
   uint8_t    alternate_vertical_scan_flag
   uint8_t    top_field_first
               }
      /**
   * destroy this video decoder
   */
   static void ruvd_destroy(struct pipe_video_codec *decoder)
   {
      struct ruvd_decoder *dec = (struct ruvd_decoder *)decoder;
                     map_msg_fb_it_buf(dec);
   dec->msg->size = sizeof(*dec->msg);
   dec->msg->msg_type = RUVD_MSG_DESTROY;
   dec->msg->stream_handle = dec->stream_handle;
                              for (i = 0; i < NUM_BUFFERS; ++i) {
      si_vid_destroy_buffer(&dec->msg_fb_it_buffers[i]);
               si_vid_destroy_buffer(&dec->dpb);
   si_vid_destroy_buffer(&dec->ctx);
               }
      /**
   * start decoding of a new frame
   */
   static void ruvd_begin_frame(struct pipe_video_codec *decoder, struct pipe_video_buffer *target,
         {
      struct ruvd_decoder *dec = (struct ruvd_decoder *)decoder;
                     frame = ++dec->frame_number;
   vl_video_buffer_set_associated_data(target, decoder, (void *)frame,
            dec->bs_size = 0;
   dec->bs_ptr = dec->ws->buffer_map(dec->ws, dec->bs_buffers[dec->cur_buffer].res->buf, &dec->cs,
      }
      /**
   * decode a macroblock
   */
   static void ruvd_decode_macroblock(struct pipe_video_codec *decoder,
                           {
      /* not supported (yet) */
      }
      /**
   * decode a bitstream
   */
   static void ruvd_decode_bitstream(struct pipe_video_codec *decoder,
                     {
      struct ruvd_decoder *dec = (struct ruvd_decoder *)decoder;
                     if (!dec->bs_ptr)
            for (i = 0; i < num_buffers; ++i) {
      struct rvid_buffer *buf = &dec->bs_buffers[dec->cur_buffer];
            if (new_size > buf->res->buf->size) {
      dec->ws->buffer_unmap(dec->ws, buf->res->buf);
   if (!si_vid_resize_buffer(dec->screen, &dec->cs, buf, new_size, NULL)) {
      RVID_ERR("Can't resize bitstream buffer!");
               dec->bs_ptr = dec->ws->buffer_map(dec->ws, buf->res->buf, &dec->cs,
                                    memcpy(dec->bs_ptr, buffers[i], sizes[i]);
   dec->bs_size += sizes[i];
         }
      /**
   * end decoding of the current frame
   */
   static void ruvd_end_frame(struct pipe_video_codec *decoder, struct pipe_video_buffer *target,
         {
      struct ruvd_decoder *dec = (struct ruvd_decoder *)decoder;
   struct pb_buffer *dt;
   struct rvid_buffer *msg_fb_it_buf, *bs_buf;
                     if (!dec->bs_ptr)
            msg_fb_it_buf = &dec->msg_fb_it_buffers[dec->cur_buffer];
            bs_size = align(dec->bs_size, 128);
   memset(dec->bs_ptr, 0, bs_size - dec->bs_size);
            map_msg_fb_it_buf(dec);
   dec->msg->size = sizeof(*dec->msg);
   dec->msg->msg_type = RUVD_MSG_DECODE;
   dec->msg->stream_handle = dec->stream_handle;
            dec->msg->body.decode.stream_type = dec->stream_type;
   dec->msg->body.decode.decode_flags = 0x1;
   dec->msg->body.decode.width_in_samples = dec->base.width;
            if ((picture->profile == PIPE_VIDEO_PROFILE_VC1_SIMPLE) ||
      (picture->profile == PIPE_VIDEO_PROFILE_VC1_MAIN)) {
   dec->msg->body.decode.width_in_samples =
         dec->msg->body.decode.height_in_samples =
               if (dec->dpb.res)
         dec->msg->body.decode.bsd_size = bs_size;
            if (dec->stream_type == RUVD_CODEC_H264_PERF &&
      ((struct si_screen *)dec->screen)->info.family >= CHIP_POLARIS10)
         dt = dec->set_dtb(dec->msg, (struct vl_video_buffer *)target);
   if (((struct si_screen *)dec->screen)->info.family >= CHIP_STONEY)
            switch (u_reduce_video_profile(picture->profile)) {
   case PIPE_VIDEO_FORMAT_MPEG4_AVC:
      dec->msg->body.decode.codec.h264 =
               case PIPE_VIDEO_FORMAT_HEVC:
      dec->msg->body.decode.codec.h265 =
         if (dec->ctx.res == NULL) {
      unsigned ctx_size;
   if (dec->base.profile == PIPE_VIDEO_PROFILE_HEVC_MAIN_10)
         else
         if (!si_vid_create_buffer(dec->screen, &dec->ctx, ctx_size, PIPE_USAGE_DEFAULT)) {
         }
               if (dec->ctx.res)
               case PIPE_VIDEO_FORMAT_VC1:
      dec->msg->body.decode.codec.vc1 = get_vc1_msg((struct pipe_vc1_picture_desc *)picture);
         case PIPE_VIDEO_FORMAT_MPEG12:
      dec->msg->body.decode.codec.mpeg2 =
               case PIPE_VIDEO_FORMAT_MPEG4:
      dec->msg->body.decode.codec.mpeg4 =
               case PIPE_VIDEO_FORMAT_JPEG:
            default:
      assert(0);
               dec->msg->body.decode.db_surf_tile_config = dec->msg->body.decode.dt_surf_tile_config;
            /* set at least the feedback buffer size */
                     if (dec->dpb.res)
      send_cmd(dec, RUVD_CMD_DPB_BUFFER, dec->dpb.res->buf, 0, RADEON_USAGE_READWRITE,
         if (dec->ctx.res)
      send_cmd(dec, RUVD_CMD_CONTEXT_BUFFER, dec->ctx.res->buf, 0, RADEON_USAGE_READWRITE,
      send_cmd(dec, RUVD_CMD_BITSTREAM_BUFFER, bs_buf->res->buf, 0, RADEON_USAGE_READ,
         send_cmd(dec, RUVD_CMD_DECODING_TARGET_BUFFER, dt, 0, RADEON_USAGE_WRITE, RADEON_DOMAIN_VRAM);
   send_cmd(dec, RUVD_CMD_FEEDBACK_BUFFER, msg_fb_it_buf->res->buf, FB_BUFFER_OFFSET,
         if (have_it(dec))
      send_cmd(dec, RUVD_CMD_ITSCALING_TABLE_BUFFER, msg_fb_it_buf->res->buf,
               flush(dec, PIPE_FLUSH_ASYNC, picture->fence);
      }
      /**
   * flush any outstanding command buffers to the hardware
   */
   static void ruvd_flush(struct pipe_video_codec *decoder)
   {
   }
      /**
   * create and UVD decoder
   */
   struct pipe_video_codec *si_common_uvd_create_decoder(struct pipe_context *context,
               {
      struct si_context *sctx = (struct si_context *)context;
   struct radeon_winsys *ws = sctx->ws;
   unsigned dpb_size;
   unsigned width = templ->width, height = templ->height;
   unsigned bs_buf_size;
   struct ruvd_decoder *dec;
            switch (u_reduce_video_profile(templ->profile)) {
   case PIPE_VIDEO_FORMAT_MPEG12:
      if (templ->entrypoint > PIPE_VIDEO_ENTRYPOINT_BITSTREAM)
               case PIPE_VIDEO_FORMAT_MPEG4:
      width = align(width, VL_MACROBLOCK_WIDTH);
   height = align(height, VL_MACROBLOCK_HEIGHT);
      case PIPE_VIDEO_FORMAT_MPEG4_AVC:
      width = align(width, VL_MACROBLOCK_WIDTH);
   height = align(height, VL_MACROBLOCK_HEIGHT);
         default:
                           if (!dec)
            if (!sctx->screen->info.is_amdgpu)
            dec->base = *templ;
   dec->base.context = context;
   dec->base.width = width;
            dec->base.destroy = ruvd_destroy;
   dec->base.begin_frame = ruvd_begin_frame;
   dec->base.decode_macroblock = ruvd_decode_macroblock;
   dec->base.decode_bitstream = ruvd_decode_bitstream;
   dec->base.end_frame = ruvd_end_frame;
   dec->base.flush = ruvd_flush;
            dec->stream_type = profile2stream_type(dec, sctx->family);
   dec->set_dtb = set_dtb;
   dec->stream_handle = si_vid_alloc_stream_handle();
   dec->screen = context->screen;
            if (!ws->cs_create(&dec->cs, sctx->ctx, AMD_IP_UVD, NULL, NULL)) {
      RVID_ERR("Can't get command submission context.\n");
               for (i = 0; i < 16; i++)
         dec->fb_size = (sctx->family == CHIP_TONGA) ? FB_BUFFER_SIZE_TONGA : FB_BUFFER_SIZE;
   bs_buf_size = width * height * (512 / (16 * 16));
   for (i = 0; i < NUM_BUFFERS; ++i) {
      unsigned msg_fb_it_size = FB_BUFFER_OFFSET + dec->fb_size;
   STATIC_ASSERT(sizeof(struct ruvd_msg) <= FB_BUFFER_OFFSET);
   if (have_it(dec))
         if (!si_vid_create_buffer(dec->screen, &dec->msg_fb_it_buffers[i], msg_fb_it_size,
            RVID_ERR("Can't allocated message buffers.\n");
               if (!si_vid_create_buffer(dec->screen, &dec->bs_buffers[i], bs_buf_size,
            RVID_ERR("Can't allocated bitstream buffers.\n");
               si_vid_clear_buffer(context, &dec->msg_fb_it_buffers[i]);
               dpb_size = calc_dpb_size(dec);
   if (dpb_size) {
      if (!si_vid_create_buffer(dec->screen, &dec->dpb, dpb_size, PIPE_USAGE_DEFAULT)) {
      RVID_ERR("Can't allocated dpb.\n");
      }
               if (dec->stream_type == RUVD_CODEC_H264_PERF && sctx->family >= CHIP_POLARIS10) {
      unsigned ctx_size = calc_ctx_size_h264_perf(dec);
   if (!si_vid_create_buffer(dec->screen, &dec->ctx, ctx_size, PIPE_USAGE_DEFAULT)) {
      RVID_ERR("Can't allocated context buffer.\n");
      }
               if (sctx->family >= CHIP_POLARIS10) {
      if (!si_vid_create_buffer(dec->screen, &dec->sessionctx, UVD_SESSION_CONTEXT_SIZE,
            RVID_ERR("Can't allocated session ctx.\n");
      }
               if (sctx->family >= CHIP_VEGA10) {
      dec->reg.data0 = RUVD_GPCOM_VCPU_DATA0_SOC15;
   dec->reg.data1 = RUVD_GPCOM_VCPU_DATA1_SOC15;
   dec->reg.cmd = RUVD_GPCOM_VCPU_CMD_SOC15;
      } else {
      dec->reg.data0 = RUVD_GPCOM_VCPU_DATA0;
   dec->reg.data1 = RUVD_GPCOM_VCPU_DATA1;
   dec->reg.cmd = RUVD_GPCOM_VCPU_CMD;
               map_msg_fb_it_buf(dec);
   dec->msg->size = sizeof(*dec->msg);
   dec->msg->msg_type = RUVD_MSG_CREATE;
   dec->msg->stream_handle = dec->stream_handle;
   dec->msg->body.create.stream_type = dec->stream_type;
   dec->msg->body.create.width_in_samples = dec->base.width;
   dec->msg->body.create.height_in_samples = dec->base.height;
   dec->msg->body.create.dpb_size = dpb_size;
   send_msg_buf(dec);
   r = flush(dec, 0, NULL);
   if (r)
                           error:
               for (i = 0; i < NUM_BUFFERS; ++i) {
      si_vid_destroy_buffer(&dec->msg_fb_it_buffers[i]);
               si_vid_destroy_buffer(&dec->dpb);
   si_vid_destroy_buffer(&dec->ctx);
                        }
      /* calculate top/bottom offset */
   static unsigned texture_offset(struct radeon_surf *surface, unsigned layer,
         {
      switch (type) {
   default:
   case RUVD_SURFACE_TYPE_LEGACY:
      return (uint64_t)surface->u.legacy.level[0].offset_256B * 256 +
            case RUVD_SURFACE_TYPE_GFX9:
      return surface->u.gfx9.surf_offset + layer * surface->u.gfx9.surf_slice_size;
         }
      /* hw encode the aspect of macro tiles */
   static unsigned macro_tile_aspect(unsigned macro_tile_aspect)
   {
      switch (macro_tile_aspect) {
   default:
   case 1:
      macro_tile_aspect = 0;
      case 2:
      macro_tile_aspect = 1;
      case 4:
      macro_tile_aspect = 2;
      case 8:
      macro_tile_aspect = 3;
      }
      }
      /* hw encode the bank width and height */
   static unsigned bank_wh(unsigned bankwh)
   {
      switch (bankwh) {
   default:
   case 1:
      bankwh = 0;
      case 2:
      bankwh = 1;
      case 4:
      bankwh = 2;
      case 8:
      bankwh = 3;
      }
      }
      /**
   * fill decoding target field from the luma and chroma surfaces
   */
   void si_uvd_set_dt_surfaces(struct ruvd_msg *msg, struct radeon_surf *luma,
         {
      switch (type) {
   default:
   case RUVD_SURFACE_TYPE_LEGACY:
      msg->body.decode.dt_pitch = luma->u.legacy.level[0].nblk_x * luma->blk_w;
   switch (luma->u.legacy.level[0].mode) {
   case RADEON_SURF_MODE_LINEAR_ALIGNED:
      msg->body.decode.dt_tiling_mode = RUVD_TILE_LINEAR;
   msg->body.decode.dt_array_mode = RUVD_ARRAY_MODE_LINEAR;
      case RADEON_SURF_MODE_1D:
      msg->body.decode.dt_tiling_mode = RUVD_TILE_8X8;
   msg->body.decode.dt_array_mode = RUVD_ARRAY_MODE_1D_THIN;
      case RADEON_SURF_MODE_2D:
      msg->body.decode.dt_tiling_mode = RUVD_TILE_8X8;
   msg->body.decode.dt_array_mode = RUVD_ARRAY_MODE_2D_THIN;
      default:
      assert(0);
               msg->body.decode.dt_luma_top_offset = texture_offset(luma, 0, type);
   if (chroma)
         if (msg->body.decode.dt_field_mode) {
      msg->body.decode.dt_luma_bottom_offset = texture_offset(luma, 1, type);
   if (chroma)
      } else {
      msg->body.decode.dt_luma_bottom_offset = msg->body.decode.dt_luma_top_offset;
               if (chroma) {
      assert(luma->u.legacy.bankw == chroma->u.legacy.bankw);
   assert(luma->u.legacy.bankh == chroma->u.legacy.bankh);
               msg->body.decode.dt_surf_tile_config |= RUVD_BANK_WIDTH(bank_wh(luma->u.legacy.bankw));
   msg->body.decode.dt_surf_tile_config |= RUVD_BANK_HEIGHT(bank_wh(luma->u.legacy.bankh));
   msg->body.decode.dt_surf_tile_config |=
            case RUVD_SURFACE_TYPE_GFX9:
      msg->body.decode.dt_pitch = luma->u.gfx9.surf_pitch * luma->blk_w;
   /* SWIZZLE LINEAR MODE */
   msg->body.decode.dt_tiling_mode = RUVD_TILE_LINEAR;
   msg->body.decode.dt_array_mode = RUVD_ARRAY_MODE_LINEAR;
   msg->body.decode.dt_luma_top_offset = texture_offset(luma, 0, type);
   msg->body.decode.dt_chroma_top_offset = texture_offset(chroma, 0, type);
   if (msg->body.decode.dt_field_mode) {
      msg->body.decode.dt_luma_bottom_offset = texture_offset(luma, 1, type);
      } else {
      msg->body.decode.dt_luma_bottom_offset = msg->body.decode.dt_luma_top_offset;
      }
   msg->body.decode.dt_surf_tile_config = 0;
         }
