   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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
         #include "util/glheader.h"
      #include "context.h"
   #include "depth.h"
   #include "enums.h"
   #include "macros.h"
   #include "mtypes.h"
   #include "state.h"
   #include "api_exec_decl.h"
      #include "state_tracker/st_context.h"
      /**********************************************************************/
   /*****                          API Functions                     *****/
   /**********************************************************************/
            void GLAPIENTRY
   _mesa_ClearDepth( GLclampd depth )
   {
               if (MESA_VERBOSE & VERBOSE_API)
            ctx->PopAttribState |= GL_DEPTH_BUFFER_BIT;
      }
         void GLAPIENTRY
   _mesa_ClearDepthf( GLclampf depth )
   {
         }
         static ALWAYS_INLINE void
   depth_func(struct gl_context *ctx, GLenum func, bool no_error)
   {
      if (ctx->Depth.Func == func)
            if (!no_error) {
      switch (func) {
   case GL_LESS:    /* (default) pass if incoming z < stored z */
   case GL_GEQUAL:
   case GL_LEQUAL:
   case GL_GREATER:
   case GL_NOTEQUAL:
   case GL_EQUAL:
   case GL_ALWAYS:
   case GL_NEVER:
         default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glDepth.Func");
                  FLUSH_VERTICES(ctx, 0, GL_DEPTH_BUFFER_BIT);
   ctx->NewDriverState |= ST_NEW_DSA;
   ctx->Depth.Func = func;
      }
         void GLAPIENTRY
   _mesa_DepthFunc_no_error(GLenum func)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_DepthFunc(GLenum func)
   {
               if (MESA_VERBOSE & VERBOSE_API)
               }
            void GLAPIENTRY
   _mesa_DepthMask( GLboolean flag )
   {
               if (MESA_VERBOSE & VERBOSE_API)
            /*
   * GL_TRUE indicates depth buffer writing is enabled (default)
   * GL_FALSE indicates depth buffer writing is disabled
   */
   if (ctx->Depth.Mask == flag)
            FLUSH_VERTICES(ctx, 0, GL_DEPTH_BUFFER_BIT);
   ctx->NewDriverState |= ST_NEW_DSA;
   ctx->Depth.Mask = flag;
      }
            /**
   * Specified by the GL_EXT_depth_bounds_test extension.
   */
   void GLAPIENTRY
   _mesa_DepthBoundsEXT( GLclampd zmin, GLclampd zmax )
   {
               if (MESA_VERBOSE & VERBOSE_API)
            if (zmin > zmax) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glDepthBoundsEXT(zmin > zmax)");
               zmin = SATURATE(zmin);
            if (ctx->Depth.BoundsMin == zmin && ctx->Depth.BoundsMax == zmax)
            FLUSH_VERTICES(ctx, 0, GL_DEPTH_BUFFER_BIT);
   ctx->NewDriverState |= ST_NEW_DSA;
   ctx->Depth.BoundsMin = zmin;
      }
         /**********************************************************************/
   /*****                      Initialization                        *****/
   /**********************************************************************/
         /**
   * Initialize the depth buffer attribute group in the given context.
   */
   void
   _mesa_init_depth(struct gl_context *ctx)
   {
      ctx->Depth.Test = GL_FALSE;
   ctx->Depth.Clear = 1.0;
   ctx->Depth.Func = GL_LESS;
      }
