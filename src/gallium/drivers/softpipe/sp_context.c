   /**************************************************************************
   * 
   * Copyright 2007 VMware, Inc.
   * All Rights Reserved.
   * Copyright 2008 VMware, Inc.  All rights reserved.
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
      /* Author:
   *    Keith Whitwell <keithw@vmware.com>
   */
      #include "draw/draw_context.h"
   #include "draw/draw_vbuf.h"
   #include "pipe/p_defines.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/u_inlines.h"
   #include "util/u_upload_mgr.h"
   #include "util/u_debug_cb.h"
   #include "tgsi/tgsi_exec.h"
   #include "sp_buffer.h"
   #include "sp_clear.h"
   #include "sp_context.h"
   #include "sp_flush.h"
   #include "sp_prim_vbuf.h"
   #include "sp_state.h"
   #include "sp_surface.h"
   #include "sp_tile_cache.h"
   #include "sp_tex_tile_cache.h"
   #include "sp_texture.h"
   #include "sp_query.h"
   #include "sp_screen.h"
   #include "sp_tex_sample.h"
   #include "sp_image.h"
      #include "nir.h"
      static void
   softpipe_destroy( struct pipe_context *pipe )
   {
      struct softpipe_context *softpipe = softpipe_context( pipe );
            if (softpipe->blitter) {
                  if (softpipe->draw)
            if (softpipe->quad.shade)
            if (softpipe->quad.depth_test)
            if (softpipe->quad.blend)
            if (softpipe->pipe.stream_uploader)
            for (i = 0; i < PIPE_MAX_COLOR_BUFS; i++) {
                  sp_destroy_tile_cache(softpipe->zsbuf_cache);
            for (sh = 0; sh < ARRAY_SIZE(softpipe->tex_cache); sh++) {
      for (i = 0; i < ARRAY_SIZE(softpipe->tex_cache[0]); i++) {
      sp_destroy_tex_tile_cache(softpipe->tex_cache[sh][i]);
                  for (sh = 0; sh < ARRAY_SIZE(softpipe->constants); sh++) {
      for (i = 0; i < ARRAY_SIZE(softpipe->constants[0]); i++) {
      if (softpipe->constants[sh][i]) {
                        for (i = 0; i < softpipe->num_vertex_buffers; i++) {
                           for (i = 0; i < PIPE_SHADER_TYPES; i++) {
      FREE(softpipe->tgsi.sampler[i]);
   FREE(softpipe->tgsi.image[i]);
                  }
         /**
   * if (the texture is being used as a framebuffer surface)
   *    return SP_REFERENCED_FOR_WRITE
   * else if (the texture is a bound texture source)
   *    return SP_REFERENCED_FOR_READ
   * else
   *    return SP_UNREFERENCED
   */
   unsigned int
   softpipe_is_resource_referenced( struct pipe_context *pipe,
               {
      struct softpipe_context *softpipe = softpipe_context( pipe );
            if (texture->target == PIPE_BUFFER)
            /* check if any of the bound drawing surfaces are this texture */
   if (softpipe->dirty_render_cache) {
      for (i = 0; i < softpipe->framebuffer.nr_cbufs; i++) {
      if (softpipe->framebuffer.cbufs[i] && 
      softpipe->framebuffer.cbufs[i]->texture == texture) {
         }
   if (softpipe->framebuffer.zsbuf && 
      softpipe->framebuffer.zsbuf->texture == texture) {
         }
      /* check if any of the tex_cache textures are this texture */
   for (sh = 0; sh < ARRAY_SIZE(softpipe->tex_cache); sh++) {
      for (i = 0; i < ARRAY_SIZE(softpipe->tex_cache[0]); i++) {
      if (softpipe->tex_cache[sh][i] &&
      softpipe->tex_cache[sh][i]->texture == texture)
                  }
               static void
   softpipe_render_condition(struct pipe_context *pipe,
                     {
               softpipe->render_cond_query = query;
   softpipe->render_cond_mode = mode;
      }
         struct pipe_context *
   softpipe_create_context(struct pipe_screen *screen,
         {
      struct softpipe_screen *sp_screen = softpipe_screen(screen);
   struct softpipe_context *softpipe = CALLOC_STRUCT(softpipe_context);
                     for (i = 0; i < PIPE_SHADER_TYPES; i++) {
                  for (i = 0; i < PIPE_SHADER_TYPES; i++) {
                  for (i = 0; i < PIPE_SHADER_TYPES; i++) {
                  softpipe->pipe.screen = screen;
   softpipe->pipe.destroy = softpipe_destroy;
            /* state setters */
   softpipe_init_blend_funcs(&softpipe->pipe);
   softpipe_init_clip_funcs(&softpipe->pipe);
   softpipe_init_query_funcs( softpipe );
   softpipe_init_rasterizer_funcs(&softpipe->pipe);
   softpipe_init_sampler_funcs(&softpipe->pipe);
   softpipe_init_shader_funcs(&softpipe->pipe);
   softpipe_init_streamout_funcs(&softpipe->pipe);
   softpipe_init_texture_funcs( &softpipe->pipe );
   softpipe_init_vertex_funcs(&softpipe->pipe);
            softpipe->pipe.set_framebuffer_state = softpipe_set_framebuffer_state;
                              softpipe->pipe.clear = softpipe_clear;
   softpipe->pipe.flush = softpipe_flush_wrapped;
   softpipe->pipe.texture_barrier = softpipe_texture_barrier;
   softpipe->pipe.memory_barrier = softpipe_memory_barrier;
   softpipe->pipe.render_condition = softpipe_render_condition;
      /*
   * Alloc caches for accessing drawing surfaces and textures.
   * Must be before quad stage setup!
   */
   for (i = 0; i < PIPE_MAX_COLOR_BUFS; i++)
                  /* Allocate texture caches */
   for (sh = 0; sh < ARRAY_SIZE(softpipe->tex_cache); sh++) {
      for (i = 0; i < ARRAY_SIZE(softpipe->tex_cache[0]); i++) {
      softpipe->tex_cache[sh][i] = sp_create_tex_tile_cache(&softpipe->pipe);
   if (!softpipe->tex_cache[sh][i])
                           /* setup quad rendering stages */
   softpipe->quad.shade = sp_quad_shade_stage(softpipe);
   softpipe->quad.depth_test = sp_quad_depth_test_stage(softpipe);
            softpipe->pipe.stream_uploader = u_upload_create_default(&softpipe->pipe);
   if (!softpipe->pipe.stream_uploader)
                  /*
   * Create drawing context and plug our rendering stage into it.
   */
   if (sp_screen->use_llvm)
         else
         if (!softpipe->draw) 
            draw_texture_sampler(softpipe->draw,
                        draw_texture_sampler(softpipe->draw,
                        draw_image(softpipe->draw,
            PIPE_SHADER_VERTEX,
         draw_image(softpipe->draw,
            PIPE_SHADER_GEOMETRY,
         draw_buffer(softpipe->draw,
            PIPE_SHADER_VERTEX,
         draw_buffer(softpipe->draw,
            PIPE_SHADER_GEOMETRY,
         softpipe->vbuf_backend = sp_create_vbuf_backend(softpipe);
   if (!softpipe->vbuf_backend)
            softpipe->vbuf = draw_vbuf_stage(softpipe->draw, softpipe->vbuf_backend);
   if (!softpipe->vbuf)
            draw_set_rasterize_stage(softpipe->draw, softpipe->vbuf);
            softpipe->blitter = util_blitter_create(&softpipe->pipe);
   if (!softpipe->blitter) {
                  /* must be done before installing Draw stages */
            /* plug in AA line/point stages */
   draw_install_aaline_stage(softpipe->draw, &softpipe->pipe);
            /* Do polygon stipple w/ texture map + frag prog. */
                                    fail:
      softpipe_destroy(&softpipe->pipe);
      }
