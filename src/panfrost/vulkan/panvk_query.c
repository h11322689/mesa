   /*
   * Copyright © 2021 Collabora Ltd.
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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      #include "panvk_private.h"
      VkResult
   panvk_CreateQueryPool(VkDevice _device,
                     {
      panvk_stub();
      }
      void
   panvk_DestroyQueryPool(VkDevice _device, VkQueryPool _pool,
         {
         }
      VkResult
   panvk_GetQueryPoolResults(VkDevice _device, VkQueryPool queryPool,
                     {
      panvk_stub();
      }
      void
   panvk_CmdCopyQueryPoolResults(VkCommandBuffer commandBuffer,
                           {
         }
      void
   panvk_CmdResetQueryPool(VkCommandBuffer commandBuffer, VkQueryPool queryPool,
         {
         }
      void
   panvk_CmdBeginQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool,
         {
         }
      void
   panvk_CmdEndQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool,
         {
         }
      void
   panvk_CmdWriteTimestamp2(VkCommandBuffer commandBuffer,
               {
         }
