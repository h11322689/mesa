   /*
   * Copyright © 2014 Connor Abbott
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
      #include "nir_instr_set.h"
   #include "util/half_float.h"
   #include "nir_vla.h"
      /* This function determines if uses of an instruction can safely be rewritten
   * to use another identical instruction instead. Note that this function must
   * be kept in sync with hash_instr() and nir_instrs_equal() -- only
   * instructions that pass this test will be handed on to those functions, and
   * conversely they must handle everything that this function returns true for.
   */
   static bool
   instr_can_rewrite(const nir_instr *instr)
   {
      switch (instr->type) {
   case nir_instr_type_alu:
   case nir_instr_type_deref:
   case nir_instr_type_tex:
   case nir_instr_type_load_const:
   case nir_instr_type_phi:
         case nir_instr_type_intrinsic:
         case nir_instr_type_call:
   case nir_instr_type_jump:
   case nir_instr_type_undef:
         case nir_instr_type_parallel_copy:
   default:
                     }
      #define HASH(hash, data) XXH32(&(data), sizeof(data), hash)
      static uint32_t
   hash_src(uint32_t hash, const nir_src *src)
   {
      hash = HASH(hash, src->ssa);
      }
      static uint32_t
   hash_alu_src(uint32_t hash, const nir_alu_src *src, unsigned num_components)
   {
      for (unsigned i = 0; i < num_components; i++)
            hash = hash_src(hash, &src->src);
      }
      static uint32_t
   hash_alu(uint32_t hash, const nir_alu_instr *instr)
   {
               /* We explicitly don't hash instr->exact. */
   uint8_t flags = instr->no_signed_wrap |
                  hash = HASH(hash, instr->def.num_components);
            if (nir_op_infos[instr->op].algebraic_properties & NIR_OP_IS_2SRC_COMMUTATIVE) {
               uint32_t hash0 = hash_alu_src(hash, &instr->src[0],
         uint32_t hash1 = hash_alu_src(hash, &instr->src[1],
         /* For commutative operations, we need some commutative way of
   * combining the hashes.  One option would be to XOR them but that
   * means that anything with two identical sources will hash to 0 and
   * that's common enough we probably don't want the guaranteed
   * collision.  Either addition or multiplication will also work.
   */
            for (unsigned i = 2; i < nir_op_infos[instr->op].num_inputs; i++) {
      hash = hash_alu_src(hash, &instr->src[i],
         } else {
      for (unsigned i = 0; i < nir_op_infos[instr->op].num_inputs; i++) {
      hash = hash_alu_src(hash, &instr->src[i],
                     }
      static uint32_t
   hash_deref(uint32_t hash, const nir_deref_instr *instr)
   {
      hash = HASH(hash, instr->deref_type);
   hash = HASH(hash, instr->modes);
            if (instr->deref_type == nir_deref_type_var)
                     switch (instr->deref_type) {
   case nir_deref_type_struct:
      hash = HASH(hash, instr->strct.index);
         case nir_deref_type_array:
   case nir_deref_type_ptr_as_array:
      hash = hash_src(hash, &instr->arr.index);
   hash = HASH(hash, instr->arr.in_bounds);
         case nir_deref_type_cast:
      hash = HASH(hash, instr->cast.ptr_stride);
   hash = HASH(hash, instr->cast.align_mul);
   hash = HASH(hash, instr->cast.align_offset);
         case nir_deref_type_var:
   case nir_deref_type_array_wildcard:
      /* Nothing to do */
         default:
                     }
      static uint32_t
   hash_load_const(uint32_t hash, const nir_load_const_instr *instr)
   {
               if (instr->def.bit_size == 1) {
      for (unsigned i = 0; i < instr->def.num_components; i++) {
      uint8_t b = instr->value[i].b;
         } else {
      unsigned size = instr->def.num_components * sizeof(*instr->value);
                  }
      static int
   cmp_phi_src(const void *data1, const void *data2)
   {
      nir_phi_src *src1 = *(nir_phi_src **)data1;
   nir_phi_src *src2 = *(nir_phi_src **)data2;
      }
      static uint32_t
   hash_phi(uint32_t hash, const nir_phi_instr *instr)
   {
               /* sort sources by predecessor, since the order shouldn't matter */
   unsigned num_preds = instr->instr.block->predecessors->entries;
   NIR_VLA(nir_phi_src *, srcs, num_preds);
   unsigned i = 0;
   nir_foreach_phi_src(src, instr) {
                           for (i = 0; i < num_preds; i++) {
      hash = hash_src(hash, &srcs[i]->src);
                  }
      static uint32_t
   hash_intrinsic(uint32_t hash, const nir_intrinsic_instr *instr)
   {
      const nir_intrinsic_info *info = &nir_intrinsic_infos[instr->intrinsic];
            if (info->has_dest) {
      hash = HASH(hash, instr->def.num_components);
                        for (unsigned i = 0; i < nir_intrinsic_infos[instr->intrinsic].num_srcs; i++)
               }
      static uint32_t
   hash_tex(uint32_t hash, const nir_tex_instr *instr)
   {
      hash = HASH(hash, instr->op);
            for (unsigned i = 0; i < instr->num_srcs; i++) {
      hash = HASH(hash, instr->src[i].src_type);
               hash = HASH(hash, instr->coord_components);
   hash = HASH(hash, instr->sampler_dim);
   hash = HASH(hash, instr->is_array);
   hash = HASH(hash, instr->is_shadow);
   hash = HASH(hash, instr->is_new_style_shadow);
   hash = HASH(hash, instr->is_sparse);
   unsigned component = instr->component;
   hash = HASH(hash, component);
   for (unsigned i = 0; i < 4; ++i)
      for (unsigned j = 0; j < 2; ++j)
      hash = HASH(hash, instr->texture_index);
   hash = HASH(hash, instr->sampler_index);
   hash = HASH(hash, instr->texture_non_uniform);
   hash = HASH(hash, instr->sampler_non_uniform);
               }
      /* Computes a hash of an instruction for use in a hash table. Note that this
   * will only work for instructions where instr_can_rewrite() returns true, and
   * it should return identical hashes for two instructions that are the same
   * according nir_instrs_equal().
   */
      static uint32_t
   hash_instr(const void *data)
   {
      const nir_instr *instr = data;
            switch (instr->type) {
   case nir_instr_type_alu:
      hash = hash_alu(hash, nir_instr_as_alu(instr));
      case nir_instr_type_deref:
      hash = hash_deref(hash, nir_instr_as_deref(instr));
      case nir_instr_type_load_const:
      hash = hash_load_const(hash, nir_instr_as_load_const(instr));
      case nir_instr_type_phi:
      hash = hash_phi(hash, nir_instr_as_phi(instr));
      case nir_instr_type_intrinsic:
      hash = hash_intrinsic(hash, nir_instr_as_intrinsic(instr));
      case nir_instr_type_tex:
      hash = hash_tex(hash, nir_instr_as_tex(instr));
      default:
                     }
      bool
   nir_srcs_equal(nir_src src1, nir_src src2)
   {
         }
      /**
   * If the \p s is an SSA value that was generated by a negation instruction,
   * that instruction is returned as a \c nir_alu_instr.  Otherwise \c NULL is
   * returned.
   */
   static nir_alu_instr *
   get_neg_instr(nir_src s)
   {
               return alu != NULL && (alu->op == nir_op_fneg || alu->op == nir_op_ineg)
            }
      bool
   nir_const_value_negative_equal(nir_const_value c1,
               {
      assert(nir_alu_type_get_base_type(full_type) != nir_type_invalid);
            switch (full_type) {
   case nir_type_float16:
            case nir_type_float32:
            case nir_type_float64:
            case nir_type_int8:
   case nir_type_uint8:
            case nir_type_int16:
   case nir_type_uint16:
            case nir_type_int32:
   case nir_type_uint32:
            case nir_type_int64:
   case nir_type_uint64:
            default:
                     }
      /**
   * Shallow compare of ALU srcs to determine if one is the negation of the other
   *
   * This function detects cases where \p alu1 is a constant and \p alu2 is a
   * constant that is its negation.  It will also detect cases where \p alu2 is
   * an SSA value that is a \c nir_op_fneg applied to \p alu1 (and vice versa).
   *
   * This function does not detect the general case when \p alu1 and \p alu2 are
   * SSA values that are the negations of each other (e.g., \p alu1 represents
   * (a * b) and \p alu2 represents (-a * b)).
   *
   * \warning
   * It is the responsibility of the caller to ensure that the component counts,
   * write masks, and base types of the sources being compared are compatible.
   */
   bool
   nir_alu_srcs_negative_equal(const nir_alu_instr *alu1,
               {
   #ifndef NDEBUG
      for (unsigned i = 0; i < NIR_MAX_VEC_COMPONENTS; i++) {
      assert(nir_alu_instr_channel_used(alu1, src1, i) ==
               if (nir_alu_type_get_base_type(nir_op_infos[alu1->op].input_types[src1]) == nir_type_float) {
      assert(nir_op_infos[alu1->op].input_types[src1] ==
      } else {
      assert(nir_op_infos[alu1->op].input_types[src1] == nir_type_int);
         #endif
                           const nir_const_value *const const1 =
            if (const1 != NULL) {
      /* Assume that constant folding will eliminate source mods and unary
   * ops.
   */
   if (parity)
            const nir_const_value *const const2 =
            if (const2 == NULL)
            if (nir_src_bit_size(alu1->src[src1].src) !=
                  const nir_alu_type full_type = nir_op_infos[alu1->op].input_types[src1] |
         for (unsigned i = 0; i < NIR_MAX_VEC_COMPONENTS; i++) {
      if (nir_alu_instr_channel_used(alu1, src1, i) &&
      !nir_const_value_negative_equal(const1[alu1->src[src1].swizzle[i]],
                                    uint8_t alu1_swizzle[NIR_MAX_VEC_COMPONENTS] = { 0 };
   nir_src alu1_actual_src;
            if (neg1) {
      parity = !parity;
            for (unsigned i = 0; i < nir_ssa_alu_instr_src_components(neg1, 0); i++)
      } else {
               for (unsigned i = 0; i < nir_src_num_components(alu1_actual_src); i++)
               uint8_t alu2_swizzle[NIR_MAX_VEC_COMPONENTS] = { 0 };
   nir_src alu2_actual_src;
            if (neg2) {
      parity = !parity;
            for (unsigned i = 0; i < nir_ssa_alu_instr_src_components(neg2, 0); i++)
      } else {
               for (unsigned i = 0; i < nir_src_num_components(alu2_actual_src); i++)
               for (unsigned i = 0; i < nir_ssa_alu_instr_src_components(alu1, src1); i++) {
      if (alu1_swizzle[alu1->src[src1].swizzle[i]] !=
      alu2_swizzle[alu2->src[src2].swizzle[i]])
               }
      bool
   nir_alu_srcs_equal(const nir_alu_instr *alu1, const nir_alu_instr *alu2,
         {
      for (unsigned i = 0; i < nir_ssa_alu_instr_src_components(alu1, src1); i++) {
      if (alu1->src[src1].swizzle[i] != alu2->src[src2].swizzle[i])
                  }
      /* Returns "true" if two instructions are equal. Note that this will only
   * work for the subset of instructions defined by instr_can_rewrite(). Also,
   * it should only return "true" for instructions that hash_instr() will return
   * the same hash for (ignoring collisions, of course).
   */
      bool
   nir_instrs_equal(const nir_instr *instr1, const nir_instr *instr2)
   {
               if (instr1->type != instr2->type)
            switch (instr1->type) {
   case nir_instr_type_alu: {
      nir_alu_instr *alu1 = nir_instr_as_alu(instr1);
            if (alu1->op != alu2->op)
                     if (alu1->no_signed_wrap != alu2->no_signed_wrap)
            if (alu1->no_unsigned_wrap != alu2->no_unsigned_wrap)
            /* TODO: We can probably acutally do something more inteligent such
   * as allowing different numbers and taking a maximum or something
   * here */
   if (alu1->def.num_components != alu2->def.num_components)
            if (alu1->def.bit_size != alu2->def.bit_size)
            if (nir_op_infos[alu1->op].algebraic_properties & NIR_OP_IS_2SRC_COMMUTATIVE) {
      if ((!nir_alu_srcs_equal(alu1, alu2, 0, 0) ||
      !nir_alu_srcs_equal(alu1, alu2, 1, 1)) &&
   (!nir_alu_srcs_equal(alu1, alu2, 0, 1) ||
               for (unsigned i = 2; i < nir_op_infos[alu1->op].num_inputs; i++) {
      if (!nir_alu_srcs_equal(alu1, alu2, i, i))
         } else {
      for (unsigned i = 0; i < nir_op_infos[alu1->op].num_inputs; i++) {
      if (!nir_alu_srcs_equal(alu1, alu2, i, i))
         }
      }
   case nir_instr_type_deref: {
      nir_deref_instr *deref1 = nir_instr_as_deref(instr1);
            if (deref1->deref_type != deref2->deref_type ||
      deref1->modes != deref2->modes ||
               if (deref1->deref_type == nir_deref_type_var)
            if (!nir_srcs_equal(deref1->parent, deref2->parent))
            switch (deref1->deref_type) {
   case nir_deref_type_struct:
      if (deref1->strct.index != deref2->strct.index)
               case nir_deref_type_array:
   case nir_deref_type_ptr_as_array:
      if (!nir_srcs_equal(deref1->arr.index, deref2->arr.index))
         if (deref1->arr.in_bounds != deref2->arr.in_bounds)
               case nir_deref_type_cast:
      if (deref1->cast.ptr_stride != deref2->cast.ptr_stride ||
      deref1->cast.align_mul != deref2->cast.align_mul ||
   deref1->cast.align_offset != deref2->cast.align_offset)
            case nir_deref_type_var:
   case nir_deref_type_array_wildcard:
                  default:
         }
      }
   case nir_instr_type_tex: {
      nir_tex_instr *tex1 = nir_instr_as_tex(instr1);
            if (tex1->op != tex2->op)
            if (tex1->num_srcs != tex2->num_srcs)
         for (unsigned i = 0; i < tex1->num_srcs; i++) {
      if (tex1->src[i].src_type != tex2->src[i].src_type ||
      !nir_srcs_equal(tex1->src[i].src, tex2->src[i].src)) {
                  if (tex1->coord_components != tex2->coord_components ||
      tex1->sampler_dim != tex2->sampler_dim ||
   tex1->is_array != tex2->is_array ||
   tex1->is_shadow != tex2->is_shadow ||
   tex1->is_new_style_shadow != tex2->is_new_style_shadow ||
   tex1->component != tex2->component ||
   tex1->texture_index != tex2->texture_index ||
   tex1->sampler_index != tex2->sampler_index ||
   tex1->backend_flags != tex2->backend_flags) {
               if (memcmp(tex1->tg4_offsets, tex2->tg4_offsets,
                     }
   case nir_instr_type_load_const: {
      nir_load_const_instr *load1 = nir_instr_as_load_const(instr1);
            if (load1->def.num_components != load2->def.num_components)
            if (load1->def.bit_size != load2->def.bit_size)
            if (load1->def.bit_size == 1) {
      for (unsigned i = 0; i < load1->def.num_components; ++i) {
      if (load1->value[i].b != load2->value[i].b)
         } else {
      unsigned size = load1->def.num_components * sizeof(*load1->value);
   if (memcmp(load1->value, load2->value, size) != 0)
      }
      }
   case nir_instr_type_phi: {
      nir_phi_instr *phi1 = nir_instr_as_phi(instr1);
            if (phi1->instr.block != phi2->instr.block)
            /* In case of phis with no sources, the dest needs to be checked
   * to ensure that phis with incompatible dests won't get merged
   * during CSE. */
   if (phi1->def.num_components != phi2->def.num_components)
         if (phi1->def.bit_size != phi2->def.bit_size)
            nir_foreach_phi_src(src1, phi1) {
      nir_foreach_phi_src(src2, phi2) {
      if (src1->pred == src2->pred) {
                                          }
   case nir_instr_type_intrinsic: {
      nir_intrinsic_instr *intrinsic1 = nir_instr_as_intrinsic(instr1);
   nir_intrinsic_instr *intrinsic2 = nir_instr_as_intrinsic(instr2);
   const nir_intrinsic_info *info =
            if (intrinsic1->intrinsic != intrinsic2->intrinsic ||
                  if (info->has_dest && intrinsic1->def.num_components !=
                  if (info->has_dest && intrinsic1->def.bit_size !=
                  for (unsigned i = 0; i < info->num_srcs; i++) {
      if (!nir_srcs_equal(intrinsic1->src[i], intrinsic2->src[i]))
               for (unsigned i = 0; i < info->num_indices; i++) {
      if (intrinsic1->const_index[i] != intrinsic2->const_index[i])
                  }
   case nir_instr_type_call:
   case nir_instr_type_jump:
   case nir_instr_type_undef:
   case nir_instr_type_parallel_copy:
   default:
                     }
      static nir_def *
   nir_instr_get_def_def(nir_instr *instr)
   {
      switch (instr->type) {
   case nir_instr_type_alu:
         case nir_instr_type_deref:
         case nir_instr_type_load_const:
         case nir_instr_type_phi:
         case nir_instr_type_intrinsic:
         case nir_instr_type_tex:
         default:
            }
      static bool
   cmp_func(const void *data1, const void *data2)
   {
         }
      struct set *
   nir_instr_set_create(void *mem_ctx)
   {
         }
      void
   nir_instr_set_destroy(struct set *instr_set)
   {
         }
      bool
   nir_instr_set_add_or_rewrite(struct set *instr_set, nir_instr *instr,
               {
      if (!instr_can_rewrite(instr))
            struct set_entry *e = _mesa_set_search_or_add(instr_set, instr, NULL);
   nir_instr *match = (nir_instr *)e->key;
   if (match == instr)
            if (!cond_function || cond_function(match, instr)) {
      /* rewrite instruction if condition is matched */
   nir_def *def = nir_instr_get_def_def(instr);
            /* It's safe to replace an exact instruction with an inexact one as
   * long as we make it exact.  If we got here, the two instructions are
   * exactly identical in every other way so, once we've set the exact
   * bit, they are the same.
   */
   if (instr->type == nir_instr_type_alu && nir_instr_as_alu(instr)->exact)
                                 } else {
      /* otherwise, replace hashed instruction */
   e->key = instr;
         }
      void
   nir_instr_set_remove(struct set *instr_set, nir_instr *instr)
   {
      if (!instr_can_rewrite(instr))
            struct set_entry *entry = _mesa_set_search(instr_set, instr);
   if (entry)
      }
