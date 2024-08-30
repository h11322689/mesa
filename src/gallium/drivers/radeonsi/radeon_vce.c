   /**************************************************************************
   *
   * Copyright 2013 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   *
   **************************************************************************/
      #include "radeon_vce.h"
      #include "pipe/p_video_codec.h"
   #include "radeon_video.h"
   #include "radeonsi/si_pipe.h"
   #include "util/u_memory.h"
   #include "util/u_video.h"
   #include "vl/vl_video_buffer.h"
      #include <stdio.h>
      #define FW_40_2_2  ((40 << 24) | (2 << 16) | (2 << 8))
   #define FW_50_0_1  ((50 << 24) | (0 << 16) | (1 << 8))
   #define FW_50_1_2  ((50 << 24) | (1 << 16) | (2 << 8))
   #define FW_50_10_2 ((50 << 24) | (10 << 16) | (2 << 8))
   #define FW_50_17_3 ((50 << 24) | (17 << 16) | (3 << 8))
   #define FW_52_0_3  ((52 << 24) | (0 << 16) | (3 << 8))
   #define FW_52_4_3  ((52 << 24) | (4 << 16) | (3 << 8))
   #define FW_52_8_3  ((52 << 24) | (8 << 16) | (3 << 8))
   #define FW_53       (53 << 24)
      /**
   * flush commands to the hardware
   */
   static void flush(struct rvce_encoder *enc)
   {
      enc->ws->cs_flush(&enc->cs, PIPE_FLUSH_ASYNC, NULL);
   enc->task_info_idx = 0;
      }
      #if 0
   static void dump_feedback(struct rvce_encoder *enc, struct rvid_buffer *fb)
   {
      uint32_t *ptr = enc->ws->buffer_map(fb->res->buf, &enc->cs, PIPE_MAP_READ_WRITE);
   unsigned i = 0;
   fprintf(stderr, "\n");
   fprintf(stderr, "encStatus:\t\t\t%08x\n", ptr[i++]);
   fprintf(stderr, "encHasBitstream:\t\t%08x\n", ptr[i++]);
   fprintf(stderr, "encHasAudioBitstream:\t\t%08x\n", ptr[i++]);
   fprintf(stderr, "encBitstreamOffset:\t\t%08x\n", ptr[i++]);
   fprintf(stderr, "encBitstreamSize:\t\t%08x\n", ptr[i++]);
   fprintf(stderr, "encAudioBitstreamOffset:\t%08x\n", ptr[i++]);
   fprintf(stderr, "encAudioBitstreamSize:\t\t%08x\n", ptr[i++]);
   fprintf(stderr, "encExtrabytes:\t\t\t%08x\n", ptr[i++]);
   fprintf(stderr, "encAudioExtrabytes:\t\t%08x\n", ptr[i++]);
   fprintf(stderr, "videoTimeStamp:\t\t\t%08x\n", ptr[i++]);
   fprintf(stderr, "audioTimeStamp:\t\t\t%08x\n", ptr[i++]);
   fprintf(stderr, "videoOutputType:\t\t%08x\n", ptr[i++]);
   fprintf(stderr, "attributeFlags:\t\t\t%08x\n", ptr[i++]);
   fprintf(stderr, "seiPrivatePackageOffset:\t%08x\n", ptr[i++]);
   fprintf(stderr, "seiPrivatePackageSize:\t\t%08x\n", ptr[i++]);
   fprintf(stderr, "\n");
      }
   #endif
      /**
   * reset the CPB handling
   */
   static void reset_cpb(struct rvce_encoder *enc)
   {
               list_inithead(&enc->cpb_slots);
   for (i = 0; i < enc->cpb_num; ++i) {
      struct rvce_cpb_slot *slot = &enc->cpb_array[i];
   slot->index = i;
   slot->picture_type = PIPE_H2645_ENC_PICTURE_TYPE_SKIP;
   slot->frame_num = 0;
   slot->pic_order_cnt = 0;
         }
      /**
   * sort l0 and l1 to the top of the list
   */
   static void sort_cpb(struct rvce_encoder *enc)
   {
               LIST_FOR_EACH_ENTRY (i, &enc->cpb_slots, list) {
      if (i->frame_num == enc->pic.ref_idx_l0_list[0])
            if (i->frame_num == enc->pic.ref_idx_l1_list[0])
            if (enc->pic.picture_type == PIPE_H2645_ENC_PICTURE_TYPE_P && l0)
            if (enc->pic.picture_type == PIPE_H2645_ENC_PICTURE_TYPE_B && l0 && l1)
               if (l1) {
      list_del(&l1->list);
               if (l0) {
      list_del(&l0->list);
         }
      /**
   * get number of cpbs based on dpb
   */
   static unsigned get_cpb_num(struct rvce_encoder *enc)
   {
      unsigned w = align(enc->base.width, 16) / 16;
   unsigned h = align(enc->base.height, 16) / 16;
            switch (enc->base.level) {
   case 10:
      dpb = 396;
      case 11:
      dpb = 900;
      case 12:
   case 13:
   case 20:
      dpb = 2376;
      case 21:
      dpb = 4752;
      case 22:
   case 30:
      dpb = 8100;
      case 31:
      dpb = 18000;
      case 32:
      dpb = 20480;
      case 40:
   case 41:
      dpb = 32768;
      case 42:
      dpb = 34816;
      case 50:
      dpb = 110400;
      default:
   case 51:
   case 52:
      dpb = 184320;
                  }
      /**
   * Get the slot for the currently encoded frame
   */
   struct rvce_cpb_slot *si_current_slot(struct rvce_encoder *enc)
   {
         }
      /**
   * Get the slot for L0
   */
   struct rvce_cpb_slot *si_l0_slot(struct rvce_encoder *enc)
   {
         }
      /**
   * Get the slot for L1
   */
   struct rvce_cpb_slot *si_l1_slot(struct rvce_encoder *enc)
   {
         }
      /**
   * Calculate the offsets into the CPB
   */
   void si_vce_frame_offset(struct rvce_encoder *enc, struct rvce_cpb_slot *slot, signed *luma_offset,
         {
      struct si_screen *sscreen = (struct si_screen *)enc->screen;
            if (sscreen->info.gfx_level < GFX9) {
      pitch = align(enc->luma->u.legacy.level[0].nblk_x * enc->luma->bpe, 128);
      } else {
      pitch = align(enc->luma->u.gfx9.surf_pitch * enc->luma->bpe, 256);
      }
            *luma_offset = slot->index * fsize;
      }
      /**
   * destroy this video encoder
   */
   static void rvce_destroy(struct pipe_video_codec *encoder)
   {
      struct rvce_encoder *enc = (struct rvce_encoder *)encoder;
   if (enc->stream_handle) {
      struct rvid_buffer fb;
   si_vid_create_buffer(enc->screen, &fb, 512, PIPE_USAGE_STAGING);
   enc->fb = &fb;
   enc->session(enc);
   enc->destroy(enc);
   flush(enc);
      }
   si_vid_destroy_buffer(&enc->cpb);
   enc->ws->cs_destroy(&enc->cs);
   FREE(enc->cpb_array);
      }
      static void rvce_begin_frame(struct pipe_video_codec *encoder, struct pipe_video_buffer *source,
         {
      struct rvce_encoder *enc = (struct rvce_encoder *)encoder;
   struct vl_video_buffer *vid_buf = (struct vl_video_buffer *)source;
            bool need_rate_control =
      enc->pic.rate_ctrl[0].rate_ctrl_method != pic->rate_ctrl[0].rate_ctrl_method ||
   enc->pic.quant_i_frames != pic->quant_i_frames ||
   enc->pic.quant_p_frames != pic->quant_p_frames ||
   enc->pic.quant_b_frames != pic->quant_b_frames ||
   enc->pic.rate_ctrl[0].target_bitrate != pic->rate_ctrl[0].target_bitrate ||
   enc->pic.rate_ctrl[0].frame_rate_num != pic->rate_ctrl[0].frame_rate_num ||
         enc->pic = *pic;
            enc->get_buffer(vid_buf->resources[0], &enc->handle, &enc->luma);
            if (pic->picture_type == PIPE_H2645_ENC_PICTURE_TYPE_IDR)
         else if (pic->picture_type == PIPE_H2645_ENC_PICTURE_TYPE_P ||
                  if (!enc->stream_handle) {
      struct rvid_buffer fb;
   enc->stream_handle = si_vid_alloc_stream_handle();
   si_vid_create_buffer(enc->screen, &fb, 512, PIPE_USAGE_STAGING);
   enc->fb = &fb;
   enc->session(enc);
   enc->create(enc);
   enc->config(enc);
   enc->feedback(enc);
   flush(enc);
   // dump_feedback(enc, &fb);
   si_vid_destroy_buffer(&fb);
               if (need_rate_control) {
      enc->session(enc);
   enc->config(enc);
         }
      static void rvce_encode_bitstream(struct pipe_video_codec *encoder,
               {
      struct rvce_encoder *enc = (struct rvce_encoder *)encoder;
   enc->get_buffer(destination, &enc->bs_handle, NULL);
            *fb = enc->fb = CALLOC_STRUCT(rvid_buffer);
   if (!si_vid_create_buffer(enc->screen, enc->fb, 512, PIPE_USAGE_STAGING)) {
      RVID_ERR("Can't create feedback buffer.\n");
      }
   if (!radeon_emitted(&enc->cs, 0))
         enc->encode(enc);
      }
      static void rvce_end_frame(struct pipe_video_codec *encoder, struct pipe_video_buffer *source,
         {
      struct rvce_encoder *enc = (struct rvce_encoder *)encoder;
            if (!enc->dual_inst || enc->bs_idx > 1)
            /* update the CPB backtrack with the just encoded frame */
   slot->picture_type = enc->pic.picture_type;
   slot->frame_num = enc->pic.frame_num;
   slot->pic_order_cnt = enc->pic.pic_order_cnt;
   if (!enc->pic.not_referenced) {
      list_del(&slot->list);
         }
      static void rvce_get_feedback(struct pipe_video_codec *encoder, void *feedback, unsigned *size)
   {
      struct rvce_encoder *enc = (struct rvce_encoder *)encoder;
            if (size) {
      uint32_t *ptr = enc->ws->buffer_map(enc->ws, fb->res->buf, &enc->cs,
            if (ptr[1]) {
         } else {
                     }
   // dump_feedback(enc, fb);
   si_vid_destroy_buffer(fb);
      }
      /**
   * flush any outstanding command buffers to the hardware
   */
   static void rvce_flush(struct pipe_video_codec *encoder)
   {
                  }
      static void rvce_cs_flush(void *ctx, unsigned flags, struct pipe_fence_handle **fence)
   {
         }
      struct pipe_video_codec *si_vce_create_encoder(struct pipe_context *context,
               {
      struct si_screen *sscreen = (struct si_screen *)context->screen;
   struct si_context *sctx = (struct si_context *)context;
   struct rvce_encoder *enc;
   struct pipe_video_buffer *tmp_buf, templat = {};
   struct radeon_surf *tmp_surf;
            if (!sscreen->info.vce_fw_version) {
      RVID_ERR("Kernel doesn't supports VCE!\n");
         } else if (!si_vce_is_fw_version_supported(sscreen)) {
      RVID_ERR("Unsupported VCE fw version loaded!\n");
               enc = CALLOC_STRUCT(rvce_encoder);
   if (!enc)
            if (sscreen->info.is_amdgpu)
                     if (sscreen->info.family >= CHIP_TONGA && sscreen->info.family != CHIP_STONEY &&
      sscreen->info.family != CHIP_POLARIS11 && sscreen->info.family != CHIP_POLARIS12 &&
   sscreen->info.family != CHIP_VEGAM)
      /* TODO enable B frame with dual instance */
   if ((sscreen->info.family >= CHIP_TONGA) && (templ->max_references == 1) &&
      (sscreen->info.vce_harvest_config == 0))
         enc->base = *templ;
            enc->base.destroy = rvce_destroy;
   enc->base.begin_frame = rvce_begin_frame;
   enc->base.encode_bitstream = rvce_encode_bitstream;
   enc->base.end_frame = rvce_end_frame;
   enc->base.flush = rvce_flush;
   enc->base.get_feedback = rvce_get_feedback;
            enc->screen = context->screen;
            if (!ws->cs_create(&enc->cs, sctx->ctx, AMD_IP_VCE, rvce_cs_flush, enc)) {
      RVID_ERR("Can't get command submission context.\n");
               templat.buffer_format = PIPE_FORMAT_NV12;
   templat.width = enc->base.width;
   templat.height = enc->base.height;
   templat.interlaced = false;
   if (!(tmp_buf = context->create_video_buffer(context, &templat))) {
      RVID_ERR("Can't create video buffer.\n");
               enc->cpb_num = get_cpb_num(enc);
   if (!enc->cpb_num)
                     cpb_size = (sscreen->info.gfx_level < GFX9)
                                       cpb_size = cpb_size * 3 / 2;
   cpb_size = cpb_size * enc->cpb_num;
   if (enc->dual_pipe)
         tmp_buf->destroy(tmp_buf);
   if (!si_vid_create_buffer(enc->screen, &enc->cpb, cpb_size, PIPE_USAGE_DEFAULT)) {
      RVID_ERR("Can't create CPB buffer.\n");
               enc->cpb_array = CALLOC(enc->cpb_num, sizeof(struct rvce_cpb_slot));
   if (!enc->cpb_array)
                     switch (sscreen->info.vce_fw_version) {
   case FW_40_2_2:
      si_vce_40_2_2_init(enc);
         case FW_50_0_1:
   case FW_50_1_2:
   case FW_50_10_2:
   case FW_50_17_3:
      si_vce_50_init(enc);
         case FW_52_0_3:
   case FW_52_4_3:
   case FW_52_8_3:
      si_vce_52_init(enc);
         default:
      if ((sscreen->info.vce_fw_version & (0xff << 24)) >= FW_53) {
         } else
                     error:
                        FREE(enc->cpb_array);
   FREE(enc);
      }
      /**
   * check if kernel has the right fw version loaded
   */
   bool si_vce_is_fw_version_supported(struct si_screen *sscreen)
   {
      switch (sscreen->info.vce_fw_version) {
   case FW_40_2_2:
   case FW_50_0_1:
   case FW_50_1_2:
   case FW_50_10_2:
   case FW_50_17_3:
   case FW_52_0_3:
   case FW_52_4_3:
   case FW_52_8_3:
         default:
      if ((sscreen->info.vce_fw_version & (0xff << 24)) >= FW_53)
         else
         }
      /**
   * Add the buffer as relocation to the current command submission
   */
   void si_vce_add_buffer(struct rvce_encoder *enc, struct pb_buffer *buf, unsigned usage,
         {
               reloc_idx = enc->ws->cs_add_buffer(&enc->cs, buf, usage | RADEON_USAGE_SYNCHRONIZED, domain);
   if (enc->use_vm) {
      uint64_t addr;
   addr = enc->ws->buffer_get_virtual_address(buf);
   addr = addr + offset;
   RVCE_CS(addr >> 32);
      } else {
      offset += enc->ws->buffer_get_reloc_offset(buf);
   RVCE_CS(reloc_idx * 4);
         }
