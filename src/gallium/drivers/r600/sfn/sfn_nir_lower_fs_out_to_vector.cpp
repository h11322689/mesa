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
      #include "sfn_nir_lower_fs_out_to_vector.h"
      #include "nir_builder.h"
   #include "nir_deref.h"
   #include "util/u_math.h"
      #include <algorithm>
   #include <array>
   #include <set>
   #include <vector>
      namespace r600 {
      using std::array;
   using std::multiset;
   using std::vector;
      struct nir_intrinsic_instr_less {
      bool operator()(const nir_intrinsic_instr *lhs, const nir_intrinsic_instr *rhs) const
   {
      nir_variable *vlhs = nir_intrinsic_get_var(lhs, 0);
            auto ltype = glsl_get_base_type(vlhs->type);
            if (ltype != rtype)
               };
      class NirLowerIOToVector {
   public:
      NirLowerIOToVector(int base_slot);
         protected:
      bool var_can_merge(const nir_variable *lhs, const nir_variable *rhs);
   bool var_can_rewrite(nir_variable *var) const;
   void create_new_io_vars(nir_shader *shader);
            nir_deref_instr *clone_deref_array(nir_builder *b,
                  bool vectorize_block(nir_builder *b, nir_block *block);
   bool instr_can_rewrite(nir_instr *instr);
            using InstrSet = multiset<nir_intrinsic_instr *, nir_intrinsic_instr_less>;
            bool
            array<array<nir_variable *, 4>, 16> m_vars;
   InstrSet m_block_io;
         private:
      virtual nir_variable_mode get_io_mode(nir_shader *shader) const = 0;
   virtual bool instr_can_rewrite_type(nir_intrinsic_instr *intr) const = 0;
   virtual bool var_can_rewrite_slot(nir_variable *var) const = 0;
   virtual void create_new_io(nir_builder *b,
                                       };
      class NirLowerFSOutToVector : public NirLowerIOToVector {
   public:
            private:
      nir_variable_mode get_io_mode(nir_shader *shader) const override;
   bool var_can_rewrite_slot(nir_variable *var) const override;
   void create_new_io(nir_builder *b,
                     nir_intrinsic_instr *intr,
   nir_variable *var,
            nir_def *create_combined_vector(nir_builder *b,
                  };
      bool
   r600_lower_fs_out_to_vector(nir_shader *shader)
   {
               assert(shader->info.stage == MESA_SHADER_FRAGMENT);
            nir_foreach_function_impl(impl, shader) {
         }
      }
      NirLowerIOToVector::NirLowerIOToVector(int base_slot):
      m_next_index(0),
      {
      for (auto& a : m_vars)
      for (auto& aa : a)
   }
      bool
   NirLowerIOToVector::run(nir_function_impl *impl)
   {
               nir_metadata_require(impl, nir_metadata_dominance);
            bool progress = vectorize_block(&b, nir_start_block(impl));
   if (progress) {
         } else {
         }
      }
      void
   NirLowerIOToVector::create_new_io_vars(nir_shader *shader)
   {
               bool can_rewrite_vars = false;
   nir_foreach_variable_with_modes(var, shader, mode)
   {
      if (var_can_rewrite(var)) {
      can_rewrite_vars = true;
   unsigned loc = var->data.location - m_base_slot;
                  if (!can_rewrite_vars)
            /* We don't handle combining vars of different type e.g. different array
   * lengths.
   */
   for (unsigned i = 0; i < 16; i++) {
               for (unsigned j = 0; j < 3; j++) {
                     for (unsigned k = j + 1; k < 4; k++) {
                                    /* Set comps */
                  for (unsigned n = 0; n < glsl_get_components(m_vars[i][k]->type); ++n)
         }
   if (comps)
         }
      bool
   NirLowerIOToVector::var_can_merge(const nir_variable *lhs, const nir_variable *rhs)
   {
         }
      void
   NirLowerIOToVector::create_new_io_var(nir_shader *shader,
               {
      unsigned num_comps = util_bitcount(comps);
            /* Note: u_bit_scan() strips a component of the comps bitfield here */
            nir_variable *var = nir_variable_clone(m_vars[location][first_comp], shader);
   var->data.location_frac = first_comp;
                              while (comps) {
      const int comp = u_bit_scan(&comps);
   if (m_vars[location][comp]) {
               }
      bool
   NirLowerIOToVector::var_can_rewrite(nir_variable *var) const
   {
      /* Skip complex types we don't split in the first place */
   if (!glsl_type_is_vector_or_scalar(glsl_without_array(var->type)))
            if (glsl_get_bit_size(glsl_without_array(var->type)) != 32)
               }
      bool
   NirLowerIOToVector::vectorize_block(nir_builder *b, nir_block *block)
   {
               nir_foreach_instr_safe(instr, block)
   {
      if (instr_can_rewrite(instr)) {
      instr->index = m_next_index++;
   nir_intrinsic_instr *ir = nir_instr_as_intrinsic(instr);
                  for (unsigned i = 0; i < block->num_dom_children; i++) {
      nir_block *child = block->dom_children[i];
               nir_foreach_instr_reverse_safe(instr, block)
   {
         }
               }
      bool
   NirLowerIOToVector::instr_can_rewrite(nir_instr *instr)
   {
      if (instr->type != nir_instr_type_intrinsic)
                     if (intr->num_components > 3)
               }
      bool
   NirLowerIOToVector::vec_instr_set_remove(nir_builder *b, nir_instr *instr)
   {
      if (!instr_can_rewrite(instr))
            nir_intrinsic_instr *ir = nir_instr_as_intrinsic(instr);
   auto entry = m_block_io.equal_range(ir);
   if (entry.first != m_block_io.end()) {
         }
      }
      nir_deref_instr *
   NirLowerIOToVector::clone_deref_array(nir_builder *b,
               {
               if (!parent)
                                 }
      NirLowerFSOutToVector::NirLowerFSOutToVector():
         {
   }
      bool
   NirLowerFSOutToVector::var_can_rewrite_slot(nir_variable *var) const
   {
      return ((var->data.mode == nir_var_shader_out) &&
         ((var->data.location == FRAG_RESULT_COLOR) ||
      }
      bool
   NirLowerIOToVector::vec_instr_stack_pop(nir_builder *b,
               {
      vector<nir_intrinsic_instr *> ir_sorted_set(ir_set.first, ir_set.second);
   std::sort(ir_sorted_set.begin(),
            ir_sorted_set.end(),
   [](const nir_intrinsic_instr *lhs, const nir_intrinsic_instr *rhs) {
         nir_intrinsic_instr *intr = *ir_sorted_set.begin();
                     nir_variable *new_var = m_vars[loc][var->data.location_frac];
   unsigned num_comps = glsl_get_vector_elements(glsl_without_array(new_var->type));
            /* Don't bother walking the stack if this component can't be vectorised. */
   if (old_num_comps > 3) {
                  if (new_var == var) {
                  b->cursor = nir_after_instr(&intr->instr);
   nir_undef_instr *instr_undef = nir_undef_instr_create(b->shader, 1, 32);
            nir_def *srcs[4];
   for (int i = 0; i < 4; i++) {
         }
            for (auto k = ir_sorted_set.begin() + 1; k != ir_sorted_set.end(); ++k) {
      nir_intrinsic_instr *intr2 = *k;
   nir_variable *var2 = nir_intrinsic_get_var(intr2, 0);
            if (m_vars[loc][var->data.location_frac] !=
      m_vars[loc2][var2->data.location_frac]) {
                        if (srcs[var2->data.location_frac] == &instr_undef->def) {
      assert(intr2->src[1].ssa);
      }
               create_new_io(b, intr, new_var, srcs, new_var->data.location_frac, num_comps);
      }
      nir_variable_mode
   NirLowerFSOutToVector::get_io_mode(nir_shader *shader) const
   {
         }
      void
   NirLowerFSOutToVector::create_new_io(nir_builder *b,
                                 {
               nir_intrinsic_instr *new_intr = nir_intrinsic_instr_create(b->shader, intr->intrinsic);
                     nir_deref_instr *deref = nir_build_deref_var(b, var);
            new_intr->src[0] = nir_src_for_ssa(&deref->def);
   new_intr->src[1] =
                     /* Remove the old store intrinsic */
      }
      bool
   NirLowerFSOutToVector::instr_can_rewrite_type(nir_intrinsic_instr *intr) const
   {
      if (intr->intrinsic != nir_intrinsic_store_deref)
            nir_deref_instr *deref = nir_src_as_deref(intr->src[0]);
   if (!nir_deref_mode_is(deref, nir_var_shader_out))
               }
      nir_def *
   NirLowerFSOutToVector::create_combined_vector(nir_builder *b,
                     {
      nir_op op;
   switch (num_comp) {
   case 2:
      op = nir_op_vec2;
      case 3:
      op = nir_op_vec3;
      case 4:
      op = nir_op_vec4;
      default:
         }
   nir_alu_instr *instr = nir_alu_instr_create(b->shader, op);
            int i = 0;
   unsigned k = 0;
   while (i < num_comp) {
      nir_def *s = srcs[first_comp + k];
   for (uint8_t kk = 0; kk < s->num_components && i < num_comp; ++kk) {
      instr->src[i].src = nir_src_for_ssa(s);
   instr->src[i].swizzle[0] = kk;
      }
               nir_def_init(&instr->instr, &instr->def, num_comp, 32);
   nir_builder_instr_insert(b, &instr->instr);
      }
      } // namespace r600
