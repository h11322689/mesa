   /*
   * Copyright © 2022 Intel Corporation
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
      #include "anv_private.h"
   #include "nir_builder.h"
      /*
   * Wa_18019110168 for gfx 12.5.
   *
   * This file implements workaround for HW bug, which leads to fragment shader
   * reading incorrect per-primitive data if mesh shader, in addition to writing
   * per-primitive data, also writes to gl_ClipDistance.
   *
   * The suggested solution to that bug is to not use per-primitive data by:
   * - creating new vertices for provoking vertices shared by multiple primitives
   * - converting per-primitive attributes read by fragment shader to flat
   *   per-vertex attributes for the provoking vertex
   * - modifying fragment shader to read those per-vertex attributes
   *
   * There are at least 2 type of failures not handled very well:
   * - if the number of varying slots overflows, than only some attributes will
   *   be converted, leading to corruption of those unconverted attributes
   * - if the overall MUE size is so large it doesn't fit in URB, then URB
   *   allocation will fail in some way; unfortunately there's no good way to
   *   say how big MUE will be at this moment and back out
   *
   * This workaround needs to be applied before linking, so that unused outputs
   * created by this code are removed at link time.
   *
   * This workaround can be controlled by a driconf option to either disable it,
   * lower its scope or force enable it.
   *
   * Option "anv_mesh_conv_prim_attrs_to_vert_attrs" is evaluated like this:
   *  value == 0 - disable workaround
   *  value < 0 - enable ONLY if workaround is required
   *  value > 0 - enable ALWAYS, even if it's not required
   *  abs(value) >= 1 - attribute conversion
   *  abs(value) >= 2 - attribute conversion and vertex duplication
   *
   *  Default: -2 (both parts of the work around, ONLY if it's required)
   *
   */
      static bool
   anv_mesh_convert_attrs_prim_to_vert(struct nir_shader *nir,
                                       {
      uint64_t per_primitive_outputs = nir->info.per_primitive_outputs;
            if (per_primitive_outputs == 0)
            uint64_t outputs_written = nir->info.outputs_written;
            if ((other_outputs & (VARYING_BIT_CLIP_DIST0 | VARYING_BIT_CLIP_DIST1)) == 0)
      if (!force_conversion)
         uint64_t all_outputs = outputs_written;
            uint64_t remapped_outputs = outputs_written & per_primitive_outputs;
            /* Skip locations not read by the fragment shader, because they will
   * be eliminated at linking time. Note that some fs inputs may be
   * removed only after optimizations, so it's possible that we will
   * create too many variables.
   */
            /* Figure out the mapping between per-primitive and new per-vertex outputs. */
   nir_foreach_shader_out_variable(var, nir) {
               if (!(BITFIELD64_BIT(location) & remapped_outputs))
            /* Although primitive shading rate, layer and viewport have predefined
   * place in MUE Primitive Header (so we can't really move them anywhere),
   * we have to copy them to per-vertex space if fragment shader reads them.
   */
   assert(location == VARYING_SLOT_PRIMITIVE_SHADING_RATE ||
         location == VARYING_SLOT_LAYER ||
   location == VARYING_SLOT_VIEWPORT ||
            const struct glsl_type *type = var->type;
   if (nir_is_arrayed_io(var, MESA_SHADER_MESH) || var->data.per_view) {
      assert(glsl_type_is_array(type));
                        for (gl_varying_slot slot = VARYING_SLOT_VAR0; slot <= VARYING_SLOT_VAR31; slot++) {
      uint64_t mask = BITFIELD64_MASK(num_slots) << slot;
   if ((all_outputs & mask) == 0) {
      wa_mapping[location] = slot;
   all_outputs |= mask;
   attrs++;
                  if (wa_mapping[location] == 0) {
      fprintf(stderr, "Not enough space for hardware per-primitive data corruption work around.\n");
                  if (attrs == 0)
      if (!force_conversion)
                  const VkPipelineRasterizationStateCreateInfo *rs_info = pCreateInfo->pRasterizationState;
   const VkPipelineRasterizationProvokingVertexStateCreateInfoEXT *rs_pv_info =
         if (rs_pv_info && rs_pv_info->provokingVertexMode == VK_PROVOKING_VERTEX_MODE_LAST_VERTEX_EXT)
            unsigned vertices_per_primitive =
            nir_function_impl *impl = nir_shader_get_entrypoint(nir);
            /* wait for all subgroups to finish */
                              nir_def *cmp = nir_ieq(&b, local_invocation_index, zero);
   nir_if *if_stmt = nir_push_if(&b, cmp);
   {
      nir_variable *primitive_count_var = NULL;
            unsigned num_other_variables = 0;
   nir_foreach_shader_out_variable(var, b.shader) {
      if ((BITFIELD64_BIT(var->data.location) & other_outputs) == 0)
                     nir_deref_instr **per_vertex_derefs =
                     unsigned processed = 0;
   nir_foreach_shader_out_variable(var, b.shader) {
                     switch (var->data.location) {
      case VARYING_SLOT_PRIMITIVE_COUNT:
      primitive_count_var = var;
      case VARYING_SLOT_PRIMITIVE_INDICES:
      primitive_indices_var = var;
      default: {
      const struct glsl_type *type = var->type;
                        if (dup_vertices) {
      /*
   * Resize type of array output to make space for one extra
   * vertex attribute for each primitive, so we ensure that
   * the provoking vertex is not shared between primitives.
   */
   const struct glsl_type *new_type =
                                          per_vertex_derefs[num_per_vertex_variables++] =
                           }
            assert(primitive_count_var != NULL);
            /* Update types of derefs to match type of variables they (de)reference. */
   if (dup_vertices) {
      nir_foreach_function_impl(impl, b.shader) {
      nir_foreach_block(block, impl) {
                                                   if (deref->var->type != deref->type)
                        /* indexed by slot of per-prim attribute */
   struct {
      nir_deref_instr *per_prim_deref;
               /* Create new per-vertex output variables mirroring per-primitive variables
   * and create derefs for both old and new variables.
   */
   nir_foreach_shader_out_variable(var, b.shader) {
               if ((BITFIELD64_BIT(location) & (outputs_written & per_primitive_outputs)) == 0)
                        const struct glsl_type *type = var->type;
                  const struct glsl_type *new_type =
         glsl_array_type(array_element_type,
                  nir_variable *new_var =
         assert(wa_mapping[location] >= VARYING_SLOT_VAR0);
   assert(wa_mapping[location] <= VARYING_SLOT_VAR31);
                  mapping[location].per_vert_deref = nir_build_deref_var(&b, new_var);
                        /*
   * for each Primitive (0 : primitiveCount)
   *    if VertexUsed[PrimitiveIndices[Primitive][provoking vertex]]
   *       create 1 new vertex at offset "Vertex"
   *       copy per vert attributes of provoking vertex to the new one
   *       update PrimitiveIndices[Primitive][provoking vertex]
   *       Vertex++
   *    else
   *       VertexUsed[PrimitiveIndices[Primitive][provoking vertex]] := true
   *
   *    for each attribute : mapping
   *       copy per_prim_attr(Primitive) to per_vert_attr[Primitive][provoking vertex]
            /* primitive count */
            /* primitive index */
   nir_variable *primitive_var =
         nir_deref_instr *primitive_deref = nir_build_deref_var(&b, primitive_var);
            /* vertex index */
   nir_variable *vertex_var =
         nir_deref_instr *vertex_deref = nir_build_deref_var(&b, vertex_var);
            /* used vertices bitvector */
   const struct glsl_type *used_vertex_type =
         glsl_array_type(glsl_bool_type(),
         nir_variable *used_vertex_var =
         nir_deref_instr *used_vertex_deref =
         /* Initialize it as "not used" */
   for (unsigned i = 0; i < nir->info.mesh.max_vertices_out; ++i) {
      nir_deref_instr *indexed_used_vertex_deref =
                     nir_loop *loop = nir_push_loop(&b);
   {
                     nir_if *loop_check = nir_push_if(&b, cmp);
                  nir_deref_instr *primitive_indices_deref =
         nir_deref_instr *indexed_primitive_indices_deref;
                  /* array of vectors, we have to extract index out of array deref */
   indexed_primitive_indices_deref = nir_build_deref_array(&b, primitive_indices_deref, primitive);
                           nir_deref_instr *indexed_used_vertex_deref =
         nir_def *used_vertex = nir_load_deref(&b, indexed_used_vertex_deref);
                  nir_if *vertex_used_check = nir_push_if(&b, used_vertex);
   {
      for (unsigned a = 0; a < num_per_vertex_variables; ++a) {
                                       /* replace one component of primitive indices vector */
                  /* and store complete vector */
                           for (unsigned i = 0; i < ARRAY_SIZE(mapping); ++i) {
                     nir_deref_instr *src =
                              }
   nir_push_else(&b, vertex_used_check);
                     for (unsigned i = 0; i < ARRAY_SIZE(mapping); ++i) {
                     nir_deref_instr *src =
                                                   }
      }
            if (dup_vertices)
            if (should_print_nir(nir)) {
      printf("%s\n", __func__);
               /* deal with copy_derefs */
   NIR_PASS(_, nir, nir_split_var_copies);
                        }
      static bool
   anv_frag_update_derefs_instr(struct nir_builder *b, nir_instr *instr, void *data)
   {
      if (instr->type != nir_instr_type_deref)
            nir_deref_instr *deref = nir_instr_as_deref(instr);
   if (deref->deref_type != nir_deref_type_var)
            nir_variable *var = deref->var;
   if (!(var->data.mode & nir_var_shader_in))
            int location = var->data.location;
   nir_deref_instr **new_derefs = (nir_deref_instr **)data;
   if (new_derefs[location] == NULL)
            nir_instr_remove(&deref->instr);
               }
      static bool
   anv_frag_update_derefs(nir_shader *shader, nir_deref_instr **mapping)
   {
      return nir_shader_instructions_pass(shader, anv_frag_update_derefs_instr,
      }
      /* Update fragment shader inputs with new ones. */
   static void
   anv_frag_convert_attrs_prim_to_vert(struct nir_shader *nir,
         {
      /* indexed by slot of per-prim attribute */
            nir_function_impl *impl = nir_shader_get_entrypoint(nir);
            nir_foreach_shader_in_variable_safe(var, nir) {
      gl_varying_slot location = var->data.location;
   gl_varying_slot new_location = wa_mapping[location];
   if (new_location == 0)
                     nir_variable *new_var =
         new_var->data.location = new_location;
   new_var->data.location_frac = var->data.location_frac;
                                    }
      void
   anv_apply_per_prim_attr_wa(struct nir_shader *ms_nir,
                     {
               int mesh_conv_prim_attrs_to_vert_attrs =
         if (mesh_conv_prim_attrs_to_vert_attrs < 0 &&
                  if (mesh_conv_prim_attrs_to_vert_attrs != 0) {
      uint64_t fs_inputs = 0;
   nir_foreach_shader_in_variable(var, fs_nir)
                              const bool dup_vertices = abs(mesh_conv_prim_attrs_to_vert_attrs) >= 2;
            if (anv_mesh_convert_attrs_prim_to_vert(ms_nir, wa_mapping,
                              }
