   /**************************************************************************
   *
   * Copyright 2019 Red Hat.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included
   * in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   **************************************************************************/
      /*
   * NIR lowering passes to handle the draw stages for
   * - pstipple
   * - aaline
   * - aapoint.
   *
   * These are all ported from the equivalent TGSI transforms.
   */
      #include "nir.h"
   #include "tgsi/tgsi_from_mesa.h"
   #include "nir_builder.h"
      #include "nir_draw_helpers.h"
      typedef struct {
      nir_builder b;
   nir_shader *shader;
   bool fs_pos_is_sysval;
   nir_variable *stip_tex;
   nir_def *fragcoord;
      } lower_pstipple;
      static nir_def *
   load_frag_coord(nir_builder *b)
   {
      nir_variable *pos = nir_get_variable_with_location(b->shader, nir_var_shader_in,
         pos->data.interpolation = INTERP_MODE_NOPERSPECTIVE;
      }
      static void
   nir_lower_pstipple_block(nir_block *block,
         {
      nir_builder *b = &state->b;
                              texcoord = nir_fmul(b, nir_trim_vector(b, frag_coord, 2),
            nir_tex_instr *tex = nir_tex_instr_create(b->shader, 1);
   tex->op = nir_texop_tex;
   tex->sampler_dim = GLSL_SAMPLER_DIM_2D;
   tex->coord_components = 2;
   tex->dest_type = nir_type_float32;
   tex->texture_index = state->stip_tex->data.binding;
   tex->sampler_index = state->stip_tex->data.binding;
   tex->src[0] = nir_tex_src_for_ssa(nir_tex_src_coord, texcoord);
                              switch (state->bool_type) {
   case nir_type_bool1:
      condition = nir_fneu_imm(b, nir_channel(b, &tex->def, 3), 0.0);
      case nir_type_bool32:
      condition = nir_fneu32(b, nir_channel(b, &tex->def, 3),
            default:
                  nir_discard_if(b, condition);
      }
      static void
   nir_lower_pstipple_impl(nir_function_impl *impl,
         {
               nir_block *start = nir_start_block(impl);
      }
      void
   nir_lower_pstipple_fs(struct nir_shader *shader,
                           {
      lower_pstipple state = {
      .shader = shader,
   .fs_pos_is_sysval = fs_pos_is_sysval,
               assert(bool_type == nir_type_bool1 ||
            if (shader->info.stage != MESA_SHADER_FRAGMENT)
            int binding = 0;
   nir_foreach_uniform_variable(var, shader) {
      if (glsl_type_is_sampler(var->type)) {
      if (var->data.binding >= binding)
         }
   const struct glsl_type *sampler2D =
            nir_variable *tex_var = nir_variable_create(shader, nir_var_uniform, sampler2D, "stipple_tex");
   tex_var->data.binding = binding;
   tex_var->data.explicit_binding = true;
            BITSET_SET(shader->info.textures_used, binding);
   BITSET_SET(shader->info.samplers_used, binding);
            nir_foreach_function_impl(impl, shader) {
         }
      }
      typedef struct {
      nir_variable *line_width_input;
   nir_variable *stipple_counter;
      } lower_aaline;
      static bool
   lower_aaline_instr(nir_builder *b, nir_instr *instr, void *data)
   {
               if (instr->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   if (intrin->intrinsic != nir_intrinsic_store_deref)
            nir_variable *var = nir_intrinsic_get_var(intrin, 0);
   if (var->data.mode != nir_var_shader_out)
         if (var->data.location < FRAG_RESULT_DATA0 && var->data.location != FRAG_RESULT_COLOR)
            nir_def *out_input = intrin->src[1].ssa;
   b->cursor = nir_before_instr(instr);
   nir_def *lw = nir_load_var(b, state->line_width_input);
   nir_def *len = nir_channel(b, lw, 3);
   len = nir_fadd_imm(b, nir_fmul_imm(b, len, 2.0), -1.0);
   nir_def *tmp = nir_fsat(b, nir_fadd(b, nir_channels(b, lw, 0xa),
            nir_def *max = len;
   if (state->stipple_counter) {
               nir_def *counter = nir_load_var(b, state->stipple_counter);
   nir_def *pattern = nir_load_var(b, state->stipple_pattern);
   nir_def *factor = nir_i2f32(b, nir_ishr_imm(b, pattern, 16));
            nir_def *stipple_pos = nir_vec2(b, nir_fadd_imm(b, counter, -0.5),
            stipple_pos = nir_frem(b, nir_fdiv(b, stipple_pos, factor),
            nir_def *p = nir_f2i32(b, stipple_pos);
            // float t = 1.0 - min((1.0 - fract(stipple_pos.x)) * factor, 1.0);
   nir_def *t = nir_ffract(b, nir_channel(b, stipple_pos, 0));
   t = nir_fsub(b, one,
                  // vec2 a = vec2((uvec2(pattern) >> p) & uvec2(1u));
   nir_def *a = nir_i2f32(b,
                  // float cov = mix(a.x, a.y, t);
                        tmp = nir_fmul(b, nir_channel(b, tmp, 0),
                  nir_def *out = nir_vec4(b, nir_channel(b, out_input, 0),
                     nir_src_rewrite(&intrin->src[1], out);
      }
      void
   nir_lower_aaline_fs(struct nir_shader *shader, int *varying,
               {
      lower_aaline state = {
      .stipple_counter = stipple_counter,
      };
            int highest_location = -1, highest_drv_location = -1;
   nir_foreach_shader_in_variable(var, shader) {
   if ((int)var->data.location > highest_location)
         if ((int)var->data.driver_location > highest_drv_location)
                  nir_variable *line_width = nir_variable_create(shader, nir_var_shader_in,
         if (highest_location == -1 || highest_location < VARYING_SLOT_VAR0) {
   line_width->data.location = VARYING_SLOT_VAR0;
   line_width->data.driver_location = highest_drv_location + 1;
   } else {
   line_width->data.location = highest_location + 1;
   line_width->data.driver_location = highest_drv_location + 1;
   }
   shader->num_inputs++;
   *varying = tgsi_get_generic_gl_varying_index(line_width->data.location, true);
            nir_shader_instructions_pass(shader, lower_aaline_instr,
      }
      typedef struct {
      nir_builder b;
   nir_shader *shader;
      } lower_aapoint;
      static void
   nir_lower_aapoint_block(nir_block *block,
         {
   nir_builder *b = &state->b;
   nir_foreach_instr(instr, block) {
         if (instr->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   if (intrin->intrinsic != nir_intrinsic_store_deref)
            nir_variable *var = nir_intrinsic_get_var(intrin, 0);
   if (var->data.mode != nir_var_shader_out)
         if (var->data.location < FRAG_RESULT_DATA0 && var->data.location != FRAG_RESULT_COLOR)
            nir_def *out_input = intrin->src[1].ssa;
            nir_def *tmp = nir_fmul(b, nir_channel(b, out_input, 3), sel);
   nir_def *out = nir_vec4(b, nir_channel(b, out_input, 0),
                              }
      static void
   nir_lower_aapoint_impl(nir_function_impl *impl, lower_aapoint *state,
         {
      nir_block *block = nir_start_block(impl);
            nir_builder *b = &state->b;
            nir_def *dist = nir_fadd(b, nir_fmul(b, nir_channel(b, aainput, 0), nir_channel(b, aainput, 0)),
            nir_def *k = nir_channel(b, aainput, 2);
   nir_def *chan_val_one = nir_channel(b, aainput, 3);
            switch (bool_type) {
   case nir_type_bool1:
      comp = nir_flt(b, chan_val_one, dist);
      case nir_type_bool32:
      comp = nir_flt32(b, chan_val_one, dist);
      case nir_type_float32:
      comp = nir_slt(b, chan_val_one, dist);
      default:
                  nir_discard_if(b, comp);
            /* compute coverage factor = (1-d)/(1-k) */
   /* 1 - k */
   nir_def *tmp = nir_fadd(b, chan_val_one, nir_fneg(b, k));
   /* 1.0 / (1 - k) */
            /* 1 - d */
            /* (1 - d) / (1 - k) */
            /* if (k >= distance)
   *    sel = coverage;
   * else
   *    sel = 1.0;
   */
            switch (bool_type) {
   case nir_type_bool1:
      sel = nir_b32csel(b, nir_fge(b, k, dist), coverage, chan_val_one);
      case nir_type_bool32:
      sel = nir_b32csel(b, nir_fge32(b, k, dist), coverage, chan_val_one);
      case nir_type_float32: {
      /* On this path, don't assume that any "fancy" instructions are
   * supported, but also try to emit something decent.
   *
   *    sel = (k >= distance) ? coverage : 1.0;
   *    sel = (k >= distance) * coverage : (1 - (k >= distance)) * 1.0
   *    sel = (k >= distance) * coverage : (1 - (k >= distance))
   *
   * Since (k >= distance) * coverage is zero when (1 - (k >= distance))
   * is not zero,
   *
   *    sel = (k >= distance) * coverage + (1 - (k >= distance))
   *
   * If we assume that coverage == fsat(coverage), this could be further
   * optimized to fsat(coverage + (1 - (k >= distance))), but I don't feel
   * like verifying that right now.
   */
   nir_def *cmp_result = nir_sge(b, k, dist);
   sel = nir_fadd(b,
                  }
   default:
                  nir_foreach_block(block, impl) {
   nir_lower_aapoint_block(block, state, sel);
      }
      void
   nir_lower_aapoint_fs(struct nir_shader *shader, int *varying, const nir_alu_type bool_type)
   {
      assert(bool_type == nir_type_bool1 ||
                  lower_aapoint state = {
         };
   if (shader->info.stage != MESA_SHADER_FRAGMENT)
            int highest_location = -1, highest_drv_location = -1;
   nir_foreach_shader_in_variable(var, shader) {
   if ((int)var->data.location > highest_location)
         if ((int)var->data.driver_location > highest_drv_location)
                  nir_variable *aapoint_input = nir_variable_create(shader, nir_var_shader_in,
         if (highest_location == -1 || highest_location < VARYING_SLOT_VAR0) {
   aapoint_input->data.location = VARYING_SLOT_VAR0;
   } else {
   aapoint_input->data.location = highest_location + 1;
   }
            shader->num_inputs++;
   *varying = tgsi_get_generic_gl_varying_index(aapoint_input->data.location, true);
            nir_foreach_function_impl(impl, shader) {
            }
