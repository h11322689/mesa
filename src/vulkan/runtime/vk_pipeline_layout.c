   /*
   * Copyright © 2022 Collabora Ltd
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
      #include "vk_pipeline_layout.h"
      #include "vk_alloc.h"
   #include "vk_common_entrypoints.h"
   #include "vk_descriptor_set_layout.h"
   #include "vk_device.h"
   #include "vk_log.h"
      #include "util/mesa-sha1.h"
      static void
   vk_pipeline_layout_init(struct vk_device *device,
               {
      assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO);
                     layout->ref_cnt = 1;
   layout->create_flags = pCreateInfo->flags;
   layout->set_count = pCreateInfo->setLayoutCount;
            for (uint32_t s = 0; s < pCreateInfo->setLayoutCount; s++) {
      VK_FROM_HANDLE(vk_descriptor_set_layout, set_layout,
            if (set_layout != NULL)
         else
         }
      void *
   vk_pipeline_layout_zalloc(struct vk_device *device, size_t size,
         {
      /* Because we're reference counting and lifetimes may not be what the
   * client expects, these have to be allocated off the device and not as
   * their own object.
   */
   struct vk_pipeline_layout *layout =
         if (layout == NULL)
            vk_pipeline_layout_init(device, layout, pCreateInfo);
      }
      void *
   vk_pipeline_layout_multizalloc(struct vk_device *device,
               {
      struct vk_pipeline_layout *layout =
      vk_multialloc_zalloc(ma, &device->alloc,
      if (layout == NULL)
            vk_pipeline_layout_init(device, layout, pCreateInfo);
      }
         VKAPI_ATTR VkResult VKAPI_CALL
   vk_common_CreatePipelineLayout(VkDevice _device,
                     {
               struct vk_pipeline_layout *layout =
      vk_pipeline_layout_zalloc(device, sizeof(struct vk_pipeline_layout),
      if (layout == NULL)
                        }
      void
   vk_pipeline_layout_destroy(struct vk_device *device,
         {
               for (uint32_t s = 0; s < layout->set_count; s++) {
      if (layout->set_layouts[s] != NULL)
                  }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_DestroyPipelineLayout(VkDevice _device,
               {
      VK_FROM_HANDLE(vk_device, device, _device);
            if (layout == NULL)
               }
