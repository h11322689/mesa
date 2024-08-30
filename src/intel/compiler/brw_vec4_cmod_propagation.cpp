   /*
   * Copyright © 2015 Intel Corporation
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
   *
   */
      /** @file brw_vec4_cmod_propagation.cpp
   *
   * Really similar to brw_fs_cmod_propagation but adapted to vec4 needs. Check
   * brw_fs_cmod_propagation for further details on the rationale behind this
   * optimization.
   */
      #include "brw_vec4.h"
   #include "brw_cfg.h"
   #include "brw_eu.h"
      namespace brw {
      static bool
   writemasks_incompatible(const vec4_instruction *earlier,
         {
      return (earlier->dst.writemask != WRITEMASK_X &&
         earlier->dst.writemask != WRITEMASK_XYZW) ||
   (earlier->dst.writemask == WRITEMASK_XYZW &&
      }
      static bool
   opt_cmod_propagation_local(bblock_t *block, vec4_visitor *v)
   {
      bool progress = false;
            foreach_inst_in_block_reverse_safe(vec4_instruction, inst, block) {
               if ((inst->opcode != BRW_OPCODE_AND &&
      inst->opcode != BRW_OPCODE_CMP &&
   inst->opcode != BRW_OPCODE_MOV) ||
   inst->predicate != BRW_PREDICATE_NONE ||
   !inst->dst.is_null() ||
   (inst->src[0].file != VGRF && inst->src[0].file != ATTR &&
               /* An ABS source modifier can only be handled when processing a compare
   * with a value other than zero.
   */
   if (inst->src[0].abs &&
                  if (inst->opcode == BRW_OPCODE_AND &&
      !(inst->src[1].is_one() &&
      inst->conditional_mod == BRW_CONDITIONAL_NZ &&
            if (inst->opcode == BRW_OPCODE_MOV &&
                  bool read_flag = false;
   foreach_inst_in_block_reverse_starting_from(vec4_instruction, scan_inst, inst) {
      /* A CMP with a second source of zero can match with anything.  A CMP
   * with a second source that is not zero can only match with an ADD
   * instruction.
   */
                                                   /* A CMP is basically a subtraction.  The result of the
   * subtraction must be the same as the result of the addition.
   * This means that one of the operands must be negated.  So (a +
   * b) vs (a == -b) or (a + -b) vs (a == b).
   */
   if ((inst->src[0].equals(scan_inst->src[0]) &&
      inst->src[1].negative_equals(scan_inst->src[1])) ||
   (inst->src[0].equals(scan_inst->src[1]) &&
   inst->src[1].negative_equals(scan_inst->src[0]))) {
      } else if ((inst->src[0].negative_equals(scan_inst->src[0]) &&
                  (inst->src[0].negative_equals(scan_inst->src[1]) &&
      } else {
                  if (scan_inst->exec_size != inst->exec_size ||
                  /* From the Sky Lake PRM Vol. 7 "Assigning Conditional Mods":
   *
   *    * Note that the [post condition signal] bits generated at
   *      the output of a compute are before the .sat.
   *
                  /* Otherwise, try propagating the conditional. */
   const enum brw_conditional_mod cond =
                  if (scan_inst->can_do_cmod() &&
      ((!read_flag && scan_inst->conditional_mod == BRW_CONDITIONAL_NONE) ||
   scan_inst->conditional_mod == cond)) {
   scan_inst->conditional_mod = cond;
   inst->remove(block);
      }
               if (regions_overlap(inst->src[0], inst->size_read(0),
            if ((scan_inst->predicate && scan_inst->opcode != BRW_OPCODE_SEL) ||
      scan_inst->dst.offset != inst->src[0].offset ||
   scan_inst->exec_size != inst->exec_size ||
                     /* If scan_inst is a CMP that produces a single value and inst is
   * a CMP.NZ that consumes only that value, remove inst.
   */
   if (inst->conditional_mod == BRW_CONDITIONAL_NZ &&
      (inst->src[0].type == BRW_REGISTER_TYPE_D ||
   inst->src[0].type == BRW_REGISTER_TYPE_UD) &&
   (inst->opcode == BRW_OPCODE_CMP ||
   inst->opcode == BRW_OPCODE_MOV) &&
   scan_inst->opcode == BRW_OPCODE_CMP &&
   ((inst->src[0].swizzle == BRW_SWIZZLE_XXXX &&
         (inst->src[0].swizzle == BRW_SWIZZLE_YYYY &&
         (inst->src[0].swizzle == BRW_SWIZZLE_ZZZZ &&
         (inst->src[0].swizzle == BRW_SWIZZLE_WWWW &&
                           /* Given a sequence like:
   *
   *    cmp.ge.f0(8)  g21<1>.zF      g20<4>.xF      g18<4>.xF
   *    ...
   *    cmp.nz.f0(8)  null<1>D       g21<4>.zD      0D
   *
   * Replace it with something like:
   *
   *    cmp.ge.f0(8)  g22<1>.zF      g20<4>.xF      g18<4>.xF
   *    mov(8)        g21<1>.xF      g22<1>.zzzzF
   *
   * The added MOV will most likely be removed later.  In the
   * worst case, it should be cheaper to schedule.
                                 /* Modify the source swizzles on scan_inst.  If scan_inst
   * was
   *
   *    cmp.ge.f0(8)  g21<1>.zF      g20<4>.wzyxF   g18<4>.yxwzF
   *
   * replace it with
   *
   *    cmp.ge.f0(8)  g21<1>.zF      g20<4>.yyyyF   g18<4>.wwwwF
   */
   unsigned src0_chan;
   unsigned src1_chan;
   switch (scan_inst->dst.writemask) {
   case WRITEMASK_X:
      src0_chan = BRW_GET_SWZ(scan_inst->src[0].swizzle, 0);
   src1_chan = BRW_GET_SWZ(scan_inst->src[1].swizzle, 0);
      case WRITEMASK_Y:
      src0_chan = BRW_GET_SWZ(scan_inst->src[0].swizzle, 1);
   src1_chan = BRW_GET_SWZ(scan_inst->src[1].swizzle, 1);
      case WRITEMASK_Z:
      src0_chan = BRW_GET_SWZ(scan_inst->src[0].swizzle, 2);
   src1_chan = BRW_GET_SWZ(scan_inst->src[1].swizzle, 2);
      case WRITEMASK_W:
      src0_chan = BRW_GET_SWZ(scan_inst->src[0].swizzle, 3);
   src1_chan = BRW_GET_SWZ(scan_inst->src[1].swizzle, 3);
                           scan_inst->src[0].swizzle = BRW_SWIZZLE4(src0_chan,
                        /* There's no swizzle on immediate value sources. */
   if (scan_inst->src[1].file != IMM) {
      scan_inst->src[1].swizzle = BRW_SWIZZLE4(src1_chan,
                                                      inst->remove(block);
                                    /* CMP's result is the same regardless of dest type. */
   if (inst->conditional_mod == BRW_CONDITIONAL_NZ &&
      scan_inst->opcode == BRW_OPCODE_CMP &&
   (inst->dst.type == BRW_REGISTER_TYPE_D ||
   inst->dst.type == BRW_REGISTER_TYPE_UD)) {
   inst->remove(block);
                     /* If the AND wasn't handled by the previous case, it isn't safe
   * to remove it.
   */
                  /* Comparisons operate differently for ints and floats */
   if (scan_inst->dst.type != inst->dst.type &&
                        /* If the instruction generating inst's source also wrote the
   * flag, and inst is doing a simple .nz comparison, then inst
   * is redundant - the appropriate value is already in the flag
   * register.  Delete inst.
   */
   if (inst->conditional_mod == BRW_CONDITIONAL_NZ &&
      !inst->src[0].negate &&
   scan_inst->writes_flag(v->devinfo)) {
   inst->remove(block);
                     /* The conditional mod of the CMP/CMPN instructions behaves
   * specially because the flag output is not calculated from the
   * result of the instruction, but the other way around, which
   * means that even if the condmod to propagate and the condmod
   * from the CMP instruction are the same they will in general give
   * different results because they are evaluated based on different
   * inputs.
   */
   if (scan_inst->opcode == BRW_OPCODE_CMP ||
                  /* From the Sky Lake PRM Vol. 7 "Assigning Conditional Mods":
   *
   *    * Note that the [post condition signal] bits generated at
   *      the output of a compute are before the .sat.
   */
                  /* From the Sky Lake PRM, Vol 2a, "Multiply":
   *
   *    "When multiplying integer data types, if one of the sources
   *    is a DW, the resulting full precision data is stored in
   *    the accumulator. However, if the destination data type is
   *    either W or DW, the low bits of the result are written to
   *    the destination register and the remaining high bits are
   *    discarded. This results in undefined Overflow and Sign
   *    flags. Therefore, conditional modifiers and saturation
   *    (.sat) cannot be used in this case.
   *
   * We just disallow cmod propagation on all integer multiplies.
   */
   if (!brw_reg_type_is_floating_point(scan_inst->dst.type) &&
                  /* Otherwise, try propagating the conditional. */
   enum brw_conditional_mod cond =
                  if (scan_inst->can_do_cmod() &&
      ((!read_flag && scan_inst->conditional_mod == BRW_CONDITIONAL_NONE) ||
   scan_inst->conditional_mod == cond)) {
   scan_inst->conditional_mod = cond;
   inst->remove(block);
      }
            not_match:
                                       }
      bool
   vec4_visitor::opt_cmod_propagation()
   {
               foreach_block_reverse(block, cfg) {
                  if (progress)
               }
      } /* namespace brw */
