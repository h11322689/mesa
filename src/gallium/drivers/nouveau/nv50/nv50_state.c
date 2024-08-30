   /*
   * Copyright 2010 Christoph Bumiller
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included in
   * all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   * OTHER DEALINGS IN THE SOFTWARE.
   */
      #include "pipe/p_defines.h"
   #include "util/u_framebuffer.h"
   #include "util/u_helpers.h"
   #include "util/u_inlines.h"
   #include "util/u_transfer.h"
   #include "util/format_srgb.h"
      #include "tgsi/tgsi_parse.h"
   #include "compiler/nir/nir.h"
   #include "nir/tgsi_to_nir.h"
      #include "nv50/nv50_stateobj.h"
   #include "nv50/nv50_context.h"
   #include "nv50/nv50_query_hw.h"
      #include "nv50/nv50_3d.xml.h"
   #include "nv50/g80_texture.xml.h"
      #include "nouveau_gldefs.h"
      /* Caveats:
   *  ! pipe_sampler_state.unnormalized_coords is ignored - rectangle textures
   *     will use non-normalized coordinates, everything else won't
   *    (The relevant bit is in the TIC entry and not the TSC entry.)
   *
   *  ! pipe_sampler_state.seamless_cube_map is ignored - seamless filtering is
   *     always activated on NVA0 +
   *    (Give me the global bit, otherwise it's not worth the CPU work.)
   *
   *  ! pipe_sampler_state.border_color is not swizzled according to the texture
   *     swizzle in pipe_sampler_view
   *    (This will be ugly with indirect independent texture/sampler access,
   *     we'd have to emulate the logic in the shader. GL doesn't have that,
   *     D3D doesn't have swizzle, if we knew what we were implementing we'd be
   *     good.)
   *
   *  ! pipe_rasterizer_state.line_last_pixel is ignored - it is never drawn
   *
   *  ! pipe_rasterizer_state.flatshade_first also applies to QUADS
   *    (There's a GL query for that, forcing an exception is just ridiculous.)
   *
   *  ! pipe_rasterizer_state.sprite_coord_enable is masked with 0xff on NVC0
   *    (The hardware only has 8 slots meant for TexCoord and we have to assign
   *     in advance to maintain elegant separate shader objects.)
   */
      static inline uint32_t
   nv50_colormask(unsigned mask)
   {
               if (mask & PIPE_MASK_R)
         if (mask & PIPE_MASK_G)
         if (mask & PIPE_MASK_B)
         if (mask & PIPE_MASK_A)
               }
      #define NV50_BLEND_FACTOR_CASE(a, b) \
            static inline uint32_t
   nv50_blend_fac(unsigned factor)
   {
      switch (factor) {
   NV50_BLEND_FACTOR_CASE(ONE, ONE);
   NV50_BLEND_FACTOR_CASE(SRC_COLOR, SRC_COLOR);
   NV50_BLEND_FACTOR_CASE(SRC_ALPHA, SRC_ALPHA);
   NV50_BLEND_FACTOR_CASE(DST_ALPHA, DST_ALPHA);
   NV50_BLEND_FACTOR_CASE(DST_COLOR, DST_COLOR);
   NV50_BLEND_FACTOR_CASE(SRC_ALPHA_SATURATE, SRC_ALPHA_SATURATE);
   NV50_BLEND_FACTOR_CASE(CONST_COLOR, CONSTANT_COLOR);
   NV50_BLEND_FACTOR_CASE(CONST_ALPHA, CONSTANT_ALPHA);
   NV50_BLEND_FACTOR_CASE(SRC1_COLOR, SRC1_COLOR);
   NV50_BLEND_FACTOR_CASE(SRC1_ALPHA, SRC1_ALPHA);
   NV50_BLEND_FACTOR_CASE(ZERO, ZERO);
   NV50_BLEND_FACTOR_CASE(INV_SRC_COLOR, ONE_MINUS_SRC_COLOR);
   NV50_BLEND_FACTOR_CASE(INV_SRC_ALPHA, ONE_MINUS_SRC_ALPHA);
   NV50_BLEND_FACTOR_CASE(INV_DST_ALPHA, ONE_MINUS_DST_ALPHA);
   NV50_BLEND_FACTOR_CASE(INV_DST_COLOR, ONE_MINUS_DST_COLOR);
   NV50_BLEND_FACTOR_CASE(INV_CONST_COLOR, ONE_MINUS_CONSTANT_COLOR);
   NV50_BLEND_FACTOR_CASE(INV_CONST_ALPHA, ONE_MINUS_CONSTANT_ALPHA);
   NV50_BLEND_FACTOR_CASE(INV_SRC1_COLOR, ONE_MINUS_SRC1_COLOR);
   NV50_BLEND_FACTOR_CASE(INV_SRC1_ALPHA, ONE_MINUS_SRC1_ALPHA);
   default:
            }
      static void *
   nv50_blend_state_create(struct pipe_context *pipe,
         {
      struct nv50_blend_stateobj *so = CALLOC_STRUCT(nv50_blend_stateobj);
   int i;
   bool emit_common_func = cso->rt[0].blend_enable;
            if (nv50_context(pipe)->screen->tesla->oclass >= NVA3_3D_CLASS) {
      SB_BEGIN_3D(so, BLEND_INDEPENDENT, 1);
                        SB_BEGIN_3D(so, COLOR_MASK_COMMON, 1);
            SB_BEGIN_3D(so, BLEND_ENABLE_COMMON, 1);
            if (cso->independent_blend_enable) {
      SB_BEGIN_3D(so, BLEND_ENABLE(0), 8);
   for (i = 0; i < 8; ++i) {
      SB_DATA(so, cso->rt[i].blend_enable);
   if (cso->rt[i].blend_enable)
               if (nv50_context(pipe)->screen->tesla->oclass >= NVA3_3D_CLASS) {
               for (i = 0; i < 8; ++i) {
      if (!cso->rt[i].blend_enable)
         SB_BEGIN_3D_(so, NVA3_3D_IBLEND_EQUATION_RGB(i), 6);
   SB_DATA     (so, nvgl_blend_eqn(cso->rt[i].rgb_func));
   SB_DATA     (so, nv50_blend_fac(cso->rt[i].rgb_src_factor));
   SB_DATA     (so, nv50_blend_fac(cso->rt[i].rgb_dst_factor));
   SB_DATA     (so, nvgl_blend_eqn(cso->rt[i].alpha_func));
   SB_DATA     (so, nv50_blend_fac(cso->rt[i].alpha_src_factor));
            } else {
      SB_BEGIN_3D(so, BLEND_ENABLE(0), 1);
               if (emit_common_func) {
      SB_BEGIN_3D(so, BLEND_EQUATION_RGB, 5);
   SB_DATA    (so, nvgl_blend_eqn(cso->rt[0].rgb_func));
   SB_DATA    (so, nv50_blend_fac(cso->rt[0].rgb_src_factor));
   SB_DATA    (so, nv50_blend_fac(cso->rt[0].rgb_dst_factor));
   SB_DATA    (so, nvgl_blend_eqn(cso->rt[0].alpha_func));
   SB_DATA    (so, nv50_blend_fac(cso->rt[0].alpha_src_factor));
   SB_BEGIN_3D(so, BLEND_FUNC_DST_ALPHA, 1);
               if (cso->logicop_enable) {
      SB_BEGIN_3D(so, LOGIC_OP_ENABLE, 2);
   SB_DATA    (so, 1);
      } else {
      SB_BEGIN_3D(so, LOGIC_OP_ENABLE, 1);
               if (cso->independent_blend_enable) {
      SB_BEGIN_3D(so, COLOR_MASK(0), 8);
   for (i = 0; i < 8; ++i)
      } else {
      SB_BEGIN_3D(so, COLOR_MASK(0), 1);
               ms = 0;
   if (cso->alpha_to_coverage)
         if (cso->alpha_to_one)
            SB_BEGIN_3D(so, MULTISAMPLE_CTRL, 1);
            assert(so->size <= ARRAY_SIZE(so->state));
      }
      static void
   nv50_blend_state_bind(struct pipe_context *pipe, void *hwcso)
   {
               nv50->blend = hwcso;
      }
      static void
   nv50_blend_state_delete(struct pipe_context *pipe, void *hwcso)
   {
         }
      /* NOTE: ignoring line_last_pixel */
   static void *
   nv50_rasterizer_state_create(struct pipe_context *pipe,
         {
      struct nv50_rasterizer_stateobj *so;
            so = CALLOC_STRUCT(nv50_rasterizer_stateobj);
   if (!so)
               #ifndef NV50_SCISSORS_CLIPPING
      for (int i = 0; i < NV50_MAX_VIEWPORTS; i++) {
      SB_BEGIN_3D(so, SCISSOR_ENABLE(i), 1);
         #endif
         SB_BEGIN_3D(so, SHADE_MODEL, 1);
   SB_DATA    (so, cso->flatshade ? NV50_3D_SHADE_MODEL_FLAT :
         SB_BEGIN_3D(so, PROVOKING_VERTEX_LAST, 1);
   SB_DATA    (so, !cso->flatshade_first);
   SB_BEGIN_3D(so, VERTEX_TWO_SIDE_ENABLE, 1);
            SB_BEGIN_3D(so, FRAG_COLOR_CLAMP_EN, 1);
            SB_BEGIN_3D(so, MULTISAMPLE_ENABLE, 1);
            SB_BEGIN_3D(so, LINE_WIDTH, 1);
   SB_DATA    (so, fui(cso->line_width));
   SB_BEGIN_3D(so, LINE_SMOOTH_ENABLE, 1);
            SB_BEGIN_3D(so, LINE_STIPPLE_ENABLE, 1);
   if (cso->line_stipple_enable) {
      SB_DATA    (so, 1);
   SB_BEGIN_3D(so, LINE_STIPPLE, 1);
   SB_DATA    (so, (cso->line_stipple_pattern << 8) |
      } else {
                  if (!cso->point_size_per_vertex) {
      SB_BEGIN_3D(so, POINT_SIZE, 1);
      }
   SB_BEGIN_3D(so, POINT_SPRITE_ENABLE, 1);
   SB_DATA    (so, cso->point_quad_rasterization);
   SB_BEGIN_3D(so, POINT_SMOOTH_ENABLE, 1);
            SB_BEGIN_3D(so, POLYGON_MODE_FRONT, 3);
   SB_DATA    (so, nvgl_polygon_mode(cso->fill_front));
   SB_DATA    (so, nvgl_polygon_mode(cso->fill_back));
            SB_BEGIN_3D(so, CULL_FACE_ENABLE, 3);
   SB_DATA    (so, cso->cull_face != PIPE_FACE_NONE);
   SB_DATA    (so, cso->front_ccw ? NV50_3D_FRONT_FACE_CCW :
         switch (cso->cull_face) {
   case PIPE_FACE_FRONT_AND_BACK:
      SB_DATA(so, NV50_3D_CULL_FACE_FRONT_AND_BACK);
      case PIPE_FACE_FRONT:
      SB_DATA(so, NV50_3D_CULL_FACE_FRONT);
      case PIPE_FACE_BACK:
   default:
   SB_DATA(so, NV50_3D_CULL_FACE_BACK);
   break;
            SB_BEGIN_3D(so, POLYGON_STIPPLE_ENABLE, 1);
   SB_DATA    (so, cso->poly_stipple_enable);
   SB_BEGIN_3D(so, POLYGON_OFFSET_POINT_ENABLE, 3);
   SB_DATA    (so, cso->offset_point);
   SB_DATA    (so, cso->offset_line);
            if (cso->offset_point || cso->offset_line || cso->offset_tri) {
      SB_BEGIN_3D(so, POLYGON_OFFSET_FACTOR, 1);
   SB_DATA    (so, fui(cso->offset_scale));
   SB_BEGIN_3D(so, POLYGON_OFFSET_UNITS, 1);
   SB_DATA    (so, fui(cso->offset_units * 2.0f));
   SB_BEGIN_3D(so, POLYGON_OFFSET_CLAMP, 1);
               if (cso->depth_clip_near) {
         } else {
      reg =
      NV50_3D_VIEW_VOLUME_CLIP_CTRL_DEPTH_CLAMP_NEAR |
   NV50_3D_VIEW_VOLUME_CLIP_CTRL_DEPTH_CLAMP_FAR |
      #ifndef NV50_SCISSORS_CLIPPING
      reg |=
      NV50_3D_VIEW_VOLUME_CLIP_CTRL_UNK7 |
   #endif
      SB_BEGIN_3D(so, VIEW_VOLUME_CLIP_CTRL, 1);
            SB_BEGIN_3D(so, DEPTH_CLIP_NEGATIVE_Z, 1);
            SB_BEGIN_3D(so, PIXEL_CENTER_INTEGER, 1);
            assert(so->size <= ARRAY_SIZE(so->state));
      }
      static void
   nv50_rasterizer_state_bind(struct pipe_context *pipe, void *hwcso)
   {
               nv50->rast = hwcso;
      }
      static void
   nv50_rasterizer_state_delete(struct pipe_context *pipe, void *hwcso)
   {
         }
      static void *
   nv50_zsa_state_create(struct pipe_context *pipe,
         {
                        SB_BEGIN_3D(so, DEPTH_WRITE_ENABLE, 1);
   SB_DATA    (so, cso->depth_writemask);
   SB_BEGIN_3D(so, DEPTH_TEST_ENABLE, 1);
   if (cso->depth_enabled) {
      SB_DATA    (so, 1);
   SB_BEGIN_3D(so, DEPTH_TEST_FUNC, 1);
      } else {
                  SB_BEGIN_3D(so, DEPTH_BOUNDS_EN, 1);
   if (cso->depth_bounds_test) {
      SB_DATA    (so, 1);
   SB_BEGIN_3D(so, DEPTH_BOUNDS(0), 2);
   SB_DATA    (so, fui(cso->depth_bounds_min));
      } else {
                  if (cso->stencil[0].enabled) {
      SB_BEGIN_3D(so, STENCIL_ENABLE, 5);
   SB_DATA    (so, 1);
   SB_DATA    (so, nvgl_stencil_op(cso->stencil[0].fail_op));
   SB_DATA    (so, nvgl_stencil_op(cso->stencil[0].zfail_op));
   SB_DATA    (so, nvgl_stencil_op(cso->stencil[0].zpass_op));
   SB_DATA    (so, nvgl_comparison_op(cso->stencil[0].func));
   SB_BEGIN_3D(so, STENCIL_FRONT_MASK, 2);
   SB_DATA    (so, cso->stencil[0].writemask);
      } else {
      SB_BEGIN_3D(so, STENCIL_ENABLE, 1);
               if (cso->stencil[1].enabled) {
      assert(cso->stencil[0].enabled);
   SB_BEGIN_3D(so, STENCIL_TWO_SIDE_ENABLE, 5);
   SB_DATA    (so, 1);
   SB_DATA    (so, nvgl_stencil_op(cso->stencil[1].fail_op));
   SB_DATA    (so, nvgl_stencil_op(cso->stencil[1].zfail_op));
   SB_DATA    (so, nvgl_stencil_op(cso->stencil[1].zpass_op));
   SB_DATA    (so, nvgl_comparison_op(cso->stencil[1].func));
   SB_BEGIN_3D(so, STENCIL_BACK_MASK, 2);
   SB_DATA    (so, cso->stencil[1].writemask);
      } else {
      SB_BEGIN_3D(so, STENCIL_TWO_SIDE_ENABLE, 1);
               SB_BEGIN_3D(so, ALPHA_TEST_ENABLE, 1);
   if (cso->alpha_enabled) {
      SB_DATA    (so, 1);
   SB_BEGIN_3D(so, ALPHA_TEST_REF, 2);
   SB_DATA    (so, fui(cso->alpha_ref_value));
      } else {
                  SB_BEGIN_3D(so, CB_ADDR, 1);
   SB_DATA    (so, NV50_CB_AUX_ALPHATEST_OFFSET << (8 - 2) | NV50_CB_AUX);
   SB_BEGIN_3D(so, CB_DATA(0), 1);
            assert(so->size <= ARRAY_SIZE(so->state));
      }
      static void
   nv50_zsa_state_bind(struct pipe_context *pipe, void *hwcso)
   {
               nv50->zsa = hwcso;
      }
      static void
   nv50_zsa_state_delete(struct pipe_context *pipe, void *hwcso)
   {
         }
      /* ====================== SAMPLERS AND TEXTURES ================================
   */
      static inline unsigned
   nv50_tsc_wrap_mode(unsigned wrap)
   {
      switch (wrap) {
   case PIPE_TEX_WRAP_REPEAT:
         case PIPE_TEX_WRAP_MIRROR_REPEAT:
         case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
         case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
         case PIPE_TEX_WRAP_CLAMP:
         case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
         case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
         case PIPE_TEX_WRAP_MIRROR_CLAMP:
         default:
      NOUVEAU_ERR("unknown wrap mode: %d\n", wrap);
         }
      void *
   nv50_sampler_state_create(struct pipe_context *pipe,
         {
      struct nv50_tsc_entry *so = MALLOC_STRUCT(nv50_tsc_entry);
                     so->tsc[0] = (0x00026000 |
                        switch (cso->mag_img_filter) {
   case PIPE_TEX_FILTER_LINEAR:
      so->tsc[1] = G80_TSC_1_MAG_FILTER_LINEAR;
      case PIPE_TEX_FILTER_NEAREST:
   default:
      so->tsc[1] = G80_TSC_1_MAG_FILTER_NEAREST;
               switch (cso->min_img_filter) {
   case PIPE_TEX_FILTER_LINEAR:
      so->tsc[1] |= G80_TSC_1_MIN_FILTER_LINEAR;
      case PIPE_TEX_FILTER_NEAREST:
   default:
      so->tsc[1] |= G80_TSC_1_MIN_FILTER_NEAREST;
               switch (cso->min_mip_filter) {
   case PIPE_TEX_MIPFILTER_LINEAR:
      so->tsc[1] |= G80_TSC_1_MIP_FILTER_LINEAR;
      case PIPE_TEX_MIPFILTER_NEAREST:
      so->tsc[1] |= G80_TSC_1_MIP_FILTER_NEAREST;
      case PIPE_TEX_MIPFILTER_NONE:
   default:
      so->tsc[1] |= G80_TSC_1_MIP_FILTER_NONE;
               if (nouveau_screen(pipe->screen)->class_3d >= NVE4_3D_CLASS) {
      if (cso->seamless_cube_map)
         if (cso->unnormalized_coords)
      } else {
                  if (nouveau_screen(pipe->screen)->class_3d >= GM200_3D_CLASS) {
      if (cso->reduction_mode == PIPE_TEX_REDUCTION_MIN)
         if (cso->reduction_mode == PIPE_TEX_REDUCTION_MAX)
               if (cso->max_anisotropy >= 16)
         else
   if (cso->max_anisotropy >= 12)
         else {
               if (cso->max_anisotropy >= 4)
         else
   if (cso->max_anisotropy >= 2)
               if (cso->compare_mode == PIPE_TEX_COMPARE_R_TO_TEXTURE) {
      /* NOTE: must be deactivated for non-shadow textures */
   so->tsc[0] |= (1 << 9);
               f[0] = CLAMP(cso->lod_bias, -16.0f, 15.0f);
            f[0] = CLAMP(cso->min_lod, 0.0f, 15.0f);
   f[1] = CLAMP(cso->max_lod, 0.0f, 15.0f);
   so->tsc[2] =
            so->tsc[2] |=
         so->tsc[3] =
         so->tsc[3] |=
            so->tsc[4] = fui(cso->border_color.f[0]);
   so->tsc[5] = fui(cso->border_color.f[1]);
   so->tsc[6] = fui(cso->border_color.f[2]);
               }
      static void
   nv50_sampler_state_delete(struct pipe_context *pipe, void *hwcso)
   {
               for (s = 0; s < NV50_MAX_SHADER_STAGES; ++s) {
      assert(nv50_context(pipe)->num_samplers[s] <= PIPE_MAX_SAMPLERS);
   for (i = 0; i < nv50_context(pipe)->num_samplers[s]; ++i)
      if (nv50_context(pipe)->samplers[s][i] == hwcso)
                        }
      static inline void
   nv50_stage_sampler_states_bind(struct nv50_context *nv50, int s,
         {
      unsigned highest_found = 0;
            assert(nr <= PIPE_MAX_SAMPLERS);
   for (i = 0; i < nr; ++i) {
      struct nv50_tsc_entry *hwcso = hwcsos ? nv50_tsc_entry(hwcsos[i]) : NULL;
            if (hwcso)
            nv50->samplers[s][i] = hwcso;
   if (old)
      }
   assert(nv50->num_samplers[s] <= PIPE_MAX_SAMPLERS);
   if (nr >= nv50->num_samplers[s])
      }
      static void
   nv50_bind_sampler_states(struct pipe_context *pipe,
               {
               assert(start == 0);
   nv50_stage_sampler_states_bind(nv50_context(pipe), s, num_samplers,
            if (unlikely(s == NV50_SHADER_STAGE_COMPUTE))
         else
      }
            /* NOTE: only called when not referenced anywhere, won't be bound */
   static void
   nv50_sampler_view_destroy(struct pipe_context *pipe,
         {
                           }
      static inline void
   nv50_stage_set_sampler_views(struct nv50_context *nv50, int s,
               {
               assert(nr <= PIPE_MAX_SAMPLERS);
   for (i = 0; i < nr; ++i) {
      struct pipe_sampler_view *view = views ? views[i] : NULL;
   struct nv50_tic_entry *old = nv50_tic_entry(nv50->textures[s][i]);
   if (old)
            if (view && view->texture) {
      struct pipe_resource *res = view->texture;
   if (res->target == PIPE_BUFFER &&
      (res->flags & PIPE_RESOURCE_FLAG_MAP_COHERENT))
      else
      } else {
                  if (take_ownership) {
      pipe_sampler_view_reference(&nv50->textures[s][i], NULL);
      } else {
                     assert(nv50->num_textures[s] <= PIPE_MAX_SAMPLERS);
   for (i = nr; i < nv50->num_textures[s]; ++i) {
      struct nv50_tic_entry *old = nv50_tic_entry(nv50->textures[s][i]);
   if (!old)
                                 }
      static void
   nv50_set_sampler_views(struct pipe_context *pipe, enum pipe_shader_type shader,
                           {
      struct nv50_context *nv50 = nv50_context(pipe);
            assert(start == 0);
            if (unlikely(s == NV50_SHADER_STAGE_COMPUTE)) {
                  } else {
                     }
            /* ============================= SHADERS =======================================
   */
      static void *
   nv50_sp_state_create(struct pipe_context *pipe,
               {
               prog = CALLOC_STRUCT(nv50_program);
   if (!prog)
                     switch (cso->type) {
   case PIPE_SHADER_IR_TGSI:
      prog->nir = tgsi_to_nir(cso->tokens, pipe->screen, false);
      case PIPE_SHADER_IR_NIR:
      prog->nir = cso->ir.nir;
      default:
      assert(!"unsupported IR!");
   free(prog);
               if (cso->stream_output.num_outputs)
            prog->translated = nv50_program_translate(
                     }
      static void
   nv50_sp_state_delete(struct pipe_context *pipe, void *hwcso)
   {
      struct nv50_context *nv50 = nv50_context(pipe);
            simple_mtx_lock(&nv50->screen->state_lock);
   nv50_program_destroy(nv50, prog);
            ralloc_free(prog->nir);
      }
      static void *
   nv50_vp_state_create(struct pipe_context *pipe,
         {
         }
      static void
   nv50_vp_state_bind(struct pipe_context *pipe, void *hwcso)
   {
               nv50->vertprog = hwcso;
      }
      static void *
   nv50_fp_state_create(struct pipe_context *pipe,
         {
         }
      static void
   nv50_fp_state_bind(struct pipe_context *pipe, void *hwcso)
   {
               nv50->fragprog = hwcso;
      }
      static void *
   nv50_gp_state_create(struct pipe_context *pipe,
         {
         }
      static void
   nv50_gp_state_bind(struct pipe_context *pipe, void *hwcso)
   {
               nv50->gmtyprog = hwcso;
      }
      static void *
   nv50_cp_state_create(struct pipe_context *pipe,
         {
               prog = CALLOC_STRUCT(nv50_program);
   if (!prog)
                  switch(cso->ir_type) {
   case PIPE_SHADER_IR_TGSI: {
      const struct tgsi_token *tokens = cso->prog;
   prog->nir = tgsi_to_nir(tokens, pipe->screen, false);
      }
   case PIPE_SHADER_IR_NIR:
      prog->nir = (nir_shader *)cso->prog;
      default:
      assert(!"unsupported IR!");
   free(prog);
               prog->cp.smem_size = cso->static_shared_mem;
               }
      static void
   nv50_cp_state_bind(struct pipe_context *pipe, void *hwcso)
   {
               nv50->compprog = hwcso;
      }
      static void
   nv50_get_compute_state_info(struct pipe_context *pipe, void *hwcso,
         {
      struct nv50_context *nv50 = nv50_context(pipe);
   struct nv50_program *prog = (struct nv50_program *)hwcso;
   uint16_t obj_class = nv50->screen->compute->oclass;
   uint32_t smregs = obj_class >= NVA3_COMPUTE_CLASS ? 16384 : 8192;
            info->max_threads = MIN2(ROUND_DOWN_TO(threads, 32), 512);
   info->private_memory = prog->tls_space;
   info->preferred_simd_size = 32;
      }
      static void
   nv50_set_constant_buffer(struct pipe_context *pipe,
                     {
      struct nv50_context *nv50 = nv50_context(pipe);
   struct pipe_resource *res = cb ? cb->buffer : NULL;
   const unsigned s = nv50_context_shader_stage(shader);
            if (unlikely(shader == PIPE_SHADER_COMPUTE)) {
      if (nv50->constbuf[s][i].user)
         else
   if (nv50->constbuf[s][i].u.buf)
               } else {
      if (nv50->constbuf[s][i].user)
         else
   if (nv50->constbuf[s][i].u.buf)
               }
            if (nv50->constbuf[s][i].u.buf)
            if (take_ownership) {
      pipe_resource_reference(&nv50->constbuf[s][i].u.buf, NULL);
      } else {
                  nv50->constbuf[s][i].user = (cb && cb->user_buffer) ? true : false;
   if (nv50->constbuf[s][i].user) {
      nv50->constbuf[s][i].u.data = cb->user_buffer;
   nv50->constbuf[s][i].size = MIN2(cb->buffer_size, 0x10000);
   nv50->constbuf_valid[s] |= 1 << i;
      } else
   if (cb) {
      nv50->constbuf[s][i].offset = cb->buffer_offset;
   nv50->constbuf[s][i].size = MIN2(align(cb->buffer_size, 0x100), 0x10000);
   nv50->constbuf_valid[s] |= 1 << i;
   if (res && res->flags & PIPE_RESOURCE_FLAG_MAP_COHERENT)
         else
      }
   else {
      nv50->constbuf_valid[s] &= ~(1 << i);
         }
      /* =============================================================================
   */
      static void
   nv50_set_blend_color(struct pipe_context *pipe,
         {
               nv50->blend_colour = *bcol;
      }
      static void
   nv50_set_stencil_ref(struct pipe_context *pipe,
         {
               nv50->stencil_ref = sr;
      }
      static void
   nv50_set_clip_state(struct pipe_context *pipe,
         {
                           }
      static void
   nv50_set_sample_mask(struct pipe_context *pipe, unsigned sample_mask)
   {
               nv50->sample_mask = sample_mask;
      }
      static void
   nv50_set_min_samples(struct pipe_context *pipe, unsigned min_samples)
   {
               if (nv50->min_samples != min_samples) {
      nv50->min_samples = min_samples;
         }
      static void
   nv50_set_framebuffer_state(struct pipe_context *pipe,
         {
                                    }
      static void
   nv50_set_polygon_stipple(struct pipe_context *pipe,
         {
               nv50->stipple = *stipple;
      }
      static void
   nv50_set_scissor_states(struct pipe_context *pipe,
                     {
      struct nv50_context *nv50 = nv50_context(pipe);
            assert(start_slot + num_scissors <= NV50_MAX_VIEWPORTS);
   for (i = 0; i < num_scissors; i++) {
      if (!memcmp(&nv50->scissors[start_slot + i], &scissor[i], sizeof(*scissor)))
         nv50->scissors[start_slot + i] = scissor[i];
   nv50->scissors_dirty |= 1 << (start_slot + i);
         }
      static void
   nv50_set_viewport_states(struct pipe_context *pipe,
                     {
      struct nv50_context *nv50 = nv50_context(pipe);
            assert(start_slot + num_viewports <= NV50_MAX_VIEWPORTS);
   for (i = 0; i < num_viewports; i++) {
      if (!memcmp(&nv50->viewports[start_slot + i], &vpt[i], sizeof(*vpt)))
         nv50->viewports[start_slot + i] = vpt[i];
   nv50->viewports_dirty |= 1 << (start_slot + i);
         }
      static void
   nv50_set_window_rectangles(struct pipe_context *pipe,
                     {
               nv50->window_rect.inclusive = include;
   nv50->window_rect.rects = MIN2(num_rectangles, NV50_MAX_WINDOW_RECTANGLES);
   memcpy(nv50->window_rect.rect, rectangles,
               }
      static void
   nv50_set_vertex_buffers(struct pipe_context *pipe,
                           {
      struct nv50_context *nv50 = nv50_context(pipe);
            nouveau_bufctx_reset(nv50->bufctx_3d, NV50_BIND_3D_VERTEX);
            util_set_vertex_buffers_count(nv50->vtxbuf, &nv50->num_vtxbufs, vb,
                  unsigned clear_mask = ~u_bit_consecutive(count, unbind_num_trailing_slots);
   nv50->vbo_user &= clear_mask;
   nv50->vbo_constant &= clear_mask;
            if (!vb) {
      clear_mask = ~u_bit_consecutive(0, count);
   nv50->vbo_user &= clear_mask;
   nv50->vbo_constant &= clear_mask;
   nv50->vtxbufs_coherent &= clear_mask;
               for (i = 0; i < count; ++i) {
               if (vb[i].is_user_buffer) {
      nv50->vbo_user |= 1 << dst_index;
      } else {
               if (vb[i].buffer.resource &&
      vb[i].buffer.resource->flags & PIPE_RESOURCE_FLAG_MAP_COHERENT)
      else
            }
      static void
   nv50_vertex_state_bind(struct pipe_context *pipe, void *hwcso)
   {
               nv50->vertex = hwcso;
      }
      static struct pipe_stream_output_target *
   nv50_so_target_create(struct pipe_context *pipe,
               {
      struct nv04_resource *buf = (struct nv04_resource *)res;
   struct nv50_so_target *targ = MALLOC_STRUCT(nv50_so_target);
   if (!targ)
            if (nouveau_context(pipe)->screen->class_3d >= NVA0_3D_CLASS) {
      targ->pq = pipe->create_query(pipe,
         if (!targ->pq) {
      FREE(targ);
         } else {
         }
            targ->pipe.buffer_size = size;
   targ->pipe.buffer_offset = offset;
   targ->pipe.context = pipe;
   targ->pipe.buffer = NULL;
   pipe_resource_reference(&targ->pipe.buffer, res);
            assert(buf->base.target == PIPE_BUFFER);
               }
      static void
   nva0_so_target_save_offset(struct pipe_context *pipe,
               {
               if (serialize) {
      struct nouveau_pushbuf *push = nv50_context(pipe)->base.pushbuf;
   PUSH_SPACE(push, 2);
   BEGIN_NV04(push, SUBC_3D(NV50_GRAPH_SERIALIZE), 1);
               nv50_query(targ->pq)->index = index;
      }
      static void
   nv50_so_target_destroy(struct pipe_context *pipe,
         {
      struct nv50_so_target *targ = nv50_so_target(ptarg);
   if (targ->pq)
         pipe_resource_reference(&targ->pipe.buffer, NULL);
      }
      static void
   nv50_set_stream_output_targets(struct pipe_context *pipe,
                     {
      struct nv50_context *nv50 = nv50_context(pipe);
   unsigned i;
   bool serialize = true;
                     for (i = 0; i < num_targets; ++i) {
      const bool changed = nv50->so_target[i] != targets[i];
   const bool append = (offsets[i] == (unsigned)-1);
   if (!changed && append)
                  if (can_resume && changed && nv50->so_target[i]) {
      nva0_so_target_save_offset(pipe, nv50->so_target[i], i, serialize);
               if (targets[i] && !append) {
      nv50_so_target(targets[i])->clean = true;
                  }
   for (; i < nv50->num_so_targets; ++i) {
      if (can_resume && nv50->so_target[i]) {
      nva0_so_target_save_offset(pipe, nv50->so_target[i], i, serialize);
      }
   pipe_so_target_reference(&nv50->so_target[i], NULL);
      }
            if (nv50->so_targets_dirty) {
      nouveau_bufctx_reset(nv50->bufctx_3d, NV50_BIND_3D_SO);
         }
      static bool
   nv50_bind_images_range(struct nv50_context *nv50,
               {
      const unsigned end = start + nr;
   unsigned mask = 0;
            if (pimages) {
      for (i = start; i < end; ++i) {
                     if (img->resource == pimages[p].resource &&
      img->format == pimages[p].format &&
   img->access == pimages[p].access) {
   if (img->resource == NULL)
         if (img->resource->target == PIPE_BUFFER &&
      img->u.buf.offset == pimages[p].u.buf.offset &&
   img->u.buf.size == pimages[p].u.buf.size)
      if (img->resource->target != PIPE_BUFFER &&
      img->u.tex.first_layer == pimages[p].u.tex.first_layer &&
   img->u.tex.last_layer == pimages[p].u.tex.last_layer &&
                  mask |= (1 << i);
   if (pimages[p].resource)
                        img->format = pimages[p].format;
   img->access = pimages[p].access;
   if (pimages[p].resource && pimages[p].resource->target == PIPE_BUFFER)
                        pipe_resource_reference(
      }
   if (!mask)
      } else {
      mask = ((1 << nr) - 1) << start;
   if (!(nv50->images_valid & mask))
         for (i = start; i < end; ++i) {
         }
      }
                        }
      static void
   nv50_set_shader_images(struct pipe_context *pipe,
                           {
               if (s != NV50_SHADER_STAGE_COMPUTE)
            nv50_bind_images_range(nv50_context(pipe), start + nr,
            if (!nv50_bind_images_range(nv50_context(pipe), start, nr, images))
               }
      static void
   nv50_set_compute_resources(struct pipe_context *pipe,
               {
         }
      static bool
   nv50_bind_buffers_range(struct nv50_context *nv50,
               {
      const unsigned end = start + nr;
   unsigned mask = 0;
            if (pbuffers) {
      for (i = start; i < end; ++i) {
      struct pipe_shader_buffer *buf = &nv50->buffers[i];
   const unsigned p = i - start;
   if (buf->buffer == pbuffers[p].buffer &&
      buf->buffer_offset == pbuffers[p].buffer_offset &&
               mask |= (1 << i);
   if (pbuffers[p].buffer)
         else
         buf->buffer_offset = pbuffers[p].buffer_offset;
   buf->buffer_size = pbuffers[p].buffer_size;
      }
   if (!mask)
      } else {
      mask = ((1 << nr) - 1) << start;
   if (!(nv50->buffers_valid & mask))
         for (i = start; i < end; ++i)
            }
                        }
      static void
   nv50_set_shader_buffers(struct pipe_context *pipe,
                           {
               if (s != NV50_SHADER_STAGE_COMPUTE)
            if (!nv50_bind_buffers_range(nv50_context(pipe), start, nr, buffers))
               }
      static inline void
   nv50_set_global_handle(uint32_t *phandle, struct pipe_resource *res)
   {
      struct nv04_resource *buf = nv04_resource(res);
   if (buf) {
      uint64_t limit = (buf->address + buf->base.width0) - 1;
   if (limit < (1ULL << 32)) {
         } else {
      NOUVEAU_ERR("Cannot map into TGSI_RESOURCE_GLOBAL: "
               } else {
            }
      static void
   nv50_set_global_bindings(struct pipe_context *pipe,
                     {
      struct nv50_context *nv50 = nv50_context(pipe);
   struct pipe_resource **ptr;
   unsigned i;
            if (nv50->global_residents.size < (end * sizeof(struct pipe_resource *))) {
      const unsigned old_size = nv50->global_residents.size;
   if (util_dynarray_resize(&nv50->global_residents, struct pipe_resource *, end)) {
      memset((uint8_t *)nv50->global_residents.data + old_size, 0,
      } else {
      NOUVEAU_ERR("Could not resize global residents array\n");
                  if (resources) {
      ptr = util_dynarray_element(
         for (i = 0; i < nr; ++i) {
      pipe_resource_reference(&ptr[i], resources[i]);
         } else {
      ptr = util_dynarray_element(
         for (i = 0; i < nr; ++i)
                           }
      void
   nv50_init_state_functions(struct nv50_context *nv50)
   {
               pipe->create_blend_state = nv50_blend_state_create;
   pipe->bind_blend_state = nv50_blend_state_bind;
            pipe->create_rasterizer_state = nv50_rasterizer_state_create;
   pipe->bind_rasterizer_state = nv50_rasterizer_state_bind;
            pipe->create_depth_stencil_alpha_state = nv50_zsa_state_create;
   pipe->bind_depth_stencil_alpha_state = nv50_zsa_state_bind;
            pipe->create_sampler_state = nv50_sampler_state_create;
   pipe->delete_sampler_state = nv50_sampler_state_delete;
            pipe->create_sampler_view = nv50_create_sampler_view;
   pipe->sampler_view_destroy = nv50_sampler_view_destroy;
            pipe->create_vs_state = nv50_vp_state_create;
   pipe->create_fs_state = nv50_fp_state_create;
   pipe->create_gs_state = nv50_gp_state_create;
   pipe->create_compute_state = nv50_cp_state_create;
   pipe->bind_vs_state = nv50_vp_state_bind;
   pipe->bind_fs_state = nv50_fp_state_bind;
   pipe->bind_gs_state = nv50_gp_state_bind;
   pipe->bind_compute_state = nv50_cp_state_bind;
   pipe->delete_vs_state = nv50_sp_state_delete;
   pipe->delete_fs_state = nv50_sp_state_delete;
   pipe->delete_gs_state = nv50_sp_state_delete;
                     pipe->set_blend_color = nv50_set_blend_color;
   pipe->set_stencil_ref = nv50_set_stencil_ref;
   pipe->set_clip_state = nv50_set_clip_state;
   pipe->set_sample_mask = nv50_set_sample_mask;
   pipe->set_min_samples = nv50_set_min_samples;
   pipe->set_constant_buffer = nv50_set_constant_buffer;
   pipe->set_framebuffer_state = nv50_set_framebuffer_state;
   pipe->set_polygon_stipple = nv50_set_polygon_stipple;
   pipe->set_scissor_states = nv50_set_scissor_states;
   pipe->set_viewport_states = nv50_set_viewport_states;
            pipe->create_vertex_elements_state = nv50_vertex_state_create;
   pipe->delete_vertex_elements_state = nv50_vertex_state_delete;
                     pipe->create_stream_output_target = nv50_so_target_create;
   pipe->stream_output_target_destroy = nv50_so_target_destroy;
            pipe->set_global_binding = nv50_set_global_bindings;
   pipe->set_compute_resources = nv50_set_compute_resources;
   pipe->set_shader_images = nv50_set_shader_images;
            nv50->sample_mask = ~0;
      }
