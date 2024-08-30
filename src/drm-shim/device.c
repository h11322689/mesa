   /*
   * Copyright © 2018 Broadcom
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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      /** @file
   *
   * Implements core GEM support (particularly ioctls) underneath the libc ioctl
   * wrappers, and calls into the driver-specific code as necessary.
   */
      #include <c11/threads.h>
   #include <errno.h>
   #include <linux/memfd.h>
   #include <stdbool.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <sys/ioctl.h>
   #include <sys/mman.h>
   #include <unistd.h>
   #include "drm-uapi/drm.h"
   #include "drm_shim.h"
   #include "util/hash_table.h"
   #include "util/u_atomic.h"
      #define SHIM_MEM_SIZE (4ull * 1024 * 1024 * 1024)
      #ifndef HAVE_MEMFD_CREATE
   #include <sys/syscall.h>
      static inline int
   memfd_create(const char *name, unsigned int flags)
   {
         }
   #endif
      /* Global state for the shim shared between libc, core, and driver. */
   struct shim_device shim_device;
      long shim_page_size;
      static uint32_t
   uint_key_hash(const void *key)
   {
         }
      static bool
   uint_key_compare(const void *a, const void *b)
   {
         }
      /**
   * Called when the first libc shim is called, to initialize GEM simulation
   * state (other than the shims themselves).
   */
   void
   drm_shim_device_init(void)
   {
      shim_device.fd_map = _mesa_hash_table_create(NULL,
                                    shim_device.mem_fd = memfd_create("shim mem", MFD_CLOEXEC);
            ASSERTED int ret = ftruncate(shim_device.mem_fd, SHIM_MEM_SIZE);
            /* The man page for mmap() says
   *
   *    offset must be a multiple of the page size as returned by
   *    sysconf(_SC_PAGE_SIZE).
   *
   * Depending on the configuration of the kernel, this may not be 4096. Get
   * this page size once and use it as the page size throughout, ensuring that
   * are offsets are page-size aligned as required. Otherwise, mmap will fail
   * with EINVAL.
                     util_vma_heap_init(&shim_device.mem_heap, shim_page_size,
               }
      static struct shim_fd *
   drm_shim_file_create(int fd)
   {
               shim_fd->fd = fd;
   p_atomic_set(&shim_fd->refcount, 1);
   mtx_init(&shim_fd->handle_lock, mtx_plain);
   shim_fd->handles = _mesa_hash_table_create(NULL,
                     }
      /**
   * Called when the libc shims have interposed an open or dup of our simulated
   * DRM device.
   */
   void drm_shim_fd_register(int fd, struct shim_fd *shim_fd)
   {
      if (!shim_fd)
         else
               }
      static void handle_delete_fxn(struct hash_entry *entry)
   {
         }
      void drm_shim_fd_unregister(int fd)
   {
      if (fd == -1)
            struct hash_entry *entry =
         if (!entry)
         struct shim_fd *shim_fd = entry->data;
            if (!p_atomic_dec_zero(&shim_fd->refcount))
            _mesa_hash_table_destroy(shim_fd->handles, handle_delete_fxn);
      }
      struct shim_fd *
   drm_shim_fd_lookup(int fd)
   {
      if (fd == -1)
            struct hash_entry *entry =
            if (!entry)
            }
      /* ioctl used by drmGetVersion() */
   static int
   drm_shim_ioctl_version(int fd, unsigned long request, void *arg)
   {
      struct drm_version *args = arg;
   const char *date = "20190320";
            args->version_major = shim_device.version_major;
   args->version_minor = shim_device.version_minor;
            if (args->name)
         if (args->date)
         if (args->desc)
         args->name_len = strlen(shim_device.driver_name);
   args->date_len = strlen(date);
               }
      static int
   drm_shim_ioctl_get_unique(int fd, unsigned long request, void *arg)
   {
               if (gu->unique && shim_device.unique)
                     }
      static int
   drm_shim_ioctl_get_cap(int fd, unsigned long request, void *arg)
   {
               switch (gc->capability) {
   case DRM_CAP_PRIME:
   case DRM_CAP_SYNCOBJ:
   case DRM_CAP_SYNCOBJ_TIMELINE:
      gc->value = 1;
         default:
      fprintf(stderr, "DRM_IOCTL_GET_CAP: unhandled 0x%x\n",
               }
      static int
   drm_shim_ioctl_gem_close(int fd, unsigned long request, void *arg)
   {
      struct shim_fd *shim_fd = drm_shim_fd_lookup(fd);
            if (!c->handle)
            mtx_lock(&shim_fd->handle_lock);
   struct hash_entry *entry =
         if (!entry) {
      mtx_unlock(&shim_fd->handle_lock);
               struct shim_bo *bo = entry->data;
   _mesa_hash_table_remove(shim_fd->handles, entry);
   drm_shim_bo_put(bo);
   mtx_unlock(&shim_fd->handle_lock);
      }
      static int
   drm_shim_ioctl_syncobj_create(int fd, unsigned long request, void *arg)
   {
                           }
      static int
   drm_shim_ioctl_stub(int fd, unsigned long request, void *arg)
   {
         }
      ioctl_fn_t core_ioctls[] = {
      [_IOC_NR(DRM_IOCTL_VERSION)] = drm_shim_ioctl_version,
   [_IOC_NR(DRM_IOCTL_GET_UNIQUE)] = drm_shim_ioctl_get_unique,
   [_IOC_NR(DRM_IOCTL_GET_CAP)] = drm_shim_ioctl_get_cap,
   [_IOC_NR(DRM_IOCTL_GEM_CLOSE)] = drm_shim_ioctl_gem_close,
   [_IOC_NR(DRM_IOCTL_SYNCOBJ_CREATE)] = drm_shim_ioctl_syncobj_create,
   [_IOC_NR(DRM_IOCTL_SYNCOBJ_DESTROY)] = drm_shim_ioctl_stub,
   [_IOC_NR(DRM_IOCTL_SYNCOBJ_HANDLE_TO_FD)] = drm_shim_ioctl_stub,
   [_IOC_NR(DRM_IOCTL_SYNCOBJ_FD_TO_HANDLE)] = drm_shim_ioctl_stub,
   [_IOC_NR(DRM_IOCTL_SYNCOBJ_WAIT)] = drm_shim_ioctl_stub,
      };
      /**
   * Implements the GEM core ioctls, and calls into driver-specific ioctls.
   */
   int
   drm_shim_ioctl(int fd, unsigned long request, void *arg)
   {
      ASSERTED int type = _IOC_TYPE(request);
                     if (nr >= DRM_COMMAND_BASE && nr < DRM_COMMAND_END) {
               if (driver_nr < shim_device.driver_ioctl_count &&
      shim_device.driver_ioctls[driver_nr]) {
         } else {
      if (nr < ARRAY_SIZE(core_ioctls) && core_ioctls[nr]) {
                     if (nr >= DRM_COMMAND_BASE && nr < DRM_COMMAND_END) {
      fprintf(stderr,
            } else {
      fprintf(stderr,
                        }
      int
   drm_shim_bo_init(struct shim_bo *bo, size_t size)
   {
         mtx_lock(&shim_device.mem_lock);
   bo->mem_addr = util_vma_heap_alloc(&shim_device.mem_heap, size, shim_page_size);
            if (!bo->mem_addr)
                        }
      struct shim_bo *
   drm_shim_bo_lookup(struct shim_fd *shim_fd, int handle)
   {
      if (!handle)
            mtx_lock(&shim_fd->handle_lock);
   struct hash_entry *entry =
         struct shim_bo *bo = entry ? entry->data : NULL;
            if (bo)
               }
      void
   drm_shim_bo_get(struct shim_bo *bo)
   {
         }
      void
   drm_shim_bo_put(struct shim_bo *bo)
   {
      if (p_atomic_dec_return(&bo->refcount) == 0)
            if (shim_device.driver_bo_free)
            mtx_lock(&shim_device.mem_lock);
   util_vma_heap_free(&shim_device.mem_heap, bo->mem_addr, bo->size);
   mtx_unlock(&shim_device.mem_lock);
      }
      int
   drm_shim_bo_get_handle(struct shim_fd *shim_fd, struct shim_bo *bo)
   {
      /* We should probably have some real datastructure for finding the free
   * number.
   */
   mtx_lock(&shim_fd->handle_lock);
   for (int new_handle = 1; ; new_handle++) {
      void *key = (void *)(uintptr_t)new_handle;
   if (!_mesa_hash_table_search(shim_fd->handles, key)) {
      drm_shim_bo_get(bo);
   _mesa_hash_table_insert(shim_fd->handles, key, bo);
   mtx_unlock(&shim_fd->handle_lock);
         }
               }
      /* Creates an mmap offset for the BO in the DRM fd.
   */
   uint64_t
   drm_shim_bo_get_mmap_offset(struct shim_fd *shim_fd, struct shim_bo *bo)
   {
      mtx_lock(&shim_device.mem_lock);
   _mesa_hash_table_u64_insert(shim_device.offset_map, bo->mem_addr, bo);
            /* reuse the buffer address as the mmap offset: */
      }
      /* For mmap() on the DRM fd, look up the BO from the "offset" and map the BO's
   * fd.
   */
   void *
   drm_shim_mmap(struct shim_fd *shim_fd, size_t length, int prot, int flags,
         {
      mtx_lock(&shim_device.mem_lock);
   struct shim_bo *bo = _mesa_hash_table_u64_search(shim_device.offset_map, offset);
            if (!bo)
            if (length > bo->size)
            /* The offset we pass to mmap must be aligned to the page size */
               }
