   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 2013  Timothy Arceri   All Rights Reserved.
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
         #include "arrayobj.h"
   #include "bufferobj.h"
   #include "context.h"
   #include "dlist.h"
   #include "enums.h"
   #include "fbobject.h"
   #include "pipelineobj.h"
   #include "queryobj.h"
   #include "samplerobj.h"
   #include "shaderobj.h"
   #include "syncobj.h"
   #include "texobj.h"
   #include "transformfeedback.h"
   #include "api_exec_decl.h"
         /**
   * Helper for _mesa_ObjectLabel() and _mesa_ObjectPtrLabel().
   */
   static void
   set_label(struct gl_context *ctx, char **labelPtr, const char *label,
         {
      free(*labelPtr);
            /* set new label string */
   if (label) {
      if ((!ext_length && length >= 0) ||
      (ext_length && length > 0)) {
   if (length >= MAX_LABEL_LENGTH)
      _mesa_error(ctx, GL_INVALID_VALUE,
                     /* explicit length */
   *labelPtr = malloc(length+1);
   if (*labelPtr) {
      memcpy(*labelPtr, label, length);
   /* length is not required to include the null terminator so
   * add one just in case
   */
         }
   else {
      if (ext_length && length < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                           int len = strlen(label);
   if (len >= MAX_LABEL_LENGTH)
      _mesa_error(ctx, GL_INVALID_VALUE,
                     /* null-terminated string */
            }
      /**
   * Helper for _mesa_GetObjectLabel() and _mesa_GetObjectPtrLabel().
   * \param src  the src label (may be null)
   * \param dst  pointer to dest buffer (may be null)
   * \param length  returns length of label (may be null)
   * \param bufsize  size of dst buffer
   */
   static void
   copy_label(const GLchar *src, GLchar *dst, GLsizei *length, GLsizei bufSize)
   {
               /* From http://www.opengl.org/registry/specs/KHR/debug.txt:
   * "If <length> is NULL, no length is returned. The maximum number of
   * characters that may be written into <label>, including the null
   * terminator, is specified by <bufSize>. If no debug label was specified
   * for the object then <label> will contain a null-terminated empty string,
   * and zero will be returned in <length>. If <label> is NULL and <length>
   * is non-NULL then no string will be returned and the length of the label
   * will be returned in <length>."
            if (src)
            if (bufSize == 0) {
      if (length)
                     if (dst) {
      if (src) {
                                             if (length)
      }
      /**
   * Helper for _mesa_ObjectLabel() and _mesa_GetObjectLabel().
   */
   static char **
   get_label_pointer(struct gl_context *ctx, GLenum identifier, GLuint name,
         {
      char **labelPtr = NULL;
   GLenum no_match_error =
            switch (identifier) {
   case GL_BUFFER:
   case GL_BUFFER_OBJECT_EXT:
      {
      struct gl_buffer_object *bufObj = _mesa_lookup_bufferobj(ctx, name);
   if (bufObj)
      }
      case GL_SHADER:
   case GL_SHADER_OBJECT_EXT:
      {
      struct gl_shader *shader = _mesa_lookup_shader(ctx, name);
   if (shader)
      }
      case GL_PROGRAM:
   case GL_PROGRAM_OBJECT_EXT:
      {
      struct gl_shader_program *program =
         if (program)
      }
      case GL_VERTEX_ARRAY:
   case GL_VERTEX_ARRAY_OBJECT_EXT:
      {
      struct gl_vertex_array_object *obj = _mesa_lookup_vao(ctx, name);
   if (obj)
      }
      case GL_QUERY:
   case GL_QUERY_OBJECT_EXT:
      {
      struct gl_query_object *query = _mesa_lookup_query_object(ctx, name);
   if (query)
      }
      case GL_TRANSFORM_FEEDBACK:
      {
      /* From the GL 4.5 specification, page 536:
   * "An INVALID_VALUE error is generated if name is not the name of a
   *  valid object of the type specified by identifier."
   */
   struct gl_transform_feedback_object *tfo =
         if (tfo && tfo->EverBound)
      }
      case GL_SAMPLER:
      {
      struct gl_sampler_object *so = _mesa_lookup_samplerobj(ctx, name);
   if (so)
      }
      case GL_TEXTURE:
      {
      struct gl_texture_object *texObj = _mesa_lookup_texture(ctx, name);
   if (texObj && texObj->Target)
      }
      case GL_RENDERBUFFER:
      {
      struct gl_renderbuffer *rb = _mesa_lookup_renderbuffer(ctx, name);
   if (rb)
      }
      case GL_FRAMEBUFFER:
      {
      struct gl_framebuffer *rb = _mesa_lookup_framebuffer(ctx, name);
   if (rb)
      }
      case GL_DISPLAY_LIST:
      if (_mesa_is_desktop_gl_compat(ctx)) {
      struct gl_display_list *list = _mesa_lookup_list(ctx, name, false);
   if (list)
      }
   else {
         }
      case GL_PROGRAM_PIPELINE:
   case GL_PROGRAM_PIPELINE_OBJECT_EXT:
      {
      struct gl_pipeline_object *pipe =
         if (pipe)
      }
      default:
                  if (NULL == labelPtr) {
                        invalid_enum:
      _mesa_error(ctx, GL_INVALID_ENUM, "%s(identifier = %s)",
            }
      void GLAPIENTRY
   _mesa_LabelObjectEXT(GLenum identifier, GLuint name, GLsizei length,
         {
      GET_CURRENT_CONTEXT(ctx);
   const char *callerstr = "glLabelObjectEXT";
            labelPtr = get_label_pointer(ctx, identifier, name, callerstr, true);
   if (!labelPtr)
               }
      void GLAPIENTRY
   _mesa_GetObjectLabelEXT(GLenum identifier, GLuint name, GLsizei bufSize,
         {
      GET_CURRENT_CONTEXT(ctx);
   const char *callerstr = "glGetObjectLabelEXT";
            if (bufSize < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(bufSize = %d)", callerstr,
                     labelPtr = get_label_pointer(ctx, identifier, name, callerstr, true);
   if (!labelPtr)
               }
      void GLAPIENTRY
   _mesa_ObjectLabel(GLenum identifier, GLuint name, GLsizei length,
         {
      GET_CURRENT_CONTEXT(ctx);
   const char *callerstr;
            if (_mesa_is_desktop_gl(ctx))
         else
            labelPtr = get_label_pointer(ctx, identifier, name, callerstr, false);
   if (!labelPtr)
               }
      void GLAPIENTRY
   _mesa_GetObjectLabel(GLenum identifier, GLuint name, GLsizei bufSize,
         {
      GET_CURRENT_CONTEXT(ctx);
   const char *callerstr;
            if (_mesa_is_desktop_gl(ctx))
         else
            if (bufSize < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(bufSize = %d)", callerstr,
                     labelPtr = get_label_pointer(ctx, identifier, name, callerstr, false);
   if (!labelPtr)
               }
      void GLAPIENTRY
   _mesa_ObjectPtrLabel(const void *ptr, GLsizei length, const GLchar *label)
   {
      GET_CURRENT_CONTEXT(ctx);
   struct gl_sync_object *syncObj;
   const char *callerstr;
                     if (_mesa_is_desktop_gl(ctx))
         else
            if (!syncObj) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s (not a valid sync object)",
                              set_label(ctx, labelPtr, label, length, callerstr, false);
      }
      void GLAPIENTRY
   _mesa_GetObjectPtrLabel(const void *ptr, GLsizei bufSize, GLsizei *length,
         {
      GET_CURRENT_CONTEXT(ctx);
   struct gl_sync_object *syncObj;
   const char *callerstr;
            if (_mesa_is_desktop_gl(ctx))
         else
            if (bufSize < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(bufSize = %d)", callerstr,
                     syncObj = _mesa_get_and_ref_sync(ctx, (void*)ptr, true);
   if (!syncObj) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s (not a valid sync object)",
                              copy_label(*labelPtr, label, length, bufSize);
      }
