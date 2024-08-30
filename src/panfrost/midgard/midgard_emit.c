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
      #include "compiler.h"
   #include "midgard_ops.h"
   #include "midgard_quirks.h"
      static midgard_int_mod
   mir_get_imod(bool shift, nir_alu_type T, bool half, bool scalar)
   {
      if (!half) {
      assert(!shift);
   /* Doesn't matter, src mods are only used when expanding */
               if (shift)
            if (nir_alu_type_get_base_type(T) == nir_type_int)
         else
      }
      void
   midgard_pack_ubo_index_imm(midgard_load_store_word *word, unsigned index)
   {
      word->arg_comp = index & 0x3;
   word->arg_reg = (index >> 2) & 0x7;
   word->bitsize_toggle = (index >> 5) & 0x1;
      }
      void
   midgard_pack_varying_params(midgard_load_store_word *word,
         {
      /* Currently these parameters are not supported. */
            unsigned u;
               }
      midgard_varying_params
   midgard_unpack_varying_params(midgard_load_store_word word)
   {
               midgard_varying_params p;
               }
      unsigned
   mir_pack_mod(midgard_instruction *ins, unsigned i, bool scalar)
   {
      bool integer = midgard_is_integer_op(ins->op);
   unsigned base_size = max_bitsize_for_alu(ins);
   unsigned sz = nir_alu_type_get_type_size(ins->src_types[i]);
            return integer
            }
      /* Midgard IR only knows vector ALU types, but we sometimes need to actually
   * use scalar ALU instructions, for functional or performance reasons. To do
   * this, we just demote vector ALU payloads to scalar. */
      static int
   component_from_mask(unsigned mask)
   {
      for (int c = 0; c < 8; ++c) {
      if (mask & (1 << c))
               assert(0);
      }
      static unsigned
   mir_pack_scalar_source(unsigned mod, bool is_full, unsigned component)
   {
      midgard_scalar_alu_src s = {
      .mod = mod,
   .full = is_full,
               unsigned o;
               }
      static midgard_scalar_alu
   vector_to_scalar_alu(midgard_vector_alu v, midgard_instruction *ins)
   {
               bool half_0 = nir_alu_type_get_type_size(ins->src_types[0]) == 16;
   bool half_1 = nir_alu_type_get_type_size(ins->src_types[1]) == 16;
            unsigned packed_src[2] = {
      mir_pack_scalar_source(mir_pack_mod(ins, 0, true), !half_0,
         mir_pack_scalar_source(mir_pack_mod(ins, 1, true), !half_1,
         /* The output component is from the mask */
   midgard_scalar_alu s = {
      .op = v.op,
   .src1 = packed_src[0],
   .src2 = packed_src[1],
   .outmod = v.outmod,
   .output_full = is_full,
               /* Full components are physically spaced out */
   if (is_full) {
      assert(s.output_component < 4);
               /* Inline constant is passed along rather than trying to extract it
            if (ins->has_inline_constant) {
      uint16_t imm = 0;
   int lower_11 = ins->inline_constant & ((1 << 12) - 1);
   imm |= (lower_11 >> 9) & 3;
   imm |= (lower_11 >> 6) & 4;
   imm |= (lower_11 >> 2) & 0x38;
                           }
      /* 64-bit swizzles are super easy since there are 2 components of 2 components
   * in an 8-bit field ... lots of duplication to go around!
   *
   * Swizzles of 32-bit vectors accessed from 64-bit instructions are a little
   * funny -- pack them *as if* they were native 64-bit, using rep_* flags to
   * flag upper. For instance, xy would become 64-bit XY but that's just xyzw
   * native. Likewise, zz would become 64-bit XX with rep* so it would be xyxy
   * with rep. Pretty nifty, huh? */
      static unsigned
   mir_pack_swizzle_64(unsigned *swizzle, unsigned max_component, bool expand_high)
   {
      unsigned packed = 0;
            for (unsigned i = base; i < base + 2; ++i) {
               unsigned a = (swizzle[i] & 1) ? (COMPONENT_W << 2) | COMPONENT_Z
            if (i & 1)
         else
                  }
      static void
   mir_pack_mask_alu(midgard_instruction *ins, midgard_vector_alu *alu)
   {
               /* If we have a destination override, we need to figure out whether to
   * override to the lower or upper half, shifting the effective mask in
            unsigned inst_size = max_bitsize_for_alu(ins);
            if (upper_shift >= 0) {
      effective >>= upper_shift;
   alu->shrink_mode =
      } else {
                  if (inst_size == 32)
         else if (inst_size == 64)
         else
      }
      static unsigned
   mir_pack_swizzle(unsigned mask, unsigned *swizzle, unsigned sz,
               {
                                 if (reg_mode == midgard_reg_mode_64) {
      assert(sz == 64 || sz == 32);
                     if (sz == 32) {
                                                /* We can't mix halves */
   assert(dontcare || (hi == hi_i));
   hi = hi_i;
                  } else if (sz < 32) {
            } else {
      /* For 32-bit, swizzle packing is stupid-simple. For 16-bit,
   * the strategy is to check whether the nibble we're on is
   * upper or lower. We need all components to be on the same
   * "side"; that much is enforced by the ISA and should have
            unsigned first = mask ? ffs(mask) - 1 : 0;
            if (upper && mask)
                     for (unsigned c = (dest_up ? 4 : 0); c < (dest_up ? 8 : 4); ++c) {
                                 if (mask & (1 << c)) {
      assert(t_upper == upper);
                                          /* Replicate for now.. should really pick a side for
            if (reg_mode == midgard_reg_mode_16 && sz == 16) {
         } else if (reg_mode == midgard_reg_mode_16 && sz == 8) {
      if (base_size == 16) {
      *expand_mode =
      } else if (upper) {
            } else if (reg_mode == midgard_reg_mode_32 && sz == 16) {
      *expand_mode =
      } else if (reg_mode == midgard_reg_mode_8) {
                        }
      static void
   mir_pack_vector_srcs(midgard_instruction *ins, midgard_vector_alu *alu)
   {
                        for (unsigned i = 0; i < 2; ++i) {
      if (ins->has_inline_constant && (i == 1))
            if (ins->src[i] == ~0)
            unsigned sz = nir_alu_type_get_type_size(ins->src_types[i]);
            midgard_src_expand_mode expand_mode = midgard_src_passthrough;
   unsigned swizzle = mir_pack_swizzle(ins->mask, ins->swizzle[i], sz,
            midgard_vector_alu_src pack = {
      .mod = mir_pack_mod(ins, i, false),
   .expand_mode = expand_mode,
                        if (i == 0)
         else
         }
      static void
   mir_pack_swizzle_ldst(midgard_instruction *ins)
   {
      unsigned compsz = OP_IS_STORE(ins->op)
               unsigned maxcomps = 128 / compsz;
            for (unsigned c = 0; c < maxcomps; c += step) {
               /* Make sure the component index doesn't exceed the maximum
   * number of components. */
            if (compsz <= 32)
         else
      ins->load_store.swizzle |=
               }
      static void
   mir_pack_swizzle_tex(midgard_instruction *ins)
   {
      for (unsigned i = 0; i < 2; ++i) {
               for (unsigned c = 0; c < 4; ++c) {
                                          if (i == 0)
         else
                  }
      /*
   * Up to 15 { ALU, LDST } bundles can execute in parallel with a texture op.
   * Given a texture op, lookahead to see how many such bundles we can flag for
   * OoO execution
   */
   static bool
   mir_can_run_ooo(midgard_block *block, midgard_bundle *bundle,
         {
      /* Don't read out of bounds */
   if (bundle >=
      (midgard_bundle *)((char *)block->bundles.data + block->bundles.size))
         /* Texture ops can't execute with other texture ops */
   if (!IS_ALU(bundle->tag) && bundle->tag != TAG_LOAD_STORE_4)
            for (unsigned i = 0; i < bundle->instruction_count; ++i) {
               /* No branches, jumps, or discards */
   if (ins->compact_branch)
            /* No read-after-write data dependencies */
   mir_foreach_src(ins, s) {
      if (ins->src[s] == dependency)
                  /* Otherwise, we're okay */
      }
      static void
   mir_pack_tex_ooo(midgard_block *block, midgard_bundle *bundle,
         {
               for (count = 0; count < 15; ++count) {
      if (!mir_can_run_ooo(block, bundle + count + 1, ins->dest))
                  }
      /* Load store masks are 4-bits. Load/store ops pack for that.
   * For most operations, vec4 is the natural mask width; vec8 is constrained to
   * be in pairs, vec2 is duplicated. TODO: 8-bit?
   * For common stores (i.e. ST.*), each bit masks a single byte in the 32-bit
   * case, 2 bytes in the 64-bit case and 4 bytes in the 128-bit case.
   */
      static unsigned
   midgard_pack_common_store_mask(midgard_instruction *ins)
   {
      ASSERTED unsigned comp_sz = nir_alu_type_get_type_size(ins->src_types[0]);
   unsigned bytemask = mir_bytemask(ins);
            switch (ins->op) {
   case midgard_op_st_u8:
         case midgard_op_st_u16:
         case midgard_op_st_32:
         case midgard_op_st_64:
      assert(comp_sz >= 16);
   for (unsigned i = 0; i < 4; i++) {
      if (bytemask & (3 << (i * 2)))
      }
      case midgard_op_st_128:
      assert(comp_sz >= 32);
   for (unsigned i = 0; i < 4; i++) {
      if (bytemask & (0xf << (i * 4)))
      }
      default:
            }
      static void
   mir_pack_ldst_mask(midgard_instruction *ins)
   {
      unsigned sz = nir_alu_type_get_type_size(ins->dest_type);
            if (OP_IS_COMMON_STORE(ins->op)) {
         } else {
      if (sz == 64) {
      packed = ((ins->mask & 0x2) ? (0x8 | 0x4) : 0) |
      } else if (sz < 32) {
                        for (unsigned i = 0; i < 4; ++i) {
                     /* Make sure we're duplicated */
   assert(submask == 0 || submask == BITFIELD_MASK(comps_per_32b));
         } else {
                        }
      static void
   mir_lower_inverts(midgard_instruction *ins)
   {
               switch (ins->op) {
   case midgard_alu_op_iand:
      /* a & ~b = iandnot(a, b) */
            if (inv[0] && inv[1])
         else if (inv[1])
               case midgard_alu_op_ior:
      /*  a | ~b = iornot(a, b) */
            if (inv[0] && inv[1])
         else if (inv[1])
                  case midgard_alu_op_ixor:
      /* ~a ^ b = a ^ ~b = ~(a ^ b) = inxor(a, b) */
            if (inv[0] ^ inv[1])
                  default:
            }
      /* Opcodes with ROUNDS are the base (rte/0) type so we can just add */
      static void
   mir_lower_roundmode(midgard_instruction *ins)
   {
      if (alu_opcode_props[ins->op].props & MIDGARD_ROUNDS) {
      assert(ins->roundmode <= 0x3);
         }
      static midgard_load_store_word
   load_store_from_instr(midgard_instruction *ins)
   {
      midgard_load_store_word ldst = ins->load_store;
            if (OP_IS_STORE(ldst.op)) {
         } else {
                  /* Atomic opcode swizzles have a special meaning:
   *   - The first two bits say which component of the implicit register should
   * be used
   *   - The next two bits say if the implicit register is r26 or r27 */
   if (OP_IS_ATOMIC(ins->op)) {
      ldst.swizzle = 0;
   ldst.swizzle |= ins->swizzle[3][0] & 3;
               if (ins->src[1] != ~0) {
      ldst.arg_reg = SSA_REG_FROM_FIXED(ins->src[1]) - REGISTER_LDST_BASE;
   unsigned sz = nir_alu_type_get_type_size(ins->src_types[1]);
               if (ins->src[2] != ~0) {
      ldst.index_reg = SSA_REG_FROM_FIXED(ins->src[2]) - REGISTER_LDST_BASE;
   unsigned sz = nir_alu_type_get_type_size(ins->src_types[2]);
   ldst.index_comp =
                  }
      static midgard_texture_word
   texture_word_from_instr(midgard_instruction *ins)
   {
      midgard_texture_word tex = ins->texture;
            unsigned src1 =
                  unsigned dest =
                  if (ins->src[2] != ~0) {
      midgard_tex_register_select sel = {
      .select = SSA_REG_FROM_FIXED(ins->src[2]) & 1,
   .full = 1,
      };
   uint8_t packed;
   memcpy(&packed, &sel, sizeof(packed));
               if (ins->src[3] != ~0) {
      unsigned x = ins->swizzle[3][0];
   unsigned y = x + 1;
            /* Check range, TODO: half-registers */
            unsigned offset_reg = SSA_REG_FROM_FIXED(ins->src[3]);
   tex.offset = (1) |                   /* full */
               (offset_reg & 1) << 1 | /* select */
   (0 << 2) |              /* upper */
                  }
      static midgard_vector_alu
   vector_alu_from_instr(midgard_instruction *ins)
   {
      midgard_vector_alu alu = {
      .op = ins->op,
   .outmod = ins->outmod,
               if (ins->has_inline_constant) {
      /* Encode inline 16-bit constant. See disassembler for
            int lower_11 = ins->inline_constant & ((1 << 12) - 1);
                           }
      static midgard_branch_extended
   midgard_create_branch_extended(midgard_condition cond,
               {
      /* The condition code is actually a LUT describing a function to
   * combine multiple condition codes. However, we only support a single
   * condition code at the moment, so we just duplicate over a bunch of
            uint16_t duplicated_cond = (cond << 14) | (cond << 12) | (cond << 10) |
                  midgard_branch_extended branch = {
      .op = op,
   .dest_tag = dest_tag,
   .offset = quadword_offset,
                  }
      static void
   emit_branch(midgard_instruction *ins, compiler_context *ctx,
               {
      /* Parse some basic branch info */
   bool is_compact = ins->unit == ALU_ENAB_BR_COMPACT;
   bool is_conditional = ins->branch.conditional;
   bool is_inverted = ins->branch.invert_conditional;
   bool is_discard = ins->branch.target_type == TARGET_DISCARD;
   bool is_tilebuf_wait = ins->branch.target_type == TARGET_TILEBUF_WAIT;
   bool is_special = is_discard || is_tilebuf_wait;
            /* Determine the block we're jumping to */
            /* Report the destination tag */
   int dest_tag = is_discard ? 0
                        /* Count up the number of quadwords we're
   * jumping over = number of quadwords until
                     if (is_discard) {
      /* Fixed encoding, not actually an offset */
      } else if (is_tilebuf_wait) {
         } else if (target_number > block->base.name) {
               for (int idx = block->base.name + 1; idx < target_number; ++idx) {
                           } else {
               for (int idx = block->base.name; idx >= target_number; --idx) {
                                    /* Unconditional extended branches (far jumps)
   * have issues, so we always use a conditional
   * branch, setting the condition to always for
   * unconditional. For compact unconditional
   * branches, cond isn't used so it doesn't
            midgard_condition cond = !is_conditional ? midgard_condition_always
                  midgard_jmp_writeout_op op =
      is_discard        ? midgard_jmp_writeout_op_discard
   : is_tilebuf_wait ? midgard_jmp_writeout_op_tilebuffer_pending
   : is_writeout     ? midgard_jmp_writeout_op_writeout
   : (is_compact && !is_conditional) ? midgard_jmp_writeout_op_branch_uncond
         if (is_compact) {
               if (is_conditional || is_special) {
      midgard_branch_cond branch = {
      .op = op,
   .dest_tag = dest_tag,
   .offset = quadword_offset,
      };
      } else {
      assert(op == midgard_jmp_writeout_op_branch_uncond);
   midgard_branch_uncond branch = {
      .op = op,
   .dest_tag = dest_tag,
   .offset = quadword_offset,
      };
   assert(branch.offset == quadword_offset);
         } else { /* `ins->compact_branch`,  misnomer */
               midgard_branch_extended branch =
                  }
      static void
   emit_alu_bundle(compiler_context *ctx, midgard_block *block,
               {
      /* Emit the control word */
            /* Next up, emit register words */
   for (unsigned i = 0; i < bundle->instruction_count; ++i) {
               /* Check if this instruction has registers */
   if (ins->compact_branch)
            unsigned src2_reg = REGISTER_UNUSED;
   if (ins->has_inline_constant)
         else if (ins->src[1] != ~0)
            /* Otherwise, just emit the registers */
   uint16_t reg_word = 0;
   midgard_reg_info registers = {
      .src1_reg = (ins->src[0] == ~0 ? REGISTER_UNUSED
         .src2_reg = src2_reg,
   .src2_imm = ins->has_inline_constant,
   .out_reg =
      };
   memcpy(&reg_word, &registers, sizeof(uint16_t));
               /* Now, we emit the body itself */
   for (unsigned i = 0; i < bundle->instruction_count; ++i) {
               if (!ins->compact_branch) {
      mir_lower_inverts(ins);
               if (midgard_is_branch_unit(ins->unit)) {
         } else if (ins->unit & UNITS_ANY_VECTOR) {
      midgard_vector_alu source = vector_alu_from_instr(ins);
   mir_pack_mask_alu(ins, &source);
   mir_pack_vector_srcs(ins, &source);
   unsigned size = sizeof(source);
      } else {
      midgard_scalar_alu source =
         unsigned size = sizeof(source);
                  /* Emit padding (all zero) */
   if (bundle->padding) {
      memset(util_dynarray_grow_bytes(emission, bundle->padding, 1), 0,
                        if (bundle->has_embedded_constants)
      }
      /* Shift applied to the immediate used as an offset. Probably this is papering
   * over some other semantic distinction else well, but it unifies things in the
   * compiler so I don't mind. */
      static void
   mir_ldst_pack_offset(midgard_instruction *ins, int offset)
   {
      /* These opcodes don't support offsets */
   assert(!OP_IS_REG2REG_LDST(ins->op) || ins->op == midgard_op_lea ||
            if (OP_IS_UBO_READ(ins->op))
         else if (OP_IS_IMAGE(ins->op))
         else if (OP_IS_SPECIAL(ins->op))
         else
      }
      static enum mali_sampler_type
   midgard_sampler_type(nir_alu_type t)
   {
      switch (nir_alu_type_get_base_type(t)) {
   case nir_type_float:
         case nir_type_int:
         case nir_type_uint:
         default:
            }
      /* After everything is scheduled, emit whole bundles at a time */
      void
   emit_binary_bundle(compiler_context *ctx, midgard_block *block,
               {
               switch (bundle->tag) {
   case TAG_ALU_4:
   case TAG_ALU_8:
   case TAG_ALU_12:
   case TAG_ALU_16:
   case TAG_ALU_4 + 4:
   case TAG_ALU_8 + 4:
   case TAG_ALU_12 + 4:
   case TAG_ALU_16 + 4:
      emit_alu_bundle(ctx, block, bundle, emission, lookahead);
         case TAG_LOAD_STORE_4: {
                                 for (unsigned i = 0; i < bundle->instruction_count; ++i) {
                     /* Atomic ops don't use this swizzle the same way as other ops */
                  /* Apply a constant offset */
   unsigned offset = ins->constants.u32[0];
   if (offset)
               midgard_load_store_word ldst0 =
                  if (bundle->instruction_count == 2) {
      midgard_load_store_word ldst1 =
                     midgard_load_store instruction = {
      .type = bundle->tag,
   .next_type = next_tag,
   .word1 = current64,
                                    case TAG_TEXTURE_4:
   case TAG_TEXTURE_4_VTX:
   case TAG_TEXTURE_4_BARRIER: {
      /* Texture instructions are easy, since there is no pipelining
   * nor VLIW to worry about. We may need to set .cont/.last
                     ins->texture.type = bundle->tag;
   ins->texture.next_type = next_tag;
            /* Nothing else to pack for barriers */
   if (ins->op == midgard_tex_op_barrier) {
      ins->texture.op = ins->op;
   util_dynarray_append(emission, midgard_texture_word, ins->texture);
                                          if (!(ctx->quirks & MIDGARD_NO_OOO))
            unsigned osz = nir_alu_type_get_type_size(ins->dest_type);
            assert(osz == 32 || osz == 16);
            ins->texture.out_full = (osz == 32);
   ins->texture.out_upper = override > 0;
   ins->texture.in_reg_full = (isz == 32);
   ins->texture.sampler_type = midgard_sampler_type(ins->dest_type);
            if (mir_op_computes_derivatives(ctx->stage, ins->op)) {
      if (ins->helper_terminate)
         else if (!ins->helper_execute)
               midgard_texture_word texture = texture_word_from_instr(ins);
   util_dynarray_append(emission, midgard_texture_word, texture);
               default:
            }
