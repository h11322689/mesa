   /*
   * Copyright © 2018 Intel Corporation
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
      #include <dirent.h>
      #include <sys/types.h>
   #include <sys/stat.h>
   #include <fcntl.h>
   #include <unistd.h>
   #include <errno.h>
      #ifndef HAVE_DIRENT_D_TYPE
   #include <limits.h> // PATH_MAX
   #endif
      #include <drm-uapi/i915_drm.h>
      #include "common/intel_gem.h"
   #include "common/i915/intel_gem.h"
      #include "dev/intel_debug.h"
   #include "dev/intel_device_info.h"
      #include "perf/intel_perf.h"
   #include "perf/intel_perf_regs.h"
   #include "perf/intel_perf_mdapi.h"
   #include "perf/intel_perf_metrics.h"
   #include "perf/intel_perf_private.h"
      #include "util/bitscan.h"
   #include "util/macros.h"
   #include "util/mesa-sha1.h"
   #include "util/u_debug.h"
   #include "util/u_math.h"
      #define FILE_DEBUG_FLAG DEBUG_PERFMON
      static bool
   is_dir_or_link(const struct dirent *entry, const char *parent_dir)
   {
   #ifdef HAVE_DIRENT_D_TYPE
         #else
      struct stat st;
   char path[PATH_MAX + 1];
   snprintf(path, sizeof(path), "%s/%s", parent_dir, entry->d_name);
   lstat(path, &st);
      #endif
   }
      static bool
   get_sysfs_dev_dir(struct intel_perf_config *perf, int fd)
   {
      struct stat sb;
   int min, maj;
   DIR *drmdir;
   struct dirent *drm_entry;
                     if (INTEL_DEBUG(DEBUG_NO_OACONFIG))
            if (fstat(fd, &sb)) {
      DBG("Failed to stat DRM fd\n");
               maj = major(sb.st_rdev);
            if (!S_ISCHR(sb.st_mode)) {
      DBG("DRM fd is not a character device as expected\n");
               len = snprintf(perf->sysfs_dev_dir,
               if (len < 0 || len >= sizeof(perf->sysfs_dev_dir)) {
      DBG("Failed to concatenate sysfs path to drm device\n");
               drmdir = opendir(perf->sysfs_dev_dir);
   if (!drmdir) {
      DBG("Failed to open %s: %m\n", perf->sysfs_dev_dir);
               while ((drm_entry = readdir(drmdir))) {
      if (is_dir_or_link(drm_entry, perf->sysfs_dev_dir) &&
         {
      len = snprintf(perf->sysfs_dev_dir,
                     closedir(drmdir);
   if (len < 0 || len >= sizeof(perf->sysfs_dev_dir))
         else
                           DBG("Failed to find cardX directory under /sys/dev/char/%d:%d/device/drm\n",
               }
      static bool
   read_file_uint64(const char *file, uint64_t *val)
   {
      char buf[32];
            fd = open(file, 0);
   if (fd < 0)
         while ((n = read(fd, buf, sizeof (buf) - 1)) < 0 &&
         close(fd);
   if (n < 0)
            buf[n] = '\0';
               }
      static bool
   read_sysfs_drm_device_file_uint64(struct intel_perf_config *perf,
               {
      char buf[512];
            len = snprintf(buf, sizeof(buf), "%s/%s", perf->sysfs_dev_dir, file);
   if (len < 0 || len >= sizeof(buf)) {
      DBG("Failed to concatenate sys filename to read u64 from\n");
                  }
      static bool
   oa_config_enabled(struct intel_perf_config *perf,
            // Hide extended metrics unless enabled with env param
               }
      static void
   register_oa_config(struct intel_perf_config *perf,
                     {
      if (!oa_config_enabled(perf, query))
            struct intel_perf_query_info *registered_query =
            *registered_query = *query;
   registered_query->oa_metrics_set_id = config_id;
   DBG("metric set registered: id = %" PRIu64", guid = %s\n",
      }
      static void
   enumerate_sysfs_metrics(struct intel_perf_config *perf,
         {
      DIR *metricsdir = NULL;
   struct dirent *metric_entry;
   char buf[256];
            len = snprintf(buf, sizeof(buf), "%s/metrics", perf->sysfs_dev_dir);
   if (len < 0 || len >= sizeof(buf)) {
      DBG("Failed to concatenate path to sysfs metrics/ directory\n");
               metricsdir = opendir(buf);
   if (!metricsdir) {
      DBG("Failed to open %s: %m\n", buf);
               while ((metric_entry = readdir(metricsdir))) {
      struct hash_entry *entry;
   if (!is_dir_or_link(metric_entry, buf) ||
                  DBG("metric set: %s\n", metric_entry->d_name);
   entry = _mesa_hash_table_search(perf->oa_metrics_table,
         if (entry) {
      uint64_t id;
   if (!intel_perf_load_metric_id(perf, metric_entry->d_name, &id)) {
      DBG("Failed to read metric set id from %s: %m", buf);
               register_oa_config(perf, devinfo,
      } else
                  }
      static void
   add_all_metrics(struct intel_perf_config *perf,
         {
      hash_table_foreach(perf->oa_metrics_table, entry) {
      const struct intel_perf_query_info *query = entry->data;
         }
      static bool
   kernel_has_dynamic_config_support(struct intel_perf_config *perf, int fd)
   {
               return intel_ioctl(fd, DRM_IOCTL_I915_PERF_REMOVE_CONFIG,
      }
      static bool
   i915_query_perf_config_supported(struct intel_perf_config *perf, int fd)
   {
      int32_t length = 0;
   return !intel_i915_query_flags(fd, DRM_I915_QUERY_PERF_CONFIG,
            }
      static bool
   i915_query_perf_config_data(struct intel_perf_config *perf,
               {
      char data[sizeof(struct drm_i915_query_perf_config) +
         struct drm_i915_query_perf_config *i915_query = (void *)data;
            memcpy(i915_query->uuid, guid, sizeof(i915_query->uuid));
            int32_t item_length = sizeof(data);
   if (intel_i915_query_flags(fd, DRM_I915_QUERY_PERF_CONFIG,
                                    }
      bool
   intel_perf_load_metric_id(struct intel_perf_config *perf_cfg,
               {
               snprintf(config_path, sizeof(config_path), "%s/metrics/%s/id",
            /* Don't recreate already loaded configs. */
      }
      static uint64_t
   i915_add_config(struct intel_perf_config *perf, int fd,
               {
                        i915_config.n_mux_regs = config->n_mux_regs;
            i915_config.n_boolean_regs = config->n_b_counter_regs;
            i915_config.n_flex_regs = config->n_flex_regs;
            int ret = intel_ioctl(fd, DRM_IOCTL_I915_PERF_ADD_CONFIG, &i915_config);
      }
      static void
   init_oa_configs(struct intel_perf_config *perf, int fd,
         {
      hash_table_foreach(perf->oa_metrics_table, entry) {
      const struct intel_perf_query_info *query = entry->data;
            if (intel_perf_load_metric_id(perf, query->guid, &config_id)) {
      DBG("metric set: %s (already loaded)\n", query->guid);
   register_oa_config(perf, devinfo, query, config_id);
               int ret = i915_add_config(perf, fd, &query->config, query->guid);
   if (ret < 0) {
      DBG("Failed to load \"%s\" (%s) metrics set in kernel: %s\n",
                     register_oa_config(perf, devinfo, query, ret);
         }
      static void
   compute_topology_builtins(struct intel_perf_config *perf)
   {
               perf->sys_vars.slice_mask = devinfo->slice_masks;
            perf->sys_vars.n_eu_slice0123 = 0;
   for (int s = 0; s < MIN2(4, devinfo->max_slices); s++) {
      if (!intel_device_info_slice_available(devinfo, s))
            for (int ss = 0; ss < devinfo->max_subslices_per_slice; ss++) {
                     for (int eu = 0; eu < devinfo->max_eus_per_subslice; eu++) {
      if (intel_device_info_eu_available(devinfo, s, ss, eu))
                     perf->sys_vars.n_eu_sub_slices = intel_device_info_subslice_total(devinfo);
            /* The subslice mask builtin contains bits for all slices. Prior to Gfx11
   * it had groups of 3bits for each slice, on Gfx11 and above it's 8bits for
   * each slice.
   *
   * Ideally equations would be updated to have a slice/subslice query
   * function/operator.
   */
                     for (int s = 0; s < util_last_bit(devinfo->slice_masks); s++) {
      for (int ss = 0; ss < (devinfo->subslice_slice_stride * 8); ss++) {
      if (intel_device_info_subslice_available(devinfo, s, ss))
            }
      static bool
   init_oa_sys_vars(struct intel_perf_config *perf,
         {
               if (!INTEL_DEBUG(DEBUG_NO_OACONFIG)) {
      if (!read_sysfs_drm_device_file_uint64(perf, "gt_min_freq_mhz", &min_freq_mhz))
            if (!read_sysfs_drm_device_file_uint64(perf,  "gt_max_freq_mhz", &max_freq_mhz))
      } else {
      min_freq_mhz = 300;
               memset(&perf->sys_vars, 0, sizeof(perf->sys_vars));
   perf->sys_vars.gt_min_freq = min_freq_mhz * 1000000;
   perf->sys_vars.gt_max_freq = max_freq_mhz * 1000000;
   perf->sys_vars.query_mode = use_register_snapshots;
               }
      typedef void (*perf_register_oa_queries_t)(struct intel_perf_config *);
      static perf_register_oa_queries_t
   get_register_queries_function(const struct intel_device_info *devinfo)
   {
      switch (devinfo->platform) {
   case INTEL_PLATFORM_HSW:
         case INTEL_PLATFORM_CHV:
         case INTEL_PLATFORM_BDW:
         case INTEL_PLATFORM_BXT:
         case INTEL_PLATFORM_SKL:
      if (devinfo->gt == 2)
         if (devinfo->gt == 3)
         if (devinfo->gt == 4)
            case INTEL_PLATFORM_KBL:
      if (devinfo->gt == 2)
         if (devinfo->gt == 3)
            case INTEL_PLATFORM_GLK:
         case INTEL_PLATFORM_CFL:
      if (devinfo->gt == 2)
         if (devinfo->gt == 3)
            case INTEL_PLATFORM_ICL:
         case INTEL_PLATFORM_EHL:
         case INTEL_PLATFORM_TGL:
      if (devinfo->gt == 1)
         if (devinfo->gt == 2)
            case INTEL_PLATFORM_RKL:
         case INTEL_PLATFORM_DG1:
         case INTEL_PLATFORM_ADL:
   case INTEL_PLATFORM_RPL:
         case INTEL_PLATFORM_DG2_G10:
         case INTEL_PLATFORM_DG2_G11:
         case INTEL_PLATFORM_DG2_G12:
         case INTEL_PLATFORM_MTL_M:
   case INTEL_PLATFORM_MTL_P:
      if (intel_device_info_eu_total(devinfo) <= 64)
         if (intel_device_info_eu_total(devinfo) <= 128)
            default:
            }
      static int
   intel_perf_compare_counter_names(const void *v1, const void *v2)
   {
      const struct intel_perf_query_counter *c1 = v1;
               }
      static void
   sort_query(struct intel_perf_query_info *q)
   {
      qsort(q->counters, q->n_counters, sizeof(q->counters[0]),
      }
      static void
   load_pipeline_statistic_metrics(struct intel_perf_config *perf_cfg,
         {
      struct intel_perf_query_info *query =
            query->kind = INTEL_PERF_QUERY_TYPE_PIPELINE;
            intel_perf_query_add_basic_stat_reg(query, IA_VERTICES_COUNT,
         intel_perf_query_add_basic_stat_reg(query, IA_PRIMITIVES_COUNT,
         intel_perf_query_add_basic_stat_reg(query, VS_INVOCATION_COUNT,
            if (devinfo->ver == 6) {
      intel_perf_query_add_stat_reg(query, GFX6_SO_PRIM_STORAGE_NEEDED, 1, 1,
               intel_perf_query_add_stat_reg(query, GFX6_SO_NUM_PRIMS_WRITTEN, 1, 1,
            } else {
      intel_perf_query_add_stat_reg(query, GFX7_SO_PRIM_STORAGE_NEEDED(0), 1, 1,
               intel_perf_query_add_stat_reg(query, GFX7_SO_PRIM_STORAGE_NEEDED(1), 1, 1,
               intel_perf_query_add_stat_reg(query, GFX7_SO_PRIM_STORAGE_NEEDED(2), 1, 1,
               intel_perf_query_add_stat_reg(query, GFX7_SO_PRIM_STORAGE_NEEDED(3), 1, 1,
               intel_perf_query_add_stat_reg(query, GFX7_SO_NUM_PRIMS_WRITTEN(0), 1, 1,
               intel_perf_query_add_stat_reg(query, GFX7_SO_NUM_PRIMS_WRITTEN(1), 1, 1,
               intel_perf_query_add_stat_reg(query, GFX7_SO_NUM_PRIMS_WRITTEN(2), 1, 1,
               intel_perf_query_add_stat_reg(query, GFX7_SO_NUM_PRIMS_WRITTEN(3), 1, 1,
                     intel_perf_query_add_basic_stat_reg(query, HS_INVOCATION_COUNT,
         intel_perf_query_add_basic_stat_reg(query, DS_INVOCATION_COUNT,
            intel_perf_query_add_basic_stat_reg(query, GS_INVOCATION_COUNT,
         intel_perf_query_add_basic_stat_reg(query, GS_PRIMITIVES_COUNT,
            intel_perf_query_add_basic_stat_reg(query, CL_INVOCATION_COUNT,
         intel_perf_query_add_basic_stat_reg(query, CL_PRIMITIVES_COUNT,
            if (devinfo->verx10 == 75 || devinfo->ver == 8) {
      intel_perf_query_add_stat_reg(query, PS_INVOCATION_COUNT, 1, 4,
            } else {
      intel_perf_query_add_basic_stat_reg(query, PS_INVOCATION_COUNT,
               intel_perf_query_add_basic_stat_reg(query, PS_DEPTH_COUNT,
            if (devinfo->ver >= 7) {
      intel_perf_query_add_basic_stat_reg(query, CS_INVOCATION_COUNT,
                           }
      static int
   i915_perf_version(int drm_fd)
   {
      int tmp = 0;
   intel_gem_get_param(drm_fd, I915_PARAM_PERF_REVISION, &tmp);
      }
      static void
   i915_get_sseu(int drm_fd, struct drm_i915_gem_context_param_sseu *sseu)
   {
      struct drm_i915_gem_context_param arg = {
      .param = I915_CONTEXT_PARAM_SSEU,
   .size = sizeof(*sseu),
                  }
      static inline int
   compare_str_or_null(const char *s1, const char *s2)
   {
      if (s1 == NULL && s2 == NULL)
         if (s1 == NULL)
         if (s2 == NULL)
               }
      static int
   compare_counter_categories_and_names(const void *_c1, const void *_c2)
   {
      const struct intel_perf_query_counter_info *c1 = (const struct intel_perf_query_counter_info *)_c1;
            /* pipeline counters don't have an assigned category */
   int r = compare_str_or_null(c1->counter->category, c2->counter->category);
   if (r)
               }
      static void
   build_unique_counter_list(struct intel_perf_config *perf)
   {
               for (int q = 0; q < perf->n_queries; q++)
            /*
   * Allocate big enough array to hold maximum possible number of counters.
   * We can't alloc it small and realloc when needed because the hash table
   * below contains pointers to this array.
   */
   struct intel_perf_query_counter_info *counter_infos =
                     struct hash_table *counters_table =
      _mesa_hash_table_create(NULL,
            struct hash_entry *entry;
   for (int q = 0; q < perf->n_queries ; q++) {
               for (int c = 0; c < query->n_counters; c++) {
                                    if (entry) {
      counter_info = entry->data;
   BITSET_SET(counter_info->query_mask, q);
                     counter_info = &counter_infos[perf->n_counters++];
                                                                  qsort(perf->counter_infos, perf->n_counters, sizeof(perf->counter_infos[0]),
      }
      static bool
   oa_metrics_available(struct intel_perf_config *perf, int fd,
               {
      perf_register_oa_queries_t oa_register = get_register_queries_function(devinfo);
   bool i915_perf_oa_available = false;
            /* TODO: Xe still don't have support for performance metrics */
   if (devinfo->kmd_type != INTEL_KMD_TYPE_I915)
                     /* Consider an invalid as supported. */
   if (fd == -1) {
      perf->i915_query_supported = true;
               perf->i915_query_supported = i915_query_perf_config_supported(perf, fd);
   perf->enable_all_metrics = debug_get_bool_option("INTEL_EXTENDED_METRICS", false);
            /* TODO: We should query this from i915 */
   if (devinfo->verx10 >= 125)
            perf->oa_timestamp_mask =
            /* Record the default SSEU configuration. */
            /* The existence of this sysctl parameter implies the kernel supports
   * the i915 perf interface.
   */
               /* If _paranoid == 1 then on Gfx8+ we won't be able to access OA
   * metrics unless running as root.
   */
   if (devinfo->platform == INTEL_PLATFORM_HSW)
         else {
                        if (paranoid == 0 || geteuid() == 0)
                           return i915_perf_oa_available &&
         oa_register &&
      }
      static void
   load_oa_metrics(struct intel_perf_config *perf, int fd,
         {
                        perf->oa_metrics_table =
      _mesa_hash_table_create(perf, _mesa_hash_string,
         /* Index all the metric sets mesa knows about before looking to see what
   * the kernel is advertising.
   */
            if (!INTEL_DEBUG(DEBUG_NO_OACONFIG)) {
      if (kernel_has_dynamic_config_support(perf, fd))
         else
      } else {
                  /* sort counters in each individual group created by this function by name */
   for (int i = existing_queries; i < perf->n_queries; ++i)
            /* Select a fallback OA metric. Look for the TestOa metric or use the last
   * one if no present (on HSW).
   */
   for (int i = existing_queries; i < perf->n_queries; i++) {
      if (perf->queries[i].symbol_name &&
      strcmp(perf->queries[i].symbol_name, "TestOa") == 0) {
   perf->fallback_raw_oa_metric = perf->queries[i].oa_metrics_set_id;
         }
   if (perf->fallback_raw_oa_metric == 0 && perf->n_queries > 0)
      }
      struct intel_perf_registers *
   intel_perf_load_configuration(struct intel_perf_config *perf_cfg, int fd, const char *guid)
   {
      if (!perf_cfg->i915_query_supported)
            struct drm_i915_perf_oa_config i915_config = { 0, };
   if (!i915_query_perf_config_data(perf_cfg, fd, guid, &i915_config))
            struct intel_perf_registers *config = rzalloc(NULL, struct intel_perf_registers);
   config->n_flex_regs = i915_config.n_flex_regs;
   config->flex_regs = rzalloc_array(config, struct intel_perf_query_register_prog, config->n_flex_regs);
   config->n_mux_regs = i915_config.n_mux_regs;
   config->mux_regs = rzalloc_array(config, struct intel_perf_query_register_prog, config->n_mux_regs);
   config->n_b_counter_regs = i915_config.n_boolean_regs;
            /*
   * struct intel_perf_query_register_prog maps exactly to the tuple of
   * (register offset, register value) returned by the i915.
   */
   i915_config.flex_regs_ptr = to_const_user_pointer(config->flex_regs);
   i915_config.mux_regs_ptr = to_const_user_pointer(config->mux_regs);
   i915_config.boolean_regs_ptr = to_const_user_pointer(config->b_counter_regs);
   if (!i915_query_perf_config_data(perf_cfg, fd, guid, &i915_config)) {
      ralloc_free(config);
                  }
      uint64_t
   intel_perf_store_configuration(struct intel_perf_config *perf_cfg, int fd,
               {
      if (guid)
            struct mesa_sha1 sha1_ctx;
            if (config->flex_regs) {
      _mesa_sha1_update(&sha1_ctx, config->flex_regs,
            }
   if (config->mux_regs) {
      _mesa_sha1_update(&sha1_ctx, config->mux_regs,
            }
   if (config->b_counter_regs) {
      _mesa_sha1_update(&sha1_ctx, config->b_counter_regs,
                     uint8_t hash[20];
            char formatted_hash[41];
            char generated_guid[37];
   snprintf(generated_guid, sizeof(generated_guid),
            "%.8s-%.4s-%.4s-%.4s-%.12s",
   &formatted_hash[0], &formatted_hash[8],
         /* Check if already present. */
   uint64_t id;
   if (intel_perf_load_metric_id(perf_cfg, generated_guid, &id))
               }
      static void
   get_passes_mask(struct intel_perf_config *perf,
                     {
      /* For each counter, look if it's already computed by a selected metric set
   * or find one that can compute it.
   */
   for (uint32_t c = 0; c < counter_indices_count; c++) {
      uint32_t counter_idx = counter_indices[c];
            const struct intel_perf_query_counter_info *counter_info =
            /* Check if the counter is already computed by one of the selected
   * metric set. If it is, there is nothing more to do with this counter.
   */
   uint32_t match = UINT32_MAX;
   for (uint32_t w = 0; w < BITSET_WORDS(INTEL_PERF_MAX_METRIC_SETS); w++) {
      if (queries_mask[w] & counter_info->query_mask[w]) {
      match = w * BITSET_WORDBITS + ffsll(queries_mask[w] & counter_info->query_mask[w]) - 1;
         }
   if (match != UINT32_MAX)
            /* Now go through each metric set and find one that contains this
   * counter.
   */
   bool found = false;
   for (uint32_t w = 0; w < BITSET_WORDS(INTEL_PERF_MAX_METRIC_SETS); w++) {
                              /* Since we already looked for this in the query_mask, it should not
   * be set.
                  BITSET_SET(queries_mask, query_idx);
   found = true;
      }
         }
      uint32_t
   intel_perf_get_n_passes(struct intel_perf_config *perf,
                     {
      BITSET_DECLARE(queries_mask, INTEL_PERF_MAX_METRIC_SETS);
                     if (pass_queries) {
      uint32_t pass = 0;
   for (uint32_t q = 0; q < perf->n_queries; q++) {
      if (BITSET_TEST(queries_mask, q))
                     }
      void
   intel_perf_get_counters_passes(struct intel_perf_config *perf,
                     {
      BITSET_DECLARE(queries_mask, INTEL_PERF_MAX_METRIC_SETS);
            get_passes_mask(perf, counter_indices, counter_indices_count, queries_mask);
            struct intel_perf_query_info **pass_array = calloc(perf->n_queries,
                  for (uint32_t i = 0; i < counter_indices_count; i++) {
               uint32_t counter_idx = counter_indices[i];
            const struct intel_perf_query_counter_info *counter_info =
            uint32_t query_idx = UINT32_MAX;
   for (uint32_t w = 0; w < BITSET_WORDS(INTEL_PERF_MAX_METRIC_SETS); w++) {
      if (counter_info->query_mask[w] & queries_mask[w]) {
      query_idx = w * BITSET_WORDBITS +
               }
                     uint32_t pass_idx = UINT32_MAX;
   for (uint32_t p = 0; p < n_written_passes; p++) {
      if (pass_array[p] == counter_pass[i].query) {
      pass_idx = p;
                  if (pass_idx == UINT32_MAX)
                           }
      /* Accumulate 32bits OA counters */
   static inline void
   accumulate_uint32(const uint32_t *report0,
               {
         }
      /* Accumulate 40bits OA counters */
   static inline void
   accumulate_uint40(int a_index,
                     {
      const uint8_t *high_bytes0 = (uint8_t *)(report0 + 40);
   const uint8_t *high_bytes1 = (uint8_t *)(report1 + 40);
   uint64_t high0 = (uint64_t)(high_bytes0[a_index]) << 32;
   uint64_t high1 = (uint64_t)(high_bytes1[a_index]) << 32;
   uint64_t value0 = report0[a_index + 4] | high0;
   uint64_t value1 = report1[a_index + 4] | high1;
            if (value0 > value1)
         else
               }
      static void
   gfx8_read_report_clock_ratios(const uint32_t *report,
               {
      /* The lower 16bits of the RPT_ID field of the OA reports contains a
   * snapshot of the bits coming from the RP_FREQ_NORMAL register and is
   * divided this way :
   *
   * RPT_ID[31:25]: RP_FREQ_NORMAL[20:14] (low squashed_slice_clock_frequency)
   * RPT_ID[10:9]:  RP_FREQ_NORMAL[22:21] (high squashed_slice_clock_frequency)
   * RPT_ID[8:0]:   RP_FREQ_NORMAL[31:23] (squashed_unslice_clock_frequency)
   *
   * RP_FREQ_NORMAL[31:23]: Software Unslice Ratio Request
   *                        Multiple of 33.33MHz 2xclk (16 MHz 1xclk)
   *
   * RP_FREQ_NORMAL[22:14]: Software Slice Ratio Request
   *                        Multiple of 33.33MHz 2xclk (16 MHz 1xclk)
            uint32_t unslice_freq = report[0] & 0x1ff;
   uint32_t slice_freq_low = (report[0] >> 25) & 0x7f;
   uint32_t slice_freq_high = (report[0] >> 9) & 0x3;
            *slice_freq_hz = slice_freq * 16666667ULL;
      }
      void
   intel_perf_query_result_read_frequencies(struct intel_perf_query_result *result,
                     {
      /* Slice/Unslice frequency is only available in the OA reports when the
   * "Disable OA reports due to clock ratio change" field in
   * OA_DEBUG_REGISTER is set to 1. This is how the kernel programs this
   * global register (see drivers/gpu/drm/i915/i915_perf.c)
   *
   * Documentation says this should be available on Gfx9+ but experimentation
   * shows that Gfx8 reports similar values, so we enable it there too.
   */
   if (devinfo->ver < 8)
            gfx8_read_report_clock_ratios(start,
               gfx8_read_report_clock_ratios(end,
            }
      static inline bool
   can_use_mi_rpc_bc_counters(const struct intel_device_info *devinfo)
   {
         }
      uint64_t
   intel_perf_report_timestamp(const struct intel_perf_query_info *query,
         {
         }
      void
   intel_perf_query_result_accumulate(struct intel_perf_query_result *result,
                     {
               if (result->hw_id == INTEL_PERF_INVALID_CTX_ID &&
      start[2] != INTEL_PERF_INVALID_CTX_ID)
      if (result->reports_accumulated == 0)
         result->end_timestamp = intel_perf_report_timestamp(query, end);
            switch (query->oa_format) {
   case I915_OA_FORMAT_A24u40_A14u32_B8_C8:
      result->accumulator[query->gpu_time_offset] =
                  accumulate_uint32(start + 3, end + 3,
            /* A0-A3 counters are 32bits */
   for (i = 0; i < 4; i++) {
      accumulate_uint32(start + 4 + i, end + 4 + i,
               /* A4-A23 counters are 40bits */
   for (i = 4; i < 24; i++) {
      accumulate_uint40(i, start, end,
               /* A24-27 counters are 32bits */
   for (i = 0; i < 4; i++) {
      accumulate_uint32(start + 28 + i, end + 28 + i,
               /* A28-31 counters are 40bits */
   for (i = 28; i < 32; i++) {
      accumulate_uint40(i, start, end,
               /* A32-35 counters are 32bits */
   for (i = 0; i < 4; i++) {
      accumulate_uint32(start + 36 + i, end + 36 + i,
               if (can_use_mi_rpc_bc_counters(&query->perf->devinfo) ||
      !query->perf->sys_vars.query_mode) {
   /* A36-37 counters are 32bits */
   accumulate_uint32(start + 40, end + 40,
                        /* 8x 32bit B counters */
   for (i = 0; i < 8; i++) {
      accumulate_uint32(start + 48 + i, end + 48 + i,
               /* 8x 32bit C counters... */
   for (i = 0; i < 8; i++) {
      accumulate_uint32(start + 56 + i, end + 56 + i,
         }
         case I915_OA_FORMAT_A32u40_A4u32_B8_C8:
      result->accumulator[query->gpu_time_offset] =
                  accumulate_uint32(start + 3, end + 3,
            /* 32x 40bit A counters... */
   for (i = 0; i < 32; i++) {
      accumulate_uint40(i, start, end,
               /* 4x 32bit A counters... */
   for (i = 0; i < 4; i++) {
      accumulate_uint32(start + 36 + i, end + 36 + i,
               if (can_use_mi_rpc_bc_counters(&query->perf->devinfo) ||
      !query->perf->sys_vars.query_mode) {
   /* 8x 32bit B counters */
   for (i = 0; i < 8; i++) {
      accumulate_uint32(start + 48 + i, end + 48 + i,
               /* 8x 32bit C counters... */
   for (i = 0; i < 8; i++) {
      accumulate_uint32(start + 56 + i, end + 56 + i,
         }
         case I915_OA_FORMAT_A45_B8_C8:
      result->accumulator[query->gpu_time_offset] =
                  for (i = 0; i < 61; i++) {
      accumulate_uint32(start + 3 + i, end + 3 + i,
      }
         default:
               }
      #define GET_FIELD(word, field) (((word)  & field ## _MASK) >> field ## _SHIFT)
      void
   intel_perf_query_result_read_gt_frequency(struct intel_perf_query_result *result,
                     {
      switch (devinfo->ver) {
   case 7:
   case 8:
      result->gt_frequency[0] = GET_FIELD(start, GFX7_RPSTAT1_CURR_GT_FREQ) * 50ULL;
   result->gt_frequency[1] = GET_FIELD(end, GFX7_RPSTAT1_CURR_GT_FREQ) * 50ULL;
      case 9:
   case 11:
   case 12:
      result->gt_frequency[0] = GET_FIELD(start, GFX9_RPSTAT0_CURR_GT_FREQ) * 50ULL / 3ULL;
   result->gt_frequency[1] = GET_FIELD(end, GFX9_RPSTAT0_CURR_GT_FREQ) * 50ULL / 3ULL;
      default:
                  /* Put the numbers into Hz. */
   result->gt_frequency[0] *= 1000000ULL;
      }
      void
   intel_perf_query_result_read_perfcnts(struct intel_perf_query_result *result,
                     {
      for (uint32_t i = 0; i < 2; i++) {
      uint64_t v0 = start[i] & PERF_CNT_VALUE_MASK;
            result->accumulator[query->perfcnt_offset + i] = v0 > v1 ?
      (PERF_CNT_VALUE_MASK + 1 + v1 - v0) :
      }
      static uint32_t
   query_accumulator_offset(const struct intel_perf_query_info *query,
               {
      switch (type) {
   case INTEL_PERF_QUERY_FIELD_TYPE_SRM_PERFCNT:
         case INTEL_PERF_QUERY_FIELD_TYPE_SRM_OA_A:
         case INTEL_PERF_QUERY_FIELD_TYPE_SRM_OA_B:
         case INTEL_PERF_QUERY_FIELD_TYPE_SRM_OA_C:
         default:
      unreachable("Invalid register type");
         }
      void
   intel_perf_query_result_accumulate_fields(struct intel_perf_query_result *result,
                           {
      const struct intel_perf_query_field_layout *layout = &query->perf->query_layout;
            for (uint32_t r = 0; r < layout->n_fields; r++) {
               if (field->type == INTEL_PERF_QUERY_FIELD_TYPE_MI_RPC) {
      intel_perf_query_result_read_frequencies(result, devinfo,
               /* no_oa_accumulate=true is used when doing GL perf queries, we
   * manually parse the OA reports from the OA buffer and subtract
   * unrelated deltas, so don't accumulate the begin/end reports here.
   */
   if (!no_oa_accumulate) {
      intel_perf_query_result_accumulate(result, query,
               } else {
               if (field->size == 4) {
      v0 = *(const uint32_t *)(start + field->location);
      } else {
      assert(field->size == 8);
   v0 = *(const uint64_t *)(start + field->location);
               if (field->mask) {
      v0 = field->mask & v0;
               /* RPSTAT is a bit of a special case because its begin/end values
   * represent frequencies. We store it in a separate location.
   */
   if (field->type == INTEL_PERF_QUERY_FIELD_TYPE_SRM_RPSTAT)
         else
            }
      void
   intel_perf_query_result_clear(struct intel_perf_query_result *result)
   {
      memset(result, 0, sizeof(*result));
      }
      void
   intel_perf_query_result_print_fields(const struct intel_perf_query_info *query,
         {
               for (uint32_t r = 0; r < layout->n_fields; r++) {
      const struct intel_perf_query_field *field = &layout->fields[r];
            switch (field->type) {
   case INTEL_PERF_QUERY_FIELD_TYPE_MI_RPC:
      fprintf(stderr, "MI_RPC:\n");
   fprintf(stderr, "  TS: 0x%08x\n", *(value32 + 1));
   fprintf(stderr, "  CLK: 0x%08x\n", *(value32 + 3));
      case INTEL_PERF_QUERY_FIELD_TYPE_SRM_OA_A:
      fprintf(stderr, "A%u: 0x%08x\n", field->index, *value32);
      case INTEL_PERF_QUERY_FIELD_TYPE_SRM_OA_B:
      fprintf(stderr, "B%u: 0x%08x\n", field->index, *value32);
      case INTEL_PERF_QUERY_FIELD_TYPE_SRM_OA_C:
      fprintf(stderr, "C%u: 0x%08x\n", field->index, *value32);
      default:
               }
      static int
   intel_perf_compare_query_names(const void *v1, const void *v2)
   {
      const struct intel_perf_query_info *q1 = v1;
               }
      static inline struct intel_perf_query_field *
   add_query_register(struct intel_perf_query_field_layout *layout,
                     enum intel_perf_query_field_type type,
   {
      /* Align MI_RPC to 64bytes (HW requirement) & 64bit registers to 8bytes
   * (shows up nicely in the debugger).
   */
   if (type == INTEL_PERF_QUERY_FIELD_TYPE_MI_RPC)
         else if (size % 8 == 0)
            layout->fields[layout->n_fields++] = (struct intel_perf_query_field) {
      .mmio_offset = offset,
   .location = layout->size,
   .type = type,
   .index = index,
      };
               }
      static void
   intel_perf_init_query_fields(struct intel_perf_config *perf_cfg,
               {
                        /* MI_RPC requires a 64byte alignment. */
                     add_query_register(layout, INTEL_PERF_QUERY_FIELD_TYPE_MI_RPC,
            if (use_register_snapshots) {
      if (devinfo->ver <= 11) {
      struct intel_perf_query_field *field =
      add_query_register(layout,
                     field = add_query_register(layout,
                           if (devinfo->ver == 8 && devinfo->platform != INTEL_PLATFORM_CHV) {
      add_query_register(layout,
                     if (devinfo->ver >= 9) {
      add_query_register(layout,
                     if (!can_use_mi_rpc_bc_counters(devinfo)) {
      if (devinfo->ver >= 8 && devinfo->ver <= 11) {
      for (uint32_t i = 0; i < GFX8_N_OA_PERF_B32; i++) {
      add_query_register(layout, INTEL_PERF_QUERY_FIELD_TYPE_SRM_OA_B,
      }
   for (uint32_t i = 0; i < GFX8_N_OA_PERF_C32; i++) {
      add_query_register(layout, INTEL_PERF_QUERY_FIELD_TYPE_SRM_OA_C,
         } else if (devinfo->verx10 == 120) {
      for (uint32_t i = 0; i < GFX12_N_OAG_PERF_B32; i++) {
      add_query_register(layout, INTEL_PERF_QUERY_FIELD_TYPE_SRM_OA_B,
      }
   for (uint32_t i = 0; i < GFX12_N_OAG_PERF_C32; i++) {
      add_query_register(layout, INTEL_PERF_QUERY_FIELD_TYPE_SRM_OA_C,
         } else if (devinfo->verx10 == 125) {
      add_query_register(layout, INTEL_PERF_QUERY_FIELD_TYPE_SRM_OA_A,
         add_query_register(layout, INTEL_PERF_QUERY_FIELD_TYPE_SRM_OA_A,
         for (uint32_t i = 0; i < GFX12_N_OAG_PERF_B32; i++) {
      add_query_register(layout, INTEL_PERF_QUERY_FIELD_TYPE_SRM_OA_B,
      }
   for (uint32_t i = 0; i < GFX12_N_OAG_PERF_C32; i++) {
      add_query_register(layout, INTEL_PERF_QUERY_FIELD_TYPE_SRM_OA_C,
                        /* Align the whole package to 64bytes so that 2 snapshots can be put
   * together without extract alignment for the user.
   */
      }
      void
   intel_perf_init_metrics(struct intel_perf_config *perf_cfg,
                           {
               if (include_pipeline_statistics) {
      load_pipeline_statistic_metrics(perf_cfg, devinfo);
               bool oa_metrics = oa_metrics_available(perf_cfg, drm_fd, devinfo,
         if (oa_metrics)
            /* sort query groups by name */
   qsort(perf_cfg->queries, perf_cfg->n_queries,
                     if (oa_metrics)
      }
