   /*
   * Copyright © 2019-2021 Collabora, Ltd.
   * Author: Antonio Caggiano <antonio.caggiano@collabora.com>
   * Author: Rohan Garg <rohan.garg@collabora.com>
   * Author: Robert Beckett <bob.beckett@collabora.com>
   *
   * SPDX-License-Identifier: MIT
   */
      #include "pan_pps_driver.h"
      #include <cstring>
   #include <perfetto.h>
   #include <xf86drm.h>
      #include <drm-uapi/panfrost_drm.h>
   #include <perf/pan_perf.h>
   #include <util/macros.h>
      #include <pps/pps.h>
   #include <pps/pps_algorithm.h>
      namespace pps {
   PanfrostDriver::PanfrostDriver()
   {
   }
      PanfrostDriver::~PanfrostDriver()
   {
   }
      uint64_t
   PanfrostDriver::get_min_sampling_period_ns()
   {
         }
      uint32_t
   find_id_within_group(uint32_t counter_id,
         {
      for (uint32_t cat_id = 0; cat_id < cfg->n_categories; ++cat_id) {
      const struct panfrost_perf_category *cat = &cfg->categories[cat_id];
   if (counter_id < cat->n_counters) {
         }
                  }
      std::pair<std::vector<CounterGroup>, std::vector<Counter>>
   PanfrostDriver::create_available_counters(const PanfrostPerf &perf)
   {
      std::pair<std::vector<CounterGroup>, std::vector<Counter>> ret;
                     for (uint32_t gid = 0; gid < perf.perf->cfg->n_categories; ++gid) {
      const auto &category = perf.perf->cfg->categories[gid];
   CounterGroup group = {};
   group.id = gid;
            for (; cid < category.n_counters; ++cid) {
      Counter counter = {};
                                 counter.set_getter([](const Counter &c, const Driver &d) {
      auto &pan_driver = PanfrostDriver::into(d);
   struct panfrost_perf *perf = pan_driver.perf->perf;
   uint32_t id_within_group = find_id_within_group(c.id, perf->cfg);
   const auto counter =
                                                         }
      bool
   PanfrostDriver::init_perfcnt()
   {
      if (!dev) {
         }
   if (!perf) {
         }
   if (groups.empty() && counters.empty()) {
         }
      }
      void
   PanfrostDriver::enable_counter(const uint32_t counter_id)
   {
         }
      void
   PanfrostDriver::enable_all_counters()
   {
      enabled_counters.resize(counters.size());
   for (size_t i = 0; i < counters.size(); ++i) {
            }
      void
   PanfrostDriver::enable_perfcnt(const uint64_t /* sampling_period_ns */)
   {
      auto res = perf->enable();
   if (!check(res, "Failed to enable performance counters")) {
      if (res == -ENOSYS) {
      PERFETTO_FATAL(
      }
         }
      bool
   PanfrostDriver::dump_perfcnt()
   {
               // Dump performance counters to buffer
   if (!check(perf->dump(), "Failed to dump performance counters")) {
      PERFETTO_ELOG("Skipping sample");
                  }
      uint64_t
   PanfrostDriver::next()
   {
      auto ret = last_dump_ts;
   last_dump_ts = 0;
      }
      void
   PanfrostDriver::disable_perfcnt()
   {
      perf->disable();
   perf.reset();
   dev.reset();
   groups.clear();
   counters.clear();
      }
      uint32_t
   PanfrostDriver::gpu_clock_id() const
   {
         }
      uint64_t
   PanfrostDriver::gpu_timestamp() const
   {
         }
      } // namespace pps
