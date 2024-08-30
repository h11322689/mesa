   /*
   * Mesa 3-D graphics library
   *
   * Copyright 2007-2008 VMware, Inc.
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
   */
      #include <stdbool.h>
      #include "util/macros.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
      #include "draw/draw_context.h"
   #include "draw/draw_private.h"
   #include "draw/draw_pt.h"
      #define SEGMENT_SIZE 1024
   #define MAP_SIZE     256
      struct vsplit_frontend {
      struct draw_pt_front_end base;
                              unsigned max_vertices;
            /* buffers for splitting */
   unsigned fetch_elts[SEGMENT_SIZE];
   uint16_t draw_elts[SEGMENT_SIZE];
            struct {
      /* map a fetch element to a draw element */
   unsigned fetches[MAP_SIZE];
   uint16_t draws[MAP_SIZE];
            uint16_t num_fetch_elts;
         };
         static void
   vsplit_clear_cache(struct vsplit_frontend *vsplit)
   {
      memset(vsplit->cache.fetches, 0xff, sizeof(vsplit->cache.fetches));
   vsplit->cache.has_max_fetch = false;
   vsplit->cache.num_fetch_elts = 0;
      }
         static void
   vsplit_flush_cache(struct vsplit_frontend *vsplit, unsigned flags)
   {
      vsplit->middle->run(vsplit->middle,
            }
         /**
   * Add a fetch element and add it to the draw elements.
   */
   static inline void
   vsplit_add_cache(struct vsplit_frontend *vsplit, unsigned fetch)
   {
                        /* If the value isn't in the cache or it's an overflow due to the
   * element bias */
   if (vsplit->cache.fetches[hash] != fetch) {
      /* update cache */
   vsplit->cache.fetches[hash] = fetch;
            /* add fetch */
   assert(vsplit->cache.num_fetch_elts < vsplit->segment_size);
                  }
         /**
   * Returns the base index to the elements array.
   * The value is checked for integer overflow (not sure it can happen?).
   */
   static inline unsigned
   vsplit_get_base_idx(unsigned start, unsigned fetch)
   {
         }
         static inline void
   vsplit_add_cache_uint8(struct vsplit_frontend *vsplit, const uint8_t *elts,
         {
      struct draw_context *draw = vsplit->draw;
   unsigned elt_idx;
   elt_idx = vsplit_get_base_idx(start, fetch);
   elt_idx = (unsigned)((int)(DRAW_GET_IDX(elts, elt_idx)) + elt_bias);
   /* unlike the uint32_t case this can only happen with elt_bias */
   if (elt_bias && elt_idx == DRAW_MAX_FETCH_IDX && !vsplit->cache.has_max_fetch) {
      unsigned hash = elt_idx % MAP_SIZE;
   vsplit->cache.fetches[hash] = 0;
      }
      }
         static inline void
   vsplit_add_cache_uint16(struct vsplit_frontend *vsplit, const uint16_t *elts,
         {
      struct draw_context *draw = vsplit->draw;
   unsigned elt_idx;
   elt_idx = vsplit_get_base_idx(start, fetch);
   elt_idx = (unsigned)((int)(DRAW_GET_IDX(elts, elt_idx)) + elt_bias);
   /* unlike the uint32_t case this can only happen with elt_bias */
   if (elt_bias && elt_idx == DRAW_MAX_FETCH_IDX && !vsplit->cache.has_max_fetch) {
      unsigned hash = elt_idx % MAP_SIZE;
   vsplit->cache.fetches[hash] = 0;
      }
      }
         /**
   * Add a fetch element and add it to the draw elements.  The fetch element is
   * in full range (uint32_t).
   */
   static inline void
   vsplit_add_cache_uint32(struct vsplit_frontend *vsplit, const uint32_t *elts,
         {
      struct draw_context *draw = vsplit->draw;
   unsigned elt_idx;
   /*
   * The final element index is just element index plus element bias.
   */
   elt_idx = vsplit_get_base_idx(start, fetch);
   elt_idx = (unsigned)((int)(DRAW_GET_IDX(elts, elt_idx)) + elt_bias);
   /* Take care for DRAW_MAX_FETCH_IDX (since cache is initialized to -1). */
   if (elt_idx == DRAW_MAX_FETCH_IDX && !vsplit->cache.has_max_fetch) {
      unsigned hash = elt_idx % MAP_SIZE;
   /* force update - any value will do except DRAW_MAX_FETCH_IDX */
   vsplit->cache.fetches[hash] = 0;
      }
      }
         #define FUNC vsplit_run_linear
   #include "draw_pt_vsplit_tmp.h"
      #define FUNC vsplit_run_uint8
   #define ELT_TYPE uint8_t
   #define ADD_CACHE(vsplit, ib, start, fetch, bias) vsplit_add_cache_uint8(vsplit,ib,start,fetch,bias)
   #include "draw_pt_vsplit_tmp.h"
      #define FUNC vsplit_run_uint16
   #define ELT_TYPE uint16_t
   #define ADD_CACHE(vsplit, ib, start, fetch, bias) vsplit_add_cache_uint16(vsplit,ib,start,fetch, bias)
   #include "draw_pt_vsplit_tmp.h"
      #define FUNC vsplit_run_uint32
   #define ELT_TYPE uint32_t
   #define ADD_CACHE(vsplit, ib, start, fetch, bias) vsplit_add_cache_uint32(vsplit, ib, start, fetch, bias)
   #include "draw_pt_vsplit_tmp.h"
         static void
   vsplit_prepare(struct draw_pt_front_end *frontend,
                     {
               switch (vsplit->draw->pt.user.eltSize) {
   case 0:
      vsplit->base.run = vsplit_run_linear;
      case 1:
      vsplit->base.run = vsplit_run_uint8;
      case 2:
      vsplit->base.run = vsplit_run_uint16;
      case 4:
      vsplit->base.run = vsplit_run_uint32;
      default:
      assert(0);
               /* split only */
            vsplit->middle = middle;
               }
         static void
   vsplit_flush(struct draw_pt_front_end *frontend, unsigned flags)
   {
               if (flags & DRAW_FLUSH_STATE_CHANGE) {
      vsplit->middle->finish(vsplit->middle);
         }
         static void
   vsplit_destroy(struct draw_pt_front_end *frontend)
   {
         }
         struct draw_pt_front_end *
   draw_pt_vsplit(struct draw_context *draw)
   {
               if (!vsplit)
            vsplit->base.prepare = vsplit_prepare;
   vsplit->base.run     = NULL;
   vsplit->base.flush   = vsplit_flush;
   vsplit->base.destroy = vsplit_destroy;
            for (unsigned i = 0; i < SEGMENT_SIZE; i++)
               }
