   /*
   * Copyright © 2020 Intel Corporation
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
      #include <stdio.h>
   #include <stdlib.h>
   #include <unistd.h>
   #include <sys/ioctl.h>
   #include <sys/mman.h>
   #include <sys/types.h>
   #include <sys/socket.h>
   #include <sys/time.h>
   #include <sys/resource.h>
   #include <sys/un.h>
      #include "common/intel_gem.h"
   #include "dev/intel_device_info.h"
   #include "drm-uapi/i915_drm.h"
   #include "drm-shim/drm_shim.h"
   #include "util/macros.h"
   #include "util/vma.h"
      struct i915_device {
      struct intel_device_info devinfo;
      };
      struct i915_bo {
      struct shim_bo base;
   uint32_t tiling_mode;
      };
      static struct i915_device i915 = {};
      bool drm_shim_driver_prefers_first_render_node = true;
      static int
   i915_ioctl_noop(int fd, unsigned long request, void *arg)
   {
         }
      static int
   i915_ioctl_gem_set_tiling(int fd, unsigned long request, void *arg)
   {
      struct shim_fd *shim_fd = drm_shim_fd_lookup(fd);
   struct drm_i915_gem_set_tiling *tiling_arg = arg;
            if (!bo)
            bo->tiling_mode = tiling_arg->tiling_mode;
               }
      static int
   i915_ioctl_gem_get_tiling(int fd, unsigned long request, void *arg)
   {
      struct shim_fd *shim_fd = drm_shim_fd_lookup(fd);
   struct drm_i915_gem_get_tiling *tiling_arg = arg;
            if (!bo)
            tiling_arg->tiling_mode = bo->tiling_mode;
   tiling_arg->swizzle_mode = I915_BIT_6_SWIZZLE_NONE;
               }
      static int
   i915_ioctl_gem_create(int fd, unsigned long request, void *arg)
   {
      struct shim_fd *shim_fd = drm_shim_fd_lookup(fd);
   struct drm_i915_gem_create *create = arg;
                                          }
      static int
   i915_ioctl_gem_create_ext(int fd, unsigned long request, void *arg)
   {
      struct shim_fd *shim_fd = drm_shim_fd_lookup(fd);
   struct drm_i915_gem_create_ext *create = arg;
                                          }
      static int
   i915_ioctl_gem_mmap(int fd, unsigned long request, void *arg)
   {
      struct shim_fd *shim_fd = drm_shim_fd_lookup(fd);
   struct drm_i915_gem_mmap *mmap_arg = arg;
            if (!bo)
            if (!bo->map)
      bo->map = drm_shim_mmap(shim_fd, bo->size, PROT_READ | PROT_WRITE, MAP_SHARED, -1,
                     }
      static int
   i915_ioctl_gem_mmap_offset(int fd, unsigned long request, void *arg)
   {
      struct shim_fd *shim_fd = drm_shim_fd_lookup(fd);
   struct drm_i915_gem_mmap_offset *mmap_arg = arg;
            if (!bo)
            if (!bo->map)
      bo->map = drm_shim_mmap(shim_fd, bo->size, PROT_READ | PROT_WRITE, MAP_SHARED, -1,
                     }
      static int
   i915_ioctl_gem_userptr(int fd, unsigned long request, void *arg)
   {
      struct shim_fd *shim_fd = drm_shim_fd_lookup(fd);
   struct drm_i915_gem_userptr *userptr = arg;
                                          }
      static int
   i915_ioctl_gem_context_create(int fd, unsigned long request, void *arg)
   {
                           }
      static int
   i915_ioctl_gem_context_getparam(int fd, unsigned long request, void *arg)
   {
               if (param->param ==  I915_CONTEXT_PARAM_GTT_SIZE) {
      if (i915.devinfo.ver >= 8 && i915.devinfo.platform != INTEL_PLATFORM_CHV)
         else
      } else {
                     }
      static int
   i915_ioctl_get_param(int fd, unsigned long request, void *arg)
   {
               switch (gp->param) {
   case I915_PARAM_CHIPSET_ID:
      *gp->value = i915.device_id;
      case I915_PARAM_REVISION:
      *gp->value = 0;
      case I915_PARAM_CS_TIMESTAMP_FREQUENCY:
      *gp->value = i915.devinfo.timestamp_frequency;
      case I915_PARAM_HAS_ALIASING_PPGTT:
      if (i915.devinfo.ver < 6)
         else if (i915.devinfo.ver <= 7)
         else
               case I915_PARAM_NUM_FENCES_AVAIL:
      *gp->value = 8; /* gfx2/3 value, unused in brw/iris */
         case I915_PARAM_HAS_BLT:
      *gp->value = 1; /* gfx2/3 value, unused in brw/iris */
         case I915_PARAM_HAS_BSD:
   case I915_PARAM_HAS_LLC:
   case I915_PARAM_HAS_VEBOX:
      *gp->value = 0; /* gfx2/3 value, unused in brw/iris */
         case I915_PARAM_HAS_GEM:
   case I915_PARAM_HAS_RELAXED_DELTA:
   case I915_PARAM_HAS_RELAXED_FENCING:
   case I915_PARAM_HAS_WAIT_TIMEOUT:
   case I915_PARAM_HAS_EXECBUF2:
   case I915_PARAM_HAS_EXEC_SOFTPIN:
   case I915_PARAM_HAS_EXEC_CAPTURE:
   case I915_PARAM_HAS_EXEC_FENCE:
   case I915_PARAM_HAS_EXEC_FENCE_ARRAY:
   case I915_PARAM_HAS_CONTEXT_ISOLATION:
   case I915_PARAM_HAS_EXEC_ASYNC:
   case I915_PARAM_HAS_EXEC_NO_RELOC:
   case I915_PARAM_HAS_EXEC_BATCH_FIRST:
      *gp->value = true;
      case I915_PARAM_HAS_EXEC_TIMELINE_FENCES:
      *gp->value = false;
      case I915_PARAM_CMD_PARSER_VERSION:
      /* Most recent version in drivers/gpu/drm/i915/i915_cmd_parser.c */
   *gp->value = 10;
      case I915_PARAM_MMAP_VERSION:
   case I915_PARAM_MMAP_GTT_VERSION:
      *gp->value = 4 /* MMAP_OFFSET support */;
      case I915_PARAM_SUBSLICE_TOTAL:
      *gp->value = 0;
   for (uint32_t s = 0; s < i915.devinfo.num_slices; s++)
            case I915_PARAM_EU_TOTAL:
      *gp->value = 0;
   for (uint32_t s = 0; s < i915.devinfo.num_slices; s++)
            case I915_PARAM_PERF_REVISION:
      *gp->value = 3;
      case I915_PARAM_HAS_USERPTR_PROBE:
      *gp->value = 0;
      default:
                  fprintf(stderr, "Unknown DRM_IOCTL_I915_GET_PARAM %d\n", gp->param);
      }
      static int
   query_write_topology(struct drm_i915_query_item *item)
   {
      struct drm_i915_query_topology_info *info =
         int32_t length =
      sizeof(*info) +
   DIV_ROUND_UP(i915.devinfo.num_slices, 8) +
   i915.devinfo.num_slices * DIV_ROUND_UP(i915.devinfo.num_subslices[0], 8) +
   i915.devinfo.num_slices * i915.devinfo.num_subslices[0] *
         if (item->length == 0) {
      item->length = length;
               if (item->length < length) {
      fprintf(stderr, "size too small\n");
               if (info->flags) {
      fprintf(stderr, "invalid topology flags\n");
               info->max_slices = i915.devinfo.num_slices;
   info->max_subslices = i915.devinfo.num_subslices[0];
            info->subslice_offset = DIV_ROUND_UP(i915.devinfo.num_slices, 8);
   info->subslice_stride = DIV_ROUND_UP(i915.devinfo.num_subslices[0], 8);
   info->eu_offset = info->subslice_offset + info->max_slices * info->subslice_stride;
            uint32_t slice_mask = (1u << i915.devinfo.num_slices) - 1;
   for (uint32_t i = 0; i < info->subslice_offset; i++)
            for (uint32_t s = 0; s < i915.devinfo.num_slices; s++) {
      uint32_t subslice_mask = (1u << i915.devinfo.num_subslices[s]) - 1;
   for (uint32_t i = 0; i < info->subslice_stride; i++) {
      info->data[info->subslice_offset + s * info->subslice_stride + i] =
                  for (uint32_t s = 0; s < i915.devinfo.num_slices; s++) {
      for (uint32_t ss = 0; ss < i915.devinfo.num_subslices[s]; ss++) {
      uint32_t eu_mask = (1u << info->max_eus_per_subslice) - 1;
   for (uint32_t i = 0; i < DIV_ROUND_UP(info->max_eus_per_subslice, 8); i++) {
      info->data[info->eu_offset +
                              }
      static int
   i915_ioctl_query(int fd, unsigned long request, void *arg)
   {
      struct drm_i915_query *query = arg;
            if (query->flags) {
      fprintf(stderr, "invalid query flags\n");
               for (uint32_t i = 0; i < query->num_items; i++) {
               switch (item->query_id) {
   case DRM_I915_QUERY_TOPOLOGY_INFO:
   case DRM_I915_QUERY_GEOMETRY_SUBSLICES: {
      int ret = query_write_topology(item);
   if (ret)
                     case DRM_I915_QUERY_ENGINE_INFO: {
      uint32_t num_copy = 1;
                                 int32_t data_length =
                  if (item->length == 0) {
      item->length = data_length;
      } else if (item->length < data_length) {
      item->length = -EINVAL;
                        for (uint32_t e = 0; e < num_render; e++, info->num_engines++) {
      info->engines[info->num_engines].engine.engine_class =
                     for (uint32_t e = 0; e < num_copy; e++, info->num_engines++) {
      info->engines[info->num_engines].engine.engine_class =
                                                            case DRM_I915_QUERY_PERF_CONFIG:
      /* This is known but not supported by the shim.  Handling this here
   * suppresses some spurious warning messages in shader-db runs.
   */
               case DRM_I915_QUERY_MEMORY_REGIONS: {
      uint32_t num_regions = i915.devinfo.has_local_mem ? 2 : 1;
   struct drm_i915_query_memory_regions *info =
                        if (item->length == 0) {
      item->length = data_length;
      } else if (item->length < (int32_t)data_length) {
      item->length = -EINVAL;
      } else {
      memset(info, 0, data_length);
   info->num_regions = num_regions;
   info->regions[0].region.memory_class = I915_MEMORY_CLASS_SYSTEM;
   info->regions[0].region.memory_instance = 0;
   /* Report 4Gb even if it's not actually true, it looks more like a
   * real device.
   */
   info->regions[0].probed_size = 4ull * 1024 * 1024 * 1024;
   info->regions[0].unallocated_size = -1ll;
   if (i915.devinfo.has_local_mem) {
      info->regions[1].region.memory_class = I915_MEMORY_CLASS_DEVICE;
   info->regions[1].region.memory_instance = 0;
   info->regions[1].probed_size = 4ull * 1024 * 1024 * 1024;
      }
      }
               default:
      fprintf(stderr, "Unknown drm_i915_query_item id=%lli\n", item->query_id);
   item->length = -EINVAL;
                     }
      static int
   i915_gem_get_aperture(int fd, unsigned long request, void *arg)
   {
               if (i915.devinfo.ver >= 8 &&
      i915.devinfo.platform != INTEL_PLATFORM_CHV) {
   aperture->aper_size = 1ull << 48;
      } else {
      aperture->aper_size = 1ull << 31;
                  }
      static ioctl_fn_t driver_ioctls[] = {
      [DRM_I915_GETPARAM] = i915_ioctl_get_param,
                     [DRM_I915_GEM_CREATE] = i915_ioctl_gem_create,
   [DRM_I915_GEM_CREATE_EXT] = i915_ioctl_gem_create_ext,
   [DRM_I915_GEM_MMAP] = i915_ioctl_gem_mmap,
   [DRM_I915_GEM_MMAP_GTT] = i915_ioctl_gem_mmap_offset,
   [DRM_I915_GEM_SET_TILING] = i915_ioctl_gem_set_tiling,
   [DRM_I915_GEM_CONTEXT_CREATE] = i915_ioctl_gem_context_create,
   [DRM_I915_GEM_CONTEXT_DESTROY] = i915_ioctl_noop,
   [DRM_I915_GEM_CONTEXT_GETPARAM] = i915_ioctl_gem_context_getparam,
   [DRM_I915_GEM_CONTEXT_SETPARAM] = i915_ioctl_noop,
   [DRM_I915_GEM_EXECBUFFER2] = i915_ioctl_noop,
   /* [DRM_I915_GEM_EXECBUFFER2_WR] = i915_ioctl_noop,
                                       [DRM_I915_GEM_SET_DOMAIN] = i915_ioctl_noop,
   [DRM_I915_GEM_GET_CACHING] = i915_ioctl_noop,
   [DRM_I915_GEM_SET_CACHING] = i915_ioctl_noop,
   [DRM_I915_GEM_GET_TILING] = i915_ioctl_gem_get_tiling,
   [DRM_I915_GEM_MADVISE] = i915_ioctl_noop,
   [DRM_I915_GEM_WAIT] = i915_ioctl_noop,
      };
      void
   drm_shim_driver_init(void)
   {
      i915.device_id = 0;
   const char *device_id_str = getenv("INTEL_STUB_GPU_DEVICE_ID");
   if (device_id_str != NULL) {
      /* Set as 0 if strtoul fails */
      }
   if (i915.device_id == 0) {
      const char *user_platform = getenv("INTEL_STUB_GPU_PLATFORM");
   /* Use SKL if nothing is specified. */
               if (!intel_get_device_info_from_pci_id(i915.device_id, &i915.devinfo))
            shim_device.bus_type = DRM_BUS_PCI;
   shim_device.driver_name = "i915";
   shim_device.driver_ioctls = driver_ioctls;
            char uevent_content[1024];
   snprintf(uevent_content, sizeof(uevent_content),
            "DRIVER=i915\n"
   "PCI_CLASS=30000\n"
   "PCI_ID=8086:%x\n"
   "PCI_SUBSYS_ID=1028:075B\n"
   "PCI_SLOT_NAME=0000:00:02.0\n"
      drm_shim_override_file(uevent_content,
               drm_shim_override_file("0x0\n",
               char device_content[10];
   snprintf(device_content, sizeof(device_content),
         drm_shim_override_file("0x8086",
               drm_shim_override_file("0x8086",
         drm_shim_override_file(device_content,
               drm_shim_override_file(device_content,
         drm_shim_override_file("0x1234",
               drm_shim_override_file("0x1234",
         drm_shim_override_file("0x1234",
               drm_shim_override_file("0x1234",
      }
