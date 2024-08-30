   /*
   * Copyright (c) 2012-2015 Etnaviv Project
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
   *    Wladimir J. van der Laan <laanwj@gmail.com>
   */
      #include "etnaviv_zsa.h"
      #include "etnaviv_context.h"
   #include "etnaviv_screen.h"
   #include "etnaviv_translate.h"
   #include "util/half_float.h"
   #include "util/u_memory.h"
      #include "hw/common.xml.h"
      void *
   etna_zsa_state_create(struct pipe_context *pctx,
         {
      struct etna_context *ctx = etna_context(pctx);
   struct etna_screen *screen = ctx->screen;
            if (!cs)
                     cs->z_test_enabled = so->depth_enabled && so->depth_func != PIPE_FUNC_ALWAYS;
                  /* Set operations to KEEP if write mask is 0.
   * When we don't do this, the depth buffer is written for the entire primitive
   * instead of just where the stencil condition holds (GC600 rev 0x0019, without
   * feature CORRECT_STENCIL).
   * Not sure if this is a hardware bug or just a strange edge case. */
   #if 0 /* TODO: It looks like a hardware bug */
      for(int i=0; i<2; ++i)
   {
      if(so->stencil[i].writemask == 0)
   {
               #endif
         /* Determine whether to enable early z reject. Don't enable it when any of
   * the stencil-modifying functions is used. */
   if (so->stencil[0].enabled) {
      if (so->stencil[0].func != PIPE_FUNC_ALWAYS ||
                  if (so->stencil[0].fail_op != PIPE_STENCIL_OP_KEEP ||
      so->stencil[0].zfail_op != PIPE_STENCIL_OP_KEEP ||
   so->stencil[0].zpass_op != PIPE_STENCIL_OP_KEEP) {
   cs->stencil_enabled = 1;
      } else if (so->stencil[1].enabled) {
      if (so->stencil[1].fail_op != PIPE_STENCIL_OP_KEEP ||
      so->stencil[1].zfail_op != PIPE_STENCIL_OP_KEEP ||
   so->stencil[1].zpass_op != PIPE_STENCIL_OP_KEEP) {
   cs->stencil_enabled = 1;
                     /* calculate extra_reference value */
            if (VIV_FEATURE(screen, chipMinorFeatures1, HALF_FLOAT))
            cs->PE_STENCIL_CONFIG_EXT =
            cs->PE_ALPHA_OP =
      COND(so->alpha_enabled, VIVS_PE_ALPHA_OP_ALPHA_TEST) |
   VIVS_PE_ALPHA_OP_ALPHA_FUNC(so->alpha_func) |
         for (unsigned i = 0; i < 2; i++) {
      const struct pipe_stencil_state *stencil_front = (so->stencil[1].enabled && so->stencil[1].valuemask) ? &so->stencil[i] : &so->stencil[0];
   const struct pipe_stencil_state *stencil_back = (so->stencil[1].enabled && so->stencil[1].valuemask) ? &so->stencil[!i] : &so->stencil[0];
   cs->PE_STENCIL_OP[i] =
      VIVS_PE_STENCIL_OP_FUNC_FRONT(stencil_front->func) |
   VIVS_PE_STENCIL_OP_FUNC_BACK(stencil_back->func) |
   VIVS_PE_STENCIL_OP_FAIL_FRONT(translate_stencil_op(stencil_front->fail_op)) |
   VIVS_PE_STENCIL_OP_FAIL_BACK(translate_stencil_op(stencil_back->fail_op)) |
   VIVS_PE_STENCIL_OP_DEPTH_FAIL_FRONT(translate_stencil_op(stencil_front->zfail_op)) |
   VIVS_PE_STENCIL_OP_DEPTH_FAIL_BACK(translate_stencil_op(stencil_back->zfail_op)) |
   VIVS_PE_STENCIL_OP_PASS_FRONT(translate_stencil_op(stencil_front->zpass_op)) |
      cs->PE_STENCIL_CONFIG[i] =
      translate_stencil_mode(so->stencil[0].enabled, so->stencil[0].enabled) |
   VIVS_PE_STENCIL_CONFIG_MASK_FRONT(stencil_front->valuemask) |
      cs->PE_STENCIL_CONFIG_EXT2[i] =
      VIVS_PE_STENCIL_CONFIG_EXT2_MASK_BACK(stencil_back->valuemask) |
            /* XXX does alpha/stencil test affect PE_COLOR_FORMAT_OVERWRITE? */
      }
