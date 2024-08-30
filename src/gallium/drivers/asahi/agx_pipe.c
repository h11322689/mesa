   /*
   * Copyright 2010 Red Hat Inc.
   * Copyright 2014-2017 Broadcom
   * Copyright 2019-2020 Collabora, Ltd.
   * Copyright 2006 VMware, Inc.
   * SPDX-License-Identifier: MIT
   */
   #include <errno.h>
   #include <stdio.h>
   #include <xf86drm.h>
   #include "asahi/compiler/agx_compile.h"
   #include "asahi/layout/layout.h"
   #include "asahi/lib/agx_formats.h"
   #include "asahi/lib/decode.h"
   #include "drm-uapi/drm_fourcc.h"
   #include "frontend/winsys_handle.h"
   #include "gallium/auxiliary/renderonly/renderonly.h"
   #include "gallium/auxiliary/util/u_debug_cb.h"
   #include "gallium/auxiliary/util/u_framebuffer.h"
   #include "gallium/auxiliary/util/u_sample_positions.h"
   #include "gallium/auxiliary/util/u_surface.h"
   #include "gallium/auxiliary/util/u_transfer.h"
   #include "gallium/auxiliary/util/u_transfer_helper.h"
   #include "pipe/p_context.h"
   #include "pipe/p_defines.h"
   #include "pipe/p_screen.h"
   #include "pipe/p_state.h"
   #include "util/format/u_format.h"
   #include "util/half_float.h"
   #include "util/u_drm.h"
   #include "util/u_gen_mipmap.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "util/u_screen.h"
   #include "util/u_upload_mgr.h"
   #include "util/xmlconfig.h"
   #include "agx_device.h"
   #include "agx_disk_cache.h"
   #include "agx_fence.h"
   #include "agx_public.h"
   #include "agx_state.h"
   #include "agx_tilebuffer.h"
      /* Fake values, pending UAPI upstreaming */
   #ifndef DRM_FORMAT_MOD_APPLE_TWIDDLED
   #define DRM_FORMAT_MOD_APPLE_TWIDDLED (2)
   #endif
   #ifndef DRM_FORMAT_MOD_APPLE_TWIDDLED_COMPRESSED
   #define DRM_FORMAT_MOD_APPLE_TWIDDLED_COMPRESSED (3)
   #endif
      /* clang-format off */
   static const struct debug_named_value agx_debug_options[] = {
      {"trace",     AGX_DBG_TRACE,    "Trace the command stream"},
   {"deqp",      AGX_DBG_DEQP,     "Hacks for dEQP"},
   {"no16",      AGX_DBG_NO16,     "Disable 16-bit support"},
      #ifndef NDEBUG
         #endif
      {"precompile",AGX_DBG_PRECOMPILE,"Precompile shaders for shader-db"},
   {"nocompress",AGX_DBG_NOCOMPRESS,"Disable lossless compression"},
   {"nocluster", AGX_DBG_NOCLUSTER,"Disable vertex clustering"},
   {"sync",      AGX_DBG_SYNC,     "Synchronously wait for all submissions"},
   {"stats",     AGX_DBG_STATS,    "Show command execution statistics"},
   {"resource",  AGX_DBG_RESOURCE, "Log resource operations"},
   {"batch",     AGX_DBG_BATCH,    "Log batches"},
   {"nowc",      AGX_DBG_NOWC,     "Disable write-combining"},
   {"synctvb",   AGX_DBG_SYNCTVB,  "Synchronous TVB growth"},
   {"smalltile", AGX_DBG_SMALLTILE,"Force 16x16 tiles"},
   {"nomsaa",    AGX_DBG_NOMSAA,   "Force disable MSAA"},
   {"noshadow",  AGX_DBG_NOSHADOW, "Force disable resource shadowing"},
      };
   /* clang-format on */
      uint64_t agx_best_modifiers[] = {
      DRM_FORMAT_MOD_APPLE_TWIDDLED_COMPRESSED,
   DRM_FORMAT_MOD_APPLE_TWIDDLED,
      };
      /* These limits are arbitrarily chosen and subject to change as
   * we discover more workloads with heavy shadowing.
   *
   * Maximum size of a shadowed object in bytes.
   * Hint: 1024x1024xRGBA8 = 4 MiB. Go higher for compression.
   */
   #define MAX_SHADOW_BYTES (6 * 1024 * 1024)
      /* Maximum cumulative size to shadow an object before we flush.
   * Allows shadowing a 4MiB + meta object 8 times with the logic
   * below (+1 shadow offset implied).
   */
   #define MAX_TOTAL_SHADOW_BYTES (32 * 1024 * 1024)
      void agx_init_state_functions(struct pipe_context *ctx);
      /*
   * resource
   */
      static enum ail_tiling
   ail_modifier_to_tiling(uint64_t modifier)
   {
      switch (modifier) {
   case DRM_FORMAT_MOD_LINEAR:
         case DRM_FORMAT_MOD_APPLE_TWIDDLED:
         case DRM_FORMAT_MOD_APPLE_TWIDDLED_COMPRESSED:
         default:
            }
      const static char *s_tiling[] = {
      [AIL_TILING_LINEAR] = "LINR",
   [AIL_TILING_TWIDDLED] = "TWID",
      };
      #define rsrc_debug(res, ...)                                                   \
      do {                                                                        \
      if (agx_device((res)->base.screen)->debug & AGX_DBG_RESOURCE)            \
            static void
   agx_resource_debug(struct agx_resource *res, const char *msg)
   {
      if (!(agx_device(res->base.screen)->debug & AGX_DBG_RESOURCE))
            int ino = -1;
   if (res->bo->prime_fd >= 0) {
      struct stat sb;
   if (!fstat(res->bo->prime_fd, &sb))
               agx_msg(
      "%s%s %dx%dx%d %dL %d/%dM %dS M:%llx %s %s%s S:0x%llx LS:0x%llx CS:0x%llx "
   "Base=0x%llx Size=0x%llx Meta=0x%llx/0x%llx (%s) %s%s%s%s%s%sfd:%d(%d) @ %p\n",
   msg ?: "", util_format_short_name(res->base.format), res->base.width0,
   res->base.height0, res->base.depth0, res->base.array_size,
   res->base.last_level, res->layout.levels, res->layout.sample_count_sa,
   (long long)res->modifier, s_tiling[res->layout.tiling],
   res->layout.mipmapped_z ? "MZ " : "",
   res->layout.page_aligned_layers ? "PL " : "",
   (long long)res->layout.linear_stride_B,
   (long long)res->layout.layer_stride_B,
   (long long)res->layout.compression_layer_stride_B,
   (long long)res->bo->ptr.gpu, (long long)res->layout.size_B,
   res->layout.metadata_offset_B
      ? ((long long)res->bo->ptr.gpu + res->layout.metadata_offset_B)
      (long long)res->layout.metadata_offset_B, res->bo->label,
   res->bo->flags & AGX_BO_SHARED ? "SH " : "",
   res->bo->flags & AGX_BO_LOW_VA ? "LO " : "",
   res->bo->flags & AGX_BO_EXEC ? "EX " : "",
   res->bo->flags & AGX_BO_WRITEBACK ? "WB " : "",
   res->bo->flags & AGX_BO_SHAREABLE ? "SA " : "",
   res->bo->flags & AGX_BO_READONLY ? "RO " : "", res->bo->prime_fd, ino,
   }
      static void
   agx_resource_setup(struct agx_device *dev, struct agx_resource *nresource)
   {
               nresource->layout = (struct ail_layout){
      .tiling = ail_modifier_to_tiling(nresource->modifier),
   .mipmapped_z = templ->target == PIPE_TEXTURE_3D,
   .format = templ->format,
   .width_px = templ->width0,
   .height_px = templ->height0,
   .depth_px = templ->depth0 * templ->array_size,
   .sample_count_sa = MAX2(templ->nr_samples, 1),
   .levels = templ->last_level + 1,
            /* Ostensibly this should be based on the bind, but Gallium bind flags are
   * notoriously unreliable. The only cost of setting this excessively is a
   * bit of extra memory use for layered textures, which isn't worth trying
   * to optimize.
   */
         }
      static struct pipe_resource *
   agx_resource_from_handle(struct pipe_screen *pscreen,
               {
      struct agx_device *dev = agx_device(pscreen);
   struct agx_resource *rsc;
                     rsc = CALLOC_STRUCT(agx_resource);
   if (!rsc)
            rsc->modifier = whandle->modifier == DRM_FORMAT_MOD_INVALID
                  /* We need strides to be aligned. ail asserts this, but we want to fail
   * gracefully so the app can handle the error.
   */
   if (rsc->modifier == DRM_FORMAT_MOD_LINEAR && (whandle->stride % 16) != 0) {
      FREE(rsc);
                                 pipe_reference_init(&prsc->reference, 1);
            rsc->bo = agx_bo_import(dev, whandle->handle);
   /* Sometimes an import can fail e.g. on an invalid buffer fd, out of
   * memory space to mmap it etc.
   */
   if (!rsc->bo) {
      FREE(rsc);
                        if (rsc->layout.tiling == AIL_TILING_LINEAR) {
         } else if (whandle->stride != ail_get_wsi_stride_B(&rsc->layout, 0)) {
      FREE(rsc);
                                 if (prsc->target == PIPE_BUFFER) {
      assert(rsc->layout.tiling == AIL_TILING_LINEAR);
                           }
      static bool
   agx_resource_get_handle(struct pipe_screen *pscreen, struct pipe_context *ctx,
               {
      struct agx_device *dev = agx_device(pscreen);
            /* Even though asahi doesn't support multi-planar formats, we
   * can get here through GBM, which does. Walk the list of planes
   * to find the right one.
   */
   for (int i = 0; i < handle->plane; i++) {
      cur = cur->next;
   if (!cur)
                        if (handle->type == WINSYS_HANDLE_TYPE_KMS && dev->ro) {
               if (!rsrc->scanout && dev->ro && (rsrc->base.bind & PIPE_BIND_SCANOUT)) {
      rsrc->scanout =
               if (!rsrc->scanout)
               } else if (handle->type == WINSYS_HANDLE_TYPE_KMS) {
                  } else if (handle->type == WINSYS_HANDLE_TYPE_FD) {
               if (fd < 0)
            handle->handle = fd;
   if (dev->debug & AGX_DBG_RESOURCE) {
      struct stat sb;
   fstat(rsrc->bo->prime_fd, &sb);
         } else {
      /* Other handle types not supported */
               handle->stride = ail_get_wsi_stride_B(&rsrc->layout, 0);
   handle->size = rsrc->layout.size_B;
   handle->offset = rsrc->layout.level_offsets_B[0];
   handle->format = rsrc->layout.format;
               }
      static bool
   agx_resource_get_param(struct pipe_screen *pscreen, struct pipe_context *pctx,
                           {
      struct agx_resource *rsrc = (struct agx_resource *)prsc;
   struct pipe_resource *cur;
            switch (param) {
   case PIPE_RESOURCE_PARAM_STRIDE:
      *value = ail_get_wsi_stride_B(&rsrc->layout, level);
      case PIPE_RESOURCE_PARAM_OFFSET:
      *value = rsrc->layout.level_offsets_B[level];
      case PIPE_RESOURCE_PARAM_MODIFIER:
      *value = rsrc->modifier;
      case PIPE_RESOURCE_PARAM_NPLANES:
      /* We don't support multi-planar formats, but we should still handle
   * this case for GBM shared resources.
   */
   for (count = 0, cur = prsc; cur; cur = cur->next)
         *value = count;
      default:
            }
      static bool
   agx_is_2d(enum pipe_texture_target target)
   {
         }
      static bool
   agx_linear_allowed(const struct agx_resource *pres)
   {
      /* Mipmapping not allowed with linear */
   if (pres->base.last_level != 0)
            /* Depth/stencil buffers must not be linear */
   if (pres->base.bind & PIPE_BIND_DEPTH_STENCIL)
            /* Multisampling not allowed with linear */
   if (pres->base.nr_samples > 1)
            /* Block compression not allowed with linear */
   if (util_format_is_compressed(pres->base.format))
            switch (pres->base.target) {
   /* Buffers are always linear, even with image atomics */
            /* Linear textures require specifying their strides explicitly, which only
   * works for 2D textures. Rectangle textures are a special case of 2D.
   *
   * 1D textures only exist in GLES and are lowered to 2D to bypass hardware
   * limitations.
   *
   * However, we don't want to support this case in the image atomic
   * implementation, so linear shader images are specially forbidden.
   */
   case PIPE_TEXTURE_1D:
   case PIPE_TEXTURE_1D_ARRAY:
   case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_2D_ARRAY:
   case PIPE_TEXTURE_RECT:
      if (pres->base.bind & PIPE_BIND_SHADER_IMAGE)
                  /* No other texture type can specify a stride */
   default:
                     }
      static bool
   agx_twiddled_allowed(const struct agx_resource *pres)
   {
      /* Certain binds force linear */
   if (pres->base.bind & (PIPE_BIND_DISPLAY_TARGET | PIPE_BIND_LINEAR))
            /* Buffers must be linear */
   if (pres->base.target == PIPE_BUFFER)
            /* Anything else may be twiddled */
      }
      static bool
   agx_compression_allowed(const struct agx_resource *pres)
   {
      /* Allow disabling compression for debugging */
   if (agx_device(pres->base.screen)->debug & AGX_DBG_NOCOMPRESS) {
      rsrc_debug(pres, "No compression: disabled\n");
               /* Limited to renderable */
   if (pres->base.bind &
      ~(PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET |
         rsrc_debug(pres, "No compression: not renderable\n");
               /* We use the PBE for compression via staging blits, so we can only compress
   * renderable formats. As framebuffer compression, other formats don't make a
   * ton of sense to compress anyway.
   */
   if (!agx_pixel_format[pres->base.format].renderable &&
      !util_format_is_depth_or_stencil(pres->base.format)) {
   rsrc_debug(pres, "No compression: format not renderable\n");
               /* Lossy-compressed texture formats cannot be compressed */
   assert(!util_format_is_compressed(pres->base.format) &&
            if (!ail_can_compress(pres->base.width0, pres->base.height0,
            rsrc_debug(pres, "No compression: too small\n");
                  }
      static uint64_t
   agx_select_modifier_from_list(const struct agx_resource *pres,
         {
      if (agx_twiddled_allowed(pres) && agx_compression_allowed(pres) &&
      drm_find_modifier(DRM_FORMAT_MOD_APPLE_TWIDDLED_COMPRESSED, modifiers,
               if (agx_twiddled_allowed(pres) &&
      drm_find_modifier(DRM_FORMAT_MOD_APPLE_TWIDDLED, modifiers, count))
         if (agx_linear_allowed(pres) &&
      drm_find_modifier(DRM_FORMAT_MOD_LINEAR, modifiers, count))
         /* We didn't find anything */
      }
      static uint64_t
   agx_select_best_modifier(const struct agx_resource *pres)
   {
      /* Prefer linear for staging resources, which should be as fast as possible
   * to write from the CPU.
   */
   if (agx_linear_allowed(pres) && pres->base.usage == PIPE_USAGE_STAGING)
            /* For SCANOUT or SHARED resources with no explicit modifier selection, force
   * linear since we cannot expect consumers to correctly pass through the
   * modifier (unless linear is not allowed at all).
   */
   if (agx_linear_allowed(pres) &&
      pres->base.bind & (PIPE_BIND_SCANOUT | PIPE_BIND_SHARED)) {
               if (agx_twiddled_allowed(pres)) {
      if (agx_compression_allowed(pres))
         else
               if (agx_linear_allowed(pres))
         else
      }
      static struct pipe_resource *
   agx_resource_create_with_modifiers(struct pipe_screen *screen,
               {
      struct agx_device *dev = agx_device(screen);
            nresource = CALLOC_STRUCT(agx_resource);
   if (!nresource)
            nresource->base = *templ;
            if (modifiers) {
      nresource->modifier =
      } else {
                  /* There may not be a matching modifier, bail if so */
   if (nresource->modifier == DRM_FORMAT_MOD_INVALID) {
      free(nresource);
               /* If there's only 1 layer and there's no compression, there's no harm in
   * inferring the shader image flag. Do so to avoid reallocation in case the
   * resource is later used as an image.
   */
   if (nresource->modifier != DRM_FORMAT_MOD_APPLE_TWIDDLED_COMPRESSED &&
                                    assert(templ->format != PIPE_FORMAT_Z24X8_UNORM &&
                                             if (templ->target == PIPE_BUFFER) {
      assert(nresource->layout.tiling == AIL_TILING_LINEAR);
               /* Guess a label based on the bind */
            const char *label = (bind & PIPE_BIND_INDEX_BUFFER)     ? "Index buffer"
                     : (bind & PIPE_BIND_SCANOUT)        ? "Scanout"
   : (bind & PIPE_BIND_DISPLAY_TARGET) ? "Display target"
   : (bind & PIPE_BIND_SHARED)         ? "Shared resource"
   : (bind & PIPE_BIND_RENDER_TARGET)  ? "Render target"
   : (bind & PIPE_BIND_DEPTH_STENCIL)
         : (bind & PIPE_BIND_SAMPLER_VIEW)    ? "Texture"
   : (bind & PIPE_BIND_VERTEX_BUFFER)   ? "Vertex buffer"
   : (bind & PIPE_BIND_CONSTANT_BUFFER) ? "Constant buffer"
                     /* Default to write-combine resources, but use writeback if that is expected
   * to be beneficial.
   */
   if (nresource->base.usage == PIPE_USAGE_STAGING ||
                           /* Allow disabling write-combine to debug performance issues */
   if (dev->debug & AGX_DBG_NOWC) {
                  /* Create buffers that might be shared with the SHAREABLE flag */
   if (bind & (PIPE_BIND_SCANOUT | PIPE_BIND_DISPLAY_TARGET | PIPE_BIND_SHARED))
            nresource->bo =
            if (!nresource->bo) {
      FREE(nresource);
               agx_resource_debug(nresource, "New: ");
      }
      static struct pipe_resource *
   agx_resource_create(struct pipe_screen *screen,
         {
         }
      static void
   agx_resource_destroy(struct pipe_screen *screen, struct pipe_resource *prsrc)
   {
      struct agx_resource *rsrc = (struct agx_resource *)prsrc;
                     if (prsrc->target == PIPE_BUFFER)
            if (rsrc->scanout)
            agx_bo_unreference(rsrc->bo);
      }
      void
   agx_batch_track_image(struct agx_batch *batch, struct pipe_image_view *image)
   {
               if (image->shader_access & PIPE_IMAGE_ACCESS_WRITE) {
               bool is_buffer = rsrc->base.target == PIPE_BUFFER;
   unsigned level = is_buffer ? 0 : image->u.tex.level;
            if (is_buffer) {
      util_range_add(&rsrc->base, &rsrc->valid_buffer_range, 0,
         } else {
            }
      /*
   * transfer
   */
      static void
   agx_transfer_flush_region(struct pipe_context *pipe,
               {
   }
      /* Reallocate the backing buffer of a resource, returns true if successful */
   static bool
   agx_shadow(struct agx_context *ctx, struct agx_resource *rsrc, bool needs_copy)
   {
      struct agx_device *dev = agx_device(ctx->base.screen);
   struct agx_bo *old = rsrc->bo;
   size_t size = rsrc->layout.size_B;
            if (dev->debug & AGX_DBG_NOSHADOW)
            /* If a resource is (or could be) shared, shadowing would desync across
   * processes. (It's also not what this path is for.)
   */
   if (flags & (AGX_BO_SHARED | AGX_BO_SHAREABLE))
            /* Do not shadow resources that are too large */
   if (size > MAX_SHADOW_BYTES)
            /* Do not shadow resources too much */
   if (rsrc->shadowed_bytes >= MAX_TOTAL_SHADOW_BYTES)
                     /* If we need to copy, we reallocate the resource with cached-coherent
   * memory. This is a heuristic: it assumes that if the app needs a shadows
   * (with a copy) now, it will again need to shadow-and-copy the same resource
   * in the future. This accelerates the later copies, since otherwise the copy
   * involves reading uncached memory.
   */
   if (needs_copy)
                     /* If allocation failed, we can fallback on a flush gracefully*/
   if (new_ == NULL)
            if (needs_copy) {
      perf_debug_ctx(ctx, "Shadowing %zu bytes on the CPU (%s)", size,
                              /* Swap the pointers, dropping a reference */
   agx_bo_unreference(rsrc->bo);
            /* Reemit descriptors using this resource */
   agx_dirty_all(ctx);
      }
      /*
   * Perform the required synchronization before a transfer_map operation can
   * complete. This may require syncing batches.
   */
   static void
   agx_prepare_for_map(struct agx_context *ctx, struct agx_resource *rsrc,
                     {
      /* Upgrade DISCARD_RANGE to WHOLE_RESOURCE if the whole resource is
   * being mapped.
   */
   if ((usage & PIPE_MAP_DISCARD_RANGE) &&
      !(rsrc->base.flags & PIPE_RESOURCE_FLAG_MAP_PERSISTENT) &&
   rsrc->base.last_level == 0 &&
   util_texrange_covers_whole_level(&rsrc->base, 0, box->x, box->y, box->z,
                        /* Shadowing doesn't work separate stencil or shared resources */
   if (rsrc->separate_stencil || (rsrc->bo->flags & AGX_BO_SHARED))
            /* If the access is unsynchronized, there's nothing to do */
   if (usage & PIPE_MAP_UNSYNCHRONIZED)
            /* Everything after this needs the context, which is not safe for
   * unsynchronized transfers when we claim
   * PIPE_CAP_MAP_UNSYNCHRONIZED_THREAD_SAFE.
   */
            /* Both writing and reading need writers synced */
            /* Additionally, writing needs readers synced */
   if (!(usage & PIPE_MAP_WRITE))
            /* If the range being written is uninitialized, we do not need to sync. */
   if (rsrc->base.target == PIPE_BUFFER && !(rsrc->bo->flags & AGX_BO_SHARED) &&
      !util_ranges_intersect(&rsrc->valid_buffer_range, box->x,
               /* If there are no readers, we're done. We check at the start to
   * avoid expensive shadowing paths or duplicated checks in this hapyp path.
   */
   if (!agx_any_batch_uses_resource(ctx, rsrc)) {
      rsrc->shadowed_bytes = 0;
               /* There are readers. Try to shadow the resource to avoid a sync */
   if (!(rsrc->base.flags & PIPE_RESOURCE_FLAG_MAP_PERSISTENT) &&
      agx_shadow(ctx, rsrc, !(usage & PIPE_MAP_DISCARD_WHOLE_RESOURCE)))
         /* Otherwise, we need to sync */
               }
      /*
   * Return a colour-renderable format compatible with a depth/stencil format, to
   * be used as an interchange format for depth/stencil blits. For
   * non-depth/stencil formats, returns the format itself.
   */
   static enum pipe_format
   agx_staging_color_format_for_zs(enum pipe_format format)
   {
      switch (format) {
   case PIPE_FORMAT_Z16_UNORM:
         case PIPE_FORMAT_Z32_FLOAT:
         case PIPE_FORMAT_S8_UINT:
         default:
      /* Z24 and combined Z/S are lowered to one of the above formats by
   * u_transfer_helper. The caller needs to pass in the rsrc->layout.format
   * and not the rsrc->base.format to get the lowered physical format
   * (rather than the API logical format).
   */
   assert(!util_format_is_depth_or_stencil(format) &&
                  }
      /* Most of the time we can do CPU-side transfers, but sometimes we need to use
   * the 3D pipe for this. Let's wrap u_blitter to blit to/from staging textures.
   * Code adapted from panfrost */
      static struct agx_resource *
   agx_alloc_staging(struct pipe_screen *screen, struct agx_resource *rsc,
         {
               tmpl.usage = PIPE_USAGE_STAGING;
   tmpl.width0 = box->width;
   tmpl.height0 = box->height;
            /* We need a linear staging resource. We have linear 2D arrays, but not
   * linear 3D or cube textures. So switch to 2D arrays if needed.
   */
   switch (tmpl.target) {
   case PIPE_TEXTURE_2D_ARRAY:
   case PIPE_TEXTURE_CUBE:
   case PIPE_TEXTURE_CUBE_ARRAY:
   case PIPE_TEXTURE_3D:
      tmpl.target = PIPE_TEXTURE_2D_ARRAY;
   tmpl.array_size = box->depth;
      default:
      assert(tmpl.array_size == 1);
   assert(box->depth == 1);
                        /* Linear is incompatible with depth/stencil, so we convert */
   tmpl.format = agx_staging_color_format_for_zs(rsc->layout.format);
   tmpl.bind &= ~PIPE_BIND_DEPTH_STENCIL;
            struct pipe_resource *pstaging = screen->resource_create(screen, &tmpl);
   if (!pstaging)
               }
      static void
   agx_blit_from_staging(struct pipe_context *pctx, struct agx_transfer *trans)
   {
      struct pipe_resource *dst = trans->base.resource;
            blit.dst.resource = dst;
   blit.dst.format =
         blit.dst.level = trans->base.level;
   blit.dst.box = trans->base.box;
   blit.src.resource = trans->staging.rsrc;
   blit.src.format = trans->staging.rsrc->format;
   blit.src.level = 0;
   blit.src.box = trans->staging.box;
   blit.mask = util_format_get_mask(blit.src.format);
               }
      static void
   agx_blit_to_staging(struct pipe_context *pctx, struct agx_transfer *trans)
   {
      struct pipe_resource *src = trans->base.resource;
            blit.src.resource = src;
   blit.src.format =
         blit.src.level = trans->base.level;
   blit.src.box = trans->base.box;
   blit.dst.resource = trans->staging.rsrc;
   blit.dst.format = trans->staging.rsrc->format;
   blit.dst.level = 0;
   blit.dst.box = trans->staging.box;
   blit.mask = util_format_get_mask(blit.dst.format);
               }
      static void *
   agx_transfer_map(struct pipe_context *pctx, struct pipe_resource *resource,
                  unsigned level,
      {
      struct agx_context *ctx = agx_context(pctx);
            /* Can't map tiled/compressed directly */
   if ((usage & PIPE_MAP_DIRECTLY) && rsrc->modifier != DRM_FORMAT_MOD_LINEAR)
            /* Can't transfer out of bounds mip levels */
   if (level >= rsrc->layout.levels)
                     /* Track the written buffer range */
   if (resource->target == PIPE_BUFFER) {
      /* Note the ordering: DISCARD|WRITE is valid, so clear before adding. */
   if (usage & PIPE_MAP_DISCARD_WHOLE_RESOURCE)
         if (usage & PIPE_MAP_WRITE) {
      util_range_add(resource, &rsrc->valid_buffer_range, box->x,
                  struct agx_transfer *transfer = CALLOC_STRUCT(agx_transfer);
   transfer->base.level = level;
   transfer->base.usage = usage;
            pipe_resource_reference(&transfer->base.resource, resource);
            /* For compression, we use a staging blit as we do not implement AGX
   * compression in software. In some cases, we could use this path for
   * twiddled too, but we don't have a use case for that yet.
   */
   if (rsrc->modifier == DRM_FORMAT_MOD_APPLE_TWIDDLED_COMPRESSED) {
      /* Should never happen for buffers, and it's not safe */
            struct agx_resource *staging =
                  /* Staging resources have one LOD: level 0. Query the strides
   * on this LOD.
   */
   transfer->base.stride = ail_get_linear_stride_B(&staging->layout, 0);
   transfer->base.layer_stride = staging->layout.layer_stride_B;
            transfer->staging.box = *box;
   transfer->staging.box.x = 0;
   transfer->staging.box.y = 0;
                     if ((usage & PIPE_MAP_READ) && agx_resource_valid(rsrc, level)) {
      agx_blit_to_staging(pctx, transfer);
               agx_bo_mmap(staging->bo);
                        if (rsrc->modifier == DRM_FORMAT_MOD_APPLE_TWIDDLED) {
      /* Should never happen for buffers, and it's not safe */
            transfer->base.stride =
            transfer->base.layer_stride = util_format_get_2d_size(
                     if ((usage & PIPE_MAP_READ) && agx_resource_valid(rsrc, level)) {
      for (unsigned z = 0; z < box->depth; ++z) {
      uint8_t *map = agx_map_texture_cpu(rsrc, level, box->z + z);
                  ail_detile(map, dst, &rsrc->layout, level, transfer->base.stride,
                     } else {
               transfer->base.stride = ail_get_linear_stride_B(&rsrc->layout, level);
            /* Be conservative for direct writes */
   if ((usage & PIPE_MAP_WRITE) &&
      (usage &
   (PIPE_MAP_DIRECTLY | PIPE_MAP_PERSISTENT | PIPE_MAP_COHERENT))) {
               uint32_t offset =
                  }
      static void
   agx_transfer_unmap(struct pipe_context *pctx, struct pipe_transfer *transfer)
   {
               struct agx_transfer *trans = agx_transfer(transfer);
   struct pipe_resource *prsrc = transfer->resource;
            if (trans->staging.rsrc && (transfer->usage & PIPE_MAP_WRITE)) {
      assert(prsrc->target != PIPE_BUFFER);
   agx_blit_from_staging(pctx, trans);
   agx_flush_readers(agx_context(pctx), agx_resource(trans->staging.rsrc),
      } else if (trans->map && (transfer->usage & PIPE_MAP_WRITE)) {
               for (unsigned z = 0; z < transfer->box.depth; ++z) {
      uint8_t *map =
                  ail_tile(map, src, &rsrc->layout, transfer->level, transfer->stride,
                        /* The level we wrote is now initialized. We do this at the end so
   * blit_from_staging can avoid reloading existing contents.
   */
   if (transfer->usage & PIPE_MAP_WRITE)
            /* Free the transfer */
   free(trans->map);
   pipe_resource_reference(&trans->staging.rsrc, NULL);
   pipe_resource_reference(&transfer->resource, NULL);
      }
      static bool
   agx_generate_mipmap(struct pipe_context *pctx, struct pipe_resource *prsrc,
                     {
               /* Generating a mipmap invalidates the written levels. Make that
   * explicit so we don't reload the previous contents.
   */
   for (unsigned l = base_level + 1; l <= last_level; ++l)
            /* For now we use util_gen_mipmap, but this has way too much overhead */
            return util_gen_mipmap(pctx, prsrc, format, base_level, last_level,
      }
      /*
   * clear/copy
   */
   static void
   agx_clear(struct pipe_context *pctx, unsigned buffers,
               {
      struct agx_context *ctx = agx_context(pctx);
            if (unlikely(!agx_render_condition_check(ctx)))
            unsigned fastclear = buffers & ~(batch->draw | batch->load);
                     /* Fast clears configure the batch */
   for (unsigned rt = 0; rt < PIPE_MAX_COLOR_BUFS; ++rt) {
      if (!(fastclear & (PIPE_CLEAR_COLOR0 << rt)))
                     batch->uploaded_clear_color[rt] =
               if (fastclear & PIPE_CLEAR_DEPTH)
            if (fastclear & PIPE_CLEAR_STENCIL)
            /* Slow clears draw a fullscreen rectangle */
   if (slowclear) {
      agx_blitter_save(ctx, ctx->blitter, false /* render cond */);
   util_blitter_clear(
      ctx->blitter, ctx->framebuffer.width, ctx->framebuffer.height,
   util_framebuffer_get_num_layers(&ctx->framebuffer), slowclear, color,
   depth, stencil,
            if (fastclear)
            batch->clear |= fastclear;
   batch->resolve |= buffers;
      }
      static void
   transition_resource(struct pipe_context *pctx, struct agx_resource *rsrc,
         {
      struct agx_resource *new_res =
            assert(new_res);
            int level;
   BITSET_FOREACH_SET(level, rsrc->data_valid, PIPE_MAX_TEXTURE_LEVELS) {
      /* Blit each valid level */
            u_box_3d(0, 0, 0, rsrc->layout.width_px, rsrc->layout.height_px,
                  blit.dst.resource = &new_res->base;
   blit.dst.format = new_res->base.format;
   blit.dst.level = level;
   blit.src.resource = &rsrc->base;
   blit.src.format = rsrc->base.format;
   blit.src.level = level;
   blit.mask = util_format_get_mask(blit.src.format);
   blit.filter = PIPE_TEX_FILTER_NEAREST;
               /* Flush the blits out, to make sure the old resource is no longer used */
            /* Copy the bind flags and swap the BOs */
   struct agx_bo *old = rsrc->bo;
   rsrc->base.bind = new_res->base.bind;
   rsrc->layout = new_res->layout;
   rsrc->modifier = new_res->modifier;
   rsrc->bo = new_res->bo;
            /* Free the new resource, which now owns the old BO */
      }
      void
   agx_decompress(struct agx_context *ctx, struct agx_resource *rsrc,
         {
      if (rsrc->layout.tiling == AIL_TILING_TWIDDLED_COMPRESSED) {
         } else if (!rsrc->layout.writeable_image) {
                  struct pipe_resource templ = rsrc->base;
   assert(!(templ.bind & PIPE_BIND_SHADER_IMAGE) && "currently compressed");
   templ.bind |= PIPE_BIND_SHADER_IMAGE /* forces off compression */;
      }
      static void
   agx_flush_resource(struct pipe_context *pctx, struct pipe_resource *pres)
   {
               /* flush_resource is used to prepare resources for sharing, so if this is not
   * already a shareabe resource, make it so
   */
   struct agx_bo *old = rsrc->bo;
   if (!(old->flags & AGX_BO_SHAREABLE)) {
      assert(rsrc->layout.levels == 1 &&
         assert(rsrc->layout.sample_count_sa == 1 &&
         assert(rsrc->bo);
            struct pipe_resource templ = *pres;
   templ.bind |= PIPE_BIND_SHARED;
      } else {
      /* Otherwise just claim it's already shared */
   pres->bind |= PIPE_BIND_SHARED;
         }
      /*
   * context
   */
   static void
   agx_flush(struct pipe_context *pctx, struct pipe_fence_handle **fence,
         {
                        /* At this point all pending work has been submitted. Since jobs are
   * started and completed sequentially from a UAPI perspective, and since
   * we submit all jobs with compute+render barriers on the prior job,
   * waiting on the last submitted job is sufficient to guarantee completion
   * of all GPU work thus far, so we can create a fence out of the latest
   * syncobj.
   *
   * See this page for more info on how the GPU/UAPI queueing works:
   * https://github.com/AsahiLinux/docs/wiki/SW:AGX-driver-notes#queues
            if (fence) {
      struct pipe_fence_handle *f = agx_fence_create(ctx);
   pctx->screen->fence_reference(pctx->screen, fence, NULL);
         }
      void
   agx_flush_batch(struct agx_context *ctx, struct agx_batch *batch)
   {
               assert(agx_batch_is_active(batch));
            /* Make sure there's something to submit. */
   if (!batch->clear && !batch->any_draws) {
      agx_batch_reset(ctx, batch);
                        /* Finalize the encoder */
   uint8_t stop[5 + 64] = {0x00, 0x00, 0x00, 0xc0, 0x00};
            uint64_t pipeline_background = agx_build_meta(batch, false, false);
   uint64_t pipeline_background_partial = agx_build_meta(batch, false, true);
            bool clear_pipeline_textures =
            for (unsigned i = 0; i < batch->key.nr_cbufs; ++i) {
               if (surf && surf->texture) {
                     if (!(batch->clear & (PIPE_CLEAR_COLOR0 << i)))
                  struct agx_resource *zbuf =
            if (zbuf) {
      unsigned level = batch->key.zsbuf->u.tex.level;
            if (zbuf->separate_stencil)
               /* Scissor and depth bias arrays are staged to dynamic arrays on the CPU. At
   * submit time, they're done growing and are uploaded to GPU memory attached
   * to the batch.
   */
   uint64_t scissor = agx_pool_upload_aligned(&batch->pool, batch->scissor.data,
         uint64_t zbias = agx_pool_upload_aligned(
            /* BO list for a given batch consists of:
   *  - BOs for the batch's pools
   *  - BOs for the encoder
   *  - BO for internal shaders
   *  - BOs added to the batch explicitly
   */
            /* Occlusion queries are allocated as a contiguous pool */
   unsigned oq_count =
                  if (oq_size) {
      batch->occlusion_buffer =
            } else {
                  unsigned handle_count = agx_batch_num_bo(batch) +
                  uint32_t *handles = calloc(sizeof(uint32_t), handle_count);
            AGX_BATCH_FOREACH_BO_HANDLE(batch, handle) {
                  agx_pool_get_bo_handles(&batch->pool, handles + handle_i);
            agx_pool_get_bo_handles(&batch->pipeline_pool, handles + handle_i);
            /* Size calculation should've been exact */
            /* TODO: Linux UAPI submission */
   (void)dev;
   (void)zbias;
   (void)scissor;
   (void)clear_pipeline_textures;
   (void)pipeline_store;
   (void)pipeline_background;
            unreachable("Linux UAPI not yet upstream");
      }
      static void
   agx_destroy_context(struct pipe_context *pctx)
   {
      struct agx_device *dev = agx_device(pctx->screen);
            /* Batch state needs to be freed on completion, and we don't want to yank
   * buffers out from in-progress GPU jobs to avoid faults, so just wait until
   * everything in progress is actually done on context destroy. This will
   * ensure everything is cleaned up properly.
   */
            if (pctx->stream_uploader)
            if (ctx->blitter)
                                       drmSyncobjDestroy(dev->fd, ctx->in_sync_obj);
   drmSyncobjDestroy(dev->fd, ctx->dummy_syncobj);
   if (ctx->in_sync_fd != -1)
            for (unsigned i = 0; i < AGX_MAX_BATCHES; ++i) {
      if (ctx->batches.slots[i].syncobj)
                  }
      static void
   agx_invalidate_resource(struct pipe_context *pctx,
         {
      struct agx_context *ctx = agx_context(pctx);
            /* Handle the glInvalidateFramebuffer case */
   if (batch->key.zsbuf && batch->key.zsbuf->texture == resource)
            for (unsigned i = 0; i < batch->key.nr_cbufs; ++i) {
               if (surf && surf->texture == resource)
         }
      static void
   agx_memory_barrier(struct pipe_context *pctx, unsigned flags)
   {
      /* Be conservative for now, we can try to optimize this more later */
      }
      static struct pipe_context *
   agx_create_context(struct pipe_screen *screen, void *priv, unsigned flags)
   {
      struct agx_context *ctx = rzalloc(NULL, struct agx_context);
   struct pipe_context *pctx = &ctx->base;
            if (!ctx)
            pctx->screen = screen;
            util_dynarray_init(&ctx->writer, ctx);
            pctx->stream_uploader = u_upload_create_default(pctx);
   if (!pctx->stream_uploader) {
      FREE(pctx);
      }
            pctx->destroy = agx_destroy_context;
   pctx->flush = agx_flush;
   pctx->clear = agx_clear;
   pctx->resource_copy_region = util_resource_copy_region;
   pctx->blit = agx_blit;
   pctx->generate_mipmap = agx_generate_mipmap;
            pctx->buffer_map = u_transfer_helper_transfer_map;
   pctx->buffer_unmap = u_transfer_helper_transfer_unmap;
   pctx->texture_map = u_transfer_helper_transfer_map;
   pctx->texture_unmap = u_transfer_helper_transfer_unmap;
            pctx->buffer_subdata = u_default_buffer_subdata;
   pctx->clear_buffer = u_default_clear_buffer;
   pctx->texture_subdata = u_default_texture_subdata;
   pctx->set_debug_callback = u_default_set_debug_callback;
   pctx->get_sample_position = u_default_get_sample_position;
   pctx->invalidate_resource = agx_invalidate_resource;
            pctx->create_fence_fd = agx_create_fence_fd;
            agx_init_state_functions(pctx);
   agx_init_query_functions(pctx);
                              ctx->result_buf = agx_bo_create(
      agx_device(screen), sizeof(union agx_batch_result) * AGX_MAX_BATCHES,
               /* Sync object/FD used for NATIVE_FENCE_FD. */
   ctx->in_sync_fd = -1;
   ret = drmSyncobjCreate(agx_device(screen)->fd, 0, &ctx->in_sync_obj);
            /* Dummy sync object used before any work has been submitted. */
   ret = drmSyncobjCreate(agx_device(screen)->fd, DRM_SYNCOBJ_CREATE_SIGNALED,
         assert(!ret);
            /* By default all samples are enabled */
                        }
      static const char *
   agx_get_vendor(struct pipe_screen *pscreen)
   {
         }
      static const char *
   agx_get_device_vendor(struct pipe_screen *pscreen)
   {
         }
      static const char *
   agx_get_name(struct pipe_screen *pscreen)
   {
                  }
      static int
   agx_get_param(struct pipe_screen *pscreen, enum pipe_cap param)
   {
               switch (param) {
   case PIPE_CAP_NPOT_TEXTURES:
   case PIPE_CAP_SHADER_STENCIL_EXPORT:
   case PIPE_CAP_MIXED_COLOR_DEPTH_BITS:
   case PIPE_CAP_FRAGMENT_SHADER_TEXTURE_LOD:
   case PIPE_CAP_VERTEX_COLOR_UNCLAMPED:
   case PIPE_CAP_DEPTH_CLIP_DISABLE:
   case PIPE_CAP_MIXED_FRAMEBUFFER_SIZES:
   case PIPE_CAP_FRAGMENT_SHADER_DERIVATIVES:
   case PIPE_CAP_FRAMEBUFFER_NO_ATTACHMENT:
   case PIPE_CAP_SHADER_PACK_HALF_FLOAT:
   case PIPE_CAP_FS_FINE_DERIVATIVE:
            /* We could support ARB_clip_control by toggling the clip control bit for
   * the render pass. Because this bit is for the whole render pass,
   * switching clip modes necessarily incurs a flush. This should be ok, from
   * the ARB_clip_control spec:
   *
   *         Some implementations may introduce a flush when changing the
   *         clip control state.  Hence frequent clip control changes are
   *         not recommended.
   *
   * However, this would require tuning to ensure we don't flush unnecessary
   * when using u_blitter clears, for example. As we don't yet have a use case,
   * don't expose the feature.
   */
   case PIPE_CAP_CLIP_HALFZ:
            case PIPE_CAP_MAX_RENDER_TARGETS:
   case PIPE_CAP_FBFETCH:
   case PIPE_CAP_FBFETCH_COHERENT:
         case PIPE_CAP_MAX_DUAL_SOURCE_RENDER_TARGETS:
            case PIPE_CAP_OCCLUSION_QUERY:
   case PIPE_CAP_GENERATE_MIPMAP:
   case PIPE_CAP_PRIMITIVE_RESTART:
   case PIPE_CAP_PRIMITIVE_RESTART_FIXED_INDEX:
   case PIPE_CAP_ANISOTROPIC_FILTER:
   case PIPE_CAP_NATIVE_FENCE_FD:
            case PIPE_CAP_SAMPLER_VIEW_TARGET:
   case PIPE_CAP_TEXTURE_SWIZZLE:
   case PIPE_CAP_BLEND_EQUATION_SEPARATE:
   case PIPE_CAP_INDEP_BLEND_ENABLE:
   case PIPE_CAP_INDEP_BLEND_FUNC:
   case PIPE_CAP_ACCELERATED:
   case PIPE_CAP_UMA:
   case PIPE_CAP_TEXTURE_FLOAT_LINEAR:
   case PIPE_CAP_TEXTURE_HALF_FLOAT_LINEAR:
   case PIPE_CAP_TEXTURE_MIRROR_CLAMP_TO_EDGE:
   case PIPE_CAP_SHADER_ARRAY_COMPONENTS:
   case PIPE_CAP_PACKED_UNIFORMS:
   case PIPE_CAP_QUADS_FOLLOW_PROVOKING_VERTEX_CONVENTION:
   case PIPE_CAP_VS_INSTANCEID:
   case PIPE_CAP_VERTEX_ELEMENT_INSTANCE_DIVISOR:
   case PIPE_CAP_CONDITIONAL_RENDER:
   case PIPE_CAP_CONDITIONAL_RENDER_INVERTED:
   case PIPE_CAP_SEAMLESS_CUBE_MAP:
   case PIPE_CAP_LOAD_CONSTBUF:
   case PIPE_CAP_SEAMLESS_CUBE_MAP_PER_TEXTURE:
   case PIPE_CAP_TEXTURE_BUFFER_OBJECTS:
   case PIPE_CAP_NULL_TEXTURES:
   case PIPE_CAP_TEXTURE_MULTISAMPLE:
   case PIPE_CAP_IMAGE_LOAD_FORMATTED:
   case PIPE_CAP_IMAGE_STORE_FORMATTED:
   case PIPE_CAP_COMPUTE:
   case PIPE_CAP_INT64:
   case PIPE_CAP_SAMPLE_SHADING:
         case PIPE_CAP_SURFACE_SAMPLE_COUNT:
      /* TODO: MSRTT */
         case PIPE_CAP_CUBE_MAP_ARRAY:
            case PIPE_CAP_COPY_BETWEEN_COMPRESSED_AND_PLAIN_FORMATS:
            case PIPE_CAP_MAX_STREAM_OUTPUT_BUFFERS:
            case PIPE_CAP_MAX_STREAM_OUTPUT_SEPARATE_COMPONENTS:
   case PIPE_CAP_MAX_STREAM_OUTPUT_INTERLEAVED_COMPONENTS:
            case PIPE_CAP_STREAM_OUTPUT_PAUSE_RESUME:
   case PIPE_CAP_STREAM_OUTPUT_INTERLEAVE_BUFFERS:
            case PIPE_CAP_MAX_TEXTURE_ARRAY_LAYERS:
            case PIPE_CAP_GLSL_FEATURE_LEVEL:
   case PIPE_CAP_GLSL_FEATURE_LEVEL_COMPATIBILITY:
         case PIPE_CAP_ESSL_FEATURE_LEVEL:
            case PIPE_CAP_CONSTANT_BUFFER_OFFSET_ALIGNMENT:
            case PIPE_CAP_MAX_TEXEL_BUFFER_ELEMENTS_UINT:
            case PIPE_CAP_TEXTURE_BUFFER_OFFSET_ALIGNMENT:
            case PIPE_CAP_VERTEX_ATTRIB_ELEMENT_ALIGNED_ONLY:
            /* We run nir_lower_point_size so we need the GLSL linker to copy
   * the original gl_PointSize when captured by transform feedback. We could
   * also copy it ourselves but it's easier to set the CAP.
   */
   case PIPE_CAP_PSIZ_CLAMPED:
            case PIPE_CAP_MAX_TEXTURE_2D_SIZE:
         case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
   case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
            case PIPE_CAP_FS_COORD_ORIGIN_UPPER_LEFT:
   case PIPE_CAP_FS_COORD_PIXEL_CENTER_HALF_INTEGER:
   case PIPE_CAP_TGSI_TEXCOORD:
   case PIPE_CAP_FS_FACE_IS_INTEGER_SYSVAL:
   case PIPE_CAP_FS_POSITION_IS_SYSVAL:
         case PIPE_CAP_FS_COORD_ORIGIN_LOWER_LEFT:
   case PIPE_CAP_FS_COORD_PIXEL_CENTER_INTEGER:
   case PIPE_CAP_FS_POINT_IS_SYSVAL:
            case PIPE_CAP_MAX_VERTEX_ELEMENT_SRC_OFFSET:
            case PIPE_CAP_TEXTURE_TRANSFER_MODES:
            case PIPE_CAP_ENDIANNESS:
            case PIPE_CAP_MAX_TEXTURE_GATHER_COMPONENTS:
         case PIPE_CAP_MIN_TEXTURE_GATHER_OFFSET:
         case PIPE_CAP_MAX_TEXTURE_GATHER_OFFSET:
         case PIPE_CAP_DRAW_INDIRECT:
            case PIPE_CAP_VIDEO_MEMORY: {
               if (!os_get_total_physical_memory(&system_memory))
                        case PIPE_CAP_SHADER_BUFFER_OFFSET_ALIGNMENT:
            case PIPE_CAP_MAX_VARYINGS:
            case PIPE_CAP_FLATSHADE:
   case PIPE_CAP_TWO_SIDED_COLOR:
   case PIPE_CAP_ALPHA_TEST:
   case PIPE_CAP_POINT_SIZE_FIXED:
   case PIPE_CAP_CLIP_PLANES:
   case PIPE_CAP_NIR_IMAGES_AS_DEREF:
            case PIPE_CAP_TEXTURE_BORDER_COLOR_QUIRK:
            case PIPE_CAP_SUPPORTED_PRIM_MODES:
   case PIPE_CAP_SUPPORTED_PRIM_MODES_WITH_RESTART:
      return BITFIELD_BIT(MESA_PRIM_POINTS) | BITFIELD_BIT(MESA_PRIM_LINES) |
         BITFIELD_BIT(MESA_PRIM_LINE_STRIP) |
   BITFIELD_BIT(MESA_PRIM_LINE_LOOP) |
   BITFIELD_BIT(MESA_PRIM_TRIANGLES) |
   BITFIELD_BIT(MESA_PRIM_TRIANGLE_STRIP) |
         case PIPE_CAP_MAP_UNSYNCHRONIZED_THREAD_SAFE:
            case PIPE_CAP_VS_LAYER_VIEWPORT:
            default:
            }
      static float
   agx_get_paramf(struct pipe_screen *pscreen, enum pipe_capf param)
   {
      switch (param) {
   case PIPE_CAPF_MIN_LINE_WIDTH:
   case PIPE_CAPF_MIN_LINE_WIDTH_AA:
   case PIPE_CAPF_MIN_POINT_SIZE:
   case PIPE_CAPF_MIN_POINT_SIZE_AA:
            case PIPE_CAPF_POINT_SIZE_GRANULARITY:
   case PIPE_CAPF_LINE_WIDTH_GRANULARITY:
            case PIPE_CAPF_MAX_LINE_WIDTH:
   case PIPE_CAPF_MAX_LINE_WIDTH_AA:
            case PIPE_CAPF_MAX_POINT_SIZE:
   case PIPE_CAPF_MAX_POINT_SIZE_AA:
            case PIPE_CAPF_MAX_TEXTURE_ANISOTROPY:
            case PIPE_CAPF_MAX_TEXTURE_LOD_BIAS:
            case PIPE_CAPF_MIN_CONSERVATIVE_RASTER_DILATE:
   case PIPE_CAPF_MAX_CONSERVATIVE_RASTER_DILATE:
   case PIPE_CAPF_CONSERVATIVE_RASTER_DILATE_GRANULARITY:
            default:
      debug_printf("Unexpected PIPE_CAPF %d query\n", param);
         }
      static int
   agx_get_shader_param(struct pipe_screen *pscreen, enum pipe_shader_type shader,
         {
               switch (shader) {
   case PIPE_SHADER_VERTEX:
   case PIPE_SHADER_FRAGMENT:
   case PIPE_SHADER_COMPUTE:
         default:
                  /* Don't allow side effects with vertex processing. The APIs don't require it
   * and it may be problematic on our hardware.
   */
            /* this is probably not totally correct.. but it's a start: */
   switch (param) {
   case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
            case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
            case PIPE_SHADER_CAP_MAX_INPUTS:
            case PIPE_SHADER_CAP_MAX_OUTPUTS:
            case PIPE_SHADER_CAP_MAX_TEMPS:
            case PIPE_SHADER_CAP_MAX_CONST_BUFFER0_SIZE:
            case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
            case PIPE_SHADER_CAP_CONT_SUPPORTED:
            case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
   case PIPE_SHADER_CAP_SUBROUTINES:
   case PIPE_SHADER_CAP_TGSI_SQRT_SUPPORTED:
            case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
   case PIPE_SHADER_CAP_INTEGERS:
            case PIPE_SHADER_CAP_FP16:
   case PIPE_SHADER_CAP_GLSL_16BIT_CONSTS:
   case PIPE_SHADER_CAP_FP16_DERIVATIVES:
   case PIPE_SHADER_CAP_FP16_CONST_BUFFERS:
         case PIPE_SHADER_CAP_INT16:
      /* GLSL compiler is broken. Flip this on when Panfrost does. */
         case PIPE_SHADER_CAP_INT64_ATOMICS:
   case PIPE_SHADER_CAP_TGSI_ANY_INOUT_DECL_RANGE:
            case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
            case PIPE_SHADER_CAP_MAX_SAMPLER_VIEWS:
            case PIPE_SHADER_CAP_SUPPORTED_IRS:
            case PIPE_SHADER_CAP_MAX_SHADER_BUFFERS:
            case PIPE_SHADER_CAP_MAX_SHADER_IMAGES:
            case PIPE_SHADER_CAP_MAX_HW_ATOMIC_COUNTERS:
   case PIPE_SHADER_CAP_MAX_HW_ATOMIC_COUNTER_BUFFERS:
            default:
      /* Other params are unknown */
                  }
      static int
   agx_get_compute_param(struct pipe_screen *pscreen, enum pipe_shader_ir ir_type,
         {
   #define RET(x)                                                                 \
      do {                                                                        \
      if (ret)                                                                 \
                     switch (param) {
   case PIPE_COMPUTE_CAP_ADDRESS_BITS:
            case PIPE_COMPUTE_CAP_IR_TARGET:
      if (ret)
               case PIPE_COMPUTE_CAP_GRID_DIMENSION:
            case PIPE_COMPUTE_CAP_MAX_GRID_SIZE:
            case PIPE_COMPUTE_CAP_MAX_BLOCK_SIZE:
            case PIPE_COMPUTE_CAP_MAX_THREADS_PER_BLOCK:
            case PIPE_COMPUTE_CAP_MAX_GLOBAL_SIZE:
   case PIPE_COMPUTE_CAP_MAX_MEM_ALLOC_SIZE: {
               if (!os_get_total_physical_memory(&system_memory))
                        case PIPE_COMPUTE_CAP_MAX_LOCAL_SIZE:
            case PIPE_COMPUTE_CAP_MAX_PRIVATE_SIZE:
   case PIPE_COMPUTE_CAP_MAX_INPUT_SIZE:
            case PIPE_COMPUTE_CAP_MAX_CLOCK_FREQUENCY:
            case PIPE_COMPUTE_CAP_MAX_COMPUTE_UNITS:
            case PIPE_COMPUTE_CAP_IMAGES_SUPPORTED:
            case PIPE_COMPUTE_CAP_SUBGROUP_SIZES:
            case PIPE_COMPUTE_CAP_MAX_SUBGROUPS:
            case PIPE_COMPUTE_CAP_MAX_VARIABLE_THREADS_PER_BLOCK:
                     }
      static bool
   agx_is_format_supported(struct pipe_screen *pscreen, enum pipe_format format,
               {
      assert(target == PIPE_BUFFER || target == PIPE_TEXTURE_1D ||
         target == PIPE_TEXTURE_1D_ARRAY || target == PIPE_TEXTURE_2D ||
   target == PIPE_TEXTURE_2D_ARRAY || target == PIPE_TEXTURE_RECT ||
            if (sample_count > 1 && sample_count != 4 && sample_count != 2)
            if (sample_count > 1 && agx_device(pscreen)->debug & AGX_DBG_NOMSAA)
            if (MAX2(sample_count, 1) != MAX2(storage_sample_count, 1))
            if ((usage & PIPE_BIND_VERTEX_BUFFER) && !agx_vbo_supports_format(format))
            /* For framebuffer_no_attachments, fake support for "none" images */
   if (format == PIPE_FORMAT_NONE)
            if (usage & (PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW |
                     /* Mimic the fixup done in create_sampler_view and u_transfer_helper so we
   * advertise GL_OES_texture_stencil8. Alternatively, we could make mesa/st
   * less stupid?
   */
   if (tex_format == PIPE_FORMAT_X24S8_UINT)
                     if (!agx_is_valid_pixel_format(tex_format))
            /* RGB32 is emulated for texture buffers only */
   if (ent.channels == AGX_CHANNELS_R32G32B32_EMULATED &&
                  if ((usage & PIPE_BIND_RENDER_TARGET) && !ent.renderable)
               if (usage & PIPE_BIND_DEPTH_STENCIL) {
      switch (format) {
   /* natively supported */
   case PIPE_FORMAT_Z16_UNORM:
   case PIPE_FORMAT_Z32_FLOAT:
            /* lowered by u_transfer_helper to one of the above */
   case PIPE_FORMAT_Z24X8_UNORM:
   case PIPE_FORMAT_Z24_UNORM_S8_UINT:
   case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
            default:
                        }
      static void
   agx_query_dmabuf_modifiers(struct pipe_screen *screen, enum pipe_format format,
               {
               if (max == 0) {
      *out_count = ARRAY_SIZE(agx_best_modifiers);
               for (i = 0; i < ARRAY_SIZE(agx_best_modifiers) && i < max; i++) {
      if (external_only)
                        /* Return the number of modifiers copied */
      }
      static bool
   agx_is_dmabuf_modifier_supported(struct pipe_screen *screen, uint64_t modifier,
         {
      if (external_only)
            for (unsigned i = 0; i < ARRAY_SIZE(agx_best_modifiers); ++i) {
      if (agx_best_modifiers[i] == modifier)
                  }
      static void
   agx_destroy_screen(struct pipe_screen *pscreen)
   {
               if (screen->dev.ro)
            u_transfer_helper_destroy(pscreen->transfer_helper);
   agx_close_device(&screen->dev);
   disk_cache_destroy(screen->disk_cache);
      }
      static const void *
   agx_get_compiler_options(struct pipe_screen *pscreen, enum pipe_shader_ir ir,
         {
         }
      static void
   agx_resource_set_stencil(struct pipe_resource *prsrc,
         {
         }
      static struct pipe_resource *
   agx_resource_get_stencil(struct pipe_resource *prsrc)
   {
         }
      static enum pipe_format
   agx_resource_get_internal_format(struct pipe_resource *prsrc)
   {
         }
      static struct disk_cache *
   agx_get_disk_shader_cache(struct pipe_screen *pscreen)
   {
         }
      static const struct u_transfer_vtbl transfer_vtbl = {
      .resource_create = agx_resource_create,
   .resource_destroy = agx_resource_destroy,
   .transfer_map = agx_transfer_map,
   .transfer_unmap = agx_transfer_unmap,
   .transfer_flush_region = agx_transfer_flush_region,
   .get_internal_format = agx_resource_get_internal_format,
   .set_stencil = agx_resource_set_stencil,
      };
      static int
   agx_screen_get_fd(struct pipe_screen *pscreen)
   {
         }
      struct pipe_screen *
   agx_screen_create(int fd, struct renderonly *ro,
         {
      struct agx_screen *agx_screen;
            agx_screen = rzalloc(NULL, struct agx_screen);
   if (!agx_screen)
                     /* Set debug before opening */
   agx_screen->dev.debug =
            /* parse driconf configuration now for device specific overrides */
   driParseConfigFiles(config->options, config->options_info, 0, "asahi", NULL,
            /* Forward no16 flag from driconf */
   if (driQueryOptionb(config->options, "no_fp16"))
            agx_screen->dev.fd = fd;
            /* Try to open an AGX device */
   if (!agx_open_device(screen, &agx_screen->dev)) {
      ralloc_free(agx_screen);
               if (agx_screen->dev.debug & AGX_DBG_DEQP) {
      /* You're on your own. */
            if (!warned_about_hacks) {
      agx_msg("\n------------------\n"
         "Unsupported debug parameter set. Expect breakage.\n"
   "Do not report bugs.\n"
                  screen->destroy = agx_destroy_screen;
   screen->get_screen_fd = agx_screen_get_fd;
   screen->get_name = agx_get_name;
   screen->get_vendor = agx_get_vendor;
   screen->get_device_vendor = agx_get_device_vendor;
   screen->get_param = agx_get_param;
   screen->get_shader_param = agx_get_shader_param;
   screen->get_compute_param = agx_get_compute_param;
   screen->get_paramf = agx_get_paramf;
   screen->is_format_supported = agx_is_format_supported;
   screen->query_dmabuf_modifiers = agx_query_dmabuf_modifiers;
   screen->is_dmabuf_modifier_supported = agx_is_dmabuf_modifier_supported;
   screen->context_create = agx_create_context;
   screen->resource_from_handle = agx_resource_from_handle;
   screen->resource_get_handle = agx_resource_get_handle;
   screen->resource_get_param = agx_resource_get_param;
   screen->resource_create_with_modifiers = agx_resource_create_with_modifiers;
   screen->get_timestamp = u_default_get_timestamp;
   screen->fence_reference = agx_fence_reference;
   screen->fence_finish = agx_fence_finish;
   screen->fence_get_fd = agx_fence_get_fd;
   screen->get_compiler_options = agx_get_compiler_options;
            screen->resource_create = u_transfer_helper_resource_create;
   screen->resource_destroy = u_transfer_helper_resource_destroy;
   screen->transfer_helper = u_transfer_helper_create(
      &transfer_vtbl,
   U_TRANSFER_HELPER_SEPARATE_Z32S8 | U_TRANSFER_HELPER_SEPARATE_STENCIL |
                     }
