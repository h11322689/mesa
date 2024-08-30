   /*
   * Copyright © 2021 Collabora, Ltd.
   * Author: Antonio Caggiano <antonio.caggiano@collabora.com>
   *
   * Permission is hereby granted, free of charge, to any person obtaining a copy
   * of this software and associated documentation files (the "Software"), to deal
   * in the Software without restriction, including without limitation the rights
   * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   * copies of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included in
   * all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   * THE SOFTWARE.
   */
      #include "pan_perf.h"
      #include <drm-uapi/panfrost_drm.h>
   #include <lib/pan_device.h>
   #include <pan_perf_metrics.h>
      #define PAN_COUNTERS_PER_CATEGORY 64
   #define PAN_SHADER_CORE_INDEX     3
      uint32_t
   panfrost_perf_counter_read(const struct panfrost_perf_counter *counter,
         {
      unsigned offset = perf->category_offset[counter->category_index];
   offset += counter->offset;
                     // If counter belongs to shader core, accumulate values for all other cores
   if (counter->category_index == PAN_SHADER_CORE_INDEX) {
      for (uint32_t core = 1; core < perf->dev->core_id_range; ++core) {
                        }
      static const struct panfrost_perf_config *
   panfrost_lookup_counters(const char *name)
   {
      for (unsigned i = 0; i < ARRAY_SIZE(panfrost_perf_configs); ++i) {
      if (strcmp(panfrost_perf_configs[i]->name, name) == 0)
                  }
      void
   panfrost_perf_init(struct panfrost_perf *perf, struct panfrost_device *dev)
   {
               if (dev->model == NULL)
                     if (perf->cfg == NULL)
            // Generally counter blocks are laid out in the following order:
   // Job manager, tiler, one or more L2 caches, and one or more shader cores.
   unsigned l2_slices = panfrost_query_l2_slices(dev);
   uint32_t n_blocks = 2 + l2_slices + dev->core_id_range;
   perf->n_counter_values = PAN_COUNTERS_PER_CATEGORY * n_blocks;
            /* Setup the layout */
   perf->category_offset[0] = PAN_COUNTERS_PER_CATEGORY * 0;
   perf->category_offset[1] = PAN_COUNTERS_PER_CATEGORY * 1;
   perf->category_offset[2] = PAN_COUNTERS_PER_CATEGORY * 2;
      }
      static int
   panfrost_perf_query(struct panfrost_perf *perf, uint32_t enable)
   {
      struct drm_panfrost_perfcnt_enable perfcnt_enable = {enable, 0};
   return drmIoctl(perf->dev->fd, DRM_IOCTL_PANFROST_PERFCNT_ENABLE,
      }
      int
   panfrost_perf_enable(struct panfrost_perf *perf)
   {
         }
      int
   panfrost_perf_disable(struct panfrost_perf *perf)
   {
         }
      int
   panfrost_perf_dump(struct panfrost_perf *perf)
   {
      // Dump performance counter values to the memory buffer pointed to by
   // counter_values
   struct drm_panfrost_perfcnt_dump perfcnt_dump = {
         return drmIoctl(perf->dev->fd, DRM_IOCTL_PANFROST_PERFCNT_DUMP,
      }
