   /*
   * Copyright © 2021 Valve Corporation
   *
   * SPDX-License-Identifier: MIT
   */
      #include "ac_nir.h"
   #include "nir_builder.h"
      /*
   * Lower NIR cross-stage I/O intrinsics into the memory accesses that actually happen on the HW.
   *
   * These HW stages are used only when a Geometry Shader is used.
   * Export Shader (ES) runs the SW stage before GS, can be either VS or TES.
   *
   * * GFX6-8:
   *   ES and GS are separate HW stages.
   *   I/O is passed between them through VRAM.
   * * GFX9+:
   *   ES and GS are merged into a single HW stage.
   *   I/O is passed between them through LDS.
   *
   */
      typedef struct {
      /* Which hardware generation we're dealing with */
            /* I/O semantic -> real location used by lowering. */
            /* Stride of an ES invocation outputs in esgs ring, in bytes. */
            /* Enable fix for triangle strip adjacency in geometry shader. */
      } lower_esgs_io_state;
      static nir_def *
   emit_split_buffer_load(nir_builder *b, nir_def *desc, nir_def *v_off, nir_def *s_off,
         {
      unsigned total_bytes = num_components * bit_size / 8u;
   unsigned full_dwords = total_bytes / 4u;
            /* Accommodate max number of split 64-bit loads */
            /* Assume that 1x32-bit load is better than 1x16-bit + 1x8-bit */
   if (remaining_bytes == 3) {
      remaining_bytes = 0;
                        for (unsigned i = 0; i < full_dwords; ++i)
      comps[i] = nir_load_buffer_amd(b, 1, 32, desc, v_off, s_off, zero,
               if (remaining_bytes)
      comps[full_dwords] = nir_load_buffer_amd(b, 1, remaining_bytes * 8, desc, v_off, s_off, zero,
                        }
      static void
   emit_split_buffer_store(nir_builder *b, nir_def *d, nir_def *desc, nir_def *v_off, nir_def *s_off,
               {
               while (writemask) {
      int start, count;
   u_bit_scan_consecutive_range(&writemask, &start, &count);
            unsigned bytes = count * bit_size / 8u;
            while (bytes) {
      unsigned store_bytes = MIN2(bytes, 4u);
   if ((start_byte % 4) == 1 || (start_byte % 4) == 3)
                        nir_def *store_val = nir_extract_bits(b, &d, 1, start_byte * 8u, 1, store_bytes * 8u);
   nir_store_buffer_amd(b, store_val, desc, v_off, s_off, zero,
                              start_byte += store_bytes;
            }
      static bool
   lower_es_output_store(nir_builder *b,
               {
      if (intrin->intrinsic != nir_intrinsic_store_output)
            /* The ARB_shader_viewport_layer_array spec contains the
   * following issue:
   *
   *    2) What happens if gl_ViewportIndex or gl_Layer is
   *    written in the vertex shader and a geometry shader is
   *    present?
   *
   *    RESOLVED: The value written by the last vertex processing
   *    stage is used. If the last vertex processing stage
   *    (vertex, tessellation evaluation or geometry) does not
   *    statically assign to gl_ViewportIndex or gl_Layer, index
   *    or layer zero is assumed.
   *
   * Vulkan spec 15.7 Built-In Variables:
   *
   *   The last active pre-rasterization shader stage (in pipeline order)
   *   controls the Layer that is used. Outputs in previous shader stages
   *   are not used, even if the last stage fails to write the Layer.
   *
   *   The last active pre-rasterization shader stage (in pipeline order)
   *   controls the ViewportIndex that is used. Outputs in previous shader
   *   stages are not used, even if the last stage fails to write the
   *   ViewportIndex.
   *
   * So writes to those outputs in ES are simply ignored.
   */
   unsigned semantic = nir_intrinsic_io_semantics(intrin).location;
   if (semantic == VARYING_SLOT_LAYER || semantic == VARYING_SLOT_VIEWPORT) {
      nir_instr_remove(&intrin->instr);
               lower_esgs_io_state *st = (lower_esgs_io_state *) state;
            b->cursor = nir_before_instr(&intrin->instr);
            if (st->gfx_level <= GFX8) {
      /* GFX6-8: ES is a separate HW stage, data is passed from ES to GS in VRAM. */
   nir_def *ring = nir_load_ring_esgs_amd(b);
   nir_def *es2gs_off = nir_load_ring_es2gs_offset_amd(b);
   emit_split_buffer_store(b, intrin->src[0].ssa, ring, io_off, es2gs_off, 4u,
            } else {
      /* GFX9+: ES is merged into GS, data is passed through LDS. */
   nir_def *vertex_idx = nir_load_local_invocation_index(b);
   nir_def *off = nir_iadd(b, nir_imul_imm(b, vertex_idx, st->esgs_itemsize), io_off);
               nir_instr_remove(&intrin->instr);
      }
      static nir_def *
   gs_get_vertex_offset(nir_builder *b, lower_esgs_io_state *st, unsigned vertex_index)
   {
      nir_def *origin = nir_load_gs_vertex_offset_amd(b, .base = vertex_index);
   if (!st->gs_triangle_strip_adjacency_fix)
            unsigned fixed_index;
   if (st->gfx_level < GFX9) {
      /* Rotate vertex index by 2. */
      } else {
      /* This issue has been fixed for GFX10+ */
   assert(st->gfx_level == GFX9);
   /* 6 vertex offset are packed to 3 vgprs for GFX9+ */
      }
            nir_def *prim_id = nir_load_primitive_id(b);
   /* odd primitive id use fixed offset */
   nir_def *cond = nir_i2b(b, nir_iand_imm(b, prim_id, 1));
      }
      static nir_def *
   gs_per_vertex_input_vertex_offset_gfx6(nir_builder *b, lower_esgs_io_state *st,
         {
      if (nir_src_is_const(*vertex_src))
                     for (unsigned i = 1; i < b->shader->info.gs.vertices_in; ++i) {
      nir_def *cond = nir_ieq_imm(b, vertex_src->ssa, i);
   nir_def *elem = gs_get_vertex_offset(b, st, i);
                  }
      static nir_def *
   gs_per_vertex_input_vertex_offset_gfx9(nir_builder *b, lower_esgs_io_state *st,
         {
      if (nir_src_is_const(*vertex_src)) {
      unsigned vertex = nir_src_as_uint(*vertex_src);
   return nir_ubfe_imm(b, gs_get_vertex_offset(b, st, vertex / 2u),
                        for (unsigned i = 1; i < b->shader->info.gs.vertices_in; i++) {
      nir_def *cond = nir_ieq_imm(b, vertex_src->ssa, i);
   nir_def *elem = gs_get_vertex_offset(b, st, i / 2u * 2u);
   if (i % 2u)
                           }
      static nir_def *
   gs_per_vertex_input_offset(nir_builder *b,
               {
      nir_src *vertex_src = nir_get_io_arrayed_index_src(instr);
   nir_def *vertex_offset = st->gfx_level >= GFX9
      ? gs_per_vertex_input_vertex_offset_gfx9(b, st, vertex_src)
         /* Gfx6-8 can't emulate VGT_ESGS_RING_ITEMSIZE because it uses the register to determine
   * the allocation size of the ESGS ring buffer in memory.
   */
   if (st->gfx_level >= GFX9)
            unsigned base_stride = st->gfx_level >= GFX9 ? 1 : 64 /* Wave size on GFX6-8 */;
   nir_def *io_off = ac_nir_calc_io_offset(b, instr, nir_imm_int(b, base_stride * 4u), base_stride, st->map_io);
   nir_def *off = nir_iadd(b, io_off, vertex_offset);
      }
      static nir_def *
   lower_gs_per_vertex_input_load(nir_builder *b,
               {
      lower_esgs_io_state *st = (lower_esgs_io_state *) state;
   nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
            if (st->gfx_level >= GFX9)
            unsigned wave_size = 64u; /* GFX6-8 only support wave64 */
   nir_def *ring = nir_load_ring_esgs_amd(b);
   return emit_split_buffer_load(b, ring, off, nir_imm_zero(b, 1, 32), 4u * wave_size,
      }
      static bool
   filter_load_per_vertex_input(const nir_instr *instr, UNUSED const void *state)
   {
         }
      void
   ac_nir_lower_es_outputs_to_mem(nir_shader *shader,
                     {
      lower_esgs_io_state state = {
      .gfx_level = gfx_level,
   .esgs_itemsize = esgs_itemsize,
               nir_shader_intrinsics_pass(shader, lower_es_output_store,
            }
      void
   ac_nir_lower_gs_inputs_to_mem(nir_shader *shader,
                     {
      lower_esgs_io_state state = {
      .gfx_level = gfx_level,
   .map_io = map,
               nir_shader_lower_instructions(shader,
                  }
