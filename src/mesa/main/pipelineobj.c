   /*
   * Mesa 3-D graphics library
   *
   * Copyright © 2013 Gregory Hainaut <gregory.hainaut@gmail.com>
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
   */
      /**
   * \file pipelineobj.c
   * \author Hainaut Gregory <gregory.hainaut@gmail.com>
   *
   * Implementation of pipeline object related API functions. Based on
   * GL_ARB_separate_shader_objects extension.
   */
      #include <stdbool.h>
   #include "util/glheader.h"
   #include "main/context.h"
   #include "main/draw_validate.h"
   #include "main/enums.h"
   #include "main/hash.h"
   #include "main/mtypes.h"
   #include "main/pipelineobj.h"
   #include "main/shaderapi.h"
   #include "main/shaderobj.h"
   #include "main/state.h"
   #include "main/transformfeedback.h"
   #include "main/uniforms.h"
   #include "compiler/glsl/glsl_parser_extras.h"
   #include "compiler/glsl/ir_uniform.h"
   #include "program/program.h"
   #include "program/prog_parameter.h"
   #include "util/ralloc.h"
   #include "util/bitscan.h"
   #include "api_exec_decl.h"
      /**
   * Delete a pipeline object.
   */
   void
   _mesa_delete_pipeline_object(struct gl_context *ctx,
         {
               for (i = 0; i < MESA_SHADER_STAGES; i++) {
      _mesa_reference_program(ctx, &obj->CurrentProgram[i], NULL);
               _mesa_reference_shader_program(ctx, &obj->ActiveProgram, NULL);
   free(obj->Label);
      }
      /**
   * Allocate and initialize a new pipeline object.
   */
   static struct gl_pipeline_object *
   _mesa_new_pipeline_object(struct gl_context *ctx, GLuint name)
   {
      struct gl_pipeline_object *obj = rzalloc(NULL, struct gl_pipeline_object);
   if (obj) {
      obj->Name = name;
   obj->RefCount = 1;
   obj->Flags = _mesa_get_shader_flags();
                  }
      /**
   * Initialize pipeline object state for given context.
   */
   void
   _mesa_init_pipeline(struct gl_context *ctx)
   {
                        /* Install a default Pipeline */
   ctx->Pipeline.Default = _mesa_new_pipeline_object(ctx, 0);
      }
         /**
   * Callback for deleting a pipeline object.  Called by _mesa_HashDeleteAll().
   */
   static void
   delete_pipelineobj_cb(void *data, void *userData)
   {
      struct gl_pipeline_object *obj = (struct gl_pipeline_object *) data;
   struct gl_context *ctx = (struct gl_context *) userData;
      }
         /**
   * Free pipeline state for given context.
   */
   void
   _mesa_free_pipeline_data(struct gl_context *ctx)
   {
               _mesa_HashDeleteAll(ctx->Pipeline.Objects, delete_pipelineobj_cb, ctx);
               }
      /**
   * Look up the pipeline object for the given ID.
   *
   * \returns
   * Either a pointer to the pipeline object with the specified ID or \c NULL for
   * a non-existent ID.  The spec defines ID 0 as being technically
   * non-existent.
   */
   struct gl_pipeline_object *
   _mesa_lookup_pipeline_object(struct gl_context *ctx, GLuint id)
   {
      if (id == 0)
         else
      return (struct gl_pipeline_object *)
   }
      /**
   * Add the given pipeline object to the pipeline object pool.
   */
   static void
   save_pipeline_object(struct gl_context *ctx, struct gl_pipeline_object *obj)
   {
      if (obj->Name > 0) {
            }
      /**
   * Remove the given pipeline object from the pipeline object pool.
   * Do not deallocate the pipeline object though.
   */
   static void
   remove_pipeline_object(struct gl_context *ctx, struct gl_pipeline_object *obj)
   {
      if (obj->Name > 0) {
            }
      /**
   * Set ptr to obj w/ reference counting.
   * Note: this should only be called from the _mesa_reference_pipeline_object()
   * inline function.
   */
   void
   _mesa_reference_pipeline_object_(struct gl_context *ctx,
               {
               if (*ptr) {
      /* Unreference the old pipeline object */
            assert(oldObj->RefCount > 0);
            if (oldObj->RefCount == 0) {
                     }
            if (obj) {
      /* reference new pipeline object */
            obj->RefCount++;
         }
      static void
   use_program_stage(struct gl_context *ctx, GLenum type,
                  gl_shader_stage stage = _mesa_shader_enum_to_shader_stage(type);
   struct gl_program *prog = NULL;
   if (shProg && shProg->_LinkedShaders[stage])
               }
      static void
   use_program_stages(struct gl_context *ctx, struct gl_shader_program *shProg,
               /* Enable individual stages from the program as requested by the
   * application.  If there is no shader for a requested stage in the
   * program, _mesa_use_shader_program will enable fixed-function processing
   * as dictated by the spec.
   *
   * Section 2.11.4 (Program Pipeline Objects) of the OpenGL 4.1 spec
   * says:
   *
   *     "If UseProgramStages is called with program set to zero or with a
   *     program object that contains no executable code for the given
   *     stages, it is as if the pipeline object has no programmable stage
   *     configured for the indicated shader stages."
   */
   if ((stages & GL_VERTEX_SHADER_BIT) != 0)
            if ((stages & GL_FRAGMENT_SHADER_BIT) != 0)
            if ((stages & GL_GEOMETRY_SHADER_BIT) != 0)
            if ((stages & GL_TESS_CONTROL_SHADER_BIT) != 0)
            if ((stages & GL_TESS_EVALUATION_SHADER_BIT) != 0)
            if ((stages & GL_COMPUTE_SHADER_BIT) != 0)
                     if (pipe == ctx->_Shader)
      }
      void GLAPIENTRY
   _mesa_UseProgramStages_no_error(GLuint pipeline, GLbitfield stages,
         {
               struct gl_pipeline_object *pipe =
                  if (prog)
            /* Object is created by any Pipeline call but glGenProgramPipelines,
   * glIsProgramPipeline and GetProgramPipelineInfoLog
   */
               }
      /**
   * Bound program to severals stages of the pipeline
   */
   void GLAPIENTRY
   _mesa_UseProgramStages(GLuint pipeline, GLbitfield stages, GLuint program)
   {
               struct gl_pipeline_object *pipe = _mesa_lookup_pipeline_object(ctx, pipeline);
   struct gl_shader_program *shProg = NULL;
            if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glUseProgramStages(%u, 0x%x, %u)\n",
         if (!pipe) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glUseProgramStages(pipeline)");
               /* Object is created by any Pipeline call but glGenProgramPipelines,
   * glIsProgramPipeline and GetProgramPipelineInfoLog
   */
            /* Section 2.11.4 (Program Pipeline Objects) of the OpenGL 4.1 spec says:
   *
   *     "If stages is not the special value ALL_SHADER_BITS, and has a bit
   *     set that is not recognized, the error INVALID_VALUE is generated."
   */
   any_valid_stages = GL_VERTEX_SHADER_BIT | GL_FRAGMENT_SHADER_BIT;
   if (_mesa_has_geometry_shaders(ctx))
         if (_mesa_has_tessellation(ctx))
      any_valid_stages |= GL_TESS_CONTROL_SHADER_BIT |
      if (_mesa_has_compute_shaders(ctx))
            if (stages != GL_ALL_SHADER_BITS && (stages & ~any_valid_stages) != 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glUseProgramStages(Stages)");
               /* Section 2.17.2 (Transform Feedback Primitive Capture) of the OpenGL 4.1
   * spec says:
   *
   *     "The error INVALID_OPERATION is generated:
   *
   *      ...
   *
   *         - by UseProgramStages if the program pipeline object it refers
   *           to is current and the current transform feedback object is
   *           active and not paused;
   */
   if (ctx->_Shader == pipe) {
      if (_mesa_is_xfb_active_and_unpaused(ctx)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                        if (program) {
      shProg = _mesa_lookup_shader_program_err(ctx, program,
         if (shProg == NULL)
            /* Section 2.11.4 (Program Pipeline Objects) of the OpenGL 4.1 spec
   * says:
   *
   *     "If the program object named by program was linked without the
   *     PROGRAM_SEPARABLE parameter set, or was not linked successfully,
   *     the error INVALID_OPERATION is generated and the corresponding
   *     shader stages in the pipeline program pipeline object are not
   *     modified."
   */
   if (!shProg->data->LinkStatus) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     if (!shProg->SeparateShader) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                                 }
      static ALWAYS_INLINE void
   active_shader_program(struct gl_context *ctx, GLuint pipeline, GLuint program,
         {
      struct gl_shader_program *shProg = NULL;
            if (program) {
      if (no_error) {
         } else {
      shProg = _mesa_lookup_shader_program_err(ctx, program,
         if (shProg == NULL)
                  if (!no_error && !pipe) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glActiveShaderProgram(pipeline)");
               /* Object is created by any Pipeline call but glGenProgramPipelines,
   * glIsProgramPipeline and GetProgramPipelineInfoLog
   */
            if (!no_error && shProg != NULL && !shProg->data->LinkStatus) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     _mesa_reference_shader_program(ctx, &pipe->ActiveProgram, shProg);
   if (pipe == ctx->_Shader)
      }
      void GLAPIENTRY
   _mesa_ActiveShaderProgram_no_error(GLuint pipeline, GLuint program)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
      /**
   * Use the named shader program for subsequent glUniform calls (if pipeline
   * bound)
   */
   void GLAPIENTRY
   _mesa_ActiveShaderProgram(GLuint pipeline, GLuint program)
   {
               if (MESA_VERBOSE & VERBOSE_API)
               }
      static ALWAYS_INLINE void
   bind_program_pipeline(struct gl_context *ctx, GLuint pipeline, bool no_error)
   {
               if (MESA_VERBOSE & VERBOSE_API)
            /* Rebinding the same pipeline object: no change.
   */
   if (ctx->_Shader->Name == pipeline)
            /* Section 2.17.2 (Transform Feedback Primitive Capture) of the OpenGL 4.1
   * spec says:
   *
   *     "The error INVALID_OPERATION is generated:
   *
   *      ...
   *
   *         - by BindProgramPipeline if the current transform feedback
   *           object is active and not paused;
   */
   if (!no_error && _mesa_is_xfb_active_and_unpaused(ctx)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     /* Get pointer to new pipeline object (newObj)
   */
   if (pipeline) {
      /* non-default pipeline object */
   newObj = _mesa_lookup_pipeline_object(ctx, pipeline);
   if (!no_error && !newObj) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     /* Object is created by any Pipeline call but glGenProgramPipelines,
   * glIsProgramPipeline and GetProgramPipelineInfoLog
   */
                  }
      void GLAPIENTRY
   _mesa_BindProgramPipeline_no_error(GLuint pipeline)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
      /**
   * Make program of the pipeline current
   */
   void GLAPIENTRY
   _mesa_BindProgramPipeline(GLuint pipeline)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
      void
   _mesa_bind_pipeline(struct gl_context *ctx,
         {
      int i;
   /* First bind the Pipeline to pipeline binding point */
            /* Section 2.11.3 (Program Objects) of the OpenGL 4.1 spec says:
   *
   *     "If there is a current program object established by UseProgram,
   *     that program is considered current for all stages. Otherwise, if
   *     there is a bound program pipeline object (see section 2.11.4), the
   *     program bound to the appropriate stage of the pipeline object is
   *     considered current."
   */
   if (&ctx->Shader != ctx->_Shader) {
               if (pipe != NULL) {
      /* Bound the pipeline to the current program and
   * restore the pipeline state
   */
      } else {
      /* Unbind the pipeline */
   _mesa_reference_pipeline_object(ctx, &ctx->_Shader,
               for (i = 0; i < MESA_SHADER_STAGES; i++) {
      struct gl_program *prog = ctx->_Shader->CurrentProgram[i];
   if (prog) {
                     _mesa_update_vertex_processing_mode(ctx);
   _mesa_update_allow_draw_out_of_order(ctx);
         }
      /**
   * Delete a set of pipeline objects.
   *
   * \param n      Number of pipeline objects to delete.
   * \param ids    pipeline of \c n pipeline object IDs.
   */
   void GLAPIENTRY
   _mesa_DeleteProgramPipelines(GLsizei n, const GLuint *pipelines)
   {
      GET_CURRENT_CONTEXT(ctx);
            if (MESA_VERBOSE & VERBOSE_API)
            if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glDeleteProgramPipelines(n<0)");
               for (i = 0; i < n; i++) {
      struct gl_pipeline_object *obj =
            if (obj) {
               /* If the pipeline object is currently bound, the spec says "If an
   * object that is currently bound is deleted, the binding for that
   * object reverts to zero and no program pipeline object becomes
   * current."
   */
   if (obj == ctx->Pipeline.Current) {
                                 /* Unreference the pipeline object.
   * If refcount hits zero, the object will be deleted.
   */
            }
      /**
   * Generate a set of unique pipeline object IDs and store them in \c pipelines.
   * \param n       Number of IDs to generate.
   * \param pipelines  pipeline of \c n locations to store the IDs.
   */
   static void
   create_program_pipelines(struct gl_context *ctx, GLsizei n, GLuint *pipelines,
         {
      const char *func = dsa ? "glCreateProgramPipelines" : "glGenProgramPipelines";
            if (!pipelines)
                     for (i = 0; i < n; i++) {
               obj = _mesa_new_pipeline_object(ctx, pipelines[i]);
   if (!obj) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "%s", func);
               if (dsa) {
      /* make dsa-allocated objects behave like program objects */
                     }
      static void
   create_program_pipelines_err(struct gl_context *ctx, GLsizei n,
         {
               if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s (n < 0)", func);
                  }
      void GLAPIENTRY
   _mesa_GenProgramPipelines_no_error(GLsizei n, GLuint *pipelines)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
      void GLAPIENTRY
   _mesa_GenProgramPipelines(GLsizei n, GLuint *pipelines)
   {
               if (MESA_VERBOSE & VERBOSE_API)
               }
      void GLAPIENTRY
   _mesa_CreateProgramPipelines_no_error(GLsizei n, GLuint *pipelines)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
      void GLAPIENTRY
   _mesa_CreateProgramPipelines(GLsizei n, GLuint *pipelines)
   {
               if (MESA_VERBOSE & VERBOSE_API)
               }
      /**
   * Determine if ID is the name of an pipeline object.
   *
   * \param id  ID of the potential pipeline object.
   * \return  \c GL_TRUE if \c id is the name of a pipeline object,
   *          \c GL_FALSE otherwise.
   */
   GLboolean GLAPIENTRY
   _mesa_IsProgramPipeline(GLuint pipeline)
   {
               if (MESA_VERBOSE & VERBOSE_API)
            struct gl_pipeline_object *obj = _mesa_lookup_pipeline_object(ctx, pipeline);
   if (obj == NULL)
               }
      /**
   * glGetProgramPipelineiv() - get pipeline shader state.
   */
   void GLAPIENTRY
   _mesa_GetProgramPipelineiv(GLuint pipeline, GLenum pname, GLint *params)
   {
      GET_CURRENT_CONTEXT(ctx);
            if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glGetProgramPipelineiv(%u, %d, %p)\n",
         /* Are geometry shaders available in this context?
   */
   const bool has_gs = _mesa_has_geometry_shaders(ctx);
            if (!pipe) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     /* Object is created by any Pipeline call but glGenProgramPipelines,
   * glIsProgramPipeline and GetProgramPipelineInfoLog
   */
            switch (pname) {
   case GL_ACTIVE_PROGRAM:
      *params = pipe->ActiveProgram ? pipe->ActiveProgram->Name : 0;
      case GL_INFO_LOG_LENGTH:
      *params = (pipe->InfoLog && pipe->InfoLog[0] != '\0') ?
            case GL_VALIDATE_STATUS:
      *params = pipe->UserValidated;
      case GL_VERTEX_SHADER:
      *params = pipe->CurrentProgram[MESA_SHADER_VERTEX]
            case GL_TESS_EVALUATION_SHADER:
      if (!has_tess)
         *params = pipe->CurrentProgram[MESA_SHADER_TESS_EVAL]
            case GL_TESS_CONTROL_SHADER:
      if (!has_tess)
         *params = pipe->CurrentProgram[MESA_SHADER_TESS_CTRL]
            case GL_GEOMETRY_SHADER:
      if (!has_gs)
         *params = pipe->CurrentProgram[MESA_SHADER_GEOMETRY]
            case GL_FRAGMENT_SHADER:
      *params = pipe->CurrentProgram[MESA_SHADER_FRAGMENT]
            case GL_COMPUTE_SHADER:
      if (!_mesa_has_compute_shaders(ctx))
         *params = pipe->CurrentProgram[MESA_SHADER_COMPUTE]
            default:
                  _mesa_error(ctx, GL_INVALID_ENUM, "glGetProgramPipelineiv(pname=%s)",
      }
      /**
   * Determines whether every stage in a linked program is active in the
   * specified pipeline.
   */
   static bool
   program_stages_all_active(struct gl_pipeline_object *pipe,
         {
               if (!prog)
            unsigned mask = prog->sh.data->linked_stages;
   while (mask) {
      const int i = u_bit_scan(&mask);
   if (pipe->CurrentProgram[i]) {
      if (prog->Id != pipe->CurrentProgram[i]->Id) {
            } else {
                     if (!status) {
      pipe->InfoLog = ralloc_asprintf(pipe,
                              }
      static bool
   program_stages_interleaved_illegally(const struct gl_pipeline_object *pipe)
   {
               /* Look for programs bound to stages: A -> B -> A, with any intervening
   * sequence of unrelated programs or empty stages.
   */
   for (unsigned i = 0; i < MESA_SHADER_STAGES; i++) {
               /* Empty stages anywhere in the pipe are OK.  Also we can be confident
   * that if the linked_stages mask matches we are looking at the same
   * linked program because a previous validation call to
   * program_stages_all_active() will have already failed if two different
   * programs with the sames stages linked are not active for all linked
   * stages.
   */
   if (!cur || cur->sh.data->linked_stages == prev_linked_stages)
            if (prev_linked_stages) {
      /* We've seen an A -> B transition; look at the rest of the pipe
   * to see if we ever see A again.
   */
   if (prev_linked_stages >> (i + 1))
                              }
      extern GLboolean
   _mesa_validate_program_pipeline(struct gl_context* ctx,
         {
      unsigned i;
                     /* Release and reset the info log.
   */
   if (pipe->InfoLog != NULL)
                     /* Section 2.11.11 (Shader Execution), subheading "Validation," of the
   * OpenGL 4.1 spec says:
   *
   *     "[INVALID_OPERATION] is generated by any command that transfers
   *     vertices to the GL if:
   *
   *         - A program object is active for at least one, but not all of
   *           the shader stages that were present when the program was
   *           linked."
   *
   * For each possible program stage, verify that the program bound to that
   * stage has all of its stages active.  In other words, if the program
   * bound to the vertex stage also has a fragment shader, the fragment
   * shader must also be bound to the fragment stage.
   */
   for (i = 0; i < MESA_SHADER_STAGES; i++) {
      if (!program_stages_all_active(pipe, pipe->CurrentProgram[i])) {
                     /* Section 2.11.11 (Shader Execution), subheading "Validation," of the
   * OpenGL 4.1 spec says:
   *
   *     "[INVALID_OPERATION] is generated by any command that transfers
   *     vertices to the GL if:
   *
   *         ...
   *
   *         - One program object is active for at least two shader stages
   *           and a second program is active for a shader stage between two
   *           stages for which the first program was active."
   */
   if (program_stages_interleaved_illegally(pipe)) {
      pipe->InfoLog =
      ralloc_strdup(pipe,
                        /* Section 2.11.11 (Shader Execution), subheading "Validation," of the
   * OpenGL 4.1 spec says:
   *
   *     "[INVALID_OPERATION] is generated by any command that transfers
   *     vertices to the GL if:
   *
   *         ...
   *
   *         - There is an active program for tessellation control,
   *           tessellation evaluation, or geometry stages with corresponding
   *           executable shader, but there is no active program with
   *           executable vertex shader."
   */
   if (!pipe->CurrentProgram[MESA_SHADER_VERTEX]
      && (pipe->CurrentProgram[MESA_SHADER_GEOMETRY] ||
      pipe->CurrentProgram[MESA_SHADER_TESS_CTRL] ||
      pipe->InfoLog = ralloc_strdup(pipe, "Program lacks a vertex shader");
               /* Section 2.11.11 (Shader Execution), subheading "Validation," of the
   * OpenGL 4.1 spec says:
   *
   *     "[INVALID_OPERATION] is generated by any command that transfers
   *     vertices to the GL if:
   *
   *         ...
   *
   *         - There is no current program object specified by UseProgram,
   *           there is a current program pipeline object, and the current
   *           program for any shader stage has been relinked since being
   *           applied to the pipeline object via UseProgramStages with the
   *           PROGRAM_SEPARABLE parameter set to FALSE.
   */
   for (i = 0; i < MESA_SHADER_STAGES; i++) {
      if (pipe->CurrentProgram[i] &&
      !pipe->CurrentProgram[i]->info.separate_shader) {
   pipe->InfoLog = ralloc_asprintf(pipe,
                                    /* Section 11.1.3.11 (Validation) of the OpenGL 4.5 spec says:
   *
   *    "An INVALID_OPERATION error is generated by any command that trans-
   *    fers vertices to the GL or launches compute work if the current set
   *    of active program objects cannot be executed, for reasons including:
   *
   *       ...
   *
   *       - There is no current program object specified by UseProgram,
   *         there is a current program pipeline object, and that object is
   *         empty (no executable code is installed for any stage).
   */
   for (i = 0; i < MESA_SHADER_STAGES; i++) {
      if (pipe->CurrentProgram[i]) {
      program_empty = false;
                  if (program_empty) {
                  /* Section 2.11.11 (Shader Execution), subheading "Validation," of the
   * OpenGL 4.1 spec says:
   *
   *     "[INVALID_OPERATION] is generated by any command that transfers
   *     vertices to the GL if:
   *
   *         ...
   *
   *         - Any two active samplers in the current program object are of
   *           different types, but refer to the same texture image unit.
   *
   *         - The number of active samplers in the program exceeds the
   *           maximum number of texture image units allowed."
   */
   if (!_mesa_sampler_uniforms_pipeline_are_valid(pipe))
            /* Validate inputs against outputs, this cannot be done during linking
   * since programs have been linked separately from each other.
   *
   * Section 11.1.3.11 (Validation) of the OpenGL 4.5 Core Profile spec says:
   *
   *     "Separable program objects may have validation failures that cannot be
   *     detected without the complete program pipeline. Mismatched interfaces,
   *     improper usage of program objects together, and the same
   *     state-dependent failures can result in validation errors for such
   *     program objects."
   *
   * OpenGL ES 3.1 specification has the same text.
   *
   * Section 11.1.3.11 (Validation) of the OpenGL ES spec also says:
   *
   *    An INVALID_OPERATION error is generated by any command that transfers
   *    vertices to the GL or launches compute work if the current set of
   *    active program objects cannot be executed, for reasons including:
   *
   *    * The current program pipeline object contains a shader interface
   *      that doesn't have an exact match (see section 7.4.1)
   *
   * Based on this, only perform the most-strict checking on ES or when the
   * application has created a debug context.
   */
   if ((_mesa_is_gles(ctx) || (ctx->Const.ContextFlags & GL_CONTEXT_FLAG_DEBUG_BIT)) &&
      !_mesa_validate_pipeline_io(pipe)) {
   if (_mesa_is_gles(ctx))
                     _mesa_gl_debugf(ctx, &msg_id,
                  MESA_DEBUG_SOURCE_API,
   MESA_DEBUG_TYPE_PORTABILITY,
   MESA_DEBUG_SEVERITY_MEDIUM,
   "glValidateProgramPipeline: pipeline %u does not meet "
            pipe->Validated = GL_TRUE;
      }
      /**
   * Check compatibility of pipeline's program
   */
   void GLAPIENTRY
   _mesa_ValidateProgramPipeline(GLuint pipeline)
   {
               if (MESA_VERBOSE & VERBOSE_API)
                     if (!pipe) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     _mesa_validate_program_pipeline(ctx, pipe);
      }
      void GLAPIENTRY
   _mesa_GetProgramPipelineInfoLog(GLuint pipeline, GLsizei bufSize,
         {
               if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glGetProgramPipelineInfoLog(%u, %d, %p, %p)\n",
                  if (!pipe) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                     if (bufSize < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                        }
