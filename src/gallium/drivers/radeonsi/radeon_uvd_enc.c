   /**************************************************************************
   *
   * Copyright 2018 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   *
   **************************************************************************/
      #include "radeon_uvd_enc.h"
      #include "pipe/p_video_codec.h"
   #include "radeon_video.h"
   #include "radeonsi/si_pipe.h"
   #include "util/u_memory.h"
   #include "util/u_video.h"
   #include "vl/vl_video_buffer.h"
      #include <stdio.h>
      #define UVD_HEVC_LEVEL_1   30
   #define UVD_HEVC_LEVEL_2   60
   #define UVD_HEVC_LEVEL_2_1 63
   #define UVD_HEVC_LEVEL_3   90
   #define UVD_HEVC_LEVEL_3_1 93
   #define UVD_HEVC_LEVEL_4   120
   #define UVD_HEVC_LEVEL_4_1 123
   #define UVD_HEVC_LEVEL_5   150
   #define UVD_HEVC_LEVEL_5_1 153
   #define UVD_HEVC_LEVEL_5_2 156
   #define UVD_HEVC_LEVEL_6   180
   #define UVD_HEVC_LEVEL_6_1 183
   #define UVD_HEVC_LEVEL_6_2 186
      static void radeon_uvd_enc_get_param(struct radeon_uvd_encoder *enc,
         {
      enc->enc_pic.picture_type = pic->picture_type;
   enc->enc_pic.frame_num = pic->frame_num;
   enc->enc_pic.pic_order_cnt = pic->pic_order_cnt;
   enc->enc_pic.pic_order_cnt_type = pic->pic_order_cnt_type;
   enc->enc_pic.not_referenced = pic->not_referenced;
   enc->enc_pic.is_iframe = (pic->picture_type == PIPE_H2645_ENC_PICTURE_TYPE_IDR) ||
            if (pic->seq.conformance_window_flag) {
         enc->enc_pic.crop_left = pic->seq.conf_win_left_offset;
   enc->enc_pic.crop_right = pic->seq.conf_win_right_offset;
   enc->enc_pic.crop_top = pic->seq.conf_win_top_offset;
   } else {
         enc->enc_pic.crop_left = 0;
   enc->enc_pic.crop_right = (align(enc->base.width, 16) - enc->base.width) / 2;
   enc->enc_pic.crop_top = 0;
            enc->enc_pic.general_tier_flag = pic->seq.general_tier_flag;
   enc->enc_pic.general_profile_idc = pic->seq.general_profile_idc;
   enc->enc_pic.general_level_idc = pic->seq.general_level_idc;
   enc->enc_pic.max_poc = MAX2(16, util_next_power_of_two(pic->seq.intra_period));
   enc->enc_pic.log2_max_poc = 0;
   for (int i = enc->enc_pic.max_poc; i != 0; enc->enc_pic.log2_max_poc++)
         enc->enc_pic.chroma_format_idc = pic->seq.chroma_format_idc;
   enc->enc_pic.pic_width_in_luma_samples = pic->seq.pic_width_in_luma_samples;
   enc->enc_pic.pic_height_in_luma_samples = pic->seq.pic_height_in_luma_samples;
   enc->enc_pic.log2_diff_max_min_luma_coding_block_size =
         enc->enc_pic.log2_min_transform_block_size_minus2 =
         enc->enc_pic.log2_diff_max_min_transform_block_size =
         enc->enc_pic.max_transform_hierarchy_depth_inter = pic->seq.max_transform_hierarchy_depth_inter;
   enc->enc_pic.max_transform_hierarchy_depth_intra = pic->seq.max_transform_hierarchy_depth_intra;
   enc->enc_pic.log2_parallel_merge_level_minus2 = pic->pic.log2_parallel_merge_level_minus2;
   enc->enc_pic.bit_depth_luma_minus8 = pic->seq.bit_depth_luma_minus8;
   enc->enc_pic.bit_depth_chroma_minus8 = pic->seq.bit_depth_chroma_minus8;
   enc->enc_pic.nal_unit_type = pic->pic.nal_unit_type;
   enc->enc_pic.max_num_merge_cand = pic->slice.max_num_merge_cand;
   enc->enc_pic.sample_adaptive_offset_enabled_flag = pic->seq.sample_adaptive_offset_enabled_flag;
   enc->enc_pic.pcm_enabled_flag = 0; /*HW not support PCM */
      }
      static void flush(struct radeon_uvd_encoder *enc)
   {
         }
      static void radeon_uvd_enc_flush(struct pipe_video_codec *encoder)
   {
      struct radeon_uvd_encoder *enc = (struct radeon_uvd_encoder *)encoder;
      }
      static void radeon_uvd_enc_cs_flush(void *ctx, unsigned flags, struct pipe_fence_handle **fence)
   {
         }
      static unsigned get_cpb_num(struct radeon_uvd_encoder *enc)
   {
      unsigned w = align(enc->base.width, 16) / 16;
   unsigned h = align(enc->base.height, 16) / 16;
            switch (enc->base.level) {
   case UVD_HEVC_LEVEL_1:
      dpb = 36864;
         case UVD_HEVC_LEVEL_2:
      dpb = 122880;
         case UVD_HEVC_LEVEL_2_1:
      dpb = 245760;
         case UVD_HEVC_LEVEL_3:
      dpb = 552960;
         case UVD_HEVC_LEVEL_3_1:
      dpb = 983040;
         case UVD_HEVC_LEVEL_4:
   case UVD_HEVC_LEVEL_4_1:
      dpb = 2228224;
         case UVD_HEVC_LEVEL_5:
   case UVD_HEVC_LEVEL_5_1:
   case UVD_HEVC_LEVEL_5_2:
      dpb = 8912896;
         case UVD_HEVC_LEVEL_6:
   case UVD_HEVC_LEVEL_6_1:
   case UVD_HEVC_LEVEL_6_2:
   default:
      dpb = 35651584;
                  }
      static void radeon_uvd_enc_begin_frame(struct pipe_video_codec *encoder,
               {
      struct radeon_uvd_encoder *enc = (struct radeon_uvd_encoder *)encoder;
                     enc->get_buffer(vid_buf->resources[0], &enc->handle, &enc->luma);
                     if (!enc->stream_handle) {
      struct rvid_buffer fb;
   enc->stream_handle = si_vid_alloc_stream_handle();
   enc->si = CALLOC_STRUCT(rvid_buffer);
   si_vid_create_buffer(enc->screen, enc->si, 128 * 1024, PIPE_USAGE_STAGING);
   si_vid_create_buffer(enc->screen, &fb, 4096, PIPE_USAGE_STAGING);
   enc->fb = &fb;
   enc->begin(enc, picture);
   flush(enc);
         }
      static void radeon_uvd_enc_encode_bitstream(struct pipe_video_codec *encoder,
               {
      struct radeon_uvd_encoder *enc = (struct radeon_uvd_encoder *)encoder;
   enc->get_buffer(destination, &enc->bs_handle, NULL);
                     if (!si_vid_create_buffer(enc->screen, enc->fb, 4096, PIPE_USAGE_STAGING)) {
      RVID_ERR("Can't create feedback buffer.\n");
               enc->need_feedback = true;
      }
      static void radeon_uvd_enc_end_frame(struct pipe_video_codec *encoder,
               {
      struct radeon_uvd_encoder *enc = (struct radeon_uvd_encoder *)encoder;
      }
      static void radeon_uvd_enc_destroy(struct pipe_video_codec *encoder)
   {
               if (enc->stream_handle) {
      struct rvid_buffer fb;
   enc->need_feedback = false;
   si_vid_create_buffer(enc->screen, &fb, 512, PIPE_USAGE_STAGING);
   enc->fb = &fb;
   enc->destroy(enc);
   flush(enc);
               si_vid_destroy_buffer(&enc->cpb);
   enc->ws->cs_destroy(&enc->cs);
      }
      static void radeon_uvd_enc_get_feedback(struct pipe_video_codec *encoder, void *feedback,
         {
      struct radeon_uvd_encoder *enc = (struct radeon_uvd_encoder *)encoder;
            if (NULL != size) {
      radeon_uvd_enc_feedback_t *fb_data = (radeon_uvd_enc_feedback_t *)enc->ws->buffer_map(
            if (!fb_data->status)
         else
                     si_vid_destroy_buffer(fb);
      }
      struct pipe_video_codec *radeon_uvd_create_encoder(struct pipe_context *context,
                     {
      struct si_screen *sscreen = (struct si_screen *)context->screen;
   struct si_context *sctx = (struct si_context *)context;
   struct radeon_uvd_encoder *enc;
   struct pipe_video_buffer *tmp_buf, templat = {};
   struct radeon_surf *tmp_surf;
            if (!si_radeon_uvd_enc_supported(sscreen)) {
      RVID_ERR("Unsupported UVD ENC fw version loaded!\n");
                        if (!enc)
            enc->base = *templ;
   enc->base.context = context;
   enc->base.destroy = radeon_uvd_enc_destroy;
   enc->base.begin_frame = radeon_uvd_enc_begin_frame;
   enc->base.encode_bitstream = radeon_uvd_enc_encode_bitstream;
   enc->base.end_frame = radeon_uvd_enc_end_frame;
   enc->base.flush = radeon_uvd_enc_flush;
   enc->base.get_feedback = radeon_uvd_enc_get_feedback;
   enc->get_buffer = get_buffer;
   enc->bits_in_shifter = 0;
   enc->screen = context->screen;
            if (!ws->cs_create(&enc->cs, sctx->ctx, AMD_IP_UVD_ENC, radeon_uvd_enc_cs_flush, enc)) {
      RVID_ERR("Can't get command submission context.\n");
               struct rvid_buffer si;
   si_vid_create_buffer(enc->screen, &si, 128 * 1024, PIPE_USAGE_STAGING);
            templat.buffer_format = PIPE_FORMAT_NV12;
   templat.width = enc->base.width;
   templat.height = enc->base.height;
            if (!(tmp_buf = context->create_video_buffer(context, &templat))) {
      RVID_ERR("Can't create video buffer.\n");
                        if (!enc->cpb_num)
                     cpb_size = (sscreen->info.gfx_level < GFX9)
               ? align(tmp_surf->u.legacy.level[0].nblk_x * tmp_surf->bpe, 128) *
            cpb_size = cpb_size * 3 / 2;
   cpb_size = cpb_size * enc->cpb_num;
            if (!si_vid_create_buffer(enc->screen, &enc->cpb, cpb_size, PIPE_USAGE_DEFAULT)) {
      RVID_ERR("Can't create CPB buffer.\n");
                              error:
                        FREE(enc);
      }
      bool si_radeon_uvd_enc_supported(struct si_screen *sscreen)
   {
         }
