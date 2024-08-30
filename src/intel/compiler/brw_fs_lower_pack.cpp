   /*
   * Copyright © 2015 Connor Abbott
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
      #include "util/half_float.h"
   #include "brw_fs.h"
   #include "brw_cfg.h"
   #include "brw_fs_builder.h"
      using namespace brw;
      bool
   fs_visitor::lower_pack()
   {
               foreach_block_and_inst_safe(block, fs_inst, inst, cfg) {
      if (inst->opcode != FS_OPCODE_PACK &&
                  assert(inst->dst.file == VGRF);
   assert(inst->saturate == false);
            const fs_builder ibld(this, block, inst);
   /* The lowering generates 2 instructions for what was previously 1. This
   * can trick the IR to believe we're doing partial writes, but the
   * register is actually fully written. Mark it as undef to help the IR
   * reduce the liveness of the register.
   */
   if (!inst->is_partial_write())
            switch (inst->opcode) {
   case FS_OPCODE_PACK:
      for (unsigned i = 0; i < inst->sources; i++)
            case FS_OPCODE_PACK_HALF_2x16_SPLIT:
               for (unsigned i = 0; i < inst->sources; i++) {
      if (inst->src[i].file == IMM) {
      const uint32_t half = _mesa_float_to_half(inst->src[i].f);
   ibld.MOV(subscript(dst, BRW_REGISTER_TYPE_UW, i),
      } else if (i == 1 && devinfo->ver < 9) {
      /* Pre-Skylake requires DWord aligned destinations */
   fs_reg tmp = ibld.vgrf(BRW_REGISTER_TYPE_UD);
   ibld.F32TO16(subscript(tmp, BRW_REGISTER_TYPE_HF, 0),
         ibld.MOV(subscript(dst, BRW_REGISTER_TYPE_UW, 1),
      } else {
      ibld.F32TO16(subscript(dst, BRW_REGISTER_TYPE_HF, i),
         }
      default:
                  inst->remove(block);
               if (progress)
               }
