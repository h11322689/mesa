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
      #include "tgsi/tgsi_text.h"
   #include "tgsi/tgsi_ureg.h"
      #include "util/u_simple_shaders.h"
      #include "freedreno_context.h"
   #include "freedreno_program.h"
      static void
   update_bound_stage(struct fd_context *ctx, enum pipe_shader_type shader,
         {
      uint32_t bound_shader_stages = ctx->bound_shader_stages;
   if (bound) {
         } else {
         }
   if (ctx->update_draw && (bound_shader_stages != ctx->bound_shader_stages))
      }
      static void
   fd_set_tess_state(struct pipe_context *pctx,
                     {
               /* These turn into driver-params where are emitted on every draw if
   * needed by the shader (they will only be needed by pass-through
   * TCS shader)
   */
   memcpy(ctx->default_outer_level,
                  memcpy(ctx->default_inner_level,
            }
      static void
   fd_set_patch_vertices(struct pipe_context *pctx, uint8_t patch_vertices) in_dt
   {
               if (ctx->patch_vertices == patch_vertices)
                     /* If we have tessellation this dirties the TCS state.  Check for TES
   * stage as TCS could be NULL (passthrough)
   */
   if (ctx->prog.ds || ctx->prog.hs) {
            }
      static void
   fd_vs_state_bind(struct pipe_context *pctx, void *hwcso) in_dt
   {
      struct fd_context *ctx = fd_context(pctx);
   ctx->prog.vs = hwcso;
   fd_context_dirty_shader(ctx, PIPE_SHADER_VERTEX, FD_DIRTY_SHADER_PROG);
      }
      static void
   fd_tcs_state_bind(struct pipe_context *pctx, void *hwcso) in_dt
   {
      struct fd_context *ctx = fd_context(pctx);
   ctx->prog.hs = hwcso;
   fd_context_dirty_shader(ctx, PIPE_SHADER_TESS_CTRL, FD_DIRTY_SHADER_PROG);
      }
      static void
   fd_tes_state_bind(struct pipe_context *pctx, void *hwcso) in_dt
   {
      struct fd_context *ctx = fd_context(pctx);
   ctx->prog.ds = hwcso;
   fd_context_dirty_shader(ctx, PIPE_SHADER_TESS_EVAL, FD_DIRTY_SHADER_PROG);
      }
      static void
   fd_gs_state_bind(struct pipe_context *pctx, void *hwcso) in_dt
   {
      struct fd_context *ctx = fd_context(pctx);
   ctx->prog.gs = hwcso;
   fd_context_dirty_shader(ctx, PIPE_SHADER_GEOMETRY, FD_DIRTY_SHADER_PROG);
      }
      static void
   fd_fs_state_bind(struct pipe_context *pctx, void *hwcso) in_dt
   {
      struct fd_context *ctx = fd_context(pctx);
   ctx->prog.fs = hwcso;
   fd_context_dirty_shader(ctx, PIPE_SHADER_FRAGMENT, FD_DIRTY_SHADER_PROG);
      }
      static const char *solid_fs = "FRAG                                        \n"
                                    static const char *solid_vs = "VERT                                        \n"
                              static void *
   assemble_tgsi(struct pipe_context *pctx, const char *src, bool frag)
   {
      struct tgsi_token toks[32];
   struct pipe_shader_state cso = {
                  bool ret = tgsi_text_translate(src, toks, ARRAY_SIZE(toks));
            if (frag)
         else
      }
      /* the correct semantic to use for the texcoord varying depends on pipe-cap: */
   static enum tgsi_semantic
   texcoord_semantic(struct pipe_context *pctx)
   {
               if (pscreen->get_param(pscreen, PIPE_CAP_TGSI_TEXCOORD)) {
         } else {
            }
      static void *
   fd_prog_blit_vs(struct pipe_context *pctx)
   {
               ureg = ureg_create(PIPE_SHADER_VERTEX);
   if (!ureg)
            struct ureg_src in0 = ureg_DECL_vs_input(ureg, 0);
            struct ureg_dst out0 = ureg_DECL_output(ureg, texcoord_semantic(pctx), 0);
            ureg_MOV(ureg, out0, in0);
                        }
      static void *
   fd_prog_blit_fs(struct pipe_context *pctx, int rts, bool depth)
   {
      int i;
   struct ureg_src tc;
                     ureg = ureg_create(PIPE_SHADER_FRAGMENT);
   if (!ureg)
            tc = ureg_DECL_fs_input(ureg, texcoord_semantic(pctx), 0,
         for (i = 0; i < rts; i++)
      ureg_TEX(ureg, ureg_DECL_output(ureg, TGSI_SEMANTIC_COLOR, i),
      if (depth)
      ureg_TEX(ureg,
                                 }
      void
   fd_prog_init(struct pipe_context *pctx)
   {
      struct fd_context *ctx = fd_context(pctx);
            pctx->bind_vs_state = fd_vs_state_bind;
   pctx->bind_tcs_state = fd_tcs_state_bind;
   pctx->bind_tes_state = fd_tes_state_bind;
   pctx->bind_gs_state = fd_gs_state_bind;
   pctx->bind_fs_state = fd_fs_state_bind;
   pctx->set_tess_state = fd_set_tess_state;
            if (ctx->flags & PIPE_CONTEXT_COMPUTE_ONLY)
            ctx->solid_prog.fs = assemble_tgsi(pctx, solid_fs, true);
            if (ctx->screen->gen >= 6) {
      ctx->solid_layered_prog.fs = assemble_tgsi(pctx, solid_fs, true);
               if (ctx->screen->gen >= 5)
            ctx->blit_prog[0].vs = fd_prog_blit_vs(pctx);
            if (ctx->screen->gen < 3)
            for (i = 1; i < ctx->screen->max_rts; i++) {
      ctx->blit_prog[i].vs = ctx->blit_prog[0].vs;
               ctx->blit_z.vs = ctx->blit_prog[0].vs;
   ctx->blit_z.fs = fd_prog_blit_fs(pctx, 0, true);
   ctx->blit_zs.vs = ctx->blit_prog[0].vs;
      }
      void
   fd_prog_fini(struct pipe_context *pctx)
   {
      struct fd_context *ctx = fd_context(pctx);
            if (ctx->flags & PIPE_CONTEXT_COMPUTE_ONLY)
            pctx->delete_vs_state(pctx, ctx->solid_prog.vs);
            if (ctx->screen->gen >= 6) {
      pctx->delete_vs_state(pctx, ctx->solid_layered_prog.vs);
               if (ctx->screen->gen >= 5)
            pctx->delete_vs_state(pctx, ctx->blit_prog[0].vs);
            if (ctx->screen->gen < 3)
            for (i = 1; i < ctx->screen->max_rts; i++)
         pctx->delete_fs_state(pctx, ctx->blit_z.fs);
      }
