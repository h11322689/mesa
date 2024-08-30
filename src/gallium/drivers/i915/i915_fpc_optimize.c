   /**************************************************************************
   *
   * Copyright 2011 The Chromium OS authors.
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
   * IN NO EVENT SHALL GOOGLE AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      #include "i915_context.h"
   #include "i915_fpc.h"
   #include "i915_reg.h"
      #include "pipe/p_shader_tokens.h"
   #include "tgsi/tgsi_dump.h"
   #include "tgsi/tgsi_exec.h"
   #include "tgsi/tgsi_parse.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/u_string.h"
      struct i915_optimize_context {
      int first_write[TGSI_EXEC_NUM_TEMPS];
      };
      static bool
   same_src_dst_reg(struct i915_full_src_register *s1,
         {
      return (s1->Register.File == d1->Register.File &&
         s1->Register.Indirect == d1->Register.Indirect &&
      }
      static bool
   same_dst_reg(struct i915_full_dst_register *d1,
         {
      return (d1->Register.File == d2->Register.File &&
         d1->Register.Indirect == d2->Register.Indirect &&
      }
      static bool
   same_src_reg(struct i915_full_src_register *d1,
         {
      return (d1->Register.File == d2->Register.File &&
         d1->Register.Indirect == d2->Register.Indirect &&
   d1->Register.Dimension == d2->Register.Dimension &&
   d1->Register.Index == d2->Register.Index &&
      }
      static const struct {
      bool is_texture;
   bool commutes;
   unsigned neutral_element;
   unsigned num_dst;
      } op_table[TGSI_OPCODE_LAST] = {
      [TGSI_OPCODE_ADD] = {false, true, TGSI_SWIZZLE_ZERO, 1, 2},
   [TGSI_OPCODE_CEIL] = {false, false, 0, 1, 1},
   [TGSI_OPCODE_CMP] = {false, false, 0, 1, 2},
   [TGSI_OPCODE_COS] = {false, false, 0, 1, 1},
   [TGSI_OPCODE_DDX] = {false, false, 0, 1, 0},
   [TGSI_OPCODE_DDY] = {false, false, 0, 1, 0},
   [TGSI_OPCODE_DP2] = {false, true, TGSI_SWIZZLE_ONE, 1, 2},
   [TGSI_OPCODE_DP3] = {false, true, TGSI_SWIZZLE_ONE, 1, 2},
   [TGSI_OPCODE_DP4] = {false, true, TGSI_SWIZZLE_ONE, 1, 2},
   [TGSI_OPCODE_DST] = {false, false, 0, 1, 2},
   [TGSI_OPCODE_END] = {false, false, 0, 0, 0},
   [TGSI_OPCODE_EX2] = {false, false, 0, 1, 1},
   [TGSI_OPCODE_FLR] = {false, false, 0, 1, 1},
   [TGSI_OPCODE_FRC] = {false, false, 0, 1, 1},
   [TGSI_OPCODE_KILL_IF] = {false, false, 0, 0, 1},
   [TGSI_OPCODE_KILL] = {false, false, 0, 0, 0},
   [TGSI_OPCODE_LG2] = {false, false, 0, 1, 1},
   [TGSI_OPCODE_LIT] = {false, false, 0, 1, 1},
   [TGSI_OPCODE_LRP] = {false, false, 0, 1, 3},
   [TGSI_OPCODE_MAX] = {false, false, 0, 1, 2},
   [TGSI_OPCODE_MAD] = {false, false, 0, 1, 3},
   [TGSI_OPCODE_MIN] = {false, false, 0, 1, 2},
   [TGSI_OPCODE_MOV] = {false, false, 0, 1, 1},
   [TGSI_OPCODE_MUL] = {false, true, TGSI_SWIZZLE_ONE, 1, 2},
   [TGSI_OPCODE_NOP] = {false, false, 0, 0, 0},
   [TGSI_OPCODE_POW] = {false, false, 0, 1, 2},
   [TGSI_OPCODE_RCP] = {false, false, 0, 1, 1},
   [TGSI_OPCODE_RET] = {false, false, 0, 0, 0},
   [TGSI_OPCODE_RSQ] = {false, false, 0, 1, 1},
   [TGSI_OPCODE_SEQ] = {false, false, 0, 1, 2},
   [TGSI_OPCODE_SGE] = {false, false, 0, 1, 2},
   [TGSI_OPCODE_SGT] = {false, false, 0, 1, 2},
   [TGSI_OPCODE_SIN] = {false, false, 0, 1, 1},
   [TGSI_OPCODE_SLE] = {false, false, 0, 1, 2},
   [TGSI_OPCODE_SLT] = {false, false, 0, 1, 2},
   [TGSI_OPCODE_SNE] = {false, false, 0, 1, 2},
   [TGSI_OPCODE_SSG] = {false, false, 0, 1, 1},
   [TGSI_OPCODE_TEX] = {true, false, 0, 1, 2},
   [TGSI_OPCODE_TRUNC] = {false, false, 0, 1, 1},
   [TGSI_OPCODE_TXB] = {true, false, 0, 1, 2},
      };
      static bool
   op_has_dst(unsigned opcode)
   {
         }
      static int
   op_num_dst(unsigned opcode)
   {
         }
      static int
   op_num_src(unsigned opcode)
   {
         }
      static bool
   op_commutes(unsigned opcode)
   {
         }
      static bool
   is_unswizzled(struct i915_full_src_register *r, unsigned write_mask)
   {
      if (write_mask & TGSI_WRITEMASK_X && r->Register.SwizzleX != TGSI_SWIZZLE_X)
         if (write_mask & TGSI_WRITEMASK_Y && r->Register.SwizzleY != TGSI_SWIZZLE_Y)
         if (write_mask & TGSI_WRITEMASK_Z && r->Register.SwizzleZ != TGSI_SWIZZLE_Z)
         if (write_mask & TGSI_WRITEMASK_W && r->Register.SwizzleW != TGSI_SWIZZLE_W)
            }
      static bool
   op_is_texture(unsigned opcode)
   {
         }
      static unsigned
   op_neutral_element(unsigned opcode)
   {
      unsigned ne = op_table[opcode].neutral_element;
   if (!ne) {
      debug_printf("No neutral element for opcode %d\n", opcode);
      }
      }
      /*
   * Sets the swizzle to the neutral element for the operation for the bits
   * of writemask which are set, swizzle to identity otherwise.
   */
   static void
   set_neutral_element_swizzle(struct i915_full_src_register *r,
         {
      if (write_mask & TGSI_WRITEMASK_X)
         else
            if (write_mask & TGSI_WRITEMASK_Y)
         else
            if (write_mask & TGSI_WRITEMASK_Z)
         else
            if (write_mask & TGSI_WRITEMASK_W)
         else
      }
      static void
   copy_src_reg(struct i915_src_register *o, const struct tgsi_src_register *i)
   {
      o->File = i->File;
   o->Indirect = i->Indirect;
   o->Dimension = i->Dimension;
   o->Index = i->Index;
   o->SwizzleX = i->SwizzleX;
   o->SwizzleY = i->SwizzleY;
   o->SwizzleZ = i->SwizzleZ;
   o->SwizzleW = i->SwizzleW;
   o->Absolute = i->Absolute;
      }
      static void
   copy_dst_reg(struct i915_dst_register *o, const struct tgsi_dst_register *i)
   {
      o->File = i->File;
   o->WriteMask = i->WriteMask;
   o->Indirect = i->Indirect;
   o->Dimension = i->Dimension;
      }
      static void
   copy_instruction(struct i915_full_instruction *o,
         {
      memcpy(&o->Instruction, &i->Instruction, sizeof(o->Instruction));
                     copy_src_reg(&o->Src[0].Register, &i->Src[0].Register);
   copy_src_reg(&o->Src[1].Register, &i->Src[1].Register);
      }
      static void
   copy_token(union i915_full_token *o, union tgsi_full_token *i)
   {
      if (i->Token.Type != TGSI_TOKEN_TYPE_INSTRUCTION)
         else
      }
      static void
   liveness_mark_written(struct i915_optimize_context *ctx,
         {
      int dst_reg_index;
   if (dst_reg->Register.File == TGSI_FILE_TEMPORARY) {
      dst_reg_index = dst_reg->Register.Index;
   assert(dst_reg_index < TGSI_EXEC_NUM_TEMPS);
   /* dead -> live transition */
   if (ctx->first_write[dst_reg_index] != -1)
         }
      static void
   liveness_mark_read(struct i915_optimize_context *ctx,
         {
      int src_reg_index;
   if (src_reg->Register.File == TGSI_FILE_TEMPORARY) {
      src_reg_index = src_reg->Register.Index;
   assert(src_reg_index < TGSI_EXEC_NUM_TEMPS);
   /* live -> dead transition */
   if (ctx->last_read[src_reg_index] != -1)
         }
      static void
   liveness_analysis(struct i915_optimize_context *ctx,
         {
      struct i915_full_dst_register *dst_reg;
   struct i915_full_src_register *src_reg;
   union i915_full_token *current;
   unsigned opcode;
   int num_dst, num_src;
            for (i = 0; i < TGSI_EXEC_NUM_TEMPS; i++) {
      ctx->first_write[i] = -1;
               for (i = 0; i < tokens->NumTokens; i++) {
               if (current->Token.Type != TGSI_TOKEN_TYPE_INSTRUCTION)
            opcode = current->FullInstruction.Instruction.Opcode;
            switch (num_dst) {
   case 1:
      dst_reg = &current->FullInstruction.Dst[0];
   liveness_mark_written(ctx, dst_reg, i);
      case 0:
         default:
      debug_printf("Op %d has %d dst regs\n", opcode, num_dst);
                  for (i = tokens->NumTokens - 1; i >= 0; i--) {
               if (current->Token.Type != TGSI_TOKEN_TYPE_INSTRUCTION)
            opcode = current->FullInstruction.Instruction.Opcode;
            switch (num_src) {
   case 3:
      src_reg = &current->FullInstruction.Src[2];
   liveness_mark_read(ctx, src_reg, i);
      case 2:
      src_reg = &current->FullInstruction.Src[1];
   liveness_mark_read(ctx, src_reg, i);
      case 1:
      src_reg = &current->FullInstruction.Src[0];
   liveness_mark_read(ctx, src_reg, i);
      case 0:
         default:
      debug_printf("Op %d has %d src regs\n", opcode, num_src);
            }
      static int
   unused_from(struct i915_optimize_context *ctx,
         {
      int dst_reg_index = dst_reg->Register.Index;
   assert(dst_reg_index < TGSI_EXEC_NUM_TEMPS);
      }
      /* Returns a mask with the components used for a texture access instruction */
   static unsigned
   i915_tex_mask(union i915_full_token *instr)
   {
      return i915_coord_mask(instr->FullInstruction.Instruction.Opcode,
      }
      static bool
   target_is_texture2d(uint32_t tex)
   {
      switch (tex) {
   case TGSI_TEXTURE_2D:
   case TGSI_TEXTURE_RECT:
         default:
            }
      /*
   * Optimize away useless indirect texture reads:
   *    MOV TEMP[0].xy, IN[0].xyyy
   *    TEX TEMP[1], TEMP[0], SAMP[0], 2D
   * into:
   *    TEX TEMP[1], IN[0], SAMP[0], 2D
   *
   * note: this only seems to work on 2D/RECT textures, but not SHAADOW2D/1D/..
   */
   static void
   i915_fpc_optimize_mov_before_tex(struct i915_optimize_context *ctx,
         {
      union i915_full_token *current = &tokens->Tokens[index - 1];
            if (current->Token.Type == TGSI_TOKEN_TYPE_INSTRUCTION &&
      next->Token.Type == TGSI_TOKEN_TYPE_INSTRUCTION &&
   current->FullInstruction.Instruction.Opcode == TGSI_OPCODE_MOV &&
   op_is_texture(next->FullInstruction.Instruction.Opcode) &&
   target_is_texture2d(next->FullInstruction.Texture.Texture) &&
   same_src_dst_reg(&next->FullInstruction.Src[0],
         is_unswizzled(&current->FullInstruction.Src[0], i915_tex_mask(next)) &&
   unused_from(ctx, &current->FullInstruction.Dst[0], index)) {
   memcpy(&next->FullInstruction.Src[0], &current->FullInstruction.Src[0],
               }
      /*
   * Optimize away things like:
   *    MOV TEMP[0].xy, TEMP[1].xyyy (first write for TEMP[0])
   *    MOV TEMP[0].w, TEMP[1].wwww (last write for TEMP[0])
   * into:
   *    NOP
   *    MOV OUT[0].xyw, TEMP[1].xyww
   */
   static void
   i915_fpc_optimize_mov_after_mov(union i915_full_token *current,
         {
      struct i915_full_src_register *src_reg1, *src_reg2;
   struct i915_full_dst_register *dst_reg1, *dst_reg2;
            if (current->Token.Type == TGSI_TOKEN_TYPE_INSTRUCTION &&
      next->Token.Type == TGSI_TOKEN_TYPE_INSTRUCTION &&
   current->FullInstruction.Instruction.Opcode == TGSI_OPCODE_MOV &&
   next->FullInstruction.Instruction.Opcode == TGSI_OPCODE_MOV &&
   current->FullInstruction.Instruction.Saturate ==
         same_dst_reg(&next->FullInstruction.Dst[0],
         same_src_reg(&next->FullInstruction.Src[0],
         !same_src_dst_reg(&current->FullInstruction.Src[0],
         src_reg1 = &current->FullInstruction.Src[0];
   dst_reg1 = &current->FullInstruction.Dst[0];
   src_reg2 = &next->FullInstruction.Src[0];
            /* Start with swizzles from the first mov */
   swizzle_x = src_reg1->Register.SwizzleX;
   swizzle_y = src_reg1->Register.SwizzleY;
   swizzle_z = src_reg1->Register.SwizzleZ;
            /* Pile the second mov on top */
   if (dst_reg2->Register.WriteMask & TGSI_WRITEMASK_X)
         if (dst_reg2->Register.WriteMask & TGSI_WRITEMASK_Y)
         if (dst_reg2->Register.WriteMask & TGSI_WRITEMASK_Z)
         if (dst_reg2->Register.WriteMask & TGSI_WRITEMASK_W)
            dst_reg2->Register.WriteMask |= dst_reg1->Register.WriteMask;
   src_reg2->Register.SwizzleX = swizzle_x;
   src_reg2->Register.SwizzleY = swizzle_y;
   src_reg2->Register.SwizzleZ = swizzle_z;
                           }
      /*
   * Optimize away things like:
   *    MUL OUT[0].xyz, TEMP[1], TEMP[2]
   *    MOV OUT[0].w, TEMP[2]
   * into:
   *    MUL OUT[0].xyzw, TEMP[1].xyz1, TEMP[2]
   * This is useful for optimizing texenv.
   */
   static void
   i915_fpc_optimize_mov_after_alu(union i915_full_token *current,
         {
      if (current->Token.Type == TGSI_TOKEN_TYPE_INSTRUCTION &&
      next->Token.Type == TGSI_TOKEN_TYPE_INSTRUCTION &&
   op_commutes(current->FullInstruction.Instruction.Opcode) &&
   current->FullInstruction.Instruction.Saturate ==
         next->FullInstruction.Instruction.Opcode == TGSI_OPCODE_MOV &&
   same_dst_reg(&next->FullInstruction.Dst[0],
         same_src_reg(&next->FullInstruction.Src[0],
         !same_src_dst_reg(&next->FullInstruction.Src[0],
         is_unswizzled(&current->FullInstruction.Src[0],
         is_unswizzled(&current->FullInstruction.Src[1],
         is_unswizzled(&next->FullInstruction.Src[0],
                  set_neutral_element_swizzle(&current->FullInstruction.Src[1], 0, 0);
   set_neutral_element_swizzle(
      &current->FullInstruction.Src[0],
               current->FullInstruction.Dst[0].Register.WriteMask =
      current->FullInstruction.Dst[0].Register.WriteMask |
                  if (current->Token.Type == TGSI_TOKEN_TYPE_INSTRUCTION &&
      next->Token.Type == TGSI_TOKEN_TYPE_INSTRUCTION &&
   op_commutes(current->FullInstruction.Instruction.Opcode) &&
   current->FullInstruction.Instruction.Saturate ==
         next->FullInstruction.Instruction.Opcode == TGSI_OPCODE_MOV &&
   same_dst_reg(&next->FullInstruction.Dst[0],
         same_src_reg(&next->FullInstruction.Src[0],
         !same_src_dst_reg(&next->FullInstruction.Src[0],
         is_unswizzled(&current->FullInstruction.Src[0],
         is_unswizzled(&current->FullInstruction.Src[1],
         is_unswizzled(&next->FullInstruction.Src[0],
                  set_neutral_element_swizzle(&current->FullInstruction.Src[0], 0, 0);
   set_neutral_element_swizzle(
      &current->FullInstruction.Src[1],
               current->FullInstruction.Dst[0].Register.WriteMask =
      current->FullInstruction.Dst[0].Register.WriteMask |
            }
      /*
   * Optimize away things like:
   *    MOV TEMP[0].xyz TEMP[0].xyzx
   * into:
   *    NOP
   */
   static bool
   i915_fpc_useless_mov(union tgsi_full_token *tgsi_current)
   {
      union i915_full_token current;
   copy_token(&current, tgsi_current);
   if (current.Token.Type == TGSI_TOKEN_TYPE_INSTRUCTION &&
      current.FullInstruction.Instruction.Opcode == TGSI_OPCODE_MOV &&
   op_has_dst(current.FullInstruction.Instruction.Opcode) &&
   !current.FullInstruction.Instruction.Saturate &&
   current.FullInstruction.Src[0].Register.Absolute == 0 &&
   current.FullInstruction.Src[0].Register.Negate == 0 &&
   is_unswizzled(&current.FullInstruction.Src[0],
         same_src_dst_reg(&current.FullInstruction.Src[0],
            }
      }
      /*
   * Optimize away things like:
   *    *** TEMP[0], TEMP[1], TEMP[2]
   *    MOV OUT[0] TEMP[0]
   * into:
   *    *** OUT[0], TEMP[1], TEMP[2]
   */
   static void
   i915_fpc_optimize_useless_mov_after_inst(struct i915_optimize_context *ctx,
               {
      union i915_full_token *current = &tokens->Tokens[index - 1];
            // &out_tokens->Tokens[i-1], &out_tokens->Tokens[i]);
   if (current->Token.Type == TGSI_TOKEN_TYPE_INSTRUCTION &&
      next->Token.Type == TGSI_TOKEN_TYPE_INSTRUCTION &&
   next->FullInstruction.Instruction.Opcode == TGSI_OPCODE_MOV &&
   op_has_dst(current->FullInstruction.Instruction.Opcode) &&
   !next->FullInstruction.Instruction.Saturate &&
   next->FullInstruction.Src[0].Register.Absolute == 0 &&
   next->FullInstruction.Src[0].Register.Negate == 0 &&
   unused_from(ctx, &current->FullInstruction.Dst[0], index) &&
   current->FullInstruction.Dst[0].Register.WriteMask ==
         is_unswizzled(&next->FullInstruction.Src[0],
         current->FullInstruction.Dst[0].Register.WriteMask ==
         same_src_dst_reg(&next->FullInstruction.Src[0],
                  current->FullInstruction.Dst[0] = next->FullInstruction.Dst[0];
         }
      struct i915_token_list *
   i915_optimize(const struct tgsi_token *tokens)
   {
      struct i915_token_list *out_tokens = MALLOC(sizeof(struct i915_token_list));
   struct tgsi_parse_context parse;
   struct i915_optimize_context *ctx;
                              /* Count the tokens */
   tgsi_parse_init(&parse, tokens);
   while (!tgsi_parse_end_of_tokens(&parse)) {
      tgsi_parse_token(&parse);
      }
            /* Allocate our tokens */
   out_tokens->Tokens =
            tgsi_parse_init(&parse, tokens);
   while (!tgsi_parse_end_of_tokens(&parse)) {
               if (i915_fpc_useless_mov(&parse.FullToken)) {
      out_tokens->NumTokens--;
                           }
                     i = 1;
   while (i < out_tokens->NumTokens) {
      i915_fpc_optimize_useless_mov_after_inst(ctx, out_tokens, i);
   i915_fpc_optimize_mov_after_alu(&out_tokens->Tokens[i - 1],
         i915_fpc_optimize_mov_after_mov(&out_tokens->Tokens[i - 1],
         i915_fpc_optimize_mov_before_tex(ctx, out_tokens, i);
                           }
      void
   i915_optimize_free(struct i915_token_list *tokens)
   {
      free(tokens->Tokens);
      }
