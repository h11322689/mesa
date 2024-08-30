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
      /*
   * Authors:
   *      Christian König <christian.koenig@amd.com>
   *
   */
      #include "pipe/p_video_codec.h"
   #include "util/vl_vlc.h"
   #include "vl/vl_zscan.h"
      #include "vid_dec.h"
      static uint8_t default_intra_matrix[64] = {
      8, 16, 19, 22, 26, 27, 29, 34,
   16, 16, 22, 24, 27, 29, 34, 37,
   19, 22, 26, 27, 29, 34, 34, 38,
   22, 22, 26, 27, 29, 34, 37, 40,
   22, 26, 27, 29, 32, 35, 40, 48,
   26, 27, 29, 32, 35, 40, 48, 58,
   26, 27, 29, 34, 38, 46, 56, 69,
      };
      static uint8_t default_non_intra_matrix[64] = {
      16, 16, 16, 16, 16, 16, 16, 16,
   16, 16, 16, 16, 16, 16, 16, 16,
   16, 16, 16, 16, 16, 16, 16, 16,
   16, 16, 16, 16, 16, 16, 16, 16,
   16, 16, 16, 16, 16, 16, 16, 16,
   16, 16, 16, 16, 16, 16, 16, 16,
   16, 16, 16, 16, 16, 16, 16, 16,
      };
      static void vid_dec_mpeg12_Decode(vid_dec_PrivateType *priv, struct vl_vlc *vlc, unsigned min_bits_left);
   static void vid_dec_mpeg12_EndFrame(vid_dec_PrivateType *priv);
   static struct pipe_video_buffer *vid_dec_mpeg12_Flush(vid_dec_PrivateType *priv, OMX_TICKS *timestamp);
      void vid_dec_mpeg12_Init(vid_dec_PrivateType *priv)
   {
      struct pipe_video_codec templat = {};
            port = (omx_base_video_PortType *)priv->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
   templat.profile = priv->profile;
   templat.entrypoint = PIPE_VIDEO_ENTRYPOINT_BITSTREAM;
   templat.chroma_format = PIPE_VIDEO_CHROMA_FORMAT_420;
   templat.max_references = 2;
   templat.expect_chunked_decode = true;
   templat.width = port->sPortParam.format.video.nFrameWidth;
                     priv->picture.base.profile = PIPE_VIDEO_PROFILE_MPEG2_MAIN;
   priv->picture.mpeg12.intra_matrix = default_intra_matrix;
            priv->Decode = vid_dec_mpeg12_Decode;
   priv->EndFrame = vid_dec_mpeg12_EndFrame;
      }
      static void BeginFrame(vid_dec_PrivateType *priv)
   {
      if (priv->picture.mpeg12.picture_coding_type != PIPE_MPEG12_PICTURE_CODING_TYPE_B) {
      priv->picture.mpeg12.ref[0] = priv->picture.mpeg12.ref[1];
               if (priv->target == priv->picture.mpeg12.ref[0]) {
      struct pipe_video_buffer *tmp = priv->target;
   priv->target = priv->shadow;
                        priv->codec->begin_frame(priv->codec, priv->target, &priv->picture.base);
      }
      static void vid_dec_mpeg12_EndFrame(vid_dec_PrivateType *priv)
   {
               priv->codec->end_frame(priv->codec, priv->target, &priv->picture.base);
                        priv->picture.mpeg12.ref[1] = priv->target;
   done = priv->picture.mpeg12.ref[0];
   if (!done) {
      priv->target = NULL;
            } else
            priv->frame_finished = true;
   priv->target = priv->in_buffers[0]->pInputPortPrivate;
      }
      static struct pipe_video_buffer *vid_dec_mpeg12_Flush(vid_dec_PrivateType *priv, OMX_TICKS *timestamp)
   {
      struct pipe_video_buffer *result = priv->picture.mpeg12.ref[1];
   priv->picture.mpeg12.ref[1] = NULL;
   if (timestamp)
            }
      static void vid_dec_mpeg12_Decode(vid_dec_PrivateType *priv, struct vl_vlc *vlc, unsigned min_bits_left)
   {
      uint8_t code;
            if (!vl_vlc_search_byte(vlc, vl_vlc_bits_left(vlc) - min_bits_left, 0x00))
            if (vl_vlc_peekbits(vlc, 24) != 0x000001) {
      vl_vlc_eatbits(vlc, 8);
               if (priv->slice) {
      unsigned bytes = priv->bytes_left - (vl_vlc_bits_left(vlc) / 8);
   priv->codec->decode_bitstream(priv->codec, priv->target, &priv->picture.base,
                     vl_vlc_eatbits(vlc, 24);
            if (priv->frame_started && (code == 0x00 || code > 0xAF))
            if (code == 0xB3) {
      /* sequence header code */
            /* horizontal_size_value */
            /* vertical_size_value */
            /* aspect_ratio_information */
            /* frame_rate_code */
                     /* bit_rate_value */
            /* marker_bit */
            /* vbv_buffer_size_value */
            /* constrained_parameters_flag */
                     /* load_intra_quantiser_matrix */
   if (vl_vlc_get_uimsbf(vlc, 1)) {
      /* intra_quantiser_matrix */
   priv->picture.mpeg12.intra_matrix = priv->codec_data.mpeg12.intra_matrix;
   for (i = 0; i < 64; ++i) {
      priv->codec_data.mpeg12.intra_matrix[vl_zscan_normal[i]] = vl_vlc_get_uimsbf(vlc, 8);
         } else
            /* load_non_intra_quantiser_matrix */
   if (vl_vlc_get_uimsbf(vlc, 1)) {
      /* non_intra_quantiser_matrix */
   priv->picture.mpeg12.non_intra_matrix = priv->codec_data.mpeg12.non_intra_matrix;
   for (i = 0; i < 64; ++i) {
      priv->codec_data.mpeg12.non_intra_matrix[i] = vl_vlc_get_uimsbf(vlc, 8);
         } else
         } else if (code == 0x00) {
      /* picture start code */
            /* temporal_reference */
                     /* vbv_delay */
            vl_vlc_fillbits(vlc);
   if (priv->picture.mpeg12.picture_coding_type == 2 ||
      priv->picture.mpeg12.picture_coding_type == 3) {
   priv->picture.mpeg12.full_pel_forward_vector = vl_vlc_get_uimsbf(vlc, 1);
   /* forward_f_code */
   priv->picture.mpeg12.f_code[0][0] = vl_vlc_get_uimsbf(vlc, 3) - 1;
      } else {
      priv->picture.mpeg12.full_pel_forward_vector = 0;
               if (priv->picture.mpeg12.picture_coding_type == 3) {
      priv->picture.mpeg12.full_pel_backward_vector = vl_vlc_get_uimsbf(vlc, 1);
   /* backward_f_code */
   priv->picture.mpeg12.f_code[1][0] = vl_vlc_get_uimsbf(vlc, 3) - 1;
      } else {
      priv->picture.mpeg12.full_pel_backward_vector = 0;
               /* extra_bit_picture */
   while (vl_vlc_get_uimsbf(vlc, 1)) {
      /* extra_information_picture */
   vl_vlc_get_uimsbf(vlc, 8);
            } else if (code == 0xB5) {
      /* extension start code */
            /* extension_start_code_identifier */
   switch (vl_vlc_get_uimsbf(vlc, 4)) {
               /* load_intra_quantiser_matrix */
   if (vl_vlc_get_uimsbf(vlc, 1)) {
      /* intra_quantiser_matrix */
   priv->picture.mpeg12.intra_matrix = priv->codec_data.mpeg12.intra_matrix;
   for (i = 0; i < 64; ++i) {
      priv->codec_data.mpeg12.intra_matrix[vl_zscan_normal[i]] = vl_vlc_get_uimsbf(vlc, 8);
                        /* load_non_intra_quantiser_matrix */
   if (vl_vlc_get_uimsbf(vlc, 1)) {
      /* non_intra_quantiser_matrix */
   priv->picture.mpeg12.non_intra_matrix = priv->codec_data.mpeg12.non_intra_matrix;
   for (i = 0; i < 64; ++i) {
      priv->codec_data.mpeg12.non_intra_matrix[i] = vl_vlc_get_uimsbf(vlc, 8);
                                          priv->picture.mpeg12.f_code[0][0] = vl_vlc_get_uimsbf(vlc, 4) - 1;
   priv->picture.mpeg12.f_code[0][1] = vl_vlc_get_uimsbf(vlc, 4) - 1;
   priv->picture.mpeg12.f_code[1][0] = vl_vlc_get_uimsbf(vlc, 4) - 1;
   priv->picture.mpeg12.f_code[1][1] = vl_vlc_get_uimsbf(vlc, 4) - 1;
   priv->picture.mpeg12.intra_dc_precision = vl_vlc_get_uimsbf(vlc, 2);
   priv->picture.mpeg12.picture_structure = vl_vlc_get_uimsbf(vlc, 2);
   priv->picture.mpeg12.top_field_first = vl_vlc_get_uimsbf(vlc, 1);
   priv->picture.mpeg12.frame_pred_frame_dct = vl_vlc_get_uimsbf(vlc, 1);
   priv->picture.mpeg12.concealment_motion_vectors = vl_vlc_get_uimsbf(vlc, 1);
   priv->picture.mpeg12.q_scale_type = vl_vlc_get_uimsbf(vlc, 1);
                                                                                                                                                      /* sub_carrier_phase */
      }
            } else if (code <= 0xAF) {
      /* slice start */
   unsigned bytes = (vl_vlc_valid_bits(vlc) / 8) + 4;
   uint8_t buf[12];
   const void *ptr = buf;
            if (!priv->frame_started)
            buf[0] = 0x00;
   buf[1] = 0x00;
   buf[2] = 0x01;
   buf[3] = code;
   for (i = 4; i < bytes; ++i)
            priv->codec->decode_bitstream(priv->codec, priv->target, &priv->picture.base,
            priv->bytes_left = vl_vlc_bits_left(vlc) / 8;
         } else if (code == 0xB2) {
            } else if (code == 0xB4) {
         } else if (code == 0xB7) {
         } else if (code == 0xB8) {
         } else if (code >= 0xB9) {
         } else {
                  /* resync to byte boundary */
      }
