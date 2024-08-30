   /*
   * Copyright © 2022 Collabora Ltc.
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      #include "util/u_memory.h"
   #include "util/u_prim.h"
   #include "nir.h"
   #include "nir_builder.h"
   #include "nir_xfb_info.h"
      static unsigned int
   gs_in_prim_for_topology(enum mesa_prim prim)
   {
      switch (prim) {
   case MESA_PRIM_QUADS:
         default:
            }
      static enum mesa_prim
   gs_out_prim_for_topology(enum mesa_prim prim)
   {
      switch (prim) {
   case MESA_PRIM_POINTS:
         case MESA_PRIM_LINES:
   case MESA_PRIM_LINE_LOOP:
   case MESA_PRIM_LINES_ADJACENCY:
   case MESA_PRIM_LINE_STRIP_ADJACENCY:
   case MESA_PRIM_LINE_STRIP:
         case MESA_PRIM_TRIANGLES:
   case MESA_PRIM_TRIANGLE_STRIP:
   case MESA_PRIM_TRIANGLE_FAN:
   case MESA_PRIM_TRIANGLES_ADJACENCY:
   case MESA_PRIM_TRIANGLE_STRIP_ADJACENCY:
   case MESA_PRIM_POLYGON:
         case MESA_PRIM_QUADS:
   case MESA_PRIM_QUAD_STRIP:
   case MESA_PRIM_PATCHES:
   default:
            }
      static unsigned int
   vertices_for_prim(enum mesa_prim prim)
   {
      switch (prim) {
   case MESA_PRIM_POINTS:
         case MESA_PRIM_LINES:
   case MESA_PRIM_LINE_LOOP:
   case MESA_PRIM_LINES_ADJACENCY:
   case MESA_PRIM_LINE_STRIP_ADJACENCY:
   case MESA_PRIM_LINE_STRIP:
         case MESA_PRIM_TRIANGLES:
   case MESA_PRIM_TRIANGLE_STRIP:
   case MESA_PRIM_TRIANGLE_FAN:
   case MESA_PRIM_TRIANGLES_ADJACENCY:
   case MESA_PRIM_TRIANGLE_STRIP_ADJACENCY:
   case MESA_PRIM_POLYGON:
         case MESA_PRIM_QUADS:
   case MESA_PRIM_QUAD_STRIP:
         case MESA_PRIM_PATCHES:
   default:
            }
      static void
   copy_vars(nir_builder *b, nir_deref_instr *dst, nir_deref_instr *src)
   {
      assert(glsl_get_bare_type(dst->type) == glsl_get_bare_type(src->type));
   if (glsl_type_is_struct_or_ifc(dst->type)) {
      for (unsigned i = 0; i < glsl_get_length(dst->type); ++i) {
            } else if (glsl_type_is_array_or_matrix(dst->type)) {
      unsigned count = glsl_type_is_array(dst->type) ? glsl_array_size(dst->type) : glsl_get_matrix_columns(dst->type);
   for (unsigned i = 0; i < count; i++) {
            } else {
      nir_def *load = nir_load_deref(b, src);
         }
      /*
   * A helper to create a passthrough GS shader for drivers that needs to lower
   * some rendering tasks to the GS.
   */
      nir_shader *
   nir_create_passthrough_gs(const nir_shader_compiler_options *options,
                           const nir_shader *prev_stage,
   {
      unsigned int vertices_out = vertices_for_prim(primitive_type);
   emulate_edgeflags = emulate_edgeflags && (prev_stage->info.outputs_written & VARYING_BIT_EDGE);
   enum mesa_prim original_our_prim = gs_out_prim_for_topology(output_primitive_type);
   nir_builder b = nir_builder_init_simple_shader(MESA_SHADER_GEOMETRY,
                  bool output_lines = force_line_strip_out || original_our_prim == MESA_PRIM_LINE_STRIP;
            nir_shader *nir = b.shader;
   nir->info.gs.input_primitive = gs_in_prim_for_topology(primitive_type);
   nir->info.gs.output_primitive = force_line_strip_out ? MESA_PRIM_LINE_STRIP : original_our_prim;
   nir->info.gs.vertices_in = u_vertices_per_prim(primitive_type);
   nir->info.gs.vertices_out = needs_closing ? vertices_out + 1 : vertices_out;
   nir->info.gs.invocations = 1;
            nir->info.has_transform_feedback_varyings = prev_stage->info.has_transform_feedback_varyings;
   memcpy(nir->info.xfb_stride, prev_stage->info.xfb_stride, sizeof(prev_stage->info.xfb_stride));
   if (prev_stage->xfb_info) {
                  bool handle_flat = output_lines && nir->info.gs.output_primitive != gs_out_prim_for_topology(primitive_type);
   nir_variable *in_vars[VARYING_SLOT_MAX * 4];
   nir_variable *out_vars[VARYING_SLOT_MAX * 4];
            /* Create input/output variables. */
   nir_foreach_shader_out_variable(var, prev_stage) {
               /* input vars can't be created for those */
   if (var->data.location == VARYING_SLOT_LAYER ||
                  char name[100];
   if (var->name)
         else
            nir_variable *in = nir_variable_clone(var, nir);
   ralloc_free(in->name);
   in->name = ralloc_strdup(in, name);
   in->type = glsl_array_type(var->type, 6, false);
   in->data.mode = nir_var_shader_in;
                     nir->num_inputs++;
   if (in->data.location == VARYING_SLOT_EDGE)
            if (var->data.location != VARYING_SLOT_POS)
            if (var->name)
         else
            nir_variable *out = nir_variable_clone(var, nir);
   ralloc_free(out->name);
   out->name = ralloc_strdup(out, name);
   out->data.mode = nir_var_shader_out;
                        unsigned int start_vert = 0;
   unsigned int end_vert = vertices_out;
   unsigned int vert_step = 1;
   switch (primitive_type) {
   case MESA_PRIM_LINES_ADJACENCY:
   case MESA_PRIM_LINE_STRIP_ADJACENCY:
      start_vert = 1;
   end_vert += 1;
      case MESA_PRIM_TRIANGLES_ADJACENCY:
   case MESA_PRIM_TRIANGLE_STRIP_ADJACENCY:
      end_vert = 5;
   vert_step = 2;
      default:
                  nir_variable *edge_var = nir_find_variable_with_location(nir, nir_var_shader_in, VARYING_SLOT_EDGE);
   nir_def *flat_interp_mask_def = nir_load_flat_mask(&b);
   nir_def *last_pv_vert_def = nir_load_provoking_last(&b);
   last_pv_vert_def = nir_ine_imm(&b, last_pv_vert_def, 0);
   nir_def *start_vert_index = nir_imm_int(&b, start_vert);
   nir_def *end_vert_index = nir_imm_int(&b, end_vert - 1);
   nir_def *pv_vert_index = nir_bcsel(&b, last_pv_vert_def, end_vert_index, start_vert_index);
   for (unsigned i = start_vert; i < end_vert || needs_closing; i += vert_step) {
      int idx = i < end_vert ? i : start_vert;
   /* Copy inputs to outputs. */
   for (unsigned j = 0, oj = 0, of = 0; j < num_inputs; ++j) {
      if (in_vars[j]->data.location == VARYING_SLOT_EDGE) {
         }
   /* no need to use copy_var to save a lower pass */
   nir_def *index;
   if (in_vars[j]->data.location == VARYING_SLOT_POS || !handle_flat)
         else {
      unsigned mask = 1u << (of++);
      }
   nir_deref_instr *value = nir_build_deref_array(&b, nir_build_deref_var(&b, in_vars[j]), index);
   copy_vars(&b, nir_build_deref_var(&b, out_vars[oj]), value);
               if (emulate_edgeflags && !output_lines) {
      nir_def *edge_value = nir_channel(&b, nir_load_array_var_imm(&b, edge_var, idx), 0);
                        if (emulate_edgeflags) {
      if (nir->info.gs.output_primitive == MESA_PRIM_LINE_STRIP) {
      nir_def *edge_value = nir_channel(&b, nir_load_array_var_imm(&b, edge_var, idx), 0);
   nir_push_if(&b, nir_fneu_imm(&b, edge_value, 1.0));
   nir_end_primitive(&b, 0);
      }
               if (i >= end_vert)
               nir_end_primitive(&b, 0);
   nir_shader_gather_info(nir, nir_shader_get_entrypoint(nir));
               }
