   /*
   * Copyright 2015 Advanced Micro Devices, Inc.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   */
      #include "state_tracker/st_context.h"
   #include "state_tracker/st_cb_bitmap.h"
   #include "state_tracker/st_cb_copyimage.h"
   #include "state_tracker/st_cb_texture.h"
   #include "state_tracker/st_texture.h"
   #include "state_tracker/st_util.h"
      #include "util/u_box.h"
   #include "util/format/u_format.h"
   #include "util/u_inlines.h"
         /**
   * Return an equivalent canonical format without "X" channels.
   *
   * Copying between incompatible formats is easier when the format is
   * canonicalized, meaning that it is in a standard form.
   *
   * The returned format has the same component sizes and swizzles as
   * the source format, the type is changed to UINT or UNORM, depending on
   * which one has the most swizzle combinations in their group.
   *
   * If it's not an array format, return a memcpy-equivalent array format.
   *
   * The key feature is that swizzled versions of formats of the same
   * component size always return the same component type.
   *
   * X returns A.
   * Luminance, intensity, alpha, depth, stencil, and 8-bit and 16-bit packed
   * formats are not supported. (same as ARB_copy_image)
   */
   static enum pipe_format
   get_canonical_format(struct pipe_screen *screen,
         {
      const struct util_format_description *desc =
            /* Packed formats. Return the equivalent array format. */
   if (format == PIPE_FORMAT_R11G11B10_FLOAT ||
      format == PIPE_FORMAT_R9G9B9E5_FLOAT)
         if (desc->nr_channels == 4 &&
      desc->channel[0].size == 10 &&
   desc->channel[1].size == 10 &&
   desc->channel[2].size == 10 &&
   desc->channel[3].size == 2) {
   if (desc->swizzle[0] == PIPE_SWIZZLE_X &&
      desc->swizzle[1] == PIPE_SWIZZLE_Y &&
                        #define RETURN_FOR_SWIZZLE1(x, format) \
      if (desc->swizzle[0] == PIPE_SWIZZLE_##x) \
      return (screen->get_canonical_format ? \
            #define RETURN_FOR_SWIZZLE2(x, y, format) \
      if (desc->swizzle[0] == PIPE_SWIZZLE_##x && \
      desc->swizzle[1] == PIPE_SWIZZLE_##y) \
   return (screen->get_canonical_format ? \
            #define RETURN_FOR_SWIZZLE3(x, y, z, format) \
      if (desc->swizzle[0] == PIPE_SWIZZLE_##x && \
      desc->swizzle[1] == PIPE_SWIZZLE_##y && \
   desc->swizzle[2] == PIPE_SWIZZLE_##z) \
   return (screen->get_canonical_format ? \
            #define RETURN_FOR_SWIZZLE4(x, y, z, w, format) \
      if (desc->swizzle[0] == PIPE_SWIZZLE_##x && \
      desc->swizzle[1] == PIPE_SWIZZLE_##y && \
   desc->swizzle[2] == PIPE_SWIZZLE_##z && \
   desc->swizzle[3] == PIPE_SWIZZLE_##w) \
   return (screen->get_canonical_format ? \
               /* Array formats. */
   if (desc->is_array) {
      switch (desc->nr_channels) {
   case 1:
      switch (desc->channel[0].size) {
   case 8:
                  case 16:
                  case 32:
      RETURN_FOR_SWIZZLE1(X, PIPE_FORMAT_R32_UINT);
                  case 2:
      switch (desc->channel[0].size) {
   case 8:
      /* All formats in each group must be of the same type.
   * We can't use UINT for R8G8 while using UNORM for G8R8.
   */
   RETURN_FOR_SWIZZLE2(X, Y, PIPE_FORMAT_R8G8_UNORM);
               case 16:
      RETURN_FOR_SWIZZLE2(X, Y, PIPE_FORMAT_R16G16_UNORM);
               case 32:
      RETURN_FOR_SWIZZLE2(X, Y, PIPE_FORMAT_R32G32_UINT);
                  case 3:
      switch (desc->channel[0].size) {
   case 8:
                  case 16:
                  case 32:
      RETURN_FOR_SWIZZLE3(X, Y, Z, PIPE_FORMAT_R32G32B32_UINT);
                  case 4:
      switch (desc->channel[0].size) {
   case 8:
      RETURN_FOR_SWIZZLE4(X, Y, Z, W, PIPE_FORMAT_R8G8B8A8_UNORM);
   RETURN_FOR_SWIZZLE4(X, Y, Z, 1, PIPE_FORMAT_R8G8B8A8_UNORM);
   RETURN_FOR_SWIZZLE4(Z, Y, X, W, PIPE_FORMAT_B8G8R8A8_UNORM);
   RETURN_FOR_SWIZZLE4(Z, Y, X, 1, PIPE_FORMAT_B8G8R8A8_UNORM);
   RETURN_FOR_SWIZZLE4(W, Z, Y, X, PIPE_FORMAT_A8B8G8R8_UNORM);
   RETURN_FOR_SWIZZLE4(W, Z, Y, 1, PIPE_FORMAT_A8B8G8R8_UNORM);
   RETURN_FOR_SWIZZLE4(Y, Z, W, X, PIPE_FORMAT_A8R8G8B8_UNORM);
               case 16:
      RETURN_FOR_SWIZZLE4(X, Y, Z, W, PIPE_FORMAT_R16G16B16A16_UINT);
               case 32:
      RETURN_FOR_SWIZZLE4(X, Y, Z, W, PIPE_FORMAT_R32G32B32A32_UINT);
   RETURN_FOR_SWIZZLE4(X, Y, Z, 1, PIPE_FORMAT_R32G32B32A32_UINT);
                  assert(!"unknown array format");
               assert(!"unknown packed format");
      }
      /**
   * Return true if the swizzle is XYZW in case of a 4-channel format,
   * XY in case of a 2-channel format, or X in case of a 1-channel format.
   */
   static bool
   has_identity_swizzle(const struct util_format_description *desc)
   {
               for (i = 0; i < desc->nr_channels; i++)
      if (desc->swizzle[i] != PIPE_SWIZZLE_X + i)
            }
      /**
   * Return a canonical format for the given bits and channel size.
   */
   static enum pipe_format
   canonical_format_from_bits(struct pipe_screen *screen,
               {
      switch (bits) {
   case 8:
      if (channel_size == 8)
               case 16:
      if (channel_size == 8)
         if (channel_size == 16)
               case 32:
      if (channel_size == 8)
         if (channel_size == 16)
         if (channel_size == 32)
               case 64:
      if (channel_size == 16)
         if (channel_size == 32)
               case 128:
      if (channel_size == 32)
                     assert(!"impossible format");
      }
      static void
   blit(struct pipe_context *pipe,
      struct pipe_resource *dst,
   enum pipe_format dst_format,
   unsigned dst_level,
   unsigned dstx, unsigned dsty, unsigned dstz,
   struct pipe_resource *src,
   enum pipe_format src_format,
   unsigned src_level,
      {
               blit.src.resource = src;
   blit.dst.resource = dst;
   blit.src.format = src_format;
   blit.dst.format = dst_format;
   blit.src.level = src_level;
   blit.dst.level = dst_level;
   blit.src.box = *src_box;
   u_box_3d(dstx, dsty, dstz, src_box->width, src_box->height,
         blit.mask = PIPE_MASK_RGBA;
               }
      static void
   swizzled_copy(struct pipe_context *pipe,
               struct pipe_resource *dst,
   unsigned dst_level,
   unsigned dstx, unsigned dsty, unsigned dstz,
   struct pipe_resource *src,
   {
      const struct util_format_description *src_desc, *dst_desc;
   unsigned bits;
            /* Get equivalent canonical formats. Those are always array formats and
   * copying between compatible canonical formats behaves either like
   * memcpy or like swizzled memcpy. The idea is that we won't have to care
   * about the channel type from this point on.
   * Only the swizzle and channel size.
   */
   blit_src_format = get_canonical_format(pipe->screen, src->format);
            assert(blit_src_format != PIPE_FORMAT_NONE);
            src_desc = util_format_description(blit_src_format);
            assert(src_desc->block.bits == dst_desc->block.bits);
            if (dst_desc->channel[0].size == src_desc->channel[0].size) {
      /* Only the swizzle is different, which means we can just blit,
   * e.g. RGBA -> BGRA.
      } else if (has_identity_swizzle(src_desc)) {
      /* Src is unswizzled and dst can be swizzled, so src is typecast
   * to an equivalent dst-compatible format.
   * e.g. R32 -> BGRA8 is realized as RGBA8 -> BGRA8
   */
   blit_src_format =
      } else if (has_identity_swizzle(dst_desc)) {
      /* Dst is unswizzled and src can be swizzled, so dst is typecast
   * to an equivalent src-compatible format.
   * e.g. BGRA8 -> R32 is realized as BGRA8 -> RGBA8
   */
   blit_dst_format =
      } else {
      assert(!"This should have been handled by handle_complex_copy.");
               blit(pipe, dst, blit_dst_format, dst_level, dstx, dsty, dstz,
      }
      static bool
   same_size_and_swizzle(const struct util_format_description *d1,
         {
               if (d1->layout != d2->layout ||
      d1->nr_channels != d2->nr_channels ||
   d1->is_array != d2->is_array)
         for (i = 0; i < d1->nr_channels; i++) {
      if (d1->channel[i].size != d2->channel[i].size)
            if (d1->swizzle[i] <= PIPE_SWIZZLE_W &&
      d2->swizzle[i] <= PIPE_SWIZZLE_W &&
   d1->swizzle[i] != d2->swizzle[i])
               }
      static struct pipe_resource *
   create_texture(struct pipe_screen *screen, enum pipe_format format,
               {
               memset(&templ, 0, sizeof(templ));
   templ.format = format;
   templ.width0 = width;
   templ.height0 = height;
   templ.depth0 = 1;
   templ.array_size = depth;
   templ.nr_samples = nr_samples;
   templ.nr_storage_samples = nr_storage_samples;
   templ.usage = PIPE_USAGE_DEFAULT;
            if (depth > 1)
         else
               }
      /**
   * Handle complex format conversions using 2 blits with a temporary texture
   * in between, e.g. blitting from B10G10R10A2 to G16R16.
   *
   * This example is implemented this way:
   * 1) First, blit from B10G10R10A2 to R10G10B10A2, which is canonical, so it
   *    can be reinterpreted as a different canonical format of the same bpp,
   *    such as R16G16. This blit only swaps R and B 10-bit components.
   * 2) Finally, blit the result, which is R10G10B10A2, as R16G16 to G16R16.
   *    This blit only swaps R and G 16-bit components.
   */
   static bool
   handle_complex_copy(struct pipe_context *pipe,
                     struct pipe_resource *dst,
   unsigned dst_level,
   unsigned dstx, unsigned dsty, unsigned dstz,
   struct pipe_resource *src,
   unsigned src_level,
   {
      struct pipe_box temp_box;
   struct pipe_resource *temp = NULL;
   const struct util_format_description *src_desc, *dst_desc;
   const struct util_format_description *canon_desc, *noncanon_desc;
   bool src_is_canon;
   bool src_is_noncanon;
   bool dst_is_canon;
            src_desc = util_format_description(src->format);
   dst_desc = util_format_description(dst->format);
   canon_desc = util_format_description(canon_format);
            src_is_canon = same_size_and_swizzle(src_desc, canon_desc);
   dst_is_canon = same_size_and_swizzle(dst_desc, canon_desc);
   src_is_noncanon = same_size_and_swizzle(src_desc, noncanon_desc);
            if (src_is_noncanon) {
      /* Simple case - only types differ (e.g. UNORM and UINT). */
   if (dst_is_noncanon) {
      blit(pipe, dst, noncanon_format, dst_level, dstx, dsty, dstz, src,
                     /* Simple case - only types and swizzles differ. */
   if (dst_is_canon) {
      blit(pipe, dst, canon_format, dst_level, dstx, dsty, dstz, src,
                     /* Use the temporary texture. Src is converted to a canonical format,
   * then proceed the generic swizzled_copy.
   */
   temp = create_texture(pipe->screen, canon_format, src->nr_samples,
                  u_box_3d(0, 0, 0, src_box->width, src_box->height, src_box->depth,
            blit(pipe, temp, canon_format, 0, 0, 0, 0, src, noncanon_format,
         swizzled_copy(pipe, dst, dst_level, dstx, dsty, dstz, temp, 0,
         pipe_resource_reference(&temp, NULL);
               if (dst_is_noncanon) {
      /* Simple case - only types and swizzles differ. */
   if (src_is_canon) {
      blit(pipe, dst, noncanon_format, dst_level, dstx, dsty, dstz, src,
                     /* Use the temporary texture. First, use the generic copy, but use
   * a canonical format in the destination. Then convert */
   temp = create_texture(pipe->screen, canon_format, dst->nr_samples,
                  u_box_3d(0, 0, 0, src_box->width, src_box->height, src_box->depth,
            swizzled_copy(pipe, temp, 0, 0, 0, 0, src, src_level, src_box);
   blit(pipe, dst, noncanon_format, dst_level, dstx, dsty, dstz, temp,
         pipe_resource_reference(&temp, NULL);
                  }
      static void
   copy_image(struct pipe_context *pipe,
            struct pipe_resource *dst,
   unsigned dst_level,
   unsigned dstx, unsigned dsty, unsigned dstz,
   struct pipe_resource *src,
      {
      if (src->format == dst->format ||
      util_format_is_compressed(src->format) ||
   util_format_is_compressed(dst->format)) {
   pipe->resource_copy_region(pipe, dst, dst_level, dstx, dsty, dstz,
                     /* Copying to/from B10G10R10*2 needs 2 blits with R10G10B10A2
   * as a temporary texture in between.
   */
   if (handle_complex_copy(pipe, dst, dst_level, dstx, dsty, dstz, src,
                        /* Copying to/from G8R8 needs 2 blits with R8G8 as a temporary texture
   * in between.
   */
   if (handle_complex_copy(pipe, dst, dst_level, dstx, dsty, dstz, src,
                        /* Copying to/from G16R16 needs 2 blits with R16G16 as a temporary texture
   * in between.
   */
   if (handle_complex_copy(pipe, dst, dst_level, dstx, dsty, dstz, src,
                                 /* Simple copy, memcpy with swizzling, no format conversion. */
   swizzled_copy(pipe, dst, dst_level, dstx, dsty, dstz, src, src_level,
      }
      static void
   fallback_copy_image(struct st_context *st,
                     struct gl_texture_image *dst_image,
   struct pipe_resource *dst_res,
   int dst_x, int dst_y, int dst_z,
   struct gl_texture_image *src_image,
   {
      uint8_t *dst, *src;
   int dst_stride, src_stride;
   struct pipe_transfer *dst_transfer, *src_transfer;
            bool dst_is_compressed = dst_image && _mesa_is_format_compressed(dst_image->TexFormat);
            unsigned dst_blk_w = 1, dst_blk_h = 1, src_blk_w = 1, src_blk_h = 1;
   if (dst_image)
         if (src_image)
            unsigned dst_w = src_w;
   unsigned dst_h = src_h;
            if (src_is_compressed && !dst_is_compressed) {
      dst_w = DIV_ROUND_UP(dst_w, src_blk_w);
      } else if (!src_is_compressed && dst_is_compressed) {
      dst_w *= dst_blk_w;
      }
   if (src_is_compressed) {
                  if (src_image)
         else
            if (src_image == dst_image && src_z == dst_z) {
               /* calculate bounding-box of the two rectangles */
   int min_x = MIN2(src_x, dst_x);
   int min_y = MIN2(src_y, dst_y);
   int max_x = MAX2(src_x + src_w, dst_x + dst_w);
   int max_y = MAX2(src_y + src_h, dst_y + dst_h);
   st_MapTextureImage(
         st->ctx, dst_image, dst_z,
   min_x, min_y, max_x - min_x, max_y - min_y,
   src = dst;
            /* adjust pointers */
   int format_bytes = _mesa_get_format_bytes(dst_image->TexFormat);
   src += ((src_y - min_y) / src_blk_h) * src_stride;
   src += ((src_x - min_x) / src_blk_w) * format_bytes;
   dst += ((dst_y - min_y) / src_blk_h) * dst_stride;
      } else {
      if (dst_image) {
      st_MapTextureImage(
         st->ctx, dst_image, dst_z,
   dst_x, dst_y, dst_w, dst_h,
      } else {
      dst = pipe_texture_map(st->pipe, dst_res, 0, dst_z,
                                 if (src_image) {
      st_MapTextureImage(
         st->ctx, src_image, src_z,
      } else {
      src = pipe_texture_map(st->pipe, src_res, 0, src_z,
                                    for (int y = 0; y < lines; y++) {
      memcpy(dst, src, line_bytes);
   dst += dst_stride;
               if (dst_image) {
         } else {
                  if (src_image) {
      if (src_image != dst_image || src_z != dst_z)
      } else {
            }
      void
   st_CopyImageSubData(struct gl_context *ctx,
                     struct gl_texture_image *src_image,
   struct gl_renderbuffer *src_renderbuffer,
   int src_x, int src_y, int src_z,
   struct gl_texture_image *dst_image,
   {
      struct st_context *st = st_context(ctx);
   struct pipe_context *pipe = st->pipe;
   struct pipe_resource *src_res, *dst_res;
   struct pipe_box box;
   int src_level, dst_level;
            st_flush_bitmap_cache(st);
            if (src_image) {
      struct gl_texture_image *src = src_image;
   struct gl_texture_object *stObj = src_image->TexObject;
   src_res = src->pt;
   src_level = stObj->pt != src_res ? 0 : src_image->Level;
   src_z += src_image->Face;
   if (src_image->TexObject->Immutable) {
      src_level += src_image->TexObject->Attrib.MinLevel;
         } else {
      src_res = src_renderbuffer->texture;
               if (dst_image) {
      struct gl_texture_image *dst = dst_image;
   struct gl_texture_object *stObj = dst_image->TexObject;
   dst_res = dst->pt;
   dst_level = stObj->pt != dst_res ? 0 : dst_image->Level;
   dst_z += dst_image->Face;
   if (dst_image->TexObject->Immutable) {
      dst_level += dst_image->TexObject->Attrib.MinLevel;
         } else {
      dst_res = dst_renderbuffer->texture;
                        if ((src_image && st_compressed_format_fallback(st, src_image->TexFormat)) ||
      (dst_image && st_compressed_format_fallback(st, dst_image->TexFormat))) {
   fallback_copy_image(st, dst_image, dst_res, dst_x, dst_y, orig_dst_z,
            } else {
      copy_image(pipe, dst_res, dst_level, dst_x, dst_y, dst_z,
         }
