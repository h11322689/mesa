   /*
   * Copyright © 2019 Valve Corporation
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
      /**
   * Although it's called a load/store "vectorization" pass, this also combines
   * intersecting and identical loads/stores. It currently supports derefs, ubo,
   * ssbo and push constant loads/stores.
   *
   * This doesn't handle copy_deref intrinsics and assumes that
   * nir_lower_alu_to_scalar() has been called and that the IR is free from ALU
   * modifiers. It also assumes that derefs have explicitly laid out types.
   *
   * After vectorization, the backend may want to call nir_lower_alu_to_scalar()
   * and nir_lower_pack(). Also this creates cast instructions taking derefs as a
   * source and some parts of NIR may not be able to handle that well.
   *
   * There are a few situations where this doesn't vectorize as well as it could:
   * - It won't turn four consecutive vec3 loads into 3 vec4 loads.
   * - It doesn't do global vectorization.
   * Handling these cases probably wouldn't provide much benefit though.
   *
   * This probably doesn't handle big-endian GPUs correctly.
   */
      #include "util/u_dynarray.h"
   #include "nir.h"
   #include "nir_builder.h"
   #include "nir_deref.h"
   #include "nir_worklist.h"
      #include <stdlib.h>
      struct intrinsic_info {
      nir_variable_mode mode; /* 0 if the mode is obtained from the deref. */
   nir_intrinsic_op op;
   bool is_atomic;
   /* Indices into nir_intrinsic::src[] or -1 if not applicable. */
   int resource_src; /* resource (e.g. from vulkan_resource_index) */
   int base_src;     /* offset which it loads/stores from */
   int deref_src;    /* deref which is loads/stores from */
      };
      static const struct intrinsic_info *
   get_info(nir_intrinsic_op op)
   {
         #define INFO(mode, op, atomic, res, base, deref, val)                                                             \
      case nir_intrinsic_##op: {                                                                                     \
      static const struct intrinsic_info op##_info = { mode, nir_intrinsic_##op, atomic, res, base, deref, val }; \
         #define LOAD(mode, op, res, base, deref)       INFO(mode, load_##op, false, res, base, deref, -1)
   #define STORE(mode, op, res, base, deref, val) INFO(mode, store_##op, false, res, base, deref, val)
   #define ATOMIC(mode, type, res, base, deref, val)         \
      INFO(mode, type##_atomic, true, res, base, deref, val) \
               LOAD(nir_var_mem_push_const, push_constant, -1, 0, -1)
   LOAD(nir_var_mem_ubo, ubo, 0, 1, -1)
   LOAD(nir_var_mem_ssbo, ssbo, 0, 1, -1)
   STORE(nir_var_mem_ssbo, ssbo, 1, 2, -1, 0)
   LOAD(0, deref, -1, -1, 0)
   STORE(0, deref, -1, -1, 0, 1)
   LOAD(nir_var_mem_shared, shared, -1, 0, -1)
   STORE(nir_var_mem_shared, shared, -1, 1, -1, 0)
   LOAD(nir_var_mem_global, global, -1, 0, -1)
   STORE(nir_var_mem_global, global, -1, 1, -1, 0)
   LOAD(nir_var_mem_global, global_constant, -1, 0, -1)
   LOAD(nir_var_mem_task_payload, task_payload, -1, 0, -1)
   STORE(nir_var_mem_task_payload, task_payload, -1, 1, -1, 0)
   ATOMIC(nir_var_mem_ssbo, ssbo, 0, 1, -1, 2)
   ATOMIC(0, deref, -1, -1, 0, 1)
   ATOMIC(nir_var_mem_shared, shared, -1, 0, -1, 1)
   ATOMIC(nir_var_mem_global, global, -1, 0, -1, 1)
   ATOMIC(nir_var_mem_task_payload, task_payload, -1, 0, -1, 1)
   LOAD(nir_var_shader_temp, stack, -1, -1, -1)
   STORE(nir_var_shader_temp, stack, -1, -1, -1, 0)
   LOAD(nir_var_shader_temp, scratch, -1, 0, -1)
   STORE(nir_var_shader_temp, scratch, -1, 1, -1, 0)
   LOAD(nir_var_mem_ubo, ubo_uniform_block_intel, 0, 1, -1)
   LOAD(nir_var_mem_ssbo, ssbo_uniform_block_intel, 0, 1, -1)
   LOAD(nir_var_mem_shared, shared_uniform_block_intel, -1, 0, -1)
      default:
      #undef ATOMIC
   #undef STORE
   #undef LOAD
   #undef INFO
      }
      }
      /*
   * Information used to compare memory operations.
   * It canonically represents an offset as:
   * `offset_defs[0]*offset_defs_mul[0] + offset_defs[1]*offset_defs_mul[1] + ...`
   * "offset_defs" is sorted in ascenting order by the ssa definition's index.
   * "resource" or "var" may be NULL.
   */
   struct entry_key {
      nir_def *resource;
   nir_variable *var;
   unsigned offset_def_count;
   nir_scalar *offset_defs;
      };
      /* Information on a single memory operation. */
   struct entry {
      struct list_head head;
            struct entry_key *key;
   union {
      uint64_t offset; /* sign-extended */
      };
   uint32_t align_mul;
            nir_instr *instr;
   nir_intrinsic_instr *intrin;
   const struct intrinsic_info *info;
   enum gl_access_qualifier access;
               };
      struct vectorize_ctx {
      nir_shader *shader;
   const nir_load_store_vectorize_options *options;
   struct list_head entries[nir_num_variable_modes];
   struct hash_table *loads[nir_num_variable_modes];
      };
      static uint32_t
   hash_entry_key(const void *key_)
   {
      /* this is careful to not include pointers in the hash calculation so that
   * the order of the hash table walk is deterministic */
            uint32_t hash = 0;
   if (key->resource)
         if (key->var) {
      hash = XXH32(&key->var->index, sizeof(key->var->index), hash);
   unsigned mode = key->var->data.mode;
               for (unsigned i = 0; i < key->offset_def_count; i++) {
      hash = XXH32(&key->offset_defs[i].def->index, sizeof(key->offset_defs[i].def->index), hash);
                           }
      static bool
   entry_key_equals(const void *a_, const void *b_)
   {
      struct entry_key *a = (struct entry_key *)a_;
            if (a->var != b->var || a->resource != b->resource)
            if (a->offset_def_count != b->offset_def_count)
            for (unsigned i = 0; i < a->offset_def_count; i++) {
      if (!nir_scalar_equal(a->offset_defs[i], b->offset_defs[i]))
               size_t offset_def_mul_size = a->offset_def_count * sizeof(uint64_t);
   if (a->offset_def_count &&
      memcmp(a->offset_defs_mul, b->offset_defs_mul, offset_def_mul_size))
            }
      static void
   delete_entry_dynarray(struct hash_entry *entry)
   {
      struct util_dynarray *arr = (struct util_dynarray *)entry->data;
      }
      static int
   sort_entries(const void *a_, const void *b_)
   {
      struct entry *a = *(struct entry *const *)a_;
            if (a->offset_signed > b->offset_signed)
         else if (a->offset_signed < b->offset_signed)
         else
      }
      static unsigned
   get_bit_size(struct entry *entry)
   {
      unsigned size = entry->is_store ? entry->intrin->src[entry->info->value_src].ssa->bit_size : entry->intrin->def.bit_size;
      }
      /* If "def" is from an alu instruction with the opcode "op" and one of it's
   * sources is a constant, update "def" to be the non-constant source, fill "c"
   * with the constant and return true. */
   static bool
   parse_alu(nir_scalar *def, nir_op op, uint64_t *c)
   {
      if (!nir_scalar_is_alu(*def) || nir_scalar_alu_op(*def) != op)
            nir_scalar src0 = nir_scalar_chase_alu_src(*def, 0);
   nir_scalar src1 = nir_scalar_chase_alu_src(*def, 1);
   if (op != nir_op_ishl && nir_scalar_is_const(src0)) {
      *c = nir_scalar_as_uint(src0);
      } else if (nir_scalar_is_const(src1)) {
      *c = nir_scalar_as_uint(src1);
      } else {
         }
      }
      /* Parses an offset expression such as "a * 16 + 4" and "(a * 16 + 4) * 64 + 32". */
   static void
   parse_offset(nir_scalar *base, uint64_t *base_mul, uint64_t *offset)
   {
      if (nir_scalar_is_const(*base)) {
      *offset = nir_scalar_as_uint(*base);
   base->def = NULL;
               uint64_t mul = 1;
   uint64_t add = 0;
   bool progress = false;
   do {
               progress = parse_alu(base, nir_op_imul, &mul2);
            mul2 = 0;
   progress |= parse_alu(base, nir_op_ishl, &mul2);
            progress |= parse_alu(base, nir_op_iadd, &add2);
            if (nir_scalar_is_alu(*base) && nir_scalar_alu_op(*base) == nir_op_mov) {
      *base = nir_scalar_chase_alu_src(*base, 0);
                  if (base->def->parent_instr->type == nir_instr_type_intrinsic) {
      nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(base->def->parent_instr);
   if (intrin->intrinsic == nir_intrinsic_load_vulkan_descriptor)
               *base_mul = mul;
      }
      static unsigned
   type_scalar_size_bytes(const struct glsl_type *type)
   {
      assert(glsl_type_is_vector_or_scalar(type) ||
            }
      static unsigned
   add_to_entry_key(nir_scalar *offset_defs, uint64_t *offset_defs_mul,
         {
               for (unsigned i = 0; i <= offset_def_count; i++) {
      if (i == offset_def_count || def.def->index > offset_defs[i].def->index) {
      /* insert before i */
   memmove(offset_defs + i + 1, offset_defs + i,
         memmove(offset_defs_mul + i + 1, offset_defs_mul + i,
         offset_defs[i] = def;
   offset_defs_mul[i] = mul;
      } else if (nir_scalar_equal(def, offset_defs[i])) {
      /* merge with offset_def at i */
   offset_defs_mul[i] += mul;
         }
   unreachable("Unreachable.");
      }
      static struct entry_key *
   create_entry_key_from_deref(void *mem_ctx,
                     {
      unsigned path_len = 0;
   while (path->path[path_len])
            nir_scalar offset_defs_stack[32];
   uint64_t offset_defs_mul_stack[32];
   nir_scalar *offset_defs = offset_defs_stack;
   uint64_t *offset_defs_mul = offset_defs_mul_stack;
   if (path_len > 32) {
      offset_defs = malloc(path_len * sizeof(nir_scalar));
      }
            struct entry_key *key = ralloc(mem_ctx, struct entry_key);
   key->resource = NULL;
   key->var = NULL;
            for (unsigned i = 0; i < path_len; i++) {
      nir_deref_instr *parent = i ? path->path[i - 1] : NULL;
            switch (deref->deref_type) {
   case nir_deref_type_var: {
      assert(!parent);
   key->var = deref->var;
      }
   case nir_deref_type_array:
   case nir_deref_type_ptr_as_array: {
      assert(parent);
                  nir_scalar base = { .def = index, .comp = 0 };
   uint64_t offset = 0, base_mul = 1;
                  *offset_base += offset * stride;
   if (base.def) {
      offset_def_count += add_to_entry_key(offset_defs, offset_defs_mul,
            }
      }
   case nir_deref_type_struct: {
      assert(parent);
   int offset = glsl_get_struct_field_offset(parent->type, deref->strct.index);
   *offset_base += offset;
      }
   case nir_deref_type_cast: {
      if (!parent)
            }
   default:
                     key->offset_def_count = offset_def_count;
   key->offset_defs = ralloc_array(mem_ctx, nir_scalar, offset_def_count);
   key->offset_defs_mul = ralloc_array(mem_ctx, uint64_t, offset_def_count);
   memcpy(key->offset_defs, offset_defs, offset_def_count * sizeof(nir_scalar));
            if (offset_defs != offset_defs_stack)
         if (offset_defs_mul != offset_defs_mul_stack)
               }
      static unsigned
   parse_entry_key_from_offset(struct entry_key *key, unsigned size, unsigned left,
         {
      uint64_t new_mul;
   uint64_t new_offset;
   parse_offset(&base, &new_mul, &new_offset);
            if (!base.def)
                              if (left >= 2) {
      if (nir_scalar_is_alu(base) && nir_scalar_alu_op(base) == nir_op_iadd) {
      nir_scalar src0 = nir_scalar_chase_alu_src(base, 0);
   nir_scalar src1 = nir_scalar_chase_alu_src(base, 1);
   unsigned amount = parse_entry_key_from_offset(key, size, left - 1, src0, base_mul, offset);
   amount += parse_entry_key_from_offset(key, size + amount, left - amount, src1, base_mul, offset);
                     }
      static struct entry_key *
   create_entry_key_from_offset(void *mem_ctx, nir_def *base, uint64_t base_mul, uint64_t *offset)
   {
      struct entry_key *key = ralloc(mem_ctx, struct entry_key);
   key->resource = NULL;
   key->var = NULL;
   if (base) {
      nir_scalar offset_defs[32];
   uint64_t offset_defs_mul[32];
   key->offset_defs = offset_defs;
            nir_scalar scalar = { .def = base, .comp = 0 };
            key->offset_defs = ralloc_array(mem_ctx, nir_scalar, key->offset_def_count);
   key->offset_defs_mul = ralloc_array(mem_ctx, uint64_t, key->offset_def_count);
   memcpy(key->offset_defs, offset_defs, key->offset_def_count * sizeof(nir_scalar));
      } else {
      key->offset_def_count = 0;
   key->offset_defs = NULL;
      }
      }
      static nir_variable_mode
   get_variable_mode(struct entry *entry)
   {
      if (entry->info->mode)
         assert(entry->deref && util_bitcount(entry->deref->modes) == 1);
      }
      static unsigned
   mode_to_index(nir_variable_mode mode)
   {
               /* Globals and SSBOs should be tracked together */
   if (mode == nir_var_mem_global)
               }
      static nir_variable_mode
   aliasing_modes(nir_variable_mode modes)
   {
      /* Global and SSBO can alias */
   if (modes & (nir_var_mem_ssbo | nir_var_mem_global))
            }
      static void
   calc_alignment(struct entry *entry)
   {
      uint32_t align_mul = 31;
   for (unsigned i = 0; i < entry->key->offset_def_count; i++) {
      if (entry->key->offset_defs_mul[i])
               entry->align_mul = 1u << (align_mul - 1);
   bool has_align = nir_intrinsic_infos[entry->intrin->intrinsic].index_map[NIR_INTRINSIC_ALIGN_MUL];
   if (!has_align || entry->align_mul >= nir_intrinsic_align_mul(entry->intrin)) {
         } else {
      entry->align_mul = nir_intrinsic_align_mul(entry->intrin);
         }
      static struct entry *
   create_entry(struct vectorize_ctx *ctx,
               {
      struct entry *entry = rzalloc(ctx, struct entry);
   entry->intrin = intrin;
   entry->instr = &intrin->instr;
   entry->info = info;
            if (entry->info->deref_src >= 0) {
      entry->deref = nir_src_as_deref(intrin->src[entry->info->deref_src]);
   nir_deref_path path;
   nir_deref_path_init(&path, entry->deref, NULL);
   entry->key = create_entry_key_from_deref(entry, ctx, &path, &entry->offset);
      } else {
      nir_def *base = entry->info->base_src >= 0 ? intrin->src[entry->info->base_src].ssa : NULL;
   uint64_t offset = 0;
   if (nir_intrinsic_has_base(intrin))
         entry->key = create_entry_key_from_offset(entry, base, 1, &offset);
            if (base)
               if (entry->info->resource_src >= 0)
            if (nir_intrinsic_has_access(intrin))
         else if (entry->key->var)
            if (nir_intrinsic_can_reorder(intrin))
            uint32_t restrict_modes = nir_var_shader_in | nir_var_shader_out;
   restrict_modes |= nir_var_shader_temp | nir_var_function_temp;
   restrict_modes |= nir_var_uniform | nir_var_mem_push_const;
   restrict_modes |= nir_var_system_value | nir_var_mem_shared;
   restrict_modes |= nir_var_mem_task_payload;
   if (get_variable_mode(entry) & restrict_modes)
                        }
      static nir_deref_instr *
   cast_deref(nir_builder *b, unsigned num_components, unsigned bit_size, nir_deref_instr *deref)
   {
      if (glsl_get_components(deref->type) == num_components &&
      type_scalar_size_bytes(deref->type) * 8u == bit_size)
         enum glsl_base_type types[] = {
         };
   enum glsl_base_type base = types[ffs(bit_size / 8u) - 1u];
            if (deref->type == type)
               }
      /* Return true if "new_bit_size" is a usable bit size for a vectorized load/store
   * of "low" and "high". */
   static bool
   new_bitsize_acceptable(struct vectorize_ctx *ctx, unsigned new_bit_size,
         {
      if (size % new_bit_size != 0)
            unsigned new_num_components = size / new_bit_size;
   if (!nir_num_components_valid(new_num_components))
                     /* check nir_extract_bits limitations */
   unsigned common_bit_size = MIN2(get_bit_size(low), get_bit_size(high));
   common_bit_size = MIN2(common_bit_size, new_bit_size);
   if (high_offset > 0)
         if (new_bit_size / common_bit_size > NIR_MAX_VEC_COMPONENTS)
            if (!ctx->options->callback(low->align_mul,
                                    if (low->is_store) {
      unsigned low_size = low->intrin->num_components * get_bit_size(low);
            if (low_size % new_bit_size != 0)
         if (high_size % new_bit_size != 0)
            unsigned write_mask = nir_intrinsic_write_mask(low->intrin);
   if (!nir_component_mask_can_reinterpret(write_mask, get_bit_size(low), new_bit_size))
            write_mask = nir_intrinsic_write_mask(high->intrin);
   if (!nir_component_mask_can_reinterpret(write_mask, get_bit_size(high), new_bit_size))
                  }
      static nir_deref_instr *
   subtract_deref(nir_builder *b, nir_deref_instr *deref, int64_t offset)
   {
      /* avoid adding another deref to the path */
   if (deref->deref_type == nir_deref_type_ptr_as_array &&
      nir_src_is_const(deref->arr.index) &&
   offset % nir_deref_instr_array_stride(deref) == 0) {
   unsigned stride = nir_deref_instr_array_stride(deref);
   nir_def *index = nir_imm_intN_t(b, nir_src_as_int(deref->arr.index) - offset / stride,
                     if (deref->deref_type == nir_deref_type_array &&
      nir_src_is_const(deref->arr.index)) {
   nir_deref_instr *parent = nir_deref_instr_parent(deref);
   unsigned stride = glsl_get_explicit_stride(parent->type);
   if (offset % stride == 0)
      return nir_build_deref_array_imm(
            deref = nir_build_deref_cast(b, &deref->def, deref->modes,
         return nir_build_deref_ptr_as_array(
      }
      static void
   vectorize_loads(nir_builder *b, struct vectorize_ctx *ctx,
                  struct entry *low, struct entry *high,
      {
      unsigned low_bit_size = get_bit_size(low);
   unsigned high_bit_size = get_bit_size(high);
   bool low_bool = low->intrin->def.bit_size == 1;
   bool high_bool = high->intrin->def.bit_size == 1;
                     /* update the load's destination size and extract data for each of the original loads */
   data->num_components = new_num_components;
            nir_def *low_def = nir_extract_bits(
         nir_def *high_def = nir_extract_bits(
            /* convert booleans */
   low_def = low_bool ? nir_i2b(b, low_def) : nir_mov(b, low_def);
            /* update uses */
   if (first == low) {
      nir_def_rewrite_uses_after(&low->intrin->def, low_def,
            } else {
      nir_def_rewrite_uses(&low->intrin->def, low_def);
   nir_def_rewrite_uses_after(&high->intrin->def, high_def,
               /* update the intrinsic */
                     /* update the offset */
   if (first != low && info->base_src >= 0) {
      /* let nir_opt_algebraic() remove this addition. this doesn't have much
   * issues with subtracting 16 from expressions like "(i + 1) * 16" because
   * nir_opt_algebraic() turns them into "i * 16 + 16" */
            nir_def *new_base = first->intrin->src[info->base_src].ssa;
                        /* update the deref */
   if (info->deref_src >= 0) {
               nir_deref_instr *deref = nir_src_as_deref(first->intrin->src[info->deref_src]);
   if (first != low && high_start != 0)
                  nir_src_rewrite(&first->intrin->src[info->deref_src],
               /* update align */
   if (nir_intrinsic_has_range_base(first->intrin)) {
      uint32_t low_base = nir_intrinsic_range_base(low->intrin);
   uint32_t high_base = nir_intrinsic_range_base(high->intrin);
   uint32_t low_end = low_base + nir_intrinsic_range(low->intrin);
            nir_intrinsic_set_range_base(first->intrin, low_base);
      } else if (nir_intrinsic_has_base(first->intrin) && info->base_src == -1 && info->deref_src == -1) {
                  first->key = low->key;
            first->align_mul = low->align_mul;
               }
      static void
   vectorize_stores(nir_builder *b, struct vectorize_ctx *ctx,
                  struct entry *low, struct entry *high,
      {
      ASSERTED unsigned low_size = low->intrin->num_components * get_bit_size(low);
                     /* get new writemasks */
   uint32_t low_write_mask = nir_intrinsic_write_mask(low->intrin);
   uint32_t high_write_mask = nir_intrinsic_write_mask(high->intrin);
   low_write_mask = nir_component_mask_reinterpret(low_write_mask,
               high_write_mask = nir_component_mask_reinterpret(high_write_mask,
                                 /* convert booleans */
   nir_def *low_val = low->intrin->src[low->info->value_src].ssa;
   nir_def *high_val = high->intrin->src[high->info->value_src].ssa;
   low_val = low_val->bit_size == 1 ? nir_b2iN(b, low_val, 32) : low_val;
            /* combine the data */
   nir_def *data_channels[NIR_MAX_VEC_COMPONENTS];
   for (unsigned i = 0; i < new_num_components; i++) {
      bool set_low = low_write_mask & (1 << i);
            if (set_low && (!set_high || low == second)) {
      unsigned offset = i * new_bit_size;
      } else if (set_high) {
      assert(!set_low || high == second);
   unsigned offset = i * new_bit_size - high_start;
      } else {
            }
            /* update the intrinsic */
   nir_intrinsic_set_write_mask(second->intrin, write_mask);
            const struct intrinsic_info *info = second->info;
   assert(info->value_src >= 0);
            /* update the offset */
   if (second != low && info->base_src >= 0)
      nir_src_rewrite(&second->intrin->src[info->base_src],
         /* update the deref */
   if (info->deref_src >= 0) {
      b->cursor = nir_before_instr(second->instr);
   second->deref = cast_deref(b, new_num_components, new_bit_size,
         nir_src_rewrite(&second->intrin->src[info->deref_src],
               /* update base/align */
   if (second != low && nir_intrinsic_has_base(second->intrin))
            second->key = low->key;
            second->align_mul = low->align_mul;
            list_del(&first->head);
      }
      /* Returns true if it can prove that "a" and "b" point to different bindings
   * and either one uses ACCESS_RESTRICT. */
   static bool
   bindings_different_restrict(nir_shader *shader, struct entry *a, struct entry *b)
   {
      bool different_bindings = false;
   nir_variable *a_var = NULL, *b_var = NULL;
   if (a->key->resource && b->key->resource) {
      nir_binding a_res = nir_chase_binding(nir_src_for_ssa(a->key->resource));
   nir_binding b_res = nir_chase_binding(nir_src_for_ssa(b->key->resource));
   if (!a_res.success || !b_res.success)
            if (a_res.num_indices != b_res.num_indices ||
      a_res.desc_set != b_res.desc_set ||
               for (unsigned i = 0; i < a_res.num_indices; i++) {
      if (nir_src_is_const(a_res.indices[i]) && nir_src_is_const(b_res.indices[i]) &&
      nir_src_as_uint(a_res.indices[i]) != nir_src_as_uint(b_res.indices[i]))
            if (different_bindings) {
      a_var = nir_get_binding_variable(shader, a_res);
         } else if (a->key->var && b->key->var) {
      a_var = a->key->var;
   b_var = b->key->var;
      } else if (!!a->key->resource != !!b->key->resource) {
      /* comparing global and ssbo access */
            if (a->key->resource) {
      nir_binding a_res = nir_chase_binding(nir_src_for_ssa(a->key->resource));
               if (b->key->resource) {
      nir_binding b_res = nir_chase_binding(nir_src_for_ssa(b->key->resource));
         } else {
                  unsigned a_access = a->access | (a_var ? a_var->data.access : 0);
            return different_bindings &&
      }
      static int64_t
   compare_entries(struct entry *a, struct entry *b)
   {
      if (!entry_key_equals(a->key, b->key))
            }
      static bool
   may_alias(nir_shader *shader, struct entry *a, struct entry *b)
   {
      assert(mode_to_index(get_variable_mode(a)) ==
            if ((a->access | b->access) & ACCESS_CAN_REORDER)
            /* if the resources/variables are definitively different and both have
   * ACCESS_RESTRICT, we can assume they do not alias. */
   if (bindings_different_restrict(shader, a, b))
            /* we can't compare offsets if the resources/variables might be different */
   if (a->key->var != b->key->var || a->key->resource != b->key->resource)
            /* use adjacency information */
   /* TODO: we can look closer at the entry keys */
   int64_t diff = compare_entries(a, b);
   if (diff != INT64_MAX) {
      /* with atomics, intrin->num_components can be 0 */
   if (diff < 0)
         else
                           }
      static bool
   check_for_aliasing(struct vectorize_ctx *ctx, struct entry *first, struct entry *second)
   {
      nir_variable_mode mode = get_variable_mode(first);
   if (mode & (nir_var_uniform | nir_var_system_value |
                  unsigned mode_index = mode_to_index(mode);
   if (first->is_store) {
      /* find first entry that aliases "first" */
   list_for_each_entry_from(struct entry, next, first, &ctx->entries[mode_index], head) {
      if (next == first)
         if (next == second)
         if (may_alias(ctx->shader, first, next))
         } else {
      /* find previous store that aliases this load */
   list_for_each_entry_from_rev(struct entry, prev, second, &ctx->entries[mode_index], head) {
      if (prev == second)
         if (prev == first)
         if (prev->is_store && may_alias(ctx->shader, second, prev))
                     }
      static uint64_t
   calc_gcd(uint64_t a, uint64_t b)
   {
      while (b != 0) {
      int tmp_a = a;
   a = b;
      }
      }
      static uint64_t
   round_down(uint64_t a, uint64_t b)
   {
         }
      static bool
   addition_wraps(uint64_t a, uint64_t b, unsigned bits)
   {
      uint64_t mask = BITFIELD64_MASK(bits);
      }
      /* Return true if the addition of "low"'s offset and "high_offset" could wrap
   * around.
   *
   * This is to prevent a situation where the hardware considers the high load
   * out-of-bounds after vectorization if the low load is out-of-bounds, even if
   * the wrap-around from the addition could make the high load in-bounds.
   */
   static bool
   check_for_robustness(struct vectorize_ctx *ctx, struct entry *low, uint64_t high_offset)
   {
      nir_variable_mode mode = get_variable_mode(low);
   if (!(mode & ctx->options->robust_modes))
            /* First, try to use alignment information in case the application provided some. If the addition
   * of the maximum offset of the low load and "high_offset" wraps around, we can't combine the low
   * and high loads.
   */
   uint64_t max_low = round_down(UINT64_MAX, low->align_mul) + low->align_offset;
   if (!addition_wraps(max_low, high_offset, 64))
            /* We can't obtain addition_bits */
   if (low->info->base_src < 0)
            /* Second, use information about the factors from address calculation (offset_defs_mul). These
   * are not guaranteed to be power-of-2.
   */
   uint64_t stride = 0;
   for (unsigned i = 0; i < low->key->offset_def_count; i++)
            unsigned addition_bits = low->intrin->src[low->info->base_src].ssa->bit_size;
   /* low's offset must be a multiple of "stride" plus "low->offset". */
   max_low = low->offset;
   if (stride)
            }
      static bool
   is_strided_vector(const struct glsl_type *type)
   {
      if (glsl_type_is_vector(type)) {
      unsigned explicit_stride = glsl_get_explicit_stride(type);
   return explicit_stride != 0 && explicit_stride !=
      } else {
            }
      static bool
   can_vectorize(struct vectorize_ctx *ctx, struct entry *first, struct entry *second)
   {
      if (!(get_variable_mode(first) & ctx->options->modes) ||
      !(get_variable_mode(second) & ctx->options->modes))
         if (check_for_aliasing(ctx, first, second))
            /* we can only vectorize non-volatile loads/stores of the same type and with
   * the same access */
   if (first->info != second->info || first->access != second->access ||
      (first->access & ACCESS_VOLATILE) || first->info->is_atomic)
            }
      static bool
   try_vectorize(nir_function_impl *impl, struct vectorize_ctx *ctx,
               {
      if (!can_vectorize(ctx, first, second))
            uint64_t diff = high->offset_signed - low->offset_signed;
   if (check_for_robustness(ctx, low, diff))
            /* don't attempt to vectorize accesses of row-major matrix columns */
   if (first->deref) {
      const struct glsl_type *first_type = first->deref->type;
   const struct glsl_type *second_type = second->deref->type;
   if (is_strided_vector(first_type) || is_strided_vector(second_type))
               /* gather information */
   unsigned low_bit_size = get_bit_size(low);
   unsigned high_bit_size = get_bit_size(high);
   unsigned low_size = low->intrin->num_components * low_bit_size;
   unsigned high_size = high->intrin->num_components * high_bit_size;
            /* find a good bit size for the new load/store */
   unsigned new_bit_size = 0;
   if (new_bitsize_acceptable(ctx, low_bit_size, low, high, new_size)) {
         } else if (low_bit_size != high_bit_size &&
               } else {
      new_bit_size = 64;
   for (; new_bit_size >= 8; new_bit_size /= 2) {
      /* don't repeat trying out bitsizes */
   if (new_bit_size == low_bit_size || new_bit_size == high_bit_size)
         if (new_bitsize_acceptable(ctx, new_bit_size, low, high, new_size))
      }
   if (new_bit_size < 8)
      }
            /* vectorize the loads/stores */
            if (first->is_store)
      vectorize_stores(&b, ctx, low, high, first, second,
      else
      vectorize_loads(&b, ctx, low, high, first, second,
            }
      static bool
   try_vectorize_shared2(struct vectorize_ctx *ctx,
               {
      if (!can_vectorize(ctx, first, second) || first->deref)
            unsigned low_bit_size = get_bit_size(low);
   unsigned high_bit_size = get_bit_size(high);
   unsigned low_size = low->intrin->num_components * low_bit_size / 8;
   unsigned high_size = high->intrin->num_components * high_bit_size / 8;
   if ((low_size != 4 && low_size != 8) || (high_size != 4 && high_size != 8))
         if (low_size != high_size)
         if (low->align_mul % low_size || low->align_offset % low_size)
         if (high->align_mul % low_size || high->align_offset % low_size)
            uint64_t diff = high->offset_signed - low->offset_signed;
   bool st64 = diff % (64 * low_size) == 0;
   unsigned stride = st64 ? 64 * low_size : low_size;
   if (diff % stride || diff > 255 * stride)
            /* try to avoid creating accesses we can't combine additions/offsets into */
   if (high->offset > 255 * stride || (st64 && high->offset % stride))
            if (first->is_store) {
      if (nir_intrinsic_write_mask(low->intrin) != BITFIELD_MASK(low->intrin->num_components))
         if (nir_intrinsic_write_mask(high->intrin) != BITFIELD_MASK(high->intrin->num_components))
               /* vectorize the accesses */
            nir_def *offset = first->intrin->src[first->is_store].ssa;
   offset = nir_iadd_imm(&b, offset, nir_intrinsic_base(first->intrin));
   if (first != low)
            if (first->is_store) {
      nir_def *low_val = low->intrin->src[low->info->value_src].ssa;
   nir_def *high_val = high->intrin->src[high->info->value_src].ssa;
   nir_def *val = nir_vec2(&b, nir_bitcast_vector(&b, low_val, low_size * 8u),
            } else {
      nir_def *new_def = nir_load_shared2_amd(&b, low_size * 8u, offset, .offset1 = diff / stride,
         nir_def_rewrite_uses(&low->intrin->def,
         nir_def_rewrite_uses(&high->intrin->def,
               nir_instr_remove(first->instr);
               }
      static bool
   update_align(struct entry *entry)
   {
      if (nir_intrinsic_has_align_mul(entry->intrin) &&
      (entry->align_mul != nir_intrinsic_align_mul(entry->intrin) ||
   entry->align_offset != nir_intrinsic_align_offset(entry->intrin))) {
   nir_intrinsic_set_align(entry->intrin, entry->align_mul, entry->align_offset);
      }
      }
      static bool
   vectorize_sorted_entries(struct vectorize_ctx *ctx, nir_function_impl *impl,
         {
               bool progress = false;
   for (unsigned first_idx = 0; first_idx < num_entries; first_idx++) {
      struct entry *low = *util_dynarray_element(arr, struct entry *, first_idx);
   if (!low)
            for (unsigned second_idx = first_idx + 1; second_idx < num_entries; second_idx++) {
      struct entry *high = *util_dynarray_element(arr, struct entry *, second_idx);
                                 uint64_t diff = high->offset_signed - low->offset_signed;
   bool separate = diff > get_bit_size(low) / 8u * low->intrin->num_components;
   if (separate) {
      if (!ctx->options->has_shared2_amd ||
                  if (try_vectorize_shared2(ctx, low, high, first, second)) {
      low = NULL;
   *util_dynarray_element(arr, struct entry *, second_idx) = NULL;
   progress = true;
         } else {
      if (try_vectorize(impl, ctx, low, high, first, second)) {
      low = low->is_store ? second : first;
   *util_dynarray_element(arr, struct entry *, second_idx) = NULL;
                                    }
      static bool
   vectorize_entries(struct vectorize_ctx *ctx, nir_function_impl *impl, struct hash_table *ht)
   {
      if (!ht)
            bool progress = false;
   hash_table_foreach(ht, entry) {
      struct util_dynarray *arr = entry->data;
   if (!arr->size)
            qsort(util_dynarray_begin(arr),
                  while (vectorize_sorted_entries(ctx, impl, arr))
            util_dynarray_foreach(arr, struct entry *, elem) {
      if (*elem)
                              }
      static bool
   handle_barrier(struct vectorize_ctx *ctx, bool *progress, nir_function_impl *impl, nir_instr *instr)
   {
      unsigned modes = 0;
   bool acquire = true;
   bool release = true;
   if (instr->type == nir_instr_type_intrinsic) {
      nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   switch (intrin->intrinsic) {
   /* prevent speculative loads/stores */
   case nir_intrinsic_discard_if:
   case nir_intrinsic_discard:
   case nir_intrinsic_terminate_if:
   case nir_intrinsic_terminate:
   case nir_intrinsic_launch_mesh_workgroups:
      modes = nir_var_all;
      case nir_intrinsic_demote_if:
   case nir_intrinsic_demote:
      acquire = false;
   modes = nir_var_all;
      case nir_intrinsic_barrier:
                     modes = nir_intrinsic_memory_modes(intrin) & (nir_var_mem_ssbo |
                     acquire = nir_intrinsic_memory_semantics(intrin) & NIR_MEMORY_ACQUIRE;
   release = nir_intrinsic_memory_semantics(intrin) & NIR_MEMORY_RELEASE;
   switch (nir_intrinsic_memory_scope(intrin)) {
   case SCOPE_INVOCATION:
      /* a barier should never be required for correctness with these scopes */
   modes = 0;
      default:
         }
      default:
            } else if (instr->type == nir_instr_type_call) {
         } else {
                  while (modes) {
      unsigned mode_index = u_bit_scan(&modes);
   if ((1 << mode_index) == nir_var_mem_global) {
      /* Global should be rolled in with SSBO */
   assert(list_is_empty(&ctx->entries[mode_index]));
   assert(ctx->loads[mode_index] == NULL);
   assert(ctx->stores[mode_index] == NULL);
               if (acquire)
         if (release)
                  }
      static bool
   process_block(nir_function_impl *impl, struct vectorize_ctx *ctx, nir_block *block)
   {
               for (unsigned i = 0; i < nir_num_variable_modes; i++) {
      list_inithead(&ctx->entries[i]);
   if (ctx->loads[i])
         if (ctx->stores[i])
               /* create entries */
            nir_foreach_instr_safe(instr, block) {
      if (handle_barrier(ctx, &progress, impl, instr))
            /* gather information */
   if (instr->type != nir_instr_type_intrinsic)
                  const struct intrinsic_info *info = get_info(intrin->intrinsic);
   if (!info)
            nir_variable_mode mode = info->mode;
   if (!mode)
         if (!(mode & aliasing_modes(ctx->options->modes)))
                  /* create entry */
   struct entry *entry = create_entry(ctx, info, intrin);
                              struct hash_table *adj_ht = NULL;
   if (entry->is_store) {
      if (!ctx->stores[mode_index])
            } else {
      if (!ctx->loads[mode_index])
                     uint32_t key_hash = hash_entry_key(entry->key);
   struct hash_entry *adj_entry = _mesa_hash_table_search_pre_hashed(adj_ht, key_hash, entry->key);
   struct util_dynarray *arr;
   if (adj_entry && adj_entry->data) {
         } else {
      arr = ralloc(ctx, struct util_dynarray);
   util_dynarray_init(arr, arr);
      }
               /* sort and combine entries */
   for (unsigned i = 0; i < nir_num_variable_modes; i++) {
      progress |= vectorize_entries(ctx, impl, ctx->loads[i]);
                  }
      bool
   nir_opt_load_store_vectorize(nir_shader *shader, const nir_load_store_vectorize_options *options)
   {
               struct vectorize_ctx *ctx = rzalloc(NULL, struct vectorize_ctx);
   ctx->shader = shader;
                     nir_foreach_function_impl(impl, shader) {
      if (options->modes & nir_var_function_temp)
            nir_foreach_block(block, impl)
            nir_metadata_preserve(impl,
                           ralloc_free(ctx);
      }
