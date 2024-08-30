   /*
   * Copyright © Microsoft Corporation
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
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "dxil_signature.h"
   #include "dxil_enums.h"
   #include "dxil_module.h"
      #include "glsl_types.h"
   #include "nir_to_dxil.h"
   #include "util/u_debug.h"
      #include <string.h>
         struct semantic_info {
      enum dxil_semantic_kind kind;
   char name[64];
   int index;
   enum dxil_prog_sig_comp_type comp_type;
   uint8_t sig_comp_type;
   int32_t start_row;
   int32_t rows;
   uint8_t start_col;
   uint8_t cols;
   uint8_t interpolation;
   uint8_t stream;
      };
         static bool
   is_depth_output(enum dxil_semantic_kind kind)
   {
      return kind == DXIL_SEM_DEPTH || kind == DXIL_SEM_DEPTH_GE ||
      }
      static uint8_t
   get_interpolation(nir_variable *var)
   {
      if (var->data.patch)
            if (glsl_type_is_integer(glsl_without_array_or_matrix(var->type)))
            if (var->data.sample) {
      if (var->data.location == VARYING_SLOT_POS)
         switch (var->data.interpolation) {
   case INTERP_MODE_NONE: return DXIL_INTERP_LINEAR_SAMPLE;
   case INTERP_MODE_FLAT: return DXIL_INTERP_CONSTANT;
   case INTERP_MODE_NOPERSPECTIVE: return DXIL_INTERP_LINEAR_NOPERSPECTIVE_SAMPLE;
   case INTERP_MODE_SMOOTH: return DXIL_INTERP_LINEAR_SAMPLE;
      } else if (unlikely(var->data.centroid)) {
      if (var->data.location == VARYING_SLOT_POS)
         switch (var->data.interpolation) {
   case INTERP_MODE_NONE: return DXIL_INTERP_LINEAR_CENTROID;
   case INTERP_MODE_FLAT: return DXIL_INTERP_CONSTANT;
   case INTERP_MODE_NOPERSPECTIVE: return DXIL_INTERP_LINEAR_NOPERSPECTIVE_CENTROID;
   case INTERP_MODE_SMOOTH: return DXIL_INTERP_LINEAR_CENTROID;
      } else {
      if (var->data.location == VARYING_SLOT_POS)
         switch (var->data.interpolation) {
   case INTERP_MODE_NONE: return DXIL_INTERP_LINEAR;
   case INTERP_MODE_FLAT: return DXIL_INTERP_CONSTANT;
   case INTERP_MODE_NOPERSPECTIVE: return DXIL_INTERP_LINEAR_NOPERSPECTIVE;
   case INTERP_MODE_SMOOTH: return DXIL_INTERP_LINEAR;
                  }
      static const char *
   in_sysvalue_name(nir_variable *var)
   {
      switch (var->data.location) {
   case VARYING_SLOT_POS:
         case VARYING_SLOT_FACE:
         case VARYING_SLOT_LAYER:
         default:
            }
      /*
   * The signatures are written into the stream in two pieces:
   * DxilProgramSignatureElement is a fixes size structure that gets dumped
   * to the stream in order of the registers and each contains an offset
   * to the semantic name string. Then these strings are dumped into the stream.
   */
   static unsigned
   get_additional_semantic_info(nir_shader *s, nir_variable *var, struct semantic_info *info,
         {
      const struct glsl_type *type = var->type;
   if (nir_is_arrayed_io(var, s->info.stage))
            info->comp_type =
                     if (!glsl_type_is_struct(glsl_without_array(type))) {
         } else {
      /* For structs, just emit them as float registers. This way, they can be
   * interpolated or not, and it doesn't matter, and it avoids linking issues
   * that we'd see if the type here tried to depend on (e.g.) interp mode. */
   info->sig_comp_type = DXIL_COMP_TYPE_F32;
               bool is_gs_input = s->info.stage == MESA_SHADER_GEOMETRY &&
            info->stream = var->data.stream;
   info->rows = 1;
   if (info->kind == DXIL_SEM_TARGET) {
      info->start_row = info->index;
      } else if (is_depth ||
            (info->kind == DXIL_SEM_PRIMITIVE_ID && is_gs_input) ||
   info->kind == DXIL_SEM_COVERAGE ||
   // This turns into a 'N/A' mask in the disassembly
   info->start_row = -1;
      } else if (info->kind == DXIL_SEM_TESS_FACTOR ||
            assert(var->data.compact);
   info->start_row = next_row;
   info->rows = glsl_get_aoa_size(type);
   info->cols = 1;
      } else if (var->data.compact) {
      info->start_row = next_row;
            assert(glsl_type_is_array(type) && info->kind == DXIL_SEM_CLIP_DISTANCE);
   unsigned num_floats = glsl_get_aoa_size(type);
   unsigned start_offset = (var->data.location - VARYING_SLOT_CLIP_DIST0) * 4 +
            if (start_offset >= clip_size) {
      info->kind = DXIL_SEM_CULL_DISTANCE;
      }
   info->cols = num_floats;
      } else {
      info->start_row = next_row;
   info->rows = glsl_count_vec4_slots(type, false, false);
   if (glsl_type_is_array(type))
         next_row += info->rows;
   info->start_col = (uint8_t)var->data.location_frac;
                  }
      typedef void (*semantic_info_proc)(nir_variable *var, struct semantic_info *info, gl_shader_stage stage);
      static void
   get_semantic_vs_in_name(nir_variable *var, struct semantic_info *info, gl_shader_stage stage)
   {
      strcpy(info->name, "TEXCOORD");
   info->index = var->data.driver_location;
      }
      static void
   get_semantic_sv_name(nir_variable *var, struct semantic_info *info, gl_shader_stage stage)
   {
      if (stage != MESA_SHADER_VERTEX)
            switch (var->data.location) {
   case SYSTEM_VALUE_VERTEX_ID_ZERO_BASE:
      info->kind = DXIL_SEM_VERTEX_ID;
      case SYSTEM_VALUE_FRONT_FACE:
      info->kind = DXIL_SEM_IS_FRONT_FACE;
      case SYSTEM_VALUE_INSTANCE_ID:
      info->kind = DXIL_SEM_INSTANCE_ID;
      case SYSTEM_VALUE_PRIMITIVE_ID:
      info->kind = DXIL_SEM_PRIMITIVE_ID;
      case SYSTEM_VALUE_SAMPLE_ID:
      info->kind = DXIL_SEM_SAMPLE_INDEX;
      default:
         }
      }
      static void
   get_semantic_ps_outname(nir_variable *var, struct semantic_info *info)
   {
      info->kind = DXIL_SEM_INVALID;
   switch (var->data.location) {
   case FRAG_RESULT_COLOR:
      snprintf(info->name, 64, "%s", "SV_Target");
   info->index = var->data.index;
   info->kind = DXIL_SEM_TARGET;
      case FRAG_RESULT_DATA0:
   case FRAG_RESULT_DATA1:
   case FRAG_RESULT_DATA2:
   case FRAG_RESULT_DATA3:
   case FRAG_RESULT_DATA4:
   case FRAG_RESULT_DATA5:
   case FRAG_RESULT_DATA6:
   case FRAG_RESULT_DATA7:
      snprintf(info->name, 64, "%s", "SV_Target");
   info->index = var->data.location - FRAG_RESULT_DATA0;
   if (var->data.location == FRAG_RESULT_DATA0 &&
      var->data.index > 0)
      info->kind = DXIL_SEM_TARGET;
      case FRAG_RESULT_DEPTH:
      snprintf(info->name, 64, "%s", "SV_Depth");
   info->kind = DXIL_SEM_DEPTH;
      case FRAG_RESULT_STENCIL:
      snprintf(info->name, 64, "%s", "SV_StencilRef");
   info->kind = DXIL_SEM_STENCIL_REF; //??
      case FRAG_RESULT_SAMPLE_MASK:
      snprintf(info->name, 64, "%s", "SV_Coverage");
   info->kind = DXIL_SEM_COVERAGE; //??
      default:
      snprintf(info->name, 64, "%s", "UNDEFINED");
         }
      static void
   get_semantic_name(nir_variable *var, struct semantic_info *info,
         {
      info->kind = DXIL_SEM_INVALID;
   info->interpolation = get_interpolation(var);
            case VARYING_SLOT_POS:
      assert(glsl_get_components(type) == 4);
   snprintf(info->name, 64, "%s", "SV_Position");
   info->kind = DXIL_SEM_POSITION;
         case VARYING_SLOT_FACE:
      assert(glsl_get_components(type) == 1);
   snprintf(info->name, 64, "%s", "SV_IsFrontFace");
   info->kind = DXIL_SEM_IS_FRONT_FACE;
         case VARYING_SLOT_PRIMITIVE_ID:
   assert(glsl_get_components(type) == 1);
   snprintf(info->name, 64, "%s", "SV_PrimitiveID");
   info->kind = DXIL_SEM_PRIMITIVE_ID;
            case VARYING_SLOT_CLIP_DIST1:
      info->index = 1;
      case VARYING_SLOT_CLIP_DIST0:
      assert(var->data.location == VARYING_SLOT_CLIP_DIST1 || info->index == 0);
   snprintf(info->name, 64, "%s", "SV_ClipDistance");
   info->kind = DXIL_SEM_CLIP_DISTANCE;
         case VARYING_SLOT_TESS_LEVEL_INNER:
      assert(glsl_get_components(type) <= 2);
   snprintf(info->name, 64, "%s", "SV_InsideTessFactor");
   info->kind = DXIL_SEM_INSIDE_TESS_FACTOR;
         case VARYING_SLOT_TESS_LEVEL_OUTER:
      assert(glsl_get_components(type) <= 4);
   snprintf(info->name, 64, "%s", "SV_TessFactor");
   info->kind = DXIL_SEM_TESS_FACTOR;
         case VARYING_SLOT_VIEWPORT:
      assert(glsl_get_components(type) == 1);
   snprintf(info->name, 64, "%s", "SV_ViewportArrayIndex");
   info->kind = DXIL_SEM_VIEWPORT_ARRAY_INDEX;
         case VARYING_SLOT_LAYER:
      assert(glsl_get_components(type) == 1);
   snprintf(info->name, 64, "%s", "SV_RenderTargetArrayIndex");
   info->kind = DXIL_SEM_RENDERTARGET_ARRAY_INDEX;
         default: {
         info->index = var->data.driver_location;
   strcpy(info->name, "TEXCOORD");
            }
      static void
   get_semantic_in_name(nir_variable *var, struct semantic_info *info, gl_shader_stage stage)
   {
      const struct glsl_type *type = var->type;
   if (nir_is_arrayed_io(var, stage) &&
      glsl_type_is_array(type))
         get_semantic_name(var, info, type);
      }
         static enum dxil_prog_sig_semantic
   prog_semantic_from_kind(enum dxil_semantic_kind kind, unsigned num_vals, unsigned start_val)
   {
      switch (kind) {
   case DXIL_SEM_ARBITRARY: return DXIL_PROG_SEM_UNDEFINED;
   case DXIL_SEM_VERTEX_ID: return DXIL_PROG_SEM_VERTEX_ID;
   case DXIL_SEM_INSTANCE_ID: return DXIL_PROG_SEM_INSTANCE_ID;
   case DXIL_SEM_POSITION: return DXIL_PROG_SEM_POSITION;
   case DXIL_SEM_COVERAGE: return DXIL_PROG_SEM_COVERAGE;
   case DXIL_SEM_INNER_COVERAGE: return DXIL_PROG_SEM_INNER_COVERAGE;
   case DXIL_SEM_PRIMITIVE_ID: return DXIL_PROG_SEM_PRIMITIVE_ID;
   case DXIL_SEM_SAMPLE_INDEX: return DXIL_PROG_SEM_SAMPLE_INDEX;
   case DXIL_SEM_IS_FRONT_FACE: return DXIL_PROG_SEM_IS_FRONTFACE;
   case DXIL_SEM_RENDERTARGET_ARRAY_INDEX: return DXIL_PROG_SEM_RENDERTARGET_ARRAY_INDEX;
   case DXIL_SEM_VIEWPORT_ARRAY_INDEX: return DXIL_PROG_SEM_VIEWPORT_ARRAY_INDEX;
   case DXIL_SEM_CLIP_DISTANCE: return DXIL_PROG_SEM_CLIP_DISTANCE;
   case DXIL_SEM_CULL_DISTANCE: return DXIL_PROG_SEM_CULL_DISTANCE;
   case DXIL_SEM_BARYCENTRICS: return DXIL_PROG_SEM_BARYCENTRICS;
   case DXIL_SEM_SHADING_RATE: return DXIL_PROG_SEM_SHADING_RATE;
   case DXIL_SEM_CULL_PRIMITIVE: return DXIL_PROG_SEM_CULL_PRIMITIVE;
   case DXIL_SEM_TARGET: return DXIL_PROG_SEM_TARGET;
   case DXIL_SEM_DEPTH: return DXIL_PROG_SEM_DEPTH;
   case DXIL_SEM_DEPTH_LE: return DXIL_PROG_SEM_DEPTH_LE;
   case DXIL_SEM_DEPTH_GE: return DXIL_PROG_SEM_DEPTH_GE;
   case DXIL_SEM_STENCIL_REF: return DXIL_PROG_SEM_STENCIL_REF;
   case DXIL_SEM_TESS_FACTOR:
      switch (num_vals) {
   case 4: return DXIL_PROG_SEM_FINAL_QUAD_EDGE_TESSFACTOR;
   case 3: return DXIL_PROG_SEM_FINAL_TRI_EDGE_TESSFACTOR;
   case 2: return start_val == 0 ?
      DXIL_PROG_SEM_FINAL_LINE_DENSITY_TESSFACTOR :
      default:
            case DXIL_SEM_INSIDE_TESS_FACTOR:
      switch (num_vals) {
   case 2: return DXIL_PROG_SEM_FINAL_QUAD_INSIDE_EDGE_TESSFACTOR;
   case 1: return DXIL_PROG_SEM_FINAL_TRI_INSIDE_EDGE_TESSFACTOR;
   default:
            default:
            }
      static
   uint32_t
   copy_semantic_name_to_string(struct _mesa_string_buffer *string_out, const char *name)
   {
      /*  copy the semantic name */
   uint32_t retval = string_out->length;
   size_t name_len = strlen(name) + 1;
   _mesa_string_buffer_append_len(string_out, name, name_len);
      }
      static
   uint32_t
   append_semantic_index_to_table(struct dxil_psv_sem_index_table *table, uint32_t index,
         {
      for (unsigned i = 0; i < table->size; ++i) {
      unsigned j = 0;
   for (; j < num_rows && i + j < table->size; ++j)
      if (table->data[i + j] != index + j)
      if (j == num_rows)
         else if (j > 0)
      }
   uint32_t retval = table->size;
   assert(table->size + num_rows <= 80);
   for (unsigned i = 0; i < num_rows; ++i)
            }
      static const struct dxil_mdnode *
   fill_SV_param_nodes(struct dxil_module *mod, unsigned record_id,
                           const struct dxil_mdnode *SV_params_nodes[11];
   /* For this to always work we should use vectorize_io, but for FS out and VS in
   * this is not implemented globally */
            for (unsigned i = 0; i < rec->num_elements; ++i)
            SV_params_nodes[0] = dxil_get_metadata_int32(mod, (int)record_id); // Unique element ID
   SV_params_nodes[1] = dxil_get_metadata_string(mod, rec->name); // Element name
   SV_params_nodes[2] = dxil_get_metadata_int8(mod, rec->sig_comp_type); // Element type
   SV_params_nodes[3] = dxil_get_metadata_int8(mod, (int8_t)psv->semantic_kind); // Effective system value
   SV_params_nodes[4] = dxil_get_metadata_node(mod, flattened_semantics,
         SV_params_nodes[5] = dxil_get_metadata_int8(mod, psv->interpolation_mode); // Interpolation mode
   SV_params_nodes[6] = dxil_get_metadata_int32(mod, psv->rows); // Number of rows
   SV_params_nodes[7] = dxil_get_metadata_int8(mod, psv->cols_and_start & 0xf); // Number of columns
   SV_params_nodes[8] = dxil_get_metadata_int32(mod, rec->elements[0].reg); // Element packing start row
            const struct dxil_mdnode *SV_metadata[6];
   unsigned num_metadata_nodes = 0;
   if (rec->elements[0].stream != 0) {
      SV_metadata[num_metadata_nodes++] = dxil_get_metadata_int32(mod, DXIL_SIGNATURE_ELEMENT_OUTPUT_STREAM);
      }
   uint8_t usage_mask = rec->elements[0].always_reads_mask;
   if (!is_input)
         if (usage_mask && mod->minor_validator >= 5) {
      usage_mask >>= (psv->cols_and_start >> 4) & 0x3;
   SV_metadata[num_metadata_nodes++] = dxil_get_metadata_int32(mod, DXIL_SIGNATURE_ELEMENT_USAGE_COMPONENT_MASK);
               uint8_t dynamic_index_mask = psv->dynamic_mask_and_stream & 0xf;
   if (dynamic_index_mask) {
      SV_metadata[num_metadata_nodes++] = dxil_get_metadata_int32(mod, DXIL_SIGNATURE_ELEMENT_DYNAMIC_INDEX_COMPONENT_MASK);
                           }
      static void
   fill_signature_element(struct dxil_signature_element *elm,
               {
      memset(elm, 0, sizeof(struct dxil_signature_element));
   elm->stream = semantic->stream;
   // elm->semantic_name_offset = 0;  // Offset needs to be filled out when writing
   elm->semantic_index = semantic->index + row;
   elm->system_value = (uint32_t) prog_semantic_from_kind(semantic->kind, semantic->rows, row);
   elm->comp_type = (uint32_t) semantic->comp_type;
            assert(semantic->cols + semantic->start_col <= 4);
   elm->mask = (uint8_t) (((1 << semantic->cols) - 1) << semantic->start_col);
      }
      static bool
   fill_psv_signature_element(struct dxil_psv_signature_element *psv_elm,
         {
      memset(psv_elm, 0, sizeof(struct dxil_psv_signature_element));
   psv_elm->rows = semantic->rows;
   if (semantic->start_row >= 0) {
      assert(semantic->start_row < 256);
   psv_elm->start_row = semantic->start_row;
      } else {
      /* The validation expects that the the start row is not egative
   * and apparently the extra bit in the cols_and_start indicates that the
   * row is meant literally, so don't set it in this case.
   * (Source of information: Comparing with the validation structures
   * created by dxcompiler)
   */
   psv_elm->start_row = 0;
      }
   psv_elm->semantic_kind = (uint8_t)semantic->kind;
   psv_elm->component_type = semantic->comp_type;
   psv_elm->interpolation_mode = semantic->interpolation;
   psv_elm->dynamic_mask_and_stream = (semantic->stream) << 4;
   if (semantic->kind == DXIL_SEM_ARBITRARY && strlen(semantic->name)) {
      psv_elm->semantic_name_offset =
            /* TODO: clean up memory */
   if (psv_elm->semantic_name_offset == (uint32_t)-1)
               psv_elm->semantic_indexes_offset =
               }
      static bool
   fill_io_signature(struct dxil_module *mod, int id,
                     {
      rec->name = ralloc_strdup(mod->ralloc_ctx, semantic->name);
   rec->num_elements = semantic->rows;
            for (unsigned i = 0; i < semantic->rows; ++i)
            }
      static unsigned
   get_input_signature_group(struct dxil_module *mod,
                           {
      nir_foreach_variable_with_modes(var, s, modes) {
      if (var->data.patch)
            struct semantic_info semantic = {0};
   get_semantics(var, &semantic, s->info.stage);
   mod->inputs[num_inputs].sysvalue = semantic.sysvalue_name;
   nir_variable *base_var = var;
   if (var->data.location_frac)
         if (base_var != var)
      /* Combine fractional vars into any already existing row */
   get_additional_semantic_info(s, var, &semantic,
            else
            mod->input_mappings[var->data.driver_location] = num_inputs;
            if (!fill_io_signature(mod, num_inputs, &semantic,
                  mod->num_psv_inputs = MAX2(mod->num_psv_inputs,
            ++num_inputs;
      }
      }
      static void
   process_input_signature(struct dxil_module *mod, nir_shader *s, unsigned input_clip_size)
   {
      if (s->info.stage == MESA_SHADER_KERNEL)
                  mod->num_sig_inputs = get_input_signature_group(mod, 0,
                              mod->num_sig_inputs = get_input_signature_group(mod, mod->num_sig_inputs,
                     }
      static const char *out_sysvalue_name(nir_variable *var)
   {
      switch (var->data.location) {
   case VARYING_SLOT_FACE:
         case VARYING_SLOT_POS:
         case VARYING_SLOT_CLIP_DIST0:
   case VARYING_SLOT_CLIP_DIST1:
         case VARYING_SLOT_PRIMITIVE_ID:
         default:
            }
      static void
   process_output_signature(struct dxil_module *mod, nir_shader *s)
   {
      unsigned num_outputs = 0;
   unsigned next_row = 0;
   nir_foreach_variable_with_modes(var, s, nir_var_shader_out) {
      struct semantic_info semantic = {0};
   if (var->data.patch)
            if (s->info.stage == MESA_SHADER_FRAGMENT) {
      get_semantic_ps_outname(var, &semantic);
      } else {
      const struct glsl_type *type = var->type;
   if (nir_is_arrayed_io(var, s->info.stage))
         get_semantic_name(var, &semantic, type);
      }
   nir_variable *base_var = var;
   if (var->data.location_frac)
         if (base_var != var &&
      base_var->data.stream == var->data.stream)
   /* Combine fractional vars into any already existing row */
   get_additional_semantic_info(s, var, &semantic,
            else
            mod->info.has_out_position |= semantic.kind== DXIL_SEM_POSITION;
                     if (!fill_io_signature(mod, num_outputs, &semantic,
                  for (unsigned i = 0; i < mod->outputs[num_outputs].num_elements; ++i) {
      struct dxil_signature_element *elm = &mod->outputs[num_outputs].elements[i];
   if (mod->minor_validator <= 4)
         else
      /* This will be updated by the module processing */
                     mod->num_psv_outputs[semantic.stream] = MAX2(mod->num_psv_outputs[semantic.stream],
      }
      }
      static const char *
   patch_sysvalue_name(nir_variable *var)
   {
      switch (var->data.location) {
   case VARYING_SLOT_TESS_LEVEL_OUTER:
      switch (glsl_get_aoa_size(var->type)) {
   case 4:
         case 3:
         case 2:
      return var->data.location_frac == 0 ?
      default:
         }
      case VARYING_SLOT_TESS_LEVEL_INNER:
      switch (glsl_get_aoa_size(var->type)) {
   case 2:
         case 1:
         default:
         }
      default:
            }
      static void
   process_patch_const_signature(struct dxil_module *mod, nir_shader *s)
   {
      if (s->info.stage != MESA_SHADER_TESS_CTRL &&
      s->info.stage != MESA_SHADER_TESS_EVAL)
         nir_variable_mode mode = s->info.stage == MESA_SHADER_TESS_CTRL ?
         unsigned num_consts = 0;
   unsigned next_row = 0;
   nir_foreach_variable_with_modes(var, s, mode) {
      struct semantic_info semantic = {0};
   if (!var->data.patch)
            const struct glsl_type *type = var->type;
            mod->patch_consts[num_consts].sysvalue = patch_sysvalue_name(var);
                     if (!fill_io_signature(mod, num_consts, &semantic,
                  if (mode == nir_var_shader_out) {
      for (unsigned i = 0; i < mod->patch_consts[num_consts].num_elements; ++i) {
      struct dxil_signature_element *elm = &mod->patch_consts[num_consts].elements[i];
   if (mod->minor_validator <= 4)
         else
      /* This will be updated by the module processing */
                        mod->num_psv_patch_consts = MAX2(mod->num_psv_patch_consts,
      }
      }
      static void
   prepare_dependency_tables(struct dxil_module *mod, nir_shader *s)
   {
      bool uses_view_id = BITSET_TEST(s->info.system_values_read, SYSTEM_VALUE_VIEW_INDEX);
   uint32_t num_output_streams = s->info.stage == MESA_SHADER_GEOMETRY ? 4 : 1;
            const uint32_t output_vecs_per_dword = 32 /* bits per dword */ / 4 /* components per vec */;
            for (uint32_t i = 0; i < num_output_streams; ++i)
         if (s->info.stage == MESA_SHADER_TESS_CTRL)
            uint32_t view_id_table_sizes[4] = { 0 };
   if (uses_view_id) {
      for (uint32_t i = 0; i < 4; ++i)
               for (uint32_t i = 0; i < num_output_streams; ++i)
         if (s->info.stage == MESA_SHADER_TESS_CTRL)
         else if (s->info.stage == MESA_SHADER_TESS_EVAL)
            mod->serialized_dependency_table_size = num_tables + 1;
   for (uint32_t i = 0; i < num_tables; ++i) {
                  uint32_t *table = calloc(mod->serialized_dependency_table_size, sizeof(uint32_t));
            *(table++) = mod->num_psv_inputs * 4;
   for (uint32_t i = 0; i < num_output_streams; ++i) {
      *(table++) = mod->num_psv_outputs[i] * 4;
   mod->viewid_dependency_table[i] = table;
   table += view_id_table_sizes[i];
   mod->io_dependency_table[i] = table;
      }
   if (s->info.stage == MESA_SHADER_TESS_CTRL || s->info.stage == MESA_SHADER_TESS_EVAL) {
      *(table++) = mod->num_psv_patch_consts * 4;
   if (s->info.stage == MESA_SHADER_TESS_CTRL) {
      mod->viewid_dependency_table[1] = table;
      }
   mod->io_dependency_table[1] = table;
         }
      void
   preprocess_signatures(struct dxil_module *mod, nir_shader *s, unsigned input_clip_size)
   {
      /* DXC does the same: Add an empty string before everything else */
   mod->sem_string_table = _mesa_string_buffer_create(mod->ralloc_ctx, 1024);
            process_input_signature(mod, s, input_clip_size);
   process_output_signature(mod, s);
               }
      static const struct dxil_mdnode *
   get_signature_metadata(struct dxil_module *mod,
                           {
      if (num_elements == 0)
            const struct dxil_mdnode *nodes[VARYING_SLOT_MAX];
   for (unsigned i = 0; i < num_elements; ++i) {
                     }
      const struct dxil_mdnode *
   get_signatures(struct dxil_module *mod)
   {
      const struct dxil_mdnode *input_signature = get_signature_metadata(mod, mod->inputs, mod->psv_inputs, mod->num_sig_inputs, true);
   const struct dxil_mdnode *output_signature = get_signature_metadata(mod, mod->outputs, mod->psv_outputs, mod->num_sig_outputs, false);
   const struct dxil_mdnode *patch_const_signature = get_signature_metadata(mod, mod->patch_consts, mod->psv_patch_consts, mod->num_sig_patch_consts,
            const struct dxil_mdnode *SV_nodes[3] = {
      input_signature,
   output_signature,
      };
   if (output_signature || input_signature || patch_const_signature)
         else
      }
