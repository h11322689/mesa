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
      #include "d3d12_video_dec_references_mgr.h"
   #include "d3d12_video_dec_h264.h"
   #include "d3d12_video_dec_hevc.h"
   #include "d3d12_video_dec_av1.h"
   #include "d3d12_video_dec_vp9.h"
   #include "d3d12_video_texture_array_dpb_manager.h"
   #include "d3d12_video_array_of_textures_dpb_manager.h"
   #include "d3d12_screen.h"
   #include "d3d12_resource.h"
   #include "d3d12_video_buffer.h"
   #include <algorithm>
   #include <string>
      //----------------------------------------------------------------------------------------------------------------------------------
   static uint16_t
   GetInvalidReferenceIndex(d3d12_video_decode_profile_type DecodeProfileType)
   {
               switch (DecodeProfileType) {
      case d3d12_video_decode_profile_type_h264:
         case d3d12_video_decode_profile_type_hevc:
         case d3d12_video_decode_profile_type_av1:
         case d3d12_video_decode_profile_type_vp9:
         default:
         }
      //----------------------------------------------------------------------------------------------------------------------------------
   ///
   /// This should always be a clear (non ref only) texture, to be presented downstream as the decoded texture
   /// Please see get_reference_only_output for the current frame recon pic ref only allocation
   ///
   void
   d3d12_video_decoder_references_manager::get_current_frame_decode_output_texture(struct pipe_video_buffer *  pCurrentDecodeTarget,
               {
   // First try to find if there's an existing entry for this pCurrentDecodeTarget already in the DPB
      // For interlaced scenarios, multiple end_frame calls will need to reference the same texture for top/bottom
   assert(m_DecodeTargetToOriginalIndex7Bits.count(pCurrentDecodeTarget) > 0); // Needs to already have a Index7Bits assigned for current pic params
            if((remappedIdx != m_invalidIndex) && !(is_reference_only())) { // If it already has a remapped index in use, reuse that allocation
      // return the existing allocation for this decode target
   d3d12_video_reconstructed_picture reconPicture = m_upD3D12TexturesStorageManager->get_reference_frame(remappedIdx);
   *ppOutTexture2D       = reconPicture.pReconstructedPicture;
      } else {
      if (is_pipe_buffer_underlying_output_decode_allocation()) {
      auto vidBuffer = (struct d3d12_video_buffer *)(pCurrentDecodeTarget);
   *ppOutTexture2D       = d3d12_resource_resource(vidBuffer->texture);
      } else {
      d3d12_video_reconstructed_picture pFreshAllocation =
         *ppOutTexture2D       = pFreshAllocation.pReconstructedPicture;
            }
      //----------------------------------------------------------------------------------------------------------------------------------
   _Use_decl_annotations_ void
   d3d12_video_decoder_references_manager::get_reference_only_output(
      struct pipe_video_buffer *  pCurrentDecodeTarget,
   ID3D12Resource **ppOutputReference,     // out -> new reference slot assigned or nullptr
   uint32_t *       pOutputSubresource,    // out -> new reference slot assigned or nullptr
   bool &outNeedsTransitionToDecodeWrite   // out -> indicates if output resource argument has to be transitioned to
      )
   {
               // First try to find if there's an existing entry for this pCurrentDecodeTarget already in the DPB
   // For interlaced scenarios, multiple end_frame calls will need to reference the same texture for top/bottom
   assert(m_DecodeTargetToOriginalIndex7Bits.count(pCurrentDecodeTarget) > 0); // Needs to already have a Index7Bits assigned for current pic params
            if(remappedIdx != m_invalidIndex) { // If it already has a remapped index in use, reuse that allocation
      // return the existing allocation for this decode target
   d3d12_video_reconstructed_picture reconPicture = m_upD3D12TexturesStorageManager->get_reference_frame(remappedIdx);
   *ppOutputReference              = reconPicture.pReconstructedPicture;
   *pOutputSubresource             = reconPicture.ReconstructedPictureSubresource;
      } else {
      // The DPB Storage only has REFERENCE_ONLY allocations, use one of those.
   d3d12_video_reconstructed_picture pFreshAllocation =
         *ppOutputReference              = pFreshAllocation.pReconstructedPicture;
   *pOutputSubresource             = pFreshAllocation.ReconstructedPictureSubresource;
         }
      //----------------------------------------------------------------------------------------------------------------------------------
   D3D12_VIDEO_DECODE_REFERENCE_FRAMES
   d3d12_video_decoder_references_manager::get_current_reference_frames()
   {
                  // Convert generic IUnknown into the actual decoder heap object
   m_ppHeaps.resize(args.NumTexture2Ds, nullptr);
   HRESULT hr = S_OK;
   for (uint32_t i = 0; i < args.NumTexture2Ds; i++) {
      if (args.ppHeaps[i]) {
      hr = args.ppHeaps[i]->QueryInterface(IID_PPV_ARGS(&m_ppHeaps[i]));
      } else {
                     D3D12_VIDEO_DECODE_REFERENCE_FRAMES retVal = {
      args.NumTexture2Ds,
   args.ppTexture2Ds,
   args.pSubresources,
                  }
      //----------------------------------------------------------------------------------------------------------------------------------
   _Use_decl_annotations_
   d3d12_video_decoder_references_manager::d3d12_video_decoder_references_manager(
      const struct d3d12_screen *       pD3D12Screen,
   uint32_t                          NodeMask,
   d3d12_video_decode_profile_type   DecodeProfileType,
   d3d12_video_decode_dpb_descriptor m_dpbDescriptor)
   : m_DecodeTargetToOriginalIndex7Bits({ }),
   m_pD3D12Screen(pD3D12Screen),
   m_invalidIndex(GetInvalidReferenceIndex(DecodeProfileType)),
   m_dpbDescriptor(m_dpbDescriptor),
      {
      ASSERTED HRESULT hr = m_pD3D12Screen->dev->CheckFeatureSupport(D3D12_FEATURE_FORMAT_INFO, &m_formatInfo, sizeof(m_formatInfo));
   assert(SUCCEEDED(hr));
   D3D12_VIDEO_ENCODER_PICTURE_RESOLUTION_DESC targetFrameResolution = { static_cast<uint32_t>(m_dpbDescriptor.Width),
         D3D12_RESOURCE_FLAGS                        resourceAllocFlags =
      m_dpbDescriptor.fReferenceOnly ?
               if (m_dpbDescriptor.fArrayOfTexture) {
      // If all subresources are 0, the DPB is loaded with an array of individual textures, the D3D Encode API expects
   // pSubresources to be null in this case The D3D Decode API expects it to be non-null even with all zeroes.
   bool setNullSubresourcesOnAllZero = false;
   m_upD3D12TexturesStorageManager =
      std::make_unique<d3d12_array_of_textures_dpb_manager>(m_dpbDescriptor.dpbSize,
                                       } else {
      m_upD3D12TexturesStorageManager = std::make_unique<d3d12_texture_array_dpb_manager>(m_dpbDescriptor.dpbSize,
                                                         for (uint32_t dpbIdx = 0; dpbIdx < m_dpbDescriptor.dpbSize; dpbIdx++) {
                  mark_all_references_as_unused();
      }
      //----------------------------------------------------------------------------------------------------------------------------------
   uint16_t
   d3d12_video_decoder_references_manager::find_remapped_index(uint16_t originalIndex)
   {
      // Check if the index is already mapped.
   for (uint16_t remappedIndex = 0; remappedIndex < m_dpbDescriptor.dpbSize; remappedIndex++) {
      if (m_referenceDXVAIndices[remappedIndex].originalIndex == originalIndex) {
                        }
      //----------------------------------------------------------------------------------------------------------------------------------
   uint16_t
   d3d12_video_decoder_references_manager::update_entry(
      uint16_t         index,                // in
   ID3D12Resource *&pOutputReference,     // out -> new reference slot assigned or nullptr
   uint32_t &       OutputSubresource,    // out -> new reference slot assigned or 0
   bool &outNeedsTransitionToDecodeRead   // out -> indicates if output resource argument has to be transitioned to
      )
   {
      uint16_t remappedIndex         = m_invalidIndex;
            if (index != m_invalidIndex) {
               outNeedsTransitionToDecodeRead = true;
   if (remappedIndex == m_invalidIndex || remappedIndex == m_currentOutputIndex) {
               remappedIndex                  = m_currentOutputIndex;
               d3d12_video_reconstructed_picture reconPicture =
         pOutputReference  = outNeedsTransitionToDecodeRead ? reconPicture.pReconstructedPicture : nullptr;
                  }
      //----------------------------------------------------------------------------------------------------------------------------------
   _Use_decl_annotations_ uint16_t
   d3d12_video_decoder_references_manager::store_future_reference(uint16_t                        index,
                     {
      // Check if the index was in use.
            if (remappedIndex == m_invalidIndex) {
      // The current output index was not used last frame.  Get an unused entry.
               if (remappedIndex == m_invalidIndex) {
      debug_printf(
      "[d3d12_video_decoder_references_manager] d3d12_video_decoder_references_manager - Decode - No available "
                  // Set the index as the key in this map entry.
   m_referenceDXVAIndices[remappedIndex].originalIndex = index;
   IUnknown *pUnkHeap                                  = nullptr;
   ASSERTED HRESULT hr = decoderHeap.Get()->QueryInterface(IID_PPV_ARGS(&pUnkHeap));
   assert(SUCCEEDED(hr));
                     // Store the index to use for error handling when caller specifies and invalid reference index.
   m_currentOutputIndex = remappedIndex;
   m_currentSubresourceIndex = subresourceIndex;
               }
      //----------------------------------------------------------------------------------------------------------------------------------
   void
   d3d12_video_decoder_references_manager::mark_reference_in_use(uint16_t index)
   {
      if (index != m_invalidIndex) {
      uint16_t remappedIndex = find_remapped_index(index);
   if (remappedIndex != m_invalidIndex) {
               }
      //----------------------------------------------------------------------------------------------------------------------------------
   void
   d3d12_video_decoder_references_manager::release_unused_references_texture_memory()
   {
      for (uint32_t index = 0; index < m_dpbDescriptor.dpbSize; index++) {
      if (!m_referenceDXVAIndices[index].fUsed) {
      d3d12_video_reconstructed_picture reconPicture = m_upD3D12TexturesStorageManager->get_reference_frame(index);
   if (reconPicture.pReconstructedPicture != nullptr) {
      ASSERTED bool wasTracked = m_upD3D12TexturesStorageManager->untrack_reconstructed_picture_allocation(reconPicture);
   // Untrack this resource, will mark it as free un the underlying storage buffer pool
                                          // Remove the entry in m_DecodeTargetToOriginalIndex7Bits
   auto value = m_referenceDXVAIndices[index].originalIndex;
   auto it = std::find_if(m_DecodeTargetToOriginalIndex7Bits.begin(), m_DecodeTargetToOriginalIndex7Bits.end(),
                                                         }
      //----------------------------------------------------------------------------------------------------------------------------------
   void
   d3d12_video_decoder_references_manager::mark_all_references_as_unused()
   {
      for (uint32_t index = 0; index < m_dpbDescriptor.dpbSize; index++) {
            }
      //----------------------------------------------------------------------------------------------------------------------------------
   void
   d3d12_video_decoder_references_manager::print_dpb()
   {
      // Resource backing storage always has to match dpbsize
   if(!is_pipe_buffer_underlying_output_decode_allocation()) {
                  // get_current_reference_frames query-interfaces the pVideoHeap's.
   D3D12_VIDEO_DECODE_REFERENCE_FRAMES curRefFrames = get_current_reference_frames();
   std::string dpbContents;
   for (uint32_t dpbResIdx = 0;dpbResIdx < curRefFrames.NumTexture2Ds;dpbResIdx++) {
      dpbContents += "\t{ DPBidx: ";
   dpbContents += std::to_string(dpbResIdx);
   dpbContents += " - ResourcePtr: ";
   char strBufTex[256];
   memset(&strBufTex, '\0', 256);
   sprintf(strBufTex, "%p", curRefFrames.ppTexture2Ds[dpbResIdx]);
   dpbContents += std::string(strBufTex);
   dpbContents += " - SubresourceIdx: ";
   dpbContents += (curRefFrames.pSubresources ? std::to_string(curRefFrames.pSubresources[dpbResIdx]) : "0");
   dpbContents += " - DecoderHeapPtr: ";
   char strBufHeap[256];
   memset(&strBufHeap, '\0', 256);
   if(curRefFrames.ppHeaps && curRefFrames.ppHeaps[dpbResIdx]) {
      sprintf(strBufHeap, "%p", curRefFrames.ppHeaps[dpbResIdx]);
      } else {
         }
   dpbContents += " - Slot type: ";
   dpbContents +=  ((m_currentResource == curRefFrames.ppTexture2Ds[dpbResIdx]) && (m_currentSubresourceIndex == curRefFrames.pSubresources[dpbResIdx])) ? "Current decoded frame output" : "Reference frame";
   dpbContents += " - DXVA_PicParams Reference Index: ";
   dpbContents += (m_referenceDXVAIndices[dpbResIdx].originalIndex != m_invalidIndex) ? std::to_string(m_referenceDXVAIndices[dpbResIdx].originalIndex) : "DXVA_UNUSED_PICENTRY";
               debug_printf("[D3D12 Video Decoder Picture Manager] Decode session information:\n"
               "\tDPB Maximum Size (max_ref_count + one_slot_curpic): %d\n"
   "\tDXGI_FORMAT: %d\n"
   "\tTexture resolution: (%" PRIu64 ", %d)\n"
   "\tD3D12_RESOURCE_FLAG_VIDEO_DECODE_REFERENCE_ONLY enforced: %d\n"
   "\tAllocation Mode: %s\n"
   "\n ----------------------\n\tCurrent frame information:\n"
   "\tD3D12_VIDEO_DECODE_REFERENCE_FRAMES.NumTexture2Ds: %d\n"
   "\tDPB Contents Table:\n%s",
   m_upD3D12TexturesStorageManager->get_number_of_tracked_allocations(),
   m_dpbDescriptor.Format,
   m_dpbDescriptor.Width,
   m_dpbDescriptor.Height,
   m_dpbDescriptor.fReferenceOnly,
      }
