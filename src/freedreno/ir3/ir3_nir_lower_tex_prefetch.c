   /*
   * Copyright © 2019 Igalia S.L.
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
      #include "ir3_nir.h"
      /**
   * A pass which detects tex instructions which are candidate to be executed
   * prior to FS shader start, and change them to nir_texop_tex_prefetch.
   */
      static int
   coord_offset(nir_def *ssa)
   {
               /* The coordinate of a texture sampling instruction eligible for
   * pre-fetch is either going to be a load_interpolated_input/
   * load_input, or a vec2 assembling non-swizzled components of
   * a load_interpolated_input/load_input (due to varying packing)
            if (parent_instr->type == nir_instr_type_alu) {
               if (alu->op != nir_op_vec2)
            int base_src_offset = coord_offset(alu->src[0].src.ssa);
   if (base_src_offset < 0)
                     /* NOTE it might be possible to support more than 2D? */
   for (int i = 1; i < 2; i++) {
      int nth_src_offset = coord_offset(alu->src[i].src.ssa);
   if (nth_src_offset < 0)
                  if (nth_offset != (base_offset + i))
                           if (parent_instr->type != nir_instr_type_intrinsic)
                     if (input->intrinsic != nir_intrinsic_load_interpolated_input)
            /* Happens with lowered load_barycentric_at_offset */
   if (input->src[0].ssa->parent_instr->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *interp =
            if (interp->intrinsic != nir_intrinsic_load_barycentric_pixel)
            /* interpolation modes such as noperspective aren't covered by the other
   * test, we need to explicitly check for them here.
   */
   unsigned interp_mode = nir_intrinsic_interp_mode(interp);
   if (interp_mode != INTERP_MODE_NONE && interp_mode != INTERP_MODE_SMOOTH)
            /* we also need a const input offset: */
   if (!nir_src_is_const(input->src[1]))
            unsigned base = nir_src_as_uint(input->src[1]) + nir_intrinsic_base(input);
               }
      int
   ir3_nir_coord_offset(nir_def *ssa)
   {
         assert(ssa->num_components == 2);
      }
      static bool
   has_src(nir_tex_instr *tex, nir_tex_src_type type)
   {
         }
      static bool
   ok_bindless_src(nir_tex_instr *tex, nir_tex_src_type type)
   {
      int idx = nir_tex_instr_src_index(tex, type);
   assert(idx >= 0);
            /* TODO from SP_FS_BINDLESS_PREFETCH[n] it looks like this limit should
   * be 1<<8 ?
   */
   return nir_src_is_const(bindless->src[0]) &&
      }
      /**
   * Check that we will be able to encode the tex/samp parameters
   * successfully.  These limits are based on the layout of
   * SP_FS_PREFETCH[n] and SP_FS_BINDLESS_PREFETCH[n], so at some
   * point (if those regs changes) they may become generation
   * specific.
   */
   static bool
   ok_tex_samp(nir_tex_instr *tex)
   {
      if (has_src(tex, nir_tex_src_texture_handle)) {
                        return ok_bindless_src(tex, nir_tex_src_texture_handle) &&
      } else {
      assert(!has_src(tex, nir_tex_src_texture_offset));
                  }
      static bool
   lower_tex_prefetch_block(nir_block *block)
   {
               nir_foreach_instr_safe (instr, block) {
      if (instr->type != nir_instr_type_tex)
            nir_tex_instr *tex = nir_instr_as_tex(instr);
   if (tex->op != nir_texop_tex)
            if (has_src(tex, nir_tex_src_bias) || has_src(tex, nir_tex_src_lod) ||
      has_src(tex, nir_tex_src_comparator) ||
   has_src(tex, nir_tex_src_projector) ||
   has_src(tex, nir_tex_src_offset) || has_src(tex, nir_tex_src_ddx) ||
   has_src(tex, nir_tex_src_ddy) || has_src(tex, nir_tex_src_ms_index) ||
   has_src(tex, nir_tex_src_texture_offset) ||
               /* only prefetch for simple 2d tex fetch case */
   if (tex->sampler_dim != GLSL_SAMPLER_DIM_2D || tex->is_array)
            if (!ok_tex_samp(tex))
            int idx = nir_tex_instr_src_index(tex, nir_tex_src_coord);
   /* First source should be the sampling coordinate. */
            if (ir3_nir_coord_offset(coord->src.ssa) >= 0) {
                                 }
      static bool
   lower_tex_prefetch_func(nir_function_impl *impl)
   {
      /* Only instructions in the the outer-most block are considered eligible for
   * pre-dispatch, because they need to be move-able to the beginning of the
   * shader to avoid locking down the register holding the pre-fetched result
   * for too long. However if there is a preamble we should skip the preamble
   * and only look in the first block after the preamble instead, because that
   * corresponds to the first block in the original program and texture fetches
   * in the preamble are never pre-dispatchable.
   */
            nir_if *nif = nir_block_get_following_if(block);
   if (nif) {
      nir_instr *cond = nif->condition.ssa->parent_instr;
   if (cond->type == nir_instr_type_intrinsic &&
      nir_instr_as_intrinsic(cond)->intrinsic ==
   nir_intrinsic_preamble_start_ir3) {
                           if (progress) {
      nir_metadata_preserve(impl,
                  }
      bool
   ir3_nir_lower_tex_prefetch(nir_shader *shader)
   {
                        nir_foreach_function (function, shader) {
      /* Only texture sampling instructions inside the main function
   * are eligible for pre-dispatch.
   */
   if (!function->impl || !function->is_entrypoint)
                           }
