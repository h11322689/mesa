   /**************************************************************************
   *
   * Copyright 2010 Thomas Balling Sørensen & Orasanu Lucian.
   * Copyright 2014 Advanced Micro Devices, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   * IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      #include "pipe/p_screen.h"
      #include "util/u_video.h"
   #include "util/u_memory.h"
      #include "vl/vl_winsys.h"
   #include "vl/vl_codec.h"
      #include "va_private.h"
      #include "util/u_handle_table.h"
      DEBUG_GET_ONCE_BOOL_OPTION(mpeg4, "VAAPI_MPEG4_ENABLED", false)
      VAStatus
   vlVaQueryConfigProfiles(VADriverContextP ctx, VAProfile *profile_list, int *num_profiles)
   {
      struct pipe_screen *pscreen;
   enum pipe_video_profile p;
            if (!ctx)
                     pscreen = VL_VA_PSCREEN(ctx);
   for (p = PIPE_VIDEO_PROFILE_MPEG2_SIMPLE; p <= PIPE_VIDEO_PROFILE_AV1_MAIN; ++p) {
      if (u_reduce_video_profile(p) == PIPE_VIDEO_FORMAT_MPEG4 && !debug_get_option_mpeg4())
            if (vl_codec_supported(pscreen, p, false) ||
      vl_codec_supported(pscreen, p, true)) {
   vap = PipeToProfile(p);
   if (vap != VAProfileNone)
                  /* Support postprocessing through vl_compositor */
               }
      VAStatus
   vlVaQueryConfigEntrypoints(VADriverContextP ctx, VAProfile profile,
         {
      struct pipe_screen *pscreen;
   enum pipe_video_profile p;
            if (!ctx)
                     if (profile == VAProfileNone) {
      entrypoint_list[(*num_entrypoints)++] = VAEntrypointVideoProc;
               p = ProfileToPipe(profile);
   if (p == PIPE_VIDEO_PROFILE_UNKNOWN ||
      (u_reduce_video_profile(p) == PIPE_VIDEO_FORMAT_MPEG4 &&
   !debug_get_option_mpeg4()))
         pscreen = VL_VA_PSCREEN(ctx);
   if (vl_codec_supported(pscreen, p, false))
         #if VA_CHECK_VERSION(1, 16, 0)
      if (p == PIPE_VIDEO_PROFILE_AV1_MAIN)
      #endif
         if (p != PIPE_VIDEO_PROFILE_AV1_MAIN || check_av1enc_support == true)
      if (vl_codec_supported(pscreen, p, true))
         if (*num_entrypoints == 0)
                        }
      static unsigned int get_screen_supported_va_rt_formats(struct pipe_screen *pscreen,
               {
               if (pscreen->is_video_format_supported(pscreen, PIPE_FORMAT_NV12,
                  pscreen->is_video_format_supported(pscreen, PIPE_FORMAT_YV12,
               pscreen->is_video_format_supported(pscreen, PIPE_FORMAT_IYUV,
                     if (pscreen->is_video_format_supported(pscreen, PIPE_FORMAT_P010,
                  pscreen->is_video_format_supported(pscreen, PIPE_FORMAT_P016,
                     if (pscreen->is_video_format_supported(pscreen, PIPE_FORMAT_Y8_400_UNORM,
                        if (pscreen->is_video_format_supported(pscreen, PIPE_FORMAT_Y8_U8_V8_444_UNORM,
                        if (pscreen->is_video_format_supported(pscreen, PIPE_FORMAT_UYVY,
                  pscreen->is_video_format_supported(pscreen, PIPE_FORMAT_YUYV,
                     if (pscreen->is_video_format_supported(pscreen, PIPE_FORMAT_R8G8B8A8_UNORM,
                  pscreen->is_video_format_supported(pscreen, PIPE_FORMAT_R8G8B8A8_UINT,
               pscreen->is_video_format_supported(pscreen, PIPE_FORMAT_R8G8B8X8_UNORM,
               pscreen->is_video_format_supported(pscreen, PIPE_FORMAT_R8G8B8X8_UINT,
                  if (pscreen->is_video_format_supported(pscreen, PIPE_FORMAT_R8_G8_B8_UNORM,
                              }
      VAStatus
   vlVaGetConfigAttributes(VADriverContextP ctx, VAProfile profile, VAEntrypoint entrypoint,
         {
      struct pipe_screen *pscreen;
            if (!ctx)
                     for (i = 0; i < num_attribs; ++i) {
      unsigned int value;
   if ((entrypoint == VAEntrypointVLD) &&
      (vl_codec_supported(pscreen, ProfileToPipe(profile), false))) {
   switch (attrib_list[i].type) {
   case VAConfigAttribRTFormat:
      /*
   * Different gallium drivers will have different supported formats
   * If modifying this, please query the driver like below
   */
   value = get_screen_supported_va_rt_formats(pscreen,
                  case VAConfigAttribMaxPictureWidth:
   {
      value = pscreen->get_video_param(pscreen, ProfileToPipe(profile),
                  } break;
   case VAConfigAttribMaxPictureHeight:
   {
      value = pscreen->get_video_param(pscreen, ProfileToPipe(profile),
                  } break;
   default:
      value = VA_ATTRIB_NOT_SUPPORTED;
         } else if ((entrypoint == VAEntrypointEncSlice) &&
            switch (attrib_list[i].type) {
   case VAConfigAttribRTFormat:
      value = get_screen_supported_va_rt_formats(pscreen,
                  case VAConfigAttribRateControl:
   {
                     /* Check for optional mode QVBR */
   int supports_qvbr = pscreen->get_video_param(pscreen, ProfileToPipe(profile),
               if (supports_qvbr > 0)
      } break;
   case VAConfigAttribEncRateControlExt:
      value = pscreen->get_video_param(pscreen, ProfileToPipe(profile),
               if (value > 0) {
      value -= 1;
      }
      case VAConfigAttribEncPackedHeaders:
      value = VA_ENC_PACKED_HEADER_NONE;
   if ((u_reduce_video_profile(ProfileToPipe(profile)) == PIPE_VIDEO_FORMAT_MPEG4_AVC))
         if ((u_reduce_video_profile(ProfileToPipe(profile)) == PIPE_VIDEO_FORMAT_HEVC))
         else if (u_reduce_video_profile(ProfileToPipe(profile)) == PIPE_VIDEO_FORMAT_AV1)
            case VAConfigAttribEncMaxSlices:
   {
      /**
   * \brief Maximum number of slices per frame. Read-only.
   *
   * This attribute determines the maximum number of slices the
   * driver can support to encode a single frame.
   */
   int maxSlicesPerEncodedPic = pscreen->get_video_param(pscreen, ProfileToPipe(profile),
               if (maxSlicesPerEncodedPic <= 0)
         else
      } break;
   case VAConfigAttribEncMaxRefFrames:
   {
      int maxL0L1ReferencesPerFrame = pscreen->get_video_param(pscreen, ProfileToPipe(profile),
               if (maxL0L1ReferencesPerFrame <= 0)
         else
      } break;
   case VAConfigAttribEncSliceStructure:
   {
      /* The VA enum values match the pipe_video_cap_slice_structure definitions*/
   int supportedSliceStructuresFlagSet = pscreen->get_video_param(pscreen, ProfileToPipe(profile),
               if (supportedSliceStructuresFlagSet <= 0)
         else
      } break;
   case VAConfigAttribEncQualityRange:
   {
      /*
   * this quality range provides different options within the range; and it isn't strictly
   * faster when higher value used.
   * 0, not used; 1, default value; others are using vlVaQualityBits for different modes.
   */
   int quality_range = pscreen->get_video_param(pscreen, ProfileToPipe(profile),
                  } break;
   case VAConfigAttribMaxFrameSize:
   {
      /* Max Frame Size can be used to control picture level frame size.
   * This frame size is in bits.
   */
   value = pscreen->get_video_param(pscreen, ProfileToPipe(profile),
                  } break;
   case VAConfigAttribMaxPictureWidth:
   {
      value = pscreen->get_video_param(pscreen, ProfileToPipe(profile),
                  } break;
   case VAConfigAttribMaxPictureHeight:
   {
      value = pscreen->get_video_param(pscreen, ProfileToPipe(profile),
               #if VA_CHECK_VERSION(1, 12, 0)
            case VAConfigAttribEncHEVCFeatures:
   {
      union pipe_h265_enc_cap_features pipe_features;
   pipe_features.value = 0u;
   /* get_video_param sets pipe_features.bits.config_supported = 1
      to distinguish between supported cap with all bits off and unsupported by driver
      */
   int supportedHEVCEncFeaturesFlagSet = pscreen->get_video_param(pscreen, ProfileToPipe(profile),
               if (supportedHEVCEncFeaturesFlagSet <= 0)
         else {
      /* Assign unsigned typed variable "value" after checking supportedHEVCEncFeaturesFlagSet > 0 */
   pipe_features.value = supportedHEVCEncFeaturesFlagSet;
   VAConfigAttribValEncHEVCFeatures va_features;
   va_features.value = 0;
   va_features.bits.separate_colour_planes = pipe_features.bits.separate_colour_planes;
   va_features.bits.scaling_lists = pipe_features.bits.scaling_lists;
   va_features.bits.amp = pipe_features.bits.amp;
   va_features.bits.sao = pipe_features.bits.sao;
   va_features.bits.pcm = pipe_features.bits.pcm;
   va_features.bits.temporal_mvp = pipe_features.bits.temporal_mvp;
   va_features.bits.strong_intra_smoothing = pipe_features.bits.strong_intra_smoothing;
   va_features.bits.dependent_slices = pipe_features.bits.dependent_slices;
   va_features.bits.sign_data_hiding = pipe_features.bits.sign_data_hiding;
   va_features.bits.constrained_intra_pred = pipe_features.bits.constrained_intra_pred;
   va_features.bits.transform_skip = pipe_features.bits.transform_skip;
   va_features.bits.cu_qp_delta = pipe_features.bits.cu_qp_delta;
   va_features.bits.weighted_prediction = pipe_features.bits.weighted_prediction;
   va_features.bits.transquant_bypass = pipe_features.bits.transquant_bypass;
   va_features.bits.deblocking_filter_disable = pipe_features.bits.deblocking_filter_disable;
         } break;
   case VAConfigAttribEncHEVCBlockSizes:
   {
      union pipe_h265_enc_cap_block_sizes pipe_block_sizes;
   pipe_block_sizes.value = 0;
   /* get_video_param sets pipe_block_sizes.bits.config_supported = 1
      to distinguish between supported cap with all bits off and unsupported by driver
      */
   int supportedHEVCEncBlockSizes = pscreen->get_video_param(pscreen, ProfileToPipe(profile),
               if (supportedHEVCEncBlockSizes <= 0)
         else {
      /* Assign unsigned typed variable "value" after checking supportedHEVCEncBlockSizes > 0 */
   pipe_block_sizes.value = supportedHEVCEncBlockSizes;
   VAConfigAttribValEncHEVCBlockSizes va_block_sizes;
   va_block_sizes.value = 0;
   va_block_sizes.bits.log2_max_coding_tree_block_size_minus3 =
         va_block_sizes.bits.log2_min_coding_tree_block_size_minus3 =
         va_block_sizes.bits.log2_min_luma_coding_block_size_minus3 =
         va_block_sizes.bits.log2_max_luma_transform_block_size_minus2 =
         va_block_sizes.bits.log2_min_luma_transform_block_size_minus2 =
         va_block_sizes.bits.max_max_transform_hierarchy_depth_inter =
         va_block_sizes.bits.min_max_transform_hierarchy_depth_inter =
         va_block_sizes.bits.max_max_transform_hierarchy_depth_intra =
         va_block_sizes.bits.min_max_transform_hierarchy_depth_intra =
         va_block_sizes.bits.log2_max_pcm_coding_block_size_minus3 =
         va_block_sizes.bits.log2_min_pcm_coding_block_size_minus3 =
            #endif
   #if VA_CHECK_VERSION(1, 6, 0)
            case VAConfigAttribPredictionDirection:
   {
      /* The VA enum values match the pipe_h265_enc_pred_direction definitions*/
   int h265_enc_pred_direction = pscreen->get_video_param(pscreen, ProfileToPipe(profile),
               if (h265_enc_pred_direction <= 0)
         else
   #endif
   #if VA_CHECK_VERSION(1, 16, 0)
            case VAConfigAttribEncAV1:
   {
                     int support = pscreen->get_video_param(pscreen, ProfileToPipe(profile),
               if (support <= 0)
         else {
      VAConfigAttribValEncAV1 attrib;
   features.value = support;
   attrib.value = features.value;
         } break;
   case VAConfigAttribEncAV1Ext1:
   {
      union pipe_av1_enc_cap_features_ext1 features_ext1;
   features_ext1.value = 0;
   int support =  pscreen->get_video_param(pscreen, ProfileToPipe(profile),
               if (support <= 0)
         else {
      VAConfigAttribValEncAV1Ext1 attrib;
   features_ext1.value = support;
                  } break;
   case VAConfigAttribEncAV1Ext2:
   {
                     int support = pscreen->get_video_param(pscreen, ProfileToPipe(profile),
               if (support <= 0)
         else {
      VAConfigAttribValEncAV1Ext2 attrib;
   features_ext2.value = support;
                  } break;
   case VAConfigAttribEncTileSupport:
   {
      int encode_tile_support = pscreen->get_video_param(pscreen, ProfileToPipe(profile),
               if (encode_tile_support <= 0)
         else
   #endif
   #if VA_CHECK_VERSION(1, 21, 0)
            case VAConfigAttribEncMaxTileRows:
   {
      int max_tile_rows = pscreen->get_video_param(pscreen, ProfileToPipe(profile),
               if (max_tile_rows <= 0)
         else
      } break;
   case VAConfigAttribEncMaxTileCols:
   {
      int max_tile_cols = pscreen->get_video_param(pscreen, ProfileToPipe(profile),
               if (max_tile_cols <= 0)
         else
   #endif
            default:
      value = VA_ATTRIB_NOT_SUPPORTED;
         } else if (entrypoint == VAEntrypointVideoProc) {
      switch (attrib_list[i].type) {
   case VAConfigAttribRTFormat:
      value = get_screen_supported_va_rt_formats(pscreen,
                  default:
      value = VA_ATTRIB_NOT_SUPPORTED;
         } else {
         }
                  }
      VAStatus
   vlVaCreateConfig(VADriverContextP ctx, VAProfile profile, VAEntrypoint entrypoint,
         {
      vlVaDriver *drv;
   vlVaConfig *config;
   struct pipe_screen *pscreen;
   enum pipe_video_profile p;
            if (!ctx)
            drv = VL_VA_DRIVER(ctx);
            if (!drv)
            config = CALLOC(1, sizeof(vlVaConfig));
   if (!config)
            if (profile == VAProfileNone) {
      if (entrypoint != VAEntrypointVideoProc) {
      FREE(config);
               config->entrypoint = PIPE_VIDEO_ENTRYPOINT_PROCESSING;
   config->profile = PIPE_VIDEO_PROFILE_UNKNOWN;
   supported_rt_formats = get_screen_supported_va_rt_formats(pscreen,
               for (int i = 0; i < num_attribs; i++) {
      if (attrib_list[i].type == VAConfigAttribRTFormat) {
      if (attrib_list[i].value & supported_rt_formats) {
         } else {
      FREE(config);
         } else {
      /*other attrib_types are not supported.*/
   FREE(config);
                  /* Default value if not specified in the input attributes. */
   if (!config->rt_format)
            mtx_lock(&drv->mutex);
   *config_id = handle_table_add(drv->htab, config);
   mtx_unlock(&drv->mutex);
               p = ProfileToPipe(profile);
   if (p == PIPE_VIDEO_PROFILE_UNKNOWN  ||
      (u_reduce_video_profile(p) == PIPE_VIDEO_FORMAT_MPEG4 &&
   !debug_get_option_mpeg4())) {
   FREE(config);
               switch (entrypoint) {
   case VAEntrypointVLD:
      if (!vl_codec_supported(pscreen, p, false)) {
      FREE(config);
   if (!vl_codec_supported(pscreen, p, true))
         else
               config->entrypoint = PIPE_VIDEO_ENTRYPOINT_BITSTREAM;
         case VAEntrypointEncSlice:
      if (!vl_codec_supported(pscreen, p, true)) {
      FREE(config);
   if (!vl_codec_supported(pscreen, p, false))
         else
               config->entrypoint = PIPE_VIDEO_ENTRYPOINT_ENCODE;
         default:
      FREE(config);
   if (!vl_codec_supported(pscreen, p, false) &&
      !vl_codec_supported(pscreen, p, true))
      else
               config->profile = p;
   supported_rt_formats = get_screen_supported_va_rt_formats(pscreen,
               for (int i = 0; i <num_attribs ; i++) {
      if (attrib_list[i].type != VAConfigAttribRTFormat &&
      entrypoint == VAEntrypointVLD ) {
   FREE(config);
      }
   if (attrib_list[i].type == VAConfigAttribRateControl) {
      if (attrib_list[i].value == VA_RC_CBR)
         else if (attrib_list[i].value == VA_RC_VBR)
         else if (attrib_list[i].value == VA_RC_CQP)
         else if (attrib_list[i].value == VA_RC_QVBR &&
               (pscreen->get_video_param(pscreen, ProfileToPipe(profile),
         else {
      FREE(config);
         }
   if (attrib_list[i].type == VAConfigAttribRTFormat) {
      if (attrib_list[i].value & supported_rt_formats) {
         } else {
      FREE(config);
         }
   if (attrib_list[i].type == VAConfigAttribEncPackedHeaders) {
      if (config->entrypoint != PIPE_VIDEO_ENTRYPOINT_ENCODE ||
      (((attrib_list[i].value != 0)) &&
   ((attrib_list[i].value != 1) || u_reduce_video_profile(ProfileToPipe(profile))
         ((attrib_list[i].value != 1) || u_reduce_video_profile(ProfileToPipe(profile))
         ((attrib_list[i].value != 3) || u_reduce_video_profile(ProfileToPipe(profile))
         FREE(config);
                     /* Default value if not specified in the input attributes. */
   if (!config->rt_format)
            mtx_lock(&drv->mutex);
   *config_id = handle_table_add(drv->htab, config);
               }
      VAStatus
   vlVaDestroyConfig(VADriverContextP ctx, VAConfigID config_id)
   {
      vlVaDriver *drv;
            if (!ctx)
                     if (!drv)
            mtx_lock(&drv->mutex);
            if (!config) {
      mtx_unlock(&drv->mutex);
               FREE(config);
   handle_table_remove(drv->htab, config_id);
               }
      VAStatus
   vlVaQueryConfigAttributes(VADriverContextP ctx, VAConfigID config_id, VAProfile *profile,
         {
      vlVaDriver *drv;
            if (!ctx)
                     if (!drv)
            mtx_lock(&drv->mutex);
   config = handle_table_get(drv->htab, config_id);
            if (!config)
                     switch (config->entrypoint) {
   case PIPE_VIDEO_ENTRYPOINT_BITSTREAM:
      *entrypoint = VAEntrypointVLD;
      case PIPE_VIDEO_ENTRYPOINT_ENCODE:
      *entrypoint = VAEntrypointEncSlice;
      case PIPE_VIDEO_ENTRYPOINT_PROCESSING:
      *entrypoint = VAEntrypointVideoProc;
      default:
                  *num_attribs = 1;
   attrib_list[0].type = VAConfigAttribRTFormat;
               }
