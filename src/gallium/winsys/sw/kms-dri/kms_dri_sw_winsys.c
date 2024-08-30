   /**************************************************************************
   *
   * Copyright 2009, VMware, Inc.
   * All Rights Reserved.
   * Copyright 2010 George Sapountzis <gsapountzis@gmail.com>
   *           2013 Red Hat, Inc.
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
   * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      #include <stdio.h>
   #include <stdlib.h>
   #include <stddef.h>
   #include <stdint.h>
   #include <string.h>
   #include <limits.h>
   #include <errno.h>
      #include <sys/types.h>
   #include <sys/mman.h>
   #include <unistd.h>
   #include <dlfcn.h>
   #include <fcntl.h>
   #include <xf86drm.h>
      #include "util/compiler.h"
   #include "util/format/u_formats.h"
   #include "pipe/p_state.h"
   #include "util/u_inlines.h"
   #include "util/format/u_format.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/list.h"
      #include "frontend/sw_winsys.h"
   #include "frontend/drm_driver.h"
   #include "kms_dri_sw_winsys.h"
      #include "util/simple_mtx.h"
      #ifdef DEBUG
   #define DEBUG_PRINT(msg, ...) fprintf(stderr, msg, __VA_ARGS__)
   #else
   #define DEBUG_PRINT(msg, ...)
   #endif
      struct kms_sw_displaytarget;
      struct kms_sw_plane
   {
      unsigned width;
   unsigned height;
   unsigned stride;
   unsigned offset;
   struct kms_sw_displaytarget *dt;
      };
      struct kms_sw_displaytarget
   {
      enum pipe_format format;
            uint32_t handle;
   void *mapped;
            int ref_count;
   int map_count;
   struct list_head link;
   struct list_head planes;
      };
      struct kms_sw_winsys
   {
               int fd;
      };
      static inline struct kms_sw_plane *
   kms_sw_plane( struct sw_displaytarget *dt )
   {
         }
      static inline struct sw_displaytarget *
   sw_displaytarget( struct kms_sw_plane *pl)
   {
         }
      static inline struct kms_sw_winsys *
   kms_sw_winsys( struct sw_winsys *ws )
   {
         }
         static bool
   kms_sw_is_displaytarget_format_supported( struct sw_winsys *ws,
               {
      /* TODO: check visuals or other sensible thing here */
      }
      static struct kms_sw_plane *get_plane(struct kms_sw_displaytarget *kms_sw_dt,
                     {
               if (offset + util_format_get_2d_size(format, stride, height) >
      kms_sw_dt->size) {
   DEBUG_PRINT("KMS-DEBUG: plane too big. format: %d stride: %d height: %d "
                           LIST_FOR_EACH_ENTRY(plane, &kms_sw_dt->planes, link) {
      if (plane->offset == offset)
               plane = CALLOC_STRUCT(kms_sw_plane);
   if (!plane)
            plane->width = width;
   plane->height = height;
   plane->stride = stride;
   plane->offset = offset;
   plane->dt = kms_sw_dt;
   list_add(&plane->link, &kms_sw_dt->planes);
      }
      static struct sw_displaytarget *
   kms_sw_displaytarget_create(struct sw_winsys *ws,
                              unsigned tex_usage,
      {
      struct kms_sw_winsys *kms_sw = kms_sw_winsys(ws);
   struct kms_sw_displaytarget *kms_sw_dt;
   struct drm_mode_create_dumb create_req;
   struct drm_mode_destroy_dumb destroy_req;
            kms_sw_dt = CALLOC_STRUCT(kms_sw_displaytarget);
   if (!kms_sw_dt)
            list_inithead(&kms_sw_dt->planes);
   kms_sw_dt->ref_count = 1;
   kms_sw_dt->mapped = MAP_FAILED;
                              memset(&create_req, 0, sizeof(create_req));
   create_req.bpp = util_format_get_blocksizebits(format);
   create_req.width = width;
   create_req.height = height;
   ret = drmIoctl(kms_sw->fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_req);
   if (ret) {
      fprintf(stderr, "KMS: DRM_IOCTL_MODE_CREATE_DUMB failed: %s\n", strerror(errno));
               kms_sw_dt->size = create_req.size;
   kms_sw_dt->handle = create_req.handle;
   struct kms_sw_plane *plane = get_plane(kms_sw_dt, format, width, height,
         if (!plane)
                              *stride = create_req.pitch;
         free_bo:
      memset(&destroy_req, 0, sizeof destroy_req);
   destroy_req.handle = create_req.handle;
   drmIoctl(kms_sw->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy_req);
      no_dt:
         }
      static void
   kms_sw_displaytarget_destroy(struct sw_winsys *ws,
         {
      struct kms_sw_winsys *kms_sw = kms_sw_winsys(ws);
   struct kms_sw_plane *plane = kms_sw_plane(dt);
   struct kms_sw_displaytarget *kms_sw_dt = plane->dt;
            kms_sw_dt->ref_count --;
   if (kms_sw_dt->ref_count > 0)
            if (kms_sw_dt->map_count > 0) {
                  memset(&destroy_req, 0, sizeof destroy_req);
   destroy_req.handle = kms_sw_dt->handle;
                     mtx_destroy(&kms_sw_dt->map_lock);
            struct kms_sw_plane *tmp;
   LIST_FOR_EACH_ENTRY_SAFE(plane, tmp, &kms_sw_dt->planes, link) {
                     }
      static void *
   kms_sw_displaytarget_map(struct sw_winsys *ws,
               {
      struct kms_sw_winsys *kms_sw = kms_sw_winsys(ws);
   struct kms_sw_plane *plane = kms_sw_plane(dt);
   struct kms_sw_displaytarget *kms_sw_dt = plane->dt;
   struct drm_mode_map_dumb map_req;
            mtx_lock(&kms_sw_dt->map_lock);
   memset(&map_req, 0, sizeof map_req);
   map_req.handle = kms_sw_dt->handle;
   ret = drmIoctl(kms_sw->fd, DRM_IOCTL_MODE_MAP_DUMB, &map_req);
   if (ret)
            prot = (flags == PIPE_MAP_READ) ? PROT_READ : (PROT_READ | PROT_WRITE);
   void **ptr = (flags == PIPE_MAP_READ) ? &kms_sw_dt->ro_mapped : &kms_sw_dt->mapped;
   if (*ptr == MAP_FAILED) {
      void *tmp = mmap(NULL, kms_sw_dt->size, prot, MAP_SHARED,
         if (tmp == MAP_FAILED)
                     DEBUG_PRINT("KMS-DEBUG: mapped buffer %u (size %u) at %p\n",
                                 fail_locked:
      mtx_unlock(&kms_sw_dt->map_lock);
      }
      static struct kms_sw_displaytarget *
   kms_sw_displaytarget_find_and_ref(struct kms_sw_winsys *kms_sw,
         {
               LIST_FOR_EACH_ENTRY(kms_sw_dt, &kms_sw->bo_list, link) {
      if (kms_sw_dt->handle == kms_handle) {
                                                }
      static struct kms_sw_plane *
   kms_sw_displaytarget_add_from_prime(struct kms_sw_winsys *kms_sw, int fd,
                     {
      uint32_t handle = -1;
   struct kms_sw_displaytarget * kms_sw_dt;
                     if (ret)
            kms_sw_dt = kms_sw_displaytarget_find_and_ref(kms_sw, handle);
   struct kms_sw_plane *plane = NULL;
   if (kms_sw_dt) {
      plane = get_plane(kms_sw_dt, format, width, height, stride, offset);
   if (!plane)
   kms_sw_dt->ref_count --;
               kms_sw_dt = CALLOC_STRUCT(kms_sw_displaytarget);
   if (!kms_sw_dt)
            list_inithead(&kms_sw_dt->planes);
   off_t lseek_ret = lseek(fd, 0, SEEK_END);
   if (lseek_ret == -1) {
      FREE(kms_sw_dt);
      }
   kms_sw_dt->mapped = MAP_FAILED;
   kms_sw_dt->ro_mapped = MAP_FAILED;
   kms_sw_dt->size = lseek_ret;
   kms_sw_dt->ref_count = 1;
            lseek(fd, 0, SEEK_SET);
   plane = get_plane(kms_sw_dt, format, width, height, stride, offset);
   if (!plane) {
      FREE(kms_sw_dt);
                           }
      static void
   kms_sw_displaytarget_unmap(struct sw_winsys *ws,
         {
      struct kms_sw_plane *plane = kms_sw_plane(dt);
            mtx_lock(&kms_sw_dt->map_lock);
   if (!kms_sw_dt->map_count)  {
      DEBUG_PRINT("KMS-DEBUG: ignore duplicated unmap %u", kms_sw_dt->handle);
   mtx_unlock(&kms_sw_dt->map_lock);
      }
   kms_sw_dt->map_count--;
   if (kms_sw_dt->map_count) {
      DEBUG_PRINT("KMS-DEBUG: ignore unmap for busy buffer %u", kms_sw_dt->handle);
   mtx_unlock(&kms_sw_dt->map_lock);
               DEBUG_PRINT("KMS-DEBUG: unmapped buffer %u (was %p)\n", kms_sw_dt->handle, kms_sw_dt->mapped);
            if (kms_sw_dt->mapped != MAP_FAILED) {
      munmap(kms_sw_dt->mapped, kms_sw_dt->size);
      }
   if (kms_sw_dt->ro_mapped != MAP_FAILED) {
      munmap(kms_sw_dt->ro_mapped, kms_sw_dt->size);
      }
      }
      static struct sw_displaytarget *
   kms_sw_displaytarget_from_handle(struct sw_winsys *ws,
                     {
      struct kms_sw_winsys *kms_sw = kms_sw_winsys(ws);
   struct kms_sw_displaytarget *kms_sw_dt;
               assert(whandle->type == WINSYS_HANDLE_TYPE_KMS ||
            switch(whandle->type) {
   case WINSYS_HANDLE_TYPE_FD:
      kms_sw_pl = kms_sw_displaytarget_add_from_prime(kms_sw, whandle->handle,
                                 if (kms_sw_pl)
            case WINSYS_HANDLE_TYPE_KMS:
      kms_sw_dt = kms_sw_displaytarget_find_and_ref(kms_sw, whandle->handle);
   if (kms_sw_dt) {
      struct kms_sw_plane *plane;
   LIST_FOR_EACH_ENTRY(plane, &kms_sw_dt->planes, link) {
      if (whandle->offset == plane->offset) {
      *stride = plane->stride;
         }
      }
      default:
                  assert(0);
      }
      static bool
   kms_sw_displaytarget_get_handle(struct sw_winsys *winsys,
               {
      struct kms_sw_winsys *kms_sw = kms_sw_winsys(winsys);
   struct kms_sw_plane *plane = kms_sw_plane(dt);
            switch(whandle->type) {
   case WINSYS_HANDLE_TYPE_KMS:
      whandle->handle = kms_sw_dt->handle;
   whandle->stride = plane->stride;
   whandle->offset = plane->offset;
      case WINSYS_HANDLE_TYPE_FD:
      if (!drmPrimeHandleToFD(kms_sw->fd, kms_sw_dt->handle,
            whandle->stride = plane->stride;
   whandle->offset = plane->offset;
      }
      default:
      whandle->handle = 0;
   whandle->stride = 0;
   whandle->offset = 0;
         }
      static void
   kms_sw_displaytarget_display(struct sw_winsys *ws,
                     {
      /* This function should not be called, instead the dri2 loader should
         */
      }
      static int
   kms_sw_winsys_get_fd(struct sw_winsys *ws)
   {
                  }
      static void
   kms_destroy_sw_winsys(struct sw_winsys *winsys)
   {
         }
      struct sw_winsys *
   kms_dri_create_winsys(int fd)
   {
               ws = CALLOC_STRUCT(kms_sw_winsys);
   if (!ws)
            ws->fd = fd;
                                       /* screen texture functions */
   ws->base.displaytarget_create = kms_sw_displaytarget_create;
   ws->base.displaytarget_destroy = kms_sw_displaytarget_destroy;
   ws->base.displaytarget_from_handle = kms_sw_displaytarget_from_handle;
            /* texture functions */
   ws->base.displaytarget_map = kms_sw_displaytarget_map;
                        }
      /* vim: set sw=3 ts=8 sts=3 expandtab: */
