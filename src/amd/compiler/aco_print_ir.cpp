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
      #include "common/ac_shader_util.h"
   #include "common/sid.h"
      #include <array>
      namespace aco {
      const std::array<const char*, num_reduce_ops> reduce_ops = []()
   {
      std::array<const char*, num_reduce_ops> ret{};
   ret[iadd8] = "iadd8";
   ret[iadd16] = "iadd16";
   ret[iadd32] = "iadd32";
   ret[iadd64] = "iadd64";
   ret[imul8] = "imul8";
   ret[imul16] = "imul16";
   ret[imul32] = "imul32";
   ret[imul64] = "imul64";
   ret[fadd16] = "fadd16";
   ret[fadd32] = "fadd32";
   ret[fadd64] = "fadd64";
   ret[fmul16] = "fmul16";
   ret[fmul32] = "fmul32";
   ret[fmul64] = "fmul64";
   ret[imin8] = "imin8";
   ret[imin16] = "imin16";
   ret[imin32] = "imin32";
   ret[imin64] = "imin64";
   ret[imax8] = "imax8";
   ret[imax16] = "imax16";
   ret[imax32] = "imax32";
   ret[imax64] = "imax64";
   ret[umin8] = "umin8";
   ret[umin16] = "umin16";
   ret[umin32] = "umin32";
   ret[umin64] = "umin64";
   ret[umax8] = "umax8";
   ret[umax16] = "umax16";
   ret[umax32] = "umax32";
   ret[umax64] = "umax64";
   ret[fmin16] = "fmin16";
   ret[fmin32] = "fmin32";
   ret[fmin64] = "fmin64";
   ret[fmax16] = "fmax16";
   ret[fmax32] = "fmax32";
   ret[fmax64] = "fmax64";
   ret[iand8] = "iand8";
   ret[iand16] = "iand16";
   ret[iand32] = "iand32";
   ret[iand64] = "iand64";
   ret[ior8] = "ior8";
   ret[ior16] = "ior16";
   ret[ior32] = "ior32";
   ret[ior64] = "ior64";
   ret[ixor8] = "ixor8";
   ret[ixor16] = "ixor16";
   ret[ixor32] = "ixor32";
   ret[ixor64] = "ixor64";
      }();
      static void
   print_reg_class(const RegClass rc, FILE* output)
   {
      if (rc.is_subdword()) {
         } else if (rc.type() == RegType::sgpr) {
         } else if (rc.is_linear()) {
         } else {
            }
      void
   print_physReg(PhysReg reg, unsigned bytes, FILE* output, unsigned flags)
   {
      if (reg == 124) {
         } else if (reg == 106) {
         } else if (reg == 253) {
         } else if (reg == 126) {
         } else if (reg == 125) {
         } else {
      bool is_vgpr = reg / 256;
   unsigned r = reg % 256;
   unsigned size = DIV_ROUND_UP(bytes, 4);
   if (size == 1 && (flags & print_no_ssa)) {
         } else {
      fprintf(output, "%c[%d", is_vgpr ? 'v' : 's', r);
   if (size > 1)
         else
      }
   if (reg.byte() || bytes % 4)
         }
      static void
   print_constant(uint8_t reg, FILE* output)
   {
      if (reg >= 128 && reg <= 192) {
      fprintf(output, "%d", reg - 128);
      } else if (reg >= 192 && reg <= 208) {
      fprintf(output, "%d", 192 - reg);
               switch (reg) {
   case 240: fprintf(output, "0.5"); break;
   case 241: fprintf(output, "-0.5"); break;
   case 242: fprintf(output, "1.0"); break;
   case 243: fprintf(output, "-1.0"); break;
   case 244: fprintf(output, "2.0"); break;
   case 245: fprintf(output, "-2.0"); break;
   case 246: fprintf(output, "4.0"); break;
   case 247: fprintf(output, "-4.0"); break;
   case 248: fprintf(output, "1/(2*PI)"); break;
      }
      void
   aco_print_operand(const Operand* operand, FILE* output, unsigned flags)
   {
      if (operand->isLiteral() || (operand->isConstant() && operand->bytes() == 1)) {
      if (operand->bytes() == 1)
         else if (operand->bytes() == 2)
         else
      } else if (operand->isConstant()) {
         } else if (operand->isUndefined()) {
      print_reg_class(operand->regClass(), output);
      } else {
      if (operand->isLateKill())
         if (operand->is16bit())
         if (operand->is24bit())
         if ((flags & print_kill) && operand->isKill())
            if (!(flags & print_no_ssa))
            if (operand->isFixed())
         }
      static void
   print_definition(const Definition* definition, FILE* output, unsigned flags)
   {
      if (!(flags & print_no_ssa))
         if (definition->isPrecise())
         if (definition->isNUW())
         if (definition->isNoCSE())
         if ((flags & print_kill) && definition->isKill())
         if (!(flags & print_no_ssa))
            if (definition->isFixed())
      }
      static void
   print_storage(storage_class storage, FILE* output)
   {
      fprintf(output, " storage:");
   int printed = 0;
   if (storage & storage_buffer)
         if (storage & storage_gds)
         if (storage & storage_image)
         if (storage & storage_shared)
         if (storage & storage_task_payload)
         if (storage & storage_vmem_output)
         if (storage & storage_scratch)
         if (storage & storage_vgpr_spill)
      }
      static void
   print_semantics(memory_semantics sem, FILE* output)
   {
      fprintf(output, " semantics:");
   int printed = 0;
   if (sem & semantic_acquire)
         if (sem & semantic_release)
         if (sem & semantic_volatile)
         if (sem & semantic_private)
         if (sem & semantic_can_reorder)
         if (sem & semantic_atomic)
         if (sem & semantic_rmw)
      }
      static void
   print_scope(sync_scope scope, FILE* output, const char* prefix = "scope")
   {
      fprintf(output, " %s:", prefix);
   switch (scope) {
   case scope_invocation: fprintf(output, "invocation"); break;
   case scope_subgroup: fprintf(output, "subgroup"); break;
   case scope_workgroup: fprintf(output, "workgroup"); break;
   case scope_queuefamily: fprintf(output, "queuefamily"); break;
   case scope_device: fprintf(output, "device"); break;
      }
      static void
   print_sync(memory_sync_info sync, FILE* output)
   {
      if (sync.storage)
         if (sync.semantics)
         if (sync.scope != scope_invocation)
      }
      static void
   print_instr_format_specific(enum amd_gfx_level gfx_level, const Instruction* instr, FILE* output)
   {
      switch (instr->format) {
   case Format::SOPK: {
      const SOPK_instruction& sopk = instr->sopk();
   fprintf(output, " imm:%d", sopk.imm & 0x8000 ? (sopk.imm - 65536) : sopk.imm);
      }
   case Format::SOPP: {
      uint16_t imm = instr->sopp().imm;
   switch (instr->opcode) {
   case aco_opcode::s_waitcnt: {
      wait_imm unpacked(gfx_level, imm);
   if (unpacked.vm != wait_imm::unset_counter)
         if (unpacked.exp != wait_imm::unset_counter)
         if (unpacked.lgkm != wait_imm::unset_counter)
            }
   case aco_opcode::s_waitcnt_depctr: {
      unsigned va_vdst = (imm >> 12) & 0xf;
   unsigned va_sdst = (imm >> 9) & 0x7;
   unsigned va_ssrc = (imm >> 8) & 0x1;
   unsigned hold_cnt = (imm >> 7) & 0x1;
   unsigned vm_vsrc = (imm >> 2) & 0x7;
   unsigned va_vcc = (imm >> 1) & 0x1;
   unsigned sa_sdst = imm & 0x1;
   if (va_vdst != 0xf)
         if (va_sdst != 0x7)
         if (va_ssrc != 0x1)
         if (hold_cnt != 0x1)
         if (vm_vsrc != 0x7)
         if (va_vcc != 0x1)
         if (sa_sdst != 0x1)
            }
   case aco_opcode::s_delay_alu: {
      unsigned delay[2] = {imm & 0xfu, (imm >> 7) & 0xfu};
   unsigned skip = (imm >> 4) & 0x3;
   for (unsigned i = 0; i < 2; i++) {
      if (i == 1 && skip) {
      if (skip == 1)
                           alu_delay_wait wait = (alu_delay_wait)delay[i];
   if (wait >= alu_delay_wait::VALU_DEP_1 && wait <= alu_delay_wait::VALU_DEP_4)
         else if (wait >= alu_delay_wait::TRANS32_DEP_1 && wait <= alu_delay_wait::TRANS32_DEP_3)
      fprintf(output, " trans32_dep_%u",
      else if (wait == alu_delay_wait::FMA_ACCUM_CYCLE_1)
         else if (wait >= alu_delay_wait::SALU_CYCLE_1 && wait <= alu_delay_wait::SALU_CYCLE_3)
      fprintf(output, " salu_cycle_%u",
   }
      }
   case aco_opcode::s_endpgm:
   case aco_opcode::s_endpgm_saved:
   case aco_opcode::s_endpgm_ordered_ps_done:
   case aco_opcode::s_wakeup:
   case aco_opcode::s_barrier:
   case aco_opcode::s_icache_inv:
   case aco_opcode::s_ttracedata:
   case aco_opcode::s_set_gpr_idx_off: {
         }
   case aco_opcode::s_sendmsg: {
      unsigned id = imm & sendmsg_id_mask;
   static_assert(sendmsg_gs == sendmsg_hs_tessfactor);
   static_assert(sendmsg_gs_done == sendmsg_dealloc_vgprs);
   switch (id) {
   case sendmsg_none: fprintf(output, " sendmsg(MSG_NONE)"); break;
   case sendmsg_gs:
      if (gfx_level >= GFX11)
         else
      fprintf(output, " sendmsg(gs%s%s, %u)", imm & 0x10 ? ", cut" : "",
         case sendmsg_gs_done:
      if (gfx_level >= GFX11)
         else
      fprintf(output, " sendmsg(gs_done%s%s, %u)", imm & 0x10 ? ", cut" : "",
         case sendmsg_save_wave: fprintf(output, " sendmsg(save_wave)"); break;
   case sendmsg_stall_wave_gen: fprintf(output, " sendmsg(stall_wave_gen)"); break;
   case sendmsg_halt_waves: fprintf(output, " sendmsg(halt_waves)"); break;
   case sendmsg_ordered_ps_done: fprintf(output, " sendmsg(ordered_ps_done)"); break;
   case sendmsg_early_prim_dealloc: fprintf(output, " sendmsg(early_prim_dealloc)"); break;
   case sendmsg_gs_alloc_req: fprintf(output, " sendmsg(gs_alloc_req)"); break;
   case sendmsg_get_doorbell: fprintf(output, " sendmsg(get_doorbell)"); break;
   case sendmsg_get_ddid: fprintf(output, " sendmsg(get_ddid)"); break;
   default: fprintf(output, " imm:%u", imm);
   }
      }
   case aco_opcode::s_wait_event: {
      if (!(imm & wait_event_imm_dont_wait_export_ready))
            }
   default: {
      if (imm)
            }
   }
   if (instr->sopp().block != -1)
            }
   case Format::SOP1: {
      if (instr->opcode == aco_opcode::s_sendmsg_rtn_b32 ||
      instr->opcode == aco_opcode::s_sendmsg_rtn_b64) {
   unsigned id = instr->operands[0].constantValue();
   switch (id) {
   case sendmsg_rtn_get_doorbell: fprintf(output, " sendmsg(rtn_get_doorbell)"); break;
   case sendmsg_rtn_get_ddid: fprintf(output, " sendmsg(rtn_get_ddid)"); break;
   case sendmsg_rtn_get_tma: fprintf(output, " sendmsg(rtn_get_tma)"); break;
   case sendmsg_rtn_get_realtime: fprintf(output, " sendmsg(rtn_get_realtime)"); break;
   case sendmsg_rtn_save_wave: fprintf(output, " sendmsg(rtn_save_wave)"); break;
   case sendmsg_rtn_get_tba: fprintf(output, " sendmsg(rtn_get_tba)"); break;
   default: break;
   }
      }
      }
   case Format::SMEM: {
      const SMEM_instruction& smem = instr->smem();
   if (smem.glc)
         if (smem.dlc)
         if (smem.nv)
         print_sync(smem.sync, output);
      }
   case Format::VINTERP_INREG: {
      const VINTERP_inreg_instruction& vinterp = instr->vinterp_inreg();
   if (vinterp.wait_exp != 7)
            }
   case Format::VINTRP: {
      const VINTRP_instruction& vintrp = instr->vintrp();
   fprintf(output, " attr%d.%c", vintrp.attribute, "xyzw"[vintrp.component]);
      }
   case Format::DS: {
      const DS_instruction& ds = instr->ds();
   if (ds.offset0)
         if (ds.offset1)
         if (ds.gds)
         print_sync(ds.sync, output);
      }
   case Format::LDSDIR: {
      const LDSDIR_instruction& ldsdir = instr->ldsdir();
   if (instr->opcode == aco_opcode::lds_param_load)
         if (ldsdir.wait_vdst != 15)
         print_sync(ldsdir.sync, output);
      }
   case Format::MUBUF: {
      const MUBUF_instruction& mubuf = instr->mubuf();
   if (mubuf.offset)
         if (mubuf.offen)
         if (mubuf.idxen)
         if (mubuf.addr64)
         if (mubuf.glc)
         if (mubuf.dlc)
         if (mubuf.slc)
         if (mubuf.tfe)
         if (mubuf.lds)
         if (mubuf.disable_wqm)
         print_sync(mubuf.sync, output);
      }
   case Format::MIMG: {
      const MIMG_instruction& mimg = instr->mimg();
   unsigned identity_dmask =
         if ((mimg.dmask & identity_dmask) != identity_dmask)
      fprintf(output, " dmask:%s%s%s%s", mimg.dmask & 0x1 ? "x" : "",
            switch (mimg.dim) {
   case ac_image_1d: fprintf(output, " 1d"); break;
   case ac_image_2d: fprintf(output, " 2d"); break;
   case ac_image_3d: fprintf(output, " 3d"); break;
   case ac_image_cube: fprintf(output, " cube"); break;
   case ac_image_1darray: fprintf(output, " 1darray"); break;
   case ac_image_2darray: fprintf(output, " 2darray"); break;
   case ac_image_2dmsaa: fprintf(output, " 2dmsaa"); break;
   case ac_image_2darraymsaa: fprintf(output, " 2darraymsaa"); break;
   }
   if (mimg.unrm)
         if (mimg.glc)
         if (mimg.dlc)
         if (mimg.slc)
         if (mimg.tfe)
         if (mimg.da)
         if (mimg.lwe)
         if (mimg.r128)
         if (mimg.a16)
         if (mimg.d16)
         if (mimg.disable_wqm)
         print_sync(mimg.sync, output);
      }
   case Format::EXP: {
      const Export_instruction& exp = instr->exp();
   unsigned identity_mask = exp.compressed ? 0x5 : 0xf;
   if ((exp.enabled_mask & identity_mask) != identity_mask)
      fprintf(output, " en:%c%c%c%c", exp.enabled_mask & 0x1 ? 'r' : '*',
            if (exp.compressed)
         if (exp.done)
         if (exp.valid_mask)
            if (exp.dest <= V_008DFC_SQ_EXP_MRT + 7)
         else if (exp.dest == V_008DFC_SQ_EXP_MRTZ)
         else if (exp.dest == V_008DFC_SQ_EXP_NULL)
         else if (exp.dest >= V_008DFC_SQ_EXP_POS && exp.dest <= V_008DFC_SQ_EXP_POS + 3)
         else if (exp.dest >= V_008DFC_SQ_EXP_PARAM && exp.dest <= V_008DFC_SQ_EXP_PARAM + 31)
            }
   case Format::PSEUDO_BRANCH: {
      const Pseudo_branch_instruction& branch = instr->branch();
   /* Note: BB0 cannot be a branch target */
   if (branch.target[0] != 0)
         if (branch.target[1] != 0)
            }
   case Format::PSEUDO_REDUCTION: {
      const Pseudo_reduction_instruction& reduce = instr->reduction();
   fprintf(output, " op:%s", reduce_ops[reduce.reduce_op]);
   if (reduce.cluster_size)
            }
   case Format::PSEUDO_BARRIER: {
      const Pseudo_barrier_instruction& barrier = instr->barrier();
   print_sync(barrier.sync, output);
   print_scope(barrier.exec_scope, output, "exec_scope");
      }
   case Format::FLAT:
   case Format::GLOBAL:
   case Format::SCRATCH: {
      const FLAT_instruction& flat = instr->flatlike();
   if (flat.offset)
         if (flat.glc)
         if (flat.dlc)
         if (flat.slc)
         if (flat.lds)
         if (flat.nv)
         if (flat.disable_wqm)
         print_sync(flat.sync, output);
      }
   case Format::MTBUF: {
      const MTBUF_instruction& mtbuf = instr->mtbuf();
   fprintf(output, " dfmt:");
   switch (mtbuf.dfmt) {
   case V_008F0C_BUF_DATA_FORMAT_8: fprintf(output, "8"); break;
   case V_008F0C_BUF_DATA_FORMAT_16: fprintf(output, "16"); break;
   case V_008F0C_BUF_DATA_FORMAT_8_8: fprintf(output, "8_8"); break;
   case V_008F0C_BUF_DATA_FORMAT_32: fprintf(output, "32"); break;
   case V_008F0C_BUF_DATA_FORMAT_16_16: fprintf(output, "16_16"); break;
   case V_008F0C_BUF_DATA_FORMAT_10_11_11: fprintf(output, "10_11_11"); break;
   case V_008F0C_BUF_DATA_FORMAT_11_11_10: fprintf(output, "11_11_10"); break;
   case V_008F0C_BUF_DATA_FORMAT_10_10_10_2: fprintf(output, "10_10_10_2"); break;
   case V_008F0C_BUF_DATA_FORMAT_2_10_10_10: fprintf(output, "2_10_10_10"); break;
   case V_008F0C_BUF_DATA_FORMAT_8_8_8_8: fprintf(output, "8_8_8_8"); break;
   case V_008F0C_BUF_DATA_FORMAT_32_32: fprintf(output, "32_32"); break;
   case V_008F0C_BUF_DATA_FORMAT_16_16_16_16: fprintf(output, "16_16_16_16"); break;
   case V_008F0C_BUF_DATA_FORMAT_32_32_32: fprintf(output, "32_32_32"); break;
   case V_008F0C_BUF_DATA_FORMAT_32_32_32_32: fprintf(output, "32_32_32_32"); break;
   case V_008F0C_BUF_DATA_FORMAT_RESERVED_15: fprintf(output, "reserved15"); break;
   }
   fprintf(output, " nfmt:");
   switch (mtbuf.nfmt) {
   case V_008F0C_BUF_NUM_FORMAT_UNORM: fprintf(output, "unorm"); break;
   case V_008F0C_BUF_NUM_FORMAT_SNORM: fprintf(output, "snorm"); break;
   case V_008F0C_BUF_NUM_FORMAT_USCALED: fprintf(output, "uscaled"); break;
   case V_008F0C_BUF_NUM_FORMAT_SSCALED: fprintf(output, "sscaled"); break;
   case V_008F0C_BUF_NUM_FORMAT_UINT: fprintf(output, "uint"); break;
   case V_008F0C_BUF_NUM_FORMAT_SINT: fprintf(output, "sint"); break;
   case V_008F0C_BUF_NUM_FORMAT_SNORM_OGL: fprintf(output, "snorm"); break;
   case V_008F0C_BUF_NUM_FORMAT_FLOAT: fprintf(output, "float"); break;
   }
   if (mtbuf.offset)
         if (mtbuf.offen)
         if (mtbuf.idxen)
         if (mtbuf.glc)
         if (mtbuf.dlc)
         if (mtbuf.slc)
         if (mtbuf.tfe)
         if (mtbuf.disable_wqm)
         print_sync(mtbuf.sync, output);
      }
   default: {
         }
   }
   if (instr->isVALU()) {
      const VALU_instruction& valu = instr->valu();
   switch (valu.omod) {
   case 1: fprintf(output, " *2"); break;
   case 2: fprintf(output, " *4"); break;
   case 3: fprintf(output, " *0.5"); break;
   }
   if (valu.clamp)
         if (valu.opsel & (1 << 3))
               if (instr->isDPP16()) {
      const DPP16_instruction& dpp = instr->dpp16();
   if (dpp.dpp_ctrl <= 0xff) {
      fprintf(output, " quad_perm:[%d,%d,%d,%d]", dpp.dpp_ctrl & 0x3, (dpp.dpp_ctrl >> 2) & 0x3,
      } else if (dpp.dpp_ctrl >= 0x101 && dpp.dpp_ctrl <= 0x10f) {
         } else if (dpp.dpp_ctrl >= 0x111 && dpp.dpp_ctrl <= 0x11f) {
         } else if (dpp.dpp_ctrl >= 0x121 && dpp.dpp_ctrl <= 0x12f) {
         } else if (dpp.dpp_ctrl == dpp_wf_sl1) {
         } else if (dpp.dpp_ctrl == dpp_wf_rl1) {
         } else if (dpp.dpp_ctrl == dpp_wf_sr1) {
         } else if (dpp.dpp_ctrl == dpp_wf_rr1) {
         } else if (dpp.dpp_ctrl == dpp_row_mirror) {
         } else if (dpp.dpp_ctrl == dpp_row_half_mirror) {
         } else if (dpp.dpp_ctrl == dpp_row_bcast15) {
         } else if (dpp.dpp_ctrl == dpp_row_bcast31) {
         } else if (dpp.dpp_ctrl >= dpp_row_share(0) && dpp.dpp_ctrl <= dpp_row_share(15)) {
         } else if (dpp.dpp_ctrl >= dpp_row_xmask(0) && dpp.dpp_ctrl <= dpp_row_xmask(15)) {
         } else {
         }
   if (dpp.row_mask != 0xf)
         if (dpp.bank_mask != 0xf)
         if (dpp.bound_ctrl)
         if (dpp.fetch_inactive)
      } else if (instr->isDPP8()) {
      const DPP8_instruction& dpp = instr->dpp8();
   fprintf(output, " dpp8:[");
   for (unsigned i = 0; i < 8; i++)
         fprintf(output, "]");
   if (dpp.fetch_inactive)
      } else if (instr->isSDWA()) {
      const SDWA_instruction& sdwa = instr->sdwa();
   if (!instr->isVOPC()) {
      char sext = sdwa.dst_sel.sign_extend() ? 's' : 'u';
   unsigned offset = sdwa.dst_sel.offset();
   if (instr->definitions[0].isFixed())
         switch (sdwa.dst_sel.size()) {
   case 1: fprintf(output, " dst_sel:%cbyte%u", sext, offset); break;
   case 2: fprintf(output, " dst_sel:%cword%u", sext, offset >> 1); break;
   case 4: fprintf(output, " dst_sel:dword"); break;
   default: break;
   }
   if (instr->definitions[0].bytes() < 4)
      }
   for (unsigned i = 0; i < std::min<unsigned>(2, instr->operands.size()); i++) {
      char sext = sdwa.sel[i].sign_extend() ? 's' : 'u';
   unsigned offset = sdwa.sel[i].offset();
   if (instr->operands[i].isFixed())
         switch (sdwa.sel[i].size()) {
   case 1: fprintf(output, " src%d_sel:%cbyte%u", i, sext, offset); break;
   case 2: fprintf(output, " src%d_sel:%cword%u", i, sext, offset >> 1); break;
   case 4: fprintf(output, " src%d_sel:dword", i); break;
   default: break;
            }
      void
   aco_print_instr(enum amd_gfx_level gfx_level, const Instruction* instr, FILE* output,
         {
      if (!instr->definitions.empty()) {
      for (unsigned i = 0; i < instr->definitions.size(); ++i) {
      print_definition(&instr->definitions[i], output, flags);
   if (i + 1 != instr->definitions.size())
      }
      }
   fprintf(output, "%s", instr_info.name[(int)instr->opcode]);
   if (instr->operands.size()) {
      const unsigned num_operands = instr->operands.size();
   bool* const abs = (bool*)alloca(num_operands * sizeof(bool));
   bool* const neg = (bool*)alloca(num_operands * sizeof(bool));
   bool* const opsel = (bool*)alloca(num_operands * sizeof(bool));
   bool* const f2f32 = (bool*)alloca(num_operands * sizeof(bool));
   for (unsigned i = 0; i < num_operands; ++i) {
      abs[i] = false;
   neg[i] = false;
   opsel[i] = false;
      }
   bool is_mad_mix = instr->opcode == aco_opcode::v_fma_mix_f32 ||
               if (instr->isVALU() && !instr->isVOP3P()) {
      const VALU_instruction& valu = instr->valu();
   for (unsigned i = 0; i < MIN2(num_operands, 3); ++i) {
      abs[i] = valu.abs[i];
   neg[i] = valu.neg[i];
         } else if (instr->isVOP3P() && is_mad_mix) {
      const VALU_instruction& vop3p = instr->valu();
   for (unsigned i = 0; i < MIN2(num_operands, 3); ++i) {
      abs[i] = vop3p.neg_hi[i];
   neg[i] = vop3p.neg_lo[i];
   f2f32[i] = vop3p.opsel_hi[i];
         }
   for (unsigned i = 0; i < num_operands; ++i) {
      if (i)
                        if (neg[i])
         if (abs[i])
         if (opsel[i])
         else if (f2f32[i])
         aco_print_operand(&instr->operands[i], output, flags);
   if (f2f32[i] || opsel[i])
                        if (instr->isVOP3P() && !is_mad_mix) {
      const VALU_instruction& vop3 = instr->valu();
   if (vop3.opsel_lo[i] || !vop3.opsel_hi[i]) {
         }
   if (vop3.neg_lo[i] && vop3.neg_hi[i])
         else if (vop3.neg_lo[i])
         else if (vop3.neg_hi[i])
            }
      }
      static void
   print_block_kind(uint16_t kind, FILE* output)
   {
      if (kind & block_kind_uniform)
         if (kind & block_kind_top_level)
         if (kind & block_kind_loop_preheader)
         if (kind & block_kind_loop_header)
         if (kind & block_kind_loop_exit)
         if (kind & block_kind_continue)
         if (kind & block_kind_break)
         if (kind & block_kind_continue_or_break)
         if (kind & block_kind_branch)
         if (kind & block_kind_merge)
         if (kind & block_kind_invert)
         if (kind & block_kind_uses_discard)
         if (kind & block_kind_export_end)
      }
      static void
   print_stage(Stage stage, FILE* output)
   {
               u_foreach_bit (s, (uint32_t)stage.sw) {
      switch ((SWStage)(1 << s)) {
   case SWStage::VS: fprintf(output, "VS"); break;
   case SWStage::GS: fprintf(output, "GS"); break;
   case SWStage::TCS: fprintf(output, "TCS"); break;
   case SWStage::TES: fprintf(output, "TES"); break;
   case SWStage::FS: fprintf(output, "FS"); break;
   case SWStage::CS: fprintf(output, "CS"); break;
   case SWStage::TS: fprintf(output, "TS"); break;
   case SWStage::MS: fprintf(output, "MS"); break;
   case SWStage::RT: fprintf(output, "RT"); break;
   default: unreachable("invalid SW stage");
   }
   if (stage.num_sw_stages() > 1)
                        switch (stage.hw) {
   case AC_HW_LOCAL_SHADER: fprintf(output, "LOCAL_SHADER"); break;
   case AC_HW_HULL_SHADER: fprintf(output, "HULL_SHADER"); break;
   case AC_HW_EXPORT_SHADER: fprintf(output, "EXPORT_SHADER"); break;
   case AC_HW_LEGACY_GEOMETRY_SHADER: fprintf(output, "LEGACY_GEOMETRY_SHADER"); break;
   case AC_HW_VERTEX_SHADER: fprintf(output, "VERTEX_SHADER"); break;
   case AC_HW_NEXT_GEN_GEOMETRY_SHADER: fprintf(output, "NEXT_GEN_GEOMETRY_SHADER"); break;
   case AC_HW_PIXEL_SHADER: fprintf(output, "PIXEL_SHADER"); break;
   case AC_HW_COMPUTE_SHADER: fprintf(output, "COMPUTE_SHADER"); break;
   default: unreachable("invalid HW stage");
               }
      void
   aco_print_block(enum amd_gfx_level gfx_level, const Block* block, FILE* output, unsigned flags,
         {
      fprintf(output, "BB%d\n", block->index);
   fprintf(output, "/* logical preds: ");
   for (unsigned pred : block->logical_preds)
         fprintf(output, "/ linear preds: ");
   for (unsigned pred : block->linear_preds)
         fprintf(output, "/ kind: ");
   print_block_kind(block->kind, output);
            if (flags & print_live_vars) {
      fprintf(output, "\tlive out:");
   for (unsigned id : live_vars.live_out[block->index])
                  RegisterDemand demand = block->register_demand;
               unsigned index = 0;
   for (auto const& instr : block->instructions) {
      fprintf(output, "\t");
   if (flags & print_live_vars) {
      RegisterDemand demand = live_vars.register_demand[block->index][index];
      }
   if (flags & print_perf_info)
            aco_print_instr(gfx_level, instr.get(), output, flags);
   fprintf(output, "\n");
         }
      void
   aco_print_program(const Program* program, FILE* output, const live& live_vars, unsigned flags)
   {
      switch (program->progress) {
   case CompilationProgress::after_isel: fprintf(output, "After Instruction Selection:\n"); break;
   case CompilationProgress::after_spilling:
      fprintf(output, "After Spilling:\n");
   flags |= print_kill;
      case CompilationProgress::after_ra: fprintf(output, "After RA:\n"); break;
                     for (Block const& block : program->blocks)
            if (program->constant_data.size()) {
      fprintf(output, "\n/* constant data */\n");
   for (unsigned i = 0; i < program->constant_data.size(); i += 32) {
      fprintf(output, "[%06d] ", i);
   unsigned line_size = std::min<size_t>(program->constant_data.size() - i, 32);
   for (unsigned j = 0; j < line_size; j += 4) {
      unsigned size = std::min<size_t>(program->constant_data.size() - (i + j), 4);
   uint32_t v = 0;
   memcpy(&v, &program->constant_data[i + j], size);
      }
                     }
      void
   aco_print_program(const Program* program, FILE* output, unsigned flags)
   {
         }
      } // namespace aco
