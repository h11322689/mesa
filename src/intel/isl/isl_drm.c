   /*
   * Copyright 2017 Intel Corporation
   *
   *  Permission is hereby granted, free of charge, to any person obtaining a
   *  copy of this software and associated documentation files (the "Software"),
   *  to deal in the Software without restriction, including without limitation
   *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
   *  and/or sell copies of the Software, and to permit persons to whom the
   *  Software is furnished to do so, subject to the following conditions:
   *
   *  The above copyright notice and this permission notice (including the next
   *  paragraph) shall be included in all copies or substantial portions of the
   *  Software.
   *
   *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   *  IN THE SOFTWARE.
   */
      #include <assert.h>
   #include <stdlib.h>
      #include "drm-uapi/drm_fourcc.h"
   #include "drm-uapi/i915_drm.h"
      #include "isl.h"
   #include "dev/intel_device_info.h"
   #include "dev/intel_debug.h"
      uint32_t
   isl_tiling_to_i915_tiling(enum isl_tiling tiling)
   {
      switch (tiling) {
   case ISL_TILING_LINEAR:
            case ISL_TILING_X:
            case ISL_TILING_Y0:
   case ISL_TILING_HIZ:
   case ISL_TILING_CCS:
            case ISL_TILING_W:
   case ISL_TILING_SKL_Yf:
   case ISL_TILING_SKL_Ys:
   case ISL_TILING_ICL_Yf:
   case ISL_TILING_ICL_Ys:
   case ISL_TILING_4:
   case ISL_TILING_64:
   case ISL_TILING_GFX12_CCS:
                     }
      enum isl_tiling
   isl_tiling_from_i915_tiling(uint32_t tiling)
   {
      switch (tiling) {
   case I915_TILING_NONE:
            case I915_TILING_X:
            case I915_TILING_Y:
                     }
      /** Sentinel is DRM_FORMAT_MOD_INVALID. */
   const struct isl_drm_modifier_info
   isl_drm_modifier_info_list[] = {
      {
      .modifier = DRM_FORMAT_MOD_NONE,
   .name = "DRM_FORMAT_MOD_NONE",
      },
   {
      .modifier = I915_FORMAT_MOD_X_TILED,
   .name = "I915_FORMAT_MOD_X_TILED",
      },
   {
      .modifier = I915_FORMAT_MOD_Y_TILED,
   .name = "I915_FORMAT_MOD_Y_TILED",
      },
   {
      .modifier = I915_FORMAT_MOD_Y_TILED_CCS,
   .name = "I915_FORMAT_MOD_Y_TILED_CCS",
   .tiling = ISL_TILING_Y0,
   .supports_render_compression = true,
      },
   {
      .modifier = I915_FORMAT_MOD_Y_TILED_GEN12_RC_CCS,
   .name = "I915_FORMAT_MOD_Y_TILED_GEN12_RC_CCS",
   .tiling = ISL_TILING_Y0,
   .supports_render_compression = true,
      },
   {
      .modifier = I915_FORMAT_MOD_Y_TILED_GEN12_MC_CCS,
   .name = "I915_FORMAT_MOD_Y_TILED_GEN12_MC_CCS",
   .tiling = ISL_TILING_Y0,
   .supports_media_compression = true,
      },
   {
      .modifier = I915_FORMAT_MOD_Y_TILED_GEN12_RC_CCS_CC,
   .name = "I915_FORMAT_MOD_Y_TILED_GEN12_RC_CCS_CC",
   .tiling = ISL_TILING_Y0,
   .supports_render_compression = true,
      },
   {
      .modifier = I915_FORMAT_MOD_4_TILED,
   .name = "I915_FORMAT_MOD_4_TILED",
      },
   {
      .modifier = I915_FORMAT_MOD_4_TILED_DG2_RC_CCS,
   .name = "I915_FORMAT_MOD_4_TILED_DG2_RC_CCS",
   .tiling = ISL_TILING_4,
   .supports_render_compression = true,
      },
   {
      .modifier = I915_FORMAT_MOD_4_TILED_DG2_MC_CCS,
   .name = "I915_FORMAT_MOD_4_TILED_DG2_MC_CCS",
   .tiling = ISL_TILING_4,
   .supports_media_compression = true,
      },
   {
      .modifier = I915_FORMAT_MOD_4_TILED_DG2_RC_CCS_CC,
   .name = "I915_FORMAT_MOD_4_TILED_DG2_RC_CCS_CC",
   .tiling = ISL_TILING_4,
   .supports_render_compression = true,
      },
   {
      .modifier = I915_FORMAT_MOD_4_TILED_MTL_RC_CCS,
   .name = "I915_FORMAT_MOD_4_TILED_MTL_RC_CCS",
   .tiling = ISL_TILING_4,
   .supports_render_compression = true,
      },
   {
      .modifier = I915_FORMAT_MOD_4_TILED_MTL_RC_CCS_CC,
   .name = "I915_FORMAT_MOD_4_TILED_MTL_RC_CCS_CC",
   .tiling = ISL_TILING_4,
   .supports_render_compression = true,
      },
   {
      .modifier = I915_FORMAT_MOD_4_TILED_MTL_MC_CCS,
   .name = "I915_FORMAT_MOD_4_TILED_MTL_MC_CCS",
   .tiling = ISL_TILING_4,
   .supports_media_compression = true,
      },
   {
            };
      const struct isl_drm_modifier_info *
   isl_drm_modifier_get_info(uint64_t modifier)
   {
      isl_drm_modifier_info_for_each(info) {
      if (info->modifier == modifier)
                  }
      uint32_t
   isl_drm_modifier_get_score(const struct intel_device_info *devinfo,
         {
      /* FINISHME: Add gfx12 modifiers */
   switch (modifier) {
   default:
         case DRM_FORMAT_MOD_LINEAR:
         case I915_FORMAT_MOD_X_TILED:
         case I915_FORMAT_MOD_Y_TILED:
      /* Gfx12.5 doesn't have Y-tiling. */
   if (devinfo->verx10 >= 125)
               case I915_FORMAT_MOD_Y_TILED_CCS:
      /* Not supported before Gfx9 and also Gfx12's CCS layout differs from
   * Gfx9-11.
   */
   if (devinfo->ver <= 8 || devinfo->ver >= 12)
            if (INTEL_DEBUG(DEBUG_NO_CCS))
               case I915_FORMAT_MOD_Y_TILED_GEN12_RC_CCS:
      if (devinfo->verx10 != 120)
            if (INTEL_DEBUG(DEBUG_NO_CCS))
               case I915_FORMAT_MOD_4_TILED:
      /* Gfx12.5 introduces Tile4. */
   if (devinfo->verx10 < 125)
               case I915_FORMAT_MOD_4_TILED_DG2_RC_CCS:
      if (!intel_device_info_is_dg2(devinfo))
            if (INTEL_DEBUG(DEBUG_NO_CCS))
               case I915_FORMAT_MOD_4_TILED_MTL_RC_CCS:
      if (!intel_device_info_is_mtl(devinfo))
            if (INTEL_DEBUG(DEBUG_NO_CCS))
                  }
      uint32_t
   isl_drm_modifier_get_plane_count(const struct intel_device_info *devinfo,
               {
      /* This function could return the wrong value if the modifier is not
   * supported by the device.
   */
            /* Planar images don't support clear color. */
   if (isl_drm_modifier_get_info(modifier)->supports_clear_color)
            if (devinfo->has_flat_ccs) {
      if (isl_drm_modifier_get_info(modifier)->supports_clear_color)
         else
      } else {
      if (isl_drm_modifier_get_info(modifier)->supports_clear_color)
         else if (isl_drm_modifier_has_aux(modifier))
         else
         }
