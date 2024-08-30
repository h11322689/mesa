   /*
   * Copyright (C) 2019 Collabora, Ltd.
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
   * Authors (Collabora):
   *    Alyssa Rosenzweig <alyssa.rosenzweig@collabora.com>
   */
      #include "compiler.h"
      /* Midgard's generic load/store instructions, particularly to implement SSBOs
   * and globals, have support for address arithmetic natively. In particularly,
   * they take two indirect arguments A, B and two immediates #s, #c, calculating
   * the address:
   *
   *      A + (zext?(B) << #s) + #c
   *
   * This allows for fast indexing into arrays. This file tries to pattern match
   * the offset in NIR with this form to reduce pressure on the ALU pipe.
   */
      struct mir_address {
      nir_scalar A;
            midgard_index_address_format type;
   unsigned shift;
      };
      static bool
   mir_args_ssa(nir_scalar s, unsigned count)
   {
               if (count > nir_op_infos[alu->op].num_inputs)
               }
      /* Matches a constant in either slot and moves it to the bias */
      static void
   mir_match_constant(struct mir_address *address)
   {
      if (address->A.def && nir_scalar_is_const(address->A)) {
      address->bias += nir_scalar_as_uint(address->A);
               if (address->B.def && nir_scalar_is_const(address->B)) {
      address->bias += nir_scalar_as_uint(address->B);
         }
      /* Matches an iadd when there is a free slot or constant */
      /* The offset field is a 18-bit signed integer */
   #define MAX_POSITIVE_OFFSET ((1 << 17) - 1)
      static void
   mir_match_iadd(struct mir_address *address, bool first_free)
   {
      if (!address->B.def || !nir_scalar_is_alu(address->B))
            if (!mir_args_ssa(address->B, 2))
                     if (op != nir_op_iadd)
            nir_scalar op1 = nir_scalar_chase_alu_src(address->B, 0);
            if (nir_scalar_is_const(op1) &&
      nir_scalar_as_uint(op1) <= MAX_POSITIVE_OFFSET) {
   address->bias += nir_scalar_as_uint(op1);
      } else if (nir_scalar_is_const(op2) &&
            address->bias += nir_scalar_as_uint(op2);
      } else if (!nir_scalar_is_const(op1) && !nir_scalar_is_const(op2) &&
            address->A = op1;
         }
      /* Matches u2u64 and sets type */
      static void
   mir_match_u2u64(struct mir_address *address)
   {
      if (!address->B.def || !nir_scalar_is_alu(address->B))
            if (!mir_args_ssa(address->B, 1))
            nir_op op = nir_scalar_alu_op(address->B);
   if (op != nir_op_u2u64)
                  address->B = arg;
      }
      /* Matches i2i64 and sets type */
      static void
   mir_match_i2i64(struct mir_address *address)
   {
      if (!address->B.def || !nir_scalar_is_alu(address->B))
            if (!mir_args_ssa(address->B, 1))
            nir_op op = nir_scalar_alu_op(address->B);
   if (op != nir_op_i2i64)
                  address->B = arg;
      }
      /* Matches ishl to shift */
      static void
   mir_match_ishl(struct mir_address *address)
   {
      if (!address->B.def || !nir_scalar_is_alu(address->B))
            if (!mir_args_ssa(address->B, 2))
            nir_op op = nir_scalar_alu_op(address->B);
   if (op != nir_op_ishl)
         nir_scalar op1 = nir_scalar_chase_alu_src(address->B, 0);
            if (!nir_scalar_is_const(op2))
            unsigned shift = nir_scalar_as_uint(op2);
   if (shift > 0x7)
            address->B = op1;
      }
      /* Strings through mov which can happen from NIR vectorization */
      static void
   mir_match_mov(struct mir_address *address)
   {
      if (address->A.def && nir_scalar_is_alu(address->A)) {
               if (op == nir_op_mov && mir_args_ssa(address->A, 1))
               if (address->B.def && nir_scalar_is_alu(address->B)) {
               if (op == nir_op_mov && mir_args_ssa(address->B, 1))
         }
      /* Tries to pattern match into mir_address */
      static struct mir_address
   mir_match_offset(nir_def *offset, bool first_free, bool extend)
   {
      struct mir_address address = {
      .B = {.def = offset},
               mir_match_mov(&address);
   mir_match_constant(&address);
   mir_match_mov(&address);
   mir_match_iadd(&address, first_free);
            if (extend) {
      mir_match_u2u64(&address);
   mir_match_i2i64(&address);
                           }
      void
   mir_set_offset(compiler_context *ctx, midgard_instruction *ins, nir_src *offset,
         {
      for (unsigned i = 0; i < 16; ++i) {
      ins->swizzle[1][i] = 0;
               /* Sign extend instead of zero extend in case the address is something
   * like `base + offset + 20`, where offset could be negative. */
   bool force_sext = (nir_src_bit_size(*offset) < 64);
                     if (match.A.def) {
      unsigned bitsize = match.A.def->bit_size;
            ins->src[1] = nir_ssa_index(match.A.def);
   ins->swizzle[1][0] = match.A.comp;
   ins->src_types[1] = nir_type_uint | bitsize;
      } else {
      ins->load_store.bitsize_toggle = true;
   ins->load_store.arg_comp = seg & 0x3;
               if (match.B.def) {
      ins->src[2] = nir_ssa_index(match.B.def);
   ins->swizzle[2][0] = match.B.comp;
      } else
            if (force_sext)
                     assert(match.shift <= 7);
               }
      void
   mir_set_ubo_offset(midgard_instruction *ins, nir_src *src, unsigned bias)
   {
               if (match.B.def) {
               for (unsigned i = 0; i < ARRAY_SIZE(ins->swizzle[2]); ++i)
               ins->load_store.index_shift = match.shift;
      }
