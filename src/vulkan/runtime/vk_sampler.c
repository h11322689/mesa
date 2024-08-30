   /*
   * Copyright © 2022 Collabora, LTD
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
      #include "vk_sampler.h"
      #include "vk_format.h"
   #include "vk_util.h"
   #include "vk_ycbcr_conversion.h"
      VkClearColorValue
   vk_border_color_value(VkBorderColor color)
   {
      switch (color) {
   case VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK:
         case VK_BORDER_COLOR_INT_TRANSPARENT_BLACK:
         case VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK:
         case VK_BORDER_COLOR_INT_OPAQUE_BLACK:
         case VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE:
         case VK_BORDER_COLOR_INT_OPAQUE_WHITE:
         default:
            }
      bool
   vk_border_color_is_int(VkBorderColor color)
   {
      switch (color) {
   case VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK:
   case VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK:
   case VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE:
   case VK_BORDER_COLOR_FLOAT_CUSTOM_EXT:
         case VK_BORDER_COLOR_INT_TRANSPARENT_BLACK:
   case VK_BORDER_COLOR_INT_OPAQUE_BLACK:
   case VK_BORDER_COLOR_INT_OPAQUE_WHITE:
   case VK_BORDER_COLOR_INT_CUSTOM_EXT:
         default:
            }
      VkClearColorValue
   vk_sampler_border_color_value(const VkSamplerCreateInfo *pCreateInfo,
         {
      if (vk_border_color_is_custom(pCreateInfo->borderColor)) {
      const VkSamplerCustomBorderColorCreateInfoEXT *border_color_info =
      vk_find_struct_const(pCreateInfo->pNext,
      if (format_out)
               } else {
      if (format_out)
                  }
      void *
   vk_sampler_create(struct vk_device *device,
                     {
               sampler = vk_object_zalloc(device, alloc, size, VK_OBJECT_TYPE_SAMPLER);
   if (!sampler)
                     sampler->format = VK_FORMAT_UNDEFINED;
   sampler->border_color = pCreateInfo->borderColor;
            if (!vk_border_color_is_custom(pCreateInfo->borderColor)) {
      sampler->border_color_value =
               vk_foreach_struct_const(ext, pCreateInfo->pNext) {
      switch (ext->sType) {
   case VK_STRUCTURE_TYPE_SAMPLER_CUSTOM_BORDER_COLOR_CREATE_INFO_EXT: {
      const VkSamplerCustomBorderColorCreateInfoEXT *cbc_info = (void *)ext;
                  sampler->border_color_value = cbc_info->customBorderColor;
   if (cbc_info->format != VK_FORMAT_UNDEFINED)
                     case VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO: {
      const VkSamplerReductionModeCreateInfo *rm_info = (void *)ext;
   sampler->reduction_mode = rm_info->reductionMode;
               case VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO: {
                     /* From the Vulkan 1.2.259 spec:
   *
   *    "A VkSamplerYcbcrConversionInfo must be provided for samplers
   *    to be used with image views that access
   *    VK_IMAGE_ASPECT_COLOR_BIT if the format is one of the formats
   *    that require a sampler YCbCr conversion, or if the image view
   *    has an external format."
   *
   * This means that on Android we can end up with one of these even if
   * YCbCr isn't being used at all. Leave sampler->ycbcr_conversion NULL
   * if it isn't a YCbCr format.
   */
                  sampler->ycbcr_conversion = conversion;
   sampler->format = conversion->state.format;
      }
   default:
                        }
      void
   vk_sampler_destroy(struct vk_device *device,
               {
         }
