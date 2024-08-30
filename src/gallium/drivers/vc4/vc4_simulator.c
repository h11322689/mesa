   /*
   * Copyright © 2014 Broadcom
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
      /**
   * @file vc4_simulator.c
   *
   * Implements VC4 simulation on top of a non-VC4 GEM fd.
   *
   * This file's goal is to emulate the VC4 ioctls' behavior in the kernel on
   * top of the simpenrose software simulator.  Generally, VC4 driver BOs have a
   * GEM-side copy of their contents and a simulator-side memory area that the
   * GEM contents get copied into during simulation.  Once simulation is done,
   * the simulator's data is copied back out to the GEM BOs, so that rendering
   * appears on the screen as if actual hardware rendering had been done.
   *
   * One of the limitations of this code is that we shouldn't really need a
   * GEM-side BO for non-window-system BOs.  However, do we need unique BO
   * handles for each of our GEM bos so that this file can look up its state
   * from the handle passed in at submit ioctl time (also, a couple of places
   * outside of this file still call ioctls directly on the fd).
   *
   * Another limitation is that BO import doesn't work unless the underlying
   * window system's BO size matches what VC4 is going to use, which of course
   * doesn't work out in practice.  This means that for now, only DRI3 (VC4
   * makes the winsys BOs) is supported, not DRI2 (window system makes the winys
   * BOs).
   */
      #ifdef USE_VC4_SIMULATOR
      #include <sys/mman.h>
   #include "xf86drm.h"
   #include "util/u_memory.h"
   #include "util/u_mm.h"
   #include "util/ralloc.h"
   #include "util/simple_mtx.h"
      #include "vc4_screen.h"
   #include "vc4_cl_dump.h"
   #include "vc4_context.h"
   #include "kernel/vc4_drv.h"
   #include "vc4_simulator_validate.h"
   #include "simpenrose/simpenrose.h"
      #include "drm-uapi/amdgpu_drm.h"
   #include "drm-uapi/i915_drm.h"
      /** Global (across GEM fds) state for the simulator */
   static struct vc4_simulator_state {
                  void *mem;
   ssize_t mem_size;
   struct mem_block *heap;
            /** Mapping from GEM handle to struct vc4_simulator_bo * */
            } sim_state = {
         };
      enum gem_type {
         GEM_I915,
   GEM_AMDGPU,
   };
      /** Per-GEM-fd state for the simulator. */
   struct vc4_simulator_file {
                  /* This is weird -- we make a "vc4_device" per file, even though on
      * the kernel side this is a global.  We do this so that kernel code
   * calling us for BO allocation can get to our screen.
               /** Mapping from GEM handle to struct vc4_simulator_bo * */
            /** for specific gpus, use their create ioctl. Otherwise use dumb bo. */
   };
      /** Wrapper for drm_vc4_bo tracking the simulator-specific state. */
   struct vc4_simulator_bo {
         struct drm_vc4_bo base;
            /** Area for this BO within sim_state->mem */
                     /* Mapping of the underlying GEM object that we copy in/out of
      * simulator memory.
      };
      static void *
   int_to_key(int key)
   {
         }
      static struct vc4_simulator_file *
   vc4_get_simulator_file_for_fd(int fd)
   {
         struct hash_entry *entry = _mesa_hash_table_search(sim_state.fd_map,
         }
      /* A marker placed just after each BO, then checked after rendering to make
   * sure it's still there.
   */
   #define BO_SENTINEL		0xfedcba98
      #define PAGE_ALIGN2		12
      static int
   vc4_gem_mmap(int fd, uint32_t handle, uint64_t *offset)
   {
         struct vc4_simulator_file *file = vc4_get_simulator_file_for_fd(fd);
                     switch (file->gem_type) {
   case GEM_I915:
   {
            struct drm_i915_gem_mmap_gtt map = {
         };
   ret = drmIoctl(fd, DRM_IOCTL_I915_GEM_MMAP_GTT, &map);
      }
   case GEM_AMDGPU:
   {
            union drm_amdgpu_gem_mmap map = { 0 };
   map.in.handle = handle;
   ret = drmIoctl(fd, DRM_IOCTL_AMDGPU_GEM_MMAP, &map);
      }
   default:
   {
            struct drm_mode_map_dumb map = {
         };
      }
            }
      static int
   vc4_gem_create(int fd, uint64_t size, uint32_t *handle)
   {
         struct vc4_simulator_file *file = vc4_get_simulator_file_for_fd(fd);
                     switch (file->gem_type) {
   case GEM_I915:
   {
            struct drm_i915_gem_create create = {
         };
   ret = drmIoctl(fd, DRM_IOCTL_I915_GEM_CREATE, &create);
      }
   case GEM_AMDGPU:
   {
            union drm_amdgpu_gem_create create = { 0 };
   create.in.bo_size = size;
   ret = drmIoctl(fd, DRM_IOCTL_AMDGPU_GEM_CREATE, &create);
      }
   default:
   {
            struct drm_mode_create_dumb create = {
            .width = 128,
      };
   ret = drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create);
      }
            }
      /**
   * Allocates space in simulator memory and returns a tracking struct for it
   * that also contains the drm_gem_cma_object struct.
   */
   static struct vc4_simulator_bo *
   vc4_create_simulator_bo(int fd, int handle, unsigned size)
   {
         struct vc4_simulator_file *file = vc4_get_simulator_file_for_fd(fd);
   struct vc4_simulator_bo *sim_bo = rzalloc(file,
         struct drm_vc4_bo *bo = &sim_bo->base;
   struct drm_gem_cma_object *obj = &bo->base;
            sim_bo->file = file;
            /* Allocate space for the buffer in simulator memory. */
   simple_mtx_lock(&sim_state.mutex);
   sim_bo->block = u_mmAllocMem(sim_state.heap, size + 4, PAGE_ALIGN2, 0);
   simple_mtx_unlock(&sim_state.mutex);
            obj->base.size = size;
   obj->base.dev = &file->dev;
   obj->vaddr = sim_state.mem + sim_bo->block->ofs;
                     /* A handle of 0 is used for vc4_gem.c internal allocations that
      * don't need to go in the lookup table.
      if (handle != 0) {
                                                      if (ret) {
            fprintf(stderr, "Failed to get MMAP offset: %d\n",
      }
   sim_bo->gem_vaddr = mmap(NULL, obj->base.size,
               if (sim_bo->gem_vaddr == MAP_FAILED) {
            fprintf(stderr, "mmap of bo %d (offset 0x%016llx, size %d) failed\n",
            }
      static void
   vc4_free_simulator_bo(struct vc4_simulator_bo *sim_bo)
   {
         struct vc4_simulator_file *sim_file = sim_bo->file;
   struct drm_vc4_bo *bo = &sim_bo->base;
            if (bo->validated_shader) {
                        if (sim_bo->gem_vaddr)
            simple_mtx_lock(&sim_state.mutex);
   u_mmFreeMem(sim_bo->block);
   if (sim_bo->handle) {
               }
   simple_mtx_unlock(&sim_state.mutex);
   }
      static struct vc4_simulator_bo *
   vc4_get_simulator_bo(struct vc4_simulator_file *file, int gem_handle)
   {
         simple_mtx_lock(&sim_state.mutex);
   struct hash_entry *entry =
                  }
      struct drm_gem_cma_object *
   drm_gem_cma_create(struct drm_device *dev, size_t size)
   {
         struct vc4_screen *screen = dev->screen;
   struct vc4_simulator_bo *sim_bo = vc4_create_simulator_bo(screen->fd,
         }
      static int
   vc4_simulator_pin_bos(struct vc4_simulator_file *file,
         {
         struct drm_vc4_submit_cl *args = exec->args;
            exec->bo_count = args->bo_handle_count;
   exec->bo = calloc(exec->bo_count, sizeof(void *));
   for (int i = 0; i < exec->bo_count; i++) {
            struct vc4_simulator_bo *sim_bo =
                              }
   }
      static int
   vc4_simulator_unpin_bos(struct vc4_exec_info *exec)
   {
         for (int i = 0; i < exec->bo_count; i++) {
            struct drm_gem_cma_object *obj = exec->bo[i];
                        assert(*(uint32_t *)(obj->vaddr +
                              }
      static void
   vc4_dump_to_file(struct vc4_exec_info *exec)
   {
         static int dumpno = 0;
   struct drm_vc4_get_hang_state *state;
   struct drm_vc4_get_hang_state_bo *bo_state;
            if (!VC4_DBG(DUMP))
                     int unref_count = 0;
   list_for_each_entry_safe(struct drm_vc4_bo, bo, &exec->unref_list,
                        /* Add one more for the overflow area that isn't wrapped in a BO. */
   state->bo_count = exec->bo_count + unref_count + 1;
            char *filename = NULL;
   asprintf(&filename, "vc4-dri-%d.dump", dumpno++);
   FILE *f = fopen(filename, "w+");
   if (!f) {
            fprintf(stderr, "Couldn't open %s: %s", filename,
                        state->ct0ca = exec->ct0ca;
   state->ct0ea = exec->ct0ea;
   state->ct1ca = exec->ct1ca;
   state->ct1ea = exec->ct1ea;
   state->start_bin = exec->ct0ca;
   state->start_render = exec->ct1ca;
            int i;
   for (i = 0; i < exec->bo_count; i++) {
            struct drm_gem_cma_object *cma_bo = exec->bo[i];
   bo_state[i].handle = i; /* Not used by the parser. */
               list_for_each_entry_safe(struct drm_vc4_bo, bo, &exec->unref_list,
                  struct drm_gem_cma_object *cma_bo = &bo->base;
   bo_state[i].handle = 0;
   bo_state[i].paddr = cma_bo->paddr;
               /* Add the static overflow memory area. */
   bo_state[i].handle = exec->bo_count;
   bo_state[i].paddr = sim_state.overflow->ofs;
   bo_state[i].size = sim_state.overflow->size;
                     for (int i = 0; i < exec->bo_count; i++) {
                        list_for_each_entry_safe(struct drm_vc4_bo, bo, &exec->unref_list,
                              void *overflow = calloc(1, sim_state.overflow->size);
   fwrite(overflow, 1, sim_state.overflow->size, f);
            free(state);
   free(bo_state);
   }
      static int
   vc4_simulator_submit_cl_ioctl(int fd, struct drm_vc4_submit_cl *args)
   {
         struct vc4_simulator_file *file = vc4_get_simulator_file_for_fd(fd);
   struct vc4_exec_info exec;
   struct drm_device *dev = &file->dev;
            memset(&exec, 0, sizeof(exec));
                     ret = vc4_simulator_pin_bos(file, &exec);
   if (ret)
            ret = vc4_cl_validate(dev, &exec);
   if (ret)
            if (VC4_DBG(CL)) {
            fprintf(stderr, "RCL:\n");
                        if (exec.ct0ca != exec.ct0ea) {
            int bfc = simpenrose_do_binning(exec.ct0ca, exec.ct0ea);
   if (bfc != 1) {
            fprintf(stderr, "Binning returned %d flushes, should be 1.\n",
         fprintf(stderr, "Relocated binning command list:\n");
   vc4_dump_cl(sim_state.mem + exec.ct0ca,
   }
   int rfc = simpenrose_do_rendering(exec.ct1ca, exec.ct1ea);
   if (rfc != 1) {
            fprintf(stderr, "Rendering returned %d frames, should be 1.\n",
         fprintf(stderr, "Relocated render command list:\n");
   vc4_dump_cl(sim_state.mem + exec.ct1ca,
               ret = vc4_simulator_unpin_bos(&exec);
   if (ret)
            list_for_each_entry_safe(struct drm_vc4_bo, bo, &exec.unref_list,
               list_del(&bo->unref_head);
                  assert(*(uint32_t *)(obj->vaddr + obj->base.size) ==
               }
      /**
   * Do fixups after a BO has been opened from a handle.
   *
   * This could be done at DRM_IOCTL_GEM_OPEN/DRM_IOCTL_GEM_PRIME_FD_TO_HANDLE
   * time, but we're still using drmPrimeFDToHandle() so we have this helper to
   * be called afterward instead.
   */
   void vc4_simulator_open_from_handle(int fd, int handle, uint32_t size)
   {
         }
      /**
   * Simulated ioctl(fd, DRM_VC4_CREATE_BO) implementation.
   *
   * Making a VC4 BO is just a matter of making a corresponding BO on the host.
   */
   static int
   vc4_simulator_create_bo_ioctl(int fd, struct drm_vc4_create_bo *args)
   {
                           }
      /**
   * Simulated ioctl(fd, DRM_VC4_CREATE_SHADER_BO) implementation.
   *
   * In simulation we defer shader validation until exec time.  Just make a host
   * BO and memcpy the contents in.
   */
   static int
   vc4_simulator_create_shader_bo_ioctl(int fd,
         {
                  if (ret)
            struct vc4_simulator_bo *sim_bo =
         struct drm_vc4_bo *drm_bo = &sim_bo->base;
            /* Copy into the simulator's BO for validation. */
            /* Copy into the GEM BO to prevent the simulator_pin_bos() from
      * smashing it.
               drm_bo->validated_shader = vc4_validate_shader(obj);
   if (!drm_bo->validated_shader)
            }
      /**
   * Simulated ioctl(fd, DRM_VC4_MMAP_BO) implementation.
   *
   * We just pass this straight through to dumb mmap.
   */
   static int
   vc4_simulator_mmap_bo_ioctl(int fd, struct drm_vc4_mmap_bo *args)
   {
         uint64_t mmap_offset;
   int ret = vc4_gem_mmap(fd, args->handle, &mmap_offset);
            }
      static int
   vc4_simulator_gem_close_ioctl(int fd, struct drm_gem_close *args)
   {
         /* Free the simulator's internal tracking. */
   struct vc4_simulator_file *file = vc4_get_simulator_file_for_fd(fd);
   struct vc4_simulator_bo *sim_bo = vc4_get_simulator_bo(file,
                     /* Pass the call on down. */
   }
      static int
   vc4_simulator_get_param_ioctl(int fd, struct drm_vc4_get_param *args)
   {
         switch (args->param) {
   case DRM_VC4_PARAM_SUPPORTS_BRANCHES:
   case DRM_VC4_PARAM_SUPPORTS_ETC1:
   case DRM_VC4_PARAM_SUPPORTS_THREADED_FS:
   case DRM_VC4_PARAM_SUPPORTS_FIXED_RCL_ORDER:
                  case DRM_VC4_PARAM_SUPPORTS_MADVISE:
   case DRM_VC4_PARAM_SUPPORTS_PERFMON:
                  case DRM_VC4_PARAM_V3D_IDENT0:
                  case DRM_VC4_PARAM_V3D_IDENT1:
                  default:
            fprintf(stderr, "Unknown DRM_IOCTL_VC4_GET_PARAM(%lld)\n",
      }
      int
   vc4_simulator_ioctl(int fd, unsigned long request, void *args)
   {
         switch (request) {
   case DRM_IOCTL_VC4_SUBMIT_CL:
         case DRM_IOCTL_VC4_CREATE_BO:
         case DRM_IOCTL_VC4_CREATE_SHADER_BO:
         case DRM_IOCTL_VC4_MMAP_BO:
            case DRM_IOCTL_VC4_WAIT_BO:
   case DRM_IOCTL_VC4_WAIT_SEQNO:
            /* We do all of the vc4 rendering synchronously, so we just
   * return immediately on the wait ioctls.  This ignores any
   * native rendering to the host BO, so it does mean we race on
               case DRM_IOCTL_VC4_LABEL_BO:
                  case DRM_IOCTL_VC4_GET_TILING:
   case DRM_IOCTL_VC4_SET_TILING:
            /* Disable these for now, since the sharing with i965 requires
   * linear buffers.
               case DRM_IOCTL_VC4_GET_PARAM:
            case DRM_IOCTL_GEM_CLOSE:
            case DRM_IOCTL_GEM_OPEN:
   case DRM_IOCTL_GEM_FLINK:
         default:
               }
      static void
   vc4_simulator_init_global(void)
   {
         simple_mtx_lock(&sim_state.mutex);
   if (sim_state.refcount++) {
                        sim_state.mem_size = 256 * 1024 * 1024;
   sim_state.mem = calloc(sim_state.mem_size, 1);
   if (!sim_state.mem)
                  /* We supply our own memory so that we can have more aperture
      * available (256MB instead of simpenrose's default 64MB).
               /* Carve out low memory for tile allocation overflow.  The kernel
      * should be automatically handling overflow memory setup on real
   * hardware, but for simulation we just get one shot to set up enough
   * overflow memory before execution.  This overflow mem will be used
   * up over the whole lifetime of simpenrose (not reused on each
   * flush), so it had better be big.
      sim_state.overflow = u_mmAllocMem(sim_state.heap, 32 * 1024 * 1024,
         simpenrose_supply_overflow_mem(sim_state.overflow->ofs,
                     sim_state.fd_map =
               }
      void
   vc4_simulator_init(struct vc4_screen *screen)
   {
                           screen->sim_file->bo_map =
                        drmVersionPtr version = drmGetVersion(screen->fd);
   if (version && strncmp(version->name, "i915", version->name_len) == 0)
         else if (version && strncmp(version->name, "amdgpu", version->name_len) == 0)
         else
         drmFreeVersion(version);
   simple_mtx_lock(&sim_state.mutex);
   _mesa_hash_table_insert(sim_state.fd_map, int_to_key(screen->fd + 1),
                  }
      void
   vc4_simulator_destroy(struct vc4_screen *screen)
   {
         simple_mtx_lock(&sim_state.mutex);
   if (!--sim_state.refcount) {
            _mesa_hash_table_destroy(sim_state.fd_map, NULL);
   u_mmDestroy(sim_state.heap);
      }
   }
      #endif /* USE_VC4_SIMULATOR */
