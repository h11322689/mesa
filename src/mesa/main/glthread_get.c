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
      #include "main/glthread_marshal.h"
   #include "main/dispatch.h"
      uint32_t
   _mesa_unmarshal_GetIntegerv(struct gl_context *ctx,
         {
      unreachable("never executed");
      }
      void GLAPIENTRY
   _mesa_marshal_GetIntegerv(GLenum pname, GLint *p)
   {
               /* This will generate GL_INVALID_OPERATION, as it should. */
   if (ctx->GLThread.inside_begin_end)
            /* TODO: Use get_hash_params.py to return values for items containing:
   * - CONST(
   * - CONTEXT_[A-Z]*(Const
            switch (pname) {
   case GL_ACTIVE_TEXTURE:
      *p = GL_TEXTURE0 + ctx->GLThread.ActiveTexture;
      case GL_ARRAY_BUFFER_BINDING:
      *p = ctx->GLThread.CurrentArrayBufferName;
      case GL_ATTRIB_STACK_DEPTH:
      *p = ctx->GLThread.AttribStackDepth;
      case GL_CLIENT_ACTIVE_TEXTURE:
      *p = GL_TEXTURE0 + ctx->GLThread.ClientActiveTexture;
      case GL_CLIENT_ATTRIB_STACK_DEPTH:
      *p = ctx->GLThread.ClientAttribStackTop;
      case GL_CURRENT_PROGRAM:
      *p = ctx->GLThread.CurrentProgram;
      case GL_DRAW_INDIRECT_BUFFER_BINDING:
      *p = ctx->GLThread.CurrentDrawIndirectBufferName;
      case GL_DRAW_FRAMEBUFFER_BINDING:
      *p = ctx->GLThread.CurrentDrawFramebuffer;
      case GL_READ_FRAMEBUFFER_BINDING:
      *p = ctx->GLThread.CurrentReadFramebuffer;
      case GL_PIXEL_PACK_BUFFER_BINDING:
      *p = ctx->GLThread.CurrentPixelPackBufferName;
      case GL_PIXEL_UNPACK_BUFFER_BINDING:
      *p = ctx->GLThread.CurrentPixelUnpackBufferName;
      case GL_QUERY_BUFFER_BINDING:
      *p = ctx->GLThread.CurrentQueryBufferName;
         case GL_MATRIX_MODE:
      *p = ctx->GLThread.MatrixMode;
      case GL_CURRENT_MATRIX_STACK_DEPTH_ARB:
      *p = ctx->GLThread.MatrixStackDepth[ctx->GLThread.MatrixIndex] + 1;
      case GL_MODELVIEW_STACK_DEPTH:
      *p = ctx->GLThread.MatrixStackDepth[M_MODELVIEW] + 1;
      case GL_PROJECTION_STACK_DEPTH:
      *p = ctx->GLThread.MatrixStackDepth[M_PROJECTION] + 1;
      case GL_TEXTURE_STACK_DEPTH:
      *p = ctx->GLThread.MatrixStackDepth[M_TEXTURE0 + ctx->GLThread.ActiveTexture] + 1;
         case GL_VERTEX_ARRAY:
      *p = (ctx->GLThread.CurrentVAO->UserEnabled & (1 << VERT_ATTRIB_POS)) != 0;
      case GL_NORMAL_ARRAY:
      *p = (ctx->GLThread.CurrentVAO->UserEnabled & (1 << VERT_ATTRIB_NORMAL)) != 0;
      case GL_COLOR_ARRAY:
      *p = (ctx->GLThread.CurrentVAO->UserEnabled & (1 << VERT_ATTRIB_COLOR0)) != 0;
      case GL_SECONDARY_COLOR_ARRAY:
      *p = (ctx->GLThread.CurrentVAO->UserEnabled & (1 << VERT_ATTRIB_COLOR1)) != 0;
      case GL_FOG_COORD_ARRAY:
      *p = (ctx->GLThread.CurrentVAO->UserEnabled & (1 << VERT_ATTRIB_FOG)) != 0;
      case GL_INDEX_ARRAY:
      *p = (ctx->GLThread.CurrentVAO->UserEnabled & (1 << VERT_ATTRIB_COLOR_INDEX)) != 0;
      case GL_EDGE_FLAG_ARRAY:
      *p = (ctx->GLThread.CurrentVAO->UserEnabled & (1 << VERT_ATTRIB_EDGEFLAG)) != 0;
      case GL_TEXTURE_COORD_ARRAY:
      *p = (ctx->GLThread.CurrentVAO->UserEnabled &
            case GL_POINT_SIZE_ARRAY_OES:
      *p = (ctx->GLThread.CurrentVAO->UserEnabled & (1 << VERT_ATTRIB_POINT_SIZE)) != 0;
            sync:
      _mesa_glthread_finish_before(ctx, "GetIntegerv");
      }
      /* TODO: Implement glGetBooleanv, glGetFloatv, etc. if needed */
