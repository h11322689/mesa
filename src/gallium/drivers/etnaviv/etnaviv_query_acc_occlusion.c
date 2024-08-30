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
   *    Rob Clark <robclark@freedesktop.org>
   *    Christian Gmeiner <christian.gmeiner@gmail.com>
   */
      #include "util/compiler.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
      #include "etnaviv_context.h"
   #include "etnaviv_debug.h"
   #include "etnaviv_emit.h"
   #include "etnaviv_query_acc.h"
   #include "etnaviv_screen.h"
      #define MAX_OQ_SAMPLES 511 /* 4KB / 8Bytes/sample */
      /*
   * Occlusion Query:
   *
   * OCCLUSION_COUNTER and OCCLUSION_PREDICATE differ only in how they
   * interpret results
   */
      static bool
   occlusion_supports(unsigned query_type)
   {
      switch (query_type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
         case PIPE_QUERY_OCCLUSION_PREDICATE:
         case PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE:
         default:
            }
      static struct etna_acc_query *
   occlusion_allocate(struct etna_context *ctx, ASSERTED unsigned query_type)
   {
         }
      static void
   occlusion_resume(struct etna_acc_query *aq, struct etna_context *ctx)
   {
      struct etna_resource *rsc = etna_resource(aq->prsc);
   struct etna_reloc r = {
      .bo = rsc->bo,
               if (aq->samples > MAX_OQ_SAMPLES) {
      aq->samples = MAX_OQ_SAMPLES;
                        etna_set_state_reloc(ctx->stream, VIVS_GL_OCCLUSION_QUERY_ADDR, &r);
      }
      static void
   occlusion_suspend(struct etna_acc_query *aq, struct etna_context *ctx)
   {
      /* 0x1DF5E76 is the value used by blob - but any random value will work */
   etna_set_state(ctx->stream, VIVS_GL_OCCLUSION_QUERY_CONTROL, 0x1DF5E76);
   resource_written(ctx, aq->prsc);
      }
      static bool
   occlusion_result(struct etna_acc_query *aq, void *buf,
         {
      uint64_t sum = 0;
            for (unsigned i = 0; i < aq->samples; i++)
            if (aq->base.type == PIPE_QUERY_OCCLUSION_COUNTER)
         else
               }
      const struct etna_acc_sample_provider occlusion_provider = {
      .supports = occlusion_supports,
   .allocate = occlusion_allocate,
   .suspend = occlusion_suspend,
   .resume = occlusion_resume,
      };
