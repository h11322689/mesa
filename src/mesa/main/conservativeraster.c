   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 2018 Rhys Perry
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
   * \file conservativeraster.c
   * glConservativeRasterParameteriNV and glConservativeRasterParameterfNV functions
   */
      #include "conservativeraster.h"
   #include "context.h"
   #include "enums.h"
   #include "api_exec_decl.h"
      #include "state_tracker/st_context.h"
      static ALWAYS_INLINE void
   conservative_raster_parameter(GLenum pname, GLfloat param,
         {
               if (!no_error && !ctx->Extensions.NV_conservative_raster_dilate &&
      !ctx->Extensions.NV_conservative_raster_pre_snap_triangles) {
   _mesa_error(ctx, GL_INVALID_OPERATION, "%s not supported", func);
               if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "%s(%s, %g)\n",
                  switch (pname) {
   case GL_CONSERVATIVE_RASTER_DILATE_NV:
      if (!no_error && !ctx->Extensions.NV_conservative_raster_dilate)
            if (!no_error && param<0.0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(param=%g)", func, param);
               FLUSH_VERTICES(ctx, 0, 0);
            ctx->ConservativeRasterDilate =
      CLAMP(param,
               case GL_CONSERVATIVE_RASTER_MODE_NV:
      if (!no_error && !ctx->Extensions.NV_conservative_raster_pre_snap_triangles)
            if (!no_error && param != GL_CONSERVATIVE_RASTER_MODE_POST_SNAP_NV &&
      param != GL_CONSERVATIVE_RASTER_MODE_PRE_SNAP_TRIANGLES_NV) {
   _mesa_error(ctx, GL_INVALID_ENUM,
                     FLUSH_VERTICES(ctx, 0, 0);
   ctx->NewDriverState |= ST_NEW_RASTERIZER;
   ctx->ConservativeRasterMode = param;
      default:
      goto invalid_pname_enum;
                  invalid_pname_enum:
      if (!no_error)
      _mesa_error(ctx, GL_INVALID_ENUM, "%s(pname=%s)",
   }
      void GLAPIENTRY
   _mesa_ConservativeRasterParameteriNV_no_error(GLenum pname, GLint param)
   {
      conservative_raster_parameter(pname, param, true, 
      }
      void GLAPIENTRY
   _mesa_ConservativeRasterParameteriNV(GLenum pname, GLint param)
   {
      conservative_raster_parameter(pname, param, false,
      }
      void GLAPIENTRY
   _mesa_ConservativeRasterParameterfNV_no_error(GLenum pname, GLfloat param)
   {
      conservative_raster_parameter(pname, param, true, 
      }
      void GLAPIENTRY
   _mesa_ConservativeRasterParameterfNV(GLenum pname, GLfloat param)
   {
      conservative_raster_parameter(pname, param, false, 
      }
      void
   _mesa_init_conservative_raster(struct gl_context *ctx)
   {
      ctx->ConservativeRasterDilate = 0.0;
      }
