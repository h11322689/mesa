   #include <float.h>
   #include "pipe/p_context.h"
   #include "pipe/p_defines.h"
   #include "pipe/p_state.h"
   #include "util/u_dynarray.h"
   #include "util/u_inlines.h"
   #include "util/u_debug.h"
   #include "util/u_memory.h"
      #include "pipe/p_shader_tokens.h"
   #include "tgsi/tgsi_parse.h"
   #include "tgsi/tgsi_dump.h"
      #include "nouveau_debug.h"
   #include "nv_object.xml.h"
   #include "nv30/nv30-40_3d.xml.h"
   #include "nv30/nvfx_shader.h"
   #include "nv30/nv30_state.h"
      struct nvfx_fpc {
               unsigned max_temps;
   unsigned long long r_temps;
   unsigned long long r_temps_discard;
   struct nvfx_reg r_result[PIPE_MAX_SHADER_OUTPUTS];
   struct nvfx_reg r_input[PIPE_MAX_SHADER_INPUTS];
                     unsigned inst_offset;
   unsigned have_const;
                     struct nvfx_reg* r_imm;
            struct util_dynarray if_stack;
   //struct util_dynarray loop_stack;
      };
      static inline struct nvfx_reg
   temp(struct nvfx_fpc *fpc)
   {
               if (idx >= fpc->max_temps) {
      NOUVEAU_ERR("out of temps!!\n");
               fpc->r_temps |= (1ULL << idx);
   fpc->r_temps_discard |= (1ULL << idx);
      }
      static inline void
   release_temps(struct nvfx_fpc *fpc)
   {
      fpc->r_temps &= ~fpc->r_temps_discard;
      }
      static inline struct nvfx_reg
   nvfx_fp_imm(struct nvfx_fpc *fpc, float a, float b, float c, float d)
   {
      float v[4] = {a, b, c, d};
            memcpy(util_dynarray_grow(&fpc->imm_data, float, 4), v, 4 * sizeof(float));
      }
      static void
   grow_insns(struct nvfx_fpc *fpc, int size)
   {
               fp->insn_len += size;
      }
      static void
   emit_src(struct nvfx_fpc *fpc, int pos, struct nvfx_src src)
   {
      struct nv30_fragprog *fp = fpc->fp;
   uint32_t *hw = &fp->insn[fpc->inst_offset];
            switch (src.reg.type) {
   case NVFXSR_INPUT:
      sr |= (NVFX_FP_REG_TYPE_INPUT << NVFX_FP_REG_TYPE_SHIFT);
   hw[0] |= (src.reg.index << NVFX_FP_OP_INPUT_SRC_SHIFT);
      case NVFXSR_OUTPUT:
      sr |= NVFX_FP_REG_SRC_HALF;
      case NVFXSR_TEMP:
      sr |= (NVFX_FP_REG_TYPE_TEMP << NVFX_FP_REG_TYPE_SHIFT);
   sr |= (src.reg.index << NVFX_FP_REG_SRC_SHIFT);
      case NVFXSR_IMM:
      if (!fpc->have_const) {
      grow_insns(fpc, 4);
   hw = &fp->insn[fpc->inst_offset];
               memcpy(&fp->insn[fpc->inst_offset + 4],
                  sr |= (NVFX_FP_REG_TYPE_CONST << NVFX_FP_REG_TYPE_SHIFT);
      case NVFXSR_CONST:
      if (!fpc->have_const) {
      grow_insns(fpc, 4);
   hw = &fp->insn[fpc->inst_offset];
               {
               fp->consts = realloc(fp->consts, ++fp->nr_consts *
         fpd = &fp->consts[fp->nr_consts - 1];
   fpd->offset = fpc->inst_offset + 4;
   fpd->index = src.reg.index;
               sr |= (NVFX_FP_REG_TYPE_CONST << NVFX_FP_REG_TYPE_SHIFT);
      case NVFXSR_NONE:
      sr |= (NVFX_FP_REG_TYPE_INPUT << NVFX_FP_REG_TYPE_SHIFT);
      default:
                  if (src.negate)
            if (src.abs)
            sr |= ((src.swz[0] << NVFX_FP_REG_SWZ_X_SHIFT) |
         (src.swz[1] << NVFX_FP_REG_SWZ_Y_SHIFT) |
               }
      static void
   emit_dst(struct nvfx_fpc *fpc, struct nvfx_reg dst)
   {
      struct nv30_fragprog *fp = fpc->fp;
            switch (dst.type) {
   case NVFXSR_OUTPUT:
      if (dst.index == 1)
         else {
      hw[0] |= NVFX_FP_OP_OUT_REG_HALF;
      }
      case NVFXSR_TEMP:
      if (fpc->num_regs < (dst.index + 1))
            case NVFXSR_NONE:
      hw[0] |= (1 << 30);
      default:
                     }
      static void
   nvfx_fp_emit(struct nvfx_fpc *fpc, struct nvfx_insn insn)
   {
      struct nv30_fragprog *fp = fpc->fp;
            fpc->inst_offset = fp->insn_len;
   fpc->have_const = 0;
   grow_insns(fpc, 4);
   hw = &fp->insn[fpc->inst_offset];
            if (insn.op == NVFX_FP_OP_OPCODE_KIL)
         hw[0] |= (insn.op << NVFX_FP_OP_OPCODE_SHIFT);
   hw[0] |= (insn.mask << NVFX_FP_OP_OUTMASK_SHIFT);
            if (insn.sat)
            if (insn.cc_update)
         hw[1] |= (insn.cc_test << NVFX_FP_OP_COND_SHIFT);
   hw[1] |= ((insn.cc_swz[0] << NVFX_FP_OP_COND_SWZ_X_SHIFT) |
      (insn.cc_swz[1] << NVFX_FP_OP_COND_SWZ_Y_SHIFT) |
   (insn.cc_swz[2] << NVFX_FP_OP_COND_SWZ_Z_SHIFT) |
         if(insn.unit >= 0)
   {
                  emit_dst(fpc, insn.dst);
   emit_src(fpc, 0, insn.src[0]);
   emit_src(fpc, 1, insn.src[1]);
      }
      #define arith(s,o,d,m,s0,s1,s2) \
         nvfx_insn((s), NVFX_FP_OP_OPCODE_##o, -1, \
      #define tex(s,o,u,d,m,s0,s1,s2) \
      nvfx_insn((s), NVFX_FP_OP_OPCODE_##o, (u), \
         /* IF src.x != 0, as TGSI specifies */
   static void
   nv40_fp_if(struct nvfx_fpc *fpc, struct nvfx_src src)
   {
      const struct nvfx_src none = nvfx_src(nvfx_reg(NVFXSR_NONE, 0));
   struct nvfx_insn insn = arith(0, MOV, none.reg, NVFX_FP_MASK_X, src, none, none);
   uint32_t *hw;
   insn.cc_update = 1;
            fpc->inst_offset = fpc->fp->insn_len;
   grow_insns(fpc, 4);
   hw = &fpc->fp->insn[fpc->inst_offset];
   /* I really wonder why fp16 precision is used. Presumably the hardware ignores it? */
   hw[0] = (NV40_FP_OP_BRA_OPCODE_IF << NVFX_FP_OP_OPCODE_SHIFT) |
      NV40_FP_OP_OUT_NONE |
      /* Use .xxxx swizzle so that we check only src[0].x*/
   hw[1] = (0 << NVFX_FP_OP_COND_SWZ_X_SHIFT) |
         (0 << NVFX_FP_OP_COND_SWZ_Y_SHIFT) |
   (0 << NVFX_FP_OP_COND_SWZ_Z_SHIFT) |
   (0 << NVFX_FP_OP_COND_SWZ_W_SHIFT) |
   hw[2] = 0; /* | NV40_FP_OP_OPCODE_IS_BRANCH | else_offset */
   hw[3] = 0; /* | endif_offset */
      }
      /* IF src.x != 0, as TGSI specifies */
   static void
   nv40_fp_cal(struct nvfx_fpc *fpc, unsigned target)
   {
         struct nvfx_relocation reloc;
   uint32_t *hw;
   fpc->inst_offset = fpc->fp->insn_len;
   grow_insns(fpc, 4);
   hw = &fpc->fp->insn[fpc->inst_offset];
   /* I really wonder why fp16 precision is used. Presumably the hardware ignores it? */
   hw[0] = (NV40_FP_OP_BRA_OPCODE_CAL << NVFX_FP_OP_OPCODE_SHIFT);
   /* Use .xxxx swizzle so that we check only src[0].x*/
   hw[1] = (NVFX_SWZ_IDENTITY << NVFX_FP_OP_COND_SWZ_ALL_SHIFT) |
         hw[2] = NV40_FP_OP_OPCODE_IS_BRANCH; /* | call_offset */
   hw[3] = 0;
   reloc.target = target;
   reloc.location = fpc->inst_offset + 2;
   }
      static void
   nv40_fp_ret(struct nvfx_fpc *fpc)
   {
      uint32_t *hw;
   fpc->inst_offset = fpc->fp->insn_len;
   grow_insns(fpc, 4);
   hw = &fpc->fp->insn[fpc->inst_offset];
   /* I really wonder why fp16 precision is used. Presumably the hardware ignores it? */
   hw[0] = (NV40_FP_OP_BRA_OPCODE_RET << NVFX_FP_OP_OPCODE_SHIFT);
   /* Use .xxxx swizzle so that we check only src[0].x*/
   hw[1] = (NVFX_SWZ_IDENTITY << NVFX_FP_OP_COND_SWZ_ALL_SHIFT) |
         hw[2] = NV40_FP_OP_OPCODE_IS_BRANCH; /* | call_offset */
      }
      static void
   nv40_fp_rep(struct nvfx_fpc *fpc, unsigned count, unsigned target)
   {
         struct nvfx_relocation reloc;
   uint32_t *hw;
   fpc->inst_offset = fpc->fp->insn_len;
   grow_insns(fpc, 4);
   hw = &fpc->fp->insn[fpc->inst_offset];
   /* I really wonder why fp16 precision is used. Presumably the hardware ignores it? */
   hw[0] = (NV40_FP_OP_BRA_OPCODE_REP << NVFX_FP_OP_OPCODE_SHIFT) |
               /* Use .xxxx swizzle so that we check only src[0].x*/
   hw[1] = (NVFX_SWZ_IDENTITY << NVFX_FP_OP_COND_SWZ_ALL_SHIFT) |
         hw[2] = NV40_FP_OP_OPCODE_IS_BRANCH |
                     hw[3] = 0; /* | end_offset */
   reloc.target = target;
   reloc.location = fpc->inst_offset + 3;
   util_dynarray_append(&fpc->label_relocs, struct nvfx_relocation, reloc);
   }
      #if 0
   /* documentation only */
   /* warning: this only works forward, and probably only if not inside any IF */
   static void
   nv40_fp_bra(struct nvfx_fpc *fpc, unsigned target)
   {
         struct nvfx_relocation reloc;
   uint32_t *hw;
   fpc->inst_offset = fpc->fp->insn_len;
   grow_insns(fpc, 4);
   hw = &fpc->fp->insn[fpc->inst_offset];
   /* I really wonder why fp16 precision is used. Presumably the hardware ignores it? */
   hw[0] = (NV40_FP_OP_BRA_OPCODE_IF << NVFX_FP_OP_OPCODE_SHIFT) |
               /* Use .xxxx swizzle so that we check only src[0].x*/
   hw[1] = (NVFX_SWZ_IDENTITY << NVFX_FP_OP_COND_SWZ_X_SHIFT) |
         hw[2] = NV40_FP_OP_OPCODE_IS_BRANCH; /* | else_offset */
   hw[3] = 0; /* | endif_offset */
   reloc.target = target;
   reloc.location = fpc->inst_offset + 2;
   util_dynarray_append(&fpc->label_relocs, struct nvfx_relocation, reloc);
   reloc.target = target;
   reloc.location = fpc->inst_offset + 3;
   }
   #endif
      static void
   nv40_fp_brk(struct nvfx_fpc *fpc)
   {
      uint32_t *hw;
   fpc->inst_offset = fpc->fp->insn_len;
   grow_insns(fpc, 4);
   hw = &fpc->fp->insn[fpc->inst_offset];
   /* I really wonder why fp16 precision is used. Presumably the hardware ignores it? */
   hw[0] = (NV40_FP_OP_BRA_OPCODE_BRK << NVFX_FP_OP_OPCODE_SHIFT) |
         /* Use .xxxx swizzle so that we check only src[0].x*/
   hw[1] = (NVFX_SWZ_IDENTITY << NVFX_FP_OP_COND_SWZ_X_SHIFT) |
         hw[2] = NV40_FP_OP_OPCODE_IS_BRANCH;
      }
      static inline struct nvfx_src
   tgsi_src(struct nvfx_fpc *fpc, const struct tgsi_full_src_register *fsrc)
   {
               switch (fsrc->Register.File) {
   case TGSI_FILE_INPUT:
      src.reg = fpc->r_input[fsrc->Register.Index];
      case TGSI_FILE_CONSTANT:
      src.reg = nvfx_reg(NVFXSR_CONST, fsrc->Register.Index);
      case TGSI_FILE_IMMEDIATE:
      assert(fsrc->Register.Index < fpc->nr_imm);
   src.reg = fpc->r_imm[fsrc->Register.Index];
      case TGSI_FILE_TEMPORARY:
      src.reg = fpc->r_temp[fsrc->Register.Index];
      /* NV40 fragprog result regs are just temps, so this is simple */
   case TGSI_FILE_OUTPUT:
      src.reg = fpc->r_result[fsrc->Register.Index];
      default:
      NOUVEAU_ERR("bad src file\n");
   src.reg.index = 0;
   src.reg.type = 0;
               src.abs = fsrc->Register.Absolute;
   src.negate = fsrc->Register.Negate;
   src.swz[0] = fsrc->Register.SwizzleX;
   src.swz[1] = fsrc->Register.SwizzleY;
   src.swz[2] = fsrc->Register.SwizzleZ;
   src.swz[3] = fsrc->Register.SwizzleW;
   src.indirect = 0;
   src.indirect_reg = 0;
   src.indirect_swz = 0;
      }
      static inline struct nvfx_reg
   tgsi_dst(struct nvfx_fpc *fpc, const struct tgsi_full_dst_register *fdst) {
      switch (fdst->Register.File) {
   case TGSI_FILE_OUTPUT:
         case TGSI_FILE_TEMPORARY:
         case TGSI_FILE_NULL:
         default:
      NOUVEAU_ERR("bad dst file %d\n", fdst->Register.File);
         }
      static inline int
   tgsi_mask(uint tgsi)
   {
               if (tgsi & TGSI_WRITEMASK_X) mask |= NVFX_FP_MASK_X;
   if (tgsi & TGSI_WRITEMASK_Y) mask |= NVFX_FP_MASK_Y;
   if (tgsi & TGSI_WRITEMASK_Z) mask |= NVFX_FP_MASK_Z;
   if (tgsi & TGSI_WRITEMASK_W) mask |= NVFX_FP_MASK_W;
      }
      static bool
   nvfx_fragprog_parse_instruction(struct nvfx_fpc *fpc,
         {
      const struct nvfx_src none = nvfx_src(nvfx_reg(NVFXSR_NONE, 0));
   struct nvfx_insn insn;
   struct nvfx_src src[3], tmp;
   struct nvfx_reg dst;
   int mask, sat, unit = 0;
   int ai = -1, ci = -1, ii = -1;
            if (finst->Instruction.Opcode == TGSI_OPCODE_END)
            for (i = 0; i < finst->Instruction.NumSrcRegs; i++) {
               fsrc = &finst->Src[i];
   if (fsrc->Register.File == TGSI_FILE_TEMPORARY) {
                     for (i = 0; i < finst->Instruction.NumSrcRegs; i++) {
                        switch (fsrc->Register.File) {
   case TGSI_FILE_INPUT:
      if(fpc->fp->info.input_semantic_name[fsrc->Register.Index] == TGSI_SEMANTIC_FOG && (0
         || fsrc->Register.SwizzleX == PIPE_SWIZZLE_W
   || fsrc->Register.SwizzleY == PIPE_SWIZZLE_W
   || fsrc->Register.SwizzleZ == PIPE_SWIZZLE_W
   || fsrc->Register.SwizzleW == PIPE_SWIZZLE_W
      /* hardware puts 0 in fogcoord.w, but GL/Gallium want 1 there */
   struct nvfx_src addend = nvfx_src(nvfx_fp_imm(fpc, 0, 0, 0, 1));
   addend.swz[0] = fsrc->Register.SwizzleX;
   addend.swz[1] = fsrc->Register.SwizzleY;
   addend.swz[2] = fsrc->Register.SwizzleZ;
   addend.swz[3] = fsrc->Register.SwizzleW;
   src[i] = nvfx_src(temp(fpc));
      } else if (ai == -1 || ai == fsrc->Register.Index) {
      ai = fsrc->Register.Index;
      } else {
      src[i] = nvfx_src(temp(fpc));
      }
      case TGSI_FILE_CONSTANT:
      if ((ci == -1 && ii == -1) ||
      ci == fsrc->Register.Index) {
   ci = fsrc->Register.Index;
      } else {
      src[i] = nvfx_src(temp(fpc));
      }
      case TGSI_FILE_IMMEDIATE:
      if ((ci == -1 && ii == -1) ||
      ii == fsrc->Register.Index) {
   ii = fsrc->Register.Index;
      } else {
      src[i] = nvfx_src(temp(fpc));
      }
      case TGSI_FILE_TEMPORARY:
      /* handled above */
      case TGSI_FILE_SAMPLER:
      unit = fsrc->Register.Index;
      case TGSI_FILE_OUTPUT:
         default:
      NOUVEAU_ERR("bad src file\n");
                  dst  = tgsi_dst(fpc, &finst->Dst[0]);
   mask = tgsi_mask(finst->Dst[0].Register.WriteMask);
            switch (finst->Instruction.Opcode) {
   case TGSI_OPCODE_ADD:
      nvfx_fp_emit(fpc, arith(sat, ADD, dst, mask, src[0], src[1], none));
      case TGSI_OPCODE_CEIL:
      tmp = nvfx_src(temp(fpc));
   nvfx_fp_emit(fpc, arith(0, FLR, tmp.reg, mask, neg(src[0]), none, none));
   nvfx_fp_emit(fpc, arith(sat, MOV, dst, mask, neg(tmp), none, none));
      case TGSI_OPCODE_CMP:
      insn = arith(0, MOV, none.reg, mask, src[0], none, none);
   insn.cc_update = 1;
            insn = arith(sat, MOV, dst, mask, src[2], none, none);
   insn.cc_test = NVFX_COND_GE;
            insn = arith(sat, MOV, dst, mask, src[1], none, none);
   insn.cc_test = NVFX_COND_LT;
   nvfx_fp_emit(fpc, insn);
      case TGSI_OPCODE_COS:
      nvfx_fp_emit(fpc, arith(sat, COS, dst, mask, src[0], none, none));
      case TGSI_OPCODE_DDX:
      if (mask & (NVFX_FP_MASK_Z | NVFX_FP_MASK_W)) {
      tmp = nvfx_src(temp(fpc));
   nvfx_fp_emit(fpc, arith(sat, DDX, tmp.reg, NVFX_FP_MASK_X | NVFX_FP_MASK_Y, swz(src[0], Z, W, Z, W), none, none));
   nvfx_fp_emit(fpc, arith(0, MOV, tmp.reg, NVFX_FP_MASK_Z | NVFX_FP_MASK_W, swz(tmp, X, Y, X, Y), none, none));
   nvfx_fp_emit(fpc, arith(sat, DDX, tmp.reg, NVFX_FP_MASK_X | NVFX_FP_MASK_Y, src[0], none, none));
      } else {
         }
      case TGSI_OPCODE_DDY:
      if (mask & (NVFX_FP_MASK_Z | NVFX_FP_MASK_W)) {
      tmp = nvfx_src(temp(fpc));
   nvfx_fp_emit(fpc, arith(sat, DDY, tmp.reg, NVFX_FP_MASK_X | NVFX_FP_MASK_Y, swz(src[0], Z, W, Z, W), none, none));
   nvfx_fp_emit(fpc, arith(0, MOV, tmp.reg, NVFX_FP_MASK_Z | NVFX_FP_MASK_W, swz(tmp, X, Y, X, Y), none, none));
   nvfx_fp_emit(fpc, arith(sat, DDY, tmp.reg, NVFX_FP_MASK_X | NVFX_FP_MASK_Y, src[0], none, none));
      } else {
         }
      case TGSI_OPCODE_DP2:
      tmp = nvfx_src(temp(fpc));
   nvfx_fp_emit(fpc, arith(0, MUL, tmp.reg, NVFX_FP_MASK_X | NVFX_FP_MASK_Y, src[0], src[1], none));
   nvfx_fp_emit(fpc, arith(0, ADD, dst, mask, swz(tmp, X, X, X, X), swz(tmp, Y, Y, Y, Y), none));
      case TGSI_OPCODE_DP3:
      nvfx_fp_emit(fpc, arith(sat, DP3, dst, mask, src[0], src[1], none));
      case TGSI_OPCODE_DP4:
      nvfx_fp_emit(fpc, arith(sat, DP4, dst, mask, src[0], src[1], none));
      case TGSI_OPCODE_DST:
      nvfx_fp_emit(fpc, arith(sat, DST, dst, mask, src[0], src[1], none));
      case TGSI_OPCODE_EX2:
      nvfx_fp_emit(fpc, arith(sat, EX2, dst, mask, src[0], none, none));
      case TGSI_OPCODE_FLR:
      nvfx_fp_emit(fpc, arith(sat, FLR, dst, mask, src[0], none, none));
      case TGSI_OPCODE_FRC:
      nvfx_fp_emit(fpc, arith(sat, FRC, dst, mask, src[0], none, none));
      case TGSI_OPCODE_KILL:
      nvfx_fp_emit(fpc, arith(0, KIL, none.reg, 0, none, none, none));
      case TGSI_OPCODE_KILL_IF:
      insn = arith(0, MOV, none.reg, NVFX_FP_MASK_ALL, src[0], none, none);
   insn.cc_update = 1;
            insn = arith(0, KIL, none.reg, 0, none, none, none);
   insn.cc_test = NVFX_COND_LT;
   nvfx_fp_emit(fpc, insn);
      case TGSI_OPCODE_LG2:
      nvfx_fp_emit(fpc, arith(sat, LG2, dst, mask, src[0], none, none));
      case TGSI_OPCODE_LIT:
      if(!fpc->is_nv4x)
         else {
      /* we use FLT_MIN, so that log2 never gives -infinity, and thus multiplication by
   * specular 0 always gives 0, so that ex2 gives 1, to satisfy the 0^0 = 1 requirement
   *
   * NOTE: if we start using half precision, we might need an fp16 FLT_MIN here instead
   */
   struct nvfx_src maxs = nvfx_src(nvfx_fp_imm(fpc, 0, FLT_MIN, 0, 0));
   tmp = nvfx_src(temp(fpc));
   if (ci>= 0 || ii >= 0) {
      nvfx_fp_emit(fpc, arith(0, MOV, tmp.reg, NVFX_FP_MASK_X | NVFX_FP_MASK_Y, maxs, none, none));
      }
   nvfx_fp_emit(fpc, arith(0, MAX, tmp.reg, NVFX_FP_MASK_Y | NVFX_FP_MASK_W, swz(src[0], X, X, X, Y), swz(maxs, X, X, Y, Y), none));
   nvfx_fp_emit(fpc, arith(0, LG2, tmp.reg, NVFX_FP_MASK_W, swz(tmp, W, W, W, W), none, none));
   nvfx_fp_emit(fpc, arith(0, MUL, tmp.reg, NVFX_FP_MASK_W, swz(tmp, W, W, W, W), swz(src[0], W, W, W, W), none));
      }
      case TGSI_OPCODE_LRP:
      if(!fpc->is_nv4x)
         else {
      tmp = nvfx_src(temp(fpc));
   nvfx_fp_emit(fpc, arith(0, MAD, tmp.reg, mask, neg(src[0]), src[2], src[2]));
      }
      case TGSI_OPCODE_MAD:
      nvfx_fp_emit(fpc, arith(sat, MAD, dst, mask, src[0], src[1], src[2]));
      case TGSI_OPCODE_MAX:
      nvfx_fp_emit(fpc, arith(sat, MAX, dst, mask, src[0], src[1], none));
      case TGSI_OPCODE_MIN:
      nvfx_fp_emit(fpc, arith(sat, MIN, dst, mask, src[0], src[1], none));
      case TGSI_OPCODE_MOV:
      nvfx_fp_emit(fpc, arith(sat, MOV, dst, mask, src[0], none, none));
      case TGSI_OPCODE_MUL:
      nvfx_fp_emit(fpc, arith(sat, MUL, dst, mask, src[0], src[1], none));
      case TGSI_OPCODE_NOP:
         case TGSI_OPCODE_POW:
      if(!fpc->is_nv4x)
         else {
      tmp = nvfx_src(temp(fpc));
   nvfx_fp_emit(fpc, arith(0, LG2, tmp.reg, NVFX_FP_MASK_X, swz(src[0], X, X, X, X), none, none));
   nvfx_fp_emit(fpc, arith(0, MUL, tmp.reg, NVFX_FP_MASK_X, swz(tmp, X, X, X, X), swz(src[1], X, X, X, X), none));
      }
      case TGSI_OPCODE_RCP:
      nvfx_fp_emit(fpc, arith(sat, RCP, dst, mask, src[0], none, none));
      case TGSI_OPCODE_RSQ:
      if(!fpc->is_nv4x)
         else {
      tmp = nvfx_src(temp(fpc));
   insn = arith(0, LG2, tmp.reg, NVFX_FP_MASK_X, abs(swz(src[0], X, X, X, X)), none, none);
   insn.scale = NVFX_FP_OP_DST_SCALE_INV_2X;
   nvfx_fp_emit(fpc, insn);
      }
      case TGSI_OPCODE_SEQ:
      nvfx_fp_emit(fpc, arith(sat, SEQ, dst, mask, src[0], src[1], none));
      case TGSI_OPCODE_SGE:
      nvfx_fp_emit(fpc, arith(sat, SGE, dst, mask, src[0], src[1], none));
      case TGSI_OPCODE_SGT:
      nvfx_fp_emit(fpc, arith(sat, SGT, dst, mask, src[0], src[1], none));
      case TGSI_OPCODE_SIN:
      nvfx_fp_emit(fpc, arith(sat, SIN, dst, mask, src[0], none, none));
      case TGSI_OPCODE_SLE:
      nvfx_fp_emit(fpc, arith(sat, SLE, dst, mask, src[0], src[1], none));
      case TGSI_OPCODE_SLT:
      nvfx_fp_emit(fpc, arith(sat, SLT, dst, mask, src[0], src[1], none));
      case TGSI_OPCODE_SNE:
      nvfx_fp_emit(fpc, arith(sat, SNE, dst, mask, src[0], src[1], none));
      case TGSI_OPCODE_SSG:
   {
               insn = arith(sat, MOV, dst, mask, src[0], none, none);
   insn.cc_update = 1;
            insn = arith(0, STR, dst, mask, none, none, none);
   insn.cc_test = NVFX_COND_GT;
            if(!sat) {
      insn = arith(0, MOV, dst, mask, minones, none, none);
   insn.cc_test = NVFX_COND_LT;
      }
      }
   case TGSI_OPCODE_TEX:
      nvfx_fp_emit(fpc, tex(sat, TEX, unit, dst, mask, src[0], none, none));
   break;
   case TGSI_OPCODE_TRUNC:
            tmp = nvfx_src(temp(fpc));
                                       insn = arith(sat, MOV, dst, mask, neg(tmp), none, none);
   insn.cc_test = NVFX_COND_LT;
      case TGSI_OPCODE_TXB:
               case TGSI_OPCODE_TXL:
            if(fpc->is_nv4x)
         else /* unsupported on nv30, use TEX and hope they like it */
      case TGSI_OPCODE_TXP:
               case TGSI_OPCODE_IF:
      // MOVRC0 R31 (TR0.xyzw), R<src>:
   // IF (NE.xxxx) ELSE <else> END <end>
   if(!fpc->is_nv4x)
         nv40_fp_if(fpc, src[0]);
         case TGSI_OPCODE_ELSE:
   {
      uint32_t *hw;
   if(!fpc->is_nv4x)
         assert(util_dynarray_contains(&fpc->if_stack, unsigned));
   hw = &fpc->fp->insn[util_dynarray_top(&fpc->if_stack, unsigned)];
   hw[2] = NV40_FP_OP_OPCODE_IS_BRANCH | fpc->fp->insn_len;
               case TGSI_OPCODE_ENDIF:
   {
      uint32_t *hw;
   if(!fpc->is_nv4x)
         assert(util_dynarray_contains(&fpc->if_stack, unsigned));
   hw = &fpc->fp->insn[util_dynarray_pop(&fpc->if_stack, unsigned)];
   if(!hw[2])
         hw[3] = fpc->fp->insn_len;
               case TGSI_OPCODE_BGNSUB:
   case TGSI_OPCODE_ENDSUB:
      /* nothing to do here */
         case TGSI_OPCODE_CAL:
      if(!fpc->is_nv4x)
         nv40_fp_cal(fpc, finst->Label.Label);
         case TGSI_OPCODE_RET:
      if(!fpc->is_nv4x)
         nv40_fp_ret(fpc);
         case TGSI_OPCODE_BGNLOOP:
      if(!fpc->is_nv4x)
         /* TODO: we should support using two nested REPs to allow a > 255 iteration count */
   nv40_fp_rep(fpc, 255, finst->Label.Label);
         case TGSI_OPCODE_ENDLOOP:
            case TGSI_OPCODE_BRK:
      if(!fpc->is_nv4x)
         nv40_fp_brk(fpc);
         case TGSI_OPCODE_CONT:
   {
      static int warned = 0;
   if(!warned) {
      NOUVEAU_ERR("Sorry, the continue keyword is not implemented: ignoring it.\n");
      }
                  default:
   NOUVEAU_ERR("invalid opcode %d\n", finst->Instruction.Opcode);
            out:
      release_temps(fpc);
      nv3x_cflow:
      {
      static int warned = 0;
   if(!warned) {
      NOUVEAU_ERR(
         "Sorry, control flow instructions are not supported in hardware on nv3x: ignoring them\n"
         }
      }
      static bool
   nvfx_fragprog_parse_decl_input(struct nvfx_fpc *fpc,
         {
      unsigned idx = fdec->Range.First;
            switch (fdec->Semantic.Name) {
   case TGSI_SEMANTIC_POSITION:
      hw = NVFX_FP_OP_INPUT_SRC_POSITION;
      case TGSI_SEMANTIC_COLOR:
      hw = NVFX_FP_OP_INPUT_SRC_COL0 + fdec->Semantic.Index;
      case TGSI_SEMANTIC_FOG:
      hw = NVFX_FP_OP_INPUT_SRC_FOGC;
      case TGSI_SEMANTIC_FACE:
      hw = NV40_FP_OP_INPUT_SRC_FACING;
      case TGSI_SEMANTIC_TEXCOORD:
      assert(fdec->Semantic.Index < 8);
   fpc->fp->texcoord[fdec->Semantic.Index] = fdec->Semantic.Index;
   fpc->fp->texcoords |= (1 << fdec->Semantic.Index);
   fpc->fp->vp_or |= (0x00004000 << fdec->Semantic.Index);
   hw = NVFX_FP_OP_INPUT_SRC_TC(fdec->Semantic.Index);
      case TGSI_SEMANTIC_GENERIC:
   case TGSI_SEMANTIC_PCOORD:
      /* will be assigned to remaining TC slots later */
      default:
      assert(0);
               fpc->r_input[idx] = nvfx_reg(NVFXSR_INPUT, hw);
      }
      static bool
   nvfx_fragprog_assign_generic(struct nvfx_fpc *fpc,
         {
      unsigned num_texcoords = fpc->is_nv4x ? 10 : 8;
   unsigned idx = fdec->Range.First;
            switch (fdec->Semantic.Name) {
   case TGSI_SEMANTIC_GENERIC:
   case TGSI_SEMANTIC_PCOORD:
      for (hw = 0; hw < num_texcoords; hw++) {
      if (fpc->fp->texcoord[hw] == 0xffff) {
      if (hw <= 7) {
      fpc->fp->texcoords |= (0x1 << hw);
      } else {
         }
   if (fdec->Semantic.Name == TGSI_SEMANTIC_PCOORD) {
      fpc->fp->texcoord[hw] = 0xfffe;
      } else {
         }
   hw = NVFX_FP_OP_INPUT_SRC_TC(hw);
   fpc->r_input[idx] = nvfx_reg(NVFXSR_INPUT, hw);
         }
      default:
            }
      static bool
   nvfx_fragprog_parse_decl_output(struct nvfx_fpc *fpc,
         {
      unsigned idx = fdec->Range.First;
            switch (fdec->Semantic.Name) {
   case TGSI_SEMANTIC_POSITION:
      hw = 1;
      case TGSI_SEMANTIC_COLOR:
      hw = ~0;
   switch (fdec->Semantic.Index) {
   case 0: hw = 0; break;
   case 1: hw = 2; break;
   case 2: hw = 3; break;
   case 3: hw = 4; break;
   }
   if(hw > ((fpc->is_nv4x) ? 4 : 2)) {
      NOUVEAU_ERR("bad rcol index\n");
      }
      default:
      NOUVEAU_ERR("bad output semantic\n");
               fpc->r_result[idx] = nvfx_reg(NVFXSR_OUTPUT, hw);
   fpc->r_temps |= (1ULL << hw);
      }
      static bool
   nvfx_fragprog_prepare(struct nvfx_fpc *fpc)
   {
      struct tgsi_parse_context p;
                     tgsi_parse_init(&p, fpc->fp->pipe.tokens);
   while (!tgsi_parse_end_of_tokens(&p)) {
               tgsi_parse_token(&p);
   switch(tok->Token.Type) {
   case TGSI_TOKEN_TYPE_DECLARATION:
   {
      const struct tgsi_full_declaration *fdec;
   fdec = &p.FullToken.FullDeclaration;
   switch (fdec->Declaration.File) {
   case TGSI_FILE_INPUT:
      if (!nvfx_fragprog_parse_decl_input(fpc, fdec))
            case TGSI_FILE_OUTPUT:
      if (!nvfx_fragprog_parse_decl_output(fpc, fdec))
            case TGSI_FILE_TEMPORARY:
      if (fdec->Range.Last > high_temp) {
      high_temp =
      }
      default:
            }
         case TGSI_TOKEN_TYPE_IMMEDIATE:
   {
               imm = &p.FullToken.FullImmediate;
                  fpc->r_imm[fpc->nr_imm++] = nvfx_fp_imm(fpc, imm->u[0].Float, imm->u[1].Float, imm->u[2].Float, imm->u[3].Float);
      }
   default:
            }
            tgsi_parse_init(&p, fpc->fp->pipe.tokens);
   while (!tgsi_parse_end_of_tokens(&p)) {
      const struct tgsi_full_declaration *fdec;
   tgsi_parse_token(&p);
   switch(p.FullToken.Token.Type) {
   case TGSI_TOKEN_TYPE_DECLARATION:
      fdec = &p.FullToken.FullDeclaration;
   switch (fdec->Declaration.File) {
   case TGSI_FILE_INPUT:
      if (!nvfx_fragprog_assign_generic(fpc, fdec))
            default:
         }
      default:
            }
            if (++high_temp) {
      fpc->r_temp = CALLOC(high_temp, sizeof(struct nvfx_reg));
   for (i = 0; i < high_temp; i++)
                           out_err:
      FREE(fpc->r_temp);
            tgsi_parse_free(&p);
      }
      DEBUG_GET_ONCE_BOOL_OPTION(nvfx_dump_fp, "NVFX_DUMP_FP", false)
      void
   _nvfx_fragprog_translate(uint16_t oclass, struct nv30_fragprog *fp)
   {
      struct tgsi_parse_context parse;
   struct nvfx_fpc *fpc = NULL;
            fp->translated = false;
   fp->point_sprite_control = 0;
            fpc = CALLOC_STRUCT(nvfx_fpc);
   if (!fpc)
            fpc->is_nv4x = (oclass >= NV40_3D_CLASS) ? ~0 : 0;
   fpc->max_temps = fpc->is_nv4x ? 48 : 32;
   fpc->fp = fp;
   fpc->num_regs = 2;
            if (fp->info.properties[TGSI_PROPERTY_FS_COORD_ORIGIN])
         if (fp->info.properties[TGSI_PROPERTY_FS_COORD_PIXEL_CENTER])
         if (fp->info.properties[TGSI_PROPERTY_FS_COLOR0_WRITES_ALL_CBUFS])
            if (!nvfx_fragprog_prepare(fpc))
            tgsi_parse_init(&parse, fp->pipe.tokens);
            while (!tgsi_parse_end_of_tokens(&parse)) {
               switch (parse.FullToken.Token.Type) {
   case TGSI_TOKEN_TYPE_INSTRUCTION:
   {
               util_dynarray_append(&insns, unsigned, fp->insn_len);
   finst = &parse.FullToken.FullInstruction;
   if (!nvfx_fragprog_parse_instruction(fpc, finst))
      }
         default:
            }
            for(unsigned i = 0; i < fpc->label_relocs.size; i += sizeof(struct nvfx_relocation))
   {
      struct nvfx_relocation* label_reloc = (struct nvfx_relocation*)((char*)fpc->label_relocs.data + i);
      }
            if(!fpc->is_nv4x)
         else
            /* Terminate final instruction */
   if(fp->insn)
            /* Append NOP + END instruction for branches to the end of the program */
   fpc->inst_offset = fp->insn_len;
   grow_insns(fpc, 4);
   fp->insn[fpc->inst_offset + 0] = 0x00000001;
   fp->insn[fpc->inst_offset + 1] = 0x00000000;
   fp->insn[fpc->inst_offset + 2] = 0x00000000;
            if(debug_get_option_nvfx_dump_fp())
   {
      debug_printf("\n");
            debug_printf("\n%s fragment program:\n", fpc->is_nv4x ? "nv4x" : "nv3x");
   for (unsigned i = 0; i < fp->insn_len; i += 4)
                           out:
      tgsi_parse_free(&parse);
   if (fpc)
   {
      FREE(fpc->r_temp);
   FREE(fpc->r_imm);
   util_dynarray_fini(&fpc->if_stack);
   util_dynarray_fini(&fpc->label_relocs);
   util_dynarray_fini(&fpc->imm_data);
   //util_dynarray_fini(&fpc->loop_stack);
                     out_err:
      _debug_printf("Error: failed to compile this fragment program:\n");
   tgsi_dump(fp->pipe.tokens, 0);
      }
