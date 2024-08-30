   #include <strings.h>
   #include "pipe/p_context.h"
   #include "pipe/p_defines.h"
   #include "pipe/p_state.h"
   #include "util/compiler.h"
   #include "util/u_dynarray.h"
   #include "util/u_debug.h"
   #include "util/u_memory.h"
      #include "pipe/p_shader_tokens.h"
   #include "tgsi/tgsi_parse.h"
   #include "tgsi/tgsi_dump.h"
      #include "draw/draw_context.h"
      #include "nv_object.xml.h"
   #include "nouveau_debug.h"
   #include "nv30/nv30-40_3d.xml.h"
   #include "nv30/nv30_state.h"
      /* TODO (at least...):
   *  1. Indexed consts  + ARL
   *  3. NV_vp11, NV_vp2, NV_vp3 features
   *       - extra arith opcodes
   *       - branching
   *       - texture sampling
   *       - indexed attribs
   *       - indexed results
   *  4. bugs
   */
      #include "nv30/nv30_vertprog.h"
   #include "nv30/nv40_vertprog.h"
      struct nvfx_loop_entry {
      unsigned brk_target;
      };
      struct nvfx_vpc {
      struct pipe_shader_state pipe;
   struct nv30_vertprog *vp;
                     unsigned r_temps;
   unsigned r_temps_discard;
   struct nvfx_reg r_result[PIPE_MAX_SHADER_OUTPUTS];
   struct nvfx_reg *r_address;
   struct nvfx_reg *r_temp;
   struct nvfx_reg *r_const;
            struct nvfx_reg *imm;
            int hpos_idx;
                     struct util_dynarray label_relocs;
      };
      static struct nvfx_reg
   temp(struct nvfx_vpc *vpc)
   {
               if (idx < 0 || (!vpc->is_nv4x && idx >= 16)) {
      NOUVEAU_ERR("out of temps!!\n");
               vpc->r_temps |= (1 << idx);
   vpc->r_temps_discard |= (1 << idx);
      }
      static inline void
   release_temps(struct nvfx_vpc *vpc)
   {
      vpc->r_temps &= ~vpc->r_temps_discard;
      }
      static struct nvfx_reg
   constant(struct nvfx_vpc *vpc, int pipe, float x, float y, float z, float w)
   {
      struct nv30_vertprog *vp = vpc->vp;
   struct nv30_vertprog_data *vpd;
            if (pipe >= 0) {
      for (idx = 0; idx < vp->nr_consts; idx++) {
      if (vp->consts[idx].index == pipe)
                  idx = vp->nr_consts++;
   vp->consts = realloc(vp->consts, sizeof(*vpd) * vp->nr_consts);
            vpd->index = pipe;
   vpd->value[0] = x;
   vpd->value[1] = y;
   vpd->value[2] = z;
   vpd->value[3] = w;
      }
      #define arith(s,t,o,d,m,s0,s1,s2) \
            static void
   emit_src(struct nvfx_vpc *vpc, uint32_t *hw,
         {
      struct nv30_vertprog *vp = vpc->vp;
   uint32_t sr = 0;
            switch (src.reg.type) {
   case NVFXSR_TEMP:
      sr |= (NVFX_VP(SRC_REG_TYPE_TEMP) << NVFX_VP(SRC_REG_TYPE_SHIFT));
   sr |= (src.reg.index << NVFX_VP(SRC_TEMP_SRC_SHIFT));
      case NVFXSR_INPUT:
      sr |= (NVFX_VP(SRC_REG_TYPE_INPUT) <<
         vp->ir |= (1 << src.reg.index);
   hw[1] |= (src.reg.index << NVFX_VP(INST_INPUT_SRC_SHIFT));
      case NVFXSR_CONST:
      sr |= (NVFX_VP(SRC_REG_TYPE_CONST) <<
         if (src.reg.index < 256 && src.reg.index >= -256) {
      reloc.location = vp->nr_insns - 1;
   reloc.target = src.reg.index;
      } else {
      hw[1] |= (src.reg.index << NVFX_VP(INST_CONST_SRC_SHIFT)) &
      }
      case NVFXSR_NONE:
      sr |= (NVFX_VP(SRC_REG_TYPE_INPUT) <<
            default:
                  if (src.negate)
            if (src.abs)
            sr |= ((src.swz[0] << NVFX_VP(SRC_SWZ_X_SHIFT)) |
         (src.swz[1] << NVFX_VP(SRC_SWZ_Y_SHIFT)) |
            if(src.indirect) {
      if(src.reg.type == NVFXSR_CONST)
         else if(src.reg.type == NVFXSR_INPUT)
         else
            if(src.indirect_reg)
                     switch (pos) {
   case 0:
      hw[1] |= ((sr & NVFX_VP(SRC0_HIGH_MASK)) >>
         hw[2] |= (sr & NVFX_VP(SRC0_LOW_MASK)) <<
            case 1:
      hw[2] |= sr << NVFX_VP(INST_SRC1_SHIFT);
      case 2:
      hw[2] |= ((sr & NVFX_VP(SRC2_HIGH_MASK)) >>
         hw[3] |= (sr & NVFX_VP(SRC2_LOW_MASK)) <<
            default:
            }
      static void
   emit_dst(struct nvfx_vpc *vpc, uint32_t *hw,
         {
               switch (dst.type) {
   case NVFXSR_NONE:
      if(!vpc->is_nv4x)
         else {
      hw[3] |= NV40_VP_INST_DEST_MASK;
   if (slot == 0)
         else
      }
      case NVFXSR_TEMP:
      if(!vpc->is_nv4x)
         else {
      hw[3] |= NV40_VP_INST_DEST_MASK;
   if (slot == 0)
         else
      }
      case NVFXSR_OUTPUT:
      /* TODO: this may be wrong because on nv30 COL0 and BFC0 are swapped */
   if(vpc->is_nv4x) {
      switch (dst.index) {
   case NV30_VP_INST_DEST_CLP(0):
      dst.index = NVFX_VP(INST_DEST_FOGC);
   vp->or   |= (1 << 6);
      case NV30_VP_INST_DEST_CLP(1):
      dst.index = NVFX_VP(INST_DEST_FOGC);
   vp->or   |= (1 << 7);
      case NV30_VP_INST_DEST_CLP(2):
      dst.index = NVFX_VP(INST_DEST_FOGC);
   vp->or   |= (1 << 8);
      case NV30_VP_INST_DEST_CLP(3):
      dst.index = NVFX_VP(INST_DEST_PSZ);
   vp->or   |= (1 << 9);
      case NV30_VP_INST_DEST_CLP(4):
      dst.index = NVFX_VP(INST_DEST_PSZ);
   vp->or   |= (1 << 10);
      case NV30_VP_INST_DEST_CLP(5):
      dst.index = NVFX_VP(INST_DEST_PSZ);
   vp->or   |= (1 << 11);
      case NV40_VP_INST_DEST_COL0: vp->or |= (1 << 0); break;
   case NV40_VP_INST_DEST_COL1: vp->or |= (1 << 1); break;
   case NV40_VP_INST_DEST_BFC0: vp->or |= (1 << 2); break;
   case NV40_VP_INST_DEST_BFC1: vp->or |= (1 << 3); break;
   case NV40_VP_INST_DEST_FOGC: vp->or |= (1 << 4); break;
   case NV40_VP_INST_DEST_PSZ : vp->or |= (1 << 5); break;
               if(!vpc->is_nv4x) {
                     /*XXX: no way this is entirely correct, someone needs to
   *     figure out what exactly it is.
   */
      } else {
      hw[3] |= (dst.index << NV40_VP_INST_DEST_SHIFT);
   if (slot == 0) {
      hw[0] |= NV40_VP_INST_VEC_RESULT;
      } else {
      hw[3] |= NV40_VP_INST_SCA_RESULT;
         }
      default:
            }
      static void
   nvfx_vp_emit(struct nvfx_vpc *vpc, struct nvfx_insn insn)
   {
      struct nv30_vertprog *vp = vpc->vp;
   unsigned slot = insn.op >> 7;
   unsigned op = insn.op & 0x7f;
            vp->insns = realloc(vp->insns, ++vp->nr_insns * sizeof(*vpc->vpi));
   vpc->vpi = &vp->insns[vp->nr_insns - 1];
                     if (insn.cc_test != NVFX_COND_TR)
         hw[0] |= (insn.cc_test << NVFX_VP(INST_COND_SHIFT));
   hw[0] |= ((insn.cc_swz[0] << NVFX_VP(INST_COND_SWZ_X_SHIFT)) |
            (insn.cc_swz[1] << NVFX_VP(INST_COND_SWZ_Y_SHIFT)) |
      if(insn.cc_update)
            if(insn.sat) {
      assert(vpc->is_nv4x);
   if(vpc->is_nv4x)
               if(!vpc->is_nv4x) {
      if(slot == 0)
         else {
      hw[0] |= ((op >> 4) << NV30_VP_INST_SCA_OPCODEH_SHIFT);
      //      hw[3] |= NVFX_VP(INST_SCA_DEST_TEMP_MASK);
   //      hw[3] |= (mask << NVFX_VP(INST_VEC_WRITEMASK_SHIFT));
            if (insn.dst.type == NVFXSR_OUTPUT) {
      if (slot)
         else
      } else {
      if (slot)
         else
         } else {
      if (slot == 0) {
      hw[1] |= (op << NV40_VP_INST_VEC_OPCODE_SHIFT);
   hw[3] |= NV40_VP_INST_SCA_DEST_TEMP_MASK;
      } else {
      hw[1] |= (op << NV40_VP_INST_SCA_OPCODE_SHIFT);
   hw[0] |= NV40_VP_INST_VEC_DEST_TEMP_MASK ;
                  emit_dst(vpc, hw, slot, insn.dst);
   emit_src(vpc, hw, 0, insn.src[0]);
   emit_src(vpc, hw, 1, insn.src[1]);
         //   if(insn.src[0].indirect || op == NVFX_VP_INST_VEC_OP_ARL)
   //      hw[3] |= NV40_VP_INST_SCA_RESULT;
   }
      static inline struct nvfx_src
   tgsi_src(struct nvfx_vpc *vpc, const struct tgsi_full_src_register *fsrc) {
               switch (fsrc->Register.File) {
   case TGSI_FILE_INPUT:
      src.reg = nvfx_reg(NVFXSR_INPUT, fsrc->Register.Index);
      case TGSI_FILE_CONSTANT:
      if(fsrc->Register.Indirect) {
      src.reg = vpc->r_const[0];
      } else {
         }
      case TGSI_FILE_IMMEDIATE:
      src.reg = vpc->imm[fsrc->Register.Index];
      case TGSI_FILE_TEMPORARY:
      src.reg = vpc->r_temp[fsrc->Register.Index];
      default:
      NOUVEAU_ERR("bad src file\n");
   src.reg.index = 0;
   src.reg.type = -1;
               src.abs = fsrc->Register.Absolute;
   src.negate = fsrc->Register.Negate;
   src.swz[0] = fsrc->Register.SwizzleX;
   src.swz[1] = fsrc->Register.SwizzleY;
   src.swz[2] = fsrc->Register.SwizzleZ;
   src.swz[3] = fsrc->Register.SwizzleW;
   src.indirect = 0;
   src.indirect_reg = 0;
            if(fsrc->Register.Indirect) {
      if(fsrc->Indirect.File == TGSI_FILE_ADDRESS &&
      (fsrc->Register.File == TGSI_FILE_CONSTANT ||
   fsrc->Register.File == TGSI_FILE_INPUT)) {
   src.indirect = 1;
   src.indirect_reg = fsrc->Indirect.Index;
      } else {
      src.reg.index = 0;
                     }
      static inline struct nvfx_reg
   tgsi_dst(struct nvfx_vpc *vpc, const struct tgsi_full_dst_register *fdst) {
               switch (fdst->Register.File) {
   case TGSI_FILE_NULL:
      dst = nvfx_reg(NVFXSR_NONE, 0);
      case TGSI_FILE_OUTPUT:
      dst = vpc->r_result[fdst->Register.Index];
      case TGSI_FILE_TEMPORARY:
      dst = vpc->r_temp[fdst->Register.Index];
      case TGSI_FILE_ADDRESS:
      dst = vpc->r_address[fdst->Register.Index];
      default:
      NOUVEAU_ERR("bad dst file %i\n", fdst->Register.File);
   dst.index = 0;
   dst.type = 0;
                  }
      static inline int
   tgsi_mask(uint tgsi)
   {
               if (tgsi & TGSI_WRITEMASK_X) mask |= NVFX_VP_MASK_X;
   if (tgsi & TGSI_WRITEMASK_Y) mask |= NVFX_VP_MASK_Y;
   if (tgsi & TGSI_WRITEMASK_Z) mask |= NVFX_VP_MASK_Z;
   if (tgsi & TGSI_WRITEMASK_W) mask |= NVFX_VP_MASK_W;
      }
      static bool
   nvfx_vertprog_parse_instruction(struct nvfx_vpc *vpc,
         {
      struct nvfx_src src[3], tmp;
   struct nvfx_reg dst;
   struct nvfx_reg final_dst;
   struct nvfx_src none = nvfx_src(nvfx_reg(NVFXSR_NONE, 0));
   struct nvfx_insn insn;
   struct nvfx_relocation reloc;
   struct nvfx_loop_entry loop;
   bool sat = false;
   int mask;
   int ai = -1, ci = -1, ii = -1;
   int i;
            for (i = 0; i < finst->Instruction.NumSrcRegs; i++) {
               fsrc = &finst->Src[i];
   if (fsrc->Register.File == TGSI_FILE_TEMPORARY) {
                     for (i = 0; i < finst->Instruction.NumSrcRegs; i++) {
                        switch (fsrc->Register.File) {
   case TGSI_FILE_INPUT:
      if (ai == -1 || ai == fsrc->Register.Index) {
      ai = fsrc->Register.Index;
      } else {
      src[i] = nvfx_src(temp(vpc));
   nvfx_vp_emit(vpc, arith(0, VEC, MOV, src[i].reg, NVFX_VP_MASK_ALL,
      }
      case TGSI_FILE_CONSTANT:
      if ((ci == -1 && ii == -1) ||
      ci == fsrc->Register.Index) {
   ci = fsrc->Register.Index;
      } else {
      src[i] = nvfx_src(temp(vpc));
   nvfx_vp_emit(vpc, arith(0, VEC, MOV, src[i].reg, NVFX_VP_MASK_ALL,
      }
      case TGSI_FILE_IMMEDIATE:
      if ((ci == -1 && ii == -1) ||
      ii == fsrc->Register.Index) {
   ii = fsrc->Register.Index;
      } else {
      src[i] = nvfx_src(temp(vpc));
   nvfx_vp_emit(vpc, arith(0, VEC, MOV, src[i].reg, NVFX_VP_MASK_ALL,
      }
      case TGSI_FILE_TEMPORARY:
      /* handled above */
      default:
      NOUVEAU_ERR("bad src file\n");
                  for (i = 0; i < finst->Instruction.NumSrcRegs; i++) {
      if(src[i].reg.type < 0)
               if(finst->Dst[0].Register.File == TGSI_FILE_ADDRESS &&
      finst->Instruction.Opcode != TGSI_OPCODE_ARL)
         final_dst = dst  = tgsi_dst(vpc, &finst->Dst[0]);
   mask = tgsi_mask(finst->Dst[0].Register.WriteMask);
   if(finst->Instruction.Saturate) {
      assert(finst->Instruction.Opcode != TGSI_OPCODE_ARL);
   if (vpc->is_nv4x)
         else
   if(dst.type != NVFXSR_TEMP)
               switch (finst->Instruction.Opcode) {
   case TGSI_OPCODE_ADD:
      nvfx_vp_emit(vpc, arith(sat, VEC, ADD, dst, mask, src[0], none, src[1]));
      case TGSI_OPCODE_ARL:
      nvfx_vp_emit(vpc, arith(0, VEC, ARL, dst, mask, src[0], none, none));
      case TGSI_OPCODE_CEIL:
      tmp = nvfx_src(temp(vpc));
   nvfx_vp_emit(vpc, arith(0, VEC, FLR, tmp.reg, mask, neg(src[0]), none, none));
   nvfx_vp_emit(vpc, arith(sat, VEC, MOV, dst, mask, neg(tmp), none, none));
      case TGSI_OPCODE_CMP:
      insn = arith(0, VEC, MOV, none.reg, mask, src[0], none, none);
   insn.cc_update = 1;
            insn = arith(sat, VEC, MOV, dst, mask, src[2], none, none);
   insn.cc_test = NVFX_COND_GE;
            insn = arith(sat, VEC, MOV, dst, mask, src[1], none, none);
   insn.cc_test = NVFX_COND_LT;
   nvfx_vp_emit(vpc, insn);
      case TGSI_OPCODE_COS:
      nvfx_vp_emit(vpc, arith(sat, SCA, COS, dst, mask, none, none, src[0]));
      case TGSI_OPCODE_DP2:
      tmp = nvfx_src(temp(vpc));
   nvfx_vp_emit(vpc, arith(0, VEC, MUL, tmp.reg, NVFX_VP_MASK_X | NVFX_VP_MASK_Y, src[0], src[1], none));
   nvfx_vp_emit(vpc, arith(sat, VEC, ADD, dst, mask, swz(tmp, X, X, X, X), none, swz(tmp, Y, Y, Y, Y)));
      case TGSI_OPCODE_DP3:
      nvfx_vp_emit(vpc, arith(sat, VEC, DP3, dst, mask, src[0], src[1], none));
      case TGSI_OPCODE_DP4:
      nvfx_vp_emit(vpc, arith(sat, VEC, DP4, dst, mask, src[0], src[1], none));
      case TGSI_OPCODE_DST:
      nvfx_vp_emit(vpc, arith(sat, VEC, DST, dst, mask, src[0], src[1], none));
      case TGSI_OPCODE_EX2:
      nvfx_vp_emit(vpc, arith(sat, SCA, EX2, dst, mask, none, none, src[0]));
      case TGSI_OPCODE_EXP:
      nvfx_vp_emit(vpc, arith(sat, SCA, EXP, dst, mask, none, none, src[0]));
      case TGSI_OPCODE_FLR:
      nvfx_vp_emit(vpc, arith(sat, VEC, FLR, dst, mask, src[0], none, none));
      case TGSI_OPCODE_FRC:
      nvfx_vp_emit(vpc, arith(sat, VEC, FRC, dst, mask, src[0], none, none));
      case TGSI_OPCODE_LG2:
      nvfx_vp_emit(vpc, arith(sat, SCA, LG2, dst, mask, none, none, src[0]));
      case TGSI_OPCODE_LIT:
      nvfx_vp_emit(vpc, arith(sat, SCA, LIT, dst, mask, none, none, src[0]));
      case TGSI_OPCODE_LOG:
      nvfx_vp_emit(vpc, arith(sat, SCA, LOG, dst, mask, none, none, src[0]));
      case TGSI_OPCODE_LRP:
      tmp = nvfx_src(temp(vpc));
   nvfx_vp_emit(vpc, arith(0, VEC, MAD, tmp.reg, mask, neg(src[0]), src[2], src[2]));
   nvfx_vp_emit(vpc, arith(sat, VEC, MAD, dst, mask, src[0], src[1], tmp));
      case TGSI_OPCODE_MAD:
      nvfx_vp_emit(vpc, arith(sat, VEC, MAD, dst, mask, src[0], src[1], src[2]));
      case TGSI_OPCODE_MAX:
      nvfx_vp_emit(vpc, arith(sat, VEC, MAX, dst, mask, src[0], src[1], none));
      case TGSI_OPCODE_MIN:
      nvfx_vp_emit(vpc, arith(sat, VEC, MIN, dst, mask, src[0], src[1], none));
      case TGSI_OPCODE_MOV:
      nvfx_vp_emit(vpc, arith(sat, VEC, MOV, dst, mask, src[0], none, none));
      case TGSI_OPCODE_MUL:
      nvfx_vp_emit(vpc, arith(sat, VEC, MUL, dst, mask, src[0], src[1], none));
      case TGSI_OPCODE_NOP:
         case TGSI_OPCODE_POW:
      tmp = nvfx_src(temp(vpc));
   nvfx_vp_emit(vpc, arith(0, SCA, LG2, tmp.reg, NVFX_VP_MASK_X, none, none, swz(src[0], X, X, X, X)));
   nvfx_vp_emit(vpc, arith(0, VEC, MUL, tmp.reg, NVFX_VP_MASK_X, swz(tmp, X, X, X, X), swz(src[1], X, X, X, X), none));
   nvfx_vp_emit(vpc, arith(sat, SCA, EX2, dst, mask, none, none, swz(tmp, X, X, X, X)));
      case TGSI_OPCODE_RCP:
      nvfx_vp_emit(vpc, arith(sat, SCA, RCP, dst, mask, none, none, src[0]));
      case TGSI_OPCODE_RSQ:
      nvfx_vp_emit(vpc, arith(sat, SCA, RSQ, dst, mask, none, none, abs(src[0])));
      case TGSI_OPCODE_SEQ:
      nvfx_vp_emit(vpc, arith(sat, VEC, SEQ, dst, mask, src[0], src[1], none));
      case TGSI_OPCODE_SGE:
      nvfx_vp_emit(vpc, arith(sat, VEC, SGE, dst, mask, src[0], src[1], none));
      case TGSI_OPCODE_SGT:
      nvfx_vp_emit(vpc, arith(sat, VEC, SGT, dst, mask, src[0], src[1], none));
      case TGSI_OPCODE_SIN:
      nvfx_vp_emit(vpc, arith(sat, SCA, SIN, dst, mask, none, none, src[0]));
      case TGSI_OPCODE_SLE:
      nvfx_vp_emit(vpc, arith(sat, VEC, SLE, dst, mask, src[0], src[1], none));
      case TGSI_OPCODE_SLT:
      nvfx_vp_emit(vpc, arith(sat, VEC, SLT, dst, mask, src[0], src[1], none));
      case TGSI_OPCODE_SNE:
      nvfx_vp_emit(vpc, arith(sat, VEC, SNE, dst, mask, src[0], src[1], none));
      case TGSI_OPCODE_SSG:
      nvfx_vp_emit(vpc, arith(sat, VEC, SSG, dst, mask, src[0], none, none));
      case TGSI_OPCODE_TRUNC:
      tmp = nvfx_src(temp(vpc));
   insn = arith(0, VEC, MOV, none.reg, mask, src[0], none, none);
   insn.cc_update = 1;
            nvfx_vp_emit(vpc, arith(0, VEC, FLR, tmp.reg, mask, abs(src[0]), none, none));
            insn = arith(sat, VEC, MOV, dst, mask, neg(tmp), none, none);
   insn.cc_test = NVFX_COND_LT;
   nvfx_vp_emit(vpc, insn);
      case TGSI_OPCODE_IF:
      insn = arith(0, VEC, MOV, none.reg, NVFX_VP_MASK_X, src[0], none, none);
   insn.cc_update = 1;
            reloc.location = vpc->vp->nr_insns;
   reloc.target = finst->Label.Label + 1;
            insn = arith(0, SCA, BRA, none.reg, 0, none, none, none);
   insn.cc_test = NVFX_COND_EQ;
   insn.cc_swz[0] = insn.cc_swz[1] = insn.cc_swz[2] = insn.cc_swz[3] = 0;
   nvfx_vp_emit(vpc, insn);
      case TGSI_OPCODE_ELSE:
   case TGSI_OPCODE_CAL:
      reloc.location = vpc->vp->nr_insns;
   reloc.target = finst->Label.Label;
            if(finst->Instruction.Opcode == TGSI_OPCODE_CAL)
         else
         nvfx_vp_emit(vpc, insn);
      case TGSI_OPCODE_RET:
      if(sub_depth || !vpc->vp->enabled_ucps) {
      tmp = none;
   tmp.swz[0] = tmp.swz[1] = tmp.swz[2] = tmp.swz[3] = 0;
      } else {
      reloc.location = vpc->vp->nr_insns;
   reloc.target = vpc->info->num_instructions;
   util_dynarray_append(&vpc->label_relocs, struct nvfx_relocation, reloc);
      }
      case TGSI_OPCODE_BGNSUB:
      ++sub_depth;
      case TGSI_OPCODE_ENDSUB:
      --sub_depth;
      case TGSI_OPCODE_ENDIF:
      /* nothing to do here */
      case TGSI_OPCODE_BGNLOOP:
      loop.cont_target = idx;
   loop.brk_target = finst->Label.Label + 1;
   util_dynarray_append(&vpc->loop_stack, struct nvfx_loop_entry, loop);
      case TGSI_OPCODE_ENDLOOP:
               reloc.location = vpc->vp->nr_insns;
   reloc.target = loop.cont_target;
            nvfx_vp_emit(vpc, arith(0, SCA, BRA, none.reg, 0, none, none, none));
      case TGSI_OPCODE_CONT:
               reloc.location = vpc->vp->nr_insns;
   reloc.target = loop.cont_target;
            nvfx_vp_emit(vpc, arith(0, SCA, BRA, none.reg, 0, none, none, none));
      case TGSI_OPCODE_BRK:
               reloc.location = vpc->vp->nr_insns;
   reloc.target = loop.brk_target;
            nvfx_vp_emit(vpc, arith(0, SCA, BRA, none.reg, 0, none, none, none));
      case TGSI_OPCODE_END:
      assert(!sub_depth);
   if(vpc->vp->enabled_ucps) {
      if(idx != (vpc->info->num_instructions - 1)) {
      reloc.location = vpc->vp->nr_insns;
   reloc.target = vpc->info->num_instructions;
   util_dynarray_append(&vpc->label_relocs, struct nvfx_relocation, reloc);
         } else {
      if(vpc->vp->nr_insns)
         nvfx_vp_emit(vpc, arith(0, VEC, NOP, none.reg, 0, none, none, none));
      }
      default:
      NOUVEAU_ERR("invalid opcode %d\n", finst->Instruction.Opcode);
               if(finst->Instruction.Saturate && !vpc->is_nv4x) {
      if (!vpc->r_0_1.type)
         nvfx_vp_emit(vpc, arith(0, VEC, MAX, dst, mask, nvfx_src(dst), swz(nvfx_src(vpc->r_0_1), X, X, X, X), none));
               release_temps(vpc);
      }
      static bool
   nvfx_vertprog_parse_decl_output(struct nvfx_vpc *vpc,
         {
      unsigned num_texcoords = vpc->is_nv4x ? 10 : 8;
   unsigned idx = fdec->Range.First;
   unsigned semantic_index = fdec->Semantic.Index;
            switch (fdec->Semantic.Name) {
   case TGSI_SEMANTIC_POSITION:
      hw = NVFX_VP(INST_DEST_POS);
   vpc->hpos_idx = idx;
      case TGSI_SEMANTIC_CLIPVERTEX:
      vpc->r_result[idx] = temp(vpc);
   vpc->r_temps_discard = 0;
   vpc->cvtx_idx = idx;
      case TGSI_SEMANTIC_COLOR:
      if (fdec->Semantic.Index == 0) {
         } else
   if (fdec->Semantic.Index == 1) {
         } else {
      NOUVEAU_ERR("bad colour semantic index\n");
      }
      case TGSI_SEMANTIC_BCOLOR:
      if (fdec->Semantic.Index == 0) {
         } else
   if (fdec->Semantic.Index == 1) {
         } else {
      NOUVEAU_ERR("bad bcolour semantic index\n");
      }
      case TGSI_SEMANTIC_FOG:
      hw = NVFX_VP(INST_DEST_FOGC);
      case TGSI_SEMANTIC_PSIZE:
      hw = NVFX_VP(INST_DEST_PSZ);
      case TGSI_SEMANTIC_GENERIC:
      /* this is really an identifier for VP/FP linkage */
   semantic_index += 8;
      case TGSI_SEMANTIC_TEXCOORD:
      for (i = 0; i < num_texcoords; i++) {
      if (vpc->vp->texcoord[i] == semantic_index) {
      hw = NVFX_VP(INST_DEST_TC(i));
                  if (i == num_texcoords) {
      vpc->r_result[idx] = nvfx_reg(NVFXSR_NONE, 0);
      }
      case TGSI_SEMANTIC_EDGEFLAG:
      vpc->r_result[idx] = nvfx_reg(NVFXSR_NONE, 0);
      default:
      NOUVEAU_ERR("bad output semantic\n");
               vpc->r_result[idx] = nvfx_reg(NVFXSR_OUTPUT, hw);
      }
      static bool
   nvfx_vertprog_prepare(struct nvfx_vpc *vpc)
   {
      struct tgsi_parse_context p;
            tgsi_parse_init(&p, vpc->pipe.tokens);
   while (!tgsi_parse_end_of_tokens(&p)) {
               tgsi_parse_token(&p);
   switch(tok->Token.Type) {
   case TGSI_TOKEN_TYPE_IMMEDIATE:
      nr_imm++;
      case TGSI_TOKEN_TYPE_DECLARATION:
   {
               fdec = &p.FullToken.FullDeclaration;
   switch (fdec->Declaration.File) {
   case TGSI_FILE_TEMPORARY:
      if (fdec->Range.Last > high_temp) {
      high_temp =
      }
      case TGSI_FILE_ADDRESS:
      if (fdec->Range.Last > high_addr) {
      high_addr =
      }
      case TGSI_FILE_CONSTANT:
      if (fdec->Range.Last > high_const) {
      high_const =
      }
      case TGSI_FILE_OUTPUT:
      if (!nvfx_vertprog_parse_decl_output(vpc, fdec))
            default:
            }
         default:
            }
            if (nr_imm) {
      vpc->imm = CALLOC(nr_imm, sizeof(struct nvfx_reg));
               if (++high_temp) {
      vpc->r_temp = CALLOC(high_temp, sizeof(struct nvfx_reg));
   for (i = 0; i < high_temp; i++)
               if (++high_addr) {
      vpc->r_address = CALLOC(high_addr, sizeof(struct nvfx_reg));
   for (i = 0; i < high_addr; i++)
               if(++high_const) {
      vpc->r_const = CALLOC(high_const, sizeof(struct nvfx_reg));
   for (i = 0; i < high_const; i++)
               vpc->r_temps_discard = 0;
      }
      DEBUG_GET_ONCE_BOOL_OPTION(nvfx_dump_vp, "NVFX_DUMP_VP", false)
      bool
   _nvfx_vertprog_translate(uint16_t oclass, struct nv30_vertprog *vp)
   {
      struct tgsi_parse_context parse;
   struct nvfx_vpc *vpc = NULL;
   struct nvfx_src none = nvfx_src(nvfx_reg(NVFXSR_NONE, 0));
   struct util_dynarray insns;
            vp->translated = false;
   vp->nr_insns = 0;
            vpc = CALLOC_STRUCT(nvfx_vpc);
   if (!vpc)
         vpc->is_nv4x = (oclass >= NV40_3D_CLASS) ? ~0 : 0;
   vpc->vp   = vp;
   vpc->pipe = vp->pipe;
   vpc->info = &vp->info;
            if (!nvfx_vertprog_prepare(vpc)) {
      FREE(vpc);
               /* Redirect post-transform vertex position to a temp if user clip
   * planes are enabled.  We need to append code to the vtxprog
   * to handle clip planes later.
   */
   if (vp->enabled_ucps && vpc->cvtx_idx < 0)  {
      vpc->r_result[vpc->hpos_idx] = temp(vpc);
   vpc->r_temps_discard = 0;
                        tgsi_parse_init(&parse, vp->pipe.tokens);
   while (!tgsi_parse_end_of_tokens(&parse)) {
               switch (parse.FullToken.Token.Type) {
   case TGSI_TOKEN_TYPE_IMMEDIATE:
   {
               imm = &parse.FullToken.FullImmediate;
   assert(imm->Immediate.DataType == TGSI_IMM_FLOAT32);
   assert(imm->Immediate.NrTokens == 4 + 1);
   vpc->imm[vpc->nr_imm++] =
      constant(vpc, -1,
      imm->u[0].Float,
   imm->u[1].Float,
      }
         case TGSI_TOKEN_TYPE_INSTRUCTION:
   {
      const struct tgsi_full_instruction *finst;
   unsigned idx = insns.size >> 2;
   util_dynarray_append(&insns, unsigned, vp->nr_insns);
   finst = &parse.FullToken.FullInstruction;
   if (!nvfx_vertprog_parse_instruction(vpc, idx, finst))
      }
         default:
                              for(unsigned i = 0; i < vpc->label_relocs.size; i += sizeof(struct nvfx_relocation))
   {
      struct nvfx_relocation* label_reloc = (struct nvfx_relocation*)((char*)vpc->label_relocs.data + i);
            hw_reloc.location = label_reloc->location;
                        }
   util_dynarray_fini(&insns);
                     /* Write out HPOS if it was redirected to a temp earlier */
   if (vpc->r_result[vpc->hpos_idx].type != NVFXSR_OUTPUT) {
      struct nvfx_reg hpos = nvfx_reg(NVFXSR_OUTPUT,
                              /* Insert code to handle user clip planes */
   ucps = vp->enabled_ucps;
   while (ucps) {
      int i = ffs(ucps) - 1; ucps &= ~(1 << i);
   struct nvfx_reg cdst = nvfx_reg(NVFXSR_OUTPUT, NV30_VP_INST_DEST_CLP(i));
   struct nvfx_src ceqn = nvfx_src(nvfx_reg(NVFXSR_CONST, 512 + i));
   struct nvfx_src htmp = nvfx_src(vpc->r_result[vpc->cvtx_idx]);
            if(vpc->is_nv4x)
   {
      switch (i) {
   case 0: case 3: mask = NVFX_VP_MASK_Y; break;
   case 1: case 4: mask = NVFX_VP_MASK_Z; break;
   case 2: case 5: mask = NVFX_VP_MASK_W; break;
   default:
      NOUVEAU_ERR("invalid clip dist #%d\n", i);
         }
   else
                        if (vpc->vp->nr_insns)
            if(debug_get_option_nvfx_dump_vp())
   {
      debug_printf("\n");
            debug_printf("\n%s vertex program:\n", vpc->is_nv4x ? "nv4x" : "nv3x");
   for (i = 0; i < vp->nr_insns; i++)
                           out:
      tgsi_parse_free(&parse);
   if (vpc) {
      util_dynarray_fini(&vpc->label_relocs);
   util_dynarray_fini(&vpc->loop_stack);
   FREE(vpc->r_temp);
   FREE(vpc->r_address);
   FREE(vpc->r_const);
   FREE(vpc->imm);
                  }
