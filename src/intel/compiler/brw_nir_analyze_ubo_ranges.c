   /*
   * Copyright © 2015 Intel Corporation
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
      #include "brw_nir.h"
   #include "compiler/nir/nir.h"
   #include "util/u_dynarray.h"
      /**
   * \file brw_nir_analyze_ubo_ranges.c
   *
   * This pass decides which portions of UBOs to upload as push constants,
   * so shaders can access them as part of the thread payload, rather than
   * having to issue expensive memory reads to pull the data.
   *
   * The 3DSTATE_CONSTANT_* mechanism can push data from up to 4 different
   * buffers, in GRF (256-bit/32-byte) units.
   *
   * To do this, we examine NIR load_ubo intrinsics, recording the number of
   * loads at each offset.  We track offsets at a 32-byte granularity, so even
   * fields with a bit of padding between them tend to fall into contiguous
   * ranges.  We build a list of these ranges, tracking their "cost" (number
   * of registers required) and "benefit" (number of pull loads eliminated
   * by pushing the range).  We then sort the list to obtain the four best
   * ranges (most benefit for the least cost).
   */
      struct ubo_range_entry
   {
      struct brw_ubo_range range;
      };
      static int
   score(const struct ubo_range_entry *entry)
   {
         }
      /**
   * Compares score for two UBO range entries.
   *
   * For a descending qsort().
   */
   static int
   cmp_ubo_range_entry(const void *va, const void *vb)
   {
      const struct ubo_range_entry *a = va;
            /* Rank based on scores, descending order */
            /* Then use the UBO block index as a tie-breaker, descending order */
   if (delta == 0)
            /* Finally use the start offset as a second tie-breaker, ascending order */
   if (delta == 0)
               }
      struct ubo_block_info
   {
      /* Each bit in the offsets bitfield represents a 32-byte section of data.
   * If it's set to one, there is interesting UBO data at that offset.  If
   * not, there's a "hole" - padding between data - or just nothing at all.
   */
   uint64_t offsets;
      };
      struct ubo_analysis_state
   {
      struct hash_table *blocks;
      };
      static struct ubo_block_info *
   get_block_info(struct ubo_analysis_state *state, int block)
   {
      uint32_t hash = block + 1;
            struct hash_entry *entry =
            if (entry)
            struct ubo_block_info *info =
                     }
      static void
   analyze_ubos_block(struct ubo_analysis_state *state, nir_block *block)
   {
      nir_foreach_instr(instr, block) {
      if (instr->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   switch (intrin->intrinsic) {
   case nir_intrinsic_load_uniform:
   case nir_intrinsic_image_deref_load:
   case nir_intrinsic_image_deref_store:
   case nir_intrinsic_image_deref_atomic:
   case nir_intrinsic_image_deref_atomic_swap:
   case nir_intrinsic_image_deref_size:
                  case nir_intrinsic_load_ubo:
            default:
                  if (brw_nir_ubo_surface_index_is_pushable(intrin->src[0]) &&
      nir_src_is_const(intrin->src[1])) {
   const int block = brw_nir_ubo_surface_index_get_push_block(intrin->src[0]);
                  /* Avoid shifting by larger than the width of our bitfield, as this
   * is undefined in C.  Even if we require multiple bits to represent
   * the entire value, it's OK to record a partial value - the backend
   * is capable of falling back to pull loads for later components of
   * vectors, as it has to shrink ranges for other reasons anyway.
   */
                  /* The value might span multiple 32-byte chunks. */
   const int bytes = nir_intrinsic_dest_components(intrin) *
         const int start = ROUND_DOWN_TO(byte_offset, 32);
                           struct ubo_block_info *info = get_block_info(state, block);
   info->offsets |= ((1ull << chunks) - 1) << offset;
            }
      static void
   print_ubo_entry(FILE *file,
               {
               fprintf(file,
         "block %2d, start %2d, length %2d, bits = %"PRIx64", "
   "benefit %2d, cost %2d, score = %2d\n",
      }
      void
   brw_nir_analyze_ubo_ranges(const struct brw_compiler *compiler,
                     {
               struct ubo_analysis_state state = {
      .uses_regular_uniforms = false,
   .blocks =
               switch (nir->info.stage) {
   case MESA_SHADER_VERTEX:
      if (vs_key && vs_key->nr_userclip_plane_consts > 0)
               case MESA_SHADER_COMPUTE:
      /* Compute shaders use push constants to get the subgroup ID so it's
   * best to just assume some system values are pushed.
   */
   state.uses_regular_uniforms = true;
         default:
                  /* Walk the IR, recording how many times each UBO block/offset is used. */
   nir_foreach_function_impl(impl, nir) {
      nir_foreach_block(block, impl) {
                     /* Find ranges: a block, starting 32-byte offset, and length. */
   struct util_dynarray ranges;
            hash_table_foreach(state.blocks, entry) {
      const int b = entry->hash - 1;
   const struct ubo_block_info *info = entry->data;
            /* Walk through the offsets bitfield, finding contiguous regions of
   * set bits:
   *
   *   0000000001111111111111000000000000111111111111110000000011111100
   *            ^^^^^^^^^^^^^            ^^^^^^^^^^^^^^        ^^^^^^
   *
   * Each of these will become a UBO range.
   */
   while (offsets != 0) {
      /* Find the first 1 in the offsets bitfield.  This represents the
   * start of a range of interesting UBO data.  Make it zero-indexed.
                  /* Find the first 0 bit in offsets beyond first_bit.  To find the
   * first zero bit, we find the first 1 bit in the complement.  In
   * order to ignore bits before first_bit, we mask off those bits.
                  if (first_hole == -1) {
      /* If we didn't find a hole, then set it to the end of the
   * bitfield.  There are no more ranges to process.
   */
   first_hole = 64;
      } else {
      /* We've processed all bits before first_hole.  Mask them off. */
                              entry->range.block = b;
   entry->range.start = first_bit;
   /* first_hole is one beyond the end, so we don't need to add 1 */
                  for (int i = 0; i < entry->range.length; i++)
                           if (0) {
      util_dynarray_foreach(&ranges, struct ubo_range_entry, entry) {
                     /* TODO: Consider combining ranges.
   *
   * We can only push 3-4 ranges via 3DSTATE_CONSTANT_XS.  If there are
   * more ranges, and two are close by with only a small hole, it may be
   * worth combining them.  The holes will waste register space, but the
   * benefit of removing pulls may outweigh that cost.
            /* Sort the list so the most beneficial ranges are at the front. */
   if (nr_entries > 0) {
      qsort(ranges.data, nr_entries, sizeof(struct ubo_range_entry),
                        /* Return the top 4 or so.  We drop by one if regular uniforms are in
   * use, assuming one push buffer will be dedicated to those.  We may
   * also only get 3 on Haswell if we can't write INSTPM.
   *
   * The backend may need to shrink these ranges to ensure that they
   * don't exceed the maximum push constant limits.  It can simply drop
   * the tail of the list, as that's the least valuable portion.  We
   * unfortunately can't truncate it here, because we don't know what
   * the backend is planning to do with regular uniforms.
   */
   const int max_ubos = (compiler->constant_buffer_0_is_relative ? 3 : 4) -
                  for (int i = 0; i < nr_entries; i++) {
         }
   for (int i = nr_entries; i < 4; i++) {
      out_ranges[i].block = 0;
   out_ranges[i].start = 0;
                  }
