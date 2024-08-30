   /*
   * Copyright © 2016 Red Hat.
   * Copyright © 2016 Bas Nieuwenhuizen
   *
   * based in part on anv driver which is:
   * Copyright © 2015 Intel Corporation
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
      #include "util/u_atomic.h"
   #include "util/u_debug.h"
   #include "vulkan/util/vk_format.h"
   #include "ac_drm_fourcc.h"
   #include "radv_debug.h"
   #include "radv_private.h"
   #include "radv_radeon_winsys.h"
   #include "sid.h"
   #include "vk_format.h"
   #include "vk_render_pass.h"
   #include "vk_util.h"
      #include "gfx10_format_table.h"
      static unsigned
   radv_choose_tiling(struct radv_device *device, const VkImageCreateInfo *pCreateInfo, VkFormat format)
   {
      if (pCreateInfo->tiling == VK_IMAGE_TILING_LINEAR) {
      assert(pCreateInfo->samples <= 1);
               if (pCreateInfo->usage & (VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR | VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR))
            /* MSAA resources must be 2D tiled. */
   if (pCreateInfo->samples > 1)
            if (!vk_format_is_compressed(format) && !vk_format_is_depth_or_stencil(format) &&
      device->physical_device->rad_info.gfx_level <= GFX8) {
   /* this causes hangs in some VK CTS tests on GFX9. */
   /* Textures with a very small height are recommended to be linear. */
   if (pCreateInfo->imageType == VK_IMAGE_TYPE_1D ||
      /* Only very thin and long 2D textures should benefit from
   * linear_aligned. */
   (pCreateInfo->extent.width > 8 && pCreateInfo->extent.height <= 2))
               }
      static bool
   radv_use_tc_compat_htile_for_image(struct radv_device *device, const VkImageCreateInfo *pCreateInfo, VkFormat format)
   {
      /* TC-compat HTILE is only available for GFX8+. */
   if (device->physical_device->rad_info.gfx_level < GFX8)
            /* TC-compat HTILE looks broken on Tonga (and Iceland is the same design) and the documented bug
   * workarounds don't help.
   */
   if (device->physical_device->rad_info.family == CHIP_TONGA ||
      device->physical_device->rad_info.family == CHIP_ICELAND)
         if (pCreateInfo->tiling == VK_IMAGE_TILING_LINEAR)
            /* Do not enable TC-compatible HTILE if the image isn't readable by a
   * shader because no texture fetches will happen.
   */
   if (!(pCreateInfo->usage &
                  if (device->physical_device->rad_info.gfx_level < GFX9) {
      /* TC-compat HTILE for MSAA depth/stencil images is broken
   * on GFX8 because the tiling doesn't match.
   */
   if (pCreateInfo->samples >= 2 && format == VK_FORMAT_D32_SFLOAT_S8_UINT)
            /* GFX9+ supports compression for both 32-bit and 16-bit depth
   * surfaces, while GFX8 only supports 32-bit natively. Though,
   * the driver allows TC-compat HTILE for 16-bit depth surfaces
   * with no Z planes compression.
   */
   if (format != VK_FORMAT_D32_SFLOAT_S8_UINT && format != VK_FORMAT_D32_SFLOAT && format != VK_FORMAT_D16_UNORM)
            /* TC-compat HTILE for layered images can have interleaved slices (see sliceInterleaved flag
   * in addrlib).  radv_clear_htile does not work.
   */
   if (pCreateInfo->arrayLayers > 1)
                  }
      static bool
   radv_surface_has_scanout(struct radv_device *device, const struct radv_image_create_info *info)
   {
      if (info->bo_metadata) {
      if (device->physical_device->rad_info.gfx_level >= GFX9)
         else
                  }
      static bool
   radv_image_use_fast_clear_for_image_early(const struct radv_device *device, const struct radv_image *image)
   {
      if (device->instance->debug_flags & RADV_DEBUG_FORCE_COMPRESS)
            if (image->vk.samples <= 1 && image->vk.extent.width * image->vk.extent.height <= 512 * 512) {
      /* Do not enable CMASK or DCC for small surfaces where the cost
   * of the eliminate pass can be higher than the benefit of fast
   * clear. RadeonSI does this, but the image threshold is
   * different.
   */
                  }
      static bool
   radv_image_use_fast_clear_for_image(const struct radv_device *device, const struct radv_image *image)
   {
      if (device->instance->debug_flags & RADV_DEBUG_FORCE_COMPRESS)
            return radv_image_use_fast_clear_for_image_early(device, image) && (image->exclusive ||
                              }
      bool
   radv_are_formats_dcc_compatible(const struct radv_physical_device *pdev, const void *pNext, VkFormat format,
         {
               if (!radv_is_colorbuffer_format_supported(pdev, format, &blendable))
            if (sign_reinterpret != NULL)
            if (flags & VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT) {
      const struct VkImageFormatListCreateInfo *format_list =
            /* We have to ignore the existence of the list if viewFormatCount = 0 */
   if (format_list && format_list->viewFormatCount) {
      /* compatibility is transitive, so we only need to check
   * one format with everything else. */
   for (unsigned i = 0; i < format_list->viewFormatCount; ++i) {
                     if (!radv_dcc_formats_compatible(pdev->rad_info.gfx_level, format, format_list->pViewFormats[i],
               } else {
                        }
      static bool
   radv_format_is_atomic_allowed(struct radv_device *device, VkFormat format)
   {
      if (format == VK_FORMAT_R32_SFLOAT && !device->image_float32_atomics)
               }
      static bool
   radv_formats_is_atomic_allowed(struct radv_device *device, const void *pNext, VkFormat format, VkImageCreateFlags flags)
   {
      if (radv_format_is_atomic_allowed(device, format))
            if (flags & VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT) {
      const struct VkImageFormatListCreateInfo *format_list =
            /* We have to ignore the existence of the list if viewFormatCount = 0 */
   if (format_list && format_list->viewFormatCount) {
      for (unsigned i = 0; i < format_list->viewFormatCount; ++i) {
      if (radv_format_is_atomic_allowed(device, format_list->pViewFormats[i]))
                        }
      static bool
   radv_use_dcc_for_image_early(struct radv_device *device, struct radv_image *image, const VkImageCreateInfo *pCreateInfo,
         {
      /* DCC (Delta Color Compression) is only available for GFX8+. */
   if (device->physical_device->rad_info.gfx_level < GFX8)
            if (device->instance->debug_flags & RADV_DEBUG_NO_DCC)
            if (image->shareable && image->vk.tiling != VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT)
            /*
   * TODO: Enable DCC for storage images on GFX9 and earlier.
   *
   * Also disable DCC with atomics because even when DCC stores are
   * supported atomics will always decompress. So if we are
   * decompressing a lot anyway we might as well not have DCC.
   */
   if ((pCreateInfo->usage & VK_IMAGE_USAGE_STORAGE_BIT) &&
      (device->physical_device->rad_info.gfx_level < GFX10 ||
   radv_formats_is_atomic_allowed(device, pCreateInfo->pNext, format, pCreateInfo->flags)))
         if (pCreateInfo->tiling == VK_IMAGE_TILING_LINEAR)
            if (vk_format_is_subsampled(format) || vk_format_get_plane_count(format) > 1)
            if (!radv_image_use_fast_clear_for_image_early(device, image) &&
      image->vk.tiling != VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT)
         /* Do not enable DCC for mipmapped arrays because performance is worse. */
   if (pCreateInfo->arrayLayers > 1 && pCreateInfo->mipLevels > 1)
            if (device->physical_device->rad_info.gfx_level < GFX10) {
      /* TODO: Add support for DCC MSAA on GFX8-9. */
   if (pCreateInfo->samples > 1 && !device->physical_device->dcc_msaa_allowed)
            /* TODO: Add support for DCC layers/mipmaps on GFX9. */
   if ((pCreateInfo->arrayLayers > 1 || pCreateInfo->mipLevels > 1) &&
      device->physical_device->rad_info.gfx_level == GFX9)
            /* DCC MSAA can't work on GFX10.3 and earlier without FMASK. */
   if (pCreateInfo->samples > 1 && device->physical_device->rad_info.gfx_level < GFX11 &&
      (device->instance->debug_flags & RADV_DEBUG_NO_FMASK))
         return radv_are_formats_dcc_compatible(device->physical_device, pCreateInfo->pNext, format, pCreateInfo->flags,
      }
      static bool
   radv_use_dcc_for_image_late(struct radv_device *device, struct radv_image *image)
   {
      if (!radv_image_has_dcc(image))
            if (image->vk.tiling == VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT)
            if (!radv_image_use_fast_clear_for_image(device, image))
            /* TODO: Fix storage images with DCC without DCC image stores.
   * Disabling it for now. */
   if ((image->vk.usage & VK_IMAGE_USAGE_STORAGE_BIT) && !radv_image_use_dcc_image_stores(device, image))
               }
      /*
   * Whether to enable image stores with DCC compression for this image. If
   * this function returns false the image subresource should be decompressed
   * before using it with image stores.
   *
   * Note that this can have mixed performance implications, see
   * https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/6796#note_643299
   *
   * This function assumes the image uses DCC compression.
   */
   bool
   radv_image_use_dcc_image_stores(const struct radv_device *device, const struct radv_image *image)
   {
         }
      /*
   * Whether to use a predicate to determine whether DCC is in a compressed
   * state. This can be used to avoid decompressing an image multiple times.
   */
   bool
   radv_image_use_dcc_predication(const struct radv_device *device, const struct radv_image *image)
   {
         }
      static inline bool
   radv_use_fmask_for_image(const struct radv_device *device, const struct radv_image *image)
   {
      return device->physical_device->use_fmask && image->vk.samples > 1 &&
            }
      static inline bool
   radv_use_htile_for_image(const struct radv_device *device, const struct radv_image *image)
   {
               /* TODO:
   * - Investigate about mips+layers.
   * - Enable on other gens.
   */
            /* Stencil texturing with HTILE doesn't work with mipmapping on Navi10-14. */
   if (device->physical_device->rad_info.gfx_level == GFX10 && image->vk.format == VK_FORMAT_D32_SFLOAT_S8_UINT &&
      image->vk.mip_levels > 1)
         /* Do not enable HTILE for very small images because it seems less performant but make sure it's
   * allowed with VRS attachments because we need HTILE on GFX10.3.
   */
   if (image->vk.extent.width * image->vk.extent.height < 8 * 8 &&
      !(device->instance->debug_flags & RADV_DEBUG_FORCE_COMPRESS) &&
   !(gfx_level == GFX10_3 && device->attachment_vrs_enabled))
            }
      static bool
   radv_use_tc_compat_cmask_for_image(struct radv_device *device, struct radv_image *image)
   {
      /* TC-compat CMASK is only available for GFX8+. */
   if (device->physical_device->rad_info.gfx_level < GFX8)
            /* GFX9 has issues when sample count is greater than 2 */
   if (device->physical_device->rad_info.gfx_level == GFX9 && image->vk.samples > 2)
            if (device->instance->debug_flags & RADV_DEBUG_NO_TC_COMPAT_CMASK)
            /* TC-compat CMASK with storage images is supported on GFX10+. */
   if ((image->vk.usage & VK_IMAGE_USAGE_STORAGE_BIT) && device->physical_device->rad_info.gfx_level < GFX10)
            /* Do not enable TC-compatible if the image isn't readable by a shader
   * because no texture fetches will happen.
   */
   if (!(image->vk.usage &
                  /* If the image doesn't have FMASK, it can't be fetchable. */
   if (!radv_image_has_fmask(image))
               }
      static uint32_t
   si_get_bo_metadata_word1(const struct radv_device *device)
   {
         }
      static bool
   radv_is_valid_opaque_metadata(const struct radv_device *device, const struct radeon_bo_metadata *md)
   {
      if (md->metadata[0] != 1 || md->metadata[1] != si_get_bo_metadata_word1(device))
            if (md->size_metadata < 40)
               }
      static void
   radv_patch_surface_from_metadata(struct radv_device *device, struct radeon_surf *surface,
         {
               if (device->physical_device->rad_info.gfx_level >= GFX9) {
      if (md->u.gfx9.swizzle_mode > 0)
         else
               } else {
      surface->u.legacy.pipe_config = md->u.legacy.pipe_config;
   surface->u.legacy.bankw = md->u.legacy.bankw;
   surface->u.legacy.bankh = md->u.legacy.bankh;
   surface->u.legacy.tile_split = md->u.legacy.tile_split;
   surface->u.legacy.mtilea = md->u.legacy.mtilea;
            if (md->u.legacy.macrotile == RADEON_LAYOUT_TILED)
         else if (md->u.legacy.microtile == RADEON_LAYOUT_TILED)
         else
         }
      static VkResult
   radv_patch_image_dimensions(struct radv_device *device, struct radv_image *image,
         {
      unsigned width = image->vk.extent.width;
            /*
   * minigbm sometimes allocates bigger images which is going to result in
   * weird strides and other properties. Lets be lenient where possible and
   * fail it on GFX10 (as we cannot cope there).
   *
   * Example hack: https://chromium-review.googlesource.com/c/chromiumos/platform/minigbm/+/1457777/
   */
   if (create_info->bo_metadata && radv_is_valid_opaque_metadata(device, create_info->bo_metadata)) {
               if (device->physical_device->rad_info.gfx_level >= GFX10) {
      width = G_00A004_WIDTH_LO(md->metadata[3]) + (G_00A008_WIDTH_HI(md->metadata[4]) << 2) + 1;
      } else {
      width = G_008F18_WIDTH(md->metadata[4]) + 1;
                  if (image->vk.extent.width == width && image->vk.extent.height == height)
            if (width < image->vk.extent.width || height < image->vk.extent.height) {
      fprintf(stderr,
         "The imported image has smaller dimensions than the internal\n"
   "dimensions. Using it is going to fail badly, so we reject\n"
   "this import.\n"
   "(internal dimensions: %d x %d, external dimensions: %d x %d)\n",
      } else if (device->physical_device->rad_info.gfx_level >= GFX10) {
      fprintf(stderr,
         "Tried to import an image with inconsistent width on GFX10.\n"
   "As GFX10 has no separate stride fields we cannot cope with\n"
   "an inconsistency in width and will fail this import.\n"
   "(internal dimensions: %d x %d, external dimensions: %d x %d)\n",
      } else {
      fprintf(stderr,
         "Tried to import an image with inconsistent width on pre-GFX10.\n"
   "As GFX10 has no separate stride fields we cannot cope with\n"
   "an inconsistency and would fail on GFX10.\n"
      }
   image_info->width = width;
               }
      static VkResult
   radv_patch_image_from_extra_info(struct radv_device *device, struct radv_image *image,
         {
      VkResult result = radv_patch_image_dimensions(device, image, create_info, image_info);
   if (result != VK_SUCCESS)
            for (unsigned plane = 0; plane < image->plane_count; ++plane) {
      if (create_info->bo_metadata) {
                  if (radv_surface_has_scanout(device, create_info)) {
      image->planes[plane].surface.flags |= RADEON_SURF_SCANOUT;
                              if (create_info->prime_blit_src && !device->physical_device->rad_info.sdma_supports_compression) {
      /* Older SDMA hw can't handle DCC */
         }
      }
      static VkFormat
   radv_image_get_plane_format(const struct radv_physical_device *pdev, const struct radv_image *image, unsigned plane)
   {
      if (radv_is_format_emulated(pdev, image->vk.format)) {
      if (plane == 0)
         if (vk_format_description(image->vk.format)->layout == UTIL_FORMAT_LAYOUT_ASTC)
         else
                  }
      static uint64_t
   radv_get_surface_flags(struct radv_device *device, struct radv_image *image, unsigned plane_id,
         {
      uint64_t flags;
   unsigned array_mode = radv_choose_tiling(device, pCreateInfo, image_format);
   VkFormat format = radv_image_get_plane_format(device->physical_device, image, plane_id);
   const struct util_format_description *desc = vk_format_description(format);
            is_depth = util_format_has_depth(desc);
                     switch (pCreateInfo->imageType) {
   case VK_IMAGE_TYPE_1D:
      if (pCreateInfo->arrayLayers > 1)
         else
            case VK_IMAGE_TYPE_2D:
      if (pCreateInfo->arrayLayers > 1)
         else
            case VK_IMAGE_TYPE_3D:
      flags |= RADEON_SURF_SET(RADEON_SURF_TYPE_3D, TYPE);
      default:
                  /* Required for clearing/initializing a specific layer on GFX8. */
            if (is_depth) {
               if (is_depth && is_stencil && device->physical_device->rad_info.gfx_level <= GFX8) {
                     /* RADV doesn't support stencil pitch adjustment. As a result there are some spec gaps that
   * are not covered by CTS.
   *
   * For D+S images with pitch constraints due to rendertarget usage it can happen that
   * sampling from mipmaps beyond the base level of the descriptor is broken as the pitch
   * adjustment can't be applied to anything beyond the first level.
   */
               if (radv_use_htile_for_image(device, image) && !(device->instance->debug_flags & RADV_DEBUG_NO_HIZ) &&
      !(flags & RADEON_SURF_NO_RENDER_TARGET)) {
   if (radv_use_tc_compat_htile_for_image(device, pCreateInfo, image_format))
      } else {
                     if (is_stencil)
            if (device->physical_device->rad_info.gfx_level >= GFX9 && pCreateInfo->imageType == VK_IMAGE_TYPE_3D &&
      vk_format_get_blocksizebits(image_format) == 128 && vk_format_is_compressed(image_format))
         if (!radv_use_dcc_for_image_early(device, image, pCreateInfo, image_format, &image->dcc_sign_reinterpret))
            if (!radv_use_fmask_for_image(device, image))
            if (pCreateInfo->flags & VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT) {
                  /* Disable DCC for VRS rate images because the hw can't handle compression. */
   if (pCreateInfo->usage & VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR)
         if (!(pCreateInfo->usage & (VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT)))
               }
      static inline unsigned
   si_tile_mode_index(const struct radv_image_plane *plane, unsigned level, bool stencil)
   {
      if (stencil)
         else
      }
      static unsigned
   radv_map_swizzle(unsigned swizzle)
   {
      switch (swizzle) {
   case PIPE_SWIZZLE_Y:
         case PIPE_SWIZZLE_Z:
         case PIPE_SWIZZLE_W:
         case PIPE_SWIZZLE_0:
         case PIPE_SWIZZLE_1:
         default: /* PIPE_SWIZZLE_X */
            }
      static void
   radv_compose_swizzle(const struct util_format_description *desc, const VkComponentMapping *mapping,
         {
      if (desc->format == PIPE_FORMAT_R64_UINT || desc->format == PIPE_FORMAT_R64_SINT) {
      /* 64-bit formats only support storage images and storage images
   * require identity component mappings. We use 32-bit
   * instructions to access 64-bit images, so we need a special
   * case here.
   *
   * The zw components are 1,0 so that they can be easily be used
   * by loads to create the w component, which has to be 0 for
   * NULL descriptors.
   */
   swizzle[0] = PIPE_SWIZZLE_X;
   swizzle[1] = PIPE_SWIZZLE_Y;
   swizzle[2] = PIPE_SWIZZLE_1;
      } else if (!mapping) {
      for (unsigned i = 0; i < 4; i++)
      } else if (desc->colorspace == UTIL_FORMAT_COLORSPACE_ZS) {
      const unsigned char swizzle_xxxx[4] = {PIPE_SWIZZLE_X, PIPE_SWIZZLE_0, PIPE_SWIZZLE_0, PIPE_SWIZZLE_1};
      } else {
            }
      void
   radv_make_texel_buffer_descriptor(struct radv_device *device, uint64_t va, VkFormat vk_format, unsigned offset,
         {
      const struct util_format_description *desc;
   unsigned stride;
   unsigned num_format, data_format;
   int first_non_void;
   enum pipe_swizzle swizzle[4];
            desc = vk_format_description(vk_format);
   first_non_void = vk_format_get_first_non_void_channel(vk_format);
                              if (device->physical_device->rad_info.gfx_level != GFX8 && stride) {
                  rsrc_word3 = S_008F0C_DST_SEL_X(radv_map_swizzle(swizzle[0])) | S_008F0C_DST_SEL_Y(radv_map_swizzle(swizzle[1])) |
            if (device->physical_device->rad_info.gfx_level >= GFX10) {
      const struct gfx10_format *fmt =
            /* OOB_SELECT chooses the out-of-bounds check.
   *
   * GFX10:
   *  - 0: (index >= NUM_RECORDS) || (offset >= STRIDE)
   *  - 1: index >= NUM_RECORDS
   *  - 2: NUM_RECORDS == 0
   *  - 3: if SWIZZLE_ENABLE:
   *          swizzle_address >= NUM_RECORDS
   *       else:
   *          offset >= NUM_RECORDS
   *
   * GFX11:
   *  - 0: (index >= NUM_RECORDS) || (offset+payload > STRIDE)
   *  - 1: index >= NUM_RECORDS
   *  - 2: NUM_RECORDS == 0
   *  - 3: if SWIZZLE_ENABLE && STRIDE:
   *          (index >= NUM_RECORDS) || ( offset+payload > STRIDE)
   *       else:
   *          offset+payload > NUM_RECORDS
   */
   rsrc_word3 |= S_008F0C_FORMAT(fmt->img_format) | S_008F0C_OOB_SELECT(V_008F0C_OOB_SELECT_STRUCTURED_WITH_OFFSET) |
      } else {
      num_format = radv_translate_buffer_numformat(desc, first_non_void);
            assert(data_format != V_008F0C_BUF_DATA_FORMAT_INVALID);
                        state[0] = va;
   state[1] = S_008F04_BASE_ADDRESS_HI(va >> 32) | S_008F04_STRIDE(stride);
   state[2] = range;
      }
      static void
   si_set_mutable_tex_desc_fields(struct radv_device *device, struct radv_image *image,
                           {
      struct radv_image_plane *plane = &image->planes[plane_id];
   struct radv_image_binding *binding = image->disjoint ? &image->bindings[plane_id] : &image->bindings[0];
   uint64_t gpu_address = binding->bo ? radv_buffer_get_va(binding->bo) + binding->offset : 0;
   uint64_t va = gpu_address;
   uint8_t swizzle = plane->surface.tile_swizzle;
   enum amd_gfx_level gfx_level = device->physical_device->rad_info.gfx_level;
   uint64_t meta_va = 0;
   if (gfx_level >= GFX9) {
      if (is_stencil)
         else
         if (nbc_view && nbc_view->valid) {
      va += nbc_view->base_address_offset;
         } else
            state[0] = va >> 8;
   if (gfx_level >= GFX9 || base_level_info->mode == RADEON_SURF_MODE_2D)
         state[1] &= C_008F14_BASE_ADDRESS_HI;
            if (gfx_level >= GFX8) {
      state[6] &= C_008F28_COMPRESSION_EN;
   state[7] = 0;
   if (!disable_compression && radv_dcc_enabled(image, first_level)) {
      meta_va = gpu_address + plane->surface.meta_offset;
                  unsigned dcc_tile_swizzle = swizzle << 8;
   dcc_tile_swizzle &= (1 << plane->surface.meta_alignment_log2) - 1;
      } else if (!disable_compression && radv_image_is_tc_compat_htile(image)) {
                  if (meta_va) {
      state[6] |= S_008F28_COMPRESSION_EN(1);
   if (gfx_level <= GFX9)
                  /* GFX10.3+ can set a custom pitch for 1D and 2D non-array, but it must be a multiple
   * of 256B.
   *
   * If an imported image is used with VK_IMAGE_VIEW_TYPE_2D_ARRAY, it may hang due to VM faults
   * because DEPTH means pitch with 2D, but it means depth with 2D array.
   */
   if (device->physical_device->rad_info.gfx_level >= GFX10_3 && plane->surface.u.gfx9.uses_custom_pitch) {
      assert((plane->surface.u.gfx9.surf_pitch * plane->surface.bpe) % 256 == 0);
   assert(image->vk.image_type == VK_IMAGE_TYPE_2D);
   assert(plane->surface.is_linear);
   assert(G_00A00C_TYPE(state[3]) == V_008F1C_SQ_RSRC_IMG_2D);
            /* Subsampled images have the pitch in the units of blocks. */
   if (plane->surface.blk_w == 2)
            state[4] &= C_00A010_DEPTH & C_00A010_PITCH_MSB;
   state[4] |= S_00A010_DEPTH(pitch - 1) | /* DEPTH contains low bits of PITCH. */
               if (gfx_level >= GFX10) {
               if (is_stencil) {
         } else {
                           if (meta_va) {
      struct gfx9_surf_meta_flags meta = {
      .rb_aligned = 1,
                                                            } else if (gfx_level == GFX9) {
      state[3] &= C_008F1C_SW_MODE;
            if (is_stencil) {
      state[3] |= S_008F1C_SW_MODE(plane->surface.u.gfx9.zs.stencil_swizzle_mode);
      } else {
      state[3] |= S_008F1C_SW_MODE(plane->surface.u.gfx9.swizzle_mode);
               state[5] &= C_008F24_META_DATA_ADDRESS & C_008F24_META_PIPE_ALIGNED & C_008F24_META_RB_ALIGNED;
   if (meta_va) {
      struct gfx9_surf_meta_flags meta = {
      .rb_aligned = 1,
                              state[5] |= S_008F24_META_DATA_ADDRESS(meta_va >> 40) | S_008F24_META_PIPE_ALIGNED(meta.pipe_aligned) |
         } else {
      /* GFX6-GFX8 */
   unsigned pitch = base_level_info->nblk_x * block_width;
            state[3] &= C_008F1C_TILING_INDEX;
   state[3] |= S_008F1C_TILING_INDEX(index);
   state[4] &= C_008F20_PITCH;
         }
      static unsigned
   radv_tex_dim(VkImageType image_type, VkImageViewType view_type, unsigned nr_layers, unsigned nr_samples,
         {
      if (view_type == VK_IMAGE_VIEW_TYPE_CUBE || view_type == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY)
            /* GFX9 allocates 1D textures as 2D. */
   if (gfx9 && image_type == VK_IMAGE_TYPE_1D)
         switch (image_type) {
   case VK_IMAGE_TYPE_1D:
         case VK_IMAGE_TYPE_2D:
      if (nr_samples > 1)
         else
      case VK_IMAGE_TYPE_3D:
      if (view_type == VK_IMAGE_VIEW_TYPE_3D)
         else
      default:
            }
      static unsigned
   gfx9_border_color_swizzle(const struct util_format_description *desc)
   {
               if (desc->format == PIPE_FORMAT_S8_UINT) {
      /* Swizzle of 8-bit stencil format is defined as _x__ but the hw expects XYZW. */
   assert(desc->swizzle[1] == PIPE_SWIZZLE_X);
               if (desc->swizzle[3] == PIPE_SWIZZLE_X) {
      /* For the pre-defined border color values (white, opaque
   * black, transparent black), the only thing that matters is
   * that the alpha channel winds up in the correct place
   * (because the RGB channels are all the same) so either of
   * these enumerations will work.
   */
   if (desc->swizzle[2] == PIPE_SWIZZLE_Y)
         else
      } else if (desc->swizzle[0] == PIPE_SWIZZLE_X) {
      if (desc->swizzle[1] == PIPE_SWIZZLE_Y)
         else
      } else if (desc->swizzle[1] == PIPE_SWIZZLE_X) {
         } else if (desc->swizzle[2] == PIPE_SWIZZLE_X) {
                     }
      bool
   vi_alpha_is_on_msb(const struct radv_device *device, const VkFormat format)
   {
      if (device->physical_device->rad_info.gfx_level >= GFX11)
                     if (device->physical_device->rad_info.gfx_level >= GFX10 && desc->nr_channels == 1)
               }
   /**
   * Build the sampler view descriptor for a texture (GFX10).
   */
   static void
   gfx10_make_texture_descriptor(struct radv_device *device, struct radv_image *image, bool is_storage_image,
                                 {
      const struct util_format_description *desc;
   enum pipe_swizzle swizzle[4];
   unsigned img_format;
                     /* For emulated ETC2 without alpha we need to override the format to a 3-componenent format, so
   * that border colors work correctly (alpha forced to 1). Since Vulkan has no such format,
   * this uses the Gallium formats to set the description. */
   if (image->vk.format == VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK && vk_format == VK_FORMAT_R8G8B8A8_UNORM) {
         } else if (image->vk.format == VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK && vk_format == VK_FORMAT_R8G8B8A8_SRGB) {
                  img_format =
                     if (img_create_flags & VK_IMAGE_CREATE_2D_VIEW_COMPATIBLE_BIT_EXT) {
      assert(image->vk.image_type == VK_IMAGE_TYPE_3D);
      } else {
      type = radv_tex_dim(image->vk.image_type, view_type, image->vk.array_layers, image->vk.samples, is_storage_image,
               if (type == V_008F1C_SQ_RSRC_IMG_1D_ARRAY) {
      height = 1;
      } else if (type == V_008F1C_SQ_RSRC_IMG_2D_ARRAY || type == V_008F1C_SQ_RSRC_IMG_2D_MSAA_ARRAY) {
      if (view_type != VK_IMAGE_VIEW_TYPE_3D)
      } else if (type == V_008F1C_SQ_RSRC_IMG_CUBE)
            state[0] = 0;
   state[1] = S_00A004_FORMAT(img_format) | S_00A004_WIDTH_LO(width - 1);
   state[2] = S_00A008_WIDTH_HI((width - 1) >> 2) | S_00A008_HEIGHT(height - 1) |
         state[3] = S_00A00C_DST_SEL_X(radv_map_swizzle(swizzle[0])) | S_00A00C_DST_SEL_Y(radv_map_swizzle(swizzle[1])) |
            S_00A00C_DST_SEL_Z(radv_map_swizzle(swizzle[2])) | S_00A00C_DST_SEL_W(radv_map_swizzle(swizzle[3])) |
   S_00A00C_BASE_LEVEL(image->vk.samples > 1 ? 0 : first_level) |
      /* Depth is the the last accessible layer on gfx9+. The hw doesn't need
   * to know the total number of layers.
   */
   state[4] =
         state[5] = S_00A014_ARRAY_PITCH(0) | S_00A014_PERF_MOD(4);
   state[6] = 0;
            if (img_create_flags & VK_IMAGE_CREATE_2D_VIEW_COMPATIBLE_BIT_EXT) {
               /* ARRAY_PITCH is only meaningful for 3D images, 0 means SRV, 1 means UAV.
   * In SRV mode, BASE_ARRAY is ignored and DEPTH is the last slice of mipmap level 0.
   * In UAV mode, BASE_ARRAY is the first slice and DEPTH is the last slice of the bound level.
   */
   state[4] &= C_00A010_DEPTH;
   state[4] |= S_00A010_DEPTH(!is_storage_image ? depth - 1 : u_minify(depth, first_level) - 1);
      } else if (sliced_3d) {
                        unsigned first_slice = sliced_3d->sliceOffset;
   unsigned slice_count = sliced_3d->sliceCount == VK_REMAINING_3D_SLICES_EXT
                        state[4] = 0;
   state[4] |= S_00A010_DEPTH(last_slice) | S_00A010_BASE_ARRAY(first_slice);
               unsigned max_mip = image->vk.samples > 1 ? util_logbase2(image->vk.samples) : image->vk.mip_levels - 1;
   if (nbc_view && nbc_view->valid)
            unsigned min_lod_clamped = radv_float_to_ufixed(CLAMP(min_lod, 0, 15), 8);
   if (device->physical_device->rad_info.gfx_level >= GFX11) {
      state[1] |= S_00A004_MAX_MIP(max_mip);
   state[5] |= S_00A014_MIN_LOD_LO(min_lod_clamped);
      } else {
      state[1] |= S_00A004_MIN_LOD(min_lod_clamped);
               if (radv_dcc_enabled(image, first_level)) {
      state[6] |=
      S_00A018_MAX_UNCOMPRESSED_BLOCK_SIZE(V_028C78_MAX_BLOCK_SIZE_256B) |
   S_00A018_MAX_COMPRESSED_BLOCK_SIZE(image->planes[0].surface.u.gfx9.color.dcc.max_compressed_block_size) |
            if (radv_image_get_iterate256(device, image)) {
                  /* Initialize the sampler view for FMASK. */
   if (fmask_state) {
      if (radv_image_has_fmask(image)) {
      uint64_t gpu_address = radv_buffer_get_va(image->bindings[0].bo);
                                    switch (image->vk.samples) {
   case 2:
      format = V_008F0C_GFX10_FORMAT_FMASK8_S2_F2;
      case 4:
      format = V_008F0C_GFX10_FORMAT_FMASK8_S4_F4;
      case 8:
      format = V_008F0C_GFX10_FORMAT_FMASK32_S8_F8;
      default:
                  fmask_state[0] = (va >> 8) | image->planes[0].surface.fmask_tile_swizzle;
   fmask_state[1] = S_00A004_BASE_ADDRESS_HI(va >> 40) | S_00A004_FORMAT(format) | S_00A004_WIDTH_LO(width - 1);
   fmask_state[2] =
         fmask_state[3] =
      S_00A00C_DST_SEL_X(V_008F1C_SQ_SEL_X) | S_00A00C_DST_SEL_Y(V_008F1C_SQ_SEL_X) |
   S_00A00C_DST_SEL_Z(V_008F1C_SQ_SEL_X) | S_00A00C_DST_SEL_W(V_008F1C_SQ_SEL_X) |
   S_00A00C_SW_MODE(image->planes[0].surface.u.gfx9.color.fmask_swizzle_mode) |
      fmask_state[4] = S_00A010_DEPTH(last_layer) | S_00A010_BASE_ARRAY(first_layer);
   fmask_state[5] = 0;
                                    fmask_state[6] |= S_00A018_COMPRESSION_EN(1);
   fmask_state[6] |= S_00A018_META_DATA_ADDRESS_LO(va >> 8);
         } else
         }
      /**
   * Build the sampler view descriptor for a texture (SI-GFX9)
   */
   static void
   si_make_texture_descriptor(struct radv_device *device, struct radv_image *image, bool is_storage_image,
                           {
      const struct util_format_description *desc;
   enum pipe_swizzle swizzle[4];
   int first_non_void;
                     /* For emulated ETC2 without alpha we need to override the format to a 3-componenent format, so
   * that border colors work correctly (alpha forced to 1). Since Vulkan has no such format,
   * this uses the Gallium formats to set the description. */
   if (image->vk.format == VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK && vk_format == VK_FORMAT_R8G8B8A8_UNORM) {
         } else if (image->vk.format == VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK && vk_format == VK_FORMAT_R8G8B8A8_SRGB) {
                                    num_format = radv_translate_tex_numformat(vk_format, desc, first_non_void);
   if (num_format == ~0) {
                  data_format = radv_translate_tex_dataformat(vk_format, desc, first_non_void);
   if (data_format == ~0) {
                  /* S8 with either Z16 or Z32 HTILE need a special format. */
   if (device->physical_device->rad_info.gfx_level == GFX9 && vk_format == VK_FORMAT_S8_UINT &&
      radv_image_is_tc_compat_htile(image)) {
   if (image->vk.format == VK_FORMAT_D32_SFLOAT_S8_UINT)
         else if (image->vk.format == VK_FORMAT_D16_UNORM_S8_UINT)
               if (device->physical_device->rad_info.gfx_level == GFX9 &&
      img_create_flags & VK_IMAGE_CREATE_2D_VIEW_COMPATIBLE_BIT_EXT) {
   assert(image->vk.image_type == VK_IMAGE_TYPE_3D);
      } else {
      type = radv_tex_dim(image->vk.image_type, view_type, image->vk.array_layers, image->vk.samples, is_storage_image,
               if (type == V_008F1C_SQ_RSRC_IMG_1D_ARRAY) {
      height = 1;
      } else if (type == V_008F1C_SQ_RSRC_IMG_2D_ARRAY || type == V_008F1C_SQ_RSRC_IMG_2D_MSAA_ARRAY) {
      if (view_type != VK_IMAGE_VIEW_TYPE_3D)
      } else if (type == V_008F1C_SQ_RSRC_IMG_CUBE)
            state[0] = 0;
   state[1] = (S_008F14_MIN_LOD(radv_float_to_ufixed(CLAMP(min_lod, 0, 15), 8)) | S_008F14_DATA_FORMAT(data_format) |
         state[2] = (S_008F18_WIDTH(width - 1) | S_008F18_HEIGHT(height - 1) | S_008F18_PERF_MOD(4));
   state[3] = (S_008F1C_DST_SEL_X(radv_map_swizzle(swizzle[0])) | S_008F1C_DST_SEL_Y(radv_map_swizzle(swizzle[1])) |
               S_008F1C_DST_SEL_Z(radv_map_swizzle(swizzle[2])) | S_008F1C_DST_SEL_W(radv_map_swizzle(swizzle[3])) |
   S_008F1C_BASE_LEVEL(image->vk.samples > 1 ? 0 : first_level) |
   state[4] = 0;
   state[5] = S_008F24_BASE_ARRAY(first_layer);
   state[6] = 0;
            if (device->physical_device->rad_info.gfx_level == GFX9) {
               /* Depth is the last accessible layer on Gfx9.
   * The hw doesn't need to know the total number of layers.
   */
   if (type == V_008F1C_SQ_RSRC_IMG_3D)
         else
            state[4] |= S_008F20_BC_SWIZZLE(bc_swizzle);
      } else {
      state[3] |= S_008F1C_POW2_PAD(image->vk.mip_levels > 1);
   state[4] |= S_008F20_DEPTH(depth - 1);
      }
   if (!(image->planes[0].surface.flags & RADEON_SURF_Z_OR_SBUFFER) && image->planes[0].surface.meta_offset) {
         } else {
      if (device->instance->disable_aniso_single_level) {
      /* The last dword is unused by hw. The shader uses it to clear
   * bits in the first dword of sampler state.
   */
   if (device->physical_device->rad_info.gfx_level <= GFX7 && image->vk.samples <= 1) {
      if (first_level == last_level)
         else
                     /* Initialize the sampler view for FMASK. */
   if (fmask_state) {
      if (radv_image_has_fmask(image)) {
      uint32_t fmask_format;
                                    if (device->physical_device->rad_info.gfx_level == GFX9) {
      fmask_format = V_008F14_IMG_DATA_FORMAT_FMASK;
   switch (image->vk.samples) {
   case 2:
      num_format = V_008F14_IMG_NUM_FORMAT_FMASK_8_2_2;
      case 4:
      num_format = V_008F14_IMG_NUM_FORMAT_FMASK_8_4_4;
      case 8:
      num_format = V_008F14_IMG_NUM_FORMAT_FMASK_32_8_8;
      default:
            } else {
      switch (image->vk.samples) {
   case 2:
      fmask_format = V_008F14_IMG_DATA_FORMAT_FMASK8_S2_F2;
      case 4:
      fmask_format = V_008F14_IMG_DATA_FORMAT_FMASK8_S4_F4;
      case 8:
      fmask_format = V_008F14_IMG_DATA_FORMAT_FMASK32_S8_F8;
      default:
      assert(0);
      }
               fmask_state[0] = va >> 8;
   fmask_state[0] |= image->planes[0].surface.fmask_tile_swizzle;
   fmask_state[1] =
         fmask_state[2] = S_008F18_WIDTH(width - 1) | S_008F18_HEIGHT(height - 1);
   fmask_state[3] =
      S_008F1C_DST_SEL_X(V_008F1C_SQ_SEL_X) | S_008F1C_DST_SEL_Y(V_008F1C_SQ_SEL_X) |
   S_008F1C_DST_SEL_Z(V_008F1C_SQ_SEL_X) | S_008F1C_DST_SEL_W(V_008F1C_SQ_SEL_X) |
      fmask_state[4] = 0;
   fmask_state[5] = S_008F24_BASE_ARRAY(first_layer);
                  if (device->physical_device->rad_info.gfx_level == GFX9) {
      fmask_state[3] |= S_008F1C_SW_MODE(image->planes[0].surface.u.gfx9.color.fmask_swizzle_mode);
   fmask_state[4] |=
                                    fmask_state[5] |= S_008F24_META_DATA_ADDRESS(va >> 40);
   fmask_state[6] |= S_008F28_COMPRESSION_EN(1);
         } else {
      fmask_state[3] |= S_008F1C_TILING_INDEX(image->planes[0].surface.u.legacy.color.fmask.tiling_index);
   fmask_state[4] |= S_008F20_DEPTH(depth - 1) |
                                    fmask_state[6] |= S_008F28_COMPRESSION_EN(1);
            } else
         }
      static void
   radv_make_texture_descriptor(struct radv_device *device, struct radv_image *image, bool is_storage_image,
                                 {
      if (device->physical_device->rad_info.gfx_level >= GFX10) {
      gfx10_make_texture_descriptor(device, image, is_storage_image, view_type, vk_format, mapping, first_level,
            } else {
      si_make_texture_descriptor(device, image, is_storage_image, view_type, vk_format, mapping, first_level,
               }
      static void
   radv_query_opaque_metadata(struct radv_device *device, struct radv_image *image, struct radeon_bo_metadata *md)
   {
      static const VkComponentMapping fixedmapping;
                     radv_make_texture_descriptor(device, image, false, (VkImageViewType)image->vk.image_type, image->vk.format,
                        si_set_mutable_tex_desc_fields(device, image, &image->planes[0].surface.u.legacy.level[0], 0, 0, 0,
            ac_surface_compute_umd_metadata(&device->physical_device->rad_info, &image->planes[0].surface, image->vk.mip_levels,
            }
      void
   radv_init_metadata(struct radv_device *device, struct radv_image *image, struct radeon_bo_metadata *metadata)
   {
                        if (device->physical_device->rad_info.gfx_level >= GFX9) {
      uint64_t dcc_offset =
         metadata->u.gfx9.swizzle_mode = surface->u.gfx9.swizzle_mode;
   metadata->u.gfx9.dcc_offset_256b = dcc_offset >> 8;
   metadata->u.gfx9.dcc_pitch_max = surface->u.gfx9.color.display_dcc_pitch_max;
   metadata->u.gfx9.dcc_independent_64b_blocks = surface->u.gfx9.color.dcc.independent_64B_blocks;
   metadata->u.gfx9.dcc_independent_128b_blocks = surface->u.gfx9.color.dcc.independent_128B_blocks;
   metadata->u.gfx9.dcc_max_compressed_block_size = surface->u.gfx9.color.dcc.max_compressed_block_size;
      } else {
      metadata->u.legacy.microtile =
         metadata->u.legacy.macrotile =
         metadata->u.legacy.pipe_config = surface->u.legacy.pipe_config;
   metadata->u.legacy.bankw = surface->u.legacy.bankw;
   metadata->u.legacy.bankh = surface->u.legacy.bankh;
   metadata->u.legacy.tile_split = surface->u.legacy.tile_split;
   metadata->u.legacy.mtilea = surface->u.legacy.mtilea;
   metadata->u.legacy.num_banks = surface->u.legacy.num_banks;
   metadata->u.legacy.stride = surface->u.legacy.level[0].nblk_x * surface->bpe;
      }
      }
      void
   radv_image_override_offset_stride(struct radv_device *device, struct radv_image *image, uint64_t offset,
         {
      ac_surface_override_offset_stride(&device->physical_device->rad_info, &image->planes[0].surface,
      }
      static void
   radv_image_alloc_single_sample_cmask(const struct radv_device *device, const struct radv_image *image,
         {
      if (!surf->cmask_size || surf->cmask_offset || surf->bpe > 8 || image->vk.mip_levels > 1 ||
      image->vk.extent.depth > 1 || radv_image_has_dcc(image) || !radv_image_use_fast_clear_for_image(device, image) ||
   (image->vk.create_flags & VK_IMAGE_CREATE_SPARSE_BINDING_BIT))
                  surf->cmask_offset = align64(surf->total_size, 1ull << surf->cmask_alignment_log2);
   surf->total_size = surf->cmask_offset + surf->cmask_size;
      }
      static void
   radv_image_alloc_values(const struct radv_device *device, struct radv_image *image)
   {
      /* images with modifiers can be potentially imported */
   if (image->vk.tiling == VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT)
            if (radv_image_has_cmask(image) || (radv_image_has_dcc(image) && !image->support_comp_to_single)) {
      image->fce_pred_offset = image->size;
               if (radv_image_use_dcc_predication(device, image)) {
      image->dcc_pred_offset = image->size;
               if ((radv_image_has_dcc(image) && !image->support_comp_to_single) || radv_image_has_cmask(image) ||
      radv_image_has_htile(image)) {
   image->clear_value_offset = image->size;
               if (radv_image_is_tc_compat_htile(image) && device->physical_device->rad_info.has_tc_compat_zrange_bug) {
      /* Metadata for the TC-compatible HTILE hardware bug which
   * have to be fixed by updating ZRANGE_PRECISION when doing
   * fast depth clears to 0.0f.
   */
   image->tc_compat_zrange_offset = image->size;
         }
      /* Determine if the image is affected by the pipe misaligned metadata issue
   * which requires to invalidate L2.
   */
   static bool
   radv_image_is_pipe_misaligned(const struct radv_device *device, const struct radv_image *image)
   {
      const struct radeon_info *rad_info = &device->physical_device->rad_info;
                     for (unsigned i = 0; i < image->plane_count; ++i) {
      VkFormat fmt = radv_image_get_plane_format(device->physical_device, image, i);
   int log2_bpp = util_logbase2(vk_format_get_blocksize(fmt));
            if (rad_info->gfx_level >= GFX10_3) {
         } else {
      if (vk_format_has_depth(image->vk.format) && image->vk.array_layers >= 8) {
                              int num_pipes = G_0098F8_NUM_PIPES(rad_info->gb_addr_config);
            if (vk_format_has_depth(image->vk.format)) {
      if (radv_image_is_tc_compat_htile(image) && overlap) {
            } else {
      int max_compressed_frags = G_0098F8_MAX_COMPRESSED_FRAGS(rad_info->gb_addr_config);
                  /* TODO: It shouldn't be necessary if the image has DCC but
   * not readable by shader.
   */
   if ((radv_image_has_dcc(image) || radv_image_is_tc_compat_cmask(image)) &&
      (samples_overlap > log2_samples_frag_diff)) {
                        }
      static bool
   radv_image_is_l2_coherent(const struct radv_device *device, const struct radv_image *image)
   {
      if (device->physical_device->rad_info.gfx_level >= GFX10) {
         } else if (device->physical_device->rad_info.gfx_level == GFX9) {
      if (image->vk.samples == 1 &&
      (image->vk.usage & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) &&
   !vk_format_has_stencil(image->vk.format)) {
   /* Single-sample color and single-sample depth
   * (not stencil) are coherent with shaders on
   * GFX9.
   */
                     }
      /**
   * Determine if the given image can be fast cleared.
   */
   static bool
   radv_image_can_fast_clear(const struct radv_device *device, const struct radv_image *image)
   {
      if (device->instance->debug_flags & RADV_DEBUG_NO_FAST_CLEARS)
            if (vk_format_is_color(image->vk.format)) {
      if (!radv_image_has_cmask(image) && !radv_image_has_dcc(image))
            /* RB+ doesn't work with CMASK fast clear on Stoney. */
   if (!radv_image_has_dcc(image) && device->physical_device->rad_info.family == CHIP_STONEY)
            /* Fast-clears with CMASK aren't supported for 128-bit formats. */
   if (radv_image_has_cmask(image) && vk_format_get_blocksizebits(image->vk.format) > 64)
      } else {
      if (!radv_image_has_htile(image))
               /* Do not fast clears 3D images. */
   if (image->vk.image_type == VK_IMAGE_TYPE_3D)
               }
      /**
   * Determine if the given image can be fast cleared using comp-to-single.
   */
   static bool
   radv_image_use_comp_to_single(const struct radv_device *device, const struct radv_image *image)
   {
      /* comp-to-single is only available for GFX10+. */
   if (device->physical_device->rad_info.gfx_level < GFX10)
            /* If the image can't be fast cleared, comp-to-single can't be used. */
   if (!radv_image_can_fast_clear(device, image))
            /* If the image doesn't have DCC, it can't be fast cleared using comp-to-single */
   if (!radv_image_has_dcc(image))
            /* It seems 8bpp and 16bpp require RB+ to work. */
   unsigned bytes_per_pixel = vk_format_get_blocksize(image->vk.format);
   if (bytes_per_pixel <= 2 && !device->physical_device->rad_info.rbplus_allowed)
               }
      static unsigned
   radv_get_internal_plane_count(const struct radv_physical_device *pdev, VkFormat fmt)
   {
      if (radv_is_format_emulated(pdev, fmt))
            }
      static void
   radv_image_reset_layout(const struct radv_physical_device *pdev, struct radv_image *image)
   {
      image->size = 0;
            image->tc_compatible_cmask = 0;
   image->fce_pred_offset = image->dcc_pred_offset = 0;
            unsigned plane_count = radv_get_internal_plane_count(pdev, image->vk.format);
   for (unsigned i = 0; i < plane_count; ++i) {
      VkFormat format = radv_image_get_plane_format(pdev, image, i);
   if (vk_format_has_depth(format))
            uint64_t flags = image->planes[i].surface.flags;
   uint64_t modifier = image->planes[i].surface.modifier;
            image->planes[i].surface.flags = flags;
   image->planes[i].surface.modifier = modifier;
   image->planes[i].surface.blk_w = vk_format_get_blockwidth(format);
   image->planes[i].surface.blk_h = vk_format_get_blockheight(format);
            /* align byte per element on dword */
   if (image->planes[i].surface.bpe == 3) {
               }
      struct ac_surf_info
   radv_get_ac_surf_info(struct radv_device *device, const struct radv_image *image)
   {
                        info.width = image->vk.extent.width;
   info.height = image->vk.extent.height;
   info.depth = image->vk.extent.depth;
   info.samples = image->vk.samples;
   info.storage_samples = image->vk.samples;
   info.array_size = image->vk.array_layers;
   info.levels = image->vk.mip_levels;
            if (!vk_format_is_depth_or_stencil(image->vk.format) && !image->shareable &&
      !(image->vk.create_flags & (VK_IMAGE_CREATE_SPARSE_ALIASED_BIT | VK_IMAGE_CREATE_ALIAS_BIT)) &&
   image->vk.tiling != VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT) {
                  }
      VkResult
   radv_image_create_layout(struct radv_device *device, struct radv_image_create_info create_info,
               {
      /* Clear the pCreateInfo pointer so we catch issues in the delayed case when we test in the
   * common internal case. */
            struct ac_surf_info image_info = radv_get_ac_surf_info(device, image);
   VkResult result = radv_patch_image_from_extra_info(device, image, &create_info, &image_info);
   if (result != VK_SUCCESS)
                              /*
   * Due to how the decoder works, the user can't supply an oversized image, because if it attempts
   * to sample it later with a linear filter, it will get garbage after the height it wants,
   * so we let the user specify the width/height unaligned, and align them preallocation.
   */
   if (image->vk.usage & (VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR | VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR)) {
      assert(profile_list);
   uint32_t width_align, height_align;
   radv_video_get_profile_alignments(device->physical_device, profile_list, &width_align, &height_align);
   image_info.width = align(image_info.width, width_align);
            if (radv_has_uvd(device->physical_device) && image->vk.usage & VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR) {
      /* UVD and kernel demand a full DPB allocation. */
                  unsigned plane_count = radv_get_internal_plane_count(device->physical_device, image->vk.format);
   for (unsigned plane = 0; plane < plane_count; ++plane) {
      struct ac_surf_info info = image_info;
   uint64_t offset;
            info.width = vk_format_get_plane_width(image->vk.format, plane, info.width);
            if (create_info.no_metadata_planes || plane_count > 1) {
                           if (plane == 0) {
      if (!radv_use_dcc_for_image_late(device, image))
               if (create_info.bo_metadata && !mod_info &&
      !ac_surface_apply_umd_metadata(&device->physical_device->rad_info, &image->planes[plane].surface,
                     if (!create_info.no_metadata_planes && !create_info.bo_metadata && plane_count == 1 && !mod_info)
            if (mod_info) {
      if (mod_info->pPlaneLayouts[plane].rowPitch % image->planes[plane].surface.bpe ||
                  offset = mod_info->pPlaneLayouts[plane].offset;
      } else {
      offset = image->disjoint ? 0 : align64(image->size, 1ull << image->planes[plane].surface.alignment_log2);
               if (!ac_surface_override_offset_stride(&device->physical_device->rad_info, &image->planes[plane].surface,
                  /* Validate DCC offsets in modifier layout. */
   if (plane_count == 1 && mod_info) {
      unsigned mem_planes = ac_surface_get_nplanes(&image->planes[plane].surface);
                  for (unsigned i = 1; i < mem_planes; ++i) {
      if (ac_surface_get_plane_offset(device->physical_device->rad_info.gfx_level, &image->planes[plane].surface,
                        image->size = MAX2(image->size, offset + image->planes[plane].surface.total_size);
                                                            assert(image->planes[0].surface.surf_size);
   assert(image->planes[0].surface.modifier == DRM_FORMAT_MOD_INVALID ||
            }
      static void
   radv_destroy_image(struct radv_device *device, const VkAllocationCallbacks *pAllocator, struct radv_image *image)
   {
      if ((image->vk.create_flags & VK_IMAGE_CREATE_SPARSE_BINDING_BIT) && image->bindings[0].bo) {
      radv_rmv_log_bo_destroy(device, image->bindings[0].bo);
               if (image->owned_memory != VK_NULL_HANDLE) {
      RADV_FROM_HANDLE(radv_device_memory, mem, image->owned_memory);
               radv_rmv_log_resource_destroy(device, (uint64_t)radv_image_to_handle(image));
   vk_image_finish(&image->vk);
      }
      static void
   radv_image_print_info(struct radv_device *device, struct radv_image *image)
   {
      fprintf(stderr, "Image:\n");
   fprintf(stderr,
         "  Info: size=%" PRIu64 ", alignment=%" PRIu32 ", "
   "width=%" PRIu32 ", height=%" PRIu32 ", depth=%" PRIu32 ", "
   "array_size=%" PRIu32 ", levels=%" PRIu32 "\n",
   image->size, image->alignment, image->vk.extent.width, image->vk.extent.height, image->vk.extent.depth,
   for (unsigned i = 0; i < image->plane_count; ++i) {
      const struct radv_image_plane *plane = &image->planes[i];
   const struct radeon_surf *surf = &plane->surface;
   const struct util_format_description *desc = vk_format_description(plane->format);
                           }
      static uint64_t
   radv_select_modifier(const struct radv_device *dev, VkFormat format,
         {
      const struct radv_physical_device *pdev = dev->physical_device;
                     /* We can allow everything here as it does not affect order and the application
   * is only allowed to specify modifiers that we support. */
   const struct ac_modifier_options modifier_options = {
      .dcc = true,
                                 /* If allocations fail, fall back to a dumber solution. */
   if (!mods)
                     for (unsigned i = 0; i < mod_count; ++i) {
      for (uint32_t j = 0; j < mod_list->drmFormatModifierCount; ++j) {
      if (mods[i] == mod_list->pDrmFormatModifiers[j]) {
      free(mods);
            }
      }
      VkResult
   radv_image_create(VkDevice _device, const struct radv_image_create_info *create_info,
         {
      RADV_FROM_HANDLE(radv_device, device, _device);
   const VkImageCreateInfo *pCreateInfo = create_info->vk_info;
   uint64_t modifier = DRM_FORMAT_MOD_INVALID;
   struct radv_image *image = NULL;
   VkFormat format = radv_select_android_external_format(pCreateInfo->pNext, pCreateInfo->format);
   const struct VkImageDrmFormatModifierListCreateInfoEXT *mod_list =
         const struct VkImageDrmFormatModifierExplicitCreateInfoEXT *explicit_mod =
         assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO);
   const struct VkVideoProfileListInfoKHR *profile_list =
                              radv_assert(pCreateInfo->mipLevels > 0);
   radv_assert(pCreateInfo->arrayLayers > 0);
   radv_assert(pCreateInfo->samples > 0);
   radv_assert(pCreateInfo->extent.width > 0);
   radv_assert(pCreateInfo->extent.height > 0);
            image = vk_zalloc2(&device->vk.alloc, alloc, image_struct_size, 8, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
   if (!image)
                     image->plane_count = vk_format_get_plane_count(format);
            image->exclusive = pCreateInfo->sharingMode == VK_SHARING_MODE_EXCLUSIVE;
   if (pCreateInfo->sharingMode == VK_SHARING_MODE_CONCURRENT) {
      for (uint32_t i = 0; i < pCreateInfo->queueFamilyIndexCount; ++i)
      if (pCreateInfo->pQueueFamilyIndices[i] == VK_QUEUE_FAMILY_EXTERNAL ||
      pCreateInfo->pQueueFamilyIndices[i] == VK_QUEUE_FAMILY_FOREIGN_EXT)
      else
                  const VkExternalMemoryImageCreateInfo *external_info =
                     if (mod_list)
         else if (explicit_mod)
            for (unsigned plane = 0; plane < plane_count; ++plane) {
      image->planes[plane].surface.flags = radv_get_surface_flags(device, image, plane, pCreateInfo, format);
                  #ifdef ANDROID
         #endif
            *pImage = radv_image_to_handle(image);
   assert(!(image->vk.create_flags & VK_IMAGE_CREATE_SPARSE_BINDING_BIT));
               VkResult result = radv_image_create_layout(device, *create_info, explicit_mod, profile_list, image);
   if (result != VK_SUCCESS) {
      radv_destroy_image(device, alloc, image);
               if (image->vk.create_flags & VK_IMAGE_CREATE_SPARSE_BINDING_BIT) {
      image->alignment = MAX2(image->alignment, 4096);
   image->size = align64(image->size, image->alignment);
            result = device->ws->buffer_create(device->ws, image->size, image->alignment, 0, RADEON_FLAG_VIRTUAL,
         if (result != VK_SUCCESS) {
      radv_destroy_image(device, alloc, image);
      }
               if (device->instance->debug_flags & RADV_DEBUG_IMG) {
                           radv_rmv_log_image_create(device, pCreateInfo, is_internal, *pImage);
   if (image->bindings[0].bo)
            }
      static inline void
   compute_non_block_compressed_view(struct radv_device *device, const struct radv_image_view *iview,
         {
      const struct radv_image *image = iview->image;
   const struct radeon_surf *surf = &image->planes[0].surface;
   struct ac_addrlib *addrlib = device->ws->get_addrlib(device->ws);
            ac_surface_compute_nbc_view(addrlib, &device->physical_device->rad_info, surf, &surf_info, iview->vk.base_mip_level,
      }
      static void
   radv_image_view_make_descriptor(struct radv_image_view *iview, struct radv_device *device, VkFormat vk_format,
                                 {
      struct radv_image *image = iview->image;
   struct radv_image_plane *plane = &image->planes[plane_id];
   bool is_stencil = iview->vk.aspects == VK_IMAGE_ASPECT_STENCIL_BIT;
   unsigned first_layer = iview->vk.base_array_layer;
   uint32_t blk_w;
   union radv_descriptor *descriptor;
            if (is_storage_image) {
         } else {
                  assert(vk_format_get_plane_count(vk_format) == 1);
   assert(plane->surface.blk_w % vk_format_get_blockwidth(plane->format) == 0);
            if (device->physical_device->rad_info.gfx_level >= GFX9) {
      hw_level = iview->vk.base_mip_level;
   if (nbc_view->valid) {
      hw_level = nbc_view->level;
                  /* Clear the base array layer because addrlib adds it as part of the base addr offset. */
                  radv_make_texture_descriptor(device, image, is_storage_image, iview->vk.view_type, vk_format, components, hw_level,
                              hw_level + iview->vk.level_count - 1, first_layer,
   iview->vk.base_array_layer + iview->vk.layer_count - 1,
         const struct legacy_surf_level *base_level_info = NULL;
   if (device->physical_device->rad_info.gfx_level <= GFX9) {
      if (is_stencil)
         else
               bool enable_write_compression = radv_image_use_dcc_image_stores(device, image);
   if (is_storage_image && !(enable_write_compression || enable_compression))
         si_set_mutable_tex_desc_fields(device, image, base_level_info, plane_id, iview->vk.base_mip_level,
                  }
      static unsigned
   radv_plane_from_aspect(VkImageAspectFlags mask)
   {
      switch (mask) {
   case VK_IMAGE_ASPECT_PLANE_1_BIT:
   case VK_IMAGE_ASPECT_MEMORY_PLANE_1_BIT_EXT:
         case VK_IMAGE_ASPECT_PLANE_2_BIT:
   case VK_IMAGE_ASPECT_MEMORY_PLANE_2_BIT_EXT:
         case VK_IMAGE_ASPECT_MEMORY_PLANE_3_BIT_EXT:
         default:
            }
      VkFormat
   radv_get_aspect_format(struct radv_image *image, VkImageAspectFlags mask)
   {
      switch (mask) {
   case VK_IMAGE_ASPECT_PLANE_0_BIT:
         case VK_IMAGE_ASPECT_PLANE_1_BIT:
         case VK_IMAGE_ASPECT_PLANE_2_BIT:
         case VK_IMAGE_ASPECT_STENCIL_BIT:
         case VK_IMAGE_ASPECT_DEPTH_BIT:
         case VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT:
         default:
            }
      /**
   * Determine if the given image view can be fast cleared.
   */
   static bool
   radv_image_view_can_fast_clear(const struct radv_device *device, const struct radv_image_view *iview)
   {
               if (!iview)
                  /* Only fast clear if the image itself can be fast cleared. */
   if (!radv_image_can_fast_clear(device, image))
            /* Only fast clear if all layers are bound. */
   if (iview->vk.base_array_layer > 0 || iview->vk.layer_count != image->vk.array_layers)
            /* Only fast clear if the view covers the whole image. */
   if (!radv_image_extent_compare(image, &iview->extent))
               }
      void
   radv_image_view_init(struct radv_image_view *iview, struct radv_device *device,
               {
      RADV_FROM_HANDLE(radv_image, image, pCreateInfo->image);
   const VkImageSubresourceRange *range = &pCreateInfo->subresourceRange;
   uint32_t plane_count = 1;
            const struct VkImageViewMinLodCreateInfoEXT *min_lod_info =
            if (min_lod_info)
            const struct VkImageViewSlicedCreateInfoEXT *sliced_3d =
            bool from_client = extra_create_info && extra_create_info->from_client;
            switch (image->vk.image_type) {
   case VK_IMAGE_TYPE_1D:
   case VK_IMAGE_TYPE_2D:
      assert(range->baseArrayLayer + vk_image_subresource_layer_count(&image->vk, range) - 1 <= image->vk.array_layers);
      case VK_IMAGE_TYPE_3D:
      assert(range->baseArrayLayer + vk_image_subresource_layer_count(&image->vk, range) - 1 <=
            default:
         }
   iview->image = image;
   iview->plane_id = radv_plane_from_aspect(pCreateInfo->subresourceRange.aspectMask);
            /* If the image has an Android external format, pCreateInfo->format will be
   * VK_FORMAT_UNDEFINED. */
   if (iview->vk.format == VK_FORMAT_UNDEFINED) {
      iview->vk.format = image->vk.format;
               /* Split out the right aspect. Note that for internal meta code we sometimes
   * use an equivalent color format for the aspect so we first have to check
   * if we actually got depth/stencil formats. */
   if (iview->vk.aspects == VK_IMAGE_ASPECT_STENCIL_BIT) {
      if (vk_format_has_stencil(iview->vk.view_format))
      } else if (iview->vk.aspects == VK_IMAGE_ASPECT_DEPTH_BIT) {
      if (vk_format_has_depth(iview->vk.view_format))
               if (vk_format_get_plane_count(image->vk.format) > 1 &&
      pCreateInfo->subresourceRange.aspectMask == VK_IMAGE_ASPECT_COLOR_BIT) {
               /* when the view format is emulated, redirect the view to the hidden plane 1 */
   if (radv_is_format_emulated(device->physical_device, iview->vk.format)) {
      assert(radv_is_format_emulated(device->physical_device, image->vk.format));
   iview->plane_id = 1;
   iview->vk.view_format = image->planes[iview->plane_id].format;
   iview->vk.format = image->planes[iview->plane_id].format;
               if (device->physical_device->rad_info.gfx_level >= GFX9) {
      iview->extent = (VkExtent3D){
      .width = image->vk.extent.width,
   .height = image->vk.extent.height,
         } else {
                  if (iview->vk.format != image->planes[iview->plane_id].format) {
      unsigned view_bw = vk_format_get_blockwidth(iview->vk.format);
   unsigned view_bh = vk_format_get_blockheight(iview->vk.format);
   unsigned img_bw = vk_format_get_blockwidth(image->planes[iview->plane_id].format);
            iview->extent.width = DIV_ROUND_UP(iview->extent.width * view_bw, img_bw);
            /* Comment ported from amdvlk -
   * If we have the following image:
   *              Uncompressed pixels   Compressed block sizes (4x4)
   *      mip0:       22 x 22                   6 x 6
   *      mip1:       11 x 11                   3 x 3
   *      mip2:        5 x  5                   2 x 2
   *      mip3:        2 x  2                   1 x 1
   *      mip4:        1 x  1                   1 x 1
   *
   * On GFX9 the descriptor is always programmed with the WIDTH and HEIGHT of the base level and
   * the HW is calculating the degradation of the block sizes down the mip-chain as follows
   * (straight-up divide-by-two integer math): mip0:  6x6 mip1:  3x3 mip2:  1x1 mip3:  1x1
   *
   * This means that mip2 will be missing texels.
   *
   * Fix this by calculating the base mip's width and height, then convert
   * that, and round it back up to get the level 0 size. Clamp the
   * converted size between the original values, and the physical extent
   * of the base mipmap.
   *
   * On GFX10 we have to take care to not go over the physical extent
   * of the base mipmap as otherwise the GPU computes a different layout.
   * Note that the GPU does use the same base-mip dimensions for both a
   * block compatible format and the compressed format, so even if we take
   * the plain converted dimensions the physical layout is correct.
   */
   if (device->physical_device->rad_info.gfx_level >= GFX9 &&
      vk_format_is_block_compressed(image->planes[iview->plane_id].format) &&
   !vk_format_is_block_compressed(iview->vk.format)) {
   /* If we have multiple levels in the view we should ideally take the last level,
   * but the mip calculation has a max(..., 1) so walking back to the base mip in an
   * useful way is hard. */
   if (iview->vk.level_count > 1) {
      iview->extent.width = iview->image->planes[0].surface.u.gfx9.base_mip_width;
      } else {
                                    iview->extent.width = CLAMP(lvl_width << range->baseMipLevel, iview->extent.width,
                        /* If the hardware-computed extent is still be too small, on GFX10
   * we can attempt another workaround provided by addrlib that
   * changes the descriptor's base level, and adjusts the address and
   * extents accordingly.
   */
   if (device->physical_device->rad_info.gfx_level >= GFX10 &&
      (radv_minify(iview->extent.width, range->baseMipLevel) < lvl_width ||
   radv_minify(iview->extent.height, range->baseMipLevel) < lvl_height) &&
   iview->vk.layer_count == 1) {
                        iview->support_fast_clear = radv_image_view_can_fast_clear(device, iview);
            bool disable_compression = extra_create_info ? extra_create_info->disable_compression : false;
   bool enable_compression = extra_create_info ? extra_create_info->enable_compression : false;
   for (unsigned i = 0; i < plane_count; ++i) {
      VkFormat format = vk_format_get_plane_format(iview->vk.view_format, i);
   radv_image_view_make_descriptor(iview, device, format, &pCreateInfo->components, min_lod, false,
               radv_image_view_make_descriptor(iview, device, format, &pCreateInfo->components, min_lod, true,
               }
      void
   radv_image_view_finish(struct radv_image_view *iview)
   {
         }
      bool
   radv_layout_is_htile_compressed(const struct radv_device *device, const struct radv_image *image, VkImageLayout layout,
         {
      switch (layout) {
   case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
   case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
   case VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL:
   case VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL:
         case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
      return radv_image_is_tc_compat_htile(image) ||
      case VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR:
   case VK_IMAGE_LAYOUT_GENERAL:
      /* It should be safe to enable TC-compat HTILE with
   * VK_IMAGE_LAYOUT_GENERAL if we are not in a render loop and
   * if the image doesn't have the storage bit set. This
   * improves performance for apps that use GENERAL for the main
   * depth pass because this allows compression and this reduces
   * the number of decompressions from/to GENERAL.
   */
   if (radv_image_is_tc_compat_htile(image) && queue_mask & (1u << RADV_QUEUE_GENERAL) &&
      !device->instance->disable_tc_compat_htile_in_general) {
      } else {
            case VK_IMAGE_LAYOUT_ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT:
      /* Do not compress HTILE with feedback loops because we can't read&write it without
   * introducing corruption.
   */
      case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
   case VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL:
      if (radv_image_is_tc_compat_htile(image) ||
      (radv_image_has_htile(image) &&
   !(image->vk.usage & (VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)))) {
   /* Keep HTILE compressed if the image is only going to
   * be used as a depth/stencil read-only attachment.
   */
      } else {
         }
      default:
            }
      bool
   radv_layout_can_fast_clear(const struct radv_device *device, const struct radv_image *image, unsigned level,
         {
      if (radv_dcc_enabled(image, level) && !radv_layout_dcc_compressed(device, image, level, layout, queue_mask))
            if (!(image->vk.usage & RADV_IMAGE_USAGE_WRITE_BITS))
            if (layout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && layout != VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL)
            /* Exclusive images with CMASK or DCC can always be fast-cleared on the gfx queue. Concurrent
   * images can only be fast-cleared if comp-to-single is supported because we don't yet support
   * FCE on the compute queue.
   */
      }
      bool
   radv_layout_dcc_compressed(const struct radv_device *device, const struct radv_image *image, unsigned level,
         {
      if (!radv_dcc_enabled(image, level))
            if (image->vk.tiling == VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT && queue_mask & (1u << RADV_QUEUE_FOREIGN))
            /* If the image is read-only, we can always just keep it compressed */
   if (!(image->vk.usage & RADV_IMAGE_USAGE_WRITE_BITS))
            /* Don't compress compute transfer dst when image stores are not supported. */
   if ((layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL || layout == VK_IMAGE_LAYOUT_GENERAL) &&
      (queue_mask & (1u << RADV_QUEUE_COMPUTE)) && !radv_image_use_dcc_image_stores(device, image))
         if (layout == VK_IMAGE_LAYOUT_ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT) {
      /* Do not compress DCC with feedback loops because we can't read&write it without introducing
   * corruption.
   */
                  }
      enum radv_fmask_compression
   radv_layout_fmask_compression(const struct radv_device *device, const struct radv_image *image, VkImageLayout layout,
         {
      if (!radv_image_has_fmask(image))
            if (layout == VK_IMAGE_LAYOUT_GENERAL)
            /* Don't compress compute transfer dst because image stores ignore FMASK and it needs to be
   * expanded before.
   */
   if (layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && (queue_mask & (1u << RADV_QUEUE_COMPUTE)))
            /* Compress images if TC-compat CMASK is enabled. */
   if (radv_image_is_tc_compat_cmask(image))
            switch (layout) {
   case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
   case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
      /* Don't compress images but no need to expand FMASK. */
      case VK_IMAGE_LAYOUT_ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT:
      /* Don't compress images that are in feedback loops. */
      default:
      /* Don't compress images that are concurrent. */
         }
      unsigned
   radv_image_queue_family_mask(const struct radv_image *image, enum radv_queue_family family,
         {
      if (!image->exclusive)
         if (family == RADV_QUEUE_FOREIGN)
         if (family == RADV_QUEUE_IGNORED)
            }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_CreateImage(VkDevice _device, const VkImageCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator,
         {
   #ifdef ANDROID
               if (gralloc_info)
      #endif
      #ifdef RADV_USE_WSI_PLATFORM
      /* Ignore swapchain creation info on Android. Since we don't have an implementation in Mesa,
   * we're guaranteed to access an Android object incorrectly.
   */
   RADV_FROM_HANDLE(radv_device, device, _device);
   const VkImageSwapchainCreateInfoKHR *swapchain_info =
         if (swapchain_info && swapchain_info->swapchain != VK_NULL_HANDLE) {
      return wsi_common_create_swapchain_image(device->physical_device->vk.wsi_device, pCreateInfo,
         #endif
         const struct wsi_image_create_info *wsi_info = vk_find_struct_const(pCreateInfo->pNext, WSI_IMAGE_CREATE_INFO_MESA);
   bool scanout = wsi_info && wsi_info->scanout;
            return radv_image_create(_device,
                           &(struct radv_image_create_info){
      }
      VKAPI_ATTR void VKAPI_CALL
   radv_DestroyImage(VkDevice _device, VkImage _image, const VkAllocationCallbacks *pAllocator)
   {
      RADV_FROM_HANDLE(radv_device, device, _device);
            if (!image)
               }
      VKAPI_ATTR void VKAPI_CALL
   radv_GetImageSubresourceLayout2KHR(VkDevice _device, VkImage _image, const VkImageSubresource2KHR *pSubresource,
         {
      RADV_FROM_HANDLE(radv_image, image, _image);
   RADV_FROM_HANDLE(radv_device, device, _device);
   int level = pSubresource->imageSubresource.mipLevel;
            unsigned plane_id = 0;
   if (vk_format_get_plane_count(image->vk.format) > 1)
            struct radv_image_plane *plane = &image->planes[plane_id];
            if (image->vk.tiling == VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT) {
               assert(level == 0);
            pLayout->subresourceLayout.offset =
         pLayout->subresourceLayout.rowPitch =
         pLayout->subresourceLayout.arrayPitch = 0;
   pLayout->subresourceLayout.depthPitch = 0;
      } else if (device->physical_device->rad_info.gfx_level >= GFX9) {
               pLayout->subresourceLayout.offset =
      ac_surface_get_plane_offset(device->physical_device->rad_info.gfx_level, &plane->surface, 0, layer) +
      if (image->vk.format == VK_FORMAT_R32G32B32_UINT || image->vk.format == VK_FORMAT_R32G32B32_SINT ||
      image->vk.format == VK_FORMAT_R32G32B32_SFLOAT) {
   /* Adjust the number of bytes between each row because
   * the pitch is actually the number of components per
   * row.
   */
      } else {
               assert(util_is_power_of_two_nonzero(surface->bpe));
               pLayout->subresourceLayout.arrayPitch = surface->u.gfx9.surf_slice_size;
   pLayout->subresourceLayout.depthPitch = surface->u.gfx9.surf_slice_size;
   pLayout->subresourceLayout.size = surface->u.gfx9.surf_slice_size;
   if (image->vk.image_type == VK_IMAGE_TYPE_3D)
      } else {
      pLayout->subresourceLayout.offset = (uint64_t)surface->u.legacy.level[level].offset_256B * 256 +
         pLayout->subresourceLayout.rowPitch = surface->u.legacy.level[level].nblk_x * surface->bpe;
   pLayout->subresourceLayout.arrayPitch = (uint64_t)surface->u.legacy.level[level].slice_size_dw * 4;
   pLayout->subresourceLayout.depthPitch = (uint64_t)surface->u.legacy.level[level].slice_size_dw * 4;
   pLayout->subresourceLayout.size = (uint64_t)surface->u.legacy.level[level].slice_size_dw * 4;
   if (image->vk.image_type == VK_IMAGE_TYPE_3D)
         }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_GetImageDrmFormatModifierPropertiesEXT(VkDevice _device, VkImage _image,
         {
               pProperties->drmFormatModifier = image->planes[0].surface.modifier;
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_CreateImageView(VkDevice _device, const VkImageViewCreateInfo *pCreateInfo,
         {
      RADV_FROM_HANDLE(radv_image, image, pCreateInfo->image);
   RADV_FROM_HANDLE(radv_device, device, _device);
            view = vk_alloc2(&device->vk.alloc, pAllocator, sizeof(*view), 8, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
   if (view == NULL)
            radv_image_view_init(view, device, pCreateInfo, image->vk.create_flags,
                        }
      VKAPI_ATTR void VKAPI_CALL
   radv_DestroyImageView(VkDevice _device, VkImageView _iview, const VkAllocationCallbacks *pAllocator)
   {
      RADV_FROM_HANDLE(radv_device, device, _device);
            if (!iview)
            radv_image_view_finish(iview);
      }
      void
   radv_buffer_view_init(struct radv_buffer_view *view, struct radv_device *device,
         {
      RADV_FROM_HANDLE(radv_buffer, buffer, pCreateInfo->buffer);
                                 }
      void
   radv_buffer_view_finish(struct radv_buffer_view *view)
   {
         }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_CreateBufferView(VkDevice _device, const VkBufferViewCreateInfo *pCreateInfo,
         {
      RADV_FROM_HANDLE(radv_device, device, _device);
            view = vk_alloc2(&device->vk.alloc, pAllocator, sizeof(*view), 8, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
   if (!view)
                                 }
      VKAPI_ATTR void VKAPI_CALL
   radv_DestroyBufferView(VkDevice _device, VkBufferView bufferView, const VkAllocationCallbacks *pAllocator)
   {
      RADV_FROM_HANDLE(radv_device, device, _device);
            if (!view)
            radv_buffer_view_finish(view);
      }
