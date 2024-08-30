   /*
   * Copyright 2009, VMware, Inc.
   * Copyright (C) 2010 LunarG Inc.
   * Copyright © Microsoft Corporation
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
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "st_interop.h"
   #include "st_cb_texture.h"
   #include "st_texture.h"
      #include "bufferobj.h"
   #include "texobj.h"
   #include "syncobj.h"
      int
   st_interop_query_device_info(struct st_context *st,
         {
               /* There is no version 0, thus we do not support it */
   if (out->version == 0)
            out->pci_segment_group = screen->get_param(screen, PIPE_CAP_PCI_GROUP);
   out->pci_bus = screen->get_param(screen, PIPE_CAP_PCI_BUS);
   out->pci_device = screen->get_param(screen, PIPE_CAP_PCI_DEVICE);
            out->vendor_id = screen->get_param(screen, PIPE_CAP_VENDOR_ID);
            if (out->version > 1 && screen->interop_query_device_info)
      out->driver_data_size = screen->interop_query_device_info(screen,
               /* Instruct the caller that we support up-to version two of the interface */
               }
      static int
   lookup_object(struct gl_context *ctx,
               {
      unsigned target = in->target;
   /* Validate the target. */
   switch (in->target) {
   case GL_TEXTURE_BUFFER:
   case GL_TEXTURE_1D:
   case GL_TEXTURE_2D:
   case GL_TEXTURE_3D:
   case GL_TEXTURE_RECTANGLE:
   case GL_TEXTURE_1D_ARRAY:
   case GL_TEXTURE_2D_ARRAY:
   case GL_TEXTURE_CUBE_MAP_ARRAY:
   case GL_TEXTURE_CUBE_MAP:
   case GL_TEXTURE_2D_MULTISAMPLE:
   case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
   case GL_TEXTURE_EXTERNAL_OES:
   case GL_RENDERBUFFER:
   case GL_ARRAY_BUFFER:
         case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
      target = GL_TEXTURE_CUBE_MAP;
      default:
                  /* Validate the simple case of miplevel. */
   if ((target == GL_RENDERBUFFER || target == GL_ARRAY_BUFFER) &&
      in->miplevel != 0)
         if (target == GL_ARRAY_BUFFER) {
      /* Buffer objects.
   *
   * The error checking is based on the documentation of
   * clCreateFromGLBuffer from OpenCL 2.0 SDK.
   */
            /* From OpenCL 2.0 SDK, clCreateFromGLBuffer:
   *  "CL_INVALID_GL_OBJECT if bufobj is not a GL buffer object or is
   *   a GL buffer object but does not have an existing data store or
   *   the size of the buffer is 0."
   */
   if (!buf || buf->Size == 0)
            *res = buf->buffer;
   /* this shouldn't happen */
   if (!*res)
            if (out) {
                           } else if (target == GL_RENDERBUFFER) {
      /* Renderbuffers.
   *
   * The error checking is based on the documentation of
   * clCreateFromGLRenderbuffer from OpenCL 2.0 SDK.
   */
            /* From OpenCL 2.0 SDK, clCreateFromGLRenderbuffer:
   *   "CL_INVALID_GL_OBJECT if renderbuffer is not a GL renderbuffer
   *    object or if the width or height of renderbuffer is zero."
   */
   if (!rb || rb->Width == 0 || rb->Height == 0)
            /* From OpenCL 2.0 SDK, clCreateFromGLRenderbuffer:
   *   "CL_INVALID_OPERATION if renderbuffer is a multi-sample GL
   *    renderbuffer object."
   */
   if (rb->NumSamples > 1)
            /* From OpenCL 2.0 SDK, clCreateFromGLRenderbuffer:
   *   "CL_OUT_OF_RESOURCES if there is a failure to allocate resources
   *    required by the OpenCL implementation on the device."
   */
   *res = rb->texture;
   if (!*res)
            if (out) {
      out->internal_format = rb->InternalFormat;
   out->view_minlevel = 0;
   out->view_numlevels = 1;
   out->view_minlayer = 0;
         } else {
      /* Texture objects.
   *
   * The error checking is based on the documentation of
   * clCreateFromGLTexture from OpenCL 2.0 SDK.
   */
            if (obj)
            /* From OpenCL 2.0 SDK, clCreateFromGLTexture:
   *   "CL_INVALID_GL_OBJECT if texture is not a GL texture object whose
   *    type matches texture_target, if the specified miplevel of texture
   *    is not defined, or if the width or height of the specified
   *    miplevel is zero or if the GL texture object is incomplete."
   */
   if (!obj ||
      obj->Target != target ||
   !obj->_BaseComplete ||
               if (target == GL_TEXTURE_BUFFER) {
                     /* this shouldn't happen */
   if (!stBuf || !stBuf->buffer)
                  if (out) {
      out->internal_format = obj->BufferObjectFormat;
   out->buf_offset = obj->BufferOffset;
                        } else {
      /* From OpenCL 2.0 SDK, clCreateFromGLTexture:
   *   "CL_INVALID_MIP_LEVEL if miplevel is less than the value of
   *    levelbase (for OpenGL implementations) or zero (for OpenGL ES
   *    implementations); or greater than the value of q (for both OpenGL
   *    and OpenGL ES). levelbase and q are defined for the texture in
   *    section 3.8.10 (Texture Completeness) of the OpenGL 2.1
   *    specification and section 3.7.10 of the OpenGL ES 2.0."
   */
                                 *res = st_get_texobj_resource(obj);
   /* Incomplete texture buffer object? This shouldn't really occur. */
                  if (out) {
      out->internal_format = obj->Image[0][0]->InternalFormat;
   out->view_minlevel = obj->Attrib.MinLevel;
   out->view_numlevels = obj->Attrib.NumLevels;
   out->view_minlayer = obj->Attrib.MinLayer;
            }
      }
      int
   st_interop_export_object(struct st_context *st,
               {
      struct pipe_screen *screen = st->pipe->screen;
   struct gl_context *ctx = st->ctx;
   struct pipe_resource *res = NULL;
   struct winsys_handle whandle;
   unsigned usage;
   bool success;
            /* There is no version 0, thus we do not support it */
   if (in->version == 0 || out->version == 0)
            /* Wait for glthread to finish to get up-to-date GL object lookups. */
            /* Validate the OpenGL object and get pipe_resource. */
            int ret = lookup_object(ctx, in, out, &res);
   if (ret != MESA_GLINTEROP_SUCCESS) {
      simple_mtx_unlock(&ctx->Shared->Mutex);
               /* Get the handle. */
   switch (in->access) {
   case MESA_GLINTEROP_ACCESS_READ_ONLY:
      usage = 0;
      case MESA_GLINTEROP_ACCESS_READ_WRITE:
   case MESA_GLINTEROP_ACCESS_WRITE_ONLY:
      usage = PIPE_HANDLE_USAGE_SHADER_WRITE;
      default:
                  out->out_driver_data_written = 0;
   if (screen->interop_export_object) {
      out->out_driver_data_written = screen->interop_export_object(screen,
                                 memset(&whandle, 0, sizeof(whandle));
      if (need_export_dmabuf) {
               success = screen->resource_get_handle(screen, st->pipe, res, &whandle,
            if (!success) {
      simple_mtx_unlock(&ctx->Shared->Mutex);
         #ifndef _WIN32
         #else
         #endif
                        if (res->target == PIPE_BUFFER)
            /* Instruct the caller that we support up-to version one of the interface */
   in->version = 1;
               }
      static int
   flush_object(struct gl_context *ctx,
         {
      struct pipe_resource *res = NULL;
   /* There is no version 0, thus we do not support it */
   if (in->version == 0)
            int ret = lookup_object(ctx, in, NULL, &res);
   if (ret != MESA_GLINTEROP_SUCCESS)
                     /* Instruct the caller that we support up-to version one of the interface */
               }
      int
   st_interop_flush_objects(struct st_context *st,
               {
               /* Wait for glthread to finish to get up-to-date GL object lookups. */
                     for (unsigned i = 0; i < count; ++i) {
               if (ret != MESA_GLINTEROP_SUCCESS) {
      simple_mtx_unlock(&ctx->Shared->Mutex);
                           if (count > 0 && sync) {
                     }
