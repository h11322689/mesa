   /*
   * Copyright © 2019 Red Hat.
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
      #include "lvp_private.h"
      VKAPI_ATTR VkResult VKAPI_CALL lvp_CreatePipelineCache(
      VkDevice                                    _device,
   const VkPipelineCacheCreateInfo*            pCreateInfo,
   const VkAllocationCallbacks*                pAllocator,
      {
      LVP_FROM_HANDLE(lvp_device, device, _device);
                     cache = vk_alloc2(&device->vk.alloc, pAllocator,
               if (cache == NULL)
            vk_object_base_init(&device->vk, &cache->base,
         if (pAllocator)
   cache->alloc = *pAllocator;
   else
            cache->device = device;
               }
      VKAPI_ATTR void VKAPI_CALL lvp_DestroyPipelineCache(
      VkDevice                                    _device,
   VkPipelineCache                             _cache,
      {
      LVP_FROM_HANDLE(lvp_device, device, _device);
            if (!_cache)
      //   lvp_pipeline_cache_finish(cache);
      vk_object_base_finish(&cache->base);
      }
      VKAPI_ATTR VkResult VKAPI_CALL lvp_GetPipelineCacheData(
         VkDevice                                    _device,
   VkPipelineCache                             _cache,
   size_t*                                     pDataSize,
   {
      VkResult result = VK_SUCCESS;
   if (pData) {
      if (*pDataSize < 32) {
      *pDataSize = 0;
      } else {
      uint32_t *hdr = (uint32_t *)pData;
   hdr[0] = 32;
   hdr[1] = 1;
   hdr[2] = VK_VENDOR_ID_MESA;
   hdr[3] = 0;
         } else
            }
      VKAPI_ATTR VkResult VKAPI_CALL lvp_MergePipelineCaches(
         VkDevice                                    _device,
   VkPipelineCache                             destCache,
   uint32_t                                    srcCacheCount,
   {
         }
