   /*
   * Copyright © 2014-2015 Broadcom
   * Copyright © 2021 Google
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
      #include "nir_builder.h"
      nir_builder MUST_CHECK PRINTFLIKE(3, 4)
      nir_builder_init_simple_shader(gl_shader_stage stage,
            {
               memset(&b, 0, sizeof(b));
            if (name) {
      va_list args;
   va_start(args, name);
   b.shader->info.name = ralloc_vasprintf(b.shader, name, args);
               nir_function *func = nir_function_create(b.shader, "main");
   func->is_entrypoint = true;
   b.exact = false;
   b.impl = nir_function_impl_create(func);
            /* Simple shaders are typically internal, e.g. blit shaders */
            /* Compute shaders on Vulkan require some workgroup size initialized, pick
   * a safe default value. This relies on merging workgroups for efficiency.
   */
   b.shader->info.workgroup_size[0] = 1;
   b.shader->info.workgroup_size[1] = 1;
               }
      nir_def *
   nir_builder_alu_instr_finish_and_insert(nir_builder *build, nir_alu_instr *instr)
   {
                        /* Guess the number of components the destination temporary should have
   * based on our input sizes, if it's not fixed for the op.
   */
   unsigned num_components = op_info->output_size;
   if (num_components == 0) {
      for (unsigned i = 0; i < op_info->num_inputs; i++) {
      if (op_info->input_sizes[i] == 0)
      num_components = MAX2(num_components,
      }
            /* Figure out the bitwidth based on the source bitwidth if the instruction
   * is variable-width.
   */
   unsigned bit_size = nir_alu_type_get_type_size(op_info->output_type);
   if (bit_size == 0) {
      for (unsigned i = 0; i < op_info->num_inputs; i++) {
      unsigned src_bit_size = instr->src[i].src.ssa->bit_size;
   if (nir_alu_type_get_type_size(op_info->input_types[i]) == 0) {
      if (bit_size)
         else
      } else {
      assert(src_bit_size ==
                     /* When in doubt, assume 32. */
   if (bit_size == 0)
            /* Make sure we don't swizzle from outside of our source vector (like if a
   * scalar value was passed into a multiply with a vector).
   */
   for (unsigned i = 0; i < op_info->num_inputs; i++) {
      for (unsigned j = instr->src[i].src.ssa->num_components;
      j < NIR_MAX_VEC_COMPONENTS; j++) {
                  nir_def_init(&instr->instr, &instr->def, num_components,
                        }
      nir_def *
   nir_build_alu(nir_builder *build, nir_op op, nir_def *src0,
         {
      nir_alu_instr *instr = nir_alu_instr_create(build->shader, op);
   if (!instr)
            instr->src[0].src = nir_src_for_ssa(src0);
   if (src1)
         if (src2)
         if (src3)
               }
      nir_def *
   nir_build_alu1(nir_builder *build, nir_op op, nir_def *src0)
   {
      nir_alu_instr *instr = nir_alu_instr_create(build->shader, op);
   if (!instr)
                        }
      nir_def *
   nir_build_alu2(nir_builder *build, nir_op op, nir_def *src0,
         {
      nir_alu_instr *instr = nir_alu_instr_create(build->shader, op);
   if (!instr)
            instr->src[0].src = nir_src_for_ssa(src0);
               }
      nir_def *
   nir_build_alu3(nir_builder *build, nir_op op, nir_def *src0,
         {
      nir_alu_instr *instr = nir_alu_instr_create(build->shader, op);
   if (!instr)
            instr->src[0].src = nir_src_for_ssa(src0);
   instr->src[1].src = nir_src_for_ssa(src1);
               }
      nir_def *
   nir_build_alu4(nir_builder *build, nir_op op, nir_def *src0,
         {
      nir_alu_instr *instr = nir_alu_instr_create(build->shader, op);
   if (!instr)
            instr->src[0].src = nir_src_for_ssa(src0);
   instr->src[1].src = nir_src_for_ssa(src1);
   instr->src[2].src = nir_src_for_ssa(src2);
               }
      /* for the couple special cases with more than 4 src args: */
   nir_def *
   nir_build_alu_src_arr(nir_builder *build, nir_op op, nir_def **srcs)
   {
      const nir_op_info *op_info = &nir_op_infos[op];
   nir_alu_instr *instr = nir_alu_instr_create(build->shader, op);
   if (!instr)
            for (unsigned i = 0; i < op_info->num_inputs; i++)
               }
      nir_def *
   nir_build_tex_deref_instr(nir_builder *build, nir_texop op,
                           {
      assert(texture != NULL);
   assert(glsl_type_is_image(texture->type) ||
                           nir_tex_instr *tex = nir_tex_instr_create(build->shader, num_srcs);
   tex->op = op;
   tex->sampler_dim = glsl_get_sampler_dim(texture->type);
   tex->is_array = glsl_sampler_type_is_array(texture->type);
            switch (op) {
   case nir_texop_txs:
   case nir_texop_texture_samples:
   case nir_texop_query_levels:
   case nir_texop_txf_ms_mcs_intel:
   case nir_texop_fragment_mask_fetch_amd:
   case nir_texop_descriptor_amd:
      tex->dest_type = nir_type_int32;
      case nir_texop_lod:
      tex->dest_type = nir_type_float32;
      case nir_texop_samples_identical:
      tex->dest_type = nir_type_bool1;
      default:
      assert(!nir_tex_instr_is_query(tex));
   tex->dest_type = nir_get_nir_type_for_glsl_base_type(
                     unsigned src_idx = 0;
   tex->src[src_idx++] = nir_tex_src_for_ssa(nir_tex_src_texture_deref,
         if (sampler != NULL) {
      assert(glsl_type_is_sampler(sampler->type));
   tex->src[src_idx++] = nir_tex_src_for_ssa(nir_tex_src_sampler_deref,
      }
   for (unsigned i = 0; i < num_extra_srcs; i++) {
      switch (extra_srcs[i].src_type) {
   case nir_tex_src_coord:
      tex->coord_components = nir_src_num_components(extra_srcs[i].src);
   assert(tex->coord_components == tex->is_array +
               case nir_tex_src_lod:
      assert(tex->sampler_dim == GLSL_SAMPLER_DIM_1D ||
         tex->sampler_dim == GLSL_SAMPLER_DIM_2D ||
               case nir_tex_src_ms_index:
                  case nir_tex_src_comparator:
      /* Assume 1-component shadow for the builder helper */
   tex->is_shadow = true;
               case nir_tex_src_texture_deref:
   case nir_tex_src_sampler_deref:
   case nir_tex_src_texture_offset:
   case nir_tex_src_sampler_offset:
   case nir_tex_src_texture_handle:
   case nir_tex_src_sampler_handle:
                  default:
                     }
            nir_def_init(&tex->instr, &tex->def, nir_tex_instr_dest_size(tex),
                     }
      nir_def *
   nir_vec_scalars(nir_builder *build, nir_scalar *comp, unsigned num_components)
   {
      nir_op op = nir_op_vec(num_components);
   nir_alu_instr *instr = nir_alu_instr_create(build->shader, op);
   if (!instr)
            for (unsigned i = 0; i < num_components; i++) {
      instr->src[i].src = nir_src_for_ssa(comp[i].def);
      }
            /* Note: not reusing nir_builder_alu_instr_finish_and_insert() because it
   * can't re-guess the num_components when num_components == 1 (nir_op_mov).
   */
   nir_def_init(&instr->instr, &instr->def, num_components,
                        }
      /**
   * Get nir_def for an alu src, respecting the nir_alu_src's swizzle.
   */
   nir_def *
   nir_ssa_for_alu_src(nir_builder *build, nir_alu_instr *instr, unsigned srcn)
   {
      if (nir_alu_src_is_trivial_ssa(instr, srcn))
            nir_alu_src *src = &instr->src[srcn];
   unsigned num_components = nir_ssa_alu_instr_src_components(instr, srcn);
      }
      /* Generic builder for system values. */
   nir_def *
   nir_load_system_value(nir_builder *build, nir_intrinsic_op op, int index,
         {
      nir_intrinsic_instr *load = nir_intrinsic_instr_create(build->shader, op);
   if (nir_intrinsic_infos[op].dest_components > 0)
         else
                  nir_def_init(&load->instr, &load->def, num_components, bit_size);
   nir_builder_instr_insert(build, &load->instr);
      }
      void
   nir_builder_instr_insert(nir_builder *build, nir_instr *instr)
   {
               if (build->update_divergence)
            /* Move the cursor forward. */
      }
      void
   nir_builder_cf_insert(nir_builder *build, nir_cf_node *cf)
   {
         }
      bool
   nir_builder_is_inside_cf(nir_builder *build, nir_cf_node *cf_node)
   {
      nir_block *block = nir_cursor_current_block(build->cursor);
   for (nir_cf_node *n = &block->cf_node; n; n = n->parent) {
      if (n == cf_node)
      }
      }
      nir_if *
   nir_push_if(nir_builder *build, nir_def *condition)
   {
      nir_if *nif = nir_if_create(build->shader);
   nif->condition = nir_src_for_ssa(condition);
   nir_builder_cf_insert(build, &nif->cf_node);
   build->cursor = nir_before_cf_list(&nif->then_list);
      }
      nir_if *
   nir_push_else(nir_builder *build, nir_if *nif)
   {
      if (nif) {
         } else {
      nir_block *block = nir_cursor_current_block(build->cursor);
      }
   build->cursor = nir_before_cf_list(&nif->else_list);
      }
      void
   nir_pop_if(nir_builder *build, nir_if *nif)
   {
      if (nif) {
         } else {
      nir_block *block = nir_cursor_current_block(build->cursor);
      }
      }
      nir_def *
   nir_if_phi(nir_builder *build, nir_def *then_def, nir_def *else_def)
   {
      nir_block *block = nir_cursor_current_block(build->cursor);
            nir_phi_instr *phi = nir_phi_instr_create(build->shader);
   nir_phi_instr_add_src(phi, nir_if_last_then_block(nif), then_def);
            assert(then_def->num_components == else_def->num_components);
   assert(then_def->bit_size == else_def->bit_size);
   nir_def_init(&phi->instr, &phi->def, then_def->num_components,
                        }
      nir_loop *
   nir_push_loop(nir_builder *build)
   {
      nir_loop *loop = nir_loop_create(build->shader);
   nir_builder_cf_insert(build, &loop->cf_node);
   build->cursor = nir_before_cf_list(&loop->body);
      }
      nir_loop *
   nir_push_continue(nir_builder *build, nir_loop *loop)
   {
      if (loop) {
         } else {
      nir_block *block = nir_cursor_current_block(build->cursor);
                        build->cursor = nir_before_cf_list(&loop->continue_list);
      }
      void
   nir_pop_loop(nir_builder *build, nir_loop *loop)
   {
      if (loop) {
         } else {
      nir_block *block = nir_cursor_current_block(build->cursor);
      }
      }
      nir_def *
   nir_compare_func(nir_builder *b, enum compare_func func,
         {
      switch (func) {
   case COMPARE_FUNC_NEVER:
         case COMPARE_FUNC_ALWAYS:
         case COMPARE_FUNC_EQUAL:
         case COMPARE_FUNC_NOTEQUAL:
         case COMPARE_FUNC_GREATER:
         case COMPARE_FUNC_GEQUAL:
         case COMPARE_FUNC_LESS:
         case COMPARE_FUNC_LEQUAL:
         }
      }
      nir_def *
   nir_type_convert(nir_builder *b,
                  nir_def *src,
      {
      assert(nir_alu_type_get_type_size(src_type) == 0 ||
            const nir_alu_type dst_base =
            const nir_alu_type src_base =
            /* b2b uses the regular type conversion path, but i2b and f2b are
   * implemented as src != 0.
   */
   if (dst_base == nir_type_bool && src_base != nir_type_bool) {
                        if (src_base == nir_type_float) {
      switch (dst_bit_size) {
   case 1:
      opcode = nir_op_fneu;
      case 8:
      opcode = nir_op_fneu8;
      case 16:
      opcode = nir_op_fneu16;
      case 32:
      opcode = nir_op_fneu32;
      default:
            } else {
               switch (dst_bit_size) {
   case 1:
      opcode = nir_op_ine;
      case 8:
      opcode = nir_op_ine8;
      case 16:
      opcode = nir_op_ine16;
      case 32:
      opcode = nir_op_ine32;
      default:
                     return nir_build_alu(b, opcode, src,
            } else {
               nir_op opcode =
         if (opcode == nir_op_mov)
                  }
      nir_def *
   nir_gen_rect_vertices(nir_builder *b, nir_def *z, nir_def *w)
   {
      if (!z)
         if (!w)
            nir_def *vertex_id;
   if (b->shader->options && b->shader->options->vertex_id_zero_based)
         else
            /* vertex 0: -1.0, -1.0
   * vertex 1: -1.0,  1.0
   * vertex 2:  1.0, -1.0
   * vertex 3:  1.0,  1.0
   *
   * so:
   *
   * channel 0 is vertex_id < 2 ? -1.0 :  1.0
   * channel 1 is vertex_id & 1 ?  1.0 : -1.0
            nir_def *c0cmp = nir_ilt_imm(b, vertex_id, 2);
            nir_def *comp[4];
   comp[0] = nir_bcsel(b, c0cmp, nir_imm_float(b, -1.0), nir_imm_float(b, 1.0));
   comp[1] = nir_bcsel(b, c1cmp, nir_imm_float(b, 1.0), nir_imm_float(b, -1.0));
   comp[2] = z;
               }
