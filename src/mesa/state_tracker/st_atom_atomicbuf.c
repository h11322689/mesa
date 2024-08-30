   /**************************************************************************
   *
   * Copyright 2014 Ilia Mirkin. All Rights Reserved.
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
         #include "program/prog_parameter.h"
   #include "program/prog_print.h"
   #include "compiler/glsl/ir_uniform.h"
      #include "pipe/p_context.h"
   #include "pipe/p_defines.h"
   #include "util/u_inlines.h"
   #include "util/u_surface.h"
      #include "st_debug.h"
   #include "st_context.h"
   #include "st_atom.h"
   #include "st_program.h"
      static void
   st_binding_to_sb(struct gl_buffer_binding *binding,
               {
               if (st_obj && st_obj->buffer) {
   unsigned offset = 0;
   sb->buffer = st_obj->buffer;
   offset = binding->Offset % alignment;
   sb->buffer_offset = binding->Offset - offset;
            /* AutomaticSize is FALSE if the buffer was set with BindBufferRange.
      * Take the minimum just to be sure.
      if (!binding->AutomaticSize)
         } else {
   sb->buffer = NULL;
   sb->buffer_offset = 0;
   sb->buffer_size = 0;
      }
      static void
   st_bind_atomics(struct st_context *st, struct gl_program *prog,
         {
      unsigned i;
            if (!prog || !st->pipe->set_shader_buffers || st->has_hw_atomics)
            /* For !has_hw_atomics, the atomic counters have been rewritten to be above
   * the SSBOs used by the program.
   */
   unsigned buffer_base = prog->info.num_ssbos;
   unsigned used_bindings = 0;
   for (i = 0; i < prog->sh.data->NumAtomicBuffers; i++) {
      struct gl_active_atomic_buffer *atomic =
                  st_binding_to_sb(&st->ctx->AtomicBufferBindings[atomic->Binding], &sb,
            st->pipe->set_shader_buffers(st->pipe, shader_type,
            }
      }
      void
   st_bind_vs_atomics(struct st_context *st)
   {
      struct gl_program *prog =
               }
      void
   st_bind_fs_atomics(struct st_context *st)
   {
      struct gl_program *prog =
               }
      void
   st_bind_gs_atomics(struct st_context *st)
   {
      struct gl_program *prog =
               }
      void
   st_bind_tcs_atomics(struct st_context *st)
   {
      struct gl_program *prog =
               }
      void
   st_bind_tes_atomics(struct st_context *st)
   {
      struct gl_program *prog =
               }
      void
   st_bind_cs_atomics(struct st_context *st)
   {
      if (st->has_hw_atomics) {
      st_bind_hw_atomic_buffers(st);
      }
   struct gl_program *prog =
               }
      void
   st_bind_hw_atomic_buffers(struct st_context *st)
   {
      struct pipe_shader_buffer buffers[PIPE_MAX_HW_ATOMIC_BUFFERS];
            if (!st->has_hw_atomics)
            for (i = 0; i < st->ctx->Const.MaxAtomicBufferBindings; i++)
               }
