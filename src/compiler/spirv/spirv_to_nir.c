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
   *
   * Authors:
   *    Faith Ekstrand (faith@gfxstrand.net)
   *
   */
      #include "glsl_types.h"
   #include "vtn_private.h"
   #include "nir/nir_vla.h"
   #include "nir/nir_control_flow.h"
   #include "nir/nir_constant_expressions.h"
   #include "nir/nir_deref.h"
   #include "spirv_info.h"
      #include "util/format/u_format.h"
   #include "util/u_math.h"
   #include "util/u_string.h"
   #include "util/u_debug.h"
      #include <stdio.h>
      #ifndef NDEBUG
   uint32_t mesa_spirv_debug = 0;
      static const struct debug_named_value mesa_spirv_debug_control[] = {
      { "structured", MESA_SPIRV_DEBUG_STRUCTURED,
   "Print information of the SPIR-V structured control flow parsing" },
      };
      DEBUG_GET_ONCE_FLAGS_OPTION(mesa_spirv_debug, "MESA_SPIRV_DEBUG", mesa_spirv_debug_control, 0)
      static enum nir_spirv_debug_level
   vtn_default_log_level(void)
   {
      enum nir_spirv_debug_level level = NIR_SPIRV_DEBUG_LEVEL_WARNING;
   const char *vtn_log_level_strings[] = {
      [NIR_SPIRV_DEBUG_LEVEL_WARNING] = "warning",
   [NIR_SPIRV_DEBUG_LEVEL_INFO]  = "info",
      };
            if (str == NULL)
            for (int i = 0; i < ARRAY_SIZE(vtn_log_level_strings); i++) {
      if (strcasecmp(str, vtn_log_level_strings[i]) == 0) {
      level = i;
                     }
   #endif
      void
   vtn_log(struct vtn_builder *b, enum nir_spirv_debug_level level,
         {
      if (b->options->debug.func) {
      b->options->debug.func(b->options->debug.private_data,
            #ifndef NDEBUG
      static enum nir_spirv_debug_level default_level =
            if (default_level == NIR_SPIRV_DEBUG_LEVEL_INVALID)
            if (level >= default_level)
      #endif
   }
      void
   vtn_logf(struct vtn_builder *b, enum nir_spirv_debug_level level,
         {
      va_list args;
            va_start(args, fmt);
   msg = ralloc_vasprintf(NULL, fmt, args);
                        }
      static void
   vtn_log_err(struct vtn_builder *b,
               enum nir_spirv_debug_level level, const char *prefix,
   {
                     #ifndef NDEBUG
         #endif
                           ralloc_asprintf_append(&msg, "\n    %zu bytes into the SPIR-V binary",
            if (b->file) {
      ralloc_asprintf_append(&msg,
                                 }
      static void
   vtn_dump_shader(struct vtn_builder *b, const char *path, const char *prefix)
   {
               char filename[1024];
   int len = snprintf(filename, sizeof(filename), "%s/%s-%d.spirv",
         if (len < 0 || len >= sizeof(filename))
            FILE *f = fopen(filename, "wb");
   if (f == NULL)
            fwrite(b->spirv, sizeof(*b->spirv), b->spirv_word_count, f);
               }
      void
   _vtn_warn(struct vtn_builder *b, const char *file, unsigned line,
         {
               va_start(args, fmt);
   vtn_log_err(b, NIR_SPIRV_DEBUG_LEVEL_WARNING, "SPIR-V WARNING:\n",
            }
      void
   _vtn_err(struct vtn_builder *b, const char *file, unsigned line,
         {
               va_start(args, fmt);
   vtn_log_err(b, NIR_SPIRV_DEBUG_LEVEL_ERROR, "SPIR-V ERROR:\n",
            }
      void
   _vtn_fail(struct vtn_builder *b, const char *file, unsigned line,
         {
               va_start(args, fmt);
   vtn_log_err(b, NIR_SPIRV_DEBUG_LEVEL_ERROR, "SPIR-V parsing FAILED:\n",
                  const char *dump_path = getenv("MESA_SPIRV_FAIL_DUMP_PATH");
   if (dump_path)
         #ifndef NDEBUG
      if (!b->options->skip_os_break_in_debug_build)
      #endif
            }
      const char *
   vtn_value_type_to_string(enum vtn_value_type t)
   {
   #define CASE(typ) case vtn_value_type_##typ: return #typ
      switch (t) {
   CASE(invalid);
   CASE(undef);
   CASE(string);
   CASE(decoration_group);
   CASE(type);
   CASE(constant);
   CASE(pointer);
   CASE(function);
   CASE(block);
   CASE(ssa);
   CASE(extension);
   CASE(image_pointer);
      #undef CASE
      unreachable("unknown value type");
      }
      void
   _vtn_fail_value_type_mismatch(struct vtn_builder *b, uint32_t value_id,
         {
      struct vtn_value *val = vtn_untyped_value(b, value_id);
   vtn_fail(
      "SPIR-V id %u is the wrong kind of value: "
   "expected '%s' but got '%s'",
   vtn_id_for_value(b, val),
   vtn_value_type_to_string(value_type),
   }
      void _vtn_fail_value_not_pointer(struct vtn_builder *b,
         {
      struct vtn_value *val = vtn_untyped_value(b, value_id);
   vtn_fail("SPIR-V id %u is the wrong kind of value: "
            "expected 'pointer' OR null constant but got "
   "'%s' (%s)", value_id,
   }
      static struct vtn_ssa_value *
   vtn_undef_ssa_value(struct vtn_builder *b, const struct glsl_type *type)
   {
      struct vtn_ssa_value *val = rzalloc(b, struct vtn_ssa_value);
            if (glsl_type_is_cmat(type)) {
      nir_deref_instr *mat = vtn_create_cmat_temporary(b, type, "cmat_undef");
      } else if (glsl_type_is_vector_or_scalar(type)) {
      unsigned num_components = glsl_get_vector_elements(val->type);
   unsigned bit_size = glsl_get_bit_size(val->type);
      } else {
      unsigned elems = glsl_get_length(val->type);
   val->elems = ralloc_array(b, struct vtn_ssa_value *, elems);
   if (glsl_type_is_array_or_matrix(type)) {
      const struct glsl_type *elem_type = glsl_get_array_element(type);
   for (unsigned i = 0; i < elems; i++)
      } else {
      vtn_assert(glsl_type_is_struct_or_ifc(type));
   for (unsigned i = 0; i < elems; i++) {
      const struct glsl_type *elem_type = glsl_get_struct_field(type, i);
                        }
      struct vtn_ssa_value *
   vtn_const_ssa_value(struct vtn_builder *b, nir_constant *constant,
         {
      struct vtn_ssa_value *val = rzalloc(b, struct vtn_ssa_value);
            if (glsl_type_is_cmat(type)) {
               nir_deref_instr *mat = vtn_create_cmat_temporary(b, type, "cmat_constant");
   nir_cmat_construct(&b->nb, &mat->def,
                  } else if (glsl_type_is_vector_or_scalar(type)) {
      val->def = nir_build_imm(&b->nb, glsl_get_vector_elements(val->type),
            } else {
      unsigned elems = glsl_get_length(val->type);
   val->elems = ralloc_array(b, struct vtn_ssa_value *, elems);
   if (glsl_type_is_array_or_matrix(type)) {
      const struct glsl_type *elem_type = glsl_get_array_element(type);
   for (unsigned i = 0; i < elems; i++) {
      val->elems[i] = vtn_const_ssa_value(b, constant->elements[i],
         } else {
      vtn_assert(glsl_type_is_struct_or_ifc(type));
   for (unsigned i = 0; i < elems; i++) {
      const struct glsl_type *elem_type = glsl_get_struct_field(type, i);
   val->elems[i] = vtn_const_ssa_value(b, constant->elements[i],
                        }
      struct vtn_ssa_value *
   vtn_ssa_value(struct vtn_builder *b, uint32_t value_id)
   {
      struct vtn_value *val = vtn_untyped_value(b, value_id);
   switch (val->value_type) {
   case vtn_value_type_undef:
            case vtn_value_type_constant:
            case vtn_value_type_ssa:
            case vtn_value_type_pointer:
      vtn_assert(val->pointer->ptr_type && val->pointer->ptr_type->type);
   struct vtn_ssa_value *ssa =
         ssa->def = vtn_pointer_to_ssa(b, val->pointer);
         default:
            }
      struct vtn_value *
   vtn_push_ssa_value(struct vtn_builder *b, uint32_t value_id,
         {
               /* See vtn_create_ssa_value */
   vtn_fail_if(ssa->type != glsl_get_bare_type(type->type),
            struct vtn_value *val;
   if (type->base_type == vtn_base_type_pointer) {
         } else {
      /* Don't trip the value_type_ssa check in vtn_push_value */
   val = vtn_push_value(b, value_id, vtn_value_type_invalid);
   val->value_type = vtn_value_type_ssa;
                  }
      nir_def *
   vtn_get_nir_ssa(struct vtn_builder *b, uint32_t value_id)
   {
      struct vtn_ssa_value *ssa = vtn_ssa_value(b, value_id);
   vtn_fail_if(!glsl_type_is_vector_or_scalar(ssa->type),
            }
      struct vtn_value *
   vtn_push_nir_ssa(struct vtn_builder *b, uint32_t value_id, nir_def *def)
   {
      /* Types for all SPIR-V SSA values are set as part of a pre-pass so the
   * type will be valid by the time we get here.
   */
   struct vtn_type *type = vtn_get_value_type(b, value_id);
   vtn_fail_if(def->num_components != glsl_get_vector_elements(type->type) ||
               struct vtn_ssa_value *ssa = vtn_create_ssa_value(b, type->type);
   ssa->def = def;
      }
      nir_deref_instr *
   vtn_get_deref_for_id(struct vtn_builder *b, uint32_t value_id)
   {
         }
      nir_deref_instr *
   vtn_get_deref_for_ssa_value(struct vtn_builder *b, struct vtn_ssa_value *ssa)
   {
      vtn_fail_if(!ssa->is_variable, "Expected an SSA value with a nir_variable");
      }
      struct vtn_value *
   vtn_push_var_ssa(struct vtn_builder *b, uint32_t value_id, nir_variable *var)
   {
      struct vtn_ssa_value *ssa = vtn_create_ssa_value(b, var->type);
   vtn_set_ssa_value_var(b, ssa, var);
      }
      static enum gl_access_qualifier
   spirv_to_gl_access_qualifier(struct vtn_builder *b,
         {
      switch (access_qualifier) {
   case SpvAccessQualifierReadOnly:
         case SpvAccessQualifierWriteOnly:
         case SpvAccessQualifierReadWrite:
         default:
            }
      static nir_deref_instr *
   vtn_get_image(struct vtn_builder *b, uint32_t value_id,
         {
      struct vtn_type *type = vtn_get_value_type(b, value_id);
   vtn_assert(type->base_type == vtn_base_type_image);
   if (access)
         nir_variable_mode mode = glsl_type_is_image(type->glsl_image) ?
         return nir_build_deref_cast(&b->nb, vtn_get_nir_ssa(b, value_id),
      }
      static void
   vtn_push_image(struct vtn_builder *b, uint32_t value_id,
         {
      struct vtn_type *type = vtn_get_value_type(b, value_id);
   vtn_assert(type->base_type == vtn_base_type_image);
   struct vtn_value *value = vtn_push_nir_ssa(b, value_id, &deref->def);
      }
      static nir_deref_instr *
   vtn_get_sampler(struct vtn_builder *b, uint32_t value_id)
   {
      struct vtn_type *type = vtn_get_value_type(b, value_id);
   vtn_assert(type->base_type == vtn_base_type_sampler);
   return nir_build_deref_cast(&b->nb, vtn_get_nir_ssa(b, value_id),
      }
      nir_def *
   vtn_sampled_image_to_nir_ssa(struct vtn_builder *b,
         {
         }
      static void
   vtn_push_sampled_image(struct vtn_builder *b, uint32_t value_id,
         {
      struct vtn_type *type = vtn_get_value_type(b, value_id);
   vtn_assert(type->base_type == vtn_base_type_sampled_image);
   struct vtn_value *value = vtn_push_nir_ssa(b, value_id,
            }
      static struct vtn_sampled_image
   vtn_get_sampled_image(struct vtn_builder *b, uint32_t value_id)
   {
      struct vtn_type *type = vtn_get_value_type(b, value_id);
   vtn_assert(type->base_type == vtn_base_type_sampled_image);
            /* Even though this is a sampled image, we can end up here with a storage
   * image because OpenCL doesn't distinguish between the two.
   */
   const struct glsl_type *image_type = type->image->glsl_image;
   nir_variable_mode image_mode = glsl_type_is_image(image_type) ?
            struct vtn_sampled_image si = { NULL, };
   si.image = nir_build_deref_cast(&b->nb, nir_channel(&b->nb, si_vec2, 0),
         si.sampler = nir_build_deref_cast(&b->nb, nir_channel(&b->nb, si_vec2, 1),
                  }
      const char *
   vtn_string_literal(struct vtn_builder *b, const uint32_t *words,
         {
      /* From the SPIR-V spec:
   *
   *    "A string is interpreted as a nul-terminated stream of characters.
   *    The character set is Unicode in the UTF-8 encoding scheme. The UTF-8
   *    octets (8-bit bytes) are packed four per word, following the
   *    little-endian convention (i.e., the first octet is in the
   *    lowest-order 8 bits of the word). The final word contains the
   *    string’s nul-termination character (0), and all contents past the
   *    end of the string in the final word are padded with 0."
   *
   * On big-endian, we need to byte-swap.
      #if UTIL_ARCH_BIG_ENDIAN
      {
      uint32_t *copy = ralloc_array(b, uint32_t, word_count);
   for (unsigned i = 0; i < word_count; i++)
               #endif
         const char *str = (char *)words;
   const char *end = memchr(str, 0, word_count * 4);
            if (words_used)
               }
      const uint32_t *
   vtn_foreach_instruction(struct vtn_builder *b, const uint32_t *start,
         {
      b->file = NULL;
   b->line = -1;
            const uint32_t *w = start;
   while (w < end) {
      SpvOp opcode = w[0] & SpvOpCodeMask;
   unsigned count = w[0] >> SpvWordCountShift;
                     switch (opcode) {
   case SpvOpNop:
            case SpvOpLine:
      b->file = vtn_value(b, w[1], vtn_value_type_string)->str;
   b->line = w[2];
               case SpvOpNoLine:
      b->file = NULL;
   b->line = -1;
               default:
      if (!handler(b, opcode, w, count))
                                 b->spirv_offset = 0;
   b->file = NULL;
   b->line = -1;
            assert(w == end);
      }
      static bool
   vtn_handle_non_semantic_instruction(struct vtn_builder *b, SpvOp ext_opcode,
         {
      /* Do nothing. */
      }
      static void
   vtn_handle_extension(struct vtn_builder *b, SpvOp opcode,
         {
      switch (opcode) {
   case SpvOpExtInstImport: {
      struct vtn_value *val = vtn_push_value(b, w[1], vtn_value_type_extension);
   const char *ext = vtn_string_literal(b, &w[2], count - 2, NULL);
   if (strcmp(ext, "GLSL.std.450") == 0) {
         } else if ((strcmp(ext, "SPV_AMD_gcn_shader") == 0)
               } else if ((strcmp(ext, "SPV_AMD_shader_ballot") == 0)
               } else if ((strcmp(ext, "SPV_AMD_shader_trinary_minmax") == 0)
               } else if ((strcmp(ext, "SPV_AMD_shader_explicit_vertex_parameter") == 0)
               } else if (strcmp(ext, "OpenCL.std") == 0) {
         } else if (strstr(ext, "NonSemantic.") == ext) {
         } else {
         }
               case SpvOpExtInst: {
      struct vtn_value *val = vtn_value(b, w[3], vtn_value_type_extension);
   bool handled = val->ext_handler(b, w[4], w, count);
   vtn_assert(handled);
               default:
            }
      static void
   _foreach_decoration_helper(struct vtn_builder *b,
                           {
      for (struct vtn_decoration *dec = value->decoration; dec; dec = dec->next) {
      int member;
   if (dec->scope == VTN_DEC_DECORATION) {
         } else if (dec->scope >= VTN_DEC_STRUCT_MEMBER0) {
      vtn_fail_if(value->value_type != vtn_value_type_type ||
               value->type->base_type != vtn_base_type_struct,
                           vtn_fail_if(member >= base_value->type->length,
                  } else {
      /* Not a decoration */
   assert(dec->scope == VTN_DEC_EXECUTION_MODE ||
                     if (dec->group) {
      assert(dec->group->value_type == vtn_value_type_decoration_group);
   _foreach_decoration_helper(b, base_value, member, dec->group,
      } else {
               }
      /** Iterates (recursively if needed) over all of the decorations on a value
   *
   * This function iterates over all of the decorations applied to a given
   * value.  If it encounters a decoration group, it recurses into the group
   * and iterates over all of those decorations as well.
   */
   void
   vtn_foreach_decoration(struct vtn_builder *b, struct vtn_value *value,
         {
         }
      void
   vtn_foreach_execution_mode(struct vtn_builder *b, struct vtn_value *value,
         {
      for (struct vtn_decoration *dec = value->decoration; dec; dec = dec->next) {
      if (dec->scope != VTN_DEC_EXECUTION_MODE)
            assert(dec->group == NULL);
         }
      void
   vtn_handle_decoration(struct vtn_builder *b, SpvOp opcode,
         {
      const uint32_t *w_end = w + count;
   const uint32_t target = w[1];
            switch (opcode) {
   case SpvOpDecorationGroup:
      vtn_push_value(b, target, vtn_value_type_decoration_group);
         case SpvOpDecorate:
   case SpvOpDecorateId:
   case SpvOpMemberDecorate:
   case SpvOpDecorateString:
   case SpvOpMemberDecorateString:
   case SpvOpExecutionMode:
   case SpvOpExecutionModeId: {
               struct vtn_decoration *dec = rzalloc(b, struct vtn_decoration);
   switch (opcode) {
   case SpvOpDecorate:
   case SpvOpDecorateId:
   case SpvOpDecorateString:
      dec->scope = VTN_DEC_DECORATION;
      case SpvOpMemberDecorate:
   case SpvOpMemberDecorateString:
      dec->scope = VTN_DEC_STRUCT_MEMBER0 + *(w++);
   vtn_fail_if(dec->scope < VTN_DEC_STRUCT_MEMBER0, /* overflow */
            case SpvOpExecutionMode:
   case SpvOpExecutionModeId:
      dec->scope = VTN_DEC_EXECUTION_MODE;
      default:
         }
   dec->decoration = *(w++);
   dec->num_operands = w_end - w;
            /* Link into the list */
   dec->next = val->decoration;
   val->decoration = dec;
               case SpvOpMemberName: {
      struct vtn_value *val = vtn_untyped_value(b, target);
                              dec->next = val->decoration;
   val->decoration = dec;
               case SpvOpGroupMemberDecorate:
   case SpvOpGroupDecorate: {
      struct vtn_value *group =
            for (; w < w_end; w++) {
                     dec->group = group;
   if (opcode == SpvOpGroupDecorate) {
         } else {
      dec->scope = VTN_DEC_STRUCT_MEMBER0 + *(++w);
   vtn_fail_if(dec->scope < 0, /* Check for overflow */
               /* Link into the list */
   dec->next = val->decoration;
      }
               default:
            }
      struct member_decoration_ctx {
      unsigned num_fields;
   struct glsl_struct_field *fields;
      };
      /**
   * Returns true if the given type contains a struct decorated Block or
   * BufferBlock
   */
   bool
   vtn_type_contains_block(struct vtn_builder *b, struct vtn_type *type)
   {
      switch (type->base_type) {
   case vtn_base_type_array:
         case vtn_base_type_struct:
      if (type->block || type->buffer_block)
         for (unsigned i = 0; i < type->length; i++) {
      if (vtn_type_contains_block(b, type->members[i]))
      }
      default:
            }
      /** Returns true if two types are "compatible", i.e. you can do an OpLoad,
   * OpStore, or OpCopyMemory between them without breaking anything.
   * Technically, the SPIR-V rules require the exact same type ID but this lets
   * us internally be a bit looser.
   */
   bool
   vtn_types_compatible(struct vtn_builder *b,
         {
      if (t1->id == t2->id)
            if (t1->base_type != t2->base_type)
            switch (t1->base_type) {
   case vtn_base_type_void:
   case vtn_base_type_scalar:
   case vtn_base_type_vector:
   case vtn_base_type_matrix:
   case vtn_base_type_image:
   case vtn_base_type_sampler:
   case vtn_base_type_sampled_image:
   case vtn_base_type_event:
   case vtn_base_type_cooperative_matrix:
            case vtn_base_type_array:
      return t1->length == t2->length &&
         case vtn_base_type_pointer:
            case vtn_base_type_struct:
      if (t1->length != t2->length)
            for (unsigned i = 0; i < t1->length; i++) {
      if (!vtn_types_compatible(b, t1->members[i], t2->members[i]))
      }
         case vtn_base_type_accel_struct:
   case vtn_base_type_ray_query:
            case vtn_base_type_function:
      /* This case shouldn't get hit since you can't copy around function
   * types.  Just require them to be identical.
   */
                  }
      struct vtn_type *
   vtn_type_without_array(struct vtn_type *type)
   {
      while (type->base_type == vtn_base_type_array)
            }
      /* does a shallow copy of a vtn_type */
      static struct vtn_type *
   vtn_type_copy(struct vtn_builder *b, struct vtn_type *src)
   {
      struct vtn_type *dest = ralloc(b, struct vtn_type);
            switch (src->base_type) {
   case vtn_base_type_void:
   case vtn_base_type_scalar:
   case vtn_base_type_vector:
   case vtn_base_type_matrix:
   case vtn_base_type_array:
   case vtn_base_type_pointer:
   case vtn_base_type_image:
   case vtn_base_type_sampler:
   case vtn_base_type_sampled_image:
   case vtn_base_type_event:
   case vtn_base_type_accel_struct:
   case vtn_base_type_ray_query:
   case vtn_base_type_cooperative_matrix:
      /* Nothing more to do */
         case vtn_base_type_struct:
      dest->members = ralloc_array(b, struct vtn_type *, src->length);
   memcpy(dest->members, src->members,
            dest->offsets = ralloc_array(b, unsigned, src->length);
   memcpy(dest->offsets, src->offsets,
               case vtn_base_type_function:
      dest->params = ralloc_array(b, struct vtn_type *, src->length);
   memcpy(dest->params, src->params, src->length * sizeof(src->params[0]));
                  }
      static bool
   vtn_type_needs_explicit_layout(struct vtn_builder *b, struct vtn_type *type,
         {
      /* For OpenCL we never want to strip the info from the types, and it makes
   * type comparisons easier in later stages.
   */
   if (b->options->environment == NIR_SPIRV_OPENCL)
            switch (mode) {
   case vtn_variable_mode_input:
   case vtn_variable_mode_output:
      /* Layout decorations kept because we need offsets for XFB arrays of
   * blocks.
   */
         case vtn_variable_mode_ssbo:
   case vtn_variable_mode_phys_ssbo:
   case vtn_variable_mode_ubo:
   case vtn_variable_mode_push_constant:
   case vtn_variable_mode_shader_record:
            case vtn_variable_mode_workgroup:
            default:
            }
      const struct glsl_type *
   vtn_type_get_nir_type(struct vtn_builder *b, struct vtn_type *type,
         {
      if (mode == vtn_variable_mode_atomic_counter) {
      vtn_fail_if(glsl_without_array(type->type) != glsl_uint_type(),
                           if (mode == vtn_variable_mode_uniform) {
      switch (type->base_type) {
   case vtn_base_type_array: {
                     return glsl_array_type(elem_type, type->length,
               case vtn_base_type_struct: {
      bool need_new_struct = false;
   const uint32_t num_fields = type->length;
   NIR_VLA(struct glsl_struct_field, fields, num_fields);
   for (unsigned i = 0; i < num_fields; i++) {
      fields[i] = *glsl_get_struct_field_data(type->type, i);
   const struct glsl_type *field_nir_type =
         if (fields[i].type != field_nir_type) {
      fields[i].type = field_nir_type;
         }
   if (need_new_struct) {
      if (glsl_type_is_interface(type->type)) {
      return glsl_interface_type(fields, num_fields,
            } else {
      return glsl_struct_type(fields, num_fields,
               } else {
      /* No changes, just pass it on */
                  case vtn_base_type_image:
                  case vtn_base_type_sampler:
            case vtn_base_type_sampled_image:
                  default:
                     if (mode == vtn_variable_mode_image) {
      struct vtn_type *image_type = vtn_type_without_array(type);
   vtn_assert(image_type->base_type == vtn_base_type_image);
               /* Layout decorations are allowed but ignored in certain conditions,
   * to allow SPIR-V generators perform type deduplication.  Discard
   * unnecessary ones when passing to NIR.
   */
   if (!vtn_type_needs_explicit_layout(b, type, mode))
               }
      static struct vtn_type *
   mutable_matrix_member(struct vtn_builder *b, struct vtn_type *type, int member)
   {
      type->members[member] = vtn_type_copy(b, type->members[member]);
            /* We may have an array of matrices.... Oh, joy! */
   while (glsl_type_is_array(type->type)) {
      type->array_element = vtn_type_copy(b, type->array_element);
                           }
      static void
   vtn_handle_access_qualifier(struct vtn_builder *b, struct vtn_type *type,
         {
      type->members[member] = vtn_type_copy(b, type->members[member]);
               }
      static void
   array_stride_decoration_cb(struct vtn_builder *b,
               {
               if (dec->decoration == SpvDecorationArrayStride) {
      if (vtn_type_contains_block(b, type)) {
      vtn_warn("The ArrayStride decoration cannot be applied to an array "
                  } else {
      vtn_fail_if(dec->operands[0] == 0, "ArrayStride must be non-zero");
            }
      static void
   struct_member_decoration_cb(struct vtn_builder *b,
               {
               if (member < 0)
                     switch (dec->decoration) {
   case SpvDecorationRelaxedPrecision:
   case SpvDecorationUniform:
   case SpvDecorationUniformId:
         case SpvDecorationNonWritable:
      vtn_handle_access_qualifier(b, ctx->type, member, ACCESS_NON_WRITEABLE);
      case SpvDecorationNonReadable:
      vtn_handle_access_qualifier(b, ctx->type, member, ACCESS_NON_READABLE);
      case SpvDecorationVolatile:
      vtn_handle_access_qualifier(b, ctx->type, member, ACCESS_VOLATILE);
      case SpvDecorationCoherent:
      vtn_handle_access_qualifier(b, ctx->type, member, ACCESS_COHERENT);
      case SpvDecorationNoPerspective:
      ctx->fields[member].interpolation = INTERP_MODE_NOPERSPECTIVE;
      case SpvDecorationFlat:
      ctx->fields[member].interpolation = INTERP_MODE_FLAT;
      case SpvDecorationExplicitInterpAMD:
      ctx->fields[member].interpolation = INTERP_MODE_EXPLICIT;
      case SpvDecorationCentroid:
      ctx->fields[member].centroid = true;
      case SpvDecorationSample:
      ctx->fields[member].sample = true;
      case SpvDecorationStream:
      /* This is handled later by var_decoration_cb in vtn_variables.c */
      case SpvDecorationLocation:
      ctx->fields[member].location = dec->operands[0];
      case SpvDecorationComponent:
         case SpvDecorationBuiltIn:
      ctx->type->members[member] = vtn_type_copy(b, ctx->type->members[member]);
   ctx->type->members[member]->is_builtin = true;
   ctx->type->members[member]->builtin = dec->operands[0];
   ctx->type->builtin_block = true;
      case SpvDecorationOffset:
      ctx->type->offsets[member] = dec->operands[0];
   ctx->fields[member].offset = dec->operands[0];
      case SpvDecorationMatrixStride:
      /* Handled as a second pass */
      case SpvDecorationColMajor:
         case SpvDecorationRowMajor:
      mutable_matrix_member(b, ctx->type, member)->row_major = true;
         case SpvDecorationPatch:
   case SpvDecorationPerPrimitiveNV:
   case SpvDecorationPerTaskNV:
   case SpvDecorationPerViewNV:
            case SpvDecorationSpecId:
   case SpvDecorationBlock:
   case SpvDecorationBufferBlock:
   case SpvDecorationArrayStride:
   case SpvDecorationGLSLShared:
   case SpvDecorationGLSLPacked:
   case SpvDecorationAliased:
   case SpvDecorationConstant:
   case SpvDecorationIndex:
   case SpvDecorationBinding:
   case SpvDecorationDescriptorSet:
   case SpvDecorationLinkageAttributes:
   case SpvDecorationNoContraction:
   case SpvDecorationInputAttachmentIndex:
   case SpvDecorationCPacked:
      vtn_warn("Decoration not allowed on struct members: %s",
               case SpvDecorationRestrict:
      /* While "Restrict" is invalid for struct members, glslang incorrectly
   * generates it and it ends up hiding actual driver issues in a wall of
   * spam from deqp-vk.  Return it to the above block once the issue is
   * resolved.  https://github.com/KhronosGroup/glslang/issues/703
   */
         case SpvDecorationInvariant:
      /* Also incorrectly generated by glslang, ignore it. */
         case SpvDecorationXfbBuffer:
   case SpvDecorationXfbStride:
      /* This is handled later by var_decoration_cb in vtn_variables.c */
         case SpvDecorationSaturatedConversion:
   case SpvDecorationFuncParamAttr:
   case SpvDecorationFPRoundingMode:
   case SpvDecorationFPFastMathMode:
   case SpvDecorationAlignment:
      if (b->shader->info.stage != MESA_SHADER_KERNEL) {
      vtn_warn("Decoration only allowed for CL-style kernels: %s",
      }
         case SpvDecorationUserSemantic:
   case SpvDecorationUserTypeGOOGLE:
      /* User semantic decorations can safely be ignored by the driver. */
         default:
            }
      /** Chases the array type all the way down to the tail and rewrites the
   * glsl_types to be based off the tail's glsl_type.
   */
   static void
   vtn_array_type_rewrite_glsl_type(struct vtn_type *type)
   {
      if (type->base_type != vtn_base_type_array)
                     type->type = glsl_array_type(type->array_element->type,
      }
      /* Matrix strides are handled as a separate pass because we need to know
   * whether the matrix is row-major or not first.
   */
   static void
   struct_member_matrix_stride_cb(struct vtn_builder *b,
                     {
      if (dec->decoration != SpvDecorationMatrixStride)
            vtn_fail_if(member < 0,
                                 struct vtn_type *mat_type = mutable_matrix_member(b, ctx->type, member);
   if (mat_type->row_major) {
      mat_type->array_element = vtn_type_copy(b, mat_type->array_element);
   mat_type->stride = mat_type->array_element->stride;
            mat_type->type = glsl_explicit_matrix_type(mat_type->type,
            } else {
      vtn_assert(mat_type->array_element->stride > 0);
            mat_type->type = glsl_explicit_matrix_type(mat_type->type,
               /* Now that we've replaced the glsl_type with a properly strided matrix
   * type, rewrite the member type so that it's an array of the proper kind
   * of glsl_type.
   */
   vtn_array_type_rewrite_glsl_type(ctx->type->members[member]);
      }
      static void
   struct_packed_decoration_cb(struct vtn_builder *b,
               {
      vtn_assert(val->type->base_type == vtn_base_type_struct);
   if (dec->decoration == SpvDecorationCPacked) {
      if (b->shader->info.stage != MESA_SHADER_KERNEL) {
      vtn_warn("Decoration only allowed for CL-style kernels: %s",
      }
         }
      static void
   struct_block_decoration_cb(struct vtn_builder *b,
               {
      if (member != -1)
            struct vtn_type *type = val->type;
   if (dec->decoration == SpvDecorationBlock)
         else if (dec->decoration == SpvDecorationBufferBlock)
      }
      static void
   type_decoration_cb(struct vtn_builder *b,
               {
               if (member != -1) {
      /* This should have been handled by OpTypeStruct */
   assert(val->type->base_type == vtn_base_type_struct);
   assert(member >= 0 && member < val->type->length);
               switch (dec->decoration) {
   case SpvDecorationArrayStride:
      vtn_assert(type->base_type == vtn_base_type_array ||
            case SpvDecorationBlock:
      vtn_assert(type->base_type == vtn_base_type_struct);
   vtn_assert(type->block);
      case SpvDecorationBufferBlock:
      vtn_assert(type->base_type == vtn_base_type_struct);
   vtn_assert(type->buffer_block);
      case SpvDecorationGLSLShared:
   case SpvDecorationGLSLPacked:
      /* Ignore these, since we get explicit offsets anyways */
         case SpvDecorationRowMajor:
   case SpvDecorationColMajor:
   case SpvDecorationMatrixStride:
   case SpvDecorationBuiltIn:
   case SpvDecorationNoPerspective:
   case SpvDecorationFlat:
   case SpvDecorationPatch:
   case SpvDecorationCentroid:
   case SpvDecorationSample:
   case SpvDecorationExplicitInterpAMD:
   case SpvDecorationVolatile:
   case SpvDecorationCoherent:
   case SpvDecorationNonWritable:
   case SpvDecorationNonReadable:
   case SpvDecorationUniform:
   case SpvDecorationUniformId:
   case SpvDecorationLocation:
   case SpvDecorationComponent:
   case SpvDecorationOffset:
   case SpvDecorationXfbBuffer:
   case SpvDecorationXfbStride:
   case SpvDecorationUserSemantic:
      vtn_warn("Decoration only allowed for struct members: %s",
               case SpvDecorationStream:
      /* We don't need to do anything here, as stream is filled up when
   * aplying the decoration to a variable, just check that if it is not a
   * struct member, it should be a struct.
   */
   vtn_assert(type->base_type == vtn_base_type_struct);
         case SpvDecorationRelaxedPrecision:
   case SpvDecorationSpecId:
   case SpvDecorationInvariant:
   case SpvDecorationRestrict:
   case SpvDecorationAliased:
   case SpvDecorationConstant:
   case SpvDecorationIndex:
   case SpvDecorationBinding:
   case SpvDecorationDescriptorSet:
   case SpvDecorationLinkageAttributes:
   case SpvDecorationNoContraction:
   case SpvDecorationInputAttachmentIndex:
      vtn_warn("Decoration not allowed on types: %s",
               case SpvDecorationCPacked:
      /* Handled when parsing a struct type, nothing to do here. */
         case SpvDecorationSaturatedConversion:
   case SpvDecorationFuncParamAttr:
   case SpvDecorationFPRoundingMode:
   case SpvDecorationFPFastMathMode:
   case SpvDecorationAlignment:
      vtn_warn("Decoration only allowed for CL-style kernels: %s",
               case SpvDecorationUserTypeGOOGLE:
      /* User semantic decorations can safely be ignored by the driver. */
         default:
            }
      static unsigned
   translate_image_format(struct vtn_builder *b, SpvImageFormat format)
   {
      switch (format) {
   case SpvImageFormatUnknown:      return PIPE_FORMAT_NONE;
   case SpvImageFormatRgba32f:      return PIPE_FORMAT_R32G32B32A32_FLOAT;
   case SpvImageFormatRgba16f:      return PIPE_FORMAT_R16G16B16A16_FLOAT;
   case SpvImageFormatR32f:         return PIPE_FORMAT_R32_FLOAT;
   case SpvImageFormatRgba8:        return PIPE_FORMAT_R8G8B8A8_UNORM;
   case SpvImageFormatRgba8Snorm:   return PIPE_FORMAT_R8G8B8A8_SNORM;
   case SpvImageFormatRg32f:        return PIPE_FORMAT_R32G32_FLOAT;
   case SpvImageFormatRg16f:        return PIPE_FORMAT_R16G16_FLOAT;
   case SpvImageFormatR11fG11fB10f: return PIPE_FORMAT_R11G11B10_FLOAT;
   case SpvImageFormatR16f:         return PIPE_FORMAT_R16_FLOAT;
   case SpvImageFormatRgba16:       return PIPE_FORMAT_R16G16B16A16_UNORM;
   case SpvImageFormatRgb10A2:      return PIPE_FORMAT_R10G10B10A2_UNORM;
   case SpvImageFormatRg16:         return PIPE_FORMAT_R16G16_UNORM;
   case SpvImageFormatRg8:          return PIPE_FORMAT_R8G8_UNORM;
   case SpvImageFormatR16:          return PIPE_FORMAT_R16_UNORM;
   case SpvImageFormatR8:           return PIPE_FORMAT_R8_UNORM;
   case SpvImageFormatRgba16Snorm:  return PIPE_FORMAT_R16G16B16A16_SNORM;
   case SpvImageFormatRg16Snorm:    return PIPE_FORMAT_R16G16_SNORM;
   case SpvImageFormatRg8Snorm:     return PIPE_FORMAT_R8G8_SNORM;
   case SpvImageFormatR16Snorm:     return PIPE_FORMAT_R16_SNORM;
   case SpvImageFormatR8Snorm:      return PIPE_FORMAT_R8_SNORM;
   case SpvImageFormatRgba32i:      return PIPE_FORMAT_R32G32B32A32_SINT;
   case SpvImageFormatRgba16i:      return PIPE_FORMAT_R16G16B16A16_SINT;
   case SpvImageFormatRgba8i:       return PIPE_FORMAT_R8G8B8A8_SINT;
   case SpvImageFormatR32i:         return PIPE_FORMAT_R32_SINT;
   case SpvImageFormatRg32i:        return PIPE_FORMAT_R32G32_SINT;
   case SpvImageFormatRg16i:        return PIPE_FORMAT_R16G16_SINT;
   case SpvImageFormatRg8i:         return PIPE_FORMAT_R8G8_SINT;
   case SpvImageFormatR16i:         return PIPE_FORMAT_R16_SINT;
   case SpvImageFormatR8i:          return PIPE_FORMAT_R8_SINT;
   case SpvImageFormatRgba32ui:     return PIPE_FORMAT_R32G32B32A32_UINT;
   case SpvImageFormatRgba16ui:     return PIPE_FORMAT_R16G16B16A16_UINT;
   case SpvImageFormatRgba8ui:      return PIPE_FORMAT_R8G8B8A8_UINT;
   case SpvImageFormatR32ui:        return PIPE_FORMAT_R32_UINT;
   case SpvImageFormatRgb10a2ui:    return PIPE_FORMAT_R10G10B10A2_UINT;
   case SpvImageFormatRg32ui:       return PIPE_FORMAT_R32G32_UINT;
   case SpvImageFormatRg16ui:       return PIPE_FORMAT_R16G16_UINT;
   case SpvImageFormatRg8ui:        return PIPE_FORMAT_R8G8_UINT;
   case SpvImageFormatR16ui:        return PIPE_FORMAT_R16_UINT;
   case SpvImageFormatR8ui:         return PIPE_FORMAT_R8_UINT;
   case SpvImageFormatR64ui:        return PIPE_FORMAT_R64_UINT;
   case SpvImageFormatR64i:         return PIPE_FORMAT_R64_SINT;
   default:
      vtn_fail("Invalid image format: %s (%u)",
         }
      static void
   validate_image_type_for_sampled_image(struct vtn_builder *b,
               {
      /* From OpTypeSampledImage description in SPIR-V 1.6, revision 1:
   *
   *   Image Type must be an OpTypeImage. It is the type of the image in the
   *   combined sampler and image type. It must not have a Dim of
   *   SubpassData. Additionally, starting with version 1.6, it must not have
   *   a Dim of Buffer.
   *
   * Same also applies to the type of the Image operand in OpSampledImage.
                     vtn_fail_if(dim == GLSL_SAMPLER_DIM_SUBPASS ||
                  if (dim == GLSL_SAMPLER_DIM_BUF) {
      if (b->version >= 0x10600) {
      vtn_fail("Starting with SPIR-V 1.6, %s "
      } else {
               }
      static void
   vtn_handle_type(struct vtn_builder *b, SpvOp opcode,
         {
               /* In order to properly handle forward declarations, we have to defer
   * allocation for pointer types.
   */
   if (opcode != SpvOpTypePointer && opcode != SpvOpTypeForwardPointer) {
      val = vtn_push_value(b, w[1], vtn_value_type_type);
   vtn_fail_if(val->type != NULL,
         val->type = rzalloc(b, struct vtn_type);
               switch (opcode) {
   case SpvOpTypeVoid:
      val->type->base_type = vtn_base_type_void;
   val->type->type = glsl_void_type();
      case SpvOpTypeBool:
      val->type->base_type = vtn_base_type_scalar;
   val->type->type = glsl_bool_type();
   val->type->length = 1;
      case SpvOpTypeInt: {
      int bit_size = w[2];
   const bool signedness = w[3];
   vtn_fail_if(bit_size != 8 && bit_size != 16 &&
               val->type->base_type = vtn_base_type_scalar;
   val->type->type = signedness ? glsl_intN_t_type(bit_size) :
         val->type->length = 1;
               case SpvOpTypeFloat: {
      int bit_size = w[2];
   val->type->base_type = vtn_base_type_scalar;
   vtn_fail_if(bit_size != 16 && bit_size != 32 && bit_size != 64,
         val->type->type = glsl_floatN_t_type(bit_size);
   val->type->length = 1;
               case SpvOpTypeVector: {
      struct vtn_type *base = vtn_get_type(b, w[2]);
            vtn_fail_if(base->base_type != vtn_base_type_scalar,
         vtn_fail_if((elems < 2 || elems > 4) && (elems != 8) && (elems != 16),
            val->type->base_type = vtn_base_type_vector;
   val->type->type = glsl_vector_type(glsl_get_base_type(base->type), elems);
   val->type->length = elems;
   val->type->stride = glsl_type_is_boolean(val->type->type)
         val->type->array_element = base;
               case SpvOpTypeMatrix: {
      struct vtn_type *base = vtn_get_type(b, w[2]);
            vtn_fail_if(base->base_type != vtn_base_type_vector,
         vtn_fail_if(columns < 2 || columns > 4,
            val->type->base_type = vtn_base_type_matrix;
   val->type->type = glsl_matrix_type(glsl_get_base_type(base->type),
               vtn_fail_if(glsl_type_is_error(val->type->type),
         assert(!glsl_type_is_error(val->type->type));
   val->type->length = columns;
   val->type->array_element = base;
   val->type->row_major = false;
   val->type->stride = 0;
               case SpvOpTypeRuntimeArray:
   case SpvOpTypeArray: {
               if (opcode == SpvOpTypeRuntimeArray) {
      /* A length of 0 is used to denote unsized arrays */
      } else {
                  val->type->base_type = vtn_base_type_array;
            vtn_foreach_decoration(b, val, array_stride_decoration_cb, NULL);
   val->type->type = glsl_array_type(array_element->type, val->type->length,
                     case SpvOpTypeStruct: {
      unsigned num_fields = count - 2;
   val->type->base_type = vtn_base_type_struct;
   val->type->length = num_fields;
   val->type->members = ralloc_array(b, struct vtn_type *, num_fields);
   val->type->offsets = ralloc_array(b, unsigned, num_fields);
            NIR_VLA(struct glsl_struct_field, fields, count);
   for (unsigned i = 0; i < num_fields; i++) {
      val->type->members[i] = vtn_get_type(b, w[i + 2]);
   const char *name = NULL;
   for (struct vtn_decoration *dec = val->decoration; dec; dec = dec->next) {
      if (dec->scope == VTN_DEC_STRUCT_MEMBER_NAME0 - i) {
      name = dec->member_name;
         }
                  fields[i] = (struct glsl_struct_field) {
      .type = val->type->members[i]->type,
   .name = name,
   .location = -1,
                           struct member_decoration_ctx ctx = {
      .num_fields = num_fields,
   .fields = fields,
                        /* Propagate access specifiers that are present on all members to the overall type */
   enum gl_access_qualifier overall_access = ACCESS_COHERENT | ACCESS_VOLATILE |
         for (unsigned i = 0; i < num_fields; ++i)
                                             if (val->type->block || val->type->buffer_block) {
      /* Packing will be ignored since types coming from SPIR-V are
   * explicitly laid out.
   */
   val->type->type = glsl_interface_type(fields, num_fields,
            } else {
      val->type->type = glsl_struct_type(fields, num_fields,
            }
               case SpvOpTypeFunction: {
      val->type->base_type = vtn_base_type_function;
                     const unsigned num_params = count - 3;
   val->type->length = num_params;
   val->type->params = ralloc_array(b, struct vtn_type *, num_params);
   for (unsigned i = 0; i < count - 3; i++) {
         }
               case SpvOpTypePointer:
   case SpvOpTypeForwardPointer: {
      /* We can't blindly push the value because it might be a forward
   * declaration.
   */
                     vtn_fail_if(opcode == SpvOpTypeForwardPointer &&
               b->shader->info.stage != MESA_SHADER_KERNEL &&
            struct vtn_type *deref_type = NULL;
   if (opcode == SpvOpTypePointer)
            bool has_forward_pointer = false;
   if (val->value_type == vtn_value_type_invalid) {
      val->value_type = vtn_value_type_type;
   val->type = rzalloc(b, struct vtn_type);
   val->type->id = w[1];
                  /* These can actually be stored to nir_variables and used as SSA
   * values so they need a real glsl_type.
   */
                  /* The deref type should only matter for the UniformConstant storage
   * class.  In particular, it should never matter for any storage
   * classes that are allowed in combination with OpTypeForwardPointer.
   */
   if (storage_class != SpvStorageClassUniform &&
      storage_class != SpvStorageClassUniformConstant) {
   assert(mode == vtn_storage_class_to_mode(b, storage_class,
               val->type->type = nir_address_format_to_glsl_type(
      } else {
      vtn_fail_if(val->type->storage_class != storage_class,
               "The storage classes of an OpTypePointer and any "
               if (opcode == SpvOpTypePointer) {
      vtn_fail_if(val->type->deref != NULL,
                        vtn_fail_if(has_forward_pointer &&
                                 /* Only certain storage classes use ArrayStride. */
   switch (storage_class) {
   case SpvStorageClassWorkgroup:
      if (!b->options->caps.workgroup_memory_explicit_layout)
               case SpvStorageClassUniform:
   case SpvStorageClassPushConstant:
   case SpvStorageClassStorageBuffer:
   case SpvStorageClassPhysicalStorageBuffer:
                  default:
      /* Nothing to do. */
         }
               case SpvOpTypeImage: {
               /* Images are represented in NIR as a scalar SSA value that is the
   * result of a deref instruction.  An OpLoad on an OpTypeImage pointer
   * from UniformConstant memory just takes the NIR deref from the pointer
   * and turns it into an SSA value.
   */
   val->type->type = nir_address_format_to_glsl_type(
            const struct vtn_type *sampled_type = vtn_get_type(b, w[2]);
   if (b->shader->info.stage == MESA_SHADER_KERNEL) {
      vtn_fail_if(sampled_type->base_type != vtn_base_type_void,
      } else {
      vtn_fail_if(sampled_type->base_type != vtn_base_type_scalar,
         if (b->options->caps.image_atomic_int64) {
      vtn_fail_if(glsl_get_bit_size(sampled_type->type) != 32 &&
                  } else {
      vtn_fail_if(glsl_get_bit_size(sampled_type->type) != 32,
                  enum glsl_sampler_dim dim;
   switch ((SpvDim)w[3]) {
   case SpvDim1D:       dim = GLSL_SAMPLER_DIM_1D;    break;
   case SpvDim2D:       dim = GLSL_SAMPLER_DIM_2D;    break;
   case SpvDim3D:       dim = GLSL_SAMPLER_DIM_3D;    break;
   case SpvDimCube:     dim = GLSL_SAMPLER_DIM_CUBE;  break;
   case SpvDimRect:     dim = GLSL_SAMPLER_DIM_RECT;  break;
   case SpvDimBuffer:   dim = GLSL_SAMPLER_DIM_BUF;   break;
   case SpvDimSubpassData: dim = GLSL_SAMPLER_DIM_SUBPASS; break;
   default:
      vtn_fail("Invalid SPIR-V image dimensionality: %s (%u)",
               /* w[4]: as per Vulkan spec "Validation Rules within a Module",
   *       The “Depth” operand of OpTypeImage is ignored.
   */
   bool is_array = w[5];
   bool multisampled = w[6];
   unsigned sampled = w[7];
            if (count > 9)
         else if (b->shader->info.stage == MESA_SHADER_KERNEL)
      /* Per the CL C spec: If no qualifier is provided, read_only is assumed. */
      else
            if (multisampled) {
      if (dim == GLSL_SAMPLER_DIM_2D)
         else if (dim == GLSL_SAMPLER_DIM_SUBPASS)
         else
                        enum glsl_base_type sampled_base_type =
         if (sampled == 1) {
      val->type->glsl_image = glsl_texture_type(dim, is_array,
      } else if (sampled == 2) {
      val->type->glsl_image = glsl_image_type(dim, is_array,
      } else if (b->shader->info.stage == MESA_SHADER_KERNEL) {
      val->type->glsl_image = glsl_image_type(dim, is_array,
      } else {
         }
               case SpvOpTypeSampledImage: {
      val->type->base_type = vtn_base_type_sampled_image;
            validate_image_type_for_sampled_image(
                  /* Sampled images are represented NIR as a vec2 SSA value where each
   * component is the result of a deref instruction.  The first component
   * is the image and the second is the sampler.  An OpLoad on an
   * OpTypeSampledImage pointer from UniformConstant memory just takes
   * the NIR deref from the pointer and duplicates it to both vector
   * components.
   */
   nir_address_format addr_format =
         assert(nir_address_format_num_components(addr_format) == 1);
   unsigned bit_size = nir_address_format_bit_size(addr_format);
            enum glsl_base_type base_type =
         val->type->type = glsl_vector_type(base_type, 2);
               case SpvOpTypeSampler:
               /* Samplers are represented in NIR as a scalar SSA value that is the
   * result of a deref instruction.  An OpLoad on an OpTypeSampler pointer
   * from UniformConstant memory just takes the NIR deref from the pointer
   * and turns it into an SSA value.
   */
   val->type->type = nir_address_format_to_glsl_type(
               case SpvOpTypeAccelerationStructureKHR:
      val->type->base_type = vtn_base_type_accel_struct;
   val->type->type = glsl_uint64_t_type();
            case SpvOpTypeOpaque: {
      val->type->base_type = vtn_base_type_struct;
   const char *name = vtn_string_literal(b, &w[2], count - 2, NULL);
   val->type->type = glsl_struct_type(NULL, 0, name, false);
               case SpvOpTypeRayQueryKHR: {
      val->type->base_type = vtn_base_type_ray_query;
   val->type->type = glsl_uint64_t_type();
   /* We may need to run queries on helper invocations. Here the parser
   * doesn't go through a deeper analysis on whether the result of a query
   * will be used in derivative instructions.
   *
   * An implementation willing to optimize this would look through the IR
   * and check if any derivative instruction uses the result of a query
   * and drop this flag if not.
   */
   if (b->shader->info.stage == MESA_SHADER_FRAGMENT)
                     case SpvOpTypeCooperativeMatrixKHR:
      vtn_handle_cooperative_type(b, val, opcode, w, count);
         case SpvOpTypeEvent:
      val->type->base_type = vtn_base_type_event;
   /*
   * this makes the event type compatible with pointer size due to LLVM 16.
   * llvm 17 fixes this properly, but with 16 and opaque ptrs it's still wrong.
   */
   val->type->type = b->shader->info.cs.ptr_size == 64 ? glsl_int64_t_type() : glsl_int_type();
         case SpvOpTypeDeviceEvent:
   case SpvOpTypeReserveId:
   case SpvOpTypeQueue:
   case SpvOpTypePipe:
   default:
                           if (val->type->base_type == vtn_base_type_struct &&
      (val->type->block || val->type->buffer_block)) {
   for (unsigned i = 0; i < val->type->length; i++) {
      vtn_fail_if(vtn_type_contains_block(b, val->type->members[i]),
               "Block and BufferBlock decorations cannot decorate a "
            }
      static nir_constant *
   vtn_null_constant(struct vtn_builder *b, struct vtn_type *type)
   {
               switch (type->base_type) {
   case vtn_base_type_scalar:
   case vtn_base_type_vector:
      c->is_null_constant = true;
   /* Nothing to do here.  It's already initialized to zero */
         case vtn_base_type_pointer: {
      enum vtn_variable_mode mode = vtn_storage_class_to_mode(
                  const nir_const_value *null_value = nir_address_format_null_value(addr_format);
   memcpy(c->values, null_value,
                     case vtn_base_type_void:
   case vtn_base_type_image:
   case vtn_base_type_sampler:
   case vtn_base_type_sampled_image:
   case vtn_base_type_function:
   case vtn_base_type_event:
      /* For those we have to return something but it doesn't matter what. */
         case vtn_base_type_matrix:
   case vtn_base_type_array:
      vtn_assert(type->length > 0);
   c->is_null_constant = true;
   c->num_elements = type->length;
            c->elements[0] = vtn_null_constant(b, type->array_element);
   for (unsigned i = 1; i < c->num_elements; i++)
               case vtn_base_type_struct:
      c->is_null_constant = true;
   c->num_elements = type->length;
   c->elements = ralloc_array(b, nir_constant *, c->num_elements);
   for (unsigned i = 0; i < c->num_elements; i++)
               default:
                     }
      static void
   spec_constant_decoration_cb(struct vtn_builder *b, UNUSED struct vtn_value *val,
               {
      vtn_assert(member == -1);
   if (dec->decoration != SpvDecorationSpecId)
            nir_const_value *value = data;
   for (unsigned i = 0; i < b->num_specializations; i++) {
      if (b->specializations[i].id == dec->operands[0]) {
      *value = b->specializations[i].value;
            }
      static void
   handle_workgroup_size_decoration_cb(struct vtn_builder *b,
                           {
      vtn_assert(member == -1);
   if (dec->decoration != SpvDecorationBuiltIn ||
      dec->operands[0] != SpvBuiltInWorkgroupSize)
         vtn_assert(val->type->type == glsl_vector_type(GLSL_TYPE_UINT, 3));
      }
      static void
   vtn_handle_constant(struct vtn_builder *b, SpvOp opcode,
         {
      struct vtn_value *val = vtn_push_value(b, w[2], vtn_value_type_constant);
   val->constant = rzalloc(b, nir_constant);
   switch (opcode) {
   case SpvOpConstantTrue:
   case SpvOpConstantFalse:
   case SpvOpSpecConstantTrue:
   case SpvOpSpecConstantFalse: {
      vtn_fail_if(val->type->type != glsl_bool_type(),
                  bool bval = (opcode == SpvOpConstantTrue ||
                     if (opcode == SpvOpSpecConstantTrue ||
                  val->constant->values[0].b = u32val.u32 != 0;
               case SpvOpConstant:
   case SpvOpSpecConstant: {
      vtn_fail_if(val->type->base_type != vtn_base_type_scalar,
               int bit_size = glsl_get_bit_size(val->type->type);
   switch (bit_size) {
   case 64:
      val->constant->values[0].u64 = vtn_u64_literal(&w[3]);
      case 32:
      val->constant->values[0].u32 = w[3];
      case 16:
      val->constant->values[0].u16 = w[3];
      case 8:
      val->constant->values[0].u8 = w[3];
      default:
                  if (opcode == SpvOpSpecConstant)
      vtn_foreach_decoration(b, val, spec_constant_decoration_cb,
                  case SpvOpSpecConstantComposite:
   case SpvOpConstantComposite: {
      unsigned elem_count = count - 3;
   unsigned expected_length = val->type->base_type == vtn_base_type_cooperative_matrix ?
         vtn_fail_if(elem_count != expected_length,
                  nir_constant **elems = ralloc_array(b, nir_constant *, elem_count);
   val->is_undef_constant = true;
   for (unsigned i = 0; i < elem_count; i++) {
               if (elem_val->value_type == vtn_value_type_constant) {
      elems[i] = elem_val->constant;
   val->is_undef_constant = val->is_undef_constant &&
      } else {
      vtn_fail_if(elem_val->value_type != vtn_value_type_undef,
               /* to make it easier, just insert a NULL constant for now */
                  switch (val->type->base_type) {
   case vtn_base_type_vector: {
      assert(glsl_type_is_vector(val->type->type));
   for (unsigned i = 0; i < elem_count; i++)
                     case vtn_base_type_matrix:
   case vtn_base_type_struct:
   case vtn_base_type_array:
      ralloc_steal(val->constant, elems);
   val->constant->num_elements = elem_count;
               case vtn_base_type_cooperative_matrix:
                  default:
      vtn_fail("Result type of %s must be a composite type",
      }
               case SpvOpSpecConstantOp: {
      nir_const_value u32op = nir_const_value_for_uint(w[3], 32);
   vtn_foreach_decoration(b, val, spec_constant_decoration_cb, &u32op);
   SpvOp opcode = u32op.u32;
   switch (opcode) {
   case SpvOpVectorShuffle: {
                     vtn_assert(v0->value_type == vtn_value_type_constant ||
                                                unsigned bit_size = glsl_get_bit_size(val->type->type);
                                                if (v0->value_type == vtn_value_type_constant) {
      for (unsigned i = 0; i < len0; i++)
      }
   if (v1->value_type == vtn_value_type_constant) {
      for (unsigned i = 0; i < len1; i++)
               for (unsigned i = 0, j = 0; i < count - 6; i++, j++) {
      uint32_t comp = w[i + 6];
   if (comp == (uint32_t)-1) {
      /* If component is not used, set the value to a known constant
   * to detect if it is wrongly used.
   */
      } else {
      vtn_fail_if(comp >= len0 + len1,
                     }
               case SpvOpCompositeExtract:
   case SpvOpCompositeInsert: {
      struct vtn_value *comp;
   unsigned deref_start;
   struct nir_constant **c;
   if (opcode == SpvOpCompositeExtract) {
      comp = vtn_value(b, w[4], vtn_value_type_constant);
   deref_start = 5;
      } else {
      comp = vtn_value(b, w[5], vtn_value_type_constant);
   deref_start = 6;
   val->constant = nir_constant_clone(comp->constant,
                     int elem = -1;
   const struct vtn_type *type = comp->type;
   for (unsigned i = deref_start; i < count; i++) {
      vtn_fail_if(w[i] > type->length,
                        switch (type->base_type) {
   case vtn_base_type_vector:
                        case vtn_base_type_matrix:
   case vtn_base_type_array:
                        case vtn_base_type_struct:
                        default:
      vtn_fail("%s must only index into composite types",
                  if (opcode == SpvOpCompositeExtract) {
      if (elem == -1) {
         } else {
      unsigned num_components = type->length;
   for (unsigned i = 0; i < num_components; i++)
         } else {
      struct vtn_value *insert =
         vtn_assert(insert->type == type);
   if (elem == -1) {
         } else {
      unsigned num_components = type->length;
   for (unsigned i = 0; i < num_components; i++)
         }
               default: {
      bool swap;
   nir_alu_type dst_alu_type = nir_get_nir_type_for_glsl_type(val->type->type);
   nir_alu_type src_alu_type = dst_alu_type;
                           switch (opcode) {
   case SpvOpSConvert:
   case SpvOpFConvert:
   case SpvOpUConvert:
      /* We have a source in a conversion */
   src_alu_type =
         /* We use the bitsize of the conversion source to evaluate the opcode later */
   bit_size = glsl_get_bit_size(vtn_get_value_type(b, w[4])->type);
      default:
                  bool exact;
   nir_op op = vtn_nir_alu_op_for_spirv_opcode(b, opcode, &swap, &exact,
                  /* No SPIR-V opcodes handled through this path should set exact.
   * Since it is ignored, assert on it.
                           for (unsigned i = 0; i < count - 4; i++) {
                     /* If this is an unsized source, pull the bit size from the
   * source; otherwise, we'll use the bit size from the destination.
   */
                  unsigned src_comps = nir_op_infos[op].input_sizes[i] ?
                  unsigned j = swap ? 1 - i : i;
   for (unsigned c = 0; c < src_comps; c++)
               /* fix up fixed size sources */
   switch (op) {
   case nir_op_ishl:
   case nir_op_ishr:
   case nir_op_ushr: {
      if (bit_size == 32)
         for (unsigned i = 0; i < num_components; ++i) {
      switch (bit_size) {
   case 64: src[1][i].u32 = src[1][i].u64; break;
   case 16: src[1][i].u32 = src[1][i].u16; break;
   case  8: src[1][i].u32 = src[1][i].u8;  break;
      }
      }
   default:
                  nir_const_value *srcs[3] = {
         };
   nir_eval_const_opcode(op, val->constant->values,
                  } /* default */
   }
               case SpvOpConstantNull:
      val->constant = vtn_null_constant(b, val->type);
   val->is_null_constant = true;
         default:
                  /* Now that we have the value, update the workgroup size if needed */
   if (gl_shader_stage_uses_workgroup(b->entry_point_stage))
      vtn_foreach_decoration(b, val, handle_workgroup_size_decoration_cb,
   }
      static void
   vtn_split_barrier_semantics(struct vtn_builder *b,
                     {
      /* For memory semantics embedded in operations, we split them into up to
   * two barriers, to be added before and after the operation.  This is less
   * strict than if we propagated until the final backend stage, but still
   * result in correct execution.
   *
   * A further improvement could be pipe this information (and use!) into the
   * next compiler layers, at the expense of making the handling of barriers
   * more complicated.
            *before = SpvMemorySemanticsMaskNone;
            SpvMemorySemanticsMask order_semantics =
      semantics & (SpvMemorySemanticsAcquireMask |
                     if (util_bitcount(order_semantics) > 1) {
      /* Old GLSLang versions incorrectly set all the ordering bits.  This was
   * fixed in c51287d744fb6e7e9ccc09f6f8451e6c64b1dad6 of glslang repo,
   * and it is in GLSLang since revision "SPIRV99.1321" (from Jul-2016).
   */
   vtn_warn("Multiple memory ordering semantics specified, "
                     const SpvMemorySemanticsMask av_vis_semantics =
      semantics & (SpvMemorySemanticsMakeAvailableMask |
         const SpvMemorySemanticsMask storage_semantics =
      semantics & (SpvMemorySemanticsUniformMemoryMask |
               SpvMemorySemanticsSubgroupMemoryMask |
   SpvMemorySemanticsWorkgroupMemoryMask |
   SpvMemorySemanticsCrossWorkgroupMemoryMask |
         const SpvMemorySemanticsMask other_semantics =
      semantics & ~(order_semantics | av_vis_semantics | storage_semantics |
         if (other_semantics)
                     /* The RELEASE barrier happens BEFORE the operation, and it is usually
   * associated with a Store.  All the write operations with a matching
   * semantics will not be reordered after the Store.
   */
   if (order_semantics & (SpvMemorySemanticsReleaseMask |
                              /* The ACQUIRE barrier happens AFTER the operation, and it is usually
   * associated with a Load.  All the operations with a matching semantics
   * will not be reordered before the Load.
   */
   if (order_semantics & (SpvMemorySemanticsAcquireMask |
                              if (av_vis_semantics & SpvMemorySemanticsMakeVisibleMask)
            if (av_vis_semantics & SpvMemorySemanticsMakeAvailableMask)
      }
      static nir_memory_semantics
   vtn_mem_semantics_to_nir_mem_semantics(struct vtn_builder *b,
         {
               SpvMemorySemanticsMask order_semantics =
      semantics & (SpvMemorySemanticsAcquireMask |
                     if (util_bitcount(order_semantics) > 1) {
      /* Old GLSLang versions incorrectly set all the ordering bits.  This was
   * fixed in c51287d744fb6e7e9ccc09f6f8451e6c64b1dad6 of glslang repo,
   * and it is in GLSLang since revision "SPIRV99.1321" (from Jul-2016).
   */
   vtn_warn("Multiple memory ordering semantics bits specified, "
                     switch (order_semantics) {
   case 0:
      /* Not an ordering barrier. */
         case SpvMemorySemanticsAcquireMask:
      nir_semantics = NIR_MEMORY_ACQUIRE;
         case SpvMemorySemanticsReleaseMask:
      nir_semantics = NIR_MEMORY_RELEASE;
         case SpvMemorySemanticsSequentiallyConsistentMask:
         case SpvMemorySemanticsAcquireReleaseMask:
      nir_semantics = NIR_MEMORY_ACQUIRE | NIR_MEMORY_RELEASE;
         default:
                  if (semantics & SpvMemorySemanticsMakeAvailableMask) {
      vtn_fail_if(!b->options->caps.vk_memory_model,
                           if (semantics & SpvMemorySemanticsMakeVisibleMask) {
      vtn_fail_if(!b->options->caps.vk_memory_model,
                              }
      static nir_variable_mode
   vtn_mem_semantics_to_nir_var_modes(struct vtn_builder *b,
         {
      /* Vulkan Environment for SPIR-V says "SubgroupMemory, CrossWorkgroupMemory,
   * and AtomicCounterMemory are ignored".
   */
   if (b->options->environment == NIR_SPIRV_VULKAN) {
      semantics &= ~(SpvMemorySemanticsSubgroupMemoryMask |
                     nir_variable_mode modes = 0;
   if (semantics & SpvMemorySemanticsUniformMemoryMask)
         if (semantics & SpvMemorySemanticsImageMemoryMask)
         if (semantics & SpvMemorySemanticsWorkgroupMemoryMask)
         if (semantics & SpvMemorySemanticsCrossWorkgroupMemoryMask)
         if (semantics & SpvMemorySemanticsOutputMemoryMask) {
               if (b->shader->info.stage == MESA_SHADER_TASK)
               if (semantics & SpvMemorySemanticsAtomicCounterMemoryMask) {
      /* There's no nir_var_atomic_counter, but since atomic counters are
   * lowered to SSBOs, we use nir_var_mem_ssbo instead.
   */
                  }
      mesa_scope
   vtn_translate_scope(struct vtn_builder *b, SpvScope scope)
   {
      switch (scope) {
   case SpvScopeDevice:
      vtn_fail_if(b->options->caps.vk_memory_model &&
               !b->options->caps.vk_memory_model_device_scope,
   "If the Vulkan memory model is declared and any instruction "
         case SpvScopeQueueFamily:
      vtn_fail_if(!b->options->caps.vk_memory_model,
                     case SpvScopeWorkgroup:
            case SpvScopeSubgroup:
            case SpvScopeInvocation:
            case SpvScopeShaderCallKHR:
            default:
            }
      static void
   vtn_emit_scoped_control_barrier(struct vtn_builder *b, SpvScope exec_scope,
               {
      nir_memory_semantics nir_semantics =
         nir_variable_mode modes = vtn_mem_semantics_to_nir_var_modes(b, semantics);
            /* Memory semantics is optional for OpControlBarrier. */
   mesa_scope nir_mem_scope;
   if (nir_semantics == 0 || modes == 0)
         else
            nir_barrier(&b->nb, .execution_scope=nir_exec_scope, .memory_scope=nir_mem_scope,
      }
      void
   vtn_emit_memory_barrier(struct vtn_builder *b, SpvScope scope,
         {
      nir_variable_mode modes = vtn_mem_semantics_to_nir_var_modes(b, semantics);
   nir_memory_semantics nir_semantics =
            /* No barrier to add. */
   if (nir_semantics == 0 || modes == 0)
            nir_barrier(&b->nb, .memory_scope=vtn_translate_scope(b, scope),
            }
      struct vtn_ssa_value *
   vtn_create_ssa_value(struct vtn_builder *b, const struct glsl_type *type)
   {
      /* Always use bare types for SSA values for a couple of reasons:
   *
   *  1. Code which emits deref chains should never listen to the explicit
   *     layout information on the SSA value if any exists.  If we've
   *     accidentally been relying on this, we want to find those bugs.
   *
   *  2. We want to be able to quickly check that an SSA value being assigned
   *     to a SPIR-V value has the right type.  Using bare types everywhere
   *     ensures that we can pointer-compare.
   */
   struct vtn_ssa_value *val = rzalloc(b, struct vtn_ssa_value);
               if (!glsl_type_is_vector_or_scalar(type)) {
      unsigned elems = glsl_get_length(val->type);
   val->elems = ralloc_array(b, struct vtn_ssa_value *, elems);
   if (glsl_type_is_array_or_matrix(type) || glsl_type_is_cmat(type)) {
      const struct glsl_type *elem_type = glsl_get_array_element(type);
   for (unsigned i = 0; i < elems; i++)
      } else {
      vtn_assert(glsl_type_is_struct_or_ifc(type));
   for (unsigned i = 0; i < elems; i++) {
      const struct glsl_type *elem_type = glsl_get_struct_field(type, i);
                        }
      void
   vtn_set_ssa_value_var(struct vtn_builder *b, struct vtn_ssa_value *ssa, nir_variable *var)
   {
      vtn_assert(glsl_type_is_cmat(var->type));
   vtn_assert(var->type == ssa->type);
   ssa->is_variable = true;
      }
      static nir_tex_src
   vtn_tex_src(struct vtn_builder *b, unsigned index, nir_tex_src_type type)
   {
         }
      static uint32_t
   image_operand_arg(struct vtn_builder *b, const uint32_t *w, uint32_t count,
         {
      static const SpvImageOperandsMask ops_with_arg =
      SpvImageOperandsBiasMask |
   SpvImageOperandsLodMask |
   SpvImageOperandsGradMask |
   SpvImageOperandsConstOffsetMask |
   SpvImageOperandsOffsetMask |
   SpvImageOperandsConstOffsetsMask |
   SpvImageOperandsSampleMask |
   SpvImageOperandsMinLodMask |
   SpvImageOperandsMakeTexelAvailableMask |
         assert(util_bitcount(op) == 1);
   assert(w[mask_idx] & op);
                     /* Adjust indices for operands with two arguments. */
   static const SpvImageOperandsMask ops_with_two_args =
                           vtn_fail_if(idx + (op & ops_with_two_args ? 1 : 0) >= count,
                     }
      static void
   non_uniform_decoration_cb(struct vtn_builder *b,
               {
      enum gl_access_qualifier *access = void_ctx;
   switch (dec->decoration) {
   case SpvDecorationNonUniformEXT:
      *access |= ACCESS_NON_UNIFORM;
         default:
            }
      /* Apply SignExtend/ZeroExtend operands to get the actual result type for
   * image read/sample operations and source type for write operations.
   */
   static nir_alu_type
   get_image_type(struct vtn_builder *b, nir_alu_type type, unsigned operands)
   {
      unsigned extend_operands =
         vtn_fail_if(nir_alu_type_get_base_type(type) == nir_type_float && extend_operands,
         vtn_fail_if(extend_operands ==
                  if (operands & SpvImageOperandsSignExtendMask)
         if (operands & SpvImageOperandsZeroExtendMask)
               }
      static void
   vtn_handle_texture(struct vtn_builder *b, SpvOp opcode,
         {
      if (opcode == SpvOpSampledImage) {
      struct vtn_sampled_image si = {
      .image = vtn_get_image(b, w[3], NULL),
               validate_image_type_for_sampled_image(
                  enum gl_access_qualifier access = 0;
   vtn_foreach_decoration(b, vtn_untyped_value(b, w[3]),
         vtn_foreach_decoration(b, vtn_untyped_value(b, w[4]),
            vtn_push_sampled_image(b, w[2], si, access & ACCESS_NON_UNIFORM);
      } else if (opcode == SpvOpImage) {
               enum gl_access_qualifier access = 0;
   vtn_foreach_decoration(b, vtn_untyped_value(b, w[3]),
            vtn_push_image(b, w[2], si.image, access & ACCESS_NON_UNIFORM);
      } else if (opcode == SpvOpImageSparseTexelsResident) {
      nir_def *code = vtn_get_nir_ssa(b, w[3]);
   vtn_push_nir_ssa(b, w[2], nir_is_sparse_texels_resident(&b->nb, 1, code));
               nir_deref_instr *image = NULL, *sampler = NULL;
   struct vtn_value *sampled_val = vtn_untyped_value(b, w[3]);
   if (sampled_val->type->base_type == vtn_base_type_sampled_image) {
      struct vtn_sampled_image si = vtn_get_sampled_image(b, w[3]);
   image = si.image;
      } else {
                  const enum glsl_sampler_dim sampler_dim = glsl_get_sampler_dim(image->type);
   const bool is_array = glsl_sampler_type_is_array(image->type);
            /* Figure out the base texture operation */
   nir_texop texop;
   switch (opcode) {
   case SpvOpImageSampleImplicitLod:
   case SpvOpImageSparseSampleImplicitLod:
   case SpvOpImageSampleDrefImplicitLod:
   case SpvOpImageSparseSampleDrefImplicitLod:
      vtn_assert(sampler_dim != GLSL_SAMPLER_DIM_BUF &&
               texop = nir_texop_tex;
         case SpvOpImageSampleProjImplicitLod:
   case SpvOpImageSampleProjDrefImplicitLod:
      vtn_assert(sampler_dim == GLSL_SAMPLER_DIM_1D ||
            sampler_dim == GLSL_SAMPLER_DIM_2D ||
      vtn_assert(!is_array);
   texop = nir_texop_tex;
         case SpvOpImageSampleExplicitLod:
   case SpvOpImageSparseSampleExplicitLod:
   case SpvOpImageSampleDrefExplicitLod:
   case SpvOpImageSparseSampleDrefExplicitLod:
      vtn_assert(sampler_dim != GLSL_SAMPLER_DIM_BUF &&
               texop = nir_texop_txl;
         case SpvOpImageSampleProjExplicitLod:
   case SpvOpImageSampleProjDrefExplicitLod:
      vtn_assert(sampler_dim == GLSL_SAMPLER_DIM_1D ||
            sampler_dim == GLSL_SAMPLER_DIM_2D ||
      vtn_assert(!is_array);
   texop = nir_texop_txl;
         case SpvOpImageFetch:
   case SpvOpImageSparseFetch:
      vtn_assert(sampler_dim != GLSL_SAMPLER_DIM_CUBE);
   if (sampler_dim == GLSL_SAMPLER_DIM_MS) {
         } else {
         }
         case SpvOpImageGather:
   case SpvOpImageSparseGather:
   case SpvOpImageDrefGather:
   case SpvOpImageSparseDrefGather:
      vtn_assert(sampler_dim == GLSL_SAMPLER_DIM_2D ||
               texop = nir_texop_tg4;
         case SpvOpImageQuerySizeLod:
      vtn_assert(sampler_dim == GLSL_SAMPLER_DIM_1D ||
            sampler_dim == GLSL_SAMPLER_DIM_2D ||
      texop = nir_texop_txs;
   dest_type = nir_type_int32;
         case SpvOpImageQuerySize:
      vtn_assert(sampler_dim == GLSL_SAMPLER_DIM_1D ||
            sampler_dim == GLSL_SAMPLER_DIM_2D ||
   sampler_dim == GLSL_SAMPLER_DIM_3D ||
   sampler_dim == GLSL_SAMPLER_DIM_CUBE ||
   sampler_dim == GLSL_SAMPLER_DIM_RECT ||
      texop = nir_texop_txs;
   dest_type = nir_type_int32;
         case SpvOpImageQueryLod:
      vtn_assert(sampler_dim == GLSL_SAMPLER_DIM_1D ||
            sampler_dim == GLSL_SAMPLER_DIM_2D ||
      texop = nir_texop_lod;
   dest_type = nir_type_float32;
         case SpvOpImageQueryLevels:
      /* This operation is not valid for a MS image but present in some old
   * shaders.  Just return 1 in those cases.
   */
   if (sampler_dim == GLSL_SAMPLER_DIM_MS) {
      vtn_warn("OpImageQueryLevels 'Sampled Image' should have an MS of 0, "
         vtn_push_nir_ssa(b, w[2], nir_imm_int(&b->nb, 1));
      }
   vtn_assert(sampler_dim == GLSL_SAMPLER_DIM_1D ||
            sampler_dim == GLSL_SAMPLER_DIM_2D ||
      texop = nir_texop_query_levels;
   dest_type = nir_type_int32;
         case SpvOpImageQuerySamples:
      vtn_assert(sampler_dim == GLSL_SAMPLER_DIM_MS);
   texop = nir_texop_texture_samples;
   dest_type = nir_type_int32;
         case SpvOpFragmentFetchAMD:
      vtn_assert(sampler_dim == GLSL_SAMPLER_DIM_MS ||
         texop = nir_texop_fragment_fetch_amd;
         case SpvOpFragmentMaskFetchAMD:
      vtn_assert(sampler_dim == GLSL_SAMPLER_DIM_MS ||
         texop = nir_texop_fragment_mask_fetch_amd;
   dest_type = nir_type_uint32;
         default:
                  nir_tex_src srcs[10]; /* 10 should be enough */
            p->src = nir_src_for_ssa(&image->def);
   p->src_type = nir_tex_src_texture_deref;
            switch (texop) {
   case nir_texop_tex:
   case nir_texop_txb:
   case nir_texop_txl:
   case nir_texop_txd:
   case nir_texop_tg4:
   case nir_texop_lod:
      vtn_fail_if(sampler == NULL,
               p->src = nir_src_for_ssa(&sampler->def);
   p->src_type = nir_tex_src_sampler_deref;
   p++;
      case nir_texop_txf:
   case nir_texop_txf_ms:
   case nir_texop_txs:
   case nir_texop_query_levels:
   case nir_texop_texture_samples:
   case nir_texop_samples_identical:
   case nir_texop_fragment_fetch_amd:
   case nir_texop_fragment_mask_fetch_amd:
      /* These don't */
      case nir_texop_txf_ms_fb:
      vtn_fail("unexpected nir_texop_txf_ms_fb");
      case nir_texop_txf_ms_mcs_intel:
      vtn_fail("unexpected nir_texop_txf_ms_mcs");
      case nir_texop_tex_prefetch:
      vtn_fail("unexpected nir_texop_tex_prefetch");
      case nir_texop_descriptor_amd:
   case nir_texop_sampler_descriptor_amd:
      vtn_fail("unexpected nir_texop_*descriptor_amd");
      case nir_texop_lod_bias_agx:
      vtn_fail("unexpected nir_texop_lod_bias_agx");
      case nir_texop_hdr_dim_nv:
   case nir_texop_tex_type_nv:
      vtn_fail("unexpected nir_texop_*_nv");
                        struct nir_def *coord;
   unsigned coord_components;
   switch (opcode) {
   case SpvOpImageSampleImplicitLod:
   case SpvOpImageSparseSampleImplicitLod:
   case SpvOpImageSampleExplicitLod:
   case SpvOpImageSparseSampleExplicitLod:
   case SpvOpImageSampleDrefImplicitLod:
   case SpvOpImageSparseSampleDrefImplicitLod:
   case SpvOpImageSampleDrefExplicitLod:
   case SpvOpImageSparseSampleDrefExplicitLod:
   case SpvOpImageSampleProjImplicitLod:
   case SpvOpImageSampleProjExplicitLod:
   case SpvOpImageSampleProjDrefImplicitLod:
   case SpvOpImageSampleProjDrefExplicitLod:
   case SpvOpImageFetch:
   case SpvOpImageSparseFetch:
   case SpvOpImageGather:
   case SpvOpImageSparseGather:
   case SpvOpImageDrefGather:
   case SpvOpImageSparseDrefGather:
   case SpvOpImageQueryLod:
   case SpvOpFragmentFetchAMD:
   case SpvOpFragmentMaskFetchAMD: {
      /* All these types have the coordinate as their first real argument */
            if (is_array && texop != nir_texop_lod)
            struct vtn_ssa_value *coord_val = vtn_ssa_value(b, w[idx++]);
   coord = coord_val->def;
   /* From the SPIR-V spec verxion 1.5, rev. 5:
   *
   *    "Coordinate must be a scalar or vector of floating-point type. It
   *    contains (u[, v] ... [, array layer]) as needed by the definition
   *    of Sampled Image. It may be a vector larger than needed, but all
   *    unused components appear after all used components."
   */
   vtn_fail_if(coord->num_components < coord_components,
                  /* OpenCL allows integer sampling coordinates */
   if (glsl_type_is_integer(coord_val->type) &&
      opcode == SpvOpImageSampleExplicitLod) {
   vtn_fail_if(b->shader->info.stage != MESA_SHADER_KERNEL,
                  nir_def *coords[4];
   nir_def *f0_5 = nir_imm_float(&b->nb, 0.5);
                     if (!is_array || i != coord_components - 1)
                           p->src_type = nir_tex_src_coord;
   p++;
               default:
      coord = NULL;
   coord_components = 0;
               switch (opcode) {
   case SpvOpImageSampleProjImplicitLod:
   case SpvOpImageSampleProjExplicitLod:
   case SpvOpImageSampleProjDrefImplicitLod:
   case SpvOpImageSampleProjDrefExplicitLod:
      /* These have the projector as the last coordinate component */
   p->src = nir_src_for_ssa(nir_channel(&b->nb, coord, coord_components));
   p->src_type = nir_tex_src_projector;
   p++;
         default:
                  bool is_shadow = false;
   unsigned gather_component = 0;
   switch (opcode) {
   case SpvOpImageSampleDrefImplicitLod:
   case SpvOpImageSparseSampleDrefImplicitLod:
   case SpvOpImageSampleDrefExplicitLod:
   case SpvOpImageSparseSampleDrefExplicitLod:
   case SpvOpImageSampleProjDrefImplicitLod:
   case SpvOpImageSampleProjDrefExplicitLod:
   case SpvOpImageDrefGather:
   case SpvOpImageSparseDrefGather:
      /* These all have an explicit depth value as their next source */
   is_shadow = true;
   (*p++) = vtn_tex_src(b, w[idx++], nir_tex_src_comparator);
         case SpvOpImageGather:
   case SpvOpImageSparseGather:
      /* This has a component as its next source */
   gather_component = vtn_constant_uint(b, w[idx++]);
         default:
                  bool is_sparse = false;
   switch (opcode) {
   case SpvOpImageSparseSampleImplicitLod:
   case SpvOpImageSparseSampleExplicitLod:
   case SpvOpImageSparseSampleDrefImplicitLod:
   case SpvOpImageSparseSampleDrefExplicitLod:
   case SpvOpImageSparseFetch:
   case SpvOpImageSparseGather:
   case SpvOpImageSparseDrefGather:
      is_sparse = true;
      default:
                  /* For OpImageQuerySizeLod, we always have an LOD */
   if (opcode == SpvOpImageQuerySizeLod)
            /* For OpFragmentFetchAMD, we always have a multisample index */
   if (opcode == SpvOpFragmentFetchAMD)
            /* Now we need to handle some number of optional arguments */
   struct vtn_value *gather_offsets = NULL;
   uint32_t operands = SpvImageOperandsMaskNone;
   if (idx < count) {
               if (operands & SpvImageOperandsBiasMask) {
      vtn_assert(texop == nir_texop_tex ||
         if (texop == nir_texop_tex)
         uint32_t arg = image_operand_arg(b, w, count, idx,
                     if (operands & SpvImageOperandsLodMask) {
      vtn_assert(texop == nir_texop_txl || texop == nir_texop_txf ||
         uint32_t arg = image_operand_arg(b, w, count, idx,
                     if (operands & SpvImageOperandsGradMask) {
      vtn_assert(texop == nir_texop_txl);
   texop = nir_texop_txd;
   uint32_t arg = image_operand_arg(b, w, count, idx,
         (*p++) = vtn_tex_src(b, w[arg], nir_tex_src_ddx);
               vtn_fail_if(util_bitcount(operands & (SpvImageOperandsConstOffsetsMask |
                              if (operands & SpvImageOperandsOffsetMask) {
      uint32_t arg = image_operand_arg(b, w, count, idx,
                     if (operands & SpvImageOperandsConstOffsetMask) {
      uint32_t arg = image_operand_arg(b, w, count, idx,
                     if (operands & SpvImageOperandsConstOffsetsMask) {
      vtn_assert(texop == nir_texop_tg4);
   uint32_t arg = image_operand_arg(b, w, count, idx,
                     if (operands & SpvImageOperandsSampleMask) {
      vtn_assert(texop == nir_texop_txf_ms);
   uint32_t arg = image_operand_arg(b, w, count, idx,
         texop = nir_texop_txf_ms;
               if (operands & SpvImageOperandsMinLodMask) {
      vtn_assert(texop == nir_texop_tex ||
               uint32_t arg = image_operand_arg(b, w, count, idx,
                        struct vtn_type *ret_type = vtn_get_type(b, w[1]);
   struct vtn_type *struct_type = NULL;
   if (is_sparse) {
      vtn_assert(glsl_type_is_struct_or_ifc(ret_type->type));
   struct_type = ret_type;
               nir_tex_instr *instr = nir_tex_instr_create(b->shader, p - srcs);
                     instr->coord_components = coord_components;
   instr->sampler_dim = sampler_dim;
   instr->is_array = is_array;
   instr->is_shadow = is_shadow;
   instr->is_sparse = is_sparse;
   instr->is_new_style_shadow =
                  /* If SpvCapabilityImageGatherBiasLodAMD is enabled, texture gather without an explicit LOD
   * has an implicit one (instead of using level 0).
   */
   if (texop == nir_texop_tg4 && b->image_gather_bias_lod &&
      !(operands & SpvImageOperandsLodMask)) {
               /* The Vulkan spec says:
   *
   *    "If an instruction loads from or stores to a resource (including
   *    atomics and image instructions) and the resource descriptor being
   *    accessed is not dynamically uniform, then the operand corresponding
   *    to that resource (e.g. the pointer or sampled image operand) must be
   *    decorated with NonUniform."
   *
   * It's very careful to specify that the exact operand must be decorated
   * NonUniform.  The SPIR-V parser is not expected to chase through long
   * chains to find the NonUniform decoration.  It's either right there or we
   * can assume it doesn't exist.
   */
   enum gl_access_qualifier access = 0;
            if (operands & SpvImageOperandsNontemporalMask)
            if (sampler && b->options->force_tex_non_uniform)
            if (sampled_val->propagated_non_uniform)
            if (image && (access & ACCESS_NON_UNIFORM))
            if (sampler && (access & ACCESS_NON_UNIFORM))
            /* for non-query ops, get dest_type from SPIR-V return type */
   if (dest_type == nir_type_invalid) {
      /* the return type should match the image type, unless the image type is
   * VOID (CL image), in which case the return type dictates the sampler
   */
   enum glsl_base_type sampler_base =
         enum glsl_base_type ret_base = glsl_get_base_type(ret_type->type);
   vtn_fail_if(sampler_base != ret_base && sampler_base != GLSL_TYPE_VOID,
               dest_type = nir_get_nir_type_for_glsl_base_type(ret_base);
                        nir_def_init(&instr->instr, &instr->def,
            vtn_assert(glsl_get_vector_elements(ret_type->type) ==
            if (gather_offsets) {
      vtn_fail_if(gather_offsets->type->base_type != vtn_base_type_array ||
                        struct vtn_type *vec_type = gather_offsets->type->array_element;
   vtn_fail_if(vec_type->base_type != vtn_base_type_vector ||
               vec_type->length != 2 ||
            unsigned bit_size = glsl_get_bit_size(vec_type->type);
   for (uint32_t i = 0; i < 4; i++) {
      const nir_const_value *cvec =
         for (uint32_t j = 0; j < 2; j++) {
      switch (bit_size) {
   case 8:  instr->tg4_offsets[i][j] = cvec[j].i8;    break;
   case 16: instr->tg4_offsets[i][j] = cvec[j].i16;   break;
   case 32: instr->tg4_offsets[i][j] = cvec[j].i32;   break;
   case 64: instr->tg4_offsets[i][j] = cvec[j].i64;   break;
   default:
                                    if (is_sparse) {
      struct vtn_ssa_value *dest = vtn_create_ssa_value(b, struct_type->type);
   unsigned result_size = glsl_get_vector_elements(ret_type->type);
   dest->elems[0]->def = nir_channel(&b->nb, &instr->def, result_size);
   dest->elems[1]->def = nir_trim_vector(&b->nb, &instr->def,
            } else {
            }
      static nir_atomic_op
   translate_atomic_op(SpvOp opcode)
   {
      switch (opcode) {
   case SpvOpAtomicExchange:            return nir_atomic_op_xchg;
   case SpvOpAtomicCompareExchange:     return nir_atomic_op_cmpxchg;
   case SpvOpAtomicCompareExchangeWeak: return nir_atomic_op_cmpxchg;
   case SpvOpAtomicIIncrement:          return nir_atomic_op_iadd;
   case SpvOpAtomicIDecrement:          return nir_atomic_op_iadd;
   case SpvOpAtomicIAdd:                return nir_atomic_op_iadd;
   case SpvOpAtomicISub:                return nir_atomic_op_iadd;
   case SpvOpAtomicSMin:                return nir_atomic_op_imin;
   case SpvOpAtomicUMin:                return nir_atomic_op_umin;
   case SpvOpAtomicSMax:                return nir_atomic_op_imax;
   case SpvOpAtomicUMax:                return nir_atomic_op_umax;
   case SpvOpAtomicAnd:                 return nir_atomic_op_iand;
   case SpvOpAtomicOr:                  return nir_atomic_op_ior;
   case SpvOpAtomicXor:                 return nir_atomic_op_ixor;
   case SpvOpAtomicFAddEXT:             return nir_atomic_op_fadd;
   case SpvOpAtomicFMinEXT:             return nir_atomic_op_fmin;
   case SpvOpAtomicFMaxEXT:             return nir_atomic_op_fmax;
   case SpvOpAtomicFlagTestAndSet:      return nir_atomic_op_cmpxchg;
   default:
            }
      static void
   fill_common_atomic_sources(struct vtn_builder *b, SpvOp opcode,
         {
      const struct glsl_type *type = vtn_get_type(b, w[1])->type;
            switch (opcode) {
   case SpvOpAtomicIIncrement:
      src[0] = nir_src_for_ssa(nir_imm_intN_t(&b->nb, 1, bit_size));
         case SpvOpAtomicIDecrement:
      src[0] = nir_src_for_ssa(nir_imm_intN_t(&b->nb, -1, bit_size));
         case SpvOpAtomicISub:
      src[0] =
               case SpvOpAtomicCompareExchange:
   case SpvOpAtomicCompareExchangeWeak:
      src[0] = nir_src_for_ssa(vtn_get_nir_ssa(b, w[8]));
   src[1] = nir_src_for_ssa(vtn_get_nir_ssa(b, w[7]));
         case SpvOpAtomicExchange:
   case SpvOpAtomicIAdd:
   case SpvOpAtomicSMin:
   case SpvOpAtomicUMin:
   case SpvOpAtomicSMax:
   case SpvOpAtomicUMax:
   case SpvOpAtomicAnd:
   case SpvOpAtomicOr:
   case SpvOpAtomicXor:
   case SpvOpAtomicFAddEXT:
   case SpvOpAtomicFMinEXT:
   case SpvOpAtomicFMaxEXT:
      src[0] = nir_src_for_ssa(vtn_get_nir_ssa(b, w[6]));
         default:
            }
      static nir_def *
   get_image_coord(struct vtn_builder *b, uint32_t value)
   {
      nir_def *coord = vtn_get_nir_ssa(b, value);
   /* The image_load_store intrinsics assume a 4-dim coordinate */
      }
      static void
   vtn_handle_image(struct vtn_builder *b, SpvOp opcode,
         {
      /* Just get this one out of the way */
   if (opcode == SpvOpImageTexelPointer) {
      struct vtn_value *val =
                  val->image->image = vtn_nir_deref(b, w[3]);
   val->image->coord = get_image_coord(b, w[4]);
   val->image->sample = vtn_get_nir_ssa(b, w[5]);
   val->image->lod = nir_imm_int(&b->nb, 0);
               struct vtn_image_pointer image;
   SpvScope scope = SpvScopeInvocation;
   SpvMemorySemanticsMask semantics = 0;
                     struct vtn_value *res_val;
   switch (opcode) {
   case SpvOpAtomicExchange:
   case SpvOpAtomicCompareExchange:
   case SpvOpAtomicCompareExchangeWeak:
   case SpvOpAtomicIIncrement:
   case SpvOpAtomicIDecrement:
   case SpvOpAtomicIAdd:
   case SpvOpAtomicISub:
   case SpvOpAtomicLoad:
   case SpvOpAtomicSMin:
   case SpvOpAtomicUMin:
   case SpvOpAtomicSMax:
   case SpvOpAtomicUMax:
   case SpvOpAtomicAnd:
   case SpvOpAtomicOr:
   case SpvOpAtomicXor:
   case SpvOpAtomicFAddEXT:
   case SpvOpAtomicFMinEXT:
   case SpvOpAtomicFMaxEXT:
      res_val = vtn_value(b, w[3], vtn_value_type_image_pointer);
   image = *res_val->image;
   scope = vtn_constant_uint(b, w[4]);
   semantics = vtn_constant_uint(b, w[5]);
   access |= ACCESS_COHERENT;
         case SpvOpAtomicStore:
      res_val = vtn_value(b, w[1], vtn_value_type_image_pointer);
   image = *res_val->image;
   scope = vtn_constant_uint(b, w[2]);
   semantics = vtn_constant_uint(b, w[3]);
   access |= ACCESS_COHERENT;
         case SpvOpImageQuerySizeLod:
      res_val = vtn_untyped_value(b, w[3]);
   image.image = vtn_get_image(b, w[3], &access);
   image.coord = NULL;
   image.sample = NULL;
   image.lod = vtn_ssa_value(b, w[4])->def;
         case SpvOpImageQuerySize:
   case SpvOpImageQuerySamples:
      res_val = vtn_untyped_value(b, w[3]);
   image.image = vtn_get_image(b, w[3], &access);
   image.coord = NULL;
   image.sample = NULL;
   image.lod = NULL;
         case SpvOpImageQueryFormat:
   case SpvOpImageQueryOrder:
      res_val = vtn_untyped_value(b, w[3]);
   image.image = vtn_get_image(b, w[3], &access);
   image.coord = NULL;
   image.sample = NULL;
   image.lod = NULL;
         case SpvOpImageRead:
   case SpvOpImageSparseRead: {
      res_val = vtn_untyped_value(b, w[3]);
   image.image = vtn_get_image(b, w[3], &access);
                     if (operands & SpvImageOperandsSampleMask) {
      uint32_t arg = image_operand_arg(b, w, count, 5,
            } else {
                  if (operands & SpvImageOperandsMakeTexelVisibleMask) {
      vtn_fail_if((operands & SpvImageOperandsNonPrivateTexelMask) == 0,
         uint32_t arg = image_operand_arg(b, w, count, 5,
         semantics = SpvMemorySemanticsMakeVisibleMask;
               if (operands & SpvImageOperandsLodMask) {
      uint32_t arg = image_operand_arg(b, w, count, 5,
            } else {
                  if (operands & SpvImageOperandsVolatileTexelMask)
         if (operands & SpvImageOperandsNontemporalMask)
                        case SpvOpImageWrite: {
      res_val = vtn_untyped_value(b, w[1]);
   image.image = vtn_get_image(b, w[1], &access);
                              if (operands & SpvImageOperandsSampleMask) {
      uint32_t arg = image_operand_arg(b, w, count, 4,
            } else {
                  if (operands & SpvImageOperandsMakeTexelAvailableMask) {
      vtn_fail_if((operands & SpvImageOperandsNonPrivateTexelMask) == 0,
         uint32_t arg = image_operand_arg(b, w, count, 4,
         semantics = SpvMemorySemanticsMakeAvailableMask;
               if (operands & SpvImageOperandsLodMask) {
      uint32_t arg = image_operand_arg(b, w, count, 4,
            } else {
                  if (operands & SpvImageOperandsVolatileTexelMask)
         if (operands & SpvImageOperandsNontemporalMask)
                        default:
                  if (semantics & SpvMemorySemanticsVolatileMask)
            nir_intrinsic_op op;
      #define OP(S, N) case SpvOp##S: op = nir_intrinsic_image_deref_##N; break;
      OP(ImageQuerySize,            size)
   OP(ImageQuerySizeLod,         size)
   OP(ImageRead,                 load)
   OP(ImageSparseRead,           sparse_load)
   OP(ImageWrite,                store)
   OP(AtomicLoad,                load)
   OP(AtomicStore,               store)
   OP(AtomicExchange,            atomic)
   OP(AtomicCompareExchange,     atomic_swap)
   OP(AtomicCompareExchangeWeak, atomic_swap)
   OP(AtomicIIncrement,          atomic)
   OP(AtomicIDecrement,          atomic)
   OP(AtomicIAdd,                atomic)
   OP(AtomicISub,                atomic)
   OP(AtomicSMin,                atomic)
   OP(AtomicUMin,                atomic)
   OP(AtomicSMax,                atomic)
   OP(AtomicUMax,                atomic)
   OP(AtomicAnd,                 atomic)
   OP(AtomicOr,                  atomic)
   OP(AtomicXor,                 atomic)
   OP(AtomicFAddEXT,             atomic)
   OP(AtomicFMinEXT,             atomic)
   OP(AtomicFMaxEXT,             atomic)
   OP(ImageQueryFormat,          format)
   OP(ImageQueryOrder,           order)
      #undef OP
      default:
                  nir_intrinsic_instr *intrin = nir_intrinsic_instr_create(b->shader, op);
   if (nir_intrinsic_has_atomic_op(intrin))
            intrin->src[0] = nir_src_for_ssa(&image.image->def);
   nir_intrinsic_set_image_dim(intrin, glsl_get_sampler_dim(image.image->type));
   nir_intrinsic_set_image_array(intrin,
            switch (opcode) {
   case SpvOpImageQuerySamples:
   case SpvOpImageQuerySize:
   case SpvOpImageQuerySizeLod:
   case SpvOpImageQueryFormat:
   case SpvOpImageQueryOrder:
         default:
      /* The image coordinate is always 4 components but we may not have that
   * many.  Swizzle to compensate.
   */
   intrin->src[1] = nir_src_for_ssa(nir_pad_vec4(&b->nb, image.coord));
   intrin->src[2] = nir_src_for_ssa(image.sample);
               /* The Vulkan spec says:
   *
   *    "If an instruction loads from or stores to a resource (including
   *    atomics and image instructions) and the resource descriptor being
   *    accessed is not dynamically uniform, then the operand corresponding
   *    to that resource (e.g. the pointer or sampled image operand) must be
   *    decorated with NonUniform."
   *
   * It's very careful to specify that the exact operand must be decorated
   * NonUniform.  The SPIR-V parser is not expected to chase through long
   * chains to find the NonUniform decoration.  It's either right there or we
   * can assume it doesn't exist.
   */
   vtn_foreach_decoration(b, res_val, non_uniform_decoration_cb, &access);
            switch (opcode) {
   case SpvOpImageQuerySamples:
   case SpvOpImageQueryFormat:
   case SpvOpImageQueryOrder:
      /* No additional sources */
      case SpvOpImageQuerySize:
      intrin->src[1] = nir_src_for_ssa(nir_imm_int(&b->nb, 0));
      case SpvOpImageQuerySizeLod:
      intrin->src[1] = nir_src_for_ssa(image.lod);
      case SpvOpAtomicLoad:
   case SpvOpImageRead:
   case SpvOpImageSparseRead:
      /* Only OpImageRead can support a lod parameter if
   * SPV_AMD_shader_image_load_store_lod is used but the current NIR
   * intrinsics definition for atomics requires us to set it for
   * OpAtomicLoad.
   */
   intrin->src[3] = nir_src_for_ssa(image.lod);
      case SpvOpAtomicStore:
   case SpvOpImageWrite: {
      const uint32_t value_id = opcode == SpvOpAtomicStore ? w[4] : w[3];
   struct vtn_ssa_value *value = vtn_ssa_value(b, value_id);
   /* nir_intrinsic_image_deref_store always takes a vec4 value */
   assert(op == nir_intrinsic_image_deref_store);
   intrin->num_components = 4;
   intrin->src[3] = nir_src_for_ssa(nir_pad_vec4(&b->nb, value->def));
   /* Only OpImageWrite can support a lod parameter if
   * SPV_AMD_shader_image_load_store_lod is used but the current NIR
   * intrinsics definition for atomics requires us to set it for
   * OpAtomicStore.
   */
            nir_alu_type src_type =
         nir_intrinsic_set_src_type(intrin, src_type);
               case SpvOpAtomicCompareExchange:
   case SpvOpAtomicCompareExchangeWeak:
   case SpvOpAtomicIIncrement:
   case SpvOpAtomicIDecrement:
   case SpvOpAtomicExchange:
   case SpvOpAtomicIAdd:
   case SpvOpAtomicISub:
   case SpvOpAtomicSMin:
   case SpvOpAtomicUMin:
   case SpvOpAtomicSMax:
   case SpvOpAtomicUMax:
   case SpvOpAtomicAnd:
   case SpvOpAtomicOr:
   case SpvOpAtomicXor:
   case SpvOpAtomicFAddEXT:
   case SpvOpAtomicFMinEXT:
   case SpvOpAtomicFMaxEXT:
      fill_common_atomic_sources(b, opcode, w, &intrin->src[3]);
         default:
                  /* Image operations implicitly have the Image storage memory semantics. */
            SpvMemorySemanticsMask before_semantics;
   SpvMemorySemanticsMask after_semantics;
            if (before_semantics)
            if (opcode != SpvOpImageWrite && opcode != SpvOpAtomicStore) {
      struct vtn_type *type = vtn_get_type(b, w[1]);
   struct vtn_type *struct_type = NULL;
   if (opcode == SpvOpImageSparseRead) {
      vtn_assert(glsl_type_is_struct_or_ifc(type->type));
   struct_type = type;
               unsigned dest_components = glsl_get_vector_elements(type->type);
   if (opcode == SpvOpImageSparseRead)
            if (nir_intrinsic_infos[op].dest_components == 0)
            unsigned bit_size = glsl_get_bit_size(type->type);
   if (opcode == SpvOpImageQuerySize ||
                  nir_def_init(&intrin->instr, &intrin->def,
                     nir_def *result = nir_trim_vector(&b->nb, &intrin->def,
            if (opcode == SpvOpImageQuerySize ||
                  if (opcode == SpvOpImageSparseRead) {
      struct vtn_ssa_value *dest = vtn_create_ssa_value(b, struct_type->type);
   unsigned res_type_size = glsl_get_vector_elements(type->type);
   dest->elems[0]->def = nir_channel(&b->nb, result, res_type_size);
   if (intrin->def.bit_size != 32)
         dest->elems[1]->def = nir_trim_vector(&b->nb, result, res_type_size);
      } else {
                  if (opcode == SpvOpImageRead || opcode == SpvOpImageSparseRead ||
      opcode == SpvOpAtomicLoad) {
   nir_alu_type dest_type =
               } else {
                  if (after_semantics)
      }
      static nir_intrinsic_op
   get_uniform_nir_atomic_op(struct vtn_builder *b, SpvOp opcode)
   {
         #define OP(S, N) case SpvOp##S: return nir_intrinsic_atomic_counter_ ##N;
      OP(AtomicLoad,                read_deref)
   OP(AtomicExchange,            exchange)
   OP(AtomicCompareExchange,     comp_swap)
   OP(AtomicCompareExchangeWeak, comp_swap)
   OP(AtomicIIncrement,          inc_deref)
   OP(AtomicIDecrement,          post_dec_deref)
   OP(AtomicIAdd,                add_deref)
   OP(AtomicISub,                add_deref)
   OP(AtomicUMin,                min_deref)
   OP(AtomicUMax,                max_deref)
   OP(AtomicAnd,                 and_deref)
   OP(AtomicOr,                  or_deref)
      #undef OP
      default:
      /* We left the following out: AtomicStore, AtomicSMin and
   * AtomicSmax. Right now there are not nir intrinsics for them. At this
   * moment Atomic Counter support is needed for ARB_spirv support, so is
   * only need to support GLSL Atomic Counters that are uints and don't
   * allow direct storage.
   */
         }
      static nir_intrinsic_op
   get_deref_nir_atomic_op(struct vtn_builder *b, SpvOp opcode)
   {
      switch (opcode) {
   case SpvOpAtomicLoad:         return nir_intrinsic_load_deref;
   case SpvOpAtomicFlagClear:
      #define OP(S, N) case SpvOp##S: return nir_intrinsic_deref_##N;
      OP(AtomicExchange,            atomic)
   OP(AtomicCompareExchange,     atomic_swap)
   OP(AtomicCompareExchangeWeak, atomic_swap)
   OP(AtomicIIncrement,          atomic)
   OP(AtomicIDecrement,          atomic)
   OP(AtomicIAdd,                atomic)
   OP(AtomicISub,                atomic)
   OP(AtomicSMin,                atomic)
   OP(AtomicUMin,                atomic)
   OP(AtomicSMax,                atomic)
   OP(AtomicUMax,                atomic)
   OP(AtomicAnd,                 atomic)
   OP(AtomicOr,                  atomic)
   OP(AtomicXor,                 atomic)
   OP(AtomicFAddEXT,             atomic)
   OP(AtomicFMinEXT,             atomic)
   OP(AtomicFMaxEXT,             atomic)
      #undef OP
      default:
            }
      /*
   * Handles shared atomics, ssbo atomics and atomic counters.
   */
   static void
   vtn_handle_atomics(struct vtn_builder *b, SpvOp opcode,
         {
      struct vtn_pointer *ptr;
            SpvScope scope = SpvScopeInvocation;
   SpvMemorySemanticsMask semantics = 0;
            switch (opcode) {
   case SpvOpAtomicLoad:
   case SpvOpAtomicExchange:
   case SpvOpAtomicCompareExchange:
   case SpvOpAtomicCompareExchangeWeak:
   case SpvOpAtomicIIncrement:
   case SpvOpAtomicIDecrement:
   case SpvOpAtomicIAdd:
   case SpvOpAtomicISub:
   case SpvOpAtomicSMin:
   case SpvOpAtomicUMin:
   case SpvOpAtomicSMax:
   case SpvOpAtomicUMax:
   case SpvOpAtomicAnd:
   case SpvOpAtomicOr:
   case SpvOpAtomicXor:
   case SpvOpAtomicFAddEXT:
   case SpvOpAtomicFMinEXT:
   case SpvOpAtomicFMaxEXT:
   case SpvOpAtomicFlagTestAndSet:
      ptr = vtn_pointer(b, w[3]);
   scope = vtn_constant_uint(b, w[4]);
   semantics = vtn_constant_uint(b, w[5]);
      case SpvOpAtomicFlagClear:
   case SpvOpAtomicStore:
      ptr = vtn_pointer(b, w[1]);
   scope = vtn_constant_uint(b, w[2]);
   semantics = vtn_constant_uint(b, w[3]);
         default:
                  if (semantics & SpvMemorySemanticsVolatileMask)
            /* uniform as "atomic counter uniform" */
   if (ptr->mode == vtn_variable_mode_atomic_counter) {
      nir_deref_instr *deref = vtn_pointer_to_deref(b, ptr);
   nir_intrinsic_op op = get_uniform_nir_atomic_op(b, opcode);
   atomic = nir_intrinsic_instr_create(b->nb.shader, op);
            /* SSBO needs to initialize index/offset. In this case we don't need to,
   * as that info is already stored on the ptr->var->var nir_variable (see
   * vtn_create_variable)
            switch (opcode) {
   case SpvOpAtomicLoad:
   case SpvOpAtomicExchange:
   case SpvOpAtomicCompareExchange:
   case SpvOpAtomicCompareExchangeWeak:
   case SpvOpAtomicIIncrement:
   case SpvOpAtomicIDecrement:
   case SpvOpAtomicIAdd:
   case SpvOpAtomicISub:
   case SpvOpAtomicSMin:
   case SpvOpAtomicUMin:
   case SpvOpAtomicSMax:
   case SpvOpAtomicUMax:
   case SpvOpAtomicAnd:
   case SpvOpAtomicOr:
   case SpvOpAtomicXor:
      /* Nothing: we don't need to call fill_common_atomic_sources here, as
   * atomic counter uniforms doesn't have sources
               default:
               } else {
      nir_deref_instr *deref = vtn_pointer_to_deref(b, ptr);
   const struct glsl_type *deref_type = deref->type;
   nir_intrinsic_op op = get_deref_nir_atomic_op(b, opcode);
   atomic = nir_intrinsic_instr_create(b->nb.shader, op);
            if (nir_intrinsic_has_atomic_op(atomic))
            if (ptr->mode != vtn_variable_mode_workgroup)
                     switch (opcode) {
   case SpvOpAtomicLoad:
                  case SpvOpAtomicStore:
      atomic->num_components = glsl_get_vector_elements(deref_type);
   nir_intrinsic_set_write_mask(atomic, (1 << atomic->num_components) - 1);
               case SpvOpAtomicFlagClear:
      atomic->num_components = 1;
   nir_intrinsic_set_write_mask(atomic, 1);
   atomic->src[1] = nir_src_for_ssa(nir_imm_intN_t(&b->nb, 0, 32));
      case SpvOpAtomicFlagTestAndSet:
      atomic->src[1] = nir_src_for_ssa(nir_imm_intN_t(&b->nb, 0, 32));
   atomic->src[2] = nir_src_for_ssa(nir_imm_intN_t(&b->nb, -1, 32));
      case SpvOpAtomicExchange:
   case SpvOpAtomicCompareExchange:
   case SpvOpAtomicCompareExchangeWeak:
   case SpvOpAtomicIIncrement:
   case SpvOpAtomicIDecrement:
   case SpvOpAtomicIAdd:
   case SpvOpAtomicISub:
   case SpvOpAtomicSMin:
   case SpvOpAtomicUMin:
   case SpvOpAtomicSMax:
   case SpvOpAtomicUMax:
   case SpvOpAtomicAnd:
   case SpvOpAtomicOr:
   case SpvOpAtomicXor:
   case SpvOpAtomicFAddEXT:
   case SpvOpAtomicFMinEXT:
   case SpvOpAtomicFMaxEXT:
                  default:
                     /* Atomic ordering operations will implicitly apply to the atomic operation
   * storage class, so include that too.
   */
            SpvMemorySemanticsMask before_semantics;
   SpvMemorySemanticsMask after_semantics;
            if (before_semantics)
            if (opcode != SpvOpAtomicStore && opcode != SpvOpAtomicFlagClear) {
               if (opcode == SpvOpAtomicFlagTestAndSet) {
      /* map atomic flag to a 32-bit atomic integer. */
      } else {
      nir_def_init(&atomic->instr, &atomic->def,
                                          if (opcode == SpvOpAtomicFlagTestAndSet) {
         }
   if (after_semantics)
      }
      static nir_alu_instr *
   create_vec(struct vtn_builder *b, unsigned num_components, unsigned bit_size)
   {
      nir_op op = nir_op_vec(num_components);
   nir_alu_instr *vec = nir_alu_instr_create(b->shader, op);
               }
      struct vtn_ssa_value *
   vtn_ssa_transpose(struct vtn_builder *b, struct vtn_ssa_value *src)
   {
      if (src->transposed)
            struct vtn_ssa_value *dest =
            for (unsigned i = 0; i < glsl_get_matrix_columns(dest->type); i++) {
      if (glsl_type_is_vector_or_scalar(src->type)) {
         } else {
      unsigned cols = glsl_get_matrix_columns(src->type);
   nir_scalar srcs[NIR_MAX_MATRIX_COLUMNS];
   for (unsigned j = 0; j < cols; j++) {
         }
                              }
      static nir_def *
   vtn_vector_shuffle(struct vtn_builder *b, unsigned num_components,
               {
               for (unsigned i = 0; i < num_components; i++) {
      uint32_t index = indices[i];
   unsigned total_components = src0->num_components + src1->num_components;
   vtn_fail_if(index != 0xffffffff && index >= total_components,
                  if (index == 0xffffffff) {
      vec->src[i].src =
      } else if (index < src0->num_components) {
      vec->src[i].src = nir_src_for_ssa(src0);
      } else {
      vec->src[i].src = nir_src_for_ssa(src1);
                              }
      /*
   * Concatentates a number of vectors/scalars together to produce a vector
   */
   static nir_def *
   vtn_vector_construct(struct vtn_builder *b, unsigned num_components,
         {
               /* From the SPIR-V 1.1 spec for OpCompositeConstruct:
   *
   *    "When constructing a vector, there must be at least two Constituent
   *    operands."
   */
            unsigned dest_idx = 0;
   for (unsigned i = 0; i < num_srcs; i++) {
      nir_def *src = srcs[i];
   vtn_assert(dest_idx + src->num_components <= num_components);
   for (unsigned j = 0; j < src->num_components; j++) {
      vec->src[dest_idx].src = nir_src_for_ssa(src);
   vec->src[dest_idx].swizzle[0] = j;
                  /* From the SPIR-V 1.1 spec for OpCompositeConstruct:
   *
   *    "When constructing a vector, the total number of components in all
   *    the operands must equal the number of components in Result Type."
   */
                        }
      static struct vtn_ssa_value *
   vtn_composite_copy(void *mem_ctx, struct vtn_ssa_value *src)
   {
               struct vtn_ssa_value *dest = rzalloc(mem_ctx, struct vtn_ssa_value);
            if (glsl_type_is_vector_or_scalar(src->type)) {
         } else {
               dest->elems = ralloc_array(mem_ctx, struct vtn_ssa_value *, elems);
   for (unsigned i = 0; i < elems; i++)
                  }
      static struct vtn_ssa_value *
   vtn_composite_insert(struct vtn_builder *b, struct vtn_ssa_value *src,
               {
      if (glsl_type_is_cmat(src->type))
                     struct vtn_ssa_value *cur = dest;
   unsigned i;
   for (i = 0; i < num_indices - 1; i++) {
      /* If we got a vector here, that means the next index will be trying to
   * dereference a scalar.
   */
   vtn_fail_if(glsl_type_is_vector_or_scalar(cur->type),
         vtn_fail_if(indices[i] >= glsl_get_length(cur->type),
                     if (glsl_type_is_vector_or_scalar(cur->type)) {
      vtn_fail_if(indices[i] >= glsl_get_vector_elements(cur->type),
            /* According to the SPIR-V spec, OpCompositeInsert may work down to
   * the component granularity. In that case, the last index will be
   * the index to insert the scalar into the vector.
               } else {
      vtn_fail_if(indices[i] >= glsl_get_length(cur->type),
                        }
      static struct vtn_ssa_value *
   vtn_composite_extract(struct vtn_builder *b, struct vtn_ssa_value *src,
         {
      if (glsl_type_is_cmat(src->type))
            struct vtn_ssa_value *cur = src;
   for (unsigned i = 0; i < num_indices; i++) {
      if (glsl_type_is_vector_or_scalar(cur->type)) {
      vtn_assert(i == num_indices - 1);
                  /* According to the SPIR-V spec, OpCompositeExtract may work down to
   * the component granularity. The last index will be the index of the
                  const struct glsl_type *scalar_type =
         struct vtn_ssa_value *ret = vtn_create_ssa_value(b, scalar_type);
   ret->def = nir_channel(&b->nb, cur->def, indices[i]);
      } else {
      vtn_fail_if(indices[i] >= glsl_get_length(cur->type),
                           }
      static void
   vtn_handle_composite(struct vtn_builder *b, SpvOp opcode,
         {
      struct vtn_type *type = vtn_get_type(b, w[1]);
            switch (opcode) {
   case SpvOpVectorExtractDynamic:
      ssa->def = nir_vector_extract(&b->nb, vtn_get_nir_ssa(b, w[3]),
               case SpvOpVectorInsertDynamic:
      ssa->def = nir_vector_insert(&b->nb, vtn_get_nir_ssa(b, w[3]),
                     case SpvOpVectorShuffle:
      ssa->def = vtn_vector_shuffle(b, glsl_get_vector_elements(type->type),
                           case SpvOpCompositeConstruct: {
      unsigned elems = count - 3;
   assume(elems >= 1);
   if (type->base_type == vtn_base_type_cooperative_matrix) {
      vtn_assert(elems == 1);
   nir_deref_instr *mat = vtn_create_cmat_temporary(b, type->type, "cmat_construct");
   nir_cmat_construct(&b->nb, &mat->def, vtn_get_nir_ssa(b, w[3]));
      } else if (glsl_type_is_vector_or_scalar(type->type)) {
      nir_def *srcs[NIR_MAX_VEC_COMPONENTS];
   for (unsigned i = 0; i < elems; i++) {
      srcs[i] = vtn_get_nir_ssa(b, w[3 + i]);
      }
   ssa->def =
      vtn_vector_construct(b, glsl_get_vector_elements(type->type),
   } else {
      ssa->elems = ralloc_array(b, struct vtn_ssa_value *, elems);
   for (unsigned i = 0; i < elems; i++)
      }
      }
   case SpvOpCompositeExtract:
      ssa = vtn_composite_extract(b, vtn_ssa_value(b, w[3]),
               case SpvOpCompositeInsert:
      ssa = vtn_composite_insert(b, vtn_ssa_value(b, w[4]),
                     case SpvOpCopyLogical: {
      ssa = vtn_composite_copy(b, vtn_ssa_value(b, w[3]));
   struct vtn_type *dst_type = vtn_get_value_type(b, w[2]);
   vtn_assert(vtn_types_compatible(b, type, dst_type));
   ssa->type = glsl_get_bare_type(dst_type->type);
      }
   case SpvOpCopyObject:
      vtn_copy_value(b, w[3], w[2]);
         default:
                     }
      static void
   vtn_handle_barrier(struct vtn_builder *b, SpvOp opcode,
         {
      switch (opcode) {
   case SpvOpEmitVertex:
   case SpvOpEmitStreamVertex:
   case SpvOpEndPrimitive:
   case SpvOpEndStreamPrimitive: {
      unsigned stream = 0;
   if (opcode == SpvOpEmitStreamVertex || opcode == SpvOpEndStreamPrimitive)
            switch (opcode) {
   case SpvOpEmitStreamVertex:
   case SpvOpEmitVertex:
      nir_emit_vertex(&b->nb, stream);
      case SpvOpEndPrimitive:
   case SpvOpEndStreamPrimitive:
      nir_end_primitive(&b->nb, stream);
      default:
         }
               case SpvOpMemoryBarrier: {
      SpvScope scope = vtn_constant_uint(b, w[1]);
   SpvMemorySemanticsMask semantics = vtn_constant_uint(b, w[2]);
   vtn_emit_memory_barrier(b, scope, semantics);
               case SpvOpControlBarrier: {
      SpvScope execution_scope = vtn_constant_uint(b, w[1]);
   SpvScope memory_scope = vtn_constant_uint(b, w[2]);
            /* GLSLang, prior to commit 8297936dd6eb3, emitted OpControlBarrier with
   * memory semantics of None for GLSL barrier().
   * And before that, prior to c3f1cdfa, emitted the OpControlBarrier with
   * Device instead of Workgroup for execution scope.
   */
   if (b->wa_glslang_cs_barrier &&
      b->nb.shader->info.stage == MESA_SHADER_COMPUTE &&
   (execution_scope == SpvScopeWorkgroup ||
   execution_scope == SpvScopeDevice) &&
   memory_semantics == SpvMemorySemanticsMaskNone) {
   execution_scope = SpvScopeWorkgroup;
   memory_scope = SpvScopeWorkgroup;
   memory_semantics = SpvMemorySemanticsAcquireReleaseMask |
               /* From the SPIR-V spec:
   *
   *    "When used with the TessellationControl execution model, it also
   *    implicitly synchronizes the Output Storage Class: Writes to Output
   *    variables performed by any invocation executed prior to a
   *    OpControlBarrier will be visible to any other invocation after
   *    return from that OpControlBarrier."
   *
   * The same applies to VK_NV_mesh_shader.
   */
   if (b->nb.shader->info.stage == MESA_SHADER_TESS_CTRL ||
      b->nb.shader->info.stage == MESA_SHADER_TASK ||
   b->nb.shader->info.stage == MESA_SHADER_MESH) {
   memory_semantics &= ~(SpvMemorySemanticsAcquireMask |
                     memory_semantics |= SpvMemorySemanticsAcquireReleaseMask |
               vtn_emit_scoped_control_barrier(b, execution_scope, memory_scope,
                     default:
            }
      static enum tess_primitive_mode
   tess_primitive_mode_from_spv_execution_mode(struct vtn_builder *b,
         {
      switch (mode) {
   case SpvExecutionModeTriangles:
         case SpvExecutionModeQuads:
         case SpvExecutionModeIsolines:
         default:
      vtn_fail("Invalid tess primitive type: %s (%u)",
         }
      static enum mesa_prim
   primitive_from_spv_execution_mode(struct vtn_builder *b,
         {
      switch (mode) {
   case SpvExecutionModeInputPoints:
   case SpvExecutionModeOutputPoints:
         case SpvExecutionModeInputLines:
   case SpvExecutionModeOutputLinesNV:
         case SpvExecutionModeInputLinesAdjacency:
         case SpvExecutionModeTriangles:
   case SpvExecutionModeOutputTrianglesNV:
         case SpvExecutionModeInputTrianglesAdjacency:
         case SpvExecutionModeQuads:
         case SpvExecutionModeOutputLineStrip:
         case SpvExecutionModeOutputTriangleStrip:
         default:
      vtn_fail("Invalid primitive type: %s (%u)",
         }
      static unsigned
   vertices_in_from_spv_execution_mode(struct vtn_builder *b,
         {
      switch (mode) {
   case SpvExecutionModeInputPoints:
         case SpvExecutionModeInputLines:
         case SpvExecutionModeInputLinesAdjacency:
         case SpvExecutionModeTriangles:
         case SpvExecutionModeInputTrianglesAdjacency:
         default:
      vtn_fail("Invalid GS input mode: %s (%u)",
         }
      gl_shader_stage
   vtn_stage_for_execution_model(SpvExecutionModel model)
   {
      switch (model) {
   case SpvExecutionModelVertex:
         case SpvExecutionModelTessellationControl:
         case SpvExecutionModelTessellationEvaluation:
         case SpvExecutionModelGeometry:
         case SpvExecutionModelFragment:
         case SpvExecutionModelGLCompute:
         case SpvExecutionModelKernel:
         case SpvExecutionModelRayGenerationKHR:
         case SpvExecutionModelAnyHitKHR:
         case SpvExecutionModelClosestHitKHR:
         case SpvExecutionModelMissKHR:
         case SpvExecutionModelIntersectionKHR:
         case SpvExecutionModelCallableKHR:
         case SpvExecutionModelTaskNV:
   case SpvExecutionModelTaskEXT:
         case SpvExecutionModelMeshNV:
   case SpvExecutionModelMeshEXT:
         default:
            }
      #define spv_check_supported(name, cap) do {                 \
         if (!(b->options && b->options->caps.name))           \
      vtn_warn("Unsupported SPIR-V capability: %s (%u)", \
            void
   vtn_handle_entry_point(struct vtn_builder *b, const uint32_t *w,
         {
      struct vtn_value *entry_point = &b->values[w[2]];
   /* Let this be a name label regardless */
   unsigned name_words;
            gl_shader_stage stage = vtn_stage_for_execution_model(w[1]);  
   vtn_fail_if(stage == MESA_SHADER_NONE,
               if (strcmp(entry_point->name, b->entry_point_name) != 0 ||
      stage != b->entry_point_stage)
         vtn_assert(b->entry_point == NULL);
            /* Entry points enumerate which global variables are used. */
   size_t start = 3 + name_words;
   b->interface_ids_count = count - start;
   b->interface_ids = ralloc_array(b, uint32_t, b->interface_ids_count);
   memcpy(b->interface_ids, &w[start], b->interface_ids_count * 4);
      }
      static bool
   vtn_handle_preamble_instruction(struct vtn_builder *b, SpvOp opcode,
         {
      switch (opcode) {
   case SpvOpString:
   case SpvOpSource:
   case SpvOpSourceExtension:
   case SpvOpSourceContinued:
   case SpvOpModuleProcessed:
      vtn_handle_debug_text(b, opcode, w, count);
         case SpvOpExtension: {
      /* Implementing both NV_mesh_shader and EXT_mesh_shader
   * is difficult without knowing which we're dealing with.
   * TODO: remove this when we stop supporting NV_mesh_shader.
   */
   const char *ext_name = (const char *)&w[1];
   if (strcmp(ext_name, "SPV_NV_mesh_shader") == 0)
                     case SpvOpCapability: {
      SpvCapability cap = w[1];
   switch (cap) {
   case SpvCapabilityMatrix:
   case SpvCapabilityShader:
   case SpvCapabilityGeometry:
   case SpvCapabilityGeometryPointSize:
   case SpvCapabilityUniformBufferArrayDynamicIndexing:
   case SpvCapabilitySampledImageArrayDynamicIndexing:
   case SpvCapabilityStorageBufferArrayDynamicIndexing:
   case SpvCapabilityStorageImageArrayDynamicIndexing:
   case SpvCapabilityImageRect:
   case SpvCapabilitySampledRect:
   case SpvCapabilitySampled1D:
   case SpvCapabilityImage1D:
   case SpvCapabilitySampledCubeArray:
   case SpvCapabilityImageCubeArray:
   case SpvCapabilitySampledBuffer:
   case SpvCapabilityImageBuffer:
   case SpvCapabilityImageQuery:
   case SpvCapabilityDerivativeControl:
   case SpvCapabilityInterpolationFunction:
   case SpvCapabilityMultiViewport:
   case SpvCapabilitySampleRateShading:
   case SpvCapabilityClipDistance:
   case SpvCapabilityCullDistance:
   case SpvCapabilityInputAttachment:
   case SpvCapabilityImageGatherExtended:
   case SpvCapabilityStorageImageExtendedFormats:
   case SpvCapabilityVector16:
   case SpvCapabilityDotProduct:
   case SpvCapabilityDotProductInputAll:
   case SpvCapabilityDotProductInput4x8Bit:
   case SpvCapabilityDotProductInput4x8BitPacked:
   case SpvCapabilityExpectAssumeKHR:
            case SpvCapabilityLinkage:
      if (!b->options->create_library)
      vtn_warn("Unsupported SPIR-V capability: %s",
                  case SpvCapabilitySparseResidency:
                  case SpvCapabilityMinLod:
                  case SpvCapabilityAtomicStorage:
                  case SpvCapabilityFloat64:
      spv_check_supported(float64, cap);
      case SpvCapabilityInt64:
      spv_check_supported(int64, cap);
      case SpvCapabilityInt16:
      spv_check_supported(int16, cap);
      case SpvCapabilityInt8:
                  case SpvCapabilityTransformFeedback:
                  case SpvCapabilityGeometryStreams:
                  case SpvCapabilityInt64Atomics:
                  case SpvCapabilityStorageImageMultisample:
                  case SpvCapabilityAddresses:
                  case SpvCapabilityKernel:
   case SpvCapabilityFloat16Buffer:
                  case SpvCapabilityGenericPointer:
                  case SpvCapabilityImageBasic:
                  case SpvCapabilityImageReadWrite:
                  case SpvCapabilityLiteralSampler:
                  case SpvCapabilityImageMipmap:
   case SpvCapabilityPipes:
   case SpvCapabilityDeviceEnqueue:
      vtn_warn("Unsupported OpenCL-style SPIR-V capability: %s",
               case SpvCapabilityImageMSArray:
                  case SpvCapabilityTessellation:
   case SpvCapabilityTessellationPointSize:
                  case SpvCapabilityDrawParameters:
                  case SpvCapabilityStorageImageReadWithoutFormat:
                  case SpvCapabilityStorageImageWriteWithoutFormat:
                  case SpvCapabilityDeviceGroup:
                  case SpvCapabilityMultiView:
                  case SpvCapabilityGroupNonUniform:
                  case SpvCapabilitySubgroupVoteKHR:
   case SpvCapabilityGroupNonUniformVote:
                  case SpvCapabilitySubgroupBallotKHR:
   case SpvCapabilityGroupNonUniformBallot:
                  case SpvCapabilityGroupNonUniformShuffle:
   case SpvCapabilityGroupNonUniformShuffleRelative:
                  case SpvCapabilityGroupNonUniformQuad:
                  case SpvCapabilityGroupNonUniformArithmetic:
   case SpvCapabilityGroupNonUniformClustered:
                  case SpvCapabilityGroups:
                  case SpvCapabilitySubgroupDispatch:
      spv_check_supported(subgroup_dispatch, cap);
   /* Missing :
   *   - SpvOpGetKernelLocalSizeForSubgroupCount
   *   - SpvOpGetKernelMaxNumSubgroups
   */
   vtn_warn("Not fully supported capability: %s",
               case SpvCapabilityVariablePointersStorageBuffer:
   case SpvCapabilityVariablePointers:
      spv_check_supported(variable_pointers, cap);
               case SpvCapabilityStorageUniformBufferBlock16:
   case SpvCapabilityStorageUniform16:
   case SpvCapabilityStoragePushConstant16:
   case SpvCapabilityStorageInputOutput16:
                  case SpvCapabilityShaderLayer:
   case SpvCapabilityShaderViewportIndex:
   case SpvCapabilityShaderViewportIndexLayerEXT:
                  case SpvCapabilityStorageBuffer8BitAccess:
   case SpvCapabilityUniformAndStorageBuffer8BitAccess:
   case SpvCapabilityStoragePushConstant8:
                  case SpvCapabilityShaderNonUniformEXT:
                  case SpvCapabilityInputAttachmentArrayDynamicIndexingEXT:
   case SpvCapabilityUniformTexelBufferArrayDynamicIndexingEXT:
   case SpvCapabilityStorageTexelBufferArrayDynamicIndexingEXT:
                  case SpvCapabilityUniformBufferArrayNonUniformIndexingEXT:
   case SpvCapabilitySampledImageArrayNonUniformIndexingEXT:
   case SpvCapabilityStorageBufferArrayNonUniformIndexingEXT:
   case SpvCapabilityStorageImageArrayNonUniformIndexingEXT:
   case SpvCapabilityInputAttachmentArrayNonUniformIndexingEXT:
   case SpvCapabilityUniformTexelBufferArrayNonUniformIndexingEXT:
   case SpvCapabilityStorageTexelBufferArrayNonUniformIndexingEXT:
                  case SpvCapabilityRuntimeDescriptorArrayEXT:
                  case SpvCapabilityStencilExportEXT:
                  case SpvCapabilitySampleMaskPostDepthCoverage:
                  case SpvCapabilityDenormFlushToZero:
   case SpvCapabilityDenormPreserve:
   case SpvCapabilitySignedZeroInfNanPreserve:
   case SpvCapabilityRoundingModeRTE:
   case SpvCapabilityRoundingModeRTZ:
                  case SpvCapabilityPhysicalStorageBufferAddresses:
                  case SpvCapabilityComputeDerivativeGroupQuadsNV:
   case SpvCapabilityComputeDerivativeGroupLinearNV:
                  case SpvCapabilityFloat16:
                  case SpvCapabilityFragmentShaderSampleInterlockEXT:
                  case SpvCapabilityFragmentShaderPixelInterlockEXT:
                  case SpvCapabilityDemoteToHelperInvocation:
      spv_check_supported(demote_to_helper_invocation, cap);
               case SpvCapabilityShaderClockKHR:
   break;
            case SpvCapabilityVulkanMemoryModel:
                  case SpvCapabilityVulkanMemoryModelDeviceScope:
                  case SpvCapabilityImageReadWriteLodAMD:
                  case SpvCapabilityIntegerFunctions2INTEL:
                  case SpvCapabilityFragmentMaskAMD:
                  case SpvCapabilityImageGatherBiasLodAMD:
      spv_check_supported(amd_image_gather_bias_lod, cap);
               case SpvCapabilityAtomicFloat16AddEXT:
                  case SpvCapabilityAtomicFloat32AddEXT:
                  case SpvCapabilityAtomicFloat64AddEXT:
                  case SpvCapabilitySubgroupShuffleINTEL:
                  case SpvCapabilitySubgroupBufferBlockIOINTEL:
                  case SpvCapabilityRayCullMaskKHR:
                  case SpvCapabilityRayTracingKHR:
                  case SpvCapabilityRayQueryKHR:
                  case SpvCapabilityRayTraversalPrimitiveCullingKHR:
                  case SpvCapabilityInt64ImageEXT:
                  case SpvCapabilityFragmentShadingRateKHR:
                  case SpvCapabilityWorkgroupMemoryExplicitLayoutKHR:
                  case SpvCapabilityWorkgroupMemoryExplicitLayout8BitAccessKHR:
      spv_check_supported(workgroup_memory_explicit_layout, cap);
               case SpvCapabilityWorkgroupMemoryExplicitLayout16BitAccessKHR:
      spv_check_supported(workgroup_memory_explicit_layout, cap);
               case SpvCapabilityAtomicFloat16MinMaxEXT:
                  case SpvCapabilityAtomicFloat32MinMaxEXT:
                  case SpvCapabilityAtomicFloat64MinMaxEXT:
                  case SpvCapabilityMeshShadingEXT:
                  case SpvCapabilityMeshShadingNV:
                  case SpvCapabilityPerViewAttributesNV:
                  case SpvCapabilityShaderViewportMaskNV:
                  case SpvCapabilityGroupNonUniformRotateKHR:
                  case SpvCapabilityFragmentFullyCoveredEXT:
                  case SpvCapabilityFragmentDensityEXT:
                  case SpvCapabilityRayTracingPositionFetchKHR:
   case SpvCapabilityRayQueryPositionFetchKHR:
                  case SpvCapabilityFragmentBarycentricKHR:
      spv_check_supported(fragment_barycentric, cap);
         case SpvCapabilityShaderEnqueueAMDX:
                  case SpvCapabilityCooperativeMatrixKHR:
                  default:
      vtn_fail("Unhandled capability: %s (%u)",
      }
               case SpvOpExtInstImport:
      vtn_handle_extension(b, opcode, w, count);
         case SpvOpMemoryModel:
      switch (w[1]) {
   case SpvAddressingModelPhysical32:
      vtn_fail_if(b->shader->info.stage != MESA_SHADER_KERNEL,
         b->shader->info.cs.ptr_size = 32;
   b->physical_ptrs = true;
   assert(nir_address_format_bit_size(b->options->global_addr_format) == 32);
   assert(nir_address_format_num_components(b->options->global_addr_format) == 1);
   assert(nir_address_format_bit_size(b->options->shared_addr_format) == 32);
   assert(nir_address_format_num_components(b->options->shared_addr_format) == 1);
   assert(nir_address_format_bit_size(b->options->constant_addr_format) == 32);
   assert(nir_address_format_num_components(b->options->constant_addr_format) == 1);
      case SpvAddressingModelPhysical64:
      vtn_fail_if(b->shader->info.stage != MESA_SHADER_KERNEL,
         b->shader->info.cs.ptr_size = 64;
   b->physical_ptrs = true;
   assert(nir_address_format_bit_size(b->options->global_addr_format) == 64);
   assert(nir_address_format_num_components(b->options->global_addr_format) == 1);
   assert(nir_address_format_bit_size(b->options->shared_addr_format) == 64);
   assert(nir_address_format_num_components(b->options->shared_addr_format) == 1);
   assert(nir_address_format_bit_size(b->options->constant_addr_format) == 64);
   assert(nir_address_format_num_components(b->options->constant_addr_format) == 1);
      case SpvAddressingModelLogical:
      vtn_fail_if(b->shader->info.stage == MESA_SHADER_KERNEL,
         b->physical_ptrs = false;
      case SpvAddressingModelPhysicalStorageBuffer64:
      vtn_fail_if(!b->options ||
                  default:
      vtn_fail("Unknown addressing model: %s (%u)",
                     b->mem_model = w[2];
   switch (w[2]) {
   case SpvMemoryModelSimple:
   case SpvMemoryModelGLSL450:
   case SpvMemoryModelOpenCL:
         case SpvMemoryModelVulkan:
      vtn_fail_if(!b->options->caps.vk_memory_model,
            default:
      vtn_fail("Unsupported memory model: %s",
            }
         case SpvOpEntryPoint:
      vtn_handle_entry_point(b, w, count);
         case SpvOpName:
      b->values[w[1]].name = vtn_string_literal(b, &w[2], count - 2, NULL);
         case SpvOpMemberName:
   case SpvOpExecutionMode:
   case SpvOpExecutionModeId:
   case SpvOpDecorationGroup:
   case SpvOpDecorate:
   case SpvOpDecorateId:
   case SpvOpMemberDecorate:
   case SpvOpGroupDecorate:
   case SpvOpGroupMemberDecorate:
   case SpvOpDecorateString:
   case SpvOpMemberDecorateString:
      vtn_handle_decoration(b, opcode, w, count);
         case SpvOpExtInst: {
      struct vtn_value *val = vtn_value(b, w[3], vtn_value_type_extension);
   if (val->ext_handler == vtn_handle_non_semantic_instruction) {
      /* NonSemantic extended instructions are acceptable in preamble. */
   vtn_handle_non_semantic_instruction(b, w[4], w, count);
      } else {
                     default:
                     }
      void
   vtn_handle_debug_text(struct vtn_builder *b, SpvOp opcode,
         {
      switch (opcode) {
   case SpvOpString:
      vtn_push_value(b, w[1], vtn_value_type_string)->str =
               case SpvOpSource: {
      const char *lang;
   switch (w[1]) {
   default:
   case SpvSourceLanguageUnknown:      lang = "unknown";    break;
   case SpvSourceLanguageESSL:         lang = "ESSL";       break;
   case SpvSourceLanguageGLSL:         lang = "GLSL";       break;
   case SpvSourceLanguageOpenCL_C:     lang = "OpenCL C";   break;
   case SpvSourceLanguageOpenCL_CPP:   lang = "OpenCL C++"; break;
   case SpvSourceLanguageHLSL:         lang = "HLSL";       break;
                     const char *file =
                     b->source_lang = w[1];
               case SpvOpSourceExtension:
   case SpvOpSourceContinued:
   case SpvOpModuleProcessed:
      /* Unhandled, but these are for debug so that's ok. */
         default:
            }
      static void
   vtn_handle_execution_mode(struct vtn_builder *b, struct vtn_value *entry_point,
         {
               switch(mode->exec_mode) {
   case SpvExecutionModeOriginUpperLeft:
   case SpvExecutionModeOriginLowerLeft:
      vtn_assert(b->shader->info.stage == MESA_SHADER_FRAGMENT);
   b->shader->info.fs.origin_upper_left =
               case SpvExecutionModeEarlyFragmentTests:
      vtn_assert(b->shader->info.stage == MESA_SHADER_FRAGMENT);
   b->shader->info.fs.early_fragment_tests = true;
         case SpvExecutionModePostDepthCoverage:
      vtn_assert(b->shader->info.stage == MESA_SHADER_FRAGMENT);
   b->shader->info.fs.post_depth_coverage = true;
         case SpvExecutionModeInvocations:
      vtn_assert(b->shader->info.stage == MESA_SHADER_GEOMETRY);
   b->shader->info.gs.invocations = MAX2(1, mode->operands[0]);
         case SpvExecutionModeDepthReplacing:
      vtn_assert(b->shader->info.stage == MESA_SHADER_FRAGMENT);
   b->shader->info.fs.depth_layout = FRAG_DEPTH_LAYOUT_ANY;
      case SpvExecutionModeDepthGreater:
      vtn_assert(b->shader->info.stage == MESA_SHADER_FRAGMENT);
   b->shader->info.fs.depth_layout = FRAG_DEPTH_LAYOUT_GREATER;
      case SpvExecutionModeDepthLess:
      vtn_assert(b->shader->info.stage == MESA_SHADER_FRAGMENT);
   b->shader->info.fs.depth_layout = FRAG_DEPTH_LAYOUT_LESS;
      case SpvExecutionModeDepthUnchanged:
      vtn_assert(b->shader->info.stage == MESA_SHADER_FRAGMENT);
   b->shader->info.fs.depth_layout = FRAG_DEPTH_LAYOUT_UNCHANGED;
         case SpvExecutionModeLocalSizeHint:
      vtn_assert(b->shader->info.stage == MESA_SHADER_KERNEL);
   b->shader->info.cs.workgroup_size_hint[0] = mode->operands[0];
   b->shader->info.cs.workgroup_size_hint[1] = mode->operands[1];
   b->shader->info.cs.workgroup_size_hint[2] = mode->operands[2];
         case SpvExecutionModeLocalSize:
      if (gl_shader_stage_uses_workgroup(b->shader->info.stage)) {
      b->shader->info.workgroup_size[0] = mode->operands[0];
   b->shader->info.workgroup_size[1] = mode->operands[1];
      } else {
      vtn_fail("Execution mode LocalSize not supported in stage %s",
      }
         case SpvExecutionModeOutputVertices:
      switch (b->shader->info.stage) {
   case MESA_SHADER_TESS_CTRL:
   case MESA_SHADER_TESS_EVAL:
      b->shader->info.tess.tcs_vertices_out = mode->operands[0];
      case MESA_SHADER_GEOMETRY:
      b->shader->info.gs.vertices_out = mode->operands[0];
      case MESA_SHADER_MESH:
      b->shader->info.mesh.max_vertices_out = mode->operands[0];
      default:
      vtn_fail("Execution mode OutputVertices not supported in stage %s",
            }
         case SpvExecutionModeInputPoints:
   case SpvExecutionModeInputLines:
   case SpvExecutionModeInputLinesAdjacency:
   case SpvExecutionModeTriangles:
   case SpvExecutionModeInputTrianglesAdjacency:
   case SpvExecutionModeQuads:
   case SpvExecutionModeIsolines:
      if (b->shader->info.stage == MESA_SHADER_TESS_CTRL ||
      b->shader->info.stage == MESA_SHADER_TESS_EVAL) {
   b->shader->info.tess._primitive_mode =
      } else {
      vtn_assert(b->shader->info.stage == MESA_SHADER_GEOMETRY);
   b->shader->info.gs.vertices_in =
         b->shader->info.gs.input_primitive =
      }
         case SpvExecutionModeOutputPrimitivesNV:
      vtn_assert(b->shader->info.stage == MESA_SHADER_MESH);
   b->shader->info.mesh.max_primitives_out = mode->operands[0];
         case SpvExecutionModeOutputLinesNV:
   case SpvExecutionModeOutputTrianglesNV:
      vtn_assert(b->shader->info.stage == MESA_SHADER_MESH);
   b->shader->info.mesh.primitive_type =
               case SpvExecutionModeOutputPoints: {
      const unsigned primitive =
            switch (b->shader->info.stage) {
   case MESA_SHADER_GEOMETRY:
      b->shader->info.gs.output_primitive = primitive;
      case MESA_SHADER_MESH:
      b->shader->info.mesh.primitive_type = primitive;
      default:
      vtn_fail("Execution mode OutputPoints not supported in stage %s",
            }
               case SpvExecutionModeOutputLineStrip:
   case SpvExecutionModeOutputTriangleStrip:
      vtn_assert(b->shader->info.stage == MESA_SHADER_GEOMETRY);
   b->shader->info.gs.output_primitive =
               case SpvExecutionModeSpacingEqual:
      vtn_assert(b->shader->info.stage == MESA_SHADER_TESS_CTRL ||
         b->shader->info.tess.spacing = TESS_SPACING_EQUAL;
      case SpvExecutionModeSpacingFractionalEven:
      vtn_assert(b->shader->info.stage == MESA_SHADER_TESS_CTRL ||
         b->shader->info.tess.spacing = TESS_SPACING_FRACTIONAL_EVEN;
      case SpvExecutionModeSpacingFractionalOdd:
      vtn_assert(b->shader->info.stage == MESA_SHADER_TESS_CTRL ||
         b->shader->info.tess.spacing = TESS_SPACING_FRACTIONAL_ODD;
      case SpvExecutionModeVertexOrderCw:
      vtn_assert(b->shader->info.stage == MESA_SHADER_TESS_CTRL ||
         b->shader->info.tess.ccw = false;
      case SpvExecutionModeVertexOrderCcw:
      vtn_assert(b->shader->info.stage == MESA_SHADER_TESS_CTRL ||
         b->shader->info.tess.ccw = true;
      case SpvExecutionModePointMode:
      vtn_assert(b->shader->info.stage == MESA_SHADER_TESS_CTRL ||
         b->shader->info.tess.point_mode = true;
         case SpvExecutionModePixelCenterInteger:
      vtn_assert(b->shader->info.stage == MESA_SHADER_FRAGMENT);
   b->shader->info.fs.pixel_center_integer = true;
         case SpvExecutionModeXfb:
      b->shader->info.has_transform_feedback_varyings = true;
         case SpvExecutionModeVecTypeHint:
            case SpvExecutionModeContractionOff:
      if (b->shader->info.stage != MESA_SHADER_KERNEL)
      vtn_warn("ExectionMode only allowed for CL-style kernels: %s",
      else
               case SpvExecutionModeStencilRefReplacingEXT:
      vtn_assert(b->shader->info.stage == MESA_SHADER_FRAGMENT);
         case SpvExecutionModeDerivativeGroupQuadsNV:
      vtn_assert(b->shader->info.stage == MESA_SHADER_COMPUTE);
   b->shader->info.cs.derivative_group = DERIVATIVE_GROUP_QUADS;
         case SpvExecutionModeDerivativeGroupLinearNV:
      vtn_assert(b->shader->info.stage == MESA_SHADER_COMPUTE);
   b->shader->info.cs.derivative_group = DERIVATIVE_GROUP_LINEAR;
         case SpvExecutionModePixelInterlockOrderedEXT:
      vtn_assert(b->shader->info.stage == MESA_SHADER_FRAGMENT);
   b->shader->info.fs.pixel_interlock_ordered = true;
         case SpvExecutionModePixelInterlockUnorderedEXT:
      vtn_assert(b->shader->info.stage == MESA_SHADER_FRAGMENT);
   b->shader->info.fs.pixel_interlock_unordered = true;
         case SpvExecutionModeSampleInterlockOrderedEXT:
      vtn_assert(b->shader->info.stage == MESA_SHADER_FRAGMENT);
   b->shader->info.fs.sample_interlock_ordered = true;
         case SpvExecutionModeSampleInterlockUnorderedEXT:
      vtn_assert(b->shader->info.stage == MESA_SHADER_FRAGMENT);
   b->shader->info.fs.sample_interlock_unordered = true;
         case SpvExecutionModeDenormPreserve:
   case SpvExecutionModeDenormFlushToZero:
   case SpvExecutionModeSignedZeroInfNanPreserve:
   case SpvExecutionModeRoundingModeRTE:
   case SpvExecutionModeRoundingModeRTZ: {
      unsigned execution_mode = 0;
   switch (mode->exec_mode) {
   case SpvExecutionModeDenormPreserve:
      switch (mode->operands[0]) {
   case 16: execution_mode = FLOAT_CONTROLS_DENORM_PRESERVE_FP16; break;
   case 32: execution_mode = FLOAT_CONTROLS_DENORM_PRESERVE_FP32; break;
   case 64: execution_mode = FLOAT_CONTROLS_DENORM_PRESERVE_FP64; break;
   default: vtn_fail("Floating point type not supported");
   }
      case SpvExecutionModeDenormFlushToZero:
      switch (mode->operands[0]) {
   case 16: execution_mode = FLOAT_CONTROLS_DENORM_FLUSH_TO_ZERO_FP16; break;
   case 32: execution_mode = FLOAT_CONTROLS_DENORM_FLUSH_TO_ZERO_FP32; break;
   case 64: execution_mode = FLOAT_CONTROLS_DENORM_FLUSH_TO_ZERO_FP64; break;
   default: vtn_fail("Floating point type not supported");
   }
      case SpvExecutionModeSignedZeroInfNanPreserve:
      switch (mode->operands[0]) {
   case 16: execution_mode = FLOAT_CONTROLS_SIGNED_ZERO_INF_NAN_PRESERVE_FP16; break;
   case 32: execution_mode = FLOAT_CONTROLS_SIGNED_ZERO_INF_NAN_PRESERVE_FP32; break;
   case 64: execution_mode = FLOAT_CONTROLS_SIGNED_ZERO_INF_NAN_PRESERVE_FP64; break;
   default: vtn_fail("Floating point type not supported");
   }
      case SpvExecutionModeRoundingModeRTE:
      switch (mode->operands[0]) {
   case 16: execution_mode = FLOAT_CONTROLS_ROUNDING_MODE_RTE_FP16; break;
   case 32: execution_mode = FLOAT_CONTROLS_ROUNDING_MODE_RTE_FP32; break;
   case 64: execution_mode = FLOAT_CONTROLS_ROUNDING_MODE_RTE_FP64; break;
   default: vtn_fail("Floating point type not supported");
   }
      case SpvExecutionModeRoundingModeRTZ:
      switch (mode->operands[0]) {
   case 16: execution_mode = FLOAT_CONTROLS_ROUNDING_MODE_RTZ_FP16; break;
   case 32: execution_mode = FLOAT_CONTROLS_ROUNDING_MODE_RTZ_FP32; break;
   case 64: execution_mode = FLOAT_CONTROLS_ROUNDING_MODE_RTZ_FP64; break;
   default: vtn_fail("Floating point type not supported");
   }
      default:
                           for (unsigned bit_size = 16; bit_size <= 64; bit_size *= 2) {
      vtn_fail_if(nir_is_denorm_flush_to_zero(b->shader->info.float_controls_execution_mode, bit_size) &&
               vtn_fail_if(nir_is_rounding_mode_rtne(b->shader->info.float_controls_execution_mode, bit_size) &&
            }
               case SpvExecutionModeLocalSizeId:
   case SpvExecutionModeLocalSizeHintId:
   case SpvExecutionModeSubgroupsPerWorkgroupId:
   case SpvExecutionModeMaxNodeRecursionAMDX:
   case SpvExecutionModeStaticNumWorkgroupsAMDX:
   case SpvExecutionModeMaxNumWorkgroupsAMDX:
   case SpvExecutionModeShaderIndexAMDX:
      /* Handled later by vtn_handle_execution_mode_id(). */
         case SpvExecutionModeSubgroupSize:
      vtn_assert(b->shader->info.stage == MESA_SHADER_KERNEL);
   vtn_assert(b->shader->info.subgroup_size == SUBGROUP_SIZE_VARYING);
   b->shader->info.subgroup_size = mode->operands[0];
         case SpvExecutionModeSubgroupsPerWorkgroup:
      vtn_assert(b->shader->info.stage == MESA_SHADER_KERNEL);
   b->shader->info.num_subgroups = mode->operands[0];
         case SpvExecutionModeSubgroupUniformControlFlowKHR:
      /* There's no corresponding SPIR-V capability, so check here. */
   vtn_fail_if(!b->options->caps.subgroup_uniform_control_flow,
               case SpvExecutionModeEarlyAndLateFragmentTestsAMD:
      vtn_assert(b->shader->info.stage == MESA_SHADER_FRAGMENT);
   b->shader->info.fs.early_and_late_fragment_tests = true;
         case SpvExecutionModeStencilRefGreaterFrontAMD:
      vtn_assert(b->shader->info.stage == MESA_SHADER_FRAGMENT);
   b->shader->info.fs.stencil_front_layout = FRAG_STENCIL_LAYOUT_GREATER;
         case SpvExecutionModeStencilRefLessFrontAMD:
      vtn_assert(b->shader->info.stage == MESA_SHADER_FRAGMENT);
   b->shader->info.fs.stencil_front_layout = FRAG_STENCIL_LAYOUT_LESS;
         case SpvExecutionModeStencilRefUnchangedFrontAMD:
      vtn_assert(b->shader->info.stage == MESA_SHADER_FRAGMENT);
   b->shader->info.fs.stencil_front_layout = FRAG_STENCIL_LAYOUT_UNCHANGED;
         case SpvExecutionModeStencilRefGreaterBackAMD:
      vtn_assert(b->shader->info.stage == MESA_SHADER_FRAGMENT);
   b->shader->info.fs.stencil_back_layout = FRAG_STENCIL_LAYOUT_GREATER;
         case SpvExecutionModeStencilRefLessBackAMD:
      vtn_assert(b->shader->info.stage == MESA_SHADER_FRAGMENT);
   b->shader->info.fs.stencil_back_layout = FRAG_STENCIL_LAYOUT_LESS;
         case SpvExecutionModeStencilRefUnchangedBackAMD:
      vtn_assert(b->shader->info.stage == MESA_SHADER_FRAGMENT);
   b->shader->info.fs.stencil_back_layout = FRAG_STENCIL_LAYOUT_UNCHANGED;
         case SpvExecutionModeCoalescingAMDX:
      vtn_assert(b->shader->info.stage == MESA_SHADER_COMPUTE);
   b->shader->info.cs.workgroup_count[0] = 1;
   b->shader->info.cs.workgroup_count[1] = 1;
   b->shader->info.cs.workgroup_count[2] = 1;
         default:
      vtn_fail("Unhandled execution mode: %s (%u)",
               }
      static void
   vtn_handle_execution_mode_id(struct vtn_builder *b, struct vtn_value *entry_point,
         {
                  switch (mode->exec_mode) {
   case SpvExecutionModeLocalSizeId:
      if (gl_shader_stage_uses_workgroup(b->shader->info.stage)) {
      b->shader->info.workgroup_size[0] = vtn_constant_uint(b, mode->operands[0]);
   b->shader->info.workgroup_size[1] = vtn_constant_uint(b, mode->operands[1]);
      } else {
      vtn_fail("Execution mode LocalSizeId not supported in stage %s",
      }
         case SpvExecutionModeLocalSizeHintId:
      vtn_assert(b->shader->info.stage == MESA_SHADER_KERNEL);
   b->shader->info.cs.workgroup_size_hint[0] = vtn_constant_uint(b, mode->operands[0]);
   b->shader->info.cs.workgroup_size_hint[1] = vtn_constant_uint(b, mode->operands[1]);
   b->shader->info.cs.workgroup_size_hint[2] = vtn_constant_uint(b, mode->operands[2]);
         case SpvExecutionModeSubgroupsPerWorkgroupId:
      vtn_assert(b->shader->info.stage == MESA_SHADER_KERNEL);
   b->shader->info.num_subgroups = vtn_constant_uint(b, mode->operands[0]);
         case SpvExecutionModeMaxNodeRecursionAMDX:
      vtn_assert(b->shader->info.stage == MESA_SHADER_COMPUTE);
         case SpvExecutionModeStaticNumWorkgroupsAMDX:
      vtn_assert(b->shader->info.stage == MESA_SHADER_COMPUTE);
   b->shader->info.cs.workgroup_count[0] = vtn_constant_uint(b, mode->operands[0]);
   b->shader->info.cs.workgroup_count[1] = vtn_constant_uint(b, mode->operands[1]);
   b->shader->info.cs.workgroup_count[2] = vtn_constant_uint(b, mode->operands[2]);
   assert(b->shader->info.cs.workgroup_count[0]);
   assert(b->shader->info.cs.workgroup_count[1]);
   assert(b->shader->info.cs.workgroup_count[2]);
         case SpvExecutionModeMaxNumWorkgroupsAMDX:
      vtn_assert(b->shader->info.stage == MESA_SHADER_COMPUTE);
         case SpvExecutionModeShaderIndexAMDX:
      vtn_assert(b->shader->info.stage == MESA_SHADER_COMPUTE);
   b->shader->info.cs.shader_index = vtn_constant_uint(b, mode->operands[0]);
         default:
      /* Nothing to do.  Literal execution modes already handled by
   * vtn_handle_execution_mode(). */
         }
      static bool
   vtn_handle_variable_or_type_instruction(struct vtn_builder *b, SpvOp opcode,
         {
               switch (opcode) {
   case SpvOpSource:
   case SpvOpSourceContinued:
   case SpvOpSourceExtension:
   case SpvOpExtension:
   case SpvOpCapability:
   case SpvOpExtInstImport:
   case SpvOpMemoryModel:
   case SpvOpEntryPoint:
   case SpvOpExecutionMode:
   case SpvOpString:
   case SpvOpName:
   case SpvOpMemberName:
   case SpvOpDecorationGroup:
   case SpvOpDecorate:
   case SpvOpDecorateId:
   case SpvOpMemberDecorate:
   case SpvOpGroupDecorate:
   case SpvOpGroupMemberDecorate:
   case SpvOpDecorateString:
   case SpvOpMemberDecorateString:
      vtn_fail("Invalid opcode types and variables section");
         case SpvOpTypeVoid:
   case SpvOpTypeBool:
   case SpvOpTypeInt:
   case SpvOpTypeFloat:
   case SpvOpTypeVector:
   case SpvOpTypeMatrix:
   case SpvOpTypeImage:
   case SpvOpTypeSampler:
   case SpvOpTypeSampledImage:
   case SpvOpTypeArray:
   case SpvOpTypeRuntimeArray:
   case SpvOpTypeStruct:
   case SpvOpTypeOpaque:
   case SpvOpTypePointer:
   case SpvOpTypeForwardPointer:
   case SpvOpTypeFunction:
   case SpvOpTypeEvent:
   case SpvOpTypeDeviceEvent:
   case SpvOpTypeReserveId:
   case SpvOpTypeQueue:
   case SpvOpTypePipe:
   case SpvOpTypeAccelerationStructureKHR:
   case SpvOpTypeRayQueryKHR:
   case SpvOpTypeCooperativeMatrixKHR:
      vtn_handle_type(b, opcode, w, count);
         case SpvOpConstantTrue:
   case SpvOpConstantFalse:
   case SpvOpConstant:
   case SpvOpConstantComposite:
   case SpvOpConstantNull:
   case SpvOpSpecConstantTrue:
   case SpvOpSpecConstantFalse:
   case SpvOpSpecConstant:
   case SpvOpSpecConstantComposite:
   case SpvOpSpecConstantOp:
      vtn_handle_constant(b, opcode, w, count);
         case SpvOpUndef:
   case SpvOpVariable:
   case SpvOpConstantSampler:
      vtn_handle_variables(b, opcode, w, count);
         case SpvOpExtInst: {
      struct vtn_value *val = vtn_value(b, w[3], vtn_value_type_extension);
   /* NonSemantic extended instructions are acceptable in preamble, others
   * will indicate the end of preamble.
   */
               default:
                     }
      static struct vtn_ssa_value *
   vtn_nir_select(struct vtn_builder *b, struct vtn_ssa_value *src0,
         {
      struct vtn_ssa_value *dest = rzalloc(b, struct vtn_ssa_value);
            if (src1->is_variable || src2->is_variable) {
               nir_variable *dest_var =
                  nir_push_if(&b->nb, src0->def);
   {
      nir_deref_instr *src1_deref = vtn_get_deref_for_ssa_value(b, src1);
      }
   nir_push_else(&b->nb, NULL);
   {
      nir_deref_instr *src2_deref = vtn_get_deref_for_ssa_value(b, src2);
      }
               } else if (glsl_type_is_vector_or_scalar(src1->type)) {
         } else {
               dest->elems = ralloc_array(b, struct vtn_ssa_value *, elems);
   for (unsigned i = 0; i < elems; i++) {
      dest->elems[i] = vtn_nir_select(b, src0,
                     }
      static void
   vtn_handle_select(struct vtn_builder *b, SpvOp opcode,
         {
      /* Handle OpSelect up-front here because it needs to be able to handle
   * pointers and not just regular vectors and scalars.
   */
   struct vtn_value *res_val = vtn_untyped_value(b, w[2]);
   struct vtn_value *cond_val = vtn_untyped_value(b, w[3]);
   struct vtn_value *obj1_val = vtn_untyped_value(b, w[4]);
            vtn_fail_if(obj1_val->type != res_val->type ||
                  vtn_fail_if((cond_val->type->base_type != vtn_base_type_scalar &&
               cond_val->type->base_type != vtn_base_type_vector) ||
            vtn_fail_if(cond_val->type->base_type == vtn_base_type_vector &&
               (res_val->type->base_type != vtn_base_type_vector ||
            switch (res_val->type->base_type) {
   case vtn_base_type_scalar:
   case vtn_base_type_vector:
   case vtn_base_type_matrix:
   case vtn_base_type_array:
   case vtn_base_type_struct:
      /* OK. */
      case vtn_base_type_pointer:
      /* We need to have actual storage for pointer types. */
   vtn_fail_if(res_val->type->type == NULL,
            default:
                  vtn_push_ssa_value(b, w[2],
      vtn_nir_select(b, vtn_ssa_value(b, w[3]),
         }
      static void
   vtn_handle_ptr(struct vtn_builder *b, SpvOp opcode,
         {
      struct vtn_type *type1 = vtn_get_value_type(b, w[3]);
   struct vtn_type *type2 = vtn_get_value_type(b, w[4]);
   vtn_fail_if(type1->base_type != vtn_base_type_pointer ||
               type2->base_type != vtn_base_type_pointer,
   vtn_fail_if(type1->storage_class != type2->storage_class,
                  struct vtn_type *vtn_type = vtn_get_type(b, w[1]);
            nir_address_format addr_format = vtn_mode_to_address_format(
                     switch (opcode) {
   case SpvOpPtrDiff: {
      /* OpPtrDiff returns the difference in number of elements (not byte offset). */
   unsigned elem_size, elem_align;
   glsl_get_natural_size_align_bytes(type1->deref->type,
            def = nir_build_addr_isub(&b->nb,
                     def = nir_idiv(&b->nb, def, nir_imm_intN_t(&b->nb, elem_size, def->bit_size));
   def = nir_i2iN(&b->nb, def, glsl_get_bit_size(type));
               case SpvOpPtrEqual:
   case SpvOpPtrNotEqual: {
      def = nir_build_addr_ieq(&b->nb,
                     if (opcode == SpvOpPtrNotEqual)
                     default:
                     }
      static void
   vtn_handle_ray_intrinsic(struct vtn_builder *b, SpvOp opcode,
         {
               switch (opcode) {
   case SpvOpTraceNV:
   case SpvOpTraceRayKHR: {
      intrin = nir_intrinsic_instr_create(b->nb.shader,
            /* The sources are in the same order in the NIR intrinsic */
   for (unsigned i = 0; i < 10; i++)
            nir_deref_instr *payload;
   if (opcode == SpvOpTraceNV)
         else
         intrin->src[10] = nir_src_for_ssa(&payload->def);
   nir_builder_instr_insert(&b->nb, &intrin->instr);
               case SpvOpReportIntersectionKHR: {
      intrin = nir_intrinsic_instr_create(b->nb.shader,
         intrin->src[0] = nir_src_for_ssa(vtn_ssa_value(b, w[3])->def);
   intrin->src[1] = nir_src_for_ssa(vtn_ssa_value(b, w[4])->def);
   nir_def_init(&intrin->instr, &intrin->def, 1, 1);
   nir_builder_instr_insert(&b->nb, &intrin->instr);
   vtn_push_nir_ssa(b, w[2], &intrin->def);
               case SpvOpIgnoreIntersectionNV:
      intrin = nir_intrinsic_instr_create(b->nb.shader,
         nir_builder_instr_insert(&b->nb, &intrin->instr);
         case SpvOpTerminateRayNV:
      intrin = nir_intrinsic_instr_create(b->nb.shader,
         nir_builder_instr_insert(&b->nb, &intrin->instr);
         case SpvOpExecuteCallableNV:
   case SpvOpExecuteCallableKHR: {
      intrin = nir_intrinsic_instr_create(b->nb.shader,
         intrin->src[0] = nir_src_for_ssa(vtn_ssa_value(b, w[1])->def);
   nir_deref_instr *payload;
   if (opcode == SpvOpExecuteCallableNV)
         else
         intrin->src[1] = nir_src_for_ssa(&payload->def);
   nir_builder_instr_insert(&b->nb, &intrin->instr);
               default:
            }
      static void
   vtn_handle_write_packed_primitive_indices(struct vtn_builder *b, SpvOp opcode,
         {
               /* TODO(mesh): Use or create a primitive that allow the unpacking to
   * happen in the backend.  What we have here is functional but too
   * blunt.
            struct vtn_type *offset_type = vtn_get_value_type(b, w[1]);
   vtn_fail_if(offset_type->base_type != vtn_base_type_scalar ||
                        struct vtn_type *packed_type = vtn_get_value_type(b, w[2]);
   vtn_fail_if(packed_type->base_type != vtn_base_type_scalar ||
                        nir_deref_instr *indices = NULL;
   nir_foreach_variable_with_modes(var, b->nb.shader, nir_var_shader_out) {
      if (var->data.location == VARYING_SLOT_PRIMITIVE_INDICES) {
      indices = nir_build_deref_var(&b->nb, var);
                  /* It may be the case that the variable is not present in the
   * entry point interface list.
   *
   * See https://github.com/KhronosGroup/SPIRV-Registry/issues/104.
            if (!indices) {
      unsigned vertices_per_prim =
         unsigned max_prim_indices =
         const struct glsl_type *t =
         nir_variable *var =
                  var->data.location = VARYING_SLOT_PRIMITIVE_INDICES;
   var->data.interpolation = INTERP_MODE_NONE;
               nir_def *offset = vtn_get_nir_ssa(b, w[1]);
   nir_def *packed = vtn_get_nir_ssa(b, w[2]);
   nir_def *unpacked = nir_unpack_bits(&b->nb, packed, 8);
   for (int i = 0; i < 4; i++) {
      nir_deref_instr *offset_deref =
      nir_build_deref_array(&b->nb, indices,
                     }
      struct ray_query_value {
      nir_ray_query_value     nir_value;
      };
      static struct ray_query_value
   spirv_to_nir_type_ray_query_intrinsic(struct vtn_builder *b,
         {
         #define CASE(_spv, _nir, _type) case SpvOpRayQueryGet##_spv:            \
         return (struct ray_query_value) { .nir_value = nir_ray_query_value_##_nir, .glsl_type = _type }
   CASE(RayTMinKHR,                                            tmin,                                   glsl_floatN_t_type(32));
   CASE(RayFlagsKHR,                                           flags,                                  glsl_uint_type());
   CASE(WorldRayDirectionKHR,                                  world_ray_direction,                    glsl_vec_type(3));
   CASE(WorldRayOriginKHR,                                     world_ray_origin,                       glsl_vec_type(3));
   CASE(IntersectionTypeKHR,                                   intersection_type,                      glsl_uint_type());
   CASE(IntersectionTKHR,                                      intersection_t,                         glsl_floatN_t_type(32));
   CASE(IntersectionInstanceCustomIndexKHR,                    intersection_instance_custom_index,     glsl_int_type());
   CASE(IntersectionInstanceIdKHR,                             intersection_instance_id,               glsl_int_type());
   CASE(IntersectionInstanceShaderBindingTableRecordOffsetKHR, intersection_instance_sbt_index,        glsl_uint_type());
   CASE(IntersectionGeometryIndexKHR,                          intersection_geometry_index,            glsl_int_type());
   CASE(IntersectionPrimitiveIndexKHR,                         intersection_primitive_index,           glsl_int_type());
   CASE(IntersectionBarycentricsKHR,                           intersection_barycentrics,              glsl_vec_type(2));
   CASE(IntersectionFrontFaceKHR,                              intersection_front_face,                glsl_bool_type());
   CASE(IntersectionCandidateAABBOpaqueKHR,                    intersection_candidate_aabb_opaque,     glsl_bool_type());
   CASE(IntersectionObjectToWorldKHR,                          intersection_object_to_world,           glsl_matrix_type(glsl_get_base_type(glsl_float_type()), 3, 4));
   CASE(IntersectionWorldToObjectKHR,                          intersection_world_to_object,           glsl_matrix_type(glsl_get_base_type(glsl_float_type()), 3, 4));
   CASE(IntersectionObjectRayOriginKHR,                        intersection_object_ray_origin,         glsl_vec_type(3));
   CASE(IntersectionObjectRayDirectionKHR,                     intersection_object_ray_direction,      glsl_vec_type(3));
   CASE(IntersectionTriangleVertexPositionsKHR,                intersection_triangle_vertex_positions, glsl_array_type(glsl_vec_type(3), 3,
   #undef CASE
      default:
            }
      static void
   ray_query_load_intrinsic_create(struct vtn_builder *b, SpvOp opcode,
               {
      struct ray_query_value value =
            if (glsl_type_is_array_or_matrix(value.glsl_type)) {
      const struct glsl_type *elem_type = glsl_get_array_element(value.glsl_type);
            struct vtn_ssa_value *ssa = vtn_create_ssa_value(b, value.glsl_type);
   for (unsigned i = 0; i < elems; i++) {
      ssa->elems[i]->def =
      nir_rq_load(&b->nb,
               glsl_get_vector_elements(elem_type),
   glsl_get_bit_size(elem_type),
   src0,
               } else {
               vtn_push_nir_ssa(b, w[2],
                  nir_rq_load(&b->nb,
                  }
      static void
   vtn_handle_ray_query_intrinsic(struct vtn_builder *b, SpvOp opcode,
         {
      switch (opcode) {
   case SpvOpRayQueryInitializeKHR: {
      nir_intrinsic_instr *intrin =
      nir_intrinsic_instr_create(b->nb.shader,
      /* The sources are in the same order in the NIR intrinsic */
   for (unsigned i = 0; i < 8; i++)
         nir_builder_instr_insert(&b->nb, &intrin->instr);
               case SpvOpRayQueryTerminateKHR:
      nir_rq_terminate(&b->nb, vtn_ssa_value(b, w[1])->def);
         case SpvOpRayQueryProceedKHR:
      vtn_push_nir_ssa(b, w[2],
               case SpvOpRayQueryGenerateIntersectionKHR:
      nir_rq_generate_intersection(&b->nb,
                     case SpvOpRayQueryConfirmIntersectionKHR:
      nir_rq_confirm_intersection(&b->nb, vtn_ssa_value(b, w[1])->def);
         case SpvOpRayQueryGetIntersectionTKHR:
   case SpvOpRayQueryGetIntersectionTypeKHR:
   case SpvOpRayQueryGetIntersectionInstanceCustomIndexKHR:
   case SpvOpRayQueryGetIntersectionInstanceIdKHR:
   case SpvOpRayQueryGetIntersectionInstanceShaderBindingTableRecordOffsetKHR:
   case SpvOpRayQueryGetIntersectionGeometryIndexKHR:
   case SpvOpRayQueryGetIntersectionPrimitiveIndexKHR:
   case SpvOpRayQueryGetIntersectionBarycentricsKHR:
   case SpvOpRayQueryGetIntersectionFrontFaceKHR:
   case SpvOpRayQueryGetIntersectionObjectRayDirectionKHR:
   case SpvOpRayQueryGetIntersectionObjectRayOriginKHR:
   case SpvOpRayQueryGetIntersectionObjectToWorldKHR:
   case SpvOpRayQueryGetIntersectionWorldToObjectKHR:
   case SpvOpRayQueryGetIntersectionTriangleVertexPositionsKHR:
      ray_query_load_intrinsic_create(b, opcode, w,
                     case SpvOpRayQueryGetRayTMinKHR:
   case SpvOpRayQueryGetRayFlagsKHR:
   case SpvOpRayQueryGetWorldRayDirectionKHR:
   case SpvOpRayQueryGetWorldRayOriginKHR:
   case SpvOpRayQueryGetIntersectionCandidateAABBOpaqueKHR:
      ray_query_load_intrinsic_create(b, opcode, w,
                           default:
            }
      static void
   vtn_handle_initialize_node_payloads(struct vtn_builder *b, SpvOp opcode,
         {
               nir_def *payloads = vtn_ssa_value(b, w[1])->def;
   mesa_scope scope = vtn_translate_scope(b, vtn_constant_uint(b, w[2]));
   nir_def *payload_count = vtn_ssa_value(b, w[3])->def;
               }
      static bool
   vtn_handle_body_instruction(struct vtn_builder *b, SpvOp opcode,
         {
      switch (opcode) {
   case SpvOpLabel:
            case SpvOpLoopMerge:
   case SpvOpSelectionMerge:
      /* This is handled by cfg pre-pass and walk_blocks */
         case SpvOpUndef: {
      struct vtn_value *val = vtn_push_value(b, w[2], vtn_value_type_undef);
   val->type = vtn_get_type(b, w[1]);
               case SpvOpExtInst:
      vtn_handle_extension(b, opcode, w, count);
         case SpvOpVariable:
   case SpvOpLoad:
   case SpvOpStore:
   case SpvOpCopyMemory:
   case SpvOpCopyMemorySized:
   case SpvOpAccessChain:
   case SpvOpPtrAccessChain:
   case SpvOpInBoundsAccessChain:
   case SpvOpInBoundsPtrAccessChain:
   case SpvOpArrayLength:
   case SpvOpConvertPtrToU:
   case SpvOpConvertUToPtr:
   case SpvOpGenericCastToPtrExplicit:
   case SpvOpGenericPtrMemSemantics:
   case SpvOpSubgroupBlockReadINTEL:
   case SpvOpSubgroupBlockWriteINTEL:
   case SpvOpConvertUToAccelerationStructureKHR:
      vtn_handle_variables(b, opcode, w, count);
         case SpvOpFunctionCall:
      vtn_handle_function_call(b, opcode, w, count);
         case SpvOpSampledImage:
   case SpvOpImage:
   case SpvOpImageSparseTexelsResident:
   case SpvOpImageSampleImplicitLod:
   case SpvOpImageSparseSampleImplicitLod:
   case SpvOpImageSampleExplicitLod:
   case SpvOpImageSparseSampleExplicitLod:
   case SpvOpImageSampleDrefImplicitLod:
   case SpvOpImageSparseSampleDrefImplicitLod:
   case SpvOpImageSampleDrefExplicitLod:
   case SpvOpImageSparseSampleDrefExplicitLod:
   case SpvOpImageSampleProjImplicitLod:
   case SpvOpImageSampleProjExplicitLod:
   case SpvOpImageSampleProjDrefImplicitLod:
   case SpvOpImageSampleProjDrefExplicitLod:
   case SpvOpImageFetch:
   case SpvOpImageSparseFetch:
   case SpvOpImageGather:
   case SpvOpImageSparseGather:
   case SpvOpImageDrefGather:
   case SpvOpImageSparseDrefGather:
   case SpvOpImageQueryLod:
   case SpvOpImageQueryLevels:
      vtn_handle_texture(b, opcode, w, count);
         case SpvOpImageRead:
   case SpvOpImageSparseRead:
   case SpvOpImageWrite:
   case SpvOpImageTexelPointer:
   case SpvOpImageQueryFormat:
   case SpvOpImageQueryOrder:
      vtn_handle_image(b, opcode, w, count);
         case SpvOpImageQuerySamples:
   case SpvOpImageQuerySizeLod:
   case SpvOpImageQuerySize: {
      struct vtn_type *image_type = vtn_get_value_type(b, w[3]);
   vtn_assert(image_type->base_type == vtn_base_type_image);
   if (glsl_type_is_image(image_type->glsl_image)) {
         } else {
      vtn_assert(glsl_type_is_texture(image_type->glsl_image));
      }
               case SpvOpFragmentMaskFetchAMD:
   case SpvOpFragmentFetchAMD:
      vtn_handle_texture(b, opcode, w, count);
         case SpvOpAtomicLoad:
   case SpvOpAtomicExchange:
   case SpvOpAtomicCompareExchange:
   case SpvOpAtomicCompareExchangeWeak:
   case SpvOpAtomicIIncrement:
   case SpvOpAtomicIDecrement:
   case SpvOpAtomicIAdd:
   case SpvOpAtomicISub:
   case SpvOpAtomicSMin:
   case SpvOpAtomicUMin:
   case SpvOpAtomicSMax:
   case SpvOpAtomicUMax:
   case SpvOpAtomicAnd:
   case SpvOpAtomicOr:
   case SpvOpAtomicXor:
   case SpvOpAtomicFAddEXT:
   case SpvOpAtomicFMinEXT:
   case SpvOpAtomicFMaxEXT:
   case SpvOpAtomicFlagTestAndSet: {
      struct vtn_value *pointer = vtn_untyped_value(b, w[3]);
   if (pointer->value_type == vtn_value_type_image_pointer) {
         } else {
      vtn_assert(pointer->value_type == vtn_value_type_pointer);
      }
               case SpvOpAtomicStore:
   case SpvOpAtomicFlagClear: {
      struct vtn_value *pointer = vtn_untyped_value(b, w[1]);
   if (pointer->value_type == vtn_value_type_image_pointer) {
         } else {
      vtn_assert(pointer->value_type == vtn_value_type_pointer);
      }
               case SpvOpSelect:
      vtn_handle_select(b, opcode, w, count);
         case SpvOpSNegate:
   case SpvOpFNegate:
   case SpvOpNot:
   case SpvOpAny:
   case SpvOpAll:
   case SpvOpConvertFToU:
   case SpvOpConvertFToS:
   case SpvOpConvertSToF:
   case SpvOpConvertUToF:
   case SpvOpUConvert:
   case SpvOpSConvert:
   case SpvOpFConvert:
   case SpvOpQuantizeToF16:
   case SpvOpSatConvertSToU:
   case SpvOpSatConvertUToS:
   case SpvOpPtrCastToGeneric:
   case SpvOpGenericCastToPtr:
   case SpvOpIsNan:
   case SpvOpIsInf:
   case SpvOpIsFinite:
   case SpvOpIsNormal:
   case SpvOpSignBitSet:
   case SpvOpLessOrGreater:
   case SpvOpOrdered:
   case SpvOpUnordered:
   case SpvOpIAdd:
   case SpvOpFAdd:
   case SpvOpISub:
   case SpvOpFSub:
   case SpvOpIMul:
   case SpvOpFMul:
   case SpvOpUDiv:
   case SpvOpSDiv:
   case SpvOpFDiv:
   case SpvOpUMod:
   case SpvOpSRem:
   case SpvOpSMod:
   case SpvOpFRem:
   case SpvOpFMod:
   case SpvOpVectorTimesScalar:
   case SpvOpDot:
   case SpvOpIAddCarry:
   case SpvOpISubBorrow:
   case SpvOpUMulExtended:
   case SpvOpSMulExtended:
   case SpvOpShiftRightLogical:
   case SpvOpShiftRightArithmetic:
   case SpvOpShiftLeftLogical:
   case SpvOpLogicalEqual:
   case SpvOpLogicalNotEqual:
   case SpvOpLogicalOr:
   case SpvOpLogicalAnd:
   case SpvOpLogicalNot:
   case SpvOpBitwiseOr:
   case SpvOpBitwiseXor:
   case SpvOpBitwiseAnd:
   case SpvOpIEqual:
   case SpvOpFOrdEqual:
   case SpvOpFUnordEqual:
   case SpvOpINotEqual:
   case SpvOpFOrdNotEqual:
   case SpvOpFUnordNotEqual:
   case SpvOpULessThan:
   case SpvOpSLessThan:
   case SpvOpFOrdLessThan:
   case SpvOpFUnordLessThan:
   case SpvOpUGreaterThan:
   case SpvOpSGreaterThan:
   case SpvOpFOrdGreaterThan:
   case SpvOpFUnordGreaterThan:
   case SpvOpULessThanEqual:
   case SpvOpSLessThanEqual:
   case SpvOpFOrdLessThanEqual:
   case SpvOpFUnordLessThanEqual:
   case SpvOpUGreaterThanEqual:
   case SpvOpSGreaterThanEqual:
   case SpvOpFOrdGreaterThanEqual:
   case SpvOpFUnordGreaterThanEqual:
   case SpvOpDPdx:
   case SpvOpDPdy:
   case SpvOpFwidth:
   case SpvOpDPdxFine:
   case SpvOpDPdyFine:
   case SpvOpFwidthFine:
   case SpvOpDPdxCoarse:
   case SpvOpDPdyCoarse:
   case SpvOpFwidthCoarse:
   case SpvOpBitFieldInsert:
   case SpvOpBitFieldSExtract:
   case SpvOpBitFieldUExtract:
   case SpvOpBitReverse:
   case SpvOpBitCount:
   case SpvOpTranspose:
   case SpvOpOuterProduct:
   case SpvOpMatrixTimesScalar:
   case SpvOpVectorTimesMatrix:
   case SpvOpMatrixTimesVector:
   case SpvOpMatrixTimesMatrix:
   case SpvOpUCountLeadingZerosINTEL:
   case SpvOpUCountTrailingZerosINTEL:
   case SpvOpAbsISubINTEL:
   case SpvOpAbsUSubINTEL:
   case SpvOpIAddSatINTEL:
   case SpvOpUAddSatINTEL:
   case SpvOpIAverageINTEL:
   case SpvOpUAverageINTEL:
   case SpvOpIAverageRoundedINTEL:
   case SpvOpUAverageRoundedINTEL:
   case SpvOpISubSatINTEL:
   case SpvOpUSubSatINTEL:
   case SpvOpIMul32x16INTEL:
   case SpvOpUMul32x16INTEL:
      vtn_handle_alu(b, opcode, w, count);
         case SpvOpSDotKHR:
   case SpvOpUDotKHR:
   case SpvOpSUDotKHR:
   case SpvOpSDotAccSatKHR:
   case SpvOpUDotAccSatKHR:
   case SpvOpSUDotAccSatKHR:
      vtn_handle_integer_dot(b, opcode, w, count);
         /* TODO: One day, we should probably do something with this information
   * For now, though, it's safe to implement them as no-ops.
   * Needed for Rusticl sycl support.
   */
   case SpvOpAssumeTrueKHR:
   case SpvOpExpectKHR:
            case SpvOpBitcast:
      vtn_handle_bitcast(b, w, count);
         case SpvOpVectorExtractDynamic:
   case SpvOpVectorInsertDynamic:
   case SpvOpVectorShuffle:
   case SpvOpCompositeConstruct:
   case SpvOpCompositeExtract:
   case SpvOpCompositeInsert:
   case SpvOpCopyLogical:
   case SpvOpCopyObject:
      vtn_handle_composite(b, opcode, w, count);
         case SpvOpEmitVertex:
   case SpvOpEndPrimitive:
   case SpvOpEmitStreamVertex:
   case SpvOpEndStreamPrimitive:
   case SpvOpControlBarrier:
   case SpvOpMemoryBarrier:
      vtn_handle_barrier(b, opcode, w, count);
         case SpvOpGroupNonUniformElect:
   case SpvOpGroupNonUniformAll:
   case SpvOpGroupNonUniformAny:
   case SpvOpGroupNonUniformAllEqual:
   case SpvOpGroupNonUniformBroadcast:
   case SpvOpGroupNonUniformBroadcastFirst:
   case SpvOpGroupNonUniformBallot:
   case SpvOpGroupNonUniformInverseBallot:
   case SpvOpGroupNonUniformBallotBitExtract:
   case SpvOpGroupNonUniformBallotBitCount:
   case SpvOpGroupNonUniformBallotFindLSB:
   case SpvOpGroupNonUniformBallotFindMSB:
   case SpvOpGroupNonUniformShuffle:
   case SpvOpGroupNonUniformShuffleXor:
   case SpvOpGroupNonUniformShuffleUp:
   case SpvOpGroupNonUniformShuffleDown:
   case SpvOpGroupNonUniformIAdd:
   case SpvOpGroupNonUniformFAdd:
   case SpvOpGroupNonUniformIMul:
   case SpvOpGroupNonUniformFMul:
   case SpvOpGroupNonUniformSMin:
   case SpvOpGroupNonUniformUMin:
   case SpvOpGroupNonUniformFMin:
   case SpvOpGroupNonUniformSMax:
   case SpvOpGroupNonUniformUMax:
   case SpvOpGroupNonUniformFMax:
   case SpvOpGroupNonUniformBitwiseAnd:
   case SpvOpGroupNonUniformBitwiseOr:
   case SpvOpGroupNonUniformBitwiseXor:
   case SpvOpGroupNonUniformLogicalAnd:
   case SpvOpGroupNonUniformLogicalOr:
   case SpvOpGroupNonUniformLogicalXor:
   case SpvOpGroupNonUniformQuadBroadcast:
   case SpvOpGroupNonUniformQuadSwap:
   case SpvOpGroupAll:
   case SpvOpGroupAny:
   case SpvOpGroupBroadcast:
   case SpvOpGroupIAdd:
   case SpvOpGroupFAdd:
   case SpvOpGroupFMin:
   case SpvOpGroupUMin:
   case SpvOpGroupSMin:
   case SpvOpGroupFMax:
   case SpvOpGroupUMax:
   case SpvOpGroupSMax:
   case SpvOpSubgroupBallotKHR:
   case SpvOpSubgroupFirstInvocationKHR:
   case SpvOpSubgroupReadInvocationKHR:
   case SpvOpSubgroupAllKHR:
   case SpvOpSubgroupAnyKHR:
   case SpvOpSubgroupAllEqualKHR:
   case SpvOpGroupIAddNonUniformAMD:
   case SpvOpGroupFAddNonUniformAMD:
   case SpvOpGroupFMinNonUniformAMD:
   case SpvOpGroupUMinNonUniformAMD:
   case SpvOpGroupSMinNonUniformAMD:
   case SpvOpGroupFMaxNonUniformAMD:
   case SpvOpGroupUMaxNonUniformAMD:
   case SpvOpGroupSMaxNonUniformAMD:
   case SpvOpSubgroupShuffleINTEL:
   case SpvOpSubgroupShuffleDownINTEL:
   case SpvOpSubgroupShuffleUpINTEL:
   case SpvOpSubgroupShuffleXorINTEL:
   case SpvOpGroupNonUniformRotateKHR:
      vtn_handle_subgroup(b, opcode, w, count);
         case SpvOpPtrDiff:
   case SpvOpPtrEqual:
   case SpvOpPtrNotEqual:
      vtn_handle_ptr(b, opcode, w, count);
         case SpvOpBeginInvocationInterlockEXT:
      nir_begin_invocation_interlock(&b->nb);
         case SpvOpEndInvocationInterlockEXT:
      nir_end_invocation_interlock(&b->nb);
         case SpvOpDemoteToHelperInvocation: {
      nir_demote(&b->nb);
               case SpvOpIsHelperInvocationEXT: {
      vtn_push_nir_ssa(b, w[2], nir_is_helper_invocation(&b->nb, 1));
               case SpvOpReadClockKHR: {
      SpvScope scope = vtn_constant_uint(b, w[3]);
   vtn_fail_if(scope != SpvScopeDevice && scope != SpvScopeSubgroup,
                  /* Operation supports two result types: uvec2 and uint64_t.  The NIR
   * intrinsic gives uvec2, so pack the result for the other case.
   */
            struct vtn_type *type = vtn_get_type(b, w[1]);
            if (glsl_type_is_vector(dest_type)) {
         } else {
      assert(glsl_type_is_scalar(dest_type));
   assert(glsl_get_base_type(dest_type) == GLSL_TYPE_UINT64);
               vtn_push_nir_ssa(b, w[2], result);
               case SpvOpTraceNV:
   case SpvOpTraceRayKHR:
   case SpvOpReportIntersectionKHR:
   case SpvOpIgnoreIntersectionNV:
   case SpvOpTerminateRayNV:
   case SpvOpExecuteCallableNV:
   case SpvOpExecuteCallableKHR:
      vtn_handle_ray_intrinsic(b, opcode, w, count);
         case SpvOpRayQueryInitializeKHR:
   case SpvOpRayQueryTerminateKHR:
   case SpvOpRayQueryGenerateIntersectionKHR:
   case SpvOpRayQueryConfirmIntersectionKHR:
   case SpvOpRayQueryProceedKHR:
   case SpvOpRayQueryGetIntersectionTypeKHR:
   case SpvOpRayQueryGetRayTMinKHR:
   case SpvOpRayQueryGetRayFlagsKHR:
   case SpvOpRayQueryGetIntersectionTKHR:
   case SpvOpRayQueryGetIntersectionInstanceCustomIndexKHR:
   case SpvOpRayQueryGetIntersectionInstanceIdKHR:
   case SpvOpRayQueryGetIntersectionInstanceShaderBindingTableRecordOffsetKHR:
   case SpvOpRayQueryGetIntersectionGeometryIndexKHR:
   case SpvOpRayQueryGetIntersectionPrimitiveIndexKHR:
   case SpvOpRayQueryGetIntersectionBarycentricsKHR:
   case SpvOpRayQueryGetIntersectionFrontFaceKHR:
   case SpvOpRayQueryGetIntersectionCandidateAABBOpaqueKHR:
   case SpvOpRayQueryGetIntersectionObjectRayDirectionKHR:
   case SpvOpRayQueryGetIntersectionObjectRayOriginKHR:
   case SpvOpRayQueryGetWorldRayDirectionKHR:
   case SpvOpRayQueryGetWorldRayOriginKHR:
   case SpvOpRayQueryGetIntersectionObjectToWorldKHR:
   case SpvOpRayQueryGetIntersectionWorldToObjectKHR:
   case SpvOpRayQueryGetIntersectionTriangleVertexPositionsKHR:
      vtn_handle_ray_query_intrinsic(b, opcode, w, count);
         case SpvOpLifetimeStart:
   case SpvOpLifetimeStop:
            case SpvOpGroupAsyncCopy:
   case SpvOpGroupWaitEvents:
      vtn_handle_opencl_core_instruction(b, opcode, w, count);
         case SpvOpWritePackedPrimitiveIndices4x8NV:
      vtn_handle_write_packed_primitive_indices(b, opcode, w, count);
         case SpvOpSetMeshOutputsEXT:
      nir_set_vertex_and_primitive_count(
               case SpvOpInitializeNodePayloadsAMDX:
      vtn_handle_initialize_node_payloads(b, opcode, w, count);
         case SpvOpFinalizeNodePayloadsAMDX:
            case SpvOpFinishWritingNodePayloadAMDX:
            case SpvOpCooperativeMatrixLoadKHR:
   case SpvOpCooperativeMatrixStoreKHR:
   case SpvOpCooperativeMatrixLengthKHR:
   case SpvOpCooperativeMatrixMulAddKHR:
      vtn_handle_cooperative_instruction(b, opcode, w, count);
         default:
                     }
      static bool
   is_glslang(const struct vtn_builder *b)
   {
      return b->generator_id == vtn_generator_glslang_reference_front_end ||
      }
      struct vtn_builder*
   vtn_create_builder(const uint32_t *words, size_t word_count,
               {
      /* Initialize the vtn_builder object */
   struct vtn_builder *b = rzalloc(NULL, struct vtn_builder);
   struct spirv_to_nir_options *dup_options =
                  b->spirv = words;
   b->spirv_word_count = word_count;
   b->file = NULL;
   b->line = -1;
   b->col = -1;
   list_inithead(&b->functions);
   b->entry_point_stage = stage;
   b->entry_point_name = entry_point_name;
            /*
   * Handle the SPIR-V header (first 5 dwords).
   * Can't use vtx_assert() as the setjmp(3) target isn't initialized yet.
   */
   if (word_count <= 5)
            if (words[0] != SpvMagicNumber) {
      vtn_err("words[0] was 0x%x, want 0x%x", words[0], SpvMagicNumber);
               b->version = words[1];
   if (b->version < 0x10000) {
      vtn_err("version was 0x%x, want >= 0x10000", b->version);
               b->generator_id = words[2] >> 16;
            /* In GLSLang commit 8297936dd6eb3, their handling of barrier() was fixed
   * to provide correct memory semantics on compute shader barrier()
   * commands.  Prior to that, we need to fix them up ourselves.  This
   * GLSLang fix caused them to bump to generator version 3.
   */
            /* Identifying the LLVM-SPIRV translator:
   *
   * The LLVM-SPIRV translator currently doesn't store any generator ID [1].
   * Our use case involving the SPIRV-Tools linker also mean we want to check
   * for that tool instead. Finally the SPIRV-Tools linker also stores its
   * generator ID in the wrong location [2].
   *
   * [1] : https://github.com/KhronosGroup/SPIRV-LLVM-Translator/pull/1223
   * [2] : https://github.com/KhronosGroup/SPIRV-Tools/pull/4549
   */
   const bool is_llvm_spirv_translator =
      (b->generator_id == 0 &&
   generator_version == vtn_generator_spirv_tools_linker) ||
         /* The LLVM-SPIRV translator generates Undef initializers for _local
   * variables [1].
   *
   * [1] : https://github.com/KhronosGroup/SPIRV-LLVM-Translator/issues/1224
   */
   b->wa_llvm_spirv_ignore_workgroup_initializer =
            /* Older versions of GLSLang would incorrectly emit OpReturn after
   * OpEmitMeshTasksEXT. This is incorrect since the latter is already
   * a terminator instruction.
   *
   * See https://github.com/KhronosGroup/glslang/issues/3020 for details.
   *
   * Clay Shader Compiler (used by GravityMark) is also affected.
   */
   b->wa_ignore_return_after_emit_mesh_tasks =
      (is_glslang(b) && generator_version < 11) ||
   (b->generator_id == vtn_generator_clay_shader_compiler &&
         /* words[2] == generator magic */
   unsigned value_id_bound = words[3];
   if (words[4] != 0) {
      vtn_err("words[4] was %u, want 0", words[4]);
               b->value_id_bound = value_id_bound;
            if (b->options->environment == NIR_SPIRV_VULKAN && b->version < 0x10400)
               fail:
      ralloc_free(b);
      }
      static nir_function *
   vtn_emit_kernel_entry_point_wrapper(struct vtn_builder *b,
         {
      vtn_assert(entry_point == b->entry_point->func->nir_func);
   vtn_fail_if(!entry_point->name, "entry points are required to have a name");
   const char *func_name =
                     nir_function *main_entry_point = nir_function_create(b->shader, func_name);
   nir_function_impl *impl = nir_function_impl_create(main_entry_point);
   b->nb = nir_builder_at(nir_after_impl(impl));
                     for (unsigned i = 0; i < entry_point->num_params; ++i) {
               b->shader->info.cs.has_variable_shared_mem |=
            /* consider all pointers to function memory to be parameters passed
   * by value
   */
   bool is_by_val = param_type->base_type == vtn_base_type_pointer &&
            /* input variable */
            if (is_by_val) {
      in_var->data.mode = nir_var_uniform;
      } else if (param_type->base_type == vtn_base_type_image) {
      in_var->data.mode = nir_var_image;
   in_var->type = param_type->glsl_image;
   in_var->data.access =
      } else if (param_type->base_type == vtn_base_type_sampler) {
      in_var->data.mode = nir_var_uniform;
      } else {
      in_var->data.mode = nir_var_uniform;
               in_var->data.read_only = true;
                     /* we have to copy the entire variable into function memory */
   if (is_by_val) {
      nir_variable *copy_var =
         nir_copy_var(&b->nb, copy_var, in_var);
   call->params[i] =
      } else if (param_type->base_type == vtn_base_type_image ||
            /* Don't load the var, just pass a deref of it */
      } else {
                                 }
      static bool
   can_remove(nir_variable *var, void *data)
   {
      const struct set *vars_used_indirectly = data;
      }
      #ifndef NDEBUG
   static void
   initialize_mesa_spirv_debug(void)
   {
         }
   #endif
      nir_shader *
   spirv_to_nir(const uint32_t *words, size_t word_count,
               struct nir_spirv_specialization *spec, unsigned num_spec,
   gl_shader_stage stage, const char *entry_point_name,
      {
   #ifndef NDEBUG
      static once_flag initialized_debug_flag = ONCE_FLAG_INIT;
      #endif
                  struct vtn_builder *b = vtn_create_builder(words, word_count,
                  if (b == NULL)
            /* See also _vtn_fail() */
   if (vtn_setjmp(b->fail_jump)) {
      ralloc_free(b);
               b->shader = nir_shader_create(b, stage, nir_options, NULL);
   b->shader->info.subgroup_size = options->subgroup_size;
   b->shader->info.float_controls_execution_mode = options->float_controls_execution_mode;
   b->shader->info.cs.shader_index = options->shader_index;
            /* Skip the SPIR-V header, handled at vtn_create_builder */
            /* Handle all the preamble instructions */
   words = vtn_foreach_instruction(b, words, word_end,
            /* DirectXShaderCompiler and glslang/shaderc both create OpKill from HLSL's
   * discard/clip, which uses demote semantics. DirectXShaderCompiler will use
   * demote if the extension is enabled, so we disable this workaround in that
   * case.
   *
   * Related glslang issue: https://github.com/KhronosGroup/glslang/issues/2416
   */
   bool dxsc = b->generator_id == vtn_generator_spiregg;
   b->convert_discard_to_demote = ((dxsc && !b->uses_demote_to_helper_invocation) ||
                  if (!options->create_library && b->entry_point == NULL) {
      vtn_fail("Entry point not found for %s shader \"%s\"",
         ralloc_free(b);
               /* Ensure a sane address mode is being used for function temps */
   assert(nir_address_format_bit_size(b->options->temp_addr_format) == nir_get_ptr_bitsize(b->shader));
            /* Set shader info defaults */
   if (stage == MESA_SHADER_GEOMETRY)
            /* Parse execution modes. */
   if (!options->create_library)
      vtn_foreach_execution_mode(b, b->entry_point,
         b->specializations = spec;
            /* Handle all variable, type, and constant instructions */
   words = vtn_foreach_instruction(b, words, word_end,
            /* Parse execution modes that depend on IDs. Must happen after we have
   * constants parsed.
   */
   if (!options->create_library)
      vtn_foreach_execution_mode(b, b->entry_point,
         if (b->workgroup_size_builtin) {
      vtn_assert(gl_shader_stage_uses_workgroup(stage));
   vtn_assert(b->workgroup_size_builtin->type->type ==
            nir_const_value *const_size =
            b->shader->info.workgroup_size[0] = const_size[0].u32;
   b->shader->info.workgroup_size[1] = const_size[1].u32;
               /* Set types on all vtn_values */
                     if (!options->create_library) {
      assert(b->entry_point->value_type == vtn_value_type_function);
               bool progress;
   do {
      progress = false;
   vtn_foreach_function(func, &b->functions) {
      if ((options->create_library || func->referenced) && !func->emitted) {
      vtn_function_emit(b, func, vtn_handle_body_instruction);
                     if (!options->create_library) {
      vtn_assert(b->entry_point->value_type == vtn_value_type_function);
   nir_function *entry_point = b->entry_point->func->nir_func;
            entry_point->dont_inline = false;
   /* post process entry_points with input params */
   if (entry_point->num_params && b->shader->info.stage == MESA_SHADER_KERNEL)
                        /* structurize the CFG */
                              /* A SPIR-V module can have multiple shaders stages and also multiple
   * shaders of the same stage.  Global variables are declared per-module.
   *
   * Starting in SPIR-V 1.4 the list of global variables is part of
   * OpEntryPoint, so only valid ones will be created.  Previous versions
   * only have Input and Output variables listed, so remove dead variables to
   * clean up the remaining ones.
   */
   if (!options->create_library && b->version < 0x10400) {
      const nir_remove_dead_variables_options dead_opts = {
      .can_remove_var = can_remove,
      };
   nir_remove_dead_variables(b->shader, ~(nir_var_function_temp |
                                 nir_foreach_variable_in_shader(var, b->shader) {
      switch (var->data.mode) {
   case nir_var_mem_ubo:
      b->shader->info.num_ubos++;
      case nir_var_mem_ssbo:
      b->shader->info.num_ssbos++;
      case nir_var_mem_push_const:
      vtn_assert(b->shader->num_uniforms == 0);
   b->shader->num_uniforms =
                        /* We sometimes generate bogus derefs that, while never used, give the
   * validator a bit of heartburn.  Run dead code to get rid of them.
   */
            /* Per SPV_KHR_workgroup_storage_explicit_layout, if one shared variable is
   * a Block, all of them will be and Blocks are explicitly laid out.
   */
   nir_foreach_variable_with_modes(var, b->shader, nir_var_mem_shared) {
      if (glsl_type_is_interface(var->type)) {
      assert(b->options->caps.workgroup_memory_explicit_layout);
   b->shader->info.shared_memory_explicit_layout = true;
         }
   if (b->shader->info.shared_memory_explicit_layout) {
      unsigned size = 0;
   nir_foreach_variable_with_modes(var, b->shader, nir_var_mem_shared) {
      assert(glsl_type_is_interface(var->type));
   const bool align_to_stride = false;
      }
               if (stage == MESA_SHADER_FRAGMENT) {
      /* From the Vulkan 1.2.199 spec:
   *
   *    "If a fragment shader entry point’s interface includes an input
   *    variable decorated with SamplePosition, Sample Shading is
   *    considered enabled with a minSampleShading value of 1.0."
   *
   * Similar text exists for SampleId.  Regarding the Sample decoration,
   * the Vulkan 1.2.199 spec says:
   *
   *    "If a fragment shader input is decorated with Sample, a separate
   *    value must be assigned to that variable for each covered sample in
   *    the fragment, and that value must be sampled at the location of
   *    the individual sample. When rasterizationSamples is
   *    VK_SAMPLE_COUNT_1_BIT, the fragment center must be used for
   *    Centroid, Sample, and undecorated attribute interpolation."
   *
   * Unfortunately, this isn't quite as clear about static use and the
   * interface but the static use check should be valid.
   *
   * For OpenGL, similar language exists but it's all more wishy-washy.
   * We'll assume the same behavior across APIs.
   */
   nir_foreach_variable_with_modes(var, b->shader,
                  struct nir_variable_data *members =
         uint16_t num_members = var->members ? var->num_members : 1;
   for (uint16_t i = 0; i < num_members; i++) {
      if (members[i].mode == nir_var_system_value &&
                        if (members[i].mode == nir_var_shader_in && members[i].sample)
                     /* Work around applications that declare shader_call_data variables inside
   * ray generation shaders.
   *
   * https://gitlab.freedesktop.org/mesa/mesa/-/issues/5326
   */
   if (stage == MESA_SHADER_RAYGEN)
      NIR_PASS(_, b->shader, nir_remove_dead_variables, nir_var_shader_call_data,
         /* Unparent the shader from the vtn_builder before we delete the builder */
            nir_shader *shader = b->shader;
               }
