   /*
   * Copyright 2022 Alyssa Rosenzweig
   * SPDX-License-Identifier: MIT
   */
      #include "compiler/nir/nir_builder.h"
   #include "util/bitset.h"
   #include "util/u_dynarray.h"
   #include "agx_state.h"
   #include "nir.h"
   #include "nir_builder_opcodes.h"
   #include "nir_intrinsics.h"
   #include "nir_intrinsics_indices.h"
      /*
   * Lower all system values to uniform loads. This pass tries to compact ranges
   * of contiguous uploaded uniforms to reduce the draw-time overhead of uploading
   * many tiny ranges. To do so, it works in 4 steps:
   *
   * 1. Lower NIR sysvals to loads from the system value buffers.
   * 2. Walk the NIR, recording loads from system value buffers.
   * 2. Walk the ranges of uniforms needed, compacting into contiguous ranges.
   * 3. Fill in the load_preamble instructions with the real uniforms.
   */
      #define MAX_TABLE_SIZE sizeof(struct agx_stage_uniforms)
   static_assert(sizeof(struct agx_draw_uniforms) <= MAX_TABLE_SIZE, "packed");
      struct table_state {
      /* Bitset of 16-bit uniforms pushed */
            /* Element size in 16-bit units, so we may split ranges of different sizes
   * to guarantee natural alignment.
   */
      };
      struct state {
      /* Array of nir_intrinsic_instr's to fix up at the end */
               };
      static nir_def *
   load_sysval(nir_builder *b, unsigned dim, unsigned bitsize, uint8_t table,
         {
      return nir_load_sysval_agx(b, dim, bitsize, .desc_set = table,
      }
      static nir_def *
   load_sysval_root(nir_builder *b, unsigned dim, unsigned bitsize, void *ptr)
   {
         }
      static nir_def *
   load_sysval_indirect(nir_builder *b, unsigned dim, unsigned bitsize,
         {
      nir_scalar scalar = {offset_el, 0};
            if (nir_scalar_is_const(scalar)) {
      /* Load the sysval directly */
   return load_sysval(
      b, dim, bitsize, table,
   } else {
      /* Load the base address of the table */
   struct agx_draw_uniforms *u = NULL;
            /* Load address of the array in the table */
            /* Index into the table and load */
   nir_def *address = nir_iadd(
               }
      static unsigned
   stage_table(nir_builder *b)
   {
      assert(b->shader->info.stage < PIPE_SHADER_TYPES);
      }
      static nir_def *
   load_ubo(nir_builder *b, nir_intrinsic_instr *intr, void *bases)
   {
      nir_def *base =
                     return nir_load_global_constant(b, address, nir_intrinsic_align(intr),
      }
      static nir_def *
   lower_intrinsic(nir_builder *b, nir_intrinsic_instr *intr)
   {
      struct agx_draw_uniforms *u = NULL;
            switch (intr->intrinsic) {
   case nir_intrinsic_load_ubo:
         case nir_intrinsic_load_vbo_base_agx:
      return load_sysval_indirect(b, 1, 64, AGX_SYSVAL_TABLE_ROOT, &u->vbo_base,
      case nir_intrinsic_load_blend_const_color_r_float:
         case nir_intrinsic_load_blend_const_color_g_float:
         case nir_intrinsic_load_blend_const_color_b_float:
         case nir_intrinsic_load_blend_const_color_a_float:
         case nir_intrinsic_load_api_sample_mask_agx:
         case nir_intrinsic_load_sample_positions_agx:
         case nir_intrinsic_load_ssbo_address:
      return load_sysval_indirect(b, 1, 64, stage_table(b), &s->ssbo_base,
      case nir_intrinsic_get_ssbo_size:
      return load_sysval_indirect(b, 1, 32, stage_table(b), &s->ssbo_size,
      case nir_intrinsic_load_num_workgroups:
         case nir_intrinsic_load_layer_id_written_agx:
         case nir_intrinsic_load_xfb_address:
         case nir_intrinsic_load_xfb_size:
         case nir_intrinsic_load_xfb_index_buffer:
         case nir_intrinsic_load_base_vertex:
         case nir_intrinsic_load_num_vertices:
         default:
            }
      /* Step 1. Lower NIR sysvals */
   static bool
   lower_sysvals(nir_builder *b, nir_instr *instr, void *data)
   {
      b->cursor = nir_before_instr(instr);
   nir_def *old;
            if (instr->type == nir_instr_type_intrinsic) {
      nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   old = &intr->def;
      } else if (instr->type == nir_instr_type_tex) {
      nir_tex_instr *tex = nir_instr_as_tex(instr);
            if (tex->op != nir_texop_lod_bias_agx)
                     int src_idx = nir_tex_instr_src_index(tex, nir_tex_src_texture_offset);
   if (src_idx >= 0) {
      replacement = load_sysval_indirect(
      } else {
      replacement = load_sysval(b, 1, 16, stage_table(b),
                  if (replacement != NULL) {
      nir_def_rewrite_uses(old, replacement);
      } else {
            }
      /* Step 2: Record system value loads */
   static bool
   record_loads(nir_builder *b, nir_intrinsic_instr *intr, void *data)
   {
      if (intr->intrinsic != nir_intrinsic_load_sysval_agx)
            assert(intr->def.bit_size >= 16 && "no 8-bit sysvals");
   unsigned dim = intr->def.num_components;
   unsigned element_size = intr->def.bit_size / 16;
            struct state *state = data;
   struct table_state *table = &state->tables[nir_intrinsic_desc_set(intr)];
   unsigned offset = nir_intrinsic_binding(intr);
                     for (unsigned i = 0; i < length; ++i) {
      if (table->element_size[(offset / 2) + i])
         else
               util_dynarray_append(&state->loads, nir_intrinsic_instr *, intr);
      }
      /* Step 3: Decide where to push the system values */
   static struct agx_push_range *
   find_push_range_containing(struct agx_compiled_shader *shader, uint8_t table,
         {
      for (unsigned i = 0; i < shader->push_range_count; ++i) {
               if (range->table != table)
            /* range->length is 16-bit words, need to convert. offset is bytes. */
            if (range->offset <= offset && offset < (range->offset + length_B))
                  }
      static unsigned
   lay_out_table(struct agx_compiled_shader *shader, struct table_state *state,
         {
      unsigned start, end;
   BITSET_FOREACH_RANGE(start, end, state->pushed, sizeof(state->pushed) * 8) {
               do {
               /* Find a range of constant element size. [range_start, range_end).
   * Ranges may be at most 64 halfs.
   */
   unsigned range_end;
   for (range_end = range_start + 1;
      range_end < end && state->element_size[range_end] == size &&
   range_end < range_start + 64;
                                             /* Offsets must be aligned to 4 bytes, this may require pushing a
   * little more than intended (otherwise we would need extra copies)
                  shader->push[shader->push_range_count++] = (struct agx_push_range){
      .uniform = uniform,
   .table = table_index,
   .offset = range_start * 2 /* bytes, not elements */,
               uniform += (range_end - range_start);
                     }
      /* Reserve u0_u1 for the texture base if needed for internal bindless operation.
   * When we have too many textures/images for the available texture state
   * registers, an early lowering pass in the driver spills some textures/images
   * out of texture state registers and instead accesses them as bindless
   * internally. That pass assumes u0_u1 points to the texture descriptors
   * otherwise bound to texture state registers.
   */
   static void
   reserve_internal_bindless(struct state *state, enum pipe_shader_type stage)
   {
      struct table_state *table = &state->tables[AGX_SYSVAL_STAGE(stage)];
   struct agx_stage_uniforms *s = NULL;
            static_assert(offsetof(struct agx_stage_uniforms, texture_base) == 0, "ABI");
                     for (unsigned i = 0; i < len_words; ++i)
      }
      static unsigned
   lay_out_uniforms(struct agx_compiled_shader *shader, struct state *state)
   {
               /* Lay out each system value table. We do this backwards to ensure the first
   * uniform goes to the bindless texture base.
   */
   for (int t = AGX_NUM_SYSVAL_TABLES - 1; t >= 0; --t)
            /* Step 4: Fill in the loads */
   util_dynarray_foreach(&state->loads, nir_intrinsic_instr *, intr_) {
      nir_intrinsic_instr *intr = *intr_;
   uint8_t table = nir_intrinsic_desc_set(intr);
            struct agx_push_range *range =
                     nir_def *repl = nir_load_preamble(
                                 }
      bool
   agx_nir_lower_sysvals(nir_shader *shader)
   {
      return nir_shader_instructions_pass(
      shader, lower_sysvals, nir_metadata_block_index | nir_metadata_dominance,
   }
      bool
   agx_nir_layout_uniforms(nir_shader *shader, bool internal_bindless,
               {
      struct state state = {0};
   nir_shader_intrinsics_pass(shader, record_loads,
                  if (internal_bindless)
                                 }
