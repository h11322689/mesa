   /*
   * Copyright © 2016 Broadcom Limited
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
      #include "vc4_qir.h"
   #include "vc4_qpu.h"
      static void
   fail_instr(struct vc4_compile *c, struct qinst *inst, const char *msg)
   {
         fprintf(stderr, "qir_validate: %s: ", msg);
   qir_dump_inst(c, inst);
   fprintf(stderr, "\n");
   }
      void qir_validate(struct vc4_compile *c)
   {
         bool already_assigned[c->num_temps];
            /* We don't want to do validation in release builds, but we want to
      * keep compiling the validation code to make sure it doesn't get
      #ifndef DEBUG
         #endif
            for (int i = 0; i < c->num_temps; i++) {
                                 qir_for_each_inst_inorder(inst, c) {
            switch (inst->dst.file) {
                                 if (c->defs[inst->dst.index] &&
                           case QFILE_NULL:
   case QFILE_VPM:
   case QFILE_TLB_COLOR_WRITE:
   case QFILE_TLB_COLOR_WRITE_MS:
                        case QFILE_VARY:
   case QFILE_UNIF:
   case QFILE_FRAG_X:
   case QFILE_FRAG_Y:
   case QFILE_FRAG_REV_FLAG:
   case QFILE_QPU_ELEMENT:
   case QFILE_SMALL_IMM:
                        case QFILE_TEX_S:
   case QFILE_TEX_T:
   case QFILE_TEX_R:
   case QFILE_TEX_B:
            if (inst->src[qir_get_tex_uniform_src(inst)].file !=
                           case QFILE_TEX_S_DIRECT:
            if (inst->op != QOP_ADD) {
         fail_instr(c, inst,
                                             switch (src.file) {
                              case QFILE_VARY:
   case QFILE_UNIF:
                                                         case QFILE_FRAG_X:
   case QFILE_FRAG_Y:
                              case QFILE_NULL:
   case QFILE_TLB_COLOR_WRITE:
   case QFILE_TLB_COLOR_WRITE_MS:
   case QFILE_TLB_Z_WRITE:
   case QFILE_TLB_STENCIL_SETUP:
   case QFILE_TEX_S_DIRECT:
   case QFILE_TEX_S:
   case QFILE_TEX_T:
   case QFILE_TEX_R:
   case QFILE_TEX_B:
         }
