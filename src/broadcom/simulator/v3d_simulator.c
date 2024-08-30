   /*
   * Copyright © 2014-2017 Broadcom
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
   * @file v3d_simulator.c
   *
   * Implements V3D simulation on top of a non-V3D GEM fd.
   *
   * This file's goal is to emulate the V3D ioctls' behavior in the kernel on
   * top of the simpenrose software simulator.  Generally, V3D driver BOs have a
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
   * window system's BO size matches what V3D is going to use, which of course
   * doesn't work out in practice.  This means that for now, only DRI3 (V3D
   * makes the winsys BOs) is supported, not DRI2 (window system makes the winys
   * BOs).
   */
      #ifdef USE_V3D_SIMULATOR
      #include <stdio.h>
   #include <sys/mman.h>
   #include "c11/threads.h"
   #include "util/hash_table.h"
   #include "util/ralloc.h"
   #include "util/set.h"
   #include "util/simple_mtx.h"
   #include "util/u_dynarray.h"
   #include "util/u_memory.h"
   #include "util/u_mm.h"
   #include "util/u_math.h"
      #include <xf86drm.h>
   #include "drm-uapi/amdgpu_drm.h"
   #include "drm-uapi/i915_drm.h"
   #include "drm-uapi/v3d_drm.h"
      #include "v3d_simulator.h"
   #include "v3d_simulator_wrapper.h"
      /** Global (across GEM fds) state for the simulator */
   static struct v3d_simulator_state {
         simple_mtx_t mutex;
            struct v3d_hw *v3d;
            /* Base virtual address of the heap. */
   void *mem;
   /* Base hardware address of the heap. */
   uint32_t mem_base;
   /* Size of the heap. */
            struct mem_block *heap;
            /** Mapping from GEM fd to struct v3d_simulator_file * */
            /** Last performance monitor ID. */
            /** Total performance counters */
            struct util_dynarray bin_oom;
   } sim_state = {
         };
      enum gem_type {
         GEM_I915,
   GEM_AMDGPU,
   };
      /** Per-GEM-fd state for the simulator. */
   struct v3d_simulator_file {
                  /** Mapping from GEM handle to struct v3d_simulator_bo * */
            /** Dynamic array with performance monitors */
   struct v3d_simulator_perfmon **perfmons;
   uint32_t perfmons_size;
            struct mem_block *gmp;
            /** For specific gpus, use their create ioctl. Otherwise use dumb bo. */
   };
      /** Wrapper for drm_v3d_bo tracking the simulator-specific state. */
   struct v3d_simulator_bo {
                  /** Area for this BO within sim_state->mem */
   struct mem_block *block;
   uint32_t size;
   uint64_t mmap_offset;
   void *sim_vaddr;
            };
      struct v3d_simulator_perfmon {
         uint32_t ncounters;
   uint8_t counters[DRM_V3D_MAX_PERF_COUNTERS];
   };
      static void *
   int_to_key(int key)
   {
         }
      #define PERFMONS_ALLOC_SIZE 100
      static uint32_t
   perfmons_next_id(struct v3d_simulator_file *sim_file) {
         sim_state.last_perfid++;
   if (sim_state.last_perfid > sim_file->perfmons_size) {
            sim_file->perfmons_size += PERFMONS_ALLOC_SIZE;
   sim_file->perfmons = reralloc(sim_file,
                     }
      static struct v3d_simulator_file *
   v3d_get_simulator_file_for_fd(int fd)
   {
         struct hash_entry *entry = _mesa_hash_table_search(sim_state.fd_map,
         }
      /* A marker placed just after each BO, then checked after rendering to make
   * sure it's still there.
   */
   #define BO_SENTINEL		0xfedcba98
      /* 128kb */
   #define GMP_ALIGN2		17
      /**
   * Sets the range of GPU virtual address space to have the given GMP
   * permissions (bit 0 = read, bit 1 = write, write-only forbidden).
   */
   static void
   set_gmp_flags(struct v3d_simulator_file *file,
         {
         assert((offset & ((1 << GMP_ALIGN2) - 1)) == 0);
   int gmp_offset = offset >> GMP_ALIGN2;
   int gmp_count = align(size, 1 << GMP_ALIGN2) >> GMP_ALIGN2;
                     for (int i = gmp_offset; i < gmp_offset + gmp_count; i++) {
            int32_t bitshift = (i % 16) * 2;
      }
      /**
   * Allocates space in simulator memory and returns a tracking struct for it
   * that also contains the drm_gem_cma_object struct.
   */
   static struct v3d_simulator_bo *
   v3d_create_simulator_bo(int fd, unsigned size)
   {
         struct v3d_simulator_file *file = v3d_get_simulator_file_for_fd(fd);
   struct v3d_simulator_bo *sim_bo = rzalloc(file,
                           simple_mtx_lock(&sim_state.mutex);
   sim_bo->block = u_mmAllocMem(sim_state.heap, size + 4, GMP_ALIGN2, 0);
   simple_mtx_unlock(&sim_state.mutex);
                              /* Allocate space for the buffer in simulator memory. */
   sim_bo->sim_vaddr = sim_state.mem + sim_bo->block->ofs - sim_state.mem_base;
                     }
      static struct v3d_simulator_bo *
   v3d_create_simulator_bo_for_gem(int fd, int handle, unsigned size)
   {
         struct v3d_simulator_file *file = v3d_get_simulator_file_for_fd(fd);
   struct v3d_simulator_bo *sim_bo =
                     /* Map the GEM buffer for copy in/out to the simulator.  i915 blocks
      * dumb mmap on render nodes, so use their ioctl directly if we're on
   * one.
      int ret;
   switch (file->gem_type) {
   case GEM_I915:
   {
                                 /* We could potentially use non-gtt (cached) for LLC systems,
   * but the copy-in/out won't be the limiting factor on
   * simulation anyway.
   */
   ret = drmIoctl(fd, DRM_IOCTL_I915_GEM_MMAP_GTT, &map);
      }
   case GEM_AMDGPU:
   {
                           ret = drmIoctl(fd, DRM_IOCTL_AMDGPU_GEM_MMAP, &map);
      }
   default:
   {
            struct drm_mode_map_dumb map = {
         };
      }
   }
   if (ret) {
                        sim_bo->gem_vaddr = mmap(NULL, sim_bo->size,
               if (sim_bo->gem_vaddr == MAP_FAILED) {
            fprintf(stderr, "mmap of bo %d (offset 0x%016llx, size %d) failed\n",
               /* A handle of 0 is used for v3d_gem.c internal allocations that
      * don't need to go in the lookup table.
      if (handle != 0) {
            simple_mtx_lock(&sim_state.mutex);
   _mesa_hash_table_insert(file->bo_map, int_to_key(handle),
               }
      static int bin_fd;
      uint32_t
   v3d_simulator_get_spill(uint32_t spill_size)
   {
         struct v3d_simulator_bo *sim_bo =
            util_dynarray_append(&sim_state.bin_oom, struct v3d_simulator_bo *,
            }
      static void
   v3d_free_simulator_bo(struct v3d_simulator_bo *sim_bo)
   {
                           if (sim_bo->gem_vaddr)
            simple_mtx_lock(&sim_state.mutex);
   u_mmFreeMem(sim_bo->block);
   if (sim_bo->handle) {
               }
   simple_mtx_unlock(&sim_state.mutex);
   }
      static struct v3d_simulator_bo *
   v3d_get_simulator_bo(struct v3d_simulator_file *file, int gem_handle)
   {
         if (gem_handle == 0)
            simple_mtx_lock(&sim_state.mutex);
   struct hash_entry *entry =
                  }
      static void
   v3d_simulator_copy_in_handle(struct v3d_simulator_file *file, int handle)
   {
                  if (!sim_bo)
            }
      static void
   v3d_simulator_copy_out_handle(struct v3d_simulator_file *file, int handle)
   {
                  if (!sim_bo)
                     if (*(uint32_t *)(sim_bo->sim_vaddr +
                     }
      static int
   v3d_simulator_pin_bos(struct v3d_simulator_file *file,
         {
                  for (int i = 0; i < submit->bo_handle_count; i++)
            }
      static int
   v3d_simulator_unpin_bos(struct v3d_simulator_file *file,
         {
                  for (int i = 0; i < submit->bo_handle_count; i++)
            }
      static struct v3d_simulator_perfmon *
   v3d_get_simulator_perfmon(int fd, uint32_t perfid)
   {
         if (!perfid || perfid > sim_state.last_perfid)
                     simple_mtx_lock(&sim_state.mutex);
   assert(perfid <= file->perfmons_size);
   struct v3d_simulator_perfmon *perfmon = file->perfmons[perfid - 1];
            }
      static void
   v3d_simulator_perfmon_switch(int fd, uint32_t perfid)
   {
         struct v3d_simulator_file *file = v3d_get_simulator_file_for_fd(fd);
            if (perfid == file->active_perfid)
            perfmon = v3d_get_simulator_perfmon(fd, file->active_perfid);
   if (perfmon)
                        perfmon = v3d_get_simulator_perfmon(fd, perfid);
   if (perfmon)
                        }
      static int
   v3d_simulator_signal_syncobjs(int fd, struct drm_v3d_multi_sync *ms)
   {
         struct drm_v3d_sem *out_syncs = (void *)(uintptr_t)ms->out_syncs;
   int n_syncobjs = ms->out_sync_count;
            for (int i = 0; i < n_syncobjs; i++)
         }
      static int
   v3d_simulator_process_post_deps(int fd, struct drm_v3d_extension *ext)
   {
         int ret = 0;
   while (ext && ext->id != DRM_V3D_EXT_ID_MULTI_SYNC)
            if (ext) {
               }
   }
      static int
   v3d_simulator_submit_cl_ioctl(int fd, struct drm_v3d_submit_cl *submit)
   {
         struct v3d_simulator_file *file = v3d_get_simulator_file_for_fd(fd);
            ret = v3d_simulator_pin_bos(file, submit);
   if (ret)
            mtx_lock(&sim_state.submit_lock);
            v3d_simulator_perfmon_switch(fd, submit->perfmon_id);
            util_dynarray_foreach(&sim_state.bin_oom, struct v3d_simulator_bo *,
               }
                     ret = v3d_simulator_unpin_bos(file, submit);
   if (ret)
            if (submit->flags & DRM_V3D_SUBMIT_EXTENSION) {
                        }
      /**
   * Do fixups after a BO has been opened from a handle.
   *
   * This could be done at DRM_IOCTL_GEM_OPEN/DRM_IOCTL_GEM_PRIME_FD_TO_HANDLE
   * time, but we're still using drmPrimeFDToHandle() so we have this helper to
   * be called afterward instead.
   */
   void v3d_simulator_open_from_handle(int fd, int handle, uint32_t size)
   {
         }
      /**
   * Simulated ioctl(fd, DRM_V3D_CREATE_BO) implementation.
   *
   * Making a V3D BO is just a matter of making a corresponding BO on the host.
   */
   static int
   v3d_simulator_create_bo_ioctl(int fd, struct drm_v3d_create_bo *args)
   {
                  /* i915 bans dumb create on render nodes, so we have to use their
      * native ioctl in case we're on a render node.
      int ret;
   switch (file->gem_type) {
   case GEM_I915:
   {
                                             }
   case GEM_AMDGPU:
   {
                                       }
   default:
   {
            struct drm_mode_create_dumb create = {
                                          }
   }
   if (ret == 0) {
                                       }
      /**
   * Simulated ioctl(fd, DRM_V3D_MMAP_BO) implementation.
   *
   * We've already grabbed the mmap offset when we created the sim bo, so just
   * return it.
   */
   static int
   v3d_simulator_mmap_bo_ioctl(int fd, struct drm_v3d_mmap_bo *args)
   {
         struct v3d_simulator_file *file = v3d_get_simulator_file_for_fd(fd);
   struct v3d_simulator_bo *sim_bo = v3d_get_simulator_bo(file,
                     }
      static int
   v3d_simulator_get_bo_offset_ioctl(int fd, struct drm_v3d_get_bo_offset *args)
   {
         struct v3d_simulator_file *file = v3d_get_simulator_file_for_fd(fd);
   struct v3d_simulator_bo *sim_bo = v3d_get_simulator_bo(file,
                     }
      static int
   v3d_simulator_gem_close_ioctl(int fd, struct drm_gem_close *args)
   {
         /* Free the simulator's internal tracking. */
   struct v3d_simulator_file *file = v3d_get_simulator_file_for_fd(fd);
   struct v3d_simulator_bo *sim_bo = v3d_get_simulator_bo(file,
                     /* Pass the call on down. */
   }
      static int
   v3d_simulator_submit_tfu_ioctl(int fd, struct drm_v3d_submit_tfu *args)
   {
         struct v3d_simulator_file *file = v3d_get_simulator_file_for_fd(fd);
            v3d_simulator_copy_in_handle(file, args->bo_handles[0]);
   v3d_simulator_copy_in_handle(file, args->bo_handles[1]);
   v3d_simulator_copy_in_handle(file, args->bo_handles[2]);
                              if (ret)
            if (args->flags & DRM_V3D_SUBMIT_EXTENSION) {
                        }
      static int
   v3d_simulator_submit_csd_ioctl(int fd, struct drm_v3d_submit_csd *args)
   {
         struct v3d_simulator_file *file = v3d_get_simulator_file_for_fd(fd);
   uint32_t *bo_handles = (uint32_t *)(uintptr_t)args->bo_handles;
            for (int i = 0; i < args->bo_handle_count; i++)
                     ret = v3d_X_simulator(submit_csd_ioctl)(sim_state.v3d, args,
            for (int i = 0; i < args->bo_handle_count; i++)
            if (ret < 0)
            if (args->flags & DRM_V3D_SUBMIT_EXTENSION) {
                        }
      static int
   v3d_simulator_perfmon_create_ioctl(int fd, struct drm_v3d_perfmon_create *args)
   {
                  if (args->ncounters == 0 ||
                  struct v3d_simulator_perfmon *perfmon = rzalloc(file,
            perfmon->ncounters = args->ncounters;
   for (int i = 0; i < args->ncounters; i++) {
            if (args->counters[i] >= sim_state.perfcnt_total) {
               } else {
               simple_mtx_lock(&sim_state.mutex);
   args->id = perfmons_next_id(file);
   file->perfmons[args->id - 1] = perfmon;
            }
      static int
   v3d_simulator_perfmon_destroy_ioctl(int fd, struct drm_v3d_perfmon_destroy *args)
   {
         struct v3d_simulator_file *file = v3d_get_simulator_file_for_fd(fd);
   struct v3d_simulator_perfmon *perfmon =
            if (!perfmon)
            simple_mtx_lock(&sim_state.mutex);
   file->perfmons[args->id - 1] = NULL;
                     }
      static int
   v3d_simulator_perfmon_get_values_ioctl(int fd, struct drm_v3d_perfmon_get_values *args)
   {
                           /* Stop the perfmon if it is still active */
   if (args->id == file->active_perfid)
                     struct v3d_simulator_perfmon *perfmon =
            if (!perfmon)
                     }
      int
   v3d_simulator_ioctl(int fd, unsigned long request, void *args)
   {
         switch (request) {
   case DRM_IOCTL_V3D_SUBMIT_CL:
         case DRM_IOCTL_V3D_CREATE_BO:
         case DRM_IOCTL_V3D_MMAP_BO:
         case DRM_IOCTL_V3D_GET_BO_OFFSET:
            case DRM_IOCTL_V3D_WAIT_BO:
            /* We do all of the v3d rendering synchronously, so we just
   * return immediately on the wait ioctls.  This ignores any
   * native rendering to the host BO, so it does mean we race on
               case DRM_IOCTL_V3D_GET_PARAM:
            case DRM_IOCTL_GEM_CLOSE:
            case DRM_IOCTL_V3D_SUBMIT_TFU:
            case DRM_IOCTL_V3D_SUBMIT_CSD:
            case DRM_IOCTL_V3D_PERFMON_CREATE:
            case DRM_IOCTL_V3D_PERFMON_DESTROY:
            case DRM_IOCTL_V3D_PERFMON_GET_VALUES:
            case DRM_IOCTL_GEM_OPEN:
   case DRM_IOCTL_GEM_FLINK:
         default:
               }
      uint32_t
   v3d_simulator_get_mem_size(void)
   {
         }
      uint32_t
   v3d_simulator_get_mem_free(void)
   {
      uint32_t total_free = 0;
   struct mem_block *p;
   for (p = sim_state.heap->next_free; p != sim_state.heap; p = p->next_free)
            }
      static void
   v3d_simulator_init_global()
   {
         simple_mtx_lock(&sim_state.mutex);
   if (sim_state.refcount++) {
                        sim_state.v3d = v3d_hw_auto_new(NULL);
   v3d_hw_alloc_mem(sim_state.v3d, 1024 * 1024 * 1024);
   sim_state.mem_base =
                  /* Allocate from anywhere from 4096 up.  We don't allocate at 0,
      * because for OQs and some other addresses in the HW, 0 means
   * disabled.
               /* Make a block of 0xd0 at address 0 to make sure we don't screw up
      * and land there.
      struct mem_block *b = u_mmAllocMem(sim_state.heap, 4096, GMP_ALIGN2, 0);
                              sim_state.fd_map =
                                 v3d_X_simulator(init_regs)(sim_state.v3d);
   }
      struct v3d_simulator_file *
   v3d_simulator_init(int fd)
   {
                           drmVersionPtr version = drmGetVersion(fd);
   if (version && strncmp(version->name, "i915", version->name_len) == 0)
         else if (version && strncmp(version->name, "amdgpu", version->name_len) == 0)
         else
                  sim_file->bo_map =
                        simple_mtx_lock(&sim_state.mutex);
   _mesa_hash_table_insert(sim_state.fd_map, int_to_key(fd + 1),
                  sim_file->gmp = u_mmAllocMem(sim_state.heap, 8096, GMP_ALIGN2, 0);
   sim_file->gmp_vaddr = (sim_state.mem + sim_file->gmp->ofs -
                  }
      void
   v3d_simulator_destroy(struct v3d_simulator_file *sim_file)
   {
         simple_mtx_lock(&sim_state.mutex);
   if (!--sim_state.refcount) {
            _mesa_hash_table_destroy(sim_state.fd_map, NULL);
   util_dynarray_fini(&sim_state.bin_oom);
   u_mmDestroy(sim_state.heap);
      }
   simple_mtx_unlock(&sim_state.mutex);
   }
      #endif /* USE_V3D_SIMULATOR */
