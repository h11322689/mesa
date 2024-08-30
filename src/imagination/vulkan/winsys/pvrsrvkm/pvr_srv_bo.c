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
   #include <fcntl.h>
   #include <stdbool.h>
   #include <stddef.h>
   #include <stdint.h>
   #include <sys/mman.h>
   #include <xf86drm.h>
      #include "pvr_private.h"
   #include "pvr_srv.h"
   #include "pvr_srv_bo.h"
   #include "pvr_srv_bridge.h"
   #include "pvr_types.h"
   #include "pvr_winsys_helper.h"
   #include "util/u_atomic.h"
   #include "util/bitscan.h"
   #include "util/macros.h"
   #include "util/u_math.h"
   #include "vk_log.h"
      /* Note: This function does not have an associated pvr_srv_free_display_pmr
   * function, use pvr_srv_free_pmr instead.
   */
   static VkResult pvr_srv_alloc_display_pmr(struct pvr_srv_winsys *srv_ws,
                           {
      uint64_t aligment_out;
   uint64_t size_out;
   VkResult result;
   uint32_t handle;
   int ret;
            result =
         if (result != VK_SUCCESS)
            ret = drmPrimeHandleToFD(srv_ws->base.display_fd, handle, O_CLOEXEC, &fd);
   if (ret) {
      result = vk_error(NULL, VK_ERROR_OUT_OF_HOST_MEMORY);
               result = pvr_srv_physmem_import_dmabuf(srv_ws->base.render_fd,
                                    assert(size_out >= size);
            /* close fd, not needed anymore */
            if (result != VK_SUCCESS)
                           err_display_buffer_destroy:
                  }
      static void buffer_acquire(struct pvr_srv_winsys_bo *srv_bo)
   {
         }
      static void buffer_release(struct pvr_srv_winsys_bo *srv_bo)
   {
               /* If all references were dropped the pmr can be freed and unlocked */
   if (p_atomic_dec_return(&srv_bo->ref_count) != 0)
            ws = srv_bo->base.ws;
            if (srv_bo->is_display_buffer)
               }
      static uint64_t pvr_srv_get_alloc_flags(uint32_t ws_flags)
   {
      /* TODO: For now we assume that buffers should always be accessible to the
   * kernel and that the PVR_WINSYS_BO_FLAG_CPU_ACCESS flag only applies to
   * userspace mappings. Check to see if there's any situations where we
   * wouldn't want this to be the case.
   */
   uint64_t srv_flags =
      PVR_SRV_MEMALLOCFLAG_GPU_READABLE | PVR_SRV_MEMALLOCFLAG_GPU_WRITEABLE |
   PVR_SRV_MEMALLOCFLAG_KERNEL_CPU_MAPPABLE |
         if (ws_flags & PVR_WINSYS_BO_FLAG_CPU_ACCESS) {
      srv_flags |= PVR_SRV_MEMALLOCFLAG_CPU_READABLE |
               if (ws_flags & PVR_WINSYS_BO_FLAG_GPU_UNCACHED)
         else
            if (ws_flags & PVR_WINSYS_BO_FLAG_PM_FW_PROTECT)
               }
      VkResult pvr_srv_winsys_buffer_create(struct pvr_winsys *ws,
                                 {
      const uint64_t srv_flags = pvr_srv_get_alloc_flags(ws_flags);
   struct pvr_srv_winsys *srv_ws = to_pvr_srv_winsys(ws);
   struct pvr_srv_winsys_bo *srv_bo;
                     /* Kernel will page align the size, we do the same here so we have access to
   * all the allocated memory.
   */
   alignment = MAX2(alignment, ws->page_size);
            srv_bo = vk_zalloc(ws->alloc,
                     if (!srv_bo)
            srv_bo->is_display_buffer = (type == PVR_WINSYS_BO_TYPE_DISPLAY);
   if (srv_bo->is_display_buffer) {
      result = pvr_srv_alloc_display_pmr(srv_ws,
                                       } else {
      result =
      pvr_srv_alloc_pmr(ws->render_fd,
                     size,
   size,
   1,
   1,
            if (result != VK_SUCCESS)
            srv_bo->base.size = size;
   srv_bo->base.ws = ws;
                                    err_vk_free_srv_bo:
                  }
      VkResult
   pvr_srv_winsys_buffer_create_from_fd(struct pvr_winsys *ws,
               {
      /* FIXME: PVR_SRV_MEMALLOCFLAG_CPU_UNCACHED_WC should be changed to
   * PVR_SRV_MEMALLOCFLAG_CPU_CACHE_INCOHERENT, as dma-buf is always mapped
   * as cacheable by the exporter. Flags are not passed to the exporter and it
   * doesn't really change the behavior, but these can be used for internal
   * checking so it should reflect the correct cachability of the buffer.
   * Ref: pvr_GetMemoryFdPropertiesKHR
   * 	    https://www.kernel.org/doc/html/latest/driver-api/dma-buf.html#c.dma_buf_ops
   */
   static const uint64_t srv_flags =
      PVR_SRV_MEMALLOCFLAG_CPU_READABLE | PVR_SRV_MEMALLOCFLAG_CPU_WRITEABLE |
   PVR_SRV_MEMALLOCFLAG_CPU_UNCACHED_WC | PVR_SRV_MEMALLOCFLAG_GPU_READABLE |
   PVR_SRV_MEMALLOCFLAG_GPU_WRITEABLE |
      struct pvr_srv_winsys_bo *srv_bo;
   uint64_t aligment_out;
   uint64_t size_out;
            srv_bo = vk_zalloc(ws->alloc,
                     if (!srv_bo)
            result = pvr_srv_physmem_import_dmabuf(ws->render_fd,
                                 if (result != VK_SUCCESS)
                     srv_bo->base.ws = ws;
   srv_bo->base.size = size_out;
   srv_bo->base.is_imported = true;
                                    err_vk_free_srv_bo:
                  }
      void pvr_srv_winsys_buffer_destroy(struct pvr_winsys_bo *bo)
   {
                  }
      VkResult pvr_srv_winsys_buffer_get_fd(struct pvr_winsys_bo *bo,
         {
      struct pvr_srv_winsys_bo *srv_bo = to_pvr_srv_winsys_bo(bo);
   struct pvr_winsys *ws = bo->ws;
            if (!srv_bo->is_display_buffer)
            /* For display buffers, export using saved buffer handle */
   ret = drmPrimeHandleToFD(ws->display_fd, srv_bo->handle, O_CLOEXEC, fd_out);
   if (ret)
               }
      VkResult pvr_srv_winsys_buffer_map(struct pvr_winsys_bo *bo)
   {
      struct pvr_srv_winsys_bo *srv_bo = to_pvr_srv_winsys_bo(bo);
   struct pvr_winsys *ws = bo->ws;
   const int prot =
      (srv_bo->flags & PVR_SRV_MEMALLOCFLAG_CPU_WRITEABLE ? PROT_WRITE : 0) |
               /* assert if memory is already mapped */
            /* Map the full PMR to CPU space */
   result = pvr_mmap(bo->size,
                     prot,
   MAP_SHARED,
   if (result != VK_SUCCESS) {
      bo->map = NULL;
                                    }
      void pvr_srv_winsys_buffer_unmap(struct pvr_winsys_bo *bo)
   {
               /* output error if trying to unmap memory that is not previously mapped */
                     /* Unmap the whole PMR from CPU space */
                        }
      /* This function must be used to allocate inside reserved region and must be
   * used internally only. This also means whoever is using it, must know what
   * they are doing.
   */
   VkResult pvr_srv_heap_alloc_reserved(struct pvr_winsys_heap *heap,
                           {
      struct pvr_srv_winsys_heap *srv_heap = to_pvr_srv_winsys_heap(heap);
   struct pvr_winsys *ws = heap->ws;
   struct pvr_srv_winsys_vma *srv_vma;
                     /* pvr_srv_winsys_buffer_create() page aligns the size. We must do the same
   * here to ensure enough heap space is allocated to be able to map the
   * buffer to the GPU.
   */
   alignment = MAX2(alignment, heap->ws->page_size);
            srv_vma = vk_alloc(ws->alloc,
                     if (!srv_vma) {
      result = vk_error(NULL, VK_ERROR_OUT_OF_HOST_MEMORY);
               /* Just check address is correct and aligned, locking is not required as
   * user is responsible to provide a distinct address.
   */
   if (reserved_dev_addr.addr < heap->base_addr.addr ||
      reserved_dev_addr.addr + size >
         reserved_dev_addr.addr & ((ws->page_size) - 1)) {
   result = vk_error(NULL, VK_ERROR_INITIALIZATION_FAILED);
               /* Reserve the virtual range in the MMU and create a mapping structure */
   result = pvr_srv_int_reserve_addr(ws->render_fd,
                           if (result != VK_SUCCESS)
            srv_vma->base.dev_addr = reserved_dev_addr;
   srv_vma->base.bo = NULL;
   srv_vma->base.heap = heap;
                                    err_vk_free_srv_vma:
            err_out:
         }
      VkResult pvr_srv_winsys_heap_alloc(struct pvr_winsys_heap *heap,
                     {
      struct pvr_srv_winsys_heap *const srv_heap = to_pvr_srv_winsys_heap(heap);
   struct pvr_srv_winsys *const srv_ws = to_pvr_srv_winsys(heap->ws);
   struct pvr_srv_winsys_vma *srv_vma;
            srv_vma = vk_alloc(srv_ws->base.alloc,
                     if (!srv_vma) {
      result = vk_error(NULL, VK_ERROR_OUT_OF_HOST_MEMORY);
               result = pvr_winsys_helper_heap_alloc(heap, size, alignment, &srv_vma->base);
   if (result != VK_SUCCESS)
            /* Reserve the virtual range in the MMU and create a mapping structure. */
   result = pvr_srv_int_reserve_addr(srv_ws->base.render_fd,
                           if (result != VK_SUCCESS)
                           err_pvr_srv_free_allocation:
            err_pvr_srv_free_vma:
            err_out:
         }
      void pvr_srv_winsys_heap_free(struct pvr_winsys_vma *vma)
   {
      struct pvr_srv_winsys *srv_ws = to_pvr_srv_winsys(vma->heap->ws);
            /* A vma with an existing device mapping should not be freed. */
            /* Remove mapping handle and underlying reservation. */
            /* Check if we are dealing with carveout address range. */
   if (vma->dev_addr.addr <
      (vma->heap->base_addr.addr + vma->heap->static_data_carveout_size)) {
   /* For the carveout addresses just decrement the reference count. */
      } else {
      /* Free allocated virtual space. */
                  }
      /* * We assume the vma has been allocated with extra space to accommodate the
   *   offset.
   * * The offset passed in is unchanged and can be used to calculate the extra
   *   size that needs to be mapped and final device virtual address.
   */
   VkResult pvr_srv_winsys_vma_map(struct pvr_winsys_vma *vma,
                           {
      struct pvr_srv_winsys_vma *srv_vma = to_pvr_srv_winsys_vma(vma);
   struct pvr_srv_winsys_bo *srv_bo = to_pvr_srv_winsys_bo(bo);
   struct pvr_srv_winsys *srv_ws = to_pvr_srv_winsys(bo->ws);
   const uint64_t srv_flags = srv_bo->flags &
         const uint32_t virt_offset = offset & (vma->heap->page_size - 1);
   const uint64_t aligned_virt_size =
                  /* Address should not be mapped already */
            if (srv_bo->is_display_buffer) {
               /* In case of display buffers, we only support to map whole PMR */
   if (offset != 0 || bo->size != ALIGN_POT(size, srv_ws->base.page_size) ||
      vma->size != bo->size) {
               /* Map the requested pmr */
   result = pvr_srv_int_map_pmr(srv_ws->base.render_fd,
                                 } else {
      const uint32_t phys_page_offset = (offset - virt_offset) >>
         const uint32_t phys_page_count = aligned_virt_size >>
            /* Check if bo and vma can accommodate the given size and offset */
   if (ALIGN_POT(offset + size, vma->heap->page_size) > bo->size ||
      aligned_virt_size > vma->size) {
               /* Map the requested pages */
   result = pvr_srv_int_map_pages(srv_ws->base.render_fd,
                                             if (result != VK_SUCCESS)
                     vma->bo = bo;
   vma->bo_offset = offset;
            if (dev_addr_out)
               }
      void pvr_srv_winsys_vma_unmap(struct pvr_winsys_vma *vma)
   {
      struct pvr_srv_winsys *srv_ws = to_pvr_srv_winsys(vma->heap->ws);
   struct pvr_srv_winsys_vma *srv_vma = to_pvr_srv_winsys_vma(vma);
            /* Address should be mapped */
                     if (srv_bo->is_display_buffer) {
      /* Unmap the requested pmr */
      } else {
      /* Unmap requested pages */
   pvr_srv_int_unmap_pages(srv_ws->base.render_fd,
                                       }
