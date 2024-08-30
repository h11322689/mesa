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
      /* Authors:
   *  Brian Paul
   */
      #include "util/u_inlines.h"
   #include "util/u_memory.h"
      #include "draw/draw_context.h"
      #include "lp_context.h"
   #include "lp_screen.h"
   #include "lp_state.h"
   #include "lp_debug.h"
   #include "frontend/sw_winsys.h"
   #include "lp_flush.h"
         static void *
   llvmpipe_create_sampler_state(struct pipe_context *pipe,
         {
               if (LP_PERF & PERF_NO_MIP_LINEAR) {
      if (state->min_mip_filter == PIPE_TEX_MIPFILTER_LINEAR)
               if (LP_PERF & PERF_NO_MIPMAPS)
            if (LP_PERF & PERF_NO_LINEAR) {
      state->mag_img_filter = PIPE_TEX_FILTER_NEAREST;
                  }
         static void
   llvmpipe_bind_sampler_states(struct pipe_context *pipe,
                           {
               assert(shader < PIPE_SHADER_MESH_TYPES);
                     /* set the new samplers */
   for (unsigned i = 0; i < num; i++) {
               if (samplers && samplers[i])
                     /* find highest non-null samplers[] entry */
   {
      unsigned j = MAX2(llvmpipe->num_samplers[shader], start + num);
   while (j > 0 && llvmpipe->samplers[shader][j - 1] == NULL)
                     switch (shader) {
   case PIPE_SHADER_VERTEX:
   case PIPE_SHADER_GEOMETRY:
   case PIPE_SHADER_TESS_CTRL:
   case PIPE_SHADER_TESS_EVAL:
      draw_set_samplers(llvmpipe->draw,
                        case PIPE_SHADER_COMPUTE:
      llvmpipe->cs_dirty |= LP_CSNEW_SAMPLER;
      case PIPE_SHADER_FRAGMENT:
      llvmpipe->dirty |= LP_NEW_SAMPLER;
      case PIPE_SHADER_TASK:
      llvmpipe->dirty |= LP_NEW_TASK_SAMPLER;
      case PIPE_SHADER_MESH:
      llvmpipe->dirty |= LP_NEW_MESH_SAMPLER;
      default:
      unreachable("Illegal shader type");
         }
         static void
   llvmpipe_set_sampler_views(struct pipe_context *pipe,
                              enum pipe_shader_type shader,
      {
      struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
                     assert(shader < PIPE_SHADER_MESH_TYPES);
                     /* set the new sampler views */
   for (i = 0; i < num; i++) {
               if (views && views[i])
            /*
   * Warn if someone tries to set a view created in a different context
   * (which is why we need the hack above in the first place).
   * An assert would be better but st/mesa relies on it...
   */
   if (view && view->context != pipe) {
      debug_printf("Illegal setting of sampler_view %d created in another "
               if (view)
            if (take_ownership) {
      pipe_sampler_view_reference(&llvmpipe->sampler_views[shader][start + i],
            } else {
      pipe_sampler_view_reference(&llvmpipe->sampler_views[shader][start + i],
                  for (; i < num + unbind_num_trailing_slots; i++) {
      pipe_sampler_view_reference(&llvmpipe->sampler_views[shader][start + i],
               /* find highest non-null sampler_views[] entry */
   {
      unsigned j = MAX2(llvmpipe->num_sampler_views[shader], start + num);
   while (j > 0 && llvmpipe->sampler_views[shader][j - 1] == NULL)
                     switch (shader) {
   case PIPE_SHADER_VERTEX:
   case PIPE_SHADER_GEOMETRY:
   case PIPE_SHADER_TESS_CTRL:
   case PIPE_SHADER_TESS_EVAL:
      draw_set_sampler_views(llvmpipe->draw,
                        case PIPE_SHADER_COMPUTE:
      llvmpipe->cs_dirty |= LP_CSNEW_SAMPLER_VIEW;
      case PIPE_SHADER_FRAGMENT:
      llvmpipe->dirty |= LP_NEW_SAMPLER_VIEW;
   lp_setup_set_fragment_sampler_views(llvmpipe->setup,
                  case PIPE_SHADER_TASK:
      llvmpipe->dirty |= LP_NEW_TASK_SAMPLER_VIEW;
      case PIPE_SHADER_MESH:
      llvmpipe->dirty |= LP_NEW_MESH_SAMPLER_VIEW;
      default:
      unreachable("Illegal shader type");
         }
         static struct pipe_sampler_view *
   llvmpipe_create_sampler_view(struct pipe_context *pipe,
               {
      struct pipe_sampler_view *view = CALLOC_STRUCT(pipe_sampler_view);
   /*
   * XXX: bind flags from OpenGL state tracker are notoriously unreliable.
   * This looks unfixable, so fix the bind flags instead when it happens.
   */
   if (!(texture->bind & PIPE_BIND_SAMPLER_VIEW)) {
      debug_printf("Illegal sampler view creation without bind flag\n");
               if (view) {
      *view = *templ;
   view->reference.count = 1;
   view->texture = NULL;
   pipe_resource_reference(&view->texture, texture);
      #ifdef DEBUG
      /*
      * This is possibly too lenient, but the primary reason is just
   * to catch gallium frontends which forget to initialize this, so
   * it only catches clearly impossible view targets.
   */
   if (view->target != texture->target) {
      if (view->target == PIPE_TEXTURE_1D)
         else if (view->target == PIPE_TEXTURE_1D_ARRAY)
         else if (view->target == PIPE_TEXTURE_2D)
      assert(texture->target == PIPE_TEXTURE_2D_ARRAY ||
         texture->target == PIPE_TEXTURE_3D ||
      else if (view->target == PIPE_TEXTURE_2D_ARRAY)
      assert(texture->target == PIPE_TEXTURE_2D ||
            else if (view->target == PIPE_TEXTURE_CUBE)
      assert(texture->target == PIPE_TEXTURE_CUBE_ARRAY ||
      else if (view->target == PIPE_TEXTURE_CUBE_ARRAY)
      assert(texture->target == PIPE_TEXTURE_CUBE ||
      else
      #endif
                  }
         static void
   llvmpipe_sampler_view_destroy(struct pipe_context *pipe,
         {
      pipe_resource_reference(&view->texture, NULL);
      }
         static void
   llvmpipe_delete_sampler_state(struct pipe_context *pipe,
         {
         }
         static void
   prepare_shader_sampling(struct llvmpipe_context *lp,
                     {
      uint32_t row_stride[PIPE_MAX_TEXTURE_LEVELS];
   uint32_t img_stride[PIPE_MAX_TEXTURE_LEVELS];
   uint32_t mip_offsets[PIPE_MAX_TEXTURE_LEVELS];
            assert(num <= PIPE_MAX_SHADER_SAMPLER_VIEWS);
   if (!num)
            for (unsigned i = 0; i < num; i++) {
               if (view) {
      struct pipe_resource *tex = view->texture;
   struct llvmpipe_resource *lp_tex = llvmpipe_resource(tex);
   unsigned width0 = tex->width0;
   unsigned num_layers = tex->depth0;
   unsigned first_level = 0;
   unsigned last_level = 0;
                  if (!lp_tex->dt) {
                     if (llvmpipe_resource_is_texture(res)) {
      first_level = view->u.tex.first_level;
   last_level = view->u.tex.last_level;
                                 for (unsigned j = first_level; j <= last_level; j++) {
      mip_offsets[j] = lp_tex->mip_offsets[j];
   row_stride[j] = lp_tex->row_stride[j];
      }
   if (tex->target == PIPE_TEXTURE_1D_ARRAY ||
      tex->target == PIPE_TEXTURE_2D_ARRAY ||
   tex->target == PIPE_TEXTURE_CUBE ||
   tex->target == PIPE_TEXTURE_CUBE_ARRAY) {
   num_layers = view->u.tex.last_layer - view->u.tex.first_layer + 1;
   for (unsigned j = first_level; j <= last_level; j++) {
      mip_offsets[j] += view->u.tex.first_layer *
      }
   if (view->target == PIPE_TEXTURE_CUBE ||
      view->target == PIPE_TEXTURE_CUBE_ARRAY) {
      }
   assert(view->u.tex.first_layer <= view->u.tex.last_layer);
         } else {
      unsigned view_blocksize = util_format_get_blocksize(view->format);
   addr = lp_tex->data;
   /* probably don't really need to fill that out */
                        /* everything specified in number of elements here. */
   width0 = view->u.buf.size / view_blocksize;
   addr = (uint8_t *)addr + view->u.buf.offset;
         } else {
      /* display target texture/surface */
   addr = llvmpipe_resource_map(tex, 0, 0, LP_TEX_USAGE_READ);
   row_stride[0] = lp_tex->row_stride[0];
   img_stride[0] = lp_tex->img_stride[0];
   mip_offsets[0] = 0;
      }
   draw_set_mapped_texture(lp->draw,
                           shader_type,
   i,
            }
         /**
   * Called whenever we're about to draw (no dirty flag, FIXME?).
   */
   void
   llvmpipe_prepare_vertex_sampling(struct llvmpipe_context *lp,
               {
         }
         /**
   * Called whenever we're about to draw (no dirty flag, FIXME?).
   */
   void
   llvmpipe_prepare_geometry_sampling(struct llvmpipe_context *lp,
               {
         }
         /**
   * Called whenever we're about to draw (no dirty flag, FIXME?).
   */
   void
   llvmpipe_prepare_tess_ctrl_sampling(struct llvmpipe_context *lp,
               {
         }
         /**
   * Called whenever we're about to draw (no dirty flag, FIXME?).
   */
   void
   llvmpipe_prepare_tess_eval_sampling(struct llvmpipe_context *lp,
               {
         }
         void
   llvmpipe_cleanup_stage_sampling(struct llvmpipe_context *ctx,
         {
      assert(ctx);
   assert(stage < ARRAY_SIZE(ctx->num_sampler_views));
            unsigned num = ctx->num_sampler_views[stage];
                     for (unsigned i = 0; i < num; i++) {
      struct pipe_sampler_view *view = views[i];
   if (view) {
      struct pipe_resource *tex = view->texture;
   if (tex)
            }
         static void
   prepare_shader_images(struct llvmpipe_context *lp,
                     {
      assert(num <= PIPE_MAX_SHADER_SAMPLER_VIEWS);
   if (!num)
            for (unsigned i = 0; i < num; i++) {
               if (view) {
      struct pipe_resource *img = view->resource;
   struct llvmpipe_resource *lp_img = llvmpipe_resource(img);
                  unsigned width = img->width0;
   unsigned height = img->height0;
                                 uint32_t row_stride;
   uint32_t img_stride;
                  if (!lp_img->dt) {
                     if (llvmpipe_resource_is_texture(res)) {
                     if (img->target == PIPE_TEXTURE_1D_ARRAY ||
      img->target == PIPE_TEXTURE_2D_ARRAY ||
   img->target == PIPE_TEXTURE_3D ||
   img->target == PIPE_TEXTURE_CUBE ||
   img->target == PIPE_TEXTURE_CUBE_ARRAY) {
   num_layers = view->u.tex.last_layer -
         assert(view->u.tex.first_layer <= view->u.tex.last_layer);
                     row_stride = lp_img->row_stride[view->u.tex.level];
   img_stride = lp_img->img_stride[view->u.tex.level];
   sample_stride = lp_img->sample_stride;
      } else {
      unsigned view_blocksize =
         addr = lp_img->data;
   /* probably don't really need to fill that out */
                        /* everything specified in number of elements here. */
   width = view->u.buf.size / view_blocksize;
   addr = (uint8_t *)addr + view->u.buf.offset;
         } else {
      /* display target texture/surface */
   addr = llvmpipe_resource_map(img, 0, 0, LP_TEX_USAGE_READ);
   row_stride = lp_img->row_stride[0];
   img_stride = lp_img->img_stride[0];
   sample_stride = 0;
      }
   draw_set_mapped_image(lp->draw, shader_type, i,
                        }
         /**
   * Called whenever we're about to draw (no dirty flag, FIXME?).
   */
   void
   llvmpipe_prepare_vertex_images(struct llvmpipe_context *lp,
               {
         }
         /**
   * Called whenever we're about to draw (no dirty flag, FIXME?).
   */
   void
   llvmpipe_prepare_geometry_images(struct llvmpipe_context *lp,
               {
         }
         /**
   * Called whenever we're about to draw (no dirty flag, FIXME?).
   */
   void
   llvmpipe_prepare_tess_ctrl_images(struct llvmpipe_context *lp,
               {
         }
         /**
   * Called whenever we're about to draw (no dirty flag, FIXME?).
   */
   void
   llvmpipe_prepare_tess_eval_images(struct llvmpipe_context *lp,
               {
         }
         void
   llvmpipe_cleanup_stage_images(struct llvmpipe_context *ctx,
         {
      assert(ctx);
   assert(stage < ARRAY_SIZE(ctx->num_images));
            unsigned num = ctx->num_images[stage];
                     for (unsigned i = 0; i < num; i++) {
      struct pipe_image_view *view = &views[i];
   assert(view);
   struct pipe_resource *img = view->resource;
   if (img)
         }
         void
   llvmpipe_init_sampler_funcs(struct llvmpipe_context *llvmpipe)
   {
               llvmpipe->pipe.bind_sampler_states = llvmpipe_bind_sampler_states;
   llvmpipe->pipe.create_sampler_view = llvmpipe_create_sampler_view;
   llvmpipe->pipe.set_sampler_views = llvmpipe_set_sampler_views;
   llvmpipe->pipe.sampler_view_destroy = llvmpipe_sampler_view_destroy;
      }
