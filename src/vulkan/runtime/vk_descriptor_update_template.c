   /*
   * Copyright © 2017 Intel Corporation
   * Copyright © 2022 Collabora, Ltd
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
      #include "vk_descriptor_update_template.h"
      #include "vk_common_entrypoints.h"
   #include "vk_device.h"
   #include "vk_log.h"
      VKAPI_ATTR VkResult VKAPI_CALL
   vk_common_CreateDescriptorUpdateTemplate(VkDevice _device,
      const VkDescriptorUpdateTemplateCreateInfo *pCreateInfo,
   const VkAllocationCallbacks *pAllocator,
      {
      VK_FROM_HANDLE(vk_device, device, _device);
            uint32_t entry_count = 0;
   for (uint32_t i = 0; i < pCreateInfo->descriptorUpdateEntryCount; i++) {
      if (pCreateInfo->pDescriptorUpdateEntries[i].descriptorCount > 0)
               size_t size = sizeof(*template) + entry_count * sizeof(template->entries[0]);
   template = vk_object_alloc(device, pAllocator, size,
         if (template == NULL)
            template->type = pCreateInfo->templateType;
            if (template->type == VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_DESCRIPTOR_SET)
            uint32_t entry_idx = 0;
   template->entry_count = entry_count;
   for (uint32_t i = 0; i < pCreateInfo->descriptorUpdateEntryCount; i++) {
      const VkDescriptorUpdateTemplateEntry *pEntry =
            if (pEntry->descriptorCount == 0)
            template->entries[entry_idx++] = (struct vk_descriptor_template_entry) {
      .type = pEntry->descriptorType,
   .binding = pEntry->dstBinding,
   .array_element = pEntry->dstArrayElement,
   .array_count = pEntry->descriptorCount,
   .offset = pEntry->offset,
         }
            *pDescriptorUpdateTemplate =
               }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_DestroyDescriptorUpdateTemplate(VkDevice _device,
      VkDescriptorUpdateTemplate descriptorUpdateTemplate,
      {
      VK_FROM_HANDLE(vk_device, device, _device);
   VK_FROM_HANDLE(vk_descriptor_update_template, template,
            if (!template)
               }
