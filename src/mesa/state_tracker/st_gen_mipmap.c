   /**************************************************************************
   *
   * Copyright 2008 VMware, Inc.
   * All Rights Reserved.
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
         #include "main/errors.h"
      #include "main/mipmap.h"
   #include "main/teximage.h"
      #include "pipe/p_context.h"
   #include "pipe/p_defines.h"
   #include "util/u_inlines.h"
   #include "util/format/u_format.h"
   #include "util/u_gen_mipmap.h"
      #include "st_debug.h"
   #include "st_context.h"
   #include "st_texture.h"
   #include "st_util.h"
   #include "st_gen_mipmap.h"
   #include "st_cb_bitmap.h"
   #include "st_cb_texture.h"
      void
   st_generate_mipmap(struct gl_context *ctx, GLenum target,
         {
      struct st_context *st = st_context(ctx);
   struct pipe_resource *pt = st_get_texobj_resource(texObj);
   uint baseLevel = texObj->Attrib.BaseLevel;
   enum pipe_format format;
            if (!pt)
            if (texObj->Immutable)
            /* not sure if this ultimately actually should work,
                  /* find expected last mipmap level to generate*/
            if (texObj->Immutable)
            if (lastLevel == 0)
            st_flush_bitmap_cache(st);
            /* The texture isn't in a "complete" state yet so set the expected
   * lastLevel here, since it won't get done in st_finalize_texture().
   */
            if (!texObj->Immutable) {
               /* Temporarily set GenerateMipmap to true so that allocate_full_mipmap()
   * makes the right decision about full mipmap allocation.
   */
                              /* At this point, memory for all the texture levels has been
   * allocated.  However, the base level image may be in one resource
   * while the subsequent/smaller levels may be in another resource.
   * Finalizing the texture will copy the base images from the former
   * resource to the latter.
   *
   * After this, we'll have all mipmap levels in one resource.
   */
               pt = texObj->pt;
   if (!pt) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "mipmap generation");
                        if (pt->target == PIPE_TEXTURE_CUBE) {
         }
   else {
      first_layer = 0;
               if (texObj->surface_based)
         else
            if (texObj->Sampler.Attrib.sRGBDecode == GL_SKIP_DECODE_EXT)
            /* For fallback formats, we need to update both the compressed and
   * uncompressed images.
   */
   if (st_compressed_format_fallback(st, _mesa_base_tex_image(texObj)->TexFormat)) {
      _mesa_generate_mipmap(ctx, target, texObj);
               /* First see if the driver supports hardware mipmap generation,
   * if not then generate the mipmap by rendering/texturing.
   * If that fails, use the software fallback.
   */
   if (!st->screen->get_param(st->screen,
            !st->pipe->generate_mipmap(st->pipe, pt, format, baseLevel,
            if (!util_gen_mipmap(st->pipe, pt, format, baseLevel, lastLevel,
                     }
