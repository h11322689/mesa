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
      #include <stdlib.h>
   #include <string.h>
   #include "vk_descriptors.h"
   #include "vk_common_entrypoints.h"
   #include "util/macros.h"
      static int
   binding_compare(const void* av, const void *bv)
   {
      const VkDescriptorSetLayoutBinding *a = (const VkDescriptorSetLayoutBinding*)av;
               }
      VkResult
   vk_create_sorted_bindings(const VkDescriptorSetLayoutBinding *bindings, unsigned count,
         {
      if (!count) {
      *sorted_bindings = NULL;
               *sorted_bindings = malloc(count * sizeof(VkDescriptorSetLayoutBinding));
   if (!*sorted_bindings)
            memcpy(*sorted_bindings, bindings, count * sizeof(VkDescriptorSetLayoutBinding));
               }
      /*
   * For drivers that don't have mutable state in buffers, images, image views, or
   * samplers, there's no need to save/restore anything to get the same
   * descriptor back as long as the user uses the same GPU virtual address. In
   * this case, the following EXT_descriptor_buffer functions are trivial.
   */
   VKAPI_ATTR VkResult VKAPI_CALL
   vk_common_GetBufferOpaqueCaptureDescriptorDataEXT(VkDevice device,
               {
         }
      VKAPI_ATTR VkResult VKAPI_CALL
   vk_common_GetImageOpaqueCaptureDescriptorDataEXT(VkDevice device,
               {
         }
      VKAPI_ATTR VkResult VKAPI_CALL
   vk_common_GetImageViewOpaqueCaptureDescriptorDataEXT(VkDevice device,
               {
         }
      VKAPI_ATTR VkResult VKAPI_CALL
   vk_common_GetSamplerOpaqueCaptureDescriptorDataEXT(VkDevice _device,
               {
         }
      VKAPI_ATTR VkResult VKAPI_CALL
   vk_common_GetAccelerationStructureOpaqueCaptureDescriptorDataEXT(VkDevice device,
               {
         }
