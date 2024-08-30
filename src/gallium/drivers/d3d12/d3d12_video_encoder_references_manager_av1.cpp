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
      #include "d3d12_video_enc.h"
   #include "d3d12_video_enc_av1.h"
   #include "d3d12_video_encoder_references_manager_av1.h"
   #include <algorithm>
   #include <string>
   #include "d3d12_screen.h"
      using namespace std;
      d3d12_video_encoder_references_manager_av1::d3d12_video_encoder_references_manager_av1(
      bool gopHasInterFrames, d3d12_video_dpb_storage_manager_interface &rDpbStorageManager)
   : m_CurrentFrameReferencesData({}),
   m_PhysicalAllocationsStorage(rDpbStorageManager),
   m_gopHasInterFrames(gopHasInterFrames),
   m_isCurrentFrameUsedAsReference(false),
      {
      assert((NUM_REF_FRAMES + 1 /*extra for cur frame output recon pic*/) ==
            debug_printf("[D3D12 Video Encoder Picture Manager AV1] Completed construction of "
      }
      void
   d3d12_video_encoder_references_manager_av1::clear_dpb()
   {
      // Reset m_CurrentFrameReferencesData tracking
   m_CurrentFrameReferencesData.pVirtualDPBEntries.clear();
   m_CurrentFrameReferencesData.pVirtualDPBEntries.resize(NUM_REF_FRAMES);
            // Initialize DPB slots as unused
   for (size_t i = 0; i < NUM_REF_FRAMES; i++)
      m_CurrentFrameReferencesData.pVirtualDPBEntries[i].ReconstructedPictureResourceIndex =
         // Reset physical DPB underlying storage
   ASSERTED uint32_t numPicsBeforeClearInDPB = m_PhysicalAllocationsStorage.get_number_of_pics_in_dpb();
   ASSERTED uint32_t cFreedResources = m_PhysicalAllocationsStorage.clear_decode_picture_buffer();
      }
      D3D12_VIDEO_ENCODER_RECONSTRUCTED_PICTURE
   d3d12_video_encoder_references_manager_av1::get_current_frame_recon_pic_output_allocation()
   {
         }
      bool
   d3d12_video_encoder_references_manager_av1::is_current_frame_used_as_reference()
   {
         }
      void
   d3d12_video_encoder_references_manager_av1::begin_frame(D3D12_VIDEO_ENCODER_PICTURE_CONTROL_CODEC_DATA curFrameData,
               {
      m_CurrentFramePicParams = *curFrameData.pAV1PicData;
            if (m_CurrentFramePicParams.FrameType == D3D12_VIDEO_ENCODER_AV1_FRAME_TYPE_KEY_FRAME)
            // Prepare current frame recon pic allocation
   m_CurrentFrameReferencesData.ReconstructedPicTexture = { nullptr, 0 };
   if (is_current_frame_used_as_reference() && m_gopHasInterFrames) {
      auto reconPic = m_PhysicalAllocationsStorage.get_new_tracked_picture_allocation();
   m_CurrentFrameReferencesData.ReconstructedPicTexture.pReconstructedPicture = reconPic.pReconstructedPicture;
   m_CurrentFrameReferencesData.ReconstructedPicTexture.ReconstructedPictureSubresource =
            #ifdef DEBUG
      assert(m_PhysicalAllocationsStorage.get_number_of_tracked_allocations() <=
            if (m_CurrentFramePicParams.FrameType == D3D12_VIDEO_ENCODER_AV1_FRAME_TYPE_KEY_FRAME) {
      // After clearing the DPB, outstanding used allocations should be 1u only for the first allocation for the
   // reconstructed picture of the initial KEY_FRAME
   assert(m_PhysicalAllocationsStorage.get_number_of_in_use_allocations() ==
         #endif
   }
      void
   d3d12_video_encoder_references_manager_av1::end_frame()
   {
         }
      D3D12_VIDEO_ENCODE_REFERENCE_FRAMES
   d3d12_video_encoder_references_manager_av1::get_current_reference_frames()
   {
      D3D12_VIDEO_ENCODE_REFERENCE_FRAMES retVal = { 0,
                              if ((m_CurrentFramePicParams.FrameType != D3D12_VIDEO_ENCODER_AV1_FRAME_TYPE_KEY_FRAME) && m_gopHasInterFrames) {
      auto curRef = m_PhysicalAllocationsStorage.get_current_reference_frames();
   retVal.NumTexture2Ds = curRef.NumTexture2Ds;
   retVal.ppTexture2Ds = curRef.ppTexture2Ds;
                  }
      void
   d3d12_video_encoder_references_manager_av1::print_ref_frame_idx()
   {
      std::string refListContentsString;
   for (uint32_t idx = 0; idx < REFS_PER_FRAME; idx++) {
      uint32_t reference = 0;
   reference = m_CurrentFramePicParams.ReferenceIndices[idx];
   refListContentsString += "{ ref_frame_idx[" + std::to_string(idx) + "] - ";
   refListContentsString += " DPBidx: ";
   refListContentsString += std::to_string(reference);
   if (m_CurrentFrameReferencesData.pVirtualDPBEntries[reference].ReconstructedPictureResourceIndex !=
      UNUSED_VIRTUAL_DPB_SLOT_PHYSICAL_INDEX) {
   refListContentsString += " - OrderHint: ";
   refListContentsString += std::to_string(m_CurrentFrameReferencesData.pVirtualDPBEntries[reference].OrderHint);
   refListContentsString += " - PictureIndex: ";
   refListContentsString +=
      } else {
                              debug_printf("[D3D12 Video Encoder Picture Manager AV1] ref_frame_idx[REFS_PER_FRAME] for frame with OrderHint %d "
               "(PictureIndex %d) is: \n%s \n",
   m_CurrentFramePicParams.OrderHint,
   debug_printf("[D3D12 Video Encoder Picture Manager AV1] Requested PrimaryRefFrame: %d\n",
         debug_printf("[D3D12 Video Encoder Picture Manager AV1] Requested RefreshFrameFlags: %d\n",
      }
      void
   d3d12_video_encoder_references_manager_av1::print_virtual_dpb_entries()
   {
      std::string dpbContents;
   for (uint32_t dpbResIdx = 0; dpbResIdx < m_CurrentFrameReferencesData.pVirtualDPBEntries.size(); dpbResIdx++) {
               if (dpbDesc.ReconstructedPictureResourceIndex != UNUSED_VIRTUAL_DPB_SLOT_PHYSICAL_INDEX) {
      d3d12_video_reconstructed_picture dpbEntry =
         dpbContents += "{ DPBidx: ";
   dpbContents += std::to_string(dpbResIdx);
   dpbContents += " - OrderHint: ";
   dpbContents += std::to_string(dpbDesc.OrderHint);
   dpbContents += " - PictureIndex: ";
   dpbContents += std::to_string(dpbDesc.PictureIndex);
   dpbContents += " - DPBStorageIdx: ";
   dpbContents += std::to_string(dpbDesc.ReconstructedPictureResourceIndex);
   dpbContents += " - DPBStorageResourcePtr: ";
   char strBuf[256];
   memset(&strBuf, '\0', 256);
   sprintf(strBuf, "%p", dpbEntry.pReconstructedPicture);
   dpbContents += std::string(strBuf);
   dpbContents += " - DPBStorageSubresource: ";
   dpbContents += std::to_string(dpbEntry.ReconstructedPictureSubresource);
   dpbContents += " - RefCount (from any ref_frame_idx[0..6]): ";
   dpbContents += std::to_string(get_dpb_virtual_slot_refcount_from_ref_frame_idx(dpbResIdx));
      } else {
      dpbContents += "{ DPBidx: ";
   dpbContents += std::to_string(dpbResIdx);
                  debug_printf(
      "[D3D12 Video Encoder Picture Manager AV1] DPB Current output reconstructed picture %p subresource %d\n",
   m_CurrentFrameReferencesData.ReconstructedPicTexture.pReconstructedPicture,
         debug_printf("[D3D12 Video Encoder Picture Manager AV1] NumTexture2Ds is %d frames - "
               "Number of DPB virtual entries is %" PRIu64 " entries for frame with OrderHint "
   "%d (PictureIndex %d) are: \n%s \n",
   m_PhysicalAllocationsStorage.get_number_of_pics_in_dpb(),
   static_cast<uint64_t>(m_CurrentFrameReferencesData.pVirtualDPBEntries.size()),
      }
      void
   d3d12_video_encoder_references_manager_av1::print_physical_resource_references()
   {
      debug_printf("[D3D12 Video Encoder Picture Manager AV1] %d physical resources IN USE out of a total of %d physical "
               "resources ALLOCATED "
   "resources at end_frame for frame with OrderHint %d (PictureIndex %d)\n",
   m_PhysicalAllocationsStorage.get_number_of_in_use_allocations(),
            D3D12_VIDEO_ENCODE_REFERENCE_FRAMES curRefs = get_current_reference_frames();
   std::string descContents;
   for (uint32_t index = 0; index < curRefs.NumTexture2Ds; index++) {
      assert(curRefs.ppTexture2Ds[index]);   // These must be contiguous when sending down to D3D12 API
   descContents += "{ REFERENCE_FRAMES Index: ";
   descContents += std::to_string(index);
   descContents += " - ppTextures ptr: ";
   char strBuf[256];
   memset(&strBuf, '\0', 256);
   sprintf(strBuf, "%p", curRefs.ppTexture2Ds[index]);
   descContents += std::string(strBuf);
   descContents += " - ppSubresources index: ";
   if (curRefs.pSubresources != nullptr)
         else
         descContents += " - RefCount (from any virtual dpb slot [0..REFS_PER_FRAME]): ";
   descContents += std::to_string(get_dpb_physical_slot_refcount_from_virtual_dpb(index));
               debug_printf("[D3D12 Video Encoder Picture Manager AV1] D3D12_VIDEO_ENCODE_REFERENCE_FRAMES has %d physical "
               "resources in ppTexture2Ds for OrderHint "
   "%d (PictureIndex %d) are: \n%s \n",
   curRefs.NumTexture2Ds,
      }
      //
   // Returns the number of Reference<XXX> (ie. LAST, LAST2, ..., ALTREF, etc)
   // pointing to dpbSlotIndex
   //
   uint32_t
   d3d12_video_encoder_references_manager_av1::get_dpb_virtual_slot_refcount_from_ref_frame_idx(uint32_t dpbSlotIndex)
   {
      uint32_t refCount = 0;
   for (uint8_t i = 0; i < ARRAY_SIZE(m_CurrentFramePicParams.ReferenceIndices); i++) {
      if (m_CurrentFramePicParams.ReferenceIndices[i] == dpbSlotIndex) {
                        }
      //
   // Returns the number of entries in the virtual DPB descriptors
   // that point to physicalSlotIndex
   //
   uint32_t
   d3d12_video_encoder_references_manager_av1::get_dpb_physical_slot_refcount_from_virtual_dpb(uint32_t physicalSlotIndex)
   {
      uint32_t refCount = 0;
   for (unsigned dpbSlotIndex = 0; dpbSlotIndex < m_CurrentFrameReferencesData.pVirtualDPBEntries.size();
      dpbSlotIndex++) {
   if (m_CurrentFrameReferencesData.pVirtualDPBEntries[dpbSlotIndex].ReconstructedPictureResourceIndex ==
      physicalSlotIndex)
   }
      }
      void
   d3d12_video_encoder_references_manager_av1::get_current_frame_picture_control_data(
         {
      assert(m_CurrentFrameReferencesData.pVirtualDPBEntries.size() == NUM_REF_FRAMES);
            // Some apps don't clean this up for INTRA/KEY frames
   if ((m_CurrentFramePicParams.FrameType != D3D12_VIDEO_ENCODER_AV1_FRAME_TYPE_INTER_FRAME)
   && (m_CurrentFramePicParams.FrameType != D3D12_VIDEO_ENCODER_AV1_FRAME_TYPE_SWITCH_FRAME))
   {
      for (uint8_t i = 0; i < ARRAY_SIZE(m_CurrentFramePicParams.ReferenceIndices); i++) {
                     for (uint8_t i = 0; i < NUM_REF_FRAMES; i++)
      m_CurrentFramePicParams.ReferenceFramesReconPictureDescriptors[i] =
      #ifdef DEBUG   // Otherwise may iterate over structures and do no-op debug_printf
      print_ref_frame_idx();
   print_virtual_dpb_entries();
      #endif
            }
      void
   d3d12_video_encoder_references_manager_av1::refresh_dpb_slots_with_current_frame_reconpic()
   {
      UINT refresh_frame_flags = m_CurrentFramePicParams.RefreshFrameFlags;
   debug_printf("[D3D12 Video Encoder Picture Manager AV1] refresh_frame_flags 0x%x for frame with OrderHint %d "
               "(PictureIndex %d)\n",
            //
   // Put current frame in all slots of DPB indicated by refresh_frame_flags
   //
               //
   // First do a eviction pass and update virtual DPB physical indices in case the physical array shifted with an
   // eviction (erasing an ppTexture2Ds entry)
            for (unsigned dpbSlotIdx = 0; dpbSlotIdx < NUM_REF_FRAMES; dpbSlotIdx++) {
                  //
   // Check if the slot this reconpic will take in the virtual DPB will leave an unreferenced
                                                         // Get the number of entries in the virtual DPB descriptors that point to
   // ReconstructedPictureResourceIndex of the current virtual dpb slot (counting the current dpbSlotIdx we
   // didn't clear yet)
   uint32_t numRefs = get_dpb_physical_slot_refcount_from_virtual_dpb(
                                 bool wasTracked = false;
   m_PhysicalAllocationsStorage.remove_reference_frame(
                        // Indices in the virtual dpb higher or equal than
   // m_CurrentFrameReferencesData.pVirtualDPBEntries[dpbSlotIdx].ReconstructedPictureResourceIndex
   // must be shifted back one place as we erased the entry in the physical allocations array (ppTexture2Ds)
   for (auto &dpbVirtualEntry : m_CurrentFrameReferencesData.pVirtualDPBEntries) {
      if (
      // Check for slot being used
                              // Decrease the index to compensate for the removed ppTexture2Ds entry
                     // Clear this virtual dpb entry (so next iterations will decrease refcount)
   m_CurrentFrameReferencesData.pVirtualDPBEntries[dpbSlotIdx].ReconstructedPictureResourceIndex =
                     //
   // Find a new free physical index for the current recon pic; always put new physical entry at the end to avoid
   // having to shift existing indices in virtual DPB
   //
   UINT allocationSlotIdx = m_PhysicalAllocationsStorage.get_number_of_pics_in_dpb();
   assert(static_cast<uint32_t>(allocationSlotIdx) < NUM_REF_FRAMES);
   D3D12_VIDEO_ENCODER_RECONSTRUCTED_PICTURE recAlloc = get_current_frame_recon_pic_output_allocation();
   d3d12_video_reconstructed_picture refFrameDesc = {};
   refFrameDesc.pReconstructedPicture = recAlloc.pReconstructedPicture;
   refFrameDesc.ReconstructedPictureSubresource = recAlloc.ReconstructedPictureSubresource;
            //
   // Update virtual DPB entries with the current frame recon picture
   //
   for (unsigned dpbSlotIdx = 0; dpbSlotIdx < NUM_REF_FRAMES; dpbSlotIdx++) {
      if (((refresh_frame_flags >> dpbSlotIdx) & 0x1) != 0) {
      m_CurrentFrameReferencesData.pVirtualDPBEntries[dpbSlotIdx] = { allocationSlotIdx,
                                                   // Number of allocations, disregarding if they are used or not, should not exceed this limit due to reuse policies
   // on DPB items removal.
      }
