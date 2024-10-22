   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 2010 LunarG Inc.
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
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   *
   * Authors:
   *    Chia-I Wu <olv@lunarg.com>
   */
      #include <GL/internal/dri_interface.h>
   #include "main/errors.h"
   #include "main/texobj.h"
   #include "main/teximage.h"
   #include "util/u_inlines.h"
   #include "util/format/u_format.h"
   #include "st_cb_eglimage.h"
   #include "st_cb_texture.h"
   #include "st_context.h"
   #include "st_texture.h"
   #include "st_format.h"
   #include "st_manager.h"
   #include "st_sampler_view.h"
   #include "util/u_surface.h"
      static bool
   is_format_supported(struct pipe_screen *screen, enum pipe_format format,
               {
      bool supported = screen->is_format_supported(screen, format, PIPE_TEXTURE_2D,
                        /* for sampling, some formats can be emulated.. it doesn't matter that
   * the surface will have a format that the driver can't cope with because
   * we'll give it sampler view formats that it can deal with and generate
   * a shader variant that converts.
   */
   if ((usage == PIPE_BIND_SAMPLER_VIEW) && !supported) {
      switch (format) {
   case PIPE_FORMAT_IYUV:
      supported = screen->is_format_supported(screen, PIPE_FORMAT_R8_UNORM,
                  case PIPE_FORMAT_NV12:
   case PIPE_FORMAT_NV21:
      supported = screen->is_format_supported(screen, PIPE_FORMAT_R8_UNORM,
                           screen->is_format_supported(screen, PIPE_FORMAT_R8G8_UNORM,
      case PIPE_FORMAT_P010:
   case PIPE_FORMAT_P012:
   case PIPE_FORMAT_P016:
   case PIPE_FORMAT_P030:
      supported = screen->is_format_supported(screen, PIPE_FORMAT_R16_UNORM,
                           screen->is_format_supported(screen, PIPE_FORMAT_R16G16_UNORM,
      case PIPE_FORMAT_Y210:
   case PIPE_FORMAT_Y212:
   case PIPE_FORMAT_Y216:
      supported = screen->is_format_supported(screen, PIPE_FORMAT_R16G16_UNORM,
                           screen->is_format_supported(screen, PIPE_FORMAT_R16G16B16A16_UNORM,
      case PIPE_FORMAT_Y410:
      supported = screen->is_format_supported(screen, PIPE_FORMAT_R10G10B10A2_UNORM,
                  case PIPE_FORMAT_Y412:
   case PIPE_FORMAT_Y416:
      supported = screen->is_format_supported(screen, PIPE_FORMAT_R16G16B16A16_UNORM,
                  case PIPE_FORMAT_YUYV:
      supported = screen->is_format_supported(screen, PIPE_FORMAT_R8G8_R8B8_UNORM,
                           (screen->is_format_supported(screen, PIPE_FORMAT_RG88_UNORM,
               screen->is_format_supported(screen, PIPE_FORMAT_BGRA8888_UNORM,
      case PIPE_FORMAT_YVYU:
      supported = screen->is_format_supported(screen, PIPE_FORMAT_R8B8_R8G8_UNORM,
                           (screen->is_format_supported(screen, PIPE_FORMAT_RG88_UNORM,
               screen->is_format_supported(screen, PIPE_FORMAT_BGRA8888_UNORM,
      case PIPE_FORMAT_UYVY:
      supported = screen->is_format_supported(screen, PIPE_FORMAT_G8R8_B8R8_UNORM,
                           (screen->is_format_supported(screen, PIPE_FORMAT_RG88_UNORM,
               screen->is_format_supported(screen, PIPE_FORMAT_RGBA8888_UNORM,
      case PIPE_FORMAT_VYUY:
      supported = screen->is_format_supported(screen, PIPE_FORMAT_B8R8_G8R8_UNORM,
                           (screen->is_format_supported(screen, PIPE_FORMAT_RG88_UNORM,
               screen->is_format_supported(screen, PIPE_FORMAT_RGBA8888_UNORM,
      case PIPE_FORMAT_AYUV:
      supported = screen->is_format_supported(screen, PIPE_FORMAT_RGBA8888_UNORM,
                  case PIPE_FORMAT_XYUV:
      supported = screen->is_format_supported(screen, PIPE_FORMAT_RGBX8888_UNORM,
                  default:
                        }
      static bool
   is_nv12_as_r8_g8b8_supported(struct pipe_screen *screen, struct st_egl_image *out,
         {
      if (out->format == PIPE_FORMAT_NV12 &&
      out->texture->format == PIPE_FORMAT_R8_G8B8_420_UNORM &&
   screen->is_format_supported(screen, PIPE_FORMAT_R8_G8B8_420_UNORM,
                           *native_supported = false;
               if (out->format == PIPE_FORMAT_NV21 &&
      out->texture->format == PIPE_FORMAT_R8_B8G8_420_UNORM &&
   screen->is_format_supported(screen, PIPE_FORMAT_R8_B8G8_420_UNORM,
                           *native_supported = false;
                  }
      static bool
   is_i420_as_r8_g8_b8_420_supported(struct pipe_screen *screen,
               {
      if (out->format == PIPE_FORMAT_IYUV &&
      out->texture->format == PIPE_FORMAT_R8_G8_B8_420_UNORM &&
   screen->is_format_supported(screen, PIPE_FORMAT_R8_G8_B8_420_UNORM,
                           *native_supported = false;
               if (out->format == PIPE_FORMAT_IYUV &&
      out->texture->format == PIPE_FORMAT_R8_B8_G8_420_UNORM &&
   screen->is_format_supported(screen, PIPE_FORMAT_R8_B8_G8_420_UNORM,
                           *native_supported = false;
                  }
      /**
   * Return the gallium texture of an EGLImage.
   */
   bool
   st_get_egl_image(struct gl_context *ctx, GLeglImageOES image_handle,
               {
      struct st_context *st = st_context(ctx);
   struct pipe_screen *screen = st->screen;
            if (!fscreen || !fscreen->get_egl_image)
            memset(out, 0, sizeof(*out));
   if (!fscreen->get_egl_image(fscreen, (void *) image_handle, out)) {
      /* image_handle does not refer to a valid EGL image object */
   _mesa_error(ctx, GL_INVALID_VALUE, "%s(image handle not found)", error);
               if (!is_nv12_as_r8_g8b8_supported(screen, out, usage, native_supported) &&
      !is_i420_as_r8_g8_b8_420_supported(screen, out, usage, native_supported) &&
   !is_format_supported(screen, out->format, out->texture->nr_samples,
               /* unable to specify a texture object using the specified EGL image */
   pipe_resource_reference(&out->texture, NULL);
   _mesa_error(ctx, GL_INVALID_OPERATION, "%s(format not supported)", error);
               ctx->Shared->HasExternallySharedImages = true;
      }
      /**
   * Return the base format just like _mesa_base_fbo_format does.
   */
   static GLenum
   st_pipe_format_to_base_format(enum pipe_format format)
   {
               if (util_format_is_depth_or_stencil(format)) {
      if (util_format_is_depth_and_stencil(format)) {
         }
   else {
      if (format == PIPE_FORMAT_S8_UINT)
         else
         }
   else {
      /* is this enough? */
   if (util_format_has_alpha(format))
         else
                  }
      void
   st_egl_image_target_renderbuffer_storage(struct gl_context *ctx,
               {
      struct st_egl_image stimg;
            if (st_get_egl_image(ctx, image_handle, PIPE_BIND_RENDER_TARGET,
                  struct pipe_context *pipe = st_context(ctx)->pipe;
            u_surface_default_template(&surf_tmpl, stimg.texture);
   surf_tmpl.format = stimg.format;
   surf_tmpl.u.tex.level = stimg.level;
   surf_tmpl.u.tex.first_layer = stimg.layer;
   surf_tmpl.u.tex.last_layer = stimg.layer;
   ps = pipe->create_surface(pipe, stimg.texture, &surf_tmpl);
            if (!ps)
            rb->Format = st_pipe_format_to_mesa_format(ps->format);
   rb->_BaseFormat = st_pipe_format_to_base_format(ps->format);
            st_set_ws_renderbuffer_surface(rb, ps);
         }
      void
   st_bind_egl_image(struct gl_context *ctx,
                     struct gl_texture_object *texObj,
   struct gl_texture_image *texImage,
   {
      struct st_context *st = st_context(ctx);
   GLenum internalFormat;
            if (stimg->texture->target != gl_target_to_pipe(texObj->Target)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, __func__);
               if (stimg->internalformat) {
         } else {
      /* map pipe format to base format */
   if (util_format_get_component_bits(stimg->format,
               else
               /* switch to surface based */
   if (!texObj->surface_based) {
      _mesa_clear_texture_object(ctx, texObj, NULL);
               /* TODO RequiredTextureImageUnits should probably be reset back
   * to 1 somewhere if different texture is bound??
   */
   if (!native_supported) {
      switch (stimg->format) {
   case PIPE_FORMAT_NV12:
   case PIPE_FORMAT_NV21:
      if (stimg->texture->format == PIPE_FORMAT_R8_G8B8_420_UNORM ||
      stimg->texture->format == PIPE_FORMAT_R8_B8G8_420_UNORM) {
   texFormat = MESA_FORMAT_R8G8B8X8_UNORM;
      } else {
      texFormat = MESA_FORMAT_R_UNORM8;
      }
      case PIPE_FORMAT_P010:
   case PIPE_FORMAT_P012:
   case PIPE_FORMAT_P016:
   case PIPE_FORMAT_P030:
      texFormat = MESA_FORMAT_R_UNORM16;
   texObj->RequiredTextureImageUnits = 2;
      case PIPE_FORMAT_Y210:
   case PIPE_FORMAT_Y212:
   case PIPE_FORMAT_Y216:
      texFormat = MESA_FORMAT_RG_UNORM16;
   texObj->RequiredTextureImageUnits = 2;
      case PIPE_FORMAT_Y410:
      texFormat = MESA_FORMAT_B10G10R10A2_UNORM;
   internalFormat = GL_RGBA;
   texObj->RequiredTextureImageUnits = 1;
      case PIPE_FORMAT_Y412:
   case PIPE_FORMAT_Y416:
      texFormat = MESA_FORMAT_RGBA_UNORM16;
   internalFormat = GL_RGBA;
   texObj->RequiredTextureImageUnits = 1;
      case PIPE_FORMAT_IYUV:
      if (stimg->texture->format == PIPE_FORMAT_R8_G8_B8_420_UNORM ||
      stimg->texture->format == PIPE_FORMAT_R8_B8_G8_420_UNORM) {
   texFormat = MESA_FORMAT_R8G8B8X8_UNORM;
      } else {
      texFormat = MESA_FORMAT_R_UNORM8;
      }
      case PIPE_FORMAT_YUYV:
   case PIPE_FORMAT_YVYU:
   case PIPE_FORMAT_UYVY:
   case PIPE_FORMAT_VYUY:
      if (stimg->texture->format == PIPE_FORMAT_R8G8_R8B8_UNORM) {
      texFormat = MESA_FORMAT_RG_RB_UNORM8;
      } else if (stimg->texture->format == PIPE_FORMAT_R8B8_R8G8_UNORM) {
      texFormat = MESA_FORMAT_RB_RG_UNORM8;
      } else if (stimg->texture->format == PIPE_FORMAT_G8R8_B8R8_UNORM) {
      texFormat = MESA_FORMAT_GR_BR_UNORM8;
      } else if (stimg->texture->format == PIPE_FORMAT_B8R8_G8R8_UNORM) {
      texFormat = MESA_FORMAT_BR_GR_UNORM8;
      } else {
      texFormat = MESA_FORMAT_RG_UNORM8;
      }
      case PIPE_FORMAT_AYUV:
      texFormat = MESA_FORMAT_R8G8B8A8_UNORM;
   internalFormat = GL_RGBA;
   texObj->RequiredTextureImageUnits = 1;
      case PIPE_FORMAT_XYUV:
      texFormat = MESA_FORMAT_R8G8B8X8_UNORM;
   texObj->RequiredTextureImageUnits = 1;
      default:
      unreachable("unexpected emulated format");
         } else {
      texFormat = st_pipe_format_to_mesa_format(stimg->format);
   /* Use previously derived internalformat as specified by
   * EXT_EGL_image_storage.
   */
   if (tex_storage && texObj->Target == GL_TEXTURE_2D
      && stimg->internalformat) {
   internalFormat = stimg->internalformat;
   if (internalFormat == GL_NONE) {
      _mesa_error(ctx, GL_INVALID_OPERATION, __func__);
            }
               /* Minify texture size based on level set on the EGLImage. */
   uint32_t width = u_minify(stimg->texture->width0, stimg->level);
            _mesa_init_teximage_fields(ctx, texImage, width, height,
            pipe_resource_reference(&texObj->pt, stimg->texture);
   st_texture_release_all_sampler_views(st, texObj);
   pipe_resource_reference(&texImage->pt, texObj->pt);
   if (st->screen->resource_changed)
                     switch (stimg->yuv_color_space) {
   case __DRI_YUV_COLOR_SPACE_ITU_REC709:
      texObj->yuv_color_space = GL_TEXTURE_YUV_COLOR_SPACE_REC709;
      case __DRI_YUV_COLOR_SPACE_ITU_REC2020:
      texObj->yuv_color_space = GL_TEXTURE_YUV_COLOR_SPACE_REC2020;
      default:
      texObj->yuv_color_space = GL_TEXTURE_YUV_COLOR_SPACE_REC601;
               if (stimg->yuv_range == __DRI_YUV_FULL_RANGE)
            texObj->level_override = stimg->level;
   texObj->layer_override = stimg->layer;
               }
      static GLboolean
   st_validate_egl_image(struct gl_context *ctx, GLeglImageOES image_handle)
   {
      struct st_context *st = st_context(ctx);
               }
      void
   st_init_eglimage_functions(struct dd_function_table *functions,
         {
      if (has_egl_image_validate)
      }
