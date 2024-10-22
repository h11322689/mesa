   /*
   * Copyright (C) 2017 Rob Clark <robclark@freedesktop.org>
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
      #include "freedreno_resource.h"
      #include "fd5_compute.h"
   #include "fd5_context.h"
   #include "fd5_emit.h"
      /* maybe move to fd5_program? */
   static void
   cs_program_emit(struct fd_context *ctx, struct fd_ringbuffer *ring, struct ir3_shader_variant *v) assert_dt
   {
      const struct ir3_info *i = &v->info;
   enum a3xx_threadsize thrsz = i->double_threadsize ? FOUR_QUADS : TWO_QUADS;
            /* if shader is more than 32*16 instructions, don't preload it.  Similar
   * to the combined restriction of 64*16 for VS+FS
   */
   if (instrlen > 32)
            OUT_PKT4(ring, REG_A5XX_SP_SP_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_HLSQ_CONTROL_0_REG, 1);
   OUT_RING(ring, A5XX_HLSQ_CONTROL_0_REG_FSTHREADSIZE(TWO_QUADS) |
                  OUT_PKT4(ring, REG_A5XX_SP_CS_CTRL_REG0, 1);
   OUT_RING(ring,
            A5XX_SP_CS_CTRL_REG0_THREADSIZE(thrsz) |
      A5XX_SP_CS_CTRL_REG0_HALFREGFOOTPRINT(i->max_half_reg + 1) |
   A5XX_SP_CS_CTRL_REG0_FULLREGFOOTPRINT(i->max_reg + 1) |
            OUT_PKT4(ring, REG_A5XX_HLSQ_CS_CONFIG, 1);
   OUT_RING(ring, A5XX_HLSQ_CS_CONFIG_CONSTOBJECTOFFSET(0) |
                  OUT_PKT4(ring, REG_A5XX_HLSQ_CS_CNTL, 1);
   OUT_RING(ring, A5XX_HLSQ_CS_CNTL_INSTRLEN(instrlen) |
            OUT_PKT4(ring, REG_A5XX_SP_CS_CONFIG, 1);
   OUT_RING(ring, A5XX_SP_CS_CONFIG_CONSTOBJECTOFFSET(0) |
                  assert(v->constlen % 4 == 0);
   unsigned constlen = v->constlen / 4;
   OUT_PKT4(ring, REG_A5XX_HLSQ_CS_CONSTLEN, 2);
   OUT_RING(ring, constlen); /* HLSQ_CS_CONSTLEN */
                     OUT_PKT4(ring, REG_A5XX_HLSQ_UPDATE_CNTL, 1);
            uint32_t local_invocation_id, work_group_id;
   local_invocation_id =
                  OUT_PKT4(ring, REG_A5XX_HLSQ_CS_CNTL_0, 2);
   OUT_RING(ring, A5XX_HLSQ_CS_CNTL_0_WGIDCONSTID(work_group_id) |
                              if (instrlen > 0)
      }
      static void
   fd5_launch_grid(struct fd_context *ctx,
         {
      struct ir3_shader_key key = {};
   struct ir3_shader_variant *v;
   struct fd_ringbuffer *ring = ctx->batch->draw;
            v =
         if (!v)
            if (ctx->dirty_shader[PIPE_SHADER_COMPUTE] & FD_DIRTY_SHADER_PROG)
            fd5_emit_cs_state(ctx, ring, v);
            u_foreach_bit (i, ctx->global_bindings.enabled_mask)
            if (nglobal > 0) {
      /* global resources don't otherwise get an OUT_RELOC(), since
   * the raw ptr address is emitted ir ir3_emit_cs_consts().
   * So to make the kernel aware that these buffers are referenced
   * by the batch, emit dummy reloc's as part of a no-op packet
   * payload:
   */
   OUT_PKT7(ring, CP_NOP, 2 * nglobal);
   u_foreach_bit (i, ctx->global_bindings.enabled_mask) {
      struct pipe_resource *prsc = ctx->global_bindings.buf[i];
                  const unsigned *local_size =
         const unsigned *num_groups = info->grid;
   /* for some reason, mesa/st doesn't set info->work_dim, so just assume 3: */
   const unsigned work_dim = info->work_dim ? info->work_dim : 3;
   OUT_PKT4(ring, REG_A5XX_HLSQ_CS_NDRANGE_0, 7);
   OUT_RING(ring, A5XX_HLSQ_CS_NDRANGE_0_KERNELDIM(work_dim) |
                     OUT_RING(ring,
         OUT_RING(ring, 0); /* HLSQ_CS_NDRANGE_2_GLOBALOFF_X */
   OUT_RING(ring,
         OUT_RING(ring, 0); /* HLSQ_CS_NDRANGE_4_GLOBALOFF_Y */
   OUT_RING(ring,
                  OUT_PKT4(ring, REG_A5XX_HLSQ_CS_KERNEL_GROUP_X, 3);
   OUT_RING(ring, 1); /* HLSQ_CS_KERNEL_GROUP_X */
   OUT_RING(ring, 1); /* HLSQ_CS_KERNEL_GROUP_Y */
            if (info->indirect) {
                        OUT_PKT7(ring, CP_EXEC_CS_INDIRECT, 4);
   OUT_RING(ring, 0x00000000);
   OUT_RELOC(ring, rsc->bo, info->indirect_offset, 0, 0); /* ADDR_LO/HI */
   OUT_RING(ring,
            A5XX_CP_EXEC_CS_INDIRECT_3_LOCALSIZEX(local_size[0] - 1) |
   } else {
      OUT_PKT7(ring, CP_EXEC_CS, 4);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, CP_EXEC_CS_1_NGROUPS_X(info->grid[0]));
   OUT_RING(ring, CP_EXEC_CS_2_NGROUPS_Y(info->grid[1]));
         }
      void
   fd5_compute_init(struct pipe_context *pctx) disable_thread_safety_analysis
   {
      struct fd_context *ctx = fd_context(pctx);
   ctx->launch_grid = fd5_launch_grid;
   pctx->create_compute_state = ir3_shader_compute_state_create;
      }
