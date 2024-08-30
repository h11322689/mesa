   /**********************************************************
   * Copyright 2008-2009 VMware, Inc.  All rights reserved.
   *
   * Permission is hereby granted, free of charge, to any person
   * obtaining a copy of this software and associated documentation
   * files (the "Software"), to deal in the Software without
   * restriction, including without limitation the rights to use, copy,
   * modify, merge, publish, distribute, sublicense, and/or sell copies
   * of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be
   * included in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   **********************************************************/
      #include "svga_cmd.h"
   #include "svga_debug.h"
      #include "pipe/p_defines.h"
   #include "util/u_pack_color.h"
   #include "util/u_surface.h"
      #include "svga_context.h"
   #include "svga_state.h"
   #include "svga_surface.h"
         /**
   * Saving blitter states before doing any blitter operation
   */
   static void
   begin_blit(struct svga_context *svga)
   {
      util_blitter_save_vertex_buffer_slot(svga->blitter, svga->curr.vb);
   util_blitter_save_vertex_elements(svga->blitter, (void*)svga->curr.velems);
   util_blitter_save_vertex_shader(svga->blitter, svga->curr.vs);
   util_blitter_save_geometry_shader(svga->blitter, svga->curr.gs);
   util_blitter_save_tessctrl_shader(svga->blitter, svga->curr.tcs);
   util_blitter_save_tesseval_shader(svga->blitter, svga->curr.tes);
   util_blitter_save_so_targets(svga->blitter, svga->num_so_targets,
         util_blitter_save_rasterizer(svga->blitter, (void*)svga->curr.rast);
   util_blitter_save_viewport(svga->blitter, &svga->curr.viewport[0]);
   util_blitter_save_scissor(svga->blitter, &svga->curr.scissor[0]);
   util_blitter_save_fragment_shader(svga->blitter, svga->curr.fs);
   util_blitter_save_blend(svga->blitter, (void*)svga->curr.blend);
   util_blitter_save_depth_stencil_alpha(svga->blitter,
         util_blitter_save_stencil_ref(svga->blitter, &svga->curr.stencil_ref);
   util_blitter_save_sample_mask(svga->blitter, svga->curr.sample_mask, 0);
   util_blitter_save_fragment_constant_buffer_slot(svga->blitter,
      }
         /**
   * Clear the whole color buffer(s) by drawing a quad.  For VGPU10 we use
   * this when clearing integer render targets.  We'll also clear the
   * depth and/or stencil buffers if the clear_buffers mask specifies them.
   */
   static void
   clear_buffers_with_quad(struct svga_context *svga,
                     {
               begin_blit(svga);
   util_blitter_clear(svga->blitter,
                     fb->width, fb->height,
      }
         /**
   * Check if any of the color buffers are integer buffers.
   */
   static bool
   is_integer_target(struct pipe_framebuffer_state *fb, unsigned buffers)
   {
               for (i = 0; i < fb->nr_cbufs; i++) {
      if ((buffers & (PIPE_CLEAR_COLOR0 << i)) &&
      fb->cbufs[i] &&
   util_format_is_pure_integer(fb->cbufs[i]->format)) {
         }
      }
         /**
   * Check if the integer values in the clear color can be represented
   * by floats.  If so, we can use the VGPU10 ClearRenderTargetView command.
   * Otherwise, we need to clear with a quad.
   */
   static bool
   ints_fit_in_floats(const union pipe_color_union *color)
   {
      const int max = 1 << 24;
   return (color->i[0] <= max &&
         color->i[1] <= max &&
      }
         static enum pipe_error
   try_clear(struct svga_context *svga, 
            unsigned buffers,
   const union pipe_color_union *color,
      {
      enum pipe_error ret = PIPE_OK;
   SVGA3dRect rect = { 0, 0, 0, 0 };
   bool restore_viewport = false;
   SVGA3dClearFlag flags = 0;
   struct pipe_framebuffer_state *fb = &svga->curr.framebuffer;
            ret = svga_update_state(svga, SVGA_STATE_HW_CLEAR);
   if (ret != PIPE_OK)
            if (svga->rebind.flags.rendertargets) {
      ret = svga_reemit_framebuffer_bindings(svga);
   if (ret != PIPE_OK) {
                     if (buffers & PIPE_CLEAR_COLOR) {
      flags |= SVGA3D_CLEAR_COLOR;
            rect.w = fb->width;
               if ((buffers & PIPE_CLEAR_DEPTHSTENCIL) && fb->zsbuf) {
      if (buffers & PIPE_CLEAR_DEPTH)
            if (buffers & PIPE_CLEAR_STENCIL)
            rect.w = MAX2(rect.w, fb->zsbuf->width);
               if (!svga_have_vgpu10(svga) &&
      !svga_rects_equal(&rect, &svga->state.hw_clear.viewport)) {
   restore_viewport = true;
   ret = SVGA3D_SetViewport(svga->swc, &rect);
   if (ret != PIPE_OK)
               if (svga_have_vgpu10(svga)) {
      if (flags & SVGA3D_CLEAR_COLOR) {
                     if (int_target && !ints_fit_in_floats(color)) {
      clear_buffers_with_quad(svga, buffers, color, depth, stencil);
   /* We also cleared depth/stencil, so that's done */
      }
   else {
                     if (int_target) {
      rgba[0] = (float) color->i[0];
   rgba[1] = (float) color->i[1];
   rgba[2] = (float) color->i[2];
      }
   else {
      rgba[0] = color->f[0];
   rgba[1] = color->f[1];
                     /* Issue VGPU10 Clear commands */
   for (i = 0; i < fb->nr_cbufs; i++) {
                           rtv = svga_validate_surface_view(svga,
                        ret = SVGA3D_vgpu10_ClearRenderTargetView(svga->swc, rtv, rgba);
   if (ret != PIPE_OK)
            }
   if (flags & (SVGA3D_CLEAR_DEPTH | SVGA3D_CLEAR_STENCIL)) {
      struct pipe_surface *dsv =
                        ret = SVGA3D_vgpu10_ClearDepthStencilView(svga->swc, dsv, flags,
         if (ret != PIPE_OK)
         }
   else {
      ret = SVGA3D_ClearRect(svga->swc, flags, uc.ui[0], (float) depth, stencil,
         if (ret != PIPE_OK)
               if (restore_viewport) {
         }
         }
      /**
   * Clear the given surface to the specified value.
   * No masking, no scissor (clear entire buffer).
   */
   static void
   svga_clear(struct pipe_context *pipe, unsigned buffers, const struct pipe_scissor_state *scissor_state,
               {
      struct svga_context *svga = svga_context( pipe );
            if (buffers & PIPE_CLEAR_COLOR) {
      struct svga_winsys_surface *h = NULL;
   if (svga->curr.framebuffer.cbufs[0]) {
         }
               /* flush any queued prims (don't want them to appear after the clear!) */
                     /*
   * Mark target surfaces as dirty
   * TODO Mark only cleared surfaces.
   */
               }
         static void
   svga_clear_texture(struct pipe_context *pipe,
                     struct pipe_resource *res,
   {
      struct svga_context *svga = svga_context(pipe);
   struct svga_surface *svga_surface_dst;
   struct pipe_surface tmpl;
            memset(&tmpl, 0, sizeof(tmpl));
   tmpl.format = res->format;
   tmpl.u.tex.first_layer = box->z;
   tmpl.u.tex.last_layer = box->z + box->depth - 1;
            surface = pipe->create_surface(pipe, res, &tmpl);
   if (surface == NULL) {
      debug_printf("failed to create surface\n");
      }
            union pipe_color_union color;
   const struct util_format_description *desc =
            if (util_format_is_depth_or_stencil(surface->format)) {
      float depth;
   uint8_t stencil;
            /* If data is NULL, then set depthValue and stencilValue to zeros */
   if (data == NULL) {
      depth = 0.0;
      }
   else {
      util_format_unpack_z_float(surface->format, &depth, data, 1);
               if (util_format_has_depth(desc)) {
         }
   if (util_format_has_stencil(desc)) {
                  /* Setup depth stencil view */
   struct pipe_surface *dsv =
            if (!dsv) {
      pipe_surface_reference(&surface, NULL);
               if (box->x == 0 && box->y == 0 && box->width == surface->width &&
      box->height == surface->height) {
                  SVGA_RETRY(svga, SVGA3D_vgpu10_ClearDepthStencilView(svga->swc, dsv,
            }
   else {
               util_blitter_save_framebuffer(svga->blitter,
         begin_blit(svga);
   util_blitter_clear_depth_stencil(svga->blitter,
                           }
   else {
               if (data == NULL) {
      /* If data is NULL, the texture image is filled with zeros */
      }
   else {
                  /* Setup render target view */
   struct pipe_surface *rtv =
            if (!rtv) {
      pipe_surface_reference(&surface, NULL);
               if (box->x == 0 && box->y == 0 && box->width == surface->width &&
      box->height == surface->height) {
                  if (int_target && !ints_fit_in_floats(&color)) {
      /* To clear full texture with integer format */
      }
                     if (int_target) {
      rgba[0] = (float) color.i[0];
   rgba[1] = (float) color.i[1];
   rgba[2] = (float) color.i[2];
      }
   else {
      rgba[0] = color.f[0];
   rgba[1] = color.f[1];
                     /* clearing whole surface using VGPU10 command */
   assert(svga_surface(rtv)->view_id != SVGA3D_INVALID_ID);
   SVGA_RETRY(svga, SVGA3D_vgpu10_ClearRenderTargetView(svga->swc, rtv,
         }
   else {
               /**
   * util_blitter_clear_render_target doesn't support PIPE_TEXTURE_3D
   * It tries to draw quad with depth 0 for PIPE_TEXTURE_3D so use
   * util_clear_render_target() for PIPE_TEXTURE_3D.
   */
   if (rtv->texture->target != PIPE_TEXTURE_3D &&
      pipe->screen->is_format_supported(pipe->screen, rtv->format,
                           /* clear with quad drawing */
   util_blitter_save_framebuffer(svga->blitter,
         begin_blit(svga);
   util_blitter_clear_render_target(svga->blitter,
                        }
                     /* store layer values */
   unsigned first_layer = rtv->u.tex.first_layer;
                  for (unsigned i = 0; i < box_depth; i++) {
      rtv->u.tex.first_layer = rtv->u.tex.last_layer =
         util_clear_render_target(pipe, rtv, &color, box->x, box->y,
      }
   /* restore layer values */
   rtv->u.tex.first_layer = first_layer;
            }
      }
      /**
   * \brief  Clear the whole render target using vgpu10 functionality
   *
   * \param svga[in]  The svga context
   * \param dst[in]  The surface to clear
   * \param color[in]  Clear color
   * \return PIPE_OK if all well, PIPE_ERROR_OUT_OF_MEMORY if ran out of
   * command submission resources.
   */
   static enum pipe_error
   svga_try_clear_render_target(struct svga_context *svga,
               {
      struct pipe_surface *rtv =
            if (!rtv)
            assert(svga_surface(rtv)->view_id != SVGA3D_INVALID_ID);
      }
      /**
   * \brief  Clear part of render target using gallium blitter utilities
   *
   * \param svga[in]  The svga context
   * \param dst[in]  The surface to clear
   * \param color[in]  Clear color
   * \param dstx[in]  Clear region left
   * \param dsty[in]  Clear region top
   * \param width[in]  Clear region width
   * \param height[in]  Clear region height
   */
   static void
   svga_blitter_clear_render_target(struct svga_context *svga,
                           {
      begin_blit(svga);
            util_blitter_clear_render_target(svga->blitter, dst, color,
      }
         /**
   * \brief Clear render target pipe callback
   *
   * \param pipe[in]  The pipe context
   * \param dst[in]  The surface to clear
   * \param color[in]  Clear color
   * \param dstx[in]  Clear region left
   * \param dsty[in]  Clear region top
   * \param width[in]  Clear region width
   * \param height[in]  Clear region height
   * \param render_condition_enabled[in]  Whether to use conditional rendering
   * to clear (if elsewhere enabled).
   */
   static void
   svga_clear_render_target(struct pipe_context *pipe,
                           struct pipe_surface *dst,
   {
               svga_toggle_render_condition(svga, render_condition_enabled, false);
   if (!svga_have_vgpu10(svga) || dstx != 0 || dsty != 0 ||
      width != dst->width || height != dst->height) {
   svga_blitter_clear_render_target(svga, dst, color, dstx, dsty, width,
      } else {
               SVGA_RETRY_OOM(svga, ret, svga_try_clear_render_target(svga, dst,
            }
      }
      void svga_init_clear_functions(struct svga_context *svga)
   {
      svga->pipe.clear_render_target = svga_clear_render_target;
   svga->pipe.clear_texture = svga_have_vgpu10(svga) ? svga_clear_texture : NULL;
      }
