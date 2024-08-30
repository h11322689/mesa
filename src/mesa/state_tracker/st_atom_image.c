   /**************************************************************************
   *
   * Copyright 2016 Ilia Mirkin. All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
         #include "main/shaderimage.h"
   #include "program/prog_parameter.h"
   #include "program/prog_print.h"
   #include "compiler/glsl/ir_uniform.h"
      #include "pipe/p_context.h"
   #include "pipe/p_defines.h"
   #include "util/u_inlines.h"
   #include "util/u_surface.h"
   #include "cso_cache/cso_context.h"
      #include "st_cb_texture.h"
   #include "st_debug.h"
   #include "st_texture.h"
   #include "st_context.h"
   #include "st_atom.h"
   #include "st_program.h"
   #include "st_format.h"
      /**
   * Convert a gl_image_unit object to a pipe_image_view object.
   */
   void
   st_convert_image(const struct st_context *st, const struct gl_image_unit *u,
         {
                        switch (u->Access) {
   case GL_READ_ONLY:
      img->access = PIPE_IMAGE_ACCESS_READ;
      case GL_WRITE_ONLY:
      img->access = PIPE_IMAGE_ACCESS_WRITE;
      case GL_READ_WRITE:
      img->access = PIPE_IMAGE_ACCESS_READ_WRITE;
      default:
                  img->shader_access = 0;
   if (!(shader_access & ACCESS_NON_READABLE))
         if (!(shader_access & ACCESS_NON_WRITEABLE))
         if (shader_access & ACCESS_COHERENT)
         if (shader_access & ACCESS_VOLATILE)
            if (stObj->Target == GL_TEXTURE_BUFFER) {
      struct gl_buffer_object *stbuf = stObj->BufferObject;
            if (!stbuf || !stbuf->buffer) {
      memset(img, 0, sizeof(*img));
      }
            base = stObj->BufferOffset;
   assert(base < buf->width0);
            img->resource = stbuf->buffer;
   img->u.buf.offset = base;
      } else {
      if (!st_finalize_texture(st->ctx, st->pipe, u->TexObj, 0) ||
      !stObj->pt) {
   memset(img, 0, sizeof(*img));
               img->resource = stObj->pt;
   img->u.tex.level = u->Level + stObj->Attrib.MinLevel;
   img->u.tex.single_layer_view = !u->Layered;
   assert(img->u.tex.level <= img->resource->last_level);
   if (stObj->pt->target == PIPE_TEXTURE_3D) {
      if (u->Layered) {
      img->u.tex.first_layer = 0;
      } else {
      img->u.tex.first_layer = u->_Layer;
         } else {
      img->u.tex.first_layer = u->_Layer + stObj->Attrib.MinLayer;
   img->u.tex.last_layer = u->_Layer + stObj->Attrib.MinLayer;
   if (u->Layered && img->resource->array_size > 1) {
      if (stObj->Immutable)
         else
               }
      /**
   * Get a pipe_image_view object from an image unit.
   */
   void
   st_convert_image_from_unit(const struct st_context *st,
                     {
               if (!_mesa_is_image_unit_valid(st->ctx, u)) {
      memset(img, 0, sizeof(*img));
                  }
      static void
   st_bind_images(struct st_context *st, struct gl_program *prog,
         {
      unsigned i;
            if (!prog || !st->pipe->set_shader_images)
                     for (i = 0; i < num_images; i++) {
               st_convert_image_from_unit(st, img, prog->sh.ImageUnits[i],
               struct pipe_context *pipe = st->pipe;
   unsigned last_num_images = st->state.num_images[shader_type];
   unsigned unbind_slots = last_num_images > num_images ?
         pipe->set_shader_images(pipe, shader_type, 0, num_images, unbind_slots,
            }
      void st_bind_vs_images(struct st_context *st)
   {
      struct gl_program *prog =
               }
      void st_bind_fs_images(struct st_context *st)
   {
      struct gl_program *prog =
               }
      void st_bind_gs_images(struct st_context *st)
   {
      struct gl_program *prog =
               }
      void st_bind_tcs_images(struct st_context *st)
   {
      struct gl_program *prog =
               }
      void st_bind_tes_images(struct st_context *st)
   {
      struct gl_program *prog =
               }
      void st_bind_cs_images(struct st_context *st)
   {
      struct gl_program *prog =
               }
