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
         #include "main/errors.h"
      #include "main/hash.h"
   #include "main/mtypes.h"
   #include "program/prog_parameter.h"
   #include "program/prog_print.h"
   #include "program/prog_to_nir.h"
      #include "compiler/glsl/gl_nir.h"
   #include "compiler/glsl/gl_nir_linker.h"
   #include "compiler/nir/nir.h"
   #include "compiler/nir/nir_serialize.h"
   #include "draw/draw_context.h"
      #include "pipe/p_context.h"
   #include "pipe/p_defines.h"
   #include "pipe/p_shader_tokens.h"
   #include "draw/draw_context.h"
      #include "util/u_memory.h"
      #include "st_debug.h"
   #include "st_cb_bitmap.h"
   #include "st_cb_drawpixels.h"
   #include "st_context.h"
   #include "st_program.h"
   #include "st_atifs_to_nir.h"
   #include "st_nir.h"
   #include "st_shader_cache.h"
   #include "st_util.h"
   #include "cso_cache/cso_context.h"
         static void
   destroy_program_variants(struct st_context *st, struct gl_program *target);
      static void
   set_affected_state_flags(uint64_t *states,
                           struct gl_program *prog,
   uint64_t new_constants,
   uint64_t new_sampler_views,
   uint64_t new_samplers,
   {
      if (prog->Parameters->NumParameters)
            if (prog->info.num_textures)
            if (prog->info.num_images)
            if (prog->info.num_ubos)
            if (prog->info.num_ssbos)
            if (prog->info.num_abos)
      }
      /**
   * This determines which states will be updated when the shader is bound.
   */
   void
   st_set_prog_affected_state_flags(struct gl_program *prog)
   {
               switch (prog->info.stage) {
   case MESA_SHADER_VERTEX:
               *states = ST_NEW_VS_STATE |
                  set_affected_state_flags(states, prog,
                           ST_NEW_VS_CONSTANTS,
   ST_NEW_VS_SAMPLER_VIEWS,
   ST_NEW_VS_SAMPLERS,
         case MESA_SHADER_TESS_CTRL:
                        set_affected_state_flags(states, prog,
                           ST_NEW_TCS_CONSTANTS,
   ST_NEW_TCS_SAMPLER_VIEWS,
   ST_NEW_TCS_SAMPLERS,
         case MESA_SHADER_TESS_EVAL:
               *states = ST_NEW_TES_STATE |
            set_affected_state_flags(states, prog,
                           ST_NEW_TES_CONSTANTS,
   ST_NEW_TES_SAMPLER_VIEWS,
   ST_NEW_TES_SAMPLERS,
         case MESA_SHADER_GEOMETRY:
               *states = ST_NEW_GS_STATE |
            set_affected_state_flags(states, prog,
                           ST_NEW_GS_CONSTANTS,
   ST_NEW_GS_SAMPLER_VIEWS,
   ST_NEW_GS_SAMPLERS,
         case MESA_SHADER_FRAGMENT:
               /* gl_FragCoord and glDrawPixels always use constants. */
   *states = ST_NEW_FS_STATE |
                  set_affected_state_flags(states, prog,
                           ST_NEW_FS_CONSTANTS,
   ST_NEW_FS_SAMPLER_VIEWS,
   ST_NEW_FS_SAMPLERS,
         case MESA_SHADER_COMPUTE:
                        set_affected_state_flags(states, prog,
                           ST_NEW_CS_CONSTANTS,
   ST_NEW_CS_SAMPLER_VIEWS,
   ST_NEW_CS_SAMPLERS,
         default:
            }
         /**
   * Delete a shader variant.  Note the caller must unlink the variant from
   * the linked list.
   */
   static void
   delete_variant(struct st_context *st, struct st_variant *v, GLenum target)
   {
      if (v->driver_shader) {
      if (target == GL_VERTEX_PROGRAM_ARB &&
      ((struct st_common_variant*)v)->key.is_draw_shader) {
   /* Draw shader. */
      } else if (st->has_shareable_shaders || v->st == st) {
      /* The shader's context matches the calling context, or we
   * don't care.
   */
   switch (target) {
   case GL_VERTEX_PROGRAM_ARB:
      st->pipe->delete_vs_state(st->pipe, v->driver_shader);
      case GL_TESS_CONTROL_PROGRAM_NV:
      st->pipe->delete_tcs_state(st->pipe, v->driver_shader);
      case GL_TESS_EVALUATION_PROGRAM_NV:
      st->pipe->delete_tes_state(st->pipe, v->driver_shader);
      case GL_GEOMETRY_PROGRAM_NV:
      st->pipe->delete_gs_state(st->pipe, v->driver_shader);
      case GL_FRAGMENT_PROGRAM_ARB:
      st->pipe->delete_fs_state(st->pipe, v->driver_shader);
      case GL_COMPUTE_PROGRAM_NV:
      st->pipe->delete_compute_state(st->pipe, v->driver_shader);
      default:
            } else {
      /* We can't delete a shader with a context different from the one
   * that created it.  Add it to the creating context's zombie list.
   */
                                    }
      static void
   st_unbind_program(struct st_context *st, struct gl_program *p)
   {
               /* Unbind the shader in cso_context and re-bind in st/mesa. */
   switch (p->info.stage) {
   case MESA_SHADER_VERTEX:
      cso_set_vertex_shader_handle(st->cso_context, NULL);
   ctx->NewDriverState |= ST_NEW_VS_STATE;
      case MESA_SHADER_TESS_CTRL:
      cso_set_tessctrl_shader_handle(st->cso_context, NULL);
   ctx->NewDriverState |= ST_NEW_TCS_STATE;
      case MESA_SHADER_TESS_EVAL:
      cso_set_tesseval_shader_handle(st->cso_context, NULL);
   ctx->NewDriverState |= ST_NEW_TES_STATE;
      case MESA_SHADER_GEOMETRY:
      cso_set_geometry_shader_handle(st->cso_context, NULL);
   ctx->NewDriverState |= ST_NEW_GS_STATE;
      case MESA_SHADER_FRAGMENT:
      cso_set_fragment_shader_handle(st->cso_context, NULL);
   ctx->NewDriverState |= ST_NEW_FS_STATE;
      case MESA_SHADER_COMPUTE:
      cso_set_compute_shader_handle(st->cso_context, NULL);
   ctx->NewDriverState |= ST_NEW_CS_STATE;
      default:
            }
      /**
   * Free all basic program variants.
   */
   void
   st_release_variants(struct st_context *st, struct gl_program *p)
   {
               /* If we are releasing shaders, re-bind them, because we don't
   * know which shaders are bound in the driver.
   */
   if (p->variants)
            for (v = p->variants; v; ) {
      struct st_variant *next = v->next;
   delete_variant(st, v, p->Target);
                        /* Note: Any setup of ->ir.nir that has had pipe->create_*_state called on
   * it has resulted in the driver taking ownership of the NIR.  Those
   * callers should be NULLing out the nir field in any pipe_shader_state
   * that might have this called in order to indicate that.
   *
   * GLSL IR and ARB programs will have set gl_program->nir to the same
   * shader as ir->ir.nir, so it will be freed by _mesa_delete_program().
      }
      /**
   * Free all basic program variants and unref program.
   */
   void
   st_release_program(struct st_context *st, struct gl_program **p)
   {
      if (!*p)
            destroy_program_variants(st, *p);
      }
      void
   st_finalize_nir_before_variants(struct nir_shader *nir)
   {
      NIR_PASS_V(nir, nir_split_var_copies);
   NIR_PASS_V(nir, nir_lower_var_copies);
   if (nir->options->lower_all_io_to_temps ||
      nir->options->lower_all_io_to_elements ||
   nir->info.stage == MESA_SHADER_VERTEX ||
   nir->info.stage == MESA_SHADER_GEOMETRY) {
      } else if (nir->info.stage == MESA_SHADER_FRAGMENT) {
                  /* st_nir_assign_vs_in_locations requires correct shader info. */
               }
      static void
   st_prog_to_nir_postprocess(struct st_context *st, nir_shader *nir,
         {
               NIR_PASS_V(nir, nir_lower_reg_intrinsics_to_ssa);
            /* Lower outputs to temporaries to avoid reading from output variables (which
   * is permitted by the language but generally not implemented in HW).
   */
   NIR_PASS_V(nir, nir_lower_io_to_temporaries,
                        NIR_PASS_V(nir, st_nir_lower_wpos_ytransform, prog, screen);
   NIR_PASS_V(nir, nir_lower_system_values);
            /* Optimise NIR */
   NIR_PASS_V(nir, nir_opt_constant_folding);
   gl_nir_opts(nir);
            if (st->allow_st_finalize_nir_twice) {
      char *msg = st_finalize_nir(st, prog, NULL, nir, true, true);
                  }
      /**
   * Translate ARB (asm) program to NIR
   */
   static nir_shader *
   st_translate_prog_to_nir(struct st_context *st, struct gl_program *prog,
         {
      const struct nir_shader_compiler_options *options =
            /* Translate to NIR */
               }
      /**
   * Prepare st_vertex_program info.
   *
   * attrib_to_index is an optional mapping from a vertex attrib to a shader
   * input index.
   */
   void
   st_prepare_vertex_program(struct gl_program *prog)
   {
               stvp->num_inputs = util_bitcount64(prog->info.inputs_read);
            /* Compute mapping of vertex program outputs to slots. */
   memset(stvp->result_to_output, ~0, sizeof(stvp->result_to_output));
   unsigned num_outputs = 0;
   for (unsigned attr = 0; attr < VARYING_SLOT_MAX; attr++) {
      if (prog->info.outputs_written & BITFIELD64_BIT(attr))
      }
   /* pre-setup potentially unused edgeflag output */
      }
      void
   st_translate_stream_output_info(struct gl_program *prog)
   {
      struct gl_transform_feedback_info *info = prog->sh.LinkedTransformFeedback;
   if (!info)
            /* Determine the (default) output register mapping for each output. */
   unsigned num_outputs = 0;
   uint8_t output_mapping[VARYING_SLOT_TESS_MAX];
            for (unsigned attr = 0; attr < VARYING_SLOT_MAX; attr++) {
      /* this output was added by mesa/st and should not be tracked for xfb:
   * drivers must check var->data.explicit_location to find the original output
   * and only emit that one for xfb
   */
   if (prog->skip_pointsize_xfb && attr == VARYING_SLOT_PSIZ)
         if (prog->info.outputs_written & BITFIELD64_BIT(attr))
               /* Translate stream output info. */
   struct pipe_stream_output_info *so_info =
            if (!num_outputs) {
      so_info->num_outputs = 0;
               for (unsigned i = 0; i < info->NumOutputs; i++) {
      so_info->output[i].register_index =
         so_info->output[i].start_component = info->Outputs[i].ComponentOffset;
   so_info->output[i].num_components = info->Outputs[i].NumComponents;
   so_info->output[i].output_buffer = info->Outputs[i].OutputBuffer;
   so_info->output[i].dst_offset = info->Outputs[i].DstOffset;
               for (unsigned i = 0; i < PIPE_MAX_SO_BUFFERS; i++) {
         }
      }
      /**
   * Creates a driver shader from a NIR shader.  Takes ownership of the
   * passed nir_shader.
   */
   struct pipe_shader_state *
   st_create_nir_shader(struct st_context *st, struct pipe_shader_state *state)
   {
               assert(state->type == PIPE_SHADER_IR_NIR);
   nir_shader *nir = state->ir.nir;
   struct shader_info info = nir->info;
            if (ST_DEBUG & DEBUG_PRINT_IR) {
      fprintf(stderr, "NIR before handing off to driver:\n");
               struct pipe_shader_state *shader;
   switch (stage) {
   case MESA_SHADER_VERTEX:
      shader = pipe->create_vs_state(pipe, state);
      case MESA_SHADER_TESS_CTRL:
      shader = pipe->create_tcs_state(pipe, state);
      case MESA_SHADER_TESS_EVAL:
      shader = pipe->create_tes_state(pipe, state);
      case MESA_SHADER_GEOMETRY:
      shader = pipe->create_gs_state(pipe, state);
      case MESA_SHADER_FRAGMENT:
      shader = pipe->create_fs_state(pipe, state);
      case MESA_SHADER_COMPUTE: {
      struct pipe_compute_state cs = {0};
   cs.ir_type = state->type;
            if (state->type == PIPE_SHADER_IR_NIR)
         else
            shader = pipe->create_compute_state(pipe, &cs);
      }
   default:
      unreachable("unsupported shader stage");
                  }
      /**
   * Translate a vertex program.
   */
   static bool
   st_translate_vertex_program(struct st_context *st,
         {
      /* This determines which states will be updated when the assembly
      * shader is bound.
      prog->affected_states = ST_NEW_VS_STATE |
                  if (prog->Parameters->NumParameters)
            if (prog->arb.Instructions && prog->nir)
            if (prog->serialized_nir) {
      free(prog->serialized_nir);
               prog->state.type = PIPE_SHADER_IR_NIR;
   if (prog->arb.Instructions)
      prog->nir = st_translate_prog_to_nir(st, prog,
      st_prog_to_nir_postprocess(st, prog->nir, prog);
            st_prepare_vertex_program(prog);
      }
      static struct nir_shader *
   get_nir_shader(struct st_context *st, struct gl_program *prog)
   {
      if (prog->nir) {
               /* The first shader variant takes ownership of NIR, so that there is
   * no cloning. Additional shader variants are always generated from
   * serialized NIR to save memory.
   */
   prog->nir = NULL;
   assert(prog->serialized_nir && prog->serialized_nir_size);
               struct blob_reader blob_reader;
   const struct nir_shader_compiler_options *options =
            blob_reader_init(&blob_reader, prog->serialized_nir, prog->serialized_nir_size);
      }
      static void
   lower_ucp(struct st_context *st,
            struct nir_shader *nir,
      {
      if (nir->info.outputs_written & VARYING_BIT_CLIP_DIST0)
         else {
      struct pipe_screen *screen = st->screen;
   bool can_compact = screen->get_param(screen,
                  gl_state_index16 clipplane_state[MAX_CLIP_PLANES][STATE_LENGTH] = {{0}};
   for (int i = 0; i < MAX_CLIP_PLANES; ++i) {
      if (use_eye) {
      clipplane_state[i][0] = STATE_CLIPPLANE;
      } else {
      clipplane_state[i][0] = STATE_CLIP_INTERNAL;
      }
               if (nir->info.stage == MESA_SHADER_VERTEX ||
      nir->info.stage == MESA_SHADER_TESS_EVAL) {
   NIR_PASS_V(nir, nir_lower_clip_vs, ucp_enables,
      } else if (nir->info.stage == MESA_SHADER_GEOMETRY) {
      NIR_PASS_V(nir, nir_lower_clip_gs, ucp_enables,
               NIR_PASS_V(nir, nir_lower_io_to_temporaries,
               }
      static struct st_common_variant *
   st_create_common_variant(struct st_context *st,
               {
               struct st_common_variant *v = CALLOC_STRUCT(st_common_variant);
            static const gl_state_index16 point_size_state[STATE_LENGTH] =
                                             state.type = PIPE_SHADER_IR_NIR;
   state.ir.nir = get_nir_shader(st, prog);
            if (key->clamp_color) {
      NIR_PASS_V(state.ir.nir, nir_lower_clamp_color_outputs);
      }
   if (key->passthrough_edgeflags) {
      NIR_PASS_V(state.ir.nir, nir_lower_passthrough_edgeflags);
               if (key->export_point_size) {
      /* if flag is set, shader must export psiz */
   _mesa_add_state_reference(params, point_size_state);
   NIR_PASS_V(state.ir.nir, nir_lower_point_size_mov,
                        if (key->lower_ucp) {
      assert(!options->unify_interfaces);
   lower_ucp(st, state.ir.nir, key->lower_ucp, params);
               if (st->emulate_gl_clamp &&
            nir_lower_tex_options tex_opts = {0};
   tex_opts.saturate_s = key->gl_clamp[0];
   tex_opts.saturate_t = key->gl_clamp[1];
   tex_opts.saturate_r = key->gl_clamp[2];
               if (finalize || !st->allow_st_finalize_nir_twice) {
      char *msg = st_finalize_nir(st, prog, prog->shader_program, state.ir.nir,
                  /* Clip lowering and edgeflags may have introduced new varyings, so
   * update the inputs_read/outputs_written. However, with
   * unify_interfaces set (aka iris) the non-SSO varyings layout is
   * decided at link time with outputs_written updated so the two line
   * up.  A driver with this flag set may not use any of the lowering
   * passes that would change the varyings, so skip to make sure we don't
   * break its linkage.
   */
   if (!options->unify_interfaces) {
      nir_shader_gather_info(state.ir.nir,
                  if (key->is_draw_shader) {
      NIR_PASS_V(state.ir.nir, gl_nir_lower_images, false);
      }
   else
               }
      static void
   st_add_variant(struct st_variant **list, struct st_variant *v)
   {
               /* Make sure that the default variant stays the first in the list, and insert
   * any later variants in as the second entry.
   */
   if (first) {
      v->next = first->next;
      } else {
            }
      /**
   * Find/create a vertex program variant.
   */
   struct st_common_variant *
   st_get_common_variant(struct st_context *st,
               {
               /* Search for existing variant */
   for (v = st_common_variant(prog->variants); v;
      v = st_common_variant(v->base.next)) {
   if (memcmp(&v->key, key, sizeof(*key)) == 0) {
                     if (!v) {
      if (prog->variants != NULL) {
      _mesa_perf_debug(st->ctx, MESA_DEBUG_SEVERITY_MEDIUM,
                  "Compiling %s shader variant (%s%s%s%s%s%s)",
   _mesa_shader_stage_to_string(prog->info.stage),
   key->passthrough_edgeflags ? "edgeflags," : "",
   key->clamp_color ? "clamp_color," : "",
   key->export_point_size ? "point_size," : "",
            /* create now */
   v = st_create_common_variant(st, prog, key);
   if (v) {
                                 v->vert_attrib_mask =
                                       }
         /**
   * Translate a non-GLSL Mesa fragment shader into a NIR shader.
   */
   static bool
   st_translate_fragment_program(struct st_context *st,
         {
      /* This determines which states will be updated when the assembly
   * shader is bound.
   *
   * fragment.position and glDrawPixels always use constants.
   */
   prog->affected_states = ST_NEW_FS_STATE |
                  if (prog->ati_fs) {
      /* Just set them for ATI_fs unconditionally. */
   prog->affected_states |= ST_NEW_FS_SAMPLER_VIEWS |
      } else {
      /* ARB_fp */
   if (prog->SamplersUsed)
      prog->affected_states |= ST_NEW_FS_SAMPLER_VIEWS |
            /* Translate to NIR. */
   if (prog->nir && prog->arb.Instructions)
            if (prog->serialized_nir) {
      free(prog->serialized_nir);
               prog->state.type = PIPE_SHADER_IR_NIR;
   if (prog->arb.Instructions) {
      prog->nir = st_translate_prog_to_nir(st, prog,
      } else if (prog->ati_fs) {
      const struct nir_shader_compiler_options *options =
            assert(!prog->nir);
      }
            prog->info = prog->nir->info;
   if (prog->ati_fs) {
      /* ATI_fs will lower fixed function fog at variant time, after the FF vertex
   * prog has been generated.  So we have to always declare a read of FOGC so
   * that FF vp feeds it to us just in case.
   */
                  }
      static struct st_fp_variant *
   st_create_fp_variant(struct st_context *st,
               {
      struct st_fp_variant *variant = CALLOC_STRUCT(st_fp_variant);
   struct pipe_shader_state state = {0};
   struct gl_program_parameter_list *params = fp->Parameters;
   static const gl_state_index16 texcoord_state[STATE_LENGTH] =
         static const gl_state_index16 scale_state[STATE_LENGTH] =
         static const gl_state_index16 bias_state[STATE_LENGTH] =
         static const gl_state_index16 alpha_ref_state[STATE_LENGTH] =
            if (!variant)
                     /* Translate ATI_fs to NIR at variant time because that's when we have the
   * texture types.
   */
   state.ir.nir = get_nir_shader(st, fp);
                     if (fp->ati_fs) {
      if (key->fog) {
      NIR_PASS_V(state.ir.nir, st_nir_lower_fog, key->fog, fp->Parameters);
   NIR_PASS_V(state.ir.nir, nir_lower_io_to_temporaries,
      nir_shader_get_entrypoint(state.ir.nir),
                                       if (key->clamp_color) {
      NIR_PASS_V(state.ir.nir, nir_lower_clamp_color_outputs);
               if (key->lower_flatshade) {
      NIR_PASS_V(state.ir.nir, nir_lower_flatshade);
               if (key->lower_alpha_func != COMPARE_FUNC_ALWAYS) {
      _mesa_add_state_reference(params, alpha_ref_state);
   NIR_PASS_V(state.ir.nir, nir_lower_alpha_test, key->lower_alpha_func,
                     if (key->lower_two_sided_color) {
      bool face_sysval = st->ctx->Const.GLSLFrontFacingIsSysVal;
   NIR_PASS_V(state.ir.nir, nir_lower_two_sided_color, face_sysval);
               if (key->persample_shading) {
      nir_shader *shader = state.ir.nir;
   nir_foreach_shader_in_variable(var, shader)
            /* In addition to requiring per-sample interpolation, sample shading
   * changes the behaviour of gl_SampleMaskIn, so we need per-sample shading
   * even if there are no shader-in variables at all. In that case,
   * uses_sample_shading won't be set by glsl_to_nir. We need to do so here.
   */
                        if (st->emulate_gl_clamp &&
            nir_lower_tex_options tex_opts = {0};
   tex_opts.saturate_s = key->gl_clamp[0];
   tex_opts.saturate_t = key->gl_clamp[1];
   tex_opts.saturate_r = key->gl_clamp[2];
   NIR_PASS_V(state.ir.nir, nir_lower_tex, &tex_opts);
                        /* glBitmap */
   if (key->bitmap) {
               variant->bitmap_sampler = ffs(~fp->SamplersUsed) - 1;
   options.sampler = variant->bitmap_sampler;
            NIR_PASS_V(state.ir.nir, nir_lower_bitmap, &options);
               /* glDrawPixels (color only) */
   if (key->drawpixels) {
      nir_lower_drawpixels_options options = {{0}};
            /* Find the first unused slot. */
   variant->drawpix_sampler = ffs(~samplers_used) - 1;
   options.drawpix_sampler = variant->drawpix_sampler;
            options.pixel_maps = key->pixelMaps;
   if (key->pixelMaps) {
      variant->pixelmap_sampler = ffs(~samplers_used) - 1;
               options.scale_and_bias = key->scaleAndBias;
   if (key->scaleAndBias) {
      _mesa_add_state_reference(params, scale_state);
   memcpy(options.scale_state_tokens, scale_state,
         _mesa_add_state_reference(params, bias_state);
   memcpy(options.bias_state_tokens, bias_state,
               _mesa_add_state_reference(params, texcoord_state);
   memcpy(options.texcoord_state_tokens, texcoord_state,
            NIR_PASS_V(state.ir.nir, nir_lower_drawpixels, &options);
                        if (unlikely(key->external.lower_nv12 || key->external.lower_nv21 ||
                  key->external.lower_iyuv ||
   key->external.lower_xy_uxvx || key->external.lower_yx_xuxv ||
   key->external.lower_yx_xvxu || key->external.lower_xy_vxux ||
            st_nir_lower_samplers(st->screen, state.ir.nir,
            nir_lower_tex_options options = {0};
   options.lower_y_uv_external = key->external.lower_nv12;
   options.lower_y_vu_external = key->external.lower_nv21;
   options.lower_y_u_v_external = key->external.lower_iyuv;
   options.lower_xy_uxvx_external = key->external.lower_xy_uxvx;
   options.lower_xy_vxux_external = key->external.lower_xy_vxux;
   options.lower_yx_xuxv_external = key->external.lower_yx_xuxv;
   options.lower_yx_xvxu_external = key->external.lower_yx_xvxu;
   options.lower_ayuv_external = key->external.lower_ayuv;
   options.lower_xyuv_external = key->external.lower_xyuv;
   options.lower_yuv_external = key->external.lower_yuv;
   options.lower_yu_yv_external = key->external.lower_yu_yv;
   options.lower_yv_yu_external = key->external.lower_yv_yu;
   options.lower_y41x_external = key->external.lower_y41x;
   options.bt709_external = key->external.bt709;
   options.bt2020_external = key->external.bt2020;
   options.yuv_full_range_external = key->external.yuv_full_range;
   NIR_PASS_V(state.ir.nir, nir_lower_tex, &options);
   finalize = true;
               if (finalize || !st->allow_st_finalize_nir_twice) {
      char *msg = st_finalize_nir(st, fp, fp->shader_program, state.ir.nir,
                     /* This pass needs to happen *after* nir_lower_sampler */
   if (unlikely(need_lower_tex_src_plane)) {
      NIR_PASS_V(state.ir.nir, st_nir_lower_tex_src_plane,
               ~fp->SamplersUsed,
   key->external.lower_nv12 | key->external.lower_nv21 |
                     /* It is undefined behavior when an ARB assembly uses SHADOW2D target
   * with a texture in not depth format. In this case NVIDIA automatically
   * replaces SHADOW sampler with a normal sampler and some games like
   * Penumbra Overture which abuses this UB (issues/8425) works fine but
   * breaks with mesa. Replace the shadow sampler with a normal one here
   */
   if (!fp->shader_program && ~key->depth_textures & fp->ShadowSamplers) {
      NIR_PASS_V(state.ir.nir, nir_remove_tex_shadow,
                     if (finalize || !st->allow_st_finalize_nir_twice) {
      /* Some of the lowering above may have introduced new varyings */
   nir_shader_gather_info(state.ir.nir,
            struct pipe_screen *screen = st->screen;
   if (screen->finalize_nir) {
      char *msg = screen->finalize_nir(screen, state.ir.nir);
                  variant->base.driver_shader = st_create_nir_shader(st, &state);
               }
      /**
   * Translate fragment program if needed.
   */
   struct st_fp_variant *
   st_get_fp_variant(struct st_context *st,
               {
               /* Search for existing variant */
   for (fpv = st_fp_variant(fp->variants); fpv;
      fpv = st_fp_variant(fpv->base.next)) {
   if (memcmp(&fpv->key, key, sizeof(*key)) == 0) {
                     if (!fpv) {
               if (fp->variants != NULL) {
      _mesa_perf_debug(st->ctx, MESA_DEBUG_SEVERITY_MEDIUM,
                  "Compiling fragment shader variant (%s%s%s%s%s%s%s%s%s%s%s%s%s%d)",
   key->bitmap ? "bitmap," : "",
   key->drawpixels ? "drawpixels," : "",
   key->scaleAndBias ? "scale_bias," : "",
   key->pixelMaps ? "pixel_maps," : "",
   key->clamp_color ? "clamp_color," : "",
   key->persample_shading ? "persample_shading," : "",
   key->fog ? "fog," : "",
   key->lower_two_sided_color ? "twoside," : "",
   key->lower_flatshade ? "flatshade," : "",
   key->lower_alpha_func != COMPARE_FUNC_ALWAYS ? "alpha_compare," : "",
   /* skipped ATI_fs targets */
            fpv = st_create_fp_variant(st, fp, key);
   if (fpv) {
                                 }
      /**
   * Vert/Geom/Frag programs have per-context variants.  Free all the
   * variants attached to the given program which match the given context.
   */
   static void
   destroy_program_variants(struct st_context *st, struct gl_program *p)
   {
      if (!p || p == &_mesa_DummyProgram)
            struct st_variant *v, **prevPtr = &p->variants;
            for (v = p->variants; v; ) {
      struct st_variant *next = v->next;
   if (v->st == st) {
      if (!unbound) {
      st_unbind_program(st, p);
               /* unlink from list */
   *prevPtr = next;
   /* destroy this variant */
      }
   else {
         }
         }
         /**
   * Callback for _mesa_HashWalk.  Free all the shader's program variants
   * which match the given context.
   */
   static void
   destroy_shader_program_variants_cb(void *data, void *userData)
   {
      struct st_context *st = (struct st_context *) userData;
            switch (shader->Type) {
   case GL_SHADER_PROGRAM_MESA:
      {
                     for (i = 0; i < ARRAY_SIZE(shProg->_LinkedShaders); i++) {
      if (shProg->_LinkedShaders[i])
         }
      case GL_VERTEX_SHADER:
   case GL_FRAGMENT_SHADER:
   case GL_GEOMETRY_SHADER:
   case GL_TESS_CONTROL_SHADER:
   case GL_TESS_EVALUATION_SHADER:
   case GL_COMPUTE_SHADER:
         default:
            }
         /**
   * Callback for _mesa_HashWalk.  Free all the program variants which match
   * the given context.
   */
   static void
   destroy_program_variants_cb(void *data, void *userData)
   {
      struct st_context *st = (struct st_context *) userData;
   struct gl_program *program = (struct gl_program *) data;
      }
         /**
   * Walk over all shaders and programs to delete any variants which
   * belong to the given context.
   * This is called during context tear-down.
   */
   void
   st_destroy_program_variants(struct st_context *st)
   {
      /* If shaders can be shared with other contexts, the last context will
   * call DeleteProgram on all shaders, releasing everything.
   */
   if (st->has_shareable_shaders)
            /* ARB vert/frag program */
   _mesa_HashWalk(st->ctx->Shared->Programs,
            /* GLSL vert/frag/geom shaders */
   _mesa_HashWalk(st->ctx->Shared->ShaderObjects,
      }
      /**
   * Compile one shader variant.
   */
   static void
   st_precompile_shader_variant(struct st_context *st,
         {
      switch (prog->Target) {
   case GL_VERTEX_PROGRAM_ARB:
   case GL_TESS_CONTROL_PROGRAM_NV:
   case GL_TESS_EVALUATION_PROGRAM_NV:
   case GL_GEOMETRY_PROGRAM_NV:
   case GL_COMPUTE_PROGRAM_NV: {
                        if (_mesa_is_desktop_gl_compat(st->ctx) &&
      st->clamp_vert_color_in_shader &&
   (prog->info.outputs_written & (VARYING_SLOT_COL0 |
                                 key.st = st->has_shareable_shaders ? NULL : st;
   st_get_common_variant(st, prog, &key);
               case GL_FRAGMENT_PROGRAM_ARB: {
                        key.st = st->has_shareable_shaders ? NULL : st;
   key.lower_alpha_func = COMPARE_FUNC_ALWAYS;
   if (prog->ati_fs) {
      for (int i = 0; i < ARRAY_SIZE(key.texture_index); i++)
               /* Shadow samplers require texture in depth format, which we lower to
   * non-shadow if necessary for ARB programs
   */
   if (!prog->shader_program)
            st_get_fp_variant(st, prog, &key);
               default:
            }
      void
   st_serialize_nir(struct gl_program *prog)
   {
      if (!prog->serialized_nir) {
      struct blob blob;
            blob_init(&blob);
   nir_serialize(&blob, prog->nir, false);
   blob_finish_get_buffer(&blob, &prog->serialized_nir, &size);
         }
      void
   st_finalize_program(struct st_context *st, struct gl_program *prog)
   {
      struct gl_context *ctx = st->ctx;
                     if (prog->info.stage == MESA_SHADER_VERTEX)
         else if (prog->info.stage == MESA_SHADER_TESS_CTRL)
         else if (prog->info.stage == MESA_SHADER_TESS_EVAL)
         else if (prog->info.stage == MESA_SHADER_GEOMETRY)
         else if (prog->info.stage == MESA_SHADER_FRAGMENT)
         else if (prog->info.stage == MESA_SHADER_COMPUTE)
            if (is_bound) {
      if (prog->info.stage == MESA_SHADER_VERTEX) {
      ctx->Array.NewVertexElements = true;
      } else {
                     if (prog->nir) {
               /* This is only needed for ARB_vp/fp programs and when the disk cache
   * is disabled. If the disk cache is enabled, GLSL programs are
   * serialized in write_nir_to_cache.
   */
               /* Always create the default variant of the program. */
      }
      /**
   * Called when the program's text/code is changed.  We have to free
   * all shader variants and corresponding gallium shaders when this happens.
   */
   GLboolean
   st_program_string_notify( struct gl_context *ctx,
               {
               /* GLSL-to-NIR should not end up here. */
                     if (target == GL_FRAGMENT_PROGRAM_ARB ||
      target == GL_FRAGMENT_SHADER_ATI) {
   if (!st_translate_fragment_program(st, prog))
      } else if (target == GL_VERTEX_PROGRAM_ARB) {
      if (!st_translate_vertex_program(st, prog))
         if (st->lower_point_size &&
      gl_nir_can_add_pointsize_to_program(&st->ctx->Const, prog)) {
   prog->skip_pointsize_xfb = true;
                  st_finalize_program(st, prog);
      }
