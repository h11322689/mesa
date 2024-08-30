   /*
   * Copyright (C) 2016 Miklós Máté
   * Copyright (C) 2020 Google LLC
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included in
   * all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   * OTHER DEALINGS IN THE SOFTWARE.
   */
      #include "main/mtypes.h"
   #include "main/atifragshader.h"
   #include "main/errors.h"
   #include "program/prog_parameter.h"
   #include "program/prog_instruction.h"
   #include "program/prog_to_nir.h"
      #include "st_program.h"
   #include "st_atifs_to_nir.h"
   #include "compiler/nir/nir_builder.h"
      /**
   * Intermediate state used during shader translation.
   */
   struct st_translate {
      nir_builder *b;
                     nir_variable *fragcolor;
   nir_variable *constants;
                                          };
      static nir_def *
   nir_channel_vec4(nir_builder *b, nir_def *src, unsigned channel)
   {
      unsigned swizzle[4] = { channel, channel, channel, channel };
      }
      static nir_def *
   nir_imm_vec4_float(nir_builder *b, float f)
   {
         }
      static nir_def *
   get_temp(struct st_translate *t, unsigned index)
   {
      if (!t->temps[index])
            }
      static nir_def *
   apply_swizzle(struct st_translate *t,
         {
      /* From the ATI_fs spec:
   *
   *     "Table 3.20 shows the <swizzle> modes:
   *
   *                           Coordinates Used for 1D or      Coordinates Used for
   *      Swizzle              2D SampleMap and PassTexCoord   3D or cubemap SampleMap
   *      -------              -----------------------------   -----------------------
   *      SWIZZLE_STR_ATI      (s, t, r, undefined)            (s, t, r, undefined)
   *      SWIZZLE_STQ_ATI      (s, t, q, undefined)            (s, t, q, undefined)
   *      SWIZZLE_STR_DR_ATI   (s/r, t/r, 1/r, undefined)      (undefined)
   *      SWIZZLE_STQ_DQ_ATI   (s/q, t/q, 1/q, undefined)      (undefined)
   */
   if (swizzle == GL_SWIZZLE_STR_ATI) {
         } else if (swizzle == GL_SWIZZLE_STQ_ATI) {
      static unsigned xywz[4] = { 0, 1, 3, 2 };
      } else {
      nir_def *rcp = nir_frcp(t->b, nir_channel(t->b, src,
                     return nir_vec4(t->b,
                  nir_channel(t->b, st_mul, 0),
      }
      static nir_def *
   load_input(struct st_translate *t, gl_varying_slot slot)
   {
      if (!t->inputs[slot]) {
      nir_variable *var = nir_create_variable_with_location(t->b->shader, nir_var_shader_in, slot,
                                 }
      static nir_def *
   atifs_load_uniform(struct st_translate *t, int index)
   {
      nir_deref_instr *deref = nir_build_deref_array(t->b,
                  }
      static struct nir_def *
   get_source(struct st_translate *t, GLenum src_type)
   {
      if (src_type >= GL_REG_0_ATI && src_type <= GL_REG_5_ATI) {
      if (t->regs_written[t->current_pass][src_type - GL_REG_0_ATI]) {
         } else {
            } else if (src_type >= GL_CON_0_ATI && src_type <= GL_CON_7_ATI) {
      int index = src_type - GL_CON_0_ATI;
   if (t->atifs->LocalConstDef & (1 << index)) {
      return nir_imm_vec4(t->b,
                        } else {
            } else if (src_type == GL_ZERO) {
         } else if (src_type == GL_ONE) {
         } else if (src_type == GL_PRIMARY_COLOR_ARB) {
         } else if (src_type == GL_SECONDARY_INTERPOLATOR_ATI) {
         } else {
      /* frontend prevents this */
         }
      static nir_def *
   prepare_argument(struct st_translate *t, const struct atifs_instruction *inst,
         {
      if (argId >= inst->ArgCount[alpha]) {
      _mesa_warning(0, "Using 0 for missing argument %d\n", argId);
                                 switch (srcReg->argRep) {
   case GL_NONE:
         case GL_RED:
      src = nir_channel_vec4(t->b, src, 0);
      case GL_GREEN:
      src = nir_channel_vec4(t->b, src, 1);
      case GL_BLUE:
      src = nir_channel_vec4(t->b, src, 2);
      case GL_ALPHA:
      src = nir_channel_vec4(t->b, src, 3);
                        if (srcReg->argMod & GL_COMP_BIT_ATI)
         if (srcReg->argMod & GL_BIAS_BIT_ATI)
         if (srcReg->argMod & GL_2X_BIT_ATI)
         if (srcReg->argMod & GL_NEGATE_BIT_ATI)
               }
      static nir_def *
   emit_arith_inst(struct st_translate *t,
               {
      nir_def *src[3] = {0};
   for (int i = 0; i < inst->ArgCount[alpha]; i++)
            switch (inst->Opcode[alpha]) {
   case GL_MOV_ATI:
            case GL_ADD_ATI:
            case GL_SUB_ATI:
            case GL_MUL_ATI:
            case GL_MAD_ATI:
            case GL_LERP_ATI:
            case GL_CND_ATI:
      return nir_bcsel(t->b,
                     case GL_CND0_ATI:
      return nir_bcsel(t->b,
                     case GL_DOT2_ADD_ATI:
      return nir_channel_vec4(t->b,
                           case GL_DOT3_ATI:
            case GL_DOT4_ATI:
            default:
            }
      static nir_def *
   emit_dstmod(struct st_translate *t,
         {
      switch (dstMod & ~GL_SATURATE_BIT_ATI) {
   case GL_2X_BIT_ATI:
      dst = nir_fmul_imm(t->b, dst, 2.0f);
      case GL_4X_BIT_ATI:
      dst = nir_fmul_imm(t->b, dst, 4.0f);
      case GL_8X_BIT_ATI:
      dst = nir_fmul_imm(t->b, dst, 8.0f);
      case GL_HALF_BIT_ATI:
      dst = nir_fmul_imm(t->b, dst, 0.5f);
      case GL_QUARTER_BIT_ATI:
      dst = nir_fmul_imm(t->b, dst, 0.25f);
      case GL_EIGHTH_BIT_ATI:
      dst = nir_fmul_imm(t->b, dst, 0.125f);
      default:
                  if (dstMod & GL_SATURATE_BIT_ATI)
               }
      /**
   * Compile one setup instruction to NIR instructions.
   */
   static void
   compile_setupinst(struct st_translate *t,
               {
      if (!texinst->Opcode)
                              if (pass_tex >= GL_TEXTURE0_ARB && pass_tex <= GL_TEXTURE7_ARB) {
                  } else if (pass_tex >= GL_REG_0_ATI && pass_tex <= GL_REG_5_ATI) {
               /* the frontend already validated that REG is only allowed in second pass */
   if (t->regs_written[0][reg]) {
         } else {
            } else {
         }
            if (texinst->Opcode == ATI_FRAGMENT_SHADER_SAMPLE_OP) {
      nir_variable *tex_var = t->samplers[r];
   if (!tex_var) {
      /* The actual sampler dim will be determined at draw time and lowered
   * by st_nir_update_atifs_samplers. Setting it to 3D for now means we
   * don't optimize out coordinate channels we may need later.
   */
                  tex_var = nir_variable_create(t->b->shader, nir_var_uniform, sampler_type, "tex");
   tex_var->data.binding = r;
   tex_var->data.explicit_binding = true;
      }
            nir_tex_instr *tex = nir_tex_instr_create(t->b->shader, 3);
   tex->op = nir_texop_tex;
   tex->sampler_dim = glsl_get_sampler_dim(tex_var->type);
   tex->dest_type = nir_type_float32;
   tex->coord_components =
            tex->src[0] = nir_tex_src_for_ssa(nir_tex_src_texture_deref,
         tex->src[1] = nir_tex_src_for_ssa(nir_tex_src_sampler_deref,
         tex->src[2] = nir_tex_src_for_ssa(nir_tex_src_coord,
            nir_def_init(&tex->instr, &tex->def, 4, 32);
               } else if (texinst->Opcode == ATI_FRAGMENT_SHADER_PASS_OP) {
                     }
      /**
   * Compile one arithmetic operation COLOR&ALPHA pair into NIR instructions.
   */
   static void
   compile_instruction(struct st_translate *t,
         {
               for (optype = 0; optype < 2; optype++) { /* color, alpha */
               if (!inst->Opcode[optype])
            /* Execute the op */
   nir_def *result = emit_arith_inst(t, inst, optype);
            /* Do the writemask */
   nir_const_value wrmask[4];
   for (int i = 0; i < 4; i++) {
      bool bit = inst->DstReg[optype].dstMask & (1 << i);
               t->temps[dstreg] = nir_bcsel(t->b,
                           }
         /* Creates the uniform variable referencing the ATI_fragment_shader constants.
   */
   static void
   st_atifs_setup_uniforms(struct st_translate *t, struct gl_program *program)
   {
      const struct glsl_type *type =
         t->constants =
      nir_variable_create(t->b->shader, nir_var_uniform, type,
   }
      /**
   * Called when a new variant is needed, we need to translate
   * the ATI fragment shader to NIR
   */
   nir_shader *
   st_translate_atifs_program(struct ati_fragment_shader *atifs,
               {
               struct st_translate translate = {
      .atifs = atifs,
      };
            /* Copy the shader_info from the gl_program */
            nir_shader *s = t->b->shader;
   s->info.name = ralloc_asprintf(s, "ATIFS%d", program->Id);
            t->fragcolor = nir_create_variable_with_location(b.shader, nir_var_shader_out,
                     /* emit instructions */
   for (unsigned pass = 0; pass < atifs->NumPasses; pass++) {
      t->current_pass = pass;
   for (unsigned r = 0; r < MAX_NUM_FRAGMENT_REGISTERS_ATI; r++) {
      struct atifs_setupinst *texinst = &atifs->SetupInst[pass][r];
      }
   for (unsigned i = 0; i < atifs->numArithInstr[pass]; i++) {
      struct atifs_instruction *inst = &atifs->Instructions[pass][i];
                  if (t->regs_written[atifs->NumPasses-1][0])
               }
      static bool
   st_nir_lower_atifs_samplers_instr(nir_builder *b, nir_instr *instr, void *data)
   {
               /* Can't just do this in tex handling below, as llvmpipe leaves dead code
   * derefs around.
   */
   if (instr->type == nir_instr_type_deref) {
      nir_deref_instr *deref = nir_instr_as_deref(instr);
   nir_variable *var = nir_deref_instr_get_variable(deref);
   if (glsl_type_is_sampler(var->type))
               if (instr->type != nir_instr_type_tex)
                              unsigned unit;
   int sampler_src_idx = nir_tex_instr_src_index(tex, nir_tex_src_sampler_deref);
   if (sampler_src_idx >= 0) {
      nir_deref_instr *deref = nir_src_as_deref(tex->src[sampler_src_idx].src);
   nir_variable *var = nir_deref_instr_get_variable(deref);
      } else {
                  bool is_array;
   tex->sampler_dim =
            int coords_idx = nir_tex_instr_src_index(tex, nir_tex_src_coord);
   assert(coords_idx >= 0);
   int coord_components =
         /* Trim unused coords, or append undefs as necessary (if someone
   * accidentally enables a cube array).
   */
   if (coord_components != tex->coord_components) {
      nir_def *coords = tex->src[coords_idx].src.ssa;
   nir_src_rewrite(&tex->src[coords_idx].src,
                        }
      /**
   * Rewrites sampler dimensions and coordinate components for the currently
   * active texture unit at draw time.
   */
   bool
   st_nir_lower_atifs_samplers(struct nir_shader *s, const uint8_t *texture_index)
   {
      nir_foreach_uniform_variable(var, s) {
      if (!glsl_type_is_sampler(var->type))
         bool is_array;
   enum glsl_sampler_dim sampler_dim =
                     return nir_shader_instructions_pass(s, st_nir_lower_atifs_samplers_instr,\
            }
