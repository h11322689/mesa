   /* Author(s):
   *   Connor Abbott
   *   Alyssa Rosenzweig
   *
   * Copyright (c) 2013 Connor Abbott (connor@abbott.cx)
   * Copyright (c) 2018 Alyssa Rosenzweig (alyssa@rosenzweig.io)
   * Copyright (C) 2019-2020 Collabora, Ltd.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a copy
   * of this software and associated documentation files (the "Software"), to deal
   * in the Software without restriction, including without limitation the rights
   * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   * copies of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included in
   * all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   * THE SOFTWARE.
   */
      #include "disassemble.h"
   #include <assert.h>
   #include <ctype.h>
   #include <inttypes.h>
   #include <stdint.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include "util/bitscan.h"
   #include "util/half_float.h"
   #include "util/u_math.h"
   #include "helpers.h"
   #include "midgard.h"
   #include "midgard_ops.h"
   #include "midgard_quirks.h"
      #define DEFINE_CASE(define, str)                                               \
      case define: {                                                              \
      fprintf(fp, str);                                                        \
            /* These are not mapped to hardware values, they just represent the possible
   * implicit arg modifiers that some midgard opcodes have, which can be decoded
   * from the opcodes via midgard_{alu,ldst,tex}_special_arg_mod() */
   typedef enum {
      midgard_arg_mod_none = 0,
   midgard_arg_mod_inv,
      } midgard_special_arg_mod;
      typedef struct {
               /* For static analysis to ensure all registers are written at least once
   * before use along the source code path (TODO: does this break done for
   * complex CF?)
               } disassemble_context;
      /* Transform an expanded writemask (duplicated 8-bit format) into its condensed
   * form (one bit per component) */
      static inline unsigned
   condense_writemask(unsigned expanded_mask, unsigned bits_per_component)
   {
      if (bits_per_component == 8) {
      /* Duplicate every bit to go from 8 to 16-channel wrmask */
            for (unsigned i = 0; i < 8; ++i) {
      if (expanded_mask & (1 << i))
                           unsigned slots_per_component = bits_per_component / 16;
   unsigned max_comp = (16 * 8) / bits_per_component;
            for (unsigned i = 0; i < max_comp; i++) {
      if (expanded_mask & (1 << (i * slots_per_component)))
                  }
      static bool
   print_alu_opcode(FILE *fp, midgard_alu_op op)
   {
      if (alu_opcode_props[op].name)
         else
            /* For constant analysis */
      }
      static void
   print_ld_st_opcode(FILE *fp, midgard_load_store_op op)
   {
      if (load_store_opcode_props[op].name)
         else
      }
      static void
   validate_sampler_type(enum mali_texture_op op,
         {
      if (op == midgard_tex_op_mov || op == midgard_tex_op_barrier)
         else
      }
      static void
   validate_expand_mode(midgard_src_expand_mode expand_mode,
         {
      switch (expand_mode) {
   case midgard_src_passthrough:
            case midgard_src_rep_low:
      assert(reg_mode == midgard_reg_mode_8 || reg_mode == midgard_reg_mode_16);
         case midgard_src_rep_high:
      assert(reg_mode == midgard_reg_mode_8 || reg_mode == midgard_reg_mode_16);
         case midgard_src_swap:
      assert(reg_mode == midgard_reg_mode_8 || reg_mode == midgard_reg_mode_16);
         case midgard_src_expand_low:
      assert(reg_mode != midgard_reg_mode_8);
         case midgard_src_expand_high:
      assert(reg_mode != midgard_reg_mode_8);
         case midgard_src_expand_low_swap:
      assert(reg_mode == midgard_reg_mode_16);
         case midgard_src_expand_high_swap:
      assert(reg_mode == midgard_reg_mode_16);
         default:
      unreachable("Invalid expand mode");
         }
      static void
   print_alu_reg(disassemble_context *ctx, FILE *fp, unsigned reg, bool is_write)
   {
      unsigned uniform_reg = 23 - reg;
            /* For r8-r15, it could be a work or uniform. We distinguish based on
   * the fact work registers are ALWAYS written before use, but uniform
            if ((reg >= 8 && reg < 16) && !(ctx->midg_ever_written & (1 << reg)))
                     if (reg >= 16 && reg <= 23)
            if (reg == REGISTER_UNUSED || reg == REGISTER_UNUSED + 1)
         else if (reg == REGISTER_TEXTURE_BASE || reg == REGISTER_TEXTURE_BASE + 1)
         else if (reg == REGISTER_LDST_BASE || reg == REGISTER_LDST_BASE + 1)
         else if (is_uniform)
         else if (reg == 31 && !is_write)
         else
      }
      static void
   print_ldst_write_reg(FILE *fp, unsigned reg)
   {
      switch (reg) {
   case 26:
   case 27:
      fprintf(fp, "AL%u", reg - REGISTER_LDST_BASE);
      case 28:
   case 29:
      fprintf(fp, "AT%u", reg - REGISTER_TEXTURE_BASE);
      case 31:
      fprintf(fp, "PC_SP");
      default:
      fprintf(fp, "R%d", reg);
         }
      static void
   print_ldst_read_reg(FILE *fp, unsigned reg)
   {
      switch (reg) {
   case 0:
   case 1:
      fprintf(fp, "AL%u", reg);
      case 2:
      fprintf(fp, "PC_SP");
      case 3:
      fprintf(fp, "LOCAL_STORAGE_PTR");
      case 4:
      fprintf(fp, "LOCAL_THREAD_ID");
      case 5:
      fprintf(fp, "GROUP_ID");
      case 6:
      fprintf(fp, "GLOBAL_THREAD_ID");
      case 7:
      fprintf(fp, "0");
      default:
            }
      static void
   print_tex_reg(FILE *fp, unsigned reg, bool is_write)
   {
      char *str = is_write ? "TA" : "AT";
            switch (reg) {
   case 0:
   case 1:
      fprintf(fp, "R%d", select);
      case 26:
   case 27:
      fprintf(fp, "AL%d", select);
      case 28:
   case 29:
      fprintf(fp, "%s%d", str, select);
      default:
            }
      static char *srcmod_names_int[4] = {
      ".sext",
   ".zext",
   ".replicate",
      };
      static char *argmod_names[3] = {
      "",
   ".inv",
      };
      static char *index_format_names[4] = {"", ".u64", ".u32", ".s32"};
      static void
   print_alu_outmod(FILE *fp, unsigned outmod, bool is_int, bool half)
   {
      if (is_int && !half) {
      assert(outmod == midgard_outmod_keeplo);
               if (!is_int && half)
               }
      /* arg == 0 (dest), arg == 1 (src1), arg == 2 (src2) */
   static midgard_special_arg_mod
   midgard_alu_special_arg_mod(midgard_alu_op op, unsigned arg)
   {
               switch (op) {
   case midgard_alu_op_ishladd:
   case midgard_alu_op_ishlsub:
      if (arg == 1)
               default:
                     }
      static void
   print_quad_word(FILE *fp, uint32_t *words, unsigned tabs)
   {
               for (i = 0; i < 4; i++)
               }
      static const char components[16] = "xyzwefghijklmnop";
      static int
   bits_for_mode(midgard_reg_mode mode)
   {
      switch (mode) {
   case midgard_reg_mode_8:
         case midgard_reg_mode_16:
         case midgard_reg_mode_32:
         case midgard_reg_mode_64:
         default:
      unreachable("Invalid reg mode");
         }
      static int
   bits_for_mode_halved(midgard_reg_mode mode, bool half)
   {
               if (half)
               }
      static void
   print_vec_selectors_64(FILE *fp, unsigned swizzle, midgard_reg_mode reg_mode,
               {
               unsigned comp_skip = expands ? 1 : 2;
   unsigned mask_bit = 0;
   for (unsigned i = selector_offset; i < 4; i += comp_skip, mask_bit += 4) {
      if (!(mask & (1 << mask_bit)))
                     if (INPUT_EXPANDS(expand_mode)) {
                     fprintf(fp, "%c", components[a / 2]);
                        /* Normally we're adjacent, but if there's an issue,
            if (b == a + 1)
         else
         }
      static void
   print_vec_selectors(FILE *fp, unsigned swizzle, midgard_reg_mode reg_mode,
               {
                                 for (unsigned i = 0; i < 4; i++, *mask_offset += mask_skip) {
      if (!(mask & (1 << *mask_offset)))
                     /* Vec16 has two components per swizzle selector. */
   if (is_vec16)
                     fprintf(fp, "%c", components[c]);
   if (is_vec16)
         }
      static void
   print_vec_swizzle(FILE *fp, unsigned swizzle, midgard_src_expand_mode expand,
         {
               /* Swizzle selectors are divided in two halves that are always
   * mirrored, the only difference is the starting component offset.
   * The number represents an offset into the components[] array. */
   unsigned first_half = 0;
            switch (expand) {
   case midgard_src_passthrough:
      if (swizzle == 0xE4)
               case midgard_src_expand_low:
      second_half /= 2;
         case midgard_src_expand_high:
      first_half = second_half;
   second_half += second_half / 2;
                  case midgard_src_rep_low:
      second_half = 0;
         case midgard_src_rep_high:
      first_half = second_half;
         case midgard_src_swap:
      first_half = second_half;
   second_half = 0;
         case midgard_src_expand_low_swap:
      first_half = second_half / 2;
   second_half = 0;
         case midgard_src_expand_high_swap:
      first_half = second_half + second_half / 2;
         default:
      unreachable("Invalid expand mode");
                        /* Vec2 are weird so we use a separate function to simplify things. */
   if (mode == midgard_reg_mode_64) {
      print_vec_selectors_64(fp, swizzle, mode, expand, first_half, mask);
               unsigned mask_offs = 0;
   print_vec_selectors(fp, swizzle, mode, first_half, mask, &mask_offs);
   if (mode == midgard_reg_mode_8 || mode == midgard_reg_mode_16)
      }
      static void
   print_scalar_constant(FILE *fp, unsigned src_binary,
         {
      midgard_scalar_alu_src *src = (midgard_scalar_alu_src *)&src_binary;
            fprintf(fp, "#");
   mir_print_constant_component(
      fp, consts, src->component,
   src->full ? midgard_reg_mode_32 : midgard_reg_mode_16, false, src->mod,
   }
      static void
   print_vector_constants(FILE *fp, unsigned src_binary,
         {
      midgard_vector_alu_src *src = (midgard_vector_alu_src *)&src_binary;
   bool expands = INPUT_EXPANDS(src->expand_mode);
   unsigned bits = bits_for_mode_halved(alu->reg_mode, expands);
   unsigned max_comp = (sizeof(*consts) * 8) / bits;
            assert(consts);
            comp_mask =
                  if (num_comp > 1)
         else
                     for (unsigned i = 0; i < max_comp; ++i) {
      if (!(comp_mask & (1 << i)))
                     if (bits == 16 && !expands) {
               switch (src->expand_mode) {
   case midgard_src_passthrough:
      c += upper * 4;
      case midgard_src_rep_low:
         case midgard_src_rep_high:
      c += 4;
      case midgard_src_swap:
      c += !upper * 4;
      default:
      unreachable("invalid expand mode");
         } else if (bits == 32 && !expands) {
         } else if (bits == 64 && !expands) {
         } else if (bits == 8 && !expands) {
               unsigned index = (i >> 1) & 3;
                  switch (src->expand_mode) {
   case midgard_src_passthrough:
      c += upper * 8;
      case midgard_src_rep_low:
         case midgard_src_rep_high:
      c += 8;
      case midgard_src_swap:
      c += !upper * 8;
      default:
      unreachable("invalid expand mode");
               /* We work on twos, actually */
   if (i & 1)
               if (first)
         else
            mir_print_constant_component(fp, consts, c, alu->reg_mode, expands,
               if (num_comp > 1)
      }
      static void
   print_srcmod(FILE *fp, bool is_int, bool expands, unsigned mod, bool scalar)
   {
               if (is_int) {
      if (expands)
      } else {
      if (mod & MIDGARD_FLOAT_MOD_ABS)
         if (mod & MIDGARD_FLOAT_MOD_NEG)
         if (expands)
         }
      static void
   print_vector_src(disassemble_context *ctx, FILE *fp, unsigned src_binary,
                     {
                                                      }
      static uint16_t
   decode_vector_imm(unsigned src2_reg, unsigned imm)
   {
      uint16_t ret;
   ret = src2_reg << 11;
   ret |= (imm & 0x7) << 8;
   ret |= (imm >> 3) & 0xFF;
      }
      static void
   print_immediate(FILE *fp, uint16_t imm, bool is_instruction_int)
   {
      if (is_instruction_int)
         else
      }
      static void
   update_dest(disassemble_context *ctx, unsigned reg)
   {
      /* We should record writes as marking this as a work register. Store
            if (reg < 16)
      }
      static void
   print_dest(disassemble_context *ctx, FILE *fp, unsigned reg)
   {
      update_dest(ctx, reg);
      }
      /* For 16-bit+ masks, we read off from the 8-bit mask field. For 16-bit (vec8),
   * it's just one bit per channel, easy peasy. For 32-bit (vec4), it's one bit
   * per channel with one duplicate bit in the middle. For 64-bit (vec2), it's
   * one-bit per channel with _3_ duplicate bits in the middle. Basically, just
   * subdividing the 128-bit word in 16-bit increments. For 64-bit, we uppercase
   * the mask to make it obvious what happened */
      static void
   print_alu_mask(FILE *fp, uint8_t mask, unsigned bits,
         {
               if (shrink_mode == midgard_shrink_mode_none && mask == 0xFF)
                     unsigned skip = MAX2(bits / 16, 1);
            /* To apply an upper destination shrink_mode, we "shift" the alphabet.
   * E.g. with an upper shrink_mode on 32-bit, instead of xyzw, print efgh.
                     if (shrink_mode == midgard_shrink_mode_upper) {
      assert(bits != 8);
               for (unsigned i = 0; i < 8; i += skip) {
               for (unsigned j = 1; j < skip; ++j) {
      bool dupe = (mask & (1 << (i + j))) != 0;
               if (a) {
      /* TODO: handle shrinking from 16-bit */
                  fprintf(fp, "%c", c);
   if (bits == 8)
                  if (tripped)
      }
      /* TODO: 16-bit mode */
   static void
   print_ldst_mask(FILE *fp, unsigned mask, unsigned swizzle)
   {
               for (unsigned i = 0; i < 4; ++i) {
      bool write = (mask & (1 << i)) != 0;
   unsigned c = (swizzle >> (i * 2)) & 3;
   /* We can't omit the swizzle here since many ldst ops have a
   * combined swizzle/writemask, and it would be ambiguous to not
   * print the masked-out components. */
         }
      static void
   print_tex_mask(FILE *fp, unsigned mask, bool upper)
   {
      if (mask == 0xF) {
      if (upper)
                                 for (unsigned i = 0; i < 4; ++i) {
      bool a = (mask & (1 << i)) != 0;
   if (a)
         }
      static void
   print_vector_field(disassemble_context *ctx, FILE *fp, const char *name,
               {
      midgard_reg_info *reg_info = (midgard_reg_info *)&reg_word;
   midgard_vector_alu *alu_field = (midgard_vector_alu *)words;
   midgard_reg_mode mode = alu_field->reg_mode;
   midgard_alu_op op = alu_field->op;
   unsigned shrink_mode = alu_field->shrink_mode;
   bool is_int = midgard_is_integer_op(op);
            if (verbose)
                     /* Print lane width */
                     /* Mask denoting status of 8-lanes */
            /* First, print the destination */
            if (shrink_mode != midgard_shrink_mode_none) {
      bool shrinkable = (mode != midgard_reg_mode_8);
            if (!(shrinkable && known))
               /* Instructions like fdot4 do *not* replicate, ensure the
                     if (rep) {
      unsigned comp_mask = condense_writemask(mask, bits_for_mode(mode));
   unsigned num_comp = util_bitcount(comp_mask);
   if (num_comp != 1)
      }
                     print_alu_outmod(fp, alu_field->outmod, is_int_out,
            /* Mask out unused components based on the writemask, but don't mask out
   * components that are used for interlane instructions like fdot3. */
   uint8_t src_mask =
      rep ? expand_writemask(mask_of(rep),
                        if (reg_info->src1_reg == REGISTER_CONSTANT)
         else {
      midgard_special_arg_mod argmod = midgard_alu_special_arg_mod(op, 1);
   print_vector_src(ctx, fp, alu_field->src1, mode, reg_info->src1_reg,
                        if (reg_info->src2_imm) {
      uint16_t imm =
            } else if (reg_info->src2_reg == REGISTER_CONSTANT) {
         } else {
      midgard_special_arg_mod argmod = midgard_alu_special_arg_mod(op, 2);
   print_vector_src(ctx, fp, alu_field->src2, mode, reg_info->src2_reg,
                  }
      static void
   print_scalar_src(disassemble_context *ctx, FILE *fp, bool is_int,
         {
                                 if (src->full) {
      assert((c & 1) == 0);
                           }
      static uint16_t
   decode_scalar_imm(unsigned src2_reg, unsigned imm)
   {
      uint16_t ret;
   ret = src2_reg << 11;
   ret |= (imm & 3) << 9;
   ret |= (imm & 4) << 6;
   ret |= (imm & 0x38) << 2;
   ret |= imm >> 6;
      }
      static void
   print_scalar_field(disassemble_context *ctx, FILE *fp, const char *name,
               {
      midgard_reg_info *reg_info = (midgard_reg_info *)&reg_word;
   midgard_scalar_alu *alu_field = (midgard_scalar_alu *)words;
   bool is_int = midgard_is_integer_op(alu_field->op);
   bool is_int_out = midgard_is_integer_out_op(alu_field->op);
            if (alu_field->reserved)
            if (verbose)
                     /* Print lane width, in this case the lane width is always 32-bit, but
   * we print it anyway to make it consistent with the other instructions. */
                     print_dest(ctx, fp, reg_info->out_reg);
            if (full) {
      assert((c & 1) == 0);
                                          if (reg_info->src1_reg == REGISTER_CONSTANT)
         else
                     if (reg_info->src2_imm) {
      uint16_t imm = decode_scalar_imm(reg_info->src2_reg, alu_field->src2);
      } else if (reg_info->src2_reg == REGISTER_CONSTANT) {
         } else
               }
      static void
   print_branch_op(FILE *fp, unsigned op)
   {
      switch (op) {
   case midgard_jmp_writeout_op_branch_uncond:
      fprintf(fp, "uncond.");
         case midgard_jmp_writeout_op_branch_cond:
      fprintf(fp, "cond.");
         case midgard_jmp_writeout_op_writeout:
      fprintf(fp, "write.");
         case midgard_jmp_writeout_op_tilebuffer_pending:
      fprintf(fp, "tilebuffer.");
         case midgard_jmp_writeout_op_discard:
      fprintf(fp, "discard.");
         default:
      fprintf(fp, "unk%u.", op);
         }
      static void
   print_branch_cond(FILE *fp, int cond)
   {
      switch (cond) {
   case midgard_condition_write0:
      fprintf(fp, "write0");
         case midgard_condition_false:
      fprintf(fp, "false");
         case midgard_condition_true:
      fprintf(fp, "true");
         case midgard_condition_always:
      fprintf(fp, "always");
         default:
      fprintf(fp, "unk%X", cond);
         }
      static const char *
   function_call_mode(enum midgard_call_mode mode)
   {
      switch (mode) {
   case midgard_call_mode_default:
         case midgard_call_mode_call:
         case midgard_call_mode_return:
         default:
            }
      static bool
   print_compact_branch_writeout_field(disassemble_context *ctx, FILE *fp,
         {
               switch (op) {
   case midgard_jmp_writeout_op_branch_uncond: {
      midgard_branch_uncond br_uncond;
   memcpy((char *)&br_uncond, (char *)&word, sizeof(br_uncond));
            if (br_uncond.offset >= 0)
            fprintf(fp, "%d -> %s", br_uncond.offset,
                              case midgard_jmp_writeout_op_branch_cond:
   case midgard_jmp_writeout_op_writeout:
   case midgard_jmp_writeout_op_discard:
   default: {
      midgard_branch_cond br_cond;
                     print_branch_op(fp, br_cond.op);
                     if (br_cond.offset >= 0)
            fprintf(fp, "%d -> %s", br_cond.offset,
                     }
               }
      static bool
   print_extended_branch_writeout_field(disassemble_context *ctx, FILE *fp,
         {
      midgard_branch_extended br;
                              /* Condition codes are a LUT in the general case, but simply repeated 8 times
                     for (unsigned i = 0; i < 16; i += 2) {
                  if (single_channel)
         else
                     if (br.offset >= 0)
                              if (ctx->midg_tags[I] && ctx->midg_tags[I] != br.dest_tag) {
      fprintf(fp, "\t/* XXX TAG ERROR: jumping to %s but tagged %s \n",
                                 }
      static unsigned
   num_alu_fields_enabled(uint32_t control_word)
   {
               if ((control_word >> 17) & 1)
            if ((control_word >> 19) & 1)
            if ((control_word >> 21) & 1)
            if ((control_word >> 23) & 1)
            if ((control_word >> 25) & 1)
               }
      static bool
   print_alu_word(disassemble_context *ctx, FILE *fp, uint32_t *words,
               {
      uint32_t control_word = words[0];
   uint16_t *beginning_ptr = (uint16_t *)(words + 1);
   unsigned num_fields = num_alu_fields_enabled(control_word);
   uint16_t *word_ptr = beginning_ptr + num_fields;
   unsigned num_words = 2 + num_fields;
   const midgard_constants *consts = NULL;
            if ((control_word >> 17) & 1)
            if ((control_word >> 19) & 1)
            if ((control_word >> 21) & 1)
            if ((control_word >> 23) & 1)
            if ((control_word >> 25) & 1)
            if ((control_word >> 26) & 1)
            if ((control_word >> 27) & 1)
            if (num_quad_words > (num_words + 7) / 8) {
      assert(num_quad_words == (num_words + 15) / 8);
   // Assume that the extra quadword is constants
               if ((control_word >> 16) & 1)
            if ((control_word >> 17) & 1) {
      print_vector_field(ctx, fp, "vmul", word_ptr, *beginning_ptr, consts,
         beginning_ptr += 1;
               if ((control_word >> 18) & 1)
            if ((control_word >> 19) & 1) {
      print_scalar_field(ctx, fp, "sadd", word_ptr, *beginning_ptr, consts,
         beginning_ptr += 1;
               if ((control_word >> 20) & 1)
            if ((control_word >> 21) & 1) {
      print_vector_field(ctx, fp, "vadd", word_ptr, *beginning_ptr, consts,
         beginning_ptr += 1;
               if ((control_word >> 22) & 1)
            if ((control_word >> 23) & 1) {
      print_scalar_field(ctx, fp, "smul", word_ptr, *beginning_ptr, consts,
         beginning_ptr += 1;
               if ((control_word >> 24) & 1)
            if ((control_word >> 25) & 1) {
      print_vector_field(ctx, fp, "lut", word_ptr, *beginning_ptr, consts, tabs,
                     if ((control_word >> 26) & 1) {
      branch_forward |= print_compact_branch_writeout_field(ctx, fp, *word_ptr);
               if ((control_word >> 27) & 1) {
      branch_forward |= print_extended_branch_writeout_field(
                     if (consts)
      fprintf(fp, "uconstants 0x%X, 0x%X, 0x%X, 0x%X\n", consts->u32[0],
            }
      /* TODO: how can we use this now that we know that these params can't be known
   * before run time in every single case? Maybe just use it in the cases we can? */
   UNUSED static void
   print_varying_parameters(FILE *fp, midgard_load_store_word *word)
   {
               /* If a varying, there are qualifiers */
   if (p.flat_shading)
            if (p.perspective_correction)
            if (p.centroid_mapping)
            if (p.interpolate_sample)
            switch (p.modifier) {
   case midgard_varying_mod_perspective_y:
      fprintf(fp, ".perspectivey");
      case midgard_varying_mod_perspective_z:
      fprintf(fp, ".perspectivez");
      case midgard_varying_mod_perspective_w:
      fprintf(fp, ".perspectivew");
      default:
      unreachable("invalid varying modifier");
         }
      /* Helper to print integer well-formatted, but only when non-zero. */
   static void
   midgard_print_sint(FILE *fp, int n)
   {
      if (n > 0)
         else if (n < 0)
      }
      static void
   print_load_store_instr(disassemble_context *ctx, FILE *fp, uint64_t data,
         {
                        if (word->op == midgard_op_trap) {
      fprintf(fp, " 0x%X\n", word->signed_offset);
                        if (OP_USES_ATTRIB(word->op)) {
      /* Print non-default attribute tables */
   bool default_secondary = (word->op == midgard_op_st_vary_32) ||
                           (word->op == midgard_op_st_vary_16) ||
   (word->op == midgard_op_st_vary_32u) ||
            bool default_primary = (word->op == midgard_op_ld_attr_32) ||
                        bool has_default = (default_secondary || default_primary);
   bool auto32 = (word->index_format >> 0) & 1;
            if (auto32)
            if (has_default && (is_secondary != default_secondary))
      } else if (word->op == midgard_op_ld_cubemap_coords ||
                                    if (!OP_IS_STORE(word->op)) {
               /* Some opcodes don't have a swizzable src register, and
   * instead the swizzle is applied before the result is written
   * to the dest reg. For these ops, we combine the writemask
   * with the swizzle to display them in the disasm compactly. */
   unsigned swizzle = word->swizzle;
   if ((OP_IS_REG2REG_LDST(word->op) && word->op != midgard_op_lea &&
      word->op != midgard_op_lea_image) ||
   OP_IS_ATOMIC(word->op))
         } else {
      uint8_t mask = (word->mask & 0x1) | ((word->mask & 0x2) << 1) |
         mask |= mask << 1;
   print_ldst_read_reg(fp, word->reg);
   print_vec_swizzle(fp, word->swizzle, midgard_src_passthrough,
               /* ld_ubo args */
   if (OP_IS_UBO_READ(word->op)) {
      if (word->signed_offset & 1) { /* buffer index imm */
      unsigned imm = midgard_unpack_ubo_index_imm(*word);
      } else { /* buffer index from reg */
      fprintf(fp, ", ");
   print_ldst_read_reg(fp, word->arg_reg);
               fprintf(fp, ", ");
   print_ldst_read_reg(fp, word->index_reg);
   fprintf(fp, ".%c", components[word->index_comp]);
   if (word->index_shift)
                     /* mem addr expression */
   if (OP_HAS_ADDRESS(word->op)) {
      fprintf(fp, ", ");
            /* Skip printing zero */
   if (word->arg_reg != 7 || verbose) {
      print_ldst_read_reg(fp, word->arg_reg);
   fprintf(fp, ".u%d.%c", word->bitsize_toggle ? 64 : 32,
                     if ((word->op < midgard_op_atomic_cmpxchg ||
      word->op > midgard_op_atomic_cmpxchg64_be) &&
   word->index_reg != 0x7) {
                  print_ldst_read_reg(fp, word->index_reg);
   fprintf(fp, "%s.%c", index_format_names[word->index_format],
         if (word->index_shift)
                           /* src reg for reg2reg ldst opcodes */
   if (OP_IS_REG2REG_LDST(word->op)) {
      fprintf(fp, ", ");
   print_ldst_read_reg(fp, word->arg_reg);
   print_vec_swizzle(fp, word->swizzle, midgard_src_passthrough,
               /* atomic ops encode the source arg where the ldst swizzle would be. */
   if (OP_IS_ATOMIC(word->op)) {
      unsigned src = (word->swizzle >> 2) & 0x7;
   unsigned src_comp = word->swizzle & 0x3;
   fprintf(fp, ", ");
   print_ldst_read_reg(fp, src);
               /* CMPXCHG encodes the extra comparison arg where the index reg would be. */
   if (word->op >= midgard_op_atomic_cmpxchg &&
      word->op <= midgard_op_atomic_cmpxchg64_be) {
   fprintf(fp, ", ");
   print_ldst_read_reg(fp, word->index_reg);
               /* index reg for attr/vary/images, selector for ld/st_special */
   if (OP_IS_SPECIAL(word->op) || OP_USES_ATTRIB(word->op)) {
      fprintf(fp, ", ");
   print_ldst_read_reg(fp, word->index_reg);
   fprintf(fp, ".%c", components[word->index_comp]);
   if (word->index_shift)
                     /* vertex reg for attrib/varying ops, coord reg for image ops */
   if (OP_USES_ATTRIB(word->op)) {
      fprintf(fp, ", ");
            if (OP_IS_IMAGE(word->op))
                     if (word->bitsize_toggle && !OP_IS_IMAGE(word->op))
               /* TODO: properly decode format specifier for PACK/UNPACK ops */
   if (OP_IS_PACK_COLOUR(word->op) || OP_IS_UNPACK_COLOUR(word->op)) {
      fprintf(fp, ", ");
   unsigned format_specifier =
                                       if (!OP_IS_STORE(word->op))
      }
      static void
   print_load_store_word(disassemble_context *ctx, FILE *fp, uint32_t *word,
         {
               if (load_store->word1 != 3) {
                  if (load_store->word2 != 3) {
            }
      static void
   print_texture_reg_select(FILE *fp, uint8_t u, unsigned base)
   {
      midgard_tex_register_select sel;
                              /* Use the upper half in half-reg mode */
   if (sel.upper) {
      assert(!sel.full);
                           }
      static void
   print_texture_format(FILE *fp, int format)
   {
      /* Act like a modifier */
            switch (format) {
      DEFINE_CASE(1, "1d");
   DEFINE_CASE(2, "2d");
   DEFINE_CASE(3, "3d");
         default:
            }
      static void
   print_texture_op(FILE *fp, unsigned op)
   {
      if (tex_opcode_props[op].name)
         else
      }
      static bool
   texture_op_takes_bias(unsigned op)
   {
         }
      static char
   sampler_type_name(enum mali_sampler_type t)
   {
      switch (t) {
   case MALI_SAMPLER_FLOAT:
         case MALI_SAMPLER_UNSIGNED:
         case MALI_SAMPLER_SIGNED:
         default:
            }
      static void
   print_texture_barrier(FILE *fp, uint32_t *word)
   {
               if (barrier->type != TAG_TEXTURE_4_BARRIER)
            if (!barrier->cont)
            if (!barrier->last)
            if (barrier->zero1)
            if (barrier->zero2)
            if (barrier->zero3)
            if (barrier->zero4)
            if (barrier->zero5)
            if (barrier->out_of_order)
               }
      #undef DEFINE_CASE
      static const char *
   texture_mode(enum mali_texture_mode mode)
   {
      switch (mode) {
   case TEXTURE_NORMAL:
         case TEXTURE_SHADOW:
         case TEXTURE_GATHER_SHADOW:
         case TEXTURE_GATHER_X:
         case TEXTURE_GATHER_Y:
         case TEXTURE_GATHER_Z:
         case TEXTURE_GATHER_W:
         default:
            }
      static const char *
   derivative_mode(enum mali_derivative_mode mode)
   {
      switch (mode) {
   case TEXTURE_DFDX:
         case TEXTURE_DFDY:
         default:
            }
      static const char *
   partial_exection_mode(enum midgard_partial_execution mode)
   {
      switch (mode) {
   case MIDGARD_PARTIAL_EXECUTION_NONE:
         case MIDGARD_PARTIAL_EXECUTION_SKIP:
         case MIDGARD_PARTIAL_EXECUTION_KILL:
         default:
            }
      static void
   print_texture_word(disassemble_context *ctx, FILE *fp, uint32_t *word,
         {
      midgard_texture_word *texture = (midgard_texture_word *)word;
            /* Broad category of texture operation in question */
            /* Barriers use a dramatically different code path */
   if (texture->op == midgard_tex_op_barrier) {
      print_texture_barrier(fp, word);
      } else if (texture->type == TAG_TEXTURE_4_BARRIER)
         else if (texture->type == TAG_TEXTURE_4_VTX)
            if (texture->op == midgard_tex_op_derivative)
         else
            /* Specific format in question */
            /* Instruction "modifiers" parallel the ALU instructions. */
            if (texture->out_of_order)
            fprintf(fp, " ");
   print_tex_reg(fp, out_reg_base + texture->out_reg_select, true);
   print_tex_mask(fp, texture->mask, texture->out_upper);
   fprintf(fp, ".%c%d", texture->sampler_type == MALI_SAMPLER_FLOAT ? 'f' : 'i',
                  /* Output modifiers are only valid for float texture operations */
   if (texture->sampler_type == MALI_SAMPLER_FLOAT)
                     /* Depending on whether we read from textures directly or indirectly,
            if (texture->texture_register) {
      fprintf(fp, "texture[");
   print_texture_reg_select(fp, texture->texture_handle, in_reg_base);
      } else {
                  /* Print the type, GL style */
            if (texture->sampler_register) {
      fprintf(fp, "[");
   print_texture_reg_select(fp, texture->sampler_handle, in_reg_base);
      } else {
                  print_vec_swizzle(fp, texture->swizzle, midgard_src_passthrough,
                     midgard_src_expand_mode exp =
         print_tex_reg(fp, in_reg_base + texture->in_reg_select, false);
   print_vec_swizzle(fp, texture->in_reg_swizzle, exp, midgard_reg_mode_32,
         fprintf(fp, ".%d", texture->in_reg_full ? 32 : 16);
            /* There is *always* an offset attached. Of
   * course, that offset is just immediate #0 for a
   * GLES call that doesn't take an offset. If there
   * is a non-negative non-zero offset, this is
   * specified in immediate offset mode, with the
   * values in the offset_* fields as immediates. If
   * this is a negative offset, we instead switch to
   * a register offset mode, where the offset_*
            if (texture->offset_register) {
               bool full = texture->offset & 1;
   bool select = texture->offset & 2;
   bool upper = texture->offset & 4;
   unsigned swizzle = texture->offset >> 3;
   midgard_src_expand_mode exp =
            print_tex_reg(fp, in_reg_base + select, false);
   print_vec_swizzle(fp, swizzle, exp, midgard_reg_mode_32, 0xFF);
   fprintf(fp, ".%d", full ? 32 : 16);
               } else if (texture->offset) {
               signed offset_x = (texture->offset & 0xF);
   signed offset_y = ((texture->offset >> 4) & 0xF);
            bool neg_x = offset_x < 0;
   bool neg_y = offset_y < 0;
   bool neg_z = offset_z < 0;
            if (any_neg && texture->op != midgard_tex_op_fetch)
                        } else {
                           if (texture->lod_register) {
      fprintf(fp, "lod %c ", lod_operand);
   print_texture_reg_select(fp, texture->bias, in_reg_base);
            if (texture->bias_int)
      } else if (texture->op == midgard_tex_op_fetch) {
      /* For texel fetch, the int LOD is in the fractional place and
   * there is no fraction. We *always* have an explicit LOD, even
            if (texture->bias_int)
               } else if (texture->bias || texture->bias_int) {
      signed bias_int = texture->bias_int;
   float bias_frac = texture->bias / 256.0f;
            bool is_bias = texture_op_takes_bias(texture->op);
   char sign = (bias >= 0.0) ? '+' : '-';
                                 /* While not zero in general, for these simple instructions the
            if (texture->unknown4 || texture->unknown8) {
      fprintf(fp, "// unknown4 = 0x%x\n", texture->unknown4);
         }
      void
   disassemble_midgard(FILE *fp, uint8_t *code, size_t size, unsigned gpu_id,
         {
      uint32_t *words = (uint32_t *)code;
   unsigned num_words = size / 4;
                                       disassemble_context ctx = {
      .midg_tags = calloc(sizeof(ctx.midg_tags[0]), num_words),
               while (i < num_words) {
      unsigned tag = words[i] & 0xF;
   unsigned next_tag = (words[i] >> 4) & 0xF;
            if (ctx.midg_tags[i] && ctx.midg_tags[i] != tag) {
      fprintf(fp, "\t/* XXX: TAG ERROR branch, got %s expected %s */\n",
                              /* Check the tag. The idea is to ensure that next_tag is
   * *always* recoverable from the disassembly, such that we may
   * safely omit printing next_tag. To show this, we first
   * consider that next tags are semantically off-byone -- we end
   * up parsing tag n during step n+1. So, we ensure after we're
   * done disassembling the next tag of the final bundle is BREAK
   * and warn otherwise. We also ensure that the next tag is
   * never INVALID. Beyond that, since the last tag is checked
   * outside the loop, we can check one tag prior. If equal to
   * the current tag (which is unique), we're done. Otherwise, we
   * print if that tag was > TAG_BREAK, which implies the tag was
   * not TAG_BREAK or TAG_INVALID. But we already checked for
   * TAG_INVALID, so it's just if the last tag was TAG_BREAK that
   * we're silent. So we throw in a print for break-next on at
   * the end of the bundle (if it's not the final bundle, which
   * we already check for above), disambiguating this case as
            if (next_tag == TAG_INVALID)
            if (last_next_tag > TAG_BREAK && last_next_tag != tag) {
      fprintf(fp, "\t/* XXX: TAG ERROR sequence, got %s expexted %s */\n",
                              /* Tags are unique in the following way:
   *
   * INVALID, BREAK, UNKNOWN_*: verbosely printed
   * TEXTURE_4_BARRIER: verified by barrier/!barrier op
   * TEXTURE_4_VTX: .vtx tag printed
   * TEXTURE_4: tetxure lack of barriers or .vtx
   * TAG_LOAD_STORE_4: only load/store
   * TAG_ALU_4/8/12/16: by number of instructions/constants
   * TAG_ALU_4_8/12/16_WRITEOUT: ^^ with .writeout tag
            switch (tag) {
   case TAG_TEXTURE_4_VTX ... TAG_TEXTURE_4_BARRIER: {
                     print_texture_word(
      &ctx, fp, &words[i], tabs, interpipe_aliasing ? 0 : REG_TEX_BASE,
                  case TAG_LOAD_STORE_4:
                  case TAG_ALU_4 ... TAG_ALU_16_WRITEOUT:
                     /* TODO: infer/verify me */
                        default:
      fprintf(fp, "Unknown word type %u:\n", words[i] & 0xF);
   num_quad_words = 1;
   print_quad_word(fp, &words[i], tabs);
   fprintf(fp, "\n");
               /* Include a synthetic "break" instruction at the end of the
   * bundle to signify that if, absent a branch, the shader
   * execution will stop here. Stop disassembly at such a break
            if (next_tag == TAG_BREAK) {
      if (branch_forward) {
         } else {
      fprintf(fp, "\n");
                                       if (last_next_tag != TAG_BREAK) {
      fprintf(fp, "/* XXX: shader ended with tag %s */\n",
                  }
