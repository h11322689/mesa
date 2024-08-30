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
   #include "d3d12_screen.h"
   #include "d3d12_video_proc.h"
   #include "d3d12_residency.h"
   #include "d3d12_util.h"
   #include "d3d12_resource.h"
   #include "d3d12_video_buffer.h"
   #include "d3d12_format.h"
      void
   d3d12_video_processor_begin_frame(struct pipe_video_codec * codec,
               {
      struct d3d12_video_processor * pD3D12Proc = (struct d3d12_video_processor *) codec;
   debug_printf("[d3d12_video_processor] d3d12_video_processor_begin_frame - "
                  ///
   /// Wait here to make sure the next in flight resource set is empty before using it
   ///
            debug_printf("[d3d12_video_processor] d3d12_video_processor_begin_frame Waiting for completion of in flight resource sets with previous work with fenceValue: %" PRIu64 "\n",
            ASSERTED bool wait_res = d3d12_video_processor_sync_completion(codec, fenceValueToWaitOn, OS_TIMEOUT_INFINITE);
            HRESULT hr = pD3D12Proc->m_spCommandList->Reset(pD3D12Proc->m_spCommandAllocators[d3d12_video_processor_pool_current_index(pD3D12Proc)].Get());
   if (FAILED(hr)) {
      debug_printf(
         "[d3d12_video_processor] resetting ID3D12GraphicsCommandList failed with HR %x\n",
               // Setup process frame arguments for output/target texture.
            ID3D12Resource *pDstD3D12Res = d3d12_resource_resource(pOutputVideoBuffer->texture);    
   auto dstDesc = GetDesc(pDstD3D12Res);
   pD3D12Proc->m_OutputArguments = {
      //struct D3D12_VIDEO_PROCESS_OUTPUT_STREAM_ARGUMENTS args;
   {
         {
      {
               },
   {
                  },
   },
   // struct d3d12_resource* buffer;
      };
         }
      void
   d3d12_video_processor_end_frame(struct pipe_video_codec * codec,
               {
      struct d3d12_video_processor * pD3D12Proc = (struct d3d12_video_processor *) codec;
   debug_printf("[d3d12_video_processor] d3d12_video_processor_end_frame - "
                  if(pD3D12Proc->m_ProcessInputs.size() > pD3D12Proc->m_vpMaxInputStreams.MaxInputStreams) {
      debug_printf("[d3d12_video_processor] ERROR: Requested number of input surfaces (%" PRIu64 ") exceeds underlying D3D12 driver capabilities (%d)\n", (uint64_t) pD3D12Proc->m_ProcessInputs.size(), pD3D12Proc->m_vpMaxInputStreams.MaxInputStreams);
               auto curOutputDesc = GetOutputStreamDesc(pD3D12Proc->m_spVideoProcessor.Get());
   auto curOutputTexFmt = GetDesc(pD3D12Proc->m_OutputArguments.args.OutputStream[0].pTexture2D).Format;
      bool inputFmtsMatch = pD3D12Proc->m_inputStreamDescs.size() == pD3D12Proc->m_ProcessInputs.size();
   unsigned curInputIdx = 0;
   while( (curInputIdx < pD3D12Proc->m_inputStreamDescs.size()) && inputFmtsMatch)    
   {
      inputFmtsMatch = inputFmtsMatch && (pD3D12Proc->m_inputStreamDescs[curInputIdx].Format == GetDesc(pD3D12Proc->m_ProcessInputs[curInputIdx].InputStream[0].pTexture2D).Format);
               bool inputCountMatches = (pD3D12Proc->m_ProcessInputs.size() == pD3D12Proc->m_spVideoProcessor->GetNumInputStreamDescs());
   bool outputFmtMatches = (curOutputDesc.Format == curOutputTexFmt);
   bool needsVPRecreation = (
      !inputCountMatches // Requested batch has different number of Inputs to be blit'd
   || !outputFmtMatches // output texture format different than vid proc object expects
               if(needsVPRecreation) {
      debug_printf("[d3d12_video_processor] d3d12_video_processor_end_frame - Attempting to re-create ID3D12VideoProcessor "
               DXGI_COLOR_SPACE_TYPE OutputColorSpace = d3d12_convert_from_legacy_color_space(
      !util_format_is_yuv(d3d12_get_pipe_format(curOutputTexFmt)),
   util_format_get_blocksize(d3d12_get_pipe_format(curOutputTexFmt)) * 8 /*bytes to bits conversion*/,
   /* StudioRGB= */ false,
   /* P709= */ true,
   /* StudioYUV= */ true);
      std::vector<DXGI_FORMAT> InputFormats;
   for(D3D12_VIDEO_PROCESS_INPUT_STREAM_ARGUMENTS1 curInput : pD3D12Proc->m_ProcessInputs)
   {
         }
   DXGI_COLOR_SPACE_TYPE InputColorSpace = d3d12_convert_from_legacy_color_space(
      !util_format_is_yuv(d3d12_get_pipe_format(InputFormats[0])),
   util_format_get_blocksize(d3d12_get_pipe_format(InputFormats[0])) * 8 /*bytes to bits conversion*/,
   /* StudioRGB= */ false,
               // Release previous allocation
   pD3D12Proc->m_spVideoProcessor.Reset();
   if(!d3d12_video_processor_check_caps_and_create_processor(pD3D12Proc, InputFormats, InputColorSpace, curOutputTexFmt, OutputColorSpace))
   {
         debug_printf("[d3d12_video_processor] d3d12_video_processor_end_frame - Failure when "
                     // Schedule barrier transitions
   std::vector<D3D12_RESOURCE_BARRIER> barrier_transitions;
   barrier_transitions.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
                        for(D3D12_VIDEO_PROCESS_INPUT_STREAM_ARGUMENTS1 curInput : pD3D12Proc->m_ProcessInputs)
      barrier_transitions.push_back(CD3DX12_RESOURCE_BARRIER::Transition(
                                                         for (auto &BarrierDesc : barrier_transitions)
                     pD3D12Proc->m_PendingFences[d3d12_video_processor_pool_current_index(pD3D12Proc)].value = pD3D12Proc->m_fenceValue;
   pD3D12Proc->m_PendingFences[d3d12_video_processor_pool_current_index(pD3D12Proc)].cmdqueue_fence = pD3D12Proc->m_spFence.Get();
      }
      void
   d3d12_video_processor_process_frame(struct pipe_video_codec *codec,
               {
               // begin_frame gets only called once so wouldn't update process_properties->src_surface_fence correctly
            // Get the underlying resources from the pipe_video_buffers
                     // y0 = top
   // x0 = left
   // x1 = right
            debug_printf("d3d12_video_processor_process_frame: Adding Input ID3D12Resource: %p to scene (Output target %p)\n", pSrcD3D12Res, pD3D12Proc->m_OutputArguments.args.OutputStream[0].pTexture2D);
   debug_printf("d3d12_video_processor_process_frame: Input box: top: %d left: %d right: %d bottom: %d\n", process_properties->src_region.y0, process_properties->src_region.x0, process_properties->src_region.x1, process_properties->src_region.y1);
   debug_printf("d3d12_video_processor_process_frame: Output box: top: %d left: %d right: %d bottom: %d\n", process_properties->dst_region.y0, process_properties->dst_region.x0, process_properties->dst_region.x1, process_properties->dst_region.y1);
                     D3D12_VIDEO_PROCESS_INPUT_STREAM_ARGUMENTS1 InputArguments = {
      {
   { // D3D12_VIDEO_PROCESS_INPUT_STREAM InputStream[0];
            pSrcD3D12Res, // ID3D12Resource *pTexture2D;
   0, // UINT Subresource
   {//D3D12_VIDEO_PROCESS_REFERENCE_SET ReferenceSet;
      0, //UINT NumPastFrames;
   NULL, //ID3D12Resource **ppPastFrames;
   NULL, // UINT *pPastSubresources;
   0, //UINT NumFutureFrames;
   NULL, //ID3D12Resource **ppFutureFrames;
   },
   { // D3D12_VIDEO_PROCESS_INPUT_STREAM InputStream[1];
            NULL, //ID3D12Resource *pTexture2D;
   0, //UINT Subresource;
   {//D3D12_VIDEO_PROCESS_REFERENCE_SET ReferenceSet;
      0, //UINT NumPastFrames;
   NULL, //ID3D12Resource **ppPastFrames;
   NULL, // UINT *pPastSubresources;
   0, //UINT NumFutureFrames;
   NULL, //ID3D12Resource **ppFutureFrames;
   }
   },
   { // D3D12_VIDEO_PROCESS_TRANSFORM Transform;
         // y0 = top
   // x0 = left
   // x1 = right
   // y1 = bottom
   // typedef struct _RECT
   // {
   //     int left;
   //     int top;
   //     int right;
   //     int bottom;
   // } RECT;
   { process_properties->src_region.x0/*left*/, process_properties->src_region.y0/*top*/, process_properties->src_region.x1/*right*/, process_properties->src_region.y1/*bottom*/ },
   { process_properties->dst_region.x0/*left*/, process_properties->dst_region.y0/*top*/, process_properties->dst_region.x1/*right*/, process_properties->dst_region.y1/*bottom*/ }, // D3D12_RECT DestinationRectangle;
   },
   D3D12_VIDEO_PROCESS_INPUT_STREAM_FLAG_NONE,
   { // D3D12_VIDEO_PROCESS_INPUT_STREAM_RATE RateInfo;
         0,
   },
   // INT                                    FilterLevels[32];
   {
         },
   //D3D12_VIDEO_PROCESS_ALPHA_BLENDING;
   { 
         (process_properties->blend.mode == PIPE_VIDEO_VPP_BLEND_MODE_GLOBAL_ALPHA),
   },
   // D3D12_VIDEO_FIELD_TYPE FieldType
               debug_printf("ProcessFrame InArgs Orientation %d \n\tSrc top: %d left: %d right: %d bottom: %d\n\tDst top: %d left: %d right: %d bottom: %d\n", InputArguments.Transform.Orientation, 
      InputArguments.Transform.SourceRectangle.top, InputArguments.Transform.SourceRectangle.left, InputArguments.Transform.SourceRectangle.right, InputArguments.Transform.SourceRectangle.bottom,
         pD3D12Proc->m_ProcessInputs.push_back(InputArguments);    
   pD3D12Proc->m_InputBuffers.push_back(pInputVideoBuffer);
      ///
   /// Flush work to the GPU and blocking wait until GPU finishes
   ///
      }
      void
   d3d12_video_processor_destroy(struct pipe_video_codec * codec)
   {
      if (codec == nullptr) {
         }
   // Flush pending work before destroying.
            uint64_t curBatchFence = pD3D12Proc->m_fenceValue;
   if (pD3D12Proc->m_needsGPUFlush)
   {
      d3d12_video_processor_flush(codec);
               // Call dtor to make ComPtr work
      }
      void
   d3d12_video_processor_flush(struct pipe_video_codec * codec)
   {
      struct d3d12_video_processor * pD3D12Proc = (struct d3d12_video_processor *) codec;
   assert(pD3D12Proc);
   assert(pD3D12Proc->m_spD3D12VideoDevice);
            debug_printf("[d3d12_video_processor] d3d12_video_processor_flush started. Will flush video queue work and CPU wait on "
                  if (!pD3D12Proc->m_needsGPUFlush) {
         } else {
      debug_printf("[d3d12_video_processor] d3d12_video_processor_flush - Promoting the output texture %p to d3d12_permanently_resident.\n", 
            // Make the resources permanently resident for video use
            for(auto curInput : pD3D12Proc->m_InputBuffers)
   {
         debug_printf("[d3d12_video_processor] d3d12_video_processor_flush - Promoting the input texture %p to d3d12_permanently_resident.\n", 
         // Make the resources permanently resident for video use
            HRESULT hr = pD3D12Proc->m_pD3D12Screen->dev->GetDeviceRemovedReason();
   if (hr != S_OK) {
         debug_printf("[d3d12_video_processor] d3d12_video_processor_flush"
                              // Close and execute command list and wait for idle on CPU blocking
            if (pD3D12Proc->m_transitionsBeforeCloseCmdList.size() > 0) {
         pD3D12Proc->m_spCommandList->ResourceBarrier(pD3D12Proc->m_transitionsBeforeCloseCmdList.size(),
                  hr = pD3D12Proc->m_spCommandList->Close();
   if (FAILED(hr)) {
         debug_printf("[d3d12_video_processor] d3d12_video_processor_flush - Can't close command list with HR %x\n", hr);
            // Flush any work batched in the d3d12_screen and Wait on the m_spCommandQueue
   struct pipe_fence_handle *completion_fence = NULL;
   pD3D12Proc->base.context->flush(pD3D12Proc->base.context, &completion_fence, PIPE_FLUSH_ASYNC | PIPE_FLUSH_HINT_FINISH);
   struct d3d12_fence *casted_completion_fence = d3d12_fence(completion_fence);
   pD3D12Proc->m_spCommandQueue->Wait(casted_completion_fence->cmdqueue_fence, casted_completion_fence->value);
            struct d3d12_fence *input_surface_fence = pD3D12Proc->input_surface_fence;
   if (input_surface_fence)
            ID3D12CommandList *ppCommandLists[1] = { pD3D12Proc->m_spCommandList.Get() };
   pD3D12Proc->m_spCommandQueue->ExecuteCommandLists(1, ppCommandLists);
            // Validate device was not removed
   hr = pD3D12Proc->m_pD3D12Screen->dev->GetDeviceRemovedReason();
   if (hr != S_OK) {
         debug_printf("[d3d12_video_processor] d3d12_video_processor_flush"
                              debug_printf(
                  pD3D12Proc->m_fenceValue++;
      }
   pD3D12Proc->m_ProcessInputs.clear();
   pD3D12Proc->m_InputBuffers.clear();
                  flush_fail:
      debug_printf("[d3d12_video_processor] d3d12_video_processor_flush failed for fenceValue: %d\n", pD3D12Proc->m_fenceValue);
      }
      struct pipe_video_codec *
   d3d12_video_processor_create(struct pipe_context *context, const struct pipe_video_codec *codec)
   {
      ///
   /// Initialize d3d12_video_processor
            // Not using new doesn't call ctor and the initializations in the class declaration are lost
            pD3D12Proc->m_PendingFences.resize(D3D12_VIDEO_PROC_ASYNC_DEPTH);
            pD3D12Proc->base.context = context;
   pD3D12Proc->base.width = codec->width;
   pD3D12Proc->base.height = codec->height;
   pD3D12Proc->base.destroy = d3d12_video_processor_destroy;
   pD3D12Proc->base.begin_frame = d3d12_video_processor_begin_frame;
   pD3D12Proc->base.process_frame = d3d12_video_processor_process_frame;
   pD3D12Proc->base.end_frame = d3d12_video_processor_end_frame;
   pD3D12Proc->base.flush = d3d12_video_processor_flush;
                     ///
   /// Try initializing D3D12 Video device and check for device caps
            struct d3d12_context *pD3D12Ctx = (struct d3d12_context *) context;
   pD3D12Proc->m_pD3D12Context = pD3D12Ctx;
            // Assume defaults for now, can re-create if necessary when d3d12_video_processor_end_frame kicks off the processing
   DXGI_COLOR_SPACE_TYPE InputColorSpace = DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P709;
   std::vector<DXGI_FORMAT> InputFormats = { DXGI_FORMAT_NV12 };
   DXGI_FORMAT OutputFormat = DXGI_FORMAT_NV12;
            ///
   /// Create processor objects
   ///
   if (FAILED(pD3D12Proc->m_pD3D12Screen->dev->QueryInterface(
            debug_printf("[d3d12_video_processor] d3d12_video_create_processor - D3D12 Device has no Video support\n");
               if (FAILED(pD3D12Proc->m_spD3D12VideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_PROCESS_MAX_INPUT_STREAMS, &pD3D12Proc->m_vpMaxInputStreams, sizeof(pD3D12Proc->m_vpMaxInputStreams)))) {
      debug_printf("[d3d12_video_processor] d3d12_video_create_processor - Failed to query D3D12_FEATURE_VIDEO_PROCESS_MAX_INPUT_STREAMS\n");
               if (!d3d12_video_processor_check_caps_and_create_processor(pD3D12Proc, InputFormats, InputColorSpace, OutputFormat, OutputColorSpace)) {
      debug_printf("[d3d12_video_processor] d3d12_video_create_processor - Failure on "
                     if (!d3d12_video_processor_create_command_objects(pD3D12Proc)) {
      debug_printf(
                                    failed:
      if (pD3D12Proc != nullptr) {
                     }
      bool
   d3d12_video_processor_check_caps_and_create_processor(struct d3d12_video_processor *pD3D12Proc,
                           {
               D3D12_VIDEO_FIELD_TYPE FieldType = D3D12_VIDEO_FIELD_TYPE_NONE;
   D3D12_VIDEO_FRAME_STEREO_FORMAT StereoFormat = D3D12_VIDEO_FRAME_STEREO_FORMAT_NONE;
   DXGI_RATIONAL FrameRate = { 30, 1 };
            struct ResolStruct {
      uint Width;
               ResolStruct resolutionsList[] = {
      { 8192, 8192 },   // 8k
   { 8192, 4320 },   // 8k - alternative
   { 7680, 4800 },   // 8k - alternative
   { 7680, 4320 },   // 8k - alternative
   { 4096, 2304 },   // 2160p (4K)
   { 4096, 2160 },   // 2160p (4K) - alternative
   { 2560, 1440 },   // 1440p
   { 1920, 1200 },   // 1200p
   { 1920, 1080 },   // 1080p
   { 1280, 720 },    // 720p
               pD3D12Proc->m_SupportCaps = 
   {
      0, // NodeIndex
   { resolutionsList[0].Width, resolutionsList[0].Height, { InputFormats[0], InputColorSpace } },
   FieldType,
   StereoFormat,
   FrameRate,
   { OutputFormat, OutputColorSpace },
   StereoFormat,
               uint32_t idxResol = 0;
   bool bSupportsAny = false;
   while ((idxResol < ARRAY_SIZE(resolutionsList)) && !bSupportsAny) {
      pD3D12Proc->m_SupportCaps.InputSample.Width = resolutionsList[idxResol].Width;
   pD3D12Proc->m_SupportCaps.InputSample.Height = resolutionsList[idxResol].Height;
   if (SUCCEEDED(pD3D12Proc->m_spD3D12VideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_PROCESS_SUPPORT, &pD3D12Proc->m_SupportCaps, sizeof(pD3D12Proc->m_SupportCaps)))) {
         }
               if ((pD3D12Proc->m_SupportCaps.SupportFlags & D3D12_VIDEO_PROCESS_SUPPORT_FLAG_SUPPORTED) != D3D12_VIDEO_PROCESS_SUPPORT_FLAG_SUPPORTED)
   {
      if((pD3D12Proc->m_SupportCaps.SupportFlags & D3D12_VIDEO_PROCESS_SUPPORT_FLAG_SUPPORTED) != D3D12_VIDEO_PROCESS_SUPPORT_FLAG_SUPPORTED) {
   debug_printf("[d3d12_video_processor] d3d12_video_processor_check_caps_and_create_processor - D3D12_VIDEO_PROCESS_SUPPORT_FLAG_SUPPORTED not returned by driver. "
                                    bool enableOrientation = (
      ((pD3D12Proc->m_SupportCaps.FeatureSupport & D3D12_VIDEO_PROCESS_FEATURE_FLAG_ROTATION) != 0)
               D3D12_VIDEO_PROCESS_INPUT_STREAM_DESC inputStreamDesc = {
      InputFormats[0],
   InputColorSpace,
   AspectRatio,                            // SourceAspectRatio;
   AspectRatio,                            // DestinationAspectRatio;
   FrameRate,                              // FrameRate
   pD3D12Proc->m_SupportCaps.ScaleSupport.OutputSizeRange, // SourceSizeRange
   pD3D12Proc->m_SupportCaps.ScaleSupport.OutputSizeRange, // DestinationSizeRange
   enableOrientation,
   enabledFilterFlags,
   StereoFormat,
   FieldType,
   D3D12_VIDEO_PROCESS_DEINTERLACE_FLAG_NONE,
   ((pD3D12Proc->m_SupportCaps.FeatureSupport & D3D12_VIDEO_PROCESS_FEATURE_FLAG_ALPHA_BLENDING) != 0)
   && ((pD3D12Proc->m_SupportCaps.FeatureSupport & D3D12_VIDEO_PROCESS_FEATURE_FLAG_ALPHA_FILL) != 0), // EnableAlphaBlending
   {},                                     // LumaKey
   0,                                      // NumPastFrames
   0,                                      // NumFutureFrames
               D3D12_VIDEO_PROCESS_OUTPUT_STREAM_DESC outputStreamDesc =
   {
      pD3D12Proc->m_SupportCaps.OutputFormat.Format,
   OutputColorSpace,
   D3D12_VIDEO_PROCESS_ALPHA_FILL_MODE_OPAQUE, // AlphaFillMode
   0u,                                         // AlphaFillModeSourceStreamIndex
   {0, 0, 0, 0},                               // BackgroundColor
   FrameRate,                                  // FrameRate
      };
      // gets the required past/future frames for VP creation
   {
      D3D12_FEATURE_DATA_VIDEO_PROCESS_REFERENCE_INFO referenceInfo = {};
   referenceInfo.NodeIndex = 0;
   D3D12_VIDEO_PROCESS_FEATURE_FLAGS featureFlags = D3D12_VIDEO_PROCESS_FEATURE_FLAG_NONE;
   featureFlags |= outputStreamDesc.AlphaFillMode ? D3D12_VIDEO_PROCESS_FEATURE_FLAG_ALPHA_FILL : D3D12_VIDEO_PROCESS_FEATURE_FLAG_NONE;
   featureFlags |= inputStreamDesc.LumaKey.Enable ? D3D12_VIDEO_PROCESS_FEATURE_FLAG_LUMA_KEY : D3D12_VIDEO_PROCESS_FEATURE_FLAG_NONE;
   featureFlags |= (inputStreamDesc.StereoFormat != D3D12_VIDEO_FRAME_STEREO_FORMAT_NONE || outputStreamDesc.EnableStereo) ? D3D12_VIDEO_PROCESS_FEATURE_FLAG_STEREO : D3D12_VIDEO_PROCESS_FEATURE_FLAG_NONE;
   featureFlags |= inputStreamDesc.EnableOrientation ? D3D12_VIDEO_PROCESS_FEATURE_FLAG_ROTATION | D3D12_VIDEO_PROCESS_FEATURE_FLAG_FLIP : D3D12_VIDEO_PROCESS_FEATURE_FLAG_NONE;
            referenceInfo.DeinterlaceMode = inputStreamDesc.DeinterlaceMode;
   referenceInfo.Filters = inputStreamDesc.FilterFlags;
   referenceInfo.FeatureSupport = featureFlags;
   referenceInfo.InputFrameRate = inputStreamDesc.FrameRate;
   referenceInfo.OutputFrameRate = outputStreamDesc.FrameRate;
            hr = pD3D12Proc->m_spD3D12VideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_PROCESS_REFERENCE_INFO, &referenceInfo, sizeof(referenceInfo));
   if (FAILED(hr)) {
   debug_printf("[d3d12_video_processor] d3d12_video_processor_check_caps_and_create_processor - CheckFeatureSupport "
               return false;
            inputStreamDesc.NumPastFrames = referenceInfo.PastFrames;
               pD3D12Proc->m_outputStreamDesc = outputStreamDesc;
      debug_printf("[d3d12_video_processor]\t Creating Video Processor\n");
            pD3D12Proc->m_inputStreamDescs.clear();
   for (unsigned i = 0; i < InputFormats.size(); i++)
   {
      inputStreamDesc.Format = InputFormats[i];
   pD3D12Proc->m_inputStreamDescs.push_back(inputStreamDesc);
      }
            hr = pD3D12Proc->m_spD3D12VideoDevice->CreateVideoProcessor(pD3D12Proc->m_NodeMask,
                           if (FAILED(hr)) {
      debug_printf("[d3d12_video_processor] d3d12_video_processor_check_caps_and_create_processor - CreateVideoProcessor "
                              }
      bool
   d3d12_video_processor_create_command_objects(struct d3d12_video_processor *pD3D12Proc)
   {
               D3D12_COMMAND_QUEUE_DESC commandQueueDesc = { D3D12_COMMAND_LIST_TYPE_VIDEO_PROCESS };
   HRESULT hr = pD3D12Proc->m_pD3D12Screen->dev->CreateCommandQueue(
                  if (FAILED(hr)) {
      debug_printf("[d3d12_video_processor] d3d12_video_processor_create_command_objects - Call to CreateCommandQueue "
                           hr = pD3D12Proc->m_pD3D12Screen->dev->CreateFence(0,
                  if (FAILED(hr)) {
      debug_printf(
         "[d3d12_video_processor] d3d12_video_processor_create_command_objects - Call to CreateFence failed with HR %x\n",
               pD3D12Proc->m_spCommandAllocators.resize(D3D12_VIDEO_PROC_ASYNC_DEPTH);
   for (uint32_t i = 0; i < pD3D12Proc->m_spCommandAllocators.size() ; i++) {
      hr = pD3D12Proc->m_pD3D12Screen->dev->CreateCommandAllocator(
                  if (FAILED(hr)) {
         debug_printf("[d3d12_video_processor] d3d12_video_processor_create_command_objects - Call to "
                           ComPtr<ID3D12Device4> spD3D12Device4;
   if (FAILED(pD3D12Proc->m_pD3D12Screen->dev->QueryInterface(
            debug_printf(
                     hr = spD3D12Device4->CreateCommandList1(0,
                        if (FAILED(hr)) {
      debug_printf("[d3d12_video_processor] d3d12_video_processor_create_command_objects - Call to CreateCommandList "
                              }
      D3D12_VIDEO_PROCESS_ORIENTATION
   d3d12_video_processor_convert_pipe_rotation(enum pipe_video_vpp_orientation orientation_flags)
   {
               if(orientation_flags & PIPE_VIDEO_VPP_ROTATION_90)
   {
      result = (orientation_flags & PIPE_VIDEO_VPP_FLIP_HORIZONTAL) ? D3D12_VIDEO_PROCESS_ORIENTATION_CLOCKWISE_90_FLIP_HORIZONTAL : D3D12_VIDEO_PROCESS_ORIENTATION_CLOCKWISE_90;
      }
   else if(orientation_flags & PIPE_VIDEO_VPP_ROTATION_180)
   {
      result = D3D12_VIDEO_PROCESS_ORIENTATION_CLOCKWISE_180;
      }
   else if(orientation_flags & PIPE_VIDEO_VPP_ROTATION_270)
   {
      result = (orientation_flags & PIPE_VIDEO_VPP_FLIP_HORIZONTAL) ? D3D12_VIDEO_PROCESS_ORIENTATION_CLOCKWISE_270_FLIP_HORIZONTAL : D3D12_VIDEO_PROCESS_ORIENTATION_CLOCKWISE_270;
      }
   else if(orientation_flags & PIPE_VIDEO_VPP_FLIP_HORIZONTAL)
   {
      result = D3D12_VIDEO_PROCESS_ORIENTATION_FLIP_HORIZONTAL;
      }
   else if(orientation_flags & PIPE_VIDEO_VPP_FLIP_VERTICAL)
   {
      result = D3D12_VIDEO_PROCESS_ORIENTATION_FLIP_VERTICAL;
                  }
      uint64_t
   d3d12_video_processor_pool_current_index(struct d3d12_video_processor *pD3D12Proc)
   {
         }
         bool
   d3d12_video_processor_ensure_fence_finished(struct pipe_video_codec *codec,
               {
      bool wait_result = true;
   struct d3d12_video_processor *pD3D12Proc = (struct d3d12_video_processor *) codec;
   HRESULT hr = S_OK;
            debug_printf(
      "[d3d12_video_processor] d3d12_video_processor_ensure_fence_finished - Waiting for fence (with timeout_ns %" PRIu64
   ") to finish with "
   "fenceValue: %" PRIu64 " - Current Fence Completed Value %" PRIu64 "\n",
   timeout_ns,
   fenceValueToWaitOn,
                     HANDLE event = {};
   int event_fd = 0;
            hr = pD3D12Proc->m_spFence->SetEventOnCompletion(fenceValueToWaitOn, event);
   if (FAILED(hr)) {
      debug_printf("[d3d12_video_processor] d3d12_video_processor_ensure_fence_finished - SetEventOnCompletion for "
               "fenceValue %" PRIu64 " failed with HR %x\n",
               wait_result = d3d12_fence_wait_event(event, event_fd, timeout_ns);
            debug_printf("[d3d12_video_processor] d3d12_video_processor_ensure_fence_finished - Waiting on fence to be done with "
                  } else {
      debug_printf("[d3d12_video_processor] d3d12_video_processor_ensure_fence_finished - Fence already done with "
                  }
         ensure_fence_finished_fail:
      debug_printf("[d3d12_video_processor] d3d12_video_processor_sync_completion failed for fenceValue: %" PRIu64 "\n",
         assert(false);
      }
      bool
   d3d12_video_processor_sync_completion(struct pipe_video_codec *codec, uint64_t fenceValueToWaitOn, uint64_t timeout_ns)
   {
      struct d3d12_video_processor *pD3D12Proc = (struct d3d12_video_processor *) codec;
   assert(pD3D12Proc);
   assert(pD3D12Proc->m_spD3D12VideoDevice);
   assert(pD3D12Proc->m_spCommandQueue);
            ASSERTED bool wait_result = d3d12_video_processor_ensure_fence_finished(codec, fenceValueToWaitOn, timeout_ns);
            hr =
         if (FAILED(hr)) {
      debug_printf("m_spCommandAllocator->Reset() failed with %x.\n", hr);
               // Validate device was not removed
   hr = pD3D12Proc->m_pD3D12Screen->dev->GetDeviceRemovedReason();
   if (hr != S_OK) {
      debug_printf("[d3d12_video_processor] d3d12_video_processor_sync_completion"
               " - D3D12Device was removed AFTER d3d12_video_processor_ensure_fence_finished "
               debug_printf(
      "[d3d12_video_processor] d3d12_video_processor_sync_completion - GPU execution finalized for fenceValue: %" PRIu64
   "\n",
               sync_with_token_fail:
      debug_printf("[d3d12_video_processor] d3d12_video_processor_sync_completion failed for fenceValue: %" PRIu64 "\n",
         assert(false);
      }
      int d3d12_video_processor_get_processor_fence(struct pipe_video_codec *codec,
               {
      struct d3d12_fence *fenceValueToWaitOn = (struct d3d12_fence *) fence;
                     // Return semantics based on p_video_codec interface
   // ret == 0 -> work in progress
   // ret != 0 -> work completed
      }
