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
      #include "fd5_format.h"
   #include "fd5_image.h"
   #include "fd5_texture.h"
   #include "freedreno_resource.h"
      static enum a4xx_state_block texsb[] = {
      [PIPE_SHADER_COMPUTE] = SB4_CS_TEX,
      };
      static enum a4xx_state_block imgsb[] = {
      [PIPE_SHADER_COMPUTE] = SB4_CS_SSBO,
      };
      struct fd5_image {
      enum pipe_format pfmt;
   enum a5xx_tex_fmt fmt;
   enum a5xx_tex_type type;
   bool srgb;
   uint32_t cpp;
   uint32_t width;
   uint32_t height;
   uint32_t depth;
   uint32_t pitch;
   uint32_t array_pitch;
   struct fd_bo *bo;
   uint32_t offset;
      };
      static void
   translate_image(struct fd5_image *img, struct pipe_image_view *pimg)
   {
      enum pipe_format format = pimg->format;
   struct pipe_resource *prsc = pimg->resource;
            if (!pimg->resource) {
      memset(img, 0, sizeof(*img));
               img->pfmt = format;
   img->fmt = fd5_pipe2tex(format);
   img->type = fd5_tex_type(prsc->target);
   img->srgb = util_format_is_srgb(format);
   img->cpp = rsc->layout.cpp;
            /* Treat cube textures as 2d-array: */
   if (img->type == A5XX_TEX_CUBE)
            if (prsc->target == PIPE_BUFFER) {
      img->buffer = true;
   img->offset = pimg->u.buf.offset;
   img->pitch = 0;
            /* size is encoded with low 15b in WIDTH and high bits in
   * HEIGHT, in units of elements:
   */
   unsigned sz = pimg->u.buf.size / util_format_get_blocksize(format);
   img->width = sz & MASK(15);
   img->height = sz >> 15;
      } else {
               unsigned lvl = pimg->u.tex.level;
   img->offset = fd_resource_offset(rsc, lvl, pimg->u.tex.first_layer);
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
      default:
      img->array_pitch = 0;
   img->depth = 0;
            }
      static void
   emit_image_tex(struct fd_ringbuffer *ring, unsigned slot, struct fd5_image *img,
         {
      OUT_PKT7(ring, CP_LOAD_STATE4, 3 + 12);
   OUT_RING(ring, CP_LOAD_STATE4_0_DST_OFF(slot) |
                     OUT_RING(ring, CP_LOAD_STATE4_1_STATE_TYPE(ST4_CONSTANTS) |
                  OUT_RING(ring, A5XX_TEX_CONST_0_FMT(img->fmt) |
                     OUT_RING(ring, A5XX_TEX_CONST_1_WIDTH(img->width) |
         OUT_RING(ring,
            COND(img->buffer, A5XX_TEX_CONST_2_BUFFER) |
      OUT_RING(ring, A5XX_TEX_CONST_3_ARRAY_PITCH(img->array_pitch));
   if (img->bo) {
      OUT_RELOC(ring, img->bo, img->offset,
      } else {
      OUT_RING(ring, 0x00000000);
      }
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
      }
      static void
   emit_image_ssbo(struct fd_ringbuffer *ring, unsigned slot,
         {
      OUT_PKT7(ring, CP_LOAD_STATE4, 3 + 2);
   OUT_RING(ring, CP_LOAD_STATE4_0_DST_OFF(slot) |
                     OUT_RING(ring, CP_LOAD_STATE4_1_STATE_TYPE(ST4_CONSTANTS) |
         OUT_RING(ring, CP_LOAD_STATE4_2_EXT_SRC_ADDR_HI(0));
   OUT_RING(ring,
         OUT_RING(ring, A5XX_SSBO_1_1_HEIGHT(img->height) |
            OUT_PKT7(ring, CP_LOAD_STATE4, 3 + 2);
   OUT_RING(ring, CP_LOAD_STATE4_0_DST_OFF(slot) |
                     OUT_RING(ring, CP_LOAD_STATE4_1_STATE_TYPE(ST4_UBO) |
         OUT_RING(ring, CP_LOAD_STATE4_2_EXT_SRC_ADDR_HI(0));
   if (img->bo) {
         } else {
      OUT_RING(ring, 0x00000000);
         }
      /* Emit required "SSBO" and sampler state.  The sampler state is used by the
   * hw for imageLoad(), and "SSBO" state for imageStore().  Returns max sampler
   * used.
   */
   void
   fd5_emit_images(struct fd_context *ctx, struct fd_ringbuffer *ring,
               {
      struct fd_shaderimg_stateobj *so = &ctx->shaderimg[shader];
   unsigned enabled_mask = so->enabled_mask;
            while (enabled_mask) {
      unsigned index = u_bit_scan(&enabled_mask);
                     if (m->image_to_tex[index] != IBO_INVALID)
         emit_image_ssbo(ring, v->num_ssbos + index, &img,
         }
