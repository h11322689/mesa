   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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
   * \file pixel.c
   * Pixel transfer functions (glPixelZoom, glPixelMap, glPixelTransfer)
   */
      #include "util/glheader.h"
   #include "bufferobj.h"
   #include "context.h"
   #include "macros.h"
   #include "pixel.h"
   #include "pbo.h"
   #include "mtypes.h"
   #include "api_exec_decl.h"
      #include <math.h>
      /**********************************************************************/
   /*****                    glPixelZoom                             *****/
   /**********************************************************************/
      void GLAPIENTRY
   _mesa_PixelZoom( GLfloat xfactor, GLfloat yfactor )
   {
               if (ctx->Pixel.ZoomX == xfactor &&
      ctx->Pixel.ZoomY == yfactor)
         FLUSH_VERTICES(ctx, _NEW_PIXEL, GL_PIXEL_MODE_BIT);
   ctx->Pixel.ZoomX = xfactor;
      }
            /**********************************************************************/
   /*****                         glPixelMap                         *****/
   /**********************************************************************/
      /**
   * Return pointer to a pixelmap by name.
   */
   static struct gl_pixelmap *
   get_pixelmap(struct gl_context *ctx, GLenum map)
   {
      switch (map) {
   case GL_PIXEL_MAP_I_TO_I:
         case GL_PIXEL_MAP_S_TO_S:
         case GL_PIXEL_MAP_I_TO_R:
         case GL_PIXEL_MAP_I_TO_G:
         case GL_PIXEL_MAP_I_TO_B:
         case GL_PIXEL_MAP_I_TO_A:
         case GL_PIXEL_MAP_R_TO_R:
         case GL_PIXEL_MAP_G_TO_G:
         case GL_PIXEL_MAP_B_TO_B:
         case GL_PIXEL_MAP_A_TO_A:
         default:
            }
         /**
   * Helper routine used by the other _mesa_PixelMap() functions.
   */
   static void
   store_pixelmap(struct gl_context *ctx, GLenum map, GLsizei mapsize,
         {
      GLint i;
   struct gl_pixelmap *pm = get_pixelmap(ctx, map);
   if (!pm) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glPixelMap(map)");
               switch (map) {
   case GL_PIXEL_MAP_S_TO_S:
      /* special case */
   ctx->PixelMaps.StoS.Size = mapsize;
   for (i = 0; i < mapsize; i++) {
         }
      case GL_PIXEL_MAP_I_TO_I:
      /* special case */
   ctx->PixelMaps.ItoI.Size = mapsize;
   for (i = 0; i < mapsize; i++) {
         }
      default:
      /* general case */
   pm->Size = mapsize;
   for (i = 0; i < mapsize; i++) {
      GLfloat val = CLAMP(values[i], 0.0F, 1.0F);
            }
         /**
   * Convenience wrapper for _mesa_validate_pbo_access() for gl[Get]PixelMap().
   */
   static GLboolean
   validate_pbo_access(struct gl_context *ctx,
                     {
               /* Note, need to use DefaultPacking and Unpack's buffer object */
   _mesa_reference_buffer_object(ctx,
                  ok = _mesa_validate_pbo_access(1, &ctx->DefaultPacking, mapsize, 1, 1,
            /* restore */
   _mesa_reference_buffer_object(ctx,
            if (!ok) {
      if (pack->BufferObj) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
      } else {
      _mesa_error(ctx, GL_INVALID_OPERATION,
               }
      }
         void GLAPIENTRY
   _mesa_PixelMapfv( GLenum map, GLsizei mapsize, const GLfloat *values )
   {
               /* XXX someday, test against ctx->Const.MaxPixelMapTableSize */
   if (mapsize < 1 || mapsize > MAX_PIXEL_MAP_TABLE) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glPixelMapfv(mapsize)" );
               if (map >= GL_PIXEL_MAP_S_TO_S && map <= GL_PIXEL_MAP_I_TO_A) {
      /* test that mapsize is a power of two */
   _mesa_error( ctx, GL_INVALID_VALUE, "glPixelMapfv(mapsize)" );
                                    if (!validate_pbo_access(ctx, &ctx->Unpack, mapsize, GL_INTENSITY,
                        values = (const GLfloat *) _mesa_map_pbo_source(ctx, &ctx->Unpack, values);
   if (!values) {
      if (ctx->Unpack.BufferObj) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
      }
                           }
         void GLAPIENTRY
   _mesa_PixelMapuiv(GLenum map, GLsizei mapsize, const GLuint *values )
   {
      GLfloat fvalues[MAX_PIXEL_MAP_TABLE];
            if (mapsize < 1 || mapsize > MAX_PIXEL_MAP_TABLE) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glPixelMapuiv(mapsize)" );
               if (map >= GL_PIXEL_MAP_S_TO_S && map <= GL_PIXEL_MAP_I_TO_A) {
      /* test that mapsize is a power of two */
   _mesa_error( ctx, GL_INVALID_VALUE, "glPixelMapuiv(mapsize)" );
                                    if (!validate_pbo_access(ctx, &ctx->Unpack, mapsize, GL_INTENSITY,
                        values = (const GLuint *) _mesa_map_pbo_source(ctx, &ctx->Unpack, values);
   if (!values) {
      if (ctx->Unpack.BufferObj) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
      }
               /* convert to floats */
   if (map == GL_PIXEL_MAP_I_TO_I || map == GL_PIXEL_MAP_S_TO_S) {
      GLint i;
   for (i = 0; i < mapsize; i++) {
            }
   else {
      GLint i;
   for (i = 0; i < mapsize; i++) {
                                 }
         void GLAPIENTRY
   _mesa_PixelMapusv(GLenum map, GLsizei mapsize, const GLushort *values )
   {
      GLfloat fvalues[MAX_PIXEL_MAP_TABLE];
            if (mapsize < 1 || mapsize > MAX_PIXEL_MAP_TABLE) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glPixelMapusv(mapsize)" );
               if (map >= GL_PIXEL_MAP_S_TO_S && map <= GL_PIXEL_MAP_I_TO_A) {
      /* test that mapsize is a power of two */
   _mesa_error( ctx, GL_INVALID_VALUE, "glPixelMapusv(mapsize)" );
                                    if (!validate_pbo_access(ctx, &ctx->Unpack, mapsize, GL_INTENSITY,
                        values = (const GLushort *) _mesa_map_pbo_source(ctx, &ctx->Unpack, values);
   if (!values) {
      if (ctx->Unpack.BufferObj) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
      }
               /* convert to floats */
   if (map == GL_PIXEL_MAP_I_TO_I || map == GL_PIXEL_MAP_S_TO_S) {
      GLint i;
   for (i = 0; i < mapsize; i++) {
            }
   else {
      GLint i;
   for (i = 0; i < mapsize; i++) {
                                 }
         void GLAPIENTRY
   _mesa_GetnPixelMapfvARB( GLenum map, GLsizei bufSize, GLfloat *values )
   {
      GET_CURRENT_CONTEXT(ctx);
   GLint mapsize, i;
            pm = get_pixelmap(ctx, map);
   if (!pm) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetPixelMapfv(map)");
                        if (!validate_pbo_access(ctx, &ctx->Pack, mapsize, GL_INTENSITY,
                        if (ctx->Pack.BufferObj)
            values = (GLfloat *) _mesa_map_pbo_dest(ctx, &ctx->Pack, values);
   if (!values) {
      if (ctx->Pack.BufferObj) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
      }
               if (map == GL_PIXEL_MAP_S_TO_S) {
      /* special case */
   for (i = 0; i < mapsize; i++) {
            }
   else {
                     }
         void GLAPIENTRY
   _mesa_GetPixelMapfv( GLenum map, GLfloat *values )
   {
         }
      void GLAPIENTRY
   _mesa_GetnPixelMapuivARB( GLenum map, GLsizei bufSize, GLuint *values )
   {
      GET_CURRENT_CONTEXT(ctx);
   GLint mapsize, i;
            pm = get_pixelmap(ctx, map);
   if (!pm) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetPixelMapuiv(map)");
                        if (!validate_pbo_access(ctx, &ctx->Pack, mapsize, GL_INTENSITY,
                        if (ctx->Pack.BufferObj)
            values = (GLuint *) _mesa_map_pbo_dest(ctx, &ctx->Pack, values);
   if (!values) {
      if (ctx->Pack.BufferObj) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
      }
               if (map == GL_PIXEL_MAP_S_TO_S) {
      /* special case */
      }
   else {
      for (i = 0; i < mapsize; i++) {
                        }
         void GLAPIENTRY
   _mesa_GetPixelMapuiv( GLenum map, GLuint *values )
   {
         }
      void GLAPIENTRY
   _mesa_GetnPixelMapusvARB( GLenum map, GLsizei bufSize, GLushort *values )
   {
      GET_CURRENT_CONTEXT(ctx);
   GLint mapsize, i;
            pm = get_pixelmap(ctx, map);
   if (!pm) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetPixelMapusv(map)");
                        if (!validate_pbo_access(ctx, &ctx->Pack, mapsize, GL_INTENSITY,
                        if (ctx->Pack.BufferObj)
            values = (GLushort *) _mesa_map_pbo_dest(ctx, &ctx->Pack, values);
   if (!values) {
      if (ctx->Pack.BufferObj) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
      }
               switch (map) {
   /* special cases */
   case GL_PIXEL_MAP_I_TO_I:
      for (i = 0; i < mapsize; i++) {
         }
      case GL_PIXEL_MAP_S_TO_S:
      for (i = 0; i < mapsize; i++) {
         }
      default:
      for (i = 0; i < mapsize; i++) {
                        }
         void GLAPIENTRY
   _mesa_GetPixelMapusv( GLenum map, GLushort *values )
   {
         }
         /**********************************************************************/
   /*****                       glPixelTransfer                      *****/
   /**********************************************************************/
         /*
   * Implements glPixelTransfer[fi] whether called immediately or from a
   * display list.
   */
   void GLAPIENTRY
   _mesa_PixelTransferf( GLenum pname, GLfloat param )
   {
               switch (pname) {
      case GL_MAP_COLOR:
         FLUSH_VERTICES(ctx, _NEW_PIXEL, GL_PIXEL_MODE_BIT);
         break;
         case GL_MAP_STENCIL:
         FLUSH_VERTICES(ctx, _NEW_PIXEL, GL_PIXEL_MODE_BIT);
         break;
         case GL_INDEX_SHIFT:
         FLUSH_VERTICES(ctx, _NEW_PIXEL, GL_PIXEL_MODE_BIT);
         break;
         case GL_INDEX_OFFSET:
         FLUSH_VERTICES(ctx, _NEW_PIXEL, GL_PIXEL_MODE_BIT);
         break;
         case GL_RED_SCALE:
         FLUSH_VERTICES(ctx, _NEW_PIXEL, GL_PIXEL_MODE_BIT);
         break;
         case GL_RED_BIAS:
         FLUSH_VERTICES(ctx, _NEW_PIXEL, GL_PIXEL_MODE_BIT);
         break;
         case GL_GREEN_SCALE:
         FLUSH_VERTICES(ctx, _NEW_PIXEL, GL_PIXEL_MODE_BIT);
         break;
         case GL_GREEN_BIAS:
         FLUSH_VERTICES(ctx, _NEW_PIXEL, GL_PIXEL_MODE_BIT);
         break;
         case GL_BLUE_SCALE:
         FLUSH_VERTICES(ctx, _NEW_PIXEL, GL_PIXEL_MODE_BIT);
         break;
         case GL_BLUE_BIAS:
         FLUSH_VERTICES(ctx, _NEW_PIXEL, GL_PIXEL_MODE_BIT);
         break;
         case GL_ALPHA_SCALE:
         FLUSH_VERTICES(ctx, _NEW_PIXEL, GL_PIXEL_MODE_BIT);
         break;
         case GL_ALPHA_BIAS:
         FLUSH_VERTICES(ctx, _NEW_PIXEL, GL_PIXEL_MODE_BIT);
         break;
         case GL_DEPTH_SCALE:
         FLUSH_VERTICES(ctx, _NEW_PIXEL, GL_PIXEL_MODE_BIT);
         break;
         case GL_DEPTH_BIAS:
         FLUSH_VERTICES(ctx, _NEW_PIXEL, GL_PIXEL_MODE_BIT);
         break;
         default:
      _mesa_error( ctx, GL_INVALID_ENUM, "glPixelTransfer(pname)" );
      }
         void GLAPIENTRY
   _mesa_PixelTransferi( GLenum pname, GLint param )
   {
         }
            /**********************************************************************/
   /*****                    State Management                        *****/
   /**********************************************************************/
         /**
   * Update mesa pixel transfer derived state to indicate which operations are
   * enabled.
   */
   void
   _mesa_update_pixel( struct gl_context *ctx )
   {
               if (ctx->Pixel.RedScale   != 1.0F || ctx->Pixel.RedBias   != 0.0F ||
      ctx->Pixel.GreenScale != 1.0F || ctx->Pixel.GreenBias != 0.0F ||
   ctx->Pixel.BlueScale  != 1.0F || ctx->Pixel.BlueBias  != 0.0F ||
   ctx->Pixel.AlphaScale != 1.0F || ctx->Pixel.AlphaBias != 0.0F)
         if (ctx->Pixel.IndexShift || ctx->Pixel.IndexOffset)
            if (ctx->Pixel.MapColorFlag)
               }
         /**********************************************************************/
   /*****                      Initialization                        *****/
   /**********************************************************************/
      static void
   init_pixelmap(struct gl_pixelmap *map)
   {
      map->Size = 1;
      }
         /**
   * Initialize the context's PIXEL attribute group.
   */
   void
   _mesa_init_pixel( struct gl_context *ctx )
   {
      /* Pixel group */
   ctx->Pixel.RedBias = 0.0;
   ctx->Pixel.RedScale = 1.0;
   ctx->Pixel.GreenBias = 0.0;
   ctx->Pixel.GreenScale = 1.0;
   ctx->Pixel.BlueBias = 0.0;
   ctx->Pixel.BlueScale = 1.0;
   ctx->Pixel.AlphaBias = 0.0;
   ctx->Pixel.AlphaScale = 1.0;
   ctx->Pixel.DepthBias = 0.0;
   ctx->Pixel.DepthScale = 1.0;
   ctx->Pixel.IndexOffset = 0;
   ctx->Pixel.IndexShift = 0;
   ctx->Pixel.ZoomX = 1.0;
   ctx->Pixel.ZoomY = 1.0;
   ctx->Pixel.MapColorFlag = GL_FALSE;
   ctx->Pixel.MapStencilFlag = GL_FALSE;
   init_pixelmap(&ctx->PixelMaps.StoS);
   init_pixelmap(&ctx->PixelMaps.ItoI);
   init_pixelmap(&ctx->PixelMaps.ItoR);
   init_pixelmap(&ctx->PixelMaps.ItoG);
   init_pixelmap(&ctx->PixelMaps.ItoB);
   init_pixelmap(&ctx->PixelMaps.ItoA);
   init_pixelmap(&ctx->PixelMaps.RtoR);
   init_pixelmap(&ctx->PixelMaps.GtoG);
   init_pixelmap(&ctx->PixelMaps.BtoB);
            if (ctx->Visual.doubleBufferMode) {
         }
   else {
                  /* Miscellaneous */
      }
