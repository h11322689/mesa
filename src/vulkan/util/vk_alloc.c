   /*
   * Copyright 2021 Google LLC
   * SPDX-License-Identifier: MIT
   */
      #include "vk_alloc.h"
      #include <stdlib.h>
      #ifndef _MSC_VER
   #include <stddef.h>
   #define MAX_ALIGN alignof(max_align_t)
   #else
   /* long double might be 128-bit, but our callers do not need that anyway(?) */
   #include <stdint.h>
   #define MAX_ALIGN alignof(uint64_t)
   #endif
      static VKAPI_ATTR void * VKAPI_CALL
   vk_default_alloc(void *pUserData,
                     {
      assert(MAX_ALIGN % alignment == 0);
      }
      static VKAPI_ATTR void * VKAPI_CALL
   vk_default_realloc(void *pUserData,
                     void *pOriginal,
   {
      assert(MAX_ALIGN % alignment == 0);
      }
      static VKAPI_ATTR void VKAPI_CALL
   vk_default_free(void *pUserData, void *pMemory)
   {
         }
      const VkAllocationCallbacks *
   vk_default_allocator(void)
   {
      static const VkAllocationCallbacks allocator = {
      .pfnAllocation = vk_default_alloc,
   .pfnReallocation = vk_default_realloc,
      };
      }
