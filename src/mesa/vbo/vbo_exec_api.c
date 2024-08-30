   /**************************************************************************
      Copyright 2002-2008 VMware, Inc.
      All Rights Reserved.
      Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   on the rights to use, copy, modify, merge, publish, distribute, sub
   license, and/or sell copies of the Software, and to permit persons to whom
   the Software is furnished to do so, subject to the following conditions:
      The above copyright notice and this permission notice (including the next
   paragraph) shall be included in all copies or substantial portions of the
   Software.
      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   VMWARE AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   USE OR OTHER DEALINGS IN THE SOFTWARE.
      **************************************************************************/
      /*
   * Authors:
   *   Keith Whitwell <keithw@vmware.com>
   */
      #include "util/glheader.h"
   #include "main/bufferobj.h"
   #include "main/context.h"
   #include "main/macros.h"
   #include "main/dlist.h"
   #include "main/eval.h"
   #include "main/state.h"
   #include "main/light.h"
   #include "main/api_arrayelt.h"
   #include "main/draw_validate.h"
   #include "main/dispatch.h"
   #include "util/bitscan.h"
   #include "util/u_memory.h"
   #include "api_exec_decl.h"
      #include "vbo_private.h"
      /** ID/name for immediate-mode VBO */
   #define IMM_BUFFER_NAME 0xaabbccdd
         static void
   vbo_reset_all_attr(struct vbo_exec_context *exec);
         /**
   * Close off the last primitive, execute the buffer, restart the
   * primitive.  This is called when we fill a vertex buffer before
   * hitting glEnd.
   */
   static void
   vbo_exec_wrap_buffers(struct vbo_exec_context *exec)
   {
      if (exec->vtx.prim_count == 0) {
      exec->vtx.copied.nr = 0;
   exec->vtx.vert_count = 0;
      }
   else {
      struct gl_context *ctx = gl_context_from_vbo_exec(exec);
   unsigned last = exec->vtx.prim_count - 1;
   struct pipe_draw_start_count_bias *last_draw = &exec->vtx.draw[last];
   const bool last_begin = exec->vtx.markers[last].begin;
            if (_mesa_inside_begin_end(ctx)) {
      last_draw->count = exec->vtx.vert_count - last_draw->start;
   last_count = last_draw->count;
               /* Special handling for wrapping GL_LINE_LOOP */
   if (exec->vtx.mode[last] == GL_LINE_LOOP &&
      last_count > 0 &&
   !exec->vtx.markers[last].end) {
   /* draw this section of the incomplete line loop as a line strip */
   exec->vtx.mode[last] = GL_LINE_STRIP;
   if (!last_begin) {
      /* This is not the first section of the line loop, so don't
   * draw the 0th vertex.  We're saving it until we draw the
   * very last section of the loop.
   */
   last_draw->start++;
                  /* Execute the buffer and save copied vertices.
   */
   if (exec->vtx.vert_count)
         else {
      exec->vtx.prim_count = 0;
               /* Emit a glBegin to start the new list.
   */
            if (_mesa_inside_begin_end(ctx)) {
      exec->vtx.mode[0] = ctx->Driver.CurrentExecPrimitive;
   exec->vtx.draw[0].start = 0;
                  if (exec->vtx.copied.nr == last_count)
            }
         /**
   * Deal with buffer wrapping where provoked by the vertex buffer
   * filling up, as opposed to upgrade_vertex().
   */
   static void
   vbo_exec_vtx_wrap(struct vbo_exec_context *exec)
   {
               /* Run pipeline on current vertices, copy wrapped vertices
   * to exec->vtx.copied.
   */
            if (!exec->vtx.buffer_ptr) {
      /* probably ran out of memory earlier when allocating the VBO */
               /* Copy stored stored vertices to start of new list.
   */
            numComponents = exec->vtx.copied.nr * exec->vtx.vertex_size;
   memcpy(exec->vtx.buffer_ptr,
         exec->vtx.copied.buffer,
   exec->vtx.buffer_ptr += numComponents;
               }
         /**
   * Copy the active vertex's values to the ctx->Current fields.
   */
   static void
   vbo_exec_copy_to_current(struct vbo_exec_context *exec)
   {
      struct gl_context *ctx = gl_context_from_vbo_exec(exec);
   struct vbo_context *vbo = vbo_context(ctx);
   GLbitfield64 enabled = exec->vtx.enabled & (~BITFIELD64_BIT(VBO_ATTRIB_POS));
            while (enabled) {
               /* Note: the exec->vtx.current[i] pointers point into the
   * ctx->Current.Attrib and ctx->Light.Material.Attrib arrays.
   */
   GLfloat *current = (GLfloat *)vbo->current[i].Ptr;
   fi_type tmp[8]; /* space for doubles */
                     /* VBO_ATTRIB_SELECT_RESULT_INDEX has no current */
   if (!current)
            if (exec->vtx.attr[i].type == GL_DOUBLE ||
      exec->vtx.attr[i].type == GL_UNSIGNED_INT64_ARB) {
   memset(tmp, 0, sizeof(tmp));
   memcpy(tmp, exec->vtx.attrptr[i], exec->vtx.attr[i].size * sizeof(GLfloat));
      } else {
      COPY_CLEAN_4V_TYPE_AS_UNION(tmp,
                           if (memcmp(current, tmp, 4 * sizeof(GLfloat) << dmul_shift) != 0) {
                              if (i >= VBO_ATTRIB_MAT_FRONT_AMBIENT) {
                     /* The fixed-func vertex program uses this. */
   if (i == VBO_ATTRIB_MAT_FRONT_SHININESS ||
      i == VBO_ATTRIB_MAT_BACK_SHININESS)
   } else {
                     ctx->NewState |= _NEW_CURRENT_ATTRIB;
                  /* Given that we explicitly state size here, there is no need
   * for the COPY_CLEAN above, could just copy 16 bytes and be
   * done.  The only problem is when Mesa accesses ctx->Current
   * directly.
   */
   /* Size here is in components - not bytes */
   if (exec->vtx.attr[i].type != vbo->current[i].Format.User.Type ||
      (exec->vtx.attr[i].size >> dmul_shift) != vbo->current[i].Format.User.Size) {
   vbo_set_vertex_format(&vbo->current[i].Format,
               /* The format changed. We need to update gallium vertex elements.
   * Material attributes don't need this because they don't have formats.
   */
   if (i <= VBO_ATTRIB_EDGEFLAG)
                  if (color0_changed && ctx->Light.ColorMaterialEnabled) {
      _mesa_update_color_material(ctx,
         }
         /**
   * Flush existing data, set new attrib size, replay copied vertices.
   * This is called when we transition from a small vertex attribute size
   * to a larger one.  Ex: glTexCoord2f -> glTexCoord4f.
   * We need to go back over the previous 2-component texcoords and insert
   * zero and one values.
   * \param attr  VBO_ATTRIB_x vertex attribute value
   */
   static void
   vbo_exec_wrap_upgrade_vertex(struct vbo_exec_context *exec,
         {
      struct gl_context *ctx = gl_context_from_vbo_exec(exec);
   struct vbo_context *vbo = vbo_context(ctx);
   const GLint lastcount = exec->vtx.vert_count;
   fi_type *old_attrptr[VBO_ATTRIB_MAX];
   const GLuint old_vtx_size_no_pos = exec->vtx.vertex_size_no_pos;
   const GLuint old_vtx_size = exec->vtx.vertex_size; /* floats per vertex */
   const GLuint oldSize = exec->vtx.attr[attr].size;
                     if (unlikely(!exec->vtx.buffer_ptr)) {
      /* We should only hit this when use_buffer_objects=true */
   assert(exec->vtx.bufferobj);
   vbo_exec_vtx_map(exec);
               /* Run pipeline on current vertices, copy wrapped vertices
   * to exec->vtx.copied.
   */
            if (unlikely(exec->vtx.copied.nr)) {
      /* We're in the middle of a primitive, keep the old vertex
   * format around to be able to translate the copied vertices to
   * the new format.
   */
               /* Heuristic: Attempt to isolate attributes received outside
   * begin/end so that they don't bloat the vertices.
   */
   if (!_mesa_inside_begin_end(ctx) &&
      !oldSize && lastcount > 8 && exec->vtx.vertex_size) {
   vbo_exec_copy_to_current(exec);
               /* Fix up sizes:
   */
   exec->vtx.attr[attr].size = newSize;
   exec->vtx.attr[attr].active_size = newSize;
   exec->vtx.attr[attr].type = newType;
   exec->vtx.vertex_size += newSize - oldSize;
   exec->vtx.vertex_size_no_pos = exec->vtx.vertex_size - exec->vtx.attr[0].size;
   exec->vtx.max_vert = vbo_compute_max_verts(exec);
   exec->vtx.vert_count = 0;
   exec->vtx.buffer_ptr = exec->vtx.buffer_map;
            if (attr != 0) {
      if (unlikely(oldSize)) {
               /* If there are attribs after the resized attrib... */
   if (offset + oldSize < old_vtx_size_no_pos) {
      int size_diff = newSize - oldSize;
   fi_type *old_first = exec->vtx.attrptr[attr] + oldSize;
   fi_type *new_first = exec->vtx.attrptr[attr] + newSize;
                  if (size_diff < 0) {
      /* Decreasing the size: Copy from first to last to move
   * elements to the left.
   */
                        do {
            } else {
      /* Increasing the size: Copy from last to first to move
   * elements to the right.
   */
                        do {
                     /* Update pointers to attribs, because we moved them. */
   GLbitfield64 enabled = exec->vtx.enabled &
                                 if (exec->vtx.attrptr[i] > exec->vtx.attrptr[attr])
            } else {
      /* Just have to append the new attribute at the end */
   exec->vtx.attrptr[attr] = exec->vtx.vertex +
                  /* The position is always last. */
            /* Replay stored vertices to translate them
   * to new format here.
   *
   * -- No need to replay - just copy piecewise
   */
   if (unlikely(exec->vtx.copied.nr)) {
      fi_type *data = exec->vtx.copied.buffer;
                     for (i = 0 ; i < exec->vtx.copied.nr ; i++) {
      GLbitfield64 enabled = exec->vtx.enabled;
   while (enabled) {
      const int j = u_bit_scan64(&enabled);
   GLuint sz = exec->vtx.attr[j].size;
                           if (j == attr) {
      if (oldSize) {
      fi_type tmp[4];
   COPY_CLEAN_4V_TYPE_AS_UNION(tmp, oldSize,
                  } else {
      fi_type *current = (fi_type *)vbo->current[j].Ptr;
         }
   else {
                     data += old_vtx_size;
               exec->vtx.buffer_ptr = dest;
   exec->vtx.vert_count += exec->vtx.copied.nr;
         }
         /**
   * This is when a vertex attribute transitions to a different size.
   * For example, we saw a bunch of glTexCoord2f() calls and now we got a
   * glTexCoord4f() call.  We promote the array from size=2 to size=4.
   * \param newSize  size of new vertex (number of 32-bit words).
   * \param attr  VBO_ATTRIB_x vertex attribute value
   */
   static void
   vbo_exec_fixup_vertex(struct gl_context *ctx, GLuint attr,
         {
                        if (newSize > exec->vtx.attr[attr].size ||
      newType != exec->vtx.attr[attr].type) {
   /* New size is larger.  Need to flush existing vertices and get
   * an enlarged vertex format.
   */
      }
   else if (newSize < exec->vtx.attr[attr].active_size) {
      GLuint i;
   const fi_type *id =
            /* New size is smaller - just need to fill in some
   * zeros.  Don't need to flush or wrap.
   */
   for (i = newSize; i <= exec->vtx.attr[attr].size; i++)
                  }
         /**
   * If index=0, does glVertexAttrib*() alias glVertex() to emit a vertex?
   * It depends on a few things, including whether we're inside or outside
   * of glBegin/glEnd.
   */
   static inline bool
   is_vertex_position(const struct gl_context *ctx, GLuint index)
   {
      return (index == 0 &&
            }
      /* Write a 64-bit value into a 32-bit pointer by preserving endianness. */
   #if UTIL_ARCH_LITTLE_ENDIAN
      #define SET_64BIT(dst32, u64) do { \
         *(dst32)++ = (u64); \
      #else
      #define SET_64BIT(dst32, u64) do { \
         *(dst32)++ = (uint64_t)(u64) >> 32; \
      #endif
         /**
   * This macro is used to implement all the glVertex, glColor, glTexCoord,
   * glVertexAttrib, etc functions.
   * \param A  VBO_ATTRIB_x attribute index
   * \param N  attribute size (1..4)
   * \param T  type (GL_FLOAT, GL_DOUBLE, GL_INT, GL_UNSIGNED_INT)
   * \param C  cast type (uint32_t or uint64_t)
   * \param V0, V1, v2, V3  attribute value
   */
   #define ATTR_UNION_BASE(A, N, T, C, V0, V1, V2, V3)                     \
   do {                                                                    \
      struct vbo_exec_context *exec = &vbo_context(ctx)->exec;             \
   int sz = (sizeof(C) / sizeof(GLfloat));                              \
         assert(sz == 1 || sz == 2);                                          \
   /* store a copy of the attribute in exec except for glVertex */      \
   if ((A) != 0) {                                                      \
      /* Check if attribute size or type is changing. */                \
   if (unlikely(exec->vtx.attr[A].active_size != N * sz ||           \
               }                                                                 \
         C *dest = (C *)exec->vtx.attrptr[A];                              \
   if (N>0) dest[0] = V0;                                            \
   if (N>1) dest[1] = V1;                                            \
   if (N>2) dest[2] = V2;                                            \
   if (N>3) dest[3] = V3;                                            \
   assert(exec->vtx.attr[A].type == T);                              \
         /* we now have accumulated a per-vertex attribute */              \
      } else {                                                             \
      /* This is a glVertex call */                                     \
   int size = exec->vtx.attr[0].size;                                \
         /* Check if attribute size or type is changing. */                \
   if (unlikely(size < N * sz ||                                     \
               }                                                                 \
         uint32_t *dst = (uint32_t *)exec->vtx.buffer_ptr;                 \
   uint32_t *src = (uint32_t *)exec->vtx.vertex;                     \
   unsigned vertex_size_no_pos = exec->vtx.vertex_size_no_pos;       \
         /* Copy over attributes from exec. */                             \
   for (unsigned i = 0; i < vertex_size_no_pos; i++)                 \
      *dst++ = *src++;                                               \
      /* Store the position, which is always last and can have 32 or */ \
   /* 64 bits per channel. */                                        \
   if (sizeof(C) == 4) {                                             \
      if (N > 0) *dst++ = V0;                                        \
   if (N > 1) *dst++ = V1;                                        \
   if (N > 2) *dst++ = V2;                                        \
   if (N > 3) *dst++ = V3;                                        \
         if (unlikely(N < size)) {                                      \
      if (N < 2 && size >= 2) *dst++ = V1;                        \
   if (N < 3 && size >= 3) *dst++ = V2;                        \
         } else {                                                          \
      /* 64 bits: dst can be unaligned, so copy each 4-byte word */  \
   /* separately */                                               \
   if (N > 0) SET_64BIT(dst, V0);                                 \
   if (N > 1) SET_64BIT(dst, V1);                                 \
   if (N > 2) SET_64BIT(dst, V2);                                 \
   if (N > 3) SET_64BIT(dst, V3);                                 \
         if (unlikely(N * 2 < size)) {                                  \
      if (N < 2 && size >= 4) SET_64BIT(dst, V1);                 \
   if (N < 3 && size >= 6) SET_64BIT(dst, V2);                 \
         }                                                                 \
         /* dst now points at the beginning of the next vertex */          \
   exec->vtx.buffer_ptr = (fi_type*)dst;                             \
         /* Don't set FLUSH_UPDATE_CURRENT because */                      \
   /* Current.Attrib[VBO_ATTRIB_POS] is never used. */               \
         if (unlikely(++exec->vtx.vert_count >= exec->vtx.max_vert))       \
         } while (0)
      #undef ERROR
   #define ERROR(err) _mesa_error(ctx, err, __func__)
   #define TAG(x) _mesa_##x
   #define SUPPRESS_STATIC
      #define ATTR_UNION(A, N, T, C, V0, V1, V2, V3) \
            #include "vbo_attrib_tmp.h"
         /**
   * Execute a glMaterial call.  Note that if GL_COLOR_MATERIAL is enabled,
   * this may be a (partial) no-op.
   */
   void GLAPIENTRY
   _mesa_Materialfv(GLenum face, GLenum pname, const GLfloat *params)
   {
      GLbitfield updateMats;
            /* This function should be a no-op when it tries to update material
   * attributes which are currently tracking glColor via glColorMaterial.
   * The updateMats var will be a mask of the MAT_BIT_FRONT/BACK_x bits
   * indicating which material attributes can actually be updated below.
   */
   if (ctx->Light.ColorMaterialEnabled) {
         }
   else {
      /* GL_COLOR_MATERIAL is disabled so don't skip any material updates */
               if (_mesa_is_desktop_gl_compat(ctx) && face == GL_FRONT) {
         }
   else if (_mesa_is_desktop_gl_compat(ctx) && face == GL_BACK) {
         }
   else if (face != GL_FRONT_AND_BACK) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glMaterial(invalid face)");
               switch (pname) {
   case GL_EMISSION:
      if (updateMats & MAT_BIT_FRONT_EMISSION)
         if (updateMats & MAT_BIT_BACK_EMISSION)
            case GL_AMBIENT:
      if (updateMats & MAT_BIT_FRONT_AMBIENT)
         if (updateMats & MAT_BIT_BACK_AMBIENT)
            case GL_DIFFUSE:
      if (updateMats & MAT_BIT_FRONT_DIFFUSE)
         if (updateMats & MAT_BIT_BACK_DIFFUSE)
            case GL_SPECULAR:
      if (updateMats & MAT_BIT_FRONT_SPECULAR)
         if (updateMats & MAT_BIT_BACK_SPECULAR)
            case GL_SHININESS:
      if (*params < 0 || *params > ctx->Const.MaxShininess) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  }
   if (updateMats & MAT_BIT_FRONT_SHININESS)
         if (updateMats & MAT_BIT_BACK_SHININESS)
            case GL_COLOR_INDEXES:
      if (ctx->API != API_OPENGL_COMPAT) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glMaterialfv(pname)");
      }
   if (updateMats & MAT_BIT_FRONT_INDEXES)
         if (updateMats & MAT_BIT_BACK_INDEXES)
            case GL_AMBIENT_AND_DIFFUSE:
      if (updateMats & MAT_BIT_FRONT_AMBIENT)
         if (updateMats & MAT_BIT_FRONT_DIFFUSE)
         if (updateMats & MAT_BIT_BACK_AMBIENT)
         if (updateMats & MAT_BIT_BACK_DIFFUSE)
            default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glMaterialfv(pname)");
         }
         /**
   * Flush (draw) vertices.
   *
   * \param flags  bitmask of FLUSH_STORED_VERTICES, FLUSH_UPDATE_CURRENT
   */
   static void
   vbo_exec_FlushVertices_internal(struct vbo_exec_context *exec, unsigned flags)
   {
               if (flags & FLUSH_STORED_VERTICES) {
      if (exec->vtx.vert_count) {
                  if (exec->vtx.vertex_size) {
      vbo_exec_copy_to_current(exec);
               /* All done. */
      } else {
               /* Note that the vertex size is unchanged.
   * (vbo_reset_all_attr isn't called)
   */
            /* Only FLUSH_UPDATE_CURRENT is done. */
         }
         void GLAPIENTRY
   _mesa_EvalCoord1f(GLfloat u)
   {
      GET_CURRENT_CONTEXT(ctx);
            {
      GLint i;
   if (exec->eval.recalculate_maps)
            for (i = 0; i <= VBO_ATTRIB_TEX7; i++) {
      if (exec->eval.map1[i].map)
      if (exec->vtx.attr[i].active_size != exec->eval.map1[i].sz)
               memcpy(exec->vtx.copied.buffer, exec->vtx.vertex,
                     memcpy(exec->vtx.vertex, exec->vtx.copied.buffer,
      }
         void GLAPIENTRY
   _mesa_EvalCoord2f(GLfloat u, GLfloat v)
   {
      GET_CURRENT_CONTEXT(ctx);
            {
      GLint i;
   if (exec->eval.recalculate_maps)
            for (i = 0; i <= VBO_ATTRIB_TEX7; i++) {
      if (exec->eval.map2[i].map)
      if (exec->vtx.attr[i].active_size != exec->eval.map2[i].sz)
            if (ctx->Eval.AutoNormal)
      if (exec->vtx.attr[VBO_ATTRIB_NORMAL].active_size != 3)
            memcpy(exec->vtx.copied.buffer, exec->vtx.vertex,
                     memcpy(exec->vtx.vertex, exec->vtx.copied.buffer,
      }
         void GLAPIENTRY
   _mesa_EvalCoord1fv(const GLfloat *u)
   {
         }
         void GLAPIENTRY
   _mesa_EvalCoord2fv(const GLfloat *u)
   {
         }
         void GLAPIENTRY
   _mesa_EvalPoint1(GLint i)
   {
      GET_CURRENT_CONTEXT(ctx);
   GLfloat du = ((ctx->Eval.MapGrid1u2 - ctx->Eval.MapGrid1u1) /
                     }
         void GLAPIENTRY
   _mesa_EvalPoint2(GLint i, GLint j)
   {
      GET_CURRENT_CONTEXT(ctx);
   GLfloat du = ((ctx->Eval.MapGrid2u2 - ctx->Eval.MapGrid2u1) /
         GLfloat dv = ((ctx->Eval.MapGrid2v2 - ctx->Eval.MapGrid2v1) /
         GLfloat u = i * du + ctx->Eval.MapGrid2u1;
               }
         /**
   * Called via glBegin.
   */
   void GLAPIENTRY
   _mesa_Begin(GLenum mode)
   {
      GET_CURRENT_CONTEXT(ctx);
   struct vbo_context *vbo = vbo_context(ctx);
   struct vbo_exec_context *exec = &vbo->exec;
            if (_mesa_inside_begin_end(ctx)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glBegin");
               if (ctx->NewState)
            GLenum error = _mesa_valid_prim_mode(ctx, mode);
   if (error != GL_NO_ERROR) {
      _mesa_error(ctx, error, "glBegin");
               /* Heuristic: attempt to isolate attributes occurring outside
   * begin/end pairs.
   *
   * Use FLUSH_STORED_VERTICES, because it updates current attribs and
   * sets vertex_size to 0. (FLUSH_UPDATE_CURRENT doesn't change vertex_size)
   */
   if (exec->vtx.vertex_size && !exec->vtx.attr[VBO_ATTRIB_POS].size)
            i = exec->vtx.prim_count++;
   exec->vtx.mode[i] = mode;
   exec->vtx.draw[i].start = exec->vtx.vert_count;
                     ctx->Dispatch.Exec = _mesa_hw_select_enabled(ctx) ?
            /* We may have been called from a display list, in which case we should
   * leave dlist.c's dispatch table in place.
   */
   if (ctx->GLThread.enabled) {
      if (ctx->Dispatch.Current == ctx->Dispatch.OutsideBeginEnd)
      } else if (ctx->GLApi == ctx->Dispatch.OutsideBeginEnd) {
      ctx->GLApi = ctx->Dispatch.Current = ctx->Dispatch.Exec;
      } else {
            }
         /**
   * Try to merge / concatenate the two most recent VBO primitives.
   */
   static void
   try_vbo_merge(struct vbo_exec_context *exec)
   {
                                 if (exec->vtx.prim_count >= 2) {
      struct gl_context *ctx = gl_context_from_vbo_exec(exec);
            if (vbo_merge_draws(ctx, false,
                     exec->vtx.mode[prev],
   exec->vtx.mode[cur],
   exec->vtx.draw[prev].start,
   exec->vtx.draw[cur].start,
   &exec->vtx.draw[prev].count,
   exec->vtx.draw[cur].count,
   0, 0,
         }
         /**
   * Called via glEnd.
   */
   void GLAPIENTRY
   _mesa_End(void)
   {
      GET_CURRENT_CONTEXT(ctx);
            if (!_mesa_inside_begin_end(ctx)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glEnd");
                        if (ctx->GLThread.enabled) {
      if (ctx->Dispatch.Current == ctx->Dispatch.BeginEnd ||
      ctx->Dispatch.Current == ctx->Dispatch.HWSelectModeBeginEnd) {
         } else if (ctx->GLApi == ctx->Dispatch.BeginEnd ||
            ctx->GLApi = ctx->Dispatch.Current = ctx->Dispatch.Exec;
               if (exec->vtx.prim_count > 0) {
      /* close off current primitive */
   unsigned last = exec->vtx.prim_count - 1;
   struct pipe_draw_start_count_bias *last_draw = &exec->vtx.draw[last];
            last_draw->count = count;
            if (count) {
      /* mark result buffer used */
                              /* Special handling for GL_LINE_LOOP */
   bool driver_supports_lineloop =
         if (exec->vtx.mode[last] == GL_LINE_LOOP &&
      (exec->vtx.markers[last].begin == 0 || !driver_supports_lineloop)) {
   /* We're finishing drawing a line loop.  Append 0th vertex onto
   * end of vertex buffer so we can draw it as a line strip.
   */
   const fi_type *src = exec->vtx.buffer_map +
                                                                     /* Increment the vertex count so the next primitive doesn't
   * overwrite the last vertex which we just added.
   */
                  if (!driver_supports_lineloop)
                                    if (exec->vtx.prim_count == VBO_MAX_PRIM)
            if (MESA_DEBUG_FLAGS & DEBUG_ALWAYS_FLUSH) {
            }
         /**
   * Called via glPrimitiveRestartNV()
   */
   void GLAPIENTRY
   _mesa_PrimitiveRestartNV(void)
   {
      GLenum curPrim;
                     if (curPrim == PRIM_OUTSIDE_BEGIN_END) {
         }
   else {
      _mesa_End();
         }
         /**
   * A special version of glVertexAttrib4f that does not treat index 0 as
   * VBO_ATTRIB_POS.
   */
   static void
   VertexAttrib4f_nopos(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
   {
      GET_CURRENT_CONTEXT(ctx);
   if (index < ctx->Const.Program[MESA_SHADER_VERTEX].MaxAttribs)
         else
      }
      static void GLAPIENTRY
   _es_VertexAttrib4fARB(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
   {
         }
         static void GLAPIENTRY
   _es_VertexAttrib1fARB(GLuint indx, GLfloat x)
   {
         }
         static void GLAPIENTRY
   _es_VertexAttrib1fvARB(GLuint indx, const GLfloat* values)
   {
         }
         static void GLAPIENTRY
   _es_VertexAttrib2fARB(GLuint indx, GLfloat x, GLfloat y)
   {
         }
         static void GLAPIENTRY
   _es_VertexAttrib2fvARB(GLuint indx, const GLfloat* values)
   {
         }
         static void GLAPIENTRY
   _es_VertexAttrib3fARB(GLuint indx, GLfloat x, GLfloat y, GLfloat z)
   {
         }
         static void GLAPIENTRY
   _es_VertexAttrib3fvARB(GLuint indx, const GLfloat* values)
   {
         }
         static void GLAPIENTRY
   _es_VertexAttrib4fvARB(GLuint indx, const GLfloat* values)
   {
         }
         void
   vbo_init_dispatch_begin_end(struct gl_context *ctx)
   {
   #define NAME_AE(x) _mesa_##x
   #define NAME_CALLLIST(x) _mesa_##x
   #define NAME(x) _mesa_##x
   #define NAME_ES(x) _es_##x
         struct _glapi_table *tab = ctx->Dispatch.OutsideBeginEnd;
            if (ctx->Dispatch.BeginEnd) {
      tab = ctx->Dispatch.BeginEnd;
         }
         static void
   vbo_reset_all_attr(struct vbo_exec_context *exec)
   {
      while (exec->vtx.enabled) {
               /* Reset the vertex attribute by setting its size to zero. */
   exec->vtx.attr[i].size = 0;
   exec->vtx.attr[i].type = GL_FLOAT;
   exec->vtx.attr[i].active_size = 0;
                  }
         void
   vbo_exec_vtx_init(struct vbo_exec_context *exec)
   {
                        exec->vtx.enabled = u_bit_consecutive64(0, VBO_ATTRIB_MAX); /* reset all */
            exec->vtx.info.instance_count = 1;
      }
         void
   vbo_exec_vtx_destroy(struct vbo_exec_context *exec)
   {
      /* using a real VBO for vertex data */
            /* True VBOs should already be unmapped
   */
   if (exec->vtx.buffer_map) {
      assert(!exec->vtx.bufferobj ||
         if (!exec->vtx.bufferobj) {
      align_free(exec->vtx.buffer_map);
   exec->vtx.buffer_map = NULL;
                  /* Free the vertex buffer.  Unmap first if needed.
   */
   if (exec->vtx.bufferobj &&
      _mesa_bufferobj_mapped(exec->vtx.bufferobj, MAP_INTERNAL)) {
      }
      }
         /**
   * If inside glBegin()/glEnd(), it should assert(0).  Otherwise, if
   * FLUSH_STORED_VERTICES bit in \p flags is set flushes any buffered
   * vertices, if FLUSH_UPDATE_CURRENT bit is set updates
   * __struct gl_contextRec::Current and gl_light_attrib::Material
   *
   * Note that the default T&L engine never clears the
   * FLUSH_UPDATE_CURRENT bit, even after performing the update.
   *
   * \param flags  bitmask of FLUSH_STORED_VERTICES, FLUSH_UPDATE_CURRENT
   */
   void
   vbo_exec_FlushVertices(struct gl_context *ctx, GLuint flags)
   {
            #ifndef NDEBUG
      /* debug check: make sure we don't get called recursively */
   exec->flush_call_depth++;
      #endif
         if (_mesa_inside_begin_end(ctx)) {
      #ifndef NDEBUG
         exec->flush_call_depth--;
   #endif
                     /* Flush (draw). */
         #ifndef NDEBUG
      exec->flush_call_depth--;
      #endif
   }
         void GLAPIENTRY
   _es_Color4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
   {
         }
         void GLAPIENTRY
   _es_Normal3f(GLfloat x, GLfloat y, GLfloat z)
   {
         }
         void GLAPIENTRY
   _es_MultiTexCoord4f(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q)
   {
         }
         void GLAPIENTRY
   _es_Materialfv(GLenum face, GLenum pname, const GLfloat *params)
   {
         }
         void GLAPIENTRY
   _es_Materialf(GLenum face, GLenum pname, GLfloat param)
   {
      GLfloat p[4];
   p[0] = param;
   p[1] = p[2] = p[3] = 0.0F;
      }
      #undef TAG
   #undef SUPPRESS_STATIC
   #define TAG(x) _hw_select_##x
   /* filter out none vertex api */
   #define HW_SELECT_MODE
      #undef ATTR_UNION
   #define ATTR_UNION(A, N, T, C, V0, V1, V2, V3)     \
      do {                                            \
      if ((A) == 0) {                              \
      ATTR_UNION_BASE(VBO_ATTRIB_SELECT_RESULT_OFFSET, 1, GL_UNSIGNED_INT, uint32_t, \
      }                                            \
            #include "vbo_attrib_tmp.h"
      void
   vbo_init_dispatch_hw_select_begin_end(struct gl_context *ctx)
   {
      int numEntries = MAX2(_gloffset_COUNT, _glapi_get_dispatch_table_size());
         #undef NAME
   #define NAME(x) _hw_select_##x
      struct _glapi_table *tab = ctx->Dispatch.HWSelectModeBeginEnd;
      }
