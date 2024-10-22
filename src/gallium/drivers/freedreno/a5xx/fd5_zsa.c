   /*
   * Copyright (C) 2016 Rob Clark <robclark@freedesktop.org>
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
   #include "util/u_memory.h"
   #include "util/u_string.h"
      #include "fd5_context.h"
   #include "fd5_format.h"
   #include "fd5_zsa.h"
      void *
   fd5_zsa_state_create(struct pipe_context *pctx,
         {
               so = CALLOC_STRUCT(fd5_zsa_stateobj);
   if (!so)
                     switch (cso->depth_func) {
   case PIPE_FUNC_LESS:
   case PIPE_FUNC_LEQUAL:
      so->gras_lrz_cntl = A5XX_GRAS_LRZ_CNTL_ENABLE;
         case PIPE_FUNC_GREATER:
   case PIPE_FUNC_GEQUAL:
      so->gras_lrz_cntl =
               default:
      /* LRZ not enabled */
   so->gras_lrz_cntl = 0;
               if (!(cso->stencil->enabled || cso->alpha_enabled || !cso->depth_writemask))
            so->rb_depth_cntl |=
            if (cso->depth_enabled)
      so->rb_depth_cntl |=
         if (cso->depth_writemask)
            if (cso->stencil[0].enabled) {
               so->rb_stencil_control |=
      A5XX_RB_STENCIL_CONTROL_STENCIL_READ |
   A5XX_RB_STENCIL_CONTROL_STENCIL_ENABLE |
   A5XX_RB_STENCIL_CONTROL_FUNC(s->func) | /* maps 1:1 */
   A5XX_RB_STENCIL_CONTROL_FAIL(fd_stencil_op(s->fail_op)) |
   A5XX_RB_STENCIL_CONTROL_ZPASS(fd_stencil_op(s->zpass_op)) |
      so->rb_stencilrefmask |=
                  if (cso->stencil[1].enabled) {
               so->rb_stencil_control |=
      A5XX_RB_STENCIL_CONTROL_STENCIL_ENABLE_BF |
   A5XX_RB_STENCIL_CONTROL_FUNC_BF(bs->func) | /* maps 1:1 */
   A5XX_RB_STENCIL_CONTROL_FAIL_BF(fd_stencil_op(bs->fail_op)) |
   A5XX_RB_STENCIL_CONTROL_ZPASS_BF(fd_stencil_op(bs->zpass_op)) |
      so->rb_stencilrefmask_bf |=
      A5XX_RB_STENCILREFMASK_BF_STENCILWRITEMASK(bs->writemask) |
               if (cso->alpha_enabled) {
      uint32_t ref = cso->alpha_ref_value * 255.0f;
   so->rb_alpha_control =
      A5XX_RB_ALPHA_CONTROL_ALPHA_TEST |
   A5XX_RB_ALPHA_CONTROL_ALPHA_REF(ref) |
      //		so->rb_depth_control |=
                  }
