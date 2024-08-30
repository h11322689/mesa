   /**************************************************************************
   * 
   * Copyright 2007 VMware, Inc.
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
      /**
   * Texture tile caching.
   *
   * Author:
   *    Brian Paul
   */
      #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "util/u_tile.h"
   #include "util/format/u_format.h"
   #include "util/u_math.h"
   #include "sp_context.h"
   #include "sp_texture.h"
   #include "sp_tex_tile_cache.h"
               struct softpipe_tex_tile_cache *
   sp_create_tex_tile_cache( struct pipe_context *pipe )
   {
      struct softpipe_tex_tile_cache *tc;
            /* make sure max texture size works */
            tc = CALLOC_STRUCT( softpipe_tex_tile_cache );
   if (tc) {
      tc->pipe = pipe;
   for (pos = 0; pos < ARRAY_SIZE(tc->entries); pos++) {
         }
      }
      }
         void
   sp_destroy_tex_tile_cache(struct softpipe_tex_tile_cache *tc)
   {
      if (tc) {
               for (pos = 0; pos < ARRAY_SIZE(tc->entries); pos++) {
         }
   if (tc->transfer) {
         }
   if (tc->tex_trans) {
                        }
         /**
   * Invalidate all cached tiles for the cached texture.
   * Should be called when the texture is modified.
   */
   void
   sp_tex_tile_cache_validate_texture(struct softpipe_tex_tile_cache *tc)
   {
               assert(tc);
            for (i = 0; i < ARRAY_SIZE(tc->entries); i++) {
            }
      static bool
   sp_tex_tile_is_compat_view(struct softpipe_tex_tile_cache *tc,
         {
      if (!view)
         return (tc->texture == view->texture &&
         tc->format == view->format &&
   tc->swizzle_r == view->swizzle_r &&
   tc->swizzle_g == view->swizzle_g &&
      }
      /**
   * Specify the sampler view to cache.
   */
   void
   sp_tex_tile_cache_set_sampler_view(struct softpipe_tex_tile_cache *tc,
         {
      struct pipe_resource *texture = view ? view->texture : NULL;
                     if (!sp_tex_tile_is_compat_view(tc, view)) {
               if (tc->tex_trans_map) {
      tc->pipe->texture_unmap(tc->pipe, tc->tex_trans);
   tc->tex_trans = NULL;
               if (view) {
      tc->swizzle_r = view->swizzle_r;
   tc->swizzle_g = view->swizzle_g;
   tc->swizzle_b = view->swizzle_b;
   tc->swizzle_a = view->swizzle_a;
               /* mark as entries as invalid/empty */
   /* XXX we should try to avoid this when the teximage hasn't changed */
   for (i = 0; i < ARRAY_SIZE(tc->entries); i++) {
                        }
               /**
   * Flush the tile cache: write all dirty tiles back to the transfer.
   * any tiles "flagged" as cleared will be "really" cleared.
   */
   void
   sp_flush_tex_tile_cache(struct softpipe_tex_tile_cache *tc)
   {
               if (tc->texture) {
      /* caching a texture, mark all entries as empty */
   for (pos = 0; pos < ARRAY_SIZE(tc->entries); pos++) {
         }
            }
         /**
   * Given the texture face, level, zslice, x and y values, compute
   * the cache entry position/index where we'd hope to find the
   * cached texture tile.
   * This is basically a direct-map cache.
   * XXX There's probably lots of ways in which we can improve this.
   */
   static inline uint
   tex_cache_pos( union tex_tile_address addr )
   {
      uint entry = (addr.bits.x + 
                           }
      /**
   * Similar to sp_get_cached_tile() but for textures.
   * Tiles are read-only and indexed with more params.
   */
   const struct softpipe_tex_cached_tile *
   sp_find_cached_tile_tex(struct softpipe_tex_tile_cache *tc, 
         {
                                    /* cache miss.  Most misses are because we've invalidated the
   * texture cache previously -- most commonly on binding a new
   * texture.  Currently we effectively flush the cache on texture
   * bind.
   #if 0
         _debug_printf("miss at %u:  x=%d y=%d z=%d face=%d level=%d\n"
               #endif
            /* check if we need to get a new transfer */
   if (!tc->tex_trans ||
      tc->tex_level != addr.bits.level ||
   tc->tex_z != addr.bits.z) {
                  if (tc->tex_trans_map) {
      tc->pipe->texture_unmap(tc->pipe, tc->tex_trans);
   tc->tex_trans = NULL;
               width = u_minify(tc->texture->width0, addr.bits.level);
   if (tc->texture->target == PIPE_TEXTURE_1D_ARRAY) {
      height = tc->texture->array_size;
      }
   else {
      height = u_minify(tc->texture->height0, addr.bits.level);
               tc->tex_trans_map =
      pipe_texture_map(tc->pipe, tc->texture,
                           tc->tex_level = addr.bits.level;
               /* Get tile from the transfer (view into texture), explicitly passing
   * the image format.
   */
   pipe_get_tile_rgba(tc->tex_trans, tc->tex_trans_map,
                     addr.bits.x * TEX_TILE_SIZE,
   addr.bits.y * TEX_TILE_SIZE,
   TEX_TILE_SIZE,
               tc->last_tile = tile;
      }
