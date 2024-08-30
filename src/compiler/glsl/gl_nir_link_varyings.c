   /*
   * Copyright © 2012 Intel Corporation
   * Copyright © 2021 Valve Corporation
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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      /**
   * Linker functions related specifically to linking varyings between shader
   * stages.
   */
      #include "main/errors.h"
   #include "main/macros.h"
   #include "main/menums.h"
   #include "main/mtypes.h"
   #include "program/symbol_table.h"
   #include "util/hash_table.h"
   #include "util/u_math.h"
   #include "util/perf/cpu_trace.h"
      #include "nir.h"
   #include "nir_builder.h"
   #include "nir_deref.h"
   #include "gl_nir.h"
   #include "gl_nir_link_varyings.h"
   #include "gl_nir_linker.h"
   #include "linker_util.h"
   #include "nir_gl_types.h"
   #include "string_to_uint_map.h"
      #define SAFE_MASK_FROM_INDEX(i) (((i) >= 32) ? ~0 : ((1 << (i)) - 1))
      /* Temporary storage for the set of attributes that need locations assigned. */
   struct temp_attr {
      unsigned slots;
      };
      /* Used below in the call to qsort. */
   static int
   compare_attr(const void *a, const void *b)
   {
      const struct temp_attr *const l = (const struct temp_attr *) a;
            /* Reversed because we want a descending order sort below. */
      }
      /**
   * Get the varying type stripped of the outermost array if we're processing
   * a stage whose varyings are arrays indexed by a vertex number (such as
   * geometry shader inputs).
   */
   static const struct glsl_type *
   get_varying_type(const nir_variable *var, gl_shader_stage stage)
   {
      const struct glsl_type *type = var->type;
   if (nir_is_arrayed_io(var, stage) || var->data.per_view) {
      assert(glsl_type_is_array(type));
                  }
      /**
   * Find a contiguous set of available bits in a bitmask.
   *
   * \param used_mask     Bits representing used (1) and unused (0) locations
   * \param needed_count  Number of contiguous bits needed.
   *
   * \return
   * Base location of the available bits on success or -1 on failure.
   */
   static int
   find_available_slots(unsigned used_mask, unsigned needed_count)
   {
      unsigned needed_mask = (1 << needed_count) - 1;
            /* The comparison to 32 is redundant, but without it GCC emits "warning:
   * cannot optimize possibly infinite loops" for the loop below.
   */
   if ((needed_count == 0) || (max_bit_to_test < 0) || (max_bit_to_test > 32))
            for (int i = 0; i <= max_bit_to_test; i++) {
      if ((needed_mask & ~used_mask) == needed_mask)
                           }
      /* Find deref based on variable name.
   * Note: This function does not support arrays.
   */
   static bool
   find_deref(nir_shader *shader, const char *name)
   {
      nir_foreach_function(func, shader) {
      nir_foreach_block(block, func->impl) {
      nir_foreach_instr(instr, block) {
      if (instr->type == nir_instr_type_deref) {
      nir_deref_instr *deref = nir_instr_as_deref(instr);
   if (deref->deref_type == nir_deref_type_var &&
      strcmp(deref->var->name, name) == 0)
                        }
      /**
   * Validate the types and qualifiers of an output from one stage against the
   * matching input to another stage.
   */
   static void
   cross_validate_types_and_qualifiers(const struct gl_constants *consts,
                                 {
      /* Check that the types match between stages.
   */
            /* VS -> GS, VS -> TCS, VS -> TES, TES -> GS */
   const bool extra_array_level = (producer_stage == MESA_SHADER_VERTEX &&
               if (extra_array_level) {
      assert(glsl_type_is_array(type_to_match));
               if (type_to_match != output->type) {
      if (glsl_type_is_struct(output->type)) {
      /* Structures across shader stages can have different name
   * and considered to match in type if and only if structure
   * members match in name, type, qualification, and declaration
   * order. The precision doesn’t need to match.
   */
   if (!glsl_record_compare(output->type, type_to_match,
                        linker_error(prog,
         "%s shader output `%s' declared as struct `%s', "
   "doesn't match in type with %s shader input "
   "declared as struct `%s'\n",
   _mesa_shader_stage_to_string(producer_stage),
   output->name,
   glsl_get_type_name(output->type),
         } else if (!glsl_type_is_array(output->type) ||
            /* There is a bit of a special case for gl_TexCoord.  This
   * built-in is unsized by default.  Applications that variable
   * access it must redeclare it with a size.  There is some
   * language in the GLSL spec that implies the fragment shader
   * and vertex shader do not have to agree on this size.  Other
   * driver behave this way, and one or two applications seem to
   * rely on it.
   *
   * Neither declaration needs to be modified here because the array
   * sizes are fixed later when update_array_sizes is called.
   *
   * From page 48 (page 54 of the PDF) of the GLSL 1.10 spec:
   *
   *     "Unlike user-defined varying variables, the built-in
   *     varying variables don't have a strict one-to-one
   *     correspondence between the vertex language and the
   *     fragment language."
   */
   linker_error(prog,
               "%s shader output `%s' declared as type `%s', "
   "but %s shader input declared as type `%s'\n",
   _mesa_shader_stage_to_string(producer_stage),
   output->name,
   glsl_get_type_name(output->type),
                  /* Check that all of the qualifiers match between stages.
            /* According to the OpenGL and OpenGLES GLSL specs, the centroid qualifier
   * should match until OpenGL 4.3 and OpenGLES 3.1. The OpenGLES 3.0
   * conformance test suite does not verify that the qualifiers must match.
   * The deqp test suite expects the opposite (OpenGLES 3.1) behavior for
   * OpenGLES 3.0 drivers, so we relax the checking in all cases.
   */
   if (false /* always skip the centroid check */ &&
      prog->GLSL_Version < (prog->IsES ? 310 : 430) &&
   input->data.centroid != output->data.centroid) {
   linker_error(prog,
               "%s shader output `%s' %s centroid qualifier, "
   "but %s shader input %s centroid qualifier\n",
   _mesa_shader_stage_to_string(producer_stage),
   output->name,
   (output->data.centroid) ? "has" : "lacks",
               if (input->data.sample != output->data.sample) {
      linker_error(prog,
               "%s shader output `%s' %s sample qualifier, "
   "but %s shader input %s sample qualifier\n",
   _mesa_shader_stage_to_string(producer_stage),
   output->name,
   (output->data.sample) ? "has" : "lacks",
               if (input->data.patch != output->data.patch) {
      linker_error(prog,
               "%s shader output `%s' %s patch qualifier, "
   "but %s shader input %s patch qualifier\n",
   _mesa_shader_stage_to_string(producer_stage),
   output->name,
   (output->data.patch) ? "has" : "lacks",
               /* The GLSL 4.20 and GLSL ES 3.00 specifications say:
   *
   *    "As only outputs need be declared with invariant, an output from
   *     one shader stage will still match an input of a subsequent stage
   *     without the input being declared as invariant."
   *
   * while GLSL 4.10 says:
   *
   *    "For variables leaving one shader and coming into another shader,
   *     the invariant keyword has to be used in both shaders, or a link
   *     error will result."
   *
   * and GLSL ES 1.00 section 4.6.4 "Invariance and Linking" says:
   *
   *    "The invariance of varyings that are declared in both the vertex
   *     and fragment shaders must match."
   */
   if (input->data.explicit_invariant != output->data.explicit_invariant &&
      prog->GLSL_Version < (prog->IsES ? 300 : 420)) {
   linker_error(prog,
               "%s shader output `%s' %s invariant qualifier, "
   "but %s shader input %s invariant qualifier\n",
   _mesa_shader_stage_to_string(producer_stage),
   output->name,
   (output->data.explicit_invariant) ? "has" : "lacks",
               /* GLSL >= 4.40 removes text requiring interpolation qualifiers
   * to match cross stage, they must only match within the same stage.
   *
   * From page 84 (page 90 of the PDF) of the GLSL 4.40 spec:
   *
   *     "It is a link-time error if, within the same stage, the interpolation
   *     qualifiers of variables of the same name do not match.
   *
   * Section 4.3.9 (Interpolation) of the GLSL ES 3.00 spec says:
   *
   *    "When no interpolation qualifier is present, smooth interpolation
   *    is used."
   *
   * So we match variables where one is smooth and the other has no explicit
   * qualifier.
   */
   unsigned input_interpolation = input->data.interpolation;
   unsigned output_interpolation = output->data.interpolation;
   if (prog->IsES) {
      if (input_interpolation == INTERP_MODE_NONE)
         if (output_interpolation == INTERP_MODE_NONE)
      }
   if (input_interpolation != output_interpolation &&
      prog->GLSL_Version < 440) {
   if (!consts->AllowGLSLCrossStageInterpolationMismatch) {
      linker_error(prog,
               "%s shader output `%s' specifies %s "
   "interpolation qualifier, "
   "but %s shader input specifies %s "
   "interpolation qualifier\n",
   _mesa_shader_stage_to_string(producer_stage),
   output->name,
   interpolation_string(output->data.interpolation),
      } else {
      linker_warning(prog,
                  "%s shader output `%s' specifies %s "
   "interpolation qualifier, "
   "but %s shader input specifies %s "
   "interpolation qualifier\n",
   _mesa_shader_stage_to_string(producer_stage),
   output->name,
         }
      /**
   * Validate front and back color outputs against single color input
   */
   static void
   cross_validate_front_and_back_color(const struct gl_constants *consts,
                                       {
      if (front_color != NULL && front_color->data.assigned)
      cross_validate_types_and_qualifiers(consts, prog, input, front_color,
         if (back_color != NULL && back_color->data.assigned)
      cross_validate_types_and_qualifiers(consts, prog, input, back_color,
   }
      static unsigned
   compute_variable_location_slot(nir_variable *var, gl_shader_stage stage)
   {
               switch (stage) {
      case MESA_SHADER_VERTEX:
      if (var->data.mode == nir_var_shader_in)
            case MESA_SHADER_TESS_CTRL:
   case MESA_SHADER_TESS_EVAL:
      if (var->data.patch)
            case MESA_SHADER_FRAGMENT:
      if (var->data.mode == nir_var_shader_out)
            default:
                  }
         struct explicit_location_info {
      nir_variable *var;
   bool base_type_is_integer;
   unsigned base_type_bit_size;
   unsigned interpolation;
   bool centroid;
   bool sample;
      };
      static bool
   check_location_aliasing(struct explicit_location_info explicit_locations[][4],
                           nir_variable *var,
   unsigned location,
   unsigned component,
   unsigned location_limit,
   const struct glsl_type *type,
   unsigned interpolation,
   bool centroid,
   {
      unsigned last_comp;
   unsigned base_type_bit_size;
   const struct glsl_type *type_without_array = glsl_without_array(type);
   const bool base_type_is_integer =
         const bool is_struct = glsl_type_is_struct(type_without_array);
   if (is_struct) {
      /* structs don't have a defined underlying base type so just treat all
   * component slots as used and set the bit size to 0. If there is
   * location aliasing, we'll fail anyway later.
   */
   last_comp = 4;
      } else {
      unsigned dmul = glsl_type_is_64bit(type_without_array) ? 2 : 1;
   last_comp = component + glsl_get_vector_elements(type_without_array) * dmul;
   base_type_bit_size =
               while (location < location_limit) {
      unsigned comp = 0;
   while (comp < 4) {
                     if (info->var) {
      if (glsl_type_is_struct(glsl_without_array(info->var->type)) ||
      is_struct) {
   /* Structs cannot share location since they are incompatible
   * with any other underlying numerical type.
   */
   linker_error(prog,
               "%s shader has multiple %sputs sharing the "
   "same location that don't have the same "
   "underlying numerical type. Struct variable '%s', "
   "location %u\n",
   _mesa_shader_stage_to_string(stage),
   var->data.mode == nir_var_shader_in ? "in" : "out",
      } else if (comp >= component && comp < last_comp) {
      /* Component aliasing is not allowed */
   linker_error(prog,
               "%s shader has multiple %sputs explicitly "
   "assigned to location %d and component %d\n",
   _mesa_shader_stage_to_string(stage),
      } else {
      /* From the OpenGL 4.60.5 spec, section 4.4.1 Input Layout
   * Qualifiers, Page 67, (Location aliasing):
   *
   *   " Further, when location aliasing, the aliases sharing the
   *     location must have the same underlying numerical type
   *     and bit width (floating-point or integer, 32-bit versus
                        /* If the underlying numerical type isn't integer, implicitly
   * it will be float or else we would have failed by now.
   */
   if (info->base_type_is_integer != base_type_is_integer) {
      linker_error(prog,
               "%s shader has multiple %sputs sharing the "
   "same location that don't have the same "
   "underlying numerical type. Location %u "
   "component %u.\n",
                     if (info->base_type_bit_size != base_type_bit_size) {
      linker_error(prog,
               "%s shader has multiple %sputs sharing the "
   "same location that don't have the same "
   "underlying numerical bit size. Location %u "
   "component %u.\n",
                     if (info->interpolation != interpolation) {
      linker_error(prog,
               "%s shader has multiple %sputs sharing the "
   "same location that don't have the same "
   "interpolation qualification. Location %u "
   "component %u.\n",
                     if (info->centroid != centroid ||
      info->sample != sample ||
   info->patch != patch) {
   linker_error(prog,
               "%s shader has multiple %sputs sharing the "
   "same location that don't have the same "
   "auxiliary storage qualification. Location %u "
   "component %u.\n",
   _mesa_shader_stage_to_string(stage),
            } else if (comp >= component && comp < last_comp) {
      info->var = var;
   info->base_type_is_integer = base_type_is_integer;
   info->base_type_bit_size = base_type_bit_size;
   info->interpolation = interpolation;
   info->centroid = centroid;
   info->sample = sample;
                        /* We need to do some special handling for doubles as dvec3 and
   * dvec4 consume two consecutive locations. We don't need to
   * worry about components beginning at anything other than 0 as
   * the spec does not allow this for dvec3 and dvec4.
   */
   if (comp == 4 && last_comp > 4) {
      last_comp = last_comp - 4;
   /* Bump location index and reset the component index */
   location++;
   comp = 0;
                                 }
      static bool
   validate_explicit_variable_location(const struct gl_constants *consts,
                           {
      const struct glsl_type *type = get_varying_type(var, sh->Stage);
   unsigned num_elements = glsl_count_attribute_slots(type, false);
   unsigned idx = compute_variable_location_slot(var, sh->Stage);
            /* Vertex shader inputs and fragment shader outputs are validated in
   * assign_attribute_or_color_locations() so we should not attempt to
   * validate them again here.
   */
   unsigned slot_max;
   if (var->data.mode == nir_var_shader_out) {
      assert(sh->Stage != MESA_SHADER_FRAGMENT);
      } else {
      assert(var->data.mode == nir_var_shader_in);
   assert(sh->Stage != MESA_SHADER_VERTEX);
               if (slot_limit > slot_max) {
      linker_error(prog,
                           const struct glsl_type *type_without_array = glsl_without_array(type);
   if (glsl_type_is_interface(type_without_array)) {
      for (unsigned i = 0; i < glsl_get_length(type_without_array); i++) {
      const struct glsl_struct_field *field =
         unsigned field_location = field->location -
         unsigned field_slots = glsl_count_attribute_slots(field->type, false);
   if (!check_location_aliasing(explicit_locations, var,
                              field_location,
   0,
   field_location + field_slots,
   field->type,
   field->interpolation,
            } else if (!check_location_aliasing(explicit_locations, var,
                                       idx, var->data.location_frac,
                     }
      /**
   * Validate explicit locations for the inputs to the first stage and the
   * outputs of the last stage in a program, if those are not the VS and FS
   * shaders.
   */
   void
   gl_nir_validate_first_and_last_interface_explicit_locations(const struct gl_constants *consts,
                     {
      /* VS inputs and FS outputs are validated in
   * assign_attribute_or_color_locations()
   */
   bool validate_first_stage = first_stage != MESA_SHADER_VERTEX;
   bool validate_last_stage = last_stage != MESA_SHADER_FRAGMENT;
   if (!validate_first_stage && !validate_last_stage)
                     gl_shader_stage stages[2] = { first_stage, last_stage };
   bool validate_stage[2] = { validate_first_stage, validate_last_stage };
            for (unsigned i = 0; i < 2; i++) {
      if (!validate_stage[i])
                     struct gl_linked_shader *sh = prog->_LinkedShaders[stage];
                     nir_foreach_variable_with_modes(var, sh->Program->nir, var_mode[i]) {
      if (!var->data.explicit_location ||
                  if (!validate_explicit_variable_location(consts, explicit_locations,
                        }
      /**
   * Check if we should force input / output matching between shader
   * interfaces.
   *
   * Section 4.3.4 (Inputs) of the GLSL 4.10 specifications say:
   *
   *   "Only the input variables that are actually read need to be
   *    written by the previous stage; it is allowed to have
   *    superfluous declarations of input variables."
   *
   * However it's not defined anywhere as to how we should handle
   * inputs that are not written in the previous stage and it's not
   * clear what "actually read" means.
   *
   * The GLSL 4.20 spec however is much clearer:
   *
   *    "Only the input variables that are statically read need to
   *     be written by the previous stage; it is allowed to have
   *     superfluous declarations of input variables."
   *
   * It also has a table that states it is an error to statically
   * read an input that is not defined in the previous stage. While
   * it is not an error to not statically write to the output (it
   * just needs to be defined to not be an error).
   *
   * The text in the GLSL 4.20 spec was an attempt to clarify the
   * previous spec iterations. However given the difference in spec
   * and that some applications seem to depend on not erroring when
   * the input is not actually read in control flow we only apply
   * this rule to GLSL 4.20 and higher. GLSL 4.10 shaders have been
   * seen in the wild that depend on the less strict interpretation.
   */
   static bool
   static_input_output_matching(struct gl_shader_program *prog)
   {
         }
      /**
   * Validate that outputs from one stage match inputs of another
   */
   void
   gl_nir_cross_validate_outputs_to_inputs(const struct gl_constants *consts,
                     {
      struct _mesa_symbol_table *table = _mesa_symbol_table_ctor();
   struct explicit_location_info output_explicit_locations[MAX_VARYING][4] = {0};
            /* Find all shader outputs in the "producer" stage.
   */
   nir_foreach_variable_with_modes(var, producer->Program->nir, nir_var_shader_out) {
      if (!var->data.explicit_location
      || var->data.location < VARYING_SLOT_VAR0) {
   /* Interface block validation is handled elsewhere */
               } else {
      /* User-defined varyings with explicit locations are handled
   * differently because they do not need to have matching names.
   */
   if (!validate_explicit_variable_location(consts,
                                    /* Find all shader inputs in the "consumer" stage.  Any variables that have
   * matching outputs already in the symbol table must have the same type and
   * qualifiers.
   *
   * Exception: if the consumer is the geometry shader, then the inputs
   * should be arrays and the type of the array element should match the type
   * of the corresponding producer output.
   */
   nir_foreach_variable_with_modes(input, consumer->Program->nir, nir_var_shader_in) {
      if (strcmp(input->name, "gl_Color") == 0 && input->data.used) {
                                    cross_validate_front_and_back_color(consts, prog, input,
            } else if (strcmp(input->name, "gl_SecondaryColor") == 0 && input->data.used) {
                                    cross_validate_front_and_back_color(consts, prog, input,
            } else {
      /* The rules for connecting inputs and outputs change in the presence
   * of explicit locations.  In this case, we no longer care about the
   * names of the variables.  Instead, we care only about the
   * explicitly assigned location.
   */
   nir_variable *output = NULL;
                     const struct glsl_type *type =
         unsigned num_elements = glsl_count_attribute_slots(type, false);
   unsigned idx =
                  if (!validate_explicit_variable_location(consts,
                              while (idx < slot_limit) {
      if (idx >= MAX_VARYING) {
      linker_error(prog,
                                    if (output == NULL) {
      /* A linker failure should only happen when there is no
   * output declaration and there is Static Use of the
   * declared input.
   */
   if (input->data.used && static_input_output_matching(prog)) {
      linker_error(prog,
               "%s shader input `%s' with explicit location "
   "has no matching output\n",
         } else if (input->data.location != output->data.location) {
      linker_error(prog,
               "%s shader input `%s' with explicit location "
   "has no matching output\n",
      }
         } else {
      /* Interface block validation is handled elsewhere */
                  output = (nir_variable *)
               if (output != NULL) {
      /* Interface blocks have their own validation elsewhere so don't
   * try validating them here.
   */
   if (!(input->interface_type && output->interface_type))
      cross_validate_types_and_qualifiers(consts, prog, input, output,
         } else {
      /* Check for input vars with unmatched output vars in prev stage
   * taking into account that interface blocks could have a matching
   * output but with different name, so we ignore them.
   */
   assert(!input->data.assigned);
   if (input->data.used && !input->interface_type &&
      !input->data.explicit_location &&
   static_input_output_matching(prog))
   linker_error(prog,
               "%s shader input `%s' "
               out:
         }
      /**
   * Assign locations for either VS inputs or FS outputs.
   *
   * \param mem_ctx        Temporary ralloc context used for linking.
   * \param prog           Shader program whose variables need locations
   *                       assigned.
   * \param constants      Driver specific constant values for the program.
   * \param target_index   Selector for the program target to receive location
   *                       assignmnets.  Must be either \c MESA_SHADER_VERTEX or
   *                       \c MESA_SHADER_FRAGMENT.
   * \param do_assignment  Whether we are actually marking the assignment or we
   *                       are just doing a dry-run checking.
   *
   * \return
   * If locations are (or can be, in case of dry-running) successfully assigned,
   * true is returned.  Otherwise an error is emitted to the shader link log and
   * false is returned.
   */
   static bool
   assign_attribute_or_color_locations(void *mem_ctx,
                           {
      /* Maximum number of generic locations.  This corresponds to either the
   * maximum number of draw buffers or the maximum number of generic
   * attributes.
   */
   unsigned max_index = (target_index == MESA_SHADER_VERTEX) ?
      constants->Program[target_index].MaxAttribs :
         assert(max_index <= 32);
            /* Mark invalid locations as being used.
   */
   unsigned used_locations = ~SAFE_MASK_FROM_INDEX(max_index);
            assert((target_index == MESA_SHADER_VERTEX)
            if (prog->_LinkedShaders[target_index] == NULL)
            /* Operate in a total of four passes.
   *
   * 1. Invalidate the location assignments for all vertex shader inputs.
   *
   * 2. Assign locations for inputs that have user-defined (via
   *    glBindVertexAttribLocation) locations and outputs that have
   *    user-defined locations (via glBindFragDataLocation).
   *
   * 3. Sort the attributes without assigned locations by number of slots
   *    required in decreasing order.  Fragmentation caused by attribute
   *    locations assigned by the application may prevent large attributes
   *    from having enough contiguous space.
   *
   * 4. Assign locations to any inputs without assigned locations.
            const int generic_base = (target_index == MESA_SHADER_VERTEX)
            nir_variable_mode io_mode =
      (target_index == MESA_SHADER_VERTEX)
         /* Temporary array for the set of attributes that have locations assigned,
   * for the purpose of checking overlapping slots/components of (non-ES)
   * fragment shader outputs.
   */
   nir_variable *assigned[FRAG_RESULT_MAX * 4]; /* (max # of FS outputs) * # components */
                     nir_shader *shader = prog->_LinkedShaders[target_index]->Program->nir;
               if (var->data.explicit_location) {
      if ((var->data.location >= (int)(max_index + generic_base))
      || (var->data.location < 0)) {
   linker_error(prog,
               "invalid explicit location %d specified for `%s'\n",
   (var->data.location < 0)
   ? var->data.location
         } else if (target_index == MESA_SHADER_VERTEX) {
               if (string_to_uint_map_get(prog->AttributeBindings, &binding, var->name)) {
      assert(binding >= VERT_ATTRIB_GENERIC0);
         } else if (target_index == MESA_SHADER_FRAGMENT) {
      unsigned binding;
   unsigned index;
                  while (type) {
      /* Check if there's a binding for the variable name */
   if (string_to_uint_map_get(prog->FragDataBindings, &binding, name)) {
                     if (string_to_uint_map_get(prog->FragDataIndexBindings, &index, name)) {
                           /* If not, but it's an array type, look for name[0] */
   if (glsl_type_is_array(type)) {
      name = ralloc_asprintf(mem_ctx, "%s[0]", name);
                                    if (strcmp(var->name, "gl_LastFragData") == 0)
            /* From GL4.5 core spec, section 15.2 (Shader Execution):
   *
   *     "Output binding assignments will cause LinkProgram to fail:
   *     ...
   *     If the program has an active output assigned to a location greater
   *     than or equal to the value of MAX_DUAL_SOURCE_DRAW_BUFFERS and has
   *     an active output assigned an index greater than or equal to one;"
   */
   if (target_index == MESA_SHADER_FRAGMENT && var->data.index >= 1 &&
      var->data.location - generic_base >=
   (int) constants->MaxDualSourceDrawBuffers) {
   linker_error(prog,
               "output location %d >= GL_MAX_DUAL_SOURCE_DRAW_BUFFERS "
   "with index %u for %s\n",
               const unsigned slots =
                  /* If the variable is not a built-in and has a location statically
   * assigned in the shader (presumably via a layout qualifier), make sure
   * that it doesn't collide with other assigned locations.  Otherwise,
   * add it to the list of variables that need linker-assigned locations.
   */
   if (var->data.location != -1) {
      if (var->data.location >= generic_base && var->data.index < 1) {
      /* From page 61 of the OpenGL 4.0 spec:
   *
   *     "LinkProgram will fail if the attribute bindings assigned
   *     by BindAttribLocation do not leave not enough space to
   *     assign a location for an active matrix attribute or an
   *     active attribute array, both of which require multiple
   *     contiguous generic attributes."
   *
   * I think above text prohibits the aliasing of explicit and
   * automatic assignments. But, aliasing is allowed in manual
   * assignments of attribute locations. See below comments for
   * the details.
   *
   * From OpenGL 4.0 spec, page 61:
   *
   *     "It is possible for an application to bind more than one
   *     attribute name to the same location. This is referred to as
   *     aliasing. This will only work if only one of the aliased
   *     attributes is active in the executable program, or if no
   *     path through the shader consumes more than one attribute of
   *     a set of attributes aliased to the same location. A link
   *     error can occur if the linker determines that every path
   *     through the shader consumes multiple aliased attributes,
   *     but implementations are not required to generate an error
   *     in this case."
   *
   * From GLSL 4.30 spec, page 54:
   *
   *    "A program will fail to link if any two non-vertex shader
   *     input variables are assigned to the same location. For
   *     vertex shaders, multiple input variables may be assigned
   *     to the same location using either layout qualifiers or via
   *     the OpenGL API. However, such aliasing is intended only to
   *     support vertex shaders where each execution path accesses
   *     at most one input per each location. Implementations are
   *     permitted, but not required, to generate link-time errors
   *     if they detect that every path through the vertex shader
   *     executable accesses multiple inputs assigned to any single
   *     location. For all shader types, a program will fail to link
   *     if explicit location assignments leave the linker unable
   *     to find space for other variables without explicit
   *     assignments."
   *
   * From OpenGL ES 3.0 spec, page 56:
   *
   *    "Binding more than one attribute name to the same location
   *     is referred to as aliasing, and is not permitted in OpenGL
   *     ES Shading Language 3.00 vertex shaders. LinkProgram will
   *     fail when this condition exists. However, aliasing is
   *     possible in OpenGL ES Shading Language 1.00 vertex shaders.
   *     This will only work if only one of the aliased attributes
   *     is active in the executable program, or if no path through
   *     the shader consumes more than one attribute of a set of
   *     attributes aliased to the same location. A link error can
   *     occur if the linker determines that every path through the
   *     shader consumes multiple aliased attributes, but implemen-
   *     tations are not required to generate an error in this case."
   *
   * After looking at above references from OpenGL, OpenGL ES and
   * GLSL specifications, we allow aliasing of vertex input variables
   * in: OpenGL 2.0 (and above) and OpenGL ES 2.0.
   *
   * NOTE: This is not required by the spec but its worth mentioning
   * here that we're not doing anything to make sure that no path
   * through the vertex shader executable accesses multiple inputs
                  /* Mask representing the contiguous slots that will be used by
   * this attribute.
   */
   const unsigned attr = var->data.location - generic_base;
   const unsigned use_mask = (1 << slots) - 1;
                  /* Generate a link error if the requested locations for this
   * attribute exceed the maximum allowed attribute location.
   */
   if (attr + slots > max_index) {
      linker_error(prog,
                                 /* Generate a link error if the set of bits requested for this
   * attribute overlaps any previously allocated bits.
   */
   if ((~(use_mask << attr) & used_locations) != used_locations) {
      if (target_index == MESA_SHADER_FRAGMENT && !prog->IsES) {
      /* From section 4.4.2 (Output Layout Qualifiers) of the GLSL
   * 4.40 spec:
   *
   *    "Additionally, for fragment shader outputs, if two
   *    variables are placed within the same location, they
   *    must have the same underlying type (floating-point or
   *    integer). No component aliasing of output variables or
   *    members is allowed.
   */
   for (unsigned i = 0; i < assigned_attr; i++) {
      unsigned assigned_slots =
                                                const struct glsl_type *assigned_type =
         const struct glsl_type *type =
         if (glsl_get_base_type(assigned_type) !=
      glsl_get_base_type(type)) {
                                 unsigned assigned_component_mask =
      ((1 << glsl_get_vector_elements(assigned_type)) - 1) <<
      unsigned component_mask =
      ((1 << glsl_get_vector_elements(type)) - 1) <<
      if (assigned_component_mask & component_mask) {
      linker_error(prog, "overlapping component is "
               "assigned to %ss %s and %s "
   "(component=%d)\n",
               } else if (target_index == MESA_SHADER_FRAGMENT ||
            linker_error(prog, "overlapping location is assigned "
                  } else {
      linker_warning(prog, "overlapping location is assigned "
                        if (target_index == MESA_SHADER_FRAGMENT && !prog->IsES) {
      /* Only track assigned variables for non-ES fragment shaders
   * to avoid overflowing the array.
   *
   * At most one variable per fragment output component should
   * reach this.
   */
   assert(assigned_attr < ARRAY_SIZE(assigned));
                              /* From the GL 4.5 core spec, section 11.1.1 (Vertex Attributes):
   *
   * "A program with more than the value of MAX_VERTEX_ATTRIBS
   *  active attribute variables may fail to link, unless
   *  device-dependent optimizations are able to make the program
   *  fit within available hardware resources. For the purposes
   *  of this test, attribute variables of the type dvec3, dvec4,
   *  dmat2x3, dmat2x4, dmat3, dmat3x4, dmat4x3, and dmat4 may
   *  count as consuming twice as many attributes as equivalent
   *  single-precision types. While these types use the same number
   *  of generic attributes as their single-precision equivalents,
   *  implementations are permitted to consume two single-precision
   *  vectors of internal storage for each three- or four-component
   *  double-precision vector."
   *
   * Mark this attribute slot as taking up twice as much space
   * so we can count it properly against limits.  According to
   * issue (3) of the GL_ARB_vertex_attrib_64bit behavior, this
   * is optional behavior, but it seems preferable.
   */
   if (glsl_type_is_dual_slot(glsl_without_array(var->type)))
                           if (num_attr >= max_index) {
      linker_error(prog, "too many %s (max %u)",
               target_index == MESA_SHADER_VERTEX ?
      }
   to_assign[num_attr].slots = slots;
   to_assign[num_attr].var = var;
               if (!do_assignment)
            if (target_index == MESA_SHADER_VERTEX) {
      unsigned total_attribs_size =
      util_bitcount(used_locations & SAFE_MASK_FROM_INDEX(max_index)) +
      if (total_attribs_size > max_index) {
      linker_error(prog,
                              /* If all of the attributes were assigned locations by the application (or
   * are built-in attributes with fixed locations), return early.  This should
   * be the common case.
   */
   if (num_attr == 0)
                     if (target_index == MESA_SHADER_VERTEX) {
      /* VERT_ATTRIB_GENERIC0 is a pseudo-alias for VERT_ATTRIB_POS.  It can
   * only be explicitly assigned by via glBindAttribLocation.  Mark it as
   * reserved to prevent it from being automatically allocated below.
   */
   if (find_deref(shader, "gl_Vertex"))
               for (unsigned i = 0; i < num_attr; i++) {
      /* Mask representing the contiguous slots that will be used by this
   * attribute.
   */
                     if (location < 0) {
                     linker_error(prog,
               "insufficient contiguous locations "
               to_assign[i].var->data.location = generic_base + location;
            if (glsl_type_is_dual_slot(glsl_without_array(to_assign[i].var->type)))
               /* Now that we have all the locations, from the GL 4.5 core spec, section
   * 11.1.1 (Vertex Attributes), dvec3, dvec4, dmat2x3, dmat2x4, dmat3,
   * dmat3x4, dmat4x3, and dmat4 count as consuming twice as many attributes
   * as equivalent single-precision types.
   */
   if (target_index == MESA_SHADER_VERTEX) {
      unsigned total_attribs_size =
      util_bitcount(used_locations & SAFE_MASK_FROM_INDEX(max_index)) +
      if (total_attribs_size > max_index) {
      linker_error(prog,
                                 }
      static bool
   varying_has_user_specified_location(const nir_variable *var)
   {
      return var->data.explicit_location &&
      }
      static void
   create_xfb_varying_names(void *mem_ctx, const struct glsl_type *t, char **name,
                           {
      if (glsl_type_is_interface(t)) {
               assert(ifc_member_name && ifc_member_t);
            create_xfb_varying_names(mem_ctx, ifc_member_t, name, new_length, count,
      } else if (glsl_type_is_struct(t)) {
      for (unsigned i = 0; i < glsl_get_length(t); i++) {
                              create_xfb_varying_names(mem_ctx, glsl_get_struct_field(t, i), name,
               } else if (glsl_type_is_struct(glsl_without_array(t)) ||
            glsl_type_is_interface(glsl_without_array(t)) ||
   for (unsigned i = 0; i < glsl_get_length(t); i++) {
                              create_xfb_varying_names(mem_ctx, glsl_get_array_element(t), name,
               } else {
            }
      static bool
   process_xfb_layout_qualifiers(void *mem_ctx, const struct gl_linked_shader *sh,
                     {
               /* We still need to enable transform feedback mode even if xfb_stride is
   * only applied to a global out. Also we don't bother to propagate
   * xfb_stride to interface block members so this will catch that case also.
   */
   for (unsigned j = 0; j < MAX_FEEDBACK_BUFFERS; j++) {
      if (prog->TransformFeedback.BufferStride[j]) {
      has_xfb_qualifiers = true;
                  nir_foreach_shader_out_variable(var, sh->Program->nir) {
      /* From the ARB_enhanced_layouts spec:
   *
   *    "Any shader making any static use (after preprocessing) of any of
   *     these *xfb_* qualifiers will cause the shader to be in a
   *     transform feedback capturing mode and hence responsible for
   *     describing the transform feedback setup.  This mode will capture
   *     any output selected by *xfb_offset*, directly or indirectly, to
   *     a transform feedback buffer."
   */
   if (var->data.explicit_xfb_buffer || var->data.explicit_xfb_stride) {
                  if (var->data.explicit_offset) {
      *num_xfb_decls += glsl_varying_count(var->type);
                  if (*num_xfb_decls == 0)
            unsigned i = 0;
   *varying_names = ralloc_array(mem_ctx, char *, *num_xfb_decls);
   nir_foreach_shader_out_variable(var, sh->Program->nir) {
      if (var->data.explicit_offset) {
                                       /* Find the member type before it was altered by lowering */
   const struct glsl_type *type_wa = glsl_without_array(type);
   member_type =
            } else {
      type = var->type;
   member_type = NULL;
      }
   create_xfb_varying_names(mem_ctx, type, &name, strlen(name), &i,
                        assert(i == *num_xfb_decls);
      }
      /**
   * Initialize this struct based on a string that was passed to
   * glTransformFeedbackVaryings.
   *
   * If the input is mal-formed, this call still succeeds, but it sets
   * this->var_name to a mal-formed input, so xfb_decl_find_output_var()
   * will fail to find any matching variable.
   */
   static void
   xfb_decl_init(struct xfb_decl *xfb_decl, const struct gl_constants *consts,
               {
      /* We don't have to be pedantic about what is a valid GLSL variable name,
   * because any variable with an invalid name can't exist in the IR anyway.
   */
   xfb_decl->location = -1;
   xfb_decl->orig_name = input;
   xfb_decl->lowered_builtin_array_variable = none;
   xfb_decl->skip_components = 0;
   xfb_decl->next_buffer_separator = false;
   xfb_decl->matched_candidate = NULL;
   xfb_decl->stream_id = 0;
   xfb_decl->buffer = 0;
            if (exts->ARB_transform_feedback3) {
      /* Parse gl_NextBuffer. */
   if (strcmp(input, "gl_NextBuffer") == 0) {
      xfb_decl->next_buffer_separator = true;
               /* Parse gl_SkipComponents. */
   if (strcmp(input, "gl_SkipComponents1") == 0)
         else if (strcmp(input, "gl_SkipComponents2") == 0)
         else if (strcmp(input, "gl_SkipComponents3") == 0)
         else if (strcmp(input, "gl_SkipComponents4") == 0)
            if (xfb_decl->skip_components)
               /* Parse a declaration. */
   const char *base_name_end;
   long subscript = link_util_parse_program_resource_name(input, strlen(input),
         xfb_decl->var_name = ralloc_strndup(mem_ctx, input, base_name_end - input);
   if (xfb_decl->var_name == NULL) {
      _mesa_error_no_memory(__func__);
               if (subscript >= 0) {
      xfb_decl->array_subscript = subscript;
      } else {
                  /* For drivers that lower gl_ClipDistance to gl_ClipDistanceMESA, this
   * class must behave specially to account for the fact that gl_ClipDistance
   * is converted from a float[8] to a vec4[2].
   */
   if (consts->ShaderCompilerOptions[MESA_SHADER_VERTEX].LowerCombinedClipCullDistance &&
      strcmp(xfb_decl->var_name, "gl_ClipDistance") == 0) {
      }
   if (consts->ShaderCompilerOptions[MESA_SHADER_VERTEX].LowerCombinedClipCullDistance &&
      strcmp(xfb_decl->var_name, "gl_CullDistance") == 0) {
         }
      /**
   * Determine whether two xfb_decl structs refer to the same variable and
   * array index (if applicable).
   */
   static bool
   xfb_decl_is_same(const struct xfb_decl *x, const struct xfb_decl *y)
   {
               if (strcmp(x->var_name, y->var_name) != 0)
         if (x->is_subscripted != y->is_subscripted)
         if (x->is_subscripted && x->array_subscript != y->array_subscript)
            }
      /**
   * The total number of varying components taken up by this variable.  Only
   * valid if assign_location() has been called.
   */
   static unsigned
   xfb_decl_num_components(struct xfb_decl *xfb_decl)
   {
      if (xfb_decl->lowered_builtin_array_variable)
         else
      return xfb_decl->vector_elements * xfb_decl->matrix_columns *
   }
      /**
   * Assign a location and stream ID for this xfb_decl object based on the
   * transform feedback candidate found by find_candidate.
   *
   * If an error occurs, the error is reported through linker_error() and false
   * is returned.
   */
   static bool
   xfb_decl_assign_location(struct xfb_decl *xfb_decl,
                     {
               unsigned fine_location
      = xfb_decl->matched_candidate->toplevel_var->data.location * 4
   + xfb_decl->matched_candidate->toplevel_var->data.location_frac
      const unsigned dmul =
            if (glsl_type_is_array(xfb_decl->matched_candidate->type)) {
      /* Array variable */
   const struct glsl_type *element_type =
         const unsigned matrix_cols = glsl_get_matrix_columns(element_type);
   const unsigned vector_elements = glsl_get_vector_elements(element_type);
   unsigned actual_array_size;
   switch (xfb_decl->lowered_builtin_array_variable) {
   case clip_distance:
      actual_array_size = prog->last_vert_prog ?
            case cull_distance:
      actual_array_size = prog->last_vert_prog ?
            case none:
   default:
      actual_array_size = glsl_array_size(xfb_decl->matched_candidate->type);
               if (xfb_decl->is_subscripted) {
      /* Check array bounds. */
   if (xfb_decl->array_subscript >= actual_array_size) {
      linker_error(prog, "Transform feedback varying %s has index "
               "%i, but the array size is %u.",
               bool array_will_be_lowered =
      lower_packed_varying_needs_lowering(prog->last_vert_prog->nir,
                           strcmp(xfb_decl->matched_candidate->toplevel_var->name, "gl_ClipDistance") == 0 ||
   strcmp(xfb_decl->matched_candidate->toplevel_var->name, "gl_CullDistance") == 0 ||
               unsigned array_elem_size = xfb_decl->lowered_builtin_array_variable ?
         fine_location += array_elem_size * xfb_decl->array_subscript;
      } else {
         }
   xfb_decl->vector_elements = vector_elements;
   xfb_decl->matrix_columns = matrix_cols;
   if (xfb_decl->lowered_builtin_array_variable)
         else
      } else {
      /* Regular variable (scalar, vector, or matrix) */
   if (xfb_decl->is_subscripted) {
      linker_error(prog, "Transform feedback varying %s requested, "
                  }
   xfb_decl->size = 1;
   xfb_decl->vector_elements = glsl_get_vector_elements(xfb_decl->matched_candidate->type);
   xfb_decl->matrix_columns = glsl_get_matrix_columns(xfb_decl->matched_candidate->type);
      }
   xfb_decl->location = fine_location / 4;
            /* From GL_EXT_transform_feedback:
   *   A program will fail to link if:
   *
   *   * the total number of components to capture in any varying
   *     variable in <varyings> is greater than the constant
   *     MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS_EXT and the
   *     buffer mode is SEPARATE_ATTRIBS_EXT;
   */
   if (prog->TransformFeedback.BufferMode == GL_SEPARATE_ATTRIBS &&
      xfb_decl_num_components(xfb_decl) >
   consts->MaxTransformFeedbackSeparateComponents) {
   linker_error(prog, "Transform feedback varying %s exceeds "
                           /* Only transform feedback varyings can be assigned to non-zero streams,
   * so assign the stream id here.
   */
            unsigned array_offset = xfb_decl->array_subscript * 4 * dmul;
   unsigned struct_offset = xfb_decl->matched_candidate->xfb_offset_floats * 4;
   xfb_decl->buffer = xfb_decl->matched_candidate->toplevel_var->data.xfb.buffer;
   xfb_decl->offset = xfb_decl->matched_candidate->toplevel_var->data.offset +
               }
      static unsigned
   xfb_decl_get_num_outputs(struct xfb_decl *xfb_decl)
   {
      if (!xfb_decl_is_varying(xfb_decl)) {
                  if (varying_has_user_specified_location(xfb_decl->matched_candidate->toplevel_var)) {
      unsigned dmul = _mesa_gl_datatype_is_64bit(xfb_decl->type) ? 2 : 1;
   unsigned rows_per_element = DIV_ROUND_UP(xfb_decl->vector_elements * dmul, 4);
      } else {
            }
      static bool
   xfb_decl_is_varying_written(struct xfb_decl *xfb_decl)
   {
      if (xfb_decl->next_buffer_separator || xfb_decl->skip_components)
               }
      /**
   * Update gl_transform_feedback_info to reflect this xfb_decl.
   *
   * If an error occurs, the error is reported through linker_error() and false
   * is returned.
   */
   static bool
   xfb_decl_store(struct xfb_decl *xfb_decl, const struct gl_constants *consts,
                  struct gl_shader_program *prog,
   struct gl_transform_feedback_info *info,
   unsigned buffer, unsigned buffer_index,
   const unsigned max_outputs,
      {
      unsigned xfb_offset = 0;
   unsigned size = xfb_decl->size;
   /* Handle gl_SkipComponents. */
   if (xfb_decl->skip_components) {
      info->Buffers[buffer].Stride += xfb_decl->skip_components;
   size = xfb_decl->skip_components;
               if (xfb_decl->next_buffer_separator) {
      size = 0;
               if (has_xfb_qualifiers) {
         } else {
         }
            {
      unsigned location = xfb_decl->location;
   unsigned location_frac = xfb_decl->location_frac;
            /* From GL_EXT_transform_feedback:
   *
   *   " A program will fail to link if:
   *
   *       * the total number of components to capture is greater than the
   *         constant MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS_EXT
   *         and the buffer mode is INTERLEAVED_ATTRIBS_EXT."
   *
   * From GL_ARB_enhanced_layouts:
   *
   *   " The resulting stride (implicit or explicit) must be less than or
   *     equal to the implementation-dependent constant
   *     gl_MaxTransformFeedbackInterleavedComponents."
   */
   if ((prog->TransformFeedback.BufferMode == GL_INTERLEAVED_ATTRIBS ||
      has_xfb_qualifiers) &&
   xfb_offset + num_components >
   consts->MaxTransformFeedbackInterleavedComponents) {
   linker_error(prog,
                           /* From the OpenGL 4.60.5 spec, section 4.4.2. Output Layout Qualifiers,
   * Page 76, (Transform Feedback Layout Qualifiers):
   *
   *   " No aliasing in output buffers is allowed: It is a compile-time or
   *     link-time error to specify variables with overlapping transform
   *     feedback offsets."
   */
   const unsigned max_components =
         const unsigned first_component = xfb_offset;
   const unsigned last_component = xfb_offset + num_components - 1;
   const unsigned start_word = BITSET_BITWORD(first_component);
   const unsigned end_word = BITSET_BITWORD(last_component);
   BITSET_WORD *used;
            if (!used_components[buffer]) {
      used_components[buffer] =
      }
            for (unsigned word = start_word; word <= end_word; word++) {
                                                   if (used[word] & BITSET_RANGE(start_range, end_range)) {
      linker_error(prog,
                  }
               const unsigned type_num_components =
      xfb_decl->vector_elements *
               while (num_components > 0) {
               /*  From GL_ARB_enhanced_layouts:
   *
   * "When an attribute variable declared using an array type is bound to
   * generic attribute index <i>, the active array elements are assigned to
   * consecutive generic attributes beginning with generic attribute <i>.  The
   * number of attributes and components assigned to each element are
   * determined according to the data type of array elements and "component"
   * layout qualifier (if any) specified in the declaration of the array."
   *
   * "When an attribute variable declared using a matrix type is bound to a
   * generic attribute index <i>, its values are taken from consecutive generic
   * attributes beginning with generic attribute <i>.  Such matrices are
   * treated as an array of column vectors with values taken from the generic
   * attributes.
   * This means there may be gaps in the varyings we are taking values from."
   *
   * Examples:
   *
   * | layout(location=0) dvec3[2] a; | layout(location=4) vec2[4] b; |
   * |                                |                               |
   * |        32b 32b 32b 32b         |        32b 32b 32b 32b        |
   * |      0  X   X   Y   Y          |      4  X   Y   0   0         |
   * |      1  Z   Z   0   0          |      5  X   Y   0   0         |
   * |      2  X   X   Y   Y          |      6  X   Y   0   0         |
   * |      3  Z   Z   0   0          |      7  X   Y   0   0         |
   *
   */
   if (varying_has_user_specified_location(xfb_decl->matched_candidate->toplevel_var)) {
      output_size = MIN3(num_components, current_type_components_left, 4);
   current_type_components_left -= output_size;
   if (current_type_components_left == 0) {
            } else {
                                 /* From the ARB_enhanced_layouts spec:
   *
   *    "If such a block member or variable is not written during a shader
   *    invocation, the buffer contents at the assigned offset will be
   *    undefined.  Even if there are no static writes to a variable or
   *    member that is assigned a transform feedback offset, the space is
   *    still allocated in the buffer and still affects the stride."
   */
   if (xfb_decl_is_varying_written(xfb_decl)) {
      info->Outputs[info->NumOutputs].ComponentOffset = location_frac;
   info->Outputs[info->NumOutputs].OutputRegister = location;
   info->Outputs[info->NumOutputs].NumComponents = output_size;
   info->Outputs[info->NumOutputs].StreamId = xfb_decl->stream_id;
   info->Outputs[info->NumOutputs].OutputBuffer = buffer;
   info->Outputs[info->NumOutputs].DstOffset = xfb_offset;
      }
                  num_components -= output_size;
   location++;
                  if (explicit_stride && explicit_stride[buffer]) {
      if (_mesa_gl_datatype_is_64bit(xfb_decl->type) &&
      info->Buffers[buffer].Stride % 2) {
   linker_error(prog, "invalid qualifier xfb_stride=%d must be a "
               "multiple of 8 as its applied to a type that is or "
               if (xfb_offset > info->Buffers[buffer].Stride) {
      linker_error(prog, "xfb_offset (%d) overflows xfb_stride (%d) for "
                     } else {
      if (max_member_alignment && has_xfb_qualifiers) {
      max_member_alignment[buffer] = MAX2(max_member_alignment[buffer],
         info->Buffers[buffer].Stride = ALIGN(xfb_offset,
      } else {
                  store_varying:
      info->Varyings[info->NumVarying].name.string =
         resource_name_updated(&info->Varyings[info->NumVarying].name);
   info->Varyings[info->NumVarying].Type = xfb_decl->type;
   info->Varyings[info->NumVarying].Size = size;
   info->Varyings[info->NumVarying].BufferIndex = buffer_index;
   info->NumVarying++;
               }
      static const struct tfeedback_candidate *
   xfb_decl_find_candidate(struct xfb_decl *xfb_decl,
               {
      const char *name = xfb_decl->var_name;
   switch (xfb_decl->lowered_builtin_array_variable) {
   case none:
      name = xfb_decl->var_name;
      case clip_distance:
   case cull_distance:
      name = "gl_ClipDistanceMESA";
      }
   struct hash_entry *entry =
            xfb_decl->matched_candidate = entry ?
            if (!xfb_decl->matched_candidate) {
      /* From GL_EXT_transform_feedback:
   *   A program will fail to link if:
   *
   *   * any variable name specified in the <varyings> array is not
   *     declared as an output in the geometry shader (if present) or
   *     the vertex shader (if no geometry shader is present);
   */
   linker_error(prog, "Transform feedback varying %s undeclared.",
                  }
      /**
   * Force a candidate over the previously matched one. It happens when a new
   * varying needs to be created to match the xfb declaration, for example,
   * to fullfil an alignment criteria.
   */
   static void
   xfb_decl_set_lowered_candidate(struct xfb_decl *xfb_decl,
         {
               /* The subscript part is no longer relevant */
   xfb_decl->is_subscripted = false;
      }
      /**
   * Parse all the transform feedback declarations that were passed to
   * glTransformFeedbackVaryings() and store them in xfb_decl objects.
   *
   * If an error occurs, the error is reported through linker_error() and false
   * is returned.
   */
   static bool
   parse_xfb_decls(const struct gl_constants *consts,
                  const struct gl_extensions *exts,
      {
      for (unsigned i = 0; i < num_names; ++i) {
               if (!xfb_decl_is_varying(&decls[i]))
            /* From GL_EXT_transform_feedback:
   *   A program will fail to link if:
   *
   *   * any two entries in the <varyings> array specify the same varying
   *     variable;
   *
   * We interpret this to mean "any two entries in the <varyings> array
   * specify the same varying variable and array index", since transform
   * feedback of arrays would be useless otherwise.
   */
   for (unsigned j = 0; j < i; ++j) {
      if (xfb_decl_is_varying(&decls[j])) {
      if (xfb_decl_is_same(&decls[i], &decls[j])) {
      linker_error(prog, "Transform feedback varying %s specified "
                     }
      }
      static int
   cmp_xfb_offset(const void * x_generic, const void * y_generic)
   {
      struct xfb_decl *x = (struct xfb_decl *) x_generic;
            if (x->buffer != y->buffer)
            }
      /**
   * Store transform feedback location assignments into
   * prog->sh.LinkedTransformFeedback based on the data stored in
   * xfb_decls.
   *
   * If an error occurs, the error is reported through linker_error() and false
   * is returned.
   */
   static bool
   store_tfeedback_info(const struct gl_constants *consts,
                     {
      if (!prog->last_vert_prog)
            /* Make sure MaxTransformFeedbackBuffers is less than 32 so the bitmask for
   * tracking the number of buffers doesn't overflow.
   */
            bool separate_attribs_mode =
            struct gl_program *xfb_prog = prog->last_vert_prog;
   xfb_prog->sh.LinkedTransformFeedback =
            /* The xfb_offset qualifier does not have to be used in increasing order
   * however some drivers expect to receive the list of transform feedback
   * declarations in order so sort it now for convenience.
   */
   if (has_xfb_qualifiers) {
      qsort(xfb_decls, num_xfb_decls, sizeof(*xfb_decls),
               xfb_prog->sh.LinkedTransformFeedback->Varyings =
      rzalloc_array(xfb_prog, struct gl_transform_feedback_varying_info,
         unsigned num_outputs = 0;
   for (unsigned i = 0; i < num_xfb_decls; ++i) {
      if (xfb_decl_is_varying_written(&xfb_decls[i]))
               xfb_prog->sh.LinkedTransformFeedback->Outputs =
      rzalloc_array(xfb_prog, struct gl_transform_feedback_output,
         unsigned num_buffers = 0;
   unsigned buffers = 0;
            if (!has_xfb_qualifiers && separate_attribs_mode) {
      /* GL_SEPARATE_ATTRIBS */
   for (unsigned i = 0; i < num_xfb_decls; ++i) {
      if (!xfb_decl_store(&xfb_decls[i], consts, prog,
                                    buffers |= 1 << num_buffers;
         }
   else {
      /* GL_INVERLEAVED_ATTRIBS */
   int buffer_stream_id = -1;
   unsigned buffer =
         bool explicit_stride[MAX_FEEDBACK_BUFFERS] = { false };
   unsigned max_member_alignment[MAX_FEEDBACK_BUFFERS] = { 1, 1, 1, 1 };
   /* Apply any xfb_stride global qualifiers */
   if (has_xfb_qualifiers) {
      for (unsigned j = 0; j < MAX_FEEDBACK_BUFFERS; j++) {
      if (prog->TransformFeedback.BufferStride[j]) {
      explicit_stride[j] = true;
   xfb_prog->sh.LinkedTransformFeedback->Buffers[j].Stride =
                     for (unsigned i = 0; i < num_xfb_decls; ++i) {
      if (has_xfb_qualifiers &&
      buffer != xfb_decls[i].buffer) {
   /* we have moved to the next buffer so reset stream id */
   buffer_stream_id = -1;
               if (xfb_decls[i].next_buffer_separator) {
      if (!xfb_decl_store(&xfb_decls[i], consts, prog,
                     xfb_prog->sh.LinkedTransformFeedback,
   buffer, num_buffers, num_outputs,
         num_buffers++;
   buffer_stream_id = -1;
               if (has_xfb_qualifiers) {
         } else {
                  if (xfb_decl_is_varying(&xfb_decls[i])) {
      if (buffer_stream_id == -1)  {
                     /* Only mark a buffer as active when there is a varying
   * attached to it. This behaviour is based on a revised version
   * of section 13.2.2 of the GL 4.6 spec.
   */
      } else if (buffer_stream_id !=
            /* Varying writes to the same buffer from a different stream */
   linker_error(prog,
               "Transform feedback can't capture varyings belonging "
   "to different vertex streams in a single buffer. "
   "Varying %s writes to buffer from stream %u, other "
   "varyings in the same buffer write from stream %u.",
   xfb_decls[i].orig_name,
                  if (!xfb_decl_store(&xfb_decls[i], consts, prog,
                     xfb_prog->sh.LinkedTransformFeedback,
         }
            xfb_prog->sh.LinkedTransformFeedback->ActiveBuffers = buffers;
      }
      /**
   * Enum representing the order in which varyings are packed within a
   * packing class.
   *
   * Currently we pack vec4's first, then vec2's, then scalar values, then
   * vec3's.  This order ensures that the only vectors that are at risk of
   * having to be "double parked" (split between two adjacent varying slots)
   * are the vec3's.
   */
   enum packing_order_enum {
      PACKING_ORDER_VEC4,
   PACKING_ORDER_VEC2,
   PACKING_ORDER_SCALAR,
      };
      /**
   * Structure recording the relationship between a single producer output
   * and a single consumer input.
   */
   struct match {
      /**
   * Packing class for this varying, computed by compute_packing_class().
   */
            /**
   * Packing order for this varying, computed by compute_packing_order().
   */
            /**
   * The output variable in the producer stage.
   */
            /**
   * The input variable in the consumer stage.
   */
            /**
   * The location which has been assigned for this varying.  This is
   * expressed in multiples of a float, with the first generic varying
   * (i.e. the one referred to by VARYING_SLOT_VAR0) represented by the
   * value 0.
   */
      };
      /**
   * Data structure recording the relationship between outputs of one shader
   * stage (the "producer") and inputs of another (the "consumer").
   */
   struct varying_matches
   {
      /**
   * If true, this driver disables varying packing, so all varyings need to
   * be aligned on slot boundaries, and take up a number of slots equal to
   * their number of matrix columns times their array size.
   *
   * Packing may also be disabled because our current packing method is not
   * safe in SSO or versions of OpenGL where interpolation qualifiers are not
   * guaranteed to match across stages.
   */
            /**
   * If true, this driver disables packing for varyings used by transform
   * feedback.
   */
            /**
   * If true, this driver has transform feedback enabled. The transform
   * feedback code usually requires at least some packing be done even
   * when varying packing is disabled, fortunately where transform feedback
   * requires packing it's safe to override the disabled setting. See
   * is_varying_packing_safe().
   */
                     /**
   * If true, this driver prefers varyings to be aligned to power of two
   * in a slot.
   */
                     /**
   * The number of elements in the \c matches array that are currently in
   * use.
   */
            /**
   * The number of elements that were set aside for the \c matches array when
   * it was allocated.
   */
            gl_shader_stage producer_stage;
      };
      /**
   * Comparison function passed to qsort() to sort varyings by packing_class and
   * then by packing_order.
   */
   static int
   varying_matches_match_comparator(const void *x_generic, const void *y_generic)
   {
      const struct match *x = (const struct match *) x_generic;
            if (x->packing_class != y->packing_class)
            }
      /**
   * Comparison function passed to qsort() to sort varyings used only by
   * transform feedback when packing of other varyings is disabled.
   */
   static int
   varying_matches_xfb_comparator(const void *x_generic, const void *y_generic)
   {
               if (x->producer_var != NULL && x->producer_var->data.is_xfb_only)
            /* FIXME: When the comparator returns 0 it means the elements being
   * compared are equivalent. However the qsort documentation says:
   *
   *    "The order of equivalent elements is undefined."
   *
   * In practice the sort ends up reversing the order of the varyings which
   * means locations are also assigned in this reversed order and happens to
   * be what we want. This is also whats happening in
   * varying_matches_match_comparator().
   */
      }
      /**
   * Comparison function passed to qsort() to sort varyings NOT used by
   * transform feedback when packing of xfb varyings is disabled.
   */
   static int
   varying_matches_not_xfb_comparator(const void *x_generic, const void *y_generic)
   {
               if (x->producer_var != NULL && !x->producer_var->data.is_xfb)
            /* FIXME: When the comparator returns 0 it means the elements being
   * compared are equivalent. However the qsort documentation says:
   *
   *    "The order of equivalent elements is undefined."
   *
   * In practice the sort ends up reversing the order of the varyings which
   * means locations are also assigned in this reversed order and happens to
   * be what we want. This is also whats happening in
   * varying_matches_match_comparator().
   */
      }
      static bool
   is_unpackable_tess(gl_shader_stage producer_stage,
         {
      if (consumer_stage == MESA_SHADER_TESS_EVAL ||
      consumer_stage == MESA_SHADER_TESS_CTRL ||
   producer_stage == MESA_SHADER_TESS_CTRL)
            }
      static void
   init_varying_matches(void *mem_ctx, struct varying_matches *vm,
                        const struct gl_constants *consts,
      {
      /* Tessellation shaders treat inputs and outputs as shared memory and can
   * access inputs and outputs of other invocations.
   * Therefore, they can't be lowered to temps easily (and definitely not
   * efficiently).
   */
   bool unpackable_tess =
            /* Transform feedback code assumes varying arrays are packed, so if the
   * driver has disabled varying packing, make sure to at least enable
   * packing required by transform feedback. See below for exception.
   */
            /* Some drivers actually requires packing to be explicitly disabled
   * for varyings used by transform feedback.
   */
            /* Disable packing on outward facing interfaces for SSO because in ES we
   * need to retain the unpacked varying information for draw time
   * validation.
   *
   * Packing is still enabled on individual arrays, structs, and matrices as
   * these are required by the transform feedback code and it is still safe
   * to do so. We also enable packing when a varying is only used for
   * transform feedback and its not a SSO.
   */
   bool disable_varying_packing =
         if (sso && (producer_stage == MESA_SHADER_NONE || consumer_stage == MESA_SHADER_NONE))
            /* Note: this initial capacity is rather arbitrarily chosen to be large
   * enough for many cases without wasting an unreasonable amount of space.
   * varying_matches_record() will resize the array if there are more than
   * this number of varyings.
   */
   vm->matches_capacity = 8;
   vm->matches = (struct match *)
                  vm->disable_varying_packing = disable_varying_packing;
   vm->disable_xfb_packing = disable_xfb_packing;
   vm->xfb_enabled = xfb_enabled;
   vm->enhanced_layouts_enabled = exts->ARB_enhanced_layouts;
   vm->prefer_pot_aligned_varyings = consts->PreferPOTAlignedVaryings;
   vm->producer_stage = producer_stage;
      }
      /**
   * Packing is always safe on individual arrays, structures, and matrices. It
   * is also safe if the varying is only used for transform feedback.
   */
   static bool
   is_varying_packing_safe(struct varying_matches *vm,
         {
      if (is_unpackable_tess(vm->producer_stage, vm->consumer_stage))
            return vm->xfb_enabled && (glsl_type_is_array_or_matrix(type) ||
            }
      static bool
   is_packing_disabled(struct varying_matches *vm, const struct glsl_type *type,
         {
      return (vm->disable_varying_packing && !is_varying_packing_safe(vm, type, var)) ||
      (vm->disable_xfb_packing && var->data.is_xfb &&
   !(glsl_type_is_array(type) || glsl_type_is_struct(type) ||
   }
      /**
   * Compute the "packing class" of the given varying.  This is an unsigned
   * integer with the property that two variables in the same packing class can
   * be safely backed into the same vec4.
   */
   static unsigned
   varying_matches_compute_packing_class(const nir_variable *var)
   {
      /* Without help from the back-end, there is no way to pack together
   * variables with different interpolation types, because
   * lower_packed_varyings must choose exactly one interpolation type for
   * each packed varying it creates.
   *
   * However, we can safely pack together floats, ints, and uints, because:
   *
   * - varyings of base type "int" and "uint" must use the "flat"
   *   interpolation type, which can only occur in GLSL 1.30 and above.
   *
   * - On platforms that support GLSL 1.30 and above, lower_packed_varyings
   *   can store flat floats as ints without losing any information (using
   *   the ir_unop_bitcast_* opcodes).
   *
   * Therefore, the packing class depends only on the interpolation type.
   */
   bool is_interpolation_flat = var->data.interpolation == INTERP_MODE_FLAT ||
            const unsigned interp = is_interpolation_flat
                     const unsigned packing_class = (interp << 0) |
                                 }
      /**
   * Compute the "packing order" of the given varying.  This is a sort key we
   * use to determine when to attempt to pack the given varying relative to
   * other varyings in the same packing class.
   */
   static enum packing_order_enum
   varying_matches_compute_packing_order(const nir_variable *var)
   {
               switch (glsl_get_component_slots(element_type) % 4) {
   case 1: return PACKING_ORDER_SCALAR;
   case 2: return PACKING_ORDER_VEC2;
   case 3: return PACKING_ORDER_VEC3;
   case 0: return PACKING_ORDER_VEC4;
   default:
      assert(!"Unexpected value of vector_elements");
         }
      /**
   * Record the given producer/consumer variable pair in the list of variables
   * that should later be assigned locations.
   *
   * It is permissible for \c consumer_var to be NULL (this happens if a
   * variable is output by the producer and consumed by transform feedback, but
   * not consumed by the consumer).
   *
   * If \c producer_var has already been paired up with a consumer_var, or
   * producer_var is part of fixed pipeline functionality (and hence already has
   * a location assigned), this function has no effect.
   *
   * Note: as a side effect this function may change the interpolation type of
   * \c producer_var, but only when the change couldn't possibly affect
   * rendering.
   */
   static void
   varying_matches_record(void *mem_ctx, struct varying_matches *vm,
         {
               if ((producer_var &&
      (producer_var->data.explicit_location || producer_var->data.location != -1)) ||
   (consumer_var &&
   (consumer_var->data.explicit_location || consumer_var->data.location != -1))) {
   /* Either a location already exists for this variable (since it is part
   * of fixed functionality), or it has already been assigned explicitly.
   */
               /* The varyings should not have been matched and assgned previously */
   assert((producer_var == NULL || producer_var->data.location == -1) &&
            bool needs_flat_qualifier = consumer_var == NULL &&
      (glsl_contains_integer(producer_var->type) ||
         if (!vm->disable_varying_packing &&
      (!vm->disable_xfb_packing || producer_var  == NULL || !producer_var->data.is_xfb) &&
   (needs_flat_qualifier ||
   (vm->consumer_stage != MESA_SHADER_NONE && vm->consumer_stage != MESA_SHADER_FRAGMENT))) {
   /* Since this varying is not being consumed by the fragment shader, its
   * interpolation type varying cannot possibly affect rendering.
   * Also, this variable is non-flat and is (or contains) an integer
   * or a double.
   * If the consumer stage is unknown, don't modify the interpolation
   * type as it could affect rendering later with separate shaders.
   *
   * lower_packed_varyings requires all integer varyings to flat,
   * regardless of where they appear.  We can trivially satisfy that
   * requirement by changing the interpolation type to flat here.
   */
   if (producer_var) {
      producer_var->data.centroid = false;
   producer_var->data.sample = false;
               if (consumer_var) {
      consumer_var->data.centroid = false;
   consumer_var->data.sample = false;
                  if (vm->num_matches == vm->matches_capacity) {
      vm->matches_capacity *= 2;
   vm->matches = (struct match *)
               /* We must use the consumer to compute the packing class because in GL4.4+
   * there is no guarantee interpolation qualifiers will match across stages.
   *
   * From Section 4.5 (Interpolation Qualifiers) of the GLSL 4.30 spec:
   *
   *    "The type and presence of interpolation qualifiers of variables with
   *    the same name declared in all linked shaders for the same cross-stage
   *    interface must match, otherwise the link command will fail.
   *
   *    When comparing an output from one stage to an input of a subsequent
   *    stage, the input and output don't match if their interpolation
   *    qualifiers (or lack thereof) are not the same."
   *
   * This text was also in at least revison 7 of the 4.40 spec but is no
   * longer in revision 9 and not in the 4.50 spec.
   */
   const nir_variable *const var = (consumer_var != NULL)
            if (producer_var && consumer_var &&
      consumer_var->data.must_be_shader_input) {
               vm->matches[vm->num_matches].packing_class
         vm->matches[vm->num_matches].packing_order
            vm->matches[vm->num_matches].producer_var = producer_var;
   vm->matches[vm->num_matches].consumer_var = consumer_var;
      }
      /**
   * Choose locations for all of the variable matches that were previously
   * passed to varying_matches_record().
   * \param components  returns array[slot] of number of components used
   *                    per slot (1, 2, 3 or 4)
   * \param reserved_slots  bitmask indicating which varying slots are already
   *                        allocated
   * \return number of slots (4-element vectors) allocated
   */
   static unsigned
   varying_matches_assign_locations(struct varying_matches *vm,
               {
      /* If packing has been disabled then we cannot safely sort the varyings by
   * class as it may mean we are using a version of OpenGL where
   * interpolation qualifiers are not guaranteed to be matching across
   * shaders, sorting in this case could result in mismatching shader
   * interfaces.
   * When packing is disabled the sort orders varyings used by transform
   * feedback first, but also depends on *undefined behaviour* of qsort to
   * reverse the order of the varyings. See: xfb_comparator().
   *
   * If packing is only disabled for xfb varyings (mutually exclusive with
   * disable_varying_packing), we then group varyings depending on if they
   * are captured for transform feedback. The same *undefined behaviour* is
   * taken advantage of.
   */
   if (vm->disable_varying_packing) {
      /* Only sort varyings that are only used by transform feedback. */
   qsort(vm->matches, vm->num_matches, sizeof(*vm->matches),
      } else if (vm->disable_xfb_packing) {
      /* Only sort varyings that are NOT used by transform feedback. */
   qsort(vm->matches, vm->num_matches, sizeof(*vm->matches),
      } else {
      /* Sort varying matches into an order that makes them easy to pack. */
   qsort(vm->matches, vm->num_matches, sizeof(*vm->matches),
               unsigned generic_location = 0;
   unsigned generic_patch_location = MAX_VARYING*4;
   bool previous_var_xfb = false;
   bool previous_var_xfb_only = false;
            /* For tranform feedback separate mode, we know the number of attributes
   * is <= the number of buffers.  So packing isn't critical.  In fact,
   * packing vec3 attributes can cause trouble because splitting a vec3
   * effectively creates an additional transform feedback output.  The
   * extra TFB output may exceed device driver limits.
   *
   * Also don't pack vec3 if the driver prefers power of two aligned
   * varyings. Packing order guarantees that vec4, vec2 and vec1 will be
   * pot-aligned, we only need to take care of vec3s
   */
   const bool dont_pack_vec3 =
      (prog->TransformFeedback.BufferMode == GL_SEPARATE_ATTRIBS &&
   prog->TransformFeedback.NumVarying > 0) ||
         for (unsigned i = 0; i < vm->num_matches; i++) {
      unsigned *location = &generic_location;
   const nir_variable *var;
   const struct glsl_type *type;
            if (vm->matches[i].consumer_var) {
      var = vm->matches[i].consumer_var;
   type = get_varying_type(var, vm->consumer_stage);
   if (vm->consumer_stage == MESA_SHADER_VERTEX)
      } else {
                     var = vm->matches[i].producer_var;
               if (var->data.patch)
            /* Advance to the next slot if this varying has a different packing
   * class than the previous one, and we're not already on a slot
   * boundary.
   *
   * Also advance if varying packing is disabled for transform feedback,
   * and previous or current varying is used for transform feedback.
   *
   * Also advance to the next slot if packing is disabled. This makes sure
   * we don't assign varyings the same locations which is possible
   * because we still pack individual arrays, records and matrices even
   * when packing is disabled. Note we don't advance to the next slot if
   * we can pack varyings together that are only used for transform
   * feedback.
   */
   if (var->data.must_be_shader_input ||
      (vm->disable_xfb_packing &&
   (previous_var_xfb || var->data.is_xfb)) ||
   (vm->disable_varying_packing &&
   !(previous_var_xfb_only && var->data.is_xfb_only)) ||
   (previous_packing_class != vm->matches[i].packing_class) ||
   (vm->matches[i].packing_order == PACKING_ORDER_VEC3 &&
   dont_pack_vec3)) {
               previous_var_xfb = var->data.is_xfb;
   previous_var_xfb_only = var->data.is_xfb_only;
            /* The number of components taken up by this variable. For vertex shader
   * inputs, we use the number of slots * 4, as they have different
   * counting rules.
   */
   unsigned num_components = 0;
   if (is_vertex_input) {
         } else {
      if (is_packing_disabled(vm, type, var)) {
         } else {
                     /* The last slot for this variable, inclusive. */
            /* FIXME: We could be smarter in the below code and loop back over
   * trying to fill any locations that we skipped because we couldn't pack
   * the varying between an explicit location. For now just let the user
   * hit the linking error if we run out of room and suggest they use
   * explicit locations.
   */
   while (slot_end < MAX_VARYING * 4u) {
                              if ((reserved_slots & slot_mask) == 0) {
                  *location = ALIGN(*location + 1, 4);
               if (!var->data.patch && slot_end >= MAX_VARYING * 4u) {
      linker_error(prog, "insufficient contiguous locations available for "
               "%s it is possible an array or struct could not be "
               if (slot_end < MAX_VARYINGS_INCL_PATCH * 4u) {
      for (unsigned j = *location / 4u; j < slot_end / 4u; j++)
                                             }
      static void
   varying_matches_assign_temp_locations(struct varying_matches *vm,
               {
      unsigned tmp_loc = 0;
   for (unsigned i = 0; i < vm->num_matches; i++) {
      nir_variable *producer_var = vm->matches[i].producer_var;
            while (tmp_loc < MAX_VARYINGS_INCL_PATCH) {
      if (reserved_slots & (UINT64_C(1) << tmp_loc))
         else
               if (producer_var) {
      assert(producer_var->data.location == -1);
               if (consumer_var) {
      assert(consumer_var->data.location == -1);
                     }
      /**
   * Update the producer and consumer shaders to reflect the locations
   * assignments that were made by varying_matches_assign_locations().
   */
   static void
   varying_matches_store_locations(struct varying_matches *vm)
   {
      /* Check is location needs to be packed with lower_packed_varyings() or if
   * we can just use ARB_enhanced_layouts packing.
   */
   bool pack_loc[MAX_VARYINGS_INCL_PATCH] = {0};
            for (unsigned i = 0; i < vm->num_matches; i++) {
      nir_variable *producer_var = vm->matches[i].producer_var;
   nir_variable *consumer_var = vm->matches[i].consumer_var;
   unsigned generic_location = vm->matches[i].generic_location;
   unsigned slot = generic_location / 4;
            if (producer_var) {
      producer_var->data.location = VARYING_SLOT_VAR0 + slot;
               if (consumer_var) {
      consumer_var->data.location = VARYING_SLOT_VAR0 + slot;
               /* Find locations suitable for native packing via
   * ARB_enhanced_layouts.
   */
   if (vm->enhanced_layouts_enabled) {
      nir_variable *var = producer_var ? producer_var : consumer_var;
   unsigned stage = producer_var ? vm->producer_stage : vm->consumer_stage;
   const struct glsl_type *type =
         unsigned comp_slots = glsl_get_component_slots(type) + offset;
   unsigned slots = comp_slots / 4;
                  if (producer_var && consumer_var) {
      if (glsl_type_is_array_or_matrix(type) || glsl_type_is_struct(type) ||
      glsl_type_is_64bit(type)) {
   for (unsigned j = 0; j < slots; j++) {
            } else if (offset + glsl_get_vector_elements(type) > 4) {
      pack_loc[slot] = true;
      } else {
            } else {
      for (unsigned j = 0; j < slots; j++) {
                           /* Attempt to use ARB_enhanced_layouts for more efficient packing if
   * suitable.
   */
   if (vm->enhanced_layouts_enabled) {
      for (unsigned i = 0; i < vm->num_matches; i++) {
      nir_variable *producer_var = vm->matches[i].producer_var;
   nir_variable *consumer_var = vm->matches[i].consumer_var;
                  unsigned generic_location = vm->matches[i].generic_location;
   unsigned slot = generic_location / 4;
                  const struct glsl_type *type =
         bool type_match = true;
   for (unsigned j = 0; j < 4; j++) {
      if (loc_type[slot][j]) {
      if (glsl_get_base_type(type) !=
      glsl_get_base_type(loc_type[slot][j]))
               if (type_match) {
      producer_var->data.explicit_location = 1;
               }
      /**
   * Is the given variable a varying variable to be counted against the
   * limit in ctx->Const.MaxVarying?
   * This includes variables such as texcoords, colors and generic
   * varyings, but excludes variables such as gl_FrontFacing and gl_FragCoord.
   */
   static bool
   var_counts_against_varying_limit(gl_shader_stage stage, const nir_variable *var)
   {
      /* Only fragment shaders will take a varying variable as an input */
   if (stage == MESA_SHADER_FRAGMENT &&
      var->data.mode == nir_var_shader_in) {
   switch (var->data.location) {
   case VARYING_SLOT_POS:
   case VARYING_SLOT_FACE:
   case VARYING_SLOT_PNTC:
         default:
            }
      }
      struct tfeedback_candidate_generator_state {
      /**
   * Memory context used to allocate hash table keys and values.
   */
            /**
   * Hash table in which tfeedback_candidate objects should be stored.
   */
                     /**
   * Pointer to the toplevel variable that is being traversed.
   */
            /**
   * Total number of varying floats that have been visited so far.  This is
   * used to determine the offset to each varying within the toplevel
   * variable.
   */
            /**
   * Offset within the xfb. Counted in floats.
   */
      };
      /**
   * Generates tfeedback_candidate structs describing all possible targets of
   * transform feedback.
   *
   * tfeedback_candidate structs are stored in the hash table
   * tfeedback_candidates.  This hash table maps varying names to instances of the
   * tfeedback_candidate struct.
   */
   static void
   tfeedback_candidate_generator(struct tfeedback_candidate_generator_state *state,
                     {
      switch (glsl_get_base_type(type)) {
   case GLSL_TYPE_INTERFACE:
      if (named_ifc_member) {
      ralloc_asprintf_rewrite_tail(name, &name_length, ".%s",
         tfeedback_candidate_generator(state, name, name_length,
            }
      case GLSL_TYPE_STRUCT:
      for (unsigned i = 0; i < glsl_get_length(type); i++) {
               /* Append '.field' to the current variable name. */
   if (name) {
      ralloc_asprintf_rewrite_tail(name, &new_length, ".%s",
               tfeedback_candidate_generator(state, name, new_length,
                  case GLSL_TYPE_ARRAY:
      if (glsl_type_is_struct(glsl_without_array(type)) ||
                                                      tfeedback_candidate_generator(state, name, new_length,
                        }
      default:
      assert(!glsl_type_is_struct(glsl_without_array(type)));
            struct tfeedback_candidate *candidate
         candidate->toplevel_var = state->toplevel_var;
            if (glsl_type_is_64bit(glsl_without_array(type))) {
      /*  From ARB_gpu_shader_fp64:
   *
   * If any variable captured in transform feedback has double-precision
   * components, the practical requirements for defined behavior are:
   *     ...
   * (c) each double-precision variable captured must be aligned to a
   *     multiple of eight bytes relative to the beginning of a vertex.
   */
   state->xfb_offset_floats = ALIGN(state->xfb_offset_floats, 2);
   /* 64-bit members of structs are also aligned. */
               candidate->xfb_offset_floats = state->xfb_offset_floats;
            _mesa_hash_table_insert(state->tfeedback_candidates,
                           if (varying_has_user_specified_location(state->toplevel_var)) {
         } else {
                        }
      static void
   populate_consumer_input_sets(void *mem_ctx, nir_shader *nir,
                     {
      memset(consumer_inputs_with_locations, 0,
            nir_foreach_shader_in_variable(input_var, nir) {
      /* All interface blocks should have been lowered by this point */
            if (input_var->data.explicit_location) {
      /* assign_varying_locations only cares about finding the
   * nir_variable at the start of a contiguous location block.
   *
   *     - For !producer, consumer_inputs_with_locations isn't used.
   *
   *     - For !consumer, consumer_inputs_with_locations is empty.
   *
   * For consumer && producer, if you were trying to set some
   * nir_variable to the middle of a location block on the other side
   * of producer/consumer, cross_validate_outputs_to_inputs() should
   * be link-erroring due to either type mismatch or location
   * overlaps.  If the variables do match up, then they've got a
   * matching data.location and you only looked at
   * consumer_inputs_with_locations[var->data.location], not any
   * following entries for the array/structure.
   */
   consumer_inputs_with_locations[input_var->data.location] =
      } else if (input_var->interface_type != NULL) {
      char *const iface_field_name =
      ralloc_asprintf(mem_ctx, "%s.%s",
      glsl_get_type_name(glsl_without_array(input_var->interface_type)),
   _mesa_hash_table_insert(consumer_interface_inputs,
      } else {
      _mesa_hash_table_insert(consumer_inputs,
                  }
      /**
   * Find a variable from the consumer that "matches" the specified variable
   *
   * This function only finds inputs with names that match.  There is no
   * validation (here) that the types, etc. are compatible.
   */
   static nir_variable *
   get_matching_input(void *mem_ctx,
                     const nir_variable *output_var,
   {
               if (output_var->data.explicit_location) {
         } else if (output_var->interface_type != NULL) {
      char *const iface_field_name =
      ralloc_asprintf(mem_ctx, "%s.%s",
      glsl_get_type_name(glsl_without_array(output_var->interface_type)),
   struct hash_entry *entry =
            } else {
      struct hash_entry *entry =
                     return (input_var == NULL || input_var->data.mode != nir_var_shader_in)
      }
      static int
   io_variable_cmp(const void *_a, const void *_b)
   {
      const nir_variable *const a = *(const nir_variable **) _a;
            if (a->data.explicit_location && b->data.explicit_location)
            if (a->data.explicit_location && !b->data.explicit_location)
            if (!a->data.explicit_location && b->data.explicit_location)
               }
      /**
   * Sort the shader IO variables into canonical order
   */
   static void
   canonicalize_shader_io(nir_shader *nir, nir_variable_mode io_mode)
   {
      nir_variable *var_table[MAX_PROGRAM_OUTPUTS * 4];
            nir_foreach_variable_with_modes(var, nir, io_mode) {
      /* If we have already encountered more I/O variables that could
   * successfully link, bail.
   */
   if (num_variables == ARRAY_SIZE(var_table))
                        if (num_variables == 0)
            /* Sort the list in reverse order (io_variable_cmp handles this).  Later
   * we're going to push the variables on to the IR list as a stack, so we
   * want the last variable (in canonical order) to be first in the list.
   */
            /* Remove the variable from it's current location in the varible list, and
   * put it at the front.
   */
   for (unsigned i = 0; i < num_variables; i++) {
      exec_node_remove(&var_table[i]->node);
         }
      /**
   * Generate a bitfield map of the explicit locations for shader varyings.
   *
   * Note: For Tessellation shaders we are sitting right on the limits of the
   * 64 bit map. Per-vertex and per-patch both have separate location domains
   * with a max of MAX_VARYING.
   */
   static uint64_t
   reserved_varying_slot(struct gl_linked_shader *sh,
         {
      assert(io_mode == nir_var_shader_in || io_mode == nir_var_shader_out);
   /* Avoid an overflow of the returned value */
            uint64_t slots = 0;
            if (!sh)
            nir_foreach_variable_with_modes(var, sh->Program->nir, io_mode) {
      if (!var->data.explicit_location ||
                           bool is_gl_vertex_input = io_mode == nir_var_shader_in &&
         unsigned num_elements =
      glsl_count_attribute_slots(get_varying_type(var, sh->Stage),
      for (unsigned i = 0; i < num_elements; i++) {
      if (var_slot >= 0 && var_slot < MAX_VARYINGS_INCL_PATCH)
                           }
      /**
   * Sets the bits in the inputs_read, or outputs_written
   * bitfield corresponding to this variable.
   */
   static void
   set_variable_io_mask(BITSET_WORD *bits, nir_variable *var, gl_shader_stage stage)
   {
      assert(var->data.mode == nir_var_shader_in ||
                  const struct glsl_type *type = var->type;
   if (nir_is_arrayed_io(var, stage) || var->data.per_view) {
      assert(glsl_type_is_array(type));
               unsigned location = var->data.location - VARYING_SLOT_VAR0;
   unsigned slots = glsl_count_attribute_slots(type, false);
   for (unsigned i = 0; i < slots; i++) {
            }
      static uint8_t
   get_num_components(nir_variable *var)
   {
      if (glsl_type_is_struct_or_ifc(glsl_without_array(var->type)))
               }
      static void
   tcs_add_output_reads(nir_shader *shader, BITSET_WORD **read)
   {
      nir_foreach_function_impl(impl, shader) {
      nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
                     nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
                  nir_deref_instr *deref = nir_src_as_deref(intrin->src[0]);
                  nir_variable *var = nir_deref_instr_get_variable(deref);
   for (unsigned i = 0; i < get_num_components(var); i++) {
                     unsigned comp = var->data.location_frac;
                  }
      /* We need to replace any interp intrinsics with undefined (shader_temp) inputs
   * as no further NIR pass expects to see this.
   */
   static bool
   replace_unused_interpolate_at_with_undef(nir_builder *b, nir_instr *instr,
         {
      if (instr->type == nir_instr_type_intrinsic) {
               if (intrin->intrinsic == nir_intrinsic_interp_deref_at_centroid ||
      intrin->intrinsic == nir_intrinsic_interp_deref_at_sample ||
   intrin->intrinsic == nir_intrinsic_interp_deref_at_offset) {
   nir_variable *var = nir_intrinsic_get_var(intrin, 0);
   if (var->data.mode == nir_var_shader_temp) {
      /* Create undef and rewrite the interp uses */
   nir_def *undef =
                        nir_instr_remove(&intrin->instr);
                        }
      static void
   fixup_vars_lowered_to_temp(nir_shader *shader, nir_variable_mode mode)
   {
      /* Remove all interpolate uses of the unset varying and replace with undef. */
   if (mode == nir_var_shader_in && shader->info.stage == MESA_SHADER_FRAGMENT) {
      (void) nir_shader_instructions_pass(shader,
                                 nir_lower_global_vars_to_local(shader);
      }
      /**
   * Helper for removing unused shader I/O variables, by demoting them to global
   * variables (which may then be dead code eliminated).
   *
   * Example usage is:
   *
   * progress = nir_remove_unused_io_vars(producer, consumer, nir_var_shader_out,
   *                                      read, patches_read) ||
   *                                      progress;
   *
   * The "used" should be an array of 4 BITSET_WORDs representing each
   * .location_frac used.  Note that for vector variables, only the first channel
   * (.location_frac) is examined for deciding if the variable is used!
   */
   static bool
   remove_unused_io_vars(nir_shader *producer, nir_shader *consumer,
                     {
               bool progress = false;
            BITSET_WORD **used;
   nir_foreach_variable_with_modes_safe(var, shader, mode) {
               /* Skip builtins dead builtins are removed elsewhere */
   if (is_gl_identifier(var->name))
            if (var->data.location < VARYING_SLOT_VAR0 && var->data.location >= 0)
            /* Skip xfb varyings and any other type we cannot remove */
   if (var->data.always_active_io)
            if (var->data.explicit_xfb_buffer)
                     /* if location == -1 lower varying to global as it has no match and is not
   * a xfb varying, this must be done after skiping bultins as builtins
   * could be assigned a location of -1.
   * We also lower unused varyings with explicit locations.
   */
   bool use_found = false;
   if (var->data.location >= 0) {
               const struct glsl_type *type = var->type;
   if (nir_is_arrayed_io(var, shader->info.stage) || var->data.per_view) {
      assert(glsl_type_is_array(type));
               unsigned slots = glsl_count_attribute_slots(type, false);
   for (unsigned i = 0; i < slots; i++) {
      if (BITSET_TEST(other_stage, location + i)) {
      use_found = true;
                     if (!use_found) {
      /* This one is invalid, make it a global variable instead */
                           if (mode == nir_var_shader_in) {
      if (!prog->IsES && prog->GLSL_Version <= 120) {
      /* On page 25 (page 31 of the PDF) of the GLSL 1.20 spec:
   *
   *     Only those varying variables used (i.e. read) in
   *     the fragment shader executable must be written to
   *     by the vertex shader executable; declaring
   *     superfluous varying variables in a vertex shader is
   *     permissible.
   *
   * We interpret this text as meaning that the VS must
   * write the variable for the FS to read it.  See
   * "glsl1-varying read but not written" in piglit.
   */
   linker_error(prog, "%s shader varying %s not written "
               "by %s shader\n.",
      } else {
      linker_warning(prog, "%s shader varying %s not written "
                  "by %s shader\n.",
                     if (progress)
               }
      static bool
   remove_unused_varyings(nir_shader *producer, nir_shader *consumer,
         {
      assert(producer->info.stage != MESA_SHADER_FRAGMENT);
            int max_loc_out = 0;
   nir_foreach_shader_out_variable(var, producer) {
      if (var->data.location < VARYING_SLOT_VAR0)
            const struct glsl_type *type = var->type;
   if (nir_is_arrayed_io(var, producer->info.stage) || var->data.per_view) {
      assert(glsl_type_is_array(type));
      }
            max_loc_out = max_loc_out < (var->data.location - VARYING_SLOT_VAR0) + slots ?
               int max_loc_in = 0;
   nir_foreach_shader_in_variable(var, consumer) {
      if (var->data.location < VARYING_SLOT_VAR0)
            const struct glsl_type *type = var->type;
   if (nir_is_arrayed_io(var, consumer->info.stage) || var->data.per_view) {
      assert(glsl_type_is_array(type));
      }
            max_loc_in = max_loc_in < (var->data.location - VARYING_SLOT_VAR0) + slots ?
               /* Old glsl shaders that don't use explicit locations can contain greater
   * than 64 varyings before unused varyings are removed so we must count them
   * and make use of the BITSET macros to keep track of used slots. Once we
   * have removed these excess varyings we can make use of further nir varying
   * linking optimimisation passes.
   */
   BITSET_WORD *read[4];
   BITSET_WORD *written[4];
   int max_loc = MAX2(max_loc_in, max_loc_out);
   for (unsigned i = 0; i < 4; i++) {
      read[i] = rzalloc_array(mem_ctx, BITSET_WORD, BITSET_WORDS(max_loc));
               nir_foreach_shader_out_variable(var, producer) {
      if (var->data.location < VARYING_SLOT_VAR0)
            for (unsigned i = 0; i < get_num_components(var); i++) {
      unsigned comp = var->data.location_frac;
                  nir_foreach_shader_in_variable(var, consumer) {
      if (var->data.location < VARYING_SLOT_VAR0)
            for (unsigned i = 0; i < get_num_components(var); i++) {
      unsigned comp = var->data.location_frac;
                  /* Each TCS invocation can read data written by other TCS invocations,
   * so even if the outputs are not used by the TES we must also make
   * sure they are not read by the TCS before demoting them to globals.
   */
   if (producer->info.stage == MESA_SHADER_TESS_CTRL)
            bool progress = false;
   progress =
         progress =
               }
      static bool
   should_add_varying_match_record(nir_variable *const input_var,
                           /* If a matching input variable was found, add this output (and the input) to
   * the set.  If this is a separable program and there is no consumer stage,
   * add the output.
   *
   * Always add TCS outputs. They are shared by all invocations
   * within a patch and can be used as shared memory.
   */
   return input_var || (prog->SeparateShader && consumer == NULL) ||
      }
      /* This assigns some initial unoptimised varying locations so that our nir
   * optimisations can perform some initial optimisations and also does initial
   * processing of
   */
   static bool
   assign_initial_varying_locations(const struct gl_constants *consts,
                                    const struct gl_extensions *exts,
   void *mem_ctx,
      {
      init_varying_matches(mem_ctx, vm, consts, exts,
                        struct hash_table *tfeedback_candidates =
         _mesa_hash_table_create(mem_ctx, _mesa_hash_string,
   struct hash_table *consumer_inputs =
         _mesa_hash_table_create(mem_ctx, _mesa_hash_string,
   struct hash_table *consumer_interface_inputs =
         _mesa_hash_table_create(mem_ctx, _mesa_hash_string,
   nir_variable *consumer_inputs_with_locations[VARYING_SLOT_TESS_MAX] = {
                  if (consumer)
      populate_consumer_input_sets(mem_ctx, consumer->Program->nir,
               if (producer) {
      nir_foreach_shader_out_variable(output_var, producer->Program->nir) {
      /* Only geometry shaders can use non-zero streams */
   assert(output_var->data.stream == 0 ||
                  if (num_xfb_decls > 0) {
      /* From OpenGL 4.6 (Core Profile) spec, section 11.1.2.1
   * ("Vertex Shader Variables / Output Variables")
   *
   * "Each program object can specify a set of output variables from
   * one shader to be recorded in transform feedback mode (see
   * section 13.3). The variables that can be recorded are those
   * emitted by the first active shader, in order, from the
   * following list:
   *
   *  * geometry shader
   *  * tessellation evaluation shader
   *  * tessellation control shader
   *  * vertex shader"
   *
   * But on OpenGL ES 3.2, section 11.1.2.1 ("Vertex Shader
   * Variables / Output Variables") tessellation control shader is
   * not included in the stages list.
                     const struct glsl_type *type = output_var->data.from_named_ifc_block ?
         if (!output_var->data.patch && producer->Stage == MESA_SHADER_TESS_CTRL) {
                        const struct glsl_struct_field *ifc_member = NULL;
   if (output_var->data.from_named_ifc_block) {
      ifc_member =
                     char *name;
   if (glsl_type_is_struct(glsl_without_array(type)) ||
      (glsl_type_is_array(type) && glsl_type_is_array(glsl_get_array_element(type)))) {
   type = output_var->type;
      } else if (glsl_type_is_interface(glsl_without_array(type))) {
                              struct tfeedback_candidate_generator_state state;
   state.mem_ctx = mem_ctx;
   state.tfeedback_candidates = tfeedback_candidates;
   state.stage = producer->Stage;
                        tfeedback_candidate_generator(&state, &name, strlen(name), type,
                        nir_variable *const input_var =
      get_matching_input(mem_ctx, output_var, consumer_inputs,
               if (should_add_varying_match_record(input_var, prog, producer,
                        /* Only stream 0 outputs can be consumed in the next stage */
   if (input_var && output_var->data.stream != 0) {
      linker_error(prog, "output %s is assigned to stream=%d but "
                        } else {
      /* If there's no producer stage, then this must be a separable program.
   * For example, we may have a program that has just a fragment shader.
   * Later this program will be used with some arbitrary vertex (or
   * geometry) shader program.  This means that locations must be assigned
   * for all the inputs.
   */
   nir_foreach_shader_in_variable(input_var, consumer->Program->nir) {
                     for (unsigned i = 0; i < num_xfb_decls; ++i) {
      if (!xfb_decl_is_varying(&xfb_decls[i]))
            const struct tfeedback_candidate *matched_candidate
            if (matched_candidate == NULL)
            /* There are two situations where a new output varying is needed:
   *
   *  - If varying packing is disabled for xfb and the current declaration
   *    is subscripting an array, whether the subscript is aligned or not.
   *    to preserve the rest of the array for the consumer.
   *
   *  - If a builtin variable needs to be copied to a new variable
   *    before its content is modified by another lowering pass (e.g.
   *    \c gl_Position is transformed by \c nir_lower_viewport_transform).
   */
   const bool lowered =
      (vm->disable_xfb_packing && xfb_decls[i].is_subscripted) ||
   (matched_candidate->toplevel_var->data.explicit_location &&
   matched_candidate->toplevel_var->data.location < VARYING_SLOT_VAR0 &&
   (!consumer || consumer->Stage == MESA_SHADER_FRAGMENT) &&
               if (lowered) {
                     new_var = gl_nir_lower_xfb_varying(producer->Program->nir,
                              /* Create new candidate and replace matched_candidate */
   new_candidate = rzalloc(mem_ctx, struct tfeedback_candidate);
   new_candidate->toplevel_var = new_var;
   new_candidate->type = new_var->type;
   new_candidate->struct_offset_floats = 0;
   new_candidate->xfb_offset_floats = 0;
   _mesa_hash_table_insert(tfeedback_candidates,
                  xfb_decl_set_lowered_candidate(&xfb_decls[i], new_candidate);
               /* Mark as xfb varying */
            /* Mark xfb varyings as always active */
            /* Mark any corresponding inputs as always active also. We must do this
   * because we have a NIR pass that lowers vectors to scalars and another
   * that removes unused varyings.
   * We don't split varyings marked as always active because there is no
   * point in doing so. This means we need to mark both sides of the
   * interface as always active otherwise we will have a mismatch and
   * start removing things we shouldn't.
   */
   nir_variable *const input_var =
      get_matching_input(mem_ctx, matched_candidate->toplevel_var,
            if (input_var) {
      input_var->data.is_xfb = 1;
               /* Add the xfb varying to varying matches if it wasn't already added */
   if ((!should_add_varying_match_record(input_var, prog, producer,
            !matched_candidate->toplevel_var->data.is_xfb_only) || lowered) {
   matched_candidate->toplevel_var->data.is_xfb_only = 1;
   varying_matches_record(mem_ctx, vm, matched_candidate->toplevel_var,
                  uint64_t reserved_out_slots = 0;
   if (producer)
            uint64_t reserved_in_slots = 0;
   if (consumer)
            /* Assign temporary user varying locations. This is required for our NIR
   * varying optimisations to do their matching.
   */
   const uint64_t reserved_slots = reserved_out_slots | reserved_in_slots;
            for (unsigned i = 0; i < num_xfb_decls; ++i) {
      if (!xfb_decl_is_varying(&xfb_decls[i]))
            xfb_decls[i].matched_candidate->initial_location =
         xfb_decls[i].matched_candidate->initial_location_frac =
                  }
      static void
   link_shader_opts(struct varying_matches *vm,
               {
      /* If we can't pack the stage using this pass then we can't lower io to
   * scalar just yet. Instead we leave it to a later NIR linking pass that uses
   * ARB_enhanced_layout style packing to pack things further.
   *
   * Otherwise we might end up causing linking errors and perf regressions
   * because the new scalars will be assigned individual slots and can overflow
   * the available slots.
   */
   if (producer->options->lower_to_scalar && !vm->disable_varying_packing &&
      !vm->disable_xfb_packing) {
   NIR_PASS_V(producer, nir_lower_io_to_scalar_early, nir_var_shader_out);
               gl_nir_opts(producer);
            if (nir_link_opt_varyings(producer, consumer))
            NIR_PASS_V(producer, nir_remove_dead_variables, nir_var_shader_out, NULL);
            if (remove_unused_varyings(producer, consumer, prog, mem_ctx)) {
      NIR_PASS_V(producer, nir_lower_global_vars_to_local);
            gl_nir_opts(producer);
            /* Optimizations can cause varyings to become unused.
   * nir_compact_varyings() depends on all dead varyings being removed so
   * we need to call nir_remove_dead_variables() again here.
   */
   NIR_PASS_V(producer, nir_remove_dead_variables, nir_var_shader_out,
         NIR_PASS_V(consumer, nir_remove_dead_variables, nir_var_shader_in,
                  }
      /**
   * Assign locations for all variables that are produced in one pipeline stage
   * (the "producer") and consumed in the next stage (the "consumer").
   *
   * Variables produced by the producer may also be consumed by transform
   * feedback.
   *
   * \param num_xfb_decls is the number of declarations indicating
   *        variables that may be consumed by transform feedback.
   *
   * \param xfb_decls is a pointer to an array of xfb_decl objects
   *        representing the result of parsing the strings passed to
   *        glTransformFeedbackVaryings().  assign_location() will be called for
   *        each of these objects that matches one of the outputs of the
   *        producer.
   *
   * When num_xfb_decls is nonzero, it is permissible for the consumer to
   * be NULL.  In this case, varying locations are assigned solely based on the
   * requirements of transform feedback.
   */
   static bool
   assign_final_varying_locations(const struct gl_constants *consts,
                                 const struct gl_extensions *exts,
   void *mem_ctx,
   struct gl_shader_program *prog,
   struct gl_linked_shader *producer,
   {
      init_varying_matches(mem_ctx, vm, consts, exts,
                        /* Regather varying matches as we ran optimisations and the previous pointers
   * are no longer valid.
   */
   if (producer) {
      nir_foreach_shader_out_variable(var_out, producer->Program->nir) {
      if (var_out->data.location < VARYING_SLOT_VAR0 ||
                  if (vm->num_matches == vm->matches_capacity) {
      vm->matches_capacity *= 2;
   vm->matches = (struct match *)
                     vm->matches[vm->num_matches].packing_class
                        vm->matches[vm->num_matches].producer_var = var_out;
   vm->matches[vm->num_matches].consumer_var = NULL;
               /* Regather xfb varyings too */
   for (unsigned i = 0; i < num_xfb_decls; i++) {
                     /* Varying pointer was already reset */
                  bool UNUSED is_reset = false;
   bool UNUSED no_outputs = true;
   nir_foreach_shader_out_variable(var_out, producer->Program->nir) {
      no_outputs = false;
   assert(var_out->data.location != -1);
   if (var_out->data.location ==
      xfb_decls[i].matched_candidate->initial_location &&
   var_out->data.location_frac ==
   xfb_decls[i].matched_candidate->initial_location_frac) {
   xfb_decls[i].matched_candidate->toplevel_var = var_out;
   xfb_decls[i].matched_candidate->initial_location = -1;
   is_reset = true;
         }
                  bool found_match = false;
   if (consumer) {
      nir_foreach_shader_in_variable(var_in, consumer->Program->nir) {
      if (var_in->data.location < VARYING_SLOT_VAR0 ||
                  found_match = false;
   for (unsigned i = 0; i < vm->num_matches; i++) {
      if (vm->matches[i].producer_var &&
                     vm->matches[i].consumer_var = var_in;
   found_match = true;
         }
   if (!found_match) {
      if (vm->num_matches == vm->matches_capacity) {
      vm->matches_capacity *= 2;
   vm->matches = (struct match *)
                     vm->matches[vm->num_matches].packing_class
                        vm->matches[vm->num_matches].producer_var = NULL;
   vm->matches[vm->num_matches].consumer_var = var_in;
                     uint8_t components[MAX_VARYINGS_INCL_PATCH] = {0};
   const unsigned slots_used =
                  for (unsigned i = 0; i < num_xfb_decls; ++i) {
      if (xfb_decl_is_varying(&xfb_decls[i])) {
      if (!xfb_decl_assign_location(&xfb_decls[i], consts, prog,
      vm->disable_varying_packing, vm->xfb_enabled))
               if (producer) {
      gl_nir_lower_packed_varyings(consts, prog, mem_ctx, slots_used, components,
                                 if (consumer) {
      unsigned consumer_vertices = 0;
   if (consumer && consumer->Stage == MESA_SHADER_GEOMETRY)
            gl_nir_lower_packed_varyings(consts, prog, mem_ctx, slots_used, components,
                                    }
      static bool
   check_against_output_limit(const struct gl_constants *consts, gl_api api,
                     {
      unsigned output_vectors = num_explicit_locations;
   nir_foreach_shader_out_variable(var, producer->Program->nir) {
      if (!var->data.explicit_location &&
      var_counts_against_varying_limit(producer->Stage, var)) {
   /* outputs for fragment shader can't be doubles */
                  assert(producer->Stage != MESA_SHADER_FRAGMENT);
   unsigned max_output_components =
            const unsigned output_components = output_vectors * 4;
   if (output_components > max_output_components) {
      if (api == API_OPENGLES2 || prog->IsES)
      linker_error(prog, "%s shader uses too many output vectors "
               "(%u > %u)\n",
      else
      linker_error(prog, "%s shader uses too many output components "
                                          }
      static bool
   check_against_input_limit(const struct gl_constants *consts, gl_api api,
                     {
               nir_foreach_shader_in_variable(var, consumer->Program->nir) {
      if (!var->data.explicit_location &&
      var_counts_against_varying_limit(consumer->Stage, var)) {
   /* vertex inputs aren't varying counted */
                  assert(consumer->Stage != MESA_SHADER_VERTEX);
   unsigned max_input_components =
            const unsigned input_components = input_vectors * 4;
   if (input_components > max_input_components) {
      if (api == API_OPENGLES2 || prog->IsES)
      linker_error(prog, "%s shader uses too many input vectors "
               "(%u > %u)\n",
      else
      linker_error(prog, "%s shader uses too many input components "
                                          }
      /* Lower unset/unused inputs/outputs */
   static void
   remove_unused_shader_inputs_and_outputs(struct gl_shader_program *prog,
         {
      bool progress = false;
            nir_foreach_variable_with_modes_safe(var, shader, mode) {
      if (!var->data.is_xfb_only && var->data.location == -1) {
      var->data.location = 0;
   var->data.mode = nir_var_shader_temp;
                  if (progress)
      }
      static bool
   link_varyings(struct gl_shader_program *prog, unsigned first,
               {
      bool has_xfb_qualifiers = false;
   unsigned num_xfb_decls = 0;
   char **varying_names = NULL;
            if (last > MESA_SHADER_FRAGMENT)
            /* From the ARB_enhanced_layouts spec:
   *
   *    "If the shader used to record output variables for transform feedback
   *    varyings uses the "xfb_buffer", "xfb_offset", or "xfb_stride" layout
   *    qualifiers, the values specified by TransformFeedbackVaryings are
   *    ignored, and the set of variables captured for transform feedback is
   *    instead derived from the specified layout qualifiers."
   */
   for (int i = MESA_SHADER_FRAGMENT - 1; i >= 0; i--) {
      /* Find last stage before fragment shader */
   if (prog->_LinkedShaders[i]) {
      has_xfb_qualifiers =
      process_xfb_layout_qualifiers(mem_ctx, prog->_LinkedShaders[i],
                           if (!has_xfb_qualifiers) {
      num_xfb_decls = prog->TransformFeedback.NumVarying;
               if (num_xfb_decls != 0) {
      /* From GL_EXT_transform_feedback:
   *   A program will fail to link if:
   *
   *   * the <count> specified by TransformFeedbackVaryingsEXT is
   *     non-zero, but the program object has no vertex or geometry
   *     shader;
   */
   if (first >= MESA_SHADER_FRAGMENT) {
      linker_error(prog, "Transform feedback varyings specified, but "
                           xfb_decls = rzalloc_array(mem_ctx, struct xfb_decl,
         if (!parse_xfb_decls(consts, exts, prog, mem_ctx, num_xfb_decls,
                     struct gl_linked_shader *linked_shader[MESA_SHADER_STAGES];
            for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
      if (prog->_LinkedShaders[i])
               struct varying_matches vm;
   if (last < MESA_SHADER_FRAGMENT &&
      (num_xfb_decls != 0 || prog->SeparateShader)) {
      struct gl_linked_shader *producer = prog->_LinkedShaders[last];
   if (!assign_initial_varying_locations(consts, exts, mem_ctx, prog,
                        if (last <= MESA_SHADER_FRAGMENT && !prog->SeparateShader) {
      remove_unused_shader_inputs_and_outputs(prog, first, nir_var_shader_in);
               if (prog->SeparateShader) {
      struct gl_linked_shader *consumer = linked_shader[0];
   if (!assign_initial_varying_locations(consts, exts, mem_ctx, prog, NULL,
                     if (num_shaders == 1) {
      /* Linking shaders also optimizes them. Separate shaders, compute shaders
   * and shaders with a fixed-func VS or FS that don't need linking are
   * optimized here.
   */
      } else {
      /* Linking the stages in the opposite order (from fragment to vertex)
   * ensures that inter-shader outputs written to in an earlier stage
   * are eliminated if they are (transitively) not used in a later
   * stage.
   */
   for (int i = num_shaders - 2; i >= 0; i--) {
      unsigned stage_num_xfb_decls =
                  if (!assign_initial_varying_locations(consts, exts, mem_ctx, prog,
                                    /* Now that validation is done its safe to remove unused varyings. As
   * we have both a producer and consumer its safe to remove unused
   * varyings even if the program is a SSO because the stages are being
   * linked together i.e. we have a multi-stage SSO.
   */
   link_shader_opts(&vm, linked_shader[i]->Program->nir,
                  remove_unused_shader_inputs_and_outputs(prog, linked_shader[i]->Stage,
         remove_unused_shader_inputs_and_outputs(prog,
                        if (!prog->SeparateShader) {
      /* If not SSO remove unused varyings from the first/last stage */
   NIR_PASS_V(prog->_LinkedShaders[first]->Program->nir,
         NIR_PASS_V(prog->_LinkedShaders[last]->Program->nir,
      } else {
      /* Sort inputs / outputs into a canonical order.  This is necessary so
   * that inputs / outputs of separable shaders will be assigned
   * predictable locations regardless of the order in which declarations
   * appeared in the shader source.
   */
   if (first != MESA_SHADER_VERTEX) {
      canonicalize_shader_io(prog->_LinkedShaders[first]->Program->nir,
               if (last != MESA_SHADER_FRAGMENT) {
      canonicalize_shader_io(prog->_LinkedShaders[last]->Program->nir,
                  /* If there is no fragment shader we need to set transform feedback.
   *
   * For SSO we also need to assign output locations.  We assign them here
   * because we need to do it for both single stage programs and multi stage
   * programs.
   */
   if (last < MESA_SHADER_FRAGMENT &&
      (num_xfb_decls != 0 || prog->SeparateShader)) {
   const uint64_t reserved_out_slots =
         if (!assign_final_varying_locations(consts, exts, mem_ctx, prog,
                                 if (prog->SeparateShader) {
               const uint64_t reserved_slots =
            /* Assign input locations for SSO, output locations are already
   * assigned.
   */
   if (!assign_final_varying_locations(consts, exts, mem_ctx, prog,
                                             if (num_shaders == 1) {
      gl_nir_opt_dead_builtin_varyings(consts, api, prog, NULL, linked_shader[0],
         gl_nir_opt_dead_builtin_varyings(consts, api, prog, linked_shader[0], NULL,
      } else {
      /* Linking the stages in the opposite order (from fragment to vertex)
   * ensures that inter-shader outputs written to in an earlier stage
   * are eliminated if they are (transitively) not used in a later
   * stage.
   */
   int next = last;
   for (int i = next - 1; i >= 0; i--) {
                                    gl_nir_opt_dead_builtin_varyings(consts, api, prog, sh_i, sh_next,
                  const uint64_t reserved_out_slots =
                        if (!assign_final_varying_locations(consts, exts, mem_ctx, prog, sh_i,
                        /* This must be done after all dead varyings are eliminated. */
   if (sh_i != NULL) {
      unsigned slots_used = util_bitcount64(reserved_out_slots);
   if (!check_against_output_limit(consts, api, prog, sh_i, slots_used))
               unsigned slots_used = util_bitcount64(reserved_in_slots);
                                 if (!store_tfeedback_info(consts, prog, num_xfb_decls, xfb_decls,
                     }
      /**
   * Store the gl_FragDepth layout in the gl_shader_program struct.
   */
   static void
   store_fragdepth_layout(struct gl_shader_program *prog)
   {
      if (prog->_LinkedShaders[MESA_SHADER_FRAGMENT] == NULL) {
                  nir_shader *nir = prog->_LinkedShaders[MESA_SHADER_FRAGMENT]->Program->nir;
   nir_foreach_shader_out_variable(var, nir) {
      if (strcmp(var->name, "gl_FragDepth") == 0) {
      switch (var->data.depth_layout) {
   case nir_depth_layout_none:
      prog->FragDepthLayout = FRAG_DEPTH_LAYOUT_NONE;
      case nir_depth_layout_any:
      prog->FragDepthLayout = FRAG_DEPTH_LAYOUT_ANY;
      case nir_depth_layout_greater:
      prog->FragDepthLayout = FRAG_DEPTH_LAYOUT_GREATER;
      case nir_depth_layout_less:
      prog->FragDepthLayout = FRAG_DEPTH_LAYOUT_LESS;
      case nir_depth_layout_unchanged:
      prog->FragDepthLayout = FRAG_DEPTH_LAYOUT_UNCHANGED;
      default:
      assert(0);
               }
      bool
   gl_assign_attribute_or_color_locations(const struct gl_constants *consts,
         {
               if (!assign_attribute_or_color_locations(mem_ctx, prog, consts,
            ralloc_free(mem_ctx);
               if (!assign_attribute_or_color_locations(mem_ctx, prog, consts,
            ralloc_free(mem_ctx);
               ralloc_free(mem_ctx);
      }
      bool
   gl_nir_link_varyings(const struct gl_constants *consts,
               {
                                          first = MESA_SHADER_STAGES;
            /* We need to initialise the program resource list because the varying
   * packing pass my start inserting varyings onto the list.
   */
            /* Determine first and last stage. */
   for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
      if (!prog->_LinkedShaders[i])
         if (first == MESA_SHADER_STAGES)
                     bool r = link_varyings(prog, first, last, consts, exts, api, mem_ctx);
   if (r) {
      for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
                     /* Check for transform feedback varyings specified via the API */
                  /* Check for transform feedback varyings specified in the Shader */
   if (prog->last_vert_prog) {
      prog->_LinkedShaders[i]->Program->nir->info.has_transform_feedback_varyings |=
                  /* Assign NIR XFB info to the last stage before the fragment shader */
   for (int stage = MESA_SHADER_FRAGMENT - 1; stage >= 0; stage--) {
      struct gl_linked_shader *sh = prog->_LinkedShaders[stage];
   if (sh && stage != MESA_SHADER_TESS_CTRL) {
      sh->Program->nir->xfb_info =
      gl_to_nir_xfb_info(sh->Program->sh.LinkedTransformFeedback,
                        ralloc_free(mem_ctx);
      }
