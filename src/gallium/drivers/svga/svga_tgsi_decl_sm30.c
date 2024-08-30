   /**********************************************************
   * Copyright 2008-2009 VMware, Inc.  All rights reserved.
   *
   * Permission is hereby granted, free of charge, to any person
   * obtaining a copy of this software and associated documentation
   * files (the "Software"), to deal in the Software without
   * restriction, including without limitation the rights to use, copy,
   * modify, merge, publish, distribute, sublicense, and/or sell copies
   * of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be
   * included in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   **********************************************************/
         #include "pipe/p_shader_tokens.h"
   #include "tgsi/tgsi_parse.h"
   #include "util/u_memory.h"
      #include "svga_tgsi_emit.h"
         /**
   * Translate TGSI semantic info into SVGA3d semantic info.
   * This is called for VS outputs and PS inputs only.
   */
   static bool
   translate_vs_ps_semantic(struct svga_shader_emitter *emit,
                     {
      switch (semantic.Name) {
   case TGSI_SEMANTIC_POSITION:
      *idx = semantic.Index;
   *usage = SVGA3D_DECLUSAGE_POSITION;
      case TGSI_SEMANTIC_COLOR:
      *idx = semantic.Index;
   *usage = SVGA3D_DECLUSAGE_COLOR;
      case TGSI_SEMANTIC_BCOLOR:
      *idx = semantic.Index + 2; /* sharing with COLOR */
   *usage = SVGA3D_DECLUSAGE_COLOR;
      case TGSI_SEMANTIC_FOG:
      *idx = 0;
   assert(semantic.Index == 0);
   *usage = SVGA3D_DECLUSAGE_TEXCOORD;
      case TGSI_SEMANTIC_PSIZE:
      *idx = semantic.Index;
   *usage = SVGA3D_DECLUSAGE_PSIZE;
      case TGSI_SEMANTIC_GENERIC:
      *idx = svga_remap_generic_index(emit->key.generic_remap_table,
         *usage = SVGA3D_DECLUSAGE_TEXCOORD;
      case TGSI_SEMANTIC_NORMAL:
      *idx = semantic.Index;
   *usage = SVGA3D_DECLUSAGE_NORMAL;
      case TGSI_SEMANTIC_CLIPDIST:
   case TGSI_SEMANTIC_CLIPVERTEX:
      /* XXX at this time we don't support clip distance or clip vertices */
   debug_warn_once("unsupported clip distance/vertex attribute\n");
   *usage = SVGA3D_DECLUSAGE_TEXCOORD;
   *idx = 0;
      default:
      assert(0);
   *usage = SVGA3D_DECLUSAGE_TEXCOORD;
   *idx = 0;
                  }
         /**
   * Emit a PS input (or VS depth/fog output) register declaration.
   * For example, if usage = SVGA3D_DECLUSAGE_TEXCOORD, reg.num = 1, and
   * index = 3, we'll emit "dcl_texcoord3 v1".
   */
   static bool
   emit_decl(struct svga_shader_emitter *emit,
            SVGA3dShaderDestToken reg,
      {
      SVGA3DOpDclArgs dcl;
            /* check values against bitfield sizes */
   assert(index < 16);
            opcode = inst_token(SVGA3DOP_DCL);
   dcl.values[0] = 0;
            dcl.dst = reg;
   dcl.usage = usage;
   dcl.index = index;
            return (emit_instruction(emit, opcode) &&
      }
         /**
   * Emit declaration for PS front/back-face input register.
   */
   static bool
   emit_vface_decl(struct svga_shader_emitter *emit)
   {
      if (!emit->emitted_vface) {
      SVGA3dShaderDestToken reg =
            if (!emit_decl(emit, reg, 0, 0))
               }
      }
         /**
   * Emit PS input register to pass depth/fog coordinates.
   * Note that this always goes into texcoord[0].
   */
   static bool
   ps30_input_emit_depth_fog(struct svga_shader_emitter *emit,
         {
               if (emit->emitted_depth_fog) {
      *out = emit->ps_depth_fog;
               if (emit->ps30_input_count >= SVGA3D_INPUTREG_MAX)
            reg = src_register(SVGA3DREG_INPUT,
                                 }
         /**
   * Process a PS input declaration.
   * We'll emit a declaration like "dcl_texcoord1 v2"
   */
   static bool
   ps30_input(struct svga_shader_emitter *emit,
               {
      unsigned usage, index;
                        emit->ps_true_pos = src_register(SVGA3DREG_MISCTYPE,
         emit->ps_true_pos.base.swizzle = TRANSLATE_SWIZZLE(TGSI_SWIZZLE_X,
                     reg = writemask(dst(emit->ps_true_pos),
                  if (emit->info.reads_z) {
                     emit->input_map[idx] = src_register(SVGA3DREG_TEMP,
                                 emit->ps_depth_pos.base.swizzle = TRANSLATE_SWIZZLE(TGSI_SWIZZLE_Z,
                  }
   else {
                     }
   else if (emit->key.fs.light_twoside &&
               if (!translate_vs_ps_semantic(emit, semantic, &usage, &index))
            emit->internal_color_idx[emit->internal_color_count] = idx;
   emit->input_map[idx] =
         emit->ps30_input_count++;
                     if (!emit_decl(emit, reg, usage, index))
            semantic.Name = TGSI_SEMANTIC_BCOLOR;
   if (!translate_vs_ps_semantic(emit, semantic, &usage, &index))
            if (emit->ps30_input_count >= SVGA3D_INPUTREG_MAX)
                     if (!emit_decl(emit, reg, usage, index))
            if (!emit_vface_decl(emit))
               }
   else if (semantic.Name == TGSI_SEMANTIC_FACE) {
      if (!emit_vface_decl(emit))
         emit->emit_frontface = true;
   emit->internal_frontface_idx = idx;
      }
                        if (!ps30_input_emit_depth_fog(emit, &emit->input_map[idx]))
            emit->input_map[idx].base.swizzle = TRANSLATE_SWIZZLE(TGSI_SWIZZLE_X,
                        }
               if (!translate_vs_ps_semantic(emit, semantic, &usage, &index))
            if (emit->ps30_input_count >= SVGA3D_INPUTREG_MAX)
            emit->input_map[idx] =
                     if (!emit_decl(emit, reg, usage, index))
            if (semantic.Name == TGSI_SEMANTIC_GENERIC &&
      emit->key.sprite_origin_lower_left &&
   index >= 1 &&
   emit->key.sprite_coord_enable & (1 << semantic.Index)) {
   /* This is a sprite texture coord with lower-left origin.
   * We need to invert the texture T coordinate since the SVGA3D
   * device only supports an upper-left origin.
                                          /* this temp register will be the results of the MAD instruction */
   emit->ps_inverted_texcoord[unit] =
                           /* replace input_map entry with the temp register */
                        }
         /**
   * Process a PS output declaration.
   * Note that we don't actually emit a SVGA3DOpDcl for PS outputs.
   * \idx  register index, such as OUT[2] (not semantic index)
   */
   static bool
   ps30_output(struct svga_shader_emitter *emit,
               {
      switch (semantic.Name) {
   case TGSI_SEMANTIC_COLOR:
      if (emit->unit == PIPE_SHADER_FRAGMENT) {
      if (emit->key.fs.white_fragments) {
      /* Used for XOR logicop mode */
   emit->output_map[idx] = dst_register(SVGA3DREG_TEMP,
         emit->temp_color_output[idx] = emit->output_map[idx];
   emit->true_color_output[idx] = dst_register(SVGA3DREG_COLOROUT,
      }
   else if (emit->key.fs.write_color0_to_n_cbufs) {
      /* We'll write color output [0] to all render targets.
   * Prepare all the output registers here, but only when the
   * semantic.Index == 0 so we don't do this more than once.
   */
   if (semantic.Index == 0) {
      unsigned i;
   for (i = 0; i < emit->key.fs.write_color0_to_n_cbufs; i++) {
      emit->output_map[idx+i] = dst_register(SVGA3DREG_TEMP,
         emit->temp_color_output[i] = emit->output_map[idx+i];
   emit->true_color_output[i] = dst_register(SVGA3DREG_COLOROUT,
            }
   else {
      emit->output_map[idx] =
         }
   else {
      emit->output_map[idx] = dst_register(SVGA3DREG_COLOROUT,
      }
      case TGSI_SEMANTIC_POSITION:
      emit->output_map[idx] = dst_register(SVGA3DREG_TEMP,
         emit->temp_pos = emit->output_map[idx];
   emit->true_pos = dst_register(SVGA3DREG_DEPTHOUT,
            default:
      assert(0);
   /* A wild stab in the dark. */
   emit->output_map[idx] = dst_register(SVGA3DREG_COLOROUT, 0);
                  }
         /**
   * Declare a VS input register.
   * We still make up the input semantics the same as in 2.0
   */
   static bool
   vs30_input(struct svga_shader_emitter *emit,
               {
      SVGA3DOpDclArgs dcl;
   SVGA3dShaderInstToken opcode;
            opcode = inst_token(SVGA3DOP_DCL);
   dcl.values[0] = 0;
            emit->input_map[idx] = src_register(SVGA3DREG_INPUT, idx);
                              dcl.usage = usage;
   dcl.index = index;
            return (emit_instruction(emit, opcode) &&
      }
         /**
   * Declare VS output for holding depth/fog.
   */
   static bool
   vs30_output_emit_depth_fog(struct svga_shader_emitter *emit,
         {
               if (emit->emitted_depth_fog) {
      *out = emit->vs_depth_fog;
                                             }
         /**
   * Declare a VS output.
   * VS3.0 outputs have proper declarations and semantic info for
   * matching against PS inputs.
   */
   static bool
   vs30_output(struct svga_shader_emitter *emit,
               {
      SVGA3DOpDclArgs dcl;
   SVGA3dShaderInstToken opcode;
            opcode = inst_token(SVGA3DOP_DCL);
   dcl.values[0] = 0;
            if (!translate_vs_ps_semantic(emit, semantic, &usage, &index))
            if (emit->vs30_output_count >= SVGA3D_OUTPUTREG_MAX)
            dcl.dst = dst_register(SVGA3DREG_OUTPUT, emit->vs30_output_count++);
   dcl.usage = usage;
   dcl.index = index;
            if (semantic.Name == TGSI_SEMANTIC_POSITION) {
      assert(idx == 0);
   emit->output_map[idx] = dst_register(SVGA3DREG_TEMP,
         emit->temp_pos = emit->output_map[idx];
            /* Grab an extra output for the depth output */
   if (!vs30_output_emit_depth_fog(emit, &emit->depth_pos))
         }
   else if (semantic.Name == TGSI_SEMANTIC_PSIZE) {
      emit->output_map[idx] = dst_register(SVGA3DREG_TEMP,
                  /* This has the effect of not declaring psiz (below) and not
   * emitting the final MOV to true_psiz in the postamble.
   */
   if (!emit->key.vs.allow_psiz)
               }
   else if (semantic.Name == TGSI_SEMANTIC_FOG) {
      /*
   * Fog is shared with depth.
   * So we need to decrement out_count since emit_depth_fog will increment it.
   */
            if (!vs30_output_emit_depth_fog(emit, &emit->output_map[idx]))
               }
   else {
                  return (emit_instruction(emit, opcode) &&
      }
         /** Translate PIPE_TEXTURE_x to SVGA3DSAMP_x */
   static uint8_t
   svga_tgsi_sampler_type(const struct svga_shader_emitter *emit, int idx)
   {
      switch (emit->sampler_target[idx]) {
   case TGSI_TEXTURE_1D:
         case TGSI_TEXTURE_2D:
   case TGSI_TEXTURE_RECT:
         case TGSI_TEXTURE_SHADOW2D:
         case TGSI_TEXTURE_3D:
         case TGSI_TEXTURE_CUBE:
                     }
         static bool
   ps30_sampler(struct svga_shader_emitter *emit,
         {
      SVGA3DOpDclArgs dcl;
            opcode = inst_token(SVGA3DOP_DCL);
   dcl.values[0] = 0;
            dcl.dst = dst_register(SVGA3DREG_SAMPLER, idx);
   dcl.type = svga_tgsi_sampler_type(emit, idx);
            return (emit_instruction(emit, opcode) &&
      }
         bool
   svga_shader_emit_samplers_decl(struct svga_shader_emitter *emit)
   {
               for (i = 0; i < emit->num_samplers; i++) {
      if (!ps30_sampler(emit, i))
      }
      }
         bool
   svga_translate_decl_sm30(struct svga_shader_emitter *emit,
         {
      unsigned first = decl->Range.First;
   unsigned last = decl->Range.Last;
            for (idx = first; idx <= last; idx++) {
               switch (decl->Declaration.File) {
   case TGSI_FILE_SAMPLER:
      assert (emit->unit == PIPE_SHADER_FRAGMENT);
   /* just keep track of the number of samplers here.
   * Will emit the declaration in the helpers function.
   */
               case TGSI_FILE_INPUT:
      if (emit->unit == PIPE_SHADER_VERTEX)
         else
               case TGSI_FILE_OUTPUT:
      if (emit->unit == PIPE_SHADER_VERTEX)
         else
               case TGSI_FILE_SAMPLER_VIEW:
      {
      unsigned unit = decl->Range.First;
   assert(decl->Range.First == decl->Range.Last);
                  default:
      /* don't need to declare other vars */
               if (!ok)
                  }
