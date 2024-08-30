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
         #include <stdbool.h>
   #include "util/glheader.h"
   #include "context.h"
   #include "debug_output.h"
   #include "get.h"
   #include "enums.h"
   #include "extensions.h"
   #include "mtypes.h"
   #include "macros.h"
   #include "version.h"
   #include "spirv_extensions.h"
   #include "api_exec_decl.h"
      #include "pipe/p_context.h"
   #include "pipe/p_screen.h"
      /**
   * Return the string for a glGetString(GL_SHADING_LANGUAGE_VERSION) query.
   */
   static const GLubyte *
   shading_language_version(struct gl_context *ctx)
   {
      switch (ctx->API) {
   case API_OPENGL_COMPAT:
   case API_OPENGL_CORE:
      switch (ctx->Const.GLSLVersion) {
   case 120:
         case 130:
         case 140:
         case 150:
         case 330:
         case 400:
         case 410:
         case 420:
         case 430:
         case 440:
         case 450:
         case 460:
         default:
      _mesa_problem(ctx,
            }
         case API_OPENGLES2:
      switch (ctx->Version) {
   case 20:
         case 30:
         case 31:
         case 32:
         default:
      _mesa_problem(ctx,
               case API_OPENGLES:
            default:
      _mesa_problem(ctx, "Unexpected API value in shading_language_version()");
         }
         /**
   * Query string-valued state.  The return value should _not_ be freed by
   * the caller.
   *
   * \param name  the state variable to query.
   *
   * \sa glGetString().
   *
   * Tries to get the string from dd_function_table::GetString, otherwise returns
   * the hardcoded strings.
   */
   const GLubyte * GLAPIENTRY
   _mesa_GetString( GLenum name )
   {
      GET_CURRENT_CONTEXT(ctx);
   static const char *vendor = "Brian Paul";
            if (!ctx)
                     if (ctx->Const.VendorOverride && name == GL_VENDOR) {
                  if (ctx->Const.RendererOverride && name == GL_RENDERER) {
                           switch (name) {
      case GL_VENDOR: {
      const GLubyte *str = (const GLubyte *)screen->get_vendor(screen);
   if (str)
            }
   case GL_RENDERER: {
      const GLubyte *str = (const GLubyte *)screen->get_name(screen);
   if (str)
            }
   case GL_VERSION:
         case GL_EXTENSIONS:
      if (_mesa_is_desktop_gl_core(ctx)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetString(GL_EXTENSIONS)");
      }
   if (!ctx->Extensions.String)
            case GL_SHADING_LANGUAGE_VERSION:
         return shading_language_version(ctx);
         case GL_PROGRAM_ERROR_STRING_ARB:
      if (_mesa_is_desktop_gl_compat(ctx) &&
      (ctx->Extensions.ARB_fragment_program ||
   ctx->Extensions.ARB_vertex_program)) {
      }
      default:
               _mesa_error( ctx, GL_INVALID_ENUM, "glGetString" );
      }
         /**
   * GL3
   */
   const GLubyte * GLAPIENTRY
   _mesa_GetStringi(GLenum name, GLuint index)
   {
               if (!ctx)
                     switch (name) {
   case GL_EXTENSIONS:
      if (index >= _mesa_get_extension_count(ctx)) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetStringi(index=%u)", index);
      }
      case GL_SHADING_LANGUAGE_VERSION:
      {
      char *version;
   int num;
   if (!_mesa_is_desktop_gl(ctx) || ctx->Version < 43) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                  }
   num = _mesa_get_shading_language_version(ctx, index, &version);
   if (index >= num) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  }
         case GL_SPIR_V_EXTENSIONS:
      if (!ctx->Extensions.ARB_spirv_extensions) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetStringi");
               if (index >= _mesa_get_spirv_extension_count(ctx)) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetStringi(index=%u)", index);
      }
         default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glGetStringi");
         }
         void
   _get_vao_pointerv(GLenum pname, struct gl_vertex_array_object* vao,
         {
      GET_CURRENT_CONTEXT(ctx);
            if (!params)
            if (MESA_VERBOSE & VERBOSE_API)
            switch (pname) {
      case GL_VERTEX_ARRAY_POINTER:
      if (ctx->API != API_OPENGL_COMPAT && ctx->API != API_OPENGLES)
         *params = (GLvoid *) vao->VertexAttrib[VERT_ATTRIB_POS].Ptr;
      case GL_NORMAL_ARRAY_POINTER:
      if (ctx->API != API_OPENGL_COMPAT && ctx->API != API_OPENGLES)
         *params = (GLvoid *) vao->VertexAttrib[VERT_ATTRIB_NORMAL].Ptr;
      case GL_COLOR_ARRAY_POINTER:
      if (ctx->API != API_OPENGL_COMPAT && ctx->API != API_OPENGLES)
         *params = (GLvoid *) vao->VertexAttrib[VERT_ATTRIB_COLOR0].Ptr;
      case GL_SECONDARY_COLOR_ARRAY_POINTER_EXT:
      if (ctx->API != API_OPENGL_COMPAT)
         *params = (GLvoid *) vao->VertexAttrib[VERT_ATTRIB_COLOR1].Ptr;
      case GL_FOG_COORDINATE_ARRAY_POINTER_EXT:
      if (ctx->API != API_OPENGL_COMPAT)
         *params = (GLvoid *) vao->VertexAttrib[VERT_ATTRIB_FOG].Ptr;
      case GL_INDEX_ARRAY_POINTER:
      if (ctx->API != API_OPENGL_COMPAT)
         *params = (GLvoid *) vao->VertexAttrib[VERT_ATTRIB_COLOR_INDEX].Ptr;
      case GL_TEXTURE_COORD_ARRAY_POINTER:
      if (ctx->API != API_OPENGL_COMPAT && ctx->API != API_OPENGLES)
         *params = (GLvoid *) vao->VertexAttrib[VERT_ATTRIB_TEX(clientUnit)].Ptr;
      case GL_EDGE_FLAG_ARRAY_POINTER:
      if (ctx->API != API_OPENGL_COMPAT)
         *params = (GLvoid *) vao->VertexAttrib[VERT_ATTRIB_EDGEFLAG].Ptr;
      case GL_FEEDBACK_BUFFER_POINTER:
      if (ctx->API != API_OPENGL_COMPAT)
         *params = ctx->Feedback.Buffer;
      case GL_SELECTION_BUFFER_POINTER:
      if (ctx->API != API_OPENGL_COMPAT)
         *params = ctx->Select.Buffer;
      case GL_POINT_SIZE_ARRAY_POINTER_OES:
      if (ctx->API != API_OPENGLES)
         *params = (GLvoid *) vao->VertexAttrib[VERT_ATTRIB_POINT_SIZE].Ptr;
      case GL_DEBUG_CALLBACK_FUNCTION_ARB:
   case GL_DEBUG_CALLBACK_USER_PARAM_ARB:
      *params = _mesa_get_debug_state_ptr(ctx, pname);
      default:
                     invalid_pname:
      _mesa_error( ctx, GL_INVALID_ENUM, "%s", callerstr);
      }
         /**
   * Return pointer-valued state, such as a vertex array pointer.
   *
   * \param pname  names state to be queried
   * \param params  returns the pointer value
   *
   * \sa glGetPointerv().
   *
   * Tries to get the specified pointer via dd_function_table::GetPointerv,
   * otherwise gets the specified pointer from the current context.
   */
   void GLAPIENTRY
   _mesa_GetPointerv( GLenum pname, GLvoid **params )
   {
      GET_CURRENT_CONTEXT(ctx);
            if (_mesa_is_desktop_gl(ctx))
         else
            if (!params)
               }
         void GLAPIENTRY
   _mesa_GetPointerIndexedvEXT( GLenum pname, GLuint index, GLvoid **params )
   {
               if (!params)
            if (MESA_VERBOSE & VERBOSE_API)
            switch (pname) {
      case GL_TEXTURE_COORD_ARRAY_POINTER:
      *params = (GLvoid *) ctx->Array.VAO->VertexAttrib[VERT_ATTRIB_TEX(index)].Ptr;
      default:
                     invalid_pname:
      _mesa_error( ctx, GL_INVALID_ENUM, "glGetPointerIndexedvEXT");
      }
      /**
   * Returns the current GL error code, or GL_NO_ERROR.
   * \return current error code
   *
   * Returns __struct gl_contextRec::ErrorValue.
   */
   GLenum GLAPIENTRY
   _mesa_GetError( void )
   {
      GET_CURRENT_CONTEXT(ctx);
   GLenum e = ctx->ErrorValue;
            /* From Issue (3) of the KHR_no_error spec:
   *
   *    "Should glGetError() always return NO_ERROR or have undefined
   *    results?
   *
   *    RESOLVED: It should for all errors except OUT_OF_MEMORY."
   */
   if (_mesa_is_no_error_enabled(ctx) && e != GL_OUT_OF_MEMORY) {
                  if (MESA_VERBOSE & VERBOSE_API)
            ctx->ErrorValue = (GLenum) GL_NO_ERROR;
   ctx->ErrorDebugCount = 0;
      }
