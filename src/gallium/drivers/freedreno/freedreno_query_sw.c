   /*
   * Copyright (C) 2014 Rob Clark <robclark@freedesktop.org>
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   * Authors:
   *    Rob Clark <robclark@freedesktop.org>
   */
      #include "pipe/p_state.h"
   #include "util/os_time.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "util/u_string.h"
      #include "freedreno_context.h"
   #include "freedreno_query_sw.h"
   #include "freedreno_util.h"
      /*
   * SW Queries:
   *
   * In the core, we have some support for basic sw counters
   */
      static void
   fd_sw_destroy_query(struct fd_context *ctx, struct fd_query *q)
   {
      struct fd_sw_query *sq = fd_sw_query(q);
      }
      static uint64_t
   read_counter(struct fd_context *ctx, int type) assert_dt
   {
      switch (type) {
   case PIPE_QUERY_PRIMITIVES_GENERATED:
         case PIPE_QUERY_PRIMITIVES_EMITTED:
         case FD_QUERY_DRAW_CALLS:
         case FD_QUERY_BATCH_TOTAL:
         case FD_QUERY_BATCH_SYSMEM:
         case FD_QUERY_BATCH_GMEM:
         case FD_QUERY_BATCH_NONDRAW:
         case FD_QUERY_BATCH_RESTORE:
         case FD_QUERY_STAGING_UPLOADS:
         case FD_QUERY_SHADOW_UPLOADS:
         case FD_QUERY_VS_REGS:
         case FD_QUERY_FS_REGS:
         }
      }
      static bool
   is_time_rate_query(struct fd_query *q)
   {
      switch (q->type) {
   case FD_QUERY_BATCH_TOTAL:
   case FD_QUERY_BATCH_SYSMEM:
   case FD_QUERY_BATCH_GMEM:
   case FD_QUERY_BATCH_NONDRAW:
   case FD_QUERY_BATCH_RESTORE:
   case FD_QUERY_STAGING_UPLOADS:
   case FD_QUERY_SHADOW_UPLOADS:
         default:
            }
      static bool
   is_draw_rate_query(struct fd_query *q)
   {
      switch (q->type) {
   case FD_QUERY_VS_REGS:
   case FD_QUERY_FS_REGS:
         default:
            }
      static void
   fd_sw_begin_query(struct fd_context *ctx, struct fd_query *q) assert_dt
   {
                        sq->begin_value = read_counter(ctx, q->type);
   if (is_time_rate_query(q)) {
         } else if (is_draw_rate_query(q)) {
            }
      static void
   fd_sw_end_query(struct fd_context *ctx, struct fd_query *q) assert_dt
   {
               assert(ctx->stats_users > 0);
            sq->end_value = read_counter(ctx, q->type);
   if (is_time_rate_query(q)) {
         } else if (is_draw_rate_query(q)) {
            }
      static bool
   fd_sw_get_query_result(struct fd_context *ctx, struct fd_query *q, bool wait,
         {
                        if (is_time_rate_query(q)) {
      double fps =
            } else if (is_draw_rate_query(q)) {
      double avg =
                        }
      static const struct fd_query_funcs sw_query_funcs = {
      .destroy_query = fd_sw_destroy_query,
   .begin_query = fd_sw_begin_query,
   .end_query = fd_sw_end_query,
      };
      struct fd_query *
   fd_sw_create_query(struct fd_context *ctx, unsigned query_type, unsigned index)
   {
      struct fd_sw_query *sq;
            switch (query_type) {
   case PIPE_QUERY_PRIMITIVES_GENERATED:
   case PIPE_QUERY_PRIMITIVES_EMITTED:
   case FD_QUERY_DRAW_CALLS:
   case FD_QUERY_BATCH_TOTAL:
   case FD_QUERY_BATCH_SYSMEM:
   case FD_QUERY_BATCH_GMEM:
   case FD_QUERY_BATCH_NONDRAW:
   case FD_QUERY_BATCH_RESTORE:
   case FD_QUERY_STAGING_UPLOADS:
   case FD_QUERY_SHADOW_UPLOADS:
   case FD_QUERY_VS_REGS:
   case FD_QUERY_FS_REGS:
         default:
                  sq = CALLOC_STRUCT(fd_sw_query);
   if (!sq)
            q = &sq->base;
   q->funcs = &sw_query_funcs;
               }
