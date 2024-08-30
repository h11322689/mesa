   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 2010  VMware, Inc.  All Rights Reserved.
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
         /*
   * Transform feedback support.
   *
   * Authors:
   *   Brian Paul
   */
         #include "buffers.h"
   #include "context.h"
   #include "draw_validate.h"
   #include "hash.h"
   #include "macros.h"
   #include "mtypes.h"
   #include "transformfeedback.h"
   #include "shaderapi.h"
   #include "shaderobj.h"
      #include "program/program.h"
   #include "program/prog_parameter.h"
      #include "util/u_memory.h"
   #include "util/u_inlines.h"
      #include "api_exec_decl.h"
      #include "cso_cache/cso_context.h"
   struct using_program_tuple
   {
      struct gl_program *prog;
      };
      static void
   active_xfb_object_references_program(void *data, void *user_data)
   {
      struct using_program_tuple *callback_data = user_data;
   struct gl_transform_feedback_object *obj = data;
   if (obj->Active && obj->program == callback_data->prog)
      }
      /**
   * Return true if any active transform feedback object is using a program.
   */
   bool
   _mesa_transform_feedback_is_using_program(struct gl_context *ctx,
         {
      if (!shProg->last_vert_prog)
            struct using_program_tuple callback_data;
   callback_data.found = false;
            _mesa_HashWalkLocked(ctx->TransformFeedback.Objects,
            /* Also check DefaultObject, as it's not in the Objects hash table. */
   active_xfb_object_references_program(ctx->TransformFeedback.DefaultObject,
               }
      static struct gl_transform_feedback_object *
   new_transform_feedback(struct gl_context *ctx, GLuint name)
   {
               obj = CALLOC_STRUCT(gl_transform_feedback_object);
   if (!obj)
            obj->Name = name;
   obj->RefCount = 1;
               }
      static void
   delete_transform_feedback(struct gl_context *ctx,
         {
               for (i = 0; i < ARRAY_SIZE(obj->draw_count); i++)
            /* Unreference targets. */
   for (i = 0; i < obj->num_targets; i++) {
                  for (unsigned i = 0; i < ARRAY_SIZE(obj->Buffers); i++) {
                  free(obj->Label);
      }
      /**
   * Do reference counting of transform feedback buffers.
   */
   static void
   reference_transform_feedback_object(struct gl_transform_feedback_object **ptr,
         {
      if (*ptr == obj)
            if (*ptr) {
      /* Unreference the old object */
            assert(oldObj->RefCount > 0);
            if (oldObj->RefCount == 0) {
      GET_CURRENT_CONTEXT(ctx);
   if (ctx)
                  }
            if (obj) {
               /* reference new object */
   obj->RefCount++;
   obj->EverBound = GL_TRUE;
         }
         /**
   * Per-context init for transform feedback.
   */
   void
   _mesa_init_transform_feedback(struct gl_context *ctx)
   {
      /* core mesa expects this, even a dummy one, to be available */
   ctx->TransformFeedback.DefaultObject =
                     reference_transform_feedback_object(&ctx->TransformFeedback.CurrentObject,
                              _mesa_reference_buffer_object(ctx,
      }
            /**
   * Callback for _mesa_HashDeleteAll().
   */
   static void
   delete_cb(void *data, void *userData)
   {
      struct gl_context *ctx = (struct gl_context *) userData;
   struct gl_transform_feedback_object *obj =
               }
         /**
   * Per-context free/clean-up for transform feedback.
   */
   void
   _mesa_free_transform_feedback(struct gl_context *ctx)
   {
      /* core mesa expects this, even a dummy one, to be available */
   _mesa_reference_buffer_object(ctx,
                  /* Delete all feedback objects */
   _mesa_HashDeleteAll(ctx->TransformFeedback.Objects, delete_cb, ctx);
            /* Delete the default feedback object */
   delete_transform_feedback(ctx,
               }
      /**
   * Fill in the correct Size value for each buffer in \c obj.
   *
   * From the GL 4.3 spec, section 6.1.1 ("Binding Buffer Objects to Indexed
   * Targets"):
   *
   *   BindBufferBase binds the entire buffer, even when the size of the buffer
   *   is changed after the binding is established. It is equivalent to calling
   *   BindBufferRange with offset zero, while size is determined by the size of
   *   the bound buffer at the time the binding is used.
   *
   *   Regardless of the size specified with BindBufferRange, or indirectly with
   *   BindBufferBase, the GL will never read or write beyond the end of a bound
   *   buffer. In some cases this constraint may result in visibly different
   *   behavior when a buffer overflow would otherwise result, such as described
   *   for transform feedback operations in section 13.2.2.
   */
   static void
   compute_transform_feedback_buffer_sizes(
         {
      unsigned i = 0;
   for (i = 0; i < MAX_FEEDBACK_BUFFERS; ++i) {
      GLintptr offset = obj->Offset[i];
   GLsizeiptr buffer_size
         GLsizeiptr available_space
         GLsizeiptr computed_size;
   if (obj->RequestedSize[i] == 0) {
      /* No size was specified at the time the buffer was bound, so allow
   * writing to all available space in the buffer.
   */
      } else {
      /* A size was specified at the time the buffer was bound, however
   * it's possible that the buffer has shrunk since then.  So only
   * allow writing to the minimum of the specified size and the space
   * available.
   */
               /* Legal sizes must be multiples of four, so round down if necessary. */
         }
         /**
   * Compute the maximum number of vertices that can be written to the currently
   * enabled transform feedback buffers without overflowing any of them.
   */
   unsigned
   _mesa_compute_max_transform_feedback_vertices(struct gl_context *ctx,
         const struct gl_transform_feedback_object *obj,
   {
      unsigned max_index = 0xffffffff;
            for (i = 0; i < ctx->Const.MaxTransformFeedbackBuffers; i++) {
      if ((info->ActiveBuffers >> i) & 1) {
                     /* Skip any inactive buffers, which have a stride of 0. */
                  max_for_this_buffer = obj->Size[i] / (4 * stride);
                     }
         /**
   ** Begin API functions
   **/
         /**
   * Figure out which stage of the pipeline is the source of transform feedback
   * data given the current context state, and return its gl_program.
   *
   * If no active program can generate transform feedback data (i.e. no vertex
   * shader is active), returns NULL.
   */
   static struct gl_program *
   get_xfb_source(struct gl_context *ctx)
   {
      int i;
   for (i = MESA_SHADER_GEOMETRY; i >= MESA_SHADER_VERTEX; i--) {
      if (ctx->_Shader->CurrentProgram[i] != NULL)
      }
      }
         static ALWAYS_INLINE void
   begin_transform_feedback(struct gl_context *ctx, GLenum mode, bool no_error)
   {
      struct gl_transform_feedback_object *obj;
   struct gl_transform_feedback_info *info = NULL;
   struct gl_program *source;
   GLuint i;
                     /* Figure out what pipeline stage is the source of data for transform
   * feedback.
   */
   source = get_xfb_source(ctx);
   if (!no_error && source == NULL) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                              if (!no_error && info->NumOutputs == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     switch (mode) {
   case GL_POINTS:
      vertices_per_prim = 1;
      case GL_LINES:
      vertices_per_prim = 2;
      case GL_TRIANGLES:
      vertices_per_prim = 3;
      default:
      if (!no_error) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBeginTransformFeedback(mode)");
      } else {
      /* Stop compiler warnings */
                  if (!no_error) {
      if (obj->Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     for (i = 0; i < ctx->Const.MaxTransformFeedbackBuffers; i++) {
      if ((info->ActiveBuffers >> i) & 1) {
      if (obj->BufferNames[i] == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                                             obj->Active = GL_TRUE;
                     if (_mesa_is_gles3(ctx)) {
      /* In GLES3, we are required to track the usage of the transform
   * feedback buffer and report INVALID_OPERATION if a draw call tries to
   * exceed it.  So compute the maximum number of vertices that we can
   * write without overflowing any of the buffers currently being used for
   * feedback.
   */
   unsigned max_vertices
                     if (obj->program != source) {
      _mesa_reference_program_(ctx, &obj->program, source);
               struct pipe_context *pipe = ctx->pipe;
   unsigned max_num_targets;
            max_num_targets = MIN2(ARRAY_SIZE(obj->Buffers),
            /* Convert the transform feedback state into the gallium representation. */
   for (i = 0; i < max_num_targets; i++) {
               if (bo && bo->buffer) {
                     /* Check whether we need to recreate the target. */
   if (!obj->targets[i] ||
      obj->targets[i] == obj->draw_count[stream] ||
   obj->targets[i]->buffer != bo->buffer ||
   obj->targets[i]->buffer_offset != obj->Offset[i] ||
   obj->targets[i]->buffer_size != obj->Size[i]) {
   /* Create a new target. */
   struct pipe_stream_output_target *so_target =
                        pipe_so_target_reference(&obj->targets[i], NULL);
                  } else {
                     /* Start writing at the beginning of each target. */
   cso_set_stream_outputs(ctx->cso_context, obj->num_targets,
            }
         void GLAPIENTRY
   _mesa_BeginTransformFeedback_no_error(GLenum mode)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_BeginTransformFeedback(GLenum mode)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         static void
   end_transform_feedback(struct gl_context *ctx,
         {
      unsigned i;
                     /* The next call to glDrawTransformFeedbackStream should use the vertex
   * count from the last call to glEndTransformFeedback.
   * Therefore, save the targets for each stream.
   *
   * NULL means the vertex counter is 0 (initial state).
   */
   for (i = 0; i < ARRAY_SIZE(obj->draw_count); i++)
            for (i = 0; i < ARRAY_SIZE(obj->targets); i++) {
      unsigned stream = obj->program->sh.LinkedTransformFeedback->
            /* Is it not bound or already set for this stream? */
   if (!obj->targets[i] || obj->draw_count[stream])
                        _mesa_reference_program_(ctx, &obj->program, NULL);
   ctx->TransformFeedback.CurrentObject->Active = GL_FALSE;
   ctx->TransformFeedback.CurrentObject->Paused = GL_FALSE;
   ctx->TransformFeedback.CurrentObject->EndedAnytime = GL_TRUE;
      }
         void GLAPIENTRY
   _mesa_EndTransformFeedback_no_error(void)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_EndTransformFeedback(void)
   {
      struct gl_transform_feedback_object *obj;
                     if (!obj->Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                        }
         /**
   * Helper used by BindBufferRange() and BindBufferBase().
   */
   static void
   bind_buffer_range(struct gl_context *ctx,
                     struct gl_transform_feedback_object *obj,
   GLuint index,
   {
      /* Note: no need to FLUSH_VERTICES because
   * transform feedback buffers can't be changed while transform feedback is
   * active.
            if (!dsa) {
      /* The general binding point */
   _mesa_reference_buffer_object(ctx,
                     /* The per-attribute binding point */
      }
         /**
   * Validate the buffer object to receive transform feedback results.  Plus,
   * validate the starting offset to place the results, and max size.
   * Called from the glBindBufferRange() and glTransformFeedbackBufferRange
   * functions.
   */
   bool
   _mesa_validate_buffer_range_xfb(struct gl_context *ctx,
                     {
      const char *gl_methd_name;
   if (dsa)
         else
               if (obj->Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(transform feedback active)",
                     if (index >= ctx->Const.MaxTransformFeedbackBuffers) {
      /* OpenGL 4.5 core profile, 6.1, pdf page 82: "An INVALID_VALUE error is
   * generated if index is greater than or equal to the number of binding
   * points for transform feedback, as described in section 6.7.1."
   */
   _mesa_error(ctx, GL_INVALID_VALUE, "%s(index=%d out of bounds)",
                     if (size & 0x3) {
      /* OpenGL 4.5 core profile, 6.7, pdf page 103: multiple of 4 */
   _mesa_error(ctx, GL_INVALID_VALUE, "%s(size=%d must be a multiple of "
                     if (offset & 0x3) {
      /* OpenGL 4.5 core profile, 6.7, pdf page 103: multiple of 4 */
   _mesa_error(ctx, GL_INVALID_VALUE, "%s(offset=%d must be a multiple "
                     if (offset < 0) {
      /* OpenGL 4.5 core profile, 6.1, pdf page 82: "An INVALID_VALUE error is
   * generated by BindBufferRange if offset is negative."
   *
   * OpenGL 4.5 core profile, 13.2, pdf page 445: "An INVALID_VALUE error
   * is generated by TransformFeedbackBufferRange if offset is negative."
   */
   _mesa_error(ctx, GL_INVALID_VALUE, "%s(offset=%d must be >= 0)",
                           if (size <= 0 && (dsa || bufObj)) {
      /* OpenGL 4.5 core profile, 6.1, pdf page 82: "An INVALID_VALUE error is
   * generated by BindBufferRange if buffer is non-zero and size is less
   * than or equal to zero."
   *
   * OpenGL 4.5 core profile, 13.2, pdf page 445: "An INVALID_VALUE error
   * is generated by TransformFeedbackBufferRange if size is less than or
   * equal to zero."
   */
   _mesa_error(ctx, GL_INVALID_VALUE, "%s(size=%d must be > 0)",
                        }
         /**
   * Specify a buffer object to receive transform feedback results.
   * As above, but start at offset = 0.
   * Called from the glBindBufferBase() and glTransformFeedbackBufferBase()
   * functions.
   */
   void
   _mesa_bind_buffer_base_transform_feedback(struct gl_context *ctx,
                           {
      if (obj->Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                           if (index >= ctx->Const.MaxTransformFeedbackBuffers) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(index=%d out of bounds)",
                              }
      /**
   * Wrapper around lookup_transform_feedback_object that throws
   * GL_INVALID_OPERATION if id is not in the hash table. After calling
   * _mesa_error, it returns NULL.
   */
   static struct gl_transform_feedback_object *
   lookup_transform_feedback_object_err(struct gl_context *ctx,
         {
               obj = _mesa_lookup_transform_feedback_object(ctx, xfb);
   if (!obj) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  }
      /**
   * Wrapper around _mesa_lookup_bufferobj that throws GL_INVALID_VALUE if id
   * is not in the hash table. Specialised version for the
   * transform-feedback-related functions. After calling _mesa_error, it
   * returns NULL.
   */
   static struct gl_buffer_object *
   lookup_transform_feedback_bufferobj_err(struct gl_context *ctx,
               {
                        /* OpenGL 4.5 core profile, 13.2, pdf page 444: buffer must be zero or the
   * name of an existing buffer object.
   */
   if (buffer) {
      bufObj = _mesa_lookup_bufferobj(ctx, buffer);
   if (!bufObj) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(invalid buffer=%u)", func,
                           }
      void GLAPIENTRY
   _mesa_TransformFeedbackBufferBase(GLuint xfb, GLuint index, GLuint buffer)
   {
      GET_CURRENT_CONTEXT(ctx);
   struct gl_transform_feedback_object *obj;
            obj = lookup_transform_feedback_object_err(ctx, xfb,
         if (!obj) {
                  bool error;
   bufObj = lookup_transform_feedback_bufferobj_err(ctx, buffer,
               if (error) {
                     }
      void GLAPIENTRY
   _mesa_TransformFeedbackBufferRange(GLuint xfb, GLuint index, GLuint buffer,
         {
      GET_CURRENT_CONTEXT(ctx);
   struct gl_transform_feedback_object *obj;
            obj = lookup_transform_feedback_object_err(ctx, xfb,
         if (!obj) {
                  bool error;
   bufObj = lookup_transform_feedback_bufferobj_err(ctx, buffer,
               if (error) {
                  if (!_mesa_validate_buffer_range_xfb(ctx, obj, index, bufObj, offset,
                  /* The per-attribute binding point */
   _mesa_set_transform_feedback_binding(ctx, obj, index, bufObj, offset,
      }
      /**
   * Specify a buffer object to receive transform feedback results, plus the
   * offset in the buffer to start placing results.
   * This function is part of GL_EXT_transform_feedback, but not GL3.
   */
   static ALWAYS_INLINE void
   bind_buffer_offset(struct gl_context *ctx,
               {
               if (buffer == 0) {
         } else {
      bufObj = _mesa_lookup_bufferobj(ctx, buffer);
   if (!no_error && !bufObj) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                           }
         void GLAPIENTRY
   _mesa_BindBufferOffsetEXT_no_error(GLenum target, GLuint index, GLuint buffer,
         {
      GET_CURRENT_CONTEXT(ctx);
   bind_buffer_offset(ctx, ctx->TransformFeedback.CurrentObject, index, buffer,
      }
         void GLAPIENTRY
   _mesa_BindBufferOffsetEXT(GLenum target, GLuint index, GLuint buffer,
         {
      struct gl_transform_feedback_object *obj;
            if (target != GL_TRANSFORM_FEEDBACK_BUFFER) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBindBufferOffsetEXT(target)");
                        if (obj->Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     if (index >= ctx->Const.MaxTransformFeedbackBuffers) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                     if (offset & 0x3) {
      /* must be multiple of four */
   _mesa_error(ctx, GL_INVALID_VALUE,
                        }
         /**
   * This function specifies the transform feedback outputs to be written
   * to the feedback buffer(s), and in what order.
   */
   static ALWAYS_INLINE void
   transform_feedback_varyings(struct gl_context *ctx,
               {
               /* free existing varyings, if any */
   for (i = 0; i < (GLint) shProg->TransformFeedback.NumVarying; i++) {
         }
            /* allocate new memory for varying names */
   shProg->TransformFeedback.VaryingNames =
            if (!shProg->TransformFeedback.VaryingNames) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTransformFeedbackVaryings()");
               /* Save the new names and the count */
   for (i = 0; i < count; i++) {
         }
                     /* No need to invoke FLUSH_VERTICES since
   * the varyings won't be used until shader link time.
      }
         void GLAPIENTRY
   _mesa_TransformFeedbackVaryings_no_error(GLuint program, GLsizei count,
               {
               struct gl_shader_program *shProg = _mesa_lookup_shader_program(ctx, program);
      }
      void GLAPIENTRY
   _mesa_TransformFeedbackVaryings(GLuint program, GLsizei count,
               {
      struct gl_shader_program *shProg;
   GLint i;
            /* From the ARB_transform_feedback2 specification:
   * "The error INVALID_OPERATION is generated by TransformFeedbackVaryings
   *  if the current transform feedback object is active, even if paused."
   */
   if (ctx->TransformFeedback.CurrentObject->Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     switch (bufferMode) {
   case GL_INTERLEAVED_ATTRIBS:
         case GL_SEPARATE_ATTRIBS:
         default:
      _mesa_error(ctx, GL_INVALID_ENUM,
                     if (count < 0 ||
      (bufferMode == GL_SEPARATE_ATTRIBS &&
   (GLuint) count > ctx->Const.MaxTransformFeedbackBuffers)) {
   _mesa_error(ctx, GL_INVALID_VALUE,
                     shProg = _mesa_lookup_shader_program_err(ctx, program,
         if (!shProg)
            if (ctx->Extensions.ARB_transform_feedback3) {
      if (bufferMode == GL_INTERLEAVED_ATTRIBS) {
               for (i = 0; i < count; i++) {
      if (strcmp(varyings[i], "gl_NextBuffer") == 0)
               if (buffers > ctx->Const.MaxTransformFeedbackBuffers) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     } else {
      for (i = 0; i < count; i++) {
      if (strcmp(varyings[i], "gl_NextBuffer") == 0 ||
      strcmp(varyings[i], "gl_SkipComponents1") == 0 ||
   strcmp(varyings[i], "gl_SkipComponents2") == 0 ||
   strcmp(varyings[i], "gl_SkipComponents3") == 0 ||
   strcmp(varyings[i], "gl_SkipComponents4") == 0) {
   _mesa_error(ctx, GL_INVALID_OPERATION,
               "glTransformFeedbackVaryings(SEPARATE_ATTRIBS,"
                           }
         /**
   * Get info about the transform feedback outputs which are to be written
   * to the feedback buffer(s).
   */
   void GLAPIENTRY
   _mesa_GetTransformFeedbackVarying(GLuint program, GLuint index,
               {
      const struct gl_shader_program *shProg;
   struct gl_program_resource *res;
            shProg = _mesa_lookup_shader_program_err(ctx, program,
         if (!shProg)
            res = _mesa_program_resource_find_index((struct gl_shader_program *) shProg,
               if (!res) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                     /* return the varying's name and length */
            /* return the datatype and value's size (in datatype units) */
   if (type)
      _mesa_program_resource_prop((struct gl_shader_program *) shProg,
            if (size)
      _mesa_program_resource_prop((struct gl_shader_program *) shProg,
         }
            struct gl_transform_feedback_object *
   _mesa_lookup_transform_feedback_object(struct gl_context *ctx, GLuint name)
   {
      /* OpenGL 4.5 core profile, 13.2 pdf page 444: "xfb must be zero, indicating
   * the default transform feedback object, or the name of an existing
   * transform feedback object."
   */
   if (name == 0) {
         }
   else
      return (struct gl_transform_feedback_object *)
   }
      static void
   create_transform_feedbacks(struct gl_context *ctx, GLsizei n, GLuint *ids,
         {
               if (dsa)
         else
            if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(n < 0)", func);
               if (!ids)
            if (_mesa_HashFindFreeKeys(ctx->TransformFeedback.Objects, ids, n)) {
      GLsizei i;
   for (i = 0; i < n; i++) {
      struct gl_transform_feedback_object *obj
         if (!obj) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "%s", func);
      }
   _mesa_HashInsertLocked(ctx->TransformFeedback.Objects, ids[i],
         if (dsa) {
      /* this is normally done at bind time in the non-dsa case */
            }
   else {
            }
      /**
   * Create new transform feedback objects.   Transform feedback objects
   * encapsulate the state related to transform feedback to allow quickly
   * switching state (and drawing the results, below).
   * Part of GL_ARB_transform_feedback2.
   */
   void GLAPIENTRY
   _mesa_GenTransformFeedbacks(GLsizei n, GLuint *names)
   {
               /* GenTransformFeedbacks should just reserve the object names that a
   * subsequent call to BindTransformFeedback should actively create. For
   * the sake of simplicity, we reserve the names and create the objects
   * straight away.
               }
      /**
   * Create new transform feedback objects.   Transform feedback objects
   * encapsulate the state related to transform feedback to allow quickly
   * switching state (and drawing the results, below).
   * Part of GL_ARB_direct_state_access.
   */
   void GLAPIENTRY
   _mesa_CreateTransformFeedbacks(GLsizei n, GLuint *names)
   {
                  }
         /**
   * Is the given ID a transform feedback object?
   * Part of GL_ARB_transform_feedback2.
   */
   GLboolean GLAPIENTRY
   _mesa_IsTransformFeedback(GLuint name)
   {
      struct gl_transform_feedback_object *obj;
                     if (name == 0)
            obj = _mesa_lookup_transform_feedback_object(ctx, name);
   if (obj == NULL)
               }
         /**
   * Bind the given transform feedback object.
   * Part of GL_ARB_transform_feedback2.
   */
   static ALWAYS_INLINE void
   bind_transform_feedback(struct gl_context *ctx, GLuint name, bool no_error)
   {
               obj = _mesa_lookup_transform_feedback_object(ctx, name);
   if (!no_error && !obj) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     reference_transform_feedback_object(&ctx->TransformFeedback.CurrentObject,
      }
         void GLAPIENTRY
   _mesa_BindTransformFeedback_no_error(GLenum target, GLuint name)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_BindTransformFeedback(GLenum target, GLuint name)
   {
               if (target != GL_TRANSFORM_FEEDBACK) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glBindTransformFeedback(target)");
               if (_mesa_is_xfb_active_and_unpaused(ctx)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                        }
         /**
   * Delete the given transform feedback objects.
   * Part of GL_ARB_transform_feedback2.
   */
   void GLAPIENTRY
   _mesa_DeleteTransformFeedbacks(GLsizei n, const GLuint *names)
   {
      GLint i;
            if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glDeleteTransformFeedbacks(n < 0)");
               if (!names)
            for (i = 0; i < n; i++) {
      if (names[i] > 0) {
      struct gl_transform_feedback_object *obj
         if (obj) {
      if (obj->Active) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  }
   _mesa_HashRemoveLocked(ctx->TransformFeedback.Objects, names[i]);
   /* unref, but object may not be deleted until later */
   if (obj == ctx->TransformFeedback.CurrentObject) {
      reference_transform_feedback_object(
            }
               }
         /**
   * Pause transform feedback.
   * Part of GL_ARB_transform_feedback2.
   */
   static void
   pause_transform_feedback(struct gl_context *ctx,
         {
                        obj->Paused = GL_TRUE;
      }
         void GLAPIENTRY
   _mesa_PauseTransformFeedback_no_error(void)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_PauseTransformFeedback(void)
   {
      struct gl_transform_feedback_object *obj;
                     if (!_mesa_is_xfb_active_and_unpaused(ctx)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                        }
         /**
   * Resume transform feedback.
   * Part of GL_ARB_transform_feedback2.
   */
   static void
   resume_transform_feedback(struct gl_context *ctx,
         {
                        unsigned offsets[PIPE_MAX_SO_BUFFERS];
            for (i = 0; i < PIPE_MAX_SO_BUFFERS; i++)
            cso_set_stream_outputs(ctx->cso_context, obj->num_targets,
            }
         void GLAPIENTRY
   _mesa_ResumeTransformFeedback_no_error(void)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_ResumeTransformFeedback(void)
   {
      struct gl_transform_feedback_object *obj;
                     if (!obj->Active || !obj->Paused) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     /* From the ARB_transform_feedback2 specification:
   * "The error INVALID_OPERATION is generated by ResumeTransformFeedback if
   *  the program object being used by the current transform feedback object
   *  is not active."
   */
   if (obj->program != get_xfb_source(ctx)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                        }
      extern void GLAPIENTRY
   _mesa_GetTransformFeedbackiv(GLuint xfb, GLenum pname, GLint *param)
   {
      struct gl_transform_feedback_object *obj;
            obj = lookup_transform_feedback_object_err(ctx, xfb,
         if (!obj) {
                  switch(pname) {
   case GL_TRANSFORM_FEEDBACK_PAUSED:
      *param = obj->Paused;
      case GL_TRANSFORM_FEEDBACK_ACTIVE:
      *param = obj->Active;
      default:
      _mesa_error(ctx, GL_INVALID_ENUM,
         }
      extern void GLAPIENTRY
   _mesa_GetTransformFeedbacki_v(GLuint xfb, GLenum pname, GLuint index,
         {
      struct gl_transform_feedback_object *obj;
            obj = lookup_transform_feedback_object_err(ctx, xfb,
         if (!obj) {
                  if (index >= ctx->Const.MaxTransformFeedbackBuffers) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                     switch(pname) {
   case GL_TRANSFORM_FEEDBACK_BUFFER_BINDING:
      *param = obj->BufferNames[index];
      default:
      _mesa_error(ctx, GL_INVALID_ENUM,
         }
      extern void GLAPIENTRY
   _mesa_GetTransformFeedbacki64_v(GLuint xfb, GLenum pname, GLuint index,
         {
      struct gl_transform_feedback_object *obj;
            obj = lookup_transform_feedback_object_err(ctx, xfb,
         if (!obj) {
                  if (index >= ctx->Const.MaxTransformFeedbackBuffers) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                     /**
   * This follows the same general rules used for BindBufferBase:
   *
   *   "To query the starting offset or size of the range of a buffer
   *    object binding in an indexed array, call GetInteger64i_v with
   *    target set to respectively the starting offset or binding size
   *    name from table 6.5 for that array. Index must be in the range
   *    zero to the number of bind points supported minus one. If the
   *    starting offset or size was not specified when the buffer object
   *    was bound (e.g. if it was bound with BindBufferBase), or if no
   *    buffer object is bound to the target array at index, zero is
   *    returned."
   */
   if (obj->RequestedSize[index] == 0 &&
      (pname == GL_TRANSFORM_FEEDBACK_BUFFER_START ||
   pname == GL_TRANSFORM_FEEDBACK_BUFFER_SIZE)) {
   *param = 0;
               compute_transform_feedback_buffer_sizes(obj);
   switch(pname) {
   case GL_TRANSFORM_FEEDBACK_BUFFER_START:
      assert(obj->RequestedSize[index] > 0);
   *param = obj->Offset[index];
      case GL_TRANSFORM_FEEDBACK_BUFFER_SIZE:
      assert(obj->RequestedSize[index] > 0);
   *param = obj->Size[index];
      default:
      _mesa_error(ctx, GL_INVALID_ENUM,
         }
