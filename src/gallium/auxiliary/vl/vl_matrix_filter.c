   /**************************************************************************
   *
   * Copyright 2012 Christian König.
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
      #include "vl_types.h"
   #include "vl_vertex_buffers.h"
   #include "vl_matrix_filter.h"
      enum VS_OUTPUT
   {
      VS_O_VPOS = 0,
      };
      static void *
   create_vert_shader(struct vl_matrix_filter *filter)
   {
      struct ureg_program *shader;
   struct ureg_src i_vpos;
            shader = ureg_create(PIPE_SHADER_VERTEX);
   if (!shader)
            i_vpos = ureg_DECL_vs_input(shader, 0);
   o_vpos = ureg_DECL_output(shader, TGSI_SEMANTIC_POSITION, VS_O_VPOS);
            ureg_MOV(shader, o_vpos, i_vpos);
                        }
      static inline bool
   is_vec_zero(struct vertex2f v)
   {
         }
      static void *
   create_frag_shader(struct vl_matrix_filter *filter, unsigned num_offsets,
         {
      struct ureg_program *shader;
   struct ureg_src i_vtex;
   struct ureg_src sampler;
   struct ureg_dst tmp;
   struct ureg_dst t_sum;
   struct ureg_dst o_fragment;
            shader = ureg_create(PIPE_SHADER_FRAGMENT);
   if (!shader) {
                  i_vtex = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_VTEX, TGSI_INTERPOLATE_LINEAR);
   sampler = ureg_DECL_sampler(shader, 0);
   ureg_DECL_sampler_view(shader, 0, TGSI_TEXTURE_2D,
                              tmp = ureg_DECL_temporary(shader);
   t_sum = ureg_DECL_temporary(shader);
            ureg_MOV(shader, t_sum, ureg_imm1f(shader, 0.0f));
   for (i = 0; i < num_offsets; ++i) {
      if (matrix_values[i] == 0.0f)
            if (!is_vec_zero(offsets[i])) {
      ureg_ADD(shader, ureg_writemask(tmp, TGSI_WRITEMASK_XY),
         ureg_MOV(shader, ureg_writemask(tmp, TGSI_WRITEMASK_ZW),
            } else {
         }
   ureg_MAD(shader, t_sum, ureg_src(tmp), ureg_imm1f(shader, matrix_values[i]),
                                    }
      bool
   vl_matrix_filter_init(struct vl_matrix_filter *filter, struct pipe_context *pipe,
                     {
      struct pipe_rasterizer_state rs_state;
   struct pipe_blend_state blend;
   struct pipe_sampler_state sampler;
   struct pipe_vertex_element ve;
   struct vertex2f *offsets;
   unsigned i, num_offsets = matrix_width * matrix_height;
   int x;
   int y;
   int size_x;
            assert(filter && pipe);
   assert(video_width && video_height);
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
   ve.src_stride = sizeof(struct vertex2f);
   ve.src_offset = 0;
   ve.instance_divisor = 0;
   ve.vertex_buffer_index = 0;
   ve.src_format = PIPE_FORMAT_R32G32_FLOAT;
   filter->ves = pipe->create_vertex_elements_state(pipe, 1, &ve);
   if (!filter->ves)
            offsets = MALLOC(sizeof(struct vertex2f) * num_offsets);
   if (!offsets)
            size_x = (matrix_width - 1) / 2;
            for (x = -size_x, i = 0; x <= size_x; x++)
      for (y = -size_y; y <= size_y; y++) {
      offsets[i].x = x;
   offsets[i].y = y;
            for (i = 0; i < num_offsets; ++i) {
      offsets[i].x /= video_width;
               filter->vs = create_vert_shader(filter);
   if (!filter->vs)
            filter->fs = create_frag_shader(filter, num_offsets, offsets, matrix_values);
   if (!filter->fs)
            FREE(offsets);
         error_fs:
            error_vs:
            error_offsets:
            error_ves:
            error_quad:
            error_sampler:
            error_blend:
            error_rs_state:
         }
      void
   vl_matrix_filter_cleanup(struct vl_matrix_filter *filter)
   {
               filter->pipe->delete_sampler_state(filter->pipe, filter->sampler);
   filter->pipe->delete_blend_state(filter->pipe, filter->blend);
   filter->pipe->delete_rasterizer_state(filter->pipe, filter->rs_state);
   filter->pipe->delete_vertex_elements_state(filter->pipe, filter->ves);
            filter->pipe->delete_vs_state(filter->pipe, filter->vs);
      }
      void
   vl_matrix_filter_render(struct vl_matrix_filter *filter,
               {
      struct pipe_viewport_state viewport;
                     memset(&viewport, 0, sizeof(viewport));
   viewport.scale[0] = dst->width;
   viewport.scale[1] = dst->height;
   viewport.scale[2] = 1;
   viewport.swizzle_x = PIPE_VIEWPORT_SWIZZLE_POSITIVE_X;
   viewport.swizzle_y = PIPE_VIEWPORT_SWIZZLE_POSITIVE_Y;
   viewport.swizzle_z = PIPE_VIEWPORT_SWIZZLE_POSITIVE_Z;
            memset(&fb_state, 0, sizeof(fb_state));
   fb_state.width = dst->width;
   fb_state.height = dst->height;
   fb_state.nr_cbufs = 1;
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
