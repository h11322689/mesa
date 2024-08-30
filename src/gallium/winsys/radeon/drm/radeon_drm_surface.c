   /*
   * Copyright © 2014 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      #include "radeon_drm_winsys.h"
   #include "util/format/u_format.h"
   #include <radeon_surface.h>
      static unsigned cik_get_macro_tile_index(struct radeon_surf *surf)
   {
               tileb = 8 * 8 * surf->bpe;
            for (index = 0; tileb > 64; index++)
            assert(index < 16);
      }
      #define   G_009910_MICRO_TILE_MODE(x)          (((x) >> 0) & 0x03)
   #define   G_009910_MICRO_TILE_MODE_NEW(x)      (((x) >> 22) & 0x07)
      static void set_micro_tile_mode(struct radeon_surf *surf,
         {
               if (info->gfx_level < GFX6) {
      surf->micro_tile_mode = 0;
                        if (info->gfx_level >= GFX7)
         else
      }
      static void surf_level_winsys_to_drm(struct radeon_surface_level *level_drm,
               {
      level_drm->offset = (uint64_t)level_ws->offset_256B * 256;
   level_drm->slice_size = (uint64_t)level_ws->slice_size_dw * 4;
   level_drm->nblk_x = level_ws->nblk_x;
   level_drm->nblk_y = level_ws->nblk_y;
   level_drm->pitch_bytes = level_ws->nblk_x * bpe;
      }
      static void surf_level_drm_to_winsys(struct legacy_surf_level *level_ws,
               {
      level_ws->offset_256B = level_drm->offset / 256;
   level_ws->slice_size_dw = level_drm->slice_size / 4;
   level_ws->nblk_x = level_drm->nblk_x;
   level_ws->nblk_y = level_drm->nblk_y;
   level_ws->mode = level_drm->mode;
      }
      static void surf_winsys_to_drm(struct radeon_surface *surf_drm,
                           {
                        surf_drm->npix_x = tex->width0;
   surf_drm->npix_y = tex->height0;
   surf_drm->npix_z = tex->depth0;
   surf_drm->blk_w = util_format_get_blockwidth(tex->format);
   surf_drm->blk_h = util_format_get_blockheight(tex->format);
   surf_drm->blk_d = 1;
   surf_drm->array_size = 1;
   surf_drm->last_level = tex->last_level;
   surf_drm->bpe = bpe;
            surf_drm->flags = flags;
   surf_drm->flags = RADEON_SURF_CLR(surf_drm->flags, TYPE);
   surf_drm->flags = RADEON_SURF_CLR(surf_drm->flags, MODE);
   surf_drm->flags |= RADEON_SURF_SET(mode, MODE) |
                  switch (tex->target) {
   case PIPE_TEXTURE_1D:
      surf_drm->flags |= RADEON_SURF_SET(RADEON_SURF_TYPE_1D, TYPE);
      case PIPE_TEXTURE_RECT:
   case PIPE_TEXTURE_2D:
      surf_drm->flags |= RADEON_SURF_SET(RADEON_SURF_TYPE_2D, TYPE);
      case PIPE_TEXTURE_3D:
      surf_drm->flags |= RADEON_SURF_SET(RADEON_SURF_TYPE_3D, TYPE);
      case PIPE_TEXTURE_1D_ARRAY:
      surf_drm->flags |= RADEON_SURF_SET(RADEON_SURF_TYPE_1D_ARRAY, TYPE);
   surf_drm->array_size = tex->array_size;
      case PIPE_TEXTURE_CUBE_ARRAY: /* cube array layout like 2d array */
      assert(tex->array_size % 6 == 0);
      case PIPE_TEXTURE_2D_ARRAY:
      surf_drm->flags |= RADEON_SURF_SET(RADEON_SURF_TYPE_2D_ARRAY, TYPE);
   surf_drm->array_size = tex->array_size;
      case PIPE_TEXTURE_CUBE:
      surf_drm->flags |= RADEON_SURF_SET(RADEON_SURF_TYPE_CUBEMAP, TYPE);
      case PIPE_BUFFER:
   default:
                  surf_drm->bo_size = surf_ws->surf_size;
            surf_drm->bankw = surf_ws->u.legacy.bankw;
   surf_drm->bankh = surf_ws->u.legacy.bankh;
   surf_drm->mtilea = surf_ws->u.legacy.mtilea;
            for (i = 0; i <= surf_drm->last_level; i++) {
      surf_level_winsys_to_drm(&surf_drm->level[i], &surf_ws->u.legacy.level[i],
                        if (flags & RADEON_SURF_SBUFFER) {
               for (i = 0; i <= surf_drm->last_level; i++) {
      surf_level_winsys_to_drm(&surf_drm->stencil_level[i],
                        }
      static void surf_drm_to_winsys(struct radeon_drm_winsys *ws,
               {
                        surf_ws->blk_w = surf_drm->blk_w;
   surf_ws->blk_h = surf_drm->blk_h;
   surf_ws->bpe = surf_drm->bpe;
   surf_ws->is_linear = surf_drm->level[0].mode <= RADEON_SURF_MODE_LINEAR_ALIGNED;
   surf_ws->has_stencil = !!(surf_drm->flags & RADEON_SURF_SBUFFER);
            surf_ws->surf_size = surf_drm->bo_size;
            surf_ws->u.legacy.bankw = surf_drm->bankw;
   surf_ws->u.legacy.bankh = surf_drm->bankh;
   surf_ws->u.legacy.mtilea = surf_drm->mtilea;
                     for (i = 0; i <= surf_drm->last_level; i++) {
      surf_level_drm_to_winsys(&surf_ws->u.legacy.level[i], &surf_drm->level[i],
                     if (surf_ws->flags & RADEON_SURF_SBUFFER) {
               for (i = 0; i <= surf_drm->last_level; i++) {
      surf_level_drm_to_winsys(&surf_ws->u.legacy.zs.stencil_level[i],
                              set_micro_tile_mode(surf_ws, &ws->info);
   surf_ws->is_displayable = surf_ws->is_linear ||
            }
      static void si_compute_cmask(const struct radeon_info *info,
               {
      unsigned pipe_interleave_bytes = info->pipe_interleave_bytes;
   unsigned num_pipes = info->num_tile_pipes;
            if (surf->flags & RADEON_SURF_Z_OR_SBUFFER)
                     switch (num_pipes) {
   case 2:
      cl_width = 32;
   cl_height = 16;
      case 4:
      cl_width = 32;
   cl_height = 32;
      case 8:
      cl_width = 64;
   cl_height = 32;
      case 16: /* Hawaii */
      cl_width = 64;
   cl_height = 64;
      default:
      assert(0);
                        unsigned width = align(surf->u.legacy.level[0].nblk_x, cl_width*8);
   unsigned height = align(surf->u.legacy.level[0].nblk_y, cl_height*8);
            /* Each element of CMASK is a nibble. */
            surf->u.legacy.color.cmask_slice_tile_max = (width * height) / (128*128);
   if (surf->u.legacy.color.cmask_slice_tile_max)
            unsigned num_layers;
   if (config->is_3d)
         else if (config->is_cube)
         else
            surf->cmask_alignment_log2 = util_logbase2(MAX2(256, base_align));
      }
      static void si_compute_htile(const struct radeon_info *info,
         {
      unsigned cl_width, cl_height, width, height;
   unsigned slice_elements, slice_bytes, pipe_interleave_bytes, base_align;
                     if (!(surf->flags & RADEON_SURF_Z_OR_SBUFFER) ||
      surf->flags & RADEON_SURF_NO_HTILE)
         /* Overalign HTILE on P2 configs to work around GPU hangs in
   * piglit/depthstencil-render-miplevels 585.
   *
   * This has been confirmed to help Kabini & Stoney, where the hangs
   * are always reproducible. I think I have seen the test hang
   * on Carrizo too, though it was very rare there.
   */
   if (info->gfx_level >= GFX7 && num_pipes < 4)
            switch (num_pipes) {
   case 1:
      cl_width = 32;
   cl_height = 16;
      case 2:
      cl_width = 32;
   cl_height = 32;
      case 4:
      cl_width = 64;
   cl_height = 32;
      case 8:
      cl_width = 64;
   cl_height = 64;
      case 16:
      cl_width = 128;
   cl_height = 64;
      default:
      assert(0);
               width = align(surf->u.legacy.level[0].nblk_x, cl_width * 8);
            slice_elements = (width * height) / (8 * 8);
            pipe_interleave_bytes = info->pipe_interleave_bytes;
            surf->meta_alignment_log2 = util_logbase2(base_align);
      }
      static int radeon_winsys_surface_init(struct radeon_winsys *rws,
                                 {
      struct radeon_drm_winsys *ws = (struct radeon_drm_winsys*)rws;
   struct radeon_surface surf_drm;
                     if (!(flags & (RADEON_SURF_IMPORTED | RADEON_SURF_FMASK))) {
      r = radeon_surface_best(ws->surf_man, &surf_drm);
   if (r)
               r = radeon_surface_init(ws->surf_man, &surf_drm);
   if (r)
                     /* Compute FMASK. */
   if (ws->gen == DRV_SI &&
      tex->nr_samples >= 2 &&
   !(flags & (RADEON_SURF_Z_OR_SBUFFER | RADEON_SURF_FMASK | RADEON_SURF_NO_FMASK))) {
   /* FMASK is allocated like an ordinary texture. */
   struct pipe_resource templ = *tex;
   struct radeon_surf fmask = {};
            templ.nr_samples = 1;
            switch (tex->nr_samples) {
   case 2:
   case 4:
      bpe = 1;
      case 8:
      bpe = 4;
      default:
      fprintf(stderr, "radeon: Invalid sample count for FMASK allocation.\n");
               if (radeon_winsys_surface_init(rws, info, &templ, fmask_flags, bpe,
            fprintf(stderr, "Got error in surface_init while allocating FMASK.\n");
                        surf_ws->fmask_size = fmask.surf_size;
   surf_ws->fmask_alignment_log2 = util_logbase2(MAX2(256, 1 << fmask.surf_alignment_log2));
            surf_ws->u.legacy.color.fmask.slice_tile_max =
         if (surf_ws->u.legacy.color.fmask.slice_tile_max)
            surf_ws->u.legacy.color.fmask.tiling_index = fmask.u.legacy.tiling_index[0];
   surf_ws->u.legacy.color.fmask.bankh = fmask.u.legacy.bankh;
               if (ws->gen == DRV_SI &&
      (tex->nr_samples <= 1 || surf_ws->fmask_size)) {
            /* Only these fields need to be set for the CMASK computation. */
   config.info.width = tex->width0;
   config.info.height = tex->height0;
   config.info.depth = tex->depth0;
   config.info.array_size = tex->array_size;
   config.is_3d = !!(tex->target == PIPE_TEXTURE_3D);
   config.is_cube = !!(tex->target == PIPE_TEXTURE_CUBE);
   config.is_array = tex->target == PIPE_TEXTURE_1D_ARRAY ||
                              if (ws->gen == DRV_SI) {
               /* Determine the memory layout of multiple allocations in one buffer. */
            if (surf_ws->meta_size) {
      surf_ws->meta_offset = align64(surf_ws->total_size, 1 << surf_ws->meta_alignment_log2);
               if (surf_ws->fmask_size) {
      assert(tex->nr_samples >= 2);
   surf_ws->fmask_offset = align64(surf_ws->total_size, 1 << surf_ws->fmask_alignment_log2);
               /* Single-sample CMASK is in a separate buffer. */
   if (surf_ws->cmask_size && tex->nr_samples >= 2) {
      surf_ws->cmask_offset = align64(surf_ws->total_size, 1 << surf_ws->cmask_alignment_log2);
                     }
      void radeon_surface_init_functions(struct radeon_drm_winsys *ws)
   {
         }
