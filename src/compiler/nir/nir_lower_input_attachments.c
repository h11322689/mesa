   /*
   * Copyright © 2016 Intel Corporation
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
      #include "nir.h"
   #include "nir_builder.h"
      static nir_def *
   load_frag_coord(nir_builder *b, nir_deref_instr *deref,
         {
      if (options->use_fragcoord_sysval) {
      nir_def *frag_coord = nir_load_frag_coord(b);
   if (options->unscaled_input_attachment_ir3) {
      nir_variable *var = nir_deref_instr_get_variable(deref);
   unsigned base = var->data.index;
   nir_def *unscaled_frag_coord = nir_load_frag_coord_unscaled_ir3(b);
   if (deref->deref_type == nir_deref_type_array) {
      nir_def *unscaled =
      nir_i2b(b, nir_iand(b, nir_ishr(b, nir_imm_int(b, options->unscaled_input_attachment_ir3 >> base), deref->arr.index.ssa),
         } else {
      assert(deref->deref_type == nir_deref_type_var);
   bool unscaled = (options->unscaled_input_attachment_ir3 >> base) & 1;
         }
               nir_variable *pos = nir_get_variable_with_location(b->shader, nir_var_shader_in,
            /**
   * From Vulkan spec:
   *   "The OriginLowerLeft execution mode must not be used; fragment entry
   *    points must declare OriginUpperLeft."
   *
   * So at this point origin_upper_left should be true
   */
               }
      static nir_def *
   load_layer_id(nir_builder *b, const nir_input_attachment_options *options)
   {
      if (options->use_layer_id_sysval) {
      if (options->use_view_id_for_layer)
         else
               gl_varying_slot slot = options->use_view_id_for_layer ? VARYING_SLOT_VIEW_INDEX : VARYING_SLOT_LAYER;
   nir_variable *layer_id = nir_get_variable_with_location(b->shader, nir_var_shader_in,
                     }
      static bool
   try_lower_input_load(nir_builder *b, nir_intrinsic_instr *load,
         {
      nir_deref_instr *deref = nir_src_as_deref(load->src[0]);
            enum glsl_sampler_dim image_dim = glsl_get_sampler_dim(deref->type);
   if (image_dim != GLSL_SAMPLER_DIM_SUBPASS &&
      image_dim != GLSL_SAMPLER_DIM_SUBPASS_MS)
                           nir_def *frag_coord = load_frag_coord(b, deref, options);
   frag_coord = nir_f2i32(b, frag_coord);
   nir_def *offset = nir_trim_vector(b, load->src[1].ssa, 2);
            nir_def *layer = load_layer_id(b, options);
   nir_def *coord =
                     tex->op = nir_texop_txf;
            tex->dest_type =
         tex->is_array = true;
   tex->is_shadow = false;
            tex->texture_index = 0;
            tex->src[0] = nir_tex_src_for_ssa(nir_tex_src_texture_deref,
         tex->src[1] = nir_tex_src_for_ssa(nir_tex_src_coord, coord);
                     if (image_dim == GLSL_SAMPLER_DIM_SUBPASS_MS) {
      tex->op = nir_texop_txf_ms;
   tex->src[3].src_type = nir_tex_src_ms_index;
                        nir_def_init(&tex->instr, &tex->def, nir_tex_instr_dest_size(tex), 32);
            if (tex->is_sparse) {
      unsigned load_result_size = load->def.num_components - 1;
   nir_component_mask_t load_result_mask = nir_component_mask(load_result_size);
   nir_def *res = nir_channels(
               } else {
      nir_def_rewrite_uses(&load->def,
                  }
      static bool
   try_lower_input_texop(nir_builder *b, nir_tex_instr *tex,
         {
               if (glsl_get_sampler_dim(deref->type) != GLSL_SAMPLER_DIM_SUBPASS_MS)
                     nir_def *frag_coord = load_frag_coord(b, deref, options);
            nir_def *layer = load_layer_id(b, options);
   nir_def *coord = nir_vec3(b, nir_channel(b, frag_coord, 0),
                                 }
      static bool
   lower_input_attachments_instr(nir_builder *b, nir_instr *instr, void *_data)
   {
               switch (instr->type) {
   case nir_instr_type_tex: {
               if (tex->op == nir_texop_fragment_mask_fetch_amd ||
                     }
   case nir_instr_type_intrinsic: {
               if (load->intrinsic == nir_intrinsic_image_deref_load ||
                              default:
            }
      bool
   nir_lower_input_attachments(nir_shader *shader,
         {
               return nir_shader_instructions_pass(shader, lower_input_attachments_instr,
                  }
