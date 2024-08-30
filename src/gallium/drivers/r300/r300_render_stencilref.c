   /*
   * Copyright 2010 Marek Olšák <maraeo@gmail.com>
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE. */
      /**
   * The two-sided stencil reference value fallback for r3xx-r4xx chips.
   * These chips support two-sided stencil functions but they do not support
   * a two-sided reference value.
   *
   * The functions below split every draw call which uses the two-sided
   * reference value into two draw calls -- the first one renders front faces
   * and the second renders back faces with the other reference value.
   */
      #include "r300_context.h"
   #include "r300_reg.h"
      struct r300_stencilref_context {
      void (*draw_vbo)(struct pipe_context *pipe,
                     const struct pipe_draw_info *info,
            uint32_t rs_cull_mode;
   uint32_t zb_stencilrefmask;
      };
      static bool r300_stencilref_needed(struct r300_context *r300)
   {
               return dsa->two_sided_stencil_ref ||
            }
      /* Set drawing for front faces. */
   static void r300_stencilref_begin(struct r300_context *r300)
   {
      struct r300_stencilref_context *sr = r300->stencilref_fallback;
   struct r300_rs_state *rs = (struct r300_rs_state*)r300->rs_state.state;
            /* Save state. */
   sr->rs_cull_mode = rs->cb_main[rs->cull_mode_index];
   sr->zb_stencilrefmask = dsa->stencil_ref_mask;
            /* We *cull* pixels, therefore no need to mask out the bits. */
               }
      /* Set drawing for back faces. */
   static void r300_stencilref_switch_side(struct r300_context *r300)
   {
      struct r300_stencilref_context *sr = r300->stencilref_fallback;
   struct r300_rs_state *rs = (struct r300_rs_state*)r300->rs_state.state;
            rs->cb_main[rs->cull_mode_index] = sr->rs_cull_mode | R300_CULL_FRONT;
   dsa->stencil_ref_mask = dsa->stencil_ref_bf;
            r300_mark_atom_dirty(r300, &r300->rs_state);
      }
      /* Restore the original state. */
   static void r300_stencilref_end(struct r300_context *r300)
   {
      struct r300_stencilref_context *sr = r300->stencilref_fallback;
   struct r300_rs_state *rs = (struct r300_rs_state*)r300->rs_state.state;
            /* Restore state. */
   rs->cb_main[rs->cull_mode_index] = sr->rs_cull_mode;
   dsa->stencil_ref_mask = sr->zb_stencilrefmask;
            r300_mark_atom_dirty(r300, &r300->rs_state);
      }
      static void r300_stencilref_draw_vbo(struct pipe_context *pipe,
                                 {
      struct r300_context *r300 = r300_context(pipe);
            if (!r300_stencilref_needed(r300)) {
         } else {
      r300_stencilref_begin(r300);
   sr->draw_vbo(pipe, info, drawid_offset, NULL, draws, num_draws);
   r300_stencilref_switch_side(r300);
   sr->draw_vbo(pipe, info, drawid_offset, NULL, draws, num_draws);
         }
      void r300_plug_in_stencil_ref_fallback(struct r300_context *r300)
   {
               /* Save original draw function. */
            /* Override the draw function. */
      }
