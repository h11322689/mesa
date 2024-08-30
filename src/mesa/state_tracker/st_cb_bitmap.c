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
      /*
   * Authors:
   *   Brian Paul
   */
      #include "main/errors.h"
      #include "main/image.h"
   #include "main/bufferobj.h"
   #include "main/framebuffer.h"
   #include "main/macros.h"
   #include "main/pbo.h"
   #include "program/program.h"
   #include "program/prog_print.h"
      #include "st_context.h"
   #include "st_atom.h"
   #include "st_atom_constbuf.h"
   #include "st_draw.h"
   #include "st_program.h"
   #include "st_cb_bitmap.h"
   #include "st_cb_drawpixels.h"
   #include "st_sampler_view.h"
   #include "st_texture.h"
   #include "st_util.h"
      #include "pipe/p_context.h"
   #include "pipe/p_defines.h"
   #include "pipe/p_shader_tokens.h"
   #include "util/u_inlines.h"
   #include "program/prog_instruction.h"
   #include "cso_cache/cso_context.h"
         /**
   * glBitmaps are drawn as textured quads.  The user's bitmap pattern
   * is stored in a texture image.  An alpha8 texture format is used.
   * The fragment shader samples a bit (texel) from the texture, then
   * discards the fragment if the bit is off.
   *
   * Note that we actually store the inverse image of the bitmap to
   * simplify the fragment program.  An "on" bit gets stored as texel=0x0
   * and an "off" bit is stored as texel=0xff.  Then we kill the
   * fragment if the negated texel value is less than zero.
   */
         /**
   * The bitmap cache attempts to accumulate multiple glBitmap calls in a
   * buffer which is then rendered en mass upon a flush, state change, etc.
   * A wide, short buffer is used to target the common case of a series
   * of glBitmap calls being used to draw text.
   */
   static GLboolean UseBitmapCache = GL_TRUE;
         #define BITMAP_CACHE_WIDTH  512
   #define BITMAP_CACHE_HEIGHT 32
         /** Epsilon for Z comparisons */
   #define Z_EPSILON 1e-06
      static void
   init_bitmap_state(struct st_context *st);
      /**
   * Copy user-provide bitmap bits into texture buffer, expanding
   * bits into texels.
   * "On" bits will set texels to 0x0.
   * "Off" bits will not modify texels.
   * Note that the image is actually going to be upside down in
   * the texture.  We deal with that with texcoords.
   */
   static void
   unpack_bitmap(struct st_context *st,
               GLint px, GLint py, GLsizei width, GLsizei height,
   const struct gl_pixelstore_attrib *unpack,
   {
               _mesa_expand_bitmap(width, height, unpack, bitmap,
      }
         /**
   * Create a texture which represents a bitmap image.
   */
   struct pipe_resource *
   st_make_bitmap_texture(struct gl_context *ctx, GLsizei width, GLsizei height,
               {
      struct st_context *st = st_context(ctx);
   struct pipe_context *pipe = st->pipe;
   struct pipe_transfer *transfer;
   uint8_t *dest;
            if (!st->bitmap.tex_format)
            /* PBO source... */
   bitmap = _mesa_map_pbo_source(ctx, unpack, bitmap);
   if (!bitmap) {
                  /**
   * Create texture to hold bitmap pattern.
   */
   pt = st_texture_create(st, st->internal_target, st->bitmap.tex_format,
               if (!pt) {
      _mesa_unmap_pbo_source(ctx, unpack);
               dest = pipe_texture_map(st->pipe, pt, 0, 0,
                  /* Put image into texture transfer */
   memset(dest, 0xff, height * transfer->stride);
   unpack_bitmap(st, 0, 0, width, height, unpack, bitmap,
                     /* Release transfer */
   pipe_texture_unmap(pipe, transfer);
      }
         /**
   * Setup pipeline state prior to rendering the bitmap textured quad.
   */
   static void
   setup_render_state(struct gl_context *ctx,
                     {
      struct st_context *st = st_context(ctx);
   struct pipe_context *pipe = st->pipe;
   struct cso_context *cso = st->cso_context;
   struct st_fp_variant *fpv;
            memset(&key, 0, sizeof(key));
   key.st = st->has_shareable_shaders ? NULL : st;
   key.bitmap = GL_TRUE;
   key.clamp_color = st->clamp_frag_color_in_shader &&
                           /* As an optimization, Mesa's fragment programs will sometimes get the
   * primary color from a statevar/constant rather than a varying variable.
   * when that's the case, we need to ensure that we use the 'color'
   * parameter and not the current attribute color (which may have changed
   * through glRasterPos and state validation.
   * So, we force the proper color here.  Not elegant, but it works.
   */
   {
      GLfloat colorSave[4];
   COPY_4V(colorSave, ctx->Current.Attrib[VERT_ATTRIB_COLOR0]);
   COPY_4V(ctx->Current.Attrib[VERT_ATTRIB_COLOR0], color);
   st_upload_constants(st, fp, MESA_SHADER_FRAGMENT);
               cso_save_state(cso, (CSO_BIT_RASTERIZER |
                        CSO_BIT_FRAGMENT_SAMPLERS |
            /* rasterizer state: just scissor */
   st->bitmap.rasterizer.scissor = scissor_enabled;
            /* fragment shader state: TEX lookup program */
            /* vertex shader state: position + texcoord pass-through */
            /* disable other shaders */
   cso_set_tessctrl_shader_handle(cso, NULL);
   cso_set_tesseval_shader_handle(cso, NULL);
            /* user samplers, plus our bitmap sampler */
   {
      struct pipe_sampler_state *samplers[PIPE_MAX_SAMPLERS];
   uint num = MAX2(fpv->bitmap_sampler + 1,
         uint i;
   for (i = 0; i < st->state.num_frag_samplers; i++) {
         }
   samplers[fpv->bitmap_sampler] = &st->bitmap.sampler;
   cso_set_samplers(cso, PIPE_SHADER_FRAGMENT, num,
               /* user textures, plus the bitmap texture */
   {
      struct pipe_sampler_view *sampler_views[PIPE_MAX_SAMPLERS];
   unsigned num_views =
            num_views = MAX2(fpv->bitmap_sampler + 1, num_views);
   sampler_views[fpv->bitmap_sampler] = sv;
   pipe->set_sampler_views(pipe, PIPE_SHADER_FRAGMENT, 0, num_views, 0,
                     /* viewport state: viewport matching window dims */
   cso_set_viewport_dims(cso, st->state.fb_width,
                  st->util_velems.count = 3;
               }
         /**
   * Restore pipeline state after rendering the bitmap textured quad.
   */
   static void
   restore_render_state(struct gl_context *ctx)
   {
      struct st_context *st = st_context(ctx);
            /* Unbind all because st/mesa won't do it if the current shader doesn't
   * use them.
   */
   cso_restore_state(cso, CSO_UNBIND_FS_SAMPLERVIEWS);
            ctx->Array.NewVertexElements = true;
   ctx->NewDriverState |= ST_NEW_VERTEX_ARRAYS |
      }
         /**
   * Render a glBitmap by drawing a textured quad
   *
   * take_ownership means the callee will be resposible for unreferencing sv.
   */
   static void
   draw_bitmap_quad(struct gl_context *ctx, GLint x, GLint y, GLfloat z,
                  GLsizei width, GLsizei height,
      {
      struct st_context *st = st_context(ctx);
   const float fb_width = (float) st->state.fb_width;
   const float fb_height = (float) st->state.fb_height;
   const float x0 = (float) x;
   const float x1 = (float) (x + width);
   const float y0 = (float) y;
   const float y1 = (float) (y + height);
   float sLeft = 0.0f, sRight = 1.0f;
   float tTop = 0.0f, tBot = 1.0f - tTop;
   const float clip_x0 = x0 / fb_width * 2.0f - 1.0f;
   const float clip_y0 = y0 / fb_height * 2.0f - 1.0f;
   const float clip_x1 = x1 / fb_width * 2.0f - 1.0f;
            /* limit checks */
   {
      /* XXX if the bitmap is larger than the max texture size, break
   * it up into chunks.
   */
   ASSERTED GLuint maxSize =
         assert(width <= (GLsizei) maxSize);
               if (sv->texture->target == PIPE_TEXTURE_RECT) {
      /* use non-normalized texcoords */
   sRight = (float) width;
                        /* convert Z from [0,1] to [-1,-1] to match viewport Z scale/bias */
            if (!st_draw_quad(st, clip_x0, clip_y0, clip_x1, clip_y1, z,
                                 /* We uploaded modified constants, need to invalidate them. */
      }
         static void
   reset_cache(struct st_context *st)
   {
               /*memset(cache->buffer, 0xff, sizeof(cache->buffer));*/
            cache->xmin = 1000000;
   cache->xmax = -1000000;
   cache->ymin = 1000000;
                              /* allocate a new texture */
   cache->texture = st_texture_create(st, st->internal_target,
                              }
         /** Print bitmap image to stdout (debug) */
   static void
   print_cache(const struct st_bitmap_cache *cache)
   {
               for (i = 0; i < BITMAP_CACHE_HEIGHT; i++) {
      k = BITMAP_CACHE_WIDTH * (BITMAP_CACHE_HEIGHT - i - 1);
   for (j = 0; j < BITMAP_CACHE_WIDTH; j++) {
      if (cache->buffer[k])
         else
            }
         }
         /**
   * Create gallium pipe_transfer object for the bitmap cache.
   */
   static void
   create_cache_trans(struct st_context *st)
   {
      struct pipe_context *pipe = st->pipe;
            if (cache->trans)
            /* Map the texture transfer.
   * Subsequent glBitmap calls will write into the texture image.
   */
   cache->buffer = pipe_texture_map(pipe, cache->texture, 0, 0,
                        /* init image to all 0xff */
      }
         /**
   * If there's anything in the bitmap cache, draw/flush it now.
   */
   void
   st_flush_bitmap_cache(struct st_context *st)
   {
               if (!cache->empty) {
      struct pipe_context *pipe = st->pipe;
                     if (0)
      printf("flush bitmap, size %d x %d  at %d, %d\n",
                     /* The texture transfer has been mapped until now.
   * So unmap and release the texture transfer before drawing.
   */
   if (cache->trans && cache->buffer) {
      if (0)
         pipe_texture_unmap(pipe, cache->trans);
   cache->buffer = NULL;
               sv = st_create_texture_sampler_view(st->pipe, cache->texture);
   if (sv) {
      draw_bitmap_quad(st->ctx,
                  cache->xpos,
   cache->ypos,
   cache->zpos,
   BITMAP_CACHE_WIDTH, BITMAP_CACHE_HEIGHT,
   sv,
   cache->color,
            /* release/free the texture */
                  }
         /**
   * Try to accumulate this glBitmap call in the bitmap cache.
   * \return  GL_TRUE for success, GL_FALSE if bitmap is too large, etc.
   */
   static GLboolean
   accum_bitmap(struct gl_context *ctx,
               GLint x, GLint y, GLsizei width, GLsizei height,
   {
      struct st_context *st = ctx->st;
   struct st_bitmap_cache *cache = &st->bitmap.cache;
   int px = -999, py = -999;
            if (width > BITMAP_CACHE_WIDTH ||
      height > BITMAP_CACHE_HEIGHT)
         bool scissor_enabled = ctx->Scissor.EnableFlags & 0x1;
            if (!cache->empty) {
      px = x - cache->xpos;  /* pos in buffer */
   py = y - cache->ypos;
   if (px < 0 || px + width > BITMAP_CACHE_WIDTH ||
      py < 0 || py + height > BITMAP_CACHE_HEIGHT ||
   !TEST_EQ_4V(ctx->Current.RasterColor, cache->color) ||
   ctx->FragmentProgram._Current != cache->fp ||
   scissor_enabled != cache->scissor_enabled ||
   clamp_frag_color != cache->clamp_frag_color ||
   ((fabsf(z - cache->zpos) > Z_EPSILON))) {
   /* This bitmap would extend beyond cache bounds, or the bitmap
   * color is changing
   * so flush and continue.
   */
                  if (cache->empty) {
      /* Initialize.  Center bitmap vertically in the buffer. */
   px = 0;
   py = (BITMAP_CACHE_HEIGHT - height) / 2;
   cache->xpos = x;
   cache->ypos = y - py;
   cache->zpos = z;
   cache->empty = GL_FALSE;
   COPY_4FV(cache->color, ctx->Current.RasterColor);
   _mesa_reference_program(ctx, &cache->fp, ctx->FragmentProgram._Current);
   cache->scissor_enabled = scissor_enabled;
               assert(px != -999);
            if (x < cache->xmin)
         if (y < cache->ymin)
         if (x + width > cache->xmax)
         if (y + height > cache->ymax)
            /* create the transfer if needed */
            /* PBO source... */
   bitmap = _mesa_map_pbo_source(ctx, unpack, bitmap);
   if (!bitmap) {
                  unpack_bitmap(st, px, py, width, height, unpack, bitmap,
                        }
         /**
   * One-time init for drawing bitmaps.
   */
   static void
   init_bitmap_state(struct st_context *st)
   {
               /* This function should only be called once */
            assert(st->internal_target == PIPE_TEXTURE_2D ||
            /* init sampler state once */
   memset(&st->bitmap.sampler, 0, sizeof(st->bitmap.sampler));
   st->bitmap.sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   st->bitmap.sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   st->bitmap.sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   st->bitmap.sampler.min_img_filter = PIPE_TEX_FILTER_NEAREST;
   st->bitmap.sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
   st->bitmap.sampler.mag_img_filter = PIPE_TEX_FILTER_NEAREST;
   st->bitmap.sampler.unnormalized_coords = !(st->internal_target == PIPE_TEXTURE_2D ||
                  /* init baseline rasterizer state once */
   memset(&st->bitmap.rasterizer, 0, sizeof(st->bitmap.rasterizer));
   st->bitmap.rasterizer.half_pixel_center = 1;
   st->bitmap.rasterizer.bottom_edge_rule = 1;
   st->bitmap.rasterizer.depth_clip_near = 1;
            /* find a usable texture format */
   if (screen->is_format_supported(screen, PIPE_FORMAT_R8_UNORM,
                     }
   else if (screen->is_format_supported(screen, PIPE_FORMAT_A8_UNORM,
                     }
   else {
      /* XXX support more formats */
               /* Create the vertex shader */
               }
      void
   st_Bitmap(struct gl_context *ctx, GLint x, GLint y,
            GLsizei width, GLsizei height,
      {
               assert(width > 0);
            st_invalidate_readpix_cache(st);
   if (tex)
            if (!st->bitmap.tex_format) {
                  /* We only need to validate any non-ST_NEW_CONSTANTS state. The VS we use
   * for bitmap drawing uses no constants and the FS constants are
   * explicitly uploaded in the draw_bitmap_quad() function.
   */
                     if (!tex) {
      if (UseBitmapCache && accum_bitmap(ctx, x, y, width, height, unpack, bitmap))
            struct pipe_resource *pt =
         if (!pt)
                     view = st_create_texture_sampler_view(st->pipe, pt);
   /* unreference the texture because it's referenced by sv */
      } else {
      /* tex comes from a display list. */
               if (view) {
      draw_bitmap_quad(ctx, x, y, ctx->Current.RasterPos[2],
                  width, height, view, ctx->Current.RasterColor,
      }
      /** Per-context tear-down */
   void
   st_destroy_bitmap(struct st_context *st)
   {
      struct pipe_context *pipe = st->pipe;
            if (cache->trans && cache->buffer) {
         }
   pipe_resource_reference(&st->bitmap.cache.texture, NULL);
      }
