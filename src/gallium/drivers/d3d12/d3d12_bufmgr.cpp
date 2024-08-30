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
      #include "d3d12_bufmgr.h"
   #include "d3d12_context.h"
   #include "d3d12_format.h"
   #include "d3d12_screen.h"
      #include "pipebuffer/pb_buffer.h"
   #include "pipebuffer/pb_bufmgr.h"
      #include "util/format/u_format.h"
   #include "util/u_memory.h"
      #include <dxguids/dxguids.h>
      struct d3d12_bufmgr {
                  };
      extern const struct pb_vtbl d3d12_buffer_vtbl;
      static inline struct d3d12_bufmgr *
   d3d12_bufmgr(struct pb_manager *mgr)
   {
                  }
      static void
   describe_direct_bo(char *buf, struct d3d12_bo *ptr)
   {
         }
      static void
   describe_suballoc_bo(char *buf, struct d3d12_bo *ptr)
   {
      char res[128];
   uint64_t offset;
   d3d12_bo *base = d3d12_bo_get_base(ptr, &offset);
   describe_direct_bo(res, base);
   sprintf(buf, "d3d12_bo<suballoc<%s>,0x%x,0x%x>", res,
      }
      void
   d3d12_debug_describe_bo(char *buf, struct d3d12_bo *ptr)
   {
      if (ptr->buffer)
         else
      }
      struct d3d12_bo *
   d3d12_bo_wrap_res(struct d3d12_screen *screen, ID3D12Resource *res, enum d3d12_residency_status residency)
   {
               bo = MALLOC_STRUCT(d3d12_bo);
   if (!bo)
                  D3D12_RESOURCE_DESC desc = GetDesc(res);
   unsigned array_size = desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ? 1 : desc.DepthOrArraySize;
   unsigned total_subresources = desc.MipLevels * array_size * d3d12_non_opaque_plane_count(desc.Format);
            pipe_reference_init(&bo->reference, 1);
   bo->screen = screen;
   bo->res = res;
   bo->unique_id = p_atomic_inc_return(&screen->resource_id_generator);
   if (!supports_simultaneous_access)
            bo->residency_status = residency;
   bo->last_used_timestamp = 0;
   screen->dev->GetCopyableFootprints(&desc, 0, total_subresources, 0, nullptr, nullptr, nullptr, &bo->estimated_size);
   if (residency == d3d12_resident) {
      mtx_lock(&screen->submit_mutex);
   list_add(&bo->residency_list_entry, &screen->residency_list);
                  }
      struct d3d12_bo *
   d3d12_bo_new(struct d3d12_screen *screen, uint64_t size, const pb_desc *pb_desc)
   {
      ID3D12Device *dev = screen->dev;
            D3D12_RESOURCE_DESC res_desc;
   res_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
   res_desc.Format = DXGI_FORMAT_UNKNOWN;
   res_desc.Alignment = 0;
   res_desc.Width = size;
   res_desc.Height = 1;
   res_desc.DepthOrArraySize = 1;
   res_desc.MipLevels = 1;
   res_desc.SampleDesc.Count = 1;
   res_desc.SampleDesc.Quality = 0;
   res_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
            D3D12_HEAP_TYPE heap_type = D3D12_HEAP_TYPE_DEFAULT;
   if (pb_desc->usage & PB_USAGE_CPU_READ)
         else if (pb_desc->usage & PB_USAGE_CPU_WRITE)
            D3D12_HEAP_FLAGS heap_flags = screen->support_create_not_resident ?
         enum d3d12_residency_status init_residency = screen->support_create_not_resident ?
            D3D12_HEAP_PROPERTIES heap_pris = GetCustomHeapProperties(dev, heap_type);
   HRESULT hres = dev->CreateCommittedResource(&heap_pris,
                                    if (FAILED(hres))
               }
      struct d3d12_bo *
   d3d12_bo_wrap_buffer(struct d3d12_screen *screen, struct pb_buffer *buf)
   {
               bo = MALLOC_STRUCT(d3d12_bo);
   if (!bo)
                  pipe_reference_init(&bo->reference, 1);
   bo->screen = screen;
   bo->buffer = buf;
   bo->unique_id = p_atomic_inc_return(&screen->resource_id_generator);
               }
      void
   d3d12_bo_unreference(struct d3d12_bo *bo)
   {
      if (bo == NULL)
                     if (pipe_reference_described(&bo->reference, NULL,
                  if (bo->buffer)
                     if (bo->residency_status == d3d12_resident)
            /* MSVC's offsetof fails when the name is ambiguous between struct and function */
   typedef struct d3d12_context d3d12_context_type;
   list_for_each_entry(d3d12_context_type, ctx, &bo->screen->context_list, context_list_entry)
                           d3d12_resource_state_cleanup(&bo->global_state);
   if (bo->res)
            uint64_t mask = bo->local_context_state_mask;
   while (mask) {
      int ctxid = u_bit_scan64(&mask);
                     }
      void *
   d3d12_bo_map(struct d3d12_bo *bo, D3D12_RANGE *range)
   {
      struct d3d12_bo *base_bo;
   D3D12_RANGE offset_range = {0, 0};
   uint64_t offset;
                     if (!range || range->Begin >= range->End) {
      offset_range.Begin = offset;
   offset_range.End = offset + d3d12_bo_get_size(bo);
      } else {
      offset_range.Begin = range->Begin + offset;
   offset_range.End = range->End + offset;
               if (FAILED(base_bo->res->Map(0, range, &ptr)))
               }
      void
   d3d12_bo_unmap(struct d3d12_bo *bo, D3D12_RANGE *range)
   {
      struct d3d12_bo *base_bo;
   D3D12_RANGE offset_range = {0, 0};
                     if (!range || range->Begin >= range->End) {
      offset_range.Begin = offset;
   offset_range.End = offset + d3d12_bo_get_size(bo);
      } else {
      offset_range.Begin = range->Begin + offset;
   offset_range.End = range->End + offset;
                  }
      static void
   d3d12_buffer_destroy(void *winsys, struct pb_buffer *pbuf)
   {
               if (buf->map)
         d3d12_bo_unreference(buf->bo);
      }
      static void *
   d3d12_buffer_map(struct pb_buffer *pbuf,
               {
         }
      static void
   d3d12_buffer_unmap(struct pb_buffer *pbuf)
   {
   }
      static void
   d3d12_buffer_get_base_buffer(struct pb_buffer *buf,
               {
      *base_buf = buf;
      }
      static enum pipe_error
   d3d12_buffer_validate(struct pb_buffer *pbuf,
               {
      /* Always pinned */
      }
      static void
   d3d12_buffer_fence(struct pb_buffer *pbuf,
         {
   }
      const struct pb_vtbl d3d12_buffer_vtbl = {
      d3d12_buffer_destroy,
   d3d12_buffer_map,
   d3d12_buffer_unmap,
   d3d12_buffer_validate,
   d3d12_buffer_fence,
      };
      static struct pb_buffer *
   d3d12_bufmgr_create_buffer(struct pb_manager *pmgr,
               {
      struct d3d12_bufmgr *mgr = d3d12_bufmgr(pmgr);
            buf = CALLOC_STRUCT(d3d12_buffer);
   if (!buf)
            pipe_reference_init(&buf->base.reference, 1);
   buf->base.alignment_log2 = util_logbase2(pb_desc->alignment);
   buf->base.usage = pb_desc->usage;
   buf->base.vtbl = &d3d12_buffer_vtbl;
   buf->base.size = size;
   buf->range.Begin = 0;
            buf->bo = d3d12_bo_new(mgr->screen, size, pb_desc);
   if (!buf->bo) {
      FREE(buf);
               if (pb_desc->usage & PB_USAGE_CPU_READ_WRITE) {
      buf->map = d3d12_bo_map(buf->bo, &buf->range);
   if (!buf->map) {
      d3d12_bo_unreference(buf->bo);
   FREE(buf);
                     }
      static void
   d3d12_bufmgr_flush(struct pb_manager *mgr)
   {
         }
      static void
   d3d12_bufmgr_destroy(struct pb_manager *_mgr)
   {
      struct d3d12_bufmgr *mgr = d3d12_bufmgr(_mgr);
      }
      static bool
   d3d12_bufmgr_is_buffer_busy(struct pb_manager *_mgr, struct pb_buffer *_buf)
   {
      /* We're only asked this on buffers that are known not busy */
      }
      struct pb_manager *
   d3d12_bufmgr_create(struct d3d12_screen *screen)
   {
               mgr = CALLOC_STRUCT(d3d12_bufmgr);
   if (!mgr)
            mgr->base.destroy = d3d12_bufmgr_destroy;
   mgr->base.create_buffer = d3d12_bufmgr_create_buffer;
   mgr->base.flush = d3d12_bufmgr_flush;
                        }
