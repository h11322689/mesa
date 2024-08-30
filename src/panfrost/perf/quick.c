   #include <stdio.h>
   #include <lib/pan_device.h>
   #include "pan_perf.h"
      int
   main(void)
   {
               if (fd < 0) {
      fprintf(stderr, "No panfrost device\n");
               void *ctx = ralloc_context(NULL);
            struct panfrost_device dev = {};
            panfrost_perf_init(perf, &dev);
            if (ret < 0) {
      fprintf(stderr, "failed to enable counters (%d)\n", ret);
   fprintf(
                                                for (unsigned i = 0; i < perf->cfg->n_categories; ++i) {
      const struct panfrost_perf_category *cat = &perf->cfg->categories[i];
            for (unsigned j = 0; j < cat->n_counters; ++j) {
      const struct panfrost_perf_counter *ctr = &cat->counters[j];
   uint32_t val = panfrost_perf_counter_read(ctr, perf);
                           if (panfrost_perf_disable(perf) < 0) {
      fprintf(stderr, "failed to disable counters\n");
                  }
