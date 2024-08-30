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
      #include "pipe/p_video_codec.h"
      #include "util/u_handle_table.h"
   #include "util/u_video.h"
   #include "util/u_memory.h"
   #include "util/set.h"
      #include "util/vl_vlc.h"
   #include "vl/vl_winsys.h"
      #include "va_private.h"
      static void
   vlVaSetSurfaceContext(vlVaDriver *drv, vlVaSurface *surf, vlVaContext *context)
   {
      if (surf->ctx == context)
            if (surf->ctx) {
      assert(_mesa_set_search(surf->ctx->surfaces, surf));
            /* Only drivers supporting PIPE_VIDEO_ENTRYPOINT_PROCESSING will create
   * decoder for postproc context and thus be able to wait on and destroy
   * the surface fence. On other drivers we need to destroy the fence here
   * otherwise vaQuerySurfaceStatus/vaSyncSurface will fail and we'll also
   * potentially leak the fence.
   */
   if (surf->fence && !context->decoder &&
      context->templat.entrypoint == PIPE_VIDEO_ENTRYPOINT_PROCESSING &&
   surf->ctx->decoder && surf->ctx->decoder->destroy_fence &&
   !drv->pipe->screen->get_video_param(drv->pipe->screen,
                     surf->ctx->decoder->destroy_fence(surf->ctx->decoder, surf->fence);
                  surf->ctx = context;
      }
      VAStatus
   vlVaBeginPicture(VADriverContextP ctx, VAContextID context_id, VASurfaceID render_target)
   {
      vlVaDriver *drv;
   vlVaContext *context;
            if (!ctx)
            drv = VL_VA_DRIVER(ctx);
   if (!drv)
            mtx_lock(&drv->mutex);
   context = handle_table_get(drv->htab, context_id);
   if (!context) {
      mtx_unlock(&drv->mutex);
               if (u_reduce_video_profile(context->templat.profile) == PIPE_VIDEO_FORMAT_MPEG12) {
      context->desc.mpeg12.intra_matrix = NULL;
               surf = handle_table_get(drv->htab, render_target);
   if (!surf || !surf->buffer) {
      mtx_unlock(&drv->mutex);
               context->target_id = render_target;
   vlVaSetSurfaceContext(drv, surf, context);
   context->target = surf->buffer;
                        /* VPP */
   if (context->templat.profile == PIPE_VIDEO_PROFILE_UNKNOWN &&
      context->target->buffer_format != PIPE_FORMAT_B8G8R8A8_UNORM &&
   context->target->buffer_format != PIPE_FORMAT_R8G8B8A8_UNORM &&
   context->target->buffer_format != PIPE_FORMAT_B8G8R8X8_UNORM &&
   context->target->buffer_format != PIPE_FORMAT_R8G8B8X8_UNORM &&
   context->target->buffer_format != PIPE_FORMAT_NV12 &&
   context->target->buffer_format != PIPE_FORMAT_P010 &&
   context->target->buffer_format != PIPE_FORMAT_P016) {
   mtx_unlock(&drv->mutex);
               if (drv->pipe->screen->get_video_param(drv->pipe->screen,
                                    mtx_unlock(&drv->mutex);
               if (context->decoder->entrypoint != PIPE_VIDEO_ENTRYPOINT_ENCODE)
            mtx_unlock(&drv->mutex);
      }
      void
   vlVaGetReferenceFrame(vlVaDriver *drv, VASurfaceID surface_id,
         {
      vlVaSurface *surf = handle_table_get(drv->htab, surface_id);
   if (surf)
         else
      }
   /*
   * in->quality = 0; without any settings, it is using speed preset
   *                  and no preencode and no vbaq. It is the fastest setting.
   * in->quality = 1; suggested setting, with balanced preset, and
   *                  preencode and vbaq
   * in->quality = others; it is the customized setting
   *                  with valid bit (bit #0) set to "1"
   *                  for example:
   *
   *                  0x3  (balance preset, no pre-encoding, no vbaq)
   *                  0x13 (balanced preset, no pre-encoding, vbaq)
   *                  0x13 (balanced preset, no pre-encoding, vbaq)
   *                  0x9  (speed preset, pre-encoding, no vbaq)
   *                  0x19 (speed preset, pre-encoding, vbaq)
   *
   *                  The quality value has to be treated as a combination
   *                  of preset mode, pre-encoding and vbaq settings.
   *                  The quality and speed could be vary according to
   *                  different settings,
   */
   void
   vlVaHandleVAEncMiscParameterTypeQualityLevel(struct pipe_enc_quality_modes *p, vlVaQualityBits *in)
   {
      if (!in->quality) {
      p->level = 0;
   p->preset_mode = PRESET_MODE_SPEED;
   p->pre_encode_mode = PREENCODING_MODE_DISABLE;
                        if (p->level != in->quality) {
      if (in->quality == 1) {
      p->preset_mode = PRESET_MODE_BALANCE;
   p->pre_encode_mode = PREENCODING_MODE_DEFAULT;
      } else {
      p->preset_mode = in->preset_mode > PRESET_MODE_HIGH_QUALITY
         p->pre_encode_mode = in->pre_encode_mode;
         }
      }
      static VAStatus
   handlePictureParameterBuffer(vlVaDriver *drv, vlVaContext *context, vlVaBuffer *buf)
   {
      VAStatus vaStatus = VA_STATUS_SUCCESS;
   enum pipe_video_format format =
            switch (format) {
   case PIPE_VIDEO_FORMAT_MPEG12:
      vlVaHandlePictureParameterBufferMPEG12(drv, context, buf);
         case PIPE_VIDEO_FORMAT_MPEG4_AVC:
      vlVaHandlePictureParameterBufferH264(drv, context, buf);
         case PIPE_VIDEO_FORMAT_VC1:
      vlVaHandlePictureParameterBufferVC1(drv, context, buf);
         case PIPE_VIDEO_FORMAT_MPEG4:
      vlVaHandlePictureParameterBufferMPEG4(drv, context, buf);
         case PIPE_VIDEO_FORMAT_HEVC:
      vlVaHandlePictureParameterBufferHEVC(drv, context, buf);
         case PIPE_VIDEO_FORMAT_JPEG:
      vlVaHandlePictureParameterBufferMJPEG(drv, context, buf);
         case PIPE_VIDEO_FORMAT_VP9:
      vlVaHandlePictureParameterBufferVP9(drv, context, buf);
         case PIPE_VIDEO_FORMAT_AV1:
      vlVaHandlePictureParameterBufferAV1(drv, context, buf);
         default:
                  /* Create the decoder once max_references is known. */
   if (!context->decoder) {
      if (!context->target)
            if (format == PIPE_VIDEO_FORMAT_MPEG4_AVC)
                  context->decoder = drv->pipe->create_video_codec(drv->pipe,
            if (!context->decoder)
                        if (format == PIPE_VIDEO_FORMAT_VP9) {
      context->decoder->width =
         context->decoder->height =
                  }
      static void
   handleIQMatrixBuffer(vlVaContext *context, vlVaBuffer *buf)
   {
      switch (u_reduce_video_profile(context->templat.profile)) {
   case PIPE_VIDEO_FORMAT_MPEG12:
      vlVaHandleIQMatrixBufferMPEG12(context, buf);
         case PIPE_VIDEO_FORMAT_MPEG4_AVC:
      vlVaHandleIQMatrixBufferH264(context, buf);
         case PIPE_VIDEO_FORMAT_MPEG4:
      vlVaHandleIQMatrixBufferMPEG4(context, buf);
         case PIPE_VIDEO_FORMAT_HEVC:
      vlVaHandleIQMatrixBufferHEVC(context, buf);
         case PIPE_VIDEO_FORMAT_JPEG:
      vlVaHandleIQMatrixBufferMJPEG(context, buf);
         default:
            }
      static void
   handleSliceParameterBuffer(vlVaContext *context, vlVaBuffer *buf, unsigned num_slices)
   {
      switch (u_reduce_video_profile(context->templat.profile)) {
   case PIPE_VIDEO_FORMAT_MPEG12:
      vlVaHandleSliceParameterBufferMPEG12(context, buf);
         case PIPE_VIDEO_FORMAT_VC1:
      vlVaHandleSliceParameterBufferVC1(context, buf);
         case PIPE_VIDEO_FORMAT_MPEG4_AVC:
      vlVaHandleSliceParameterBufferH264(context, buf);
         case PIPE_VIDEO_FORMAT_MPEG4:
      vlVaHandleSliceParameterBufferMPEG4(context, buf);
         case PIPE_VIDEO_FORMAT_HEVC:
      vlVaHandleSliceParameterBufferHEVC(context, buf);
         case PIPE_VIDEO_FORMAT_JPEG:
      vlVaHandleSliceParameterBufferMJPEG(context, buf);
         case PIPE_VIDEO_FORMAT_VP9:
      vlVaHandleSliceParameterBufferVP9(context, buf);
         case PIPE_VIDEO_FORMAT_AV1:
      vlVaHandleSliceParameterBufferAV1(context, buf, num_slices);
         default:
            }
      static unsigned int
   bufHasStartcode(vlVaBuffer *buf, unsigned int code, unsigned int bits)
   {
      struct vl_vlc vlc = {0};
            /* search the first 64 bytes for a startcode */
   vl_vlc_init(&vlc, 1, (const void * const*)&buf->data, &buf->size);
   for (i = 0; i < 64 && vl_vlc_bits_left(&vlc) >= bits; ++i) {
      if (vl_vlc_peekbits(&vlc, bits) == code)
         vl_vlc_eatbits(&vlc, 8);
                  }
      static void
   handleVAProtectedSliceDataBufferType(vlVaContext *context, vlVaBuffer *buf)
   {
   uint8_t* encrypted_data = (uint8_t*) buf->data;
            unsigned int drm_key_size = buf->size;
            drm_key = REALLOC(context->desc.base.decrypt_key,
         if (!drm_key)
         memcpy(context->desc.base.decrypt_key, encrypted_data, drm_key_size);
   context->desc.base.key_size = drm_key_size;
   context->desc.base.protected_playback = true;
   }
      static VAStatus
   handleVASliceDataBufferType(vlVaContext *context, vlVaBuffer *buf)
   {
      enum pipe_video_format format = u_reduce_video_profile(context->templat.profile);
   unsigned num_buffers = 0;
   void * const *buffers[3];
   unsigned sizes[3];
   static const uint8_t start_code_h264[] = { 0x00, 0x00, 0x01 };
   static const uint8_t start_code_h265[] = { 0x00, 0x00, 0x01 };
   static const uint8_t start_code_vc1[] = { 0x00, 0x00, 0x01, 0x0d };
            if (!context->decoder)
            format = u_reduce_video_profile(context->templat.profile);
   if (!context->desc.base.protected_playback) {
      switch (format) {
   case PIPE_VIDEO_FORMAT_MPEG4_AVC:
                     buffers[num_buffers] = (void *const)&start_code_h264;
   sizes[num_buffers++] = sizeof(start_code_h264);
      case PIPE_VIDEO_FORMAT_HEVC:
                     buffers[num_buffers] = (void *const)&start_code_h265;
   sizes[num_buffers++] = sizeof(start_code_h265);
      case PIPE_VIDEO_FORMAT_VC1:
      if (bufHasStartcode(buf, 0x0000010d, 32) ||
      bufHasStartcode(buf, 0x0000010c, 32) ||
               if (context->decoder->profile == PIPE_VIDEO_PROFILE_VC1_ADVANCED) {
      buffers[num_buffers] = (void *const)&start_code_vc1;
      }
      case PIPE_VIDEO_FORMAT_MPEG4:
                     vlVaDecoderFixMPEG4Startcode(context);
   buffers[num_buffers] = (void *)context->mpeg4.start_code;
   sizes[num_buffers++] = context->mpeg4.start_code_size;
      case PIPE_VIDEO_FORMAT_JPEG:
                     vlVaGetJpegSliceHeader(context);
   buffers[num_buffers] = (void *)context->mjpeg.slice_header;
   sizes[num_buffers++] = context->mjpeg.slice_header_size;
      case PIPE_VIDEO_FORMAT_VP9:
      if (false == context->desc.base.protected_playback)
            case PIPE_VIDEO_FORMAT_AV1:
         default:
                     buffers[num_buffers] = buf->data;
   sizes[num_buffers] = buf->size;
            if (format == PIPE_VIDEO_FORMAT_JPEG) {
      buffers[num_buffers] = (void *const)&eoi_jpeg;
               if (context->needs_begin_frame) {
      context->decoder->begin_frame(context->decoder, context->target,
            }
   context->decoder->decode_bitstream(context->decoder, context->target, &context->desc.base,
            }
      static VAStatus
   handleVAEncMiscParameterTypeRateControl(vlVaContext *context, VAEncMiscParameterBuffer *misc)
   {
               switch (u_reduce_video_profile(context->templat.profile)) {
   case PIPE_VIDEO_FORMAT_MPEG4_AVC:
      status = vlVaHandleVAEncMiscParameterTypeRateControlH264(context, misc);
         case PIPE_VIDEO_FORMAT_HEVC:
      status = vlVaHandleVAEncMiscParameterTypeRateControlHEVC(context, misc);
      #if VA_CHECK_VERSION(1, 16, 0)
      case PIPE_VIDEO_FORMAT_AV1:
      status = vlVaHandleVAEncMiscParameterTypeRateControlAV1(context, misc);
   #endif
      default:
                     }
      static VAStatus
   handleVAEncMiscParameterTypeFrameRate(vlVaContext *context, VAEncMiscParameterBuffer *misc)
   {
               switch (u_reduce_video_profile(context->templat.profile)) {
   case PIPE_VIDEO_FORMAT_MPEG4_AVC:
      status = vlVaHandleVAEncMiscParameterTypeFrameRateH264(context, misc);
         case PIPE_VIDEO_FORMAT_HEVC:
      status = vlVaHandleVAEncMiscParameterTypeFrameRateHEVC(context, misc);
      #if VA_CHECK_VERSION(1, 16, 0)
      case PIPE_VIDEO_FORMAT_AV1:
      status = vlVaHandleVAEncMiscParameterTypeFrameRateAV1(context, misc);
   #endif
      default:
                     }
      static VAStatus
   handleVAEncMiscParameterTypeTemporalLayer(vlVaContext *context, VAEncMiscParameterBuffer *misc)
   {
               switch (u_reduce_video_profile(context->templat.profile)) {
   case PIPE_VIDEO_FORMAT_MPEG4_AVC:
      status = vlVaHandleVAEncMiscParameterTypeTemporalLayerH264(context, misc);
         case PIPE_VIDEO_FORMAT_HEVC:
            default:
                     }
      static VAStatus
   handleVAEncSequenceParameterBufferType(vlVaDriver *drv, vlVaContext *context, vlVaBuffer *buf)
   {
               switch (u_reduce_video_profile(context->templat.profile)) {
   case PIPE_VIDEO_FORMAT_MPEG4_AVC:
      status = vlVaHandleVAEncSequenceParameterBufferTypeH264(drv, context, buf);
         case PIPE_VIDEO_FORMAT_HEVC:
      status = vlVaHandleVAEncSequenceParameterBufferTypeHEVC(drv, context, buf);
      #if VA_CHECK_VERSION(1, 16, 0)
      case PIPE_VIDEO_FORMAT_AV1:
      status = vlVaHandleVAEncSequenceParameterBufferTypeAV1(drv, context, buf);
   #endif
         default:
                     }
      static VAStatus
   handleVAEncMiscParameterTypeQualityLevel(vlVaContext *context, VAEncMiscParameterBuffer *misc)
   {
               switch (u_reduce_video_profile(context->templat.profile)) {
   case PIPE_VIDEO_FORMAT_MPEG4_AVC:
      status = vlVaHandleVAEncMiscParameterTypeQualityLevelH264(context, misc);
         case PIPE_VIDEO_FORMAT_HEVC:
      status = vlVaHandleVAEncMiscParameterTypeQualityLevelHEVC(context, misc);
      #if VA_CHECK_VERSION(1, 16, 0)
      case PIPE_VIDEO_FORMAT_AV1:
      status = vlVaHandleVAEncMiscParameterTypeQualityLevelAV1(context, misc);
   #endif
         default:
                     }
      static VAStatus
   handleVAEncMiscParameterTypeMaxFrameSize(vlVaContext *context, VAEncMiscParameterBuffer *misc)
   {
               switch (u_reduce_video_profile(context->templat.profile)) {
   case PIPE_VIDEO_FORMAT_MPEG4_AVC:
      status = vlVaHandleVAEncMiscParameterTypeMaxFrameSizeH264(context, misc);
         case PIPE_VIDEO_FORMAT_HEVC:
      status = vlVaHandleVAEncMiscParameterTypeMaxFrameSizeHEVC(context, misc);
      #if VA_CHECK_VERSION(1, 16, 0)
      case PIPE_VIDEO_FORMAT_AV1:
      status = vlVaHandleVAEncMiscParameterTypeMaxFrameSizeAV1(context, misc);
   #endif
         default:
                     }
   static VAStatus
   handleVAEncMiscParameterTypeHRD(vlVaContext *context, VAEncMiscParameterBuffer *misc)
   {
               switch (u_reduce_video_profile(context->templat.profile)) {
   case PIPE_VIDEO_FORMAT_MPEG4_AVC:
      status = vlVaHandleVAEncMiscParameterTypeHRDH264(context, misc);
         case PIPE_VIDEO_FORMAT_HEVC:
      status = vlVaHandleVAEncMiscParameterTypeHRDHEVC(context, misc);
      #if VA_CHECK_VERSION(1, 16, 0)
      case PIPE_VIDEO_FORMAT_AV1:
      status = vlVaHandleVAEncMiscParameterTypeHRDAV1(context, misc);
   #endif
         default:
                     }
      static VAStatus
   handleVAEncMiscParameterBufferType(vlVaContext *context, vlVaBuffer *buf)
   {
      VAStatus vaStatus = VA_STATUS_SUCCESS;
   VAEncMiscParameterBuffer *misc;
            switch (misc->type) {
   case VAEncMiscParameterTypeRateControl:
      vaStatus = handleVAEncMiscParameterTypeRateControl(context, misc);
         case VAEncMiscParameterTypeFrameRate:
      vaStatus = handleVAEncMiscParameterTypeFrameRate(context, misc);
         case VAEncMiscParameterTypeTemporalLayerStructure:
      vaStatus = handleVAEncMiscParameterTypeTemporalLayer(context, misc);
         case VAEncMiscParameterTypeQualityLevel:
      vaStatus = handleVAEncMiscParameterTypeQualityLevel(context, misc);
         case VAEncMiscParameterTypeMaxFrameSize:
      vaStatus = handleVAEncMiscParameterTypeMaxFrameSize(context, misc);
         case VAEncMiscParameterTypeHRD:
      vaStatus = handleVAEncMiscParameterTypeHRD(context, misc);
         default:
                     }
      static VAStatus
   handleVAEncPictureParameterBufferType(vlVaDriver *drv, vlVaContext *context, vlVaBuffer *buf)
   {
               switch (u_reduce_video_profile(context->templat.profile)) {
   case PIPE_VIDEO_FORMAT_MPEG4_AVC:
      status = vlVaHandleVAEncPictureParameterBufferTypeH264(drv, context, buf);
         case PIPE_VIDEO_FORMAT_HEVC:
      status = vlVaHandleVAEncPictureParameterBufferTypeHEVC(drv, context, buf);
      #if VA_CHECK_VERSION(1, 16, 0)
      case PIPE_VIDEO_FORMAT_AV1:
      status = vlVaHandleVAEncPictureParameterBufferTypeAV1(drv, context, buf);
   #endif
         default:
                     }
      static VAStatus
   handleVAEncSliceParameterBufferType(vlVaDriver *drv, vlVaContext *context, vlVaBuffer *buf)
   {
               switch (u_reduce_video_profile(context->templat.profile)) {
   case PIPE_VIDEO_FORMAT_MPEG4_AVC:
      status = vlVaHandleVAEncSliceParameterBufferTypeH264(drv, context, buf);
         case PIPE_VIDEO_FORMAT_HEVC:
      status = vlVaHandleVAEncSliceParameterBufferTypeHEVC(drv, context, buf);
      #if VA_CHECK_VERSION(1, 16, 0)
      case PIPE_VIDEO_FORMAT_AV1:
      status = vlVaHandleVAEncSliceParameterBufferTypeAV1(drv, context, buf);
   #endif
         default:
                     }
      static VAStatus
   handleVAEncPackedHeaderParameterBufferType(vlVaContext *context, vlVaBuffer *buf)
   {
      VAStatus status = VA_STATUS_SUCCESS;
                     switch (u_reduce_video_profile(context->templat.profile)) {
   case PIPE_VIDEO_FORMAT_MPEG4_AVC:
      if (param->type == VAEncPackedHeaderSequence)
         else
            case PIPE_VIDEO_FORMAT_HEVC:
      if (param->type == VAEncPackedHeaderSequence)
         else
            case PIPE_VIDEO_FORMAT_AV1:
                  default:
                     }
      static VAStatus
   handleVAEncPackedHeaderDataBufferType(vlVaContext *context, vlVaBuffer *buf)
   {
               switch (u_reduce_video_profile(context->templat.profile)) {
   case PIPE_VIDEO_FORMAT_MPEG4_AVC:
      if (context->packed_header_type != VAEncPackedHeaderSequence)
            status = vlVaHandleVAEncPackedHeaderDataBufferTypeH264(context, buf);
         case PIPE_VIDEO_FORMAT_HEVC:
      if (context->packed_header_type != VAEncPackedHeaderSequence)
            status = vlVaHandleVAEncPackedHeaderDataBufferTypeHEVC(context, buf);
      #if VA_CHECK_VERSION(1, 16, 0)
      case PIPE_VIDEO_FORMAT_AV1:
      status = vlVaHandleVAEncPackedHeaderDataBufferTypeAV1(context, buf);
   #endif
         default:
                     }
      static VAStatus
   handleVAStatsStatisticsBufferType(VADriverContextP ctx, vlVaContext *context, vlVaBuffer *buf)
   {
      if (context->decoder->entrypoint != PIPE_VIDEO_ENTRYPOINT_ENCODE)
            vlVaDriver *drv;
            if (!drv)
            if (!buf->derived_surface.resource)
      buf->derived_surface.resource = pipe_buffer_create(drv->pipe->screen, PIPE_BIND_VERTEX_BUFFER,
                     }
      VAStatus
   vlVaRenderPicture(VADriverContextP ctx, VAContextID context_id, VABufferID *buffers, int num_buffers)
   {
      vlVaDriver *drv;
   vlVaContext *context;
            unsigned i;
   unsigned slice_idx = 0;
            if (!ctx)
            drv = VL_VA_DRIVER(ctx);
   if (!drv)
            mtx_lock(&drv->mutex);
   context = handle_table_get(drv->htab, context_id);
   if (!context) {
      mtx_unlock(&drv->mutex);
               /* Always process VAProtectedSliceDataBufferType first because it changes the state */
   for (i = 0; i < num_buffers; ++i) {
      vlVaBuffer *buf = handle_table_get(drv->htab, buffers[i]);
   if (!buf) {
      mtx_unlock(&drv->mutex);
               if (buf->type == VAProtectedSliceDataBufferType)
         else if (buf->type == VAEncSequenceParameterBufferType)
               /* Now process VAEncSequenceParameterBufferType where the encoder is created
   * and some default parameters are set to make sure it won't overwrite
   * parameters already set by application from earlier buffers. */
   if (seq_param_buf)
            for (i = 0; i < num_buffers && vaStatus == VA_STATUS_SUCCESS; ++i) {
               switch (buf->type) {
   case VAPictureParameterBufferType:
                  case VAIQMatrixBufferType:
                  case VASliceParameterBufferType:
   {
                        slice_idx is the zero based number of total slices received
      */
   handleSliceParameterBuffer(context, buf, slice_idx);
               case VASliceDataBufferType:
                  case VAProcPipelineParameterBufferType:
                  case VAEncMiscParameterBufferType:
                  case VAEncPictureParameterBufferType:
                  case VAEncSliceParameterBufferType:
                  case VAHuffmanTableBufferType:
                  case VAEncPackedHeaderParameterBufferType:
      handleVAEncPackedHeaderParameterBufferType(context, buf);
      case VAEncPackedHeaderDataBufferType:
                  case VAStatsStatisticsBufferType:
                  default:
            }
               }
      static bool vlVaQueryApplyFilmGrainAV1(vlVaContext *context,
               {
               if (u_reduce_video_profile(context->templat.profile) != PIPE_VIDEO_FORMAT_AV1 ||
      context->decoder->entrypoint != PIPE_VIDEO_ENTRYPOINT_BITSTREAM)
         av1 = &context->desc.av1;
   if (!av1->picture_parameter.film_grain_info.film_grain_info_fields.apply_grain)
            *output_id = av1->picture_parameter.current_frame_id;
   *out_target = &av1->film_grain_target;
      }
      static bool vlVaQueryDecodeInterlacedH264(vlVaContext *context)
   {
               if (u_reduce_video_profile(context->templat.profile) != PIPE_VIDEO_FORMAT_MPEG4_AVC ||
      context->decoder->entrypoint != PIPE_VIDEO_ENTRYPOINT_BITSTREAM)
                  if (h264->pps->sps->frame_mbs_only_flag)
            return h264->field_pic_flag || /* PAFF */
      }
      VAStatus
   vlVaEndPicture(VADriverContextP ctx, VAContextID context_id)
   {
      vlVaDriver *drv;
   vlVaContext *context;
   vlVaBuffer *coded_buf;
   vlVaSurface *surf;
   void *feedback = NULL;
   struct pipe_screen *screen;
   bool supported;
   bool realloc = false;
   bool apply_av1_fg = false;
   enum pipe_format format;
   struct pipe_video_buffer **out_target;
   int output_id;
            if (!ctx)
            drv = VL_VA_DRIVER(ctx);
   if (!drv)
            mtx_lock(&drv->mutex);
   context = handle_table_get(drv->htab, context_id);
   mtx_unlock(&drv->mutex);
   if (!context)
            if (!context->decoder) {
      if (context->templat.profile != PIPE_VIDEO_PROFILE_UNKNOWN)
            /* VPP */
               output_id = context->target_id;
   out_target = &context->target;
   apply_av1_fg = vlVaQueryApplyFilmGrainAV1(context, &output_id, &out_target);
            mtx_lock(&drv->mutex);
   surf = handle_table_get(drv->htab, output_id);
   if (!surf || !surf->buffer) {
      mtx_unlock(&drv->mutex);
               if (apply_av1_fg) {
      vlVaSetSurfaceContext(drv, surf, context);
                        screen = context->decoder->context->screen;
   supported = screen->get_video_param(screen, context->decoder->profile,
                              if (!supported) {
      surf->templat.interlaced = screen->get_video_param(screen,
                        } else if (decode_interlaced && !surf->buffer->interlaced) {
      surf->templat.interlaced = true;
               format = screen->get_video_param(screen, context->decoder->profile,
                  if (surf->buffer->buffer_format != format &&
      surf->buffer->buffer_format == PIPE_FORMAT_NV12) {
   /* check originally as NV12 only */
   surf->templat.buffer_format = format;
               if (u_reduce_video_profile(context->templat.profile) == PIPE_VIDEO_FORMAT_JPEG) {
      if (surf->buffer->buffer_format == PIPE_FORMAT_NV12 &&
      context->mjpeg.sampling_factor != MJPEG_SAMPLING_FACTOR_NV12) {
   /* workaround to reallocate surface buffer with right format
   * if it doesnt match with sampling_factor. ffmpeg doesnt
   * use VASurfaceAttribPixelFormat and defaults to NV12.
   */
   switch (context->mjpeg.sampling_factor) {
      case MJPEG_SAMPLING_FACTOR_YUV422:
   case MJPEG_SAMPLING_FACTOR_YUY2:
      surf->templat.buffer_format = PIPE_FORMAT_YUYV;
      case MJPEG_SAMPLING_FACTOR_YUV444:
      surf->templat.buffer_format = PIPE_FORMAT_Y8_U8_V8_444_UNORM;
      case MJPEG_SAMPLING_FACTOR_YUV400:
      surf->templat.buffer_format = PIPE_FORMAT_Y8_400_UNORM;
      default:
      mtx_unlock(&drv->mutex);
   }
      }
   /* check if format is supported before proceeding with realloc,
   * also avoid submission if hardware doesnt support the format and
   * applcation failed to check the supported rt_formats.
   */
   if (!screen->is_video_format_supported(screen, surf->templat.buffer_format,
      PIPE_VIDEO_PROFILE_JPEG_BASELINE, PIPE_VIDEO_ENTRYPOINT_BITSTREAM)) {
   mtx_unlock(&drv->mutex);
                  if ((bool)(surf->templat.bind & PIPE_BIND_PROTECTED) != context->desc.base.protected_playback) {
      if (context->desc.base.protected_playback) {
         }
   else
                     if (u_reduce_video_profile(context->templat.profile) == PIPE_VIDEO_FORMAT_AV1 &&
      surf->buffer->buffer_format == PIPE_FORMAT_NV12 &&
   context->decoder->entrypoint == PIPE_VIDEO_ENTRYPOINT_BITSTREAM) {
   if (context->desc.av1.picture_parameter.bit_depth_idx == 1) {
      surf->templat.buffer_format = PIPE_FORMAT_P010;
                  if (realloc) {
               if (vlVaHandleSurfaceAllocate(drv, surf, &surf->templat, NULL, 0) != VA_STATUS_SUCCESS) {
      mtx_unlock(&drv->mutex);
               if (context->decoder->entrypoint == PIPE_VIDEO_ENTRYPOINT_ENCODE) {
                        dst_rect.x0 = src_rect.x0 = 0;
   dst_rect.y0 = src_rect.y0 = 0;
   dst_rect.x1 = src_rect.x1 = surf->templat.width;
   dst_rect.y1 = src_rect.y1 = surf->templat.height;
   vl_compositor_yuv_deint_full(&drv->cstate, &drv->compositor,
            } else {
      /* Can't convert from progressive to interlaced yet */
   mtx_unlock(&drv->mutex);
                  old_buf->destroy(old_buf);
               if (context->decoder->entrypoint == PIPE_VIDEO_ENTRYPOINT_ENCODE) {
      context->desc.base.fence = &surf->fence;
   struct pipe_screen *screen = context->decoder->context->screen;
   coded_buf = context->coded_buf;
   if (u_reduce_video_profile(context->templat.profile) == PIPE_VIDEO_FORMAT_MPEG4_AVC)
            /* keep other path the same way */
   if (!screen->get_video_param(screen, context->templat.profile,
                     if (u_reduce_video_profile(context->templat.profile) == PIPE_VIDEO_FORMAT_MPEG4_AVC)
         else if (u_reduce_video_profile(context->templat.profile) == PIPE_VIDEO_FORMAT_HEVC)
               context->desc.base.input_format = surf->buffer->buffer_format;
   context->desc.base.input_full_range = surf->full_range;
            context->decoder->begin_frame(context->decoder, context->target, &context->desc.base);
   context->decoder->encode_bitstream(context->decoder, context->target,
         coded_buf->feedback = feedback;
   coded_buf->ctx = context_id;
   surf->feedback = feedback;
   surf->coded_buf = coded_buf;
      } else if (context->decoder->entrypoint == PIPE_VIDEO_ENTRYPOINT_BITSTREAM) {
         } else if (context->decoder->entrypoint == PIPE_VIDEO_ENTRYPOINT_PROCESSING) {
                           if (drv->pipe->screen->get_video_param(drv->pipe->screen,
                           else {
      if (context->decoder->entrypoint == PIPE_VIDEO_ENTRYPOINT_ENCODE &&
      u_reduce_video_profile(context->templat.profile) == PIPE_VIDEO_FORMAT_MPEG4_AVC) {
   int idr_period = context->desc.h264enc.gop_size / context->gop_coeff;
   int p_remain_in_idr = idr_period - context->desc.h264enc.frame_num;
   surf->frame_num_cnt = context->desc.h264enc.frame_num_cnt;
   surf->force_flushed = false;
   if (context->first_single_submitted) {
      context->decoder->flush(context->decoder);
   context->first_single_submitted = false;
      }
   if (p_remain_in_idr == 1) {
      if ((context->desc.h264enc.frame_num_cnt % 2) != 0) {
      context->decoder->flush(context->decoder);
      }
   else
                           /* Update frame_num disregarding PIPE_VIDEO_CAP_REQUIRES_FLUSH_ON_END_FRAME check above */
   if (context->decoder->entrypoint == PIPE_VIDEO_ENTRYPOINT_ENCODE) {
      if ((u_reduce_video_profile(context->templat.profile) == PIPE_VIDEO_FORMAT_MPEG4_AVC)
      && (!context->desc.h264enc.not_referenced))
      else if (u_reduce_video_profile(context->templat.profile) == PIPE_VIDEO_FORMAT_HEVC)
         else if (u_reduce_video_profile(context->templat.profile) == PIPE_VIDEO_FORMAT_AV1)
               mtx_unlock(&drv->mutex);
      }
