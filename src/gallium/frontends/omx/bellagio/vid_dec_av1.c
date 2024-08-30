   /**************************************************************************
   *
   * Copyright 2020 Advanced Micro Devices, Inc.
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
   #include "vl/vl_video_buffer.h"
      #include "vl/vl_codec.h"
      #include "entrypoint.h"
   #include "vid_dec.h"
   #include "vid_dec_av1.h"
      static unsigned av1_f(struct vl_vlc *vlc, unsigned n)
   {
               if (n == 0)
            if (valid < 32)
               }
      static unsigned av1_uvlc(struct vl_vlc *vlc)
   {
      unsigned value;
            while (1) {
      bool done = av1_f(vlc, 1);
   if (done)
                     if (leadingZeros >= 32)
                        }
      static int av1_le(struct vl_vlc *vlc, const unsigned n)
   {
      unsigned byte, t = 0;
            for (i = 0; i < n; ++i) {
      byte = av1_f(vlc, 8);
                  }
      static unsigned av1_uleb128(struct vl_vlc *vlc)
   {
      unsigned value = 0;
   unsigned leb128Bytes = 0;
            for (i = 0; i < 8; ++i) {
      leb128Bytes = av1_f(vlc, 8);
   value |= ((leb128Bytes & 0x7f) << (i * 7));
   if (!(leb128Bytes & 0x80))
                  }
      static int av1_su(struct vl_vlc *vlc, const unsigned n)
   {
      unsigned value = av1_f(vlc, n);
            if (value && signMask)
               }
      static unsigned FloorLog2(unsigned x)
   {
      unsigned s = 0;
            while (x1 != 0) {
      x1 = x1 >> 1;
                  }
      static unsigned av1_ns(struct vl_vlc *vlc, unsigned n)
   {
      unsigned w = FloorLog2(n) + 1;
   unsigned m = (1 << w) - n;
            if (v < m)
                        }
      static void av1_byte_alignment(struct vl_vlc *vlc)
   {
         }
      static void sequence_header_obu(vid_dec_PrivateType *priv, struct vl_vlc *vlc)
   {
      struct av1_sequence_header_obu *seq = &(priv->codec_data.av1.seq);
   bool timing_info_present_flag;
   bool initial_display_delay_present_flag;
   uint8_t seq_level_idx;
   bool initial_display_delay_present_for_this_op;
   bool high_bitdepth;
   bool twelve_bit;
   bool color_description_present_flag;
   uint8_t color_primaries;
   uint8_t transfer_characteristics;
   uint8_t matrix_coefficients;
            seq->seq_profile = av1_f(vlc, 3);
            av1_f(vlc, 1); /* still_picture */
   seq->reduced_still_picture_header = av1_f(vlc, 1);
   if (seq->reduced_still_picture_header) {
      timing_info_present_flag = 0;
   seq->decoder_model_info_present_flag = 0;
   initial_display_delay_present_flag = 0;
   seq->operating_points_cnt_minus_1 = 0;
   seq->operating_point_idc[0] = 0;
   seq_level_idx = av1_f(vlc, 5);
   seq->decoder_model_present_for_this_op[0] = 0;
      } else {
               timing_info_present_flag = av1_f(vlc, 1);
   if (timing_info_present_flag) {
      av1_f(vlc, 32); /* num_units_in_display_tick */
   av1_f(vlc, 32); /* time_scale */
   seq->timing_info.equal_picture_interval = av1_f(vlc, 1);
                  seq->decoder_model_info_present_flag = av1_f(vlc, 1);
   if (seq->decoder_model_info_present_flag) {
      /* decoder_model_info */
   buffer_delay_length_minus_1 = av1_f(vlc, 5);
   seq->decoder_model_info.num_units_in_decoding_tick = av1_f(vlc, 32);
   seq->decoder_model_info.buffer_removal_time_length_minus_1 = av1_f(vlc, 5);
         } else {
                  initial_display_delay_present_flag = av1_f(vlc, 1);
   seq->operating_points_cnt_minus_1 = av1_f(vlc, 5);
   for (i = 0; i < seq->operating_points_cnt_minus_1 + 1; ++i) {
      seq->operating_point_idc[i] = av1_f(vlc, 12);
   seq_level_idx = av1_f(vlc, 5);
                  if (seq->decoder_model_info_present_flag) {
      seq->decoder_model_present_for_this_op[i] = av1_f(vlc, 1);
   if (seq->decoder_model_present_for_this_op[i]) {
      uint8_t n = buffer_delay_length_minus_1 + 1;
   av1_f(vlc, n); /* decoder_buffer_delay */
   av1_f(vlc, n); /* encoder_buffer_delay */
         } else {
                  if (initial_display_delay_present_flag) {
      initial_display_delay_present_for_this_op = av1_f(vlc, 1);
   if (initial_display_delay_present_for_this_op)
                     seq->frame_width_bits_minus_1 = av1_f(vlc, 4);
   seq->frame_height_bits_minus_1 = av1_f(vlc, 4);
   seq->max_frame_width_minus_1 = av1_f(vlc, seq->frame_width_bits_minus_1 + 1);
            if (seq->reduced_still_picture_header)
         else
         if (seq->frame_id_numbers_present_flag) {
      seq->delta_frame_id_length_minus_2 = av1_f(vlc, 4);
               seq->use_128x128_superblock = av1_f(vlc, 1);
   seq->enable_filter_intra = av1_f(vlc, 1);
   seq->enable_intra_edge_filter = av1_f(vlc, 1);
   if (seq->reduced_still_picture_header) {
      seq->enable_interintra_compound = 0;
   seq->enable_masked_compound = 0;
   seq->enable_warped_motion = 0;
   seq->enable_dual_filter = 0;
   seq->enable_order_hint = 0;
   seq->enable_jnt_comp = 0;
   seq->enable_ref_frame_mvs = 0;
   seq->seq_force_screen_content_tools = AV1_SELECT_SCREEN_CONTENT_TOOLS;
   seq->seq_force_integer_mv = AV1_SELECT_INTEGER_MV;
   } else {
         bool seq_choose_screen_content_tools;
   seq->enable_interintra_compound = av1_f(vlc, 1);
   seq->enable_masked_compound = av1_f(vlc, 1);
   seq->enable_warped_motion = av1_f(vlc, 1);
   seq->enable_dual_filter = av1_f(vlc, 1);
   seq->enable_order_hint = av1_f(vlc, 1);
   if (seq->enable_order_hint) {
      seq->enable_jnt_comp = av1_f(vlc, 1);
      } else {
      seq->enable_jnt_comp = 0;
               seq_choose_screen_content_tools = av1_f(vlc, 1);
   seq->seq_force_screen_content_tools =
            if (seq->seq_force_screen_content_tools > 0) {
      bool seq_choose_integer_mv = av1_f(vlc, 1);
   seq->seq_force_integer_mv =
      } else {
                  if (seq->enable_order_hint) {
      seq->order_hint_bits_minus_1 = av1_f(vlc, 3);
      } else {
                     seq->enable_superres = av1_f(vlc, 1);
   seq->enable_cdef = av1_f(vlc, 1);
            high_bitdepth = av1_f(vlc, 1);
   if (seq->seq_profile == 2 && high_bitdepth) {
      twelve_bit = av1_f(vlc, 1);
      } else if (seq->seq_profile <= 2) {
                  seq->color_config.mono_chrome = (seq->seq_profile == 1) ? 0 : av1_f(vlc, 1);
            color_description_present_flag = av1_f(vlc, 1);
   if (color_description_present_flag) {
      color_primaries = av1_f(vlc, 8);
   transfer_characteristics = av1_f(vlc, 8);
      } else {
      color_primaries = AV1_CP_UNSPECIFIED;
   transfer_characteristics = AV1_TC_UNSPECIFIED;
               if (seq->color_config.mono_chrome) {
      av1_f(vlc, 1); /* color_range */
   seq->color_config.subsampling_x = 1;
   seq->color_config.subsampling_y = 1;
      } else if (color_primaries == AV1_CP_BT_709 &&
            transfer_characteristics == AV1_TC_SRGB &&
   seq->color_config.subsampling_x = 0;
      } else {
      av1_f(vlc, 1); /* color_range */
   if (seq->seq_profile == 0) {
      seq->color_config.subsampling_x = 1;
      } else if (seq->seq_profile == 1 ) {
      seq->color_config.subsampling_x = 0;
      } else {
      if (seq->color_config.BitDepth == 12) {
      seq->color_config.subsampling_x = av1_f(vlc, 1);
   if (seq->color_config.subsampling_x)
         else
      } else {
      seq->color_config.subsampling_x = 1;
         }
   if (seq->color_config.subsampling_x && seq->color_config.subsampling_y)
      }
   if (!seq->color_config.mono_chrome)
                     priv->picture.av1.picture_parameter.profile = seq->seq_profile;
   priv->picture.av1.picture_parameter.seq_info_fields.use_128x128_superblock =
         priv->picture.av1.picture_parameter.seq_info_fields.enable_filter_intra =
         priv->picture.av1.picture_parameter.seq_info_fields.enable_intra_edge_filter =
         priv->picture.av1.picture_parameter.order_hint_bits_minus_1 =
         priv->picture.av1.picture_parameter.max_width = seq->max_frame_width_minus_1 + 1;
   priv->picture.av1.picture_parameter.max_height = seq->max_frame_height_minus_1 + 1;
   priv->picture.av1.picture_parameter.seq_info_fields.enable_interintra_compound =
         priv->picture.av1.picture_parameter.seq_info_fields.enable_masked_compound =
         priv->picture.av1.picture_parameter.seq_info_fields.enable_dual_filter =
         priv->picture.av1.picture_parameter.seq_info_fields.enable_order_hint =
         priv->picture.av1.picture_parameter.seq_info_fields.enable_jnt_comp =
         priv->picture.av1.picture_parameter.seq_info_fields.ref_frame_mvs =
         priv->picture.av1.picture_parameter.bit_depth_idx =
         priv->picture.av1.picture_parameter.seq_info_fields.mono_chrome =
      }
      static void superres_params(vid_dec_PrivateType *priv, struct vl_vlc *vlc)
   {
      struct av1_sequence_header_obu *seq = &(priv->codec_data.av1.seq);
   struct av1_uncompressed_header_obu *hdr = &(priv->codec_data.av1.uncompressed_header);
            if (seq->enable_superres)
         else
            if (hdr->use_superres) {
      coded_denom = av1_f(vlc, 3 /* SUPERRES_DENOM_BITS */);
      } else {
                  hdr->UpscaledWidth = hdr->FrameWidth;
   hdr->FrameWidth = (hdr->UpscaledWidth * 8 + (hdr->SuperresDenom / 2)) /
      }
      static void compute_image_size(vid_dec_PrivateType *priv)
   {
               hdr->MiCols = 2 * ((hdr->FrameWidth + 7) >> 3);
      }
      static void frame_size(vid_dec_PrivateType *priv, struct vl_vlc *vlc)
   {
      struct av1_sequence_header_obu *seq = &(priv->codec_data.av1.seq);
   struct av1_uncompressed_header_obu *hdr = &(priv->codec_data.av1.uncompressed_header);
   unsigned frame_width_minus_1;
            if (hdr->frame_size_override_flag) {
      frame_width_minus_1 = av1_f(vlc, seq->frame_width_bits_minus_1 + 1);
   frame_height_minus_1 = av1_f(vlc, seq->frame_height_bits_minus_1 + 1);
   hdr->FrameWidth = frame_width_minus_1 + 1;
      } else {
      hdr->FrameWidth = seq->max_frame_width_minus_1 + 1;
               superres_params(priv, vlc);
      }
      static void render_size(vid_dec_PrivateType *priv, struct vl_vlc *vlc)
   {
      struct av1_uncompressed_header_obu *hdr = &(priv->codec_data.av1.uncompressed_header);
   bool render_and_frame_size_different;
   unsigned render_width_minus_1;
            render_and_frame_size_different = av1_f(vlc, 1);
   if (render_and_frame_size_different) {
      render_width_minus_1 = av1_f(vlc, 16);
   render_height_minus_1 = av1_f(vlc, 16);
   hdr->RenderWidth = render_width_minus_1 + 1;
      } else {
      hdr->RenderWidth = hdr->UpscaledWidth;
         }
      static int get_relative_dist(vid_dec_PrivateType *priv, int a, int b)
   {
      struct av1_sequence_header_obu *seq = &(priv->codec_data.av1.seq);
   int diff;
            if (!seq->enable_order_hint)
            diff = a - b;
   m = 1 << (seq->OrderHintBits - 1);
               }
      static uint8_t find_latest_backward(vid_dec_PrivateType *priv)
   {
      struct av1_uncompressed_header_obu *hdr = &(priv->codec_data.av1.uncompressed_header);
   uint8_t ref = 0xff;
   unsigned latestOrderHint = 0;
            for (i = 0; i < AV1_NUM_REF_FRAMES; ++i) {
      unsigned hint = hdr->shiftedOrderHints[i];
   if (!hdr->usedFrame[i] &&
      hint >= hdr->curFrameHint &&
   (ref == 0xff || hint >= latestOrderHint)) {
   ref = i;
                     }
      static uint8_t find_earliest_backward(vid_dec_PrivateType *priv)
   {
      struct av1_uncompressed_header_obu *hdr = &(priv->codec_data.av1.uncompressed_header);
   uint8_t ref = 0xff;
   unsigned earliestOrderHint = 0;
            for (i = 0; i < AV1_NUM_REF_FRAMES; ++i) {
      unsigned hint = hdr->shiftedOrderHints[i];
   if (!hdr->usedFrame[i] &&
      hint >= hdr->curFrameHint &&
   (ref == 0xff || hint < earliestOrderHint)) {
   ref = i;
                     }
      static uint8_t find_latest_forward(vid_dec_PrivateType *priv)
   {
      struct av1_uncompressed_header_obu *hdr = &(priv->codec_data.av1.uncompressed_header);
   uint8_t ref = 0xff;
   unsigned latestOrderHint = 0;
            for (i = 0; i < AV1_NUM_REF_FRAMES; ++i) {
      unsigned hint = hdr->shiftedOrderHints[i];
   if (!hdr->usedFrame[i] &&
      hint < hdr->curFrameHint &&
   (ref == 0xff || hint >= latestOrderHint)) {
   ref = i;
                     }
      static void set_frame_refs(vid_dec_PrivateType *priv, struct vl_vlc *vlc)
   {
      struct av1_sequence_header_obu *seq = &(priv->codec_data.av1.seq);
   struct av1_uncompressed_header_obu *hdr = &(priv->codec_data.av1.uncompressed_header);
   uint8_t Ref_Frame_List[5] = { AV1_LAST2_FRAME , AV1_LAST3_FRAME, AV1_BWDREF_FRAME,
         unsigned earliestOrderHint = 0;
   uint8_t ref;
            for (i = 0; i < AV1_REFS_PER_FRAME; ++i)
            hdr->ref_frame_idx[0] = hdr->last_frame_idx;
            for (i = 0; i < AV1_NUM_REF_FRAMES; ++i)
            hdr->usedFrame[hdr->last_frame_idx] = 1;
                     for (i = 0; i < AV1_NUM_REF_FRAMES; ++i)
      hdr->shiftedOrderHints[i] =
               ref = find_latest_backward(priv);
   if (ref != 0xff) {
      hdr->ref_frame_idx[AV1_ALTREF_FRAME - AV1_LAST_FRAME] = ref;
               ref = find_earliest_backward(priv);
   if (ref != 0xff) {
      hdr->ref_frame_idx[AV1_BWDREF_FRAME - AV1_LAST_FRAME] = ref;
               ref = find_earliest_backward(priv);
   if (ref != 0xff) {
      hdr->ref_frame_idx[AV1_ALTREF2_FRAME - AV1_LAST_FRAME] = ref;
               for (i = 0; i < AV1_REFS_PER_FRAME - 2; ++i) {
      uint8_t refFrame = Ref_Frame_List[i];
   if (hdr->ref_frame_idx[refFrame - AV1_LAST_FRAME] == 0xff) {
      ref = find_latest_forward(priv);
   if (ref != 0xff) {
      hdr->ref_frame_idx[refFrame - AV1_LAST_FRAME] = ref;
                     ref = 0xff;
   for (i = 0; i < AV1_NUM_REF_FRAMES; ++i) {
      unsigned hint = hdr->shiftedOrderHints[i];
   if (ref == 0xff || hint < earliestOrderHint) {
      ref = i;
                  for (i = 0; i < AV1_REFS_PER_FRAME; ++i) {
   if (hdr->ref_frame_idx[i] == 0xff)
            }
      static void frame_size_with_refs(vid_dec_PrivateType *priv, struct vl_vlc *vlc)
   {
      struct av1_uncompressed_header_obu *hdr = &(priv->codec_data.av1.uncompressed_header);
   bool found_ref;
            for (i = 0; i < AV1_REFS_PER_FRAME; ++i) {
      found_ref = av1_f(vlc, 1);
   if (found_ref) {
      hdr->UpscaledWidth =
         hdr->FrameWidth = hdr->UpscaledWidth;
   hdr->FrameHeight =
         hdr->RenderWidth =
         hdr->RenderHeight =
                        if (!found_ref) {
      frame_size(priv, vlc);
      } else {
      superres_params(priv, vlc);
         }
      static unsigned tile_log2(unsigned blkSize, unsigned target)
   {
                           }
      static void tile_info(vid_dec_PrivateType *priv, struct vl_vlc *vlc)
   {
      struct av1_sequence_header_obu *seq = &(priv->codec_data.av1.seq);
   struct av1_uncompressed_header_obu *hdr = &(priv->codec_data.av1.uncompressed_header);
   struct tile_info *ti = &(priv->codec_data.av1.uncompressed_header.ti);
   unsigned sbCols;
   unsigned sbRows;
   int width_sb;
   int height_sb;
   unsigned sbSize;
   unsigned maxTileWidthSb;
   unsigned minLog2TileCols;
   unsigned maxLog2TileCols;
   unsigned maxLog2TileRows;
   unsigned minLog2Tiles;
   bool uniform_tile_spacing_flag;
   unsigned maxTileAreaSb;
            sbCols = (seq->use_128x128_superblock) ?
         sbRows = (seq->use_128x128_superblock) ?
         width_sb = sbCols;
   height_sb = sbRows;
   sbSize = (seq->use_128x128_superblock ? 5 : 4) + 2;
   maxTileWidthSb = AV1_MAX_TILE_WIDTH >> sbSize;
   maxTileAreaSb = AV1_MAX_TILE_AREA >> (2 * sbSize);
   minLog2TileCols = tile_log2(maxTileWidthSb, sbCols);
   maxLog2TileCols = tile_log2(1, MIN2(sbCols, AV1_MAX_TILE_COLS));
   maxLog2TileRows = tile_log2(1, MIN2(sbRows, AV1_MAX_TILE_ROWS));
            uniform_tile_spacing_flag = av1_f(vlc, 1);
   if (uniform_tile_spacing_flag) {
      unsigned tileWidthSb, tileHeightSb;
            ti->TileColsLog2 = minLog2TileCols;
   while (ti->TileColsLog2 < maxLog2TileCols) {
      bool increment_tile_cols_log2 = av1_f(vlc, 1);
   if (increment_tile_cols_log2)
         else
      }
   tileWidthSb = (sbCols + (1 << ti->TileColsLog2) - 1) >> ti->TileColsLog2;
   i = 0;
   for (startSb = 0; startSb < sbCols; startSb += tileWidthSb) {
      ti->tile_col_start_sb[i] = startSb;
      }
   ti->tile_col_start_sb[i] = sbCols;
            minLog2TileRows = (minLog2Tiles > ti->TileColsLog2)?
         ti->TileRowsLog2 = minLog2TileRows;
   while (ti->TileRowsLog2 < maxLog2TileRows) {
      bool increment_tile_rows_log2 = av1_f(vlc, 1);
   if (increment_tile_rows_log2)
         else
      }
   tileHeightSb = (sbRows + (1 << ti->TileRowsLog2) - 1) >> ti->TileRowsLog2;
   i = 0;
   for (startSb = 0; startSb < sbRows; startSb += tileHeightSb) {
      ti->tile_row_start_sb[i] = startSb;
      }
   ti->tile_row_start_sb[i] = sbRows;
      } else {
      unsigned widestTileSb = 0;
            startSb = 0;
   for (i = 0; startSb < sbCols; ++i) {
      uint8_t maxWidth;
                  ti->tile_col_start_sb[i] = startSb;
   maxWidth = MIN2(sbCols - startSb, maxTileWidthSb);
   width_in_sbs_minus_1 = av1_ns(vlc, maxWidth);
   sizeSb = width_in_sbs_minus_1 + 1;
   widestTileSb = MAX2(sizeSb, widestTileSb);
   startSb += sizeSb;
      }
            ti->tile_col_start_sb[i] = startSb + width_sb;
            if (minLog2Tiles > 0)
         else
                  startSb = 0;
   for (i = 0; startSb < sbRows; ++i) {
                     maxHeight = MIN2(sbRows - startSb, maxTileHeightSb);
   height_in_sbs_minus_1 = av1_ns(vlc, maxHeight);
   ti->tile_row_start_sb[i] = startSb;
   startSb += height_in_sbs_minus_1 + 1;
      }
   ti->TileRows = i;
   ti->tile_row_start_sb[i] = startSb + height_sb;
               if (ti->TileColsLog2 > 0 || ti->TileRowsLog2 > 0) {
      ti->context_update_tile_id =
         uint8_t tile_size_bytes_minus_1 = av1_f(vlc, 2);
      } else {
            }
      static int read_delta_q(struct vl_vlc *vlc)
   {
      bool delta_coded = av1_f(vlc, 1);
            if (delta_coded)
               }
      static void quantization_params(vid_dec_PrivateType *priv, struct vl_vlc *vlc)
   {
      struct av1_sequence_header_obu *seq = &(priv->codec_data.av1.seq);
   struct quantization_params* qp = &(priv->codec_data.av1.uncompressed_header.qp);
            qp->base_q_idx = av1_f(vlc, 8);
   qp->DeltaQYDc = read_delta_q(vlc);
   if (seq->color_config.NumPlanes > 1) {
      bool diff_uv_delta =
            qp->DeltaQUDc = read_delta_q(vlc);
   qp->DeltaQUAc = read_delta_q(vlc);
   if (diff_uv_delta) {
      qp->DeltaQVDc = read_delta_q(vlc);
      } else {
      qp->DeltaQVDc = qp->DeltaQUDc;
         } else {
      qp->DeltaQVDc = 0;
   qp->DeltaQVAc = 0;
   qp->DeltaQUDc = 0;
               using_qmatrix = av1_f(vlc, 1);
   if (using_qmatrix) {
      qp->qm_y = av1_f(vlc, 4);
   qp->qm_u = av1_f(vlc, 4);
   if (!seq->color_config.separate_uv_delta_q)
         else
      } else {
      qp->qm_y = 0xf;
   qp->qm_u = 0xf;
         }
      static void segmentation_params(vid_dec_PrivateType *priv, struct vl_vlc *vlc)
   {
      struct av1_uncompressed_header_obu *hdr = &(priv->codec_data.av1.uncompressed_header);
   struct segmentation_params* sp = &(priv->codec_data.av1.uncompressed_header.sp);
            sp->segmentation_enabled = av1_f(vlc, 1);
   if (sp->segmentation_enabled) {
               if (hdr->primary_ref_frame == AV1_PRIMARY_REF_NONE) {
      sp->segmentation_update_map = 1;
   sp->segmentation_temporal_update = 0;
      } else {
      sp->segmentation_update_map = av1_f(vlc, 1);
   if (sp->segmentation_update_map)
         else
                     if (segmentation_update_data) {
      uint8_t Segmentation_Feature_Bits[AV1_SEG_LVL_MAX] = { 8, 6, 6, 6, 6, 3, 0, 0 };
                  memset(sp->FeatureData, 0, sizeof(sp->FeatureData));
   memset(sp->FeatureMask, 0, sizeof(sp->FeatureMask));
   for (i = 0; i < AV1_MAX_SEGMENTS; ++i) {
      for (j = 0; j < AV1_SEG_LVL_MAX; ++j) {
                     sp->FeatureEnabled[i][j] = feature_enabled;
   int clippedValue = 0;
   if (feature_enabled) {
      uint8_t bitsToRead = Segmentation_Feature_Bits[j];
   int limit = Segmentation_Feature_Max[j];
   if (Segmentation_Feature_Signed[j]) {
      feature_value = av1_su(vlc, 1 + bitsToRead);
   clippedValue = CLAMP(feature_value, -limit, limit);
      } else {
      feature_value = av1_f(vlc, bitsToRead);
   clippedValue = CLAMP(feature_value, 0, limit);
         }
            } else {
      int r = hdr->ref_frame_idx[hdr->primary_ref_frame];
         } else {
            }
      static void delta_q_params(vid_dec_PrivateType *priv, struct vl_vlc *vlc)
   {
      struct quantization_params* qp = &(priv->codec_data.av1.uncompressed_header.qp);
            dqp->delta_q_present = 0;
   dqp->delta_q_res = 0;
   if (qp->base_q_idx > 0)
         if (dqp->delta_q_present)
      }
      static void delta_lf_params(vid_dec_PrivateType *priv, struct vl_vlc *vlc)
   {
      struct av1_uncompressed_header_obu *hdr = &(priv->codec_data.av1.uncompressed_header);
   struct delta_q_params * dqp = &(priv->codec_data.av1.uncompressed_header.dqp);
            dlfp->delta_lf_present = 0;
   dlfp->delta_lf_res = 0;
   dlfp->delta_lf_multi = 0;
   if (dqp->delta_q_present) {
      if (!hdr->allow_intrabc)
            if (dlfp->delta_lf_present) {
      dlfp->delta_lf_res = av1_f(vlc, 2);
            }
      static unsigned get_qindex(vid_dec_PrivateType * priv, bool ignoreDeltaQ, unsigned segmentId)
   {
      struct av1_uncompressed_header_obu *hdr = &(priv->codec_data.av1.uncompressed_header);
   struct segmentation_params* sp = &(priv->codec_data.av1.uncompressed_header.sp);
   struct quantization_params* qp = &(priv->codec_data.av1.uncompressed_header.qp);
            if (sp->segmentation_enabled && sp->FeatureEnabled[segmentId][AV1_SEG_LVL_ALT_Q]) {
      unsigned data = sp->FeatureData[segmentId][AV1_SEG_LVL_ALT_Q];
   qindex = qp->base_q_idx + data;
   if (!ignoreDeltaQ && hdr->dqp.delta_q_present)
                        if (!ignoreDeltaQ && hdr->dqp.delta_q_present)
               }
      static void loop_filter_params(vid_dec_PrivateType *priv, struct vl_vlc *vlc)
   {
      struct av1_sequence_header_obu *seq = &(priv->codec_data.av1.seq);
   struct av1_uncompressed_header_obu *hdr = &(priv->codec_data.av1.uncompressed_header);
   struct loop_filter_params *lfp = &(priv->codec_data.av1.uncompressed_header.lfp);
            if (hdr->CodedLossless || hdr->allow_intrabc) {
      lfp->loop_filter_level[0] = 0;
   lfp->loop_filter_level[1] = 0;
   lfp->loop_filter_ref_deltas[AV1_INTRA_FRAME] = 1;
   lfp->loop_filter_ref_deltas[AV1_LAST_FRAME] = 0;
   lfp->loop_filter_ref_deltas[AV1_LAST2_FRAME] = 0;
   lfp->loop_filter_ref_deltas[AV1_LAST3_FRAME] = 0;
   lfp->loop_filter_ref_deltas[AV1_BWDREF_FRAME] = 0;
   lfp->loop_filter_ref_deltas[AV1_GOLDEN_FRAME] = -1;
   lfp->loop_filter_ref_deltas[AV1_ALTREF2_FRAME] = -1;
   lfp->loop_filter_ref_deltas[AV1_ALTREF_FRAME] = -1;
   lfp->loop_filter_mode_deltas[0] = 0;
   lfp->loop_filter_mode_deltas[1] = 0;
               if (hdr->primary_ref_frame == AV1_PRIMARY_REF_NONE) {
      lfp->loop_filter_ref_deltas[AV1_INTRA_FRAME] = 1;
   lfp->loop_filter_ref_deltas[AV1_LAST_FRAME] = 0;
   lfp->loop_filter_ref_deltas[AV1_LAST2_FRAME] = 0;
   lfp->loop_filter_ref_deltas[AV1_LAST3_FRAME] = 0;
   lfp->loop_filter_ref_deltas[AV1_BWDREF_FRAME] = 0;
   lfp->loop_filter_ref_deltas[AV1_GOLDEN_FRAME] = -1;
   lfp->loop_filter_ref_deltas[AV1_ALTREF2_FRAME] = -1;
   lfp->loop_filter_ref_deltas[AV1_ALTREF_FRAME] = -1;
   lfp->loop_filter_mode_deltas[0] = 0;
      } else {
      int r = hdr->ref_frame_idx[hdr->primary_ref_frame];
   memcpy(lfp->loop_filter_ref_deltas,
         memcpy(lfp->loop_filter_mode_deltas,
               lfp->loop_filter_level[0] = av1_f(vlc, 6);
   lfp->loop_filter_level[1] = av1_f(vlc, 6);
   if (seq->color_config.NumPlanes > 1) {
      if (lfp->loop_filter_level[0] || lfp->loop_filter_level[1]) {
      lfp->loop_filter_level[2] = av1_f(vlc, 6);
                  lfp->loop_filter_sharpness = av1_f(vlc, 3);
   lfp->loop_filter_delta_enabled = av1_f(vlc, 1);
   if (lfp->loop_filter_delta_enabled) {
      lfp->loop_filter_delta_update = av1_f(vlc, 1);
   if (lfp->loop_filter_delta_update) {
      for (i = 0; i < AV1_NUM_REF_FRAMES; ++i) {
      int8_t update_ref_delta = av1_f(vlc, 1);
   if (update_ref_delta)
               for (i = 0; i < 2; ++i) {
      int8_t update_mode_delta = av1_f(vlc, 1);
   if (update_mode_delta)
               }
      static void cdef_params(vid_dec_PrivateType *priv, struct vl_vlc *vlc)
   {
      struct av1_sequence_header_obu *seq = &(priv->codec_data.av1.seq);
   struct av1_uncompressed_header_obu *hdr = &(priv->codec_data.av1.uncompressed_header);
   struct cdef_params *cdefp = &(priv->codec_data.av1.uncompressed_header.cdefp);;
            if (hdr->CodedLossless || hdr->allow_intrabc || !seq->enable_cdef) {
      cdefp->cdef_bits = 0;
   cdefp->cdef_y_strengths[0] = 0;
   cdefp->cdef_uv_strengths[0] = 0;
               cdefp->cdef_damping_minus_3 = av1_f(vlc, 2);
   cdefp->cdef_bits = av1_f(vlc, 2);
   for (i = 0; i < (1 << cdefp->cdef_bits); ++i) {
      cdefp->cdef_y_strengths[i] = av1_f(vlc, 6);
   if (seq->color_config.NumPlanes > 1)
         }
      static void lr_params(vid_dec_PrivateType *priv, struct vl_vlc *vlc)
   {
      struct av1_sequence_header_obu *seq = &(priv->codec_data.av1.seq);
   struct av1_uncompressed_header_obu *hdr = &(priv->codec_data.av1.uncompressed_header);
   struct loop_restoration_params *lrp = &(priv->codec_data.av1.uncompressed_header.lrp);
   uint8_t Remap_Lr_Type[4] =
         bool UsesLr = false;
   bool UsesChromaLr = false;
   uint8_t lr_unit_shift, lr_uv_shift;
            if (hdr->AllLossless || hdr->allow_intrabc || !seq->enable_restoration) {
      lrp->FrameRestorationType[0] = AV1_RESTORE_NONE;
   lrp->FrameRestorationType[1] = AV1_RESTORE_NONE;
   lrp->FrameRestorationType[2] = AV1_RESTORE_NONE;
               for (i = 0; i < seq->color_config.NumPlanes; ++i) {
      uint8_t lr_type = av1_f(vlc, 2);
   lrp->FrameRestorationType[i] = Remap_Lr_Type[lr_type];
   if (lrp->FrameRestorationType[i] != AV1_RESTORE_NONE) {
      UsesLr = true;
   if (i > 0)
                  if (UsesLr) {
      if (seq->use_128x128_superblock) {
         } else {
      lr_unit_shift = av1_f(vlc, 1);
   if (lr_unit_shift) {
      uint8_t lr_unit_extra_shift = av1_f(vlc, 1);
                  lrp->LoopRestorationSize[0] = AV1_RESTORATION_TILESIZE >> (2 - lr_unit_shift);
   lr_uv_shift =
                  lrp->LoopRestorationSize[1] = lrp->LoopRestorationSize[0] >> lr_uv_shift;
      } else {
      lrp->LoopRestorationSize[0] = lrp->LoopRestorationSize[1] =
         }
      static void tx_mode(vid_dec_PrivateType *priv, struct vl_vlc *vlc)
   {
      struct av1_uncompressed_header_obu *hdr = &(priv->codec_data.av1.uncompressed_header);
            if (hdr->CodedLossless) {
         } else {
      bool tx_mode_select = av1_f(vlc, 1);
         }
      static void frame_reference_mode(vid_dec_PrivateType *priv, struct vl_vlc *vlc)
   {
               if (hdr->FrameIsIntra)
         else
      }
      static void skip_mode_params(vid_dec_PrivateType *priv, struct vl_vlc *vlc)
   {
      struct av1_sequence_header_obu *seq = &(priv->codec_data.av1.seq);
   struct av1_uncompressed_header_obu *hdr = &(priv->codec_data.av1.uncompressed_header);
   struct skip_mode_params *smp = &(priv->codec_data.av1.uncompressed_header.smp);;
   bool skipModeAllowed;
            if (hdr->FrameIsIntra || hdr->reference_select == SINGLE_REFERENCE ||
               } else {
      int ref_frame_offset[2] = { -1, INT_MAX };
            skipModeAllowed = 0;
   for (i = 0; i < AV1_REFS_PER_FRAME; ++i) {
      unsigned ref_offset = priv->codec_data.av1.refs[hdr->ref_frame_idx[i]].OrderHint;
   if (get_relative_dist(priv, ref_offset, hdr->OrderHint) < 0) {
      if (ref_frame_offset[0] == -1 ||
      get_relative_dist(priv, ref_offset, ref_frame_offset[0]) > 0) {
   ref_frame_offset[0] = ref_offset;
         } else if (get_relative_dist(priv, ref_offset, hdr->OrderHint) > 0) {
      if (ref_frame_offset[1] == INT_MAX ||
      get_relative_dist(priv, ref_offset, ref_frame_offset[1]) < 0) {
   ref_frame_offset[1] = ref_offset;
                     if (ref_idx[0] != -1 && ref_idx[1] != -1) {
         } else if (ref_idx[0] != -1 && ref_idx[1] == -1) {
      ref_frame_offset[1] = -1;
   for (i = 0; i < AV1_ALTREF_FRAME - AV1_LAST_FRAME + 1; ++i) {
      unsigned ref_offset = priv->codec_data.av1.refs[hdr->ref_frame_idx[i]].OrderHint;
   if ((ref_frame_offset[0] != -1 &&
      get_relative_dist(priv, ref_offset, ref_frame_offset[0]) < 0) &&
   (ref_frame_offset[1] == -1 ||
   get_relative_dist(priv, ref_offset, ref_frame_offset[1]) > 0)) {
   ref_frame_offset[1] = ref_offset;
         }
   if (ref_frame_offset[1] != -1)
                     }
      static unsigned inverse_recenter(unsigned r, unsigned v)
   {
      if (v > (2 * r))
         else if (v & 1)
         else
      }
      static unsigned decode_subexp(struct vl_vlc *vlc, unsigned numSyms)
   {
      unsigned i = 0;
   unsigned mk = 0;
            while (1) {
      unsigned b2 = (i) ? (k + i - 1) : k;
   unsigned a = 1 << b2;
   if (numSyms <= (mk + 3 * a)) {
      unsigned subexp_final_bits = av1_ns(vlc, (numSyms - mk));
      } else {
      bool subexp_more_bits = av1_f(vlc, 1);
   if (subexp_more_bits) {
      i++;
      } else {
      unsigned subexp_bits = av1_f(vlc, b2);
               }
      static unsigned decode_unsigned_subexp_with_ref(struct vl_vlc *vlc,
         {
      unsigned smart;
            if ((r << 1) <= mx) {
      smart = inverse_recenter(r, v);
      } else {
      smart = inverse_recenter(mx - 1 - r, v);
         }
      static int decode_signed_subexp_with_ref(struct vl_vlc *vlc, int low, int high, int r)
   {
                  }
      static void read_global_param(struct global_motion_params* global_params,
                     {
      struct av1_uncompressed_header_obu *hdr = &(priv->codec_data.av1.uncompressed_header);
   uint8_t absBits = 12; /* GM_ABS_ALPHA_BITS */
   uint8_t precBits = 15; /* GM_ALPHA_PREC_BITS */
            if (idx < 2) {
      if (type == AV1_TRANSLATION) {
      absBits = 9 /* GM_ABS_TRANS_ONLY_BITS */ - !hdr->allow_high_precision_mv;
      } else {
      absBits = 12; /* GM_ABS_TRANS_BITS */
                  precDiff = AV1_WARPEDMODEL_PREC_BITS - precBits;
   round = ((idx % 3) == 2) ? (1 << AV1_WARPEDMODEL_PREC_BITS) : 0;
   sub = ((idx % 3) == 2) ? (1 << precBits) : 0;
   mx = (int)(1 << absBits);
   if (ref_params)
            global_params->gm_params[ref][idx] =
      }
      static void global_motion_params(vid_dec_PrivateType *priv, struct vl_vlc *vlc)
   {
      struct av1_uncompressed_header_obu *hdr = &(priv->codec_data.av1.uncompressed_header);
   struct global_motion_params *gmp = &(priv->codec_data.av1.uncompressed_header.gmp);
   struct global_motion_params *ref_gmp = NULL;
            if (hdr->primary_ref_frame == AV1_PRIMARY_REF_NONE) {
      for (ref = 0; ref < AV1_NUM_REF_FRAMES; ++ref) {
      gmp->GmType[ref] = AV1_IDENTITY;
   for (i = 0; i < 6; ++i)
         } else {
      const int r = hdr->ref_frame_idx[hdr->primary_ref_frame];
               for (ref = AV1_LAST_FRAME; ref <= AV1_ALTREF_FRAME; ++ref) {
      gmp->GmType[ref] = AV1_IDENTITY;
   for (i = 0; i < 6; ++i)
               if (hdr->FrameIsIntra)
            for (ref = AV1_LAST_FRAME; ref <= AV1_ALTREF_FRAME; ++ref) {
      uint8_t type = AV1_IDENTITY;
            gmp->GmType[ref] = AV1_IDENTITY;
   for (i = 0; i < 6; ++i)
            is_global = av1_f(vlc, 1);
   if (is_global) {
      bool is_rot_zoom = av1_f(vlc, 1);
   if (is_rot_zoom) {
         } else {
      bool is_translation = av1_f(vlc, 1);
                           if (type >= AV1_ROTZOOM) {
      read_global_param(gmp, ref_gmp, priv, vlc, type, ref, 2);
   read_global_param(gmp, ref_gmp, priv, vlc, type, ref, 3);
   if (type == AV1_AFFINE) {
      read_global_param(gmp, ref_gmp, priv, vlc, type, ref, 4);
      } else {
      gmp->gm_params[ref][4] = -gmp->gm_params[ref][3];
                  if (type >= AV1_TRANSLATION) {
      read_global_param(gmp, ref_gmp, priv, vlc, type, ref, 0);
            }
      static void film_grain_params(vid_dec_PrivateType *priv, struct vl_vlc *vlc)
   {
      struct av1_sequence_header_obu *seq = &(priv->codec_data.av1.seq);
   struct av1_uncompressed_header_obu *hdr = &(priv->codec_data.av1.uncompressed_header);
            bool update_grain;
   uint8_t numPosLuma;
   uint8_t numPosChroma;
            if (!seq->film_grain_params_present ||
            memset(fgp, 0, sizeof(*fgp));
               fgp->apply_grain = av1_f(vlc, 1);
   if (!fgp->apply_grain) {
      memset(fgp, 0, sizeof(*fgp));
               fgp->grain_seed = av1_f(vlc, 16);
   update_grain =
            if (!update_grain) {
      uint8_t film_grain_params_ref_idx = av1_f(vlc, 3);
   uint16_t tempGrainSeed = fgp->grain_seed;
   memcpy(fgp, &(priv->codec_data.av1.refs[film_grain_params_ref_idx].fgp),
         fgp->grain_seed = tempGrainSeed;
               fgp->num_y_points = av1_f(vlc, 4);
   for (i = 0; i < fgp->num_y_points; ++i) {
      fgp->point_y_value[i] = av1_f(vlc, 8);
               fgp->chroma_scaling_from_luma =
         if (seq->color_config.mono_chrome || fgp->chroma_scaling_from_luma ||
      (seq->color_config.subsampling_x && seq->color_config.subsampling_y &&
   (fgp->num_y_points == 0))) {
   fgp->num_cb_points = 0;
      } else {
      fgp->num_cb_points = av1_f(vlc, 4);
   for (i = 0; i < fgp->num_cb_points; ++i) {
      fgp->point_cb_value[i] = av1_f(vlc, 8);
      }
   fgp->num_cr_points = av1_f(vlc, 4);
   for (i = 0; i < fgp->num_cr_points; ++i) {
      fgp->point_cr_value[i] = av1_f(vlc, 8);
                  fgp->grain_scaling_minus_8 = av1_f(vlc, 2);
   fgp->ar_coeff_lag = av1_f(vlc, 2);
   numPosLuma = 2 * fgp->ar_coeff_lag * (fgp->ar_coeff_lag + 1);
   if (fgp->num_y_points) {
      numPosChroma = numPosLuma + 1;
   for (i = 0; i < numPosLuma; ++i) {
      uint8_t ar_coeffs_y_plus_128 = av1_f(vlc, 8);
         } else {
                  if (fgp->chroma_scaling_from_luma || fgp->num_cb_points) {
      for (i = 0; i < numPosChroma; ++i) {
      uint8_t ar_coeffs_cb_plus_128 = av1_f(vlc, 8);
                  if (fgp->chroma_scaling_from_luma || fgp->num_cr_points) {
      for (i = 0; i < numPosChroma; ++i) {
      uint8_t ar_coeffs_cr_plus_128 = av1_f(vlc, 8);
                  fgp->ar_coeff_shift_minus_6 = av1_f(vlc, 2);
   fgp->grain_scale_shift = av1_f(vlc, 2);
   if (fgp->num_cb_points) {
      fgp->cb_mult = av1_f(vlc, 8);
   fgp->cb_luma_mult = av1_f(vlc, 8);
               if (fgp->num_cr_points) {
      fgp->cr_mult = av1_f(vlc, 8);
   fgp->cr_luma_mult = av1_f(vlc, 8);
               fgp->overlap_flag = av1_f(vlc, 1);
      }
      static void frame_header_obu(vid_dec_PrivateType *priv, struct vl_vlc *vlc)
   {
      struct av1_sequence_header_obu *seq = &(priv->codec_data.av1.seq);
   struct av1_uncompressed_header_obu *hdr = &(priv->codec_data.av1.uncompressed_header);
   unsigned idLen = 0;
   unsigned allFrames;
                     if (seq->frame_id_numbers_present_flag)
      idLen = seq->additional_frame_id_length_minus_1 +
         allFrames = (1 << AV1_NUM_REF_FRAMES) - 1;
   if (seq->reduced_still_picture_header) {
      hdr->show_existing_frame = 0;
   hdr->frame_type = AV1_KEY_FRAME;
   hdr->FrameIsIntra = 1;
   hdr->show_frame = 1;
      } else {
      hdr->show_existing_frame = av1_f(vlc, 1);
   if (hdr->show_existing_frame) {
      hdr->frame_to_show_map_idx = av1_f(vlc, 3);
   if (seq->decoder_model_info_present_flag &&
            av1_f(vlc, seq->decoder_model_info.
      hdr->refresh_frame_flags  = 0;
                  hdr->frame_type =
                              hdr->frame_type = av1_f(vlc, 2);
   hdr->FrameIsIntra = (hdr->frame_type == AV1_INTRA_ONLY_FRAME ||
                  if (hdr->show_frame && seq->decoder_model_info_present_flag &&
                  hdr->showable_frame =
            hdr->error_resilient_mode = (hdr->frame_type == AV1_SWITCH_FRAME ||
               if (hdr->frame_type == AV1_KEY_FRAME && hdr->show_frame) {
      for (i = 0; i < AV1_NUM_REF_FRAMES; ++i)
                        hdr->allow_screen_content_tools =
      (seq->seq_force_screen_content_tools == AV1_SELECT_SCREEN_CONTENT_TOOLS) ?
         if (hdr->allow_screen_content_tools) {
      if (seq->seq_force_integer_mv == AV1_SELECT_INTEGER_MV)
         else
      } else {
                  if (hdr->FrameIsIntra)
            hdr->current_frame_id =
            if (hdr->frame_type == AV1_SWITCH_FRAME)
         else if (seq->reduced_still_picture_header)
         else
                     if (hdr->FrameIsIntra || hdr->error_resilient_mode)
         else
            if (seq->decoder_model_info_present_flag) {
      bool buffer_removal_time_present_flag = av1_f(vlc, 1);
   if (buffer_removal_time_present_flag) {
      for (i = 0; i <= seq->operating_points_cnt_minus_1; ++i) {
      if (seq->decoder_model_present_for_this_op[i]) {
      unsigned opPtIdc;
   bool inTemporalLayer;
   bool inSpatialLayer;
   opPtIdc = seq->operating_point_idc[i];
   inTemporalLayer =
         inSpatialLayer =
         if ((opPtIdc == 0) || (inTemporalLayer && inSpatialLayer))
      av1_f(vlc, seq->decoder_model_info.
                     hdr->allow_high_precision_mv = 0;
   hdr->use_ref_frame_mvs = 0;
            hdr->refresh_frame_flags = allFrames = (hdr->frame_type == AV1_SWITCH_FRAME ||
                  if (!hdr->FrameIsIntra || hdr->refresh_frame_flags != allFrames) {
      if (hdr->error_resilient_mode && seq->enable_order_hint) {
      for (i = 0; i < AV1_NUM_REF_FRAMES; ++i)
                  if (hdr->FrameIsIntra) {
      frame_size(priv, vlc);
   render_size(priv, vlc);
   if (hdr->allow_screen_content_tools && (hdr->UpscaledWidth == hdr->FrameWidth))
      } else {
      bool is_filter_switchable;
            if (!seq->enable_order_hint) {
         } else {
      frame_refs_short_signaling = av1_f(vlc, 1);
   if (frame_refs_short_signaling) {
      hdr->last_frame_idx = av1_f(vlc, 3);
   hdr->gold_frame_idx = av1_f(vlc, 3);
                  for (i = 0; i < AV1_REFS_PER_FRAME; ++i) {
      if (!frame_refs_short_signaling)
         if (seq->frame_id_numbers_present_flag)
               if (hdr->frame_size_override_flag && !hdr->error_resilient_mode) {
         } else {
      frame_size(priv, vlc);
                        is_filter_switchable = av1_f(vlc, 1);
            hdr->is_motion_mode_switchable = av1_f(vlc, 1);
   hdr->use_ref_frame_mvs =
               hdr->disable_frame_end_update_cdf =
            tile_info(priv, vlc);
   quantization_params(priv, vlc);
   segmentation_params(priv, vlc);
   delta_q_params(priv, vlc);
            hdr->CodedLossless = 1;
   for (i = 0; i < AV1_MAX_SEGMENTS; ++i) {
      unsigned qindex = get_qindex(priv, 1, i);
   bool LosslessArray =
      (qindex == 0) && (hdr->qp.DeltaQYDc == 0) &&
               if (!LosslessArray)
      }
            loop_filter_params(priv, vlc);
   cdef_params(priv, vlc);
   lr_params(priv, vlc);
   tx_mode(priv, vlc);
   frame_reference_mode(priv, vlc);
            if (hdr->FrameIsIntra || hdr->error_resilient_mode || !seq->enable_warped_motion)
         else
                                    priv->picture.av1.film_grain_target = NULL;
   priv->picture.av1.picture_parameter.pic_info_fields.frame_type = hdr->frame_type;
   priv->picture.av1.picture_parameter.pic_info_fields.show_frame = hdr->show_frame;
   priv->picture.av1.picture_parameter.pic_info_fields.error_resilient_mode =
         priv->picture.av1.picture_parameter.pic_info_fields.disable_cdf_update =
         priv->picture.av1.picture_parameter.pic_info_fields.allow_screen_content_tools =
         priv->picture.av1.picture_parameter.pic_info_fields.force_integer_mv =
         priv->picture.av1.picture_parameter.current_frame_id = hdr->current_frame_id;
   priv->picture.av1.picture_parameter.order_hint = hdr->OrderHint;
   priv->picture.av1.picture_parameter.primary_ref_frame = hdr->primary_ref_frame;
   priv->picture.av1.picture_parameter.frame_width = hdr->FrameWidth;
   priv->picture.av1.picture_parameter.frame_height = hdr->FrameHeight;
   priv->picture.av1.picture_parameter.pic_info_fields.use_superres =
         priv->picture.av1.picture_parameter.superres_scale_denominator =
            for (i = 0; i < AV1_REFS_PER_FRAME; ++i)
            priv->picture.av1.picture_parameter.pic_info_fields.allow_high_precision_mv =
         priv->picture.av1.picture_parameter.pic_info_fields.allow_intrabc = hdr->allow_intrabc;
   priv->picture.av1.picture_parameter.pic_info_fields.use_ref_frame_mvs =
         priv->picture.av1.picture_parameter.interp_filter = hdr->interpolation_filter;
   priv->picture.av1.picture_parameter.pic_info_fields.is_motion_mode_switchable =
         priv->picture.av1.picture_parameter.refresh_frame_flags =
         priv->picture.av1.picture_parameter.pic_info_fields.disable_frame_end_update_cdf =
            /* Tile Info */
   priv->picture.av1.picture_parameter.tile_rows = hdr->ti.TileRows;
   priv->picture.av1.picture_parameter.tile_cols = hdr->ti.TileCols;
   priv->picture.av1.picture_parameter.context_update_tile_id =
         for (i = 0; i <AV1_MAX_TILE_ROWS; ++i)
      priv->picture.av1.picture_parameter.tile_row_start_sb[i] =
      for (i = 0; i <AV1_MAX_TILE_COLS; ++i)
      priv->picture.av1.picture_parameter.tile_col_start_sb[i] =
         /* Quantization Params */
   priv->picture.av1.picture_parameter.base_qindex =  hdr->qp.base_q_idx;
   priv->picture.av1.picture_parameter.y_dc_delta_q = hdr->qp.DeltaQYDc;
   priv->picture.av1.picture_parameter.u_dc_delta_q = hdr->qp.DeltaQUDc;
   priv->picture.av1.picture_parameter.u_ac_delta_q = hdr->qp.DeltaQUAc;
   priv->picture.av1.picture_parameter.v_dc_delta_q = hdr->qp.DeltaQVDc;
   priv->picture.av1.picture_parameter.v_ac_delta_q = hdr->qp.DeltaQVAc;
   priv->picture.av1.picture_parameter.qmatrix_fields.qm_y = hdr->qp.qm_y;
   priv->picture.av1.picture_parameter.qmatrix_fields.qm_u = hdr->qp.qm_u;
            /* Segmentation Params */
   priv->picture.av1.picture_parameter.seg_info.segment_info_fields.enabled =
         priv->picture.av1.picture_parameter.seg_info.segment_info_fields.update_map =
         priv->picture.av1.picture_parameter.seg_info.segment_info_fields.temporal_update =
         for (i = 0; i < AV1_MAX_SEGMENTS; ++i) {
      for (j = 0; j < AV1_SEG_LVL_MAX; ++j)
      priv->picture.av1.picture_parameter.seg_info.feature_data[i][j] =
      priv->picture.av1.picture_parameter.seg_info.feature_mask[i] =
               /* Delta Q Params */
   priv->picture.av1.picture_parameter.mode_control_fields.delta_q_present_flag =
         priv->picture.av1.picture_parameter.mode_control_fields.log2_delta_q_res =
            /* Delta LF Params */
   priv->picture.av1.picture_parameter.mode_control_fields.delta_lf_present_flag =
         priv->picture.av1.picture_parameter.mode_control_fields.log2_delta_lf_res =
         priv->picture.av1.picture_parameter.mode_control_fields.delta_lf_multi =
            /* Loop Filter Params */
   for (i = 0; i < 2; ++i)
         priv->picture.av1.picture_parameter.filter_level_u = hdr->lfp.loop_filter_level[2];
   priv->picture.av1.picture_parameter.filter_level_v = hdr->lfp.loop_filter_level[3];
   priv->picture.av1.picture_parameter.loop_filter_info_fields.sharpness_level =
         priv->picture.av1.picture_parameter.loop_filter_info_fields.mode_ref_delta_enabled =
         priv->picture.av1.picture_parameter.loop_filter_info_fields.mode_ref_delta_update =
         for (i = 0; i < AV1_NUM_REF_FRAMES; ++i)
      priv->picture.av1.picture_parameter.ref_deltas[i] =
      for (i = 0; i < 2; ++i)
      priv->picture.av1.picture_parameter.mode_deltas[i] =
         /* CDEF Params */
   priv->picture.av1.picture_parameter.cdef_damping_minus_3 =
         priv->picture.av1.picture_parameter.cdef_bits = hdr->cdefp.cdef_bits;
   for (i = 0; i < AV1_MAX_CDEF_BITS_ARRAY; ++i) {
      priv->picture.av1.picture_parameter.cdef_y_strengths[i] =
         priv->picture.av1.picture_parameter.cdef_uv_strengths[i] =
               /* Loop Restoration Params */
   priv->picture.av1.picture_parameter.loop_restoration_fields.yframe_restoration_type =
         priv->picture.av1.picture_parameter.loop_restoration_fields.cbframe_restoration_type =
         priv->picture.av1.picture_parameter.loop_restoration_fields.crframe_restoration_type =
         for (i = 0; i < 3; ++i)
            priv->picture.av1.picture_parameter.mode_control_fields.tx_mode = hdr->tm.TxMode;
   priv->picture.av1.picture_parameter.mode_control_fields.reference_select =
         priv->picture.av1.picture_parameter.mode_control_fields.skip_mode_present =
         priv->picture.av1.picture_parameter.pic_info_fields.allow_warped_motion =
         priv->picture.av1.picture_parameter.mode_control_fields.reduced_tx_set_used =
            /* Global Motion Params */
   for (i = 0; i < 7; ++i) {
      priv->picture.av1.picture_parameter.wm[i].wmtype = hdr->gmp.GmType[i + 1];
   for (j = 0; j < 6; ++j)
               /* Film Grain Params */
   priv->picture.av1.picture_parameter.film_grain_info.film_grain_info_fields.apply_grain =
         priv->picture.av1.picture_parameter.film_grain_info.grain_seed =
         priv->picture.av1.picture_parameter.film_grain_info.num_y_points =
         for (i = 0; i < AV1_FG_MAX_NUM_Y_POINTS; ++i) {
      priv->picture.av1.picture_parameter.film_grain_info.point_y_value[i] =
         priv->picture.av1.picture_parameter.film_grain_info.point_y_scaling[i] =
      }
   priv->picture.av1.picture_parameter.film_grain_info.film_grain_info_fields.
         priv->picture.av1.picture_parameter.film_grain_info.num_cb_points =
         priv->picture.av1.picture_parameter.film_grain_info.num_cr_points =
         for (i = 0; i < AV1_FG_MAX_NUM_CBR_POINTS; ++i) {
      priv->picture.av1.picture_parameter.film_grain_info.point_cb_value[i] =
         priv->picture.av1.picture_parameter.film_grain_info.point_cb_scaling[i] =
         priv->picture.av1.picture_parameter.film_grain_info.point_cr_value[i] =
         priv->picture.av1.picture_parameter.film_grain_info.point_cr_scaling[i] =
      }
   priv->picture.av1.picture_parameter.film_grain_info.film_grain_info_fields.
         priv->picture.av1.picture_parameter.film_grain_info.film_grain_info_fields.
         for (i = 0; i < AV1_FG_MAX_NUM_POS_LUMA; ++i)
      priv->picture.av1.picture_parameter.film_grain_info.ar_coeffs_y[i] =
      for (i = 0; i < AV1_FG_MAX_NUM_POS_CHROMA; ++i) {
      priv->picture.av1.picture_parameter.film_grain_info.ar_coeffs_cb[i] =
         priv->picture.av1.picture_parameter.film_grain_info.ar_coeffs_cr[i] =
      }
   priv->picture.av1.picture_parameter.film_grain_info.film_grain_info_fields.
         priv->picture.av1.picture_parameter.film_grain_info.film_grain_info_fields.
         priv->picture.av1.picture_parameter.film_grain_info.cb_mult = hdr->fgp.cb_mult;
   priv->picture.av1.picture_parameter.film_grain_info.cb_luma_mult = hdr->fgp.cb_luma_mult;
   priv->picture.av1.picture_parameter.film_grain_info.cb_offset = hdr->fgp.cb_offset;
   priv->picture.av1.picture_parameter.film_grain_info.cr_mult = hdr->fgp.cr_mult;
   priv->picture.av1.picture_parameter.film_grain_info.cr_luma_mult = hdr->fgp.cr_luma_mult;
   priv->picture.av1.picture_parameter.film_grain_info.cr_offset = hdr->fgp.cr_offset;
   priv->picture.av1.picture_parameter.film_grain_info.film_grain_info_fields.
         priv->picture.av1.picture_parameter.film_grain_info.film_grain_info_fields.
      }
      static void parse_tile_hdr(vid_dec_PrivateType *priv, struct vl_vlc *vlc,
         {
      struct tile_info *ti = &(priv->codec_data.av1.uncompressed_header.ti);
   unsigned tg_start, tg_end;
   unsigned NumTiles, tileBits;
   bool tile_start_and_end_present_flag;
   unsigned size[AV1_MAX_NUM_TILES] = { 0 };
   unsigned offset[AV1_MAX_NUM_TILES] = { 0 };
   unsigned frame_header_size, left_size;
            NumTiles = ti->TileCols * ti->TileRows;
   tile_start_and_end_present_flag = 0;
   if (NumTiles > 1)
            if (NumTiles == 1 || !tile_start_and_end_present_flag) {
      tg_start = 0;
      } else {
      tileBits = ti->TileColsLog2 + ti->TileRowsLog2;
   tg_start = av1_f(vlc, tileBits);
                        frame_header_size = (start_bits_pos - vl_vlc_bits_left(vlc)) / 8;
   left_size = total_obu_len - frame_header_size;
   for (i = tg_start; i <= tg_end; ++i) {
      if (i == tg_start) {
      offset[i] = priv->codec_data.av1.bs_obu_td_sz +
      priv->codec_data.av1.bs_obu_seq_sz + frame_header_size +
      if (tg_start == tg_end) {
      size[i] = left_size;
   for (j = 0; j < size[i]; ++j) {
      vl_vlc_fillbits(vlc);
      }
         } else {
      offset[i] = offset[i - 1] + ti->TileSizeBytes + size[i - 1];
               if (i != tg_end) {
         } else {
      offset[i] = offset[i - 1] + size[i - 1];
               for (j = 0; j < size[i]; ++j) {
      vl_vlc_fillbits(vlc);
                  for (i = tg_start; i <= tg_end; ++i) {
      priv->picture.av1.slice_parameter.slice_data_offset[i] = offset[i];
         }
      static struct dec_av1_task *dec_av1_NeedTask(vid_dec_PrivateType *priv)
   {
      struct pipe_video_buffer templat = {};
   struct dec_av1_task *task;
   struct vl_screen *omx_screen;
            omx_screen = priv->screen;
            pscreen = omx_screen->pscreen;
            if (!list_is_empty(&priv->codec_data.av1.free_tasks)) {
      task = list_entry(priv->codec_data.av1.free_tasks.next,
         task->buf_ref_count = 1;
   list_del(&task->list);
               task = CALLOC_STRUCT(dec_av1_task);
   if (!task)
            memset(&templat, 0, sizeof(templat));
   templat.width = priv->codec->width;
   templat.height = priv->codec->height;
   templat.buffer_format = pscreen->get_video_param(
         pscreen,
   PIPE_VIDEO_PROFILE_UNKNOWN,
   PIPE_VIDEO_ENTRYPOINT_BITSTREAM,
   );
            task->buf = priv->pipe->create_video_buffer(priv->pipe, &templat);
   if (!task->buf) {
      FREE(task);
      }
   task->buf_ref_count = 1;
               }
      static void dec_av1_ReleaseTask(vid_dec_PrivateType *priv,
         {
      if (!head || !head->next)
            list_for_each_entry_safe(struct dec_av1_task, task, head, list) {
      task->buf->destroy(task->buf);
         }
      static void dec_av1_MoveTask(struct list_head *from,
         {
      to->prev->next = from->next;
   from->next->prev = to->prev;
   from->prev->next = to;
   to->prev = from->prev;
      }
      static void dec_av1_SortTask(vid_dec_PrivateType *priv)
   {
               list_for_each_entry_safe(struct dec_av1_task, t,
            bool found = false;
   for (i = 0; i < 8; ++i) {
      if (t->buf == priv->picture.av1.ref[i]) {
      found = true;
         }
   if (!found && t->buf_ref_count == 0) {
      list_del(&t->list);
            }
      static struct dec_av1_task *dec_av1_SearchTask(vid_dec_PrivateType *priv,
         {
      unsigned idx =
            list_for_each_entry_safe(struct dec_av1_task, t, tasks, list) {
      if (t->buf == priv->picture.av1.ref[idx])
                  }
      static bool dec_av1_GetStartedTask(vid_dec_PrivateType *priv,
         {
               ++priv->codec_data.av1.que_num;
   list_addtail(&task->list, &priv->codec_data.av1.started_tasks);
   if (priv->codec_data.av1.que_num <= 16)
            started_task = list_entry(priv->codec_data.av1.started_tasks.next,
         list_del(&started_task->list);
   list_addtail(&started_task->list, tasks);
               }
      static void dec_av1_ShowExistingframe(vid_dec_PrivateType *priv)
   {
      struct input_buf_private *inp = priv->in_buffers[0]->pInputPortPrivate;
   struct dec_av1_task *task, *existing_task;
            task = CALLOC_STRUCT(dec_av1_task);
   if (!task)
                     mtx_lock(&priv->codec_data.av1.mutex);
   dec_av1_MoveTask(&inp->tasks, &priv->codec_data.av1.finished_tasks);
   dec_av1_SortTask(priv);
   existing_task = dec_av1_SearchTask(priv, &priv->codec_data.av1.started_tasks);
   if (existing_task) {
      ++existing_task->buf_ref_count;
   task->buf = existing_task->buf;
   task->buf_ref = &existing_task->buf;
      } else {
      existing_task = dec_av1_SearchTask(priv, &priv->codec_data.av1.finished_tasks);
   if (existing_task) {
      struct vl_screen *omx_screen;
   struct pipe_screen *pscreen;
   struct pipe_video_buffer templat = {};
                                                memset(&templat, 0, sizeof(templat));
   templat.width = priv->codec->width;
   templat.height = priv->codec->height;
   templat.buffer_format = pscreen->get_video_param(
      pscreen,
   PIPE_VIDEO_PROFILE_UNKNOWN,
   PIPE_VIDEO_ENTRYPOINT_BITSTREAM,
      );
   templat.interlaced = false;
   buf = priv->pipe->create_video_buffer(priv->pipe, &templat);
   if (!buf) {
      FREE(task);
   mtx_unlock(&priv->codec_data.av1.mutex);
               box.width = priv->codec->width;
   box.height = priv->codec->height;
   box.depth = 1;
   priv->pipe->resource_copy_region(priv->pipe,
         ((struct vl_video_buffer *)buf)->resources[0],
   0, 0, 0, 0,
   ((struct vl_video_buffer *)(existing_task->buf))->resources[0],
   box.width /= 2;
   box.height/= 2;
   priv->pipe->resource_copy_region(priv->pipe,
         ((struct vl_video_buffer *)buf)->resources[1],
   0, 0, 0, 0,
   ((struct vl_video_buffer *)(existing_task->buf))->resources[1],
   priv->pipe->flush(priv->pipe, NULL, 0);
   existing_task->buf_ref_count = 0;
   task->buf = buf;
      } else {
      FREE(task);
   mtx_unlock(&priv->codec_data.av1.mutex);
         }
            fnd = dec_av1_GetStartedTask(priv, task, &inp->tasks);
   mtx_unlock(&priv->codec_data.av1.mutex);
   if (fnd)
      }
      static struct dec_av1_task *dec_av1_BeginFrame(vid_dec_PrivateType *priv)
   {
      struct input_buf_private *inp = priv->in_buffers[0]->pInputPortPrivate;
            if (priv->frame_started)
            if (!priv->codec) {
      struct vl_screen *omx_screen;
   struct pipe_screen *pscreen;
   struct pipe_video_codec templat = {};
            omx_screen = priv->screen;
            pscreen = omx_screen->pscreen;
            supported = vl_codec_supported(pscreen, priv->profile, false);
            templat.profile = priv->profile;
   templat.entrypoint = PIPE_VIDEO_ENTRYPOINT_BITSTREAM;
   templat.chroma_format = PIPE_VIDEO_CHROMA_FORMAT_420;
   templat.max_references = AV1_NUM_REF_FRAMES;
   templat.expect_chunked_decode = true;
   omx_base_video_PortType *port;
   port = (omx_base_video_PortType *)priv->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
   templat.width = port->sPortParam.format.video.nFrameWidth;
                        mtx_lock(&priv->codec_data.av1.mutex);
   dec_av1_MoveTask(&inp->tasks, &priv->codec_data.av1.finished_tasks);
   dec_av1_SortTask(priv);
            task = dec_av1_NeedTask(priv);
   if (!task)
            priv->codec->begin_frame(priv->codec, task->buf, &priv->picture.base);
               }
      static void dec_av1_EndFrame(vid_dec_PrivateType *priv, struct dec_av1_task *task)
   {
      struct input_buf_private *inp = priv->in_buffers[0]->pInputPortPrivate;
   unsigned refresh_frame_flags;
   bool fnd;
            if (!priv->frame_started || ! task)
            priv->codec->end_frame(priv->codec, task->buf, &priv->picture.base);
            refresh_frame_flags = priv->codec_data.av1.uncompressed_header.refresh_frame_flags;
   for (i = 0; i < AV1_NUM_REF_FRAMES; ++i) {
      if (refresh_frame_flags & (1 << i)) {
      memcpy(&priv->codec_data.av1.refs[i], &priv->codec_data.av1.uncompressed_header,
         priv->picture.av1.ref[i] = task->buf;
   priv->codec_data.av1.RefFrames[i].RefFrameType =
         priv->codec_data.av1.RefFrames[i].RefFrameId =
         priv->codec_data.av1.RefFrames[i].RefUpscaledWidth =
         priv->codec_data.av1.RefFrames[i].RefFrameWidth =
         priv->codec_data.av1.RefFrames[i].RefFrameHeight =
         priv->codec_data.av1.RefFrames[i].RefRenderWidth =
         priv->codec_data.av1.RefFrames[i].RefRenderHeight =
         }
   if (!priv->picture.av1.picture_parameter.pic_info_fields.show_frame)
            mtx_lock(&priv->codec_data.av1.mutex);
   fnd = dec_av1_GetStartedTask(priv, task, &priv->codec_data.av1.decode_tasks);
   if (!fnd) {
      mtx_unlock(&priv->codec_data.av1.mutex);
      }
   if (!priv->codec_data.av1.stacked_frame)
         mtx_unlock(&priv->codec_data.av1.mutex);
      }
      static void dec_av1_Decode(vid_dec_PrivateType *priv, struct vl_vlc *vlc,
         {
      unsigned start_bits_pos = vl_vlc_bits_left(vlc);
   unsigned start_bits = vl_vlc_valid_bits(vlc);
   unsigned start_bytes = start_bits / 8;
   const void *obu_data = vlc->data;
   uint8_t start_buf[8];
   unsigned num_buffers = 0;
   void * const * buffers[4];
   unsigned sizes[4];
   unsigned obu_size = 0;
   unsigned total_obu_len;
   enum av1_obu_type type;
   bool obu_extension_flag;
   bool obu_has_size_field;
            for (i = 0; i < start_bytes; ++i)
      start_buf[i] =
         /* obu header */
   av1_f(vlc, 1); /* obu_forbidden_bit */
   type = av1_f(vlc, 4);
   obu_extension_flag = av1_f(vlc, 1);
   obu_has_size_field = av1_f(vlc, 1);
   av1_f(vlc, 1); /* obu_reserved_1bit */
   if (obu_extension_flag) {
      priv->codec_data.av1.ext.temporal_id = av1_f(vlc, 3);
   priv->codec_data.av1.ext.spatial_id = av1_f(vlc, 2);
               obu_size = (obu_has_size_field) ? av1_uleb128(vlc) :
                  switch (type) {
   case AV1_OBU_SEQUENCE_HEADER: {
      sequence_header_obu(priv, vlc);
   av1_byte_alignment(vlc);
   priv->codec_data.av1.bs_obu_seq_sz = total_obu_len;
   memcpy(priv->codec_data.av1.bs_obu_seq_buf, start_buf, start_bytes);
   memcpy(priv->codec_data.av1.bs_obu_seq_buf + start_bytes, obu_data,
            }
   case AV1_OBU_TEMPORAL_DELIMITER:
      av1_byte_alignment(vlc);
   priv->codec_data.av1.bs_obu_td_sz = total_obu_len;
   memcpy(priv->codec_data.av1.bs_obu_td_buf, start_buf, total_obu_len);
      case AV1_OBU_FRAME_HEADER:
      frame_header_obu(priv, vlc);
   if (priv->codec_data.av1.uncompressed_header.show_existing_frame)
         av1_byte_alignment(vlc);
      case AV1_OBU_FRAME: {
               frame_header_obu(priv, vlc);
            parse_tile_hdr(priv, vlc, start_bits_pos, total_obu_len);
            task = dec_av1_BeginFrame(priv);
   if (!task)
            if (priv->codec_data.av1.bs_obu_td_sz) {
      buffers[num_buffers] = (void *)priv->codec_data.av1.bs_obu_td_buf;
   sizes[num_buffers++] = priv->codec_data.av1.bs_obu_td_sz;
      }
   if (priv->codec_data.av1.bs_obu_seq_sz) {
      buffers[num_buffers] = (void *)priv->codec_data.av1.bs_obu_seq_buf;
   sizes[num_buffers++] = priv->codec_data.av1.bs_obu_seq_sz;
      }
   buffers[num_buffers] = (void *)start_buf;
   sizes[num_buffers++] = start_bytes;
   buffers[num_buffers] = (void *)obu_data;
            priv->codec->decode_bitstream(priv->codec, priv->target,
            priv->codec_data.av1.stacked_frame =
            dec_av1_EndFrame(priv, task);
      }
   default:
      av1_byte_alignment(vlc);
                  }
      OMX_ERRORTYPE vid_dec_av1_AllocateInBuffer(omx_base_PortType *port,
               {
      struct input_buf_private *inp;
            r = base_port_AllocateBuffer(port, buf, idx, private, size);
   if (r)
            inp = (*buf)->pInputPortPrivate = CALLOC_STRUCT(input_buf_private);
   if (!inp) {
      base_port_FreeBuffer(port, idx, *buf);
                           }
      OMX_ERRORTYPE vid_dec_av1_UseInBuffer(omx_base_PortType *port,
               {
      struct input_buf_private *inp;
            r = base_port_UseBuffer(port, buf, idx, private, size, mem);
   if (r)
            inp = (*buf)->pInputPortPrivate = CALLOC_STRUCT(input_buf_private);
   if (!inp) {
      base_port_FreeBuffer(port, idx, *buf);
                           }
      void vid_dec_av1_FreeInputPortPrivate(vid_dec_PrivateType *priv,
         {
               if (!inp || !inp->tasks.next)
            list_for_each_entry_safe(struct dec_av1_task, task, &inp->tasks, list) {
      task->buf->destroy(task->buf);
         }
      void vid_dec_av1_ReleaseTasks(vid_dec_PrivateType *priv)
   {
      dec_av1_ReleaseTask(priv, &priv->codec_data.av1.free_tasks);
   dec_av1_ReleaseTask(priv, &priv->codec_data.av1.started_tasks);
   dec_av1_ReleaseTask(priv, &priv->codec_data.av1.decode_tasks);
   dec_av1_ReleaseTask(priv, &priv->codec_data.av1.finished_tasks);
      }
      void vid_dec_av1_FrameDecoded(OMX_COMPONENTTYPE *comp,
               {
      vid_dec_PrivateType *priv = comp->pComponentPrivate;
   bool eos = !!(input->nFlags & OMX_BUFFERFLAG_EOS);
   struct input_buf_private *inp = input->pInputPortPrivate;
   struct dec_av1_task *task;
            mtx_lock(&priv->codec_data.av1.mutex);
   if (list_length(&inp->tasks) > 1)
            if (list_is_empty(&inp->tasks)) {
      task = list_entry(priv->codec_data.av1.started_tasks.next,
         list_del(&task->list);
   list_addtail(&task->list, &inp->tasks);
                        if (!task->no_show_frame) {
      vid_dec_FillOutput(priv, task->buf, output);
   output->nFilledLen = output->nAllocLen;
      } else {
      task->no_show_frame = false;
               if (task->is_sef_task) {
      if (task->buf_ref_count == 0) {
      struct dec_av1_task *t = container_of(task->buf_ref, struct dec_av1_task, buf);
   list_del(&task->list);
   t->buf_ref_count--;
   list_del(&t->list);
      } else if (task->buf_ref_count == 1) {
      list_del(&task->list);
   task->buf->destroy(task->buf);
      }
      } else {
      if (task->buf_ref_count == 1) {
      list_del(&task->list);
   list_addtail(&task->list, &priv->codec_data.av1.finished_tasks);
      } else if (task->buf_ref_count == 2) {
      list_del(&task->list);
   task->buf_ref_count--;
                  if (eos && input->pInputPortPrivate) {
      if (!priv->codec_data.av1.que_num)
         else
      }
   else {
      if (!stacked)
      }
      }
      void vid_dec_av1_Init(vid_dec_PrivateType *priv)
   {
      priv->picture.base.profile = PIPE_VIDEO_PROFILE_AV1_MAIN;
   priv->Decode = dec_av1_Decode;
   list_inithead(&priv->codec_data.av1.free_tasks);
   list_inithead(&priv->codec_data.av1.started_tasks);
   list_inithead(&priv->codec_data.av1.decode_tasks);
   list_inithead(&priv->codec_data.av1.finished_tasks);
      }
