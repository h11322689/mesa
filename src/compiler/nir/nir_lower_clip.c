   /*
   * Copyright © 2015 Red Hat
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   *
   * Authors:
   *    Rob Clark <robclark@freedesktop.org>
   */
      #include "nir.h"
   #include "nir_builder.h"
      #define MAX_CLIP_PLANES 8
      /* Generates the lowering code for user-clip-planes, generating CLIPDIST
   * from UCP[n] + CLIPVERTEX or POSITION.  Additionally, an optional pass
   * for fragment shaders to insert conditional kills based on the inter-
   * polated CLIPDIST
   *
   * NOTE: should be run after nir_lower_outputs_to_temporaries() (or at
   * least in scenarios where you can count on each output written once
   * and only once).
   */
      static nir_variable *
   create_clipdist_var(nir_shader *shader,
         {
               if (output) {
      var->data.driver_location = shader->num_outputs;
   var->data.mode = nir_var_shader_out;
      } else {
      var->data.driver_location = shader->num_inputs;
   var->data.mode = nir_var_shader_in;
      }
   var->name = ralloc_asprintf(var, "clipdist_%d", var->data.driver_location);
   var->data.index = 0;
            if (array_size > 0) {
      var->type = glsl_array_type(glsl_float_type(), array_size,
            } else
            nir_shader_add_variable(shader, var);
      }
      static void
   create_clipdist_vars(nir_shader *shader, nir_variable **io_vars,
               {
      shader->info.clip_distance_array_size = util_last_bit(ucp_enables);
   if (use_clipdist_array) {
      io_vars[0] =
      create_clipdist_var(shader, output,
         } else {
      if (ucp_enables & 0x0f)
      io_vars[0] =
      create_clipdist_var(shader, output,
   if (ucp_enables & 0xf0)
      io_vars[1] =
            }
      static void
   store_clipdist_output(nir_builder *b, nir_variable *out, int location_offset,
         {
      nir_io_semantics semantics = {
      .location = out->data.location,
               nir_store_output(b, nir_vec4(b, val[0], val[1], val[2], val[3]), nir_imm_int(b, location_offset),
                  .base = out->data.driver_location,
   }
      static void
   load_clipdist_input(nir_builder *b, nir_variable *in, int location_offset,
         {
      nir_io_semantics semantics = {
      .location = in->data.location,
               nir_def *load;
   if (b->shader->options->use_interpolated_input_intrinsics) {
      /* TODO: use sample when per-sample shading? */
   nir_def *barycentric = nir_load_barycentric(
         load = nir_load_interpolated_input(
      b, 4, 32, barycentric, nir_imm_int(b, location_offset),
   .base = in->data.driver_location,
            } else {
      load = nir_load_input(b, 4, 32, nir_imm_int(b, location_offset),
                           val[0] = nir_channel(b, load, 0);
   val[1] = nir_channel(b, load, 1);
   val[2] = nir_channel(b, load, 2);
      }
      static nir_def *
   find_output_in_block(nir_block *block, unsigned drvloc)
   {
                  if (instr->type == nir_instr_type_intrinsic) {
      nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   if ((intr->intrinsic == nir_intrinsic_store_output) &&
      nir_intrinsic_base(intr) == drvloc) {
   assert(nir_src_is_const(intr->src[1]));
                        }
      /* TODO: maybe this would be a useful helper?
   * NOTE: assumes each output is written exactly once (and unconditionally)
   * so if needed nir_lower_outputs_to_temporaries()
   */
   static nir_def *
   find_output(nir_shader *shader, unsigned drvloc)
   {
      nir_def *def = NULL;
   nir_foreach_function_impl(impl, shader) {
      nir_foreach_block_reverse(block, impl) {
      nir_def *new_def = find_output_in_block(block, drvloc);
      #if !defined(DEBUG)
            /* for debug builds, scan entire shader to assert
   * if output is written multiple times.  For release
   * builds just assume all is well and bail when we
   * find first:
   */
      #endif
                        }
      static bool
   find_clipvertex_and_position_outputs(nir_shader *shader,
               {
      nir_foreach_shader_out_variable(var, shader) {
      switch (var->data.location) {
   case VARYING_SLOT_POS:
      *position = var;
      case VARYING_SLOT_CLIP_VERTEX:
      *clipvertex = var;
      case VARYING_SLOT_CLIP_DIST0:
   case VARYING_SLOT_CLIP_DIST1:
      /* if shader is already writing CLIPDIST, then
   * there should be no user-clip-planes to deal
   * with.
   *
   * We assume nir_remove_dead_variables has removed the clipdist
   * variables if they're not written.
   */
                     }
      static nir_def *
   get_ucp(nir_builder *b, int plane,
         {
      if (clipplane_state_tokens) {
      char tmp[100];
   snprintf(tmp, ARRAY_SIZE(tmp), "gl_ClipPlane%dMESA", plane);
   nir_variable *var = nir_state_variable_create(b->shader,
                  } else
      }
      static void
   lower_clip_outputs(nir_builder *b, nir_variable *position,
                     nir_variable *clipvertex, nir_variable **out,
   {
      nir_def *clipdist[MAX_CLIP_PLANES];
            if (use_vars) {
               if (clipvertex) {
      clipvertex->data.mode = nir_var_shader_temp;
         } else {
      if (clipvertex)
         else {
      assert(position);
                  for (int plane = 0; plane < MAX_CLIP_PLANES; plane++) {
      if (ucp_enables & (1 << plane)) {
               /* calculate clipdist[plane] - dot(ucp, cv): */
      } else {
      /* 0.0 == don't-clip == disabled: */
      }
   if (use_clipdist_array && use_vars && plane < util_last_bit(ucp_enables)) {
      nir_deref_instr *deref;
   deref = nir_build_deref_array_imm(b,
                              if (!use_clipdist_array || !use_vars) {
      if (use_vars) {
      if (ucp_enables & 0x0f)
         if (ucp_enables & 0xf0)
      } else if (use_clipdist_array) {
      if (ucp_enables & 0x0f)
         if (ucp_enables & 0xf0)
      } else {
      if (ucp_enables & 0x0f)
         if (ucp_enables & 0xf0)
            }
      /*
   * VS lowering
   */
      /* ucp_enables is bitmask of enabled ucps.  Actual ucp values are
   * passed in to shader via user_clip_plane system-values
   *
   * If use_vars is true, the pass will use variable loads and stores instead
   * of working with store_output intrinsics.
   *
   * If use_clipdist_array is true, the pass will use compact arrays for the
   * clipdist output instead of two vec4s.
   */
   bool
   nir_lower_clip_vs(nir_shader *shader, unsigned ucp_enables, bool use_vars,
               {
      nir_function_impl *impl = nir_shader_get_entrypoint(shader);
   nir_builder b;
   nir_variable *position = NULL;
   nir_variable *clipvertex = NULL;
            if (!ucp_enables)
                     /* NIR should ensure that, even in case of loops/if-else, there
   * should be only a single predecessor block to end_block, which
   * makes the perfect place to insert the clipdist calculations.
   *
   * NOTE: in case of early returns, these would have to be lowered
   * to jumps to end_block predecessor in a previous pass.  Not sure
   * if there is a good way to sanity check this, but for now the
   * users of this pass don't support sub-routines.
   */
   assert(impl->end_block->predecessors->entries == 1);
            /* find clipvertex/position outputs */
   if (!find_clipvertex_and_position_outputs(shader, &clipvertex, &position))
            /* insert CLIPDIST outputs */
   create_clipdist_vars(shader, out, ucp_enables, true,
            lower_clip_outputs(&b, position, clipvertex, out, ucp_enables, use_vars,
                        }
      static void
   lower_clip_in_gs_block(nir_builder *b, nir_block *block, nir_variable *position,
                     {
      nir_foreach_instr_safe(instr, block) {
      if (instr->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   switch (intrin->intrinsic) {
   case nir_intrinsic_emit_vertex_with_counter:
   case nir_intrinsic_emit_vertex:
      b->cursor = nir_before_instr(instr);
   lower_clip_outputs(b, position, clipvertex, out, ucp_enables, true,
            default:
      /* not interesting; skip this */
            }
      /*
   * GS lowering
   */
      bool
   nir_lower_clip_gs(nir_shader *shader, unsigned ucp_enables,
               {
      nir_function_impl *impl = nir_shader_get_entrypoint(shader);
   nir_builder b;
   nir_variable *position = NULL;
   nir_variable *clipvertex = NULL;
            if (!ucp_enables)
            /* find clipvertex/position outputs */
   if (!find_clipvertex_and_position_outputs(shader, &clipvertex, &position))
            /* insert CLIPDIST outputs */
   create_clipdist_vars(shader, out, ucp_enables, true,
                     nir_foreach_block(block, impl)
      lower_clip_in_gs_block(&b, block, position, clipvertex, out,
                           }
      /*
   * FS lowering
   */
      static void
   lower_clip_fs(nir_function_impl *impl, unsigned ucp_enables,
         {
      nir_def *clipdist[MAX_CLIP_PLANES];
            if (!use_clipdist_array) {
      if (ucp_enables & 0x0f)
         if (ucp_enables & 0xf0)
      } else {
      if (ucp_enables & 0x0f)
         if (ucp_enables & 0xf0)
                        for (int plane = 0; plane < MAX_CLIP_PLANES; plane++) {
      if (ucp_enables & (1 << plane)) {
                                    if (cond != NULL) {
      nir_discard_if(&b, cond);
                  }
      static bool
   fs_has_clip_dist_input_var(nir_shader *shader, nir_variable **io_vars,
         {
      assert(shader->info.stage == MESA_SHADER_FRAGMENT);
   nir_foreach_shader_in_variable(var, shader) {
      switch (var->data.location) {
   case VARYING_SLOT_CLIP_DIST0:
      assert(var->data.compact);
   io_vars[0] = var;
   *ucp_enables &= (1 << glsl_get_length(var->type)) - 1;
      default:
            }
      }
      /* insert conditional kill based on interpolated CLIPDIST
   */
   bool
   nir_lower_clip_fs(nir_shader *shader, unsigned ucp_enables,
         {
               if (!ucp_enables)
            /* No hard reason to require use_clipdist_arr to work with
   * frag-shader-based gl_ClipDistance, except that the only user that does
   * not enable this does not support GL 3.0 (or EXT_clip_cull_distance).
   */
   if (!fs_has_clip_dist_input_var(shader, in, &ucp_enables))
         else
            nir_foreach_function_with_impl(function, impl, shader) {
      if (!strcmp(function->name, "main"))
                  }
