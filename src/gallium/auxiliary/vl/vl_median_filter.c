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
      #include "pipe/p_context.h"
      #include "tgsi/tgsi_ureg.h"
      #include "util/u_draw.h"
   #include "util/u_memory.h"
   #include "util/u_math.h"
      #include "vl_types.h"
   #include "vl_vertex_buffers.h"
   #include "vl_median_filter.h"
      enum VS_OUTPUT
   {
      VS_O_VPOS = 0,
      };
      static void *
   create_vert_shader(struct vl_median_filter *filter)
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
   create_frag_shader(struct vl_median_filter *filter,
               {
      struct pipe_screen *screen = filter->pipe->screen;
   struct ureg_program *shader;
   struct ureg_src i_vtex;
   struct ureg_src sampler;
   struct ureg_dst *t_array = MALLOC(sizeof(struct ureg_dst) * num_offsets);
   struct ureg_dst o_fragment;
   const unsigned median = num_offsets >> 1;
            assert(num_offsets & 1); /* we need an odd number of offsets */
   if (!(num_offsets & 1)) { /* yeah, we REALLY need an odd number of offsets!!! */
      FREE(t_array);
               if (num_offsets > screen->get_shader_param(
               FREE(t_array);
               shader = ureg_create(PIPE_SHADER_FRAGMENT);
   if (!shader) {
      FREE(t_array);
               i_vtex = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_VTEX, TGSI_INTERPOLATE_LINEAR);
   sampler = ureg_DECL_sampler(shader, 0);
   ureg_DECL_sampler_view(shader, 0, TGSI_TEXTURE_2D,
                              for (i = 0; i < num_offsets; ++i)
                  /*
   * t_array[0..*] = vtex + offset[0..*]
   * t_array[0..*] = tex(t_array[0..*], sampler)
   * result = partial_bubblesort(t_array)[mid]
            for (i = 0; i < num_offsets; ++i) {
      if (!is_vec_zero(offsets[i])) {
      ureg_ADD(shader, ureg_writemask(t_array[i], TGSI_WRITEMASK_XY),
         ureg_MOV(shader, ureg_writemask(t_array[i], TGSI_WRITEMASK_ZW),
                  for (i = 0; i < num_offsets; ++i) {
      struct ureg_src src = is_vec_zero(offsets[i]) ? i_vtex : ureg_src(t_array[i]);
               // TODO: Couldn't this be improved even more?
   for (i = 0; i <= median; ++i) {
      for (j = 1; j < (num_offsets - i - 1); ++j) {
      struct ureg_dst tmp = ureg_DECL_temporary(shader);
   ureg_MOV(shader, tmp, ureg_src(t_array[j]));
   ureg_MAX(shader, t_array[j], ureg_src(t_array[j]), ureg_src(t_array[j - 1]));
   ureg_MIN(shader, t_array[j - 1], ureg_src(tmp), ureg_src(t_array[j - 1]));
      }
   if (i == median)
         else
      }
                     FREE(t_array);
      }
      static void
   generate_offsets(enum vl_median_filter_shape shape, unsigned size,
         {
      unsigned i = 0;
   int half_size;
   int x;
                     /* size needs to be odd */
   size = align(size + 1, 2) - 1;
            switch(shape) {
   case VL_MEDIAN_FILTER_BOX:
      *num_offsets = size*size;
         case VL_MEDIAN_FILTER_CROSS:
   case VL_MEDIAN_FILTER_X:
      *num_offsets = size + size - 1;
         case VL_MEDIAN_FILTER_HORIZONTAL:
   case VL_MEDIAN_FILTER_VERTICAL:
      *num_offsets = size;
         default:
      *num_offsets = 0;
               *offsets = MALLOC(sizeof(struct vertex2f) * *num_offsets);
   if (!*offsets)
            switch(shape) {
   case VL_MEDIAN_FILTER_BOX:
      for (x = -half_size; x <= half_size; ++x)
      for (y = -half_size; y <= half_size; ++y) {
      (*offsets)[i].x = x;
   (*offsets)[i].y = y;
               case VL_MEDIAN_FILTER_CROSS:
      y = 0;
   for (x = -half_size; x <= half_size; ++x) {
      (*offsets)[i].x = x;
   (*offsets)[i].y = y;
               x = 0;
   for (y = -half_size; y <= half_size; ++y)
      if (y != 0) {
      (*offsets)[i].x = x;
   (*offsets)[i].y = y;
               case VL_MEDIAN_FILTER_X:
      for (x = y = -half_size; x <= half_size; ++x, ++y) {
      (*offsets)[i].x = x;
   (*offsets)[i].y = y;
               for (x = -half_size, y = half_size; x <= half_size; ++x, --y)
      if (y != 0) {
      (*offsets)[i].x = x;
   (*offsets)[i].y = y;
               case VL_MEDIAN_FILTER_HORIZONTAL:
      y = 0;
   for (x = -half_size; x <= half_size; ++x) {
      (*offsets)[i].x = x;
   (*offsets)[i].y = y;
      }
         case VL_MEDIAN_FILTER_VERTICAL:
      x = 0;
   for (y = -half_size; y <= half_size; ++y) {
      (*offsets)[i].x = x;
   (*offsets)[i].y = y;
      }
                  }
      bool
   vl_median_filter_init(struct vl_median_filter *filter, struct pipe_context *pipe,
               {
      struct pipe_rasterizer_state rs_state;
   struct pipe_blend_state blend;
   struct pipe_sampler_state sampler;
   struct vertex2f *offsets = NULL;
   struct pipe_vertex_element ve;
            assert(filter && pipe);
   assert(width && height);
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
            generate_offsets(shape, size, &offsets, &num_offsets);
   if (!offsets)
            for (i = 0; i < num_offsets; ++i) {
      offsets[i].x /= width;
               filter->vs = create_vert_shader(filter);
   if (!filter->vs)
            filter->fs = create_frag_shader(filter, offsets, num_offsets);
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
   vl_median_filter_cleanup(struct vl_median_filter *filter)
   {
               filter->pipe->delete_sampler_state(filter->pipe, filter->sampler);
   filter->pipe->delete_blend_state(filter->pipe, filter->blend);
   filter->pipe->delete_rasterizer_state(filter->pipe, filter->rs_state);
   filter->pipe->delete_vertex_elements_state(filter->pipe, filter->ves);
            filter->pipe->delete_vs_state(filter->pipe, filter->vs);
      }
      void
   vl_median_filter_render(struct vl_median_filter *filter,
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
