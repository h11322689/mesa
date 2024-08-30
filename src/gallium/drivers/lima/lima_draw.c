   /*
   * Copyright (c) 2011-2013 Luc Verhaegen <libv@skynet.be>
   * Copyright (c) 2017-2019 Lima Project
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sub license,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   *
   */
      #include "util/format/u_format.h"
   #include "util/u_debug.h"
   #include "util/u_draw.h"
   #include "util/half_float.h"
   #include "util/u_helpers.h"
   #include "util/u_inlines.h"
   #include "util/u_pack_color.h"
   #include "util/u_split_draw.h"
   #include "util/u_upload_mgr.h"
   #include "util/u_prim.h"
   #include "util/u_vbuf.h"
   #include "util/hash_table.h"
      #include "lima_context.h"
   #include "lima_screen.h"
   #include "lima_resource.h"
   #include "lima_program.h"
   #include "lima_bo.h"
   #include "lima_job.h"
   #include "lima_texture.h"
   #include "lima_util.h"
   #include "lima_gpu.h"
      #include "pan_minmax_cache.h"
      #include <drm-uapi/lima_drm.h>
      static void
   lima_clip_scissor_to_viewport(struct lima_context *ctx)
   {
      struct lima_context_framebuffer *fb = &ctx->framebuffer;
   struct pipe_scissor_state *cscissor = &ctx->clipped_scissor;
            if (ctx->rasterizer && ctx->rasterizer->base.scissor) {
      struct pipe_scissor_state *scissor = &ctx->scissor;
   cscissor->minx = scissor->minx;
   cscissor->maxx = scissor->maxx;
   cscissor->miny = scissor->miny;
      } else {
      cscissor->minx = 0;
   cscissor->maxx = fb->base.width;
   cscissor->miny = 0;
               viewport_left = MAX2(ctx->viewport.left, 0);
   cscissor->minx = MAX2(cscissor->minx, viewport_left);
   viewport_right = MIN2(MAX2(ctx->viewport.right, 0), fb->base.width);
   cscissor->maxx = MIN2(cscissor->maxx, viewport_right);
   if (cscissor->minx > cscissor->maxx)
            viewport_bottom = MAX2(ctx->viewport.bottom, 0);
   cscissor->miny = MAX2(cscissor->miny, viewport_bottom);
   viewport_top = MIN2(MAX2(ctx->viewport.top, 0), fb->base.height);
   cscissor->maxy = MIN2(cscissor->maxy, viewport_top);
   if (cscissor->miny > cscissor->maxy)
      }
      static void
   lima_extend_viewport(struct lima_context *ctx, const struct pipe_draw_info *info)
   {
      /* restore the original values */
   ctx->ext_viewport.left = ctx->viewport.left;
   ctx->ext_viewport.right = ctx->viewport.right;
   ctx->ext_viewport.bottom = ctx->viewport.bottom;
            if (info->mode != MESA_PRIM_LINES)
            if (!ctx->rasterizer)
                     if (line_width == 1.0f)
            ctx->ext_viewport.left = ctx->viewport.left - line_width / 2;
   ctx->ext_viewport.right = ctx->viewport.right + line_width / 2;
   ctx->ext_viewport.bottom = ctx->viewport.bottom - line_width / 2;
      }
      static bool
   lima_is_scissor_zero(struct lima_context *ctx)
   {
                  }
      static void
   lima_update_job_wb(struct lima_context *ctx, unsigned buffers)
   {
      struct lima_job *job = lima_job_get(ctx);
            /* add to job when the buffer is dirty and resolve is clear (not added before) */
   if (fb->base.nr_cbufs && (buffers & PIPE_CLEAR_COLOR0) &&
      !(job->resolve & PIPE_CLEAR_COLOR0)) {
   struct lima_resource *res = lima_resource(fb->base.cbufs[0]->texture);
   lima_flush_job_accessing_bo(ctx, res->bo, true);
   _mesa_hash_table_insert(ctx->write_jobs, &res->base, job);
               /* add to job when the buffer is dirty and resolve is clear (not added before) */
   if (fb->base.zsbuf && (buffers & (PIPE_CLEAR_DEPTH | PIPE_CLEAR_STENCIL)) &&
      !(job->resolve & (PIPE_CLEAR_DEPTH | PIPE_CLEAR_STENCIL))) {
   struct lima_resource *res = lima_resource(fb->base.zsbuf->texture);
   lima_flush_job_accessing_bo(ctx, res->bo, true);
   _mesa_hash_table_insert(ctx->write_jobs, &res->base, job);
                  }
      static void
   lima_clear(struct pipe_context *pctx, unsigned buffers, const struct pipe_scissor_state *scissor_state,
         {
      struct lima_context *ctx = lima_context(pctx);
            /* flush if this job already contains any draw, otherwise multi clear can be
   * combined into a single job */
   if (lima_job_has_draw_pending(job)) {
      lima_do_job(job);
                        /* no need to reload if cleared */
   if (ctx->framebuffer.base.nr_cbufs && (buffers & PIPE_CLEAR_COLOR0)) {
      struct lima_surface *surf = lima_surface(ctx->framebuffer.base.cbufs[0]);
               struct lima_job_clear *clear = &job->clear;
            if (buffers & PIPE_CLEAR_COLOR0) {
      clear->color_8pc =
      ((uint32_t)float_to_ubyte(color->f[3]) << 24) |
   ((uint32_t)float_to_ubyte(color->f[2]) << 16) |
               clear->color_16pc =
      ((uint64_t)float_to_ushort(color->f[3]) << 48) |
   ((uint64_t)float_to_ushort(color->f[2]) << 32) |
   ((uint64_t)float_to_ushort(color->f[1]) << 16) |
                     if (buffers & PIPE_CLEAR_DEPTH) {
      clear->depth = util_pack_z(PIPE_FORMAT_Z24X8_UNORM, depth);
   if (zsbuf)
               if (buffers & PIPE_CLEAR_STENCIL) {
      clear->stencil = stencil;
   if (zsbuf)
                        lima_damage_rect_union(&job->damage_rect,
            }
      enum lima_attrib_type {
      LIMA_ATTRIB_FLOAT = 0x000,
   LIMA_ATTRIB_I32   = 0x001,
   LIMA_ATTRIB_U32   = 0x002,
   LIMA_ATTRIB_FP16  = 0x003,
   LIMA_ATTRIB_I16   = 0x004,
   LIMA_ATTRIB_U16   = 0x005,
   LIMA_ATTRIB_I8    = 0x006,
   LIMA_ATTRIB_U8    = 0x007,
   LIMA_ATTRIB_I8N   = 0x008,
   LIMA_ATTRIB_U8N   = 0x009,
   LIMA_ATTRIB_I16N  = 0x00A,
   LIMA_ATTRIB_U16N  = 0x00B,
   LIMA_ATTRIB_I32N  = 0x00D,
   LIMA_ATTRIB_U32N  = 0x00E,
      };
      static enum lima_attrib_type
   lima_pipe_format_to_attrib_type(enum pipe_format format)
   {
      const struct util_format_description *desc = util_format_description(format);
   int i = util_format_get_first_non_void_channel(format);
            switch (c->type) {
   case UTIL_FORMAT_TYPE_FLOAT:
      if (c->size == 16)
         else
      case UTIL_FORMAT_TYPE_FIXED:
         case UTIL_FORMAT_TYPE_SIGNED:
      if (c->size == 8) {
      if (c->normalized)
         else
      }
   else if (c->size == 16) {
      if (c->normalized)
         else
      }
   else if (c->size == 32) {
      if (c->normalized)
         else
      }
      case UTIL_FORMAT_TYPE_UNSIGNED:
      if (c->size == 8) {
      if (c->normalized)
         else
      }
   else if (c->size == 16) {
      if (c->normalized)
         else
      }
   else if (c->size == 32) {
      if (c->normalized)
         else
      }
                  }
      static void
   lima_pack_vs_cmd(struct lima_context *ctx, const struct pipe_draw_info *info,
         {
      struct lima_context_constant_buffer *ccb =
         struct lima_vs_compiled_shader *vs = ctx->vs;
                     if (!info->index_size) {
      VS_CMD_ARRAYS_SEMAPHORE_BEGIN_1();
      }
            int size = uniform_size + vs->state.constant_size + 32;
   VS_CMD_UNIFORMS_ADDRESS(
      lima_ctx_buff_va(ctx, lima_ctx_buff_gp_uniform),
         VS_CMD_SHADER_ADDRESS(ctx->vs->bo->va, ctx->vs->state.shader_size);
            int num_outputs = ctx->vs->state.num_outputs;
   int num_attributes = ctx->vertex_elements->num_elements;
                     VS_CMD_ATTRIBUTES_ADDRESS(
      lima_ctx_buff_va(ctx, lima_ctx_buff_gp_attribute_info),
         VS_CMD_VARYINGS_ADDRESS(
      lima_ctx_buff_va(ctx, lima_ctx_buff_gp_varying_info),
         unsigned num = info->index_size ? (ctx->max_index - ctx->min_index + 1) : draw->count;
                                 }
      static void
   lima_pack_plbu_cmd(struct lima_context *ctx, const struct pipe_draw_info *info,
         {
      struct lima_vs_compiled_shader *vs = ctx->vs;
   struct pipe_scissor_state *cscissor = &ctx->clipped_scissor;
   struct lima_job *job = lima_job_get(ctx);
            PLBU_CMD_VIEWPORT_LEFT(fui(ctx->ext_viewport.left));
   PLBU_CMD_VIEWPORT_RIGHT(fui(ctx->ext_viewport.right));
   PLBU_CMD_VIEWPORT_BOTTOM(fui(ctx->ext_viewport.bottom));
            if (!info->index_size)
            int cf = ctx->rasterizer->base.cull_face;
   int ccw = ctx->rasterizer->base.front_ccw;
   uint32_t cull = 0;
            if (cf != PIPE_FACE_NONE) {
      if (cf & PIPE_FACE_FRONT)
         if (cf & PIPE_FACE_BACK)
               /* Specify point size with PLBU command if shader doesn't write */
   if (info->mode == MESA_PRIM_POINTS && ctx->vs->state.point_size_idx == -1)
            /* Specify line width with PLBU command for lines */
   if (info->mode > MESA_PRIM_POINTS && info->mode < MESA_PRIM_TRIANGLES)
                     PLBU_CMD_RSW_VERTEX_ARRAY(
      lima_ctx_buff_va(ctx, lima_ctx_buff_pp_plb_rsw),
         /* TODO
   * - we should set it only for the first draw that enabled the scissor and for
   *   latter draw only if scissor is dirty
            assert(cscissor->minx < cscissor->maxx && cscissor->miny < cscissor->maxy);
            lima_damage_rect_union(&job->damage_rect, cscissor->minx, cscissor->maxx,
                     PLBU_CMD_DEPTH_RANGE_NEAR(fui(ctx->viewport.near));
            if ((info->mode == MESA_PRIM_POINTS && ctx->vs->state.point_size_idx == -1) ||
         {
      uint32_t v = info->mode == MESA_PRIM_POINTS ?
                     if (info->index_size) {
      PLBU_CMD_INDEXED_DEST(ctx->gp_output->va);
   if (vs->state.point_size_idx != -1)
               }
   else {
      /* can this make the attribute info static? */
                        if (info->index_size)
               }
      static int
   lima_blend_func(enum pipe_blend_func pipe)
   {
      switch (pipe) {
   case PIPE_BLEND_ADD:
         case PIPE_BLEND_SUBTRACT:
         case PIPE_BLEND_REVERSE_SUBTRACT:
         case PIPE_BLEND_MIN:
         case PIPE_BLEND_MAX:
         }
      }
      static int
   lima_blend_factor(enum pipe_blendfactor pipe)
   {
      /* Bits 0-2 indicate the blendfactor type,
   * Bit 3 is set if blendfactor is inverted
   * Bit 4 is set if blendfactor has alpha */
   switch (pipe) {
   case PIPE_BLENDFACTOR_SRC_COLOR:
         case PIPE_BLENDFACTOR_SRC_ALPHA:
         case PIPE_BLENDFACTOR_INV_SRC_COLOR:
         case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
            case PIPE_BLENDFACTOR_DST_COLOR:
         case PIPE_BLENDFACTOR_DST_ALPHA:
         case PIPE_BLENDFACTOR_INV_DST_COLOR:
         case PIPE_BLENDFACTOR_INV_DST_ALPHA:
            case PIPE_BLENDFACTOR_CONST_COLOR:
         case PIPE_BLENDFACTOR_CONST_ALPHA:
         case PIPE_BLENDFACTOR_INV_CONST_COLOR:
         case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
            case PIPE_BLENDFACTOR_ZERO:
         case PIPE_BLENDFACTOR_ONE:
            case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
            case PIPE_BLENDFACTOR_SRC1_COLOR:
         case PIPE_BLENDFACTOR_SRC1_ALPHA:
         case PIPE_BLENDFACTOR_INV_SRC1_COLOR:
         case PIPE_BLENDFACTOR_INV_SRC1_ALPHA:
         }
      }
      static int
   lima_calculate_alpha_blend(enum pipe_blend_func rgb_func, enum pipe_blend_func alpha_func,
               {
      /* PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE has to be changed to PIPE_BLENDFACTOR_ONE
   * if it is set for alpha_src or alpha_dst.
   */
   if (alpha_src_factor == PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE)
            if (alpha_dst_factor == PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE)
            /* MIN and MAX ops actually do OP(As * S + Ad * D, Ad), so
   * we need to set S to 1 and D to 0 to get correct result */
   if (alpha_func == PIPE_BLEND_MIN ||
      alpha_func == PIPE_BLEND_MAX) {
   alpha_src_factor = PIPE_BLENDFACTOR_ONE;
               /* MIN and MAX ops actually do OP(Cs * S + Cd * D, Cd), so
   * we need to set S to 1 and D to 0 to get correct result */
   if (rgb_func == PIPE_BLEND_MIN ||
      rgb_func == PIPE_BLEND_MAX) {
   rgb_src_factor = PIPE_BLENDFACTOR_ONE;
               return lima_blend_func(rgb_func) |
      (lima_blend_func(alpha_func) << 3) |
   (lima_blend_factor(rgb_src_factor) << 6) |
   (lima_blend_factor(rgb_dst_factor) << 11) |
   /* alpha_src and alpha_dst are 4 bit, so need to mask 5th bit */
   ((lima_blend_factor(alpha_src_factor) & 0xf) << 16) |
   ((lima_blend_factor(alpha_dst_factor) & 0xf) << 20) |
   }
      static int
   lima_stencil_op(enum pipe_stencil_op pipe)
   {
      switch (pipe) {
   case PIPE_STENCIL_OP_KEEP:
         case PIPE_STENCIL_OP_ZERO:
         case PIPE_STENCIL_OP_REPLACE:
         case PIPE_STENCIL_OP_INCR:
         case PIPE_STENCIL_OP_DECR:
         case PIPE_STENCIL_OP_INCR_WRAP:
         case PIPE_STENCIL_OP_DECR_WRAP:
         case PIPE_STENCIL_OP_INVERT:
         }
      }
      static unsigned
   lima_calculate_depth_test(struct pipe_depth_stencil_alpha_state *depth,
         {
      int offset_scale = 0, offset_units = 0;
            offset_scale = CLAMP(rst->offset_scale * 4, -128, 127);
   if (offset_scale < 0)
            offset_units = CLAMP(rst->offset_units * 2, -128, 127);
   if (offset_units < 0)
            return (depth->depth_enabled && depth->depth_writemask) |
      ((int)func << 1) |
   (offset_scale << 16) |
   }
      static void
   lima_pack_render_state(struct lima_context *ctx, const struct pipe_draw_info *info)
   {
      struct lima_fs_compiled_shader *fs = ctx->fs;
   struct lima_render_state *render =
      lima_ctx_buff_alloc(ctx, lima_ctx_buff_pp_plb_rsw,
      bool early_z = true;
            /* do hw support RGBA independ blend?
   * PIPE_CAP_INDEP_BLEND_ENABLE
   *
   * how to handle the no cbuf only zbuf case?
   */
   struct pipe_rt_blend_state *rt = ctx->blend->base.rt;
   render->blend_color_bg = float_to_ubyte(ctx->blend_color.color[2]) |
         render->blend_color_ra = float_to_ubyte(ctx->blend_color.color[0]) |
            if (rt->blend_enable) {
      render->alpha_blend = lima_calculate_alpha_blend(rt->rgb_func, rt->alpha_func,
      rt->rgb_src_factor, rt->rgb_dst_factor,
   }
   else {
      /*
   * Special handling for blending disabled.
   * Binary driver is generating the same alpha_value,
   * as when we would just enable blending, without changing/setting any blend equation/params.
   * Normaly in this case mesa would set all rt fields (func/factor) to zero.
   */
   render->alpha_blend = lima_calculate_alpha_blend(PIPE_BLEND_ADD, PIPE_BLEND_ADD,
      PIPE_BLENDFACTOR_ONE, PIPE_BLENDFACTOR_ZERO,
                     struct pipe_rasterizer_state *rst = &ctx->rasterizer->base;
            if (!rst->depth_clip_near || ctx->viewport.near == 0.0f)
         if (!rst->depth_clip_far || ctx->viewport.far == 1.0f)
            if (fs->state.frag_depth_reg != -1) {
      render->depth_test |= (fs->state.frag_depth_reg << 6);
   /* Shader writes depth */
                        near = float_to_ushort(ctx->viewport.near);
            /* overlap with plbu? any place can remove one? */
            struct pipe_stencil_state *stencil = ctx->zsa->base.stencil;
            if (stencil[0].enabled) { /* stencil is enabled */
      render->stencil_front = stencil[0].func |
      (lima_stencil_op(stencil[0].fail_op) << 3) |
   (lima_stencil_op(stencil[0].zfail_op) << 6) |
   (lima_stencil_op(stencil[0].zpass_op) << 9) |
   (ref->ref_value[0] << 16) |
      render->stencil_back = render->stencil_front;
   render->stencil_test = (stencil[0].writemask & 0xff) | (stencil[0].writemask & 0xff) << 8;
   if (stencil[1].enabled) { /* two-side is enabled */
      render->stencil_back = stencil[1].func |
      (lima_stencil_op(stencil[1].fail_op) << 3) |
   (lima_stencil_op(stencil[1].zfail_op) << 6) |
   (lima_stencil_op(stencil[1].zpass_op) << 9) |
   (ref->ref_value[1] << 16) |
         }
      }
   else {
      /* Default values, when stencil is disabled:
   * stencil[0|1].valuemask = 0xff
   * stencil[0|1].func = PIPE_FUNC_ALWAYS
   * stencil[0|1].writemask = 0xff
   */
   render->stencil_front = 0xff000007;
   render->stencil_back = 0xff000007;
               /* need more investigation */
   if (info->mode == MESA_PRIM_POINTS)
         else if (info->mode < MESA_PRIM_TRIANGLES)
         else
         if (ctx->framebuffer.base.samples)
         if (ctx->blend->base.alpha_to_coverage)
         if (ctx->blend->base.alpha_to_one)
                  /* Set gl_FragColor register, need to specify it 4 times */
   render->multi_sample |= (fs->state.frag_color0_reg << 28) |
                        /* alpha test */
   if (ctx->zsa->base.alpha_enabled) {
      render->multi_sample |= ctx->zsa->base.alpha_func;
      } else {
      /* func = PIPE_FUNC_ALWAYS */
               render->shader_address =
            /* seems not needed */
                     render->aux0 = (ctx->vs->state.varying_stride >> 3);
   render->aux1 = 0x00000000;
   if (ctx->rasterizer->base.front_ccw)
            if (ctx->blend->base.dither)
            if (fs->state.uses_discard ||
      ctx->zsa->base.alpha_enabled ||
   fs->state.frag_depth_reg != -1 ||
   ctx->blend->base.alpha_to_coverage) {
   early_z = false;
               if (rt->blend_enable)
            if ((rt->colormask & PIPE_MASK_RGBA) != PIPE_MASK_RGBA)
            if (early_z)
            if (pixel_kill)
            if (ctx->tex_stateobj.num_samplers) {
      render->textures_address =
         render->aux0 |= ctx->tex_stateobj.num_samplers << 14;
               if (ctx->const_buffer[PIPE_SHADER_FRAGMENT].buffer) {
      render->uniforms_address =
         uint32_t size = ctx->buffer_state[lima_ctx_buff_pp_uniform].size;
   uint32_t bits = 0;
   if (size >= 8) {
      bits = util_last_bit(size >> 3) - 1;
      }
            render->aux0 |= 0x80;
               /* Set secondary output color */
   if (fs->state.frag_color1_reg != -1)
            if (ctx->vs->state.num_varyings) {
      render->varying_types = 0x00000000;
   render->varyings_address = ctx->gp_output->va +
         for (int i = 0, index = 0; i < ctx->vs->state.num_outputs; i++) {
               if (i == ctx->vs->state.gl_pos_idx ||
                  struct lima_varying_info *v = ctx->vs->state.varying + i;
   if (v->component_size == 4)
                        if (index < 10)
         else if (index == 10) {
      render->varying_types |= val << 30;
      }
                        }
   else {
      render->varying_types = 0x00000000;
                        lima_dump_command_stream_print(
      job->dump, render, sizeof(*render),
   false, "add render state at va %x\n",
         lima_dump_rsw_command_stream_print(
      job->dump, render, sizeof(*render),
   }
      static void
   lima_update_gp_attribute_info(struct lima_context *ctx, const struct pipe_draw_info *info,
         {
      struct lima_job *job = lima_job_get(ctx);
   struct lima_vertex_element_state *ve = ctx->vertex_elements;
            uint32_t *attribute =
      lima_ctx_buff_alloc(ctx, lima_ctx_buff_gp_attribute_info,
         int n = 0;
   for (int i = 0; i < ve->num_elements; i++) {
               assert(pve->vertex_buffer_index < vb->count);
            struct pipe_vertex_buffer *pvb = vb->vb + pve->vertex_buffer_index;
                     unsigned start = info->index_size ? (ctx->min_index + draw->index_bias) : draw->start;
   attribute[n++] = res->bo->va + pvb->buffer_offset + pve->src_offset
         attribute[n++] = (pve->src_stride << 11) |
      (lima_pipe_format_to_attrib_type(pve->src_format) << 2) |
            lima_dump_command_stream_print(
      job->dump, attribute, n * 4, false, "update attribute info at va %x\n",
   }
      static void
   lima_update_gp_uniform(struct lima_context *ctx)
   {
      struct lima_context_constant_buffer *ccb =
         struct lima_vs_compiled_shader *vs = ctx->vs;
            int size = uniform_size + vs->state.constant_size + 32;
   void *vs_const_buff =
            if (ccb->buffer)
            memcpy(vs_const_buff + uniform_size,
         ctx->viewport.transform.scale,
   memcpy(vs_const_buff + uniform_size + 16,
                  if (vs->constant)
      memcpy(vs_const_buff + uniform_size + 32,
                  if (lima_debug & LIMA_DEBUG_GP) {
      float *vs_const_buff_f = vs_const_buff;
   printf("gp uniforms:\n");
   for (int i = 0; i < (size / sizeof(float)); i++) {
      if ((i % 4) == 0)
         printf(" %8.4f", vs_const_buff_f[i]);
   if ((i % 4) == 3)
      }
               lima_dump_command_stream_print(
      job->dump, vs_const_buff, size, true,
   "update gp uniform at va %x\n",
   }
      static void
   lima_update_pp_uniform(struct lima_context *ctx)
   {
      const float *const_buff = ctx->const_buffer[PIPE_SHADER_FRAGMENT].buffer;
            if (!const_buff)
            uint16_t *fp16_const_buff =
      lima_ctx_buff_alloc(ctx, lima_ctx_buff_pp_uniform,
         uint32_t *array =
            for (int i = 0; i < const_buff_size; i++)
                              lima_dump_command_stream_print(
      job->dump, fp16_const_buff, const_buff_size * 2,
   false, "add pp uniform data at va %x\n",
      lima_dump_command_stream_print(
      job->dump, array, 4, false, "add pp uniform info at va %x\n",
   }
      static void
   lima_update_varying(struct lima_context *ctx, const struct pipe_draw_info *info,
         {
      struct lima_job *job = lima_job_get(ctx);
   struct lima_screen *screen = lima_screen(ctx->base.screen);
   struct lima_vs_compiled_shader *vs = ctx->vs;
   uint32_t gp_output_size;
            uint32_t *varying =
      lima_ctx_buff_alloc(ctx, lima_ctx_buff_gp_varying_info,
                        for (int i = 0; i < vs->state.num_outputs; i++) {
               if (i == vs->state.gl_pos_idx ||
                           /* does component_size == 2 need to be 16 aligned? */
   if (v->component_size == 4)
            v->offset = offset;
                        /* gl_Position is always present, allocate space for it */
            /* Allocate space for varyings if there're any */
   if (vs->state.num_varyings) {
      ctx->gp_output_varyings_offt = gp_output_size;
               /* Allocate space for gl_PointSize if it's there */
   if (vs->state.point_size_idx != -1) {
      ctx->gp_output_point_size_offt = gp_output_size;
               /* gp_output can be too large for the suballocator, so create a
   * separate bo for it. The bo cache should prevent performance hit.
   */
   ctx->gp_output = lima_bo_create(screen, gp_output_size, 0);
   assert(ctx->gp_output);
   lima_job_add_bo(job, LIMA_PIPE_GP, ctx->gp_output, LIMA_SUBMIT_BO_WRITE);
            for (int i = 0; i < vs->state.num_outputs; i++) {
               if (i == vs->state.gl_pos_idx) {
      /* gl_Position */
   varying[n++] = ctx->gp_output->va;
      } else if (i == vs->state.point_size_idx) {
      /* gl_PointSize */
   varying[n++] = ctx->gp_output->va + ctx->gp_output_point_size_offt;
      } else {
      /* Varying */
   varying[n++] = ctx->gp_output->va + ctx->gp_output_varyings_offt +
         varying[n++] = (vs->state.varying_stride << 11) | (v->components - 1) |
                  lima_dump_command_stream_print(
      job->dump, varying, n * 4, false, "update varying info at va %x\n",
   }
      static void
   lima_draw_vbo_update(struct pipe_context *pctx,
               {
      struct lima_context *ctx = lima_context(pctx);
   struct lima_context_framebuffer *fb = &ctx->framebuffer;
            if (fb->base.zsbuf) {
      if (ctx->zsa->base.depth_enabled)
         if (ctx->zsa->base.stencil[0].enabled ||
      ctx->zsa->base.stencil[1].enabled)
            if (fb->base.nr_cbufs)
                              if ((ctx->dirty & LIMA_CONTEXT_DIRTY_CONST_BUFF &&
      ctx->const_buffer[PIPE_SHADER_VERTEX].dirty) ||
   ctx->dirty & LIMA_CONTEXT_DIRTY_VIEWPORT ||
   ctx->dirty & LIMA_CONTEXT_DIRTY_COMPILED_VS) {
   lima_update_gp_uniform(ctx);
                                 if (ctx->dirty & LIMA_CONTEXT_DIRTY_CONST_BUFF &&
      ctx->const_buffer[PIPE_SHADER_FRAGMENT].dirty) {
   lima_update_pp_uniform(ctx);
                        lima_pack_render_state(ctx, info);
            if (ctx->gp_output) {
      lima_bo_unreference(ctx->gp_output); /* held by job */
                  }
      static void
   lima_draw_vbo_indexed(struct pipe_context *pctx,
               {
      struct lima_context *ctx = lima_context(pctx);
   struct lima_job *job = lima_job_get(ctx);
   struct pipe_resource *indexbuf = NULL;
            /* Mali Utgard GPU always need min/max index info for index draw,
   * compute it if upper layer does not do for us */
   if (info->index_bounds_valid) {
      ctx->min_index = info->min_index;
   ctx->max_index = info->max_index;
               if (info->has_user_indices) {
      util_upload_index_buffer(&ctx->base, info, draw, &indexbuf, &ctx->index_offset, 0x40);
      }
   else {
      ctx->index_res = lima_resource(info->index.resource);
   ctx->index_offset = 0;
   needs_indices = !panfrost_minmax_cache_get(ctx->index_res->index_cache, draw->start,
               if (needs_indices) {
      u_vbuf_get_minmax_index(pctx, info, draw, &ctx->min_index, &ctx->max_index);
   if (!info->has_user_indices)
      panfrost_minmax_cache_add(ctx->index_res->index_cache, draw->start, draw->count,
            lima_job_add_bo(job, LIMA_PIPE_GP, ctx->index_res->bo, LIMA_SUBMIT_BO_READ);
   lima_job_add_bo(job, LIMA_PIPE_PP, ctx->index_res->bo, LIMA_SUBMIT_BO_READ);
            if (indexbuf)
      }
      static void
   lima_draw_vbo_count(struct pipe_context *pctx,
               {
               struct pipe_draw_start_count_bias local_draw = *draw;
   unsigned start = draw->start;
            while (count) {
      unsigned this_count = count;
                     local_draw.start = start;
                     count -= step;
         }
      static void
   lima_draw_vbo(struct pipe_context *pctx,
               const struct pipe_draw_info *info,
   unsigned drawid_offset,
   const struct pipe_draw_indirect_info *indirect,
   {
      if (num_draws > 1) {
      util_draw_multi(pctx, info, drawid_offset, indirect, draws, num_draws);
               /* check if draw mode and vertex/index count match,
   * otherwise gp will hang */
   if (!u_trim_pipe_prim(info->mode, (unsigned*)&draws[0].count)) {
      debug_printf("draw mode and vertex/index count mismatch\n");
                        if (!ctx->uncomp_fs || !ctx->uncomp_vs) {
      debug_warn_once("no shader, skip draw\n");
               lima_clip_scissor_to_viewport(ctx);
   if (lima_is_scissor_zero(ctx))
            /* extend the viewport in case of line draws with a line_width > 1.0f,
   * otherwise use the original values */
            if (!lima_update_fs_state(ctx) || !lima_update_vs_state(ctx))
            struct lima_job *job = lima_job_get(ctx);
            lima_dump_command_stream_print(
      job->dump, ctx->vs->bo->map, ctx->vs->state.shader_size, false,
               lima_dump_command_stream_print(
      job->dump, ctx->fs->bo->map, ctx->fs->state.shader_size, false,
               lima_job_add_bo(job, LIMA_PIPE_GP, ctx->vs->bo, LIMA_SUBMIT_BO_READ);
            if (info->index_size)
         else
            job->draws++;
   /* Flush job if we hit the limit of draws per job otherwise we may
   * hit tile heap size limit */
   if (job->draws > MAX_DRAWS_PER_JOB) {
      unsigned resolve = job->resolve;
   lima_do_job(job);
   /* Subsequent job will need to resolve the same buffers */
         }
      void
   lima_draw_init(struct lima_context *ctx)
   {
      ctx->base.clear = lima_clear;
      }
