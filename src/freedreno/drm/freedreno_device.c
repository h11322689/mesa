   /*
   * Copyright (C) 2012-2018 Rob Clark <robclark@freedesktop.org>
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
   *    Rob Clark <robclark@freedesktop.org>
   */
      #include <unistd.h>
   #include <sys/stat.h>
   #include <sys/types.h>
      #include "util/os_file.h"
      #include "freedreno_drmif.h"
   #include "freedreno_priv.h"
      struct fd_device *msm_device_new(int fd, drmVersionPtr version);
   #ifdef HAVE_FREEDRENO_VIRTIO
   struct fd_device *virtio_device_new(int fd, drmVersionPtr version);
   #endif
      struct fd_device *
   fd_device_new(int fd)
   {
      struct fd_device *dev = NULL;
   drmVersionPtr version;
            /* figure out if we are kgsl or msm drm driver: */
   version = drmGetVersion(fd);
   if (!version) {
      ERROR_MSG("cannot get version: %s", strerror(errno));
               if (!strcmp(version->name, "msm")) {
      DEBUG_MSG("msm DRM device");
   if (version->version_major != 1) {
      ERROR_MSG("unsupported version: %u.%u.%u", version->version_major,
                     #ifdef HAVE_FREEDRENO_VIRTIO
      } else if (!strcmp(version->name, "virtio_gpu")) {
      DEBUG_MSG("virtio_gpu DRM device");
   dev = virtio_device_new(fd, version);
   /* Only devices that support a hypervisor are a6xx+, so avoid the
   * extra guest<->host round trips associated with pipe creation:
   */
   #endif
   #if HAVE_FREEDRENO_KGSL
      } else if (!strcmp(version->name, "kgsl")) {
      DEBUG_MSG("kgsl DRM device");
   #endif
               if (!dev) {
      INFO_MSG("unsupported device: %s", version->name);
            out:
               if (!dev)
            p_atomic_set(&dev->refcnt, 1);
   dev->fd = fd;
   dev->handle_table =
         dev->name_table =
         fd_bo_cache_init(&dev->bo_cache, false, "bo");
            list_inithead(&dev->deferred_submits);
   simple_mtx_init(&dev->submit_lock, mtx_plain);
            if (!use_heap) {
               if (!pipe)
            /* Userspace fences don't appear to be reliable enough (missing some
   * cache flushes?) on older gens, so limit sub-alloc heaps to a6xx+
   * for now:
   */
                        if (use_heap) {
      dev->ring_heap = fd_bo_heap_new(dev, RING_FLAGS);
                     fail:
      fd_device_del(dev);
      }
      /* like fd_device_new() but creates it's own private dup() of the fd
   * which is close()d when the device is finalized.
   */
   struct fd_device *
   fd_device_new_dup(int fd)
   {
      int dup_fd = os_dupfd_cloexec(fd);
   struct fd_device *dev = fd_device_new(dup_fd);
   if (dev)
         else
            }
      /* Convenience helper to open the drm device and return new fd_device:
   */
   struct fd_device *
   fd_device_open(void)
   {
                  #ifdef HAVE_FREEDRENO_VIRTIO
      if (fd < 0)
      #endif
      if (fd < 0)
               }
      struct fd_device *
   fd_device_ref(struct fd_device *dev)
   {
      ref(&dev->refcnt);
      }
      void
   fd_device_purge(struct fd_device *dev)
   {
      fd_bo_cache_cleanup(&dev->bo_cache, 0);
      }
      void
   fd_device_del(struct fd_device *dev)
   {
      if (!unref(&dev->refcnt))
            assert(list_is_empty(&dev->deferred_submits));
            if (dev->suballoc_bo)
            if (dev->ring_heap)
            if (dev->default_heap)
            fd_bo_cache_cleanup(&dev->bo_cache, 0);
            /* Needs to be after bo cache cleanup in case backend has a
   * util_vma_heap that it destroys:
   */
            _mesa_hash_table_destroy(dev->handle_table, NULL);
            if (fd_device_threaded_submit(dev))
            if (dev->closefd)
               }
      int
   fd_device_fd(struct fd_device *dev)
   {
         }
      enum fd_version
   fd_device_version(struct fd_device *dev)
   {
         }
      DEBUG_GET_ONCE_BOOL_OPTION(libgl, "LIBGL_DEBUG", false)
      bool
   fd_dbg(void)
   {
         }
      bool
   fd_has_syncobj(struct fd_device *dev)
   {
      uint64_t value;
   if (drmGetCap(dev->fd, DRM_CAP_SYNCOBJ, &value))
            }
