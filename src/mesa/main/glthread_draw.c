   /*
   * Copyright © 2020 Advanced Micro Devices, Inc.
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
      /* Draw function marshalling for glthread.
   *
   * The purpose of these glDraw wrappers is to upload non-VBO vertex and
   * index data, so that glthread doesn't have to execute synchronously.
   */
      #include "c99_alloca.h"
      #include "api_exec_decl.h"
   #include "main/glthread_marshal.h"
   #include "main/dispatch.h"
   #include "main/varray.h"
      static inline unsigned
   get_index_size(GLenum type)
   {
      /* GL_UNSIGNED_BYTE  - GL_UNSIGNED_BYTE = 0
   * GL_UNSIGNED_SHORT - GL_UNSIGNED_BYTE = 2
   * GL_UNSIGNED_INT   - GL_UNSIGNED_BYTE = 4
   *
   * Divide by 2 to get n=0,1,2, then the index size is: 1 << n
   */
      }
      static inline bool
   is_index_type_valid(GLenum type)
   {
      /* GL_UNSIGNED_BYTE  = 0x1401
   * GL_UNSIGNED_SHORT = 0x1403
   * GL_UNSIGNED_INT   = 0x1405
   *
   * The trick is that bit 1 and bit 2 mean USHORT and UINT, respectively.
   * After clearing those two bits (with ~6), we should get UBYTE.
   * Both bits can't be set, because the enum would be greater than UINT.
   */
      }
      static ALWAYS_INLINE struct gl_buffer_object *
   upload_indices(struct gl_context *ctx, unsigned count, unsigned index_size,
         {
      struct gl_buffer_object *upload_buffer = NULL;
                     _mesa_glthread_upload(ctx, *indices, index_size * count,
                  if (!upload_buffer)
               }
      static ALWAYS_INLINE struct gl_buffer_object *
   upload_multi_indices(struct gl_context *ctx, unsigned total_count,
                     {
      struct gl_buffer_object *upload_buffer = NULL;
   unsigned upload_offset = 0;
                     _mesa_glthread_upload(ctx, NULL, index_size * total_count,
         if (!upload_buffer) {
      _mesa_marshal_InternalSetError(GL_OUT_OF_MEMORY);
               for (unsigned i = 0, offset = 0; i < draw_count; i++) {
      if (!count[i]) {
      /* Set some valid value so as not to leave it uninitialized. */
   out_indices[i] = (const GLvoid*)(intptr_t)upload_offset;
                        memcpy(upload_ptr + offset, indices[i], size);
   out_indices[i] = (const GLvoid*)(intptr_t)(upload_offset + offset);
                  }
      static ALWAYS_INLINE bool
   upload_vertices(struct gl_context *ctx, unsigned user_buffer_mask,
                     {
      struct glthread_vao *vao = ctx->GLThread.CurrentVAO;
   unsigned attrib_mask_iter = vao->Enabled;
            assert((num_vertices || !(user_buffer_mask & ~vao->NonZeroDivisorMask)) &&
            if (unlikely(vao->BufferInterleaved & user_buffer_mask)) {
      /* Slower upload path where some buffers reference multiple attribs,
   * so we have to use 2 while loops instead of 1.
   */
   unsigned start_offset[VERT_ATTRIB_MAX];
   unsigned end_offset[VERT_ATTRIB_MAX];
            while (attrib_mask_iter) {
                                    unsigned stride = vao->Attrib[binding_index].Stride;
   unsigned instance_div = vao->Attrib[binding_index].Divisor;
   unsigned element_size = vao->Attrib[i].ElementSize;
                                    /* Figure out how many instances we'll render given instance_div.  We
   * can't use the typical div_round_up() pattern because the CTS uses
   * instance_div = ~0 for a test, which overflows div_round_up()'s
   * addition.
   */
   unsigned count = num_instances / instance_div;
                  offset += stride * start_instance;
      } else {
      /* Per-vertex attrib. */
   offset += stride * start_vertex;
                        /* Update upload offsets. */
   if (!(buffer_mask & binding_index_bit)) {
      start_offset[binding_index] = offset;
      } else {
      if (offset < start_offset[binding_index])
         if (offset + size > end_offset[binding_index])
                           /* Upload buffers. */
   while (buffer_mask) {
      struct gl_buffer_object *upload_buffer = NULL;
                           start = start_offset[binding_index];
                  /* If the draw start index is non-zero, glthread can upload to offset 0,
   * which means the attrib offset has to be -(first * stride).
   * So use signed vertex buffer offsets when possible to save memory.
   */
   const void *ptr = vao->Attrib[binding_index].Pointer;
   _mesa_glthread_upload(ctx, (uint8_t*)ptr + start,
               if (!upload_buffer) {
                     _mesa_marshal_InternalSetError(GL_OUT_OF_MEMORY);
               buffers[num_buffers].buffer = upload_buffer;
   buffers[num_buffers].offset = upload_offset - start;
                           /* Faster path where all attribs are separate. */
   while (attrib_mask_iter) {
      unsigned i = u_bit_scan(&attrib_mask_iter);
            if (!(user_buffer_mask & (1 << binding_index)))
            struct gl_buffer_object *upload_buffer = NULL;
   unsigned upload_offset = 0;
   unsigned stride = vao->Attrib[binding_index].Stride;
   unsigned instance_div = vao->Attrib[binding_index].Divisor;
   unsigned element_size = vao->Attrib[i].ElementSize;
   unsigned offset = vao->Attrib[i].RelativeOffset;
            if (instance_div) {
               /* Figure out how many instances we'll render given instance_div.  We
   * can't use the typical div_round_up() pattern because the CTS uses
   * instance_div = ~0 for a test, which overflows div_round_up()'s
   * addition.
   */
   unsigned count = num_instances / instance_div;
                  offset += stride * start_instance;
      } else {
      /* Per-vertex attrib. */
   offset += stride * start_vertex;
               /* If the draw start index is non-zero, glthread can upload to offset 0,
   * which means the attrib offset has to be -(first * stride).
   * So use signed vertex buffer offsets when possible to save memory.
   */
   const void *ptr = vao->Attrib[binding_index].Pointer;
   _mesa_glthread_upload(ctx, (uint8_t*)ptr + offset,
               if (!upload_buffer) {
                     _mesa_marshal_InternalSetError(GL_OUT_OF_MEMORY);
               buffers[num_buffers].buffer = upload_buffer;
   buffers[num_buffers].offset = upload_offset - offset;
                  }
      /* DrawArrays without user buffers. */
   struct marshal_cmd_DrawArrays
   {
      struct marshal_cmd_base cmd_base;
   GLenum mode;
   GLint first;
      };
      uint32_t
   _mesa_unmarshal_DrawArrays(struct gl_context *ctx,
         {
      const GLenum mode = cmd->mode;
   const GLint first = cmd->first;
            CALL_DrawArrays(ctx->Dispatch.Current, (mode, first, count));
   const unsigned cmd_size = align(sizeof(*cmd), 8) / 8;
   assert(cmd_size == cmd->cmd_base.cmd_size);
      }
      /* DrawArraysInstancedBaseInstance without user buffers. */
   struct marshal_cmd_DrawArraysInstancedBaseInstance
   {
      struct marshal_cmd_base cmd_base;
   GLenum mode;
   GLint first;
   GLsizei count;
   GLsizei instance_count;
      };
      uint32_t
   _mesa_unmarshal_DrawArraysInstancedBaseInstance(struct gl_context *ctx,
         {
      const GLenum mode = cmd->mode;
   const GLint first = cmd->first;
   const GLsizei count = cmd->count;
   const GLsizei instance_count = cmd->instance_count;
            CALL_DrawArraysInstancedBaseInstance(ctx->Dispatch.Current,
               const unsigned cmd_size = align(sizeof(*cmd), 8) / 8;
   assert(cmd_size == cmd->cmd_base.cmd_size);
      }
      struct marshal_cmd_DrawArraysInstancedBaseInstanceDrawID
   {
      struct marshal_cmd_base cmd_base;
   GLenum mode;
   GLint first;
   GLsizei count;
   GLsizei instance_count;
   GLuint baseinstance;
      };
      uint32_t
   _mesa_unmarshal_DrawArraysInstancedBaseInstanceDrawID(struct gl_context *ctx,
         {
      const GLenum mode = cmd->mode;
   const GLint first = cmd->first;
   const GLsizei count = cmd->count;
   const GLsizei instance_count = cmd->instance_count;
            ctx->DrawID = cmd->drawid;
   CALL_DrawArraysInstancedBaseInstance(ctx->Dispatch.Current,
                        const unsigned cmd_size = align(sizeof(*cmd), 8) / 8;
   assert(cmd_size == cmd->cmd_base.cmd_size);
      }
      /* DrawArraysInstancedBaseInstance with user buffers. */
   struct marshal_cmd_DrawArraysUserBuf
   {
      struct marshal_cmd_base cmd_base;
   GLenum mode;
   GLint first;
   GLsizei count;
   GLsizei instance_count;
   GLuint baseinstance;
   GLuint drawid;
      };
      uint32_t
   _mesa_unmarshal_DrawArraysUserBuf(struct gl_context *ctx,
         {
      const GLuint user_buffer_mask = cmd->user_buffer_mask;
   const struct glthread_attrib_binding *buffers =
            /* Bind uploaded buffers if needed. */
   if (user_buffer_mask)
            const GLenum mode = cmd->mode;
   const GLint first = cmd->first;
   const GLsizei count = cmd->count;
   const GLsizei instance_count = cmd->instance_count;
            ctx->DrawID = cmd->drawid;
   CALL_DrawArraysInstancedBaseInstance(ctx->Dispatch.Current,
               ctx->DrawID = 0;
      }
      static inline unsigned
   get_user_buffer_mask(struct gl_context *ctx)
   {
               /* BufferEnabled means which attribs are enabled in terms of buffer
   * binding slots (not attrib slots).
   *
   * UserPointerMask means which buffer bindings don't have a buffer bound.
   *
   * NonNullPointerMask means which buffer bindings have a NULL pointer.
   * Those are not uploaded. This can happen when an attrib is enabled, but
   * the shader doesn't use it, so it's ignored by mesa/state_tracker.
   */
      }
      static ALWAYS_INLINE void
   draw_arrays(GLuint drawid, GLenum mode, GLint first, GLsizei count,
               {
               if (unlikely(compiled_into_dlist && ctx->GLThread.ListMode)) {
      _mesa_glthread_finish_before(ctx, "DrawArrays");
   /* Use the function that's compiled into a display list. */
   CALL_DrawArrays(ctx->Dispatch.Current, (mode, first, count));
               unsigned user_buffer_mask =
            /* Fast path when nothing needs to be done.
   *
   * This is also an error path. Zero counts should still call the driver
   * for possible GL errors.
   */
   if (!user_buffer_mask || count <= 0 || instance_count <= 0 ||
      /* This will just generate GL_INVALID_OPERATION, as it should. */
   ctx->GLThread.inside_begin_end ||
   ctx->Dispatch.Current == ctx->Dispatch.ContextLost ||
   ctx->GLThread.ListMode) {
   if (instance_count == 1 && baseinstance == 0 && drawid == 0) {
      int cmd_size = sizeof(struct marshal_cmd_DrawArrays);
                  cmd->mode = mode;
   cmd->first = first;
      } else if (drawid == 0) {
      int cmd_size = sizeof(struct marshal_cmd_DrawArraysInstancedBaseInstance);
                  cmd->mode = mode;
   cmd->first = first;
   cmd->count = count;
   cmd->instance_count = instance_count;
      } else {
      int cmd_size = sizeof(struct marshal_cmd_DrawArraysInstancedBaseInstanceDrawID);
                  cmd->mode = mode;
   cmd->first = first;
   cmd->count = count;
   cmd->instance_count = instance_count;
   cmd->baseinstance = baseinstance;
      }
               /* Upload and draw. */
            if (!upload_vertices(ctx, user_buffer_mask, first, count, baseinstance,
                  int buffers_size = util_bitcount(user_buffer_mask) * sizeof(buffers[0]);
   int cmd_size = sizeof(struct marshal_cmd_DrawArraysUserBuf) +
                  cmd = _mesa_glthread_allocate_command(ctx, DISPATCH_CMD_DrawArraysUserBuf,
         cmd->mode = mode;
   cmd->first = first;
   cmd->count = count;
   cmd->instance_count = instance_count;
   cmd->baseinstance = baseinstance;
   cmd->drawid = drawid;
            if (user_buffer_mask)
      }
      /* MultiDrawArrays with user buffers. */
   struct marshal_cmd_MultiDrawArraysUserBuf
   {
      struct marshal_cmd_base cmd_base;
   GLenum mode;
   GLsizei draw_count;
      };
      uint32_t
   _mesa_unmarshal_MultiDrawArraysUserBuf(struct gl_context *ctx,
         {
      const GLenum mode = cmd->mode;
   const GLsizei draw_count = cmd->draw_count;
            const char *variable_data = (const char *)(cmd + 1);
   const GLint *first = (GLint *)variable_data;
   variable_data += sizeof(GLint) * draw_count;
   const GLsizei *count = (GLsizei *)variable_data;
   variable_data += sizeof(GLsizei) * draw_count;
   const struct glthread_attrib_binding *buffers =
            /* Bind uploaded buffers if needed. */
   if (user_buffer_mask)
            CALL_MultiDrawArrays(ctx->Dispatch.Current,
            }
      void GLAPIENTRY
   _mesa_marshal_MultiDrawArrays(GLenum mode, const GLint *first,
         {
               if (unlikely(ctx->GLThread.ListMode)) {
      _mesa_glthread_finish_before(ctx, "MultiDrawArrays");
   CALL_MultiDrawArrays(ctx->Dispatch.Current,
                     struct glthread_attrib_binding buffers[VERT_ATTRIB_MAX];
   unsigned user_buffer_mask =
      _mesa_is_desktop_gl_core(ctx) || draw_count <= 0 ||
   ctx->Dispatch.Current == ctx->Dispatch.ContextLost ||
         if (user_buffer_mask) {
      unsigned min_index = ~0;
            for (int i = 0; i < draw_count; i++) {
               if (vertex_count < 0) {
      /* This will just call the driver to set the GL error. */
   min_index = ~0;
      }
                  min_index = MIN2(min_index, first[i]);
               if (min_index >= max_index_exclusive) {
      /* Nothing to do, but call the driver to set possible GL errors. */
      } else {
                     if (!upload_vertices(ctx, user_buffer_mask, min_index, num_vertices,
                        /* Add the call into the batch buffer. */
   int real_draw_count = MAX2(draw_count, 0);
   int first_size = sizeof(GLint) * real_draw_count;
   int count_size = sizeof(GLsizei) * real_draw_count;
   int buffers_size = util_bitcount(user_buffer_mask) * sizeof(buffers[0]);
   int cmd_size = sizeof(struct marshal_cmd_MultiDrawArraysUserBuf) +
                  /* Make sure cmd can fit in the batch buffer */
   if (cmd_size <= MARSHAL_MAX_CMD_SIZE) {
      cmd = _mesa_glthread_allocate_command(ctx, DISPATCH_CMD_MultiDrawArraysUserBuf,
         cmd->mode = mode;
   cmd->draw_count = draw_count;
            char *variable_data = (char*)(cmd + 1);
   memcpy(variable_data, first, first_size);
   variable_data += first_size;
            if (user_buffer_mask) {
      variable_data += count_size;
         } else {
      /* The call is too large, so sync and execute the unmarshal code here. */
            if (user_buffer_mask)
            CALL_MultiDrawArrays(ctx->Dispatch.Current,
         }
      /* DrawElementsInstanced without user buffers. */
   struct marshal_cmd_DrawElementsInstanced
   {
      struct marshal_cmd_base cmd_base;
   GLenum16 mode;
   GLenum16 type;
   GLsizei count;
   GLsizei instance_count;
      };
      uint32_t
   _mesa_unmarshal_DrawElementsInstanced(struct gl_context *ctx,
         {
      const GLenum mode = cmd->mode;
   const GLsizei count = cmd->count;
   const GLenum type = cmd->type;
   const GLvoid *indices = cmd->indices;
            CALL_DrawElementsInstanced(ctx->Dispatch.Current,
         const unsigned cmd_size = align(sizeof(*cmd), 8) / 8;
   assert(cmd_size == cmd->cmd_base.cmd_size);
      }
      /* DrawElementsBaseVertex without user buffers. */
   struct marshal_cmd_DrawElementsBaseVertex
   {
      struct marshal_cmd_base cmd_base;
   GLenum16 mode;
   GLenum16 type;
   GLsizei count;
   GLint basevertex;
      };
      uint32_t
   _mesa_unmarshal_DrawElementsBaseVertex(struct gl_context *ctx,
         {
      const GLenum mode = cmd->mode;
   const GLsizei count = cmd->count;
   const GLenum type = cmd->type;
   const GLvoid *indices = cmd->indices;
            CALL_DrawElementsBaseVertex(ctx->Dispatch.Current,
         const unsigned cmd_size = align(sizeof(*cmd), 8) / 8;
   assert(cmd_size == cmd->cmd_base.cmd_size);
      }
      /* DrawElementsInstancedBaseVertexBaseInstance without user buffers. */
   struct marshal_cmd_DrawElementsInstancedBaseVertexBaseInstance
   {
      struct marshal_cmd_base cmd_base;
   GLenum16 mode;
   GLenum16 type;
   GLsizei count;
   GLsizei instance_count;
   GLint basevertex;
   GLuint baseinstance;
      };
      uint32_t
   _mesa_unmarshal_DrawElementsInstancedBaseVertexBaseInstance(struct gl_context *ctx,
         {
      const GLenum mode = cmd->mode;
   const GLsizei count = cmd->count;
   const GLenum type = cmd->type;
   const GLvoid *indices = cmd->indices;
   const GLsizei instance_count = cmd->instance_count;
   const GLint basevertex = cmd->basevertex;
            CALL_DrawElementsInstancedBaseVertexBaseInstance(ctx->Dispatch.Current,
                     const unsigned cmd_size = align(sizeof(*cmd), 8) / 8;
   assert(cmd_size == cmd->cmd_base.cmd_size);
      }
      struct marshal_cmd_DrawElementsInstancedBaseVertexBaseInstanceDrawID
   {
      struct marshal_cmd_base cmd_base;
   GLenum16 mode;
   GLenum16 type;
   GLsizei count;
   GLsizei instance_count;
   GLint basevertex;
   GLuint baseinstance;
   GLuint drawid;
      };
      uint32_t
   _mesa_unmarshal_DrawElementsInstancedBaseVertexBaseInstanceDrawID(struct gl_context *ctx,
         {
      const GLenum mode = cmd->mode;
   const GLsizei count = cmd->count;
   const GLenum type = cmd->type;
   const GLvoid *indices = cmd->indices;
   const GLsizei instance_count = cmd->instance_count;
   const GLint basevertex = cmd->basevertex;
            ctx->DrawID = cmd->drawid;
   CALL_DrawElementsInstancedBaseVertexBaseInstance(ctx->Dispatch.Current,
                              const unsigned cmd_size = align(sizeof(*cmd), 8) / 8;
   assert(cmd_size == cmd->cmd_base.cmd_size);
      }
      struct marshal_cmd_DrawElementsUserBuf
   {
      struct marshal_cmd_base cmd_base;
   GLenum16 mode;
   GLenum16 type;
   GLsizei count;
   GLsizei instance_count;
   GLint basevertex;
   GLuint baseinstance;
   GLuint drawid;
   GLuint user_buffer_mask;
   const GLvoid *indices;
      };
      uint32_t
   _mesa_unmarshal_DrawElementsUserBuf(struct gl_context *ctx,
         {
      const GLuint user_buffer_mask = cmd->user_buffer_mask;
   const struct glthread_attrib_binding *buffers =
            /* Bind uploaded buffers if needed. */
   if (user_buffer_mask)
            /* Draw. */
   const GLenum mode = cmd->mode;
   const GLsizei count = cmd->count;
   const GLenum type = cmd->type;
   const GLvoid *indices = cmd->indices;
   const GLsizei instance_count = cmd->instance_count;
   const GLint basevertex = cmd->basevertex;
   const GLuint baseinstance = cmd->baseinstance;
            ctx->DrawID = cmd->drawid;
   CALL_DrawElementsUserBuf(ctx->Dispatch.Current,
                     ctx->DrawID = 0;
   _mesa_reference_buffer_object(ctx, &index_buffer, NULL);
      }
      static inline bool
   should_convert_to_begin_end(struct gl_context *ctx, unsigned count,
               {
      /* Some of these are limitations of _mesa_glthread_UnrollDrawElements.
   * Others prevent syncing, such as disallowing buffer objects because we
   * can't map them without syncing.
   */
   return ctx->API == API_OPENGL_COMPAT &&
         util_is_vbo_upload_ratio_too_large(count, num_upload_vertices) &&
   instance_count == 1 &&                /* no instancing */
   vao->CurrentElementBufferName == 0 && /* only user indices */
   !ctx->GLThread._PrimitiveRestart &&   /* no primitive restart */
      }
      static void
   draw_elements(GLuint drawid, GLenum mode, GLsizei count, GLenum type,
               const GLvoid *indices, GLsizei instance_count, GLint basevertex,
   {
               if (unlikely(compiled_into_dlist && ctx->GLThread.ListMode)) {
               /* Only use the ones that are compiled into display lists. */
   if (basevertex) {
      CALL_DrawElementsBaseVertex(ctx->Dispatch.Current,
      } else if (index_bounds_valid) {
      CALL_DrawRangeElements(ctx->Dispatch.Current,
      } else {
         }
               if (unlikely(index_bounds_valid && max_index < min_index)) {
      _mesa_marshal_InternalSetError(GL_INVALID_VALUE);
               struct glthread_vao *vao = ctx->GLThread.CurrentVAO;
   unsigned user_buffer_mask =
                  /* Fast path when nothing needs to be done.
   *
   * This is also an error path. Zero counts should still call the driver
   * for possible GL errors.
   */
   if (count <= 0 || instance_count <= 0 ||
      !is_index_type_valid(type) ||
   (!user_buffer_mask && !has_user_indices) ||
   ctx->Dispatch.Current == ctx->Dispatch.ContextLost ||
   /* This will just generate GL_INVALID_OPERATION, as it should. */
   ctx->GLThread.inside_begin_end ||
   ctx->GLThread.ListMode) {
   if (instance_count == 1 && baseinstance == 0 && drawid == 0) {
      int cmd_size = sizeof(struct marshal_cmd_DrawElementsBaseVertex);
                  cmd->mode = MIN2(mode, 0xffff);
   cmd->type = MIN2(type, 0xffff);
   cmd->count = count;
   cmd->indices = indices;
      } else {
      if (basevertex == 0 && baseinstance == 0 && drawid == 0) {
      int cmd_size = sizeof(struct marshal_cmd_DrawElementsInstanced);
                  cmd->mode = MIN2(mode, 0xffff);
   cmd->type = MIN2(type, 0xffff);
   cmd->count = count;
   cmd->instance_count = instance_count;
      } else if (drawid == 0) {
      int cmd_size = sizeof(struct marshal_cmd_DrawElementsInstancedBaseVertexBaseInstance);
                  cmd->mode = MIN2(mode, 0xffff);
   cmd->type = MIN2(type, 0xffff);
   cmd->count = count;
   cmd->instance_count = instance_count;
   cmd->basevertex = basevertex;
   cmd->baseinstance = baseinstance;
      } else {
      int cmd_size = sizeof(struct marshal_cmd_DrawElementsInstancedBaseVertexBaseInstanceDrawID);
                  cmd->mode = MIN2(mode, 0xffff);
   cmd->type = MIN2(type, 0xffff);
   cmd->count = count;
   cmd->instance_count = instance_count;
   cmd->basevertex = basevertex;
   cmd->baseinstance = baseinstance;
   cmd->drawid = drawid;
         }
               bool need_index_bounds = user_buffer_mask & ~vao->NonZeroDivisorMask;
            if (need_index_bounds && !index_bounds_valid) {
      /* Compute the index bounds. */
   if (has_user_indices) {
      min_index = ~0;
   max_index = 0;
   vbo_get_minmax_index_mapped(count, index_size,
                  } else {
      /* Indices in a buffer. */
   _mesa_glthread_finish_before(ctx, "DrawElements - need index bounds");
   vbo_get_minmax_index(ctx, ctx->Array.VAO->IndexBufferObj,
                        }
               unsigned start_vertex = min_index + basevertex;
            /* If the vertex range to upload is much greater than the vertex count (e.g.
   * only 3 vertices with indices 0, 1, 999999), uploading the whole range
   * would take too much time. If all buffers are user buffers, have glthread
   * fetch all indices and vertices and convert the draw into glBegin/glEnd.
   * For such pathological cases, it's the fastest way.
   *
   * The game Cogs benefits from this - its FPS increases from 0 to 197.
   */
   if (should_convert_to_begin_end(ctx, count, num_vertices, instance_count,
            _mesa_glthread_UnrollDrawElements(ctx, mode, count, type, indices,
                     struct glthread_attrib_binding buffers[VERT_ATTRIB_MAX];
   if (user_buffer_mask) {
      if (!upload_vertices(ctx, user_buffer_mask, start_vertex, num_vertices,
                     /* Upload indices. */
   struct gl_buffer_object *index_buffer = NULL;
   if (has_user_indices) {
      index_buffer = upload_indices(ctx, count, index_size, &indices);
   if (!index_buffer)
               /* Draw asynchronously. */
   int buffers_size = util_bitcount(user_buffer_mask) * sizeof(buffers[0]);
   int cmd_size = sizeof(struct marshal_cmd_DrawElementsUserBuf) +
                  cmd = _mesa_glthread_allocate_command(ctx, DISPATCH_CMD_DrawElementsUserBuf, cmd_size);
   cmd->mode = MIN2(mode, 0xffff);
   cmd->type = MIN2(type, 0xffff);
   cmd->count = count;
   cmd->indices = indices;
   cmd->instance_count = instance_count;
   cmd->basevertex = basevertex;
   cmd->baseinstance = baseinstance;
   cmd->user_buffer_mask = user_buffer_mask;
   cmd->index_buffer = index_buffer;
            if (user_buffer_mask)
      }
      struct marshal_cmd_MultiDrawElementsUserBuf
   {
      struct marshal_cmd_base cmd_base;
   bool has_base_vertex;
   GLenum8 mode;
   GLenum16 type;
   GLsizei draw_count;
   GLuint user_buffer_mask;
      };
      uint32_t
   _mesa_unmarshal_MultiDrawElementsUserBuf(struct gl_context *ctx,
         {
      const GLsizei draw_count = cmd->draw_count;
   const GLuint user_buffer_mask = cmd->user_buffer_mask;
            const char *variable_data = (const char *)(cmd + 1);
   const GLsizei *count = (GLsizei *)variable_data;
   variable_data += sizeof(GLsizei) * draw_count;
   const GLvoid *const *indices = (const GLvoid *const *)variable_data;
   variable_data += sizeof(const GLvoid *const *) * draw_count;
   const GLsizei *basevertex = NULL;
   if (has_base_vertex) {
      basevertex = (GLsizei *)variable_data;
      }
   const struct glthread_attrib_binding *buffers =
            /* Bind uploaded buffers if needed. */
   if (user_buffer_mask)
            /* Draw. */
   const GLenum mode = cmd->mode;
   const GLenum type = cmd->type;
            CALL_MultiDrawElementsUserBuf(ctx->Dispatch.Current,
               _mesa_reference_buffer_object(ctx, &index_buffer, NULL);
      }
      static void
   multi_draw_elements_async(struct gl_context *ctx, GLenum mode,
                           const GLsizei *count, GLenum type,
   const GLvoid *const *indices, GLsizei draw_count,
   {
      int real_draw_count = MAX2(draw_count, 0);
   int count_size = sizeof(GLsizei) * real_draw_count;
   int indices_size = sizeof(indices[0]) * real_draw_count;
   int basevertex_size = basevertex ? sizeof(GLsizei) * real_draw_count : 0;
   int buffers_size = util_bitcount(user_buffer_mask) * sizeof(buffers[0]);
   int cmd_size = sizeof(struct marshal_cmd_MultiDrawElementsUserBuf) +
                  /* Make sure cmd can fit the queue buffer */
   if (cmd_size <= MARSHAL_MAX_CMD_SIZE) {
      cmd = _mesa_glthread_allocate_command(ctx, DISPATCH_CMD_MultiDrawElementsUserBuf, cmd_size);
   cmd->mode = MIN2(mode, 0xff); /* primitive types go from 0 to 14 */
   cmd->type = MIN2(type, 0xffff);
   cmd->draw_count = draw_count;
   cmd->user_buffer_mask = user_buffer_mask;
   cmd->index_buffer = index_buffer;
            char *variable_data = (char*)(cmd + 1);
   memcpy(variable_data, count, count_size);
   variable_data += count_size;
   memcpy(variable_data, indices, indices_size);
            if (basevertex) {
      memcpy(variable_data, basevertex, basevertex_size);
               if (user_buffer_mask)
      } else {
      /* The call is too large, so sync and execute the unmarshal code here. */
            /* Bind uploaded buffers if needed. */
   if (user_buffer_mask)
            /* Draw. */
   CALL_MultiDrawElementsUserBuf(ctx->Dispatch.Current,
                     }
      void GLAPIENTRY
   _mesa_marshal_MultiDrawElementsBaseVertex(GLenum mode, const GLsizei *count,
                           {
               if (unlikely(ctx->GLThread.ListMode)) {
               if (basevertex) {
      CALL_MultiDrawElementsBaseVertex(ctx->Dispatch.Current,
            } else {
      CALL_MultiDrawElements(ctx->Dispatch.Current,
      }
               struct glthread_vao *vao = ctx->GLThread.CurrentVAO;
   unsigned user_buffer_mask = 0;
            /* Non-VBO vertex arrays are used only if this is true.
   * When nothing needs to be uploaded or the draw is no-op or generates
   * a GL error, we don't upload anything.
   */
   if (draw_count > 0 && is_index_type_valid(type) &&
      ctx->Dispatch.Current != ctx->Dispatch.ContextLost &&
   !ctx->GLThread.inside_begin_end) {
   user_buffer_mask = _mesa_is_desktop_gl_core(ctx) ? 0 : get_user_buffer_mask(ctx);
               /* Fast path when we don't need to upload anything. */
   if (!user_buffer_mask && !has_user_indices) {
      multi_draw_elements_async(ctx, mode, count, type, indices,
                     bool need_index_bounds = user_buffer_mask & ~vao->NonZeroDivisorMask;
   unsigned index_size = get_index_size(type);
   unsigned min_index = ~0;
   unsigned max_index = 0;
   unsigned total_count = 0;
            /* This is always true if there is per-vertex data that needs to be
   * uploaded.
   */
   if (need_index_bounds) {
               /* Compute the index bounds. */
   for (unsigned i = 0; i < draw_count; i++) {
               if (vertex_count < 0) {
      /* Just call the driver to set the error. */
   multi_draw_elements_async(ctx, mode, count, type, indices, draw_count,
            }
                  unsigned min = ~0, max = 0;
   if (has_user_indices) {
      vbo_get_minmax_index_mapped(vertex_count, index_size,
                  } else {
      if (!synced) {
      _mesa_glthread_finish_before(ctx, "MultiDrawElements - need index bounds");
      }
   vbo_get_minmax_index(ctx, ctx->Array.VAO->IndexBufferObj,
                                 if (basevertex) {
      min += basevertex[i];
      }
   min_index = MIN2(min_index, min);
   max_index = MAX2(max_index, max);
                        if (total_count == 0 || num_vertices == 0) {
      /* Nothing to do, but call the driver to set possible GL errors. */
   multi_draw_elements_async(ctx, mode, count, type, indices, draw_count,
               } else if (has_user_indices) {
      /* Only compute total_count for the upload of indices. */
   for (unsigned i = 0; i < draw_count; i++) {
               if (vertex_count < 0) {
      /* Just call the driver to set the error. */
   multi_draw_elements_async(ctx, mode, count, type, indices, draw_count,
            }
                              if (total_count == 0) {
      /* Nothing to do, but call the driver to set possible GL errors. */
   multi_draw_elements_async(ctx, mode, count, type, indices, draw_count,
                        /* Upload vertices. */
   struct glthread_attrib_binding buffers[VERT_ATTRIB_MAX];
   if (user_buffer_mask) {
      if (!upload_vertices(ctx, user_buffer_mask, min_index, num_vertices,
                     /* Upload indices. */
   struct gl_buffer_object *index_buffer = NULL;
   if (has_user_indices) {
               index_buffer = upload_multi_indices(ctx, total_count, index_size,
               if (!index_buffer)
                        /* Draw asynchronously. */
   multi_draw_elements_async(ctx, mode, count, type, indices, draw_count,
            }
      void GLAPIENTRY
   _mesa_marshal_MultiModeDrawArraysIBM(const GLenum *mode, const GLint *first,
               {
      for (int i = 0 ; i < primcount; i++) {
      if (count[i] > 0) {
      GLenum m = *((GLenum *)((GLubyte *)mode + i * modestride));
            }
      void GLAPIENTRY
   _mesa_marshal_MultiModeDrawElementsIBM(const GLenum *mode,
                     {
      for (int i = 0 ; i < primcount; i++) {
      if (count[i] > 0) {
      GLenum m = *((GLenum *)((GLubyte *)mode + i * modestride));
            }
      static const void *
   map_draw_indirect_params(struct gl_context *ctx, GLintptr offset,
         {
               if (!obj)
            return _mesa_bufferobj_map_range(ctx, offset,
            }
      static void
   unmap_draw_indirect_params(struct gl_context *ctx)
   {
      if (ctx->DrawIndirectBuffer)
      }
      static unsigned
   read_draw_indirect_count(struct gl_context *ctx, GLintptr offset)
   {
               if (ctx->ParameterBuffer) {
      _mesa_bufferobj_get_subdata(ctx, offset, sizeof(result), &result,
      }
      }
      static void
   lower_draw_arrays_indirect(struct gl_context *ctx, GLenum mode,
               {
      /* If <stride> is zero, the elements are tightly packed. */
   if (stride == 0)
            const uint32_t *params =
            for (unsigned i = 0; i < draw_count; i++) {
      draw_arrays(i, mode,
               params[i * stride / 4 + 2],
                  }
      static void
   lower_draw_elements_indirect(struct gl_context *ctx, GLenum mode, GLenum type,
               {
      /* If <stride> is zero, the elements are tightly packed. */
   if (stride == 0)
            const uint32_t *params =
            for (unsigned i = 0; i < draw_count; i++) {
      draw_elements(i, mode,
               params[i * stride / 4 + 0],
   type,
   (GLvoid*)((uintptr_t)params[i * stride / 4 + 2] *
         params[i * stride / 4 + 1],
      }
      }
      static inline bool
   draw_indirect_async_allowed(struct gl_context *ctx, unsigned user_buffer_mask)
   {
      return ctx->API != API_OPENGL_COMPAT ||
         /* This will just generate GL_INVALID_OPERATION, as it should. */
   ctx->GLThread.inside_begin_end ||
   ctx->GLThread.ListMode ||
   ctx->Dispatch.Current == ctx->Dispatch.ContextLost ||
   /* If the DrawIndirect buffer is bound, it behaves like profile != compat
      }
      struct marshal_cmd_DrawArraysIndirect
   {
      struct marshal_cmd_base cmd_base;
   GLenum16 mode;
      };
      uint32_t
   _mesa_unmarshal_DrawArraysIndirect(struct gl_context *ctx,
         {
      GLenum mode = cmd->mode;
                     const unsigned cmd_size =
         assert(cmd_size == cmd->cmd_base.cmd_size);
      }
      void GLAPIENTRY
   _mesa_marshal_DrawArraysIndirect(GLenum mode, const GLvoid *indirect)
   {
      GET_CURRENT_CONTEXT(ctx);
   struct glthread_vao *vao = ctx->GLThread.CurrentVAO;
   unsigned user_buffer_mask =
            if (draw_indirect_async_allowed(ctx, user_buffer_mask)) {
      int cmd_size = sizeof(struct marshal_cmd_DrawArraysIndirect);
            cmd = _mesa_glthread_allocate_command(ctx, DISPATCH_CMD_DrawArraysIndirect, cmd_size);
   cmd->mode = MIN2(mode, 0xffff); /* clamped to 0xffff (invalid enum) */
   cmd->indirect = indirect;
               _mesa_glthread_finish_before(ctx, "DrawArraysIndirect");
      }
      struct marshal_cmd_DrawElementsIndirect
   {
      struct marshal_cmd_base cmd_base;
   GLenum16 mode;
   GLenum16 type;
      };
      uint32_t
   _mesa_unmarshal_DrawElementsIndirect(struct gl_context *ctx,
         {
      GLenum mode = cmd->mode;
   GLenum type = cmd->type;
                     const unsigned cmd_size =
         assert(cmd_size == cmd->cmd_base.cmd_size);
      }
      void GLAPIENTRY
   _mesa_marshal_DrawElementsIndirect(GLenum mode, GLenum type, const GLvoid *indirect)
   {
      GET_CURRENT_CONTEXT(ctx);
   struct glthread_vao *vao = ctx->GLThread.CurrentVAO;
   unsigned user_buffer_mask =
            if (draw_indirect_async_allowed(ctx, user_buffer_mask) ||
      !is_index_type_valid(type)) {
   int cmd_size = sizeof(struct marshal_cmd_DrawElementsIndirect);
            cmd = _mesa_glthread_allocate_command(ctx, DISPATCH_CMD_DrawElementsIndirect, cmd_size);
   cmd->mode = MIN2(mode, 0xffff); /* clamped to 0xffff (invalid enum) */
   cmd->type = MIN2(type, 0xffff); /* clamped to 0xffff (invalid enum) */
   cmd->indirect = indirect;
               _mesa_glthread_finish_before(ctx, "DrawElementsIndirect");
      }
      struct marshal_cmd_MultiDrawArraysIndirect
   {
      struct marshal_cmd_base cmd_base;
   GLenum16 mode;
   GLsizei primcount;
   GLsizei stride;
      };
      uint32_t
   _mesa_unmarshal_MultiDrawArraysIndirect(struct gl_context *ctx,
         {
      GLenum mode = cmd->mode;
   const GLvoid * indirect = cmd->indirect;
   GLsizei primcount = cmd->primcount;
            CALL_MultiDrawArraysIndirect(ctx->Dispatch.Current,
            const unsigned cmd_size =
         assert(cmd_size == cmd->cmd_base.cmd_size);
      }
      void GLAPIENTRY
   _mesa_marshal_MultiDrawArraysIndirect(GLenum mode, const GLvoid *indirect,
         {
      GET_CURRENT_CONTEXT(ctx);
   struct glthread_vao *vao = ctx->GLThread.CurrentVAO;
   unsigned user_buffer_mask =
            if (draw_indirect_async_allowed(ctx, user_buffer_mask) ||
      primcount <= 0) {
   int cmd_size = sizeof(struct marshal_cmd_MultiDrawArraysIndirect);
            cmd = _mesa_glthread_allocate_command(ctx, DISPATCH_CMD_MultiDrawArraysIndirect,
         cmd->mode = MIN2(mode, 0xffff); /* clamped to 0xffff (invalid enum) */
   cmd->indirect = indirect;
   cmd->primcount = primcount;
   cmd->stride = stride;
               /* Lower the draw to direct due to non-VBO vertex arrays. */
   _mesa_glthread_finish_before(ctx, "MultiDrawArraysIndirect");
      }
      struct marshal_cmd_MultiDrawElementsIndirect
   {
      struct marshal_cmd_base cmd_base;
   GLenum16 mode;
   GLenum16 type;
   GLsizei primcount;
   GLsizei stride;
      };
      uint32_t
   _mesa_unmarshal_MultiDrawElementsIndirect(struct gl_context *ctx,
         {
      GLenum mode = cmd->mode;
   GLenum type = cmd->type;
   const GLvoid * indirect = cmd->indirect;
   GLsizei primcount = cmd->primcount;
            CALL_MultiDrawElementsIndirect(ctx->Dispatch.Current,
            const unsigned cmd_size =
         assert(cmd_size == cmd->cmd_base.cmd_size);
      }
      void GLAPIENTRY
   _mesa_marshal_MultiDrawElementsIndirect(GLenum mode, GLenum type,
               {
      GET_CURRENT_CONTEXT(ctx);
   struct glthread_vao *vao = ctx->GLThread.CurrentVAO;
   unsigned user_buffer_mask =
            if (draw_indirect_async_allowed(ctx, user_buffer_mask) ||
      primcount <= 0 ||
   !is_index_type_valid(type)) {
   int cmd_size = sizeof(struct marshal_cmd_MultiDrawElementsIndirect);
            cmd = _mesa_glthread_allocate_command(ctx, DISPATCH_CMD_MultiDrawElementsIndirect,
         cmd->mode = MIN2(mode, 0xffff); /* clamped to 0xffff (invalid enum) */
   cmd->type = MIN2(type, 0xffff); /* clamped to 0xffff (invalid enum) */
   cmd->indirect = indirect;
   cmd->primcount = primcount;
   cmd->stride = stride;
               /* Lower the draw to direct due to non-VBO vertex arrays. */
   _mesa_glthread_finish_before(ctx, "MultiDrawElementsIndirect");
   lower_draw_elements_indirect(ctx, mode, type, (GLintptr)indirect, stride,
      }
      struct marshal_cmd_MultiDrawArraysIndirectCountARB
   {
      struct marshal_cmd_base cmd_base;
   GLenum16 mode;
   GLsizei maxdrawcount;
   GLsizei stride;
   GLintptr indirect;
      };
      uint32_t
   _mesa_unmarshal_MultiDrawArraysIndirectCountARB(struct gl_context *ctx,
         {
      GLenum mode = cmd->mode;
   GLintptr indirect = cmd->indirect;
   GLintptr drawcount = cmd->drawcount;
   GLsizei maxdrawcount = cmd->maxdrawcount;
            CALL_MultiDrawArraysIndirectCountARB(ctx->Dispatch.Current,
                  const unsigned cmd_size =
         assert(cmd_size == cmd->cmd_base.cmd_size);
      }
      void GLAPIENTRY
   _mesa_marshal_MultiDrawArraysIndirectCountARB(GLenum mode, GLintptr indirect,
                     {
      GET_CURRENT_CONTEXT(ctx);
   struct glthread_vao *vao = ctx->GLThread.CurrentVAO;
   unsigned user_buffer_mask =
            if (draw_indirect_async_allowed(ctx, user_buffer_mask) ||
      /* This will just generate GL_INVALID_OPERATION because Draw*IndirectCount
   * functions forbid a user indirect buffer in the Compat profile. */
   !ctx->GLThread.CurrentDrawIndirectBufferName) {
   int cmd_size =
         struct marshal_cmd_MultiDrawArraysIndirectCountARB *cmd =
                  cmd->mode = MIN2(mode, 0xffff); /* clamped to 0xffff (invalid enum) */
   cmd->indirect = indirect;
   cmd->drawcount = drawcount;
   cmd->maxdrawcount = maxdrawcount;
   cmd->stride = stride;
               /* Lower the draw to direct due to non-VBO vertex arrays. */
   _mesa_glthread_finish_before(ctx, "MultiDrawArraysIndirectCountARB");
   lower_draw_arrays_indirect(ctx, mode, indirect, stride,
      }
      struct marshal_cmd_MultiDrawElementsIndirectCountARB
   {
      struct marshal_cmd_base cmd_base;
   GLenum16 mode;
   GLenum16 type;
   GLsizei maxdrawcount;
   GLsizei stride;
   GLintptr indirect;
      };
      uint32_t
   _mesa_unmarshal_MultiDrawElementsIndirectCountARB(struct gl_context *ctx,
         {
      GLenum mode = cmd->mode;
   GLenum type = cmd->type;
   GLintptr indirect = cmd->indirect;
   GLintptr drawcount = cmd->drawcount;
   GLsizei maxdrawcount = cmd->maxdrawcount;
                     const unsigned cmd_size = (align(sizeof(struct marshal_cmd_MultiDrawElementsIndirectCountARB), 8) / 8);
   assert(cmd_size == cmd->cmd_base.cmd_size);
      }
      void GLAPIENTRY
   _mesa_marshal_MultiDrawElementsIndirectCountARB(GLenum mode, GLenum type,
                           {
      GET_CURRENT_CONTEXT(ctx);
   struct glthread_vao *vao = ctx->GLThread.CurrentVAO;
   unsigned user_buffer_mask =
            if (draw_indirect_async_allowed(ctx, user_buffer_mask) ||
      /* This will just generate GL_INVALID_OPERATION because Draw*IndirectCount
   * functions forbid a user indirect buffer in the Compat profile. */
   !ctx->GLThread.CurrentDrawIndirectBufferName ||
   !is_index_type_valid(type)) {
   int cmd_size = sizeof(struct marshal_cmd_MultiDrawElementsIndirectCountARB);
   struct marshal_cmd_MultiDrawElementsIndirectCountARB *cmd =
            cmd->mode = MIN2(mode, 0xffff); /* clamped to 0xffff (invalid enum) */
   cmd->type = MIN2(type, 0xffff); /* clamped to 0xffff (invalid enum) */
   cmd->indirect = indirect;
   cmd->drawcount = drawcount;
   cmd->maxdrawcount = maxdrawcount;
   cmd->stride = stride;
               /* Lower the draw to direct due to non-VBO vertex arrays. */
   _mesa_glthread_finish_before(ctx, "MultiDrawElementsIndirectCountARB");
   lower_draw_elements_indirect(ctx, mode, type, indirect, stride,
      }
      void GLAPIENTRY
   _mesa_marshal_DrawArrays(GLenum mode, GLint first, GLsizei count)
   {
         }
      void GLAPIENTRY
   _mesa_marshal_DrawArraysInstanced(GLenum mode, GLint first, GLsizei count,
         {
         }
      void GLAPIENTRY
   _mesa_marshal_DrawArraysInstancedBaseInstance(GLenum mode, GLint first,
               {
         }
      void GLAPIENTRY
   _mesa_marshal_DrawElements(GLenum mode, GLsizei count, GLenum type,
         {
         }
      void GLAPIENTRY
   _mesa_marshal_DrawRangeElements(GLenum mode, GLuint start, GLuint end,
               {
         }
      void GLAPIENTRY
   _mesa_marshal_DrawElementsInstanced(GLenum mode, GLsizei count, GLenum type,
         {
         }
      void GLAPIENTRY
   _mesa_marshal_DrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type,
         {
         }
      void GLAPIENTRY
   _mesa_marshal_DrawRangeElementsBaseVertex(GLenum mode, GLuint start, GLuint end,
               {
         }
      void GLAPIENTRY
   _mesa_marshal_DrawElementsInstancedBaseVertex(GLenum mode, GLsizei count,
               {
         }
      void GLAPIENTRY
   _mesa_marshal_DrawElementsInstancedBaseInstance(GLenum mode, GLsizei count,
               {
         }
      void GLAPIENTRY
   _mesa_marshal_DrawElementsInstancedBaseVertexBaseInstance(GLenum mode, GLsizei count,
                     {
         }
      void GLAPIENTRY
   _mesa_marshal_MultiDrawElements(GLenum mode, const GLsizei *count,
               {
      _mesa_marshal_MultiDrawElementsBaseVertex(mode, count, type, indices,
      }
      uint32_t
   _mesa_unmarshal_DrawArraysInstanced(struct gl_context *ctx,
         {
      unreachable("should never end up here");
      }
      uint32_t
   _mesa_unmarshal_MultiDrawArrays(struct gl_context *ctx,
         {
      unreachable("should never end up here");
      }
      uint32_t
   _mesa_unmarshal_DrawElements(struct gl_context *ctx,
         {
      unreachable("should never end up here");
      }
      uint32_t
   _mesa_unmarshal_DrawRangeElements(struct gl_context *ctx,
         {
      unreachable("should never end up here");
      }
      uint32_t
   _mesa_unmarshal_DrawRangeElementsBaseVertex(struct gl_context *ctx,
         {
      unreachable("should never end up here");
      }
      uint32_t
   _mesa_unmarshal_DrawElementsInstancedBaseVertex(struct gl_context *ctx,
         {
      unreachable("should never end up here");
      }
      uint32_t
   _mesa_unmarshal_DrawElementsInstancedBaseInstance(struct gl_context *ctx,
         {
      unreachable("should never end up here");
      }
      uint32_t
   _mesa_unmarshal_MultiDrawElements(struct gl_context *ctx,
         {
      unreachable("should never end up here");
      }
      uint32_t
   _mesa_unmarshal_MultiDrawElementsBaseVertex(struct gl_context *ctx,
         {
      unreachable("should never end up here");
      }
      uint32_t
   _mesa_unmarshal_MultiModeDrawArraysIBM(struct gl_context *ctx,
         {
      unreachable("should never end up here");
      }
      uint32_t
   _mesa_unmarshal_MultiModeDrawElementsIBM(struct gl_context *ctx,
         {
      unreachable("should never end up here");
      }
      void GLAPIENTRY
   _mesa_marshal_DrawArraysUserBuf(void)
   {
         }
      void GLAPIENTRY
   _mesa_marshal_DrawElementsUserBuf(GLintptr indexBuf, GLenum mode,
                     {
         }
      void GLAPIENTRY
   _mesa_marshal_MultiDrawArraysUserBuf(void)
   {
         }
      void GLAPIENTRY
   _mesa_marshal_MultiDrawElementsUserBuf(GLintptr indexBuf, GLenum mode,
                           {
         }
      void GLAPIENTRY
   _mesa_marshal_DrawArraysInstancedBaseInstanceDrawID(void)
   {
         }
      void GLAPIENTRY
   _mesa_marshal_DrawElementsInstancedBaseVertexBaseInstanceDrawID(void)
   {
         }
      void GLAPIENTRY
   _mesa_DrawArraysUserBuf(void)
   {
         }
      void GLAPIENTRY
   _mesa_MultiDrawArraysUserBuf(void)
   {
         }
      void GLAPIENTRY
   _mesa_DrawArraysInstancedBaseInstanceDrawID(void)
   {
         }
      void GLAPIENTRY
   _mesa_DrawElementsInstancedBaseVertexBaseInstanceDrawID(void)
   {
         }
