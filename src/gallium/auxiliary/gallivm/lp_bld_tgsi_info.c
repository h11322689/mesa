   /**************************************************************************
   *
   * Copyright 2010 VMware, Inc.
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
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   **************************************************************************/
         #include "util/compiler.h"
   #include "util/u_memory.h"
   #include "util/u_math.h"
   #include "tgsi/tgsi_parse.h"
   #include "tgsi/tgsi_util.h"
   #include "tgsi/tgsi_dump.h"
   #include "tgsi/tgsi_strings.h"
   #include "lp_bld_debug.h"
   #include "lp_bld_tgsi.h"
         /**
   * Analysis context.
   *
   * This is where we keep store the value of each channel of the IMM/TEMP/OUT
   * register values, as we walk the shader.
   */
   struct analysis_context
   {
               unsigned num_imms;
   float imm[LP_MAX_TGSI_IMMEDIATES][4];
               };
         /**
   * Describe the specified channel of the src register.
   */
   static void
   analyse_src(struct analysis_context *ctx,
               struct lp_tgsi_channel_info *chan_info,
   {
      chan_info->file = TGSI_FILE_NULL;
   if (!src->Indirect && !src->Absolute && !src->Negate) {
      unsigned swizzle = tgsi_util_get_src_register_swizzle(src, chan);
   if (src->File == TGSI_FILE_TEMPORARY) {
      if (src->Index < ARRAY_SIZE(ctx->temp)) {
            } else {
      chan_info->file = src->File;
   if (src->File == TGSI_FILE_IMMEDIATE) {
      assert(src->Index < ARRAY_SIZE(ctx->imm));
   if (src->Index < ARRAY_SIZE(ctx->imm)) {
            } else {
      chan_info->u.index = src->Index;
               }
         /**
   * Whether this register channel refers to a specific immediate value.
   */
   static bool
   is_immediate(const struct lp_tgsi_channel_info *chan_info, float value)
   {
      return chan_info->file == TGSI_FILE_IMMEDIATE &&
      }
         /**
   * Analyse properties of tex instructions, in particular used
   * to figure out if a texture is considered indirect.
   * Not actually used by much except the tgsi dumping code.
   */
   static void
   analyse_tex(struct analysis_context *ctx,
               {
      struct lp_tgsi_info *info = ctx->info;
            if (info->num_texs < ARRAY_SIZE(info->tex)) {
      struct lp_tgsi_texture_info *tex_info = &info->tex[info->num_texs];
   bool indirect = false;
            tex_info->target = inst->Texture.Texture;
   switch (inst->Texture.Texture) {
   case TGSI_TEXTURE_1D:
      readmask = TGSI_WRITEMASK_X;
      case TGSI_TEXTURE_1D_ARRAY:
   case TGSI_TEXTURE_2D:
   case TGSI_TEXTURE_RECT:
      readmask = TGSI_WRITEMASK_XY;
      case TGSI_TEXTURE_SHADOW1D:
   case TGSI_TEXTURE_SHADOW1D_ARRAY:
   case TGSI_TEXTURE_SHADOW2D:
   case TGSI_TEXTURE_SHADOWRECT:
   case TGSI_TEXTURE_2D_ARRAY:
   case TGSI_TEXTURE_2D_MSAA:
   case TGSI_TEXTURE_3D:
   case TGSI_TEXTURE_CUBE:
      readmask = TGSI_WRITEMASK_XYZ;
      case TGSI_TEXTURE_SHADOW2D_ARRAY:
   case TGSI_TEXTURE_SHADOWCUBE:
   case TGSI_TEXTURE_2D_ARRAY_MSAA:
   case TGSI_TEXTURE_CUBE_ARRAY:
      readmask = TGSI_WRITEMASK_XYZW;
   /* modifier would be in another not analyzed reg so just say indirect */
   if (modifier != LP_BLD_TEX_MODIFIER_NONE) {
         }
      case TGSI_TEXTURE_SHADOWCUBE_ARRAY:
      readmask = TGSI_WRITEMASK_XYZW;
   indirect = true;
      default:
      assert(0);
               if (modifier == LP_BLD_TEX_MODIFIER_EXPLICIT_DERIV) {
      /* We don't track explicit derivatives, although we could */
   indirect = true;
   tex_info->sampler_unit = inst->Src[3].Register.Index;
      }  else {
      if (modifier == LP_BLD_TEX_MODIFIER_PROJECTED ||
      modifier == LP_BLD_TEX_MODIFIER_LOD_BIAS ||
   modifier == LP_BLD_TEX_MODIFIER_EXPLICIT_LOD) {
      }
   tex_info->sampler_unit = inst->Src[1].Register.Index;
               for (chan = 0; chan < 4; ++chan) {
      struct lp_tgsi_channel_info *chan_info = &tex_info->coord[chan];
   if (readmask & (1 << chan)) {
      analyse_src(ctx, chan_info, &inst->Src[0].Register, chan);
   if (chan_info->file != TGSI_FILE_INPUT) {
            } else {
                     if (indirect) {
                     } else {
            }
         /**
   * Analyse properties of sample instructions, in particular used
   * to figure out if a texture is considered indirect.
   * Not actually used by much except the tgsi dumping code.
   */
   static void
   analyse_sample(struct analysis_context *ctx,
                     {
      struct lp_tgsi_info *info = ctx->info;
            if (info->num_texs < ARRAY_SIZE(info->tex)) {
      struct lp_tgsi_texture_info *tex_info = &info->tex[info->num_texs];
   unsigned target = ctx->sample_target[inst->Src[1].Register.Index];
   bool indirect = false;
   bool shadow = false;
            switch (target) {
   /* note no shadow targets here */
   case TGSI_TEXTURE_BUFFER:
   case TGSI_TEXTURE_1D:
      readmask = TGSI_WRITEMASK_X;
      case TGSI_TEXTURE_1D_ARRAY:
   case TGSI_TEXTURE_2D:
   case TGSI_TEXTURE_RECT:
      readmask = TGSI_WRITEMASK_XY;
      case TGSI_TEXTURE_2D_ARRAY:
   case TGSI_TEXTURE_2D_MSAA:
   case TGSI_TEXTURE_3D:
   case TGSI_TEXTURE_CUBE:
      readmask = TGSI_WRITEMASK_XYZ;
      case TGSI_TEXTURE_CUBE_ARRAY:
   case TGSI_TEXTURE_2D_ARRAY_MSAA:
      readmask = TGSI_WRITEMASK_XYZW;
      default:
      assert(0);
               tex_info->target = target;
   tex_info->texture_unit = inst->Src[1].Register.Index;
            if (tex_info->texture_unit != tex_info->sampler_unit) {
                  if (modifier == LP_BLD_TEX_MODIFIER_EXPLICIT_DERIV ||
      modifier == LP_BLD_TEX_MODIFIER_EXPLICIT_LOD ||
   modifier == LP_BLD_TEX_MODIFIER_LOD_BIAS || shadow) {
   /* We don't track insts with additional regs, although we could */
               for (chan = 0; chan < 4; ++chan) {
      struct lp_tgsi_channel_info *chan_info = &tex_info->coord[chan];
   if (readmask & (1 << chan)) {
      analyse_src(ctx, chan_info, &inst->Src[0].Register, chan);
   if (chan_info->file != TGSI_FILE_INPUT) {
            } else {
                     if (indirect) {
                     } else {
            }
         /**
   * Process an instruction, and update the register values accordingly.
   */
   static void
   analyse_instruction(struct analysis_context *ctx,
         {
      struct lp_tgsi_info *info = ctx->info;
   struct lp_tgsi_channel_info (*regs)[4];
   unsigned max_regs;
   unsigned i;
   unsigned index;
            for (i = 0; i < inst->Instruction.NumDstRegs; ++i) {
               /*
   * Get the lp_tgsi_channel_info array corresponding to the destination
   * register file.
            if (dst->File == TGSI_FILE_TEMPORARY) {
      regs = ctx->temp;
      } else if (dst->File == TGSI_FILE_OUTPUT) {
      regs = info->output;
      } else if (dst->File == TGSI_FILE_ADDRESS) {
         } else if (dst->File == TGSI_FILE_BUFFER) {
         } else if (dst->File == TGSI_FILE_IMAGE) {
         } else if (dst->File == TGSI_FILE_MEMORY) {
         } else {
      assert(0);
               /*
   * Detect direct TEX instructions
            switch (inst->Instruction.Opcode) {
   case TGSI_OPCODE_TEX:
      analyse_tex(ctx, inst, LP_BLD_TEX_MODIFIER_NONE);
      case TGSI_OPCODE_TXD:
      analyse_tex(ctx, inst, LP_BLD_TEX_MODIFIER_EXPLICIT_DERIV);
      case TGSI_OPCODE_TXB:
      analyse_tex(ctx, inst, LP_BLD_TEX_MODIFIER_LOD_BIAS);
      case TGSI_OPCODE_TXL:
      analyse_tex(ctx, inst, LP_BLD_TEX_MODIFIER_EXPLICIT_LOD);
      case TGSI_OPCODE_TXP:
      analyse_tex(ctx, inst, LP_BLD_TEX_MODIFIER_PROJECTED);
      case TGSI_OPCODE_TEX2:
      analyse_tex(ctx, inst, LP_BLD_TEX_MODIFIER_NONE);
      case TGSI_OPCODE_TXB2:
      analyse_tex(ctx, inst, LP_BLD_TEX_MODIFIER_LOD_BIAS);
      case TGSI_OPCODE_TXL2:
      analyse_tex(ctx, inst, LP_BLD_TEX_MODIFIER_EXPLICIT_LOD);
      case TGSI_OPCODE_SAMPLE:
      analyse_sample(ctx, inst, LP_BLD_TEX_MODIFIER_NONE, false);
      case TGSI_OPCODE_SAMPLE_C:
      analyse_sample(ctx, inst, LP_BLD_TEX_MODIFIER_NONE, true);
      case TGSI_OPCODE_SAMPLE_C_LZ:
      analyse_sample(ctx, inst, LP_BLD_TEX_MODIFIER_LOD_ZERO, true);
      case TGSI_OPCODE_SAMPLE_D:
      analyse_sample(ctx, inst, LP_BLD_TEX_MODIFIER_EXPLICIT_DERIV, false);
      case TGSI_OPCODE_SAMPLE_B:
      analyse_sample(ctx, inst, LP_BLD_TEX_MODIFIER_LOD_BIAS, false);
      case TGSI_OPCODE_SAMPLE_L:
      analyse_sample(ctx, inst, LP_BLD_TEX_MODIFIER_EXPLICIT_LOD, false);
      default:
                  /*
   * Keep track of assignments and writes
            if (dst->Indirect) {
      /*
                  for (chan = 0; chan < 4; ++chan) {
      if (dst->WriteMask & (1 << chan)) {
      for (index = 0; index < max_regs; ++index) {
                  } else if (dst->Index < max_regs) {
      /*
                                    if (!inst->Instruction.Saturate) {
      for (chan = 0; chan < 4; ++chan) {
      if (dst->WriteMask & (1 << chan)) {
      if (inst->Instruction.Opcode == TGSI_OPCODE_MOV) {
      analyse_src(ctx, &res[chan],
      } else if (inst->Instruction.Opcode == TGSI_OPCODE_MUL) {
                                                         if (is_immediate(&src0, 0.0f)) {
         } else if (is_immediate(&src1, 0.0f)) {
         } else if (is_immediate(&src0, 1.0f)) {
         } else if (is_immediate(&src1, 1.0f)) {
                              for (chan = 0; chan < 4; ++chan) {
      if (dst->WriteMask & (1 << chan)) {
                           /*
   * Clear all temporaries information in presence of a control flow opcode.
            switch (inst->Instruction.Opcode) {
   case TGSI_OPCODE_IF:
   case TGSI_OPCODE_UIF:
   case TGSI_OPCODE_ELSE:
   case TGSI_OPCODE_ENDIF:
   case TGSI_OPCODE_BGNLOOP:
   case TGSI_OPCODE_BRK:
   case TGSI_OPCODE_CONT:
   case TGSI_OPCODE_ENDLOOP:
   case TGSI_OPCODE_CAL:
   case TGSI_OPCODE_BGNSUB:
   case TGSI_OPCODE_ENDSUB:
   case TGSI_OPCODE_SWITCH:
   case TGSI_OPCODE_CASE:
   case TGSI_OPCODE_DEFAULT:
   case TGSI_OPCODE_ENDSWITCH:
   case TGSI_OPCODE_RET:
   case TGSI_OPCODE_END:
      /* XXX: Are there more cases? */
   memset(&ctx->temp, 0, sizeof ctx->temp);
   memset(&info->output, 0, sizeof info->output);
      default:
            }
         static inline void
   dump_info(const struct tgsi_token *tokens,
         {
      unsigned index;
                     for (index = 0; index < info->num_texs; ++index) {
      const struct lp_tgsi_texture_info *tex_info = &info->tex[index];
   debug_printf("TEX[%u] =", index);
   for (chan = 0; chan < 4; ++chan) {
      const struct lp_tgsi_channel_info *chan_info =
         if (chan_info->file != TGSI_FILE_NULL) {
      debug_printf(" %s[%u].%c",
                  } else {
            }
   debug_printf(", RES[%u], SAMP[%u], %s\n",
                           for (index = 0; index < PIPE_MAX_SHADER_OUTPUTS; ++index) {
      for (chan = 0; chan < 4; ++chan) {
      const struct lp_tgsi_channel_info *chan_info =
         if (chan_info->file != TGSI_FILE_NULL) {
      debug_printf("OUT[%u].%c = ", index, "xyzw"[chan]);
   if (chan_info->file == TGSI_FILE_IMMEDIATE) {
         } else {
      const char *file_name;
   switch (chan_info->file) {
   case TGSI_FILE_CONSTANT:
      file_name = "CONST";
      case TGSI_FILE_INPUT:
      file_name = "IN";
      default:
      file_name = "???";
      }
   debug_printf("%s[%u].%c",
                  }
               }
         /**
   * Detect any direct relationship between the output color
   */
   void
   lp_build_tgsi_info(const struct tgsi_token *tokens,
         {
      struct tgsi_parse_context parse;
   struct analysis_context *ctx;
   unsigned index;
                              ctx = CALLOC(1, sizeof(struct analysis_context));
                     while (!tgsi_parse_end_of_tokens(&parse)) {
               switch (parse.FullToken.Token.Type) {
   case TGSI_TOKEN_TYPE_DECLARATION: {
      struct tgsi_full_declaration *decl = &parse.FullToken.FullDeclaration;
   if (decl->Declaration.File == TGSI_FILE_SAMPLER_VIEW) {
      for (index = decl->Range.First; index <= decl->Range.Last; index++) {
               }
            case TGSI_TOKEN_TYPE_INSTRUCTION:
      {
                     if (inst->Instruction.Opcode == TGSI_OPCODE_END ||
      inst->Instruction.Opcode == TGSI_OPCODE_BGNSUB) {
                                    case TGSI_TOKEN_TYPE_IMMEDIATE:
      {
      const unsigned size =
         assert(size <= 4);
   if (ctx->num_imms < ARRAY_SIZE(ctx->imm)) {
                              if (value < 0.0f || value > 1.0f) {
            }
                     case TGSI_TOKEN_TYPE_PROPERTY:
            default:
               finished:
         tgsi_parse_free(&parse);
               /*
   * Link the output color values.
            for (index = 0; index < PIPE_MAX_COLOR_BUFS; ++index) {
      static const struct lp_tgsi_channel_info null_output[4];
               for (index = 0; index < info->base.num_outputs; ++index) {
      unsigned semantic_name = info->base.output_semantic_name[index];
   unsigned semantic_index = info->base.output_semantic_index[index];
   if (semantic_name == TGSI_SEMANTIC_COLOR &&
      semantic_index < PIPE_MAX_COLOR_BUFS) {
                  if (gallivm_debug & GALLIVM_DEBUG_TGSI) {
            }
