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
      #include "d3d12_video_texture_array_dpb_manager.h"
      #include "d3d12_common.h"
      #include "d3d12_util.h"
      ///
   /// d3d12_texture_array_dpb_manager
   ///
      // Differences with ArrayOfTextures
   // Uses a D3D12 Texture Array instead of an std::vector with individual D3D resources as backing storage
   // Doesn't support extension (by reallocation and copy) of the pool
      void
   d3d12_texture_array_dpb_manager::create_reconstructed_picture_allocations(ID3D12Resource **ppResource,
         {
      if (texArraySize > 0) {
      D3D12_HEAP_PROPERTIES Properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT, m_nodeMask, m_nodeMask);
   CD3DX12_RESOURCE_DESC reconstructedPictureResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(m_encodeFormat,
                                                HRESULT hr = m_pDevice->CreateCommittedResource(&Properties,
                                 if (FAILED(hr)) {
      debug_printf("CreateCommittedResource failed with HR %x\n", hr);
            }
      d3d12_texture_array_dpb_manager::~d3d12_texture_array_dpb_manager()
   { }
      d3d12_texture_array_dpb_manager::d3d12_texture_array_dpb_manager(
      uint16_t                                    dpbTextureArraySize,
   ID3D12Device *                              pDevice,
   DXGI_FORMAT                                 encodeSessionFormat,
   D3D12_VIDEO_ENCODER_PICTURE_RESOLUTION_DESC encodeSessionResolution,
   D3D12_RESOURCE_FLAGS                        resourceAllocFlags,
   uint32_t                                    nodeMask)
   : m_pDevice(pDevice),
   m_encodeFormat(encodeSessionFormat),
   m_encodeResolution(encodeSessionResolution),
   m_dpbTextureArraySize(dpbTextureArraySize),
   m_resourceAllocFlags(resourceAllocFlags),
      {
      // Initialize D3D12 DPB exposed in this class implemented CRUD interface for a DPB
            // Implement a reusable pool of D3D12 Resources as an array of textures
   uint16_t poolFixedSize = m_dpbTextureArraySize;
            // Build resource pool with commitedresources with a d3ddevice and the encoding session settings (eg. resolution) and
   // the reference_only flag
            for (uint32_t idxSubres = 0; idxSubres < poolFixedSize; idxSubres++) {
      m_ResourcesPool[idxSubres].pResource   = m_baseTexArrayResource;
   m_ResourcesPool[idxSubres].subresource = idxSubres;
         }
      uint32_t
   d3d12_texture_array_dpb_manager::clear_decode_picture_buffer()
   {
               uint32_t untrackCount = 0;
   // Mark resources used in DPB as re-usable in the resources pool
   for (uint32_t idx = 0; idx < m_D3D12DPB.pResources.size(); idx++) {
      // Don't assert the untracking result here in case the DPB contains resources not adquired using the pool methods
   // in this interface
   untrackCount +=
      untrack_reconstructed_picture_allocation({ m_D3D12DPB.pResources[idx], m_D3D12DPB.pSubresources[idx] }) ? 1 :
            // Clear DPB
   m_D3D12DPB.pResources.clear();
   m_D3D12DPB.pSubresources.clear();
   m_D3D12DPB.pHeaps.clear();
   m_D3D12DPB.pResources.reserve(m_dpbTextureArraySize);
   m_D3D12DPB.pSubresources.reserve(m_dpbTextureArraySize);
               }
      // Assigns a reference frame at a given position
   void
   d3d12_texture_array_dpb_manager::assign_reference_frame(d3d12_video_reconstructed_picture pReconPicture,
         {
      assert(m_D3D12DPB.pResources.size() == m_D3D12DPB.pSubresources.size());
                     m_D3D12DPB.pResources[dpbPosition]    = pReconPicture.pReconstructedPicture;
   m_D3D12DPB.pSubresources[dpbPosition] = pReconPicture.ReconstructedPictureSubresource;
      }
      // Adds a new reference frame at a given position
   void
   d3d12_texture_array_dpb_manager::insert_reference_frame(d3d12_video_reconstructed_picture pReconPicture,
         {
      assert(m_D3D12DPB.pResources.size() == m_D3D12DPB.pSubresources.size());
            if (dpbPosition > m_D3D12DPB.pResources.size()) {
      // extend capacity
   m_D3D12DPB.pResources.resize(dpbPosition);
   m_D3D12DPB.pSubresources.resize(dpbPosition);
               m_D3D12DPB.pResources.insert(m_D3D12DPB.pResources.begin() + dpbPosition, pReconPicture.pReconstructedPicture);
   m_D3D12DPB.pSubresources.insert(m_D3D12DPB.pSubresources.begin() + dpbPosition,
            }
      // Gets a reference frame at a given position
   d3d12_video_reconstructed_picture
   d3d12_texture_array_dpb_manager::get_reference_frame(uint32_t dpbPosition)
   {
               d3d12_video_reconstructed_picture retVal = { m_D3D12DPB.pResources[dpbPosition],
                     }
      // Removes a new reference frame at a given position and returns operation success
   bool
   d3d12_texture_array_dpb_manager::remove_reference_frame(uint32_t dpbPosition, bool *pResourceUntracked)
   {
      assert(m_D3D12DPB.pResources.size() == m_D3D12DPB.pSubresources.size());
                     // If removed resource came from resource pool, mark it as free
   // to free it for a new usage
   // Don't assert the untracking result here in case the DPB contains resources not adquired using the pool methods in
   // this interface
   bool resUntracked = untrack_reconstructed_picture_allocation(
            if (pResourceUntracked != nullptr) {
                  // Remove from DPB tables
   m_D3D12DPB.pResources.erase(m_D3D12DPB.pResources.begin() + dpbPosition);
   m_D3D12DPB.pSubresources.erase(m_D3D12DPB.pSubresources.begin() + dpbPosition);
               }
      // Returns true if the trackedItem was allocated (and is being tracked) by this class
   bool
   d3d12_texture_array_dpb_manager::is_tracked_allocation(d3d12_video_reconstructed_picture trackedItem)
   {
      for (auto &reusableRes : m_ResourcesPool) {
      if ((trackedItem.pReconstructedPicture == reusableRes.pResource.Get()) &&
      (trackedItem.ReconstructedPictureSubresource == reusableRes.subresource) && !reusableRes.isFree) {
         }
      }
      // Returns whether it found the tracked resource on this instance pool tracking and was able to free it
   bool
   d3d12_texture_array_dpb_manager::untrack_reconstructed_picture_allocation(d3d12_video_reconstructed_picture trackedItem)
   {
      for (auto &reusableRes : m_ResourcesPool) {
      if ((trackedItem.pReconstructedPicture == reusableRes.pResource.Get()) &&
      (trackedItem.ReconstructedPictureSubresource == reusableRes.subresource)) {
   reusableRes.isFree = true;
         }
      }
      // Returns a fresh resource for a NEW picture to be written to
   // this class implements the dpb allocations as an array of textures
   d3d12_video_reconstructed_picture
   d3d12_texture_array_dpb_manager::get_new_tracked_picture_allocation()
   {
      d3d12_video_reconstructed_picture freshAllocation = { // pResource
                              // Find first (if any) available resource to (re-)use
   bool bAvailableResourceInPool = false;
   for (auto &reusableRes : m_ResourcesPool) {
      if (reusableRes.isFree) {
      bAvailableResourceInPool                        = true;
   freshAllocation.pReconstructedPicture           = reusableRes.pResource.Get();
   freshAllocation.ReconstructedPictureSubresource = reusableRes.subresource;
   reusableRes.isFree                              = false;
                  if (!bAvailableResourceInPool) {
      debug_printf("[d3d12_texture_array_dpb_manager] ID3D12Resource pool is full - Pool capacity (%" PRIu32 ") - Returning null allocation",
                  }
      uint32_t
   d3d12_texture_array_dpb_manager::get_number_of_pics_in_dpb()
   {
      assert(m_D3D12DPB.pResources.size() == m_D3D12DPB.pSubresources.size());
   assert(m_D3D12DPB.pResources.size() == m_D3D12DPB.pHeaps.size());
   assert(m_D3D12DPB.pResources.size() < UINT32_MAX);
      }
      d3d12_video_reference_frames
   d3d12_texture_array_dpb_manager::get_current_reference_frames()
   {
      d3d12_video_reference_frames retVal = {
      get_number_of_pics_in_dpb(),
   m_D3D12DPB.pResources.data(),
   m_D3D12DPB.pSubresources.data(),
                  }
      // number of resources in the pool that are marked as in use
   uint32_t
   d3d12_texture_array_dpb_manager::get_number_of_in_use_allocations()
   {
      uint32_t countOfInUseResourcesInPool = 0;
   for (auto &reusableRes : m_ResourcesPool) {
      if (!reusableRes.isFree) {
            }
      }
      // Returns the number of pictures currently stored in the DPB
   uint32_t
   d3d12_texture_array_dpb_manager::get_number_of_tracked_allocations()
   {
      assert(m_ResourcesPool.size() < UINT32_MAX);
      }
