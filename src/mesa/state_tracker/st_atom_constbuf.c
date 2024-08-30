   /**************************************************************************
   *
   * Copyright 2007 VMware, Inc.
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
   * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      /*
   * Authors:
   *   Keith Whitwell <keithw@vmware.com>
   *   Brian Paul
   */
         #include "program/prog_parameter.h"
   #include "program/prog_print.h"
   #include "main/shaderapi.h"
   #include "pipe/p_context.h"
   #include "pipe/p_defines.h"
   #include "util/u_inlines.h"
   #include "util/u_upload_mgr.h"
   #include "cso_cache/cso_context.h"
      #include "main/bufferobj.h"
   #include "st_debug.h"
   #include "st_context.h"
   #include "st_atom.h"
   #include "st_atom_constbuf.h"
   #include "st_program.h"
      /* Unbinds the CB0 if it's not used by the current program to avoid leaving
   * dangling pointers to old (potentially deleted) shaders in the driver.
   */
   static void
   st_unbind_unused_cb0(struct st_context *st, enum pipe_shader_type shader_type)
   {
      if (st->state.constbuf0_enabled_shader_mask & (1 << shader_type)) {
               pipe->set_constant_buffer(pipe, shader_type, 0, false, NULL);
         }
      /**
   * Pass the given program parameters to the graphics pipe as a
   * constant buffer.
   */
   void
   st_upload_constants(struct st_context *st, struct gl_program *prog, gl_shader_stage stage)
   {
      enum pipe_shader_type shader_type = pipe_shader_type_from_mesa(stage);
   if (!prog) {
      st_unbind_unused_cb0(st, shader_type);
                        assert(shader_type == PIPE_SHADER_VERTEX ||
         shader_type == PIPE_SHADER_FRAGMENT ||
   shader_type == PIPE_SHADER_GEOMETRY ||
   shader_type == PIPE_SHADER_TESS_CTRL ||
            /* update the ATI constants before rendering */
   if (shader_type == PIPE_SHADER_FRAGMENT && prog->ati_fs) {
      struct ati_fragment_shader *ati_fs = prog->ati_fs;
            for (c = 0; c < MAX_NUM_FRAGMENT_CONSTANTS_ATI; c++) {
      unsigned offset = params->Parameters[c].ValueOffset;
   if (ati_fs->LocalConstDef & (1 << c))
      memcpy(params->ParameterValues + offset,
      else
      memcpy(params->ParameterValues + offset,
                     /* Make all bindless samplers/images bound texture/image units resident in
   * the context.
   */
   st_make_bound_samplers_resident(st, prog);
            /* update constants */
   if (params && params->NumParameters) {
      struct pipe_constant_buffer cb;
                     cb.buffer = NULL;
   cb.user_buffer = NULL;
   cb.buffer_offset = 0;
            if (st->prefer_real_buffer_in_constbuf0) {
                                    /* fetch_state always stores 4 components (16 bytes) per matrix row,
   * but matrix rows are sometimes allocated partially, so add 12
   * to compensate for the fetch_state defect.
   */
                  int uniform_bytes = params->UniformBytes;
                  /* Upload the constants which come from fixed-function state, such as
   * transformation matrices, fog factors, etc.
   */
                                 /* Set inlinable constants. This is more involved because state
   * parameters are uploaded directly above instead of being loaded
   * into gl_program_parameter_list. The easiest way to get their values
   * is to load them.
   */
   unsigned num_inlinable_uniforms = prog->info.num_inlinable_uniforms;
   if (num_inlinable_uniforms) {
      uint32_t values[MAX_INLINABLE_UNIFORMS];
                                    if (dw_offset * 4 >= uniform_bytes && !loaded_state_vars) {
                                    pipe->set_inlinable_constants(pipe, shader_type,
               } else {
                        /* Update the constants which come from fixed-function state, such as
   * transformation matrices, fog factors, etc.
   */
                           /* Set inlinable constants. */
   unsigned num_inlinable_uniforms = prog->info.num_inlinable_uniforms;
   if (num_inlinable_uniforms) {
                                    pipe->set_inlinable_constants(pipe, shader_type,
                           } else {
            }
         /**
   * Vertex shader:
   */
   void
   st_update_vs_constants(struct st_context *st)
   {
      st_upload_constants(st, st->ctx->VertexProgram._Current,
      }
      /**
   * Fragment shader:
   */
   void
   st_update_fs_constants(struct st_context *st)
   {
      st_upload_constants(st, st->ctx->FragmentProgram._Current,
      }
         /* Geometry shader:
   */
   void
   st_update_gs_constants(struct st_context *st)
   {
      st_upload_constants(st, st->ctx->GeometryProgram._Current,
      }
      /* Tessellation control shader:
   */
   void
   st_update_tcs_constants(struct st_context *st)
   {
      st_upload_constants(st, st->ctx->TessCtrlProgram._Current,
      }
      /* Tessellation evaluation shader:
   */
   void
   st_update_tes_constants(struct st_context *st)
   {
      st_upload_constants(st, st->ctx->TessEvalProgram._Current,
      }
      /* Compute shader:
   */
   void
   st_update_cs_constants(struct st_context *st)
   {
      st_upload_constants(st, st->ctx->ComputeProgram._Current,
      }
      static void
   st_bind_ubos(struct st_context *st, struct gl_program *prog,
         {
      unsigned i;
            if (!prog)
                     for (i = 0; i < prog->sh.NumUniformBlocks; i++) {
               binding =
                     if (cb.buffer) {
                     /* AutomaticSize is FALSE if the buffer was set with BindBufferRange.
   * Take the minimum just to be sure.
   */
   if (!binding->AutomaticSize)
      }
   else {
      cb.buffer_offset = 0;
                     }
      void
   st_bind_vs_ubos(struct st_context *st)
   {
      struct gl_program *prog =
               }
      void
   st_bind_fs_ubos(struct st_context *st)
   {
      struct gl_program *prog =
               }
      void
   st_bind_gs_ubos(struct st_context *st)
   {
      struct gl_program *prog =
               }
      void
   st_bind_tcs_ubos(struct st_context *st)
   {
      struct gl_program *prog =
               }
      void
   st_bind_tes_ubos(struct st_context *st)
   {
      struct gl_program *prog =
               }
      void
   st_bind_cs_ubos(struct st_context *st)
   {
      struct gl_program *prog =
               }
