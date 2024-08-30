   /*
   * Copyright © 2018 Intel Corporation
   * Copyright © 2023 Collabora, Ltd.
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
      #include "util/bitscan.h"
   #include "util/u_math.h"
   #include "nir_builder.h"
      static nir_intrinsic_instr *
   dup_mem_intrinsic(nir_builder *b, nir_intrinsic_instr *intrin,
                     nir_def *offset,
   {
               nir_intrinsic_instr *dup =
            nir_src *intrin_offset_src = nir_get_io_offset_src(intrin);
   for (unsigned i = 0; i < info->num_srcs; i++) {
      if (i == 0 && data != NULL) {
      assert(!info->has_dest);
   assert(&intrin->src[i] != intrin_offset_src);
      } else if (&intrin->src[i] == intrin_offset_src) {
         } else {
                     dup->num_components = num_components;
   for (unsigned i = 0; i < info->num_indices; i++)
                     if (info->has_dest) {
         } else {
                              }
      static bool
   lower_mem_load(nir_builder *b, nir_intrinsic_instr *intrin,
               {
      const unsigned bit_size = intrin->def.bit_size;
   const unsigned num_components = intrin->def.num_components;
   const unsigned bytes_read = num_components * (bit_size / 8);
   const uint32_t align_mul = nir_intrinsic_align_mul(intrin);
   const uint32_t whole_align_offset = nir_intrinsic_align_offset(intrin);
   const uint32_t whole_align = nir_intrinsic_align(intrin);
   nir_src *offset_src = nir_get_io_offset_src(intrin);
   const bool offset_is_const = nir_src_is_const(*offset_src);
            nir_mem_access_size_align requested =
      mem_access_size_align_cb(intrin->intrinsic, bytes_read,
               assert(util_is_power_of_two_nonzero(align_mul));
   assert(util_is_power_of_two_nonzero(requested.align));
   if (requested.num_components == num_components &&
      requested.bit_size == bit_size &&
   requested.align <= whole_align)
         /* Otherwise, we have to break it into chunks.  We could end up with as
   * many as 32 chunks if we're loading a u64vec16 as individual dwords.
   */
   nir_def *chunks[32];
   unsigned num_chunks = 0;
   unsigned chunk_start = 0;
   while (chunk_start < bytes_read) {
      const unsigned bytes_left = bytes_read - chunk_start;
   const uint32_t chunk_align_offset =
         const uint32_t chunk_align =
         requested = mem_access_size_align_cb(intrin->intrinsic, bytes_left,
                  unsigned chunk_bytes;
   assert(util_is_power_of_two_nonzero(requested.align));
   if (align_mul < requested.align) {
      /* For this case, we need to be able to shift the value so we assume
   * the alignment is less than the size of a single component.  This
   * ensures that we don't need to upcast in order to shift.
                  uint64_t align_mask = requested.align - 1;
   nir_def *chunk_offset = nir_iadd_imm(b, offset, chunk_start);
                  nir_intrinsic_instr *load =
      dup_mem_intrinsic(b, intrin, chunk_offset,
               unsigned max_pad = requested.align - chunk_align;
   unsigned requested_bytes =
                                 if (load->def.num_components > 1) {
      nir_def *rev_shift =
                  nir_def *comps[NIR_MAX_VEC_COMPONENTS];
                                 rev_shifted = nir_vec(b, comps, load->def.num_components);
   shifted = nir_bcsel(b, nir_ieq_imm(b, shift, 0), &load->def,
                              /* There's no guarantee that chunk_num_components is a valid NIR
   * vector size, so just loop one chunk component at a time
   */
   for (unsigned i = 0; i < chunk_num_components; i++) {
      assert(num_chunks < ARRAY_SIZE(chunks));
   chunks[num_chunks++] =
      nir_extract_bits(b, &shifted, 1, i * chunk_bit_size,
      } else if (chunk_align_offset % requested.align) {
      /* In this case, we know how much to adjust the offset */
   uint32_t delta = chunk_align_offset % requested.align;
                                 nir_intrinsic_instr *load =
      dup_mem_intrinsic(b, intrin, load_offset,
               assert(requested.bit_size >= 8);
   chunk_bytes = requested.num_components * (requested.bit_size / 8);
                                 /* There's no guarantee that chunk_num_components is a valid NIR
   * vector size, so just loop one chunk component at a time
   */
   nir_def *chunk_data = &load->def;
   for (unsigned i = 0; i < chunk_num_components; i++) {
      assert(num_chunks < ARRAY_SIZE(chunks));
   chunks[num_chunks++] =
      nir_extract_bits(b, &chunk_data, 1,
            } else {
      nir_def *chunk_offset = nir_iadd_imm(b, offset, chunk_start);
   nir_intrinsic_instr *load =
      dup_mem_intrinsic(b, intrin, chunk_offset,
               chunk_bytes = requested.num_components * (requested.bit_size / 8);
   assert(num_chunks < ARRAY_SIZE(chunks));
                           nir_def *result = nir_extract_bits(b, chunks, num_chunks, 0,
         nir_def_rewrite_uses(&intrin->def, result);
               }
      static bool
   lower_mem_store(nir_builder *b, nir_intrinsic_instr *intrin,
               {
               assert(intrin->num_components == value->num_components);
   const unsigned bit_size = value->bit_size;
   const unsigned byte_size = bit_size / 8;
   const unsigned num_components = intrin->num_components;
   const unsigned bytes_written = num_components * byte_size;
   const uint32_t align_mul = nir_intrinsic_align_mul(intrin);
   const uint32_t whole_align_offset = nir_intrinsic_align_offset(intrin);
   const uint32_t whole_align = nir_intrinsic_align(intrin);
   nir_src *offset_src = nir_get_io_offset_src(intrin);
   const bool offset_is_const = nir_src_is_const(*offset_src);
            nir_component_mask_t writemask = nir_intrinsic_write_mask(intrin);
            nir_mem_access_size_align requested =
      mem_access_size_align_cb(intrin->intrinsic, bytes_written,
               assert(util_is_power_of_two_nonzero(align_mul));
   assert(util_is_power_of_two_nonzero(requested.align));
   if (requested.num_components == num_components &&
      requested.bit_size == bit_size &&
   requested.align <= whole_align &&
   writemask == BITFIELD_MASK(num_components))
         assert(byte_size <= sizeof(uint64_t));
   BITSET_DECLARE(mask, NIR_MAX_VEC_COMPONENTS * sizeof(uint64_t));
            for (unsigned i = 0; i < num_components; i++) {
      if (writemask & (1u << i)) {
      BITSET_SET_RANGE_INSIDE_WORD(mask, i * byte_size,
                  while (BITSET_FFS(mask) != 0) {
               uint32_t end;
   for (end = chunk_start + 1; end < bytes_written; end++) {
      if (!(BITSET_TEST(mask, end)))
      }
   /* The size of the current contiguous chunk in bytes */
   const uint32_t max_chunk_bytes = end - chunk_start;
   const uint32_t chunk_align_offset =
         const uint32_t chunk_align =
            requested = mem_access_size_align_cb(intrin->intrinsic, max_chunk_bytes,
                           assert(util_is_power_of_two_nonzero(requested.align));
   if (chunk_align < requested.align ||
      chunk_bytes > max_chunk_bytes) {
   /* Otherwise the caller made a mistake with their return values. */
                  /* We'll turn this into a pair of 32-bit atomics to modify only the right
   * bits of memory.
   */
   requested = (nir_mem_access_size_align){
      .align = 4,
   .bit_size = 32,
               uint64_t align_mask = requested.align - 1;
   nir_def *chunk_offset = nir_iadd_imm(b, offset, chunk_start);
                  unsigned max_pad = chunk_align < requested.align ? requested.align - chunk_align : 0;
   unsigned requested_bytes =
                        nir_def *data;
   if (chunk_bits == 24) {
      /* This is a bit of a special case because we don't have 24-bit integers */
   data = nir_extract_bits(b, &value, 1, chunk_start * 8, 3, 8);
      } else {
      data = nir_extract_bits(b, &value, 1, chunk_start * 8, 1, chunk_bits);
                        if (chunk_align < requested.align) {
      nir_def *shift = nir_u2u32(b, nir_imul_imm(b, pad, 8));
   data = nir_ishl(b, data, shift);
                        switch (intrin->intrinsic) {
   case nir_intrinsic_store_ssbo:
      nir_ssbo_atomic(b, 32, intrin->src[1].ssa, chunk_offset, iand_mask,
               nir_ssbo_atomic(b, 32, intrin->src[1].ssa, chunk_offset, data,
                  case nir_intrinsic_store_global:
      nir_global_atomic(b, 32, chunk_offset, iand_mask,
         nir_global_atomic(b, 32, chunk_offset, data,
            case nir_intrinsic_store_shared:
      nir_shared_atomic(b, 32, chunk_offset, iand_mask,
               nir_shared_atomic(b, 32, chunk_offset, data,
                  default:
            } else {
      nir_def *packed = nir_extract_bits(b, &value, 1, chunk_start * 8,
                  nir_def *chunk_offset = nir_iadd_imm(b, offset, chunk_start);
   dup_mem_intrinsic(b, intrin, chunk_offset,
            }
                           }
      static nir_variable_mode
   intrin_to_variable_mode(nir_intrinsic_op intrin)
   {
      switch (intrin) {
   case nir_intrinsic_load_ubo:
            case nir_intrinsic_load_global:
   case nir_intrinsic_store_global:
            case nir_intrinsic_load_global_constant:
            case nir_intrinsic_load_ssbo:
   case nir_intrinsic_store_ssbo:
            case nir_intrinsic_load_shared:
   case nir_intrinsic_store_shared:
            case nir_intrinsic_load_scratch:
   case nir_intrinsic_store_scratch:
            case nir_intrinsic_load_task_payload:
   case nir_intrinsic_store_task_payload:
            default:
            }
      static bool
   lower_mem_access_instr(nir_builder *b, nir_instr *instr, void *_data)
   {
               if (instr->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   if (!(state->modes & intrin_to_variable_mode(intrin->intrinsic)))
                     switch (intrin->intrinsic) {
   case nir_intrinsic_load_ubo:
   case nir_intrinsic_load_global:
   case nir_intrinsic_load_global_constant:
   case nir_intrinsic_load_ssbo:
   case nir_intrinsic_load_shared:
   case nir_intrinsic_load_scratch:
   case nir_intrinsic_load_task_payload:
            case nir_intrinsic_store_global:
   case nir_intrinsic_store_ssbo:
   case nir_intrinsic_store_shared:
   case nir_intrinsic_store_scratch:
   case nir_intrinsic_store_task_payload:
      return lower_mem_store(b, intrin, state->callback, state->cb_data,
         default:
            }
      bool
   nir_lower_mem_access_bit_sizes(nir_shader *shader,
         {
      return nir_shader_instructions_pass(shader, lower_mem_access_instr,
                  }
