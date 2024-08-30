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
      #include "pipe/p_context.h"
      #include "util/u_sampler.h"
   #include "util/u_draw.h"
      #include "tgsi/tgsi_ureg.h"
      #include "vl_defines.h"
   #include "vl_vertex_buffers.h"
   #include "vl_mc.h"
   #include "vl_idct.h"
      enum VS_OUTPUT
   {
      VS_O_VPOS = 0,
   VS_O_VTOP = 0,
            VS_O_FLAGS = VS_O_VTOP,
      };
      static struct ureg_dst
   calc_position(struct vl_mc *r, struct ureg_program *shader, struct ureg_src block_scale)
   {
      struct ureg_src vrect, vpos;
   struct ureg_dst t_vpos;
            vrect = ureg_DECL_vs_input(shader, VS_I_RECT);
                              /*
   * block_scale = (VL_MACROBLOCK_WIDTH, VL_MACROBLOCK_HEIGHT) / (dst.width, dst.height)
   *
   * t_vpos = (vpos + vrect) * block_scale
   * o_vpos.xy = t_vpos
   * o_vpos.zw = vpos
   */
   ureg_ADD(shader, ureg_writemask(t_vpos, TGSI_WRITEMASK_XY), vpos, vrect);
   ureg_MUL(shader, ureg_writemask(t_vpos, TGSI_WRITEMASK_XY), ureg_src(t_vpos), block_scale);
   ureg_MOV(shader, ureg_writemask(o_vpos, TGSI_WRITEMASK_XY), ureg_src(t_vpos));
               }
      static struct ureg_dst
   calc_line(struct pipe_screen *screen, struct ureg_program *shader)
   {
      struct ureg_dst tmp;
                     if (screen->get_param(screen, PIPE_CAP_FS_POSITION_IS_SYSVAL))
         else
      pos = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_POSITION, VS_O_VPOS,
         /*
   * tmp.y = fraction(pos.y / 2) >= 0.5 ? 1 : 0
   */
   ureg_MUL(shader, ureg_writemask(tmp, TGSI_WRITEMASK_Y), pos, ureg_imm1f(shader, 0.5f));
   ureg_FRC(shader, ureg_writemask(tmp, TGSI_WRITEMASK_Y), ureg_src(tmp));
               }
      static void *
   create_ref_vert_shader(struct vl_mc *r)
   {
      struct ureg_program *shader;
   struct ureg_src mv_scale;
   struct ureg_src vmv[2];
   struct ureg_dst t_vpos;
   struct ureg_dst o_vmv[2];
            shader = ureg_create(PIPE_SHADER_VERTEX);
   if (!shader)
            vmv[0] = ureg_DECL_vs_input(shader, VS_I_MV_TOP);
            t_vpos = calc_position(r, shader, ureg_imm2f(shader,
      (float)VL_MACROBLOCK_WIDTH / r->buffer_width,
               o_vmv[0] = ureg_DECL_output(shader, TGSI_SEMANTIC_GENERIC, VS_O_VTOP);
            /*
   * mv_scale.xy = 0.5 / (dst.width, dst.height);
   * mv_scale.z = 1.0f / 4.0f
   * mv_scale.w = 1.0f / 255.0f
   *
   * // Apply motion vectors
   * o_vmv[0..1].xy = vmv[0..1] * mv_scale + t_vpos
   * o_vmv[0..1].zw = vmv[0..1] * mv_scale
   *
            mv_scale = ureg_imm4f(shader,
      0.5f / r->buffer_width,
   0.5f / r->buffer_height,
   1.0f / 4.0f,
         for (i = 0; i < 2; ++i) {
      ureg_MAD(shader, ureg_writemask(o_vmv[i], TGSI_WRITEMASK_XY), mv_scale, vmv[i], ureg_src(t_vpos));
                                    }
      static void *
   create_ref_frag_shader(struct vl_mc *r)
   {
      const float y_scale =
      r->buffer_height / 2 *
         struct ureg_program *shader;
   struct ureg_src tc[2], sampler;
   struct ureg_dst ref, field;
   struct ureg_dst fragment;
            shader = ureg_create(PIPE_SHADER_FRAGMENT);
   if (!shader)
            tc[0] = ureg_DECL_fs_input(shader, TGSI_SEMANTIC_GENERIC, VS_O_VTOP, TGSI_INTERPOLATE_LINEAR);
            sampler = ureg_DECL_sampler(shader, 0);
                              /*
   * ref = field.z ? tc[1] : tc[0]
   *
   * // Adjust tc acording to top/bottom field selection
   * if (|ref.z|) {
   *    ref.y *= y_scale
   *    ref.y = floor(ref.y)
   *    ref.y += ref.z
   *    ref.y /= y_scale
   * }
   * fragment.xyz = tex(ref, sampler[0])
   */
   ureg_CMP(shader, ureg_writemask(ref, TGSI_WRITEMASK_XYZ),
               ureg_CMP(shader, ureg_writemask(fragment, TGSI_WRITEMASK_W),
                              ureg_MUL(shader, ureg_writemask(ref, TGSI_WRITEMASK_Y),
         ureg_FLR(shader, ureg_writemask(ref, TGSI_WRITEMASK_Y), ureg_src(ref));
   ureg_ADD(shader, ureg_writemask(ref, TGSI_WRITEMASK_Y),
         ureg_MUL(shader, ureg_writemask(ref, TGSI_WRITEMASK_Y),
         ureg_fixup_label(shader, label, ureg_get_instruction_number(shader));
                              ureg_release_temporary(shader, field);
               }
      static void *
   create_ycbcr_vert_shader(struct vl_mc *r, vl_mc_ycbcr_vert_shader vs_callback, void *callback_priv)
   {
               struct ureg_src vrect, vpos;
   struct ureg_dst t_vpos, t_vtex;
            struct vertex2f scale = {
      (float)VL_BLOCK_WIDTH / r->buffer_width * VL_MACROBLOCK_WIDTH / r->macroblock_size,
                        shader = ureg_create(PIPE_SHADER_VERTEX);
   if (!shader)
            vrect = ureg_DECL_vs_input(shader, VS_I_RECT);
            t_vpos = calc_position(r, shader, ureg_imm2f(shader, scale.x, scale.y));
            o_vpos = ureg_DECL_output(shader, TGSI_SEMANTIC_POSITION, VS_O_VPOS);
            /*
   * o_vtex.xy = t_vpos
   * o_flags.z = intra * 0.5
   *
   * if(interlaced) {
   *    t_vtex.xy = vrect.y ? { 0, scale.y } : { -scale.y : 0 }
   *    t_vtex.z = vpos.y % 2
   *    t_vtex.y = t_vtex.z ? t_vtex.x : t_vtex.y
   *    o_vpos.y = t_vtex.y + t_vpos.y
   *
   *    o_flags.w = t_vtex.z ? 0 : 1
   * }
   *
                     ureg_MUL(shader, ureg_writemask(o_flags, TGSI_WRITEMASK_Z),
                  if (r->macroblock_size == VL_MACROBLOCK_HEIGHT) { //TODO
                  ureg_CMP(shader, ureg_writemask(t_vtex, TGSI_WRITEMASK_XY),
            ureg_negate(ureg_scalar(vrect, TGSI_SWIZZLE_Y)),
                              ureg_CMP(shader, ureg_writemask(t_vtex, TGSI_WRITEMASK_Y),
            ureg_negate(ureg_scalar(ureg_src(t_vtex), TGSI_SWIZZLE_Z)),
                     ureg_CMP(shader, ureg_writemask(o_flags, TGSI_WRITEMASK_W),
               ureg_fixup_label(shader, label, ureg_get_instruction_number(shader));
               ureg_release_temporary(shader, t_vtex);
                        }
      static void *
   create_ycbcr_frag_shader(struct vl_mc *r, float scale, bool invert,
         {
      struct ureg_program *shader;
   struct ureg_src flags;
   struct ureg_dst tmp;
   struct ureg_dst fragment;
            shader = ureg_create(PIPE_SHADER_FRAGMENT);
   if (!shader)
                                       /*
   * if (field == tc.w)
   *    kill();
   * else {
   *    fragment.xyz  = tex(tc, sampler) * scale + tc.z
   *    fragment.w = 1.0f
   * }
            ureg_SEQ(shader, ureg_writemask(tmp, TGSI_WRITEMASK_Y),
                              ureg_fixup_label(shader, label, ureg_get_instruction_number(shader));
                        if (scale != 1.0f)
      ureg_MAD(shader, ureg_writemask(tmp, TGSI_WRITEMASK_XYZ),
            else
      ureg_ADD(shader, ureg_writemask(tmp, TGSI_WRITEMASK_XYZ),
            ureg_MUL(shader, ureg_writemask(fragment, TGSI_WRITEMASK_XYZ), ureg_src(tmp), ureg_imm1f(shader, invert ? -1.0f : 1.0f));
         ureg_fixup_label(shader, label, ureg_get_instruction_number(shader));
                                 }
      static bool
   init_pipe_state(struct vl_mc *r)
   {
      struct pipe_sampler_state sampler;
   struct pipe_blend_state blend;
   struct pipe_rasterizer_state rs_state;
                     memset(&sampler, 0, sizeof(sampler));
   sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_BORDER;
   sampler.min_img_filter = PIPE_TEX_FILTER_LINEAR;
   sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
   sampler.mag_img_filter = PIPE_TEX_FILTER_LINEAR;
   sampler.compare_mode = PIPE_TEX_COMPARE_NONE;
   sampler.compare_func = PIPE_FUNC_ALWAYS;
   r->sampler_ref = r->pipe->create_sampler_state(r->pipe, &sampler);
   if (!r->sampler_ref)
            for (i = 0; i < VL_MC_NUM_BLENDERS; ++i) {
      memset(&blend, 0, sizeof blend);
   blend.independent_blend_enable = 0;
   blend.rt[0].blend_enable = 1;
   blend.rt[0].rgb_func = PIPE_BLEND_ADD;
   blend.rt[0].rgb_src_factor = PIPE_BLENDFACTOR_SRC_ALPHA;
   blend.rt[0].rgb_dst_factor = PIPE_BLENDFACTOR_ZERO;
   blend.rt[0].alpha_func = PIPE_BLEND_ADD;
   blend.rt[0].alpha_src_factor = PIPE_BLENDFACTOR_SRC_ALPHA;
   blend.rt[0].alpha_dst_factor = PIPE_BLENDFACTOR_ZERO;
   blend.logicop_enable = 0;
   blend.logicop_func = PIPE_LOGICOP_CLEAR;
   blend.rt[0].colormask = i;
   blend.dither = 0;
   r->blend_clear[i] = r->pipe->create_blend_state(r->pipe, &blend);
   if (!r->blend_clear[i])
            blend.rt[0].rgb_dst_factor = PIPE_BLENDFACTOR_ONE;
   blend.rt[0].alpha_dst_factor = PIPE_BLENDFACTOR_ONE;
   r->blend_add[i] = r->pipe->create_blend_state(r->pipe, &blend);
   if (!r->blend_add[i])
            blend.rt[0].rgb_func = PIPE_BLEND_REVERSE_SUBTRACT;
   blend.rt[0].alpha_dst_factor = PIPE_BLEND_REVERSE_SUBTRACT;
   r->blend_sub[i] = r->pipe->create_blend_state(r->pipe, &blend);
   if (!r->blend_sub[i])
               memset(&rs_state, 0, sizeof(rs_state));
   /*rs_state.sprite_coord_enable */
   rs_state.sprite_coord_mode = PIPE_SPRITE_COORD_UPPER_LEFT;
   rs_state.point_quad_rasterization = true;
   rs_state.point_size = VL_BLOCK_WIDTH;
   rs_state.half_pixel_center = true;
   rs_state.bottom_edge_rule = true;
   rs_state.depth_clip_near = 1;
            r->rs_state = r->pipe->create_rasterizer_state(r->pipe, &rs_state);
   if (!r->rs_state)
                  error_rs_state:
   error_blend:
      for (i = 0; i < VL_MC_NUM_BLENDERS; ++i) {
      if (r->blend_sub[i])
            if (r->blend_add[i])
            if (r->blend_clear[i])
                     error_sampler_ref:
         }
      static void
   cleanup_pipe_state(struct vl_mc *r)
   {
                        r->pipe->delete_sampler_state(r->pipe, r->sampler_ref);
   for (i = 0; i < VL_MC_NUM_BLENDERS; ++i) {
      r->pipe->delete_blend_state(r->pipe, r->blend_clear[i]);
   r->pipe->delete_blend_state(r->pipe, r->blend_add[i]);
      }
      }
      bool
   vl_mc_init(struct vl_mc *renderer, struct pipe_context *pipe,
            unsigned buffer_width, unsigned buffer_height,
   unsigned macroblock_size, float scale,
   vl_mc_ycbcr_vert_shader vs_callback,
      {
      assert(renderer);
                     renderer->pipe = pipe;
   renderer->buffer_width = buffer_width;
   renderer->buffer_height = buffer_height;
            if (!init_pipe_state(renderer))
            renderer->vs_ref = create_ref_vert_shader(renderer);
   if (!renderer->vs_ref)
            renderer->vs_ycbcr = create_ycbcr_vert_shader(renderer, vs_callback, callback_priv);
   if (!renderer->vs_ycbcr)
            renderer->fs_ref = create_ref_frag_shader(renderer);
   if (!renderer->fs_ref)
            renderer->fs_ycbcr = create_ycbcr_frag_shader(renderer, scale, false, fs_callback, callback_priv);
   if (!renderer->fs_ycbcr)
            renderer->fs_ycbcr_sub = create_ycbcr_frag_shader(renderer, scale, true, fs_callback, callback_priv);
   if (!renderer->fs_ycbcr_sub)
            return true;
      error_fs_ycbcr_sub:
            error_fs_ycbcr:
            error_fs_ref:
            error_vs_ycbcr:
            error_vs_ref:
            error_pipe_state:
         }
      void
   vl_mc_cleanup(struct vl_mc *renderer)
   {
                        renderer->pipe->delete_vs_state(renderer->pipe, renderer->vs_ref);
   renderer->pipe->delete_vs_state(renderer->pipe, renderer->vs_ycbcr);
   renderer->pipe->delete_fs_state(renderer->pipe, renderer->fs_ref);
   renderer->pipe->delete_fs_state(renderer->pipe, renderer->fs_ycbcr);
      }
      bool
   vl_mc_init_buffer(struct vl_mc *renderer, struct vl_mc_buffer *buffer)
   {
               buffer->viewport.scale[2] = 1;
   buffer->viewport.translate[0] = 0;
   buffer->viewport.translate[1] = 0;
   buffer->viewport.translate[2] = 0;
   buffer->viewport.swizzle_x = PIPE_VIEWPORT_SWIZZLE_POSITIVE_X;
   buffer->viewport.swizzle_y = PIPE_VIEWPORT_SWIZZLE_POSITIVE_Y;
   buffer->viewport.swizzle_z = PIPE_VIEWPORT_SWIZZLE_POSITIVE_Z;
            buffer->fb_state.nr_cbufs = 1;
               }
      void
   vl_mc_cleanup_buffer(struct vl_mc_buffer *buffer)
   {
         }
      void
   vl_mc_set_surface(struct vl_mc_buffer *buffer, struct pipe_surface *surface)
   {
                        buffer->viewport.scale[0] = surface->width;
            buffer->fb_state.width = surface->width;
   buffer->fb_state.height = surface->height;
      }
      static void
   prepare_pipe_4_rendering(struct vl_mc *renderer, struct vl_mc_buffer *buffer, unsigned mask)
   {
                        if (buffer->surface_cleared)
         else
            renderer->pipe->set_framebuffer_state(renderer->pipe, &buffer->fb_state);
      }
      void
   vl_mc_render_ref(struct vl_mc *renderer, struct vl_mc_buffer *buffer, struct pipe_sampler_view *ref)
   {
                        renderer->pipe->bind_vs_state(renderer->pipe, renderer->vs_ref);
            renderer->pipe->set_sampler_views(renderer->pipe, PIPE_SHADER_FRAGMENT,
         renderer->pipe->bind_sampler_states(renderer->pipe, PIPE_SHADER_FRAGMENT,
            util_draw_arrays_instanced(renderer->pipe, MESA_PRIM_QUADS, 0, 4, 0,
                     }
      void
   vl_mc_render_ycbcr(struct vl_mc *renderer, struct vl_mc_buffer *buffer, unsigned component, unsigned num_instances)
   {
                        if (num_instances == 0)
                     renderer->pipe->bind_vs_state(renderer->pipe, renderer->vs_ycbcr);
            util_draw_arrays_instanced(renderer->pipe, MESA_PRIM_QUADS, 0, 4, 0, num_instances);
      if (buffer->surface_cleared) {
      renderer->pipe->bind_blend_state(renderer->pipe, renderer->blend_sub[mask]);
   renderer->pipe->bind_fs_state(renderer->pipe, renderer->fs_ycbcr_sub);
         }
