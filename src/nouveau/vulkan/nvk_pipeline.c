   /*
   * Copyright © 2022 Collabora Ltd. and Red Hat Inc.
   * SPDX-License-Identifier: MIT
   */
   #include "nvk_pipeline.h"
      #include "nvk_device.h"
   #include "nvk_entrypoints.h"
      #include "vk_pipeline_cache.h"
      struct nvk_pipeline *
   nvk_pipeline_zalloc(struct nvk_device *dev,
               {
               assert(size >= sizeof(*pipeline));
   pipeline = vk_object_zalloc(&dev->vk, pAllocator, size,
         if (pipeline == NULL)
                        }
      void
   nvk_pipeline_free(struct nvk_device *dev,
               {
      for (uint32_t s = 0; s < ARRAY_SIZE(pipeline->shaders); s++)
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   nvk_CreateGraphicsPipelines(VkDevice _device,
                                 {
      VK_FROM_HANDLE(nvk_device, dev, _device);
   VK_FROM_HANDLE(vk_pipeline_cache, cache, pipelineCache);
            unsigned i = 0;
   for (; i < createInfoCount; i++) {
      VkResult r = nvk_graphics_pipeline_create(dev, cache, &pCreateInfos[i],
         if (r == VK_SUCCESS)
            result = r;
   pPipelines[i] = VK_NULL_HANDLE;
   if (pCreateInfos[i].flags & VK_PIPELINE_CREATE_EARLY_RETURN_ON_FAILURE_BIT)
               for (; i < createInfoCount; i++)
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   nvk_CreateComputePipelines(VkDevice _device,
                                 {
      VK_FROM_HANDLE(nvk_device, dev, _device);
   VK_FROM_HANDLE(vk_pipeline_cache, cache, pipelineCache);
            unsigned i = 0;
   for (; i < createInfoCount; i++) {
      VkResult r = nvk_compute_pipeline_create(dev, cache, &pCreateInfos[i],
         if (r == VK_SUCCESS)
            result = r;
   pPipelines[i] = VK_NULL_HANDLE;
   if (pCreateInfos[i].flags & VK_PIPELINE_CREATE_EARLY_RETURN_ON_FAILURE_BIT)
               for (; i < createInfoCount; i++)
               }
      VKAPI_ATTR void VKAPI_CALL
   nvk_DestroyPipeline(VkDevice _device, VkPipeline _pipeline,
         {
      VK_FROM_HANDLE(nvk_device, dev, _device);
            if (!pipeline)
               }
