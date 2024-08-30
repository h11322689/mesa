   /* -*- mesa-c++  -*-
   *
   * Copyright (c) 2019 Collabora LTD
   *
   * Author: Gert Wollny <gert.wollny@collabora.com>
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   */
      #include "sfn_nir.h"
      #include "../r600_pipe.h"
   #include "../r600_shader.h"
   #include "nir.h"
   #include "nir_builder.h"
   #include "nir_intrinsics.h"
   #include "r600_asm.h"
   #include "sfn_assembler.h"
   #include "sfn_debug.h"
   #include "sfn_instr_tex.h"
   #include "sfn_liverangeevaluator.h"
   #include "sfn_nir_lower_alu.h"
   #include "sfn_nir_lower_fs_out_to_vector.h"
   #include "sfn_nir_lower_tex.h"
   #include "sfn_optimizer.h"
   #include "sfn_ra.h"
   #include "sfn_scheduler.h"
   #include "sfn_shader.h"
   #include "sfn_split_address_loads.h"
   #include "util/u_prim.h"
      #include <vector>
      namespace r600 {
      using std::vector;
      NirLowerInstruction::NirLowerInstruction():
         {
   }
      bool
   NirLowerInstruction::filter_instr(const nir_instr *instr, const void *data)
   {
      auto me = reinterpret_cast<const NirLowerInstruction *>(data);
      }
      nir_def *
   NirLowerInstruction::lower_instr(nir_builder *b, nir_instr *instr, void *data)
   {
      auto me = reinterpret_cast<NirLowerInstruction *>(data);
   me->set_builder(b);
      }
      bool
   NirLowerInstruction::run(nir_shader *shader)
   {
         }
      AssemblyFromShader::~AssemblyFromShader() {}
      bool
   AssemblyFromShader::lower(const Shader& ir)
   {
         }
      static void
   r600_nir_lower_scratch_address_impl(nir_builder *b, nir_intrinsic_instr *instr)
   {
               int address_index = 0;
            if (instr->intrinsic == nir_intrinsic_store_scratch) {
      align = instr->src[0].ssa->num_components;
      } else {
                  nir_def *address = instr->src[address_index].ssa;
               }
      bool
   r600_lower_scratch_addresses(nir_shader *shader)
   {
      bool progress = false;
   nir_foreach_function_impl(impl, shader)
   {
               nir_foreach_block(block, impl)
   {
      nir_foreach_instr(instr, block)
   {
      if (instr->type != nir_instr_type_intrinsic)
         nir_intrinsic_instr *op = nir_instr_as_intrinsic(instr);
   if (op->intrinsic != nir_intrinsic_load_scratch &&
      op->intrinsic != nir_intrinsic_store_scratch)
      r600_nir_lower_scratch_address_impl(&build, op);
            }
      }
      static void
   insert_uniform_sorted(struct exec_list *var_list, nir_variable *new_var)
   {
      nir_foreach_variable_in_list(var, var_list)
   {
      if (var->data.binding > new_var->data.binding ||
      (var->data.binding == new_var->data.binding &&
   var->data.offset > new_var->data.offset)) {
   exec_node_insert_node_before(&var->node, &new_var->node);
         }
      }
      void
   sort_uniforms(nir_shader *shader)
   {
      struct exec_list new_list;
            nir_foreach_uniform_variable_safe(var, shader)
   {
      exec_node_remove(&var->node);
      }
      }
      static void
   insert_fsoutput_sorted(struct exec_list *var_list, nir_variable *new_var)
   {
         nir_foreach_variable_in_list(var, var_list)
   {
      if ((var->data.location >= FRAG_RESULT_DATA0 ||
      var->data.location == FRAG_RESULT_COLOR) &&
   (new_var->data.location < FRAG_RESULT_COLOR ||
   new_var->data.location == FRAG_RESULT_SAMPLE_MASK)) {
   exec_node_insert_after(&var->node, &new_var->node);
      } else if ((new_var->data.location >= FRAG_RESULT_DATA0 ||
                  (var->data.location < FRAG_RESULT_COLOR ||
   exec_node_insert_node_before(&var->node, &new_var->node);
      } else if (var->data.location > new_var->data.location ||
      (var->data.location == new_var->data.location &&
   var->data.index > new_var->data.index)) {
   exec_node_insert_node_before(&var->node, &new_var->node);
                     }
      void
   sort_fsoutput(nir_shader *shader)
   {
      struct exec_list new_list;
            nir_foreach_shader_out_variable_safe(var, shader)
   {
      exec_node_remove(&var->node);
               unsigned driver_location = 0;
   nir_foreach_variable_in_list(var, &new_list) var->data.driver_location =
               }
      class LowerClipvertexWrite : public NirLowerInstruction {
      public:
      LowerClipvertexWrite(int noutputs, pipe_stream_output_info& so_info):
      m_clipplane1(noutputs),
   m_clipvtx(noutputs + 1),
      {
         private:
      bool filter(const nir_instr *instr) const override
   {
      if (instr->type != nir_instr_type_intrinsic)
            auto intr = nir_instr_as_intrinsic(instr);
   if (intr->intrinsic != nir_intrinsic_store_output)
                        nir_def *lower(nir_instr *instr) override
               auto intr = nir_instr_as_intrinsic(instr);
                              for (int i = 0; i < 8; ++i) {
      auto sel = nir_imm_int(b, i);
   auto mrow = nir_load_ubo_vec4(b, 4, 32, buf_id, sel);
                        for (int i = 0; i < 2; ++i) {
      auto clip_i = nir_vec(b, &output[4 * i], 4);
   auto store = nir_store_output(b, clip_i, intr->src[1].ssa);
   nir_intrinsic_set_write_mask(store, 0xf);
   nir_intrinsic_set_base(store, clip_vertex_index);
   nir_io_semantics semantic = nir_intrinsic_io_semantics(intr);
                  if (i > 0)
         nir_intrinsic_set_write_mask(store, 0xf);
      }
            nir_def *result = NIR_LOWER_INSTR_PROGRESS_REPLACE;
   for (unsigned i = 0; i < m_so_info.num_outputs; ++i) {
      if (m_so_info.output[i].register_index == clip_vertex_index) {
      m_so_info.output[i].register_index = m_clipvtx;
         }
      }
   int m_clipplane1;
   int m_clipvtx;
      };
      /* lower_uniforms_to_ubo adds a 1 to the UBO buffer ID.
   * If the buffer ID is a non-constant value we end up
   * with "iadd bufid, 1", bot on r600 we can put that constant
   * "1" as constant cache ID into the CF instruction and don't need
   * to execute that extra ADD op, so eliminate the addition here
   * again and move the buffer base ID into the base value of
   * the intrinsic that is not used otherwise */
   class OptIndirectUBOLoads : public NirLowerInstruction {
   private:
      bool filter(const nir_instr *instr) const override
   {
      if (instr->type != nir_instr_type_intrinsic)
            auto intr = nir_instr_as_intrinsic(instr);
   if (intr->intrinsic != nir_intrinsic_load_ubo_vec4)
            if (nir_src_as_const_value(intr->src[0]) != nullptr)
                        nir_def *lower(nir_instr *instr) override
   {
      auto intr = nir_instr_as_intrinsic(instr);
                     if (parent->type != nir_instr_type_alu)
                     if (alu->op != nir_op_iadd)
            int new_base = 0;
   nir_src *new_bufid = nullptr;
   auto src0 = nir_src_as_const_value(alu->src[0].src);
   if (src0) {
      new_bufid = &alu->src[1].src;
      } else if (auto src1 = nir_src_as_const_value(alu->src[1].src)) {
      new_bufid = &alu->src[0].src;
      } else {
                  nir_intrinsic_set_base(intr, new_base);
   nir_src_rewrite(&intr->src[0], new_bufid->ssa);
         };
      } // namespace r600
      static nir_intrinsic_op
   r600_map_atomic(nir_intrinsic_op op)
   {
      switch (op) {
   case nir_intrinsic_atomic_counter_read_deref:
         case nir_intrinsic_atomic_counter_inc_deref:
         case nir_intrinsic_atomic_counter_pre_dec_deref:
         case nir_intrinsic_atomic_counter_post_dec_deref:
         case nir_intrinsic_atomic_counter_add_deref:
         case nir_intrinsic_atomic_counter_min_deref:
         case nir_intrinsic_atomic_counter_max_deref:
         case nir_intrinsic_atomic_counter_and_deref:
         case nir_intrinsic_atomic_counter_or_deref:
         case nir_intrinsic_atomic_counter_xor_deref:
         case nir_intrinsic_atomic_counter_exchange_deref:
         case nir_intrinsic_atomic_counter_comp_swap_deref:
         default:
            }
      static bool
   r600_lower_deref_instr(nir_builder *b, nir_intrinsic_instr *instr,
         {
      nir_intrinsic_op op = r600_map_atomic(instr->intrinsic);
   if (nir_num_intrinsics == op)
            nir_deref_instr *deref = nir_src_as_deref(instr->src[0]);
            if (var->data.mode != nir_var_uniform && var->data.mode != nir_var_mem_ssbo &&
      var->data.mode != nir_var_mem_shared)
                           nir_def *offset = nir_imm_int(b, 0);
   for (nir_deref_instr *d = deref; d->deref_type != nir_deref_type_var;
      d = nir_deref_instr_parent(d)) {
            unsigned array_stride = 1;
   if (glsl_type_is_array(d->type))
            offset =
               /* Since the first source is a deref and the first source in the lowered
   * instruction is the offset, we can just swap it out and change the
   * opcode.
   */
   instr->intrinsic = op;
   nir_src_rewrite(&instr->src[0], offset);
   nir_intrinsic_set_base(instr, idx);
                        }
      static bool
   r600_lower_clipvertex_to_clipdist(nir_shader *sh, pipe_stream_output_info& so_info)
   {
      if (!(sh->info.outputs_written & VARYING_BIT_CLIP_VERTEX))
            int noutputs = util_bitcount64(sh->info.outputs_written);
   bool result = r600::LowerClipvertexWrite(noutputs, so_info).run(sh);
      }
      static bool
   r600_nir_lower_atomics(nir_shader *shader)
   {
      /* In Hardware we start at a zero index for each new
   * binding, and we use an offset of one per counter. We also
   * need to sort the atomics according to binding and offset. */
   std::map<unsigned, unsigned> binding_offset;
            nir_foreach_variable_with_modes_safe(var, shader, nir_var_uniform) {
      if (var->type->contains_atomic()) {
      sorted_var[(var->data.binding << 16) | var->data.offset] = var;
                  for (auto& [dummy, var] : sorted_var) {
      auto iindex = binding_offset.find(var->data.binding);
   unsigned offset_update = var->type->atomic_size() / 4; /* ATOMIC_COUNTER_SIZE */
   if (iindex == binding_offset.end()) {
      var->data.index = 0;
      } else {
      var->data.index = iindex->second;
      }
               return nir_shader_intrinsics_pass(shader, r600_lower_deref_instr,
            }
   using r600::r600_lower_fs_out_to_vector;
   using r600::r600_lower_scratch_addresses;
   using r600::r600_lower_ubo_to_align16;
      int
   r600_glsl_type_size(const struct glsl_type *type, bool is_bindless)
   {
         }
      void
   r600_get_natural_size_align_bytes(const struct glsl_type *type,
               {
      if (type->base_type != GLSL_TYPE_ARRAY) {
      *align = 1;
      } else {
      unsigned elem_size, elem_align;
   glsl_get_natural_size_align_bytes(type->fields.array, &elem_size, &elem_align);
   *align = 1;
         }
      static bool
   r600_lower_shared_io_impl(nir_function_impl *impl)
   {
               bool progress = false;
   nir_foreach_block(block, impl)
   {
      nir_foreach_instr_safe(instr, block)
                              nir_intrinsic_instr *op = nir_instr_as_intrinsic(instr);
   if (op->intrinsic != nir_intrinsic_load_shared &&
                                             switch (op->def.num_components) {
   case 2: {
      auto addr2 = nir_iadd_imm(&b, addr, 4);
   addr = nir_vec2(&b, addr, addr2);
      }
   case 3: {
      auto addr2 = nir_iadd(&b, addr, nir_imm_ivec2(&b, 4, 8));
   addr =
            }
   case 4: {
      addr = nir_iadd(&b, addr, nir_imm_ivec4(&b, 0, 4, 8, 12));
                     auto load =
         load->num_components = op->def.num_components;
   load->src[0] = nir_src_for_ssa(addr);
   nir_def_init(&load->instr, &load->def, load->num_components,
         nir_def_rewrite_uses(&op->def, &load->def);
      } else {
      nir_def *addr = op->src[1].ssa;
   for (int i = 0; i < 2; ++i) {
                           auto store =
      nir_intrinsic_instr_create(b.shader,
      unsigned writemask = nir_intrinsic_write_mask(op) & test_mask;
   nir_intrinsic_set_write_mask(store, writemask);
                                                   }
   nir_instr_remove(instr);
         }
      }
      static bool
   r600_lower_shared_io(nir_shader *nir)
   {
      bool progress = false;
   nir_foreach_function_impl(impl, nir)
   {
      if (r600_lower_shared_io_impl(impl))
      }
      }
      static nir_def *
   r600_lower_fs_pos_input_impl(nir_builder *b, nir_instr *instr, void *_options)
   {
      (void)_options;
   auto old_ir = nir_instr_as_intrinsic(instr);
   auto load = nir_intrinsic_instr_create(b->shader, nir_intrinsic_load_input);
   nir_def_init(&load->instr, &load->def,
                  nir_intrinsic_set_base(load, nir_intrinsic_base(old_ir));
   nir_intrinsic_set_component(load, nir_intrinsic_component(old_ir));
   nir_intrinsic_set_dest_type(load, nir_type_float32);
   load->num_components = old_ir->num_components;
   load->src[0] = old_ir->src[1];
   nir_builder_instr_insert(b, &load->instr);
      }
      bool
   r600_lower_fs_pos_input_filter(const nir_instr *instr, const void *_options)
   {
               if (instr->type != nir_instr_type_intrinsic)
            auto ir = nir_instr_as_intrinsic(instr);
   if (ir->intrinsic != nir_intrinsic_load_interpolated_input)
               }
      /* Strip the interpolator specification, it is not needed and irritates */
   bool
   r600_lower_fs_pos_input(nir_shader *shader)
   {
      return nir_shader_lower_instructions(shader,
                  };
      bool
   r600_opt_indirect_fbo_loads(nir_shader *shader)
   {
         }
      static bool
   optimize_once(nir_shader *shader)
   {
      bool progress = false;
   NIR_PASS(progress, shader, nir_lower_alu_to_scalar, r600_lower_to_scalar_instr_filter, NULL);
   NIR_PASS(progress, shader, nir_lower_vars_to_ssa);
   NIR_PASS(progress, shader, nir_copy_prop);
   NIR_PASS(progress, shader, nir_opt_dce);
   NIR_PASS(progress, shader, nir_opt_algebraic);
   NIR_PASS(progress, shader, nir_opt_constant_folding);
   NIR_PASS(progress, shader, nir_opt_copy_prop_vars);
            if (nir_opt_trivial_continues(shader)) {
      progress = true;
   NIR_PASS(progress, shader, nir_copy_prop);
               NIR_PASS(progress, shader, nir_opt_if, nir_opt_if_optimize_phi_true_false);
   NIR_PASS(progress, shader, nir_opt_dead_cf);
   NIR_PASS(progress, shader, nir_opt_cse);
            NIR_PASS(progress, shader, nir_opt_conditional_discard);
   NIR_PASS(progress, shader, nir_opt_dce);
   NIR_PASS(progress, shader, nir_opt_undef);
   NIR_PASS(progress, shader, nir_opt_loop_unroll);
      }
      static bool
   r600_is_last_vertex_stage(nir_shader *nir, const r600_shader_key& key)
   {
      if (nir->info.stage == MESA_SHADER_GEOMETRY)
            if (nir->info.stage == MESA_SHADER_TESS_EVAL && !key.tes.as_es)
            if (nir->info.stage == MESA_SHADER_VERTEX && !key.vs.as_es && !key.vs.as_ls)
               }
      extern "C" bool
   r600_lower_to_scalar_instr_filter(const nir_instr *instr, const void *)
   {
      if (instr->type != nir_instr_type_alu)
            auto alu = nir_instr_as_alu(instr);
   switch (alu->op) {
   case nir_op_bany_fnequal3:
   case nir_op_bany_fnequal4:
   case nir_op_ball_fequal3:
   case nir_op_ball_fequal4:
   case nir_op_bany_inequal3:
   case nir_op_bany_inequal4:
   case nir_op_ball_iequal3:
   case nir_op_ball_iequal4:
   case nir_op_fdot2:
   case nir_op_fdot3:
   case nir_op_fdot4:
   case nir_op_fddx:
   case nir_op_fddx_coarse:
   case nir_op_fddx_fine:
   case nir_op_fddy:
   case nir_op_fddy_coarse:
   case nir_op_fddy_fine:
         default:
            }
      class MallocPoolRelease {
   public:
      MallocPoolRelease() { r600::init_pool(); }
      };
      extern "C" char *
   r600_finalize_nir(pipe_screen *screen, void *shader)
   {
                                          nir_lower_idiv_options idiv_options = {0};
            NIR_PASS_V(nir, r600_nir_lower_trigen, rs->b.gfx_level);
   NIR_PASS_V(nir, nir_lower_phis_to_scalar, false);
            struct nir_lower_tex_options lower_tex_options = {0};
   lower_tex_options.lower_txp = ~0u;
   lower_tex_options.lower_txf_offset = true;
   lower_tex_options.lower_invalid_implicit_lod = true;
            NIR_PASS_V(nir, nir_lower_tex, &lower_tex_options);
   NIR_PASS_V(nir, r600_nir_lower_txl_txf_array_or_cube);
                     NIR_PASS_V(nir, r600_lower_shared_io);
            if (rs->b.gfx_level == CAYMAN)
            while (optimize_once(nir))
               }
      int
   r600_shader_from_nir(struct r600_context *rctx,
               {
                           bool lower_64bit =
      (rctx->b.gfx_level < CAYMAN &&
   (sel->nir->options->lower_int64_options ||
   sel->nir->options->lower_doubles_options) &&
         if (rctx->screen->b.debug_flags & DBG_PREOPT_IR) {
      fprintf(stderr, "PRE-OPT-NIR-----------.------------------------------\n");
   nir_print_shader(sel->nir, stderr);
               auto sh = nir_shader_clone(sel->nir, sel->nir);
            while (optimize_once(sh))
               if (sh->info.stage == MESA_SHADER_VERTEX)
            if (sh->info.stage == MESA_SHADER_FRAGMENT) {
      NIR_PASS_V(sh, nir_lower_fragcoord_wtrans);
   NIR_PASS_V(sh, r600_lower_fs_out_to_vector);
   NIR_PASS_V(sh, nir_opt_dce);
   NIR_PASS_V(sh, nir_remove_dead_variables, nir_var_shader_out, 0);
      }
            NIR_PASS_V(sh, nir_opt_combine_stores, nir_var_shader_out);
   NIR_PASS_V(sh,
            nir_lower_io,
   io_modes,
         if (sh->info.stage == MESA_SHADER_FRAGMENT)
            /**/
   if (lower_64bit)
            NIR_PASS_V(sh, nir_opt_constant_folding);
            NIR_PASS_V(sh, nir_lower_alu_to_scalar, r600_lower_to_scalar_instr_filter, NULL);
   NIR_PASS_V(sh, nir_lower_phis_to_scalar, false);
   if (lower_64bit)
         NIR_PASS_V(sh, nir_lower_alu_to_scalar, r600_lower_to_scalar_instr_filter, NULL);
   NIR_PASS_V(sh, nir_lower_phis_to_scalar, false);
   NIR_PASS_V(sh, nir_lower_alu_to_scalar, r600_lower_to_scalar_instr_filter, NULL);
   NIR_PASS_V(sh, nir_copy_prop);
                  if (r600_is_last_vertex_stage(sh, *key))
            if (sh->info.stage == MESA_SHADER_TESS_CTRL ||
      sh->info.stage == MESA_SHADER_TESS_EVAL ||
   (sh->info.stage == MESA_SHADER_VERTEX && key->vs.as_ls)) {
   auto prim_type = sh->info.stage == MESA_SHADER_TESS_EVAL
                           if (sh->info.stage == MESA_SHADER_TESS_CTRL)
            if (sh->info.stage == MESA_SHADER_TESS_EVAL) {
      NIR_PASS_V(sh, nir_lower_tess_coord_z,
               NIR_PASS_V(sh, nir_lower_alu_to_scalar, r600_lower_to_scalar_instr_filter, NULL);
   NIR_PASS_V(sh, nir_lower_phis_to_scalar, false);
   NIR_PASS_V(sh, nir_lower_alu_to_scalar, r600_lower_to_scalar_instr_filter, NULL);
   NIR_PASS_V(sh, r600_nir_lower_int_tg4);
            if ((sh->info.bit_sizes_float | sh->info.bit_sizes_int) & 64) {
      NIR_PASS_V(sh, r600::r600_nir_split_64bit_io);
   NIR_PASS_V(sh, r600::r600_split_64bit_alu_and_phi);
   NIR_PASS_V(sh, nir_split_64bit_vec3_and_vec4);
               NIR_PASS_V(sh, nir_lower_ubo_vec4);
            if (lower_64bit)
            if ((sh->info.bit_sizes_float | sh->info.bit_sizes_int) & 64)
            /* Lower to scalar to let some optimization work out better */
   while (optimize_once(sh))
            if (lower_64bit)
            NIR_PASS_V(sh, nir_remove_dead_variables, nir_var_shader_in, NULL);
            NIR_PASS_V(sh,
            nir_lower_vars_to_scratch,
   nir_var_function_temp,
         while (optimize_once(sh))
            if ((sh->info.bit_sizes_float | sh->info.bit_sizes_int) & 64)
            bool late_algebraic_progress;
   do {
      late_algebraic_progress = false;
   NIR_PASS(late_algebraic_progress, sh, nir_opt_algebraic_late);
   NIR_PASS(late_algebraic_progress, sh, nir_opt_constant_folding);
   NIR_PASS(late_algebraic_progress, sh, nir_copy_prop);
   NIR_PASS(late_algebraic_progress, sh, nir_opt_dce);
                        NIR_PASS_V(sh, nir_lower_locals_to_regs, 32);
   NIR_PASS_V(sh, nir_convert_from_ssa, true);
            if (rctx->screen->b.debug_flags & DBG_ALL_SHADERS) {
      fprintf(stderr,
         struct nir_function *func =
         nir_index_ssa_defs(func->impl);
   nir_print_shader(sh, stderr);
   fprintf(stderr,
               memset(&pipeshader->shader, 0, sizeof(r600_shader));
            if (sh->info.stage == MESA_SHADER_TESS_EVAL || sh->info.stage == MESA_SHADER_VERTEX ||
      sh->info.stage == MESA_SHADER_GEOMETRY) {
   pipeshader->shader.clip_dist_write |=
         pipeshader->shader.cull_dist_write = ((1 << sh->info.cull_distance_array_size) - 1)
         pipeshader->shader.cc_dist_mask =
      (1 << (sh->info.cull_distance_array_size + sh->info.clip_distance_array_size)) -
   }
   struct r600_shader *gs_shader = nullptr;
   if (rctx->gs_shader)
                  r600::Shader *shader =
      r600::Shader::translate_from_nir(sh, &sel->so, gs_shader, *key,
         assert(shader);
   if (!shader)
            pipeshader->enabled_stream_buffers_mask = shader->enabled_stream_buffers_mask();
   pipeshader->selector->info.file_count[TGSI_FILE_HW_ATOMIC] +=
         pipeshader->selector->info.writes_memory =
            if (r600::sfn_log.has_debug_flag(r600::SfnLog::steps)) {
      std::cerr << "Shader after conversion from nir\n";
               if (!r600::sfn_log.has_debug_flag(r600::SfnLog::noopt)) {
               if (r600::sfn_log.has_debug_flag(r600::SfnLog::steps)) {
      std::cerr << "Shader after optimization\n";
                  split_address_loads(*shader);
      if (r600::sfn_log.has_debug_flag(r600::SfnLog::steps)) {
      std::cerr << "Shader after splitting address loads\n";
      }
      if (!r600::sfn_log.has_debug_flag(r600::SfnLog::noopt)) {
               if (r600::sfn_log.has_debug_flag(r600::SfnLog::steps)) {
      std::cerr << "Shader after optimization\n";
                  auto scheduled_shader = r600::schedule(shader);
   if (r600::sfn_log.has_debug_flag(r600::SfnLog::steps)) {
      std::cerr << "Shader after scheduling\n";
                           if (r600::sfn_log.has_debug_flag(r600::SfnLog::merge)) {
      r600::sfn_log << r600::SfnLog::merge << "Shader before RA\n";
               r600::sfn_log << r600::SfnLog::trans << "Merge registers\n";
            if (!r600::register_allocation(lrm)) {
      R600_ERR("%s: Register allocation failed\n", __func__);
   /* For now crash if the shader could not be benerated */
   assert(0);
      } else if (r600::sfn_log.has_debug_flag(r600::SfnLog::merge) ||
            r600::sfn_log << "Shader after RA\n";
                  scheduled_shader->get_shader_info(&pipeshader->shader);
            r600_bytecode_init(&pipeshader->shader.bc,
                        /* We already schedule the code with this in mind, no need to handle this
   * in the backend assembler */
   pipeshader->shader.bc.ar_handling = AR_HANDLE_NORMAL;
            r600::sfn_log << r600::SfnLog::shader_info << "pipeshader->shader.processor_type = "
            pipeshader->shader.bc.type = pipeshader->shader.processor_type;
   pipeshader->shader.bc.isa = rctx->isa;
            r600::Assembler afs(&pipeshader->shader, *key);
   if (!afs.lower(scheduled_shader)) {
               scheduled_shader->print(std::cerr);
   /* For now crash if the shader could not be generated */
   assert(0);
               if (sh->info.stage == MESA_SHADER_VERTEX) {
      pipeshader->shader.vs_position_window_space =
               if (sh->info.stage == MESA_SHADER_FRAGMENT)
      pipeshader->shader.ps_conservative_z =
         if (sh->info.stage == MESA_SHADER_GEOMETRY) {
      r600::sfn_log << r600::SfnLog::shader_info
         generate_gs_copy_shader(rctx, pipeshader, &sel->so);
      } else {
         }
               }
