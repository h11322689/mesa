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
      #include "d3d12_video_dec.h"
   #include "d3d12_video_dec_vp9.h"
   #include <cmath>
      void
   d3d12_video_decoder_refresh_dpb_active_references_vp9(struct d3d12_video_decoder *pD3D12Dec)
   {
   // Method overview
      // 1. Codec specific strategy in switch statement regarding reference frames eviction policy. Should only mark active
   // DPB references, leaving evicted ones as unused
   // 2. Call release_unused_references_texture_memory(); at the end of this method. Any references (and texture
   // allocations associated)
            // Assign DXVA original Index indices to current frame and references
            for (uint8_t i = 0; i < _countof(pCurrPicParams->ref_frame_map); i++) {
      if (pD3D12Dec->m_pCurrentReferenceTargets[i]) {
      pCurrPicParams->ref_frame_map[i].Index7Bits =
                  for (uint8_t i = 0; i < _countof(pCurrPicParams->frame_refs); i++) {
      if(!pCurrPicParams->frame_refs[i].AssociatedFlag)
               pD3D12Dec->m_spDPBManager->mark_all_references_as_unused();
            // Releases the underlying reference picture texture objects of all references that were not marked as used in this
   // method.
               }
      void
   d3d12_video_decoder_get_frame_info_vp9(
         {
      auto pPicParams = d3d12_video_decoder_get_current_dxva_picparams<DXVA_PicParams_VP9>(pD3D12Dec);
   *pWidth = pPicParams->width;
   *pHeight = pPicParams->height;
      /*
      – The VP9 decoder maintains a pool (ref_frame_map[]) of 8 reference pictures at all times. Each 
   frame picks 3 reference frames (frame_refs[]) from the pool to use for inter prediction (known as Last, 
      */
   *pMaxDPB = 8 + 1 /*current picture*/;
      }
      void
   d3d12_video_decoder_prepare_current_frame_references_vp9(struct d3d12_video_decoder *pD3D12Dec,
               {
      DXVA_PicParams_VP9 *pPicParams = d3d12_video_decoder_get_current_dxva_picparams<DXVA_PicParams_VP9>(pD3D12Dec);
   pPicParams->CurrPic.Index7Bits = pD3D12Dec->m_spDPBManager->store_future_reference(pPicParams->CurrPic.Index7Bits,
                     pD3D12Dec->m_spDPBManager->update_entries(
      d3d12_video_decoder_get_current_dxva_picparams<DXVA_PicParams_VP9>(pD3D12Dec)->frame_refs,
         pD3D12Dec->m_spDPBManager->update_entries(
      d3d12_video_decoder_get_current_dxva_picparams<DXVA_PicParams_VP9>(pD3D12Dec)->ref_frame_map,
                  // Schedule reverse (back to common) transitions before command list closes for current frame
   for (auto BarrierDesc : pD3D12Dec->m_transitionsStorage) {
      std::swap(BarrierDesc.Transition.StateBefore, BarrierDesc.Transition.StateAfter);
               debug_printf(
         d3d12_video_decoder_log_pic_params_vp9(
      }
      void
   d3d12_video_decoder_log_pic_params_vp9(DXVA_PicParams_VP9 *pPicParams)
      {
   debug_printf("\n=============================================\n");
            debug_printf("CurrPic.Index7Bits = %d\n", pPicParams->CurrPic.Index7Bits);
   debug_printf("CurrPic.AssociatedFlag = %d\n", pPicParams->CurrPic.AssociatedFlag);
   debug_printf("profile = %d\n", pPicParams->profile);
   debug_printf("frame_type = %d\n", pPicParams->frame_type);
   debug_printf("show_frame = %d\n", pPicParams->show_frame);
   debug_printf("error_resilient_mode = %d\n", pPicParams->error_resilient_mode);
   debug_printf("subsampling_x = %d\n", pPicParams->subsampling_x);
   debug_printf("subsampling_y = %d\n", pPicParams->subsampling_y);
   debug_printf("extra_plane = %d\n", pPicParams->extra_plane);
   debug_printf("refresh_frame_context = %d\n", pPicParams->refresh_frame_context);
   debug_printf("frame_parallel_decoding_mode = %d\n", pPicParams->frame_parallel_decoding_mode);
   debug_printf("intra_only = %d\n", pPicParams->intra_only);
   debug_printf("frame_context_idx = %d\n", pPicParams->frame_context_idx);
   debug_printf("reset_frame_context = %d\n", pPicParams->reset_frame_context);
   debug_printf("allow_high_precision_mv = %d\n", pPicParams->allow_high_precision_mv);
   debug_printf("ReservedFormatInfo2Bits = %d\n", pPicParams->ReservedFormatInfo2Bits);
   debug_printf("wFormatAndPictureInfoFlags = %d\n", pPicParams->wFormatAndPictureInfoFlags);
   debug_printf("width = %d\n", pPicParams->width);
   debug_printf("height = %d\n", pPicParams->height);
   debug_printf("BitDepthMinus8Luma = %d\n", pPicParams->BitDepthMinus8Luma);
   debug_printf("BitDepthMinus8Chroma = %d\n", pPicParams->BitDepthMinus8Chroma);
   debug_printf("interp_filter = %d\n", pPicParams->interp_filter);
            for (uint32_t i = 0; i < _countof(pPicParams->ref_frame_map); i++) {
      debug_printf("ref_frame_map[%d].Index7Bits = %d\n", i, pPicParams->ref_frame_map[i].Index7Bits);
               for (uint32_t i = 0; i < _countof(pPicParams->ref_frame_coded_width); i++)
            for (uint32_t i = 0; i < _countof(pPicParams->ref_frame_coded_height); i++)
            for (uint32_t i = 0; i < _countof(pPicParams->frame_refs); i++) {
      debug_printf("frame_refs[%d].Index7Bits = %d\n", i, pPicParams->frame_refs[i].Index7Bits);
               for (uint32_t i = 0; i < _countof(pPicParams->ref_frame_sign_bias); i++)
            debug_printf("filter_level = %d\n", pPicParams->filter_level);
   debug_printf("sharpness_level = %d\n", pPicParams->sharpness_level);
   debug_printf("mode_ref_delta_enabled = %d\n", pPicParams->mode_ref_delta_enabled);
   debug_printf("mode_ref_delta_update = %d\n", pPicParams->mode_ref_delta_update);
   debug_printf("use_prev_in_find_mv_refs = %d\n", pPicParams->use_prev_in_find_mv_refs);
   debug_printf("ReservedControlInfo5Bits = %d\n", pPicParams->ReservedControlInfo5Bits);
            for (uint32_t i = 0; i < _countof(pPicParams->ref_deltas); i++)
         for (uint32_t i = 0; i < _countof(pPicParams->mode_deltas); i++)
            debug_printf("base_qindex = %d\n", pPicParams->base_qindex);
   debug_printf("y_dc_delta_q = %d\n", pPicParams->y_dc_delta_q);
   debug_printf("uv_dc_delta_q = %d\n", pPicParams->uv_dc_delta_q);
            debug_printf("stVP9Segments.enabled = %d\n", pPicParams->stVP9Segments.enabled);
   debug_printf("stVP9Segments.update_map = %d\n", pPicParams->stVP9Segments.update_map);
   debug_printf("stVP9Segments.temporal_update = %d\n", pPicParams->stVP9Segments.temporal_update);
   debug_printf("stVP9Segments.abs_delta = %d\n", pPicParams->stVP9Segments.abs_delta);
   debug_printf("stVP9Segments.ReservedSegmentFlags4Bits = %d\n", pPicParams->stVP9Segments.ReservedSegmentFlags4Bits);
            for (uint32_t i = 0; i < _countof(pPicParams->stVP9Segments.tree_probs); i++)
            for (uint32_t i = 0; i < _countof(pPicParams->stVP9Segments.pred_probs); i++)
            for (uint32_t i = 0; i < _countof(pPicParams->stVP9Segments.feature_data); i++)
      for (uint32_t j = 0; j < _countof(pPicParams->stVP9Segments.feature_data[0]); j++)
         for (uint32_t i = 0; i < _countof(pPicParams->stVP9Segments.feature_mask); i++)
            debug_printf("log2_tile_cols = %d\n", pPicParams->log2_tile_cols);
   debug_printf("log2_tile_rows = %d\n", pPicParams->log2_tile_rows);
   debug_printf("uncompressed_header_size_byte_aligned = %d\n", pPicParams->uncompressed_header_size_byte_aligned);
   debug_printf("first_partition_size = %d\n", pPicParams->first_partition_size);
   debug_printf("Reserved16Bits = %d\n", pPicParams->Reserved16Bits);
   debug_printf("Reserved32Bits = %d\n", pPicParams->Reserved32Bits);
      }
      void
   d3d12_video_decoder_prepare_dxva_slices_control_vp9(struct d3d12_video_decoder *pD3D12Dec,
               {
      if(!picture_vp9->slice_parameter.slice_info_present)
   {
                  debug_printf("[d3d12_video_decoder_vp9] Upper layer reported %d slices for this frame, parsing them below...\n",
            uint64_t TotalSlicesDXVAArrayByteSize = picture_vp9->slice_parameter.slice_count * sizeof(DXVA_Slice_VPx_Short);
            uint8_t* pData = vecOutSliceControlBuffers.data();
   for (uint32_t sliceIdx = 0; sliceIdx < picture_vp9->slice_parameter.slice_count; sliceIdx++)
   {
      DXVA_Slice_VPx_Short currentSliceEntry = {};
   // From DXVA Spec
   // wBadSliceChopping
   // 0	All bits for the slice are located within the corresponding bitstream data buffer. 
   // 1	The bitstream data buffer contains the start of the slice, but not the entire slice, because the buffer is full. 
   // 2	The bitstream data buffer contains the end of the slice. It does not contain the start of the slice, because the start of the slice was located in the previous bitstream data buffer. 
   // 3	The bitstream data buffer does not contain the start of the slice (because the start of the slice was located in the previous bitstream data buffer),
            switch (picture_vp9->slice_parameter.slice_data_flag[sliceIdx]) {
      /* whole slice is in the buffer */
   case PIPE_SLICE_BUFFER_PLACEMENT_TYPE_WHOLE:
      currentSliceEntry.wBadSliceChopping = 0u;
      /* The beginning of the slice is in the buffer but the end is not */
   case PIPE_SLICE_BUFFER_PLACEMENT_TYPE_BEGIN:
      currentSliceEntry.wBadSliceChopping = 1u;
      /* Neither beginning nor end of the slice is in the buffer */
   case PIPE_SLICE_BUFFER_PLACEMENT_TYPE_MIDDLE:
      currentSliceEntry.wBadSliceChopping = 3u;
      /* end of the slice is in the buffer */
   case PIPE_SLICE_BUFFER_PLACEMENT_TYPE_END:
      currentSliceEntry.wBadSliceChopping = 2u;
      default:
   {
                     currentSliceEntry.SliceBytesInBuffer = picture_vp9->slice_parameter.slice_data_size[sliceIdx];
            debug_printf("[d3d12_video_decoder_vp9] Detected slice index %" PRIu32 " with SliceBytesInBuffer %d - BSNALunitDataLocation %d - wBadSliceChopping: %" PRIu16
               " for frame with "
   "fenceValue: %d\n",
   sliceIdx,
   currentSliceEntry.SliceBytesInBuffer,
            memcpy(pData, &currentSliceEntry, sizeof(DXVA_Slice_VPx_Short));
      }
      }
      DXVA_PicParams_VP9
   d3d12_video_decoder_dxva_picparams_from_pipe_picparams_vp9(
      struct d3d12_video_decoder *pD3D12Dec,
   pipe_video_profile profile,
      {
      uint32_t frameNum = pD3D12Dec->m_fenceValue;
   DXVA_PicParams_VP9 dxvaStructure;   
            dxvaStructure.profile = pipe_vp9->picture_parameter.profile;
   dxvaStructure.wFormatAndPictureInfoFlags = ((pipe_vp9->picture_parameter.pic_fields.frame_type != 0)   <<  0) |
                                    ((pipe_vp9->picture_parameter.pic_fields.show_frame != 0)             <<  1) |
   (pipe_vp9->picture_parameter.pic_fields.error_resilient_mode          <<  2) |
   (pipe_vp9->picture_parameter.pic_fields.subsampling_x                 <<  3) |
   (pipe_vp9->picture_parameter.pic_fields.subsampling_y                 <<  4) |
   (0                                                                    <<  5) |
   (pipe_vp9->picture_parameter.pic_fields.refresh_frame_context         <<  6) |
         dxvaStructure.width  = pipe_vp9->picture_parameter.frame_width;
   dxvaStructure.height = pipe_vp9->picture_parameter.frame_height;
   dxvaStructure.BitDepthMinus8Luma   = pipe_vp9->picture_parameter.bit_depth - 8;
   dxvaStructure.BitDepthMinus8Chroma = pipe_vp9->picture_parameter.bit_depth - 8;
   dxvaStructure.interp_filter = pipe_vp9->picture_parameter.pic_fields.mcomp_filter_type;
   dxvaStructure.Reserved8Bits = 0;
   for (uint32_t i = 0; i < 8; i++) {
      if (pipe_vp9->ref[i]) {
      dxvaStructure.ref_frame_coded_width[i]  = pipe_vp9->ref[i]->width;
      } else
               /* DXVA spec The enums and indices for ref_frame_sign_bias[] are defined */
   const uint8_t signbias_last_index = 1;
   const uint8_t signbias_golden_index = 2;
            /* AssociatedFlag When Index7Bits does not contain an index to a valid uncompressed surface, the value shall be set to 127, to indicate that the index is invalid. */
            if (pipe_vp9->ref[pipe_vp9->picture_parameter.pic_fields.last_ref_frame]) {
      /* AssociatedFlag When Index7Bits does not contain an index to a valid uncompressed surface, the value shall be set to 127, to indicate that the index is invalid. */
   /* Mark AssociatedFlag = 0 so last_ref_frame will be replaced with the correct reference index in d3d12_video_decoder_refresh_dpb_active_references_vp9 */
   dxvaStructure.frame_refs[0].AssociatedFlag = 0;
   dxvaStructure.frame_refs[0].Index7Bits = pipe_vp9->picture_parameter.pic_fields.last_ref_frame;
               if (pipe_vp9->ref[pipe_vp9->picture_parameter.pic_fields.golden_ref_frame]) {
      /* AssociatedFlag When Index7Bits does not contain an index to a valid uncompressed surface, the value shall be set to 127, to indicate that the index is invalid. */
   /* Mark AssociatedFlag = 0 so golden_ref_frame will be replaced with the correct reference index in d3d12_video_decoder_refresh_dpb_active_references_vp9 */
   dxvaStructure.frame_refs[1].AssociatedFlag = 0;
   dxvaStructure.frame_refs[1].Index7Bits = pipe_vp9->picture_parameter.pic_fields.golden_ref_frame;
            if (pipe_vp9->ref[pipe_vp9->picture_parameter.pic_fields.alt_ref_frame]) {
         /* AssociatedFlag When Index7Bits does not contain an index to a valid uncompressed surface, the value shall be set to 127, to indicate that the index is invalid. */
   /* Mark AssociatedFlag = 0 so alt_ref_frame will be replaced with the correct reference index in d3d12_video_decoder_refresh_dpb_active_references_vp9 */
   dxvaStructure.frame_refs[2].AssociatedFlag = 0;
   dxvaStructure.frame_refs[2].Index7Bits = pipe_vp9->picture_parameter.pic_fields.alt_ref_frame;
               dxvaStructure.filter_level    = pipe_vp9->picture_parameter.filter_level;
            bool use_prev_in_find_mv_refs =
      !pipe_vp9->picture_parameter.pic_fields.error_resilient_mode &&
   !(pipe_vp9->picture_parameter.pic_fields.frame_type == 0 /*KEY_FRAME*/ || pipe_vp9->picture_parameter.pic_fields.intra_only) &&
   pipe_vp9->picture_parameter.pic_fields.prev_show_frame &&
   pipe_vp9->picture_parameter.frame_width == pipe_vp9->picture_parameter.prev_frame_width &&
         dxvaStructure.wControlInfoFlags = (pipe_vp9->picture_parameter.mode_ref_delta_enabled  << 0) |
                        for (uint32_t i = 0; i < 4; i++)
            for (uint32_t i = 0; i < 2; i++)
            dxvaStructure.base_qindex   = pipe_vp9->picture_parameter.base_qindex;
   dxvaStructure.y_dc_delta_q  = pipe_vp9->picture_parameter.y_dc_delta_q;
   dxvaStructure.uv_dc_delta_q = pipe_vp9->picture_parameter.uv_ac_delta_q;
            /* segmentation data */
   dxvaStructure.stVP9Segments.wSegmentInfoFlags = (pipe_vp9->picture_parameter.pic_fields.segmentation_enabled   << 0) |
                              for (uint32_t i = 0; i < 7; i++)
            if (pipe_vp9->picture_parameter.pic_fields.segmentation_temporal_update)
      for (uint32_t i = 0; i < 3; i++)
      else
            for (uint32_t i = 0; i < 8; i++) {
      dxvaStructure.stVP9Segments.feature_mask[i] = (pipe_vp9->slice_parameter.seg_param[i].alt_quant_enabled              << 0) |
                        dxvaStructure.stVP9Segments.feature_data[i][0] = pipe_vp9->slice_parameter.seg_param[i].alt_quant;
   dxvaStructure.stVP9Segments.feature_data[i][1] = pipe_vp9->slice_parameter.seg_param[i].alt_lf;
   dxvaStructure.stVP9Segments.feature_data[i][2] = pipe_vp9->slice_parameter.seg_param[i].segment_flags.segment_reference;
               dxvaStructure.log2_tile_cols = pipe_vp9->picture_parameter.log2_tile_columns;
   dxvaStructure.log2_tile_rows = pipe_vp9->picture_parameter.log2_tile_rows;
   dxvaStructure.uncompressed_header_size_byte_aligned = pipe_vp9->picture_parameter.frame_header_length_in_bytes;
   dxvaStructure.first_partition_size = pipe_vp9->picture_parameter.first_partition_size;
   dxvaStructure.StatusReportFeedbackNumber = frameNum;
   assert(dxvaStructure.StatusReportFeedbackNumber > 0);
      }
