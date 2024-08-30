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
      #include "d3d12_video_encoder_nalu_writer_h264.h"
   #include <algorithm>
      void
   d3d12_video_nalu_writer_h264::rbsp_trailing(d3d12_video_encoder_bitstream *pBitstream)
   {
      pBitstream->put_bits(1, 1);
            if (iLeft) {
                  ASSERTED bool isAligned = pBitstream->is_byte_aligned();   // causes side-effects in object state, don't put inside assert()
      }
      uint32_t
   d3d12_video_nalu_writer_h264::write_sps_bytes(d3d12_video_encoder_bitstream *pBitstream, H264_SPS *pSPS)
   {
               // Standard constraint to be between 0 and 31 inclusive
   assert(pSPS->seq_parameter_set_id >= 0);
            pBitstream->put_bits(8, pSPS->profile_idc);
   pBitstream->put_bits(1, 0);   // constraint_set0_flag
   pBitstream->put_bits(1, 0);   // constraint_set1_flag
   pBitstream->put_bits(1, 0);   // constraint_set2_flag
   pBitstream->put_bits(1, pSPS->constraint_set3_flag);
   pBitstream->put_bits(1, 0);   // constraint_set4_flag
   pBitstream->put_bits(1, 0);   // constraint_set5_flag
   pBitstream->put_bits(2, 0);
   pBitstream->put_bits(8, pSPS->level_idc);
            // Only support profiles defined in D3D12 Video Encode
   // If adding new profile support, check that the chroma_format_idc and bit depth are set correctly below
   // for the new additions
   assert((pSPS->profile_idc == H264_PROFILE_MAIN) || (pSPS->profile_idc == H264_PROFILE_HIGH) ||
            if ((pSPS->profile_idc == H264_PROFILE_HIGH) || (pSPS->profile_idc == H264_PROFILE_HIGH10)) {
      // chroma_format_idc always 4.2.0
   pBitstream->exp_Golomb_ue(1);
   // Assume no separate_colour_plane_flag given chroma_format_idc = 1
   pBitstream->exp_Golomb_ue(pSPS->bit_depth_luma_minus8);
   pBitstream->exp_Golomb_ue(pSPS->bit_depth_chroma_minus8);
   // qpprime_y_zero_transform_bypass_flag
   pBitstream->put_bits(1, 0);
   // seq_scaling_matrix_present_flag)
                        pBitstream->exp_Golomb_ue(pSPS->pic_order_cnt_type);
   if (pSPS->pic_order_cnt_type == 0) {
         }
   pBitstream->exp_Golomb_ue(pSPS->max_num_ref_frames);
   pBitstream->put_bits(1, pSPS->gaps_in_frame_num_value_allowed_flag);
   pBitstream->exp_Golomb_ue(pSPS->pic_width_in_mbs_minus1);
            // No support for interlace in D3D12 Video Encode
   // frame_mbs_only_flag coded as 1
   pBitstream->put_bits(1, 1);   // frame_mbs_only_flag
            // no cropping
   pBitstream->put_bits(1, pSPS->frame_cropping_flag);   // frame_cropping_flag
   if (pSPS->frame_cropping_flag) {
      pBitstream->exp_Golomb_ue(pSPS->frame_cropping_rect_left_offset);
   pBitstream->exp_Golomb_ue(pSPS->frame_cropping_rect_right_offset);
   pBitstream->exp_Golomb_ue(pSPS->frame_cropping_rect_top_offset);
               // We're not including the VUI so this better be zero.
            rbsp_trailing(pBitstream);
            iBytesWritten = pBitstream->get_byte_count() - iBytesWritten;
      }
      uint32_t
   d3d12_video_nalu_writer_h264::write_pps_bytes(d3d12_video_encoder_bitstream *pBitstream,
               {
               // Standard constraint to be between 0 and 31 inclusive
   assert(pPPS->seq_parameter_set_id >= 0);
            // Standard constraint to be between 0 and 255 inclusive
   assert(pPPS->pic_parameter_set_id >= 0);
            pBitstream->exp_Golomb_ue(pPPS->pic_parameter_set_id);
   pBitstream->exp_Golomb_ue(pPPS->seq_parameter_set_id);
   pBitstream->put_bits(1, pPPS->entropy_coding_mode_flag);
   pBitstream->put_bits(1, pPPS->pic_order_present_flag);   // bottom_field_pic_order_in_frame_present_flag
               pBitstream->exp_Golomb_ue(pPPS->num_ref_idx_l0_active_minus1);
   pBitstream->exp_Golomb_ue(pPPS->num_ref_idx_l1_active_minus1);
   pBitstream->put_bits(1, 0);     // weighted_pred_flag
   pBitstream->put_bits(2, 0);     // weighted_bipred_idc
   pBitstream->exp_Golomb_se(0);   // pic_init_qp_minus26
   pBitstream->exp_Golomb_se(0);   // pic_init_qs_minus26
   pBitstream->exp_Golomb_se(0);   // chroma_qp_index_offset
   pBitstream->put_bits(1, 1);     // deblocking_filter_control_present_flag
   pBitstream->put_bits(1, pPPS->constrained_intra_pred_flag);
            if (bIsHighProfile) {
      pBitstream->put_bits(1, pPPS->transform_8x8_mode_flag);
   pBitstream->put_bits(1, 0);     // pic_scaling_matrix_present_flag
               rbsp_trailing(pBitstream);
            iBytesWritten = pBitstream->get_byte_count() - iBytesWritten;
      }
      uint32_t
   d3d12_video_nalu_writer_h264::wrap_sps_nalu(d3d12_video_encoder_bitstream *pNALU, d3d12_video_encoder_bitstream *pRBSP)
   {
         }
      uint32_t
   d3d12_video_nalu_writer_h264::wrap_pps_nalu(d3d12_video_encoder_bitstream *pNALU, d3d12_video_encoder_bitstream *pRBSP)
   {
         }
      void
   d3d12_video_nalu_writer_h264::write_nalu_end(d3d12_video_encoder_bitstream *pNALU)
   {
      pNALU->flush();
   pNALU->set_start_code_prevention(false);
            if (false == pNALU->m_bBufferOverflow && 0x00 == pNALU->get_bitstream_buffer()[iNALUnitLen - 1]) {
      pNALU->put_bits(8, 0x03);
         }
      uint32_t
   d3d12_video_nalu_writer_h264::wrap_rbsp_into_nalu(d3d12_video_encoder_bitstream *pNALU,
                     {
      bool isAligned = pRBSP->is_byte_aligned();   // causes side-effects in object state, don't put inside assert()
                              // NAL start code
   pNALU->put_bits(24, 0);
            // NAL header
   pNALU->put_bits(1, 0);
   pNALU->put_bits(2, iNaluIdc);
   pNALU->put_bits(5, iNaluType);
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
   d3d12_video_nalu_writer_h264::sps_to_nalu_bytes(H264_SPS *                     pSPS,
                     {
      // Wrap SPS into NALU and copy full NALU into output byte array
            if (!rbsp.create_bitstream(MAX_COMPRESSED_SPS)) {
      debug_printf("rbsp.create_bitstream(MAX_COMPRESSED_SPS) failed\n");
               if (!nalu.create_bitstream(2 * MAX_COMPRESSED_SPS)) {
      debug_printf("nalu.create_bitstream(2 * MAX_COMPRESSED_SPS) failed\n");
               rbsp.set_start_code_prevention(true);
   if (write_sps_bytes(&rbsp, pSPS) <= 0u) {
      debug_printf("write_sps_bytes(&rbsp, pSPS) didn't write any bytes.\n");
               if (wrap_sps_nalu(&nalu, &rbsp) <= 0u) {
      debug_printf("wrap_sps_nalu(&nalu, &rbsp) didn't write any bytes.\n");
               // Deep copy nalu into headerBitstream, nalu gets out of scope here and its destructor frees the nalu object buffer
   // memory.
   uint8_t *naluBytes    = nalu.get_bitstream_buffer();
            auto startDstIndex = std::distance(headerBitstream.begin(), placingPositionStart);
   if (headerBitstream.size() < (startDstIndex + naluByteSize)) {
                              }
      void
   d3d12_video_nalu_writer_h264::pps_to_nalu_bytes(H264_PPS *                     pPPS,
                           {
      // Wrap PPS into NALU and copy full NALU into output byte array
   d3d12_video_encoder_bitstream rbsp, nalu;
   if (!rbsp.create_bitstream(MAX_COMPRESSED_PPS)) {
      debug_printf("rbsp.create_bitstream(MAX_COMPRESSED_PPS) failed\n");
               if (!nalu.create_bitstream(2 * MAX_COMPRESSED_PPS)) {
      debug_printf("nalu.create_bitstream(2 * MAX_COMPRESSED_PPS) failed\n");
                        if (write_pps_bytes(&rbsp, pPPS, bIsHighProfile) <= 0u) {
      debug_printf("write_pps_bytes(&rbsp, pPPS, bIsHighProfile) didn't write any bytes.\n");
               if (wrap_pps_nalu(&nalu, &rbsp) <= 0u) {
      debug_printf("wrap_pps_nalu(&nalu, &rbsp) didn't write any bytes.\n");
               // Deep copy nalu into headerBitstream, nalu gets out of scope here and its destructor frees the nalu object buffer
   // memory.
   uint8_t *naluBytes    = nalu.get_bitstream_buffer();
            auto startDstIndex = std::distance(headerBitstream.begin(), placingPositionStart);
   if (headerBitstream.size() < (startDstIndex + naluByteSize)) {
                              }
      void
   d3d12_video_nalu_writer_h264::write_end_of_stream_nalu(std::vector<uint8_t> &         headerBitstream,
               {
      d3d12_video_encoder_bitstream rbsp, nalu;
   if (!rbsp.create_bitstream(8)) {
      debug_printf("rbsp.create_bitstream(8) failed\n");
      }
   if (!nalu.create_bitstream(2 * MAX_COMPRESSED_PPS)) {
      debug_printf("nalu.create_bitstream(2 * MAX_COMPRESSED_PPS) failed\n");
               rbsp.set_start_code_prevention(true);
   if (wrap_rbsp_into_nalu(&nalu, &rbsp, NAL_REFIDC_REF, NAL_TYPE_END_OF_STREAM) <= 0u) {
      debug_printf(
                     // Deep copy nalu into headerBitstream, nalu gets out of scope here and its destructor frees the nalu object buffer
   // memory.
   uint8_t *naluBytes    = nalu.get_bitstream_buffer();
            auto startDstIndex = std::distance(headerBitstream.begin(), placingPositionStart);
   if (headerBitstream.size() < (startDstIndex + naluByteSize)) {
                              }
      void
   d3d12_video_nalu_writer_h264::write_end_of_sequence_nalu(std::vector<uint8_t> &         headerBitstream,
               {
      d3d12_video_encoder_bitstream rbsp, nalu;
   if (!rbsp.create_bitstream(8)) {
      debug_printf("rbsp.create_bitstream(8) failed.\n");
               if (!nalu.create_bitstream(2 * MAX_COMPRESSED_PPS)) {
      debug_printf("nalu.create_bitstream(2 * MAX_COMPRESSED_PPS) failed.\n");
               rbsp.set_start_code_prevention(true);
               debug_printf(
                     // Deep copy nalu into headerBitstream, nalu gets out of scope here and its destructor frees the nalu object buffer
   // memory.
   uint8_t *naluBytes    = nalu.get_bitstream_buffer();
            auto startDstIndex = std::distance(headerBitstream.begin(), placingPositionStart);
   if (headerBitstream.size() < (startDstIndex + naluByteSize)) {
                              }
