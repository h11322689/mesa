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
   */
      /* Author:
   *    Keith Whitwell <keithw@vmware.com>
   */
      #include <stdbool.h>
   #include "main/arrayobj.h"
   #include "util/glheader.h"
   #include "main/bufferobj.h"
   #include "main/context.h"
   #include "main/enable.h"
   #include "main/mesa_private.h"
   #include "main/macros.h"
   #include "main/light.h"
   #include "main/state.h"
   #include "main/varray.h"
   #include "util/bitscan.h"
      #include "vbo_private.h"
      static void
   copy_vao(struct gl_context *ctx, const struct gl_vertex_array_object *vao,
               {
               mask &= vao->Enabled;
   while (mask) {
      const int i = u_bit_scan(&mask);
   const struct gl_array_attributes *attrib = &vao->VertexAttrib[i];
   unsigned current_index = shift + i;
   struct gl_array_attributes *currval = &vbo->current[current_index];
   const GLubyte size = attrib->Format.User.Size;
   const GLenum16 type = attrib->Format.User.Type;
   fi_type tmp[8];
            if (type == GL_DOUBLE ||
      type == GL_UNSIGNED_INT64_ARB) {
   dmul_shift = 1;
      } else {
                  if (memcmp(currval->Ptr, tmp, 4 * sizeof(GLfloat) << dmul_shift) != 0) {
                              /* The fixed-func vertex program uses this. */
   if (current_index == VBO_ATTRIB_MAT_FRONT_SHININESS ||
                                 ctx->NewState |= state;
               if (type != currval->Format.User.Type ||
      (size >> dmul_shift) != currval->Format.User.Size) {
   vbo_set_vertex_format(&currval->Format, size >> dmul_shift, type);
   /* The format changed. We need to update gallium vertex elements. */
   if (state == _NEW_CURRENT_ATTRIB)
                     }
      /**
   * After playback, copy everything but the position from the
   * last vertex to the saved state
   */
   static void
   playback_copy_to_current(struct gl_context *ctx,
         {
      if (!node->cold->current_data)
            fi_type *data = node->cold->current_data;
            /* Copy conventional attribs and generics except pos */
   copy_vao(ctx, node->cold->VAO[VP_MODE_SHADER], ~VERT_BIT_POS,
         /* Copy materials */
   copy_vao(ctx, node->cold->VAO[VP_MODE_FF], VERT_BIT_MAT_ALL,
                  if (color0_changed && ctx->Light.ColorMaterialEnabled) {
                  /* CurrentExecPrimitive
   */
   if (node->cold->prim_count) {
      const struct _mesa_prim *prim = &node->cold->prims[node->cold->prim_count - 1];
   if (prim->end)
         else
         }
         static void
   loopback_vertex_list(struct gl_context *ctx,
         {
      struct gl_buffer_object *bo = list->cold->VAO[0]->BufferBinding[0].BufferObj;
            /* Reuse BO mapping when possible to avoid costly mapping on every glCallList(). */
   if (_mesa_bufferobj_mapped(bo, MAP_INTERNAL)) {
      if (list->cold->bo_bytes_used <= bo->Mappings[MAP_INTERNAL].Length)
         else
               if (!buffer && list->cold->bo_bytes_used)
      buffer = _mesa_bufferobj_map_range(ctx, 0, list->cold->bo_bytes_used, GL_MAP_READ_BIT,
         /* TODO: in this case, we shouldn't create a bo at all and instead keep
   * the in-RAM buffer. */
            if (!ctx->Const.AllowMappedBuffersDuringExecution && buffer)
      }
         void
   vbo_save_playback_vertex_list_loopback(struct gl_context *ctx, void *data)
   {
      const struct vbo_save_vertex_list *node =
                     if (_mesa_inside_begin_end(ctx) && node->draw_begins) {
      /* Error: we're about to begin a new primitive but we're already
   * inside a glBegin/End pair.
   */
   _mesa_error(ctx, GL_INVALID_OPERATION,
            }
   /* Various degenerate cases: translate into immediate mode
   * calls rather than trying to execute in place.
   */
      }
      enum vbo_save_status {
      DONE,
      };
      static enum vbo_save_status
   vbo_save_playback_vertex_list_gallium(struct gl_context *ctx,
               {
      /* Don't use this if selection or feedback mode is enabled. st/mesa can't
   * handle it.
   */
   if (!ctx->Driver.DrawGalliumVertexState || ctx->RenderMode != GL_RENDER)
                     /* This sets which vertex arrays are enabled, which determines
   * which attribs have stride = 0 and whether edge flags are enabled.
   */
   const GLbitfield enabled = node->enabled_attribs[mode];
            if (ctx->NewState)
            /* Return precomputed GL errors such as invalid shaders. */
   if (!ctx->ValidPrimMask) {
      _mesa_error(ctx, ctx->DrawGLError, "glCallList");
               /* Use the slow path when there are vertex inputs without vertex
   * elements. This happens with zero-stride attribs and non-fixed-func
   * shaders.
   *
   * Dual-slot inputs are also unsupported because the higher slot is
   * always missing in vertex elements.
   *
   * TODO: Add support for zero-stride attribs.
   */
            if (vp->info.inputs_read & ~enabled || vp->DualSlotInputs)
            struct pipe_vertex_state *state = node->state[mode];
            info.mode = node->mode;
            if (node->ctx == ctx) {
      /* This mechanism allows passing references to the driver without
   * using atomics to increase the reference count.
   *
   * This private refcount can be decremented without atomics but only
   * one context (ctx above) can use this counter (so that it's only
   * used by 1 thread).
   *
   * This number is atomically added to reference.count at
   * initialization. If it's never used, the same number is atomically
   * subtracted from reference.count before destruction. If this number
   * is decremented, we can pass one reference to the driver without
   * touching reference.count with atomics. At destruction we only
   * subtract the number of references we have not returned. This can
   * possibly turn a million atomic increments into 1 add and 1 subtract
   * atomic op over the whole lifetime of an app.
   */
   int16_t * const private_refcount = (int16_t*)&node->private_refcount[mode];
            if (unlikely(*private_refcount == 0)) {
      /* pipe_vertex_state can be reused through util_vertex_state_cache,
   * and there can be many display lists over-incrementing this number,
   * causing it to overflow.
   *
   * Guess that the same state can never be used by N=500000 display
   * lists, so one display list can only increment it by
   * INT_MAX / N.
   */
   const int16_t add_refs = INT_MAX / 500000;
   p_atomic_add(&state->reference.count, add_refs);
               (*private_refcount)--;
               /* Set edge flags. */
            /* Fast path using a pre-built gallium vertex buffer state. */
   if (node->modes || node->num_draws > 1) {
      ctx->Driver.DrawGalliumVertexState(ctx, state, info,
                  } else if (node->num_draws) {
      ctx->Driver.DrawGalliumVertexState(ctx, state, info,
                     /* Restore edge flag state and ctx->VertexProgram._VaryingInputs. */
            if (copy_to_current)
            }
      /**
   * Execute the buffer and save copied verts.
   * This is called from the display list code when executing
   * a drawing command.
   */
   void
   vbo_save_playback_vertex_list(struct gl_context *ctx, void *data, bool copy_to_current)
   {
      const struct vbo_save_vertex_list *node =
                     if (_mesa_inside_begin_end(ctx) && node->draw_begins) {
      /* Error: we're about to begin a new primitive but we're already
   * inside a glBegin/End pair.
   */
   _mesa_error(ctx, GL_INVALID_OPERATION,
                     if (vbo_save_playback_vertex_list_gallium(ctx, node, copy_to_current) == DONE)
            /* Save the Draw VAO before we override it. */
   const gl_vertex_processing_mode mode = ctx->VertexProgram._VPMode;
   GLbitfield vao_filter = _vbo_get_vao_filter(mode);
   struct gl_vertex_array_object *old_vao;
            _mesa_save_and_set_draw_vao(ctx, node->cold->VAO[mode], vao_filter,
         _mesa_set_varying_vp_inputs(ctx, vao_filter &
            /* Need that at least one time. */
   if (ctx->NewState)
            /* Return precomputed GL errors such as invalid shaders. */
   if (!ctx->ValidPrimMask) {
      _mesa_restore_draw_vao(ctx, old_vao, old_vp_input_filter);
   _mesa_error(ctx, ctx->DrawGLError, "glCallList");
                                 if (node->modes) {
      ctx->Driver.DrawGalliumMultiMode(ctx, info,
                  } else if (node->num_draws == 1) {
         } else if (node->num_draws) {
      ctx->Driver.DrawGallium(ctx, info, 0, node->start_counts,
                        if (copy_to_current)
      }
