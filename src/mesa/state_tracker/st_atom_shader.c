   /**************************************************************************
   *
   * Copyright 2003 VMware, Inc.
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
      /**
   * State validation for vertex/fragment shaders.
   * Note that we have to delay most vertex/fragment shader translation
   * until rendering time since the linkage between the vertex outputs and
   * fragment inputs can vary depending on the pairing of shaders.
   *
   * Authors:
   *   Brian Paul
   */
         #include "main/mtypes.h"
   #include "main/framebuffer.h"
   #include "main/state.h"
   #include "main/texobj.h"
   #include "main/teximage.h"
   #include "main/texstate.h"
   #include "program/program.h"
      #include "pipe/p_context.h"
   #include "pipe/p_shader_tokens.h"
   #include "util/u_simple_shaders.h"
   #include "cso_cache/cso_context.h"
   #include "util/u_debug.h"
      #include "st_context.h"
   #include "st_atom.h"
   #include "st_program.h"
   #include "st_texture.h"
   #include "st_util.h"
         static unsigned
   get_texture_index(struct gl_context *ctx, const unsigned unit)
   {
      struct gl_texture_object *texObj = _mesa_get_tex_unit(ctx, unit)->_Current;
            if (texObj) {
         } else {
      /* fallback for missing texture */
                  }
      static void
   update_gl_clamp(struct st_context *st, struct gl_program *prog, uint32_t *gl_clamp)
   {
      if (!st->emulate_gl_clamp)
            if (!st->ctx->Texture.NumSamplersWithClamp)
            gl_clamp[0] = gl_clamp[1] = gl_clamp[2] = 0;
   GLbitfield samplers_used = prog->SamplersUsed;
   unsigned unit;
   /* same as st_atom_sampler.c */
   for (unit = 0; samplers_used; unit++, samplers_used >>= 1) {
      unsigned tex_unit = prog->SamplerUnits[unit];
   if (samplers_used & 1 &&
      (st->ctx->Texture.Unit[tex_unit]._Current->Target != GL_TEXTURE_BUFFER)) {
   ASSERTED const struct gl_texture_object *texobj;
                                 msamp = _mesa_get_samplerobj(ctx, tex_unit);
   if (is_wrap_gl_clamp(msamp->Attrib.WrapS))
         if (is_wrap_gl_clamp(msamp->Attrib.WrapT))
         if (is_wrap_gl_clamp(msamp->Attrib.WrapR))
            }
      /**
   * Update fragment program state/atom.  This involves translating the
   * Mesa fragment program into a gallium fragment program and binding it.
   */
   void
   st_update_fp( struct st_context *st )
   {
               assert(st->ctx->FragmentProgram._Current);
   fp = st->ctx->FragmentProgram._Current;
                     if (st->shader_has_one_variant[MESA_SHADER_FRAGMENT] &&
      !fp->ati_fs && /* ATI_fragment_shader always has multiple variants */
   !fp->ExternalSamplersUsed && /* external samplers need variants */
   !(!fp->shader_program && fp->ShadowSamplers)) {
      } else {
               /* use memset, not an initializer to be sure all memory is zeroed */
                     key.lower_flatshade = st->lower_flatshade &&
            /* _NEW_COLOR */
   key.lower_alpha_func = COMPARE_FUNC_ALWAYS;
   if (st->lower_alpha_test && _mesa_is_alpha_test_enabled(st->ctx))
            /* _NEW_LIGHT_STATE | _NEW_PROGRAM */
   key.lower_two_sided_color = st->lower_two_sided_color &&
            /* gl_driver_flags::NewFragClamp */
   key.clamp_color = st->clamp_frag_color_in_shader &&
            /* _NEW_MULTISAMPLE | _NEW_BUFFERS */
   key.persample_shading =
      st->force_persample_in_shader &&
   _mesa_is_multisample_enabled(st->ctx) &&
   st->ctx->Multisample.SampleShading &&
               if (fp->ati_fs) {
               for (unsigned u = 0; u < MAX_NUM_FRAGMENT_REGISTERS_ATI; u++) {
                     if (!fp->shader_program && fp->ShadowSamplers) {
      u_foreach_bit(i, fp->ShadowSamplers) {
      struct gl_texture_object *tex_obj =
                  if (baseFormat == GL_DEPTH_COMPONENT ||
      baseFormat == GL_DEPTH_STENCIL)
               key.external = st_get_external_sampler_key(st, fp);
            simple_mtx_lock(&st->ctx->Shared->Mutex);
   shader = st_get_fp_variant(st, fp, &key)->base.driver_shader;
                           }
         /**
   * Update vertex program state/atom.  This involves translating the
   * Mesa vertex program into a gallium fragment program and binding it.
   */
   void
   st_update_vp( struct st_context *st )
   {
               /* find active shader and params -- Should be covered by
   * ST_NEW_VERTEX_PROGRAM
   */
   assert(st->ctx->VertexProgram._Current);
   vp = st->ctx->VertexProgram._Current;
            if (st->shader_has_one_variant[MESA_SHADER_VERTEX] &&
      !st->ctx->Array._PerVertexEdgeFlagsEnabled) {
      } else {
                                 /* When this is true, we will add an extra input to the vertex
   * shader translation (for edgeflags), an extra output with
   * edgeflag semantics, and extend the vertex shader to pass through
   * the input to the output.  We'll need to use similar logic to set
   * up the extra vertex_element input for edgeflags.
   */
            key.clamp_color = st->clamp_vert_color_in_shader &&
                     st->ctx->Light._ClampVertexColor &&
   (vp->info.outputs_written &
            if (!st->ctx->GeometryProgram._Current &&
      !st->ctx->TessEvalProgram._Current) {
   /* _NEW_POINT */
   if (st->lower_point_size)
         /* _NEW_TRANSFORM */
   if (st->lower_ucp && st_user_clip_planes_enabled(st->ctx))
                        simple_mtx_lock(&st->ctx->Shared->Mutex);
   st->vp_variant = st_get_common_variant(st, vp, &key);
                        cso_set_vertex_shader_handle(st->cso_context,
      }
         static void *
   st_update_common_program(struct st_context *st, struct gl_program *prog,
         {
      if (!prog) {
      _mesa_reference_program(st->ctx, dst, NULL);
                        if (st->shader_has_one_variant[prog->info.stage])
                     /* use memset, not an initializer to be sure all memory is zeroed */
                     if (pipe_shader == PIPE_SHADER_GEOMETRY ||
      pipe_shader == PIPE_SHADER_TESS_EVAL) {
   key.clamp_color = st->clamp_vert_color_in_shader &&
                     st->ctx->Light._ClampVertexColor &&
   (prog->info.outputs_written &
            if (st->lower_ucp && st_user_clip_planes_enabled(st->ctx) &&
      (pipe_shader == PIPE_SHADER_GEOMETRY ||
               if (st->lower_point_size)
                        simple_mtx_lock(&st->ctx->Shared->Mutex);
   void *result = st_get_common_variant(st, prog, &key)->base.driver_shader;
               }
         void
   st_update_gp(struct st_context *st)
   {
      void *shader = st_update_common_program(st,
                  }
         void
   st_update_tcp(struct st_context *st)
   {
      void *shader = st_update_common_program(st,
                  }
         void
   st_update_tep(struct st_context *st)
   {
      void *shader = st_update_common_program(st,
                  }
         void
   st_update_cp(struct st_context *st)
   {
      void *shader = st_update_common_program(st,
                  }
