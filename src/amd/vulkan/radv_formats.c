   /*
   * Copyright © 2016 Red Hat.
   * Copyright © 2016 Bas Nieuwenhuizen
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
      #include "radv_debug.h"
   #include "radv_private.h"
      #include "sid.h"
   #include "vk_format.h"
      #include "vk_android.h"
   #include "vk_util.h"
      #include "util/format_r11g11b10f.h"
   #include "util/format_rgb9e5.h"
   #include "util/format_srgb.h"
   #include "util/half_float.h"
   #include "vulkan/util/vk_enum_defines.h"
   #include "vulkan/util/vk_format.h"
   #include "ac_drm_fourcc.h"
      uint32_t
   radv_translate_buffer_dataformat(const struct util_format_description *desc, int first_non_void)
   {
      unsigned type;
                     if (desc->format == PIPE_FORMAT_R11G11B10_FLOAT)
            if (first_non_void < 0)
                  if (type == UTIL_FORMAT_TYPE_FIXED)
         if (desc->nr_channels == 4 && desc->channel[0].size == 10 && desc->channel[1].size == 10 &&
      desc->channel[2].size == 10 && desc->channel[3].size == 2)
         /* See whether the components are of the same size. */
   for (i = 0; i < desc->nr_channels; i++) {
      if (desc->channel[first_non_void].size != desc->channel[i].size)
               switch (desc->channel[first_non_void].size) {
   case 8:
      switch (desc->nr_channels) {
   case 1:
         case 2:
         case 4:
         }
      case 16:
      switch (desc->nr_channels) {
   case 1:
         case 2:
         case 4:
         }
      case 32:
      /* From the Southern Islands ISA documentation about MTBUF:
   * 'Memory reads of data in memory that is 32 or 64 bits do not
   * undergo any format conversion.'
   */
   if (type != UTIL_FORMAT_TYPE_FLOAT && !desc->channel[first_non_void].pure_integer)
            switch (desc->nr_channels) {
   case 1:
         case 2:
         case 3:
         case 4:
         }
      case 64:
      if (type != UTIL_FORMAT_TYPE_FLOAT && desc->nr_channels == 1)
                  }
      uint32_t
   radv_translate_buffer_numformat(const struct util_format_description *desc, int first_non_void)
   {
               if (desc->format == PIPE_FORMAT_R11G11B10_FLOAT)
            if (first_non_void < 0)
            switch (desc->channel[first_non_void].type) {
   case UTIL_FORMAT_TYPE_SIGNED:
      if (desc->channel[first_non_void].normalized)
         else if (desc->channel[first_non_void].pure_integer)
         else
            case UTIL_FORMAT_TYPE_UNSIGNED:
      if (desc->channel[first_non_void].normalized)
         else if (desc->channel[first_non_void].pure_integer)
         else
            case UTIL_FORMAT_TYPE_FLOAT:
   default:
            }
      static bool
   radv_is_vertex_buffer_format_supported(VkFormat format)
   {
      if (format == VK_FORMAT_B10G11R11_UFLOAT_PACK32)
         if (format == VK_FORMAT_UNDEFINED || vk_format_is_srgb(format))
            int first_non_void = vk_format_get_first_non_void_channel(format);
   if (first_non_void < 0)
            const struct util_format_description *desc = vk_format_description(format);
   unsigned type = desc->channel[first_non_void].type;
   if (type == UTIL_FORMAT_TYPE_FIXED)
            if (desc->nr_channels == 4 && desc->channel[0].size == 10 && desc->channel[1].size == 10 &&
      desc->channel[2].size == 10 && desc->channel[3].size == 2)
         switch (desc->channel[first_non_void].size) {
   case 8:
   case 16:
   case 32:
   case 64:
         default:
            }
      uint32_t
   radv_translate_tex_dataformat(VkFormat format, const struct util_format_description *desc, int first_non_void)
   {
      bool uniform = true;
                     /* Colorspace (return non-RGB formats directly). */
   switch (desc->colorspace) {
         case UTIL_FORMAT_COLORSPACE_ZS:
      switch (format) {
   case VK_FORMAT_D16_UNORM:
         case VK_FORMAT_D24_UNORM_S8_UINT:
   case VK_FORMAT_X8_D24_UNORM_PACK32:
         case VK_FORMAT_S8_UINT:
         case VK_FORMAT_D32_SFLOAT:
         case VK_FORMAT_D32_SFLOAT_S8_UINT:
         default:
               case UTIL_FORMAT_COLORSPACE_YUV:
            default:
                  if (desc->layout == UTIL_FORMAT_LAYOUT_SUBSAMPLED) {
      switch (format) {
   /* Don't ask me why this looks inverted. PAL does the same. */
   case VK_FORMAT_G8B8G8R8_422_UNORM:
         case VK_FORMAT_B8G8R8G8_422_UNORM:
         default:
                     if (desc->layout == UTIL_FORMAT_LAYOUT_RGTC) {
      switch (format) {
   case VK_FORMAT_BC4_UNORM_BLOCK:
   case VK_FORMAT_BC4_SNORM_BLOCK:
         case VK_FORMAT_BC5_UNORM_BLOCK:
   case VK_FORMAT_BC5_SNORM_BLOCK:
         default:
                     if (desc->layout == UTIL_FORMAT_LAYOUT_S3TC) {
      switch (format) {
   case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
   case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
   case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
   case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
         case VK_FORMAT_BC2_UNORM_BLOCK:
   case VK_FORMAT_BC2_SRGB_BLOCK:
         case VK_FORMAT_BC3_UNORM_BLOCK:
   case VK_FORMAT_BC3_SRGB_BLOCK:
         default:
                     if (desc->layout == UTIL_FORMAT_LAYOUT_BPTC) {
      switch (format) {
   case VK_FORMAT_BC6H_UFLOAT_BLOCK:
   case VK_FORMAT_BC6H_SFLOAT_BLOCK:
         case VK_FORMAT_BC7_UNORM_BLOCK:
   case VK_FORMAT_BC7_SRGB_BLOCK:
         default:
                     if (desc->layout == UTIL_FORMAT_LAYOUT_ETC) {
      switch (format) {
   case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
   case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
         case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK:
   case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
         case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK:
   case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
         case VK_FORMAT_EAC_R11_UNORM_BLOCK:
   case VK_FORMAT_EAC_R11_SNORM_BLOCK:
         case VK_FORMAT_EAC_R11G11_UNORM_BLOCK:
   case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
         default:
                     if (format == VK_FORMAT_E5B9G9R9_UFLOAT_PACK32) {
         } else if (format == VK_FORMAT_B10G11R11_UFLOAT_PACK32) {
                           /* hw cannot support mixed formats (except depth/stencil, since only
   * depth is read).*/
   if (desc->is_mixed && desc->colorspace != UTIL_FORMAT_COLORSPACE_ZS)
            /* See whether the components are of the same size. */
   for (i = 1; i < desc->nr_channels; i++) {
                  /* Non-uniform formats. */
   if (!uniform) {
      switch (desc->nr_channels) {
   case 3:
      if (desc->channel[0].size == 5 && desc->channel[1].size == 6 && desc->channel[2].size == 5) {
         }
      case 4:
      if (desc->channel[0].size == 5 && desc->channel[1].size == 5 && desc->channel[2].size == 5 &&
      desc->channel[3].size == 1) {
      }
   if (desc->channel[0].size == 1 && desc->channel[1].size == 5 && desc->channel[2].size == 5 &&
      desc->channel[3].size == 5) {
      }
   if (desc->channel[0].size == 10 && desc->channel[1].size == 10 && desc->channel[2].size == 10 &&
      desc->channel[3].size == 2) {
   /* Closed VK driver does this also no 2/10/10/10 snorm */
   if (desc->channel[0].type == UTIL_FORMAT_TYPE_SIGNED && desc->channel[0].normalized)
            }
      }
               if (first_non_void < 0 || first_non_void > 3)
            /* uniform formats */
   switch (desc->channel[first_non_void].size) {
   case 4:
      #if 0 /* Not supported for render targets */
   case 2:
         #endif
         case 4:
         }
      case 8:
      switch (desc->nr_channels) {
   case 1:
         case 2:
         case 4:
         }
      case 16:
      switch (desc->nr_channels) {
   case 1:
         case 2:
         case 4:
         }
      case 32:
      switch (desc->nr_channels) {
   case 1:
         case 2:
         case 3:
         case 4:
         }
      case 64:
      if (desc->channel[0].type != UTIL_FORMAT_TYPE_FLOAT && desc->nr_channels == 1)
                  out_unknown:
      /* R600_ERR("Unable to handle texformat %d %s\n", format, vk_format_name(format)); */
      }
      uint32_t
   radv_translate_tex_numformat(VkFormat format, const struct util_format_description *desc, int first_non_void)
   {
               switch (format) {
   case VK_FORMAT_D24_UNORM_S8_UINT:
         default:
      if (first_non_void < 0) {
      if (vk_format_is_compressed(format)) {
      switch (format) {
   case VK_FORMAT_BC1_RGB_SRGB_BLOCK:
   case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:
   case VK_FORMAT_BC2_SRGB_BLOCK:
   case VK_FORMAT_BC3_SRGB_BLOCK:
   case VK_FORMAT_BC7_SRGB_BLOCK:
   case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
   case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK:
   case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK:
         case VK_FORMAT_BC4_SNORM_BLOCK:
   case VK_FORMAT_BC5_SNORM_BLOCK:
   case VK_FORMAT_BC6H_SFLOAT_BLOCK:
   case VK_FORMAT_EAC_R11_SNORM_BLOCK:
   case VK_FORMAT_EAC_R11G11_SNORM_BLOCK:
         default:
            } else if (desc->layout == UTIL_FORMAT_LAYOUT_SUBSAMPLED) {
         } else {
            } else if (desc->colorspace == UTIL_FORMAT_COLORSPACE_SRGB) {
         } else {
      switch (desc->channel[first_non_void].type) {
   case UTIL_FORMAT_TYPE_FLOAT:
         case UTIL_FORMAT_TYPE_SIGNED:
      if (desc->channel[first_non_void].normalized)
         else if (desc->channel[first_non_void].pure_integer)
         else
      case UTIL_FORMAT_TYPE_UNSIGNED:
      if (desc->channel[first_non_void].normalized)
         else if (desc->channel[first_non_void].pure_integer)
         else
      default:
                  }
      static bool
   radv_is_sampler_format_supported(VkFormat format, bool *linear_sampling)
   {
      const struct util_format_description *desc = vk_format_description(format);
   uint32_t num_format;
   if (format == VK_FORMAT_UNDEFINED || format == VK_FORMAT_R64_UINT || format == VK_FORMAT_R64_SINT)
                  if (num_format == V_008F14_IMG_NUM_FORMAT_USCALED || num_format == V_008F14_IMG_NUM_FORMAT_SSCALED)
            if (num_format == V_008F14_IMG_NUM_FORMAT_UNORM || num_format == V_008F14_IMG_NUM_FORMAT_SNORM ||
      num_format == V_008F14_IMG_NUM_FORMAT_FLOAT || num_format == V_008F14_IMG_NUM_FORMAT_SRGB)
      else
         return radv_translate_tex_dataformat(format, vk_format_description(format),
      }
      bool
   radv_is_atomic_format_supported(VkFormat format)
   {
      return format == VK_FORMAT_R32_UINT || format == VK_FORMAT_R32_SINT || format == VK_FORMAT_R32_SFLOAT ||
      }
      bool
   radv_is_storage_image_format_supported(const struct radv_physical_device *physical_device, VkFormat format)
   {
      const struct util_format_description *desc = vk_format_description(format);
   unsigned data_format, num_format;
   if (format == VK_FORMAT_UNDEFINED)
            if (vk_format_is_depth_or_stencil(format))
            data_format = radv_translate_tex_dataformat(format, desc, vk_format_get_first_non_void_channel(format));
            if (data_format == ~0 || num_format == ~0)
            /* Extracted from the GCN3 ISA document. */
   switch (num_format) {
   case V_008F14_IMG_NUM_FORMAT_UNORM:
   case V_008F14_IMG_NUM_FORMAT_SNORM:
   case V_008F14_IMG_NUM_FORMAT_UINT:
   case V_008F14_IMG_NUM_FORMAT_SINT:
   case V_008F14_IMG_NUM_FORMAT_FLOAT:
         default:
                  switch (data_format) {
   case V_008F14_IMG_DATA_FORMAT_8:
   case V_008F14_IMG_DATA_FORMAT_16:
   case V_008F14_IMG_DATA_FORMAT_8_8:
   case V_008F14_IMG_DATA_FORMAT_32:
   case V_008F14_IMG_DATA_FORMAT_16_16:
   case V_008F14_IMG_DATA_FORMAT_10_11_11:
   case V_008F14_IMG_DATA_FORMAT_11_11_10:
   case V_008F14_IMG_DATA_FORMAT_10_10_10_2:
   case V_008F14_IMG_DATA_FORMAT_2_10_10_10:
   case V_008F14_IMG_DATA_FORMAT_8_8_8_8:
   case V_008F14_IMG_DATA_FORMAT_32_32:
   case V_008F14_IMG_DATA_FORMAT_16_16_16_16:
   case V_008F14_IMG_DATA_FORMAT_32_32_32_32:
   case V_008F14_IMG_DATA_FORMAT_5_6_5:
   case V_008F14_IMG_DATA_FORMAT_1_5_5_5:
   case V_008F14_IMG_DATA_FORMAT_5_5_5_1:
   case V_008F14_IMG_DATA_FORMAT_4_4_4_4:
      /* TODO: FMASK formats. */
      case V_008F14_IMG_DATA_FORMAT_5_9_9_9:
         default:
            }
      bool
   radv_is_buffer_format_supported(VkFormat format, bool *scaled)
   {
      const struct util_format_description *desc = vk_format_description(format);
   unsigned data_format, num_format;
   if (format == VK_FORMAT_UNDEFINED)
            data_format = radv_translate_buffer_dataformat(desc, vk_format_get_first_non_void_channel(format));
            if (scaled)
            }
      bool
   radv_is_colorbuffer_format_supported(const struct radv_physical_device *pdevice, VkFormat format, bool *blendable)
   {
      const struct util_format_description *desc = vk_format_description(format);
   uint32_t color_format = ac_get_cb_format(pdevice->rad_info.gfx_level, desc->format);
   uint32_t color_swap = radv_translate_colorswap(format, false);
            if (color_num_format == V_028C70_NUMBER_UINT || color_num_format == V_028C70_NUMBER_SINT ||
      color_format == V_028C70_COLOR_8_24 || color_format == V_028C70_COLOR_24_8 ||
   color_format == V_028C70_COLOR_X24_8_32_FLOAT) {
      } else
            if (format == VK_FORMAT_E5B9G9R9_UFLOAT_PACK32 && pdevice->rad_info.gfx_level < GFX10_3)
               }
      static bool
   radv_is_zs_format_supported(VkFormat format)
   {
         }
      static bool
   radv_is_filter_minmax_format_supported(VkFormat format)
   {
      /* From the Vulkan spec 1.1.71:
   *
   * "The following formats must support the
   *  VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_FILTER_MINMAX_BIT feature with
   *  VK_IMAGE_TILING_OPTIMAL, if they support
   *  VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_BIT."
   */
   /* TODO: enable more formats. */
   switch (format) {
   case VK_FORMAT_R8_UNORM:
   case VK_FORMAT_R8_SNORM:
   case VK_FORMAT_R16_UNORM:
   case VK_FORMAT_R16_SNORM:
   case VK_FORMAT_R16_SFLOAT:
   case VK_FORMAT_R32_SFLOAT:
   case VK_FORMAT_D16_UNORM:
   case VK_FORMAT_X8_D24_UNORM_PACK32:
   case VK_FORMAT_D32_SFLOAT:
   case VK_FORMAT_D16_UNORM_S8_UINT:
   case VK_FORMAT_D24_UNORM_S8_UINT:
   case VK_FORMAT_D32_SFLOAT_S8_UINT:
         default:
            }
      bool
   radv_is_format_emulated(const struct radv_physical_device *physical_device, VkFormat format)
   {
      if (physical_device->emulate_etc2 && vk_texcompress_etc2_emulation_format(format) != VK_FORMAT_UNDEFINED)
            if (physical_device->emulate_astc && vk_texcompress_astc_emulation_format(format) != VK_FORMAT_UNDEFINED)
               }
      bool
   radv_device_supports_etc(const struct radv_physical_device *physical_device)
   {
      return physical_device->rad_info.family == CHIP_VEGA10 || physical_device->rad_info.family == CHIP_RAVEN ||
      }
      static void
   radv_physical_device_get_format_properties(struct radv_physical_device *physical_device, VkFormat format,
         {
      VkFormatFeatureFlags2 linear = 0, tiled = 0, buffer = 0;
   const struct util_format_description *desc = vk_format_description(format);
   bool blendable;
   bool scaled = false;
   /* TODO: implement some software emulation of SUBSAMPLED formats. */
   if (desc->format == PIPE_FORMAT_NONE || desc->layout == UTIL_FORMAT_LAYOUT_SUBSAMPLED) {
      out_properties->linearTilingFeatures = linear;
   out_properties->optimalTilingFeatures = tiled;
   out_properties->bufferFeatures = buffer;
               if ((desc->layout == UTIL_FORMAT_LAYOUT_ETC && !radv_device_supports_etc(physical_device)) ||
      desc->layout == UTIL_FORMAT_LAYOUT_ASTC) {
   if (radv_is_format_emulated(physical_device, format)) {
      /* required features for compressed formats */
   tiled = VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_FILTER_LINEAR_BIT |
            }
   out_properties->linearTilingFeatures = linear;
   out_properties->optimalTilingFeatures = tiled;
   out_properties->bufferFeatures = buffer;
               const bool multiplanar = vk_format_get_plane_count(format) > 1;
   if (multiplanar || desc->layout == UTIL_FORMAT_LAYOUT_SUBSAMPLED) {
      uint64_t tiling = VK_FORMAT_FEATURE_2_TRANSFER_SRC_BIT | VK_FORMAT_FEATURE_2_TRANSFER_DST_BIT |
            if (vk_format_get_ycbcr_info(format)) {
               /* The subsampled formats have no support for linear filters. */
   if (desc->layout != UTIL_FORMAT_LAYOUT_SUBSAMPLED)
               if (physical_device->instance->perftest_flags & RADV_PERFTEST_VIDEO_DECODE) {
      if (format == VK_FORMAT_G8_B8R8_2PLANE_420_UNORM ||
      format == VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16)
            if (multiplanar)
            /* Fails for unknown reasons with linear tiling & subsampled formats. */
   out_properties->linearTilingFeatures = desc->layout == UTIL_FORMAT_LAYOUT_SUBSAMPLED ? 0 : tiling;
   out_properties->optimalTilingFeatures = tiling;
   out_properties->bufferFeatures = 0;
               if (radv_is_storage_image_format_supported(physical_device, format)) {
      tiled |= VK_FORMAT_FEATURE_2_STORAGE_IMAGE_BIT | VK_FORMAT_FEATURE_2_STORAGE_READ_WITHOUT_FORMAT_BIT |
         linear |= VK_FORMAT_FEATURE_2_STORAGE_IMAGE_BIT | VK_FORMAT_FEATURE_2_STORAGE_READ_WITHOUT_FORMAT_BIT |
               if (radv_is_vertex_buffer_format_supported(format))
            if (radv_is_buffer_format_supported(format, &scaled)) {
      if (format != VK_FORMAT_R64_UINT && format != VK_FORMAT_R64_SINT && !scaled && !vk_format_is_srgb(format))
         buffer |= VK_FORMAT_FEATURE_2_STORAGE_TEXEL_BUFFER_BIT | VK_FORMAT_FEATURE_2_STORAGE_READ_WITHOUT_FORMAT_BIT |
               if (vk_format_is_depth_or_stencil(format)) {
      if (radv_is_zs_format_supported(format)) {
      tiled |= VK_FORMAT_FEATURE_2_DEPTH_STENCIL_ATTACHMENT_BIT;
   tiled |= VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_BIT;
                                 if (vk_format_has_depth(format)) {
      tiled |= VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_FILTER_LINEAR_BIT |
               /* Don't support blitting surfaces with depth/stencil. */
                  /* Don't support linear depth surfaces */
         } else {
      bool linear_sampling;
   if (radv_is_sampler_format_supported(format, &linear_sampling)) {
                                    if (linear_sampling) {
      linear |= VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
               /* Don't support blitting for R32G32B32 formats. */
   if (format == VK_FORMAT_R32G32B32_SFLOAT || format == VK_FORMAT_R32G32B32_UINT ||
      format == VK_FORMAT_R32G32B32_SINT) {
         }
   if (radv_is_colorbuffer_format_supported(physical_device, format, &blendable) && desc->channel[0].size != 64) {
      linear |= VK_FORMAT_FEATURE_2_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_2_BLIT_DST_BIT;
   tiled |= VK_FORMAT_FEATURE_2_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_2_BLIT_DST_BIT;
   if (blendable) {
      linear |= VK_FORMAT_FEATURE_2_COLOR_ATTACHMENT_BLEND_BIT;
         }
   if (tiled && !scaled) {
                  /* Tiled formatting does not support NPOT pixel sizes */
   if (!util_is_power_of_two_or_zero(vk_format_get_blocksize(format)))
               if (linear && !scaled) {
                  if (radv_is_atomic_format_supported(format)) {
      buffer |= VK_FORMAT_FEATURE_2_STORAGE_TEXEL_BUFFER_ATOMIC_BIT;
   linear |= VK_FORMAT_FEATURE_2_STORAGE_IMAGE_ATOMIC_BIT;
               switch (format) {
   case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
   case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
   case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
   case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
   case VK_FORMAT_A2R10G10B10_SINT_PACK32:
   case VK_FORMAT_A2B10G10R10_SINT_PACK32:
      buffer &= ~(VK_FORMAT_FEATURE_2_UNIFORM_TEXEL_BUFFER_BIT | VK_FORMAT_FEATURE_2_STORAGE_TEXEL_BUFFER_BIT);
   linear = 0;
   tiled = 0;
      case VK_FORMAT_R64_UINT:
   case VK_FORMAT_R64_SINT:
   case VK_FORMAT_R64_SFLOAT:
      tiled |= VK_FORMAT_FEATURE_2_TRANSFER_SRC_BIT | VK_FORMAT_FEATURE_2_TRANSFER_DST_BIT;
   linear |= VK_FORMAT_FEATURE_2_TRANSFER_SRC_BIT | VK_FORMAT_FEATURE_2_TRANSFER_DST_BIT;
      default:
                  switch (format) {
   case VK_FORMAT_R32G32_SFLOAT:
   case VK_FORMAT_R32G32B32_SFLOAT:
   case VK_FORMAT_R32G32B32A32_SFLOAT:
   case VK_FORMAT_R16G16_SFLOAT:
   case VK_FORMAT_R16G16B16_SFLOAT:
   case VK_FORMAT_R16G16B16A16_SFLOAT:
   case VK_FORMAT_R16G16_SNORM:
   case VK_FORMAT_R16G16_UNORM:
   case VK_FORMAT_R16G16B16A16_SNORM:
   case VK_FORMAT_R16G16B16A16_UNORM:
   case VK_FORMAT_R8G8_SNORM:
   case VK_FORMAT_R8G8_UNORM:
   case VK_FORMAT_R8G8B8A8_SNORM:
   case VK_FORMAT_R8G8B8A8_UNORM:
   case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
      buffer |= VK_FORMAT_FEATURE_2_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR;
      default:
         }
   /* addrlib does not support linear compressed textures. */
   if (vk_format_is_compressed(format))
            /* From the Vulkan spec 1.2.163:
   *
   * "VK_FORMAT_FEATURE_2_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT must be supported for the
   *  following formats if the attachmentFragmentShadingRate feature is supported:"
   *
   * - VK_FORMAT_R8_UINT
   */
   if (format == VK_FORMAT_R8_UINT) {
                  /* It's invalid to expose buffer features with depth/stencil formats. */
   if (vk_format_is_depth_or_stencil(format)) {
                  out_properties->linearTilingFeatures = linear;
   out_properties->optimalTilingFeatures = tiled;
      }
      uint32_t
   radv_colorformat_endian_swap(uint32_t colorformat)
   {
      if (0 /*UTIL_ARCH_BIG_ENDIAN*/) {
      switch (colorformat) {
         case V_028C70_COLOR_8:
                  case V_028C70_COLOR_5_6_5:
   case V_028C70_COLOR_1_5_5_5:
   case V_028C70_COLOR_4_4_4_4:
   case V_028C70_COLOR_16:
   case V_028C70_COLOR_8_8:
                  case V_028C70_COLOR_8_8_8_8:
   case V_028C70_COLOR_2_10_10_10:
   case V_028C70_COLOR_8_24:
   case V_028C70_COLOR_24_8:
   case V_028C70_COLOR_16_16:
                  case V_028C70_COLOR_16_16_16_16:
            case V_028C70_COLOR_32_32:
                  case V_028C70_COLOR_32_32_32_32:
         default:
            } else {
            }
      uint32_t
   radv_translate_dbformat(VkFormat format)
   {
      switch (format) {
   case VK_FORMAT_D16_UNORM:
   case VK_FORMAT_D16_UNORM_S8_UINT:
         case VK_FORMAT_D32_SFLOAT:
   case VK_FORMAT_D32_SFLOAT_S8_UINT:
         default:
            }
      unsigned
   radv_translate_colorswap(VkFormat format, bool do_endian_swap)
   {
            #define HAS_SWIZZLE(chan, swz) (desc->swizzle[chan] == PIPE_SWIZZLE_##swz)
         if (format == VK_FORMAT_B10G11R11_UFLOAT_PACK32)
            if (format == VK_FORMAT_E5B9G9R9_UFLOAT_PACK32)
            if (desc->layout != UTIL_FORMAT_LAYOUT_PLAIN)
            switch (desc->nr_channels) {
   case 1:
      if (HAS_SWIZZLE(0, X))
         else if (HAS_SWIZZLE(3, X))
            case 2:
      if ((HAS_SWIZZLE(0, X) && HAS_SWIZZLE(1, Y)) || (HAS_SWIZZLE(0, X) && HAS_SWIZZLE(1, NONE)) ||
      (HAS_SWIZZLE(0, NONE) && HAS_SWIZZLE(1, Y)))
      else if ((HAS_SWIZZLE(0, Y) && HAS_SWIZZLE(1, X)) || (HAS_SWIZZLE(0, Y) && HAS_SWIZZLE(1, NONE)) ||
            /* YX__ */
      else if (HAS_SWIZZLE(0, X) && HAS_SWIZZLE(3, Y))
         else if (HAS_SWIZZLE(0, Y) && HAS_SWIZZLE(3, X))
            case 3:
      if (HAS_SWIZZLE(0, X))
         else if (HAS_SWIZZLE(0, Z))
            case 4:
      /* check the middle channels, the 1st and 4th channel can be NONE */
   if (HAS_SWIZZLE(1, Y) && HAS_SWIZZLE(2, Z)) {
         } else if (HAS_SWIZZLE(1, Z) && HAS_SWIZZLE(2, Y)) {
         } else if (HAS_SWIZZLE(1, Y) && HAS_SWIZZLE(2, X)) {
         } else if (HAS_SWIZZLE(1, Z) && HAS_SWIZZLE(2, W)) {
      /* YZWX */
   if (desc->is_array)
         else
      }
      }
      }
      bool
   radv_format_pack_clear_color(VkFormat format, uint32_t clear_vals[2], VkClearColorValue *value)
   {
               if (format == VK_FORMAT_B10G11R11_UFLOAT_PACK32) {
      clear_vals[0] = float3_to_r11g11b10f(value->float32);
   clear_vals[1] = 0;
      } else if (format == VK_FORMAT_E5B9G9R9_UFLOAT_PACK32) {
      clear_vals[0] = float3_to_rgb9e5(value->float32);
   clear_vals[1] = 0;
               if (desc->layout != UTIL_FORMAT_LAYOUT_PLAIN) {
      fprintf(stderr, "failed to fast clear for non-plain format %d\n", format);
               if (!util_is_power_of_two_or_zero(desc->block.bits)) {
      fprintf(stderr, "failed to fast clear for NPOT format %d\n", format);
               if (desc->block.bits > 64) {
      /*
   * We have a 128 bits format, check if the first 3 components are the same.
   * Every elements has to be 32 bits since we don't support 64-bit formats,
   * and we can skip swizzling checks as alpha always comes last for these and
   * we do not care about the rest as they have to be the same.
   */
   if (desc->channel[0].type == UTIL_FORMAT_TYPE_FLOAT) {
      if (value->float32[0] != value->float32[1] || value->float32[0] != value->float32[2])
      } else {
      if (value->uint32[0] != value->uint32[1] || value->uint32[0] != value->uint32[2])
      }
   clear_vals[0] = value->uint32[0];
   clear_vals[1] = value->uint32[3];
      }
            for (unsigned c = 0; c < 4; ++c) {
      if (desc->swizzle[c] >= 4)
            const struct util_format_channel_description *channel = &desc->channel[desc->swizzle[c]];
            uint64_t v = 0;
   if (channel->pure_integer) {
         } else if (channel->normalized) {
      if (channel->type == UTIL_FORMAT_TYPE_UNSIGNED && desc->swizzle[c] < 3 &&
                                          if (channel->type == UTIL_FORMAT_TYPE_UNSIGNED) {
         } else {
                  /* The hardware rounds before conversion. */
   if (f > 0)
                              } else if (channel->type == UTIL_FORMAT_TYPE_FLOAT) {
      if (channel->size == 32) {
         } else if (channel->size == 16) {
         } else {
      fprintf(stderr, "failed to fast clear for unhandled float size in format %d\n", format);
         } else {
      fprintf(stderr, "failed to fast clear for unhandled component type in format %d\n", format);
      }
               clear_vals[0] = clear_val;
               }
      static const struct ac_modifier_options radv_modifier_options = {
      .dcc = true,
      };
      static VkFormatFeatureFlags2
   radv_get_modifier_flags(struct radv_physical_device *dev, VkFormat format, uint64_t modifier,
         {
               if (vk_format_is_compressed(format) || vk_format_is_depth_or_stencil(format))
            if (modifier == DRM_FORMAT_MOD_LINEAR)
         else
            /* Unconditionally disable DISJOINT support for modifiers for now */
            if (ac_modifier_has_dcc(modifier)) {
      /* Only disable support for STORAGE_IMAGE on modifiers that
   * do not support DCC image stores.
   */
   if (!ac_modifier_supports_dcc_image_stores(dev->rad_info.gfx_level, modifier) ||
                  if (dev->instance->debug_flags & (RADV_DEBUG_NO_DCC | RADV_DEBUG_NO_DISPLAY_DCC))
                  }
      static void
   radv_list_drm_format_modifiers(struct radv_physical_device *dev, VkFormat format,
         {
               if (!mod_list)
            if (vk_format_is_compressed(format) || vk_format_is_depth_or_stencil(format)) {
      mod_list->drmFormatModifierCount = 0;
               VK_OUTARRAY_MAKE_TYPED(VkDrmFormatModifierPropertiesEXT, out, mod_list->pDrmFormatModifierProperties,
            ac_get_supported_modifiers(&dev->rad_info, &radv_modifier_options, vk_format_to_pipe_format(format), &mod_count,
            uint64_t *mods = malloc(mod_count * sizeof(uint64_t));
   if (!mods) {
      /* We can't return an error here ... */
   mod_list->drmFormatModifierCount = 0;
      }
   ac_get_supported_modifiers(&dev->rad_info, &radv_modifier_options, vk_format_to_pipe_format(format), &mod_count,
            for (unsigned i = 0; i < mod_count; ++i) {
      VkFormatFeatureFlags2 features = radv_get_modifier_flags(dev, format, mods[i], format_props);
   unsigned planes = vk_format_get_plane_count(format);
   if (planes == 1) {
      if (ac_modifier_has_dcc_retile(mods[i]))
         else if (ac_modifier_has_dcc(mods[i]))
               if (!features)
            vk_outarray_append_typed(VkDrmFormatModifierPropertiesEXT, &out, out_props)
   {
      *out_props = (VkDrmFormatModifierPropertiesEXT){
      .drmFormatModifier = mods[i],
   .drmFormatModifierPlaneCount = planes,
                        }
      static void
   radv_list_drm_format_modifiers_2(struct radv_physical_device *dev, VkFormat format,
               {
               if (!mod_list)
            if (vk_format_is_compressed(format) || vk_format_is_depth_or_stencil(format)) {
      mod_list->drmFormatModifierCount = 0;
               VK_OUTARRAY_MAKE_TYPED(VkDrmFormatModifierProperties2EXT, out, mod_list->pDrmFormatModifierProperties,
            ac_get_supported_modifiers(&dev->rad_info, &radv_modifier_options, vk_format_to_pipe_format(format), &mod_count,
            uint64_t *mods = malloc(mod_count * sizeof(uint64_t));
   if (!mods) {
      /* We can't return an error here ... */
   mod_list->drmFormatModifierCount = 0;
      }
   ac_get_supported_modifiers(&dev->rad_info, &radv_modifier_options, vk_format_to_pipe_format(format), &mod_count,
            for (unsigned i = 0; i < mod_count; ++i) {
      VkFormatFeatureFlags2 features = radv_get_modifier_flags(dev, format, mods[i], format_props);
   unsigned planes = vk_format_get_plane_count(format);
   if (planes == 1) {
      if (ac_modifier_has_dcc_retile(mods[i]))
         else if (ac_modifier_has_dcc(mods[i]))
               if (!features)
            vk_outarray_append_typed(VkDrmFormatModifierProperties2EXT, &out, out_props)
   {
      *out_props = (VkDrmFormatModifierProperties2EXT){
      .drmFormatModifier = mods[i],
   .drmFormatModifierPlaneCount = planes,
                        }
      static VkResult
   radv_check_modifier_support(struct radv_physical_device *dev, const VkPhysicalDeviceImageFormatInfo2 *info,
         {
               if (info->type != VK_IMAGE_TYPE_2D)
            if (radv_is_format_emulated(dev, format))
            /* We did not add modifiers for sparse textures. */
   if (info->flags &
      (VK_IMAGE_CREATE_SPARSE_BINDING_BIT | VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT | VK_IMAGE_CREATE_SPARSE_ALIASED_BIT))
         /*
   * Need to check the modifier is supported in general:
   * "If the drmFormatModifier is incompatible with the parameters specified
   * in VkPhysicalDeviceImageFormatInfo2 and its pNext chain, then
   * vkGetPhysicalDeviceImageFormatProperties2 returns VK_ERROR_FORMAT_NOT_SUPPORTED.
   * The implementation must support the query of any drmFormatModifier,
   * including unknown and invalid modifier values."
   */
   VkDrmFormatModifierPropertiesListEXT mod_list = {
                                    if (!mod_list.drmFormatModifierCount)
            mod_list.pDrmFormatModifierProperties =
         if (!mod_list.pDrmFormatModifierProperties)
                     bool found = false;
   for (uint32_t i = 0; i < mod_list.drmFormatModifierCount && !found; ++i)
      if (mod_list.pDrmFormatModifierProperties[i].drmFormatModifier == modifier)
                  if (!found)
            bool need_dcc_sign_reinterpret = false;
   if (ac_modifier_has_dcc(modifier) &&
      !radv_are_formats_dcc_compatible(dev, info->pNext, format, info->flags, &need_dcc_sign_reinterpret) &&
   !need_dcc_sign_reinterpret)
         /* We can expand this as needed and implemented but there is not much demand
   * for more. */
   if (ac_modifier_has_dcc(modifier)) {
      props->maxMipLevels = 1;
               ac_modifier_max_extent(&dev->rad_info, modifier, &max_width, &max_height);
   props->maxExtent.width = MIN2(props->maxExtent.width, max_width);
            /* We don't support MSAA for modifiers */
   props->sampleCounts &= VK_SAMPLE_COUNT_1_BIT;
      }
      VKAPI_ATTR void VKAPI_CALL
   radv_GetPhysicalDeviceFormatProperties2(VkPhysicalDevice physicalDevice, VkFormat format,
         {
      RADV_FROM_HANDLE(radv_physical_device, physical_device, physicalDevice);
                     pFormatProperties->formatProperties.linearTilingFeatures =
         pFormatProperties->formatProperties.optimalTilingFeatures =
                  VkFormatProperties3 *format_props_extended = vk_find_struct(pFormatProperties, FORMAT_PROPERTIES_3);
   if (format_props_extended) {
      format_props_extended->linearTilingFeatures = format_props.linearTilingFeatures;
   format_props_extended->optimalTilingFeatures = format_props.optimalTilingFeatures;
               radv_list_drm_format_modifiers(physical_device, format, &format_props,
         radv_list_drm_format_modifiers_2(physical_device, format, &format_props,
      }
      static VkResult
   radv_get_image_format_properties(struct radv_physical_device *physical_device,
                  {
      VkFormatProperties3 format_props;
   VkFormatFeatureFlags2 format_feature_flags;
   VkExtent3D maxExtent;
   uint32_t maxMipLevels;
   uint32_t maxArraySize;
   VkSampleCountFlags sampleCounts = VK_SAMPLE_COUNT_1_BIT;
   const struct util_format_description *desc = vk_format_description(format);
   enum amd_gfx_level gfx_level = physical_device->rad_info.gfx_level;
   VkImageTiling tiling = info->tiling;
   const VkPhysicalDeviceImageDrmFormatModifierInfoEXT *mod_info =
                  radv_physical_device_get_format_properties(physical_device, format, &format_props);
   if (tiling == VK_IMAGE_TILING_LINEAR) {
         } else if (tiling == VK_IMAGE_TILING_OPTIMAL) {
         } else if (tiling == VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT) {
      format_feature_flags =
      } else {
                  if (format_feature_flags == 0)
            if (info->type == VK_IMAGE_TYPE_1D && radv_is_format_emulated(physical_device, format))
         if (info->type != VK_IMAGE_TYPE_2D && vk_format_is_depth_or_stencil(format))
            switch (info->type) {
   default:
         case VK_IMAGE_TYPE_1D:
      maxExtent.width = 16384;
   maxExtent.height = 1;
   maxExtent.depth = 1;
   maxMipLevels = 15; /* log2(maxWidth) + 1 */
   maxArraySize = gfx_level >= GFX10 ? 8192 : 2048;
      case VK_IMAGE_TYPE_2D:
      maxExtent.width = 16384;
   maxExtent.height = 16384;
   maxExtent.depth = 1;
   maxMipLevels = 15; /* log2(maxWidth) + 1 */
   maxArraySize = gfx_level >= GFX10 ? 8192 : 2048;
      case VK_IMAGE_TYPE_3D:
      if (gfx_level >= GFX10) {
      maxExtent.width = 8192;
   maxExtent.height = 8192;
      } else {
      maxExtent.width = 2048;
   maxExtent.height = 2048;
      }
   maxMipLevels = util_logbase2(maxExtent.width) + 1;
   maxArraySize = 1;
               if (desc->layout == UTIL_FORMAT_LAYOUT_SUBSAMPLED) {
      /* Might be able to support but the entire format support is
   * messy, so taking the lazy way out. */
               if (tiling == VK_IMAGE_TILING_OPTIMAL && info->type == VK_IMAGE_TYPE_2D &&
      (format_feature_flags &
   (VK_FORMAT_FEATURE_2_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_2_DEPTH_STENCIL_ATTACHMENT_BIT)) &&
   !(info->flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) &&
   !(info->usage & VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR)) {
               if (tiling == VK_IMAGE_TILING_LINEAR && (format == VK_FORMAT_R32G32B32_SFLOAT ||
            /* R32G32B32 is a weird format and the driver currently only
   * supports the barely minimum.
   * TODO: Implement more if we really need to.
   */
   if (info->type == VK_IMAGE_TYPE_3D)
         maxArraySize = 1;
               /* We can't create 3d compressed 128bpp images that can be rendered to on GFX9 */
   if (physical_device->rad_info.gfx_level >= GFX9 && info->type == VK_IMAGE_TYPE_3D &&
      vk_format_get_blocksizebits(format) == 128 && vk_format_is_compressed(format) &&
   (info->flags & VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT) &&
   ((info->flags & VK_IMAGE_CREATE_EXTENDED_USAGE_BIT) || (info->usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT))) {
               /* For some reasons, we can't create 1d block-compressed images that can be stored to with a
   * different format on GFX6.
   */
   if (physical_device->rad_info.gfx_level == GFX6 && info->type == VK_IMAGE_TYPE_1D &&
      vk_format_is_block_compressed(format) && (info->flags & VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT) &&
   ((info->flags & VK_IMAGE_CREATE_EXTENDED_USAGE_BIT) || (info->usage & VK_IMAGE_USAGE_STORAGE_BIT))) {
               /* From the Vulkan 1.3.206 spec:
   *
   * "VK_IMAGE_CREATE_EXTENDED_USAGE_BIT specifies that the image can be created with usage flags
   * that are not supported for the format the image is created with but are supported for at least
   * one format a VkImageView created from the image can have."
   */
   VkImageUsageFlags image_usage = info->usage;
   if (info->flags & VK_IMAGE_CREATE_EXTENDED_USAGE_BIT)
            if (image_usage & VK_IMAGE_USAGE_SAMPLED_BIT) {
      if (!(format_feature_flags & VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_BIT)) {
                     if (image_usage & VK_IMAGE_USAGE_STORAGE_BIT) {
      if (!(format_feature_flags & VK_FORMAT_FEATURE_2_STORAGE_IMAGE_BIT)) {
                     if (image_usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
      if (!(format_feature_flags & VK_FORMAT_FEATURE_2_COLOR_ATTACHMENT_BIT)) {
                     if (image_usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
      if (!(format_feature_flags & VK_FORMAT_FEATURE_2_DEPTH_STENCIL_ATTACHMENT_BIT)) {
                     if (image_usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) {
      if (!(format_feature_flags & VK_FORMAT_FEATURE_2_TRANSFER_SRC_BIT)) {
                     if (image_usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
      if (!(format_feature_flags & VK_FORMAT_FEATURE_2_TRANSFER_DST_BIT)) {
                     if (image_usage & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT) {
      if (!(format_feature_flags &
                           /* Sparse resources with multi-planar formats are unsupported. */
   if (info->flags & VK_IMAGE_CREATE_SPARSE_BINDING_BIT) {
      if (vk_format_get_plane_count(format) > 1)
               if (info->flags & VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT) {
      /* Sparse textures are only supported on GFX8+. */
   if (physical_device->rad_info.gfx_level < GFX8)
            if (vk_format_get_plane_count(format) > 1 || info->type == VK_IMAGE_TYPE_1D ||
      info->tiling != VK_IMAGE_TILING_OPTIMAL || vk_format_is_depth_or_stencil(format))
            if ((info->flags & (VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT | VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT)) &&
      radv_is_format_emulated(physical_device, format)) {
               *pImageFormatProperties = (VkImageFormatProperties){
      .maxExtent = maxExtent,
   .maxMipLevels = maxMipLevels,
   .maxArrayLayers = maxArraySize,
            /* FINISHME: Accurately calculate
   * VkImageFormatProperties::maxResourceSize.
   */
               if (mod_info) {
      result = radv_check_modifier_support(physical_device, info, pImageFormatProperties, format,
         if (result != VK_SUCCESS)
                  unsupported:
      *pImageFormatProperties = (VkImageFormatProperties){
      .maxExtent = {0, 0, 0},
   .maxMipLevels = 0,
   .maxArrayLayers = 0,
   .sampleCounts = 0,
                  }
      static void
   get_external_image_format_properties(struct radv_physical_device *physical_device,
                           {
      VkExternalMemoryFeatureFlagBits flags = 0;
   VkExternalMemoryHandleTypeFlags export_flags = 0;
            if (radv_is_format_emulated(physical_device, pImageFormatInfo->format))
            if (pImageFormatInfo->flags & VK_IMAGE_CREATE_SPARSE_BINDING_BIT)
            switch (handleType) {
   case VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT:
      if (pImageFormatInfo->tiling != VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT)
            case VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT:
      if (pImageFormatInfo->type != VK_IMAGE_TYPE_2D)
         flags = VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT | VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT;
   if (handleType == VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT &&
                  compat_flags = export_flags = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
   if (pImageFormatInfo->tiling == VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT) {
      compat_flags |= VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;
      }
      case VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID:
      if (!physical_device->vk.supported_extensions.ANDROID_external_memory_android_hardware_buffer)
            if (pImageFormatInfo->type != VK_IMAGE_TYPE_2D)
            format_properties->maxMipLevels = MIN2(1, format_properties->maxMipLevels);
   format_properties->maxArrayLayers = MIN2(1, format_properties->maxArrayLayers);
                     /* advertise EXPORTABLE only when radv_create_ahb_memory supports the format */
   if (radv_android_gralloc_supports_format(pImageFormatInfo->format, pImageFormatInfo->usage))
            compat_flags = VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID;
      case VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT:
      flags = VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT;
   compat_flags = VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT;
      default:
                  *external_properties = (VkExternalMemoryProperties){
      .externalMemoryFeatures = flags,
   .exportFromImportedHandleTypes = export_flags,
         }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_GetPhysicalDeviceImageFormatProperties2(VkPhysicalDevice physicalDevice,
               {
      RADV_FROM_HANDLE(radv_physical_device, physical_device, physicalDevice);
   const VkPhysicalDeviceExternalImageFormatInfo *external_info = NULL;
   VkExternalImageFormatProperties *external_props = NULL;
   struct VkAndroidHardwareBufferUsageANDROID *android_usage = NULL;
   VkSamplerYcbcrConversionImageFormatProperties *ycbcr_props = NULL;
   VkTextureLODGatherFormatPropertiesAMD *texture_lod_props = NULL;
   VkResult result;
            result = radv_get_image_format_properties(physical_device, base_info, format, &base_props->imageFormatProperties);
   if (result != VK_SUCCESS)
            /* Extract input structs */
   vk_foreach_struct_const (s, base_info->pNext) {
      switch (s->sType) {
   case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO:
      external_info = (const void *)s;
      default:
                     /* Extract output structs */
   vk_foreach_struct (s, base_props->pNext) {
      switch (s->sType) {
   case VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES:
      external_props = (void *)s;
      case VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_IMAGE_FORMAT_PROPERTIES:
      ycbcr_props = (void *)s;
      case VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_USAGE_ANDROID:
      android_usage = (void *)s;
      case VK_STRUCTURE_TYPE_TEXTURE_LOD_GATHER_FORMAT_PROPERTIES_AMD:
      texture_lod_props = (void *)s;
      default:
                     bool ahb_supported = physical_device->vk.supported_extensions.ANDROID_external_memory_android_hardware_buffer;
   if (android_usage && ahb_supported) {
                  /* From the Vulkan 1.0.97 spec:
   *
   *    If handleType is 0, vkGetPhysicalDeviceImageFormatProperties2 will
   *    behave as if VkPhysicalDeviceExternalImageFormatInfo was not
   *    present and VkExternalImageFormatProperties will be ignored.
   */
   if (external_info && external_info->handleType != 0) {
               if (!external_props) {
      memset(&fallback_external_props, 0, sizeof(fallback_external_props));
               get_external_image_format_properties(physical_device, base_info, external_info->handleType,
               if (!external_props->externalMemoryProperties.externalMemoryFeatures) {
      /* From the Vulkan 1.0.97 spec:
   *
   *    If handleType is not compatible with the [parameters] specified
   *    in VkPhysicalDeviceImageFormatInfo2, then
   *    vkGetPhysicalDeviceImageFormatProperties2 returns
   *    VK_ERROR_FORMAT_NOT_SUPPORTED.
   */
   result = vk_errorf(physical_device, VK_ERROR_FORMAT_NOT_SUPPORTED,
                        if (ycbcr_props) {
                  if (texture_lod_props) {
      if (physical_device->rad_info.gfx_level >= GFX9) {
         } else {
                           fail:
      if (result == VK_ERROR_FORMAT_NOT_SUPPORTED) {
      /* From the Vulkan 1.0.97 spec:
   *
   *    If the combination of parameters to
   *    vkGetPhysicalDeviceImageFormatProperties2 is not supported by
   *    the implementation for use in vkCreateImage, then all members of
   *    imageFormatProperties will be filled with zero.
   */
                  }
      static void
   fill_sparse_image_format_properties(struct radv_physical_device *pdev, VkImageType type, VkFormat format,
         {
      prop->aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            /* On GFX8 we first subdivide by level and then layer, leading to a single
   * miptail. On GFX9+ we first subdivide by layer and then level which results
   * in a miptail per layer. */
   if (pdev->rad_info.gfx_level < GFX9)
            unsigned w, h;
   unsigned d = 1;
   if (type == VK_IMAGE_TYPE_3D) {
      if (pdev->rad_info.gfx_level >= GFX9) {
      unsigned l2_size = 16 - util_logbase2(vk_format_get_blocksize(format));
   w = (1u << ((l2_size + 2) / 3)) * vk_format_get_blockwidth(format);
   h = (1u << ((l2_size + 1) / 3)) * vk_format_get_blockheight(format);
      } else {
      /* GFX7/GFX8 thick tiling modes */
   unsigned bs = vk_format_get_blocksize(format);
   unsigned l2_size = 16 - util_logbase2(bs) - (bs <= 4 ? 2 : 0);
   w = (1u << ((l2_size + 1) / 2)) * vk_format_get_blockwidth(format);
   h = (1u << (l2_size / 2)) * vk_format_get_blockheight(format);
         } else {
      /* This assumes the sparse image tile size is always 64 KiB (1 << 16) */
   unsigned l2_size = 16 - util_logbase2(vk_format_get_blocksize(format));
   w = (1u << ((l2_size + 1) / 2)) * vk_format_get_blockwidth(format);
      }
      }
      VKAPI_ATTR void VKAPI_CALL
   radv_GetPhysicalDeviceSparseImageFormatProperties2(VkPhysicalDevice physicalDevice,
                     {
      RADV_FROM_HANDLE(radv_physical_device, pdev, physicalDevice);
            if (pFormatInfo->samples > VK_SAMPLE_COUNT_1_BIT) {
      *pPropertyCount = 0;
               const VkPhysicalDeviceImageFormatInfo2 fmt_info = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2,
   .format = pFormatInfo->format,
   .type = pFormatInfo->type,
   .tiling = pFormatInfo->tiling,
   .usage = pFormatInfo->usage,
         VkImageFormatProperties fmt_props;
   result = radv_get_image_format_properties(pdev, &fmt_info, pFormatInfo->format, &fmt_props);
   if (result != VK_SUCCESS) {
      *pPropertyCount = 0;
                        vk_outarray_append_typed(VkSparseImageFormatProperties2, &out, prop)
   {
            }
      VKAPI_ATTR void VKAPI_CALL
   radv_GetImageSparseMemoryRequirements2(VkDevice _device, const VkImageSparseMemoryRequirementsInfo2 *pInfo,
               {
      RADV_FROM_HANDLE(radv_device, device, _device);
            if (!(image->vk.create_flags & VK_IMAGE_CREATE_SPARSE_BINDING_BIT)) {
      *pSparseMemoryRequirementCount = 0;
               VK_OUTARRAY_MAKE_TYPED(VkSparseImageMemoryRequirements2, out, pSparseMemoryRequirements,
            vk_outarray_append_typed(VkSparseImageMemoryRequirements2, &out, req)
   {
      fill_sparse_image_format_properties(device->physical_device, image->vk.image_type, image->vk.format,
                  if (req->memoryRequirements.imageMipTailFirstLod < image->vk.mip_levels) {
      if (device->physical_device->rad_info.gfx_level >= GFX9) {
      /* The tail is always a single tile per layer. */
   req->memoryRequirements.imageMipTailSize = 65536;
   req->memoryRequirements.imageMipTailOffset =
            } else {
      req->memoryRequirements.imageMipTailOffset =
      (uint64_t)image->planes[0]
      .surface.u.legacy.level[req->memoryRequirements.imageMipTailFirstLod]
         req->memoryRequirements.imageMipTailSize = image->size - req->memoryRequirements.imageMipTailOffset;
         } else {
      req->memoryRequirements.imageMipTailSize = 0;
   req->memoryRequirements.imageMipTailOffset = 0;
            }
      VKAPI_ATTR void VKAPI_CALL
   radv_GetDeviceImageSparseMemoryRequirements(VkDevice device, const VkDeviceImageMemoryRequirements *pInfo,
               {
      UNUSED VkResult result;
            /* Determining the image size/alignment require to create a surface, which is complicated without
   * creating an image.
   * TODO: Avoid creating an image.
   */
   result =
                  VkImageSparseMemoryRequirementsInfo2 info2 = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_SPARSE_MEMORY_REQUIREMENTS_INFO_2,
                           }
      VKAPI_ATTR void VKAPI_CALL
   radv_GetPhysicalDeviceExternalBufferProperties(VkPhysicalDevice physicalDevice,
               {
      VkExternalMemoryFeatureFlagBits flags = 0;
   VkExternalMemoryHandleTypeFlags export_flags = 0;
   VkExternalMemoryHandleTypeFlags compat_flags = 0;
   switch (pExternalBufferInfo->handleType) {
   case VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT:
   case VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT:
      flags = VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT | VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT;
   compat_flags = export_flags =
            case VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT:
      flags = VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT;
   compat_flags = VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT;
      default:
         }
   pExternalBufferProperties->externalMemoryProperties = (VkExternalMemoryProperties){
      .externalMemoryFeatures = flags,
   .exportFromImportedHandleTypes = export_flags,
         }
      /* DCC channel type categories within which formats can be reinterpreted
   * while keeping the same DCC encoding. The swizzle must also match. */
   enum dcc_channel_type {
      dcc_channel_float,
   dcc_channel_uint,
   dcc_channel_sint,
      };
      /* Return the type of DCC encoding. */
   static void
   radv_get_dcc_channel_type(const struct util_format_description *desc, enum dcc_channel_type *type, unsigned *size)
   {
      int i = util_format_get_first_non_void_channel(desc->format);
   if (i == -1) {
      *type = dcc_channel_incompatible;
               switch (desc->channel[i].size) {
   case 32:
   case 16:
   case 10:
   case 8:
      *size = desc->channel[i].size;
   if (desc->channel[i].type == UTIL_FORMAT_TYPE_FLOAT)
         else if (desc->channel[i].type == UTIL_FORMAT_TYPE_UNSIGNED)
         else
            default:
      *type = dcc_channel_incompatible;
         }
      /* Return if it's allowed to reinterpret one format as another with DCC enabled. */
   bool
   radv_dcc_formats_compatible(enum amd_gfx_level gfx_level, VkFormat format1, VkFormat format2, bool *sign_reinterpret)
   {
      const struct util_format_description *desc1, *desc2;
   enum dcc_channel_type type1, type2;
   unsigned size1, size2;
            if (format1 == format2)
            desc1 = vk_format_description(format1);
            if (desc1->nr_channels != desc2->nr_channels)
            /* Swizzles must be the same. */
   for (i = 0; i < desc1->nr_channels; i++)
      if (desc1->swizzle[i] <= PIPE_SWIZZLE_W && desc2->swizzle[i] <= PIPE_SWIZZLE_W &&
               radv_get_dcc_channel_type(desc1, &type1, &size1);
            if (type1 == dcc_channel_incompatible || type2 == dcc_channel_incompatible ||
      (type1 == dcc_channel_float) != (type2 == dcc_channel_float) || size1 != size2)
         if (type1 != type2) {
      /* FIXME: All formats should be compatible on GFX11 but for some reasons DCC with signedness
   * reinterpretation doesn't work as expected, like R8_UINT<->R8_SINT. Note that disabling
   * fast-clears doesn't help.
   */
   if (gfx_level >= GFX11)
                           }
