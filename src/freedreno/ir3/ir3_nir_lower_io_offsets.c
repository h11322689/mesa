   /*
   * Copyright © 2018-2019 Igalia S.L.
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
      #include "compiler/nir/nir_builder.h"
   #include "ir3_nir.h"
      /**
   * This pass moves to NIR certain offset computations for different I/O
   * ops that are currently implemented on the IR3 backend compiler, to
   * give NIR a chance to optimize them:
   *
   * - Dword-offset for SSBO load, store and atomics: A new, similar intrinsic
   *   is emitted that replaces the original one, adding a new source that
   *   holds the result of the original byte-offset source divided by 4.
   */
      /* Returns the ir3-specific intrinsic opcode corresponding to an SSBO
   * instruction that is handled by this pass. It also conveniently returns
   * the offset source index in @offset_src_idx.
   *
   * If @intrinsic is not SSBO, or it is not handled by the pass, -1 is
   * returned.
   */
   static int
   get_ir3_intrinsic_for_ssbo_intrinsic(unsigned intrinsic,
         {
                        switch (intrinsic) {
   case nir_intrinsic_store_ssbo:
      *offset_src_idx = 2;
      case nir_intrinsic_load_ssbo:
         case nir_intrinsic_ssbo_atomic:
         case nir_intrinsic_ssbo_atomic_swap:
         default:
                     }
      static nir_def *
   check_and_propagate_bit_shift32(nir_builder *b, nir_alu_instr *alu_instr,
         {
               /* Only propagate if the shift is a const value so we can check value range
   * statically.
   */
   nir_const_value *const_val = nir_src_as_const_value(alu_instr->src[1].src);
   if (!const_val)
            int32_t current_shift = const_val[0].i32 * direction;
            /* If the merge would reverse the direction, bail out.
   * e.g, 'x << 2' then 'x >> 4' is not 'x >> 2'.
   */
   if (current_shift * new_shift < 0)
            /* If the propagation would overflow an int32_t, bail out too to be on the
   * safe side.
   */
   if (new_shift < -31 || new_shift > 31)
            /* Add or substract shift depending on the final direction (SHR vs. SHL). */
   if (shift * direction < 0)
         else
               }
      nir_def *
   ir3_nir_try_propagate_bit_shift(nir_builder *b, nir_def *offset,
         {
      nir_instr *offset_instr = offset->parent_instr;
   if (offset_instr->type != nir_instr_type_alu)
            nir_alu_instr *alu = nir_instr_as_alu(offset_instr);
   nir_def *shift_ssa;
            /* the first src could be something like ssa_18.x, but we only want
   * the single component.  Otherwise the ishl/ishr/ushr could turn
   * into a vec4 operation:
   */
            switch (alu->op) {
   case nir_op_ishl:
      shift_ssa = check_and_propagate_bit_shift32(b, alu, 1, shift);
   if (shift_ssa)
            case nir_op_ishr:
      shift_ssa = check_and_propagate_bit_shift32(b, alu, -1, shift);
   if (shift_ssa)
            case nir_op_ushr:
      shift_ssa = check_and_propagate_bit_shift32(b, alu, -1, shift);
   if (shift_ssa)
            default:
                     }
      /* isam doesn't have an "untyped" field, so it can only load 1 component at a
   * time because our storage buffer descriptors use a 1-component format.
   * Therefore we need to scalarize any loads that would use isam.
   */
   static void
   scalarize_load(nir_intrinsic_instr *intrinsic, nir_builder *b)
   {
               nir_def *descriptor = intrinsic->src[0].ssa;
   nir_def *offset = intrinsic->src[1].ssa;
   nir_def *new_offset = intrinsic->src[2].ssa;
   unsigned comp_size = intrinsic->def.bit_size / 8;
   for (unsigned i = 0; i < intrinsic->def.num_components; i++) {
      results[i] =
      nir_load_ssbo_ir3(b, 1, intrinsic->def.bit_size, descriptor,
                     nir_iadd_imm(b, offset, i * comp_size),
                                 }
      static bool
   lower_offset_for_ssbo(nir_intrinsic_instr *intrinsic, nir_builder *b,
         {
      unsigned num_srcs = nir_intrinsic_infos[intrinsic->intrinsic].num_srcs;
            bool has_dest = nir_intrinsic_infos[intrinsic->intrinsic].has_dest;
            /* for 16-bit ssbo access, offset is in 16-bit words instead of dwords */
   if ((has_dest && intrinsic->def.bit_size == 16) ||
      (!has_dest && intrinsic->src[0].ssa->bit_size == 16))
         /* Here we create a new intrinsic and copy over all contents from the old
            nir_intrinsic_instr *new_intrinsic;
                     /* 'offset_src_idx' holds the index of the source that represent the offset. */
                     /* Since we don't have value range checking, we first try to propagate
   * the division by 4 ('offset >> 2') into another bit-shift instruction that
   * possibly defines the offset. If that's the case, we emit a similar
   * instructions adjusting (merging) the shift value.
   *
   * Here we use the convention that shifting right is negative while shifting
   * left is positive. So 'x / 4' ~ 'x >> 2' or 'x << -2'.
   */
            /* The new source that will hold the dword-offset is always the last
   * one for every intrinsic.
   */
   target_src = &new_intrinsic->src[num_srcs];
            if (has_dest) {
      nir_def *dest = &intrinsic->def;
   nir_def_init(&new_intrinsic->instr, &new_intrinsic->def,
                     for (unsigned i = 0; i < num_srcs; i++)
                              /* If we managed to propagate the division by 4, just use the new offset
   * register and don't emit the SHR.
   */
   if (new_offset)
         else
            /* Insert the new intrinsic right before the old one. */
            /* Replace the last source of the new intrinsic by the result of
   * the offset divided by 4.
   */
            if (has_dest) {
      /* Replace the uses of the original destination by that
   * of the new intrinsic.
   */
               /* Finally remove the original intrinsic. */
            if (new_intrinsic->intrinsic == nir_intrinsic_load_ssbo_ir3 &&
      (nir_intrinsic_access(new_intrinsic) & ACCESS_CAN_REORDER) &&
   ir3_bindless_resource(new_intrinsic->src[0]) &&
   new_intrinsic->num_components > 1)
            }
      static bool
   lower_io_offsets_block(nir_block *block, nir_builder *b, void *mem_ctx)
   {
               nir_foreach_instr_safe (instr, block) {
      if (instr->type != nir_instr_type_intrinsic)
                     /* SSBO */
   int ir3_intrinsic;
   uint8_t offset_src_idx;
   ir3_intrinsic =
         if (ir3_intrinsic != -1) {
      progress |= lower_offset_for_ssbo(intr, b, (unsigned)ir3_intrinsic,
                     }
      static bool
   lower_io_offsets_func(nir_function_impl *impl)
   {
      void *mem_ctx = ralloc_parent(impl);
            bool progress = false;
   nir_foreach_block_safe (block, impl) {
                  if (progress) {
      nir_metadata_preserve(impl,
                  }
      bool
   ir3_nir_lower_io_offsets(nir_shader *shader)
   {
               nir_foreach_function (function, shader) {
      if (function->impl)
                  }
