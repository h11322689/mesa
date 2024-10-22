   /*
   * Copyright (C) 2012-2013 Rob Clark <robclark@freedesktop.org>
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
   #include "util/u_blend.h"
   #include "util/u_memory.h"
   #include "util/u_string.h"
      #include "fd2_blend.h"
   #include "fd2_context.h"
   #include "fd2_util.h"
      static enum a2xx_rb_blend_opcode
   blend_func(unsigned func)
   {
      switch (func) {
   case PIPE_BLEND_ADD:
         case PIPE_BLEND_MIN:
         case PIPE_BLEND_MAX:
         case PIPE_BLEND_SUBTRACT:
         case PIPE_BLEND_REVERSE_SUBTRACT:
         default:
      DBG("invalid blend func: %x", func);
         }
      void *
   fd2_blend_state_create(struct pipe_context *pctx,
         {
      const struct pipe_rt_blend_state *rt = &cso->rt[0];
   struct fd2_blend_stateobj *so;
            if (cso->logicop_enable)
            if (cso->independent_blend_enable) {
      DBG("Unsupported! independent blend state");
               so = CALLOC_STRUCT(fd2_blend_stateobj);
   if (!so)
                              so->rb_blendcontrol =
      A2XX_RB_BLEND_CONTROL_COLOR_SRCBLEND(
         A2XX_RB_BLEND_CONTROL_COLOR_COMB_FCN(blend_func(rt->rgb_func)) |
   A2XX_RB_BLEND_CONTROL_COLOR_DESTBLEND(
         /* hardware doesn't support SRC_ALPHA_SATURATE for alpha, but it is
   * equivalent to ONE */
   unsigned alpha_src_factor = rt->alpha_src_factor;
   if (alpha_src_factor == PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE)
            so->rb_blendcontrol |=
      A2XX_RB_BLEND_CONTROL_ALPHA_SRCBLEND(fd_blend_factor(alpha_src_factor)) |
   A2XX_RB_BLEND_CONTROL_ALPHA_COMB_FCN(blend_func(rt->alpha_func)) |
   A2XX_RB_BLEND_CONTROL_ALPHA_DESTBLEND(
         if (rt->colormask & PIPE_MASK_R)
         if (rt->colormask & PIPE_MASK_G)
         if (rt->colormask & PIPE_MASK_B)
         if (rt->colormask & PIPE_MASK_A)
            if (!rt->blend_enable)
            if (cso->dither)
               }
