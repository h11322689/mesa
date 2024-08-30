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
      #include "vk_object.h"
      #include "vk_alloc.h"
   #include "vk_common_entrypoints.h"
   #include "vk_instance.h"
   #include "vk_device.h"
   #include "util/hash_table.h"
   #include "util/ralloc.h"
   #include "vk_enum_to_str.h"
      void
   vk_object_base_init(struct vk_device *device,
               {
      base->_loader_data.loaderMagic = ICD_LOADER_MAGIC;
   base->type = obj_type;
   base->device = device;
   base->instance = NULL;
   base->client_visible = false;
   base->object_name = NULL;
      }
      void vk_object_base_instance_init(struct vk_instance *instance,
               {
      base->_loader_data.loaderMagic = ICD_LOADER_MAGIC;
   base->type = obj_type;
   base->device = NULL;
   base->instance = instance;
   base->client_visible = false;
   base->object_name = NULL;
      }
      void
   vk_object_base_finish(struct vk_object_base *base)
   {
               if (base->object_name == NULL)
            assert(base->device != NULL || base->instance != NULL);
   if (base->device)
         else
      }
      void
   vk_object_base_recycle(struct vk_object_base *base)
   {
      struct vk_device *device = base->device;
   VkObjectType obj_type = base->type;
   vk_object_base_finish(base);
      }
      void *
   vk_object_alloc(struct vk_device *device,
                     {
      void *ptr = vk_alloc2(&device->alloc, alloc, size, 8,
         if (ptr == NULL)
                        }
      void *
   vk_object_zalloc(struct vk_device *device,
                     {
      void *ptr = vk_zalloc2(&device->alloc, alloc, size, 8,
         if (ptr == NULL)
                        }
      void *
   vk_object_multialloc(struct vk_device *device,
                     {
      void *ptr = vk_multialloc_alloc2(ma, &device->alloc, alloc,
         if (ptr == NULL)
                        }
      void *
   vk_object_multizalloc(struct vk_device *device,
                     {
      void *ptr = vk_multialloc_zalloc2(ma, &device->alloc, alloc,
         if (ptr == NULL)
                        }
      void
   vk_object_free(struct vk_device *device,
               {
      vk_object_base_finish((struct vk_object_base *)data);
      }
      VkResult
   vk_private_data_slot_create(struct vk_device *device,
                     {
      struct vk_private_data_slot *slot =
      vk_alloc2(&device->alloc, pAllocator, sizeof(*slot), 8,
      if (slot == NULL)
            vk_object_base_init(device, &slot->base,
                              }
      void
   vk_private_data_slot_destroy(struct vk_device *device,
               {
      VK_FROM_HANDLE(vk_private_data_slot, slot, privateDataSlot);
   if (slot == NULL)
            vk_object_base_finish(&slot->base);
      }
      static VkResult
   get_swapchain_private_data_locked(struct vk_device *device,
                     {
      if (unlikely(device->swapchain_private == NULL)) {
      /* Even though VkSwapchain/Surface are non-dispatchable objects, we know
   * a priori that these are actually pointers so we can use
   * the pointer hash table for them.
   */
   device->swapchain_private = _mesa_pointer_hash_table_create(NULL);
   if (device->swapchain_private == NULL)
               struct hash_entry *entry =
      _mesa_hash_table_search(device->swapchain_private,
      if (unlikely(entry == NULL)) {
      struct util_sparse_array *swapchain_private =
                  entry = _mesa_hash_table_insert(device->swapchain_private,
               if (entry == NULL)
               struct util_sparse_array *swapchain_private = entry->data;
               }
      static VkResult
   vk_object_base_private_data(struct vk_device *device,
                           {
               /* There is an annoying spec corner here on Android.  Because WSI is
   * implemented in the Vulkan loader which doesn't know about the
   * VK_EXT_private_data extension, we have to handle VkSwapchainKHR in the
   * driver as a special case.  On future versions of Android where the
   * loader does understand VK_EXT_private_data, we'll never see a
   * vkGet/SetPrivateDataEXT call on a swapchain because the loader will
   * handle it.
      #ifdef ANDROID
      if (objectType == VK_OBJECT_TYPE_SWAPCHAIN_KHR ||
      #else
         #endif
         mtx_lock(&device->swapchain_private_mtx);
   VkResult result = get_swapchain_private_data_locked(device, objectHandle,
         mtx_unlock(&device->swapchain_private_mtx);
               struct vk_object_base *obj =
                     }
      VkResult
   vk_object_base_set_private_data(struct vk_device *device,
                           {
      uint64_t *private_data;
   VkResult result = vk_object_base_private_data(device,
                     if (unlikely(result != VK_SUCCESS))
            *private_data = data;
      }
      void
   vk_object_base_get_private_data(struct vk_device *device,
                           {
      uint64_t *private_data;
   VkResult result = vk_object_base_private_data(device,
                     if (likely(result == VK_SUCCESS)) {
         } else {
            }
      VKAPI_ATTR VkResult VKAPI_CALL
   vk_common_CreatePrivateDataSlotEXT(VkDevice _device,
                     {
      VK_FROM_HANDLE(vk_device, device, _device);
   return vk_private_data_slot_create(device, pCreateInfo, pAllocator,
      }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_DestroyPrivateDataSlotEXT(VkDevice _device,
               {
      VK_FROM_HANDLE(vk_device, device, _device);
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   vk_common_SetPrivateDataEXT(VkDevice _device,
                           {
      VK_FROM_HANDLE(vk_device, device, _device);
   return vk_object_base_set_private_data(device,
            }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_GetPrivateDataEXT(VkDevice _device,
                           {
      VK_FROM_HANDLE(vk_device, device, _device);
   vk_object_base_get_private_data(device,
            }
      const char *
   vk_object_base_name(struct vk_object_base *obj)
   {
      if (obj->object_name)
            obj->object_name = vk_asprintf(&obj->device->alloc,
                                 }
