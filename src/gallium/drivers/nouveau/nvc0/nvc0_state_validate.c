   #include "util/format/u_format.h"
   #include "util/u_framebuffer.h"
   #include "util/u_math.h"
   #include "util/u_viewport.h"
      #include "nvc0/nvc0_context.h"
      static inline void
   nvc0_fb_set_null_rt(struct nouveau_pushbuf *push, unsigned i, unsigned layers)
   {
      BEGIN_NVC0(push, NVC0_3D(RT_ADDRESS_HIGH(i)), 9);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 64);     // width
   PUSH_DATA (push, 0);      // height
   PUSH_DATA (push, 0);      // format
   PUSH_DATA (push, 0);      // tile mode
   PUSH_DATA (push, layers); // layers
   PUSH_DATA (push, 0);      // layer stride
      }
      static uint32_t
   gm200_encode_cb_sample_location(uint8_t x, uint8_t y)
   {
      static const uint8_t lut[] = {
      0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf,
      uint32_t result = 0;
   /* S0.12 representation for TGSI_OPCODE_INTERP_SAMPLE */
   result |= lut[x] << 8 | lut[y] << 24;
   /* fill in gaps with data in a representation for SV_SAMPLE_POS */
   result |= x << 12 | y << 28;
      }
      static void
   gm200_validate_sample_locations(struct nvc0_context *nvc0, unsigned ms)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nvc0_screen *screen = nvc0->screen;
   unsigned grid_width, grid_height, hw_grid_width;
   uint8_t sample_locations[16][2];
   unsigned cb[64];
   unsigned i, pixel, pixel_y, pixel_x, sample;
            screen->base.base.get_sample_pixel_grid(
            hw_grid_width = grid_width;
   if (ms == 1) /* get_sample_pixel_grid() exposes 2x4 for 1x msaa */
            if (nvc0->sample_locations_enabled) {
      uint8_t locations[2 * 4 * 8];
   memcpy(locations, nvc0->sample_locations, sizeof(locations));
   util_sample_locations_flip_y(
            for (pixel = 0; pixel < hw_grid_width*grid_height; pixel++) {
      for (sample = 0; sample < ms; sample++) {
      unsigned pixel_x = pixel % hw_grid_width;
   unsigned pixel_y = pixel / hw_grid_width;
   unsigned wi = pixel * ms + sample;
   unsigned ri = (pixel_y * grid_width + pixel_x % grid_width);
   ri = ri * ms + sample;
   sample_locations[wi][0] = locations[ri] & 0xf;
            } else {
      const uint8_t (*ptr)[2] = nvc0_get_sample_locations(ms);
   for (i = 0; i < 16; i++) {
      sample_locations[i][0] = ptr[i % ms][0];
                  BEGIN_NVC0(push, NVC0_3D(CB_SIZE), 3);
   PUSH_DATA (push, NVC0_CB_AUX_SIZE);
   PUSH_DATAh(push, screen->uniform_bo->offset + NVC0_CB_AUX_INFO(4));
   PUSH_DATA (push, screen->uniform_bo->offset + NVC0_CB_AUX_INFO(4));
   BEGIN_1IC0(push, NVC0_3D(CB_POS), 1 + 64);
   PUSH_DATA (push, NVC0_CB_AUX_SAMPLE_INFO);
   for (pixel_y = 0; pixel_y < 4; pixel_y++) {
      for (pixel_x = 0; pixel_x < 2; pixel_x++) {
      for (sample = 0; sample < ms; sample++) {
      unsigned write_index = (pixel_y * 2 + pixel_x) * 8 + sample;
   unsigned read_index = pixel_y % grid_height * hw_grid_width;
   uint8_t x, y;
   read_index += pixel_x % grid_width;
   read_index = read_index * ms + sample;
   x = sample_locations[read_index][0];
   y = sample_locations[read_index][1];
            }
            for (i = 0; i < 16; i++) {
      packed_locations[i / 4] |= sample_locations[i][0] << ((i % 4) * 8);
               BEGIN_NVC0(push, SUBC_3D(0x11e0), 4);
      }
      static void
   nvc0_validate_sample_locations(struct nvc0_context *nvc0, unsigned ms)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nvc0_screen *screen = nvc0->screen;
            BEGIN_NVC0(push, NVC0_3D(CB_SIZE), 3);
   PUSH_DATA (push, NVC0_CB_AUX_SIZE);
   PUSH_DATAh(push, screen->uniform_bo->offset + NVC0_CB_AUX_INFO(4));
   PUSH_DATA (push, screen->uniform_bo->offset + NVC0_CB_AUX_INFO(4));
   BEGIN_1IC0(push, NVC0_3D(CB_POS), 1 + 2 * ms);
   PUSH_DATA (push, NVC0_CB_AUX_SAMPLE_INFO);
   for (i = 0; i < ms; i++) {
      float xy[2];
   nvc0->base.pipe.get_sample_position(&nvc0->base.pipe, ms, i, xy);
   PUSH_DATAf(push, xy[0]);
         }
      static void
   validate_sample_locations(struct nvc0_context *nvc0)
   {
               if (nvc0->screen->base.class_3d >= GM200_3D_CLASS)
         else
      }
      static void
   nvc0_validate_fb(struct nvc0_context *nvc0)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct pipe_framebuffer_state *fb = &nvc0->framebuffer;
   unsigned i;
   unsigned ms_mode = NVC0_3D_MULTISAMPLE_MODE_MS1;
   unsigned nr_cbufs = fb->nr_cbufs;
                     BEGIN_NVC0(push, NVC0_3D(SCREEN_SCISSOR_HORIZ), 2);
   PUSH_DATA (push, fb->width << 16);
            for (i = 0; i < fb->nr_cbufs; ++i) {
      struct nv50_surface *sf;
   struct nv04_resource *res;
            if (!fb->cbufs[i]) {
      nvc0_fb_set_null_rt(push, i, 0);
               sf = nv50_surface(fb->cbufs[i]);
   res = nv04_resource(sf->base.texture);
            BEGIN_NVC0(push, NVC0_3D(RT_ADDRESS_HIGH(i)), 9);
   PUSH_DATAh(push, res->address + sf->offset);
   PUSH_DATA (push, res->address + sf->offset);
   if (likely(nouveau_bo_memtype(bo))) {
                        PUSH_DATA(push, sf->width);
   PUSH_DATA(push, sf->height);
   PUSH_DATA(push, nvc0_format_table[sf->base.format].rt);
   PUSH_DATA(push, (mt->layout_3d << 16) |
         PUSH_DATA(push, sf->base.u.tex.first_layer + sf->depth);
                     } else {
      if (res->base.target == PIPE_BUFFER) {
      PUSH_DATA(push, 262144);
      } else {
      PUSH_DATA(push, nv50_miptree(sf->base.texture)->level[0].pitch);
      }
   PUSH_DATA(push, nvc0_format_table[sf->base.format].rt);
   PUSH_DATA(push, 1 << 12);
   PUSH_DATA(push, 1);
                                       if (res->status & NOUVEAU_BUFFER_STATUS_GPU_READING)
         res->status |=  NOUVEAU_BUFFER_STATUS_GPU_WRITING;
            /* only register for writing, otherwise we'd always serialize here */
               if (fb->zsbuf) {
      struct nv50_miptree *mt = nv50_miptree(fb->zsbuf->texture);
   struct nv50_surface *sf = nv50_surface(fb->zsbuf);
            BEGIN_NVC0(push, NVC0_3D(ZETA_ADDRESS_HIGH), 5);
   PUSH_DATAh(push, mt->base.address + sf->offset);
   PUSH_DATA (push, mt->base.address + sf->offset);
   PUSH_DATA (push, nvc0_format_table[fb->zsbuf->format].rt);
   PUSH_DATA (push, mt->level[sf->base.u.tex.level].tile_mode);
   PUSH_DATA (push, mt->layer_stride >> 2);
   BEGIN_NVC0(push, NVC0_3D(ZETA_ENABLE), 1);
   PUSH_DATA (push, 1);
   BEGIN_NVC0(push, NVC0_3D(ZETA_HORIZ), 3);
   PUSH_DATA (push, sf->width);
   PUSH_DATA (push, sf->height);
   PUSH_DATA (push, (unk << 16) |
         BEGIN_NVC0(push, NVC0_3D(ZETA_BASE_LAYER), 1);
                     if (mt->base.status & NOUVEAU_BUFFER_STATUS_GPU_READING)
         mt->base.status |=  NOUVEAU_BUFFER_STATUS_GPU_WRITING;
               } else {
      BEGIN_NVC0(push, NVC0_3D(ZETA_ENABLE), 1);
               if (nr_cbufs == 0 && !fb->zsbuf) {
      assert(util_is_power_of_two_or_zero(fb->samples));
                     if (fb->samples > 1)
                     BEGIN_NVC0(push, NVC0_3D(RT_CONTROL), 1);
   PUSH_DATA (push, (076543210 << 4) | nr_cbufs);
            if (serialize)
               }
      static void
   nvc0_validate_blend_colour(struct nvc0_context *nvc0)
   {
               BEGIN_NVC0(push, NVC0_3D(BLEND_COLOR(0)), 4);
   PUSH_DATAf(push, nvc0->blend_colour.color[0]);
   PUSH_DATAf(push, nvc0->blend_colour.color[1]);
   PUSH_DATAf(push, nvc0->blend_colour.color[2]);
      }
      static void
   nvc0_validate_stencil_ref(struct nvc0_context *nvc0)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
            IMMED_NVC0(push, NVC0_3D(STENCIL_FRONT_FUNC_REF), ref[0]);
      }
      static void
   nvc0_validate_stipple(struct nvc0_context *nvc0)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
            BEGIN_NVC0(push, NVC0_3D(POLYGON_STIPPLE_PATTERN(0)), 32);
   for (i = 0; i < 32; ++i)
      }
      static void
   nvc0_validate_scissor(struct nvc0_context *nvc0)
   {
      int i;
            if (!(nvc0->dirty_3d & NVC0_NEW_3D_SCISSOR) &&
      nvc0->rast->pipe.scissor == nvc0->state.scissor)
         if (nvc0->state.scissor != nvc0->rast->pipe.scissor)
                     for (i = 0; i < NVC0_MAX_VIEWPORTS; i++) {
      struct pipe_scissor_state *s = &nvc0->scissors[i];
   if (!(nvc0->scissors_dirty & (1 << i)))
            BEGIN_NVC0(push, NVC0_3D(SCISSOR_HORIZ(i)), 2);
   if (nvc0->rast->pipe.scissor) {
      PUSH_DATA(push, (s->maxx << 16) | s->minx);
      } else {
      PUSH_DATA(push, (0xffff << 16) | 0);
         }
      }
      static void
   nvc0_validate_viewport(struct nvc0_context *nvc0)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   uint16_t class_3d = nvc0->screen->base.class_3d;
   int x, y, w, h, i;
            for (i = 0; i < NVC0_MAX_VIEWPORTS; i++) {
               if (!(nvc0->viewports_dirty & (1 << i)))
            BEGIN_NVC0(push, NVC0_3D(VIEWPORT_TRANSLATE_X(i)), 3);
   PUSH_DATAf(push, vp->translate[0]);
   PUSH_DATAf(push, vp->translate[1]);
            BEGIN_NVC0(push, NVC0_3D(VIEWPORT_SCALE_X(i)), 3);
   PUSH_DATAf(push, vp->scale[0]);
   PUSH_DATAf(push, vp->scale[1]);
                     x = util_iround(MAX2(0.0f, vp->translate[0] - fabsf(vp->scale[0])));
   y = util_iround(MAX2(0.0f, vp->translate[1] - fabsf(vp->scale[1])));
   w = util_iround(vp->translate[0] + fabsf(vp->scale[0])) - x;
            BEGIN_NVC0(push, NVC0_3D(VIEWPORT_HORIZ(i)), 2);
   PUSH_DATA (push, (w << 16) | x);
            /* If the halfz setting ever changes, the viewports will also get
   * updated. The rast will get updated before the validate function has a
   * chance to hit, so we can just use it directly without an atom
   * dependency.
   */
            BEGIN_NVC0(push, NVC0_3D(DEPTH_RANGE_NEAR(i)), 2);
   PUSH_DATAf(push, zmin);
            if (class_3d >= GM200_3D_CLASS) {
      BEGIN_NVC0(push, NVC0_3D(VIEWPORT_SWIZZLE(i)), 1);
   PUSH_DATA (push, vp->swizzle_x << 0 |
                     }
      }
      static void
   nvc0_validate_window_rects(struct nvc0_context *nvc0)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   bool enable = nvc0->window_rect.rects > 0 || nvc0->window_rect.inclusive;
            IMMED_NVC0(push, NVC0_3D(CLIP_RECTS_EN), enable);
   if (!enable)
            IMMED_NVC0(push, NVC0_3D(CLIP_RECTS_MODE), !nvc0->window_rect.inclusive);
   BEGIN_NVC0(push, NVC0_3D(CLIP_RECT_HORIZ(0)), NVC0_MAX_WINDOW_RECTANGLES * 2);
   for (i = 0; i < nvc0->window_rect.rects; i++) {
      struct pipe_scissor_state *s = &nvc0->window_rect.rect[i];
   PUSH_DATA(push, (s->maxx << 16) | s->minx);
      }
   for (; i < NVC0_MAX_WINDOW_RECTANGLES; i++) {
      PUSH_DATA(push, 0);
         }
      static inline void
   nvc0_upload_uclip_planes(struct nvc0_context *nvc0, unsigned s)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
            BEGIN_NVC0(push, NVC0_3D(CB_SIZE), 3);
   PUSH_DATA (push, NVC0_CB_AUX_SIZE);
   PUSH_DATAh(push, screen->uniform_bo->offset + NVC0_CB_AUX_INFO(s));
   PUSH_DATA (push, screen->uniform_bo->offset + NVC0_CB_AUX_INFO(s));
   BEGIN_1IC0(push, NVC0_3D(CB_POS), PIPE_MAX_CLIP_PLANES * 4 + 1);
   PUSH_DATA (push, NVC0_CB_AUX_UCP_INFO);
      }
      static inline void
   nvc0_check_program_ucps(struct nvc0_context *nvc0,
         {
               if (vp->vp.num_ucps >= n)
                  vp->vp.num_ucps = n;
   if (likely(vp == nvc0->vertprog))
         else
   if (likely(vp == nvc0->gmtyprog))
         else
      }
      static void
   nvc0_validate_clip(struct nvc0_context *nvc0)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nvc0_program *vp;
   unsigned stage;
            if (nvc0->gmtyprog) {
      stage = 3;
      } else
   if (nvc0->tevlprog) {
      stage = 2;
      } else {
      stage = 0;
               if (clip_enable && vp->vp.num_ucps < PIPE_MAX_CLIP_PLANES)
            if (nvc0->dirty_3d & (NVC0_NEW_3D_CLIP | (NVC0_NEW_3D_VERTPROG << stage)))
      if (vp->vp.num_ucps > 0 && vp->vp.num_ucps <= PIPE_MAX_CLIP_PLANES)
         clip_enable &= vp->vp.clip_enable;
            if (nvc0->state.clip_enable != clip_enable) {
      nvc0->state.clip_enable = clip_enable;
      }
   if (nvc0->state.clip_mode != vp->vp.clip_mode) {
      nvc0->state.clip_mode = vp->vp.clip_mode;
   BEGIN_NVC0(push, NVC0_3D(CLIP_DISTANCE_MODE), 1);
         }
      static void
   nvc0_validate_blend(struct nvc0_context *nvc0)
   {
               PUSH_SPACE(push, nvc0->blend->size);
      }
      static void
   nvc0_validate_zsa(struct nvc0_context *nvc0)
   {
               PUSH_SPACE(push, nvc0->zsa->size);
      }
      static void
   nvc0_validate_rasterizer(struct nvc0_context *nvc0)
   {
               PUSH_SPACE(push, nvc0->rast->size);
      }
      static void
   nvc0_constbufs_validate(struct nvc0_context *nvc0)
   {
               bool can_serialize = true;
            for (s = 0; s < 5; ++s) {
      while (nvc0->constbuf_dirty[s]) {
                     if (nvc0->constbuf[s][i].user) {
      struct nouveau_bo *bo = nvc0->screen->uniform_bo;
   const unsigned base = NVC0_CB_USR_INFO(s);
   const unsigned size = nvc0->constbuf[s][0].size;
                                    nvc0_screen_bind_cb_3d(nvc0->screen, push, &can_serialize, s, i,
      }
   nvc0_cb_bo_push(&nvc0->base, bo, NV_VRAM_DOMAIN(&nvc0->screen->base),
                  } else {
      struct nv04_resource *res =
         if (res) {
                                                   if (i == 0)
      } else if (i != 0) {
                           if (nvc0->screen->base.class_3d < NVE4_3D_CLASS) {
      /* Invalidate all COMPUTE constbufs because they are aliased with 3D. */
   nvc0->dirty_cp |= NVC0_NEW_CP_CONSTBUF;
   nvc0->constbuf_dirty[5] |= nvc0->constbuf_valid[5];
         }
      static void
   nvc0_validate_buffers(struct nvc0_context *nvc0)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nvc0_screen *screen = nvc0->screen;
            for (s = 0; s < 5; s++) {
      BEGIN_NVC0(push, NVC0_3D(CB_SIZE), 3);
   PUSH_DATA (push, NVC0_CB_AUX_SIZE);
   PUSH_DATAh(push, screen->uniform_bo->offset + NVC0_CB_AUX_INFO(s));
   PUSH_DATA (push, screen->uniform_bo->offset + NVC0_CB_AUX_INFO(s));
   BEGIN_1IC0(push, NVC0_3D(CB_POS), 1 + 4 * NVC0_MAX_BUFFERS);
   PUSH_DATA (push, NVC0_CB_AUX_BUF_INFO(0));
   for (i = 0; i < NVC0_MAX_BUFFERS; i++) {
      if (nvc0->buffers[s][i].buffer) {
      struct nv04_resource *res =
         PUSH_DATA (push, res->address + nvc0->buffers[s][i].buffer_offset);
   PUSH_DATAh(push, res->address + nvc0->buffers[s][i].buffer_offset);
   PUSH_DATA (push, nvc0->buffers[s][i].buffer_size);
   PUSH_DATA (push, 0);
   BCTX_REFN(nvc0->bufctx_3d, 3D_BUF, res, RDWR);
   util_range_add(&res->base, &res->valid_buffer_range,
                  } else {
      PUSH_DATA (push, 0);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 0);
                  }
      static void
   nvc0_validate_sample_mask(struct nvc0_context *nvc0)
   {
               unsigned mask[4] =
   {
      nvc0->sample_mask & 0xffff,
   nvc0->sample_mask & 0xffff,
   nvc0->sample_mask & 0xffff,
               BEGIN_NVC0(push, NVC0_3D(MSAA_MASK(0)), 4);
   PUSH_DATA (push, mask[0]);
   PUSH_DATA (push, mask[1]);
   PUSH_DATA (push, mask[2]);
      }
      static void
   nvc0_validate_min_samples(struct nvc0_context *nvc0)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
            samples = util_next_power_of_two(nvc0->min_samples);
   if (samples > 1) {
      // If we're using the incoming sample mask and doing sample shading, we
   // have to do sample shading "to the max", otherwise there's no way to
   // tell which sets of samples are covered by the current invocation.
   // Similarly for reading the framebuffer.
   if (nvc0->fragprog && (
            nvc0->fragprog->fp.sample_mask_in ||
                     }
      static void
   nvc0_validate_driverconst(struct nvc0_context *nvc0)
   {
      struct nvc0_screen *screen = nvc0->screen;
            for (i = 0; i < 5; ++i)
      nvc0_screen_bind_cb_3d(screen, nvc0->base.pushbuf, NULL, i, 15, NVC0_CB_AUX_SIZE,
            }
      static void
   nvc0_validate_fp_zsa_rast(struct nvc0_context *nvc0)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
            if (nvc0->rast && nvc0->rast->pipe.rasterizer_discard) {
         } else {
      bool zs = nvc0->zsa &&
         rasterizer_discard = !zs &&
               if (rasterizer_discard != nvc0->state.rasterizer_discard) {
      nvc0->state.rasterizer_discard = rasterizer_discard;
         }
      /* alpha test is disabled if there are no color RTs, so make sure we have at
   * least one if alpha test is enabled. Note that this must run after
   * nvc0_validate_fb, otherwise that will override the RT count setting.
   */
   static void
   nvc0_validate_zsa_fb(struct nvc0_context *nvc0)
   {
               if (nvc0->zsa && nvc0->zsa->pipe.alpha_enabled &&
      nvc0->framebuffer.zsbuf &&
   nvc0->framebuffer.nr_cbufs == 0) {
   nvc0_fb_set_null_rt(push, 0, 0);
   BEGIN_NVC0(push, NVC0_3D(RT_CONTROL), 1);
         }
      static void
   nvc0_validate_rast_fb(struct nvc0_context *nvc0)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct pipe_framebuffer_state *fb = &nvc0->framebuffer;
            if (!rast)
            if (rast->offset_units_unscaled) {
      BEGIN_NVC0(push, NVC0_3D(POLYGON_OFFSET_UNITS), 1);
   if (fb->zsbuf && fb->zsbuf->format == PIPE_FORMAT_Z16_UNORM)
         else
         }
         static void
   nvc0_validate_tess_state(struct nvc0_context *nvc0)
   {
               BEGIN_NVC0(push, NVC0_3D(TESS_LEVEL_OUTER(0)), 6);
   PUSH_DATAp(push, nvc0->default_tess_outer, 4);
      }
      /* If we have a frag shader bound which tries to read from the framebuffer, we
   * have to make sure that the fb is bound as a texture in the expected
   * location. For Fermi, that's in the special driver slot 16, while for Kepler
   * it's a regular binding stored in the driver constbuf.
   */
   static void
   nvc0_validate_fbread(struct nvc0_context *nvc0)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nvc0_screen *screen = nvc0->screen;
   struct pipe_context *pipe = &nvc0->base.pipe;
   struct pipe_sampler_view *old_view = nvc0->fbtexture;
            if (nvc0->fragprog &&
      nvc0->fragprog->fp.reads_framebuffer &&
   nvc0->framebuffer.nr_cbufs &&
   nvc0->framebuffer.cbufs[0]) {
   struct pipe_sampler_view tmpl = {0};
            tmpl.target = PIPE_TEXTURE_2D_ARRAY;
   tmpl.format = sf->format;
   tmpl.u.tex.first_level = tmpl.u.tex.last_level = sf->u.tex.level;
   tmpl.u.tex.first_layer = sf->u.tex.first_layer;
   tmpl.u.tex.last_layer = sf->u.tex.last_layer;
   tmpl.swizzle_r = PIPE_SWIZZLE_X;
   tmpl.swizzle_g = PIPE_SWIZZLE_Y;
   tmpl.swizzle_b = PIPE_SWIZZLE_Z;
            /* Bail if it's the same parameters */
   if (old_view && old_view->texture == sf->texture &&
      old_view->format == sf->format &&
   old_view->u.tex.first_level == sf->u.tex.level &&
   old_view->u.tex.first_layer == sf->u.tex.first_layer &&
                  } else if (old_view == NULL) {
                  if (old_view)
                  if (new_view) {
      struct nv50_tic_entry *tic = nv50_tic_entry(new_view);
   assert(tic->id < 0);
   tic->id = nvc0_screen_tic_alloc(screen, tic);
   nvc0->base.push_data(&nvc0->base, screen->txc, tic->id * 32,
                  if (screen->base.class_3d >= NVE4_3D_CLASS) {
      BEGIN_NVC0(push, NVC0_3D(CB_SIZE), 3);
   PUSH_DATA (push, NVC0_CB_AUX_SIZE);
   PUSH_DATAh(push, screen->uniform_bo->offset + NVC0_CB_AUX_INFO(4));
   PUSH_DATA (push, screen->uniform_bo->offset + NVC0_CB_AUX_INFO(4));
   BEGIN_1IC0(push, NVC0_3D(CB_POS), 1 + 1);
   PUSH_DATA (push, NVC0_CB_AUX_FB_TEX_INFO);
      } else {
      BEGIN_NVC0(push, NVC0_3D(BIND_TIC2(0)), 1);
                     }
      static void
   nvc0_switch_pipe_context(struct nvc0_context *ctx_to)
   {
      struct nvc0_context *ctx_from = ctx_to->screen->cur_ctx;
            simple_mtx_assert_locked(&ctx_to->screen->state_lock);
   if (ctx_from)
         else
            ctx_to->dirty_3d = ~0;
   ctx_to->dirty_cp = ~0;
   ctx_to->viewports_dirty = ~0;
            for (s = 0; s < 6; ++s) {
      ctx_to->samplers_dirty[s] = ~0;
   ctx_to->textures_dirty[s] = ~0;
   ctx_to->constbuf_dirty[s] = (1 << NVC0_MAX_PIPE_CONSTBUFS) - 1;
   ctx_to->buffers_dirty[s]  = ~0;
               /* Reset tfb as the shader that owns it may have been deleted. */
            if (!ctx_to->vertex)
            if (!ctx_to->vertprog)
         if (!ctx_to->fragprog)
            if (!ctx_to->blend)
         if (!ctx_to->rast)
         if (!ctx_to->zsa)
               }
      static struct nvc0_state_validate
   validate_list_3d[] = {
      { nvc0_validate_fb,            NVC0_NEW_3D_FRAMEBUFFER },
   { nvc0_validate_blend,         NVC0_NEW_3D_BLEND },
   { nvc0_validate_zsa,           NVC0_NEW_3D_ZSA },
   { nvc0_validate_sample_mask,   NVC0_NEW_3D_SAMPLE_MASK },
   { nvc0_validate_rasterizer,    NVC0_NEW_3D_RASTERIZER },
   { nvc0_validate_blend_colour,  NVC0_NEW_3D_BLEND_COLOUR },
   { nvc0_validate_stencil_ref,   NVC0_NEW_3D_STENCIL_REF },
   { nvc0_validate_stipple,       NVC0_NEW_3D_STIPPLE },
   { nvc0_validate_scissor,       NVC0_NEW_3D_SCISSOR | NVC0_NEW_3D_RASTERIZER },
   { nvc0_validate_viewport,      NVC0_NEW_3D_VIEWPORT },
   { nvc0_validate_window_rects,  NVC0_NEW_3D_WINDOW_RECTS },
   { nvc0_vertprog_validate,      NVC0_NEW_3D_VERTPROG },
   { nvc0_tctlprog_validate,      NVC0_NEW_3D_TCTLPROG },
   { nvc0_tevlprog_validate,      NVC0_NEW_3D_TEVLPROG },
   { nvc0_validate_tess_state,    NVC0_NEW_3D_TESSFACTOR },
   { nvc0_gmtyprog_validate,      NVC0_NEW_3D_GMTYPROG },
   { nvc0_validate_min_samples,   NVC0_NEW_3D_MIN_SAMPLES |
               { nvc0_fragprog_validate,      NVC0_NEW_3D_FRAGPROG | NVC0_NEW_3D_RASTERIZER },
   { nvc0_validate_fp_zsa_rast,   NVC0_NEW_3D_FRAGPROG | NVC0_NEW_3D_ZSA |
         { nvc0_validate_zsa_fb,        NVC0_NEW_3D_ZSA | NVC0_NEW_3D_FRAMEBUFFER },
   { nvc0_validate_rast_fb,       NVC0_NEW_3D_RASTERIZER | NVC0_NEW_3D_FRAMEBUFFER },
   { nvc0_validate_clip,          NVC0_NEW_3D_CLIP | NVC0_NEW_3D_RASTERIZER |
                     { nvc0_constbufs_validate,     NVC0_NEW_3D_CONSTBUF },
   { nvc0_validate_textures,      NVC0_NEW_3D_TEXTURES },
   { nvc0_validate_samplers,      NVC0_NEW_3D_SAMPLERS },
   { nve4_set_tex_handles,        NVC0_NEW_3D_TEXTURES | NVC0_NEW_3D_SAMPLERS },
   { nvc0_validate_fbread,        NVC0_NEW_3D_FRAGPROG |
         { nvc0_vertex_arrays_validate, NVC0_NEW_3D_VERTEX | NVC0_NEW_3D_ARRAYS },
   { nvc0_validate_surfaces,      NVC0_NEW_3D_SURFACES },
   { nvc0_validate_buffers,       NVC0_NEW_3D_BUFFERS },
   { nvc0_tfb_validate,           NVC0_NEW_3D_TFB_TARGETS | NVC0_NEW_3D_GMTYPROG },
   { nvc0_layer_validate,         NVC0_NEW_3D_VERTPROG |
               { nvc0_validate_driverconst,   NVC0_NEW_3D_DRIVERCONST },
   { validate_sample_locations,   NVC0_NEW_3D_SAMPLE_LOCATIONS |
      };
      bool
   nvc0_state_validate(struct nvc0_context *nvc0, uint32_t mask,
               {
      uint32_t state_mask;
   int ret;
                     if (nvc0->screen->cur_ctx != nvc0)
                     if (state_mask) {
      for (i = 0; i < size; ++i) {
               if (state_mask & validate->states)
      }
                        nouveau_pushbuf_bufctx(nvc0->base.pushbuf, bufctx);
               }
      bool
   nvc0_state_validate_3d(struct nvc0_context *nvc0, uint32_t mask)
   {
               ret = nvc0_state_validate(nvc0, mask, validate_list_3d,
                  if (unlikely(nvc0->state.flushed)) {
      nvc0->state.flushed = false;
      }
      }
