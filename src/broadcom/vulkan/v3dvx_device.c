   /*
   * Copyright © 2021 Raspberry Pi Ltd
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
      #include "v3dv_private.h"
      #include "broadcom/common/v3d_macros.h"
   #include "broadcom/cle/v3dx_pack.h"
   #include "broadcom/compiler/v3d_compiler.h"
   #include "util/u_pack_color.h"
   #include "util/half_float.h"
      static const enum V3DX(Wrap_Mode) vk_to_v3d_wrap_mode[] = {
      [VK_SAMPLER_ADDRESS_MODE_REPEAT]          = V3D_WRAP_MODE_REPEAT,
   [VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT] = V3D_WRAP_MODE_MIRROR,
   [VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE]   = V3D_WRAP_MODE_CLAMP,
   [VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE] = V3D_WRAP_MODE_MIRROR_ONCE,
      };
      static const enum V3DX(Compare_Function)
   vk_to_v3d_compare_func[] = {
      [VK_COMPARE_OP_NEVER]                        = V3D_COMPARE_FUNC_NEVER,
   [VK_COMPARE_OP_LESS]                         = V3D_COMPARE_FUNC_LESS,
   [VK_COMPARE_OP_EQUAL]                        = V3D_COMPARE_FUNC_EQUAL,
   [VK_COMPARE_OP_LESS_OR_EQUAL]                = V3D_COMPARE_FUNC_LEQUAL,
   [VK_COMPARE_OP_GREATER]                      = V3D_COMPARE_FUNC_GREATER,
   [VK_COMPARE_OP_NOT_EQUAL]                    = V3D_COMPARE_FUNC_NOTEQUAL,
   [VK_COMPARE_OP_GREATER_OR_EQUAL]             = V3D_COMPARE_FUNC_GEQUAL,
      };
      static union pipe_color_union encode_border_color(
      const struct v3dv_device *device,
      {
      const struct util_format_description *desc =
                     /* YCbCr doesn't interact with border color at all. From spec:
   *
   *   "If sampler YCBCR conversion is enabled, addressModeU, addressModeV,
   *    and addressModeW must be VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
   *    anisotropyEnable must be VK_FALSE, and unnormalizedCoordinates must
   *    be VK_FALSE"
   */
            /* We use the swizzle in our format table to determine swizzle configuration
   * for sampling as well as to decide if we need to use the Swap R/B and
   * Reverse Channels bits for Tile Load/Store operations. The order of the
   * R/B swap and Reverse operations matters and gives different swizzles.
   * Our format table assumes that Reverse happens first and R/B Swap second.
   * This seems to match semantics for texture sampling and Tile load/store,
   * however, it seems that the semantics are reversed for custom border
   * colors so we need to fix up the swizzle manually for this case.
   */
   uint8_t swizzle[4];
   const bool v3d_has_reverse_swap_rb_bits =
         if (!v3d_has_reverse_swap_rb_bits &&
      v3dv_format_swizzle_needs_reverse(format->planes[0].swizzle) &&
   v3dv_format_swizzle_needs_rb_swap(format->planes[0].swizzle)) {
   swizzle[0] = PIPE_SWIZZLE_W;
   swizzle[1] = PIPE_SWIZZLE_X;
   swizzle[2] = PIPE_SWIZZLE_Y;
      }
   /* In v3d 7.x we no longer have a reverse flag for the border color. Instead
   * we have to use the new reverse and swap_r/b flags in the texture shader
   * state which will apply the format swizzle automatically when sampling
   * the border color too and we should not apply it manually here.
   */
   else if (v3d_has_reverse_swap_rb_bits &&
            (v3dv_format_swizzle_needs_rb_swap(format->planes[0].swizzle) ||
   swizzle[0] = PIPE_SWIZZLE_X;
   swizzle[1] = PIPE_SWIZZLE_Y;
   swizzle[2] = PIPE_SWIZZLE_Z;
      } else {
                  union pipe_color_union border;
   for (int i = 0; i < 4; i++) {
      if (format->planes[0].swizzle[i] <= 3)
         else
               /* handle clamping */
   if (vk_format_has_depth(bc_info->format) &&
      vk_format_has_stencil(bc_info->format)) {
   border.f[0] = CLAMP(border.f[0], 0, 1);
      } else if (vk_format_is_unorm(bc_info->format)) {
      for (int i = 0; i < 4; i++)
      } else if (vk_format_is_snorm(bc_info->format)) {
      for (int i = 0; i < 4; i++)
      } else if (vk_format_is_uint(bc_info->format) &&
            for (int i = 0; i < 4; i++)
      } else if (vk_format_is_sint(bc_info->format) &&
            for (int i = 0; i < 4; i++)
      border.i[i] = CLAMP(border.i[i],
               #if V3D_VERSION <= 42
      /* The TMU in V3D 7.x always takes 32-bit floats and handles conversions
   * for us. In V3D 4.x we need to manually convert floating point color
   * values to the expected format.
   */
   if (vk_format_is_srgb(bc_info->format) ||
      vk_format_is_compressed(bc_info->format)) {
   for (int i = 0; i < 4; i++)
      } else if (vk_format_is_unorm(bc_info->format)) {
      for (int i = 0; i < 4; i++) {
      switch (desc->channel[i].size) {
   case 8:
   case 16:
      /* expect u16 for non depth values */
   if (!vk_format_has_depth(bc_info->format))
            case 24:
   case 32:
      /* uses full f32; no conversion needed */
      default:
      border.ui[i] = _mesa_float_to_half(border.f[i]);
            } else if (vk_format_is_snorm(bc_info->format)) {
      for (int i = 0; i < 4; i++) {
      switch (desc->channel[i].size) {
   case 8:
      border.ui[i] = (int32_t) (border.f[i] * (float) 0x3fff);
      case 16:
      border.i[i] = (int32_t) (border.f[i] * (float) 0x7fff);
      case 24:
   case 32:
      /* uses full f32; no conversion needed */
      default:
      border.ui[i] = _mesa_float_to_half(border.f[i]);
            } else if (vk_format_is_float(bc_info->format)) {
      for (int i = 0; i < 4; i++) {
      switch(desc->channel[i].size) {
   case 16:
      border.ui[i] = _mesa_float_to_half(border.f[i]);
      default:
                  #endif
            }
      void
   v3dX(pack_sampler_state)(const struct v3dv_device *device,
                     {
               switch (pCreateInfo->borderColor) {
   case VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK:
   case VK_BORDER_COLOR_INT_TRANSPARENT_BLACK:
      border_color_mode = V3D_BORDER_COLOR_0000;
      case VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK:
   case VK_BORDER_COLOR_INT_OPAQUE_BLACK:
      border_color_mode = V3D_BORDER_COLOR_0001;
      case VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE:
   case VK_BORDER_COLOR_INT_OPAQUE_WHITE:
      border_color_mode = V3D_BORDER_COLOR_1111;
      case VK_BORDER_COLOR_FLOAT_CUSTOM_EXT:
   case VK_BORDER_COLOR_INT_CUSTOM_EXT:
      border_color_mode = V3D_BORDER_COLOR_FOLLOWS;
      default:
      unreachable("Unknown border color");
               v3dvx_pack(sampler->sampler_state, SAMPLER_STATE, s) {
      if (pCreateInfo->anisotropyEnable) {
      s.anisotropy_enable = true;
   if (pCreateInfo->maxAnisotropy > 8)
         else if (pCreateInfo->maxAnisotropy > 4)
         else if (pCreateInfo->maxAnisotropy > 2)
                        if (s.border_color_mode == V3D_BORDER_COLOR_FOLLOWS) {
               s.border_color_word_0 = border.ui[0];
   s.border_color_word_1 = border.ui[1];
   s.border_color_word_2 = border.ui[2];
               s.wrap_i_border = false; /* Also hardcoded on v3d */
   s.wrap_s = vk_to_v3d_wrap_mode[pCreateInfo->addressModeU];
   s.wrap_t = vk_to_v3d_wrap_mode[pCreateInfo->addressModeV];
   s.wrap_r = vk_to_v3d_wrap_mode[pCreateInfo->addressModeW];
   s.fixed_bias = pCreateInfo->mipLodBias;
   s.max_level_of_detail = MIN2(MAX2(0, pCreateInfo->maxLod), 15);
   s.min_level_of_detail = MIN2(MAX2(0, pCreateInfo->minLod), 15);
   s.srgb_disable = 0; /* Not even set by v3d */
   s.depth_compare_function =
      vk_to_v3d_compare_func[pCreateInfo->compareEnable ?
      s.mip_filter_nearest = pCreateInfo->mipmapMode == VK_SAMPLER_MIPMAP_MODE_NEAREST;
   s.min_filter_nearest = pCreateInfo->minFilter == VK_FILTER_NEAREST;
         }
      /**
   * This computes the maximum bpp used by any of the render targets used by
   * a particular subpass and checks if any of those render targets are
   * multisampled. If we don't have a subpass (when we are not inside a
   * render pass), then we assume that all framebuffer attachments are used.
   */
   void
   v3dX(framebuffer_compute_internal_bpp_msaa)(
      const struct v3dv_framebuffer *framebuffer,
   const struct v3dv_cmd_buffer_attachment_state *attachments,
   const struct v3dv_subpass *subpass,
   uint8_t *max_internal_bpp,
   uint8_t *total_color_bpp,
      {
      STATIC_ASSERT(V3D_INTERNAL_BPP_32 == 0);
   *max_internal_bpp = V3D_INTERNAL_BPP_32;
   *total_color_bpp = 0;
            if (subpass) {
      for (uint32_t i = 0; i < subpass->color_count; i++) {
      uint32_t att_idx = subpass->color_attachments[i].attachment;
                  const struct v3dv_image_view *att = attachments[att_idx].image_view;
                  if (att->vk.aspects & VK_IMAGE_ASPECT_COLOR_BIT) {
      const uint32_t internal_bpp = att->planes[0].internal_bpp;
   *max_internal_bpp = MAX2(*max_internal_bpp, internal_bpp);
               if (att->vk.image->samples > VK_SAMPLE_COUNT_1_BIT)
               if (!*msaa && subpass->ds_attachment.attachment != VK_ATTACHMENT_UNUSED) {
      const struct v3dv_image_view *att =
                  if (att->vk.image->samples > VK_SAMPLE_COUNT_1_BIT)
      }
               assert(framebuffer->attachment_count <= 4);
   for (uint32_t i = 0; i < framebuffer->attachment_count; i++) {
      const struct v3dv_image_view *att = attachments[i].image_view;
   assert(att);
            if (att->vk.aspects & VK_IMAGE_ASPECT_COLOR_BIT) {
      const uint32_t internal_bpp = att->planes[0].internal_bpp;
   *max_internal_bpp = MAX2(*max_internal_bpp, internal_bpp);
               if (att->vk.image->samples > VK_SAMPLE_COUNT_1_BIT)
                  }
      uint32_t
   v3dX(zs_buffer_from_aspect_bits)(VkImageAspectFlags aspects)
   {
      const VkImageAspectFlags zs_aspects =
                  if (filtered_aspects == zs_aspects)
         else if (filtered_aspects == VK_IMAGE_ASPECT_DEPTH_BIT)
         else if (filtered_aspects == VK_IMAGE_ASPECT_STENCIL_BIT)
         else
      }
      void
   v3dX(get_hw_clear_color)(const VkClearColorValue *color,
                     {
      union util_color uc;
   switch (internal_type) {
   case V3D_INTERNAL_TYPE_8:
      util_pack_color(color->float32, PIPE_FORMAT_R8G8B8A8_UNORM, &uc);
      break;
   case V3D_INTERNAL_TYPE_8I:
   case V3D_INTERNAL_TYPE_8UI:
      hw_color[0] = ((color->uint32[0] & 0xff) |
                  break;
   case V3D_INTERNAL_TYPE_16F:
      util_pack_color(color->float32, PIPE_FORMAT_R16G16B16A16_FLOAT, &uc);
      break;
   case V3D_INTERNAL_TYPE_16I:
   case V3D_INTERNAL_TYPE_16UI:
      hw_color[0] = ((color->uint32[0] & 0xffff) | color->uint32[1] << 16);
      break;
   case V3D_INTERNAL_TYPE_32F:
   case V3D_INTERNAL_TYPE_32I:
   case V3D_INTERNAL_TYPE_32UI:
      memcpy(hw_color, color->uint32, internal_size);
         }
      #ifdef DEBUG
   void
   v3dX(device_check_prepacked_sizes)(void)
   {
      STATIC_ASSERT(V3DV_SAMPLER_STATE_LENGTH >=
         STATIC_ASSERT(V3DV_TEXTURE_SHADER_STATE_LENGTH >=
         STATIC_ASSERT(V3DV_SAMPLER_STATE_LENGTH >=
         STATIC_ASSERT(V3DV_BLEND_CFG_LENGTH>=
         STATIC_ASSERT(V3DV_CFG_BITS_LENGTH>=
         STATIC_ASSERT(V3DV_GL_SHADER_STATE_RECORD_LENGTH >=
         STATIC_ASSERT(V3DV_VCM_CACHE_SIZE_LENGTH>=
         STATIC_ASSERT(V3DV_GL_SHADER_STATE_ATTRIBUTE_RECORD_LENGTH >=
         STATIC_ASSERT(V3DV_STENCIL_CFG_LENGTH >=
      }
   #endif
