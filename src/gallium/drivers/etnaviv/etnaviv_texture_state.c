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
      #include "etnaviv_texture_state.h"
      #include "hw/common.xml.h"
      #include "etnaviv_clear_blit.h"
   #include "etnaviv_context.h"
   #include "etnaviv_emit.h"
   #include "etnaviv_format.h"
   #include "etnaviv_texture.h"
   #include "etnaviv_translate.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
      #include "drm-uapi/drm_fourcc.h"
      struct etna_sampler_state {
               /* sampler offset +4*sampler, interleave when committing state */
   uint32_t config0;
   uint32_t config1;
   uint32_t config_lod;
   uint32_t config_3d;
   uint32_t baselod;
      };
      static inline struct etna_sampler_state *
   etna_sampler_state(struct pipe_sampler_state *samp)
   {
         }
      struct etna_sampler_view {
               /* sampler offset +4*sampler, interleave when committing state */
   uint32_t config0;
   uint32_t config0_mask;
   uint32_t config1;
   uint32_t config_3d;
   uint32_t size;
   uint32_t log_size;
   uint32_t astc0;
   uint32_t linear_stride;  /* only LOD0 */
   struct etna_reloc lod_addr[VIVS_TE_SAMPLER_LOD_ADDR__LEN];
               };
      static inline struct etna_sampler_view *
   etna_sampler_view(struct pipe_sampler_view *view)
   {
         }
      static void *
   etna_create_sampler_state_state(struct pipe_context *pipe,
         {
      struct etna_sampler_state *cs = CALLOC_STRUCT(etna_sampler_state);
   struct etna_context *ctx = etna_context(pipe);
   struct etna_screen *screen = ctx->screen;
   const bool ansio = ss->max_anisotropy > 1;
            if (!cs)
                     cs->config0 =
      VIVS_TE_SAMPLER_CONFIG0_UWRAP(translate_texture_wrapmode(ss->wrap_s)) |
   VIVS_TE_SAMPLER_CONFIG0_VWRAP(translate_texture_wrapmode(ss->wrap_t)) |
   VIVS_TE_SAMPLER_CONFIG0_MIN(translate_texture_filter(ss->min_img_filter)) |
   VIVS_TE_SAMPLER_CONFIG0_MIP(translate_texture_mipfilter(ss->min_mip_filter)) |
   VIVS_TE_SAMPLER_CONFIG0_MAG(translate_texture_filter(ss->mag_img_filter)) |
         /* ROUND_UV improves precision - but not compatible with NEAREST filter */
   if (ss->min_img_filter != PIPE_TEX_FILTER_NEAREST &&
      ss->mag_img_filter != PIPE_TEX_FILTER_NEAREST) {
               cs->config1 = screen->specs.seamless_cube_map ?
            cs->config_lod =
      COND(ss->lod_bias != 0.0 && mipmap, VIVS_TE_SAMPLER_LOD_CONFIG_BIAS_ENABLE) |
         cs->config_3d =
            if (mipmap) {
      cs->min_lod = etna_float_to_fixp55(ss->min_lod);
      } else {
      /* when not mipmapping, we need to set max/min lod so that always
   * lowest LOD is selected */
               /* if max_lod is 0, MIN filter will never be used (GC3000)
   * when min filter is different from mag filter, we need HW to compute LOD
   * the workaround is to set max_lod to at least 1
   */
            cs->baselod =
      COND(ss->compare_mode, VIVS_NTE_SAMPLER_BASELOD_COMPARE_ENABLE) |
         /* force nearest filting for nir_lower_sample_tex_compare(..) */
   if ((ctx->screen->specs.halti < 2) && ss->compare_mode) {
      cs->config0 &= ~VIVS_TE_SAMPLER_CONFIG0_MIN__MASK;
            cs->config0 |=
      VIVS_TE_SAMPLER_CONFIG0_MIN(TEXTURE_FILTER_NEAREST) |
               }
      static void
   etna_delete_sampler_state_state(struct pipe_context *pctx, void *ss)
   {
         }
      static struct pipe_sampler_view *
   etna_create_sampler_view_state(struct pipe_context *pctx, struct pipe_resource *prsc,
         {
      struct etna_sampler_view *sv = CALLOC_STRUCT(etna_sampler_view);
   struct etna_context *ctx = etna_context(pctx);
   struct etna_screen *screen = ctx->screen;
   const uint32_t format = translate_texture_format(so->format);
   const bool ext = !!(format & EXT_FORMAT);
   const bool astc = !!(format & ASTC_FORMAT);
   const bool srgb = util_format_is_srgb(so->format);
   const uint32_t swiz = get_texture_swiz(so->format, so->swizzle_r,
                  if (!sv)
            struct etna_resource *res = etna_texture_handle_incompatible(pctx, prsc);
   if (!res) {
      free(sv);
               sv->base = *so;
   pipe_reference_init(&sv->base.reference, 1);
   sv->base.texture = NULL;
   pipe_resource_reference(&sv->base.texture, prsc);
            /* merged with sampler state */
   sv->config0 =
      VIVS_TE_SAMPLER_CONFIG0_TYPE(translate_texture_target(sv->base.target)) |
               uint32_t base_height = res->base.height0;
   uint32_t base_depth = res->base.depth0;
            switch (sv->base.target) {
   case PIPE_TEXTURE_1D:
      /* use 2D texture with T wrap to repeat for 1D texture
   * TODO: check if old HW supports 1D texture
   */
   sv->config0_mask = ~VIVS_TE_SAMPLER_CONFIG0_VWRAP__MASK;
   sv->config0 &= ~VIVS_TE_SAMPLER_CONFIG0_TYPE__MASK;
   sv->config0 |=
      VIVS_TE_SAMPLER_CONFIG0_TYPE(TEXTURE_TYPE_2D) |
         case PIPE_TEXTURE_1D_ARRAY:
      is_array = true;
   base_height = res->base.array_size;
      case PIPE_TEXTURE_2D_ARRAY:
      is_array = true;
   base_depth = res->base.array_size;
      default:
                  if (res->layout == ETNA_LAYOUT_LINEAR && !util_format_is_compressed(so->format)) {
               assert(res->base.last_level == 0);
      } else {
      sv->config0 |= VIVS_TE_SAMPLER_CONFIG0_ADDRESSING_MODE(TEXTURE_ADDRESSING_MODE_TILED);
               sv->config1 |= COND(ext, VIVS_TE_SAMPLER_CONFIG1_FORMAT_EXT(format)) |
                     sv->astc0 = COND(astc, VIVS_NTE_SAMPLER_ASTC0_ASTC_FORMAT(format)) |
               COND(astc && srgb, VIVS_NTE_SAMPLER_ASTC0_ASTC_SRGB) |
   VIVS_NTE_SAMPLER_ASTC0_UNK8(0xc) |
   sv->size = VIVS_TE_SAMPLER_SIZE_WIDTH(res->base.width0) |
         sv->log_size =
      VIVS_TE_SAMPLER_LOG_SIZE_WIDTH(etna_log2_fixp55(res->base.width0)) |
   VIVS_TE_SAMPLER_LOG_SIZE_HEIGHT(etna_log2_fixp55(base_height)) |
   COND(util_format_is_srgb(so->format) && !astc, VIVS_TE_SAMPLER_LOG_SIZE_SRGB) |
      sv->config_3d =
      VIVS_TE_SAMPLER_3D_CONFIG_DEPTH(base_depth) |
         /* Set up levels-of-detail */
   for (int lod = 0; lod <= res->base.last_level; ++lod) {
      sv->lod_addr[lod].bo = res->bo;
   sv->lod_addr[lod].offset = res->levels[lod].offset;
      }
   sv->min_lod = sv->base.u.tex.first_level << 5;
            /* Workaround for npot textures -- it appears that only CLAMP_TO_EDGE is
   * supported when the appropriate capability is not set. */
   if (!screen->specs.npot_tex_any_wrap &&
      (!util_is_power_of_two_or_zero(res->base.width0) ||
   !util_is_power_of_two_or_zero(res->base.height0))) {
   sv->config0_mask = ~(VIVS_TE_SAMPLER_CONFIG0_UWRAP__MASK |
         sv->config0 |=
      VIVS_TE_SAMPLER_CONFIG0_UWRAP(TEXTURE_WRAPMODE_CLAMP_TO_EDGE) |
               }
      static void
   etna_sampler_view_state_destroy(struct pipe_context *pctx,
         {
      pipe_resource_reference(&view->texture, NULL);
      }
      #define EMIT_STATE(state_name, src_value) \
            #define EMIT_STATE_FIXP(state_name, src_value) \
            #define EMIT_STATE_RELOC(state_name, src_value) \
            static void
   etna_emit_ts_state(struct etna_context *ctx)
   {
      struct etna_cmd_stream *stream = ctx->stream;
   uint32_t active_samplers = active_samplers_bits(ctx);
   uint32_t dirty = ctx->dirty;
                     if (unlikely(dirty & ETNA_DIRTY_SAMPLER_VIEWS)) {
      for (int x = 0; x < VIVS_TS_SAMPLER__LEN; ++x) {
      if ((1 << x) & active_samplers) {
      struct etna_sampler_view *sv = etna_sampler_view(ctx->sampler_view[x]);
         }
   for (int x = 0; x < VIVS_TS_SAMPLER__LEN; ++x) {
      if ((1 << x) & active_samplers) {
      struct etna_sampler_view *sv = etna_sampler_view(ctx->sampler_view[x]);
         }
   for (int x = 0; x < VIVS_TS_SAMPLER__LEN; ++x) {
      if ((1 << x) & active_samplers) {
      struct etna_sampler_view *sv = etna_sampler_view(ctx->sampler_view[x]);
         }
   for (int x = 0; x < VIVS_TS_SAMPLER__LEN; ++x) {
      if ((1 << x) & active_samplers) {
      struct etna_sampler_view *sv = etna_sampler_view(ctx->sampler_view[x]);
                        }
      static void
   etna_emit_new_texture_state(struct etna_context *ctx)
   {
      struct etna_cmd_stream *stream = ctx->stream;
   struct etna_screen *screen = ctx->screen;
   uint32_t active_samplers = active_samplers_bits(ctx);
   uint32_t dirty = ctx->dirty;
                              if (unlikely(dirty & (ETNA_DIRTY_SAMPLER_VIEWS | ETNA_DIRTY_SAMPLERS))) {
      for (int x = 0; x < VIVS_NTE_SAMPLER__LEN; ++x) {
                        /* set active samplers to their configuration value (determined by
   * both the sampler state and sampler view) */
   if ((1 << x) & active_samplers) {
                                          }
   if (unlikely(dirty & (ETNA_DIRTY_SAMPLER_VIEWS))) {
      struct etna_sampler_state *ss;
            for (int x = 0; x < VIVS_NTE_SAMPLER__LEN; ++x) {
      if ((1 << x) & active_samplers) {
      sv = etna_sampler_view(ctx->sampler_view[x]);
         }
   for (int x = 0; x < VIVS_NTE_SAMPLER__LEN; ++x) {
      if ((1 << x) & active_samplers) {
      ss = etna_sampler_state(ctx->sampler[x]);
                                          }
   if (unlikely(dirty & (ETNA_DIRTY_SAMPLER_VIEWS | ETNA_DIRTY_SAMPLERS))) {
      struct etna_sampler_state *ss;
            for (int x = 0; x < VIVS_NTE_SAMPLER__LEN; ++x) {
      if ((1 << x) & active_samplers) {
                                    /* min and max lod is determined both by the sampler and the view */
   /*10180*/ EMIT_STATE(NTE_SAMPLER_LOD_CONFIG(x),
                        if (unlikely(dirty & (ETNA_DIRTY_SAMPLER_VIEWS))) {
      /* only LOD0 is valid for this register */
   for (int x = 0; x < VIVS_NTE_SAMPLER__LEN; ++x) {
      if ((1 << x) & active_samplers) {
      struct etna_sampler_view *sv = etna_sampler_view(ctx->sampler_view[x]);
            }
      for (int x = 0; x < VIVS_NTE_SAMPLER__LEN; ++x) {
      if ((1 << x) & active_samplers) {
                     /*10300*/ EMIT_STATE(NTE_SAMPLER_3D_CONFIG(x), ss->config_3d |
         }
   for (int x = 0; x < VIVS_NTE_SAMPLER__LEN; ++x) {
      if ((1 << x) & active_samplers) {
                     /*10380*/ EMIT_STATE(NTE_SAMPLER_CONFIG1(x), ss->config1 |
                  }
   if (unlikely(screen->specs.tex_astc && (dirty & (ETNA_DIRTY_SAMPLER_VIEWS)))) {
      for (int x = 0; x < VIVS_NTE_SAMPLER__LEN; ++x) {
      if ((1 << x) & active_samplers) {
      struct etna_sampler_view *sv = etna_sampler_view(ctx->sampler_view[x]);
            }
   if (unlikely(dirty & (ETNA_DIRTY_SAMPLERS))) {
      for (int x = 0; x < VIVS_NTE_SAMPLER__LEN; ++x) {
      if ((1 << x) & active_samplers) {
      struct etna_sampler_state *ss = etna_sampler_state(ctx->sampler[x]);
                     if (unlikely(dirty & (ETNA_DIRTY_SAMPLER_VIEWS))) {
      for (int x = 0; x < VIVS_NTE_SAMPLER__LEN; ++x) {
      if ((1 << x) & active_samplers) {
      for (int y = 0; y < VIVS_NTE_SAMPLER_ADDR_LOD__LEN; ++y) {
      struct etna_sampler_view *sv = etna_sampler_view(ctx->sampler_view[x]);
                                    }
      /* Emit plain (non-descriptor) texture state */
   static void
   etna_emit_texture_state(struct etna_context *ctx)
   {
      struct etna_cmd_stream *stream = ctx->stream;
   struct etna_screen *screen = ctx->screen;
   uint32_t active_samplers = active_samplers_bits(ctx);
   uint32_t dirty = ctx->dirty;
                              if (unlikely(dirty & (ETNA_DIRTY_SAMPLER_VIEWS | ETNA_DIRTY_SAMPLERS))) {
      for (int x = 0; x < VIVS_TE_SAMPLER__LEN; ++x) {
                        /* set active samplers to their configuration value (determined by
   * both the sampler state and sampler view) */
   if ((1 << x) & active_samplers) {
                                          }
   if (unlikely(dirty & (ETNA_DIRTY_SAMPLER_VIEWS))) {
      struct etna_sampler_state *ss;
            for (int x = 0; x < VIVS_TE_SAMPLER__LEN; ++x) {
      if ((1 << x) & active_samplers) {
      sv = etna_sampler_view(ctx->sampler_view[x]);
         }
   for (int x = 0; x < VIVS_TE_SAMPLER__LEN; ++x) {
      if ((1 << x) & active_samplers) {
      ss = etna_sampler_state(ctx->sampler[x]);
                                          }
   if (unlikely(dirty & (ETNA_DIRTY_SAMPLER_VIEWS | ETNA_DIRTY_SAMPLERS))) {
      struct etna_sampler_state *ss;
            for (int x = 0; x < VIVS_TE_SAMPLER__LEN; ++x) {
      if ((1 << x) & active_samplers) {
                                    /* min and max lod is determined both by the sampler and the view */
   /*020C0*/ EMIT_STATE(TE_SAMPLER_LOD_CONFIG(x),
                     }
   for (int x = 0; x < VIVS_TE_SAMPLER__LEN; ++x) {
      if ((1 << x) & active_samplers) {
                     /*02180*/ EMIT_STATE(TE_SAMPLER_3D_CONFIG(x), ss->config_3d |
         }
   for (int x = 0; x < VIVS_TE_SAMPLER__LEN; ++x) {
      if ((1 << x) & active_samplers) {
                     /*021C0*/ EMIT_STATE(TE_SAMPLER_CONFIG1(x), ss->config1 |
                  }
   if (unlikely(dirty & (ETNA_DIRTY_SAMPLER_VIEWS))) {
      for (int y = 0; y < VIVS_TE_SAMPLER_LOD_ADDR__LEN; ++y) {
      for (int x = 0; x < VIVS_TE_SAMPLER__LEN; ++x) {
      if ((1 << x) & active_samplers) {
      struct etna_sampler_view *sv = etna_sampler_view(ctx->sampler_view[x]);
               }
   if (unlikely(dirty & (ETNA_DIRTY_SAMPLER_VIEWS))) {
      /* only LOD0 is valid for this register */
   for (int x = 0; x < VIVS_TE_SAMPLER__LEN; ++x) {
      if ((1 << x) & active_samplers) {
      struct etna_sampler_view *sv = etna_sampler_view(ctx->sampler_view[x]);
            }
   if (unlikely(screen->specs.tex_astc && (dirty & (ETNA_DIRTY_SAMPLER_VIEWS)))) {
      for (int x = 0; x < VIVS_TE_SAMPLER__LEN; ++x) {
      if ((1 << x) & active_samplers) {
      struct etna_sampler_view *sv = etna_sampler_view(ctx->sampler_view[x]);
                                 }
      #undef EMIT_STATE
   #undef EMIT_STATE_FIXP
   #undef EMIT_STATE_RELOC
      static struct etna_sampler_ts*
   etna_ts_for_sampler_view_state(struct pipe_sampler_view *pview)
   {
      struct etna_sampler_view *sv = etna_sampler_view(pview);
      }
      void
   etna_texture_state_init(struct pipe_context *pctx)
   {
      struct etna_context *ctx = etna_context(pctx);
   DBG("etnaviv: Using state-based texturing");
   ctx->base.create_sampler_state = etna_create_sampler_state_state;
   ctx->base.delete_sampler_state = etna_delete_sampler_state_state;
   ctx->base.create_sampler_view = etna_create_sampler_view_state;
   ctx->base.sampler_view_destroy = etna_sampler_view_state_destroy;
                     if (ctx->screen->specs.halti >= 1)
         else
      }
