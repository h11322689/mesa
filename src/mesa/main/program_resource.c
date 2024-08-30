   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 2015 Intel Corporation.  All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included
   * in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   * OTHER DEALINGS IN THE SOFTWARE.
   *
   */
      #include "main/enums.h"
   #include "main/macros.h"
   #include "main/mtypes.h"
   #include "main/shaderapi.h"
   #include "main/shaderobj.h"
   #include "main/context.h"
   #include "compiler/glsl/ir_uniform.h"
   #include "api_exec_decl.h"
      static bool
   supported_interface_enum(struct gl_context *ctx, GLenum iface)
   {
      switch (iface) {
   case GL_UNIFORM:
   case GL_UNIFORM_BLOCK:
   case GL_PROGRAM_INPUT:
   case GL_PROGRAM_OUTPUT:
   case GL_TRANSFORM_FEEDBACK_BUFFER:
   case GL_TRANSFORM_FEEDBACK_VARYING:
   case GL_ATOMIC_COUNTER_BUFFER:
   case GL_BUFFER_VARIABLE:
   case GL_SHADER_STORAGE_BLOCK:
         case GL_VERTEX_SUBROUTINE:
   case GL_FRAGMENT_SUBROUTINE:
   case GL_VERTEX_SUBROUTINE_UNIFORM:
   case GL_FRAGMENT_SUBROUTINE_UNIFORM:
         case GL_GEOMETRY_SUBROUTINE:
   case GL_GEOMETRY_SUBROUTINE_UNIFORM:
         case GL_COMPUTE_SUBROUTINE:
   case GL_COMPUTE_SUBROUTINE_UNIFORM:
         case GL_TESS_CONTROL_SUBROUTINE:
   case GL_TESS_EVALUATION_SUBROUTINE:
   case GL_TESS_CONTROL_SUBROUTINE_UNIFORM:
   case GL_TESS_EVALUATION_SUBROUTINE_UNIFORM:
         default:
            }
      static struct gl_shader_program *
   lookup_linked_program(GLuint program, const char *caller)
   {
      GET_CURRENT_CONTEXT(ctx);
   struct gl_shader_program *prog =
            if (!prog)
            if (prog->data->LinkStatus == LINKING_FAILURE) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(program not linked)",
            }
      }
      void GLAPIENTRY
   _mesa_GetProgramInterfaceiv(GLuint program, GLenum programInterface,
         {
               if (MESA_VERBOSE & VERBOSE_API) {
      _mesa_debug(ctx, "glGetProgramInterfaceiv(%u, %s, %s, %p)\n",
                     struct gl_shader_program *shProg =
      _mesa_lookup_shader_program_err(ctx, program,
      if (!shProg)
            if (!params) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     /* Validate interface. */
   if (!supported_interface_enum(ctx, programInterface)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetProgramInterfaceiv(%s)",
                        }
      static bool
   is_xfb_marker(const char *str)
   {
      static const char *markers[] = {
      "gl_NextBuffer",
   "gl_SkipComponents1",
   "gl_SkipComponents2",
   "gl_SkipComponents3",
   "gl_SkipComponents4",
      };
            if (strncmp(str, "gl_", 3) != 0)
            for (; *m; m++)
      if (strcmp(*m, str) == 0)
            }
      GLuint GLAPIENTRY
   _mesa_GetProgramResourceIndex(GLuint program, GLenum programInterface,
         {
               if (MESA_VERBOSE & VERBOSE_API) {
      _mesa_debug(ctx, "glGetProgramResourceIndex(%u, %s, %s)\n",
               unsigned array_index = 0;
   struct gl_program_resource *res;
   struct gl_shader_program *shProg =
      _mesa_lookup_shader_program_err(ctx, program,
      if (!shProg || !name)
            if (!supported_interface_enum(ctx, programInterface)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramResourceIndex(%s)",
            }
   /*
   * For the interface TRANSFORM_FEEDBACK_VARYING, the value INVALID_INDEX
   * should be returned when querying the index assigned to the special names
   * "gl_NextBuffer", "gl_SkipComponents1", "gl_SkipComponents2",
   * "gl_SkipComponents3", and "gl_SkipComponents4".
   */
   if (programInterface == GL_TRANSFORM_FEEDBACK_VARYING &&
      is_xfb_marker(name))
         switch (programInterface) {
   case GL_TESS_CONTROL_SUBROUTINE:
   case GL_TESS_CONTROL_SUBROUTINE_UNIFORM:
   case GL_TESS_EVALUATION_SUBROUTINE:
   case GL_TESS_EVALUATION_SUBROUTINE_UNIFORM:
   case GL_COMPUTE_SUBROUTINE:
   case GL_COMPUTE_SUBROUTINE_UNIFORM:
   case GL_GEOMETRY_SUBROUTINE:
   case GL_GEOMETRY_SUBROUTINE_UNIFORM:
   case GL_VERTEX_SUBROUTINE:
   case GL_FRAGMENT_SUBROUTINE:
   case GL_VERTEX_SUBROUTINE_UNIFORM:
   case GL_FRAGMENT_SUBROUTINE_UNIFORM:
   case GL_PROGRAM_INPUT:
   case GL_PROGRAM_OUTPUT:
   case GL_UNIFORM:
   case GL_BUFFER_VARIABLE:
   case GL_TRANSFORM_FEEDBACK_VARYING:
   case GL_UNIFORM_BLOCK:
   case GL_SHADER_STORAGE_BLOCK:
      res = _mesa_program_resource_find_name(shProg, programInterface, name,
         if (!res || array_index > 0)
               case GL_ATOMIC_COUNTER_BUFFER:
   case GL_TRANSFORM_FEEDBACK_BUFFER:
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramResourceIndex(%s)",
                  }
      void GLAPIENTRY
   _mesa_GetProgramResourceName(GLuint program, GLenum programInterface,
               {
               if (MESA_VERBOSE & VERBOSE_API) {
      _mesa_debug(ctx, "glGetProgramResourceName(%u, %s, %u, %d, %p, %p)\n",
                     struct gl_shader_program *shProg =
      _mesa_lookup_shader_program_err(ctx, program,
         if (!shProg || !name)
            if (programInterface == GL_ATOMIC_COUNTER_BUFFER ||
      programInterface == GL_TRANSFORM_FEEDBACK_BUFFER ||
   !supported_interface_enum(ctx, programInterface)) {
   _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramResourceName(%s)",
                     _mesa_get_program_resource_name(shProg, programInterface, index, bufSize,
            }
      void GLAPIENTRY
   _mesa_GetProgramResourceiv(GLuint program, GLenum programInterface,
                     {
               if (MESA_VERBOSE & VERBOSE_API) {
      _mesa_debug(ctx, "glGetProgramResourceiv(%u, %s, %u, %d, %p, %d, %p, %p)\n",
                     struct gl_shader_program *shProg =
            if (!shProg || !params)
            /* The error INVALID_VALUE is generated if <propCount> is zero.
   * Note that we check < 0 here because it makes sense to bail early.
   */
   if (propCount <= 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                     _mesa_get_program_resourceiv(shProg, programInterface, index,
      }
      GLint GLAPIENTRY
   _mesa_GetProgramResourceLocation(GLuint program, GLenum programInterface,
         {
               if (MESA_VERBOSE & VERBOSE_API) {
      _mesa_debug(ctx, "glGetProgramResourceLocation(%u, %s, %s)\n",
               struct gl_shader_program *shProg =
            if (!shProg || !name)
            /* Validate programInterface. */
   switch (programInterface) {
   case GL_UNIFORM:
   case GL_PROGRAM_INPUT:
   case GL_PROGRAM_OUTPUT:
            case GL_VERTEX_SUBROUTINE_UNIFORM:
   case GL_FRAGMENT_SUBROUTINE_UNIFORM:
      if (!_mesa_has_ARB_shader_subroutine(ctx))
            case GL_GEOMETRY_SUBROUTINE_UNIFORM:
      if (!_mesa_has_geometry_shaders(ctx) || !_mesa_has_ARB_shader_subroutine(ctx))
            case GL_COMPUTE_SUBROUTINE_UNIFORM:
      if (!_mesa_has_compute_shaders(ctx) || !_mesa_has_ARB_shader_subroutine(ctx))
            case GL_TESS_CONTROL_SUBROUTINE_UNIFORM:
   case GL_TESS_EVALUATION_SUBROUTINE_UNIFORM:
      if (!_mesa_has_tessellation(ctx) || !_mesa_has_ARB_shader_subroutine(ctx))
            default:
                     fail:
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramResourceLocation(%s %s)",
            }
      /**
   * Returns output index for dual source blending.
   */
   GLint GLAPIENTRY
   _mesa_GetProgramResourceLocationIndex(GLuint program, GLenum programInterface,
         {
               if (MESA_VERBOSE & VERBOSE_API) {
      _mesa_debug(ctx, "glGetProgramResourceLocationIndex(%u, %s, %s)\n",
               struct gl_shader_program *shProg =
            if (!shProg || !name)
            /* From the GL_ARB_program_interface_query spec:
   *
   * "For GetProgramResourceLocationIndex, <programInterface> must be
   * PROGRAM_OUTPUT."
   */
   if (programInterface != GL_PROGRAM_OUTPUT) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                           return _mesa_program_resource_location_index(shProg, GL_PROGRAM_OUTPUT,
      }
