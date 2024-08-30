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
   * Render target tile caching.
   *
   * Author:
   *    Brian Paul
   */
      #include "util/u_inlines.h"
   #include "util/format/u_format.h"
   #include "util/u_memory.h"
   #include "util/u_tile.h"
   #include "sp_tile_cache.h"
      static struct softpipe_cached_tile *
   sp_alloc_tile(struct softpipe_tile_cache *tc);
         /**
   * Return the position in the cache for the tile that contains win pos (x,y).
   * We currently use a direct mapped cache so this is like a hack key.
   * At some point we should investigate something more sophisticated, like
   * a LRU replacement policy.
   */
   #define CACHE_POS(x, y, l)                        \
               static inline int addr_to_clear_pos(union tile_address addr)
   {
      int pos;
   pos = addr.bits.layer * (MAX_WIDTH / TILE_SIZE) * (MAX_HEIGHT / TILE_SIZE);
   pos += addr.bits.y * (MAX_WIDTH / TILE_SIZE);
   pos += addr.bits.x;
      }
   /**
   * Is the tile at (x,y) in cleared state?
   */
   static inline uint
   is_clear_flag_set(const uint *bitvec, union tile_address addr, unsigned max)
   {
      int pos, bit;
   pos = addr_to_clear_pos(addr);
   assert(pos / 32 < max);
   bit = bitvec[pos / 32] & (1 << (pos & 31));
      }
            /**
   * Mark the tile at (x,y) as not cleared.
   */
   static inline void
   clear_clear_flag(uint *bitvec, union tile_address addr, unsigned max)
   {
      int pos;
   pos = addr_to_clear_pos(addr);
   assert(pos / 32 < max);
      }
            struct softpipe_tile_cache *
   sp_create_tile_cache( struct pipe_context *pipe )
   {
      struct softpipe_tile_cache *tc;
            /* sanity checking: max sure MAX_WIDTH/HEIGHT >= largest texture image */
   assert(MAX_WIDTH >= pipe->screen->get_param(pipe->screen,
                              tc = CALLOC_STRUCT( softpipe_tile_cache );
   if (tc) {
      tc->pipe = pipe;
   for (pos = 0; pos < ARRAY_SIZE(tc->tile_addrs); pos++) {
         }
            /* this allocation allows us to guarantee that allocation
   * failures are never fatal later
   */
   tc->tile = MALLOC_STRUCT( softpipe_cached_tile );
   if (!tc->tile)
   {
      FREE(tc);
               /* XXX this code prevents valgrind warnings about use of uninitialized
   * memory in programs that don't clear the surface before rendering.
   * However, it breaks clearing in other situations (such as in
   * progs/tests/drawbuffers, see bug 24402).
   #if 0
         /* set flags to indicate all the tiles are cleared */
   #endif
      }
      }
         void
   sp_destroy_tile_cache(struct softpipe_tile_cache *tc)
   {
      if (tc) {
               for (pos = 0; pos < ARRAY_SIZE(tc->entries); pos++) {
      /*assert(tc->entries[pos].x < 0);*/
      }
            if (tc->num_maps) {
      int i;
   for (i = 0; i < tc->num_maps; i++)
      if (tc->transfer[i]) {
            FREE(tc->transfer);
   FREE(tc->transfer_map);
                     }
         /**
   * Specify the surface to cache.
   */
   void
   sp_tile_cache_set_surface(struct softpipe_tile_cache *tc,
         {
      struct pipe_context *pipe = tc->pipe;
            if (tc->num_maps) {
      if (ps == tc->surface)
            for (i = 0; i < tc->num_maps; i++) {
      pipe->texture_unmap(pipe, tc->transfer[i]);
   tc->transfer[i] = NULL;
      }
   FREE(tc->transfer);
   FREE(tc->transfer_map);
            FREE(tc->clear_flags);
                        if (ps) {
      tc->num_maps = ps->u.tex.last_layer - ps->u.tex.first_layer + 1;
   tc->transfer = CALLOC(tc->num_maps, sizeof(struct pipe_transfer *));
            tc->clear_flags_size = (MAX_WIDTH / TILE_SIZE) * (MAX_HEIGHT / TILE_SIZE) * tc->num_maps / 32 * sizeof(uint);
            if (ps->texture->target != PIPE_BUFFER) {
      for (i = 0; i < tc->num_maps; i++) {
      tc->transfer_map[i] = pipe_texture_map(pipe, ps->texture,
                                 }
   else {
      /* can't render to buffers */
                     }
         /**
   * Return the transfer being cached.
   */
   struct pipe_surface *
   sp_tile_cache_get_surface(struct softpipe_tile_cache *tc)
   {
         }
         /**
   * Set pixels in a tile to the given clear color/value, float.
   */
   static void
   clear_tile_rgba(struct softpipe_cached_tile *tile,
               {
      if (clear_value->f[0] == 0.0 &&
      clear_value->f[1] == 0.0 &&
   clear_value->f[2] == 0.0 &&
   clear_value->f[3] == 0.0) {
      }
   else {
               if (util_format_is_pure_uint(format)) {
      for (i = 0; i < TILE_SIZE; i++) {
      for (j = 0; j < TILE_SIZE; j++) {
      tile->data.colorui128[i][j][0] = clear_value->ui[0];
   tile->data.colorui128[i][j][1] = clear_value->ui[1];
   tile->data.colorui128[i][j][2] = clear_value->ui[2];
            } else if (util_format_is_pure_sint(format)) {
      for (i = 0; i < TILE_SIZE; i++) {
      for (j = 0; j < TILE_SIZE; j++) {
      tile->data.colori128[i][j][0] = clear_value->i[0];
   tile->data.colori128[i][j][1] = clear_value->i[1];
   tile->data.colori128[i][j][2] = clear_value->i[2];
            } else {
      for (i = 0; i < TILE_SIZE; i++) {
      for (j = 0; j < TILE_SIZE; j++) {
      tile->data.color[i][j][0] = clear_value->f[0];
   tile->data.color[i][j][1] = clear_value->f[1];
   tile->data.color[i][j][2] = clear_value->f[2];
                  }
         /**
   * Set a tile to a solid value/color.
   */
   static void
   clear_tile(struct softpipe_cached_tile *tile,
               {
               switch (util_format_get_blocksize(format)) {
   case 1:
      memset(tile->data.any, (int) clear_value, TILE_SIZE * TILE_SIZE);
      case 2:
      if (clear_value == 0) {
         }
   else {
      for (i = 0; i < TILE_SIZE; i++) {
      for (j = 0; j < TILE_SIZE; j++) {
               }
      case 4:
      if (clear_value == 0) {
         }
   else {
      for (i = 0; i < TILE_SIZE; i++) {
      for (j = 0; j < TILE_SIZE; j++) {
               }
      case 8:
      if (clear_value == 0) {
         }
   else {
      for (i = 0; i < TILE_SIZE; i++) {
      for (j = 0; j < TILE_SIZE; j++) {
               }
      default:
            }
         /**
   * Actually clear the tiles which were flagged as being in a clear state.
   */
   static void
   sp_tile_cache_flush_clear(struct softpipe_tile_cache *tc, int layer)
   {
      struct pipe_transfer *pt = tc->transfer[layer];
   const uint w = tc->transfer[layer]->box.width;
   const uint h = tc->transfer[layer]->box.height;
   uint x, y;
                     /* clear the scratch tile to the clear value */
   if (tc->depth_stencil) {
         } else {
                  /* push the tile to all positions marked as clear */
   for (y = 0; y < h; y += TILE_SIZE) {
      for (x = 0; x < w; x += TILE_SIZE) {
               if (is_clear_flag_set(tc->clear_flags, addr, tc->clear_flags_size)) {
      /* write the scratch tile to the surface */
   if (tc->depth_stencil) {
      pipe_put_tile_raw(pt, tc->transfer_map[layer],
            }
   else {
      pipe_put_tile_rgba(pt, tc->transfer_map[layer],
                  }
                     #if 0
         #endif
   }
      static void
   sp_flush_tile(struct softpipe_tile_cache* tc, unsigned pos)
   {
      int layer = tc->tile_addrs[pos].bits.layer;
   if (!tc->tile_addrs[pos].bits.invalid) {
      if (tc->depth_stencil) {
      pipe_put_tile_raw(tc->transfer[layer], tc->transfer_map[layer],
                        }
   else {
      pipe_put_tile_rgba(tc->transfer[layer], tc->transfer_map[layer],
                     tc->tile_addrs[pos].bits.x * TILE_SIZE,
      }
         }
      /**
   * Flush the tile cache: write all dirty tiles back to the transfer.
   * any tiles "flagged" as cleared will be "really" cleared.
   */
   void
   sp_flush_tile_cache(struct softpipe_tile_cache *tc)
   {
      UNUSED int inuse = 0;
   int i;
   if (tc->num_maps) {
      /* caching a drawing transfer */
   for (int pos = 0; pos < ARRAY_SIZE(tc->entries); pos++) {
      struct softpipe_cached_tile *tile = tc->entries[pos];
   if (!tile)
   {
      assert(tc->tile_addrs[pos].bits.invalid);
      }
   sp_flush_tile(tc, pos);
               if (!tc->tile)
            for (i = 0; i < tc->num_maps; i++)
         /* reset all clear flags to zero */
                     #if 0
         #endif
   }
      static struct softpipe_cached_tile *
   sp_alloc_tile(struct softpipe_tile_cache *tc)
   {
      struct softpipe_cached_tile * tile = MALLOC_STRUCT(softpipe_cached_tile);
   if (!tile)
   {
      /* in this case, steal an existing tile */
   if (!tc->tile)
   {
      unsigned pos;
   for (pos = 0; pos < ARRAY_SIZE(tc->entries); ++pos) {
                     sp_flush_tile(tc, pos);
   tc->tile = tc->entries[pos];
   tc->entries[pos] = NULL;
               /* this should never happen */
   if (!tc->tile)
               tile = tc->tile;
               }
      }
      /**
   * Get a tile from the cache.
   * \param x, y  position of tile, in pixels
   */
   struct softpipe_cached_tile *
   sp_find_cached_tile(struct softpipe_tile_cache *tc, 
         {
      struct pipe_transfer *pt;
   /* cache pos/entry: */
   const int pos = CACHE_POS(addr.bits.x,
         struct softpipe_cached_tile *tile = tc->entries[pos];
   int layer;
   if (!tile) {
      tile = sp_alloc_tile(tc);
                           layer = tc->tile_addrs[pos].bits.layer;
   if (tc->tile_addrs[pos].bits.invalid == 0) {
      /* put dirty tile back in framebuffer */
   if (tc->depth_stencil) {
      pipe_put_tile_raw(tc->transfer[layer], tc->transfer_map[layer],
                        }
   else {
      pipe_put_tile_rgba(tc->transfer[layer], tc->transfer_map[layer],
                     tc->tile_addrs[pos].bits.x * TILE_SIZE,
                           layer = tc->tile_addrs[pos].bits.layer;
   pt = tc->transfer[layer];
            if (is_clear_flag_set(tc->clear_flags, addr, tc->clear_flags_size)) {
      /* don't get tile from framebuffer, just clear it */
   if (tc->depth_stencil) {
         }
   else {
         }
      }
   else {
      /* get new tile data from transfer */
   if (tc->depth_stencil) {
      pipe_get_tile_raw(tc->transfer[layer], tc->transfer_map[layer],
                        }
   else {
      pipe_get_tile_rgba(tc->transfer[layer], tc->transfer_map[layer],
                     tc->tile_addrs[pos].bits.x * TILE_SIZE,
                     tc->last_tile = tile;
   tc->last_tile_addr = addr;
      }
                  /**
   * When a whole surface is being cleared to a value we can avoid
   * fetching tiles above.
   * Save the color and set a 'clearflag' for each tile of the screen.
   */
   void
   sp_tile_cache_clear(struct softpipe_tile_cache *tc,
               {
                                 /* set flags to indicate all the tiles are cleared */
            for (pos = 0; pos < ARRAY_SIZE(tc->tile_addrs); pos++) {
         }
      }
