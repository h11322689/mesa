   /*
   * Copyright (c) 2018 Lima Project
   *
   * Copyright (c) 2013 Codethink (http://www.codethink.co.uk)
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sub license,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   *
   */
      #include "util/half_float.h"
      #include "ppir.h"
   #include "codegen.h"
      typedef struct {
      char *name;
      } asm_op;
      static void
   print_swizzle(uint8_t swizzle, FILE *fp)
   {
      if (swizzle == 0xE4)
            fprintf(fp, ".");
   for (unsigned i = 0; i < 4; i++, swizzle >>= 2)
      }
      static void
   print_mask(uint8_t mask, FILE *fp)
   {
      if (mask == 0xF)
            fprintf(fp, ".");
   if (mask & 1) fprintf(fp, "x");
   if (mask & 2) fprintf(fp, "y");
   if (mask & 4) fprintf(fp, "z");
      }
      static void
   print_reg(ppir_codegen_vec4_reg reg, const char *special, FILE *fp)
   {
      if (special) {
         } else {
      switch (reg)
   {
      case ppir_codegen_vec4_reg_constant0:
      fprintf(fp, "^const0");
      case ppir_codegen_vec4_reg_constant1:
      fprintf(fp, "^const1");
      case ppir_codegen_vec4_reg_texture:
      fprintf(fp, "^texture");
      case ppir_codegen_vec4_reg_uniform:
      fprintf(fp, "^uniform");
      default:
      fprintf(fp, "$%u", reg);
         }
      static void
   print_vector_source(ppir_codegen_vec4_reg reg, const char *special,
         {
      if (neg)
         if (abs)
            print_reg(reg, special, fp);
            if (abs)
      }
      static void
   print_source_scalar(unsigned reg, const char *special, bool abs, bool neg, FILE *fp)
   {
      if (neg)
         if (abs)
            print_reg(reg >> 2, special, fp);
   if (!special)
            if (abs)
      }
      static void
   print_varying_source(ppir_codegen_field_varying *varying, FILE *fp)
   {
      switch (varying->imm.alignment) {
   case 0:
      fprintf(fp, "%u.%c", varying->imm.index >> 2,
            case 1: {
      const char *c[2] = {"xy", "zw"};
   fprintf(fp, "%u.%s", varying->imm.index >> 1, c[varying->imm.index & 1]);
      }
   default:
      fprintf(fp, "%u", varying->imm.index);
               if (varying->imm.offset_vector != 15) {
      unsigned reg = (varying->imm.offset_vector << 2) +
         fprintf(fp, "+");
         }
      static void
   print_outmod(ppir_codegen_outmod modifier, FILE *fp)
   {
      switch (modifier)
   {
      case ppir_codegen_outmod_clamp_fraction:
      fprintf(fp, ".sat");
      case ppir_codegen_outmod_clamp_positive:
      fprintf(fp, ".pos");
      case ppir_codegen_outmod_round:
      fprintf(fp, ".int");
      default:
         }
      static void
   print_dest_scalar(unsigned reg, FILE *fp)
   {
      fprintf(fp, "$%u", reg >> 2);
      }
      static void
   print_const(unsigned const_num, uint16_t *val, FILE *fp)
   {
      fprintf(fp, "const%u", const_num);
   for (unsigned i = 0; i < 4; i++)
      }
      static void
   print_const0(void *code, unsigned offset, FILE *fp)
   {
                  }
      static void
   print_const1(void *code, unsigned offset, FILE *fp)
   {
                  }
      static void
   print_varying(void *code, unsigned offset, FILE *fp)
   {
      (void) offset;
                     bool perspective = varying->imm.source_type < 2 && varying->imm.perspective;
   if (perspective)
   {
      fprintf(fp, ".perspective");
   switch (varying->imm.perspective)
   {
   case 2:
      fprintf(fp, ".z");
      case 3:
      fprintf(fp, ".w");
      default:
      fprintf(fp, ".unknown");
                           switch (varying->imm.dest)
   {
   case ppir_codegen_vec4_reg_discard:
      fprintf(fp, "^discard");
      default:
      fprintf(fp, "$%u", varying->imm.dest);
      }
   print_mask(varying->imm.mask, fp);
            switch (varying->imm.source_type) {
   case 1:
      print_vector_source(varying->reg.source, NULL, varying->reg.swizzle,
            case 2:
      switch (varying->imm.perspective) {
   case 0:
      fprintf(fp, "cube(");
   print_varying_source(varying, fp);
   fprintf(fp, ")");
      case 1:
      fprintf(fp, "cube(");
   print_vector_source(varying->reg.source, NULL, varying->reg.swizzle,
         fprintf(fp, ")");
      case 2:
      fprintf(fp, "normalize(");
   print_vector_source(varying->reg.source, NULL, varying->reg.swizzle,
         fprintf(fp, ")");
      default:
      fprintf(fp, "gl_FragCoord");
      }
      case 3:
      if (varying->imm.perspective)
         else
            default:
      print_varying_source(varying, fp);
         }
      static void
   print_sampler(void *code, unsigned offset, FILE *fp)
   {
      (void) offset;
            fprintf(fp, "texld");
   if (sampler->lod_bias_en)
            switch (sampler->type) {
   case ppir_codegen_sampler_type_generic:
         case ppir_codegen_sampler_type_cube:
      fprintf(fp, ".cube");
      default:
      fprintf(fp, "_t%u", sampler->type);
                        if (sampler->offset_en)
   {
      fprintf(fp, "+");
               if (sampler->lod_bias_en)
   {
      fprintf(fp, " ");
         }
      static void
   print_uniform(void *code, unsigned offset, FILE *fp)
   {
      (void) offset;
                     switch (uniform->source) {
   case ppir_codegen_uniform_src_uniform:
      fprintf(fp, "u");
      case ppir_codegen_uniform_src_temporary:
      fprintf(fp, "t");
      default:
      fprintf(fp, ".u%u", uniform->source);
               int16_t index = uniform->index;
   switch (uniform->alignment) {
   case 2:
      fprintf(fp, " %d", index);
      case 1:
      fprintf(fp, " %d.%s", index / 2, (index & 1) ? "zw" : "xy");
      default:
      fprintf(fp, " %d.%c", index / 4, "xyzw"[index & 3]);
               if (uniform->offset_en) {
      fprintf(fp, "+");
         }
      #define CASE(_name, _srcs) \
   [ppir_codegen_vec4_mul_op_##_name] = { \
      .name = #_name, \
      }
      static const asm_op vec4_mul_ops[] = {
      [0 ... 7] = {
      .name = "mul",
      },
   CASE(not, 1),
   CASE(and, 2),
   CASE(or, 2),
   CASE(xor, 2),
   CASE(ne, 2),
   CASE(gt, 2),
   CASE(ge, 2),
   CASE(eq, 2),
   CASE(min, 2),
   CASE(max, 2),
      };
      #undef CASE
      static void
   print_vec4_mul(void *code, unsigned offset, FILE *fp)
   {
      (void) offset;
                     if (op.name)
         else
         print_outmod(vec4_mul->dest_modifier, fp);
            if (vec4_mul->mask) {
      fprintf(fp, "$%u", vec4_mul->dest);
   print_mask(vec4_mul->mask, fp);
               print_vector_source(vec4_mul->arg0_source, NULL,
                        if (vec4_mul->op < 8 && vec4_mul->op != 0) {
                           if (op.srcs > 1) {
      print_vector_source(vec4_mul->arg1_source, NULL,
                     }
      #define CASE(_name, _srcs) \
   [ppir_codegen_vec4_acc_op_##_name] = { \
      .name = #_name, \
      }
      static const asm_op vec4_acc_ops[] = {
      CASE(add, 2),
   CASE(fract, 1),
   CASE(ne, 2),
   CASE(gt, 2),
   CASE(ge, 2),
   CASE(eq, 2),
   CASE(floor, 1),
   CASE(ceil, 1),
   CASE(min, 2),
   CASE(max, 2),
   CASE(sum3, 1),
   CASE(sum4, 1),
   CASE(dFdx, 2),
   CASE(dFdy, 2),
   CASE(sel, 2),
      };
      #undef CASE
      static void
   print_vec4_acc(void *code, unsigned offset, FILE *fp)
   {
      (void) offset;
                     if (op.name)
         else
         print_outmod(vec4_acc->dest_modifier, fp);
            if (vec4_acc->mask) {
      fprintf(fp, "$%u", vec4_acc->dest);
   print_mask(vec4_acc->mask, fp);
               print_vector_source(vec4_acc->arg0_source, vec4_acc->mul_in ? "^v0" : NULL,
                        if (op.srcs > 1) {
      fprintf(fp, " ");
   print_vector_source(vec4_acc->arg1_source, NULL,
                     }
      #define CASE(_name, _srcs) \
   [ppir_codegen_float_mul_op_##_name] = { \
      .name = #_name, \
      }
      static const asm_op float_mul_ops[] = {
      [0 ... 7] = {
      .name = "mul",
      },
   CASE(not, 1),
   CASE(and, 2),
   CASE(or, 2),
   CASE(xor, 2),
   CASE(ne, 2),
   CASE(gt, 2),
   CASE(ge, 2),
   CASE(eq, 2),
   CASE(min, 2),
   CASE(max, 2),
      };
      #undef CASE
      static void
   print_float_mul(void *code, unsigned offset, FILE *fp)
   {
      (void) offset;
                     if (op.name)
         else
         print_outmod(float_mul->dest_modifier, fp);
            if (float_mul->output_en)
            print_source_scalar(float_mul->arg0_source, NULL,
                  if (float_mul->op < 8 && float_mul->op != 0) {
                  if (op.srcs > 1) {
               print_source_scalar(float_mul->arg1_source, NULL,
               }
      #define CASE(_name, _srcs) \
   [ppir_codegen_float_acc_op_##_name] = { \
      .name = #_name, \
      }
      static const asm_op float_acc_ops[] = {
      CASE(add, 2),
   CASE(fract, 1),
   CASE(ne, 2),
   CASE(gt, 2),
   CASE(ge, 2),
   CASE(eq, 2),
   CASE(floor, 1),
   CASE(ceil, 1),
   CASE(min, 2),
   CASE(max, 2),
   CASE(dFdx, 2),
   CASE(dFdy, 2),
   CASE(sel, 2),
      };
      #undef CASE
      static void
   print_float_acc(void *code, unsigned offset, FILE *fp)
   {
      (void) offset;
                     if (op.name)
         else
         print_outmod(float_acc->dest_modifier, fp);
            if (float_acc->output_en)
            print_source_scalar(float_acc->arg0_source, float_acc->mul_in ? "^s0" : NULL,
                  if (op.srcs > 1) {
      fprintf(fp, " ");
   print_source_scalar(float_acc->arg1_source, NULL,
               }
      #define CASE(_name, _srcs) \
   [ppir_codegen_combine_scalar_op_##_name] = { \
      .name = #_name, \
      }
      static const asm_op combine_ops[] = {
      CASE(rcp, 1),
   CASE(mov, 1),
   CASE(sqrt, 1),
   CASE(rsqrt, 1),
   CASE(exp2, 1),
   CASE(log2, 1),
   CASE(sin, 1),
   CASE(cos, 1),
   CASE(atan, 1),
      };
      #undef CASE
      static void
   print_combine(void *code, unsigned offset, FILE *fp)
   {
      (void) offset;
            if (combine->scalar.dest_vec &&
      combine->scalar.arg1_en) {
   /* This particular combination can only be valid for scalar * vector
   * multiplies, and the opcode field is reused for something else.
   */
      } else {
               if (op.name)
         else
               if (!combine->scalar.dest_vec)
                  if (combine->scalar.dest_vec) {
      fprintf(fp, "$%u", combine->vector.dest);
      } else {
         }
            print_source_scalar(combine->scalar.arg0_src, NULL,
                        if (combine->scalar.arg1_en) {
      if (combine->scalar.dest_vec) {
      print_vector_source(combine->vector.arg1_source, NULL,
            } else {
      print_source_scalar(combine->scalar.arg1_src, NULL,
                  }
      static void
   print_temp_write(void *code, unsigned offset, FILE *fp)
   {
      (void) offset;
            if (temp_write->fb_read.unknown_0 == 0x7) {
      if (temp_write->fb_read.source)
         else
                                       int16_t index = temp_write->temp_write.index;
   switch (temp_write->temp_write.alignment) {
   case 2:
      fprintf(fp, " %d", index);
      case 1:
      fprintf(fp, " %d.%s", index / 2, (index & 1) ? "zw" : "xy");
      default:
      fprintf(fp, " %d.%c", index / 4, "xyzw"[index & 3]);
               if (temp_write->temp_write.offset_en) {
      fprintf(fp, "+");
   print_source_scalar(temp_write->temp_write.offset_reg,
                        if (temp_write->temp_write.alignment) {
         } else {
            }
      static void
   print_branch(void *code, unsigned offset, FILE *fp)
   {
               if (branch->discard.word0 == PPIR_CODEGEN_DISCARD_WORD0 &&
      branch->discard.word1 == PPIR_CODEGEN_DISCARD_WORD1 &&
   branch->discard.word2 == PPIR_CODEGEN_DISCARD_WORD2) {
   fprintf(fp, "discard");
               const char* cond[] = {
      "nv", "lt", "eq", "le",
               unsigned cond_mask = 0;
   cond_mask |= (branch->branch.cond_lt ? 1 : 0);
   cond_mask |= (branch->branch.cond_eq ? 2 : 0);
   cond_mask |= (branch->branch.cond_gt ? 4 : 0);
   fprintf(fp, "branch");
   if (cond_mask != 0x7) {
      fprintf(fp, ".%s ", cond[cond_mask]);
   print_source_scalar(branch->branch.arg0_source, NULL, false, false, fp);
   fprintf(fp, " ");
                  }
      typedef void (*print_field_func)(void *, unsigned, FILE *);
      static const print_field_func print_field[ppir_codegen_field_shift_count] = {
      [ppir_codegen_field_shift_varying] = print_varying,
   [ppir_codegen_field_shift_sampler] = print_sampler,
   [ppir_codegen_field_shift_uniform] = print_uniform,
   [ppir_codegen_field_shift_vec4_mul] = print_vec4_mul,
   [ppir_codegen_field_shift_float_mul] = print_float_mul,
   [ppir_codegen_field_shift_vec4_acc] = print_vec4_acc,
   [ppir_codegen_field_shift_float_acc] = print_float_acc,
   [ppir_codegen_field_shift_combine] = print_combine,
   [ppir_codegen_field_shift_temp_write] = print_temp_write,
   [ppir_codegen_field_shift_branch] = print_branch,
   [ppir_codegen_field_shift_vec4_const_0] = print_const0,
      };
      static const int ppir_codegen_field_size[] = {
         };
      static void
   bitcopy(unsigned char *src, unsigned char *dst, unsigned bits, unsigned src_offset)
   {
      src += src_offset / 8;
            for (unsigned b = bits; b > 0; b -= MIN2(b, 8), src++, dst++) {
      unsigned char out = *src >> src_offset;
   if (src_offset > 0 && src_offset + b > 8)
               }
      void
   ppir_disassemble_instr(uint32_t *instr, unsigned offset, FILE *fp)
   {
               unsigned char *instr_code = (unsigned char *) (instr + 1);
   unsigned bit_offset = 0;
   bool first = true;
   for (unsigned i = 0; i < ppir_codegen_field_shift_count; i++) {
               if (!((ctrl->fields >> i) & 1))
            unsigned bits = ppir_codegen_field_size[i];
            if (first)
         else
                                 if (ctrl->sync)
         if (ctrl->stop)
               }
   