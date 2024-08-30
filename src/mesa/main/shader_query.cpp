   /*
   * Copyright © 2011 Intel Corporation
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
   * \file shader_query.cpp
   * C-to-C++ bridge functions to query GLSL shader data
   *
   * \author Ian Romanick <ian.d.romanick@intel.com>
   */
      #include "main/context.h"
   #include "main/enums.h"
   #include "main/shaderapi.h"
   #include "main/shaderobj.h"
   #include "main/uniforms.h"
   #include "compiler/glsl/glsl_symbol_table.h"
   #include "compiler/glsl/ir.h"
   #include "compiler/glsl/linker_util.h"
   #include "compiler/glsl/string_to_uint_map.h"
   #include "util/mesa-sha1.h"
   #include "c99_alloca.h"
   #include "api_exec_decl.h"
      static GLint
   program_resource_location(struct gl_program_resource *res,
            /**
   * Declare convenience functions to return resource data in a given type.
   * Warning! this is not type safe so be *very* careful when using these.
   */
   #define DECL_RESOURCE_FUNC(name, type) \
   const type * RESOURCE_ ## name (gl_program_resource *res) { \
      assert(res->Data); \
      }
      DECL_RESOURCE_FUNC(VAR, gl_shader_variable);
   DECL_RESOURCE_FUNC(UBO, gl_uniform_block);
   DECL_RESOURCE_FUNC(UNI, gl_uniform_storage);
   DECL_RESOURCE_FUNC(ATC, gl_active_atomic_buffer);
   DECL_RESOURCE_FUNC(XFV, gl_transform_feedback_varying_info);
   DECL_RESOURCE_FUNC(XFB, gl_transform_feedback_buffer);
   DECL_RESOURCE_FUNC(SUB, gl_subroutine_function);
      static GLenum
   mediump_to_highp_type(GLenum type)
   {
      switch (type) {
   case GL_FLOAT16_NV:
         case GL_FLOAT16_VEC2_NV:
         case GL_FLOAT16_VEC3_NV:
         case GL_FLOAT16_VEC4_NV:
         case GL_FLOAT16_MAT2_AMD:
         case GL_FLOAT16_MAT3_AMD:
         case GL_FLOAT16_MAT4_AMD:
         case GL_FLOAT16_MAT2x3_AMD:
         case GL_FLOAT16_MAT2x4_AMD:
         case GL_FLOAT16_MAT3x2_AMD:
         case GL_FLOAT16_MAT3x4_AMD:
         case GL_FLOAT16_MAT4x2_AMD:
         case GL_FLOAT16_MAT4x3_AMD:
         default:
            }
      static void
   bind_attrib_location(struct gl_context *ctx,
               {
      if (!name)
            if (!no_error) {
      if (strncmp(name, "gl_", 3) == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     if (index >= ctx->Const.Program[MESA_SHADER_VERTEX].MaxAttribs) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindAttribLocation(%u >= %u)",
                        /* Replace the current value if it's already in the list.  Add
   * VERT_ATTRIB_GENERIC0 because that's how the linker differentiates
   * between built-in attributes and user-defined attributes.
   */
            /*
   * Note that this attribute binding won't go into effect until
   * glLinkProgram is called again.
      }
      void GLAPIENTRY
   _mesa_BindAttribLocation_no_error(GLuint program, GLuint index,
         {
               struct gl_shader_program *const shProg =
            }
      void GLAPIENTRY
   _mesa_BindAttribLocation(GLuint program, GLuint index,
         {
               struct gl_shader_program *const shProg =
         if (!shProg)
               }
      void GLAPIENTRY
   _mesa_GetActiveAttrib(GLuint program, GLuint desired_index,
               {
      GET_CURRENT_CONTEXT(ctx);
            if (maxLength < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetActiveAttrib(maxLength < 0)");
               shProg = _mesa_lookup_shader_program_err(ctx, program, "glGetActiveAttrib");
   if (!shProg)
            if (!shProg->data->LinkStatus) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                     if (shProg->_LinkedShaders[MESA_SHADER_VERTEX] == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetActiveAttrib(no vertex shader)");
               struct gl_program_resource *res =
      _mesa_program_resource_find_index(shProg, GL_PROGRAM_INPUT,
         /* User asked for index that does not exist. */
   if (!res) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetActiveAttrib(index)");
                                          if (size)
      _mesa_program_resource_prop(shProg, res, desired_index, GL_ARRAY_SIZE,
         if (type)
      _mesa_program_resource_prop(shProg, res, desired_index, GL_TYPE,
   }
      GLint GLAPIENTRY
   _mesa_GetAttribLocation(GLuint program, const GLchar * name)
   {
      GET_CURRENT_CONTEXT(ctx);
   struct gl_shader_program *const shProg =
            if (!shProg) {
                  if (!shProg->data->LinkStatus) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     if (!name)
            /* Not having a vertex shader is not an error.
   */
   if (shProg->_LinkedShaders[MESA_SHADER_VERTEX] == NULL)
            unsigned array_index = 0;
   struct gl_program_resource *res =
      _mesa_program_resource_find_name(shProg, GL_PROGRAM_INPUT, name,
         if (!res)
               }
      unsigned
   _mesa_count_active_attribs(struct gl_shader_program *shProg)
   {
      if (!shProg->data->LinkStatus
      || shProg->_LinkedShaders[MESA_SHADER_VERTEX] == NULL) {
               struct gl_program_resource *res = shProg->data->ProgramResourceList;
   unsigned count = 0;
   for (unsigned j = 0; j < shProg->data->NumProgramResourceList;
      j++, res++) {
   if (res->Type == GL_PROGRAM_INPUT &&
      res->StageReferences & (1 << MESA_SHADER_VERTEX))
   }
      }
         size_t
   _mesa_longest_attribute_name_length(struct gl_shader_program *shProg)
   {
      if (!shProg->data->LinkStatus
      || shProg->_LinkedShaders[MESA_SHADER_VERTEX] == NULL) {
               struct gl_program_resource *res = shProg->data->ProgramResourceList;
   size_t longest = 0;
   for (unsigned j = 0; j < shProg->data->NumProgramResourceList;
      j++, res++) {
   if (res->Type == GL_PROGRAM_INPUT &&
               /* From the ARB_gl_spirv spec:
   *
   *   "If pname is ACTIVE_ATTRIBUTE_MAX_LENGTH, the length of the
   *    longest active attribute name, including a null terminator, is
   *    returned.  If no active attributes exist, zero is returned. If
   *    no name reflection information is available, one is returned."
                  if (length >= longest)
                     }
      void static
   bind_frag_data_location(struct gl_shader_program *const shProg,
               {
      /* Replace the current value if it's already in the list.  Add
   * FRAG_RESULT_DATA0 because that's how the linker differentiates
   * between built-in attributes and user-defined attributes.
   */
   shProg->FragDataBindings->put(colorNumber + FRAG_RESULT_DATA0, name);
            /*
   * Note that this binding won't go into effect until
   * glLinkProgram is called again.
      }
      void GLAPIENTRY
   _mesa_BindFragDataLocation(GLuint program, GLuint colorNumber,
         {
         }
      void GLAPIENTRY
   _mesa_BindFragDataLocation_no_error(GLuint program, GLuint colorNumber,
         {
               if (!name)
            struct gl_shader_program *const shProg =
               }
      void GLAPIENTRY
   _mesa_BindFragDataLocationIndexed(GLuint program, GLuint colorNumber,
         {
               struct gl_shader_program *const shProg =
         if (!shProg)
            if (!name)
            if (strncmp(name, "gl_", 3) == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glBindFragDataLocationIndexed(illegal name)");
               if (index > 1) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindFragDataLocationIndexed(index)");
               if (index == 0 && colorNumber >= ctx->Const.MaxDrawBuffers) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindFragDataLocationIndexed(colorNumber)");
               if (index == 1 && colorNumber >= ctx->Const.MaxDualSourceDrawBuffers) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glBindFragDataLocationIndexed(colorNumber)");
                  }
      void GLAPIENTRY
   _mesa_BindFragDataLocationIndexed_no_error(GLuint program, GLuint colorNumber,
         {
               if (!name)
            struct gl_shader_program *const shProg =
               }
      GLint GLAPIENTRY
   _mesa_GetFragDataIndex(GLuint program, const GLchar *name)
   {
      GET_CURRENT_CONTEXT(ctx);
   struct gl_shader_program *const shProg =
            if (!shProg) {
                  if (!shProg->data->LinkStatus) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     if (!name)
            /* Not having a fragment shader is not an error.
   */
   if (shProg->_LinkedShaders[MESA_SHADER_FRAGMENT] == NULL)
            return _mesa_program_resource_location_index(shProg, GL_PROGRAM_OUTPUT,
      }
      GLint GLAPIENTRY
   _mesa_GetFragDataLocation(GLuint program, const GLchar *name)
   {
      GET_CURRENT_CONTEXT(ctx);
   struct gl_shader_program *const shProg =
            if (!shProg) {
                  if (!shProg->data->LinkStatus) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     if (!name)
            /* Not having a fragment shader is not an error.
   */
   if (shProg->_LinkedShaders[MESA_SHADER_FRAGMENT] == NULL)
            unsigned array_index = 0;
   struct gl_program_resource *res =
      _mesa_program_resource_find_name(shProg, GL_PROGRAM_OUTPUT, name,
         if (!res)
               }
      const char*
   _mesa_program_resource_name(struct gl_program_resource *res)
   {
      switch (res->Type) {
   case GL_UNIFORM_BLOCK:
   case GL_SHADER_STORAGE_BLOCK:
         case GL_TRANSFORM_FEEDBACK_VARYING:
         case GL_PROGRAM_INPUT:
   case GL_PROGRAM_OUTPUT:
         case GL_UNIFORM:
   case GL_BUFFER_VARIABLE:
         case GL_VERTEX_SUBROUTINE_UNIFORM:
   case GL_GEOMETRY_SUBROUTINE_UNIFORM:
   case GL_FRAGMENT_SUBROUTINE_UNIFORM:
   case GL_COMPUTE_SUBROUTINE_UNIFORM:
   case GL_TESS_CONTROL_SUBROUTINE_UNIFORM:
   case GL_TESS_EVALUATION_SUBROUTINE_UNIFORM:
         case GL_VERTEX_SUBROUTINE:
   case GL_GEOMETRY_SUBROUTINE:
   case GL_FRAGMENT_SUBROUTINE:
   case GL_COMPUTE_SUBROUTINE:
   case GL_TESS_CONTROL_SUBROUTINE:
   case GL_TESS_EVALUATION_SUBROUTINE:
         default:
         }
      }
      int
   _mesa_program_resource_name_length(struct gl_program_resource *res)
   {
      switch (res->Type) {
   case GL_UNIFORM_BLOCK:
   case GL_SHADER_STORAGE_BLOCK:
         case GL_TRANSFORM_FEEDBACK_VARYING:
         case GL_PROGRAM_INPUT:
   case GL_PROGRAM_OUTPUT:
         case GL_UNIFORM:
   case GL_BUFFER_VARIABLE:
         case GL_VERTEX_SUBROUTINE_UNIFORM:
   case GL_GEOMETRY_SUBROUTINE_UNIFORM:
   case GL_FRAGMENT_SUBROUTINE_UNIFORM:
   case GL_COMPUTE_SUBROUTINE_UNIFORM:
   case GL_TESS_CONTROL_SUBROUTINE_UNIFORM:
   case GL_TESS_EVALUATION_SUBROUTINE_UNIFORM:
         case GL_VERTEX_SUBROUTINE:
   case GL_GEOMETRY_SUBROUTINE:
   case GL_FRAGMENT_SUBROUTINE:
   case GL_COMPUTE_SUBROUTINE:
   case GL_TESS_CONTROL_SUBROUTINE:
   case GL_TESS_EVALUATION_SUBROUTINE:
         default:
         }
      }
      bool
   _mesa_program_get_resource_name(struct gl_program_resource *res,
         {
      switch (res->Type) {
   case GL_UNIFORM_BLOCK:
   case GL_SHADER_STORAGE_BLOCK:
      *out = RESOURCE_UBO(res)->name;
      case GL_TRANSFORM_FEEDBACK_VARYING:
      *out = RESOURCE_XFV(res)->name;
      case GL_PROGRAM_INPUT:
   case GL_PROGRAM_OUTPUT:
      *out = RESOURCE_VAR(res)->name;
      case GL_UNIFORM:
   case GL_BUFFER_VARIABLE:
      *out = RESOURCE_UNI(res)->name;
      case GL_VERTEX_SUBROUTINE_UNIFORM:
   case GL_GEOMETRY_SUBROUTINE_UNIFORM:
   case GL_FRAGMENT_SUBROUTINE_UNIFORM:
   case GL_COMPUTE_SUBROUTINE_UNIFORM:
   case GL_TESS_CONTROL_SUBROUTINE_UNIFORM:
   case GL_TESS_EVALUATION_SUBROUTINE_UNIFORM:
      *out = RESOURCE_UNI(res)->name;
   out->string += MESA_SUBROUTINE_PREFIX_LEN;
   out->length -= MESA_SUBROUTINE_PREFIX_LEN;
   assert(out->string); /* always non-NULL */
      case GL_VERTEX_SUBROUTINE:
   case GL_GEOMETRY_SUBROUTINE:
   case GL_FRAGMENT_SUBROUTINE:
   case GL_COMPUTE_SUBROUTINE:
   case GL_TESS_CONTROL_SUBROUTINE:
   case GL_TESS_EVALUATION_SUBROUTINE:
      *out = RESOURCE_SUB(res)->name;
      default:
            }
      unsigned
   _mesa_program_resource_array_size(struct gl_program_resource *res)
   {
      switch (res->Type) {
   case GL_TRANSFORM_FEEDBACK_VARYING:
      return RESOURCE_XFV(res)->Size > 1 ?
      case GL_PROGRAM_INPUT:
   case GL_PROGRAM_OUTPUT:
         case GL_UNIFORM:
   case GL_VERTEX_SUBROUTINE_UNIFORM:
   case GL_GEOMETRY_SUBROUTINE_UNIFORM:
   case GL_FRAGMENT_SUBROUTINE_UNIFORM:
   case GL_COMPUTE_SUBROUTINE_UNIFORM:
   case GL_TESS_CONTROL_SUBROUTINE_UNIFORM:
   case GL_TESS_EVALUATION_SUBROUTINE_UNIFORM:
         case GL_BUFFER_VARIABLE:
      /* Unsized arrays */
   if (RESOURCE_UNI(res)->array_stride > 0 &&
      RESOURCE_UNI(res)->array_elements == 0)
      else
      case GL_VERTEX_SUBROUTINE:
   case GL_GEOMETRY_SUBROUTINE:
   case GL_FRAGMENT_SUBROUTINE:
   case GL_COMPUTE_SUBROUTINE:
   case GL_TESS_CONTROL_SUBROUTINE:
   case GL_TESS_EVALUATION_SUBROUTINE:
   case GL_ATOMIC_COUNTER_BUFFER:
   case GL_UNIFORM_BLOCK:
   case GL_SHADER_STORAGE_BLOCK:
         default:
         }
      }
      /**
   * Checks if array subscript is valid and if so sets array_index.
   */
   static bool
   valid_array_index(const GLchar *name, int len, unsigned *array_index)
   {
      long idx = 0;
            idx = link_util_parse_program_resource_name(name, len, &out_base_name_end);
   if (idx < 0)
            if (array_index)
               }
      static struct gl_program_resource *
   search_resource_hash(struct gl_shader_program *shProg,
               {
      unsigned type = GET_PROGRAM_RESOURCE_TYPE_FROM_GLENUM(programInterface);
            if (!shProg->data->ProgramResourceHash[type])
            const char *base_name_end;
   long index = link_util_parse_program_resource_name(name, len, &base_name_end);
            /* If dealing with array, we need to get the basename. */
   if (index >= 0) {
      name_copy = (char *) alloca(base_name_end - name + 1);
   memcpy(name_copy, name, base_name_end - name);
   name_copy[base_name_end - name] = '\0';
      } else {
                  uint32_t hash = _mesa_hash_string_with_length(name_copy, len);
   struct hash_entry *entry =
      _mesa_hash_table_search_pre_hashed(shProg->data->ProgramResourceHash[type],
      if (!entry)
            if (array_index)
               }
      /* Find a program resource with specific name in given interface.
   */
   struct gl_program_resource *
   _mesa_program_resource_find_name(struct gl_shader_program *shProg,
               {
      if (name == NULL)
                     /* If we have a name, try the ProgramResourceHash first. */
   struct gl_program_resource *res =
            if (res)
            res = shProg->data->ProgramResourceList;
   for (unsigned i = 0; i < shProg->data->NumProgramResourceList; i++, res++) {
      if (res->Type != programInterface)
                     /* Since ARB_gl_spirv lack of name reflections is a possibility */
   if (!_mesa_program_get_resource_name(res, &rname))
                     /* From ARB_program_interface_query spec:
   *
   * "uint GetProgramResourceIndex(uint program, enum programInterface,
   *                               const char *name);
   *  [...]
   *  If <name> exactly matches the name string of one of the active
   *  resources for <programInterface>, the index of the matched resource is
   *  returned. Additionally, if <name> would exactly match the name string
   *  of an active resource if "[0]" were appended to <name>, the index of
   *  the matched resource is returned. [...]"
   *
   * "A string provided to GetProgramResourceLocation or
   * GetProgramResourceLocationIndex is considered to match an active variable
   * if:
   *
   *  * the string exactly matches the name of the active variable;
   *
   *  * if the string identifies the base name of an active array, where the
   *    string would exactly match the name of the variable if the suffix
   *    "[0]" were appended to the string; [...]"
   */
   /* Remove array's index from interface block name comparison only if
   * array's index is zero and the resulting string length is the same
   * than the provided name's length.
   */
   int length_without_array_index =
         bool rname_has_array_index_zero = rname.suffix_is_zero_square_bracketed &&
            if (len >= rname.length && strncmp(rname.string, name, rname.length) == 0)
         else if (rname_has_array_index_zero &&
                  if (found) {
      switch (programInterface) {
   case GL_UNIFORM_BLOCK:
   case GL_SHADER_STORAGE_BLOCK:
      /* Basename match, check if array or struct. */
   if (rname_has_array_index_zero ||
      name[rname.length] == '\0' ||
   name[rname.length] == '[' ||
   name[rname.length] == '.') {
      }
      case GL_TRANSFORM_FEEDBACK_VARYING:
   case GL_BUFFER_VARIABLE:
   case GL_UNIFORM:
   case GL_VERTEX_SUBROUTINE_UNIFORM:
   case GL_GEOMETRY_SUBROUTINE_UNIFORM:
   case GL_FRAGMENT_SUBROUTINE_UNIFORM:
   case GL_COMPUTE_SUBROUTINE_UNIFORM:
   case GL_TESS_CONTROL_SUBROUTINE_UNIFORM:
   case GL_TESS_EVALUATION_SUBROUTINE_UNIFORM:
   case GL_VERTEX_SUBROUTINE:
   case GL_GEOMETRY_SUBROUTINE:
   case GL_FRAGMENT_SUBROUTINE:
   case GL_COMPUTE_SUBROUTINE:
   case GL_TESS_CONTROL_SUBROUTINE:
   case GL_TESS_EVALUATION_SUBROUTINE:
      if (name[rname.length] == '.') {
         }
      case GL_PROGRAM_INPUT:
   case GL_PROGRAM_OUTPUT:
      if (name[rname.length] == '\0') {
         } else if (name[rname.length] == '[' &&
      valid_array_index(name, len, array_index)) {
      }
      default:
               }
      }
      /* Find an uniform or buffer variable program resource with an specific offset
   * inside a block with an specific binding.
   *
   * Valid interfaces are GL_BUFFER_VARIABLE and GL_UNIFORM.
   */
   static struct gl_program_resource *
   program_resource_find_binding_offset(struct gl_shader_program *shProg,
                     {
         /* First we need to get the BLOCK_INDEX from the BUFFER_BINDING */
            switch (programInterface) {
   case GL_BUFFER_VARIABLE:
      blockInterface = GL_SHADER_STORAGE_BLOCK;
      case GL_UNIFORM:
      blockInterface = GL_UNIFORM_BLOCK;
      default:
      assert("Invalid program interface");
               int block_index = -1;
   int starting_index = -1;
            /* Blocks are added to the resource list in the same order that they are
   * added to UniformBlocks/ShaderStorageBlocks. Furthermore, all the blocks
   * of each type (UBO/SSBO) are contiguous, so we can infer block_index from
   * the resource list.
   */
   for (unsigned i = 0; i < shProg->data->NumProgramResourceList; i++, res++) {
      if (res->Type != blockInterface)
            /* Store the first index where a resource of the specific interface is. */
   if (starting_index == -1)
                     if (block->Binding == binding) {
      /* For arrays, or arrays of arrays of blocks, we want the resource
   * for the block with base index. Most properties for members of each
   * block are inherited from the block with the base index, including
   * a uniform being active or not.
   */
   block_index = i - starting_index - block->linearized_array_index;
                  if (block_index == -1)
            /* We now look for the resource corresponding to the uniform or buffer
   * variable using the BLOCK_INDEX and OFFSET.
   */
   res = shProg->data->ProgramResourceList;
   for (unsigned i = 0; i < shProg->data->NumProgramResourceList; i++, res++) {
      if (res->Type != programInterface)
                     if (uniform->block_index == block_index && uniform->offset == offset) {
                        }
      /* Checks if an uniform or buffer variable is in the active program resource
   * list.
   *
   * It takes into accout that for variables coming from SPIR-V binaries their
   * names could not be available (ARB_gl_spirv). In that case, it will use the
   * the offset and the block binding to locate the resource.
   *
   * Valid interfaces are GL_BUFFER_VARIABLE and GL_UNIFORM.
   */
   struct gl_program_resource *
   _mesa_program_resource_find_active_variable(struct gl_shader_program *shProg,
                     {
      struct gl_program_resource *res;
            assert(programInterface == GL_UNIFORM ||
            if (uni.IndexName) {
      res = _mesa_program_resource_find_name(shProg, programInterface, uni.IndexName,
      } else {
      /* As the resource has no associated name (ARB_gl_spirv),
   * we can use the UBO/SSBO binding and offset to find it.
   */
   res = program_resource_find_binding_offset(shProg, programInterface,
                  }
      static GLuint
   calc_resource_index(struct gl_shader_program *shProg,
         {
      unsigned i;
   GLuint index = 0;
   for (i = 0; i < shProg->data->NumProgramResourceList; i++) {
      if (&shProg->data->ProgramResourceList[i] == res)
         if (shProg->data->ProgramResourceList[i].Type == res->Type)
      }
      }
      /**
   * Calculate index for the given resource.
   */
   GLuint
   _mesa_program_resource_index(struct gl_shader_program *shProg,
         {
      if (!res)
            switch (res->Type) {
   case GL_ATOMIC_COUNTER_BUFFER:
         case GL_VERTEX_SUBROUTINE:
   case GL_GEOMETRY_SUBROUTINE:
   case GL_FRAGMENT_SUBROUTINE:
   case GL_COMPUTE_SUBROUTINE:
   case GL_TESS_CONTROL_SUBROUTINE:
   case GL_TESS_EVALUATION_SUBROUTINE:
         case GL_UNIFORM_BLOCK:
   case GL_SHADER_STORAGE_BLOCK:
   case GL_TRANSFORM_FEEDBACK_BUFFER:
   case GL_TRANSFORM_FEEDBACK_VARYING:
   default:
            }
      /**
   * Find a program resource that points to given data.
   */
   static struct gl_program_resource*
   program_resource_find_data(struct gl_shader_program *shProg, void *data)
   {
      struct gl_program_resource *res = shProg->data->ProgramResourceList;
   for (unsigned i = 0; i < shProg->data->NumProgramResourceList;
      i++, res++) {
   if (res->Data == data)
      }
      }
      /* Find a program resource with specific index in given interface.
   */
   struct gl_program_resource *
   _mesa_program_resource_find_index(struct gl_shader_program *shProg,
         {
      struct gl_program_resource *res = shProg->data->ProgramResourceList;
            for (unsigned i = 0; i < shProg->data->NumProgramResourceList;
      i++, res++) {
   if (res->Type != programInterface)
            switch (res->Type) {
   case GL_UNIFORM_BLOCK:
   case GL_ATOMIC_COUNTER_BUFFER:
   case GL_SHADER_STORAGE_BLOCK:
   case GL_TRANSFORM_FEEDBACK_BUFFER:
      if (_mesa_program_resource_index(shProg, res) == index)
            case GL_TRANSFORM_FEEDBACK_VARYING:
   case GL_PROGRAM_INPUT:
   case GL_PROGRAM_OUTPUT:
   case GL_UNIFORM:
   case GL_VERTEX_SUBROUTINE_UNIFORM:
   case GL_GEOMETRY_SUBROUTINE_UNIFORM:
   case GL_FRAGMENT_SUBROUTINE_UNIFORM:
   case GL_COMPUTE_SUBROUTINE_UNIFORM:
   case GL_TESS_CONTROL_SUBROUTINE_UNIFORM:
   case GL_TESS_EVALUATION_SUBROUTINE_UNIFORM:
   case GL_VERTEX_SUBROUTINE:
   case GL_GEOMETRY_SUBROUTINE:
   case GL_FRAGMENT_SUBROUTINE:
   case GL_COMPUTE_SUBROUTINE:
   case GL_TESS_CONTROL_SUBROUTINE:
   case GL_TESS_EVALUATION_SUBROUTINE:
   case GL_BUFFER_VARIABLE:
      if (++idx == (int) index)
            default:
            }
      }
      /* Function returns if resource name is expected to have index
   * appended into it.
   *
   *
   * Page 61 (page 73 of the PDF) in section 2.11 of the OpenGL ES 3.0
   * spec says:
   *
   *     "If the active uniform is an array, the uniform name returned in
   *     name will always be the name of the uniform array appended with
   *     "[0]"."
   *
   * The same text also appears in the OpenGL 4.2 spec.  It does not,
   * however, appear in any previous spec.  Previous specifications are
   * ambiguous in this regard.  However, either name can later be passed
   * to glGetUniformLocation (and related APIs), so there shouldn't be any
   * harm in always appending "[0]" to uniform array names.
   */
   static bool
   add_index_to_name(struct gl_program_resource *res)
   {
      /* Transform feedback varyings have array index already appended
   * in their names.
   */
      }
      /* Get name length of a program resource. This consists of
   * base name + 3 for '[0]' if resource is an array.
   */
   extern unsigned
   _mesa_program_resource_name_length_array(struct gl_program_resource *res)
   {
               /* For shaders constructed from SPIR-V binaries, variables may not
   * have names associated with them.
   */
   if (!length)
            if (_mesa_program_resource_array_size(res) && add_index_to_name(res))
            }
      /* Get full name of a program resource.
   */
   bool
   _mesa_get_program_resource_name(struct gl_shader_program *shProg,
                           {
               /* Find resource with given interface and index. */
   struct gl_program_resource *res =
            /* The error INVALID_VALUE is generated if <index> is greater than
   * or equal to the number of entries in the active resource list for
   * <programInterface>.
   */
   if (!res) {
      _mesa_error_glthread_safe(ctx, GL_INVALID_VALUE, glthread,
                     if (bufSize < 0) {
      _mesa_error_glthread_safe(ctx, GL_INVALID_VALUE, glthread,
                              if (length == NULL)
                     /* The resource name can be NULL for shaders constructed from SPIR-V
   * binaries. In that case, we do not add the '[0]'.
   */
   if (name && name[0] != '\0' &&
      _mesa_program_resource_array_size(res) && add_index_to_name(res)) {
            /* The comparison is strange because *length does *NOT* include the
   * terminating NUL, but maxLength does.
   */
   for (i = 0; i < 3 && (*length + i + 1) < bufSize; i++)
            name[*length + i] = '\0';
      }
      }
      static GLint
   program_resource_location(struct gl_program_resource *res, unsigned array_index)
   {
      switch (res->Type) {
   case GL_PROGRAM_INPUT: {
               if (var->location == -1)
            /* If the input is an array, fail if the index is out of bounds. */
   if (array_index > 0
      && array_index >= var->type->length) {
      }
   return var->location +
      }
   case GL_PROGRAM_OUTPUT:
      if (RESOURCE_VAR(res)->location == -1)
            /* If the output is an array, fail if the index is out of bounds. */
   if (array_index > 0
      && array_index >= RESOURCE_VAR(res)->type->length) {
      }
      case GL_UNIFORM:
      /* If the uniform is built-in, fail. */
   if (RESOURCE_UNI(res)->builtin)
         /* From page 79 of the OpenGL 4.2 spec:
      *
   *     "A valid name cannot be a structure, an array of structures, or any
   *     portion of a single vector or a matrix."
   */
   if (RESOURCE_UNI(res)->type->without_array()->is_struct())
            /* From the GL_ARB_uniform_buffer_object spec:
   *
   *     "The value -1 will be returned if <name> does not correspond to an
   *     active uniform variable name in <program>, if <name> is associated
   *     with a named uniform block, or if <name> starts with the reserved
   *     prefix "gl_"."
   */
   if (RESOURCE_UNI(res)->block_index != -1 ||
                     case GL_VERTEX_SUBROUTINE_UNIFORM:
   case GL_GEOMETRY_SUBROUTINE_UNIFORM:
   case GL_FRAGMENT_SUBROUTINE_UNIFORM:
   case GL_COMPUTE_SUBROUTINE_UNIFORM:
   case GL_TESS_CONTROL_SUBROUTINE_UNIFORM:
   case GL_TESS_EVALUATION_SUBROUTINE_UNIFORM:
      /* If the uniform is an array, fail if the index is out of bounds. */
   if (array_index > 0
      && array_index >= RESOURCE_UNI(res)->array_elements) {
               /* location in remap table + array element offset */
      default:
            }
      /**
   * Function implements following location queries:
   *    glGetUniformLocation
   */
   GLint
   _mesa_program_resource_location(struct gl_shader_program *shProg,
         {
      unsigned array_index = 0;
   struct gl_program_resource *res =
      _mesa_program_resource_find_name(shProg, programInterface, name,
         /* Resource not found. */
   if (!res)
               }
      static GLint
   _get_resource_location_index(struct gl_program_resource *res)
   {
      /* Non-existent variable or resource is not referenced by fragment stage. */
   if (!res || !(res->StageReferences & (1 << MESA_SHADER_FRAGMENT)))
            /* From OpenGL 4.5 spec, 7.3 Program Objects
   * "The value -1 will be returned by either command...
   *  ... or if name identifies an active variable that does not have a
   * valid location assigned.
   */
   if (RESOURCE_VAR(res)->location == -1)
            }
      /**
   * Function implements following index queries:
   *    glGetFragDataIndex
   */
   GLint
   _mesa_program_resource_location_index(struct gl_shader_program *shProg,
         {
      struct gl_program_resource *res =
               }
      static uint8_t
   stage_from_enum(GLenum ref)
   {
      switch (ref) {
   case GL_REFERENCED_BY_VERTEX_SHADER:
         case GL_REFERENCED_BY_TESS_CONTROL_SHADER:
         case GL_REFERENCED_BY_TESS_EVALUATION_SHADER:
         case GL_REFERENCED_BY_GEOMETRY_SHADER:
         case GL_REFERENCED_BY_FRAGMENT_SHADER:
         case GL_REFERENCED_BY_COMPUTE_SHADER:
         default:
      assert(!"shader stage not supported");
         }
      /**
   * Check if resource is referenced by given 'referenced by' stage enum.
   * ATC and UBO resources hold stage references of their own.
   */
   static bool
   is_resource_referenced(struct gl_shader_program *shProg,
               {
      /* First, check if we even have such a stage active. */
   if (!shProg->_LinkedShaders[stage])
            if (res->Type == GL_ATOMIC_COUNTER_BUFFER)
            if (res->Type == GL_UNIFORM_BLOCK)
            if (res->Type == GL_SHADER_STORAGE_BLOCK)
               }
      static unsigned
   get_buffer_property(struct gl_shader_program *shProg,
               {
      GET_CURRENT_CONTEXT(ctx);
   if (res->Type != GL_UNIFORM_BLOCK &&
      res->Type != GL_ATOMIC_COUNTER_BUFFER &&
   res->Type != GL_SHADER_STORAGE_BLOCK &&
   res->Type != GL_TRANSFORM_FEEDBACK_BUFFER)
         if (res->Type == GL_UNIFORM_BLOCK) {
      switch (prop) {
   case GL_BUFFER_BINDING:
      *val = RESOURCE_UBO(res)->Binding;
      case GL_BUFFER_DATA_SIZE:
      *val = RESOURCE_UBO(res)->UniformBufferSize;
      case GL_NUM_ACTIVE_VARIABLES:
      *val = 0;
   for (unsigned i = 0; i < RESOURCE_UBO(res)->NumUniforms; i++) {
      struct gl_program_resource *uni =
      _mesa_program_resource_find_active_variable(
      shProg,
                  if (!uni)
            }
      case GL_ACTIVE_VARIABLES: {
      unsigned num_values = 0;
   for (unsigned i = 0; i < RESOURCE_UBO(res)->NumUniforms; i++) {
      struct gl_program_resource *uni =
      _mesa_program_resource_find_active_variable(
      shProg,
                  if (!uni)
         *val++ =
            }
      }
      } else if (res->Type == GL_SHADER_STORAGE_BLOCK) {
      switch (prop) {
   case GL_BUFFER_BINDING:
      *val = RESOURCE_UBO(res)->Binding;
      case GL_BUFFER_DATA_SIZE:
      *val = RESOURCE_UBO(res)->UniformBufferSize;
      case GL_NUM_ACTIVE_VARIABLES:
      *val = 0;
   for (unsigned i = 0; i < RESOURCE_UBO(res)->NumUniforms; i++) {
      struct gl_program_resource *uni =
      _mesa_program_resource_find_active_variable(
      shProg,
                  if (!uni)
            }
      case GL_ACTIVE_VARIABLES: {
      unsigned num_values = 0;
   for (unsigned i = 0; i < RESOURCE_UBO(res)->NumUniforms; i++) {
      struct gl_program_resource *uni =
      _mesa_program_resource_find_active_variable(
      shProg,
                  if (!uni)
         *val++ =
            }
      }
      } else if (res->Type == GL_ATOMIC_COUNTER_BUFFER) {
      switch (prop) {
   case GL_BUFFER_BINDING:
      *val = RESOURCE_ATC(res)->Binding;
      case GL_BUFFER_DATA_SIZE:
      *val = RESOURCE_ATC(res)->MinimumSize;
      case GL_NUM_ACTIVE_VARIABLES:
      *val = RESOURCE_ATC(res)->NumUniforms;
      case GL_ACTIVE_VARIABLES:
      for (unsigned i = 0; i < RESOURCE_ATC(res)->NumUniforms; i++) {
      /* Active atomic buffer contains index to UniformStorage. Find
   * out gl_program_resource via data pointer and then calculate
   * index of that uniform.
   */
   unsigned idx = RESOURCE_ATC(res)->Uniforms[i];
   struct gl_program_resource *uni =
      program_resource_find_data(shProg,
      assert(uni);
      }
         } else if (res->Type == GL_TRANSFORM_FEEDBACK_BUFFER) {
      switch (prop) {
   case GL_BUFFER_BINDING:
      *val = RESOURCE_XFB(res)->Binding;
      case GL_NUM_ACTIVE_VARIABLES:
      *val = RESOURCE_XFB(res)->NumVaryings;
      case GL_ACTIVE_VARIABLES:
      struct gl_transform_feedback_info *linked_xfb =
         for (int i = 0; i < linked_xfb->NumVarying; i++) {
      unsigned index = linked_xfb->Varyings[i].BufferIndex;
   struct gl_program_resource *buf_res =
      _mesa_program_resource_find_index(shProg,
            assert(buf_res);
   if (res == buf_res) {
            }
         }
         invalid_operation:
      _mesa_error_glthread_safe(ctx, GL_INVALID_OPERATION, glthread,
                           }
      unsigned
   _mesa_program_resource_prop(struct gl_shader_program *shProg,
                     {
            #define VALIDATE_TYPE(type)\
      if (res->Type != type)\
         #define VALIDATE_TYPE_2(type1, type2)\
      if (res->Type != type1 && res->Type != type2)\
            switch(prop) {
   case GL_NAME_LENGTH:
      switch (res->Type) {
   case GL_ATOMIC_COUNTER_BUFFER:
   case GL_TRANSFORM_FEEDBACK_BUFFER:
         default:
      /* Resource name length + terminator. */
      }
      case GL_TYPE:
      switch (res->Type) {
   case GL_UNIFORM:
   case GL_BUFFER_VARIABLE:
      *val = RESOURCE_UNI(res)->type->gl_type;
   *val = mediump_to_highp_type(*val);
      case GL_PROGRAM_INPUT:
   case GL_PROGRAM_OUTPUT:
      *val = RESOURCE_VAR(res)->type->gl_type;
   *val = mediump_to_highp_type(*val);
      case GL_TRANSFORM_FEEDBACK_VARYING:
      *val = RESOURCE_XFV(res)->Type;
   *val = mediump_to_highp_type(*val);
      default:
            case GL_ARRAY_SIZE:
      switch (res->Type) {
   case GL_UNIFORM:
   case GL_BUFFER_VARIABLE:
   case GL_VERTEX_SUBROUTINE_UNIFORM:
   case GL_GEOMETRY_SUBROUTINE_UNIFORM:
   case GL_FRAGMENT_SUBROUTINE_UNIFORM:
   case GL_COMPUTE_SUBROUTINE_UNIFORM:
   case GL_TESS_CONTROL_SUBROUTINE_UNIFORM:
               /* Test if a buffer variable is an array or an unsized array.
   * Unsized arrays return zero as array size.
   */
   if (RESOURCE_UNI(res)->is_shader_storage &&
      RESOURCE_UNI(res)->array_stride > 0)
      else
            case GL_PROGRAM_INPUT:
   case GL_PROGRAM_OUTPUT:
      *val = MAX2(_mesa_program_resource_array_size(res), 1);
      case GL_TRANSFORM_FEEDBACK_VARYING:
      *val = RESOURCE_XFV(res)->Size;
      default:
            case GL_OFFSET:
      switch (res->Type) {
   case GL_UNIFORM:
   case GL_BUFFER_VARIABLE:
      *val = RESOURCE_UNI(res)->offset;
      case GL_TRANSFORM_FEEDBACK_VARYING:
      *val = RESOURCE_XFV(res)->Offset;
      default:
            case GL_BLOCK_INDEX:
      VALIDATE_TYPE_2(GL_UNIFORM, GL_BUFFER_VARIABLE);
   *val = RESOURCE_UNI(res)->block_index;
      case GL_ARRAY_STRIDE:
      VALIDATE_TYPE_2(GL_UNIFORM, GL_BUFFER_VARIABLE);
   *val = RESOURCE_UNI(res)->array_stride;
      case GL_MATRIX_STRIDE:
      VALIDATE_TYPE_2(GL_UNIFORM, GL_BUFFER_VARIABLE);
   *val = RESOURCE_UNI(res)->matrix_stride;
      case GL_IS_ROW_MAJOR:
      VALIDATE_TYPE_2(GL_UNIFORM, GL_BUFFER_VARIABLE);
   *val = RESOURCE_UNI(res)->row_major;
      case GL_ATOMIC_COUNTER_BUFFER_INDEX:
      VALIDATE_TYPE(GL_UNIFORM);
   *val = RESOURCE_UNI(res)->atomic_buffer_index;
      case GL_BUFFER_BINDING:
   case GL_BUFFER_DATA_SIZE:
   case GL_NUM_ACTIVE_VARIABLES:
   case GL_ACTIVE_VARIABLES:
         case GL_REFERENCED_BY_COMPUTE_SHADER:
      if (!_mesa_has_compute_shaders(ctx))
            case GL_REFERENCED_BY_VERTEX_SHADER:
   case GL_REFERENCED_BY_TESS_CONTROL_SHADER:
   case GL_REFERENCED_BY_TESS_EVALUATION_SHADER:
   case GL_REFERENCED_BY_GEOMETRY_SHADER:
   case GL_REFERENCED_BY_FRAGMENT_SHADER:
      switch (res->Type) {
   case GL_UNIFORM:
   case GL_PROGRAM_INPUT:
   case GL_PROGRAM_OUTPUT:
   case GL_UNIFORM_BLOCK:
   case GL_BUFFER_VARIABLE:
   case GL_SHADER_STORAGE_BLOCK:
   case GL_ATOMIC_COUNTER_BUFFER:
      *val = is_resource_referenced(shProg, res, index,
            default:
            case GL_LOCATION:
      switch (res->Type) {
   case GL_UNIFORM:
   case GL_VERTEX_SUBROUTINE_UNIFORM:
   case GL_GEOMETRY_SUBROUTINE_UNIFORM:
   case GL_FRAGMENT_SUBROUTINE_UNIFORM:
   case GL_COMPUTE_SUBROUTINE_UNIFORM:
   case GL_TESS_CONTROL_SUBROUTINE_UNIFORM:
   case GL_TESS_EVALUATION_SUBROUTINE_UNIFORM:
   case GL_PROGRAM_INPUT:
   case GL_PROGRAM_OUTPUT:
      *val = program_resource_location(res, 0);
      default:
            case GL_LOCATION_COMPONENT:
      switch (res->Type) {
   case GL_PROGRAM_INPUT:
   case GL_PROGRAM_OUTPUT:
      *val = RESOURCE_VAR(res)->component;
      default:
            case GL_LOCATION_INDEX: {
      int tmp;
   if (res->Type != GL_PROGRAM_OUTPUT)
         tmp = program_resource_location(res, 0);
   if (tmp == -1)
         else
            }
   case GL_NUM_COMPATIBLE_SUBROUTINES:
      if (res->Type != GL_VERTEX_SUBROUTINE_UNIFORM &&
      res->Type != GL_FRAGMENT_SUBROUTINE_UNIFORM &&
   res->Type != GL_GEOMETRY_SUBROUTINE_UNIFORM &&
   res->Type != GL_COMPUTE_SUBROUTINE_UNIFORM &&
   res->Type != GL_TESS_CONTROL_SUBROUTINE_UNIFORM &&
   res->Type != GL_TESS_EVALUATION_SUBROUTINE_UNIFORM)
      *val = RESOURCE_UNI(res)->num_compatible_subroutines;
      case GL_COMPATIBLE_SUBROUTINES: {
      const struct gl_uniform_storage *uni;
   struct gl_program *p;
   unsigned count, i;
            if (res->Type != GL_VERTEX_SUBROUTINE_UNIFORM &&
      res->Type != GL_FRAGMENT_SUBROUTINE_UNIFORM &&
   res->Type != GL_GEOMETRY_SUBROUTINE_UNIFORM &&
   res->Type != GL_COMPUTE_SUBROUTINE_UNIFORM &&
   res->Type != GL_TESS_CONTROL_SUBROUTINE_UNIFORM &&
   res->Type != GL_TESS_EVALUATION_SUBROUTINE_UNIFORM)
               p = shProg->_LinkedShaders[_mesa_shader_stage_from_subroutine_uniform(res->Type)]->Program;
   count = 0;
   for (i = 0; i < p->sh.NumSubroutineFunctions; i++) {
      struct gl_subroutine_function *fn = &p->sh.SubroutineFunctions[i];
   for (j = 0; j < fn->num_compat_types; j++) {
      if (fn->types[j] == uni->type) {
      val[count++] = i;
            }
               case GL_TOP_LEVEL_ARRAY_SIZE:
      VALIDATE_TYPE(GL_BUFFER_VARIABLE);
   *val = RESOURCE_UNI(res)->top_level_array_size;
         case GL_TOP_LEVEL_ARRAY_STRIDE:
      VALIDATE_TYPE(GL_BUFFER_VARIABLE);
   *val = RESOURCE_UNI(res)->top_level_array_stride;
         /* GL_ARB_tessellation_shader */
   case GL_IS_PER_PATCH:
      switch (res->Type) {
   case GL_PROGRAM_INPUT:
   case GL_PROGRAM_OUTPUT:
      *val = RESOURCE_VAR(res)->patch;
      default:
               case GL_TRANSFORM_FEEDBACK_BUFFER_INDEX:
      VALIDATE_TYPE(GL_TRANSFORM_FEEDBACK_VARYING);
   *val = RESOURCE_XFV(res)->BufferIndex;
      case GL_TRANSFORM_FEEDBACK_BUFFER_STRIDE:
      VALIDATE_TYPE(GL_TRANSFORM_FEEDBACK_BUFFER);
   *val = RESOURCE_XFB(res)->Stride * 4;
         default:
               #undef VALIDATE_TYPE
   #undef VALIDATE_TYPE_2
      invalid_enum:
      _mesa_error_glthread_safe(ctx, GL_INVALID_ENUM, glthread,
                           invalid_operation:
      _mesa_error_glthread_safe(ctx, GL_INVALID_OPERATION, glthread,
                        }
      extern void
   _mesa_get_program_resourceiv(struct gl_shader_program *shProg,
                     {
      GET_CURRENT_CONTEXT(ctx);
   GLint *val = (GLint *) params;
   const GLenum *prop = props;
            struct gl_program_resource *res =
            /* No such resource found or bufSize negative. */
   if (!res || bufSize < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                           /* Write propCount values until error occurs or bufSize reached. */
   for (int i = 0; i < propCount && i < bufSize; i++, val++, prop++) {
      int props_written =
                  /* Error happened. */
   if (props_written == 0)
                        /* If <length> is not NULL, the actual number of integer values
   * written to <params> will be written to <length>.
   */
   if (length)
      }
      extern void
   _mesa_get_program_interfaceiv(struct gl_shader_program *shProg,
               {
      GET_CURRENT_CONTEXT(ctx);
            /* Validate pname against interface. */
   switch(pname) {
   case GL_ACTIVE_RESOURCES:
      for (i = 0, *params = 0; i < shProg->data->NumProgramResourceList; i++)
      if (shProg->data->ProgramResourceList[i].Type == programInterface)
         case GL_MAX_NAME_LENGTH:
      if (programInterface == GL_ATOMIC_COUNTER_BUFFER ||
      programInterface == GL_TRANSFORM_FEEDBACK_BUFFER) {
   _mesa_error(ctx, GL_INVALID_OPERATION,
               "glGetProgramInterfaceiv(%s pname %s)",
      }
   /* Name length consists of base name, 3 additional chars '[0]' if
   * resource is an array and finally 1 char for string terminator.
   */
   for (i = 0, *params = 0; i < shProg->data->NumProgramResourceList; i++) {
      if (shProg->data->ProgramResourceList[i].Type != programInterface)
         unsigned len =
            }
      case GL_MAX_NUM_ACTIVE_VARIABLES:
      switch (programInterface) {
   case GL_UNIFORM_BLOCK:
      for (i = 0, *params = 0; i < shProg->data->NumProgramResourceList; i++) {
      if (shProg->data->ProgramResourceList[i].Type == programInterface) {
      struct gl_uniform_block *block =
      (struct gl_uniform_block *)
            }
      case GL_SHADER_STORAGE_BLOCK:
      for (i = 0, *params = 0; i < shProg->data->NumProgramResourceList; i++) {
      if (shProg->data->ProgramResourceList[i].Type == programInterface) {
      struct gl_uniform_block *block =
      (struct gl_uniform_block *)
      GLint block_params = 0;
   for (unsigned j = 0; j < block->NumUniforms; j++) {
      struct gl_program_resource *uni =
      _mesa_program_resource_find_active_variable(
      shProg,
   GL_BUFFER_VARIABLE,
   block,
   if (!uni)
            }
         }
      case GL_ATOMIC_COUNTER_BUFFER:
      for (i = 0, *params = 0; i < shProg->data->NumProgramResourceList; i++) {
      if (shProg->data->ProgramResourceList[i].Type == programInterface) {
      struct gl_active_atomic_buffer *buffer =
      (struct gl_active_atomic_buffer *)
            }
      case GL_TRANSFORM_FEEDBACK_BUFFER:
      for (i = 0, *params = 0; i < shProg->data->NumProgramResourceList; i++) {
      if (shProg->data->ProgramResourceList[i].Type == programInterface) {
      struct gl_transform_feedback_buffer *buffer =
      (struct gl_transform_feedback_buffer *)
            }
      default:
   _mesa_error(ctx, GL_INVALID_OPERATION,
               "glGetProgramInterfaceiv(%s pname %s)",
   }
      case GL_MAX_NUM_COMPATIBLE_SUBROUTINES:
      switch (programInterface) {
   case GL_VERTEX_SUBROUTINE_UNIFORM:
   case GL_FRAGMENT_SUBROUTINE_UNIFORM:
   case GL_GEOMETRY_SUBROUTINE_UNIFORM:
   case GL_COMPUTE_SUBROUTINE_UNIFORM:
   case GL_TESS_CONTROL_SUBROUTINE_UNIFORM:
   case GL_TESS_EVALUATION_SUBROUTINE_UNIFORM: {
      for (i = 0, *params = 0; i < shProg->data->NumProgramResourceList; i++) {
      if (shProg->data->ProgramResourceList[i].Type == programInterface) {
      struct gl_uniform_storage *uni =
      (struct gl_uniform_storage *)
            }
               default:
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  }
      default:
      _mesa_error(ctx, GL_INVALID_OPERATION,
               }
      static bool
   validate_io(struct gl_program *producer, struct gl_program *consumer)
   {
      if (producer->sh.data->linked_stages == consumer->sh.data->linked_stages)
            const bool producer_is_array_stage =
         const bool consumer_is_array_stage =
      consumer->info.stage == MESA_SHADER_GEOMETRY ||
   consumer->info.stage == MESA_SHADER_TESS_CTRL ||
                  gl_shader_variable const **outputs =
      (gl_shader_variable const **) calloc(producer->sh.data->NumProgramResourceList,
      if (outputs == NULL)
            /* Section 7.4.1 (Shader Interface Matching) of the OpenGL ES 3.1 spec
   * says:
   *
   *    At an interface between program objects, the set of inputs and
   *    outputs are considered to match exactly if and only if:
   *
   *    - Every declared input variable has a matching output, as described
   *      above.
   *    - There are no user-defined output variables declared without a
   *      matching input variable declaration.
   *
   * Every input has an output, and every output has an input.  Scan the list
   * of producer resources once, and generate the list of outputs.  As inputs
   * and outputs are matched, remove the matched outputs from the set.  At
   * the end, the set must be empty.  If the set is not empty, then there is
   * some output that did not have an input.
   */
   unsigned num_outputs = 0;
   for (unsigned i = 0; i < producer->sh.data->NumProgramResourceList; i++) {
      struct gl_program_resource *res =
            if (res->Type != GL_PROGRAM_OUTPUT)
                     /* Section 7.4.1 (Shader Interface Matching) of the OpenGL ES 3.1 spec
   * says:
   *
   *    Built-in inputs or outputs do not affect interface matching.
   */
   if (is_gl_identifier(var->name.string))
                        unsigned match_index = 0;
   for (unsigned i = 0; i < consumer->sh.data->NumProgramResourceList; i++) {
      struct gl_program_resource *res =
            if (res->Type != GL_PROGRAM_INPUT)
            gl_shader_variable const *const consumer_var = RESOURCE_VAR(res);
            if (is_gl_identifier(consumer_var->name.string))
            /* Inputs with explicit locations match other outputs with explicit
   * locations by location instead of by name.
   */
   if (consumer_var->explicit_location) {
                        if (var->explicit_location &&
      consumer_var->location == var->location) {
   producer_var = var;
   match_index = j;
            } else {
                        if (!var->explicit_location &&
      strcmp(consumer_var->name.string, var->name.string) == 0) {
   producer_var = var;
   match_index = j;
                     /* Section 7.4.1 (Shader Interface Matching) of the OpenGL ES 3.1 spec
   * says:
   *
   *    - An output variable is considered to match an input variable in
   *      the subsequent shader if:
   *
   *      - the two variables match in name, type, and qualification; or
   *
   *      - the two variables are declared with the same location
   *        qualifier and match in type and qualification.
   */
   if (producer_var == NULL) {
      valid = false;
               /* An output cannot match more than one input, so remove the output from
   * the set of possible outputs.
   */
   outputs[match_index] = NULL;
   num_outputs--;
   if (match_index < num_outputs)
            /* Section 7.4.1 (Shader Interface Matching) of the ES 3.2 spec says:
   *
   *    "Tessellation control shader per-vertex output variables and
   *     blocks and tessellation control, tessellation evaluation, and
   *     geometry shader per-vertex input variables and blocks are
   *     required to be declared as arrays, with each element representing
   *     input or output values for a single vertex of a multi-vertex
   *     primitive. For the purposes of interface matching, such variables
   *     and blocks are treated as though they were not declared as
   *     arrays."
   *
   * So we unwrap those types before matching.
   */
   const glsl_type *consumer_type = consumer_var->type;
   const glsl_type *consumer_interface_type = consumer_var->interface_type;
   const glsl_type *producer_type = producer_var->type;
            if (consumer_is_array_stage) {
      if (consumer_interface_type) {
      /* the interface is the array; the underlying types should match */
   if (consumer_interface_type->is_array() && !consumer_var->patch)
      } else {
      if (consumer_type->is_array() && !consumer_var->patch)
                  if (producer_is_array_stage) {
      if (producer_interface_type) {
      /* the interface is the array; the underlying types should match */
   if (producer_interface_type->is_array() && !producer_var->patch)
      } else {
      if (producer_type->is_array() && !producer_var->patch)
                  if (producer_type != consumer_type) {
      valid = false;
               if (producer_interface_type != consumer_interface_type) {
      valid = false;
               /* Section 9.2.2 (Separable Programs) of the GLSL ES spec says:
   *
   *    Qualifier Class|  Qualifier  |in/out
   *    ---------------+-------------+------
   *    Storage        |     in      |
   *                   |     out     |  N/A
   *                   |   uniform   |
   *    ---------------+-------------+------
   *    Auxiliary      |   centroid  |   No
   *    ---------------+-------------+------
   *                   |   location  |  Yes
   *                   | Block layout|  N/A
   *                   |   binding   |  N/A
   *                   |   offset    |  N/A
   *                   |   format    |  N/A
   *    ---------------+-------------+------
   *    Interpolation  |   smooth    |
   *                   |    flat     |  Yes
   *    ---------------+-------------+------
   *                   |    lowp     |
   *    Precision      |   mediump   |  Yes
   *                   |    highp    |
   *    ---------------+-------------+------
   *    Variance       |  invariant  |   No
   *    ---------------+-------------+------
   *    Memory         |     all     |  N/A
   *
   * Note that location mismatches are detected by the loops above that
   * find the producer variable that goes with the consumer variable.
   */
   unsigned producer_interpolation = producer_var->interpolation;
   unsigned consumer_interpolation = consumer_var->interpolation;
   if (producer_interpolation == INTERP_MODE_NONE)
         if (consumer_interpolation == INTERP_MODE_NONE)
         if (producer_interpolation != consumer_interpolation) {
      valid = false;
               if (producer_var->precision != consumer_var->precision) {
      valid = false;
               if (producer_var->outermost_struct_type != consumer_var->outermost_struct_type) {
      valid = false;
               out:
      free(outputs);
      }
      /**
   * Validate inputs against outputs in a program pipeline.
   */
   extern "C" bool
   _mesa_validate_pipeline_io(struct gl_pipeline_object *pipeline)
   {
               /* Find first active stage in pipeline. */
   unsigned idx, prev = 0;
   for (idx = 0; idx < ARRAY_SIZE(pipeline->CurrentProgram); idx++) {
      if (prog[idx]) {
      prev = idx;
                  for (idx = prev + 1; idx < ARRAY_SIZE(pipeline->CurrentProgram); idx++) {
      if (prog[idx]) {
      /* Pipeline might include both non-compute and a compute program, do
   * not attempt to validate varyings between non-compute and compute
   * stage.
   */
                                       }
      }
      extern "C" void
   _mesa_program_resource_hash_destroy(struct gl_shader_program *shProg)
   {
      for (unsigned i = 0; i < ARRAY_SIZE(shProg->data->ProgramResourceHash); i++) {
      if (shProg->data->ProgramResourceHash[i]) {
      _mesa_hash_table_destroy(shProg->data->ProgramResourceHash[i], NULL);
            }
      extern "C" void
   _mesa_create_program_resource_hash(struct gl_shader_program *shProg)
   {
      /* Rebuild resource hash. */
            struct gl_program_resource *res = shProg->data->ProgramResourceList;
   for (unsigned i = 0; i < shProg->data->NumProgramResourceList; i++, res++) {
      struct gl_resource_name name;
   if (_mesa_program_get_resource_name(res, &name)) {
                     if (!shProg->data->ProgramResourceHash[type]) {
      shProg->data->ProgramResourceHash[type] =
                     _mesa_hash_table_insert(shProg->data->ProgramResourceHash[type],
            }
