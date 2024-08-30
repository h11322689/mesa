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
   #include "pipe/p_video_codec.h"
   #include "util/u_memory.h"
   #include "util/u_handle_table.h"
   #include "util/u_video.h"
   #include "util/set.h"
   #include "vl/vl_deint_filter.h"
   #include "vl/vl_winsys.h"
      #include "va_private.h"
   #ifdef HAVE_DRISW_KMS
   #include "loader/loader.h"
   #endif
      #include <va/va_drmcommon.h>
      static struct VADriverVTable vtable =
   {
      &vlVaTerminate,
   &vlVaQueryConfigProfiles,
   &vlVaQueryConfigEntrypoints,
   &vlVaGetConfigAttributes,
   &vlVaCreateConfig,
   &vlVaDestroyConfig,
   &vlVaQueryConfigAttributes,
   &vlVaCreateSurfaces,
   &vlVaDestroySurfaces,
   &vlVaCreateContext,
   &vlVaDestroyContext,
   &vlVaCreateBuffer,
   &vlVaBufferSetNumElements,
   &vlVaMapBuffer,
   &vlVaUnmapBuffer,
   &vlVaDestroyBuffer,
   &vlVaBeginPicture,
   &vlVaRenderPicture,
   &vlVaEndPicture,
   &vlVaSyncSurface,
   &vlVaQuerySurfaceStatus,
   &vlVaQuerySurfaceError,
   &vlVaPutSurface,
   &vlVaQueryImageFormats,
   &vlVaCreateImage,
   &vlVaDeriveImage,
   &vlVaDestroyImage,
   &vlVaSetImagePalette,
   &vlVaGetImage,
   &vlVaPutImage,
   &vlVaQuerySubpictureFormats,
   &vlVaCreateSubpicture,
   &vlVaDestroySubpicture,
   &vlVaSubpictureImage,
   &vlVaSetSubpictureChromakey,
   &vlVaSetSubpictureGlobalAlpha,
   &vlVaAssociateSubpicture,
   &vlVaDeassociateSubpicture,
   &vlVaQueryDisplayAttributes,
   &vlVaGetDisplayAttributes,
   &vlVaSetDisplayAttributes,
   &vlVaBufferInfo,
   &vlVaLockSurface,
   &vlVaUnlockSurface,
   NULL, /* DEPRECATED VaGetSurfaceAttributes */
   &vlVaCreateSurfaces2,
   &vlVaQuerySurfaceAttributes,
   &vlVaAcquireBufferHandle,
      #if VA_CHECK_VERSION(1, 1, 0)
      NULL, /* vaCreateMFContext */
   NULL, /* vaMFAddContext */
   NULL, /* vaMFReleaseContext */
   NULL, /* vaMFSubmit */
   NULL, /* vaCreateBuffer2 */
   NULL, /* vaQueryProcessingRate */
      #endif
   #if VA_CHECK_VERSION(1, 15, 0)
      NULL, /* vaSyncSurface2 */
      #endif
   #if VA_CHECK_VERSION(1, 21, 0)
      NULL, /* vaCopy */
      #endif
   };
      static struct VADriverVTableVPP vtable_vpp =
   {
      1,
   &vlVaQueryVideoProcFilters,
   &vlVaQueryVideoProcFilterCaps,
      };
      VA_PUBLIC_API VAStatus
   VA_DRIVER_INIT_FUNC(VADriverContextP ctx)
   {
               if (!ctx)
            drv = CALLOC(1, sizeof(vlVaDriver));
   if (!drv)
               #ifdef _WIN32
      case VA_DISPLAY_WIN32: {
      drv->vscreen = vl_win32_screen_create(ctx->native_dpy);
         #else
      case VA_DISPLAY_ANDROID:
      FREE(drv);
      case VA_DISPLAY_GLX:
   case VA_DISPLAY_X11:
      drv->vscreen = vl_dri3_screen_create(ctx->native_dpy, ctx->x11_screen);
   if (!drv->vscreen)
         if (!drv->vscreen)
            case VA_DISPLAY_WAYLAND:
   case VA_DISPLAY_DRM:
   case VA_DISPLAY_DRM_RENDERNODES: {
               if (!drm_info || drm_info->fd < 0) {
      FREE(drv);
      #ifdef HAVE_DRISW_KMS
         char* drm_driver_name = loader_get_driver_for_fd(drm_info->fd);
   if(drm_driver_name) {
      if (strcmp(drm_driver_name, "vgem") == 0)
            #endif
         if(!drv->vscreen)
               #endif
      default:
      FREE(drv);
               if (!drv->vscreen)
            drv->pipe = pipe_create_multimedia_context(drv->vscreen->pscreen);
   if (!drv->pipe)
            drv->htab = handle_table_create();
   if (!drv->htab)
            if (!vl_compositor_init(&drv->compositor, drv->pipe))
         if (!vl_compositor_init_state(&drv->cstate, drv->pipe))
            vl_csc_get_matrix(VL_CSC_COLOR_STANDARD_BT_601, NULL, true, &drv->csc);
   if (!vl_compositor_set_csc_matrix(&drv->cstate, (const vl_csc_matrix *)&drv->csc, 1.0f, 0.0f))
                  ctx->pDriverData = (void *)drv;
   ctx->version_major = 0;
   ctx->version_minor = 1;
   *ctx->vtable = vtable;
   *ctx->vtable_vpp = vtable_vpp;
   ctx->max_profiles = PIPE_VIDEO_PROFILE_MAX - PIPE_VIDEO_PROFILE_UNKNOWN - 1;
   ctx->max_entrypoints = 2;
   ctx->max_attributes = 1;
   ctx->max_image_formats = VL_VA_MAX_IMAGE_FORMATS;
   ctx->max_subpic_formats = 1;
            snprintf(drv->vendor_string, sizeof(drv->vendor_string),
                              error_csc_matrix:
            error_compositor_state:
            error_compositor:
            error_htab:
            error_pipe:
            error_screen:
      FREE(drv);
      }
      VAStatus
   vlVaCreateContext(VADriverContextP ctx, VAConfigID config_id, int picture_width,
               {
      vlVaDriver *drv;
   vlVaContext *context;
   vlVaConfig *config;
   int is_vpp;
   int min_supported_width, min_supported_height;
            if (!ctx)
            drv = VL_VA_DRIVER(ctx);
   mtx_lock(&drv->mutex);
   config = handle_table_get(drv->htab, config_id);
            if (!config)
            is_vpp = config->profile == PIPE_VIDEO_PROFILE_UNKNOWN && !picture_width &&
            if (!(picture_width && picture_height) && !is_vpp)
            context = CALLOC(1, sizeof(vlVaContext));
   if (!context)
            if (is_vpp && !drv->vscreen->pscreen->get_video_param(drv->vscreen->pscreen,
                           } else {
      if (config->entrypoint != PIPE_VIDEO_ENTRYPOINT_PROCESSING) {
      min_supported_width = drv->vscreen->pscreen->get_video_param(drv->vscreen->pscreen,
               min_supported_height = drv->vscreen->pscreen->get_video_param(drv->vscreen->pscreen,
               max_supported_width = drv->vscreen->pscreen->get_video_param(drv->vscreen->pscreen,
               max_supported_height = drv->vscreen->pscreen->get_video_param(drv->vscreen->pscreen,
                  if (picture_width < min_supported_width || picture_height < min_supported_height ||
      picture_width > max_supported_width || picture_height > max_supported_height) {
   FREE(context);
         }
   context->templat.profile = config->profile;
   context->templat.entrypoint = config->entrypoint;
   context->templat.chroma_format = PIPE_VIDEO_CHROMA_FORMAT_420;
   context->templat.width = picture_width;
   context->templat.height = picture_height;
            switch (u_reduce_video_profile(context->templat.profile)) {
   case PIPE_VIDEO_FORMAT_MPEG12:
   case PIPE_VIDEO_FORMAT_VC1:
   case PIPE_VIDEO_FORMAT_MPEG4:
                  case PIPE_VIDEO_FORMAT_MPEG4_AVC:
      context->templat.max_references = 0;
   if (config->entrypoint != PIPE_VIDEO_ENTRYPOINT_ENCODE) {
      context->desc.h264.pps = CALLOC_STRUCT(pipe_h264_pps);
   if (!context->desc.h264.pps) {
      FREE(context);
      }
   context->desc.h264.pps->sps = CALLOC_STRUCT(pipe_h264_sps);
   if (!context->desc.h264.pps->sps) {
      FREE(context->desc.h264.pps);
   FREE(context);
                  case PIPE_VIDEO_FORMAT_HEVC:
         if (config->entrypoint != PIPE_VIDEO_ENTRYPOINT_ENCODE) {
      context->desc.h265.pps = CALLOC_STRUCT(pipe_h265_pps);
   if (!context->desc.h265.pps) {
      FREE(context);
      }
   context->desc.h265.pps->sps = CALLOC_STRUCT(pipe_h265_sps);
   if (!context->desc.h265.pps->sps) {
      FREE(context->desc.h265.pps);
   FREE(context);
                     case PIPE_VIDEO_FORMAT_VP9:
            default:
                     context->desc.base.profile = config->profile;
   context->desc.base.entry_point = config->entrypoint;
   if (config->entrypoint == PIPE_VIDEO_ENTRYPOINT_ENCODE) {
      switch (u_reduce_video_profile(context->templat.profile)) {
   case PIPE_VIDEO_FORMAT_MPEG4_AVC:
      context->desc.h264enc.rate_ctrl[0].rate_ctrl_method = config->rc;
   context->desc.h264enc.frame_idx = util_hash_table_create_ptr_keys();
      case PIPE_VIDEO_FORMAT_HEVC:
      context->desc.h265enc.rc.rate_ctrl_method = config->rc;
   context->desc.h265enc.frame_idx = util_hash_table_create_ptr_keys();
      case PIPE_VIDEO_FORMAT_AV1:
      context->desc.av1enc.rc[0].rate_ctrl_method = config->rc;
      default:
                              mtx_lock(&drv->mutex);
   *context_id = handle_table_add(drv->htab, context);
               }
      VAStatus
   vlVaDestroyContext(VADriverContextP ctx, VAContextID context_id)
   {
      vlVaDriver *drv;
            if (!ctx)
            if (context_id == 0)
            drv = VL_VA_DRIVER(ctx);
   mtx_lock(&drv->mutex);
   context = handle_table_get(drv->htab, context_id);
   if (!context) {
      mtx_unlock(&drv->mutex);
               set_foreach(context->surfaces, entry) {
      vlVaSurface *surf = (vlVaSurface *)entry->key;
   assert(surf->ctx == context);
   surf->ctx = NULL;
   if (surf->fence && context->decoder && context->decoder->destroy_fence) {
      context->decoder->destroy_fence(context->decoder, surf->fence);
         }
            if (context->decoder) {
      if (context->desc.base.entry_point == PIPE_VIDEO_ENTRYPOINT_ENCODE) {
      if (u_reduce_video_profile(context->decoder->profile) ==
      PIPE_VIDEO_FORMAT_MPEG4_AVC) {
   if (context->desc.h264enc.frame_idx)
      }
   if (u_reduce_video_profile(context->decoder->profile) ==
      PIPE_VIDEO_FORMAT_HEVC) {
   if (context->desc.h265enc.frame_idx)
         } else {
      if (u_reduce_video_profile(context->decoder->profile) ==
            FREE(context->desc.h264.pps->sps);
      }
   if (u_reduce_video_profile(context->decoder->profile) ==
            FREE(context->desc.h265.pps->sps);
         }
      }
   if (context->blit_cs)
         if (context->deint) {
      vl_deint_filter_cleanup(context->deint);
      }
   FREE(context->desc.base.decrypt_key);
   FREE(context);
   handle_table_remove(drv->htab, context_id);
               }
      VAStatus
   vlVaTerminate(VADriverContextP ctx)
   {
               if (!ctx)
            drv = ctx->pDriverData;
   vl_compositor_cleanup_state(&drv->cstate);
   vl_compositor_cleanup(&drv->compositor);
   drv->pipe->destroy(drv->pipe);
   drv->vscreen->destroy(drv->vscreen);
   handle_table_destroy(drv->htab);
   mtx_destroy(&drv->mutex);
               }
