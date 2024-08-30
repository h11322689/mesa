   /*
   * Copyright © 2021 Intel Corporation
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
      #include "vk_framebuffer.h"
      #include "vk_common_entrypoints.h"
   #include "vk_device.h"
      VKAPI_ATTR VkResult VKAPI_CALL
   vk_common_CreateFramebuffer(VkDevice _device,
                     {
      VK_FROM_HANDLE(vk_device, device, _device);
                              /* VK_KHR_imageless_framebuffer extension says:
   *
   *    If flags includes VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT,
   *    parameter pAttachments is ignored.
   */
   if (!(pCreateInfo->flags & VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT))
            framebuffer = vk_object_alloc(device, pAllocator, size,
         if (framebuffer == NULL)
            framebuffer->flags = pCreateInfo->flags;
   framebuffer->width = pCreateInfo->width;
   framebuffer->height = pCreateInfo->height;
            if (!(pCreateInfo->flags & VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT)) {
      for (uint32_t i = 0; i < pCreateInfo->attachmentCount; i++)
                                 }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_DestroyFramebuffer(VkDevice _device,
               {
      VK_FROM_HANDLE(vk_device, device, _device);
            if (!framebuffer)
               }
