   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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
      #include <stdbool.h>
   #include <stdio.h>
   #include "main/arrayobj.h"
   #include "util/glheader.h"
   #include "main/bufferobj.h"
   #include "main/context.h"
   #include "main/enums.h"
   #include "main/state.h"
   #include "main/varray.h"
      #include "vbo_private.h"
      static void
   vbo_exec_debug_verts(struct vbo_exec_context *exec)
   {
      GLuint count = exec->vtx.vert_count;
            printf("%s: %u vertices %d primitives, %d vertsize\n",
         __func__,
   count,
            for (i = 0 ; i < exec->vtx.prim_count ; i++) {
      printf("   prim %d: %s %d..%d %s %s\n",
         i,
   _mesa_lookup_prim_by_nr(exec->vtx.mode[i]),
   exec->vtx.draw[i].start,
   exec->vtx.draw[i].start + exec->vtx.draw[i].count,
         }
         static GLuint
   vbo_exec_copy_vertices(struct vbo_exec_context *exec)
   {
      struct gl_context *ctx = gl_context_from_vbo_exec(exec);
   const GLuint sz = exec->vtx.vertex_size;
   fi_type *dst = exec->vtx.copied.buffer;
   unsigned last = exec->vtx.prim_count - 1;
   unsigned start = exec->vtx.draw[last].start;
            return vbo_copy_vertices(ctx, ctx->Driver.CurrentExecPrimitive,
                        }
            /* TODO: populate these as the vertex is defined:
   */
   static void
   vbo_exec_bind_arrays(struct gl_context *ctx,
               {
      struct vbo_context *vbo = vbo_context(ctx);
   struct gl_vertex_array_object *vao = vbo->VAO;
            GLintptr buffer_offset;
   if (exec->vtx.bufferobj) {
      assert(exec->vtx.bufferobj->Mappings[MAP_INTERNAL].Pointer);
   buffer_offset = exec->vtx.bufferobj->Mappings[MAP_INTERNAL].Offset +
      } else {
      /* Ptr into ordinary app memory */
                        GLbitfield vao_enabled, vao_filter;
   if (_mesa_hw_select_enabled(ctx)) {
      /* HW GL_SELECT has fixed input */
      } else {
      vao_enabled = _vbo_get_vao_enabled_from_vbo(mode, exec->vtx.enabled);
               /* At first disable arrays no longer needed */
   _mesa_disable_vertex_array_attribs(ctx, vao, ~vao_enabled);
            /* Bind the buffer object */
   const GLuint stride = exec->vtx.vertex_size*sizeof(GLfloat);
   _mesa_bind_vertex_buffer(ctx, vao, 0, exec->vtx.bufferobj, buffer_offset,
            /* Retrieve the mapping from VBO_ATTRIB to VERT_ATTRIB space
   * Note that the position/generic0 aliasing is done in the VAO.
   */
   const GLubyte *const vao_to_vbo_map = _vbo_attribute_alias_map[mode];
   /* Now set the enabled arrays */
   GLbitfield mask = vao_enabled;
   while (mask) {
      const int vao_attr = u_bit_scan(&mask);
            const GLubyte size = exec->vtx.attr[vbo_attr].size;
   const GLenum16 type = exec->vtx.attr[vbo_attr].type;
   const GLuint offset = (GLuint)((GLbyte *)exec->vtx.attrptr[vbo_attr] -
                  /* Set and enable */
   _vbo_set_attrib_format(ctx, vao, vao_attr, buffer_offset,
            /* The vao is initially created with all bindings set to 0. */
      }
   _mesa_enable_vertex_array_attribs(ctx, vao, vao_enabled);
   assert(vao_enabled == vao->Enabled);
   assert(!exec->vtx.bufferobj ||
            _mesa_save_and_set_draw_vao(ctx, vao, vao_filter,
         _mesa_set_varying_vp_inputs(ctx, vao_filter &
      }
         /**
   * Unmap the VBO.  This is called before drawing.
   */
   static void
   vbo_exec_vtx_unmap(struct vbo_exec_context *exec)
   {
      if (exec->vtx.bufferobj) {
               if (!ctx->Extensions.ARB_buffer_storage) {
      GLintptr offset = exec->vtx.buffer_used -
                        if (length)
      _mesa_bufferobj_flush_mapped_range(ctx, offset, length,
                  exec->vtx.buffer_used += (exec->vtx.buffer_ptr -
            assert(exec->vtx.buffer_used <= ctx->Const.glBeginEndBufferSize);
            _mesa_bufferobj_unmap(ctx, exec->vtx.bufferobj, MAP_INTERNAL);
   exec->vtx.buffer_map = NULL;
   exec->vtx.buffer_ptr = NULL;
         }
      static bool
   vbo_exec_buffer_has_space(struct vbo_exec_context *exec)
   {
                  }
         /**
   * Map the vertex buffer to begin storing glVertex, glColor, etc data.
   */
   void
   vbo_exec_vtx_map(struct vbo_exec_context *exec)
   {
      struct gl_context *ctx = gl_context_from_vbo_exec(exec);
   const GLenum usage = GL_STREAM_DRAW_ARB;
   GLenum accessRange = GL_MAP_WRITE_BIT |  /* for MapBufferRange */
            if (ctx->Extensions.ARB_buffer_storage) {
      /* We sometimes read from the buffer, so map it for read too.
   * Only the persistent mapping can do that, because the non-persistent
   * mapping uses flags that are incompatible with GL_MAP_READ_BIT.
   */
   accessRange |= GL_MAP_PERSISTENT_BIT |
            } else {
      accessRange |= GL_MAP_INVALIDATE_RANGE_BIT |
                     if (!exec->vtx.bufferobj)
            assert(!exec->vtx.buffer_map);
            if (vbo_exec_buffer_has_space(exec)) {
      /* The VBO exists and there's room for more */
   if (exec->vtx.bufferobj->Size > 0) {
      exec->vtx.buffer_map = (fi_type *)
      _mesa_bufferobj_map_range(ctx,
                           exec->vtx.buffer_used,
         }
   else {
                     if (!exec->vtx.buffer_map) {
      /* Need to allocate a new VBO */
            if (_mesa_bufferobj_data(ctx, GL_ARRAY_BUFFER_ARB,
                           ctx->Const.glBeginEndBufferSize,
   NULL, usage,
   GL_MAP_WRITE_BIT |
   (ctx->Extensions.ARB_buffer_storage ?
   GL_MAP_PERSISTENT_BIT |
   GL_MAP_COHERENT_BIT |
      /* buffer allocation worked, now map the buffer */
   exec->vtx.buffer_map =
      (fi_type *)_mesa_bufferobj_map_range(ctx,
                     }
   else {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "VBO allocation");
                  exec->vtx.buffer_ptr = exec->vtx.buffer_map;
            if (!exec->vtx.buffer_map) {
      /* out of memory */
      }
   else {
      if (_mesa_using_noop_vtxfmt(ctx->Dispatch.Exec)) {
      /* The no-op functions are installed so switch back to regular
   * functions.  We do this test just to avoid frequent and needless
   * calls to vbo_install_exec_vtxfmt().
   */
                  if (0)
      }
            /**
   * Execute the buffer and save copied verts.
   */
   void
   vbo_exec_vtx_flush(struct vbo_exec_context *exec)
   {
               /* Only unmap if persistent mappings are unsupported. */
   bool persistent_mapping = ctx->Extensions.ARB_buffer_storage &&
                  if (0)
            if (exec->vtx.prim_count &&
                        if (exec->vtx.copied.nr != exec->vtx.vert_count) {
                                                                           if (0)
                  ctx->Driver.DrawGalliumMultiMode(ctx, &exec->vtx.info,
                        /* Get new storage -- unless asked not to. */
                                 if (persistent_mapping) {
      exec->vtx.buffer_used += (exec->vtx.buffer_ptr - exec->vtx.buffer_map) *
                  /* Set the buffer offset for the next draw. */
            if (!vbo_exec_buffer_has_space(exec)) {
      /* This will allocate a new buffer. */
   vbo_exec_vtx_unmap(exec);
                  if (exec->vtx.vertex_size == 0)
         else
            exec->vtx.buffer_ptr = exec->vtx.buffer_map;
   exec->vtx.prim_count = 0;
      }
