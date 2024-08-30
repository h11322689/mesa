   /*
   * Copyright © 2021 Intel Corporation
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
      #include <list>
   #include <vector>
   #include "brw_compiler.h"
   #include "brw_fs.h"
   #include "brw_nir.h"
   #include "brw_private.h"
   #include "compiler/nir/nir_builder.h"
   #include "dev/intel_debug.h"
      #include <memory>
      using namespace brw;
      static bool
   brw_nir_lower_load_uniforms_filter(const nir_instr *instr,
         {
      if (instr->type != nir_instr_type_intrinsic)
         nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
      }
      static nir_def *
   brw_nir_lower_load_uniforms_impl(nir_builder *b, nir_instr *instr,
         {
      assert(instr->type == nir_instr_type_intrinsic);
   nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
            /* Read the first few 32-bit scalars from InlineData. */
   if (nir_src_is_const(intrin->src[0]) &&
      intrin->def.bit_size == 32 &&
   intrin->def.num_components == 1) {
   unsigned off = nir_intrinsic_base(intrin) + nir_src_as_uint(intrin->src[0]);
   unsigned off_dw = off / 4;
   if (off % 4 == 0 && off_dw < BRW_TASK_MESH_PUSH_CONSTANTS_SIZE_DW) {
      off_dw += BRW_TASK_MESH_PUSH_CONSTANTS_START_DW;
                  return brw_nir_load_global_const(b, intrin,
      }
      static bool
   brw_nir_lower_load_uniforms(nir_shader *nir)
   {
      return nir_shader_lower_instructions(nir, brw_nir_lower_load_uniforms_filter,
      }
      static inline int
   type_size_scalar_dwords(const struct glsl_type *type, bool bindless)
   {
         }
      /* TODO(mesh): Make this a common function. */
   static void
   shared_type_info(const struct glsl_type *type, unsigned *size, unsigned *align)
   {
               uint32_t comp_size = glsl_type_is_boolean(type)
         unsigned length = glsl_get_vector_elements(type);
   *size = comp_size * length,
      }
      static bool
   brw_nir_lower_launch_mesh_workgroups_instr(nir_builder *b,
               {
      if (intrin->intrinsic != nir_intrinsic_launch_mesh_workgroups)
                              /* Make sure that the mesh workgroup size is taken from the first invocation
   * (nir_intrinsic_launch_mesh_workgroups requirement)
   */
   nir_def *cmp = nir_ieq_imm(b, local_invocation_index, 0);
   nir_if *if_stmt = nir_push_if(b, cmp);
   {
      /* TUE header contains 4 words:
   *
   * - Word 0 for Task Count.
   *
   * - Words 1-3 used for "Dispatch Dimensions" feature, to allow mapping a
   *   3D dispatch into the 1D dispatch supported by HW.
   */
   nir_def *x = nir_channel(b, intrin->src[0].ssa, 0);
   nir_def *y = nir_channel(b, intrin->src[0].ssa, 1);
   nir_def *z = nir_channel(b, intrin->src[0].ssa, 2);
   nir_def *task_count = nir_imul(b, x, nir_imul(b, y, z));
   nir_def *tue_header = nir_vec4(b, task_count, x, y, z);
      }
                        }
      static bool
   brw_nir_lower_launch_mesh_workgroups(nir_shader *nir)
   {
      return nir_shader_intrinsics_pass(nir,
                  }
      static void
   brw_nir_lower_tue_outputs(nir_shader *nir, brw_tue_map *map)
   {
               NIR_PASS(_, nir, nir_lower_io, nir_var_shader_out,
            /* From bspec: "It is suggested that SW reserve the 16 bytes following the
   * TUE Header, and therefore start the SW-defined data structure at 32B
   * alignment.  This allows the TUE Header to always be written as 32 bytes
   * with 32B alignment, the most optimal write performance case."
   */
            /* Lowering to explicit types will start offsets from task_payload_size, so
   * set it to start after the header.
   */
   nir->info.task_payload_size = map->per_task_data_start_dw * 4;
   NIR_PASS(_, nir, nir_lower_vars_to_explicit_types,
         NIR_PASS(_, nir, nir_lower_explicit_io,
               }
      static void
   brw_print_tue_map(FILE *fp, const struct brw_tue_map *map)
   {
         }
      static bool
   brw_nir_adjust_task_payload_offsets_instr(struct nir_builder *b,
               {
      switch (intrin->intrinsic) {
   case nir_intrinsic_store_task_payload:
   case nir_intrinsic_load_task_payload: {
               if (nir_src_is_const(*offset_src))
                     /* Regular I/O uses dwords while explicit I/O used for task payload uses
   * bytes.  Normalize it to dwords.
   *
   * TODO(mesh): Figure out how to handle 8-bit, 16-bit.
            nir_def *offset = nir_ishr_imm(b, offset_src->ssa, 2);
            unsigned base = nir_intrinsic_base(intrin);
   assert(base % 4 == 0);
                        default:
            }
      static bool
   brw_nir_adjust_task_payload_offsets(nir_shader *nir)
   {
      return nir_shader_intrinsics_pass(nir,
                        }
      void
   brw_nir_adjust_payload(nir_shader *shader, const struct brw_compiler *compiler)
   {
      /* Adjustment of task payload offsets must be performed *after* last pass
   * which interprets them as bytes, because it changes their unit.
   */
   bool adjusted = false;
   NIR_PASS(adjusted, shader, brw_nir_adjust_task_payload_offsets);
   if (adjusted) /* clean up the mess created by offset adjustments */
      }
      static bool
   brw_nir_align_launch_mesh_workgroups_instr(nir_builder *b,
               {
      if (intrin->intrinsic != nir_intrinsic_launch_mesh_workgroups)
            /* nir_lower_task_shader uses "range" as task payload size. */
   unsigned range = nir_intrinsic_range(intrin);
   /* This will avoid special case in nir_lower_task_shader dealing with
   * not vec4-aligned payload when payload_in_shared workaround is enabled.
   */
               }
      static bool
   brw_nir_align_launch_mesh_workgroups(nir_shader *nir)
   {
      return nir_shader_intrinsics_pass(nir,
                        }
      const unsigned *
   brw_compile_task(const struct brw_compiler *compiler,
         {
      struct nir_shader *nir = params->base.nir;
   const struct brw_task_prog_key *key = params->key;
   struct brw_task_prog_data *prog_data = params->prog_data;
                              nir_lower_task_shader_options lower_ts_opt = {
      .payload_to_shared_for_atomics = true,
   .payload_to_shared_for_small_types = true,
   /* The actual payload data starts after the TUE header and padding,
   * so skip those when copying.
   */
      };
                     prog_data->base.base.stage = MESA_SHADER_TASK;
   prog_data->base.base.total_shared = nir->info.shared_size;
            prog_data->base.local_size[0] = nir->info.workgroup_size[0];
   prog_data->base.local_size[1] = nir->info.workgroup_size[1];
            prog_data->uses_drawid =
            brw_simd_selection_state simd_state{
      .devinfo = compiler->devinfo,
   .prog_data = &prog_data->base,
                        for (unsigned simd = 0; simd < 3; simd++) {
      if (!brw_simd_should_compile(simd_state, simd))
                     nir_shader *shader = nir_shader_clone(params->base.mem_ctx, nir);
            NIR_PASS(_, shader, brw_nir_lower_load_uniforms);
            brw_postprocess_nir(shader, compiler, debug_enabled,
            v[simd] = std::make_unique<fs_visitor>(compiler, &params->base,
                                    if (prog_data->base.prog_mask) {
      unsigned first = ffs(prog_data->base.prog_mask) - 1;
               const bool allow_spilling = !brw_simd_any_compiled(simd_state);
   if (v[simd]->run_task(allow_spilling))
         else
               int selected_simd = brw_simd_select(simd_state);
   if (selected_simd < 0) {
      params->base.error_str =
      ralloc_asprintf(params->base.mem_ctx,
                  "Can't compile shader: "
               fs_visitor *selected = v[selected_simd].get();
            if (unlikely(debug_enabled)) {
      fprintf(stderr, "Task Output ");
               fs_generator g(compiler, &params->base, &prog_data->base.base,
         if (unlikely(debug_enabled)) {
      g.enable_debug(ralloc_asprintf(params->base.mem_ctx,
                                 g.generate_code(selected->cfg, selected->dispatch_width, selected->shader_stats,
         g.add_const_data(nir->constant_data, nir->constant_data_size);
      }
      static void
   brw_nir_lower_tue_inputs(nir_shader *nir, const brw_tue_map *map)
   {
      if (!map)
                              NIR_PASS(progress, nir, nir_lower_vars_to_explicit_types,
            if (progress) {
      /* The types for Task Output and Mesh Input should match, so their sizes
   * should also match.
   */
      } else {
      /* Mesh doesn't read any input, to make it clearer set the
   * task_payload_size to zero instead of keeping an incomplete size that
   * just includes the header.
   */
               NIR_PASS(_, nir, nir_lower_explicit_io, nir_var_mem_task_payload,
      }
      /* Attribute types. Flat attributes have to be a separate class because
   * flat and interpolated attributes can't share the same vec4 slot
   * (see 3DSTATE_SBE.ConstantInterpolationEnable).
   */
   enum {
      PRIM, /* per primitive */
   VERT, /* per vertex interpolated */
      };
      struct attr_desc {
      int location;
   const struct glsl_type *type;
   unsigned dwords;
      };
      struct attr_type_info {
      /* order of attributes, negative values are holes */
            /* attributes after which there's hole of size equal to array index */
      };
      static void
   brw_mue_assign_position(const struct attr_desc *attr,
               {
      bool is_array = glsl_type_is_array(attr->type);
   int location = attr->location;
            for (unsigned slot = 0; slot < attr->slots; ++slot) {
                        if (is_array) {
      assert(attr->dwords % attr->slots == 0);
      } else {
                  map->len_dw[location + slot] = sz;
   start_dw += sz;
         }
      static nir_variable *
   brw_nir_find_complete_variable_with_location(nir_shader *shader,
               {
      nir_variable *best_var = NULL;
            nir_foreach_variable_with_modes(var, shader, mode) {
      if (var->data.location != location)
            unsigned new_size = glsl_count_dword_slots(var->type, false);
   if (new_size > last_size) {
      best_var = var;
                     }
      static unsigned
   brw_sum_size(const std::list<struct attr_desc> &orders)
   {
      unsigned sz = 0;
   for (auto it = orders.cbegin(); it != orders.cend(); ++it)
            }
      /* Finds order of outputs which require minimum size, without splitting
   * of URB read/write messages (which operate on vec4-aligned memory).
   */
   static void
   brw_compute_mue_layout(const struct brw_compiler *compiler,
                        std::list<struct attr_desc> *orders,
      {
                        if ((compiler->mesh.mue_header_packing & 1) == 0)
         if ((compiler->mesh.mue_header_packing & 2) == 0)
            for (unsigned i = PRIM; i <= VERT_FLAT; ++i)
            /* If packing into header is enabled, add a hole of size 4 and add
   * a virtual location to keep the algorithm happy (it expects holes
   * to be preceded by some location). We'll remove those virtual
   * locations at the end.
   */
   const gl_varying_slot virtual_header_location = VARYING_SLOT_POS;
            struct attr_desc d;
   d.location = virtual_header_location;
   d.type = NULL;
   d.dwords = 0;
            struct attr_desc h;
   h.location = -1;
   h.type = NULL;
   h.dwords = 4;
            if (*pack_prim_data_into_header) {
      orders[PRIM].push_back(d);
   orders[PRIM].push_back(h);
               if (*pack_vert_data_into_header) {
      orders[VERT].push_back(d);
   orders[VERT].push_back(h);
               u_foreach_bit64(location, outputs_written) {
      if ((BITFIELD64_BIT(location) & outputs_written) == 0)
            /* At this point there are both complete and split variables as
   * outputs. We need the complete variable to compute the required
   * size.
   */
   nir_variable *var =
         brw_nir_find_complete_variable_with_location(nir,
            d.location = location;
   d.type     = brw_nir_get_var_type(nir, var);
   d.dwords   = glsl_count_dword_slots(d.type, false);
                     if (BITFIELD64_BIT(location) & info->per_primitive_outputs)
         else if (var->data.interpolation == INTERP_MODE_FLAT)
         else
            std::list<struct attr_desc> *order = type_data->order;
                     /* special case to use hole of size 4 */
   if (d.dwords == 4 && !holes[4].empty()) {
                              assert(order->front().location == -1);
                              int mod = d.dwords % 4;
   if (mod == 0) {
      order->push_back(d);
               h.location = -1;
   h.type = NULL;
   h.dwords = 4 - mod;
            if (!compiler->mesh.mue_compaction) {
      order->push_back(d);
   order->push_back(h);
               if (d.dwords > 4) {
      order->push_back(d);
   order->push_back(h);
   holes[h.dwords].push_back(location);
                        unsigned found = 0;
   /* try to find the smallest hole big enough to hold this attribute */
   for (unsigned sz = d.dwords; sz <= 4; sz++){
      if (!holes[sz].empty()) {
      found = sz;
                  /* append at the end if not found */
   if (found == 0) {
      order->push_back(d);
                              assert(found <= 4);
   assert(!holes[found].empty());
   int after_loc = holes[found].back();
                     for (auto it = order->begin(); it != order->end(); ++it) {
                     ++it;
   /* must be a hole */
   assert((*it).location < 0);
                  if (d.dwords == (*it).dwords) {
      /* exact size, just replace */
      } else {
      /* inexact size, shrink hole */
   (*it).dwords -= d.dwords;
                  /* Insert shrunk hole in a spot so that the order of attributes
   * is preserved.
   */
                  for (auto it2 = hole_list.begin(); it2 != hole_list.end(); ++it2) {
      if ((*it2) >= (int)location) {
      insert_before = it2;
                              inserted_back = true;
                           if (*pack_prim_data_into_header) {
      if (orders[PRIM].front().location == virtual_header_location)
            if (!data[PRIM].holes[4].empty()) {
               assert(orders[PRIM].front().location == -1);
   assert(orders[PRIM].front().dwords == 4);
               if (*pack_prim_data_into_header) {
               if (sz % 8 == 0 || sz % 8 > 4)
                  if (*pack_vert_data_into_header) {
      if (orders[VERT].front().location == virtual_header_location)
            if (!data[VERT].holes[4].empty()) {
               assert(orders[VERT].front().location == -1);
   assert(orders[VERT].front().dwords == 4);
               if (*pack_vert_data_into_header) {
                     if (sz % 8 == 0 || sz % 8 > 4)
                     if (INTEL_DEBUG(DEBUG_MESH)) {
      fprintf(stderr, "MUE attribute order:\n");
   for (unsigned i = PRIM; i <= VERT_FLAT; ++i) {
      if (!orders[i].empty())
         for (auto it = orders[i].cbegin(); it != orders[i].cend(); ++it) {
         }
   if (!orders[i].empty())
            }
      /* Mesh URB Entry consists of an initial section
   *
   *  - Primitive Count
   *  - Primitive Indices (from 0 to Max-1)
   *  - Padding to 32B if needed
   *
   * optionally followed by a section for per-primitive data,
   * in which each primitive (from 0 to Max-1) gets
   *
   *  - Primitive Header (e.g. ViewportIndex)
   *  - Primitive Custom Attributes
   *
   * then followed by a section for per-vertex data
   *
   *  - Vertex Header (e.g. Position)
   *  - Vertex Custom Attributes
   *
   * Each per-element section has a pitch and a starting offset.  All the
   * individual attributes offsets in start_dw are considering the first entry
   * of the section (i.e. where the Position for first vertex, or ViewportIndex
   * for first primitive).  Attributes for other elements are calculated using
   * the pitch.
   */
   static void
   brw_compute_mue_map(const struct brw_compiler *compiler,
               {
               memset(&map->start_dw[0], -1, sizeof(map->start_dw));
            unsigned vertices_per_primitive =
            map->max_primitives = nir->info.mesh.max_primitives_out;
                     /* One dword for primitives count then K extra dwords for each primitive. */
   switch (index_format) {
   case BRW_INDEX_FORMAT_U32:
      map->per_primitive_indices_dw = vertices_per_primitive;
      case BRW_INDEX_FORMAT_U888X:
      map->per_primitive_indices_dw = 1;
      default:
                  map->per_primitive_start_dw = ALIGN(map->per_primitive_indices_dw *
            /* Assign initial section. */
   if (BITFIELD64_BIT(VARYING_SLOT_PRIMITIVE_COUNT) & outputs_written) {
      map->start_dw[VARYING_SLOT_PRIMITIVE_COUNT] = 0;
   map->len_dw[VARYING_SLOT_PRIMITIVE_COUNT] = 1;
      }
   if (BITFIELD64_BIT(VARYING_SLOT_PRIMITIVE_INDICES) & outputs_written) {
      map->start_dw[VARYING_SLOT_PRIMITIVE_INDICES] = 1;
   map->len_dw[VARYING_SLOT_PRIMITIVE_INDICES] =
                     const uint64_t per_primitive_header_bits =
         BITFIELD64_BIT(VARYING_SLOT_PRIMITIVE_SHADING_RATE) |
   BITFIELD64_BIT(VARYING_SLOT_LAYER) |
            const uint64_t per_vertex_header_bits =
         BITFIELD64_BIT(VARYING_SLOT_PSIZ) |
   BITFIELD64_BIT(VARYING_SLOT_POS) |
            std::list<struct attr_desc> orders[3];
   uint64_t regular_outputs = outputs_written &
            /* packing into prim header is possible only if prim header is present */
   map->user_data_in_primitive_header = compact_mue &&
            /* Packing into vert header is always possible, but we allow it only
   * if full vec4 is available (so point size is not used) and there's
   * nothing between it and normal vertex data (so no clip distances).
   */
   map->user_data_in_vertex_header = compact_mue &&
                  if (outputs_written & per_primitive_header_bits) {
      if (outputs_written & BITFIELD64_BIT(VARYING_SLOT_PRIMITIVE_SHADING_RATE)) {
      map->start_dw[VARYING_SLOT_PRIMITIVE_SHADING_RATE] =
                     if (outputs_written & BITFIELD64_BIT(VARYING_SLOT_LAYER)) {
      map->start_dw[VARYING_SLOT_LAYER] =
                     if (outputs_written & BITFIELD64_BIT(VARYING_SLOT_VIEWPORT)) {
      map->start_dw[VARYING_SLOT_VIEWPORT] =
                     if (outputs_written & BITFIELD64_BIT(VARYING_SLOT_CULL_PRIMITIVE)) {
      map->start_dw[VARYING_SLOT_CULL_PRIMITIVE] =
                     map->per_primitive_header_size_dw = 8;
      } else {
                           /* For fast linked libraries, we can't pack the MUE, as the fragment shader
   * will be compiled without access to the MUE map and won't be able to find
   * out where everything is.
   * Instead, keep doing things as we did before the packing, just laying out
   * everything in varying order, which is how the FS will expect them.
   */
   if (compact_mue) {
      brw_compute_mue_layout(compiler, orders, regular_outputs, nir,
                  unsigned start_dw = map->per_primitive_start_dw;
   if (map->user_data_in_primitive_header)
         else
                  for (auto it = orders[PRIM].cbegin(); it != orders[PRIM].cend(); ++it) {
      int location = (*it).location;
   if (location < 0) {
      start_dw += (*it).dwords;
   if (map->user_data_in_primitive_header && header_used_dw < 4)
         else
         assert(header_used_dw <= 4);
                                                start_dw += (*it).dwords;
   if (map->user_data_in_primitive_header && header_used_dw < 4)
         else
         assert(header_used_dw <= 4);
         } else {
      unsigned start_dw = map->per_primitive_start_dw +
            uint64_t per_prim_outputs = outputs_written & nir->info.per_primitive_outputs;
   while (per_prim_outputs) {
               assert(map->start_dw[location] == -1);
                  nir_variable *var =
      brw_nir_find_complete_variable_with_location(nir,
            struct attr_desc d;
   d.location = location;
   d.type     = brw_nir_get_var_type(nir, var);
                                                         map->per_primitive_pitch_dw = ALIGN(map->per_primitive_header_size_dw +
            map->per_vertex_start_dw = ALIGN(map->per_primitive_start_dw +
                  /* TODO(mesh): Multiview. */
   unsigned fixed_header_size = 8;
   map->per_vertex_header_size_dw = ALIGN(fixed_header_size +
                  if (outputs_written & per_vertex_header_bits) {
      if (outputs_written & BITFIELD64_BIT(VARYING_SLOT_PSIZ)) {
      map->start_dw[VARYING_SLOT_PSIZ] = map->per_vertex_start_dw + 3;
               if (outputs_written & BITFIELD64_BIT(VARYING_SLOT_POS)) {
      map->start_dw[VARYING_SLOT_POS] = map->per_vertex_start_dw + 4;
               if (outputs_written & BITFIELD64_BIT(VARYING_SLOT_CLIP_DIST0)) {
      map->start_dw[VARYING_SLOT_CLIP_DIST0] =
                     if (outputs_written & BITFIELD64_BIT(VARYING_SLOT_CLIP_DIST1)) {
      map->start_dw[VARYING_SLOT_CLIP_DIST1] =
                                 /* cull distances should be lowered earlier */
   assert(!(outputs_written & BITFIELD64_BIT(VARYING_SLOT_CULL_DIST0)));
                     /* For fast linked libraries, we can't pack the MUE, as the fragment shader
   * will be compiled without access to the MUE map and won't be able to find
   * out where everything is.
   * Instead, keep doing things as we did before the packing, just laying out
   * everything in varying order, which is how the FS will expect them.
   */
   if (compact_mue) {
      unsigned start_dw = map->per_vertex_start_dw;
   if (!map->user_data_in_vertex_header)
            unsigned header_used_dw = 0;
   for (unsigned type = VERT; type <= VERT_FLAT; ++type) {
      for (auto it = orders[type].cbegin(); it != orders[type].cend(); ++it) {
      int location = (*it).location;
   if (location < 0) {
      start_dw += (*it).dwords;
   if (map->user_data_in_vertex_header && header_used_dw < 4) {
      header_used_dw += (*it).dwords;
   assert(header_used_dw <= 4);
   if (header_used_dw == 4)
      } else {
                                                      start_dw += (*it).dwords;
   if (map->user_data_in_vertex_header && header_used_dw < 4) {
      header_used_dw += (*it).dwords;
   assert(header_used_dw <= 4);
   if (header_used_dw == 4)
      } else {
         }
            } else {
      unsigned start_dw = map->per_vertex_start_dw +
            uint64_t per_vertex_outputs = outputs_written & ~nir->info.per_primitive_outputs;
   while (per_vertex_outputs) {
                              nir_variable *var =
      brw_nir_find_complete_variable_with_location(nir,
            struct attr_desc d;
   d.location = location;
   d.type     = brw_nir_get_var_type(nir, var);
                                                         map->per_vertex_pitch_dw = ALIGN(map->per_vertex_header_size_dw +
            map->size_dw =
               }
      static void
   brw_print_mue_map(FILE *fp, const struct brw_mue_map *map, struct nir_shader *nir)
   {
      fprintf(fp, "MUE map (%d dwords, %d primitives, %d vertices)\n",
         fprintf(fp, "  <%4d, %4d>: VARYING_SLOT_PRIMITIVE_COUNT\n",
         map->start_dw[VARYING_SLOT_PRIMITIVE_COUNT],
   map->start_dw[VARYING_SLOT_PRIMITIVE_COUNT] +
   fprintf(fp, "  <%4d, %4d>: VARYING_SLOT_PRIMITIVE_INDICES\n",
         map->start_dw[VARYING_SLOT_PRIMITIVE_INDICES],
            fprintf(fp, "  ----- per primitive (start %d, header_size %d, data_size %d, pitch %d)\n",
         map->per_primitive_start_dw,
   map->per_primitive_header_size_dw,
            for (unsigned i = 0; i < VARYING_SLOT_MAX; i++) {
      if (map->start_dw[i] < 0)
            const unsigned offset = map->start_dw[i];
            if (offset < map->per_primitive_start_dw ||
                  const char *name =
                  fprintf(fp, "  <%4d, %4d>: %s (%d)\n", offset, offset + len - 1,
               fprintf(fp, "  ----- per vertex (start %d, header_size %d, data_size %d, pitch %d)\n",
         map->per_vertex_start_dw,
   map->per_vertex_header_size_dw,
            for (unsigned i = 0; i < VARYING_SLOT_MAX; i++) {
      if (map->start_dw[i] < 0)
            const unsigned offset = map->start_dw[i];
            if (offset < map->per_vertex_start_dw ||
                  nir_variable *var =
                  const char *name =
                  fprintf(fp, "  <%4d, %4d>: %s (%d)%s\n", offset, offset + len - 1,
                  }
      static void
   brw_nir_lower_mue_outputs(nir_shader *nir, const struct brw_mue_map *map)
   {
      nir_foreach_shader_out_variable(var, nir) {
      int location = var->data.location;
   assert(location >= 0);
   assert(map->start_dw[location] != -1);
               NIR_PASS(_, nir, nir_lower_io, nir_var_shader_out,
      }
      static void
   brw_nir_initialize_mue(nir_shader *nir,
               {
               nir_builder b;
   nir_function_impl *entrypoint = nir_shader_get_entrypoint(nir);
            nir_def *dw_off = nir_imm_int(&b, 0);
                     assert(!nir->info.workgroup_size_variable);
   const unsigned workgroup_size = nir->info.workgroup_size[0] *
                           /* How many prims each invocation needs to cover without checking its index? */
            /* Zero first 4 dwords of MUE Primitive Header:
   * Reserved, RTAIndex, ViewportIndex, CullPrimitiveMask.
                     /* Zero primitive headers distanced by workgroup_size, starting from
   * invocation index.
   */
   for (unsigned prim_in_inv = 0; prim_in_inv < prims_per_inv; ++prim_in_inv) {
      nir_def *prim = nir_iadd_imm(&b, local_invocation_index,
            nir_store_per_primitive_output(&b, zerovec, prim, dw_off,
                                 /* How many prims are left? */
            if (remaining) {
      /* Zero "remaining" primitive headers starting from the last one covered
   * by the loop above + workgroup_size.
   */
   nir_def *cmp = nir_ilt_imm(&b, local_invocation_index, remaining);
   nir_if *if_stmt = nir_push_if(&b, cmp);
   {
                     nir_store_per_primitive_output(&b, zerovec, prim, dw_off,
                        }
               /* If there's more than one subgroup, then we need to wait for all of them
   * to finish initialization before we can proceed. Otherwise some subgroups
   * may start filling MUE before other finished initializing.
   */
   if (workgroup_size > dispatch_width) {
      nir_barrier(&b, SCOPE_WORKGROUP, SCOPE_WORKGROUP,
               if (remaining) {
         } else {
      nir_metadata_preserve(entrypoint, nir_metadata_block_index |
         }
      static void
   brw_nir_adjust_offset(nir_builder *b, nir_intrinsic_instr *intrin, uint32_t pitch)
   {
      nir_src *index_src = nir_get_io_arrayed_index_src(intrin);
            b->cursor = nir_before_instr(&intrin->instr);
   nir_def *offset =
      nir_iadd(b,
               }
      static bool
   brw_nir_adjust_offset_for_arrayed_indices_instr(nir_builder *b,
               {
               /* Remap per_vertex and per_primitive offsets using the extra source and
   * the pitch.
   */
   switch (intrin->intrinsic) {
   case nir_intrinsic_load_per_vertex_output:
   case nir_intrinsic_store_per_vertex_output:
                     case nir_intrinsic_load_per_primitive_output:
   case nir_intrinsic_store_per_primitive_output: {
      struct nir_io_semantics sem = nir_intrinsic_io_semantics(intrin);
   uint32_t pitch;
   if (sem.location == VARYING_SLOT_PRIMITIVE_INDICES)
         else
                                 default:
            }
      static bool
   brw_nir_adjust_offset_for_arrayed_indices(nir_shader *nir, const struct brw_mue_map *map)
   {
      return nir_shader_intrinsics_pass(nir,
                        }
      struct index_packing_state {
      unsigned vertices_per_primitive;
   nir_variable *original_prim_indices;
      };
      static bool
   brw_can_pack_primitive_indices(nir_shader *nir, struct index_packing_state *state)
   {
      /* can single index fit into one byte of U888X format? */
   if (nir->info.mesh.max_vertices_out > 255)
            state->vertices_per_primitive =
         /* packing point indices doesn't help */
   if (state->vertices_per_primitive == 1)
            state->original_prim_indices =
      nir_find_variable_with_location(nir,
            /* no indices = no changes to the shader, but it's still worth it,
   * because less URB space will be used
   */
   if (!state->original_prim_indices)
            ASSERTED const struct glsl_type *type = state->original_prim_indices->type;
   assert(type->is_array());
   assert(type->without_array()->is_vector());
            nir_foreach_function_impl(impl, nir) {
      nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
                              if (intrin->intrinsic != nir_intrinsic_store_deref) {
      /* any unknown deref operation on primitive indices -> don't pack */
   unsigned num_srcs = nir_intrinsic_infos[intrin->intrinsic].num_srcs;
   for (unsigned i = 0; i < num_srcs; i++) {
      nir_deref_instr *deref = nir_src_as_deref(intrin->src[i]);
                                                      nir_deref_instr *deref = nir_src_as_deref(intrin->src[0]);
                  nir_variable *var = nir_deref_instr_get_variable(deref);
                                 nir_deref_instr *var_deref = nir_src_as_deref(deref->parent);
                                    /* If only some components are written, then we can't easily pack.
   * In theory we could, by loading current dword value, bitmasking
   * one byte and storing back the whole dword, but it would be slow
   * and could actually decrease performance. TODO: reevaluate this
   * once there will be something hitting this.
   */
   if (write_mask != BITFIELD_MASK(state->vertices_per_primitive))
                        }
      static bool
   brw_pack_primitive_indices_instr(nir_builder *b, nir_intrinsic_instr *intrin,
         {
      if (intrin->intrinsic != nir_intrinsic_store_deref)
            nir_deref_instr *array_deref = nir_src_as_deref(intrin->src[0]);
   if (!array_deref || array_deref->deref_type != nir_deref_type_array)
            nir_deref_instr *var_deref = nir_src_as_deref(array_deref->parent);
   if (!var_deref || var_deref->deref_type != nir_deref_type_var)
            struct index_packing_state *state =
                     if (var != state->original_prim_indices)
                              nir_deref_instr *new_var_deref =
         nir_deref_instr *new_array_deref =
            nir_src *data_src = &intrin->src[1];
   nir_def *data_def =
            nir_def *new_data =
                  if (vertices_per_primitive >= 3) {
      new_data =
                                          }
      static bool
   brw_pack_primitive_indices(nir_shader *nir, void *data)
   {
               const struct glsl_type *new_type =
         glsl_array_type(glsl_uint_type(),
            state->packed_prim_indices =
         nir_variable_create(nir, nir_var_shader_out,
   state->packed_prim_indices->data.location = VARYING_SLOT_PRIMITIVE_INDICES;
   state->packed_prim_indices->data.interpolation = INTERP_MODE_NONE;
            return nir_shader_intrinsics_pass(nir, brw_pack_primitive_indices_instr,
                  }
      const unsigned *
   brw_compile_mesh(const struct brw_compiler *compiler,
         {
      struct nir_shader *nir = params->base.nir;
   const struct brw_mesh_prog_key *key = params->key;
   struct brw_mesh_prog_data *prog_data = params->prog_data;
            prog_data->base.base.stage = MESA_SHADER_MESH;
   prog_data->base.base.total_shared = nir->info.shared_size;
            prog_data->base.local_size[0] = nir->info.workgroup_size[0];
   prog_data->base.local_size[1] = nir->info.workgroup_size[1];
            prog_data->clip_distance_mask = (1 << nir->info.clip_distance_array_size) - 1;
   prog_data->cull_distance_mask =
         ((1 << nir->info.cull_distance_array_size) - 1) <<
            struct index_packing_state index_packing_state = {};
   if (brw_can_pack_primitive_indices(nir, &index_packing_state)) {
      if (index_packing_state.original_prim_indices)
            } else {
                  prog_data->uses_drawid =
                     brw_compute_mue_map(compiler, nir, &prog_data->map,
                  brw_simd_selection_state simd_state{
      .devinfo = compiler->devinfo,
   .prog_data = &prog_data->base,
                        for (int simd = 0; simd < 3; simd++) {
      if (!brw_simd_should_compile(simd_state, simd))
                              /*
   * When Primitive Header is enabled, we may not generates writes to all
   * fields, so let's initialize everything.
   */
   if (prog_data->map.per_primitive_header_size_dw > 0)
                     NIR_PASS(_, shader, brw_nir_adjust_offset_for_arrayed_indices, &prog_data->map);
   /* Load uniforms can do a better job for constants, so fold before it. */
   NIR_PASS(_, shader, nir_opt_constant_folding);
                     brw_postprocess_nir(shader, compiler, debug_enabled,
            v[simd] = std::make_unique<fs_visitor>(compiler, &params->base,
                                    if (prog_data->base.prog_mask) {
      unsigned first = ffs(prog_data->base.prog_mask) - 1;
               const bool allow_spilling = !brw_simd_any_compiled(simd_state);
   if (v[simd]->run_mesh(allow_spilling))
         else
               int selected_simd = brw_simd_select(simd_state);
   if (selected_simd < 0) {
      params->base.error_str =
      ralloc_asprintf(params->base.mem_ctx,
                  "Can't compile shader: "
               fs_visitor *selected = v[selected_simd].get();
            if (unlikely(debug_enabled)) {
      if (params->tue_map) {
      fprintf(stderr, "Mesh Input ");
      }
   fprintf(stderr, "Mesh Output ");
               fs_generator g(compiler, &params->base, &prog_data->base.base,
         if (unlikely(debug_enabled)) {
      g.enable_debug(ralloc_asprintf(params->base.mem_ctx,
                                 g.generate_code(selected->cfg, selected->dispatch_width, selected->shader_stats,
         g.add_const_data(nir->constant_data, nir->constant_data_size);
      }
      static unsigned
   component_from_intrinsic(nir_intrinsic_instr *instr)
   {
      if (nir_intrinsic_has_component(instr))
         else
      }
      static void
   adjust_handle_and_offset(const fs_builder &bld,
               {
      /* Make sure that URB global offset is below 2048 (2^11), because
   * that's the maximum possible value encoded in Message Descriptor.
   */
            if (adjustment) {
      fs_builder ubld8 = bld.group(8, 0).exec_all();
   /* Allocate new register to not overwrite the shared URB handle. */
   fs_reg new_handle = ubld8.vgrf(BRW_REGISTER_TYPE_UD);
   ubld8.ADD(new_handle, urb_handle, brw_imm_ud(adjustment));
   urb_handle = new_handle;
         }
      static void
   emit_urb_direct_vec4_write(const fs_builder &bld,
                              unsigned urb_global_offset,
      {
      for (unsigned q = 0; q < bld.dispatch_width() / 8; q++) {
               fs_reg payload_srcs[8];
            for (unsigned i = 0; i < dst_comp_offset; i++)
            for (unsigned c = 0; c < comps; c++)
            fs_reg srcs[URB_LOGICAL_NUM_SRCS];
   srcs[URB_LOGICAL_SRC_HANDLE] = urb_handle;
   srcs[URB_LOGICAL_SRC_CHANNEL_MASK] = brw_imm_ud(mask << 16);
   srcs[URB_LOGICAL_SRC_DATA] = fs_reg(VGRF, bld.shader->alloc.allocate(length),
         srcs[URB_LOGICAL_SRC_COMPONENTS] = brw_imm_ud(length);
            fs_inst *inst = bld8.emit(SHADER_OPCODE_URB_WRITE_LOGICAL,
         inst->offset = urb_global_offset;
         }
      static void
   emit_urb_direct_writes(const fs_builder &bld, nir_intrinsic_instr *instr,
         {
               nir_src *offset_nir_src = nir_get_io_offset_src(instr);
            const unsigned comps = nir_src_num_components(instr->src[0]);
            const unsigned offset_in_dwords = nir_intrinsic_base(instr) +
                  /* URB writes are vec4 aligned but the intrinsic offsets are in dwords.
   * We can write up to 8 dwords, so single vec4 write is enough.
   */
   const unsigned comp_shift = offset_in_dwords % 4;
            unsigned urb_global_offset = offset_in_dwords / 4;
            emit_urb_direct_vec4_write(bld, urb_global_offset, src, urb_handle,
      }
      static void
   emit_urb_direct_vec4_write_xe2(const fs_builder &bld,
                                 {
      const struct intel_device_info *devinfo = bld.shader->devinfo;
   const unsigned runit = reg_unit(devinfo);
            if (offset_in_bytes > 0) {
      fs_builder bldall = bld.group(write_size, 0).exec_all();
   fs_reg new_handle = bldall.vgrf(BRW_REGISTER_TYPE_UD);
   bldall.ADD(new_handle, urb_handle, brw_imm_ud(offset_in_bytes));
               for (unsigned q = 0; q < bld.dispatch_width() / write_size; q++) {
                        for (unsigned c = 0; c < comps; c++)
            fs_reg srcs[URB_LOGICAL_NUM_SRCS];
   srcs[URB_LOGICAL_SRC_HANDLE] = urb_handle;
   srcs[URB_LOGICAL_SRC_CHANNEL_MASK] = brw_imm_ud(mask << 16);
   int nr = bld.shader->alloc.allocate(comps * runit);
   srcs[URB_LOGICAL_SRC_DATA] = fs_reg(VGRF, nr, BRW_REGISTER_TYPE_F);
   srcs[URB_LOGICAL_SRC_COMPONENTS] = brw_imm_ud(comps);
            hbld.emit(SHADER_OPCODE_URB_WRITE_LOGICAL,
         }
      static void
   emit_urb_direct_writes_xe2(const fs_builder &bld, nir_intrinsic_instr *instr,
         {
               nir_src *offset_nir_src = nir_get_io_offset_src(instr);
            const unsigned comps = nir_src_num_components(instr->src[0]);
            const unsigned offset_in_dwords = nir_intrinsic_base(instr) +
                           emit_urb_direct_vec4_write_xe2(bld, offset_in_dwords * 4, src,
      }
      static void
   emit_urb_indirect_vec4_write(const fs_builder &bld,
                              const fs_reg &offset_src,
   unsigned base,
      {
      for (unsigned q = 0; q < bld.dispatch_width() / 8; q++) {
               /* offset is always positive, so signedness doesn't matter */
   assert(offset_src.type == BRW_REGISTER_TYPE_D ||
         fs_reg off = bld8.vgrf(offset_src.type, 1);
   bld8.MOV(off, quarter(offset_src, q));
   bld8.ADD(off, off, brw_imm_ud(base));
            fs_reg payload_srcs[8];
            for (unsigned i = 0; i < dst_comp_offset; i++)
            for (unsigned c = 0; c < comps; c++)
            fs_reg srcs[URB_LOGICAL_NUM_SRCS];
   srcs[URB_LOGICAL_SRC_HANDLE] = urb_handle;
   srcs[URB_LOGICAL_SRC_PER_SLOT_OFFSETS] = off;
   srcs[URB_LOGICAL_SRC_CHANNEL_MASK] = brw_imm_ud(mask << 16);
   srcs[URB_LOGICAL_SRC_DATA] = fs_reg(VGRF, bld.shader->alloc.allocate(length),
         srcs[URB_LOGICAL_SRC_COMPONENTS] = brw_imm_ud(length);
            fs_inst *inst = bld8.emit(SHADER_OPCODE_URB_WRITE_LOGICAL,
               }
      static void
   emit_urb_indirect_writes_mod(const fs_builder &bld, nir_intrinsic_instr *instr,
               {
               const unsigned comps = nir_src_num_components(instr->src[0]);
            const unsigned base_in_dwords = nir_intrinsic_base(instr) +
            const unsigned comp_shift = mod;
            emit_urb_indirect_vec4_write(bld, offset_src, base_in_dwords, src,
      }
      static void
   emit_urb_indirect_writes_xe2(const fs_builder &bld, nir_intrinsic_instr *instr,
               {
               const struct intel_device_info *devinfo = bld.shader->devinfo;
   const unsigned runit = reg_unit(devinfo);
            const unsigned comps = nir_src_num_components(instr->src[0]);
            const unsigned base_in_dwords = nir_intrinsic_base(instr) +
            if (base_in_dwords > 0) {
      fs_builder bldall = bld.group(write_size, 0).exec_all();
   fs_reg new_handle = bldall.vgrf(BRW_REGISTER_TYPE_UD);
   bldall.ADD(new_handle, urb_handle, brw_imm_ud(base_in_dwords * 4));
                        for (unsigned q = 0; q < bld.dispatch_width() / write_size; q++) {
                        for (unsigned c = 0; c < comps; c++)
            fs_reg addr = wbld.vgrf(BRW_REGISTER_TYPE_UD);
   wbld.SHL(addr, horiz_offset(offset_src, write_size * q), brw_imm_ud(2));
            fs_reg srcs[URB_LOGICAL_NUM_SRCS];
   srcs[URB_LOGICAL_SRC_HANDLE] = addr;
   srcs[URB_LOGICAL_SRC_CHANNEL_MASK] = brw_imm_ud(mask << 16);
   int nr = bld.shader->alloc.allocate(comps * runit);
   srcs[URB_LOGICAL_SRC_DATA] = fs_reg(VGRF, nr, BRW_REGISTER_TYPE_F);
   srcs[URB_LOGICAL_SRC_COMPONENTS] = brw_imm_ud(comps);
            wbld.emit(SHADER_OPCODE_URB_WRITE_LOGICAL,
         }
      static void
   emit_urb_indirect_writes(const fs_builder &bld, nir_intrinsic_instr *instr,
               {
               const unsigned comps = nir_src_num_components(instr->src[0]);
            const unsigned base_in_dwords = nir_intrinsic_base(instr) +
            /* Use URB write message that allow different offsets per-slot.  The offset
   * is in units of vec4s (128 bits), so we use a write for each component,
   * replicating it in the sources and applying the appropriate mask based on
   * the dword offset.
            for (unsigned c = 0; c < comps; c++) {
      if (((1 << c) & nir_intrinsic_write_mask(instr)) == 0)
                     for (unsigned q = 0; q < bld.dispatch_width() / 8; q++) {
               /* offset is always positive, so signedness doesn't matter */
   assert(offset_src.type == BRW_REGISTER_TYPE_D ||
         fs_reg off = bld8.vgrf(offset_src.type, 1);
                                 fs_reg one = bld8.vgrf(BRW_REGISTER_TYPE_UD, 1);
   bld8.MOV(one, brw_imm_ud(1));
                                                         fs_reg srcs[URB_LOGICAL_NUM_SRCS];
   srcs[URB_LOGICAL_SRC_HANDLE] = urb_handle;
   srcs[URB_LOGICAL_SRC_PER_SLOT_OFFSETS] = off;
   srcs[URB_LOGICAL_SRC_CHANNEL_MASK] = mask;
   srcs[URB_LOGICAL_SRC_DATA] = fs_reg(VGRF, bld.shader->alloc.allocate(length),
                        fs_inst *inst = bld8.emit(SHADER_OPCODE_URB_WRITE_LOGICAL,
                  }
      static void
   emit_urb_direct_reads(const fs_builder &bld, nir_intrinsic_instr *instr,
         {
               unsigned comps = instr->def.num_components;
   if (comps == 0)
            nir_src *offset_nir_src = nir_get_io_offset_src(instr);
            const unsigned offset_in_dwords = nir_intrinsic_base(instr) +
                  unsigned urb_global_offset = offset_in_dwords / 4;
            const unsigned comp_offset = offset_in_dwords % 4;
            fs_builder ubld8 = bld.group(8, 0).exec_all();
   fs_reg data = ubld8.vgrf(BRW_REGISTER_TYPE_UD, num_regs);
   fs_reg srcs[URB_LOGICAL_NUM_SRCS];
            fs_inst *inst = ubld8.emit(SHADER_OPCODE_URB_READ_LOGICAL, data,
         inst->offset = urb_global_offset;
   assert(inst->offset < 2048);
            for (unsigned c = 0; c < comps; c++) {
      fs_reg dest_comp = offset(dest, bld, c);
   fs_reg data_comp = horiz_stride(offset(data, ubld8, comp_offset + c), 0);
         }
      static void
   emit_urb_direct_reads_xe2(const fs_builder &bld, nir_intrinsic_instr *instr,
         {
               unsigned comps = instr->def.num_components;
   if (comps == 0)
            nir_src *offset_nir_src = nir_get_io_offset_src(instr);
                     const unsigned offset_in_dwords = nir_intrinsic_base(instr) +
                  if (offset_in_dwords > 0) {
      fs_reg new_handle = ubld16.vgrf(BRW_REGISTER_TYPE_UD);
   ubld16.ADD(new_handle, urb_handle, brw_imm_ud(offset_in_dwords * 4));
               fs_reg data = ubld16.vgrf(BRW_REGISTER_TYPE_UD, comps);
   fs_reg srcs[URB_LOGICAL_NUM_SRCS];
            fs_inst *inst = ubld16.emit(SHADER_OPCODE_URB_READ_LOGICAL,
                  for (unsigned c = 0; c < comps; c++) {
      fs_reg dest_comp = offset(dest, bld, c);
   fs_reg data_comp = horiz_stride(offset(data, ubld16, c), 0);
         }
      static void
   emit_urb_indirect_reads(const fs_builder &bld, nir_intrinsic_instr *instr,
         {
               unsigned comps = instr->def.num_components;
   if (comps == 0)
            fs_reg seq_ud;
   {
      fs_builder ubld8 = bld.group(8, 0).exec_all();
   seq_ud = ubld8.vgrf(BRW_REGISTER_TYPE_UD, 1);
   fs_reg seq_uw = ubld8.vgrf(BRW_REGISTER_TYPE_UW, 1);
   ubld8.MOV(seq_uw, fs_reg(brw_imm_v(0x76543210)));
   ubld8.MOV(seq_ud, seq_uw);
               const unsigned base_in_dwords = nir_intrinsic_base(instr) +
            for (unsigned c = 0; c < comps; c++) {
      for (unsigned q = 0; q < bld.dispatch_width() / 8; q++) {
               /* offset is always positive, so signedness doesn't matter */
   assert(offset_src.type == BRW_REGISTER_TYPE_D ||
         fs_reg off = bld8.vgrf(offset_src.type, 1);
                           fs_reg comp = bld8.vgrf(BRW_REGISTER_TYPE_UD, 1);
   bld8.AND(comp, off, brw_imm_ud(0x3));
                           fs_reg srcs[URB_LOGICAL_NUM_SRCS];
                           fs_inst *inst = bld8.emit(SHADER_OPCODE_URB_READ_LOGICAL,
                        fs_reg dest_comp = offset(dest, bld, c);
   bld8.emit(SHADER_OPCODE_MOV_INDIRECT,
            retype(quarter(dest_comp, q), BRW_REGISTER_TYPE_UD),
   data,
         }
      static void
   emit_urb_indirect_reads_xe2(const fs_builder &bld, nir_intrinsic_instr *instr,
               {
               unsigned comps = instr->def.num_components;
   if (comps == 0)
                     const unsigned offset_in_dwords = nir_intrinsic_base(instr) +
            if (offset_in_dwords > 0) {
      fs_reg new_handle = ubld16.vgrf(BRW_REGISTER_TYPE_UD);
   ubld16.ADD(new_handle, urb_handle, brw_imm_ud(offset_in_dwords * 4));
                           for (unsigned q = 0; q < bld.dispatch_width() / 16; q++) {
               fs_reg addr = wbld.vgrf(BRW_REGISTER_TYPE_UD);
   wbld.SHL(addr, horiz_offset(offset_src, 16 * q), brw_imm_ud(2));
            fs_reg srcs[URB_LOGICAL_NUM_SRCS];
            fs_inst *inst = wbld.emit(SHADER_OPCODE_URB_READ_LOGICAL,
                  for (unsigned c = 0; c < comps; c++) {
      fs_reg dest_comp = horiz_offset(offset(dest, bld, c), 16 * q);
   fs_reg data_comp = offset(data, wbld, c);
            }
      void
   fs_visitor::emit_task_mesh_store(const fs_builder &bld, nir_intrinsic_instr *instr,
         {
      fs_reg src = get_nir_src(instr->src[0]);
            if (nir_src_is_const(*offset_nir_src)) {
      if (bld.shader->devinfo->ver >= 20)
         else
      } else {
      if (bld.shader->devinfo->ver >= 20) {
      emit_urb_indirect_writes_xe2(bld, instr, src, get_nir_src(*offset_nir_src), urb_handle);
      }
   bool use_mod = false;
            /* Try to calculate the value of (offset + base) % 4. If we can do
   * this, then we can do indirect writes using only 1 URB write.
   */
   use_mod = nir_mod_analysis(nir_get_scalar(offset_nir_src->ssa, 0), nir_type_uint, 4, &mod);
   if (use_mod) {
      mod += nir_intrinsic_base(instr) + component_from_intrinsic(instr);
               if (use_mod) {
         } else {
               }
      void
   fs_visitor::emit_task_mesh_load(const fs_builder &bld, nir_intrinsic_instr *instr,
         {
      fs_reg dest = get_nir_def(instr->def);
            /* TODO(mesh): for per_vertex and per_primitive, if we could keep around
   * the non-array-index offset, we could use to decide if we can perform
   * a single large aligned read instead one per component.
            if (nir_src_is_const(*offset_nir_src)) {
      if (bld.shader->devinfo->ver >= 20)
         else
      } else {
      if (bld.shader->devinfo->ver >= 20)
         else
         }
      void
   fs_visitor::nir_emit_task_intrinsic(const fs_builder &bld,
         {
      assert(stage == MESA_SHADER_TASK);
            switch (instr->intrinsic) {
   case nir_intrinsic_store_output:
   case nir_intrinsic_store_task_payload:
      emit_task_mesh_store(bld, instr, payload.urb_output);
         case nir_intrinsic_load_output:
   case nir_intrinsic_load_task_payload:
      emit_task_mesh_load(bld, instr, payload.urb_output);
         default:
      nir_emit_task_mesh_intrinsic(bld, instr);
         }
      void
   fs_visitor::nir_emit_mesh_intrinsic(const fs_builder &bld,
         {
      assert(stage == MESA_SHADER_MESH);
            switch (instr->intrinsic) {
   case nir_intrinsic_store_per_primitive_output:
   case nir_intrinsic_store_per_vertex_output:
   case nir_intrinsic_store_output:
      emit_task_mesh_store(bld, instr, payload.urb_output);
         case nir_intrinsic_load_per_vertex_output:
   case nir_intrinsic_load_per_primitive_output:
   case nir_intrinsic_load_output:
      emit_task_mesh_load(bld, instr, payload.urb_output);
         case nir_intrinsic_load_task_payload:
      emit_task_mesh_load(bld, instr, payload.task_urb_input);
         default:
      nir_emit_task_mesh_intrinsic(bld, instr);
         }
      void
   fs_visitor::nir_emit_task_mesh_intrinsic(const fs_builder &bld,
         {
      assert(stage == MESA_SHADER_MESH || stage == MESA_SHADER_TASK);
            fs_reg dest;
   if (nir_intrinsic_infos[instr->intrinsic].has_dest)
            switch (instr->intrinsic) {
   case nir_intrinsic_load_mesh_inline_data_intel: {
      fs_reg data = offset(payload.inline_parameter, 1, nir_intrinsic_align_offset(instr));
   bld.MOV(dest, retype(data, dest.type));
               case nir_intrinsic_load_draw_id:
      dest = retype(dest, BRW_REGISTER_TYPE_UD);
   bld.MOV(dest, payload.extended_parameter_0);
         case nir_intrinsic_load_local_invocation_id:
      unreachable("local invocation id should have been lowered earlier");
         case nir_intrinsic_load_local_invocation_index:
      dest = retype(dest, BRW_REGISTER_TYPE_UD);
   bld.MOV(dest, payload.local_index);
         case nir_intrinsic_load_num_workgroups:
      dest = retype(dest, BRW_REGISTER_TYPE_UD);
   bld.MOV(offset(dest, bld, 0), brw_uw1_grf(0, 13)); /* g0.6 >> 16 */
   bld.MOV(offset(dest, bld, 1), brw_uw1_grf(0, 8));  /* g0.4 & 0xffff */
   bld.MOV(offset(dest, bld, 2), brw_uw1_grf(0, 9));  /* g0.4 >> 16 */
         case nir_intrinsic_load_workgroup_index:
      dest = retype(dest, BRW_REGISTER_TYPE_UD);
   bld.MOV(dest, retype(brw_vec1_grf(0, 1), BRW_REGISTER_TYPE_UD));
         default:
      nir_emit_cs_intrinsic(bld, instr);
         }
