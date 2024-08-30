   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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
   * \file arbprogram.c
   * ARB_vertex/fragment_program state management functions.
   * \author Brian Paul
   */
         #include "util/glheader.h"
   #include "main/context.h"
   #include "main/draw_validate.h"
   #include "main/hash.h"
      #include "main/macros.h"
   #include "main/mtypes.h"
   #include "main/shaderapi.h"
   #include "main/state.h"
   #include "program/arbprogparse.h"
   #include "program/program.h"
   #include "program/prog_print.h"
   #include "api_exec_decl.h"
      #include "state_tracker/st_program.h"
      static void
   flush_vertices_for_program_constants(struct gl_context *ctx, GLenum target)
   {
               if (target == GL_FRAGMENT_PROGRAM_ARB) {
      new_driver_state =
      } else {
      new_driver_state =
               FLUSH_VERTICES(ctx, new_driver_state ? 0 : _NEW_PROGRAM_CONSTANTS, 0);
      }
      static struct gl_program*
   lookup_or_create_program(GLuint id, GLenum target, const char* caller)
   {
      GET_CURRENT_CONTEXT(ctx);
            if (id == 0) {
      /* Bind a default program */
   if (target == GL_VERTEX_PROGRAM_ARB)
         else
      }
   else {
      /* Bind a user program */
   newProg = _mesa_lookup_program(ctx, id);
   if (!newProg || newProg == &_mesa_DummyProgram) {
      bool isGenName = newProg != NULL;
   /* allocate a new program now */
   newProg = ctx->Driver.NewProgram(ctx, _mesa_program_enum_to_shader_stage(target),
         if (!newProg) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "%s", caller);
      }
      }
   else if (newProg->Target != target) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
               }
      }
      /**
   * Bind a program (make it current)
   * \note Called from the GL API dispatcher by both glBindProgramNV
   * and glBindProgramARB.
   */
   void GLAPIENTRY
   _mesa_BindProgramARB(GLenum target, GLuint id)
   {
      struct gl_program *curProg, *newProg;
            /* Error-check target and get curProg */
   if (target == GL_VERTEX_PROGRAM_ARB && ctx->Extensions.ARB_vertex_program) {
         }
   else if (target == GL_FRAGMENT_PROGRAM_ARB
               }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBindProgramARB(target)");
               /*
   * Get pointer to new program to bind.
   * NOTE: binding to a non-existant program is not an error.
   * That's supposed to be caught in glBegin.
   */
   newProg = lookup_or_create_program(id, target, "glBindProgram");
   if (!newProg)
                     if (curProg->Id == id) {
      /* binding same program - no change */
               /* signal new program (and its new constants) */
   FLUSH_VERTICES(ctx, _NEW_PROGRAM, 0);
            /* bind newProg */
   if (target == GL_VERTEX_PROGRAM_ARB) {
         }
   else if (target == GL_FRAGMENT_PROGRAM_ARB) {
                  _mesa_update_vertex_processing_mode(ctx);
            /* Never null pointers */
   assert(ctx->VertexProgram.Current);
      }
         /**
   * Delete a list of programs.
   * \note Not compiled into display lists.
   * \note Called by both glDeleteProgramsNV and glDeleteProgramsARB.
   */
   void GLAPIENTRY
   _mesa_DeleteProgramsARB(GLsizei n, const GLuint *ids)
   {
      GLint i;
                     if (n < 0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glDeleteProgramsNV" );
               for (i = 0; i < n; i++) {
      if (ids[i] != 0) {
      struct gl_program *prog = _mesa_lookup_program(ctx, ids[i]);
   if (prog == &_mesa_DummyProgram) {
         }
   else if (prog) {
      /* Unbind program if necessary */
   switch (prog->Target) {
   case GL_VERTEX_PROGRAM_ARB:
      if (ctx->VertexProgram.Current &&
      ctx->VertexProgram.Current->Id == ids[i]) {
   /* unbind this currently bound program */
      }
      case GL_FRAGMENT_PROGRAM_ARB:
      if (ctx->FragmentProgram.Current &&
      ctx->FragmentProgram.Current->Id == ids[i]) {
   /* unbind this currently bound program */
      }
      default:
      _mesa_problem(ctx, "bad target in glDeleteProgramsNV");
      }
   /* The ID is immediately available for re-use now */
   _mesa_HashRemove(ctx->Shared->Programs, ids[i]);
               }
         /**
   * Generate a list of new program identifiers.
   * \note Not compiled into display lists.
   * \note Called by both glGenProgramsNV and glGenProgramsARB.
   */
   void GLAPIENTRY
   _mesa_GenProgramsARB(GLsizei n, GLuint *ids)
   {
      GLuint i;
            if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGenPrograms");
               if (!ids)
                              /* Insert pointer to dummy program as placeholder */
   for (i = 0; i < (GLuint) n; i++) {
      _mesa_HashInsertLocked(ctx->Shared->Programs, ids[i],
                  }
         /**
   * Determine if id names a vertex or fragment program.
   * \note Not compiled into display lists.
   * \note Called from both glIsProgramNV and glIsProgramARB.
   * \param id is the program identifier
   * \return GL_TRUE if id is a program, else GL_FALSE.
   */
   GLboolean GLAPIENTRY
   _mesa_IsProgramARB(GLuint id)
   {
      struct gl_program *prog = NULL;
   GET_CURRENT_CONTEXT(ctx);
            if (id == 0)
            prog = _mesa_lookup_program(ctx, id);
   if (prog && (prog != &_mesa_DummyProgram))
         else
      }
      static struct gl_program*
   get_current_program(struct gl_context* ctx, GLenum target, const char* caller)
   {
      if (target == GL_VERTEX_PROGRAM_ARB
      && ctx->Extensions.ARB_vertex_program) {
      }
   else if (target == GL_FRAGMENT_PROGRAM_ARB
               }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM,
               }
      static GLboolean
   get_local_param_pointer(struct gl_context *ctx, const char *func,
               {
      if (unlikely(index + count > prog->arb.MaxLocalParams)) {
      /* If arb.MaxLocalParams == 0, we need to do initialization. */
   if (!prog->arb.MaxLocalParams) {
               if (target == GL_VERTEX_PROGRAM_ARB)
                        /* Allocate LocalParams. */
   if (!prog->arb.LocalParams) {
      prog->arb.LocalParams = rzalloc_array_size(prog, sizeof(float[4]),
         if (!prog->arb.LocalParams) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "%s", func);
                  /* Initialize MaxLocalParams. */
               /* Check again after initializing MaxLocalParams. */
   if (index + count > prog->arb.MaxLocalParams) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(index)", func);
                  *param = prog->arb.LocalParams[index];
      }
         static GLboolean
   get_env_param_pointer(struct gl_context *ctx, const char *func,
         {
      if (target == GL_FRAGMENT_PROGRAM_ARB
      && ctx->Extensions.ARB_fragment_program) {
   if (index >= ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxEnvParams) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(index)", func);
      }
   *param = ctx->FragmentProgram.Parameters[index];
      }
   else if (target == GL_VERTEX_PROGRAM_ARB &&
            if (index >= ctx->Const.Program[MESA_SHADER_VERTEX].MaxEnvParams) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(index)", func);
      }
   *param = ctx->VertexProgram.Parameters[index];
      } else {
      _mesa_error(ctx, GL_INVALID_ENUM, "%s(target)", func);
         }
      static void
   set_program_string(struct gl_program *prog, GLenum target, GLenum format, GLsizei len,
         {
      bool failed;
                     if (!ctx->Extensions.ARB_vertex_program
      && !ctx->Extensions.ARB_fragment_program) {
   _mesa_error(ctx, GL_INVALID_OPERATION, "glProgramStringARB()");
               if (format != GL_PROGRAM_FORMAT_ASCII_ARB) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glProgramStringARB(format)");
            #ifdef ENABLE_SHADER_CACHE
                        uint8_t sha1[SHA1_DIGEST_LENGTH];
            /* Dump original shader source to MESA_SHADER_DUMP_PATH and replace
   * if corresponding entry found from MESA_SHADER_READ_PATH.
   */
            replacement = _mesa_read_shader_source(stage, string, sha1);
   if (replacement)
      #endif /* ENABLE_SHADER_CACHE */
         if (target == GL_VERTEX_PROGRAM_ARB && ctx->Extensions.ARB_vertex_program) {
         }
   else if (target == GL_FRAGMENT_PROGRAM_ARB
               }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glProgramStringARB(target)");
                        if (!failed) {
      /* finally, give the program to the driver for translation/checking */
   if (!st_program_string_notify(ctx, target, prog)) {
      failed = true;
   _mesa_error(ctx, GL_INVALID_OPERATION,
                  _mesa_update_vertex_processing_mode(ctx);
            if (ctx->_Shader->Flags & GLSL_DUMP) {
      const char *shader_type =
            fprintf(stderr, "ARB_%s_program source for program %d:\n",
                  if (failed) {
      fprintf(stderr, "ARB_%s_program %d failed to compile.\n",
      } else {
      fprintf(stderr, "Mesa IR for ARB_%s_program %d:\n",
         _mesa_print_program(prog);
      }
               /* Capture vp-*.shader_test/fp-*.shader_test files. */
   const char *capture_path = _mesa_get_shader_capture_path();
   if (capture_path != NULL) {
      FILE *file;
   const char *shader_type =
         char *filename =
                  file = fopen(filename, "w");
   if (file) {
      fprintf(file,
         "[require]\nGL_ARB_%s_program\n\n[%s program]\n%s\n",
      } else {
         }
         }
      void GLAPIENTRY
   _mesa_ProgramStringARB(GLenum target, GLenum format, GLsizei len,
         {
      GET_CURRENT_CONTEXT(ctx);
   if (target == GL_VERTEX_PROGRAM_ARB && ctx->Extensions.ARB_vertex_program) {
         }
   else if (target == GL_FRAGMENT_PROGRAM_ARB
               }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glProgramStringARB(target)");
         }
      void GLAPIENTRY
   _mesa_NamedProgramStringEXT(GLuint program, GLenum target, GLenum format, GLsizei len,
         {
               if (!prog) {
         }
      }
         /**
   * Set a program env parameter register.
   * \note Called from the GL API dispatcher.
   */
   void GLAPIENTRY
   _mesa_ProgramEnvParameter4dARB(GLenum target, GLuint index,
         {
      _mesa_ProgramEnvParameter4fARB(target, index, (GLfloat) x, (GLfloat) y,
      }
         /**
   * Set a program env parameter register.
   * \note Called from the GL API dispatcher.
   */
   void GLAPIENTRY
   _mesa_ProgramEnvParameter4dvARB(GLenum target, GLuint index,
         {
      _mesa_ProgramEnvParameter4fARB(target, index, (GLfloat) params[0],
            }
         /**
   * Set a program env parameter register.
   * \note Called from the GL API dispatcher.
   */
   void GLAPIENTRY
   _mesa_ProgramEnvParameter4fARB(GLenum target, GLuint index,
         {
                                 if (get_env_param_pointer(ctx, "glProgramEnvParameter",
      target, index, &param)) {
         }
            /**
   * Set a program env parameter register.
   * \note Called from the GL API dispatcher.
   */
   void GLAPIENTRY
   _mesa_ProgramEnvParameter4fvARB(GLenum target, GLuint index,
         {
                                 if (get_env_param_pointer(ctx, "glProgramEnvParameter4fv",
                  }
         void GLAPIENTRY
   _mesa_ProgramEnvParameters4fvEXT(GLenum target, GLuint index, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
                     if (count <= 0) {
                  if (target == GL_FRAGMENT_PROGRAM_ARB
      && ctx->Extensions.ARB_fragment_program) {
   if ((index + count) > ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxEnvParams) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glProgramEnvParameters4fv(index + count)");
      }
      }
   else if (target == GL_VERTEX_PROGRAM_ARB
      && ctx->Extensions.ARB_vertex_program) {
   if ((index + count) > ctx->Const.Program[MESA_SHADER_VERTEX].MaxEnvParams) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glProgramEnvParameters4fv(index + count)");
      }
      }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glProgramEnvParameters4fv(target)");
                  }
         void GLAPIENTRY
   _mesa_GetProgramEnvParameterdvARB(GLenum target, GLuint index,
         {
      GET_CURRENT_CONTEXT(ctx);
            if (get_env_param_pointer(ctx, "glGetProgramEnvParameterdv",
      target, index, &fparam)) {
         }
         void GLAPIENTRY
   _mesa_GetProgramEnvParameterfvARB(GLenum target, GLuint index,
         {
                        if (get_env_param_pointer(ctx, "glGetProgramEnvParameterfv",
                  }
         void GLAPIENTRY
   _mesa_ProgramLocalParameter4fARB(GLenum target, GLuint index,
         {
      GET_CURRENT_CONTEXT(ctx);
   GLfloat *param;
   struct gl_program* prog = get_current_program(ctx, target, "glProgramLocalParameterARB");
   if (!prog) {
                           if (get_local_param_pointer(ctx, "glProgramLocalParameterARB",
            assert(index < MAX_PROGRAM_LOCAL_PARAMS);
         }
      void GLAPIENTRY
   _mesa_NamedProgramLocalParameter4fEXT(GLuint program, GLenum target, GLuint index,
         {
      GET_CURRENT_CONTEXT(ctx);
   GLfloat *param;
   struct gl_program* prog = lookup_or_create_program(program, target,
            if (!prog) {
                  if ((target == GL_VERTEX_PROGRAM_ARB && prog == ctx->VertexProgram.Current) ||
      (target == GL_FRAGMENT_PROGRAM_ARB && prog == ctx->FragmentProgram.Current)) {
               if (get_local_param_pointer(ctx, "glNamedProgramLocalParameter4fEXT",
            assert(index < MAX_PROGRAM_LOCAL_PARAMS);
         }
         void GLAPIENTRY
   _mesa_ProgramLocalParameter4fvARB(GLenum target, GLuint index,
         {
      _mesa_ProgramLocalParameter4fARB(target, index, params[0], params[1],
      }
         void GLAPIENTRY
   _mesa_NamedProgramLocalParameter4fvEXT(GLuint program, GLenum target, GLuint index,
         {
      _mesa_NamedProgramLocalParameter4fEXT(program, target, index, params[0],
      }
         static void
   program_local_parameters4fv(struct gl_program* prog, GLuint index, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   GLfloat *dest;
            if (count <= 0) {
                  if (get_local_param_pointer(ctx, caller,
            }
         void GLAPIENTRY
   _mesa_ProgramLocalParameters4fvEXT(GLenum target, GLuint index, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
   struct gl_program* prog = get_current_program(ctx, target,
         if (!prog) {
                  program_local_parameters4fv(prog, index, count, params,
      }
      void GLAPIENTRY
   _mesa_NamedProgramLocalParameters4fvEXT(GLuint program, GLenum target, GLuint index,
         {
      struct gl_program* prog =
      lookup_or_create_program(program, target,
      if (!prog) {
                  program_local_parameters4fv(prog, index, count, params,
      }
         void GLAPIENTRY
   _mesa_ProgramLocalParameter4dARB(GLenum target, GLuint index,
               {
      _mesa_ProgramLocalParameter4fARB(target, index, (GLfloat) x, (GLfloat) y,
      }
         void GLAPIENTRY
   _mesa_NamedProgramLocalParameter4dEXT(GLuint program, GLenum target, GLuint index,
               {
      _mesa_NamedProgramLocalParameter4fEXT(program, target, index, (GLfloat) x, (GLfloat) y,
      }
         void GLAPIENTRY
   _mesa_ProgramLocalParameter4dvARB(GLenum target, GLuint index,
         {
      _mesa_ProgramLocalParameter4fARB(target, index,
            }
         void GLAPIENTRY
   _mesa_NamedProgramLocalParameter4dvEXT(GLuint program, GLenum target, GLuint index,
         {
      _mesa_NamedProgramLocalParameter4fEXT(program, target, index,
            }
         void GLAPIENTRY
   _mesa_GetProgramLocalParameterfvARB(GLenum target, GLuint index,
         {
      GLfloat *param;
   GET_CURRENT_CONTEXT(ctx);
   struct gl_program* prog = get_current_program(ctx, target, "glGetProgramLocalParameterfvARB");
   if (!prog) {
                  if (get_local_param_pointer(ctx, "glProgramLocalParameters4fvEXT",
   prog, target, index, 1, &param)) {
            }
         void GLAPIENTRY
   _mesa_GetNamedProgramLocalParameterfvEXT(GLuint program, GLenum target, GLuint index,
         {
      GLfloat *param;
   GET_CURRENT_CONTEXT(ctx);
   struct gl_program* prog = lookup_or_create_program(program, target,
         if (!prog) {
                  if (get_local_param_pointer(ctx, "glGetNamedProgramLocalParameterfvEXT",
                  }
         void GLAPIENTRY
   _mesa_GetProgramLocalParameterdvARB(GLenum target, GLuint index,
         {
      GLfloat *param;
   GET_CURRENT_CONTEXT(ctx);
   struct gl_program* prog = get_current_program(ctx, target, "glGetProgramLocalParameterdvARB");
   if (!prog) {
                  if (get_local_param_pointer(ctx, "glProgramLocalParameters4fvEXT",
   prog, target, index, 1, &param)) {
            }
         void GLAPIENTRY
   _mesa_GetNamedProgramLocalParameterdvEXT(GLuint program, GLenum target, GLuint index,
         {
      GLfloat *param;
   GET_CURRENT_CONTEXT(ctx);
   struct gl_program* prog = lookup_or_create_program(program, target,
         if (!prog) {
                  if (get_local_param_pointer(ctx, "glGetNamedProgramLocalParameterdvEXT",
                  }
         static void
   get_program_iv(struct gl_program *prog, GLenum target, GLenum pname,
         {
                        if (target == GL_VERTEX_PROGRAM_ARB) {
         }
   else {
                  assert(prog);
            /* Queries supported for both vertex and fragment programs */
   switch (pname) {
      case GL_PROGRAM_LENGTH_ARB:
      *params
            case GL_PROGRAM_FORMAT_ARB:
      *params = prog->Format;
      case GL_PROGRAM_BINDING_ARB:
      *params = prog->Id;
      case GL_PROGRAM_INSTRUCTIONS_ARB:
      *params = prog->arb.NumInstructions;
      case GL_MAX_PROGRAM_INSTRUCTIONS_ARB:
      *params = limits->MaxInstructions;
      case GL_PROGRAM_NATIVE_INSTRUCTIONS_ARB:
      *params = prog->arb.NumNativeInstructions;
      case GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB:
      *params = limits->MaxNativeInstructions;
      case GL_PROGRAM_TEMPORARIES_ARB:
      *params = prog->arb.NumTemporaries;
      case GL_MAX_PROGRAM_TEMPORARIES_ARB:
      *params = limits->MaxTemps;
      case GL_PROGRAM_NATIVE_TEMPORARIES_ARB:
      *params = prog->arb.NumNativeTemporaries;
      case GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB:
      *params = limits->MaxNativeTemps;
      case GL_PROGRAM_PARAMETERS_ARB:
      *params = prog->arb.NumParameters;
      case GL_MAX_PROGRAM_PARAMETERS_ARB:
      *params = limits->MaxParameters;
      case GL_PROGRAM_NATIVE_PARAMETERS_ARB:
      *params = prog->arb.NumNativeParameters;
      case GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB:
      *params = limits->MaxNativeParameters;
      case GL_PROGRAM_ATTRIBS_ARB:
      *params = prog->arb.NumAttributes;
      case GL_MAX_PROGRAM_ATTRIBS_ARB:
      *params = limits->MaxAttribs;
      case GL_PROGRAM_NATIVE_ATTRIBS_ARB:
      *params = prog->arb.NumNativeAttributes;
      case GL_MAX_PROGRAM_NATIVE_ATTRIBS_ARB:
      *params = limits->MaxNativeAttribs;
      case GL_PROGRAM_ADDRESS_REGISTERS_ARB:
      *params = prog->arb.NumAddressRegs;
      case GL_MAX_PROGRAM_ADDRESS_REGISTERS_ARB:
      *params = limits->MaxAddressRegs;
      case GL_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB:
      *params = prog->arb.NumNativeAddressRegs;
      case GL_MAX_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB:
      *params = limits->MaxNativeAddressRegs;
      case GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB:
      *params = limits->MaxLocalParams;
      case GL_MAX_PROGRAM_ENV_PARAMETERS_ARB:
      *params = limits->MaxEnvParams;
      case GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB:
      /*
   * XXX we may not really need a driver callback here.
   * If the number of native instructions, registers, etc. used
   * are all below the maximums, we could return true.
   * The spec says that even if this query returns true, there's
   * no guarantee that the program will run in hardware.
   */
   if (prog->Id == 0) {
      /* default/null program */
   else {
            *params = GL_TRUE;
         }
      default:
      /* continue with fragment-program only queries below */
            /*
   * The following apply to fragment programs only (at this time)
   */
   if (target == GL_FRAGMENT_PROGRAM_ARB) {
      const struct gl_program *fp = ctx->FragmentProgram.Current;
   switch (pname) {
      case GL_PROGRAM_ALU_INSTRUCTIONS_ARB:
      *params = fp->arb.NumNativeAluInstructions;
      case GL_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB:
      *params = fp->arb.NumAluInstructions;
      case GL_PROGRAM_TEX_INSTRUCTIONS_ARB:
      *params = fp->arb.NumTexInstructions;
      case GL_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB:
      *params = fp->arb.NumNativeTexInstructions;
      case GL_PROGRAM_TEX_INDIRECTIONS_ARB:
      *params = fp->arb.NumTexIndirections;
      case GL_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB:
      *params = fp->arb.NumNativeTexIndirections;
      case GL_MAX_PROGRAM_ALU_INSTRUCTIONS_ARB:
      *params = limits->MaxAluInstructions;
      case GL_MAX_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB:
      *params = limits->MaxNativeAluInstructions;
      case GL_MAX_PROGRAM_TEX_INSTRUCTIONS_ARB:
      *params = limits->MaxTexInstructions;
      case GL_MAX_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB:
      *params = limits->MaxNativeTexInstructions;
      case GL_MAX_PROGRAM_TEX_INDIRECTIONS_ARB:
      *params = limits->MaxTexIndirections;
      case GL_MAX_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB:
      *params = limits->MaxNativeTexIndirections;
      default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramivARB(pname)");
      } else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramivARB(pname)");
         }
         void GLAPIENTRY
   _mesa_GetProgramivARB(GLenum target, GLenum pname, GLint *params)
   {
      GET_CURRENT_CONTEXT(ctx);
   struct gl_program* prog = get_current_program(ctx, target,
         if (!prog) {
         }
      }
      void GLAPIENTRY
   _mesa_GetNamedProgramivEXT(GLuint program, GLenum target, GLenum pname,
         {
      struct gl_program* prog;
   if (pname == GL_PROGRAM_BINDING_ARB) {
      _mesa_GetProgramivARB(target, pname, params);
      }
   prog = lookup_or_create_program(program, target,
         if (!prog) {
         }
      }
         void GLAPIENTRY
   _mesa_GetProgramStringARB(GLenum target, GLenum pname, GLvoid *string)
   {
      const struct gl_program *prog;
   char *dst = (char *) string;
            if (target == GL_VERTEX_PROGRAM_ARB) {
         }
   else if (target == GL_FRAGMENT_PROGRAM_ARB) {
         }
   else {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramStringARB(target)");
                        if (pname != GL_PROGRAM_STRING_ARB) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramStringARB(pname)");
               if (prog->String)
         else
      }
         void GLAPIENTRY
   _mesa_GetNamedProgramStringEXT(GLuint program, GLenum target,
            char *dst = (char *) string;
   GET_CURRENT_CONTEXT(ctx);
   struct gl_program* prog = lookup_or_create_program(program, target,
         if (!prog)
            if (pname != GL_PROGRAM_STRING_ARB) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetNamedProgramStringEXT(pname)");
               if (prog->String)
         else
      }
