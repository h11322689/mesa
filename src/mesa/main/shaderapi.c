   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 2004-2008  Brian Paul   All Rights Reserved.
   * Copyright (C) 2009-2010  VMware, Inc.  All Rights Reserved.
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
   */
      /**
   * \file shaderapi.c
   * \author Brian Paul
   *
   * Implementation of GLSL-related API functions.
   * The glUniform* functions are in uniforms.c
   */
         #include <errno.h>
   #include <stdbool.h>
   #include <c99_alloca.h>
      #include "util/glheader.h"
   #include "main/context.h"
   #include "draw_validate.h"
   #include "main/enums.h"
   #include "main/glspirv.h"
   #include "main/hash.h"
   #include "main/mtypes.h"
   #include "main/pipelineobj.h"
   #include "main/program_binary.h"
   #include "main/shaderapi.h"
   #include "main/shaderobj.h"
   #include "main/state.h"
   #include "main/transformfeedback.h"
   #include "main/uniforms.h"
   #include "compiler/glsl/builtin_functions.h"
   #include "compiler/glsl/glsl_parser_extras.h"
   #include "compiler/glsl/ir.h"
   #include "compiler/glsl/ir_uniform.h"
   #include "compiler/glsl/program.h"
   #include "program/program.h"
   #include "program/prog_print.h"
   #include "program/prog_parameter.h"
   #include "util/ralloc.h"
   #include "util/hash_table.h"
   #include "util/crc32.h"
   #include "util/os_file.h"
   #include "util/list.h"
   #include "util/perf/cpu_trace.h"
   #include "util/u_process.h"
   #include "util/u_string.h"
   #include "api_exec_decl.h"
      #include "state_tracker/st_context.h"
   #include "state_tracker/st_glsl_to_nir.h"
   #include "state_tracker/st_program.h"
      #ifdef ENABLE_SHADER_CACHE
   #if CUSTOM_SHADER_REPLACEMENT
   #include "shader_replacement.h"
   /* shader_replacement.h must declare a variable like this:
         struct _shader_replacement {
      // process name. If null, only sha1 is used to match
   const char *app;
   // original glsl shader sha1
   const char *sha1;
   // shader stage
   gl_shader_stage stage;
      };
            And a method to load a given replacement and return the new
                                       shader_replacement.h can be generated at build time, or copied
      */
   #else
   struct _shader_replacement {
      const char *app;
   const char *sha1;
      };
   struct _shader_replacement shader_replacements[0];
      static char *try_direct_replace(const char *app, const char *source)
   {
         }
      static char* load_shader_replacement(struct _shader_replacement *repl)
   {
         }
   #endif
   #endif
      /**
   * Return mask of GLSL_x flags by examining the MESA_GLSL env var.
   */
   GLbitfield
   _mesa_get_shader_flags(void)
   {
      GLbitfield flags = 0x0;
            if (env) {
      if (strstr(env, "dump_on_error"))
   #ifndef CUSTOM_SHADER_REPLACEMENT
         else if (strstr(env, "dump"))
         if (strstr(env, "log"))
         if (strstr(env, "source"))
   #endif
         if (strstr(env, "cache_fb"))
         if (strstr(env, "cache_info"))
         if (strstr(env, "nopvert"))
         if (strstr(env, "nopfrag"))
         if (strstr(env, "uniform"))
         if (strstr(env, "useprog"))
         if (strstr(env, "errors"))
                  }
      #define ANDROID_SHADER_CAPTURE 0
      #if ANDROID_SHADER_CAPTURE
   #include "util/u_process.h"
   #include <sys/stat.h>
   #include <sys/types.h>
   #endif
      /**
   * Memoized version of getenv("MESA_SHADER_CAPTURE_PATH").
   */
   const char *
   _mesa_get_shader_capture_path(void)
   {
      static bool read_env_var = false;
            if (!read_env_var) {
      path = secure_getenv("MESA_SHADER_CAPTURE_PATH");
      #if ANDROID_SHADER_CAPTURE
         if (!path) {
      char *p;
   asprintf(&p, "/data/shaders/%s", util_get_process_name());
   mkdir(p, 0755);
      #endif
                  }
      /**
   * Initialize context's shader state.
   */
   void
   _mesa_init_shader_state(struct gl_context *ctx)
   {
      /* Device drivers may override these to control what kind of instructions
   * are generated by the GLSL compiler.
   */
   struct gl_shader_compiler_options options;
   gl_shader_stage sh;
            memset(&options, 0, sizeof(options));
            for (sh = 0; sh < MESA_SHADER_STAGES; ++sh)
                     if (ctx->Shader.Flags != 0)
            /* Extended for ARB_separate_shader_objects */
   ctx->Shader.RefCount = 1;
   ctx->TessCtrlProgram.patch_vertices = 3;
   for (i = 0; i < 4; ++i)
         for (i = 0; i < 2; ++i)
      }
         /**
   * Free the per-context shader-related state.
   */
   void
   _mesa_free_shader_state(struct gl_context *ctx)
   {
      for (int i = 0; i < MESA_SHADER_STAGES; i++) {
      _mesa_reference_program(ctx, &ctx->Shader.CurrentProgram[i], NULL);
   _mesa_reference_shader_program(ctx,
               free(ctx->SubroutineIndex[i].IndexPtr);
      }
            /* Extended for ARB_separate_shader_objects */
               }
         /**
   * Copy string from <src> to <dst>, up to maxLength characters, returning
   * length of <dst> in <length>.
   * \param src  the strings source
   * \param maxLength  max chars to copy
   * \param length  returns number of chars copied
   * \param dst  the string destination
   */
   void
   _mesa_copy_string(GLchar *dst, GLsizei maxLength,
         {
      GLsizei len;
   for (len = 0; len < maxLength - 1 && src && src[len]; len++)
         if (maxLength > 0)
         if (length)
      }
            /**
   * Confirm that the a shader type is valid and supported by the implementation
   *
   * \param ctx   Current GL context
   * \param type  Shader target
   *
   */
   bool
   _mesa_validate_shader_target(const struct gl_context *ctx, GLenum type)
   {
      /* Note: when building built-in GLSL functions, this function may be
   * invoked with ctx == NULL.  In that case, we can only validate that it's
   * a shader target we recognize, not that it's supported in the current
   * context.  But that's fine--we don't need any further validation than
   * that when building built-in GLSL functions.
            switch (type) {
   case GL_FRAGMENT_SHADER:
         case GL_VERTEX_SHADER:
         case GL_GEOMETRY_SHADER_ARB:
         case GL_TESS_CONTROL_SHADER:
   case GL_TESS_EVALUATION_SHADER:
         case GL_COMPUTE_SHADER:
         default:
            }
         static GLboolean
   is_program(struct gl_context *ctx, GLuint name)
   {
      struct gl_shader_program *shProg = _mesa_lookup_shader_program(ctx, name);
      }
         static GLboolean
   is_shader(struct gl_context *ctx, GLuint name)
   {
      struct gl_shader *shader = _mesa_lookup_shader(ctx, name);
      }
         /**
   * Attach shader to a shader program.
   */
   static void
   attach_shader(struct gl_context *ctx, struct gl_shader_program *shProg,
         {
               shProg->Shaders = realloc(shProg->Shaders,
         if (!shProg->Shaders) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glAttachShader");
               /* append */
   shProg->Shaders[n] = NULL; /* since realloc() didn't zero the new space */
   _mesa_reference_shader(ctx, &shProg->Shaders[n], sh);
      }
      static void
   attach_shader_err(struct gl_context *ctx, GLuint program, GLuint shader,
         {
      struct gl_shader_program *shProg;
   struct gl_shader *sh;
                     shProg = _mesa_lookup_shader_program_err(ctx, program, caller);
   if (!shProg)
            sh = _mesa_lookup_shader_err(ctx, shader, caller);
   if (!sh) {
                  n = shProg->NumShaders;
   for (i = 0; i < n; i++) {
      if (shProg->Shaders[i] == sh) {
      /* The shader is already attched to this program.  The
   * GL_ARB_shader_objects spec says:
   *
   *     "The error INVALID_OPERATION is generated by AttachObjectARB
   *     if <obj> is already attached to <containerObj>."
   */
   _mesa_error(ctx, GL_INVALID_OPERATION, "%s", caller);
      } else if (same_type_disallowed &&
         /* Shader with the same type is already attached to this program,
      * OpenGL ES 2.0 and 3.0 specs say:
   *
   *      "Multiple shader objects of the same type may not be attached
   *      to a single program object. [...] The error INVALID_OPERATION
   *      is generated if [...] another shader object of the same type
   *      as shader is already attached to program."
   */
   _mesa_error(ctx, GL_INVALID_OPERATION, "%s", caller);
                     }
      static void
   attach_shader_no_error(struct gl_context *ctx, GLuint program, GLuint shader)
   {
      struct gl_shader_program *shProg;
            shProg = _mesa_lookup_shader_program(ctx, program);
               }
      static GLuint
   create_shader(struct gl_context *ctx, GLenum type)
   {
      struct gl_shader *sh;
            _mesa_HashLockMutex(ctx->Shared->ShaderObjects);
   name = _mesa_HashFindFreeKeyBlock(ctx->Shared->ShaderObjects, 1);
   sh = _mesa_new_shader(name, _mesa_shader_enum_to_shader_stage(type));
   sh->Type = type;
   _mesa_HashInsertLocked(ctx->Shared->ShaderObjects, name, sh, true);
               }
         static GLuint
   create_shader_err(struct gl_context *ctx, GLenum type, const char *caller)
   {
      if (!_mesa_validate_shader_target(ctx, type)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "%s(%s)",
                        }
         static GLuint
   create_shader_program(struct gl_context *ctx)
   {
      GLuint name;
                                                                     }
         /**
   * Delete a shader program.  Actually, just decrement the program's
   * reference count and mark it as DeletePending.
   * Used to implement glDeleteProgram() and glDeleteObjectARB().
   */
   static void
   delete_shader_program(struct gl_context *ctx, GLuint name)
   {
      /*
   * NOTE: deleting shaders/programs works a bit differently than
   * texture objects (and buffer objects, etc).  Shader/program
   * handles/IDs exist in the hash table until the object is really
   * deleted (refcount==0).  With texture objects, the handle/ID is
   * removed from the hash table in glDeleteTextures() while the tex
   * object itself might linger until its refcount goes to zero.
   */
            shProg = _mesa_lookup_shader_program_err(ctx, name, "glDeleteProgram");
   if (!shProg)
            if (!shProg->DeletePending) {
               /* effectively, decr shProg's refcount */
         }
         static void
   delete_shader(struct gl_context *ctx, GLuint shader)
   {
               sh = _mesa_lookup_shader_err(ctx, shader, "glDeleteShader");
   if (!sh)
            if (!sh->DeletePending) {
               /* effectively, decr sh's refcount */
         }
         static ALWAYS_INLINE void
   detach_shader(struct gl_context *ctx, GLuint program, GLuint shader,
         {
      struct gl_shader_program *shProg;
   GLuint n;
            if (!no_error) {
      shProg = _mesa_lookup_shader_program_err(ctx, program, "glDetachShader");
   if (!shProg)
      } else {
                           for (i = 0; i < n; i++) {
      if (shProg->Shaders[i]->Name == shader) {
                                    /* alloc new, smaller array */
   newList = malloc((n - 1) * sizeof(struct gl_shader *));
   if (!newList) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glDetachShader");
      }
   /* Copy old list entries to new list, skipping removed entry at [i] */
   for (j = 0; j < i; j++) {
         }
   while (++i < n) {
                  /* Free old list and install new one */
   free(shProg->Shaders);
         #ifndef NDEBUG
            /* sanity check - make sure the new list's entries are sensible */
   for (j = 0; j < shProg->NumShaders; j++) {
      assert(shProg->Shaders[j]->Stage == MESA_SHADER_VERTEX ||
         shProg->Shaders[j]->Stage == MESA_SHADER_TESS_CTRL ||
   shProg->Shaders[j]->Stage == MESA_SHADER_TESS_EVAL ||
   shProg->Shaders[j]->Stage == MESA_SHADER_GEOMETRY ||
   #endif
                              /* not found */
   if (!no_error) {
      GLenum err;
   if (is_shader(ctx, shader) || is_program(ctx, shader))
         else
         _mesa_error(ctx, err, "glDetachShader(shader)");
         }
         static void
   detach_shader_error(struct gl_context *ctx, GLuint program, GLuint shader)
   {
         }
         static void
   detach_shader_no_error(struct gl_context *ctx, GLuint program, GLuint shader)
   {
         }
         /**
   * Return list of shaders attached to shader program.
   * \param objOut  returns GLuint ids
   * \param handleOut  returns GLhandleARB handles
   */
   static void
   get_attached_shaders(struct gl_context *ctx, GLuint program, GLsizei maxCount,
         {
               if (maxCount < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetAttachedShaders(maxCount < 0)");
               shProg =
            if (shProg) {
      GLuint i;
   for (i = 0; i < (GLuint) maxCount && i < shProg->NumShaders; i++) {
      if (objOut) {
                  if (handleOut) {
            }
   if (countOut) {
               }
      /**
   * glGetHandleARB() - return ID/name of currently bound shader program.
   */
   static GLuint
   get_handle(struct gl_context *ctx, GLenum pname)
   {
      if (pname == GL_PROGRAM_OBJECT_ARB) {
      if (ctx->_Shader->ActiveProgram)
         else
      }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetHandleARB");
         }
         /**
   * Check if a geometry shader query is valid at this time.  If not, report an
   * error and return false.
   *
   * From GL 3.2 section 6.1.16 (Shader and Program Queries):
   *
   *     "If GEOMETRY_VERTICES_OUT, GEOMETRY_INPUT_TYPE, or GEOMETRY_OUTPUT_TYPE
   *     are queried for a program which has not been linked successfully, or
   *     which does not contain objects to form a geometry shader, then an
   *     INVALID_OPERATION error is generated."
   */
   static bool
   check_gs_query(struct gl_context *ctx, const struct gl_shader_program *shProg)
   {
      if (shProg->data->LinkStatus &&
      shProg->_LinkedShaders[MESA_SHADER_GEOMETRY] != NULL) {
               _mesa_error(ctx, GL_INVALID_OPERATION,
            }
         /**
   * Check if a tessellation control shader query is valid at this time.
   * If not, report an error and return false.
   *
   * From GL 4.0 section 6.1.12 (Shader and Program Queries):
   *
   *     "If TESS_CONTROL_OUTPUT_VERTICES is queried for a program which has
   *     not been linked successfully, or which does not contain objects to
   *     form a tessellation control shader, then an INVALID_OPERATION error is
   *     generated."
   */
   static bool
   check_tcs_query(struct gl_context *ctx, const struct gl_shader_program *shProg)
   {
      if (shProg->data->LinkStatus &&
      shProg->_LinkedShaders[MESA_SHADER_TESS_CTRL] != NULL) {
               _mesa_error(ctx, GL_INVALID_OPERATION,
            }
         /**
   * Check if a tessellation evaluation shader query is valid at this time.
   * If not, report an error and return false.
   *
   * From GL 4.0 section 6.1.12 (Shader and Program Queries):
   *
   *     "If any of the pname values in this paragraph are queried for a program
   *     which has not been linked successfully, or which does not contain
   *     objects to form a tessellation evaluation shader, then an
   *     INVALID_OPERATION error is generated."
   *
   */
   static bool
   check_tes_query(struct gl_context *ctx, const struct gl_shader_program *shProg)
   {
      if (shProg->data->LinkStatus &&
      shProg->_LinkedShaders[MESA_SHADER_TESS_EVAL] != NULL) {
               _mesa_error(ctx, GL_INVALID_OPERATION, "glGetProgramv(linked tessellation "
            }
      static bool
   get_shader_program_completion_status(struct gl_context *ctx,
         {
               if (!screen->is_parallel_shader_compilation_finished)
            for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
      struct gl_linked_shader *linked = shprog->_LinkedShaders[i];
            if (!linked || !linked->Program)
            if (linked->Program->variants)
                     if (sh &&
      !screen->is_parallel_shader_compilation_finished(screen, sh, type))
   }
      }
      /**
   * glGetProgramiv() - get shader program state.
   * Note that this is for GLSL shader programs, not ARB vertex/fragment
   * programs (see glGetProgramivARB).
   */
   static void
   get_programiv(struct gl_context *ctx, GLuint program, GLenum pname,
         {
      struct gl_shader_program *shProg
            /* Is transform feedback available in this context?
   */
   const bool has_xfb =
      (_mesa_is_desktop_gl_compat(ctx) && ctx->Extensions.EXT_transform_feedback)
   || _mesa_is_desktop_gl_core(ctx)
         /* True if geometry shaders (of the form that was adopted into GLSL 1.50
   * and GL 3.2) are available in this context
   */
   const bool has_gs = _mesa_has_geometry_shaders(ctx);
            /* Are uniform buffer objects available in this context?
   */
   const bool has_ubo =
      (_mesa_is_desktop_gl_compat(ctx) &&
   ctx->Extensions.ARB_uniform_buffer_object)
   || _mesa_is_desktop_gl_core(ctx)
         if (!shProg) {
                  switch (pname) {
   case GL_DELETE_STATUS:
      *params = shProg->DeletePending;
      case GL_COMPLETION_STATUS_ARB:
      *params = get_shader_program_completion_status(ctx, shProg);
      case GL_LINK_STATUS:
      *params = shProg->data->LinkStatus ? GL_TRUE : GL_FALSE;
      case GL_VALIDATE_STATUS:
      *params = shProg->data->Validated;
      case GL_INFO_LOG_LENGTH:
      *params = (shProg->data->InfoLog && shProg->data->InfoLog[0] != '\0') ?
            case GL_ATTACHED_SHADERS:
      *params = shProg->NumShaders;
      case GL_ACTIVE_ATTRIBUTES:
      *params = _mesa_count_active_attribs(shProg);
      case GL_ACTIVE_ATTRIBUTE_MAX_LENGTH:
      *params = _mesa_longest_attribute_name_length(shProg);
      case GL_ACTIVE_UNIFORMS: {
      _mesa_get_program_interfaceiv(shProg, GL_UNIFORM, GL_ACTIVE_RESOURCES,
            }
   case GL_ACTIVE_UNIFORM_MAX_LENGTH: {
      _mesa_get_program_interfaceiv(shProg, GL_UNIFORM, GL_MAX_NAME_LENGTH,
            }
   case GL_TRANSFORM_FEEDBACK_VARYINGS:
      if (!has_xfb)
            /* Check first if there are transform feedback varyings specified in the
   * shader (ARB_enhanced_layouts). If there isn't any, return the number of
   * varyings specified using the API.
   */
   if (shProg->last_vert_prog &&
      shProg->last_vert_prog->sh.LinkedTransformFeedback->NumVarying > 0)
   *params =
      else
            case GL_TRANSFORM_FEEDBACK_VARYING_MAX_LENGTH: {
      if (!has_xfb)
            _mesa_get_program_interfaceiv(shProg, GL_TRANSFORM_FEEDBACK_VARYING,
            }
   case GL_TRANSFORM_FEEDBACK_BUFFER_MODE:
      if (!has_xfb)
         *params = shProg->TransformFeedback.BufferMode;
      case GL_GEOMETRY_VERTICES_OUT:
      if (!has_gs)
         if (check_gs_query(ctx, shProg)) {
      *params = shProg->_LinkedShaders[MESA_SHADER_GEOMETRY]->
      }
      case GL_GEOMETRY_SHADER_INVOCATIONS:
      if (!has_gs ||
      (_mesa_is_desktop_gl(ctx) && !ctx->Extensions.ARB_gpu_shader5)) {
      }
   if (check_gs_query(ctx, shProg)) {
      *params = shProg->_LinkedShaders[MESA_SHADER_GEOMETRY]->
      }
      case GL_GEOMETRY_INPUT_TYPE:
      if (!has_gs)
         if (check_gs_query(ctx, shProg)) {
      *params = shProg->_LinkedShaders[MESA_SHADER_GEOMETRY]->
      }
      case GL_GEOMETRY_OUTPUT_TYPE:
      if (!has_gs)
         if (check_gs_query(ctx, shProg)) {
      *params = shProg->_LinkedShaders[MESA_SHADER_GEOMETRY]->
      }
      case GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH: {
      if (!has_ubo)
            _mesa_get_program_interfaceiv(shProg, GL_UNIFORM_BLOCK,
            }
   case GL_ACTIVE_UNIFORM_BLOCKS:
      if (!has_ubo)
            *params = shProg->data->NumUniformBlocks;
      case GL_PROGRAM_BINARY_RETRIEVABLE_HINT:
      /* This enum isn't part of the OES extension for OpenGL ES 2.0.  It is
   * only available with desktop OpenGL 3.0+ with the
   * GL_ARB_get_program_binary extension or OpenGL ES 3.0.
   *
   * On desktop, we ignore the 3.0+ requirement because it is silly.
   */
   if (!_mesa_is_desktop_gl(ctx) && !_mesa_is_gles3(ctx))
            *params = shProg->BinaryRetrievableHint;
      case GL_PROGRAM_BINARY_LENGTH:
      if (ctx->Const.NumProgramBinaryFormats == 0 || !shProg->data->LinkStatus) {
         } else {
         }
      case GL_ACTIVE_ATOMIC_COUNTER_BUFFERS:
      if (!ctx->Extensions.ARB_shader_atomic_counters && !_mesa_is_gles31(ctx))
            *params = shProg->data->NumAtomicBuffers;
      case GL_COMPUTE_WORK_GROUP_SIZE: {
      int i;
   if (!_mesa_has_compute_shaders(ctx))
         if (!shProg->data->LinkStatus) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetProgramiv(program not "
            }
   if (shProg->_LinkedShaders[MESA_SHADER_COMPUTE] == NULL) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetProgramiv(no compute "
            }
   for (i = 0; i < 3; i++)
      params[i] = shProg->_LinkedShaders[MESA_SHADER_COMPUTE]->
         }
   case GL_PROGRAM_SEPARABLE:
      /* If the program has not been linked, return initial value 0. */
   *params = (shProg->data->LinkStatus == LINKING_FAILURE) ? 0 : shProg->SeparateShader;
         /* ARB_tessellation_shader */
   case GL_TESS_CONTROL_OUTPUT_VERTICES:
      if (!has_tess)
         if (check_tcs_query(ctx, shProg)) {
      *params = shProg->_LinkedShaders[MESA_SHADER_TESS_CTRL]->
      }
      case GL_TESS_GEN_MODE:
      if (!has_tess)
         if (check_tes_query(ctx, shProg)) {
      const struct gl_linked_shader *tes =
         switch (tes->Program->info.tess._primitive_mode) {
   case TESS_PRIMITIVE_TRIANGLES:
      *params = GL_TRIANGLES;
      case TESS_PRIMITIVE_QUADS:
      *params = GL_QUADS;
      case TESS_PRIMITIVE_ISOLINES:
      *params = GL_ISOLINES;
      case TESS_PRIMITIVE_UNSPECIFIED:
      *params = 0;
         }
      case GL_TESS_GEN_SPACING:
      if (!has_tess)
         if (check_tes_query(ctx, shProg)) {
      const struct gl_linked_shader *tes =
         switch (tes->Program->info.tess.spacing) {
   case TESS_SPACING_EQUAL:
      *params = GL_EQUAL;
      case TESS_SPACING_FRACTIONAL_ODD:
      *params = GL_FRACTIONAL_ODD;
      case TESS_SPACING_FRACTIONAL_EVEN:
      *params = GL_FRACTIONAL_EVEN;
      case TESS_SPACING_UNSPECIFIED:
      *params = 0;
         }
      case GL_TESS_GEN_VERTEX_ORDER:
      if (!has_tess)
         if (check_tes_query(ctx, shProg)) {
      *params = shProg->_LinkedShaders[MESA_SHADER_TESS_EVAL]->
               case GL_TESS_GEN_POINT_MODE:
      if (!has_tess)
         if (check_tes_query(ctx, shProg)) {
      *params = shProg->_LinkedShaders[MESA_SHADER_TESS_EVAL]->
      }
      default:
                  _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramiv(pname=%s)",
      }
         /**
   * glGetShaderiv() - get GLSL shader state
   */
   static void
   get_shaderiv(struct gl_context *ctx, GLuint name, GLenum pname, GLint *params)
   {
      struct gl_shader *shader =
            if (!shader) {
                  switch (pname) {
   case GL_SHADER_TYPE:
      *params = shader->Type;
      case GL_DELETE_STATUS:
      *params = shader->DeletePending;
      case GL_COMPLETION_STATUS_ARB:
      /* _mesa_glsl_compile_shader is not offloaded to other threads. */
   *params = GL_TRUE;
      case GL_COMPILE_STATUS:
      *params = shader->CompileStatus ? GL_TRUE : GL_FALSE;
      case GL_INFO_LOG_LENGTH:
      *params = (shader->InfoLog && shader->InfoLog[0] != '\0') ?
            case GL_SHADER_SOURCE_LENGTH:
      *params = shader->Source ? strlen((char *) shader->Source) + 1 : 0;
      case GL_SPIR_V_BINARY_ARB:
      *params = (shader->spirv_data != NULL);
      default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetShaderiv(pname)");
         }
         static void
   get_program_info_log(struct gl_context *ctx, GLuint program, GLsizei bufSize,
         {
               /* Section 2.5 GL Errors (page 18) of the OpenGL ES 3.0.4 spec and
   * section 2.3.1 (Errors) of the OpenGL 4.5 spec say:
   *
   *     "If a negative number is provided where an argument of type sizei or
   *     sizeiptr is specified, an INVALID_VALUE error is generated."
   */
   if (bufSize < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetProgramInfoLog(bufSize < 0)");
               shProg = _mesa_lookup_shader_program_err(ctx, program,
         if (!shProg) {
                     }
         static void
   get_shader_info_log(struct gl_context *ctx, GLuint shader, GLsizei bufSize,
         {
               /* Section 2.5 GL Errors (page 18) of the OpenGL ES 3.0.4 spec and
   * section 2.3.1 (Errors) of the OpenGL 4.5 spec say:
   *
   *     "If a negative number is provided where an argument of type sizei or
   *     sizeiptr is specified, an INVALID_VALUE error is generated."
   */
   if (bufSize < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetShaderInfoLog(bufSize < 0)");
               sh = _mesa_lookup_shader_err(ctx, shader, "glGetShaderInfoLog(shader)");
   if (!sh) {
                     }
         /**
   * Return shader source code.
   */
   static void
   get_shader_source(struct gl_context *ctx, GLuint shader, GLsizei maxLength,
         {
               if (maxLength < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetShaderSource(bufSize < 0)");
               sh = _mesa_lookup_shader_err(ctx, shader, "glGetShaderSource");
   if (!sh) {
         }
      }
         /**
   * Set/replace shader source code.  A helper function used by
   * glShaderSource[ARB].
   */
   static void
   set_shader_source(struct gl_shader *sh, const GLchar *source,
         {
               /* The GL_ARB_gl_spirv spec adds the following to the end of the description
   * of ShaderSource:
   *
   *   "If <shader> was previously associated with a SPIR-V module (via the
   *    ShaderBinary command), that association is broken. Upon successful
   *    completion of this command the SPIR_V_BINARY_ARB state of <shader>
   *    is set to FALSE."
   */
            if (sh->CompileStatus == COMPILE_SKIPPED && !sh->FallbackSource) {
      /* If shader was previously compiled back-up the source in case of cache
   * fallback.
   */
   sh->FallbackSource = sh->Source;
   memcpy(sh->fallback_source_sha1, sh->source_sha1, SHA1_DIGEST_LENGTH);
      } else {
      /* free old shader source string and install new one */
   free((void *)sh->Source);
                  }
      static void
   ensure_builtin_types(struct gl_context *ctx)
   {
      if (!ctx->shader_builtin_ref) {
      _mesa_glsl_builtin_functions_init_or_ref();
         }
      /**
   * Compile a shader.
   */
   void
   _mesa_compile_shader(struct gl_context *ctx, struct gl_shader *sh)
   {
      if (!sh)
            /* The GL_ARB_gl_spirv spec says:
   *
   *    "Add a new error for the CompileShader command:
   *
   *      An INVALID_OPERATION error is generated if the SPIR_V_BINARY_ARB
   *      state of <shader> is TRUE."
   */
   if (sh->spirv_data) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glCompileShader(SPIR-V)");
               if (!sh->Source) {
      /* If the user called glCompileShader without first calling
   * glShaderSource, we should fail to compile, but not raise a GL_ERROR.
   */
      } else {
      if (ctx->_Shader->Flags & (GLSL_DUMP | GLSL_SOURCE)) {
      _mesa_log("GLSL source for %s shader %d:\n",
                              /* this call will set the shader->CompileStatus field to indicate if
   * compilation was successful.
   */
            if (ctx->_Shader->Flags & GLSL_LOG) {
                  if (ctx->_Shader->Flags & GLSL_DUMP) {
      if (sh->CompileStatus) {
      if (sh->ir) {
      _mesa_log("GLSL IR for shader %d:\n", sh->Name);
      } else {
      _mesa_log("No GLSL IR for shader %d (shader may be from "
      }
      } else {
         }
   if (sh->InfoLog && sh->InfoLog[0] != 0) {
      _mesa_log("GLSL shader %d info log:\n", sh->Name);
                     if (!sh->CompileStatus) {
      if (ctx->_Shader->Flags & GLSL_DUMP_ON_ERROR) {
      _mesa_log("GLSL source for %s shader %d:\n",
         _mesa_log("%s\n", sh->Source);
               if (ctx->_Shader->Flags & GLSL_REPORT_ERRORS) {
      _mesa_debug(ctx, "Error compiling shader %u:\n%s\n",
            }
         struct update_programs_in_pipeline_params
   {
      struct gl_context *ctx;
      };
      static void
   update_programs_in_pipeline(void *data, void *userData)
   {
      struct update_programs_in_pipeline_params *params =
                  for (unsigned stage = 0; stage < MESA_SHADER_STAGES; stage++) {
      if (obj->CurrentProgram[stage] &&
      obj->CurrentProgram[stage]->Id == params->shProg->Name) {
   struct gl_program *prog = params->shProg->_LinkedShaders[stage]->Program;
            }
         /**
   * Link a program's shaders.
   */
   static ALWAYS_INLINE void
   link_program(struct gl_context *ctx, struct gl_shader_program *shProg,
         {
      if (!shProg)
                     if (!no_error) {
      /* From the ARB_transform_feedback2 specification:
   * "The error INVALID_OPERATION is generated by LinkProgram if <program>
   * is the name of a program being used by one or more transform feedback
   * objects, even if the objects are not currently bound or are paused."
   */
   if (_mesa_transform_feedback_is_using_program(ctx, shProg)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                        unsigned programs_in_use = 0;
   if (ctx->_Shader)
      for (unsigned stage = 0; stage < MESA_SHADER_STAGES; stage++) {
      if (ctx->_Shader->CurrentProgram[stage] &&
      ctx->_Shader->CurrentProgram[stage]->Id == shProg->Name) {
                        FLUSH_VERTICES(ctx, 0, 0);
            /* From section 7.3 (Program Objects) of the OpenGL 4.5 spec:
   *
   *    "If LinkProgram or ProgramBinary successfully re-links a program
   *     object that is active for any shader stage, then the newly generated
   *     executable code will be installed as part of the current rendering
   *     state for all shader stages where the program is active.
   *     Additionally, the newly generated executable code is made part of
   *     the state of any program pipeline for all stages where the program
   *     is attached."
   */
   if (shProg->data->LinkStatus) {
      while (programs_in_use) {
               struct gl_program *prog = NULL;
                              if (ctx->Pipeline.Objects) {
      struct update_programs_in_pipeline_params params = {
      .ctx = ctx,
      };
   _mesa_HashWalk(ctx->Pipeline.Objects, update_programs_in_pipeline,
               #ifndef CUSTOM_SHADER_REPLACEMENT
      /* Capture .shader_test files. */
   const char *capture_path = _mesa_get_shader_capture_path();
   if (shProg->Name != 0 && shProg->Name != ~0 && capture_path != NULL) {
      /* Find an unused filename. */
   FILE *file = NULL;
   char *filename = NULL;
   for (unsigned i = 0;; i++) {
      if (i) {
      filename = ralloc_asprintf(NULL, "%s/%u-%u.shader_test",
      } else {
      filename = ralloc_asprintf(NULL, "%s/%u.shader_test",
      }
   file = os_file_create_unique(filename, 0644);
   if (file)
         /* If we are failing for another reason than "this filename already
   * exists", we are likely to fail again with another filename, so
   * let's just give up */
   if (errno != EEXIST)
            }
   if (file) {
      fprintf(file, "[require]\nGLSL%s >= %u.%02u\n",
         shProg->IsES ? " ES" : "", shProg->GLSL_Version / 100,
   if (shProg->SeparateShader)
                  for (unsigned i = 0; i < shProg->NumShaders; i++) {
      fprintf(file, "[%s shader]\n%s\n",
            }
      } else {
                        #endif
         if (shProg->data->LinkStatus == LINKING_FAILURE &&
      (ctx->_Shader->Flags & GLSL_REPORT_ERRORS)) {
   _mesa_debug(ctx, "Error linking program %u:\n%s\n",
               _mesa_update_vertex_processing_mode(ctx);
                     /* debug code */
   if (0) {
               printf("Link %u shaders in program %u: %s\n",
                  for (i = 0; i < shProg->NumShaders; i++) {
      printf(" shader %u, stage %u\n",
                  }
         static void
   link_program_error(struct gl_context *ctx, struct gl_shader_program *shProg)
   {
         }
         static void
   link_program_no_error(struct gl_context *ctx, struct gl_shader_program *shProg)
   {
         }
         void
   _mesa_link_program(struct gl_context *ctx, struct gl_shader_program *shProg)
   {
         }
         /**
   * Print basic shader info (for debug).
   */
   static void
   print_shader_info(const struct gl_shader_program *shProg)
   {
               printf("Mesa: glUseProgram(%u)\n", shProg->Name);
   for (i = 0; i < shProg->NumShaders; i++) {
      printf("  %s shader %u\n",
            }
   if (shProg->_LinkedShaders[MESA_SHADER_VERTEX])
      printf("  vert prog %u\n",
      if (shProg->_LinkedShaders[MESA_SHADER_FRAGMENT])
      printf("  frag prog %u\n",
      if (shProg->_LinkedShaders[MESA_SHADER_GEOMETRY])
      printf("  geom prog %u\n",
      if (shProg->_LinkedShaders[MESA_SHADER_TESS_CTRL])
      printf("  tesc prog %u\n",
      if (shProg->_LinkedShaders[MESA_SHADER_TESS_EVAL])
      printf("  tese prog %u\n",
   }
         /**
   * Use the named shader program for subsequent glUniform calls
   */
   void
   _mesa_active_program(struct gl_context *ctx, struct gl_shader_program *shProg,
         {
      if ((shProg != NULL) && !shProg->data->LinkStatus) {
         "%s(program %u not linked)", caller, shProg->Name);
                  if (ctx->Shader.ActiveProgram != shProg) {
      _mesa_reference_shader_program(ctx, &ctx->Shader.ActiveProgram, shProg);
         }
         /**
   * Use the named shader program for subsequent rendering.
   */
   void
   _mesa_use_shader_program(struct gl_context *ctx,
         {
      for (int i = 0; i < MESA_SHADER_STAGES; i++) {
      struct gl_program *new_prog = NULL;
   if (shProg && shProg->_LinkedShaders[i])
            }
      }
         /**
   * Do validation of the given shader program.
   * \param errMsg  returns error message if validation fails.
   * \return GL_TRUE if valid, GL_FALSE if invalid (and set errMsg)
   */
   static GLboolean
   validate_shader_program(const struct gl_shader_program *shProg,
         {
      if (!shProg->data->LinkStatus) {
                           any two active samplers in the current program object are of
            any active sampler in the current program object refers to a texture
   image unit where fixed-function fragment processing accesses a
            the sum of the number of active samplers in the program and the
   number of texture image units enabled for fixed-function fragment
   processing exceeds the combined limit on the total number of texture
   image units allowed.
            /*
   * Check: any two active samplers in the current program object are of
   * different types, but refer to the same texture image unit,
   */
   if (!_mesa_sampler_uniforms_are_valid(shProg, errMsg, 100))
               }
         /**
   * Called via glValidateProgram()
   */
   static void
   validate_program(struct gl_context *ctx, GLuint program)
   {
      struct gl_shader_program *shProg;
            shProg = _mesa_lookup_shader_program_err(ctx, program, "glValidateProgram");
   if (!shProg) {
                  shProg->data->Validated = validate_shader_program(shProg, errMsg);
   if (!shProg->data->Validated) {
      /* update info log */
   if (shProg->data->InfoLog) {
         }
         }
         void GLAPIENTRY
   _mesa_AttachObjectARB_no_error(GLhandleARB program, GLhandleARB shader)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_AttachObjectARB(GLhandleARB program, GLhandleARB shader)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_AttachShader_no_error(GLuint program, GLuint shader)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_AttachShader(GLuint program, GLuint shader)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_CompileShader(GLuint shaderObj)
   {
      GET_CURRENT_CONTEXT(ctx);
   if (MESA_VERBOSE & VERBOSE_API)
         _mesa_compile_shader(ctx, _mesa_lookup_shader_err(ctx, shaderObj,
      }
         GLuint GLAPIENTRY
   _mesa_CreateShader_no_error(GLenum type)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         GLuint GLAPIENTRY
   _mesa_CreateShader(GLenum type)
   {
               if (MESA_VERBOSE & VERBOSE_API)
               }
         GLhandleARB GLAPIENTRY
   _mesa_CreateShaderObjectARB_no_error(GLenum type)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         GLhandleARB GLAPIENTRY
   _mesa_CreateShaderObjectARB(GLenum type)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         GLuint GLAPIENTRY
   _mesa_CreateProgram(void)
   {
      GET_CURRENT_CONTEXT(ctx);
   if (MESA_VERBOSE & VERBOSE_API)
            }
         GLhandleARB GLAPIENTRY
   _mesa_CreateProgramObjectARB(void)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_DeleteObjectARB(GLhandleARB obj)
   {
      if (MESA_VERBOSE & VERBOSE_API) {
      GET_CURRENT_CONTEXT(ctx);
               if (obj) {
      GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0, 0);
   if (is_program(ctx, obj)) {
         }
   else if (is_shader(ctx, obj)) {
         }
   else {
               }
         void GLAPIENTRY
   _mesa_DeleteProgram(GLuint name)
   {
      if (name) {
      GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0, 0);
         }
         void GLAPIENTRY
   _mesa_DeleteShader(GLuint name)
   {
      if (name) {
      GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0, 0);
         }
         void GLAPIENTRY
   _mesa_DetachObjectARB_no_error(GLhandleARB program, GLhandleARB shader)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_DetachObjectARB(GLhandleARB program, GLhandleARB shader)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_DetachShader_no_error(GLuint program, GLuint shader)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_DetachShader(GLuint program, GLuint shader)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_GetAttachedObjectsARB(GLhandleARB container, GLsizei maxCount,
         {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_GetAttachedShaders(GLuint program, GLsizei maxCount,
         {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_GetInfoLogARB(GLhandleARB object, GLsizei maxLength, GLsizei * length,
         {
      GET_CURRENT_CONTEXT(ctx);
   if (is_program(ctx, object)) {
         }
   else if (is_shader(ctx, object)) {
         }
   else {
            }
         void GLAPIENTRY
   _mesa_GetObjectParameterivARB(GLhandleARB object, GLenum pname, GLint *params)
   {
      GET_CURRENT_CONTEXT(ctx);
   /* Implement in terms of GetProgramiv, GetShaderiv */
   if (is_program(ctx, object)) {
      *params = GL_PROGRAM_OBJECT_ARB;
         }
   get_programiv(ctx, object, pname, params);
            }
   else if (is_shader(ctx, object)) {
      *params = GL_SHADER_OBJECT_ARB;
         }
   get_shaderiv(ctx, object, pname, params);
            }
   else {
            }
         void GLAPIENTRY
   _mesa_GetObjectParameterfvARB(GLhandleARB object, GLenum pname,
         {
      GLint iparams[1] = {0};  /* XXX is one element enough? */
   _mesa_GetObjectParameterivARB(object, pname, iparams);
      }
         void GLAPIENTRY
   _mesa_GetProgramiv(GLuint program, GLenum pname, GLint *params)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_GetShaderiv(GLuint shader, GLenum pname, GLint *params)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_GetProgramInfoLog(GLuint program, GLsizei bufSize,
         {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_GetShaderInfoLog(GLuint shader, GLsizei bufSize,
         {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_GetShaderSource(GLuint shader, GLsizei maxLength,
         {
      GET_CURRENT_CONTEXT(ctx);
      }
         GLhandleARB GLAPIENTRY
   _mesa_GetHandleARB(GLenum pname)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         GLboolean GLAPIENTRY
   _mesa_IsProgram(GLuint name)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         GLboolean GLAPIENTRY
   _mesa_IsShader(GLuint name)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_LinkProgram_no_error(GLuint programObj)
   {
               struct gl_shader_program *shProg =
            }
         void GLAPIENTRY
   _mesa_LinkProgram(GLuint programObj)
   {
               if (MESA_VERBOSE & VERBOSE_API)
            struct gl_shader_program *shProg =
            }
      #ifdef ENABLE_SHADER_CACHE
      /**
   * Construct a full path for shader replacement functionality using
   * following format:
   *
   * <path>/<stage prefix>_<CHECKSUM>.glsl
   * <path>/<stage prefix>_<CHECKSUM>.arb
   */
   static char *
   construct_name(const gl_shader_stage stage, const char *sha,
         {
      static const char *types[] = {
                              }
      /**
   * Write given shader source to a file in MESA_SHADER_DUMP_PATH.
   */
   void
   _mesa_dump_shader_source(const gl_shader_stage stage, const char *source,
         {
   #ifndef CUSTOM_SHADER_REPLACEMENT
      static bool path_exists = true;
   char *dump_path;
   FILE *f;
            if (!path_exists)
            dump_path = secure_getenv("MESA_SHADER_DUMP_PATH");
   if (!dump_path) {
      path_exists = false;
               _mesa_sha1_format(sha, sha1);
            f = fopen(name, "w");
   if (f) {
      fputs(source, f);
      } else {
      GET_CURRENT_CONTEXT(ctx);
   _mesa_warning(ctx, "could not open %s for dumping shader (%s)", name,
      }
      #endif
   }
      /**
   * Read shader source code from a file.
   * Useful for debugging to override an app's shader.
   */
   GLcharARB *
   _mesa_read_shader_source(const gl_shader_stage stage, const char *source,
         {
      char *read_path;
   static bool path_exists = true;
   int len, shader_size = 0;
   GLcharARB *buffer;
   FILE *f;
                     if (!debug_get_bool_option("MESA_NO_SHADER_REPLACEMENT", false)) {
               char *new_source = try_direct_replace(process_name, source);
   if (new_source)
            for (size_t i = 0; i < ARRAY_SIZE(shader_replacements); i++) {
                     if (shader_replacements[i].app &&
                                                if (!path_exists)
            read_path = getenv("MESA_SHADER_READ_PATH");
   if (!read_path) {
      path_exists = false;
               char *name = construct_name(stage, sha, source, read_path);
   f = fopen(name, "r");
   ralloc_free(name);
   if (!f)
            /* allocate enough room for the entire shader */
   fseek(f, 0, SEEK_END);
   shader_size = ftell(f);
   rewind(f);
            /* add one for terminating zero */
            buffer = malloc(shader_size);
            len = fread(buffer, 1, shader_size, f);
                        }
      #endif /* ENABLE_SHADER_CACHE */
      /**
   * Called via glShaderSource() and glShaderSourceARB() API functions.
   * Basically, concatenate the source code strings into one long string
   * and pass it to _mesa_shader_source().
   */
   static ALWAYS_INLINE void
   shader_source(struct gl_context *ctx, GLuint shaderObj, GLsizei count,
         {
      GLint *offsets;
   GLsizei i, totalLength;
   GLcharARB *source;
            if (!no_error) {
      sh = _mesa_lookup_shader_err(ctx, shaderObj, "glShaderSourceARB");
   if (!sh)
            if (string == NULL || count < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glShaderSourceARB");
         } else {
                  /* Return silently the spec doesn't define this as an error */
   if (count == 0)
            /*
   * This array holds offsets of where the appropriate string ends, thus the
   * last element will be set to the total length of the source code.
   */
   offsets = calloc(count, sizeof(GLint));
   if (offsets == NULL) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glShaderSourceARB");
               for (i = 0; i < count; i++) {
      if (!no_error && string[i] == NULL) {
      free((GLvoid *) offsets);
   _mesa_error(ctx, GL_INVALID_OPERATION,
            }
   if (length == NULL || length[i] < 0)
         else
         /* accumulate string lengths */
   if (i > 0)
               /* Total length of source string is sum off all strings plus two.
   * One extra byte for terminating zero, another extra byte to silence
   * valgrind warnings in the parser/grammer code.
   */
   totalLength = offsets[count - 1] + 2;
   source = malloc(totalLength * sizeof(GLcharARB));
   if (source == NULL) {
      free((GLvoid *) offsets);
   _mesa_error(ctx, GL_OUT_OF_MEMORY, "glShaderSourceARB");
               for (i = 0; i < count; i++) {
      GLint start = (i > 0) ? offsets[i - 1] : 0;
   memcpy(source + start, string[i],
      }
   source[totalLength - 1] = '\0';
            /* Compute the original source sha1 before shader replacement. */
   uint8_t original_sha1[SHA1_DIGEST_LENGTH];
         #ifdef ENABLE_SHADER_CACHE
               /* Dump original shader source to MESA_SHADER_DUMP_PATH and replace
   * if corresponding entry found from MESA_SHADER_READ_PATH.
   */
            replacement = _mesa_read_shader_source(sh->Stage, source, original_sha1);
   if (replacement) {
      free(source);
         #endif /* ENABLE_SHADER_CACHE */
                     }
         void GLAPIENTRY
   _mesa_ShaderSource_no_error(GLuint shaderObj, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_ShaderSource(GLuint shaderObj, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
      }
         static ALWAYS_INLINE void
   use_program(GLuint program, bool no_error)
   {
      GET_CURRENT_CONTEXT(ctx);
            if (MESA_VERBOSE & VERBOSE_API)
            if (no_error) {
      if (program) {
            } else {
      if (_mesa_is_xfb_active_and_unpaused(ctx)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     if (program) {
      shProg =
                        if (!shProg->data->LinkStatus) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     /* debug code */
   if (ctx->_Shader->Flags & GLSL_USE_PROG) {
                        /* The ARB_separate_shader_object spec says:
   *
   *     "The executable code for an individual shader stage is taken from
   *     the current program for that stage.  If there is a current program
   *     object established by UseProgram, that program is considered current
   *     for all stages.  Otherwise, if there is a bound program pipeline
   *     object (section 2.14.PPO), the program bound to the appropriate
   *     stage of the pipeline object is considered current."
   */
   if (shProg) {
      /* Attach shader state to the binding point */
   _mesa_reference_pipeline_object(ctx, &ctx->_Shader, &ctx->Shader);
   /* Update the program */
      } else {
      /* Must be done first: detach the progam */
   _mesa_use_shader_program(ctx, shProg);
   /* Unattach shader_state binding point */
   _mesa_reference_pipeline_object(ctx, &ctx->_Shader,
         /* If a pipeline was bound, rebind it */
   if (ctx->Pipeline.Current) {
      if (no_error)
         else
                     }
         void GLAPIENTRY
   _mesa_UseProgram_no_error(GLuint program)
   {
         }
         void GLAPIENTRY
   _mesa_UseProgram(GLuint program)
   {
         }
         void GLAPIENTRY
   _mesa_ValidateProgram(GLuint program)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         /**
   * For OpenGL ES 2.0, GL_ARB_ES2_compatibility
   */
   void GLAPIENTRY
   _mesa_GetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype,
         {
      const struct gl_program_constants *limits;
   const struct gl_precision *p;
            switch (shadertype) {
   case GL_VERTEX_SHADER:
      limits = &ctx->Const.Program[MESA_SHADER_VERTEX];
      case GL_FRAGMENT_SHADER:
      limits = &ctx->Const.Program[MESA_SHADER_FRAGMENT];
      default:
      _mesa_error(ctx, GL_INVALID_ENUM,
                     switch (precisiontype) {
   case GL_LOW_FLOAT:
      p = &limits->LowFloat;
      case GL_MEDIUM_FLOAT:
      p = &limits->MediumFloat;
      case GL_HIGH_FLOAT:
      p = &limits->HighFloat;
      case GL_LOW_INT:
      p = &limits->LowInt;
      case GL_MEDIUM_INT:
      p = &limits->MediumInt;
      case GL_HIGH_INT:
      p = &limits->HighInt;
      default:
      _mesa_error(ctx, GL_INVALID_ENUM,
                     range[0] = p->RangeMin;
   range[1] = p->RangeMax;
      }
         /**
   * For OpenGL ES 2.0, GL_ARB_ES2_compatibility
   */
   void GLAPIENTRY
   _mesa_ReleaseShaderCompiler(void)
   {
               if (ctx->shader_builtin_ref) {
      _mesa_glsl_builtin_functions_decref();
         }
         /**
   * For OpenGL ES 2.0, GL_ARB_ES2_compatibility
   */
   void GLAPIENTRY
   _mesa_ShaderBinary(GLint n, const GLuint* shaders, GLenum binaryformat,
         {
      GET_CURRENT_CONTEXT(ctx);
            /* Page 68, section 7.2 'Shader Binaries" of the of the OpenGL ES 3.1, and
   * page 88 of the OpenGL 4.5 specs state:
   *
   *     "An INVALID_VALUE error is generated if count or length is negative.
   *      An INVALID_ENUM error is generated if binaryformat is not a supported
   *      format returned in SHADER_BINARY_FORMATS."
   */
   if (n < 0 || length < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glShaderBinary(count or length < 0)");
               /* Get all shader objects at once so we can make the operation
   * all-or-nothing.
   */
   if (n > SIZE_MAX / sizeof(*sh)) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glShaderBinary(count)");
               sh = alloca(sizeof(*sh) * (size_t)n);
   if (!sh) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glShaderBinary");
               for (int i = 0; i < n; ++i) {
      sh[i] = _mesa_lookup_shader_err(ctx, shaders[i], "glShaderBinary");
   if (!sh[i])
               if (binaryformat == GL_SHADER_BINARY_FORMAT_SPIR_V_ARB) {
      if (!ctx->Extensions.ARB_gl_spirv) {
         } else if (n > 0) {
      _mesa_spirv_shader_binary(ctx, (unsigned) n, sh, binary,
                              }
         void GLAPIENTRY
   _mesa_GetProgramBinary(GLuint program, GLsizei bufSize, GLsizei *length,
         {
      struct gl_shader_program *shProg;
   GLsizei length_dummy;
            if (bufSize < 0){
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetProgramBinary(bufSize < 0)");
               shProg = _mesa_lookup_shader_program_err(ctx, program, "glGetProgramBinary");
   if (!shProg)
            /* The ARB_get_program_binary spec says:
   *
   *     "If <length> is NULL, then no length is returned."
   *
   * Ensure that length always points to valid storage to avoid multiple NULL
   * pointer checks below.
   */
   if (length == NULL)
               /* The ARB_get_program_binary spec says:
   *
   *     "When a program object's LINK_STATUS is FALSE, its program binary
   *     length is zero, and a call to GetProgramBinary will generate an
   *     INVALID_OPERATION error.
   */
   if (!shProg->data->LinkStatus) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
               *length = 0;
               if (ctx->Const.NumProgramBinaryFormats == 0) {
      *length = 0;
   _mesa_error(ctx, GL_INVALID_OPERATION,
      } else {
      _mesa_get_program_binary(ctx, shProg, bufSize, length, binaryFormat,
               }
      void GLAPIENTRY
   _mesa_ProgramBinary(GLuint program, GLenum binaryFormat,
         {
      struct gl_shader_program *shProg;
            shProg = _mesa_lookup_shader_program_err(ctx, program, "glProgramBinary");
   if (!shProg)
            _mesa_clear_shader_program_data(ctx, shProg);
            /* Section 2.3.1 (Errors) of the OpenGL 4.5 spec says:
   *
   *     "If a negative number is provided where an argument of type sizei or
   *     sizeiptr is specified, an INVALID_VALUE error is generated."
   */
   if (length < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glProgramBinary(length < 0)");
               if (ctx->Const.NumProgramBinaryFormats == 0 ||
      binaryFormat != GL_PROGRAM_BINARY_FORMAT_MESA) {
   /* The ARB_get_program_binary spec says:
   *
   *     "<binaryFormat> and <binary> must be those returned by a previous
   *     call to GetProgramBinary, and <length> must be the length of the
   *     program binary as returned by GetProgramBinary or GetProgramiv with
   *     <pname> PROGRAM_BINARY_LENGTH. Loading the program binary will fail,
   *     setting the LINK_STATUS of <program> to FALSE, if these conditions
   *     are not met."
   *
   * Since any value of binaryFormat passed "is not one of those specified as
   * allowable for [this] command, an INVALID_ENUM error is generated."
   */
   shProg->data->LinkStatus = LINKING_FAILURE;
      } else {
            }
         static ALWAYS_INLINE void
   program_parameteri(struct gl_context *ctx, struct gl_shader_program *shProg,
         {
      switch (pname) {
   case GL_PROGRAM_BINARY_RETRIEVABLE_HINT:
      /* This enum isn't part of the OES extension for OpenGL ES 2.0, but it
   * is part of OpenGL ES 3.0.  For the ES2 case, this function shouldn't
   * even be in the dispatch table, so we shouldn't need to expclicitly
   * check here.
   *
   * On desktop, we ignore the 3.0+ requirement because it is silly.
            /* The ARB_get_program_binary extension spec says:
   *
   *     "An INVALID_VALUE error is generated if the <value> argument to
   *     ProgramParameteri is not TRUE or FALSE."
   */
   if (!no_error && value != GL_TRUE && value != GL_FALSE) {
                  /* No need to notify the driver.  Any changes will actually take effect
   * the next time the shader is linked.
   *
   * The ARB_get_program_binary extension spec says:
   *
   *     "To indicate that a program binary is likely to be retrieved,
   *     ProgramParameteri should be called with <pname>
   *     PROGRAM_BINARY_RETRIEVABLE_HINT and <value> TRUE. This setting
   *     will not be in effect until the next time LinkProgram or
   *     ProgramBinary has been called successfully."
   *
   * The resolution of issue 9 in the extension spec also says:
   *
   *     "The application may use the PROGRAM_BINARY_RETRIEVABLE_HINT hint
   *     to indicate to the GL implementation that this program will
   *     likely be saved with GetProgramBinary at some point. This will
   *     give the GL implementation the opportunity to track any state
   *     changes made to the program before being saved such that when it
   *     is loaded again a recompile can be avoided."
   */
   shProg->BinaryRetrievableHintPending = value;
         case GL_PROGRAM_SEPARABLE:
      /* Spec imply that the behavior is the same as ARB_get_program_binary
   * Chapter 7.3 Program Objects
   */
   if (!no_error && value != GL_TRUE && value != GL_FALSE) {
         }
   shProg->SeparateShader = value;
         default:
      if (!no_error) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glProgramParameteri(pname=%s)",
      }
            invalid_value:
      _mesa_error(ctx, GL_INVALID_VALUE,
               "glProgramParameteri(pname=%s, value=%d): "
      }
         void GLAPIENTRY
   _mesa_ProgramParameteri_no_error(GLuint program, GLenum pname, GLint value)
   {
               struct gl_shader_program *shProg = _mesa_lookup_shader_program(ctx, program);
      }
         void GLAPIENTRY
   _mesa_ProgramParameteri(GLuint program, GLenum pname, GLint value)
   {
      struct gl_shader_program *shProg;
            shProg = _mesa_lookup_shader_program_err(ctx, program,
         if (!shProg)
               }
         void
   _mesa_use_program(struct gl_context *ctx, gl_shader_stage stage,
               {
               target = &shTarget->CurrentProgram[stage];
   if (prog) {
                  if (*target != prog) {
      /* Program is current, flush it */
   if (shTarget == ctx->_Shader) {
                  _mesa_reference_shader_program(ctx,
               _mesa_reference_program(ctx, target, prog);
   _mesa_update_allow_draw_out_of_order(ctx);
   _mesa_update_valid_to_render_state(ctx);
   if (stage == MESA_SHADER_VERTEX)
                  }
         /**
   * Copy program-specific data generated by linking from the gl_shader_program
   * object to the gl_program object referred to by the gl_linked_shader.
   *
   * This function expects _mesa_reference_program() to have been previously
   * called setting the gl_linked_shaders program reference.
   */
   void
   _mesa_copy_linked_program_data(const struct gl_shader_program *src,
         {
                                 switch (dst_sh->Stage) {
   case MESA_SHADER_GEOMETRY: {
      dst->info.gs.vertices_in = src->Geom.VerticesIn;
   dst->info.gs.uses_end_primitive = src->Geom.UsesEndPrimitive;
   dst->info.gs.active_stream_mask = src->Geom.ActiveStreamMask;
      }
   case MESA_SHADER_FRAGMENT: {
      dst->info.fs.depth_layout = src->FragDepthLayout;
      }
   default:
            }
      /**
   * ARB_separate_shader_objects: Compile & Link Program
   */
   GLuint
   _mesa_CreateShaderProgramv_impl(struct gl_context *ctx,
               {
      const GLuint shader = create_shader_err(ctx, type, "glCreateShaderProgramv");
            /*
   * According to OpenGL 4.5 and OpenGL ES 3.1 standards, section 7.3:
   * GL_INVALID_VALUE should be generated if count < 0
   */
   if (count < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glCreateShaderProgram (count < 0)");
               if (shader) {
               _mesa_ShaderSource(shader, count, strings, NULL);
            program = create_shader_program(ctx);
   struct gl_shader_program *shProg;
   GLint compiled = GL_FALSE;
      shProg = _mesa_lookup_shader_program(ctx, program);
      shProg->SeparateShader = GL_TRUE;
      get_shaderiv(ctx, shader, GL_COMPILE_STATUS, &compiled);
   if (compiled) {
      attach_shader_err(ctx, program, shader, "glCreateShaderProgramv");
   _mesa_link_program(ctx, shProg);
         #if 0
      /* Possibly... */
   if (active-user-defined-varyings-in-linked-program) {
      append-error-to-info-log;
         #endif
   }
            if (sh->InfoLog)
                              }
      /**
   * ARB_separate_shader_objects: Compile & Link Program
   */
   GLuint GLAPIENTRY
   _mesa_CreateShaderProgramv(GLenum type, GLsizei count,
         {
                  }
      static void
   set_patch_vertices(struct gl_context *ctx, GLint value)
   {
      if (ctx->TessCtrlProgram.patch_vertices != value) {
      FLUSH_VERTICES(ctx, 0, GL_CURRENT_BIT);
   ctx->NewDriverState |= ST_NEW_TESS_STATE;
         }
      /**
   * For GL_ARB_tessellation_shader
   */
   void GLAPIENTRY
   _mesa_PatchParameteri_no_error(GLenum pname, GLint value)
   {
                  }
         extern void GLAPIENTRY
   _mesa_PatchParameteri(GLenum pname, GLint value)
   {
               if (!_mesa_has_tessellation(ctx)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glPatchParameteri");
               if (pname != GL_PATCH_VERTICES) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glPatchParameteri");
               if (value <= 0 || value > ctx->Const.MaxPatchVertices) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glPatchParameteri");
                  }
         extern void GLAPIENTRY
   _mesa_PatchParameterfv(GLenum pname, const GLfloat *values)
   {
               if (!_mesa_has_tessellation(ctx)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glPatchParameterfv");
               switch(pname) {
   case GL_PATCH_DEFAULT_OUTER_LEVEL:
      FLUSH_VERTICES(ctx, 0, 0);
   memcpy(ctx->TessCtrlProgram.patch_default_outer_level, values,
         ctx->NewDriverState |= ST_NEW_TESS_STATE;
      case GL_PATCH_DEFAULT_INNER_LEVEL:
      FLUSH_VERTICES(ctx, 0, 0);
   memcpy(ctx->TessCtrlProgram.patch_default_inner_level, values,
         ctx->NewDriverState |= ST_NEW_TESS_STATE;
      default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glPatchParameterfv");
         }
      /**
   * ARB_shader_subroutine
   */
   GLint GLAPIENTRY
   _mesa_GetSubroutineUniformLocation(GLuint program, GLenum shadertype,
         {
      GET_CURRENT_CONTEXT(ctx);
   const char *api_name = "glGetSubroutineUniformLocation";
   struct gl_shader_program *shProg;
   GLenum resource_type;
            if (!_mesa_validate_shader_target(ctx, shadertype)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s", api_name);
               shProg = _mesa_lookup_shader_program_err(ctx, program, api_name);
   if (!shProg)
            stage = _mesa_shader_enum_to_shader_stage(shadertype);
   if (!shProg->_LinkedShaders[stage]) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s", api_name);
               resource_type = _mesa_shader_stage_to_subroutine_uniform(stage);
      }
      GLuint GLAPIENTRY
   _mesa_GetSubroutineIndex(GLuint program, GLenum shadertype,
         {
      GET_CURRENT_CONTEXT(ctx);
   const char *api_name = "glGetSubroutineIndex";
   struct gl_shader_program *shProg;
   struct gl_program_resource *res;
   GLenum resource_type;
            if (!_mesa_validate_shader_target(ctx, shadertype)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s", api_name);
               shProg = _mesa_lookup_shader_program_err(ctx, program, api_name);
   if (!shProg)
            stage = _mesa_shader_enum_to_shader_stage(shadertype);
   if (!shProg->_LinkedShaders[stage]) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s", api_name);
               resource_type = _mesa_shader_stage_to_subroutine(stage);
   res = _mesa_program_resource_find_name(shProg, resource_type, name, NULL);
   if (!res) {
   return -1;
               }
         GLvoid GLAPIENTRY
   _mesa_GetActiveSubroutineUniformiv(GLuint program, GLenum shadertype,
         {
      GET_CURRENT_CONTEXT(ctx);
   const char *api_name = "glGetActiveSubroutineUniformiv";
   struct gl_shader_program *shProg;
   struct gl_linked_shader *sh;
   gl_shader_stage stage;
   struct gl_program_resource *res;
   const struct gl_uniform_storage *uni;
   GLenum resource_type;
            if (!_mesa_validate_shader_target(ctx, shadertype)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s", api_name);
               shProg = _mesa_lookup_shader_program_err(ctx, program, api_name);
   if (!shProg)
            stage = _mesa_shader_enum_to_shader_stage(shadertype);
            sh = shProg->_LinkedShaders[stage];
   if (!sh) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s", api_name);
               struct gl_program *p = shProg->_LinkedShaders[stage]->Program;
   if (index >= p->sh.NumSubroutineUniforms) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s: invalid index greater than GL_ACTIVE_SUBROUTINE_UNIFORMS", api_name);
               switch (pname) {
   case GL_NUM_COMPATIBLE_SUBROUTINES: {
      res = _mesa_program_resource_find_index(shProg, resource_type, index);
   if (res) {
      uni = res->Data;
      }
      }
   case GL_COMPATIBLE_SUBROUTINES: {
      res = _mesa_program_resource_find_index(shProg, resource_type, index);
   if (res) {
      uni = res->Data;
   count = 0;
   for (i = 0; i < p->sh.NumSubroutineFunctions; i++) {
      struct gl_subroutine_function *fn = &p->sh.SubroutineFunctions[i];
   for (j = 0; j < fn->num_compat_types; j++) {
      if (fn->types[j] == uni->type) {
      values[count++] = i;
               }
      }
   case GL_UNIFORM_SIZE:
      res = _mesa_program_resource_find_index(shProg, resource_type, index);
   if (res) {
      uni = res->Data;
      }
      case GL_UNIFORM_NAME_LENGTH:
      res = _mesa_program_resource_find_index(shProg, resource_type, index);
   if (res) {
      values[0] = _mesa_program_resource_name_length(res) + 1
      }
      default:
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s", api_name);
         }
         GLvoid GLAPIENTRY
   _mesa_GetActiveSubroutineUniformName(GLuint program, GLenum shadertype,
               {
      GET_CURRENT_CONTEXT(ctx);
   const char *api_name = "glGetActiveSubroutineUniformName";
   struct gl_shader_program *shProg;
   GLenum resource_type;
            if (!_mesa_validate_shader_target(ctx, shadertype)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s", api_name);
               shProg = _mesa_lookup_shader_program_err(ctx, program, api_name);
   if (!shProg)
            stage = _mesa_shader_enum_to_shader_stage(shadertype);
   if (!shProg->_LinkedShaders[stage]) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s", api_name);
               resource_type = _mesa_shader_stage_to_subroutine_uniform(stage);
   /* get program resource name */
   _mesa_get_program_resource_name(shProg, resource_type,
            }
         GLvoid GLAPIENTRY
   _mesa_GetActiveSubroutineName(GLuint program, GLenum shadertype,
               {
      GET_CURRENT_CONTEXT(ctx);
   const char *api_name = "glGetActiveSubroutineName";
   struct gl_shader_program *shProg;
   GLenum resource_type;
            if (!_mesa_validate_shader_target(ctx, shadertype)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s", api_name);
               shProg = _mesa_lookup_shader_program_err(ctx, program, api_name);
   if (!shProg)
            stage = _mesa_shader_enum_to_shader_stage(shadertype);
   if (!shProg->_LinkedShaders[stage]) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s", api_name);
      }
   resource_type = _mesa_shader_stage_to_subroutine(stage);
   _mesa_get_program_resource_name(shProg, resource_type,
            }
      GLvoid GLAPIENTRY
   _mesa_UniformSubroutinesuiv(GLenum shadertype, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   const char *api_name = "glUniformSubroutinesuiv";
   gl_shader_stage stage;
            if (!_mesa_validate_shader_target(ctx, shadertype)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s", api_name);
               stage = _mesa_shader_enum_to_shader_stage(shadertype);
   struct gl_program *p = ctx->_Shader->CurrentProgram[stage];
   if (!p) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s", api_name);
               if (count != p->sh.NumSubroutineUniformRemapTable) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s", api_name);
               i = 0;
   bool flushed = false;
   do {
      struct gl_uniform_storage *uni = p->sh.SubroutineUniformRemapTable[i];
   if (uni == NULL) {
      i++;
               if (!flushed) {
      _mesa_flush_vertices_for_uniforms(ctx, uni);
               int uni_count = uni->array_elements ? uni->array_elements : 1;
            for (j = i; j < i + uni_count; j++) {
      struct gl_subroutine_function *subfn = NULL;
   if (indices[j] > p->sh.MaxSubroutineFunctionIndex) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s", api_name);
               for (f = 0; f < p->sh.NumSubroutineFunctions; f++) {
      if (p->sh.SubroutineFunctions[f].index == indices[j])
               if (!subfn) {
                  for (k = 0; k < subfn->num_compat_types; k++) {
      if (subfn->types[k] == uni->type)
      }
   if (k == subfn->num_compat_types) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s", api_name);
                  }
         }
         GLvoid GLAPIENTRY
   _mesa_GetUniformSubroutineuiv(GLenum shadertype, GLint location,
         {
      GET_CURRENT_CONTEXT(ctx);
   const char *api_name = "glGetUniformSubroutineuiv";
            if (!_mesa_validate_shader_target(ctx, shadertype)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s", api_name);
               stage = _mesa_shader_enum_to_shader_stage(shadertype);
   struct gl_program *p = ctx->_Shader->CurrentProgram[stage];
   if (!p) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s", api_name);
               if (location >= p->sh.NumSubroutineUniformRemapTable) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s", api_name);
                  }
         GLvoid GLAPIENTRY
   _mesa_GetProgramStageiv(GLuint program, GLenum shadertype,
         {
      GET_CURRENT_CONTEXT(ctx);
   const char *api_name = "glGetProgramStageiv";
   struct gl_shader_program *shProg;
   struct gl_linked_shader *sh;
            if (!_mesa_validate_shader_target(ctx, shadertype)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s", api_name);
               shProg = _mesa_lookup_shader_program_err(ctx, program, api_name);
   if (!shProg)
            stage = _mesa_shader_enum_to_shader_stage(shadertype);
            /* ARB_shader_subroutine doesn't ask the program to be linked, or list any
   * INVALID_OPERATION in the case of not be linked.
   *
   * And for some pnames, like GL_ACTIVE_SUBROUTINE_UNIFORMS, you can ask the
   * same info using other specs (ARB_program_interface_query), without the
   * need of the program to be linked, being the value for that case 0.
   *
   * But at the same time, some other methods require the program to be
   * linked for pname related to locations, so it would be inconsistent to
   * not do the same here. So we are:
   *   * Return GL_INVALID_OPERATION if not linked only for locations.
   *   * Setting a default value of 0, to be returned if not linked.
   */
   if (!sh) {
      values[0] = 0;
   if (pname == GL_ACTIVE_SUBROUTINE_UNIFORM_LOCATIONS) {
         }
               struct gl_program *p = sh->Program;
   switch (pname) {
   case GL_ACTIVE_SUBROUTINES:
      values[0] = p->sh.NumSubroutineFunctions;
      case GL_ACTIVE_SUBROUTINE_UNIFORM_LOCATIONS:
      values[0] = p->sh.NumSubroutineUniformRemapTable;
      case GL_ACTIVE_SUBROUTINE_UNIFORMS:
      values[0] = p->sh.NumSubroutineUniforms;
      case GL_ACTIVE_SUBROUTINE_MAX_LENGTH:
   {
      unsigned i;
   GLint max_len = 0;
   GLenum resource_type;
            resource_type = _mesa_shader_stage_to_subroutine(stage);
   for (i = 0; i < p->sh.NumSubroutineFunctions; i++) {
      res = _mesa_program_resource_find_index(shProg, resource_type, i);
   if (res) {
      const GLint len = _mesa_program_resource_name_length(res) + 1;
   if (len > max_len)
         }
   values[0] = max_len;
      }
   case GL_ACTIVE_SUBROUTINE_UNIFORM_MAX_LENGTH:
   {
      unsigned i;
   GLint max_len = 0;
   GLenum resource_type;
            resource_type = _mesa_shader_stage_to_subroutine_uniform(stage);
   for (i = 0; i < p->sh.NumSubroutineUniformRemapTable; i++) {
      res = _mesa_program_resource_find_index(shProg, resource_type, i);
   if (res) {
                     if (len > max_len)
         }
   values[0] = max_len;
      }
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "%s", api_name);
   values[0] = -1;
         }
      /* This is simple list entry that will be used to hold a list of string
   * tokens of a parsed shader include path.
   */
   struct sh_incl_path_entry
   {
                  };
      /* Nodes of the shader include tree */
   struct sh_incl_path_ht_entry
   {
      struct hash_table *path;
      };
      struct shader_includes {
      /* Array to hold include paths given to glCompileShaderIncludeARB() */
   struct sh_incl_path_entry **include_paths;
   size_t num_include_paths;
            /* Root hash table holding the shader include tree */
      };
      void
   _mesa_init_shader_includes(struct gl_shared_state *shared)
   {
      shared->ShaderIncludes = calloc(1, sizeof(struct shader_includes));
   shared->ShaderIncludes->shader_include_tree =
      _mesa_hash_table_create(NULL, _mesa_hash_string,
   }
      size_t
   _mesa_get_shader_include_cursor(struct gl_shared_state *shared)
   {
         }
      void
   _mesa_set_shader_include_cursor(struct gl_shared_state *shared, size_t cursor)
   {
         }
      static void
   destroy_shader_include(struct hash_entry *entry)
   {
      struct sh_incl_path_ht_entry *sh_incl_ht_entry =
            _mesa_hash_table_destroy(sh_incl_ht_entry->path, destroy_shader_include);
   free(sh_incl_ht_entry->shader_source);
   free(sh_incl_ht_entry);
      }
      void
   _mesa_destroy_shader_includes(struct gl_shared_state *shared)
   {
      _mesa_hash_table_destroy(shared->ShaderIncludes->shader_include_tree,
            }
      static bool
   valid_path_format(const char *str, bool relative_path)
   {
               if (!str[i] || (!relative_path && str[i] != '/'))
                     while (str[i]) {
      const char c = str[i++];
   if (('A' <= c && c <= 'Z') ||
      ('a' <= c && c <= 'z') ||
               if (c == '/') {
                                 if (strchr("^. _+*%[](){}|&~=!:;,?-", c) == NULL)
   }
      if (str[i - 1] == '/')
            return true;
   }
         static bool
   validate_and_tokenise_sh_incl(struct gl_context *ctx,
                     {
               if (!valid_path_format(full_path, relative_path)) {
      if (error_check) {
      _mesa_error(ctx, GL_INVALID_VALUE,
      }
               char *save_ptr = NULL;
            *path_list = rzalloc(mem_ctx, struct sh_incl_path_entry);
   struct sh_incl_path_entry * list = *path_list;
            while (path_str != NULL) {
      if (strlen(path_str) == 0) {
      if (error_check) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                           if (strcmp(path_str, ".") == 0) {
         } else if (strcmp(path_str, "..") == 0) {
         } else {
                     path->path = ralloc_strdup(mem_ctx, path_str);
                              }
      static struct sh_incl_path_ht_entry *
   lookup_shader_include(struct gl_context *ctx, char *path,
         {
      void *mem_ctx = ralloc_context(NULL);
            if (!validate_and_tokenise_sh_incl(ctx, mem_ctx, &path_list, path,
            ralloc_free(mem_ctx);
               struct sh_incl_path_ht_entry *sh_incl_ht_entry = NULL;
   struct hash_table *path_ht =
            size_t count = ctx->Shared->ShaderIncludes->num_include_paths;
            size_t i = ctx->Shared->ShaderIncludes->relative_path_cursor;
            do {
               next_relative_path:
            {
      struct sh_incl_path_entry *rel_path_list =
         LIST_FOR_EACH_ENTRY(entry, &rel_path_list->list, list) {
                     if (!ht_entry) {
      /* Reset search path and skip to the next include path */
   path_ht = ctx->Shared->ShaderIncludes->shader_include_tree;
   sh_incl_ht_entry = NULL;
                              }
   i++;
   if (i < count)
         else
      } else {
                                          LIST_FOR_EACH_ENTRY(entry, &path_list->list, list) {
                     if (!ht_entry) {
      /* Reset search path and skip to the next include path */
   path_ht = ctx->Shared->ShaderIncludes->shader_include_tree;
   sh_incl_ht_entry = NULL;
   if (use_cursor) {
                        }
   i++;
                  sh_incl_ht_entry =
                           if (i < count &&
                  /* If we get here then we have found a matching path or exahusted our
   * relative search paths.
   */
   ctx->Shared->ShaderIncludes->relative_path_cursor = i;
                           }
      const char *
   _mesa_lookup_shader_include(struct gl_context *ctx, char *path,
         {
      struct sh_incl_path_ht_entry *shader_include =
               }
      static char *
   copy_string(struct gl_context *ctx, const char *str, int str_len,
         {
      if (!str) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(NULL string)", caller);
               char *cp;
   if (str_len == -1)
         else {
      cp = calloc(sizeof(char), str_len + 1);
                  }
      GLvoid GLAPIENTRY
   _mesa_NamedStringARB(GLenum type, GLint namelen, const GLchar *name,
         {
      GET_CURRENT_CONTEXT(ctx);
            if (type != GL_SHADER_INCLUDE_ARB) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(invalid type)", caller);
               char *name_cp = copy_string(ctx, name, namelen, caller);
   char *string_cp = copy_string(ctx, string, stringlen, caller);
   if (!name_cp || !string_cp) {
      free(string_cp);
   free(name_cp);
               void *mem_ctx = ralloc_context(NULL);
            if (!validate_and_tokenise_sh_incl(ctx, mem_ctx, &path_list, name_cp,
            free(string_cp);
   free(name_cp);
   ralloc_free(mem_ctx);
                        struct hash_table *path_ht =
            struct sh_incl_path_entry *entry;
   LIST_FOR_EACH_ENTRY(entry, &path_list->list, list) {
      struct hash_entry *ht_entry =
            struct sh_incl_path_ht_entry *sh_incl_ht_entry;
   if (!ht_entry) {
      sh_incl_ht_entry = calloc(1, sizeof(struct sh_incl_path_ht_entry));
   sh_incl_ht_entry->path =
      _mesa_hash_table_create(NULL, _mesa_hash_string,
      _mesa_hash_table_insert(path_ht, strdup(entry->path),
      } else {
                           if (list_last_entry(&path_list->list, struct sh_incl_path_entry, list) == entry) {
      free(sh_incl_ht_entry->shader_source);
                           free(name_cp);
      }
      GLvoid GLAPIENTRY
   _mesa_DeleteNamedStringARB(GLint namelen, const GLchar *name)
   {
      GET_CURRENT_CONTEXT(ctx);
            char *name_cp = copy_string(ctx, name, namelen, caller);
   if (!name_cp)
            struct sh_incl_path_ht_entry *shader_include =
            if (!shader_include) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
         free(name_cp);
                        free(shader_include->shader_source);
                        }
      GLvoid GLAPIENTRY
   _mesa_CompileShaderIncludeARB(GLuint shader, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
            if (count > 0 && path == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(count > 0 && path == NULL)",
                                       ctx->Shared->ShaderIncludes->include_paths =
            for (size_t i = 0; i < count; i++) {
      char *path_cp = copy_string(ctx, path[i], length ? length[i] : -1,
         if (!path_cp) {
                           if (!validate_and_tokenise_sh_incl(ctx, mem_ctx, &path_list, path_cp,
            free(path_cp);
                                    /* We must set this *after* all calls to validate_and_tokenise_sh_incl()
   * are done as we use this to decide if we need to check the start of the
   * path for a '/'
   */
            struct gl_shader *sh = _mesa_lookup_shader(ctx, shader);
   if (!sh) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(shader)", caller);
                     exit:
      ctx->Shared->ShaderIncludes->num_include_paths = 0;
   ctx->Shared->ShaderIncludes->relative_path_cursor = 0;
                        }
      GLboolean GLAPIENTRY
   _mesa_IsNamedStringARB(GLint namelen, const GLchar *name)
   {
               if (!name)
                     const char *source = _mesa_lookup_shader_include(ctx, name_cp, false);
            if (!source)
               }
      GLvoid GLAPIENTRY
   _mesa_GetNamedStringARB(GLint namelen, const GLchar *name, GLsizei bufSize,
         {
      GET_CURRENT_CONTEXT(ctx);
            char *name_cp = copy_string(ctx, name, namelen, caller);
   if (!name_cp)
            const char *source = _mesa_lookup_shader_include(ctx, name_cp, true);
   if (!source) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
         free(name_cp);
               size_t size = MIN2(strlen(source), bufSize - 1);
   memcpy(string, source, size);
                        }
      GLvoid GLAPIENTRY
   _mesa_GetNamedStringivARB(GLint namelen, const GLchar *name,
         {
      GET_CURRENT_CONTEXT(ctx);
            char *name_cp = copy_string(ctx, name, namelen, caller);
   if (!name_cp)
            const char *source = _mesa_lookup_shader_include(ctx, name_cp, true);
   if (!source) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
         free(name_cp);
               switch (pname) {
   case GL_NAMED_STRING_LENGTH_ARB:
      *params = strlen(source) + 1;
      case GL_NAMED_STRING_TYPE_ARB:
      *params = GL_SHADER_INCLUDE_ARB;
      default:
      _mesa_error(ctx, GL_INVALID_ENUM, "%s(pname)", caller);
                  }
      static int
   find_compat_subroutine(struct gl_program *p, const struct glsl_type *type)
   {
               for (i = 0; i < p->sh.NumSubroutineFunctions; i++) {
      struct gl_subroutine_function *fn = &p->sh.SubroutineFunctions[i];
   for (j = 0; j < fn->num_compat_types; j++) {
      if (fn->types[j] == type)
         }
      }
      static void
   _mesa_shader_write_subroutine_index(struct gl_context *ctx,
         {
               if (p->sh.NumSubroutineUniformRemapTable == 0)
            i = 0;
   do {
      struct gl_uniform_storage *uni = p->sh.SubroutineUniformRemapTable[i];
   int uni_count;
            if (!uni) {
      i++;
               uni_count = uni->array_elements ? uni->array_elements : 1;
   for (j = 0; j < uni_count; j++) {
      val = ctx->SubroutineIndex[p->info.stage].IndexPtr[i + j];
               _mesa_propagate_uniforms_to_driver_storage(uni, 0, uni_count);
         }
      void
   _mesa_shader_write_subroutine_indices(struct gl_context *ctx,
         {
      if (ctx->_Shader->CurrentProgram[stage])
      _mesa_shader_write_subroutine_index(ctx,
   }
      void
   _mesa_program_init_subroutine_defaults(struct gl_context *ctx,
         {
               struct gl_subroutine_index_binding *binding = &ctx->SubroutineIndex[p->info.stage];
   if (binding->NumIndex != p->sh.NumSubroutineUniformRemapTable) {
      binding->IndexPtr = realloc(binding->IndexPtr,
                     for (int i = 0; i < p->sh.NumSubroutineUniformRemapTable; i++) {
               if (!uni)
                  }
