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
      #include "etnaviv_texture_desc.h"
      #include "hw/common.xml.h"
   #include "hw/texdesc_3d.xml.h"
      #include "etnaviv_clear_blit.h"
   #include "etnaviv_context.h"
   #include "etnaviv_emit.h"
   #include "etnaviv_format.h"
   #include "etnaviv_translate.h"
   #include "etnaviv_texture.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
      #include <drm_fourcc.h>
      struct etna_sampler_state_desc {
      struct pipe_sampler_state base;
   uint32_t SAMP_CTRL0;
   uint32_t SAMP_CTRL1;
   uint32_t SAMP_LOD_MINMAX;
   uint32_t SAMP_LOD_BIAS;
      };
      static inline struct etna_sampler_state_desc *
   etna_sampler_state_desc(struct pipe_sampler_state *samp)
   {
         }
      struct etna_sampler_view_desc {
      struct pipe_sampler_view base;
   /* format-dependent merged with sampler state */
   uint32_t SAMP_CTRL0;
   uint32_t SAMP_CTRL0_MASK;
            struct pipe_resource *res;
   struct etna_reloc DESC_ADDR;
      };
      static inline struct etna_sampler_view_desc *
   etna_sampler_view_desc(struct pipe_sampler_view *view)
   {
         }
      static void *
   etna_create_sampler_state_desc(struct pipe_context *pipe,
         {
      struct etna_sampler_state_desc *cs = CALLOC_STRUCT(etna_sampler_state_desc);
            if (!cs)
                     cs->SAMP_CTRL0 =
      VIVS_NTE_DESCRIPTOR_SAMP_CTRL0_UWRAP(translate_texture_wrapmode(ss->wrap_s)) |
   VIVS_NTE_DESCRIPTOR_SAMP_CTRL0_VWRAP(translate_texture_wrapmode(ss->wrap_t)) |
   VIVS_NTE_DESCRIPTOR_SAMP_CTRL0_WWRAP(translate_texture_wrapmode(ss->wrap_r)) |
   VIVS_NTE_DESCRIPTOR_SAMP_CTRL0_MIN(translate_texture_filter(ss->min_img_filter)) |
   VIVS_NTE_DESCRIPTOR_SAMP_CTRL0_MIP(translate_texture_mipfilter(ss->min_mip_filter)) |
   VIVS_NTE_DESCRIPTOR_SAMP_CTRL0_MAG(translate_texture_filter(ss->mag_img_filter)) |
   VIVS_NTE_DESCRIPTOR_SAMP_CTRL0_UNK21;
      cs->SAMP_CTRL1 = VIVS_NTE_DESCRIPTOR_SAMP_CTRL1_UNK1;
   uint32_t min_lod_fp8 = MIN2(etna_float_to_fixp88(ss->min_lod), 0xfff);
   uint32_t max_lod_fp8 = MIN2(etna_float_to_fixp88(ss->max_lod), 0xfff);
            cs->SAMP_LOD_MINMAX =
      VIVS_NTE_DESCRIPTOR_SAMP_LOD_MINMAX_MAX(MAX2(max_lod_fp8, max_lod_min)) |
         cs->SAMP_LOD_BIAS =
      VIVS_NTE_DESCRIPTOR_SAMP_LOD_BIAS_BIAS(etna_float_to_fixp88(ss->lod_bias)) |
                  }
      static void
   etna_delete_sampler_state_desc(struct pipe_context *pctx, void *ss)
   {
         }
      static struct pipe_sampler_view *
   etna_create_sampler_view_desc(struct pipe_context *pctx, struct pipe_resource *prsc,
         {
      const struct util_format_description *desc = util_format_description(so->format);
   struct etna_sampler_view_desc *sv = CALLOC_STRUCT(etna_sampler_view_desc);
   struct etna_context *ctx = etna_context(pctx);
   const uint32_t format = translate_texture_format(so->format);
   const bool ext = !!(format & EXT_FORMAT);
   const bool astc = !!(format & ASTC_FORMAT);
   const uint32_t swiz = get_texture_swiz(so->format, so->swizzle_r,
                        if (!sv)
            struct etna_resource *res = etna_texture_handle_incompatible(pctx, prsc);
   if (!res)
            sv->base = *so;
   pipe_reference_init(&sv->base.reference, 1);
   sv->base.texture = NULL;
   pipe_resource_reference(&sv->base.texture, prsc);
   sv->base.context = pctx;
            /* Determine whether target supported */
   uint32_t target_hw = translate_texture_target(sv->base.target);
   if (target_hw == ETNA_NO_MATCH) {
      BUG("Unhandled texture target");
               /* Texture descriptor sampler bits */
   if (util_format_is_srgb(so->format))
            /* Create texture descriptor */
   u_suballocator_alloc(&ctx->tex_desc_allocator, 256, 64,
         if (!sv->res)
                     /** GC7000 needs the size of the BASELOD level */
   uint32_t base_width = u_minify(res->base.width0, sv->base.u.tex.first_level);
   uint32_t base_height = u_minify(res->base.height0, sv->base.u.tex.first_level);
   uint32_t base_depth = u_minify(res->base.depth0, sv->base.u.tex.first_level);
   bool is_array = false;
            switch(sv->base.target) {
   case PIPE_TEXTURE_1D:
      target_hw = TEXTURE_TYPE_2D;
   sv->SAMP_CTRL0_MASK = ~VIVS_NTE_DESCRIPTOR_SAMP_CTRL0_VWRAP__MASK;
   sv->SAMP_CTRL0 = VIVS_NTE_DESCRIPTOR_SAMP_CTRL0_VWRAP(TEXTURE_WRAPMODE_REPEAT);
      case PIPE_TEXTURE_1D_ARRAY:
      is_array = true;
   base_height = res->base.array_size;
      case PIPE_TEXTURE_2D_ARRAY:
      is_array = true;
   base_depth = res->base.array_size;
      default:
               #define DESC_SET(x, y) buf[(TEXDESC_##x)>>2] = (y)
      DESC_SET(CONFIG0, COND(!ext && !astc, VIVS_TE_SAMPLER_CONFIG0_FORMAT(format))
                     DESC_SET(CONFIG1, COND(ext, VIVS_TE_SAMPLER_CONFIG1_FORMAT_EXT(format)) |
                     DESC_SET(CONFIG2, 0x00030000 |
         COND(sint && desc->channel[0].size == 8, TE_SAMPLER_CONFIG2_SIGNED_INT8) |
   DESC_SET(LINEAR_STRIDE, res->levels[0].stride);
   DESC_SET(VOLUME, etna_log2_fixp88(base_depth));
   DESC_SET(SLICE, res->levels[0].layer_stride);
   DESC_SET(3D_CONFIG, VIVS_TE_SAMPLER_3D_CONFIG_DEPTH(base_depth));
   DESC_SET(ASTC0, COND(astc, VIVS_NTE_SAMPLER_ASTC0_ASTC_FORMAT(format)) |
                     DESC_SET(BASELOD, TEXDESC_BASELOD_BASELOD(sv->base.u.tex.first_level) |
         DESC_SET(LOG_SIZE_EXT, TEXDESC_LOG_SIZE_EXT_WIDTH(etna_log2_fixp88(base_width)) |
         DESC_SET(SIZE, VIVS_TE_SAMPLER_SIZE_WIDTH(base_width) |
         for (int lod = 0; lod <= res->base.last_level; ++lod)
      #undef DESC_SET
         sv->DESC_ADDR.bo = etna_resource(sv->res)->bo;
   sv->DESC_ADDR.offset = suballoc_offset;
                  error:
      free(sv);
      }
      static void
   etna_sampler_view_update_descriptor(struct etna_context *ctx,
               {
               if (res->texture) {
                  /* No need to ref LOD levels individually as they'll always come from the same bo */
      }
      static void
   etna_sampler_view_desc_destroy(struct pipe_context *pctx,
         {
               pipe_resource_reference(&sv->base.texture, NULL);
   pipe_resource_reference(&sv->res, NULL);
      }
      static void
   etna_emit_texture_desc(struct etna_context *ctx)
   {
      struct etna_cmd_stream *stream = ctx->stream;
   uint32_t active_samplers = active_samplers_bits(ctx);
            if (unlikely(dirty & ETNA_DIRTY_SAMPLER_VIEWS)) {
      for (int x = 0; x < VIVS_TS_SAMPLER__LEN; ++x) {
      if ((1 << x) & active_samplers) {
      struct etna_sampler_view_desc *sv = etna_sampler_view_desc(ctx->sampler_view[x]);
                                 etna_set_state(stream, VIVS_TS_SAMPLER_CONFIG(x), sv->ts.TS_SAMPLER_CONFIG);
   etna_set_state_reloc(stream, VIVS_TS_SAMPLER_STATUS_BASE(x), &sv->ts.TS_SAMPLER_STATUS_BASE);
                  LOD_ADDR_0.bo = res->bo;
                                    if (unlikely(dirty & (ETNA_DIRTY_SAMPLERS | ETNA_DIRTY_SAMPLER_VIEWS))) {
      for (int x = 0; x < PIPE_MAX_SAMPLERS; ++x) {
      if ((1 << x) & active_samplers) {
      struct etna_sampler_state_desc *ss = etna_sampler_state_desc(ctx->sampler[x]);
                                 etna_set_state(stream, VIVS_NTE_DESCRIPTOR_TX_CTRL(x),
      COND(sv->ts.enable, VIVS_NTE_DESCRIPTOR_TX_CTRL_TS_ENABLE) |
   VIVS_NTE_DESCRIPTOR_TX_CTRL_TS_MODE(sv->ts.mode) |
   VIVS_NTE_DESCRIPTOR_TX_CTRL_TS_INDEX(x)|
   COND(sv->ts.comp, VIVS_NTE_DESCRIPTOR_TX_CTRL_COMPRESSION) |
      etna_set_state(stream, VIVS_NTE_DESCRIPTOR_SAMP_CTRL0(x), SAMP_CTRL0);
   etna_set_state(stream, VIVS_NTE_DESCRIPTOR_SAMP_CTRL1(x), ss->SAMP_CTRL1 | sv->SAMP_CTRL1);
   etna_set_state(stream, VIVS_NTE_DESCRIPTOR_SAMP_LOD_MINMAX(x), ss->SAMP_LOD_MINMAX);
   etna_set_state(stream, VIVS_NTE_DESCRIPTOR_SAMP_LOD_BIAS(x), ss->SAMP_LOD_BIAS);
                     if (unlikely(dirty & ETNA_DIRTY_SAMPLER_VIEWS)) {
      /* Set texture descriptors */
   for (int x = 0; x < PIPE_MAX_SAMPLERS; ++x) {
      if ((1 << x) & ctx->dirty_sampler_views) {
      if ((1 << x) & active_samplers) {
      struct etna_sampler_view_desc *sv = etna_sampler_view_desc(ctx->sampler_view[x]);
   etna_sampler_view_update_descriptor(ctx, stream, sv);
      } else if ((1 << x) & ctx->prev_active_samplers){
      /* dummy texture descriptors for unused samplers */
   etna_set_state_reloc(stream, VIVS_NTE_DESCRIPTOR_ADDR(x),
                        if (unlikely(dirty & ETNA_DIRTY_SAMPLER_VIEWS)) {
      /* Invalidate all dirty sampler views.
   */
   for (int x = 0; x < PIPE_MAX_SAMPLERS; ++x) {
      if ((1 << x) & ctx->dirty_sampler_views) {
      etna_set_state(stream, VIVS_NTE_DESCRIPTOR_INVALIDATE,
                              }
      static struct etna_sampler_ts*
   etna_ts_for_sampler_view_state(struct pipe_sampler_view *pview)
   {
      struct etna_sampler_view_desc *sv = etna_sampler_view_desc(pview);
      }
      void
   etna_texture_desc_init(struct pipe_context *pctx)
   {
      struct etna_context *ctx = etna_context(pctx);
   DBG("etnaviv: Using descriptor-based texturing\n");
   ctx->base.create_sampler_state = etna_create_sampler_state_desc;
   ctx->base.delete_sampler_state = etna_delete_sampler_state_desc;
   ctx->base.create_sampler_view = etna_create_sampler_view_desc;
   ctx->base.sampler_view_destroy = etna_sampler_view_desc_destroy;
   ctx->emit_texture_state = etna_emit_texture_desc;
      }
   