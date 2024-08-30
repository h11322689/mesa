   /*
   * Copyright © 2013 Intel Corporation
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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
         #include "brw_vec4_vs.h"
   #include "dev/intel_debug.h"
      namespace brw {
      void
   vec4_vs_visitor::emit_prolog()
   {
   }
         void
   vec4_vs_visitor::emit_urb_write_header(int mrf)
   {
      /* No need to do anything for VS; an implied write to this MRF will be
   * performed by VEC4_VS_OPCODE_URB_WRITE.
   */
      }
         vec4_instruction *
   vec4_vs_visitor::emit_urb_write_opcode(bool complete)
   {
      vec4_instruction *inst = emit(VEC4_VS_OPCODE_URB_WRITE);
   inst->urb_write_flags = complete ?
               }
         void
   vec4_vs_visitor::emit_urb_slot(dst_reg reg, int varying)
   {
      reg.type = BRW_REGISTER_TYPE_F;
            switch (varying) {
   case VARYING_SLOT_COL0:
   case VARYING_SLOT_COL1:
   case VARYING_SLOT_BFC0:
   case VARYING_SLOT_BFC1: {
      /* These built-in varyings are only supported in compatibility mode,
   * and we only support GS in core profile.  So, this must be a vertex
   * shader.
   */
   vec4_instruction *inst = emit_generic_urb_slot(reg, varying, 0);
   if (inst && key->clamp_vertex_color)
            }
   default:
            }
         void
   vec4_vs_visitor::emit_thread_end()
   {
      /* For VS, we always end the thread by emitting a single vertex.
   * emit_urb_write_opcode() will take care of setting the eot flag on the
   * SEND instruction.
   */
      }
         vec4_vs_visitor::vec4_vs_visitor(const struct brw_compiler *compiler,
                                    : vec4_visitor(compiler, params, &key->base.tex, &vs_prog_data->base,
         key(key),
      {
   }
         } /* namespace brw */
