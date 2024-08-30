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
      #include "d3d12_context.h"
   #include "d3d12_format.h"
   #include "d3d12_resource.h"
   #include "d3d12_screen.h"
   #include "d3d12_surface.h"
   #include "d3d12_video_dec.h"
   #include "d3d12_video_dec_h264.h"
   #include "d3d12_video_dec_hevc.h"
   #include "d3d12_video_dec_av1.h"
   #include "d3d12_video_dec_vp9.h"
   #include "d3d12_video_buffer.h"
   #include "d3d12_residency.h"
      #include "vl/vl_video_buffer.h"
   #include "util/format/u_format.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "util/u_video.h"
      uint64_t
   d3d12_video_decoder_pool_current_index(struct d3d12_video_decoder *pD3D12Dec)
   {
         }
      struct pipe_video_codec *
   d3d12_video_create_decoder(struct pipe_context *context, const struct pipe_video_codec *codec)
   {
      ///
   /// Initialize d3d12_video_decoder
               // Not using new doesn't call ctor and the initializations in the class declaration are lost
                     pD3D12Dec->base = *codec;
            pD3D12Dec->base.context = context;
   pD3D12Dec->base.width = codec->width;
   pD3D12Dec->base.height = codec->height;
   // Only fill methods that are supported by the d3d12 decoder, leaving null the rest (ie. encode_* / decode_macroblock
   // / get_feedback for encode)
   pD3D12Dec->base.destroy = d3d12_video_decoder_destroy;
   pD3D12Dec->base.begin_frame = d3d12_video_decoder_begin_frame;
   pD3D12Dec->base.decode_bitstream = d3d12_video_decoder_decode_bitstream;
   pD3D12Dec->base.end_frame = d3d12_video_decoder_end_frame;
   pD3D12Dec->base.flush = d3d12_video_decoder_flush;
            pD3D12Dec->m_decodeFormat = d3d12_convert_pipe_video_profile_to_dxgi_format(codec->profile);
   pD3D12Dec->m_d3d12DecProfileType = d3d12_video_decoder_convert_pipe_video_profile_to_profile_type(codec->profile);
            ///
   /// Try initializing D3D12 Video device and check for device caps
            struct d3d12_context *pD3D12Ctx = (struct d3d12_context *) context;
            ///
   /// Create decode objects
   ///
   HRESULT hr = S_OK;
   if (FAILED(pD3D12Dec->m_pD3D12Screen->dev->QueryInterface(
            debug_printf("[d3d12_video_decoder] d3d12_video_create_decoder - D3D12 Device has no Video support\n");
               if (!d3d12_video_decoder_check_caps_and_create_decoder(pD3D12Dec->m_pD3D12Screen, pD3D12Dec)) {
      debug_printf("[d3d12_video_decoder] d3d12_video_create_decoder - Failure on "
                     if (!d3d12_video_decoder_create_command_objects(pD3D12Dec->m_pD3D12Screen, pD3D12Dec)) {
      debug_printf(
                     if (!d3d12_video_decoder_create_video_state_buffers(pD3D12Dec->m_pD3D12Screen, pD3D12Dec)) {
      debug_printf("[d3d12_video_decoder] d3d12_video_create_decoder - Failure on "
                     pD3D12Dec->m_decodeFormatInfo = { pD3D12Dec->m_decodeFormat };
   hr = pD3D12Dec->m_pD3D12Screen->dev->CheckFeatureSupport(D3D12_FEATURE_FORMAT_INFO,
               if (FAILED(hr)) {
      debug_printf("CheckFeatureSupport failed with HR %x\n", hr);
                     failed:
      if (pD3D12Dec != nullptr) {
                     }
      /**
   * Destroys a d3d12_video_decoder
   * Call destroy_XX for applicable XX nested member types before deallocating
   * Destroy methods should check != nullptr on their input target argument as this method can be called as part of
   * cleanup from failure on the creation method
   */
   void
   d3d12_video_decoder_destroy(struct pipe_video_codec *codec)
   {
      if (codec == nullptr) {
                  struct d3d12_video_decoder *pD3D12Dec = (struct d3d12_video_decoder *) codec;
   // Flush and wait for completion of any in-flight GPU work before destroying objects
   d3d12_video_decoder_flush(codec);
   if (pD3D12Dec->m_fenceValue > 1 /* Check we submitted at least one frame */) {
      auto decode_queue_completion_fence = pD3D12Dec->m_inflightResourcesPool[(pD3D12Dec->m_fenceValue - 1u) % D3D12_VIDEO_DEC_ASYNC_DEPTH].m_FenceData;
   d3d12_video_decoder_sync_completion(codec, decode_queue_completion_fence.cmdqueue_fence, decode_queue_completion_fence.value, OS_TIMEOUT_INFINITE);
   struct pipe_fence_handle *context_queue_completion_fence = NULL;
   pD3D12Dec->base.context->flush(pD3D12Dec->base.context, &context_queue_completion_fence, PIPE_FLUSH_ASYNC | PIPE_FLUSH_HINT_FINISH);
   pD3D12Dec->m_pD3D12Screen->base.fence_finish(&pD3D12Dec->m_pD3D12Screen->base, NULL, context_queue_completion_fence, OS_TIMEOUT_INFINITE);
               //
   // Destroys a decoder
   // Call destroy_XX for applicable XX nested member types before deallocating
   // Destroy methods should check != nullptr on their input target argument as this method can be called as part of
   // cleanup from failure on the creation method
            // No need for d3d12_destroy_video_objects
   //    All the objects created here are smart pointer members of d3d12_video_decoder
   // No need for d3d12_destroy_video_decoder_and_heap
   //    All the objects created here are smart pointer members of d3d12_video_decoder
   // No need for d3d12_destroy_video_dpbmanagers
                     // Call dtor to make ComPtr work
      }
      /**
   * start decoding of a new frame
   */
   void
   d3d12_video_decoder_begin_frame(struct pipe_video_codec *codec,
               {
      // Do nothing here. Initialize happens on decoder creation, re-config (if any) happens in
   // d3d12_video_decoder_decode_bitstream
   struct d3d12_video_decoder *pD3D12Dec = (struct d3d12_video_decoder *) codec;
            ///
   /// Wait here to make sure the next in flight resource set is empty before using it
   ///
   uint64_t fenceValueToWaitOn = static_cast<uint64_t>(
      std::max(static_cast<int64_t>(0l),
         debug_printf("[d3d12_video_decoder] d3d12_video_decoder_begin_frame Waiting for completion of in flight resource "
                  ASSERTED bool wait_res =
                  HRESULT hr = pD3D12Dec->m_spDecodeCommandList->Reset(
         if (FAILED(hr)) {
      debug_printf("[d3d12_video_decoder] resetting ID3D12GraphicsCommandList failed with HR %x\n", hr);
               debug_printf("[d3d12_video_decoder] d3d12_video_decoder_begin_frame finalized for fenceValue: %d\n",
      }
      /**
   * decode a bitstream
   */
   void
   d3d12_video_decoder_decode_bitstream(struct pipe_video_codec *codec,
                                 {
      struct d3d12_video_decoder *pD3D12Dec = (struct d3d12_video_decoder *) codec;
   assert(pD3D12Dec);
   debug_printf("[d3d12_video_decoder] d3d12_video_decoder_decode_bitstream started for fenceValue: %d\n",
         assert(pD3D12Dec->m_spD3D12VideoDevice);
   assert(pD3D12Dec->m_spDecodeCommandQueue);
   assert(pD3D12Dec->m_pD3D12Screen);
   ASSERTED struct d3d12_video_buffer *pD3D12VideoBuffer = (struct d3d12_video_buffer *) target;
            ///
   /// Compressed bitstream buffers
            /// Mesa VA frontend Video buffer passing semantics for H264, HEVC, MPEG4, VC1 and PIPE_VIDEO_PROFILE_VC1_ADVANCED
   /// are: If num_buffers == 1 -> buf[0] has the compressed bitstream WITH the starting code If num_buffers == 2 ->
   /// buf[0] has the NALU starting code and buf[1] has the compressed bitstream WITHOUT any starting code. If
   /// num_buffers = 3 -> It's JPEG, not supported in D3D12. num_buffers is at most 3.
   /// Mesa VDPAU frontend passes the buffers as they get passed in VdpDecoderRender without fixing any start codes
   /// except for PIPE_VIDEO_PROFILE_VC1_ADVANCED
   // In https://http.download.nvidia.com/XFree86/vdpau/doxygen/html/index.html#video_mixer_usage it's mentioned that:
   // It is recommended that applications pass solely the slice data to VDPAU; specifically that any header data
   // structures be excluded from the portion of the bitstream passed to VDPAU. VDPAU implementations must operate
   // correctly if non-slice data is included, at least for formats employing start codes to delimit slice data. For all
   // codecs/profiles it's highly recommended (when the codec/profile has such codes...) that the start codes are passed
   // to VDPAU, even when not included in the bitstream the VDPAU client is parsing. Let's assume we get all the start
   // codes for VDPAU. The doc also says "VDPAU implementations must operate correctly if non-slice data is included, at
   // least for formats employing start codes to delimit slice data" if we ever get an issue with VDPAU start codes we
            // To handle the multi-slice case end_frame already takes care of this by parsing the start codes from the
            // VAAPI seems to send one decode_bitstream command per slice, but we should also support the VDPAU case where the
   // buffers have multiple buffer array entry per slice {startCode (optional), slice1, slice2, ..., startCode
            if (num_buffers > 2)   // Assume this means multiple slices at once in a decode_bitstream call
   {
      // Based on VA frontend codebase, this never happens for video (no JPEG)
            // To handle the case where VDPAU send all the slices at once in a single decode_bitstream call, let's pretend it
            // group by start codes and buffers and perform calls for the number of slices
   debug_printf("[d3d12_video_decoder] d3d12_video_decoder_decode_bitstream multiple slices on same call detected "
                           // Vars to be used for the delegation calls to decode_bitstream
   unsigned call_num_buffers = 0;
   const void *const *call_buffers = nullptr;
            while (curBufferIdx < num_buffers) {
      // Store the current buffer as the base array pointer for the delegated call, later decide if it'll be a
   // startcode+slicedata or just slicedata call
                  // Usually start codes are less or equal than 4 bytes
   // If the current buffer is a start code buffer, send it along with the next buffer. Otherwise, just send the
                                       } else {
      ///
   /// Handle single slice buffer path, maybe with an extra start code buffer at buffers[0].
            // Both the start codes being present at buffers[0] and the rest in buffers [1] or full buffer at [0] cases can be
            size_t totalReceivedBuffersSize = 0u;   // Combined size of all sizes[]
   for (size_t bufferIdx = 0; bufferIdx < num_buffers; bufferIdx++) {
                  // Bytes of data pre-staged before this decode_frame call
   auto &inFlightResources = pD3D12Dec->m_inflightResourcesPool[d3d12_video_decoder_pool_current_index(pD3D12Dec)];
            // Extend the staging buffer size, as decode_frame can be called several times before end_frame
            // Point newSliceDataPositionDstBase to the end of the pre-staged data in m_stagingDecodeBitstream, where the new
   // buffers will be appended
            // Append new data at the end.
   size_t dstOffset = 0u;
   for (size_t bufferIdx = 0; bufferIdx < num_buffers; bufferIdx++) {
      memcpy(newSliceDataPositionDstBase + dstOffset, buffers[bufferIdx], sizes[bufferIdx]);
               debug_printf("[d3d12_video_decoder] d3d12_video_decoder_decode_bitstream finalized for fenceValue: %d\n",
               if (pD3D12Dec->m_d3d12DecProfileType == d3d12_video_decode_profile_type_h264) {
      struct pipe_h264_picture_desc *h264 = (pipe_h264_picture_desc*) picture;
         }
      void
   d3d12_video_decoder_store_upper_layer_references(struct d3d12_video_decoder *pD3D12Dec,
               {
      pD3D12Dec->m_pCurrentDecodeTarget = target;
   switch (pD3D12Dec->m_d3d12DecProfileType) {
      case d3d12_video_decode_profile_type_h264:
   {
      pipe_h264_picture_desc *pPicControlH264 = (pipe_h264_picture_desc *) picture;
               case d3d12_video_decode_profile_type_hevc:
   {
      pipe_h265_picture_desc *pPicControlHevc = (pipe_h265_picture_desc *) picture;
               case d3d12_video_decode_profile_type_av1:
   {
      pipe_av1_picture_desc *pPicControlAV1 = (pipe_av1_picture_desc *) picture;
               case d3d12_video_decode_profile_type_vp9:
   {
      pipe_vp9_picture_desc *pPicControlVP9 = (pipe_vp9_picture_desc *) picture;
               default:
   {
               }
      /**
   * end decoding of the current frame
   */
   void
   d3d12_video_decoder_end_frame(struct pipe_video_codec *codec,
               {
      struct d3d12_video_decoder *pD3D12Dec = (struct d3d12_video_decoder *) codec;
   assert(pD3D12Dec);
   struct d3d12_screen *pD3D12Screen = (struct d3d12_screen *) pD3D12Dec->m_pD3D12Screen;
   assert(pD3D12Screen);
   debug_printf("[d3d12_video_decoder] d3d12_video_decoder_end_frame started for fenceValue: %d\n",
         assert(pD3D12Dec->m_spD3D12VideoDevice);
   assert(pD3D12Dec->m_spDecodeCommandQueue);
   struct d3d12_video_buffer *pD3D12VideoBuffer = (struct d3d12_video_buffer *) target;
            ///
   /// Store current decode output target texture and reference textures from upper layer
   ///
            ///
   /// Codec header picture parameters buffers
                     d3d12_video_decoder_store_converted_dxva_picparams_from_pipe_input(pD3D12Dec, picture, pD3D12VideoBuffer);
            ///
   /// Prepare Slice control buffers before clearing staging buffer
   ///
   assert(inFlightResources.m_stagingDecodeBitstream.size() >
         d3d12_video_decoder_prepare_dxva_slices_control(pD3D12Dec, picture);
            ///
   /// Upload m_stagingDecodeBitstream to GPU memory now that end_frame is called and clear staging buffer
            uint64_t sliceDataStagingBufferSize = inFlightResources.m_stagingDecodeBitstream.size();
            // Reallocate if necessary to accomodate the current frame bitstream buffer in GPU memory
   if (inFlightResources.m_curFrameCompressedBitstreamBufferAllocatedSize < sliceDataStagingBufferSize) {
      if (!d3d12_video_decoder_create_staging_bitstream_buffer(pD3D12Screen, pD3D12Dec, sliceDataStagingBufferSize)) {
      debug_printf("[d3d12_video_decoder] d3d12_video_decoder_end_frame - Failure on "
         debug_printf("[d3d12_video_decoder] d3d12_video_decoder_end_frame failed for fenceValue: %d\n",
         assert(false);
                  // Upload frame bitstream CPU data to ID3D12Resource buffer
   inFlightResources.m_curFrameCompressedBitstreamBufferPayloadSize =
         assert(inFlightResources.m_curFrameCompressedBitstreamBufferPayloadSize <=
            /* One-shot transfer operation with data supplied in a user
   * pointer.
   */
   inFlightResources.pPipeCompressedBufferObj =
         assert(inFlightResources.pPipeCompressedBufferObj);
   pD3D12Dec->base.context->buffer_subdata(pD3D12Dec->base.context,                      // context
                                          // Flush buffer_subdata batch
            pD3D12Dec->base.context->flush(pD3D12Dec->base.context,
               assert(inFlightResources.m_pBitstreamUploadGPUCompletionFence);
            ///
   /// Proceed to record the GPU Decode commands
            // Requested conversions by caller upper layer (none for now)
            ///
   /// Record DecodeFrame operation and resource state transitions.
            // Translate input D3D12 structure
            d3d12InputArguments.CompressedBitstream.pBuffer = inFlightResources.m_curFrameCompressedBitstreamBuffer.Get();
   d3d12InputArguments.CompressedBitstream.Offset = 0u;
   ASSERTED constexpr uint64_t d3d12BitstreamOffsetAlignment =
      128u;   // specified in
      assert((d3d12InputArguments.CompressedBitstream.Offset == 0) ||
                  D3D12_RESOURCE_BARRIER resourceBarrierCommonToDecode[1] = {
      CD3DX12_RESOURCE_BARRIER::Transition(d3d12InputArguments.CompressedBitstream.pBuffer,
            };
            // Schedule reverse (back to common) transitions before command list closes for current frame
   pD3D12Dec->m_transitionsBeforeCloseCmdList.push_back(
      CD3DX12_RESOURCE_BARRIER::Transition(d3d12InputArguments.CompressedBitstream.pBuffer,
               ///
   /// Clear texture (no reference only flags in resource allocation) to use as decode output to send downstream for
   /// display/consumption
   ///
   ID3D12Resource *pOutputD3D12Texture;
            ///
   /// Ref Only texture (with reference only flags in resource allocation) to use as reconstructed picture decode output
   /// and to store as future reference in DPB
   ///
   ID3D12Resource *pRefOnlyOutputD3D12Texture;
            if (!d3d12_video_decoder_prepare_for_decode_frame(pD3D12Dec,
                                                debug_printf("[d3d12_video_decoder] d3d12_video_decoder_end_frame - Failure on "
         debug_printf("[d3d12_video_decoder] d3d12_video_decoder_end_frame failed for fenceValue: %d\n",
         assert(false);
               ///
   /// Set codec picture parameters CPU buffer
            d3d12InputArguments.NumFrameArguments =
         d3d12InputArguments.FrameArguments[d3d12InputArguments.NumFrameArguments - 1] = {
      D3D12_VIDEO_DECODE_ARGUMENT_TYPE_PICTURE_PARAMETERS,
   static_cast<uint32_t>(inFlightResources.m_picParamsBuffer.size()),
               if (inFlightResources.m_SliceControlBuffer.size() > 0) {
      d3d12InputArguments.NumFrameArguments++;
   d3d12InputArguments.FrameArguments[d3d12InputArguments.NumFrameArguments - 1] = {
      D3D12_VIDEO_DECODE_ARGUMENT_TYPE_SLICE_CONTROL,
   static_cast<uint32_t>(inFlightResources.m_SliceControlBuffer.size()),
                  if (inFlightResources.qp_matrix_frame_argument_enabled &&
      (inFlightResources.m_InverseQuantMatrixBuffer.size() > 0)) {
   d3d12InputArguments.NumFrameArguments++;
   d3d12InputArguments.FrameArguments[d3d12InputArguments.NumFrameArguments - 1] = {
      D3D12_VIDEO_DECODE_ARGUMENT_TYPE_INVERSE_QUANTIZATION_MATRIX,
   static_cast<uint32_t>(inFlightResources.m_InverseQuantMatrixBuffer.size()),
                  d3d12InputArguments.ReferenceFrames = pD3D12Dec->m_spDPBManager->get_current_reference_frames();
   if (D3D12_DEBUG_VERBOSE & d3d12_debug) {
                           // translate output D3D12 structure
   D3D12_VIDEO_DECODE_OUTPUT_STREAM_ARGUMENTS1 d3d12OutputArguments = {};
   d3d12OutputArguments.pOutputTexture2D = pOutputD3D12Texture;
            bool fReferenceOnly = (pD3D12Dec->m_ConfigDecoderSpecificFlags &
         if (fReferenceOnly) {
               assert(pRefOnlyOutputD3D12Texture);
   d3d12OutputArguments.ConversionArguments.pReferenceTexture2D = pRefOnlyOutputD3D12Texture;
            const D3D12_RESOURCE_DESC &descReference = GetDesc(d3d12OutputArguments.ConversionArguments.pReferenceTexture2D);
   d3d12OutputArguments.ConversionArguments.DecodeColorSpace = d3d12_convert_from_legacy_color_space(
      !util_format_is_yuv(d3d12_get_pipe_format(descReference.Format)),
   util_format_get_blocksize(d3d12_get_pipe_format(descReference.Format)) * 8 /*bytes to bits conversion*/,
   /* StudioRGB= */ false,
               const D3D12_RESOURCE_DESC &descOutput = GetDesc(d3d12OutputArguments.pOutputTexture2D);
   d3d12OutputArguments.ConversionArguments.OutputColorSpace = d3d12_convert_from_legacy_color_space(
      !util_format_is_yuv(d3d12_get_pipe_format(descOutput.Format)),
   util_format_get_blocksize(d3d12_get_pipe_format(descOutput.Format)) * 8 /*bytes to bits conversion*/,
   /* StudioRGB= */ false,
               const D3D12_VIDEO_DECODER_HEAP_DESC &HeapDesc = GetDesc(pD3D12Dec->m_spVideoDecoderHeap.Get());
   d3d12OutputArguments.ConversionArguments.OutputWidth = HeapDesc.DecodeWidth;
      } else {
                  CD3DX12_RESOURCE_DESC outputDesc(GetDesc(d3d12OutputArguments.pOutputTexture2D));
   uint32_t MipLevel, PlaneSlice, ArraySlice;
   D3D12DecomposeSubresource(d3d12OutputArguments.OutputSubresource,
                                    for (PlaneSlice = 0; PlaneSlice < pD3D12Dec->m_decodeFormatInfo.PlaneCount; PlaneSlice++) {
               D3D12_RESOURCE_BARRIER resourceBarrierCommonToDecode[1] = {
      CD3DX12_RESOURCE_BARRIER::Transition(d3d12OutputArguments.pOutputTexture2D,
                  };
               // Schedule reverse (back to common) transitions before command list closes for current frame
   for (PlaneSlice = 0; PlaneSlice < pD3D12Dec->m_decodeFormatInfo.PlaneCount; PlaneSlice++) {
      uint planeOutputSubresource = outputDesc.CalcSubresource(MipLevel, ArraySlice, PlaneSlice);
   pD3D12Dec->m_transitionsBeforeCloseCmdList.push_back(
      CD3DX12_RESOURCE_BARRIER::Transition(d3d12OutputArguments.pOutputTexture2D,
                                 pD3D12Dec->m_spDecodeCommandList->DecodeFrame1(pD3D12Dec->m_spVideoDecoder.Get(),
                  debug_printf("[d3d12_video_decoder] d3d12_video_decoder_end_frame finalized for fenceValue: %d\n",
            // Save extra references of Decoder, DecoderHeap and DPB allocations in case
   // there's a reconfiguration that trigers the construction of new objects
   inFlightResources.m_spDecoder = pD3D12Dec->m_spVideoDecoder;
   inFlightResources.m_spDecoderHeap = pD3D12Dec->m_spVideoDecoderHeap;
            ///
   /// Flush work to the GPU
   ///
   pD3D12Dec->m_needsGPUFlush = true;
   d3d12_video_decoder_flush(codec);
   // Call to d3d12_video_decoder_flush increases m_FenceValue
            if (pD3D12Dec->m_spDPBManager->is_pipe_buffer_underlying_output_decode_allocation()) {
      // No need to copy, the output surface fence is merely the decode queue fence
      } else {
      ///
   /// If !pD3D12Dec->m_spDPBManager->is_pipe_buffer_underlying_output_decode_allocation()
   /// We cannot use the standalone video buffer allocation directly and we must use instead
   /// either a ID3D12Resource with DECODE_REFERENCE only flag or a texture array within the same
   /// allocation
   /// Do GPU->GPU texture copy from decode output to pipe target decode texture sampler view planes
            // Get destination resource
            // Get source pipe_resource
   pipe_resource *pPipeSrc =
                  // GPU wait on the graphics context which will do the copy until the decode finishes
   pD3D12Screen->cmdqueue->Wait(
                  // Copy all format subresources/texture planes
   for (PlaneSlice = 0; PlaneSlice < pD3D12Dec->m_decodeFormatInfo.PlaneCount; PlaneSlice++) {
      assert(d3d12OutputArguments.OutputSubresource < INT16_MAX);
   struct pipe_box box = { 0,
                                          pD3D12Dec->base.context->resource_copy_region(pD3D12Dec->base.context,
                                                }
   // Flush resource_copy_region batch
   // The output surface fence is the graphics queue that will signal after the copy ends
         }
      /**
   * Get decoder fence.
   */
   int
   d3d12_video_decoder_get_decoder_fence(struct pipe_video_codec *codec, struct pipe_fence_handle *fence, uint64_t timeout)
   {
      struct d3d12_fence *fenceValueToWaitOn = (struct d3d12_fence *) fence;
            ASSERTED bool wait_res =
            // Return semantics based on p_video_codec interface
   // ret == 0 -> Decode in progress
   // ret != 0 -> Decode completed
      }
      /**
   * flush any outstanding command buffers to the hardware
   * should be called before a video_buffer is acessed by the gallium frontend again
   */
   void
   d3d12_video_decoder_flush(struct pipe_video_codec *codec)
   {
      struct d3d12_video_decoder *pD3D12Dec = (struct d3d12_video_decoder *) codec;
   assert(pD3D12Dec);
   assert(pD3D12Dec->m_spD3D12VideoDevice);
   assert(pD3D12Dec->m_spDecodeCommandQueue);
   debug_printf("[d3d12_video_decoder] d3d12_video_decoder_flush started. Will flush video queue work and CPU wait on "
                  if (!pD3D12Dec->m_needsGPUFlush) {
         } else {
      HRESULT hr = pD3D12Dec->m_pD3D12Screen->dev->GetDeviceRemovedReason();
   if (hr != S_OK) {
      debug_printf("[d3d12_video_decoder] d3d12_video_decoder_flush"
               " - D3D12Device was removed BEFORE commandlist "
               if (pD3D12Dec->m_transitionsBeforeCloseCmdList.size() > 0) {
      pD3D12Dec->m_spDecodeCommandList->ResourceBarrier(pD3D12Dec->m_transitionsBeforeCloseCmdList.size(),
                     hr = pD3D12Dec->m_spDecodeCommandList->Close();
   if (FAILED(hr)) {
      debug_printf("[d3d12_video_decoder] d3d12_video_decoder_flush - Can't close command list with HR %x\n", hr);
               auto &inFlightResources = pD3D12Dec->m_inflightResourcesPool[d3d12_video_decoder_pool_current_index(pD3D12Dec)];
   ID3D12CommandList *ppCommandLists[1] = { pD3D12Dec->m_spDecodeCommandList.Get() };
   struct d3d12_fence *pUploadBitstreamFence = d3d12_fence(inFlightResources.m_pBitstreamUploadGPUCompletionFence);
   pD3D12Dec->m_spDecodeCommandQueue->Wait(pUploadBitstreamFence->cmdqueue_fence, pUploadBitstreamFence->value);
   pD3D12Dec->m_spDecodeCommandQueue->ExecuteCommandLists(1, ppCommandLists);
            // Validate device was not removed
   hr = pD3D12Dec->m_pD3D12Screen->dev->GetDeviceRemovedReason();
   if (hr != S_OK) {
      debug_printf("[d3d12_video_decoder] d3d12_video_decoder_flush"
               " - D3D12Device was removed AFTER commandlist "
               // Set async fence info
            inFlightResources.m_FenceData.value = pD3D12Dec->m_fenceValue;
            pD3D12Dec->m_fenceValue++;
      }
         flush_fail:
      debug_printf("[d3d12_video_decoder] d3d12_video_decoder_flush failed for fenceValue: %d\n", pD3D12Dec->m_fenceValue);
      }
      bool
   d3d12_video_decoder_create_command_objects(const struct d3d12_screen *pD3D12Screen,
         {
               D3D12_COMMAND_QUEUE_DESC commandQueueDesc = { D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE };
   HRESULT hr = pD3D12Screen->dev->CreateCommandQueue(&commandQueueDesc,
         if (FAILED(hr)) {
      debug_printf("[d3d12_video_decoder] d3d12_video_decoder_create_command_objects - Call to CreateCommandQueue "
                           hr = pD3D12Screen->dev->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pD3D12Dec->m_spFence));
   if (FAILED(hr)) {
      debug_printf(
      "[d3d12_video_decoder] d3d12_video_decoder_create_command_objects - Call to CreateFence failed with HR %x\n",
                  for (auto &inputResource : pD3D12Dec->m_inflightResourcesPool) {
      hr = pD3D12Dec->m_pD3D12Screen->dev->CreateCommandAllocator(
      D3D12_COMMAND_LIST_TYPE_VIDEO_DECODE,
      if (FAILED(hr)) {
      debug_printf("[d3d12_video_decoder] d3d12_video_decoder_create_command_objects - Call to "
                              ComPtr<ID3D12Device4> spD3D12Device4;
   if (FAILED(pD3D12Dec->m_pD3D12Screen->dev->QueryInterface(IID_PPV_ARGS(spD3D12Device4.GetAddressOf())))) {
      debug_printf(
                     hr = spD3D12Device4->CreateCommandList1(0,
                        if (FAILED(hr)) {
      debug_printf("[d3d12_video_decoder] d3d12_video_decoder_create_command_objects - Call to CreateCommandList "
                              }
      bool
   d3d12_video_decoder_check_caps_and_create_decoder(const struct d3d12_screen *pD3D12Screen,
         {
                        D3D12_VIDEO_DECODE_CONFIGURATION decodeConfiguration = { pD3D12Dec->m_d3d12DecProfile,
                  D3D12_FEATURE_DATA_VIDEO_DECODE_SUPPORT decodeSupport = {};
   decodeSupport.NodeIndex = pD3D12Dec->m_NodeIndex;
   decodeSupport.Configuration = decodeConfiguration;
   decodeSupport.Width = pD3D12Dec->base.width;
   decodeSupport.Height = pD3D12Dec->base.height;
   decodeSupport.DecodeFormat = pD3D12Dec->m_decodeFormat;
   // no info from above layer on framerate/bitrate
   decodeSupport.FrameRate.Numerator = 0;
   decodeSupport.FrameRate.Denominator = 0;
            HRESULT hr = pD3D12Dec->m_spD3D12VideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_DECODE_SUPPORT,
               if (FAILED(hr)) {
      debug_printf("[d3d12_video_decoder] d3d12_video_decoder_check_caps_and_create_decoder - CheckFeatureSupport "
                           if (!(decodeSupport.SupportFlags & D3D12_VIDEO_DECODE_SUPPORT_FLAG_SUPPORTED)) {
      debug_printf("[d3d12_video_decoder] d3d12_video_decoder_check_caps_and_create_decoder - "
                     pD3D12Dec->m_configurationFlags = decodeSupport.ConfigurationFlags;
            if (d3d12_video_decoder_supports_aot_dpb(decodeSupport, pD3D12Dec->m_d3d12DecProfileType)) {
                  if (decodeSupport.ConfigurationFlags & D3D12_VIDEO_DECODE_CONFIGURATION_FLAG_HEIGHT_ALIGNMENT_MULTIPLE_32_REQUIRED) {
                  if (decodeSupport.ConfigurationFlags & D3D12_VIDEO_DECODE_CONFIGURATION_FLAG_REFERENCE_ONLY_ALLOCATIONS_REQUIRED) {
      pD3D12Dec->m_ConfigDecoderSpecificFlags |=
               pD3D12Dec->m_decoderDesc.NodeMask = pD3D12Dec->m_NodeMask;
            hr = pD3D12Dec->m_spD3D12VideoDevice->CreateVideoDecoder(&pD3D12Dec->m_decoderDesc,
         if (FAILED(hr)) {
      debug_printf("[d3d12_video_decoder] d3d12_video_decoder_check_caps_and_create_decoder - CreateVideoDecoder "
                              }
      bool
   d3d12_video_decoder_create_video_state_buffers(const struct d3d12_screen *pD3D12Screen,
         {
      assert(pD3D12Dec->m_spD3D12VideoDevice);
   if (!d3d12_video_decoder_create_staging_bitstream_buffer(pD3D12Screen,
                  debug_printf("[d3d12_video_decoder] d3d12_video_decoder_create_video_state_buffers - Failure on "
                        }
      bool
   d3d12_video_decoder_create_staging_bitstream_buffer(const struct d3d12_screen *pD3D12Screen,
               {
      assert(pD3D12Dec->m_spD3D12VideoDevice);
   auto &inFlightResources = pD3D12Dec->m_inflightResourcesPool[d3d12_video_decoder_pool_current_index(pD3D12Dec)];
   if (inFlightResources.m_curFrameCompressedBitstreamBuffer.Get() != nullptr) {
                  auto descHeap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT, pD3D12Dec->m_NodeMask, pD3D12Dec->m_NodeMask);
   auto descResource = CD3DX12_RESOURCE_DESC::Buffer(bufSize);
   HRESULT hr = pD3D12Screen->dev->CreateCommittedResource(
      &descHeap,
   D3D12_HEAP_FLAG_NONE,
   &descResource,
   D3D12_RESOURCE_STATE_COMMON,
   nullptr,
      if (FAILED(hr)) {
      debug_printf("[d3d12_video_decoder] d3d12_video_decoder_create_staging_bitstream_buffer - "
                           inFlightResources.m_curFrameCompressedBitstreamBufferAllocatedSize = bufSize;
      }
      bool
   d3d12_video_decoder_prepare_for_decode_frame(struct d3d12_video_decoder *pD3D12Dec,
                                             {
      if (!d3d12_video_decoder_reconfigure_dpb(pD3D12Dec, pD3D12VideoBuffer, conversionArgs)) {
      debug_printf("d3d12_video_decoder_reconfigure_dpb failed!\n");
               // Refresh DPB active references for current frame, release memory for unused references.
            // Get the output texture for the current frame to be decoded
   pD3D12Dec->m_spDPBManager->get_current_frame_decode_output_texture(pCurrentDecodeTarget,
                  auto vidBuffer = (struct d3d12_video_buffer *) (pCurrentDecodeTarget);
   // If is_pipe_buffer_underlying_output_decode_allocation is enabled,
   // we can just use the underlying allocation in pCurrentDecodeTarget
   // and avoid an extra copy after decoding the frame.
   // If this is the case, we need to handle the residency of this resource
   // (if not we're actually creating the resources with CreateCommitedResource with
   // residency by default)
   if (pD3D12Dec->m_spDPBManager->is_pipe_buffer_underlying_output_decode_allocation()) {
      assert(d3d12_resource_resource(vidBuffer->texture) == *ppOutTexture2D);
   // Make it permanently resident for video use
               // Get the reference only texture for the current frame to be decoded (if applicable)
   bool fReferenceOnly = (pD3D12Dec->m_ConfigDecoderSpecificFlags &
         if (fReferenceOnly) {
      bool needsTransitionToDecodeWrite = false;
   pD3D12Dec->m_spDPBManager->get_reference_only_output(pCurrentDecodeTarget,
                              CD3DX12_RESOURCE_DESC outputDesc(GetDesc(*ppRefOnlyOutTexture2D));
   uint32_t MipLevel, PlaneSlice, ArraySlice;
   D3D12DecomposeSubresource(*pRefOnlyOutSubresourceIndex,
                                    for (PlaneSlice = 0; PlaneSlice < pD3D12Dec->m_decodeFormatInfo.PlaneCount; PlaneSlice++) {
               D3D12_RESOURCE_BARRIER resourceBarrierCommonToDecode[1] = {
      CD3DX12_RESOURCE_BARRIER::Transition(*ppRefOnlyOutTexture2D,
                  };
               // Schedule reverse (back to common) transitions before command list closes for current frame
   for (PlaneSlice = 0; PlaneSlice < pD3D12Dec->m_decodeFormatInfo.PlaneCount; PlaneSlice++) {
      uint planeOutputSubresource = outputDesc.CalcSubresource(MipLevel, ArraySlice, PlaneSlice);
   pD3D12Dec->m_transitionsBeforeCloseCmdList.push_back(
      CD3DX12_RESOURCE_BARRIER::Transition(*ppRefOnlyOutTexture2D,
                           // If decoded needs reference_only entries in the dpb, use the reference_only allocation for current frame
   // otherwise, use the standard output resource
   ID3D12Resource *pCurrentFrameDPBEntry = fReferenceOnly ? *ppRefOnlyOutTexture2D : *ppOutTexture2D;
            switch (pD3D12Dec->m_d3d12DecProfileType) {
      case d3d12_video_decode_profile_type_h264:
   {
      d3d12_video_decoder_prepare_current_frame_references_h264(pD3D12Dec,
                     case d3d12_video_decode_profile_type_hevc:
   {
      d3d12_video_decoder_prepare_current_frame_references_hevc(pD3D12Dec,
                     case d3d12_video_decode_profile_type_av1:
   {
      d3d12_video_decoder_prepare_current_frame_references_av1(pD3D12Dec,
                     case d3d12_video_decode_profile_type_vp9:
   {
      d3d12_video_decoder_prepare_current_frame_references_vp9(pD3D12Dec,
                     default:
   {
                        }
      bool
   d3d12_video_decoder_reconfigure_dpb(struct d3d12_video_decoder *pD3D12Dec,
               {
      uint32_t width;
   uint32_t height;
   uint16_t maxDPB;
   bool isInterlaced;
            ID3D12Resource *pPipeD3D12DstResource = d3d12_resource_resource(pD3D12VideoBuffer->texture);
            assert(pD3D12VideoBuffer->base.interlaced == isInterlaced);
   D3D12_VIDEO_FRAME_CODED_INTERLACE_TYPE interlaceTypeRequested =
         if ((pD3D12Dec->m_decodeFormat != outputResourceDesc.Format) ||
      (pD3D12Dec->m_decoderDesc.Configuration.InterlaceType != interlaceTypeRequested)) {
   // Copy current pD3D12Dec->m_decoderDesc, modify decodeprofile and re-create decoder.
   D3D12_VIDEO_DECODER_DESC decoderDesc = pD3D12Dec->m_decoderDesc;
   decoderDesc.Configuration.InterlaceType = interlaceTypeRequested;
   decoderDesc.Configuration.DecodeProfile =
         pD3D12Dec->m_spVideoDecoder.Reset();
   HRESULT hr =
      pD3D12Dec->m_spD3D12VideoDevice->CreateVideoDecoder(&decoderDesc,
      if (FAILED(hr)) {
      debug_printf(
      "[d3d12_video_decoder] d3d12_video_decoder_reconfigure_dpb - CreateVideoDecoder failed with HR %x\n",
         }
   // Update state after CreateVideoDecoder succeeds only.
               if (!pD3D12Dec->m_spDPBManager || !pD3D12Dec->m_spVideoDecoderHeap ||
      pD3D12Dec->m_decodeFormat != outputResourceDesc.Format || pD3D12Dec->m_decoderHeapDesc.DecodeWidth != width ||
   pD3D12Dec->m_decoderHeapDesc.DecodeHeight != height ||
   pD3D12Dec->m_decoderHeapDesc.MaxDecodePictureBufferCount < maxDPB) {
   // Detect the combination of AOT/ReferenceOnly to configure the DPB manager
   uint16_t referenceCount = (conversionArguments.Enable) ? (uint16_t) conversionArguments.ReferenceFrameCount +
               d3d12_video_decode_dpb_descriptor dpbDesc = {};
   dpbDesc.Width = (conversionArguments.Enable) ? conversionArguments.ReferenceInfo.Width : width;
   dpbDesc.Height = (conversionArguments.Enable) ? conversionArguments.ReferenceInfo.Height : height;
   dpbDesc.Format =
         dpbDesc.fArrayOfTexture =
         dpbDesc.dpbSize = referenceCount;
   dpbDesc.m_NodeMask = pD3D12Dec->m_NodeMask;
   dpbDesc.fReferenceOnly = ((pD3D12Dec->m_ConfigDecoderSpecificFlags &
            // Create DPB manager
   if (pD3D12Dec->m_spDPBManager == nullptr) {
      pD3D12Dec->m_spDPBManager.reset(new d3d12_video_decoder_references_manager(pD3D12Dec->m_pD3D12Screen,
                           //
   // (Re)-create decoder heap
   //
   D3D12_VIDEO_DECODER_HEAP_DESC decoderHeapDesc = {};
   decoderHeapDesc.NodeMask = pD3D12Dec->m_NodeMask;
   decoderHeapDesc.Configuration = pD3D12Dec->m_decoderDesc.Configuration;
   decoderHeapDesc.DecodeWidth = dpbDesc.Width;
   decoderHeapDesc.DecodeHeight = dpbDesc.Height;
   decoderHeapDesc.Format = dpbDesc.Format;
   decoderHeapDesc.MaxDecodePictureBufferCount = maxDPB;
   pD3D12Dec->m_spVideoDecoderHeap.Reset();
   HRESULT hr = pD3D12Dec->m_spD3D12VideoDevice->CreateVideoDecoderHeap(
      &decoderHeapDesc,
      if (FAILED(hr)) {
      debug_printf(
      "[d3d12_video_decoder] d3d12_video_decoder_reconfigure_dpb - CreateVideoDecoderHeap failed with HR %x\n",
         }
   // Update pD3D12Dec after CreateVideoDecoderHeap succeeds only.
                           }
      void
   d3d12_video_decoder_refresh_dpb_active_references(struct d3d12_video_decoder *pD3D12Dec)
   {
      switch (pD3D12Dec->m_d3d12DecProfileType) {
      case d3d12_video_decode_profile_type_h264:
   {
                  case d3d12_video_decode_profile_type_hevc:
   {
                  case d3d12_video_decode_profile_type_av1:
   {
                  case d3d12_video_decode_profile_type_vp9:
   {
                  default:
   {
               }
      void
   d3d12_video_decoder_get_frame_info(
         {
      *pWidth = 0;
   *pHeight = 0;
   *pMaxDPB = 0;
            switch (pD3D12Dec->m_d3d12DecProfileType) {
      case d3d12_video_decode_profile_type_h264:
   {
                  case d3d12_video_decode_profile_type_hevc:
   {
                  case d3d12_video_decode_profile_type_av1:
   {
                  case d3d12_video_decode_profile_type_vp9:
   {
                  default:
   {
                     if (pD3D12Dec->m_ConfigDecoderSpecificFlags & d3d12_video_decode_config_specific_flag_alignment_height) {
      const uint32_t AlignmentMask = 31;
         }
      void
   d3d12_video_decoder_store_converted_dxva_picparams_from_pipe_input(
      struct d3d12_video_decoder *codec,   // input argument, current decoder
   struct pipe_picture_desc
            )
   {
      assert(picture);
   assert(codec);
            d3d12_video_decode_profile_type profileType =
         ID3D12Resource *pPipeD3D12DstResource = d3d12_resource_resource(pD3D12VideoBuffer->texture);
   D3D12_RESOURCE_DESC outputResourceDesc = GetDesc(pPipeD3D12DstResource);
   auto &inFlightResources = pD3D12Dec->m_inflightResourcesPool[d3d12_video_decoder_pool_current_index(pD3D12Dec)];
   inFlightResources.qp_matrix_frame_argument_enabled = false;
   switch (profileType) {
      case d3d12_video_decode_profile_type_h264:
   {
      size_t dxvaPicParamsBufferSize = sizeof(DXVA_PicParams_H264);
   pipe_h264_picture_desc *pPicControlH264 = (pipe_h264_picture_desc *) picture;
   DXVA_PicParams_H264 dxvaPicParamsH264 =
      d3d12_video_decoder_dxva_picparams_from_pipe_picparams_h264(pD3D12Dec->m_fenceValue,
                           d3d12_video_decoder_store_dxva_picparams_in_picparams_buffer(codec,
                  size_t dxvaQMatrixBufferSize = sizeof(DXVA_Qmatrix_H264);
   DXVA_Qmatrix_H264 dxvaQmatrixH264 = {};
   d3d12_video_decoder_dxva_qmatrix_from_pipe_picparams_h264((pipe_h264_picture_desc *) picture, dxvaQmatrixH264);
   inFlightResources.qp_matrix_frame_argument_enabled =
                     case d3d12_video_decode_profile_type_hevc:
   {
      size_t dxvaPicParamsBufferSize = sizeof(DXVA_PicParams_HEVC);
   pipe_h265_picture_desc *pPicControlHEVC = (pipe_h265_picture_desc *) picture;
                  d3d12_video_decoder_store_dxva_picparams_in_picparams_buffer(codec,
                  size_t dxvaQMatrixBufferSize = sizeof(DXVA_Qmatrix_HEVC);
   DXVA_Qmatrix_HEVC dxvaQmatrixHEVC = {};
   inFlightResources.qp_matrix_frame_argument_enabled = false;
   d3d12_video_decoder_dxva_qmatrix_from_pipe_picparams_hevc((pipe_h265_picture_desc *) picture,
                  } break;
   case d3d12_video_decode_profile_type_av1:
   {
      size_t dxvaPicParamsBufferSize = sizeof(DXVA_PicParams_AV1);
   pipe_av1_picture_desc *pPicControlAV1 = (pipe_av1_picture_desc *) picture;
   DXVA_PicParams_AV1 dxvaPicParamsAV1 =
      d3d12_video_decoder_dxva_picparams_from_pipe_picparams_av1(pD3D12Dec->m_fenceValue,
               d3d12_video_decoder_store_dxva_picparams_in_picparams_buffer(codec, &dxvaPicParamsAV1, dxvaPicParamsBufferSize);
      } break;
   case d3d12_video_decode_profile_type_vp9:
   {
      size_t dxvaPicParamsBufferSize = sizeof(DXVA_PicParams_VP9);
   pipe_vp9_picture_desc *pPicControlVP9 = (pipe_vp9_picture_desc *) picture;
                  d3d12_video_decoder_store_dxva_picparams_in_picparams_buffer(codec, &dxvaPicParamsVP9, dxvaPicParamsBufferSize);
      } break;
   default:
   {
               }
      void
   d3d12_video_decoder_prepare_dxva_slices_control(
      struct d3d12_video_decoder *pD3D12Dec,   // input argument, current decoder
      {
      auto &inFlightResources = pD3D12Dec->m_inflightResourcesPool[d3d12_video_decoder_pool_current_index(pD3D12Dec)];
   d3d12_video_decode_profile_type profileType =
         switch (profileType) {
      case d3d12_video_decode_profile_type_h264:
   {
      d3d12_video_decoder_prepare_dxva_slices_control_h264(pD3D12Dec,
                     case d3d12_video_decode_profile_type_hevc:
   {
      d3d12_video_decoder_prepare_dxva_slices_control_hevc(pD3D12Dec,
            } break;
   case d3d12_video_decode_profile_type_av1:
   {
      d3d12_video_decoder_prepare_dxva_slices_control_av1(pD3D12Dec,
            } break;
   case d3d12_video_decode_profile_type_vp9:
   {
      d3d12_video_decoder_prepare_dxva_slices_control_vp9(pD3D12Dec,
                     default:
   {
               }
      void
   d3d12_video_decoder_store_dxva_qmatrix_in_qmatrix_buffer(struct d3d12_video_decoder *pD3D12Dec,
               {
      auto &inFlightResources = pD3D12Dec->m_inflightResourcesPool[d3d12_video_decoder_pool_current_index(pD3D12Dec)];
   if (inFlightResources.m_InverseQuantMatrixBuffer.capacity() < DXVAStructSize) {
                  inFlightResources.m_InverseQuantMatrixBuffer.resize(DXVAStructSize);
      }
      void
   d3d12_video_decoder_store_dxva_picparams_in_picparams_buffer(struct d3d12_video_decoder *pD3D12Dec,
               {
      auto &inFlightResources = pD3D12Dec->m_inflightResourcesPool[d3d12_video_decoder_pool_current_index(pD3D12Dec)];
   if (inFlightResources.m_picParamsBuffer.capacity() < DXVAStructSize) {
                  inFlightResources.m_picParamsBuffer.resize(DXVAStructSize);
      }
      bool
   d3d12_video_decoder_supports_aot_dpb(D3D12_FEATURE_DATA_VIDEO_DECODE_SUPPORT decodeSupport,
         {
      bool supportedProfile = false;
   switch (profileType) {
      case d3d12_video_decode_profile_type_h264:
   case d3d12_video_decode_profile_type_hevc:
   case d3d12_video_decode_profile_type_av1:
   case d3d12_video_decode_profile_type_vp9:
      supportedProfile = true;
      default:
      supportedProfile = false;
               }
      d3d12_video_decode_profile_type
   d3d12_video_decoder_convert_pipe_video_profile_to_profile_type(enum pipe_video_profile profile)
   {
      switch (profile) {
      case PIPE_VIDEO_PROFILE_MPEG4_AVC_BASELINE:
   case PIPE_VIDEO_PROFILE_MPEG4_AVC_CONSTRAINED_BASELINE:
   case PIPE_VIDEO_PROFILE_MPEG4_AVC_MAIN:
   case PIPE_VIDEO_PROFILE_MPEG4_AVC_EXTENDED:
   case PIPE_VIDEO_PROFILE_MPEG4_AVC_HIGH:
   case PIPE_VIDEO_PROFILE_MPEG4_AVC_HIGH10:
         case PIPE_VIDEO_PROFILE_HEVC_MAIN:
   case PIPE_VIDEO_PROFILE_HEVC_MAIN_10:
         case PIPE_VIDEO_PROFILE_AV1_MAIN:
         case PIPE_VIDEO_PROFILE_VP9_PROFILE0:
   case PIPE_VIDEO_PROFILE_VP9_PROFILE2:
         default:
   {
               }
      GUID
   d3d12_video_decoder_convert_pipe_video_profile_to_d3d12_profile(enum pipe_video_profile profile)
   {
      switch (profile) {
      case PIPE_VIDEO_PROFILE_MPEG4_AVC_BASELINE:
   case PIPE_VIDEO_PROFILE_MPEG4_AVC_CONSTRAINED_BASELINE:
   case PIPE_VIDEO_PROFILE_MPEG4_AVC_MAIN:
   case PIPE_VIDEO_PROFILE_MPEG4_AVC_EXTENDED:
   case PIPE_VIDEO_PROFILE_MPEG4_AVC_HIGH:
   case PIPE_VIDEO_PROFILE_MPEG4_AVC_HIGH10:
         case PIPE_VIDEO_PROFILE_HEVC_MAIN:
         case PIPE_VIDEO_PROFILE_HEVC_MAIN_10:
         case PIPE_VIDEO_PROFILE_AV1_MAIN:
         case PIPE_VIDEO_PROFILE_VP9_PROFILE0:
         case PIPE_VIDEO_PROFILE_VP9_PROFILE2:
         default:
         }
      GUID
   d3d12_video_decoder_resolve_profile(d3d12_video_decode_profile_type profileType, DXGI_FORMAT decode_format)
   {
      switch (profileType) {
      case d3d12_video_decode_profile_type_h264:
         case d3d12_video_decode_profile_type_hevc:
   {
      switch (decode_format) {
      case DXGI_FORMAT_NV12:
         case DXGI_FORMAT_P010:
         default:
   {
               } break;
   case d3d12_video_decode_profile_type_av1:
      return D3D12_VIDEO_DECODE_PROFILE_AV1_PROFILE0;
      case d3d12_video_decode_profile_type_vp9:
   {
      switch (decode_format) {
      case DXGI_FORMAT_NV12:
         case DXGI_FORMAT_P010:
         default:
   {
               } break;
   default:
   {
               }
      bool
   d3d12_video_decoder_ensure_fence_finished(struct pipe_video_codec *codec,
                     {
      bool wait_result = true;
   HRESULT hr = S_OK;
            debug_printf(
      "[d3d12_video_decoder] d3d12_video_decoder_ensure_fence_finished - Waiting for fence (with timeout_ns %" PRIu64
   ") to finish with "
   "fenceValue: %" PRIu64 " - Current Fence Completed Value %" PRIu64 "\n",
   timeout_ns,
   fenceValueToWaitOn,
                     HANDLE event = {};
   int event_fd = 0;
            hr = fence->SetEventOnCompletion(fenceValueToWaitOn, event);
   if (FAILED(hr)) {
      debug_printf("[d3d12_video_decoder] d3d12_video_decoder_ensure_fence_finished - SetEventOnCompletion for "
               "fenceValue %" PRIu64 " failed with HR %x\n",
               wait_result = d3d12_fence_wait_event(event, event_fd, timeout_ns);
            debug_printf("[d3d12_video_decoder] d3d12_video_decoder_ensure_fence_finished - Waiting on fence to be done with "
                  } else {
      debug_printf("[d3d12_video_decoder] d3d12_video_decoder_ensure_fence_finished - Fence already done with "
                  }
      }
      bool
   d3d12_video_decoder_sync_completion(struct pipe_video_codec *codec,
                     {
      struct d3d12_video_decoder *pD3D12Dec = (struct d3d12_video_decoder *) codec;
   assert(pD3D12Dec);
   assert(pD3D12Dec->m_spD3D12VideoDevice);
   assert(pD3D12Dec->m_spDecodeCommandQueue);
            ASSERTED bool wait_result = d3d12_video_decoder_ensure_fence_finished(codec, fence, fenceValueToWaitOn, timeout_ns);
            // Release references granted on end_frame for this inflight operations
   pD3D12Dec->m_inflightResourcesPool[fenceValueToWaitOn % D3D12_VIDEO_DEC_ASYNC_DEPTH].m_spDecoder.Reset();
   pD3D12Dec->m_inflightResourcesPool[fenceValueToWaitOn % D3D12_VIDEO_DEC_ASYNC_DEPTH].m_spDecoderHeap.Reset();
   pD3D12Dec->m_inflightResourcesPool[fenceValueToWaitOn % D3D12_VIDEO_DEC_ASYNC_DEPTH].m_References.reset();
   pD3D12Dec->m_inflightResourcesPool[fenceValueToWaitOn % D3D12_VIDEO_DEC_ASYNC_DEPTH].m_stagingDecodeBitstream.resize(
         pipe_resource_reference(
      &pD3D12Dec->m_inflightResourcesPool[fenceValueToWaitOn % D3D12_VIDEO_DEC_ASYNC_DEPTH].pPipeCompressedBufferObj,
         struct d3d12_screen *pD3D12Screen = (struct d3d12_screen *) pD3D12Dec->m_pD3D12Screen;
            pD3D12Screen->base.fence_reference(
      &pD3D12Screen->base,
   &pD3D12Dec->m_inflightResourcesPool[d3d12_video_decoder_pool_current_index(pD3D12Dec)]
               hr =
         if (FAILED(hr)) {
      debug_printf("failed with %x.\n", hr);
               // Validate device was not removed
   hr = pD3D12Dec->m_pD3D12Screen->dev->GetDeviceRemovedReason();
   if (hr != S_OK) {
      debug_printf("[d3d12_video_decoder] d3d12_video_decoder_sync_completion"
               " - D3D12Device was removed AFTER d3d12_video_decoder_ensure_fence_finished "
               debug_printf(
      "[d3d12_video_decoder] d3d12_video_decoder_sync_completion - GPU execution finalized for fenceValue: %" PRIu64
   "\n",
               sync_with_token_fail:
      debug_printf("[d3d12_video_decoder] d3d12_video_decoder_sync_completion failed for fenceValue: %" PRIu64 "\n",
         assert(false);
      }