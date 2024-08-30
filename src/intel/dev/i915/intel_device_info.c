   /*
   * Copyright © 2023 Intel Corporation
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
      #include <string.h>
      #include "intel/dev/i915/intel_device_info.h"
   #include "intel/dev/intel_device_info.h"
      #include "intel/dev/intel_hwconfig.h"
   #include "intel/common/intel_gem.h"
   #include "intel/common/i915/intel_gem.h"
      #include "util/bitscan.h"
   #include "util/log.h"
   #include "util/os_misc.h"
      #include "drm-uapi/i915_drm.h"
      /* At some point in time, some people decided to redefine what topology means,
   * from useful HW related information (slice, subslice, etc...), to much less
   * useful generic stuff that no one cares about (a single slice with lots of
   * subslices). Of course all of this was done without asking the people who
   * defined the topology query in the first place, to solve a lack of
   * information Gfx10+. This function is here to workaround the fact it's not
   * possible to change people's mind even before this stuff goes upstream. Sad
   * times...
   */
   static void
   update_from_single_slice_topology(struct intel_device_info *devinfo,
               {
      /* An array of bit masks of the subslices available for 3D
   * workloads, analogous to intel_device_info::subslice_masks.  This
   * may differ from the set of enabled subslices on XeHP+ platforms
   * with compute-only subslices.
   */
                              assert(topology->max_slices == 1);
   assert(topology->max_subslices > 0);
            /* i915 gives us only one slice so we have to rebuild that out of groups of
   * 4 dualsubslices.
   */
   devinfo->max_subslices_per_slice = 4;
   devinfo->max_eus_per_subslice = 16;
   devinfo->subslice_slice_stride = 1;
   devinfo->eu_slice_stride = DIV_ROUND_UP(16 * 4, 8);
            for (uint32_t ss_idx = 0; ss_idx < topology->max_subslices; ss_idx++) {
      const uint32_t s = ss_idx / 4;
            /* Determine whether ss_idx is enabled (ss_idx_available) and
   * available for 3D workloads (geom_ss_idx_available), which may
   * differ on XeHP+ if ss_idx is a compute-only DSS.
   */
   const bool ss_idx_available =
      (topology->data[topology->subslice_offset + ss_idx / 8] >>
      const bool geom_ss_idx_available =
                  if (geom_ss_idx_available) {
      assert(ss_idx_available);
   geom_subslice_masks[s * devinfo->subslice_slice_stride +
               if (!ss_idx_available)
            devinfo->max_slices = MAX2(devinfo->max_slices, s + 1);
            devinfo->subslice_masks[s * devinfo->subslice_slice_stride +
            for (uint32_t eu = 0; eu < devinfo->max_eus_per_subslice; eu++) {
      const bool eu_available =
      (topology->data[topology->eu_offset +
                              devinfo->eu_masks[s * devinfo->eu_slice_stride +
                        intel_device_info_topology_update_counts(devinfo);
   intel_device_info_update_pixel_pipes(devinfo, geom_subslice_masks);
      }
      static void
   update_from_topology(struct intel_device_info *devinfo,
         {
               assert(topology->max_slices > 0);
   assert(topology->max_subslices > 0);
                     devinfo->eu_subslice_stride = DIV_ROUND_UP(topology->max_eus_per_subslice, 8);
            assert(sizeof(devinfo->slice_masks) >= DIV_ROUND_UP(topology->max_slices, 8));
   memcpy(&devinfo->slice_masks, topology->data, DIV_ROUND_UP(topology->max_slices, 8));
   devinfo->max_slices = topology->max_slices;
   devinfo->max_subslices_per_slice = topology->max_subslices;
            uint32_t subslice_mask_len =
         assert(sizeof(devinfo->subslice_masks) >= subslice_mask_len);
   memcpy(devinfo->subslice_masks, &topology->data[topology->subslice_offset],
            uint32_t eu_mask_len =
         assert(sizeof(devinfo->eu_masks) >= eu_mask_len);
            /* Now that all the masks are in place, update the counts. */
   intel_device_info_topology_update_counts(devinfo);
   intel_device_info_update_pixel_pipes(devinfo, devinfo->subslice_masks);
      }
      /* Generate detailed mask from the I915_PARAM_SLICE_MASK,
   * I915_PARAM_SUBSLICE_MASK & I915_PARAM_EU_TOTAL getparam.
   */
   bool
   intel_device_info_i915_update_from_masks(struct intel_device_info *devinfo, uint32_t slice_mask,
         {
                                 topology = calloc(1, sizeof(*topology) + data_length);
   if (!topology)
            topology->max_slices = util_last_bit(slice_mask);
            topology->subslice_offset = DIV_ROUND_UP(topology->max_slices, 8);
            uint32_t n_subslices = __builtin_popcount(slice_mask) *
         uint32_t max_eus_per_subslice = DIV_ROUND_UP(n_eus, n_subslices);
            topology->max_eus_per_subslice = max_eus_per_subslice;
   topology->eu_offset = topology->subslice_offset +
                  /* Set slice mask in topology */
   for (int b = 0; b < topology->subslice_offset; b++)
                        /* Set subslice mask in topology */
   for (int b = 0; b < topology->subslice_stride; b++) {
                                 /* Set eu mask in topology */
   for (int ss = 0; ss < topology->max_subslices; ss++) {
      for (int b = 0; b < topology->eu_stride; b++) {
                                       update_from_topology(devinfo, topology);
               }
      static bool
   getparam(int fd, uint32_t param, int *value)
   {
               struct drm_i915_getparam gp = {
      .param = param,
               int ret = intel_ioctl(fd, DRM_IOCTL_I915_GETPARAM, &gp);
   if (ret != 0)
            *value = tmp;
      }
      static bool
   get_context_param(int fd, uint32_t context, uint32_t param, uint64_t *value)
   {
      struct drm_i915_gem_context_param gp = {
      .ctx_id = context,
               int ret = intel_ioctl(fd, DRM_IOCTL_I915_GEM_CONTEXT_GETPARAM, &gp);
   if (ret != 0)
            *value = gp.value;
      }
      /**
   * for gfx8/gfx9, SLICE_MASK/SUBSLICE_MASK can be used to compute the topology
   * (kernel 4.13+)
   */
   static bool
   getparam_topology(struct intel_device_info *devinfo, int fd)
   {
      int slice_mask = 0;
   if (!getparam(fd, I915_PARAM_SLICE_MASK, &slice_mask))
            int n_eus;
   if (!getparam(fd, I915_PARAM_EU_TOTAL, &n_eus))
            int subslice_mask = 0;
   if (!getparam(fd, I915_PARAM_SUBSLICE_MASK, &subslice_mask))
                  maybe_warn:
      /* Only with Gfx8+ are we starting to see devices with fusing that can only
   * be detected at runtime.
   */
   if (devinfo->ver >= 8)
               }
      /**
   * preferred API for updating the topology in devinfo (kernel 4.17+)
   */
   static bool
   query_topology(struct intel_device_info *devinfo, int fd)
   {
      struct drm_i915_query_topology_info *topo_info =
         if (topo_info == NULL)
            if (devinfo->verx10 >= 125) {
      struct drm_i915_query_topology_info *geom_topo_info =
         if (geom_topo_info == NULL) {
      free(topo_info);
               update_from_single_slice_topology(devinfo, topo_info, geom_topo_info);
      } else {
                                 }
      /**
   * Reports memory region info, and allows buffers to target system-memory,
   * and/or device local memory.
   */
   bool
   intel_device_info_i915_query_regions(struct intel_device_info *devinfo, int fd, bool update)
   {
      struct drm_i915_query_memory_regions *meminfo =
         if (meminfo == NULL)
            for (int i = 0; i < meminfo->num_regions; i++) {
      const struct drm_i915_memory_region_info *mem = &meminfo->regions[i];
   switch (mem->region.memory_class) {
   case I915_MEMORY_CLASS_SYSTEM: {
      if (!update) {
      devinfo->mem.sram.mem.klass = mem->region.memory_class;
   devinfo->mem.sram.mem.instance = mem->region.memory_instance;
      } else {
      assert(devinfo->mem.sram.mem.klass == mem->region.memory_class);
   assert(devinfo->mem.sram.mem.instance == mem->region.memory_instance);
      }
   /* The kernel uAPI only reports an accurate unallocated_size value
   * for I915_MEMORY_CLASS_DEVICE.
   */
   uint64_t available;
   if (os_get_available_system_memory(&available))
            }
   case I915_MEMORY_CLASS_DEVICE:
      if (!update) {
      devinfo->mem.vram.mem.klass = mem->region.memory_class;
   devinfo->mem.vram.mem.instance = mem->region.memory_instance;
   if (mem->probed_cpu_visible_size > 0) {
      devinfo->mem.vram.mappable.size = mem->probed_cpu_visible_size;
   devinfo->mem.vram.unmappable.size =
      } else {
      /* We are running on an older kernel without support for the
   * small-bar uapi. These kernels only support systems where the
   * entire vram is mappable.
   */
   devinfo->mem.vram.mappable.size = mem->probed_size;
         } else {
      assert(devinfo->mem.vram.mem.klass == mem->region.memory_class);
   assert(devinfo->mem.vram.mem.instance == mem->region.memory_instance);
   assert((devinfo->mem.vram.mappable.size +
      }
   if (mem->unallocated_cpu_visible_size > 0) {
      if (mem->unallocated_size != -1) {
      devinfo->mem.vram.mappable.free = mem->unallocated_cpu_visible_size;
   devinfo->mem.vram.unmappable.free =
         } else {
      /* We are running on an older kernel without support for the
   * small-bar uapi. These kernels only support systems where the
   * entire vram is mappable.
   */
   if (mem->unallocated_size != -1) {
      devinfo->mem.vram.mappable.free = mem->unallocated_size;
         }
      default:
                     free(meminfo);
   devinfo->mem.use_class_instance = true;
      }
      static int
   intel_get_aperture_size(int fd, uint64_t *size)
   {
               int ret = intel_ioctl(fd, DRM_IOCTL_I915_GEM_GET_APERTURE, &aperture);
   if (ret == 0 && size)
               }
      static bool
   has_bit6_swizzle(int fd)
   {
               struct drm_i915_gem_create gem_create = {
                  if (intel_ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create)) {
      unreachable("Failed to create GEM BO");
                        /* set_tiling overwrites the input on the error path, so we have to open
   * code intel_ioctl.
   */
   struct drm_i915_gem_set_tiling set_tiling = {
      .handle = gem_create.handle,
   .tiling_mode = I915_TILING_X,
               if (intel_ioctl(fd, DRM_IOCTL_I915_GEM_SET_TILING, &set_tiling)) {
      unreachable("Failed to set BO tiling");
               struct drm_i915_gem_get_tiling get_tiling = {
                  if (intel_ioctl(fd, DRM_IOCTL_I915_GEM_GET_TILING, &get_tiling)) {
      unreachable("Failed to get BO tiling");
               assert(get_tiling.tiling_mode == I915_TILING_X);
         close_and_return:
      memset(&close, 0, sizeof(close));
   close.handle = gem_create.handle;
               }
      static bool
   has_get_tiling(int fd)
   {
               struct drm_i915_gem_create gem_create = {
                  if (intel_ioctl(fd, DRM_IOCTL_I915_GEM_CREATE, &gem_create)) {
      unreachable("Failed to create GEM BO");
               struct drm_i915_gem_get_tiling get_tiling = {
         };
            struct drm_gem_close close = {
         };
               }
      static void
   fixup_chv_device_info(struct intel_device_info *devinfo)
   {
               /* Cherryview is annoying.  The number of EUs is depending on fusing and
   * isn't determinable from the PCI ID alone.  We default to the minimum
   * available for that PCI ID and then compute the real value from the
   * subslice information we get from the kernel.
   */
   const uint32_t subslice_total = intel_device_info_subslice_total(devinfo);
            /* Logical CS threads = EUs per subslice * num threads per EU */
   uint32_t max_cs_threads =
            /* Fuse configurations may give more threads than expected, never less. */
   if (max_cs_threads > devinfo->max_cs_threads)
                     /* Braswell is even more annoying.  Its marketing name isn't determinable
   * from the PCI ID and is also dependent on fusing.
   */
   if (devinfo->pci_device_id != 0x22B1)
            char *bsw_model;
   switch (eu_total) {
   case 16: bsw_model = "405"; break;
   case 12: bsw_model = "400"; break;
   default: bsw_model = "   "; break;
            char *needle = strstr(devinfo->name, "XXX");
   assert(needle);
   if (needle)
      }
      void *
   intel_device_info_i915_query_hwconfig(int fd, int32_t *len)
   {
         }
      bool intel_device_info_i915_get_info_from_fd(int fd, struct intel_device_info *devinfo)
   {
      void *hwconfig_blob;
            hwconfig_blob = intel_device_info_i915_query_hwconfig(fd, &len);
   if (hwconfig_blob) {
      if (intel_hwconfig_process_table(devinfo, hwconfig_blob, len))
                        int val;
   if (getparam(fd, I915_PARAM_CS_TIMESTAMP_FREQUENCY, &val))
         else if (devinfo->ver >= 10) {
      mesa_loge("Kernel 4.15 required to read the CS timestamp frequency.");
               if (!getparam(fd, I915_PARAM_REVISION, &devinfo->revision))
            if (!query_topology(devinfo, fd)) {
      if (devinfo->ver >= 10) {
      /* topology uAPI required for CNL+ (kernel 4.17+) */
               /* else use the kernel 4.13+ api for gfx8+.  For older kernels, topology
   * will be wrong, affecting GPU metrics. In this case, fail silently.
   */
               /* If the memory region uAPI query is not available, try to generate some
   * numbers out of os_* utils for sram only.
   */
   if (!intel_device_info_i915_query_regions(devinfo, fd, false))
            if (devinfo->platform == INTEL_PLATFORM_CHV)
            /* Broadwell PRM says:
   *
   *   "Before Gfx8, there was a historical configuration control field to
   *    swizzle address bit[6] for in X/Y tiling modes. This was set in three
   *    different places: TILECTL[1:0], ARB_MODE[5:4], and
   *    DISP_ARB_CTL[14:13].
   *
   *    For Gfx8 and subsequent generations, the swizzle fields are all
   *    reserved, and the CPU's memory controller performs all address
   *    swizzling modifications."
   */
            intel_get_aperture_size(fd, &devinfo->aperture_bytes);
   get_context_param(fd, 0, I915_CONTEXT_PARAM_GTT_SIZE, &devinfo->gtt_size);
   devinfo->has_tiling_uapi = has_get_tiling(fd);
   devinfo->has_caching_uapi =
         if (devinfo->ver > 12 || intel_device_info_is_mtl(devinfo))
            if (getparam(fd, I915_PARAM_MMAP_GTT_VERSION, &val))
         if (getparam(fd, I915_PARAM_HAS_USERPTR_PROBE, &val))
         if (getparam(fd, I915_PARAM_HAS_CONTEXT_ISOLATION, &val))
            /* TODO: We might be able to reduce alignment to 4Kb on DG1. */
   if (devinfo->verx10 >= 125)
         else if (devinfo->has_local_mem)
         else
               }
