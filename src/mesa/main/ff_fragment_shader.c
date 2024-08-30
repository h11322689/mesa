   /**************************************************************************
   *
   * Copyright 2007 VMware, Inc.
   * All Rights Reserved.
   * Copyright 2009 VMware, Inc.  All Rights Reserved.
   * Copyright © 2010-2011 Intel Corporation
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
      #include "util/glheader.h"
   #include "main/context.h"
      #include "main/macros.h"
   #include "main/state.h"
   #include "main/texenvprogram.h"
   #include "main/texobj.h"
   #include "program/program.h"
   #include "program/prog_cache.h"
   #include "program/prog_statevars.h"
   #include "program/prog_to_nir.h"
   #include "util/bitscan.h"
      #include "state_tracker/st_context.h"
   #include "state_tracker/st_program.h"
   #include "state_tracker/st_nir.h"
      #include "compiler/nir/nir_builder.h"
   #include "compiler/nir/nir_builtin_builder.h"
      /*
   * Note on texture units:
   *
   * The number of texture units supported by fixed-function fragment
   * processing is MAX_TEXTURE_COORD_UNITS, not MAX_TEXTURE_IMAGE_UNITS.
   * That's because there's a one-to-one correspondence between texture
   * coordinates and samplers in fixed-function processing.
   *
   * Since fixed-function vertex processing is limited to MAX_TEXTURE_COORD_UNITS
   * sets of texcoords, so is fixed-function fragment processing.
   *
   * We can safely use ctx->Const.MaxTextureUnits for loop bounds.
   */
         static GLboolean
   texenv_doing_secondary_color(struct gl_context *ctx)
   {
      if (ctx->Light.Enabled &&
      (ctx->Light.Model.ColorControl == GL_SEPARATE_SPECULAR_COLOR))
         if (ctx->Fog.ColorSumEnabled)
               }
      struct state_key {
      GLuint nr_enabled_units:4;
   GLuint separate_specular:1;
   GLuint fog_mode:2;          /**< FOG_x */
   GLuint inputs_available:12;
            /* NOTE: This array of structs must be last! (see "keySize" below) */
   struct {
      GLuint enabled:1;
   GLuint source_index:4;   /**< TEXTURE_x_INDEX */
            /***
   * These are taken from struct gl_tex_env_combine_packed
   * @{
   */
   GLuint ModeRGB:4;
   GLuint ModeA:4;
   GLuint ScaleShiftRGB:2;
   GLuint ScaleShiftA:2;
   GLuint NumArgsRGB:3;
   GLuint NumArgsA:3;
   struct gl_tex_env_argument ArgsRGB[MAX_COMBINER_TERMS];
   struct gl_tex_env_argument ArgsA[MAX_COMBINER_TERMS];
         };
         /**
   * Do we need to clamp the results of the given texture env/combine mode?
   * If the inputs to the mode are in [0,1] we don't always have to clamp
   * the results.
   */
   static GLboolean
   need_saturate( GLuint mode )
   {
      switch (mode) {
   case TEXENV_MODE_REPLACE:
   case TEXENV_MODE_MODULATE:
   case TEXENV_MODE_INTERPOLATE:
         case TEXENV_MODE_ADD:
   case TEXENV_MODE_ADD_SIGNED:
   case TEXENV_MODE_SUBTRACT:
   case TEXENV_MODE_DOT3_RGB:
   case TEXENV_MODE_DOT3_RGB_EXT:
   case TEXENV_MODE_DOT3_RGBA:
   case TEXENV_MODE_DOT3_RGBA_EXT:
   case TEXENV_MODE_MODULATE_ADD_ATI:
   case TEXENV_MODE_MODULATE_SIGNED_ADD_ATI:
   case TEXENV_MODE_MODULATE_SUBTRACT_ATI:
   case TEXENV_MODE_ADD_PRODUCTS_NV:
   case TEXENV_MODE_ADD_PRODUCTS_SIGNED_NV:
         default:
      assert(0);
         }
      #define VERT_BIT_TEX_ANY    (0xff << VERT_ATTRIB_TEX0)
      /**
   * Identify all possible varying inputs.  The fragment program will
   * never reference non-varying inputs, but will track them via state
   * constants instead.
   *
   * This function figures out all the inputs that the fragment program
   * has access to and filters input bitmask.
   */
   static GLbitfield filter_fp_input_mask( GLbitfield fp_inputs,
         {
      if (ctx->VertexProgram._Overriden) {
      /* Somebody's messing with the vertex program and we don't have
   * a clue what's happening.  Assume that it could be producing
   * all possible outputs.
   */
               if (ctx->RenderMode == GL_FEEDBACK) {
      /* _NEW_RENDERMODE */
               /* _NEW_PROGRAM */
   const GLboolean vertexShader =
                  if (!(vertexProgram || vertexShader)) {
      /* Fixed function vertex logic */
            GLbitfield varying_inputs = ctx->VertexProgram._VaryingInputs;
   /* We only update ctx->VertexProgram._VaryingInputs when in VP_MODE_FF _VPMode */
            /* These get generated in the setup routine regardless of the
   * vertex program:
   */
   /* _NEW_POINT */
   if (ctx->Point.PointSprite) {
      /* All texture varyings are possible to use */
      }
   else {
      const GLbitfield possible_tex_inputs =
                                    /* First look at what values may be computed by the generated
   * vertex program:
   */
   if (ctx->Light.Enabled) {
               if (texenv_doing_secondary_color(ctx))
               /* Then look at what might be varying as a result of enabled
   * arrays, etc:
   */
   if (varying_inputs & VERT_BIT_COLOR0)
         if (varying_inputs & VERT_BIT_COLOR1)
                        /* calculate from vp->outputs */
            /* Choose GLSL vertex shader over ARB vertex program.  Need this
   * since vertex shader state validation comes after fragment state
   * validation (see additional comments in state.c).
   */
   if (ctx->_Shader->CurrentProgram[MESA_SHADER_GEOMETRY] != NULL)
         else if (ctx->_Shader->CurrentProgram[MESA_SHADER_TESS_EVAL] != NULL)
         else if (vertexShader)
         else
                     /* These get generated in the setup routine regardless of the
   * vertex program:
   */
   /* _NEW_POINT */
   if (ctx->Point.PointSprite) {
      /* All texture varyings are possible to use */
                  }
         /**
   * Examine current texture environment state and generate a unique
   * key to identify it.
   */
   static GLuint make_state_key( struct gl_context *ctx,  struct state_key *key )
   {
      GLbitfield inputs_referenced = VARYING_BIT_COL0;
   GLbitfield mask;
                     /* _NEW_TEXTURE_OBJECT | _NEW_TEXTURE_STATE */
   mask = ctx->Texture._EnabledCoordUnits;
   int i = -1;
   while (mask) {
      i = u_bit_scan(&mask);
   const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[i];
   const struct gl_texture_object *texObj = texUnit->_Current;
   const struct gl_tex_env_combine_packed *comb =
            if (!texObj)
            key->unit[i].enabled = 1;
                     const struct gl_sampler_object *samp = _mesa_get_samplerobj(ctx, i);
   if (samp->Attrib.CompareMode == GL_COMPARE_R_TO_TEXTURE) {
      const GLenum format = _mesa_texture_base_format(texObj);
   key->unit[i].shadow = (format == GL_DEPTH_COMPONENT ||
               key->unit[i].ModeRGB = comb->ModeRGB;
   key->unit[i].ModeA = comb->ModeA;
   key->unit[i].ScaleShiftRGB = comb->ScaleShiftRGB;
   key->unit[i].ScaleShiftA = comb->ScaleShiftA;
   key->unit[i].NumArgsRGB = comb->NumArgsRGB;
            memcpy(key->unit[i].ArgsRGB, comb->ArgsRGB, sizeof comb->ArgsRGB);
                        /* _NEW_FOG */
   if (texenv_doing_secondary_color(ctx)) {
      key->separate_specular = 1;
               /* _NEW_FOG */
            /* _NEW_BUFFERS */
            /* _NEW_COLOR */
   if (ctx->Color.AlphaEnabled && key->num_draw_buffers == 0) {
      /* if alpha test is enabled we need to emit at least one color */
                        /* compute size of state key, ignoring unused texture units */
   keySize = sizeof(*key) - sizeof(key->unit)
               }
         /** State used to build the fragment program:
   */
   struct texenv_fragment_program {
      nir_builder *b;
                              nir_def *src_texture[MAX_TEXTURE_COORD_UNITS];
   /* ssa-def containing each texture unit's sampled texture color,
   * else NULL.
               };
      static nir_variable *
   register_state_var(struct texenv_fragment_program *p,
                     gl_state_index s0,
   gl_state_index s1,
   {
      gl_state_index16 tokens[STATE_LENGTH];
   tokens[0] = s0;
   tokens[1] = s1;
   tokens[2] = s2;
   tokens[3] = s3;
   nir_variable *var = nir_find_state_variable(p->b->shader, tokens);
   if (var)
                     char *name = _mesa_program_state_string(tokens);
   var = nir_variable_create(p->b->shader, nir_var_uniform, type, name);
            var->num_state_slots = 1;
   var->state_slots = ralloc_array(var, nir_state_slot, 1);
   var->data.driver_location = loc;
   memcpy(var->state_slots[0].tokens, tokens,
            p->b->shader->num_uniforms++;
      }
      static nir_def *
   load_state_var(struct texenv_fragment_program *p,
                  gl_state_index s0,
   gl_state_index s1,
      {
      nir_variable *var = register_state_var(p, s0, s1, s2, s3, type);
      }
      static nir_def *
   load_input(struct texenv_fragment_program *p, gl_varying_slot slot,
         {
      nir_variable *var =
      nir_get_variable_with_location(p->b->shader,
                  var->data.interpolation = INTERP_MODE_NONE;
      }
      static nir_def *
   get_current_attrib(struct texenv_fragment_program *p, GLuint attrib)
   {
      return load_state_var(p, STATE_CURRENT_ATTRIB_MAYBE_VP_CLAMPED,
            }
      static nir_def *
   get_gl_Color(struct texenv_fragment_program *p)
   {
      if (p->state->inputs_available & VARYING_BIT_COL0) {
         } else {
            }
      static nir_def *
   get_source(struct texenv_fragment_program *p,
         {
      switch (src) {
   case TEXENV_SRC_TEXTURE:
            case TEXENV_SRC_TEXTURE0:
   case TEXENV_SRC_TEXTURE1:
   case TEXENV_SRC_TEXTURE2:
   case TEXENV_SRC_TEXTURE3:
   case TEXENV_SRC_TEXTURE4:
   case TEXENV_SRC_TEXTURE5:
   case TEXENV_SRC_TEXTURE6:
   case TEXENV_SRC_TEXTURE7:
            case TEXENV_SRC_CONSTANT:
      return load_state_var(p, STATE_TEXENV_COLOR,
               case TEXENV_SRC_PRIMARY_COLOR:
            case TEXENV_SRC_ZERO:
            case TEXENV_SRC_ONE:
            case TEXENV_SRC_PREVIOUS:
      if (!p->src_previous) {
         } else {
               default:
      assert(0);
         }
      static nir_def *
   emit_combine_source(struct texenv_fragment_program *p,
                     {
                        switch (operand) {
   case TEXENV_OPR_ONE_MINUS_COLOR:
            case TEXENV_OPR_ALPHA:
            case TEXENV_OPR_ONE_MINUS_ALPHA: {
      nir_def *scalar =
                        case TEXENV_OPR_COLOR:
            default:
      assert(0);
         }
      /**
   * Check if the RGB and Alpha sources and operands match for the given
   * texture unit's combinder state.  When the RGB and A sources and
   * operands match, we can emit fewer instructions.
   */
   static GLboolean args_match( const struct state_key *key, GLuint unit )
   {
               for (i = 0; i < numArgs; i++) {
      if (key->unit[unit].ArgsA[i].Source != key->unit[unit].ArgsRGB[i].Source)
            switch (key->unit[unit].ArgsA[i].Operand) {
   case TEXENV_OPR_ALPHA:
      switch (key->unit[unit].ArgsRGB[i].Operand) {
   case TEXENV_OPR_COLOR:
   case TEXENV_OPR_ALPHA:
         default:
         }
      case TEXENV_OPR_ONE_MINUS_ALPHA:
      switch (key->unit[unit].ArgsRGB[i].Operand) {
   case TEXENV_OPR_ONE_MINUS_COLOR:
   case TEXENV_OPR_ONE_MINUS_ALPHA:
         default:
         }
      default:
                        }
      static nir_def *
   smear(nir_builder *b, nir_def *val)
   {
      if (val->num_components != 1)
               }
      static nir_def *
   emit_combine(struct texenv_fragment_program *p,
               GLuint unit,
   GLuint nr,
   {
      nir_def *src[MAX_COMBINER_TERMS];
   nir_def *tmp0, *tmp1;
                     for (i = 0; i < nr; i++)
            switch (mode) {
   case TEXENV_MODE_REPLACE:
            case TEXENV_MODE_MODULATE:
            case TEXENV_MODE_ADD:
            case TEXENV_MODE_ADD_SIGNED:
            case TEXENV_MODE_INTERPOLATE:
            case TEXENV_MODE_SUBTRACT:
            case TEXENV_MODE_DOT3_RGBA:
   case TEXENV_MODE_DOT3_RGBA_EXT:
   case TEXENV_MODE_DOT3_RGB_EXT:
   case TEXENV_MODE_DOT3_RGB:
      tmp0 = nir_fadd_imm(p->b, nir_fmul_imm(p->b, src[0], 2.0f), -1.0f);
   tmp1 = nir_fadd_imm(p->b, nir_fmul_imm(p->b, src[1], 2.0f), -1.0f);
         case TEXENV_MODE_MODULATE_ADD_ATI:
            case TEXENV_MODE_MODULATE_SIGNED_ADD_ATI:
      return nir_fadd_imm(p->b,
                           case TEXENV_MODE_MODULATE_SUBTRACT_ATI:
            case TEXENV_MODE_ADD_PRODUCTS_NV:
      return nir_fadd(p->b, nir_fmul(p->b, src[0], src[1]),
         case TEXENV_MODE_ADD_PRODUCTS_SIGNED_NV:
      return nir_fadd_imm(p->b,
                        default:
      assert(0);
         }
      /**
   * Generate instructions for one texture unit's env/combiner mode.
   */
   static nir_def *
   emit_texenv(struct texenv_fragment_program *p, GLuint unit)
   {
      const struct state_key *key = p->state;
   GLboolean rgb_saturate, alpha_saturate;
            if (!key->unit[unit].enabled) {
                  switch (key->unit[unit].ModeRGB) {
   case TEXENV_MODE_DOT3_RGB_EXT:
      alpha_shift = key->unit[unit].ScaleShiftA;
   rgb_shift = 0;
      case TEXENV_MODE_DOT3_RGBA_EXT:
      alpha_shift = 0;
   rgb_shift = 0;
      default:
      rgb_shift = key->unit[unit].ScaleShiftRGB;
   alpha_shift = key->unit[unit].ScaleShiftA;
               /* If we'll do rgb/alpha shifting don't saturate in emit_combine().
   * We don't want to clamp twice.
   */
   if (rgb_shift)
         else if (need_saturate(key->unit[unit].ModeRGB))
         else
            if (alpha_shift)
         else if (need_saturate(key->unit[unit].ModeA))
         else
                     /* Emit the RGB and A combine ops
   */
   if (key->unit[unit].ModeRGB == key->unit[unit].ModeA &&
      args_match(key, unit)) {
   val = emit_combine(p, unit,
                     val = smear(p->b, val);
   if (rgb_saturate)
      }
   else if (key->unit[unit].ModeRGB == TEXENV_MODE_DOT3_RGBA_EXT ||
            val = emit_combine(p, unit,
                     val = smear(p->b, val);
   if (rgb_saturate)
      }
   else {
      /* Need to do something to stop from re-emitting identical
   * argument calculations here:
   */
   val = emit_combine(p, unit,
                     val = smear(p->b, val);
   if (rgb_saturate)
                  val = emit_combine(p, unit,
                        if (val->num_components != 1)
            if (alpha_saturate)
                              /* Deal with the final shift:
   */
   if (alpha_shift || rgb_shift) {
               if (rgb_shift == alpha_shift) {
         }
   else {
      shift = nir_imm_vec4(p->b,
      (float)(1 << rgb_shift),
   (float)(1 << rgb_shift),
   (float)(1 << rgb_shift),
               }
   else
      }
         /**
   * Generate instruction for getting a texture source term.
   */
   static void
   load_texture(struct texenv_fragment_program *p, GLuint unit)
   {
      if (p->src_texture[unit])
            const GLuint texTarget = p->state->unit[unit].source_index;
            if (!(p->state->inputs_available & (VARYING_BIT_TEX0 << unit))) {
         } else {
      texcoord = load_input(p,
      VARYING_SLOT_TEX0 + unit,
            if (!p->state->unit[unit].enabled) {
      p->src_texture[unit] = nir_imm_zero(p->b, 4, 32);
               unsigned num_srcs = 4;
   if (p->state->unit[unit].shadow)
            nir_tex_instr *tex = nir_tex_instr_create(p->b->shader, num_srcs);
   tex->op = nir_texop_tex;
   tex->dest_type = nir_type_float32;
   tex->texture_index = unit;
            tex->sampler_dim =
      _mesa_texture_index_to_sampler_dim(texTarget,
         tex->coord_components =
         if (tex->is_array)
            nir_variable *var = p->sampler_vars[unit];
   if (!var) {
      const struct glsl_type *sampler_type =
      glsl_sampler_type(tex->sampler_dim,
               var = nir_variable_create(p->b->shader, nir_var_uniform,
                     var->data.binding = unit;
                        nir_deref_instr *deref = nir_build_deref_var(p->b, var);
   tex->src[0] = nir_tex_src_for_ssa(nir_tex_src_texture_deref,
         tex->src[1] = nir_tex_src_for_ssa(nir_tex_src_sampler_deref,
            nir_def *src2 =
      nir_channels(p->b, texcoord,
               tex->src[3] = nir_tex_src_for_ssa(nir_tex_src_projector,
            if (p->state->unit[unit].shadow) {
      tex->is_shadow = true;
   nir_def *src4 =
                     nir_def_init(&tex->instr, &tex->def, 4, 32);
            nir_builder_instr_insert(p->b, &tex->instr);
   BITSET_SET(p->b->shader->info.textures_used, unit);
      }
      static void
   load_texenv_source(struct texenv_fragment_program *p,
         {
      switch (src) {
   case TEXENV_SRC_TEXTURE:
      load_texture(p, unit);
         case TEXENV_SRC_TEXTURE0:
   case TEXENV_SRC_TEXTURE1:
   case TEXENV_SRC_TEXTURE2:
   case TEXENV_SRC_TEXTURE3:
   case TEXENV_SRC_TEXTURE4:
   case TEXENV_SRC_TEXTURE5:
   case TEXENV_SRC_TEXTURE6:
   case TEXENV_SRC_TEXTURE7:
      load_texture(p, src - TEXENV_SRC_TEXTURE0);
         default:
      /* not a texture src - do nothing */
         }
         /**
   * Generate instructions for loading all texture source terms.
   */
   static GLboolean
   load_texunit_sources(struct texenv_fragment_program *p, GLuint unit)
   {
      const struct state_key *key = p->state;
            for (i = 0; i < key->unit[unit].NumArgsRGB; i++) {
                  for (i = 0; i < key->unit[unit].NumArgsA; i++) {
                     }
      static void
   emit_instructions(struct texenv_fragment_program *p)
   {
      struct state_key *key = p->state;
            if (key->nr_enabled_units) {
      /* First pass - to support texture_env_crossbar, first identify
   * all referenced texture sources and emit texld instructions
   * for each:
   */
   for (unit = 0; unit < key->nr_enabled_units; unit++)
      if (key->unit[unit].enabled) {
               /* Second pass - emit combine instructions to build final color:
   */
   for (unit = 0; unit < key->nr_enabled_units; unit++) {
      if (key->unit[unit].enabled) {
                                 if (key->separate_specular) {
               nir_def *secondary;
   if (p->state->inputs_available & VARYING_BIT_COL1)
         else
            secondary = nir_vector_insert_imm(p->b, secondary,
                     const char *name =
         nir_variable *var =
      nir_variable_create(p->b->shader, nir_var_shader_out,
         var->data.location = FRAG_RESULT_COLOR;
                        }
      /**
   * Generate a new fragment program which implements the context's
   * current texture env/combine mode.
   */
   static nir_shader *
   create_new_program(struct state_key *key,
               {
               memset(&p, 0, sizeof(p));
            program->Parameters = _mesa_new_parameter_list();
            nir_builder b = nir_builder_init_simple_shader(MESA_SHADER_FRAGMENT,
                           s->info.separate_shader = true;
                     if (key->num_draw_buffers)
                     if (key->fog_mode)
            _mesa_add_separate_state_parameters(program, p.state_params);
               }
      /**
   * Return a fragment program which implements the current
   * fixed-function texture, fog and color-sum operations.
   */
   struct gl_program *
   _mesa_get_fixed_func_fragment_program(struct gl_context *ctx)
   {
      struct gl_program *prog;
   struct state_key key;
                     prog = (struct gl_program *)
      _mesa_search_program_cache(ctx->FragmentProgram.Cache,
         if (!prog) {
      prog = ctx->Driver.NewProgram(ctx, MESA_SHADER_FRAGMENT, 0, false);
   if (!prog)
            const struct nir_shader_compiler_options *options =
            nir_shader *s =
            prog->state.type = PIPE_SHADER_IR_NIR;
                     /* default mapping from samplers to texture units */
   for (unsigned i = 0; i < MAX_SAMPLERS; i++)
                     _mesa_program_cache_insert(ctx, ctx->FragmentProgram.Cache,
                  }
