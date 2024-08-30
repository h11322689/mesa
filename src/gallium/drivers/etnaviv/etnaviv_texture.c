   /*
   * Copyright (c) 2012-2015 Etnaviv Project
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
      #include "etnaviv_texture.h"
      #include "hw/common.xml.h"
      #include "etnaviv_clear_blit.h"
   #include "etnaviv_context.h"
   #include "etnaviv_emit.h"
   #include "etnaviv_format.h"
   #include "etnaviv_texture_desc.h"
   #include "etnaviv_texture_state.h"
   #include "etnaviv_translate.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
      #include "drm-uapi/drm_fourcc.h"
      static void
   etna_bind_sampler_states(struct pipe_context *pctx, enum pipe_shader_type shader,
               {
      /* bind fragment sampler */
   struct etna_context *ctx = etna_context(pctx);
   struct etna_screen *screen = ctx->screen;
            switch (shader) {
   case PIPE_SHADER_FRAGMENT:
      offset = 0;
   ctx->num_fragment_samplers = num_samplers;
      case PIPE_SHADER_VERTEX:
      offset = screen->specs.vertex_sampler_offset;
      default:
      assert(!"Invalid shader");
               uint32_t mask = 1 << offset;
   for (int idx = 0; idx < num_samplers; ++idx, mask <<= 1) {
      ctx->sampler[offset + idx] = samplers[idx];
   if (samplers[idx])
         else
                  }
      static bool
   etna_configure_sampler_ts(struct etna_sampler_ts *sts, struct pipe_sampler_view *pview, bool enable)
   {
               assert(sts);
            if (!enable) {
      sts->TS_SAMPLER_CONFIG = 0;
   sts->TS_SAMPLER_STATUS_BASE.bo = NULL;
               struct etna_resource *rsc = etna_resource(pview->texture);
            if ((lev->clear_value & 0xffffffff) != sts->TS_SAMPLER_CLEAR_VALUE ||
      (lev->clear_value >> 32) != sts->TS_SAMPLER_CLEAR_VALUE2)
                  sts->mode = lev->ts_mode;
   sts->comp = lev->ts_compress_fmt >= 0;
   sts->TS_SAMPLER_CONFIG =
      VIVS_TS_SAMPLER_CONFIG_ENABLE |
   COND(lev->ts_compress_fmt >= 0, VIVS_TS_SAMPLER_CONFIG_COMPRESSION) |
      sts->TS_SAMPLER_CLEAR_VALUE = lev->clear_value;
   sts->TS_SAMPLER_CLEAR_VALUE2 = lev->clear_value >> 32;
   sts->TS_SAMPLER_STATUS_BASE.bo = rsc->ts_bo;
   sts->TS_SAMPLER_STATUS_BASE.offset = lev->ts_offset;
               }
      /* Return true if the GPU can use sampler TS with this sampler view.
   * Sampler TS is an optimization used when rendering to textures, where
   * a resolve-in-place can be avoided when rendering has left a (valid) TS.
   */
   static bool
   etna_can_use_sampler_ts(struct pipe_sampler_view *view, int num)
   {
      struct etna_resource *rsc = etna_resource(view->texture);
                     /* The resource TS is valid for level 0. */
   if (!etna_resource_level_ts_valid(&rsc->levels[0]))
            /* The hardware supports it. */
   if (!VIV_FEATURE(screen, chipMinorFeatures2, TEXTURE_TILED_READ))
            /* The sampler view will be bound to sampler < VIVS_TS_SAMPLER__LEN.
   * HALTI5 adds a mapping from sampler to sampler TS unit, but this is AFAIK
   * absent on earlier models. */
   if (num >= VIVS_TS_SAMPLER__LEN)
            /* It is a texture, not a buffer. */
   if (rsc->base.target == PIPE_BUFFER)
            /* Does not use compression or the hardware supports V4 compression. */
   if (rsc->levels[0].ts_compress_fmt >= 0 && !screen->specs.v4_compression)
            /* The sampler will have one LOD, and it happens to be level 0.
   * (It is not sure if the hw supports it for other levels, but available
   *  state strongly suggests only one at a time). */
   if (view->u.tex.first_level != 0 ||
      MIN2(view->u.tex.last_level, rsc->base.last_level) != 0)
            }
      void
   etna_update_sampler_source(struct pipe_sampler_view *view, int num)
   {
      struct etna_resource *base = etna_resource(view->texture);
   struct etna_resource *to = base, *from = base;
   struct etna_context *ctx = etna_context(view->context);
            if (base->render && etna_resource_newer(etna_resource(base->render), base))
            if (base->texture)
            if ((to != from) && etna_resource_older(to, from)) {
      etna_copy_resource(view->context, &to->base, &from->base,
                  } else if (to == from) {
      if (etna_can_use_sampler_ts(view, num)) {
         } else if (etna_resource_needs_flush(to)) {
      /* Resolve TS if needed */
   etna_copy_resource(view->context, &to->base, &from->base,
                              if (etna_configure_sampler_ts(ctx->ts_for_sampler_view(view), view, enable_sampler_ts)) {
      ctx->dirty |= ETNA_DIRTY_SAMPLER_VIEWS | ETNA_DIRTY_TEXTURE_CACHES;
         }
      static bool
   etna_resource_sampler_compatible(struct etna_resource *res)
   {
      if (util_format_is_compressed(res->base.format))
            struct etna_screen *screen = etna_screen(res->base.screen);
   /* This GPU supports texturing from supertiled textures? */
   if (res->layout == ETNA_LAYOUT_SUPER_TILED && VIV_FEATURE(screen, chipMinorFeatures2, SUPERTILED_TEXTURE))
            /* This GPU supports texturing from linear textures? */
   if (res->layout == ETNA_LAYOUT_LINEAR && VIV_FEATURE(screen, chipMinorFeatures1, LINEAR_TEXTURE_SUPPORT))
            /* Otherwise, only support tiled layouts */
   if (res->layout != ETNA_LAYOUT_TILED)
            /* If we have HALIGN support, we can allow for the RS padding */
   if (VIV_FEATURE(screen, chipMinorFeatures1, TEXTURE_HALIGN))
            /* Non-HALIGN GPUs only accept 4x4 tile-aligned textures */
   if (res->halign != TEXTURE_HALIGN_FOUR)
               }
      struct etna_resource *
   etna_texture_handle_incompatible(struct pipe_context *pctx, struct pipe_resource *prsc)
   {
               if (!etna_resource_sampler_compatible(res)) {
      /* The original resource is not compatible with the sampler.
   * Allocate an appropriately tiled texture. */
   if (!res->texture) {
               templat.bind &= ~(PIPE_BIND_DEPTH_STENCIL | PIPE_BIND_RENDER_TARGET |
         res->texture =
      etna_resource_alloc(pctx->screen, ETNA_LAYOUT_TILED,
            if (!res->texture) {
         }
      }
      }
      static void
   set_sampler_views(struct etna_context *ctx, unsigned start, unsigned end,
         {
      unsigned i, j;
   uint32_t mask = 1 << start;
            for (i = start, j = 0; j < nr; i++, j++, mask <<= 1) {
               if (take_ownership) {
      pipe_sampler_view_reference(&ctx->sampler_view[i], NULL);
      } else {
         }
   if (view) {
      ctx->active_sampler_views |= mask;
      } else
               for (; i < end; i++, mask <<= 1) {
      pipe_sampler_view_reference(&ctx->sampler_view[i], NULL);
               /* sampler views that changed state (even to inactive) are also dirty */
      }
      static inline void
   etna_fragtex_set_sampler_views(struct etna_context *ctx, unsigned nr,
               {
      struct etna_screen *screen = ctx->screen;
   unsigned start = 0;
            set_sampler_views(ctx, start, end, nr, take_ownership, views);
      }
         static inline void
   etna_vertex_set_sampler_views(struct etna_context *ctx, unsigned nr,
               {
      struct etna_screen *screen = ctx->screen;
   unsigned start = screen->specs.vertex_sampler_offset;
               }
      static void
   etna_set_sampler_views(struct pipe_context *pctx, enum pipe_shader_type shader,
                           {
      struct etna_context *ctx = etna_context(pctx);
                     switch (shader) {
   case PIPE_SHADER_FRAGMENT:
      etna_fragtex_set_sampler_views(ctx, num_views, take_ownership, views);
      case PIPE_SHADER_VERTEX:
      etna_vertex_set_sampler_views(ctx, num_views, take_ownership, views);
      default:;
      }
      static void
   etna_texture_barrier(struct pipe_context *pctx, unsigned flags)
   {
               etna_set_state(ctx->stream, VIVS_GL_FLUSH_CACHE,
               etna_set_state(ctx->stream, VIVS_GL_FLUSH_CACHE,
            }
      uint32_t
   active_samplers_bits(struct etna_context *ctx)
   {
         }
      void
   etna_texture_init(struct pipe_context *pctx)
   {
      struct etna_context *ctx = etna_context(pctx);
            pctx->bind_sampler_states = etna_bind_sampler_states;
   pctx->set_sampler_views = etna_set_sampler_views;
            if (screen->specs.halti >= 5) {
      u_suballocator_init(&ctx->tex_desc_allocator, pctx, 4096, 0,
            } else {
            }
      void
   etna_texture_fini(struct pipe_context *pctx)
   {
      struct etna_context *ctx = etna_context(pctx);
            if (screen->specs.halti >= 5)
      }
