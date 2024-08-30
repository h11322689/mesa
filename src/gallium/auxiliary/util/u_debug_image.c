   /*
   * Copyright (c) 2008-2016 VMware, Inc.
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
   */
         #include "util/u_debug_image.h"
   #include "util/format/u_format.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "util/u_string.h"
   #include "util/u_surface.h"
   #include "util/u_tile.h"
      #include <stdio.h>
         #ifdef DEBUG
      /**
   * Dump an image to .ppm file.
   * \param format  PIPE_FORMAT_x
   * \param cpp  bytes per pixel
   * \param width  width in pixels
   * \param height height in pixels
   * \param stride  row stride in bytes
   */
   void
   debug_dump_image(const char *prefix,
                  enum pipe_format format, UNUSED unsigned cpp,
      {
      /* write a ppm file */
   char filename[256];
   unsigned char *rgb8;
                     rgb8 = MALLOC(height * width * 3);
   if (!rgb8) {
                  util_format_translate(
         PIPE_FORMAT_R8G8B8_UNORM,
   rgb8, width * 3,
   0, 0,
   format,
            /* Must be opened in binary mode or DOS line ending causes data
   * to be read with one byte offset.
   */
   f = fopen(filename, "wb");
   if (f) {
      fprintf(f, "P6\n");
   fprintf(f, "# ppm-file created by gallium\n");
   fprintf(f, "%i %i\n", width, height);
   fprintf(f, "255\n");
   fwrite(rgb8, 1, height * width * 3, f);
      }
   else {
                     }
         /* FIXME: dump resources, not surfaces... */
   void
   debug_dump_surface(struct pipe_context *pipe,
               {
      struct pipe_resource *texture;
   struct pipe_transfer *transfer;
            if (!surface)
            /* XXX: this doesn't necessarily work, as the driver may be using
   * temporary storage for the surface which hasn't been propagated
   * back into the texture.  Need to nail down the semantics of views
   * and transfers a bit better before we can say if extra work needs
   * to be done here:
   */
            data = pipe_texture_map(pipe, texture, surface->u.tex.level,
                     if (!data)
            debug_dump_image(prefix,
                  texture->format,
   util_format_get_blocksize(texture->format),
   util_format_get_nblocksx(texture->format, surface->width),
            }
         void
   debug_dump_texture(struct pipe_context *pipe,
               {
               if (!texture)
            /* XXX for now, just dump image for layer=0, level=0 */
   u_surface_default_template(&surf_tmpl, texture);
   surface = pipe->create_surface(pipe, texture, &surf_tmpl);
   if (surface) {
      debug_dump_surface(pipe, prefix, surface);
         }
         #pragma pack(push,2)
   struct bmp_file_header {
      uint16_t bfType;
   uint32_t bfSize;
   uint16_t bfReserved1;
   uint16_t bfReserved2;
      };
   #pragma pack(pop)
      struct bmp_info_header {
      uint32_t biSize;
   int32_t biWidth;
   int32_t biHeight;
   uint16_t biPlanes;
   uint16_t biBitCount;
   uint32_t biCompression;
   uint32_t biSizeImage;
   int32_t biXPelsPerMeter;
   int32_t biYPelsPerMeter;
   uint32_t biClrUsed;
      };
      struct bmp_rgb_quad {
      uint8_t rgbBlue;
   uint8_t rgbGreen;
   uint8_t rgbRed;
      };
      void
   debug_dump_surface_bmp(struct pipe_context *pipe,
               {
      struct pipe_transfer *transfer;
   struct pipe_resource *texture = surface->texture;
            ptr = pipe_texture_map(pipe, texture, surface->u.tex.level,
                              }
      void
   debug_dump_transfer_bmp(UNUSED struct pipe_context *pipe,
               {
               if (!transfer)
            rgba = MALLOC(transfer->box.width *
   transfer->box.height *
   transfer->box.depth *
   4*sizeof(float));
   if (!rgba)
            pipe_get_tile_rgba(transfer, ptr, 0, 0,
                        debug_dump_float_rgba_bmp(filename,
                     error1:
         }
      void
   debug_dump_float_rgba_bmp(const char *filename,
               {
      FILE *stream;
   struct bmp_file_header bmfh;
   struct bmp_info_header bmih;
            if (!rgba)
            bmfh.bfType = 0x4d42;
   bmfh.bfSize = 14 + 40 + height*width*4;
   bmfh.bfReserved1 = 0;
   bmfh.bfReserved2 = 0;
            bmih.biSize = 40;
   bmih.biWidth = width;
   bmih.biHeight = height;
   bmih.biPlanes = 1;
   bmih.biBitCount = 32;
   bmih.biCompression = 0;
   bmih.biSizeImage = height*width*4;
   bmih.biXPelsPerMeter = 0;
   bmih.biYPelsPerMeter = 0;
   bmih.biClrUsed = 0;
            stream = fopen(filename, "wb");
   if (!stream)
            fwrite(&bmfh, 14, 1, stream);
            y = height;
   while (y--) {
      float *ptr = rgba + (stride * y * 4);
   for (x = 0; x < width; ++x) {
      struct bmp_rgb_quad pixel;
   pixel.rgbRed   = float_to_ubyte(ptr[x*4 + 0]);
   pixel.rgbGreen = float_to_ubyte(ptr[x*4 + 1]);
   pixel.rgbBlue  = float_to_ubyte(ptr[x*4 + 2]);
   pixel.rgbAlpha = float_to_ubyte(ptr[x*4 + 3]);
                     error1:
         }
      void
   debug_dump_ubyte_rgba_bmp(const char *filename,
               {
      FILE *stream;
   struct bmp_file_header bmfh;
   struct bmp_info_header bmih;
            assert(rgba);
   if (!rgba)
            bmfh.bfType = 0x4d42;
   bmfh.bfSize = 14 + 40 + height*width*4;
   bmfh.bfReserved1 = 0;
   bmfh.bfReserved2 = 0;
            bmih.biSize = 40;
   bmih.biWidth = width;
   bmih.biHeight = height;
   bmih.biPlanes = 1;
   bmih.biBitCount = 32;
   bmih.biCompression = 0;
   bmih.biSizeImage = height*width*4;
   bmih.biXPelsPerMeter = 0;
   bmih.biYPelsPerMeter = 0;
   bmih.biClrUsed = 0;
            stream = fopen(filename, "wb");
   assert(stream);
   if (!stream)
            fwrite(&bmfh, 14, 1, stream);
            y = height;
   while (y--) {
      const uint8_t *ptr = rgba + (stride * y * 4);
   for (x = 0; x < width; ++x) {
      struct bmp_rgb_quad pixel;
   pixel.rgbRed   = ptr[x*4 + 0];
   pixel.rgbGreen = ptr[x*4 + 1];
   pixel.rgbBlue  = ptr[x*4 + 2];
   pixel.rgbAlpha = ptr[x*4 + 3];
                     error1:
         }
      #endif
