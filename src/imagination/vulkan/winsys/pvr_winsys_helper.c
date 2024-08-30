   /*
   * Copyright © 2022 Imagination Technologies Ltd.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a copy
   * of this software and associated documentation files (the "Software"), to deal
   * in the Software without restriction, including without limitation the rights
   * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   * copies of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      #include <assert.h>
   #include <stdbool.h>
   #include <stdint.h>
   #include <vulkan/vulkan.h>
   #include <xf86drm.h>
      #include "pvr_private.h"
   #include "pvr_types.h"
   #include "pvr_winsys.h"
   #include "pvr_winsys_helper.h"
   #include "util/u_atomic.h"
   #include "vk_log.h"
      VkResult pvr_winsys_helper_display_buffer_create(struct pvr_winsys *const ws,
               {
      struct drm_mode_create_dumb args = {
      .width = size,
   .height = 1,
      };
            result = pvr_ioctl(ws->display_fd,
                     if (result != VK_SUCCESS)
                        }
      VkResult pvr_winsys_helper_display_buffer_destroy(struct pvr_winsys *ws,
         {
      struct drm_mode_destroy_dumb args = {
                  return pvr_ioctl(ws->display_fd,
                  }
      bool pvr_winsys_helper_winsys_heap_finish(struct pvr_winsys_heap *const heap)
   {
      if (p_atomic_read(&heap->ref_count) != 0)
            pthread_mutex_destroy(&heap->lock);
               }
      VkResult pvr_winsys_helper_heap_alloc(struct pvr_winsys_heap *const heap,
                     {
      struct pvr_winsys_vma vma = {
                           /* pvr_srv_winsys_buffer_create() page aligns the size. We must do the same
   * here to ensure enough heap space is allocated to be able to map the
   * buffer to the GPU.
   * We have to do this for the powervr kernel mode driver as well, as it
   * returns a page aligned size when allocating buffers.
   */
            size = ALIGN_POT(size, alignment);
            pthread_mutex_lock(&heap->lock);
   vma.dev_addr =
                  if (!vma.dev_addr.addr)
                                 }
      void pvr_winsys_helper_heap_free(struct pvr_winsys_vma *const vma)
   {
               /* A vma with an existing device mapping should not be freed. */
            pthread_mutex_lock(&heap->lock);
   util_vma_heap_free(&heap->vma_heap, vma->dev_addr.addr, vma->size);
               }
      /* Note: the function assumes the heap allocation in the carveout memory area
   * can be freed with the regular heap allocation free function. The free
   * function gets called on mapping failure.
   */
   static VkResult
   pvr_buffer_create_and_map(struct pvr_winsys *const ws,
                           heap_alloc_reserved_func heap_alloc_reserved,
   struct pvr_winsys_heap *heap,
   {
      struct pvr_winsys_vma *vma;
   struct pvr_winsys_bo *bo;
            /* Address should not be NULL, this function is used to allocate and map
   * reserved addresses and is only supposed to be used internally.
   */
            result = ws->ops->buffer_create(ws,
                                 if (result != VK_SUCCESS)
            result = heap_alloc_reserved(heap, dev_addr, size, alignment, &vma);
   if (result != VK_SUCCESS)
            result = ws->ops->vma_map(vma, bo, 0, size, NULL);
   if (result != VK_SUCCESS)
            /* Note this won't destroy bo as its being used by VMA, once vma is
   * unmapped, bo will be destroyed automatically.
   */
                           err_pvr_winsys_heap_free:
            err_pvr_winsys_buffer_destroy:
            err_out:
         }
      static void inline pvr_buffer_destroy_and_unmap(struct pvr_winsys_vma *vma)
   {
               /* Buffer object associated with the vma will be automatically destroyed
   * once vma is unmapped.
   */
   ws->ops->vma_unmap(vma);
      }
      VkResult pvr_winsys_helper_allocate_static_memory(
      struct pvr_winsys *const ws,
   heap_alloc_reserved_func heap_alloc_reserved,
   struct pvr_winsys_heap *const general_heap,
   struct pvr_winsys_heap *const pds_heap,
   struct pvr_winsys_heap *const usc_heap,
   struct pvr_winsys_vma **const general_vma_out,
   struct pvr_winsys_vma **const pds_vma_out,
      {
      struct pvr_winsys_vma *general_vma;
   struct pvr_winsys_vma *pds_vma;
   struct pvr_winsys_vma *usc_vma;
            result = pvr_buffer_create_and_map(ws,
                                       if (result != VK_SUCCESS)
            result = pvr_buffer_create_and_map(ws,
                                       if (result != VK_SUCCESS)
            result = pvr_buffer_create_and_map(ws,
                                       if (result != VK_SUCCESS)
            *general_vma_out = general_vma;
   *pds_vma_out = pds_vma;
                  err_pvr_buffer_destroy_and_unmap_pds:
            err_pvr_buffer_destroy_and_unmap_general:
            err_out:
         }
      void pvr_winsys_helper_free_static_memory(
      struct pvr_winsys_vma *const general_vma,
   struct pvr_winsys_vma *const pds_vma,
      {
      pvr_buffer_destroy_and_unmap(usc_vma);
   pvr_buffer_destroy_and_unmap(pds_vma);
      }
      static void pvr_setup_static_vdm_sync(uint8_t *const pds_ptr,
                     {
      /* TODO: this needs to be auto-generated */
   const uint8_t state_update[] = { 0x44, 0xA0, 0x80, 0x05,
                     memcpy(usc_ptr + usc_sync_offset_in_bytes,
                  pvr_pds_setup_doutu(&ppp_state_update_program.usc_task_control,
                              pvr_pds_kick_usc(&ppp_state_update_program,
                  (uint32_t *)&pds_ptr[pds_sync_offset_in_bytes],
   }
      static void
   pvr_setup_static_pixel_event_program(uint8_t *const pds_ptr,
         {
               pvr_pds_generate_pixel_event(&pixel_event_program,
                  }
      VkResult
   pvr_winsys_helper_fill_static_memory(struct pvr_winsys *const ws,
                     {
               result = ws->ops->buffer_map(general_vma->bo);
   if (result != VK_SUCCESS)
            result = ws->ops->buffer_map(pds_vma->bo);
   if (result != VK_SUCCESS)
            result = ws->ops->buffer_map(usc_vma->bo);
   if (result != VK_SUCCESS)
            pvr_setup_static_vdm_sync(pds_vma->bo->map,
                        pvr_setup_static_pixel_event_program(pds_vma->bo->map,
            ws->ops->buffer_unmap(usc_vma->bo);
   ws->ops->buffer_unmap(pds_vma->bo);
                  err_pvr_srv_winsys_buffer_unmap_pds:
            err_pvr_srv_winsys_buffer_unmap_general:
            err_out:
         }
