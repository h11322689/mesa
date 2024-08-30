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
      #include "anv_private.h"
      #include "genxml/gen_macros.h"
   #include "genxml/genX_pack.h"
      #include "util/vl_zscan_data.h"
      void
   genX(CmdBeginVideoCodingKHR)(VkCommandBuffer commandBuffer,
         {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   ANV_FROM_HANDLE(anv_video_session, vid, pBeginInfo->videoSession);
            cmd_buffer->video.vid = vid;
      }
      void
   genX(CmdControlVideoCodingKHR)(VkCommandBuffer commandBuffer,
         {
               if (pCodingControlInfo->flags & VK_VIDEO_CODING_CONTROL_RESET_BIT_KHR) {
      anv_batch_emit(&cmd_buffer->batch, GENX(MI_FLUSH_DW), flush) {
               }
      void
   genX(CmdEndVideoCodingKHR)(VkCommandBuffer commandBuffer,
         {
               cmd_buffer->video.vid = NULL;
      }
      static void
   scaling_list(struct anv_cmd_buffer *cmd_buffer,
         {
      /* 4x4, 8x8, 16x16, 32x32 */
   for (uint8_t size = 0; size < 4; size++) {
      /* Intra, Inter */
   for (uint8_t pred = 0; pred < 2; pred++) {
      /* Y, Cb, Cr */
   for (uint8_t color = 0; color < 3; color++) {
                     anv_batch_emit(&cmd_buffer->batch, GENX(HCP_QM_STATE), qm) {
                                                if (size == 0) {
      for (uint8_t i = 0; i < 4; i++)
      for (uint8_t j = 0; j < 4; j++)
         } else if (size == 1) {
      for (uint8_t i = 0; i < 8; i++)
      for (uint8_t j = 0; j < 8; j++)
         } else if (size == 2) {
      for (uint8_t i = 0; i < 8; i++)
      for (uint8_t j = 0; j < 8; j++)
         } else if (size == 3) {
      for (uint8_t i = 0; i < 8; i++)
      for (uint8_t j = 0; j < 8; j++)
                        }
      static void
   anv_h265_decode_video(struct anv_cmd_buffer *cmd_buffer,
         {
      ANV_FROM_HANDLE(anv_buffer, src_buffer, frame_info->srcBuffer);
   struct anv_video_session *vid = cmd_buffer->video.vid;
            const struct VkVideoDecodeH265PictureInfoKHR *h265_pic_info =
            const StdVideoH265SequenceParameterSet *sps =
         const StdVideoH265PictureParameterSet *pps =
            struct vk_video_h265_reference ref_slots[2][8] = { 0 };
   uint8_t dpb_idx[ANV_VIDEO_H265_MAX_NUM_REF_FRAME] = { 0,};
            anv_batch_emit(&cmd_buffer->batch, GENX(MI_FLUSH_DW), flush) {
               #if GFX_VER >= 12
      anv_batch_emit(&cmd_buffer->batch, GENX(MI_FORCE_WAKEUP), wake) {
      wake.HEVCPowerWellControl = 1;
               anv_batch_emit(&cmd_buffer->batch, GENX(VD_CONTROL_STATE), cs) {
                  anv_batch_emit(&cmd_buffer->batch, GENX(MFX_WAIT), mfx) {
            #endif
         anv_batch_emit(&cmd_buffer->batch, GENX(HCP_PIPE_MODE_SELECT), sel) {
      sel.CodecSelect = Decode;
            #if GFX_VER >= 12
      anv_batch_emit(&cmd_buffer->batch, GENX(MFX_WAIT), mfx) {
            #endif
         const struct anv_image_view *iv =
                  anv_batch_emit(&cmd_buffer->batch, GENX(HCP_SURFACE_STATE), ss) {
      ss.SurfacePitch = img->planes[0].primary_surface.isl.row_pitch_B - 1;
   ss.SurfaceID = HCP_CurrentDecodedPicture;
            ss.YOffsetforUCb = img->planes[1].primary_surface.memory_range.offset /
      #if GFX_VER >= 11
         #endif
            #if GFX_VER >= 12
      /* Seems to need to set same states to ref as decode on gen12 */
   anv_batch_emit(&cmd_buffer->batch, GENX(HCP_SURFACE_STATE), ss) {
      ss.SurfacePitch = img->planes[0].primary_surface.isl.row_pitch_B - 1;
   ss.SurfaceID = HCP_ReferencePicture;
            ss.YOffsetforUCb = img->planes[1].primary_surface.memory_range.offset /
                  #endif
         anv_batch_emit(&cmd_buffer->batch, GENX(HCP_PIPE_BUF_ADDR_STATE), buf) {
      buf.DecodedPictureAddress =
            buf.DecodedPictureMemoryAddressAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
                  buf.DeblockingFilterLineBufferAddress = (struct anv_address) {
      vid->vid_mem[ANV_VID_MEM_H265_DEBLOCK_FILTER_ROW_STORE_LINE].mem->bo,
               buf.DeblockingFilterLineBufferMemoryAddressAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
                  buf.DeblockingFilterTileLineBufferAddress = (struct anv_address) {
      vid->vid_mem[ANV_VID_MEM_H265_DEBLOCK_FILTER_ROW_STORE_TILE_LINE].mem->bo,
               buf.DeblockingFilterTileLineBufferMemoryAddressAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
                  buf.DeblockingFilterTileColumnBufferAddress = (struct anv_address) {
      vid->vid_mem[ANV_VID_MEM_H265_DEBLOCK_FILTER_ROW_STORE_TILE_COLUMN].mem->bo,
               buf.DeblockingFilterTileColumnBufferMemoryAddressAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
                  buf.MetadataLineBufferAddress = (struct anv_address) {
      vid->vid_mem[ANV_VID_MEM_H265_METADATA_LINE].mem->bo,
               buf.MetadataLineBufferMemoryAddressAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
                  buf.MetadataTileLineBufferAddress = (struct anv_address) {
      vid->vid_mem[ANV_VID_MEM_H265_METADATA_TILE_LINE].mem->bo,
               buf.MetadataTileLineBufferMemoryAddressAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
                  buf.MetadataTileColumnBufferAddress = (struct anv_address) {
      vid->vid_mem[ANV_VID_MEM_H265_METADATA_TILE_COLUMN].mem->bo,
               buf.MetadataTileColumnBufferMemoryAddressAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
                  buf.SAOLineBufferAddress = (struct anv_address) {
      vid->vid_mem[ANV_VID_MEM_H265_SAO_LINE].mem->bo,
               buf.SAOLineBufferMemoryAddressAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
                  buf.SAOTileLineBufferAddress = (struct anv_address) {
      vid->vid_mem[ANV_VID_MEM_H265_SAO_TILE_LINE].mem->bo,
               buf.SAOTileLineBufferMemoryAddressAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
                  buf.SAOTileColumnBufferAddress = (struct anv_address) {
      vid->vid_mem[ANV_VID_MEM_H265_SAO_TILE_COLUMN].mem->bo,
               buf.SAOTileColumnBufferMemoryAddressAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
                           buf.CurrentMVTemporalBufferMemoryAddressAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
                  for (unsigned i = 0; i < frame_info->referenceSlotCount; i++) {
      const struct anv_image_view *ref_iv =
                                 buf.ReferencePictureAddress[i] =
               buf.ReferencePictureMemoryAddressAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
                  buf.OriginalUncompressedPictureSourceMemoryAddressAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
                  buf.StreamOutDataDestinationMemoryAddressAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
                  buf.DecodedPictureStatusBufferMemoryAddressAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
                  buf.LCUILDBStreamOutBufferMemoryAddressAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
                  for (unsigned i = 0; i < frame_info->referenceSlotCount; i++) {
                     buf.CollocatedMVTemporalBufferAddress[i] =
               buf.CollocatedMVTemporalBufferMemoryAddressAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
                  buf.VP9ProbabilityBufferMemoryAddressAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
                  buf.VP9SegmentIDBufferMemoryAddressAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
                  buf.VP9HVDLineRowStoreBufferMemoryAddressAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
                  buf.VP9HVDTileRowStoreBufferMemoryAddressAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
         #if GFX_VER >= 11
         buf.SAOStreamOutDataDestinationBufferMemoryAddressAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
         };
   buf.FrameStatisticsStreamOutDataDestinationBufferMemoryAddressAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
         };
   buf.SSESourcePixelRowStoreBufferMemoryAddressAttributesReadWrite = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
         };
   buf.HCPScalabilitySliceStateBufferMemoryAddressAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
         };
   buf.HCPScalabilityCABACDecodedSyntaxElementsBufferMemoryAddressAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
         };
   buf.MVUpperRightColumnStoreBufferMemoryAddressAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
         };
   buf.IntraPredictionUpperRightColumnStoreBufferMemoryAddressAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
         };
   buf.IntraPredictionLeftReconColumnStoreBufferMemoryAddressAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
         #endif
               anv_batch_emit(&cmd_buffer->batch, GENX(HCP_IND_OBJ_BASE_ADDR_STATE), indirect) {
      indirect.HCPIndirectBitstreamObjectBaseAddress =
            indirect.HCPIndirectBitstreamObjectMemoryAddressAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
                  indirect.HCPIndirectBitstreamObjectAccessUpperBound =
            indirect.HCPIndirectCUObjectMemoryAddressAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
                  indirect.HCPPAKBSEObjectMemoryAddressAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
            #if GFX_VER >= 11
         indirect.HCPVP9PAKCompressedHeaderSyntaxStreamInMemoryAddressAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
         };
   indirect.HCPVP9PAKProbabilityCounterStreamOutMemoryAddressAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
         };
   indirect.HCPVP9PAKProbabilityDeltasStreamInMemoryAddressAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
         };
   indirect.HCPVP9PAKTileRecordStreamOutMemoryAddressAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
         };
   indirect.HCPVP9PAKCULevelStatisticStreamOutMemoryAddressAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
         #endif
               if (sps->flags.scaling_list_enabled_flag) {
      if (pps->flags.pps_scaling_list_data_present_flag) {
         } else if (sps->flags.sps_scaling_list_data_present_flag) {
            } else {
      for (uint8_t size = 0; size < 4; size++) {
                                          anv_batch_emit(&cmd_buffer->batch, GENX(HCP_QM_STATE), qm) {
      qm.SizeID = size;
   qm.PredictionType = pred;
                        for (uint8_t q = 0; q < len; q++)
                           anv_batch_emit(&cmd_buffer->batch, GENX(HCP_PIC_STATE), pic) {
      pic.FrameWidthInMinimumCodingBlockSize =
         pic.FrameHeightInMinimumCodingBlockSize =
            pic.MinCUSize = sps->log2_min_luma_coding_block_size_minus3 & 0x3;
   pic.LCUSize = (sps->log2_diff_max_min_luma_coding_block_size +
            pic.MinTUSize = sps->log2_min_luma_transform_block_size_minus2 & 0x3;
   pic.MaxTUSize = (sps->log2_diff_max_min_luma_transform_block_size + sps->log2_min_luma_transform_block_size_minus2) & 0x3;
   pic.MinPCMSize = sps->log2_min_pcm_luma_coding_block_size_minus3 & 0x3;
      #if GFX_VER >= 11
         pic.Log2SAOOffsetScaleLuma = pps->log2_sao_offset_scale_luma;
   pic.Log2SAOOffsetScaleChroma = pps->log2_sao_offset_scale_chroma;
   pic.ChromaQPOffsetListLength = pps->chroma_qp_offset_list_len_minus1;
   pic.DiffCUChromaQPOffsetDepth = pps->diff_cu_chroma_qp_offset_depth;
   pic.ChromaQPOffsetListEnable = pps->flags.chroma_qp_offset_list_enabled_flag;
            pic.HighPrecisionOffsetsEnable = sps->flags.high_precision_offsets_enabled_flag;
   pic.Log2MaxTransformSkipSize = pps->log2_max_transform_skip_block_size_minus2 + 2;
   pic.CrossComponentPredictionEnable = pps->flags.cross_component_prediction_enabled_flag;
   pic.CABACBypassAlignmentEnable = sps->flags.cabac_bypass_alignment_enabled_flag;
   pic.PersistentRiceAdaptationEnable = sps->flags.persistent_rice_adaptation_enabled_flag;
   pic.IntraSmoothingDisable = sps->flags.intra_smoothing_disabled_flag;
   pic.ExplicitRDPCMEnable = sps->flags.explicit_rdpcm_enabled_flag;
   pic.ImplicitRDPCMEnable = sps->flags.implicit_rdpcm_enabled_flag;
   pic.TransformSkipContextEnable = sps->flags.transform_skip_context_enabled_flag;
   pic.TransformSkipRotationEnable = sps->flags.transform_skip_rotation_enabled_flag;
   #endif
            pic.CollocatedPictureIsISlice = false;
   pic.CurrentPictureIsISlice = false;
   pic.SampleAdaptiveOffsetEnable = sps->flags.sample_adaptive_offset_enabled_flag;
   pic.PCMEnable = sps->flags.pcm_enabled_flag;
   pic.CUQPDeltaEnable = pps->flags.cu_qp_delta_enabled_flag;
   pic.MaxDQPDepth = pps->diff_cu_qp_delta_depth;
   pic.PCMLoopFilterDisable = sps->flags.pcm_loop_filter_disabled_flag;
   pic.ConstrainedIntraPrediction = pps->flags.constrained_intra_pred_flag;
   pic.Log2ParallelMergeLevel = pps->log2_parallel_merge_level_minus2;
   pic.SignDataHiding = pps->flags.sign_data_hiding_enabled_flag;
   pic.LoopFilterEnable = pps->flags.loop_filter_across_tiles_enabled_flag;
   pic.EntropyCodingSyncEnable = pps->flags.entropy_coding_sync_enabled_flag;
   pic.TilingEnable = pps->flags.tiles_enabled_flag;
   pic.WeightedBiPredicationEnable = pps->flags.weighted_bipred_flag;
   pic.WeightedPredicationEnable = pps->flags.weighted_pred_flag;
   pic.FieldPic = 0;
   pic.TopField = true;
   pic.TransformSkipEnable = pps->flags.transform_skip_enabled_flag;
   pic.AMPEnable = sps->flags.amp_enabled_flag;
   pic.TransquantBypassEnable = pps->flags.transquant_bypass_enabled_flag;
   pic.StrongIntraSmoothingEnable = sps->flags.strong_intra_smoothing_enabled_flag;
            pic.PictureCbQPOffset = pps->pps_cb_qp_offset;
   pic.PictureCrQPOffset = pps->pps_cr_qp_offset;
   pic.IntraMaxTransformHierarchyDepth = sps->max_transform_hierarchy_depth_intra;
   pic.InterMaxTransformHierarchyDepth = sps->max_transform_hierarchy_depth_inter;
   pic.ChromaPCMSampleBitDepth = sps->pcm_sample_bit_depth_chroma_minus1 & 0xf;
            pic.ChromaBitDepth = sps->bit_depth_chroma_minus8;
      #if GFX_VER >= 11
         pic.CbQPOffsetList0 = pps->cb_qp_offset_list[0];
   pic.CbQPOffsetList1 = pps->cb_qp_offset_list[1];
   pic.CbQPOffsetList2 = pps->cb_qp_offset_list[2];
   pic.CbQPOffsetList3 = pps->cb_qp_offset_list[3];
   pic.CbQPOffsetList4 = pps->cb_qp_offset_list[4];
            pic.CrQPOffsetList0 = pps->cr_qp_offset_list[0];
   pic.CrQPOffsetList1 = pps->cr_qp_offset_list[1];
   pic.CrQPOffsetList2 = pps->cr_qp_offset_list[2];
   pic.CrQPOffsetList3 = pps->cr_qp_offset_list[3];
   pic.CrQPOffsetList4 = pps->cr_qp_offset_list[4];
   #endif
               if (pps->flags.tiles_enabled_flag) {
      int cum = 0;
   anv_batch_emit(&cmd_buffer->batch, GENX(HCP_TILE_STATE), tile) {
      tile.NumberofTileColumns = pps->num_tile_columns_minus1;
   tile.NumberofTileRows = pps->num_tile_rows_minus1;
   for (unsigned i = 0; i < 5; i++) {
      tile.ColumnPosition[i].CtbPos0i = cum;
                                 if ((4 * i + 1) == pps->num_tile_columns_minus1)
                        if ((4 * i + 2) == pps->num_tile_columns_minus1)
                        if ((4 * i + 3) >= MIN2(pps->num_tile_columns_minus1,
                                       for (unsigned i = 0; i < 5; i++) {
      tile.Rowposition[i].CtbPos0i = cum;
                                 if ((4 * i + 1) == pps->num_tile_rows_minus1)
                        if ((4 * i + 2) == pps->num_tile_rows_minus1)
                                                   if (pps->num_tile_rows_minus1 == 20) {
         }
   if (pps->num_tile_rows_minus1 == 20) {
      tile.Rowposition[5].CtbPos0i = cum;
   cum += pps->row_height_minus1[20] + 1;
                     /* Slice parsing */
   uint32_t last_slice = h265_pic_info->sliceSegmentCount - 1;
   void *slice_map = anv_gem_mmap(cmd_buffer->device, src_buffer->address.bo,
                     /* All slices should be parsed in advance to collect information necessary */
   for (unsigned s = 0; s < h265_pic_info->sliceSegmentCount; s++) {
      uint32_t current_offset = h265_pic_info->pSliceSegmentOffsets[s];
   void *map = slice_map + current_offset;
            if (s == last_slice)
         else
            vk_video_parse_h265_slice_header(frame_info, h265_pic_info, sps, pps, map, slice_size, &slice_params[s]);
                        for (unsigned s = 0; s < h265_pic_info->sliceSegmentCount; s++) {
      uint32_t ctb_size = 1 << (sps->log2_diff_max_min_luma_coding_block_size +
         uint32_t pic_width_in_min_cbs_y = sps->pic_width_in_luma_samples /
         uint32_t width_in_pix = (1 << (sps->log2_min_luma_coding_block_size_minus3 + 3)) *
         uint32_t ctb_w = DIV_ROUND_UP(width_in_pix, ctb_size);
   bool is_last = (s == last_slice);
            anv_batch_emit(&cmd_buffer->batch, GENX(HCP_SLICE_STATE), slice) {
                     if (is_last) {
      slice.NextSliceHorizontalPosition = 0;
      } else {
      slice.NextSliceHorizontalPosition = (slice_params[s + 1].slice_segment_address) % ctb_w;
               slice.SliceType = slice_params[s].slice_type;
   slice.LastSlice = is_last;
   slice.DependentSlice = slice_params[s].dependent_slice_segment;
   slice.SliceTemporalMVPEnable = slice_params[s].temporal_mvp_enable;
   slice.SliceQP = abs(slice_qp);
   slice.SliceQPSign = slice_qp >= 0 ? 0 : 1;
   slice.SliceCbQPOffset = slice_params[s].slice_cb_qp_offset;
   slice.SliceCrQPOffset = slice_params[s].slice_cr_qp_offset;
   slice.SliceHeaderDisableDeblockingFilter = pps->flags.deblocking_filter_override_enabled_flag ?
         slice.SliceTCOffsetDiv2 = slice_params[s].tc_offset_div2;
   slice.SliceBetaOffsetDiv2 = slice_params[s].beta_offset_div2;
   slice.SliceLoopFilterEnable = slice_params[s].loop_filter_across_slices_enable;
   slice.SliceSAOChroma = slice_params[s].sao_chroma_flag;
                           if (slice_params[s].slice_type == STD_VIDEO_H265_SLICE_TYPE_I) {
         } else {
                        if (vk_video_h265_poc_by_slot(frame_info, slot_idx) >
            low_delay = false;
                  for (unsigned i = 0; i < slice_params[s].num_ref_idx_l1_active; i++) {
      int slot_idx = ref_slots[1][i].slot_index;
   if (vk_video_h265_poc_by_slot(frame_info, slot_idx) >
            low_delay = false;
                     slice.LowDelay = low_delay;
   slice.CollocatedFromL0 = slice_params[s].collocated_list == 0 ? true : false;
   slice.Log2WeightDenominatorChroma = slice_params[s].luma_log2_weight_denom +
         slice.Log2WeightDenominatorLuma = slice_params[s].luma_log2_weight_denom;
   slice.CABACInit = slice_params[s].cabac_init_idc;
   slice.MaxMergeIndex = slice_params[s].max_num_merge_cand - 1;
   slice.CollocatedMVTemporalBufferIndex =
                  slice.SliceHeaderLength = slice_params[s].slice_data_bytes_offset;
   slice.CABACZeroWordInsertionEnable = false;
   slice.EmulationByteSliceInsertEnable = false;
   slice.TailInsertionPresent = false;
                  slice.IndirectPAKBSEDataStartOffset = 0;
   slice.TransformSkipLambda = 0;
   slice.TransformSkipNumberofNonZeroCoeffsFactor0 = 0;
   slice.TransformSkipNumberofZeroCoeffsFactor0 = 0;
         #if GFX_VER >= 12
               #endif
                  if (slice_params[s].slice_type != STD_VIDEO_H265_SLICE_TYPE_I) {
      anv_batch_emit(&cmd_buffer->batch, GENX(HCP_REF_IDX_STATE), ref) {
                     for (unsigned i = 0; i < ref.NumberofReferenceIndexesActive + 1; i++) {
                                    ref.ReferenceListEntry[i].ListEntry = dpb_idx[slot_idx];
   ref.ReferenceListEntry[i].ReferencePicturetbValue = CLAMP(diff_poc, -128, 127) & 0xff;
                     if (slice_params[s].slice_type == STD_VIDEO_H265_SLICE_TYPE_B) {
      anv_batch_emit(&cmd_buffer->batch, GENX(HCP_REF_IDX_STATE), ref) {
                     for (unsigned i = 0; i < ref.NumberofReferenceIndexesActive + 1; i++) {
                                    ref.ReferenceListEntry[i].ListEntry = dpb_idx[slot_idx];
   ref.ReferenceListEntry[i].ReferencePicturetbValue = CLAMP(diff_poc, -128, 127) & 0xff;
                     if ((pps->flags.weighted_pred_flag && (slice_params[s].slice_type == STD_VIDEO_H265_SLICE_TYPE_P)) ||
                              for (unsigned i = 0; i < ANV_VIDEO_H265_MAX_NUM_REF_FRAME; i++) {
      w.LumaOffsets[i].DeltaLumaWeightLX = slice_params[s].delta_luma_weight_l0[i] & 0xff;
   w.LumaOffsets[i].LumaOffsetLX = slice_params[s].luma_offset_l0[i] & 0xff;
   w.ChromaOffsets[i].DeltaChromaWeightLX0 = slice_params[s].delta_chroma_weight_l0[i][0] & 0xff;
   w.ChromaOffsets[i].ChromaOffsetLX0 = slice_params[s].chroma_offset_l0[i][0] & 0xff;
   w.ChromaOffsets[i].DeltaChromaWeightLX1 = slice_params[s].delta_chroma_weight_l0[i][1] & 0xff;
                  if (slice_params[s].slice_type == STD_VIDEO_H265_SLICE_TYPE_B) {
                        for (unsigned i = 0; i < ANV_VIDEO_H265_MAX_NUM_REF_FRAME; i++) {
      w.LumaOffsets[i].DeltaLumaWeightLX = slice_params[s].delta_luma_weight_l1[i] & 0xff;
   w.LumaOffsets[i].LumaOffsetLX = slice_params[s].luma_offset_l1[i] & 0xff;
   w.ChromaOffsets[i].DeltaChromaWeightLX0 = slice_params[s].delta_chroma_weight_l1[i][0] & 0xff;
   w.ChromaOffsets[i].DeltaChromaWeightLX1 = slice_params[s].delta_chroma_weight_l1[i][1] & 0xff;
   w.ChromaOffsets[i].ChromaOffsetLX0 = slice_params[s].chroma_offset_l1[i][0] & 0xff;
                                 anv_batch_emit(&cmd_buffer->batch, GENX(HCP_BSD_OBJECT), bsd) {
      bsd.IndirectBSDDataLength = slice_params[s].slice_size - 3;
               #if GFX_VER >= 12
      anv_batch_emit(&cmd_buffer->batch, GENX(VD_CONTROL_STATE), cs) {
            #endif
         anv_batch_emit(&cmd_buffer->batch, GENX(VD_PIPELINE_FLUSH), flush) {
      flush.HEVCPipelineDone = true;
   flush.HEVCPipelineCommandFlush = true;
         }
      static void
   anv_h264_decode_video(struct anv_cmd_buffer *cmd_buffer,
         {
      ANV_FROM_HANDLE(anv_buffer, src_buffer, frame_info->srcBuffer);
   struct anv_video_session *vid = cmd_buffer->video.vid;
   struct anv_video_session_params *params = cmd_buffer->video.params;
   const struct VkVideoDecodeH264PictureInfoKHR *h264_pic_info =
         const StdVideoH264SequenceParameterSet *sps = vk_video_find_h264_dec_std_sps(&params->vk, h264_pic_info->pStdPictureInfo->seq_parameter_set_id);
            anv_batch_emit(&cmd_buffer->batch, GENX(MI_FLUSH_DW), flush) {
      flush.DWordLength = 2;
            #if GFX_VER >= 12
      anv_batch_emit(&cmd_buffer->batch, GENX(MI_FORCE_WAKEUP), wake) {
      wake.MFXPowerWellControl = 1;
               anv_batch_emit(&cmd_buffer->batch, GENX(MFX_WAIT), mfx) {
            #endif
         anv_batch_emit(&cmd_buffer->batch, GENX(MFX_PIPE_MODE_SELECT), sel) {
      sel.StandardSelect = SS_AVC;
   sel.CodecSelect = Decode;
   sel.DecoderShortFormatMode = ShortFormatDriverInterface;
            sel.PreDeblockingOutputEnable = 0;
            #if GFX_VER >= 12
      anv_batch_emit(&cmd_buffer->batch, GENX(MFX_WAIT), mfx) {
            #endif
         const struct anv_image_view *iv = anv_image_view_from_handle(frame_info->dstPictureResource.imageViewBinding);
   const struct anv_image *img = iv->image;
   anv_batch_emit(&cmd_buffer->batch, GENX(MFX_SURFACE_STATE), ss) {
      ss.Width = img->vk.extent.width - 1;
   ss.Height = img->vk.extent.height - 1;
   ss.SurfaceFormat = PLANAR_420_8; // assert on this?
   ss.InterleaveChroma = 1;
   ss.SurfacePitch = img->planes[0].primary_surface.isl.row_pitch_B - 1;
   ss.TiledSurface = img->planes[0].primary_surface.isl.tiling != ISL_TILING_LINEAR;
            ss.YOffsetforUCb = ss.YOffsetforVCr =
               anv_batch_emit(&cmd_buffer->batch, GENX(MFX_PIPE_BUF_ADDR_STATE), buf) {
      bool use_pre_deblock = false;
   if (use_pre_deblock) {
      buf.PreDeblockingDestinationAddress = anv_image_address(img,
      } else {
      buf.PostDeblockingDestinationAddress = anv_image_address(img,
      }
   buf.PreDeblockingDestinationAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
         };
   buf.PostDeblockingDestinationAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
                  buf.IntraRowStoreScratchBufferAddress = (struct anv_address) { vid->vid_mem[ANV_VID_MEM_H264_INTRA_ROW_STORE].mem->bo, vid->vid_mem[ANV_VID_MEM_H264_INTRA_ROW_STORE].offset };
   buf.IntraRowStoreScratchBufferAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
         };
   buf.DeblockingFilterRowStoreScratchAddress = (struct anv_address) { vid->vid_mem[ANV_VID_MEM_H264_DEBLOCK_FILTER_ROW_STORE].mem->bo, vid->vid_mem[ANV_VID_MEM_H264_DEBLOCK_FILTER_ROW_STORE].offset };
   buf.DeblockingFilterRowStoreScratchAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
         };
   buf.MBStatusBufferAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
         };
   buf.MBILDBStreamOutBufferAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
         };
   buf.SecondMBILDBStreamOutBufferAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
         };
   buf.ScaledReferenceSurfaceAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
         };
   buf.OriginalUncompressedPictureSourceAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
         };
   buf.StreamOutDataDestinationAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
                  struct anv_bo *ref_bo = NULL;
   for (unsigned i = 0; i < frame_info->referenceSlotCount; i++) {
      const struct anv_image_view *ref_iv = anv_image_view_from_handle(frame_info->pReferenceSlots[i].pPictureResource->imageViewBinding);
   int idx = frame_info->pReferenceSlots[i].slotIndex;
                  if (i == 0) {
            }
   buf.ReferencePictureAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
                     anv_batch_emit(&cmd_buffer->batch, GENX(MFX_IND_OBJ_BASE_ADDR_STATE), index_obj) {
      index_obj.MFXIndirectBitstreamObjectAddress = anv_address_add(src_buffer->address,
         index_obj.MFXIndirectBitstreamObjectAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
         };
   index_obj.MFXIndirectMVObjectAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
         };
   index_obj.MFDIndirectITCOEFFObjectAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
         };
   index_obj.MFDIndirectITDBLKObjectAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
         };
   index_obj.MFCIndirectPAKBSEObjectAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
                     anv_batch_emit(&cmd_buffer->batch, GENX(MFX_BSP_BUF_BASE_ADDR_STATE), bsp) {
      bsp.BSDMPCRowStoreScratchBufferAddress = (struct anv_address) { vid->vid_mem[ANV_VID_MEM_H264_BSD_MPC_ROW_SCRATCH].mem->bo,
            bsp.BSDMPCRowStoreScratchBufferAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
         };
   bsp.MPRRowStoreScratchBufferAddress = (struct anv_address) { vid->vid_mem[ANV_VID_MEM_H264_MPR_ROW_SCRATCH].mem->bo,
            bsp.MPRRowStoreScratchBufferAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
         };
   bsp.BitplaneReadBufferAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
                     anv_batch_emit(&cmd_buffer->batch, GENX(MFD_AVC_DPB_STATE), avc_dpb) {
      for (unsigned i = 0; i < frame_info->referenceSlotCount; i++) {
      const struct VkVideoDecodeH264DpbSlotInfoKHR *dpb_slot =
         const StdVideoDecodeH264ReferenceInfo *ref_info = dpb_slot->pStdReferenceInfo;
   int idx = frame_info->pReferenceSlots[i].slotIndex;
   avc_dpb.NonExistingFrame[idx] = ref_info->flags.is_non_existing;
   avc_dpb.LongTermFrame[idx] = ref_info->flags.used_for_long_term_reference;
   if (!ref_info->flags.top_field_flag && !ref_info->flags.bottom_field_flag)
         else
                        anv_batch_emit(&cmd_buffer->batch, GENX(MFD_AVC_PICID_STATE), picid) {
                  uint32_t pic_height = sps->pic_height_in_map_units_minus1 + 1;
   if (!sps->flags.frame_mbs_only_flag)
         anv_batch_emit(&cmd_buffer->batch, GENX(MFX_AVC_IMG_STATE), avc_img) {
      avc_img.FrameWidth = sps->pic_width_in_mbs_minus1;
   avc_img.FrameHeight = pic_height - 1;
            if (!h264_pic_info->pStdPictureInfo->flags.field_pic_flag)
         else if (h264_pic_info->pStdPictureInfo->flags.bottom_field_flag)
         else
            avc_img.WeightedBiPredictionIDC = pps->weighted_bipred_idc;
   avc_img.WeightedPredictionEnable = pps->flags.weighted_pred_flag;
   avc_img.FirstChromaQPOffset = pps->chroma_qp_index_offset;
   avc_img.SecondChromaQPOffset = pps->second_chroma_qp_index_offset;
   avc_img.FieldPicture = h264_pic_info->pStdPictureInfo->flags.field_pic_flag;
   avc_img.MBAFFMode = (sps->flags.mb_adaptive_frame_field_flag &&
         avc_img.FrameMBOnly = sps->flags.frame_mbs_only_flag;
   avc_img._8x8IDCTTransformMode = pps->flags.transform_8x8_mode_flag;
   avc_img.Direct8x8Inference = sps->flags.direct_8x8_inference_flag;
   avc_img.ConstrainedIntraPrediction = pps->flags.constrained_intra_pred_flag;
   avc_img.NonReferencePicture = !h264_pic_info->pStdPictureInfo->flags.is_reference;
   avc_img.EntropyCodingSyncEnable = pps->flags.entropy_coding_mode_flag;
   avc_img.ChromaFormatIDC = sps->chroma_format_idc;
   avc_img.TrellisQuantizationChromaDisable = true;
   avc_img.NumberofReferenceFrames = frame_info->referenceSlotCount;
   avc_img.NumberofActiveReferencePicturesfromL0 = pps->num_ref_idx_l0_default_active_minus1 + 1;
   avc_img.NumberofActiveReferencePicturesfromL1 = pps->num_ref_idx_l1_default_active_minus1 + 1;
   avc_img.InitialQPValue = pps->pic_init_qp_minus26;
   avc_img.PicOrderPresent = pps->flags.bottom_field_pic_order_in_frame_present_flag;
   avc_img.DeltaPicOrderAlwaysZero = sps->flags.delta_pic_order_always_zero_flag;
   avc_img.PicOrderCountType = sps->pic_order_cnt_type;
   avc_img.DeblockingFilterControlPresent = pps->flags.deblocking_filter_control_present_flag;
   avc_img.RedundantPicCountPresent = pps->flags.redundant_pic_cnt_present_flag;
   avc_img.Log2MaxFrameNumber = sps->log2_max_frame_num_minus4;
   avc_img.Log2MaxPicOrderCountLSB = sps->log2_max_pic_order_cnt_lsb_minus4;
               StdVideoH264ScalingLists scaling_lists;
   vk_video_derive_h264_scaling_list(sps, pps, &scaling_lists);
   anv_batch_emit(&cmd_buffer->batch, GENX(MFX_QM_STATE), qm) {
      qm.DWordLength = 16;
   qm.AVC = AVC_4x4_Intra_MATRIX;
   for (unsigned m = 0; m < 3; m++)
      for (unsigned q = 0; q < 16; q++)
   }
   anv_batch_emit(&cmd_buffer->batch, GENX(MFX_QM_STATE), qm) {
      qm.DWordLength = 16;
   qm.AVC = AVC_4x4_Inter_MATRIX;
   for (unsigned m = 0; m < 3; m++)
      for (unsigned q = 0; q < 16; q++)
   }
   if (pps->flags.transform_8x8_mode_flag) {
      anv_batch_emit(&cmd_buffer->batch, GENX(MFX_QM_STATE), qm) {
      qm.DWordLength = 16;
   qm.AVC = AVC_8x8_Intra_MATRIX;
   for (unsigned q = 0; q < 64; q++)
      }
   anv_batch_emit(&cmd_buffer->batch, GENX(MFX_QM_STATE), qm) {
      qm.DWordLength = 16;
   qm.AVC = AVC_8x8_Inter_MATRIX;
   for (unsigned q = 0; q < 64; q++)
                  anv_batch_emit(&cmd_buffer->batch, GENX(MFX_AVC_DIRECTMODE_STATE), avc_directmode) {
      /* bind reference frame DMV */
   struct anv_bo *dmv_bo = NULL;
   for (unsigned i = 0; i < frame_info->referenceSlotCount; i++) {
      int idx = frame_info->pReferenceSlots[i].slotIndex;
   const struct VkVideoDecodeH264DpbSlotInfoKHR *dpb_slot =
         const struct anv_image_view *ref_iv = anv_image_view_from_handle(frame_info->pReferenceSlots[i].pPictureResource->imageViewBinding);
   const StdVideoDecodeH264ReferenceInfo *ref_info = dpb_slot->pStdReferenceInfo;
   avc_directmode.DirectMVBufferAddress[idx] = anv_image_address(ref_iv->image,
         if (i == 0) {
         }
   avc_directmode.POCList[2 * idx] = ref_info->PicOrderCnt[0];
      }
   avc_directmode.DirectMVBufferAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
                  avc_directmode.DirectMVBufferWriteAddress = anv_image_address(img,
         avc_directmode.DirectMVBufferWriteAttributes = (struct GENX(MEMORYADDRESSATTRIBUTES)) {
         };
   avc_directmode.POCList[32] = h264_pic_info->pStdPictureInfo->PicOrderCnt[0];
                  #define HEADER_OFFSET 3
      for (unsigned s = 0; s < h264_pic_info->sliceCount; s++) {
      bool last_slice = s == (h264_pic_info->sliceCount - 1);
   uint32_t current_offset = h264_pic_info->pSliceOffsets[s];
   uint32_t this_end;
   if (!last_slice) {
      uint32_t next_offset = h264_pic_info->pSliceOffsets[s + 1];
   uint32_t next_end = h264_pic_info->pSliceOffsets[s + 2];
   if (s == h264_pic_info->sliceCount - 2)
         anv_batch_emit(&cmd_buffer->batch, GENX(MFD_AVC_SLICEADDR), sliceaddr) {
      sliceaddr.IndirectBSDDataLength = next_end - next_offset - HEADER_OFFSET;
   /* start decoding after the 3-byte header. */
      };
      } else
         anv_batch_emit(&cmd_buffer->batch, GENX(MFD_AVC_BSD_OBJECT), avc_bsd) {
      avc_bsd.IndirectBSDDataLength = this_end - current_offset - HEADER_OFFSET;
   /* start decoding after the 3-byte header. */
   avc_bsd.IndirectBSDDataStartAddress = buffer_offset + current_offset + HEADER_OFFSET;
   avc_bsd.InlineData.LastSlice = last_slice;
   avc_bsd.InlineData.FixPrevMBSkipped = 1;
   avc_bsd.InlineData.IntraPredictionErrorControl = 1;
   avc_bsd.InlineData.Intra8x84x4PredictionErrorConcealmentControl = 1;
            }
      void
   genX(CmdDecodeVideoKHR)(VkCommandBuffer commandBuffer,
         {
               switch (cmd_buffer->video.vid->vk.op) {
   case VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR:
      anv_h264_decode_video(cmd_buffer, frame_info);
      case VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR:
      anv_h265_decode_video(cmd_buffer, frame_info);
      default:
            }
      #ifdef VK_ENABLE_BETA_EXTENSIONS
   void
   genX(CmdEncodeVideoKHR)(VkCommandBuffer commandBuffer,
         {
   }
   #endif
