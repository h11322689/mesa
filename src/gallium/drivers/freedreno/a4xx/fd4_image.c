   /*
   * Copyright (C) 2017 Rob Clark <robclark@freedesktop.org>
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   * Authors:
   *    Rob Clark <robclark@freedesktop.org>
   */
      #include "pipe/p_state.h"
      #include "freedreno_resource.h"
   #include "fd4_image.h"
   #include "fd4_format.h"
   #include "fd4_texture.h"
      static enum a4xx_state_block texsb[] = {
      [PIPE_SHADER_COMPUTE] = SB4_CS_TEX,
      };
      static enum a4xx_state_block imgsb[] = {
      [PIPE_SHADER_COMPUTE] = SB4_CS_SSBO,
      };
      struct fd4_image {
      enum pipe_format pfmt;
   enum a4xx_color_fmt fmt;
   enum a4xx_tex_fmt texfmt;
   enum a4xx_tex_type type;
   bool srgb;
   uint32_t cpp;
   uint32_t width;
   uint32_t height;
   uint32_t depth;
   uint32_t pitch;
   uint32_t array_pitch;
   uint32_t pitchalign;
   struct fd_bo *bo;
   uint32_t offset;
   bool buffer;
      };
      static void translate_image(struct fd4_image *img, struct pipe_image_view *pimg)
   {
      enum pipe_format format = pimg->format;
   struct pipe_resource *prsc = pimg->resource;
            if (!pimg->resource) {
      memset(img, 0, sizeof(*img));
               img->pfmt      = format;
   img->fmt       = fd4_pipe2color(format);
   img->texfmt    = fd4_pipe2tex(format);
   img->type      = fd4_tex_type(prsc->target);
   img->srgb      = util_format_is_srgb(format);
   img->bo        = rsc->bo;
            /* Treat cube textures as 2d-array: */
   if (img->type == A4XX_TEX_CUBE)
            if (prsc->target == PIPE_BUFFER) {
      img->buffer = true;
   img->offset = pimg->u.buf.offset;
   img->cpp = util_format_get_blocksize(format);
   img->array_pitch = 0;
   img->pitch = 0;
            /* size is encoded with low 16b in WIDTH and high bits in
   * HEIGHT, in units of elements:
   */
   unsigned sz = pimg->u.buf.size / img->cpp;
   img->width = sz & MASK(16);
   img->height = sz >> 16;
            /* Note that the blob sets the PITCH to the CPP in the SSBO descriptor,
   * but that messes up the sampler we create, so skip that.
      } else {
      img->buffer = false;
            unsigned lvl = pimg->u.tex.level;
   img->offset = fd_resource_offset(rsc, lvl, pimg->u.tex.first_layer);
   img->pitch = fd_resource_pitch(rsc, lvl);
            img->width = u_minify(prsc->width0, lvl);
                     switch (prsc->target) {
   case PIPE_TEXTURE_RECT:
   case PIPE_TEXTURE_1D:
   case PIPE_TEXTURE_2D:
      img->array_pitch = rsc->layout.layer_size;
   img->depth = 1;
      case PIPE_TEXTURE_1D_ARRAY:
   case PIPE_TEXTURE_2D_ARRAY:
      img->array_pitch = rsc->layout.layer_size;
   img->depth = layers;
      case PIPE_TEXTURE_CUBE:
   case PIPE_TEXTURE_CUBE_ARRAY:
      img->array_pitch = rsc->layout.layer_size;
   img->depth = layers;
      case PIPE_TEXTURE_3D:
      img->array_pitch = fd_resource_slice(rsc, lvl)->size0;
   img->depth = u_minify(prsc->depth0, lvl);
   if (layers == 1 && img->depth > 1) {
      img->type = A4XX_TEX_2D;
      } else {
         }
      default:
      img->array_pitch = 0;
   img->depth = 0;
            }
      static void emit_image_tex(struct fd_ringbuffer *ring, unsigned slot,
         {
      OUT_PKT3(ring, CP_LOAD_STATE4, 2 + 8);
   OUT_RING(ring, CP_LOAD_STATE4_0_DST_OFF(slot) |
      CP_LOAD_STATE4_0_STATE_SRC(SS4_DIRECT) |
   CP_LOAD_STATE4_0_STATE_BLOCK(texsb[shader]) |
      OUT_RING(ring, CP_LOAD_STATE4_1_STATE_TYPE(ST4_CONSTANTS) |
            OUT_RING(ring, A4XX_TEX_CONST_0_FMT(img->texfmt) |
         A4XX_TEX_CONST_0_TYPE(img->type) |
   fd4_tex_swiz(img->pfmt, PIPE_SWIZZLE_X, PIPE_SWIZZLE_Y,
         OUT_RING(ring, A4XX_TEX_CONST_1_WIDTH(img->width) |
         OUT_RING(ring, A4XX_TEX_CONST_2_PITCHALIGN(img->pitchalign) |
      A4XX_TEX_CONST_2_PITCH(img->pitch) |
      OUT_RING(ring, A4XX_TEX_CONST_3_DEPTH(img->depth) |
         if (img->bo) {
         } else {
         }
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
            /* Per fd4_emit.c, some of the hw likes samplers in pairs */
   OUT_PKT3(ring, CP_LOAD_STATE4, 2 + 4);
   OUT_RING(ring, CP_LOAD_STATE4_0_DST_OFF(slot) |
      CP_LOAD_STATE4_0_STATE_SRC(SS4_DIRECT) |
   CP_LOAD_STATE4_0_STATE_BLOCK(texsb[shader]) |
      OUT_RING(ring, CP_LOAD_STATE4_1_STATE_TYPE(ST4_SHADER) |
         OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
      }
      static void emit_image_ssbo(struct fd_ringbuffer *ring, unsigned slot,
         {
      OUT_PKT3(ring, CP_LOAD_STATE4, 2 + 4);
   OUT_RING(ring, CP_LOAD_STATE4_0_DST_OFF(slot) |
      CP_LOAD_STATE4_0_STATE_SRC(SS4_DIRECT) |
   CP_LOAD_STATE4_0_STATE_BLOCK(imgsb[shader]) |
      OUT_RING(ring, CP_LOAD_STATE4_1_STATE_TYPE(0) |
         OUT_RELOC(ring, img->bo, img->offset, 0, 0);
   OUT_RING(ring, A4XX_SSBO_0_1_PITCH(img->pitch));
   OUT_RING(ring, A4XX_SSBO_0_2_ARRAY_PITCH(img->array_pitch));
            OUT_PKT3(ring, CP_LOAD_STATE4, 2 + 2);
   OUT_RING(ring, CP_LOAD_STATE4_0_DST_OFF(slot) |
      CP_LOAD_STATE4_0_STATE_SRC(SS4_DIRECT) |
   CP_LOAD_STATE4_0_STATE_BLOCK(imgsb[shader]) |
      OUT_RING(ring, CP_LOAD_STATE4_1_STATE_TYPE(1) |
         OUT_RING(ring, A4XX_SSBO_1_0_CPP(img->cpp) |
      A4XX_SSBO_1_0_FMT(img->fmt) |
      OUT_RING(ring, A4XX_SSBO_1_1_HEIGHT(img->height) |
      }
      /* Emit required "SSBO" and sampler state.  The sampler state is used by the
   * hw for imageLoad(), and "SSBO" state for imageStore().  Returns max sampler
   * used.
   */
   void
   fd4_emit_images(struct fd_context *ctx, struct fd_ringbuffer *ring,
         enum pipe_shader_type shader,
   {
      struct fd_shaderimg_stateobj *so = &ctx->shaderimg[shader];
   unsigned enabled_mask = so->enabled_mask;
            while (enabled_mask) {
      unsigned index = u_bit_scan(&enabled_mask);
                     if (m->image_to_tex[index] != IBO_INVALID)
               }
