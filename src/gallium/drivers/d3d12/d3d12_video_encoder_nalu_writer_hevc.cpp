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
      #include "d3d12_video_encoder_nalu_writer_hevc.h"
   #include <algorithm>
      // Writes the HEVC VPS structure into a bitstream passed in headerBitstream
   // Function resizes bitstream accordingly and puts result in byte vector
   void
   d3d12_video_nalu_writer_hevc::vps_to_nalu_bytes(HevcVideoParameterSet *pVPS,
                           }
      // Writes the HEVC pSPS structure into a bitstream passed in headerBitstream
   // Function resizes bitstream accordingly and puts result in byte vector
   void
   d3d12_video_nalu_writer_hevc::sps_to_nalu_bytes(HevcSeqParameterSet *pSPS,
                           }
      // Writes the HEVC PPS structure into a bitstream passed in headerBitstream
   // Function resizes bitstream accordingly and puts result in byte vector
   void
   d3d12_video_nalu_writer_hevc::pps_to_nalu_bytes(HevcPicParameterSet *pPPS,
                           }
      void
   d3d12_video_nalu_writer_hevc::write_end_of_stream_nalu(std::vector<BYTE> &headerBitstream,
                  HEVCNaluHeader endOfStreamNALU =
   {
      // uint8_t forbidden_zero_bit;
   static_cast<uint8_t>(0u),
   // uint8_t nal_unit_type
   static_cast<uint8_t>(HEVC_NALU_EOB_NUT),
   // uint8_t nuh_layer_id
   static_cast<uint8_t>(0u),
   // uint8_t nuh_temporal_id_plus1
      };
      }
      void
   d3d12_video_nalu_writer_hevc::write_end_of_sequence_nalu(std::vector<BYTE> &headerBitstream,
                  HEVCNaluHeader endOfSeqNALU =
   {
      // uint8_t forbidden_zero_bit;
   static_cast<uint8_t>(0u),
   // uint8_t nal_unit_type
   static_cast<uint8_t>(HEVC_NALU_EOS_NUT),        
   // uint8_t nuh_layer_id
   static_cast<uint8_t>(0u),
   // uint8_t nuh_temporal_id_plus1
      };
      }
      void 
   d3d12_video_nalu_writer_hevc::generic_write_bytes( std::vector<BYTE> &headerBitstream,
                     {
      // Wrap pSPS into NALU and copy full NALU into output byte array
            /*HEVCNaluHeader nalu is in all Hevc*ParameterSet structures at the beggining*/
            if (!rbsp.create_bitstream(MAX_COMPRESSED_NALU)) {
      debug_printf("rbsp.create_bitstream(MAX_COMPRESSED_NALU) failed\n");
               if (!nalu.create_bitstream(2 * MAX_COMPRESSED_NALU)) {
      debug_printf("nalu.create_bitstream(2 * MAX_COMPRESSED_NALU) failed\n");
               rbsp.set_start_code_prevention(true);
   if (write_bytes_from_struct(&rbsp, pStructure, nal_header->nal_unit_type) <= 0u) {
      debug_printf("write_bytes_from_struct(&rbsp, pStructure, nal_header->nal_unit_type) didn't write any bytes.\n");
               if (wrap_rbsp_into_nalu(&nalu, &rbsp, nal_header) <= 0u) {
      debug_printf("wrap_rbsp_into_nalu(&nalu, &rbsp, nal_header) didn't write any bytes.\n");
               // Deep copy nalu into headerBitstream, nalu gets out of scope here and its destructor frees the nalu object buffer
   // memory.
   uint8_t *naluBytes    = nalu.get_bitstream_buffer();
            auto startDstIndex = std::distance(headerBitstream.begin(), placingPositionStart);
   if (headerBitstream.size() < (startDstIndex + naluByteSize)) {
                              }
      uint32_t
   d3d12_video_nalu_writer_hevc::write_bytes_from_struct(d3d12_video_encoder_bitstream *pBitstream, void *pData, uint8_t nal_unit_type)
   {
      switch(nal_unit_type)
   {
      case HEVC_NALU_VPS_NUT:
   {
         } break;
   case HEVC_NALU_SPS_NUT:
   {
         } break;
   case HEVC_NALU_PPS_NUT:
   {
         } break;
   case HEVC_NALU_EOS_NUT:
   case HEVC_NALU_EOB_NUT:
   {
         // Do nothing for these two, just the header suffices
   } break;
   default:
   {
               }
      uint32_t
   d3d12_video_nalu_writer_hevc::write_vps_bytes(d3d12_video_encoder_bitstream *pBitstream, HevcVideoParameterSet *vps)
   {
               pBitstream->put_bits(4, vps->vps_video_parameter_set_id);
   pBitstream->put_bits(2, 3); //vps_reserved_three_2bits
   pBitstream->put_bits(6, vps->vps_max_layers_minus1);
   pBitstream->put_bits(3, vps->vps_max_sub_layers_minus1);
   pBitstream->put_bits(1, vps->vps_temporal_id_nesting_flag);
                              for (int i = 0; i <= vps->vps_max_sub_layers_minus1; i++) {
      pBitstream->exp_Golomb_ue(vps->vps_max_dec_pic_buffering_minus1[i]);
   pBitstream->exp_Golomb_ue(vps->vps_max_num_reorder_pics[i]);
               pBitstream->put_bits(6, vps->vps_max_layer_id);
   pBitstream->exp_Golomb_ue(vps->vps_num_layer_sets_minus1);
                     rbsp_trailing(pBitstream);
            iBytesWritten = pBitstream->get_byte_count() - iBytesWritten;
      }
      uint32_t
   d3d12_video_nalu_writer_hevc::write_sps_bytes(d3d12_video_encoder_bitstream *pBitstream,
         {
               pBitstream->put_bits(4, pSPS->sps_video_parameter_set_id);
   pBitstream->put_bits(3, pSPS->sps_max_sub_layers_minus1);
                                       pBitstream->exp_Golomb_ue(pSPS->pic_width_in_luma_samples);
            pBitstream->put_bits(1, pSPS->conformance_window_flag);
   if (pSPS->conformance_window_flag) {
      pBitstream->exp_Golomb_ue(pSPS->conf_win_left_offset);
   pBitstream->exp_Golomb_ue(pSPS->conf_win_right_offset);
   pBitstream->exp_Golomb_ue(pSPS->conf_win_top_offset);
               pBitstream->exp_Golomb_ue(pSPS->bit_depth_luma_minus8);
                              for (int i = 0; i <= pSPS->sps_max_sub_layers_minus1; i++) {
      pBitstream->exp_Golomb_ue(pSPS->sps_max_dec_pic_buffering_minus1[i]);
   pBitstream->exp_Golomb_ue(pSPS->sps_max_num_reorder_pics[i]);
               pBitstream->exp_Golomb_ue(pSPS->log2_min_luma_coding_block_size_minus3);
   pBitstream->exp_Golomb_ue(pSPS->log2_diff_max_min_luma_coding_block_size);
   pBitstream->exp_Golomb_ue(pSPS->log2_min_transform_block_size_minus2);
            pBitstream->exp_Golomb_ue(pSPS->max_transform_hierarchy_depth_inter);
                     pBitstream->put_bits(1, pSPS->amp_enabled_flag);
            pBitstream->put_bits(1, pSPS->pcm_enabled_flag);
   if (pSPS->pcm_enabled_flag) {
   pBitstream->put_bits(4, pSPS->bit_depth_luma_minus8 + 7);
   pBitstream->put_bits(4, pSPS->bit_depth_chroma_minus8 + 7);
      pBitstream->exp_Golomb_ue(pSPS->log2_min_luma_coding_block_size_minus3);
      pBitstream->put_bits(1, pSPS->pcm_loop_filter_disabled_flag);
            pBitstream->exp_Golomb_ue(pSPS->num_short_term_ref_pic_sets);
   for (int i = 0; i < pSPS->num_short_term_ref_pic_sets; i++) {
                  pBitstream->put_bits(1, pSPS->long_term_ref_pics_present_flag);
   if (pSPS->long_term_ref_pics_present_flag) {
      pBitstream->exp_Golomb_ue(pSPS->num_long_term_ref_pics_sps);
   for (int i = 0; i < pSPS->num_long_term_ref_pics_sps; i++) {
         pBitstream->put_bits(pSPS->log2_max_pic_order_cnt_lsb_minus4 + 4, pSPS->lt_ref_pic_poc_lsb_sps[i]);
               pBitstream->put_bits(1, pSPS->sps_temporal_mvp_enabled_flag);
   pBitstream->put_bits(1, pSPS->strong_intra_smoothing_enabled_flag);
   pBitstream->put_bits(1, pSPS->vui_parameters_present_flag);
            //  pSps_extension_flag
            rbsp_trailing(pBitstream);
            iBytesWritten = pBitstream->get_byte_count() - iBytesWritten;
      }
      uint32_t
   d3d12_video_nalu_writer_hevc::write_pps_bytes(d3d12_video_encoder_bitstream *pBitstream,
         {
               pBitstream->exp_Golomb_ue(pPPS->pps_pic_parameter_set_id);
                     pBitstream->put_bits(1, pPPS->output_flag_present_flag);
            pBitstream->put_bits(1, pPPS->sign_data_hiding_enabled_flag);
            pBitstream->exp_Golomb_ue(pPPS->num_ref_idx_lx_default_active_minus1[0]);
                     pBitstream->put_bits(1, pPPS->constrained_intra_pred_flag);
   pBitstream->put_bits(1, pPPS->transform_skip_enabled_flag);
            if (pPPS->cu_qp_delta_enabled_flag) {
                  pBitstream->exp_Golomb_se(pPPS->pps_cb_qp_offset);
                     pBitstream->put_bits(1, pPPS->weighted_pred_flag);
   pBitstream->put_bits(1, pPPS->weighted_bipred_flag);
            pBitstream->put_bits(1, pPPS->tiles_enabled_flag);
            if (pPPS->tiles_enabled_flag) {
      pBitstream->exp_Golomb_ue(pPPS->num_tile_columns_minus1);
   pBitstream->exp_Golomb_ue(pPPS->num_tile_rows_minus1);
   pBitstream->put_bits(1, pPPS->uniform_spacing_flag);
   if (!pPPS->uniform_spacing_flag) {
         for (int i = 0; i < pPPS->num_tile_columns_minus1; i++) {
         }
   for (int i = 0; i < pPPS->num_tile_rows_minus1; i++) {
         }
               pBitstream->put_bits(1, pPPS->pps_loop_filter_across_slices_enabled_flag);
   pBitstream->put_bits(1, pPPS->deblocking_filter_control_present_flag);
   if (pPPS->deblocking_filter_control_present_flag) {
      pBitstream->put_bits(1, pPPS->deblocking_filter_override_enabled_flag);
   pBitstream->put_bits(1, pPPS->pps_deblocking_filter_disabled_flag);
   if (!pPPS->pps_deblocking_filter_disabled_flag) {
         pBitstream->exp_Golomb_se(pPPS->pps_beta_offset_div2);
               pBitstream->put_bits(1, pPPS->pps_scaling_list_data_present_flag);
   if (pPPS->pps_scaling_list_data_present_flag) {
                  pBitstream->put_bits(1, pPPS->lists_modification_present_flag);
   pBitstream->exp_Golomb_ue(pPPS->log2_parallel_merge_level_minus2);
            //pps_extension_flag
            rbsp_trailing(pBitstream);
            iBytesWritten = pBitstream->get_byte_count() - iBytesWritten;
      }
      uint32_t
   d3d12_video_nalu_writer_hevc::wrap_rbsp_into_nalu(d3d12_video_encoder_bitstream *pNALU,
               {
      ASSERTED bool isAligned = pRBSP->is_byte_aligned();   // causes side-effects in object state, don't put inside assert()
                              // NAL start code
   pNALU->put_bits(24, 0);
            // NAL header
   pNALU->put_bits(1, pHeader->forbidden_zero_bit);
   pNALU->put_bits(6, pHeader->nal_unit_type);
   pNALU->put_bits(6, pHeader->nuh_layer_id);
   pNALU->put_bits(3, pHeader->nuh_temporal_id_plus1);
            // NAL body
            if (pRBSP->get_start_code_prevention_status()) {
      // Direct copying.
      } else {
      // Copy with start code prevention.
   pNALU->set_start_code_prevention(true);
   int32_t  iLength = pRBSP->get_byte_count();
            for (int32_t i = 0; i < iLength; i++) {
                     isAligned = pNALU->is_byte_aligned();   // causes side-effects in object state, don't put inside assert()
   assert(isAligned);
                     iBytesWritten = pNALU->get_byte_count() - iBytesWritten;
      }
      void
   d3d12_video_nalu_writer_hevc::write_nalu_end(d3d12_video_encoder_bitstream *pNALU)
   {
      pNALU->flush();
   pNALU->set_start_code_prevention(false);
            if (false == pNALU->m_bBufferOverflow && 0x00 == pNALU->get_bitstream_buffer()[iNALUnitLen - 1]) {
      pNALU->put_bits(8, 0x03);
         }
      void
   d3d12_video_nalu_writer_hevc::rbsp_trailing(d3d12_video_encoder_bitstream *pBitstream)
   {
      pBitstream->put_bits(1, 1);
            if (iLeft) {
                  ASSERTED bool isAligned = pBitstream->is_byte_aligned();   // causes side-effects in object state, don't put inside assert()
      }
      void
   d3d12_video_nalu_writer_hevc::write_profile_tier_level(d3d12_video_encoder_bitstream* rbsp, HEVCProfileTierLevel* ptl)
   {
      rbsp->put_bits(2, ptl->general_profile_space);
   rbsp->put_bits(1, ptl->general_tier_flag);
            for (int j = 0; j < 32; j++) {
                  rbsp->put_bits(1, ptl->general_progressive_source_flag);
   rbsp->put_bits(1, ptl->general_interlaced_source_flag);
   rbsp->put_bits(1, ptl->general_non_packed_constraint_flag);
   rbsp->put_bits(1, ptl->general_frame_only_constraint_flag);
   rbsp->put_bits(31, 0); //first 31 bits of general_reserved_zero_44bits
   rbsp->put_bits(13, 0); //last 13 bits of general_reserved_zero_44bits
      }
      void
   d3d12_video_nalu_writer_hevc::write_rps(d3d12_video_encoder_bitstream* rbsp, HevcSeqParameterSet* pSPS, int stRpsIdx, bool sliceRPS)
   {    
               if (stRpsIdx != 0) {
                  if (rps->inter_ref_pic_set_prediction_flag) {
      if (sliceRPS) {
         }
   int RefRpsIdx = stRpsIdx - (rps->delta_idx_minus1 + 1);        
   rbsp->put_bits(1, rps->delta_rps_sign);
   rbsp->exp_Golomb_ue(rps->abs_delta_rps_minus1);
         HEVCReferencePictureSet* rpsRef = &(pSPS->rpsShortTerm[RefRpsIdx]);
   auto numDeltaPocs = rpsRef->num_negative_pics + rpsRef->num_positive_pics;
   for (int j = 0; j <= numDeltaPocs; j++) {
         rbsp->put_bits(1, rps->used_by_curr_pic_flag[j]);
   if (!rps->used_by_curr_pic_flag[j]) {
            } else {
      rbsp->exp_Golomb_ue(rps->num_negative_pics);
            for (int i = 0; i < rps->num_negative_pics; i++) {
         rbsp->exp_Golomb_ue(rps->delta_poc_s0_minus1[i]);
            for (int i = 0; i < rps->num_positive_pics; i++) {
         rbsp->exp_Golomb_ue(rps->delta_poc_s1_minus1[i]);
         }
