   /*
   * Copyright (c) 2013  Brian Paul   All Rights Reserved.
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
         /*
   * Off-Screen rendering into client memory.
   * OpenGL gallium frontend for softpipe and llvmpipe.
   *
   * Notes:
   *
   * If Gallium is built with LLVM support we use the llvmpipe driver.
   * Otherwise we use softpipe.  The GALLIUM_DRIVER environment variable
   * may be set to "softpipe" or "llvmpipe" to override.
   *
   * With softpipe we could render directly into the user's buffer by using a
   * display target resource.  However, softpipe doesn't support "upside-down"
   * rendering which would be needed for the OSMESA_Y_UP=TRUE case.
   *
   * With llvmpipe we could only render directly into the user's buffer when its
   * width and height is a multiple of the tile size (64 pixels).
   *
   * Because of these constraints we always render into ordinary resources then
   * copy the results to the user's buffer in the flush_front() function which
   * is called when the app calls glFlush/Finish.
   *
   * In general, the OSMesa interface is pretty ugly and not a good match
   * for Gallium.  But we're interested in doing the best we can to preserve
   * application portability.  With a little work we could come up with a
   * much nicer, new off-screen Gallium interface...
   */
         #include <stdio.h>
   #include <c11/threads.h>
      #include "state_tracker/st_context.h"
      #include "GL/osmesa.h"
      #include "glapi/glapi.h"  /* for OSMesaGetProcAddress below */
      #include "pipe/p_context.h"
   #include "pipe/p_screen.h"
   #include "pipe/p_state.h"
      #include "util/u_atomic.h"
   #include "util/u_box.h"
   #include "util/u_debug.h"
   #include "util/format/u_format.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
      #include "postprocess/filters.h"
   #include "postprocess/postprocess.h"
      #include "frontend/api.h"
            extern struct pipe_screen *
   osmesa_create_screen(void);
            struct osmesa_buffer
   {
      struct pipe_frontend_drawable base;
   struct st_visual visual;
                                 };
         struct osmesa_context
   {
                                 /* Storage for depth/stencil, if the user has requested access.  The backing
   * driver always has its own storage for the actual depth/stencil, which we
   * have to transfer in and out.
   */
   void *zs;
                     GLenum format;         /*< User-specified context format */
   GLenum type;           /*< Buffer's data type */
   GLint user_row_length; /*< user-specified number of pixels per row */
   GLboolean y_up;        /*< TRUE  -> Y increases upward */
            /** Which postprocessing filters are enabled. */
   unsigned pp_enabled[PP_FILTERS];
      };
      /**
   * Called from the ST manager.
   */
   static int
   osmesa_st_get_param(struct pipe_frontend_screen *fscreen, enum st_manager_param param)
   {
      /* no-op */
      }
      static struct pipe_frontend_screen *global_fscreen = NULL;
      static void
   destroy_st_manager(void)
   {
      if (global_fscreen) {
      if (global_fscreen->screen)
               }
      static void
   create_st_manager(void)
   {
      if (atexit(destroy_st_manager) != 0)
            global_fscreen = CALLOC_STRUCT(pipe_frontend_screen);
   if (global_fscreen) {
      global_fscreen->screen = osmesa_create_screen();
   global_fscreen->get_param = osmesa_st_get_param;
         }
      /**
   * Create/return a singleton st_manager object.
   */
   static struct pipe_frontend_screen *
   get_st_manager(void)
   {
                           }
      /* Reads the color or depth buffer from the backing context to either the user storage
   * (color buffer) or our temporary (z/s)
   */
   static void
   osmesa_read_buffer(OSMesaContext osmesa, struct pipe_resource *res, void *dst,
         {
               struct pipe_box box;
            struct pipe_transfer *transfer = NULL;
   uint8_t *src = pipe->texture_map(pipe, res, 0, PIPE_MAP_READ, &box,
            /*
   * Copy the color buffer from the resource to the user's buffer.
            if (y_up) {
      /* need to flip image upside down */
   dst = (uint8_t *)dst + (res->height0 - 1) * dst_stride;
               unsigned bpp = util_format_get_blocksize(res->format);
   for (unsigned y = 0; y < res->height0; y++)
   {
      memcpy(dst, src, bpp * res->width0);
   dst = (uint8_t *)dst + dst_stride;
                  }
         /**
   * Given an OSMESA_x format and a GL_y type, return the best
   * matching PIPE_FORMAT_z.
   * Note that we can't exactly match all user format/type combinations
   * with gallium formats.  If we find this to be a problem, we can
   * implement more elaborate format/type conversion in the flush_front()
   * function.
   */
   static enum pipe_format
   osmesa_choose_format(GLenum format, GLenum type)
   {
      switch (format) {
   case OSMESA_RGBA:
      #if UTIL_ARCH_LITTLE_ENDIAN
         #else
         #endif
         }
   else if (type == GL_UNSIGNED_SHORT) {
         }
   else if (type == GL_FLOAT) {
         }
   else {
         }
      case OSMESA_BGRA:
      #if UTIL_ARCH_LITTLE_ENDIAN
         #else
         #endif
         }
   else if (type == GL_UNSIGNED_SHORT) {
         }
   else if (type == GL_FLOAT) {
         }
   else {
         }
      case OSMESA_ARGB:
      #if UTIL_ARCH_LITTLE_ENDIAN
         #else
         #endif
         }
   else if (type == GL_UNSIGNED_SHORT) {
         }
   else if (type == GL_FLOAT) {
         }
   else {
         }
      case OSMESA_RGB:
      if (type == GL_UNSIGNED_BYTE) {
         }
   else if (type == GL_UNSIGNED_SHORT) {
         }
   else if (type == GL_FLOAT) {
         }
   else {
         }
      case OSMESA_BGR:
      /* No gallium format for this one */
      case OSMESA_RGB_565:
      if (type != GL_UNSIGNED_SHORT_5_6_5)
            default:
            }
         /**
   * Initialize an st_visual object.
   */
   static void
   osmesa_init_st_visual(struct st_visual *vis,
                     {
               if (ds_format != PIPE_FORMAT_NONE)
         if (accum_format != PIPE_FORMAT_NONE)
            vis->color_format = color_format;
   vis->depth_stencil_format = ds_format;
   vis->accum_format = accum_format;
      }
         /**
   * Return the osmesa_buffer that corresponds to an pipe_frontend_drawable.
   */
   static inline struct osmesa_buffer *
   drawable_to_osbuffer(struct pipe_frontend_drawable *drawable)
   {
         }
         /**
   * Called via glFlush/glFinish.  This is where we copy the contents
   * of the driver's color buffer into the user-specified buffer.
   */
   static bool
   osmesa_st_framebuffer_flush_front(struct st_context *st,
               {
      OSMesaContext osmesa = OSMesaGetCurrentContext();
   struct osmesa_buffer *osbuffer = drawable_to_osbuffer(drawable);
   struct pipe_resource *res = osbuffer->textures[statt];
   unsigned bpp;
            if (statt != ST_ATTACHMENT_FRONT_LEFT)
            if (osmesa->pp) {
      struct pipe_resource *zsbuf = NULL;
            /* Find the z/stencil buffer if there is one */
   for (i = 0; i < ARRAY_SIZE(osbuffer->textures); i++) {
      struct pipe_resource *res = osbuffer->textures[i];
   if (res) {
                     if (util_format_has_depth(desc)) {
      zsbuf = res;
                     /* run the postprocess stage(s) */
               /* Snapshot the color buffer to the user's buffer. */
   bpp = util_format_get_blocksize(osbuffer->visual.color_format);
   if (osmesa->user_row_length)
         else
                     /* If the user has requested the Z/S buffer, then snapshot that one too. */
   if (osmesa->zs) {
      osmesa_read_buffer(osmesa, osbuffer->textures[ST_ATTACHMENT_DEPTH_STENCIL],
                  }
         /**
   * Called by the st manager to validate the framebuffer (allocate
   * its resources).
   */
   static bool
   osmesa_st_framebuffer_validate(struct st_context *st,
                                 {
      struct pipe_screen *screen = get_st_manager()->screen;
   enum st_attachment_type i;
   struct osmesa_buffer *osbuffer = drawable_to_osbuffer(drawable);
            memset(&templat, 0, sizeof(templat));
   templat.target = PIPE_TEXTURE_RECT;
   templat.format = 0; /* setup below */
   templat.last_level = 0;
   templat.width0 = osbuffer->width;
   templat.height0 = osbuffer->height;
   templat.depth0 = 1;
   templat.array_size = 1;
   templat.usage = PIPE_USAGE_DEFAULT;
   templat.bind = 0; /* setup below */
            for (i = 0; i < count; i++) {
      enum pipe_format format = PIPE_FORMAT_NONE;
            /*
   * At this time, we really only need to handle the front-left color
   * attachment, since that's all we specified for the visual in
   * osmesa_init_st_visual().
   */
   if (statts[i] == ST_ATTACHMENT_FRONT_LEFT) {
      format = osbuffer->visual.color_format;
      }
   else if (statts[i] == ST_ATTACHMENT_DEPTH_STENCIL) {
      format = osbuffer->visual.depth_stencil_format;
      }
   else if (statts[i] == ST_ATTACHMENT_ACCUM) {
      format = osbuffer->visual.accum_format;
      }
   else {
      debug_warning("Unexpected attachment type in "
               templat.format = format;
   templat.bind = bind;
   pipe_resource_reference(&out[i], NULL);
   out[i] = osbuffer->textures[statts[i]] =
                  }
      static uint32_t osmesa_fb_ID = 0;
         /**
   * Create new buffer and add to linked list.
   */
   static struct osmesa_buffer *
   osmesa_create_buffer(enum pipe_format color_format,
               {
      struct osmesa_buffer *osbuffer = CALLOC_STRUCT(osmesa_buffer);
   if (osbuffer) {
      osbuffer->base.flush_front = osmesa_st_framebuffer_flush_front;
   osbuffer->base.validate = osmesa_st_framebuffer_validate;
   p_atomic_set(&osbuffer->base.stamp, 1);
   osbuffer->base.ID = p_atomic_inc_return(&osmesa_fb_ID);
   osbuffer->base.fscreen = get_st_manager();
            osmesa_init_st_visual(&osbuffer->visual, color_format,
                  }
         static void
   osmesa_destroy_buffer(struct osmesa_buffer *osbuffer)
   {
      /*
   * Notify the state manager that the associated framebuffer interface
   * is no longer valid.
   */
               }
            /**********************************************************************/
   /*****                    Public Functions                        *****/
   /**********************************************************************/
         /**
   * Create an Off-Screen Mesa rendering context.  The only attribute needed is
   * an RGBA vs Color-Index mode flag.
   *
   * Input:  format - Must be GL_RGBA
   *         sharelist - specifies another OSMesaContext with which to share
   *                     display lists.  NULL indicates no sharing.
   * Return:  an OSMesaContext or 0 if error
   */
   GLAPI OSMesaContext GLAPIENTRY
   OSMesaCreateContext(GLenum format, OSMesaContext sharelist)
   {
         }
         /**
   * New in Mesa 3.5
   *
   * Create context and specify size of ancillary buffers.
   */
   GLAPI OSMesaContext GLAPIENTRY
   OSMesaCreateContextExt(GLenum format, GLint depthBits, GLint stencilBits,
         {
               attribs[n++] = OSMESA_FORMAT;
   attribs[n++] = format;
   attribs[n++] = OSMESA_DEPTH_BITS;
   attribs[n++] = depthBits;
   attribs[n++] = OSMESA_STENCIL_BITS;
   attribs[n++] = stencilBits;
   attribs[n++] = OSMESA_ACCUM_BITS;
   attribs[n++] = accumBits;
               }
         /**
   * New in Mesa 11.2
   *
   * Create context with attribute list.
   */
   GLAPI OSMesaContext GLAPIENTRY
   OSMesaCreateContextAttribs(const int *attribList, OSMesaContext sharelist)
   {
      OSMesaContext osmesa;
   struct st_context *st_shared;
   enum st_context_error st_error = 0;
   struct st_context_attribs attribs;
   GLenum format = GL_RGBA;
   int depthBits = 0, stencilBits = 0, accumBits = 0;
   int profile = OSMESA_COMPAT_PROFILE, version_major = 1, version_minor = 0;
            if (sharelist) {
         }
   else {
                  for (i = 0; attribList[i]; i += 2) {
      switch (attribList[i]) {
   case OSMESA_FORMAT:
      format = attribList[i+1];
   switch (format) {
   case OSMESA_COLOR_INDEX:
   case OSMESA_RGBA:
   case OSMESA_BGRA:
   case OSMESA_ARGB:
   case OSMESA_RGB:
   case OSMESA_BGR:
   case OSMESA_RGB_565:
      /* legal */
      default:
         }
      case OSMESA_DEPTH_BITS:
      depthBits = attribList[i+1];
   if (depthBits < 0)
            case OSMESA_STENCIL_BITS:
      stencilBits = attribList[i+1];
   if (stencilBits < 0)
            case OSMESA_ACCUM_BITS:
      accumBits = attribList[i+1];
   if (accumBits < 0)
            case OSMESA_PROFILE:
      profile = attribList[i+1];
   if (profile != OSMESA_CORE_PROFILE &&
      profile != OSMESA_COMPAT_PROFILE)
         case OSMESA_CONTEXT_MAJOR_VERSION:
      version_major = attribList[i+1];
   if (version_major < 1)
            case OSMESA_CONTEXT_MINOR_VERSION:
      version_minor = attribList[i+1];
   if (version_minor < 0)
            case 0:
      /* end of list */
      default:
      fprintf(stderr, "Bad attribute in OSMesaCreateContextAttribs()\n");
                  osmesa = (OSMesaContext) CALLOC_STRUCT(osmesa_context);
   if (!osmesa)
            /* Choose depth/stencil/accum buffer formats */
   if (accumBits > 0) {
         }
   if (depthBits > 0 && stencilBits > 0) {
         }
   else if (stencilBits > 0) {
         }
   else if (depthBits >= 24) {
         }
   else if (depthBits >= 16) {
                  /*
   * Create the rendering context
   */
   memset(&attribs, 0, sizeof(attribs));
   attribs.profile = (profile == OSMESA_CORE_PROFILE)
         attribs.major = version_major;
   attribs.minor = version_minor;
   attribs.flags = 0;  /* ST_CONTEXT_FLAG_x */
   attribs.options.force_glsl_extensions_warn = false;
   attribs.options.disable_blend_func_extended = false;
   attribs.options.disable_glsl_line_continuations = false;
            osmesa_init_st_visual(&attribs.visual,
                        osmesa->st = st_api_create_context(get_st_manager(),
         if (!osmesa->st) {
      FREE(osmesa);
                        osmesa->format = format;
   osmesa->user_row_length = 0;
               }
            /**
   * Destroy an Off-Screen Mesa rendering context.
   *
   * \param osmesa  the context to destroy
   */
   GLAPI void GLAPIENTRY
   OSMesaDestroyContext(OSMesaContext osmesa)
   {
      if (osmesa) {
      pp_free(osmesa->pp);
   st_destroy_context(osmesa->st);
   free(osmesa->zs);
         }
         /**
   * Bind an OSMesaContext to an image buffer.  The image buffer is just a
   * block of memory which the client provides.  Its size must be at least
   * as large as width*height*pixelSize.  Its address should be a multiple
   * of 4 if using RGBA mode.
   *
   * By default, image data is stored in the order of glDrawPixels: row-major
   * order with the lower-left image pixel stored in the first array position
   * (ie. bottom-to-top).
   *
   * If the context's viewport hasn't been initialized yet, it will now be
   * initialized to (0,0,width,height).
   *
   * Input:  osmesa - the rendering context
   *         buffer - the image buffer memory
   *         type - data type for pixel components
   *                GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT_5_6_5, GL_UNSIGNED_SHORT
   *                or GL_FLOAT.
   *         width, height - size of image buffer in pixels, at least 1
   * Return:  GL_TRUE if success, GL_FALSE if error because of invalid osmesa,
   *          invalid type, invalid size, etc.
   */
   GLAPI GLboolean GLAPIENTRY
   OSMesaMakeCurrent(OSMesaContext osmesa, void *buffer, GLenum type,
         {
               if (!osmesa && !buffer) {
      st_api_make_current(NULL, NULL, NULL);
               if (!osmesa || !buffer || width < 1 || height < 1) {
                  color_format = osmesa_choose_format(osmesa->format, type);
   if (color_format == PIPE_FORMAT_NONE) {
      fprintf(stderr, "OSMesaMakeCurrent(unsupported format/type)\n");
               /* See if we already have a buffer that uses these pixel formats */
   if (osmesa->current_buffer &&
      (osmesa->current_buffer->visual.color_format != color_format ||
   osmesa->current_buffer->visual.depth_stencil_format != osmesa->depth_stencil_format ||
   osmesa->current_buffer->visual.accum_format != osmesa->accum_format ||
   osmesa->current_buffer->width != width ||
   osmesa->current_buffer->height != height)) {
   osmesa_destroy_buffer(osmesa->current_buffer);
               if (!osmesa->current_buffer) {
      osmesa->current_buffer = osmesa_create_buffer(color_format,
                              osbuffer->width = width;
   osbuffer->height = height;
                              /* XXX: We should probably load the current color value into the buffer here
   * to match classic swrast behavior (context's fb starts with the contents of
   * your pixel buffer).
            if (!osmesa->ever_used) {
      /* one-time init, just postprocessing for now */
   bool any_pp_enabled = false;
            for (i = 0; i < ARRAY_SIZE(osmesa->pp_enabled); i++) {
      if (osmesa->pp_enabled[i]) {
      any_pp_enabled = true;
                  if (any_pp_enabled) {
      osmesa->pp = pp_init(osmesa->st->pipe,
                                                         }
            GLAPI OSMesaContext GLAPIENTRY
   OSMesaGetCurrentContext(void)
   {
      struct st_context *st = st_api_get_current();
      }
            GLAPI void GLAPIENTRY
   OSMesaPixelStore(GLint pname, GLint value)
   {
               switch (pname) {
   case OSMESA_ROW_LENGTH:
      osmesa->user_row_length = value;
      case OSMESA_Y_UP:
      osmesa->y_up = value ? GL_TRUE : GL_FALSE;
      default:
      fprintf(stderr, "Invalid pname in OSMesaPixelStore()\n");
         }
         GLAPI void GLAPIENTRY
   OSMesaGetIntegerv(GLint pname, GLint *value)
   {
      OSMesaContext osmesa = OSMesaGetCurrentContext();
            switch (pname) {
   case OSMESA_WIDTH:
      *value = osbuffer ? osbuffer->width : 0;
      case OSMESA_HEIGHT:
      *value = osbuffer ? osbuffer->height : 0;
      case OSMESA_FORMAT:
      *value = osmesa->format;
      case OSMESA_TYPE:
      /* current color buffer's data type */
   *value = osmesa->type;
      case OSMESA_ROW_LENGTH:
      *value = osmesa->user_row_length;
      case OSMESA_Y_UP:
      *value = osmesa->y_up;
      case OSMESA_MAX_WIDTH:
         case OSMESA_MAX_HEIGHT:
      {
      struct pipe_screen *screen = get_st_manager()->screen;
      }
      default:
      fprintf(stderr, "Invalid pname in OSMesaGetIntegerv()\n");
         }
         /**
   * Return information about the depth buffer associated with an OSMesa context.
   * Input:  c - the OSMesa context
   * Output:  width, height - size of buffer in pixels
   *          bytesPerValue - bytes per depth value (2 or 4)
   *          buffer - pointer to depth buffer values
   * Return:  GL_TRUE or GL_FALSE to indicate success or failure.
   */
   GLAPI GLboolean GLAPIENTRY
   OSMesaGetDepthBuffer(OSMesaContext c, GLint *width, GLint *height,
         {
      struct osmesa_buffer *osbuffer = c->current_buffer;
            if (!res) {
      *width = 0;
   *height = 0;
   *bytesPerValue = 0;
   *buffer = NULL;
               *width = res->width0;
   *height = res->height0;
            if (!c->zs) {
      c->zs_stride = *width * *bytesPerValue;
   c->zs = calloc(c->zs_stride, *height);
   if (!c->zs)
                                    }
         /**
   * Return the color buffer associated with an OSMesa context.
   * Input:  c - the OSMesa context
   * Output:  width, height - size of buffer in pixels
   *          format - the pixel format (OSMESA_FORMAT)
   *          buffer - pointer to color buffer values
   * Return:  GL_TRUE or GL_FALSE to indicate success or failure.
   */
   GLAPI GLboolean GLAPIENTRY
   OSMesaGetColorBuffer(OSMesaContext osmesa, GLint *width,
         {
               if (osbuffer) {
      *width = osbuffer->width;
   *height = osbuffer->height;
   *format = osmesa->format;
   *buffer = osbuffer->map;
      }
   else {
      *width = 0;
   *height = 0;
   *format = 0;
   *buffer = 0;
         }
         struct name_function
   {
      const char *Name;
      };
      static struct name_function functions[] = {
      { "OSMesaCreateContext", (OSMESAproc) OSMesaCreateContext },
   { "OSMesaCreateContextExt", (OSMESAproc) OSMesaCreateContextExt },
   { "OSMesaCreateContextAttribs", (OSMESAproc) OSMesaCreateContextAttribs },
   { "OSMesaDestroyContext", (OSMESAproc) OSMesaDestroyContext },
   { "OSMesaMakeCurrent", (OSMESAproc) OSMesaMakeCurrent },
   { "OSMesaGetCurrentContext", (OSMESAproc) OSMesaGetCurrentContext },
   { "OSMesaPixelStore", (OSMESAproc) OSMesaPixelStore },
   { "OSMesaGetIntegerv", (OSMESAproc) OSMesaGetIntegerv },
   { "OSMesaGetDepthBuffer", (OSMESAproc) OSMesaGetDepthBuffer },
   { "OSMesaGetColorBuffer", (OSMESAproc) OSMesaGetColorBuffer },
   { "OSMesaGetProcAddress", (OSMESAproc) OSMesaGetProcAddress },
   { "OSMesaColorClamp", (OSMESAproc) OSMesaColorClamp },
   { "OSMesaPostprocess", (OSMESAproc) OSMesaPostprocess },
      };
         GLAPI OSMESAproc GLAPIENTRY
   OSMesaGetProcAddress(const char *funcName)
   {
      int i;
   for (i = 0; functions[i].Name; i++) {
      if (strcmp(functions[i].Name, funcName) == 0)
      }
      }
         GLAPI void GLAPIENTRY
   OSMesaColorClamp(GLboolean enable)
   {
               _mesa_ClampColor(GL_CLAMP_FRAGMENT_COLOR_ARB,
      }
         GLAPI void GLAPIENTRY
   OSMesaPostprocess(OSMesaContext osmesa, const char *filter,
         {
      if (!osmesa->ever_used) {
      /* We can only enable/disable postprocess filters before a context
   * is made current for the first time.
   */
            for (i = 0; i < PP_FILTERS; i++) {
      if (strcmp(pp_filters[i].name, filter) == 0) {
      osmesa->pp_enabled[i] = enable_value;
         }
      }
   else {
            }
