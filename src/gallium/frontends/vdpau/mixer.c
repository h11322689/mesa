   /**************************************************************************
   *
   * Copyright 2010 Thomas Balling Sørensen.
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
   * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      #include <vdpau/vdpau.h>
      #include "util/u_memory.h"
   #include "util/u_debug.h"
      #include "vl/vl_csc.h"
      #include "vdpau_private.h"
      /**
   * Create a VdpVideoMixer.
   */
   VdpStatus
   vlVdpVideoMixerCreate(VdpDevice device,
                        uint32_t feature_count,
   VdpVideoMixerFeature const *features,
      {
      vlVdpVideoMixer *vmixer = NULL;
   VdpStatus ret;
   struct pipe_screen *screen;
            vlVdpDevice *dev = vlGetDataHTAB(device);
   if (!dev)
                  vmixer = CALLOC(1, sizeof(vlVdpVideoMixer));
   if (!vmixer)
                              if (!vl_compositor_init_state(&vmixer->cstate, dev->context)) {
      ret = VDP_STATUS_ERROR;
               vl_csc_get_matrix(VL_CSC_COLOR_STANDARD_BT_601, NULL, true, &vmixer->csc);
   if (!debug_get_bool_option("G3DVL_NO_CSC", false)) {
      if (!vl_compositor_set_csc_matrix(&vmixer->cstate, (const vl_csc_matrix *)&vmixer->csc, 1.0f, 0.0f)) {
      ret = VDP_STATUS_ERROR;
                  *mixer = vlAddDataHTAB(vmixer);
   if (*mixer == 0) {
      ret = VDP_STATUS_ERROR;
               ret = VDP_STATUS_INVALID_VIDEO_MIXER_FEATURE;
   for (i = 0; i < feature_count; ++i) {
      switch (features[i]) {
   /* they are valid, but we doesn't support them */
   case VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL_SPATIAL:
   case VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L2:
   case VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L3:
   case VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L4:
   case VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L5:
   case VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L6:
   case VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L7:
   case VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L8:
   case VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L9:
   case VDP_VIDEO_MIXER_FEATURE_INVERSE_TELECINE:
            case VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL:
                  case VDP_VIDEO_MIXER_FEATURE_SHARPNESS:
                  case VDP_VIDEO_MIXER_FEATURE_NOISE_REDUCTION:
                  case VDP_VIDEO_MIXER_FEATURE_LUMA_KEY:
                  case VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L1:
      vmixer->bicubic.supported = true;
      default: goto no_params;
               vmixer->chroma_format = PIPE_VIDEO_CHROMA_FORMAT_420;
   ret = VDP_STATUS_INVALID_VIDEO_MIXER_PARAMETER;
   for (i = 0; i < parameter_count; ++i) {
      switch (parameters[i]) {
   case VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_WIDTH:
      vmixer->video_width = *(uint32_t*)parameter_values[i];
      case VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_HEIGHT:
      vmixer->video_height = *(uint32_t*)parameter_values[i];
      case VDP_VIDEO_MIXER_PARAMETER_CHROMA_TYPE:
      vmixer->chroma_format = ChromaToPipe(*(VdpChromaType*)parameter_values[i]);
      case VDP_VIDEO_MIXER_PARAMETER_LAYERS:
      vmixer->max_layers = *(uint32_t*)parameter_values[i];
      default: goto no_params;
      }
   ret = VDP_STATUS_INVALID_VALUE;
   if (vmixer->max_layers > 4) {
      VDPAU_MSG(VDPAU_WARN, "[VDPAU] Max layers %u > 4 not supported\n", vmixer->max_layers);
               max_size = screen->get_param(screen, PIPE_CAP_MAX_TEXTURE_2D_SIZE);
   if (vmixer->video_width < 48 || vmixer->video_width > max_size) {
      VDPAU_MSG(VDPAU_WARN, "[VDPAU] 48 < %u < %u not valid for width\n",
            }
   if (vmixer->video_height < 48 || vmixer->video_height > max_size) {
      VDPAU_MSG(VDPAU_WARN, "[VDPAU] 48 < %u < %u  not valid for height\n",
            }
   vmixer->luma_key.luma_min = 1.0f;
   vmixer->luma_key.luma_max = 0.0f;
                  no_params:
            no_handle:
   err_csc_matrix:
         no_compositor_state:
      mtx_unlock(&dev->mutex);
   DeviceReference(&vmixer->device, NULL);
   FREE(vmixer);
      }
      /**
   * Destroy a VdpVideoMixer.
   */
   VdpStatus
   vlVdpVideoMixerDestroy(VdpVideoMixer mixer)
   {
               vmixer = vlGetDataHTAB(mixer);
   if (!vmixer)
                                       if (vmixer->deint.filter) {
      vl_deint_filter_cleanup(vmixer->deint.filter);
               if (vmixer->noise_reduction.filter) {
      vl_median_filter_cleanup(vmixer->noise_reduction.filter);
               if (vmixer->sharpness.filter) {
      vl_matrix_filter_cleanup(vmixer->sharpness.filter);
               if (vmixer->bicubic.filter) {
      vl_bicubic_filter_cleanup(vmixer->bicubic.filter);
      }
   mtx_unlock(&vmixer->device->mutex);
                        }
      /**
   * Perform a video post-processing and compositing operation.
   */
   VdpStatus vlVdpVideoMixerRender(VdpVideoMixer mixer,
                                 VdpOutputSurface background_surface,
   VdpRect const *background_source_rect,
   VdpVideoMixerPictureStructure current_picture_structure,
   uint32_t video_surface_past_count,
   VdpVideoSurface const *video_surface_past,
   VdpVideoSurface video_surface_current,
   uint32_t video_surface_future_count,
   VdpVideoSurface const *video_surface_future,
   VdpRect const *video_source_rect,
   {
      enum vl_compositor_deinterlace deinterlace;
   struct u_rect rect, clip, *prect, dirty_area;
   unsigned i, layer = 0;
   struct pipe_video_buffer *video_buffer;
   struct pipe_sampler_view *sampler_view, sv_templ;
   struct pipe_surface *surface, surf_templ;
   struct pipe_context *pipe = NULL;
            vlVdpVideoMixer *vmixer;
   vlVdpSurface *surf;
                     vmixer = vlGetDataHTAB(mixer);
   if (!vmixer)
                     surf = vlGetDataHTAB(video_surface_current);
   if (!surf)
                  if (surf->device != vmixer->device)
            if (vmixer->video_width > video_buffer->width ||
      vmixer->video_height > video_buffer->height ||
   vmixer->chroma_format != pipe_format_to_chroma_format(video_buffer->buffer_format))
         if (layer_count > vmixer->max_layers)
            dst = vlGetDataHTAB(destination_surface);
   if (!dst)
            if (background_surface != VDP_INVALID_HANDLE) {
      bg = vlGetDataHTAB(background_surface);
   if (!bg)
                                 if (bg)
      vl_compositor_set_rgba_layer(&vmixer->cstate, compositor, layer++, bg->sampler_view,
         switch (current_picture_structure) {
   case VDP_VIDEO_MIXER_PICTURE_STRUCTURE_TOP_FIELD:
      deinterlace = VL_COMPOSITOR_BOB_TOP;
         case VDP_VIDEO_MIXER_PICTURE_STRUCTURE_BOTTOM_FIELD:
      deinterlace = VL_COMPOSITOR_BOB_BOTTOM;
         case VDP_VIDEO_MIXER_PICTURE_STRUCTURE_FRAME:
      deinterlace = VL_COMPOSITOR_WEAVE;
         default:
      mtx_unlock(&vmixer->device->mutex);
               if (deinterlace != VL_COMPOSITOR_WEAVE && vmixer->deint.enabled &&
      video_surface_past_count > 1 && video_surface_future_count > 0) {
   vlVdpSurface *prevprev = vlGetDataHTAB(video_surface_past[1]);
   vlVdpSurface *prev = vlGetDataHTAB(video_surface_past[0]);
   vlVdpSurface *next = vlGetDataHTAB(video_surface_future[0]);
   if (prevprev && prev && next &&
      vl_deint_filter_check_buffers(vmixer->deint.filter,
   prevprev->video_buffer, prev->video_buffer, surf->video_buffer, next->video_buffer)) {
   vl_deint_filter_render(vmixer->deint.filter, prevprev->video_buffer,
                     deinterlace = VL_COMPOSITOR_WEAVE;
                  if (!destination_video_rect)
            prect = RectToPipe(video_source_rect, &rect);
   if (!prect) {
      rect.x0 = 0;
   rect.y0 = 0;
   rect.x1 = surf->templat.width;
   rect.y1 = surf->templat.height;
      }
            if (vmixer->bicubic.filter || vmixer->sharpness.filter || vmixer->noise_reduction.filter) {
      pipe = vmixer->device->context;
            res_tmpl.target = PIPE_TEXTURE_2D;
   res_tmpl.format = dst->sampler_view->format;
   res_tmpl.depth0 = 1;
   res_tmpl.array_size = 1;
   res_tmpl.bind = PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET;
            if (!vmixer->bicubic.filter) {
      res_tmpl.width0 = dst->surface->width;
      } else {
      res_tmpl.width0 = surf->templat.width;
                        vlVdpDefaultSamplerViewTemplate(&sv_templ, res);
            memset(&surf_templ, 0, sizeof(surf_templ));
   surf_templ.format = res->format;
            vl_compositor_reset_dirty_area(&dirty_area);
      } else {
      surface = dst->surface;
   sampler_view = dst->sampler_view;
               if (!vmixer->bicubic.filter) {
      vl_compositor_set_layer_dst_area(&vmixer->cstate, layer++, RectToPipe(destination_video_rect, &rect));
               for (i = 0; i < layer_count; ++i) {
      vlVdpOutputSurface *src = vlGetDataHTAB(layers->source_surface);
   if (!src) {
      mtx_unlock(&vmixer->device->mutex);
                        vl_compositor_set_rgba_layer(&vmixer->cstate, compositor, layer, src->sampler_view,
                                       if (vmixer->noise_reduction.filter) {
      if (!vmixer->sharpness.filter && !vmixer->bicubic.filter) {
      vl_median_filter_render(vmixer->noise_reduction.filter,
      } else {
      res = pipe->screen->resource_create(pipe->screen, &res_tmpl);
   struct pipe_sampler_view *sampler_view_temp = pipe->create_sampler_view(pipe, res, &sv_templ);
                                                sampler_view = sampler_view_temp;
                  if (vmixer->sharpness.filter) {
      if (!vmixer->bicubic.filter) {
      vl_matrix_filter_render(vmixer->sharpness.filter,
      } else {
      res = pipe->screen->resource_create(pipe->screen, &res_tmpl);
   struct pipe_sampler_view *sampler_view_temp = pipe->create_sampler_view(pipe, res, &sv_templ);
                                                sampler_view = sampler_view_temp;
                  if (vmixer->bicubic.filter)
      vl_bicubic_filter_render(vmixer->bicubic.filter,
                     if(surface != dst->surface) {
      pipe_sampler_view_reference(&sampler_view, NULL);
      }
               }
      static void
   vlVdpVideoMixerUpdateDeinterlaceFilter(vlVdpVideoMixer *vmixer)
   {
      struct pipe_context *pipe = vmixer->device->context;
            /* remove existing filter */
   if (vmixer->deint.filter) {
      vl_deint_filter_cleanup(vmixer->deint.filter);
   FREE(vmixer->deint.filter);
               /* create a new filter if requested */
   if (vmixer->deint.enabled && vmixer->chroma_format == PIPE_VIDEO_CHROMA_FORMAT_420) {
      vmixer->deint.filter = MALLOC(sizeof(struct vl_deint_filter));
   vmixer->deint.enabled = vl_deint_filter_init(vmixer->deint.filter, pipe,
         vmixer->video_width, vmixer->video_height,
   if (!vmixer->deint.enabled) {
               }
      /**
   * Update the noise reduction setting
   */
   static void
   vlVdpVideoMixerUpdateNoiseReductionFilter(vlVdpVideoMixer *vmixer)
   {
               /* if present remove the old filter first */
   if (vmixer->noise_reduction.filter) {
      vl_median_filter_cleanup(vmixer->noise_reduction.filter);
   FREE(vmixer->noise_reduction.filter);
               /* and create a new filter as needed */
   if (vmixer->noise_reduction. enabled && vmixer->noise_reduction.level > 0) {
      vmixer->noise_reduction.filter = MALLOC(sizeof(struct vl_median_filter));
   vl_median_filter_init(vmixer->noise_reduction.filter, vmixer->device->context,
                     }
      static void
   vlVdpVideoMixerUpdateSharpnessFilter(vlVdpVideoMixer *vmixer)
   {
               /* if present remove the old filter first */
   if (vmixer->sharpness.filter) {
      vl_matrix_filter_cleanup(vmixer->sharpness.filter);
   FREE(vmixer->sharpness.filter);
               /* and create a new filter as needed */
   if (vmixer->sharpness.enabled && vmixer->sharpness.value != 0.0f) {
      float matrix[9];
            if (vmixer->sharpness.value > 0.0f) {
      matrix[0] = -1.0f; matrix[1] = -1.0f; matrix[2] = -1.0f;
                                       } else {
      matrix[0] = 1.0f; matrix[1] = 2.0f; matrix[2] = 1.0f;
                                             vmixer->sharpness.filter = MALLOC(sizeof(struct vl_matrix_filter));
   vl_matrix_filter_init(vmixer->sharpness.filter, vmixer->device->context,
               }
      /**
   * Update the bicubic filter
   */
   static void
   vlVdpVideoMixerUpdateBicubicFilter(vlVdpVideoMixer *vmixer)
   {
               /* if present remove the old filter first */
   if (vmixer->bicubic.filter) {
      vl_bicubic_filter_cleanup(vmixer->bicubic.filter);
   FREE(vmixer->bicubic.filter);
      }
   /* and create a new filter as needed */
   if (vmixer->bicubic.enabled) {
      vmixer->bicubic.filter = MALLOC(sizeof(struct vl_bicubic_filter));
   vl_bicubic_filter_init(vmixer->bicubic.filter, vmixer->device->context,
         }
      /**
   * Retrieve whether features were requested at creation time.
   */
   VdpStatus
   vlVdpVideoMixerGetFeatureSupport(VdpVideoMixer mixer,
                     {
      vlVdpVideoMixer *vmixer;
            if (!(features && feature_supports))
            vmixer = vlGetDataHTAB(mixer);
   if (!vmixer)
            for (i = 0; i < feature_count; ++i) {
      switch (features[i]) {
   /* they are valid, but we doesn't support them */
   case VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL_SPATIAL:
   case VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L2:
   case VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L3:
   case VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L4:
   case VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L5:
   case VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L6:
   case VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L7:
   case VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L8:
   case VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L9:
   case VDP_VIDEO_MIXER_FEATURE_INVERSE_TELECINE:
                  case VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL:
                  case VDP_VIDEO_MIXER_FEATURE_SHARPNESS:
                  case VDP_VIDEO_MIXER_FEATURE_NOISE_REDUCTION:
                  case VDP_VIDEO_MIXER_FEATURE_LUMA_KEY:
                  case VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L1:
                  default:
                        }
      /**
   * Enable or disable features.
   */
   VdpStatus
   vlVdpVideoMixerSetFeatureEnables(VdpVideoMixer mixer,
                     {
      vlVdpVideoMixer *vmixer;
            if (!(features && feature_enables))
            vmixer = vlGetDataHTAB(mixer);
   if (!vmixer)
            mtx_lock(&vmixer->device->mutex);
   for (i = 0; i < feature_count; ++i) {
      switch (features[i]) {
   /* they are valid, but we doesn't support them */
   case VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL_SPATIAL:
   case VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L2:
   case VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L3:
   case VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L4:
   case VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L5:
   case VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L6:
   case VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L7:
   case VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L8:
   case VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L9:
   case VDP_VIDEO_MIXER_FEATURE_INVERSE_TELECINE:
            case VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL:
      vmixer->deint.enabled = feature_enables[i];
               case VDP_VIDEO_MIXER_FEATURE_SHARPNESS:
      vmixer->sharpness.enabled = feature_enables[i];
               case VDP_VIDEO_MIXER_FEATURE_NOISE_REDUCTION:
      vmixer->noise_reduction.enabled = feature_enables[i];
               case VDP_VIDEO_MIXER_FEATURE_LUMA_KEY:
      vmixer->luma_key.enabled = feature_enables[i];
   if (!debug_get_bool_option("G3DVL_NO_CSC", false))
      if (!vl_compositor_set_csc_matrix(&vmixer->cstate, (const vl_csc_matrix *)&vmixer->csc,
            mtx_unlock(&vmixer->device->mutex);
               case VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L1:
      vmixer->bicubic.enabled = feature_enables[i];
               default:
      mtx_unlock(&vmixer->device->mutex);
         }
               }
      /**
   * Retrieve whether features are enabled.
   */
   VdpStatus
   vlVdpVideoMixerGetFeatureEnables(VdpVideoMixer mixer,
                     {
      vlVdpVideoMixer *vmixer;
            if (!(features && feature_enables))
            vmixer = vlGetDataHTAB(mixer);
   if (!vmixer)
            for (i = 0; i < feature_count; ++i) {
      switch (features[i]) {
   /* they are valid, but we doesn't support them */
   case VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL:
   case VDP_VIDEO_MIXER_FEATURE_DEINTERLACE_TEMPORAL_SPATIAL:
   case VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L2:
   case VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L3:
   case VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L4:
   case VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L5:
   case VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L6:
   case VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L7:
   case VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L8:
   case VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L9:
   case VDP_VIDEO_MIXER_FEATURE_INVERSE_TELECINE:
            case VDP_VIDEO_MIXER_FEATURE_SHARPNESS:
                  case VDP_VIDEO_MIXER_FEATURE_NOISE_REDUCTION:
                  case VDP_VIDEO_MIXER_FEATURE_LUMA_KEY:
                  case VDP_VIDEO_MIXER_FEATURE_HIGH_QUALITY_SCALING_L1:
                  default:
                        }
      /**
   * Set attribute values.
   */
   VdpStatus
   vlVdpVideoMixerSetAttributeValues(VdpVideoMixer mixer,
                     {
      const VdpColor *background_color;
   union pipe_color_union color;
   const float *vdp_csc;
   float val;
   unsigned i;
            if (!(attributes && attribute_values))
            vlVdpVideoMixer *vmixer = vlGetDataHTAB(mixer);
   if (!vmixer)
            mtx_lock(&vmixer->device->mutex);
   for (i = 0; i < attribute_count; ++i) {
      switch (attributes[i]) {
   case VDP_VIDEO_MIXER_ATTRIBUTE_BACKGROUND_COLOR:
      background_color = attribute_values[i];
   color.f[0] = background_color->red;
   color.f[1] = background_color->green;
   color.f[2] = background_color->blue;
   color.f[3] = background_color->alpha;
   vl_compositor_set_clear_color(&vmixer->cstate, &color);
      case VDP_VIDEO_MIXER_ATTRIBUTE_CSC_MATRIX:
      vdp_csc = attribute_values[i];
   vmixer->custom_csc = !!vdp_csc;
   if (!vdp_csc)
         else
         if (!debug_get_bool_option("G3DVL_NO_CSC", false))
      if (!vl_compositor_set_csc_matrix(&vmixer->cstate, (const vl_csc_matrix *)&vmixer->csc,
            ret = VDP_STATUS_ERROR;
                           val = *(float*)attribute_values[i];
   if (val < 0.0f || val > 1.0f) {
      ret = VDP_STATUS_INVALID_VALUE;
               vmixer->noise_reduction.level = val * 10;
               case VDP_VIDEO_MIXER_ATTRIBUTE_LUMA_KEY_MIN_LUMA:
      val = *(float*)attribute_values[i];
   if (val < 0.0f || val > 1.0f) {
      ret = VDP_STATUS_INVALID_VALUE;
      }
   vmixer->luma_key.luma_min = val;
   if (!debug_get_bool_option("G3DVL_NO_CSC", false))
      if (!vl_compositor_set_csc_matrix(&vmixer->cstate, (const vl_csc_matrix *)&vmixer->csc,
            ret = VDP_STATUS_ERROR;
               case VDP_VIDEO_MIXER_ATTRIBUTE_LUMA_KEY_MAX_LUMA:
      val = *(float*)attribute_values[i];
   if (val < 0.0f || val > 1.0f) {
      ret = VDP_STATUS_INVALID_VALUE;
      }
   vmixer->luma_key.luma_max = val;
   if (!debug_get_bool_option("G3DVL_NO_CSC", false))
      if (!vl_compositor_set_csc_matrix(&vmixer->cstate, (const vl_csc_matrix *)&vmixer->csc,
            ret = VDP_STATUS_ERROR;
                           val = *(float*)attribute_values[i];
   if (val < -1.0f || val > 1.0f) {
      ret = VDP_STATUS_INVALID_VALUE;
               vmixer->sharpness.value = val;
               case VDP_VIDEO_MIXER_ATTRIBUTE_SKIP_CHROMA_DEINTERLACE:
      if (*(uint8_t*)attribute_values[i] > 1) {
      ret = VDP_STATUS_INVALID_VALUE;
      }
   vmixer->skip_chroma_deint = *(uint8_t*)attribute_values[i];
   vlVdpVideoMixerUpdateDeinterlaceFilter(vmixer);
      default:
      ret = VDP_STATUS_INVALID_VIDEO_MIXER_ATTRIBUTE;
         }
               fail:
      mtx_unlock(&vmixer->device->mutex);
      }
      /**
   * Retrieve parameter values given at creation time.
   */
   VdpStatus
   vlVdpVideoMixerGetParameterValues(VdpVideoMixer mixer,
                     {
      vlVdpVideoMixer *vmixer = vlGetDataHTAB(mixer);
   unsigned i;
   if (!vmixer)
            if (!parameter_count)
         if (!(parameters && parameter_values))
         for (i = 0; i < parameter_count; ++i) {
      switch (parameters[i]) {
   case VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_WIDTH:
      *(uint32_t*)parameter_values[i] = vmixer->video_width;
      case VDP_VIDEO_MIXER_PARAMETER_VIDEO_SURFACE_HEIGHT:
      *(uint32_t*)parameter_values[i] = vmixer->video_height;
      case VDP_VIDEO_MIXER_PARAMETER_CHROMA_TYPE:
      *(VdpChromaType*)parameter_values[i] = PipeToChroma(vmixer->chroma_format);
      case VDP_VIDEO_MIXER_PARAMETER_LAYERS:
      *(uint32_t*)parameter_values[i] = vmixer->max_layers;
      default:
            }
      }
      /**
   * Retrieve current attribute values.
   */
   VdpStatus
   vlVdpVideoMixerGetAttributeValues(VdpVideoMixer mixer,
                     {
      unsigned i;
            if (!(attributes && attribute_values))
            vlVdpVideoMixer *vmixer = vlGetDataHTAB(mixer);
   if (!vmixer)
            mtx_lock(&vmixer->device->mutex);
   for (i = 0; i < attribute_count; ++i) {
      switch (attributes[i]) {
   case VDP_VIDEO_MIXER_ATTRIBUTE_BACKGROUND_COLOR:
      vl_compositor_get_clear_color(&vmixer->cstate, attribute_values[i]);
      case VDP_VIDEO_MIXER_ATTRIBUTE_CSC_MATRIX:
      vdp_csc = attribute_values[i];
   if (!vmixer->custom_csc) {
      *vdp_csc = NULL;
      }
               case VDP_VIDEO_MIXER_ATTRIBUTE_NOISE_REDUCTION_LEVEL:
                  case VDP_VIDEO_MIXER_ATTRIBUTE_LUMA_KEY_MIN_LUMA:
      *(float*)attribute_values[i] = vmixer->luma_key.luma_min;
      case VDP_VIDEO_MIXER_ATTRIBUTE_LUMA_KEY_MAX_LUMA:
      *(float*)attribute_values[i] = vmixer->luma_key.luma_max;
      case VDP_VIDEO_MIXER_ATTRIBUTE_SHARPNESS_LEVEL:
      *(float*)attribute_values[i] = vmixer->sharpness.value;
      case VDP_VIDEO_MIXER_ATTRIBUTE_SKIP_CHROMA_DEINTERLACE:
      *(uint8_t*)attribute_values[i] = vmixer->skip_chroma_deint;
      default:
      mtx_unlock(&vmixer->device->mutex);
         }
   mtx_unlock(&vmixer->device->mutex);
      }
      /**
   * Generate a color space conversion matrix.
   */
   VdpStatus
   vlVdpGenerateCSCMatrix(VdpProcamp *procamp,
               {
      enum VL_CSC_COLOR_STANDARD vl_std;
            if (!csc_matrix)
            switch (standard) {
      case VDP_COLOR_STANDARD_ITUR_BT_601: vl_std = VL_CSC_COLOR_STANDARD_BT_601; break;
   case VDP_COLOR_STANDARD_ITUR_BT_709: vl_std = VL_CSC_COLOR_STANDARD_BT_709; break;
   case VDP_COLOR_STANDARD_SMPTE_240M:  vl_std = VL_CSC_COLOR_STANDARD_SMPTE_240M; break;
               if (procamp) {
      if (procamp->struct_version > VDP_PROCAMP_VERSION)
         camp.brightness = procamp->brightness;
   camp.contrast = procamp->contrast;
   camp.saturation = procamp->saturation;
               vl_csc_get_matrix(vl_std, procamp ? &camp : NULL, true, csc_matrix);
      }
