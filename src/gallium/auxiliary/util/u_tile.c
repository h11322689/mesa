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
      /**
   * RGBA/float tile get/put functions.
   * Usable both by drivers and gallium frontends.
   */
         #include "pipe/p_defines.h"
   #include "util/u_inlines.h"
      #include "util/format/u_format.h"
   #include "util/format/u_format_bptc.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/u_surface.h"
   #include "util/u_tile.h"
         /**
   * Move raw block of pixels from transfer object to user memory.
   */
   void
   pipe_get_tile_raw(struct pipe_transfer *pt,
                     const void *src,
   {
      if (dst_stride == 0)
            if (u_clip_tile(x, y, &w, &h, &pt->box))
               }
         /**
   * Move raw block of pixels from user memory to transfer object.
   */
   void
   pipe_put_tile_raw(struct pipe_transfer *pt,
                     void *dst,
   {
               if (src_stride == 0)
            if (u_clip_tile(x, y, &w, &h, &pt->box))
               }
               /** Convert short in [-32768,32767] to GLfloat in [-1.0,1.0] */
   #define SHORT_TO_FLOAT(S)   ((2.0F * (S) + 1.0F) * (1.0F/65535.0F))
      #define UNCLAMPED_FLOAT_TO_SHORT(us, f)  \
                  /*** PIPE_FORMAT_Z16_UNORM ***/
      /**
   * Return each Z value as four floats in [0,1].
   */
   static void
   z16_get_tile_rgba(const uint16_t *src,
                     {
      const float scale = 1.0f / 65535.0f;
            for (i = 0; i < h; i++) {
      float *pRow = p;
   for (j = 0; j < w; j++, pRow += 4) {
      pRow[0] =
   pRow[1] =
   pRow[2] =
      }
         }
               /*** PIPE_FORMAT_Z32_UNORM ***/
      /**
   * Return each Z value as four floats in [0,1].
   */
   static void
   z32_get_tile_rgba(const uint32_t *src,
                     {
      const double scale = 1.0 / (double) 0xffffffff;
            for (i = 0; i < h; i++) {
      float *pRow = p;
   for (j = 0; j < w; j++, pRow += 4) {
      pRow[0] =
   pRow[1] =
   pRow[2] =
      }
         }
         /*** PIPE_FORMAT_Z24_UNORM_S8_UINT ***/
      /**
   * Return Z component as four float in [0,1].  Stencil part ignored.
   */
   static void
   s8z24_get_tile_rgba(const uint32_t *src,
                     {
      const double scale = 1.0 / ((1 << 24) - 1);
            for (i = 0; i < h; i++) {
      float *pRow = p;
   for (j = 0; j < w; j++, pRow += 4) {
      pRow[0] =
   pRow[1] =
   pRow[2] =
      }
         }
         /*** PIPE_FORMAT_S8_UINT_Z24_UNORM ***/
      /**
   * Return Z component as four float in [0,1].  Stencil part ignored.
   */
   static void
   z24s8_get_tile_rgba(const uint32_t *src,
                     {
      const double scale = 1.0 / ((1 << 24) - 1);
            for (i = 0; i < h; i++) {
      float *pRow = p;
   for (j = 0; j < w; j++, pRow += 4) {
      pRow[0] =
   pRow[1] =
   pRow[2] =
      }
         }
      /*** PIPE_FORMAT_S8X24_UINT ***/
      /**
   * Return S component as four uint32_t in [0..255].  Z part ignored.
   */
   static void
   s8x24_get_tile_rgba(const uint32_t *src,
                     {
               for (i = 0; i < h; i++) {
               for (j = 0; j < w; j++, pRow += 4) {
      pRow[0] =
   pRow[1] =
   pRow[2] =
                     }
      /*** PIPE_FORMAT_X24S8_UINT ***/
      /**
   * Return S component as four uint32_t in [0..255].  Z part ignored.
   */
   static void
   x24s8_get_tile_rgba(const uint32_t *src,
                     {
               for (i = 0; i < h; i++) {
      uint32_t *pRow = (uint32_t *)p;
   for (j = 0; j < w; j++, pRow += 4) {
      pRow[0] =
   pRow[1] =
   pRow[2] =
      }
         }
         /**
   * Return S component as four uint32_t in [0..255].  Z part ignored.
   */
   static void
   s8_get_tile_rgba(const unsigned char *src,
      unsigned w, unsigned h,
   float *p,
      {
               for (i = 0; i < h; i++) {
      uint32_t *pRow = (uint32_t *)p;
   for (j = 0; j < w; j++, pRow += 4) {
      pRow[0] =
   pRow[1] =
   pRow[2] =
      }
         }
      /*** PIPE_FORMAT_Z32_FLOAT ***/
      /**
   * Return each Z value as four floats in [0,1].
   */
   static void
   z32f_get_tile_rgba(const float *src,
                     {
               for (i = 0; i < h; i++) {
      float *pRow = p;
   for (j = 0; j < w; j++, pRow += 4) {
      pRow[0] =
   pRow[1] =
   pRow[2] =
      }
         }
      /*** PIPE_FORMAT_Z32_FLOAT_S8X24_UINT ***/
      /**
   * Return each Z value as four floats in [0,1].
   */
   static void
   z32f_x24s8_get_tile_rgba(const float *src,
                     {
               for (i = 0; i < h; i++) {
      float *pRow = p;
   for (j = 0; j < w; j++, pRow += 4) {
      pRow[0] =
   pRow[1] =
   pRow[2] =
   pRow[3] = *src;
      }
         }
      /*** PIPE_FORMAT_X32_S8X24_UINT ***/
      /**
   * Return S component as four uint32_t in [0..255].  Z part ignored.
   */
   static void
   x32_s8_get_tile_rgba(const uint32_t *src,
                     {
               for (i = 0; i < h; i++) {
      uint32_t *pRow = (uint32_t *)p;
   for (j = 0; j < w; j++, pRow += 4) {
      src++;
   pRow[0] =
   pRow[1] =
   pRow[2] =
      }
         }
      void
   pipe_put_tile_rgba(struct pipe_transfer *pt,
                     {
               if (u_clip_tile(x, y, &w, &h, &pt->box))
            /* While we do generate RGBA tiles for z/s for softpipe's texture fetch
   * path, we never have to store from "RGBA" to Z/S.
   */
   if (util_format_is_depth_or_stencil(format))
            util_format_write_4(format,
                  }
      void
   pipe_get_tile_rgba(struct pipe_transfer *pt,
                     const void *src,
   {
      unsigned dst_stride = w * 4;
            if (u_clip_tile(x, y, &w, &h, &pt->box)) {
                  packed = MALLOC(util_format_get_nblocks(format, w, h) * util_format_get_blocksize(format));
   if (!packed) {
                  if (format == PIPE_FORMAT_UYVY || format == PIPE_FORMAT_YUYV) {
                           switch (format) {
   case PIPE_FORMAT_Z16_UNORM:
      z16_get_tile_rgba((uint16_t *) packed, w, h, dst, dst_stride);
      case PIPE_FORMAT_Z32_UNORM:
      z32_get_tile_rgba((uint32_t *) packed, w, h, dst, dst_stride);
      case PIPE_FORMAT_Z24_UNORM_S8_UINT:
   case PIPE_FORMAT_Z24X8_UNORM:
      s8z24_get_tile_rgba((uint32_t *) packed, w, h, dst, dst_stride);
      case PIPE_FORMAT_S8_UINT:
      s8_get_tile_rgba((uint8_t *) packed, w, h, dst, dst_stride);
      case PIPE_FORMAT_X24S8_UINT:
      s8x24_get_tile_rgba((uint32_t *) packed, w, h, dst, dst_stride);
      case PIPE_FORMAT_S8_UINT_Z24_UNORM:
   case PIPE_FORMAT_X8Z24_UNORM:
      z24s8_get_tile_rgba((uint32_t *) packed, w, h, dst, dst_stride);
      case PIPE_FORMAT_S8X24_UINT:
      x24s8_get_tile_rgba((uint32_t *) packed, w, h, dst, dst_stride);
      case PIPE_FORMAT_Z32_FLOAT:
      z32f_get_tile_rgba((float *) packed, w, h, dst, dst_stride);
      case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
      z32f_x24s8_get_tile_rgba((float *) packed, w, h, dst, dst_stride);
      case PIPE_FORMAT_X32_S8X24_UINT:
      x32_s8_get_tile_rgba((uint32_t *) packed, w, h, dst, dst_stride);
      default:
      util_format_read_4(format,
                              }
