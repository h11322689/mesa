   /*
   * Copyright © 2018 Valve Corporation
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
      #include "aco_builder.h"
   #include "aco_ir.h"
      #include "common/sid.h"
      #include "util/memstream.h"
      #include "ac_shader_util.h"
   #include <algorithm>
   #include <map>
   #include <vector>
      namespace aco {
      struct constaddr_info {
      unsigned getpc_end;
      };
      struct asm_context {
      Program* program;
   enum amd_gfx_level gfx_level;
   std::vector<std::pair<int, SOPP_instruction*>> branches;
   std::map<unsigned, constaddr_info> constaddrs;
   std::map<unsigned, constaddr_info> resumeaddrs;
   std::vector<struct aco_symbol>* symbols;
   Block* loop_header = NULL;
   const int16_t* opcode;
   // TODO: keep track of branch instructions referring blocks
   // and, when emitting the block, correct the offset in instr
   asm_context(Program* program_, std::vector<struct aco_symbol>* symbols_)
         {
      if (gfx_level <= GFX7)
         else if (gfx_level <= GFX9)
         else if (gfx_level <= GFX10_3)
         else if (gfx_level >= GFX11)
                  };
      unsigned
   get_mimg_nsa_dwords(const Instruction* instr)
   {
      unsigned addr_dwords = instr->operands.size() - 3;
   for (unsigned i = 1; i < addr_dwords; i++) {
      if (instr->operands[3 + i].physReg() !=
      instr->operands[3 + (i - 1)].physReg().advance(instr->operands[3 + (i - 1)].bytes()))
   }
      }
      uint32_t
   reg(asm_context& ctx, PhysReg reg)
   {
      if (ctx.gfx_level >= GFX11) {
      if (reg == m0)
         else if (reg == sgpr_null)
      }
      }
      ALWAYS_INLINE uint32_t
   reg(asm_context& ctx, Operand op, unsigned width = 32)
   {
         }
      ALWAYS_INLINE uint32_t
   reg(asm_context& ctx, Definition def, unsigned width = 32)
   {
         }
      bool
   needs_vop3_gfx11(asm_context& ctx, Instruction* instr)
   {
      if (ctx.gfx_level <= GFX10_3)
            uint8_t mask = get_gfx11_true16_mask(instr->opcode);
   if (!mask)
            u_foreach_bit (i, mask & 0x3) {
      if (instr->operands[i].physReg().reg() >= (256 + 128))
      }
   if ((mask & 0x8) && instr->definitions[0].physReg().reg() >= (256 + 128))
            }
      void
   emit_instruction(asm_context& ctx, std::vector<uint32_t>& out, Instruction* instr)
   {
      /* lower remaining pseudo-instructions */
   if (instr->opcode == aco_opcode::p_constaddr_getpc) {
               instr->opcode = aco_opcode::s_getpc_b64;
      } else if (instr->opcode == aco_opcode::p_constaddr_addlo) {
               instr->opcode = aco_opcode::s_add_u32;
   instr->operands.pop_back();
   assert(instr->operands[1].isConstant());
   /* in case it's an inline constant, make it a literal */
      } else if (instr->opcode == aco_opcode::p_resumeaddr_getpc) {
               instr->opcode = aco_opcode::s_getpc_b64;
      } else if (instr->opcode == aco_opcode::p_resumeaddr_addlo) {
               instr->opcode = aco_opcode::s_add_u32;
   instr->operands.pop_back();
   assert(instr->operands[1].isConstant());
   /* in case it's an inline constant, make it a literal */
      } else if (instr->opcode == aco_opcode::p_load_symbol) {
      assert(instr->operands[0].isConstant());
            struct aco_symbol info;
   info.id = (enum aco_symbol_id)instr->operands[0].constantValue();
   info.offset = out.size() + 1;
            instr->opcode = aco_opcode::s_mov_b32;
   /* in case it's an inline constant, make it a literal */
               /* Promote VOP12C to VOP3 if necessary. */
   if ((instr->isVOP1() || instr->isVOP2() || instr->isVOPC()) && !instr->isVOP3() &&
      needs_vop3_gfx11(ctx, instr)) {
   instr->format = asVOP3(instr->format);
   if (instr->opcode == aco_opcode::v_fmaak_f16) {
      instr->opcode = aco_opcode::v_fma_f16;
      } else if (instr->opcode == aco_opcode::v_fmamk_f16) {
      instr->valu().swapOperands(1, 2);
   instr->opcode = aco_opcode::v_fma_f16;
                  uint32_t opcode = ctx.opcode[(int)instr->opcode];
   if (opcode == (uint32_t)-1) {
      char* outmem;
   size_t outsize;
   struct u_memstream mem;
   u_memstream_open(&mem, &outmem, &outsize);
            fprintf(memf, "Unsupported opcode: ");
   aco_print_instr(ctx.gfx_level, instr, memf);
            aco_err(ctx.program, outmem);
                        switch (instr->format) {
   case Format::SOP2: {
      uint32_t encoding = (0b10 << 30);
   encoding |= opcode << 23;
   encoding |= !instr->definitions.empty() ? reg(ctx, instr->definitions[0]) << 16 : 0;
   encoding |= instr->operands.size() >= 2 ? reg(ctx, instr->operands[1]) << 8 : 0;
   encoding |= !instr->operands.empty() ? reg(ctx, instr->operands[0]) : 0;
   out.push_back(encoding);
      }
   case Format::SOPK: {
               if (instr->opcode == aco_opcode::s_subvector_loop_begin) {
      assert(ctx.gfx_level >= GFX10);
   assert(ctx.subvector_begin_pos == -1);
      } else if (instr->opcode == aco_opcode::s_subvector_loop_end) {
      assert(ctx.gfx_level >= GFX10);
   assert(ctx.subvector_begin_pos != -1);
   /* Adjust s_subvector_loop_begin instruction to the address after the end  */
   out[ctx.subvector_begin_pos] |= (out.size() - ctx.subvector_begin_pos);
   /* Adjust s_subvector_loop_end instruction to the address after the beginning  */
   sopk.imm = (uint16_t)(ctx.subvector_begin_pos - (int)out.size());
               uint32_t encoding = (0b1011 << 28);
   encoding |= opcode << 23;
   encoding |= !instr->definitions.empty() && !(instr->definitions[0].physReg() == scc)
                     : !instr->operands.empty() && instr->operands[0].physReg() <= 127
   encoding |= sopk.imm;
   out.push_back(encoding);
      }
   case Format::SOP1: {
      uint32_t encoding = (0b101111101 << 23);
   encoding |= !instr->definitions.empty() ? reg(ctx, instr->definitions[0]) << 16 : 0;
   encoding |= opcode << 8;
   encoding |= !instr->operands.empty() ? reg(ctx, instr->operands[0]) : 0;
   out.push_back(encoding);
      }
   case Format::SOPC: {
      uint32_t encoding = (0b101111110 << 23);
   encoding |= opcode << 16;
   encoding |= instr->operands.size() == 2 ? reg(ctx, instr->operands[1]) << 8 : 0;
   encoding |= !instr->operands.empty() ? reg(ctx, instr->operands[0]) : 0;
   out.push_back(encoding);
      }
   case Format::SOPP: {
      SOPP_instruction& sopp = instr->sopp();
   uint32_t encoding = (0b101111111 << 23);
   encoding |= opcode << 16;
   encoding |= (uint16_t)sopp.imm;
   if (sopp.block != -1) {
      sopp.pass_flags = 0;
      }
   out.push_back(encoding);
      }
   case Format::SMEM: {
      SMEM_instruction& smem = instr->smem();
   bool soe = instr->operands.size() >= (!instr->definitions.empty() ? 3 : 4);
   bool is_load = !instr->definitions.empty();
            if (ctx.gfx_level <= GFX7) {
      encoding = (0b11000 << 27);
   encoding |= opcode << 22;
   encoding |= instr->definitions.size() ? reg(ctx, instr->definitions[0]) << 15 : 0;
   encoding |= instr->operands.size() ? (reg(ctx, instr->operands[0]) >> 1) << 9 : 0;
   if (instr->operands.size() >= 2) {
      if (!instr->operands[1].isConstant()) {
         } else if (instr->operands[1].constantValue() >= 1024) {
         } else {
      encoding |= instr->operands[1].constantValue() >> 2;
         }
   out.push_back(encoding);
   /* SMRD instructions can take a literal on GFX7 */
   if (instr->operands.size() >= 2 && instr->operands[1].isConstant() &&
      instr->operands[1].constantValue() >= 1024)
                  if (ctx.gfx_level <= GFX9) {
      encoding = (0b110000 << 26);
   assert(!smem.dlc); /* Device-level coherent is not supported on GFX9 and lower */
      } else {
      encoding = (0b111101 << 26);
   assert(!smem.nv); /* Non-volatile is not supported on GFX10 */
               encoding |= opcode << 18;
            if (ctx.gfx_level <= GFX9) {
      if (instr->operands.size() >= 2)
      }
   if (ctx.gfx_level == GFX9) {
                  if (is_load || instr->operands.size() >= 3) { /* SDATA */
      encoding |= (is_load ? reg(ctx, instr->definitions[0]) : reg(ctx, instr->operands[2]))
      }
   if (instr->operands.size() >= 1) { /* SBASE */
                  out.push_back(encoding);
            int32_t offset = 0;
   uint32_t soffset =
      ctx.gfx_level >= GFX10
      ? reg(ctx, sgpr_null) /* On GFX10 this is disabled by specifying SGPR_NULL */
   : 0;                  /* On GFX9, it is disabled by the SOE bit (and it's not present on
   if (instr->operands.size() >= 2) {
      const Operand& op_off1 = instr->operands[1];
   if (ctx.gfx_level <= GFX9) {
         } else {
      /* GFX10 only supports constants in OFFSET, so put the operand in SOFFSET if it's an
   * SGPR */
   if (op_off1.isConstant()) {
         } else {
      soffset = reg(ctx, op_off1);
                  if (soe) {
      const Operand& op_off2 = instr->operands.back();
   assert(ctx.gfx_level >= GFX9); /* GFX8 and below don't support specifying a constant
         assert(!op_off2.isConstant());
         }
   encoding |= offset;
            out.push_back(encoding);
      }
   case Format::VOP2: {
      VALU_instruction& valu = instr->valu();
   uint32_t encoding = 0;
   encoding |= opcode << 25;
   encoding |= reg(ctx, instr->definitions[0], 8) << 17;
   encoding |= (valu.opsel[3] ? 128 : 0) << 17;
   encoding |= reg(ctx, instr->operands[1], 8) << 9;
   encoding |= (valu.opsel[1] ? 128 : 0) << 9;
   encoding |= reg(ctx, instr->operands[0]);
   encoding |= valu.opsel[0] ? 128 : 0;
   out.push_back(encoding);
      }
   case Format::VOP1: {
      VALU_instruction& valu = instr->valu();
   uint32_t encoding = (0b0111111 << 25);
   if (!instr->definitions.empty()) {
      encoding |= reg(ctx, instr->definitions[0], 8) << 17;
      }
   encoding |= opcode << 9;
   if (!instr->operands.empty()) {
      encoding |= reg(ctx, instr->operands[0]);
      }
   out.push_back(encoding);
      }
   case Format::VOPC: {
      VALU_instruction& valu = instr->valu();
   uint32_t encoding = (0b0111110 << 25);
   encoding |= opcode << 17;
   encoding |= reg(ctx, instr->operands[1], 8) << 9;
   encoding |= (valu.opsel[1] ? 128 : 0) << 9;
   encoding |= reg(ctx, instr->operands[0]);
   encoding |= valu.opsel[0] ? 128 : 0;
   out.push_back(encoding);
      }
   case Format::VINTRP: {
      VINTRP_instruction& interp = instr->vintrp();
            if (instr->opcode == aco_opcode::v_interp_p1ll_f16 ||
      instr->opcode == aco_opcode::v_interp_p1lv_f16 ||
   instr->opcode == aco_opcode::v_interp_p2_legacy_f16 ||
   instr->opcode == aco_opcode::v_interp_p2_f16) {
   if (ctx.gfx_level == GFX8 || ctx.gfx_level == GFX9) {
         } else if (ctx.gfx_level >= GFX10) {
         } else {
                  encoding |= opcode << 16;
                  encoding = 0;
   encoding |= interp.attribute;
   encoding |= interp.component << 6;
   encoding |= reg(ctx, instr->operands[0]) << 9;
   if (instr->opcode == aco_opcode::v_interp_p2_f16 ||
      instr->opcode == aco_opcode::v_interp_p2_legacy_f16 ||
   instr->opcode == aco_opcode::v_interp_p1lv_f16) {
      }
      } else {
      if (ctx.gfx_level == GFX8 || ctx.gfx_level == GFX9) {
         } else {
                  assert(encoding);
   encoding |= reg(ctx, instr->definitions[0], 8) << 18;
   encoding |= opcode << 16;
   encoding |= interp.attribute << 10;
   encoding |= interp.component << 8;
   if (instr->opcode == aco_opcode::v_interp_mov_f32)
         else
            }
      }
   case Format::VINTERP_INREG: {
      VINTERP_inreg_instruction& interp = instr->vinterp_inreg();
   uint32_t encoding = (0b11001101 << 24);
   encoding |= reg(ctx, instr->definitions[0], 8);
   encoding |= (uint32_t)interp.wait_exp << 8;
   encoding |= (uint32_t)interp.opsel << 11;
   encoding |= (uint32_t)interp.clamp << 15;
   encoding |= opcode << 16;
            encoding = 0;
   for (unsigned i = 0; i < instr->operands.size(); i++)
         for (unsigned i = 0; i < 3; i++)
         out.push_back(encoding);
      }
   case Format::DS: {
      DS_instruction& ds = instr->ds();
   uint32_t encoding = (0b110110 << 26);
   if (ctx.gfx_level == GFX8 || ctx.gfx_level == GFX9) {
      encoding |= opcode << 17;
      } else {
      encoding |= opcode << 18;
      }
   encoding |= ((0xFF & ds.offset1) << 8);
   encoding |= (0xFFFF & ds.offset0);
   out.push_back(encoding);
   encoding = 0;
   if (!instr->definitions.empty())
         if (instr->operands.size() >= 3 && instr->operands[2].physReg() != m0)
         if (instr->operands.size() >= 2 && instr->operands[1].physReg() != m0)
         if (!instr->operands[0].isUndefined())
         out.push_back(encoding);
      }
   case Format::LDSDIR: {
      LDSDIR_instruction& dir = instr->ldsdir();
   uint32_t encoding = (0b11001110 << 24);
   encoding |= opcode << 20;
   encoding |= (uint32_t)dir.wait_vdst << 16;
   encoding |= (uint32_t)dir.attr << 10;
   encoding |= (uint32_t)dir.attr_chan << 8;
   encoding |= reg(ctx, instr->definitions[0], 8);
   out.push_back(encoding);
      }
   case Format::MUBUF: {
      MUBUF_instruction& mubuf = instr->mubuf();
   uint32_t encoding = (0b111000 << 26);
   if (ctx.gfx_level >= GFX11 && mubuf.lds) /* GFX11 has separate opcodes for LDS loads */
         else
         encoding |= opcode << 18;
   encoding |= (mubuf.glc ? 1 : 0) << 14;
   if (ctx.gfx_level <= GFX10_3)
         assert(!mubuf.addr64 || ctx.gfx_level <= GFX7);
   if (ctx.gfx_level == GFX6 || ctx.gfx_level == GFX7)
         if (ctx.gfx_level <= GFX10_3)
         if (ctx.gfx_level == GFX8 || ctx.gfx_level == GFX9) {
      assert(!mubuf.dlc); /* Device-level coherent is not supported on GFX9 and lower */
      } else if (ctx.gfx_level >= GFX11) {
      encoding |= (mubuf.slc ? 1 : 0) << 12;
      } else if (ctx.gfx_level >= GFX10) {
         }
   encoding |= 0x0FFF & mubuf.offset;
   out.push_back(encoding);
   encoding = 0;
   if (ctx.gfx_level <= GFX7 || (ctx.gfx_level >= GFX10 && ctx.gfx_level <= GFX10_3)) {
         }
   encoding |= reg(ctx, instr->operands[2]) << 24;
   if (ctx.gfx_level >= GFX11) {
      encoding |= (mubuf.tfe ? 1 : 0) << 21;
   encoding |= (mubuf.offen ? 1 : 0) << 22;
      } else {
         }
   encoding |= (reg(ctx, instr->operands[0]) >> 2) << 16;
   if (instr->operands.size() > 3 && !mubuf.lds)
         else if (!mubuf.lds)
         encoding |= reg(ctx, instr->operands[1], 8);
   out.push_back(encoding);
      }
   case Format::MTBUF: {
               uint32_t img_format = ac_get_tbuffer_format(ctx.gfx_level, mtbuf.dfmt, mtbuf.nfmt);
   uint32_t encoding = (0b111010 << 26);
   assert(img_format <= 0x7F);
   assert(!mtbuf.dlc || ctx.gfx_level >= GFX10);
   if (ctx.gfx_level >= GFX11) {
      encoding |= (mtbuf.slc ? 1 : 0) << 12;
      } else {
      /* DLC bit replaces one bit of the OPCODE on GFX10 */
      }
   if (ctx.gfx_level <= GFX10_3) {
      encoding |= (mtbuf.idxen ? 1 : 0) << 13;
      }
   encoding |= (mtbuf.glc ? 1 : 0) << 14;
   encoding |= 0x0FFF & mtbuf.offset;
            if (ctx.gfx_level == GFX8 || ctx.gfx_level == GFX9 || ctx.gfx_level >= GFX11) {
         } else {
                  out.push_back(encoding);
            encoding |= reg(ctx, instr->operands[2]) << 24;
   if (ctx.gfx_level >= GFX11) {
      encoding |= (mtbuf.tfe ? 1 : 0) << 21;
   encoding |= (mtbuf.offen ? 1 : 0) << 22;
      } else {
      encoding |= (mtbuf.tfe ? 1 : 0) << 23;
      }
   encoding |= (reg(ctx, instr->operands[0]) >> 2) << 16;
   if (instr->operands.size() > 3)
         else
                  if (ctx.gfx_level >= GFX10) {
                  out.push_back(encoding);
      }
   case Format::MIMG: {
      unsigned nsa_dwords = get_mimg_nsa_dwords(instr);
            MIMG_instruction& mimg = instr->mimg();
   uint32_t encoding = (0b111100 << 26);
   if (ctx.gfx_level >= GFX11) { /* GFX11: rearranges most fields */
      assert(nsa_dwords <= 1);
   encoding |= nsa_dwords;
   encoding |= mimg.dim << 2;
   encoding |= mimg.unrm ? 1 << 7 : 0;
   encoding |= (0xF & mimg.dmask) << 8;
   encoding |= mimg.slc ? 1 << 12 : 0;
   encoding |= mimg.dlc ? 1 << 13 : 0;
   encoding |= mimg.glc ? 1 << 14 : 0;
   encoding |= mimg.r128 ? 1 << 15 : 0;
   encoding |= mimg.a16 ? 1 << 16 : 0;
   encoding |= mimg.d16 ? 1 << 17 : 0;
      } else {
      encoding |= mimg.slc ? 1 << 25 : 0;
   encoding |= (opcode & 0x7f) << 18;
   encoding |= (opcode >> 7) & 1;
   encoding |= mimg.lwe ? 1 << 17 : 0;
   encoding |= mimg.tfe ? 1 << 16 : 0;
   encoding |= mimg.glc ? 1 << 13 : 0;
   encoding |= mimg.unrm ? 1 << 12 : 0;
   if (ctx.gfx_level <= GFX9) {
      assert(!mimg.dlc); /* Device-level coherent is not supported on GFX9 and lower */
   assert(!mimg.r128);
   encoding |= mimg.a16 ? 1 << 15 : 0;
      } else {
      encoding |= mimg.r128
               encoding |= nsa_dwords << 1;
   encoding |= mimg.dim << 3; /* GFX10: dimensionality instead of declare array */
      }
      }
            encoding = reg(ctx, instr->operands[3], 8); /* VADDR */
   if (!instr->definitions.empty()) {
         } else if (!instr->operands[2].isUndefined()) {
         }
            assert(!mimg.d16 || ctx.gfx_level >= GFX9);
   if (ctx.gfx_level >= GFX11) {
                     encoding |= mimg.tfe ? 1 << 21 : 0;
      } else {
                     encoding |= mimg.d16 ? 1 << 31 : 0;
   if (ctx.gfx_level >= GFX10) {
      /* GFX10: A16 still exists, but is in a different place */
                           if (nsa_dwords) {
      out.resize(out.size() + nsa_dwords);
   std::vector<uint32_t>::iterator nsa = std::prev(out.end(), nsa_dwords);
   for (unsigned i = 0; i < instr->operands.size() - 4u; i++)
      }
      }
   case Format::FLAT:
   case Format::SCRATCH:
   case Format::GLOBAL: {
      FLAT_instruction& flat = instr->flatlike();
   uint32_t encoding = (0b110111 << 26);
   encoding |= opcode << 18;
   if (ctx.gfx_level == GFX9 || ctx.gfx_level >= GFX11) {
      if (instr->isFlat())
         else
            } else if (ctx.gfx_level <= GFX8 || instr->isFlat()) {
      /* GFX10 has a 12-bit immediate OFFSET field,
   * but it has a hw bug: it ignores the offset, called FlatSegmentOffsetBug
   */
      } else {
      assert(flat.offset >= -2048 && flat.offset <= 2047);
      }
   if (instr->isScratch())
         else if (instr->isGlobal())
         encoding |= flat.lds ? 1 << 13 : 0;
   encoding |= flat.glc ? 1 << (ctx.gfx_level >= GFX11 ? 14 : 16) : 0;
   encoding |= flat.slc ? 1 << (ctx.gfx_level >= GFX11 ? 15 : 17) : 0;
   if (ctx.gfx_level >= GFX10) {
      assert(!flat.nv);
      } else {
         }
   out.push_back(encoding);
   encoding = reg(ctx, instr->operands[0], 8);
   if (!instr->definitions.empty())
         if (instr->operands.size() >= 3)
         if (!instr->operands[1].isUndefined()) {
      assert(ctx.gfx_level >= GFX10 || instr->operands[1].physReg() != 0x7F);
   assert(instr->format != Format::FLAT);
      } else if (instr->format != Format::FLAT ||
            /* For GFX10.3 scratch, 0x7F disables both ADDR and SADDR, unlike sgpr_null, which only
   * disables SADDR.
   */
   if (ctx.gfx_level <= GFX9 ||
      (instr->format == Format::SCRATCH && instr->operands[0].isUndefined()))
      else
      }
   if (ctx.gfx_level >= GFX11 && instr->isScratch())
         else
         out.push_back(encoding);
      }
   case Format::EXP: {
      Export_instruction& exp = instr->exp();
   uint32_t encoding;
   if (ctx.gfx_level == GFX8 || ctx.gfx_level == GFX9) {
         } else {
                  if (ctx.gfx_level >= GFX11) {
         } else {
      encoding |= exp.valid_mask ? 0b1 << 12 : 0;
      }
   encoding |= exp.done ? 0b1 << 11 : 0;
   encoding |= exp.dest << 4;
   encoding |= exp.enabled_mask;
   out.push_back(encoding);
   encoding = reg(ctx, exp.operands[0], 8);
   encoding |= reg(ctx, exp.operands[1], 8) << 8;
   encoding |= reg(ctx, exp.operands[2], 8) << 16;
   encoding |= reg(ctx, exp.operands[3], 8) << 24;
   out.push_back(encoding);
      }
   case Format::PSEUDO:
   case Format::PSEUDO_BARRIER:
      if (instr->opcode != aco_opcode::p_unit_test)
            default:
      if (instr->isDPP16()) {
                     /* first emit the instruction without the DPP operand */
   Operand dpp_op = instr->operands[0];
   instr->operands[0] = Operand(PhysReg{250}, v1);
   instr->format = (Format)((uint16_t)instr->format & ~(uint16_t)Format::DPP16);
   emit_instruction(ctx, out, instr);
   uint32_t encoding = (0xF & dpp.row_mask) << 28;
   encoding |= (0xF & dpp.bank_mask) << 24;
   encoding |= dpp.abs[1] << 23;
   encoding |= dpp.neg[1] << 22;
   encoding |= dpp.abs[0] << 21;
   encoding |= dpp.neg[0] << 20;
   encoding |= dpp.fetch_inactive << 18;
   encoding |= dpp.bound_ctrl << 19;
   encoding |= dpp.dpp_ctrl << 8;
   encoding |= reg(ctx, dpp_op, 8);
   encoding |= dpp.opsel[0] && !instr->isVOP3() ? 128 : 0;
   out.push_back(encoding);
      } else if (instr->isDPP8()) {
                     /* first emit the instruction without the DPP operand */
   Operand dpp_op = instr->operands[0];
   instr->operands[0] = Operand(PhysReg{233u + dpp.fetch_inactive}, v1);
   instr->format = (Format)((uint16_t)instr->format & ~(uint16_t)Format::DPP8);
   emit_instruction(ctx, out, instr);
   uint32_t encoding = reg(ctx, dpp_op, 8);
   encoding |= dpp.opsel[0] && !instr->isVOP3() ? 128 : 0;
   encoding |= dpp.lane_sel << 8;
   out.push_back(encoding);
      } else if (instr->isVOP3()) {
               if (instr->isVOP2()) {
         } else if (instr->isVOP1()) {
      if (ctx.gfx_level == GFX8 || ctx.gfx_level == GFX9)
         else
      } else if (instr->isVOPC()) {
         } else if (instr->isVINTRP()) {
                  uint32_t encoding;
   if (ctx.gfx_level <= GFX9) {
         } else if (ctx.gfx_level >= GFX10) {
         } else {
                  if (ctx.gfx_level <= GFX7) {
      encoding |= opcode << 17;
      } else {
      encoding |= opcode << 16;
      }
   encoding |= vop3.opsel << 11;
   for (unsigned i = 0; i < 3; i++)
         /* On GFX9 and older, v_cmpx implicitly writes exec besides writing an SGPR pair.
   * On GFX10 and newer, v_cmpx always writes just exec.
   */
   if (instr->definitions.size() == 2 && instr->isVOPC())
         else if (instr->definitions.size() == 2)
         encoding |= reg(ctx, instr->definitions[0], 8);
   out.push_back(encoding);
   encoding = 0;
   if (instr->opcode == aco_opcode::v_interp_mov_f32) {
         } else if (instr->opcode == aco_opcode::v_writelane_b32_e64) {
      encoding |= reg(ctx, instr->operands[0]) << 0;
   encoding |= reg(ctx, instr->operands[1]) << 9;
      } else {
      for (unsigned i = 0; i < instr->operands.size(); i++)
      }
   encoding |= vop3.omod << 27;
   for (unsigned i = 0; i < 3; i++)
               } else if (instr->isVOP3P()) {
               uint32_t encoding;
   if (ctx.gfx_level == GFX9) {
         } else if (ctx.gfx_level >= GFX10) {
         } else {
                  encoding |= opcode << 16;
   encoding |= (vop3.clamp ? 1 : 0) << 15;
   encoding |= vop3.opsel_lo << 11;
   encoding |= ((vop3.opsel_hi & 0x4) ? 1 : 0) << 14;
   for (unsigned i = 0; i < 3; i++)
         encoding |= reg(ctx, instr->definitions[0], 8);
   out.push_back(encoding);
   encoding = 0;
   for (unsigned i = 0; i < instr->operands.size(); i++)
         encoding |= (vop3.opsel_hi & 0x3) << 27;
   for (unsigned i = 0; i < 3; i++)
            } else if (instr->isSDWA()) {
                     /* first emit the instruction without the SDWA operand */
   Operand sdwa_op = instr->operands[0];
   instr->operands[0] = Operand(PhysReg{249}, v1);
                           if (instr->isVOPC()) {
      if (instr->definitions[0].physReg() !=
      (ctx.gfx_level >= GFX10 && is_cmpx(instr->opcode) ? exec : vcc)) {
   encoding |= reg(ctx, instr->definitions[0]) << 8;
      }
      } else {
      encoding |= sdwa.dst_sel.to_sdwa_sel(instr->definitions[0].physReg().byte()) << 8;
   uint32_t dst_u = sdwa.dst_sel.sign_extend() ? 1 : 0;
   if (instr->definitions[0].bytes() < 4) /* dst_preserve */
         encoding |= dst_u << 11;
   encoding |= (sdwa.clamp ? 1 : 0) << 13;
               encoding |= sdwa.sel[0].to_sdwa_sel(sdwa_op.physReg().byte()) << 16;
   encoding |= sdwa.sel[0].sign_extend() ? 1 << 19 : 0;
                  if (instr->operands.size() >= 2) {
      encoding |= sdwa.sel[1].to_sdwa_sel(instr->operands[1].physReg().byte()) << 24;
   encoding |= sdwa.sel[1].sign_extend() ? 1 << 27 : 0;
   encoding |= sdwa.abs[1] << 29;
               encoding |= reg(ctx, sdwa_op, 8);
   encoding |= (sdwa_op.physReg() < 256) << 23;
   if (instr->operands.size() >= 2)
            } else {
         }
               /* append literal dword */
   for (const Operand& op : instr->operands) {
      if (op.isLiteral()) {
      out.push_back(op.constantValue());
            }
      void
   emit_block(asm_context& ctx, std::vector<uint32_t>& out, Block& block)
   {
         #if 0
         int start_idx = out.size();
   std::cerr << "Encoding:\t" << std::endl;
   aco_print_instr(&*instr, stderr);
   #endif
         #if 0
         for (int i = start_idx; i < out.size(); i++)
   #endif
         }
      void
   fix_exports(asm_context& ctx, std::vector<uint32_t>& out, Program* program)
   {
      bool exported = false;
   for (Block& block : program->blocks) {
      if (!(block.kind & block_kind_export_end))
         std::vector<aco_ptr<Instruction>>::reverse_iterator it = block.instructions.rbegin();
   while (it != block.instructions.rend()) {
      if ((*it)->isEXP()) {
      Export_instruction& exp = (*it)->exp();
   if (program->stage.hw == AC_HW_VERTEX_SHADER ||
      program->stage.hw == AC_HW_NEXT_GEN_GEOMETRY_SHADER) {
   if (exp.dest >= V_008DFC_SQ_EXP_POS && exp.dest <= (V_008DFC_SQ_EXP_POS + 3)) {
      exp.done = true;
   exported = true;
         } else {
      exp.done = true;
   exp.valid_mask = true;
   exported = true;
         } else if ((*it)->definitions.size() && (*it)->definitions[0].physReg() == exec) {
         } else if ((*it)->opcode == aco_opcode::s_setpc_b64) {
      /* Do not abort for VS/TES as NGG if they are non-monolithic shaders
   * because a jump would be emitted.
   */
   exported |= (program->stage.sw == SWStage::VS || program->stage.sw == SWStage::TES) &&
            }
                  /* GFX10+ FS may not export anything if no discard is used. */
            if (!exported && !may_skip_export) {
      /* Abort in order to avoid a GPU hang. */
   bool is_vertex_or_ngg = (program->stage.hw == AC_HW_VERTEX_SHADER ||
         aco_err(program,
         aco_print_program(program, stderr);
         }
      static void
   insert_code(asm_context& ctx, std::vector<uint32_t>& out, unsigned insert_before,
         {
               /* Update the offset of each affected block */
   for (Block& block : ctx.program->blocks) {
      if (block.offset >= insert_before)
               /* Find first branch after the inserted code */
   auto branch_it = std::find_if(ctx.branches.begin(), ctx.branches.end(),
                  /* Update the locations of branches */
   for (; branch_it != ctx.branches.end(); ++branch_it)
            /* Update the locations of p_constaddr instructions */
   for (auto& constaddr : ctx.constaddrs) {
      constaddr_info& info = constaddr.second;
   if (info.getpc_end >= insert_before)
         if (info.add_literal >= insert_before)
      }
   for (auto& constaddr : ctx.resumeaddrs) {
      constaddr_info& info = constaddr.second;
   if (info.getpc_end >= insert_before)
         if (info.add_literal >= insert_before)
               if (ctx.symbols) {
      for (auto& symbol : *ctx.symbols) {
      if (symbol.offset >= insert_before)
            }
      static void
   fix_branches_gfx10(asm_context& ctx, std::vector<uint32_t>& out)
   {
      /* Branches with an offset of 0x3f are buggy on GFX10,
   * we workaround by inserting NOPs if needed.
   */
            do {
      auto buggy_branch_it = std::find_if(
      ctx.branches.begin(), ctx.branches.end(),
   [&ctx](const auto& branch) -> bool {
      return ((int)ctx.program->blocks[branch.second->block].offset - branch.first - 1) ==
                     if (gfx10_3f_bug) {
      /* Insert an s_nop after the branch */
   constexpr uint32_t s_nop_0 = 0xbf800000u;
            }
      void
   emit_long_jump(asm_context& ctx, SOPP_instruction* branch, bool backwards,
         {
               Definition def;
   if (branch->definitions.empty()) {
      assert(ctx.program->blocks[branch->block].kind & block_kind_discard_early_exit);
      } else {
                  Definition def_tmp_lo(def.physReg(), s1);
   Operand op_tmp_lo(def.physReg(), s1);
   Definition def_tmp_hi(def.physReg().advance(4), s1);
                     if (branch->opcode != aco_opcode::s_branch) {
      /* for conditional branches, skip the long jump if the condition is false */
   aco_opcode inv;
   switch (branch->opcode) {
   case aco_opcode::s_cbranch_scc0: inv = aco_opcode::s_cbranch_scc1; break;
   case aco_opcode::s_cbranch_scc1: inv = aco_opcode::s_cbranch_scc0; break;
   case aco_opcode::s_cbranch_vccz: inv = aco_opcode::s_cbranch_vccnz; break;
   case aco_opcode::s_cbranch_vccnz: inv = aco_opcode::s_cbranch_vccz; break;
   case aco_opcode::s_cbranch_execz: inv = aco_opcode::s_cbranch_execnz; break;
   case aco_opcode::s_cbranch_execnz: inv = aco_opcode::s_cbranch_execz; break;
   default: unreachable("Unhandled long jump.");
   }
   instr.reset(bld.sopp(inv, -1, 6));
               /* create the new PC and stash SCC in the LSB */
   instr.reset(bld.sop1(aco_opcode::s_getpc_b64, def).instr);
            instr.reset(
         emit_instruction(ctx, out, instr.get());
                     /* restore SCC and clear the LSB of the new PC */
   instr.reset(bld.sopc(aco_opcode::s_bitcmp1_b32, def_tmp_lo, op_tmp_lo, Operand::zero()).instr);
   emit_instruction(ctx, out, instr.get());
   instr.reset(bld.sop1(aco_opcode::s_bitset0_b32, def_tmp_lo, Operand::zero()).instr);
            /* create the s_setpc_b64 to jump */
   instr.reset(bld.sop1(aco_opcode::s_setpc_b64, Operand(def.physReg(), s2)).instr);
      }
      void
   fix_branches(asm_context& ctx, std::vector<uint32_t>& out)
   {
      bool repeat = false;
   do {
               if (ctx.gfx_level == GFX10)
            for (std::pair<int, SOPP_instruction*>& branch : ctx.branches) {
      int offset = (int)ctx.program->blocks[branch.second->block].offset - branch.first - 1;
   if ((offset < INT16_MIN || offset > INT16_MAX) && !branch.second->pass_flags) {
      std::vector<uint32_t> long_jump;
   bool backwards =
                                 repeat = true;
               if (branch.second->pass_flags) {
      int after_getpc = branch.first + branch.second->pass_flags - 2;
   offset = (int)ctx.program->blocks[branch.second->block].offset - after_getpc;
      } else {
      out[branch.first] &= 0xffff0000u;
               }
      void
   fix_constaddrs(asm_context& ctx, std::vector<uint32_t>& out)
   {
      for (auto& constaddr : ctx.constaddrs) {
      constaddr_info& info = constaddr.second;
            if (ctx.symbols) {
      struct aco_symbol sym;
   sym.id = aco_symbol_const_data_addr;
   sym.offset = info.add_literal;
         }
   for (auto& addr : ctx.resumeaddrs) {
      constaddr_info& info = addr.second;
   const Block& block = ctx.program->blocks[out[info.add_literal]];
   assert(block.kind & block_kind_resume);
         }
      void
   align_block(asm_context& ctx, std::vector<uint32_t>& code, Block& block)
   {
      /* Blocks with block_kind_loop_exit might be eliminated after jump threading, so we instead find
   * loop exits using loop_nest_depth.
   */
   if (ctx.loop_header && !block.linear_preds.empty() &&
      block.loop_nest_depth < ctx.loop_header->loop_nest_depth) {
   Block* loop_header = ctx.loop_header;
   ctx.loop_header = NULL;
                     /* On GFX10.3+, change the prefetch mode if the loop fits into 2 or 3 cache lines.
   * Don't use the s_inst_prefetch instruction on GFX10 as it might cause hangs.
   */
   const bool change_prefetch =
            if (change_prefetch) {
      Builder bld(ctx.program);
   int16_t prefetch_mode = loop_num_cl == 3 ? 0x1 : 0x2;
   aco_ptr<Instruction> instr(bld.sopp(aco_opcode::s_inst_prefetch, -1, prefetch_mode));
                  /* Change prefetch mode back to default (0x3). */
   instr->sopp().imm = 0x3;
               const unsigned loop_start_cl = loop_header->offset >> 4;
            /* Align the loop if it fits into the fetched cache lines or if we can
   * reduce the number of cache lines with less than 8 NOPs.
   */
   const bool align_loop = loop_end_cl - loop_start_cl >= loop_num_cl &&
            if (align_loop) {
      nops.clear();
   nops.resize(16 - (loop_header->offset % 16), 0xbf800000u);
                  if (block.kind & block_kind_loop_header) {
      /* In case of nested loops, only handle the inner-most loops in order
   * to not break the alignment of inner loops by handling outer loops.
   * Also ignore loops without back-edge.
   */
               /* align resume shaders with cache line */
   if (block.kind & block_kind_resume) {
      size_t cache_aligned = align(code.size(), 16);
   code.resize(cache_aligned, 0xbf800000u); /* s_nop 0 */
         }
      unsigned
   emit_program(Program* program, std::vector<uint32_t>& code, std::vector<struct aco_symbol>* symbols,
         {
               /* Prolog has no exports. */
   if (!program->is_prolog && !program->info.has_epilog &&
      (program->stage.hw == AC_HW_VERTEX_SHADER || program->stage.hw == AC_HW_PIXEL_SHADER ||
   program->stage.hw == AC_HW_NEXT_GEN_GEOMETRY_SHADER))
         for (Block& block : program->blocks) {
      block.offset = code.size();
   align_block(ctx, code, block);
                                 /* Add end-of-code markers for the UMR disassembler. */
   if (append_endpgm)
                     while (program->constant_data.size() % 4u)
         /* Copy constant data */
   code.insert(code.end(), (uint32_t*)program->constant_data.data(),
            program->config->scratch_bytes_per_wave =
               }
      } // namespace aco
