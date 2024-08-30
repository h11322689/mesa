   /*
   * Copyright © 2018 Intel Corporation
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
   */
   #include <ctype.h>
      #include "glsl_types.h"
   #include "linker_util.h"
   #include "util/bitscan.h"
   #include "util/set.h"
   #include "ir_uniform.h" /* for gl_uniform_storage */
   #include "main/shader_types.h"
   #include "main/consts_exts.h"
      /**
   * Given a string identifying a program resource, break it into a base name
   * and an optional array index in square brackets.
   *
   * If an array index is present, \c out_base_name_end is set to point to the
   * "[" that precedes the array index, and the array index itself is returned
   * as a long.
   *
   * If no array index is present (or if the array index is negative or
   * mal-formed), \c out_base_name_end, is set to point to the null terminator
   * at the end of the input string, and -1 is returned.
   *
   * Only the final array index is parsed; if the string contains other array
   * indices (or structure field accesses), they are left in the base name.
   *
   * No attempt is made to check that the base name is properly formed;
   * typically the caller will look up the base name in a hash table, so
   * ill-formed base names simply turn into hash table lookup failures.
   */
   long
   link_util_parse_program_resource_name(const GLchar *name, const size_t len,
         {
      /* Section 7.3.1 ("Program Interfaces") of the OpenGL 4.3 spec says:
   *
   *     "When an integer array element or block instance number is part of
   *     the name string, it will be specified in decimal form without a "+"
   *     or "-" sign or any extra leading zeroes. Additionally, the name
   *     string will not include white space anywhere in the string."
                     if (len == 0 || name[len-1] != ']')
            /* Walk backwards over the string looking for a non-digit character.  This
   * had better be the opening bracket for an array index.
   *
   * Initially, i specifies the location of the ']'.  Since the string may
   * contain only the ']' charcater, walk backwards very carefully.
   */
   unsigned i;
   for (i = len - 1; (i > 0) && isdigit(name[i-1]); --i)
            if ((i == 0) || name[i-1] != '[')
            long array_index = strtol(&name[i], NULL, 10);
   if (array_index < 0)
            /* Check for leading zero */
   if (name[i] == '0' && name[i+1] != ']')
            *out_base_name_end = name + (i - 1);
      }
      /* Utility methods shared between the GLSL IR and the NIR */
      /* From the OpenGL 4.6 specification, 7.3.1.1 Naming Active Resources:
   *
   *    "For an active shader storage block member declared as an array of an
   *     aggregate type, an entry will be generated only for the first array
   *     element, regardless of its type. Such block members are referred to as
   *     top-level arrays. If the block member is an aggregate type, the
   *     enumeration rules are then applied recursively."
   */
   bool
   link_util_should_add_buffer_variable(struct gl_shader_program *prog,
                                 {
      /* If the uniform is not a shader storage buffer or is not an array return
   * true.
   */
   if (!uniform->is_shader_storage || top_level_array_size_in_bytes == 0)
            int after_top_level_array = top_level_array_base_offset +
            /* Check for a new block, or that we are not dealing with array elements of
   * a top member array other than the first element.
   */
   if (block_index != uniform->block_index ||
      uniform->offset >= after_top_level_array ||
   uniform->offset < second_element_offset) {
                  }
      bool
   link_util_add_program_resource(struct gl_shader_program *prog,
               {
               /* If resource already exists, do not add it again. */
   if (_mesa_set_search(resource_set, data))
            prog->data->ProgramResourceList =
      reralloc(prog->data,
                     if (!prog->data->ProgramResourceList) {
      linker_error(prog, "Out of memory during linking.\n");
               struct gl_program_resource *res =
            res->Type = type;
   res->Data = data;
                                 }
      /**
   * Search through the list of empty blocks to find one that fits the current
   * uniform.
   */
   int
   link_util_find_empty_block(struct gl_shader_program *prog,
         {
               foreach_list_typed(struct empty_uniform_block, block, link,
            /* Found a block with enough slots to fit the uniform */
   if (block->slots == entries) {
      unsigned start = block->start;
                     /* Found a block with more slots than needed. It can still be used. */
   } else if (block->slots > entries) {
      unsigned start = block->start;
                                    }
      void
   link_util_update_empty_uniform_locations(struct gl_shader_program *prog)
   {
               for (unsigned i = 0; i < prog->NumUniformRemapTable; i++) {
      /* We found empty space in UniformRemapTable. */
   if (prog->UniformRemapTable[i] == NULL) {
      /* We've found the beginning of a new continous block of empty slots */
   if (!current_block || current_block->start + current_block->slots != i) {
      current_block = rzalloc(prog, struct empty_uniform_block);
   current_block->start = i;
   exec_list_push_tail(&prog->EmptyUniformLocations,
               /* The current block continues, so we simply increment its slots */
            }
      void
   link_util_check_subroutine_resources(struct gl_shader_program *prog)
   {
      unsigned mask = prog->data->linked_stages;
   while (mask) {
      const int i = u_bit_scan(&mask);
            if (p->sh.NumSubroutineUniformRemapTable > MAX_SUBROUTINE_UNIFORM_LOCATIONS) {
      linker_error(prog, "Too many %s shader subroutine uniforms\n",
            }
      /**
   * Validate uniform resources used by a program versus the implementation limits
   */
   void
   link_util_check_uniform_resources(const struct gl_constants *consts,
         {
      unsigned total_uniform_blocks = 0;
            for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
               if (sh == NULL)
            if (sh->num_uniform_components >
      consts->Program[i].MaxUniformComponents) {
   if (consts->GLSLSkipStrictMaxUniformLimitCheck) {
      linker_warning(prog, "Too many %s shader default uniform block "
                  "components, but the driver will try to optimize "
   } else {
      linker_error(prog, "Too many %s shader default uniform block "
                        if (sh->num_combined_uniform_components >
      consts->Program[i].MaxCombinedUniformComponents) {
   if (consts->GLSLSkipStrictMaxUniformLimitCheck) {
      linker_warning(prog, "Too many %s shader uniform components, "
                  } else {
      linker_error(prog, "Too many %s shader uniform components\n",
                  total_shader_storage_blocks += sh->Program->info.num_ssbos;
               if (total_uniform_blocks > consts->MaxCombinedUniformBlocks) {
      linker_error(prog, "Too many combined uniform blocks (%d/%d)\n",
               if (total_shader_storage_blocks > consts->MaxCombinedShaderStorageBlocks) {
      linker_error(prog, "Too many combined shader storage blocks (%d/%d)\n",
                     for (unsigned i = 0; i < prog->data->NumUniformBlocks; i++) {
      if (prog->data->UniformBlocks[i].UniformBufferSize >
      consts->MaxUniformBlockSize) {
   linker_error(prog, "Uniform block %s too big (%d/%d)\n",
                              for (unsigned i = 0; i < prog->data->NumShaderStorageBlocks; i++) {
      if (prog->data->ShaderStorageBlocks[i].UniformBufferSize >
      consts->MaxShaderStorageBlockSize) {
   linker_error(prog, "Shader storage block %s too big (%d/%d)\n",
                        }
      void
   link_util_calculate_subroutine_compat(struct gl_shader_program *prog)
   {
      unsigned mask = prog->data->linked_stages;
   while (mask) {
      const int i = u_bit_scan(&mask);
            for (unsigned j = 0; j < p->sh.NumSubroutineUniformRemapTable; j++) {
                                             int count = 0;
   if (p->sh.NumSubroutineFunctions == 0) {
      linker_error(prog, "subroutine uniform %s defined but no valid functions found\n", glsl_get_type_name(uni->type));
      }
   for (unsigned f = 0; f < p->sh.NumSubroutineFunctions; f++) {
      struct gl_subroutine_function *fn = &p->sh.SubroutineFunctions[f];
   for (int k = 0; k < fn->num_compat_types; k++) {
      if (fn->types[k] == uni->type) {
      count++;
            }
            }
      /**
   * Recursive part of the public mark_array_elements_referenced function.
   *
   * The recursion occurs when an entire array-of- is accessed.  See the
   * implementation for more details.
   *
   * \param dr                List of array_deref_range elements to be
   *                          processed.
   * \param count             Number of array_deref_range elements to be
   *                          processed.
   * \param scale             Current offset scale.
   * \param linearized_index  Current accumulated linearized array index.
   */
   void
   _mark_array_elements_referenced(const struct array_deref_range *dr,
                     {
      /* Walk through the list of array dereferences in least- to
   * most-significant order.  Along the way, accumulate the current
   * linearized offset and the scale factor for each array-of-.
   */
   for (unsigned i = 0; i < count; i++) {
      if (dr[i].index < dr[i].size) {
      linearized_index += dr[i].index * scale;
      } else {
      /* For each element in the current array, update the count and
   * offset, then recurse to process the remaining arrays.
   *
   * There is some inefficency here if the last eBITSET_WORD *bitslement in the
   * array_deref_range list specifies the entire array.  In that case,
   * the loop will make recursive calls with count == 0.  In the call,
   * all that will happen is the bit will be set.
   */
   for (unsigned j = 0; j < dr[i].size; j++) {
      _mark_array_elements_referenced(&dr[i + 1],
                                                   }
      /**
   * Mark a set of array elements as accessed.
   *
   * If every \c array_deref_range is for a single index, only a single
   * element will be marked.  If any \c array_deref_range is for an entire
   * array-of-, then multiple elements will be marked.
   *
   * Items in the \c array_deref_range list appear in least- to
   * most-significant order.  This is the \b opposite order the indices
   * appear in the GLSL shader text.  An array access like
   *
   *     x = y[1][i][3];
   *
   * would appear as
   *
   *     { { 3, n }, { m, m }, { 1, p } }
   *
   * where n, m, and p are the sizes of the arrays-of-arrays.
   *
   * The set of marked array elements can later be queried by
   * \c ::is_linearized_index_referenced.
   *
   * \param dr     List of array_deref_range elements to be processed.
   * \param count  Number of array_deref_range elements to be processed.
   */
   void
   link_util_mark_array_elements_referenced(const struct array_deref_range *dr,
               {
      if (count != array_depth)
               }
      const char *
   interpolation_string(unsigned interpolation)
   {
      switch (interpolation) {
   case INTERP_MODE_NONE:          return "no";
   case INTERP_MODE_SMOOTH:        return "smooth";
   case INTERP_MODE_FLAT:          return "flat";
   case INTERP_MODE_NOPERSPECTIVE: return "noperspective";
            assert(!"Should not get here.");
      }
