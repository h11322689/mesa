   /*
   * Copyright 2014, 2015 Red Hat.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   */
   #include <stdio.h>
   #include "util/u_surface.h"
   #include "util/u_memory.h"
   #include "util/format/u_format.h"
   #include "util/u_inlines.h"
   #include "util/os_time.h"
   #include "frontend/sw_winsys.h"
   #include "util/os_mman.h"
      #include "virgl_vtest_winsys.h"
   #include "virgl_vtest_public.h"
      /* Gets a pointer to the virgl_hw_res containing the pointed to cache entry. */
   #define cache_entry_container_res(ptr) \
            static void *virgl_vtest_resource_map(struct virgl_winsys *vws,
         static void virgl_vtest_resource_unmap(struct virgl_winsys *vws,
            static inline bool can_cache_resource_with_bind(uint32_t bind)
   {
      return bind == VIRGL_BIND_CONSTANT_BUFFER ||
         bind == VIRGL_BIND_INDEX_BUFFER ||
   bind == VIRGL_BIND_VERTEX_BUFFER ||
      }
      static uint32_t vtest_get_transfer_size(struct virgl_hw_res *res,
                     {
               valid_stride = util_format_get_stride(res->format, box->width);
   if (stride) {
      if (box->height > 1)
               valid_layer_stride = util_format_get_2d_size(res->format, valid_stride,
         if (layer_stride) {
      if (box->depth > 1)
               *valid_stride_p = valid_stride;
      }
      static int
   virgl_vtest_transfer_put(struct virgl_winsys *vws,
                           {
      struct virgl_vtest_winsys *vtws = virgl_vtest_winsys(vws);
   uint32_t size;
   void *ptr;
            size = vtest_get_transfer_size(res, box, stride, layer_stride, level,
            virgl_vtest_send_transfer_put(vtws, res->res_handle,
                  if (vtws->protocol_version >= 2)
            ptr = virgl_vtest_resource_map(vws, res);
   virgl_vtest_send_transfer_put_data(vtws, ptr + buf_offset, size);
   virgl_vtest_resource_unmap(vws, res);
      }
      static int
   virgl_vtest_transfer_get_internal(struct virgl_winsys *vws,
                                 {
      struct virgl_vtest_winsys *vtws = virgl_vtest_winsys(vws);
   uint32_t size;
   void *ptr;
            size = vtest_get_transfer_size(res, box, stride, layer_stride, level,
         virgl_vtest_send_transfer_get(vtws, res->res_handle,
                  if (flush_front_buffer || vtws->protocol_version >= 2)
            if (vtws->protocol_version >= 2) {
      if (flush_front_buffer) {
      if (box->depth > 1 || box->z > 1) {
      fprintf(stderr, "Expected a 2D resource, received a 3D resource\n");
                              /*
   * The display target is aligned to 64 bytes, while the shared resource
   * between the client/server is not.
   */
   shm_stride = util_format_get_stride(res->format, res->width);
                  util_copy_rect(dt_map, res->format, res->stride, box->x, box->y,
                  virgl_vtest_resource_unmap(vws, res);
         } else {
      ptr = virgl_vtest_resource_map(vws, res);
   virgl_vtest_recv_transfer_get_data(vtws, ptr + buf_offset, size,
                        }
      static int
   virgl_vtest_transfer_get(struct virgl_winsys *vws,
                           {
      return virgl_vtest_transfer_get_internal(vws, res, box, stride,
            }
      static void virgl_hw_res_destroy(struct virgl_vtest_winsys *vtws,
         {
      virgl_vtest_send_resource_unref(vtws, res->res_handle);
   if (res->dt)
         if (vtws->protocol_version >= 2) {
      if (res->ptr)
      } else {
                     }
      static bool virgl_vtest_resource_is_busy(struct virgl_winsys *vws,
         {
               /* implement busy check */
   int ret;
            if (ret < 0)
               }
      static void virgl_vtest_resource_reference(struct virgl_winsys *vws,
               {
      struct virgl_vtest_winsys *vtws = virgl_vtest_winsys(vws);
            if (pipe_reference(&(*dres)->reference, &sres->reference)) {
      if (!can_cache_resource_with_bind(old->bind)) {
         } else {
      mtx_lock(&vtws->mutex);
   virgl_resource_cache_add(&vtws->cache, &old->cache_entry);
         }
      }
      static struct virgl_hw_res *
   virgl_vtest_winsys_resource_create(struct virgl_winsys *vws,
                                    enum pipe_texture_target target,
   const void *map_front_private,
   uint32_t format,
   uint32_t bind,
   uint32_t width,
      {
      struct virgl_vtest_winsys *vtws = virgl_vtest_winsys(vws);
   struct virgl_hw_res *res;
   static int handle = 1;
   int fd = -1;
   struct virgl_resource_params params = { .size = size,
                                          .bind = bind,
   .format = format,
   .flags = 0,
         res = CALLOC_STRUCT(virgl_hw_res);
   if (!res)
            if (bind & (VIRGL_BIND_DISPLAY_TARGET | VIRGL_BIND_SCANOUT)) {
      res->dt = vtws->sws->displaytarget_create(vtws->sws, bind, format,
               } else if (vtws->protocol_version < 2) {
      res->ptr = align_malloc(size, 64);
   if (!res->ptr) {
      FREE(res);
                  res->bind = bind;
   res->format = format;
   res->height = height;
   res->width = width;
   res->size = size;
   virgl_vtest_send_resource_create(vtws, handle, target, pipe_to_virgl_format(format), bind,
                  if (vtws->protocol_version >= 2) {
      if (res->size == 0) {
      res->ptr = NULL;
   res->res_handle = handle;
               if (fd < 0) {
      FREE(res);
   fprintf(stderr, "Unable to get a valid fd\n");
               res->ptr = os_mmap(NULL, res->size, PROT_WRITE | PROT_READ, MAP_SHARED,
            if (res->ptr == MAP_FAILED) {
      fprintf(stderr, "Client failed to map shared memory region\n");
   close(fd);
   FREE(res);
                           res->res_handle = handle;
   if (map_front_private && res->ptr && res->dt) {
      void *dt_map = vtws->sws->displaytarget_map(vtws->sws, res->dt, PIPE_MAP_READ_WRITE);
   uint32_t shm_stride = util_format_get_stride(res->format, res->width);
   util_copy_rect(res->ptr, res->format, shm_stride, 0, 0,
            struct pipe_box box;
   u_box_2d(0, 0, res->width, res->height, &box);
            out:
      virgl_resource_cache_entry_init(&res->cache_entry, params);
   handle++;
   pipe_reference_init(&res->reference, 1);
   p_atomic_set(&res->num_cs_references, 0);
      }
      static void *virgl_vtest_resource_map(struct virgl_winsys *vws,
         {
               /*
   * With protocol v0 we can either have a display target or a resource backing
   * store. With protocol v2 we can have both, so only return the memory mapped
   * backing store in this function. We can copy to the display target when
   * appropriate.
   */
   if (vtws->protocol_version >= 2 || !res->dt) {
      res->mapped = res->ptr;
      } else {
            }
      static void virgl_vtest_resource_unmap(struct virgl_winsys *vws,
         {
      struct virgl_vtest_winsys *vtws = virgl_vtest_winsys(vws);
   if (res->mapped)
            if (res->dt && vtws->protocol_version < 2)
      }
      static void virgl_vtest_resource_wait(struct virgl_winsys *vws,
         {
                  }
      static struct virgl_hw_res *
   virgl_vtest_winsys_resource_cache_create(struct virgl_winsys *vws,
                                          enum pipe_texture_target target,
   const void *map_front_private,
   uint32_t format,
   uint32_t bind,
   uint32_t width,
      {
      struct virgl_vtest_winsys *vtws = virgl_vtest_winsys(vws);
   struct virgl_hw_res *res;
   struct virgl_resource_cache_entry *entry;
   struct virgl_resource_params params = { .size = size,
                                          .bind = bind,
   .format = format,
   .flags = 0,
         if (!can_cache_resource_with_bind(bind))
                     entry = virgl_resource_cache_remove_compatible(&vtws->cache, params);
   if (entry) {
      res = cache_entry_container_res(entry);
   mtx_unlock(&vtws->mutex);
   pipe_reference_init(&res->reference, 1);
                     alloc:
      res = virgl_vtest_winsys_resource_create(vws, target, map_front_private,
                        }
      static bool virgl_vtest_lookup_res(struct virgl_vtest_cmd_buf *cbuf,
         {
      unsigned hash = res->res_handle & (sizeof(cbuf->is_handle_added)-1);
            if (cbuf->is_handle_added[hash]) {
      i = cbuf->reloc_indices_hashlist[hash];
   if (cbuf->res_bo[i] == res)
            for (i = 0; i < cbuf->cres; i++) {
      if (cbuf->res_bo[i] == res) {
      cbuf->reloc_indices_hashlist[hash] = i;
            }
      }
      static void virgl_vtest_release_all_res(struct virgl_vtest_winsys *vtws,
         {
               for (i = 0; i < cbuf->cres; i++) {
      p_atomic_dec(&cbuf->res_bo[i]->num_cs_references);
      }
      }
      static void virgl_vtest_add_res(struct virgl_vtest_winsys *vtws,
               {
               if (cbuf->cres >= cbuf->nres) {
      unsigned new_nres = cbuf->nres + 256;
   struct virgl_hw_res **new_re_bo = REALLOC(cbuf->res_bo,
               if (!new_re_bo) {
      fprintf(stderr,"failure to add relocation %d, %d\n", cbuf->cres, cbuf->nres);
               cbuf->res_bo = new_re_bo;
               cbuf->res_bo[cbuf->cres] = NULL;
   virgl_vtest_resource_reference(&vtws->base, &cbuf->res_bo[cbuf->cres], res);
            cbuf->reloc_indices_hashlist[hash] = cbuf->cres;
   p_atomic_inc(&res->num_cs_references);
      }
      static struct virgl_cmd_buf *virgl_vtest_cmd_buf_create(struct virgl_winsys *vws,
         {
               cbuf = CALLOC_STRUCT(virgl_vtest_cmd_buf);
   if (!cbuf)
            cbuf->nres = 512;
   cbuf->res_bo = CALLOC(cbuf->nres, sizeof(struct virgl_hw_buf*));
   if (!cbuf->res_bo) {
      FREE(cbuf);
               cbuf->buf = CALLOC(size, sizeof(uint32_t));
   if (!cbuf->buf) {
      FREE(cbuf->res_bo);
   FREE(cbuf);
               cbuf->ws = vws;
   cbuf->base.buf = cbuf->buf;
      }
      static void virgl_vtest_cmd_buf_destroy(struct virgl_cmd_buf *_cbuf)
   {
               virgl_vtest_release_all_res(virgl_vtest_winsys(cbuf->ws), cbuf);
   FREE(cbuf->res_bo);
   FREE(cbuf->buf);
      }
      static struct pipe_fence_handle *
   virgl_vtest_fence_create(struct virgl_winsys *vws)
   {
               /* Resources for fences should not be from the cache, since we are basing
   * the fence status on the resource creation busy status.
   */
   res = virgl_vtest_winsys_resource_create(vws,
                                       }
      static int virgl_vtest_winsys_submit_cmd(struct virgl_winsys *vws,
               {
      struct virgl_vtest_winsys *vtws = virgl_vtest_winsys(vws);
   struct virgl_vtest_cmd_buf *cbuf = virgl_vtest_cmd_buf(_cbuf);
            if (cbuf->base.cdw == 0)
            ret = virgl_vtest_submit_cmd(vtws, cbuf);
   if (fence && ret == 0)
            virgl_vtest_release_all_res(vtws, cbuf);
   memset(cbuf->is_handle_added, 0, sizeof(cbuf->is_handle_added));
   cbuf->base.cdw = 0;
      }
      static void virgl_vtest_emit_res(struct virgl_winsys *vws,
               {
      struct virgl_vtest_winsys *vtws = virgl_vtest_winsys(vws);
   struct virgl_vtest_cmd_buf *cbuf = virgl_vtest_cmd_buf(_cbuf);
            if (write_buf)
         if (!already_in_list)
      }
      static bool virgl_vtest_res_is_ref(struct virgl_winsys *vws,
               {
      if (!p_atomic_read(&res->num_cs_references))
               }
      static int virgl_vtest_get_caps(struct virgl_winsys *vws,
         {
      struct virgl_vtest_winsys *vtws = virgl_vtest_winsys(vws);
            virgl_ws_fill_new_caps_defaults(caps);
   ret = virgl_vtest_send_get_caps(vtws, caps);
   // vtest doesn't support that
   if (caps->caps.v2.capability_bits_v2 & VIRGL_CAP_V2_COPY_TRANSFER_BOTH_DIRECTIONS)
            }
      static struct pipe_fence_handle *
   virgl_cs_create_fence(struct virgl_winsys *vws, int fd)
   {
         }
      static bool virgl_fence_wait(struct virgl_winsys *vws,
               {
               if (timeout == 0)
            if (timeout != OS_TIMEOUT_INFINITE) {
      int64_t start_time = os_time_get();
   timeout /= 1000;
   while (virgl_vtest_resource_is_busy(vws, res)) {
      if (os_time_get() - start_time >= timeout)
            }
      }
   virgl_vtest_resource_wait(vws, res);
      }
      static void virgl_fence_reference(struct virgl_winsys *vws,
               {
      struct virgl_vtest_winsys *vdws = virgl_vtest_winsys(vws);
   virgl_vtest_resource_reference(&vdws->base, (struct virgl_hw_res **)dst,
      }
      static void virgl_vtest_flush_frontbuffer(struct virgl_winsys *vws,
                           {
      struct virgl_vtest_winsys *vtws = virgl_vtest_winsys(vws);
   struct pipe_box box;
   uint32_t offset = 0;
   if (!res->dt)
                     if (sub_box) {
      box = *sub_box;
   uint32_t shm_stride = util_format_get_stride(res->format, res->width);
   offset = box.y / util_format_get_blockheight(res->format) * shm_stride +
      } else {
      box.z = layer;
   box.width = res->width;
   box.height = res->height;
               virgl_vtest_transfer_get_internal(vws, res, &box, res->stride, 0, offset,
            vtws->sws->displaytarget_display(vtws->sws, res->dt, winsys_drawable_handle,
      }
      static void
   virgl_vtest_winsys_destroy(struct virgl_winsys *vws)
   {
                        mtx_destroy(&vtws->mutex);
      }
      static bool
   virgl_vtest_resource_cache_entry_is_busy(struct virgl_resource_cache_entry *entry,
         {
      struct virgl_vtest_winsys *vtws = user_data;
               }
      static void
   virgl_vtest_resource_cache_entry_release(struct virgl_resource_cache_entry *entry,
         {
      struct virgl_vtest_winsys *vtws = user_data;
               }
      struct virgl_winsys *
   virgl_vtest_winsys_wrap(struct sw_winsys *sws)
   {
      static const unsigned CACHE_TIMEOUT_USEC = 1000000;
            vtws = CALLOC_STRUCT(virgl_vtest_winsys);
   if (!vtws)
            virgl_vtest_connect(vtws);
            virgl_resource_cache_init(&vtws->cache, CACHE_TIMEOUT_USEC,
                                       vtws->base.transfer_put = virgl_vtest_transfer_put;
            vtws->base.resource_create = virgl_vtest_winsys_resource_cache_create;
   vtws->base.resource_reference = virgl_vtest_resource_reference;
   vtws->base.resource_map = virgl_vtest_resource_map;
   vtws->base.resource_wait = virgl_vtest_resource_wait;
   vtws->base.resource_is_busy = virgl_vtest_resource_is_busy;
   vtws->base.cmd_buf_create = virgl_vtest_cmd_buf_create;
   vtws->base.cmd_buf_destroy = virgl_vtest_cmd_buf_destroy;
            vtws->base.emit_res = virgl_vtest_emit_res;
   vtws->base.res_is_referenced = virgl_vtest_res_is_ref;
            vtws->base.cs_create_fence = virgl_cs_create_fence;
   vtws->base.fence_wait = virgl_fence_wait;
   vtws->base.fence_reference = virgl_fence_reference;
   vtws->base.supports_fences =  0;
                        }
