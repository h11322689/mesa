   /*
   * Copyright © 2020 Intel Corporation
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
      #include "vk_ycbcr_conversion.h"
      #include <vulkan/vulkan_android.h>
      #include "vk_common_entrypoints.h"
   #include "vk_device.h"
   #include "vk_format.h"
   #include "vk_util.h"
      VKAPI_ATTR VkResult VKAPI_CALL
   vk_common_CreateSamplerYcbcrConversion(VkDevice _device,
                     {
      VK_FROM_HANDLE(vk_device, device, _device);
                     conversion = vk_object_zalloc(device, pAllocator, sizeof(*conversion),
         if (!conversion)
                     state->format = pCreateInfo->format;
   state->ycbcr_model = pCreateInfo->ycbcrModel;
            /* Search for VkExternalFormatANDROID and resolve the format. */
   const VkExternalFormatANDROID *android_ext_info =
            /* We assume that Android externalFormat is just a VkFormat */
   if (android_ext_info && android_ext_info->externalFormat) {
      assert(pCreateInfo->format == VK_FORMAT_UNDEFINED);
      } else {
      /* The Vulkan 1.1.95 spec says:
   *
   *    "When creating an external format conversion, the value of
   *    components if ignored."
   */
   state->mapping[0] = pCreateInfo->components.r;
   state->mapping[1] = pCreateInfo->components.g;
   state->mapping[2] = pCreateInfo->components.b;
               state->chroma_offsets[0] = pCreateInfo->xChromaOffset;
   state->chroma_offsets[1] = pCreateInfo->yChromaOffset;
            const struct vk_format_ycbcr_info *ycbcr_info =
            bool has_chroma_subsampled = false;
   if (ycbcr_info) {
      for (uint32_t p = 0; p < ycbcr_info->n_planes; p++) {
      if (ycbcr_info->planes[p].has_chroma &&
      (ycbcr_info->planes[p].denominator_scales[0] > 1 ||
   ycbcr_info->planes[p].denominator_scales[1] > 1))
      }
   state->chroma_reconstruction = has_chroma_subsampled &&
      (state->chroma_offsets[0] == VK_CHROMA_LOCATION_COSITED_EVEN ||
                     }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_DestroySamplerYcbcrConversion(VkDevice _device,
               {
      VK_FROM_HANDLE(vk_device, device, _device);
            if (!conversion)
               }
