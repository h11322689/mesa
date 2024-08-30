   /**************************************************************************
   *
   * Copyright 2007 VMware, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      #include <stdarg.h>
      #include "i915_context.h"
   #include "i915_debug.h"
   #include "i915_debug_private.h"
   #include "i915_fpc.h"
   #include "i915_reg.h"
      #include "nir/nir.h"
   #include "pipe/p_shader_tokens.h"
   #include "tgsi/tgsi_dump.h"
   #include "tgsi/tgsi_info.h"
   #include "tgsi/tgsi_parse.h"
   #include "util/log.h"
   #include "util/ralloc.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/u_string.h"
      #include "draw/draw_vertex.h"
      #ifndef M_PI
   #define M_PI 3.14159265358979323846
   #endif
      /**
   * Simple pass-through fragment shader to use when we don't have
   * a real shader (or it fails to compile for some reason).
   */
   static unsigned passthrough_program[] = {
      _3DSTATE_PIXEL_SHADER_PROGRAM | ((1 * 3) - 1),
   /* move to output color:
   */
   (A0_MOV | (REG_TYPE_OC << A0_DEST_TYPE_SHIFT) | A0_DEST_CHANNEL_ALL |
   (REG_TYPE_R << A0_SRC0_TYPE_SHIFT) | (0 << A0_SRC0_NR_SHIFT)),
   ((SRC_ONE << A1_SRC0_CHANNEL_X_SHIFT) |
   (SRC_ZERO << A1_SRC0_CHANNEL_Y_SHIFT) |
   (SRC_ZERO << A1_SRC0_CHANNEL_Z_SHIFT) |
   (SRC_ONE << A1_SRC0_CHANNEL_W_SHIFT)),
         /**
   * component-wise negation of ureg
   */
   static inline int
   negate(int reg, int x, int y, int z, int w)
   {
      /* Another neat thing about the UREG representation */
   return reg ^ (((x & 1) << UREG_CHANNEL_X_NEGATE_SHIFT) |
                  }
      /**
   * In the event of a translation failure, we'll generate a simple color
   * pass-through program.
   */
   static void
   i915_use_passthrough_shader(struct i915_fragment_shader *fs)
   {
      fs->program = (uint32_t *)MALLOC(sizeof(passthrough_program));
   if (fs->program) {
      memcpy(fs->program, passthrough_program, sizeof(passthrough_program));
      }
      }
      void
   i915_program_error(struct i915_fp_compile *p, const char *msg, ...)
   {
      va_list args;
   va_start(args, msg);
   ralloc_vasprintf_append(&p->error, msg, args);
      }
      static uint32_t
   get_mapping(struct i915_fragment_shader *fs, enum tgsi_semantic semantic,
         {
      int i;
   for (i = 0; i < I915_TEX_UNITS; i++) {
      if (fs->texcoords[i].semantic == -1) {
      fs->texcoords[i].semantic = semantic;
   fs->texcoords[i].index = index;
      }
   if (fs->texcoords[i].semantic == semantic &&
      fs->texcoords[i].index == index)
   }
   debug_printf("Exceeded max generics\n");
      }
      /**
   * Construct a ureg for the given source register.  Will emit
   * constants, apply swizzling and negation as needed.
   */
   static uint32_t
   src_vector(struct i915_fp_compile *p,
               {
      uint32_t index = source->Register.Index;
            switch (source->Register.File) {
   case TGSI_FILE_TEMPORARY:
      if (source->Register.Index >= I915_MAX_TEMPORARY) {
      i915_program_error(p, "Exceeded max temporary reg");
      }
   src = UREG(REG_TYPE_R, index);
      case TGSI_FILE_INPUT:
      /* XXX: Packing COL1, FOGC into a single attribute works for
   * texenv programs, but will fail for real fragment programs
   * that use these attributes and expect them to be a full 4
   * components wide.  Could use a texcoord to pass these
   * attributes if necessary, but that won't work in the general
   * case.
   *
   * We also use a texture coordinate to pass wpos when possible.
            sem_name = p->shader->info.input_semantic_name[index];
            switch (sem_name) {
   case TGSI_SEMANTIC_GENERIC:
   case TGSI_SEMANTIC_TEXCOORD:
   case TGSI_SEMANTIC_PCOORD:
   case TGSI_SEMANTIC_POSITION: {
                     int real_tex_unit = get_mapping(fs, sem_name, sem_ind);
   src = i915_emit_decl(p, REG_TYPE_T, T_TEX0 + real_tex_unit,
            }
   case TGSI_SEMANTIC_COLOR:
      if (sem_ind == 0) {
         } else {
      /* secondary color */
   assert(sem_ind == 1);
   src = i915_emit_decl(p, REG_TYPE_T, T_SPECULAR, D0_CHANNEL_XYZ);
      }
      case TGSI_SEMANTIC_FOG:
      src = i915_emit_decl(p, REG_TYPE_T, T_FOG_W, D0_CHANNEL_W);
   src = swizzle(src, W, W, W, W);
      case TGSI_SEMANTIC_FACE: {
      /* for back/front faces */
   int real_tex_unit = get_mapping(fs, sem_name, sem_ind);
   src =
            }
   default:
      i915_program_error(p, "Bad source->Index");
      }
         case TGSI_FILE_IMMEDIATE: {
               uint8_t swiz[4] = {source->Register.SwizzleX, source->Register.SwizzleY,
            uint8_t neg[4] = {source->Register.Negate, source->Register.Negate,
                     for (i = 0; i < 4; i++) {
      if (swiz[i] == TGSI_SWIZZLE_ZERO || swiz[i] == TGSI_SWIZZLE_ONE) {
         } else if (p->immediates[index][swiz[i]] == 0.0) {
         } else if (p->immediates[index][swiz[i]] == 1.0) {
         } else if (p->immediates[index][swiz[i]] == -1.0) {
      swiz[i] = TGSI_SWIZZLE_ONE;
      } else {
                     if (i == 4) {
      return negate(
      swizzle(UREG(REG_TYPE_R, 0), swiz[0], swiz[1], swiz[2], swiz[3]),
            index = p->immediates_map[index];
               case TGSI_FILE_CONSTANT:
      src = UREG(REG_TYPE_CONST, index);
         default:
      i915_program_error(p, "Bad source->File");
               src = swizzle(src, source->Register.SwizzleX, source->Register.SwizzleY,
            /* No HW abs flag, so we have to max with the negation. */
   if (source->Register.Absolute) {
      uint32_t tmp = i915_get_utemp(p);
   i915_emit_arith(p, A0_MAX, tmp, A0_DEST_CHANNEL_ALL, 0, src,
                     /* There's both negate-all-components and per-component negation.
   * Try to handle both here.
   */
   {
      int n = source->Register.Negate;
                  }
      /**
   * Construct a ureg for a destination register.
   */
   static uint32_t
   get_result_vector(struct i915_fp_compile *p,
         {
      switch (dest->Register.File) {
   case TGSI_FILE_OUTPUT: {
      uint32_t sem_name =
         switch (sem_name) {
   case TGSI_SEMANTIC_POSITION:
         case TGSI_SEMANTIC_COLOR:
         default:
      i915_program_error(p, "Bad inst->DstReg.Index/semantics");
         }
   case TGSI_FILE_TEMPORARY:
         default:
      i915_program_error(p, "Bad inst->DstReg.File");
         }
      /**
   * Compute flags for saturation and writemask.
   */
   static uint32_t
   get_result_flags(const struct i915_full_instruction *inst)
   {
      const uint32_t writeMask = inst->Dst[0].Register.WriteMask;
            if (inst->Instruction.Saturate)
            if (writeMask & TGSI_WRITEMASK_X)
         if (writeMask & TGSI_WRITEMASK_Y)
         if (writeMask & TGSI_WRITEMASK_Z)
         if (writeMask & TGSI_WRITEMASK_W)
               }
      /**
   * Convert TGSI_TEXTURE_x token to DO_SAMPLE_TYPE_x token
   */
   static uint32_t
   translate_tex_src_target(struct i915_fp_compile *p, uint32_t tex)
   {
      switch (tex) {
   case TGSI_TEXTURE_SHADOW1D:
         case TGSI_TEXTURE_1D:
            case TGSI_TEXTURE_SHADOW2D:
         case TGSI_TEXTURE_2D:
            case TGSI_TEXTURE_SHADOWRECT:
         case TGSI_TEXTURE_RECT:
            case TGSI_TEXTURE_3D:
            case TGSI_TEXTURE_CUBE:
            default:
      i915_program_error(p, "TexSrc type");
         }
      /**
   * Return the number of coords needed to access a given TGSI_TEXTURE_*
   */
   uint32_t
   i915_coord_mask(enum tgsi_opcode opcode, enum tgsi_texture_type tex)
   {
               if (opcode == TGSI_OPCODE_TXP || opcode == TGSI_OPCODE_TXB)
            switch (tex) {
   case TGSI_TEXTURE_1D: /* See the 1D coord swizzle below. */
   case TGSI_TEXTURE_2D:
   case TGSI_TEXTURE_RECT:
            case TGSI_TEXTURE_SHADOW1D:
   case TGSI_TEXTURE_SHADOW2D:
   case TGSI_TEXTURE_SHADOWRECT:
   case TGSI_TEXTURE_3D:
   case TGSI_TEXTURE_CUBE:
            default:
            }
      /**
   * Generate texel lookup instruction.
   */
   static void
   emit_tex(struct i915_fp_compile *p, const struct i915_full_instruction *inst,
         {
      uint32_t texture = inst->Texture.Texture;
   uint32_t unit = inst->Src[1].Register.Index;
   uint32_t tex = translate_tex_src_target(p, texture);
   uint32_t sampler = i915_emit_decl(p, REG_TYPE_S, unit, tex);
            /* For 1D textures, set the Y coord to the same as X.  Otherwise, we could
   * select the wrong LOD based on the uninitialized Y coord when we sample our
   * 1D textures as 2D.
   */
   if (texture == TGSI_TEXTURE_1D || texture == TGSI_TEXTURE_SHADOW1D)
            i915_emit_texld(p, get_result_vector(p, &inst->Dst[0]),
            }
      /**
   * Generate a simple arithmetic instruction
   * \param opcode  the i915 opcode
   * \param numArgs  the number of input/src arguments
   */
   static void
   emit_simple_arith(struct i915_fp_compile *p,
               {
                        arg1 = (numArgs < 1) ? 0 : src_vector(p, &inst->Src[0], fs);
   arg2 = (numArgs < 2) ? 0 : src_vector(p, &inst->Src[1], fs);
            i915_emit_arith(p, opcode, get_result_vector(p, &inst->Dst[0]),
      }
      /** As above, but swap the first two src regs */
   static void
   emit_simple_arith_swap2(struct i915_fp_compile *p,
                     {
                        /* transpose first two registers */
   inst2 = *inst;
   inst2.Src[0] = inst->Src[1];
               }
      /*
   * Translate TGSI instruction to i915 instruction.
   *
   * Possible concerns:
   *
   * DDX, DDY -- return 0
   * SIN, COS -- could use another taylor step?
   * LIT      -- results seem a little different to sw mesa
   * LOG      -- different to mesa on negative numbers, but this is conformant.
   */
   static void
   i915_translate_instruction(struct i915_fp_compile *p,
               {
      uint32_t src0, src1, src2, flags;
            switch (inst->Instruction.Opcode) {
   case TGSI_OPCODE_ADD:
      emit_simple_arith(p, inst, A0_ADD, 2, fs);
         case TGSI_OPCODE_CEIL:
      src0 = src_vector(p, &inst->Src[0], fs);
   tmp = i915_get_utemp(p);
   flags = get_result_flags(inst);
   i915_emit_arith(p, A0_FLR, tmp, flags & A0_DEST_CHANNEL_ALL, 0,
         i915_emit_arith(p, A0_MOV, get_result_vector(p, &inst->Dst[0]), flags, 0,
               case TGSI_OPCODE_CMP:
      src0 = src_vector(p, &inst->Src[0], fs);
   src1 = src_vector(p, &inst->Src[1], fs);
   src2 = src_vector(p, &inst->Src[2], fs);
   i915_emit_arith(p, A0_CMP, get_result_vector(p, &inst->Dst[0]),
                     case TGSI_OPCODE_DDX:
   case TGSI_OPCODE_DDY:
      /* XXX We just output 0 here */
   debug_printf("Punting DDX/DDY\n");
   src0 = get_result_vector(p, &inst->Dst[0]);
   i915_emit_arith(p, A0_MOV, get_result_vector(p, &inst->Dst[0]),
                     case TGSI_OPCODE_DP2:
      src0 = src_vector(p, &inst->Src[0], fs);
            i915_emit_arith(p, A0_DP3, get_result_vector(p, &inst->Dst[0]),
                     case TGSI_OPCODE_DP3:
      emit_simple_arith(p, inst, A0_DP3, 2, fs);
         case TGSI_OPCODE_DP4:
      emit_simple_arith(p, inst, A0_DP4, 2, fs);
         case TGSI_OPCODE_DST:
      src0 = src_vector(p, &inst->Src[0], fs);
            /* result[0] = 1    * 1;
   * result[1] = a[1] * b[1];
   * result[2] = a[2] * 1;
   * result[3] = 1    * b[3];
   */
   i915_emit_arith(p, A0_MUL, get_result_vector(p, &inst->Dst[0]),
                     case TGSI_OPCODE_END:
      /* no-op */
         case TGSI_OPCODE_EX2:
               i915_emit_arith(p, A0_EXP, get_result_vector(p, &inst->Dst[0]),
                     case TGSI_OPCODE_FLR:
      emit_simple_arith(p, inst, A0_FLR, 1, fs);
         case TGSI_OPCODE_FRC:
      emit_simple_arith(p, inst, A0_FRC, 1, fs);
         case TGSI_OPCODE_KILL_IF:
      /* kill if src[0].x < 0 || src[0].y < 0 ... */
   src0 = src_vector(p, &inst->Src[0], fs);
            i915_emit_texld(p, tmp,               /* dest reg: a dummy reg */
                  A0_DEST_CHANNEL_ALL,  /* dest writemask */
   0,                    /* sampler */
            case TGSI_OPCODE_KILL:
      /* unconditional kill */
            i915_emit_texld(p, tmp,              /* dest reg: a dummy reg */
                  A0_DEST_CHANNEL_ALL, /* dest writemask */
   0,                   /* sampler */
   negate(swizzle(UREG(REG_TYPE_R, 0), ONE, ONE, ONE, ONE),
            case TGSI_OPCODE_LG2:
               i915_emit_arith(p, A0_LOG, get_result_vector(p, &inst->Dst[0]),
                     case TGSI_OPCODE_LIT:
      src0 = src_vector(p, &inst->Src[0], fs);
            /* tmp = max( a.xyzw, a.00zw )
   * XXX: Clamp tmp.w to -128..128
   * tmp.y = log(tmp.y)
   * tmp.y = tmp.w * tmp.y
   * tmp.y = exp(tmp.y)
   * result = cmp (a.11-x1, a.1x01, a.1xy1 )
   */
   i915_emit_arith(p, A0_MAX, tmp, A0_DEST_CHANNEL_ALL, 0, src0,
            i915_emit_arith(p, A0_LOG, tmp, A0_DEST_CHANNEL_Y, 0,
            i915_emit_arith(p, A0_MUL, tmp, A0_DEST_CHANNEL_Y, 0,
                  i915_emit_arith(p, A0_EXP, tmp, A0_DEST_CHANNEL_Y, 0,
            i915_emit_arith(
      p, A0_CMP, get_result_vector(p, &inst->Dst[0]), get_result_flags(inst),
                     case TGSI_OPCODE_LRP:
      src0 = src_vector(p, &inst->Src[0], fs);
   src1 = src_vector(p, &inst->Src[1], fs);
   src2 = src_vector(p, &inst->Src[2], fs);
   flags = get_result_flags(inst);
            /* b*a + c*(1-a)
   *
   * b*a + c - ca
   *
   * tmp = b*a + c,
   * result = (-c)*a + tmp
   */
   i915_emit_arith(p, A0_MAD, tmp, flags & A0_DEST_CHANNEL_ALL, 0, src1,
            i915_emit_arith(p, A0_MAD, get_result_vector(p, &inst->Dst[0]), flags, 0,
               case TGSI_OPCODE_MAD:
      emit_simple_arith(p, inst, A0_MAD, 3, fs);
         case TGSI_OPCODE_MAX:
      emit_simple_arith(p, inst, A0_MAX, 2, fs);
         case TGSI_OPCODE_MIN:
      emit_simple_arith(p, inst, A0_MIN, 2, fs);
         case TGSI_OPCODE_MOV:
      emit_simple_arith(p, inst, A0_MOV, 1, fs);
         case TGSI_OPCODE_MUL:
      emit_simple_arith(p, inst, A0_MUL, 2, fs);
         case TGSI_OPCODE_NOP:
            case TGSI_OPCODE_POW:
      src0 = src_vector(p, &inst->Src[0], fs);
   src1 = src_vector(p, &inst->Src[1], fs);
   tmp = i915_get_utemp(p);
            /* XXX: masking on intermediate values, here and elsewhere.
   */
   i915_emit_arith(p, A0_LOG, tmp, A0_DEST_CHANNEL_X, 0,
                     i915_emit_arith(p, A0_EXP, get_result_vector(p, &inst->Dst[0]), flags, 0,
               case TGSI_OPCODE_RET:
      /* XXX: no-op? */
         case TGSI_OPCODE_RCP:
               i915_emit_arith(p, A0_RCP, get_result_vector(p, &inst->Dst[0]),
                     case TGSI_OPCODE_RSQ:
               i915_emit_arith(p, A0_RSQ, get_result_vector(p, &inst->Dst[0]),
                     case TGSI_OPCODE_SEQ: {
      const uint32_t zero =
            /* if we're both >= and <= then we're == */
   src0 = src_vector(p, &inst->Src[0], fs);
   src1 = src_vector(p, &inst->Src[1], fs);
            if (src0 == zero || src1 == zero) {
                     /* x == 0 is equivalent to -abs(x) >= 0, but the latter requires only
   * two instructions instead of three.
   */
   i915_emit_arith(p, A0_MAX, tmp, A0_DEST_CHANNEL_ALL, 0, src0,
         i915_emit_arith(p, A0_SGE, get_result_vector(p, &inst->Dst[0]),
            } else {
                              i915_emit_arith(p, A0_MUL, get_result_vector(p, &inst->Dst[0]),
                                 case TGSI_OPCODE_SGE:
      emit_simple_arith(p, inst, A0_SGE, 2, fs);
         case TGSI_OPCODE_SLE:
      /* like SGE, but swap reg0, reg1 */
   emit_simple_arith_swap2(p, inst, A0_SGE, 2, fs);
         case TGSI_OPCODE_SLT:
      emit_simple_arith(p, inst, A0_SLT, 2, fs);
         case TGSI_OPCODE_SGT:
      /* like SLT, but swap reg0, reg1 */
   emit_simple_arith_swap2(p, inst, A0_SLT, 2, fs);
         case TGSI_OPCODE_SNE: {
      const uint32_t zero =
            /* if we're < or > then we're != */
   src0 = src_vector(p, &inst->Src[0], fs);
   src1 = src_vector(p, &inst->Src[1], fs);
            if (src0 == zero || src1 == zero) {
                     /* x != 0 is equivalent to -abs(x) < 0, but the latter requires only
   * two instructions instead of three.
   */
   i915_emit_arith(p, A0_MAX, tmp, A0_DEST_CHANNEL_ALL, 0, src0,
         i915_emit_arith(p, A0_SLT, get_result_vector(p, &inst->Dst[0]),
            } else {
                              i915_emit_arith(p, A0_ADD, get_result_vector(p, &inst->Dst[0]),
            }
               case TGSI_OPCODE_SSG:
      /* compute (src>0) - (src<0) */
   src0 = src_vector(p, &inst->Src[0], fs);
            i915_emit_arith(p, A0_SLT, tmp, A0_DEST_CHANNEL_ALL, 0, src0,
            i915_emit_arith(p, A0_SLT, get_result_vector(p, &inst->Dst[0]),
                  i915_emit_arith(
      p, A0_ADD, get_result_vector(p, &inst->Dst[0]), get_result_flags(inst),
            case TGSI_OPCODE_TEX:
      emit_tex(p, inst, T0_TEXLD, fs);
         case TGSI_OPCODE_TRUNC:
      emit_simple_arith(p, inst, A0_TRC, 1, fs);
         case TGSI_OPCODE_TXB:
      emit_tex(p, inst, T0_TEXLDB, fs);
         case TGSI_OPCODE_TXP:
      emit_tex(p, inst, T0_TEXLDP, fs);
         default:
      i915_program_error(p, "bad opcode %s (%d)",
                              }
      static void
   i915_translate_token(struct i915_fp_compile *p,
               {
      struct i915_fragment_shader *ifs = p->shader;
   switch (token->Token.Type) {
   case TGSI_TOKEN_TYPE_PROPERTY:
      /* Ignore properties where we only support one value. */
   assert(token->FullProperty.Property.PropertyName ==
               token->FullProperty.Property.PropertyName ==
         token->FullProperty.Property.PropertyName ==
         token->FullProperty.Property.PropertyName ==
         case TGSI_TOKEN_TYPE_DECLARATION:
      if (token->FullDeclaration.Declaration.File == TGSI_FILE_CONSTANT) {
      if (token->FullDeclaration.Range.Last >= I915_MAX_CONSTANT) {
      i915_program_error(p, "Exceeded %d max uniforms",
      } else {
      uint32_t i;
   for (i = token->FullDeclaration.Range.First;
      i <= token->FullDeclaration.Range.Last; i++) {
   ifs->constant_flags[i] = I915_CONSTFLAG_USER;
            } else if (token->FullDeclaration.Declaration.File ==
            if (token->FullDeclaration.Range.Last >= I915_MAX_TEMPORARY) {
      i915_program_error(p, "Exceeded %d max TGSI temps",
      } else {
      uint32_t i;
   for (i = token->FullDeclaration.Range.First;
      i <= token->FullDeclaration.Range.Last; i++) {
   /* XXX just use shader->info->file_mask[TGSI_FILE_TEMPORARY] */
            }
         case TGSI_TOKEN_TYPE_IMMEDIATE: {
      const struct tgsi_full_immediate *imm = &token->FullImmediate;
   const uint32_t pos = p->num_immediates++;
   uint32_t j;
   assert(imm->Immediate.NrTokens <= 4 + 1);
   for (j = 0; j < imm->Immediate.NrTokens - 1; j++) {
                     case TGSI_TOKEN_TYPE_INSTRUCTION:
      if (p->first_instruction) {
      /* resolve location of immediates */
   uint32_t i, j;
   for (i = 0; i < p->num_immediates; i++) {
      /* find constant slot for this immediate */
   for (j = 0; j < I915_MAX_CONSTANT; j++) {
      if (ifs->constant_flags[j] == 0x0) {
      memcpy(ifs->constants[j], p->immediates[i],
         /*printf("immediate %d maps to const %d\n", i, j);*/
   ifs->constant_flags[j] = 0xf; /* all four comps used */
   p->immediates_map[i] = j;
   ifs->num_constants = MAX2(ifs->num_constants, j + 1);
         }
   if (j == I915_MAX_CONSTANT) {
      i915_program_error(p, "Exceeded %d max uniforms and immediates.",
                              i915_translate_instruction(p, &token->FullInstruction, fs);
         default:
            }
      /**
   * Translate TGSI fragment shader into i915 hardware instructions.
   * \param p  the translation state
   * \param tokens  the TGSI token array
   */
   static void
   i915_translate_instructions(struct i915_fp_compile *p,
               {
      int i;
   for (i = 0; i < tokens->NumTokens && !p->error[0]; i++) {
            }
      static struct i915_fp_compile *
   i915_init_compile(struct i915_fragment_shader *ifs)
   {
      struct i915_fp_compile *p = CALLOC_STRUCT(i915_fp_compile);
            p->shader = ifs;
            /* Put new constants at end of const buffer, growing downward.
   * The problem is we don't know how many user-defined constants might
   * be specified with pipe->set_constant_buffer().
   * Should pre-scan the user's program to determine the highest-numbered
   * constant referenced.
   */
   ifs->num_constants = 0;
                     for (i = 0; i < I915_TEX_UNITS; i++)
                     p->nr_tex_indirect = 1; /* correct? */
   p->nr_tex_insn = 0;
   p->nr_alu_insn = 0;
            p->csr = p->program;
   p->decl = p->declarations;
   p->decl_s = 0;
   p->decl_t = 0;
   p->temp_flag = ~0x0U << I915_MAX_TEMPORARY;
            /* initialize the first program word */
               }
      /* Copy compile results to the fragment program struct and destroy the
   * compilation context.
   */
   static void
   i915_fini_compile(struct i915_context *i915, struct i915_fp_compile *p)
   {
      struct i915_fragment_shader *ifs = p->shader;
   unsigned long program_size = (unsigned long)(p->csr - p->program);
            if (p->nr_tex_indirect > I915_MAX_TEX_INDIRECT) {
      i915_program_error(p,
                     if (p->nr_tex_insn > I915_MAX_TEX_INSN) {
      i915_program_error(p, "Exceeded max TEX instructions (%d/%d)",
               if (p->nr_alu_insn > I915_MAX_ALU_INSN) {
      i915_program_error(p, "Exceeded max ALU instructions (%d/%d)",
               if (p->nr_decl_insn > I915_MAX_DECL_INSN) {
      i915_program_error(p, "Exceeded max DECL instructions (%d/%d)",
               /* hw doesn't seem to like empty frag programs (num_instructions == 1 is just
   * TGSI_END), even when the depth write fixup gets emitted below - maybe that
   * one is fishy, too?
   */
   if (ifs->info.num_instructions == 1)
            if (strlen(p->error) != 0) {
      p->NumNativeInstructions = 0;
   p->NumNativeAluInstructions = 0;
   p->NumNativeTexInstructions = 0;
               } else {
      p->NumNativeInstructions =
         p->NumNativeAluInstructions = p->nr_alu_insn;
   p->NumNativeTexInstructions = p->nr_tex_insn;
            /* patch in the program length */
            /* Copy compilation results to fragment program struct:
   */
            ifs->program_len = decl_size + program_size;
   ifs->program = (uint32_t *)MALLOC(ifs->program_len * sizeof(uint32_t));
   memcpy(ifs->program, p->declarations, decl_size * sizeof(uint32_t));
   memcpy(&ifs->program[decl_size], p->program,
            if (i915) {
      util_debug_message(
      &i915->debug, SHADER_INFO,
   "%s shader: %d inst, %d tex, %d tex_indirect, %d temps, %d const",
   _mesa_shader_stage_to_abbrev(MESA_SHADER_FRAGMENT),
   (int)program_size, p->nr_tex_insn, p->nr_tex_indirect,
   p->shader->info.file_max[TGSI_FILE_TEMPORARY] + 1,
               if (strlen(p->error) != 0)
         else
            /* Release the compilation struct:
   */
      }
      /**
   * Rather than trying to intercept and jiggle depth writes during
   * emit, just move the value into its correct position at the end of
   * the program:
   */
   static void
   i915_fixup_depth_write(struct i915_fp_compile *p)
   {
      for (int i = 0; i < p->shader->info.num_outputs; i++) {
      if (p->shader->info.output_semantic_name[i] != TGSI_SEMANTIC_POSITION)
                     i915_emit_arith(p, A0_MOV,                  /* opcode */
                  depth,                      /* dest reg */
   A0_DEST_CHANNEL_W,          /* write mask */
      }
      void
   i915_translate_fragment_program(struct i915_context *i915,
         {
      struct i915_fp_compile *p;
   const struct tgsi_token *tokens = fs->state.tokens;
   struct i915_token_list *i_tokens;
   bool debug =
            if (debug) {
      mesa_logi("TGSI fragment shader:");
                        i_tokens = i915_optimize(tokens);
   i915_translate_instructions(p, i_tokens, fs);
            i915_fini_compile(i915, p);
            if (debug) {
      if (fs->error)
            mesa_logi("i915 fragment shader with %d constants%s", fs->num_constants,
            for (int i = 0; i < I915_MAX_CONSTANT; i++) {
      if (fs->constant_flags[i] &&
      fs->constant_flags[i] != I915_CONSTFLAG_USER) {
   mesa_logi("\t\tC[%d] = { %f, %f, %f, %f }", i, fs->constants[i][0],
               }
         }
