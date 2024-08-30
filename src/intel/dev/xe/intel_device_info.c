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
      #include "xe/intel_device_info.h"
      #include "common/intel_gem.h"
   #include "dev/intel_device_info.h"
   #include "dev/intel_hwconfig.h"
      #include "util/log.h"
      #include "drm-uapi/xe_drm.h"
      static void *
   xe_query_alloc_fetch(int fd, uint32_t query_id, int32_t *len)
   {
      struct drm_xe_device_query query = {
         };
   if (intel_ioctl(fd, DRM_IOCTL_XE_DEVICE_QUERY, &query))
            void *data = calloc(1, query.size);
   if (!data)
            query.data = (uintptr_t)data;
   if (intel_ioctl(fd, DRM_IOCTL_XE_DEVICE_QUERY, &query))
            if (len)
               data_query_failed:
      free(data);
      }
      static bool
   xe_query_config(int fd, struct intel_device_info *devinfo)
   {
      struct drm_xe_query_config *config;
   config = xe_query_alloc_fetch(fd, DRM_XE_DEVICE_QUERY_CONFIG, NULL);
   if (!config)
            if (config->info[XE_QUERY_CONFIG_FLAGS] & XE_QUERY_CONFIG_FLAGS_HAS_VRAM)
            devinfo->revision = (config->info[XE_QUERY_CONFIG_REV_AND_DEVICE_ID] >> 16) & 0xFFFF;
   devinfo->gtt_size = 1ull << config->info[XE_QUERY_CONFIG_VA_BITS];
            free(config);
      }
      bool
   intel_device_info_xe_query_regions(int fd, struct intel_device_info *devinfo,
         {
      struct drm_xe_query_mem_usage *regions;
   regions = xe_query_alloc_fetch(fd, DRM_XE_DEVICE_QUERY_MEM_USAGE, NULL);
   if (!regions)
            for (int i = 0; i < regions->num_regions; i++) {
               switch (region->mem_class) {
   case XE_MEM_REGION_CLASS_SYSMEM: {
      if (!update) {
      devinfo->mem.sram.mem.klass = region->mem_class;
   devinfo->mem.sram.mem.instance = region->instance;
      } else {
      assert(devinfo->mem.sram.mem.klass == region->mem_class);
   assert(devinfo->mem.sram.mem.instance == region->instance);
      }
   devinfo->mem.sram.mappable.free = region->total_size - region->used;
      }
   case XE_MEM_REGION_CLASS_VRAM: {
      if (!update) {
      devinfo->mem.vram.mem.klass = region->mem_class;
   devinfo->mem.vram.mem.instance = region->instance;
   devinfo->mem.vram.mappable.size = region->cpu_visible_size;
      } else {
      assert(devinfo->mem.vram.mem.klass == region->mem_class);
   assert(devinfo->mem.vram.mem.instance == region->instance);
   assert(devinfo->mem.vram.mappable.size == region->cpu_visible_size);
      }
   devinfo->mem.vram.mappable.free = devinfo->mem.vram.mappable.size - region->cpu_visible_used;
   devinfo->mem.vram.unmappable.free = devinfo->mem.vram.unmappable.size - (region->used - region->cpu_visible_used);
      }
   default:
      mesa_loge("Unhandled Xe memory class");
                  devinfo->mem.use_class_instance = true;
   free(regions);
      }
      static bool
   xe_query_gts(int fd, struct intel_device_info *devinfo)
   {
      struct drm_xe_query_gt_list *gt_list;
   gt_list = xe_query_alloc_fetch(fd, DRM_XE_DEVICE_QUERY_GT_LIST, NULL);
   if (!gt_list)
            for (uint32_t i = 0; i < gt_list->num_gt; i++) {
      if (gt_list->gt_list[i].type == XE_QUERY_GT_TYPE_MAIN)
               free(gt_list);
      }
      void *
   intel_device_info_xe_query_hwconfig(int fd, int32_t *len)
   {
         }
      static bool
   xe_query_process_hwconfig(int fd, struct intel_device_info *devinfo)
   {
      int32_t len;
            if (!data)
            bool ret = intel_hwconfig_process_table(devinfo, data, len);
   free(data);
      }
      static void
   xe_compute_topology(struct intel_device_info * devinfo,
                     {
      intel_device_info_topology_reset_masks(devinfo);
   /* TGL/DG1/ADL-P: 1 slice x 6 dual sub slices
   * RKL/ADL-S: 1 slice x 2 dual sub slices
   * DG2: 8 slices x 4 dual sub slices
   */
   if (devinfo->verx10 >= 125) {
      devinfo->max_slices = 8;
      } else {
      devinfo->max_slices = 1;
      }
   devinfo->max_eus_per_subslice = 16;
   devinfo->subslice_slice_stride = 1;
   devinfo->eu_slice_stride = DIV_ROUND_UP(16 * 4, 8);
            assert((sizeof(uint32_t) * 8) >= devinfo->max_subslices_per_slice);
            const uint32_t dss_mask_in_slice = (1u << devinfo->max_subslices_per_slice) - 1;
   struct slice {
      uint32_t dss_mask;
   struct {
      bool enabled;
                  /* Compute and fill slices */
   for (unsigned s = 0; s < devinfo->max_slices; s++) {
      const unsigned first_bit = s * devinfo->max_subslices_per_slice;
   const unsigned dss_index = first_bit / 8;
                     const uint32_t *dss_mask_ptr = (const uint32_t *)&geo_dss_mask[dss_index];
   uint32_t dss_mask = *dss_mask_ptr;
   dss_mask >>= shift;
            if (dss_mask) {
      slices[s].dss_mask = dss_mask;
   for (uint32_t dss = 0; dss < devinfo->max_subslices_per_slice; dss++) {
      if ((1u << dss) & slices[s].dss_mask) {
      slices[s].dual_subslice[dss].enabled = true;
                        /* Set devinfo masks */
   for (unsigned s = 0; s < devinfo->max_slices; s++) {
      if (!slices[s].dss_mask)
                     for (unsigned ss = 0; ss < devinfo->max_subslices_per_slice; ss++) {
                                    for (unsigned eu = 0; eu < devinfo->max_eus_per_subslice; eu++) {
                     devinfo->eu_masks[s * devinfo->eu_slice_stride +
                              intel_device_info_topology_update_counts(devinfo);
   intel_device_info_update_pixel_pipes(devinfo, devinfo->subslice_masks);
      }
      static bool
   xe_query_topology(int fd, struct intel_device_info *devinfo)
   {
      struct drm_xe_query_topology_mask *topology;
   int32_t len;
   topology = xe_query_alloc_fetch(fd, DRM_XE_DEVICE_QUERY_GT_TOPOLOGY, &len);
   if (!topology)
            uint32_t geo_dss_num_bytes = 0, *eu_per_dss_mask = NULL;
   uint8_t *geo_dss_mask = NULL, *tmp;
            tmp = (uint8_t *)topology + len;
            while (topology < end) {
      if (topology->gt_id == 0) {
      switch (topology->type) {
   case XE_TOPO_DSS_GEOMETRY:
      geo_dss_mask = topology->mask;
   geo_dss_num_bytes = topology->num_bytes;
      case XE_TOPO_EU_PER_DSS:
      eu_per_dss_mask = (uint32_t *)topology->mask;
                              bool ret = true;
   if (!geo_dss_num_bytes || !geo_dss_mask || !eu_per_dss_mask) {
      ret = false;
                     parse_failed:
      free((void *)head);
      }
      bool
   intel_device_info_xe_get_info_from_fd(int fd, struct intel_device_info *devinfo)
   {
      if (!intel_device_info_xe_query_regions(fd, devinfo, false))
            if (!xe_query_config(fd, devinfo))
            if (!xe_query_gts(fd, devinfo))
            if (xe_query_process_hwconfig(fd, devinfo))
            if (!xe_query_topology(fd, devinfo))
            devinfo->has_context_isolation = true;
   devinfo->has_mmap_offset = true;
               }
