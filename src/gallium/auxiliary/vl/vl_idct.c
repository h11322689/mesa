   /**************************************************************************
   *
   * Copyright 2010 Christian König
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
      #include "pipe/p_context.h"
   #include "pipe/p_screen.h"
      #include "util/u_draw.h"
   #include "util/u_sampler.h"
   #include "util/u_memory.h"
      #include "tgsi/tgsi_ureg.h"
      #include "vl_defines.h"
   #include "vl_types.h"
   #include "vl_vertex_buffers.h"
   #include "vl_idct.h"
      enum VS_OUTPUT
   {
      VS_O_VPOS = 0,
   VS_O_L_ADDR0 = 0,
   VS_O_L_ADDR1,
   VS_O_R_ADDR0,
      };
      /**
   * The DCT matrix stored as hex representation of floats. Equal to the following equation:
   * for (i = 0; i < 8; ++i)
   *    for (j = 0; j < 8; ++j)
   *       if (i == 0) const_matrix[i][j] = 1.0f / sqrtf(8.0f);
   *       else const_matrix[i][j] = sqrtf(2.0f / 8.0f) * cosf((2 * j + 1) * i * M_PI / (2.0f * 8.0f));
   */
   static const uint32_t const_matrix[8][8] = {
      { 0x3eb504f3, 0x3eb504f3, 0x3eb504f3, 0x3eb504f3, 0x3eb504f3, 0x3eb504f3, 0x3eb504f3, 0x3eb504f3 },
   { 0x3efb14be, 0x3ed4db31, 0x3e8e39da, 0x3dc7c5c4, 0xbdc7c5c2, 0xbe8e39d9, 0xbed4db32, 0xbefb14bf },
   { 0x3eec835f, 0x3e43ef15, 0xbe43ef14, 0xbeec835e, 0xbeec835f, 0xbe43ef1a, 0x3e43ef1b, 0x3eec835f },
   { 0x3ed4db31, 0xbdc7c5c2, 0xbefb14bf, 0xbe8e39dd, 0x3e8e39d7, 0x3efb14bf, 0x3dc7c5d0, 0xbed4db34 },
   { 0x3eb504f3, 0xbeb504f3, 0xbeb504f4, 0x3eb504f1, 0x3eb504f3, 0xbeb504f0, 0xbeb504ef, 0x3eb504f4 },
   { 0x3e8e39da, 0xbefb14bf, 0x3dc7c5c8, 0x3ed4db32, 0xbed4db34, 0xbdc7c5bb, 0x3efb14bf, 0xbe8e39d7 },
   { 0x3e43ef15, 0xbeec835f, 0x3eec835f, 0xbe43ef07, 0xbe43ef23, 0x3eec8361, 0xbeec835c, 0x3e43ef25 },
      };
      static void
   calc_addr(struct ureg_program *shader, struct ureg_dst addr[2],
               {
      unsigned wm_start = (right_side == transposed) ? TGSI_WRITEMASK_X : TGSI_WRITEMASK_Y;
            unsigned wm_tc = (right_side == transposed) ? TGSI_WRITEMASK_Y : TGSI_WRITEMASK_X;
            /*
   * addr[0..1].(start) = right_side ? start.x : tc.x
   * addr[0..1].(tc) = right_side ? tc.y : start.y
   * addr[0..1].z = tc.z
   * addr[1].(start) += 1.0f / scale
   */
   ureg_MOV(shader, ureg_writemask(addr[0], wm_start), ureg_scalar(start, sw_start));
            ureg_ADD(shader, ureg_writemask(addr[1], wm_start), ureg_scalar(start, sw_start), ureg_imm1f(shader, 1.0f / size));
      }
      static void
   increment_addr(struct ureg_program *shader, struct ureg_dst daddr[2],
               {
      unsigned wm_start = (right_side == transposed) ? TGSI_WRITEMASK_X : TGSI_WRITEMASK_Y;
            /*
   * daddr[0..1].(start) = saddr[0..1].(start)
   * daddr[0..1].(tc) = saddr[0..1].(tc)
            ureg_MOV(shader, ureg_writemask(daddr[0], wm_start), saddr[0]);
   ureg_ADD(shader, ureg_writemask(daddr[0], wm_tc), saddr[0], ureg_imm1f(shader, pos / size));
   ureg_MOV(shader, ureg_writemask(daddr[1], wm_start), saddr[1]);
      }
      static void
   fetch_four(struct ureg_program *shader, struct ureg_dst m[2], struct ureg_src addr[2],
         {
      ureg_TEX(shader, m[0], resource3d ? TGSI_TEXTURE_3D : TGSI_TEXTURE_2D, addr[0], sampler);
      }
      static void
   matrix_mul(struct ureg_program *shader, struct ureg_dst dst, struct ureg_dst l[2], struct ureg_dst r[2])
   {
                        /*
   * tmp.xy = dot4(m[0][0..1], m[1][0..1])
   * dst = tmp.x + tmp.y
   */
   ureg_DP4(shader, ureg_writemask(tmp, TGSI_WRITEMASK_X), ureg_src(l[0]), ureg_src(r[0]));
   ureg_DP4(shader, ureg_writemask(tmp, TGSI_WRITEMASK_Y), ureg_src(l[1]), ureg_src(r[1]));
   ureg_ADD(shader, dst,
      ureg_scalar(ureg_src(tmp), TGSI_SWIZZLE_X),
            }
      static void *
   create_mismatch_vert_shader(struct vl_idct *idct)
   {
      struct ureg_program *shader;
   struct ureg_src vpos;
   struct ureg_src scale;
   struct ureg_dst t_tex;
            shader = ureg_create(PIPE_SHADER_VERTEX);
   if (!shader)
                                       o_addr[0] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_L_ADDR0);
            /*
   * scale = (VL_BLOCK_WIDTH, VL_BLOCK_HEIGHT) / (dst.width, dst.height)
   *
   * t_vpos = vpos + 7 / VL_BLOCK_WIDTH
   * o_vpos.xy = t_vpos * scale
   *
   * o_addr = calc_addr(...)
   *
            scale = ureg_imm2f(shader,
      (float)VL_BLOCK_WIDTH / idct->buffer_width,
         ureg_MAD(shader, ureg_writemask(o_vpos, TGSI_WRITEMASK_XY), vpos, scale, scale);
            ureg_MUL(shader, ureg_writemask(t_tex, TGSI_WRITEMASK_XY), vpos, scale);
                                 }
      static void *
   create_mismatch_frag_shader(struct vl_idct *idct)
   {
                        struct ureg_dst m[8][2];
                     shader = ureg_create(PIPE_SHADER_FRAGMENT);
   if (!shader)
            addr[0] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_L_ADDR0, TGSI_INTERPOLATE_LINEAR);
                     for (i = 0; i < 8; ++i) {
      m[i][0] = ureg_DECL_temporary(shader);
               for (i = 0; i < 8; ++i) {
                  for (i = 0; i < 8; ++i) {
      struct ureg_src s_address[2];
   s_address[0] = ureg_src(m[i][0]);
   s_address[1] = ureg_src(m[i][1]);
               for (i = 1; i < 8; ++i) {
      ureg_ADD(shader, m[0][0], ureg_src(m[0][0]), ureg_src(m[i][0]));
               ureg_ADD(shader, m[0][0], ureg_src(m[0][0]), ureg_src(m[0][1]));
            ureg_MUL(shader, ureg_writemask(m[0][0], TGSI_WRITEMASK_W), ureg_abs(ureg_src(m[7][1])), ureg_imm1f(shader, 1 << 14));
   ureg_FRC(shader, m[0][0], ureg_src(m[0][0]));
            ureg_CMP(shader, ureg_writemask(m[0][0], TGSI_WRITEMASK_W), ureg_negate(ureg_src(m[0][0])),
         ureg_MUL(shader, ureg_writemask(m[0][0], TGSI_WRITEMASK_W), ureg_src(m[0][0]),
            ureg_MOV(shader, ureg_writemask(fragment, TGSI_WRITEMASK_XYZ), ureg_src(m[7][1]));
            for (i = 0; i < 8; ++i) {
      ureg_release_temporary(shader, m[i][0]);
                           }
      static void *
   create_stage1_vert_shader(struct vl_idct *idct)
   {
      struct ureg_program *shader;
   struct ureg_src vrect, vpos;
   struct ureg_src scale;
   struct ureg_dst t_tex, t_start;
            shader = ureg_create(PIPE_SHADER_VERTEX);
   if (!shader)
            vrect = ureg_DECL_vs_input(shader, VS_I_RECT);
            t_tex = ureg_DECL_temporary(shader);
                     o_l_addr[0] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_L_ADDR0);
            o_r_addr[0] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_R_ADDR0);
            /*
   * scale = (VL_BLOCK_WIDTH, VL_BLOCK_HEIGHT) / (dst.width, dst.height)
   *
   * t_vpos = vpos + vrect
   * o_vpos.xy = t_vpos * scale
   * o_vpos.zw = vpos
   *
   * o_l_addr = calc_addr(...)
   * o_r_addr = calc_addr(...)
   *
            scale = ureg_imm2f(shader,
      (float)VL_BLOCK_WIDTH / idct->buffer_width,
         ureg_ADD(shader, ureg_writemask(t_tex, TGSI_WRITEMASK_XY), vpos, vrect);
            ureg_MOV(shader, ureg_writemask(o_vpos, TGSI_WRITEMASK_XY), ureg_src(t_tex));
                     calc_addr(shader, o_l_addr, ureg_src(t_tex), ureg_src(t_start), false, false, idct->buffer_width / 4);
            ureg_release_temporary(shader, t_tex);
                        }
      static void *
   create_stage1_frag_shader(struct vl_idct *idct)
   {
      struct ureg_program *shader;
   struct ureg_src l_addr[2], r_addr[2];
   struct ureg_dst l[4][2], r[2];
   struct ureg_dst *fragment;
   unsigned i;
            shader = ureg_create(PIPE_SHADER_FRAGMENT);
   if (!shader)
                     l_addr[0] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_L_ADDR0, TGSI_INTERPOLATE_LINEAR);
            r_addr[0] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_R_ADDR0, TGSI_INTERPOLATE_LINEAR);
            for (i = 0; i < idct->nr_of_render_targets; ++i)
            for (i = 0; i < 4; ++i) {
      l[i][0] = ureg_DECL_temporary(shader);
               r[0] = ureg_DECL_temporary(shader);
            for (i = 0; i < 4; ++i) {
                  for (i = 0; i < 4; ++i) {
      struct ureg_src s_address[2];
   s_address[0] = ureg_src(l[i][0]);
   s_address[1] = ureg_src(l[i][1]);
               for (i = 0; i < idct->nr_of_render_targets; ++i) {
                        s_address[0] = ureg_src(r[0]);
   s_address[1] = ureg_src(r[1]);
            for (j = 0; j < 4; ++j) {
                     for (i = 0; i < 4; ++i) {
      ureg_release_temporary(shader, l[i][0]);
      }
   ureg_release_temporary(shader, r[0]);
                                 }
      void
   vl_idct_stage2_vert_shader(struct vl_idct *idct, struct ureg_program *shader,
         {
      struct ureg_src vrect, vpos;
   struct ureg_src scale;
   struct ureg_dst t_start;
            vrect = ureg_DECL_vs_input(shader, VS_I_RECT);
                              o_l_addr[0] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, first_output + VS_O_L_ADDR0);
            o_r_addr[0] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, first_output + VS_O_R_ADDR0);
            scale = ureg_imm2f(shader,
      (float)VL_BLOCK_WIDTH / idct->buffer_width,
         ureg_MUL(shader, ureg_writemask(tex, TGSI_WRITEMASK_Z),
      ureg_scalar(vrect, TGSI_SWIZZLE_X),
               calc_addr(shader, o_l_addr, vrect, ureg_imm1f(shader, 0.0f), false, false, VL_BLOCK_WIDTH / 4);
            ureg_MOV(shader, ureg_writemask(o_r_addr[0], TGSI_WRITEMASK_Z), ureg_src(tex));
      }
      void
   vl_idct_stage2_frag_shader(struct vl_idct *idct, struct ureg_program *shader,
         {
                                 l_addr[0] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, first_input + VS_O_L_ADDR0, TGSI_INTERPOLATE_LINEAR);
            r_addr[0] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, first_input + VS_O_R_ADDR0, TGSI_INTERPOLATE_LINEAR);
            l[0] = ureg_DECL_temporary(shader);
   l[1] = ureg_DECL_temporary(shader);
   r[0] = ureg_DECL_temporary(shader);
            fetch_four(shader, l, l_addr, ureg_DECL_sampler(shader, 1), false);
                     ureg_release_temporary(shader, l[0]);
   ureg_release_temporary(shader, l[1]);
   ureg_release_temporary(shader, r[0]);
      }
      static bool
   init_shaders(struct vl_idct *idct)
   {
      idct->vs_mismatch = create_mismatch_vert_shader(idct);
   if (!idct->vs_mismatch)
            idct->fs_mismatch = create_mismatch_frag_shader(idct);
   if (!idct->fs_mismatch)
            idct->vs = create_stage1_vert_shader(idct);
   if (!idct->vs)
            idct->fs = create_stage1_frag_shader(idct);
   if (!idct->fs)
                  error_fs:
            error_vs:
            error_fs_mismatch:
            error_vs_mismatch:
         }
      static void
   cleanup_shaders(struct vl_idct *idct)
   {
      idct->pipe->delete_vs_state(idct->pipe, idct->vs_mismatch);
   idct->pipe->delete_fs_state(idct->pipe, idct->fs_mismatch);
   idct->pipe->delete_vs_state(idct->pipe, idct->vs);
      }
      static bool
   init_state(struct vl_idct *idct)
   {
      struct pipe_blend_state blend;
   struct pipe_rasterizer_state rs_state;
   struct pipe_sampler_state sampler;
                     memset(&rs_state, 0, sizeof(rs_state));
   rs_state.point_size = 1;
   rs_state.half_pixel_center = true;
   rs_state.bottom_edge_rule = true;
   rs_state.depth_clip_near = 1;
            idct->rs_state = idct->pipe->create_rasterizer_state(idct->pipe, &rs_state);
   if (!idct->rs_state)
                     blend.independent_blend_enable = 0;
   blend.rt[0].blend_enable = 0;
   blend.rt[0].rgb_func = PIPE_BLEND_ADD;
   blend.rt[0].rgb_src_factor = PIPE_BLENDFACTOR_ONE;
   blend.rt[0].rgb_dst_factor = PIPE_BLENDFACTOR_ONE;
   blend.rt[0].alpha_func = PIPE_BLEND_ADD;
   blend.rt[0].alpha_src_factor = PIPE_BLENDFACTOR_ONE;
   blend.rt[0].alpha_dst_factor = PIPE_BLENDFACTOR_ONE;
   blend.logicop_enable = 0;
   blend.logicop_func = PIPE_LOGICOP_CLEAR;
   /* Needed to allow color writes to FB, even if blending disabled */
   blend.rt[0].colormask = PIPE_MASK_RGBA;
   blend.dither = 0;
   idct->blend = idct->pipe->create_blend_state(idct->pipe, &blend);
   if (!idct->blend)
            for (i = 0; i < 2; ++i) {
      memset(&sampler, 0, sizeof(sampler));
   sampler.wrap_s = PIPE_TEX_WRAP_REPEAT;
   sampler.wrap_t = PIPE_TEX_WRAP_REPEAT;
   sampler.wrap_r = PIPE_TEX_WRAP_REPEAT;
   sampler.min_img_filter = PIPE_TEX_FILTER_NEAREST;
   sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
   sampler.mag_img_filter = PIPE_TEX_FILTER_NEAREST;
   sampler.compare_mode = PIPE_TEX_COMPARE_NONE;
   sampler.compare_func = PIPE_FUNC_ALWAYS;
   idct->samplers[i] = idct->pipe->create_sampler_state(idct->pipe, &sampler);
   if (!idct->samplers[i])
                     error_samplers:
      for (i = 0; i < 2; ++i)
      if (idct->samplers[i])
               error_blend:
            error_rs_state:
         }
      static void
   cleanup_state(struct vl_idct *idct)
   {
               for (i = 0; i < 2; ++i)
            idct->pipe->delete_rasterizer_state(idct->pipe, idct->rs_state);
      }
      static bool
   init_source(struct vl_idct *idct, struct vl_idct_buffer *buffer)
   {
      struct pipe_resource *tex;
                              buffer->fb_state_mismatch.width = tex->width0;
   buffer->fb_state_mismatch.height = tex->height0;
            memset(&surf_templ, 0, sizeof(surf_templ));
   surf_templ.format = tex->format;
   surf_templ.u.tex.first_layer = 0;
   surf_templ.u.tex.last_layer = 0;
            buffer->viewport_mismatch.scale[0] = tex->width0;
   buffer->viewport_mismatch.scale[1] = tex->height0;
   buffer->viewport_mismatch.scale[2] = 1;
   buffer->viewport_mismatch.swizzle_x = PIPE_VIEWPORT_SWIZZLE_POSITIVE_X;
   buffer->viewport_mismatch.swizzle_y = PIPE_VIEWPORT_SWIZZLE_POSITIVE_Y;
   buffer->viewport_mismatch.swizzle_z = PIPE_VIEWPORT_SWIZZLE_POSITIVE_Z;
               }
      static void
   cleanup_source(struct vl_idct_buffer *buffer)
   {
                           }
      static bool
   init_intermediate(struct vl_idct *idct, struct vl_idct_buffer *buffer)
   {
      struct pipe_resource *tex;
   struct pipe_surface surf_templ;
                              buffer->fb_state.width = tex->width0;
   buffer->fb_state.height = tex->height0;
   buffer->fb_state.nr_cbufs = idct->nr_of_render_targets;
   for(i = 0; i < idct->nr_of_render_targets; ++i) {
      memset(&surf_templ, 0, sizeof(surf_templ));
   surf_templ.format = tex->format;
   surf_templ.u.tex.first_layer = i;
   surf_templ.u.tex.last_layer = i;
   buffer->fb_state.cbufs[i] = idct->pipe->create_surface(
            if (!buffer->fb_state.cbufs[i])
               buffer->viewport.scale[0] = tex->width0;
   buffer->viewport.scale[1] = tex->height0;
   buffer->viewport.scale[2] = 1;
   buffer->viewport.swizzle_x = PIPE_VIEWPORT_SWIZZLE_POSITIVE_X;
   buffer->viewport.swizzle_y = PIPE_VIEWPORT_SWIZZLE_POSITIVE_Y;
   buffer->viewport.swizzle_z = PIPE_VIEWPORT_SWIZZLE_POSITIVE_Z;
                  error_surfaces:
      for(i = 0; i < idct->nr_of_render_targets; ++i)
               }
      static void
   cleanup_intermediate(struct vl_idct_buffer *buffer)
   {
                        for(i = 0; i < PIPE_MAX_COLOR_BUFS; ++i)
               }
      struct pipe_sampler_view *
   vl_idct_upload_matrix(struct pipe_context *pipe, float scale)
   {
      struct pipe_resource tex_templ, *matrix;
   struct pipe_sampler_view sv_templ, *sv;
   struct pipe_transfer *buf_transfer;
   unsigned i, j, pitch;
            struct pipe_box rect =
   {
      0, 0, 0,
   VL_BLOCK_WIDTH / 4,
   VL_BLOCK_HEIGHT,
                        memset(&tex_templ, 0, sizeof(tex_templ));
   tex_templ.target = PIPE_TEXTURE_2D;
   tex_templ.format = PIPE_FORMAT_R32G32B32A32_FLOAT;
   tex_templ.last_level = 0;
   tex_templ.width0 = 2;
   tex_templ.height0 = 8;
   tex_templ.depth0 = 1;
   tex_templ.array_size = 1;
   tex_templ.usage = PIPE_USAGE_IMMUTABLE;
   tex_templ.bind = PIPE_BIND_SAMPLER_VIEW;
            matrix = pipe->screen->resource_create(pipe->screen, &tex_templ);
   if (!matrix)
            f = pipe->texture_map(pipe, matrix, 0,
                     if (!f)
                     for(i = 0; i < VL_BLOCK_HEIGHT; ++i)
      for(j = 0; j < VL_BLOCK_WIDTH; ++j)
                        memset(&sv_templ, 0, sizeof(sv_templ));
   u_sampler_view_default_template(&sv_templ, matrix, matrix->format);
   sv = pipe->create_sampler_view(pipe, matrix, &sv_templ);
   pipe_resource_reference(&matrix, NULL);
   if (!sv)
                  error_map:
            error_matrix:
         }
      bool vl_idct_init(struct vl_idct *idct, struct pipe_context *pipe,
                     unsigned buffer_width, unsigned buffer_height,
   {
      assert(idct && pipe);
            idct->pipe = pipe;
   idct->buffer_width = buffer_width;
   idct->buffer_height = buffer_height;
            pipe_sampler_view_reference(&idct->matrix, matrix);
            if(!init_shaders(idct))
            if(!init_state(idct)) {
      cleanup_shaders(idct);
                  }
      void
   vl_idct_cleanup(struct vl_idct *idct)
   {
      cleanup_shaders(idct);
            pipe_sampler_view_reference(&idct->matrix, NULL);
      }
      bool
   vl_idct_init_buffer(struct vl_idct *idct, struct vl_idct_buffer *buffer,
               {
      assert(buffer && idct);
                     pipe_sampler_view_reference(&buffer->sampler_views.individual.matrix, idct->matrix);
   pipe_sampler_view_reference(&buffer->sampler_views.individual.source, source);
   pipe_sampler_view_reference(&buffer->sampler_views.individual.transpose, idct->transpose);
            if (!init_source(idct, buffer))
            if (!init_intermediate(idct, buffer))
               }
      void
   vl_idct_cleanup_buffer(struct vl_idct_buffer *buffer)
   {
               cleanup_source(buffer);
            pipe_sampler_view_reference(&buffer->sampler_views.individual.matrix, NULL);
      }
      void
   vl_idct_flush(struct vl_idct *idct, struct vl_idct_buffer *buffer, unsigned num_instances)
   {
               idct->pipe->bind_rasterizer_state(idct->pipe, idct->rs_state);
            idct->pipe->bind_sampler_states(idct->pipe, PIPE_SHADER_FRAGMENT,
            idct->pipe->set_sampler_views(idct->pipe, PIPE_SHADER_FRAGMENT, 0, 2, 0,
            /* mismatch control */
   idct->pipe->set_framebuffer_state(idct->pipe, &buffer->fb_state_mismatch);
   idct->pipe->set_viewport_states(idct->pipe, 0, 1, &buffer->viewport_mismatch);
   idct->pipe->bind_vs_state(idct->pipe, idct->vs_mismatch);
   idct->pipe->bind_fs_state(idct->pipe, idct->fs_mismatch);
            /* first stage */
   idct->pipe->set_framebuffer_state(idct->pipe, &buffer->fb_state);
   idct->pipe->set_viewport_states(idct->pipe, 0, 1, &buffer->viewport);
   idct->pipe->bind_vs_state(idct->pipe, idct->vs);
   idct->pipe->bind_fs_state(idct->pipe, idct->fs);
      }
      void
   vl_idct_prepare_stage2(struct vl_idct *idct, struct vl_idct_buffer *buffer)
   {
               /* second stage */
   idct->pipe->bind_rasterizer_state(idct->pipe, idct->rs_state);
   idct->pipe->bind_sampler_states(idct->pipe, PIPE_SHADER_FRAGMENT,
         idct->pipe->set_sampler_views(idct->pipe, PIPE_SHADER_FRAGMENT,
      }
   