      /**************************************************************************
   *
   * Copyright 2007 VMware, Inc.
   * Copyright 2012 Marek Olšák <maraeo@gmail.com>
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
   * IN NO EVENT SHALL AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      /*
   * This converts the VBO's vertex attribute/array information into
   * Gallium vertex state and binds it.
   *
   * Authors:
   *   Keith Whitwell <keithw@vmware.com>
   *   Marek Olšák <maraeo@gmail.com>
   */
      #include "st_context.h"
   #include "st_atom.h"
   #include "st_draw.h"
   #include "st_program.h"
      #include "cso_cache/cso_context.h"
   #include "util/u_math.h"
   #include "util/u_upload_mgr.h"
   #include "main/bufferobj.h"
   #include "main/glformats.h"
   #include "main/varray.h"
   #include "main/arrayobj.h"
      enum st_update_flag {
      UPDATE_ALL,
      };
      /* Always inline the non-64bit element code, so that the compiler can see
   * that velements is on the stack.
   */
   static void ALWAYS_INLINE
   init_velement(struct pipe_vertex_element *velements,
               const struct gl_vertex_format *vformat,
   int src_offset, unsigned src_stride,
   {
      velements[idx].src_offset = src_offset;
   velements[idx].src_stride = src_stride;
   velements[idx].src_format = vformat->_PipeFormat;
   velements[idx].instance_divisor = instance_divisor;
   velements[idx].vertex_buffer_index = vbo_index;
   velements[idx].dual_slot = dual_slot;
      }
      /* ALWAYS_INLINE helps the compiler realize that most of the parameters are
   * on the stack.
   */
   template<util_popcnt POPCNT, st_update_flag UPDATE> void ALWAYS_INLINE
   setup_arrays(struct st_context *st,
               const struct gl_vertex_array_object *vao,
   const GLbitfield dual_slot_inputs,
   const GLbitfield inputs_read,
   const GLbitfield enabled_attribs,
   {
               /* Process attribute array data. */
            if (vao->IsDynamic) {
      while (mask) {
      const gl_vert_attrib attr = (gl_vert_attrib)u_bit_scan(&mask);
   const struct gl_array_attributes *const attrib =
         const struct gl_vertex_buffer_binding *const binding =
                  /* Set the vertex buffer. */
   if (binding->BufferObj) {
      vbuffer[bufidx].buffer.resource =
         vbuffer[bufidx].is_user_buffer = false;
   vbuffer[bufidx].buffer_offset = binding->Offset +
      } else {
      vbuffer[bufidx].buffer.user = attrib->Ptr;
   vbuffer[bufidx].is_user_buffer = true;
                              /* Set the vertex element. */
   init_velement(velements->velems, &attrib->Format, 0,
               binding->Stride,
      }
               while (mask) {
      /* The attribute index to start pulling a binding */
   const gl_vert_attrib i = (gl_vert_attrib)(ffs(mask) - 1);
   const struct gl_vertex_buffer_binding *const binding
                  if (binding->BufferObj) {
      /* Set the binding */
   vbuffer[bufidx].buffer.resource =
         vbuffer[bufidx].is_user_buffer = false;
      } else {
      /* Set the binding */
   const void *ptr = (const void *)_mesa_draw_binding_offset(binding);
   vbuffer[bufidx].buffer.user = ptr;
   vbuffer[bufidx].is_user_buffer = true;
               const GLbitfield boundmask = _mesa_draw_bound_attrib_bits(binding);
   GLbitfield attrmask = mask & boundmask;
   /* Mark the those attributes as processed */
   mask &= ~boundmask;
   /* We can assume that we have array for the binding */
            if (UPDATE == UPDATE_BUFFERS_ONLY)
            /* Walk attributes belonging to the binding */
   do {
      const gl_vert_attrib attr = (gl_vert_attrib)u_bit_scan(&attrmask);
   const struct gl_array_attributes *const attrib
         const GLuint off = _mesa_draw_attributes_relative_offset(attrib);
   init_velement(velements->velems, &attrib->Format, off,
                        }
      /* Only used by the select/feedback mode. */
   void
   st_setup_arrays(struct st_context *st,
                  const struct gl_vertex_program *vp,
      {
      struct gl_context *ctx = st->ctx;
            setup_arrays<POPCNT_NO, UPDATE_ALL>
      (st, ctx->Array._DrawVAO, vp->Base.DualSlotInputs,
   vp_variant->vert_attrib_mask, enabled_attribs,
   }
      /* ALWAYS_INLINE helps the compiler realize that most of the parameters are
   * on the stack.
   *
   * Return the index of the vertex buffer where current attribs have been
   * uploaded.
   */
   template<util_popcnt POPCNT, st_update_flag UPDATE> void ALWAYS_INLINE
   st_setup_current(struct st_context *st,
                  const GLbitfield inputs_read,
   const GLbitfield dual_slot_inputs,
      {
               /* Process values that should have better been uniforms in the application */
   GLbitfield curmask = inputs_read & ~enabled_attribs;
   if (curmask) {
      unsigned num_attribs = util_bitcount_fast<POPCNT>(curmask);
   unsigned num_dual_attribs = util_bitcount_fast<POPCNT>(curmask &
         /* num_attribs includes num_dual_attribs, so adding num_dual_attribs
   * doubles the size of those attribs.
   */
            const unsigned bufidx = (*num_vbuffers)++;
   vbuffer[bufidx].is_user_buffer = false;
   vbuffer[bufidx].buffer.resource = NULL;
            /* Use const_uploader for zero-stride vertex attributes, because
   * it may use a better memory placement than stream_uploader.
   * The reason is that zero-stride attributes can be fetched many
   * times (thousands of times), so a better placement is going to
   * perform better.
   */
   struct u_upload_mgr *uploader = st->can_bind_const_buffer_as_vertex ?
                        u_upload_alloc(uploader, 0, max_size, 16,
                        do {
      const gl_vert_attrib attr = (gl_vert_attrib)u_bit_scan(&curmask);
   const struct gl_array_attributes *const attrib
                  /* When the current attribs are set (e.g. via glColor3ub or
   * glVertexAttrib2s), they are always converted to float32 or int32
   * or dual slots being 2x int32, so they are always dword-aligned.
   * glBegin/End behaves in the same way. It's really an internal Mesa
   * inefficiency that is convenient here, which is why this assertion
   * is always true.
   */
                  if (UPDATE == UPDATE_ALL) {
      init_velement(velements->velems, &attrib->Format, cursor - ptr,
                                 /* Always unmap. The uploader might use explicit flushes. */
         }
      /* Only used by the select/feedback mode. */
   void
   st_setup_current_user(struct st_context *st,
                           {
      struct gl_context *ctx = st->ctx;
   const GLbitfield enabled_attribs = _mesa_get_enabled_vertex_arrays(ctx);
   const GLbitfield inputs_read = vp_variant->vert_attrib_mask;
            /* Process values that should have better been uniforms in the application */
   GLbitfield curmask = inputs_read & ~enabled_attribs;
   /* For each attribute, make an own user buffer binding. */
   while (curmask) {
      const gl_vert_attrib attr = (gl_vert_attrib)u_bit_scan(&curmask);
   const struct gl_array_attributes *const attrib
                  init_velement(velements->velems, &attrib->Format, 0, 0, 0,
                  vbuffer[bufidx].is_user_buffer = true;
   vbuffer[bufidx].buffer.user = attrib->Ptr;
         }
      template<util_popcnt POPCNT, st_update_flag UPDATE> void ALWAYS_INLINE
   st_update_array_templ(struct st_context *st,
                     {
               /* vertex program validation must be done before this */
   /* _NEW_PROGRAM, ST_NEW_VS_STATE */
   const struct gl_vertex_program *vp =
         const struct st_common_variant *vp_variant = st->vp_variant;
   const GLbitfield inputs_read = vp_variant->vert_attrib_mask;
   const GLbitfield dual_slot_inputs = vp->Base.DualSlotInputs;
   const GLbitfield userbuf_attribs = inputs_read & enabled_user_attribs;
            st->draw_needs_minmax_index =
            struct pipe_vertex_buffer vbuffer[PIPE_MAX_ATTRIBS];
   unsigned num_vbuffers = 0;
            /* ST_NEW_VERTEX_ARRAYS */
   /* Setup arrays */
   setup_arrays<POPCNT, UPDATE>
      (st, ctx->Array._DrawVAO, dual_slot_inputs, inputs_read,
         /* _NEW_CURRENT_ATTRIB */
   /* Setup zero-stride attribs. */
   st_setup_current<POPCNT, UPDATE>(st, inputs_read, dual_slot_inputs,
                  unsigned unbind_trailing_vbuffers =
      st->last_num_vbuffers > num_vbuffers ?
                        if (UPDATE == UPDATE_ALL) {
               /* Set vertex buffers and elements. */
   cso_set_vertex_buffers_and_elements(cso, &velements,
                                 /* The driver should clear this after it has processed the update. */
   ctx->Array.NewVertexElements = false;
      } else {
      /* Only vertex buffers. */
   cso_set_vertex_buffers(cso, num_vbuffers, unbind_trailing_vbuffers,
         /* This can change only when we update vertex elements. */
         }
      template<util_popcnt POPCNT> void ALWAYS_INLINE
   st_update_array_impl(struct st_context *st)
   {
      struct gl_context *ctx = st->ctx;
   struct gl_vertex_array_object *vao = ctx->Array._DrawVAO;
   const GLbitfield enabled_attribs = _mesa_get_enabled_vertex_arrays(ctx);
   GLbitfield enabled_user_attribs;
            assert(vao->_EnabledWithMapMode ==
            if (!vao->IsDynamic && !vao->SharedAndImmutable)
            _mesa_get_derived_vao_masks(ctx, enabled_attribs, &enabled_user_attribs,
            /* Changing from user to non-user buffers and vice versa can switch between
   * cso and u_vbuf, which means that we need to update vertex elements even
   * when they have not changed.
   */
   if (ctx->Array.NewVertexElements ||
      st->uses_user_vertex_buffers !=
   !!(st->vp_variant->vert_attrib_mask & enabled_user_attribs)) {
   st_update_array_templ<POPCNT, UPDATE_ALL>
      } else {
      st_update_array_templ<POPCNT, UPDATE_BUFFERS_ONLY>
         }
      void
   st_update_array(struct st_context *st)
   {
         }
      void
   st_update_array_with_popcnt(struct st_context *st)
   {
         }
      struct pipe_vertex_state *
   st_create_gallium_vertex_state(struct gl_context *ctx,
                     {
      struct st_context *st = st_context(ctx);
   const GLbitfield inputs_read = enabled_attribs;
   const GLbitfield dual_slot_inputs = 0; /* always zero */
   struct pipe_vertex_buffer vbuffer[PIPE_MAX_ATTRIBS];
   unsigned num_vbuffers = 0;
            setup_arrays<POPCNT_NO, UPDATE_ALL>(st, vao, dual_slot_inputs, inputs_read,
            if (num_vbuffers != 1) {
      assert(!"this should never happen with display lists");
                        struct pipe_screen *screen = st->screen;
   struct pipe_vertex_state *state =
      screen->create_vertex_state(screen, &vbuffer[0], velements.velems,
                           for (unsigned i = 0; i < num_vbuffers; i++)
            }
