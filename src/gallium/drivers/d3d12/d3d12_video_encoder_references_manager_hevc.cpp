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
      #include "d3d12_video_encoder_references_manager_hevc.h"
   #include <algorithm>
   #include <string>
   #include "d3d12_screen.h"
      using namespace std;
      d3d12_video_encoder_references_manager_hevc::d3d12_video_encoder_references_manager_hevc(
      bool gopHasIorPFrames, d3d12_video_dpb_storage_manager_interface &rDpbStorageManager, uint32_t MaxDPBCapacity)
   : m_MaxDPBCapacity(MaxDPBCapacity),
   m_rDPBStorageManager(rDpbStorageManager),
   m_CurrentFrameReferencesData({}),
      {
      assert((m_MaxDPBCapacity + 1 /*extra for cur frame output recon pic*/) ==
            debug_printf("[D3D12 Video Encoder Picture Manager HEVC] Completed construction of "
            }
      void
   d3d12_video_encoder_references_manager_hevc::reset_gop_tracking_and_dpb()
   {
      // Reset m_CurrentFrameReferencesData tracking
   m_CurrentFrameReferencesData.pReferenceFramesReconPictureDescriptors.clear();
   m_CurrentFrameReferencesData.pReferenceFramesReconPictureDescriptors.reserve(m_MaxDPBCapacity);
   m_curFrameStateDescriptorStorage.reserve(m_MaxDPBCapacity);
            // Reset DPB storage
   ASSERTED uint32_t numPicsBeforeClearInDPB = m_rDPBStorageManager.get_number_of_pics_in_dpb();
   ASSERTED uint32_t cFreedResources = m_rDPBStorageManager.clear_decode_picture_buffer();
            // Initialize if needed the reconstructed picture allocation for the first IDR picture in the GOP
   // This needs to be done after initializing the GOP tracking state above since it makes decisions based on the
   // current picture type.
            // After clearing the DPB, outstanding used allocations should be 1u only for the first allocation for the
   // reconstructed picture of the initial IDR in the GOP
   assert(m_rDPBStorageManager.get_number_of_in_use_allocations() == (m_gopHasInterFrames ? 1u : 0u));
   assert(m_rDPBStorageManager.get_number_of_tracked_allocations() <=
      }
      // Calculates the picture control structure for the current frame
   void
   d3d12_video_encoder_references_manager_hevc::get_current_frame_picture_control_data(
         {
      // Update reference picture control structures (L0/L1 and DPB descriptors lists based on current frame and next frame
            debug_printf("[D3D12 Video Encoder Picture Manager HEVC] %d resources IN USE out of a total of %d ALLOCATED "
               "resources at frame with POC: %d\n",
            // See casts below
            bool needsL0List = (m_curFrameState.FrameType == D3D12_VIDEO_ENCODER_FRAME_TYPE_HEVC_P_FRAME) ||
         bool needsL1List = (m_curFrameState.FrameType == D3D12_VIDEO_ENCODER_FRAME_TYPE_HEVC_B_FRAME);
      // (in HEVC I pics might contain following pics, ref not used in curr)    
   bool needsRefPicDescriptors = (m_curFrameState.FrameType != D3D12_VIDEO_ENCODER_FRAME_TYPE_HEVC_IDR_FRAME);
               // See D3D12 Encode spec below
   // pList0ReferenceFrames
   //    List of past frame reference frames to be used for this frame. Each integer value in this array indices into
   //    pReferenceFramesReconPictureDescriptors to reference pictures kept in the DPB.
   // pList1ReferenceFrames
   //    List of future frame reference frames to be used for this frame. Each integer value in this array indices into
            // Need to map from frame_num in the receiving ref_idx_l0_list/ref_idx_l1_list to the position with that
                     // Set all in the DPB as unused for the current frame, then below just mark as used the ones references by L0 and L1
   for(UINT idx = 0 ; idx < m_CurrentFrameReferencesData.pReferenceFramesReconPictureDescriptors.size() ; idx++)
   {
                  if (needsL0List && (m_curFrameState.List0ReferenceFramesCount > 0)) {
      std::vector<uint32_t> tmpL0(m_curFrameState.List0ReferenceFramesCount, 0);
   memcpy(tmpL0.data(),
                  for (size_t l0Idx = 0; l0Idx < m_curFrameState.List0ReferenceFramesCount; l0Idx++) {
      // tmpL0[l0Idx] has frame_num's (reference_lists_frame_idx)
   // m_curFrameState.pList0ReferenceFrames[l0Idx] needs to have the index j of
                  auto value = tmpL0[l0Idx];
   auto foundItemIt = std::find_if(m_CurrentFrameReferencesData.pReferenceFramesReconPictureDescriptors.begin(),
                              assert(foundItemIt != m_CurrentFrameReferencesData.pReferenceFramesReconPictureDescriptors.end());
   m_curFrameState.pList0ReferenceFrames[l0Idx] =
                        if (needsL1List && (m_curFrameState.List1ReferenceFramesCount > 0)) {
      std::vector<uint32_t> tmpL1(m_curFrameState.List1ReferenceFramesCount, 0);
   memcpy(tmpL1.data(),
                  for (size_t l1Idx = 0; l1Idx < m_curFrameState.List1ReferenceFramesCount; l1Idx++) {
      // tmpL1[l1Idx] has frame_num's (reference_lists_frame_idx)
   // m_curFrameState.pList1ReferenceFrames[l1Idx] needs to have the index j of
                  auto value = tmpL1[l1Idx];
   auto foundItemIt = std::find_if(m_CurrentFrameReferencesData.pReferenceFramesReconPictureDescriptors.begin(),
                              assert(foundItemIt != m_CurrentFrameReferencesData.pReferenceFramesReconPictureDescriptors.end());
   m_curFrameState.pList1ReferenceFrames[l1Idx] =
                        m_curFrameState.List0ReferenceFramesCount = needsL0List ? m_curFrameState.List0ReferenceFramesCount : 0;
   m_curFrameState.pList0ReferenceFrames = needsL0List ? m_curFrameState.pList0ReferenceFrames : nullptr,
   m_curFrameState.List1ReferenceFramesCount = needsL1List ? m_curFrameState.List1ReferenceFramesCount : 0,
   m_curFrameState.pList1ReferenceFrames = needsL1List ? m_curFrameState.pList1ReferenceFrames : nullptr;
      if (!needsRefPicDescriptors) {
      m_curFrameState.ReferenceFramesReconPictureDescriptorsCount = 0;
      } else {
      m_curFrameState.ReferenceFramesReconPictureDescriptorsCount = static_cast<uint32_t>(m_CurrentFrameReferencesData.pReferenceFramesReconPictureDescriptors.size());
   m_curFrameStateDescriptorStorage.resize(m_curFrameState.ReferenceFramesReconPictureDescriptorsCount);
   for(uint32_t idx = 0; idx < m_curFrameState.ReferenceFramesReconPictureDescriptorsCount ; idx++) {
         }
                        print_l0_l1_lists();
      }
      // Returns the resource allocation for a reconstructed picture output for the current frame
   D3D12_VIDEO_ENCODER_RECONSTRUCTED_PICTURE
   d3d12_video_encoder_references_manager_hevc::get_current_frame_recon_pic_output_allocation()
   {
         }
      D3D12_VIDEO_ENCODE_REFERENCE_FRAMES
   d3d12_video_encoder_references_manager_hevc::get_current_reference_frames()
   {
      D3D12_VIDEO_ENCODE_REFERENCE_FRAMES retVal = { 0,
                              // Return nullptr for fully intra frames (eg IDR)
            if ((m_curFrameState.FrameType != D3D12_VIDEO_ENCODER_FRAME_TYPE_HEVC_IDR_FRAME) && m_gopHasInterFrames) {
      auto curRef = m_rDPBStorageManager.get_current_reference_frames();
   retVal.NumTexture2Ds = curRef.NumTexture2Ds;
   retVal.ppTexture2Ds = curRef.ppTexture2Ds;
                  }
      void
   d3d12_video_encoder_references_manager_hevc::prepare_current_frame_recon_pic_allocation()
   {
               // If all GOP are intra frames, no point in doing reference pic allocations
   if (is_current_frame_used_as_reference() && m_gopHasInterFrames) {
      auto reconPic = m_rDPBStorageManager.get_new_tracked_picture_allocation();
   m_CurrentFrameReferencesData.ReconstructedPicTexture.pReconstructedPicture = reconPic.pReconstructedPicture;
   m_CurrentFrameReferencesData.ReconstructedPicTexture.ReconstructedPictureSubresource =
         }
      void
   d3d12_video_encoder_references_manager_hevc::update_fifo_dpb_push_front_cur_recon_pic()
   {
      // Keep the order of the dpb storage and dpb descriptors in a circular buffer
   // order such that the DPB array consists of a sequence of frames in DECREASING encoding order
   // eg. last frame encoded at first, followed by one to last frames encoded, and at the end
            // If current pic was not used as reference, current reconstructed picture resource is empty,
   // No need to to anything in that case.
            // If GOP are all intra frames, do nothing also.
   if (is_current_frame_used_as_reference() && m_gopHasInterFrames) {
      debug_printf("[D3D12 Video Encoder Picture Manager HEVC] MaxDPBCapacity is %d - Number of pics in DPB is %d "
               "when trying to put frame with POC %d (frame_num %d) at front of the DPB\n",
   m_MaxDPBCapacity,
            // Release least recently used in DPB if we filled the m_MaxDPBCapacity allowed
   if (m_rDPBStorageManager.get_number_of_pics_in_dpb() == m_MaxDPBCapacity) {
      bool untrackedRes = false;
   m_rDPBStorageManager.remove_reference_frame(m_rDPBStorageManager.get_number_of_pics_in_dpb() - 1,
         // Verify that resource was untracked since this class is using the pool completely for allocations
   assert(untrackedRes);
               // Add new dpb to front of DPB
   D3D12_VIDEO_ENCODER_RECONSTRUCTED_PICTURE recAlloc = get_current_frame_recon_pic_output_allocation();
   d3d12_video_reconstructed_picture refFrameDesc = {};
   refFrameDesc.pReconstructedPicture = recAlloc.pReconstructedPicture;
   refFrameDesc.ReconstructedPictureSubresource = recAlloc.ReconstructedPictureSubresource;
   refFrameDesc.pVideoHeap = nullptr;   // D3D12 Video Encode does not need the D3D12VideoEncoderHeap struct for HEVC
                  // Prepare D3D12_VIDEO_ENCODER_REFERENCE_PICTURE_DESCRIPTOR_HEVC_EX for added DPB member
   D3D12_VIDEO_ENCODER_REFERENCE_PICTURE_DESCRIPTOR_HEVC_EX newDPBDescriptor = 
   {
      // D3D12_VIDEO_ENCODER_REFERENCE_PICTURE_DESCRIPTOR_HEVC
   {
      // UINT ReconstructedPictureResourceIndex;
   0, // the associated reconstructed picture is also being pushed_front in m_rDPBStorageManager
   // BOOL IsRefUsedByCurrentPic; 
   false, // usage is determined in method AdvanceFrame according to picture type to be encoded
   // BOOL IsLongTermReference
   false, // not implemented - neither FFMPEG or GSTREAMER use LTR for HEVC VAAPI
   // UINT PictureOrderCountNumber;
   m_curFrameState.PictureOrderCountNumber,
   // UINT TemporalLayerIndex;
      },
   // reference_lists_frame_idx
               // Add DPB entry
   m_CurrentFrameReferencesData.pReferenceFramesReconPictureDescriptors.insert(
                  // Update the indices for ReconstructedPictureResourceIndex in pReferenceFramesReconPictureDescriptors
   // to be in identity mapping with m_rDPBStorageManager indices
   // after pushing the elements to the right in the push_front operation
   for (uint32_t dpbResIdx = 1;
      dpbResIdx < m_CurrentFrameReferencesData.pReferenceFramesReconPictureDescriptors.size();
   dpbResIdx++) {
   auto &dpbDesc = m_CurrentFrameReferencesData.pReferenceFramesReconPictureDescriptors[dpbResIdx];
                  // Number of allocations, disregarding if they are used or not, should not exceed this limit due to reuse policies on
   // DPB items removal.
      }
      void
   d3d12_video_encoder_references_manager_hevc::print_l0_l1_lists()
   {
      if ((D3D12_DEBUG_VERBOSE & d3d12_debug) &&
      ((m_curFrameState.FrameType == D3D12_VIDEO_ENCODER_FRAME_TYPE_HEVC_P_FRAME) ||
   (m_curFrameState.FrameType == D3D12_VIDEO_ENCODER_FRAME_TYPE_HEVC_B_FRAME))) {
   std::string list0ContentsString;
   for (uint32_t idx = 0; idx < m_curFrameState.List0ReferenceFramesCount; idx++) {
      uint32_t value = m_curFrameState.pList0ReferenceFrames[idx];
   list0ContentsString += "{ DPBidx: ";
   list0ContentsString += std::to_string(value);
   list0ContentsString += " - POC: ";
   list0ContentsString += std::to_string(
         list0ContentsString += " - IsRefUsedByCurrentPic: ";
   list0ContentsString += std::to_string(
         list0ContentsString += " - IsLongTermReference: ";
   list0ContentsString += std::to_string(
         list0ContentsString += " - reference_lists_frame_idx: ";
   list0ContentsString += std::to_string(
                     debug_printf(
      "[D3D12 Video Encoder Picture Manager HEVC] L0 list for frame with POC %d (frame_num %d) is: \n %s \n",
   m_curFrameState.PictureOrderCountNumber,
               std::string list1ContentsString;
   for (uint32_t idx = 0; idx < m_curFrameState.List1ReferenceFramesCount; idx++) {
      uint32_t value = m_curFrameState.pList1ReferenceFrames[idx];
   list1ContentsString += "{ DPBidx: ";
   list1ContentsString += std::to_string(value);
   list1ContentsString += " - POC: ";
   list1ContentsString += std::to_string(
         list1ContentsString += " - IsRefUsedByCurrentPic: ";
   list1ContentsString += std::to_string(
         list1ContentsString += " - IsLongTermReference: ";
   list1ContentsString += std::to_string(
         list1ContentsString += " - reference_lists_frame_idx: ";
   list1ContentsString += std::to_string(
                     debug_printf(
      "[D3D12 Video Encoder Picture Manager HEVC] L1 list for frame with POC %d (frame_num %d) is: \n %s \n",
   m_curFrameState.PictureOrderCountNumber,
   m_current_frame_idx,
      }
      void
   d3d12_video_encoder_references_manager_hevc::print_dpb()
   {
      if (D3D12_DEBUG_VERBOSE & d3d12_debug) {
      std::string dpbContents;
   for (uint32_t dpbResIdx = 0;
      dpbResIdx < m_CurrentFrameReferencesData.pReferenceFramesReconPictureDescriptors.size();
   dpbResIdx++) {
                  dpbContents += "{ DPBidx: ";
   dpbContents += std::to_string(dpbResIdx);
   dpbContents += " - POC: ";
   dpbContents += std::to_string(dpbDesc.base.PictureOrderCountNumber);
   dpbContents += " - IsRefUsedByCurrentPic: ";
   dpbContents += std::to_string(dpbDesc.base.IsRefUsedByCurrentPic);
   dpbContents += " - DPBStorageIdx: ";
   dpbContents += std::to_string(dpbDesc.base.ReconstructedPictureResourceIndex);
   dpbContents += " - reference_lists_frame_idx: ";
   dpbContents += std::to_string(dpbDesc.reference_lists_frame_idx);
   dpbContents += " - DPBStorageResourcePtr: ";
   char strBuf[256];
   memset(&strBuf, '\0', 256);
   sprintf(strBuf, "%p", dpbEntry.pReconstructedPicture);
   dpbContents += std::string(strBuf);
   dpbContents += " - DPBStorageSubresource: ";
   dpbContents += std::to_string(dpbEntry.ReconstructedPictureSubresource);
               debug_printf("[D3D12 Video Encoder Picture Manager HEVC] DPB has %d frames - DPB references for frame with POC "
               "%d are: \n %s \n",
         }
      // Advances state to next frame in GOP; subsequent calls to GetCurrentFrame* point to the advanced frame status
   void
   d3d12_video_encoder_references_manager_hevc::end_frame()
   {
      debug_printf("[D3D12 Video Encoder Picture Manager HEVC] %d resources IN USE out of a total of %d ALLOCATED "
               "resources at end_frame for frame with POC: %d\n",
            // Adds last used (if not null) get_current_frame_recon_pic_output_allocation to DPB for next EncodeFrame if
               }
      bool
   d3d12_video_encoder_references_manager_hevc::is_current_frame_used_as_reference()
   {
         }
      void
   d3d12_video_encoder_references_manager_hevc::begin_frame(D3D12_VIDEO_ENCODER_PICTURE_CONTROL_CODEC_DATA curFrameData,
         {
               m_curFrameState = *curFrameData.pHEVCPicData;
   m_isCurrentFrameUsedAsReference = bUsedAsReference;
   m_current_frame_idx = pPipeDesc->frame_num;
   debug_printf("Marking POC %d (frame_num %d) as reference ? %d\n",
                        // Advance the GOP tracking state
   bool isDPBFlushNeeded = (m_curFrameState.FrameType == D3D12_VIDEO_ENCODER_FRAME_TYPE_HEVC_IDR_FRAME);
   if (isDPBFlushNeeded) {
         } else {
      // Get new allocation from DPB storage for reconstructed picture
   // This is only necessary for the frames that come after an IDR
   // since in the initial state already has this initialized
                  }
