   /**************************************************************************
   *
   * Copyright 2010 Thomas Balling Sørensen & Orasanu Lucian.
   * Copyright 2014 Advanced Micro Devices, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   * IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      #include "pipe/p_screen.h"
   #include "frontend/drm_driver.h"
   #include "util/u_memory.h"
   #include "util/u_handle_table.h"
   #include "util/u_transfer.h"
   #include "vl/vl_winsys.h"
      #include "va_private.h"
      #ifdef _WIN32
   #include <va/va_win32.h>
   #endif
      #ifndef VA_MAPBUFFER_FLAG_DEFAULT
   #define VA_MAPBUFFER_FLAG_DEFAULT 0
   #define VA_MAPBUFFER_FLAG_READ    1
   #define VA_MAPBUFFER_FLAG_WRITE   2
   #endif
      VAStatus
   vlVaCreateBuffer(VADriverContextP ctx, VAContextID context, VABufferType type,
               {
      vlVaDriver *drv;
            if (!ctx)
            buf = CALLOC(1, sizeof(vlVaBuffer));
   if (!buf)
            buf->type = type;
   buf->size = size;
            if (buf->type == VAEncCodedBufferType)
         else
            if (!buf->data) {
      FREE(buf);
               if (data)
            drv = VL_VA_DRIVER(ctx);
   mtx_lock(&drv->mutex);
   *buf_id = handle_table_add(drv->htab, buf);
               }
      VAStatus
   vlVaBufferSetNumElements(VADriverContextP ctx, VABufferID buf_id,
         {
      vlVaDriver *drv;
            if (!ctx)
            drv = VL_VA_DRIVER(ctx);
   mtx_lock(&drv->mutex);
   buf = handle_table_get(drv->htab, buf_id);
   mtx_unlock(&drv->mutex);
   if (!buf)
            if (buf->derived_surface.resource)
            buf->data = REALLOC(buf->data, buf->size * buf->num_elements,
                  if (!buf->data)
               }
      VAStatus
   vlVaMapBuffer(VADriverContextP ctx, VABufferID buf_id, void **pbuff)
   {
         }
      VAStatus vlVaMapBuffer2(VADriverContextP ctx, VABufferID buf_id,
         {
      vlVaDriver *drv;
            if (!ctx)
            drv = VL_VA_DRIVER(ctx);
   if (!drv)
            if (!pbuff)
            mtx_lock(&drv->mutex);
   buf = handle_table_get(drv->htab, buf_id);
   if (!buf || buf->export_refcount > 0) {
      mtx_unlock(&drv->mutex);
               if (buf->derived_surface.resource) {
      struct pipe_resource *resource;
   struct pipe_box box;
   unsigned usage = 0;
   void *(*map_func)(struct pipe_context *,
         struct pipe_resource *resource,
   unsigned level,
   unsigned usage,  /* a combination of PIPE_MAP_x */
            memset(&box, 0, sizeof(box));
   resource = buf->derived_surface.resource;
   box.width = resource->width0;
   box.height = resource->height0;
            if (resource->target == PIPE_BUFFER)
         else
            if (flags == VA_MAPBUFFER_FLAG_DEFAULT) {
      /* For VAImageBufferType, use PIPE_MAP_WRITE for now,
   * PIPE_MAP_READ_WRITE degradate perf with two copies when map/unmap. */
   if (buf->type == VAEncCodedBufferType)
                        /* Map decoder and postproc surfaces also for reading. */
   if (buf->derived_surface.entrypoint == PIPE_VIDEO_ENTRYPOINT_BITSTREAM ||
      buf->derived_surface.entrypoint == PIPE_VIDEO_ENTRYPOINT_PROCESSING)
            if (flags & VA_MAPBUFFER_FLAG_READ)
         if (flags & VA_MAPBUFFER_FLAG_WRITE)
                     *pbuff = map_func(drv->pipe, resource, 0, usage,
                  if (!buf->derived_surface.transfer || !*pbuff)
            if (buf->type == VAEncCodedBufferType) {
      ((VACodedBufferSegment*)buf->data)->buf = *pbuff;
   ((VACodedBufferSegment*)buf->data)->size = buf->coded_size;
         } else {
      mtx_unlock(&drv->mutex);
                  }
      VAStatus
   vlVaUnmapBuffer(VADriverContextP ctx, VABufferID buf_id)
   {
      vlVaDriver *drv;
   vlVaBuffer *buf;
            if (!ctx)
            drv = VL_VA_DRIVER(ctx);
   if (!drv)
            mtx_lock(&drv->mutex);
   buf = handle_table_get(drv->htab, buf_id);
   if (!buf || buf->export_refcount > 0) {
      mtx_unlock(&drv->mutex);
               resource = buf->derived_surface.resource;
   if (resource) {
      void (*unmap_func)(struct pipe_context *pipe,
            if (!buf->derived_surface.transfer) {
      mtx_unlock(&drv->mutex);
               if (resource->target == PIPE_BUFFER)
         else
            unmap_func(drv->pipe, buf->derived_surface.transfer);
            if (buf->type == VAImageBufferType)
      }
               }
      VAStatus
   vlVaDestroyBuffer(VADriverContextP ctx, VABufferID buf_id)
   {
      vlVaDriver *drv;
            if (!ctx)
            drv = VL_VA_DRIVER(ctx);
   mtx_lock(&drv->mutex);
   buf = handle_table_get(drv->htab, buf_id);
   if (!buf) {
      mtx_unlock(&drv->mutex);
               if (buf->derived_surface.resource) {
               if (buf->derived_image_buffer)
               FREE(buf->data);
   FREE(buf);
   handle_table_remove(VL_VA_DRIVER(ctx)->htab, buf_id);
               }
      VAStatus
   vlVaBufferInfo(VADriverContextP ctx, VABufferID buf_id, VABufferType *type,
         {
      vlVaDriver *drv;
            if (!ctx)
            drv = VL_VA_DRIVER(ctx);
   mtx_lock(&drv->mutex);
   buf = handle_table_get(drv->htab, buf_id);
   mtx_unlock(&drv->mutex);
   if (!buf)
            *type = buf->type;
   *size = buf->size;
               }
      VAStatus
   vlVaAcquireBufferHandle(VADriverContextP ctx, VABufferID buf_id,
         {
      vlVaDriver *drv;
   uint32_t i;
   uint32_t mem_type;
   vlVaBuffer *buf ;
            /* List of supported memory types, in preferred order. */
      #ifdef _WIN32
         VA_SURFACE_ATTRIB_MEM_TYPE_NTHANDLE,
   #else
         #endif
                     if (!ctx)
            drv = VL_VA_DRIVER(ctx);
   screen = VL_VA_PSCREEN(ctx);
   mtx_lock(&drv->mutex);
   buf = handle_table_get(VL_VA_DRIVER(ctx)->htab, buf_id);
            if (!buf)
            /* Only VA surface|image like buffers are supported for now .*/
   if (buf->type != VAImageBufferType)
            if (!out_buf_info)
            if (!out_buf_info->mem_type)
         else {
      mem_type = 0;
   for (i = 0; mem_types[i] != 0; i++) {
      if (out_buf_info->mem_type & mem_types[i]) {
      mem_type = out_buf_info->mem_type;
         }
   if (!mem_type)
               if (!buf->derived_surface.resource)
            if (buf->export_refcount > 0) {
      if (buf->export_state.mem_type != mem_type)
      } else {
               #ifdef _WIN32
         case VA_SURFACE_ATTRIB_MEM_TYPE_D3D12_RESOURCE:
   #else
         #endif
         {
                                    #ifdef _WIN32
               #endif
            if (!screen->resource_get_handle(screen, drv->pipe,
                  mtx_unlock(&drv->mutex);
                        #ifdef _WIN32
               #endif
               }
   default:
                  buf_info->type = buf->type;
   buf_info->mem_type = mem_type;
                                    }
      VAStatus
   vlVaReleaseBufferHandle(VADriverContextP ctx, VABufferID buf_id)
   {
      vlVaDriver *drv;
            if (!ctx)
            drv = VL_VA_DRIVER(ctx);
   mtx_lock(&drv->mutex);
   buf = handle_table_get(drv->htab, buf_id);
            if (!buf)
            if (buf->export_refcount == 0)
            if (--buf->export_refcount == 0) {
               #ifdef _WIN32
         case VA_SURFACE_ATTRIB_MEM_TYPE_D3D12_RESOURCE:
      // Do nothing for this case.
      case VA_SURFACE_ATTRIB_MEM_TYPE_NTHANDLE:
         #else
         case VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME:
         #endif
         default:
                                 }
      #if VA_CHECK_VERSION(1, 15, 0)
   VAStatus
   vlVaSyncBuffer(VADriverContextP ctx, VABufferID buf_id, uint64_t timeout_ns)
   {
      vlVaDriver *drv;
   vlVaContext *context;
            if (!ctx)
            drv = VL_VA_DRIVER(ctx);
   if (!drv)
            /* Some apps like ffmpeg check for vaSyncBuffer to be present
      to do async enqueuing of multiple vaEndPicture encode calls
   before calling vaSyncBuffer with a pre-defined latency
   If vaSyncBuffer is not implemented, they fallback to the
            As this might require the driver to support multiple
   operations and/or store multiple feedback values before sync
   fallback to backward compatible behaviour unless driver
      */
   if (!drv->pipe->screen->get_video_param(drv->pipe->screen,
                              /* vaSyncBuffer spec states that "If timeout is zero, the function returns immediately." */
   if (timeout_ns == 0)
            if (timeout_ns != VA_TIMEOUT_INFINITE)
            mtx_lock(&drv->mutex);
            if (!buf) {
      mtx_unlock(&drv->mutex);
               if (!buf->feedback) {
      /* No outstanding operation: nothing to do. */
   mtx_unlock(&drv->mutex);
               context = handle_table_get(drv->htab, buf->ctx);
   if (!context) {
      mtx_unlock(&drv->mutex);
                        if ((buf->feedback) && (context->decoder->entrypoint == PIPE_VIDEO_ENTRYPOINT_ENCODE)) {
      context->decoder->get_feedback(context->decoder, buf->feedback, &(buf->coded_size));
   buf->feedback = NULL;
   /* Also mark the associated render target (encode source texture) surface as done
         if(surf)
   {
      surf->feedback = NULL;
                  mtx_unlock(&drv->mutex);
      }
   #endif
