   /*
   * Copyright © 2014 Intel Corporation
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
   *
   * Authors:
   *    Connor Abbott (cwabbott0@gmail.com)
   *
   */
      #include <inttypes.h> /* for PRIx64 macro */
   #include <math.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include "compiler/shader_enums.h"
   #include "util/half_float.h"
   #include "util/memstream.h"
   #include "util/mesa-sha1.h"
   #include "vulkan/vulkan_core.h"
   #include "nir.h"
      static void
   print_indentation(unsigned levels, FILE *fp)
   {
      for (unsigned i = 0; i < levels; i++)
      }
      typedef struct {
      FILE *fp;
   nir_shader *shader;
   /** map from nir_variable -> printable name */
            /** set of names used so far for nir_variables */
            /* an index used to make new non-conflicting names */
            /* Used with nir_gather_types() to identify best representation
   * to print terse inline constant values together with SSA sources.
   * Updated per nir_function_impl being printed.
   */
   BITSET_WORD *float_types;
            /**
   * Optional table of annotations mapping nir object
   * (such as instr or var) to message to print.
   */
            /* Maximum length for SSA or Reg index in the current impl */
            /* Padding for instructions without destination to make
   * them align with the `=` for instructions with destination.
   */
      } print_state;
      static void
   print_annotation(print_state *state, void *obj)
   {
               if (!state->annotations)
            struct hash_entry *entry = _mesa_hash_table_search(state->annotations, obj);
   if (!entry)
            const char *note = entry->data;
               }
      /* For 1 element, the size is intentionally omitted. */
   static const char *sizes[] = { "x??", "   ", "x2 ", "x3 ", "x4 ",
                        static const char *
   divergence_status(print_state *state, bool divergent)
   {
      if (state->shader->info.divergence_analysis_run)
               }
      static unsigned
   count_digits(unsigned n)
   {
         }
      static void
   print_def(nir_def *def, print_state *state)
   {
                                 fprintf(fp, "%s%u%s%*s%%%u",
         divergence_status(state, def->divergent),
      }
      static unsigned
   calculate_padding_for_no_dest(print_state *state)
   {
      const unsigned div = state->shader->info.divergence_analysis_run ? 4 : 0;
   const unsigned ssa_size = 5;
   const unsigned percent = 1;
   const unsigned ssa_index = count_digits(state->max_dest_index);
   const unsigned equals = 1;
      }
      static void
   print_no_dest_padding(print_state *state)
   {
               if (state->padding_for_no_dest)
      }
      static void
   print_hex_padded_const_value(const nir_const_value *value, unsigned bit_size, FILE *fp)
   {
      switch (bit_size) {
   case 64:
      fprintf(fp, "0x%016" PRIx64, value->u64);
      case 32:
      fprintf(fp, "0x%08x", value->u32);
      case 16:
      fprintf(fp, "0x%04x", value->u16);
      case 8:
      fprintf(fp, "0x%02x", value->u8);
      default:
            }
      static void
   print_hex_terse_const_value(const nir_const_value *value, unsigned bit_size, FILE *fp)
   {
      switch (bit_size) {
   case 64:
      fprintf(fp, "0x%" PRIx64, value->u64);
      case 32:
      fprintf(fp, "0x%x", value->u32);
      case 16:
      fprintf(fp, "0x%x", value->u16);
      case 8:
      fprintf(fp, "0x%x", value->u8);
      default:
            }
      static void
   print_float_const_value(const nir_const_value *value, unsigned bit_size, FILE *fp)
   {
      switch (bit_size) {
   case 64:
      fprintf(fp, "%f", value->f64);
      case 32:
      fprintf(fp, "%f", value->f32);
      case 16:
      fprintf(fp, "%f", _mesa_half_to_float(value->u16));
      default:
            }
      static void
   print_int_const_value(const nir_const_value *value, unsigned bit_size, FILE *fp)
   {
      switch (bit_size) {
   case 64:
      fprintf(fp, "%+" PRIi64, value->i64);
      case 32:
      fprintf(fp, "%+d", value->i32);
      case 16:
      fprintf(fp, "%+d", value->i16);
      case 8:
      fprintf(fp, "%+d", value->i8);
      default:
            }
      static void
   print_uint_const_value(const nir_const_value *value, unsigned bit_size, FILE *fp)
   {
      switch (bit_size) {
   case 64:
      fprintf(fp, "%" PRIu64, value->u64);
      case 32:
      fprintf(fp, "%u", value->u32);
      case 16:
      fprintf(fp, "%u", value->u16);
      case 8:
      fprintf(fp, "%u", value->u8);
      default:
            }
      static void
   print_const_from_load(nir_load_const_instr *instr, print_state *state, nir_alu_type type)
   {
               const unsigned bit_size = instr->def.bit_size;
            /* There's only one way to print booleans. */
   if (bit_size == 1) {
      fprintf(fp, "(");
   for (unsigned i = 0; i < num_components; i++) {
      if (i != 0)
            }
   fprintf(fp, ")");
                                 if (type != nir_type_invalid) {
      for (unsigned i = 0; i < num_components; i++) {
      const nir_const_value *v = &instr->value[i];
   if (i != 0)
         switch (type) {
   case nir_type_float:
      print_float_const_value(v, bit_size, fp);
      case nir_type_int:
   case nir_type_uint:
                  default:
                  #define PRINT_VALUES(F)                               \
      do {                                               \
      for (unsigned i = 0; i < num_components; i++) { \
      if (i != 0)                                  \
                     #define SEPARATOR()         \
      if (num_components > 1)  \
         else                     \
               bool needs_float = bit_size > 8;
   bool needs_signed = false;
   bool needs_decimal = false;
   for (unsigned i = 0; i < num_components; i++) {
      const nir_const_value *v = &instr->value[i];
   switch (bit_size) {
   case 64:
      needs_signed |= v->i64 < 0;
   needs_decimal |= v->u64 >= 10;
      case 32:
      needs_signed |= v->i32 < 0;
   needs_decimal |= v->u32 >= 10;
      case 16:
      needs_signed |= v->i16 < 0;
   needs_decimal |= v->u16 >= 10;
      case 8:
      needs_signed |= v->i8 < 0;
   needs_decimal |= v->u8 >= 10;
      default:
                     if (state->int_types) {
      const unsigned index = instr->def.index;
                  if (inferred_int && !inferred_float) {
         } else if (inferred_float && !inferred_int) {
      needs_signed = false;
                           if (needs_float) {
      SEPARATOR();
               if (needs_signed) {
      SEPARATOR();
               if (needs_decimal) {
      SEPARATOR();
                     }
      static void
   print_load_const_instr(nir_load_const_instr *instr, print_state *state)
   {
                                 /* In the definition, print all interpretations of the value. */
      }
      static void
   print_ssa_use(nir_def *def, print_state *state, nir_alu_type src_type)
   {
      FILE *fp = state->fp;
   fprintf(fp, "%%%u", def->index);
            if (instr->type == nir_instr_type_load_const && !NIR_DEBUG(PRINT_NO_INLINE_CONSTS)) {
      nir_load_const_instr *load_const = nir_instr_as_load_const(instr);
                     if (type == nir_type_invalid && state->int_types) {
      const unsigned index = load_const->def.index;
                  if (inferred_float && !inferred_int)
               if (type == nir_type_invalid)
            /* For a constant in a source, always pick one interpretation. */
   assert(type != nir_type_invalid);
         }
      static void print_src(const nir_src *src, print_state *state, nir_alu_type src_type);
      static void
   print_src(const nir_src *src, print_state *state, nir_alu_type src_type)
   {
         }
      static const char *
   comp_mask_string(unsigned num_components)
   {
         }
      static void
   print_alu_src(nir_alu_instr *instr, unsigned src, print_state *state)
   {
               const nir_op_info *info = &nir_op_infos[instr->op];
            bool print_swizzle = false;
            for (unsigned i = 0; i < NIR_MAX_VEC_COMPONENTS; i++) {
      if (!nir_alu_instr_channel_used(instr, src, i))
                     if (instr->src[src].swizzle[i] != i) {
      print_swizzle = true;
                           if (print_swizzle || used_channels != live_channels) {
      fprintf(fp, ".");
   for (unsigned i = 0; i < NIR_MAX_VEC_COMPONENTS; i++) {
                              }
      static void
   print_alu_instr(nir_alu_instr *instr, print_state *state)
   {
                        fprintf(fp, " = %s", nir_op_infos[instr->op].name);
   if (instr->exact)
         if (instr->no_signed_wrap)
         if (instr->no_unsigned_wrap)
                  for (unsigned i = 0; i < nir_op_infos[instr->op].num_inputs; i++) {
      if (i != 0)
                  }
      static const char *
   get_var_name(nir_variable *var, print_state *state)
   {
      if (state->ht == NULL)
                     struct hash_entry *entry = _mesa_hash_table_search(state->ht, var);
   if (entry)
            char *name;
   if (var->name == NULL) {
         } else {
      struct set_entry *set_entry = _mesa_set_search(state->syms, var->name);
   if (set_entry != NULL) {
      /* we have a collision with another name, append an # + a unique
   * index */
   name = ralloc_asprintf(state->syms, "%s#%u", var->name,
      } else {
      /* Mark this one as seen */
   _mesa_set_add(state->syms, var->name);
                              }
      static const char *
   get_constant_sampler_addressing_mode(enum cl_sampler_addressing_mode mode)
   {
      switch (mode) {
   case SAMPLER_ADDRESSING_MODE_NONE:
         case SAMPLER_ADDRESSING_MODE_CLAMP_TO_EDGE:
         case SAMPLER_ADDRESSING_MODE_CLAMP:
         case SAMPLER_ADDRESSING_MODE_REPEAT:
         case SAMPLER_ADDRESSING_MODE_REPEAT_MIRRORED:
         default:
            }
      static const char *
   get_constant_sampler_filter_mode(enum cl_sampler_filter_mode mode)
   {
      switch (mode) {
   case SAMPLER_FILTER_MODE_NEAREST:
         case SAMPLER_FILTER_MODE_LINEAR:
         default:
            }
      static void
   print_constant(nir_constant *c, const struct glsl_type *type, print_state *state)
   {
      FILE *fp = state->fp;
   const unsigned rows = glsl_get_vector_elements(type);
   const unsigned cols = glsl_get_matrix_columns(type);
            switch (glsl_get_base_type(type)) {
   case GLSL_TYPE_BOOL:
      /* Only float base types can be matrices. */
            for (i = 0; i < rows; i++) {
      if (i > 0)
            }
         case GLSL_TYPE_UINT8:
   case GLSL_TYPE_INT8:
      /* Only float base types can be matrices. */
            for (i = 0; i < rows; i++) {
      if (i > 0)
            }
         case GLSL_TYPE_UINT16:
   case GLSL_TYPE_INT16:
      /* Only float base types can be matrices. */
            for (i = 0; i < rows; i++) {
      if (i > 0)
            }
         case GLSL_TYPE_UINT:
   case GLSL_TYPE_INT:
      /* Only float base types can be matrices. */
            for (i = 0; i < rows; i++) {
      if (i > 0)
            }
         case GLSL_TYPE_FLOAT16:
   case GLSL_TYPE_FLOAT:
   case GLSL_TYPE_DOUBLE:
      if (cols > 1) {
      for (i = 0; i < cols; i++) {
      if (i > 0)
               } else {
      switch (glsl_get_base_type(type)) {
   case GLSL_TYPE_FLOAT16:
      for (i = 0; i < rows; i++) {
      if (i > 0)
                        case GLSL_TYPE_FLOAT:
      for (i = 0; i < rows; i++) {
      if (i > 0)
                        case GLSL_TYPE_DOUBLE:
      for (i = 0; i < rows; i++) {
      if (i > 0)
                        default:
            }
         case GLSL_TYPE_UINT64:
   case GLSL_TYPE_INT64:
      /* Only float base types can be matrices. */
            for (i = 0; i < cols; i++) {
      if (i > 0)
            }
         case GLSL_TYPE_STRUCT:
   case GLSL_TYPE_INTERFACE:
      for (i = 0; i < c->num_elements; i++) {
      if (i > 0)
         fprintf(fp, "{ ");
   print_constant(c->elements[i], glsl_get_struct_field(type, i), state);
      }
         case GLSL_TYPE_ARRAY:
      for (i = 0; i < c->num_elements; i++) {
      if (i > 0)
         fprintf(fp, "{ ");
   print_constant(c->elements[i], glsl_get_array_element(type), state);
      }
         default:
            }
      static const char *
   get_variable_mode_str(nir_variable_mode mode, bool want_local_global_mode)
   {
      switch (mode) {
   case nir_var_shader_in:
         case nir_var_shader_out:
         case nir_var_uniform:
         case nir_var_mem_ubo:
         case nir_var_system_value:
         case nir_var_mem_ssbo:
         case nir_var_mem_shared:
         case nir_var_mem_global:
         case nir_var_mem_push_const:
         case nir_var_mem_constant:
         case nir_var_image:
         case nir_var_shader_temp:
         case nir_var_function_temp:
         case nir_var_shader_call_data:
         case nir_var_ray_hit_attrib:
         case nir_var_mem_task_payload:
         case nir_var_mem_node_payload:
         case nir_var_mem_node_payload_in:
         default:
      if (mode && (mode & nir_var_mem_generic) == mode)
               }
      static const char *
   get_location_str(unsigned location, gl_shader_stage stage,
         {
      switch (stage) {
   case MESA_SHADER_VERTEX:
      if (mode == nir_var_shader_in)
         else if (mode == nir_var_shader_out)
               case MESA_SHADER_TESS_CTRL:
   case MESA_SHADER_TESS_EVAL:
      if (location >= VARYING_SLOT_MAX)
            case MESA_SHADER_TASK:
   case MESA_SHADER_MESH:
   case MESA_SHADER_GEOMETRY:
      if (mode == nir_var_shader_in || mode == nir_var_shader_out)
               case MESA_SHADER_FRAGMENT:
      if (mode == nir_var_shader_in)
         else if (mode == nir_var_shader_out)
               case MESA_SHADER_COMPUTE:
   case MESA_SHADER_KERNEL:
   default:
      /* TODO */
               if (mode == nir_var_system_value)
            if (location == ~0) {
         } else {
      snprintf(buf, 4, "%u", location);
         }
      static void
   print_access(enum gl_access_qualifier access, print_state *state, const char *separator)
   {
      if (!access) {
      fputs("none", state->fp);
               static const struct {
      enum gl_access_qualifier bit;
      } modes[] = {
      { ACCESS_COHERENT, "coherent" },
   { ACCESS_VOLATILE, "volatile" },
   { ACCESS_RESTRICT, "restrict" },
   { ACCESS_NON_WRITEABLE, "readonly" },
   { ACCESS_NON_READABLE, "writeonly" },
   { ACCESS_CAN_REORDER, "reorderable" },
   { ACCESS_CAN_SPECULATE, "speculatable" },
   { ACCESS_NON_TEMPORAL, "non-temporal" },
               bool first = true;
   for (unsigned i = 0; i < ARRAY_SIZE(modes); ++i) {
      if (access & modes[i].bit) {
      fprintf(state->fp, "%s%s", first ? "" : separator, modes[i].name);
            }
      static void
   print_var_decl(nir_variable *var, print_state *state)
   {
                        const char *const bindless = (var->data.bindless) ? "bindless " : "";
   const char *const cent = (var->data.centroid) ? "centroid " : "";
   const char *const samp = (var->data.sample) ? "sample " : "";
   const char *const patch = (var->data.patch) ? "patch " : "";
   const char *const inv = (var->data.invariant) ? "invariant " : "";
   const char *const per_view = (var->data.per_view) ? "per_view " : "";
   const char *const per_primitive = (var->data.per_primitive) ? "per_primitive " : "";
   const char *const ray_query = (var->data.ray_query) ? "ray_query " : "";
   fprintf(fp, "%s%s%s%s%s%s%s%s%s %s ",
         bindless, cent, samp, patch, inv, per_view, per_primitive, ray_query,
            print_access(var->data.access, state, " ");
            if (glsl_get_base_type(glsl_without_array(var->type)) == GLSL_TYPE_IMAGE) {
                  if (var->data.precision) {
      const char *precisions[] = {
      "",
   "highp",
   "mediump",
      };
               fprintf(fp, "%s %s", glsl_get_type_name(var->type),
            if (var->data.mode & (nir_var_shader_in |
                        nir_var_shader_out |
   nir_var_uniform |
   nir_var_system_value |
   char buf[4];
   const char *loc = get_location_str(var->data.location,
                  /* For shader I/O vars that have been split to components or packed,
   * print the fractional location within the input/output.
   */
   unsigned int num_components =
         const char *components = "";
   char components_local[18] = { '.' /* the rest is 0-filled */ };
   switch (var->data.mode) {
   case nir_var_shader_in:
   case nir_var_shader_out:
      if (num_components < 16 && num_components != 0) {
      const char *xyzw = comp_mask_string(num_components);
                     }
      default:
                  if (var->data.mode & nir_var_system_value) {
         } else {
      fprintf(fp, " (%s%s, %u, %u)%s", loc,
         components,
                  if (var->constant_initializer) {
      if (var->constant_initializer->is_null_constant) {
         } else {
      fprintf(fp, " = { ");
   print_constant(var->constant_initializer, var->type, state);
         }
   if (glsl_type_is_sampler(var->type) && var->data.sampler.is_inline_sampler) {
      fprintf(fp, " = { %s, %s, %s }",
         get_constant_sampler_addressing_mode(var->data.sampler.addressing_mode),
      }
   if (var->pointer_initializer)
            fprintf(fp, "\n");
      }
      static void
   print_deref_link(const nir_deref_instr *instr, bool whole_chain, print_state *state)
   {
               if (instr->deref_type == nir_deref_type_var) {
      fprintf(fp, "%s", get_var_name(instr->var, state));
      } else if (instr->deref_type == nir_deref_type_cast) {
      fprintf(fp, "(%s *)", glsl_get_type_name(instr->type));
   print_src(&instr->parent, state, nir_type_invalid);
               nir_deref_instr *parent =
            /* Is the parent we're going to print a bare cast? */
   const bool is_parent_cast =
            /* If we're not printing the whole chain, the parent we print will be a SSA
   * value that represents a pointer.  The only deref type that naturally
   * gives a pointer is a cast.
   */
   const bool is_parent_pointer =
            /* Struct derefs have a nice syntax that works on pointers, arrays derefs
   * do not.
   */
   const bool need_deref =
            /* Cast need extra parens and so * dereferences */
   if (is_parent_cast || need_deref)
            if (need_deref)
            if (whole_chain) {
         } else {
                  if (is_parent_cast || need_deref)
            switch (instr->deref_type) {
   case nir_deref_type_struct:
      fprintf(fp, "%s%s", is_parent_pointer ? "->" : ".",
               case nir_deref_type_array:
   case nir_deref_type_ptr_as_array: {
      if (nir_src_is_const(instr->arr.index)) {
         } else {
      fprintf(fp, "[");
   print_src(&instr->arr.index, state, nir_type_invalid);
      }
               case nir_deref_type_array_wildcard:
      fprintf(fp, "[*]");
         default:
            }
      static void
   print_deref_instr(nir_deref_instr *instr, print_state *state)
   {
                        switch (instr->deref_type) {
   case nir_deref_type_var:
      fprintf(fp, " = deref_var ");
      case nir_deref_type_array:
   case nir_deref_type_array_wildcard:
      fprintf(fp, " = deref_array ");
      case nir_deref_type_struct:
      fprintf(fp, " = deref_struct ");
      case nir_deref_type_cast:
      fprintf(fp, " = deref_cast ");
      case nir_deref_type_ptr_as_array:
      fprintf(fp, " = deref_ptr_as_array ");
      default:
                  /* Only casts naturally return a pointer type */
   if (instr->deref_type != nir_deref_type_cast)
                     fprintf(fp, " (");
   unsigned modes = instr->modes;
   while (modes) {
      int m = u_bit_scan(&modes);
   fprintf(fp, "%s%s", get_variable_mode_str(1 << m, true),
      }
            if (instr->deref_type == nir_deref_type_cast) {
      fprintf(fp, "  (ptr_stride=%u, align_mul=%u, align_offset=%u)",
                     if (instr->deref_type != nir_deref_type_var &&
      instr->deref_type != nir_deref_type_cast) {
   /* Print the entire chain as a comment */
   fprintf(fp, "  // &");
         }
      static const char *
   vulkan_descriptor_type_name(VkDescriptorType type)
   {
      switch (type) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
         case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
         case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
         case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
         case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
         case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
         case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
         case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
         case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
         case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
         case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
         case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK:
         case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
         default:
            }
      static void
   print_alu_type(nir_alu_type type, print_state *state)
   {
      FILE *fp = state->fp;
   unsigned size = nir_alu_type_get_type_size(type);
            switch (nir_alu_type_get_base_type(type)) {
   case nir_type_int:
      name = "int";
      case nir_type_uint:
      name = "uint";
      case nir_type_bool:
      name = "bool";
      case nir_type_float:
      name = "float";
      default:
         }
   if (size)
         else
      }
      static void
   print_intrinsic_instr(nir_intrinsic_instr *instr, print_state *state)
   {
      const nir_intrinsic_info *info = &nir_intrinsic_infos[instr->intrinsic];
   unsigned num_srcs = info->num_srcs;
            if (info->has_dest) {
      print_def(&instr->def, state);
      } else {
                           for (unsigned i = 0; i < num_srcs; i++) {
      if (i != 0)
                                 for (unsigned i = 0; i < info->num_indices; i++) {
      unsigned idx = info->indices[i];
   if (i != 0)
         switch (idx) {
   case NIR_INTRINSIC_WRITE_MASK: {
      /* special case wrmask to show it as a writemask.. */
   unsigned wrmask = nir_intrinsic_write_mask(instr);
   fprintf(fp, "wrmask=");
   for (unsigned i = 0; i < instr->num_components; i++)
      if ((wrmask >> i) & 1)
                  case NIR_INTRINSIC_REDUCTION_OP: {
      nir_op reduction_op = nir_intrinsic_reduction_op(instr);
   fprintf(fp, "reduction_op=%s", nir_op_infos[reduction_op].name);
               case NIR_INTRINSIC_ATOMIC_OP: {
                     switch (atomic_op) {
   case nir_atomic_op_iadd:
      fprintf(fp, "iadd");
      case nir_atomic_op_imin:
      fprintf(fp, "imin");
      case nir_atomic_op_umin:
      fprintf(fp, "umin");
      case nir_atomic_op_imax:
      fprintf(fp, "imax");
      case nir_atomic_op_umax:
      fprintf(fp, "umax");
      case nir_atomic_op_iand:
      fprintf(fp, "iand");
      case nir_atomic_op_ior:
      fprintf(fp, "ior");
      case nir_atomic_op_ixor:
      fprintf(fp, "ixor");
      case nir_atomic_op_xchg:
      fprintf(fp, "xchg");
      case nir_atomic_op_fadd:
      fprintf(fp, "fadd");
      case nir_atomic_op_fmin:
      fprintf(fp, "fmin");
      case nir_atomic_op_fmax:
      fprintf(fp, "fmax");
      case nir_atomic_op_cmpxchg:
      fprintf(fp, "cmpxchg");
      case nir_atomic_op_fcmpxchg:
      fprintf(fp, "fcmpxchg");
      case nir_atomic_op_inc_wrap:
      fprintf(fp, "inc_wrap");
      case nir_atomic_op_dec_wrap:
      fprintf(fp, "dec_wrap");
      }
               case NIR_INTRINSIC_IMAGE_DIM: {
      static const char *dim_name[] = {
      [GLSL_SAMPLER_DIM_1D] = "1D",
   [GLSL_SAMPLER_DIM_2D] = "2D",
   [GLSL_SAMPLER_DIM_3D] = "3D",
   [GLSL_SAMPLER_DIM_CUBE] = "Cube",
   [GLSL_SAMPLER_DIM_RECT] = "Rect",
   [GLSL_SAMPLER_DIM_BUF] = "Buf",
   [GLSL_SAMPLER_DIM_MS] = "2D-MSAA",
   [GLSL_SAMPLER_DIM_SUBPASS] = "Subpass",
      };
   enum glsl_sampler_dim dim = nir_intrinsic_image_dim(instr);
   assert(dim < ARRAY_SIZE(dim_name) && dim_name[dim]);
   fprintf(fp, "image_dim=%s", dim_name[dim]);
               case NIR_INTRINSIC_IMAGE_ARRAY: {
      bool array = nir_intrinsic_image_array(instr);
   fprintf(fp, "image_array=%s", array ? "true" : "false");
               case NIR_INTRINSIC_FORMAT: {
      enum pipe_format format = nir_intrinsic_format(instr);
   fprintf(fp, "format=%s", util_format_short_name(format));
               case NIR_INTRINSIC_DESC_TYPE: {
      VkDescriptorType desc_type = nir_intrinsic_desc_type(instr);
   fprintf(fp, "desc_type=%s", vulkan_descriptor_type_name(desc_type));
               case NIR_INTRINSIC_SRC_TYPE: {
      fprintf(fp, "src_type=");
   print_alu_type(nir_intrinsic_src_type(instr), state);
               case NIR_INTRINSIC_DEST_TYPE: {
      fprintf(fp, "dest_type=");
   print_alu_type(nir_intrinsic_dest_type(instr), state);
               case NIR_INTRINSIC_SWIZZLE_MASK: {
      fprintf(fp, "swizzle_mask=");
   unsigned mask = nir_intrinsic_swizzle_mask(instr);
   if (instr->intrinsic == nir_intrinsic_quad_swizzle_amd) {
      for (unsigned i = 0; i < 4; i++)
      } else if (instr->intrinsic == nir_intrinsic_masked_swizzle_amd) {
      fprintf(fp, "((id & %d) | %d) ^ %d", mask & 0x1F,
            } else {
         }
               case NIR_INTRINSIC_MEMORY_SEMANTICS: {
      nir_memory_semantics semantics = nir_intrinsic_memory_semantics(instr);
   fprintf(fp, "mem_semantics=");
   switch (semantics & (NIR_MEMORY_ACQUIRE | NIR_MEMORY_RELEASE)) {
   case 0:
      fprintf(fp, "NONE");
      case NIR_MEMORY_ACQUIRE:
      fprintf(fp, "ACQ");
      case NIR_MEMORY_RELEASE:
      fprintf(fp, "REL");
      default:
      fprintf(fp, "ACQ|REL");
      }
   if (semantics & (NIR_MEMORY_MAKE_AVAILABLE))
         if (semantics & (NIR_MEMORY_MAKE_VISIBLE))
                     case NIR_INTRINSIC_MEMORY_MODES: {
      fprintf(fp, "mem_modes=");
   unsigned int modes = nir_intrinsic_memory_modes(instr);
   if (modes == 0)
         while (modes) {
      nir_variable_mode m = u_bit_scan(&modes);
      }
               case NIR_INTRINSIC_EXECUTION_SCOPE:
   case NIR_INTRINSIC_MEMORY_SCOPE: {
      mesa_scope scope =
      idx == NIR_INTRINSIC_MEMORY_SCOPE ? nir_intrinsic_memory_scope(instr)
      const char *name = mesa_scope_name(scope);
   static const char prefix[] = "SCOPE_";
   if (strncmp(name, prefix, sizeof(prefix) - 1) == 0)
         fprintf(fp, "%s=%s", nir_intrinsic_index_names[idx], name);
               case NIR_INTRINSIC_IO_SEMANTICS: {
               /* Try to figure out the mode so we can interpret the location */
   nir_variable_mode mode = nir_var_mem_generic;
   switch (instr->intrinsic) {
   case nir_intrinsic_load_input:
   case nir_intrinsic_load_interpolated_input:
   case nir_intrinsic_load_per_vertex_input:
   case nir_intrinsic_load_input_vertex:
   case nir_intrinsic_load_coefficients_agx:
                  case nir_intrinsic_load_output:
   case nir_intrinsic_store_output:
   case nir_intrinsic_store_per_primitive_output:
   case nir_intrinsic_store_per_vertex_output:
                  default:
                  /* Using that mode, we should be able to name the location */
   char buf[4];
   const char *loc = get_location_str(io.location,
                                                                                                                                                   if (state->shader &&
      state->shader->info.stage == MESA_SHADER_GEOMETRY &&
   (instr->intrinsic == nir_intrinsic_store_output ||
   instr->intrinsic == nir_intrinsic_store_per_primitive_output ||
   instr->intrinsic == nir_intrinsic_store_per_vertex_output)) {
   unsigned gs_streams = io.gs_streams;
   fprintf(fp, " gs_streams(");
   for (unsigned i = 0; i < 4; i++) {
      fprintf(fp, "%s%c=%u", i ? " " : "", "xyzw"[i],
      }
                           case NIR_INTRINSIC_IO_XFB:
   case NIR_INTRINSIC_IO_XFB2: {
      /* This prints both IO_XFB and IO_XFB2. */
   fprintf(fp, "xfb%s(", idx == NIR_INTRINSIC_IO_XFB ? "" : "2");
   bool first = true;
   for (unsigned i = 0; i < 2; i++) {
                                    if (!first)
                  if (xfb.out[i].num_components > 1) {
      fprintf(fp, "components=%u..%u",
      } else {
         }
   fprintf(fp, " buffer=%u offset=%u",
      }
   fprintf(fp, ")");
               case NIR_INTRINSIC_ROUNDING_MODE: {
      fprintf(fp, "rounding_mode=");
   switch (nir_intrinsic_rounding_mode(instr)) {
   case nir_rounding_mode_undef:
      fprintf(fp, "undef");
      case nir_rounding_mode_rtne:
      fprintf(fp, "rtne");
      case nir_rounding_mode_ru:
      fprintf(fp, "ru");
      case nir_rounding_mode_rd:
      fprintf(fp, "rd");
      case nir_rounding_mode_rtz:
      fprintf(fp, "rtz");
      default:
      fprintf(fp, "unknown");
      }
               case NIR_INTRINSIC_RAY_QUERY_VALUE: {
         #define VAL(_name)                   \
      case nir_ray_query_value_##_name: \
      fprintf(fp, #_name);           \
   break
         VAL(intersection_type);
   VAL(intersection_t);
   VAL(intersection_instance_custom_index);
   VAL(intersection_instance_id);
   VAL(intersection_instance_sbt_index);
   VAL(intersection_geometry_index);
   VAL(intersection_primitive_index);
   VAL(intersection_barycentrics);
   VAL(intersection_front_face);
   VAL(intersection_object_ray_direction);
   VAL(intersection_object_ray_origin);
   VAL(intersection_object_to_world);
   VAL(intersection_world_to_object);
   VAL(intersection_candidate_aabb_opaque);
   VAL(tmin);
   VAL(flags);
   #undef VAL
            default:
      fprintf(fp, "unknown");
      }
               case NIR_INTRINSIC_RESOURCE_ACCESS_INTEL: {
      fprintf(fp, "resource_intel=");
   unsigned int modes = nir_intrinsic_resource_access_intel(instr);
   if (modes == 0)
         while (modes) {
      nir_resource_data_intel i = 1u << u_bit_scan(&modes);
   switch (i) {
   case nir_resource_intel_bindless:
      fprintf(fp, "bindless");
      case nir_resource_intel_pushable:
      fprintf(fp, "pushable");
      case nir_resource_intel_sampler:
      fprintf(fp, "sampler");
      case nir_resource_intel_non_uniform:
      fprintf(fp, "non-uniform");
      default:
      fprintf(fp, "unknown");
      }
      }
               case NIR_INTRINSIC_ACCESS: {
      fprintf(fp, "access=");
   print_access(nir_intrinsic_access(instr), state, "|");
               case NIR_INTRINSIC_MATRIX_LAYOUT: {
      fprintf(fp, "matrix_layout=");
   switch (nir_intrinsic_matrix_layout(instr)) {
   case GLSL_MATRIX_LAYOUT_ROW_MAJOR:
      fprintf(fp, "row_major");
      case GLSL_MATRIX_LAYOUT_COLUMN_MAJOR:
      fprintf(fp, "col_major");
      default:
      fprintf(fp, "unknown");
      }
               case NIR_INTRINSIC_CMAT_DESC: {
      struct glsl_cmat_description desc = nir_intrinsic_cmat_desc(instr);
   const struct glsl_type *t = glsl_cmat_type(&desc);
   fprintf(fp, "%s", glsl_get_type_name(t));
               case NIR_INTRINSIC_CMAT_SIGNED_MASK: {
      fprintf(fp, "cmat_signed=");
   unsigned int mask = nir_intrinsic_cmat_signed_mask(instr);
   if (mask == 0)
         while (mask) {
      nir_cmat_signed i = 1u << u_bit_scan(&mask);
   switch (i) {
   case NIR_CMAT_A_SIGNED:
      fputc('A', fp);
      case NIR_CMAT_B_SIGNED:
      fputc('B', fp);
      case NIR_CMAT_C_SIGNED:
      fputc('C', fp);
      case NIR_CMAT_RESULT_SIGNED:
      fprintf(fp, "Result");
      default:
      fprintf(fp, "unknown");
      }
      }
               case NIR_INTRINSIC_ALU_OP: {
      nir_op alu_op = nir_intrinsic_alu_op(instr);
   fprintf(fp, "alu_op=%s", nir_op_infos[alu_op].name);
               default: {
      unsigned off = info->index_map[idx] - 1;
   fprintf(fp, "%s=%d", nir_intrinsic_index_names[idx], instr->const_index[off]);
      }
      }
            if (!state->shader)
            nir_variable_mode var_mode;
   switch (instr->intrinsic) {
   case nir_intrinsic_load_uniform:
      var_mode = nir_var_uniform;
      case nir_intrinsic_load_input:
   case nir_intrinsic_load_interpolated_input:
   case nir_intrinsic_load_per_vertex_input:
      var_mode = nir_var_shader_in;
      case nir_intrinsic_load_output:
   case nir_intrinsic_store_output:
   case nir_intrinsic_store_per_vertex_output:
      var_mode = nir_var_shader_out;
      default:
                  nir_foreach_variable_with_modes(var, state->shader, var_mode) {
      if ((var->data.driver_location == nir_intrinsic_base(instr)) &&
      (instr->intrinsic == nir_intrinsic_load_uniform ||
   (nir_intrinsic_component(instr) >= var->data.location_frac &&
      nir_intrinsic_component(instr) <
      var->name) {
   fprintf(fp, "  // %s", var->name);
            }
      static void
   print_tex_instr(nir_tex_instr *instr, print_state *state)
   {
                        fprintf(fp, " = (");
   print_alu_type(instr->dest_type, state);
            switch (instr->op) {
   case nir_texop_tex:
      fprintf(fp, "tex ");
      case nir_texop_txb:
      fprintf(fp, "txb ");
      case nir_texop_txl:
      fprintf(fp, "txl ");
      case nir_texop_txd:
      fprintf(fp, "txd ");
      case nir_texop_txf:
      fprintf(fp, "txf ");
      case nir_texop_txf_ms:
      fprintf(fp, "txf_ms ");
      case nir_texop_txf_ms_fb:
      fprintf(fp, "txf_ms_fb ");
      case nir_texop_txf_ms_mcs_intel:
      fprintf(fp, "txf_ms_mcs_intel ");
      case nir_texop_txs:
      fprintf(fp, "txs ");
      case nir_texop_lod:
      fprintf(fp, "lod ");
      case nir_texop_tg4:
      fprintf(fp, "tg4 ");
      case nir_texop_query_levels:
      fprintf(fp, "query_levels ");
      case nir_texop_texture_samples:
      fprintf(fp, "texture_samples ");
      case nir_texop_samples_identical:
      fprintf(fp, "samples_identical ");
      case nir_texop_tex_prefetch:
      fprintf(fp, "tex (pre-dispatchable) ");
      case nir_texop_fragment_fetch_amd:
      fprintf(fp, "fragment_fetch_amd ");
      case nir_texop_fragment_mask_fetch_amd:
      fprintf(fp, "fragment_mask_fetch_amd ");
      case nir_texop_descriptor_amd:
      fprintf(fp, "descriptor_amd ");
      case nir_texop_sampler_descriptor_amd:
      fprintf(fp, "sampler_descriptor_amd ");
      case nir_texop_lod_bias_agx:
      fprintf(fp, "lod_bias_agx ");
      case nir_texop_hdr_dim_nv:
      fprintf(fp, "hdr_dim_nv ");
      case nir_texop_tex_type_nv:
      fprintf(fp, "tex_type_nv ");
      default:
      unreachable("Invalid texture operation");
               bool has_texture_deref = false, has_sampler_deref = false;
   for (unsigned i = 0; i < instr->num_srcs; i++) {
      if (i > 0) {
                  print_src(&instr->src[i].src, state, nir_tex_instr_src_type(instr, i));
            switch (instr->src[i].src_type) {
   case nir_tex_src_backend1:
      fprintf(fp, "(backend1)");
      case nir_tex_src_backend2:
      fprintf(fp, "(backend2)");
      case nir_tex_src_coord:
      fprintf(fp, "(coord)");
      case nir_tex_src_projector:
      fprintf(fp, "(projector)");
      case nir_tex_src_comparator:
      fprintf(fp, "(comparator)");
      case nir_tex_src_offset:
      fprintf(fp, "(offset)");
      case nir_tex_src_bias:
      fprintf(fp, "(bias)");
      case nir_tex_src_lod:
      fprintf(fp, "(lod)");
      case nir_tex_src_min_lod:
      fprintf(fp, "(min_lod)");
      case nir_tex_src_ms_index:
      fprintf(fp, "(ms_index)");
      case nir_tex_src_ms_mcs_intel:
      fprintf(fp, "(ms_mcs_intel)");
      case nir_tex_src_ddx:
      fprintf(fp, "(ddx)");
      case nir_tex_src_ddy:
      fprintf(fp, "(ddy)");
      case nir_tex_src_texture_deref:
      has_texture_deref = true;
   fprintf(fp, "(texture_deref)");
      case nir_tex_src_sampler_deref:
      has_sampler_deref = true;
   fprintf(fp, "(sampler_deref)");
      case nir_tex_src_texture_offset:
      fprintf(fp, "(texture_offset)");
      case nir_tex_src_sampler_offset:
      fprintf(fp, "(sampler_offset)");
      case nir_tex_src_texture_handle:
      fprintf(fp, "(texture_handle)");
      case nir_tex_src_sampler_handle:
      fprintf(fp, "(sampler_handle)");
      case nir_tex_src_plane:
                  default:
      unreachable("Invalid texture source type");
                  if (instr->is_gather_implicit_lod)
            if (instr->op == nir_texop_tg4) {
                  if (nir_tex_instr_has_explicit_tg4_offsets(instr)) {
      fprintf(fp, ", { (%i, %i)", instr->tg4_offsets[0][0], instr->tg4_offsets[0][1]);
   for (unsigned i = 1; i < 4; ++i)
      fprintf(fp, ", (%i, %i)", instr->tg4_offsets[i][0],
                  if (instr->op != nir_texop_txf_ms_fb && !has_texture_deref) {
                  if (nir_tex_instr_need_sampler(instr) && !has_sampler_deref) {
                  if (instr->texture_non_uniform) {
                  if (instr->sampler_non_uniform) {
                  if (instr->is_sparse) {
            }
      static void
   print_call_instr(nir_call_instr *instr, print_state *state)
   {
                                 for (unsigned i = 0; i < instr->num_params; i++) {
      if (i != 0)
                  }
      static void
   print_jump_instr(nir_jump_instr *instr, print_state *state)
   {
                        switch (instr->type) {
   case nir_jump_break:
      fprintf(fp, "break");
         case nir_jump_continue:
      fprintf(fp, "continue");
         case nir_jump_return:
      fprintf(fp, "return");
         case nir_jump_halt:
      fprintf(fp, "halt");
         case nir_jump_goto:
      fprintf(fp, "goto b%u",
               case nir_jump_goto_if:
      fprintf(fp, "goto b%u if ",
         print_src(&instr->condition, state, nir_type_invalid);
   fprintf(fp, " else b%u",
               }
      static void
   print_ssa_undef_instr(nir_undef_instr *instr, print_state *state)
   {
      FILE *fp = state->fp;
   print_def(&instr->def, state);
      }
      static void
   print_phi_instr(nir_phi_instr *instr, print_state *state)
   {
      FILE *fp = state->fp;
   print_def(&instr->def, state);
   fprintf(fp, " = phi ");
   nir_foreach_phi_src(src, instr) {
      if (&src->node != exec_list_get_head(&instr->srcs))
            fprintf(fp, "b%u: ", src->pred->index);
         }
      static void
   print_parallel_copy_instr(nir_parallel_copy_instr *instr, print_state *state)
   {
      FILE *fp = state->fp;
   nir_foreach_parallel_copy_entry(entry, instr) {
      if (&entry->node != exec_list_get_head(&instr->entries))
            if (entry->dest_is_reg) {
      fprintf(fp, "*");
      } else {
         }
            if (entry->src_is_reg)
               }
      static void
   print_instr(const nir_instr *instr, print_state *state, unsigned tabs)
   {
      FILE *fp = state->fp;
            switch (instr->type) {
   case nir_instr_type_alu:
      print_alu_instr(nir_instr_as_alu(instr), state);
         case nir_instr_type_deref:
      print_deref_instr(nir_instr_as_deref(instr), state);
         case nir_instr_type_call:
      print_call_instr(nir_instr_as_call(instr), state);
         case nir_instr_type_intrinsic:
      print_intrinsic_instr(nir_instr_as_intrinsic(instr), state);
         case nir_instr_type_tex:
      print_tex_instr(nir_instr_as_tex(instr), state);
         case nir_instr_type_load_const:
      print_load_const_instr(nir_instr_as_load_const(instr), state);
         case nir_instr_type_jump:
      print_jump_instr(nir_instr_as_jump(instr), state);
         case nir_instr_type_undef:
      print_ssa_undef_instr(nir_instr_as_undef(instr), state);
         case nir_instr_type_phi:
      print_phi_instr(nir_instr_as_phi(instr), state);
         case nir_instr_type_parallel_copy:
      print_parallel_copy_instr(nir_instr_as_parallel_copy(instr), state);
         default:
      unreachable("Invalid instruction type");
               if (NIR_DEBUG(PRINT_PASS_FLAGS) && instr->pass_flags)
      }
      static bool
   block_has_instruction_with_dest(nir_block *block)
   {
      nir_foreach_instr(instr, block) {
      switch (instr->type) {
   case nir_instr_type_load_const:
   case nir_instr_type_deref:
   case nir_instr_type_alu:
   case nir_instr_type_tex:
   case nir_instr_type_undef:
   case nir_instr_type_phi:
   case nir_instr_type_parallel_copy:
            case nir_instr_type_intrinsic: {
      nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   const nir_intrinsic_info *info = &nir_intrinsic_infos[intrin->intrinsic];
                  /* Doesn't define a new value. */
               case nir_instr_type_jump:
   case nir_instr_type_call:
      /* Doesn't define a new value. */
                     }
      static void print_cf_node(nir_cf_node *node, print_state *state,
            static void
   print_block_preds(nir_block *block, print_state *state)
   {
      FILE *fp = state->fp;
   nir_block **preds = nir_block_get_predecessors_sorted(block, NULL);
   for (unsigned i = 0; i < block->predecessors->entries; i++) {
      if (i != 0)
            }
      }
      static void
   print_block_succs(nir_block *block, print_state *state)
   {
      FILE *fp = state->fp;
   for (unsigned i = 0; i < 2; i++) {
      if (block->successors[i]) {
               }
      static void
   print_block(nir_block *block, print_state *state, unsigned tabs)
   {
               if (block_has_instruction_with_dest(block))
         else
            print_indentation(tabs, fp);
            const bool empty_block = exec_list_is_empty(&block->instr_list);
   if (empty_block) {
      fprintf(fp, "  // preds: ");
   print_block_preds(block, state);
   fprintf(fp, ", succs: ");
   print_block_succs(block, state);
   fprintf(fp, "\n");
               const unsigned block_length = 7 + count_digits(block->index) + 1;
            fprintf(fp, "%*s// preds: ", pred_padding, "");
   print_block_preds(block, state);
            nir_foreach_instr(instr, block) {
      print_instr(instr, state, tabs);
   fprintf(fp, "\n");
               print_indentation(tabs, fp);
   fprintf(fp, "%*s// succs: ", state->padding_for_no_dest, "");
   print_block_succs(block, state);
      }
      static void
   print_if(nir_if *if_stmt, print_state *state, unsigned tabs)
   {
               print_indentation(tabs, fp);
   fprintf(fp, "if ");
   print_src(&if_stmt->condition, state, nir_type_invalid);
   switch (if_stmt->control) {
   case nir_selection_control_flatten:
      fprintf(fp, "  // flatten");
      case nir_selection_control_dont_flatten:
      fprintf(fp, "  // don't flatten");
      case nir_selection_control_divergent_always_taken:
      fprintf(fp, "  // divergent always taken");
      case nir_selection_control_none:
   default:
         }
   fprintf(fp, " {\n");
   foreach_list_typed(nir_cf_node, node, node, &if_stmt->then_list) {
         }
   print_indentation(tabs, fp);
   fprintf(fp, "} else {\n");
   foreach_list_typed(nir_cf_node, node, node, &if_stmt->else_list) {
         }
   print_indentation(tabs, fp);
      }
      static void
   print_loop(nir_loop *loop, print_state *state, unsigned tabs)
   {
               print_indentation(tabs, fp);
   fprintf(fp, "loop {\n");
   foreach_list_typed(nir_cf_node, node, node, &loop->body) {
         }
            if (nir_loop_has_continue_construct(loop)) {
      fprintf(fp, "} continue {\n");
   foreach_list_typed(nir_cf_node, node, node, &loop->continue_list) {
         }
                  }
      static void
   print_cf_node(nir_cf_node *node, print_state *state, unsigned int tabs)
   {
      switch (node->type) {
   case nir_cf_node_block:
      print_block(nir_cf_node_as_block(node), state, tabs);
         case nir_cf_node_if:
      print_if(nir_cf_node_as_if(node), state, tabs);
         case nir_cf_node_loop:
      print_loop(nir_cf_node_as_loop(node), state, tabs);
         default:
            }
      static void
   print_function_impl(nir_function_impl *impl, print_state *state)
   {
                                          if (impl->preamble) {
      print_indentation(1, fp);
               if (!NIR_DEBUG(PRINT_NO_INLINE_CONSTS)) {
      /* Don't reindex the SSA as suggested by nir_gather_types() because
   * nir_print don't modify the shader.  If needed, a limit for ssa_alloc
   * can be added.
   */
   state->float_types = calloc(BITSET_WORDS(impl->ssa_alloc), sizeof(BITSET_WORD));
   state->int_types = calloc(BITSET_WORDS(impl->ssa_alloc), sizeof(BITSET_WORD));
               nir_foreach_function_temp_variable(var, impl) {
      print_indentation(1, fp);
                        foreach_list_typed(nir_cf_node, node, node, &impl->body) {
                  print_indentation(1, fp);
            free(state->float_types);
   free(state->int_types);
      }
      static void
   print_function(nir_function *function, print_state *state)
   {
               fprintf(fp, "decl_function %s (%d params) %s", function->name,
                           if (function->impl != NULL) {
      print_function_impl(function->impl, state);
         }
      static void
   init_print_state(print_state *state, nir_shader *shader, FILE *fp)
   {
      state->fp = fp;
   state->shader = shader;
   state->ht = _mesa_pointer_hash_table_create(NULL);
   state->syms = _mesa_set_create(NULL, _mesa_hash_string,
         state->index = 0;
   state->int_types = NULL;
   state->float_types = NULL;
   state->max_dest_index = 0;
      }
      static void
   destroy_print_state(print_state *state)
   {
      _mesa_hash_table_destroy(state->ht, NULL);
      }
      static const char *
   primitive_name(unsigned primitive)
   {
   #define PRIM(X)        \
      case MESA_PRIM_##X: \
         switch (primitive) {
      PRIM(POINTS);
   PRIM(LINES);
   PRIM(LINE_LOOP);
   PRIM(LINE_STRIP);
   PRIM(TRIANGLES);
   PRIM(TRIANGLE_STRIP);
   PRIM(TRIANGLE_FAN);
   PRIM(QUADS);
   PRIM(QUAD_STRIP);
      default:
            }
      static void
   print_bitset(FILE *fp, const char *label, const unsigned *words, int size)
   {
      fprintf(fp, "%s: ", label);
   /* Iterate back-to-front to get proper digit order (most significant first). */
   for (int i = size - 1; i >= 0; --i) {
         }
      }
      /* Print bitset, only if some bits are set */
   static void
   print_nz_bitset(FILE *fp, const char *label, const unsigned *words, int size)
   {
      bool is_all_zero = true;
   for (int i = 0; i < size; ++i) {
      if (words[i]) {
      is_all_zero = false;
                  if (!is_all_zero)
      }
      /* Print uint64_t value, only if non-zero.
   * The value is printed by enumerating the ranges of bits that are set.
   * E.g. inputs_read: 0,15-17
   */
   static void
   print_nz_x64(FILE *fp, const char *label, uint64_t value)
   {
      if (value) {
      char acc[256] = { 0 };
   char buf[32];
   int start = 0;
   int count = 0;
   while (value) {
      u_bit_scan_consecutive_range64(&value, &start, &count);
   assert(count > 0);
   bool is_first = !acc[0];
   if (count > 1) {
         } else {
         }
   assert(strlen(acc) + strlen(buf) + 1 < sizeof(acc));
      }
         }
      /* Print uint32_t value in hex, only if non-zero */
   static void
   print_nz_x32(FILE *fp, const char *label, uint32_t value)
   {
      if (value)
      }
      /* Print uint16_t value in hex, only if non-zero */
   static void
   print_nz_x16(FILE *fp, const char *label, uint16_t value)
   {
      if (value)
      }
      /* Print uint8_t value in hex, only if non-zero */
   static void
   print_nz_x8(FILE *fp, const char *label, uint8_t value)
   {
      if (value)
      }
      /* Print unsigned value in decimal, only if non-zero */
   static void
   print_nz_unsigned(FILE *fp, const char *label, unsigned value)
   {
      if (value)
      }
      /* Print bool only if set */
   static void
   print_nz_bool(FILE *fp, const char *label, bool value)
   {
      if (value)
      }
      static void
   print_shader_info(const struct shader_info *info, FILE *fp)
   {
               fprintf(fp, "source_sha1: {");
   _mesa_sha1_print(fp, info->source_sha1);
            if (info->name)
            if (info->label)
                     if (gl_shader_stage_uses_workgroup(info->stage)) {
      fprintf(fp, "workgroup-size: %u, %u, %u%s\n",
         info->workgroup_size[0],
   info->workgroup_size[1],
   info->workgroup_size[2],
               fprintf(fp, "stage: %d\n"
                  print_nz_unsigned(fp, "num_textures", info->num_textures);
   print_nz_unsigned(fp, "num_ubos", info->num_ubos);
   print_nz_unsigned(fp, "num_abos", info->num_abos);
   print_nz_unsigned(fp, "num_ssbos", info->num_ssbos);
            print_nz_x64(fp, "inputs_read", info->inputs_read);
   print_nz_x64(fp, "dual_slot_inputs", info->dual_slot_inputs);
   print_nz_x64(fp, "outputs_written", info->outputs_written);
                     print_nz_x64(fp, "per_primitive_inputs", info->per_primitive_inputs);
   print_nz_x64(fp, "per_primitive_outputs", info->per_primitive_outputs);
            print_nz_x16(fp, "inputs_read_16bit", info->inputs_read_16bit);
   print_nz_x16(fp, "outputs_written_16bit", info->outputs_written_16bit);
   print_nz_x16(fp, "outputs_read_16bit", info->outputs_read_16bit);
   print_nz_x16(fp, "inputs_read_indirectly_16bit", info->inputs_read_indirectly_16bit);
            print_nz_x32(fp, "patch_inputs_read", info->patch_inputs_read);
   print_nz_x32(fp, "patch_outputs_written", info->patch_outputs_written);
            print_nz_x64(fp, "inputs_read_indirectly", info->inputs_read_indirectly);
   print_nz_x64(fp, "outputs_accessed_indirectly", info->outputs_accessed_indirectly);
   print_nz_x64(fp, "patch_inputs_read_indirectly", info->patch_inputs_read_indirectly);
            print_nz_bitset(fp, "textures_used", info->textures_used, ARRAY_SIZE(info->textures_used));
   print_nz_bitset(fp, "textures_used_by_txf", info->textures_used_by_txf, ARRAY_SIZE(info->textures_used_by_txf));
   print_nz_bitset(fp, "samplers_used", info->samplers_used, ARRAY_SIZE(info->samplers_used));
   print_nz_bitset(fp, "images_used", info->images_used, ARRAY_SIZE(info->images_used));
   print_nz_bitset(fp, "image_buffers", info->image_buffers, ARRAY_SIZE(info->image_buffers));
                              if (info->stage == MESA_SHADER_MESH || info->stage == MESA_SHADER_TASK) {
                                             bool has_xfb_stride = info->xfb_stride[0] || info->xfb_stride[1] || info->xfb_stride[2] || info->xfb_stride[3];
   if (has_xfb_stride)
      fprintf(fp, "xfb_stride: {%u, %u, %u, %u}\n",
         info->xfb_stride[0],
   info->xfb_stride[1],
         bool has_inlinable_uniform_dw_offsets = info->inlinable_uniform_dw_offsets[0] || info->inlinable_uniform_dw_offsets[1] || info->inlinable_uniform_dw_offsets[2] || info->inlinable_uniform_dw_offsets[3];
   if (has_inlinable_uniform_dw_offsets)
      fprintf(fp, "inlinable_uniform_dw_offsets: {%u, %u, %u, %u}\n",
         info->inlinable_uniform_dw_offsets[0],
   info->inlinable_uniform_dw_offsets[1],
         print_nz_unsigned(fp, "num_inlinable_uniforms", info->num_inlinable_uniforms);
   print_nz_unsigned(fp, "clip_distance_array_size", info->clip_distance_array_size);
            print_nz_bool(fp, "uses_texture_gather", info->uses_texture_gather);
   print_nz_bool(fp, "uses_resource_info_query", info->uses_resource_info_query);
   print_nz_bool(fp, "uses_fddx_fddy", info->uses_fddx_fddy);
            print_nz_x8(fp, "bit_sizes_float", info->bit_sizes_float);
            print_nz_bool(fp, "first_ubo_is_default_ubo", info->first_ubo_is_default_ubo);
   print_nz_bool(fp, "separate_shader", info->separate_shader);
   print_nz_bool(fp, "has_transform_feedback_varyings", info->has_transform_feedback_varyings);
   print_nz_bool(fp, "flrp_lowered", info->flrp_lowered);
   print_nz_bool(fp, "io_lowered", info->io_lowered);
            switch (info->stage) {
   case MESA_SHADER_VERTEX:
      print_nz_x64(fp, "double_inputs", info->vs.double_inputs);
   print_nz_unsigned(fp, "blit_sgprs_amd", info->vs.blit_sgprs_amd);
   print_nz_bool(fp, "window_space_position", info->vs.window_space_position);
   print_nz_bool(fp, "needs_edge_flag", info->vs.needs_edge_flag);
         case MESA_SHADER_TESS_CTRL:
   case MESA_SHADER_TESS_EVAL:
      fprintf(fp, "primitive_mode: %u\n", info->tess._primitive_mode);
   fprintf(fp, "tcs_vertices_out: %u\n", info->tess.tcs_vertices_out);
            print_nz_bool(fp, "ccw", info->tess.ccw);
   print_nz_bool(fp, "point_mode", info->tess.point_mode);
   print_nz_x64(fp, "tcs_cross_invocation_inputs_read", info->tess.tcs_cross_invocation_inputs_read);
   print_nz_x64(fp, "tcs_cross_invocation_outputs_read", info->tess.tcs_cross_invocation_outputs_read);
         case MESA_SHADER_GEOMETRY:
      fprintf(fp, "output_primitive: %s\n", primitive_name(info->gs.output_primitive));
   fprintf(fp, "input_primitive: %s\n", primitive_name(info->gs.input_primitive));
   fprintf(fp, "vertices_out: %u\n", info->gs.vertices_out);
   fprintf(fp, "invocations: %u\n", info->gs.invocations);
   fprintf(fp, "vertices_in: %u\n", info->gs.vertices_in);
   print_nz_bool(fp, "uses_end_primitive", info->gs.uses_end_primitive);
   fprintf(fp, "active_stream_mask: 0x%02x\n", info->gs.active_stream_mask);
         case MESA_SHADER_FRAGMENT:
      print_nz_bool(fp, "uses_discard", info->fs.uses_discard);
   print_nz_bool(fp, "uses_demote", info->fs.uses_demote);
   print_nz_bool(fp, "uses_fbfetch_output", info->fs.uses_fbfetch_output);
            print_nz_bool(fp, "needs_quad_helper_invocations", info->fs.needs_quad_helper_invocations);
   print_nz_bool(fp, "needs_all_helper_invocations", info->fs.needs_all_helper_invocations);
   print_nz_bool(fp, "uses_sample_qualifier", info->fs.uses_sample_qualifier);
   print_nz_bool(fp, "uses_sample_shading", info->fs.uses_sample_shading);
   print_nz_bool(fp, "early_fragment_tests", info->fs.early_fragment_tests);
   print_nz_bool(fp, "inner_coverage", info->fs.inner_coverage);
            print_nz_bool(fp, "pixel_center_integer", info->fs.pixel_center_integer);
   print_nz_bool(fp, "origin_upper_left", info->fs.origin_upper_left);
   print_nz_bool(fp, "pixel_interlock_ordered", info->fs.pixel_interlock_ordered);
   print_nz_bool(fp, "pixel_interlock_unordered", info->fs.pixel_interlock_unordered);
   print_nz_bool(fp, "sample_interlock_ordered", info->fs.sample_interlock_ordered);
   print_nz_bool(fp, "sample_interlock_unordered", info->fs.sample_interlock_unordered);
                     if (info->fs.color0_interp != INTERP_MODE_NONE) {
      fprintf(fp, "color0_interp: %s\n",
      }
   print_nz_bool(fp, "color0_sample", info->fs.color0_sample);
            if (info->fs.color1_interp != INTERP_MODE_NONE) {
      fprintf(fp, "color1_interp: %s\n",
      }
   print_nz_bool(fp, "color1_sample", info->fs.color1_sample);
            print_nz_x32(fp, "advanced_blend_modes", info->fs.advanced_blend_modes);
         case MESA_SHADER_COMPUTE:
   case MESA_SHADER_KERNEL:
      if (info->cs.workgroup_size_hint[0] || info->cs.workgroup_size_hint[1] || info->cs.workgroup_size_hint[2])
      fprintf(fp, "workgroup_size_hint: {%u, %u, %u}\n",
         info->cs.workgroup_size_hint[0],
      print_nz_unsigned(fp, "user_data_components_amd", info->cs.user_data_components_amd);
   print_nz_unsigned(fp, "derivative_group", info->cs.derivative_group);
   fprintf(fp, "ptr_size: %u\n", info->cs.ptr_size);
         case MESA_SHADER_MESH:
      print_nz_x64(fp, "ms_cross_invocation_output_access", info->mesh.ms_cross_invocation_output_access);
   fprintf(fp, "max_vertices_out: %u\n", info->mesh.max_vertices_out);
   fprintf(fp, "max_primitives_out: %u\n", info->mesh.max_primitives_out);
   fprintf(fp, "primitive_type: %s\n", primitive_name(info->mesh.primitive_type));
   print_nz_bool(fp, "nv", info->mesh.nv);
         default:
            }
      void
   nir_print_shader_annotated(nir_shader *shader, FILE *fp,
         {
      print_state state;
   init_print_state(&state, shader, fp);
                     fprintf(fp, "inputs: %u\n", shader->num_inputs);
   fprintf(fp, "outputs: %u\n", shader->num_outputs);
   fprintf(fp, "uniforms: %u\n", shader->num_uniforms);
   if (shader->scratch_size)
         if (shader->constant_data_size)
         for (unsigned i = 0; i < nir_num_variable_modes; i++) {
      if (BITFIELD_BIT(i) == nir_var_function_temp)
         nir_foreach_variable_with_modes(var, shader, BITFIELD_BIT(i))
               foreach_list_typed(nir_function, func, node, &shader->functions) {
                     }
      void
   nir_print_shader(nir_shader *shader, FILE *fp)
   {
      nir_print_shader_annotated(shader, fp, NULL);
      }
      char *
   nir_shader_as_str_annotated(nir_shader *nir, struct hash_table *annotations, void *mem_ctx)
   {
      char *stream_data = NULL;
   size_t stream_size = 0;
   struct u_memstream mem;
   if (u_memstream_open(&mem, &stream_data, &stream_size)) {
      FILE *const stream = u_memstream_get(&mem);
   nir_print_shader_annotated(nir, stream, annotations);
               char *str = ralloc_size(mem_ctx, stream_size + 1);
   memcpy(str, stream_data, stream_size);
                        }
      char *
   nir_shader_as_str(nir_shader *nir, void *mem_ctx)
   {
         }
      void
   nir_print_instr(const nir_instr *instr, FILE *fp)
   {
      print_state state = {
         };
   if (instr->block) {
      nir_function_impl *impl = nir_cf_node_get_function(&instr->block->cf_node);
                  }
      char *
   nir_instr_as_str(const nir_instr *instr, void *mem_ctx)
   {
      char *stream_data = NULL;
   size_t stream_size = 0;
   struct u_memstream mem;
   if (u_memstream_open(&mem, &stream_data, &stream_size)) {
      FILE *const stream = u_memstream_get(&mem);
   nir_print_instr(instr, stream);
               char *str = ralloc_size(mem_ctx, stream_size + 1);
   memcpy(str, stream_data, stream_size);
                        }
      void
   nir_print_deref(const nir_deref_instr *deref, FILE *fp)
   {
      print_state state = {
         };
      }
      void
   nir_log_shader_annotated_tagged(enum mesa_log_level level, const char *tag,
         {
      char *str = nir_shader_as_str_annotated(shader, annotations, NULL);
   _mesa_log_multiline(level, tag, str);
      }
