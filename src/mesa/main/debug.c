   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
   * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
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
      #include <stdio.h>
   #include "errors.h"
   #include "mtypes.h"
   #include "attrib.h"
   #include "enums.h"
   #include "formats.h"
   #include "hash.h"
      #include "macros.h"
   #include "debug.h"
   #include "get.h"
   #include "pixelstore.h"
   #include "readpix.h"
   #include "texobj.h"
   #include "api_exec_decl.h"
      #include "state_tracker/st_cb_texture.h"
   #include "state_tracker/st_cb_readpixels.h"
      static const char *
   tex_target_name(GLenum tgt)
   {
      static const struct {
      GLenum target;
      } tex_targets[] = {
      { GL_TEXTURE_1D, "GL_TEXTURE_1D" },
   { GL_TEXTURE_2D, "GL_TEXTURE_2D" },
   { GL_TEXTURE_3D, "GL_TEXTURE_3D" },
   { GL_TEXTURE_CUBE_MAP, "GL_TEXTURE_CUBE_MAP" },
   { GL_TEXTURE_RECTANGLE, "GL_TEXTURE_RECTANGLE" },
   { GL_TEXTURE_1D_ARRAY_EXT, "GL_TEXTURE_1D_ARRAY" },
   { GL_TEXTURE_2D_ARRAY_EXT, "GL_TEXTURE_2D_ARRAY" },
   { GL_TEXTURE_CUBE_MAP_ARRAY, "GL_TEXTURE_CUBE_MAP_ARRAY" },
   { GL_TEXTURE_BUFFER, "GL_TEXTURE_BUFFER" },
   { GL_TEXTURE_2D_MULTISAMPLE, "GL_TEXTURE_2D_MULTISAMPLE" },
   { GL_TEXTURE_2D_MULTISAMPLE_ARRAY, "GL_TEXTURE_2D_MULTISAMPLE_ARRAY" },
      };
   GLuint i;
   STATIC_ASSERT(ARRAY_SIZE(tex_targets) == NUM_TEXTURE_TARGETS);
   for (i = 0; i < ARRAY_SIZE(tex_targets); i++) {
      if (tex_targets[i].target == tgt)
      }
      }
         void
   _mesa_print_state( const char *msg, GLuint state )
   {
      _mesa_debug(NULL,
   "%s: (0x%x) %s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
   msg,
   state,
   (state & _NEW_MODELVIEW)       ? "ctx->ModelView, " : "",
   (state & _NEW_PROJECTION)      ? "ctx->Projection, " : "",
   (state & _NEW_TEXTURE_MATRIX)  ? "ctx->TextureMatrix, " : "",
   (state & _NEW_COLOR)           ? "ctx->Color, " : "",
   (state & _NEW_DEPTH)           ? "ctx->Depth, " : "",
   (state & _NEW_FOG)             ? "ctx->Fog, " : "",
   (state & _NEW_HINT)            ? "ctx->Hint, " : "",
   (state & _NEW_LIGHT_CONSTANTS) ? "ctx->Light(Constants), " : "",
         (state & _NEW_LINE)            ? "ctx->Line, " : "",
   (state & _NEW_PIXEL)           ? "ctx->Pixel, " : "",
   (state & _NEW_POINT)           ? "ctx->Point, " : "",
   (state & _NEW_POLYGON)         ? "ctx->Polygon, " : "",
   (state & _NEW_POLYGONSTIPPLE)  ? "ctx->PolygonStipple, " : "",
   (state & _NEW_SCISSOR)         ? "ctx->Scissor, " : "",
   (state & _NEW_STENCIL)         ? "ctx->Stencil, " : "",
   (state & _NEW_TEXTURE_OBJECT)  ? "ctx->Texture(Object), " : "",
   (state & _NEW_TRANSFORM)       ? "ctx->Transform, " : "",
   (state & _NEW_VIEWPORT)        ? "ctx->Viewport, " : "",
         (state & _NEW_RENDERMODE)      ? "ctx->RenderMode, " : "",
      }
            /**
   * Print information about this Mesa version and build options.
   */
   void _mesa_print_info( struct gl_context *ctx )
   {
      _mesa_debug(NULL, "Mesa GL_VERSION = %s\n",
   (char *) _mesa_GetString(GL_VERSION));
   _mesa_debug(NULL, "Mesa GL_RENDERER = %s\n",
   (char *) _mesa_GetString(GL_RENDERER));
   _mesa_debug(NULL, "Mesa GL_VENDOR = %s\n",
            /* use ctx as GL_EXTENSIONS will not work on 3.0 or higher
   * core contexts.
   */
         #if defined(USE_X86_ASM)
         #else
         #endif
   #if defined(USE_SPARC_ASM)
         #else
         #endif
   }
         /**
   * Set verbose logging flags.  When these flags are set, GL API calls
   * in the various categories will be printed to stderr.
   * \param str  a comma-separated list of keywords
   */
   static void
   set_verbose_flags(const char *str)
   {
   #ifndef NDEBUG
      struct option {
      const char *name;
      };
   static const struct option opts[] = {
      { "varray",    VERBOSE_VARRAY },
   { "tex",       VERBOSE_TEXTURE },
   { "mat",       VERBOSE_MATERIAL },
   { "pipe",      VERBOSE_PIPELINE },
   { "driver",    VERBOSE_DRIVER },
   { "state",     VERBOSE_STATE },
   { "api",       VERBOSE_API },
   { "list",      VERBOSE_DISPLAY_LIST },
   { "lighting",  VERBOSE_LIGHTING },
   { "disassem",  VERBOSE_DISASSEM },
      };
            if (!str)
            MESA_VERBOSE = 0x0;
   for (i = 0; i < ARRAY_SIZE(opts); i++) {
      if (strstr(str, opts[i].name) || strcmp(str, "all") == 0)
         #endif
   }
         /**
   * Set debugging flags.  When these flags are set, Mesa will do additional
   * debug checks or actions.
   * \param str  a comma-separated list of keywords
   */
   static void
   set_debug_flags(const char *str)
   {
   #ifndef NDEBUG
      struct option {
      const char *name;
      };
   static const struct option opts[] = {
      { "silent", DEBUG_SILENT }, /* turn off debug messages */
   { "flush", DEBUG_ALWAYS_FLUSH }, /* flush after each drawing command */
   { "incomplete_tex", DEBUG_INCOMPLETE_TEXTURE },
   { "incomplete_fbo", DEBUG_INCOMPLETE_FBO },
      };
            if (!str)
            MESA_DEBUG_FLAGS = 0x0;
   for (i = 0; i < ARRAY_SIZE(opts); i++) {
      if (strstr(str, opts[i].name))
         #endif
   }
         /**
   * Initialize debugging variables from env vars.
   */
   void
   _mesa_init_debug( struct gl_context *ctx )
   {
      set_debug_flags(getenv("MESA_DEBUG"));
      }
         /*
   * Write ppm file
   */
   static void
   write_ppm(const char *filename, const GLubyte *buffer, int width, int height,
         {
      FILE *f = fopen( filename, "w" );
   if (f) {
      int x, y;
   const GLubyte *ptr = buffer;
   fprintf(f,"P6\n");
   fprintf(f,"# ppm-file created by osdemo.c\n");
   fprintf(f,"%i %i\n", width,height);
   fprintf(f,"255\n");
   fclose(f);
   f = fopen( filename, "ab" );  /* reopen in binary append mode */
   if (!f) {
      fprintf(stderr, "Error while reopening %s in write_ppm()\n",
            }
   for (y=0; y < height; y++) {
      for (x = 0; x < width; x++) {
      int yy = invert ? (height - 1 - y) : y;
   int i = (yy * width + x) * comps;
   fputc(ptr[i+rcomp], f); /* write red */
   fputc(ptr[i+gcomp], f); /* write green */
         }
      }
   else {
            }
         /**
   * Write a texture image to a ppm file.
   * \param face  cube face in [0,5]
   * \param level  mipmap level
   */
   static void
   write_texture_image(struct gl_texture_object *texObj,
         {
      struct gl_texture_image *img = texObj->Image[face][level];
   if (img) {
      GET_CURRENT_CONTEXT(ctx);
   struct gl_pixelstore_attrib store;
   GLubyte *buffer;
            buffer = malloc(img->Width * img->Height
            store = ctx->Pack; /* save */
            st_GetTexSubImage(ctx,
                  /* make filename */
            printf("  Writing image level %u to %s\n", level, s);
                           }
         /**
   * Write renderbuffer image to a ppm file.
   */
   void
   _mesa_write_renderbuffer_image(const struct gl_renderbuffer *rb)
   {
      GET_CURRENT_CONTEXT(ctx);
   GLubyte *buffer;
   char s[100];
            if (rb->_BaseFormat == GL_RGB ||
      rb->_BaseFormat == GL_RGBA) {
   format = GL_RGBA;
      }
   else if (rb->_BaseFormat == GL_DEPTH_STENCIL) {
      format = GL_DEPTH_STENCIL;
      }
   else {
      _mesa_debug(NULL,
               "Unsupported BaseFormat 0x%x in "
                        st_ReadPixels(ctx, 0, 0, rb->Width, rb->Height,
            /* make filename */
   snprintf(s, sizeof(s), "/tmp/renderbuffer%u.ppm", rb->Name);
                                          }
         /** How many texture images (mipmap levels, faces) to write to files */
   #define WRITE_NONE 0
   #define WRITE_ONE  1
   #define WRITE_ALL  2
      static GLuint WriteImages;
         static void
   dump_texture(struct gl_texture_object *texObj, GLuint writeImages)
   {
      const GLuint numFaces = texObj->Target == GL_TEXTURE_CUBE_MAP ? 6 : 1;
   GLboolean written = GL_FALSE;
            printf("Texture %u\n", texObj->Name);
   printf("  Target %s\n", tex_target_name(texObj->Target));
   for (i = 0; i < MAX_TEXTURE_LEVELS; i++) {
      for (j = 0; j < numFaces; j++) {
      struct gl_texture_image *texImg = texObj->Image[j][i];
   if (texImg) {
   j, i,
   texImg->Width, texImg->Height, texImg->Depth,
   _mesa_get_format_name(texImg->TexFormat));
            if (writeImages == WRITE_ALL ||
      (writeImages == WRITE_ONE && !written)) {
   write_texture_image(texObj, j, i);
                  }
         /**
   * Dump a single texture.
   */
   void
   _mesa_dump_texture(GLuint texture, GLuint writeImages)
   {
      GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_object *texObj = _mesa_lookup_texture(ctx, texture);
   if (texObj) {
            }
         static void
   dump_texture_cb(void *data, UNUSED void *userData)
   {
      struct gl_texture_object *texObj = (struct gl_texture_object *) data;
      }
         /**
   * Print basic info about all texture objext to stdout.
   * If dumpImages is true, write PPM of level[0] image to a file.
   */
   void
   _mesa_dump_textures(GLuint writeImages)
   {
      GET_CURRENT_CONTEXT(ctx);
   WriteImages = writeImages;
      }
         static void
   dump_renderbuffer(const struct gl_renderbuffer *rb, GLboolean writeImage)
   {
      printf("Renderbuffer %u: %u x %u  IntFormat = %s\n",
   rb->Name, rb->Width, rb->Height,
   _mesa_enum_to_string(rb->InternalFormat));
   if (writeImage) {
            }
         static void
   dump_renderbuffer_cb(void *data, UNUSED void *userData)
   {
      const struct gl_renderbuffer *rb = (const struct gl_renderbuffer *) data;
      }
         /**
   * Print basic info about all renderbuffers to stdout.
   * If dumpImages is true, write PPM of level[0] image to a file.
   */
   void
   _mesa_dump_renderbuffers(GLboolean writeImages)
   {
      GET_CURRENT_CONTEXT(ctx);
   WriteImages = writeImages;
      }
            void
   _mesa_dump_color_buffer(const char *filename)
   {
      GET_CURRENT_CONTEXT(ctx);
   const GLuint w = ctx->DrawBuffer->Width;
   const GLuint h = ctx->DrawBuffer->Height;
                     _mesa_PushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
   _mesa_PixelStorei(GL_PACK_ALIGNMENT, 1);
                     printf("ReadBuffer %p 0x%x  DrawBuffer %p 0x%x\n",
   (void *) ctx->ReadBuffer->_ColorReadBuffer,
   ctx->ReadBuffer->ColorReadBuffer,
   (void *) ctx->DrawBuffer->_ColorDrawBuffers[0],
   ctx->DrawBuffer->ColorDrawBuffer[0]);
   printf("Writing %d x %d color buffer to %s\n", w, h, filename);
                        }
         void
   _mesa_dump_depth_buffer(const char *filename)
   {
      GET_CURRENT_CONTEXT(ctx);
   const GLuint w = ctx->DrawBuffer->Width;
   const GLuint h = ctx->DrawBuffer->Height;
   GLuint *buf;
   GLubyte *buf2;
            buf = malloc(w * h * 4);  /* 4 bpp */
            _mesa_PushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
   _mesa_PixelStorei(GL_PACK_ALIGNMENT, 1);
                     /* spread 24 bits of Z across R, G, B */
   for (i = 0; i < w * h; i++) {
      buf2[i*3+0] = (buf[i] >> 24) & 0xff;
   buf2[i*3+1] = (buf[i] >> 16) & 0xff;
               printf("Writing %d x %d depth buffer to %s\n", w, h, filename);
                     free(buf);
      }
         void
   _mesa_dump_stencil_buffer(const char *filename)
   {
      GET_CURRENT_CONTEXT(ctx);
   const GLuint w = ctx->DrawBuffer->Width;
   const GLuint h = ctx->DrawBuffer->Height;
   GLubyte *buf;
   GLubyte *buf2;
            buf = malloc(w * h);  /* 1 bpp */
            _mesa_PushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
   _mesa_PixelStorei(GL_PACK_ALIGNMENT, 1);
                     for (i = 0; i < w * h; i++) {
      buf2[i*3+0] = buf[i];
   buf2[i*3+1] = (buf[i] & 127) * 2;
               printf("Writing %d x %d stencil buffer to %s\n", w, h, filename);
                     free(buf);
      }
         void
   _mesa_dump_image(const char *filename, const void *image, GLuint w, GLuint h,
         {
               if (format == GL_RGBA && type == GL_UNSIGNED_BYTE) {
         }
   else if (format == GL_BGRA && type == GL_UNSIGNED_BYTE) {
         }
   else if (format == GL_LUMINANCE_ALPHA && type == GL_UNSIGNED_BYTE) {
         }
   else if (format == GL_RED && type == GL_UNSIGNED_BYTE) {
         }
   else if (format == GL_RGBA && type == GL_FLOAT) {
      /* convert floats to ubyte */
   GLubyte *buf = malloc(w * h * 4 * sizeof(GLubyte));
   const GLfloat *f = (const GLfloat *) image;
   GLuint i;
   for (i = 0; i < w * h * 4; i++) {
         }
   write_ppm(filename, buf, w, h, 4, 0, 1, 2, invert);
      }
   else if (format == GL_RED && type == GL_FLOAT) {
      /* convert floats to ubyte */
   GLubyte *buf = malloc(w * h * sizeof(GLubyte));
   const GLfloat *f = (const GLfloat *) image;
   GLuint i;
   for (i = 0; i < w * h; i++) {
         }
   write_ppm(filename, buf, w, h, 1, 0, 0, 0, invert);
      }
   else {
      _mesa_problem(NULL,
               }
         /**
   * Quick and dirty function to "print" a texture to stdout.
   */
   void
   _mesa_print_texture(struct gl_context *ctx, struct gl_texture_image *img)
   {
      const GLint slice = 0;
   GLint srcRowStride;
   GLuint i, j, c;
            st_MapTextureImage(ctx, img, slice,
                  if (!data) {
         }
   else {
      /* XXX add more formats or make into a new format utility function */
   switch (img->TexFormat) {
      case MESA_FORMAT_A_UNORM8:
   case MESA_FORMAT_L_UNORM8:
   case MESA_FORMAT_I_UNORM8:
      c = 1;
      case MESA_FORMAT_LA_UNORM8:
      c = 2;
      case MESA_FORMAT_BGR_UNORM8:
   case MESA_FORMAT_RGB_UNORM8:
      c = 3;
      case MESA_FORMAT_A8B8G8R8_UNORM:
   case MESA_FORMAT_B8G8R8A8_UNORM:
      c = 4;
      default:
      _mesa_problem(NULL, "error in PrintTexture\n");
            for (i = 0; i < img->Height; i++) {
      for (j = 0; j < img->Width; j++) {
      if (c==1)
         else if (c==2)
         else if (c==3)
         else if (c==4)
            }
                              }
