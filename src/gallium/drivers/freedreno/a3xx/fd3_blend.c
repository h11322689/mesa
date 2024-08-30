   /*
   * Copyright (C) 2013 Rob Clark <robclark@freedesktop.org>
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
   #include "util/u_dual_blend.h"
   #include "util/u_memory.h"
   #include "util/u_string.h"
      #include "fd3_blend.h"
   #include "fd3_context.h"
   #include "fd3_format.h"
      static enum a3xx_rb_blend_opcode
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
   fd3_blend_state_create(struct pipe_context *pctx,
         {
      struct fd3_blend_stateobj *so;
   enum a3xx_rop_code rop = ROP_COPY;
   bool reads_dest = false;
            if (cso->logicop_enable) {
      rop = cso->logicop_func; /* maps 1:1 */
               so = CALLOC_STRUCT(fd3_blend_stateobj);
   if (!so)
                     for (i = 0; i < ARRAY_SIZE(so->rb_mrt); i++) {
      const struct pipe_rt_blend_state *rt;
   if (cso->independent_blend_enable)
         else
            so->rb_mrt[i].blend_control =
      A3XX_RB_MRT_BLEND_CONTROL_RGB_SRC_FACTOR(
         A3XX_RB_MRT_BLEND_CONTROL_RGB_BLEND_OPCODE(blend_func(rt->rgb_func)) |
   A3XX_RB_MRT_BLEND_CONTROL_RGB_DEST_FACTOR(
         A3XX_RB_MRT_BLEND_CONTROL_ALPHA_SRC_FACTOR(
         A3XX_RB_MRT_BLEND_CONTROL_ALPHA_BLEND_OPCODE(
                     so->rb_mrt[i].control =
                  if (rt->blend_enable)
      so->rb_mrt[i].control |= A3XX_RB_MRT_CONTROL_READ_DEST_ENABLE |
               if (reads_dest)
            if (cso->dither)
      so->rb_mrt[i].control |=
            if (cso->rt[0].blend_enable && util_blend_state_is_dual(cso, 0))
               }
