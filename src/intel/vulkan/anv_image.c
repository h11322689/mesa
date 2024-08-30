   /*
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
      #include <assert.h>
   #include <stdbool.h>
   #include <string.h>
   #include <unistd.h>
   #include <fcntl.h>
   #include <sys/mman.h>
   #include "drm-uapi/drm_fourcc.h"
      #include "anv_private.h"
   #include "common/intel_aux_map.h"
   #include "util/u_debug.h"
   #include "vk_util.h"
   #include "util/u_math.h"
      #include "vk_format.h"
      #define ANV_OFFSET_IMPLICIT UINT64_MAX
      static const enum isl_surf_dim
   vk_to_isl_surf_dim[] = {
      [VK_IMAGE_TYPE_1D] = ISL_SURF_DIM_1D,
   [VK_IMAGE_TYPE_2D] = ISL_SURF_DIM_2D,
      };
      static uint64_t MUST_CHECK UNUSED
   memory_range_end(struct anv_image_memory_range memory_range)
   {
      assert(anv_is_aligned(memory_range.offset, memory_range.alignment));
      }
      /**
   * Get binding for VkImagePlaneMemoryRequirementsInfo,
   * VkBindImagePlaneMemoryInfo and VkDeviceImageMemoryRequirements.
   */
   struct anv_image_binding *
   anv_image_aspect_to_binding(struct anv_image *image,
         {
                        if (image->vk.tiling == VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT) {
      /* Spec requires special aspects for modifier images. */
   assert(aspect == VK_IMAGE_ASPECT_MEMORY_PLANE_0_BIT_EXT ||
         aspect == VK_IMAGE_ASPECT_MEMORY_PLANE_1_BIT_EXT ||
            /* We don't advertise DISJOINT for modifiers with aux, and therefore we
   * don't handle queries of the modifier's "aux plane" here.
   */
            switch(aspect) {
      case VK_IMAGE_ASPECT_MEMORY_PLANE_0_BIT_EXT: plane = 0; break;
   case VK_IMAGE_ASPECT_MEMORY_PLANE_1_BIT_EXT: plane = 1; break;
   case VK_IMAGE_ASPECT_MEMORY_PLANE_2_BIT_EXT: plane = 2; break;
         } else {
                     }
      /**
   * Extend the memory binding's range by appending a new memory range with `size`
   * and `alignment` at `offset`. Return the appended range.
   *
   * Offset is ignored if ANV_OFFSET_IMPLICIT.
   *
   * The given binding must not be ANV_IMAGE_MEMORY_BINDING_MAIN. The function
   * converts to MAIN as needed.
   */
   static VkResult MUST_CHECK
   image_binding_grow(const struct anv_device *device,
                     struct anv_image *image,
   enum anv_image_memory_binding binding,
   uint64_t offset,
   {
      /* We overwrite 'offset' but need to remember if it was implicit. */
            assert(size > 0);
            switch (binding) {
   case ANV_IMAGE_MEMORY_BINDING_MAIN:
      /* The caller must not pre-translate BINDING_PLANE_i to BINDING_MAIN. */
      case ANV_IMAGE_MEMORY_BINDING_PLANE_0:
   case ANV_IMAGE_MEMORY_BINDING_PLANE_1:
   case ANV_IMAGE_MEMORY_BINDING_PLANE_2:
      if (!image->disjoint)
            case ANV_IMAGE_MEMORY_BINDING_PRIVATE:
      assert(offset == ANV_OFFSET_IMPLICIT);
      case ANV_IMAGE_MEMORY_BINDING_END:
                  struct anv_image_memory_range *container =
            if (has_implicit_offset) {
         } else {
      /* Offset must be validated because it comes from
   * VkImageDrmFormatModifierExplicitCreateInfoEXT.
   */
   if (unlikely(!anv_is_aligned(offset, alignment))) {
      return vk_errorf(device,
                              /* Surfaces can be added out of memory-order. Track the end of each memory
   * plane to update the binding size properly.
   */
   uint64_t memory_range_end;
   if (__builtin_add_overflow(offset, size, &memory_range_end)) {
      if (has_implicit_offset) {
      assert(!"overflow");
   return vk_errorf(device, VK_ERROR_UNKNOWN,
      } else {
      return vk_errorf(device,
                              container->size = MAX2(container->size, memory_range_end);
            *out_range = (struct anv_image_memory_range) {
      .binding = binding,
   .offset = offset,
   .size = size,
                  }
      /**
   * Adjust range 'a' to contain range 'b'.
   *
   * For simplicity's sake, the offset of 'a' must be 0 and remains 0.
   * If 'a' and 'b' target different bindings, then no merge occurs.
   */
   static void
   memory_range_merge(struct anv_image_memory_range *a,
         {
      if (b.size == 0)
            if (a->binding != b.binding)
            assert(a->offset == 0);
   assert(anv_is_aligned(a->offset, a->alignment));
            a->alignment = MAX2(a->alignment, b.alignment);
      }
      isl_surf_usage_flags_t
   anv_image_choose_isl_surf_usage(VkImageCreateFlags vk_create_flags,
                     {
               if (vk_usage & VK_IMAGE_USAGE_SAMPLED_BIT)
            if (vk_usage & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)
            if (vk_usage & VK_IMAGE_USAGE_STORAGE_BIT)
            if (vk_usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
            if (vk_usage & VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR)
            if (vk_create_flags & VK_IMAGE_CREATE_SPARSE_BINDING_BIT)
      isl_usage |= ISL_SURF_USAGE_SPARSE_BIT |
         if (vk_usage & VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR ||
      vk_usage & VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR)
         if (vk_create_flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)
            if (vk_create_flags & (VK_IMAGE_CREATE_2D_VIEW_COMPATIBLE_BIT_EXT |
                  /* Even if we're only using it for transfer operations, clears to depth and
   * stencil images happen as depth and stencil so they need the right ISL
   * usage bits or else things will fall apart.
   */
   switch (aspect) {
   case VK_IMAGE_ASPECT_DEPTH_BIT:
      isl_usage |= ISL_SURF_USAGE_DEPTH_BIT;
      case VK_IMAGE_ASPECT_STENCIL_BIT:
      isl_usage |= ISL_SURF_USAGE_STENCIL_BIT;
      case VK_IMAGE_ASPECT_COLOR_BIT:
   case VK_IMAGE_ASPECT_PLANE_0_BIT:
   case VK_IMAGE_ASPECT_PLANE_1_BIT:
   case VK_IMAGE_ASPECT_PLANE_2_BIT:
         default:
                  if (vk_usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
      /* blorp implements transfers by sampling from the source image. */
               if (vk_usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT &&
      aspect == VK_IMAGE_ASPECT_COLOR_BIT) {
   /* blorp implements transfers by rendering into the destination image.
   * Only request this with color images, as we deal with depth/stencil
   * formats differently. */
                  }
      static isl_tiling_flags_t
   choose_isl_tiling_flags(const struct intel_device_info *devinfo,
                     {
      const VkImageCreateInfo *base_info = anv_info->vk_info;
            assert((isl_mod_info != NULL) ==
            switch (base_info->tiling) {
   default:
         case VK_IMAGE_TILING_OPTIMAL:
      flags = ISL_TILING_ANY_MASK;
      case VK_IMAGE_TILING_LINEAR:
      flags = ISL_TILING_LINEAR_BIT;
      case VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT:
                  if (anv_info->isl_tiling_flags) {
      assert(isl_mod_info == NULL);
               if (legacy_scanout) {
      isl_tiling_flags_t legacy_mask = ISL_TILING_LINEAR_BIT;
   if (devinfo->has_tiling_uapi)
                                 }
      /**
   * Add the surface to the binding at the given offset.
   *
   * \see image_binding_grow()
   */
   static VkResult MUST_CHECK
   add_surface(struct anv_device *device,
               struct anv_image *image,
   struct anv_surface *surf,
   {
      /* isl surface must be initialized */
            return image_binding_grow(device, image, binding, offset,
                  }
      static bool
   can_fast_clear_with_non_zero_color(const struct intel_device_info *devinfo,
                     {
      /* If we don't have an AUX surface where fast clears apply, we can return
   * early.
   */
   if (!isl_aux_usage_has_fast_clears(image->planes[plane].aux_usage))
            /* On TGL (< C0), if a block of fragment shader outputs match the surface's
   * clear color, the HW may convert them to fast-clears (see HSD 1607794140).
   * This can lead to rendering corruptions if not handled properly. We
   * restrict the clear color to zero to avoid issues that can occur with:
   *     - Texture view rendering (including blorp_copy calls)
   *     - Images with multiple levels or array layers
   */
   if (image->planes[plane].aux_usage == ISL_AUX_USAGE_FCV_CCS_E)
            /* Turning on non zero fast clears for CCS_E introduces a performance
   * regression for games such as F1 22 and RDR2 by introducing additional
   * partial resolves. Let's turn non zero fast clears back off till we can
   * fix performance.
   */
   if (image->planes[plane].aux_usage == ISL_AUX_USAGE_CCS_E &&
      devinfo->ver >= 12)
         /* Non mutable image, we can fast clear with any color supported by HW.
   */
   if (!(image->vk.create_flags & VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT))
            /* Mutable image with no format list, we have to assume all formats */
   if (!fmt_list || fmt_list->viewFormatCount == 0)
                     /* Check bit compatibility for clear color components */
   for (uint32_t i = 0; i < fmt_list->viewFormatCount; i++) {
      if (fmt_list->pViewFormats[i] == VK_FORMAT_UNDEFINED)
            struct anv_format_plane view_format_plane =
                           if (!isl_formats_have_same_bits_per_channel(img_format, view_format))
                  }
      /**
   * Return true if the storage image could be used with atomics.
   *
   * If the image was created with an explicit format, we check it for typed
   * atomic support.  If MUTABLE_FORMAT_BIT is set, then we check the optional
   * format list, seeing if /any/ of the formats support typed atomics.  If no
   * list is supplied, we fall back to using the bpb, as the application could
   * make an image view with a format that does use atomics.
   */
   static bool
   storage_image_format_supports_atomic(const struct intel_device_info *devinfo,
                           {
      if (isl_format_supports_typed_atomics(devinfo, format))
            if (!(create_flags & VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT))
            if (fmt_list) {
      for (uint32_t i = 0; i < fmt_list->viewFormatCount; i++) {
                     enum isl_format view_format =
                  if (isl_format_supports_typed_atomics(devinfo, view_format))
                           /* No explicit format list.  Any 16/32/64bpp format could be used with atomics. */
   unsigned bpb = isl_format_get_layout(format)->bpb;
      }
      static enum isl_format
   anv_get_isl_format_with_usage(const struct intel_device_info *devinfo,
                           {
      assert(util_bitcount(vk_usage) == 1);
   struct anv_format_plane format =
      anv_get_format_aspect(devinfo, vk_format, vk_aspect,
            }
      static bool
   formats_ccs_e_compatible(const struct intel_device_info *devinfo,
                           {
      if (!anv_format_supports_ccs_e(devinfo, format))
            /* For images created without MUTABLE_FORMAT_BIT set, we know that they will
   * always be used with the original format. In particular, they will always
   * be used with a format that supports color compression.
   */
   if (!(create_flags & VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT))
            if (!fmt_list || fmt_list->viewFormatCount == 0)
            for (uint32_t i = 0; i < fmt_list->viewFormatCount; i++) {
      if (fmt_list->pViewFormats[i] == VK_FORMAT_UNDEFINED)
            enum isl_format view_format =
      anv_get_isl_format_with_usage(devinfo, fmt_list->pViewFormats[i],
               if (!isl_formats_are_ccs_e_compatible(devinfo, format, view_format))
                  }
      bool
   anv_format_supports_ccs_e(const struct intel_device_info *devinfo,
         {
      /* CCS_E for YCRCB_NORMAL and YCRCB_SWAP_UV is not currently supported by
   * ANV so leave it disabled for now.
   */
   if (isl_format_is_yuv(format))
               }
      bool
   anv_formats_ccs_e_compatible(const struct intel_device_info *devinfo,
                           {
      enum isl_format format =
      anv_get_isl_format_with_usage(devinfo, vk_format,
               if (!formats_ccs_e_compatible(devinfo, create_flags, format, vk_tiling,
                  if (vk_usage & VK_IMAGE_USAGE_STORAGE_BIT) {
      if (devinfo->verx10 < 125)
            enum isl_format lower_format =
      anv_get_isl_format_with_usage(devinfo, vk_format,
               if (!isl_formats_are_ccs_e_compatible(devinfo, format, lower_format))
            if (!formats_ccs_e_compatible(devinfo, create_flags, format, vk_tiling,
                  /* Disable compression when surface can be potentially used for atomic
   * operation.
   */
   if (storage_image_format_supports_atomic(devinfo, create_flags, format,
                        }
      /**
   * For color images that have an auxiliary surface, request allocation for an
   * additional buffer that mainly stores fast-clear values. Use of this buffer
   * allows us to access the image's subresources while being aware of their
   * fast-clear values in non-trivial cases (e.g., outside of a render pass in
   * which a fast clear has occurred).
   *
   * In order to avoid having multiple clear colors for a single plane of an
   * image (hence a single RENDER_SURFACE_STATE), we only allow fast-clears on
   * the first slice (level 0, layer 0).  At the time of our testing (Jan 17,
   * 2018), there were no known applications which would benefit from fast-
   * clearing more than just the first slice.
   *
   * The fast clear portion of the image is laid out in the following order:
   *
   *  * 1 or 4 dwords (depending on hardware generation) for the clear color
   *  * 1 dword for the anv_fast_clear_type of the clear color
   *  * On gfx9+, 1 dword per level and layer of the image (3D levels count
   *    multiple layers) in level-major order for compression state.
   *
   * For the purpose of discoverability, the algorithm used to manage
   * compression and fast-clears is described here:
   *
   *  * On a transition from UNDEFINED or PREINITIALIZED to a defined layout,
   *    all of the values in the fast clear portion of the image are initialized
   *    to default values.
   *
   *  * On fast-clear, the clear value is written into surface state and also
   *    into the buffer and the fast clear type is set appropriately.  Both
   *    setting the fast-clear value in the buffer and setting the fast-clear
   *    type happen from the GPU using MI commands.
   *
   *  * Whenever a render or blorp operation is performed with CCS_E, we call
   *    genX(cmd_buffer_mark_image_written) to set the compression state to
   *    true (which is represented by UINT32_MAX).
   *
   *  * On pipeline barrier transitions, the worst-case transition is computed
   *    from the image layouts.  The command streamer inspects the fast clear
   *    type and compression state dwords and constructs a predicate.  The
   *    worst-case resolve is performed with the given predicate and the fast
   *    clear and compression state is set accordingly.
   *
   * See anv_layout_to_aux_usage and anv_layout_to_fast_clear_type functions for
   * details on exactly what is allowed in what layouts.
   *
   * On gfx7-9, we do not have a concept of indirect clear colors in hardware.
   * In order to deal with this, we have to do some clear color management.
   *
   *  * For LOAD_OP_LOAD at the top of a renderpass, we have to copy the clear
   *    value from the buffer into the surface state with MI commands.
   *
   *  * For any blorp operations, we pass the address to the clear value into
   *    blorp and it knows to copy the clear color.
   */
   static VkResult MUST_CHECK
   add_aux_state_tracking_buffer(struct anv_device *device,
               {
      assert(image && device);
   assert(image->planes[plane].aux_usage != ISL_AUX_USAGE_NONE &&
                  const unsigned clear_color_state_size = device->info->ver >= 10 ?
      device->isl_dev.ss.clear_color_state_size :
         /* Clear color and fast clear type */
            /* We only need to track compression on CCS_E surfaces. */
   if (isl_aux_usage_has_ccs_e(image->planes[plane].aux_usage)) {
      if (image->vk.image_type == VK_IMAGE_TYPE_3D) {
      for (uint32_t l = 0; l < image->vk.mip_levels; l++)
      } else {
                     enum anv_image_memory_binding binding =
            /* If an auxiliary surface is used for an externally-shareable image,
   * we have to hide this from the memory of the image since other
   * processes with access to the memory may not be aware of it or of
   * its current state. So put that auxiliary data into a separate
   * buffer (ANV_IMAGE_MEMORY_BINDING_PRIVATE).
   */
   if (anv_image_is_externally_shared(image)) {
                  /* We believe that 256B alignment may be sufficient, but we choose 4K due to
   * lack of testing.  And MI_LOAD/STORE operations require dword-alignment.
   */
   return image_binding_grow(device, image, binding,
            }
      static VkResult MUST_CHECK
   add_compression_control_buffer(struct anv_device *device,
                           {
               uint64_t ratio = intel_aux_get_main_to_aux_ratio(device->aux_map_ctx);
   assert(image->planes[plane].primary_surface.isl.size_B % ratio == 0);
            /* The diagram in the Bspec section, Memory Compression - Gfx12 (44930),
   * shows that the CCS is indexed in 256B chunks for TGL, 4K chunks for MTL.
   * When modifiers are in use, the 4K alignment requirement of the
   * PLANE_AUX_DIST::Auxiliary Surface Distance field must be considered
   * (Bspec 50379). Keep things simple and just use 4K.
   */
            return image_binding_grow(device, image, binding, offset, size, alignment,
      }
      /**
   * The return code indicates whether creation of the VkImage should continue
   * or fail, not whether the creation of the aux surface succeeded.  If the aux
   * surface is not required (for example, by neither hardware nor DRM format
   * modifier), then this may return VK_SUCCESS when creation of the aux surface
   * fails.
   *
   * @param offset See add_surface()
   */
   static VkResult
   add_aux_surface_if_supported(struct anv_device *device,
                              struct anv_image *image,
   uint32_t plane,
      {
      VkImageAspectFlags aspect = plane_format.aspect;
   VkResult result;
            /* The aux surface must not be already added. */
            if ((isl_extra_usage_flags & ISL_SURF_USAGE_DISABLE_AUX_BIT))
            /* TODO: consider whether compression with sparse is workable. */
   if (anv_image_is_sparse(image))
            uint32_t binding;
   if (image->vk.drm_format_mod == DRM_FORMAT_MOD_INVALID ||
      isl_drm_modifier_has_aux(image->vk.drm_format_mod)) {
      } else {
                  if (aspect == VK_IMAGE_ASPECT_DEPTH_BIT) {
      /* We don't advertise that depth buffers could be used as storage
   * images.
   */
            /* Allow the user to control HiZ enabling. Disable by default on gfx7
   * because resolves are not currently implemented pre-BDW.
   */
   if (!(image->vk.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) {
      /* It will never be used as an attachment, HiZ is pointless. */
               if (image->vk.mip_levels > 1) {
      anv_perf_warn(VK_LOG_OBJS(&image->vk.base), "Enable multi-LOD HiZ");
               ok = isl_surf_get_hiz_surf(&device->isl_dev,
               if (!ok)
            if (!isl_surf_supports_ccs(&device->isl_dev,
                     } else if (image->vk.usage & (VK_IMAGE_USAGE_SAMPLED_BIT |
                  /* If it's used as an input attachment or a texture and it's
   * single-sampled (this is a requirement for HiZ+CCS write-through
   * mode), use write-through mode so that we don't need to resolve
   * before texturing.  This will make depth testing a bit slower but
   * texturing faster.
   *
   * TODO: This is a heuristic trade-off; we haven't tuned it at all.
   */
   assert(device->info->ver >= 12);
      } else {
      assert(device->info->ver >= 12);
               result = add_surface(device, image, &image->planes[plane].aux_surface,
         if (result != VK_SUCCESS)
            if (anv_image_plane_uses_aux_map(device, image, plane)) {
      result = add_compression_control_buffer(device, image, plane,
               if (result != VK_SUCCESS)
               if (image->planes[plane].aux_usage == ISL_AUX_USAGE_HIZ_CCS_WT)
      } else if (aspect == VK_IMAGE_ASPECT_STENCIL_BIT) {
      if (!isl_surf_supports_ccs(&device->isl_dev,
                                 if (device->info->has_aux_map) {
      result = add_compression_control_buffer(device, image, plane,
               if (result != VK_SUCCESS)
         } else if ((aspect & VK_IMAGE_ASPECT_ANY_COLOR_BIT_ANV) && image->vk.samples == 1) {
      if (image->n_planes != 1) {
      /* Multiplanar images seem to hit a sampler bug with CCS and R16G16
   * format. (Putting the clear state a page/4096bytes further fixes
   * the issue).
   */
               if ((image->vk.create_flags & VK_IMAGE_CREATE_ALIAS_BIT) && !image->from_wsi) {
      /* The image may alias a plane of a multiplanar image. Above we ban
   * CCS on multiplanar images.
   *
   * We must also reject aliasing of any image that uses
   * ANV_IMAGE_MEMORY_BINDING_PRIVATE. Since we're already rejecting all
   * aliasing here, there's no need to further analyze if the image needs
   * a private binding.
   */
               ok = isl_surf_get_ccs_surf(&device->isl_dev,
                           if (!ok)
            /* Choose aux usage */
   if (anv_formats_ccs_e_compatible(device->info, image->vk.create_flags,
                  if (intel_needs_workaround(device->info, 1607794140)) {
      /* FCV is permanently enabled on this HW. */
      } else if (device->info->verx10 >= 125 &&
            /* FCV is enabled via 3DSTATE_3D_MODE. We'd expect plain CCS_E to
   * perform better because it allows for non-zero fast clear colors,
   * but we've run into regressions in several benchmarks (F1 22 and
   * RDR2) when trying to enable it. When non-zero clear colors are
   * enabled, we've observed many partial resolves. We haven't yet
   * root-caused what layout transitions are causing these resolves,
   * so in the meantime, we choose to reduce our clear color support.
   * With only zero clear colors being supported, we might as well
   * turn on FCV.
   */
      } else {
            } else if (device->info->ver >= 12) {
      anv_perf_warn(VK_LOG_OBJS(&image->vk.base),
               image->planes[plane].aux_surface.isl.size_B = 0;
      } else {
                  if (device->info->has_flat_ccs) {
         } else if (device->info->has_aux_map) {
      result = add_compression_control_buffer(device, image, plane,
      } else {
      result = add_surface(device, image,
            }
   if (result != VK_SUCCESS)
               } else if ((aspect & VK_IMAGE_ASPECT_ANY_COLOR_BIT_ANV) && image->vk.samples > 1) {
      assert(!(image->vk.usage & VK_IMAGE_USAGE_STORAGE_BIT));
   ok = isl_surf_get_mcs_surf(&device->isl_dev,
               if (!ok)
                     result = add_surface(device, image, &image->planes[plane].aux_surface,
         if (result != VK_SUCCESS)
                           }
      static VkResult
   add_video_buffers(struct anv_device *device,
               {
      ASSERTED bool ok;
            for (unsigned i = 0; i < profile_list->profileCount; i++) {
      if (profile_list->pProfiles[i].videoCodecOperation == VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR) {
      unsigned w_mb = DIV_ROUND_UP(image->vk.extent.width, ANV_MB_WIDTH);
   unsigned h_mb = DIV_ROUND_UP(image->vk.extent.height, ANV_MB_HEIGHT);
      }
   else if (profile_list->pProfiles[i].videoCodecOperation == VK_VIDEO_CODEC_OPERATION_DECODE_H265_BIT_KHR) {
      unsigned w_mb = DIV_ROUND_UP(image->vk.extent.width, 32);
   unsigned h_mb = DIV_ROUND_UP(image->vk.extent.height, 32);
                  if (size == 0)
            ok = image_binding_grow(device, image, ANV_IMAGE_MEMORY_BINDING_PRIVATE,
            }
      /**
   * Initialize the anv_image::*_surface selected by \a aspect. Then update the
   * image's memory requirements (that is, the image's size and alignment).
   *
   * @param offset See add_surface()
   */
   static VkResult
   add_primary_surface(struct anv_device *device,
                     struct anv_image *image,
   uint32_t plane,
   struct anv_format_plane plane_format,
   uint64_t offset,
   {
      struct anv_surface *anv_surf = &image->planes[plane].primary_surface;
            uint32_t width = image->vk.extent.width;
   uint32_t height = image->vk.extent.height;
   const struct vk_format_ycbcr_info *ycbcr_info =
         if (ycbcr_info) {
      assert(plane < ycbcr_info->n_planes);
   width /= ycbcr_info->planes[plane].denominator_scales[0];
               ok = isl_surf_init(&device->isl_dev, &anv_surf->isl,
      .dim = vk_to_isl_surf_dim[image->vk.image_type],
   .format = plane_format.isl_format,
   .width = width,
   .height = height,
   .depth = image->vk.extent.depth,
   .levels = image->vk.mip_levels,
   .array_len = image->vk.array_layers,
   .samples = image->vk.samples,
   .min_alignment_B = 0,
   .row_pitch_B = stride,
   .usage = isl_usage,
         if (!ok) {
      /* TODO: Should return
   * VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT in come cases.
   */
                        return add_surface(device, image, anv_surf,
      }
      #ifndef NDEBUG
   static bool MUST_CHECK
   memory_range_is_aligned(struct anv_image_memory_range memory_range)
   {
         }
      static bool MUST_CHECK
   memory_ranges_equal(struct anv_image_memory_range a,
         {
      return a.binding == b.binding &&
         a.offset == b.offset &&
      }
   #endif
      struct check_memory_range_params {
      struct anv_image_memory_range *accum_ranges;
   const struct anv_surface *test_surface;
   const struct anv_image_memory_range *test_range;
      };
      #define check_memory_range(...) \
            static void UNUSED
   check_memory_range_s(const struct check_memory_range_params *p)
   {
               const struct anv_image_memory_range *test_range =
            struct anv_image_memory_range *accum_range =
            assert(test_range->binding == p->expect_binding);
   assert(test_range->offset >= memory_range_end(*accum_range));
            if (p->test_surface) {
      assert(anv_surface_is_valid(p->test_surface));
   assert(p->test_surface->memory_range.alignment ==
                  }
      /**
   * Validate the image's memory bindings *after* all its surfaces and memory
   * ranges are final.
   *
   * For simplicity's sake, we do not validate free-form layout of the image's
   * memory bindings. We validate the layout described in the comments of struct
   * anv_image.
   */
   static void
   check_memory_bindings(const struct anv_device *device,
         {
   #ifdef DEBUG
      /* As we inspect each part of the image, we merge the part's memory range
   * into these accumulation ranges.
   */
   struct anv_image_memory_range accum_ranges[ANV_IMAGE_MEMORY_BINDING_END];
   for (int i = 0; i < ANV_IMAGE_MEMORY_BINDING_END; ++i) {
      accum_ranges[i] = (struct anv_image_memory_range) {
                     for (uint32_t p = 0; p < image->n_planes; ++p) {
               /* The binding that must contain the plane's primary surface. */
   const enum anv_image_memory_binding primary_binding = image->disjoint
                  /* Aliasing is incompatible with the private binding because it does not
   * live in a VkDeviceMemory.  The exception is either swapchain images or
   * that the private binding is for a video motion vector buffer.
   */
   assert(!(image->vk.create_flags & VK_IMAGE_CREATE_ALIAS_BIT) ||
         image->from_wsi ||
            /* Check primary surface */
   check_memory_range(accum_ranges,
                  /* Check aux_surface */
   const struct anv_image_memory_range *aux_mem_range =
         if (aux_mem_range->size > 0) {
               /* If an auxiliary surface is used for an externally-shareable image,
   * we have to hide this from the memory of the image since other
   * processes with access to the memory may not be aware of it or of
   * its current state. So put that auxiliary data into a separate
   * buffer (ANV_IMAGE_MEMORY_BINDING_PRIVATE).
   */
   if (anv_image_is_externally_shared(image) &&
      !isl_drm_modifier_has_aux(image->vk.drm_format_mod)) {
               /* Display hardware requires that the aux surface start at
   * a higher address than the primary surface. The 3D hardware
   * doesn't care, but we enforce the display requirement in case
   * the image is sent to display.
   */
   check_memory_range(accum_ranges,
                     /* Check fast clear state */
   if (plane->fast_clear_memory_range.size > 0) {
               /* If an auxiliary surface is used for an externally-shareable image,
   * we have to hide this from the memory of the image since other
   * processes with access to the memory may not be aware of it or of
   * its current state. So put that auxiliary data into a separate
   * buffer (ANV_IMAGE_MEMORY_BINDING_PRIVATE).
   */
   if (anv_image_is_externally_shared(image)) {
                  /* We believe that 256B alignment may be sufficient, but we choose 4K
   * due to lack of testing.  And MI_LOAD/STORE operations require
   * dword-alignment.
   */
   assert(plane->fast_clear_memory_range.alignment == 4096);
   check_memory_range(accum_ranges,
                  #endif
   }
      /**
   * Check that the fully-initialized anv_image is compatible with its DRM format
   * modifier.
   *
   * Checking compatibility at the end of image creation is prudent, not
   * superfluous, because usage of modifiers triggers numerous special cases
   * throughout queries and image creation, and because
   * vkGetPhysicalDeviceImageFormatProperties2 has difficulty detecting all
   * incompatibilities.
   *
   * Return VK_ERROR_UNKNOWN if the incompatibility is difficult to detect in
   * vkGetPhysicalDeviceImageFormatProperties2.  Otherwise, assert fail.
   *
   * Ideally, if vkGetPhysicalDeviceImageFormatProperties2() succeeds with a given
   * modifier, then vkCreateImage() produces an image that is compatible with the
   * modifier. However, it is difficult to reconcile the two functions to agree
   * due to their complexity. For example, isl_surf_get_ccs_surf() may
   * unexpectedly fail in vkCreateImage(), eliminating the image's aux surface
   * even when the modifier requires one. (Maybe we should reconcile the two
   * functions despite the difficulty).
   */
   static VkResult MUST_CHECK
   check_drm_format_mod(const struct anv_device *device,
         {
      /* Image must have a modifier if and only if it has modifier tiling. */
   assert((image->vk.drm_format_mod != DRM_FORMAT_MOD_INVALID) ==
            if (image->vk.drm_format_mod == DRM_FORMAT_MOD_INVALID)
            const struct isl_drm_modifier_info *isl_mod_info =
            /* Driver must support the modifier. */
            /* Enforced by us, not the Vulkan spec. */
   assert(image->vk.image_type == VK_IMAGE_TYPE_2D);
   assert(!(image->vk.aspects & VK_IMAGE_ASPECT_DEPTH_BIT));
   assert(!(image->vk.aspects & VK_IMAGE_ASPECT_STENCIL_BIT));
   assert(image->vk.mip_levels == 1);
   assert(image->vk.array_layers == 1);
            for (int i = 0; i < image->n_planes; ++i) {
      const struct anv_image_plane *plane = &image->planes[i];
   ASSERTED const struct isl_format_layout *isl_layout =
            /* Enforced by us, not the Vulkan spec. */
   assert(isl_layout->txc == ISL_TXC_NONE);
   assert(isl_layout->colorspace == ISL_COLORSPACE_LINEAR ||
            if (isl_drm_modifier_has_aux(isl_mod_info->modifier)) {
                     /* The modifier's required aux usage mandates the image's aux usage.
   * The inverse, however, does not hold; if the modifier has no aux
   * usage, then we may enable a private aux surface.
   */
   if ((isl_mod_info->supports_media_compression &&
      plane->aux_usage != ISL_AUX_USAGE_MC) ||
   (isl_mod_info->supports_render_compression &&
   !isl_aux_usage_has_ccs_e(plane->aux_usage))) {
   return vk_errorf(device, VK_ERROR_UNKNOWN,
                              }
      /**
   * Use when the app does not provide
   * VkImageDrmFormatModifierExplicitCreateInfoEXT.
   */
   static VkResult MUST_CHECK
   add_all_surfaces_implicit_layout(
      struct anv_device *device,
   struct anv_image *image,
   const VkImageFormatListCreateInfo *format_list_info,
   uint32_t stride,
   isl_tiling_flags_t isl_tiling_flags,
      {
      const struct intel_device_info *devinfo = device->info;
            const struct vk_format_ycbcr_info *ycbcr_info =
         if (ycbcr_info)
            unsigned num_aspects = 0;
   VkImageAspectFlagBits aspects[3];
   u_foreach_bit(b, image->vk.aspects) {
      assert(num_aspects < 3);
      }
            /* The Android hardware buffer YV12 format has the planes ordered as Y-Cr-Cb,
   * while Vulkan expects VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM to be in Y-Cb-Cr.
   * Adjust the order we add the ISL surfaces accordingly so the implicit
   * offset gets calculated correctly.
   */
   if (image->from_ahb && image->vk.format == VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM) {
      assert(num_aspects == 3);
   assert(aspects[1] == VK_IMAGE_ASPECT_PLANE_1_BIT);
   assert(aspects[2] == VK_IMAGE_ASPECT_PLANE_2_BIT);
   aspects[1] = VK_IMAGE_ASPECT_PLANE_2_BIT;
               for (unsigned i = 0; i < num_aspects; i++) {
      VkImageAspectFlagBits aspect = aspects[i];
   const uint32_t plane = anv_image_aspect_to_plane(image, aspect);
   const  struct anv_format_plane plane_format =
            enum isl_format isl_fmt = plane_format.isl_format;
            uint32_t plane_stride = stride * isl_format_get_layout(isl_fmt)->bpb / 8;
   if (ycbcr_info)
            VkImageUsageFlags vk_usage = vk_image_usage(&image->vk, aspect);
   isl_surf_usage_flags_t isl_usage =
                  result = add_primary_surface(device, image, plane, plane_format,
               if (result != VK_SUCCESS)
            /* Disable aux if image supports export without modifiers. */
   if (image->vk.external_handle_types != 0 &&
                  result = add_aux_surface_if_supported(device, image, plane, plane_format,
                     if (result != VK_SUCCESS)
                  }
      /**
   * Use when the app provides VkImageDrmFormatModifierExplicitCreateInfoEXT.
   */
   static VkResult
   add_all_surfaces_explicit_layout(
      struct anv_device *device,
   struct anv_image *image,
   const VkImageFormatListCreateInfo *format_list_info,
   const VkImageDrmFormatModifierExplicitCreateInfoEXT *drm_info,
   isl_tiling_flags_t isl_tiling_flags,
      {
      const struct intel_device_info *devinfo = device->info;
   const uint32_t mod_plane_count = drm_info->drmFormatModifierPlaneCount;
   const bool mod_has_aux =
                  /* About valid usage in the Vulkan spec:
   *
   * Unlike vanilla vkCreateImage, which produces undefined behavior on user
   * error, here the spec requires the implementation to return
   * VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT if the app provides
   * a bad plane layout. However, the spec does require
   * drmFormatModifierPlaneCount to be valid.
   *
   * Most validation of plane layout occurs in add_surface().
            /* We support a restricted set of images with modifiers.
   *
   * With aux usage on platforms without flat-CCS,
   * - Format plane count must be 1.
   * - Memory plane count must be 2.
   * Otherwise,
   * - Each format plane must map to a distint memory plane.
   *
   * For the other cases, currently there is no way to properly map memory
   * planes to format planes and aux planes due to the lack of defined ABI
   * for external multi-planar images.
   */
   if (image->n_planes == 1)
         else
            if (mod_has_aux && !devinfo->has_flat_ccs)
         else
            /* Reject special values in the app-provided plane layouts. */
   for (uint32_t i = 0; i < mod_plane_count; ++i) {
      if (drm_info->pPlaneLayouts[i].rowPitch == 0) {
      return vk_errorf(device,
                           if (drm_info->pPlaneLayouts[i].offset == ANV_OFFSET_IMPLICIT) {
      return vk_errorf(device,
                  VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT,
               u_foreach_bit(b, image->vk.aspects) {
      const VkImageAspectFlagBits aspect = 1 << b;
   const uint32_t plane = anv_image_aspect_to_plane(image, aspect);
   const struct anv_format_plane format_plane =
                  result = add_primary_surface(device, image, plane,
                                 if (result != VK_SUCCESS)
            if (mod_has_aux) {
                     const VkSubresourceLayout flat_ccs_layout = {
         };
                  result = add_aux_surface_if_supported(device, image, plane,
                                                                  }
      static const struct isl_drm_modifier_info *
   choose_drm_format_mod(const struct anv_physical_device *device,
         {
      uint64_t best_mod = UINT64_MAX;
            for (uint32_t i = 0; i < modifier_count; ++i) {
      uint32_t score = isl_drm_modifier_get_score(&device->info, modifiers[i]);
   if (score > best_score) {
      best_mod = modifiers[i];
                  if (best_score > 0)
         else
      }
      static VkImageUsageFlags
   anv_image_create_usage(const VkImageCreateInfo *pCreateInfo,
         {
      /* Add TRANSFER_SRC usage for multisample attachment images. This is
   * because we might internally use the TRANSFER_SRC layout on them for
   * blorp operations associated with resolving those into other attachments
   * at the end of a subpass.
   *
   * Without this additional usage, we compute an incorrect AUX state in
   * anv_layout_to_aux_state().
   */
   if (pCreateInfo->samples > VK_SAMPLE_COUNT_1_BIT &&
      (usage & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
               }
      static VkResult MUST_CHECK
   alloc_private_binding(struct anv_device *device,
               {
      struct anv_image_binding *binding =
            if (binding->memory_range.size == 0)
            const VkImageSwapchainCreateInfoKHR *swapchain_info =
            if (swapchain_info && swapchain_info->swapchain != VK_NULL_HANDLE) {
      /* The image will be bound to swapchain memory. */
               VkResult result = anv_device_alloc_bo(device, "image-binding-private",
               if (result == VK_SUCCESS) {
      pthread_mutex_lock(&device->mutex);
   list_addtail(&image->link, &device->image_private_objects);
                  }
      static void
   anv_image_finish_sparse_bindings(struct anv_image *image)
   {
      struct anv_device *device =
                     for (int i = 0; i < ANV_IMAGE_MEMORY_BINDING_END; i++) {
               if (b->sparse_data.size != 0) {
      assert(b->memory_range.size == b->sparse_data.size);
   assert(b->address.offset == b->sparse_data.address);
            }
      static VkResult MUST_CHECK
   anv_image_init_sparse_bindings(struct anv_image *image)
   {
      struct anv_device *device =
                           for (int i = 0; i < ANV_IMAGE_MEMORY_BINDING_END; i++) {
               if (b->memory_range.size != 0) {
               /* From the spec, Custom Sparse Image Block Shapes section:
   *   "... the size in bytes of the custom sparse image block shape
   *    will be reported in VkMemoryRequirements::alignment."
   *
   * ISL should have set this for us, so just assert it here.
   */
                  result = anv_init_sparse_bindings(device,
                     if (result != VK_SUCCESS) {
      anv_image_finish_sparse_bindings(image);
                        }
      VkResult
   anv_image_init(struct anv_device *device, struct anv_image *image,
         {
      const VkImageCreateInfo *pCreateInfo = create_info->vk_info;
   const struct VkImageDrmFormatModifierExplicitCreateInfoEXT *mod_explicit_info = NULL;
   const struct isl_drm_modifier_info *isl_mod_info = NULL;
                     image->vk.usage = anv_image_create_usage(pCreateInfo, image->vk.usage);
   image->vk.stencil_usage =
            if (pCreateInfo->tiling == VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT) {
      assert(!image->vk.wsi_legacy_scanout);
   mod_explicit_info =
      vk_find_struct_const(pCreateInfo->pNext,
      if (mod_explicit_info) {
         } else {
      const struct VkImageDrmFormatModifierListCreateInfoEXT *mod_list_info =
      vk_find_struct_const(pCreateInfo->pNext,
      isl_mod_info = choose_drm_format_mod(device->physical,
                     assert(isl_mod_info);
   assert(image->vk.drm_format_mod == DRM_FORMAT_MOD_INVALID);
               for (int i = 0; i < ANV_IMAGE_MEMORY_BINDING_END; ++i) {
      image->bindings[i] = (struct anv_image_binding) {
                     /* In case of AHardwareBuffer import, we don't know the layout yet */
   if (image->vk.external_handle_types &
      VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID) {
   #ifdef ANDROID
         #endif
                              image->from_wsi =
            /* The Vulkan 1.2.165 glossary says:
   *
   *    A disjoint image consists of multiple disjoint planes, and is created
   *    with the VK_IMAGE_CREATE_DISJOINT_BIT bit set.
   */
   image->disjoint = image->n_planes > 1 &&
            isl_surf_usage_flags_t isl_extra_usage_flags = create_info->isl_extra_usage_flags;
   if (anv_is_format_emulated(device->physical, pCreateInfo->format)) {
      assert(image->n_planes == 1 &&
                  image->emu_plane_format =
            /* for fetching the raw copmressed data and storing the decompressed
   * data
   */
   image->vk.create_flags |=
      VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT |
      if (image->vk.image_type == VK_IMAGE_TYPE_3D)
         image->vk.usage |=
            /* TODO: enable compression on emulation plane */
               const isl_tiling_flags_t isl_tiling_flags =
      choose_isl_tiling_flags(device->info, create_info, isl_mod_info,
         const VkImageFormatListCreateInfo *fmt_list =
      vk_find_struct_const(pCreateInfo->pNext,
         if (mod_explicit_info) {
      r = add_all_surfaces_explicit_layout(device, image, fmt_list,
            } else {
      r = add_all_surfaces_implicit_layout(device, image, fmt_list, create_info->stride,
                     if (r != VK_SUCCESS)
            if (image->emu_plane_format != VK_FORMAT_UNDEFINED) {
      const struct intel_device_info *devinfo = device->info;
   const uint32_t plane = image->n_planes;
   const struct anv_format_plane plane_format = anv_get_format_plane(
            isl_surf_usage_flags_t isl_usage = anv_image_choose_isl_surf_usage(
                  r = add_primary_surface(device, image, plane, plane_format,
               if (r != VK_SUCCESS)
               const VkVideoProfileListInfoKHR *video_profile =
      vk_find_struct_const(pCreateInfo->pNext,
      if (video_profile) {
      r = add_video_buffers(device, image, video_profile);
   if (r != VK_SUCCESS)
               if (!create_info->no_private_binding_alloc) {
      r = alloc_private_binding(device, image, pCreateInfo);
   if (r != VK_SUCCESS)
                        r = check_drm_format_mod(device, image);
   if (r != VK_SUCCESS)
            /* Once we have all the bindings, determine whether we can do non 0 fast
   * clears for each plane.
   */
   for (uint32_t p = 0; p < image->n_planes; p++) {
      image->planes[p].can_non_zero_fast_clear =
               if (anv_image_is_sparse(image)) {
      r = anv_image_init_sparse_bindings(image);
   if (r != VK_SUCCESS)
                     fail:
      vk_image_finish(&image->vk);
      }
      void
   anv_image_finish(struct anv_image *image)
   {
      struct anv_device *device =
            if (anv_image_is_sparse(image))
            /* Unmap a CCS so that if the bound region of the image is rebound to
   * another image, the AUX tables will be cleared to allow for a new
   * mapping.
   */
   for (int p = 0; p < image->n_planes; ++p) {
      if (!image->planes[p].aux_ccs_mapped)
            const struct anv_address main_addr =
      anv_image_address(image,
      const struct isl_surf *surf =
            intel_aux_map_del_mapping(device->aux_map_ctx,
                     if (image->from_gralloc) {
      assert(!image->disjoint);
   assert(image->n_planes == 1);
   assert(image->planes[0].primary_surface.memory_range.binding ==
         assert(image->bindings[ANV_IMAGE_MEMORY_BINDING_MAIN].address.bo != NULL);
               struct anv_bo *private_bo = image->bindings[ANV_IMAGE_MEMORY_BINDING_PRIVATE].address.bo;
   if (private_bo) {
      pthread_mutex_lock(&device->mutex);
   list_del(&image->link);
   pthread_mutex_unlock(&device->mutex);
                  }
      static struct anv_image *
   anv_swapchain_get_image(VkSwapchainKHR swapchain,
         {
      VkImage image = wsi_common_get_image(swapchain, index);
      }
      static VkResult
   anv_image_init_from_create_info(struct anv_device *device,
                     {
      if (pCreateInfo->flags & VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT) {
      VkResult result =
      anv_sparse_image_check_support(device->physical,
                              if (result != VK_SUCCESS)
               const VkNativeBufferANDROID *gralloc_info =
         if (gralloc_info)
      return anv_image_init_from_gralloc(device, image, pCreateInfo,
         struct anv_image_create_info create_info = {
      .vk_info = pCreateInfo,
               /* For dmabuf imports, configure the primary surface without support for
   * compression if the modifier doesn't specify it. This helps to create
   * VkImages with memory requirements that are compatible with the buffers
   * apps provide.
   */
   const struct VkImageDrmFormatModifierExplicitCreateInfoEXT *mod_explicit_info =
      vk_find_struct_const(pCreateInfo->pNext,
      if (mod_explicit_info &&
      !isl_drm_modifier_has_aux(mod_explicit_info->drmFormatModifier))
            }
      VkResult anv_CreateImage(
      VkDevice                                    _device,
   const VkImageCreateInfo*                    pCreateInfo,
   const VkAllocationCallbacks*                pAllocator,
      {
               if (!device->physical->has_sparse &&
      INTEL_DEBUG(DEBUG_SPARSE) &&
   pCreateInfo->flags & (VK_IMAGE_CREATE_SPARSE_BINDING_BIT |
               fprintf(stderr, "=== %s %s:%d flags:0x%08x\n", __func__, __FILE__,
      #ifndef VK_USE_PLATFORM_ANDROID_KHR
      /* Ignore swapchain creation info on Android. Since we don't have an
   * implementation in Mesa, we're guaranteed to access an Android object
   * incorrectly.
   */
   const VkImageSwapchainCreateInfoKHR *swapchain_info =
         if (swapchain_info && swapchain_info->swapchain != VK_NULL_HANDLE) {
      return wsi_common_create_swapchain_image(&device->physical->wsi_device,
                     #endif
         struct anv_image *image =
      vk_object_zalloc(&device->vk, pAllocator, sizeof(*image),
      if (!image)
            VkResult result = anv_image_init_from_create_info(device, image,
               if (result != VK_SUCCESS) {
      vk_object_free(&device->vk, pAllocator, image);
                           }
      void
   anv_DestroyImage(VkDevice _device, VkImage _image,
         {
      ANV_FROM_HANDLE(anv_device, device, _device);
            if (!image)
            assert(&device->vk == image->vk.base.device);
               }
      /* We are binding AHardwareBuffer. Get a description, resolve the
   * format and prepare anv_image properly.
   */
   static void
   resolve_ahw_image(struct anv_device *device,
               {
   #if defined(ANDROID) && ANDROID_API_LEVEL >= 26
      assert(mem->vk.ahardware_buffer);
   AHardwareBuffer_Desc desc;
   AHardwareBuffer_describe(mem->vk.ahardware_buffer, &desc);
            /* Check tiling. */
   enum isl_tiling tiling;
   result = anv_device_get_bo_tiling(device, mem->bo, &tiling);
   assert(result == VK_SUCCESS);
            /* Check format. */
   VkFormat vk_format = vk_format_from_android(desc.format, desc.usage);
            /* Now we are able to fill anv_image fields properly and create
   * isl_surface for it.
   */
   vk_image_set_format(&image->vk, vk_format);
            result = add_all_surfaces_implicit_layout(device, image, NULL, desc.stride,
                  #endif
   }
      void
   anv_image_get_memory_requirements(struct anv_device *device,
                     {
      /* The Vulkan spec (git aaed022) says:
   *
   *    memoryTypeBits is a bitfield and contains one bit set for every
   *    supported memory type for the resource. The bit `1<<i` is set if and
   *    only if the memory type `i` in the VkPhysicalDeviceMemoryProperties
   *    structure for the physical device is supported.
   *
   * All types are currently supported for images.
   */
            vk_foreach_struct(ext, pMemoryRequirements->pNext) {
      switch (ext->sType) {
   case VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS: {
      VkMemoryDedicatedRequirements *requirements = (void *)ext;
   if (image->vk.wsi_legacy_scanout ||
      image->from_ahb ||
   (isl_drm_modifier_has_aux(image->vk.drm_format_mod) &&
   anv_image_uses_aux_map(device, image))) {
   /* If we need to set the tiling for external consumers or the
   * modifier involves AUX tables, we need a dedicated allocation.
   *
   * See also anv_AllocateMemory.
   */
   requirements->prefersDedicatedAllocation = true;
      } else {
      requirements->prefersDedicatedAllocation = false;
      }
               default:
      anv_debug_ignored_stype(ext->sType);
                  /* If the image is disjoint, then we must return the memory requirements for
   * the single plane specified in VkImagePlaneMemoryRequirementsInfo. If
   * non-disjoint, then exactly one set of memory requirements exists for the
   * whole image.
   *
   * This is enforced by the Valid Usage for VkImageMemoryRequirementsInfo2,
   * which requires that the app provide VkImagePlaneMemoryRequirementsInfo if
   * and only if the image is disjoint (that is, multi-planar format and
   * VK_IMAGE_CREATE_DISJOINT_BIT).
   */
   const struct anv_image_binding *binding;
   if (image->disjoint) {
      assert(util_bitcount(aspects) == 1);
   assert(aspects & image->vk.aspects);
      } else {
      assert(aspects == image->vk.aspects);
               pMemoryRequirements->memoryRequirements = (VkMemoryRequirements) {
      .size = binding->memory_range.size,
   .alignment = binding->memory_range.alignment,
         }
      void anv_GetImageMemoryRequirements2(
      VkDevice                                    _device,
   const VkImageMemoryRequirementsInfo2*       pInfo,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
                     vk_foreach_struct_const(ext, pInfo->pNext) {
      switch (ext->sType) {
   case VK_STRUCTURE_TYPE_IMAGE_PLANE_MEMORY_REQUIREMENTS_INFO: {
      assert(image->disjoint);
   const VkImagePlaneMemoryRequirementsInfo *plane_reqs =
         aspects = plane_reqs->planeAspect;
               default:
      anv_debug_ignored_stype(ext->sType);
                  anv_image_get_memory_requirements(device, image, aspects,
      }
      void anv_GetDeviceImageMemoryRequirementsKHR(
      VkDevice                                    _device,
   const VkDeviceImageMemoryRequirements*   pInfo,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
            if (!device->physical->has_sparse &&
      INTEL_DEBUG(DEBUG_SPARSE) &&
   pInfo->pCreateInfo->flags & (VK_IMAGE_CREATE_SPARSE_BINDING_BIT |
               fprintf(stderr, "=== %s %s:%d flags:0x%08x\n", __func__, __FILE__,
         ASSERTED VkResult result =
                  VkImageAspectFlags aspects =
            anv_image_get_memory_requirements(device, &image, aspects,
            }
      static void
   anv_image_get_sparse_memory_requirements(
         struct anv_device *device,
   struct anv_image *image,
   VkImageAspectFlags aspects,
   uint32_t *pSparseMemoryRequirementCount,
   {
      VK_OUTARRAY_MAKE_TYPED(VkSparseImageMemoryRequirements2, reqs,
                  /* From the spec:
   *   "The sparse image must have been created using the
   *    VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT flag to retrieve valid sparse
   *    image memory requirements."
   */
   if (!(image->vk.create_flags & VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT))
            VkSparseImageMemoryRequirements ds_mem_reqs = {};
            u_foreach_bit(b, aspects) {
      VkImageAspectFlagBits aspect = 1 << b;
   const uint32_t plane = anv_image_aspect_to_plane(image, aspect);
            VkSparseImageFormatProperties format_props =
                  uint32_t miptail_first_lod;
   VkDeviceSize miptail_size, miptail_offset, miptail_stride;
   anv_sparse_calc_miptail_properties(device, image, aspect,
                  VkSparseImageMemoryRequirements mem_reqs = {
      .formatProperties = format_props,
   .imageMipTailFirstLod = miptail_first_lod,
   .imageMipTailSize = miptail_size,
   .imageMipTailOffset = miptail_offset,
               /* If both depth and stencil are the same, unify them if possible. */
   if (aspect & (VK_IMAGE_ASPECT_DEPTH_BIT |
            if (!ds_reqs_ptr) {
         } else if (ds_mem_reqs.formatProperties.imageGranularity.width ==
                  ds_mem_reqs.formatProperties.imageGranularity.height ==
         ds_mem_reqs.formatProperties.imageGranularity.depth ==
         ds_mem_reqs.imageMipTailFirstLod ==
         ds_mem_reqs.imageMipTailSize ==
         ds_mem_reqs.imageMipTailOffset ==
         ds_mem_reqs.imageMipTailStride ==
   ds_reqs_ptr->memoryRequirements.formatProperties.aspectMask |=
                        vk_outarray_append_typed(VkSparseImageMemoryRequirements2, &reqs, r) {
      r->memoryRequirements = mem_reqs;
   if (aspect & (VK_IMAGE_ASPECT_DEPTH_BIT |
                  }
      void anv_GetImageSparseMemoryRequirements2(
      VkDevice                                    _device,
   const VkImageSparseMemoryRequirementsInfo2* pInfo,
   uint32_t*                                   pSparseMemoryRequirementCount,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
            if (!anv_sparse_residency_is_enabled(device)) {
      if (!device->physical->has_sparse && INTEL_DEBUG(DEBUG_SPARSE))
            *pSparseMemoryRequirementCount = 0;
               anv_image_get_sparse_memory_requirements(device, image, image->vk.aspects,
            }
      void anv_GetDeviceImageSparseMemoryRequirements(
      VkDevice                                    _device,
   const VkDeviceImageMemoryRequirements*      pInfo,
   uint32_t*                                   pSparseMemoryRequirementCount,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
            if (!anv_sparse_residency_is_enabled(device)) {
      if (!device->physical->has_sparse && INTEL_DEBUG(DEBUG_SPARSE))
            *pSparseMemoryRequirementCount = 0;
               /* This function is similar to anv_GetDeviceImageMemoryRequirementsKHR, in
   * which it actually creates an image, gets the properties and then
   * destroys the image.
   *
   * We could one day refactor things to allow us to gather the properties
   * without having to actually create the image, maybe by reworking ISL to
   * separate creation from parameter computing.
            ASSERTED VkResult result =
      anv_image_init_from_create_info(device, &image, pInfo->pCreateInfo,
               /* The spec says:
   *  "planeAspect is a VkImageAspectFlagBits value specifying the aspect
   *   corresponding to the image plane to query. This parameter is ignored
   *   unless pCreateInfo::tiling is VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT,
   *   or pCreateInfo::flags has VK_IMAGE_CREATE_DISJOINT_BIT set."
   */
   VkImageAspectFlags aspects =
      (pInfo->pCreateInfo->flags & VK_IMAGE_CREATE_DISJOINT_BIT) ||
   (pInfo->pCreateInfo->tiling == VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT)
         anv_image_get_sparse_memory_requirements(device, &image, aspects,
                     }
      VkResult anv_BindImageMemory2(
      VkDevice                                    _device,
   uint32_t                                    bindInfoCount,
      {
               for (uint32_t i = 0; i < bindInfoCount; i++) {
      const VkBindImageMemoryInfo *bind_info = &pBindInfos[i];
   ANV_FROM_HANDLE(anv_device_memory, mem, bind_info->memory);
   ANV_FROM_HANDLE(anv_image, image, bind_info->image);
                     /* Resolve will alter the image's aspects, do this first. */
   if (mem && mem->vk.ahardware_buffer)
            vk_foreach_struct_const(s, bind_info->pNext) {
      switch (s->sType) {
   case VK_STRUCTURE_TYPE_BIND_IMAGE_PLANE_MEMORY_INFO: {
                     /* Workaround for possible spec bug.
   *
   * Unlike VkImagePlaneMemoryRequirementsInfo, which requires that
   * the image be disjoint (that is, multi-planar format and
   * VK_IMAGE_CREATE_DISJOINT_BIT), VkBindImagePlaneMemoryInfo allows
   * the image to be non-disjoint and requires only that the image
   * have the DISJOINT flag. In this case, regardless of the value of
   * VkImagePlaneMemoryRequirementsInfo::planeAspect, the behavior is
   * the same as if VkImagePlaneMemoryRequirementsInfo were omitted.
   */
                                 binding->address = (struct anv_address) {
                        did_bind = true;
      }
   case VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_SWAPCHAIN_INFO_KHR: {
      /* Ignore this struct on Android, we cannot access swapchain
   #ifndef VK_USE_PLATFORM_ANDROID_KHR
               const VkBindImageMemorySwapchainInfoKHR *swapchain_info =
         struct anv_image *swapchain_image =
      anv_swapchain_get_image(swapchain_info->swapchain,
      assert(swapchain_image);
                  for (int j = 0; j < ARRAY_SIZE(image->bindings); ++j) {
      assert(memory_ranges_equal(image->bindings[j].memory_range,
                     /* We must bump the private binding's bo's refcount because, unlike the other
   * bindings, its lifetime is not application-managed.
   */
   struct anv_bo *private_bo =
                  #endif
               #pragma GCC diagnostic push
   #pragma GCC diagnostic ignored "-Wswitch"
            case VK_STRUCTURE_TYPE_NATIVE_BUFFER_ANDROID: {
      const VkNativeBufferANDROID *gralloc_info =
         VkResult result = anv_image_bind_from_gralloc(device, image,
         if (result != VK_SUCCESS)
         did_bind = true;
   #pragma GCC diagnostic pop
            default:
      anv_debug_ignored_stype(s->sType);
                  if (!did_bind) {
               image->bindings[ANV_IMAGE_MEMORY_BINDING_MAIN].address =
      (struct anv_address) {
                                 /* Now that we have the BO, finalize CCS setup. */
   for (int p = 0; p < image->n_planes; ++p) {
      enum anv_image_memory_binding binding =
                                       /* Do nothing if flat CCS requirements are satisfied.
   *
   * Also, assume that imported BOs with a modifier including
   * CCS live only in local memory. Otherwise the exporter should
   * have failed the creation of the BO.
   */
   if (device->info->has_flat_ccs &&
                  /* Add the plane to the aux map when applicable. */
   const struct anv_address main_addr = anv_image_address(
         if (anv_address_allows_aux_map(device, main_addr)) {
      const struct anv_address aux_addr =
      anv_image_address(image,
      const struct isl_surf *surf =
         const uint64_t format_bits =
         image->planes[p].aux_ccs_mapped =
      intel_aux_map_add_mapping(device->aux_map_ctx,
                  if (image->planes[p].aux_ccs_mapped)
               /* Do nothing prior to gfx12. There are no special requirements. */
                                                if (image->planes[p].aux_surface.memory_range.size > 0) {
      assert(image->planes[p].aux_usage == ISL_AUX_USAGE_HIZ_CCS ||
            } else {
      assert(image->planes[p].aux_usage == ISL_AUX_USAGE_CCS_E ||
         image->planes[p].aux_usage == ISL_AUX_USAGE_FCV_CCS_E ||
                        }
      static void
   anv_get_image_subresource_layout(const struct anv_image *image,
               {
      const struct anv_image_memory_range *mem_range;
                     /* The Vulkan spec requires that aspectMask be
   * VK_IMAGE_ASPECT_MEMORY_PLANE_i_BIT_EXT if tiling is
   * VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT.
   *
   * For swapchain images, the Vulkan spec says that every swapchain image has
   * tiling VK_IMAGE_TILING_OPTIMAL, but we may choose
   * VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT internally.  Vulkan doesn't allow
   * vkGetImageSubresourceLayout for images with VK_IMAGE_TILING_OPTIMAL,
   * therefore it's invalid for the application to call this on a swapchain
   * image.  The WSI code, however, knows when it has internally created
   * a swapchain image with VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT,
   * so it _should_ correctly use VK_IMAGE_ASPECT_MEMORY_PLANE_* in that case.
   * But it incorrectly uses VK_IMAGE_ASPECT_PLANE_*, so we have a temporary
   * workaround.
   */
   if (image->vk.tiling == VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT) {
      /* TODO(chadv): Drop this workaround when WSI gets fixed. */
   uint32_t mem_plane;
   switch (subresource->imageSubresource.aspectMask) {
   case VK_IMAGE_ASPECT_MEMORY_PLANE_0_BIT_EXT:
   case VK_IMAGE_ASPECT_PLANE_0_BIT:
      mem_plane = 0;
      case VK_IMAGE_ASPECT_MEMORY_PLANE_1_BIT_EXT:
   case VK_IMAGE_ASPECT_PLANE_1_BIT:
      mem_plane = 1;
      case VK_IMAGE_ASPECT_MEMORY_PLANE_2_BIT_EXT:
   case VK_IMAGE_ASPECT_PLANE_2_BIT:
      mem_plane = 2;
      default:
                  if (mem_plane == 1 && isl_drm_modifier_has_aux(image->vk.drm_format_mod)) {
      assert(image->n_planes == 1);
   /* If the memory binding differs between primary and aux, then the
   * returned offset will be incorrect.
   */
   mem_range = anv_image_get_aux_memory_range(image, 0);
   assert(mem_range->binding ==
            } else {
      assert(mem_plane < image->n_planes);
   mem_range = &image->planes[mem_plane].primary_surface.memory_range;
         } else {
      const uint32_t plane =
         mem_range = &image->planes[plane].primary_surface.memory_range;
               layout->subresourceLayout.offset = mem_range->offset;
   layout->subresourceLayout.rowPitch = isl_surf->row_pitch_B;
   layout->subresourceLayout.depthPitch = isl_surf_get_array_pitch(isl_surf);
            if (subresource->imageSubresource.mipLevel > 0 ||
      subresource->imageSubresource.arrayLayer > 0) {
            uint64_t offset_B;
   isl_surf_get_image_offset_B_tile_sa(isl_surf,
                           layout->subresourceLayout.offset += offset_B;
   layout->subresourceLayout.size =
      layout->subresourceLayout.rowPitch *
   u_minify(image->vk.extent.height,
         } else {
            }
      void anv_GetImageSubresourceLayout(
      VkDevice                                    device,
   VkImage                                     _image,
   const VkImageSubresource*                   pSubresource,
      {
               VkImageSubresource2KHR subresource = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_SUBRESOURCE_2_KHR,
      };
   VkSubresourceLayout2KHR layout = {
         };
               }
      void anv_GetDeviceImageSubresourceLayoutKHR(
      VkDevice                                    _device,
   const VkDeviceImageSubresourceInfoKHR*      pInfo,
      {
                        if (anv_image_init_from_create_info(device, &image, pInfo->pCreateInfo,
            pLayout->subresourceLayout = (VkSubresourceLayout) { 0, };
                  }
      void anv_GetImageSubresourceLayout2KHR(
      VkDevice                                    device,
   VkImage                                     _image,
   const VkImageSubresource2KHR*               pSubresource,
      {
                  }
      static VkImageUsageFlags
   anv_image_flags_filter_for_queue(VkImageUsageFlags usages,
         {
      /* Eliminate graphics usages if the queue is not graphics capable */
   if (!(queue_flags & VK_QUEUE_GRAPHICS_BIT)) {
      usages &= ~(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
               VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
   VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
   VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT |
               /* Eliminate sampling & storage usages if the queue is neither graphics nor
   * compute capable
   */
   if (!(queue_flags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))) {
      usages &= ~(VK_IMAGE_USAGE_SAMPLED_BIT |
               /* Eliminate transfer usages if the queue is neither transfer, compute or
   * graphics capable
   */
   if (!(queue_flags & (VK_QUEUE_TRANSFER_BIT |
                  usages &= ~(VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                  }
      /**
   * This function returns the assumed isl_aux_state for a given VkImageLayout.
   * Because Vulkan image layouts don't map directly to isl_aux_state enums, the
   * returned enum is the assumed worst case.
   *
   * @param devinfo The device information of the Intel GPU.
   * @param image The image that may contain a collection of buffers.
   * @param aspect The aspect of the image to be accessed.
   * @param layout The current layout of the image aspect(s).
   *
   * @return The primary buffer that should be used for the given layout.
   */
   enum isl_aux_state ATTRIBUTE_PURE
   anv_layout_to_aux_state(const struct intel_device_info * const devinfo,
                           {
               /* The devinfo is needed as the optimal buffer varies across generations. */
            /* The layout of a NULL image is not properly defined. */
            /* The aspect must be exactly one of the image aspects. */
                              /* If we don't have an aux buffer then aux state makes no sense */
   const enum isl_aux_usage aux_usage = image->planes[plane].aux_usage;
            /* All images that use an auxiliary surface are required to be tiled. */
            /* Handle a few special cases */
   switch (layout) {
   /* Invalid layouts */
   case VK_IMAGE_LAYOUT_MAX_ENUM:
            /* Undefined layouts
   *
   * The pre-initialized layout is equivalent to the undefined layout for
   * optimally-tiled images.  We can only do color compression (CCS or HiZ)
   * on tiled images.
   */
   case VK_IMAGE_LAYOUT_UNDEFINED:
   case VK_IMAGE_LAYOUT_PREINITIALIZED:
            case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: {
               enum isl_aux_state aux_state =
            switch (aux_state) {
   case ISL_AUX_STATE_AUX_INVALID:
      /* The modifier does not support compression. But, if we arrived
   * here, then we have enabled compression on it anyway, in which case
   * we must resolve the aux surface before we release ownership to the
   * presentation engine (because, having no modifier, the presentation
   * engine will not be aware of the aux surface). The presentation
   * engine will not access the aux surface (because it is unware of
   * it), and so the aux surface will still be resolved when we
   * re-acquire ownership.
   *
   * Therefore, at ownership transfers in either direction, there does
   * exist an aux surface despite the lack of modifier and its state is
   * pass-through.
   */
      case ISL_AUX_STATE_COMPRESSED_NO_CLEAR:
         default:
                     default:
                           const VkImageUsageFlags image_aspect_usage =
      anv_image_flags_filter_for_queue(
      const VkImageUsageFlags usage =
            bool aux_supported = true;
            if ((usage & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT) && !read_only) {
      /* This image could be used as both an input attachment and a render
   * target (depth, stencil, or color) at the same time and this can cause
   * corruption.
   *
   * We currently only disable aux in this way for depth even though we
   * disable it for color in GL.
   *
   * TODO: Should we be disabling this in more cases?
   */
   if (aspect == VK_IMAGE_ASPECT_DEPTH_BIT && devinfo->ver <= 9) {
      aux_supported = false;
                  if (usage & (VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                  switch (aux_usage) {
   case ISL_AUX_USAGE_HIZ:
      if (!anv_can_sample_with_hiz(devinfo, image)) {
      aux_supported = false;
                  case ISL_AUX_USAGE_HIZ_CCS:
      aux_supported = false;
               case ISL_AUX_USAGE_HIZ_CCS_WT:
            case ISL_AUX_USAGE_CCS_D:
      aux_supported = false;
               case ISL_AUX_USAGE_MCS:
      if (!anv_can_sample_mcs_with_clear(devinfo, image))
               case ISL_AUX_USAGE_CCS_E:
   case ISL_AUX_USAGE_FCV_CCS_E:
   case ISL_AUX_USAGE_STC_CCS:
            default:
                     switch (aux_usage) {
   case ISL_AUX_USAGE_HIZ:
   case ISL_AUX_USAGE_HIZ_CCS:
   case ISL_AUX_USAGE_HIZ_CCS_WT:
      if (aux_supported) {
      assert(clear_supported);
      } else if (read_only) {
         } else {
               case ISL_AUX_USAGE_CCS_D:
      /* We only support clear in exactly one state */
   if (layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ||
      layout == VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL) {
   assert(aux_supported);
   assert(clear_supported);
      } else {
               case ISL_AUX_USAGE_CCS_E:
   case ISL_AUX_USAGE_FCV_CCS_E:
      if (aux_supported) {
      assert(clear_supported);
      } else {
               case ISL_AUX_USAGE_MCS:
      assert(aux_supported);
   if (clear_supported) {
         } else {
               case ISL_AUX_USAGE_STC_CCS:
      assert(aux_supported);
   assert(!clear_supported);
         default:
            }
      /**
   * This function determines the optimal buffer to use for a given
   * VkImageLayout and other pieces of information needed to make that
   * determination. This does not determine the optimal buffer to use
   * during a resolve operation.
   *
   * @param devinfo The device information of the Intel GPU.
   * @param image The image that may contain a collection of buffers.
   * @param aspect The aspect of the image to be accessed.
   * @param usage The usage which describes how the image will be accessed.
   * @param layout The current layout of the image aspect(s).
   *
   * @return The primary buffer that should be used for the given layout.
   */
   enum isl_aux_usage ATTRIBUTE_PURE
   anv_layout_to_aux_usage(const struct intel_device_info * const devinfo,
                           const struct anv_image * const image,
   {
               /* If there is no auxiliary surface allocated, we must use the one and only
   * main buffer.
   */
   if (image->planes[plane].aux_usage == ISL_AUX_USAGE_NONE)
            enum isl_aux_state aux_state =
            switch (aux_state) {
   case ISL_AUX_STATE_CLEAR:
            case ISL_AUX_STATE_PARTIAL_CLEAR:
      assert(image->vk.aspects & VK_IMAGE_ASPECT_ANY_COLOR_BIT_ANV);
   assert(image->planes[plane].aux_usage == ISL_AUX_USAGE_CCS_D);
   assert(image->vk.samples == 1);
         case ISL_AUX_STATE_COMPRESSED_CLEAR:
   case ISL_AUX_STATE_COMPRESSED_NO_CLEAR:
            case ISL_AUX_STATE_RESOLVED:
      /* We can only use RESOLVED in read-only layouts because any write will
   * either land us in AUX_INVALID or COMPRESSED_NO_CLEAR.  We can do
   * writes in PASS_THROUGH without destroying it so that is allowed.
   */
   assert(vk_image_layout_is_read_only(layout, aspect));
   assert(util_is_power_of_two_or_zero(usage));
   if (usage == VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
      /* If we have valid HiZ data and are using the image as a read-only
   * depth/stencil attachment, we should enable HiZ so that we can get
   * faster depth testing.
   */
      } else {
               case ISL_AUX_STATE_PASS_THROUGH:
   case ISL_AUX_STATE_AUX_INVALID:
                     }
      /**
   * This function returns the level of unresolved fast-clear support of the
   * given image in the given VkImageLayout.
   *
   * @param devinfo The device information of the Intel GPU.
   * @param image The image that may contain a collection of buffers.
   * @param aspect The aspect of the image to be accessed.
   * @param usage The usage which describes how the image will be accessed.
   * @param layout The current layout of the image aspect(s).
   */
   enum anv_fast_clear_type ATTRIBUTE_PURE
   anv_layout_to_fast_clear_type(const struct intel_device_info * const devinfo,
                           {
      if (INTEL_DEBUG(DEBUG_NO_FAST_CLEAR))
                     /* If there is no auxiliary surface allocated, there are no fast-clears */
   if (image->planes[plane].aux_usage == ISL_AUX_USAGE_NONE)
            enum isl_aux_state aux_state =
            const VkImageUsageFlags layout_usage =
            switch (aux_state) {
   case ISL_AUX_STATE_CLEAR:
            case ISL_AUX_STATE_PARTIAL_CLEAR:
   case ISL_AUX_STATE_COMPRESSED_CLEAR:
      if (aspect == VK_IMAGE_ASPECT_DEPTH_BIT) {
         } else if (layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL ||
            /* The image might not support non zero fast clears when mutable. */
                  /* When we're in a render pass we have the clear color data from the
   * VkRenderPassBeginInfo and we can use arbitrary clear colors.  They
   * must get partially resolved before we leave the render pass.
   */
      } else if (layout_usage & (VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
            /* Fast clear with non zero color is not supported during transfer
   * operations since transfer may do format reinterpretation.
   */
      } else if (image->planes[plane].aux_usage == ISL_AUX_USAGE_MCS ||
            image->planes[plane].aux_usage == ISL_AUX_USAGE_CCS_E ||
   if (devinfo->ver >= 11) {
      /* The image might not support non zero fast clears when mutable. */
                  /* On ICL and later, the sampler hardware uses a copy of the clear
   * value that is encoded as a pixel value.  Therefore, we can use
   * any clear color we like for sampling.
   */
      } else {
      /* If the image has MCS or CCS_E enabled all the time then we can
   * use fast-clear as long as the clear color is the default value
   * of zero since this is the default value we program into every
   * surface state used for texturing.
   */
         } else {
               case ISL_AUX_STATE_COMPRESSED_NO_CLEAR:
   case ISL_AUX_STATE_RESOLVED:
   case ISL_AUX_STATE_PASS_THROUGH:
   case ISL_AUX_STATE_AUX_INVALID:
                     }
         /**
   * This function determines if the layout & usage of an image can have
   * untracked aux writes. When we see a transition that matches this criteria,
   * we need to mark the image as compressed written so that our predicated
   * resolves work properly.
   *
   * @param devinfo The device information of the Intel GPU.
   * @param image The image that may contain a collection of buffers.
   * @param aspect The aspect of the image to be accessed.
   * @param layout The current layout of the image aspect(s).
   */
   bool
   anv_layout_has_untracked_aux_writes(const struct intel_device_info * const devinfo,
                           {
      const VkImageUsageFlags image_aspect_usage =
         const VkImageUsageFlags usage =
            /* Storage is the only usage where we do not write the image through a
   * render target but through a descriptor. Since VK_EXT_descriptor_indexing
   * and the update-after-bind feature, it has become impossible to track
   * writes to images in descriptor at the command buffer build time. So it's
   * not possible to mark an image as compressed like we do in
   * genX_cmd_buffer.c(EndRendering) or anv_blorp.c for all transfer
   * operations.
   */
   if (!(usage & VK_IMAGE_USAGE_STORAGE_BIT))
            /* No AUX, no writes to the AUX surface :) */
   const uint32_t plane = anv_image_aspect_to_plane(image, aspect);
   const enum isl_aux_usage aux_usage = image->planes[plane].aux_usage;
   if (aux_usage == ISL_AUX_USAGE_NONE)
               }
      static struct anv_state
   maybe_alloc_surface_state(struct anv_device *device,
         {
      if (device->physical->indirect_descriptors) {
      if (surface_state_stream)
            } else {
            }
      static enum isl_channel_select
   remap_swizzle(VkComponentSwizzle swizzle,
         {
      switch (swizzle) {
   case VK_COMPONENT_SWIZZLE_ZERO:  return ISL_CHANNEL_SELECT_ZERO;
   case VK_COMPONENT_SWIZZLE_ONE:   return ISL_CHANNEL_SELECT_ONE;
   case VK_COMPONENT_SWIZZLE_R:     return format_swizzle.r;
   case VK_COMPONENT_SWIZZLE_G:     return format_swizzle.g;
   case VK_COMPONENT_SWIZZLE_B:     return format_swizzle.b;
   case VK_COMPONENT_SWIZZLE_A:     return format_swizzle.a;
   default:
            }
      void
   anv_image_fill_surface_state(struct anv_device *device,
                              const struct anv_image *image,
   VkImageAspectFlagBits aspect,
   const struct isl_view *view_in,
      {
      uint32_t plane = anv_image_aspect_to_plane(image, aspect);
   if (image->emu_plane_format != VK_FORMAT_UNDEFINED) {
      const uint16_t view_bpb = isl_format_get_layout(view_in->format)->bpb;
   const uint16_t plane_bpb = isl_format_get_layout(
            /* We should redirect to the hidden plane when the original view format
   * is compressed or when the view usage is storage.  But we don't always
   * have visibility to the original view format so we also check for size
   * compatibility.
   */
   if (isl_format_is_compressed(view_in->format) ||
      (view_usage & ISL_SURF_USAGE_STORAGE_BIT) ||
   view_bpb != plane_bpb) {
   plane = image->n_planes;
   assert(isl_format_get_layout(
                        const struct anv_surface *surface = &image->planes[plane].primary_surface,
            struct isl_view view = *view_in;
            if (view_usage == ISL_SURF_USAGE_RENDER_TARGET_BIT)
            /* If this is a HiZ buffer we can sample from with a programmable clear
   * value (SKL+), define the clear value to the optimal constant.
   */
   union isl_color_value default_clear_color = { .u32 = { 0, } };
   if (aspect == VK_IMAGE_ASPECT_DEPTH_BIT)
         if (!clear_color)
            const struct anv_address address =
                              struct isl_surf tmp_surf;
   uint64_t offset_B = 0;
   uint32_t tile_x_sa = 0, tile_y_sa = 0;
   if (isl_format_is_compressed(surface->isl.format) &&
      !isl_format_is_compressed(view.format)) {
   /* We're creating an uncompressed view of a compressed surface. This is
   * allowed but only for a single level/layer.
   */
   assert(surface->isl.samples == 1);
   assert(view.levels == 1);
            ASSERTED bool ok =
      isl_surf_get_uncompressed_surf(&device->isl_dev, isl_surf, &view,
            assert(ok);
                        struct anv_address aux_address = ANV_NULL_ADDRESS;
   if (aux_usage != ISL_AUX_USAGE_NONE)
                  struct anv_address clear_address = ANV_NULL_ADDRESS;
   if (device->info->ver >= 10 && isl_aux_usage_has_fast_clears(aux_usage)) {
         }
            isl_surf_fill_state(&device->isl_dev, surface_state_map,
                     .surf = isl_surf,
   .view = &view,
   .address = anv_address_physical(state_inout->address),
   .clear_color = *clear_color,
   .aux_surf = &aux_surface->isl,
   .aux_usage = aux_usage,
   .aux_address = anv_address_physical(aux_address),
   .clear_address = anv_address_physical(clear_address),
   .use_clear_address = !anv_address_is_null(clear_address),
   .mocs = anv_mocs(device, state_inout->address.bo,
         .x_offset_sa = tile_x_sa,
   .y_offset_sa = tile_y_sa,
   /* Assume robustness with EXT_pipeline_robustness
      * because this can be turned on/off per pipeline and
   * we have no visibility on this here.
               /* With the exception of gfx8, the bottom 12 bits of the MCS base address
   * are used to store other information. This should be ok, however, because
   * the surface buffer addresses are always 4K page aligned.
   */
   if (!anv_address_is_null(aux_address)) {
      uint32_t *aux_addr_dw = surface_state_map +
         assert((aux_address.offset & 0xfff) == 0);
               if (device->info->ver >= 10 && clear_address.bo) {
      uint32_t *clear_addr_dw = surface_state_map +
         assert((clear_address.offset & 0x3f) == 0);
               if (state_inout->state.map)
      }
      static uint32_t
   anv_image_aspect_get_planes(VkImageAspectFlags aspect_mask)
   {
      anv_assert_valid_aspect_set(aspect_mask);
      }
      bool
   anv_can_hiz_clear_ds_view(struct anv_device *device,
                           const struct anv_image_view *iview,
   VkImageLayout layout,
   {
      if (INTEL_DEBUG(DEBUG_NO_FAST_CLEAR))
            /* If we're just clearing stencil, we can always HiZ clear */
   if (!(clear_aspects & VK_IMAGE_ASPECT_DEPTH_BIT))
            /* We must have depth in order to have HiZ */
   if (!(iview->image->vk.aspects & VK_IMAGE_ASPECT_DEPTH_BIT))
            const enum isl_aux_usage clear_aux_usage =
      anv_layout_to_aux_usage(device->info, iview->image,
                  if (!blorp_can_hiz_clear_depth(device->info,
                                 &iview->image->planes[0].primary_surface.isl,
   clear_aux_usage,
   iview->planes[0].isl.base_level,
   iview->planes[0].isl.base_array_layer,
   render_area.offset.x,
            if (depth_clear_value != ANV_HZ_FC_VAL)
            /* If we got here, then we can fast clear */
      }
      static bool
   isl_color_value_requires_conversion(union isl_color_value color,
               {
      if (surf->format == view->format && isl_swizzle_is_identity(view->swizzle))
            uint32_t surf_pack[4] = { 0, 0, 0, 0 };
            uint32_t view_pack[4] = { 0, 0, 0, 0 };
   union isl_color_value swiz_color =
                     }
      bool
   anv_can_fast_clear_color_view(struct anv_device *device,
                                 struct anv_image_view *iview,
   {
      if (INTEL_DEBUG(DEBUG_NO_FAST_CLEAR))
            if (iview->planes[0].isl.base_array_layer >=
      anv_image_aux_layers(iview->image, VK_IMAGE_ASPECT_COLOR_BIT,
               /* Start by getting the fast clear type.  We use the first subpass
   * layout here because we don't want to fast-clear if the first subpass
   * to use the attachment can't handle fast-clears.
   */
   enum anv_fast_clear_type fast_clear_type =
      anv_layout_to_fast_clear_type(device->info, iview->image,
            switch (fast_clear_type) {
   case ANV_FAST_CLEAR_NONE:
         case ANV_FAST_CLEAR_DEFAULT_VALUE:
      if (!isl_color_value_is_zero(clear_color, iview->planes[0].isl.format))
            case ANV_FAST_CLEAR_ANY:
                  /* Potentially, we could do partial fast-clears but doing so has crazy
   * alignment restrictions.  It's easier to just restrict to full size
   * fast clears for now.
   */
   if (render_area.offset.x != 0 ||
      render_area.offset.y != 0 ||
   render_area.extent.width != iview->vk.extent.width ||
   render_area.extent.height != iview->vk.extent.height)
         /* If the clear color is one that would require non-trivial format
   * conversion on resolve, we don't bother with the fast clear.  This
   * shouldn't be common as most clear colors are 0/1 and the most common
   * format re-interpretation is for sRGB.
   */
   if (isl_color_value_requires_conversion(clear_color,
                  anv_perf_warn(VK_LOG_OBJS(&iview->vk.base),
                           /* We only allow fast clears to the first slice of an image (level 0,
   * layer 0) and only for the entire slice.  This guarantees us that, at
   * any given time, there is only one clear color on any given image at
   * any given time.  At the time of our testing (Jan 17, 2018), there
   * were no known applications which would benefit from fast-clearing
   * more than just the first slice.
   */
   if (iview->planes[0].isl.base_level > 0 ||
      iview->planes[0].isl.base_array_layer > 0) {
   anv_perf_warn(VK_LOG_OBJS(&iview->image->vk.base),
               "Rendering with multi-lod or multi-layer framebuffer "
               if (num_layers > 1) {
      anv_perf_warn(VK_LOG_OBJS(&iview->image->vk.base),
                        }
      void
   anv_image_view_init(struct anv_device *device,
                     {
               vk_image_view_init(&device->vk, &iview->vk, false, pCreateInfo);
   iview->image = image;
   iview->n_planes = anv_image_aspect_get_planes(iview->vk.aspects);
            /* Now go through the underlying image selected planes and map them to
   * planes in the image view.
   */
   anv_foreach_image_aspect_bit(iaspect_bit, image, iview->vk.aspects) {
      const uint32_t vplane =
            VkFormat view_format = iview->vk.view_format;
   if (anv_is_format_emulated(device->physical, view_format)) {
      assert(image->emu_plane_format != VK_FORMAT_UNDEFINED);
   view_format =
      }
   const struct anv_format_plane format = anv_get_format_plane(
            iview->planes[vplane].isl = (struct isl_view) {
      .format = format.isl_format,
   .base_level = iview->vk.base_mip_level,
   .levels = iview->vk.level_count,
   .base_array_layer = iview->vk.base_array_layer,
   .array_len = iview->vk.layer_count,
   .min_lod_clamp = iview->vk.min_lod,
   .swizzle = {
      .r = remap_swizzle(iview->vk.swizzle.r, format.swizzle),
   .g = remap_swizzle(iview->vk.swizzle.g, format.swizzle),
   .b = remap_swizzle(iview->vk.swizzle.b, format.swizzle),
                  if (pCreateInfo->viewType == VK_IMAGE_VIEW_TYPE_3D) {
      iview->planes[vplane].isl.base_array_layer = 0;
               if (pCreateInfo->viewType == VK_IMAGE_VIEW_TYPE_CUBE ||
      pCreateInfo->viewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY) {
      } else {
                  if (iview->vk.usage & (VK_IMAGE_USAGE_SAMPLED_BIT |
            iview->planes[vplane].optimal_sampler.state =
                        enum isl_aux_usage general_aux_usage =
      anv_layout_to_aux_usage(device->info, image, 1UL << iaspect_bit,
                              enum isl_aux_usage optimal_aux_usage =
      anv_layout_to_aux_usage(device->info, image, 1UL << iaspect_bit,
                                 anv_image_fill_surface_state(device, image, 1ULL << iaspect_bit,
                                    anv_image_fill_surface_state(device, image, 1ULL << iaspect_bit,
                                       /* NOTE: This one needs to go last since it may stomp isl_view.format */
   if (iview->vk.usage & VK_IMAGE_USAGE_STORAGE_BIT) {
      struct isl_view storage_view = iview->planes[vplane].isl;
   if (iview->vk.view_type == VK_IMAGE_VIEW_TYPE_3D) {
      storage_view.base_array_layer = iview->vk.storage.z_slice_offset;
               enum isl_aux_usage general_aux_usage =
      anv_layout_to_aux_usage(device->info, image, 1UL << iaspect_bit,
                                             anv_image_fill_surface_state(device, image, 1ULL << iaspect_bit,
                                    }
      void
   anv_image_view_finish(struct anv_image_view *iview)
   {
      struct anv_device *device =
            if (!iview->use_surface_state_stream) {
      for (uint32_t plane = 0; plane < iview->n_planes; plane++) {
      if (iview->planes[plane].optimal_sampler.state.alloc_size) {
      anv_state_pool_free(&device->bindless_surface_state_pool,
               if (iview->planes[plane].general_sampler.state.alloc_size) {
      anv_state_pool_free(&device->bindless_surface_state_pool,
               if (iview->planes[plane].storage.state.alloc_size) {
      anv_state_pool_free(&device->bindless_surface_state_pool,
                        }
      VkResult
   anv_CreateImageView(VkDevice _device,
                     {
      ANV_FROM_HANDLE(anv_device, device, _device);
            iview = vk_zalloc2(&device->vk.alloc, pAllocator, sizeof(*iview), 8,
         if (iview == NULL)
                                 }
      void
   anv_DestroyImageView(VkDevice _device, VkImageView _iview,
         {
               if (!iview)
            anv_image_view_finish(iview);
      }
      static void
   anv_fill_buffer_view_surface_state(struct anv_device *device,
                                       {
      anv_fill_buffer_surface_state(device,
                              if (state->state.map)
      }
      VkResult
   anv_CreateBufferView(VkDevice _device,
                     {
      ANV_FROM_HANDLE(anv_device, device, _device);
   ANV_FROM_HANDLE(anv_buffer, buffer, pCreateInfo->buffer);
            view = vk_buffer_view_create(&device->vk, pCreateInfo,
         if (!view)
            const VkBufferUsageFlags2CreateInfoKHR *view_usage_info =
         const VkBufferUsageFlags buffer_usage =
            struct anv_format_plane format;
   format = anv_get_format_plane(device->info, pCreateInfo->format,
            const uint32_t format_bs = isl_format_get_layout(format.isl_format)->bpb / 8;
   const uint32_t align_range =
                     if (buffer_usage & VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT) {
               anv_fill_buffer_view_surface_state(device,
                              } else {
                  if (buffer_usage & VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT) {
               anv_fill_buffer_view_surface_state(device,
                        } else {
                              }
      void
   anv_DestroyBufferView(VkDevice _device, VkBufferView bufferView,
         {
      ANV_FROM_HANDLE(anv_device, device, _device);
            if (!view)
            if (view->general.state.alloc_size > 0) {
      anv_state_pool_free(&device->bindless_surface_state_pool,
               if (view->storage.state.alloc_size > 0) {
      anv_state_pool_free(&device->bindless_surface_state_pool,
                  }
      void anv_GetRenderingAreaGranularityKHR(
      VkDevice                                    _device,
   const VkRenderingAreaInfoKHR*               pRenderingAreaInfo,
      {
      *pGranularity = (VkExtent2D) {
      .width = 1,
         }
