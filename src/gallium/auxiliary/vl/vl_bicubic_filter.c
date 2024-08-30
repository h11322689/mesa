   /**************************************************************************
   *
   * Copyright 2016 Nayan Deshmukh.
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
      #include <stdio.h>
      #include "pipe/p_context.h"
      #include "tgsi/tgsi_ureg.h"
      #include "util/u_draw.h"
   #include "util/u_memory.h"
   #include "util/u_math.h"
   #include "util/u_rect.h"
   #include "util/u_upload_mgr.h"
      #include "vl_types.h"
   #include "vl_vertex_buffers.h"
   #include "vl_bicubic_filter.h"
      enum VS_OUTPUT
   {
      VS_O_VPOS = 0,
      };
      static void *
   create_vert_shader(struct vl_bicubic_filter *filter)
   {
      struct ureg_program *shader;
   struct ureg_src i_vpos;
            shader = ureg_create(PIPE_SHADER_VERTEX);
   if (!shader)
            i_vpos = ureg_DECL_vs_input(shader, 0);
   o_vpos = ureg_DECL_output(shader, TGSI_SEMANTIC_POSITION, VS_O_VPOS);
            ureg_MOV(shader, o_vpos, i_vpos);
                        }
      static void
   create_frag_shader_cubic_interpolater(struct ureg_program *shader, struct ureg_src tex_a,
                     {
      struct ureg_dst temp[11];
   struct ureg_dst t_2;
            for(i = 0; i < 11; ++i)
                  /*
   * |temp[0]|   |  0  2  0  0 |  |tex_a|
   * |temp[1]| = | -1  0  1  0 |* |tex_b|
   * |temp[2]|   |  2 -5  4 -1 |  |tex_c|
   * |temp[3]|   | -1  3 -3  1 |  |tex_d|
   */
            ureg_MUL(shader, temp[1], tex_a, ureg_imm1f(shader, -1.0f));
   ureg_MAD(shader, temp[1], tex_c, ureg_imm1f(shader, 1.0f),
            ureg_MUL(shader, temp[2], tex_a, ureg_imm1f(shader, 2.0f));
   ureg_MAD(shader, temp[2], tex_b, ureg_imm1f(shader, -5.0f),
         ureg_MAD(shader, temp[2], tex_c, ureg_imm1f(shader, 4.0f),
         ureg_MAD(shader, temp[2], tex_d, ureg_imm1f(shader, -1.0f),
            ureg_MUL(shader, temp[3], tex_a, ureg_imm1f(shader, -1.0f));
   ureg_MAD(shader, temp[3], tex_b, ureg_imm1f(shader, 3.0f),
         ureg_MAD(shader, temp[3], tex_c, ureg_imm1f(shader, -3.0f),
         ureg_MAD(shader, temp[3], tex_d, ureg_imm1f(shader, 1.0f),
            /*
   * t_2 = t*t
   * o_fragment = 0.5*|1  t  t^2  t^3|*|temp[0]|
   *                                   |temp[1]|
   *                                   |temp[2]|
   *                                   |temp[3]|
            ureg_MUL(shader, t_2, t, t);
            ureg_MUL(shader, temp[4], ureg_src(temp[4]),
         ureg_MUL(shader, temp[5], ureg_src(t_2),
         ureg_MUL(shader, temp[6], t,
         ureg_MUL(shader, temp[7], ureg_imm1f(shader, 1.0f),
         ureg_ADD(shader, temp[8], ureg_src(temp[4]),
         ureg_ADD(shader, temp[9], ureg_src(temp[6]),
            ureg_ADD(shader, temp[10], ureg_src(temp[8]),
         ureg_MUL(shader, o_fragment, ureg_src(temp[10]),
               for(i = 0; i < 11; ++i)
            }
      static void *
   create_frag_shader(struct vl_bicubic_filter *filter, unsigned video_width,
         {
      struct pipe_screen *screen = filter->pipe->screen;
   struct ureg_program *shader;
   struct ureg_src i_vtex, vtex;
   struct ureg_src sampler;
   struct ureg_src half_pixel;
   struct ureg_dst t_array[23];
   struct ureg_dst o_fragment;
   struct ureg_dst t;
            if (screen->get_shader_param(
                           shader = ureg_create(PIPE_SHADER_FRAGMENT);
   if (!shader) {
                  i_vtex = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_VTEX, TGSI_INTERPOLATE_LINEAR);
   sampler = ureg_DECL_sampler(shader, 0);
   ureg_DECL_sampler_view(shader, 0, TGSI_TEXTURE_2D,
                              for (i = 0; i < 23; ++i)
                  half_pixel = ureg_DECL_constant(shader, 0);
            /*
   * temp = (i_vtex - (0.5/dst_size)) * i_size)
   * t = frac(temp)
   * vtex = floor(i_vtex)/i_size
   */
   ureg_ADD(shader, ureg_writemask(t_array[21], TGSI_WRITEMASK_XY),
         ureg_MUL(shader, ureg_writemask(t_array[22], TGSI_WRITEMASK_XY),
         ureg_FRC(shader, ureg_writemask(t, TGSI_WRITEMASK_XY),
            ureg_FLR(shader, ureg_writemask(t_array[22], TGSI_WRITEMASK_XY),
            ureg_MAD(shader, ureg_writemask(t_array[22], TGSI_WRITEMASK_XY),
            ureg_src(t_array[22]),
         /*
   * t_array[0..*] = vtex + offset[0..*]
   * t_array[0..*] = tex(t_array[0..*], sampler)
   * t_array[16+i] = cubic_interpolate(t_array[4*i..4*i+3], t_x)
   * o_fragment = cubic_interpolate(t_array[16..19], t_y)
   */
   vtex = ureg_src(t_array[22]);
   for (i = 0; i < 16; ++i) {
      ureg_ADD(shader, ureg_writemask(t_array[i], TGSI_WRITEMASK_XY),
         ureg_MOV(shader, ureg_writemask(t_array[i], TGSI_WRITEMASK_ZW),
               for (i = 0; i < 16; ++i) {
                  for(i = 0; i < 4; ++i)
      create_frag_shader_cubic_interpolater(shader, ureg_src(t_array[4*i]),
               create_frag_shader_cubic_interpolater(shader, ureg_src(t_array[16]),
                  for(i = 0; i < 23; ++i)
                              }
      bool
   vl_bicubic_filter_init(struct vl_bicubic_filter *filter, struct pipe_context *pipe,
         {
      struct pipe_rasterizer_state rs_state;
   struct pipe_blend_state blend;
   struct vertex2f offsets[16];
   struct pipe_sampler_state sampler;
   struct pipe_vertex_element ve;
            assert(filter && pipe);
            memset(filter, 0, sizeof(*filter));
            memset(&rs_state, 0, sizeof(rs_state));
   rs_state.half_pixel_center = true;
   rs_state.bottom_edge_rule = true;
   rs_state.depth_clip_near = 1;
            filter->rs_state = pipe->create_rasterizer_state(pipe, &rs_state);
   if (!filter->rs_state)
            memset(&blend, 0, sizeof blend);
   blend.rt[0].rgb_func = PIPE_BLEND_ADD;
   blend.rt[0].rgb_src_factor = PIPE_BLENDFACTOR_ONE;
   blend.rt[0].rgb_dst_factor = PIPE_BLENDFACTOR_ONE;
   blend.rt[0].alpha_func = PIPE_BLEND_ADD;
   blend.rt[0].alpha_src_factor = PIPE_BLENDFACTOR_ONE;
   blend.rt[0].alpha_dst_factor = PIPE_BLENDFACTOR_ONE;
   blend.logicop_func = PIPE_LOGICOP_CLEAR;
   blend.rt[0].colormask = PIPE_MASK_RGBA;
   filter->blend = pipe->create_blend_state(pipe, &blend);
   if (!filter->blend)
            memset(&sampler, 0, sizeof(sampler));
   sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.min_img_filter = PIPE_TEX_FILTER_NEAREST;
   sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
   sampler.mag_img_filter = PIPE_TEX_FILTER_NEAREST;
   sampler.compare_mode = PIPE_TEX_COMPARE_NONE;
   sampler.compare_func = PIPE_FUNC_ALWAYS;
   filter->sampler = pipe->create_sampler_state(pipe, &sampler);
   if (!filter->sampler)
            filter->quad = vl_vb_upload_quads(pipe);
   if(!filter->quad.buffer.resource)
            memset(&ve, 0, sizeof(ve));
   ve.src_offset = 0;
   ve.instance_divisor = 0;
   ve.vertex_buffer_index = 0;
   ve.src_format = PIPE_FORMAT_R32G32_FLOAT;
   ve.src_stride = sizeof(struct vertex2f);
   filter->ves = pipe->create_vertex_elements_state(pipe, 1, &ve);
   if (!filter->ves)
            offsets[0].x = -1.0f; offsets[0].y = -1.0f;
   offsets[1].x = 0.0f; offsets[1].y = -1.0f;
   offsets[2].x = 1.0f; offsets[2].y = -1.0f;
            offsets[4].x = -1.0f; offsets[4].y = 0.0f;
   offsets[5].x = 0.0f; offsets[5].y = 0.0f;
   offsets[6].x = 1.0f; offsets[6].y = 0.0f;
            offsets[8].x = -1.0f; offsets[8].y = 1.0f;
   offsets[9].x = 0.0f; offsets[9].y = 1.0f;
   offsets[10].x = 1.0f; offsets[10].y = 1.0f;
            offsets[12].x = -1.0f; offsets[12].y = 2.0f;
   offsets[13].x = 0.0f; offsets[13].y = 2.0f;
   offsets[14].x = 1.0f; offsets[14].y = 2.0f;
            for (i = 0; i < 16; ++i) {
      offsets[i].x /= width;
               filter->vs = create_vert_shader(filter);
   if (!filter->vs)
            filter->fs = create_frag_shader(filter, width, height, offsets);
   if (!filter->fs)
                  error_fs:
            error_vs:
            error_ves:
            error_quad:
            error_sampler:
            error_blend:
            error_rs_state:
         }
      void
   vl_bicubic_filter_cleanup(struct vl_bicubic_filter *filter)
   {
               filter->pipe->delete_sampler_state(filter->pipe, filter->sampler);
   filter->pipe->delete_blend_state(filter->pipe, filter->blend);
   filter->pipe->delete_rasterizer_state(filter->pipe, filter->rs_state);
   filter->pipe->delete_vertex_elements_state(filter->pipe, filter->ves);
            filter->pipe->delete_vs_state(filter->pipe, filter->vs);
      }
      void
   vl_bicubic_filter_render(struct vl_bicubic_filter *filter,
                           {
      struct pipe_viewport_state viewport;
   struct pipe_framebuffer_state fb_state;
   struct pipe_scissor_state scissor;
                     if (dst_clip) {
      scissor.minx = dst_clip->x0;
   scissor.miny = dst_clip->y0;
   scissor.maxx = dst_clip->x1;
      } else {
      scissor.minx = 0;
   scissor.miny = 0;
   scissor.maxx = dst->width;
               clear_color.f[0] = clear_color.f[1] = 0.0f;
            memset(&viewport, 0, sizeof(viewport));
   if(dst_area){
      viewport.scale[0] = dst_area->x1 - dst_area->x0;
   viewport.scale[1] = dst_area->y1 - dst_area->y0;
   viewport.translate[0] = dst_area->x0;
      } else {
      viewport.scale[0] = dst->width;
      }
   viewport.scale[2] = 1;
   viewport.swizzle_x = PIPE_VIEWPORT_SWIZZLE_POSITIVE_X;
   viewport.swizzle_y = PIPE_VIEWPORT_SWIZZLE_POSITIVE_Y;
   viewport.swizzle_z = PIPE_VIEWPORT_SWIZZLE_POSITIVE_Z;
            struct pipe_constant_buffer cb = {0};
            u_upload_alloc(filter->pipe->const_uploader, 0, 2 * sizeof(float), 256,
                  if (ptr) {
      ptr[0] = 0.5f/viewport.scale[0];
      }
            memset(&fb_state, 0, sizeof(fb_state));
   fb_state.width = dst->width;
   fb_state.height = dst->height;
   fb_state.nr_cbufs = 1;
            filter->pipe->set_scissor_states(filter->pipe, 0, 1, &scissor);
   filter->pipe->clear_render_target(filter->pipe, dst, &clear_color,
         filter->pipe->set_constant_buffer(filter->pipe, PIPE_SHADER_FRAGMENT,
         filter->pipe->bind_rasterizer_state(filter->pipe, filter->rs_state);
   filter->pipe->bind_blend_state(filter->pipe, filter->blend);
   filter->pipe->bind_sampler_states(filter->pipe, PIPE_SHADER_FRAGMENT,
         filter->pipe->set_sampler_views(filter->pipe, PIPE_SHADER_FRAGMENT,
         filter->pipe->bind_vs_state(filter->pipe, filter->vs);
   filter->pipe->bind_fs_state(filter->pipe, filter->fs);
   filter->pipe->set_framebuffer_state(filter->pipe, &fb_state);
   filter->pipe->set_viewport_states(filter->pipe, 0, 1, &viewport);
   filter->pipe->set_vertex_buffers(filter->pipe, 1, 0, false, &filter->quad);
               }
