   /*
   * Copyright (C) 2016 Christian Gmeiner <christian.gmeiner@gmail.com>
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   * Authors:
   *    Christian Gmeiner <christian.gmeiner@gmail.com>
   */
      #include "renderonly/renderonly.h"
      #include <errno.h>
   #include <fcntl.h>
   #include <stdio.h>
   #include <xf86drm.h>
      #include "frontend/drm_driver.h"
   #include "pipe/p_screen.h"
   #include "util/format/u_format.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
      void
   renderonly_scanout_destroy(struct renderonly_scanout *scanout,
         {
               assert(p_atomic_read(&scanout->refcnt) > 0);
   if (p_atomic_dec_return(&scanout->refcnt))
                     /* Someone might have imported this BO while we were waiting for the
   * lock, let's make sure it's still not referenced before freeing it.
   */
   if (p_atomic_read(&scanout->refcnt) == 0 && ro->kms_fd != -1) {
      destroy_dumb.handle = scanout->handle;
   scanout->handle = 0;
   scanout->stride = 0;
                  }
      struct renderonly_scanout *
   renderonly_create_kms_dumb_buffer_for_resource(struct pipe_resource *rsc,
               {
      struct renderonly_scanout *scanout = NULL;
   int err;
   struct drm_mode_create_dumb create_dumb = {
      .width = rsc->width0,
   .height = rsc->height0,
      };
            /* create dumb buffer at scanout GPU */
   err = drmIoctl(ro->kms_fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_dumb);
   if (err < 0) {
      fprintf(stderr, "DRM_IOCTL_MODE_CREATE_DUMB failed: %s\n",
                     simple_mtx_lock(&ro->bo_map_lock);
   scanout = util_sparse_array_get(&ro->bo_map, create_dumb.handle);
            if (!scanout)
            scanout->handle = create_dumb.handle;
            assert(p_atomic_read(&scanout->refcnt) == 0);
            if (!out_handle)
            /* fill in winsys handle */
   memset(out_handle, 0, sizeof(*out_handle));
   out_handle->type = WINSYS_HANDLE_TYPE_FD;
            err = drmPrimeHandleToFD(ro->kms_fd, create_dumb.handle, O_CLOEXEC,
         if (err < 0) {
      fprintf(stderr, "failed to export dumb buffer: %s\n", strerror(errno));
                     free_dumb:
      /* If an error occured, make sure we reset the scanout object before
   * leaving.
   */
   if (scanout)
            destroy_dumb.handle = create_dumb.handle;
               }
      struct renderonly_scanout *
   renderonly_create_gpu_import_for_resource(struct pipe_resource *rsc,
               {
      struct pipe_screen *screen = rsc->screen;
   struct renderonly_scanout *scanout = NULL;
   bool status;
   uint32_t scanout_handle;
   int fd, err;
   struct winsys_handle handle = {
                  status = screen->resource_get_handle(screen, NULL, rsc, &handle,
         if (!status)
                     simple_mtx_lock(&ro->bo_map_lock);
   err = drmPrimeFDToHandle(ro->kms_fd, fd, &scanout_handle);
            if (err < 0)
            scanout = util_sparse_array_get(&ro->bo_map, scanout_handle);
   if (!scanout)
            if (p_atomic_inc_return(&scanout->refcnt) == 1) {
      scanout->handle = scanout_handle;
            err_unlock:
                  }
   