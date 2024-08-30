   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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
   * Authors:
   *    Keith Whitwell <keithw@vmware.com>
   */
      #include "main/errors.h"
   #include "main/bufferobj.h"
   #include "math/m_eval.h"
   #include "main/api_arrayelt.h"
   #include "main/arrayobj.h"
   #include "main/varray.h"
   #include "main/context.h"
   #include "util/u_memory.h"
   #include "vbo.h"
   #include "vbo_private.h"
         static GLuint
   check_size(const GLfloat *attr)
   {
      if (attr[3] != 1.0F)
         if (attr[2] != 0.0F)
         if (attr[1] != 0.0F)
            }
         /**
   * Helper for initializing a vertex array.
   */
   static void
   init_array(struct gl_context *ctx, struct gl_array_attributes *attrib,
         {
               vbo_set_vertex_format(&attrib->Format, size, GL_FLOAT);
   attrib->Stride = 0;
      }
         /**
   * Set up the vbo->currval arrays to point at the context's current
   * vertex attributes (with strides = 0).
   */
   static void
   init_legacy_currval(struct gl_context *ctx)
   {
               /* Set up a constant (Stride == 0) array for each current
   * attribute:
   */
   for (int attr = 0; attr < VERT_ATTRIB_MAX; attr++) {
      if (VERT_BIT(attr) & VERT_BIT_GENERIC_ALL)
                     init_array(ctx, attrib, check_size(ctx->Current.Attrib[attr]),
         }
         static void
   init_generic_currval(struct gl_context *ctx)
   {
      struct vbo_context *vbo = vbo_context(ctx);
            for (i = 0; i < VERT_ATTRIB_GENERIC_MAX; i++) {
      const unsigned attr = VBO_ATTRIB_GENERIC0 + i;
                  }
         static void
   init_mat_currval(struct gl_context *ctx)
   {
      struct vbo_context *vbo = vbo_context(ctx);
            /* Set up a constant (StrideB == 0) array for each current
   * attribute:
   */
   for (i = 0; i < MAT_ATTRIB_MAX; i++) {
      const unsigned attr = VBO_ATTRIB_MAT_FRONT_AMBIENT + i;
   struct gl_array_attributes *attrib = &vbo->current[attr];
            /* Size is fixed for the material attributes, for others will
   * be determined at runtime:
   */
   switch (i) {
   case MAT_ATTRIB_FRONT_SHININESS:
   case MAT_ATTRIB_BACK_SHININESS:
      size = 1;
      case MAT_ATTRIB_FRONT_INDEXES:
   case MAT_ATTRIB_BACK_INDEXES:
      size = 3;
      default:
      size = 4;
                     }
         void
   vbo_exec_update_eval_maps(struct gl_context *ctx)
   {
                  }
         GLboolean
   _vbo_CreateContext(struct gl_context *ctx)
   {
                        init_legacy_currval(ctx);
   init_generic_currval(ctx);
            /* make sure all VBO_ATTRIB_ values can fit in an unsigned byte */
            vbo_exec_init(ctx);
   if (_mesa_is_desktop_gl_compat(ctx))
            vbo->VAO = _mesa_new_vao(ctx, ~((GLuint)0));
   /* The exec VAO assumes to have all arributes bound to binding 0 */
   for (unsigned i = 0; i < VERT_ATTRIB_MAX; ++i)
                        }
         void
   _vbo_DestroyContext(struct gl_context *ctx)
   {
               if (vbo) {
      vbo_exec_destroy(ctx);
   if (_mesa_is_desktop_gl_compat(ctx))
               }
         const struct gl_array_attributes *
   _vbo_current_attrib(const struct gl_context *ctx, gl_vert_attrib attr)
   {
      const struct vbo_context *vbo = vbo_context_const(ctx);
   const gl_vertex_processing_mode vmp = ctx->VertexProgram._VPMode;
      }
