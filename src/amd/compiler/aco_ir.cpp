   /*
   * Copyright © 2020 Valve Corporation
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
      #include "aco_ir.h"
      #include "aco_builder.h"
      #include "util/u_debug.h"
      #include "c11/threads.h"
      namespace aco {
      thread_local aco::monotonic_buffer_resource* instruction_buffer = nullptr;
      uint64_t debug_flags = 0;
      static const struct debug_control aco_debug_options[] = {{"validateir", DEBUG_VALIDATE_IR},
                                                            {"validatera", DEBUG_VALIDATE_RA},
         static once_flag init_once_flag = ONCE_FLAG_INIT;
      static void
   init_once()
   {
            #ifndef NDEBUG
      /* enable some flags by default on debug builds */
      #endif
         if (debug_flags & aco::DEBUG_NO_VALIDATE_IR)
      }
      void
   init()
   {
         }
      void
   init_program(Program* program, Stage stage, const struct aco_shader_info* info,
               {
      instruction_buffer = &program->m;
   program->stage = stage;
   program->config = config;
   program->info = *info;
   program->gfx_level = gfx_level;
   if (family == CHIP_UNKNOWN) {
      switch (gfx_level) {
   case GFX6: program->family = CHIP_TAHITI; break;
   case GFX7: program->family = CHIP_BONAIRE; break;
   case GFX8: program->family = CHIP_POLARIS10; break;
   case GFX9: program->family = CHIP_VEGA10; break;
   case GFX10: program->family = CHIP_NAVI10; break;
   case GFX10_3: program->family = CHIP_NAVI21; break;
   case GFX11: program->family = CHIP_NAVI31; break;
   default: program->family = CHIP_UNKNOWN; break;
      } else {
         }
   program->wave_size = info->wave_size;
            program->dev.lds_encoding_granule = gfx_level >= GFX11 && stage == fragment_fs ? 1024
                        /* GFX6: There is 64KB LDS per CU, but a single workgroup can only use 32KB. */
            /* apparently gfx702 also has 16-bank LDS but I can't find a family for that */
            program->dev.vgpr_limit = 256;
   program->dev.physical_vgprs = 256;
            if (gfx_level >= GFX10) {
      program->dev.physical_sgprs = 5120; /* doesn't matter as long as it's at least 128 * 40 */
   program->dev.sgpr_alloc_granule = 128;
   program->dev.sgpr_limit =
            if (family == CHIP_NAVI31 || family == CHIP_NAVI32) {
      program->dev.physical_vgprs = program->wave_size == 32 ? 1536 : 768;
      } else {
      program->dev.physical_vgprs = program->wave_size == 32 ? 1024 : 512;
   if (gfx_level >= GFX10_3)
         else
         } else if (program->gfx_level >= GFX8) {
      program->dev.physical_sgprs = 800;
   program->dev.sgpr_alloc_granule = 16;
   program->dev.sgpr_limit = 102;
   if (family == CHIP_TONGA || family == CHIP_ICELAND)
      } else {
      program->dev.physical_sgprs = 512;
   program->dev.sgpr_alloc_granule = 8;
                        program->dev.max_wave64_per_simd = 10;
   if (program->gfx_level >= GFX10_3)
         else if (program->gfx_level == GFX10)
         else if (program->family >= CHIP_POLARIS10 && program->family <= CHIP_VEGAM)
                     switch (program->family) {
   /* GFX8 APUs */
   case CHIP_CARRIZO:
   case CHIP_STONEY:
   /* GFX9 APUS */
   case CHIP_RAVEN:
   case CHIP_RAVEN2:
   case CHIP_RENOIR: program->dev.xnack_enabled = true; break;
   default: break;
            program->dev.sram_ecc_enabled = program->family == CHIP_MI100;
   /* apparently gfx702 also has fast v_fma_f32 but I can't find a family for that */
   program->dev.has_fast_fma32 = program->gfx_level >= GFX9;
   if (program->family == CHIP_TAHITI || program->family == CHIP_CARRIZO ||
      program->family == CHIP_HAWAII)
               program->dev.fused_mad_mix = program->gfx_level >= GFX10;
   if (program->family == CHIP_VEGA12 || program->family == CHIP_VEGA20 ||
      program->family == CHIP_MI100 || program->family == CHIP_MI200)
         if (program->gfx_level >= GFX11) {
      program->dev.scratch_global_offset_min = -4096;
      } else if (program->gfx_level >= GFX10 || program->gfx_level == GFX8) {
      program->dev.scratch_global_offset_min = -2048;
      } else if (program->gfx_level == GFX9) {
      /* The minimum is actually -4096, but negative offsets are broken when SADDR is used. */
   program->dev.scratch_global_offset_min = 0;
               if (program->gfx_level >= GFX11) {
      /* GFX11 can have only 1 NSA dword. The last VGPR isn't included here because it contains the
   * rest of the address.
   */
      } else if (program->gfx_level >= GFX10_3) {
      /* GFX10.3 can have up to 3 NSA dwords. */
      } else if (program->gfx_level >= GFX10) {
      /* Limit NSA instructions to 1 NSA dword on GFX10 to avoid stability issues. */
      } else {
                                    program->next_fp_mode.preserve_signed_zero_inf_nan32 = false;
   program->next_fp_mode.preserve_signed_zero_inf_nan16_64 = false;
   program->next_fp_mode.must_flush_denorms32 = false;
   program->next_fp_mode.must_flush_denorms16_64 = false;
   program->next_fp_mode.care_about_round32 = false;
   program->next_fp_mode.care_about_round16_64 = false;
   program->next_fp_mode.denorm16_64 = fp_denorm_keep;
   program->next_fp_mode.denorm32 = 0;
   program->next_fp_mode.round16_64 = fp_round_ne;
      }
      memory_sync_info
   get_sync_info(const Instruction* instr)
   {
      /* Primitive Ordered Pixel Shading barriers necessary for accesses to memory shared between
   * overlapping waves in the queue family.
   */
   if (instr->opcode == aco_opcode::p_pops_gfx9_overlapped_wave_wait_done ||
      (instr->opcode == aco_opcode::s_wait_event &&
   !(instr->sopp().imm & wait_event_imm_dont_wait_export_ready))) {
      } else if (instr->opcode == aco_opcode::p_pops_gfx9_ordered_section_done) {
                  switch (instr->format) {
   case Format::SMEM: return instr->smem().sync;
   case Format::MUBUF: return instr->mubuf().sync;
   case Format::MIMG: return instr->mimg().sync;
   case Format::MTBUF: return instr->mtbuf().sync;
   case Format::FLAT:
   case Format::GLOBAL:
   case Format::SCRATCH: return instr->flatlike().sync;
   case Format::DS: return instr->ds().sync;
   case Format::LDSDIR: return instr->ldsdir().sync;
   default: return memory_sync_info();
      }
      bool
   can_use_SDWA(amd_gfx_level gfx_level, const aco_ptr<Instruction>& instr, bool pre_ra)
   {
      if (!instr->isVALU())
            if (gfx_level < GFX8 || gfx_level >= GFX11 || instr->isDPP() || instr->isVOP3P())
            if (instr->isSDWA())
            if (instr->isVOP3()) {
      VALU_instruction& vop3 = instr->valu();
   if (instr->format == Format::VOP3)
         if (vop3.clamp && instr->isVOPC() && gfx_level != GFX8)
         if (vop3.omod && gfx_level < GFX9)
            // TODO: return true if we know we will use vcc
   if (!pre_ra && instr->definitions.size() >= 2)
            for (unsigned i = 1; i < instr->operands.size(); i++) {
      if (instr->operands[i].isLiteral())
         if (gfx_level < GFX9 && !instr->operands[i].isOfType(RegType::vgpr))
                  if (!instr->definitions.empty() && instr->definitions[0].bytes() > 4 && !instr->isVOPC())
            if (!instr->operands.empty()) {
      if (instr->operands[0].isLiteral())
         if (gfx_level < GFX9 && !instr->operands[0].isOfType(RegType::vgpr))
         if (instr->operands[0].bytes() > 4)
         if (instr->operands.size() > 1 && instr->operands[1].bytes() > 4)
               bool is_mac = instr->opcode == aco_opcode::v_mac_f32 || instr->opcode == aco_opcode::v_mac_f16 ||
            if (gfx_level != GFX8 && is_mac)
            // TODO: return true if we know we will use vcc
   if (!pre_ra && instr->isVOPC() && gfx_level == GFX8)
         if (!pre_ra && instr->operands.size() >= 3 && !is_mac)
            return instr->opcode != aco_opcode::v_madmk_f32 && instr->opcode != aco_opcode::v_madak_f32 &&
         instr->opcode != aco_opcode::v_madmk_f16 && instr->opcode != aco_opcode::v_madak_f16 &&
   instr->opcode != aco_opcode::v_fmamk_f32 && instr->opcode != aco_opcode::v_fmaak_f32 &&
   instr->opcode != aco_opcode::v_fmamk_f16 && instr->opcode != aco_opcode::v_fmaak_f16 &&
      }
      /* updates "instr" and returns the old instruction (or NULL if no update was needed) */
   aco_ptr<Instruction>
   convert_to_SDWA(amd_gfx_level gfx_level, aco_ptr<Instruction>& instr)
   {
      if (instr->isSDWA())
            aco_ptr<Instruction> tmp = std::move(instr);
   Format format = asSDWA(withoutVOP3(tmp->format));
   instr.reset(create_instruction<SDWA_instruction>(tmp->opcode, format, tmp->operands.size(),
         std::copy(tmp->operands.cbegin(), tmp->operands.cend(), instr->operands.begin());
                     if (tmp->isVOP3()) {
      VALU_instruction& vop3 = tmp->valu();
   sdwa.neg = vop3.neg;
   sdwa.abs = vop3.abs;
   sdwa.omod = vop3.omod;
               for (unsigned i = 0; i < instr->operands.size(); i++) {
      /* SDWA only uses operands 0 and 1. */
   if (i >= 2)
                                 if (instr->definitions[0].getTemp().type() == RegType::sgpr && gfx_level == GFX8)
         if (instr->definitions.size() >= 2)
         if (instr->operands.size() >= 3)
                        }
      bool
   can_use_DPP(amd_gfx_level gfx_level, const aco_ptr<Instruction>& instr, bool dpp8)
   {
               if (instr->isDPP())
            if (instr->isSDWA() || instr->isVINTERP_INREG())
            if ((instr->format == Format::VOP3 || instr->isVOP3P()) && gfx_level < GFX11)
            if ((instr->isVOPC() || instr->definitions.size() > 1) && instr->definitions.back().isFixed() &&
      instr->definitions.back().physReg() != vcc && gfx_level < GFX11)
         if (instr->operands.size() >= 3 && instr->operands[2].isFixed() &&
      instr->operands[2].isOfType(RegType::sgpr) && instr->operands[2].physReg() != vcc &&
   gfx_level < GFX11)
         if (instr->isVOP3() && gfx_level < GFX11) {
      const VALU_instruction* vop3 = &instr->valu();
   if (vop3->clamp || vop3->omod)
         if (dpp8)
               for (unsigned i = 0; i < instr->operands.size(); i++) {
      if (instr->operands[i].isLiteral())
         if (!instr->operands[i].isOfType(RegType::vgpr) && i < 2)
               /* According to LLVM, it's unsafe to combine DPP into v_cmpx. */
   if (instr->writes_exec())
            /* simpler than listing all VOP3P opcodes which do not support DPP */
   if (instr->isVOP3P()) {
      return instr->opcode == aco_opcode::v_fma_mix_f32 ||
         instr->opcode == aco_opcode::v_fma_mixlo_f16 ||
   instr->opcode == aco_opcode::v_fma_mixhi_f16 ||
               if (instr->opcode == aco_opcode::v_pk_fmac_f16)
            /* there are more cases but those all take 64-bit inputs */
   return instr->opcode != aco_opcode::v_madmk_f32 && instr->opcode != aco_opcode::v_madak_f32 &&
         instr->opcode != aco_opcode::v_madmk_f16 && instr->opcode != aco_opcode::v_madak_f16 &&
   instr->opcode != aco_opcode::v_fmamk_f32 && instr->opcode != aco_opcode::v_fmaak_f32 &&
   instr->opcode != aco_opcode::v_fmamk_f16 && instr->opcode != aco_opcode::v_fmaak_f16 &&
   instr->opcode != aco_opcode::v_readfirstlane_b32 &&
   instr->opcode != aco_opcode::v_cvt_f64_i32 &&
   instr->opcode != aco_opcode::v_cvt_f64_f32 &&
   instr->opcode != aco_opcode::v_cvt_f64_u32 && instr->opcode != aco_opcode::v_mul_lo_u32 &&
   instr->opcode != aco_opcode::v_mul_lo_i32 && instr->opcode != aco_opcode::v_mul_hi_u32 &&
   instr->opcode != aco_opcode::v_mul_hi_i32 &&
   instr->opcode != aco_opcode::v_qsad_pk_u16_u8 &&
   instr->opcode != aco_opcode::v_mqsad_pk_u16_u8 &&
   instr->opcode != aco_opcode::v_mqsad_u32_u8 &&
   instr->opcode != aco_opcode::v_mad_u64_u32 &&
   instr->opcode != aco_opcode::v_mad_i64_i32 &&
   instr->opcode != aco_opcode::v_permlane16_b32 &&
   instr->opcode != aco_opcode::v_permlanex16_b32 &&
   instr->opcode != aco_opcode::v_permlane64_b32 &&
      }
      aco_ptr<Instruction>
   convert_to_DPP(amd_gfx_level gfx_level, aco_ptr<Instruction>& instr, bool dpp8)
   {
      if (instr->isDPP())
            aco_ptr<Instruction> tmp = std::move(instr);
   Format format =
         if (dpp8)
      instr.reset(create_instruction<DPP8_instruction>(tmp->opcode, format, tmp->operands.size(),
      else
      instr.reset(create_instruction<DPP16_instruction>(tmp->opcode, format, tmp->operands.size(),
      std::copy(tmp->operands.cbegin(), tmp->operands.cend(), instr->operands.begin());
            if (dpp8) {
      DPP8_instruction* dpp = &instr->dpp8();
   dpp->lane_sel = 0xfac688; /* [0,1,2,3,4,5,6,7] */
      } else {
      DPP16_instruction* dpp = &instr->dpp16();
   dpp->dpp_ctrl = dpp_quad_perm(0, 1, 2, 3);
   dpp->row_mask = 0xf;
   dpp->bank_mask = 0xf;
               instr->valu().neg = tmp->valu().neg;
   instr->valu().abs = tmp->valu().abs;
   instr->valu().omod = tmp->valu().omod;
   instr->valu().clamp = tmp->valu().clamp;
   instr->valu().opsel = tmp->valu().opsel;
   instr->valu().opsel_lo = tmp->valu().opsel_lo;
            if ((instr->isVOPC() || instr->definitions.size() > 1) && gfx_level < GFX11)
            if (instr->operands.size() >= 3 && instr->operands[2].isOfType(RegType::sgpr) &&
      gfx_level < GFX11)
                  /* DPP16 supports input modifiers, so we might no longer need VOP3. */
   bool remove_vop3 = !dpp8 && !instr->valu().omod && !instr->valu().clamp &&
            /* VOPC/add_co/sub_co definition needs VCC without VOP3. */
   remove_vop3 &= instr->definitions.back().regClass().type() != RegType::sgpr ||
                  /* addc/subb/cndmask 3rd operand needs VCC without VOP3. */
   remove_vop3 &= instr->operands.size() < 3 || !instr->operands[2].isFixed() ||
            if (remove_vop3)
               }
      bool
   can_use_input_modifiers(amd_gfx_level gfx_level, aco_opcode op, int idx)
   {
      if (op == aco_opcode::v_mov_b32)
            if (op == aco_opcode::v_ldexp_f16 || op == aco_opcode::v_ldexp_f32 ||
      op == aco_opcode::v_ldexp_f64)
            }
      bool
   can_use_opsel(amd_gfx_level gfx_level, aco_opcode op, int idx)
   {
      /* opsel is only GFX9+ */
   if (gfx_level < GFX9)
            switch (op) {
   case aco_opcode::v_div_fixup_f16:
   case aco_opcode::v_fma_f16:
   case aco_opcode::v_mad_f16:
   case aco_opcode::v_mad_u16:
   case aco_opcode::v_mad_i16:
   case aco_opcode::v_med3_f16:
   case aco_opcode::v_med3_i16:
   case aco_opcode::v_med3_u16:
   case aco_opcode::v_min3_f16:
   case aco_opcode::v_min3_i16:
   case aco_opcode::v_min3_u16:
   case aco_opcode::v_max3_f16:
   case aco_opcode::v_max3_i16:
   case aco_opcode::v_max3_u16:
   case aco_opcode::v_minmax_f16:
   case aco_opcode::v_maxmin_f16:
   case aco_opcode::v_max_u16_e64:
   case aco_opcode::v_max_i16_e64:
   case aco_opcode::v_min_u16_e64:
   case aco_opcode::v_min_i16_e64:
   case aco_opcode::v_add_i16:
   case aco_opcode::v_sub_i16:
   case aco_opcode::v_add_u16_e64:
   case aco_opcode::v_sub_u16_e64:
   case aco_opcode::v_lshlrev_b16_e64:
   case aco_opcode::v_lshrrev_b16_e64:
   case aco_opcode::v_ashrrev_i16_e64:
   case aco_opcode::v_and_b16:
   case aco_opcode::v_or_b16:
   case aco_opcode::v_xor_b16:
   case aco_opcode::v_mul_lo_u16_e64: return true;
   case aco_opcode::v_pack_b32_f16:
   case aco_opcode::v_cvt_pknorm_i16_f16:
   case aco_opcode::v_cvt_pknorm_u16_f16: return idx != -1;
   case aco_opcode::v_mad_u32_u16:
   case aco_opcode::v_mad_i32_i16: return idx >= 0 && idx < 2;
   case aco_opcode::v_dot2_f16_f16:
   case aco_opcode::v_dot2_bf16_bf16: return idx == -1 || idx == 2;
   case aco_opcode::v_cndmask_b16: return idx != 2;
   case aco_opcode::v_interp_p10_f16_f32_inreg:
   case aco_opcode::v_interp_p10_rtz_f16_f32_inreg: return idx == 0 || idx == 2;
   case aco_opcode::v_interp_p2_f16_f32_inreg:
   case aco_opcode::v_interp_p2_rtz_f16_f32_inreg: return idx == -1 || idx == 0;
   default:
            }
      bool
   can_write_m0(const aco_ptr<Instruction>& instr)
   {
      if (instr->isSALU())
            /* VALU can't write m0 on any GPU generations. */
   if (instr->isVALU())
            switch (instr->opcode) {
   case aco_opcode::p_parallelcopy:
   case aco_opcode::p_extract:
   case aco_opcode::p_insert:
      /* These pseudo instructions are implemented with SALU when writing m0. */
      default:
      /* Assume that no other instructions can write m0. */
         }
      bool
   instr_is_16bit(amd_gfx_level gfx_level, aco_opcode op)
   {
      /* partial register writes are GFX9+, only */
   if (gfx_level < GFX9)
            switch (op) {
   /* VOP3 */
   case aco_opcode::v_mad_f16:
   case aco_opcode::v_mad_u16:
   case aco_opcode::v_mad_i16:
   case aco_opcode::v_fma_f16:
   case aco_opcode::v_div_fixup_f16:
   case aco_opcode::v_interp_p2_f16:
   case aco_opcode::v_fma_mixlo_f16:
   case aco_opcode::v_fma_mixhi_f16:
   /* VOP2 */
   case aco_opcode::v_mac_f16:
   case aco_opcode::v_madak_f16:
   case aco_opcode::v_madmk_f16: return gfx_level >= GFX9;
   case aco_opcode::v_add_f16:
   case aco_opcode::v_sub_f16:
   case aco_opcode::v_subrev_f16:
   case aco_opcode::v_mul_f16:
   case aco_opcode::v_max_f16:
   case aco_opcode::v_min_f16:
   case aco_opcode::v_ldexp_f16:
   case aco_opcode::v_fmac_f16:
   case aco_opcode::v_fmamk_f16:
   case aco_opcode::v_fmaak_f16:
   /* VOP1 */
   case aco_opcode::v_cvt_f16_f32:
   case aco_opcode::p_cvt_f16_f32_rtne:
   case aco_opcode::v_cvt_f16_u16:
   case aco_opcode::v_cvt_f16_i16:
   case aco_opcode::v_rcp_f16:
   case aco_opcode::v_sqrt_f16:
   case aco_opcode::v_rsq_f16:
   case aco_opcode::v_log_f16:
   case aco_opcode::v_exp_f16:
   case aco_opcode::v_frexp_mant_f16:
   case aco_opcode::v_frexp_exp_i16_f16:
   case aco_opcode::v_floor_f16:
   case aco_opcode::v_ceil_f16:
   case aco_opcode::v_trunc_f16:
   case aco_opcode::v_rndne_f16:
   case aco_opcode::v_fract_f16:
   case aco_opcode::v_sin_f16:
   case aco_opcode::v_cos_f16:
   case aco_opcode::v_cvt_u16_f16:
   case aco_opcode::v_cvt_i16_f16:
   case aco_opcode::v_cvt_norm_i16_f16:
   case aco_opcode::v_cvt_norm_u16_f16: return gfx_level >= GFX10;
   /* on GFX10, all opsel instructions preserve the high bits */
   default: return gfx_level >= GFX10 && can_use_opsel(gfx_level, op, -1);
      }
      /* On GFX11, for some instructions, bit 7 of the destination/operand vgpr is opsel and the field
   * only supports v0-v127.
   * The first three bits are used for operands 0-2, and the 4th bit is used for the destination.
   */
   uint8_t
   get_gfx11_true16_mask(aco_opcode op)
   {
      switch (op) {
   case aco_opcode::v_ceil_f16:
   case aco_opcode::v_cos_f16:
   case aco_opcode::v_cvt_f16_i16:
   case aco_opcode::v_cvt_f16_u16:
   case aco_opcode::v_cvt_i16_f16:
   case aco_opcode::v_cvt_u16_f16:
   case aco_opcode::v_cvt_norm_i16_f16:
   case aco_opcode::v_cvt_norm_u16_f16:
   case aco_opcode::v_exp_f16:
   case aco_opcode::v_floor_f16:
   case aco_opcode::v_fract_f16:
   case aco_opcode::v_frexp_exp_i16_f16:
   case aco_opcode::v_frexp_mant_f16:
   case aco_opcode::v_log_f16:
   case aco_opcode::v_not_b16:
   case aco_opcode::v_rcp_f16:
   case aco_opcode::v_rndne_f16:
   case aco_opcode::v_rsq_f16:
   case aco_opcode::v_sin_f16:
   case aco_opcode::v_sqrt_f16:
   case aco_opcode::v_trunc_f16:
   case aco_opcode::v_mov_b16: return 0x1 | 0x8;
   case aco_opcode::v_add_f16:
   case aco_opcode::v_fmaak_f16:
   case aco_opcode::v_fmac_f16:
   case aco_opcode::v_fmamk_f16:
   case aco_opcode::v_ldexp_f16:
   case aco_opcode::v_max_f16:
   case aco_opcode::v_min_f16:
   case aco_opcode::v_mul_f16:
   case aco_opcode::v_sub_f16:
   case aco_opcode::v_subrev_f16:
   case aco_opcode::v_and_b16:
   case aco_opcode::v_or_b16:
   case aco_opcode::v_xor_b16: return 0x3 | 0x8;
   case aco_opcode::v_cvt_f32_f16:
   case aco_opcode::v_cvt_i32_i16:
   case aco_opcode::v_cvt_u32_u16: return 0x1;
   case aco_opcode::v_cmp_class_f16:
   case aco_opcode::v_cmp_eq_f16:
   case aco_opcode::v_cmp_eq_i16:
   case aco_opcode::v_cmp_eq_u16:
   case aco_opcode::v_cmp_ge_f16:
   case aco_opcode::v_cmp_ge_i16:
   case aco_opcode::v_cmp_ge_u16:
   case aco_opcode::v_cmp_gt_f16:
   case aco_opcode::v_cmp_gt_i16:
   case aco_opcode::v_cmp_gt_u16:
   case aco_opcode::v_cmp_le_f16:
   case aco_opcode::v_cmp_le_i16:
   case aco_opcode::v_cmp_le_u16:
   case aco_opcode::v_cmp_lg_f16:
   case aco_opcode::v_cmp_lg_i16:
   case aco_opcode::v_cmp_lg_u16:
   case aco_opcode::v_cmp_lt_f16:
   case aco_opcode::v_cmp_lt_i16:
   case aco_opcode::v_cmp_lt_u16:
   case aco_opcode::v_cmp_neq_f16:
   case aco_opcode::v_cmp_nge_f16:
   case aco_opcode::v_cmp_ngt_f16:
   case aco_opcode::v_cmp_nle_f16:
   case aco_opcode::v_cmp_nlg_f16:
   case aco_opcode::v_cmp_nlt_f16:
   case aco_opcode::v_cmp_o_f16:
   case aco_opcode::v_cmp_u_f16:
   case aco_opcode::v_cmpx_class_f16:
   case aco_opcode::v_cmpx_eq_f16:
   case aco_opcode::v_cmpx_eq_i16:
   case aco_opcode::v_cmpx_eq_u16:
   case aco_opcode::v_cmpx_ge_f16:
   case aco_opcode::v_cmpx_ge_i16:
   case aco_opcode::v_cmpx_ge_u16:
   case aco_opcode::v_cmpx_gt_f16:
   case aco_opcode::v_cmpx_gt_i16:
   case aco_opcode::v_cmpx_gt_u16:
   case aco_opcode::v_cmpx_le_f16:
   case aco_opcode::v_cmpx_le_i16:
   case aco_opcode::v_cmpx_le_u16:
   case aco_opcode::v_cmpx_lg_f16:
   case aco_opcode::v_cmpx_lg_i16:
   case aco_opcode::v_cmpx_lg_u16:
   case aco_opcode::v_cmpx_lt_f16:
   case aco_opcode::v_cmpx_lt_i16:
   case aco_opcode::v_cmpx_lt_u16:
   case aco_opcode::v_cmpx_neq_f16:
   case aco_opcode::v_cmpx_nge_f16:
   case aco_opcode::v_cmpx_ngt_f16:
   case aco_opcode::v_cmpx_nle_f16:
   case aco_opcode::v_cmpx_nlg_f16:
   case aco_opcode::v_cmpx_nlt_f16:
   case aco_opcode::v_cmpx_o_f16:
   case aco_opcode::v_cmpx_u_f16: return 0x3;
   case aco_opcode::v_cvt_f16_f32:
   case aco_opcode::v_sat_pk_u8_i16: return 0x8;
   default: return 0x0;
      }
      uint32_t
   get_reduction_identity(ReduceOp op, unsigned idx)
   {
      switch (op) {
   case iadd8:
   case iadd16:
   case iadd32:
   case iadd64:
   case fadd16:
   case fadd32:
   case fadd64:
   case ior8:
   case ior16:
   case ior32:
   case ior64:
   case ixor8:
   case ixor16:
   case ixor32:
   case ixor64:
   case umax8:
   case umax16:
   case umax32:
   case umax64: return 0;
   case imul8:
   case imul16:
   case imul32:
   case imul64: return idx ? 0 : 1;
   case fmul16: return 0x3c00u;                /* 1.0 */
   case fmul32: return 0x3f800000u;            /* 1.0 */
   case fmul64: return idx ? 0x3ff00000u : 0u; /* 1.0 */
   case imin8: return INT8_MAX;
   case imin16: return INT16_MAX;
   case imin32: return INT32_MAX;
   case imin64: return idx ? 0x7fffffffu : 0xffffffffu;
   case imax8: return INT8_MIN;
   case imax16: return INT16_MIN;
   case imax32: return INT32_MIN;
   case imax64: return idx ? 0x80000000u : 0;
   case umin8:
   case umin16:
   case iand8:
   case iand16: return 0xffffffffu;
   case umin32:
   case umin64:
   case iand32:
   case iand64: return 0xffffffffu;
   case fmin16: return 0x7c00u;                /* infinity */
   case fmin32: return 0x7f800000u;            /* infinity */
   case fmin64: return idx ? 0x7ff00000u : 0u; /* infinity */
   case fmax16: return 0xfc00u;                /* negative infinity */
   case fmax32: return 0xff800000u;            /* negative infinity */
   case fmax64: return idx ? 0xfff00000u : 0u; /* negative infinity */
   default: unreachable("Invalid reduction operation"); break;
   }
      }
      unsigned
   get_operand_size(aco_ptr<Instruction>& instr, unsigned index)
   {
      if (instr->isPseudo())
         else if (instr->opcode == aco_opcode::v_mad_u64_u32 ||
               else if (instr->opcode == aco_opcode::v_fma_mix_f32 ||
            instr->opcode == aco_opcode::v_fma_mixlo_f16 ||
      else if (instr->isVALU() || instr->isSALU())
         else
      }
      bool
   needs_exec_mask(const Instruction* instr)
   {
      if (instr->isVALU()) {
      return instr->opcode != aco_opcode::v_readlane_b32 &&
         instr->opcode != aco_opcode::v_readlane_b32_e64 &&
               if (instr->isVMEM() || instr->isFlatLike())
            if (instr->isSALU() || instr->isBranch() || instr->isSMEM() || instr->isBarrier())
            if (instr->isPseudo()) {
      switch (instr->opcode) {
   case aco_opcode::p_create_vector:
   case aco_opcode::p_extract_vector:
   case aco_opcode::p_split_vector:
   case aco_opcode::p_phi:
   case aco_opcode::p_parallelcopy:
      for (Definition def : instr->definitions) {
      if (def.getTemp().type() == RegType::vgpr)
      }
      case aco_opcode::p_spill:
   case aco_opcode::p_reload:
   case aco_opcode::p_end_linear_vgpr:
   case aco_opcode::p_logical_start:
   case aco_opcode::p_logical_end:
   case aco_opcode::p_startpgm:
   case aco_opcode::p_end_wqm:
   case aco_opcode::p_init_scratch: return instr->reads_exec();
   case aco_opcode::p_start_linear_vgpr: return instr->operands.size();
   default: break;
                  }
      struct CmpInfo {
      aco_opcode ordered;
   aco_opcode unordered;
   aco_opcode swapped;
   aco_opcode inverse;
   aco_opcode vcmpx;
   aco_opcode f32;
      };
      ALWAYS_INLINE bool
   get_cmp_info(aco_opcode op, CmpInfo* info)
   {
      info->ordered = aco_opcode::num_opcodes;
   info->unordered = aco_opcode::num_opcodes;
   info->swapped = aco_opcode::num_opcodes;
   info->inverse = aco_opcode::num_opcodes;
   info->f32 = aco_opcode::num_opcodes;
   info->vcmpx = aco_opcode::num_opcodes;
   switch (op) {
      #define CMP2(ord, unord, ord_swap, unord_swap, sz)                                                 \
      case aco_opcode::v_cmp_##ord##_f##sz:                                                           \
   case aco_opcode::v_cmp_n##unord##_f##sz:                                                        \
      info->ordered = aco_opcode::v_cmp_##ord##_f##sz;                                             \
   info->unordered = aco_opcode::v_cmp_n##unord##_f##sz;                                        \
   info->swapped = op == aco_opcode::v_cmp_##ord##_f##sz ? aco_opcode::v_cmp_##ord_swap##_f##sz \
         info->inverse = op == aco_opcode::v_cmp_n##unord##_f##sz ? aco_opcode::v_cmp_##unord##_f##sz \
         info->f32 = op == aco_opcode::v_cmp_##ord##_f##sz ? aco_opcode::v_cmp_##ord##_f32            \
         info->vcmpx = op == aco_opcode::v_cmp_##ord##_f##sz ? aco_opcode::v_cmpx_##ord##_f##sz       \
         info->size = sz;                                                                             \
   #define CMP(ord, unord, ord_swap, unord_swap)                                                      \
      CMP2(ord, unord, ord_swap, unord_swap, 16)                                                      \
   CMP2(ord, unord, ord_swap, unord_swap, 32)                                                      \
   CMP2(ord, unord, ord_swap, unord_swap, 64)
      CMP(lt, /*n*/ge, gt, /*n*/le)
   CMP(eq, /*n*/lg, eq, /*n*/lg)
   CMP(le, /*n*/gt, ge, /*n*/lt)
   CMP(gt, /*n*/le, lt, /*n*/ge)
   CMP(lg, /*n*/eq, lg, /*n*/eq)
   #undef CMP
   #undef CMP2
   #define ORD_TEST(sz)                                                                               \
      case aco_opcode::v_cmp_u_f##sz:                                                                 \
      info->f32 = aco_opcode::v_cmp_u_f32;                                                         \
   info->swapped = aco_opcode::v_cmp_u_f##sz;                                                   \
   info->inverse = aco_opcode::v_cmp_o_f##sz;                                                   \
   info->vcmpx = aco_opcode::v_cmpx_u_f##sz;                                                    \
   info->size = sz;                                                                             \
      case aco_opcode::v_cmp_o_f##sz:                                                                 \
      info->f32 = aco_opcode::v_cmp_o_f32;                                                         \
   info->swapped = aco_opcode::v_cmp_o_f##sz;                                                   \
   info->inverse = aco_opcode::v_cmp_u_f##sz;                                                   \
   info->vcmpx = aco_opcode::v_cmpx_o_f##sz;                                                    \
   info->size = sz;                                                                             \
   return true;
   ORD_TEST(16)
   ORD_TEST(32)
   #undef ORD_TEST
   #define CMPI2(op, swap, inv, type, sz)                                                             \
      case aco_opcode::v_cmp_##op##_##type##sz:                                                       \
      info->swapped = aco_opcode::v_cmp_##swap##_##type##sz;                                       \
   info->inverse = aco_opcode::v_cmp_##inv##_##type##sz;                                        \
   info->vcmpx = aco_opcode::v_cmpx_##op##_##type##sz;                                          \
   info->size = sz;                                                                             \
   #define CMPI(op, swap, inv)                                                                        \
      CMPI2(op, swap, inv, i, 16)                                                                     \
   CMPI2(op, swap, inv, u, 16)                                                                     \
   CMPI2(op, swap, inv, i, 32)                                                                     \
   CMPI2(op, swap, inv, u, 32)                                                                     \
   CMPI2(op, swap, inv, i, 64)                                                                     \
   CMPI2(op, swap, inv, u, 64)
      CMPI(lt, gt, ge)
   CMPI(eq, eq, lg)
   CMPI(le, ge, gt)
   CMPI(gt, lt, le)
   CMPI(lg, lg, eq)
   #undef CMPI
   #undef CMPI2
   #define CMPCLASS(sz)                                                                               \
      case aco_opcode::v_cmp_class_f##sz:                                                             \
      info->vcmpx = aco_opcode::v_cmpx_class_f##sz;                                                \
   info->size = sz;                                                                             \
   return true;
   CMPCLASS(16)
   CMPCLASS(32)
   #undef CMPCLASS
            default: return false;
      }
      aco_opcode
   get_ordered(aco_opcode op)
   {
      CmpInfo info;
      }
      aco_opcode
   get_unordered(aco_opcode op)
   {
      CmpInfo info;
      }
      aco_opcode
   get_inverse(aco_opcode op)
   {
      CmpInfo info;
      }
      aco_opcode
   get_swapped(aco_opcode op)
   {
      CmpInfo info;
      }
      aco_opcode
   get_f32_cmp(aco_opcode op)
   {
      CmpInfo info;
      }
      aco_opcode
   get_vcmpx(aco_opcode op)
   {
      CmpInfo info;
      }
      unsigned
   get_cmp_bitsize(aco_opcode op)
   {
      CmpInfo info;
      }
      bool
   is_fp_cmp(aco_opcode op)
   {
      CmpInfo info;
      }
      bool
   is_cmpx(aco_opcode op)
   {
      CmpInfo info;
      }
      bool
   can_swap_operands(aco_ptr<Instruction>& instr, aco_opcode* new_op, unsigned idx0, unsigned idx1)
   {
      if (idx0 == idx1) {
      *new_op = instr->opcode;
               if (idx0 > idx1)
            if (instr->isDPP())
            if (!instr->isVOP3() && !instr->isVOP3P() && !instr->operands[0].isOfType(RegType::vgpr))
            if (instr->isVOPC()) {
      CmpInfo info;
   if (get_cmp_info(instr->opcode, &info) && info.swapped != aco_opcode::num_opcodes) {
      *new_op = info.swapped;
                  /* opcodes not relevant for DPP or SGPRs optimizations are not included. */
   switch (instr->opcode) {
   case aco_opcode::v_med3_f32: return false; /* order matters for clamp+GFX8+denorm ftz. */
   case aco_opcode::v_add_u32:
   case aco_opcode::v_add_co_u32:
   case aco_opcode::v_add_co_u32_e64:
   case aco_opcode::v_add_i32:
   case aco_opcode::v_add_i16:
   case aco_opcode::v_add_u16_e64:
   case aco_opcode::v_add3_u32:
   case aco_opcode::v_add_f16:
   case aco_opcode::v_add_f32:
   case aco_opcode::v_mul_i32_i24:
   case aco_opcode::v_mul_hi_i32_i24:
   case aco_opcode::v_mul_u32_u24:
   case aco_opcode::v_mul_hi_u32_u24:
   case aco_opcode::v_mul_lo_u16:
   case aco_opcode::v_mul_lo_u16_e64:
   case aco_opcode::v_mul_f16:
   case aco_opcode::v_mul_f32:
   case aco_opcode::v_mul_legacy_f32:
   case aco_opcode::v_or_b32:
   case aco_opcode::v_and_b32:
   case aco_opcode::v_xor_b32:
   case aco_opcode::v_xnor_b32:
   case aco_opcode::v_xor3_b32:
   case aco_opcode::v_or3_b32:
   case aco_opcode::v_and_b16:
   case aco_opcode::v_or_b16:
   case aco_opcode::v_xor_b16:
   case aco_opcode::v_max3_f32:
   case aco_opcode::v_min3_f32:
   case aco_opcode::v_max3_f16:
   case aco_opcode::v_min3_f16:
   case aco_opcode::v_med3_f16:
   case aco_opcode::v_max3_u32:
   case aco_opcode::v_min3_u32:
   case aco_opcode::v_med3_u32:
   case aco_opcode::v_max3_i32:
   case aco_opcode::v_min3_i32:
   case aco_opcode::v_med3_i32:
   case aco_opcode::v_max3_u16:
   case aco_opcode::v_min3_u16:
   case aco_opcode::v_med3_u16:
   case aco_opcode::v_max3_i16:
   case aco_opcode::v_min3_i16:
   case aco_opcode::v_med3_i16:
   case aco_opcode::v_max_f16:
   case aco_opcode::v_max_f32:
   case aco_opcode::v_min_f16:
   case aco_opcode::v_min_f32:
   case aco_opcode::v_max_i32:
   case aco_opcode::v_min_i32:
   case aco_opcode::v_max_u32:
   case aco_opcode::v_min_u32:
   case aco_opcode::v_max_i16:
   case aco_opcode::v_min_i16:
   case aco_opcode::v_max_u16:
   case aco_opcode::v_min_u16:
   case aco_opcode::v_max_i16_e64:
   case aco_opcode::v_min_i16_e64:
   case aco_opcode::v_max_u16_e64:
   case aco_opcode::v_min_u16_e64: *new_op = instr->opcode; return true;
   case aco_opcode::v_sub_f16: *new_op = aco_opcode::v_subrev_f16; return true;
   case aco_opcode::v_sub_f32: *new_op = aco_opcode::v_subrev_f32; return true;
   case aco_opcode::v_sub_co_u32: *new_op = aco_opcode::v_subrev_co_u32; return true;
   case aco_opcode::v_sub_u16: *new_op = aco_opcode::v_subrev_u16; return true;
   case aco_opcode::v_sub_u32: *new_op = aco_opcode::v_subrev_u32; return true;
   case aco_opcode::v_sub_co_u32_e64: *new_op = aco_opcode::v_subrev_co_u32_e64; return true;
   case aco_opcode::v_subrev_f16: *new_op = aco_opcode::v_sub_f16; return true;
   case aco_opcode::v_subrev_f32: *new_op = aco_opcode::v_sub_f32; return true;
   case aco_opcode::v_subrev_co_u32: *new_op = aco_opcode::v_sub_co_u32; return true;
   case aco_opcode::v_subrev_u16: *new_op = aco_opcode::v_sub_u16; return true;
   case aco_opcode::v_subrev_u32: *new_op = aco_opcode::v_sub_u32; return true;
   case aco_opcode::v_subrev_co_u32_e64: *new_op = aco_opcode::v_sub_co_u32_e64; return true;
   case aco_opcode::v_addc_co_u32:
   case aco_opcode::v_mad_i32_i24:
   case aco_opcode::v_mad_u32_u24:
   case aco_opcode::v_lerp_u8:
   case aco_opcode::v_sad_u8:
   case aco_opcode::v_sad_hi_u8:
   case aco_opcode::v_sad_u16:
   case aco_opcode::v_sad_u32:
   case aco_opcode::v_xad_u32:
   case aco_opcode::v_add_lshl_u32:
   case aco_opcode::v_and_or_b32:
   case aco_opcode::v_mad_u16:
   case aco_opcode::v_mad_i16:
   case aco_opcode::v_mad_u32_u16:
   case aco_opcode::v_mad_i32_i16:
   case aco_opcode::v_maxmin_f32:
   case aco_opcode::v_minmax_f32:
   case aco_opcode::v_maxmin_f16:
   case aco_opcode::v_minmax_f16:
   case aco_opcode::v_maxmin_u32:
   case aco_opcode::v_minmax_u32:
   case aco_opcode::v_maxmin_i32:
   case aco_opcode::v_minmax_i32:
   case aco_opcode::v_fma_f32:
   case aco_opcode::v_fma_legacy_f32:
   case aco_opcode::v_fmac_f32:
   case aco_opcode::v_fmac_legacy_f32:
   case aco_opcode::v_mac_f32:
   case aco_opcode::v_mac_legacy_f32:
   case aco_opcode::v_fma_f16:
   case aco_opcode::v_fmac_f16:
   case aco_opcode::v_mac_f16:
   case aco_opcode::v_dot4c_i32_i8:
   case aco_opcode::v_dot2c_f32_f16:
   case aco_opcode::v_dot2_f32_f16:
   case aco_opcode::v_dot2_f32_bf16:
   case aco_opcode::v_dot2_f16_f16:
   case aco_opcode::v_dot2_bf16_bf16:
   case aco_opcode::v_fma_mix_f32:
   case aco_opcode::v_fma_mixlo_f16:
   case aco_opcode::v_fma_mixhi_f16:
   case aco_opcode::v_pk_fmac_f16: {
      if (idx1 == 2)
         *new_op = instr->opcode;
      }
   case aco_opcode::v_subb_co_u32: {
      if (idx1 == 2)
         *new_op = aco_opcode::v_subbrev_co_u32;
      }
   case aco_opcode::v_subbrev_co_u32: {
      if (idx1 == 2)
         *new_op = aco_opcode::v_subb_co_u32;
      }
   default: return false;
      }
      wait_imm::wait_imm() : vm(unset_counter), exp(unset_counter), lgkm(unset_counter), vs(unset_counter)
   {}
   wait_imm::wait_imm(uint16_t vm_, uint16_t exp_, uint16_t lgkm_, uint16_t vs_)
         {}
      wait_imm::wait_imm(enum amd_gfx_level gfx_level, uint16_t packed) : vs(unset_counter)
   {
      if (gfx_level >= GFX11) {
      vm = (packed >> 10) & 0x3f;
   lgkm = (packed >> 4) & 0x3f;
      } else {
      vm = packed & 0xf;
   if (gfx_level >= GFX9)
                     lgkm = (packed >> 8) & 0xf;
   if (gfx_level >= GFX10)
               if (vm == (gfx_level >= GFX9 ? 0x3f : 0xf))
         if (exp == 0x7)
         if (lgkm == (gfx_level >= GFX10 ? 0x3f : 0xf))
      }
      uint16_t
   wait_imm::pack(enum amd_gfx_level gfx_level) const
   {
      uint16_t imm = 0;
   assert(exp == unset_counter || exp <= 0x7);
   if (gfx_level >= GFX11) {
      assert(lgkm == unset_counter || lgkm <= 0x3f);
   assert(vm == unset_counter || vm <= 0x3f);
      } else if (gfx_level >= GFX10) {
      assert(lgkm == unset_counter || lgkm <= 0x3f);
   assert(vm == unset_counter || vm <= 0x3f);
      } else if (gfx_level >= GFX9) {
      assert(lgkm == unset_counter || lgkm <= 0xf);
   assert(vm == unset_counter || vm <= 0x3f);
      } else {
      assert(lgkm == unset_counter || lgkm <= 0xf);
   assert(vm == unset_counter || vm <= 0xf);
      }
   if (gfx_level < GFX9 && vm == wait_imm::unset_counter)
      imm |= 0xc000; /* should have no effect on pre-GFX9 and now we won't have to worry about the
      if (gfx_level < GFX10 && lgkm == wait_imm::unset_counter)
      imm |= 0x3000; /* should have no effect on pre-GFX10 and now we won't have to worry about the
         }
      bool
   wait_imm::combine(const wait_imm& other)
   {
      bool changed = other.vm < vm || other.exp < exp || other.lgkm < lgkm || other.vs < vs;
   vm = std::min(vm, other.vm);
   exp = std::min(exp, other.exp);
   lgkm = std::min(lgkm, other.lgkm);
   vs = std::min(vs, other.vs);
      }
      bool
   wait_imm::empty() const
   {
      return vm == unset_counter && exp == unset_counter && lgkm == unset_counter &&
      }
      void
   wait_imm::print(FILE* output) const
   {
      if (exp != unset_counter)
         if (vm != unset_counter)
         if (lgkm != unset_counter)
         if (vs != unset_counter)
      }
      bool
   should_form_clause(const Instruction* a, const Instruction* b)
   {
      if (a->definitions.empty() != b->definitions.empty())
            if (a->format != b->format)
            /* Assume loads which don't use descriptors might load from similar addresses. */
   if (a->isFlatLike())
         if (a->isSMEM() && a->operands[0].bytes() == 8 && b->operands[0].bytes() == 8)
            /* If they load from the same descriptor, assume they might load from similar
   * addresses.
   */
   if (a->isVMEM() || a->isSMEM())
               }
      int
   get_op_fixed_to_def(Instruction* instr)
   {
      if (instr->opcode == aco_opcode::v_interp_p2_f32 || instr->opcode == aco_opcode::v_mac_f32 ||
      instr->opcode == aco_opcode::v_fmac_f32 || instr->opcode == aco_opcode::v_mac_f16 ||
   instr->opcode == aco_opcode::v_fmac_f16 || instr->opcode == aco_opcode::v_mac_legacy_f32 ||
   instr->opcode == aco_opcode::v_fmac_legacy_f32 ||
   instr->opcode == aco_opcode::v_pk_fmac_f16 || instr->opcode == aco_opcode::v_writelane_b32 ||
   instr->opcode == aco_opcode::v_writelane_b32_e64 ||
   instr->opcode == aco_opcode::v_dot4c_i32_i8) {
      } else if (instr->opcode == aco_opcode::s_addk_i32 || instr->opcode == aco_opcode::s_mulk_i32 ||
               } else if (instr->isMUBUF() && instr->definitions.size() == 1 && instr->operands.size() == 4) {
         } else if (instr->isMIMG() && instr->definitions.size() == 1 &&
               }
      }
      bool
   dealloc_vgprs(Program* program)
   {
      if (program->gfx_level < GFX11)
            /* skip if deallocating VGPRs won't increase occupancy */
   uint16_t max_waves = program->dev.max_wave64_per_simd * (64 / program->wave_size);
   max_waves = max_suitable_waves(program, max_waves);
   if (program->max_reg_demand.vgpr <= get_addr_vgpr_from_waves(program, max_waves))
            /* sendmsg(dealloc_vgprs) releases scratch, so this isn't safe if there is a in-progress scratch
   * store. */
   if (uses_scratch(program))
                     /* don't bother checking if there is a pending VMEM store or export: there almost always is */
   Builder bld(program);
   if (!block.instructions.empty() && block.instructions.back()->opcode == aco_opcode::s_endpgm) {
      bld.reset(&block.instructions, block.instructions.begin() + (block.instructions.size() - 1));
   /* Due to a hazard, an s_nop is needed before "s_sendmsg sendmsg_dealloc_vgprs". */
   bld.sopp(aco_opcode::s_nop, -1, 0);
                  }
      bool
   Instruction::isTrans() const noexcept
   {
      return instr_info.classes[(int)opcode] == instr_class::valu_transcendental32 ||
      }
      } // namespace aco
