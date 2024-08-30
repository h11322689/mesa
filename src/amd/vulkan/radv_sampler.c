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
      #include "radv_private.h"
      #include "vk_sampler.h"
      static unsigned
   radv_tex_wrap(VkSamplerAddressMode address_mode)
   {
      switch (address_mode) {
   case VK_SAMPLER_ADDRESS_MODE_REPEAT:
         case VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT:
         case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE:
         case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER:
         case VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE:
         default:
      unreachable("illegal tex wrap mode");
      }
      }
      static unsigned
   radv_tex_compare(VkCompareOp op)
   {
      switch (op) {
   case VK_COMPARE_OP_NEVER:
         case VK_COMPARE_OP_LESS:
         case VK_COMPARE_OP_EQUAL:
         case VK_COMPARE_OP_LESS_OR_EQUAL:
         case VK_COMPARE_OP_GREATER:
         case VK_COMPARE_OP_NOT_EQUAL:
         case VK_COMPARE_OP_GREATER_OR_EQUAL:
         case VK_COMPARE_OP_ALWAYS:
         default:
      unreachable("illegal compare mode");
      }
      }
      static unsigned
   radv_tex_filter(VkFilter filter, unsigned max_ansio)
   {
      switch (filter) {
   case VK_FILTER_NEAREST:
         case VK_FILTER_LINEAR:
         case VK_FILTER_CUBIC_EXT:
   default:
      fprintf(stderr, "illegal texture filter");
         }
      static unsigned
   radv_tex_mipfilter(VkSamplerMipmapMode mode)
   {
      switch (mode) {
   case VK_SAMPLER_MIPMAP_MODE_NEAREST:
         case VK_SAMPLER_MIPMAP_MODE_LINEAR:
         default:
            }
      static unsigned
   radv_tex_bordercolor(VkBorderColor bcolor)
   {
      switch (bcolor) {
   case VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK:
   case VK_BORDER_COLOR_INT_TRANSPARENT_BLACK:
         case VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK:
   case VK_BORDER_COLOR_INT_OPAQUE_BLACK:
         case VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE:
   case VK_BORDER_COLOR_INT_OPAQUE_WHITE:
         case VK_BORDER_COLOR_FLOAT_CUSTOM_EXT:
   case VK_BORDER_COLOR_INT_CUSTOM_EXT:
         default:
         }
      }
      static unsigned
   radv_tex_aniso_filter(unsigned filter)
   {
         }
      static unsigned
   radv_tex_filter_mode(VkSamplerReductionMode mode)
   {
      switch (mode) {
   case VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE:
         case VK_SAMPLER_REDUCTION_MODE_MIN:
         case VK_SAMPLER_REDUCTION_MODE_MAX:
         default:
         }
      }
      static uint32_t
   radv_get_max_anisotropy(struct radv_device *device, const VkSamplerCreateInfo *pCreateInfo)
   {
      if (device->force_aniso >= 0)
            if (pCreateInfo->anisotropyEnable && pCreateInfo->maxAnisotropy > 1.0f)
               }
      static uint32_t
   radv_register_border_color(struct radv_device *device, VkClearColorValue value)
   {
                        for (slot = 0; slot < RADV_BORDER_COLOR_COUNT; slot++) {
      if (!device->border_color_data.used[slot]) {
                     device->border_color_data.used[slot] = true;
                              }
      static void
   radv_unregister_border_color(struct radv_device *device, uint32_t slot)
   {
                           }
      static void
   radv_init_sampler(struct radv_device *device, struct radv_sampler *sampler, const VkSamplerCreateInfo *pCreateInfo)
   {
      uint32_t max_aniso = radv_get_max_anisotropy(device, pCreateInfo);
   uint32_t max_aniso_ratio = radv_tex_aniso_filter(max_aniso);
   bool compat_mode =
         unsigned filter_mode = radv_tex_filter_mode(sampler->vk.reduction_mode);
   unsigned depth_compare_func = V_008F30_SQ_TEX_DEPTH_COMPARE_NEVER;
   bool trunc_coord = ((pCreateInfo->minFilter == VK_FILTER_NEAREST && pCreateInfo->magFilter == VK_FILTER_NEAREST) ||
               bool uses_border_color = pCreateInfo->addressModeU == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER ||
               VkBorderColor border_color = uses_border_color ? pCreateInfo->borderColor : VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
   uint32_t border_color_ptr;
            if (pCreateInfo->compareEnable)
                     if (vk_border_color_is_custom(border_color)) {
               /* Did we fail to find a slot? */
   if (sampler->border_color_slot == RADV_BORDER_COLOR_COUNT) {
      fprintf(stderr, "WARNING: no free border color slots, defaulting to TRANS_BLACK.\n");
                  /* If we don't have a custom color, set the ptr to 0 */
            sampler->state[0] = (S_008F30_CLAMP_X(radv_tex_wrap(pCreateInfo->addressModeU)) |
                        S_008F30_CLAMP_Y(radv_tex_wrap(pCreateInfo->addressModeV)) |
   S_008F30_CLAMP_Z(radv_tex_wrap(pCreateInfo->addressModeW)) |
   S_008F30_MAX_ANISO_RATIO(max_aniso_ratio) | S_008F30_DEPTH_COMPARE_FUNC(depth_compare_func) |
      sampler->state[1] = (S_008F34_MIN_LOD(radv_float_to_ufixed(CLAMP(pCreateInfo->minLod, 0, 15), 8)) |
               sampler->state[2] = (S_008F38_XY_MAG_FILTER(radv_tex_filter(pCreateInfo->magFilter, max_aniso)) |
                        if (device->physical_device->rad_info.gfx_level >= GFX10) {
      sampler->state[2] |= S_008F38_LOD_BIAS(radv_float_to_sfixed(CLAMP(pCreateInfo->mipLodBias, -32, 31), 8)) |
      } else {
      sampler->state[2] |= S_008F38_LOD_BIAS(radv_float_to_sfixed(CLAMP(pCreateInfo->mipLodBias, -16, 16), 8)) |
                                 if (device->physical_device->rad_info.gfx_level >= GFX11) {
         } else {
            }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_CreateSampler(VkDevice _device, const VkSamplerCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator,
         {
      RADV_FROM_HANDLE(radv_device, device, _device);
            sampler = vk_sampler_create(&device->vk, pCreateInfo, pAllocator, sizeof(*sampler));
   if (!sampler)
                                 }
      VKAPI_ATTR void VKAPI_CALL
   radv_DestroySampler(VkDevice _device, VkSampler _sampler, const VkAllocationCallbacks *pAllocator)
   {
      RADV_FROM_HANDLE(radv_device, device, _device);
            if (!sampler)
            if (sampler->border_color_slot != RADV_BORDER_COLOR_COUNT)
               }
