   /*
   * Copyright © 2020 Valve Corporation
   *
   * based on amdgpu winsys.
   * Copyright © 2016 Red Hat.
   * Copyright © 2016 Bas Nieuwenhuizen
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
      #include "radv_null_cs.h"
   #include "util/u_memory.h"
      struct radv_null_cs {
      struct radeon_cmdbuf base;
      };
      static inline struct radv_null_cs *
   radv_null_cs(struct radeon_cmdbuf *base)
   {
         }
      static VkResult
   radv_null_ctx_create(struct radeon_winsys *_ws, enum radeon_ctx_priority priority, struct radeon_winsys_ctx **rctx)
   {
               if (!ctx)
            *rctx = (struct radeon_winsys_ctx *)ctx;
      }
      static void
   radv_null_ctx_destroy(struct radeon_winsys_ctx *rwctx)
   {
      struct radv_null_ctx *ctx = (struct radv_null_ctx *)rwctx;
      }
      static enum radeon_bo_domain
   radv_null_cs_domain(const struct radeon_winsys *_ws)
   {
         }
      static struct radeon_cmdbuf *
   radv_null_cs_create(struct radeon_winsys *ws, enum amd_ip_type ip_type, UNUSED bool is_secondary)
   {
      struct radv_null_cs *cs = calloc(1, sizeof(struct radv_null_cs));
   if (!cs)
                     cs->base.buf = malloc(16384);
   cs->base.max_dw = 4096;
   if (!cs->base.buf) {
      FREE(cs);
                  }
      static VkResult
   radv_null_cs_finalize(struct radeon_cmdbuf *_cs)
   {
         }
      static void
   radv_null_cs_destroy(struct radeon_cmdbuf *rcs)
   {
      struct radv_null_cs *cs = radv_null_cs(rcs);
   FREE(cs->base.buf);
      }
      void
   radv_null_cs_init_functions(struct radv_null_winsys *ws)
   {
      ws->base.ctx_create = radv_null_ctx_create;
   ws->base.ctx_destroy = radv_null_ctx_destroy;
   ws->base.cs_domain = radv_null_cs_domain;
   ws->base.cs_create = radv_null_cs_create;
   ws->base.cs_finalize = radv_null_cs_finalize;
      }
