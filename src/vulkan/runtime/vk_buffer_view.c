   /*
   * Copyright © 2022 Collabora, Ltd.
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
      #include "vk_buffer_view.h"
      #include "vk_alloc.h"
   #include "vk_buffer.h"
   #include "vk_device.h"
   #include "vk_format.h"
      void
   vk_buffer_view_init(struct vk_device *device,
               {
                        assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO);
   assert(pCreateInfo->flags == 0);
            buffer_view->buffer = buffer;
   buffer_view->format = pCreateInfo->format;
   buffer_view->offset = pCreateInfo->offset;
   buffer_view->range = vk_buffer_range(buffer, pCreateInfo->offset,
         buffer_view->elements = buffer_view->range /
      }
      void *
   vk_buffer_view_create(struct vk_device *device,
                     {
               buffer_view = vk_zalloc2(&device->alloc, alloc, size, 8,
         if (!buffer_view)
                        }
      void
   vk_buffer_view_finish(struct vk_buffer_view *buffer_view)
   {
         }
      void
   vk_buffer_view_destroy(struct vk_device *device,
               {
         }
