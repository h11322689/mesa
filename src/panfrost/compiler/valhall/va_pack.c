   /*
   * Copyright (C) 2021 Collabora, Ltd.
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
      #include "bi_builder.h"
   #include "va_compiler.h"
   #include "valhall.h"
   #include "valhall_enums.h"
      /* This file contains the final passes of the compiler. Running after
   * scheduling and RA, the IR is now finalized, so we need to emit it to actual
   * bits on the wire (as well as fixup branches)
   */
      /*
   * Unreachable for encoding failures, when hitting an invalid instruction.
   * Prints the (first) failing instruction to aid debugging.
   */
   NORETURN static void PRINTFLIKE(2, 3)
         {
               va_list ap;
   va_start(ap, cause);
   vfprintf(stderr, cause, ap);
            fputs(":\n\t", stderr);
   bi_print_instr(I, stderr);
               }
      /*
   * Like assert, but prints the instruction if the assertion fails to aid
   * debugging invalid inputs to the packing module.
   */
   #define pack_assert(I, cond)                                                   \
      if (!(cond))                                                                \
         /*
   * Validate that two adjacent 32-bit sources form an aligned 64-bit register
   * pair. This is a compiler invariant, required on Valhall but not on Bifrost.
   */
   static void
   va_validate_register_pair(const bi_instr *I, unsigned s)
   {
                        if (lo.type == BI_INDEX_REGISTER) {
      pack_assert(I, hi.value & 1);
      } else if (lo.type == BI_INDEX_FAU && lo.value & BIR_FAU_IMMEDIATE) {
      /* Small constants are zero extended, so the top word encode zero */
      } else {
      pack_assert(I, hi.offset & 1);
         }
      static unsigned
   va_pack_reg(const bi_instr *I, bi_index idx)
   {
      pack_assert(I, idx.type == BI_INDEX_REGISTER);
               }
      static unsigned
   va_pack_fau_special(const bi_instr *I, enum bir_fau fau)
   {
      switch (fau) {
   case BIR_FAU_ATEST_PARAM:
         case BIR_FAU_TLS_PTR:
         case BIR_FAU_WLS_PTR:
         case BIR_FAU_LANE_ID:
         case BIR_FAU_PROGRAM_COUNTER:
         case BIR_FAU_SAMPLE_POS_ARRAY:
            case BIR_FAU_BLEND_0 ...(BIR_FAU_BLEND_0 + 7):
            default:
            }
      /*
   * Encode a 64-bit FAU source. The offset is ignored, so this function can be
   * used to encode a 32-bit FAU source by or'ing in the appropriate offset.
   */
   static unsigned
   va_pack_fau_64(const bi_instr *I, bi_index idx)
   {
                        if (idx.value & BIR_FAU_IMMEDIATE)
         else if (idx.value & BIR_FAU_UNIFORM)
         else
      }
      static unsigned
   va_pack_src(const bi_instr *I, unsigned s)
   {
               if (idx.type == BI_INDEX_REGISTER) {
      unsigned value = va_pack_reg(I, idx);
   if (idx.discard)
            } else if (idx.type == BI_INDEX_FAU) {
      pack_assert(I, idx.offset <= 1);
                  }
      static unsigned
   va_pack_wrmask(const bi_instr *I)
   {
      switch (I->dest[0].swizzle) {
   case BI_SWIZZLE_H00:
         case BI_SWIZZLE_H11:
         case BI_SWIZZLE_H01:
         default:
            }
      static enum va_atomic_operation
   va_pack_atom_opc(const bi_instr *I)
   {
      switch (I->atom_opc) {
   case BI_ATOM_OPC_AADD:
         case BI_ATOM_OPC_ASMIN:
         case BI_ATOM_OPC_ASMAX:
         case BI_ATOM_OPC_AUMIN:
         case BI_ATOM_OPC_AUMAX:
         case BI_ATOM_OPC_AAND:
         case BI_ATOM_OPC_AOR:
         case BI_ATOM_OPC_AXOR:
         case BI_ATOM_OPC_ACMPXCHG:
   case BI_ATOM_OPC_AXCHG:
         default:
            }
      static enum va_atomic_operation_with_1
   va_pack_atom_opc_1(const bi_instr *I)
   {
      switch (I->atom_opc) {
   case BI_ATOM_OPC_AINC:
         case BI_ATOM_OPC_ADEC:
         case BI_ATOM_OPC_AUMAX1:
         case BI_ATOM_OPC_ASMAX1:
         case BI_ATOM_OPC_AOR1:
         default:
            }
      static unsigned
   va_pack_dest(const bi_instr *I)
   {
      assert(I->nr_dests);
      }
      static enum va_widen
   va_pack_widen_f32(const bi_instr *I, enum bi_swizzle swz)
   {
      switch (swz) {
   case BI_SWIZZLE_H01:
         case BI_SWIZZLE_H00:
         case BI_SWIZZLE_H11:
         default:
            }
      static enum va_swizzles_16_bit
   va_pack_swizzle_f16(const bi_instr *I, enum bi_swizzle swz)
   {
      switch (swz) {
   case BI_SWIZZLE_H00:
         case BI_SWIZZLE_H10:
         case BI_SWIZZLE_H01:
         case BI_SWIZZLE_H11:
         default:
            }
      static unsigned
   va_pack_widen(const bi_instr *I, enum bi_swizzle swz, enum va_size size)
   {
      if (size == VA_SIZE_8) {
      switch (swz) {
   case BI_SWIZZLE_H01:
         case BI_SWIZZLE_H00:
         case BI_SWIZZLE_H11:
         case BI_SWIZZLE_B0000:
         case BI_SWIZZLE_B1111:
         case BI_SWIZZLE_B2222:
         case BI_SWIZZLE_B3333:
         default:
            } else if (size == VA_SIZE_16) {
      switch (swz) {
   case BI_SWIZZLE_H00:
         case BI_SWIZZLE_H10:
         case BI_SWIZZLE_H01:
         case BI_SWIZZLE_H11:
         case BI_SWIZZLE_B0000:
         case BI_SWIZZLE_B1111:
         case BI_SWIZZLE_B2222:
         case BI_SWIZZLE_B3333:
         default:
            } else if (size == VA_SIZE_32) {
      switch (swz) {
   case BI_SWIZZLE_H01:
         case BI_SWIZZLE_H00:
         case BI_SWIZZLE_H11:
         case BI_SWIZZLE_B0000:
         case BI_SWIZZLE_B1111:
         case BI_SWIZZLE_B2222:
         case BI_SWIZZLE_B3333:
         default:
            } else {
            }
      static enum va_half_swizzles_8_bit
   va_pack_halfswizzle(const bi_instr *I, enum bi_swizzle swz)
   {
      switch (swz) {
   case BI_SWIZZLE_B0000:
         case BI_SWIZZLE_B1111:
         case BI_SWIZZLE_B2222:
         case BI_SWIZZLE_B3333:
         case BI_SWIZZLE_B0011:
         case BI_SWIZZLE_B2233:
         case BI_SWIZZLE_B0022:
         default:
            }
      static enum va_lanes_8_bit
   va_pack_shift_lanes(const bi_instr *I, enum bi_swizzle swz)
   {
      switch (swz) {
   case BI_SWIZZLE_H01:
         case BI_SWIZZLE_B0000:
         case BI_SWIZZLE_B1111:
         case BI_SWIZZLE_B2222:
         case BI_SWIZZLE_B3333:
         default:
            }
      static enum va_combine
   va_pack_combine(const bi_instr *I, enum bi_swizzle swz)
   {
      switch (swz) {
   case BI_SWIZZLE_H01:
         case BI_SWIZZLE_H00:
         case BI_SWIZZLE_H11:
         default:
            }
      static enum va_source_format
   va_pack_source_format(const bi_instr *I)
   {
      switch (I->source_format) {
   case BI_SOURCE_FORMAT_FLAT32:
         case BI_SOURCE_FORMAT_FLAT16:
         case BI_SOURCE_FORMAT_F32:
         case BI_SOURCE_FORMAT_F16:
                     }
      static uint64_t
   va_pack_rhadd(const bi_instr *I)
   {
      switch (I->round) {
   case BI_ROUND_RTN:
         case BI_ROUND_RTP:
         default:
            }
      static uint64_t
   va_pack_alu(const bi_instr *I)
   {
      struct va_opcode_info info = valhall_opcodes[I->op];
            switch (I->op) {
   /* Add FREXP flags */
   case BI_OPCODE_FREXPE_F32:
   case BI_OPCODE_FREXPE_V2F16:
   case BI_OPCODE_FREXPM_F32:
   case BI_OPCODE_FREXPM_V2F16:
      if (I->sqrt)
         if (I->log)
               /* Add mux type */
   case BI_OPCODE_MUX_I32:
   case BI_OPCODE_MUX_V2I16:
   case BI_OPCODE_MUX_V4I8:
      hex |= (uint64_t)I->mux << 32;
         /* Add .eq flag */
   case BI_OPCODE_BRANCHZ_I16:
   case BI_OPCODE_BRANCHZI:
               if (I->cmpf == BI_CMPF_EQ)
            if (I->op == BI_OPCODE_BRANCHZI)
         else
                  /* Add arithmetic flag */
   case BI_OPCODE_RSHIFT_AND_I32:
   case BI_OPCODE_RSHIFT_AND_V2I16:
   case BI_OPCODE_RSHIFT_AND_V4I8:
   case BI_OPCODE_RSHIFT_OR_I32:
   case BI_OPCODE_RSHIFT_OR_V2I16:
   case BI_OPCODE_RSHIFT_OR_V4I8:
   case BI_OPCODE_RSHIFT_XOR_I32:
   case BI_OPCODE_RSHIFT_XOR_V2I16:
   case BI_OPCODE_RSHIFT_XOR_V4I8:
      hex |= (uint64_t)I->arithmetic << 34;
         case BI_OPCODE_LEA_BUF_IMM:
      /* Buffer table index */
   hex |= 0xD << 8;
         case BI_OPCODE_LEA_ATTR_IMM:
      hex |= ((uint64_t)I->table) << 16;
   hex |= ((uint64_t)I->attribute_index) << 20;
         case BI_OPCODE_IADD_IMM_I32:
   case BI_OPCODE_IADD_IMM_V2I16:
   case BI_OPCODE_IADD_IMM_V4I8:
   case BI_OPCODE_FADD_IMM_F32:
   case BI_OPCODE_FADD_IMM_V2F16:
      hex |= ((uint64_t)I->index) << 8;
         case BI_OPCODE_CLPER_I32:
      hex |= ((uint64_t)I->inactive_result) << 22;
   hex |= ((uint64_t)I->lane_op) << 32;
   hex |= ((uint64_t)I->subgroup) << 36;
         case BI_OPCODE_LD_VAR:
   case BI_OPCODE_LD_VAR_FLAT:
   case BI_OPCODE_LD_VAR_IMM:
   case BI_OPCODE_LD_VAR_FLAT_IMM:
   case BI_OPCODE_LD_VAR_BUF_F16:
   case BI_OPCODE_LD_VAR_BUF_F32:
   case BI_OPCODE_LD_VAR_BUF_IMM_F16:
   case BI_OPCODE_LD_VAR_BUF_IMM_F32:
   case BI_OPCODE_LD_VAR_SPECIAL:
      if (I->op == BI_OPCODE_LD_VAR_SPECIAL)
         else if (I->op == BI_OPCODE_LD_VAR_BUF_IMM_F16 ||
               } else if (I->op == BI_OPCODE_LD_VAR_IMM ||
            hex |= ((uint64_t)I->table) << 8;
               hex |= ((uint64_t)va_pack_source_format(I)) << 24;
   hex |= ((uint64_t)I->update) << 36;
   hex |= ((uint64_t)I->sample) << 38;
         case BI_OPCODE_LD_ATTR_IMM:
      hex |= ((uint64_t)I->table) << 16;
   hex |= ((uint64_t)I->attribute_index) << 20;
         case BI_OPCODE_LD_TEX_IMM:
   case BI_OPCODE_LEA_TEX_IMM:
      hex |= ((uint64_t)I->table) << 16;
   hex |= ((uint64_t)I->texture_index) << 20;
         case BI_OPCODE_ZS_EMIT:
      if (I->stencil)
         if (I->z)
               default:
                  /* FMA_RSCALE.f32 special modes treated as extra opcodes */
   if (I->op == BI_OPCODE_FMA_RSCALE_F32) {
      pack_assert(I, I->special < 4);
               /* Add the normal destination or a placeholder.  Staging destinations are
   * added elsewhere, as they require special handling for control fields.
   */
   if (info.has_dest && info.nr_staging_dests == 0) {
         } else if (info.nr_staging_dests == 0 && info.nr_staging_srcs == 0) {
      pack_assert(I, I->nr_dests == 0);
                        /* First src is staging if we read, skip it when packing sources */
            for (unsigned i = 0; i < info.nr_srcs; ++i) {
               struct va_src_info src_info = info.srcs[i];
            bi_index src = I->src[logical_i + src_offset];
            if (src_info.notted) {
      if (src.neg)
      } else if (src_info.absneg) {
                     if (src.neg)
         if (src.abs)
      } else {
      if (src.neg)
         if (src.abs)
               if (src_info.swizzle) {
      unsigned offs = 24 + ((2 - i) * 2);
                  uint64_t v = (size == VA_SIZE_32 ? va_pack_widen_f32(I, S)
            } else if (src_info.widen) {
      unsigned offs = (i == 1) ? 26 : 36;
      } else if (src_info.lane) {
                     if (src_info.size == VA_SIZE_16) {
         } else if (I->op == BI_OPCODE_BRANCHZ_I16) {
         } else {
      pack_assert(I, src_info.size == VA_SIZE_8);
   unsigned comp = src.swizzle - BI_SWIZZLE_B0000;
   pack_assert(I, comp < 4);
         } else if (src_info.lanes) {
      pack_assert(I, src_info.size == VA_SIZE_8);
   pack_assert(I, i == 1);
      } else if (src_info.combine) {
      /* Treat as swizzle, subgroup ops not yet supported */
   pack_assert(I, src_info.size == VA_SIZE_32);
   pack_assert(I, i == 0);
      } else if (src_info.halfswizzle) {
      pack_assert(I, src_info.size == VA_SIZE_8);
   pack_assert(I, i == 0);
      } else if (src.swizzle != BI_SWIZZLE_H01) {
                     if (info.saturate)
         if (info.rhadd)
         if (info.clamp)
         if (info.round_mode)
         if (info.condition)
         if (info.result_type)
               }
      static uint64_t
   va_pack_byte_offset(const bi_instr *I)
   {
      int16_t offset = I->byte_offset;
   if (offset != I->byte_offset)
            uint16_t offset_as_u16 = offset;
      }
      static uint64_t
   va_pack_byte_offset_8(const bi_instr *I)
   {
      uint8_t offset = I->byte_offset;
   if (offset != I->byte_offset)
               }
      static uint64_t
   va_pack_load(const bi_instr *I, bool buffer_descriptor)
   {
      const uint8_t load_lane_identity[8] = {
      VA_LOAD_LANE_8_BIT_B0,        VA_LOAD_LANE_16_BIT_H0,
   VA_LOAD_LANE_24_BIT_IDENTITY, VA_LOAD_LANE_32_BIT_W0,
   VA_LOAD_LANE_48_BIT_IDENTITY, VA_LOAD_LANE_64_BIT_IDENTITY,
               unsigned memory_size = (valhall_opcodes[I->op].exact >> 27) & 0x7;
            // unsigned
            if (!buffer_descriptor)
                     if (buffer_descriptor)
               }
      static uint64_t
   va_pack_memory_access(const bi_instr *I)
   {
      switch (I->seg) {
   case BI_SEG_TL:
         case BI_SEG_POS:
         case BI_SEG_VARY:
         default:
            }
      static uint64_t
   va_pack_store(const bi_instr *I)
   {
               va_validate_register_pair(I, 1);
                        }
      static enum va_lod_mode
   va_pack_lod_mode(const bi_instr *I)
   {
      switch (I->va_lod_mode) {
   case BI_VA_LOD_MODE_ZERO_LOD:
         case BI_VA_LOD_MODE_COMPUTED_LOD:
         case BI_VA_LOD_MODE_EXPLICIT:
         case BI_VA_LOD_MODE_COMPUTED_BIAS:
         case BI_VA_LOD_MODE_GRDESC:
                     }
      static enum va_register_type
   va_pack_register_type(const bi_instr *I)
   {
      switch (I->register_format) {
   case BI_REGISTER_FORMAT_F16:
   case BI_REGISTER_FORMAT_F32:
            case BI_REGISTER_FORMAT_U16:
   case BI_REGISTER_FORMAT_U32:
            case BI_REGISTER_FORMAT_S16:
   case BI_REGISTER_FORMAT_S32:
            default:
            }
      static enum va_register_format
   va_pack_register_format(const bi_instr *I)
   {
      switch (I->register_format) {
   case BI_REGISTER_FORMAT_AUTO:
         case BI_REGISTER_FORMAT_F32:
         case BI_REGISTER_FORMAT_F16:
         case BI_REGISTER_FORMAT_S32:
         case BI_REGISTER_FORMAT_S16:
         case BI_REGISTER_FORMAT_U32:
         case BI_REGISTER_FORMAT_U16:
         default:
            }
      uint64_t
   va_pack_instr(const bi_instr *I)
   {
               uint64_t hex = info.exact | (((uint64_t)I->flow) << 59);
            if (info.slot)
            if (info.sr_count) {
      bool read = bi_opcode_props[I->op].sr_read;
            unsigned count =
            hex |= ((uint64_t)count << 33);
   hex |= (uint64_t)va_pack_reg(I, sr) << 40;
               if (info.sr_write_count) {
      hex |= ((uint64_t)bi_count_write_registers(I, 0) - 1) << 36;
               if (info.vecsize)
            if (info.register_format)
            switch (I->op) {
   case BI_OPCODE_LOAD_I8:
   case BI_OPCODE_LOAD_I16:
   case BI_OPCODE_LOAD_I24:
   case BI_OPCODE_LOAD_I32:
   case BI_OPCODE_LOAD_I48:
   case BI_OPCODE_LOAD_I64:
   case BI_OPCODE_LOAD_I96:
   case BI_OPCODE_LOAD_I128:
      hex |= va_pack_load(I, false);
         case BI_OPCODE_LD_BUFFER_I8:
   case BI_OPCODE_LD_BUFFER_I16:
   case BI_OPCODE_LD_BUFFER_I24:
   case BI_OPCODE_LD_BUFFER_I32:
   case BI_OPCODE_LD_BUFFER_I48:
   case BI_OPCODE_LD_BUFFER_I64:
   case BI_OPCODE_LD_BUFFER_I96:
   case BI_OPCODE_LD_BUFFER_I128:
      hex |= va_pack_load(I, true);
         case BI_OPCODE_STORE_I8:
   case BI_OPCODE_STORE_I16:
   case BI_OPCODE_STORE_I24:
   case BI_OPCODE_STORE_I32:
   case BI_OPCODE_STORE_I48:
   case BI_OPCODE_STORE_I64:
   case BI_OPCODE_STORE_I96:
   case BI_OPCODE_STORE_I128:
      hex |= va_pack_store(I);
         case BI_OPCODE_ATOM1_RETURN_I32:
      /* Permit omitting the destination for plain ATOM1 */
   if (!bi_count_write_registers(I, 0)) {
                  /* 64-bit source */
   va_validate_register_pair(I, 0);
   hex |= (uint64_t)va_pack_src(I, 0) << 0;
   hex |= va_pack_byte_offset_8(I);
   hex |= ((uint64_t)va_pack_atom_opc_1(I)) << 22;
         case BI_OPCODE_ATOM_I32:
   case BI_OPCODE_ATOM_RETURN_I32:
      /* 64-bit source */
   va_validate_register_pair(I, 1);
   hex |= (uint64_t)va_pack_src(I, 1) << 0;
   hex |= va_pack_byte_offset_8(I);
            if (I->op == BI_OPCODE_ATOM_RETURN_I32)
            if (I->atom_opc == BI_ATOM_OPC_ACMPXCHG)
                  case BI_OPCODE_ST_CVT:
      /* Staging read */
            /* Conversion descriptor */
   hex |= (uint64_t)va_pack_src(I, 3) << 16;
         case BI_OPCODE_BLEND: {
      /* Source 0 - Blend descriptor (64-bit) */
   hex |= ((uint64_t)va_pack_src(I, 2)) << 0;
            /* Target */
   if (I->branch_offset & 0x7)
                  /* Source 2 - coverage mask */
            /* Vector size */
   unsigned vecsize = 4;
                        case BI_OPCODE_TEX_SINGLE:
   case BI_OPCODE_TEX_FETCH:
   case BI_OPCODE_TEX_GATHER: {
      /* Image to read from */
            if (I->op == BI_OPCODE_TEX_FETCH && I->shadow)
            if (I->array_enable)
         if (I->texel_offset)
         if (I->shadow)
         if (I->skip)
         if (!bi_is_regfmt_16(I->register_format))
            if (I->op == BI_OPCODE_TEX_SINGLE)
            if (I->op == BI_OPCODE_TEX_GATHER) {
      if (I->integer_coordinates)
                     hex |= (I->write_mask << 22);
   hex |= ((uint64_t)va_pack_register_type(I)) << 26;
                        default:
      if (!info.exact && I->op != BI_OPCODE_NOP)
            hex |= va_pack_alu(I);
                  }
      static unsigned
   va_instructions_in_block(bi_block *block)
   {
               bi_foreach_instr_in_block(block, _) {
                     }
      /* Calculate branch_offset from a branch_target for a direct relative branch */
      static void
   va_lower_branch_target(bi_context *ctx, bi_block *start, bi_instr *I)
   {
      /* Precondition: unlowered relative branch */
   bi_block *target = I->branch_target;
            /* Signed since we might jump backwards */
            /* Determine if the target block is strictly greater in source order */
            if (forwards) {
      /* We have to jump through this block */
   bi_foreach_instr_in_block_from(start, _, I) {
                  /* We then need to jump over every following block until the target */
   bi_foreach_block_from(ctx, start, blk) {
      /* End just before the target */
                  /* Count other blocks */
   if (blk != start)
         } else {
      /* Jump through the beginning of this block */
   bi_foreach_instr_in_block_from_rev(start, ins, I) {
      if (ins != I)
               /* Jump over preceding blocks up to and including the target to get to
   * the beginning of the target */
   bi_foreach_block_from_rev(ctx, start, blk) {
                              /* End just after the target */
   if (blk == target)
                  /* Offset is relative to the next instruction, so bias */
            /* Update the instruction */
      }
      /*
   * Late lowering to insert blend shader calls after BLEND instructions. Required
   * to support blend shaders, so this pass may be omitted if it is known that
   * blend shaders are never used.
   *
   * This lowering runs late because it introduces control flow changes without
   * modifying the control flow graph. It hardcodes registers, meaning running
   * after RA makes sense. Finally, it hardcodes a manually sized instruction
   * sequence, requiring it to run after scheduling.
   *
   * As it is Valhall specific, running it as a pre-pack lowering is sensible.
   */
   static void
   va_lower_blend(bi_context *ctx)
   {
      /* Program counter for *next* instruction */
            bi_foreach_instr_global_safe(ctx, I) {
      if (I->op != BI_OPCODE_BLEND)
                              /* By ABI, r48 is the link register shared with blend shaders */
            if (I->flow == VA_FLOW_END)
         else
                     /* For fixed function: skip the prologue, or return */
   if (I->flow != VA_FLOW_END)
         }
      void
   bi_pack_valhall(bi_context *ctx, struct util_dynarray *emission)
   {
                        /* Late lowering */
   if (ctx->stage == MESA_SHADER_FRAGMENT && !ctx->inputs->is_blend)
            bi_foreach_block(ctx, block) {
      bi_foreach_instr_in_block(block, I) {
                     uint64_t hex = va_pack_instr(I);
                  /* Pad with zeroes, but keep empty programs empty so they may be omitted
   * altogether. Failing to do this would result in a program containing only
   * zeroes, which is invalid and will raise an encoding fault.
   *
   * Pad an extra 16 byte (one instruction) to separate primary and secondary
   * shader disassembles. This is not strictly necessary, but it's a good
   * practice. 128 bytes is the optimal program alignment on Trym, so pad
   * secondary shaders up to 128 bytes. This may help the instruction cache.
   */
   if (orig_size != emission->size) {
      unsigned aligned = ALIGN_POT(emission->size + 16, 128);
                  }
