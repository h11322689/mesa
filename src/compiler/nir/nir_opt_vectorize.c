   /*
   * Copyright © 2015 Connor Abbott
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
   */
      /**
   * nir_opt_vectorize() aims to vectorize ALU instructions.
   *
   * The default vectorization width is 4.
   * If desired, a callback function which returns the max vectorization width
   * per instruction can be provided.
   *
   * The max vectorization width must be a power of 2.
   */
      #include "util/u_dynarray.h"
   #include "nir.h"
   #include "nir_builder.h"
   #include "nir_vla.h"
      #define HASH(hash, data) XXH32(&data, sizeof(data), hash)
      static uint32_t
   hash_src(uint32_t hash, const nir_src *src)
   {
                  }
      static uint32_t
   hash_alu_src(uint32_t hash, const nir_alu_src *src,
         {
      /* hash whether a swizzle accesses elements beyond the maximum
   * vectorization factor:
   * For example accesses to .x and .y are considered different variables
   * compared to accesses to .z and .w for 16-bit vec2.
   */
   uint32_t swizzle = (src->swizzle[0] & ~(max_vec - 1));
               }
      static uint32_t
   hash_instr(const void *data)
   {
      const nir_instr *instr = (nir_instr *)data;
   assert(instr->type == nir_instr_type_alu);
            uint32_t hash = HASH(0, alu->op);
            for (unsigned i = 0; i < nir_op_infos[alu->op].num_inputs; i++)
      hash = hash_alu_src(hash, &alu->src[i],
                  }
      static bool
   srcs_equal(const nir_src *src1, const nir_src *src2)
   {
         return src1->ssa == src2->ssa ||
      }
      static bool
   alu_srcs_equal(const nir_alu_src *src1, const nir_alu_src *src2,
         {
      uint32_t mask = ~(max_vec - 1);
   if ((src1->swizzle[0] & mask) != (src2->swizzle[0] & mask))
               }
      static bool
   instrs_equal(const void *data1, const void *data2)
   {
      const nir_instr *instr1 = (nir_instr *)data1;
   const nir_instr *instr2 = (nir_instr *)data2;
   assert(instr1->type == nir_instr_type_alu);
            nir_alu_instr *alu1 = nir_instr_as_alu(instr1);
            if (alu1->op != alu2->op)
            if (alu1->def.bit_size != alu2->def.bit_size)
            for (unsigned i = 0; i < nir_op_infos[alu1->op].num_inputs; i++) {
      if (!alu_srcs_equal(&alu1->src[i], &alu2->src[i], instr1->pass_flags))
                  }
      static bool
   instr_can_rewrite(nir_instr *instr)
   {
      switch (instr->type) {
   case nir_instr_type_alu: {
               /* Don't try and vectorize mov's. Either they'll be handled by copy
   * prop, or they're actually necessary and trying to vectorize them
   * would result in fighting with copy prop.
   */
   if (alu->op == nir_op_mov)
            /* no need to hash instructions which are already vectorized */
   if (alu->def.num_components >= instr->pass_flags)
            if (nir_op_infos[alu->op].output_size != 0)
            for (unsigned i = 0; i < nir_op_infos[alu->op].num_inputs; i++) {
                     /* don't hash instructions which are already swizzled
   * outside of max_components: these should better be scalarized */
   uint32_t mask = ~(instr->pass_flags - 1);
   for (unsigned j = 1; j < alu->def.num_components; j++) {
      if ((alu->src[i].swizzle[0] & mask) != (alu->src[i].swizzle[j] & mask))
                              /* TODO support phi nodes */
   default:
                     }
      /*
   * Tries to combine two instructions whose sources are different components of
   * the same instructions into one vectorized instruction. Note that instr1
   * should dominate instr2.
   */
   static nir_instr *
   instr_try_combine(struct set *instr_set, nir_instr *instr1, nir_instr *instr2)
   {
      assert(instr1->type == nir_instr_type_alu);
   assert(instr2->type == nir_instr_type_alu);
   nir_alu_instr *alu1 = nir_instr_as_alu(instr1);
            assert(alu1->def.bit_size == alu2->def.bit_size);
   unsigned alu1_components = alu1->def.num_components;
   unsigned alu2_components = alu2->def.num_components;
            assert(instr1->pass_flags == instr2->pass_flags);
   if (total_components > instr1->pass_flags)
                     nir_alu_instr *new_alu = nir_alu_instr_create(b.shader, alu1->op);
   nir_def_init(&new_alu->instr, &new_alu->def, total_components,
                  /* If either channel is exact, we have to preserve it even if it's
   * not optimal for other channels.
   */
            /* If all channels don't wrap, we can say that the whole vector doesn't
   * wrap.
   */
   new_alu->no_signed_wrap = alu1->no_signed_wrap && alu2->no_signed_wrap;
            for (unsigned i = 0; i < nir_op_infos[alu1->op].num_inputs; i++) {
      /* handle constant merging case */
   if (alu1->src[i].src.ssa != alu2->src[i].src.ssa) {
      nir_const_value *c1 = nir_src_as_const_value(alu1->src[i].src);
   nir_const_value *c2 = nir_src_as_const_value(alu2->src[i].src);
   assert(c1 && c2);
                  for (unsigned j = 0; j < total_components; j++) {
                        new_alu->src[i].src = nir_src_for_ssa(def);
   for (unsigned j = 0; j < total_components; j++)
                              for (unsigned j = 0; j < alu1_components; j++)
            for (unsigned j = 0; j < alu2_components; j++) {
      new_alu->src[i].swizzle[j + alu1_components] =
                           /* update all ALU uses */
   nir_foreach_use_safe(src, &alu1->def) {
      nir_instr *user_instr = nir_src_parent_instr(src);
   if (user_instr->type == nir_instr_type_alu) {
                     /* For ALU instructions, rewrite the source directly to avoid a
   * round-trip through copy propagation.
                  /* Rehash user if it was found in the hashset */
   if (entry && entry->key == user_instr) {
      _mesa_set_remove(instr_set, entry);
                     nir_foreach_use_safe(src, &alu2->def) {
      if (nir_src_parent_instr(src)->type == nir_instr_type_alu) {
      /* For ALU instructions, rewrite the source directly to avoid a
   * round-trip through copy propagation.
                  nir_alu_src *alu_src = container_of(src, nir_alu_src, src);
   nir_alu_instr *use = nir_instr_as_alu(nir_src_parent_instr(src));
   unsigned components = nir_ssa_alu_instr_src_components(use, alu_src - use->src);
   for (unsigned i = 0; i < components; i++)
                  /* update all other uses if there are any */
            if (!nir_def_is_unused(&alu1->def)) {
      for (unsigned i = 0; i < alu1_components; i++)
         nir_def *new_alu1 = nir_swizzle(&b, &new_alu->def, swiz,
                     if (!nir_def_is_unused(&alu2->def)) {
      for (unsigned i = 0; i < alu2_components; i++)
         nir_def *new_alu2 = nir_swizzle(&b, &new_alu->def, swiz,
                     nir_instr_remove(instr1);
               }
      static struct set *
   vec_instr_set_create(void)
   {
         }
      static void
   vec_instr_set_destroy(struct set *instr_set)
   {
         }
      static bool
   vec_instr_set_add_or_rewrite(struct set *instr_set, nir_instr *instr,
         {
      /* set max vector to instr pass flags: this is used to hash swizzles */
   instr->pass_flags = filter ? filter(instr, data) : 4;
            if (!instr_can_rewrite(instr))
            struct set_entry *entry = _mesa_set_search(instr_set, instr);
   if (entry) {
      nir_instr *old_instr = (nir_instr *)entry->key;
   _mesa_set_remove(instr_set, entry);
   nir_instr *new_instr = instr_try_combine(instr_set, old_instr, instr);
   if (new_instr) {
      if (instr_can_rewrite(new_instr))
                        _mesa_set_add(instr_set, instr);
      }
      static bool
   vectorize_block(nir_block *block, struct set *instr_set,
         {
               nir_foreach_instr_safe(instr, block) {
      if (vec_instr_set_add_or_rewrite(instr_set, instr, filter, data))
               for (unsigned i = 0; i < block->num_dom_children; i++) {
      nir_block *child = block->dom_children[i];
               nir_foreach_instr_reverse(instr, block) {
      if (instr_can_rewrite(instr))
                  }
      static bool
   nir_opt_vectorize_impl(nir_function_impl *impl,
         {
                        bool progress = vectorize_block(nir_start_block(impl), instr_set,
            if (progress) {
      nir_metadata_preserve(impl, nir_metadata_block_index |
      } else {
                  vec_instr_set_destroy(instr_set);
      }
      bool
   nir_opt_vectorize(nir_shader *shader, nir_vectorize_cb filter,
         {
               nir_foreach_function_impl(impl, shader) {
                     }
