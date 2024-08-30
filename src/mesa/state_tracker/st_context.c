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
         #include "main/accum.h"
   #include "main/context.h"
   #include "main/debug_output.h"
   #include "main/framebuffer.h"
   #include "main/glthread.h"
   #include "main/shaderobj.h"
   #include "main/state.h"
   #include "main/version.h"
   #include "main/hash.h"
   #include "program/prog_cache.h"
   #include "vbo/vbo.h"
   #include "glapi/glapi.h"
   #include "st_manager.h"
   #include "st_context.h"
   #include "st_debug.h"
   #include "st_cb_bitmap.h"
   #include "st_cb_clear.h"
   #include "st_cb_drawpixels.h"
   #include "st_cb_drawtex.h"
   #include "st_cb_eglimage.h"
   #include "st_cb_feedback.h"
   #include "st_cb_flush.h"
   #include "st_atom.h"
   #include "st_draw.h"
   #include "st_extensions.h"
   #include "st_gen_mipmap.h"
   #include "st_pbo.h"
   #include "st_program.h"
   #include "st_sampler_view.h"
   #include "st_shader_cache.h"
   #include "st_texcompress_compute.h"
   #include "st_texture.h"
   #include "st_util.h"
   #include "pipe/p_context.h"
   #include "util/u_cpu_detect.h"
   #include "util/u_inlines.h"
   #include "util/u_upload_mgr.h"
   #include "util/u_vbuf.h"
   #include "util/u_memory.h"
   #include "util/hash_table.h"
   #include "cso_cache/cso_context.h"
   #include "compiler/glsl/glsl_parser_extras.h"
      DEBUG_GET_ONCE_BOOL_OPTION(mesa_mvp_dp4, "MESA_MVP_DP4", false)
      /* The list of state update functions. */
   st_update_func_t st_update_functions[ST_NUM_ATOMS];
      static void
   init_atoms_once(void)
   {
            #define ST_STATE(FLAG, st_update) st_update_functions[FLAG##_INDEX] = st_update;
   #include "st_atom_list.h"
   #undef ST_STATE
         if (util_get_cpu_caps()->has_popcnt)
      }
      void
   st_invalidate_buffers(struct st_context *st)
   {
      st->ctx->NewDriverState |= ST_NEW_BLEND |
                              ST_NEW_DSA |
   ST_NEW_FB_STATE |
   ST_NEW_SAMPLE_STATE |
   ST_NEW_SAMPLE_SHADING |
   ST_NEW_FS_STATE |
   }
         static inline bool
   st_vp_uses_current_values(const struct gl_context *ctx)
   {
                  }
         void
   st_invalidate_state(struct gl_context *ctx)
   {
      GLbitfield new_state = ctx->NewState;
            if (new_state & _NEW_BUFFERS) {
         } else {
      /* These set a subset of flags set by _NEW_BUFFERS, so we only have to
   * check them when _NEW_BUFFERS isn't set.
   */
   if (new_state & _NEW_FOG)
               if (new_state & (_NEW_LIGHT_STATE |
                  if ((new_state & _NEW_LIGHT_STATE) &&
      (st->lower_flatshade || st->lower_two_sided_color))
         if (new_state & _NEW_PROJECTION &&
      st_user_clip_planes_enabled(ctx))
         if (new_state & _NEW_PIXEL)
            if (new_state & _NEW_CURRENT_ATTRIB && st_vp_uses_current_values(ctx)) {
      ctx->NewDriverState |= ST_NEW_VERTEX_ARRAYS;
   /* glColor3f -> glColor4f changes the vertex format. */
               /* Update the vertex shader if ctx->Light._ClampVertexColor was changed. */
   if (st->clamp_vert_color_in_shader && (new_state & _NEW_LIGHT_STATE)) {
      ctx->NewDriverState |= ST_NEW_VS_STATE;
   if (_mesa_is_desktop_gl_compat(st->ctx) && ctx->Version >= 32) {
                     /* Update the vertex shader if ctx->Point was changed. */
   if (st->lower_point_size && new_state & _NEW_POINT) {
      if (ctx->GeometryProgram._Current)
         else if (ctx->TessEvalProgram._Current)
         else
               if (new_state & _NEW_TEXTURE_OBJECT) {
      ctx->NewDriverState |= st->active_states &
                     if (ctx->FragmentProgram._Current) {
               if (fp->ExternalSamplersUsed || fp->ati_fs ||
      (!fp->shader_program && fp->ShadowSamplers))
         }
         /*
   * In some circumstances (such as running google-chrome) the state
   * tracker may try to delete a resource view from a context different
   * than when it was created.  We don't want to do that.
   *
   * In that situation, st_texture_release_all_sampler_views() calls this
   * function to transfer the sampler view reference to this context (expected
   * to be the context which created the view.)
   */
   void
   st_save_zombie_sampler_view(struct st_context *st,
         {
                        entry = MALLOC_STRUCT(st_zombie_sampler_view_node);
   if (!entry)
                     /* We need a mutex since this function may be called from one thread
   * while free_zombie_resource_views() is called from another.
   */
   simple_mtx_lock(&st->zombie_sampler_views.mutex);
   list_addtail(&entry->node, &st->zombie_sampler_views.list.node);
      }
         /*
   * Since OpenGL shaders may be shared among contexts, we can wind up
   * with variants of a shader created with different contexts.
   * When we go to destroy a gallium shader, we want to free it with the
   * same context that it was created with, unless the driver reports
   * PIPE_CAP_SHAREABLE_SHADERS = TRUE.
   */
   void
   st_save_zombie_shader(struct st_context *st,
               {
               /* we shouldn't be here if the driver supports shareable shaders */
            entry = MALLOC_STRUCT(st_zombie_shader_node);
   if (!entry)
            entry->shader = shader;
            /* We need a mutex since this function may be called from one thread
   * while free_zombie_shaders() is called from another.
   */
   simple_mtx_lock(&st->zombie_shaders.mutex);
   list_addtail(&entry->node, &st->zombie_shaders.list.node);
      }
         /*
   * Free any zombie sampler views that may be attached to this context.
   */
   static void
   free_zombie_sampler_views(struct st_context *st)
   {
               if (list_is_empty(&st->zombie_sampler_views.list.node)) {
                           LIST_FOR_EACH_ENTRY_SAFE(entry, next,
                     assert(entry->view->context == st->pipe);
                                    }
         /*
   * Free any zombie shaders that may be attached to this context.
   */
   static void
   free_zombie_shaders(struct st_context *st)
   {
               if (list_is_empty(&st->zombie_shaders.list.node)) {
                           LIST_FOR_EACH_ENTRY_SAFE(entry, next,
                     switch (entry->type) {
   case PIPE_SHADER_VERTEX:
      st->pipe->bind_vs_state(st->pipe, NULL);
   st->pipe->delete_vs_state(st->pipe, entry->shader);
      case PIPE_SHADER_FRAGMENT:
      st->pipe->bind_fs_state(st->pipe, NULL);
   st->pipe->delete_fs_state(st->pipe, entry->shader);
      case PIPE_SHADER_GEOMETRY:
      st->pipe->bind_gs_state(st->pipe, NULL);
   st->pipe->delete_gs_state(st->pipe, entry->shader);
      case PIPE_SHADER_TESS_CTRL:
      st->pipe->bind_tcs_state(st->pipe, NULL);
   st->pipe->delete_tcs_state(st->pipe, entry->shader);
      case PIPE_SHADER_TESS_EVAL:
      st->pipe->bind_tes_state(st->pipe, NULL);
   st->pipe->delete_tes_state(st->pipe, entry->shader);
      case PIPE_SHADER_COMPUTE:
      st->pipe->bind_compute_state(st->pipe, NULL);
   st->pipe->delete_compute_state(st->pipe, entry->shader);
      default:
         }
                           }
         /*
   * This function is called periodically to free any zombie objects
   * which are attached to this context.
   */
   void
   st_context_free_zombie_objects(struct st_context *st)
   {
      free_zombie_sampler_views(st);
      }
         static void
   st_destroy_context_priv(struct st_context *st, bool destroy_pipe)
   {
      st_destroy_draw(st);
   st_destroy_clear(st);
   st_destroy_bitmap(st);
   st_destroy_drawpix(st);
   st_destroy_drawtex(st);
            if (_mesa_has_compute_shaders(st->ctx) && st->transcode_astc)
            st_destroy_bound_texture_handles(st);
            /* free glReadPixels cache data */
   st_invalidate_readpix_cache(st);
                     if (st->pipe && destroy_pipe)
            st->ctx->st = NULL;
      }
         static void
   st_init_driver_flags(struct st_context *st)
   {
               /* Shader resources */
   if (st->has_hw_atomics)
         else
            f->NewShaderConstants[MESA_SHADER_VERTEX] = ST_NEW_VS_CONSTANTS;
   f->NewShaderConstants[MESA_SHADER_TESS_CTRL] = ST_NEW_TCS_CONSTANTS;
   f->NewShaderConstants[MESA_SHADER_TESS_EVAL] = ST_NEW_TES_CONSTANTS;
   f->NewShaderConstants[MESA_SHADER_GEOMETRY] = ST_NEW_GS_CONSTANTS;
   f->NewShaderConstants[MESA_SHADER_FRAGMENT] = ST_NEW_FS_CONSTANTS;
            if (st->lower_alpha_test)
         else
            f->NewMultisampleEnable = ST_NEW_BLEND | ST_NEW_RASTERIZER |
                  /* This depends on what the gallium driver wants. */
   if (st->force_persample_in_shader) {
      f->NewMultisampleEnable |= ST_NEW_FS_STATE;
      } else {
                  if (st->clamp_frag_color_in_shader) {
         } else {
                  f->NewClipPlaneEnable = ST_NEW_RASTERIZER;
   if (st->lower_ucp)
            if (st->emulate_gl_clamp)
      f->NewSamplersWithClamp = ST_NEW_SAMPLERS |
                     if (!st->has_hw_atomics && st->ctx->Const.ShaderStorageBufferOffsetAlignment > 4)
      }
      static bool
   st_have_perfmon(struct st_context *st)
   {
               if (!screen->get_driver_query_info || !screen->get_driver_query_group_info)
               }
      static bool
   st_have_perfquery(struct st_context *ctx)
   {
               return pipe->init_intel_perf_query_info && pipe->get_intel_perf_query_info &&
         pipe->get_intel_perf_query_counter_info &&
   pipe->new_intel_perf_query_obj && pipe->begin_intel_perf_query &&
   pipe->end_intel_perf_query && pipe->delete_intel_perf_query &&
      }
      static struct st_context *
   st_create_context_priv(struct gl_context *ctx, struct pipe_context *pipe,
         {
      struct pipe_screen *screen = pipe->screen;
                     ctx->st_opts = &st->options;
            st->ctx = ctx;
   st->screen = screen;
            st->can_bind_const_buffer_as_vertex =
            /* st/mesa always uploads zero-stride vertex attribs, and other user
   * vertex buffers are only possible with a compatibility profile.
   * So tell the u_vbuf module that user VBOs are not possible with the Core
   * profile, so that u_vbuf is bypassed completely if there is nothing else
   * to do.
   */
   unsigned cso_flags;
   switch (ctx->API) {
   case API_OPENGL_CORE:
      cso_flags = CSO_NO_USER_VERTEX_BUFFERS;
      case API_OPENGLES:
   case API_OPENGLES2:
      cso_flags = CSO_NO_64B_VERTEX_BUFFERS;
      default:
      cso_flags = 0;
               st->cso_context = cso_create_context(pipe, cso_flags);
            static once_flag flag = ONCE_FLAG_INIT;
            st_init_clear(st);
   {
      enum pipe_texture_transfer_mode val = screen->get_param(screen, PIPE_CAP_TEXTURE_TRANSFER_MODES);
   st->prefer_blit_based_texture_transfer = (val & PIPE_TEXTURE_TRANSFER_BLIT) != 0;
      }
            /* Choose texture target for glDrawPixels, glBitmap, renderbuffers */
   if (screen->get_param(screen, PIPE_CAP_NPOT_TEXTURES))
         else
            /* Setup vertex element info for 'struct st_util_vertex'.
   */
   {
               memset(&st->util_velems, 0, sizeof(st->util_velems));
   st->util_velems.velems[0].src_offset = 0;
   st->util_velems.velems[0].vertex_buffer_index = 0;
   st->util_velems.velems[0].src_format = PIPE_FORMAT_R32G32B32_FLOAT;
   st->util_velems.velems[0].src_stride = sizeof(struct st_util_vertex);
   st->util_velems.velems[1].src_offset = 3 * sizeof(float);
   st->util_velems.velems[1].vertex_buffer_index = 0;
   st->util_velems.velems[1].src_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
   st->util_velems.velems[1].src_stride = sizeof(struct st_util_vertex);
   st->util_velems.velems[2].src_offset = 7 * sizeof(float);
   st->util_velems.velems[2].vertex_buffer_index = 0;
   st->util_velems.velems[2].src_format = PIPE_FORMAT_R32G32_FLOAT;
               ctx->Const.PackedDriverUniformStorage =
            ctx->Const.BitmapUsesRed =
      screen->is_format_supported(screen, PIPE_FORMAT_R8_UNORM,
               ctx->Const.QueryCounterBits.Timestamp =
            st->has_stencil_export =
         st->has_etc1 = screen->is_format_supported(screen, PIPE_FORMAT_ETC1_RGB8,
               st->has_etc2 = screen->is_format_supported(screen, PIPE_FORMAT_ETC2_RGB8,
               st->transcode_etc = options->transcode_etc &&
                     st->transcode_astc = options->transcode_astc &&
                        screen->is_format_supported(screen, PIPE_FORMAT_DXT5_SRGBA,
            st->has_astc_2d_ldr =
      screen->is_format_supported(screen, PIPE_FORMAT_ASTC_4x4_SRGB,
      st->has_astc_5x5_ldr =
      screen->is_format_supported(screen, PIPE_FORMAT_ASTC_5x5_SRGB,
      st->astc_void_extents_need_denorm_flush =
            st->has_s3tc = screen->is_format_supported(screen, PIPE_FORMAT_DXT5_RGBA,
               st->has_rgtc = screen->is_format_supported(screen, PIPE_FORMAT_RGTC2_UNORM,
               st->has_latc = screen->is_format_supported(screen, PIPE_FORMAT_LATC2_UNORM,
               st->has_bptc = screen->is_format_supported(screen, PIPE_FORMAT_BPTC_SRGBA,
               st->force_persample_in_shader =
      screen->get_param(screen, PIPE_CAP_SAMPLE_SHADING) &&
      st->has_shareable_shaders = screen->get_param(screen,
         st->needs_texcoord_semantic =
         st->apply_texture_swizzle_to_border_color =
      !!(screen->get_param(screen, PIPE_CAP_TEXTURE_BORDER_COLOR_QUIRK) &
      (PIPE_QUIRK_TEXTURE_BORDER_COLOR_SWIZZLE_NV50 |
   st->use_format_with_border_color =
      !!(screen->get_param(screen, PIPE_CAP_TEXTURE_BORDER_COLOR_QUIRK) &
      st->alpha_border_color_is_not_w =
      !!(screen->get_param(screen, PIPE_CAP_TEXTURE_BORDER_COLOR_QUIRK) &
      st->emulate_gl_clamp =
         st->has_time_elapsed =
         st->has_half_float_packing =
         st->has_multi_draw_indirect =
         st->has_indirect_partial_stride =
         st->has_occlusion_query =
         st->has_single_pipe_stat =
         st->has_pipeline_stat =
         st->has_indep_blend_enable =
         st->has_indep_blend_func =
         st->can_dither =
         st->lower_flatshade =
         st->lower_alpha_test =
         st->lower_point_size =
         st->lower_two_sided_color =
         st->lower_ucp =
         st->prefer_real_buffer_in_constbuf0 =
         st->has_conditional_render =
         st->lower_rect_tex =
                  st->has_hw_atomics =
      screen->get_shader_param(screen, PIPE_SHADER_FRAGMENT,
               st->validate_all_dirty_states =
      screen->get_param(screen, PIPE_CAP_VALIDATE_ALL_DIRTY_STATES)
      st->can_null_texture =
      screen->get_param(screen, PIPE_CAP_NULL_TEXTURES)
         util_throttle_init(&st->throttle,
                  /* GL limits and extensions */
   st_init_limits(screen, &ctx->Const, &ctx->Extensions, ctx->API);
   st_init_extensions(screen, &ctx->Const,
            if (st_have_perfmon(st)) {
                  if (st_have_perfquery(st)) {
                  /* Enable shader-based fallbacks for ARB_color_buffer_float if needed. */
   if (screen->get_param(screen, PIPE_CAP_VERTEX_COLOR_UNCLAMPED)) {
      if (!screen->get_param(screen, PIPE_CAP_VERTEX_COLOR_CLAMPED)) {
                  if (!screen->get_param(screen, PIPE_CAP_FRAGMENT_COLOR_CLAMPED)) {
                  /* For drivers which cannot do color clamping, it's better to just
   * disable ARB_color_buffer_float in the core profile, because
   * the clamping is deprecated there anyway. */
   if (_mesa_is_desktop_gl_core(ctx) &&
      (st->clamp_frag_color_in_shader || st->clamp_vert_color_in_shader)) {
   st->clamp_vert_color_in_shader = GL_FALSE;
   st->clamp_frag_color_in_shader = GL_FALSE;
                  /* called after _mesa_create_context/_mesa_init_point, fix default user
   * settable max point size up
   */
   ctx->Point.MaxSize = MAX2(ctx->Const.MaxPointSize,
            ctx->Const.NoClippingOnCopyTex = screen->get_param(screen,
            ctx->Const.ForceFloat32TexNearest =
                              /* NIR drivers that support tess shaders and compact arrays need to use
   * GLSLTessLevelsAsInputs / PIPE_CAP_GLSL_TESS_LEVELS_AS_INPUTS. The NIR
   * linker doesn't support linking these as compat arrays of sysvals.
   */
   assert(ctx->Const.GLSLTessLevelsAsInputs ||
      !screen->get_param(screen, PIPE_CAP_NIR_COMPACT_ARRAYS) ||
         /* Set which shader types can be compiled at link time. */
   st->shader_has_one_variant[MESA_SHADER_VERTEX] =
         st->has_shareable_shaders &&
   !st->clamp_vert_color_in_shader &&
            st->shader_has_one_variant[MESA_SHADER_FRAGMENT] =
         st->has_shareable_shaders &&
   !st->lower_flatshade &&
   !st->lower_alpha_test &&
   !st->clamp_frag_color_in_shader &&
            st->shader_has_one_variant[MESA_SHADER_TESS_CTRL] = st->has_shareable_shaders;
   st->shader_has_one_variant[MESA_SHADER_TESS_EVAL] =
         st->has_shareable_shaders &&
   !st->clamp_vert_color_in_shader &&
            st->shader_has_one_variant[MESA_SHADER_GEOMETRY] =
         st->has_shareable_shaders &&
   !st->clamp_vert_color_in_shader &&
   !st->lower_point_size &&
            if (util_get_cpu_caps()->num_L3_caches == 1 ||
      !st->pipe->set_context_param)
                  if (ctx->Const.ForceGLNamesReuse && ctx->Shared->RefCount == 1) {
      _mesa_HashEnableNameReuse(ctx->Shared->TexObjects);
   _mesa_HashEnableNameReuse(ctx->Shared->ShaderObjects);
   _mesa_HashEnableNameReuse(ctx->Shared->BufferObjects);
   _mesa_HashEnableNameReuse(ctx->Shared->SamplerObjects);
   _mesa_HashEnableNameReuse(ctx->Shared->FrameBuffers);
   _mesa_HashEnableNameReuse(ctx->Shared->RenderBuffers);
   _mesa_HashEnableNameReuse(ctx->Shared->MemoryObjects);
      }
   /* SPECviewperf13/sw-04 crashes since a56849ddda6 if Mesa is build with
   * -O3 on gcc 7.5, which doesn't happen with ForceGLNamesReuse, which is
   * the default setting for SPECviewperf because it simulates glGen behavior
   * of closed source drivers.
   */
   if (ctx->Const.ForceGLNamesReuse)
            _mesa_override_extensions(ctx);
            if (ctx->Version == 0 ||
      !_mesa_initialize_dispatch_tables(ctx)) {
   /* This can happen when a core profile was requested, but the driver
   * does not support some features of GL 3.1 or later.
   */
   st_destroy_context_priv(st, false);
               if (_mesa_has_compute_shaders(ctx) &&
      st->transcode_astc && !st_init_texcompress_compute(st)) {
   /* Transcoding ASTC to DXT5 using compute shaders can provide a
   * significant performance benefit over the CPU path. It isn't strictly
   * necessary to fail if we can't use the compute shader path, but it's
   * very convenient to do so. This should be rare.
   */
   st_destroy_context_priv(st, false);
               /* This must be done after extensions are initialized to enable persistent
   * mappings immediately.
   */
                     /* Initialize context's winsys buffers list */
            list_inithead(&st->zombie_sampler_views.list.node);
   simple_mtx_init(&st->zombie_sampler_views.mutex, mtx_plain);
   list_inithead(&st->zombie_shaders.list.node);
            ctx->Const.DriverSupportedPrimMask = screen->get_param(screen, PIPE_CAP_SUPPORTED_PRIM_MODES) |
                           }
      void
   st_set_background_context(struct gl_context *ctx,
         {
      struct st_context *st = ctx->st;
            assert(fscreen->set_background_context);
      }
      static void
   st_init_driver_functions(struct pipe_screen *screen,
               {
                        functions->NewProgram = _mesa_new_program;
            /* GL_ARB_get_program_binary */
   functions->ShaderCacheSerializeDriverBlob =  st_serialise_nir_program;
   functions->ProgramBinarySerializeDriverBlob =
         functions->ProgramBinaryDeserializeDriverBlob =
      }
         struct st_context *
   st_create_context(gl_api api, struct pipe_context *pipe,
                     const struct gl_config *visual,
   {
      struct gl_context *ctx;
   struct gl_context *shareCtx = share ? share->ctx : NULL;
   struct dd_function_table funcs;
            memset(&funcs, 0, sizeof(funcs));
            /* gl_context must be 16-byte aligned due to the alignment on GLmatrix. */
   ctx = align_malloc(sizeof(struct gl_context), 16);
   if (!ctx)
                  ctx->pipe = pipe;
            if (!_mesa_initialize_context(ctx, api, no_error, visual, shareCtx, &funcs)) {
      align_free(ctx);
                        if (pipe->screen->get_disk_shader_cache)
            /* XXX: need a capability bit in gallium to query if the pipe
   * driver prefers DP4 or MUL/MAD for vertex transformation.
   */
   if (debug_get_option_mesa_mvp_dp4())
            if (pipe->screen->get_param(pipe->screen, PIPE_CAP_INVALIDATE_BUFFER))
            if (pipe->screen->get_param(pipe->screen, PIPE_CAP_STRING_MARKER))
            st = st_create_context_priv(ctx, pipe, options);
   if (!st) {
      _mesa_free_context_data(ctx, true);
                  }
         /**
   * When we destroy a context, we must examine all texture objects to
   * find/release any sampler views created by that context.
   *
   * This callback is called per-texture object.  It releases all the
   * texture's sampler views which belong to the context.
   */
   static void
   destroy_tex_sampler_cb(void *data, void *userData)
   {
      struct gl_texture_object *texObj = (struct gl_texture_object *) data;
               }
      static void
   destroy_framebuffer_attachment_sampler_cb(void *data, void *userData)
   {
      struct gl_framebuffer* glfb = (struct gl_framebuffer*) data;
            for (unsigned i = 0; i < BUFFER_COUNT; i++) {
      struct gl_renderbuffer_attachment *att = &glfb->Attachment[i];
   if (att->Texture) {
   st_texture_release_context_sampler_view(st, att->Texture);
         }
      void
   st_destroy_context(struct st_context *st)
   {
      struct gl_context *ctx = st->ctx;
   struct gl_framebuffer *stfb, *next;
   struct gl_framebuffer *save_drawbuffer;
            /* Save the current context and draw/read buffers*/
   GET_CURRENT_CONTEXT(save_ctx);
   if (save_ctx) {
      save_drawbuffer = save_ctx->WinSysDrawBuffer;
      } else {
                  /*
   * We need to bind the context we're deleting so that
   * _mesa_reference_texobj_() uses this context when deleting textures.
   * Similarly for framebuffer objects, etc.
   */
            /* This must be called first so that glthread has a chance to finish */
                     /* For the fallback textures, free any sampler views belonging to this
   * context.
   */
   for (unsigned i = 0; i < NUM_TEXTURE_TARGETS; i++) {
      for (unsigned j = 0; j < ARRAY_SIZE(ctx->Shared->FallbackTex[0]); j++) {
      struct gl_texture_object *stObj =
         if (stObj) {
                        st_release_program(st, &st->fp);
   st_release_program(st, &st->gp);
   st_release_program(st, &st->vp);
   st_release_program(st, &st->tcp);
   st_release_program(st, &st->tep);
            if (st->hw_select_shaders) {
      hash_table_foreach(st->hw_select_shaders, entry)
                     /* release framebuffer in the winsys buffers list */
   LIST_FOR_EACH_ENTRY_SAFE_REV(stfb, next, &st->winsys_buffers, head) {
                           pipe_sampler_view_reference(&st->pixel_xfer.pixelmap_sampler_view, NULL);
                                       simple_mtx_destroy(&st->zombie_sampler_views.mutex);
            /* Do not release debug_output yet because it might be in use by other threads.
   * These threads will be terminated by _mesa_free_context_data and
   * st_destroy_context_priv.
   */
            /* This will free the st_context too, so 'st' must not be accessed
   * afterwards. */
   st_destroy_context_priv(st, true);
                              if (save_ctx == ctx) {
      /* unbind the context we just deleted */
      } else {
      /* Restore the current context and draw/read buffers (may be NULL) */
         }
      const struct nir_shader_compiler_options *
   st_get_nir_compiler_options(struct st_context *st, gl_shader_stage stage)
   {
         }
