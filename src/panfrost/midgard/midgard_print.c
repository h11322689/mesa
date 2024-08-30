   /*
   * Copyright (C) 2018-2019 Alyssa Rosenzweig <alyssa@rosenzweig.io>
   * Copyright (C) 2019-2020 Collabora, Ltd.
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
   */
      #include <math.h>
      #include "util/bitscan.h"
   #include "util/half_float.h"
   #include "compiler.h"
   #include "helpers.h"
   #include "midgard_ops.h"
      /* Pretty printer for Midgard IR, for use debugging compiler-internal
   * passes like register allocation. The output superficially resembles
   * Midgard assembly, with the exception that unit information and such is
   * (normally) omitted, and generic indices are usually used instead of
   * registers */
      static void
   mir_print_index(int source)
   {
      if (source == ~0) {
      printf("_");
               if (source >= SSA_FIXED_MINIMUM) {
      /* Specific register */
            /* TODO: Moving threshold */
   if (reg > 16 && reg < 24)
         else
      } else if (source & PAN_IS_REG) {
         } else {
            }
      static const char components[16] = "xyzwefghijklmnop";
      static void
   mir_print_mask(unsigned mask)
   {
               for (unsigned i = 0; i < 16; ++i) {
      if (mask & (1 << i))
         }
      /*
   * Print a swizzle. We only print the components enabled by the corresponding
   * writemask, as the other components will be ignored by the hardware and so
   * don't matter.
   */
   static void
   mir_print_swizzle(unsigned mask, unsigned *swizzle)
   {
               for (unsigned i = 0; i < 16; ++i) {
      if (mask & BITFIELD_BIT(i)) {
      unsigned C = swizzle[i];
            }
      static const char *
   mir_get_unit(unsigned unit)
   {
      switch (unit) {
   case ALU_ENAB_VEC_MUL:
         case ALU_ENAB_SCAL_ADD:
         case ALU_ENAB_VEC_ADD:
         case ALU_ENAB_SCAL_MUL:
         case ALU_ENAB_VEC_LUT:
         case ALU_ENAB_BR_COMPACT:
         case ALU_ENAB_BRANCH:
         default:
            }
      static void
   mir_print_embedded_constant(midgard_instruction *ins, unsigned src_idx)
   {
               unsigned base_size = max_bitsize_for_alu(ins);
   unsigned sz = nir_alu_type_get_type_size(ins->src_types[src_idx]);
   bool half = (sz == (base_size >> 1));
   unsigned mod = mir_pack_mod(ins, src_idx, false);
   unsigned *swizzle = ins->swizzle[src_idx];
   midgard_reg_mode reg_mode = reg_mode_for_bitsize(max_bitsize_for_alu(ins));
   unsigned comp_mask = effective_writemask(ins->op, ins->mask);
   unsigned num_comp = util_bitcount(comp_mask);
   unsigned max_comp = mir_components_for_type(ins->dest_type);
                     if (num_comp > 1)
            for (unsigned comp = 0; comp < max_comp; comp++) {
      if (!(comp_mask & (1 << comp)))
            if (first)
         else
            mir_print_constant_component(stdout, &ins->constants, swizzle[comp],
               if (num_comp > 1)
      }
      static void
   mir_print_src(midgard_instruction *ins, unsigned c)
   {
               if (ins->src[c] != ~0 && ins->src_types[c] != nir_type_invalid) {
      pan_print_alu_type(ins->src_types[c], stdout);
         }
      void
   mir_print_instruction(midgard_instruction *ins)
   {
               if (midgard_is_branch_unit(ins->unit)) {
      const char *branch_target_names[] = {"goto", "break", "continue",
            printf("%s.", mir_get_unit(ins->unit));
   if (ins->branch.target_type == TARGET_DISCARD)
         else if (ins->writeout)
         else if (ins->unit == ALU_ENAB_BR_COMPACT && !ins->branch.conditional)
         else
            if (!ins->branch.conditional)
         else if (ins->branch.invert_conditional)
         else
            if (ins->writeout) {
      printf(" (c: ");
   mir_print_src(ins, 0);
   printf(", z: ");
   mir_print_src(ins, 2);
   printf(", s: ");
   mir_print_src(ins, 3);
               if (ins->branch.target_type != TARGET_DISCARD)
      printf(" %s -> block(%d)\n",
         ins->branch.target_type < 4
                           switch (ins->type) {
   case TAG_ALU_4: {
      midgard_alu_op op = ins->op;
            if (ins->unit)
                     if (!(midgard_is_integer_out_op(ins->op) &&
            mir_print_outmod(stdout, ins->outmod,
                           case TAG_LOAD_STORE_4: {
      midgard_load_store_op op = ins->op;
            assert(name);
   printf("%s", name);
               case TAG_TEXTURE_4: {
               if (ins->helper_terminate)
            if (ins->helper_execute)
                        default:
                  if (ins->compact_branch && ins->branch.invert_conditional)
            printf(" ");
            if (ins->dest != ~0) {
      pan_print_alu_type(ins->dest_type, stdout);
                        /* Only ALU can have an embedded constant, r26 as read on load/store is
   * something else entirely */
   bool is_alu = ins->type == TAG_ALU_4;
            if (is_alu && alu_opcode_props[ins->op].props & QUIRK_FLIPPED_R24) {
      /* Moves (indicated by QUIRK_FLIPPED_R24) are 1-src, with their
   * one source in the second slot
   */
      } else {
      if (ins->src[0] == r_constant && is_alu)
         else
                        if (ins->has_inline_constant)
         else if (ins->src[1] == r_constant && is_alu)
         else
            if (is_alu) {
      /* ALU ops are all 2-src, though CSEL is treated like a 3-src
   * pseudo op with the third source scheduler lowered
   */
   switch (ins->op) {
   case midgard_alu_op_icsel:
   case midgard_alu_op_fcsel:
   case midgard_alu_op_icsel_v:
   case midgard_alu_op_fcsel_v:
      printf(", ");
   mir_print_src(ins, 2);
      default:
      assert(ins->src[2] == ~0);
                  } else {
      for (unsigned c = 2; c <= 3; ++c) {
      printf(", ");
                  if (ins->no_spill)
               }
      /* Dumps MIR for a block or entire shader respective */
      void
   mir_print_block(midgard_block *block)
   {
               if (block->scheduled) {
      mir_foreach_bundle_in_block(block, bundle) {
                           } else {
      mir_foreach_instr_in_block(block, ins) {
                              if (block->base.successors[0]) {
      printf(" -> ");
   pan_foreach_successor((&block->base), succ)
               printf(" from { ");
   mir_foreach_predecessor(block, pred)
                     }
      void
   mir_print_shader(compiler_context *ctx)
   {
      mir_foreach_block(ctx, block) {
            }
