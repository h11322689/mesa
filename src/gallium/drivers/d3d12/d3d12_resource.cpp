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
      #include "d3d12_resource.h"
      #include "d3d12_blit.h"
   #include "d3d12_context.h"
   #include "d3d12_format.h"
   #include "d3d12_screen.h"
   #include "d3d12_debug.h"
      #include "pipebuffer/pb_bufmgr.h"
   #include "util/slab.h"
   #include "util/format/u_format.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "util/format/u_format_zs.h"
      #include "frontend/sw_winsys.h"
      #include <dxguids/dxguids.h>
   #include <memory>
      #ifndef _GAMING_XBOX
   #include <wrl/client.h>
   using Microsoft::WRL::ComPtr;
   #endif
      #ifndef GENERIC_ALL
   // This is only added to winadapter.h in newer DirectX-Headers
   #define GENERIC_ALL 0x10000000L
   #endif
      static bool
   can_map_directly(struct pipe_resource *pres)
   {
      return pres->target == PIPE_BUFFER &&
            }
      static void
   init_valid_range(struct d3d12_resource *res)
   {
      if (can_map_directly(&res->base.b))
      }
      static void
   d3d12_resource_destroy(struct pipe_screen *pscreen,
         {
               // When instanciating a planar d3d12_resource, the same resource->dt pointer
   // is copied to all their planes linked list resources
   // Different instances of objects like d3d12_surface, can be pointing
   // to different planes of the same overall (ie. NV12) planar d3d12_resource
   // sharing the same dt, so keep a refcount when destroying them
   // and only destroy it on the last plane being destroyed
   if (resource->dt_refcount > 0)
         if ((resource->dt_refcount == 0) && resource->dt)
   {
      struct d3d12_screen *screen = d3d12_screen(pscreen);
               threaded_resource_deinit(presource);
   if (can_map_directly(presource))
         if (resource->bo)
            }
      static bool
   resource_is_busy(struct d3d12_context *ctx,
               {
      if (d3d12_batch_has_references(d3d12_current_batch(ctx), res->bo, want_to_write))
            bool busy = false;
   d3d12_foreach_submitted_batch(ctx, batch) {
      if (!d3d12_reset_batch(ctx, batch, 0))
      }
      }
      void
   d3d12_resource_wait_idle(struct d3d12_context *ctx,
               {
      if (d3d12_batch_has_references(d3d12_current_batch(ctx), res->bo, want_to_write)) {
         } else {
      d3d12_foreach_submitted_batch(ctx, batch) {
      if (d3d12_batch_has_references(batch, res->bo, want_to_write))
            }
      void
   d3d12_resource_release(struct d3d12_resource *resource)
   {
      if (!resource->bo)
         d3d12_bo_unreference(resource->bo);
      }
      static bool
   init_buffer(struct d3d12_screen *screen,
               {
      struct pb_desc buf_desc;
   struct pb_manager *bufmgr;
            /* Assert that we don't want to create a buffer with one of the emulated
   * formats, these are (currently) only supported when passing the vertex
   * element state */
            if ((templ->flags & PIPE_RESOURCE_FLAG_MAP_PERSISTENT) &&
         {
         }
   switch (res->base.b.usage) {
   case PIPE_USAGE_DEFAULT:
   case PIPE_USAGE_IMMUTABLE:
      bufmgr = screen->cache_bufmgr;
   buf_desc.usage = (pb_usage_flags)PB_USAGE_GPU_READ_WRITE;
      case PIPE_USAGE_DYNAMIC:
   case PIPE_USAGE_STREAM:
      bufmgr = screen->slab_bufmgr;
   buf_desc.usage = (pb_usage_flags)(PB_USAGE_CPU_WRITE | PB_USAGE_GPU_READ);
      case PIPE_USAGE_STAGING:
      bufmgr = screen->readback_slab_bufmgr;
   buf_desc.usage = (pb_usage_flags)(PB_USAGE_GPU_WRITE | PB_USAGE_CPU_READ_WRITE);
      default:
                  /* We can't suballocate buffers that might be bound as a sampler view, *only*
   * because in the case of R32G32B32 formats (12 bytes per pixel), it's not possible
   * to guarantee the offset will be divisible.
   */
   if (templ->bind & PIPE_BIND_SAMPLER_VIEW)
            buf_desc.alignment = D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT;
   res->dxgi_format = DXGI_FORMAT_UNKNOWN;
   buf = bufmgr->create_buffer(bufmgr, templ->width0, &buf_desc);
   if (!buf)
                     }
      static bool
   init_texture(struct d3d12_screen *screen,
               struct d3d12_resource *res,
   const struct pipe_resource *templ,
   {
               res->mip_levels = templ->last_level + 1;
            D3D12_RESOURCE_DESC desc;
   desc.Format = res->dxgi_format;
   desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
   desc.Width = templ->width0;
   desc.Height = templ->height0;
   desc.DepthOrArraySize = templ->array_size;
            desc.SampleDesc.Count = MAX2(templ->nr_samples, 1);
            desc.Flags = D3D12_RESOURCE_FLAG_NONE;
            switch (templ->target) {
   case PIPE_BUFFER:
      desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
   desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
   desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
         case PIPE_TEXTURE_1D:
   case PIPE_TEXTURE_1D_ARRAY:
      desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
         case PIPE_TEXTURE_CUBE:
   case PIPE_TEXTURE_CUBE_ARRAY:
   case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_2D_ARRAY:
   case PIPE_TEXTURE_RECT:
      desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
         case PIPE_TEXTURE_3D:
      desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
   desc.DepthOrArraySize = templ->depth0;
         default:
                  if (templ->bind & PIPE_BIND_SHADER_BUFFER)
            if (templ->bind & PIPE_BIND_RENDER_TARGET)
            if (templ->bind & PIPE_BIND_DEPTH_STENCIL) {
               /* Sadly, we can't set D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE in the
   * case where PIPE_BIND_SAMPLER_VIEW isn't set, because that would
   * prevent us from using the resource with u_blitter, which requires
   * sneaking in sampler-usage throught the back-door.
               /* The VA frontend VaFourccToPipeFormat chooses _UNORM types for RGBx formats as typeless formats
   * such as DXGI_R8G8B8A8_TYPELESS are not supported as Video Processor input/output as specified in:
   * https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/hardware-support-for-direct3d-12-1-formats
   * PIPE_BIND_CUSTOM is used by the video frontend to hint this resource will be used in video and the
   * original format must be not converted to _TYPELESS
   */
   if ( ((templ->bind & PIPE_BIND_CUSTOM) == 0) &&
      (screen->support_shader_images && templ->nr_samples <= 1)) {
   /* Ideally, we'd key off of PIPE_BIND_SHADER_IMAGE for this, but it doesn't
   * seem to be set properly. So, all UAV-capable resources need the UAV flag.
   */
   D3D12_FEATURE_DATA_FORMAT_SUPPORT support = { desc.Format };
   if (SUCCEEDED(screen->dev->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &support, sizeof(support))) &&
      (support.Support2 & (D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD | D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE)) ==
   (D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD | D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE)) {
   desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
                  if (templ->bind & (PIPE_BIND_SCANOUT | PIPE_BIND_LINEAR))
            HRESULT hres = E_FAIL;
            if (heap) {
      init_residency = d3d12_permanently_resident;
   hres = screen->dev->CreatePlacedResource(heap,
                              } else {
               D3D12_HEAP_FLAGS heap_flags = screen->support_create_not_resident ?
                  hres = screen->dev->CreateCommittedResource(&heap_pris,
                                       if (FAILED(hres))
            if (screen->winsys && (templ->bind & PIPE_BIND_DISPLAY_TARGET)) {
      struct sw_winsys *winsys = screen->winsys;
   res->dt = winsys->displaytarget_create(screen->winsys,
                                                               }
      static void
   convert_planar_resource(struct d3d12_resource *res)
   {
      unsigned num_planes = util_format_get_num_planes(res->base.b.format);
   if (num_planes <= 1 || res->base.b.next || !res->bo)
            struct pipe_resource *next = nullptr;
   struct pipe_resource *planes[3] = {
         };
   for (int plane = num_planes - 1; plane >= 0; --plane) {
      struct d3d12_resource *plane_res = d3d12_resource(planes[plane]);
   if (!plane_res) {
      plane_res = CALLOC_STRUCT(d3d12_resource);
   *plane_res = *res;
   plane_res->dt_refcount = num_planes;
   d3d12_bo_reference(plane_res->bo);
   pipe_reference_init(&plane_res->base.b.reference, 1);
               plane_res->base.b.next = next;
            plane_res->plane_slice = plane;
   plane_res->base.b.format = util_format_get_plane_format(res->base.b.format, plane);
   plane_res->base.b.width0 = util_format_get_plane_width(res->base.b.format, plane, res->base.b.width0);
      #if DEBUG
         struct d3d12_screen *screen = d3d12_screen(res->base.b.screen);
   D3D12_RESOURCE_DESC desc = GetDesc(res->bo->res);
   D3D12_PLACED_SUBRESOURCE_FOOTPRINT placed_footprint = {};
   D3D12_SUBRESOURCE_FOOTPRINT *footprint = &placed_footprint.Footprint;
   unsigned subresource = plane * desc.MipLevels * desc.DepthOrArraySize;
   screen->dev->GetCopyableFootprints(&desc, subresource, 1, 0, &placed_footprint, nullptr, nullptr, nullptr);
   assert(plane_res->base.b.width0 == footprint->Width);
   assert(plane_res->base.b.height0 == footprint->Height);
   assert(plane_res->base.b.depth0 == footprint->Depth);
   #endif
         }
      static struct pipe_resource *
   d3d12_resource_create_or_place(struct d3d12_screen *screen,
                           {
                        res->overall_format = templ->format;
   res->plane_slice = 0;
            if (D3D12_DEBUG_RESOURCE & d3d12_debug) {
      debug_printf("D3D12: Create %sresource %s@%d %dx%dx%d as:%d mip:%d\n",
               templ->usage == PIPE_USAGE_STAGING ? "STAGING " :"",
               pipe_reference_init(&res->base.b.reference, 1);
            if (templ->target == PIPE_BUFFER && !heap) {
         } else {
                  if (!ret) {
      FREE(res);
               init_valid_range(res);
   threaded_resource_init(&res->base.b,
      templ->usage == PIPE_USAGE_DEFAULT &&
                              }
      static struct pipe_resource *
   d3d12_resource_create(struct pipe_screen *pscreen,
         {
      struct d3d12_resource *res = CALLOC_STRUCT(d3d12_resource);
   if (!res)
               }
      static struct pipe_resource *
   d3d12_resource_from_handle(struct pipe_screen *pscreen,
               {
      struct d3d12_screen *screen = d3d12_screen(pscreen);
   if (handle->type != WINSYS_HANDLE_TYPE_D3D12_RES &&
      handle->type != WINSYS_HANDLE_TYPE_FD &&
   handle->type != WINSYS_HANDLE_TYPE_WIN32_NAME)
         struct d3d12_resource *res = CALLOC_STRUCT(d3d12_resource);
   if (!res)
            if (templ && templ->next) {
      struct d3d12_resource* next = d3d12_resource(templ->next);
   if (next->bo) {
      res->base.b = *templ;
   res->bo = next->bo;
               #ifdef _WIN32
         #else
         #endif
      #ifndef _GAMING_XBOX
      if (handle->type == WINSYS_HANDLE_TYPE_D3D12_RES) {
      ComPtr<IUnknown> screen_device;
   ComPtr<IUnknown> res_device;
   screen->dev->QueryInterface(screen_device.GetAddressOf());
            if (screen_device.Get() != res_device.Get()) {
      debug_printf("d3d12: Importing resource - Resource's parent device (%p) does not"
                  handle->type = WINSYS_HANDLE_TYPE_FD;
   HRESULT hr = screen->dev->CreateSharedHandle(((ID3D12DeviceChild *)handle->com_obj),
         nullptr,
                  if (FAILED(hr)) {
      debug_printf("d3d12: Error %x - Couldn't export incoming resource com_obj "
                     #endif
      #ifdef _WIN32
      HANDLE d3d_handle_to_close = nullptr;
   if (handle->type == WINSYS_HANDLE_TYPE_WIN32_NAME) {
      screen->dev->OpenSharedHandleByName((LPCWSTR)handle->name, GENERIC_ALL, &d3d_handle_to_close);
         #endif
         ID3D12Resource *d3d12_res = nullptr;
   ID3D12Heap *d3d12_heap = nullptr;
   if (res->bo) {
         } else if (handle->type == WINSYS_HANDLE_TYPE_D3D12_RES) {
      if (handle->modifier == 1) {
         } else {
            } else {
               #ifdef _WIN32
      if (d3d_handle_to_close) {
            #endif
         D3D12_PLACED_SUBRESOURCE_FOOTPRINT placed_footprint = {};
   D3D12_SUBRESOURCE_FOOTPRINT *footprint = &placed_footprint.Footprint;
            if (!d3d12_res && !d3d12_heap)
            if (d3d12_heap) {
      assert(templ);
   assert(!res->bo);
   assert(!d3d12_res);
               pipe_reference_init(&res->base.b.reference, 1);
   res->base.b.screen = pscreen;
            /* Get a description for this plane */
   if (templ && handle->format != templ->format) {
      unsigned subresource = handle->plane * incoming_res_desc.MipLevels * incoming_res_desc.DepthOrArraySize;
      } else {
      footprint->Format = incoming_res_desc.Format;
   footprint->Width = incoming_res_desc.Width;
   footprint->Height = incoming_res_desc.Height;
               if (footprint->Width > UINT32_MAX ||
      footprint->Height > UINT16_MAX) {
   debug_printf("d3d12: Importing resource too large\n");
      }
   res->base.b.width0 = incoming_res_desc.Width;
   res->base.b.height0 = incoming_res_desc.Height;
   res->base.b.depth0 = 1;
            switch (incoming_res_desc.Dimension) {
   case D3D12_RESOURCE_DIMENSION_BUFFER:
      res->base.b.target = PIPE_BUFFER;
   res->base.b.bind = PIPE_BIND_VERTEX_BUFFER | PIPE_BIND_CONSTANT_BUFFER |
      PIPE_BIND_INDEX_BUFFER | PIPE_BIND_STREAM_OUTPUT | PIPE_BIND_SHADER_BUFFER |
         case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
      res->base.b.target = incoming_res_desc.DepthOrArraySize > 1 ?
         res->base.b.array_size = incoming_res_desc.DepthOrArraySize;
      case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
      res->base.b.target = incoming_res_desc.DepthOrArraySize > 1 ?
         res->base.b.array_size = incoming_res_desc.DepthOrArraySize;
      case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
      res->base.b.target = PIPE_TEXTURE_3D;
   res->base.b.depth0 = footprint->Depth;
      default:
      unreachable("Invalid dimension");
      }
   res->base.b.nr_samples = incoming_res_desc.SampleDesc.Count;
   res->base.b.last_level = incoming_res_desc.MipLevels - 1;
   res->base.b.usage = PIPE_USAGE_DEFAULT;
   res->base.b.bind |= PIPE_BIND_SHARED;
   if (incoming_res_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
         if (incoming_res_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
         if (incoming_res_desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
         if ((incoming_res_desc.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) == D3D12_RESOURCE_FLAG_NONE)
            if (templ) {
      if (res->base.b.target == PIPE_TEXTURE_2D_ARRAY &&
         (templ->target == PIPE_TEXTURE_CUBE ||
      if (res->base.b.array_size < 6) {
      debug_printf("d3d12: Importing cube resource with too few array layers\n");
      }
   res->base.b.target = templ->target;
      }
   unsigned templ_samples = MAX2(templ->nr_samples, 1);
   if (res->base.b.target != templ->target ||
      footprint->Width != templ->width0 ||
   footprint->Height != templ->height0 ||
   footprint->Depth != templ->depth0 ||
   res->base.b.array_size != templ->array_size ||
   incoming_res_desc.SampleDesc.Count != templ_samples ||
   res->base.b.last_level != templ->last_level) {
   debug_printf("d3d12: Importing resource with mismatched dimensions: "
      "plane: %d, target: %d vs %d, width: %d vs %d, height: %d vs %d, "
   "depth: %d vs %d, array_size: %d vs %d, samples: %d vs %d, mips: %d vs %d\n",
   handle->plane,
   res->base.b.target, templ->target,
   footprint->Width, templ->width0,
   footprint->Height, templ->height0,
   footprint->Depth, templ->depth0,
   res->base.b.array_size, templ->array_size,
   incoming_res_desc.SampleDesc.Count, templ_samples,
         }
   if (templ->target != PIPE_BUFFER) {
      if ((footprint->Format != d3d12_get_format(templ->format) &&
      footprint->Format != d3d12_get_typeless_format(templ->format)) ||
   (incoming_res_desc.Format != d3d12_get_format((enum pipe_format)handle->format) &&
   incoming_res_desc.Format != d3d12_get_typeless_format((enum pipe_format)handle->format))) {
   debug_printf("d3d12: Importing resource with mismatched format: "
      "plane could be DXGI format %d or %d, but is %d, "
   "overall could be DXGI format %d or %d, but is %d\n",
   d3d12_get_format(templ->format),
   d3d12_get_typeless_format(templ->format),
   footprint->Format,
   d3d12_get_format((enum pipe_format)handle->format),
   d3d12_get_typeless_format((enum pipe_format)handle->format),
            }
   /* In an ideal world we'd be able to validate this, but gallium's use of bind
   * flags during resource creation is pretty bad: some bind flags are always set
   * (like PIPE_BIND_RENDER_TARGET) while others are never set (PIPE_BIND_SHADER_BUFFER)
   * 
   if (templ->bind & ~res->base.b.bind) {
      debug_printf("d3d12: Imported resource doesn't have necessary bind flags\n");
               res->base.b.format = templ->format;
      } else {
      /* Search the pipe format lookup table for an entry */
            if (res->base.b.format == PIPE_FORMAT_NONE) {
                     if (res->base.b.format == PIPE_FORMAT_NONE) {
      debug_printf("d3d12: Unable to deduce non-typeless resource format %d\n", incoming_res_desc.Format);
                              if (!templ)
            res->dxgi_format = d3d12_get_format(res->overall_format);
   res->plane_slice = handle->plane;
            if (!res->bo) {
         }
            threaded_resource_init(&res->base.b, false);
                  invalid:
      if (res->bo)
         else if (d3d12_res)
         FREE(res);
      }
      static bool
   d3d12_resource_get_handle(struct pipe_screen *pscreen,
                           {
      struct d3d12_resource *res = d3d12_resource(pres);
            switch (handle->type) {
   case WINSYS_HANDLE_TYPE_D3D12_RES:
      handle->com_obj = d3d12_resource_resource(res);
      case WINSYS_HANDLE_TYPE_FD: {
               screen->dev->CreateSharedHandle(d3d12_resource_resource(res),
                           if (!d3d_handle)
         #ifdef _WIN32
         #else
         #endif
         handle->format = pres->format;
   handle->modifier = ~0ull;
      }
   default:
            }
      struct pipe_resource *
   d3d12_resource_from_resource(struct pipe_screen *pscreen,
         {
      D3D12_RESOURCE_DESC input_desc = GetDesc(input_res);
   struct winsys_handle handle;
   memset(&handle, 0, sizeof(handle));
   handle.type = WINSYS_HANDLE_TYPE_D3D12_RES;
   handle.format = d3d12_get_pipe_format(input_desc.Format);
   handle.com_obj = input_res;
            struct pipe_resource templ;
   memset(&templ, 0, sizeof(templ));
   if(input_desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
         } else {
         }
      templ.format = d3d12_get_pipe_format(input_desc.Format);
   templ.width0 = input_desc.Width;
   templ.height0 = input_desc.Height;
   templ.depth0 = input_desc.DepthOrArraySize;
   templ.array_size = input_desc.DepthOrArraySize;
            return d3d12_resource_from_handle(
      pscreen,
   &templ,
   &handle,
         }
      /**
   * On Map/Unmap operations, we readback or flush all the underlying planes
   * of planar resources. The map/unmap operation from the caller is 
   * expected to be done for res->plane_slice plane only, but some
   * callers expect adjacent allocations for next contiguous plane access
   * 
   * In this function, we take the res and box the caller passed, and the plane_* properties
   * that are currently being readback/flushed, and adjust the d3d12_transfer ptrans
   * accordingly for the GPU copy operation between planes.
   */
   static void d3d12_adjust_transfer_dimensions_for_plane(const struct d3d12_resource *res,
                                       {
      /* Adjust strides, offsets to the corresponding plane*/
   ptrans->stride = plane_stride;
   ptrans->layer_stride = plane_layer_stride;
            /* Find multipliers such that:*/
   /* first_plane.width = width_multiplier * planes[res->plane_slice].width*/
   /* first_plane.height = height_multiplier * planes[res->plane_slice].height*/
   float width_multiplier = res->first_plane->width0 / (float) util_format_get_plane_width(res->overall_format, res->plane_slice, res->first_plane->width0);
   float height_multiplier = res->first_plane->height0 / (float) util_format_get_plane_height(res->overall_format, res->plane_slice, res->first_plane->height0);
      /* Normalize box back to overall dimensions (first plane)*/
   ptrans->box.width = width_multiplier * original_box->width;
   ptrans->box.height = height_multiplier * original_box->height;
   ptrans->box.x = width_multiplier * original_box->x;
            /* Now adjust dimensions to plane_slice*/
   ptrans->box.width = util_format_get_plane_width(res->overall_format, plane_slice, ptrans->box.width);
   ptrans->box.height = util_format_get_plane_height(res->overall_format, plane_slice, ptrans->box.height);
   ptrans->box.x = util_format_get_plane_width(res->overall_format, plane_slice, ptrans->box.x);
      }
      static
   void d3d12_resource_get_planes_info(pipe_resource *pres,
                                       {
      struct d3d12_resource* res = d3d12_resource(pres);
   *staging_res_size = 0;
   struct pipe_resource *cur_plane_resource = res->first_plane;
   for (uint plane_slice = 0; plane_slice < num_planes; ++plane_slice) {
      planes[plane_slice] = cur_plane_resource;
   int width = util_format_get_plane_width(res->base.b.format, plane_slice, res->first_plane->width0);
            strides[plane_slice] = align(util_format_get_stride(cur_plane_resource->format, width),
            layer_strides[plane_slice] = align(util_format_get_2d_size(cur_plane_resource->format,
                        offsets[plane_slice] = *staging_res_size;
   *staging_res_size += layer_strides[plane_slice];
         }
      static constexpr unsigned d3d12_max_planes = 3;
      /**
   * Get stride and offset for the given pipe resource without the need to get
   * a winsys_handle.
   */
   void
   d3d12_resource_get_info(struct pipe_screen *pscreen,
                     {
         struct d3d12_resource* res = d3d12_resource(pres);
            pipe_resource *planes[d3d12_max_planes];
   unsigned int strides[d3d12_max_planes];
   unsigned int layer_strides[d3d12_max_planes];
   unsigned int offsets[d3d12_max_planes];
   unsigned staging_res_size = 0;
   d3d12_resource_get_planes_info(
      pres,
   num_planes,
   planes,
   strides,
   layer_strides,
   offsets,
               if(stride) {
                  if(offset) {
            }
      static struct pipe_memory_object *
   d3d12_memobj_create_from_handle(struct pipe_screen *pscreen, struct winsys_handle *handle, bool dedicated)
   {
      if (handle->type != WINSYS_HANDLE_TYPE_WIN32_HANDLE &&
      handle->type != WINSYS_HANDLE_TYPE_WIN32_NAME) {
   debug_printf("d3d12: Unsupported memobj handle type\n");
                  #ifdef _GAMING_XBOX
         #else
         #endif
         #ifdef _WIN32
         #else
         #endif
      #ifdef _WIN32
         HANDLE d3d_handle_to_close = nullptr;
   if (handle->type == WINSYS_HANDLE_TYPE_WIN32_NAME) {
      screen->dev->OpenSharedHandleByName((LPCWSTR) handle->name, GENERIC_ALL, &d3d_handle_to_close);
      #endif
               #ifdef _WIN32
      if (d3d_handle_to_close) {
            #endif
         if (!obj) {
      debug_printf("d3d12: Failed to open memobj handle as anything\n");
               struct d3d12_memory_object *memobj = CALLOC_STRUCT(d3d12_memory_object);
   if (!memobj) {
      obj->Release();
      }
            obj->AddRef();
   if (handle->modifier == 1) {
         } else {
                  obj->Release();
   if (!memobj->res && !memobj->heap) {
      debug_printf("d3d12: Memory object isn't a resource or heap\n");
   free(memobj);
               bool expect_dedicated = memobj->res != nullptr;
   if (dedicated != expect_dedicated)
      debug_printf("d3d12: Expected dedicated to be %s for imported %s\n",
                  }
      static void
   d3d12_memobj_destroy(struct pipe_screen *pscreen, struct pipe_memory_object *pmemobj)
   {
      struct d3d12_memory_object *memobj = d3d12_memory_object(pmemobj);
   if (memobj->res)
         if (memobj->heap)
            }
      static pipe_resource *
   d3d12_resource_from_memobj(struct pipe_screen *pscreen,
                     {
               struct winsys_handle whandle = {};
   whandle.type = WINSYS_HANDLE_TYPE_D3D12_RES;
   whandle.com_obj = memobj->res ? (void *) memobj->res : (void *) memobj->heap;
   whandle.offset = offset;
   whandle.format = templ->format;
            // WINSYS_HANDLE_TYPE_D3D12_RES implies taking ownership of the reference
   ((IUnknown *)whandle.com_obj)->AddRef();
      }
      void
   d3d12_screen_resource_init(struct pipe_screen *pscreen)
   {
      pscreen->resource_create = d3d12_resource_create;
   pscreen->resource_from_handle = d3d12_resource_from_handle;
   pscreen->resource_get_handle = d3d12_resource_get_handle;
   pscreen->resource_destroy = d3d12_resource_destroy;
            pscreen->memobj_create_from_handle = d3d12_memobj_create_from_handle;
   pscreen->memobj_destroy = d3d12_memobj_destroy;
      }
      unsigned int
   get_subresource_id(struct d3d12_resource *res, unsigned resid,
         {
      unsigned resource_stride = (res->base.b.last_level + 1) * res->base.b.array_size;
            return resid * resource_stride + z * layer_stride +
      }
      static D3D12_TEXTURE_COPY_LOCATION
   fill_texture_location(struct d3d12_resource *res,
         {
      D3D12_TEXTURE_COPY_LOCATION tex_loc = {0};
            tex_loc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
   tex_loc.SubresourceIndex = subres;
   tex_loc.pResource = d3d12_resource_resource(res);
      }
      static D3D12_TEXTURE_COPY_LOCATION
   fill_buffer_location(struct d3d12_context *ctx,
                        struct d3d12_resource *res,
      {
      D3D12_TEXTURE_COPY_LOCATION buf_loc = {0};
   D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
   uint64_t offset = 0;
   auto descr = GetDesc(d3d12_resource_underlying(res, &offset));
   struct d3d12_screen *screen = d3d12_screen(ctx->base.screen);
            unsigned sub_resid = get_subresource_id(res, resid, z, trans->base.b.level);
            buf_loc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
   buf_loc.pResource = d3d12_resource_underlying(staging_res, &offset);
   buf_loc.PlacedFootprint = footprint;
   buf_loc.PlacedFootprint.Offset = offset;
            if (util_format_has_depth(util_format_description(res->base.b.format)) &&
      screen->opts2.ProgrammableSamplePositionsTier == D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER_NOT_SUPPORTED) {
   buf_loc.PlacedFootprint.Footprint.Width = res->base.b.width0;
   buf_loc.PlacedFootprint.Footprint.Height = res->base.b.height0;
      } else {
      buf_loc.PlacedFootprint.Footprint.Width = ALIGN(trans->base.b.box.width,
         buf_loc.PlacedFootprint.Footprint.Height = ALIGN(trans->base.b.box.height,
         buf_loc.PlacedFootprint.Footprint.Depth = ALIGN(depth,
                           }
      struct copy_info {
      struct d3d12_resource *dst;
   D3D12_TEXTURE_COPY_LOCATION dst_loc;
   UINT dst_x, dst_y, dst_z;
   struct d3d12_resource *src;
   D3D12_TEXTURE_COPY_LOCATION src_loc;
      };
         static void
   copy_texture_region(struct d3d12_context *ctx,
         {
               d3d12_batch_reference_resource(batch, info.src, false);
   d3d12_batch_reference_resource(batch, info.dst, true);
   d3d12_transition_resource_state(ctx, info.src, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_TRANSITION_FLAG_INVALIDATE_BINDINGS);
   d3d12_transition_resource_state(ctx, info.dst, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_TRANSITION_FLAG_INVALIDATE_BINDINGS);
   d3d12_apply_resource_states(ctx, false);
   ctx->cmdlist->CopyTextureRegion(&info.dst_loc, info.dst_x, info.dst_y, info.dst_z,
      }
      static void
   transfer_buf_to_image_part(struct d3d12_context *ctx,
                                 {
      if (D3D12_DEBUG_RESOURCE & d3d12_debug) {
      debug_printf("D3D12: Copy %dx%dx%d + %dx%dx%d from buffer %s to image %s\n",
               trans->base.b.box.x, trans->base.b.box.y, trans->base.b.box.z,
               struct d3d12_screen *screen = d3d12_screen(res->base.b.screen);
   struct copy_info copy_info;
   copy_info.src = staging_res;
   copy_info.src_loc = fill_buffer_location(ctx, res, staging_res, trans, depth, resid, z);
   copy_info.src_loc.PlacedFootprint.Offset += (z  - start_z) * trans->base.b.layer_stride;
   copy_info.src_box = nullptr;
   copy_info.dst = res;
   copy_info.dst_loc = fill_texture_location(res, trans, resid, z);
   if (util_format_has_depth(util_format_description(res->base.b.format)) &&
      screen->opts2.ProgrammableSamplePositionsTier == D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER_NOT_SUPPORTED) {
   copy_info.dst_x = 0;
      } else {
      copy_info.dst_x = trans->base.b.box.x;
      }
   copy_info.dst_z = res->base.b.target == PIPE_TEXTURE_CUBE ? 0 : dest_z;
               }
      static bool
   transfer_buf_to_image(struct d3d12_context *ctx,
                     {
      if (res->base.b.target == PIPE_TEXTURE_3D) {
      assert(resid == 0);
   transfer_buf_to_image_part(ctx, res, staging_res, trans,
            } else {
      int num_layers = trans->base.b.box.depth;
            for (int z = start_z; z < start_z + num_layers; ++z) {
      transfer_buf_to_image_part(ctx, res, staging_res, trans,
         }
      }
      static void
   transfer_image_part_to_buf(struct d3d12_context *ctx,
                                 {
      struct pipe_box *box = &trans->base.b.box;
            struct d3d12_screen *screen = d3d12_screen(res->base.b.screen);
   struct copy_info copy_info;
   copy_info.src_box = nullptr;
   copy_info.src = res;
   copy_info.src_loc = fill_texture_location(res, trans, resid, z);
   copy_info.dst = staging_res;
   copy_info.dst_loc = fill_buffer_location(ctx, res, staging_res, trans,
         copy_info.dst_loc.PlacedFootprint.Offset += (z  - start_layer) * trans->base.b.layer_stride;
            bool whole_resource = util_texrange_covers_whole_level(&res->base.b, trans->base.b.level,
               if (util_format_has_depth(util_format_description(res->base.b.format)) &&
      screen->opts2.ProgrammableSamplePositionsTier == D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER_NOT_SUPPORTED)
      if (!whole_resource) {
      src_box.left = box->x;
   src_box.right = box->x + box->width;
   src_box.top = box->y;
   src_box.bottom = box->y + box->height;
   src_box.front = start_box_z;
   src_box.back = start_box_z + depth;
                  }
      static bool
   transfer_image_to_buf(struct d3d12_context *ctx,
                           {
         /* We only suppport loading from either an texture array
   * or a ZS texture, so either resid is zero, or num_layers == 1)
   */
            if (D3D12_DEBUG_RESOURCE & d3d12_debug) {
      debug_printf("D3D12: Copy %dx%dx%d + %dx%dx%d from %s@%d to %s\n",
               trans->base.b.box.x, trans->base.b.box.y, trans->base.b.box.z,
               struct pipe_resource *resolved_resource = nullptr;
   if (res->base.b.nr_samples > 1) {
      struct pipe_resource tmpl = res->base.b;
   tmpl.nr_samples = 0;
   resolved_resource = d3d12_resource_create(ctx->base.screen, &tmpl);
   struct pipe_blit_info resolve_info = {};
   struct pipe_box box = {0,0,0, (int)res->base.b.width0, (int16_t)res->base.b.height0, (int16_t)res->base.b.depth0};
   resolve_info.dst.resource = resolved_resource;
   resolve_info.dst.box = box;
   resolve_info.dst.format = res->base.b.format;
   resolve_info.src.resource = &res->base.b;
   resolve_info.src.box = box;
   resolve_info.src.format = res->base.b.format;
   resolve_info.filter = PIPE_TEX_FILTER_NEAREST;
                  d3d12_blit(&ctx->base, &resolve_info);
                  if (res->base.b.target == PIPE_TEXTURE_3D) {
      transfer_image_part_to_buf(ctx, res, staging_res, trans, resid,
      } else {
      int start_layer = trans->base.b.box.z;
   for (int z = start_layer; z < start_layer + trans->base.b.box.depth; ++z) {
      transfer_image_part_to_buf(ctx, res, staging_res, trans, resid,
                              }
      static void
   transfer_buf_to_buf(struct d3d12_context *ctx,
                     struct d3d12_resource *src,
   struct d3d12_resource *dst,
   {
               d3d12_batch_reference_resource(batch, src, false);
            uint64_t src_offset_suballoc = 0;
   uint64_t dst_offset_suballoc = 0;
   auto src_d3d12 = d3d12_resource_underlying(src, &src_offset_suballoc);
   auto dst_d3d12 = d3d12_resource_underlying(dst, &dst_offset_suballoc);
   src_offset += src_offset_suballoc;
            // Same-resource copies not supported, since the resource would need to be in both states
   assert(src_d3d12 != dst_d3d12);
   d3d12_transition_resource_state(ctx, src, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_TRANSITION_FLAG_INVALIDATE_BINDINGS);
   d3d12_transition_resource_state(ctx, dst, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_TRANSITION_FLAG_INVALIDATE_BINDINGS);
   d3d12_apply_resource_states(ctx, false);
   ctx->cmdlist->CopyBufferRegion(dst_d3d12, dst_offset,
            }
      static unsigned
   linear_offset(int x, int y, int z, unsigned stride, unsigned layer_stride)
   {
      return x +
            }
      static D3D12_RANGE
   linear_range(const struct pipe_box *box, unsigned stride, unsigned layer_stride)
   {
               range.Begin = linear_offset(box->x, box->y, box->z,
         range.End = linear_offset(box->x + box->width,
                           }
      static bool
   synchronize(struct d3d12_context *ctx,
               struct d3d12_resource *res,
   {
               /* Check whether that range contains valid data; if not, we might not need to sync */
   if (!(usage & PIPE_MAP_UNSYNCHRONIZED) &&
      usage & PIPE_MAP_WRITE &&
   !util_ranges_intersect(&res->valid_buffer_range, range->Begin, range->End)) {
               if (!(usage & PIPE_MAP_UNSYNCHRONIZED) && resource_is_busy(ctx, res, usage & PIPE_MAP_WRITE)) {
      if (usage & PIPE_MAP_DONTBLOCK) {
      if (d3d12_batch_has_references(d3d12_current_batch(ctx), res->bo, usage & PIPE_MAP_WRITE))
                                 if (usage & PIPE_MAP_WRITE)
      util_range_add(&res->base.b, &res->valid_buffer_range,
            }
      /* A wrapper to make sure local resources are freed and unmapped with
   * any exit path */
   struct local_resource {
      local_resource(pipe_screen *s, struct pipe_resource *tmpl) :
         {
                  ~local_resource() {
      if (res) {
      if (mapped)
                        void *
   map() {
      void *ptr;
   ptr = d3d12_bo_map(res->bo, nullptr);
   if (ptr)
                     void unmap()
   {
      if (mapped)
                     operator struct d3d12_resource *() {
                  bool operator !() {
            private:
      struct d3d12_resource *res;
      };
      /* Combined depth-stencil needs a special handling for reading back: DX handled
   * depth and stencil parts as separate resources and handles copying them only
   * by using seperate texture copy calls with different formats. So create two
   * buffers, read back both resources and interleave the data.
   */
   static void
   prepare_zs_layer_strides(struct d3d12_screen *screen,
                     {
      bool copy_whole_resource = screen->opts2.ProgrammableSamplePositionsTier == D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER_NOT_SUPPORTED;
   int width = copy_whole_resource ? res->base.b.width0 : box->width;
            trans->base.b.stride = align(util_format_get_stride(res->base.b.format, width),
         trans->base.b.layer_stride = util_format_get_2d_size(res->base.b.format,
                  if (copy_whole_resource) {
      trans->zs_cpu_copy_stride = align(util_format_get_stride(res->base.b.format, box->width),
         trans->zs_cpu_copy_layer_stride = util_format_get_2d_size(res->base.b.format,
            } else {
      trans->zs_cpu_copy_stride = trans->base.b.stride;
         }
      static void *
   read_zs_surface(struct d3d12_context *ctx, struct d3d12_resource *res,
               {
      pipe_screen *pscreen = ctx->base.screen;
                     struct pipe_resource tmpl;
   memset(&tmpl, 0, sizeof tmpl);
   tmpl.target = PIPE_BUFFER;
   tmpl.format = PIPE_FORMAT_R32_UNORM;
   tmpl.bind = 0;
   tmpl.usage = PIPE_USAGE_STAGING;
   tmpl.flags = 0;
   tmpl.width0 = trans->base.b.layer_stride;
   tmpl.height0 = 1;
   tmpl.depth0 = 1;
            local_resource depth_buffer(pscreen, &tmpl);
   if (!depth_buffer) {
      debug_printf("Allocating staging buffer for depth failed\n");
               if (!transfer_image_to_buf(ctx, res, depth_buffer, trans, 0))
                     local_resource stencil_buffer(pscreen, &tmpl);
   if (!stencil_buffer) {
      debug_printf("Allocating staging buffer for stencilfailed\n");
               if (!transfer_image_to_buf(ctx, res, stencil_buffer, trans, 1))
                     uint8_t *depth_ptr = (uint8_t *)depth_buffer.map();
   if (!depth_ptr) {
      debug_printf("Mapping staging depth buffer failed\n");
               uint8_t *stencil_ptr =  (uint8_t *)stencil_buffer.map();
   if (!stencil_ptr) {
      debug_printf("Mapping staging stencil buffer failed\n");
               uint8_t *buf = (uint8_t *)malloc(trans->zs_cpu_copy_layer_stride);
   if (!buf)
                     switch (res->base.b.format) {
   case PIPE_FORMAT_Z24_UNORM_S8_UINT:
      if (screen->opts2.ProgrammableSamplePositionsTier == D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER_NOT_SUPPORTED) {
      depth_ptr += trans->base.b.box.y * trans->base.b.stride + trans->base.b.box.x * 4;
      }
   util_format_z24_unorm_s8_uint_pack_separate(buf, trans->zs_cpu_copy_stride,
                        case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
      if (screen->opts2.ProgrammableSamplePositionsTier == D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER_NOT_SUPPORTED) {
      depth_ptr += trans->base.b.box.y * trans->base.b.stride + trans->base.b.box.x * 4;
      }
   util_format_z32_float_s8x24_uint_pack_z_float(buf, trans->zs_cpu_copy_stride,
               util_format_z32_float_s8x24_uint_pack_s_8uint(buf, trans->zs_cpu_copy_stride,
                  default:
                     }
      static void *
   prepare_write_zs_surface(struct d3d12_resource *res,
               {
      struct d3d12_screen *screen = d3d12_screen(res->base.b.screen);
   prepare_zs_layer_strides(screen, res, box, trans);
   uint32_t *buf = (uint32_t *)malloc(trans->base.b.layer_stride);
   if (!buf)
            trans->data = buf;
      }
      static void
   write_zs_surface(struct pipe_context *pctx, struct d3d12_resource *res,
         {
      struct d3d12_screen *screen = d3d12_screen(res->base.b.screen);
   struct pipe_resource tmpl;
   memset(&tmpl, 0, sizeof tmpl);
   tmpl.target = PIPE_BUFFER;
   tmpl.format = PIPE_FORMAT_R32_UNORM;
   tmpl.bind = 0;
   tmpl.usage = PIPE_USAGE_STAGING;
   tmpl.flags = 0;
   tmpl.width0 = trans->base.b.layer_stride;
   tmpl.height0 = 1;
   tmpl.depth0 = 1;
            local_resource depth_buffer(pctx->screen, &tmpl);
   if (!depth_buffer) {
      debug_printf("Allocating staging buffer for depth failed\n");
               local_resource stencil_buffer(pctx->screen, &tmpl);
   if (!stencil_buffer) {
      debug_printf("Allocating staging buffer for depth failed\n");
               uint8_t *depth_ptr = (uint8_t *)depth_buffer.map();
   if (!depth_ptr) {
      debug_printf("Mapping staging depth buffer failed\n");
               uint8_t *stencil_ptr =  (uint8_t *)stencil_buffer.map();
   if (!stencil_ptr) {
      debug_printf("Mapping staging stencil buffer failed\n");
               switch (res->base.b.format) {
   case PIPE_FORMAT_Z24_UNORM_S8_UINT:
      if (screen->opts2.ProgrammableSamplePositionsTier == D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER_NOT_SUPPORTED) {
      depth_ptr += trans->base.b.box.y * trans->base.b.stride + trans->base.b.box.x * 4;
      }
   util_format_z32_unorm_unpack_z_32unorm((uint32_t *)depth_ptr, trans->base.b.stride, (uint8_t*)trans->data,
               util_format_z24_unorm_s8_uint_unpack_s_8uint(stencil_ptr, trans->base.b.stride, (uint8_t*)trans->data,
                  case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
      if (screen->opts2.ProgrammableSamplePositionsTier == D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER_NOT_SUPPORTED) {
      depth_ptr += trans->base.b.box.y * trans->base.b.stride + trans->base.b.box.x * 4;
      }
   util_format_z32_float_s8x24_uint_unpack_z_float((float *)depth_ptr, trans->base.b.stride, (uint8_t*)trans->data,
               util_format_z32_float_s8x24_uint_unpack_s_8uint(stencil_ptr, trans->base.b.stride, (uint8_t*)trans->data,
                  default:
                  stencil_buffer.unmap();
            transfer_buf_to_image(d3d12_context(pctx), res, depth_buffer, trans, 0);
      }
      #define BUFFER_MAP_ALIGNMENT 64
      static void *
   d3d12_transfer_map(struct pipe_context *pctx,
                     struct pipe_resource *pres,
   unsigned level,
   {
      struct d3d12_context *ctx = d3d12_context(pctx);
   struct d3d12_resource *res = d3d12_resource(pres);
            if (usage & PIPE_MAP_DIRECTLY || !res->bo)
            slab_child_pool* transfer_pool = (usage & TC_TRANSFER_MAP_THREADED_UNSYNC) ?
         struct d3d12_transfer *trans = (struct d3d12_transfer *)slab_zalloc(transfer_pool);
   struct pipe_transfer *ptrans = &trans->base.b;
   if (!trans)
            ptrans->level = level;
   ptrans->usage = (enum pipe_map_flags)usage;
            D3D12_RANGE range;
            void *ptr;
   if (can_map_directly(&res->base.b)) {
      if (pres->target == PIPE_BUFFER) {
      ptrans->stride = 0;
      } else {
      ptrans->stride = util_format_get_stride(pres->format, box->width);
   ptrans->layer_stride = util_format_get_2d_size(pres->format,
                     range = linear_range(box, ptrans->stride, ptrans->layer_stride);
   if (!synchronize(ctx, res, usage, &range)) {
      slab_free(transfer_pool, trans);
      }
      } else if (unlikely(pres->format == PIPE_FORMAT_Z24_UNORM_S8_UINT ||
            if (usage & PIPE_MAP_READ) {
         } else if (usage & PIPE_MAP_WRITE){
         } else {
                                 unsigned num_planes = util_format_get_num_planes(res->overall_format);
   pipe_resource *planes[d3d12_max_planes];
   unsigned int strides[d3d12_max_planes];
   unsigned int layer_strides[d3d12_max_planes];
   unsigned int offsets[d3d12_max_planes];
            d3d12_resource_get_planes_info(
      pres,
   num_planes,
   planes,
   strides,
   layer_strides,
   offsets,
      );
               pipe_resource_usage staging_usage = (usage & (PIPE_MAP_READ | PIPE_MAP_READ_WRITE)) ?
         trans->staging_res = pipe_buffer_create(pctx->screen, 0,
               if (!trans->staging_res)
                              /* Read all planes if readback needed*/
   if (usage & PIPE_MAP_READ) {
      pipe_box original_box = ptrans->box;
   for (uint plane_slice = 0; plane_slice < num_planes; ++plane_slice) {
      /* Adjust strides, offsets, box to the corresponding plane for the copytexture operation*/
   d3d12_adjust_transfer_dimensions_for_plane(res,
                                       /* Perform the readback*/
   if(!transfer_image_to_buf(ctx, d3d12_resource(planes[plane_slice]), staging_res, trans, 0)){
            }
   ptrans->box = original_box;
               /* Map the whole staging buffer containing all the planes contiguously*/
            range.End = staging_res_size - range.Begin;
            ptrans->stride = strides[res->plane_slice];
   ptrans->layer_stride = layer_strides[res->plane_slice];
         } else {
      ptrans->stride = align(util_format_get_stride(pres->format, box->width),
         ptrans->layer_stride = util_format_get_2d_size(pres->format,
                  if (res->base.b.target != PIPE_TEXTURE_3D)
                  if (util_format_has_depth(util_format_description(pres->format)) &&
      screen->opts2.ProgrammableSamplePositionsTier == D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER_NOT_SUPPORTED) {
   trans->zs_cpu_copy_stride = ptrans->stride;
   trans->zs_cpu_copy_layer_stride = ptrans->layer_stride;
      ptrans->stride = align(util_format_get_stride(pres->format, pres->width0),
         ptrans->layer_stride = util_format_get_2d_size(pres->format,
                  range.Begin = box->y * ptrans->stride +
               unsigned staging_res_size = ptrans->layer_stride * box->depth;
   if (res->base.b.target == PIPE_BUFFER) {
      /* To properly support ARB_map_buffer_alignment, we need to return a pointer
   * that's appropriately offset from a 64-byte-aligned base address.
   */
   assert(box->x >= 0);
   unsigned aligned_x = (unsigned)box->x % BUFFER_MAP_ALIGNMENT;
   staging_res_size = align(box->width + aligned_x,
                     pipe_resource_usage staging_usage = (usage & (PIPE_MAP_DISCARD_RANGE | PIPE_MAP_DISCARD_WHOLE_RESOURCE)) ?
            trans->staging_res = pipe_buffer_create(pctx->screen, 0,
               if (!trans->staging_res) {
      slab_free(transfer_pool, trans);
                        if ((usage & (PIPE_MAP_DISCARD_RANGE | PIPE_MAP_DISCARD_WHOLE_RESOURCE | TC_TRANSFER_MAP_THREADED_UNSYNC)) == 0) {
      bool ret = true;
   if (pres->target == PIPE_BUFFER) {
      uint64_t src_offset = box->x;
   uint64_t dst_offset = src_offset % BUFFER_MAP_ALIGNMENT;
      } else
         if (!ret)
                                          pipe_resource_reference(&ptrans->resource, pres);
   *transfer = ptrans;
      }
      static void
   d3d12_transfer_unmap(struct pipe_context *pctx,
         {
      struct d3d12_context *ctx = d3d12_context(pctx);
   struct d3d12_resource *res = d3d12_resource(ptrans->resource);
   struct d3d12_transfer *trans = (struct d3d12_transfer *)ptrans;
            if (trans->data != nullptr) {
      if (trans->base.b.usage & PIPE_MAP_WRITE)
            } else if (trans->staging_res) {
                  /* Get planes information*/
   unsigned num_planes = util_format_get_num_planes(res->overall_format);
   pipe_resource *planes[d3d12_max_planes];
   unsigned int strides[d3d12_max_planes];
   unsigned int layer_strides[d3d12_max_planes];
                  d3d12_resource_get_planes_info(
      ptrans->resource,
   num_planes,
   planes,
   strides,
   layer_strides,
   offsets,
                        /* In theory we should just flush only the contents for the plane*/
   /* requested in res->plane_slice, but the VAAPI frontend has this*/
   /* behaviour in which they assume that mapping the first plane of*/
   /* NV12, P010, etc resources will will give them a buffer containing*/
   /* both Y and UV planes contigously in vaDeriveImage and then vaMapBuffer*/
   /* so, flush them all*/
      struct d3d12_resource *staging_res = d3d12_resource(trans->staging_res);
   if (trans->base.b.usage & PIPE_MAP_WRITE) {
      assert(ptrans->box.x >= 0);
   range.Begin = res->base.b.target == PIPE_BUFFER ?
         range.End = staging_res->base.b.width0 - range.Begin;
      d3d12_bo_unmap(staging_res->bo, &range);
   pipe_box original_box = ptrans->box;
   for (uint plane_slice = 0; plane_slice < num_planes; ++plane_slice) {
      /* Adjust strides, offsets to the corresponding plane for the copytexture operation*/
   d3d12_adjust_transfer_dimensions_for_plane(res,
                                             }
                  } else {
      struct d3d12_resource *staging_res = d3d12_resource(trans->staging_res);
   if (trans->base.b.usage & PIPE_MAP_WRITE) {
      assert(ptrans->box.x >= 0);
   range.Begin = res->base.b.target == PIPE_BUFFER ?
                           if (trans->base.b.usage & PIPE_MAP_WRITE) {
      struct d3d12_context *ctx = d3d12_context(pctx);
   if (res->base.b.target == PIPE_BUFFER) {
      uint64_t dst_offset = trans->base.b.box.x;
   uint64_t src_offset = dst_offset % BUFFER_MAP_ALIGNMENT;
      } else
                     } else {
      if (trans->base.b.usage & PIPE_MAP_WRITE) {
      range.Begin = ptrans->box.x;
      }
               pipe_resource_reference(&ptrans->resource, NULL);
      }
      void
   d3d12_context_resource_init(struct pipe_context *pctx)
   {
      pctx->buffer_map = d3d12_transfer_map;
   pctx->buffer_unmap = d3d12_transfer_unmap;
   pctx->texture_map = d3d12_transfer_map;
            pctx->transfer_flush_region = u_default_transfer_flush_region;
   pctx->buffer_subdata = u_default_buffer_subdata;
      }
