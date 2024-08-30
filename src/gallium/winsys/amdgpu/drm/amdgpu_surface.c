   /*
   * Copyright © 2011 Red Hat All Rights Reserved.
   * Copyright © 2014 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      #include "amdgpu_winsys.h"
   #include "util/format/u_format.h"
      static int amdgpu_surface_sanity(const struct pipe_resource *tex)
   {
      switch (tex->target) {
   case PIPE_TEXTURE_1D:
      if (tex->height0 > 1)
            case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_RECT:
      if (tex->depth0 > 1 || tex->array_size > 1)
            case PIPE_TEXTURE_3D:
      if (tex->array_size > 1)
            case PIPE_TEXTURE_1D_ARRAY:
      if (tex->height0 > 1)
            case PIPE_TEXTURE_CUBE:
   case PIPE_TEXTURE_2D_ARRAY:
   case PIPE_TEXTURE_CUBE_ARRAY:
      if (tex->depth0 > 1)
            default:
         }
      }
      static int amdgpu_surface_init(struct radeon_winsys *rws,
                                 {
      struct amdgpu_winsys *ws = amdgpu_winsys(rws);
            r = amdgpu_surface_sanity(tex);
   if (r)
            surf->blk_w = util_format_get_blockwidth(tex->format);
   surf->blk_h = util_format_get_blockheight(tex->format);
   surf->bpe = bpe;
                     config.info.width = tex->width0;
   config.info.height = tex->height0;
   config.info.depth = tex->depth0;
   config.info.array_size = tex->array_size;
   config.info.samples = tex->nr_samples;
   config.info.storage_samples = tex->nr_storage_samples;
   config.info.levels = tex->last_level + 1;
   config.info.num_channels = util_format_get_nr_components(tex->format);
   config.is_1d = tex->target == PIPE_TEXTURE_1D ||
         config.is_3d = tex->target == PIPE_TEXTURE_3D;
   config.is_cube = tex->target == PIPE_TEXTURE_CUBE;
   config.is_array = tex->target == PIPE_TEXTURE_1D_ARRAY ||
                  /* Use different surface counters for color and FMASK, so that MSAA MRTs
   * always use consecutive surface indices when FMASK is allocated between
   * them.
   */
   config.info.surf_index = &ws->surf_index_color;
            if (flags & RADEON_SURF_Z_OR_SBUFFER)
            /* Use radeon_info from the driver, not the winsys. The driver is allowed to change it. */
      }
      void amdgpu_surface_init_functions(struct amdgpu_screen_winsys *ws)
   {
         }
