   /*
   * Copyright © Microsoft Corporation
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
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "d3d12_video_encoder_bitstream_builder_av1.h"
      void
   d3d12_video_bitstream_builder_av1::write_obu_header(d3d12_video_encoder_bitstream *pBit,
                           {
      pBit->put_bits(1, 0);          // obu_forbidden_bit
   pBit->put_bits(4, obu_type);   // type
   pBit->put_bits(1, obu_extension_flag);
   pBit->put_bits(1, 1);   // obu_has_size_field
   pBit->put_bits(1, 0);   // reserved
   if (obu_extension_flag) {
      // obu_extension_header()
   pBit->put_bits(3, temporal_id);
   pBit->put_bits(2, spatial_id);
         }
      void
   d3d12_video_bitstream_builder_av1::pack_obu_header_size(d3d12_video_encoder_bitstream *pBit, uint64_t val)
   {
         }
      void
   d3d12_video_bitstream_builder_av1::write_seq_data(d3d12_video_encoder_bitstream *pBit, const av1_seq_header_t *pSeqHdr)
   {
      pBit->put_bits(3, pSeqHdr->seq_profile);
   pBit->put_bits(1, 0);   // still_picture default 0
   pBit->put_bits(1, 0);   // reduced_still_picture_header
   pBit->put_bits(1, 0);   // timing_info_present_flag
            pBit->put_bits(5, pSeqHdr->operating_points_cnt_minus_1);
   for (uint8_t i = 0; i <= pSeqHdr->operating_points_cnt_minus_1; i++) {
      pBit->put_bits(8, pSeqHdr->operating_point_idc[i] >> 4);
   pBit->put_bits(4, pSeqHdr->operating_point_idc[i] & 0x9f);
   pBit->put_bits(5, pSeqHdr->seq_level_idx[i]);
   if (pSeqHdr->seq_level_idx[i] > 7)
               pBit->put_bits(4, d3d12_video_bitstream_builder_av1::frame_width_bits_minus_1);    // frame_width_bits_minus_1
   pBit->put_bits(4, d3d12_video_bitstream_builder_av1::frame_height_bits_minus_1);   // frame_height_bits_minus_1
   pBit->put_bits(d3d12_video_bitstream_builder_av1::frame_width_bits_minus_1 + 1,
         pBit->put_bits(d3d12_video_bitstream_builder_av1::frame_height_bits_minus_1 + 1,
         pBit->put_bits(1, 0);                                     // frame_id_numbers_present_flag
   pBit->put_bits(1, pSeqHdr->use_128x128_superblock);       // use_128x128_superblock
   pBit->put_bits(1, pSeqHdr->enable_filter_intra);          // enable_filter_intra
   pBit->put_bits(1, pSeqHdr->enable_intra_edge_filter);     // enable_intra_edge_filter
   pBit->put_bits(1, pSeqHdr->enable_interintra_compound);   // enable_interintra_compound
   pBit->put_bits(1, pSeqHdr->enable_masked_compound);       // enable_masked_compound
   pBit->put_bits(1, pSeqHdr->enable_warped_motion);         // enable_warped_motion
   pBit->put_bits(1, pSeqHdr->enable_dual_filter);           // enable_dual_filter
            if (pSeqHdr->enable_order_hint) {
      pBit->put_bits(1, pSeqHdr->enable_jnt_comp);        // enable_jnt_comp
               pBit->put_bits(1, pSeqHdr->seq_choose_screen_content_tools);   // seq_choose_screen_content_tools
   if (!pSeqHdr->seq_choose_screen_content_tools)
            if (pSeqHdr->seq_force_screen_content_tools) {
      pBit->put_bits(1, pSeqHdr->seq_choose_integer_mv);   // seq_choose_integer_mv
   if (!pSeqHdr->seq_choose_integer_mv)
               if (pSeqHdr->enable_order_hint)
            pBit->put_bits(1, pSeqHdr->enable_superres);      // enable_superres
   pBit->put_bits(1, pSeqHdr->enable_cdef);          // enable_cdef
            // color_config ()
   pBit->put_bits(1,
         if (pSeqHdr->seq_profile != 1)
                     if (pSeqHdr->color_config.color_description_present_flag) {
      pBit->put_bits(8, pSeqHdr->color_config.color_primaries);
   pBit->put_bits(8, pSeqHdr->color_config.transfer_characteristics);
                        if (pSeqHdr->seq_profile == 0)
                                 }
      void
   d3d12_video_bitstream_builder_av1::write_temporal_delimiter_obu(std::vector<uint8_t> &headerBitstream,
               {
      auto startByteOffset = std::distance(headerBitstream.begin(), placingPositionStart);
   if (headerBitstream.size() < (startByteOffset + c_DefaultBitstreamBufSize))
            d3d12_video_encoder_bitstream bitstream_full_obu;
            {
               // Write the header
   constexpr uint32_t obu_extension_flag = 0;
   constexpr uint32_t temporal_id = 0;
   constexpr uint32_t spatial_id = 0;
            // Write the data size
   const uint64_t obu_size_in_bytes = 0;
   debug_printf("obu_size: %" PRIu64 " (temporal_delimiter_obu() has empty payload as per AV1 codec spec)\n",
                              // Shrink headerBitstream to fit
   writtenBytes = bitstream_full_obu.get_byte_count() - startByteOffset;
      }
      void
   d3d12_video_bitstream_builder_av1::write_sequence_header(const av1_seq_header_t *pSeqHdr,
                     {
      auto startByteOffset = std::distance(headerBitstream.begin(), placingPositionStart);
   if (headerBitstream.size() < (startByteOffset + c_DefaultBitstreamBufSize))
            d3d12_video_encoder_bitstream bitstream_full_obu;
            // to handle variable length we first write the content
   // and later the obu header and concatenate both bitstreams
   d3d12_video_encoder_bitstream bitstream_seq;
            {
      // Write the data
   write_seq_data(&bitstream_seq, pSeqHdr);
   bitstream_seq.flush();
            // Write the header
   constexpr uint32_t obu_extension_flag = 0;
   constexpr uint32_t temporal_id = 0;
   constexpr uint32_t spatial_id = 0;
            // Write the data size
   const uint64_t obu_size_in_bytes = bitstream_seq.get_byte_count();
   debug_printf("obu_size: %" PRIu64 "\n", obu_size_in_bytes);
                     // bitstream_full_obu has external buffer allocation and
   // append_bitstream deep copies bitstream_seq, so it's okay
   // for RAII of bitstream_seq to be deallocated out of scope
                        // Shrink headerBitstream to fit
   writtenBytes = bitstream_full_obu.get_byte_count() - startByteOffset;
      }
      void
   d3d12_video_bitstream_builder_av1::write_frame_size_with_refs(d3d12_video_encoder_bitstream *pBit,
               {
      bool found_ref = false;   // Send explicitly as default
   for (int i = 0; i < 7 /*REFS_PER_FRAME*/; i++) {
                  if (found_ref) {
      // frame_size()
   write_frame_size(pBit, pSeqHdr, pPicHdr);
   // render_size()
      } else {
      // superres_params()
         }
      void
   d3d12_video_bitstream_builder_av1::write_frame_size(d3d12_video_encoder_bitstream *pBit,
               {
      if (pPicHdr->frame_size_override_flag) {
      pBit->put_bits(d3d12_video_bitstream_builder_av1::frame_width_bits_minus_1 + 1,
         pBit->put_bits(d3d12_video_bitstream_builder_av1::frame_height_bits_minus_1 + 1,
      }
   // superres_params()
      }
         void
   d3d12_video_bitstream_builder_av1::write_superres_params(d3d12_video_encoder_bitstream *pBit,
               {
      if (pSeqHdr->enable_superres)
            constexpr unsigned SUPERRES_DENOM_BITS = 3;   // As per AV1 codec spec
   if (pPicHdr->use_superres) {
      constexpr uint32_t SUPERRES_DENOM_MIN = 9;   // As per AV1 codec spec
   assert(pPicHdr->SuperresDenom >= SUPERRES_DENOM_MIN);
   uint32_t coded_denom = pPicHdr->SuperresDenom - SUPERRES_DENOM_MIN;
         }
      void
   d3d12_video_bitstream_builder_av1::write_render_size(d3d12_video_encoder_bitstream *pBit,
         {
      uint8_t render_and_frame_size_different =
                     if (render_and_frame_size_different == 1) {
      pBit->put_bits(16, pPicHdr->RenderWidth - 1);    // render_width_minus_1
         }
      void
   d3d12_video_bitstream_builder_av1::write_delta_q_value(d3d12_video_encoder_bitstream *pBit, int32_t delta_q_val)
   {
      if (delta_q_val) {
      pBit->put_bits(1, 1);
      } else {
            }
      inline int
   get_relative_dist(int a, int b, int OrderHintBits, uint8_t enable_order_hint)
   {
      if (!enable_order_hint)
         int diff = a - b;
   int m = 1 << (OrderHintBits - 1);
   diff = (diff & (m - 1)) - (diff & m);
      }
      void
   d3d12_video_bitstream_builder_av1::write_pic_data(d3d12_video_encoder_bitstream *pBit,
               {
                        if (pPicHdr->show_existing_frame) {
               // decoder_model_info_present_flag Default 0
   // if ( decoder_model_info_present_flag && !equal_picture_interval ) {
   //       temporal_point_info( )
            // frame_id_numbers_present_flag default 0
   // if ( frame_id_numbers_present_flag ) {
   //       display_frame_id	f(idLen)
                  const uint8_t FrameIsIntra = (pPicHdr->frame_type == D3D12_VIDEO_ENCODER_AV1_FRAME_TYPE_INTRA_ONLY_FRAME ||
                  pBit->put_bits(1, pPicHdr->show_frame);   // show_frame
   if (!pPicHdr->show_frame)
               if (pPicHdr->frame_type == D3D12_VIDEO_ENCODER_AV1_FRAME_TYPE_SWITCH_FRAME ||
      (pPicHdr->frame_type == D3D12_VIDEO_ENCODER_AV1_FRAME_TYPE_KEY_FRAME && pPicHdr->show_frame)) {
      } else {
                  pBit->put_bits(1, pPicHdr->disable_cdf_update);   // disable_cdf_update
   if (pSeqHdr->seq_force_screen_content_tools == /*SELECT_SCREEN_CONTENT_TOOLS */ 2)
            if (pPicHdr->allow_screen_content_tools && (pSeqHdr->seq_force_integer_mv == /*SELECT_INTEGER_MV */ 2))
            // reduced_still_picture_header default 0 and frame_type != SWITCH
   if (pPicHdr->frame_type != D3D12_VIDEO_ENCODER_AV1_FRAME_TYPE_SWITCH_FRAME) {
      // Expicitly coded if NOT SWITCH FRAME
      } else {
      assert(pPicHdr->frame_size_override_flag ==
                        if (!(FrameIsIntra || pPicHdr->error_resilient_mode))
                     if (!(pPicHdr->frame_type == D3D12_VIDEO_ENCODER_AV1_FRAME_TYPE_SWITCH_FRAME ||
                  constexpr uint32_t allFrames = (1 << 8 /* NUM_REF_FRAMES from AV1 spec */) - 1;
               if (pPicHdr->error_resilient_mode && pSeqHdr->enable_order_hint) {
      for (uint8_t i = 0; i < 8 /* NUM_REF_FRAMES from AV1 spec */; i++) {
      pBit->put_bits(pSeqHdr->order_hint_bits_minus1 + 1,
                     if (FrameIsIntra) {
      // frame_size()
   write_frame_size(pBit, pSeqHdr, pPicHdr);
                  if (pPicHdr->allow_screen_content_tools && pPicHdr->UpscaledWidth == pPicHdr->FrameWidth)
      } else {
                                             if (pPicHdr->frame_size_override_flag && !pPicHdr->error_resilient_mode) {
      // frame_size_with_refs()
      } else {
      // frame_size()
   write_frame_size(pBit, pSeqHdr, pPicHdr);
   // render_size()
                              // read_interpolation_filter()
   {
      const uint8_t is_filter_switchable =
         pBit->put_bits(1, is_filter_switchable);   // is_filter_switchable
   if (!is_filter_switchable) {
                              if (!(pPicHdr->error_resilient_mode || !pPicHdr->use_ref_frame_mvs))
               if (!pPicHdr->disable_cdf_update /* || reduced_still_picture_header default 0 */)
            // tile_info()
   {
                     unsigned minLog2TileCols = log2(pPicHdr->tile_info.tile_support_caps.MinTileCols);
                  unsigned minLog2TileRows = log2(pPicHdr->tile_info.tile_support_caps.MinTileRows);
                           if (pPicHdr->tile_info.uniform_tile_spacing_flag) {
      for (unsigned i = minLog2TileCols; i < log2TileCols; i++)
         if (log2TileCols < maxLog2TileCols)
         for (unsigned i = minLog2TileRows; i < log2TileRows; i++)
         if (log2TileRows < maxLog2TileRows)
      } else {
      unsigned sizeSb = 0;
   unsigned widestTileSb = 0;
   unsigned widthSb = pPicHdr->frame_width_sb;
   for (unsigned i = 0; i < pPicHdr->tile_info.tile_partition.ColCount; i++) {
      sizeSb = pPicHdr->tile_info.tile_partition.ColWidths[i];
   unsigned maxWidth = std::min(widthSb, maxTileWidthSb);
   pBit->put_ns_bits(maxWidth, sizeSb - 1);   // width_in_sbs_minus_1
                     unsigned maxTileHeightSb = std::max(maxTileAreaSb / widestTileSb, 1u);
   unsigned heightSb = pPicHdr->frame_height_sb;
   for (unsigned i = 0; i < pPicHdr->tile_info.tile_partition.RowCount; i++) {
      sizeSb = pPicHdr->tile_info.tile_partition.RowHeights[i];
   unsigned maxHeight = std::min(heightSb, maxTileHeightSb);
   pBit->put_ns_bits(maxHeight, sizeSb - 1);   // height_in_sbs_minus_1
                  if (log2TileCols > 0 || log2TileRows > 0) {
      pBit->put_bits(log2TileRows + log2TileCols,
         pBit->put_bits(2, pPicHdr->tile_info.tile_support_caps.TileSizeBytesMinus1);   // tile_size_bytes_minus_1
                  // quantization_params()
   {
                     bool diff_uv_delta = false;
   if (pPicHdr->quantization_params.UDCDeltaQ != pPicHdr->quantization_params.VDCDeltaQ ||
                                                               if (diff_uv_delta) {
      write_delta_q_value(pBit, pPicHdr->quantization_params.VDCDeltaQ);
               pBit->put_bits(1, pPicHdr->quantization_params.UsingQMatrix);   // using_qmatrix
   if (pPicHdr->quantization_params.UsingQMatrix) {
      pBit->put_bits(4, pPicHdr->quantization_params.QMY);   // qm_y
   pBit->put_bits(4, pPicHdr->quantization_params.QMU);   // qm_u
   if (pSeqHdr->color_config.separate_uv_delta_q)
                  // segmentation_params()
   {
      pBit->put_bits(1, pPicHdr->segmentation_enabled);   // segmentation_enabled
   if (pPicHdr->segmentation_enabled) {
      if (pPicHdr->primary_ref_frame != 7 /*PRIMARY_REF_NONE*/) {
      pBit->put_bits(1, pPicHdr->segmentation_config.UpdateMap);   // segmentation_update_map f(1)
   if (pPicHdr->segmentation_config.UpdateMap == 1)
                     if (pPicHdr->segmentation_config.UpdateData == 1) {
                     for (int i = 0; i < 8 /*MAX_SEGMENTS*/; i++) {
      for (int j = 0; j < 8 /*SEG_LVL_MAX*/; j++) {
                           if (feature_enabled) {
      int bitsToRead = av1_segmentation_feature_bits[j];
   if (av1_segmentation_feature_signed[j] == 1) {
      pBit->put_su_bits(
      1 + bitsToRead,
   } else {
      pBit->put_bits(
      bitsToRead,
                              // delta_q_params()
   // combined with delta_lf_params()
   {
      if (pPicHdr->quantization_params.BaseQIndex)
                           // delta_lf_params()
   if (!pPicHdr->allow_intrabc) {
      pBit->put_bits(1, pPicHdr->delta_lf_params.DeltaLFPresent);   // delta_lf_present
   if (pPicHdr->delta_lf_params.DeltaLFPresent) {
      pBit->put_bits(2, pPicHdr->delta_lf_params.DeltaLFRes);     // delta_lf_res
                        constexpr bool CodedLossless = false;   // CodedLossless default 0
   constexpr bool AllLossless = false;     // AllLossless default 0
   // loop_filter_params()
   {
      if (!(CodedLossless || pPicHdr->allow_intrabc)) {
                     if (pPicHdr->loop_filter_params.LoopFilterLevel[0] || pPicHdr->loop_filter_params.LoopFilterLevel[1]) {
                                       if (pPicHdr->loop_filter_params.LoopFilterDeltaEnabled) {
      bool loop_filter_delta_update =
         pBit->put_bits(1, loop_filter_delta_update);   // loop_filter_delta_update
   if (loop_filter_delta_update) {
      constexpr uint8_t TOTAL_REFS_PER_FRAME = 8;   // From AV1 spec
   static_assert(ARRAY_SIZE(pPicHdr->loop_filter_params.RefDeltas) == TOTAL_REFS_PER_FRAME);
   for (uint8_t i = 0; i < TOTAL_REFS_PER_FRAME; i++) {
      pBit->put_bits(1, pPicHdr->loop_filter_params.UpdateRefDelta);   // loop_filter_delta_update
                           static_assert(ARRAY_SIZE(pPicHdr->loop_filter_params.ModeDeltas) == 2);   // From AV1 spec
   for (uint8_t i = 0; i < 2; i++) {
      pBit->put_bits(1, pPicHdr->loop_filter_params.UpdateModeDelta);   // update_mode_delta
   if (pPicHdr->loop_filter_params.UpdateModeDelta) {
                                 // cdef_params()
   {
      if (!(!pSeqHdr->enable_cdef || CodedLossless || pPicHdr->allow_intrabc)) {
      uint16_t num_planes = 3;                                     // mono_chrome not supported
   pBit->put_bits(2, pPicHdr->cdef_params.CdefDampingMinus3);   // cdef_damping_minus_3
   pBit->put_bits(2, pPicHdr->cdef_params.CdefBits);            // cdef_bits
   for (uint16_t i = 0; i < (1 << pPicHdr->cdef_params.CdefBits); ++i) {
      pBit->put_bits(4, pPicHdr->cdef_params.CdefYPriStrength[i]);   // cdef_y_pri_strength[i]
   pBit->put_bits(2, pPicHdr->cdef_params.CdefYSecStrength[i]);   // cdef_y_sec_strength[i]
   if (num_planes > 1) {
      pBit->put_bits(4, pPicHdr->cdef_params.CdefUVPriStrength[i]);   // cdef_uv_pri_strength[i]
                        // lr_params()
   {
      if (!(AllLossless || pPicHdr->allow_intrabc || !pSeqHdr->enable_restoration)) {
      bool uses_lr = false;
   bool uses_chroma_lr = false;
   for (int i = 0; i < 3 /*MaxNumPlanes*/; i++) {
      pBit->put_bits(2, pPicHdr->lr_params.lr_type[i]);
   if (pPicHdr->lr_params.lr_type[i] != D3D12_VIDEO_ENCODER_AV1_RESTORATION_TYPE_DISABLED) {
      uses_lr = true;
   if (i > 0)
                                                         if (pSeqHdr->color_config.subsampling_x && pSeqHdr->color_config.subsampling_y && uses_chroma_lr) {
                           // read_tx_mode()
   {
      const uint8_t tx_mode_select = (pPicHdr->TxMode == D3D12_VIDEO_ENCODER_AV1_TX_MODE_SELECT) ? 1 : 0;
   if (!CodedLossless)
               // frame_reference_mode()
   {
      if (!FrameIsIntra)
               // skip_mode_params()
   {
      uint8_t skipModeAllowed = 0;
   if (!(FrameIsIntra || !pPicHdr->reference_select || !pSeqHdr->enable_order_hint)) {
      int forwardIdx = -1;
   int backwardIdx = -1;
   int forwardHint = 0;
   int backwardHint = 0;
   for (int i = 0; i < 7 /*REFS_PER_FRAME*/; i++) {
      uint32_t refHint = pPicHdr->ref_order_hint[pPicHdr->ref_frame_idx[i]];
   if (get_relative_dist(refHint,
                        if (forwardIdx < 0 || get_relative_dist(refHint,
                        forwardIdx = i;
         } else if (get_relative_dist(refHint,
                        if (backwardIdx < 0 || get_relative_dist(refHint,
                        backwardIdx = i;
            }
   if (forwardIdx < 0) {
         } else if (backwardIdx >= 0) {
         } else {
      int secondForwardIdx = -1;
   int secondForwardHint = 0;
   for (int i = 0; i < 7 /*REFS_PER_FRAME*/; i++) {
      uint32_t refHint = pPicHdr->ref_order_hint[pPicHdr->ref_frame_idx[i]];
   if (get_relative_dist(refHint,
                        if (secondForwardIdx < 0 || get_relative_dist(refHint,
                        secondForwardIdx = i;
            }
   if (secondForwardIdx < 0) {
         } else {
                                       if (!(FrameIsIntra || pPicHdr->error_resilient_mode || !pSeqHdr->enable_warped_motion)) {
                              // global_motion_params()
   {
      if (!FrameIsIntra) {
      for (uint8_t i = 0; i < 7; i++) {
      pBit->put_bits(1, 0);   // is_global[7]
   // Unimplemented: Enable global_motion_params with ref_global_motion_info
   assert(pPicHdr->ref_global_motion_info[i].TransformationType ==
                     // film_grain_params()
   // constexpr uint8_t film_grain_params_present = 0; // film_grain_params_present default 0
   // {
   // if (!(!film_grain_params_present || (!pPicHdr->show_frame && !pPicHdr->showable_frame))
   // ... this will be unreachable as film_grain_params_present is zero.
         }
      void
   d3d12_video_bitstream_builder_av1::write_frame_header(const av1_seq_header_t *pSeqHdr,
                                       {
      assert((frame_pack_type == OBU_FRAME) || (frame_pack_type == OBU_FRAME_HEADER));
   auto startByteOffset = std::distance(headerBitstream.begin(), placingPositionStart);
   if (headerBitstream.size() < (startByteOffset + c_DefaultBitstreamBufSize))
            d3d12_video_encoder_bitstream bitstream_full_obu;
            // to handle variable length we first write the content
   // and later the obu header and concatenate both bitstreams
   d3d12_video_encoder_bitstream bitstream_pic;
            {
      // Write frame_header_obu()
            debug_printf("frame_header_obu() bytes (without OBU_FRAME nor OBU_FRAME_HEADER alignment padding): %" PRId32 "\n",
         debug_printf("extra_obu_size_bytes (ie. tile_group_obu_size if writing OBU_FRAME ): %" PRIu64 "\n",
            // Write the obu_header
   constexpr uint32_t obu_extension_flag = 0;
   constexpr uint32_t temporal_id = 0;
   constexpr uint32_t spatial_id = 0;
            if (frame_pack_type == OBU_FRAME) {
      // Required byte_alignment() in frame_obu() after frame_header_obu()
   bitstream_pic.put_aligning_bits();
      } else if (frame_pack_type == OBU_FRAME_HEADER) {
      // whole open_bitstream_unit() for OBU_FRAME_HEADER
   // required in open_bitstream_unit () for OBU_FRAME_HEADER
   bitstream_pic.put_trailing_bits();
   debug_printf("Adding trailing_bits() after frame_header_obu() for OBU_FRAME\n");
                        // Write the obu_size element
   const uint64_t obu_size_in_bytes = bitstream_pic.get_byte_count() + extra_obu_size_bytes;
   debug_printf("obu_size: %" PRIu64 "\n", obu_size_in_bytes);
                     // bitstream_full_obu has external buffer allocation and
   // append_bitstream deep copies bitstream_pic, so it's okay
   // for RAII of bitstream_pic to be deallocated out of scope
                        // Shrink headerBitstream to fit
   writtenBytes = bitstream_full_obu.get_byte_count() - startByteOffset;
      }
      void
   d3d12_video_bitstream_builder_av1::calculate_tile_group_obu_size(
      const D3D12_VIDEO_ENCODER_OUTPUT_METADATA *pParsedMetadata,
   const D3D12_VIDEO_ENCODER_FRAME_SUBREGION_METADATA *pFrameSubregionMetadata,
   size_t TileSizeBytes,   // Pass already +1'd from TileSizeBytesMinus1
   const D3D12_VIDEO_ENCODER_AV1_PICTURE_CONTROL_SUBREGIONS_LAYOUT_DATA_TILES &TilesPartition,
   const av1_tile_group_t &tileGroup,
   size_t &tile_group_obu_size,
      {
               uint8_t NumTiles = TilesPartition.ColCount * TilesPartition.RowCount;
   if (NumTiles > 1)
            bool tile_start_and_end_present_flag = !(tileGroup.tg_start == 0 && (tileGroup.tg_end == (NumTiles - 1)));
   if (!(NumTiles == 1 || !tile_start_and_end_present_flag)) {
      uint8_t tileBits = log2(TilesPartition.ColCount) + log2(TilesPartition.RowCount);
   tile_group_obu_size_bits += tileBits;   // tg_start	f(tileBits)
               while (tile_group_obu_size_bits & 7)   // byte_alignment()
            decode_tile_elements_size = 0;
   for (UINT64 TileIdx = tileGroup.tg_start; TileIdx <= tileGroup.tg_end; TileIdx++) {
      // tile_size_minus_1	not coded for last tile
   if ((TileIdx != tileGroup.tg_end))
            size_t tile_effective_bytes_size =
         decode_tile_elements_size += tile_effective_bytes_size;
               assert((tile_group_obu_size_bits % 8) == 0);
      }
      void
   d3d12_video_bitstream_builder_av1::write_obu_tile_group_header(size_t tile_group_obu_size,
                     {
      auto startByteOffset = std::distance(headerBitstream.begin(), placingPositionStart);
   if (headerBitstream.size() < (startByteOffset + c_DefaultBitstreamBufSize))
            d3d12_video_encoder_bitstream bitstream_full_obu;
            // Write the obu_header
   constexpr uint32_t obu_extension_flag = 0;
   constexpr uint32_t temporal_id = 0;
   constexpr uint32_t spatial_id = 0;
            // tile_group_obu() will be copied by get_feedback from EncodeFrame output
   // we have to calculate its size anyways using the metadata for the obu_header.
   // so we just add below the argument tile_group_obu_size informing about the
   // tile_group_obu() byte size
            // Write the obu_size element
   pack_obu_header_size(&bitstream_full_obu, tile_group_obu_size);
                     // Shrink headerBitstream to fit
   writtenBytes = bitstream_full_obu.get_byte_count() - startByteOffset;
      }
