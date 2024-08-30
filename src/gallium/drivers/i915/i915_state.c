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
      /* Authors:  Keith Whitwell <keithw@vmware.com>
   */
      #include "compiler/nir/nir_builder.h"
   #include "draw/draw_context.h"
   #include "nir/nir_to_tgsi.h"
   #include "tgsi/tgsi_parse.h"
   #include "util/u_helpers.h"
   #include "util/u_inlines.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/u_transfer.h"
   #include "nir.h"
      #include "i915_context.h"
   #include "i915_fpc.h"
   #include "i915_reg.h"
   #include "i915_resource.h"
   #include "i915_state.h"
   #include "i915_state_inlines.h"
      /* The i915 (and related graphics cores) do not support GL_CLAMP.  The
   * Intel drivers for "other operating systems" implement GL_CLAMP as
   * GL_CLAMP_TO_EDGE, so the same is done here.
   */
   static unsigned
   translate_wrap_mode(unsigned wrap)
   {
      switch (wrap) {
   case PIPE_TEX_WRAP_REPEAT:
         case PIPE_TEX_WRAP_CLAMP:
         case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
         case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
         case PIPE_TEX_WRAP_MIRROR_REPEAT:
         default:
            }
      static unsigned
   translate_img_filter(unsigned filter)
   {
      switch (filter) {
   case PIPE_TEX_FILTER_NEAREST:
         case PIPE_TEX_FILTER_LINEAR:
         default:
      assert(0);
         }
      static unsigned
   translate_mip_filter(unsigned filter)
   {
      switch (filter) {
   case PIPE_TEX_MIPFILTER_NONE:
         case PIPE_TEX_MIPFILTER_NEAREST:
         case PIPE_TEX_MIPFILTER_LINEAR:
         default:
      assert(0);
         }
      static uint32_t
   i915_remap_lis6_blend_dst_alpha(uint32_t lis6, uint32_t normal, uint32_t inv)
   {
      uint32_t src = (lis6 >> S6_CBUF_SRC_BLEND_FACT_SHIFT) & BLENDFACT_MASK;
   lis6 &= ~SRC_BLND_FACT(BLENDFACT_MASK);
   if (src == BLENDFACT_DST_ALPHA)
         else if (src == BLENDFACT_INV_DST_ALPHA)
                  uint32_t dst = (lis6 >> S6_CBUF_DST_BLEND_FACT_SHIFT) & BLENDFACT_MASK;
   lis6 &= ~DST_BLND_FACT(BLENDFACT_MASK);
   if (dst == BLENDFACT_DST_ALPHA)
         else if (dst == BLENDFACT_INV_DST_ALPHA)
                     }
      static uint32_t
   i915_remap_iab_blend_dst_alpha(uint32_t iab, uint32_t normal, uint32_t inv)
   {
      uint32_t src = (iab >> IAB_SRC_FACTOR_SHIFT) & BLENDFACT_MASK;
   iab &= ~SRC_BLND_FACT(BLENDFACT_MASK);
   if (src == BLENDFACT_DST_ALPHA)
         else if (src == BLENDFACT_INV_DST_ALPHA)
                  uint32_t dst = (iab >> IAB_DST_FACTOR_SHIFT) & BLENDFACT_MASK;
   iab &= ~DST_BLND_FACT(BLENDFACT_MASK);
   if (dst == BLENDFACT_DST_ALPHA)
         else if (dst == BLENDFACT_INV_DST_ALPHA)
                     }
      /* None of this state is actually used for anything yet.
   */
   static void *
   i915_create_blend_state(struct pipe_context *pipe,
         {
               {
      unsigned eqRGB = blend->rt[0].rgb_func;
   unsigned srcRGB = blend->rt[0].rgb_src_factor;
            unsigned eqA = blend->rt[0].alpha_func;
   unsigned srcA = blend->rt[0].alpha_src_factor;
            /* Special handling for MIN/MAX filter modes handled at
   * frontend level.
                        cso_data->iab = (_3DSTATE_INDEPENDENT_ALPHA_BLEND_CMD |
                  IAB_MODIFY_ENABLE | IAB_ENABLE | IAB_MODIFY_FUNC |
   IAB_MODIFY_SRC_FACTOR | IAB_MODIFY_DST_FACTOR |
   } else {
      cso_data->iab =
                  cso_data->modes4 |=
      (_3DSTATE_MODES_4_CMD | ENABLE_LOGIC_OP_FUNC |
         if (blend->logicop_enable)
            if (blend->dither)
            /* We potentially do some fixup at emission for non-BGRA targets */
   if ((blend->rt[0].colormask & PIPE_MASK_R) == 0)
            if ((blend->rt[0].colormask & PIPE_MASK_G) == 0)
            if ((blend->rt[0].colormask & PIPE_MASK_B) == 0)
            if ((blend->rt[0].colormask & PIPE_MASK_A) == 0)
            if (blend->rt[0].blend_enable) {
      unsigned funcRGB = blend->rt[0].rgb_func;
   unsigned srcRGB = blend->rt[0].rgb_src_factor;
            cso_data->LIS6 |=
      (S6_CBUF_BLEND_ENABLE |
   SRC_BLND_FACT(i915_translate_blend_factor(srcRGB)) |
   DST_BLND_FACT(i915_translate_blend_factor(dstRGB)) |
            cso_data->LIS6_alpha_in_g = i915_remap_lis6_blend_dst_alpha(
         cso_data->LIS6_alpha_is_x = i915_remap_lis6_blend_dst_alpha(
            cso_data->iab_alpha_in_g = i915_remap_iab_blend_dst_alpha(
         cso_data->iab_alpha_is_x = i915_remap_iab_blend_dst_alpha(
               }
      static void
   i915_bind_blend_state(struct pipe_context *pipe, void *blend)
   {
               if (i915->blend == blend)
                        }
      static void
   i915_delete_blend_state(struct pipe_context *pipe, void *blend)
   {
         }
      static void
   i915_set_blend_color(struct pipe_context *pipe,
         {
               if (!blend_color)
                        }
      static void
   i915_set_stencil_ref(struct pipe_context *pipe,
         {
                           }
      static void *
   i915_create_sampler_state(struct pipe_context *pipe,
         {
      struct i915_sampler_state *cso = CALLOC_STRUCT(i915_sampler_state);
   const unsigned ws = sampler->wrap_s;
   const unsigned wt = sampler->wrap_t;
   const unsigned wr = sampler->wrap_r;
   unsigned minFilt, magFilt;
                     mipFilt = translate_mip_filter(sampler->min_mip_filter);
   minFilt = translate_img_filter(sampler->min_img_filter);
            if (sampler->max_anisotropy > 1)
            if (sampler->max_anisotropy > 2) {
                  {
      int b = (int)(sampler->lod_bias * 16.0);
   b = CLAMP(b, -256, 255);
               /* Shadow:
   */
   if (sampler->compare_mode == PIPE_TEX_COMPARE_R_TO_TEXTURE) {
      cso->state[0] |= (SS2_SHADOW_ENABLE | i915_translate_shadow_compare_func(
            minFilt = FILTER_4X4_FLAT;
               cso->state[0] |=
      ((minFilt << SS2_MIN_FILTER_SHIFT) | (mipFilt << SS2_MIP_FILTER_SHIFT) |
         cso->state[1] |= ((translate_wrap_mode(ws) << SS3_TCX_ADDR_MODE_SHIFT) |
                  if (!sampler->unnormalized_coords)
            {
      int minlod = (int)(16.0 * sampler->min_lod);
   int maxlod = (int)(16.0 * sampler->max_lod);
   minlod = CLAMP(minlod, 0, 16 * 11);
            if (minlod > maxlod)
            cso->minlod = minlod;
               {
      uint8_t r = float_to_ubyte(sampler->border_color.f[0]);
   uint8_t g = float_to_ubyte(sampler->border_color.f[1]);
   uint8_t b = float_to_ubyte(sampler->border_color.f[2]);
   uint8_t a = float_to_ubyte(sampler->border_color.f[3]);
      }
      }
      static void
   i915_bind_sampler_states(struct pipe_context *pipe,
               {
      if (shader != PIPE_SHADER_FRAGMENT) {
      assert(num == 0);
               struct i915_context *i915 = i915_context(pipe);
            /* Check for no-op */
   if (num == i915->num_samplers &&
      !memcmp(i915->fragment_sampler + start, samplers, num * sizeof(void *)))
         for (i = 0; i < num; ++i)
            /* find highest non-null samplers[] entry */
   {
      unsigned j = MAX2(i915->num_samplers, start + num);
   while (j > 0 && i915->fragment_sampler[j - 1] == NULL)
                        }
      static void
   i915_delete_sampler_state(struct pipe_context *pipe, void *sampler)
   {
         }
      /** XXX move someday?  Or consolidate all these simple state setters
   * into one file.
   */
      static uint32_t
   i915_get_modes4_stencil(const struct pipe_stencil_state *stencil)
   {
      int testmask = stencil->valuemask & 0xff;
            return (_3DSTATE_MODES_4_CMD | ENABLE_STENCIL_TEST_MASK |
            }
      static uint32_t
   i915_get_lis5_stencil(const struct pipe_stencil_state *stencil)
   {
      int test = i915_translate_compare_func(stencil->func);
   int fop = i915_translate_stencil_op(stencil->fail_op);
   int dfop = i915_translate_stencil_op(stencil->zfail_op);
            return (S5_STENCIL_TEST_ENABLE | S5_STENCIL_WRITE_ENABLE |
         (test << S5_STENCIL_TEST_FUNC_SHIFT) |
   (fop << S5_STENCIL_FAIL_SHIFT) |
      }
      static uint32_t
   i915_get_bfo(const struct pipe_stencil_state *stencil)
   {
      int test = i915_translate_compare_func(stencil->func);
   int fop = i915_translate_stencil_op(stencil->fail_op);
   int dfop = i915_translate_stencil_op(stencil->zfail_op);
            return (_3DSTATE_BACKFACE_STENCIL_OPS | BFO_ENABLE_STENCIL_FUNCS |
         BFO_ENABLE_STENCIL_TWO_SIDE | BFO_ENABLE_STENCIL_REF |
   BFO_STENCIL_TWO_SIDE | (test << BFO_STENCIL_TEST_SHIFT) |
   (fop << BFO_STENCIL_FAIL_SHIFT) |
      }
      static uint32_t
   i915_get_bfm(const struct pipe_stencil_state *stencil)
   {
      return (_3DSTATE_BACKFACE_STENCIL_MASKS | BFM_ENABLE_STENCIL_TEST_MASK |
         BFM_ENABLE_STENCIL_WRITE_MASK |
      }
      static void *
   i915_create_depth_stencil_state(
      struct pipe_context *pipe,
      {
      struct i915_depth_stencil_state *cso =
            cso->stencil_modes4_cw = i915_get_modes4_stencil(&depth_stencil->stencil[0]);
   cso->stencil_modes4_ccw =
            if (depth_stencil->stencil[0].enabled) {
                  if (depth_stencil->stencil[1].enabled) {
      cso->bfo_cw[0] = i915_get_bfo(&depth_stencil->stencil[1]);
            /* Precompute the backface stencil settings if front winding order is
   * reversed -- HW doesn't have a bit to flip it for us.
   */
   cso->stencil_LIS5_ccw = i915_get_lis5_stencil(&depth_stencil->stencil[1]);
   cso->bfo_ccw[0] = i915_get_bfo(&depth_stencil->stencil[0]);
      } else {
      /* This actually disables two-side stencil: The bit set is a
   * modify-enable bit to indicate we are changing the two-side
   * setting.  Then there is a symbolic zero to show that we are
   * setting the flag to zero/off.
   */
   cso->bfo_cw[0] = cso->bfo_ccw[0] =
                              if (depth_stencil->depth_enabled) {
               cso->depth_LIS6 |=
            if (depth_stencil->depth_writemask)
               if (depth_stencil->alpha_enabled) {
      int test = i915_translate_compare_func(depth_stencil->alpha_func);
            cso->depth_LIS6 |=
      (S6_ALPHA_TEST_ENABLE | (test << S6_ALPHA_TEST_FUNC_SHIFT) |
               }
      static void
   i915_bind_depth_stencil_state(struct pipe_context *pipe, void *depth_stencil)
   {
               if (i915->depth_stencil == depth_stencil)
                        }
      static void
   i915_delete_depth_stencil_state(struct pipe_context *pipe, void *depth_stencil)
   {
         }
      static void
   i915_set_scissor_states(struct pipe_context *pipe, unsigned start_slot,
               {
               memcpy(&i915->scissor, scissor, sizeof(*scissor));
      }
      static void
   i915_set_polygon_stipple(struct pipe_context *pipe,
         {
   }
      static const struct nir_to_tgsi_options ntt_options = {
         };
      static void *
   i915_create_fs_state(struct pipe_context *pipe,
         {
      struct i915_context *i915 = i915_context(pipe);
   struct i915_fragment_shader *ifs = CALLOC_STRUCT(i915_fragment_shader);
   if (!ifs)
                     if (templ->type == PIPE_SHADER_IR_NIR) {
      nir_shader *s = templ->ir.nir;
               } else {
      assert(templ->type == PIPE_SHADER_IR_TGSI);
   /* we need to keep a local copy of the tokens */
   ifs->state.tokens = tgsi_dup_tokens(templ->tokens);
                                 /* The shader's compiled to i915 instructions here */
               }
      static void
   i915_bind_fs_state(struct pipe_context *pipe, void *shader)
   {
               if (i915->fs == shader)
                     draw_bind_fragment_shader(i915->draw,
            /* Tell draw if we need to do point sprites so we can get PNTC. */
   if (i915->fs)
               }
      static void
   i915_delete_fs_state(struct pipe_context *pipe, void *shader)
   {
               ralloc_free(ifs->error);
   FREE(ifs->program);
   ifs->program = NULL;
   FREE((struct tgsi_token *)ifs->state.tokens);
                        }
      /* Does a test compile at link time to see if we'll be able to run this shader
   * at runtime.  Return a string to the GLSL compiler for anything we should
   * report as link failure.
   */
   char *
   i915_test_fragment_shader_compile(struct pipe_screen *screen, nir_shader *s)
   {
      struct i915_fragment_shader *ifs = CALLOC_STRUCT(i915_fragment_shader);
   if (!ifs)
            /* NTT takes ownership of the shader, give it a clone. */
            ifs->internal = s->info.internal;
   ifs->state.tokens = nir_to_tgsi_options(s, screen, &ntt_options);
                              char *msg = NULL;
   if (ifs->error)
                        }
      static void *
   i915_create_vs_state(struct pipe_context *pipe,
         {
               struct pipe_shader_state from_nir = {PIPE_SHADER_IR_TGSI};
   if (templ->type == PIPE_SHADER_IR_NIR) {
                        /* The gallivm draw path doesn't support non-native-integers NIR shaders,
   * st/mesa does native-integers for the screen as a whole rather than
   * per-stage, and i915 FS can't do native integers.  So, convert to TGSI,
   * where the draw path *does* support non-native-integers.
   */
   from_nir.tokens = nir_to_tgsi(s, pipe->screen);
                  }
      static void
   i915_bind_vs_state(struct pipe_context *pipe, void *shader)
   {
               if (i915->vs == shader)
                     /* just pass-through to draw module */
               }
      static void
   i915_delete_vs_state(struct pipe_context *pipe, void *shader)
   {
               /* just pass-through to draw module */
      }
      static void
   i915_set_constant_buffer(struct pipe_context *pipe,
                     {
      struct i915_context *i915 = i915_context(pipe);
   struct pipe_resource *buf = cb ? cb->buffer : NULL;
   unsigned new_num = 0;
            /* XXX don't support geom shaders now */
   if (shader == PIPE_SHADER_GEOMETRY)
            if (cb && cb->user_buffer) {
      buf = i915_user_buffer_create(pipe->screen, (void *)cb->user_buffer,
               /* if we have a new buffer compare it with the old one */
   if (buf) {
      struct i915_buffer *ibuf = i915_buffer(buf);
   struct pipe_resource *old_buf = i915->constants[shader];
   struct i915_buffer *old = old_buf ? i915_buffer(old_buf) : NULL;
                     if (old_num == new_num) {
         #if 0
            /* XXX no point in running this code since st/mesa only uses user buffers */
   /* Can't compare the buffer data since they are userbuffers */
      #else
         #endif
            } else {
                  if (take_ownership) {
      pipe_resource_reference(&i915->constants[shader], NULL);
      } else {
         }
            if (diff)
      i915->dirty |= shader == PIPE_SHADER_VERTEX ? I915_NEW_VS_CONSTANTS
         if (cb && cb->user_buffer) {
            }
      static void
   i915_set_sampler_views(struct pipe_context *pipe, enum pipe_shader_type shader,
                     {
      if (shader != PIPE_SHADER_FRAGMENT) {
      /* No support for VS samplers, because it would mean accessing the
   * write-combined maps of the textures, which is very slow.  VS samplers
   * are not a required feature of GL2.1 or GLES2.
   */
   assert(num == 0);
      }
   struct i915_context *i915 = i915_context(pipe);
                     /* Check for no-op */
   if (views && num == i915->num_fragment_sampler_views &&
      !memcmp(i915->fragment_sampler_views, views,
         if (take_ownership) {
      for (unsigned i = 0; i < num; i++) {
      struct pipe_sampler_view *view = views[i];
         }
               for (i = 0; i < num; i++) {
      if (take_ownership) {
      pipe_sampler_view_reference(&i915->fragment_sampler_views[i], NULL);
      } else {
      pipe_sampler_view_reference(&i915->fragment_sampler_views[i],
                  for (i = num; i < i915->num_fragment_sampler_views; i++)
                        }
      struct pipe_sampler_view *
   i915_create_sampler_view_custom(struct pipe_context *pipe,
                     {
               if (view) {
      *view = *templ;
   view->reference.count = 1;
   view->texture = NULL;
   pipe_resource_reference(&view->texture, texture);
                  }
      static struct pipe_sampler_view *
   i915_create_sampler_view(struct pipe_context *pipe,
               {
               if (view) {
      *view = *templ;
   view->reference.count = 1;
   view->texture = NULL;
   pipe_resource_reference(&view->texture, texture);
                  }
      static void
   i915_sampler_view_destroy(struct pipe_context *pipe,
         {
      pipe_resource_reference(&view->texture, NULL);
      }
      static void
   i915_set_framebuffer_state(struct pipe_context *pipe,
         {
               util_copy_framebuffer_state(&i915->framebuffer, fb);
   if (fb->nr_cbufs) {
      struct i915_surface *surf = i915_surface(i915->framebuffer.cbufs[0]);
   if (i915->current.fixup_swizzle != surf->oc_swizzle) {
      i915->current.fixup_swizzle = surf->oc_swizzle;
   memcpy(i915->current.color_swizzle, surf->color_swizzle,
               }
   if (fb->zsbuf)
               }
      static void
   i915_set_clip_state(struct pipe_context *pipe,
         {
                                    }
      /* Called when gallium frontends notice changes to the viewport
   * matrix:
   */
   static void
   i915_set_viewport_states(struct pipe_context *pipe, unsigned start_slot,
               {
                        /* pass the viewport info to the draw module */
   draw_set_viewport_states(i915->draw, start_slot, num_viewports,
               }
      static void *
   i915_create_rasterizer_state(struct pipe_context *pipe,
         {
               cso->templ = *rasterizer;
   cso->light_twoside = rasterizer->light_twoside;
   cso->ds[0].u = _3DSTATE_DEPTH_OFFSET_SCALE;
   cso->ds[1].f = rasterizer->offset_scale;
   if (rasterizer->poly_stipple_enable) {
                  if (rasterizer->scissor)
         else
            switch (rasterizer->cull_face) {
   case PIPE_FACE_NONE:
      cso->LIS4 |= S4_CULLMODE_NONE;
      case PIPE_FACE_FRONT:
      if (rasterizer->front_ccw)
         else
            case PIPE_FACE_BACK:
      if (rasterizer->front_ccw)
         else
            case PIPE_FACE_FRONT_AND_BACK:
      cso->LIS4 |= S4_CULLMODE_BOTH;
               {
                        if (rasterizer->line_smooth)
               {
                           if (rasterizer->flatshade) {
      cso->LIS4 |=
               if (!rasterizer->flatshade_first)
                        }
      static void
   i915_bind_rasterizer_state(struct pipe_context *pipe, void *raster)
   {
               if (i915->rasterizer == raster)
                     /* pass-through to draw module */
   draw_set_rasterizer_state(
      i915->draw, (i915->rasterizer ? &(i915->rasterizer->templ) : NULL),
            }
      static void
   i915_delete_rasterizer_state(struct pipe_context *pipe, void *raster)
   {
         }
      static void
   i915_set_vertex_buffers(struct pipe_context *pipe, unsigned count,
               {
      struct i915_context *i915 = i915_context(pipe);
            util_set_vertex_buffers_count(i915->vertex_buffers, &i915->nr_vertex_buffers,
                  /* pass-through to draw module */
      }
      static void *
   i915_create_vertex_elements_state(struct pipe_context *pipe, unsigned count,
         {
      struct i915_velems_state *velems;
   assert(count <= PIPE_MAX_ATTRIBS);
   velems =
         if (velems) {
      velems->count = count;
      }
      }
      static void
   i915_bind_vertex_elements_state(struct pipe_context *pipe, void *velems)
   {
      struct i915_context *i915 = i915_context(pipe);
            if (i915->velems == velems)
                     /* pass-through to draw module */
   if (i915_velems) {
      draw_set_vertex_elements(i915->draw, i915_velems->count,
         }
      static void
   i915_delete_vertex_elements_state(struct pipe_context *pipe, void *velems)
   {
         }
      static void
   i915_set_sample_mask(struct pipe_context *pipe, unsigned sample_mask)
   {
   }
      void
   i915_init_state_functions(struct i915_context *i915)
   {
      i915->base.create_blend_state = i915_create_blend_state;
   i915->base.bind_blend_state = i915_bind_blend_state;
            i915->base.create_sampler_state = i915_create_sampler_state;
   i915->base.bind_sampler_states = i915_bind_sampler_states;
            i915->base.create_depth_stencil_alpha_state =
         i915->base.bind_depth_stencil_alpha_state = i915_bind_depth_stencil_state;
   i915->base.delete_depth_stencil_alpha_state =
            i915->base.create_rasterizer_state = i915_create_rasterizer_state;
   i915->base.bind_rasterizer_state = i915_bind_rasterizer_state;
   i915->base.delete_rasterizer_state = i915_delete_rasterizer_state;
   i915->base.create_fs_state = i915_create_fs_state;
   i915->base.bind_fs_state = i915_bind_fs_state;
   i915->base.delete_fs_state = i915_delete_fs_state;
   i915->base.create_vs_state = i915_create_vs_state;
   i915->base.bind_vs_state = i915_bind_vs_state;
   i915->base.delete_vs_state = i915_delete_vs_state;
   i915->base.create_vertex_elements_state = i915_create_vertex_elements_state;
   i915->base.bind_vertex_elements_state = i915_bind_vertex_elements_state;
            i915->base.set_blend_color = i915_set_blend_color;
   i915->base.set_stencil_ref = i915_set_stencil_ref;
   i915->base.set_clip_state = i915_set_clip_state;
   i915->base.set_sample_mask = i915_set_sample_mask;
   i915->base.set_constant_buffer = i915_set_constant_buffer;
            i915->base.set_polygon_stipple = i915_set_polygon_stipple;
   i915->base.set_scissor_states = i915_set_scissor_states;
   i915->base.set_sampler_views = i915_set_sampler_views;
   i915->base.create_sampler_view = i915_create_sampler_view;
   i915->base.sampler_view_destroy = i915_sampler_view_destroy;
   i915->base.set_viewport_states = i915_set_viewport_states;
      }
