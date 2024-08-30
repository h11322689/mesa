   /**************************************************************************
   *
   * Copyright 2009 Younes Manton.
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
      #include <assert.h>
      #include "util/compiler.h"
   #include "pipe/p_context.h"
      #include "util/u_memory.h"
   #include "util/u_draw.h"
   #include "util/u_surface.h"
   #include "util/u_upload_mgr.h"
      #include "tgsi/tgsi_ureg.h"
      #include "vl_csc.h"
   #include "vl_types.h"
      #include "vl_compositor_gfx.h"
      enum VS_OUTPUT
   {
      VS_O_VPOS = 0,
   VS_O_COLOR = 0,
   VS_O_VTEX = 0,
   VS_O_VTOP,
      };
      void *
   create_vert_shader(struct vl_compositor *c)
   {
      struct ureg_program *shader;
   struct ureg_src vpos, vtex, color;
   struct ureg_dst tmp;
   struct ureg_dst o_vpos, o_vtex, o_color;
            shader = ureg_create(PIPE_SHADER_VERTEX);
   if (!shader)
            vpos = ureg_DECL_vs_input(shader, 0);
   vtex = ureg_DECL_vs_input(shader, 1);
   color = ureg_DECL_vs_input(shader, 2);
   tmp = ureg_DECL_temporary(shader);
   o_vpos = ureg_DECL_output(shader, TGSI_SEMANTIC_POSITION, VS_O_VPOS);
   o_color = ureg_DECL_output(shader, TGSI_SEMANTIC_COLOR, VS_O_COLOR);
   o_vtex = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_VTEX);
   o_vtop = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_VTOP);
            /*
   * o_vpos = vpos
   * o_vtex = vtex
   * o_color = color
   */
   ureg_MOV(shader, o_vpos, vpos);
   ureg_MOV(shader, o_vtex, vtex);
            /*
   * tmp.x = vtex.w / 2
   * tmp.y = vtex.w / 4
   *
   * o_vtop.x = vtex.x
   * o_vtop.y = vtex.y * tmp.x + 0.25f
   * o_vtop.z = vtex.y * tmp.y + 0.25f
   * o_vtop.w = 1 / tmp.x
   *
   * o_vbottom.x = vtex.x
   * o_vbottom.y = vtex.y * tmp.x - 0.25f
   * o_vbottom.z = vtex.y * tmp.y - 0.25f
   * o_vbottom.w = 1 / tmp.y
   */
   ureg_MUL(shader, ureg_writemask(tmp, TGSI_WRITEMASK_X),
         ureg_MUL(shader, ureg_writemask(tmp, TGSI_WRITEMASK_Y),
            ureg_MOV(shader, ureg_writemask(o_vtop, TGSI_WRITEMASK_X), vtex);
   ureg_MAD(shader, ureg_writemask(o_vtop, TGSI_WRITEMASK_Y), ureg_scalar(vtex, TGSI_SWIZZLE_Y),
         ureg_MAD(shader, ureg_writemask(o_vtop, TGSI_WRITEMASK_Z), ureg_scalar(vtex, TGSI_SWIZZLE_Y),
         ureg_RCP(shader, ureg_writemask(o_vtop, TGSI_WRITEMASK_W),
            ureg_MOV(shader, ureg_writemask(o_vbottom, TGSI_WRITEMASK_X), vtex);
   ureg_MAD(shader, ureg_writemask(o_vbottom, TGSI_WRITEMASK_Y), ureg_scalar(vtex, TGSI_SWIZZLE_Y),
         ureg_MAD(shader, ureg_writemask(o_vbottom, TGSI_WRITEMASK_Z), ureg_scalar(vtex, TGSI_SWIZZLE_Y),
         ureg_RCP(shader, ureg_writemask(o_vbottom, TGSI_WRITEMASK_W),
                        }
      static void
   create_frag_shader_weave(struct ureg_program *shader, struct ureg_dst fragment)
   {
      struct ureg_src i_tc[2];
   struct ureg_src sampler[3];
   struct ureg_dst t_tc[2];
   struct ureg_dst t_texel[2];
            i_tc[0] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_VTOP, TGSI_INTERPOLATE_LINEAR);
            for (i = 0; i < 3; ++i) {
      sampler[i] = ureg_DECL_sampler(shader, i);
   ureg_DECL_sampler_view(shader, i, TGSI_TEXTURE_2D_ARRAY,
                                 for (i = 0; i < 2; ++i) {
      t_tc[i] = ureg_DECL_temporary(shader);
               /* calculate the texture offsets
   * t_tc.x = i_tc.x
   * t_tc.y = (round(i_tc.y - 0.5) + 0.5) / height * 2
   */
   for (i = 0; i < 2; ++i) {
      ureg_MOV(shader, ureg_writemask(t_tc[i], TGSI_WRITEMASK_X), i_tc[i]);
   ureg_ADD(shader, ureg_writemask(t_tc[i], TGSI_WRITEMASK_YZ),
         ureg_ROUND(shader, ureg_writemask(t_tc[i], TGSI_WRITEMASK_YZ), ureg_src(t_tc[i]));
   ureg_MOV(shader, ureg_writemask(t_tc[i], TGSI_WRITEMASK_W),
         ureg_ADD(shader, ureg_writemask(t_tc[i], TGSI_WRITEMASK_YZ),
         ureg_MUL(shader, ureg_writemask(t_tc[i], TGSI_WRITEMASK_Y),
         ureg_MUL(shader, ureg_writemask(t_tc[i], TGSI_WRITEMASK_Z),
               /* fetch the texels
   * texel[0..1].x = tex(t_tc[0..1][0])
   * texel[0..1].y = tex(t_tc[0..1][1])
   * texel[0..1].z = tex(t_tc[0..1][2])
   */
   for (i = 0; i < 2; ++i)
      for (j = 0; j < 3; ++j) {
                     ureg_TEX(shader, ureg_writemask(t_texel[i], TGSI_WRITEMASK_X << j),
            /* calculate linear interpolation factor
   * factor = |round(i_tc.y) - i_tc.y| * 2
   */
   ureg_ROUND(shader, ureg_writemask(t_tc[0], TGSI_WRITEMASK_YZ), i_tc[0]);
   ureg_ADD(shader, ureg_writemask(t_tc[0], TGSI_WRITEMASK_YZ),
         ureg_MUL(shader, ureg_writemask(t_tc[0], TGSI_WRITEMASK_YZ),
         ureg_LRP(shader, fragment, ureg_swizzle(ureg_src(t_tc[0]),
                  for (i = 0; i < 2; ++i) {
      ureg_release_temporary(shader, t_texel[i]);
         }
      static void
   create_frag_shader_csc(struct ureg_program *shader, struct ureg_dst texel,
         {
      struct ureg_src csc[3];
   struct ureg_src lumakey;
   struct ureg_dst temp[2];
            for (i = 0; i < 3; ++i)
                     for (i = 0; i < 2; ++i)
            ureg_MOV(shader, ureg_writemask(texel, TGSI_WRITEMASK_W),
            for (i = 0; i < 3; ++i)
      ureg_DP4(shader, ureg_writemask(fragment, TGSI_WRITEMASK_X << i), csc[i],
         ureg_MOV(shader, ureg_writemask(temp[0], TGSI_WRITEMASK_W),
         ureg_SLE(shader, ureg_writemask(temp[1], TGSI_WRITEMASK_W),
         ureg_SGT(shader, ureg_writemask(temp[0], TGSI_WRITEMASK_W),
         ureg_MAX(shader, ureg_writemask(fragment, TGSI_WRITEMASK_W),
            for (i = 0; i < 2; ++i)
      }
      static void
   create_frag_shader_yuv(struct ureg_program *shader, struct ureg_dst texel)
   {
      struct ureg_src tc;
   struct ureg_src sampler[3];
            tc = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_VTEX, TGSI_INTERPOLATE_LINEAR);
   for (i = 0; i < 3; ++i) {
      sampler[i] = ureg_DECL_sampler(shader, i);
   ureg_DECL_sampler_view(shader, i, TGSI_TEXTURE_2D_ARRAY,
                                 /*
   * texel.xyz = tex(tc, sampler[i])
   */
   for (i = 0; i < 3; ++i)
      }
      void *
   create_frag_shader_video_buffer(struct vl_compositor *c)
   {
      struct ureg_program *shader;
   struct ureg_dst texel;
            shader = ureg_create(PIPE_SHADER_FRAGMENT);
   if (!shader)
            texel = ureg_DECL_temporary(shader);
            create_frag_shader_yuv(shader, texel);
            ureg_release_temporary(shader, texel);
               }
      void *
   create_frag_shader_weave_rgb(struct vl_compositor *c)
   {
      struct ureg_program *shader;
            shader = ureg_create(PIPE_SHADER_FRAGMENT);
   if (!shader)
            texel = ureg_DECL_temporary(shader);
            create_frag_shader_weave(shader, texel);
                                 }
      void *
   create_frag_shader_deint_yuv(struct vl_compositor *c, bool y, bool w)
   {
      struct ureg_program *shader;
            shader = ureg_create(PIPE_SHADER_FRAGMENT);
   if (!shader)
            texel = ureg_DECL_temporary(shader);
            if (w)
         else
            if (y)
         else
      ureg_MOV(shader, ureg_writemask(fragment, TGSI_WRITEMASK_XY),
                                    }
      void *
   create_frag_shader_palette(struct vl_compositor *c, bool include_cc)
   {
      struct ureg_program *shader;
   struct ureg_src csc[3];
   struct ureg_src tc;
   struct ureg_src sampler;
   struct ureg_src palette;
   struct ureg_dst texel;
   struct ureg_dst fragment;
            shader = ureg_create(PIPE_SHADER_FRAGMENT);
   if (!shader)
            for (i = 0; include_cc && i < 3; ++i)
            tc = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_VTEX, TGSI_INTERPOLATE_LINEAR);
   sampler = ureg_DECL_sampler(shader, 0);
   ureg_DECL_sampler_view(shader, 0, TGSI_TEXTURE_2D,
                           palette = ureg_DECL_sampler(shader, 1);
   ureg_DECL_sampler_view(shader, 1, TGSI_TEXTURE_1D,
                              texel = ureg_DECL_temporary(shader);
            /*
   * texel = tex(tc, sampler)
   * fragment.xyz = tex(texel, palette) * csc
   * fragment.a = texel.a
   */
   ureg_TEX(shader, texel, TGSI_TEXTURE_2D, tc, sampler);
            if (include_cc) {
      ureg_TEX(shader, texel, TGSI_TEXTURE_1D, ureg_src(texel), palette);
   for (i = 0; i < 3; ++i)
      } else {
      ureg_TEX(shader, ureg_writemask(fragment, TGSI_WRITEMASK_XYZ),
               ureg_release_temporary(shader, texel);
               }
      void *
   create_frag_shader_rgba(struct vl_compositor *c)
   {
      struct ureg_program *shader;
   struct ureg_src tc, color, sampler;
            shader = ureg_create(PIPE_SHADER_FRAGMENT);
   if (!shader)
            tc = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_VTEX, TGSI_INTERPOLATE_LINEAR);
   color = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_COLOR, VS_O_COLOR, TGSI_INTERPOLATE_LINEAR);
   sampler = ureg_DECL_sampler(shader, 0);
   ureg_DECL_sampler_view(shader, 0, TGSI_TEXTURE_2D,
                           texel = ureg_DECL_temporary(shader);
            /*
   * fragment = tex(tc, sampler)
   */
   ureg_TEX(shader, texel, TGSI_TEXTURE_2D, tc, sampler);
   ureg_MUL(shader, fragment, ureg_src(texel), color);
               }
      void *
   create_frag_shader_rgb_yuv(struct vl_compositor *c, bool y)
   {
      struct ureg_program *shader;
   struct ureg_src tc, sampler;
            struct ureg_src csc[3];
            shader = ureg_create(PIPE_SHADER_FRAGMENT);
   if (!shader)
            for (i = 0; i < 3; ++i)
            sampler = ureg_DECL_sampler(shader, 0);
   tc = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_VTEX, TGSI_INTERPOLATE_LINEAR);
   texel = ureg_DECL_temporary(shader);
                     if (y) {
         } else {
      for (i = 0; i < 2; ++i)
               ureg_release_temporary(shader, texel);
               }
      static void
   gen_rect_verts(struct vertex2f *vb, struct vl_compositor_layer *layer)
   {
                        switch (layer->rotate) {
   default:
   case VL_COMPOSITOR_ROTATE_0:
      tl = layer->dst.tl;
   tr.x = layer->dst.br.x;
   tr.y = layer->dst.tl.y;
   br = layer->dst.br;
   bl.x = layer->dst.tl.x;
   bl.y = layer->dst.br.y;
      case VL_COMPOSITOR_ROTATE_90:
      tl.x = layer->dst.br.x;
   tl.y = layer->dst.tl.y;
   tr = layer->dst.br;
   br.x = layer->dst.tl.x;
   br.y = layer->dst.br.y;
   bl = layer->dst.tl;
      case VL_COMPOSITOR_ROTATE_180:
      tl = layer->dst.br;
   tr.x = layer->dst.tl.x;
   tr.y = layer->dst.br.y;
   br = layer->dst.tl;
   bl.x = layer->dst.br.x;
   bl.y = layer->dst.tl.y;
      case VL_COMPOSITOR_ROTATE_270:
      tl.x = layer->dst.tl.x;
   tl.y = layer->dst.br.y;
   tr = layer->dst.tl;
   br.x = layer->dst.br.x;
   br.y = layer->dst.tl.y;
   bl = layer->dst.br;
               vb[ 0].x = tl.x;
   vb[ 0].y = tl.y;
   vb[ 1].x = layer->src.tl.x;
   vb[ 1].y = layer->src.tl.y;
   vb[ 2] = layer->zw;
   vb[ 3].x = layer->colors[0].x;
   vb[ 3].y = layer->colors[0].y;
   vb[ 4].x = layer->colors[0].z;
            vb[ 5].x = tr.x;
   vb[ 5].y = tr.y;
   vb[ 6].x = layer->src.br.x;
   vb[ 6].y = layer->src.tl.y;
   vb[ 7] = layer->zw;
   vb[ 8].x = layer->colors[1].x;
   vb[ 8].y = layer->colors[1].y;
   vb[ 9].x = layer->colors[1].z;
            vb[10].x = br.x;
   vb[10].y = br.y;
   vb[11].x = layer->src.br.x;
   vb[11].y = layer->src.br.y;
   vb[12] = layer->zw;
   vb[13].x = layer->colors[2].x;
   vb[13].y = layer->colors[2].y;
   vb[14].x = layer->colors[2].z;
            vb[15].x = bl.x;
   vb[15].y = bl.y;
   vb[16].x = layer->src.tl.x;
   vb[16].y = layer->src.br.y;
   vb[17] = layer->zw;
   vb[18].x = layer->colors[3].x;
   vb[18].y = layer->colors[3].y;
   vb[19].x = layer->colors[3].z;
      }
      static inline struct u_rect
   calc_drawn_area(struct vl_compositor_state *s, struct vl_compositor_layer *layer)
   {
      struct vertex2f tl, br;
                     // rotate
   switch (layer->rotate) {
   default:
   case VL_COMPOSITOR_ROTATE_0:
      tl = layer->dst.tl;
   br = layer->dst.br;
      case VL_COMPOSITOR_ROTATE_90:
      tl.x = layer->dst.br.x;
   tl.y = layer->dst.tl.y;
   br.x = layer->dst.tl.x;
   br.y = layer->dst.br.y;
      case VL_COMPOSITOR_ROTATE_180:
      tl = layer->dst.br;
   br = layer->dst.tl;
      case VL_COMPOSITOR_ROTATE_270:
      tl.x = layer->dst.tl.x;
   tl.y = layer->dst.br.y;
   br.x = layer->dst.br.x;
   br.y = layer->dst.tl.y;
               // scale
   result.x0 = tl.x * layer->viewport.scale[0] + layer->viewport.translate[0];
   result.y0 = tl.y * layer->viewport.scale[1] + layer->viewport.translate[1];
   result.x1 = br.x * layer->viewport.scale[0] + layer->viewport.translate[0];
            // and clip
   result.x0 = MAX2(result.x0, s->scissor.minx);
   result.y0 = MAX2(result.y0, s->scissor.miny);
   result.x1 = MIN2(result.x1, s->scissor.maxx);
   result.y1 = MIN2(result.y1, s->scissor.maxy);
      }
      static void
   gen_vertex_data(struct vl_compositor *c, struct vl_compositor_state *s, struct u_rect *dirty)
   {
      struct vertex2f *vb;
                     /* Allocate new memory for vertices. */
   u_upload_alloc(c->pipe->stream_uploader, 0,
                  VL_COMPOSITOR_VB_STRIDE * VL_COMPOSITOR_MAX_LAYERS * 4, /* size */
         for (i = 0; i < VL_COMPOSITOR_MAX_LAYERS; i++) {
      if (s->used_layers & (1 << i)) {
      struct vl_compositor_layer *layer = &s->layers[i];
                  if (!layer->viewport_valid) {
      layer->viewport.scale[0] = c->fb_state.width;
   layer->viewport.scale[1] = c->fb_state.height;
   layer->viewport.translate[0] = 0;
               if (dirty && layer->clearing) {
      struct u_rect drawn = calc_drawn_area(s, layer);
   if (
   dirty->x0 >= drawn.x0 &&
   dirty->y0 >= drawn.y0 &&
                     // We clear the dirty area anyway, no need for clear_render_target
   dirty->x0 = dirty->y0 = VL_COMPOSITOR_MAX_DIRTY;
                           }
      static void
   set_csc_matrix(struct vl_compositor_state *s)
   {
               float *ptr = pipe_buffer_map(s->pipe, s->shader_params,
                  if (!ptr)
                     ptr += sizeof(vl_csc_matrix) / sizeof(float);
   *ptr++ = s->luma_min;
               }
      static void
   draw_layers(struct vl_compositor *c, struct vl_compositor_state *s, struct u_rect *dirty)
   {
                        for (i = 0, vb_index = 0; i < VL_COMPOSITOR_MAX_LAYERS; ++i) {
      if (s->used_layers & (1 << i)) {
      struct vl_compositor_layer *layer = &s->layers[i];
   struct pipe_sampler_view **samplers = &layer->sampler_views[0];
                  c->pipe->bind_blend_state(c->pipe, blend);
   c->pipe->set_viewport_states(c->pipe, 0, 1, &layer->viewport);
   c->pipe->bind_fs_state(c->pipe, layer->fs);
   c->pipe->bind_sampler_states(c->pipe, PIPE_SHADER_FRAGMENT, 0,
                                       if (dirty) {
      // Remember the currently drawn area as dirty for the next draw command
   struct u_rect drawn = calc_drawn_area(s, layer);
   dirty->x0 = MIN2(drawn.x0, dirty->x0);
   dirty->y0 = MIN2(drawn.y0, dirty->y0);
   dirty->x1 = MAX2(drawn.x1, dirty->x1);
               }
      void
   vl_compositor_gfx_render(struct vl_compositor_state *s,
                           {
      assert(c);
            c->fb_state.width = dst_surface->width;
   c->fb_state.height = dst_surface->height;
            if (!s->scissor_valid) {
      s->scissor.minx = 0;
   s->scissor.miny = 0;
   s->scissor.maxx = dst_surface->width;
      }
            gen_vertex_data(c, s, dirty_area);
            if (clear_dirty && dirty_area &&
               c->pipe->clear_render_target(c->pipe, dst_surface, &s->clear_color,
         dirty_area->x0 = dirty_area->y0 = VL_COMPOSITOR_MAX_DIRTY;
               c->pipe->set_framebuffer_state(c->pipe, &c->fb_state);
   c->pipe->bind_vs_state(c->pipe, c->vs);
   c->pipe->set_vertex_buffers(c->pipe, 1, 0, false, &c->vertex_buf);
   c->pipe->bind_vertex_elements_state(c->pipe, c->vertex_elems_state);
   pipe_set_constant_buffer(c->pipe, PIPE_SHADER_FRAGMENT, 0, s->shader_params);
               }
