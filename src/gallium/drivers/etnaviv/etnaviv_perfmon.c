   /*
   * Copyright (c) 2017 Etnaviv Project
   * Copyright (C) 2017 Zodiac Inflight Innovations
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sub license,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   *
   * Authors:
   *    Christian Gmeiner <christian.gmeiner@gmail.com>
   */
      #include "etnaviv_context.h"
   #include "etnaviv_perfmon.h"
   #include "etnaviv_screen.h"
      static const char *group_names[] = {
      [ETNA_QUERY_HI_GROUP_ID] = "HI",
   [ETNA_QUERY_PE_GROUP_ID] = "PE",
   [ETNA_QUERY_SH_GROUP_ID] = "SH",
   [ETNA_QUERY_PA_GROUP_ID] = "PA",
   [ETNA_QUERY_SE_GROUP_ID] = "SE",
   [ETNA_QUERY_RA_GROUP_ID] = "RA",
   [ETNA_QUERY_TX_GROUP_ID] = "TX",
      };
      static const struct etna_perfmon_config query_config[] = {
      {
      .name = "hi-total-read-bytes",
   .type = ETNA_QUERY_HI_TOTAL_READ_BYTES8,
   .group_id = ETNA_QUERY_HI_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
         },
      },
   {
      .name = "hi-total-write-bytes",
   .type = ETNA_QUERY_HI_TOTAL_WRITE_BYTES8,
   .group_id = ETNA_QUERY_HI_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
         },
      },
   {
      .name = "hi-total-cycles",
   .type = ETNA_QUERY_HI_TOTAL_CYCLES,
   .group_id = ETNA_QUERY_HI_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "hi-idle-cycles",
   .type = ETNA_QUERY_HI_IDLE_CYCLES,
   .group_id = ETNA_QUERY_HI_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "hi-axi-cycles-read-request-stalled",
   .type = ETNA_QUERY_HI_AXI_CYCLES_READ_REQUEST_STALLED,
   .group_id = ETNA_QUERY_HI_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "hi-axi-cycles-write-request-stalled",
   .type = ETNA_QUERY_HI_AXI_CYCLES_WRITE_REQUEST_STALLED,
   .group_id = ETNA_QUERY_HI_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "hi-axi-cycles-write-data-stalled",
   .type = ETNA_QUERY_HI_AXI_CYCLES_WRITE_DATA_STALLED,
   .group_id = ETNA_QUERY_HI_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "pe-pixel-count-killed-by-color-pipe",
   .type = ETNA_QUERY_PE_PIXEL_COUNT_KILLED_BY_COLOR_PIPE,
   .group_id = ETNA_QUERY_PE_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "pe-pixel-count-killed-by-depth-pipe",
   .type = ETNA_QUERY_PE_PIXEL_COUNT_KILLED_BY_DEPTH_PIPE,
   .group_id = ETNA_QUERY_PE_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "pe-pixel-count-drawn-by-color-pipe",
   .type = ETNA_QUERY_PE_PIXEL_COUNT_DRAWN_BY_COLOR_PIPE,
   .group_id = ETNA_QUERY_PE_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "pe-pixel-count-drawn-by-depth-pipe",
   .type = ETNA_QUERY_PE_PIXEL_COUNT_DRAWN_BY_DEPTH_PIPE,
   .group_id = ETNA_QUERY_PE_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "sh-shader-cycles",
   .type = ETNA_QUERY_SH_SHADER_CYCLES,
   .group_id = ETNA_QUERY_SH_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "sh-ps-inst-counter",
   .type = ETNA_QUERY_SH_PS_INST_COUNTER,
   .group_id = ETNA_QUERY_SH_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "sh-rendered-pixel-counter",
   .type = ETNA_QUERY_SH_RENDERED_PIXEL_COUNTER,
   .group_id = ETNA_QUERY_SH_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "sh-vs-inst-counter",
   .type = ETNA_QUERY_SH_VS_INST_COUNTER,
   .group_id = ETNA_QUERY_SH_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "sh-rendered-vertice-counter",
   .type = ETNA_QUERY_SH_RENDERED_VERTICE_COUNTER,
   .group_id = ETNA_QUERY_SH_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "sh-vtx-branch-inst-counter",
   .type = ETNA_QUERY_SH_RENDERED_VERTICE_COUNTER,
   .group_id = ETNA_QUERY_SH_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "sh-vtx-texld-inst-counter",
   .type = ETNA_QUERY_SH_RENDERED_VERTICE_COUNTER,
   .group_id = ETNA_QUERY_SH_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "sh-plx-branch-inst-counter",
   .type = ETNA_QUERY_SH_RENDERED_VERTICE_COUNTER,
   .group_id = ETNA_QUERY_SH_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "sh-plx-texld-inst-counter",
   .type = ETNA_QUERY_SH_RENDERED_VERTICE_COUNTER,
   .group_id = ETNA_QUERY_SH_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "pa-input-vtx-counter",
   .type = ETNA_QUERY_PA_INPUT_VTX_COUNTER,
   .group_id = ETNA_QUERY_PA_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "pa-input-prim-counter",
   .type = ETNA_QUERY_PA_INPUT_PRIM_COUNTER,
   .group_id = ETNA_QUERY_PA_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "pa-output-prim-counter",
   .type = ETNA_QUERY_PA_OUTPUT_PRIM_COUNTER,
   .group_id = ETNA_QUERY_PA_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "pa-depth-clipped-counter",
   .type = ETNA_QUERY_PA_DEPTH_CLIPPED_COUNTER,
   .group_id = ETNA_QUERY_PA_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "pa-trivial-rejected-counter",
   .type = ETNA_QUERY_PA_TRIVIAL_REJECTED_COUNTER,
   .group_id = ETNA_QUERY_PA_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "pa-culled-counter",
   .type = ETNA_QUERY_PA_CULLED_COUNTER,
   .group_id = ETNA_QUERY_PA_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "se-culled-triangle-count",
   .type = ETNA_QUERY_SE_CULLED_TRIANGLE_COUNT,
   .group_id = ETNA_QUERY_SE_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "se-culled-lines-count",
   .type = ETNA_QUERY_SE_CULLED_LINES_COUNT,
   .group_id = ETNA_QUERY_SE_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "ra-valid-pixel-count",
   .type = ETNA_QUERY_RA_VALID_PIXEL_COUNT,
   .group_id = ETNA_QUERY_RA_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "ra-total-quad-count",
   .type = ETNA_QUERY_RA_TOTAL_QUAD_COUNT,
   .group_id = ETNA_QUERY_RA_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "ra-valid-quad-count-after-early-z",
   .type = ETNA_QUERY_RA_VALID_QUAD_COUNT_AFTER_EARLY_Z,
   .group_id = ETNA_QUERY_RA_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "ra-total-primitive-count",
   .type = ETNA_QUERY_RA_TOTAL_PRIMITIVE_COUNT,
   .group_id = ETNA_QUERY_RA_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "ra-pipe-cache-miss-counter",
   .type = ETNA_QUERY_RA_PIPE_CACHE_MISS_COUNTER,
   .group_id = ETNA_QUERY_RA_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "ra-prefetch-cache-miss-counter",
   .type = ETNA_QUERY_RA_PREFETCH_CACHE_MISS_COUNTER,
   .group_id = ETNA_QUERY_RA_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "ra-pculled-quad-count",
   .type = ETNA_QUERY_RA_CULLED_QUAD_COUNT,
   .group_id = ETNA_QUERY_RA_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "tx-total-bilinear-requests",
   .type = ETNA_QUERY_TX_TOTAL_BILINEAR_REQUESTS,
   .group_id = ETNA_QUERY_TX_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "tx-total-trilinear-requests",
   .type = ETNA_QUERY_TX_TOTAL_TRILINEAR_REQUESTS,
   .group_id = ETNA_QUERY_TX_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "tx-total-discarded-texture-requests",
   .type = ETNA_QUERY_TX_TOTAL_DISCARDED_TEXTURE_REQUESTS,
   .group_id = ETNA_QUERY_TX_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "tx-total-texture-requests",
   .type = ETNA_QUERY_TX_TOTAL_TEXTURE_REQUESTS,
   .group_id = ETNA_QUERY_TX_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "tx-mem-read-count",
   .type = ETNA_QUERY_TX_MEM_READ_COUNT,
   .group_id = ETNA_QUERY_TX_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "tx-mem-read-bytes",
   .type = ETNA_QUERY_TX_MEM_READ_IN_8B_COUNT,
   .group_id = ETNA_QUERY_TX_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
         },
      },
   {
      .name = "tx-cache-miss-count",
   .type = ETNA_QUERY_TX_CACHE_MISS_COUNT,
   .group_id = ETNA_QUERY_TX_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "tx-cache-hit-texel-count",
   .type = ETNA_QUERY_TX_CACHE_HIT_TEXEL_COUNT,
   .group_id = ETNA_QUERY_TX_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "tx-cache-miss-texel-count",
   .type = ETNA_QUERY_TX_CACHE_MISS_TEXEL_COUNT,
   .group_id = ETNA_QUERY_TX_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "mc-total-read-req-8b-from-pipeline",
   .type = ETNA_QUERY_MC_TOTAL_READ_REQ_8B_FROM_PIPELINE,
   .group_id = ETNA_QUERY_MC_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "mc-total-read-req-8b-from-ip",
   .type = ETNA_QUERY_MC_TOTAL_READ_REQ_8B_FROM_IP,
   .group_id = ETNA_QUERY_MC_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
            },
   {
      .name = "mc-total-write-req-8b-from-pipeline",
   .type = ETNA_QUERY_MC_TOTAL_WRITE_REQ_8B_FROM_PIPELINE,
   .group_id = ETNA_QUERY_MC_GROUP_ID,
   .source = (const struct etna_perfmon_source[]) {
               };
      struct etna_perfmon_signal *
   etna_pm_query_signal(struct etna_perfmon *perfmon,
         {
               domain = etna_perfmon_get_dom_by_name(perfmon, source->domain);
   if (!domain)
               }
      void
   etna_pm_query_setup(struct etna_screen *screen)
   {
               if (!screen->perfmon)
            for (unsigned i = 0; i < ARRAY_SIZE(query_config); i++) {
               if (!etna_pm_cfg_supported(screen->perfmon, cfg))
                  }
      const struct etna_perfmon_config *
   etna_pm_query_config(unsigned type)
   {
      for (unsigned i = 0; i < ARRAY_SIZE(query_config); i++)
      if (query_config[i].type == type)
            }
      int
   etna_pm_get_driver_query_info(struct pipe_screen *pscreen, unsigned index,
         {
      const struct etna_screen *screen = etna_screen(pscreen);
   const unsigned num = screen->supported_pm_queries.size / sizeof(unsigned);
            if (!info)
            if (index >= num)
            i = *util_dynarray_element(&screen->supported_pm_queries, unsigned, index);
            info->name = query_config[i].name;
   info->query_type = query_config[i].type;
               }
      static
   unsigned query_count(unsigned group)
   {
               for (unsigned i = 0; i < ARRAY_SIZE(query_config); i++)
      if (query_config[i].group_id == group)
                     }
      int
   etna_pm_get_driver_query_group_info(struct pipe_screen *pscreen,
               {
      if (!info)
            if (index >= ARRAY_SIZE(group_names))
                     info->name = group_names[index];
   info->max_active_queries = count;
               }
