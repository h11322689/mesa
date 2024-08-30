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
      #include <errno.h>
   #include <fcntl.h>
   #include <limits.h>
   #include <stdio.h>
   #include <sys/ioctl.h>
   #include <sys/stat.h>
      #include "util/os_mman.h"
   #include "util/os_file.h"
   #include "util/os_time.h"
   #include "util/simple_mtx.h"
   #include "util/u_memory.h"
   #include "util/format/u_format.h"
   #include "util/u_hash_table.h"
   #include "util/u_inlines.h"
   #include "util/u_pointer.h"
   #include "frontend/drm_driver.h"
   #include "virgl/virgl_screen.h"
   #include "virgl/virgl_public.h"
   #include "virtio-gpu/virgl_protocol.h"
      #include <xf86drm.h>
   #include <libsync.h>
   #include "drm-uapi/virtgpu_drm.h"
      #include "virgl_drm_winsys.h"
   #include "virgl_drm_public.h"
      // Delete local definitions when virglrenderer_hw.h becomes public
   #define VIRGL_DRM_CAPSET_VIRGL  1
   #define VIRGL_DRM_CAPSET_VIRGL2 2
      #define VIRGL_DRM_VERSION(major, minor) ((major) << 16 | (minor))
   #define VIRGL_DRM_VERSION_FENCE_FD      VIRGL_DRM_VERSION(0, 1)
      /* Gets a pointer to the virgl_hw_res containing the pointed to cache entry. */
   #define cache_entry_container_res(ptr) \
            static inline bool can_cache_resource(uint32_t bind)
   {
      return bind == VIRGL_BIND_CONSTANT_BUFFER ||
         bind == VIRGL_BIND_INDEX_BUFFER ||
   bind == VIRGL_BIND_VERTEX_BUFFER ||
   bind == VIRGL_BIND_CUSTOM ||
   bind == VIRGL_BIND_STAGING ||
   bind == VIRGL_BIND_DEPTH_STENCIL ||
      }
      static void virgl_hw_res_destroy(struct virgl_drm_winsys *qdws,
         {
                           /* We intentionally avoid taking the lock in
   * virgl_drm_resource_reference. Now that the
   * lock is taken, we need to check the refcount
   * again. */
   if (pipe_is_referenced(&res->reference)) {
      mtx_unlock(&qdws->bo_handles_mutex);
               _mesa_hash_table_remove_key(qdws->bo_handles,
         if (res->flink_name)
      _mesa_hash_table_remove_key(qdws->bo_names,
      mtx_unlock(&qdws->bo_handles_mutex);
   if (res->ptr)
            memset(&args, 0, sizeof(args));
   args.handle = res->bo_handle;
   drmIoctl(qdws->fd, DRM_IOCTL_GEM_CLOSE, &args);
   }
      static bool virgl_drm_resource_is_busy(struct virgl_winsys *vws,
         {
      struct virgl_drm_winsys *vdws = virgl_drm_winsys(vws);
   struct drm_virtgpu_3d_wait waitcmd;
            if (!p_atomic_read(&res->maybe_busy) && !p_atomic_read(&res->external))
            memset(&waitcmd, 0, sizeof(waitcmd));
   waitcmd.handle = res->bo_handle;
            ret = drmIoctl(vdws->fd, DRM_IOCTL_VIRTGPU_WAIT, &waitcmd);
   if (ret && errno == EBUSY)
                        }
      static void
   virgl_drm_winsys_destroy(struct virgl_winsys *qws)
   {
                        _mesa_hash_table_destroy(qdws->bo_handles, NULL);
   _mesa_hash_table_destroy(qdws->bo_names, NULL);
   mtx_destroy(&qdws->bo_handles_mutex);
               }
      static void virgl_drm_resource_reference(struct virgl_winsys *qws,
               {
      struct virgl_drm_winsys *qdws = virgl_drm_winsys(qws);
                        if (!can_cache_resource(old->bind) ||
      p_atomic_read(&old->external)) {
      } else {
      mtx_lock(&qdws->mutex);
   virgl_resource_cache_add(&qdws->cache, &old->cache_entry);
         }
      }
      static struct virgl_hw_res *
   virgl_drm_winsys_resource_create_blob(struct virgl_winsys *qws,
                                       enum pipe_texture_target target,
   uint32_t format,
   uint32_t bind,
   uint32_t width,
   uint32_t height,
   {
      int ret;
   int32_t blob_id;
   uint32_t cmd[VIRGL_PIPE_RES_CREATE_SIZE + 1] = { 0 };
   struct virgl_drm_winsys *qdws = virgl_drm_winsys(qws);
   struct drm_virtgpu_resource_create_blob drm_rc_blob = { 0 };
   struct virgl_hw_res *res;
   struct virgl_resource_params params = { .size = size,
                                          .bind = bind,
   .format = format,
   .flags = flags,
         res = CALLOC_STRUCT(virgl_hw_res);
   if (!res)
            /* Make sure blob is page aligned. */
   if (flags & (VIRGL_RESOURCE_FLAG_MAP_PERSISTENT |
            width = ALIGN(width, getpagesize());
               blob_id = p_atomic_inc_return(&qdws->blob_id);
   cmd[0] = VIRGL_CMD0(VIRGL_CCMD_PIPE_RESOURCE_CREATE, 0, VIRGL_PIPE_RES_CREATE_SIZE);
   cmd[VIRGL_PIPE_RES_CREATE_FORMAT] = format;
   cmd[VIRGL_PIPE_RES_CREATE_BIND] = bind;
   cmd[VIRGL_PIPE_RES_CREATE_TARGET] = target;
   cmd[VIRGL_PIPE_RES_CREATE_WIDTH] = width;
   cmd[VIRGL_PIPE_RES_CREATE_HEIGHT] = height;
   cmd[VIRGL_PIPE_RES_CREATE_DEPTH] = depth;
   cmd[VIRGL_PIPE_RES_CREATE_ARRAY_SIZE] = array_size;
   cmd[VIRGL_PIPE_RES_CREATE_LAST_LEVEL] = last_level;
   cmd[VIRGL_PIPE_RES_CREATE_NR_SAMPLES] = nr_samples;
   cmd[VIRGL_PIPE_RES_CREATE_FLAGS] = flags;
            drm_rc_blob.cmd = (unsigned long)(void *)&cmd;
   drm_rc_blob.cmd_size = 4 * (VIRGL_PIPE_RES_CREATE_SIZE + 1);
   drm_rc_blob.size = size;
   drm_rc_blob.blob_mem = VIRTGPU_BLOB_MEM_HOST3D;
   drm_rc_blob.blob_flags = VIRTGPU_BLOB_FLAG_USE_MAPPABLE;
            ret = drmIoctl(qdws->fd, DRM_IOCTL_VIRTGPU_RESOURCE_CREATE_BLOB, &drm_rc_blob);
   if (ret != 0) {
      FREE(res);
               res->bind = bind;
   res->res_handle = drm_rc_blob.res_handle;
   res->bo_handle = drm_rc_blob.bo_handle;
   res->size = size;
   res->flags = flags;
   res->maybe_untyped = false;
   pipe_reference_init(&res->reference, 1);
   p_atomic_set(&res->external, false);
   p_atomic_set(&res->num_cs_references, 0);
   virgl_resource_cache_entry_init(&res->cache_entry, params);
      }
      static struct virgl_hw_res *
   virgl_drm_winsys_resource_create(struct virgl_winsys *qws,
                                    enum pipe_texture_target target,
   uint32_t format,
   uint32_t bind,
   uint32_t width,
   uint32_t height,
      {
      struct virgl_drm_winsys *qdws = virgl_drm_winsys(qws);
   struct drm_virtgpu_resource_create createcmd;
   int ret;
   struct virgl_hw_res *res;
   uint32_t stride = width * util_format_get_blocksize(format);
   struct virgl_resource_params params = { .size = size,
                                          .bind = bind,
   .format = format,
   .flags = 0,
         res = CALLOC_STRUCT(virgl_hw_res);
   if (!res)
            memset(&createcmd, 0, sizeof(createcmd));
   createcmd.target = target;
   createcmd.format = pipe_to_virgl_format(format);
   createcmd.bind = bind;
   createcmd.width = width;
   createcmd.height = height;
   createcmd.depth = depth;
   createcmd.array_size = array_size;
   createcmd.last_level = last_level;
   createcmd.nr_samples = nr_samples;
   createcmd.stride = stride;
            ret = drmIoctl(qdws->fd, DRM_IOCTL_VIRTGPU_RESOURCE_CREATE, &createcmd);
   if (ret != 0) {
      FREE(res);
                        res->res_handle = createcmd.res_handle;
   res->bo_handle = createcmd.bo_handle;
   res->size = size;
   res->target = target;
   res->maybe_untyped = false;
   pipe_reference_init(&res->reference, 1);
   p_atomic_set(&res->external, false);
            /* A newly created resource is considered busy by the kernel until the
   * command is retired.  But for our purposes, we can consider it idle
   * unless it is used for fencing.
   */
                        }
      /*
   * Previously, with DRM_IOCTL_VIRTGPU_RESOURCE_CREATE, all host resources had
   * a guest memory shadow resource with size = stride * bpp.  Virglrenderer
   * would guess the stride implicitly when performing transfer operations, if
   * the stride wasn't specified.  Interestingly, vtest would specify the stride.
   *
   * Guessing the stride breaks down with YUV images, which may be imported into
   * Mesa as 3R8 images. It also doesn't work if an external allocator
   * (i.e, minigbm) decides to use a stride not equal to stride * bpp. With blob
   * resources, the size = stride * bpp restriction no longer holds, so use
   * explicit strides passed into Mesa.
   */
   static inline bool use_explicit_stride(struct virgl_hw_res *res, uint32_t level,
         {
      return (params[param_resource_blob].value &&
         res->blob_mem == VIRTGPU_BLOB_MEM_HOST3D_GUEST &&
      }
      static int
   virgl_bo_transfer_put(struct virgl_winsys *vws,
                           {
      struct virgl_drm_winsys *vdws = virgl_drm_winsys(vws);
                     memset(&tohostcmd, 0, sizeof(tohostcmd));
   tohostcmd.bo_handle = res->bo_handle;
   tohostcmd.box.x = box->x;
   tohostcmd.box.y = box->y;
   tohostcmd.box.z = box->z;
   tohostcmd.box.w = box->width;
   tohostcmd.box.h = box->height;
   tohostcmd.box.d = box->depth;
   tohostcmd.offset = buf_offset;
            if (use_explicit_stride(res, level, box->depth))
               }
      static int
   virgl_bo_transfer_get(struct virgl_winsys *vws,
                           {
      struct virgl_drm_winsys *vdws = virgl_drm_winsys(vws);
                     memset(&fromhostcmd, 0, sizeof(fromhostcmd));
   fromhostcmd.bo_handle = res->bo_handle;
   fromhostcmd.level = level;
   fromhostcmd.offset = buf_offset;
   fromhostcmd.box.x = box->x;
   fromhostcmd.box.y = box->y;
   fromhostcmd.box.z = box->z;
   fromhostcmd.box.w = box->width;
   fromhostcmd.box.h = box->height;
            if (use_explicit_stride(res, level, box->depth))
               }
      static struct virgl_hw_res *
   virgl_drm_winsys_resource_cache_create(struct virgl_winsys *qws,
                                          enum pipe_texture_target target,
   const void *map_front_private,
   uint32_t format,
   uint32_t bind,
   uint32_t width,
      {
      bool need_sync = false;
   struct virgl_drm_winsys *qdws = virgl_drm_winsys(qws);
   struct virgl_hw_res *res;
   struct virgl_resource_cache_entry *entry;
   struct virgl_resource_params params = { .size = size,
                                    .bind = bind,
   .format = format,
   .flags = flags,
   .nr_samples = nr_samples,
         if (!can_cache_resource(bind))
                     entry = virgl_resource_cache_remove_compatible(&qdws->cache, params);
   if (entry) {
      res = cache_entry_container_res(entry);
   mtx_unlock(&qdws->mutex);
   pipe_reference_init(&res->reference, 1);
                     alloc:
      /* PIPE_BUFFER with VIRGL_BIND_CUSTOM flag will access data when attaching,
   * in order to avoid race conditions we need to treat it as busy during
   * creation
   */
   if (target == PIPE_BUFFER && (bind & VIRGL_BIND_CUSTOM))
            if (flags & (VIRGL_RESOURCE_FLAG_MAP_PERSISTENT |
            res = virgl_drm_winsys_resource_create_blob(qws, target, format, bind,
                  else
      res = virgl_drm_winsys_resource_create(qws, target, format, bind, width,
                     }
      static uint32_t
   virgl_drm_winsys_resource_get_storage_size(struct virgl_winsys *qws,
         {
         }
      static struct virgl_hw_res *
   virgl_drm_winsys_resource_create_handle(struct virgl_winsys *qws,
                                       {
      struct virgl_drm_winsys *qdws = virgl_drm_winsys(qws);
   struct drm_gem_open open_arg = {};
   struct drm_virtgpu_resource_info info_arg = {};
   struct virgl_hw_res *res = NULL;
            if (whandle->plane >= VIRGL_MAX_PLANE_COUNT) {
                  if (whandle->offset != 0 && whandle->type == WINSYS_HANDLE_TYPE_SHARED) {
      _debug_printf("attempt to import unsupported winsys offset %u\n",
            } else if (whandle->type == WINSYS_HANDLE_TYPE_FD) {
      *plane = whandle->plane;
   *stride = whandle->stride;
   *plane_offset = whandle->offset;
                        /* We must maintain a list of pairs <handle, bo>, so that we always return
   * the same BO for one particular handle. If we didn't do that and created
   * more than one BO for the same handle and then relocated them in a CS,
   * we would hit a deadlock in the kernel.
   *
   * The list of pairs is guarded by a mutex, of course. */
   if (whandle->type == WINSYS_HANDLE_TYPE_SHARED) {
         } else if (whandle->type == WINSYS_HANDLE_TYPE_FD) {
      int r;
   r = drmPrimeFDToHandle(qdws->fd, whandle->handle, &handle);
   if (r)
            } else {
      /* Unknown handle type */
               if (res) {
      /* qdws->bo_{names,handles} hold weak pointers to virgl_hw_res. Because
   * virgl_drm_resource_reference does not take qdws->bo_handles_mutex
   * until it enters virgl_hw_res_destroy, there is a small window that
   * the refcount can drop to zero. Call p_atomic_inc directly instead of
   * virgl_drm_resource_reference to avoid hitting assert failures.
   */
   p_atomic_inc(&res->reference.count);
               res = CALLOC_STRUCT(virgl_hw_res);
   if (!res)
            if (whandle->type == WINSYS_HANDLE_TYPE_FD) {
         } else {
      memset(&open_arg, 0, sizeof(open_arg));
   open_arg.name = whandle->handle;
   if (drmIoctl(qdws->fd, DRM_IOCTL_GEM_OPEN, &open_arg)) {
      FREE(res);
   res = NULL;
      }
   res->bo_handle = open_arg.handle;
               memset(&info_arg, 0, sizeof(info_arg));
            if (drmIoctl(qdws->fd, DRM_IOCTL_VIRTGPU_RESOURCE_INFO, &info_arg)) {
      /* close */
   FREE(res);
   res = NULL;
               res->res_handle = info_arg.res_handle;
   res->blob_mem = info_arg.blob_mem;
            res->size = info_arg.size;
   res->maybe_untyped = info_arg.blob_mem ? true : false;
   pipe_reference_init(&res->reference, 1);
   p_atomic_set(&res->external, true);
            if (res->flink_name)
               done:
      mtx_unlock(&qdws->bo_handles_mutex);
      }
      static void
   virgl_drm_winsys_resource_set_type(struct virgl_winsys *qws,
                                    struct virgl_hw_res *res,
      {
      struct virgl_drm_winsys *qdws = virgl_drm_winsys(qws);
   uint32_t cmd[VIRGL_PIPE_RES_SET_TYPE_SIZE(VIRGL_MAX_PLANE_COUNT) + 1];
   struct drm_virtgpu_execbuffer eb;
                     if (!res->maybe_untyped) {
      mtx_unlock(&qdws->bo_handles_mutex);
      }
                     cmd[0] = VIRGL_CMD0(VIRGL_CCMD_PIPE_RESOURCE_SET_TYPE, 0, VIRGL_PIPE_RES_SET_TYPE_SIZE(plane_count));
   cmd[VIRGL_PIPE_RES_SET_TYPE_RES_HANDLE] = res->res_handle,
   cmd[VIRGL_PIPE_RES_SET_TYPE_FORMAT] = format;
   cmd[VIRGL_PIPE_RES_SET_TYPE_BIND] = bind;
   cmd[VIRGL_PIPE_RES_SET_TYPE_WIDTH] = width;
   cmd[VIRGL_PIPE_RES_SET_TYPE_HEIGHT] = height;
   cmd[VIRGL_PIPE_RES_SET_TYPE_USAGE] = usage;
   cmd[VIRGL_PIPE_RES_SET_TYPE_MODIFIER_LO] = (uint32_t)modifier;
   cmd[VIRGL_PIPE_RES_SET_TYPE_MODIFIER_HI] = (uint32_t)(modifier >> 32);
   for (uint32_t i = 0; i < plane_count; i++) {
      cmd[VIRGL_PIPE_RES_SET_TYPE_PLANE_STRIDE(i)] = plane_strides[i];
               memset(&eb, 0, sizeof(eb));
   eb.command = (uintptr_t)cmd;
   eb.size = (1 + VIRGL_PIPE_RES_SET_TYPE_SIZE(plane_count)) * 4;
   eb.num_bo_handles = 1;
            ret = drmIoctl(qdws->fd, DRM_IOCTL_VIRTGPU_EXECBUFFER, &eb);
   if (ret == -1)
               }
      static bool virgl_drm_winsys_resource_get_handle(struct virgl_winsys *qws,
                     {
      struct virgl_drm_winsys *qdws = virgl_drm_winsys(qws);
            if (!res)
            if (whandle->type == WINSYS_HANDLE_TYPE_SHARED) {
      if (!res->flink_name) {
                     if (drmIoctl(qdws->fd, DRM_IOCTL_GEM_FLINK, &flink)) {
                        mtx_lock(&qdws->bo_handles_mutex);
   _mesa_hash_table_insert(qdws->bo_names, (void *)(uintptr_t)res->flink_name, res);
      }
      } else if (whandle->type == WINSYS_HANDLE_TYPE_KMS) {
         } else if (whandle->type == WINSYS_HANDLE_TYPE_FD) {
      if (drmPrimeHandleToFD(qdws->fd, res->bo_handle, DRM_CLOEXEC, (int*)&whandle->handle))
         mtx_lock(&qdws->bo_handles_mutex);
   _mesa_hash_table_insert(qdws->bo_handles, (void *)(uintptr_t)res->bo_handle, res);
                        whandle->stride = stride;
      }
      static void *virgl_drm_resource_map(struct virgl_winsys *qws,
         {
      struct virgl_drm_winsys *qdws = virgl_drm_winsys(qws);
   struct drm_virtgpu_map mmap_arg;
            if (res->ptr)
            memset(&mmap_arg, 0, sizeof(mmap_arg));
   mmap_arg.handle = res->bo_handle;
   if (drmIoctl(qdws->fd, DRM_IOCTL_VIRTGPU_MAP, &mmap_arg))
            ptr = os_mmap(0, res->size, PROT_READ|PROT_WRITE, MAP_SHARED,
         if (ptr == MAP_FAILED)
            res->ptr = ptr;
         }
      static void virgl_drm_resource_wait(struct virgl_winsys *qws,
         {
      struct virgl_drm_winsys *qdws = virgl_drm_winsys(qws);
   struct drm_virtgpu_3d_wait waitcmd;
            if (!p_atomic_read(&res->maybe_busy) && !p_atomic_read(&res->external))
            memset(&waitcmd, 0, sizeof(waitcmd));
            ret = drmIoctl(qdws->fd, DRM_IOCTL_VIRTGPU_WAIT, &waitcmd);
   if (ret)
               }
      static bool virgl_drm_alloc_res_list(struct virgl_drm_cmd_buf *cbuf,
         {
      cbuf->nres = initial_size;
            cbuf->res_bo = CALLOC(cbuf->nres, sizeof(struct virgl_hw_buf*));
   if (!cbuf->res_bo)
            cbuf->res_hlist = MALLOC(cbuf->nres * sizeof(uint32_t));
   if (!cbuf->res_hlist) {
      FREE(cbuf->res_bo);
                  }
      static void virgl_drm_free_res_list(struct virgl_drm_cmd_buf *cbuf)
   {
               for (i = 0; i < cbuf->cres; i++) {
      p_atomic_dec(&cbuf->res_bo[i]->num_cs_references);
      }
   FREE(cbuf->res_hlist);
      }
      static bool virgl_drm_lookup_res(struct virgl_drm_cmd_buf *cbuf,
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
      static void virgl_drm_add_res(struct virgl_drm_winsys *qdws,
               {
               if (cbuf->cres >= cbuf->nres) {
      unsigned new_nres = cbuf->nres + 256;
   void *new_ptr = REALLOC(cbuf->res_bo,
               if (!new_ptr) {
      _debug_printf("failure to add relocation %d, %d\n", cbuf->cres, new_nres);
      }
            new_ptr = REALLOC(cbuf->res_hlist,
               if (!new_ptr) {
      _debug_printf("failure to add hlist relocation %d, %d\n", cbuf->cres, cbuf->nres);
      }
   cbuf->res_hlist = new_ptr;
               cbuf->res_bo[cbuf->cres] = NULL;
   virgl_drm_resource_reference(&qdws->base, &cbuf->res_bo[cbuf->cres], res);
   cbuf->res_hlist[cbuf->cres] = res->bo_handle;
            cbuf->reloc_indices_hashlist[hash] = cbuf->cres;
   p_atomic_inc(&res->num_cs_references);
      }
      /* This is called after the cbuf is submitted. */
   static void virgl_drm_clear_res_list(struct virgl_drm_cmd_buf *cbuf)
   {
               for (i = 0; i < cbuf->cres; i++) {
      /* mark all BOs busy after submission */
            p_atomic_dec(&cbuf->res_bo[i]->num_cs_references);
                           }
      static void virgl_drm_emit_res(struct virgl_winsys *qws,
               {
      struct virgl_drm_winsys *qdws = virgl_drm_winsys(qws);
   struct virgl_drm_cmd_buf *cbuf = virgl_drm_cmd_buf(_cbuf);
            if (write_buf)
            if (!already_in_list)
      }
      static bool virgl_drm_res_is_ref(struct virgl_winsys *qws,
               {
      if (!p_atomic_read(&res->num_cs_references))
               }
      static struct virgl_cmd_buf *virgl_drm_cmd_buf_create(struct virgl_winsys *qws,
         {
               cbuf = CALLOC_STRUCT(virgl_drm_cmd_buf);
   if (!cbuf)
                     if (!virgl_drm_alloc_res_list(cbuf, 512)) {
      FREE(cbuf);
               cbuf->buf = CALLOC(size, sizeof(uint32_t));
   if (!cbuf->buf) {
      FREE(cbuf->res_hlist);
   FREE(cbuf->res_bo);
   FREE(cbuf);
               cbuf->in_fence_fd = -1;
   cbuf->base.buf = cbuf->buf;
      }
      static void virgl_drm_cmd_buf_destroy(struct virgl_cmd_buf *_cbuf)
   {
                        FREE(cbuf->buf);
      }
      static struct pipe_fence_handle *
   virgl_drm_fence_create(struct virgl_winsys *vws, int fd, bool external)
   {
                        if (external) {
      fd = os_dupfd_cloexec(fd);
   if (fd < 0)
               fence = CALLOC_STRUCT(virgl_drm_fence);
   if (!fence) {
      close(fd);
               fence->fd = fd;
                        }
      static struct pipe_fence_handle *
   virgl_drm_fence_create_legacy(struct virgl_winsys *vws)
   {
                        fence = CALLOC_STRUCT(virgl_drm_fence);
   if (!fence)
                  /* Resources for fences should not be from the cache, since we are basing
   * the fence status on the resource creation busy status.
   */
   fence->hw_res = virgl_drm_winsys_resource_create(vws, PIPE_BUFFER,
         if (!fence->hw_res) {
      FREE(fence);
                           }
      static int virgl_drm_winsys_submit_cmd(struct virgl_winsys *qws,
               {
      struct virgl_drm_winsys *qdws = virgl_drm_winsys(qws);
   struct virgl_drm_cmd_buf *cbuf = virgl_drm_cmd_buf(_cbuf);
   struct drm_virtgpu_execbuffer eb;
            if (cbuf->base.cdw == 0)
            memset(&eb, 0, sizeof(struct drm_virtgpu_execbuffer));
   eb.command = (unsigned long)(void*)cbuf->buf;
   eb.size = cbuf->base.cdw * 4;
   eb.num_bo_handles = cbuf->cres;
            eb.fence_fd = -1;
   if (qws->supports_fences) {
      if (cbuf->in_fence_fd >= 0) {
      eb.flags |= VIRTGPU_EXECBUF_FENCE_FD_IN;
               if (fence != NULL)
      } else {
                  ret = drmIoctl(qdws->fd, DRM_IOCTL_VIRTGPU_EXECBUFFER, &eb);
   if (ret == -1)
                  if (qws->supports_fences) {
      if (cbuf->in_fence_fd >= 0) {
      close(cbuf->in_fence_fd);
               if (fence != NULL && ret == 0)
      } else {
      if (fence != NULL && ret == 0)
                           }
      static int virgl_drm_get_caps(struct virgl_winsys *vws,
         {
      struct virgl_drm_winsys *vdws = virgl_drm_winsys(vws);
   struct drm_virtgpu_get_caps args;
                     memset(&args, 0, sizeof(args));
   if (params[param_capset_fix].value) {
      /* if we have the query fix - try and get cap set id 2 first */
   args.cap_set_id = 2;
      } else {
      args.cap_set_id = 1;
      }
            ret = drmIoctl(vdws->fd, DRM_IOCTL_VIRTGPU_GET_CAPS, &args);
   if (ret == -1 && errno == EINVAL) {
      /* Fallback to v1 */
   args.cap_set_id = 1;
   args.size = sizeof(struct virgl_caps_v1);
   ret = drmIoctl(vdws->fd, DRM_IOCTL_VIRTGPU_GET_CAPS, &args);
   if (ret == -1)
      }
      }
      static struct pipe_fence_handle *
   virgl_cs_create_fence(struct virgl_winsys *vws, int fd)
   {
      if (!vws->supports_fences)
               }
      static bool virgl_fence_wait(struct virgl_winsys *vws,
               {
               if (vws->supports_fences) {
      uint64_t timeout_ms;
            if (timeout == 0)
            timeout_ms = timeout / 1000000;
   /* round up */
   if (timeout_ms * 1000000 < timeout)
                                 if (timeout == 0)
            if (timeout != OS_TIMEOUT_INFINITE) {
      int64_t start_time = os_time_get();
   timeout /= 1000;
   while (virgl_drm_resource_is_busy(vws, fence->hw_res)) {
      if (os_time_get() - start_time >= timeout)
            }
      }
               }
      static void virgl_fence_reference(struct virgl_winsys *vws,
               {
      struct virgl_drm_fence *dfence = virgl_drm_fence(*dst);
            if (pipe_reference(&dfence->reference, &sfence->reference)) {
      if (vws->supports_fences) {
         } else {
         }
                  }
      static void virgl_fence_server_sync(struct virgl_winsys *vws,
               {
      struct virgl_drm_cmd_buf *cbuf = virgl_drm_cmd_buf(_cbuf);
            if (!vws->supports_fences)
            /* if not an external fence, then nothing more to do without preemption: */
   if (!fence->external)
               }
      static int virgl_fence_get_fd(struct virgl_winsys *vws,
         {
               if (!vws->supports_fences)
               }
      static int virgl_drm_get_version(int fd)
   {
   int ret;
   drmVersionPtr version;
      version = drmGetVersion(fd);
      if (!version)
   ret = -EFAULT;
   else if (version->version_major != 0)
   ret = -EINVAL;
   else
   ret = VIRGL_DRM_VERSION(0, version->version_minor);
      drmFreeVersion(version);
      return ret;
   }
      static bool
   virgl_drm_resource_cache_entry_is_busy(struct virgl_resource_cache_entry *entry,
         {
      struct virgl_drm_winsys *qdws = user_data;
               }
      static void
   virgl_drm_resource_cache_entry_release(struct virgl_resource_cache_entry *entry,
         {
      struct virgl_drm_winsys *qdws = user_data;
               }
      static int virgl_init_context(int drmFD)
   {
      int ret;
   struct drm_virtgpu_context_init init = { 0 };
   struct drm_virtgpu_context_set_param ctx_set_param = { 0 };
   uint64_t supports_capset_virgl, supports_capset_virgl2;
            supports_capset_virgl = ((1 << VIRGL_DRM_CAPSET_VIRGL) &
            supports_capset_virgl2 = ((1 << VIRGL_DRM_CAPSET_VIRGL2) &
            if (!supports_capset_virgl && !supports_capset_virgl2) {
      _debug_printf("No virgl contexts available on host");
               ctx_set_param.param = VIRTGPU_CONTEXT_PARAM_CAPSET_ID;
   ctx_set_param.value = (supports_capset_virgl2) ?
                  init.ctx_set_params = (unsigned long)(void *)&ctx_set_param;
            ret = drmIoctl(drmFD, DRM_IOCTL_VIRTGPU_CONTEXT_INIT, &init);
   /*
   * EEXIST happens when a compositor does DUMB_CREATE before initializing
   * virgl.
   */
   if (ret && errno != EEXIST) {
      _debug_printf("DRM_IOCTL_VIRTGPU_CONTEXT_INIT failed with %s\n",
                        }
      static int
   virgl_drm_winsys_get_fd(struct virgl_winsys *vws)
   {
                  }
      static struct virgl_winsys *
   virgl_drm_winsys_create(int drmFD)
   {
      static const unsigned CACHE_TIMEOUT_USEC = 1000000;
   struct virgl_drm_winsys *qdws;
   int drm_version;
            for (uint32_t i = 0; i < ARRAY_SIZE(params); i++) {
      struct drm_virtgpu_getparam getparam = { 0 };
   uint64_t value = 0;
   getparam.param = params[i].param;
   getparam.value = (uint64_t)(uintptr_t)&value;
   ret = drmIoctl(drmFD, DRM_IOCTL_VIRTGPU_GETPARAM, &getparam);
               if (!params[param_3d_features].value)
            drm_version = virgl_drm_get_version(drmFD);
   if (drm_version < 0)
            if (params[param_context_init].value) {
      ret = virgl_init_context(drmFD);
   if (ret)
               qdws = CALLOC_STRUCT(virgl_drm_winsys);
   if (!qdws)
            qdws->fd = drmFD;
   virgl_resource_cache_init(&qdws->cache, CACHE_TIMEOUT_USEC,
                     (void) mtx_init(&qdws->mutex, mtx_plain);
   (void) mtx_init(&qdws->bo_handles_mutex, mtx_plain);
            qdws->bo_handles = util_hash_table_create_ptr_keys();
   qdws->bo_names = util_hash_table_create_ptr_keys();
            qdws->base.transfer_put = virgl_bo_transfer_put;
   qdws->base.transfer_get = virgl_bo_transfer_get;
   qdws->base.resource_create = virgl_drm_winsys_resource_cache_create;
   qdws->base.resource_reference = virgl_drm_resource_reference;
   qdws->base.resource_create_from_handle = virgl_drm_winsys_resource_create_handle;
   qdws->base.resource_set_type = virgl_drm_winsys_resource_set_type;
   qdws->base.resource_get_handle = virgl_drm_winsys_resource_get_handle;
   qdws->base.resource_get_storage_size = virgl_drm_winsys_resource_get_storage_size;
   qdws->base.resource_map = virgl_drm_resource_map;
   qdws->base.resource_wait = virgl_drm_resource_wait;
   qdws->base.resource_is_busy = virgl_drm_resource_is_busy;
   qdws->base.cmd_buf_create = virgl_drm_cmd_buf_create;
   qdws->base.cmd_buf_destroy = virgl_drm_cmd_buf_destroy;
   qdws->base.submit_cmd = virgl_drm_winsys_submit_cmd;
   qdws->base.emit_res = virgl_drm_emit_res;
            qdws->base.cs_create_fence = virgl_cs_create_fence;
   qdws->base.fence_wait = virgl_fence_wait;
   qdws->base.fence_reference = virgl_fence_reference;
   qdws->base.fence_server_sync = virgl_fence_server_sync;
   qdws->base.fence_get_fd = virgl_fence_get_fd;
   qdws->base.get_caps = virgl_drm_get_caps;
   qdws->base.get_fd = virgl_drm_winsys_get_fd;
   qdws->base.supports_fences =  drm_version >= VIRGL_DRM_VERSION_FENCE_FD;
            qdws->base.supports_coherent = params[param_resource_blob].value &&
               }
      static struct hash_table *fd_tab = NULL;
   static simple_mtx_t virgl_screen_mutex = SIMPLE_MTX_INITIALIZER;
      static void
   virgl_drm_screen_destroy(struct pipe_screen *pscreen)
   {
      struct virgl_screen *screen = virgl_screen(pscreen);
            simple_mtx_lock(&virgl_screen_mutex);
   destroy = --screen->refcnt == 0;
   if (destroy) {
      int fd = virgl_drm_winsys(screen->vws)->fd;
   _mesa_hash_table_remove_key(fd_tab, intptr_to_pointer(fd));
      }
            if (destroy) {
      pscreen->destroy = screen->winsys_priv;
         }
      static uint32_t
   hash_fd(const void *key)
   {
                  }
      static bool
   equal_fd(const void *key1, const void *key2)
   {
      int ret;
   int fd1 = pointer_to_intptr(key1);
            /* Since the scope of prime handle is limited to drm_file,
   * virgl_screen is only shared at the drm_file level,
   * not at the device (/dev/dri/cardX) level.
   */
   ret = os_same_file_description(fd1, fd2);
   if (ret == 0) {
         } else if (ret < 0) {
               if (!logged) {
      _debug_printf("virgl: os_same_file_description couldn't "
               "determine if two DRM fds reference the same "
                     }
      struct pipe_screen *
   virgl_drm_screen_create(int fd, const struct pipe_screen_config *config)
   {
               simple_mtx_lock(&virgl_screen_mutex);
   if (!fd_tab) {
      fd_tab = _mesa_hash_table_create(NULL, hash_fd, equal_fd);
   if (!fd_tab)
               pscreen = util_hash_table_get(fd_tab, intptr_to_pointer(fd));
   if (pscreen) {
         } else {
      struct virgl_winsys *vws;
            vws = virgl_drm_winsys_create(dup_fd);
   if (!vws) {
      close(dup_fd);
               pscreen = virgl_create_screen(vws, config);
   if (pscreen) {
               /* Bit of a hack, to avoid circular linkage dependency,
   * ie. pipe driver having to call in to winsys, we
   * override the pipe drivers screen->destroy():
   */
   virgl_screen(pscreen)->winsys_priv = pscreen->destroy;
               unlock:
      simple_mtx_unlock(&virgl_screen_mutex);
      }
