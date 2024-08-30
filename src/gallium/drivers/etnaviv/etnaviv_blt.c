   /*
   * Copyright (c) 2017 Etnaviv Project
   * Copyright (C) 2017 Zodiac Inflight Innovations
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sub license,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   *
   * Authors:
   *    Wladimir J. van der Laan <laanwj@gmail.com>
   */
   #include "etnaviv_blt.h"
      #include "etnaviv_emit.h"
   #include "etnaviv_clear_blit.h"
   #include "etnaviv_context.h"
   #include "etnaviv_emit.h"
   #include "etnaviv_format.h"
   #include "etnaviv_resource.h"
   #include "etnaviv_surface.h"
   #include "etnaviv_translate.h"
      #include "util/u_math.h"
   #include "pipe/p_defines.h"
   #include "pipe/p_state.h"
   #include "util/u_blitter.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "util/u_surface.h"
      #include "hw/common_3d.xml.h"
   #include "hw/state_blt.xml.h"
   #include "hw/common.xml.h"
      #include <assert.h>
      static uint32_t
   etna_compatible_blt_format(enum pipe_format fmt)
   {
      /* YUYV and UYVY are blocksize 4, but 2 bytes per pixel */
   if (fmt == PIPE_FORMAT_YUYV || fmt == PIPE_FORMAT_UYVY)
            switch (util_format_get_blocksize(fmt)) {
   case 1: return BLT_FORMAT_R8;
   case 2: return BLT_FORMAT_R8G8;
   case 4: return BLT_FORMAT_A8R8G8B8;
   case 8: return BLT_FORMAT_A16R16G16B16;
   default: return ETNA_NO_MATCH;
      }
      static inline uint32_t
   blt_compute_stride_bits(const struct blt_imginfo *img)
   {
      return VIVS_BLT_DEST_STRIDE_TILING(img->tiling == ETNA_LAYOUT_LINEAR ? 0 : 3) | /* 1/3? */
         VIVS_BLT_DEST_STRIDE_FORMAT(img->format) |
   VIVS_BLT_DEST_STRIDE_STRIDE(img->stride) |
      }
      static inline uint32_t
   blt_compute_img_config_bits(const struct blt_imginfo *img, bool for_dest)
   {
      uint32_t tiling_bits = 0;
   if (img->tiling == ETNA_LAYOUT_SUPER_TILED) {
                  return BLT_IMAGE_CONFIG_TS_MODE(img->ts_mode) |
         COND(img->use_ts, BLT_IMAGE_CONFIG_TS) |
   COND(img->use_ts && img->ts_compress_fmt >= 0, BLT_IMAGE_CONFIG_COMPRESSION) |
   BLT_IMAGE_CONFIG_COMPRESSION_FORMAT(img->ts_compress_fmt) |
   COND(for_dest, BLT_IMAGE_CONFIG_UNK22) |
   BLT_IMAGE_CONFIG_SWIZ_R(0) | /* not used? */
   BLT_IMAGE_CONFIG_SWIZ_G(1) |
   BLT_IMAGE_CONFIG_SWIZ_B(2) |
      }
      static inline uint32_t
   blt_compute_swizzle_bits(const struct blt_imginfo *img, bool for_dest)
   {
      uint32_t swiz = VIVS_BLT_SWIZZLE_SRC_R(img->swizzle[0]) |
                        }
      /* Clear (part of) an image */
   static void
   emit_blt_clearimage(struct etna_cmd_stream *stream, const struct blt_clear_op *op)
   {
               etna_set_state(stream, VIVS_BLT_ENABLE, 0x00000001);
   assert(op->dest.bpp);
   etna_set_state(stream, VIVS_BLT_CONFIG, VIVS_BLT_CONFIG_CLEAR_BPP(op->dest.bpp-1));
   /* NB: blob sets format to 1 in dest/src config for clear, and the swizzle to RRRR.
   * does this matter? It seems to just be ignored. But if we run into issues with BLT
   * behaving stragely, it's something to look at.
   */
   etna_set_state(stream, VIVS_BLT_DEST_STRIDE, blt_compute_stride_bits(&op->dest));
   etna_set_state(stream, VIVS_BLT_DEST_CONFIG, blt_compute_img_config_bits(&op->dest, true));
   etna_set_state_reloc(stream, VIVS_BLT_DEST_ADDR, &op->dest.addr);
   etna_set_state(stream, VIVS_BLT_SRC_STRIDE, blt_compute_stride_bits(&op->dest));
   etna_set_state(stream, VIVS_BLT_SRC_CONFIG, blt_compute_img_config_bits(&op->dest, false));
   etna_set_state_reloc(stream, VIVS_BLT_SRC_ADDR, &op->dest.addr);
   etna_set_state(stream, VIVS_BLT_DEST_POS, VIVS_BLT_DEST_POS_X(op->rect_x) | VIVS_BLT_DEST_POS_Y(op->rect_y));
   etna_set_state(stream, VIVS_BLT_IMAGE_SIZE, VIVS_BLT_IMAGE_SIZE_WIDTH(op->rect_w) | VIVS_BLT_IMAGE_SIZE_HEIGHT(op->rect_h));
   etna_set_state(stream, VIVS_BLT_CLEAR_COLOR0, op->clear_value[0]);
   etna_set_state(stream, VIVS_BLT_CLEAR_COLOR1, op->clear_value[1]);
   etna_set_state(stream, VIVS_BLT_CLEAR_BITS0, op->clear_bits[0]);
   etna_set_state(stream, VIVS_BLT_CLEAR_BITS1, op->clear_bits[1]);
   if (op->dest.use_ts) {
      etna_set_state_reloc(stream, VIVS_BLT_DEST_TS, &op->dest.ts_addr);
   etna_set_state_reloc(stream, VIVS_BLT_SRC_TS, &op->dest.ts_addr);
   etna_set_state(stream, VIVS_BLT_DEST_TS_CLEAR_VALUE0, op->dest.ts_clear_value[0]);
   etna_set_state(stream, VIVS_BLT_DEST_TS_CLEAR_VALUE1, op->dest.ts_clear_value[1]);
   etna_set_state(stream, VIVS_BLT_SRC_TS_CLEAR_VALUE0, op->dest.ts_clear_value[0]);
      }
   etna_set_state(stream, VIVS_BLT_SET_COMMAND, 0x00000003);
   etna_set_state(stream, VIVS_BLT_COMMAND, VIVS_BLT_COMMAND_COMMAND_CLEAR_IMAGE);
   etna_set_state(stream, VIVS_BLT_SET_COMMAND, 0x00000003);
      }
      /* Copy (a subset of) an image to another image. */
   static void
   emit_blt_copyimage(struct etna_cmd_stream *stream, const struct blt_imgcopy_op *op)
   {
               etna_set_state(stream, VIVS_BLT_ENABLE, 0x00000001);
   etna_set_state(stream, VIVS_BLT_CONFIG,
         VIVS_BLT_CONFIG_SRC_ENDIAN(op->src.endian_mode) |
   etna_set_state(stream, VIVS_BLT_SRC_STRIDE, blt_compute_stride_bits(&op->src));
   etna_set_state(stream, VIVS_BLT_SRC_CONFIG, blt_compute_img_config_bits(&op->src, false));
   etna_set_state(stream, VIVS_BLT_SWIZZLE,
         blt_compute_swizzle_bits(&op->src, false) |
   etna_set_state(stream, VIVS_BLT_UNK140A0, 0x00040004);
   etna_set_state(stream, VIVS_BLT_UNK1409C, 0x00400040);
   if (op->src.use_ts) {
      etna_set_state_reloc(stream, VIVS_BLT_SRC_TS, &op->src.ts_addr);
   etna_set_state(stream, VIVS_BLT_SRC_TS_CLEAR_VALUE0, op->src.ts_clear_value[0]);
      }
   etna_set_state_reloc(stream, VIVS_BLT_SRC_ADDR, &op->src.addr);
   etna_set_state(stream, VIVS_BLT_DEST_STRIDE, blt_compute_stride_bits(&op->dest));
   etna_set_state(stream, VIVS_BLT_DEST_CONFIG,
         blt_compute_img_config_bits(&op->dest, true) |
   assert(!op->dest.use_ts); /* Dest TS path doesn't work for copies? */
   if (op->dest.use_ts) {
      etna_set_state_reloc(stream, VIVS_BLT_DEST_TS, &op->dest.ts_addr);
   etna_set_state(stream, VIVS_BLT_DEST_TS_CLEAR_VALUE0, op->dest.ts_clear_value[0]);
      }
   etna_set_state_reloc(stream, VIVS_BLT_DEST_ADDR, &op->dest.addr);
   etna_set_state(stream, VIVS_BLT_SRC_POS, VIVS_BLT_DEST_POS_X(op->src_x) | VIVS_BLT_DEST_POS_Y(op->src_y));
   etna_set_state(stream, VIVS_BLT_DEST_POS, VIVS_BLT_DEST_POS_X(op->dest_x) | VIVS_BLT_DEST_POS_Y(op->dest_y));
   etna_set_state(stream, VIVS_BLT_IMAGE_SIZE, VIVS_BLT_IMAGE_SIZE_WIDTH(op->rect_w) | VIVS_BLT_IMAGE_SIZE_HEIGHT(op->rect_h));
   etna_set_state(stream, VIVS_BLT_UNK14058, 0xffffffff);
   etna_set_state(stream, VIVS_BLT_UNK1405C, 0xffffffff);
   etna_set_state(stream, VIVS_BLT_SET_COMMAND, 0x00000003);
   etna_set_state(stream, VIVS_BLT_COMMAND, VIVS_BLT_COMMAND_COMMAND_COPY_IMAGE);
   etna_set_state(stream, VIVS_BLT_SET_COMMAND, 0x00000003);
      }
      /* Emit in-place resolve using BLT. */
   static void
   emit_blt_inplace(struct etna_cmd_stream *stream, const struct blt_inplace_op *op)
   {
      assert(op->bpp > 0 && util_is_power_of_two_nonzero(op->bpp));
   etna_cmd_stream_reserve(stream, 64*2); /* Never allow BLT sequences to be broken up */
   etna_set_state(stream, VIVS_BLT_ENABLE, 0x00000001);
   etna_set_state(stream, VIVS_BLT_CONFIG,
         VIVS_BLT_CONFIG_INPLACE_TS_MODE(op->ts_mode) |
   VIVS_BLT_CONFIG_INPLACE_BOTH |
   etna_set_state(stream, VIVS_BLT_DEST_TS_CLEAR_VALUE0, op->ts_clear_value[0]);
   etna_set_state(stream, VIVS_BLT_DEST_TS_CLEAR_VALUE1, op->ts_clear_value[1]);
   etna_set_state_reloc(stream, VIVS_BLT_DEST_ADDR, &op->addr);
   etna_set_state_reloc(stream, VIVS_BLT_DEST_TS, &op->ts_addr);
   etna_set_state(stream, 0x14068, op->num_tiles);
   etna_set_state(stream, VIVS_BLT_SET_COMMAND, 0x00000003);
   etna_set_state(stream, VIVS_BLT_COMMAND, 0x00000004);
   etna_set_state(stream, VIVS_BLT_SET_COMMAND, 0x00000003);
      }
      static void
   etna_blit_clear_color_blt(struct pipe_context *pctx, struct pipe_surface *dst,
         {
      struct etna_context *ctx = etna_context(pctx);
   struct etna_surface *surf = etna_surface(dst);
   uint64_t new_clear_value = etna_clear_blit_pack_rgba(surf->base.format, color);
            translate_samples_to_xyscale(surf->base.texture->nr_samples,
            struct etna_resource *res = etna_resource(surf->base.texture);
   struct blt_clear_op clr = {};
   clr.dest.addr.bo = res->bo;
   clr.dest.addr.offset = surf->offset;
   clr.dest.addr.flags = ETNA_RELOC_WRITE;
   clr.dest.bpp = util_format_get_blocksize(surf->base.format);
   clr.dest.stride = surf->level->stride;
            if (surf->level->ts_size) {
      clr.dest.use_ts = 1;
   clr.dest.ts_addr.bo = res->ts_bo;
   clr.dest.ts_addr.offset = surf->ts_offset;
   clr.dest.ts_addr.flags = ETNA_RELOC_WRITE;
   clr.dest.ts_clear_value[0] = new_clear_value;
   clr.dest.ts_clear_value[1] = new_clear_value >> 32;
   clr.dest.ts_mode = surf->level->ts_mode;
               clr.clear_value[0] = new_clear_value;
   clr.clear_value[1] = new_clear_value >> 32;
   clr.clear_bits[0] = 0xffffffff; /* TODO: Might want to clear only specific channels? */
   clr.clear_bits[1] = 0xffffffff;
   clr.rect_x = 0; /* What about scissors? */
   clr.rect_y = 0;
   clr.rect_w = surf->level->width * msaa_xscale;
                     /* This made the TS valid */
   if (surf->level->ts_size) {
      ctx->framebuffer.TS_COLOR_CLEAR_VALUE = new_clear_value;
            /* update clear color in SW meta area of the buffer if TS is exported */
   if (unlikely(new_clear_value != surf->level->clear_value &&
                  etna_resource_level_ts_mark_valid(surf->level);
               surf->level->clear_value = new_clear_value;
   resource_written(ctx, surf->base.texture);
      }
      static void
   etna_blit_clear_zs_blt(struct pipe_context *pctx, struct pipe_surface *dst,
         {
      struct etna_context *ctx = etna_context(pctx);
   struct etna_surface *surf = etna_surface(dst);
   uint32_t new_clear_value = translate_clear_depth_stencil(surf->base.format, depth, stencil);
   uint32_t new_clear_bits = 0, clear_bits_depth, clear_bits_stencil;
            translate_samples_to_xyscale(surf->base.texture->nr_samples,
            /* Get the channels to clear */
   switch (surf->base.format) {
   case PIPE_FORMAT_Z16_UNORM:
   case PIPE_FORMAT_X8Z24_UNORM:
      clear_bits_depth = 0xffffffff;
   clear_bits_stencil = 0x00000000;
      case PIPE_FORMAT_S8_UINT_Z24_UNORM:
      clear_bits_depth = 0xffffff00;
   clear_bits_stencil = 0x000000ff;
      default:
      clear_bits_depth = clear_bits_stencil = 0xffffffff;
               if (buffers & PIPE_CLEAR_DEPTH)
         if (buffers & PIPE_CLEAR_STENCIL)
            /* if all bits are cleared, update TS clear value */
   if (new_clear_bits == 0xffffffff)
            /* TODO unduplicate this */
   struct etna_resource *res = etna_resource(surf->base.texture);
   struct blt_clear_op clr = {};
   clr.dest.addr.bo = res->bo;
   clr.dest.addr.offset = surf->offset;
   clr.dest.addr.flags = ETNA_RELOC_WRITE;
   clr.dest.bpp = util_format_get_blocksize(surf->base.format);
   clr.dest.stride = surf->level->stride;
            if (surf->level->ts_size) {
      clr.dest.use_ts = 1;
   clr.dest.ts_addr.bo = res->ts_bo;
   clr.dest.ts_addr.offset = surf->ts_offset;
   clr.dest.ts_addr.flags = ETNA_RELOC_WRITE;
   clr.dest.ts_clear_value[0] = surf->level->clear_value;
   clr.dest.ts_clear_value[1] = surf->level->clear_value;
   clr.dest.ts_mode = surf->level->ts_mode;
               clr.clear_value[0] = new_clear_value;
   clr.clear_value[1] = new_clear_value;
   clr.clear_bits[0] = new_clear_bits;
   clr.clear_bits[1] = new_clear_bits;
   clr.rect_x = 0; /* What about scissors? */
   clr.rect_y = 0;
   clr.rect_w = surf->level->width * msaa_xscale;
                     /* This made the TS valid */
   if (surf->level->ts_size) {
      ctx->framebuffer.TS_DEPTH_CLEAR_VALUE = surf->level->clear_value;
   etna_resource_level_ts_mark_valid(surf->level);
               resource_written(ctx, surf->base.texture);
      }
      static void
   etna_clear_blt(struct pipe_context *pctx, unsigned buffers, const struct pipe_scissor_state *scissor_state,
         {
               if (!etna_render_condition_check(pctx))
            etna_set_state(ctx->stream, VIVS_GL_FLUSH_CACHE, 0x00000c23);
            if (buffers & PIPE_CLEAR_COLOR) {
      for (int idx = 0; idx < ctx->framebuffer_s.nr_cbufs; ++idx) {
                              if (!etna_resource(surf->prsc)->explicit_flush)
                  if ((buffers & PIPE_CLEAR_DEPTHSTENCIL) && ctx->framebuffer_s.zsbuf != NULL)
                     if ((buffers & PIPE_CLEAR_COLOR) && (buffers & PIPE_CLEAR_DEPTH))
         else
      }
      static bool
   etna_try_blt_blit(struct pipe_context *pctx,
         {
      struct etna_context *ctx = etna_context(pctx);
   struct etna_resource *src = etna_resource(blit_info->src.resource);
   struct etna_resource *dst = etna_resource(blit_info->dst.resource);
   int src_xscale, src_yscale, dst_xscale, dst_yscale;
            /* Ensure that the level is valid */
   assert(blit_info->src.level <= src->base.last_level);
            if (!translate_samples_to_xyscale(src->base.nr_samples, &src_xscale, &src_yscale))
         if (!translate_samples_to_xyscale(dst->base.nr_samples, &dst_xscale, &dst_yscale))
            /* BLT does not support upscaling */
   if ((src_xscale < dst_xscale) || (src_yscale < dst_yscale))
            if (src_xscale > dst_xscale)
         if (src_yscale > dst_yscale)
            /* The width/height are in pixels; they do not change as a result of
   * multi-sampling. So, when blitting from a 4x multisampled surface
   * to a non-multisampled surface, the width and height will be
   * identical. As we do not support scaling, reject different sizes.
   * TODO: could handle 2x downsample here with emit_blt_genmipmaps */
   if (blit_info->dst.box.width != blit_info->src.box.width ||
      blit_info->dst.box.height != abs(blit_info->src.box.height)) { /* allow y flip for glTexImage2D */
   DBG("scaling requested: source %dx%d destination %dx%d",
      blit_info->src.box.width, blit_info->src.box.height,
                  /* No masks - not sure if BLT can copy individual channels */
   unsigned mask = util_format_get_mask(blit_info->dst.format);
   if ((blit_info->mask & mask) != mask) {
      DBG("sub-mask requested: 0x%02x vs format mask 0x%02x", blit_info->mask, mask);
               /* Only support same format (used tiling/detiling) blits for now.
   * TODO: figure out which different-format blits are possible and test them
   *  - need to use correct swizzle
   *  - set sRGB bits correctly
   *  - avoid trying to convert between float/int formats?
   */
   if (blit_info->src.format != blit_info->dst.format)
            /* try to find a exact format match first */
   uint32_t format = translate_blt_format(blit_info->dst.format);
   /* When not resolving MSAA, but only doing a layout conversion, we can get
   * away with a fallback format of matching size.
   */
   if (format == ETNA_NO_MATCH && !downsample_x && !downsample_y)
         if (format == ETNA_NO_MATCH)
            if (blit_info->scissor_enable ||
      blit_info->dst.box.depth != blit_info->src.box.depth ||
   blit_info->dst.box.depth != 1) {
               struct etna_resource_level *src_lev = &src->levels[blit_info->src.level];
            /* if we asked for in-place resolve, return immediately if ts isn't valid
   * do this check separately because it applies when compression is used, but
   * we can't use inplace resolve path with compression
   */
   if (src == dst) {
      assert(!memcmp(&blit_info->src, &blit_info->dst, sizeof(blit_info->src)));
   if (!etna_resource_level_ts_valid(src_lev)) /* No TS, no worries */
               /* Flush destination, as the blit will invalidate any pending TS changes. */
   if (dst != src && etna_resource_level_needs_flush(dst_lev))
      etna_copy_resource(pctx, &dst->base, &dst->base,
         /* Kick off BLT here */
   if (src == dst && src_lev->ts_compress_fmt < 0) {
      /* Resolve-in-place */
   struct blt_inplace_op op = {};
   size_t tile_size = etna_screen_get_tile_size(ctx->screen, src_lev->ts_mode,
            op.addr.bo = src->bo;
   op.addr.offset = src_lev->offset + blit_info->src.box.z * src_lev->layer_stride;
   op.addr.flags = ETNA_RELOC_READ | ETNA_RELOC_WRITE;
   op.ts_addr.bo = src->ts_bo;
   op.ts_addr.offset = src_lev->ts_offset + blit_info->src.box.z * src_lev->ts_layer_stride;
   op.ts_addr.flags = ETNA_RELOC_READ;
   op.ts_clear_value[0] = src_lev->clear_value;
   op.ts_clear_value[1] = src_lev->clear_value >> 32;
   op.ts_mode = src_lev->ts_mode;
   op.num_tiles = DIV_ROUND_UP(src_lev->size, tile_size);
            etna_set_state(ctx->stream, VIVS_GL_FLUSH_CACHE, 0x00000c23);
   etna_set_state(ctx->stream, VIVS_TS_FLUSH_CACHE, 0x00000001);
      } else {
      /* Copy op */
            op.src.addr.bo = src->bo;
   op.src.addr.offset = src_lev->offset + blit_info->src.box.z * src_lev->layer_stride;
   op.src.addr.flags = ETNA_RELOC_READ;
   op.src.format = format;
   op.src.stride = src_lev->stride;
   op.src.tiling = src->layout;
   op.src.downsample_x = downsample_x;
   op.src.downsample_y = downsample_y;
   for (unsigned x=0; x<4; ++x)
            if (etna_resource_level_ts_valid(src_lev)) {
      op.src.use_ts = 1;
   op.src.ts_addr.bo = src->ts_bo;
   op.src.ts_addr.offset = src_lev->ts_offset + blit_info->src.box.z * src_lev->ts_layer_stride;
   op.src.ts_addr.flags = ETNA_RELOC_READ;
   op.src.ts_clear_value[0] = src_lev->clear_value;
   op.src.ts_clear_value[1] = src_lev->clear_value >> 32;
   op.src.ts_mode = src_lev->ts_mode;
               op.dest.addr.bo = dst->bo;
   op.dest.addr.offset = dst_lev->offset + blit_info->dst.box.z * dst_lev->layer_stride;
   op.dest.addr.flags = ETNA_RELOC_WRITE;
   op.dest.format = format;
   op.dest.stride = dst_lev->stride;
   op.dest.tiling = dst->layout;
   for (unsigned x=0; x<4; ++x)
            op.dest_x = blit_info->dst.box.x;
   op.dest_y = blit_info->dst.box.y;
   op.src_x = blit_info->src.box.x;
   op.src_y = blit_info->src.box.y;
   op.rect_w = blit_info->dst.box.width;
            assert(op.dest_x < dst_lev->padded_width);
   assert(op.dest_y < dst_lev->padded_height);
   assert((op.dest_x + op.rect_w) <= dst_lev->padded_width);
            if (blit_info->src.box.height < 0) { /* flipped? fix up base y */
      op.flip_y = 1;
               op.src_x *= src_xscale;
   op.src_y *= src_yscale;
   op.rect_w *= src_xscale;
            assert(op.src_x < src_lev->padded_width);
   assert(op.src_y < src_lev->padded_height);
   assert((op.src_x + op.rect_w) <= src_lev->padded_width);
            etna_set_state(ctx->stream, VIVS_GL_FLUSH_CACHE, 0x00000c23);
   etna_set_state(ctx->stream, VIVS_TS_FLUSH_CACHE, 0x00000001);
               /* Make FE wait for BLT, in case we want to do something with the image next.
   * This probably shouldn't be here, and depend on what is done with the resource.
   */
   etna_stall(ctx->stream, SYNC_RECIPIENT_FE, SYNC_RECIPIENT_BLT);
            resource_read(ctx, &src->base);
                     /* We don't need to mark the TS as invalid if this was just a flush without
   * compression, as in that case only clear tiles are filled and the tile
   * status still matches the blit target buffer. For compressed formats the
   * tiles are decompressed, so tile status doesn't match anymore.
   */
   if (src != dst || src_lev->ts_compress_fmt >= 0)
               }
      void
   etna_clear_blit_blt_init(struct pipe_context *pctx)
   {
               DBG("etnaviv: Using BLT blit engine");
   pctx->clear = etna_clear_blt;
      }
