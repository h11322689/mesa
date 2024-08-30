   /*
   * Copyright © 2014 Broadcom
   * Copyright © 208 Alyssa Rosenzweig
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include <errno.h>
   #include <fcntl.h>
   #include <unistd.h>
      #include "util/format/u_format.h"
   #include "util/os_file.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/u_screen.h"
      #include "drm-uapi/drm.h"
   #include "panfrost/pan_public.h"
   #include "renderonly/renderonly.h"
   #include "panfrost_drm_public.h"
   #include "xf86drm.h"
      struct renderonly_scanout *
   panfrost_create_kms_dumb_buffer_for_resource(struct pipe_resource *rsc,
               {
      /* Find the smallest width alignment that gives us a 64byte aligned stride */
   unsigned blk_sz = util_format_get_blocksize(rsc->format);
                     unsigned align_w = 1;
   for (unsigned i = 1; i <= blk_sz; i++) {
      if (!((64 * i) % blk_sz)) {
      align_w = (64 * i) / blk_sz;
                  struct drm_mode_create_dumb create_dumb = {
      .width = ALIGN_NPOT(rsc->width0, align_w),
   .height = rsc->height0,
      };
            /* create dumb buffer at scanout GPU */
   int err = drmIoctl(ro->kms_fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_dumb);
   if (err < 0) {
      fprintf(stderr, "DRM_IOCTL_MODE_CREATE_DUMB failed: %s\n",
                     if (create_dumb.pitch % 64)
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
      struct pipe_screen *
   panfrost_drm_screen_create(int fd)
   {
      return u_pipe_screen_lookup_or_create(os_dupfd_cloexec(fd), NULL, NULL,
      }
      struct pipe_screen *
   panfrost_drm_screen_create_renderonly(int fd, struct renderonly *ro,
         {
      return u_pipe_screen_lookup_or_create(os_dupfd_cloexec(fd), config, ro,
      }
