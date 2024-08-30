   /**************************************************************************
   *
   * Copyright 2003 VMware, Inc.
   * Copyright 2009 VMware, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      #include <stdio.h>
   #include "arrayobj.h"
   #include "util/glheader.h"
   #include "c99_alloca.h"
   #include "context.h"
   #include "state.h"
   #include "draw.h"
   #include "draw_validate.h"
   #include "dispatch.h"
   #include "varray.h"
   #include "bufferobj.h"
   #include "enums.h"
   #include "macros.h"
   #include "transformfeedback.h"
   #include "pipe/p_state.h"
   #include "api_exec_decl.h"
      #include "state_tracker/st_context.h"
   #include "state_tracker/st_draw.h"
   #include "util/u_threaded_context.h"
      typedef struct {
      GLuint count;
   GLuint primCount;
   GLuint first;
      } DrawArraysIndirectCommand;
      typedef struct {
      GLuint count;
   GLuint primCount;
   GLuint firstIndex;
   GLint  baseVertex;
      } DrawElementsIndirectCommand;
         /**
   * Want to figure out which fragment program inputs are actually
   * constant/current values from ctx->Current.  These should be
   * referenced as a tracked state variable rather than a fragment
   * program input, to save the overhead of putting a constant value in
   * every submitted vertex, transferring it to hardware, interpolating
   * it across the triangle, etc...
   *
   * When there is a VP bound, just use vp->outputs.  But when we're
   * generating vp from fixed function state, basically want to
   * calculate:
   *
   * vp_out_2_fp_in( vp_in_2_vp_out( varying_inputs ) |
   *                 potential_vp_outputs )
   *
   * Where potential_vp_outputs is calculated by looking at enabled
   * texgen, etc.
   *
   * The generated fragment program should then only declare inputs that
   * may vary or otherwise differ from the ctx->Current values.
   * Otherwise, the fp should track them as state values instead.
   */
   void
   _mesa_set_varying_vp_inputs(struct gl_context *ctx, GLbitfield varying_inputs)
   {
      if (ctx->VertexProgram._VPModeOptimizesConstantAttribs &&
      ctx->VertexProgram._VaryingInputs != varying_inputs) {
   ctx->VertexProgram._VaryingInputs = varying_inputs;
         }
         /**
   * Set the _DrawVAO and the net enabled arrays.
   * The vao->_Enabled bitmask is transformed due to position/generic0
   * as stored in vao->_AttributeMapMode. Then the filter bitmask is applied
   * to filter out arrays unwanted for the currently executed draw operation.
   * For example, the generic attributes are masked out form the _DrawVAO's
   * enabled arrays when a fixed function array draw is executed.
   */
   void
   _mesa_set_draw_vao(struct gl_context *ctx, struct gl_vertex_array_object *vao)
   {
               if (*ptr != vao) {
      _mesa_reference_vao_(ctx, ptr, vao);
   _mesa_update_edgeflag_state_vao(ctx);
   ctx->NewDriverState |= ST_NEW_VERTEX_ARRAYS;
         }
      /**
   * Other than setting the new VAO, this returns a VAO reference to
   * the previously-bound VAO and the previous _VPModeInputFilter value through
   * parameters. The caller must call _mesa_restore_draw_vao to ensure
   * reference counting is done properly and the affected states are restored.
   *
   * \param ctx  GL context
   * \param vao  VAO to set.
   * \param vp_input_filter  Mask of enabled vertex attribs.
   *        Possible values that can also be OR'd with each other:
   *        - VERT_BIT_FF_ALL
   *        - VERT_BIT_MAT_ALL
   *        - VERT_BIT_ALL
   *        - VERT_BIT_SELECT_RESULT_OFFSET
   * \param old_vao  Previous bound VAO.
   * \param old_vp_input_filter  Previous value of vp_input_filter.
   */
   void
   _mesa_save_and_set_draw_vao(struct gl_context *ctx,
                           {
      *old_vao = ctx->Array._DrawVAO;
            ctx->Array._DrawVAO = NULL;
   ctx->VertexProgram._VPModeInputFilter = vp_input_filter;
      }
      void
   _mesa_restore_draw_vao(struct gl_context *ctx,
               {
      /* Restore states. */
   _mesa_reference_vao(ctx, &ctx->Array._DrawVAO, NULL);
   ctx->Array._DrawVAO = saved;
            /* Update states. */
   ctx->NewDriverState |= ST_NEW_VERTEX_ARRAYS;
            /* Restore original states. */
      }
      /**
   * Is 'mode' a valid value for glBegin(), glDrawArrays(), glDrawElements(),
   * etc?  Also, do additional checking related to transformation feedback.
   * Note: this function cannot be called during glNewList(GL_COMPILE) because
   * this code depends on current transform feedback state.
   * Also, do additional checking related to tessellation shaders.
   */
   static GLenum
   valid_prim_mode_custom(struct gl_context *ctx, GLenum mode,
         {
   #if DEBUG
      ASSERTED unsigned mask = ctx->ValidPrimMask;
   ASSERTED unsigned mask_indexed = ctx->ValidPrimMaskIndexed;
   ASSERTED bool drawpix_valid = ctx->DrawPixValid;
   _mesa_update_valid_to_render_state(ctx);
   assert(mask == ctx->ValidPrimMask &&
            #endif
         /* All primitive type enums are less than 32, so we can use the shift. */
   if (mode >= 32 || !((1u << mode) & valid_prim_mask)) {
      /* If the primitive type is not in SupportedPrimMask, set GL_INVALID_ENUM,
   * else set DrawGLError (e.g. GL_INVALID_OPERATION).
   */
   return mode >= 32 || !((1u << mode) & ctx->SupportedPrimMask) ?
                  }
      GLenum
   _mesa_valid_prim_mode(struct gl_context *ctx, GLenum mode)
   {
         }
      static GLenum
   valid_prim_mode_indexed(struct gl_context *ctx, GLenum mode)
   {
         }
      /**
   * Verify that the element type is valid.
   *
   * Generates \c GL_INVALID_ENUM and returns \c false if it is not.
   */
   static GLenum
   valid_elements_type(struct gl_context *ctx, GLenum type)
   {
      /* GL_UNSIGNED_BYTE  = 0x1401
   * GL_UNSIGNED_SHORT = 0x1403
   * GL_UNSIGNED_INT   = 0x1405
   *
   * The trick is that bit 1 and bit 2 mean USHORT and UINT, respectively.
   * After clearing those two bits (with ~6), we should get UBYTE.
   * Both bits can't be set, because the enum would be greater than UINT.
   */
   if (!(type <= GL_UNSIGNED_INT && (type & ~6) == GL_UNSIGNED_BYTE))
               }
      static inline bool
   indices_aligned(unsigned index_size_shift, const GLvoid *indices)
   {
      /* Require that indices are aligned to the element size. GL doesn't specify
   * an error for this, but the ES 3.0 spec says:
   *
   *    "Clients must align data elements consistently with the requirements
   *     of the client platform, with an additional base-level requirement
   *     that an offset within a buffer to a datum comprising N basic machine
   *     units be a multiple of N"
   *
   * This is only required by index buffers, not user indices.
   */
      }
      static GLenum
   validate_DrawElements_common(struct gl_context *ctx, GLenum mode,
         {
      if (count < 0 || numInstances < 0)
            GLenum error = valid_prim_mode_indexed(ctx, mode);
   if (error)
               }
      /**
   * Error checking for glDrawElements().  Includes parameter checking
   * and VBO bounds checking.
   * \return GL_TRUE if OK to render, GL_FALSE if error found
   */
   static GLboolean
   _mesa_validate_DrawElements(struct gl_context *ctx,
         {
      GLenum error = validate_DrawElements_common(ctx, mode, count, 1, type);
   if (error)
               }
         /**
   * Error checking for glMultiDrawElements().  Includes parameter checking
   * and VBO bounds checking.
   * \return GL_TRUE if OK to render, GL_FALSE if error found
   */
   static GLboolean
   _mesa_validate_MultiDrawElements(struct gl_context *ctx,
                           {
               /*
   * Section 2.3.1 (Errors) of the OpenGL 4.5 (Core Profile) spec says:
   *
   *    "If a negative number is provided where an argument of type sizei or
   *     sizeiptr is specified, an INVALID_VALUE error is generated."
   *
   * and in the same section:
   *
   *    "In other cases, there are no side effects unless otherwise noted;
   *     the command which generates the error is ignored so that it has no
   *     effect on GL state or framebuffer contents."
   *
   * Hence, check both primcount and all the count[i].
   */
   if (primcount < 0) {
         } else {
               if (!error) {
               if (!error) {
      for (int i = 0; i < primcount; i++) {
      if (count[i] < 0) {
      error = GL_INVALID_VALUE;
                           if (error)
            /* Not using a VBO for indices, so avoid NULL pointer derefs later.
   */
   if (!index_bo) {
      for (int i = 0; i < primcount; i++) {
      if (!indices[i])
                     }
         /**
   * Error checking for glDrawRangeElements().  Includes parameter checking
   * and VBO bounds checking.
   * \return GL_TRUE if OK to render, GL_FALSE if error found
   */
   static GLboolean
   _mesa_validate_DrawRangeElements(struct gl_context *ctx, GLenum mode,
               {
               if (end < start) {
         } else {
                  if (error)
               }
         static bool
   need_xfb_remaining_prims_check(const struct gl_context *ctx)
   {
      /* From the GLES3 specification, section 2.14.2 (Transform Feedback
   * Primitive Capture):
   *
   *   The error INVALID_OPERATION is generated by DrawArrays and
   *   DrawArraysInstanced if recording the vertices of a primitive to the
   *   buffer objects being used for transform feedback purposes would result
   *   in either exceeding the limits of any buffer object’s size, or in
   *   exceeding the end position offset + size − 1, as set by
   *   BindBufferRange.
   *
   * This is in contrast to the behaviour of desktop GL, where the extra
   * primitives are silently dropped from the transform feedback buffer.
   *
   * This text is removed in ES 3.2, presumably because it's not really
   * implementable with geometry and tessellation shaders.  In fact,
   * the OES_geometry_shader spec says:
   *
   *    "(13) Does this extension change how transform feedback operates
   *     compared to unextended OpenGL ES 3.0 or 3.1?
   *
   *     RESOLVED: Yes. Because dynamic geometry amplification in a geometry
   *     shader can make it difficult if not impossible to predict the amount
   *     of geometry that may be generated in advance of executing the shader,
   *     the draw-time error for transform feedback buffer overflow conditions
   *     is removed and replaced with the GL behavior (primitives are not
   *     written and the corresponding counter is not updated)..."
   */
   return _mesa_is_gles3(ctx) && _mesa_is_xfb_active_and_unpaused(ctx) &&
            }
         /**
   * Figure out the number of transform feedback primitives that will be output
   * considering the drawing mode, number of vertices, and instance count,
   * assuming that no geometry shading is done and primitive restart is not
   * used.
   *
   * This is used by driver back-ends in implementing the PRIMITIVES_GENERATED
   * and TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN queries.  It is also used to
   * pre-validate draw calls in GLES3 (where draw calls only succeed if there is
   * enough room in the transform feedback buffer for the result).
   */
   static size_t
   count_tessellated_primitives(GLenum mode, GLuint count, GLuint num_instances)
   {
      size_t num_primitives;
   switch (mode) {
   case GL_POINTS:
      num_primitives = count;
      case GL_LINE_STRIP:
      num_primitives = count >= 2 ? count - 1 : 0;
      case GL_LINE_LOOP:
      num_primitives = count >= 2 ? count : 0;
      case GL_LINES:
      num_primitives = count / 2;
      case GL_TRIANGLE_STRIP:
   case GL_TRIANGLE_FAN:
   case GL_POLYGON:
      num_primitives = count >= 3 ? count - 2 : 0;
      case GL_TRIANGLES:
      num_primitives = count / 3;
      case GL_QUAD_STRIP:
      num_primitives = count >= 4 ? ((count / 2) - 1) * 2 : 0;
      case GL_QUADS:
      num_primitives = (count / 4) * 2;
      case GL_LINES_ADJACENCY:
      num_primitives = count / 4;
      case GL_LINE_STRIP_ADJACENCY:
      num_primitives = count >= 4 ? count - 3 : 0;
      case GL_TRIANGLES_ADJACENCY:
      num_primitives = count / 6;
      case GL_TRIANGLE_STRIP_ADJACENCY:
      num_primitives = count >= 6 ? (count - 4) / 2 : 0;
      default:
      assert(!"Unexpected primitive type in count_tessellated_primitives");
   num_primitives = 0;
      }
      }
         static GLenum
   validate_draw_arrays(struct gl_context *ctx,
         {
      if (count < 0 || numInstances < 0)
            GLenum error = _mesa_valid_prim_mode(ctx, mode);
   if (error)
            if (need_xfb_remaining_prims_check(ctx)) {
      struct gl_transform_feedback_object *xfb_obj
         size_t prim_count = count_tessellated_primitives(mode, count, numInstances);
   if (xfb_obj->GlesRemainingPrims < prim_count)
                           }
      /**
   * Called from the tnl module to error check the function parameters and
   * verify that we really can draw something.
   * \return GL_TRUE if OK to render, GL_FALSE if error found
   */
   static GLboolean
   _mesa_validate_DrawArrays(struct gl_context *ctx, GLenum mode, GLsizei count)
   {
               if (error)
               }
         static GLboolean
   _mesa_validate_DrawArraysInstanced(struct gl_context *ctx, GLenum mode, GLint first,
         {
               if (first < 0) {
         } else {
                  if (error)
               }
         /**
   * Called to error check the function parameters.
   *
   * Note that glMultiDrawArrays is not part of GLES, so there's limited scope
   * for sharing code with the validation of glDrawArrays.
   */
   static bool
   _mesa_validate_MultiDrawArrays(struct gl_context *ctx, GLenum mode,
         {
               if (primcount < 0) {
         } else {
               if (!error) {
      for (int i = 0; i < primcount; ++i) {
      if (count[i] < 0) {
      error = GL_INVALID_VALUE;
                  if (!error) {
      if (need_xfb_remaining_prims_check(ctx)) {
                           for (int i = 0; i < primcount; ++i) {
                        if (xfb_obj->GlesRemainingPrims < xfb_prim_count) {
         } else {
                              if (error)
               }
         static GLboolean
   _mesa_validate_DrawElementsInstanced(struct gl_context *ctx,
               {
      GLenum error =
            if (error)
               }
         static GLboolean
   _mesa_validate_DrawTransformFeedback(struct gl_context *ctx,
                           {
               /* From the GL 4.5 specification, page 429:
   * "An INVALID_VALUE error is generated if id is not the name of a
   *  transform feedback object."
   */
   if (!obj || !obj->EverBound || stream >= ctx->Const.MaxVertexStreams ||
      numInstances < 0) {
      } else {
               if (!error) {
      if (!obj->EndedAnytime)
                  if (error)
               }
      static GLenum
   valid_draw_indirect(struct gl_context *ctx,
               {
               /* OpenGL ES 3.1 spec. section 10.5:
   *
   *      "DrawArraysIndirect requires that all data sourced for the
   *      command, including the DrawArraysIndirectCommand
   *      structure,  be in buffer objects,  and may not be called when
   *      the default vertex array object is bound."
   */
   if (ctx->API != API_OPENGL_COMPAT &&
      ctx->Array.VAO == ctx->Array.DefaultVAO)
         /* From OpenGL ES 3.1 spec. section 10.5:
   *     "An INVALID_OPERATION error is generated if zero is bound to
   *     VERTEX_ARRAY_BINDING, DRAW_INDIRECT_BUFFER or to any enabled
   *     vertex array."
   *
   * Here we check that for each enabled vertex array we have a vertex
   * buffer bound.
   */
   if (_mesa_is_gles31(ctx) &&
      ctx->Array.VAO->Enabled & ~ctx->Array.VAO->VertexAttribBufferMask)
         GLenum error = _mesa_valid_prim_mode(ctx, mode);
   if (error)
            /* OpenGL ES 3.1 specification, section 10.5:
   *
   *      "An INVALID_OPERATION error is generated if
   *      transform feedback is active and not paused."
   *
   * The OES_geometry_shader spec says:
   *
   *    On p. 250 in the errors section for the DrawArraysIndirect command,
   *    and on p. 254 in the errors section for the DrawElementsIndirect
   *    command, delete the errors which state:
   *
   *    "An INVALID_OPERATION error is generated if transform feedback is
   *    active and not paused."
   *
   *    (thus allowing transform feedback to work with indirect draw commands).
   */
   if (_mesa_is_gles31(ctx) && !ctx->Extensions.OES_geometry_shader &&
      _mesa_is_xfb_active_and_unpaused(ctx))
         /* From OpenGL version 4.4. section 10.5
   * and OpenGL ES 3.1, section 10.6:
   *
   *      "An INVALID_VALUE error is generated if indirect is not a
   *       multiple of the size, in basic machine units, of uint."
   */
   if ((GLsizeiptr)indirect & (sizeof(GLuint) - 1))
            if (!ctx->DrawIndirectBuffer)
            if (_mesa_check_disallowed_mapping(ctx->DrawIndirectBuffer))
            /* From the ARB_draw_indirect specification:
   * "An INVALID_OPERATION error is generated if the commands source data
   *  beyond the end of the buffer object [...]"
   */
   if (ctx->DrawIndirectBuffer->Size < end)
               }
      static inline GLenum
   valid_draw_indirect_elements(struct gl_context *ctx,
               {
      GLenum error = valid_elements_type(ctx, type);
   if (error)
            /*
   * Unlike regular DrawElementsInstancedBaseVertex commands, the indices
   * may not come from a client array and must come from an index buffer.
   * If no element array buffer is bound, an INVALID_OPERATION error is
   * generated.
   */
   if (!ctx->Array.VAO->IndexBufferObj)
               }
      static GLboolean
   _mesa_valid_draw_indirect_multi(struct gl_context *ctx,
               {
         /* From the ARB_multi_draw_indirect specification:
   * "INVALID_VALUE is generated by MultiDrawArraysIndirect or
   *  MultiDrawElementsIndirect if <primcount> is negative."
   *
   * "<primcount> must be positive, otherwise an INVALID_VALUE error will
   *  be generated."
   */
   if (primcount < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(primcount < 0)", name);
                  /* From the ARB_multi_draw_indirect specification:
   * "<stride> must be a multiple of four, otherwise an INVALID_VALUE
   *  error is generated."
   */
   if (stride % 4) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(stride %% 4)", name);
                  }
      static GLboolean
   _mesa_validate_DrawArraysIndirect(struct gl_context *ctx,
               {
      const unsigned drawArraysNumParams = 4;
   GLenum error =
      valid_draw_indirect(ctx, mode, indirect,
         if (error)
               }
      static GLboolean
   _mesa_validate_DrawElementsIndirect(struct gl_context *ctx,
               {
      const unsigned drawElementsNumParams = 5;
   GLenum error = valid_draw_indirect_elements(ctx, mode, type, indirect,
               if (error)
               }
      static GLboolean
   _mesa_validate_MultiDrawArraysIndirect(struct gl_context *ctx,
                     {
      GLsizeiptr size = 0;
            /* caller has converted stride==0 to drawArraysNumParams * sizeof(GLuint) */
            if (!_mesa_valid_draw_indirect_multi(ctx, primcount, stride,
                  /* number of bytes of the indirect buffer which will be read */
   size = primcount
      ? (primcount - 1) * stride + drawArraysNumParams * sizeof(GLuint)
         GLenum error = valid_draw_indirect(ctx, mode, indirect, size);
   if (error)
               }
      static GLboolean
   _mesa_validate_MultiDrawElementsIndirect(struct gl_context *ctx,
                     {
      GLsizeiptr size = 0;
            /* caller has converted stride==0 to drawElementsNumParams * sizeof(GLuint) */
            if (!_mesa_valid_draw_indirect_multi(ctx, primcount, stride,
                  /* number of bytes of the indirect buffer which will be read */
   size = primcount
      ? (primcount - 1) * stride + drawElementsNumParams * sizeof(GLuint)
         GLenum error = valid_draw_indirect_elements(ctx, mode, type, indirect,
         if (error)
               }
      static GLenum
   valid_draw_indirect_parameters(struct gl_context *ctx,
         {
      /* From the ARB_indirect_parameters specification:
   * "INVALID_VALUE is generated by MultiDrawArraysIndirectCountARB or
   *  MultiDrawElementsIndirectCountARB if <drawcount> is not a multiple of
   *  four."
   */
   if (drawcount & 3)
            /* From the ARB_indirect_parameters specification:
   * "INVALID_OPERATION is generated by MultiDrawArraysIndirectCountARB or
   *  MultiDrawElementsIndirectCountARB if no buffer is bound to the
   *  PARAMETER_BUFFER_ARB binding point."
   */
   if (!ctx->ParameterBuffer)
            if (_mesa_check_disallowed_mapping(ctx->ParameterBuffer))
            /* From the ARB_indirect_parameters specification:
   * "INVALID_OPERATION is generated by MultiDrawArraysIndirectCountARB or
   *  MultiDrawElementsIndirectCountARB if reading a <sizei> typed value
   *  from the buffer bound to the PARAMETER_BUFFER_ARB target at the offset
   *  specified by <drawcount> would result in an out-of-bounds access."
   */
   if (ctx->ParameterBuffer->Size < drawcount + sizeof(GLsizei))
               }
      static GLboolean
   _mesa_validate_MultiDrawArraysIndirectCount(struct gl_context *ctx,
                                 {
      GLsizeiptr size = 0;
            /* caller has converted stride==0 to drawArraysNumParams * sizeof(GLuint) */
            if (!_mesa_valid_draw_indirect_multi(ctx, maxdrawcount, stride,
                  /* number of bytes of the indirect buffer which will be read */
   size = maxdrawcount
      ? (maxdrawcount - 1) * stride + drawArraysNumParams * sizeof(GLuint)
         GLenum error = valid_draw_indirect(ctx, mode, (void *)indirect, size);
   if (!error)
            if (error)
               }
      static GLboolean
   _mesa_validate_MultiDrawElementsIndirectCount(struct gl_context *ctx,
                                 {
      GLsizeiptr size = 0;
            /* caller has converted stride==0 to drawElementsNumParams * sizeof(GLuint) */
            if (!_mesa_valid_draw_indirect_multi(ctx, maxdrawcount, stride,
                  /* number of bytes of the indirect buffer which will be read */
   size = maxdrawcount
      ? (maxdrawcount - 1) * stride + drawElementsNumParams * sizeof(GLuint)
         GLenum error = valid_draw_indirect_elements(ctx, mode, type,
         if (!error)
            if (error)
               }
      static inline struct pipe_draw_start_count_bias *
   get_temp_draws(struct gl_context *ctx, unsigned primcount)
   {
      if (primcount > ctx->num_tmp_draws) {
      struct pipe_draw_start_count_bias *tmp =
            if (tmp) {
      ctx->tmp_draws = tmp;
      } else {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "can't alloc tmp_draws");
   free(ctx->tmp_draws); /* realloc doesn't free on failure */
   ctx->tmp_draws = NULL;
         }
      }
      /**
   * Check that element 'j' of the array has reasonable data.
   * Map VBO if needed.
   * For debugging purposes; not normally used.
   */
   static void
   check_array_data(struct gl_context *ctx, struct gl_vertex_array_object *vao,
         {
      const struct gl_array_attributes *array = &vao->VertexAttrib[attrib];
   if (vao->Enabled & VERT_BIT(attrib)) {
      const struct gl_vertex_buffer_binding *binding =
         struct gl_buffer_object *bo = binding->BufferObj;
   const void *data = array->Ptr;
   if (bo) {
      data = ADD_POINTERS(_mesa_vertex_attrib_address(array, binding),
      }
   switch (array->Format.User.Type) {
   case GL_FLOAT:
      {
      GLfloat *f = (GLfloat *) ((GLubyte *) data + binding->Stride * j);
   GLint k;
   for (k = 0; k < array->Format.User.Size; k++) {
      if (util_is_inf_or_nan(f[k]) || f[k] >= 1.0e20F || f[k] <= -1.0e10F) {
      printf("Bad array data:\n");
   printf("  Element[%u].%u = %f\n", j, k, f[k]);
   printf("  Array %u at %p\n", attrib, (void *) array);
   printf("  Type 0x%x, Size %d, Stride %d\n",
         array->Format.User.Type, array->Format.User.Size,
   printf("  Address/offset %p in Buffer Object %u\n",
            }
         }
      default:
               }
         static inline unsigned
   get_index_size_shift(GLenum type)
   {
      /* The type is already validated, so use a fast conversion.
   *
   * GL_UNSIGNED_BYTE  - GL_UNSIGNED_BYTE = 0
   * GL_UNSIGNED_SHORT - GL_UNSIGNED_BYTE = 2
   * GL_UNSIGNED_INT   - GL_UNSIGNED_BYTE = 4
   *
   * Divide by 2 to get 0,1,2.
   */
      }
      /**
   * Examine the array's data for NaNs, etc.
   * For debug purposes; not normally used.
   */
   static void
   check_draw_elements_data(struct gl_context *ctx, GLsizei count,
               {
      struct gl_vertex_array_object *vao = ctx->Array.VAO;
   GLint i;
                     if (vao->IndexBufferObj)
      elements =
         for (i = 0; i < count; i++) {
               /* j = element[i] */
   switch (elemType) {
   case GL_UNSIGNED_BYTE:
      j = ((const GLubyte *) elements)[i];
      case GL_UNSIGNED_SHORT:
      j = ((const GLushort *) elements)[i];
      case GL_UNSIGNED_INT:
      j = ((const GLuint *) elements)[i];
      default:
                  /* check element j of each enabled array */
   for (k = 0; k < VERT_ATTRIB_MAX; k++) {
                        }
         /**
   * Check array data, looking for NaNs, etc.
   */
   static void
   check_draw_arrays_data(struct gl_context *ctx, GLint start, GLsizei count)
   {
         }
         /**
   * Print info/data for glDrawArrays(), for debugging.
   */
   static void
   print_draw_arrays(struct gl_context *ctx,
         {
               printf("_mesa_DrawArrays(mode 0x%x, start %d, count %d):\n",
                     GLbitfield mask = vao->Enabled;
   while (mask) {
      const gl_vert_attrib i = u_bit_scan(&mask);
            const struct gl_vertex_buffer_binding *binding =
                  printf("attr %s: size %d stride %d  "
         "ptr %p  Bufobj %u\n",
   gl_vert_attrib_name((gl_vert_attrib) i),
            if (bufObj) {
      GLubyte *p = bufObj->Mappings[MAP_INTERNAL].Pointer;
                  unsigned multiplier;
   switch (array->Format.User.Type) {
   case GL_DOUBLE:
   case GL_INT64_ARB:
   case GL_UNSIGNED_INT64_ARB:
      multiplier = 2;
      default:
                  float *f = (float *) (p + offset);
   int *k = (int *) f;
   int i = 0;
   int n = (count - 1) * (binding->Stride / (4 * multiplier))
         if (n > 32)
         printf("  Data at offset %d:\n", offset);
   do {
      if (multiplier == 2)
      printf("    double[%d] = 0x%016llx %lf\n", i,
      else
                              }
         /**
   * Helper function called by the other DrawArrays() functions below.
   * This is where we handle primitive restart for drawing non-indexed
   * arrays.  If primitive restart is enabled, it typically means
   * splitting one DrawArrays() into two.
   */
   static void
   _mesa_draw_arrays(struct gl_context *ctx, GLenum mode, GLint start,
         {
      /* Viewperf has many draws with count=0. Discarding them is faster than
   * processing them.
   */
   if (!count || !numInstances)
            /* OpenGL 4.5 says that primitive restart is ignored with non-indexed
   * draws.
   */
   struct pipe_draw_info info;
            info.mode = mode;
   info.index_size = 0;
   /* Packed section begin. */
   info.primitive_restart = false;
   info.has_user_indices = false;
   info.index_bounds_valid = true;
   info.increment_draw_id = false;
   info.was_line_loop = false;
   info.take_index_buffer_ownership = false;
   info.index_bias_varies = false;
   /* Packed section end. */
   info.start_instance = baseInstance;
   info.instance_count = numInstances;
   info.view_mask = 0;
   info.min_index = start;
            draw.start = start;
                     if (MESA_DEBUG_FLAGS & DEBUG_ALWAYS_FLUSH) {
            }
         /**
   * Execute a glRectf() function.
   */
   void GLAPIENTRY
   _mesa_Rectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
   {
      GET_CURRENT_CONTEXT(ctx);
            CALL_Begin(ctx->Dispatch.Current, (GL_QUADS));
   /* Begin can change Dispatch.Current. */
   struct _glapi_table *dispatch = ctx->Dispatch.Current;
   CALL_Vertex2f(dispatch, (x1, y1));
   CALL_Vertex2f(dispatch, (x2, y1));
   CALL_Vertex2f(dispatch, (x2, y2));
   CALL_Vertex2f(dispatch, (x1, y2));
      }
         void GLAPIENTRY
   _mesa_Rectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)
   {
         }
      void GLAPIENTRY
   _mesa_Rectdv(const GLdouble *v1, const GLdouble *v2)
   {
         }
      void GLAPIENTRY
   _mesa_Rectfv(const GLfloat *v1, const GLfloat *v2)
   {
         }
      void GLAPIENTRY
   _mesa_Recti(GLint x1, GLint y1, GLint x2, GLint y2)
   {
         }
      void GLAPIENTRY
   _mesa_Rectiv(const GLint *v1, const GLint *v2)
   {
         }
      void GLAPIENTRY
   _mesa_Rects(GLshort x1, GLshort y1, GLshort x2, GLshort y2)
   {
         }
      void GLAPIENTRY
   _mesa_Rectsv(const GLshort *v1, const GLshort *v2)
   {
         }
         void GLAPIENTRY
   _mesa_EvalMesh1(GLenum mode, GLint i1, GLint i2)
   {
      GET_CURRENT_CONTEXT(ctx);
   GLint i;
   GLfloat u, du;
            switch (mode) {
   case GL_POINT:
      prim = GL_POINTS;
      case GL_LINE:
      prim = GL_LINE_STRIP;
      default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glEvalMesh1(mode)");
               /* No effect if vertex maps disabled.
   */
   if (!ctx->Eval.Map1Vertex4 && !ctx->Eval.Map1Vertex3)
            du = ctx->Eval.MapGrid1du;
               CALL_Begin(ctx->Dispatch.Current, (prim));
   /* Begin can change Dispatch.Current. */
   struct _glapi_table *dispatch = ctx->Dispatch.Current;
   for (i = i1; i <= i2; i++, u += du) {
         }
      }
         void GLAPIENTRY
   _mesa_EvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2)
   {
      GET_CURRENT_CONTEXT(ctx);
   GLfloat u, du, v, dv, v1, u1;
            switch (mode) {
   case GL_POINT:
   case GL_LINE:
   case GL_FILL:
         default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glEvalMesh2(mode)");
               /* No effect if vertex maps disabled.
   */
   if (!ctx->Eval.Map2Vertex4 && !ctx->Eval.Map2Vertex3)
            du = ctx->Eval.MapGrid2du;
   dv = ctx->Eval.MapGrid2dv;
   v1 = ctx->Eval.MapGrid2v1 + j1 * dv;
                     switch (mode) {
   case GL_POINT:
      CALL_Begin(ctx->Dispatch.Current, (GL_POINTS));
   /* Begin can change Dispatch.Current. */
   dispatch = ctx->Dispatch.Current;
   for (v = v1, j = j1; j <= j2; j++, v += dv) {
      for (u = u1, i = i1; i <= i2; i++, u += du) {
            }
   CALL_End(dispatch, ());
      case GL_LINE:
      for (v = v1, j = j1; j <= j2; j++, v += dv) {
      CALL_Begin(ctx->Dispatch.Current, (GL_LINE_STRIP));
   /* Begin can change Dispatch.Current. */
   dispatch = ctx->Dispatch.Current;
   for (u = u1, i = i1; i <= i2; i++, u += du) {
         }
      }
   for (u = u1, i = i1; i <= i2; i++, u += du) {
      CALL_Begin(ctx->Dispatch.Current, (GL_LINE_STRIP));
   /* Begin can change Dispatch.Current. */
   dispatch = ctx->Dispatch.Current;
   for (v = v1, j = j1; j <= j2; j++, v += dv) {
         }
      }
      case GL_FILL:
      for (v = v1, j = j1; j < j2; j++, v += dv) {
      CALL_Begin(ctx->Dispatch.Current, (GL_TRIANGLE_STRIP));
   /* Begin can change Dispatch.Current. */
   dispatch = ctx->Dispatch.Current;
   for (u = u1, i = i1; i <= i2; i++, u += du) {
      CALL_EvalCoord2f(dispatch, (u, v));
      }
      }
         }
         /**
   * Called from glDrawArrays when in immediate mode (not display list mode).
   */
   void GLAPIENTRY
   _mesa_DrawArrays(GLenum mode, GLint start, GLsizei count)
   {
      GET_CURRENT_CONTEXT(ctx);
            _mesa_set_varying_vp_inputs(ctx, ctx->VertexProgram._VPModeInputFilter &
         if (ctx->NewState)
            if (!_mesa_is_no_error_enabled(ctx) &&
      !_mesa_validate_DrawArrays(ctx, mode, count))
         if (0)
                     if (0)
      }
         /**
   * Called from glDrawArraysInstanced when in immediate mode (not
   * display list mode).
   */
   void GLAPIENTRY
   _mesa_DrawArraysInstanced(GLenum mode, GLint start, GLsizei count,
         {
      GET_CURRENT_CONTEXT(ctx);
            _mesa_set_varying_vp_inputs(ctx, ctx->VertexProgram._VPModeInputFilter &
         if (ctx->NewState)
            if (!_mesa_is_no_error_enabled(ctx) &&
      !_mesa_validate_DrawArraysInstanced(ctx, mode, start, count,
               if (0)
                     if (0)
      }
         /**
   * Called from glDrawArraysInstancedBaseInstance when in immediate mode.
   */
   void GLAPIENTRY
   _mesa_DrawArraysInstancedBaseInstance(GLenum mode, GLint first,
               {
      GET_CURRENT_CONTEXT(ctx);
            _mesa_set_varying_vp_inputs(ctx, ctx->VertexProgram._VPModeInputFilter &
         if (ctx->NewState)
            if (!_mesa_is_no_error_enabled(ctx) &&
      !_mesa_validate_DrawArraysInstanced(ctx, mode, first, count,
               if (0)
                     if (0)
      }
         /**
   * Called from glMultiDrawArrays when in immediate mode.
   */
   void GLAPIENTRY
   _mesa_MultiDrawArrays(GLenum mode, const GLint *first,
         {
      GET_CURRENT_CONTEXT(ctx);
            _mesa_set_varying_vp_inputs(ctx, ctx->VertexProgram._VPModeInputFilter &
         if (ctx->NewState)
            if (!_mesa_is_no_error_enabled(ctx) &&
      !_mesa_validate_MultiDrawArrays(ctx, mode, count, primcount))
         if (primcount == 0)
            struct pipe_draw_info info;
   struct pipe_draw_start_count_bias *draw = get_temp_draws(ctx, primcount);
   if (!draw)
            info.mode = mode;
   info.index_size = 0;
   /* Packed section begin. */
   info.primitive_restart = false;
   info.has_user_indices = false;
   info.index_bounds_valid = false;
   info.increment_draw_id = primcount > 1;
   info.was_line_loop = false;
   info.take_index_buffer_ownership = false;
   info.index_bias_varies = false;
   /* Packed section end. */
   info.start_instance = 0;
   info.instance_count = 1;
            for (int i = 0; i < primcount; i++) {
      draw[i].start = first[i];
                        if (MESA_DEBUG_FLAGS & DEBUG_ALWAYS_FLUSH)
      }
            /**
   * Map GL_ELEMENT_ARRAY_BUFFER and print contents.
   * For debugging.
   */
   #if 0
   static void
   dump_element_buffer(struct gl_context *ctx, GLenum type)
   {
      const GLvoid *map =
      ctx->Driver.MapBufferRange(ctx, 0,
                        switch (type) {
   case GL_UNSIGNED_BYTE:
      {
      const GLubyte *us = (const GLubyte *) map;
   GLint i;
   for (i = 0; i < ctx->Array.VAO->IndexBufferObj->Size; i++) {
      printf("%02x ", us[i]);
   if (i % 32 == 31)
      }
      }
      case GL_UNSIGNED_SHORT:
      {
      const GLushort *us = (const GLushort *) map;
   GLint i;
   for (i = 0; i < ctx->Array.VAO->IndexBufferObj->Size / 2; i++) {
      printf("%04x ", us[i]);
   if (i % 16 == 15)
      }
      }
      case GL_UNSIGNED_INT:
      {
      const GLuint *us = (const GLuint *) map;
   GLint i;
   for (i = 0; i < ctx->Array.VAO->IndexBufferObj->Size / 4; i++) {
      printf("%08x ", us[i]);
   if (i % 8 == 7)
      }
      }
      default:
                     }
   #endif
         /**
   * Inner support for both _mesa_DrawElements and _mesa_DrawRangeElements.
   * Do the rendering for a glDrawElements or glDrawRangeElements call after
   * we've validated buffer bounds, etc.
   */
   static void
   _mesa_validated_drawrangeelements(struct gl_context *ctx,
                                    struct gl_buffer_object *index_bo,
   GLenum mode,
      {
      /* Viewperf has many draws with count=0. Discarding them is faster than
   * processing them.
   */
   if (!count || !numInstances)
            if (!index_bounds_valid) {
      assert(start == 0u);
               struct pipe_draw_info info;
   struct pipe_draw_start_count_bias draw;
            if (index_bo && !indices_aligned(index_size_shift, indices))
            info.mode = mode;
   info.index_size = 1 << index_size_shift;
   /* Packed section begin. */
   info.primitive_restart = ctx->Array._PrimitiveRestart[index_size_shift];
   info.has_user_indices = index_bo == NULL;
   info.index_bounds_valid = index_bounds_valid;
   info.increment_draw_id = false;
   info.was_line_loop = false;
   info.take_index_buffer_ownership = false;
   info.index_bias_varies = false;
   /* Packed section end. */
   info.start_instance = baseInstance;
   info.instance_count = numInstances;
   info.view_mask = 0;
            if (info.has_user_indices) {
      info.index.user = indices;
      } else {
      uintptr_t start = (uintptr_t) indices;
   if (unlikely(index_bo->Size < start || !index_bo->buffer)) {
      _mesa_warning(ctx, "Invalid indices offset 0x%" PRIxPTR
                                          if (ctx->st->pipe->draw_vbo == tc_draw_vbo) {
      /* Fast path for u_threaded_context to eliminate atomics. */
   info.index.resource = _mesa_get_bufferobj_reference(ctx, index_bo);
      } else {
            }
            info.min_index = start;
   info.max_index = end;
            /* Need to give special consideration to rendering a range of
   * indices starting somewhere above zero.  Typically the
   * application is issuing multiple DrawRangeElements() to draw
   * successive primitives layed out linearly in the vertex arrays.
   * Unless the vertex arrays are all in a VBO (or locked as with
   * CVA), the OpenGL semantics imply that we need to re-read or
   * re-upload the vertex data on each draw call.
   *
   * In the case of hardware tnl, we want to avoid starting the
   * upload at zero, as it will mean every draw call uploads an
   * increasing amount of not-used vertex data.  Worse - in the
   * software tnl module, all those vertices might be transformed and
   * lit but never rendered.
   *
   * If we just upload or transform the vertices in start..end,
   * however, the indices will be incorrect.
   *
   * At this level, we don't know exactly what the requirements of
   * the backend are going to be, though it will likely boil down to
   * either:
   *
   * 1) Do nothing, everything is in a VBO and is processed once
   *       only.
   *
   * 2) Adjust the indices and vertex arrays so that start becomes
   *    zero.
   *
   * Rather than doing anything here, I'll provide a helper function
   * for the latter case elsewhere.
                     if (MESA_DEBUG_FLAGS & DEBUG_ALWAYS_FLUSH) {
            }
         /**
   * Called by glDrawRangeElementsBaseVertex() in immediate mode.
   */
   void GLAPIENTRY
   _mesa_DrawRangeElementsBaseVertex(GLenum mode, GLuint start, GLuint end,
               {
      static GLuint warnCount = 0;
            /* This is only useful to catch invalid values in the "end" parameter
   * like ~0.
   */
            GET_CURRENT_CONTEXT(ctx);
            _mesa_set_varying_vp_inputs(ctx, ctx->VertexProgram._VPModeInputFilter &
         if (ctx->NewState)
            if (!_mesa_is_no_error_enabled(ctx) &&
      !_mesa_validate_DrawRangeElements(ctx, mode, start, end, count,
               if ((int) end + basevertex < 0 || start + basevertex >= max_element) {
      /* The application requested we draw using a range of indices that's
   * outside the bounds of the current VBO.  This is invalid and appears
   * to give undefined results.  The safest thing to do is to simply
   * ignore the range, in case the application botched their range tracking
   * but did provide valid indices.  Also issue a warning indicating that
   * the application is broken.
   */
   if (warnCount++ < 10) {
      _mesa_warning(ctx, "glDrawRangeElements(start %u, end %u, "
               "basevertex %d, count %d, type 0x%x, indices=%p):\n"
   "\trange is outside VBO bounds (max=%u); ignoring.\n"
      }
               /* NOTE: It's important that 'end' is a reasonable value.
   * in _tnl_draw_prims(), we use end to determine how many vertices
   * to transform.  If it's too large, we can unnecessarily split prims
   * or we can read/write out of memory in several different places!
            /* Catch/fix some potential user errors */
   if (type == GL_UNSIGNED_BYTE) {
      start = MIN2(start, 0xff);
      }
   else if (type == GL_UNSIGNED_SHORT) {
      start = MIN2(start, 0xffff);
               if (0) {
      printf("glDraw[Range]Elements{,BaseVertex}"
         "(start %u, end %u, type 0x%x, count %d) ElemBuf %u, "
   "base %d\n",
   start, end, type, count,
               if ((int) start + basevertex < 0 || end + basevertex >= max_element)
         #if 0
         #else
         #endif
         if (!index_bounds_valid) {
      start = 0;
               _mesa_validated_drawrangeelements(ctx, ctx->Array.VAO->IndexBufferObj,
            }
         /**
   * Called by glDrawRangeElements() in immediate mode.
   */
   void GLAPIENTRY
   _mesa_DrawRangeElements(GLenum mode, GLuint start, GLuint end,
         {
      _mesa_DrawRangeElementsBaseVertex(mode, start, end, count, type,
      }
         /**
   * Called by glDrawElements() in immediate mode.
   */
   void GLAPIENTRY
   _mesa_DrawElements(GLenum mode, GLsizei count, GLenum type,
         {
      GET_CURRENT_CONTEXT(ctx);
            _mesa_set_varying_vp_inputs(ctx, ctx->VertexProgram._VPModeInputFilter &
         if (ctx->NewState)
            if (!_mesa_is_no_error_enabled(ctx) &&
      !_mesa_validate_DrawElements(ctx, mode, count, type))
         _mesa_validated_drawrangeelements(ctx, ctx->Array.VAO->IndexBufferObj,
            }
         /**
   * Called by glDrawElementsBaseVertex() in immediate mode.
   */
   void GLAPIENTRY
   _mesa_DrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type,
         {
      GET_CURRENT_CONTEXT(ctx);
            _mesa_set_varying_vp_inputs(ctx, ctx->VertexProgram._VPModeInputFilter &
         if (ctx->NewState)
            if (!_mesa_is_no_error_enabled(ctx) &&
      !_mesa_validate_DrawElements(ctx, mode, count, type))
         _mesa_validated_drawrangeelements(ctx, ctx->Array.VAO->IndexBufferObj,
            }
         /**
   * Called by glDrawElementsInstanced() in immediate mode.
   */
   void GLAPIENTRY
   _mesa_DrawElementsInstanced(GLenum mode, GLsizei count, GLenum type,
         {
      GET_CURRENT_CONTEXT(ctx);
            _mesa_set_varying_vp_inputs(ctx, ctx->VertexProgram._VPModeInputFilter &
         if (ctx->NewState)
            if (!_mesa_is_no_error_enabled(ctx) &&
      !_mesa_validate_DrawElementsInstanced(ctx, mode, count, type,
               _mesa_validated_drawrangeelements(ctx, ctx->Array.VAO->IndexBufferObj,
            }
         /**
   * Called by glDrawElementsInstancedBaseVertex() in immediate mode.
   */
   void GLAPIENTRY
   _mesa_DrawElementsInstancedBaseVertex(GLenum mode, GLsizei count,
                     {
      GET_CURRENT_CONTEXT(ctx);
            _mesa_set_varying_vp_inputs(ctx, ctx->VertexProgram._VPModeInputFilter &
         if (ctx->NewState)
            if (!_mesa_is_no_error_enabled(ctx) &&
      !_mesa_validate_DrawElementsInstanced(ctx, mode, count, type,
               _mesa_validated_drawrangeelements(ctx, ctx->Array.VAO->IndexBufferObj,
                  }
         /**
   * Called by glDrawElementsInstancedBaseInstance() in immediate mode.
   */
   void GLAPIENTRY
   _mesa_DrawElementsInstancedBaseInstance(GLenum mode, GLsizei count,
                           {
      GET_CURRENT_CONTEXT(ctx);
            _mesa_set_varying_vp_inputs(ctx, ctx->VertexProgram._VPModeInputFilter &
         if (ctx->NewState)
            if (!_mesa_is_no_error_enabled(ctx) &&
      !_mesa_validate_DrawElementsInstanced(ctx, mode, count, type,
               _mesa_validated_drawrangeelements(ctx, ctx->Array.VAO->IndexBufferObj,
                  }
         /**
   * Called by glDrawElementsInstancedBaseVertexBaseInstance() in immediate mode.
   */
   void GLAPIENTRY
   _mesa_DrawElementsInstancedBaseVertexBaseInstance(GLenum mode,
                                       {
      GET_CURRENT_CONTEXT(ctx);
            _mesa_set_varying_vp_inputs(ctx, ctx->VertexProgram._VPModeInputFilter &
         if (ctx->NewState)
            if (!_mesa_is_no_error_enabled(ctx) &&
      !_mesa_validate_DrawElementsInstanced(ctx, mode, count, type,
               _mesa_validated_drawrangeelements(ctx, ctx->Array.VAO->IndexBufferObj,
                  }
      /**
   * Same as glDrawElementsInstancedBaseVertexBaseInstance, but the index
   * buffer is set by the indexBuf parameter instead of using the bound
   * GL_ELEMENT_ARRAY_BUFFER if indexBuf != NULL.
   */
   void GLAPIENTRY
   _mesa_DrawElementsUserBuf(GLintptr indexBuf, GLenum mode,
                     {
      GET_CURRENT_CONTEXT(ctx);
            _mesa_set_varying_vp_inputs(ctx, ctx->VertexProgram._VPModeInputFilter &
         if (ctx->NewState)
            if (!_mesa_is_no_error_enabled(ctx) &&
      !_mesa_validate_DrawElementsInstanced(ctx, mode, count, type,
               struct gl_buffer_object *index_bo =
      indexBuf ? (struct gl_buffer_object*)indexBuf :
         _mesa_validated_drawrangeelements(ctx, index_bo,
                  }
         /**
   * Inner support for both _mesa_MultiDrawElements() and
   * _mesa_MultiDrawRangeElements().
   * This does the actual rendering after we've checked array indexes, etc.
   */
   static void
   _mesa_validated_multidrawelements(struct gl_context *ctx,
                           {
      uintptr_t min_index_ptr, max_index_ptr;
   bool fallback = false;
            if (primcount == 0)
                     min_index_ptr = (uintptr_t) indices[0];
   max_index_ptr = 0;
   for (i = 0; i < primcount; i++) {
      if (count[i]) {
      min_index_ptr = MIN2(min_index_ptr, (uintptr_t) indices[i]);
   max_index_ptr = MAX2(max_index_ptr, (uintptr_t) indices[i] +
                  /* Check if we can handle this thing as a bunch of index offsets from the
   * same index pointer.  If we can't, then we have to fall back to doing
   * a draw_prims per primitive.
   * Check that the difference between each prim's indexes is a multiple of
   * the index/element size.
   */
   if (index_size_shift) {
      for (i = 0; i < primcount; i++) {
      if (count[i] &&
      (((uintptr_t)indices[i] - min_index_ptr) &
   ((1 << index_size_shift) - 1)) != 0) {
   fallback = true;
                              info.mode = mode;
   info.index_size = 1 << index_size_shift;
   /* Packed section begin. */
   info.primitive_restart = ctx->Array._PrimitiveRestart[index_size_shift];
   info.has_user_indices = index_bo == NULL;
   info.index_bounds_valid = false;
   info.increment_draw_id = primcount > 1;
   info.was_line_loop = false;
   info.take_index_buffer_ownership = false;
   info.index_bias_varies = !!basevertex;
   /* Packed section end. */
   info.start_instance = 0;
   info.instance_count = 1;
   info.view_mask = 0;
            if (info.has_user_indices) {
         } else {
      if (ctx->st->pipe->draw_vbo == tc_draw_vbo) {
      /* Fast path for u_threaded_context to eliminate atomics. */
   info.index.resource = _mesa_get_bufferobj_reference(ctx, index_bo);
      } else {
                  /* No index buffer storage allocated - nothing to do. */
   if (!info.index.resource)
               if (!fallback &&
      (!info.has_user_indices ||
   /* "max_index_ptr - min_index_ptr >> index_size_shift" is stored
      * in draw[i].start. The driver will multiply it later by index_size
   * so make sure the final value won't overflow.
   *
   * For real index buffers, gallium doesn't support index buffer offsets
   * greater than UINT32_MAX bytes.
      max_index_ptr - min_index_ptr <= UINT32_MAX)) {
   struct pipe_draw_start_count_bias *draw = get_temp_draws(ctx, primcount);
   if (!draw)
            if (info.has_user_indices) {
      for (int i = 0; i < primcount; i++) {
      draw[i].start =
         draw[i].count = count[i];
         } else {
      for (int i = 0; i < primcount; i++) {
      draw[i].start = (uintptr_t)indices[i] >> index_size_shift;
   draw[i].count =
                           } else {
      /* draw[i].start would overflow. Draw one at a time. */
   assert(info.has_user_indices);
            for (int i = 0; i < primcount; i++) {
                              /* Reset these, because the callee can change them. */
   info.index_bounds_valid = false;
   info.index.user = indices[i];
   draw.start = 0;
                                 if (MESA_DEBUG_FLAGS & DEBUG_ALWAYS_FLUSH) {
            }
         void GLAPIENTRY
   _mesa_MultiDrawElements(GLenum mode, const GLsizei *count, GLenum type,
         {
      GET_CURRENT_CONTEXT(ctx);
            _mesa_set_varying_vp_inputs(ctx, ctx->VertexProgram._VPModeInputFilter &
         if (ctx->NewState)
                     if (!_mesa_is_no_error_enabled(ctx) &&
      !_mesa_validate_MultiDrawElements(ctx, mode, count, type, indices,
               _mesa_validated_multidrawelements(ctx, index_bo, mode, count, type,
      }
         void GLAPIENTRY
   _mesa_MultiDrawElementsBaseVertex(GLenum mode,
                           {
      GET_CURRENT_CONTEXT(ctx);
            _mesa_set_varying_vp_inputs(ctx, ctx->VertexProgram._VPModeInputFilter &
         if (ctx->NewState)
                     if (!_mesa_is_no_error_enabled(ctx) &&
      !_mesa_validate_MultiDrawElements(ctx, mode, count, type, indices,
               _mesa_validated_multidrawelements(ctx, index_bo, mode, count, type,
      }
         /**
   * Same as glMultiDrawElementsBaseVertex, but the index buffer is set by
   * the indexBuf parameter instead of using the bound GL_ELEMENT_ARRAY_BUFFER
   * if indexBuf != NULL.
   */
   void GLAPIENTRY
   _mesa_MultiDrawElementsUserBuf(GLintptr indexBuf, GLenum mode,
                     {
      GET_CURRENT_CONTEXT(ctx);
            _mesa_set_varying_vp_inputs(ctx, ctx->VertexProgram._VPModeInputFilter &
         if (ctx->NewState)
            struct gl_buffer_object *index_bo =
      indexBuf ? (struct gl_buffer_object*)indexBuf :
         if (!_mesa_is_no_error_enabled(ctx) &&
      !_mesa_validate_MultiDrawElements(ctx, mode, count, type, indices,
               _mesa_validated_multidrawelements(ctx, index_bo, mode, count, type,
      }
         /**
   * Draw a GL primitive using a vertex count obtained from transform feedback.
   * \param mode  the type of GL primitive to draw
   * \param obj  the transform feedback object to use
   * \param stream  index of the transform feedback stream from which to
   *                get the primitive count.
   * \param numInstances  number of instances to draw
   */
   static void
   _mesa_draw_transform_feedback(struct gl_context *ctx, GLenum mode,
               {
               _mesa_set_varying_vp_inputs(ctx, ctx->VertexProgram._VPModeInputFilter &
         if (ctx->NewState)
            if (!_mesa_is_no_error_enabled(ctx) &&
      !_mesa_validate_DrawTransformFeedback(ctx, mode, obj, stream,
               /* Maybe we should do some primitive splitting for primitive restart
   * (like in DrawArrays), but we have no way to know how many vertices
                     if (MESA_DEBUG_FLAGS & DEBUG_ALWAYS_FLUSH) {
            }
         /**
   * Like DrawArrays, but take the count from a transform feedback object.
   * \param mode  GL_POINTS, GL_LINES, GL_TRIANGLE_STRIP, etc.
   * \param name  the transform feedback object
   * User still has to setup of the vertex attribute info with
   * glVertexPointer, glColorPointer, etc.
   * Part of GL_ARB_transform_feedback2.
   */
   void GLAPIENTRY
   _mesa_DrawTransformFeedback(GLenum mode, GLuint name)
   {
      GET_CURRENT_CONTEXT(ctx);
   struct gl_transform_feedback_object *obj =
               }
         void GLAPIENTRY
   _mesa_DrawTransformFeedbackStream(GLenum mode, GLuint name, GLuint stream)
   {
      GET_CURRENT_CONTEXT(ctx);
   struct gl_transform_feedback_object *obj =
               }
         void GLAPIENTRY
   _mesa_DrawTransformFeedbackInstanced(GLenum mode, GLuint name,
         {
      GET_CURRENT_CONTEXT(ctx);
   struct gl_transform_feedback_object *obj =
               }
         void GLAPIENTRY
   _mesa_DrawTransformFeedbackStreamInstanced(GLenum mode, GLuint name,
               {
      GET_CURRENT_CONTEXT(ctx);
   struct gl_transform_feedback_object *obj =
               }
         /**
   * Like [Multi]DrawArrays/Elements, but they take most arguments from
   * a buffer object.
   */
   void GLAPIENTRY
   _mesa_DrawArraysIndirect(GLenum mode, const GLvoid *indirect)
   {
               /* From the ARB_draw_indirect spec:
   *
   *    "Initially zero is bound to DRAW_INDIRECT_BUFFER. In the
   *    compatibility profile, this indicates that DrawArraysIndirect and
   *    DrawElementsIndirect are to source their arguments directly from the
   *    pointer passed as their <indirect> parameters."
   */
   if (_mesa_is_desktop_gl_compat(ctx) &&
      !ctx->DrawIndirectBuffer) {
            _mesa_DrawArraysInstancedBaseInstance(mode, cmd->first, cmd->count,
                                    _mesa_set_varying_vp_inputs(ctx, ctx->VertexProgram._VPModeInputFilter &
         if (ctx->NewState)
            if (!_mesa_is_no_error_enabled(ctx) &&
      !_mesa_validate_DrawArraysIndirect(ctx, mode, indirect))
            }
         void GLAPIENTRY
   _mesa_DrawElementsIndirect(GLenum mode, GLenum type, const GLvoid *indirect)
   {
               /* From the ARB_draw_indirect spec:
   *
   *    "Initially zero is bound to DRAW_INDIRECT_BUFFER. In the
   *    compatibility profile, this indicates that DrawArraysIndirect and
   *    DrawElementsIndirect are to source their arguments directly from the
   *    pointer passed as their <indirect> parameters."
   */
   if (_mesa_is_desktop_gl_compat(ctx) &&
      !ctx->DrawIndirectBuffer) {
   /*
   * Unlike regular DrawElementsInstancedBaseVertex commands, the indices
   * may not come from a client array and must come from an index buffer.
   * If no element array buffer is bound, an INVALID_OPERATION error is
   * generated.
   */
   if (!ctx->Array.VAO->IndexBufferObj) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
            } else {
                     /* Convert offset to pointer */
                  _mesa_DrawElementsInstancedBaseVertexBaseInstance(mode, cmd->count,
                                                      _mesa_set_varying_vp_inputs(ctx, ctx->VertexProgram._VPModeInputFilter &
         if (ctx->NewState)
            if (!_mesa_is_no_error_enabled(ctx) &&
      !_mesa_validate_DrawElementsIndirect(ctx, mode, type, indirect))
            }
         void GLAPIENTRY
   _mesa_MultiDrawArraysIndirect(GLenum mode, const GLvoid *indirect,
         {
               /* If <stride> is zero, the array elements are treated as tightly packed. */
   if (stride == 0)
                     _mesa_set_varying_vp_inputs(ctx, ctx->VertexProgram._VPModeInputFilter &
         if (ctx->NewState)
            /* From the ARB_draw_indirect spec:
   *
   *    "Initially zero is bound to DRAW_INDIRECT_BUFFER. In the
   *    compatibility profile, this indicates that DrawArraysIndirect and
   *    DrawElementsIndirect are to source their arguments directly from the
   *    pointer passed as their <indirect> parameters."
   */
   if (_mesa_is_desktop_gl_compat(ctx) &&
               if (!_mesa_is_no_error_enabled(ctx) &&
      (!_mesa_valid_draw_indirect_multi(ctx, primcount, stride,
                     struct pipe_draw_info info;
   info.mode = mode;
   info.index_size = 0;
   info.view_mask = 0;
   /* Packed section begin. */
   info.primitive_restart = false;
   info.has_user_indices = false;
   info.index_bounds_valid = false;
   info.increment_draw_id = primcount > 1;
   info.was_line_loop = false;
   info.take_index_buffer_ownership = false;
   info.index_bias_varies = false;
            const uint8_t *ptr = (const uint8_t *) indirect;
   for (unsigned i = 0; i < primcount; i++) {
                              struct pipe_draw_start_count_bias draw;
                  ctx->Driver.DrawGallium(ctx, &info, i, &draw, 1);
                           if (!_mesa_is_no_error_enabled(ctx) &&
      !_mesa_validate_MultiDrawArraysIndirect(ctx, mode, indirect,
                  }
         void GLAPIENTRY
   _mesa_MultiDrawElementsIndirect(GLenum mode, GLenum type,
               {
                        _mesa_set_varying_vp_inputs(ctx, ctx->VertexProgram._VPModeInputFilter &
         if (ctx->NewState)
            /* If <stride> is zero, the array elements are treated as tightly packed. */
   if (stride == 0)
            /* From the ARB_draw_indirect spec:
   *
   *    "Initially zero is bound to DRAW_INDIRECT_BUFFER. In the
   *    compatibility profile, this indicates that DrawArraysIndirect and
   *    DrawElementsIndirect are to source their arguments directly from the
   *    pointer passed as their <indirect> parameters."
   */
   if (_mesa_is_desktop_gl_compat(ctx) &&
      !ctx->DrawIndirectBuffer) {
   /*
   * Unlike regular DrawElementsInstancedBaseVertex commands, the indices
   * may not come from a client array and must come from an index buffer.
   * If no element array buffer is bound, an INVALID_OPERATION error is
   * generated.
   */
   if (!ctx->Array.VAO->IndexBufferObj) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                              if (!_mesa_is_no_error_enabled(ctx) &&
      (!_mesa_valid_draw_indirect_multi(ctx, primcount, stride,
                              struct pipe_draw_info info;
   info.mode = mode;
   info.index_size = 1 << index_size_shift;
   info.view_mask = 0;
   /* Packed section begin. */
   info.primitive_restart = ctx->Array._PrimitiveRestart[index_size_shift];
   info.has_user_indices = false;
   info.index_bounds_valid = false;
   info.increment_draw_id = primcount > 1;
   info.was_line_loop = false;
   info.take_index_buffer_ownership = false;
   info.index_bias_varies = false;
   /* Packed section end. */
                     if (ctx->st->pipe->draw_vbo == tc_draw_vbo) {
      /* Fast path for u_threaded_context to eliminate atomics. */
   info.index.resource = _mesa_get_bufferobj_reference(ctx, index_bo);
   info.take_index_buffer_ownership = true;
   /* Increase refcount so be able to use take_index_buffer_ownership with
   * multiple draws.
   */
   if (primcount > 1 && info.index.resource)
      } else {
                  /* No index buffer storage allocated - nothing to do. */
   if (!info.index.resource)
            const uint8_t *ptr = (const uint8_t *) indirect;
   for (unsigned i = 0; i < primcount; i++) {
                              struct pipe_draw_start_count_bias draw;
   draw.start = cmd->firstIndex;
                  ctx->Driver.DrawGallium(ctx, &info, i, &draw, 1);
                           if (!_mesa_is_no_error_enabled(ctx) &&
      !_mesa_validate_MultiDrawElementsIndirect(ctx, mode, type, indirect,
                  }
         void GLAPIENTRY
   _mesa_MultiDrawArraysIndirectCountARB(GLenum mode, GLintptr indirect,
               {
      GET_CURRENT_CONTEXT(ctx);
            /* If <stride> is zero, the array elements are treated as tightly packed. */
   if (stride == 0)
            _mesa_set_varying_vp_inputs(ctx, ctx->VertexProgram._VPModeInputFilter &
         if (ctx->NewState)
            if (!_mesa_is_no_error_enabled(ctx) &&
      !_mesa_validate_MultiDrawArraysIndirectCount(ctx, mode, indirect,
                     st_indirect_draw_vbo(ctx, mode, 0, (GLintptr)indirect, drawcount_offset,
      }
         void GLAPIENTRY
   _mesa_MultiDrawElementsIndirectCountARB(GLenum mode, GLenum type,
                     {
      GET_CURRENT_CONTEXT(ctx);
            /* If <stride> is zero, the array elements are treated as tightly packed. */
   if (stride == 0)
            _mesa_set_varying_vp_inputs(ctx, ctx->VertexProgram._VPModeInputFilter &
         if (ctx->NewState)
            if (!_mesa_is_no_error_enabled(ctx) &&
      !_mesa_validate_MultiDrawElementsIndirectCount(ctx, mode, type,
                           st_indirect_draw_vbo(ctx, mode, type, (GLintptr)indirect, drawcount_offset,
      }
         /* GL_IBM_multimode_draw_arrays */
   void GLAPIENTRY
   _mesa_MultiModeDrawArraysIBM( const GLenum * mode, const GLint * first,
               {
      GET_CURRENT_CONTEXT(ctx);
            for ( i = 0 ; i < primcount ; i++ ) {
      if ( count[i] > 0 ) {
      GLenum m = *((GLenum *) ((GLubyte *) mode + i * modestride));
            }
         /* GL_IBM_multimode_draw_arrays */
   void GLAPIENTRY
   _mesa_MultiModeDrawElementsIBM( const GLenum * mode, const GLsizei * count,
               {
      GET_CURRENT_CONTEXT(ctx);
            for ( i = 0 ; i < primcount ; i++ ) {
      if ( count[i] > 0 ) {
      GLenum m = *((GLenum *) ((GLubyte *) mode + i * modestride));
   CALL_DrawElements(ctx->Dispatch.Current, ( m, count[i], type,
            }
