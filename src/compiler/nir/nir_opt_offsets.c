   /*
   * Copyright © 2021 Valve Corporation
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
   * Authors:
   *    Timur Kristóf
   *
   */
      #include "nir.h"
   #include "nir_builder.h"
      typedef struct
   {
      struct hash_table *range_ht;
      } opt_offsets_state;
      static nir_scalar
   try_extract_const_addition(nir_builder *b, nir_scalar val, opt_offsets_state *state, unsigned *out_const, uint32_t max)
   {
               if (!nir_scalar_is_alu(val))
            nir_alu_instr *alu = nir_instr_as_alu(val.def->parent_instr);
   if (alu->op != nir_op_iadd)
            nir_scalar src[2] = {
      { alu->src[0].src.ssa, alu->src[0].swizzle[val.comp] },
               /* Make sure that we aren't taking out an addition that could trigger
   * unsigned wrapping in a way that would change the semantics of the load.
   * Ignored for ints-as-floats (lower_bitops is a proxy for that), where
   * unsigned wrapping doesn't make sense.
   */
   if (!alu->no_unsigned_wrap && !b->shader->options->lower_bitops) {
      if (!state->range_ht) {
      /* Cache for nir_unsigned_upper_bound */
               /* Check if there can really be an unsigned wrap. */
   uint32_t ub0 = nir_unsigned_upper_bound(b->shader, state->range_ht, src[0], NULL);
            if ((UINT32_MAX - ub0) < ub1)
            /* We proved that unsigned wrap won't be possible, so we can set the flag too. */
               for (unsigned i = 0; i < 2; ++i) {
      src[i] = nir_scalar_chase_movs(src[i]);
   if (nir_scalar_is_const(src[i])) {
      uint32_t offset = nir_scalar_as_uint(src[i]);
   if (offset + *out_const <= max) {
      *out_const += offset;
                     uint32_t orig_offset = *out_const;
   src[0] = try_extract_const_addition(b, src[0], state, out_const, max);
   src[1] = try_extract_const_addition(b, src[1], state, out_const, max);
   if (*out_const == orig_offset)
            b->cursor = nir_before_instr(&alu->instr);
   nir_def *r =
      nir_iadd(b, nir_channel(b, src[0].def, src[0].comp),
         }
      static bool
   try_fold_load_store(nir_builder *b,
                     nir_intrinsic_instr *intrin,
   {
      /* Assume that BASE is the constant offset of a load/store.
   * Try to constant-fold additions to the offset source
   * into the actual const offset of the instruction.
            unsigned off_const = nir_intrinsic_base(intrin);
   nir_src *off_src = &intrin->src[offset_src_idx];
            if (off_src->ssa->bit_size != 32)
            if (!nir_src_is_const(*off_src)) {
      uint32_t add_offset = 0;
   nir_scalar val = { .def = off_src->ssa, .comp = 0 };
   val = try_extract_const_addition(b, val, state, &add_offset, max - off_const);
   if (add_offset == 0)
         off_const += add_offset;
   b->cursor = nir_before_instr(&intrin->instr);
      } else if (nir_src_as_uint(*off_src) && off_const + nir_src_as_uint(*off_src) <= max) {
      off_const += nir_src_as_uint(*off_src);
   b->cursor = nir_before_instr(&intrin->instr);
               if (!replace_src)
                     assert(off_const <= max);
   nir_intrinsic_set_base(intrin, off_const);
      }
      static bool
   try_fold_shared2(nir_builder *b,
                     {
      unsigned comp_size = (intrin->intrinsic == nir_intrinsic_load_shared2_amd ? intrin->def.bit_size : intrin->src[0].ssa->bit_size) / 8;
   unsigned stride = (nir_intrinsic_st64(intrin) ? 64 : 1) * comp_size;
   unsigned offset0 = nir_intrinsic_offset0(intrin) * stride;
   unsigned offset1 = nir_intrinsic_offset1(intrin) * stride;
            if (!nir_src_is_const(*off_src))
            unsigned const_offset = nir_src_as_uint(*off_src);
   offset0 += const_offset;
   offset1 += const_offset;
   bool st64 = offset0 % (64 * comp_size) == 0 && offset1 % (64 * comp_size) == 0;
   stride = (st64 ? 64 : 1) * comp_size;
   if (const_offset % stride || offset0 > 255 * stride || offset1 > 255 * stride)
            b->cursor = nir_before_instr(&intrin->instr);
   nir_src_rewrite(off_src, nir_imm_zero(b, 1, 32));
   nir_intrinsic_set_offset0(intrin, offset0 / stride);
   nir_intrinsic_set_offset1(intrin, offset1 / stride);
               }
      static bool
   process_instr(nir_builder *b, nir_instr *instr, void *s)
   {
      if (instr->type != nir_instr_type_intrinsic)
            opt_offsets_state *state = (opt_offsets_state *)s;
            switch (intrin->intrinsic) {
   case nir_intrinsic_load_uniform:
         case nir_intrinsic_load_ubo_vec4:
         case nir_intrinsic_load_shared:
   case nir_intrinsic_load_shared_ir3:
         case nir_intrinsic_store_shared:
   case nir_intrinsic_store_shared_ir3:
         case nir_intrinsic_load_shared2_amd:
         case nir_intrinsic_store_shared2_amd:
         case nir_intrinsic_load_buffer_amd:
         case nir_intrinsic_store_buffer_amd:
         default:
                     }
      bool
   nir_opt_offsets(nir_shader *shader, const nir_opt_offsets_options *options)
   {
      opt_offsets_state state;
   state.range_ht = NULL;
            bool p = nir_shader_instructions_pass(shader, process_instr,
                        if (state.range_ht)
               }
