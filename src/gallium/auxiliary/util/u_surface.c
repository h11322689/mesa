   /**************************************************************************
   *
   * Copyright 2009 VMware, Inc.  All Rights Reserved.
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
      /**
   * @file
   * Surface utility functions.
   *  
   * @author Brian Paul
   */
         #include "pipe/p_defines.h"
   #include "pipe/p_screen.h"
   #include "pipe/p_state.h"
      #include "util/format/u_format.h"
   #include "util/u_inlines.h"
   #include "util/u_rect.h"
   #include "util/u_surface.h"
   #include "util/u_pack_color.h"
   #include "util/u_memset.h"
      /**
   * Initialize a pipe_surface object.  'view' is considered to have
   * uninitialized contents.
   */
   void
   u_surface_default_template(struct pipe_surface *surf,
         {
                  }
         /**
   * Copy 3D box from one place to another.
   * Position and sizes are in pixels.
   */
   void
   util_copy_box(uint8_t * dst,
               enum pipe_format format,
   unsigned dst_stride, uint64_t dst_slice_stride,
   unsigned dst_x, unsigned dst_y, unsigned dst_z,
   unsigned width, unsigned height, unsigned depth,
   const uint8_t * src,
   {
      unsigned z;
   dst += dst_z * dst_slice_stride;
   src += src_z * src_slice_stride;
   for (z = 0; z < depth; ++z) {
      util_copy_rect(dst,
                  format,
   dst_stride,
   dst_x, dst_y,
               dst += dst_slice_stride;
         }
         void
   util_fill_rect(uint8_t * dst,
                  enum pipe_format format,
   unsigned dst_stride,
   unsigned dst_x,
   unsigned dst_y,
      {
      const struct util_format_description *desc = util_format_description(format);
   unsigned i, j;
   unsigned width_size;
   int blocksize = desc->block.bits / 8;
   int blockwidth = desc->block.width;
            assert(blocksize > 0);
   assert(blockwidth > 0);
            dst_x /= blockwidth;
   dst_y /= blockheight;
   width = (width + blockwidth - 1)/blockwidth;
            dst += dst_x * blocksize;
   dst += (uint64_t)dst_y * dst_stride;
            switch (blocksize) {
   case 1:
      if(dst_stride == width_size)
         else {
      for (i = 0; i < height; i++) {
      memset(dst, uc->ub, width_size);
         }
      case 2:
      for (i = 0; i < height; i++) {
      uint16_t *row = (uint16_t *)dst;
   for (j = 0; j < width; j++)
            }
      case 4:
      for (i = 0; i < height; i++) {
      util_memset32(dst, uc->ui[0], width);
      }
      case 8:
      for (i = 0; i < height; i++) {
      util_memset64(dst, ((uint64_t *)uc)[0], width);
      }
      default:
      for (i = 0; i < height; i++) {
      uint8_t *row = dst;
   for (j = 0; j < width; j++) {
      memcpy(row, uc, blocksize);
      }
      }
         }
         void
   util_fill_box(uint8_t * dst,
               enum pipe_format format,
   unsigned stride,
   uintptr_t layer_stride,
   unsigned x,
   unsigned y,
   unsigned z,
   unsigned width,
   unsigned height,
   {
      unsigned layer;
   dst += z * layer_stride;
   for (layer = z; layer < depth; layer++) {
      util_fill_rect(dst, format,
                     }
         /**
   * Fallback function for pipe->resource_copy_region().
   * We support copying between different formats (including compressed/
   * uncompressed) if the bytes per block or pixel matches.  If copying
   * compressed -> uncompressed, the dst region is reduced by the block
   * width, height.  If copying uncompressed -> compressed, the dest region
   * is expanded by the block width, height.  See GL_ARB_copy_image.
   * Note: (X,Y)=(0,0) is always the upper-left corner.
   */
   void
   util_resource_copy_region(struct pipe_context *pipe,
                           struct pipe_resource *dst,
   unsigned dst_level,
   {
      struct pipe_transfer *src_trans, *dst_trans;
   uint8_t *dst_map;
   const uint8_t *src_map;
   enum pipe_format src_format;
   enum pipe_format dst_format;
   struct pipe_box src_box, dst_box;
            assert(src && dst);
   if (!src || !dst)
            assert((src->target == PIPE_BUFFER && dst->target == PIPE_BUFFER) ||
            src_format = src->format;
            /* init src box */
            /* init dst box */
   dst_box.x = dst_x;
   dst_box.y = dst_y;
   dst_box.z = dst_z;
   dst_box.width  = src_box.width;
   dst_box.height = src_box.height;
            src_bs = util_format_get_blocksize(src_format);
   src_bw = util_format_get_blockwidth(src_format);
   src_bh = util_format_get_blockheight(src_format);
   dst_bs = util_format_get_blocksize(dst_format);
   dst_bw = util_format_get_blockwidth(dst_format);
            /* Note: all box positions and sizes are in pixels */
   if (src_bw > 1 && dst_bw == 1) {
      /* Copy from compressed to uncompressed.
   * Shrink dest box by the src block size.
   */
   dst_box.width /= src_bw;
      }
   else if (src_bw == 1 && dst_bw > 1) {
      /* Copy from uncompressed to compressed.
   * Expand dest box by the dest block size.
   */
   dst_box.width *= dst_bw;
      }
   else {
      /* compressed -> compressed or uncompressed -> uncompressed copy */
   assert(src_bw == dst_bw);
               assert(src_bs == dst_bs);
   if (src_bs != dst_bs) {
      /* This can happen if we fail to do format checking before hand.
   * Don't crash below.
   */
               /* check that region boxes are block aligned */
   assert(src_box.x % src_bw == 0);
   assert(src_box.y % src_bh == 0);
   assert(dst_box.x % dst_bw == 0);
            /* check that region boxes are not out of bounds */
   assert(src_box.x + src_box.width <= (int)u_minify(src->width0, src_level));
   assert(src_box.y + src_box.height <= (int)u_minify(src->height0, src_level));
   assert(dst_box.x + dst_box.width <= (int)u_minify(dst->width0, dst_level));
            /* check that total number of src, dest bytes match */
   assert((src_box.width / src_bw) * (src_box.height / src_bh) * src_bs ==
            if (dst->target == PIPE_BUFFER && src->target == PIPE_BUFFER) {
      src_map = pipe->buffer_map(pipe,
                           assert(src_map);
   if (!src_map) {
                  dst_map = pipe->buffer_map(pipe,
                                 assert(dst_map);
   if (!dst_map) {
                  assert(src_box.height == 1);
   assert(src_box.depth == 1);
               no_dst_map_buf:
         no_src_map_buf:
         } else {
      src_map = pipe->texture_map(pipe,
                           assert(src_map);
   if (!src_map) {
                  dst_map = pipe->texture_map(pipe,
                                 assert(dst_map);
   if (!dst_map) {
                  util_copy_box(dst_map,
               src_format,
   dst_trans->stride, dst_trans->layer_stride,
   0, 0, 0,
   src_box.width, src_box.height, src_box.depth,
               no_dst_map:
         no_src_map:
            }
      static void
   util_clear_color_texture_helper(struct pipe_transfer *dst_trans,
                           {
                                 util_fill_box(dst_map, format,
            }
      static void
   util_clear_color_texture(struct pipe_context *pipe,
                           struct pipe_resource *texture,
   enum pipe_format format,
   {
      struct pipe_transfer *dst_trans;
            dst_map = pipe_texture_map_3d(pipe,
                                 texture,
   if (!dst_map)
            if (dst_trans->stride > 0) {
      util_clear_color_texture_helper(dst_trans, dst_map, format, color,
      }
      }
         #define UBYTE_TO_USHORT(B) ((B) | ((B) << 8))
         /**
   * Fallback for pipe->clear_render_target() function.
   * XXX this looks too hackish to be really useful.
   * cpp > 4 looks like a gross hack at best...
   * Plus can't use these transfer fallbacks when clearing
   * multisampled surfaces for instance.
   * Clears all bound layers.
   */
   void
   util_clear_render_target(struct pipe_context *pipe,
                           {
      struct pipe_transfer *dst_trans;
            assert(dst->texture);
   if (!dst->texture)
            if (dst->texture->target == PIPE_BUFFER) {
      /*
   * The fill naturally works on the surface format, however
   * the transfer uses resource format which is just bytes for buffers.
   */
   unsigned dx, w;
   unsigned pixstride = util_format_get_blocksize(dst->format);
   dx = (dst->u.buf.first_element + dstx) * pixstride;
   w = width * pixstride;
   dst_map = pipe_texture_map(pipe,
                                 if (dst_map) {
      util_clear_color_texture_helper(dst_trans, dst_map, dst->format,
               }
   else {
      unsigned depth = dst->u.tex.last_layer - dst->u.tex.first_layer + 1;
   util_clear_color_texture(pipe, dst->texture, dst->format, color,
               }
      static void
   util_fill_zs_rect(uint8_t *dst_map,
                     enum pipe_format format,
   bool need_rmw,
   unsigned clear_flags,
   unsigned dst_stride,
   {
      unsigned i, j;
   switch (util_format_get_blocksize(format)) {
   case 1:
      assert(format == PIPE_FORMAT_S8_UINT);
   if(dst_stride == width)
         else {
      for (i = 0; i < height; i++) {
      memset(dst_map, (uint8_t) zstencil, width);
         }
      case 2:
      assert(format == PIPE_FORMAT_Z16_UNORM);
   for (i = 0; i < height; i++) {
      uint16_t *row = (uint16_t *)dst_map;
   for (j = 0; j < width; j++)
            }
      case 4:
      if (!need_rmw) {
      for (i = 0; i < height; i++) {
      util_memset32(dst_map, (uint32_t)zstencil, width);
         }
   else {
      uint32_t dst_mask;
   if (format == PIPE_FORMAT_Z24_UNORM_S8_UINT)
         else {
      assert(format == PIPE_FORMAT_S8_UINT_Z24_UNORM);
      }
   if (clear_flags & PIPE_CLEAR_DEPTH)
         for (i = 0; i < height; i++) {
      uint32_t *row = (uint32_t *)dst_map;
   for (j = 0; j < width; j++) {
      uint32_t tmp = *row & dst_mask;
      }
         }
      case 8:
      if (!need_rmw) {
      for (i = 0; i < height; i++) {
      util_memset64(dst_map, zstencil, width);
         }
   else {
               if (clear_flags & PIPE_CLEAR_DEPTH)
                        for (i = 0; i < height; i++) {
      uint64_t *row = (uint64_t *)dst_map;
   for (j = 0; j < width; j++) {
      uint64_t tmp = *row & ~src_mask;
      }
         }
      default:
      assert(0);
         }
      void
   util_fill_zs_box(uint8_t *dst,
                  enum pipe_format format,
   bool need_rmw,
   unsigned clear_flags,
   unsigned stride,
   unsigned layer_stride,
   unsigned width,
      {
               for (layer = 0; layer < depth; layer++) {
      util_fill_zs_rect(dst, format, need_rmw, clear_flags, stride,
               }
      static void
   util_clear_depth_stencil_texture(struct pipe_context *pipe,
                                       {
      struct pipe_transfer *dst_trans;
   uint8_t *dst_map;
            if ((clear_flags & PIPE_CLEAR_DEPTHSTENCIL) &&
      ((clear_flags & PIPE_CLEAR_DEPTHSTENCIL) != PIPE_CLEAR_DEPTHSTENCIL) &&
   util_format_is_depth_and_stencil(format))
         dst_map = pipe_texture_map_3d(pipe,
                                 texture,
   assert(dst_map);
   if (!dst_map)
                     util_fill_zs_box(dst_map, format, need_rmw, clear_flags,
                           }
         /* Try to clear the texture as a surface, returns true if successful.
   */
   static bool
   util_clear_texture_as_surface(struct pipe_context *pipe,
                           {
               tmpl.format = res->format;
   tmpl.u.tex.first_layer = box->z;
   tmpl.u.tex.last_layer = box->z + box->depth - 1;
            if (util_format_is_depth_or_stencil(res->format)) {
      if (!pipe->clear_depth_stencil)
            sf = pipe->create_surface(pipe, res, &tmpl);
   if (!sf)
            float depth = 0;
   uint8_t stencil = 0;
   unsigned clear = 0;
   const struct util_format_description *desc =
            if (util_format_has_depth(desc)) {
      clear |= PIPE_CLEAR_DEPTH;
      }
   if (util_format_has_stencil(desc)) {
      clear |= PIPE_CLEAR_STENCIL;
      }
   pipe->clear_depth_stencil(pipe, sf, clear, depth, stencil,
                     } else {
      if (!pipe->clear_render_target)
            if (!pipe->screen->is_format_supported(pipe->screen, tmpl.format,
                                          if (!pipe->screen->is_format_supported(pipe->screen, tmpl.format,
                           sf = pipe->create_surface(pipe, res, &tmpl);
   if (!sf)
            union pipe_color_union color;
   util_format_unpack_rgba(sf->format, color.ui, data, 1);
   pipe->clear_render_target(pipe, sf, &color, box->x, box->y,
                           }
      /* First attempt to clear using HW, fallback to SW if needed.
   */
   void
   u_default_clear_texture(struct pipe_context *pipe,
                           {
      struct pipe_screen *screen = pipe->screen;
   bool cleared = false;
            bool has_layers = screen->get_param(screen, PIPE_CAP_VS_INSTANCEID) &&
            if (has_layers) {
      cleared = util_clear_texture_as_surface(pipe, tex, level,
      } else {
      struct pipe_box layer = *box;
   layer.depth = 1;
   int l;
   for (l = box->z; l < box->z + box->depth; l++) {
      layer.z = l;
   cleared |= util_clear_texture_as_surface(pipe, tex, level,
         if (!cleared) {
      /* If one layer is cleared, all layers should also be clearable.
   * Therefore, if we fail on any later other than the first, it
   * is a bug somewhere.
   */
   assert(l == box->z);
                     /* Fallback to clearing it in SW if the HW paths failed. */
   if (!cleared)
      }
      void
   util_clear_texture_sw(struct pipe_context *pipe,
                           {
      const struct util_format_description *desc =
                  if (level > tex->last_level)
            if (util_format_is_depth_or_stencil(tex->format)) {
      unsigned clear = 0;
   float depth = 0.0f;
   uint8_t stencil = 0;
            if (util_format_has_depth(desc)) {
      clear |= PIPE_CLEAR_DEPTH;
               if (util_format_has_stencil(desc)) {
      clear |= PIPE_CLEAR_STENCIL;
                        util_clear_depth_stencil_texture(pipe, tex, tex->format, clear, zstencil,
            } else {
      union pipe_color_union color;
            util_clear_color_texture(pipe, tex, tex->format, &color, level,
               }
         /**
   * Fallback for pipe->clear_stencil() function.
   * sw fallback doesn't look terribly useful here.
   * Plus can't use these transfer fallbacks when clearing
   * multisampled surfaces for instance.
   * Clears all bound layers.
   */
   void
   util_clear_depth_stencil(struct pipe_context *pipe,
                           struct pipe_surface *dst,
   unsigned clear_flags,
   {
      uint64_t zstencil;
            assert(dst->texture);
   if (!dst->texture)
            zstencil = util_pack64_z_stencil(dst->format, depth, stencil);
   max_layer = dst->u.tex.last_layer - dst->u.tex.first_layer;
   util_clear_depth_stencil_texture(pipe, dst->texture, dst->format,
                  }
         /* Return if the box is totally inside the resource.
   */
   static bool
   is_box_inside_resource(const struct pipe_resource *res,
               {
               switch (res->target) {
   case PIPE_BUFFER:
      width = res->width0;
   height = 1;
   depth = 1;
      case PIPE_TEXTURE_1D:
      width = u_minify(res->width0, level);
   height = 1;
   depth = 1;
      case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_RECT:
      width = u_minify(res->width0, level);
   height = u_minify(res->height0, level);
   depth = 1;
      case PIPE_TEXTURE_3D:
      width = u_minify(res->width0, level);
   height = u_minify(res->height0, level);
   depth = u_minify(res->depth0, level);
      case PIPE_TEXTURE_CUBE:
      width = u_minify(res->width0, level);
   height = u_minify(res->height0, level);
   depth = 6;
      case PIPE_TEXTURE_1D_ARRAY:
      width = u_minify(res->width0, level);
   height = 1;
   depth = res->array_size;
      case PIPE_TEXTURE_2D_ARRAY:
      width = u_minify(res->width0, level);
   height = u_minify(res->height0, level);
   depth = res->array_size;
      case PIPE_TEXTURE_CUBE_ARRAY:
      width = u_minify(res->width0, level);
   height = u_minify(res->height0, level);
   depth = res->array_size;
   assert(res->array_size % 6 == 0);
      case PIPE_MAX_TEXTURE_TYPES:
                  return box->x >= 0 &&
         box->x + box->width <= (int) width &&
   box->y >= 0 &&
   box->y + box->height <= (int) height &&
      }
      static unsigned
   get_sample_count(const struct pipe_resource *res)
   {
         }
         /**
   * Check if a blit() command can be implemented with a resource_copy_region().
   * If tight_format_check is true, only allow the resource_copy_region() if
   * the blit src/dst formats are identical, ignoring the resource formats.
   * Otherwise, check for format casting and compatibility.
   */
   bool
   util_can_blit_via_copy_region(const struct pipe_blit_info *blit,
               {
               src_desc = util_format_description(blit->src.resource->format);
            if (tight_format_check) {
      /* no format conversions allowed */
   if (blit->src.format != blit->dst.format) {
            }
   else {
      /* do loose format compatibility checking */
   if ((blit->src.format != blit->dst.format ||
      src_desc != dst_desc) &&
   (blit->src.resource->format != blit->src.format ||
   blit->dst.resource->format != blit->dst.format ||
   !util_is_format_compatible(src_desc, dst_desc))) {
                           /* No masks, no filtering, no scissor, no blending */
   if ((blit->mask & mask) != mask ||
      blit->filter != PIPE_TEX_FILTER_NEAREST ||
   blit->scissor_enable ||
   blit->num_window_rectangles > 0 ||
   blit->alpha_blend ||
   (blit->render_condition_enable && render_condition_bound)) {
               /* Only the src box can have negative dims for flipping */
   assert(blit->dst.box.width >= 1);
   assert(blit->dst.box.height >= 1);
            /* No scaling or flipping */
   if (blit->src.box.width != blit->dst.box.width ||
      blit->src.box.height != blit->dst.box.height ||
   blit->src.box.depth != blit->dst.box.depth) {
               /* No out-of-bounds access. */
   if (!is_box_inside_resource(blit->src.resource, &blit->src.box,
            !is_box_inside_resource(blit->dst.resource, &blit->dst.box,
                     /* Sample counts must match. */
   if (get_sample_count(blit->src.resource) !=
      get_sample_count(blit->dst.resource)) {
                  }
         /**
   * Try to do a blit using resource_copy_region. The function calls
   * resource_copy_region if the blit description is compatible with it.
   *
   * It returns TRUE if the blit was done using resource_copy_region.
   *
   * It returns FALSE otherwise and the caller must fall back to a more generic
   * codepath for the blit operation. (e.g. by using u_blitter)
   */
   bool
   util_try_blit_via_copy_region(struct pipe_context *ctx,
               {
      if (util_can_blit_via_copy_region(blit, false, render_condition_bound)) {
      ctx->resource_copy_region(ctx, blit->dst.resource, blit->dst.level,
                              }
   else {
            }
