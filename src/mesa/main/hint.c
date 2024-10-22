      /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 1999-2002  Brian Paul   All Rights Reserved.
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
   #include "enums.h"
   #include "context.h"
   #include "hint.h"
      #include "mtypes.h"
   #include "api_exec_decl.h"
      #include "pipe/p_screen.h"
      void GLAPIENTRY
   _mesa_Hint( GLenum target, GLenum mode )
   {
               if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glHint %s %s\n",
               if (mode != GL_NICEST && mode != GL_FASTEST && mode != GL_DONT_CARE) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glHint(mode)");
               switch (target) {
      case GL_FOG_HINT:
      if (ctx->API != API_OPENGL_COMPAT && ctx->API != API_OPENGLES)
            FLUSH_VERTICES(ctx, _NEW_HINT, GL_HINT_BIT);
            ctx->Hint.Fog = mode;
      case GL_LINE_SMOOTH_HINT:
      if (!_mesa_is_desktop_gl(ctx) && ctx->API != API_OPENGLES)
            FLUSH_VERTICES(ctx, _NEW_HINT, GL_HINT_BIT);
            ctx->Hint.LineSmooth = mode;
      case GL_PERSPECTIVE_CORRECTION_HINT:
      if (ctx->API != API_OPENGL_COMPAT && ctx->API != API_OPENGLES)
            FLUSH_VERTICES(ctx, _NEW_HINT, GL_HINT_BIT);
            ctx->Hint.PerspectiveCorrection = mode;
      case GL_POINT_SMOOTH_HINT:
      if (ctx->API != API_OPENGL_COMPAT && ctx->API != API_OPENGLES)
            FLUSH_VERTICES(ctx, _NEW_HINT, GL_HINT_BIT);
            ctx->Hint.PointSmooth = mode;
      case GL_POLYGON_SMOOTH_HINT:
      if (!_mesa_is_desktop_gl(ctx))
            FLUSH_VERTICES(ctx, _NEW_HINT, GL_HINT_BIT);
                        /* GL_ARB_texture_compression */
   case GL_TEXTURE_COMPRESSION_HINT_ARB:
         if (ctx->Hint.TextureCompression == mode)
         FLUSH_VERTICES(ctx, _NEW_HINT, GL_HINT_BIT);
   ctx->Hint.TextureCompression = mode;
                  /* GL_SGIS_generate_mipmap */
   case GL_GENERATE_MIPMAP_HINT_SGIS:
      if (_mesa_is_desktop_gl_core(ctx))
            FLUSH_VERTICES(ctx, _NEW_HINT, GL_HINT_BIT);
   ctx->Hint.GenerateMipmap = mode;
                  /* GL_ARB_fragment_shader */
   case GL_FRAGMENT_SHADER_DERIVATIVE_HINT_ARB:
      if (_mesa_is_gles1(ctx) || !ctx->Extensions.ARB_fragment_shader)
         if (ctx->Hint.FragmentShaderDerivative == mode)
         FLUSH_VERTICES(ctx, _NEW_HINT, GL_HINT_BIT);
               default:
      }
         invalid_target:
      _mesa_error(ctx, GL_INVALID_ENUM, "glHint(target)");
      }
      /* GL_ARB_parallel_shader_compile */
   void GLAPIENTRY
   _mesa_MaxShaderCompilerThreadsKHR(GLuint count)
   {
                        struct pipe_screen *screen = ctx->screen;
   if (screen->set_max_shader_compiler_threads)
      }
      /**********************************************************************/
   /*****                      Initialization                        *****/
   /**********************************************************************/
      void _mesa_init_hint( struct gl_context * ctx )
   {
      /* Hint group */
   ctx->Hint.PerspectiveCorrection = GL_DONT_CARE;
   ctx->Hint.PointSmooth = GL_DONT_CARE;
   ctx->Hint.LineSmooth = GL_DONT_CARE;
   ctx->Hint.PolygonSmooth = GL_DONT_CARE;
   ctx->Hint.Fog = GL_DONT_CARE;
   ctx->Hint.TextureCompression = GL_DONT_CARE;
   ctx->Hint.GenerateMipmap = GL_DONT_CARE;
   ctx->Hint.FragmentShaderDerivative = GL_DONT_CARE;
      }
