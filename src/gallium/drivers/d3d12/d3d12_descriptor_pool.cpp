   /*
   * Copyright © Microsoft Corporation
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
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "d3d12_context.h"
   #include "d3d12_descriptor_pool.h"
   #include "d3d12_screen.h"
      #include "pipe/p_context.h"
   #include "pipe/p_state.h"
      #include "util/list.h"
   #include "util/u_dynarray.h"
   #include "util/u_memory.h"
      #include <dxguids/dxguids.h>
      struct d3d12_descriptor_pool {
      ID3D12Device *dev;
   D3D12_DESCRIPTOR_HEAP_TYPE type;
   uint32_t num_descriptors;
      };
      struct d3d12_descriptor_heap {
               D3D12_DESCRIPTOR_HEAP_DESC desc;
   ID3D12Device *dev;
   ID3D12DescriptorHeap *heap;
   uint32_t desc_size;
   uint64_t cpu_base;
   uint64_t gpu_base;
   uint32_t size;
   uint32_t next;
   util_dynarray free_list;
      };
      struct d3d12_descriptor_heap*
   d3d12_descriptor_heap_new(ID3D12Device *dev,
                     {
               heap->desc.NumDescriptors = num_descriptors;
   heap->desc.Type = type;
   heap->desc.Flags = flags;
   if (FAILED(dev->CreateDescriptorHeap(&heap->desc,
            FREE(heap);
               heap->dev = dev;
   heap->desc_size = dev->GetDescriptorHandleIncrementSize(type);
   heap->size = num_descriptors * heap->desc_size;
   heap->cpu_base = GetCPUDescriptorHandleForHeapStart(heap->heap).ptr;
   if (flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
                     }
      void
   d3d12_descriptor_heap_free(struct d3d12_descriptor_heap *heap)
   {
      heap->heap->Release();
   util_dynarray_fini(&heap->free_list);
      }
      ID3D12DescriptorHeap*
   d3d12_descriptor_heap_get(struct d3d12_descriptor_heap *heap)
   {
         }
      static uint32_t
   d3d12_descriptor_heap_is_online(struct d3d12_descriptor_heap *heap)
   {
         }
      static uint32_t
   d3d12_descriptor_heap_can_allocate(struct d3d12_descriptor_heap *heap)
   {
      return (heap->free_list.size > 0 ||
      }
      uint32_t
   d3d12_descriptor_heap_get_remaining_handles(struct d3d12_descriptor_heap *heap)
   {
         }
      void
   d2d12_descriptor_heap_get_next_handle(struct d3d12_descriptor_heap *heap,
         {
      handle->heap = heap;
   handle->cpu_handle.ptr = heap->cpu_base + heap->next;
   handle->gpu_handle.ptr = d3d12_descriptor_heap_is_online(heap) ?
      }
      uint32_t
   d3d12_descriptor_heap_alloc_handle(struct d3d12_descriptor_heap *heap,
         {
                        if (heap->free_list.size > 0) {
         } else if (heap->size >= heap->next + heap->desc_size) {
      offset = heap->next;
      } else {
      /* Todo: we should add a new descriptor heap to get more handles */
   assert(0 && "No handles available in descriptor heap");
               handle->heap = heap;
   handle->cpu_handle.ptr = heap->cpu_base + offset;
   handle->gpu_handle.ptr = d3d12_descriptor_heap_is_online(heap) ?
               }
      void
   d3d12_descriptor_handle_free(struct d3d12_descriptor_handle *handle)
   {
      const uint32_t index = handle->cpu_handle.ptr - handle->heap->cpu_base;
   if (index + handle->heap->desc_size == handle->heap->next) {
         } else {
                  handle->heap = NULL;
   handle->cpu_handle.ptr = 0;
      }
      void
   d3d12_descriptor_heap_append_handles(struct d3d12_descriptor_heap *heap,
               {
               assert(heap->next + (num_handles * heap->desc_size) <= heap->size);
   dst.ptr = heap->cpu_base + heap->next;
   heap->dev->CopyDescriptors(1, &dst, &num_handles,
                  }
      void
   d3d12_descriptor_heap_clear(struct d3d12_descriptor_heap *heap)
   {
      heap->next = 0;
      }
      struct d3d12_descriptor_pool*
   d3d12_descriptor_pool_new(struct d3d12_screen *screen,
               {
      struct d3d12_descriptor_pool *pool = CALLOC_STRUCT(d3d12_descriptor_pool);
   if (!pool)
            pool->dev = screen->dev;
   pool->type = type;
   pool->num_descriptors = num_descriptors;
               }
      void
   d3d12_descriptor_pool_free(struct d3d12_descriptor_pool *pool)
   {
      list_for_each_entry_safe(struct d3d12_descriptor_heap, heap, &pool->heaps, link) {
      list_del(&heap->link);
      }
      }
      uint32_t
   d3d12_descriptor_pool_alloc_handle(struct d3d12_descriptor_pool *pool,
         {
               list_for_each_entry(struct d3d12_descriptor_heap, heap, &pool->heaps, link) {
      if (d3d12_descriptor_heap_can_allocate(heap)) {
      valid_heap = heap;
                  if (!valid_heap) {
      valid_heap = d3d12_descriptor_heap_new(pool->dev,
                                    }
