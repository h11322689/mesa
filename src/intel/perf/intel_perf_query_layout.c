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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include <stdio.h>
   #include <stdlib.h>
   #include <getopt.h>
      #include "dev/intel_debug.h"
   #include "dev/intel_device_info.h"
   #include "intel_perf.h"
      static void
   print_help(const char *name)
   {
      fprintf(stdout, "%s -p <platform> ...\n", name);
   fprintf(stdout, "    -p/--platform      <platform>  Platform to use (skl, icl, tgl, etc...)\n");
      }
      static void
   print_metric_set(const struct intel_perf_query_info *metric_set)
   {
      for (uint32_t c = 0; c < metric_set->n_counters; c++) {
      const struct intel_perf_query_counter *counter = &metric_set->counters[c];
   fprintf(stdout, "   %s: offset=%zx/0x%zx name=%s\n",
         }
      int
   main(int argc, char *argv[])
   {
      const struct option perf_opts[] = {
      { "help",          no_argument,       NULL, 'h' },
   { "platform",      required_argument, NULL, 'p' },
   { "print-metric",  required_argument, NULL, 'P' },
      };
            int o = 0;
   while ((o = getopt_long(argc, argv, "hp:P:a", perf_opts, NULL)) != -1) {
      switch (o) {
   case 'h':
      print_help(argv[0]);
      case 'p':
      platform_name = optarg;
      case 'P':
      print_metric = optarg;
      default:
                     if (!platform_name) {
      fprintf(stderr, "No platform specified.\n");
               int devid = intel_device_name_to_pci_device_id(platform_name);
   if (!devid) {
      fprintf(stderr, "Invalid platform name.\n");
               struct intel_device_info devinfo;
   if (!intel_get_device_info_from_pci_id(devid, &devinfo)) {
      fprintf(stderr, "Unknown platform.\n");
               /* Force metric loading. */
            struct intel_perf_config *perf_cfg = intel_perf_new(NULL);
            if (!perf_cfg->i915_query_supported) {
      fprintf(stderr, "No supported queries for platform.\n");
               if (print_metric) {
      bool found = false;
   for (uint32_t i = 0; i < perf_cfg->n_queries; i++) {
               if (metric_set->symbol_name && !strcmp(metric_set->symbol_name, print_metric)) {
      fprintf(stdout, "%s name=%s size=%zx counters=%u:\n",
         metric_set->symbol_name, metric_set->name,
   print_metric_set(metric_set);
   found = true;
                  if (!found) {
      fprintf(stderr, "Unknown metric '%s'.\n", print_metric);
         } else {
      for (uint32_t i = 0; i < perf_cfg->n_queries; i++) {
               fprintf(stdout, "%s name=%s size=%zx counters=%u:\n",
         metric_set->symbol_name, metric_set->name,
                     }
