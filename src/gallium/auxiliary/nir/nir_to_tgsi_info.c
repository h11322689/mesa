   /*
   * Copyright 2017 Advanced Micro Devices, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   */
      /*
   * This is ported mostly out of radeonsi, if we can drop TGSI, we can likely
   * make a lot this go away.
   */
      #include "nir_to_tgsi_info.h"
   #include "util/u_math.h"
   #include "util/u_prim.h"
   #include "nir.h"
   #include "nir_deref.h"
   #include "tgsi/tgsi_scan.h"
   #include "tgsi/tgsi_from_mesa.h"
      static nir_variable* tex_get_texture_var(const nir_tex_instr *instr)
   {
      for (unsigned i = 0; i < instr->num_srcs; i++) {
      switch (instr->src[i].src_type) {
   case nir_tex_src_texture_deref:
         default:
                        }
      static void gather_usage_helper(const nir_deref_instr **deref_ptr,
                     {
      for (; *deref_ptr; deref_ptr++) {
      const nir_deref_instr *deref = *deref_ptr;
   switch (deref->deref_type) {
   case nir_deref_type_array: {
      bool is_compact = nir_deref_instr_get_variable(deref)->data.compact;
   unsigned elem_size = is_compact ? DIV_ROUND_UP(glsl_get_length(deref->type), 4) :
         if (nir_src_is_const(deref->arr.index)) {
      if (is_compact) {
      location += nir_src_as_uint(deref->arr.index) / 4;
      } else
      } else {
      unsigned array_elems =
         for (unsigned i = 0; i < array_elems; i++) {
      gather_usage_helper(deref_ptr + 1,
            }
      }
      }
   case nir_deref_type_struct: {
      const struct glsl_type *parent_type =
         unsigned index = deref->strct.index;
   for (unsigned i = 0; i < index; i++) {
      const struct glsl_type *ft = glsl_get_struct_field(parent_type, i);
      }
      }
   default:
                     usage_mask[location] |= mask & 0xf;
   if (mask & 0xf0)
      }
      static void gather_usage(const nir_deref_instr *deref,
               {
      nir_deref_path path;
            unsigned location_frac = path.path[0]->var->data.location_frac;
   if (glsl_type_is_64bit(deref->type)) {
      uint8_t new_mask = 0;
   for (unsigned i = 0; i < 4; i++) {
      if (mask & (1 << i))
      }
      } else {
      mask <<= location_frac;
               gather_usage_helper((const nir_deref_instr **)&path.path[1],
                     }
      static void gather_intrinsic_load_deref_info(const nir_shader *nir,
                                 {
               if (nir->info.stage == MESA_SHADER_FRAGMENT)
      gather_usage(deref, nir_def_components_read(&instr->def),
         switch (nir->info.stage) {
                  }
   default: {
      unsigned semantic_name, semantic_index;
   tgsi_get_gl_varying_semantic(var->data.location, need_texcoord,
            if (semantic_name == TGSI_SEMANTIC_FACE) {
         }
      }
      }
      static void scan_instruction(const struct nir_shader *nir,
                     {
               if (instr->type == nir_instr_type_tex) {
               switch (tex->op) {
   case nir_texop_tex:
      info->opcode_count[TGSI_OPCODE_TEX]++;
      default:
            } else if (instr->type == nir_instr_type_intrinsic) {
               switch (intr->intrinsic) {
   case nir_intrinsic_load_front_face:
      info->uses_frontface = 1;
      case nir_intrinsic_load_instance_id:
      info->uses_instanceid = 1;
      case nir_intrinsic_load_invocation_id:
      info->uses_invocationid = true;
      case nir_intrinsic_load_num_workgroups:
      info->uses_grid_size = true;
      case nir_intrinsic_load_vertex_id:
      info->uses_vertexid = 1;
      case nir_intrinsic_load_vertex_id_zero_base:
      info->uses_vertexid_nobase = 1;
      case nir_intrinsic_load_base_vertex:
      info->uses_basevertex = 1;
      case nir_intrinsic_load_primitive_id:
      info->uses_primid = 1;
      case nir_intrinsic_bindless_image_store:
      info->writes_memory = true;
      case nir_intrinsic_image_deref_store:
   case nir_intrinsic_image_store:
      info->writes_memory = true;
      case nir_intrinsic_bindless_image_atomic:
   case nir_intrinsic_bindless_image_atomic_swap:
      info->writes_memory = true;
      case nir_intrinsic_image_deref_atomic:
   case nir_intrinsic_image_deref_atomic_swap:
   case nir_intrinsic_image_atomic:
   case nir_intrinsic_image_atomic_swap:
      info->writes_memory = true;
      case nir_intrinsic_store_ssbo:
   case nir_intrinsic_ssbo_atomic:
   case nir_intrinsic_ssbo_atomic_swap:
      info->writes_memory = true;
      case nir_intrinsic_interp_deref_at_centroid:
   case nir_intrinsic_interp_deref_at_offset:
   case nir_intrinsic_interp_deref_at_sample:
   case nir_intrinsic_interp_deref_at_vertex:
   case nir_intrinsic_load_deref: {
      const nir_variable *var = nir_intrinsic_get_var(intr, 0);
                  if (nir_deref_instr_has_indirect(deref)) {
      if (mode == nir_var_shader_in)
      }
   if (mode == nir_var_shader_in)
            }
   default:
               }
      void nir_tgsi_scan_shader(const struct nir_shader *nir,
               {
               info->processor = pipe_shader_type_from_mesa(nir->info.stage);
            info->properties[TGSI_PROPERTY_NEXT_SHADER] =
            if (nir->info.stage == MESA_SHADER_VERTEX) {
      info->properties[TGSI_PROPERTY_VS_WINDOW_SPACE_POSITION] =
               if (nir->info.stage == MESA_SHADER_TESS_CTRL) {
      info->properties[TGSI_PROPERTY_TCS_VERTICES_OUT] =
               if (nir->info.stage == MESA_SHADER_TESS_EVAL) {
               STATIC_ASSERT((TESS_SPACING_EQUAL + 1) % 3 == PIPE_TESS_SPACING_EQUAL);
   STATIC_ASSERT((TESS_SPACING_FRACTIONAL_ODD + 1) % 3 ==
         STATIC_ASSERT((TESS_SPACING_FRACTIONAL_EVEN + 1) % 3 ==
            info->properties[TGSI_PROPERTY_TES_SPACING] = (nir->info.tess.spacing + 1) % 3;
   info->properties[TGSI_PROPERTY_TES_VERTEX_ORDER_CW] = !nir->info.tess.ccw;
               if (nir->info.stage == MESA_SHADER_GEOMETRY) {
      info->properties[TGSI_PROPERTY_GS_INPUT_PRIM] = nir->info.gs.input_primitive;
   info->properties[TGSI_PROPERTY_GS_OUTPUT_PRIM] = nir->info.gs.output_primitive;
   info->properties[TGSI_PROPERTY_GS_MAX_OUTPUT_VERTICES] = nir->info.gs.vertices_out;
               if (nir->info.stage == MESA_SHADER_FRAGMENT) {
      info->properties[TGSI_PROPERTY_FS_EARLY_DEPTH_STENCIL] =
         info->properties[TGSI_PROPERTY_FS_POST_DEPTH_COVERAGE] = nir->info.fs.post_depth_coverage;
            if (nir->info.fs.pixel_center_integer) {
      info->properties[TGSI_PROPERTY_FS_COORD_PIXEL_CENTER] =
               if (nir->info.fs.depth_layout != FRAG_DEPTH_LAYOUT_NONE) {
      switch (nir->info.fs.depth_layout) {
   case FRAG_DEPTH_LAYOUT_ANY:
      info->properties[TGSI_PROPERTY_FS_DEPTH_LAYOUT] = TGSI_FS_DEPTH_LAYOUT_ANY;
      case FRAG_DEPTH_LAYOUT_GREATER:
      info->properties[TGSI_PROPERTY_FS_DEPTH_LAYOUT] = TGSI_FS_DEPTH_LAYOUT_GREATER;
      case FRAG_DEPTH_LAYOUT_LESS:
      info->properties[TGSI_PROPERTY_FS_DEPTH_LAYOUT] = TGSI_FS_DEPTH_LAYOUT_LESS;
      case FRAG_DEPTH_LAYOUT_UNCHANGED:
      info->properties[TGSI_PROPERTY_FS_DEPTH_LAYOUT] = TGSI_FS_DEPTH_LAYOUT_UNCHANGED;
      default:
                        if (gl_shader_stage_is_compute(nir->info.stage) ||
      gl_shader_stage_is_mesh(nir->info.stage)) {
   info->properties[TGSI_PROPERTY_CS_FIXED_BLOCK_WIDTH] = nir->info.workgroup_size[0];
   info->properties[TGSI_PROPERTY_CS_FIXED_BLOCK_HEIGHT] = nir->info.workgroup_size[1];
               i = 0;
   uint64_t processed_inputs = 0;
   nir_foreach_shader_in_variable(variable, nir) {
               const struct glsl_type *type = variable->type;
   if (nir_is_arrayed_io(variable, nir->info.stage)) {
      assert(glsl_type_is_array(type));
                                 /* Vertex shader inputs don't have semantics. The state
   * tracker has already mapped them to attributes via
   * variable->data.driver_location.
   */
   if (nir->info.stage == MESA_SHADER_VERTEX) {
                                                                                                                  if (variable->data.centroid)
                        switch (variable->data.interpolation) {
   case INTERP_MODE_NONE:
      if (glsl_base_type_is_integer(base_type) || variable->data.per_primitive) {
                        if (semantic_name == TGSI_SEMANTIC_COLOR) {
      info->input_interpolate[i] = TGSI_INTERPOLATE_COLOR;
                                                                              case INTERP_MODE_FLAT:
      info->input_interpolate[i] = TGSI_INTERPOLATE_CONSTANT;
                     info->num_inputs = nir->num_inputs;
   if (nir->info.io_lowered) {
      info->num_inputs = util_bitcount64(nir->info.inputs_read);
   if (nir->info.inputs_read_indirectly)
            } else {
      int max = info->file_max[TGSI_FILE_INPUT] = -1;
   nir_foreach_shader_in_variable(var, nir) {
      int slots = glsl_count_attribute_slots(var->type, false);
   int tmax = var->data.driver_location + slots - 1;
   if (tmax > max)
                        i = 0;
   uint64_t processed_outputs = 0;
   unsigned num_outputs = 0;
   nir_foreach_shader_out_variable(variable, nir) {
                        const struct glsl_type *type = variable->type;
   if (nir_is_arrayed_io(variable, nir->info.stage)) {
      assert(glsl_type_is_array(type));
               unsigned attrib_count = nir_variable_count_slots(variable, type);
               if (nir->info.stage == MESA_SHADER_FRAGMENT) {
                     /* Adjust for dual source blending */
   if (variable->data.index > 0) {
            } else {
      tgsi_get_gl_varying_semantic(variable->data.location + k, need_texcoord,
               unsigned num_components = 4;
   unsigned vector_elements = glsl_get_vector_elements(glsl_without_array(variable->type));
                  unsigned component = variable->data.location_frac;
   if (glsl_type_is_64bit(glsl_without_array(variable->type))) {
      if (glsl_type_is_dual_slot(glsl_without_array(variable->type)) && k % 2) {
      num_components = (num_components * 2) - 4;
      } else {
                     uint8_t usagemask = 0;
   for (unsigned j = component; j < num_components + component; j++) {
      switch (j) {
   case 0:
      usagemask |= TGSI_WRITEMASK_X;
      case 1:
      usagemask |= TGSI_WRITEMASK_Y;
      case 2:
      usagemask |= TGSI_WRITEMASK_Z;
      case 3:
      usagemask |= TGSI_WRITEMASK_W;
      default:
                     unsigned gs_out_streams;
   if (variable->data.stream & NIR_STREAM_PACKED) {
         } else {
      assert(variable->data.stream < 4);
   gs_out_streams = 0;
   for (unsigned j = 0; j < num_components; ++j)
               unsigned streamx = gs_out_streams & 3;
   unsigned streamy = (gs_out_streams >> 2) & 3;
                  if (usagemask & TGSI_WRITEMASK_X) {
      info->output_usagemask[i] |= TGSI_WRITEMASK_X;
   info->output_streams[i] |= streamx;
      }
   if (usagemask & TGSI_WRITEMASK_Y) {
      info->output_usagemask[i] |= TGSI_WRITEMASK_Y;
   info->output_streams[i] |= streamy << 2;
      }
   if (usagemask & TGSI_WRITEMASK_Z) {
      info->output_usagemask[i] |= TGSI_WRITEMASK_Z;
   info->output_streams[i] |= streamz << 4;
      }
   if (usagemask & TGSI_WRITEMASK_W) {
      info->output_usagemask[i] |= TGSI_WRITEMASK_W;
   info->output_streams[i] |= streamw << 6;
               /* make sure we only count this location once against
   * the num_outputs counter.
   */
                                                switch (semantic_name) {
   case TGSI_SEMANTIC_VIEWPORT_INDEX:
      info->writes_viewport_index = true;
      case TGSI_SEMANTIC_LAYER:
      info->writes_layer = true;
      case TGSI_SEMANTIC_PSIZE:
      info->writes_psize = true;
      case TGSI_SEMANTIC_CLIPVERTEX:
      info->writes_clipvertex = true;
      case TGSI_SEMANTIC_STENCIL:
      if (!variable->data.fb_fetch_output)
            case TGSI_SEMANTIC_SAMPLEMASK:
      info->writes_samplemask = true;
      case TGSI_SEMANTIC_EDGEFLAG:
      info->writes_edgeflag = true;
      case TGSI_SEMANTIC_POSITION:
      if (info->processor == PIPE_SHADER_FRAGMENT) {
      if (!variable->data.fb_fetch_output)
      } else {
         }
               if (nir->info.stage == MESA_SHADER_TESS_CTRL) {
      switch (semantic_name) {
   case TGSI_SEMANTIC_PATCH:
      info->reads_perpatch_outputs = true;
      case TGSI_SEMANTIC_TESSINNER:
   case TGSI_SEMANTIC_TESSOUTER:
      info->reads_tessfactor_outputs = true;
      default:
                        unsigned loc = variable->data.location;
   if (nir->info.stage == MESA_SHADER_FRAGMENT &&
      loc == FRAG_RESULT_COLOR &&
   nir->info.outputs_written & (1ull << loc)) {
   assert(attrib_count == 1);
                  if (nir->info.io_lowered) {
               while (outputs_written) {
      unsigned location = u_bit_scan64(&outputs_written);
   unsigned i = util_bitcount64(nir->info.outputs_written &
                                 info->output_semantic_name[i] = semantic_name;
   info->output_semantic_index[i] = semantic_index;
      }
   num_outputs = util_bitcount64(nir->info.outputs_written);
   if (nir->info.outputs_accessed_indirectly)
                        info->const_file_max[0] = nir->num_uniforms - 1;
   info->images_declared = nir->info.images_used[0];
            info->file_max[TGSI_FILE_SAMPLER] = BITSET_LAST_BIT(nir->info.samplers_used) - 1;
   info->file_max[TGSI_FILE_SAMPLER_VIEW] = BITSET_LAST_BIT(nir->info.textures_used) - 1;
   info->file_mask[TGSI_FILE_SAMPLER] = nir->info.samplers_used[0];
   info->file_mask[TGSI_FILE_SAMPLER_VIEW] = nir->info.textures_used[0];
   info->file_max[TGSI_FILE_IMAGE] = BITSET_LAST_BIT(nir->info.images_used) - 1;
            info->num_written_clipdistance = nir->info.clip_distance_array_size;
            if (info->processor == PIPE_SHADER_FRAGMENT)
            nir_function *func = (struct nir_function *)
            nir_foreach_block(block, func->impl) {
      nir_foreach_instr(instr, block)
         }
