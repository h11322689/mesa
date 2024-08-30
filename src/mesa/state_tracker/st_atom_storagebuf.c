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
   st_bind_ssbos(struct st_context *st, struct gl_program *prog,
         {
      unsigned i;
   struct pipe_shader_buffer buffers[MAX_SHADER_STORAGE_BUFFERS];
   if (!prog || !st->pipe->set_shader_buffers)
            for (i = 0; i < prog->info.num_ssbos; i++) {
      struct gl_buffer_binding *binding;
   struct gl_buffer_object *st_obj;
            binding = &st->ctx->ShaderStorageBufferBindings[
                           if (sb->buffer) {
                     /* AutomaticSize is FALSE if the buffer was set with BindBufferRange.
   * Take the minimum just to be sure.
   */
   if (!binding->AutomaticSize)
      }
   else {
      sb->buffer_offset = 0;
         }
   st->pipe->set_shader_buffers(st->pipe, shader_type, 0,
                  /* Clear out any stale shader buffers (or lowered atomic counters). */
   int num_ssbos = prog->info.num_ssbos;
   if (!st->has_hw_atomics)
         if (st->last_num_ssbos[shader_type] > num_ssbos) {
      st->pipe->set_shader_buffers(
         st->pipe, shader_type,
   num_ssbos,
   st->last_num_ssbos[shader_type] - num_ssbos,
         }
      void st_bind_vs_ssbos(struct st_context *st)
   {
      struct gl_program *prog =
               }
      void st_bind_fs_ssbos(struct st_context *st)
   {
      struct gl_program *prog =
               }
      void st_bind_gs_ssbos(struct st_context *st)
   {
      struct gl_program *prog =
               }
      void st_bind_tcs_ssbos(struct st_context *st)
   {
      struct gl_program *prog =
               }
      void st_bind_tes_ssbos(struct st_context *st)
   {
      struct gl_program *prog =
               }
      void st_bind_cs_ssbos(struct st_context *st)
   {
      struct gl_program *prog =
               }
