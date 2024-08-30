   /*
   * Copyright © 2015 Intel Corporation
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
      #include <stdlib.h>
   #include <stdio.h>
   #include <string.h>
   #include <stdint.h>
   #include <stdbool.h>
   #include <signal.h>
   #include <stdarg.h>
   #include <fcntl.h>
   #include <sys/types.h>
   #include <sys/sysmacros.h>
   #include <sys/stat.h>
   #include <sys/ioctl.h>
   #include <unistd.h>
   #include <errno.h>
   #include <sys/mman.h>
   #include <dlfcn.h>
   #include "drm-uapi/i915_drm.h"
   #include <inttypes.h>
      #include "intel_aub.h"
   #include "aub_write.h"
      #include "c11/threads.h"
   #include "dev/intel_debug.h"
   #include "dev/intel_device_info.h"
   #include "common/intel_gem.h"
   #include "util/macros.h"
   #include "util/u_math.h"
      static int close_init_helper(int fd);
   static int ioctl_init_helper(int fd, unsigned long request, ...);
   static int munmap_init_helper(void *addr, size_t length);
      static int (*libc_close)(int fd) = close_init_helper;
   static int (*libc_ioctl)(int fd, unsigned long request, ...) = ioctl_init_helper;
   static int (*libc_munmap)(void *addr, size_t length) = munmap_init_helper;
      static int drm_fd = -1;
   static char *output_filename = NULL;
   static FILE *output_file = NULL;
   static int verbose = 0;
   static bool device_override = false;
   static bool capture_only = false;
   static int64_t frame_id = -1;
   static bool capture_finished = false;
      #define MAX_FD_COUNT 64
   #define MAX_BO_COUNT 64 * 1024
      struct bo {
      uint32_t size;
   uint64_t offset;
   void *map;
   /* Whether the buffer has been positioned in the GTT already. */
   bool gtt_mapped : 1;
   /* Tracks userspace mmapping of the buffer */
   bool user_mapped : 1;
   /* Using the i915-gem mmapping ioctl & execbuffer ioctl, track whether a
   * buffer has been updated.
   */
      };
      static struct bo *bos;
      #define DRM_MAJOR 226
      /* We set bit 0 in the map pointer for userptr BOs so we know not to
   * munmap them on DRM_IOCTL_GEM_CLOSE.
   */
   #define USERPTR_FLAG 1
   #define IS_USERPTR(p) ((uintptr_t) (p) & USERPTR_FLAG)
   #define GET_PTR(p) ( (void *) ((uintptr_t) p & ~(uintptr_t) 1) )
      #define fail_if(cond, ...) _fail_if(cond, "intel_dump_gpu", __VA_ARGS__)
      static struct bo *
   get_bo(unsigned fd, uint32_t handle)
   {
               fail_if(handle >= MAX_BO_COUNT, "bo handle too large\n");
   fail_if(fd >= MAX_FD_COUNT, "bo fd too large\n");
               }
      static struct intel_device_info devinfo = {0};
   static int device = 0;
   static struct aub_file aub_file;
      static void
   ensure_device_info(int fd)
   {
      /* We can't do this at open time as we're not yet authenticated. */
   if (device == 0) {
      fail_if(!intel_get_device_info_from_fd(fd, &devinfo),
            } else if (devinfo.ver == 0) {
      fail_if(!intel_get_device_info_from_pci_id(device, &devinfo),
         }
      static void *
   relocate_bo(int fd, struct bo *bo, const struct drm_i915_gem_execbuffer2 *execbuffer2,
         {
      const struct drm_i915_gem_exec_object2 *exec_objects =
         const struct drm_i915_gem_relocation_entry *relocs =
         void *relocated;
            relocated = malloc(bo->size);
   fail_if(relocated == NULL, "out of memory\n");
   memcpy(relocated, GET_PTR(bo->map), bo->size);
   for (size_t i = 0; i < obj->relocation_count; i++) {
               if (execbuffer2->flags & I915_EXEC_HANDLE_LUT)
         else
            aub_write_reloc(&devinfo, ((char *)relocated) + relocs[i].offset,
                  }
      static int
   gem_ioctl(int fd, unsigned long request, void *argp)
   {
               do {
                     }
      static void *
   gem_mmap(int fd, uint32_t handle, uint64_t offset, uint64_t size)
   {
      struct drm_i915_gem_mmap mmap = {
      .handle = handle,
   .offset = offset,
               if (gem_ioctl(fd, DRM_IOCTL_I915_GEM_MMAP, &mmap) == -1)
               }
      static enum intel_engine_class
   engine_class_from_ring_flag(uint32_t ring_flag)
   {
      switch (ring_flag) {
   case I915_EXEC_DEFAULT:
   case I915_EXEC_RENDER:
         case I915_EXEC_BSD:
         case I915_EXEC_BLT:
         case I915_EXEC_VEBOX:
         default:
            }
      static void
   dump_execbuffer2(int fd, struct drm_i915_gem_execbuffer2 *execbuffer2)
   {
      struct drm_i915_gem_exec_object2 *exec_objects =
         uint32_t ring_flag = execbuffer2->flags & I915_EXEC_RING_MASK;
   uint32_t offset;
   struct drm_i915_gem_exec_object2 *obj;
   struct bo *bo, *batch_bo;
   int batch_index;
                     if (capture_finished)
            if (!aub_file.file) {
      aub_file_init(&aub_file, output_file,
                        if (verbose)
      printf("[running, output file %s, chipset id 0x%04x, gen %d]\n",
            if (aub_use_execlists(&aub_file))
         else
            for (uint32_t i = 0; i < execbuffer2->buffer_count; i++) {
      obj = &exec_objects[i];
            /* If bo->size == 0, this means they passed us an invalid
   * buffer.  The kernel will reject it and so should we.
   */
   if (bo->size == 0) {
      if (verbose)
                     if (obj->flags & EXEC_OBJECT_PINNED) {
      if (bo->offset != obj->offset)
            } else {
      if (obj->alignment != 0)
         bo->offset = offset;
               if (bo->map == NULL && bo->size > 0)
                     uint64_t current_frame_id = 0;
   if (frame_id >= 0) {
      for (uint32_t i = 0; i < execbuffer2->buffer_count; i++) {
                     /* Check against frame_id requirements. */
   if (memcmp(bo->map, intel_debug_identifier(),
            const struct intel_debug_block_frame *frame_desc =
                  current_frame_id = frame_desc ? frame_desc->frame_id : 0;
                     if (verbose)
      printf("Dumping execbuffer2 (frame_id=%"PRIu64", buffers=%u):\n",
         /* Check whether we can stop right now. */
   if (frame_id >= 0) {
      if (current_frame_id < frame_id)
            if (current_frame_id > frame_id) {
      aub_file_finish(&aub_file);
   capture_finished = true;
                     /* Map buffers into the PPGTT. */
   for (uint32_t i = 0; i < execbuffer2->buffer_count; i++) {
      obj = &exec_objects[i];
            if (verbose) {
      printf("BO #%d (%dB) @ 0x%" PRIx64 "\n",
               if (aub_use_execlists(&aub_file) && !bo->gtt_mapped) {
      aub_map_ppgtt(&aub_file, bo->offset, bo->size);
                  /* Write the buffer content into the Aub. */
   batch_index = (execbuffer2->flags & I915_EXEC_BATCH_FIRST) ? 0 :
         batch_bo = get_bo(fd, exec_objects[batch_index].handle);
   for (uint32_t i = 0; i < execbuffer2->buffer_count; i++) {
      obj = &exec_objects[i];
            if (obj->relocation_count > 0)
         else
                     if (write && bo->dirty) {
      if (bo == batch_bo) {
      aub_write_trace_block(&aub_file, AUB_TRACE_TYPE_BATCH,
      } else {
      aub_write_trace_block(&aub_file, AUB_TRACE_TYPE_NOTYPE,
               if (!bo->user_mapped)
               if (data != bo->map)
                        aub_write_exec(&aub_file, ctx_id,
                  if (device_override &&
      (execbuffer2->flags & I915_EXEC_FENCE_ARRAY) != 0) {
   struct drm_i915_gem_exec_fence *fences =
         for (uint32_t i = 0; i < execbuffer2->num_cliprects; i++) {
      if ((fences[i].flags & I915_EXEC_FENCE_SIGNAL) != 0) {
      struct drm_syncobj_array arg = {
      .handles = (uintptr_t)&fences[i].handle,
   .count_handles = 1,
      };
               }
      static void
   add_new_bo(unsigned fd, int handle, uint64_t size, void *map)
   {
               fail_if(handle >= MAX_BO_COUNT, "bo handle out of range\n");
   fail_if(fd >= MAX_FD_COUNT, "bo fd out of range\n");
            bo->size = size;
   bo->map = map;
   bo->user_mapped = false;
      }
      static void
   remove_bo(int fd, int handle)
   {
               if (bo->map && !IS_USERPTR(bo->map))
            }
      __attribute__ ((visibility ("default"))) int
   close(int fd)
   {
      if (fd == drm_fd)
               }
      static int
   get_pci_id(int fd, int *pci_id)
   {
      if (device_override) {
      *pci_id = device;
                  }
      static void
   maybe_init(int fd)
   {
      static bool initialized = false;
   FILE *config;
            if (initialized)
                     const char *config_path = getenv("INTEL_DUMP_GPU_CONFIG");
            config = fopen(config_path, "r");
            while (fscanf(config, "%m[^=]=%m[^\n]\n", &key, &value) != EOF) {
      if (!strcmp(key, "verbose")) {
      if (!strcmp(value, "1")) {
         } else if (!strcmp(value, "2")) {
            } else if (!strcmp(key, "device")) {
      fail_if(device != 0, "Device/Platform override specified multiple times.\n");
   fail_if(sscanf(value, "%i", &device) != 1,
         "failed to parse device id '%s'\n",
      } else if (!strcmp(key, "platform")) {
      fail_if(device != 0, "Device/Platform override specified multiple times.\n");
   device = intel_device_name_to_pci_device_id(value);
   fail_if(device == -1, "Unknown platform '%s'\n", value);
      } else if (!strcmp(key, "file")) {
      free(output_filename);
   if (output_file)
         output_filename = strdup(value);
   output_file = fopen(output_filename, "w+");
   fail_if(output_file == NULL,
            } else if (!strcmp(key, "capture_only")) {
         } else if (!strcmp(key, "frame")) {
         } else {
                  free(key);
      }
            bos = calloc(MAX_FD_COUNT * MAX_BO_COUNT, sizeof(bos[0]));
            ASSERTED int ret = get_pci_id(fd, &device);
            aub_file_init(&aub_file, output_file,
                        if (verbose)
      printf("[running, output file %s, chipset id 0x%04x, gen %d]\n",
   }
      static int
   intercept_ioctl(int fd, unsigned long request, ...)
   {
      va_list args;
   void *argp;
   int ret;
            va_start(args, request);
   argp = va_arg(args, void *);
            if (_IOC_TYPE(request) == DRM_IOCTL_BASE &&
      drm_fd != fd && fstat(fd, &buf) == 0 &&
   (buf.st_mode & S_IFMT) == S_IFCHR && major(buf.st_rdev) == DRM_MAJOR) {
   drm_fd = fd;
   if (verbose)
               if (fd == drm_fd) {
               switch (request) {
   case DRM_IOCTL_SYNCOBJ_WAIT:
   case DRM_IOCTL_I915_GEM_WAIT: {
      if (device_override)
                     case DRM_IOCTL_I915_GET_RESET_STATS: {
                        stats->reset_count = 0;
   stats->batch_active = 0;
   stats->batch_pending = 0;
      }
               case DRM_IOCTL_I915_GETPARAM: {
                                       if (device_override) {
      switch (getparam->param) {
   case I915_PARAM_CS_TIMESTAMP_FREQUENCY:
                  case I915_PARAM_HAS_WAIT_TIMEOUT:
   case I915_PARAM_HAS_EXECBUF2:
   case I915_PARAM_MMAP_VERSION:
   case I915_PARAM_HAS_EXEC_ASYNC:
   case I915_PARAM_HAS_EXEC_FENCE:
   case I915_PARAM_HAS_EXEC_FENCE_ARRAY:
                  case I915_PARAM_HAS_EXEC_SOFTPIN:
                  default:
                                 case DRM_IOCTL_I915_GEM_CONTEXT_GETPARAM: {
                        if (device_override) {
      switch (getparam->param) {
   case I915_CONTEXT_PARAM_GTT_SIZE:
      if (devinfo.platform == INTEL_PLATFORM_EHL)
         else if (devinfo.ver >= 8 && devinfo.platform != INTEL_PLATFORM_CHV)
                           default:
                                 case DRM_IOCTL_I915_GEM_EXECBUFFER: {
      static bool once;
   if (!once) {
      fprintf(stderr,
            }
               case DRM_IOCTL_I915_GEM_EXECBUFFER2:
   case DRM_IOCTL_I915_GEM_EXECBUFFER2_WR: {
      dump_execbuffer2(fd, argp);
                              case DRM_IOCTL_I915_GEM_CONTEXT_CREATE: {
      uint32_t *ctx_id = NULL;
   struct drm_i915_gem_context_create *create = argp;
   ret = 0;
   if (!device_override) {
      ret = libc_ioctl(fd, request, argp);
                                          case DRM_IOCTL_I915_GEM_CONTEXT_CREATE_EXT: {
      uint32_t *ctx_id = NULL;
   struct drm_i915_gem_context_create_ext *create = argp;
   ret = 0;
   if (!device_override) {
      ret = libc_ioctl(fd, request, argp);
                                          case DRM_IOCTL_I915_GEM_CREATE: {
               ret = libc_ioctl(fd, request, argp);
                              case DRM_IOCTL_I915_GEM_CREATE_EXT: {
               ret = libc_ioctl(fd, request, argp);
                              case DRM_IOCTL_I915_GEM_USERPTR: {
               ret = libc_ioctl(fd, request, argp);
   if (ret == 0)
                              case DRM_IOCTL_GEM_CLOSE: {
                                    case DRM_IOCTL_GEM_OPEN: {
               ret = libc_ioctl(fd, request, argp);
                              case DRM_IOCTL_PRIME_FD_TO_HANDLE: {
               ret = libc_ioctl(fd, request, argp);
                     size = lseek(prime->fd, 0, SEEK_END);
                                    case DRM_IOCTL_I915_GEM_MMAP: {
      ret = libc_ioctl(fd, request, argp);
   if (ret == 0) {
      struct drm_i915_gem_mmap *mmap = argp;
   struct bo *bo = get_bo(fd, mmap->handle);
   bo->user_mapped = true;
      }
               case DRM_IOCTL_I915_GEM_MMAP_OFFSET: {
      ret = libc_ioctl(fd, request, argp);
   if (ret == 0) {
      struct drm_i915_gem_mmap_offset *mmap = argp;
   struct bo *bo = get_bo(fd, mmap->handle);
   bo->user_mapped = true;
      }
               default:
            } else {
            }
      __attribute__ ((visibility ("default"))) int
   ioctl(int fd, unsigned long request, ...)
   {
      static thread_local bool entered = false;
   va_list args;
   void *argp;
            va_start(args, request);
   argp = va_arg(args, void *);
            /* Some of the functions called by intercept_ioctl call ioctls of their
   * own. These need to go to the libc ioctl instead of being passed back to
   * intercept_ioctl to avoid a stack overflow. */
   if (entered) {
         } else {
      entered = true;
   ret = intercept_ioctl(fd, request, argp);
   entered = false;
         }
      static void
   init(void)
   {
      libc_close = dlsym(RTLD_NEXT, "close");
   libc_ioctl = dlsym(RTLD_NEXT, "ioctl");
   libc_munmap = dlsym(RTLD_NEXT, "munmap");
   fail_if(libc_close == NULL || libc_ioctl == NULL,
      }
      static int
   close_init_helper(int fd)
   {
      init();
      }
      static int
   ioctl_init_helper(int fd, unsigned long request, ...)
   {
      va_list args;
            va_start(args, request);
   argp = va_arg(args, void *);
            init();
      }
      static int
   munmap_init_helper(void *addr, size_t length)
   {
      init();
   for (uint32_t i = 0; i < MAX_FD_COUNT * MAX_BO_COUNT; i++) {
      struct bo *bo = &bos[i];
   if (bo->map == addr) {
      bo->user_mapped = false;
         }
      }
      static void __attribute__ ((destructor))
   fini(void)
   {
      if (devinfo.ver != 0) {
      free(output_filename);
   if (!capture_finished)
               }
