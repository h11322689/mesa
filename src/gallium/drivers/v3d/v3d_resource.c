   /*
   * Copyright © 2014-2017 Broadcom
   * Copyright (C) 2012 Rob Clark <robclark@freedesktop.org>
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
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "pipe/p_defines.h"
   #include "util/u_memory.h"
   #include "util/format/u_format.h"
   #include "util/u_inlines.h"
   #include "util/u_resource.h"
   #include "util/u_surface.h"
   #include "util/u_transfer_helper.h"
   #include "util/u_upload_mgr.h"
   #include "util/format/u_format_zs.h"
   #include "util/u_drm.h"
      #include "drm-uapi/drm_fourcc.h"
   #include "v3d_screen.h"
   #include "v3d_context.h"
   #include "v3d_resource.h"
   #include "broadcom/cle/v3d_packet_v33_pack.h"
      static void
   v3d_debug_resource_layout(struct v3d_resource *rsc, const char *caller)
   {
         if (!V3D_DBG(SURFACE))
                     if (prsc->target == PIPE_BUFFER) {
            fprintf(stderr,
            "rsc %s %p (format %s), %dx%d buffer @0x%08x-0x%08x\n",
   caller, rsc,
   util_format_short_name(prsc->format),
   prsc->width0, prsc->height0,
            static const char *const tiling_descriptions[] = {
            [V3D_TILING_RASTER] = "R",
   [V3D_TILING_LINEARTILE] = "LT",
   [V3D_TILING_UBLINEAR_1_COLUMN] = "UB1",
   [V3D_TILING_UBLINEAR_2_COLUMN] = "UB2",
               for (int i = 0; i <= prsc->last_level; i++) {
                     int level_width = slice->stride / rsc->cpp;
                        fprintf(stderr,
            "rsc %s %p (format %s), %dx%d: "
   "level %d (%s) %dx%dx%d -> %dx%dx%d, stride %d@0x%08x\n",
   caller, rsc,
   util_format_short_name(prsc->format),
   prsc->width0, prsc->height0,
   i, tiling_descriptions[slice->tiling],
   u_minify(prsc->width0, i),
   u_minify(prsc->height0, i),
   u_minify(prsc->depth0, i),
   level_width,
   level_height,
   }
      static bool
   v3d_resource_bo_alloc(struct v3d_resource *rsc)
   {
         struct pipe_resource *prsc = &rsc->base;
   struct pipe_screen *pscreen = prsc->screen;
            /* Buffers may be read using ldunifa, which prefetches the next
      * 4 bytes after a read. If the buffer's size is exactly a multiple
   * of a page size and the shader reads the last 4 bytes with ldunifa
   * the prefetching would read out of bounds and cause an MMU error,
   * so we allocate extra space to avoid kernel error spamming.
      uint32_t size = rsc->size;
   if (rsc->base.target == PIPE_BUFFER && (size % 4096 == 0))
            bo = v3d_bo_alloc(v3d_screen(pscreen), size, "resource");
   if (bo) {
            v3d_bo_unreference(&rsc->bo);
   rsc->bo = bo;
   rsc->serial_id++;
      } else {
         }
      static void
   v3d_resource_transfer_unmap(struct pipe_context *pctx,
         {
         struct v3d_context *v3d = v3d_context(pctx);
            if (trans->map) {
                           if (ptrans->usage & PIPE_MAP_WRITE) {
            for (int z = 0; z < ptrans->box.depth; z++) {
         void *dst = rsc->bo->map +
            v3d_layer_offset(&rsc->base,
      v3d_store_tiled_image(dst,
                           slice->stride,
   (trans->map +
                  pipe_resource_reference(&ptrans->resource, NULL);
   }
      static void
   rebind_sampler_views(struct v3d_context *v3d,
         {
         for (int st = 0; st < PIPE_SHADER_TYPES; st++) {
                                                                                          }
      static void
   v3d_map_usage_prep(struct pipe_context *pctx,
               {
         struct v3d_context *v3d = v3d_context(pctx);
            if (usage & PIPE_MAP_DISCARD_WHOLE_RESOURCE) {
            if (v3d_resource_bo_alloc(rsc)) {
            /* If it might be bound as one of our vertex buffers
   * or UBOs, make sure we re-emit vertex buffer state
   * or uniforms.
   */
   if (prsc->bind & PIPE_BIND_VERTEX_BUFFER)
         if (prsc->bind & PIPE_BIND_CONSTANT_BUFFER)
         /* Since we are changing the texture BO we need to
   * update any bound samplers to point to the new
   * BO. Notice we can have samplers that are not
   * currently bound to the state that won't be
   * updated. These will be fixed when they are bound in
   * v3d_set_sampler_views.
   */
      } else {
            /* If we failed to reallocate, flush users so that we
   * don't violate any syncing requirements.
   */
   v3d_flush_jobs_reading_resource(v3d, prsc,
   } else if (!(usage & PIPE_MAP_UNSYNCHRONIZED)) {
            /* If we're writing and the buffer is being used by the CL, we
   * have to flush the CL first.  If we're only reading, we need
   * to flush if the CL has written our buffer.
   */
   if (usage & PIPE_MAP_WRITE) {
            v3d_flush_jobs_reading_resource(v3d, prsc,
      } else {
            v3d_flush_jobs_writing_resource(v3d, prsc,
            if (usage & PIPE_MAP_WRITE) {
               }
      static void *
   v3d_resource_transfer_map(struct pipe_context *pctx,
                           {
         struct v3d_context *v3d = v3d_context(pctx);
   struct v3d_resource *rsc = v3d_resource(prsc);
   struct v3d_transfer *trans;
   struct pipe_transfer *ptrans;
   enum pipe_format format = prsc->format;
            /* MSAA maps should have been handled by u_transfer_helper. */
            /* Upgrade DISCARD_RANGE to WHOLE_RESOURCE if the whole resource is
      * being mapped.
      if ((usage & PIPE_MAP_DISCARD_RANGE) &&
         !(usage & PIPE_MAP_UNSYNCHRONIZED) &&
   !(prsc->flags & PIPE_RESOURCE_FLAG_MAP_PERSISTENT) &&
   prsc->last_level == 0 &&
   prsc->width0 == box->width &&
   prsc->height0 == box->height &&
   prsc->depth0 == box->depth &&
   prsc->array_size == 1 &&
   rsc->bo->private) {
                     trans = slab_zalloc(&v3d->transfer_pool);
   if (!trans)
                              pipe_resource_reference(&ptrans->resource, prsc);
   ptrans->level = level;
   ptrans->usage = usage;
            /* Note that the current kernel implementation is synchronous, so no
                  if (usage & PIPE_MAP_UNSYNCHRONIZED)
         else
         if (!buf) {
                                 /* Our load/store routines work on entire compressed blocks. */
            struct v3d_resource_slice *slice = &rsc->slices[level];
   if (rsc->tiled) {
            /* No direct mappings of tiled, since we need to manually
   * tile/untile.
                                                if (usage & PIPE_MAP_READ) {
            for (int z = 0; z < ptrans->box.depth; z++) {
         void *src = rsc->bo->map +
            v3d_layer_offset(&rsc->base,
      v3d_load_tiled_image((trans->map +
                           ptrans->stride *
      ptrans->stride,
      } else {
                           return buf + slice->offset +
                  fail:
         v3d_resource_transfer_unmap(pctx, ptrans);
   }
      static void
   v3d_texture_subdata(struct pipe_context *pctx,
                     struct pipe_resource *prsc,
   unsigned level,
   unsigned usage,
   const struct pipe_box *box,
   {
         struct v3d_resource *rsc = v3d_resource(prsc);
            /* For a direct mapping, we can just take the u_transfer path. */
   if (!rsc->tiled) {
                        /* Otherwise, map and store the texture data directly into the tiled
      * texture.  Note that gallium's texture_subdata may be called with
   * obvious usage flags missing!
      v3d_map_usage_prep(pctx, prsc, usage | (PIPE_MAP_WRITE |
            void *buf;
   if (usage & PIPE_MAP_UNSYNCHRONIZED)
         else
            for (int i = 0; i < box->depth; i++) {
            v3d_store_tiled_image(buf +
                        v3d_layer_offset(&rsc->base,
               }
      static void
   v3d_resource_destroy(struct pipe_screen *pscreen,
         {
         struct v3d_screen *screen = v3d_screen(pscreen);
            if (rsc->scanout)
            v3d_bo_unreference(&rsc->bo);
   }
      static uint64_t
   v3d_resource_modifier(struct v3d_resource *rsc)
   {
         if (rsc->tiled) {
            /* A shared tiled buffer should always be allocated as UIF,
   * not UBLINEAR or LT.
   */
   assert(rsc->slices[0].tiling == V3D_TILING_UIF_XOR ||
      } else {
         }
      static bool
   v3d_resource_get_handle(struct pipe_screen *pscreen,
                           {
         struct v3d_screen *screen = v3d_screen(pscreen);
   struct v3d_resource *rsc = v3d_resource(prsc);
            whandle->stride = rsc->slices[0].stride;
   whandle->offset = 0;
            /* If we're passing some reference to our BO out to some other part of
      * the system, then we can't do any optimizations about only us being
   * the ones seeing it (like BO caching).
               switch (whandle->type) {
   case WINSYS_HANDLE_TYPE_SHARED:
         case WINSYS_HANDLE_TYPE_KMS:
            if (screen->ro) {
            if (renderonly_get_handle(rsc->scanout, whandle)) {
         whandle->stride = rsc->slices[0].stride;
      }
      case WINSYS_HANDLE_TYPE_FD:
                        }
      static bool
   v3d_resource_get_param(struct pipe_screen *pscreen,
                           {
         struct v3d_resource *rsc =
            switch (param) {
   case PIPE_RESOURCE_PARAM_STRIDE:
               case PIPE_RESOURCE_PARAM_OFFSET:
               case PIPE_RESOURCE_PARAM_MODIFIER:
               case PIPE_RESOURCE_PARAM_NPLANES:
               default:
         }
      #define PAGE_UB_ROWS (V3D_UIFCFG_PAGE_SIZE / V3D_UIFBLOCK_ROW_SIZE)
   #define PAGE_UB_ROWS_TIMES_1_5 ((PAGE_UB_ROWS * 3) >> 1)
   #define PAGE_CACHE_UB_ROWS (V3D_PAGE_CACHE_SIZE / V3D_UIFBLOCK_ROW_SIZE)
   #define PAGE_CACHE_MINUS_1_5_UB_ROWS (PAGE_CACHE_UB_ROWS - PAGE_UB_ROWS_TIMES_1_5)
      /**
   * Computes the HW's UIFblock padding for a given height/cpp.
   *
   * The goal of the padding is to keep pages of the same color (bank number) at
   * least half a page away from each other vertically when crossing between
   * between columns of UIF blocks.
   */
   static uint32_t
   v3d_get_ub_pad(struct v3d_resource *rsc, uint32_t height)
   {
         uint32_t utile_h = v3d_utile_height(rsc->cpp);
   uint32_t uif_block_h = utile_h * 2;
                     /* For the perfectly-aligned-for-UIF-XOR case, don't add any pad. */
   if (height_offset_in_pc == 0)
            /* Try padding up to where we're offset by at least half a page. */
   if (height_offset_in_pc < PAGE_UB_ROWS_TIMES_1_5) {
            /* If we fit entirely in the page cache, don't pad. */
   if (height_ub < PAGE_CACHE_UB_ROWS)
                     /* If we're close to being aligned to page cache size, then round up
      * and rely on XOR.
      if (height_offset_in_pc > PAGE_CACHE_MINUS_1_5_UB_ROWS)
            /* Otherwise, we're far enough away (top and bottom) to not need any
      * padding.
      }
      /**
   * Computes the dimension with required padding for mip levels.
   *
   * This padding is required for width and height dimensions when the mip
   * level is greater than 1, and for the depth dimension when the mip level
   * is greater than 0. This function expects to be passed a mip level >= 1.
   *
   * Note: Hardware documentation seems to suggest that the third argument
   * should be the utile dimensions, but through testing it was found that
   * the block dimension should be used instead.
   */
   static uint32_t
   v3d_get_dimension_mpad(uint32_t dimension, uint32_t level, uint32_t block_dimension)
   {
         assert(level >= 1);
   uint32_t pot_dim = u_minify(dimension, 1);
   pot_dim = util_next_power_of_two(DIV_ROUND_UP(pot_dim, block_dimension));
   uint32_t padded_dim = block_dimension * pot_dim;
   }
      static void
   v3d_setup_slices(struct v3d_resource *rsc, uint32_t winsys_stride,
         {
         struct pipe_resource *prsc = &rsc->base;
   uint32_t width = prsc->width0;
   uint32_t height = prsc->height0;
   uint32_t depth = prsc->depth0;
   uint32_t offset = 0;
   uint32_t utile_w = v3d_utile_width(rsc->cpp);
   uint32_t utile_h = v3d_utile_height(rsc->cpp);
   uint32_t uif_block_w = utile_w * 2;
   uint32_t uif_block_h = utile_h * 2;
   uint32_t block_width = util_format_get_blockwidth(prsc->format);
            /* Note that power-of-two padding is based on level 1.  These are not
      * equivalent to just util_next_power_of_two(dimension), because at a
   * level 0 dimension of 9, the level 1 power-of-two padded value is 4,
   * not 8. Additionally the pot padding is based on the block size.
      uint32_t pot_width = 2 * v3d_get_dimension_mpad(width,
               uint32_t pot_height = 2 * v3d_get_dimension_mpad(height,
               uint32_t pot_depth = 2 * v3d_get_dimension_mpad(depth,
                        /* MSAA textures/renderbuffers are always laid out as single-level
      * UIF.
               /* Check some easy mistakes to make in a resource_create() call that
      * will break our setup.
      assert(prsc->array_size != 0);
            for (int i = prsc->last_level; i >= 0; i--) {
                     uint32_t level_width, level_height, level_depth;
   if (i < 2) {
               } else {
               }
   if (i < 1)
                        if (msaa) {
                                       if (!rsc->tiled) {
            slice->tiling = V3D_TILING_RASTER;
   if (prsc->target == PIPE_TEXTURE_1D ||
      } else {
            if ((i != 0 || !uif_top) &&
      (level_width <= utile_w ||
   level_height <= utile_h)) {
      slice->tiling = V3D_TILING_LINEARTILE;
   level_width = align(level_width, utile_w);
   } else if ((i != 0 || !uif_top) &&
               slice->tiling = V3D_TILING_UBLINEAR_1_COLUMN;
   level_width = align(level_width, uif_block_w);
   } else if ((i != 0 || !uif_top) &&
               slice->tiling = V3D_TILING_UBLINEAR_2_COLUMN;
   level_width = align(level_width, 2 * uif_block_w);
   } else {
         /* We align the width to a 4-block column of
      * UIF blocks, but we only align height to UIF
                                                            /* If the padding set us to to be aligned to
      * the page cache size, then the HW will use
   * the XOR bit on odd columns to get us
   * perfectly misaligned
      if ((level_height / uif_block_h) %
         (V3D_PAGE_CACHE_SIZE /
   V3D_UIFBLOCK_ROW_SIZE) == 0) {
                     slice->offset = offset;
   if (winsys_stride)
         else
                                 /* The HW aligns level 1's base to a page if any of level 1 or
   * below could be UIF XOR.  The lower levels then inherit the
   * alignment for as long as necessary, thanks to being power of
   * two aligned.
   */
   if (i == 1 &&
      level_width > 4 * uif_block_w &&
   level_height > PAGE_CACHE_MINUS_1_5_UB_ROWS * uif_block_h) {
                     }
            /* UIF/UBLINEAR levels need to be aligned to UIF-blocks, and LT only
      * needs to be aligned to utile boundaries.  Since tiles are laid out
   * from small to big in memory, we need to align the later UIF slices
   * to UIF blocks, if they were preceded by non-UIF-block-aligned LT
   * slices.
   *
   * We additionally align to 4k, which improves UIF XOR performance.
      uint32_t page_align_offset = (align(rsc->slices[0].offset, 4096) -
         if (page_align_offset) {
            rsc->size += page_align_offset;
               /* Arrays and cube textures have a stride which is the distance from
      * one full mipmap tree to the next (64b aligned).  For 3D textures,
   * we need to program the stride between slices of miplevel 0.
      if (prsc->target != PIPE_TEXTURE_3D) {
            rsc->cube_map_stride = align(rsc->slices[0].offset +
      } else {
         }
      uint32_t
   v3d_layer_offset(struct pipe_resource *prsc, uint32_t level, uint32_t layer)
   {
         struct v3d_resource *rsc = v3d_resource(prsc);
            if (prsc->target == PIPE_TEXTURE_3D)
         else
   }
      static struct v3d_resource *
   v3d_resource_setup(struct pipe_screen *pscreen,
         {
         struct v3d_screen *screen = v3d_screen(pscreen);
   struct v3d_device_info *devinfo = &screen->devinfo;
            if (!rsc)
                           pipe_reference_init(&prsc->reference, 1);
            if (prsc->nr_samples <= 1 ||
         devinfo->ver >= 40 ||
   util_format_is_depth_or_stencil(prsc->format)) {
      rsc->cpp = util_format_get_blocksize(prsc->format);
      } else {
            assert(v3d_rt_format_supported(devinfo, prsc->format));
   uint32_t output_image_format =
         uint32_t internal_type;
                        switch (internal_bpp) {
   case V3D_INTERNAL_BPP_32:
               case V3D_INTERNAL_BPP_64:
               case V3D_INTERNAL_BPP_128:
                                       }
      static struct pipe_resource *
   v3d_resource_create_with_modifiers(struct pipe_screen *pscreen,
                     {
                  bool linear_ok = drm_find_modifier(DRM_FORMAT_MOD_LINEAR, modifiers, count);
   struct v3d_resource *rsc = v3d_resource_setup(pscreen, tmpl);
   struct pipe_resource *prsc = &rsc->base;
   /* Use a tiled layout if we can, for better 3D performance. */
            assert(tmpl->target != PIPE_BUFFER ||
                  /* VBOs/PBOs/Texture Buffer Objects are untiled (and 1 height). */
   if (tmpl->target == PIPE_BUFFER)
            /* Cursors are always linear, and the user can request linear as well.
         if (tmpl->bind & (PIPE_BIND_LINEAR | PIPE_BIND_CURSOR))
            /* 1D and 1D_ARRAY textures are always raster-order. */
   if (tmpl->target == PIPE_TEXTURE_1D ||
                  /* Scanout BOs for simulator need to be linear for interaction with
      * i965.
      if (using_v3d_simulator &&
                  /* If using the old-school SCANOUT flag, we don't know what the screen
      * might support other than linear. Just force linear.
      if (tmpl->bind & PIPE_BIND_SCANOUT)
            /* No user-specified modifier; determine our own. */
   if (count == 1 && modifiers[0] == DRM_FORMAT_MOD_INVALID) {
               } else if (should_tile &&
               drm_find_modifier(DRM_FORMAT_MOD_BROADCOM_UIF,
   } else if (linear_ok) {
         } else {
                                          if (screen->ro && (tmpl->bind & PIPE_BIND_SCANOUT)) {
            struct winsys_handle handle;
   struct pipe_resource scanout_tmpl = {
            .target = prsc->target,
   .format = PIPE_FORMAT_RGBA8888_UNORM,
   .width0 = 1024, /* one page */
                     rsc->scanout =
                        if (!rsc->scanout) {
               }
                                             } else {
                        fail:
         v3d_resource_destroy(pscreen, prsc);
   }
      struct pipe_resource *
   v3d_resource_create(struct pipe_screen *pscreen,
         {
         const uint64_t mod = DRM_FORMAT_MOD_INVALID;
   }
      static struct pipe_resource *
   v3d_resource_from_handle(struct pipe_screen *pscreen,
                     {
         struct v3d_screen *screen = v3d_screen(pscreen);
   struct v3d_resource *rsc = v3d_resource_setup(pscreen, tmpl);
   struct pipe_resource *prsc = &rsc->base;
            if (!rsc)
            switch (whandle->modifier) {
   case DRM_FORMAT_MOD_LINEAR:
               case DRM_FORMAT_MOD_BROADCOM_UIF:
               case DRM_FORMAT_MOD_INVALID:
               default:
            switch(fourcc_mod_broadcom_mod(whandle->modifier)) {
   case DRM_FORMAT_MOD_BROADCOM_SAND128:
            rsc->tiled = false;
   rsc->sand_col128_stride =
      default:
            fprintf(stderr,
                  switch (whandle->type) {
   case WINSYS_HANDLE_TYPE_SHARED:
               case WINSYS_HANDLE_TYPE_FD:
               default:
            fprintf(stderr,
                     if (!rsc->bo)
                     v3d_setup_slices(rsc, whandle->stride, true);
            if (whandle->offset != 0) {
            if (rsc->tiled) {
            fprintf(stderr,
                           if (rsc->slices[0].offset + rsc->slices[0].size >
      rsc->bo->size) {
         fprintf(stderr, "Attempt to import "
         "with overflowing offset (%d + %d > %d)\n",
   whandle->offset,
            if (screen->ro) {
            /* Make sure that renderonly has a handle to our buffer in the
   * display's fd, so that a later renderonly_get_handle()
   * returns correct handles or GEM names.
   */
   rsc->scanout =
                     if (rsc->tiled && whandle->stride != slice->stride) {
            static bool warned = false;
   if (!warned) {
            warned = true;
   fprintf(stderr,
         "Attempting to import %dx%d %s with "
   "unsupported stride %d instead of %d\n",
   prsc->width0, prsc->height0,
         } else if (!rsc->tiled) {
                  /* Prevent implicit clearing of the imported buffer contents. */
               fail:
         v3d_resource_destroy(pscreen, prsc);
   }
      void
   v3d_update_shadow_texture(struct pipe_context *pctx,
         {
         struct v3d_context *v3d = v3d_context(pctx);
   struct v3d_sampler_view *view = v3d_sampler_view(pview);
   struct v3d_resource *shadow = v3d_resource(view->texture);
                     if (shadow->writes == orig->writes && orig->bo->private)
            perf_debug("Updating %dx%d@%d shadow for linear texture\n",
                  for (int i = 0; i <= shadow->base.last_level; i++) {
            unsigned width = u_minify(shadow->base.width0, i);
   unsigned height = u_minify(shadow->base.height0, i);
   struct pipe_blit_info info = {
            .dst = {
         .resource = &shadow->base,
   .level = i,
   .box = {
            .x = 0,
   .y = 0,
   .z = 0,
   .width = width,
      },
   },
   .src = {
         .resource = &orig->base,
   .level = pview->u.tex.first_level + i,
   .box = {
            .x = 0,
   .y = 0,
   .z = 0,
   .width = width,
      },
                  }
      static struct pipe_surface *
   v3d_create_surface(struct pipe_context *pctx,
               {
         struct v3d_context *v3d = v3d_context(pctx);
   struct v3d_screen *screen = v3d->screen;
   struct v3d_device_info *devinfo = &screen->devinfo;
   struct v3d_surface *surface = CALLOC_STRUCT(v3d_surface);
            if (!surface)
            struct pipe_surface *psurf = &surface->base;
   unsigned level = surf_tmpl->u.tex.level;
            pipe_reference_init(&psurf->reference, 1);
            psurf->context = pctx;
   psurf->format = surf_tmpl->format;
   psurf->width = u_minify(ptex->width0, level);
   psurf->height = u_minify(ptex->height0, level);
   psurf->u.tex.level = level;
   psurf->u.tex.first_layer = surf_tmpl->u.tex.first_layer;
            surface->offset = v3d_layer_offset(ptex, level,
                           const struct util_format_description *desc =
            surface->swap_rb = (desc->swizzle[0] == PIPE_SWIZZLE_Z &&
            if (util_format_is_depth_or_stencil(psurf->format)) {
            switch (psurf->format) {
   case PIPE_FORMAT_Z16_UNORM:
               case PIPE_FORMAT_Z32_FLOAT:
   case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
               default:
      } else {
            uint32_t bpp, type;
   v3d_X(devinfo, get_internal_type_bpp_for_output_format)
                     if (surface->tiling == V3D_TILING_UIF_NO_XOR ||
         surface->tiling == V3D_TILING_UIF_XOR) {
      surface->padded_height_of_output_image_in_uif_blocks =
               if (rsc->separate_stencil) {
            surface->separate_stencil =
               }
      static void
   v3d_surface_destroy(struct pipe_context *pctx, struct pipe_surface *psurf)
   {
                  if (surf->separate_stencil)
            pipe_resource_reference(&psurf->texture, NULL);
   }
      static void
   v3d_flush_resource(struct pipe_context *pctx, struct pipe_resource *resource)
   {
         /* All calls to flush_resource are followed by a flush of the context,
         }
      static enum pipe_format
   v3d_resource_get_internal_format(struct pipe_resource *prsc)
   {
         }
      static void
   v3d_resource_set_stencil(struct pipe_resource *prsc,
         {
         }
      static struct pipe_resource *
   v3d_resource_get_stencil(struct pipe_resource *prsc)
   {
                  }
      static const struct u_transfer_vtbl transfer_vtbl = {
         .resource_create          = v3d_resource_create,
   .resource_destroy         = v3d_resource_destroy,
   .transfer_map             = v3d_resource_transfer_map,
   .transfer_unmap           = v3d_resource_transfer_unmap,
   .transfer_flush_region    = u_default_transfer_flush_region,
   .get_internal_format      = v3d_resource_get_internal_format,
   .set_stencil              = v3d_resource_set_stencil,
   };
      void
   v3d_resource_screen_init(struct pipe_screen *pscreen)
   {
         pscreen->resource_create_with_modifiers =
         pscreen->resource_create = u_transfer_helper_resource_create;
   pscreen->resource_from_handle = v3d_resource_from_handle;
   pscreen->resource_get_handle = v3d_resource_get_handle;
   pscreen->resource_get_param = v3d_resource_get_param;
   pscreen->resource_destroy = u_transfer_helper_resource_destroy;
   pscreen->transfer_helper = u_transfer_helper_create(&transfer_vtbl,
         }
      void
   v3d_resource_context_init(struct pipe_context *pctx)
   {
         pctx->buffer_map = u_transfer_helper_transfer_map;
   pctx->texture_map = u_transfer_helper_transfer_map;
   pctx->transfer_flush_region = u_transfer_helper_transfer_flush_region;
   pctx->buffer_unmap = u_transfer_helper_transfer_unmap;
   pctx->texture_unmap = u_transfer_helper_transfer_unmap;
   pctx->buffer_subdata = u_default_buffer_subdata;
   pctx->texture_subdata = v3d_texture_subdata;
   pctx->create_surface = v3d_create_surface;
   pctx->surface_destroy = v3d_surface_destroy;
   pctx->resource_copy_region = util_resource_copy_region;
   pctx->blit = v3d_blit;
   pctx->generate_mipmap = v3d_generate_mipmap;
   }
