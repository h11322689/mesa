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
      #include "d3d12_common.h"
      #include "d3d12_util.h"
   #include "d3d12_context.h"
   #include "d3d12_format.h"
   #include "d3d12_resource.h"
   #include "d3d12_screen.h"
   #include "d3d12_surface.h"
   #include "d3d12_video_enc.h"
   #include "d3d12_video_enc_h264.h"
   #include "d3d12_video_enc_hevc.h"
   #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
   #include "d3d12_video_enc_av1.h"
   #endif
   #include "d3d12_video_buffer.h"
   #include "d3d12_video_texture_array_dpb_manager.h"
   #include "d3d12_video_array_of_textures_dpb_manager.h"
   #include "d3d12_video_encoder_references_manager_h264.h"
   #include "d3d12_video_encoder_references_manager_hevc.h"
   #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
   #include "d3d12_video_encoder_references_manager_av1.h"
   #endif
   #include "d3d12_residency.h"
      #include "vl/vl_video_buffer.h"
   #include "util/format/u_format.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "util/u_video.h"
      #include <cmath>
      uint64_t
   d3d12_video_encoder_pool_current_index(struct d3d12_video_encoder *pD3D12Enc)
   {
         }
      void
   d3d12_video_encoder_flush(struct pipe_video_codec *codec)
   {
      struct d3d12_video_encoder *pD3D12Enc = (struct d3d12_video_encoder *) codec;
   assert(pD3D12Enc);
   assert(pD3D12Enc->m_spD3D12VideoDevice);
            // Flush any work batched (ie. shaders blit on input texture, etc or bitstream headers buffer_subdata batched upload)
   // and Wait the m_spEncodeCommandQueue for GPU upload completion
   // before recording EncodeFrame below.
   struct pipe_fence_handle *completion_fence = NULL;
   debug_printf("[d3d12_video_encoder] d3d12_video_encoder_flush - Flushing pD3D12Enc->base.context and GPU sync between Video/Context queues before flushing Video Encode Queue.\n");
   pD3D12Enc->base.context->flush(pD3D12Enc->base.context, &completion_fence, PIPE_FLUSH_ASYNC | PIPE_FLUSH_HINT_FINISH);
   assert(completion_fence);
   struct d3d12_fence *casted_completion_fence = d3d12_fence(completion_fence);
   pD3D12Enc->m_spEncodeCommandQueue->Wait(casted_completion_fence->cmdqueue_fence, casted_completion_fence->value);
            struct d3d12_fence *input_surface_fence = pD3D12Enc->m_inflightResourcesPool[d3d12_video_encoder_pool_current_index(pD3D12Enc)].m_InputSurfaceFence;
   if (input_surface_fence)
            if (!pD3D12Enc->m_bPendingWorkNotFlushed) {
         } else {
      debug_printf("[d3d12_video_encoder] d3d12_video_encoder_flush started. Will flush video queue work async"
                  HRESULT hr = pD3D12Enc->m_pD3D12Screen->dev->GetDeviceRemovedReason();
   if (hr != S_OK) {
      debug_printf("[d3d12_video_encoder] d3d12_video_encoder_flush"
                                 if (pD3D12Enc->m_transitionsBeforeCloseCmdList.size() > 0) {
      pD3D12Enc->m_spEncodeCommandList->ResourceBarrier(pD3D12Enc->m_transitionsBeforeCloseCmdList.size(),
                     hr = pD3D12Enc->m_spEncodeCommandList->Close();
   if (FAILED(hr)) {
      debug_printf("[d3d12_video_encoder] d3d12_video_encoder_flush - Can't close command list with HR %x\n", hr);
               ID3D12CommandList *ppCommandLists[1] = { pD3D12Enc->m_spEncodeCommandList.Get() };
   pD3D12Enc->m_spEncodeCommandQueue->ExecuteCommandLists(1, ppCommandLists);
            // Validate device was not removed
   hr = pD3D12Enc->m_pD3D12Screen->dev->GetDeviceRemovedReason();
   if (hr != S_OK) {
      debug_printf("[d3d12_video_encoder] d3d12_video_encoder_flush" 
                                 pD3D12Enc->m_fenceValue++;
      }
         flush_fail:
      debug_printf("[d3d12_video_encoder] d3d12_video_encoder_flush failed for fenceValue: %" PRIu64 "\n", pD3D12Enc->m_fenceValue);
      }
      void
   d3d12_video_encoder_ensure_fence_finished(struct pipe_video_codec *codec, uint64_t fenceValueToWaitOn, uint64_t timeout_ns)
   {
         struct d3d12_video_encoder *pD3D12Enc = (struct d3d12_video_encoder *) codec;
   HRESULT hr = S_OK;
            debug_printf("[d3d12_video_encoder] d3d12_video_encoder_ensure_fence_finished - Waiting for fence (with timeout_ns %" PRIu64 ") to finish with "
                              HANDLE              event = { };
                  hr = pD3D12Enc->m_spFence->SetEventOnCompletion(fenceValueToWaitOn, event);
   if (FAILED(hr)) {
      debug_printf(
      "[d3d12_video_encoder] d3d12_video_encoder_ensure_fence_finished - SetEventOnCompletion for fenceValue %" PRIu64 " failed with HR %x\n",
                                 debug_printf("[d3d12_video_encoder] d3d12_video_encoder_ensure_fence_finished - Waiting on fence to be done with "
         "fenceValue: %" PRIu64 " - current CompletedValue: %" PRIu64 "\n",
      } else {
      debug_printf("[d3d12_video_encoder] d3d12_video_encoder_ensure_fence_finished - Fence already done with "
         "fenceValue: %" PRIu64 " - current CompletedValue: %" PRIu64 "\n",
      }
      ensure_fence_finished_fail:
      debug_printf("[d3d12_video_encoder] d3d12_video_encoder_sync_completion failed for fenceValue: %" PRIu64 "\n", fenceValueToWaitOn);
      }
      void
   d3d12_video_encoder_sync_completion(struct pipe_video_codec *codec, uint64_t fenceValueToWaitOn, uint64_t timeout_ns)
   {
         struct d3d12_video_encoder *pD3D12Enc = (struct d3d12_video_encoder *) codec;
   assert(pD3D12Enc);
   assert(pD3D12Enc->m_spD3D12VideoDevice);
   assert(pD3D12Enc->m_spEncodeCommandQueue);
                     debug_printf("[d3d12_video_encoder] d3d12_video_encoder_sync_completion - resetting ID3D12CommandAllocator %p...\n",
         hr = pD3D12Enc->m_inflightResourcesPool[fenceValueToWaitOn % D3D12_VIDEO_ENC_ASYNC_DEPTH].m_spCommandAllocator->Reset();
   if(FAILED(hr)) {
      debug_printf("failed with %x.\n", hr);
               // Release references granted on end_frame for this inflight operations
   pD3D12Enc->m_inflightResourcesPool[fenceValueToWaitOn % D3D12_VIDEO_ENC_ASYNC_DEPTH].m_spEncoder.Reset();
   pD3D12Enc->m_inflightResourcesPool[fenceValueToWaitOn % D3D12_VIDEO_ENC_ASYNC_DEPTH].m_spEncoderHeap.Reset();
   pD3D12Enc->m_inflightResourcesPool[fenceValueToWaitOn % D3D12_VIDEO_ENC_ASYNC_DEPTH].m_References.reset();
            // Validate device was not removed
   hr = pD3D12Enc->m_pD3D12Screen->dev->GetDeviceRemovedReason();
   if (hr != S_OK) {
      debug_printf("[d3d12_video_encoder] d3d12_video_encoder_sync_completion"
                                 debug_printf(
                     sync_with_token_fail:
      debug_printf("[d3d12_video_encoder] d3d12_video_encoder_sync_completion failed for fenceValue: %" PRIu64 "\n", fenceValueToWaitOn);
      }
      /**
   * Destroys a d3d12_video_encoder
   * Call destroy_XX for applicable XX nested member types before deallocating
   * Destroy methods should check != nullptr on their input target argument as this method can be called as part of
   * cleanup from failure on the creation method
   */
   void
   d3d12_video_encoder_destroy(struct pipe_video_codec *codec)
   {
      if (codec == nullptr) {
                                 if(pD3D12Enc->m_bPendingWorkNotFlushed){
      uint64_t curBatchFence = pD3D12Enc->m_fenceValue;
   d3d12_video_encoder_flush(codec);
               // Call d3d12_video_encoder dtor to make ComPtr and other member's destructors work
      }
      void
   d3d12_video_encoder_update_picparams_tracking(struct d3d12_video_encoder *pD3D12Enc,
               {
      D3D12_VIDEO_ENCODER_PICTURE_CONTROL_CODEC_DATA currentPicParams =
            enum pipe_video_format codec = u_reduce_video_profile(pD3D12Enc->base.profile);
   bool bUsedAsReference = false;
   switch (codec) {
      case PIPE_VIDEO_FORMAT_MPEG4_AVC:
   {
                  case PIPE_VIDEO_FORMAT_HEVC:
   {
         #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
         case PIPE_VIDEO_FORMAT_AV1:
   {
         #endif
         default:
   {
                        }
      bool
   d3d12_video_encoder_reconfigure_encoder_objects(struct d3d12_video_encoder *pD3D12Enc,
               {
      bool codecChanged =
         bool profileChanged =
         bool levelChanged =
         bool codecConfigChanged =
         bool inputFormatChanged =
         bool resolutionChanged =
         bool rateControlChanged =
         bool slicesChanged =
         bool gopChanged =
         bool motionPrecisionLimitChanged = ((pD3D12Enc->m_currentEncodeConfig.m_ConfigDirtyFlags &
            // Events that that trigger a re-creation of the reference picture manager
   // Stores codec agnostic textures so only input format, resolution and gop (num dpb references) affects this
   if (!pD3D12Enc->m_upDPBManager
      // || codecChanged
   // || profileChanged
   // || levelChanged
   // || codecConfigChanged
   || inputFormatChanged ||
   resolutionChanged
   // || rateControlChanged
   // || slicesChanged
   || gopChanged
      ) {
      if (!pD3D12Enc->m_upDPBManager) {
      debug_printf("[d3d12_video_encoder] d3d12_video_encoder_reconfigure_encoder_objects - Creating Reference "
      } else {
                  D3D12_RESOURCE_FLAGS resourceAllocFlags =
         bool     fArrayOfTextures = ((pD3D12Enc->m_currentEncodeCapabilities.m_SupportFlags &
         uint32_t texturePoolSize  = d3d12_video_encoder_get_current_max_dpb_capacity(pD3D12Enc) +
               assert(texturePoolSize < UINT16_MAX);
   pD3D12Enc->m_upDPBStorageManager.reset();
   if (fArrayOfTextures) {
      pD3D12Enc->m_upDPBStorageManager = std::make_unique<d3d12_array_of_textures_dpb_manager>(
      static_cast<uint16_t>(texturePoolSize),
   pD3D12Enc->m_pD3D12Screen->dev,
   pD3D12Enc->m_currentEncodeConfig.m_encodeFormatInfo.Format,
   pD3D12Enc->m_currentEncodeConfig.m_currentResolution,
   (D3D12_RESOURCE_FLAG_VIDEO_ENCODE_REFERENCE_ONLY | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE),
   true,   // setNullSubresourcesOnAllZero - D3D12 Video Encode expects nullptr pSubresources if AoT,
   pD3D12Enc->m_NodeMask,
   /*use underlying pool, we can't reuse upper level allocations, need D3D12_RESOURCE_FLAG_VIDEO_ENCODE_REFERENCE_ONLY*/
   } else {
      pD3D12Enc->m_upDPBStorageManager = std::make_unique<d3d12_texture_array_dpb_manager>(
      static_cast<uint16_t>(texturePoolSize),
   pD3D12Enc->m_pD3D12Screen->dev,
   pD3D12Enc->m_currentEncodeConfig.m_encodeFormatInfo.Format,
   pD3D12Enc->m_currentEncodeConfig.m_currentResolution,
   resourceAllocFlags,
   }
               bool reCreatedEncoder = false;
   // Events that that trigger a re-creation of the encoder
   if (!pD3D12Enc->m_spVideoEncoder || codecChanged ||
      profileChanged
   // || levelChanged // Only affects encoder heap
   || codecConfigChanged ||
   inputFormatChanged
   // || resolutionChanged // Only affects encoder heap
   // Only re-create if there is NO SUPPORT for reconfiguring rateControl on the fly
   || (rateControlChanged && ((pD3D12Enc->m_currentEncodeCapabilities.m_SupportFlags &
               // Only re-create if there is NO SUPPORT for reconfiguring slices on the fly
   || (slicesChanged && ((pD3D12Enc->m_currentEncodeCapabilities.m_SupportFlags &
               // Only re-create if there is NO SUPPORT for reconfiguring gop on the fly
   || (gopChanged && ((pD3D12Enc->m_currentEncodeCapabilities.m_SupportFlags &
               motionPrecisionLimitChanged) {
   if (!pD3D12Enc->m_spVideoEncoder) {
      debug_printf("[d3d12_video_encoder] d3d12_video_encoder_reconfigure_encoder_objects - Creating "
      } else {
      debug_printf("[d3d12_video_encoder] Reconfiguration triggered -> Re-creating D3D12VideoEncoder\n");
               D3D12_VIDEO_ENCODER_DESC encoderDesc = { pD3D12Enc->m_NodeMask,
                                          // Create encoder
   pD3D12Enc->m_spVideoEncoder.Reset();
   HRESULT hr = pD3D12Enc->m_spD3D12VideoDevice->CreateVideoEncoder(&encoderDesc,
         if (FAILED(hr)) {
      debug_printf("CreateVideoEncoder failed with HR %x\n", hr);
                  bool reCreatedEncoderHeap = false;
   // Events that that trigger a re-creation of the encoder heap
   if (!pD3D12Enc->m_spVideoEncoderHeap || codecChanged || profileChanged ||
      levelChanged
   // || codecConfigChanged // Only affects encoder
   || inputFormatChanged   // Might affect internal textures in the heap
   || resolutionChanged
   // Only re-create if there is NO SUPPORT for reconfiguring rateControl on the fly
   || (rateControlChanged && ((pD3D12Enc->m_currentEncodeCapabilities.m_SupportFlags &
               // Only re-create if there is NO SUPPORT for reconfiguring slices on the fly
   || (slicesChanged && ((pD3D12Enc->m_currentEncodeCapabilities.m_SupportFlags &
               // Only re-create if there is NO SUPPORT for reconfiguring gop on the fly
   || (gopChanged && ((pD3D12Enc->m_currentEncodeCapabilities.m_SupportFlags &
                  ) {
      if (!pD3D12Enc->m_spVideoEncoderHeap) {
      debug_printf("[d3d12_video_encoder] d3d12_video_encoder_reconfigure_encoder_objects - Creating "
      } else {
      debug_printf("[d3d12_video_encoder] Reconfiguration triggered -> Re-creating D3D12VideoEncoderHeap\n");
               D3D12_VIDEO_ENCODER_HEAP_DESC heapDesc = { pD3D12Enc->m_NodeMask,
                                                      // Create encoder heap
   pD3D12Enc->m_spVideoEncoderHeap.Reset();
   HRESULT hr = pD3D12Enc->m_spD3D12VideoDevice->CreateVideoEncoderHeap(&heapDesc,
         if (FAILED(hr)) {
      debug_printf("CreateVideoEncoderHeap failed with HR %x\n", hr);
                  // If on-the-fly reconfiguration happened without object recreation, set
   // D3D12_VIDEO_ENCODER_SEQUENCE_CONTROL_FLAG_*_CHANGED reconfiguration flags in EncodeFrame
   if (rateControlChanged &&
      ((pD3D12Enc->m_currentEncodeCapabilities.m_SupportFlags &
         0 /*checking if the flag it's actually set*/) &&
   (pD3D12Enc->m_fenceValue > 1) && (!reCreatedEncoder || !reCreatedEncoderHeap)) {
               if (slicesChanged &&
      ((pD3D12Enc->m_currentEncodeCapabilities.m_SupportFlags &
         0 /*checking if the flag it's actually set*/) &&
   (pD3D12Enc->m_fenceValue > 1) && (!reCreatedEncoder || !reCreatedEncoderHeap)) {
               if (gopChanged &&
      ((pD3D12Enc->m_currentEncodeCapabilities.m_SupportFlags &
         0 /*checking if the flag it's actually set*/) &&
   (pD3D12Enc->m_fenceValue > 1) && (!reCreatedEncoder || !reCreatedEncoderHeap)) {
      }
      }
      void
   d3d12_video_encoder_create_reference_picture_manager(struct d3d12_video_encoder *pD3D12Enc)
   {
      pD3D12Enc->m_upDPBManager.reset();
   pD3D12Enc->m_upBitstreamBuilder.reset();
   enum pipe_video_format codec = u_reduce_video_profile(pD3D12Enc->base.profile);
   switch (codec) {
      case PIPE_VIDEO_FORMAT_MPEG4_AVC:
   {
      bool gopHasPFrames =
      (pD3D12Enc->m_currentEncodeConfig.m_encoderGOPConfigDesc.m_H264GroupOfPictures.PPicturePeriod > 0) &&
   ((pD3D12Enc->m_currentEncodeConfig.m_encoderGOPConfigDesc.m_H264GroupOfPictures.GOPLength == 0) ||
               pD3D12Enc->m_upDPBManager = std::make_unique<d3d12_video_encoder_references_manager_h264>(
      gopHasPFrames,
   *pD3D12Enc->m_upDPBStorageManager,
   // Max number of frames to be used as a reference, without counting the current recon picture
                           case PIPE_VIDEO_FORMAT_HEVC:
   {
      bool gopHasPFrames =
      (pD3D12Enc->m_currentEncodeConfig.m_encoderGOPConfigDesc.m_HEVCGroupOfPictures.PPicturePeriod > 0) &&
   ((pD3D12Enc->m_currentEncodeConfig.m_encoderGOPConfigDesc.m_HEVCGroupOfPictures.GOPLength == 0) ||
               pD3D12Enc->m_upDPBManager = std::make_unique<d3d12_video_encoder_references_manager_hevc>(
      gopHasPFrames,
   *pD3D12Enc->m_upDPBStorageManager,
   // Max number of frames to be used as a reference, without counting the current recon picture
                  #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
         case PIPE_VIDEO_FORMAT_AV1:
   {
      bool hasInterFrames =
      (pD3D12Enc->m_currentEncodeConfig.m_encoderGOPConfigDesc.m_AV1SequenceStructure.InterFramePeriod > 0) &&
   ((pD3D12Enc->m_currentEncodeConfig.m_encoderGOPConfigDesc.m_AV1SequenceStructure.IntraDistance == 0) ||
               pD3D12Enc->m_upDPBManager = std::make_unique<d3d12_video_encoder_references_manager_av1>(
      hasInterFrames,
               // We use packed headers and pist encode execution syntax for AV1
      #endif
         default:
   {
               }
      D3D12_VIDEO_ENCODER_PICTURE_CONTROL_SUBREGIONS_LAYOUT_DATA
   d3d12_video_encoder_get_current_slice_param_settings(struct d3d12_video_encoder *pD3D12Enc)
   {
      enum pipe_video_format codec = u_reduce_video_profile(pD3D12Enc->base.profile);
   switch (codec) {
      case PIPE_VIDEO_FORMAT_MPEG4_AVC:
   {
      D3D12_VIDEO_ENCODER_PICTURE_CONTROL_SUBREGIONS_LAYOUT_DATA subregionData = {};
   if (pD3D12Enc->m_currentEncodeConfig.m_encoderSliceConfigMode !=
      D3D12_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE_FULL_FRAME) {
   subregionData.pSlicesPartition_H264 =
            }
               case PIPE_VIDEO_FORMAT_HEVC:
   {
      D3D12_VIDEO_ENCODER_PICTURE_CONTROL_SUBREGIONS_LAYOUT_DATA subregionData = {};
   if (pD3D12Enc->m_currentEncodeConfig.m_encoderSliceConfigMode !=
      D3D12_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE_FULL_FRAME) {
   subregionData.pSlicesPartition_HEVC =
            }
      #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
         case PIPE_VIDEO_FORMAT_AV1:
   {
      D3D12_VIDEO_ENCODER_PICTURE_CONTROL_SUBREGIONS_LAYOUT_DATA subregionData = {};
   if (pD3D12Enc->m_currentEncodeConfig.m_encoderSliceConfigMode !=
      D3D12_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE_FULL_FRAME) {
   subregionData.pTilesPartition_AV1 =
            }
      #endif
         default:
   {
               }
      D3D12_VIDEO_ENCODER_PICTURE_CONTROL_CODEC_DATA
   d3d12_video_encoder_get_current_picture_param_settings(struct d3d12_video_encoder *pD3D12Enc)
   {
      enum pipe_video_format codec = u_reduce_video_profile(pD3D12Enc->base.profile);
   switch (codec) {
      case PIPE_VIDEO_FORMAT_MPEG4_AVC:
   {
      D3D12_VIDEO_ENCODER_PICTURE_CONTROL_CODEC_DATA curPicParamsData = {};
   curPicParamsData.pH264PicData = &pD3D12Enc->m_currentEncodeConfig.m_encoderPicParamsDesc.m_H264PicData;
   curPicParamsData.DataSize     = sizeof(pD3D12Enc->m_currentEncodeConfig.m_encoderPicParamsDesc.m_H264PicData);
               case PIPE_VIDEO_FORMAT_HEVC:
   {
      D3D12_VIDEO_ENCODER_PICTURE_CONTROL_CODEC_DATA curPicParamsData = {};
   curPicParamsData.pHEVCPicData = &pD3D12Enc->m_currentEncodeConfig.m_encoderPicParamsDesc.m_HEVCPicData;
   curPicParamsData.DataSize     = sizeof(pD3D12Enc->m_currentEncodeConfig.m_encoderPicParamsDesc.m_HEVCPicData);
      #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
         case PIPE_VIDEO_FORMAT_AV1:
   {
      D3D12_VIDEO_ENCODER_PICTURE_CONTROL_CODEC_DATA curPicParamsData = {};
   curPicParamsData.pAV1PicData = &pD3D12Enc->m_currentEncodeConfig.m_encoderPicParamsDesc.m_AV1PicData;
   curPicParamsData.DataSize     = sizeof(pD3D12Enc->m_currentEncodeConfig.m_encoderPicParamsDesc.m_AV1PicData);
      #endif
         default:
   {
               }
      D3D12_VIDEO_ENCODER_RATE_CONTROL
   d3d12_video_encoder_get_current_rate_control_settings(struct d3d12_video_encoder *pD3D12Enc)
   {
      D3D12_VIDEO_ENCODER_RATE_CONTROL curRateControlDesc = {};
   curRateControlDesc.Mode            = pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Mode;
   curRateControlDesc.Flags           = pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags;
         #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
      if ((curRateControlDesc.Flags & D3D12_VIDEO_ENCODER_RATE_CONTROL_FLAG_ENABLE_EXTENSION1_SUPPORT) != 0)
   {
      switch (pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Mode) {
      case D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_ABSOLUTE_QP_MAP:
   {
      curRateControlDesc.ConfigParams.pConfiguration_CQP1 = nullptr;
      } break;
   case D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_CQP:
   {
      curRateControlDesc.ConfigParams.pConfiguration_CQP1 =
         curRateControlDesc.ConfigParams.DataSize =
      } break;
   case D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_CBR:
   {
      curRateControlDesc.ConfigParams.pConfiguration_CBR1 =
         curRateControlDesc.ConfigParams.DataSize =
      } break;
   case D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_VBR:
   {
      curRateControlDesc.ConfigParams.pConfiguration_VBR1 =
         curRateControlDesc.ConfigParams.DataSize =
      } break;
   case D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_QVBR:
   {
      curRateControlDesc.ConfigParams.pConfiguration_QVBR1 =
         curRateControlDesc.ConfigParams.DataSize =
      } break;
   default:
   {
               }
      #endif
      {
      switch (pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Mode) {
      case D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_ABSOLUTE_QP_MAP:
   {
      curRateControlDesc.ConfigParams.pConfiguration_CQP = nullptr;
      } break;
   case D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_CQP:
   {
      curRateControlDesc.ConfigParams.pConfiguration_CQP =
         curRateControlDesc.ConfigParams.DataSize =
      } break;
   case D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_CBR:
   {
      curRateControlDesc.ConfigParams.pConfiguration_CBR =
         curRateControlDesc.ConfigParams.DataSize =
      } break;
   case D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_VBR:
   {
      curRateControlDesc.ConfigParams.pConfiguration_VBR =
         curRateControlDesc.ConfigParams.DataSize =
      } break;
   case D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_QVBR:
   {
      curRateControlDesc.ConfigParams.pConfiguration_QVBR =
         curRateControlDesc.ConfigParams.DataSize =
      } break;
   default:
   {
                           }
      D3D12_VIDEO_ENCODER_LEVEL_SETTING
   d3d12_video_encoder_get_current_level_desc(struct d3d12_video_encoder *pD3D12Enc)
   {
      enum pipe_video_format codec = u_reduce_video_profile(pD3D12Enc->base.profile);
   switch (codec) {
      case PIPE_VIDEO_FORMAT_MPEG4_AVC:
   {
      D3D12_VIDEO_ENCODER_LEVEL_SETTING curLevelDesc = {};
   curLevelDesc.pH264LevelSetting = &pD3D12Enc->m_currentEncodeConfig.m_encoderLevelDesc.m_H264LevelSetting;
   curLevelDesc.DataSize = sizeof(pD3D12Enc->m_currentEncodeConfig.m_encoderLevelDesc.m_H264LevelSetting);
               case PIPE_VIDEO_FORMAT_HEVC:
   {
      D3D12_VIDEO_ENCODER_LEVEL_SETTING curLevelDesc = {};
   curLevelDesc.pHEVCLevelSetting = &pD3D12Enc->m_currentEncodeConfig.m_encoderLevelDesc.m_HEVCLevelSetting;
   curLevelDesc.DataSize = sizeof(pD3D12Enc->m_currentEncodeConfig.m_encoderLevelDesc.m_HEVCLevelSetting);
      #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
         case PIPE_VIDEO_FORMAT_AV1:
   {
      D3D12_VIDEO_ENCODER_LEVEL_SETTING curLevelDesc = {};
   curLevelDesc.pAV1LevelSetting = &pD3D12Enc->m_currentEncodeConfig.m_encoderLevelDesc.m_AV1LevelSetting;
   curLevelDesc.DataSize = sizeof(pD3D12Enc->m_currentEncodeConfig.m_encoderLevelDesc.m_AV1LevelSetting);
      #endif
         default:
   {
               }
      void
   d3d12_video_encoder_build_pre_encode_codec_headers(struct d3d12_video_encoder *pD3D12Enc,
               {
      enum pipe_video_format codec = u_reduce_video_profile(pD3D12Enc->base.profile);
   switch (codec) {
      case PIPE_VIDEO_FORMAT_MPEG4_AVC:
   {
      postEncodeHeadersNeeded = false;
               case PIPE_VIDEO_FORMAT_HEVC:
   {
      postEncodeHeadersNeeded = false;
               case PIPE_VIDEO_FORMAT_AV1:
   { 
      pD3D12Enc->m_BitstreamHeadersBuffer.resize(0);
   postEncodeHeadersNeeded = true;
               default:
   {
               }
      D3D12_VIDEO_ENCODER_SEQUENCE_GOP_STRUCTURE
   d3d12_video_encoder_get_current_gop_desc(struct d3d12_video_encoder *pD3D12Enc)
   {
      enum pipe_video_format codec = u_reduce_video_profile(pD3D12Enc->base.profile);
   switch (codec) {
      case PIPE_VIDEO_FORMAT_MPEG4_AVC:
   {
      D3D12_VIDEO_ENCODER_SEQUENCE_GOP_STRUCTURE curGOPDesc = {};
   curGOPDesc.pH264GroupOfPictures =
         curGOPDesc.DataSize = sizeof(pD3D12Enc->m_currentEncodeConfig.m_encoderGOPConfigDesc.m_H264GroupOfPictures);
               case PIPE_VIDEO_FORMAT_HEVC:
   {
      D3D12_VIDEO_ENCODER_SEQUENCE_GOP_STRUCTURE curGOPDesc = {};
   curGOPDesc.pHEVCGroupOfPictures =
         curGOPDesc.DataSize = sizeof(pD3D12Enc->m_currentEncodeConfig.m_encoderGOPConfigDesc.m_HEVCGroupOfPictures);
         #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
         case PIPE_VIDEO_FORMAT_AV1:
   {
      D3D12_VIDEO_ENCODER_SEQUENCE_GOP_STRUCTURE curGOPDesc = {};
   curGOPDesc.pAV1SequenceStructure =
         curGOPDesc.DataSize = sizeof(pD3D12Enc->m_currentEncodeConfig.m_encoderGOPConfigDesc.m_AV1SequenceStructure);
      #endif
         default:
   {
               }
      D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION
   d3d12_video_encoder_get_current_codec_config_desc(struct d3d12_video_encoder *pD3D12Enc)
   {
      enum pipe_video_format codec = u_reduce_video_profile(pD3D12Enc->base.profile);
   switch (codec) {
      case PIPE_VIDEO_FORMAT_MPEG4_AVC:
   {
      D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION codecConfigDesc = {};
   codecConfigDesc.pH264Config = &pD3D12Enc->m_currentEncodeConfig.m_encoderCodecSpecificConfigDesc.m_H264Config;
   codecConfigDesc.DataSize =
                     case PIPE_VIDEO_FORMAT_HEVC:
   {
      D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION codecConfigDesc = {};
   codecConfigDesc.pHEVCConfig = &pD3D12Enc->m_currentEncodeConfig.m_encoderCodecSpecificConfigDesc.m_HEVCConfig;
   codecConfigDesc.DataSize =
               #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
         case PIPE_VIDEO_FORMAT_AV1:
   {
      D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION codecConfigDesc = {};
   codecConfigDesc.pAV1Config = &pD3D12Enc->m_currentEncodeConfig.m_encoderCodecSpecificConfigDesc.m_AV1Config;
   codecConfigDesc.DataSize =
            #endif
         default:
   {
               }
      D3D12_VIDEO_ENCODER_CODEC
   d3d12_video_encoder_get_current_codec(struct d3d12_video_encoder *pD3D12Enc)
   {
      enum pipe_video_format codec = u_reduce_video_profile(pD3D12Enc->base.profile);
   switch (codec) {
      case PIPE_VIDEO_FORMAT_MPEG4_AVC:
   {
         } break;
   case PIPE_VIDEO_FORMAT_HEVC:
   {
         #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
         case PIPE_VIDEO_FORMAT_AV1:
   {
         #endif
         default:
   {
               }
      static void
   d3d12_video_encoder_disable_rc_vbv_sizes(struct D3D12EncodeRateControlState & rcState)
   {
      rcState.m_Flags &= ~D3D12_VIDEO_ENCODER_RATE_CONTROL_FLAG_ENABLE_VBV_SIZES;
   switch (rcState.m_Mode) {
      case D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_CBR:
   {
      rcState.m_Config.m_Configuration_CBR.VBVCapacity = 0;
      } break;
   case D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_VBR:
   {
      rcState.m_Config.m_Configuration_VBR.VBVCapacity = 0;
      #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
         case D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_QVBR:
   {
      rcState.m_Config.m_Configuration_QVBR1.VBVCapacity = 0;
      #endif
         default:
   {
               }
      static void
   d3d12_video_encoder_disable_rc_maxframesize(struct D3D12EncodeRateControlState & rcState)
   {
      rcState.m_Flags &= ~D3D12_VIDEO_ENCODER_RATE_CONTROL_FLAG_ENABLE_MAX_FRAME_SIZE;
   switch (rcState.m_Mode) {
      case D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_CBR:
   {
         } break;
   case D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_VBR:
   {
         } break;
   case D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_QVBR:
   {
         } break;
   default:
   {
               }
      #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
   static bool
   d3d12_video_encoder_is_qualitylevel_in_range(struct D3D12EncodeRateControlState & rcState, UINT MaxQualityVsSpeed)
   {
      switch (rcState.m_Mode) {
      case D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_CQP:
   {
         } break;
   case D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_CBR:
   {
         } break;
   case D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_VBR:
   {
         } break;
   case D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_QVBR:
   {
         } break;
   default:
   {
               }
      static void
   d3d12_video_encoder_disable_rc_qualitylevels(struct D3D12EncodeRateControlState & rcState)
   {
      rcState.m_Flags &= ~D3D12_VIDEO_ENCODER_RATE_CONTROL_FLAG_ENABLE_QUALITY_VS_SPEED;
   switch (rcState.m_Mode) {
      case D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_CBR:
   {
         } break;
   case D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_VBR:
   {
         } break;
   case D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_QVBR:
   {
         } break;
   default:
   {
               }
   #endif
      static void
   d3d12_video_encoder_disable_rc_minmaxqp(struct D3D12EncodeRateControlState & rcState)
   {
      rcState.m_Flags &= ~D3D12_VIDEO_ENCODER_RATE_CONTROL_FLAG_ENABLE_QP_RANGE;
   switch (rcState.m_Mode) {
      case D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_CBR:
   {
      rcState.m_Config.m_Configuration_CBR.MinQP = 0;
      } break;
   case D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_VBR:
   {
      rcState.m_Config.m_Configuration_VBR.MinQP = 0;
      } break;
   case D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_QVBR:
   {
      rcState.m_Config.m_Configuration_QVBR.MinQP = 0;
      } break;
   default:
   {
               }
      #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
   static void
   d3d12_video_encoder_disable_rc_extended1_to_legacy(struct D3D12EncodeRateControlState & rcState)
   {
      rcState.m_Flags &= ~D3D12_VIDEO_ENCODER_RATE_CONTROL_FLAG_ENABLE_EXTENSION1_SUPPORT;
   // Also remove features that require extension1 enabled (eg. quality levels)
   rcState.m_Flags &= ~D3D12_VIDEO_ENCODER_RATE_CONTROL_FLAG_ENABLE_QUALITY_VS_SPEED;
   // rcState.m_Configuration_XXX and m_Configuration_XXX1 are unions, can be aliased
      }
   #endif
      ///
   /// Call d3d12_video_encoder_query_d3d12_driver_caps and see if any optional feature requested
   /// is not supported, disable it, query again until finding a negotiated cap/feature set
   /// Note that with fallbacks, the upper layer will not get exactly the encoding seetings they requested
   /// but for very particular settings it's better to continue with warnings than failing the whole encoding process
   ///
   bool d3d12_video_encoder_negotiate_requested_features_and_d3d12_driver_caps(struct d3d12_video_encoder *pD3D12Enc, D3D12_FEATURE_DATA_VIDEO_ENCODER_SUPPORT1 &capEncoderSupportData1) {
         ///
   /// Check for general support
   /// Check for validation errors (some drivers return general support but also validation errors anyways, work around for those unexpected cases)
            bool configSupported = d3d12_video_encoder_query_d3d12_driver_caps(pD3D12Enc, /*inout*/ capEncoderSupportData1)
   && (((capEncoderSupportData1.SupportFlags & D3D12_VIDEO_ENCODER_SUPPORT_FLAG_GENERAL_SUPPORT_OK) != 0)
            ///
   /// If D3D12_FEATURE_VIDEO_ENCODER_SUPPORT is not supported, try falling back to unsetting optional features and check for caps again
            if (!configSupported) {
               bool isRequestingVBVSizesSupported = ((capEncoderSupportData1.SupportFlags & D3D12_VIDEO_ENCODER_SUPPORT_FLAG_RATE_CONTROL_VBV_SIZE_CONFIG_AVAILABLE) != 0);
   bool isClientRequestingVBVSizes = ((pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags & D3D12_VIDEO_ENCODER_RATE_CONTROL_FLAG_ENABLE_VBV_SIZES) != 0);
      if(isClientRequestingVBVSizes && !isRequestingVBVSizesSupported) {
      debug_printf("[d3d12_video_encoder] WARNING: Requested D3D12_VIDEO_ENCODER_RATE_CONTROL_FLAG_ENABLE_VBV_SIZES with VBVCapacity and InitialVBVFullness is not supported, will continue encoding unsetting this feature as fallback.\n");
               bool isRequestingPeakFrameSizeSupported = ((capEncoderSupportData1.SupportFlags & D3D12_VIDEO_ENCODER_SUPPORT_FLAG_RATE_CONTROL_MAX_FRAME_SIZE_AVAILABLE) != 0);
            if(isClientRequestingPeakFrameSize && !isRequestingPeakFrameSizeSupported) {
      debug_printf("[d3d12_video_encoder] WARNING: Requested D3D12_VIDEO_ENCODER_RATE_CONTROL_FLAG_ENABLE_MAX_FRAME_SIZE with MaxFrameBitSize but the feature is not supported, will continue encoding unsetting this feature as fallback.\n");
               bool isRequestingQPRangesSupported = ((capEncoderSupportData1.SupportFlags & D3D12_VIDEO_ENCODER_SUPPORT_FLAG_RATE_CONTROL_ADJUSTABLE_QP_RANGE_AVAILABLE) != 0);
            if(isClientRequestingQPRanges && !isRequestingQPRangesSupported) {
      debug_printf("[d3d12_video_encoder] WARNING: Requested D3D12_VIDEO_ENCODER_RATE_CONTROL_FLAG_ENABLE_QP_RANGE with QPMin QPMax but the feature is not supported, will continue encoding unsetting this feature as fallback.\n");
         #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
         bool isRequestingExtended1RCSupported = ((capEncoderSupportData1.SupportFlags & D3D12_VIDEO_ENCODER_SUPPORT_FLAG_RATE_CONTROL_EXTENSION1_SUPPORT) != 0);
            if(isClientRequestingExtended1RC && !isRequestingExtended1RCSupported) {
      debug_printf("[d3d12_video_encoder] WARNING: Requested D3D12_VIDEO_ENCODER_RATE_CONTROL_FLAG_ENABLE_EXTENSION1_SUPPORT but the feature is not supported, will continue encoding unsetting this feature and dependent features as fallback.\n");
               /* d3d12_video_encoder_disable_rc_extended1_to_legacy may change m_Flags */
   if ((pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc.m_Flags & D3D12_VIDEO_ENCODER_RATE_CONTROL_FLAG_ENABLE_EXTENSION1_SUPPORT) != 0)
   { // Quality levels also requires D3D12_VIDEO_ENCODER_RATE_CONTROL_FLAG_ENABLE_EXTENSION1_SUPPORT
                     if (isClientRequestingQualityLevels)
   {
      if (!isRequestingQualityLevelsSupported) {
      debug_printf("[d3d12_video_encoder] WARNING: Requested D3D12_VIDEO_ENCODER_RATE_CONTROL_FLAG_ENABLE_QUALITY_VS_SPEED but the feature is not supported, will continue encoding unsetting this feature as fallback.\n");
      } else if (!d3d12_video_encoder_is_qualitylevel_in_range(pD3D12Enc->m_currentEncodeConfig.m_encoderRateControlDesc, capEncoderSupportData1.MaxQualityVsSpeed)) {
      debug_printf("[d3d12_video_encoder] WARNING: Requested D3D12_VIDEO_ENCODER_RATE_CONTROL_FLAG_ENABLE_QUALITY_VS_SPEED but the value is out of supported range, will continue encoding unsetting this feature as fallback.\n");
            #endif
            ///
   /// Try fallback configuration
   ///
   configSupported = d3d12_video_encoder_query_d3d12_driver_caps(pD3D12Enc, /*inout*/ capEncoderSupportData1)
      && (((capEncoderSupportData1.SupportFlags & D3D12_VIDEO_ENCODER_SUPPORT_FLAG_GENERAL_SUPPORT_OK) != 0)
            if(!configSupported) {
      debug_printf("[d3d12_video_encoder] Cap negotiation failed, see more details below:\n");
      if ((capEncoderSupportData1.ValidationFlags & D3D12_VIDEO_ENCODER_VALIDATION_FLAG_CODEC_NOT_SUPPORTED) != 0) {
                  if ((capEncoderSupportData1.ValidationFlags &
      D3D12_VIDEO_ENCODER_VALIDATION_FLAG_RESOLUTION_NOT_SUPPORTED_IN_LIST) != 0) {
               if ((capEncoderSupportData1.ValidationFlags &
      D3D12_VIDEO_ENCODER_VALIDATION_FLAG_RATE_CONTROL_CONFIGURATION_NOT_SUPPORTED) != 0) {
               if ((capEncoderSupportData1.ValidationFlags &
      D3D12_VIDEO_ENCODER_VALIDATION_FLAG_CODEC_CONFIGURATION_NOT_SUPPORTED) != 0) {
               if ((capEncoderSupportData1.ValidationFlags &
      D3D12_VIDEO_ENCODER_VALIDATION_FLAG_RATE_CONTROL_MODE_NOT_SUPPORTED) != 0) {
               if ((capEncoderSupportData1.ValidationFlags &
      D3D12_VIDEO_ENCODER_VALIDATION_FLAG_INTRA_REFRESH_MODE_NOT_SUPPORTED) != 0) {
               if ((capEncoderSupportData1.ValidationFlags &
      D3D12_VIDEO_ENCODER_VALIDATION_FLAG_SUBREGION_LAYOUT_MODE_NOT_SUPPORTED) != 0) {
               if ((capEncoderSupportData1.ValidationFlags & D3D12_VIDEO_ENCODER_VALIDATION_FLAG_INPUT_FORMAT_NOT_SUPPORTED) !=
      0) {
                     }
      bool d3d12_video_encoder_query_d3d12_driver_caps(struct d3d12_video_encoder *pD3D12Enc, D3D12_FEATURE_DATA_VIDEO_ENCODER_SUPPORT1 &capEncoderSupportData1) {
      capEncoderSupportData1.NodeIndex                                = pD3D12Enc->m_NodeIndex;
   capEncoderSupportData1.Codec                                    = d3d12_video_encoder_get_current_codec(pD3D12Enc);
   capEncoderSupportData1.InputFormat            = pD3D12Enc->m_currentEncodeConfig.m_encodeFormatInfo.Format;
   capEncoderSupportData1.RateControl            = d3d12_video_encoder_get_current_rate_control_settings(pD3D12Enc);
   capEncoderSupportData1.IntraRefresh           = pD3D12Enc->m_currentEncodeConfig.m_IntraRefresh.Mode;
   capEncoderSupportData1.SubregionFrameEncoding = pD3D12Enc->m_currentEncodeConfig.m_encoderSliceConfigMode;
   capEncoderSupportData1.ResolutionsListCount   = 1;
   capEncoderSupportData1.pResolutionList        = &pD3D12Enc->m_currentEncodeConfig.m_currentResolution;
   capEncoderSupportData1.CodecGopSequence       = d3d12_video_encoder_get_current_gop_desc(pD3D12Enc);
   capEncoderSupportData1.MaxReferenceFramesInDPB = d3d12_video_encoder_get_current_max_dpb_capacity(pD3D12Enc);
            enum pipe_video_format codec = u_reduce_video_profile(pD3D12Enc->base.profile);
   switch (codec) {
      case PIPE_VIDEO_FORMAT_MPEG4_AVC:
   {
      capEncoderSupportData1.SuggestedProfile.pH264Profile =
         capEncoderSupportData1.SuggestedProfile.DataSize =
         capEncoderSupportData1.SuggestedLevel.pH264LevelSetting =
         capEncoderSupportData1.SuggestedLevel.DataSize =
               case PIPE_VIDEO_FORMAT_HEVC:
   {
      capEncoderSupportData1.SuggestedProfile.pHEVCProfile =
         capEncoderSupportData1.SuggestedProfile.DataSize =
         capEncoderSupportData1.SuggestedLevel.pHEVCLevelSetting =
         capEncoderSupportData1.SuggestedLevel.DataSize =
         #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
         case PIPE_VIDEO_FORMAT_AV1:
   {
      capEncoderSupportData1.SuggestedProfile.pAV1Profile =
         capEncoderSupportData1.SuggestedProfile.DataSize =
         capEncoderSupportData1.SuggestedLevel.pAV1LevelSetting =
         capEncoderSupportData1.SuggestedLevel.DataSize =
      #endif
         default:
   {
                     // prepare inout storage for the resolution dependent result.
   capEncoderSupportData1.pResolutionDependentSupport =
            #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
         #endif
      HRESULT hr = pD3D12Enc->m_spD3D12VideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_SUPPORT1,
               if (FAILED(hr)) {
      debug_printf("CheckFeatureSupport D3D12_FEATURE_VIDEO_ENCODER_SUPPORT1 failed with HR %x\n", hr);
            // D3D12_FEATURE_DATA_VIDEO_ENCODER_SUPPORT1 extends D3D12_FEATURE_DATA_VIDEO_ENCODER_SUPPORT
   // in a binary compatible way, so just cast it and try with the older query D3D12_FEATURE_VIDEO_ENCODER_SUPPORT
   D3D12_FEATURE_DATA_VIDEO_ENCODER_SUPPORT * casted_down_cap_data = reinterpret_cast<D3D12_FEATURE_DATA_VIDEO_ENCODER_SUPPORT*>(&capEncoderSupportData1);
   hr = pD3D12Enc->m_spD3D12VideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_SUPPORT,
               if (FAILED(hr)) {
      debug_printf("CheckFeatureSupport D3D12_FEATURE_VIDEO_ENCODER_SUPPORT failed with HR %x\n", hr);
         }
   pD3D12Enc->m_currentEncodeCapabilities.m_SupportFlags    = capEncoderSupportData1.SupportFlags;
   pD3D12Enc->m_currentEncodeCapabilities.m_ValidationFlags = capEncoderSupportData1.ValidationFlags;
      }
      bool d3d12_video_encoder_check_subregion_mode_support(struct d3d12_video_encoder *pD3D12Enc,
               {
      D3D12_FEATURE_DATA_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE capDataSubregionLayout = { }; 
   capDataSubregionLayout.NodeIndex = pD3D12Enc->m_NodeIndex;
   capDataSubregionLayout.Codec = d3d12_video_encoder_get_current_codec(pD3D12Enc);
   capDataSubregionLayout.Profile = d3d12_video_encoder_get_current_profile_desc(pD3D12Enc);
   capDataSubregionLayout.Level = d3d12_video_encoder_get_current_level_desc(pD3D12Enc);
   capDataSubregionLayout.SubregionMode = requestedSlicesMode;
   HRESULT hr = pD3D12Enc->m_spD3D12VideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE, &capDataSubregionLayout, sizeof(capDataSubregionLayout));
   if (FAILED(hr)) {
      debug_printf("CheckFeatureSupport failed with HR %x\n", hr);
      }
      }
      D3D12_VIDEO_ENCODER_PROFILE_DESC
   d3d12_video_encoder_get_current_profile_desc(struct d3d12_video_encoder *pD3D12Enc)
   {
      enum pipe_video_format codec = u_reduce_video_profile(pD3D12Enc->base.profile);
   switch (codec) {
      case PIPE_VIDEO_FORMAT_MPEG4_AVC:
   {
      D3D12_VIDEO_ENCODER_PROFILE_DESC curProfDesc = {};
   curProfDesc.pH264Profile = &pD3D12Enc->m_currentEncodeConfig.m_encoderProfileDesc.m_H264Profile;
   curProfDesc.DataSize     = sizeof(pD3D12Enc->m_currentEncodeConfig.m_encoderProfileDesc.m_H264Profile);
               case PIPE_VIDEO_FORMAT_HEVC:
   {
      D3D12_VIDEO_ENCODER_PROFILE_DESC curProfDesc = {};
   curProfDesc.pHEVCProfile = &pD3D12Enc->m_currentEncodeConfig.m_encoderProfileDesc.m_HEVCProfile;
   curProfDesc.DataSize     = sizeof(pD3D12Enc->m_currentEncodeConfig.m_encoderProfileDesc.m_HEVCProfile);
      #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
         case PIPE_VIDEO_FORMAT_AV1:
   {
      D3D12_VIDEO_ENCODER_PROFILE_DESC curProfDesc = {};
   curProfDesc.pAV1Profile = &pD3D12Enc->m_currentEncodeConfig.m_encoderProfileDesc.m_AV1Profile;
   curProfDesc.DataSize     = sizeof(pD3D12Enc->m_currentEncodeConfig.m_encoderProfileDesc.m_AV1Profile);
      #endif
         default:
   {
               }
      uint32_t
   d3d12_video_encoder_get_current_max_dpb_capacity(struct d3d12_video_encoder *pD3D12Enc)
   {
         }
      bool
   d3d12_video_encoder_update_current_encoder_config_state(struct d3d12_video_encoder *pD3D12Enc,
               {
      enum pipe_video_format codec = u_reduce_video_profile(pD3D12Enc->base.profile);
   switch (codec) {
      case PIPE_VIDEO_FORMAT_MPEG4_AVC:
   {
                  case PIPE_VIDEO_FORMAT_HEVC:
   {
         #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
         case PIPE_VIDEO_FORMAT_AV1:
   {
         #endif
         default:
   {
               }
      bool
   d3d12_video_encoder_create_command_objects(struct d3d12_video_encoder *pD3D12Enc)
   {
               D3D12_COMMAND_QUEUE_DESC commandQueueDesc = { D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE };
   HRESULT                  hr               = pD3D12Enc->m_pD3D12Screen->dev->CreateCommandQueue(
      &commandQueueDesc,
      if (FAILED(hr)) {
      debug_printf("[d3d12_video_encoder] d3d12_video_encoder_create_command_objects - Call to CreateCommandQueue "
                           hr = pD3D12Enc->m_pD3D12Screen->dev->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pD3D12Enc->m_spFence));
   if (FAILED(hr)) {
      debug_printf(
      "[d3d12_video_encoder] d3d12_video_encoder_create_command_objects - Call to CreateFence failed with HR %x\n",
                  for (auto& inputResource : pD3D12Enc->m_inflightResourcesPool)
   {
      // Create associated command allocator for Encode, Resolve operations
   hr = pD3D12Enc->m_pD3D12Screen->dev->CreateCommandAllocator(
      D3D12_COMMAND_LIST_TYPE_VIDEO_ENCODE,
      if (FAILED(hr)) {
      debug_printf("[d3d12_video_encoder] d3d12_video_encoder_create_command_objects - Call to "
                              ComPtr<ID3D12Device4> spD3D12Device4;
   if (FAILED(pD3D12Enc->m_pD3D12Screen->dev->QueryInterface(
            debug_printf(
                     hr = spD3D12Device4->CreateCommandList1(0,
                        if (FAILED(hr)) {
      debug_printf("[d3d12_video_encoder] d3d12_video_encoder_create_command_objects - Call to CreateCommandList "
                              }
      struct pipe_video_codec *
   d3d12_video_encoder_create_encoder(struct pipe_context *context, const struct pipe_video_codec *codec)
   {
      ///
   /// Initialize d3d12_video_encoder
            // Not using new doesn't call ctor and the initializations in the class declaration are lost
            pD3D12Enc->m_spEncodedFrameMetadata.resize(D3D12_VIDEO_ENC_METADATA_BUFFERS_COUNT, {nullptr, 0, 0});
            pD3D12Enc->base         = *codec;
   pD3D12Enc->m_screen     = context->screen;
   pD3D12Enc->base.context = context;
   pD3D12Enc->base.width   = codec->width;
   pD3D12Enc->base.height  = codec->height;
   pD3D12Enc->base.max_references  = codec->max_references;
   // Only fill methods that are supported by the d3d12 encoder, leaving null the rest (ie. encode_* / encode_macroblock)
   pD3D12Enc->base.destroy          = d3d12_video_encoder_destroy;
   pD3D12Enc->base.begin_frame      = d3d12_video_encoder_begin_frame;
   pD3D12Enc->base.encode_bitstream = d3d12_video_encoder_encode_bitstream;
   pD3D12Enc->base.end_frame        = d3d12_video_encoder_end_frame;
   pD3D12Enc->base.flush            = d3d12_video_encoder_flush;
            struct d3d12_context *pD3D12Ctx = (struct d3d12_context *) context;
            if (FAILED(pD3D12Enc->m_pD3D12Screen->dev->QueryInterface(
            debug_printf(
                     if (!d3d12_video_encoder_create_command_objects(pD3D12Enc)) {
      debug_printf("[d3d12_video_encoder] d3d12_video_encoder_create_encoder - Failure on "
                     // Cache quality levels cap
   pD3D12Enc->max_quality_levels = context->screen->get_video_param(context->screen, codec->profile,
                        failed:
      if (pD3D12Enc != nullptr) {
                     }
      bool
   d3d12_video_encoder_prepare_output_buffers(struct d3d12_video_encoder *pD3D12Enc,
               {
      pD3D12Enc->m_currentEncodeCapabilities.m_ResourceRequirementsCaps.NodeIndex = pD3D12Enc->m_NodeIndex;
   pD3D12Enc->m_currentEncodeCapabilities.m_ResourceRequirementsCaps.Codec =
         pD3D12Enc->m_currentEncodeCapabilities.m_ResourceRequirementsCaps.Profile =
         pD3D12Enc->m_currentEncodeCapabilities.m_ResourceRequirementsCaps.InputFormat =
         pD3D12Enc->m_currentEncodeCapabilities.m_ResourceRequirementsCaps.PictureTargetResolution =
            HRESULT hr = pD3D12Enc->m_spD3D12VideoDevice->CheckFeatureSupport(
      D3D12_FEATURE_VIDEO_ENCODER_RESOURCE_REQUIREMENTS,
   &pD3D12Enc->m_currentEncodeCapabilities.m_ResourceRequirementsCaps,
         if (FAILED(hr)) {
      debug_printf("CheckFeatureSupport failed with HR %x\n", hr);
               if (!pD3D12Enc->m_currentEncodeCapabilities.m_ResourceRequirementsCaps.IsSupported) {
      debug_printf("[d3d12_video_encoder] D3D12_FEATURE_VIDEO_ENCODER_RESOURCE_REQUIREMENTS arguments are not supported.\n");
                        enum pipe_video_format codec = u_reduce_video_profile(pD3D12Enc->base.profile);
   d3d12_video_encoder_calculate_metadata_resolved_buffer_size(
      codec,
   pD3D12Enc->m_currentEncodeCapabilities.m_MaxSlicesInOutput,
         D3D12_HEAP_PROPERTIES Properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
   if ((pD3D12Enc->m_spEncodedFrameMetadata[current_metadata_slot].spBuffer == nullptr) ||
      (GetDesc(pD3D12Enc->m_spEncodedFrameMetadata[current_metadata_slot].spBuffer.Get()).Width <
   pD3D12Enc->m_spEncodedFrameMetadata[current_metadata_slot].bufferSize)) {
   CD3DX12_RESOURCE_DESC resolvedMetadataBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
            pD3D12Enc->m_spEncodedFrameMetadata[current_metadata_slot].spBuffer.Reset();
   HRESULT hr = pD3D12Enc->m_pD3D12Screen->dev->CreateCommittedResource(
      &Properties,
   D3D12_HEAP_FLAG_NONE,
   &resolvedMetadataBufferDesc,
   D3D12_RESOURCE_STATE_COMMON,
               if (FAILED(hr)) {
      debug_printf("CreateCommittedResource failed with HR %x\n", hr);
                  if ((pD3D12Enc->m_spEncodedFrameMetadata[current_metadata_slot].m_spMetadataOutputBuffer == nullptr) ||
      (GetDesc(pD3D12Enc->m_spEncodedFrameMetadata[current_metadata_slot].m_spMetadataOutputBuffer.Get()).Width <
   pD3D12Enc->m_currentEncodeCapabilities.m_ResourceRequirementsCaps.MaxEncoderOutputMetadataBufferSize)) {
   CD3DX12_RESOURCE_DESC metadataBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(
            pD3D12Enc->m_spEncodedFrameMetadata[current_metadata_slot].m_spMetadataOutputBuffer.Reset();
   HRESULT hr = pD3D12Enc->m_pD3D12Screen->dev->CreateCommittedResource(
      &Properties,
   D3D12_HEAP_FLAG_NONE,
   &metadataBufferDesc,
   D3D12_RESOURCE_STATE_COMMON,
               if (FAILED(hr)) {
      debug_printf("CreateCommittedResource failed with HR %x\n", hr);
         }
      }
      bool
   d3d12_video_encoder_reconfigure_session(struct d3d12_video_encoder *pD3D12Enc,
               {
      assert(pD3D12Enc->m_spD3D12VideoDevice);
   if(!d3d12_video_encoder_update_current_encoder_config_state(pD3D12Enc, srcTexture, picture)) {
      debug_printf("d3d12_video_encoder_update_current_encoder_config_state failed!\n");
      }
   if(!d3d12_video_encoder_reconfigure_encoder_objects(pD3D12Enc, srcTexture, picture)) {
      debug_printf("d3d12_video_encoder_reconfigure_encoder_objects failed!\n");
      }
   d3d12_video_encoder_update_picparams_tracking(pD3D12Enc, srcTexture, picture);
   if(!d3d12_video_encoder_prepare_output_buffers(pD3D12Enc, srcTexture, picture)) {
      debug_printf("d3d12_video_encoder_prepare_output_buffers failed!\n");
      }
      }
      /**
   * start encoding of a new frame
   */
   void
   d3d12_video_encoder_begin_frame(struct pipe_video_codec * codec,
               {
      // Do nothing here. Initialize happens on encoder creation, re-config (if any) happens in
   // d3d12_video_encoder_encode_bitstream
   struct d3d12_video_encoder *pD3D12Enc = (struct d3d12_video_encoder *) codec;
   assert(pD3D12Enc);
   HRESULT hr = S_OK;
   debug_printf("[d3d12_video_encoder] d3d12_video_encoder_begin_frame started for fenceValue: %" PRIu64 "\n",
            ///
   /// Wait here to make sure the next in flight resource set is empty before using it
   ///
            debug_printf("[d3d12_video_encoder] d3d12_video_encoder_begin_frame Waiting for completion of in flight resource sets with previous work with fenceValue: %" PRIu64 "\n",
                     if (!d3d12_video_encoder_reconfigure_session(pD3D12Enc, target, picture)) {
      debug_printf("[d3d12_video_encoder] d3d12_video_encoder_begin_frame - Failure on "
                     hr = pD3D12Enc->m_spEncodeCommandList->Reset(pD3D12Enc->m_inflightResourcesPool[d3d12_video_encoder_pool_current_index(pD3D12Enc)].m_spCommandAllocator.Get());
   if (FAILED(hr)) {
      debug_printf(
      "[d3d12_video_encoder] d3d12_video_encoder_flush - resetting ID3D12GraphicsCommandList failed with HR %x\n",
                           debug_printf("[d3d12_video_encoder] d3d12_video_encoder_begin_frame finalized for fenceValue: %" PRIu64 "\n",
               fail:
      debug_printf("[d3d12_video_encoder] d3d12_video_encoder_begin_frame failed for fenceValue: %" PRIu64 "\n",
            }
      void
   d3d12_video_encoder_calculate_metadata_resolved_buffer_size(enum pipe_video_format codec, uint32_t maxSliceNumber, uint64_t &bufferSize)
   {
      bufferSize = sizeof(D3D12_VIDEO_ENCODER_OUTPUT_METADATA) +
               switch (codec) {
      case PIPE_VIDEO_FORMAT_MPEG4_AVC:
   case PIPE_VIDEO_FORMAT_HEVC:
   #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
         case PIPE_VIDEO_FORMAT_AV1:
   {
      size_t extra_av1_size = d3d12_video_encoder_calculate_metadata_resolved_buffer_size_av1(maxSliceNumber);
      #endif
         default:
   {
               }
      // Returns the number of slices that the output will contain for fixed slicing modes
   // and the maximum number of slices the output might contain for dynamic slicing modes (eg. max bytes per slice)
   uint32_t
   d3d12_video_encoder_calculate_max_slices_count_in_output(
      D3D12_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE                          slicesMode,
   const D3D12_VIDEO_ENCODER_PICTURE_CONTROL_SUBREGIONS_LAYOUT_DATA_SLICES *slicesConfig,
   uint32_t                                                                 MaxSubregionsNumberFromCaps,
   D3D12_VIDEO_ENCODER_PICTURE_RESOLUTION_DESC                              sequenceTargetResolution,
      {
      uint32_t pic_width_in_subregion_units =
         uint32_t pic_height_in_subregion_units =
         uint32_t total_picture_subregion_units = pic_width_in_subregion_units * pic_height_in_subregion_units;
   uint32_t maxSlices                     = 0u;
   switch (slicesMode) {
      case D3D12_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE_FULL_FRAME:
   {
         } break;
   case D3D12_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE_BYTES_PER_SUBREGION:
   {
         } break;
   case D3D12_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE_SQUARE_UNITS_PER_SUBREGION_ROW_UNALIGNED:
   {
      maxSlices = static_cast<uint32_t>(
      } break;
   case D3D12_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE_UNIFORM_PARTITIONING_ROWS_PER_SUBREGION:
   {
      maxSlices = static_cast<uint32_t>(
      } break;
   case D3D12_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE_UNIFORM_PARTITIONING_SUBREGIONS_PER_FRAME:
   {
         } break;
   default:
   {
                        }
      /**
   * encode a bitstream
   */
   void
   d3d12_video_encoder_encode_bitstream(struct pipe_video_codec * codec,
                     {
      struct d3d12_video_encoder *pD3D12Enc = (struct d3d12_video_encoder *) codec;
   assert(pD3D12Enc);
   debug_printf("[d3d12_video_encoder] d3d12_video_encoder_encode_bitstream started for fenceValue: %" PRIu64 "\n",
         assert(pD3D12Enc->m_spD3D12VideoDevice);
   assert(pD3D12Enc->m_spEncodeCommandQueue);
            // Since this can be queried out of order in get_feedback, we need to pass out the actual value of the fence
   // and not the pointer to it (the fence value will keep increasing in the surfaces that have a pointer to it)
            struct d3d12_video_buffer *pInputVideoBuffer = (struct d3d12_video_buffer *) source;
   assert(pInputVideoBuffer);
   ID3D12Resource *pInputVideoD3D12Res        = d3d12_resource_resource(pInputVideoBuffer->texture);
                     // Make them permanently resident for video use
   d3d12_promote_to_permanent_residency(pD3D12Enc->m_pD3D12Screen, pOutputBitstreamBuffer);
                     /* Warning if the previous finished async execution stored was read not by get_feedback()
      before overwriting. This should be handled correctly by the app by calling vaSyncBuffer/vaSyncSurface
      if(!pD3D12Enc->m_spEncodedFrameMetadata[current_metadata_slot].bRead) {
      debug_printf("WARNING: [d3d12_video_encoder] d3d12_video_encoder_encode_bitstream - overwriting metadata slot %" PRIu64 " before calling get_feedback", current_metadata_slot);
      }
            ///
   /// Record Encode operation
            ///
   /// pInputVideoBuffer and pOutputBitstreamBuffer are passed externally
   /// and could be tracked by pipe_context and have pending ops. Flush any work on them and transition to
   /// D3D12_RESOURCE_STATE_COMMON before issuing work in Video command queue below. After the video work is done in the
   /// GPU, transition back to D3D12_RESOURCE_STATE_COMMON
   ///
   /// Note that unlike the D3D12TranslationLayer codebase, the state tracker here doesn't (yet) have any kind of
   /// multi-queue support, so it wouldn't implicitly synchronize when trying to transition between a graphics op and a
   /// video op.
            d3d12_transition_resource_state(
      d3d12_context(pD3D12Enc->base.context),
   pInputVideoBuffer->texture,
   D3D12_RESOURCE_STATE_COMMON,
      d3d12_transition_resource_state(d3d12_context(pD3D12Enc->base.context),
                              d3d12_resource_wait_idle(d3d12_context(pD3D12Enc->base.context),
                        ///
   /// Process pre-encode bitstream headers
            // Decide the D3D12 buffer EncodeFrame will write to based on pre-post encode headers generation policy
            d3d12_video_encoder_build_pre_encode_codec_headers(pD3D12Enc, 
                        // Only upload headers now and leave prefix offset space gap in compressed bitstream if the codec builds headers before execution.
   if (!pD3D12Enc->m_spEncodedFrameMetadata[current_metadata_slot].postEncodeHeadersNeeded)
               // Headers are written before encode execution, have EncodeFrame write directly into the pipe destination buffer
            // It can happen that codecs like H264/HEVC don't write pre-headers for all frames (ie. reuse previous PPS)
   if (pD3D12Enc->m_spEncodedFrameMetadata[current_metadata_slot].preEncodeGeneratedHeadersByteSize > 0)
   {
      // If driver needs offset alignment for bitstream resource, we will pad zeroes on the codec header to this end.
   if (
      (pD3D12Enc->m_currentEncodeCapabilities.m_ResourceRequirementsCaps.CompressedBitstreamBufferAccessAlignment > 1)
      ) {
      pD3D12Enc->m_spEncodedFrameMetadata[current_metadata_slot].preEncodeGeneratedHeadersByteSize = ALIGN(pD3D12Enc->m_spEncodedFrameMetadata[current_metadata_slot].preEncodeGeneratedHeadersByteSize, pD3D12Enc->m_currentEncodeCapabilities.m_ResourceRequirementsCaps.CompressedBitstreamBufferAccessAlignment);
               // Upload the CPU buffers with the bitstream headers to the compressed bitstream resource in the interval
   // [0..pD3D12Enc->m_spEncodedFrameMetadata[current_metadata_slot].preEncodeGeneratedHeadersByteSize)
                  pD3D12Enc->base.context->buffer_subdata(
      pD3D12Enc->base.context,         // context
   &pOutputBitstreamBuffer->base.b, // dst buffer
   PIPE_MAP_WRITE,                  // usage PIPE_MAP_x
   0,                               // offset
   pD3D12Enc->m_BitstreamHeadersBuffer.size(),
      }
   else
   {
      assert(pD3D12Enc->m_spEncodedFrameMetadata[current_metadata_slot].preEncodeGeneratedHeadersByteSize == 0);
   if (pD3D12Enc->m_spEncodedFrameMetadata[current_metadata_slot].spStagingBitstream == nullptr) {
      D3D12_HEAP_PROPERTIES Properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
   CD3DX12_RESOURCE_DESC resolvedMetadataBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(D3D12_DEFAULT_COMPBIT_STAGING_SIZE);
   HRESULT hr = pD3D12Enc->m_pD3D12Screen->dev->CreateCommittedResource(
      &Properties,
   D3D12_HEAP_FLAG_NONE,
   &resolvedMetadataBufferDesc,
   D3D12_RESOURCE_STATE_COMMON,
               if (FAILED(hr)) {
      debug_printf("CreateCommittedResource failed with HR %x\n", hr);
         }
      // Headers are written after execution, have EncodeFrame write into a staging buffer 
   // and then get_feedback will pack the finalized bitstream and copy into comp_bit_destination
   pOutputBufferD3D12Res = pD3D12Enc->m_spEncodedFrameMetadata[current_metadata_slot].spStagingBitstream.Get();
      // Save the pipe destination buffer the headers need to be written to in get_feedback
               std::vector<D3D12_RESOURCE_BARRIER> rgCurrentFrameStateTransitions = {
      CD3DX12_RESOURCE_BARRIER::Transition(pInputVideoD3D12Res,
               CD3DX12_RESOURCE_BARRIER::Transition(pOutputBufferD3D12Res,
               CD3DX12_RESOURCE_BARRIER::Transition(pD3D12Enc->m_spEncodedFrameMetadata[current_metadata_slot].m_spMetadataOutputBuffer.Get(),
                     pD3D12Enc->m_spEncodeCommandList->ResourceBarrier(rgCurrentFrameStateTransitions.size(),
            D3D12_VIDEO_ENCODER_RECONSTRUCTED_PICTURE reconPicOutputTextureDesc =
         D3D12_VIDEO_ENCODE_REFERENCE_FRAMES referenceFramesDescriptor =
                  // Transition DPB reference pictures to read mode
   uint32_t                            maxReferences = d3d12_video_encoder_get_current_max_dpb_capacity(pD3D12Enc);
   std::vector<D3D12_RESOURCE_BARRIER> rgReferenceTransitions(maxReferences);
   if ((referenceFramesDescriptor.NumTexture2Ds > 0) ||
      (pD3D12Enc->m_upDPBManager->is_current_frame_used_as_reference())) {
   rgReferenceTransitions.clear();
            if (reconPicOutputTextureDesc.pReconstructedPicture != nullptr)
                                          // Transition all subresources of each reference frame independent resource allocation
   for (uint32_t referenceIdx = 0; referenceIdx < referenceFramesDescriptor.NumTexture2Ds; referenceIdx++) {
      rgReferenceTransitions.push_back(
      CD3DX12_RESOURCE_BARRIER::Transition(referenceFramesDescriptor.ppTexture2Ds[referenceIdx],
                  // Transition all subresources the output recon pic independent resource allocation
   if (reconPicOutputTextureDesc.pReconstructedPicture != nullptr) {
      rgReferenceTransitions.push_back(
      CD3DX12_RESOURCE_BARRIER::Transition(reconPicOutputTextureDesc.pReconstructedPicture,
                                 // In Texture array mode, the dpb storage allocator uses the same texture array for all the input
                  #if DEBUG
      // the reconpic output should be all the same texarray allocation
   if((reconPicOutputTextureDesc.pReconstructedPicture) && (referenceFramesDescriptor.NumTexture2Ds > 0))
            for (uint32_t refIndex = 0; refIndex < referenceFramesDescriptor.NumTexture2Ds; refIndex++) {
            // all reference frames inputs should be all the same texarray allocation
         #endif
                                 uint32_t MipLevel, PlaneSlice, ArraySlice;
   D3D12DecomposeSubresource(referenceSubresource,
                                                                     rgReferenceTransitions.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
      // Always same allocation in texarray mode
   referenceFramesDescriptor.ppTexture2Ds[0],
   D3D12_RESOURCE_STATE_COMMON,
   // If this is the subresource for the reconpic output allocation, transition to ENCODE_WRITE
   // Otherwise, it's a subresource for an input reference picture, transition to ENCODE_READ
   (referenceSubresource == reconPicOutputTextureDesc.ReconstructedPictureSubresource) ?
      D3D12_RESOURCE_STATE_VIDEO_ENCODE_WRITE :
                     if (rgReferenceTransitions.size() > 0) {
      pD3D12Enc->m_spEncodeCommandList->ResourceBarrier(static_cast<uint32_t>(rgReferenceTransitions.size()),
                  // Update current frame pic params state after reconfiguring above.
   D3D12_VIDEO_ENCODER_PICTURE_CONTROL_CODEC_DATA currentPicParams =
                  // Stores D3D12_VIDEO_ENCODER_AV1_REFERENCE_PICTURE_DESCRIPTOR in the associated metadata
   // for header generation after execution (if applicable)
   if (pD3D12Enc->m_spEncodedFrameMetadata[current_metadata_slot].postEncodeHeadersNeeded) {
                  const D3D12_VIDEO_ENCODER_ENCODEFRAME_INPUT_ARGUMENTS inputStreamArguments = {
      // D3D12_VIDEO_ENCODER_SEQUENCE_CONTROL_DESC
   { // D3D12_VIDEO_ENCODER_SEQUENCE_CONTROL_FLAGS
   pD3D12Enc->m_currentEncodeConfig.m_seqFlags,
   // D3D12_VIDEO_ENCODER_INTRA_REFRESH
   pD3D12Enc->m_currentEncodeConfig.m_IntraRefresh,
   d3d12_video_encoder_get_current_rate_control_settings(pD3D12Enc),
   // D3D12_VIDEO_ENCODER_PICTURE_RESOLUTION_DESC
   pD3D12Enc->m_currentEncodeConfig.m_currentResolution,
   pD3D12Enc->m_currentEncodeConfig.m_encoderSliceConfigMode,
   d3d12_video_encoder_get_current_slice_param_settings(pD3D12Enc),
   d3d12_video_encoder_get_current_gop_desc(pD3D12Enc) },
   // D3D12_VIDEO_ENCODER_PICTURE_CONTROL_DESC
   { // uint32_t IntraRefreshFrameIndex;
   pD3D12Enc->m_currentEncodeConfig.m_IntraRefreshCurrentFrameIndex,
   // D3D12_VIDEO_ENCODER_PICTURE_CONTROL_FLAGS Flags;
   picCtrlFlags,
   // D3D12_VIDEO_ENCODER_PICTURE_CONTROL_CODEC_DATA PictureControlCodecData;
   currentPicParams,
   // D3D12_VIDEO_ENCODE_REFERENCE_FRAMES ReferenceFrames;
   referenceFramesDescriptor },
   pInputVideoD3D12Res,
   inputVideoD3D12Subresource,
   static_cast<UINT>(pD3D12Enc->m_spEncodedFrameMetadata[current_metadata_slot].preEncodeGeneratedHeadersByteSize)
   // budgeting. - User can also calculate headers fixed size beforehand (eg. no VUI,
               const D3D12_VIDEO_ENCODER_ENCODEFRAME_OUTPUT_ARGUMENTS outputStreamArguments = {
      // D3D12_VIDEO_ENCODER_COMPRESSED_BITSTREAM
   {
      pOutputBufferD3D12Res,
      },
   // D3D12_VIDEO_ENCODER_RECONSTRUCTED_PICTURE
   reconPicOutputTextureDesc,
   // D3D12_VIDEO_ENCODER_ENCODE_OPERATION_METADATA_BUFFER
               // Record EncodeFrame
   pD3D12Enc->m_spEncodeCommandList->EncodeFrame(pD3D12Enc->m_spVideoEncoder.Get(),
                        D3D12_RESOURCE_BARRIER rgResolveMetadataStateTransitions[] = {
      CD3DX12_RESOURCE_BARRIER::Transition(pD3D12Enc->m_spEncodedFrameMetadata[current_metadata_slot].spBuffer.Get(),
               CD3DX12_RESOURCE_BARRIER::Transition(pD3D12Enc->m_spEncodedFrameMetadata[current_metadata_slot].m_spMetadataOutputBuffer.Get(),
               CD3DX12_RESOURCE_BARRIER::Transition(pInputVideoD3D12Res,
               CD3DX12_RESOURCE_BARRIER::Transition(pOutputBufferD3D12Res,
                     pD3D12Enc->m_spEncodeCommandList->ResourceBarrier(_countof(rgResolveMetadataStateTransitions),
            const D3D12_VIDEO_ENCODER_RESOLVE_METADATA_INPUT_ARGUMENTS inputMetadataCmd = {
      pD3D12Enc->m_currentEncodeConfig.m_encoderCodecDesc,
   d3d12_video_encoder_get_current_profile_desc(pD3D12Enc),
   pD3D12Enc->m_currentEncodeConfig.m_encodeFormatInfo.Format,
   // D3D12_VIDEO_ENCODER_PICTURE_RESOLUTION_DESC
   pD3D12Enc->m_currentEncodeConfig.m_currentResolution,
               const D3D12_VIDEO_ENCODER_RESOLVE_METADATA_OUTPUT_ARGUMENTS outputMetadataCmd = {
      /*If offset were to change, has to be aligned to pD3D12Enc->m_currentEncodeCapabilities.m_ResourceRequirementsCaps.EncoderMetadataBufferAccessAlignment*/
      };
            debug_printf("[d3d12_video_encoder_encode_bitstream] EncodeFrame slot %" PRIu64 " encoder %p encoderheap %p input tex %p output bitstream %p raw metadata buf %p resolved metadata buf %p Command allocator %p\n",
               d3d12_video_encoder_pool_current_index(pD3D12Enc),
   pD3D12Enc->m_spVideoEncoder.Get(),
   pD3D12Enc->m_spVideoEncoderHeap.Get(),
   inputStreamArguments.pInputFrame,
   outputStreamArguments.Bitstream.pBuffer,
            // Transition DPB reference pictures back to COMMON
   if ((referenceFramesDescriptor.NumTexture2Ds > 0) ||
      (pD3D12Enc->m_upDPBManager->is_current_frame_used_as_reference())) {
   for (auto &BarrierDesc : rgReferenceTransitions) {
                  if (rgReferenceTransitions.size() > 0) {
      pD3D12Enc->m_spEncodeCommandList->ResourceBarrier(static_cast<uint32_t>(rgReferenceTransitions.size()),
                  D3D12_RESOURCE_BARRIER rgRevertResolveMetadataStateTransitions[] = {
      CD3DX12_RESOURCE_BARRIER::Transition(pD3D12Enc->m_spEncodedFrameMetadata[current_metadata_slot].spBuffer.Get(),
               CD3DX12_RESOURCE_BARRIER::Transition(pD3D12Enc->m_spEncodedFrameMetadata[current_metadata_slot].m_spMetadataOutputBuffer.Get(),
                     pD3D12Enc->m_spEncodeCommandList->ResourceBarrier(_countof(rgRevertResolveMetadataStateTransitions),
            debug_printf("[d3d12_video_encoder] d3d12_video_encoder_encode_bitstream finalized for fenceValue: %" PRIu64 "\n",
      }
      void
   d3d12_video_encoder_get_feedback(struct pipe_video_codec *codec, void *feedback, unsigned *size)
   {
      struct d3d12_video_encoder *pD3D12Enc = (struct d3d12_video_encoder *) codec;
            uint64_t requested_metadata_fence = ((uint64_t) feedback);
                     debug_printf("d3d12_video_encoder_get_feedback with feedback: %" PRIu64 ", resources slot %" PRIu64 " metadata resolved ID3D12Resource buffer %p metadata required size %" PRIu64 "\n",
      requested_metadata_fence,
   (requested_metadata_fence % D3D12_VIDEO_ENC_ASYNC_DEPTH),
   pD3D12Enc->m_spEncodedFrameMetadata[current_metadata_slot].spBuffer.Get(),
         if((pD3D12Enc->m_fenceValue - requested_metadata_fence) > D3D12_VIDEO_ENC_METADATA_BUFFERS_COUNT)
   {
      debug_printf("[d3d12_video_encoder_get_feedback] Requested metadata for fence %" PRIu64 " at current fence %" PRIu64
      " is too far back in time for the ring buffer of size %" PRIu64 " we keep track off - "
   " Please increase the D3D12_VIDEO_ENC_METADATA_BUFFERS_COUNT environment variable and try again.\n",
   requested_metadata_fence,
   pD3D12Enc->m_fenceValue,
      *size = 0;
   assert(false);
               if(pD3D12Enc->m_spEncodedFrameMetadata[current_metadata_slot].postEncodeHeadersNeeded)
   {
      ///
   /// If we didn't write headers before encode execution, finalize the codec specific bitsteam now
            *size = d3d12_video_encoder_build_post_encode_codec_bitstream(
      // Current encoder
   pD3D12Enc,
   // Associated frame fenceValue
   requested_metadata_fence,
   // metadata desc
         }
   else
   {
      ///
   /// If we wrote headers (if any) before encode execution, use that size to calculate feedback size of complete bitstream.
            D3D12_VIDEO_ENCODER_OUTPUT_METADATA                       encoderMetadata;
   std::vector<D3D12_VIDEO_ENCODER_FRAME_SUBREGION_METADATA> pSubregionsMetadata;
   d3d12_video_encoder_extract_encode_metadata(
      pD3D12Enc,
   pD3D12Enc->m_spEncodedFrameMetadata[current_metadata_slot].spBuffer.Get(),
   pD3D12Enc->m_spEncodedFrameMetadata[current_metadata_slot].bufferSize,
               // Read metadata from encoderMetadata
   if (encoderMetadata.EncodeErrorFlags != D3D12_VIDEO_ENCODER_ENCODE_ERROR_FLAG_NO_ERROR) {
      debug_printf("[d3d12_video_encoder] Encode GPU command for fence %" PRIu64 " failed - EncodeErrorFlags: %" PRIu64 "\n",
               *size = 0;
   assert(false);
                           }
      debug_printf("[d3d12_video_encoder_get_feedback] Requested metadata for encoded frame at fence %" PRIu64 " is %d (feedback was requested at current fence %" PRIu64 ")\n",
         requested_metadata_fence,
               }
      unsigned
   d3d12_video_encoder_build_post_encode_codec_bitstream(struct d3d12_video_encoder * pD3D12Enc,
               {
      enum pipe_video_format codec_format = u_reduce_video_profile(pD3D12Enc->base.profile);
   switch (codec_format) {
      case PIPE_VIDEO_FORMAT_MPEG4_AVC:
   case PIPE_VIDEO_FORMAT_HEVC:
   {
         #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
         case PIPE_VIDEO_FORMAT_AV1:
   {
      return d3d12_video_encoder_build_post_encode_codec_bitstream_av1(
      // Current encoder
   pD3D12Enc,
   // associated fence value
   associated_fence_value,
   // Metadata desc
         #endif
         default:
         }
      void
   d3d12_video_encoder_extract_encode_metadata(
      struct d3d12_video_encoder *                               pD3D12Enc,
   ID3D12Resource *                                           pResolvedMetadataBuffer,   // input
   uint64_t                                                   resourceMetadataSize,      // input
   D3D12_VIDEO_ENCODER_OUTPUT_METADATA &                      parsedMetadata,            // output
      )
   {
      struct d3d12_screen *pD3D12Screen = (struct d3d12_screen *) pD3D12Enc->m_pD3D12Screen;
   assert(pD3D12Screen);
   pipe_resource *pPipeResolvedMetadataBuffer =
         assert(pPipeResolvedMetadataBuffer);
   assert(resourceMetadataSize < INT_MAX);
   struct pipe_box box = {
      0,                                        // x
   0,                                        // y
   0,                                        // z
   static_cast<int>(resourceMetadataSize),   // width
   1,                                        // height
      };
   struct pipe_transfer *mapTransfer;
   unsigned mapUsage = PIPE_MAP_READ;
   void *                pMetadataBufferSrc = pD3D12Enc->base.context->buffer_map(pD3D12Enc->base.context,
                                    assert(mapUsage & PIPE_MAP_READ);
   assert(pPipeResolvedMetadataBuffer->usage == PIPE_USAGE_DEFAULT);
   // Note: As we're calling buffer_map with PIPE_MAP_READ on a pPipeResolvedMetadataBuffer which has pipe_usage_default
   // buffer_map itself will do all the synchronization and waits so once the function returns control here
            // Clear output
            // Calculate sizes
            // Copy buffer to the appropriate D3D12_VIDEO_ENCODER_OUTPUT_METADATA memory layout
            // As specified in D3D12 Encode spec, the array base for metadata for the slices
   // (D3D12_VIDEO_ENCODER_FRAME_SUBREGION_METADATA[]) is placed in memory immediately after the
   // D3D12_VIDEO_ENCODER_OUTPUT_METADATA structure
   D3D12_VIDEO_ENCODER_FRAME_SUBREGION_METADATA *pFrameSubregionMetadata =
      reinterpret_cast<D3D12_VIDEO_ENCODER_FRAME_SUBREGION_METADATA *>(reinterpret_cast<uint8_t *>(pMetadataBufferSrc) +
         // Copy fields into D3D12_VIDEO_ENCODER_FRAME_SUBREGION_METADATA
   assert(parsedMetadata.WrittenSubregionsCount < SIZE_MAX);
   pSubregionsMetadata.resize(static_cast<size_t>(parsedMetadata.WrittenSubregionsCount));
   for (uint32_t sliceIdx = 0; sliceIdx < parsedMetadata.WrittenSubregionsCount; sliceIdx++) {
      pSubregionsMetadata[sliceIdx].bHeaderSize  = pFrameSubregionMetadata[sliceIdx].bHeaderSize;
   pSubregionsMetadata[sliceIdx].bSize        = pFrameSubregionMetadata[sliceIdx].bSize;
               // Unmap the buffer tmp storage
   pipe_buffer_unmap(pD3D12Enc->base.context, mapTransfer);
      }
      /**
   * end encoding of the current frame
   */
   void
   d3d12_video_encoder_end_frame(struct pipe_video_codec * codec,
               {
      struct d3d12_video_encoder *pD3D12Enc = (struct d3d12_video_encoder *) codec;
   assert(pD3D12Enc);
   debug_printf("[d3d12_video_encoder] d3d12_video_encoder_end_frame started for fenceValue: %" PRIu64 "\n",
            // Signal finish of current frame encoding to the picture management tracker
            // Save extra references of Encoder, EncoderHeap and DPB allocations in case
   // there's a reconfiguration that trigers the construction of new objects
   pD3D12Enc->m_inflightResourcesPool[d3d12_video_encoder_pool_current_index(pD3D12Enc)].m_spEncoder = pD3D12Enc->m_spVideoEncoder;
   pD3D12Enc->m_inflightResourcesPool[d3d12_video_encoder_pool_current_index(pD3D12Enc)].m_spEncoderHeap = pD3D12Enc->m_spVideoEncoderHeap;
            debug_printf("[d3d12_video_encoder] d3d12_video_encoder_end_frame finalized for fenceValue: %" PRIu64 "\n",
               }
      void
   d3d12_video_encoder_store_current_picture_references(d3d12_video_encoder *pD3D12Enc,
         {
      enum pipe_video_format codec = u_reduce_video_profile(pD3D12Enc->base.profile);
   switch (codec) {
      case PIPE_VIDEO_FORMAT_MPEG4_AVC:
   {
                  case PIPE_VIDEO_FORMAT_HEVC:
   {
         #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
         case PIPE_VIDEO_FORMAT_AV1:
   {
         #endif
         default:
   {
               }
