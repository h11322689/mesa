   /*
   * Copyright © 2017 Intel Corporation
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included
   * in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      /**
   * @file iris_resource.c
   *
   * Resources are images, buffers, and other objects used by the GPU.
   *
   * XXX: explain resources
   */
      #include <stdio.h>
   #include <errno.h>
   #include "pipe/p_defines.h"
   #include "pipe/p_state.h"
   #include "pipe/p_context.h"
   #include "pipe/p_screen.h"
   #include "util/os_memory.h"
   #include "util/u_cpu_detect.h"
   #include "util/u_inlines.h"
   #include "util/format/u_format.h"
   #include "util/u_memory.h"
   #include "util/u_threaded_context.h"
   #include "util/u_transfer.h"
   #include "util/u_transfer_helper.h"
   #include "util/u_upload_mgr.h"
   #include "util/ralloc.h"
   #include "iris_batch.h"
   #include "iris_context.h"
   #include "iris_resource.h"
   #include "iris_screen.h"
   #include "intel/common/intel_aux_map.h"
   #include "intel/dev/intel_debug.h"
   #include "isl/isl.h"
   #include "drm-uapi/drm_fourcc.h"
   #include "drm-uapi/i915_drm.h"
      enum modifier_priority {
      MODIFIER_PRIORITY_INVALID = 0,
   MODIFIER_PRIORITY_LINEAR,
   MODIFIER_PRIORITY_X,
   MODIFIER_PRIORITY_Y,
   MODIFIER_PRIORITY_Y_CCS,
   MODIFIER_PRIORITY_Y_GFX12_RC_CCS,
   MODIFIER_PRIORITY_Y_GFX12_RC_CCS_CC,
   MODIFIER_PRIORITY_4,
   MODIFIER_PRIORITY_4_DG2_RC_CCS,
   MODIFIER_PRIORITY_4_DG2_RC_CCS_CC,
   MODIFIER_PRIORITY_4_MTL_RC_CCS,
      };
      static const uint64_t priority_to_modifier[] = {
      [MODIFIER_PRIORITY_INVALID] = DRM_FORMAT_MOD_INVALID,
   [MODIFIER_PRIORITY_LINEAR] = DRM_FORMAT_MOD_LINEAR,
   [MODIFIER_PRIORITY_X] = I915_FORMAT_MOD_X_TILED,
   [MODIFIER_PRIORITY_Y] = I915_FORMAT_MOD_Y_TILED,
   [MODIFIER_PRIORITY_Y_CCS] = I915_FORMAT_MOD_Y_TILED_CCS,
   [MODIFIER_PRIORITY_Y_GFX12_RC_CCS] = I915_FORMAT_MOD_Y_TILED_GEN12_RC_CCS,
   [MODIFIER_PRIORITY_Y_GFX12_RC_CCS_CC] = I915_FORMAT_MOD_Y_TILED_GEN12_RC_CCS_CC,
   [MODIFIER_PRIORITY_4] = I915_FORMAT_MOD_4_TILED,
   [MODIFIER_PRIORITY_4_DG2_RC_CCS] = I915_FORMAT_MOD_4_TILED_DG2_RC_CCS,
   [MODIFIER_PRIORITY_4_DG2_RC_CCS_CC] = I915_FORMAT_MOD_4_TILED_DG2_RC_CCS_CC,
   [MODIFIER_PRIORITY_4_MTL_RC_CCS] = I915_FORMAT_MOD_4_TILED_MTL_RC_CCS,
      };
      static bool
   modifier_is_supported(const struct intel_device_info *devinfo,
               {
      /* Check for basic device support. */
   switch (modifier) {
   case DRM_FORMAT_MOD_LINEAR:
   case I915_FORMAT_MOD_X_TILED:
         case I915_FORMAT_MOD_Y_TILED:
      if (devinfo->ver <= 8 && (bind & PIPE_BIND_SCANOUT))
         if (devinfo->verx10 >= 125)
            case I915_FORMAT_MOD_Y_TILED_CCS:
      if (devinfo->ver <= 8 || devinfo->ver >= 12)
            case I915_FORMAT_MOD_Y_TILED_GEN12_RC_CCS:
   case I915_FORMAT_MOD_Y_TILED_GEN12_MC_CCS:
   case I915_FORMAT_MOD_Y_TILED_GEN12_RC_CCS_CC:
      if (devinfo->verx10 != 120)
            case I915_FORMAT_MOD_4_TILED:
      if (devinfo->verx10 < 125)
            case I915_FORMAT_MOD_4_TILED_DG2_RC_CCS:
   case I915_FORMAT_MOD_4_TILED_DG2_MC_CCS:
   case I915_FORMAT_MOD_4_TILED_DG2_RC_CCS_CC:
      if (!intel_device_info_is_dg2(devinfo))
            case I915_FORMAT_MOD_4_TILED_MTL_RC_CCS:
   case I915_FORMAT_MOD_4_TILED_MTL_MC_CCS:
   case I915_FORMAT_MOD_4_TILED_MTL_RC_CCS_CC:
      if (!intel_device_info_is_mtl(devinfo))
            case DRM_FORMAT_MOD_INVALID:
   default:
                           /* Check remaining requirements. */
   switch (modifier) {
   case I915_FORMAT_MOD_4_TILED_MTL_MC_CCS:
   case I915_FORMAT_MOD_4_TILED_DG2_MC_CCS:
   case I915_FORMAT_MOD_Y_TILED_GEN12_MC_CCS:
      if (no_ccs)
            if (pfmt != PIPE_FORMAT_BGRA8888_UNORM &&
      pfmt != PIPE_FORMAT_RGBA8888_UNORM &&
   pfmt != PIPE_FORMAT_BGRX8888_UNORM &&
   pfmt != PIPE_FORMAT_RGBX8888_UNORM &&
   pfmt != PIPE_FORMAT_NV12 &&
   pfmt != PIPE_FORMAT_P010 &&
   pfmt != PIPE_FORMAT_P012 &&
   pfmt != PIPE_FORMAT_P016 &&
   pfmt != PIPE_FORMAT_YUYV &&
   pfmt != PIPE_FORMAT_UYVY) {
      }
      case I915_FORMAT_MOD_4_TILED_MTL_RC_CCS:
   case I915_FORMAT_MOD_4_TILED_MTL_RC_CCS_CC:
   case I915_FORMAT_MOD_4_TILED_DG2_RC_CCS_CC:
   case I915_FORMAT_MOD_4_TILED_DG2_RC_CCS:
   case I915_FORMAT_MOD_Y_TILED_GEN12_RC_CCS_CC:
   case I915_FORMAT_MOD_Y_TILED_GEN12_RC_CCS:
   case I915_FORMAT_MOD_Y_TILED_CCS: {
      if (no_ccs)
            enum isl_format rt_format =
                  if (rt_format == ISL_FORMAT_UNSUPPORTED ||
      !isl_format_supports_ccs_e(devinfo, rt_format))
         }
   default:
                     }
      static uint64_t
   select_best_modifier(const struct intel_device_info *devinfo,
                     {
               for (int i = 0; i < count; i++) {
      if (!modifier_is_supported(devinfo, templ->format, templ->bind,
                  switch (modifiers[i]) {
   case I915_FORMAT_MOD_4_TILED_MTL_RC_CCS_CC:
      prio = MAX2(prio, MODIFIER_PRIORITY_4_MTL_RC_CCS_CC);
      case I915_FORMAT_MOD_4_TILED_MTL_RC_CCS:
      prio = MAX2(prio, MODIFIER_PRIORITY_4_MTL_RC_CCS);
      case I915_FORMAT_MOD_4_TILED_DG2_RC_CCS_CC:
      prio = MAX2(prio, MODIFIER_PRIORITY_4_DG2_RC_CCS_CC);
      case I915_FORMAT_MOD_4_TILED_DG2_RC_CCS:
      prio = MAX2(prio, MODIFIER_PRIORITY_4_DG2_RC_CCS);
      case I915_FORMAT_MOD_4_TILED:
      prio = MAX2(prio, MODIFIER_PRIORITY_4);
      case I915_FORMAT_MOD_Y_TILED_GEN12_RC_CCS_CC:
      prio = MAX2(prio, MODIFIER_PRIORITY_Y_GFX12_RC_CCS_CC);
      case I915_FORMAT_MOD_Y_TILED_GEN12_RC_CCS:
      prio = MAX2(prio, MODIFIER_PRIORITY_Y_GFX12_RC_CCS);
      case I915_FORMAT_MOD_Y_TILED_CCS:
      prio = MAX2(prio, MODIFIER_PRIORITY_Y_CCS);
      case I915_FORMAT_MOD_Y_TILED:
      prio = MAX2(prio, MODIFIER_PRIORITY_Y);
      case I915_FORMAT_MOD_X_TILED:
      prio = MAX2(prio, MODIFIER_PRIORITY_X);
      case DRM_FORMAT_MOD_LINEAR:
      prio = MAX2(prio, MODIFIER_PRIORITY_LINEAR);
      case DRM_FORMAT_MOD_INVALID:
   default:
                        }
      static inline bool is_modifier_external_only(enum pipe_format pfmt,
         {
      /* Only allow external usage for the following cases: YUV formats
   * and the media-compression modifier. The render engine lacks
   * support for rendering to a media-compressed surface if the
   * compression ratio is large enough. By requiring external usage
   * of media-compressed surfaces, resolves are avoided.
   */
   return util_format_is_yuv(pfmt) ||
      }
      static void
   iris_query_dmabuf_modifiers(struct pipe_screen *pscreen,
                                 {
      struct iris_screen *screen = (void *) pscreen;
            uint64_t all_modifiers[] = {
      DRM_FORMAT_MOD_LINEAR,
   I915_FORMAT_MOD_X_TILED,
   I915_FORMAT_MOD_4_TILED,
   I915_FORMAT_MOD_4_TILED_DG2_RC_CCS,
   I915_FORMAT_MOD_4_TILED_DG2_MC_CCS,
   I915_FORMAT_MOD_4_TILED_DG2_RC_CCS_CC,
   I915_FORMAT_MOD_4_TILED_MTL_RC_CCS,
   I915_FORMAT_MOD_4_TILED_MTL_RC_CCS_CC,
   I915_FORMAT_MOD_4_TILED_MTL_MC_CCS,
   I915_FORMAT_MOD_Y_TILED,
   I915_FORMAT_MOD_Y_TILED_CCS,
   I915_FORMAT_MOD_Y_TILED_GEN12_RC_CCS,
   I915_FORMAT_MOD_Y_TILED_GEN12_MC_CCS,
                        for (int i = 0; i < ARRAY_SIZE(all_modifiers); i++) {
      if (!modifier_is_supported(devinfo, pfmt, 0, all_modifiers[i]))
            if (supported_mods < max) {
                     if (external_only) {
      external_only[supported_mods] =
                                 }
      static bool
   iris_is_dmabuf_modifier_supported(struct pipe_screen *pscreen,
               {
      struct iris_screen *screen = (void *) pscreen;
            if (modifier_is_supported(devinfo, pfmt, 0, modifier)) {
      if (external_only)
                           }
      static unsigned int
   iris_get_dmabuf_modifier_planes(struct pipe_screen *pscreen, uint64_t modifier,
         {
               switch (modifier) {
   case I915_FORMAT_MOD_4_TILED_MTL_RC_CCS_CC:
   case I915_FORMAT_MOD_Y_TILED_GEN12_RC_CCS_CC:
         case I915_FORMAT_MOD_4_TILED_MTL_RC_CCS:
   case I915_FORMAT_MOD_4_TILED_MTL_MC_CCS:
   case I915_FORMAT_MOD_4_TILED_DG2_RC_CCS_CC:
   case I915_FORMAT_MOD_Y_TILED_GEN12_MC_CCS:
   case I915_FORMAT_MOD_Y_TILED_GEN12_RC_CCS:
   case I915_FORMAT_MOD_Y_TILED_CCS:
         case I915_FORMAT_MOD_4_TILED_DG2_RC_CCS:
   case I915_FORMAT_MOD_4_TILED_DG2_MC_CCS:
   default:
            }
      enum isl_format
   iris_image_view_get_format(struct iris_context *ice,
         {
      struct iris_screen *screen = (struct iris_screen *)ice->ctx.screen;
            isl_surf_usage_flags_t usage = ISL_SURF_USAGE_STORAGE_BIT;
   enum isl_format isl_fmt =
            if (img->shader_access & PIPE_IMAGE_ACCESS_READ) {
      /* On Gfx8, try to use typed surfaces reads (which support a
   * limited number of formats), and if not possible, fall back
   * to untyped reads.
   */
   if (devinfo->ver == 8 &&
      !isl_has_matching_typed_storage_image_format(devinfo, isl_fmt))
      else
                  }
      static struct pipe_memory_object *
   iris_memobj_create_from_handle(struct pipe_screen *pscreen,
               {
      struct iris_screen *screen = (struct iris_screen *)pscreen;
   struct iris_memory_object *memobj = CALLOC_STRUCT(iris_memory_object);
            if (!memobj)
            switch (whandle->type) {
   case WINSYS_HANDLE_TYPE_SHARED:
      bo = iris_bo_gem_create_from_name(screen->bufmgr, "winsys image",
            case WINSYS_HANDLE_TYPE_FD:
      bo = iris_bo_import_dmabuf(screen->bufmgr, whandle->handle,
            default:
                  if (!bo) {
      free(memobj);
               memobj->b.dedicated = dedicated;
   memobj->bo = bo;
   memobj->format = whandle->format;
               }
      static void
   iris_memobj_destroy(struct pipe_screen *pscreen,
         {
               iris_bo_unreference(memobj->bo);
      }
      struct pipe_resource *
   iris_resource_get_separate_stencil(struct pipe_resource *p_res)
   {
      /* For packed depth-stencil, we treat depth as the primary resource
   * and store S8 as the "second plane" resource.
   */
   if (p_res->next && p_res->next->format == PIPE_FORMAT_S8_UINT)
                  }
      static void
   iris_resource_set_separate_stencil(struct pipe_resource *p_res,
         {
      assert(util_format_has_depth(util_format_description(p_res->format)));
      }
      void
   iris_get_depth_stencil_resources(struct pipe_resource *res,
               {
      if (!res) {
      *out_z = NULL;
   *out_s = NULL;
               if (res->format != PIPE_FORMAT_S8_UINT) {
      *out_z = (void *) res;
      } else {
      *out_z = NULL;
         }
      void
   iris_resource_disable_aux(struct iris_resource *res)
   {
      iris_bo_unreference(res->aux.bo);
   iris_bo_unreference(res->aux.clear_color_bo);
            res->aux.usage = ISL_AUX_USAGE_NONE;
   res->aux.surf.size_B = 0;
   res->aux.bo = NULL;
   res->aux.extra_aux.surf.size_B = 0;
   res->aux.clear_color_bo = NULL;
      }
      static unsigned
   iris_resource_alloc_flags(const struct iris_screen *screen,
               {
      if (templ->flags & IRIS_RESOURCE_FLAG_DEVICE_MEM)
                     switch (templ->usage) {
   case PIPE_USAGE_STAGING:
      flags |= BO_ALLOC_SMEM | BO_ALLOC_COHERENT;
      case PIPE_USAGE_STREAM:
      flags |= BO_ALLOC_SMEM;
      case PIPE_USAGE_DYNAMIC:
   case PIPE_USAGE_DEFAULT:
   case PIPE_USAGE_IMMUTABLE:
      /* Use LMEM for these if possible */
               if (templ->bind & PIPE_BIND_SCANOUT)
            if (templ->flags & (PIPE_RESOURCE_FLAG_MAP_COHERENT |
                  if (screen->devinfo->verx10 >= 125 && screen->devinfo->has_local_mem &&
      isl_aux_usage_has_ccs(aux_usage)) {
   assert((flags & BO_ALLOC_SMEM) == 0);
               if ((templ->bind & PIPE_BIND_SHARED) ||
      util_format_get_num_planes(templ->format) > 1)
         if (templ->bind & PIPE_BIND_PROTECTED)
            if (templ->bind & PIPE_BIND_SHARED) {
               /* We request that the bufmgr zero because, if a buffer gets re-used
   * from the pool, we don't want to leak random garbage from our process
   * to some other.
   */
                  }
      static void
   iris_resource_destroy(struct pipe_screen *screen,
         {
               if (p_res->target == PIPE_BUFFER)
                     threaded_resource_deinit(p_res);
   iris_bo_unreference(res->bo);
               }
      static struct iris_resource *
   iris_alloc_resource(struct pipe_screen *pscreen,
         {
      struct iris_resource *res = calloc(1, sizeof(struct iris_resource));
   if (!res)
            res->base.b = *templ;
   res->base.b.screen = pscreen;
   res->orig_screen = iris_pscreen_ref(pscreen);
   pipe_reference_init(&res->base.b.reference, 1);
            if (templ->target == PIPE_BUFFER)
               }
      unsigned
   iris_get_num_logical_layers(const struct iris_resource *res, unsigned level)
   {
      if (res->surf.dim == ISL_SURF_DIM_3D)
         else
      }
      static enum isl_aux_state **
   create_aux_state_map(struct iris_resource *res, enum isl_aux_state initial)
   {
               uint32_t total_slices = 0;
   for (uint32_t level = 0; level < res->surf.levels; level++)
            const size_t per_level_array_size =
            /* We're going to allocate a single chunk of data for both the per-level
   * reference array and the arrays of aux_state.  This makes cleanup
   * significantly easier.
   */
   const size_t total_size =
            void *data = malloc(total_size);
   if (!data)
            enum isl_aux_state **per_level_arr = data;
   enum isl_aux_state *s = data + per_level_array_size;
   for (uint32_t level = 0; level < res->surf.levels; level++) {
      per_level_arr[level] = s;
   const unsigned level_layers = iris_get_num_logical_layers(res, level);
   for (uint32_t a = 0; a < level_layers; a++)
      }
               }
      static unsigned
   iris_get_aux_clear_color_state_size(struct iris_screen *screen,
         {
      if (!isl_aux_usage_has_fast_clears(res->aux.usage))
                     /* Depth packets can't specify indirect clear values. The only time depth
   * buffers can use indirect clear values is when they're accessed by the
   * sampler via render surface state objects.
   */
   if (isl_surf_usage_is_depth(res->surf.usage) &&
      !iris_sample_with_depth_aux(screen->devinfo, res))
            }
      static void
   map_aux_addresses(struct iris_screen *screen, struct iris_resource *res,
         {
      void *aux_map_ctx = iris_bufmgr_get_aux_map_context(screen->bufmgr);
   if (!aux_map_ctx)
            if (isl_aux_usage_has_ccs(res->aux.usage)) {
      const unsigned aux_offset = res->aux.extra_aux.surf.size_B > 0 ?
         const enum isl_format format =
         const uint64_t format_bits =
         const bool mapped =
      intel_aux_map_add_mapping(aux_map_ctx,
                  assert(mapped);
         }
      static bool
   want_ccs_e_for_format(const struct intel_device_info *devinfo,
         {
      if (!isl_format_supports_ccs_e(devinfo, format))
                     /* Prior to TGL, CCS_E seems to significantly hurt performance with 32-bit
   * floating point formats.  For example, Paraview's "Wavelet Volume" case
   * uses both R32_FLOAT and R32G32B32A32_FLOAT, and enabling CCS_E for those
   * formats causes a 62% FPS drop.
   *
   * However, many benchmarks seem to use 16-bit float with no issues.
   */
   if (devinfo->ver <= 11 &&
      fmtl->channels.r.bits == 32 && fmtl->channels.r.type == ISL_SFLOAT)
            }
      static enum isl_surf_dim
   target_to_isl_surf_dim(enum pipe_texture_target target)
   {
      switch (target) {
   case PIPE_BUFFER:
   case PIPE_TEXTURE_1D:
   case PIPE_TEXTURE_1D_ARRAY:
         case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_CUBE:
   case PIPE_TEXTURE_RECT:
   case PIPE_TEXTURE_2D_ARRAY:
   case PIPE_TEXTURE_CUBE_ARRAY:
         case PIPE_TEXTURE_3D:
         case PIPE_MAX_TEXTURE_TYPES:
         }
      }
      static bool
   iris_resource_configure_main(const struct iris_screen *screen,
                     {
               if (modifier != DRM_FORMAT_MOD_INVALID && res->mod_info == NULL)
                     if (res->mod_info != NULL) {
         } else if (templ->usage == PIPE_USAGE_STAGING ||
               } else if (res->external_format != PIPE_FORMAT_NONE) {
      /* This came from iris_resource_from_memobj and didn't have
   * PIPE_BIND_LINEAR set, so "optimal" tiling is desired.  Let isl
   * select the tiling.  The implicit contract is that both drivers
   * will arrive at the same tiling by using the same code to decide.
   */
   assert(modifier == DRM_FORMAT_MOD_INVALID);
      } else if (!screen->devinfo->has_tiling_uapi &&
               } else if (templ->bind & PIPE_BIND_SCANOUT) {
         } else {
                  /* We don't support Yf or Ys tiling yet */
   tiling_flags &= ~ISL_TILING_STD_Y_MASK;
                     if (res->mod_info && !isl_drm_modifier_has_aux(modifier))
         else if (templ->bind & PIPE_BIND_CONST_BW)
            if (templ->usage == PIPE_USAGE_STAGING)
            if (templ->bind & PIPE_BIND_RENDER_TARGET)
            if (templ->bind & PIPE_BIND_SAMPLER_VIEW)
            if (templ->bind & PIPE_BIND_SHADER_IMAGE)
            if (templ->bind & PIPE_BIND_SCANOUT)
            if (templ->target == PIPE_TEXTURE_CUBE ||
      templ->target == PIPE_TEXTURE_CUBE_ARRAY) {
               if (templ->usage != PIPE_USAGE_STAGING &&
               /* Should be handled by u_transfer_helper */
            usage |= templ->format == PIPE_FORMAT_S8_UINT ?
               const enum isl_format format =
            const struct isl_surf_init_info init_info = {
      .dim = target_to_isl_surf_dim(templ->target),
   .format = format,
   .width = templ->width0,
   .height = templ->height0,
   .depth = templ->depth0,
   .levels = templ->last_level + 1,
   .array_len = templ->array_size,
   .samples = MAX2(templ->nr_samples, 1),
   .min_alignment_B = 0,
   .row_pitch_B = row_pitch_B,
   .usage = usage,
               if (!isl_surf_init_s(&screen->isl_dev, &res->surf, &init_info))
                        }
      static bool
   iris_get_ccs_surf_or_support(const struct isl_device *dev,
                     {
               struct isl_surf *ccs_surf;
   const struct isl_surf *hiz_or_mcs_surf;
   if (aux_surf->size_B > 0) {
      assert(aux_surf->usage & (ISL_SURF_USAGE_HIZ_BIT |
         hiz_or_mcs_surf = aux_surf;
      } else {
      hiz_or_mcs_surf = NULL;
               if (dev->info->has_flat_ccs) {
      /* CCS doesn't require VMA on XeHP. So, instead of creating a separate
   * surface, we can just return whether CCS is supported for the given
   * input surfaces.
   */
      } else  {
            }
      /**
   * Configure aux for the resource, but don't allocate it. For images which
   * might be shared with modifiers, we must allocate the image and aux data in
   * a single bo.
   *
   * Returns false on unexpected error (e.g. allocation failed, or invalid
   * configuration result).
   */
   static bool
   iris_resource_configure_aux(struct iris_screen *screen,
         {
               const bool has_mcs =
            const bool has_hiz =
            const bool has_ccs =
      iris_get_ccs_surf_or_support(&screen->isl_dev, &res->surf,
         if (has_mcs) {
      assert(!res->mod_info);
   assert(!has_hiz);
   if (has_ccs) {
         } else {
            } else if (has_hiz) {
      assert(!res->mod_info);
   assert(!has_mcs);
   if (!has_ccs) {
         } else if (res->surf.samples == 1 &&
            /* If this resource is single-sampled and will be used as a texture,
   * put the HiZ surface in write-through mode so that we can sample
   * from it.
   */
      } else {
            } else if (has_ccs) {
      if (isl_surf_usage_is_stencil(res->surf.usage)) {
      assert(!res->mod_info);
      } else if (res->mod_info && res->mod_info->supports_media_compression) {
         } else if (want_ccs_e_for_format(devinfo, res->surf.format)) {
      res->aux.usage = devinfo->ver < 12 ?
      } else {
      assert(isl_format_supports_ccs_d(devinfo, res->surf.format));
                  enum isl_aux_state initial_state;
   switch (res->aux.usage) {
   case ISL_AUX_USAGE_NONE:
      /* Having no aux buffer is only okay if there's no modifier with aux. */
   return !res->mod_info ||
      case ISL_AUX_USAGE_HIZ:
   case ISL_AUX_USAGE_HIZ_CCS:
   case ISL_AUX_USAGE_HIZ_CCS_WT:
   case ISL_AUX_USAGE_MCS:
   case ISL_AUX_USAGE_MCS_CCS:
      /* Leave the auxiliary buffer uninitialized. We can ambiguate it before
   * accessing it later on, if needed.
   */
   initial_state = ISL_AUX_STATE_AUX_INVALID;
      case ISL_AUX_USAGE_CCS_D:
   case ISL_AUX_USAGE_CCS_E:
   case ISL_AUX_USAGE_FCV_CCS_E:
   case ISL_AUX_USAGE_STC_CCS:
   case ISL_AUX_USAGE_MC:
      if (imported) {
      assert(res->aux.usage != ISL_AUX_USAGE_STC_CCS);
   initial_state =
      } else if (devinfo->has_flat_ccs) {
      assert(res->aux.surf.size_B == 0);
   /* From Bspec 47709, "MCS/CCS Buffers for Render Target(s)":
   *
   *    "CCS surface does not require initialization. Illegal CCS
   *     [values] are treated as uncompressed memory."
   *
   * The above quote is from the render target section, but we assume
   * it applies to CCS in general (e.g., STC_CCS). The uninitialized
   * CCS may be in any aux state. We choose the one which is most
   * convenient.
   *
   * We avoid states with CLEAR because stencil does not support it.
   * Those states also create a dependency on the clear color, which
   * can have negative performance implications. Even though some
   * blocks may actually be encoded with CLEAR, we can get away with
   * ignoring them - there are no known issues that require fast
   * cleared blocks to be tracked and avoided.
   *
   * We specifically avoid the AUX_INVALID state because it could
   * trigger an ambiguate. BLORP does not have support for ambiguating
   * stencil. Also, ambiguating some LODs of mipmapped 8bpp surfaces
   * seems to stomp on neighboring miplevels.
   *
   * There is only one remaining aux state which can give us correct
   * behavior, COMPRESSED_NO_CLEAR.
   */
      } else {
      assert(res->aux.surf.size_B > 0);
   /* When CCS is used, we need to ensure that it starts off in a valid
   * state. From the Sky Lake PRM, "MCS Buffer for Render Target(s)":
   *
   *    "If Software wants to enable Color Compression without Fast
   *     clear, Software needs to initialize MCS with zeros."
   *
   * A CCS surface initialized to zero is in the pass-through state.
   * This state can avoid the need to ambiguate in some cases. We'll
   * map and zero the CCS later on in iris_resource_init_aux_buf.
   */
      }
      default:
                  /* Create the aux_state for the auxiliary buffer. */
   res->aux.state = create_aux_state_map(res, initial_state);
   if (!res->aux.state)
               }
      /**
   * Initialize the aux buffer contents.
   *
   * Returns false on unexpected error (e.g. mapping a BO failed).
   */
   static bool
   iris_resource_init_aux_buf(struct iris_screen *screen,
         {
               if (iris_resource_get_aux_state(res, 0, 0) != ISL_AUX_STATE_AUX_INVALID &&
      res->aux.surf.size_B > 0) {
   if (!map)
         if (!map)
                        if (res->aux.extra_aux.surf.size_B > 0) {
      if (!map)
         if (!map)
            memset((char*)map + res->aux.extra_aux.offset,
               unsigned clear_color_size = iris_get_aux_clear_color_state_size(screen, res);
   if (clear_color_size > 0) {
      if (iris_bo_mmap_mode(res->bo) != IRIS_MMAP_NONE) {
      if (!map)
                        /* Zero the indirect clear color to match ::fast_clear_color. */
      } else {
                     if (map)
            if (res->aux.surf.size_B > 0) {
      res->aux.bo = res->bo;
   iris_bo_reference(res->aux.bo);
               if (clear_color_size > 0) {
      res->aux.clear_color_bo = res->bo;
                  }
      static void
   import_aux_info(struct iris_resource *res,
         {
      assert(aux_res->aux.surf.row_pitch_B && aux_res->aux.offset);
   assert(res->bo == aux_res->aux.bo);
   assert(res->aux.surf.row_pitch_B == aux_res->aux.surf.row_pitch_B);
            iris_bo_reference(aux_res->aux.bo);
   res->aux.bo = aux_res->aux.bo;
      }
      static void
   iris_resource_finish_aux_import(struct pipe_screen *pscreen,
         {
               /* Create an array of resources. Combining main and aux planes is easier
   * with indexing as opposed to scanning the linked list.
   */
   struct iris_resource *r[4] = { NULL, };
   unsigned num_planes = 0;
   unsigned num_main_planes = 0;
   for (struct pipe_resource *p_res = &res->base.b; p_res; p_res = p_res->next) {
      r[num_planes] = (struct iris_resource *)p_res;
               /* Combine main and aux plane information. */
   switch (res->mod_info->modifier) {
   case I915_FORMAT_MOD_4_TILED_MTL_RC_CCS:
   case I915_FORMAT_MOD_Y_TILED_CCS:
   case I915_FORMAT_MOD_Y_TILED_GEN12_RC_CCS:
      assert(num_main_planes == 1 && num_planes == 2);
   import_aux_info(r[0], r[1]);
            /* Add on a clear color BO.
   *
   * Also add some padding to make sure the fast clear color state buffer
   * starts at a 4K alignment to avoid some unknown issues.  See the
   * matching comment in iris_resource_create_with_modifiers().
   */
   if (iris_get_aux_clear_color_state_size(screen, res) > 0) {
      res->aux.clear_color_bo =
      iris_bo_alloc(screen->bufmgr, "clear color_buffer",
         }
      case I915_FORMAT_MOD_4_TILED_DG2_RC_CCS:
      assert(num_main_planes == 1);
   assert(num_planes == 1);
   res->aux.clear_color_bo =
      iris_bo_alloc(screen->bufmgr, "clear color_buffer",
               case I915_FORMAT_MOD_4_TILED_MTL_RC_CCS_CC:
   case I915_FORMAT_MOD_Y_TILED_GEN12_RC_CCS_CC:
      assert(num_main_planes == 1 && num_planes == 3);
   import_aux_info(r[0], r[1]);
            /* Import the clear color BO. */
   iris_bo_reference(r[2]->aux.clear_color_bo);
   r[0]->aux.clear_color_bo = r[2]->aux.clear_color_bo;
   r[0]->aux.clear_color_offset = r[2]->aux.clear_color_offset;
   r[0]->aux.clear_color_unknown = true;
      case I915_FORMAT_MOD_4_TILED_DG2_RC_CCS_CC:
      assert(num_main_planes == 1);
            /* Import the clear color BO. */
   iris_bo_reference(r[1]->aux.clear_color_bo);
   r[0]->aux.clear_color_bo = r[1]->aux.clear_color_bo;
   r[0]->aux.clear_color_offset = r[1]->aux.clear_color_offset;
   r[0]->aux.clear_color_unknown = true;
      case I915_FORMAT_MOD_4_TILED_MTL_MC_CCS:
   case I915_FORMAT_MOD_Y_TILED_GEN12_MC_CCS:
      if (num_main_planes == 1 && num_planes == 2) {
      import_aux_info(r[0], r[1]);
      } else {
      assert(num_main_planes == 2 && num_planes == 4);
   import_aux_info(r[0], r[2]);
   import_aux_info(r[1], r[3]);
   map_aux_addresses(screen, r[0], res->external_format, 0);
      }
      case I915_FORMAT_MOD_4_TILED_DG2_MC_CCS:
      assert(num_main_planes == num_planes);
      default:
      assert(!isl_drm_modifier_has_aux(res->mod_info->modifier));
         }
      static uint32_t
   iris_buffer_alignment(uint64_t size)
   {
      /* Some buffer operations want some amount of alignment.  The largest
   * buffer texture pixel size is 4 * 4 = 16B.  OpenCL data is also supposed
   * to be aligned and largest OpenCL data type is a double16 which is
   * 8 * 16 = 128B.  Align to the largest power of 2 which fits in the size,
   * up to 128B.
   */
   uint32_t align = MAX2(4 * 4, 8 * 16);
   while (align > size)
               }
      static struct pipe_resource *
   iris_resource_create_for_buffer(struct pipe_screen *pscreen,
         {
      struct iris_screen *screen = (struct iris_screen *)pscreen;
            assert(templ->target == PIPE_BUFFER);
   assert(templ->height0 <= 1);
   assert(templ->depth0 <= 1);
   assert(templ->format == PIPE_FORMAT_NONE ||
            res->internal_format = templ->format;
            enum iris_memory_zone memzone = IRIS_MEMZONE_OTHER;
   const char *name = templ->target == PIPE_BUFFER ? "buffer" : "miptree";
   if (templ->flags & IRIS_RESOURCE_FLAG_SHADER_MEMZONE) {
      memzone = IRIS_MEMZONE_SHADER;
      } else if (templ->flags & IRIS_RESOURCE_FLAG_SURFACE_MEMZONE) {
      memzone = IRIS_MEMZONE_SURFACE;
      } else if (templ->flags & IRIS_RESOURCE_FLAG_DYNAMIC_MEMZONE) {
      memzone = IRIS_MEMZONE_DYNAMIC;
      } else if (templ->flags & IRIS_RESOURCE_FLAG_SCRATCH_MEMZONE) {
      memzone = IRIS_MEMZONE_SCRATCH;
                        res->bo = iris_bo_alloc(screen->bufmgr, name, templ->width0,
                  if (!res->bo) {
      iris_resource_destroy(pscreen, &res->base.b);
               if (templ->bind & PIPE_BIND_SHARED) {
      iris_bo_mark_exported(res->bo);
                  }
      static struct pipe_resource *
   iris_resource_create_for_image(struct pipe_screen *pscreen,
                           {
      struct iris_screen *screen = (struct iris_screen *)pscreen;
   const struct intel_device_info *devinfo = screen->devinfo;
            if (!res)
            uint64_t modifier =
            if (modifier == DRM_FORMAT_MOD_INVALID && modifiers_count > 0) {
      fprintf(stderr, "Unsupported modifier, resource creation failed.\n");
               const bool isl_surf_created_successfully =
         if (!isl_surf_created_successfully)
            /* Don't create staging surfaces that will use over half the sram,
   * since staging implies you are copying data to another resource that's
   * at least as large, and then both wouldn't fit in system memory.
   *
   * Skip this for discrete cards, as the destination buffer might be in
   * device local memory while the staging buffer would be in system memory,
   * so both would fit.
   */
   if (templ->usage == PIPE_USAGE_STAGING && !devinfo->has_local_mem &&
      res->surf.size_B > (iris_bufmgr_sram_size(screen->bufmgr) / 2))
         if (!iris_resource_configure_aux(screen, res, false))
            const char *name = "miptree";
                     /* These are for u_upload_mgr buffers only */
   assert(!(templ->flags & (IRIS_RESOURCE_FLAG_SHADER_MEMZONE |
                        /* Modifiers require the aux data to be in the same buffer as the main
   * surface, but we combine them even when a modifier is not being used.
   */
            /* Allocate space for the aux buffer. */
   if (res->aux.surf.size_B > 0) {
      res->aux.offset = ALIGN(bo_size, res->aux.surf.alignment_B);
               /* Allocate space for the extra aux buffer. */
   if (res->aux.extra_aux.surf.size_B > 0) {
      res->aux.extra_aux.offset =
                     /* Allocate space for the indirect clear color.
   *
   * Also add some padding to make sure the fast clear color state buffer
   * starts at a 4K alignment. We believe that 256B might be enough, but due
   * to lack of testing we will leave this as 4K for now.
   */
   if (iris_get_aux_clear_color_state_size(screen, res) > 0) {
      res->aux.clear_color_offset = ALIGN(bo_size, 4096);
   bo_size = res->aux.clear_color_offset +
               /* The ISL alignment already includes AUX-TT requirements, so no additional
   * attention required here :)
   */
   uint32_t alignment = MAX2(4096, res->surf.alignment_B);
   res->bo =
            if (!res->bo)
            if (res->aux.usage != ISL_AUX_USAGE_NONE &&
      !iris_resource_init_aux_buf(screen, res))
         if (templ->bind & PIPE_BIND_SHARED) {
      iris_bo_mark_exported(res->bo);
                     fail:
      iris_resource_destroy(pscreen, &res->base.b);
      }
      static struct pipe_resource *
   iris_resource_create_with_modifiers(struct pipe_screen *pscreen,
                     {
      return iris_resource_create_for_image(pscreen, templ, modifiers,
      }
      static struct pipe_resource *
   iris_resource_create(struct pipe_screen *pscreen,
         {
      if (templ->target == PIPE_BUFFER)
         else
      }
      static uint64_t
   tiling_to_modifier(uint32_t tiling)
   {
      static const uint64_t map[] = {
      [I915_TILING_NONE]   = DRM_FORMAT_MOD_LINEAR,
   [I915_TILING_X]      = I915_FORMAT_MOD_X_TILED,
                           }
      static struct pipe_resource *
   iris_resource_from_user_memory(struct pipe_screen *pscreen,
               {
      struct iris_screen *screen = (struct iris_screen *)pscreen;
   struct iris_bufmgr *bufmgr = screen->bufmgr;
   struct iris_resource *res = iris_alloc_resource(pscreen, templ);
   if (!res)
            if (templ->target != PIPE_BUFFER &&
      templ->target != PIPE_TEXTURE_1D &&
   templ->target != PIPE_TEXTURE_2D)
         if (templ->array_size > 1)
            size_t res_size = templ->width0;
   if (templ->target != PIPE_BUFFER) {
      const uint32_t row_pitch_B =
                  if (!iris_resource_configure_main(screen, res, templ,
                  iris_resource_destroy(pscreen, &res->base.b);
      }
               /* The userptr ioctl only works on whole pages.  Because we know that
   * things will exist in memory at a page granularity, we can expand the
   * range given by the client into the whole number of pages and use an
   * offset on the resource to make it looks like it starts at the user's
   * pointer.
   */
   size_t page_size = getpagesize();
   assert(util_is_power_of_two_nonzero(page_size));
   size_t offset = (uintptr_t)user_memory & (page_size - 1);
   void *mem_start = (char *)user_memory - offset;
   size_t mem_size = offset + res_size;
            res->internal_format = templ->format;
   res->base.is_user_ptr = true;
   res->bo = iris_bo_create_userptr(bufmgr, "user", mem_start, mem_size,
         res->offset = offset;
   if (!res->bo) {
      iris_resource_destroy(pscreen, &res->base.b);
                           }
      static bool
   mod_plane_is_clear_color(uint64_t modifier, uint32_t plane)
   {
      ASSERTED const struct isl_drm_modifier_info *mod_info =
                  switch (modifier) {
   case I915_FORMAT_MOD_4_TILED_MTL_RC_CCS_CC:
   case I915_FORMAT_MOD_Y_TILED_GEN12_RC_CCS_CC:
      assert(mod_info->supports_clear_color);
      case I915_FORMAT_MOD_4_TILED_DG2_RC_CCS_CC:
      assert(mod_info->supports_clear_color);
      default:
      assert(!mod_info->supports_clear_color);
         }
      static struct iris_resource *
   get_resource_for_plane(struct pipe_resource *resource,
         {
      unsigned count = 0;
   for (struct pipe_resource *cur = resource; cur; cur = cur->next) {
      if (count++ == plane)
                  }
      static unsigned
   get_num_planes(const struct pipe_resource *resource)
   {
      unsigned count = 0;
   for (const struct pipe_resource *cur = resource; cur; cur = cur->next)
               }
      static unsigned
   get_main_plane_for_plane(enum pipe_format format,
               {
               if (n_planes == 1)
            if (!mod_info)
            if (mod_info->supports_media_compression) {
         } else {
      assert(!mod_info->supports_render_compression);
         }
      static struct pipe_resource *
   iris_resource_from_handle(struct pipe_screen *pscreen,
                     {
               struct iris_screen *screen = (struct iris_screen *)pscreen;
   struct iris_bufmgr *bufmgr = screen->bufmgr;
   struct iris_resource *res = iris_alloc_resource(pscreen, templ);
   if (!res)
            switch (whandle->type) {
   case WINSYS_HANDLE_TYPE_FD:
      res->bo = iris_bo_import_dmabuf(bufmgr, whandle->handle,
            case WINSYS_HANDLE_TYPE_SHARED:
      res->bo = iris_bo_gem_create_from_name(bufmgr, "winsys image",
            default:
         }
   if (!res->bo)
            uint64_t modifier;
   if (whandle->modifier == DRM_FORMAT_MOD_INVALID) {
      /* We don't have a modifier; match whatever GEM_GET_TILING says */
   uint32_t tiling;
   iris_gem_get_tiling(res->bo, &tiling);
      } else {
                  res->offset = whandle->offset;
            /* Create a surface for each plane specified by the external format. */
   if (whandle->plane < util_format_get_num_planes(whandle->format)) {
      const bool isl_surf_created_successfully =
      iris_resource_configure_main(screen, res, templ, modifier,
      if (!isl_surf_created_successfully)
            if (!iris_resource_configure_aux(screen, res, true))
            /* The gallium dri layer will create a separate plane resource for the
   * aux image. iris_resource_finish_aux_import will merge the separate aux
   * parameters back into a single iris_resource.
      } else if (mod_plane_is_clear_color(modifier, whandle->plane)) {
      res->aux.clear_color_offset = whandle->offset;
   res->aux.clear_color_bo = res->bo;
      } else {
      /* Save modifier import information to reconstruct later. After import,
   * this will be available under a second image accessible from the main
   * image with res->base.next. See iris_resource_finish_aux_import.
   */
   res->aux.surf.row_pitch_B = whandle->stride;
   res->aux.offset = whandle->offset;
   res->aux.bo = res->bo;
               if (get_num_planes(&res->base.b) ==
      iris_get_dmabuf_modifier_planes(pscreen, modifier, whandle->format)) {
                     fail:
      iris_resource_destroy(pscreen, &res->base.b);
      }
      static struct pipe_resource *
   iris_resource_from_memobj(struct pipe_screen *pscreen,
                     {
      struct iris_screen *screen = (struct iris_screen *)pscreen;
   struct iris_memory_object *memobj = (struct iris_memory_object *)pmemobj;
            if (!res)
            res->bo = memobj->bo;
   res->offset = offset;
   res->external_format = templ->format;
            if (templ->flags & PIPE_RESOURCE_FLAG_TEXTURING_MORE_LIKELY) {
      UNUSED const bool isl_surf_created_successfully =
                                 }
      /* Handle combined depth/stencil with memory objects.
   *
   * This function is modeled after u_transfer_helper_resource_create.
   */
   static struct pipe_resource *
   iris_resource_from_memobj_wrapper(struct pipe_screen *pscreen,
                     {
               /* Normal case, no special handling: */
   if (!(util_format_is_depth_and_stencil(format)))
            struct pipe_resource t = *templ;
            struct pipe_resource *prsc =
         if (!prsc)
                     /* Stencil offset in the buffer without aux. */
   uint64_t s_offset = offset +
                     t.format = PIPE_FORMAT_S8_UINT;
   struct pipe_resource *stencil =
         if (!stencil) {
      iris_resource_destroy(pscreen, prsc);
               iris_resource_set_separate_stencil(prsc, stencil);
      }
      static void
   iris_flush_resource(struct pipe_context *ctx, struct pipe_resource *resource)
   {
      struct iris_context *ice = (struct iris_context *)ctx;
   struct iris_resource *res = (void *) resource;
            iris_resource_prepare_access(ice, res,
                              if (!res->mod_info && res->aux.usage != ISL_AUX_USAGE_NONE) {
      /* flush_resource may be used to prepare an image for sharing external
   * to the driver (e.g. via eglCreateImage). To account for this, make
   * sure to get rid of any compression that a consumer wouldn't know how
   * to handle.
   */
   iris_foreach_batch(ice, batch) {
      if (iris_batch_references(batch, res->bo))
                     }
      /**
   * Reallocate a (non-external) resource into new storage, copying the data
   * and modifying the original resource to point at the new storage.
   *
   * This is useful for e.g. moving a suballocated internal resource to a
   * dedicated allocation that can be exported by itself.
   */
   static void
   iris_reallocate_resource_inplace(struct iris_context *ice,
               {
               if (iris_bo_is_external(old_res->bo))
            assert(old_res->mod_info == NULL);
   assert(old_res->bo == old_res->aux.bo || old_res->aux.bo == NULL);
   assert(old_res->bo == old_res->aux.clear_color_bo ||
                  struct pipe_resource templ = old_res->base.b;
            struct iris_resource *new_res =
                              if (old_res->base.b.target == PIPE_BUFFER) {
      struct pipe_box box = (struct pipe_box) {
      .width = old_res->base.b.width0,
               iris_copy_region(&ice->blorp, batch, &new_res->base.b, 0, 0, 0, 0,
      } else {
      for (unsigned l = 0; l <= templ.last_level; l++) {
      struct pipe_box box = (struct pipe_box) {
      .width = u_minify(templ.width0, l),
   .height = u_minify(templ.height0, l),
               iris_copy_region(&ice->blorp, batch, &new_res->base.b, l, 0, 0, 0,
                           struct iris_bo *old_bo = old_res->bo;
   struct iris_bo *old_aux_bo = old_res->aux.bo;
            /* Replace the structure fields with the new ones */
   old_res->base.b.bind = templ.bind;
   old_res->bo = new_res->bo;
   old_res->aux.surf = new_res->aux.surf;
   old_res->aux.bo = new_res->aux.bo;
   old_res->aux.offset = new_res->aux.offset;
   old_res->aux.extra_aux.surf = new_res->aux.extra_aux.surf;
   old_res->aux.extra_aux.offset = new_res->aux.extra_aux.offset;
   old_res->aux.clear_color_bo = new_res->aux.clear_color_bo;
   old_res->aux.clear_color_offset = new_res->aux.clear_color_offset;
            if (new_res->aux.state) {
      assert(old_res->aux.state);
   for (unsigned l = 0; l <= templ.last_level; l++) {
      unsigned layers = util_num_layers(&templ, l);
   for (unsigned z = 0; z < layers; z++) {
      enum isl_aux_state aux =
                           /* old_res now points at the new BOs, make new_res point at the old ones
   * so they'll be freed when we unreference the resource below.
   */
   new_res->bo = old_bo;
   new_res->aux.bo = old_aux_bo;
               }
      static void
   iris_resource_disable_suballoc_on_first_query(struct pipe_screen *pscreen,
               {
      if (iris_bo_is_real(res->bo))
                     bool destroy_context;
   if (ctx) {
      ctx = threaded_context_unwrap_sync(ctx);
      } else {
      /* We need to execute a blit on some GPU context, but the DRI layer
   * often doesn't give us one.  So we have to invent a temporary one.
   *
   * We can't store a permanent context in the screen, as it would cause
   * circular refcounting where screens reference contexts that reference
   * resources, while resources reference screens...causing nothing to be
   * freed.  So we just create and destroy a temporary one here.
   */
   ctx = iris_create_context(pscreen, NULL, 0);
                        iris_reallocate_resource_inplace(ice, res, PIPE_BIND_SHARED);
            if (destroy_context)
      }
         static void
   iris_resource_disable_aux_on_first_query(struct pipe_resource *resource,
         {
      struct iris_resource *res = (struct iris_resource *)resource;
   bool mod_with_aux =
            /* Disable aux usage if explicit flush not set and this is the first time
   * we are dealing with this resource and the resource was not created with
   * a modifier with aux.
   */
   if (!mod_with_aux &&
      (!(usage & PIPE_HANDLE_USAGE_EXPLICIT_FLUSH) && res->aux.usage != 0) &&
   p_atomic_read(&resource->reference.count) == 1) {
         }
      static bool
   iris_resource_get_param(struct pipe_screen *pscreen,
                           struct pipe_context *ctx,
   struct pipe_resource *resource,
   unsigned plane,
   unsigned layer,
   {
      struct iris_screen *screen = (struct iris_screen *)pscreen;
   struct iris_resource *base_res = (struct iris_resource *)resource;
   unsigned main_plane = get_main_plane_for_plane(base_res->external_format,
         struct iris_resource *res = get_resource_for_plane(resource, main_plane);
            bool mod_with_aux =
         bool wants_aux = mod_with_aux && plane != main_plane;
   bool wants_cc = mod_with_aux &&
         bool result;
            iris_resource_disable_aux_on_first_query(resource, handle_usage);
            struct iris_bo *bo = wants_cc ? res->aux.clear_color_bo :
                     switch (param) {
   case PIPE_RESOURCE_PARAM_NPLANES:
      if (mod_with_aux) {
      *value = iris_get_dmabuf_modifier_planes(pscreen,
            } else {
         }
      case PIPE_RESOURCE_PARAM_STRIDE:
      *value = wants_cc ? 64 :
            /* Mesa's implementation of eglCreateImage rejects strides of zero (see
   * dri2_check_dma_buf_attribs). Ensure we return a non-zero stride as
   * this value may be queried from GBM and passed into EGL.
   *
   * Also, although modifiers which use a clear color plane specify that
   * the plane's pitch should be ignored, some kernels have been found to
   * require 64-byte alignment.
   */
               case PIPE_RESOURCE_PARAM_OFFSET:
      *value = wants_cc ? res->aux.clear_color_offset :
            case PIPE_RESOURCE_PARAM_MODIFIER:
      *value = res->mod_info ? res->mod_info->modifier :
            case PIPE_RESOURCE_PARAM_HANDLE_TYPE_SHARED:
      if (!wants_aux)
            result = iris_bo_flink(bo, &handle) == 0;
   if (result)
            case PIPE_RESOURCE_PARAM_HANDLE_TYPE_KMS: {
      if (!wants_aux)
            /* Because we share the same drm file across multiple iris_screen, when
   * we export a GEM handle we must make sure it is valid in the DRM file
   * descriptor the caller is using (this is the FD given at screen
   * creation).
   */
   uint32_t handle;
   if (iris_bo_export_gem_handle_for_device(bo, screen->winsys_fd, &handle))
         *value = handle;
               case PIPE_RESOURCE_PARAM_HANDLE_TYPE_FD:
      if (!wants_aux)
            result = iris_bo_export_dmabuf(bo, (int *) &handle) == 0;
   if (result)
            default:
            }
      static bool
   iris_resource_get_handle(struct pipe_screen *pscreen,
                           {
      struct iris_screen *screen = (struct iris_screen *) pscreen;
   struct iris_resource *res = (struct iris_resource *)resource;
   bool mod_with_aux =
            iris_resource_disable_aux_on_first_query(resource, usage);
                     struct iris_bo *bo;
   if (res->mod_info &&
      mod_plane_is_clear_color(res->mod_info->modifier, whandle->plane)) {
   bo = res->aux.clear_color_bo;
      } else if (mod_with_aux && whandle->plane > 0) {
      bo = res->aux.bo;
   whandle->stride = res->aux.surf.row_pitch_B;
      } else {
      /* If this is a buffer, stride should be 0 - no need to special case */
   whandle->stride = res->surf.row_pitch_B;
               whandle->format = res->external_format;
   whandle->modifier =
      res->mod_info ? res->mod_info->modifier
      #ifndef NDEBUG
      enum isl_aux_usage allowed_usage =
      (usage & PIPE_HANDLE_USAGE_EXPLICIT_FLUSH) || mod_with_aux ?
         if (res->aux.usage != allowed_usage) {
      enum isl_aux_state aux_state = iris_resource_get_aux_state(res, 0, 0);
   assert(aux_state == ISL_AUX_STATE_RESOLVED ||
         #endif
         switch (whandle->type) {
   case WINSYS_HANDLE_TYPE_SHARED:
      iris_gem_set_tiling(bo, &res->surf);
      case WINSYS_HANDLE_TYPE_KMS: {
               /* Because we share the same drm file across multiple iris_screen, when
   * we export a GEM handle we must make sure it is valid in the DRM file
   * descriptor the caller is using (this is the FD given at screen
   * creation).
   */
   uint32_t handle;
   if (iris_bo_export_gem_handle_for_device(bo, screen->winsys_fd, &handle))
         whandle->handle = handle;
      }
   case WINSYS_HANDLE_TYPE_FD:
      iris_gem_set_tiling(bo, &res->surf);
                  }
      static bool
   resource_is_busy(struct iris_context *ice,
         {
               iris_foreach_batch(ice, batch)
               }
      void
   iris_replace_buffer_storage(struct pipe_context *ctx,
                                 {
      struct iris_screen *screen = (void *) ctx->screen;
   struct iris_context *ice = (void *) ctx;
   struct iris_resource *dst = (void *) p_dst;
                              /* Swap out the backing storage */
   iris_bo_reference(src->bo);
            /* Rebind the buffer, replacing any state referring to the old BO's
   * address, and marking state dirty so it's reemitted.
   */
               }
      /**
   * Discard a buffer's contents and replace it's backing storage with a
   * fresh, idle buffer if necessary.
   *
   * Returns true if the storage can be considered idle.
   */
   static bool
   iris_invalidate_buffer(struct iris_context *ice, struct iris_resource *res)
   {
               if (res->base.b.target != PIPE_BUFFER)
            /* If it's already invalidated, don't bother doing anything.
   * We consider the storage to be idle, because either it was freshly
   * allocated (and not busy), or a previous call here was what cleared
   * the range, and that call replaced the storage with an idle buffer.
   */
   if (res->valid_buffer_range.start > res->valid_buffer_range.end)
            if (!resource_is_busy(ice, res)) {
      /* The resource is idle, so just mark that it contains no data and
   * keep using the same underlying buffer object.
   */
   util_range_set_empty(&res->valid_buffer_range);
                        /* We can't reallocate memory we didn't allocate in the first place. */
   if (res->bo->gem_handle && res->bo->real.userptr)
            /* Nor can we allocate buffers we imported or exported. */
   if (iris_bo_is_external(res->bo))
            struct iris_bo *old_bo = res->bo;
   unsigned flags = old_bo->real.protected ? BO_ALLOC_PROTECTED : BO_ALLOC_PLAIN;
   struct iris_bo *new_bo =
      iris_bo_alloc(screen->bufmgr, res->bo->name, res->base.b.width0,
                  if (!new_bo)
            /* Swap out the backing storage */
            /* Rebind the buffer, replacing any state referring to the old BO's
   * address, and marking state dirty so it's reemitted.
   */
                              /* The new buffer is idle. */
      }
      static void
   iris_invalidate_resource(struct pipe_context *ctx,
         {
      struct iris_context *ice = (void *) ctx;
               }
      static void
   iris_flush_staging_region(struct pipe_transfer *xfer,
         {
      if (!(xfer->usage & PIPE_MAP_WRITE))
                              /* Account for extra alignment padding in staging buffer */
   if (xfer->resource->target == PIPE_BUFFER)
            struct pipe_box dst_box = (struct pipe_box) {
      .x = xfer->box.x + flush_box->x,
   .y = xfer->box.y + flush_box->y,
   .z = xfer->box.z + flush_box->z,
   .width = flush_box->width,
   .height = flush_box->height,
               iris_copy_region(map->blorp, map->batch, xfer->resource, xfer->level,
            }
      static void
   iris_unmap_copy_region(struct iris_transfer *map)
   {
                  }
      static void
   iris_map_copy_region(struct iris_transfer *map)
   {
      struct pipe_screen *pscreen = &map->batch->screen->base;
   struct pipe_transfer *xfer = &map->base.b;
   struct pipe_box *box = &xfer->box;
            unsigned extra = xfer->resource->target == PIPE_BUFFER ?
            struct pipe_resource templ = (struct pipe_resource) {
      .usage = PIPE_USAGE_STAGING,
   .width0 = box->width + extra,
   .height0 = box->height,
   .depth0 = 1,
   .nr_samples = xfer->resource->nr_samples,
   .nr_storage_samples = xfer->resource->nr_storage_samples,
   .array_size = box->depth,
               if (xfer->resource->target == PIPE_BUFFER) {
      templ.target = PIPE_BUFFER;
      } else {
      templ.target = templ.array_size > 1 ? PIPE_TEXTURE_2D_ARRAY
               #ifdef ANDROID
         /* Staging buffers for stall-avoidance blits don't always have the
   * same restrictions on stride as the original buffer.  For example,
   * the original buffer may be used for scanout, while the staging
   * buffer will not be.  So we may compute a smaller stride for the
   * staging buffer than the original.
   *
   * Normally, this is good, as it saves memory.  Unfortunately, for
   * Android, gbm_gralloc incorrectly asserts that the stride returned
   * by gbm_bo_map() must equal the result of gbm_bo_get_stride(),
   * which simply isn't always the case.
   *
   * Because gralloc is unlikely to be fixed, we hack around it in iris
   * by forcing the staging buffer to have a matching stride.
   */
   if (iris_bo_is_external(res->bo))
   #endif
            map->staging =
               /* If we fail to create a staging resource, the caller will fallback
   * to mapping directly on the CPU.
   */
   if (!map->staging)
            if (templ.target != PIPE_BUFFER) {
      struct isl_surf *surf = &((struct iris_resource *) map->staging)->surf;
   xfer->stride = isl_surf_get_row_pitch_B(surf);
               if ((xfer->usage & PIPE_MAP_READ) ||
      (res->base.b.target == PIPE_BUFFER &&
   !(xfer->usage & PIPE_MAP_DISCARD_RANGE))) {
   iris_copy_region(map->blorp, map->batch, map->staging, 0, extra, 0, 0,
         /* Ensure writes to the staging BO land before we map it below. */
   iris_emit_pipe_control_flush(map->batch,
                                          if (iris_batch_references(map->batch, staging_bo))
            assert(((struct iris_resource *)map->staging)->offset == 0);
   map->ptr =
               }
      static void
   get_image_offset_el(const struct isl_surf *surf, unsigned level, unsigned z,
         {
      ASSERTED uint32_t z0_el, a0_el;
   if (surf->dim == ISL_SURF_DIM_3D) {
      isl_surf_get_image_offset_el(surf, level, 0, z,
      } else {
      isl_surf_get_image_offset_el(surf, level, z, 0,
      }
      }
      /**
   * Get pointer offset into stencil buffer.
   *
   * The stencil buffer is W tiled. Since the GTT is incapable of W fencing, we
   * must decode the tile's layout in software.
   *
   * See
   *   - PRM, 2011 Sandy Bridge, Volume 1, Part 2, Section 4.5.2.1 W-Major Tile
   *     Format.
   *   - PRM, 2011 Sandy Bridge, Volume 1, Part 2, Section 4.5.3 Tiling Algorithm
   *
   * Even though the returned offset is always positive, the return type is
   * signed due to
   *    commit e8b1c6d6f55f5be3bef25084fdd8b6127517e137
   *    mesa: Fix return type of  _mesa_get_format_bytes() (#37351)
   */
   static intptr_t
   s8_offset(uint32_t stride, uint32_t x, uint32_t y)
   {
      uint32_t tile_size = 4096;
   uint32_t tile_width = 64;
   uint32_t tile_height = 64;
            uint32_t tile_x = x / tile_width;
            /* The byte's address relative to the tile's base addres. */
   uint32_t byte_x = x % tile_width;
            uintptr_t u = tile_y * row_size
               + tile_x * tile_size
   + 512 * (byte_x / 8)
   +  64 * (byte_y / 8)
   +  32 * ((byte_y / 4) % 2)
   +  16 * ((byte_x / 4) % 2)
   +   8 * ((byte_y / 2) % 2)
               }
      static void
   iris_unmap_s8(struct iris_transfer *map)
   {
      struct pipe_transfer *xfer = &map->base.b;
   const struct pipe_box *box = &xfer->box;
   struct iris_resource *res = (struct iris_resource *) xfer->resource;
            if (xfer->usage & PIPE_MAP_WRITE) {
      uint8_t *untiled_s8_map = map->ptr;
   uint8_t *tiled_s8_map = res->offset +
            for (int s = 0; s < box->depth; s++) {
                     for (uint32_t y = 0; y < box->height; y++) {
      for (uint32_t x = 0; x < box->width; x++) {
      ptrdiff_t offset = s8_offset(surf->row_pitch_B,
               tiled_s8_map[offset] =
                           }
      static void
   iris_map_s8(struct iris_transfer *map)
   {
      struct pipe_transfer *xfer = &map->base.b;
   const struct pipe_box *box = &xfer->box;
   struct iris_resource *res = (struct iris_resource *) xfer->resource;
            xfer->stride = surf->row_pitch_B;
            /* The tiling and detiling functions require that the linear buffer has
   * a 16-byte alignment (that is, its `x0` is 16-byte aligned).  Here we
   * over-allocate the linear buffer to get the proper alignment.
   */
   map->buffer = map->ptr = malloc(xfer->layer_stride * box->depth);
            /* One of either READ_BIT or WRITE_BIT or both is set.  READ_BIT implies no
   * INVALIDATE_RANGE_BIT.  WRITE_BIT needs the original values read in unless
   * invalidate is set, since we'll be writing the whole rectangle from our
   * temporary buffer back out.
   */
   if (xfer->usage & PIPE_MAP_READ) {
      uint8_t *untiled_s8_map = map->ptr;
   uint8_t *tiled_s8_map = res->offset +
            for (int s = 0; s < box->depth; s++) {
                     for (uint32_t y = 0; y < box->height; y++) {
      for (uint32_t x = 0; x < box->width; x++) {
      ptrdiff_t offset = s8_offset(surf->row_pitch_B,
               untiled_s8_map[s * xfer->layer_stride + y * xfer->stride + x] =
                           }
      /* Compute extent parameters for use with tiled_memcpy functions.
   * xs are in units of bytes and ys are in units of strides.
   */
   static inline void
   tile_extents(const struct isl_surf *surf,
               const struct pipe_box *box,
   unsigned level, int z,
   {
      const struct isl_format_layout *fmtl = isl_format_get_layout(surf->format);
            assert(box->x % fmtl->bw == 0);
            unsigned x0_el, y0_el;
            *x1_B = (box->x / fmtl->bw + x0_el) * cpp;
   *y1_el = box->y / fmtl->bh + y0_el;
   *x2_B = (DIV_ROUND_UP(box->x + box->width, fmtl->bw) + x0_el) * cpp;
      }
      static void
   iris_unmap_tiled_memcpy(struct iris_transfer *map)
   {
      struct pipe_transfer *xfer = &map->base.b;
   const struct pipe_box *box = &xfer->box;
   struct iris_resource *res = (struct iris_resource *) xfer->resource;
                     if (xfer->usage & PIPE_MAP_WRITE) {
      char *dst = res->offset +
            for (int s = 0; s < box->depth; s++) {
                              isl_memcpy_linear_to_tiled(x1, x2, y1, y2, dst, ptr,
               }
   os_free_aligned(map->buffer);
      }
      static void
   iris_map_tiled_memcpy(struct iris_transfer *map)
   {
      struct pipe_transfer *xfer = &map->base.b;
   const struct pipe_box *box = &xfer->box;
   struct iris_resource *res = (struct iris_resource *) xfer->resource;
            xfer->stride = ALIGN(surf->row_pitch_B, 16);
            unsigned x1, x2, y1, y2;
            /* The tiling and detiling functions require that the linear buffer has
   * a 16-byte alignment (that is, its `x0` is 16-byte aligned).  Here we
   * over-allocate the linear buffer to get the proper alignment.
   */
   map->buffer =
         assert(map->buffer);
                     if (xfer->usage & PIPE_MAP_READ) {
      char *src = res->offset +
            for (int s = 0; s < box->depth; s++) {
                                    isl_memcpy_tiled_to_linear(x1, x2, y1, y2, ptr, src, xfer->stride,
      #if defined(USE_SSE41)
               #endif
                              }
      static void
   iris_map_direct(struct iris_transfer *map)
   {
      struct pipe_transfer *xfer = &map->base.b;
   struct pipe_box *box = &xfer->box;
            void *ptr = res->offset +
            if (res->base.b.target == PIPE_BUFFER) {
      xfer->stride = 0;
               } else {
      struct isl_surf *surf = &res->surf;
   const struct isl_format_layout *fmtl =
         const unsigned cpp = fmtl->bpb / 8;
            assert(box->x % fmtl->bw == 0);
   assert(box->y % fmtl->bh == 0);
            x0_el += box->x / fmtl->bw;
            xfer->stride = isl_surf_get_row_pitch_B(surf);
                  }
      static bool
   can_promote_to_async(const struct iris_resource *res,
               {
      /* If we're writing to a section of the buffer that hasn't even been
   * initialized with useful data, then we can safely promote this write
   * to be unsynchronized.  This helps the common pattern of appending data.
   */
   return res->base.b.target == PIPE_BUFFER && (usage & PIPE_MAP_WRITE) &&
         !(usage & TC_TRANSFER_MAP_NO_INFER_UNSYNCHRONIZED) &&
      }
      static bool
   prefer_cpu_access(const struct iris_resource *res,
                     const struct pipe_box *box,
   {
               /* We must be able to map it. */
   if (mmap_mode == IRIS_MMAP_NONE)
            const bool write = usage & PIPE_MAP_WRITE;
   const bool read = usage & PIPE_MAP_READ;
   const bool preserve =
            /* We want to avoid uncached reads because they are slow. */
   if (read && mmap_mode != IRIS_MMAP_WB)
            /* We want to avoid stalling.  We can't avoid stalling for reads, though,
   * because the destination of a GPU staging copy would be busy and stall
   * in the exact same manner.  So don't consider it for those.
   *
   * For buffer maps which aren't invalidating the destination, the GPU
   * staging copy path would have to read the existing buffer contents in
   * order to preserve them, effectively making it a read.  But a direct
   * mapping would be able to just write the necessary parts without the
   * overhead of the copy.  It may stall, but we would anyway.
   */
   if (map_would_stall && !read && !preserve)
            /* Use the GPU for writes if it would compress the data. */
   if (write && isl_aux_usage_has_compression(res->aux.usage))
            /* Writes & Cached CPU reads are fine as long as the primary is valid. */
      }
      static void *
   iris_transfer_map(struct pipe_context *ctx,
                     struct pipe_resource *resource,
   unsigned level,
   {
      struct iris_context *ice = (struct iris_context *)ctx;
   struct iris_resource *res = (struct iris_resource *)resource;
            /* From GL_AMD_pinned_memory issues:
   *
   *     4) Is glMapBuffer on a shared buffer guaranteed to return the
   *        same system address which was specified at creation time?
   *
   *        RESOLVED: NO. The GL implementation might return a different
   *        virtual mapping of that memory, although the same physical
   *        page will be used.
   *
   * So don't ever use staging buffers.
   */
   if (res->base.is_user_ptr)
            /* Promote discarding a range to discarding the entire buffer where
   * possible.  This may allow us to replace the backing storage entirely
   * and let us do an unsynchronized map when we otherwise wouldn't.
   */
   if (resource->target == PIPE_BUFFER &&
      (usage & PIPE_MAP_DISCARD_RANGE) &&
   box->x == 0 && box->width == resource->width0) {
               if (usage & PIPE_MAP_DISCARD_WHOLE_RESOURCE) {
      /* Replace the backing storage with a fresh buffer for non-async maps */
   if (!(usage & (PIPE_MAP_UNSYNCHRONIZED | TC_TRANSFER_MAP_NO_INVALIDATE))
                  /* If we can discard the whole resource, we can discard the range. */
               if (!(usage & PIPE_MAP_UNSYNCHRONIZED) &&
      can_promote_to_async(res, box, usage)) {
               /* Avoid using GPU copies for persistent/coherent buffers, as the idea
   * there is to access them simultaneously on the CPU & GPU.  This also
   * avoids trying to use GPU copies for our u_upload_mgr buffers which
   * contain state we're constructing for a GPU draw call, which would
   * kill us with infinite stack recursion.
   */
   if (usage & (PIPE_MAP_PERSISTENT | PIPE_MAP_COHERENT))
            /* We cannot provide a direct mapping of tiled resources, and we
   * may not be able to mmap imported BOs since they may come from
   * other devices that I915_GEM_MMAP cannot work with.
   */
   if ((usage & PIPE_MAP_DIRECTLY) &&
      (surf->tiling != ISL_TILING_LINEAR || iris_bo_is_imported(res->bo)))
                  if (!(usage & PIPE_MAP_UNSYNCHRONIZED)) {
      map_would_stall =
                  if (map_would_stall && (usage & PIPE_MAP_DONTBLOCK) &&
                              if (usage & PIPE_MAP_THREAD_SAFE)
         else if (usage & TC_TRANSFER_MAP_THREADED_UNSYNC)
         else
            if (!map)
                              pipe_resource_reference(&xfer->resource, resource);
   xfer->level = level;
   xfer->usage = usage;
   xfer->box = *box;
            if (usage & PIPE_MAP_WRITE)
            if (prefer_cpu_access(res, box, usage, level, map_would_stall))
            /* TODO: Teach iris_map_tiled_memcpy about Tile64... */
   if (res->surf.tiling == ISL_TILING_64)
            if (!(usage & PIPE_MAP_DIRECTLY)) {
      /* If we need a synchronous mapping and the resource is busy, or needs
   * resolving, we copy to/from a linear temporary buffer using the GPU.
   */
   map->batch = &ice->batches[IRIS_BATCH_RENDER];
   map->blorp = &ice->blorp;
               /* If we've requested a direct mapping, or iris_map_copy_region failed
   * to create a staging resource, then map it directly on the CPU.
   */
   if (!map->ptr) {
      if (resource->target != PIPE_BUFFER) {
      iris_resource_access_raw(ice, res, level, box->z, box->depth,
               if (!(usage & PIPE_MAP_UNSYNCHRONIZED)) {
      iris_foreach_batch(ice, batch) {
      if (iris_batch_references(batch, res->bo))
                  if (surf->tiling == ISL_TILING_W) {
      /* TODO: Teach iris_map_tiled_memcpy about W-tiling... */
      } else if (surf->tiling != ISL_TILING_LINEAR) {
         } else {
                        }
      static void
   iris_transfer_flush_region(struct pipe_context *ctx,
               {
      struct iris_context *ice = (struct iris_context *)ctx;
   struct iris_resource *res = (struct iris_resource *) xfer->resource;
            if (map->staging)
            if (res->base.b.target == PIPE_BUFFER) {
                  /* Make sure we flag constants dirty even if there's no need to emit
   * any PIPE_CONTROLs to a batch.
   */
      }
      static void
   iris_transfer_unmap(struct pipe_context *ctx, struct pipe_transfer *xfer)
   {
      struct iris_context *ice = (struct iris_context *)ctx;
            if (!(xfer->usage & (PIPE_MAP_FLUSH_EXPLICIT |
            struct pipe_box flush_box = {
      .x = 0, .y = 0, .z = 0,
   .width  = xfer->box.width,
   .height = xfer->box.height,
      };
               if (map->unmap)
                     if (xfer->usage & PIPE_MAP_THREAD_SAFE) {
         } else {
      /* transfer_unmap is called from the driver thread, so we have to use
   * transfer_pool, not transfer_pool_unsync.  Freeing an object into a
   * different pool is allowed, however.
   */
         }
      /**
   * The pipe->texture_subdata() driver hook.
   *
   * Mesa's state tracker takes this path whenever possible, even with
   * PIPE_CAP_TEXTURE_TRANSFER_MODES set.
   */
   static void
   iris_texture_subdata(struct pipe_context *ctx,
                        struct pipe_resource *resource,
   unsigned level,
   unsigned usage,
      {
      struct iris_context *ice = (struct iris_context *)ctx;
   struct iris_resource *res = (struct iris_resource *)resource;
                     /* Just use the transfer-based path for linear buffers - it will already
   * do a direct mapping, or a simple linear staging buffer.
   *
   * Linear staging buffers appear to be better than tiled ones, too, so
   * take that path if we need the GPU to perform color compression, or
   * stall-avoidance blits.
   *
   * TODO: Teach isl_memcpy_linear_to_tiled about Tile64...
   */
   if (surf->tiling == ISL_TILING_LINEAR ||
      surf->tiling == ISL_TILING_64 ||
   isl_aux_usage_has_compression(res->aux.usage) ||
   resource_is_busy(ice, res) ||
   iris_bo_mmap_mode(res->bo) == IRIS_MMAP_NONE) {
   return u_default_texture_subdata(ctx, resource, level, usage, box,
                                 iris_foreach_batch(ice, batch) {
      if (iris_batch_references(batch, res->bo))
                        for (int s = 0; s < box->depth; s++) {
               if (surf->tiling == ISL_TILING_W) {
                     for (unsigned y = 0; y < box->height; y++) {
      for (unsigned x = 0; x < box->width; x++) {
      ptrdiff_t offset = s8_offset(surf->row_pitch_B,
                        } else {
                        isl_memcpy_linear_to_tiled(x1, x2, y1, y2,
                        }
      /**
   * Mark state dirty that needs to be re-emitted when a resource is written.
   */
   void
   iris_dirty_for_history(struct iris_context *ice,
         {
      const uint64_t stages = res->bind_stages;
   uint64_t dirty = 0ull;
            if (res->bind_history & PIPE_BIND_CONSTANT_BUFFER) {
      for (unsigned stage = 0; stage < MESA_SHADER_STAGES; stage++) {
      if (stages & (1u << stage)) {
      struct iris_shader_state *shs = &ice->state.shaders[stage];
         }
   dirty |= IRIS_DIRTY_RENDER_MISC_BUFFER_FLUSHES |
                     if (res->bind_history & (PIPE_BIND_SAMPLER_VIEW |
            dirty |= IRIS_DIRTY_RENDER_RESOLVES_AND_FLUSHES |
                     if (res->bind_history & PIPE_BIND_SHADER_BUFFER) {
      dirty |= IRIS_DIRTY_RENDER_MISC_BUFFER_FLUSHES |
                     if (res->bind_history & PIPE_BIND_VERTEX_BUFFER)
            if (ice->state.streamout_active && (res->bind_history & PIPE_BIND_STREAM_OUTPUT))
            ice->state.dirty |= dirty;
      }
      bool
   iris_resource_set_clear_color(struct iris_context *ice,
               {
      if (res->aux.clear_color_unknown ||
      memcmp(&res->aux.clear_color, &color, sizeof(color)) != 0) {
   res->aux.clear_color = color;
   res->aux.clear_color_unknown = false;
                  }
      static enum pipe_format
   iris_resource_get_internal_format(struct pipe_resource *p_res)
   {
      struct iris_resource *res = (void *) p_res;
      }
      static const struct u_transfer_vtbl transfer_vtbl = {
      .resource_create       = iris_resource_create,
   .resource_destroy      = iris_resource_destroy,
   .transfer_map          = iris_transfer_map,
   .transfer_unmap        = iris_transfer_unmap,
   .transfer_flush_region = iris_transfer_flush_region,
   .get_internal_format   = iris_resource_get_internal_format,
   .set_stencil           = iris_resource_set_separate_stencil,
      };
      void
   iris_init_screen_resource_functions(struct pipe_screen *pscreen)
   {
      pscreen->query_dmabuf_modifiers = iris_query_dmabuf_modifiers;
   pscreen->is_dmabuf_modifier_supported = iris_is_dmabuf_modifier_supported;
   pscreen->get_dmabuf_modifier_planes = iris_get_dmabuf_modifier_planes;
   pscreen->resource_create_with_modifiers =
         pscreen->resource_create = u_transfer_helper_resource_create;
   pscreen->resource_from_user_memory = iris_resource_from_user_memory;
   pscreen->resource_from_handle = iris_resource_from_handle;
   pscreen->resource_from_memobj = iris_resource_from_memobj_wrapper;
   pscreen->resource_get_handle = iris_resource_get_handle;
   pscreen->resource_get_param = iris_resource_get_param;
   pscreen->resource_destroy = u_transfer_helper_resource_destroy;
   pscreen->memobj_create_from_handle = iris_memobj_create_from_handle;
   pscreen->memobj_destroy = iris_memobj_destroy;
   pscreen->transfer_helper =
      u_transfer_helper_create(&transfer_vtbl,
               }
      void
   iris_init_resource_functions(struct pipe_context *ctx)
   {
      ctx->flush_resource = iris_flush_resource;
   ctx->invalidate_resource = iris_invalidate_resource;
   ctx->buffer_map = u_transfer_helper_transfer_map;
   ctx->texture_map = u_transfer_helper_transfer_map;
   ctx->transfer_flush_region = u_transfer_helper_transfer_flush_region;
   ctx->buffer_unmap = u_transfer_helper_transfer_unmap;
   ctx->texture_unmap = u_transfer_helper_transfer_unmap;
   ctx->buffer_subdata = u_default_buffer_subdata;
   ctx->clear_buffer = u_default_clear_buffer;
      }
