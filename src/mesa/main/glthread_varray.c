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
      /* This implements vertex array state tracking for glthread. It's separate
   * from the rest of Mesa. Only minimum functionality is implemented here
   * to serve glthread.
   */
      #include "main/glthread.h"
   #include "main/glformats.h"
   #include "main/mtypes.h"
   #include "main/hash.h"
   #include "main/dispatch.h"
   #include "main/varray.h"
      static unsigned
   element_size(union gl_vertex_format_user format)
   {
         }
      static void
   init_attrib(struct glthread_attrib *attrib, int index, int size, GLenum type)
   {
      attrib->Format = MESA_PACK_VFORMAT(type, size, 0, 0, 0);
   attrib->ElementSize = element_size(attrib->Format);
   attrib->RelativeOffset = 0;
   attrib->BufferIndex = index;
   attrib->Stride = attrib->ElementSize;
   attrib->Divisor = 0;
   attrib->EnabledAttribCount = 0;
      }
      void
   _mesa_glthread_reset_vao(struct glthread_vao *vao)
   {
      vao->CurrentElementBufferName = 0;
   vao->UserEnabled = 0;
   vao->Enabled = 0;
   vao->BufferEnabled = 0;
   vao->UserPointerMask = 0;
   vao->NonNullPointerMask = 0;
            for (unsigned i = 0; i < ARRAY_SIZE(vao->Attrib); i++) {
      switch (i) {
   case VERT_ATTRIB_NORMAL:
      init_attrib(&vao->Attrib[i], i, 3, GL_FLOAT);
      case VERT_ATTRIB_COLOR1:
      init_attrib(&vao->Attrib[i], i, 3, GL_FLOAT);
      case VERT_ATTRIB_FOG:
      init_attrib(&vao->Attrib[i], i, 1, GL_FLOAT);
      case VERT_ATTRIB_COLOR_INDEX:
      init_attrib(&vao->Attrib[i], i, 1, GL_FLOAT);
      case VERT_ATTRIB_EDGEFLAG:
      init_attrib(&vao->Attrib[i], i, 1, GL_UNSIGNED_BYTE);
      case VERT_ATTRIB_POINT_SIZE:
      init_attrib(&vao->Attrib[i], i, 1, GL_FLOAT);
      default:
      init_attrib(&vao->Attrib[i], i, 4, GL_FLOAT);
            }
      static struct glthread_vao *
   lookup_vao(struct gl_context *ctx, GLuint id)
   {
      struct glthread_state *glthread = &ctx->GLThread;
                     if (glthread->LastLookedUpVAO &&
      glthread->LastLookedUpVAO->Name == id) {
      } else {
      vao = _mesa_HashLookupLocked(glthread->VAOs, id);
   if (!vao)
                           }
      void
   _mesa_glthread_BindVertexArray(struct gl_context *ctx, GLuint id)
   {
               if (id == 0) {
         } else {
               if (vao)
         }
      void
   _mesa_glthread_DeleteVertexArrays(struct gl_context *ctx,
         {
               if (!ids)
            for (int i = 0; i < n; i++) {
      /* IDs equal to 0 should be silently ignored. */
   if (!ids[i])
            struct glthread_vao *vao = lookup_vao(ctx, ids[i]);
   if (!vao)
            /* If the array object is currently bound, the spec says "the binding
   * for that object reverts to zero and the default vertex array
   * becomes current."
   */
   if (glthread->CurrentVAO == vao)
            if (glthread->LastLookedUpVAO == vao)
            /* The ID is immediately freed for re-use */
   _mesa_HashRemoveLocked(glthread->VAOs, vao->Name);
         }
      void
   _mesa_glthread_GenVertexArrays(struct gl_context *ctx,
         {
               if (!arrays)
            /* The IDs have been generated at this point. Create VAOs for glthread. */
   for (int i = 0; i < n; i++) {
      GLuint id = arrays[i];
            vao = calloc(1, sizeof(*vao));
   if (!vao)
            vao->Name = id;
   _mesa_glthread_reset_vao(vao);
         }
      /* If vaobj is NULL, use the currently-bound VAO. */
   static inline struct glthread_vao *
   get_vao(struct gl_context *ctx, const GLuint *vaobj)
   {
      if (vaobj)
               }
      static void
   update_primitive_restart(struct gl_context *ctx)
   {
               glthread->_PrimitiveRestart = glthread->PrimitiveRestart ||
         glthread->_RestartIndex[0] =
      _mesa_get_prim_restart_index(glthread->PrimitiveRestartFixedIndex,
      glthread->_RestartIndex[1] =
      _mesa_get_prim_restart_index(glthread->PrimitiveRestartFixedIndex,
      glthread->_RestartIndex[3] =
      _mesa_get_prim_restart_index(glthread->PrimitiveRestartFixedIndex,
   }
      void
   _mesa_glthread_set_prim_restart(struct gl_context *ctx, GLenum cap, bool value)
   {
      switch (cap) {
   case GL_PRIMITIVE_RESTART:
      ctx->GLThread.PrimitiveRestart = value;
      case GL_PRIMITIVE_RESTART_FIXED_INDEX:
      ctx->GLThread.PrimitiveRestartFixedIndex = value;
                  }
      void
   _mesa_glthread_PrimitiveRestartIndex(struct gl_context *ctx, GLuint index)
   {
      ctx->GLThread.RestartIndex = index;
      }
      static inline void
   enable_buffer(struct glthread_vao *vao, unsigned binding_index)
   {
               if (attrib_count == 1)
         else if (attrib_count == 2)
      }
      static inline void
   disable_buffer(struct glthread_vao *vao, unsigned binding_index)
   {
               if (attrib_count == 0)
         else if (attrib_count == 1)
         else
      }
      void
   _mesa_glthread_ClientState(struct gl_context *ctx, GLuint *vaobj,
         {
      /* The primitive restart client state uses a special value. */
   if (attrib == VERT_ATTRIB_PRIMITIVE_RESTART_NV) {
      ctx->GLThread.PrimitiveRestart = enable;
   update_primitive_restart(ctx);
               if (attrib >= VERT_ATTRIB_MAX)
            struct glthread_vao *vao = get_vao(ctx, vaobj);
   if (!vao)
                     if (enable && !(vao->UserEnabled & attrib_bit)) {
               /* The generic0 attribute supersedes the position attribute. We need to
   * update BufferBindingEnabled accordingly.
   */
   if (attrib == VERT_ATTRIB_POS) {
      if (!(vao->UserEnabled & VERT_BIT_GENERIC0))
      } else {
               if (attrib == VERT_ATTRIB_GENERIC0 && vao->UserEnabled & VERT_BIT_POS)
         } else if (!enable && (vao->UserEnabled & attrib_bit)) {
               /* The generic0 attribute supersedes the position attribute. We need to
   * update BufferBindingEnabled accordingly.
   */
   if (attrib == VERT_ATTRIB_POS) {
      if (!(vao->UserEnabled & VERT_BIT_GENERIC0))
      } else {
               if (attrib == VERT_ATTRIB_GENERIC0 && vao->UserEnabled & VERT_BIT_POS)
                  /* The generic0 attribute supersedes the position attribute. */
   vao->Enabled = vao->UserEnabled;
   if (vao->Enabled & VERT_BIT_GENERIC0)
      }
      static void
   set_attrib_binding(struct glthread_state *glthread, struct glthread_vao *vao,
         {
               if (old_binding_index != new_binding_index) {
               if (vao->Enabled & (1u << attrib)) {
      /* Update BufferBindingEnabled. */
   enable_buffer(vao, new_binding_index);
            }
      void _mesa_glthread_AttribDivisor(struct gl_context *ctx, const GLuint *vaobj,
         {
      if (attrib >= VERT_ATTRIB_MAX)
            struct glthread_vao *vao = get_vao(ctx, vaobj);
   if (!vao)
                              if (divisor)
         else
      }
      static void
   attrib_pointer(struct glthread_state *glthread, struct glthread_vao *vao,
                     {
      if (attrib >= VERT_ATTRIB_MAX)
                     vao->Attrib[attrib].Format = format;
   vao->Attrib[attrib].ElementSize = elem_size;
   vao->Attrib[attrib].Stride = stride ? stride : elem_size;
   vao->Attrib[attrib].Pointer = pointer;
                     if (buffer != 0)
         else
            if (pointer)
         else
      }
      void
   _mesa_glthread_AttribPointer(struct gl_context *ctx, gl_vert_attrib attrib,
               {
               attrib_pointer(glthread, glthread->CurrentVAO,
            }
      void
   _mesa_glthread_DSAAttribPointer(struct gl_context *ctx, GLuint vaobj,
                     {
      struct glthread_state *glthread = &ctx->GLThread;
            vao = lookup_vao(ctx, vaobj);
   if (!vao)
            attrib_pointer(glthread, vao, buffer, attrib, format, stride,
      }
      static void
   attrib_format(struct glthread_state *glthread, struct glthread_vao *vao,
               {
      if (attribindex >= VERT_ATTRIB_GENERIC_MAX)
                     unsigned i = VERT_ATTRIB_GENERIC(attribindex);
   vao->Attrib[i].Format = format;
   vao->Attrib[i].ElementSize = elem_size;
      }
      void
   _mesa_glthread_AttribFormat(struct gl_context *ctx, GLuint attribindex,
               {
               attrib_format(glthread, glthread->CurrentVAO, attribindex, format,
      }
      void
   _mesa_glthread_DSAAttribFormat(struct gl_context *ctx, GLuint vaobj,
                     {
      struct glthread_state *glthread = &ctx->GLThread;
            if (vao)
      }
      static void
   bind_vertex_buffer(struct glthread_state *glthread, struct glthread_vao *vao,
               {
      if (bindingindex >= VERT_ATTRIB_GENERIC_MAX)
            unsigned i = VERT_ATTRIB_GENERIC(bindingindex);
   vao->Attrib[i].Pointer = (const void*)offset;
            if (buffer != 0)
         else
            if (offset)
         else
      }
      void
   _mesa_glthread_VertexBuffer(struct gl_context *ctx, GLuint bindingindex,
         {
               bind_vertex_buffer(glthread, glthread->CurrentVAO, bindingindex, buffer,
      }
      void
   _mesa_glthread_DSAVertexBuffer(struct gl_context *ctx, GLuint vaobj,
               {
      struct glthread_state *glthread = &ctx->GLThread;
            if (vao)
      }
      void
   _mesa_glthread_DSAVertexBuffers(struct gl_context *ctx, GLuint vaobj,
                           {
      struct glthread_state *glthread = &ctx->GLThread;
            vao = lookup_vao(ctx, vaobj);
   if (!vao)
            for (unsigned i = 0; i < count; i++) {
      bind_vertex_buffer(glthread, vao, first + i, buffers[i], offsets[i],
         }
      static void
   binding_divisor(struct glthread_state *glthread, struct glthread_vao *vao,
         {
      if (bindingindex >= VERT_ATTRIB_GENERIC_MAX)
            unsigned i = VERT_ATTRIB_GENERIC(bindingindex);
            if (divisor)
         else
      }
      void
   _mesa_glthread_BindingDivisor(struct gl_context *ctx, GLuint bindingindex,
         {
                  }
      void
   _mesa_glthread_DSABindingDivisor(struct gl_context *ctx, GLuint vaobj,
         {
      struct glthread_state *glthread = &ctx->GLThread;
            if (vao)
      }
      void
   _mesa_glthread_AttribBinding(struct gl_context *ctx, GLuint attribindex,
         {
               if (attribindex >= VERT_ATTRIB_GENERIC_MAX ||
      bindingindex >= VERT_ATTRIB_GENERIC_MAX)
         set_attrib_binding(glthread, glthread->CurrentVAO,
            }
      void
   _mesa_glthread_DSAAttribBinding(struct gl_context *ctx, GLuint vaobj,
         {
               if (attribindex >= VERT_ATTRIB_GENERIC_MAX ||
      bindingindex >= VERT_ATTRIB_GENERIC_MAX)
         struct glthread_vao *vao = lookup_vao(ctx, vaobj);
   if (vao) {
      set_attrib_binding(glthread, vao,
               }
      void
   _mesa_glthread_DSAElementBuffer(struct gl_context *ctx, GLuint vaobj,
         {
               if (vao)
      }
      void
   _mesa_glthread_PushClientAttrib(struct gl_context *ctx, GLbitfield mask,
         {
               if (glthread->ClientAttribStackTop >= MAX_CLIENT_ATTRIB_STACK_DEPTH)
            struct glthread_client_attrib *top =
            if (mask & GL_CLIENT_VERTEX_ARRAY_BIT) {
      top->VAO = *glthread->CurrentVAO;
   top->CurrentArrayBufferName = glthread->CurrentArrayBufferName;
   top->ClientActiveTexture = glthread->ClientActiveTexture;
   top->RestartIndex = glthread->RestartIndex;
   top->PrimitiveRestart = glthread->PrimitiveRestart;
   top->PrimitiveRestartFixedIndex = glthread->PrimitiveRestartFixedIndex;
      } else {
                           if (set_default)
      }
      void
   _mesa_glthread_PopClientAttrib(struct gl_context *ctx)
   {
               if (glthread->ClientAttribStackTop == 0)
                     struct glthread_client_attrib *top =
            if (!top->Valid)
            /* Popping a delete VAO is an error. */
   struct glthread_vao *vao = NULL;
   if (top->VAO.Name) {
      vao = lookup_vao(ctx, top->VAO.Name);
   if (!vao)
               /* Restore states. */
   glthread->CurrentArrayBufferName = top->CurrentArrayBufferName;
   glthread->ClientActiveTexture = top->ClientActiveTexture;
   glthread->RestartIndex = top->RestartIndex;
   glthread->PrimitiveRestart = top->PrimitiveRestart;
            if (!vao)
            assert(top->VAO.Name == vao->Name);
   *vao = top->VAO; /* Copy all fields. */
      }
      void
   _mesa_glthread_ClientAttribDefault(struct gl_context *ctx, GLbitfield mask)
   {
               if (!(mask & GL_CLIENT_VERTEX_ARRAY_BIT))
            glthread->CurrentArrayBufferName = 0;
   glthread->ClientActiveTexture = 0;
   glthread->RestartIndex = 0;
   glthread->PrimitiveRestart = false;
   glthread->PrimitiveRestartFixedIndex = false;
   glthread->CurrentVAO = &glthread->DefaultVAO;
      }
      void
   _mesa_glthread_InterleavedArrays(struct gl_context *ctx, GLenum format,
         {
      struct gl_interleaved_layout layout;
            if (stride < 0 || !_mesa_get_interleaved_layout(format, &layout))
            if (!stride)
            _mesa_glthread_ClientState(ctx, NULL, VERT_ATTRIB_EDGEFLAG, false);
   _mesa_glthread_ClientState(ctx, NULL, VERT_ATTRIB_COLOR_INDEX, false);
            /* Texcoords */
   if (layout.tflag) {
      _mesa_glthread_ClientState(ctx, NULL, tex, true);
   _mesa_glthread_AttribPointer(ctx, tex,
                  } else {
                  /* Color */
   if (layout.cflag) {
      _mesa_glthread_ClientState(ctx, NULL, VERT_ATTRIB_COLOR0, true);
   _mesa_glthread_AttribPointer(ctx, VERT_ATTRIB_COLOR0,
                  } else {
                  /* Normals */
   if (layout.nflag) {
      _mesa_glthread_ClientState(ctx, NULL, VERT_ATTRIB_NORMAL, true);
   _mesa_glthread_AttribPointer(ctx, VERT_ATTRIB_NORMAL,
            } else {
                  /* Vertices */
   _mesa_glthread_ClientState(ctx, NULL, VERT_ATTRIB_POS, true);
   _mesa_glthread_AttribPointer(ctx, VERT_ATTRIB_POS,
                  }
      static void
   unbind_uploaded_vbos(void *_vao, void *_ctx)
   {
      struct gl_context *ctx = _ctx;
                     for (unsigned i = 0; i < ARRAY_SIZE(vao->BufferBinding); i++) {
      if (vao->BufferBinding[i].BufferObj &&
      vao->BufferBinding[i].BufferObj->GLThreadInternal) {
   /* We don't need to restore the user pointer because it's never
   * overwritten. When we bind a VBO internally, the user pointer
   * in gl_array_attribute::Ptr becomes ignored and unchanged.
   */
   _mesa_bind_vertex_buffer(ctx, vao, i, NULL, 0,
            }
      /* Unbind VBOs in all VAOs that glthread bound for non-VBO vertex uploads
   * to restore original states.
   */
   void
   _mesa_glthread_unbind_uploaded_vbos(struct gl_context *ctx)
   {
               /* Iterate over all VAOs. */
   _mesa_HashWalk(ctx->Array.Objects, unbind_uploaded_vbos, ctx);
      }
