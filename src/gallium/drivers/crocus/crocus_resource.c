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
   * @file crocus_resource.c
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
   #include "util/u_threaded_context.h"
   #include "util/u_transfer.h"
   #include "util/u_transfer_helper.h"
   #include "util/u_upload_mgr.h"
   #include "util/ralloc.h"
   #include "util/u_memory.h"
   #include "crocus_batch.h"
   #include "crocus_context.h"
   #include "crocus_resource.h"
   #include "crocus_screen.h"
   #include "intel/dev/intel_debug.h"
   #include "isl/isl.h"
   #include "drm-uapi/drm_fourcc.h"
   #include "drm-uapi/i915_drm.h"
      enum modifier_priority {
      MODIFIER_PRIORITY_INVALID = 0,
   MODIFIER_PRIORITY_LINEAR,
   MODIFIER_PRIORITY_X,
   MODIFIER_PRIORITY_Y,
      };
      static const uint64_t priority_to_modifier[] = {
      [MODIFIER_PRIORITY_INVALID] = DRM_FORMAT_MOD_INVALID,
   [MODIFIER_PRIORITY_LINEAR] = DRM_FORMAT_MOD_LINEAR,
   [MODIFIER_PRIORITY_X] = I915_FORMAT_MOD_X_TILED,
      };
      static bool
   modifier_is_supported(const struct intel_device_info *devinfo,
               {
      /* XXX: do something real */
   switch (modifier) {
   case I915_FORMAT_MOD_Y_TILED:
      if (bind & PIPE_BIND_SCANOUT)
            case I915_FORMAT_MOD_X_TILED:
   case DRM_FORMAT_MOD_LINEAR:
         case DRM_FORMAT_MOD_INVALID:
   default:
            }
      static uint64_t
   select_best_modifier(struct intel_device_info *devinfo,
                     {
               for (int i = 0; i < count; i++) {
      if (!modifier_is_supported(devinfo, templ->format, templ->bind,
                  switch (modifiers[i]) {
   case I915_FORMAT_MOD_Y_TILED:
      prio = MAX2(prio, MODIFIER_PRIORITY_Y);
      case I915_FORMAT_MOD_X_TILED:
      prio = MAX2(prio, MODIFIER_PRIORITY_X);
      case DRM_FORMAT_MOD_LINEAR:
      prio = MAX2(prio, MODIFIER_PRIORITY_LINEAR);
      case DRM_FORMAT_MOD_INVALID:
   default:
                        }
      static enum isl_surf_dim
   crocus_target_to_isl_surf_dim(enum pipe_texture_target target)
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
      static isl_surf_usage_flags_t
   pipe_bind_to_isl_usage(unsigned bindings)
   {
               if (bindings & PIPE_BIND_RENDER_TARGET)
            if (bindings & PIPE_BIND_SAMPLER_VIEW)
            if (bindings & (PIPE_BIND_SHADER_IMAGE | PIPE_BIND_SHADER_BUFFER))
            if (bindings & PIPE_BIND_SCANOUT)
            }
      static bool
   crocus_resource_configure_main(const struct crocus_screen *screen,
                     {
      const struct intel_device_info *devinfo = &screen->devinfo;
   const struct util_format_description *format_desc =
         const bool has_depth = util_format_has_depth(format_desc);
   isl_surf_usage_flags_t usage = pipe_bind_to_isl_usage(templ->bind);
            /* TODO: This used to be because there wasn't BLORP to handle Y-tiling. */
   if (devinfo->ver < 6 && !util_format_is_depth_or_stencil(templ->format))
            if (modifier != DRM_FORMAT_MOD_INVALID) {
                  } else {
      if (templ->bind & PIPE_BIND_RENDER_TARGET && devinfo->ver < 6)
         /* Use linear for staging buffers */
   if (templ->usage == PIPE_USAGE_STAGING ||
      templ->bind & (PIPE_BIND_LINEAR | PIPE_BIND_CURSOR) )
      else if (templ->bind & PIPE_BIND_SCANOUT)
      tiling_flags = screen->devinfo.has_tiling_uapi ?
            if (templ->target == PIPE_TEXTURE_CUBE ||
      templ->target == PIPE_TEXTURE_CUBE_ARRAY)
         if (templ->usage != PIPE_USAGE_STAGING) {
      if (templ->format == PIPE_FORMAT_S8_UINT)
         else if (has_depth) {
      /* combined DS only on gen4/5 */
   if (devinfo->ver < 6) {
      if (templ->format == PIPE_FORMAT_Z24X8_UNORM ||
      templ->format == PIPE_FORMAT_Z24_UNORM_S8_UINT ||
   templ->format == PIPE_FORMAT_Z32_FLOAT_S8X24_UINT)
   }
               if (templ->format == PIPE_FORMAT_S8_UINT)
               const enum isl_format format =
            if (row_pitch_B == 0 && templ->usage == PIPE_USAGE_STAGING &&
      templ->target == PIPE_TEXTURE_2D &&
   devinfo->ver < 6) {
   /* align row pitch to 4 so we can keep using BLT engine */
   row_pitch_B = util_format_get_stride(templ->format, templ->width0);
               const struct isl_surf_init_info init_info = {
      .dim = crocus_target_to_isl_surf_dim(templ->target),
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
            /*
   * Don't create staging surfaces that will use > half the aperture
   * since staging implies you are sending to another resource,
   * which there is no way to fit both into aperture.
   */
   if (templ->usage == PIPE_USAGE_STAGING)
      if (res->surf.size_B > screen->aperture_threshold / 2)
                     }
      static void
   crocus_query_dmabuf_modifiers(struct pipe_screen *pscreen,
                                 {
      struct crocus_screen *screen = (void *) pscreen;
            uint64_t all_modifiers[] = {
      DRM_FORMAT_MOD_LINEAR,
   I915_FORMAT_MOD_X_TILED,
                        for (int i = 0; i < ARRAY_SIZE(all_modifiers); i++) {
      if (!modifier_is_supported(devinfo, pfmt, 0, all_modifiers[i]))
            if (supported_mods < max) {
                     if (external_only)
                              }
      static struct pipe_resource *
   crocus_resource_get_separate_stencil(struct pipe_resource *p_res)
   {
         }
      static void
   crocus_resource_set_separate_stencil(struct pipe_resource *p_res,
         {
      assert(util_format_has_depth(util_format_description(p_res->format)));
      }
      void
   crocus_resource_disable_aux(struct crocus_resource *res)
   {
      crocus_bo_unreference(res->aux.bo);
            res->aux.usage = ISL_AUX_USAGE_NONE;
   res->aux.has_hiz = 0;
   res->aux.surf.size_B = 0;
   res->aux.surf.levels = 0;
   res->aux.bo = NULL;
      }
      static void
   crocus_resource_destroy(struct pipe_screen *screen,
         {
               if (resource->target == PIPE_BUFFER)
            if (res->shadow)
                  threaded_resource_deinit(resource);
   crocus_bo_unreference(res->bo);
   crocus_pscreen_unref(res->orig_screen);
      }
      static struct crocus_resource *
   crocus_alloc_resource(struct pipe_screen *pscreen,
         {
      struct crocus_resource *res = calloc(1, sizeof(struct crocus_resource));
   if (!res)
            res->base.b = *templ;
   res->base.b.screen = pscreen;
   res->orig_screen = crocus_pscreen_ref(pscreen);
   pipe_reference_init(&res->base.b.reference, 1);
            if (templ->target == PIPE_BUFFER)
               }
      unsigned
   crocus_get_num_logical_layers(const struct crocus_resource *res, unsigned level)
   {
      if (res->surf.dim == ISL_SURF_DIM_3D)
         else
      }
      static enum isl_aux_state **
   create_aux_state_map(struct crocus_resource *res, enum isl_aux_state initial)
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
   const unsigned level_layers = crocus_get_num_logical_layers(res, level);
   for (uint32_t a = 0; a < level_layers; a++)
      }
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
   crocus_resource_configure_aux(struct crocus_screen *screen,
                     {
               /* Modifiers with compression are not supported. */
   assert(!res->mod_info ||
            const bool has_mcs = devinfo->ver >= 7 && !res->mod_info &&
            const bool has_hiz = devinfo->ver >= 6 && !res->mod_info &&
            const bool has_ccs =
      devinfo->ver >= 7 && !res->mod_info &&
   isl_surf_get_ccs_surf(&screen->isl_dev, &res->surf, NULL,
         /* Having more than one type of compression is impossible */
            if (has_mcs) {
         } else if (has_hiz) {
         } else if (has_ccs) {
      if (isl_format_supports_ccs_d(devinfo, res->surf.format))
               enum isl_aux_state initial_state = ISL_AUX_STATE_AUX_INVALID;
   *aux_size_B = 0;
   *alloc_flags = 0;
            switch (res->aux.usage) {
   case ISL_AUX_USAGE_NONE:
      res->aux.surf.levels = 0;
      case ISL_AUX_USAGE_HIZ:
      initial_state = ISL_AUX_STATE_AUX_INVALID;
      case ISL_AUX_USAGE_MCS:
      /* The Ivybridge PRM, Vol 2 Part 1 p326 says:
   *
   *    "When MCS buffer is enabled and bound to MSRT, it is required
   *     that it is cleared prior to any rendering."
   *
   * Since we only use the MCS buffer for rendering, we just clear it
   * immediately on allocation.  The clear value for MCS buffers is all
   * 1's, so we simply memset it to 0xff.
   */
   initial_state = ISL_AUX_STATE_CLEAR;
      case ISL_AUX_USAGE_CCS_D:
      /* When CCS_E is used, we need to ensure that the CCS starts off in
   * a valid state.  From the Sky Lake PRM, "MCS Buffer for Render
   * Target(s)":
   *
   *    "If Software wants to enable Color Compression without Fast
   *     clear, Software needs to initialize MCS with zeros."
   *
   * A CCS value of 0 indicates that the corresponding block is in the
   * pass-through state which is what we want.
   *
   * For CCS_D, do the same thing.  On Gen9+, this avoids having any
   * undefined bits in the aux buffer.
   */
   initial_state = ISL_AUX_STATE_PASS_THROUGH;
   *alloc_flags |= BO_ALLOC_ZEROED;
      default:
                  /* Create the aux_state for the auxiliary buffer. */
   res->aux.state = create_aux_state_map(res, initial_state);
   if (!res->aux.state)
            /* Increase the aux offset if the main and aux surfaces will share a BO. */
   res->aux.offset = ALIGN(res->surf.size_B, res->aux.surf.alignment_B);
            /* Allocate space in the buffer for storing the clear color. On modern
   * platforms (gen > 9), we can read it directly from such buffer.
   *
   * On gen <= 9, we are going to store the clear color on the buffer
   * anyways, and copy it back to the surface state during state emission.
   *
   * Also add some padding to make sure the fast clear color state buffer
   * starts at a 4K alignment. We believe that 256B might be enough, but due
   * to lack of testing we will leave this as 4K for now.
   */
   size = ALIGN(size, 4096);
            if (isl_aux_usage_has_hiz(res->aux.usage)) {
      for (unsigned level = 0; level < res->surf.levels; ++level) {
                     /* Disable HiZ for LOD > 0 unless the width/height are 8x4 aligned.
   * For LOD == 0, we can grow the dimensions to make it work.
   */
   if (devinfo->verx10 < 75 ||
      (level == 0 || ((width & 7) == 0 && (height & 3) == 0)))
                  }
      /**
   * Initialize the aux buffer contents.
   *
   * Returns false on unexpected error (e.g. mapping a BO failed).
   */
   static bool
   crocus_resource_init_aux_buf(struct crocus_resource *res, uint32_t alloc_flags)
   {
      if (!(alloc_flags & BO_ALLOC_ZEROED)) {
               if (!map)
            if (crocus_resource_get_aux_state(res, 0, 0) != ISL_AUX_STATE_AUX_INVALID) {
      uint8_t memset_value = isl_aux_usage_has_mcs(res->aux.usage) ? 0xFF : 0;
   memset((char*)map + res->aux.offset, memset_value,
                              }
      /**
   * Allocate the initial aux surface for a resource based on aux.usage
   *
   * Returns false on unexpected error (e.g. allocation failed, or invalid
   * configuration result).
   */
   static bool
   crocus_resource_alloc_separate_aux(struct crocus_screen *screen,
         {
      uint32_t alloc_flags;
   uint64_t size;
   if (!crocus_resource_configure_aux(screen, res, &size, &alloc_flags))
            if (size == 0)
            /* Allocate the auxiliary buffer.  ISL has stricter set of alignment rules
   * the drm allocator.  Therefore, one can pass the ISL dimensions in terms
   * of bytes instead of trying to recalculate based on different format
   * block sizes.
   */
   res->aux.bo = crocus_bo_alloc_tiled(screen->bufmgr, "aux buffer", size, 4096,
               if (!res->aux.bo) {
                  if (!crocus_resource_init_aux_buf(res, alloc_flags))
               }
      static struct pipe_resource *
   crocus_resource_create_for_buffer(struct pipe_screen *pscreen,
         {
      struct crocus_screen *screen = (struct crocus_screen *)pscreen;
            assert(templ->target == PIPE_BUFFER);
   assert(templ->height0 <= 1);
   assert(templ->depth0 <= 1);
   assert(templ->format == PIPE_FORMAT_NONE ||
            res->internal_format = templ->format;
                     res->bo = crocus_bo_alloc(screen->bufmgr, name, templ->width0);
   if (!res->bo) {
      crocus_resource_destroy(pscreen, &res->base.b);
                  }
      static struct pipe_resource *
   crocus_resource_create_with_modifiers(struct pipe_screen *pscreen,
                     {
      struct crocus_screen *screen = (struct crocus_screen *)pscreen;
   struct intel_device_info *devinfo = &screen->devinfo;
            if (!res)
            uint64_t modifier =
            if (modifier == DRM_FORMAT_MOD_INVALID && modifiers_count > 0) {
      fprintf(stderr, "Unsupported modifier, resource creation failed.\n");
               if (templ->usage == PIPE_USAGE_STAGING &&
      templ->bind == PIPE_BIND_DEPTH_STENCIL &&
   devinfo->ver < 6)
         const bool isl_surf_created_successfully =
         if (!isl_surf_created_successfully)
                     unsigned int flags = 0;
   if (templ->usage == PIPE_USAGE_STAGING)
            /* Scanout buffers need to be WC. */
   if (templ->bind & PIPE_BIND_SCANOUT)
            uint64_t aux_size = 0;
            if (!crocus_resource_configure_aux(screen, res, &aux_size,
                        /* Modifiers require the aux data to be in the same buffer as the main
   * surface, but we combine them even when a modifiers is not being used.
   */
   const uint64_t bo_size =
         uint32_t alignment = MAX2(4096, res->surf.alignment_B);
   res->bo = crocus_bo_alloc_tiled(screen->bufmgr, name, bo_size, alignment,
                  if (!res->bo)
            if (aux_size > 0) {
      res->aux.bo = res->bo;
   crocus_bo_reference(res->aux.bo);
   if (!crocus_resource_init_aux_buf(res, flags))
               if (templ->format == PIPE_FORMAT_S8_UINT && !(templ->usage == PIPE_USAGE_STAGING) &&
      devinfo->ver == 7 && (templ->bind & PIPE_BIND_SAMPLER_VIEW)) {
   struct pipe_resource templ_shadow = (struct pipe_resource) {
      .usage = 0,
   .bind = PIPE_BIND_SAMPLER_VIEW,
   .width0 = res->base.b.width0,
   .height0 = res->base.b.height0,
   .depth0 = res->base.b.depth0,
   .last_level = res->base.b.last_level,
   .nr_samples = res->base.b.nr_samples,
   .nr_storage_samples = res->base.b.nr_storage_samples,
   .array_size = res->base.b.array_size,
   .format = PIPE_FORMAT_R8_UINT,
      };
   res->shadow = (struct crocus_resource *)screen->base.resource_create(&screen->base, &templ_shadow);
                     fail:
      crocus_resource_destroy(pscreen, &res->base.b);
         }
      static struct pipe_resource *
   crocus_resource_create(struct pipe_screen *pscreen,
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
   crocus_resource_from_user_memory(struct pipe_screen *pscreen,
               {
      struct crocus_screen *screen = (struct crocus_screen *)pscreen;
   struct crocus_bufmgr *bufmgr = screen->bufmgr;
   struct crocus_resource *res = crocus_alloc_resource(pscreen, templ);
   if (!res)
                     res->internal_format = templ->format;
   res->bo = crocus_bo_create_userptr(bufmgr, "user",
         if (!res->bo) {
      free(res);
                           }
      static struct pipe_resource *
   crocus_resource_from_handle(struct pipe_screen *pscreen,
                     {
               struct crocus_screen *screen = (struct crocus_screen *)pscreen;
   struct crocus_bufmgr *bufmgr = screen->bufmgr;
            if (!res)
            switch (whandle->type) {
   case WINSYS_HANDLE_TYPE_FD:
      res->bo = crocus_bo_import_dmabuf(bufmgr, whandle->handle,
            case WINSYS_HANDLE_TYPE_SHARED:
      res->bo = crocus_bo_gem_create_from_name(bufmgr, "winsys image",
            default:
         }
   if (!res->bo)
            res->offset = whandle->offset;
            assert(whandle->plane < util_format_get_num_planes(whandle->format));
   const uint64_t modifier =
      whandle->modifier != DRM_FORMAT_MOD_INVALID ?
         UNUSED const bool isl_surf_created_successfully =
      crocus_resource_configure_main(screen, res, templ, modifier,
      assert(isl_surf_created_successfully);
   assert(res->bo->tiling_mode ==
            // XXX: create_ccs_buf_for_image?
   if (whandle->modifier == DRM_FORMAT_MOD_INVALID) {
      if (!crocus_resource_alloc_separate_aux(screen, res))
      } else {
                        fail:
      crocus_resource_destroy(pscreen, &res->base.b);
      }
      static struct pipe_resource *
   crocus_resource_from_memobj(struct pipe_screen *pscreen,
                     {
      struct crocus_screen *screen = (struct crocus_screen *)pscreen;
   struct crocus_memory_object *memobj = (struct crocus_memory_object *)pmemobj;
            if (!res)
            /* Disable Depth, and combined Depth+Stencil for now. */
   if (util_format_has_depth(util_format_description(templ->format)))
            if (templ->flags & PIPE_RESOURCE_FLAG_TEXTURING_MORE_LIKELY) {
      UNUSED const bool isl_surf_created_successfully =
                     res->bo = memobj->bo;
   res->offset = offset;
                        }
      static void
   crocus_flush_resource(struct pipe_context *ctx, struct pipe_resource *resource)
   {
      struct crocus_context *ice = (struct crocus_context *)ctx;
            /* Modifiers with compression are not supported. */
   assert(!res->mod_info ||
            crocus_resource_prepare_access(ice, res,
                  }
      static void
   crocus_resource_disable_aux_on_first_query(struct pipe_resource *resource,
         {
               /* Modifiers with compression are not supported. */
   assert(!res->mod_info ||
            /* Disable aux usage if explicit flush not set and this is the first time
   * we are dealing with this resource.
   */
   if ((!(usage & PIPE_HANDLE_USAGE_EXPLICIT_FLUSH) && res->aux.usage != 0) &&
      p_atomic_read(&resource->reference.count) == 1) {
         }
      static bool
   crocus_resource_get_param(struct pipe_screen *pscreen,
                           struct pipe_context *context,
   struct pipe_resource *resource,
   unsigned plane,
   unsigned layer,
   {
      struct crocus_screen *screen = (struct crocus_screen *)pscreen;
            /* Modifiers with compression are not supported. */
   assert(!res->mod_info ||
            bool result;
                              switch (param) {
   case PIPE_RESOURCE_PARAM_NPLANES: {
      unsigned count = 0;
   for (struct pipe_resource *cur = resource; cur; cur = cur->next)
         *value = count;
   return true;
      case PIPE_RESOURCE_PARAM_STRIDE:
      *value = res->surf.row_pitch_B;
      case PIPE_RESOURCE_PARAM_OFFSET:
      *value = 0;
      case PIPE_RESOURCE_PARAM_MODIFIER:
      *value = res->mod_info ? res->mod_info->modifier :
            case PIPE_RESOURCE_PARAM_HANDLE_TYPE_SHARED:
      result = crocus_bo_flink(bo, &handle) == 0;
   if (result)
            case PIPE_RESOURCE_PARAM_HANDLE_TYPE_KMS: {
      /* Because we share the same drm file across multiple crocus_screen, when
   * we export a GEM handle we must make sure it is valid in the DRM file
   * descriptor the caller is using (this is the FD given at screen
   * creation).
   */
   uint32_t handle;
   if (crocus_bo_export_gem_handle_for_device(bo, screen->winsys_fd, &handle))
         *value = handle;
      }
   case PIPE_RESOURCE_PARAM_HANDLE_TYPE_FD:
      result = crocus_bo_export_dmabuf(bo, (int *) &handle) == 0;
   if (result)
            default:
            }
      static bool
   crocus_resource_get_handle(struct pipe_screen *pscreen,
                           {
      struct crocus_screen *screen = (struct crocus_screen *) pscreen;
            /* Modifiers with compression are not supported. */
   assert(!res->mod_info ||
                     struct crocus_bo *bo;
   /* If this is a buffer, stride should be 0 - no need to special case */
   whandle->stride = res->surf.row_pitch_B;
   bo = res->bo;
   whandle->format = res->external_format;
   whandle->modifier =
      res->mod_info ? res->mod_info->modifier
      #ifndef NDEBUG
               if (res->aux.usage != allowed_usage) {
      enum isl_aux_state aux_state = crocus_resource_get_aux_state(res, 0, 0);
   assert(aux_state == ISL_AUX_STATE_RESOLVED ||
         #endif
         switch (whandle->type) {
   case WINSYS_HANDLE_TYPE_SHARED:
         case WINSYS_HANDLE_TYPE_KMS: {
      /* Because we share the same drm file across multiple crocus_screen, when
   * we export a GEM handle we must make sure it is valid in the DRM file
   * descriptor the caller is using (this is the FD given at screen
   * creation).
   */
   uint32_t handle;
   if (crocus_bo_export_gem_handle_for_device(bo, screen->winsys_fd, &handle))
         whandle->handle = handle;
      }
   case WINSYS_HANDLE_TYPE_FD:
                     }
      static bool
   resource_is_busy(struct crocus_context *ice,
         {
               for (int i = 0; i < ice->batch_count; i++)
               }
      void
   crocus_replace_buffer_storage(struct pipe_context *ctx,
                                 {
      struct crocus_screen *screen = (void *) ctx->screen;
   struct crocus_context *ice = (void *) ctx;
   struct crocus_resource *dst = (void *) p_dst;
                              /* Swap out the backing storage */
   crocus_bo_reference(src->bo);
            /* Rebind the buffer, replacing any state referring to the old BO's
   * address, and marking state dirty so it's reemitted.
   */
               }
      static void
   crocus_invalidate_resource(struct pipe_context *ctx,
         {
      struct crocus_screen *screen = (void *) ctx->screen;
   struct crocus_context *ice = (void *) ctx;
            if (resource->target != PIPE_BUFFER)
            /* If it's already invalidated, don't bother doing anything. */
   if (res->valid_buffer_range.start > res->valid_buffer_range.end)
            if (!resource_is_busy(ice, res)) {
      /* The resource is idle, so just mark that it contains no data and
   * keep using the same underlying buffer object.
   */
   util_range_set_empty(&res->valid_buffer_range);
                        /* We can't reallocate memory we didn't allocate in the first place. */
   if (res->bo->userptr)
            struct crocus_bo *old_bo = res->bo;
   struct crocus_bo *new_bo =
            if (!new_bo)
            /* Swap out the backing storage */
            /* Rebind the buffer, replacing any state referring to the old BO's
   * address, and marking state dirty so it's reemitted.
   */
                        }
      static void
   crocus_flush_staging_region(struct pipe_transfer *xfer,
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
               crocus_copy_region(map->blorp, map->batch, xfer->resource, xfer->level,
            }
      static void
   crocus_unmap_copy_region(struct crocus_transfer *map)
   {
                  }
      static void
   crocus_map_copy_region(struct crocus_transfer *map)
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
               if (xfer->resource->target == PIPE_BUFFER)
         else if (templ.array_size > 1)
         else
                     /* If we fail to create a staging resource, the caller will fallback
   * to mapping directly on the CPU.
   */
   if (!map->staging)
            if (templ.target != PIPE_BUFFER) {
      struct isl_surf *surf = &((struct crocus_resource *) map->staging)->surf;
   xfer->stride = isl_surf_get_row_pitch_B(surf);
               if (!(xfer->usage & PIPE_MAP_DISCARD_RANGE)) {
      crocus_copy_region(map->blorp, map->batch, map->staging, 0, extra, 0, 0,
         /* Ensure writes to the staging BO land before we map it below. */
   crocus_emit_pipe_control_flush(map->batch,
                                    if (crocus_batch_references(map->batch, staging_bo))
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
      void
   crocus_resource_get_image_offset(struct crocus_resource *res,
               {
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
   s8_offset(uint32_t stride, uint32_t x, uint32_t y, bool swizzled)
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
            if (swizzled) {
      /* adjust for bit6 swizzling */
   if (((byte_x / 8) % 2) == 1) {
      if (((byte_y / 8) % 2) == 0) {
         } else {
                           }
      static void
   crocus_unmap_s8(struct crocus_transfer *map)
   {
      struct pipe_transfer *xfer = &map->base.b;
   const struct pipe_box *box = &xfer->box;
   struct crocus_resource *res = (struct crocus_resource *) xfer->resource;
            if (xfer->usage & PIPE_MAP_WRITE) {
      uint8_t *untiled_s8_map = map->ptr;
   uint8_t *tiled_s8_map =
            for (int s = 0; s < box->depth; s++) {
                     for (uint32_t y = 0; y < box->height; y++) {
      for (uint32_t x = 0; x < box->width; x++) {
      ptrdiff_t offset = s8_offset(surf->row_pitch_B,
                     tiled_s8_map[offset] =
                           }
      static void
   crocus_map_s8(struct crocus_transfer *map)
   {
      struct pipe_transfer *xfer = &map->base.b;
   const struct pipe_box *box = &xfer->box;
   struct crocus_resource *res = (struct crocus_resource *) xfer->resource;
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
   if (!(xfer->usage & PIPE_MAP_DISCARD_RANGE)) {
      uint8_t *untiled_s8_map = map->ptr;
   uint8_t *tiled_s8_map =
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
   crocus_unmap_tiled_memcpy(struct crocus_transfer *map)
   {
      struct pipe_transfer *xfer = &map->base.b;
   const struct pipe_box *box = &xfer->box;
   struct crocus_resource *res = (struct crocus_resource *) xfer->resource;
            if (xfer->usage & PIPE_MAP_WRITE) {
      char *dst =
            for (int s = 0; s < box->depth; s++) {
                              isl_memcpy_linear_to_tiled(x1, x2, y1, y2, dst, ptr,
                     }
   os_free_aligned(map->buffer);
      }
      static void
   crocus_map_tiled_memcpy(struct crocus_transfer *map)
   {
      struct pipe_transfer *xfer = &map->base.b;
   const struct pipe_box *box = &xfer->box;
   struct crocus_resource *res = (struct crocus_resource *) xfer->resource;
            xfer->stride = ALIGN(surf->row_pitch_B, 16);
            unsigned x1, x2, y1, y2;
            /* The tiling and detiling functions require that the linear buffer has
   * a 16-byte alignment (that is, its `x0` is 16-byte aligned).  Here we
   * over-allocate the linear buffer to get the proper alignment.
   */
   map->buffer =
         assert(map->buffer);
            if (!(xfer->usage & PIPE_MAP_DISCARD_RANGE)) {
      char *src =
            for (int s = 0; s < box->depth; s++) {
                                    isl_memcpy_tiled_to_linear(x1, x2, y1, y2, ptr, src, xfer->stride,
            #if defined(USE_SSE41)
         #endif
                              }
      static void
   crocus_map_direct(struct crocus_transfer *map)
   {
      struct pipe_transfer *xfer = &map->base.b;
   struct pipe_box *box = &xfer->box;
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
   can_promote_to_async(const struct crocus_resource *res,
               {
      /* If we're writing to a section of the buffer that hasn't even been
   * initialized with useful data, then we can safely promote this write
   * to be unsynchronized.  This helps the common pattern of appending data.
   */
   return res->base.b.target == PIPE_BUFFER && (usage & PIPE_MAP_WRITE) &&
         !(usage & TC_TRANSFER_MAP_NO_INFER_UNSYNCHRONIZED) &&
      }
      static void *
   crocus_transfer_map(struct pipe_context *ctx,
                     struct pipe_resource *resource,
   unsigned level,
   {
      struct crocus_context *ice = (struct crocus_context *)ctx;
   struct crocus_resource *res = (struct crocus_resource *)resource;
   struct isl_surf *surf = &res->surf;
            if (usage & PIPE_MAP_DISCARD_WHOLE_RESOURCE) {
      /* Replace the backing storage with a fresh buffer for non-async maps */
   if (!(usage & (PIPE_MAP_UNSYNCHRONIZED |
                  /* If we can discard the whole resource, we can discard the range. */
               if (!(usage & PIPE_MAP_UNSYNCHRONIZED) &&
      can_promote_to_async(res, box, usage)) {
                        if (!(usage & PIPE_MAP_UNSYNCHRONIZED)) {
      map_would_stall = resource_is_busy(ice, res) ||
               if (map_would_stall && (usage & PIPE_MAP_DONTBLOCK) &&
                     if (surf->tiling != ISL_TILING_LINEAR &&
      (usage & PIPE_MAP_DIRECTLY))
         struct crocus_transfer *map;
   if (usage & TC_TRANSFER_MAP_THREADED_UNSYNC)
         else
                     if (!map)
                     map->has_swizzling = screen->devinfo.has_bit6_swizzle;
   pipe_resource_reference(&xfer->resource, resource);
   xfer->level = level;
   xfer->usage = usage;
   xfer->box = *box;
            map->dest_had_defined_contents =
      util_ranges_intersect(&res->valid_buffer_range, box->x,
         if (usage & PIPE_MAP_WRITE)
            /* Avoid using GPU copies for persistent/coherent buffers, as the idea
   * there is to access them simultaneously on the CPU & GPU.  This also
   * avoids trying to use GPU copies for our u_upload_mgr buffers which
   * contain state we're constructing for a GPU draw call, which would
   * kill us with infinite stack recursion.
   */
   bool no_gpu = usage & (PIPE_MAP_PERSISTENT |
                  /* GPU copies are not useful for buffer reads.  Instead of stalling to
   * read from the original buffer, we'd simply copy it to a temporary...
   * then stall (a bit longer) to read from that buffer.
   *
   * Images are less clear-cut.  Color resolves are destructive, removing
   * the underlying compression, so we'd rather blit the data to a linear
   * temporary and map that, to avoid the resolve.  (It might be better to
   * a tiled temporary and use the tiled_memcpy paths...)
   */
   if (!(usage & PIPE_MAP_DISCARD_RANGE) &&
      !crocus_has_invalid_primary(res, level, 1, box->z, box->depth))
         const struct isl_format_layout *fmtl = isl_format_get_layout(surf->format);
   if (fmtl->txc == ISL_TXC_ASTC)
            if (map_would_stall && !no_gpu) {
      /* If we need a synchronous mapping and the resource is busy, or needs
   * resolving, we copy to/from a linear temporary buffer using the GPU.
   */
   map->batch = &ice->batches[CROCUS_BATCH_RENDER];
   map->blorp = &ice->blorp;
               /* If we've requested a direct mapping, or crocus_map_copy_region failed
   * to create a staging resource, then map it directly on the CPU.
   */
   if (!map->ptr) {
      if (resource->target != PIPE_BUFFER) {
      crocus_resource_access_raw(ice, res,
                     if (!(usage & PIPE_MAP_UNSYNCHRONIZED)) {
      for (int i = 0; i < ice->batch_count; i++) {
      if (crocus_batch_references(&ice->batches[i], res->bo))
                  if (surf->tiling == ISL_TILING_W) {
      /* TODO: Teach crocus_map_tiled_memcpy about W-tiling... */
      } else if (surf->tiling != ISL_TILING_LINEAR && screen->devinfo.ver > 4) {
         } else {
                        }
      static void
   crocus_transfer_flush_region(struct pipe_context *ctx,
               {
      struct crocus_context *ice = (struct crocus_context *)ctx;
   struct crocus_resource *res = (struct crocus_resource *) xfer->resource;
            if (map->staging)
                     if (res->base.b.target == PIPE_BUFFER) {
      if (map->staging)
            if (map->dest_had_defined_contents)
                        if (history_flush & ~PIPE_CONTROL_CS_STALL) {
      for (int i = 0; i < ice->batch_count; i++) {
               if (!batch->command.bo)
         if (batch->contains_draw || batch->cache.render->entries) {
      crocus_batch_maybe_flush(batch, 24);
   crocus_emit_pipe_control_flush(batch,
                           /* Make sure we flag constants dirty even if there's no need to emit
   * any PIPE_CONTROLs to a batch.
   */
      }
      static void
   crocus_transfer_unmap(struct pipe_context *ctx, struct pipe_transfer *xfer)
   {
      struct crocus_context *ice = (struct crocus_context *)ctx;
            if (!(xfer->usage & (PIPE_MAP_FLUSH_EXPLICIT |
            struct pipe_box flush_box = {
      .x = 0, .y = 0, .z = 0,
   .width  = xfer->box.width,
   .height = xfer->box.height,
      };
               if (map->unmap)
            pipe_resource_reference(&xfer->resource, NULL);
   /* transfer_unmap is always called from the driver thread, so we have to
   * use transfer_pool, not transfer_pool_unsync.  Freeing an object into a
   * different pool is allowed, however.
   */
      }
      /**
   * Mark state dirty that needs to be re-emitted when a resource is written.
   */
   void
   crocus_dirty_for_history(struct crocus_context *ice,
         {
               if (res->bind_history & PIPE_BIND_CONSTANT_BUFFER) {
                     }
      /**
   * Produce a set of PIPE_CONTROL bits which ensure data written to a
   * resource becomes visible, and any stale read cache data is invalidated.
   */
   uint32_t
   crocus_flush_bits_for_history(struct crocus_resource *res)
   {
               if (res->bind_history & PIPE_BIND_CONSTANT_BUFFER) {
      flush |= PIPE_CONTROL_CONST_CACHE_INVALIDATE |
               if (res->bind_history & PIPE_BIND_SAMPLER_VIEW)
            if (res->bind_history & (PIPE_BIND_VERTEX_BUFFER | PIPE_BIND_INDEX_BUFFER))
            if (res->bind_history & (PIPE_BIND_SHADER_BUFFER | PIPE_BIND_SHADER_IMAGE))
               }
      void
   crocus_flush_and_dirty_for_history(struct crocus_context *ice,
                           {
      if (res->base.b.target != PIPE_BUFFER)
                                 }
      bool
   crocus_resource_set_clear_color(struct crocus_context *ice,
               {
      if (memcmp(&res->aux.clear_color, &color, sizeof(color)) != 0) {
      res->aux.clear_color = color;
                  }
      union isl_color_value
   crocus_resource_get_clear_color(const struct crocus_resource *res)
   {
                  }
      static enum pipe_format
   crocus_resource_get_internal_format(struct pipe_resource *p_res)
   {
      struct crocus_resource *res = (void *) p_res;
      }
      static const struct u_transfer_vtbl transfer_vtbl = {
      .resource_create       = crocus_resource_create,
   .resource_destroy      = crocus_resource_destroy,
   .transfer_map          = crocus_transfer_map,
   .transfer_unmap        = crocus_transfer_unmap,
   .transfer_flush_region = crocus_transfer_flush_region,
   .get_internal_format   = crocus_resource_get_internal_format,
   .set_stencil           = crocus_resource_set_separate_stencil,
      };
      static bool
   crocus_is_dmabuf_modifier_supported(struct pipe_screen *pscreen,
               {
      struct crocus_screen *screen = (void *) pscreen;
            if (modifier_is_supported(devinfo, pfmt, 0, modifier)) {
      if (external_only)
                           }
      static unsigned int
   crocus_get_dmabuf_modifier_planes(struct pipe_screen *pscreen, uint64_t modifier,
         {
         }
      static struct pipe_memory_object *
   crocus_memobj_create_from_handle(struct pipe_screen *pscreen,
               {
      struct crocus_screen *screen = (struct crocus_screen *)pscreen;
   struct crocus_memory_object *memobj = CALLOC_STRUCT(crocus_memory_object);
   struct crocus_bo *bo;
            if (!memobj)
            switch (whandle->type) {
   case WINSYS_HANDLE_TYPE_SHARED:
      bo = crocus_bo_gem_create_from_name(screen->bufmgr, "winsys image",
            case WINSYS_HANDLE_TYPE_FD:
      mod_inf = isl_drm_modifier_get_info(whandle->modifier);
   if (mod_inf) {
      bo = crocus_bo_import_dmabuf(screen->bufmgr, whandle->handle,
      } else {
      /* If we can't get information about the tiling from the
   * kernel we ignore it. We are going to set it when we
   * create the resource.
   */
   bo = crocus_bo_import_dmabuf_no_mods(screen->bufmgr,
                  default:
                  if (!bo) {
      free(memobj);
               memobj->b.dedicated = dedicated;
   memobj->bo = bo;
   memobj->format = whandle->format;
               }
      static void
   crocus_memobj_destroy(struct pipe_screen *pscreen,
         {
               crocus_bo_unreference(memobj->bo);
      }
      void
   crocus_init_screen_resource_functions(struct pipe_screen *pscreen)
   {
      struct crocus_screen *screen = (void *) pscreen;
   pscreen->query_dmabuf_modifiers = crocus_query_dmabuf_modifiers;
   pscreen->is_dmabuf_modifier_supported = crocus_is_dmabuf_modifier_supported;
   pscreen->get_dmabuf_modifier_planes = crocus_get_dmabuf_modifier_planes;
   pscreen->resource_create_with_modifiers =
         pscreen->resource_create = u_transfer_helper_resource_create;
   pscreen->resource_from_user_memory = crocus_resource_from_user_memory;
   pscreen->resource_from_handle = crocus_resource_from_handle;
   pscreen->resource_from_memobj = crocus_resource_from_memobj;
   pscreen->resource_get_handle = crocus_resource_get_handle;
   pscreen->resource_get_param = crocus_resource_get_param;
   pscreen->resource_destroy = u_transfer_helper_resource_destroy;
   pscreen->memobj_create_from_handle = crocus_memobj_create_from_handle;
            enum u_transfer_helper_flags transfer_flags = U_TRANSFER_HELPER_MSAA_MAP;
   if (screen->devinfo.ver >= 6) {
      transfer_flags |= U_TRANSFER_HELPER_SEPARATE_Z32S8 |
               pscreen->transfer_helper =
      }
      void
   crocus_init_resource_functions(struct pipe_context *ctx)
   {
      ctx->flush_resource = crocus_flush_resource;
   ctx->clear_buffer = u_default_clear_buffer;
   ctx->invalidate_resource = crocus_invalidate_resource;
   ctx->buffer_map = u_transfer_helper_transfer_map;
   ctx->texture_map = u_transfer_helper_transfer_map;
   ctx->transfer_flush_region = u_transfer_helper_transfer_flush_region;
   ctx->buffer_unmap = u_transfer_helper_transfer_unmap;
   ctx->texture_unmap = u_transfer_helper_transfer_unmap;
   ctx->buffer_subdata = u_default_buffer_subdata;
      }
