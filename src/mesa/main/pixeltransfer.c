   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
   * Copyright (C) 2009-2010  VMware, Inc.  All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included
   * in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   * OTHER DEALINGS IN THE SOFTWARE.
   */
         /**
   * \file pixeltransfer.c
   * Pixel transfer operations (scale, bias, table lookups, etc)
   */
         #include "util/glheader.h"
   #include "macros.h"
   #include "pixeltransfer.h"
      #include "mtypes.h"
   #include "util/rounding.h"
         /*
   * Apply scale and bias factors to an array of RGBA pixels.
   */
   void
   _mesa_scale_and_bias_rgba(GLuint n, GLfloat rgba[][4],
                           {
      if (rScale != 1.0F || rBias != 0.0F) {
      GLuint i;
   for (i = 0; i < n; i++) {
            }
   if (gScale != 1.0F || gBias != 0.0F) {
      GLuint i;
   for (i = 0; i < n; i++) {
            }
   if (bScale != 1.0F || bBias != 0.0F) {
      GLuint i;
   for (i = 0; i < n; i++) {
            }
   if (aScale != 1.0F || aBias != 0.0F) {
      GLuint i;
   for (i = 0; i < n; i++) {
               }
         /*
   * Apply pixel mapping to an array of floating point RGBA pixels.
   */
   void
   _mesa_map_rgba( const struct gl_context *ctx, GLuint n, GLfloat rgba[][4] )
   {
      const GLfloat rscale = (GLfloat) (ctx->PixelMaps.RtoR.Size - 1);
   const GLfloat gscale = (GLfloat) (ctx->PixelMaps.GtoG.Size - 1);
   const GLfloat bscale = (GLfloat) (ctx->PixelMaps.BtoB.Size - 1);
   const GLfloat ascale = (GLfloat) (ctx->PixelMaps.AtoA.Size - 1);
   const GLfloat *rMap = ctx->PixelMaps.RtoR.Map;
   const GLfloat *gMap = ctx->PixelMaps.GtoG.Map;
   const GLfloat *bMap = ctx->PixelMaps.BtoB.Map;
   const GLfloat *aMap = ctx->PixelMaps.AtoA.Map;
   GLuint i;
   for (i=0;i<n;i++) {
      GLfloat r = CLAMP(rgba[i][RCOMP], 0.0F, 1.0F);
   GLfloat g = CLAMP(rgba[i][GCOMP], 0.0F, 1.0F);
   GLfloat b = CLAMP(rgba[i][BCOMP], 0.0F, 1.0F);
   GLfloat a = CLAMP(rgba[i][ACOMP], 0.0F, 1.0F);
   rgba[i][RCOMP] = rMap[(int)_mesa_lroundevenf(r * rscale)];
   rgba[i][GCOMP] = gMap[(int)_mesa_lroundevenf(g * gscale)];
   rgba[i][BCOMP] = bMap[(int)_mesa_lroundevenf(b * bscale)];
         }
      /*
   * Map color indexes to float rgba values.
   */
   void
   _mesa_map_ci_to_rgba( const struct gl_context *ctx, GLuint n,
         {
      GLuint rmask = ctx->PixelMaps.ItoR.Size - 1;
   GLuint gmask = ctx->PixelMaps.ItoG.Size - 1;
   GLuint bmask = ctx->PixelMaps.ItoB.Size - 1;
   GLuint amask = ctx->PixelMaps.ItoA.Size - 1;
   const GLfloat *rMap = ctx->PixelMaps.ItoR.Map;
   const GLfloat *gMap = ctx->PixelMaps.ItoG.Map;
   const GLfloat *bMap = ctx->PixelMaps.ItoB.Map;
   const GLfloat *aMap = ctx->PixelMaps.ItoA.Map;
   GLuint i;
   for (i=0;i<n;i++) {
      rgba[i][RCOMP] = rMap[index[i] & rmask];
   rgba[i][GCOMP] = gMap[index[i] & gmask];
   rgba[i][BCOMP] = bMap[index[i] & bmask];
         }
         void
   _mesa_scale_and_bias_depth(const struct gl_context *ctx, GLuint n,
         {
      const GLfloat scale = ctx->Pixel.DepthScale;
   const GLfloat bias = ctx->Pixel.DepthBias;
   GLuint i;
   for (i = 0; i < n; i++) {
      GLfloat d = depthValues[i] * scale + bias;
         }
         void
   _mesa_scale_and_bias_depth_uint(const struct gl_context *ctx, GLuint n,
         {
      const GLdouble max = (double) 0xffffffff;
   const GLdouble scale = ctx->Pixel.DepthScale;
   const GLdouble bias = ctx->Pixel.DepthBias * max;
   GLuint i;
   for (i = 0; i < n; i++) {
      GLdouble d = (GLdouble) depthValues[i] * scale + bias;
   d = CLAMP(d, 0.0, max);
         }
      /**
   * Apply various pixel transfer operations to an array of RGBA pixels
   * as indicated by the transferOps bitmask
   */
   void
   _mesa_apply_rgba_transfer_ops(struct gl_context *ctx, GLbitfield transferOps,
         {
      /* scale & bias */
   if (transferOps & IMAGE_SCALE_BIAS_BIT) {
      _mesa_scale_and_bias_rgba(n, rgba,
                        }
   /* color map lookup */
   if (transferOps & IMAGE_MAP_COLOR_BIT) {
                  /* clamping to [0,1] */
   if (transferOps & IMAGE_CLAMP_BIT) {
      GLuint i;
   for (i = 0; i < n; i++) {
      rgba[i][RCOMP] = CLAMP(rgba[i][RCOMP], 0.0F, 1.0F);
   rgba[i][GCOMP] = CLAMP(rgba[i][GCOMP], 0.0F, 1.0F);
   rgba[i][BCOMP] = CLAMP(rgba[i][BCOMP], 0.0F, 1.0F);
            }
         /*
   * Apply color index shift and offset to an array of pixels.
   */
   void
   _mesa_shift_and_offset_ci(const struct gl_context *ctx,
         {
      GLint shift = ctx->Pixel.IndexShift;
   GLint offset = ctx->Pixel.IndexOffset;
   GLuint i;
   if (shift > 0) {
      for (i=0;i<n;i++) {
            }
   else if (shift < 0) {
      shift = -shift;
   for (i=0;i<n;i++) {
            }
   else {
      for (i=0;i<n;i++) {
               }
            /**
   * Apply stencil index shift, offset and table lookup to an array
   * of stencil values.
   */
   void
   _mesa_apply_stencil_transfer_ops(const struct gl_context *ctx, GLuint n,
         {
      if (ctx->Pixel.IndexShift != 0 || ctx->Pixel.IndexOffset != 0) {
      const GLint offset = ctx->Pixel.IndexOffset;
   GLint shift = ctx->Pixel.IndexShift;
   GLuint i;
   if (shift > 0) {
      for (i = 0; i < n; i++) {
            }
   else if (shift < 0) {
      shift = -shift;
   for (i = 0; i < n; i++) {
            }
   else {
      for (i = 0; i < n; i++) {
               }
   if (ctx->Pixel.MapStencilFlag) {
      GLuint mask = ctx->PixelMaps.StoS.Size - 1;
   GLuint i;
   for (i = 0; i < n; i++) {
               }
