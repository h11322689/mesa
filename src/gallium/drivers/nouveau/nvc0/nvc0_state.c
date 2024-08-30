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
      #include "tgsi/tgsi_parse.h"
   #include "compiler/nir/nir.h"
   #include "compiler/nir/nir_serialize.h"
   #include "nir/tgsi_to_nir.h"
      #include "nvc0/nvc0_stateobj.h"
   #include "nvc0/nvc0_context.h"
   #include "nvc0/nvc0_query_hw.h"
      #include "nvc0/nvc0_3d.xml.h"
      #include "nouveau_gldefs.h"
      static inline uint32_t
   nvc0_colormask(unsigned mask)
   {
               if (mask & PIPE_MASK_R)
         if (mask & PIPE_MASK_G)
         if (mask & PIPE_MASK_B)
         if (mask & PIPE_MASK_A)
               }
      #define NVC0_BLEND_FACTOR_CASE(a, b) \
            static inline uint32_t
   nvc0_blend_fac(unsigned factor)
   {
      switch (factor) {
   NVC0_BLEND_FACTOR_CASE(ONE, ONE);
   NVC0_BLEND_FACTOR_CASE(SRC_COLOR, SRC_COLOR);
   NVC0_BLEND_FACTOR_CASE(SRC_ALPHA, SRC_ALPHA);
   NVC0_BLEND_FACTOR_CASE(DST_ALPHA, DST_ALPHA);
   NVC0_BLEND_FACTOR_CASE(DST_COLOR, DST_COLOR);
   NVC0_BLEND_FACTOR_CASE(SRC_ALPHA_SATURATE, SRC_ALPHA_SATURATE);
   NVC0_BLEND_FACTOR_CASE(CONST_COLOR, CONSTANT_COLOR);
   NVC0_BLEND_FACTOR_CASE(CONST_ALPHA, CONSTANT_ALPHA);
   NVC0_BLEND_FACTOR_CASE(SRC1_COLOR, SRC1_COLOR);
   NVC0_BLEND_FACTOR_CASE(SRC1_ALPHA, SRC1_ALPHA);
   NVC0_BLEND_FACTOR_CASE(ZERO, ZERO);
   NVC0_BLEND_FACTOR_CASE(INV_SRC_COLOR, ONE_MINUS_SRC_COLOR);
   NVC0_BLEND_FACTOR_CASE(INV_SRC_ALPHA, ONE_MINUS_SRC_ALPHA);
   NVC0_BLEND_FACTOR_CASE(INV_DST_ALPHA, ONE_MINUS_DST_ALPHA);
   NVC0_BLEND_FACTOR_CASE(INV_DST_COLOR, ONE_MINUS_DST_COLOR);
   NVC0_BLEND_FACTOR_CASE(INV_CONST_COLOR, ONE_MINUS_CONSTANT_COLOR);
   NVC0_BLEND_FACTOR_CASE(INV_CONST_ALPHA, ONE_MINUS_CONSTANT_ALPHA);
   NVC0_BLEND_FACTOR_CASE(INV_SRC1_COLOR, ONE_MINUS_SRC1_COLOR);
   NVC0_BLEND_FACTOR_CASE(INV_SRC1_ALPHA, ONE_MINUS_SRC1_ALPHA);
   default:
            }
      static void *
   nvc0_blend_state_create(struct pipe_context *pipe,
         {
      struct nvc0_blend_stateobj *so = CALLOC_STRUCT(nvc0_blend_stateobj);
   int i;
   int r; /* reference */
   uint32_t ms;
   uint8_t blend_en = 0;
   bool indep_masks = false;
                     /* check which states actually have differing values */
   if (cso->independent_blend_enable) {
      for (r = 0; r < 8 && !cso->rt[r].blend_enable; ++r);
   blend_en |= 1 << r;
   for (i = r + 1; i < 8; ++i) {
      if (!cso->rt[i].blend_enable)
         blend_en |= 1 << i;
   if (cso->rt[i].rgb_func != cso->rt[r].rgb_func ||
      cso->rt[i].rgb_src_factor != cso->rt[r].rgb_src_factor ||
   cso->rt[i].rgb_dst_factor != cso->rt[r].rgb_dst_factor ||
   cso->rt[i].alpha_func != cso->rt[r].alpha_func ||
   cso->rt[i].alpha_src_factor != cso->rt[r].alpha_src_factor ||
   cso->rt[i].alpha_dst_factor != cso->rt[r].alpha_dst_factor) {
   indep_funcs = true;
         }
   for (; i < 8; ++i)
            for (i = 1; i < 8; ++i) {
      if (cso->rt[i].colormask != cso->rt[0].colormask) {
      indep_masks = true;
            } else {
      r = 0;
   if (cso->rt[0].blend_enable)
               if (cso->logicop_enable) {
      SB_BEGIN_3D(so, LOGIC_OP_ENABLE, 2);
   SB_DATA    (so, 1);
               } else {
               SB_IMMED_3D(so, BLEND_INDEPENDENT, indep_funcs);
   SB_IMMED_3D(so, MACRO_BLEND_ENABLES, blend_en);
   if (indep_funcs) {
      for (i = 0; i < 8; ++i) {
      if (cso->rt[i].blend_enable) {
      SB_BEGIN_3D(so, IBLEND_EQUATION_RGB(i), 6);
   SB_DATA    (so, nvgl_blend_eqn(cso->rt[i].rgb_func));
   SB_DATA    (so, nvc0_blend_fac(cso->rt[i].rgb_src_factor));
   SB_DATA    (so, nvc0_blend_fac(cso->rt[i].rgb_dst_factor));
   SB_DATA    (so, nvgl_blend_eqn(cso->rt[i].alpha_func));
   SB_DATA    (so, nvc0_blend_fac(cso->rt[i].alpha_src_factor));
            } else
   if (blend_en) {
      SB_BEGIN_3D(so, BLEND_EQUATION_RGB, 5);
   SB_DATA    (so, nvgl_blend_eqn(cso->rt[r].rgb_func));
   SB_DATA    (so, nvc0_blend_fac(cso->rt[r].rgb_src_factor));
   SB_DATA    (so, nvc0_blend_fac(cso->rt[r].rgb_dst_factor));
   SB_DATA    (so, nvgl_blend_eqn(cso->rt[r].alpha_func));
   SB_DATA    (so, nvc0_blend_fac(cso->rt[r].alpha_src_factor));
   SB_BEGIN_3D(so, BLEND_FUNC_DST_ALPHA, 1);
               SB_IMMED_3D(so, COLOR_MASK_COMMON, !indep_masks);
   if (indep_masks) {
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
   nvc0_blend_state_bind(struct pipe_context *pipe, void *hwcso)
   {
               nvc0->blend = hwcso;
      }
      static void
   nvc0_blend_state_delete(struct pipe_context *pipe, void *hwcso)
   {
         }
      /* NOTE: ignoring line_last_pixel */
   static void *
   nvc0_rasterizer_state_create(struct pipe_context *pipe,
         {
      struct nvc0_rasterizer_stateobj *so;
   uint16_t class_3d = nouveau_screen(pipe->screen)->class_3d;
            so = CALLOC_STRUCT(nvc0_rasterizer_stateobj);
   if (!so)
                  /* Scissor enables are handled in scissor state, we will not want to
   * always emit 16 commands, one for each scissor rectangle, here.
            SB_IMMED_3D(so, PROVOKING_VERTEX_LAST, !cso->flatshade_first);
            SB_IMMED_3D(so, VERT_COLOR_CLAMP_EN, cso->clamp_vertex_color);
   SB_BEGIN_3D(so, FRAG_COLOR_CLAMP_EN, 1);
                     SB_IMMED_3D(so, LINE_SMOOTH_ENABLE, cso->line_smooth);
   if (cso->line_smooth || cso->multisample)
         else
                  SB_IMMED_3D(so, LINE_STIPPLE_ENABLE, cso->line_stipple_enable);
   if (cso->line_stipple_enable) {
      SB_BEGIN_3D(so, LINE_STIPPLE_PATTERN, 1);
   SB_DATA    (so, (cso->line_stipple_pattern << 8) |
                  SB_IMMED_3D(so, VP_POINT_SIZE, cso->point_size_per_vertex);
   if (!cso->point_size_per_vertex) {
      SB_BEGIN_3D(so, POINT_SIZE, 1);
               reg = (cso->sprite_coord_mode == PIPE_SPRITE_COORD_UPPER_LEFT) ?
      NVC0_3D_POINT_COORD_REPLACE_COORD_ORIGIN_UPPER_LEFT :
         SB_BEGIN_3D(so, POINT_COORD_REPLACE, 1);
   SB_DATA    (so, ((cso->sprite_coord_enable & 0xff) << 3) | reg);
   SB_IMMED_3D(so, POINT_SPRITE_ENABLE, cso->point_quad_rasterization);
            if (class_3d >= GM200_3D_CLASS) {
      SB_IMMED_3D(so, FILL_RECTANGLE,
                     SB_BEGIN_3D(so, MACRO_POLYGON_MODE_FRONT, 1);
   SB_DATA    (so, nvgl_polygon_mode(cso->fill_front));
   SB_BEGIN_3D(so, MACRO_POLYGON_MODE_BACK, 1);
   SB_DATA    (so, nvgl_polygon_mode(cso->fill_back));
            SB_BEGIN_3D(so, CULL_FACE_ENABLE, 3);
   SB_DATA    (so, cso->cull_face != PIPE_FACE_NONE);
   SB_DATA    (so, cso->front_ccw ? NVC0_3D_FRONT_FACE_CCW :
         switch (cso->cull_face) {
   case PIPE_FACE_FRONT_AND_BACK:
      SB_DATA(so, NVC0_3D_CULL_FACE_FRONT_AND_BACK);
      case PIPE_FACE_FRONT:
      SB_DATA(so, NVC0_3D_CULL_FACE_FRONT);
      case PIPE_FACE_BACK:
   default:
      SB_DATA(so, NVC0_3D_CULL_FACE_BACK);
               SB_IMMED_3D(so, POLYGON_STIPPLE_ENABLE, cso->poly_stipple_enable);
   SB_BEGIN_3D(so, POLYGON_OFFSET_POINT_ENABLE, 3);
   SB_DATA    (so, cso->offset_point);
   SB_DATA    (so, cso->offset_line);
            if (cso->offset_point || cso->offset_line || cso->offset_tri) {
      SB_BEGIN_3D(so, POLYGON_OFFSET_FACTOR, 1);
   SB_DATA    (so, fui(cso->offset_scale));
   if (!cso->offset_units_unscaled) {
      SB_BEGIN_3D(so, POLYGON_OFFSET_UNITS, 1);
      }
   SB_BEGIN_3D(so, POLYGON_OFFSET_CLAMP, 1);
               if (cso->depth_clip_near)
         else
      reg =
      NVC0_3D_VIEW_VOLUME_CLIP_CTRL_UNK1_UNK1 |
   NVC0_3D_VIEW_VOLUME_CLIP_CTRL_DEPTH_CLAMP_NEAR |
            SB_BEGIN_3D(so, VIEW_VOLUME_CLIP_CTRL, 1);
                              if (class_3d >= GM200_3D_CLASS) {
      if (cso->conservative_raster_mode != PIPE_CONSERVATIVE_RASTER_OFF) {
         bool post_snap = cso->conservative_raster_mode ==
         uint32_t state = cso->subpixel_precision_x;
   state |= cso->subpixel_precision_y << 4;
   state |= (uint32_t)(cso->conservative_raster_dilate * 4) << 8;
   state |= (post_snap || class_3d < GP100_3D_CLASS) ? 1 << 10 : 0;
   } else {
                     assert(so->size <= ARRAY_SIZE(so->state));
      }
      static void
   nvc0_rasterizer_state_bind(struct pipe_context *pipe, void *hwcso)
   {
               nvc0->rast = hwcso;
      }
      static void
   nvc0_rasterizer_state_delete(struct pipe_context *pipe, void *hwcso)
   {
         }
      static void *
   nvc0_zsa_state_create(struct pipe_context *pipe,
         {
                        SB_IMMED_3D(so, DEPTH_TEST_ENABLE, cso->depth_enabled);
   if (cso->depth_enabled) {
      SB_IMMED_3D(so, DEPTH_WRITE_ENABLE, cso->depth_writemask);
   SB_BEGIN_3D(so, DEPTH_TEST_FUNC, 1);
               SB_IMMED_3D(so, DEPTH_BOUNDS_EN, cso->depth_bounds_test);
   if (cso->depth_bounds_test) {
      SB_BEGIN_3D(so, DEPTH_BOUNDS(0), 2);
   SB_DATA    (so, fui(cso->depth_bounds_min));
               if (cso->stencil[0].enabled) {
      SB_BEGIN_3D(so, STENCIL_ENABLE, 5);
   SB_DATA    (so, 1);
   SB_DATA    (so, nvgl_stencil_op(cso->stencil[0].fail_op));
   SB_DATA    (so, nvgl_stencil_op(cso->stencil[0].zfail_op));
   SB_DATA    (so, nvgl_stencil_op(cso->stencil[0].zpass_op));
   SB_DATA    (so, nvgl_comparison_op(cso->stencil[0].func));
   SB_BEGIN_3D(so, STENCIL_FRONT_FUNC_MASK, 2);
   SB_DATA    (so, cso->stencil[0].valuemask);
      } else {
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
      } else
   if (cso->stencil[0].enabled) {
                  SB_IMMED_3D(so, ALPHA_TEST_ENABLE, cso->alpha_enabled);
   if (cso->alpha_enabled) {
      SB_BEGIN_3D(so, ALPHA_TEST_REF, 2);
   SB_DATA    (so, fui(cso->alpha_ref_value));
               assert(so->size <= ARRAY_SIZE(so->state));
      }
      static void
   nvc0_zsa_state_bind(struct pipe_context *pipe, void *hwcso)
   {
               nvc0->zsa = hwcso;
      }
      static void
   nvc0_zsa_state_delete(struct pipe_context *pipe, void *hwcso)
   {
         }
      /* ====================== SAMPLERS AND TEXTURES ================================
   */
      #define NV50_TSC_WRAP_CASE(n) \
            static void
   nvc0_sampler_state_delete(struct pipe_context *pipe, void *hwcso)
   {
               for (s = 0; s < 6; ++s)
      for (i = 0; i < nvc0_context(pipe)->num_samplers[s]; ++i)
                           }
      static inline void
   nvc0_stage_sampler_states_bind(struct nvc0_context *nvc0,
               {
      unsigned highest_found = 0;
            for (i = 0; i < nr; ++i) {
      struct nv50_tsc_entry *hwcso = hwcsos ? nv50_tsc_entry(hwcsos[i]) : NULL;
            if (hwcso)
            if (hwcso == old)
                  nvc0->samplers[s][i] = hwcso;
   if (old)
      }
   if (nr >= nvc0->num_samplers[s])
      }
      static void
   nvc0_bind_sampler_states(struct pipe_context *pipe,
               {
               assert(start == 0);
            if (s == 5)
         else
      }
         /* NOTE: only called when not referenced anywhere, won't be bound */
   static void
   nvc0_sampler_view_destroy(struct pipe_context *pipe,
         {
                           }
      static inline void
   nvc0_stage_set_sampler_views(struct nvc0_context *nvc0, int s,
               {
               for (i = 0; i < nr; ++i) {
      struct pipe_sampler_view *view = views ? views[i] : NULL;
            if (view == nvc0->textures[s][i]) {
      if (take_ownership)
            }
            if (view && view->texture) {
      struct pipe_resource *res = view->texture;
   if (res->target == PIPE_BUFFER &&
      (res->flags & PIPE_RESOURCE_FLAG_MAP_COHERENT))
      else
      } else {
                  if (old) {
      if (s == 5)
         else
                     if (take_ownership) {
      pipe_sampler_view_reference(&nvc0->textures[s][i], NULL);
      } else {
                     for (i = nr; i < nvc0->num_textures[s]; ++i) {
      struct nv50_tic_entry *old = nv50_tic_entry(nvc0->textures[s][i]);
   if (old) {
      if (s == 5)
         else
         nvc0_screen_tic_unlock(nvc0->screen, old);
                     }
      static void
   nvc0_set_sampler_views(struct pipe_context *pipe, enum pipe_shader_type shader,
                           {
               assert(start == 0);
            if (s == 5)
         else
      }
      /* ============================= SHADERS =======================================
   */
      static void *
   nvc0_sp_state_create(struct pipe_context *pipe,
         {
               prog = CALLOC_STRUCT(nvc0_program);
   if (!prog)
                     switch(cso->type) {
   case PIPE_SHADER_IR_TGSI:
      prog->nir = tgsi_to_nir(cso->tokens, pipe->screen, false);
      case PIPE_SHADER_IR_NIR:
      prog->nir = cso->ir.nir;
      default:
      assert(!"unsupported IR!");
   free(prog);
               if (cso->stream_output.num_outputs)
            prog->translated = nvc0_program_translate(
      prog, nvc0_context(pipe)->screen->base.device->chipset,
   nvc0_context(pipe)->screen->base.disk_shader_cache,
            }
      static void
   nvc0_sp_state_delete(struct pipe_context *pipe, void *hwcso)
   {
      struct nvc0_context *nvc0 = nvc0_context(pipe);
            simple_mtx_lock(&nvc0->screen->state_lock);
   nvc0_program_destroy(nvc0_context(pipe), prog);
            ralloc_free(prog->nir);
      }
      static void *
   nvc0_vp_state_create(struct pipe_context *pipe,
         {
         }
      static void
   nvc0_vp_state_bind(struct pipe_context *pipe, void *hwcso)
   {
               nvc0->vertprog = hwcso;
      }
      static void *
   nvc0_fp_state_create(struct pipe_context *pipe,
         {
         }
      static void
   nvc0_fp_state_bind(struct pipe_context *pipe, void *hwcso)
   {
               nvc0->fragprog = hwcso;
      }
      static void *
   nvc0_gp_state_create(struct pipe_context *pipe,
         {
         }
      static void
   nvc0_gp_state_bind(struct pipe_context *pipe, void *hwcso)
   {
               nvc0->gmtyprog = hwcso;
      }
      static void *
   nvc0_tcp_state_create(struct pipe_context *pipe,
         {
         }
      static void
   nvc0_tcp_state_bind(struct pipe_context *pipe, void *hwcso)
   {
               nvc0->tctlprog = hwcso;
      }
      static void *
   nvc0_tep_state_create(struct pipe_context *pipe,
         {
         }
      static void
   nvc0_tep_state_bind(struct pipe_context *pipe, void *hwcso)
   {
               nvc0->tevlprog = hwcso;
      }
      static void *
   nvc0_cp_state_create(struct pipe_context *pipe,
         {
               prog = CALLOC_STRUCT(nvc0_program);
   if (!prog)
                  prog->cp.smem_size = cso->static_shared_mem;
            switch(cso->ir_type) {
   case PIPE_SHADER_IR_TGSI: {
      const struct tgsi_token *tokens = cso->prog;
   prog->nir = tgsi_to_nir(tokens, pipe->screen, false);
      }
   case PIPE_SHADER_IR_NIR:
      prog->nir = (nir_shader *)cso->prog;
      case PIPE_SHADER_IR_NIR_SERIALIZED: {
      struct blob_reader reader;
            blob_reader_init(&reader, hdr->blob, hdr->num_bytes);
   prog->nir = nir_deserialize(NULL, pipe->screen->get_compiler_options(pipe->screen, PIPE_SHADER_IR_NIR, PIPE_SHADER_COMPUTE), &reader);
      }
   default:
      assert(!"unsupported IR!");
   free(prog);
               prog->translated = nvc0_program_translate(
      prog, nvc0_context(pipe)->screen->base.device->chipset,
   nvc0_context(pipe)->screen->base.disk_shader_cache,
            }
      static void
   nvc0_cp_state_bind(struct pipe_context *pipe, void *hwcso)
   {
               nvc0->compprog = hwcso;
      }
      static void
   nvc0_get_compute_state_info(struct pipe_context *pipe, void *hwcso,
         {
      struct nvc0_context *nvc0 = nvc0_context(pipe);
   struct nvc0_program *prog = (struct nvc0_program *)hwcso;
   uint16_t obj_class = nvc0->screen->compute->oclass;
   uint32_t chipset = nvc0->screen->base.device->chipset;
            // fermi and a handful of tegra devices have less gprs per SM
   if (obj_class < NVE4_COMPUTE_CLASS || chipset == 0xea || chipset == 0x12b || chipset == 0x13b)
         else
            // TODO: not 100% sure about 8 for volta, but earlier reverse engineering indicates it
   uint32_t gpr_alloc_size = obj_class >= GV100_COMPUTE_CLASS ? 8 : 4;
            info->max_threads = MIN2(ROUND_DOWN_TO(threads, 32), 1024);
   info->private_memory = prog->hdr[1] & 0xfffff0;
   info->preferred_simd_size = 32;
      }
      static void
   nvc0_set_constant_buffer(struct pipe_context *pipe,
                     {
      struct nvc0_context *nvc0 = nvc0_context(pipe);
   struct pipe_resource *res = cb ? cb->buffer : NULL;
   const unsigned s = nvc0_shader_stage(shader);
            if (unlikely(shader == PIPE_SHADER_COMPUTE)) {
      if (nvc0->constbuf[s][i].user)
         else
   if (nvc0->constbuf[s][i].u.buf)
               } else {
      if (nvc0->constbuf[s][i].user)
         else
   if (nvc0->constbuf[s][i].u.buf)
               }
            if (nvc0->constbuf[s][i].u.buf)
            if (take_ownership) {
      pipe_resource_reference(&nvc0->constbuf[s][i].u.buf, NULL);
      } else {
                  nvc0->constbuf[s][i].user = (cb && cb->user_buffer) ? true : false;
   if (nvc0->constbuf[s][i].user) {
      nvc0->constbuf[s][i].u.data = cb->user_buffer;
   nvc0->constbuf[s][i].size = MIN2(cb->buffer_size, 0x10000);
   nvc0->constbuf_valid[s] |= 1 << i;
      } else
   if (cb) {
      nvc0->constbuf[s][i].offset = cb->buffer_offset;
   nvc0->constbuf[s][i].size = MIN2(align(cb->buffer_size, 0x100), 0x10000);
   nvc0->constbuf_valid[s] |= 1 << i;
   if (res && res->flags & PIPE_RESOURCE_FLAG_MAP_COHERENT)
         else
      }
   else {
      nvc0->constbuf_valid[s] &= ~(1 << i);
         }
      /* =============================================================================
   */
      static void
   nvc0_set_blend_color(struct pipe_context *pipe,
         {
               nvc0->blend_colour = *bcol;
      }
      static void
   nvc0_set_stencil_ref(struct pipe_context *pipe,
         {
               nvc0->stencil_ref = sr;
      }
      static void
   nvc0_set_clip_state(struct pipe_context *pipe,
         {
                           }
      static void
   nvc0_set_sample_mask(struct pipe_context *pipe, unsigned sample_mask)
   {
               nvc0->sample_mask = sample_mask;
      }
      static void
   nvc0_set_min_samples(struct pipe_context *pipe, unsigned min_samples)
   {
               if (nvc0->min_samples != min_samples) {
      nvc0->min_samples = min_samples;
         }
      static void
   nvc0_set_framebuffer_state(struct pipe_context *pipe,
         {
                                 nvc0->dirty_3d |= NVC0_NEW_3D_FRAMEBUFFER | NVC0_NEW_3D_SAMPLE_LOCATIONS |
            }
      static void
   nvc0_set_sample_locations(struct pipe_context *pipe,
         {
               nvc0->sample_locations_enabled = size && locations;
   if (size > sizeof(nvc0->sample_locations))
                     }
      static void
   nvc0_set_polygon_stipple(struct pipe_context *pipe,
         {
               nvc0->stipple = *stipple;
      }
      static void
   nvc0_set_scissor_states(struct pipe_context *pipe,
                     {
      struct nvc0_context *nvc0 = nvc0_context(pipe);
            assert(start_slot + num_scissors <= NVC0_MAX_VIEWPORTS);
   for (i = 0; i < num_scissors; i++) {
      if (!memcmp(&nvc0->scissors[start_slot + i], &scissor[i], sizeof(*scissor)))
         nvc0->scissors[start_slot + i] = scissor[i];
   nvc0->scissors_dirty |= 1 << (start_slot + i);
         }
      static void
   nvc0_set_viewport_states(struct pipe_context *pipe,
                     {
      struct nvc0_context *nvc0 = nvc0_context(pipe);
            assert(start_slot + num_viewports <= NVC0_MAX_VIEWPORTS);
   for (i = 0; i < num_viewports; i++) {
      if (!memcmp(&nvc0->viewports[start_slot + i], &vpt[i], sizeof(*vpt)))
         nvc0->viewports[start_slot + i] = vpt[i];
   nvc0->viewports_dirty |= 1 << (start_slot + i);
            }
      static void
   nvc0_set_window_rectangles(struct pipe_context *pipe,
                     {
               nvc0->window_rect.inclusive = include;
   nvc0->window_rect.rects = MIN2(num_rectangles, NVC0_MAX_WINDOW_RECTANGLES);
   memcpy(nvc0->window_rect.rect, rectangles,
               }
      static void
   nvc0_set_tess_state(struct pipe_context *pipe,
               {
               memcpy(nvc0->default_tess_outer, default_tess_outer, 4 * sizeof(float));
   memcpy(nvc0->default_tess_inner, default_tess_inner, 2 * sizeof(float));
      }
      static void
   nvc0_set_patch_vertices(struct pipe_context *pipe, uint8_t patch_vertices)
   {
                  }
      static void
   nvc0_set_vertex_buffers(struct pipe_context *pipe,
                           {
      struct nvc0_context *nvc0 = nvc0_context(pipe);
            nouveau_bufctx_reset(nvc0->bufctx_3d, NVC0_BIND_3D_VTX);
            util_set_vertex_buffers_count(nvc0->vtxbuf, &nvc0->num_vtxbufs, vb,
                  unsigned clear_mask = ~u_bit_consecutive(count, unbind_num_trailing_slots);
   nvc0->vbo_user &= clear_mask;
   nvc0->constant_vbos &= clear_mask;
            if (!vb) {
      clear_mask = ~u_bit_consecutive(0, count);
   nvc0->vbo_user &= clear_mask;
   nvc0->constant_vbos &= clear_mask;
   nvc0->vtxbufs_coherent &= clear_mask;
               for (i = 0; i < count; ++i) {
               if (vb[i].is_user_buffer) {
      nvc0->vbo_user |= 1 << dst_index;
      } else {
               if (vb[i].buffer.resource &&
      vb[i].buffer.resource->flags & PIPE_RESOURCE_FLAG_MAP_COHERENT)
      else
            }
      static void
   nvc0_vertex_state_bind(struct pipe_context *pipe, void *hwcso)
   {
               nvc0->vertex = hwcso;
      }
      static struct pipe_stream_output_target *
   nvc0_so_target_create(struct pipe_context *pipe,
               {
      struct nv04_resource *buf = (struct nv04_resource *)res;
   struct nvc0_so_target *targ = MALLOC_STRUCT(nvc0_so_target);
   if (!targ)
            targ->pq = pipe->create_query(pipe, NVC0_HW_QUERY_TFB_BUFFER_OFFSET, 0);
   if (!targ->pq) {
      FREE(targ);
      }
            targ->pipe.buffer_size = size;
   targ->pipe.buffer_offset = offset;
   targ->pipe.context = pipe;
   targ->pipe.buffer = NULL;
   pipe_resource_reference(&targ->pipe.buffer, res);
            assert(buf->base.target == PIPE_BUFFER);
               }
      static void
   nvc0_so_target_save_offset(struct pipe_context *pipe,
               {
               if (*serialize) {
      *serialize = false;
   PUSH_SPACE(nvc0_context(pipe)->base.pushbuf, 1);
                        nvc0_query(targ->pq)->index = index;
      }
      static void
   nvc0_so_target_destroy(struct pipe_context *pipe,
         {
      struct nvc0_so_target *targ = nvc0_so_target(ptarg);
   pipe->destroy_query(pipe, targ->pq);
   pipe_resource_reference(&targ->pipe.buffer, NULL);
      }
      static void
   nvc0_set_transform_feedback_targets(struct pipe_context *pipe,
                     {
      struct nvc0_context *nvc0 = nvc0_context(pipe);
   unsigned i;
                     for (i = 0; i < num_targets; ++i) {
      const bool changed = nvc0->tfbbuf[i] != targets[i];
   const bool append = (offsets[i] == ((unsigned)-1));
   if (!changed && append)
                  if (nvc0->tfbbuf[i] && changed)
            if (targets[i] && !append)
               }
   for (; i < nvc0->num_tfbbufs; ++i) {
      if (nvc0->tfbbuf[i]) {
      nvc0->tfbbuf_dirty |= 1 << i;
   nvc0_so_target_save_offset(pipe, nvc0->tfbbuf[i], i, &serialize);
         }
            if (nvc0->tfbbuf_dirty) {
      nouveau_bufctx_reset(nvc0->bufctx_3d, NVC0_BIND_3D_TFB);
         }
      static void
   nvc0_bind_surfaces_range(struct nvc0_context *nvc0, const unsigned t,
               {
      const unsigned end = start + nr;
   const unsigned mask = ((1 << nr) - 1) << start;
            if (psurfaces) {
      for (i = start; i < end; ++i) {
      const unsigned p = i - start;
   if (psurfaces[p])
         else
               } else {
      for (i = start; i < end; ++i)
            }
            if (t == 0)
         else
      }
      static void
   nvc0_set_compute_resources(struct pipe_context *pipe,
               {
                  }
      static bool
   nvc0_bind_images_range(struct nvc0_context *nvc0, const unsigned s,
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
                                       if (nvc0->screen->base.class_3d >= GM107_3D_CLASS) {
      if (nvc0->images_tic[s][i]) {
      struct nv50_tic_entry *old =
                           nvc0->images_tic[s][i] =
      gm107_create_texture_view_from_image(&nvc0->base.pipe,
      }
   if (!mask)
      } else {
      mask = ((1 << nr) - 1) << start;
   if (!(nvc0->images_valid[s] & mask))
         for (i = start; i < end; ++i) {
      pipe_resource_reference(&nvc0->images[s][i].resource, NULL);
   if (nvc0->screen->base.class_3d >= GM107_3D_CLASS) {
      struct nv50_tic_entry *old = nv50_tic_entry(nvc0->images_tic[s][i]);
   if (old) {
      nvc0_screen_tic_unlock(nvc0->screen, old);
            }
      }
            if (s == 5)
         else
               }
      static void
   nvc0_set_shader_images(struct pipe_context *pipe,
                           {
               nvc0_bind_images_range(nvc0_context(pipe), s, start + nr,
            if (!nvc0_bind_images_range(nvc0_context(pipe), s, start, nr, images))
            if (s == 5)
         else
      }
      static bool
   nvc0_bind_buffers_range(struct nvc0_context *nvc0, const unsigned t,
               {
      const unsigned end = start + nr;
   unsigned mask = 0;
                     if (pbuffers) {
      for (i = start; i < end; ++i) {
      struct pipe_shader_buffer *buf = &nvc0->buffers[t][i];
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
   if (!(nvc0->buffers_valid[t] & mask))
         for (i = start; i < end; ++i)
            }
            if (t == 5)
         else
               }
      static void
   nvc0_set_shader_buffers(struct pipe_context *pipe,
                           {
      const unsigned s = nvc0_shader_stage(shader);
   if (!nvc0_bind_buffers_range(nvc0_context(pipe), s, start, nr, buffers))
            if (s == 5)
         else
      }
      static inline void
   nvc0_set_global_handle(uint32_t *phandle, struct pipe_resource *res)
   {
      struct nv04_resource *buf = nv04_resource(res);
   if (buf) {
      uint64_t address = buf->address + *phandle;
   /* even though it's a pointer to uint32_t that's fine */
      } else {
            }
      static void
   nvc0_set_global_bindings(struct pipe_context *pipe,
                     {
      struct nvc0_context *nvc0 = nvc0_context(pipe);
   struct pipe_resource **ptr;
   unsigned i;
            if (!nr)
            if (nvc0->global_residents.size < (end * sizeof(struct pipe_resource *))) {
      const unsigned old_size = nvc0->global_residents.size;
   if (util_dynarray_resize(&nvc0->global_residents, struct pipe_resource *, end)) {
      memset((uint8_t *)nvc0->global_residents.data + old_size, 0,
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
   nvc0_init_state_functions(struct nvc0_context *nvc0)
   {
               pipe->create_blend_state = nvc0_blend_state_create;
   pipe->bind_blend_state = nvc0_blend_state_bind;
            pipe->create_rasterizer_state = nvc0_rasterizer_state_create;
   pipe->bind_rasterizer_state = nvc0_rasterizer_state_bind;
            pipe->create_depth_stencil_alpha_state = nvc0_zsa_state_create;
   pipe->bind_depth_stencil_alpha_state = nvc0_zsa_state_bind;
            pipe->create_sampler_state = nv50_sampler_state_create;
   pipe->delete_sampler_state = nvc0_sampler_state_delete;
            pipe->create_sampler_view = nvc0_create_sampler_view;
   pipe->sampler_view_destroy = nvc0_sampler_view_destroy;
            pipe->create_vs_state = nvc0_vp_state_create;
   pipe->create_fs_state = nvc0_fp_state_create;
   pipe->create_gs_state = nvc0_gp_state_create;
   pipe->create_tcs_state = nvc0_tcp_state_create;
   pipe->create_tes_state = nvc0_tep_state_create;
   pipe->bind_vs_state = nvc0_vp_state_bind;
   pipe->bind_fs_state = nvc0_fp_state_bind;
   pipe->bind_gs_state = nvc0_gp_state_bind;
   pipe->bind_tcs_state = nvc0_tcp_state_bind;
   pipe->bind_tes_state = nvc0_tep_state_bind;
   pipe->delete_vs_state = nvc0_sp_state_delete;
   pipe->delete_fs_state = nvc0_sp_state_delete;
   pipe->delete_gs_state = nvc0_sp_state_delete;
   pipe->delete_tcs_state = nvc0_sp_state_delete;
            pipe->create_compute_state = nvc0_cp_state_create;
   pipe->bind_compute_state = nvc0_cp_state_bind;
   pipe->get_compute_state_info = nvc0_get_compute_state_info;
            pipe->set_blend_color = nvc0_set_blend_color;
   pipe->set_stencil_ref = nvc0_set_stencil_ref;
   pipe->set_clip_state = nvc0_set_clip_state;
   pipe->set_sample_mask = nvc0_set_sample_mask;
   pipe->set_min_samples = nvc0_set_min_samples;
   pipe->set_constant_buffer = nvc0_set_constant_buffer;
   pipe->set_framebuffer_state = nvc0_set_framebuffer_state;
   pipe->set_sample_locations = nvc0_set_sample_locations;
   pipe->set_polygon_stipple = nvc0_set_polygon_stipple;
   pipe->set_scissor_states = nvc0_set_scissor_states;
   pipe->set_viewport_states = nvc0_set_viewport_states;
   pipe->set_window_rectangles = nvc0_set_window_rectangles;
   pipe->set_tess_state = nvc0_set_tess_state;
            pipe->create_vertex_elements_state = nvc0_vertex_state_create;
   pipe->delete_vertex_elements_state = nvc0_vertex_state_delete;
                     pipe->create_stream_output_target = nvc0_so_target_create;
   pipe->stream_output_target_destroy = nvc0_so_target_destroy;
            pipe->set_global_binding = nvc0_set_global_bindings;
   pipe->set_compute_resources = nvc0_set_compute_resources;
   pipe->set_shader_images = nvc0_set_shader_images;
            nvc0->sample_mask = ~0;
   nvc0->min_samples = 1;
   nvc0->default_tess_outer[0] =
   nvc0->default_tess_outer[1] =
   nvc0->default_tess_outer[2] =
   nvc0->default_tess_outer[3] = 1.0;
   nvc0->default_tess_inner[0] =
      }
