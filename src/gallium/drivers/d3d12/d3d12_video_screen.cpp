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
      #include "d3d12_screen.h"
   #include "d3d12_video_screen.h"
   #include "d3d12_format.h"
   #include "util/u_video.h"
   #include <directx/d3d12video.h>
   #include <cmath>
      #include <wrl/client.h>
   using Microsoft::WRL::ComPtr;
      #include "d3d12_video_types.h"
      struct d3d12_encode_codec_support {
      enum pipe_video_profile profile;
   union {
      struct {
      enum pipe_h265_enc_pred_direction prediction_direction;
   union pipe_h265_enc_cap_features hevc_features;
   union pipe_h265_enc_cap_block_sizes hevc_block_sizes;
      } hevc_support;
   struct {
      union pipe_av1_enc_cap_features features;
      #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
               #endif
               };
      struct d3d12_video_resolution_to_level_mapping_entry
   {
      D3D12_VIDEO_ENCODER_PICTURE_RESOLUTION_DESC resolution;
      };
      static void
   get_level_resolution_video_decode_support(D3D12_VIDEO_DECODE_CONFIGURATION decoderConfig,
                                       {
      outSupportAny = false;
   outMaxSupportedConfig = {};
   outMinResol = {};
            ComPtr<ID3D12VideoDevice> spD3D12VideoDevice;
   struct d3d12_screen *pD3D12Screen = (struct d3d12_screen *) pscreen;
   if (FAILED(pD3D12Screen->dev->QueryInterface(IID_PPV_ARGS(spD3D12VideoDevice.GetAddressOf())))) {
      // No video support in underlying d3d12 device (decode needs ID3D12VideoDevice)
               d3d12_video_resolution_to_level_mapping_entry resolutionsLevelList[] = {
      { { 8192, 4320 }, 61 },   // 8k
   { { 7680, 4800 }, 61 },   // 8k - alternative
   { { 7680, 4320 }, 61 },   // 8k - alternative
   { { 4096, 2304 }, 52 },   // 2160p (4K)
   { { 4096, 2160 }, 52 },   // 2160p (4K) - alternative
   { { 2560, 1440 }, 51 },   // 1440p
   { { 1920, 1200 }, 5 },    // 1200p
   { { 1920, 1080 }, 42 },   // 1080p
   { { 1280, 720 }, 4 },     // 720p
   { { 800, 600 }, 31 },
   { { 352, 480 }, 3 },
   { { 352, 240 }, 2 },
   { { 176, 144 }, 11 },
               D3D12_FEATURE_DATA_VIDEO_DECODE_SUPPORT decodeSupport = {};
   decodeSupport.Configuration = decoderConfig;
            uint32_t idxResol = 0;
               decodeSupport.Width = resolutionsLevelList[idxResol].resolution.Width;
            if (SUCCEEDED(spD3D12VideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_DECODE_SUPPORT,
                                    // Save the first (maximum)
   if(!outSupportAny) {
      outMaxSupportedConfig = decodeSupport;
                     // Keep saving the other supported values to get the minimum
         }
         }
      static bool
   d3d12_has_video_decode_support(struct pipe_screen *pscreen, enum pipe_video_profile profile)
   {
      ComPtr<ID3D12VideoDevice> spD3D12VideoDevice;
   struct d3d12_screen *pD3D12Screen = (struct d3d12_screen *) pscreen;
   if (FAILED(pD3D12Screen->dev->QueryInterface(IID_PPV_ARGS(spD3D12VideoDevice.GetAddressOf())))) {
      // No video support in underlying d3d12 device (needs ID3D12VideoDevice)
               D3D12_FEATURE_DATA_VIDEO_FEATURE_AREA_SUPPORT VideoFeatureAreaSupport = {};
   if (FAILED(spD3D12VideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_FEATURE_AREA_SUPPORT,
                              // Supported profiles below
   bool supportsProfile = false;
   switch (profile) {
      case PIPE_VIDEO_PROFILE_MPEG4_AVC_BASELINE:
   case PIPE_VIDEO_PROFILE_MPEG4_AVC_EXTENDED:
   case PIPE_VIDEO_PROFILE_MPEG4_AVC_CONSTRAINED_BASELINE:
   case PIPE_VIDEO_PROFILE_MPEG4_AVC_MAIN:
   case PIPE_VIDEO_PROFILE_MPEG4_AVC_HIGH:
   case PIPE_VIDEO_PROFILE_MPEG4_AVC_HIGH10:
   case PIPE_VIDEO_PROFILE_HEVC_MAIN:
   case PIPE_VIDEO_PROFILE_HEVC_MAIN_10:
   case PIPE_VIDEO_PROFILE_AV1_MAIN:
   case PIPE_VIDEO_PROFILE_VP9_PROFILE0:
   case PIPE_VIDEO_PROFILE_VP9_PROFILE2:
   {
         } break;
   default:
                  }
      static bool
   d3d12_video_encode_max_supported_level_for_profile(const D3D12_VIDEO_ENCODER_CODEC &argCodec,
                           {
      D3D12_FEATURE_DATA_VIDEO_ENCODER_PROFILE_LEVEL capLevelData = {};
   capLevelData.NodeIndex = 0;
   capLevelData.Codec = argCodec;
   capLevelData.Profile = argTargetProfile;
   capLevelData.MinSupportedLevel = minLvl;
            if (FAILED(pD3D12VideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_PROFILE_LEVEL,
                                 }
      static bool
   d3d12_video_encode_supported_resolution_range(const D3D12_VIDEO_ENCODER_CODEC &argTargetCodec,
                     {
               if (FAILED(pD3D12VideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_OUTPUT_RESOLUTION_RATIOS_COUNT,
                              D3D12_FEATURE_DATA_VIDEO_ENCODER_OUTPUT_RESOLUTION capOutputResolutionData = {};
   capOutputResolutionData.NodeIndex = 0;
   capOutputResolutionData.Codec = argTargetCodec;
            std::vector<D3D12_VIDEO_ENCODER_PICTURE_RESOLUTION_RATIO_DESC> ratiosTmpOutput;
   if (capResRatiosCountData.ResolutionRatiosCount > 0) {
      ratiosTmpOutput.resize(capResRatiosCountData.ResolutionRatiosCount);
      } else {
                  if (FAILED(pD3D12VideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_OUTPUT_RESOLUTION,
                  !capOutputResolutionData.IsSupported) {
               minResolution = capOutputResolutionData.MinResolutionSupported;
               }
      static uint32_t
   d3d12_video_encode_supported_references_per_frame_structures(const D3D12_VIDEO_ENCODER_CODEC &codec,
                     {
               D3D12_FEATURE_DATA_VIDEO_ENCODER_CODEC_PICTURE_CONTROL_SUPPORT capPictureControlData = {};
   capPictureControlData.NodeIndex = 0;
            if(codec == D3D12_VIDEO_ENCODER_CODEC_H264) {
      D3D12_VIDEO_ENCODER_CODEC_PICTURE_CONTROL_SUPPORT_H264 h264PictureControl = {};
   capPictureControlData.Profile = profile;
   capPictureControlData.PictureSupport.pH264Support = &h264PictureControl;
   capPictureControlData.PictureSupport.DataSize = sizeof(h264PictureControl);
   HRESULT hr = pD3D12VideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_CODEC_PICTURE_CONTROL_SUPPORT,
               if (FAILED(hr)) {
                  if (capPictureControlData.IsSupported) {
      /* This attribute determines the maximum number of reference
   * frames supported for encoding.
   *
   * Note: for H.264 encoding, the value represents the maximum number
   * of reference frames for both the reference picture list 0 (bottom
   * 16 bits) and the reference picture list 1 (top 16 bits).
   */
   uint32_t maxRefForL0 = std::min(capPictureControlData.PictureSupport.pH264Support->MaxL0ReferencesForP,
         uint32_t maxRefForL1 = capPictureControlData.PictureSupport.pH264Support->MaxL1ReferencesForB;
         } else if(codec == D3D12_VIDEO_ENCODER_CODEC_HEVC) {
      D3D12_VIDEO_ENCODER_CODEC_PICTURE_CONTROL_SUPPORT_HEVC hevcPictureControl = {};
   capPictureControlData.Profile = profile;
   capPictureControlData.PictureSupport.pHEVCSupport = &hevcPictureControl;
   capPictureControlData.PictureSupport.DataSize = sizeof(hevcPictureControl);
   HRESULT hr = pD3D12VideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_CODEC_PICTURE_CONTROL_SUPPORT,
               if (FAILED(hr)) {
                  if (capPictureControlData.IsSupported) {
      /* This attribute determines the maximum number of reference
   * frames supported for encoding.
   *
   * Note: for H.265 encoding, the value represents the maximum number
   * of reference frames for both the reference picture list 0 (bottom
   * 16 bits) and the reference picture list 1 (top 16 bits).
   */
   uint32_t maxRefForL0 = std::min(capPictureControlData.PictureSupport.pHEVCSupport->MaxL0ReferencesForP,
         uint32_t maxRefForL1 = capPictureControlData.PictureSupport.pHEVCSupport->MaxL1ReferencesForB;
            #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
      else if(codec == D3D12_VIDEO_ENCODER_CODEC_AV1){
      codecSupport.av1_support.d3d12_picture_control = {};
   capPictureControlData.Profile = profile;
   capPictureControlData.PictureSupport.pAV1Support = &codecSupport.av1_support.d3d12_picture_control;
   capPictureControlData.PictureSupport.DataSize = sizeof(codecSupport.av1_support.d3d12_picture_control);
   HRESULT hr = pD3D12VideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_CODEC_PICTURE_CONTROL_SUPPORT,
               if (FAILED(hr)) {
                  if (capPictureControlData.IsSupported) {
      /* This attribute determines the maximum number of reference
   * frames supported for encoding.
   */
   supportedMaxRefFrames = capPictureControlData.PictureSupport.pAV1Support->MaxUniqueReferencesPerFrame;
   if (capPictureControlData.PictureSupport.pAV1Support->PredictionMode)
            #endif
         }
      #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
   static void
   d3d12_video_encode_supported_tile_structures(const D3D12_VIDEO_ENCODER_CODEC &codec,
                                       )
   {
      // Assume no support and then add as queries succeed
            // Only codecs supporting tiles should use this method. For slices related info, use d3d12_video_encode_supported_slice_structures
   assert (codec == D3D12_VIDEO_ENCODER_CODEC_AV1);
      D3D12_FEATURE_DATA_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_CONFIG capDataTilesSupport = { };
   capDataTilesSupport.NodeIndex = 0;
   capDataTilesSupport.Codec = codec;
   capDataTilesSupport.Profile = profile;
   capDataTilesSupport.Level = level;
   capDataTilesSupport.FrameResolution = maxRes; // Query for worst case resolution
   av1TileSupport = { };
   capDataTilesSupport.CodecSupport.DataSize = sizeof(av1TileSupport);
   capDataTilesSupport.CodecSupport.pAV1Support = &av1TileSupport;
   av1TileSupport.Use128SuperBlocks = false; // return units in 64x64 default size
            {
      capDataTilesSupport.SubregionMode = D3D12_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE_UNIFORM_GRID_PARTITION;
   // Try with simple one tile request
   av1TileSupport.TilesConfiguration.ColCount = 1;
   av1TileSupport.TilesConfiguration.RowCount = 1;
   av1TileSupport.TilesConfiguration.ColWidths[0] = (capDataTilesSupport.FrameResolution.Width / superBlockSize);
   av1TileSupport.TilesConfiguration.RowHeights[0] = (capDataTilesSupport.FrameResolution.Height / superBlockSize);
   HRESULT hr = pD3D12VideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_CONFIG,
               if(SUCCEEDED(hr) && !capDataTilesSupport.IsSupported) {
      // Try with minimum driver indicated params for the given resolution
   // (ie. this could happen for high resolutions like 8K where AV1 max_tile_width (4096) is exceeded)
   av1TileSupport.TilesConfiguration.ColCount = std::max(av1TileSupport.MinTileCols, 1u);
   av1TileSupport.TilesConfiguration.RowCount = std::max(av1TileSupport.MinTileRows, 1u);
   hr = pD3D12VideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_CONFIG,
                     // Try with lower resolution as fallback
   D3D12_VIDEO_ENCODER_PICTURE_RESOLUTION_DESC fallbackRes = { 1920u, 1080u };
   if(SUCCEEDED(hr) && !capDataTilesSupport.IsSupported && (fallbackRes.Width <= maxRes.Width)
   && (fallbackRes.Height <= maxRes.Height) ) {
      auto oldRes = capDataTilesSupport.FrameResolution;
   capDataTilesSupport.FrameResolution = fallbackRes;
   hr = pD3D12VideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_CONFIG,
                           if(SUCCEEDED(hr) && capDataTilesSupport.IsSupported)
      supportedSliceStructures |= (PIPE_VIDEO_CAP_SLICE_STRUCTURE_POWER_OF_TWO_ROWS |
                  {
      capDataTilesSupport.SubregionMode = D3D12_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE_CONFIGURABLE_GRID_PARTITION;
   // Try with simple one tile request
   av1TileSupport.TilesConfiguration.ColCount = 1;
   av1TileSupport.TilesConfiguration.RowCount = 1;
   av1TileSupport.TilesConfiguration.ColWidths[0] = capDataTilesSupport.FrameResolution.Width;
   av1TileSupport.TilesConfiguration.RowHeights[0] = capDataTilesSupport.FrameResolution.Height;
   HRESULT hr = pD3D12VideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_CONFIG,
               if(SUCCEEDED(hr) && !capDataTilesSupport.IsSupported) {
      // Try with minimum driver indicated params for the given resolution
   // (ie. this could happen for high resolutions like 8K where AV1 max_tile_width (4096) is exceeded)
   av1TileSupport.TilesConfiguration.ColCount = std::max(av1TileSupport.MinTileCols, 1u);
   av1TileSupport.TilesConfiguration.RowCount = std::max(av1TileSupport.MinTileRows, 1u);
   // Try for uniform grid tiles
   UINT tileWPixel = capDataTilesSupport.FrameResolution.Width / av1TileSupport.TilesConfiguration.ColCount;
   UINT tileHPixel = capDataTilesSupport.FrameResolution.Height / av1TileSupport.TilesConfiguration.RowCount;
   for (UINT i = 0; i < av1TileSupport.TilesConfiguration.ColCount; i++)
                        hr = pD3D12VideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_CONFIG,
                     if(SUCCEEDED(hr) && capDataTilesSupport.IsSupported)
      supportedSliceStructures |= (PIPE_VIDEO_CAP_SLICE_STRUCTURE_ARBITRARY_MACROBLOCKS |
      }
   #endif
      static uint32_t
   d3d12_video_encode_supported_slice_structures(const D3D12_VIDEO_ENCODER_CODEC &codec,
                     {
      // Only codecs supporting slices should use this method. For tile related info, use d3d12_video_encode_supported_tile_structures
                     D3D12_FEATURE_DATA_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE capDataSubregionLayout = {};
   capDataSubregionLayout.NodeIndex = 0;
   capDataSubregionLayout.Codec = codec;
   capDataSubregionLayout.Profile = profile;
            /**
   * pipe_video_cap_slice_structure
   *
   * This attribute determines slice structures supported by the
   * driver for encoding. This attribute is a hint to the user so
   * that he can choose a suitable surface size and how to arrange
   * the encoding process of multiple slices per frame.
   *
   * More specifically, for H.264 encoding, this attribute
   * determines the range of accepted values to
   * h264_slice_descriptor::macroblock_address and
   * h264_slice_descriptor::num_macroblocks.
   *
   * For HEVC, similarly determines the ranges for
   * slice_segment_address
   * num_ctu_in_slice
   */
   capDataSubregionLayout.SubregionMode =
         HRESULT hr = pD3D12VideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE,
               if (FAILED(hr)) {
         } else if (capDataSubregionLayout.IsSupported) {
      /* This would be setting N subregions per frame in this D3D12 mode where N = (height/blocksize) / K */
   supportedSliceStructuresBitMask |= PIPE_VIDEO_CAP_SLICE_STRUCTURE_EQUAL_MULTI_ROWS;
   /* Assuming height/blocksize >= max_supported_slices, which is reported
   in PIPE_VIDEO_CAP_ENC_MAX_SLICES_PER_FRAME and should be checked by the client*/
   /* This would be setting N subregions per frame in this D3D12 mode where N = (height/blocksize) */
   supportedSliceStructuresBitMask |= PIPE_VIDEO_CAP_SLICE_STRUCTURE_EQUAL_ROWS;
   /* This is ok, would be setting K rows per subregions in this D3D12 mode (and rounding the last one) */
               capDataSubregionLayout.SubregionMode =
         hr = pD3D12VideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE,
               if (FAILED(hr)) {
         } else if (capDataSubregionLayout.IsSupported) {
      /* This would be setting K rows per subregions in this D3D12 mode */
   supportedSliceStructuresBitMask |= PIPE_VIDEO_CAP_SLICE_STRUCTURE_EQUAL_MULTI_ROWS;
   /* Assuming height/blocksize >= max_supported_slices, which is reported
   in PIPE_VIDEO_CAP_ENC_MAX_SLICES_PER_FRAME and should be checked by the client*/
   /* This would be setting 1 row per subregion in this D3D12 mode */
   supportedSliceStructuresBitMask |= PIPE_VIDEO_CAP_SLICE_STRUCTURE_EQUAL_ROWS;
   /* This is ok, would be setting K rows per subregions in this D3D12 mode (and rounding the last one) */
               /* Needs more work in VA frontend to support VAEncMiscParameterMaxSliceSize 
            /*capDataSubregionLayout.SubregionMode = D3D12_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE_BYTES_PER_SUBREGION;
   hr = pD3D12VideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_FRAME_SUBREGION_LAYOUT_MODE,
               if (FAILED(hr)) {
         } else if (capDataSubregionLayout.IsSupported) {
                     }
      static bool
   d3d12_video_encode_support_caps(const D3D12_VIDEO_ENCODER_CODEC &argTargetCodec,
                                 D3D12_VIDEO_ENCODER_PICTURE_RESOLUTION_DESC maxResolution,
   DXGI_FORMAT encodeFormat,
   {
      capEncoderSupportData1.NodeIndex = 0;
   capEncoderSupportData1.Codec = argTargetCodec;
   capEncoderSupportData1.InputFormat = encodeFormat;
   capEncoderSupportData1.RateControl = {};
   capEncoderSupportData1.RateControl.Mode = D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_CQP;
   capEncoderSupportData1.RateControl.TargetFrameRate.Numerator = 60;
   capEncoderSupportData1.RateControl.TargetFrameRate.Denominator = 1;
   D3D12_VIDEO_ENCODER_RATE_CONTROL_CQP rcCqp = { 25, 25, 25 };
   capEncoderSupportData1.RateControl.ConfigParams.pConfiguration_CQP = &rcCqp;
   capEncoderSupportData1.RateControl.ConfigParams.DataSize = sizeof(rcCqp);
   capEncoderSupportData1.IntraRefresh = D3D12_VIDEO_ENCODER_INTRA_REFRESH_MODE_NONE;
   capEncoderSupportData1.ResolutionsListCount = 1;
   capEncoderSupportData1.pResolutionList = &maxResolution;
   capEncoderSupportData1.MaxReferenceFramesInDPB = 1;
   /*
      All codec structures must be declared outside the switch statement to be
      */
   D3D12_VIDEO_ENCODER_PROFILE_H264 h264prof = {};
   D3D12_VIDEO_ENCODER_LEVELS_H264 h264lvl = {};
   D3D12_VIDEO_ENCODER_SEQUENCE_GOP_STRUCTURE_H264 h264Gop = { 1, 0, 0, 0, 0 };
   D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_H264 h264Config = {};
   D3D12_VIDEO_ENCODER_PROFILE_HEVC hevcprof = { };
   D3D12_VIDEO_ENCODER_LEVEL_TIER_CONSTRAINTS_HEVC hevcLvl = { };
   D3D12_VIDEO_ENCODER_SEQUENCE_GOP_STRUCTURE_HEVC hevcGop = { 1, 0, 0 };
      #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
      D3D12_VIDEO_ENCODER_AV1_PROFILE av1prof = { };
   D3D12_VIDEO_ENCODER_AV1_LEVEL_TIER_CONSTRAINTS av1Lvl = { };
   D3D12_VIDEO_ENCODER_AV1_SEQUENCE_STRUCTURE av1Gop = { 1, 0 };
      #endif
      switch (argTargetCodec) {
      case D3D12_VIDEO_ENCODER_CODEC_H264:
   {
      // assert(codecSupport.pH264Support); // Fill this in caller if ever used
   capEncoderSupportData1.SuggestedProfile.pH264Profile = &h264prof;
   capEncoderSupportData1.SuggestedProfile.DataSize = sizeof(h264prof);
   capEncoderSupportData1.SuggestedLevel.pH264LevelSetting = &h264lvl;
   capEncoderSupportData1.SuggestedLevel.DataSize = sizeof(h264lvl);
   capEncoderSupportData1.CodecGopSequence.pH264GroupOfPictures = &h264Gop;
   capEncoderSupportData1.CodecGopSequence.DataSize = sizeof(h264Gop);
   capEncoderSupportData1.CodecConfiguration.DataSize = sizeof(h264Config);
               case D3D12_VIDEO_ENCODER_CODEC_HEVC:
   {
      /* Only read from codecSupport.pHEVCSupport in this case (union of pointers definition) */
   assert(codecSupport.pHEVCSupport);
   hevcConfig = {
      D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_FLAG_NONE,
   codecSupport.pHEVCSupport->MinLumaCodingUnitSize,
   codecSupport.pHEVCSupport->MaxLumaCodingUnitSize,
   codecSupport.pHEVCSupport->MinLumaTransformUnitSize,
   codecSupport.pHEVCSupport->MaxLumaTransformUnitSize,
   codecSupport.pHEVCSupport->max_transform_hierarchy_depth_inter,
                              capEncoderSupportData1.SuggestedProfile.pHEVCProfile = &hevcprof;
   capEncoderSupportData1.SuggestedProfile.DataSize = sizeof(hevcprof);
   capEncoderSupportData1.SuggestedLevel.pHEVCLevelSetting = &hevcLvl;
   capEncoderSupportData1.SuggestedLevel.DataSize = sizeof(hevcLvl);
   capEncoderSupportData1.CodecGopSequence.pHEVCGroupOfPictures = &hevcGop;
   capEncoderSupportData1.CodecGopSequence.DataSize = sizeof(hevcGop);
   capEncoderSupportData1.CodecConfiguration.DataSize = sizeof(hevcConfig);
         #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
         case D3D12_VIDEO_ENCODER_CODEC_AV1:
   {
      capEncoderSupportData1.SuggestedProfile.pAV1Profile = &av1prof;
   capEncoderSupportData1.SuggestedProfile.DataSize = sizeof(av1prof);
   capEncoderSupportData1.SuggestedLevel.pAV1LevelSetting = &av1Lvl;
   capEncoderSupportData1.SuggestedLevel.DataSize = sizeof(av1Lvl);
   capEncoderSupportData1.CodecGopSequence.pAV1SequenceStructure = &av1Gop;
   capEncoderSupportData1.CodecGopSequence.DataSize = sizeof(av1Gop);
   D3D12_FEATURE_DATA_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT capCodecConfigData = { };
   capCodecConfigData.NodeIndex = 0;
   capCodecConfigData.Codec = D3D12_VIDEO_ENCODER_CODEC_AV1;
   capCodecConfigData.Profile.pAV1Profile = &av1prof;
   capCodecConfigData.Profile.DataSize = sizeof(av1prof);
   D3D12_VIDEO_ENCODER_AV1_CODEC_CONFIGURATION_SUPPORT av1CodecSupport = { };
   capCodecConfigData.CodecSupportLimits.pAV1Support = &av1CodecSupport;
   capCodecConfigData.CodecSupportLimits.DataSize = sizeof(av1CodecSupport);
   HRESULT hr = pD3D12VideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT, &capCodecConfigData, sizeof(capCodecConfigData));
   if (FAILED(hr)) {
      debug_printf("CheckFeatureSupport D3D12_FEATURE_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT failed with HR %x\n", hr);
      } else if (!capCodecConfigData.IsSupported) {
      debug_printf("CheckFeatureSupport D3D12_FEATURE_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT IsSupported is false\n");
      }
   av1Config.OrderHintBitsMinus1 = 7;
   av1Config.FeatureFlags = av1CodecSupport.RequiredFeatureFlags;
   capEncoderSupportData1.CodecConfiguration.DataSize = sizeof(av1Config);
      #endif
         default:
   {
                     // prepare inout storage for the resolution dependent result.
   resolutionDepCaps = {};
            HRESULT hr = pD3D12VideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_SUPPORT1,
               if (FAILED(hr)) {
      debug_printf("CheckFeatureSupport D3D12_FEATURE_VIDEO_ENCODER_SUPPORT1 failed with HR %x\n", hr);
            // D3D12_FEATURE_DATA_VIDEO_ENCODER_SUPPORT1 extends D3D12_FEATURE_DATA_VIDEO_ENCODER_SUPPORT
   // in a binary compatible way, so just cast it and try with the older query D3D12_FEATURE_VIDEO_ENCODER_SUPPORT
   D3D12_FEATURE_DATA_VIDEO_ENCODER_SUPPORT * casted_down_cap_data = reinterpret_cast<D3D12_FEATURE_DATA_VIDEO_ENCODER_SUPPORT*>(&capEncoderSupportData1);
   hr = pD3D12VideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_SUPPORT,
               if (FAILED(hr)) {
      debug_printf("CheckFeatureSupport D3D12_FEATURE_VIDEO_ENCODER_SUPPORT failed with HR %x\n", hr);
               #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
               // D3D12: QualityVsSpeed must be in the range [0, D3D12_FEATURE_DATA_VIDEO_ENCODER_SUPPORT1.MaxQualityVsSpeed]
            // PIPE: The quality level range can be queried through the VAConfigAttribEncQualityRange attribute. 
   // A lower value means higher quality, and a value of 1 represents the highest quality. 
   // The quality level setting is used as a trade-off between quality and speed/power 
   // consumption, with higher quality corresponds to lower speed and higher power consumption.
      #else
         #endif
         bool configSupported =
      (((capEncoderSupportData1.SupportFlags & D3D12_VIDEO_ENCODER_SUPPORT_FLAG_GENERAL_SUPPORT_OK) != 0) &&
            }
      #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
   bool
   static d3d12_video_encode_get_av1_codec_support ( const D3D12_VIDEO_ENCODER_CODEC &argCodec,
                     {
      D3D12_FEATURE_DATA_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT capCodecConfigData = { };
   capCodecConfigData.NodeIndex = 0;
   capCodecConfigData.Codec = D3D12_VIDEO_ENCODER_CODEC_AV1;
   capCodecConfigData.Profile = argTargetProfile;
   capCodecConfigData.CodecSupportLimits.pAV1Support = &av1Support;
   capCodecConfigData.CodecSupportLimits.DataSize = sizeof(D3D12_VIDEO_ENCODER_AV1_CODEC_CONFIGURATION_SUPPORT);
   if(SUCCEEDED(pD3D12VideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT, &capCodecConfigData, sizeof(capCodecConfigData)))
      && capCodecConfigData.IsSupported) {
               memset(&av1Support, 0, sizeof(D3D12_VIDEO_ENCODER_AV1_CODEC_CONFIGURATION_SUPPORT));
      }
   #endif
      bool
   static d3d12_video_encode_get_hevc_codec_support ( const D3D12_VIDEO_ENCODER_CODEC &argCodec,
                     {
      constexpr unsigned c_hevcConfigurationSets = 5u;
   const D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT_HEVC hevcConfigurationSets[c_hevcConfigurationSets] = 
   {
      {
      D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT_HEVC_FLAG_NONE,
   D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_CUSIZE_8x8,
   D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_CUSIZE_32x32,
   D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_TUSIZE_4x4,
   D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_TUSIZE_32x32,
   3u,
      },
   {
      D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT_HEVC_FLAG_NONE,
   D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_CUSIZE_8x8,
   D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_CUSIZE_32x32,
   D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_TUSIZE_4x4,
   D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_TUSIZE_32x32,            
   0u,            
      },
   {
      D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT_HEVC_FLAG_NONE,
   D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_CUSIZE_8x8,
   D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_CUSIZE_32x32,
   D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_TUSIZE_4x4,
   D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_TUSIZE_32x32,            
   2u,            
      },
   {
      D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT_HEVC_FLAG_NONE,
   D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_CUSIZE_8x8,
   D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_CUSIZE_64x64,
   D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_TUSIZE_4x4,
   D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_TUSIZE_32x32,            
   2u,            
      },
   {
      D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT_HEVC_FLAG_NONE,
   D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_CUSIZE_8x8,
   D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_CUSIZE_64x64,
   D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_TUSIZE_4x4,
   D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_HEVC_TUSIZE_32x32,
   4u,
                  D3D12_FEATURE_DATA_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT capCodecConfigData = { };
   capCodecConfigData.NodeIndex = 0;
   capCodecConfigData.Codec = D3D12_VIDEO_ENCODER_CODEC_HEVC;
   capCodecConfigData.Profile = argTargetProfile;
            for (uint32_t i = 0 ; i < c_hevcConfigurationSets ; i++) {
      supportedCaps = hevcConfigurationSets[i];
   capCodecConfigData.CodecSupportLimits.pHEVCSupport = &supportedCaps;
   if(SUCCEEDED(pD3D12VideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT, &capCodecConfigData, sizeof(capCodecConfigData)))
      && capCodecConfigData.IsSupported) {
                  memset(&supportedCaps, 0, sizeof(D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT_HEVC));
      }
      static bool
   d3d12_has_video_encode_support(struct pipe_screen *pscreen,
                                 enum pipe_video_profile profile,
   uint32_t &maxLvlSpec,
   D3D12_VIDEO_ENCODER_PICTURE_RESOLUTION_DESC &minRes,
   D3D12_VIDEO_ENCODER_PICTURE_RESOLUTION_DESC &maxRes,
   uint32_t &maxSlices,
   uint32_t &supportedSliceStructures,
   uint32_t &maxReferencesPerFrame,
   {
      ComPtr<ID3D12VideoDevice3> spD3D12VideoDevice;
   struct d3d12_screen *pD3D12Screen = (struct d3d12_screen *) pscreen;
   if (FAILED(pD3D12Screen->dev->QueryInterface(IID_PPV_ARGS(spD3D12VideoDevice.GetAddressOf())))) {
      // No video encode support in underlying d3d12 device (needs ID3D12VideoDevice3)
               D3D12_FEATURE_DATA_VIDEO_FEATURE_AREA_SUPPORT VideoFeatureAreaSupport = {};
   if (FAILED(spD3D12VideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_FEATURE_AREA_SUPPORT,
                     }
   D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT d3d12_codec_support = { };
   bool supportsProfile = false;
   switch (profile) {
      case PIPE_VIDEO_PROFILE_MPEG4_AVC_CONSTRAINED_BASELINE:
   case PIPE_VIDEO_PROFILE_MPEG4_AVC_BASELINE:
   case PIPE_VIDEO_PROFILE_MPEG4_AVC_MAIN:
   case PIPE_VIDEO_PROFILE_MPEG4_AVC_HIGH:
   case PIPE_VIDEO_PROFILE_MPEG4_AVC_HIGH10:
   {
      D3D12_VIDEO_ENCODER_PROFILE_DESC profDesc = {};
   D3D12_VIDEO_ENCODER_PROFILE_H264 profH264 =
         profDesc.DataSize = sizeof(profH264);
   profDesc.pH264Profile = &profH264;
   D3D12_VIDEO_ENCODER_CODEC codecDesc = d3d12_video_encoder_convert_codec_to_d3d12_enc_codec(profile);
   D3D12_VIDEO_ENCODER_LEVELS_H264 minLvlSettingH264 = static_cast<D3D12_VIDEO_ENCODER_LEVELS_H264>(0);
   D3D12_VIDEO_ENCODER_LEVELS_H264 maxLvlSettingH264 = static_cast<D3D12_VIDEO_ENCODER_LEVELS_H264>(0);
   D3D12_VIDEO_ENCODER_LEVEL_SETTING minLvl = {};
   D3D12_VIDEO_ENCODER_LEVEL_SETTING maxLvl = {};
   minLvl.pH264LevelSetting = &minLvlSettingH264;
   minLvl.DataSize = sizeof(minLvlSettingH264);
   maxLvl.pH264LevelSetting = &maxLvlSettingH264;
   maxLvl.DataSize = sizeof(maxLvlSettingH264);
   if (d3d12_video_encode_max_supported_level_for_profile(codecDesc,
                              uint32_t constraintset3flag = false;
                  DXGI_FORMAT encodeFormat = d3d12_convert_pipe_video_profile_to_dxgi_format(profile);
                  D3D12_VIDEO_ENCODER_PROFILE_DESC profile;
   profile.pH264Profile = &profH264;
   profile.DataSize = sizeof(profH264);
   D3D12_VIDEO_ENCODER_LEVEL_SETTING level;
   level.pH264LevelSetting = &maxLvlSettingH264;
   level.DataSize = sizeof(maxLvlSettingH264);
   supportedSliceStructures = d3d12_video_encode_supported_slice_structures(codecDesc,
                     D3D12_FEATURE_DATA_VIDEO_ENCODER_SUPPORT1 capEncoderSupportData1 = {};
   D3D12_FEATURE_DATA_VIDEO_ENCODER_RESOLUTION_SUPPORT_LIMITS resolutionDepCaps;
   capEncoderSupportData1.SubregionFrameEncoding = (supportedSliceStructures == PIPE_VIDEO_CAP_SLICE_STRUCTURE_NONE) ?
   #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
               D3D12_VIDEO_ENCODER_PICTURE_CONTROL_SUBREGIONS_LAYOUT_DATA_SLICES sliceData = { };
   #endif
               supportsProfile = supportsProfile && d3d12_video_encode_support_caps(codecDesc,
                                             if (supportedSliceStructures == PIPE_VIDEO_CAP_SLICE_STRUCTURE_NONE)
                        isRCMaxFrameSizeSupported = ((capEncoderSupportData1.SupportFlags & D3D12_VIDEO_ENCODER_SUPPORT_FLAG_RATE_CONTROL_MAX_FRAME_SIZE_AVAILABLE) != 0) ? 1 : 0;
   maxReferencesPerFrame =
      d3d12_video_encode_supported_references_per_frame_structures(codecDesc,
                  } break;
   case PIPE_VIDEO_PROFILE_HEVC_MAIN:
   case PIPE_VIDEO_PROFILE_HEVC_MAIN_10:
   {
      D3D12_VIDEO_ENCODER_PROFILE_DESC profDesc = {};
   D3D12_VIDEO_ENCODER_PROFILE_HEVC profHEVC =
         profDesc.DataSize = sizeof(profHEVC);
   profDesc.pHEVCProfile = &profHEVC;
   D3D12_VIDEO_ENCODER_CODEC codecDesc = d3d12_video_encoder_convert_codec_to_d3d12_enc_codec(profile);
   D3D12_VIDEO_ENCODER_LEVEL_TIER_CONSTRAINTS_HEVC minLvlSettingHEVC = { };
   D3D12_VIDEO_ENCODER_LEVEL_TIER_CONSTRAINTS_HEVC maxLvlSettingHEVC = { };
   D3D12_VIDEO_ENCODER_LEVEL_SETTING minLvl = {};
   D3D12_VIDEO_ENCODER_LEVEL_SETTING maxLvl = {};
   minLvl.pHEVCLevelSetting = &minLvlSettingHEVC;
   minLvl.DataSize = sizeof(minLvlSettingHEVC);
   maxLvl.pHEVCLevelSetting = &maxLvlSettingHEVC;
   maxLvl.DataSize = sizeof(maxLvlSettingHEVC);
   if (d3d12_video_encode_max_supported_level_for_profile(codecDesc,
                                       D3D12_VIDEO_ENCODER_PROFILE_DESC d3d12_profile;
   d3d12_profile.pHEVCProfile = &profHEVC;
   d3d12_profile.DataSize = sizeof(profHEVC);
   D3D12_VIDEO_ENCODER_LEVEL_SETTING level;
   level.pHEVCLevelSetting = &maxLvlSettingHEVC;
   level.DataSize = sizeof(maxLvlSettingHEVC);
   supportedSliceStructures = d3d12_video_encode_supported_slice_structures(codecDesc,
                        maxReferencesPerFrame =
      d3d12_video_encode_supported_references_per_frame_structures(codecDesc,
                     supportsProfile = d3d12_video_encode_get_hevc_codec_support(codecDesc,
                     if (supportsProfile) {
                     /* get_video_param sets pipe_features.bits.config_supported = 1
      to distinguish between supported cap with all bits off and unsupported by driver
                                    uint8_t minCuSize = d3d12_video_encoder_convert_12cusize_to_pixel_size_hevc(codecSupport.hevc_support.d3d12_caps.MinLumaCodingUnitSize);
   uint8_t maxCuSize = d3d12_video_encoder_convert_12cusize_to_pixel_size_hevc(codecSupport.hevc_support.d3d12_caps.MaxLumaCodingUnitSize);
   uint8_t MinCbLog2SizeY = std::log2(minCuSize);
                        codecSupport.hevc_support.hevc_block_sizes.bits.log2_max_coding_tree_block_size_minus3
                                                                                                                                                                        codecSupport.hevc_support.prediction_direction = PIPE_H265_PRED_DIRECTION_ALL;
   if(ref_l0)
                        codecSupport.hevc_support.hevc_features.bits.separate_colour_planes = PIPE_ENC_FEATURE_NOT_SUPPORTED;
   codecSupport.hevc_support.hevc_features.bits.scaling_lists = PIPE_ENC_FEATURE_NOT_SUPPORTED;
   codecSupport.hevc_support.hevc_features.bits.pcm = PIPE_ENC_FEATURE_NOT_SUPPORTED;
   codecSupport.hevc_support.hevc_features.bits.temporal_mvp = PIPE_ENC_FEATURE_NOT_SUPPORTED;
   codecSupport.hevc_support.hevc_features.bits.strong_intra_smoothing = PIPE_ENC_FEATURE_NOT_SUPPORTED;
   codecSupport.hevc_support.hevc_features.bits.dependent_slices = PIPE_ENC_FEATURE_NOT_SUPPORTED;
   codecSupport.hevc_support.hevc_features.bits.sign_data_hiding = PIPE_ENC_FEATURE_NOT_SUPPORTED;
                                                                                                   if ((codecSupport.hevc_support.d3d12_caps.SupportFlags & D3D12_VIDEO_ENCODER_CODEC_CONFIGURATION_SUPPORT_HEVC_FLAG_CONSTRAINED_INTRAPREDICTION_SUPPORT) != 0)
                                             D3D12_FEATURE_DATA_VIDEO_ENCODER_SUPPORT1 capEncoderSupportData1 = {};
   D3D12_FEATURE_DATA_VIDEO_ENCODER_RESOLUTION_SUPPORT_LIMITS resolutionDepCaps;
      #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
                     #endif
                  supportsProfile = supportsProfile && d3d12_video_encode_support_caps(codecDesc,
                                             if (supportedSliceStructures == PIPE_VIDEO_CAP_SLICE_STRUCTURE_NONE)
         else
                  #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
         case PIPE_VIDEO_PROFILE_AV1_MAIN:
   {
      D3D12_VIDEO_ENCODER_PROFILE_DESC profDesc = {};
   D3D12_VIDEO_ENCODER_AV1_PROFILE profAV1 =
         profDesc.DataSize = sizeof(profAV1);
   profDesc.pAV1Profile = &profAV1;
   D3D12_VIDEO_ENCODER_CODEC codecDesc = d3d12_video_encoder_convert_codec_to_d3d12_enc_codec(profile);
   D3D12_VIDEO_ENCODER_AV1_LEVEL_TIER_CONSTRAINTS minLvlSettingAV1 = { };
   D3D12_VIDEO_ENCODER_AV1_LEVEL_TIER_CONSTRAINTS maxLvlSettingAV1 = { };
   D3D12_VIDEO_ENCODER_LEVEL_SETTING minLvl = {};
   D3D12_VIDEO_ENCODER_LEVEL_SETTING maxLvl = {};
   minLvl.pAV1LevelSetting = &minLvlSettingAV1;
   minLvl.DataSize = sizeof(minLvlSettingAV1);
   maxLvl.pAV1LevelSetting = &maxLvlSettingAV1;
   maxLvl.DataSize = sizeof(maxLvlSettingAV1);
   if (d3d12_video_encode_max_supported_level_for_profile(codecDesc,
                                       D3D12_VIDEO_ENCODER_PROFILE_DESC d3d12_profile;
                  maxReferencesPerFrame =
      d3d12_video_encode_supported_references_per_frame_structures(codecDesc,
                     supportsProfile = d3d12_video_encode_get_av1_codec_support(codecDesc,
                     if (supportsProfile) {
                                    if ((codecSupport.av1_support.d3d12_caps.RequiredFeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_128x128_SUPERBLOCK) != 0)
                              if ((codecSupport.av1_support.d3d12_caps.RequiredFeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_FILTER_INTRA) != 0)
      codecSupport.av1_support.features.bits.support_filter_intra = (PIPE_ENC_FEATURE_SUPPORTED | PIPE_ENC_FEATURE_REQUIRED);   
                     if ((codecSupport.av1_support.d3d12_caps.RequiredFeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_INTRA_EDGE_FILTER) != 0)
                              if ((codecSupport.av1_support.d3d12_caps.RequiredFeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_INTERINTRA_COMPOUND) != 0)
                                                            if ((codecSupport.av1_support.d3d12_caps.RequiredFeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_WARPED_MOTION) != 0)
                                                            if ((codecSupport.av1_support.d3d12_caps.RequiredFeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_DUAL_FILTER) != 0)
      codecSupport.av1_support.features.bits.support_dual_filter = (PIPE_ENC_FEATURE_SUPPORTED | PIPE_ENC_FEATURE_REQUIRED);    
                                                                                 if ((codecSupport.av1_support.d3d12_caps.RequiredFeatureFlags & D3D12_VIDEO_ENCODER_AV1_FEATURE_FLAG_SUPER_RESOLUTION) != 0)
                                                                                                                                             if ((codecSupport.av1_support.d3d12_caps.SupportedInterpolationFilters & D3D12_VIDEO_ENCODER_AV1_INTERPOLATION_FILTERS_FLAG_EIGHTTAP_SHARP) != 0)
            if ((codecSupport.av1_support.d3d12_caps.SupportedInterpolationFilters & D3D12_VIDEO_ENCODER_AV1_INTERPOLATION_FILTERS_FLAG_BILINEAR) != 0)
                                                                                                                                                                                    // tx_mode_support query cap
   {
      // libva tx_mode_support, PIPE_XX and D3D12 flags are defined with the same numerical values.
   static_assert(static_cast<uint32_t>(D3D12_VIDEO_ENCODER_AV1_TX_MODE_FLAG_SELECT) ==
         static_assert(static_cast<uint32_t>(D3D12_VIDEO_ENCODER_AV1_TX_MODE_FLAG_LARGEST) ==
                        // Iterate over the tx_modes and generate the D3D12_VIDEO_ENCODER_AV1_TX_MODE_FLAGS d3d12SupportFlag
   for(uint8_t i = D3D12_VIDEO_ENCODER_AV1_TX_MODE_ONLY4x4; i <= D3D12_VIDEO_ENCODER_AV1_TX_MODE_SELECT; i++)
   {
      uint32_t d3d12SupportFlag = (1 << i); // See definition of D3D12_VIDEO_ENCODER_AV1_TX_MODE_FLAGS
   // Check the current d3d12SupportFlag (ie. D3D12_VIDEO_ENCODER_AV1_TX_MODE_FLAG_XXX) is supported for all frame types
   bool tx_mode_supported = true;
   for(uint8_t j = D3D12_VIDEO_ENCODER_AV1_FRAME_TYPE_KEY_FRAME; j <= D3D12_VIDEO_ENCODER_AV1_FRAME_TYPE_SWITCH_FRAME; j++)
   {
                                                      // As per d3d12 spec, driver must support at least one default mode for all frame types
   // Workaround for mismatch between VAAPI/D3D12 and TxMode support for all/some frame types
   if (!codecSupport.av1_support.features_ext2.bits.tx_mode_support)
   {
      debug_printf("[d3d12_has_video_encode_support] Reporting features_ext2.bits.tx_mode_support = D3D12_VIDEO_ENCODER_AV1_TX_MODE_FLAG_SELECT"
                                       D3D12_VIDEO_ENCODER_AV1_FRAME_SUBREGION_LAYOUT_CONFIG_SUPPORT av1TileSupport = {};
   d3d12_video_encode_supported_tile_structures(codecDesc,
                                             // Cannot pass pipe 2 bit-field as reference, use aux variable instead.
                                 D3D12_FEATURE_DATA_VIDEO_ENCODER_SUPPORT1 capEncoderSupportData1 = {};
   D3D12_FEATURE_DATA_VIDEO_ENCODER_RESOLUTION_SUPPORT_LIMITS resolutionDepCaps;
                                       supportsProfile = supportsProfile && d3d12_video_encode_support_caps(codecDesc,
                                             if (supportedSliceStructures == PIPE_VIDEO_CAP_SLICE_STRUCTURE_NONE)
                                          #endif
         default:
                  }
      static int
   d3d12_screen_get_video_param_decode(struct pipe_screen *pscreen,
                     {
      switch (param) {
      case PIPE_VIDEO_CAP_REQUIRES_FLUSH_ON_END_FRAME:
      /* As sometimes we need to copy the output
      and sync with the context, we handle the
      */
      case PIPE_VIDEO_CAP_NPOT_TEXTURES:
         case PIPE_VIDEO_CAP_MAX_WIDTH:
   case PIPE_VIDEO_CAP_MAX_HEIGHT:
   case PIPE_VIDEO_CAP_MAX_LEVEL:
   case PIPE_VIDEO_CAP_MIN_WIDTH:
   case PIPE_VIDEO_CAP_MIN_HEIGHT:
   case PIPE_VIDEO_CAP_SUPPORTED:
   {
      if (d3d12_has_video_decode_support(pscreen, profile)) {
      DXGI_FORMAT format = d3d12_convert_pipe_video_profile_to_dxgi_format(profile);
   auto pipeFmt = d3d12_get_pipe_format(format);
   bool formatSupported = pscreen->is_video_format_supported(pscreen, pipeFmt, profile, entrypoint);
   if (formatSupported) {
      GUID decodeGUID = d3d12_video_decoder_convert_pipe_video_profile_to_d3d12_profile(profile);
   GUID emptyGUID = {};
   if (decodeGUID != emptyGUID) {
      bool supportAny = false;
   D3D12_FEATURE_DATA_VIDEO_DECODE_SUPPORT outSupportedConfig = {};
                        d3d12_video_resolution_to_level_mapping_entry lowestSupportedConfig = {};
   d3d12_video_resolution_to_level_mapping_entry bestSupportedConfig = {};
      get_level_resolution_video_decode_support(decoderConfig,
                                    if (supportAny) {
      if (param == PIPE_VIDEO_CAP_MAX_WIDTH) {
         } else if (param == PIPE_VIDEO_CAP_MAX_HEIGHT) {
         } else if (param == PIPE_VIDEO_CAP_MIN_WIDTH) {
         } else if (param == PIPE_VIDEO_CAP_MIN_HEIGHT) {
         } else if (param == PIPE_VIDEO_CAP_MAX_LEVEL) {
         } else if (param == PIPE_VIDEO_CAP_SUPPORTED) {
                     }
      } break;
   case PIPE_VIDEO_CAP_PREFERED_FORMAT:
         case PIPE_VIDEO_CAP_PREFERS_INTERLACED:
         case PIPE_VIDEO_CAP_SUPPORTS_INTERLACED:
         case PIPE_VIDEO_CAP_SUPPORTS_PROGRESSIVE:
         case PIPE_VIDEO_CAP_SUPPORTS_CONTIGUOUS_PLANES_MAP:
         default:
      debug_printf("[d3d12_screen_get_video_param] unknown video param: %d\n", param);
      }
         static bool
   d3d12_has_video_process_support(struct pipe_screen *pscreen,
                     {
      ComPtr<ID3D12VideoDevice> spD3D12VideoDevice;
   struct d3d12_screen *pD3D12Screen = (struct d3d12_screen *) pscreen;
   if (FAILED(pD3D12Screen->dev->QueryInterface(IID_PPV_ARGS(spD3D12VideoDevice.GetAddressOf())))) {
      // No video process support in underlying d3d12 device (needs ID3D12VideoDevice)
               D3D12_FEATURE_DATA_VIDEO_FEATURE_AREA_SUPPORT VideoFeatureAreaSupport = {};
   if (FAILED(spD3D12VideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_FEATURE_AREA_SUPPORT,
                              D3D12_VIDEO_ENCODER_PICTURE_RESOLUTION_DESC resolutionsList[] = {
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
   { 800, 600 },
   { 352, 480 },
   { 352, 240 },
   { 176, 144 },
   { 128, 128 },
   { 96, 96 },
   { 64, 64 },
   { 32, 32 },
   { 16, 16 },
   { 8, 8 },
   { 4, 4 },
   { 2, 2 },
               outMinSupportedInput = {};
   outMaxSupportedInput = {};
   uint32_t idxResol = 0;
   bool bSupportsAny = false;
   while (idxResol < ARRAY_SIZE(resolutionsList)) {
      supportCaps.InputSample.Width = resolutionsList[idxResol].Width;
   supportCaps.InputSample.Height = resolutionsList[idxResol].Height;
   if (SUCCEEDED(spD3D12VideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_PROCESS_SUPPORT, &supportCaps, sizeof(supportCaps)))) {
      if ((supportCaps.SupportFlags & D3D12_VIDEO_PROCESS_SUPPORT_FLAG_SUPPORTED) != 0)
   {
      // Save the first (maximum)
   if(!bSupportsAny) {
                        // Keep saving the other supported values to get the minimum
         }
                  }
      static int
   d3d12_screen_get_video_param_postproc(struct pipe_screen *pscreen,
                     {
      switch (param) {
      case PIPE_VIDEO_CAP_REQUIRES_FLUSH_ON_END_FRAME:
         case PIPE_VIDEO_CAP_NPOT_TEXTURES:
         case PIPE_VIDEO_CAP_MAX_WIDTH:
   case PIPE_VIDEO_CAP_MAX_HEIGHT:
   case PIPE_VIDEO_CAP_MIN_WIDTH:
   case PIPE_VIDEO_CAP_MIN_HEIGHT:
   case PIPE_VIDEO_CAP_SUPPORTED:
   case PIPE_VIDEO_CAP_PREFERED_FORMAT:
   case PIPE_VIDEO_CAP_SUPPORTS_INTERLACED:
   case PIPE_VIDEO_CAP_SUPPORTS_PROGRESSIVE:
   case PIPE_VIDEO_CAP_SUPPORTS_CONTIGUOUS_PLANES_MAP:
   case PIPE_VIDEO_CAP_VPP_MAX_INPUT_WIDTH:
   case PIPE_VIDEO_CAP_VPP_MAX_INPUT_HEIGHT:
   case PIPE_VIDEO_CAP_VPP_MIN_INPUT_WIDTH:
   case PIPE_VIDEO_CAP_VPP_MIN_INPUT_HEIGHT:
   case PIPE_VIDEO_CAP_VPP_MAX_OUTPUT_WIDTH:
   case PIPE_VIDEO_CAP_VPP_MAX_OUTPUT_HEIGHT:
   case PIPE_VIDEO_CAP_VPP_MIN_OUTPUT_WIDTH:
   case PIPE_VIDEO_CAP_VPP_MIN_OUTPUT_HEIGHT:
   case PIPE_VIDEO_CAP_VPP_ORIENTATION_MODES:
   case PIPE_VIDEO_CAP_VPP_BLEND_MODES:
   {
      // Assume defaults for now, we don't have the input args passed by get_video_param to be accurate here.
   const D3D12_VIDEO_FIELD_TYPE FieldType = D3D12_VIDEO_FIELD_TYPE_NONE;
   const D3D12_VIDEO_FRAME_STEREO_FORMAT StereoFormat = D3D12_VIDEO_FRAME_STEREO_FORMAT_NONE;
   const DXGI_RATIONAL FrameRate = { 30, 1 };
   const DXGI_FORMAT InputFormat = DXGI_FORMAT_NV12;
   const DXGI_COLOR_SPACE_TYPE InputColorSpace = DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P709;
   const DXGI_FORMAT OutputFormat = DXGI_FORMAT_NV12;
   const DXGI_COLOR_SPACE_TYPE OutputColorSpace = DXGI_COLOR_SPACE_YCBCR_STUDIO_G22_LEFT_P709;
   const UINT Width = 1280;
   const UINT Height = 720;
   D3D12_FEATURE_DATA_VIDEO_PROCESS_SUPPORT supportCaps =
   {
      0, // NodeIndex
   { Width, Height, { InputFormat, InputColorSpace } },
   FieldType,
   StereoFormat,
   FrameRate,
   { OutputFormat, OutputColorSpace },
   StereoFormat,
               D3D12_VIDEO_ENCODER_PICTURE_RESOLUTION_DESC minSupportedInput = {};
   D3D12_VIDEO_ENCODER_PICTURE_RESOLUTION_DESC maxSupportedInput = {};
   if (d3d12_has_video_process_support(pscreen, supportCaps, minSupportedInput, maxSupportedInput)) {
      if (param == PIPE_VIDEO_CAP_SUPPORTED) {
         } else if (param == PIPE_VIDEO_CAP_PREFERED_FORMAT) {
         } else if (param == PIPE_VIDEO_CAP_SUPPORTS_INTERLACED) {
         } else if (param == PIPE_VIDEO_CAP_MIN_WIDTH) {
         } else if (param == PIPE_VIDEO_CAP_MIN_HEIGHT) {
         } else if (param == PIPE_VIDEO_CAP_MAX_WIDTH) {
         } else if (param == PIPE_VIDEO_CAP_MAX_HEIGHT) {
         } else if (param == PIPE_VIDEO_CAP_SUPPORTS_CONTIGUOUS_PLANES_MAP) {
         } else if (param == PIPE_VIDEO_CAP_SUPPORTS_PROGRESSIVE) {
         } else if (param == PIPE_VIDEO_CAP_VPP_MAX_INPUT_WIDTH) {
         } else if (param == PIPE_VIDEO_CAP_VPP_MAX_INPUT_HEIGHT) {
         } else if (param == PIPE_VIDEO_CAP_VPP_MIN_INPUT_WIDTH) {
         } else if (param == PIPE_VIDEO_CAP_VPP_MIN_INPUT_HEIGHT) {
         } else if (param == PIPE_VIDEO_CAP_VPP_MAX_OUTPUT_WIDTH) {
         } else if (param == PIPE_VIDEO_CAP_VPP_MAX_OUTPUT_HEIGHT) {
         } else if (param == PIPE_VIDEO_CAP_VPP_MIN_OUTPUT_WIDTH) {
         } else if (param == PIPE_VIDEO_CAP_VPP_MIN_OUTPUT_HEIGHT) {
         } else if (param == PIPE_VIDEO_CAP_VPP_BLEND_MODES) {
      uint32_t blend_modes = PIPE_VIDEO_VPP_BLEND_MODE_NONE;
   if (((supportCaps.FeatureSupport & D3D12_VIDEO_PROCESS_FEATURE_FLAG_ALPHA_BLENDING) != 0)
      && ((supportCaps.FeatureSupport & D3D12_VIDEO_PROCESS_FEATURE_FLAG_ALPHA_FILL) != 0))
   {
         }
   } else if (param == PIPE_VIDEO_CAP_VPP_ORIENTATION_MODES) {
      uint32_t orientation_modes = PIPE_VIDEO_VPP_ORIENTATION_DEFAULT;
   if((supportCaps.FeatureSupport & D3D12_VIDEO_PROCESS_FEATURE_FLAG_FLIP) != 0) {
                        if((supportCaps.FeatureSupport & D3D12_VIDEO_PROCESS_FEATURE_FLAG_ROTATION) != 0) {
      orientation_modes |= PIPE_VIDEO_VPP_ROTATION_90;
   orientation_modes |= PIPE_VIDEO_VPP_ROTATION_180;
      }
         }
      } break;
   default:
         }
      static int
   d3d12_screen_get_video_param_encode(struct pipe_screen *pscreen,
                     {
      uint32_t maxLvlEncode = 0u;
   D3D12_VIDEO_ENCODER_PICTURE_RESOLUTION_DESC minResEncode = {};
   D3D12_VIDEO_ENCODER_PICTURE_RESOLUTION_DESC maxResEncode = {};
   uint32_t maxSlices = 0u;
   uint32_t supportedSliceStructures = 0u;
   uint32_t maxReferencesPerFrame = 0u;
   uint32_t isRCMaxFrameSizeSupported = 0u;
   uint32_t maxQualityLevels = 0u;
   uint32_t max_tile_rows = 0u;
   uint32_t max_tile_cols = 0u;
   struct d3d12_encode_codec_support codec_specific_support;
   memset(&codec_specific_support, 0, sizeof(codec_specific_support));
   switch (param) {
      case PIPE_VIDEO_CAP_ENC_SUPPORTS_ASYNC_OPERATION:
         case PIPE_VIDEO_CAP_REQUIRES_FLUSH_ON_END_FRAME:
         case PIPE_VIDEO_CAP_NPOT_TEXTURES:
         case PIPE_VIDEO_CAP_MAX_WIDTH:
   case PIPE_VIDEO_CAP_MAX_HEIGHT:
   case PIPE_VIDEO_CAP_MIN_WIDTH:
   case PIPE_VIDEO_CAP_MIN_HEIGHT:
   case PIPE_VIDEO_CAP_MAX_LEVEL:
   case PIPE_VIDEO_CAP_SUPPORTED:
   case PIPE_VIDEO_CAP_ENC_MAX_SLICES_PER_FRAME:
   case PIPE_VIDEO_CAP_ENC_SLICES_STRUCTURE:
   case PIPE_VIDEO_CAP_ENC_MAX_REFERENCES_PER_FRAME:
   case PIPE_VIDEO_CAP_ENC_HEVC_FEATURE_FLAGS:
   case PIPE_VIDEO_CAP_ENC_HEVC_BLOCK_SIZES:
   case PIPE_VIDEO_CAP_ENC_HEVC_PREDICTION_DIRECTION:
   case PIPE_VIDEO_CAP_ENC_AV1_FEATURE:
   case PIPE_VIDEO_CAP_ENC_AV1_FEATURE_EXT1:
   case PIPE_VIDEO_CAP_ENC_AV1_FEATURE_EXT2:
   case PIPE_VIDEO_CAP_ENC_SUPPORTS_TILE:
   case PIPE_VIDEO_CAP_ENC_SUPPORTS_MAX_FRAME_SIZE:
   case PIPE_VIDEO_CAP_ENC_QUALITY_LEVEL:
   case PIPE_VIDEO_CAP_ENC_MAX_TILE_ROWS:
   case PIPE_VIDEO_CAP_ENC_MAX_TILE_COLS:
   {
      if (d3d12_has_video_encode_support(pscreen,
                                    profile,
   maxLvlEncode,
   minResEncode,
   maxResEncode,
   maxSlices,
                  DXGI_FORMAT format = d3d12_convert_pipe_video_profile_to_dxgi_format(profile);
   auto pipeFmt = d3d12_get_pipe_format(format);
   bool formatSupported = pscreen->is_video_format_supported(pscreen, pipeFmt, profile, entrypoint);
   if (formatSupported) {
      if (param == PIPE_VIDEO_CAP_MAX_WIDTH) {
         } else if (param == PIPE_VIDEO_CAP_MAX_HEIGHT) {
         } else if (param == PIPE_VIDEO_CAP_MIN_WIDTH) {
         } else if (param == PIPE_VIDEO_CAP_MIN_HEIGHT) {
         } else if (param == PIPE_VIDEO_CAP_MAX_LEVEL) {
         } else if (param == PIPE_VIDEO_CAP_SUPPORTED) {
         } else if (param == PIPE_VIDEO_CAP_ENC_MAX_SLICES_PER_FRAME) {
         } else if (param == PIPE_VIDEO_CAP_ENC_SLICES_STRUCTURE) {
         } else if (param == PIPE_VIDEO_CAP_ENC_MAX_TILE_ROWS) {
         } else if (param == PIPE_VIDEO_CAP_ENC_MAX_TILE_COLS) {
         } else if (param == PIPE_VIDEO_CAP_ENC_MAX_REFERENCES_PER_FRAME) {
         } else if (param == PIPE_VIDEO_CAP_ENC_SUPPORTS_MAX_FRAME_SIZE) {
         } else if (param == PIPE_VIDEO_CAP_ENC_HEVC_FEATURE_FLAGS) {
      /* get_video_param sets hevc_features.bits.config_supported = 1
      to distinguish between supported cap with all bits off and unsupported by driver
      */
      } else if (param == PIPE_VIDEO_CAP_ENC_HEVC_BLOCK_SIZES) {
      /* get_video_param sets hevc_block_sizes.bits.config_supported = 1
      to distinguish between supported cap with all bits off and unsupported by driver
      */
      } else if (param == PIPE_VIDEO_CAP_ENC_HEVC_PREDICTION_DIRECTION) {
      if (PIPE_VIDEO_FORMAT_HEVC == u_reduce_video_profile(profile))
   #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
                  else if (param == PIPE_VIDEO_CAP_ENC_AV1_FEATURE) {
   return codec_specific_support.av1_support.features.value;
   } else if (param == PIPE_VIDEO_CAP_ENC_AV1_FEATURE_EXT1) {
   return codec_specific_support.av1_support.features_ext1.value;
   } else if (param == PIPE_VIDEO_CAP_ENC_AV1_FEATURE_EXT2) {
   return codec_specific_support.av1_support.features_ext2.value;
   } else if(param == PIPE_VIDEO_CAP_ENC_SUPPORTS_TILE) {
            #endif
                  } else if (param == PIPE_VIDEO_CAP_ENC_QUALITY_LEVEL) {
         }
      } break;
   case PIPE_VIDEO_CAP_PREFERED_FORMAT:
         case PIPE_VIDEO_CAP_PREFERS_INTERLACED:
         case PIPE_VIDEO_CAP_SUPPORTS_INTERLACED:
         case PIPE_VIDEO_CAP_SUPPORTS_PROGRESSIVE:
         case PIPE_VIDEO_CAP_SUPPORTS_CONTIGUOUS_PLANES_MAP:
         case PIPE_VIDEO_CAP_ENC_RATE_CONTROL_QVBR:
   {
      D3D12_FEATURE_DATA_VIDEO_ENCODER_RATE_CONTROL_MODE capRateControlModeData =
   {
      0,
   d3d12_video_encoder_convert_codec_to_d3d12_enc_codec(profile),
   D3D12_VIDEO_ENCODER_RATE_CONTROL_MODE_QVBR,
               ComPtr<ID3D12VideoDevice3> spD3D12VideoDevice;
   struct d3d12_screen *pD3D12Screen = (struct d3d12_screen *) pscreen;
   if (FAILED(pD3D12Screen->dev->QueryInterface(IID_PPV_ARGS(spD3D12VideoDevice.GetAddressOf())))) {
      // No video encode support in underlying d3d12 device (needs ID3D12VideoDevice3)
               if (SUCCEEDED(spD3D12VideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_RATE_CONTROL_MODE, &capRateControlModeData, sizeof(capRateControlModeData)))
                  // No QVBR support
      } break;
   default:
      debug_printf("[d3d12_screen_get_video_param] unknown video param: %d\n", param);
      }
      static int
   d3d12_screen_get_video_param(struct pipe_screen *pscreen,
                     {
      if (entrypoint == PIPE_VIDEO_ENTRYPOINT_BITSTREAM) {
         } else if (entrypoint == PIPE_VIDEO_ENTRYPOINT_ENCODE) {
         } else if (entrypoint == PIPE_VIDEO_ENTRYPOINT_PROCESSING) {
         }
      }
      static bool
   is_d3d12_video_encode_format_supported(struct pipe_screen *screen,
               {
      D3D12_VIDEO_ENCODER_PROFILE_H264 profH264 = {};
      #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
         #endif
      D3D12_FEATURE_DATA_VIDEO_ENCODER_INPUT_FORMAT capDataFmt = {};
   capDataFmt.NodeIndex = 0;
   capDataFmt.Codec = d3d12_video_encoder_convert_codec_to_d3d12_enc_codec(profile);
   capDataFmt.Format = d3d12_get_format(format);
   switch (u_reduce_video_profile(profile)) {
      case PIPE_VIDEO_FORMAT_MPEG4_AVC:
   {
      profH264 = d3d12_video_encoder_convert_profile_to_d3d12_enc_profile_h264(profile);
   capDataFmt.Profile.DataSize = sizeof(profH264);
      } break;
   case PIPE_VIDEO_FORMAT_HEVC:
   {
      profHEVC = d3d12_video_encoder_convert_profile_to_d3d12_enc_profile_hevc(profile);
   capDataFmt.Profile.DataSize = sizeof(profHEVC);
         #if ((D3D12_SDK_VERSION >= 611) && (D3D12_PREVIEW_SDK_VERSION >= 712))
      case PIPE_VIDEO_FORMAT_AV1:
   {
      profAV1 = d3d12_video_encoder_convert_profile_to_d3d12_enc_profile_av1(profile);
   capDataFmt.Profile.DataSize = sizeof(profAV1);
         #endif
      default:
   {
            }
   ComPtr<ID3D12VideoDevice3> spD3D12VideoDevice;
   struct d3d12_screen *pD3D12Screen = (struct d3d12_screen *) screen;
   if (FAILED(pD3D12Screen->dev->QueryInterface(IID_PPV_ARGS(spD3D12VideoDevice.GetAddressOf())))) {
      // No video encode support in underlying d3d12 device (needs ID3D12VideoDevice3)
      }
   HRESULT hr = spD3D12VideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_ENCODER_INPUT_FORMAT,
                  }
      static bool
   is_d3d12_video_decode_format_supported(struct pipe_screen *screen,
               {
      ComPtr<ID3D12VideoDevice> spD3D12VideoDevice;
   struct d3d12_screen *pD3D12Screen = (struct d3d12_screen *) screen;
   if (FAILED(pD3D12Screen->dev->QueryInterface(IID_PPV_ARGS(spD3D12VideoDevice.GetAddressOf()))))
            GUID decodeGUID = d3d12_video_decoder_convert_pipe_video_profile_to_d3d12_profile(profile);
   GUID emptyGUID = {};
            D3D12_VIDEO_DECODE_CONFIGURATION decoderConfig = { decodeGUID,
                  D3D12_FEATURE_DATA_VIDEO_DECODE_FORMAT_COUNT decodeFormatCount = {0 /* NodeIndex*/, decoderConfig };
   if(FAILED(spD3D12VideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_DECODE_FORMAT_COUNT,
                        std::vector<DXGI_FORMAT> supportedDecodeFormats;
            D3D12_FEATURE_DATA_VIDEO_DECODE_FORMATS decodeFormats =
   {
      0, // NodeIndex
   decoderConfig,
   static_cast<UINT>(supportedDecodeFormats.size()),
               if(FAILED(spD3D12VideoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_DECODE_FORMATS,
                        DXGI_FORMAT requestedDXGIFormat = d3d12_get_format(format);
   for (DXGI_FORMAT fmt : supportedDecodeFormats)
      if (fmt == requestedDXGIFormat)
         }
      static bool
   is_d3d12_video_process_format_supported(struct pipe_screen *screen,
         {
      // Return both VPBlit support and format is in known list
   return (screen->get_video_param(screen,
                     &&
   ((format == PIPE_FORMAT_NV12) || (format == PIPE_FORMAT_P010)
      || (format == PIPE_FORMAT_R8G8B8A8_UNORM) || (format == PIPE_FORMAT_R8G8B8A8_UINT)
   }
      static bool
   is_d3d12_video_allowed_format(enum pipe_format format, enum pipe_video_entrypoint entrypoint)
   {
      if (entrypoint == PIPE_VIDEO_ENTRYPOINT_BITSTREAM) {
         } else if (entrypoint == PIPE_VIDEO_ENTRYPOINT_ENCODE) {
         } else if (entrypoint == PIPE_VIDEO_ENTRYPOINT_PROCESSING) {
      return (format == PIPE_FORMAT_NV12) || (format == PIPE_FORMAT_P010)
      || (format == PIPE_FORMAT_R8G8B8A8_UNORM) || (format == PIPE_FORMAT_R8G8B8A8_UINT)
   }
      }
      static bool
   d3d12_video_buffer_is_format_supported(struct pipe_screen *screen,
                     {
      // Check in allowed list of formats first
   if(!is_d3d12_video_allowed_format(format, entrypoint))
            // If the VA frontend asks for all profiles, assign
   // a default profile based on the bitdepth
   if(u_reduce_video_profile(profile) == PIPE_VIDEO_FORMAT_UNKNOWN)
   {
                  // Then check is the underlying driver supports the allowed formats
   if (entrypoint == PIPE_VIDEO_ENTRYPOINT_BITSTREAM) {
         } else if (entrypoint == PIPE_VIDEO_ENTRYPOINT_ENCODE) {
         } else if (entrypoint == PIPE_VIDEO_ENTRYPOINT_PROCESSING) {
         }
      }
      void
   d3d12_screen_video_init(struct pipe_screen *pscreen)
   {
      pscreen->get_video_param = d3d12_screen_get_video_param;
      }
