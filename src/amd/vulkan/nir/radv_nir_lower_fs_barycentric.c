   /*
   * Copyright © 2023 Valve Corporation
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
   */
      #include "nir/nir.h"
   #include "nir/nir_builder.h"
   #include "radv_nir.h"
   #include "radv_private.h"
      typedef struct {
      bool dynamic_rasterization_samples;
   unsigned num_rasterization_samples;
      } lower_fs_barycentric_state;
      static nir_def *
   lower_interp_center_smooth(nir_builder *b, nir_def *offset)
   {
               nir_def *deriv_x =
      nir_vec3(b, nir_fddx_fine(b, nir_channel(b, pull_model, 0)), nir_fddx_fine(b, nir_channel(b, pull_model, 1)),
      nir_def *deriv_y =
      nir_vec3(b, nir_fddy_fine(b, nir_channel(b, pull_model, 0)), nir_fddy_fine(b, nir_channel(b, pull_model, 1)),
         nir_def *offset_x = nir_channel(b, offset, 0);
            nir_def *adjusted_x = nir_fadd(b, pull_model, nir_fmul(b, deriv_x, offset_x));
                     /* Get W by using the reciprocal of 1/W. */
               }
      static nir_def *
   lower_barycentric_coord_at_offset(nir_builder *b, nir_def *src, enum glsl_interp_mode mode)
   {
      if (mode == INTERP_MODE_SMOOTH)
               }
      static nir_def *
   lower_barycentric_coord_at_sample(nir_builder *b, lower_fs_barycentric_state *state, nir_intrinsic_instr *intrin)
   {
      const enum glsl_interp_mode mode = (enum glsl_interp_mode)nir_intrinsic_interp_mode(intrin);
   nir_def *num_samples = nir_load_rasterization_samples_amd(b);
            if (state->dynamic_rasterization_samples) {
               nir_push_if(b, nir_ieq_imm(b, num_samples, 1));
   {
         }
   nir_push_else(b, NULL);
   {
                                 }
               } else {
      if (!state->num_rasterization_samples) {
         } else {
                                                }
      static nir_def *
   get_interp_param(nir_builder *b, lower_fs_barycentric_state *state, nir_intrinsic_instr *intrin)
   {
               if (intrin->intrinsic == nir_intrinsic_load_barycentric_coord_pixel) {
         } else if (intrin->intrinsic == nir_intrinsic_load_barycentric_coord_at_offset) {
         } else if (intrin->intrinsic == nir_intrinsic_load_barycentric_coord_at_sample) {
         } else if (intrin->intrinsic == nir_intrinsic_load_barycentric_coord_centroid) {
         } else {
      assert(intrin->intrinsic == nir_intrinsic_load_barycentric_coord_sample);
                  }
      static nir_def *
   lower_point(nir_builder *b)
   {
               coords[0] = nir_imm_float(b, 1.0f);
   coords[1] = nir_imm_float(b, 0.0f);
               }
      static nir_def *
   lower_line(nir_builder *b, nir_def *p1, nir_def *p2)
   {
               coords[1] = nir_fadd(b, p1, p2);
   coords[0] = nir_fsub_imm(b, 1.0f, coords[1]);
               }
      static nir_def *
   lower_triangle(nir_builder *b, nir_def *p1, nir_def *p2)
   {
      nir_def *v0_bary[3], *v1_bary[3], *v2_bary[3];
            /* Compute the provoking vertex ID:
   *
   * quad_id = thread_id >> 2
   * provoking_vtx_id = (provoking_vtx >> (quad_id << 1)) & 3
   */
   nir_def *quad_id = nir_ushr_imm(b, nir_load_subgroup_invocation(b), 2);
   nir_def *provoking_vtx = nir_load_provoking_vtx_amd(b);
            /* Compute barycentrics. */
   v0_bary[0] = nir_fsub(b, nir_fsub_imm(b, 1.0f, p2), p1);
   v0_bary[1] = p1;
            v1_bary[0] = p1;
   v1_bary[1] = p2;
            v2_bary[0] = p2;
   v2_bary[1] = nir_fsub(b, nir_fsub_imm(b, 1.0f, p2), p1);
            /* Select barycentrics for the given provoking vertex ID. */
   for (unsigned i = 0; i < 3; i++) {
      coords[i] = nir_bcsel(b, nir_ieq_imm(b, provoking_vtx_id, 2), v2_bary[i],
                  }
      static bool
   lower_load_barycentric_coord(nir_builder *b, lower_fs_barycentric_state *state, nir_intrinsic_instr *intrin)
   {
      nir_def *interp, *p1, *p2;
                     /* When the rasterization primitive isn't known at compile time (GPL), load it. */
   if (state->rast_prim == -1) {
      nir_def *rast_prim = nir_load_rasterization_primitive_amd(b);
            nir_def *is_point = nir_ieq_imm(b, rast_prim, V_028A6C_POINTLIST);
   nir_if *if_point = nir_push_if(b, is_point);
   {
         }
   nir_push_else(b, if_point);
   {
               interp = get_interp_param(b, state, intrin);
                  nir_def *is_line = nir_ieq_imm(b, rast_prim, V_028A6C_LINESTRIP);
   nir_if *if_line = nir_push_if(b, is_line);
   {
         }
   nir_push_else(b, if_line);
   {
                           }
               } else {
      if (state->rast_prim == V_028A6C_POINTLIST) {
         } else {
      interp = get_interp_param(b, state, intrin);
                  if (state->rast_prim == V_028A6C_LINESTRIP) {
         } else {
      assert(state->rast_prim == V_028A6C_TRISTRIP);
                     nir_def_rewrite_uses(&intrin->def, new_dest);
               }
      bool
   radv_nir_lower_fs_barycentric(nir_shader *shader, const struct radv_pipeline_key *key, unsigned rast_prim)
   {
      nir_function_impl *impl = nir_shader_get_entrypoint(shader);
                     lower_fs_barycentric_state state = {
      .dynamic_rasterization_samples = key->dynamic_rasterization_samples,
   .num_rasterization_samples = key->ps.num_samples,
               nir_foreach_function (function, shader) {
      if (!function->impl)
                     nir_foreach_block (block, impl) {
      nir_foreach_instr_safe (instr, block) {
                     nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   if (intrin->intrinsic != nir_intrinsic_load_barycentric_coord_pixel &&
      intrin->intrinsic != nir_intrinsic_load_barycentric_coord_centroid &&
   intrin->intrinsic != nir_intrinsic_load_barycentric_coord_sample &&
                                                   }
