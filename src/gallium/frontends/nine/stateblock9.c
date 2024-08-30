   /*
   * Copyright 2011 Joakim Sindholt <opensource@zhasha.com>
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
   * USE OR OTHER DEALINGS IN THE SOFTWARE. */
      #include "stateblock9.h"
   #include "device9.h"
   #include "basetexture9.h"
   #include "nine_helpers.h"
   #include "vertexdeclaration9.h"
   #include "vertexbuffer9.h"
   #include "indexbuffer9.h"
      #define DBG_CHANNEL DBG_STATEBLOCK
      /* XXX TODO: handling of lights is broken */
      HRESULT
   NineStateBlock9_ctor( struct NineStateBlock9 *This,
               {
                        if (FAILED(hr))
                     This->state.vs_const_f = MALLOC(VS_CONST_F_SIZE(This->base.device));
   This->state.ps_const_f = MALLOC(This->base.device->ps_const_size);
   This->state.vs_const_i = MALLOC(VS_CONST_I_SIZE(This->base.device));
   This->state.vs_const_b = MALLOC(VS_CONST_B_SIZE(This->base.device));
   if (!This->state.vs_const_f || !This->state.ps_const_f ||
      !This->state.vs_const_i || !This->state.vs_const_b)
            }
      void
   NineStateBlock9_dtor( struct NineStateBlock9 *This )
   {
      struct nine_state *state = &This->state;
   struct nine_range *r;
   struct nine_range_pool *pool = &This->base.device->range_pool;
            for (i = 0; i < ARRAY_SIZE(state->rt); ++i)
         nine_bind(&state->ds, NULL);
   nine_bind(&state->vs, NULL);
   nine_bind(&state->ps, NULL);
   nine_bind(&state->vdecl, NULL);
   for (i = 0; i < PIPE_MAX_ATTRIBS; ++i)
            nine_bind(&state->idxbuf, NULL);
   for (i = 0; i < NINE_MAX_SAMPLERS; ++i)
            FREE(state->vs_const_f);
   FREE(state->ps_const_f);
   FREE(state->vs_const_i);
                              if (This->state.changed.ps_const_f) {
      for (r = This->state.changed.ps_const_f; r->next; r = r->next);
      }
   if (This->state.changed.vs_const_f) {
      for (r = This->state.changed.vs_const_f; r->next; r = r->next);
      }
   if (This->state.changed.vs_const_i) {
      for (r = This->state.changed.vs_const_i; r->next; r = r->next);
      }
   if (This->state.changed.vs_const_b) {
      for (r = This->state.changed.vs_const_b; r->next; r = r->next);
                  }
      static void
   NineStateBlock9_BindBuffer( struct NineDevice9 *device,
                     {
      if (applyToDevice)
         else
      }
      static void
   NineStateBlock9_BindTexture( struct NineDevice9 *device,
                     {
      if (applyToDevice)
         else
      }
      /* Copy state marked changed in @mask from @src to @dst.
   * If @apply is false, updating dst->changed can be omitted.
   * TODO: compare ?
   */
   static void
   nine_state_copy_common(struct NineDevice9 *device,
                        struct nine_state *dst,
      {
                        /* device changed.* are unused.
   * Instead nine_context_apply_stateblock is used and will
   * internally set the right context->changed fields.
   * Uncomment these only if we want to apply a stateblock onto a stateblock.
   *
   * if (apply)
   *     dst->changed.group |= mask->changed.group;
            if (mask->changed.group & NINE_STATE_VIEWPORT)
         if (mask->changed.group & NINE_STATE_SCISSOR)
            if (mask->changed.group & NINE_STATE_VS)
         if (mask->changed.group & NINE_STATE_PS)
            /* Vertex constants.
   *
   * Various possibilities for optimization here, like creating a per-SB
   * constant buffer, or memcmp'ing for changes.
   * Will do that later depending on what works best for specific apps.
   *
   * Note: Currently when we apply stateblocks, it's always on the device state.
   * Should it affect recording stateblocks ? Since it's on device state, there
   * is no need to copy which ranges are dirty. If it turns out we should affect
   * recording stateblocks, the info should be copied.
   */
   if (mask->changed.group & NINE_STATE_VS_CONST) {
      struct nine_range *r;
   for (r = mask->changed.vs_const_f; r; r = r->next) {
         memcpy(&dst->vs_const_f[r->bgn * 4],
         }
   for (r = mask->changed.vs_const_i; r; r = r->next) {
         memcpy(&dst->vs_const_i[r->bgn * 4],
         }
   for (r = mask->changed.vs_const_b; r; r = r->next) {
         memcpy(&dst->vs_const_b[r->bgn],
                     /* Pixel constants. */
   if (mask->changed.group & NINE_STATE_PS_CONST) {
      struct nine_range *r;
   for (r = mask->changed.ps_const_f; r; r = r->next) {
         memcpy(&dst->ps_const_f[r->bgn * 4],
         }
   if (mask->changed.ps_const_i) {
         uint16_t m = mask->changed.ps_const_i;
   for (i = ffs(m) - 1, m >>= i; m; ++i, m >>= 1)
         }
   if (mask->changed.ps_const_b) {
         uint16_t m = mask->changed.ps_const_b;
   for (i = ffs(m) - 1, m >>= i; m; ++i, m >>= 1)
                     /* Render states.
   * TODO: Maybe build a list ?
   */
   for (i = 0; i < ARRAY_SIZE(mask->changed.rs); ++i) {
      uint32_t m = mask->changed.rs[i];
   /* if (apply)
         while (m) {
         const int r = ffs(m) - 1;
   m &= ~(1 << r);
   DBG("State %d %s = %d\n", i * 32 + r, nine_d3drs_to_string(i * 32 + r), (int)src->rs_advertised[i * 32 + r]);
                  /* Clip planes. */
   if (mask->changed.ucp) {
      DBG("ucp: %x\n", mask->changed.ucp);
   for (i = 0; i < PIPE_MAX_CLIP_PLANES; ++i)
         if (mask->changed.ucp & (1 << i))
         /* if (apply)
               /* Sampler state. */
   if (mask->changed.group & NINE_STATE_SAMPLER) {
      for (s = 0; s < NINE_MAX_SAMPLERS; ++s) {
         if (mask->changed.sampler[s] == 0x3ffe) {
         } else {
      uint32_t m = mask->changed.sampler[s];
   DBG("samp %d: changed = %x\n", i, (int)m);
   while (m) {
      const int i = ffs(m) - 1;
   m &= ~(1 << i);
         }
   /* if (apply)
               /* Index buffer. */
   if (mask->changed.group & NINE_STATE_IDXBUF)
      NineStateBlock9_BindBuffer(device,
                     /* Vertex streams. */
   if (mask->changed.vtxbuf | mask->changed.stream_freq) {
      DBG("vtxbuf/stream_freq: %x/%x\n", mask->changed.vtxbuf, mask->changed.stream_freq);
   uint32_t m = mask->changed.vtxbuf | mask->changed.stream_freq;
   for (i = 0; m; ++i, m >>= 1) {
         if (mask->changed.vtxbuf & (1 << i)) {
      NineStateBlock9_BindBuffer(device,
                     if (src->stream[i]) {
      dst->vtxbuf[i].buffer_offset = src->vtxbuf[i].buffer_offset;
         }
   if (mask->changed.stream_freq & (1 << i))
   }
   /*
      * if (apply) {
   *     dst->changed.vtxbuf |= mask->changed.vtxbuf;
   *     dst->changed.stream_freq |= mask->changed.stream_freq;
            /* Textures */
   if (mask->changed.texture) {
      uint32_t m = mask->changed.texture;
   for (s = 0; m; ++s, m >>= 1)
                     if (!(mask->changed.group & NINE_STATE_FF))
                           if (mask->changed.group & NINE_STATE_FF_MATERIAL)
            if (mask->changed.group & NINE_STATE_FF_PS_CONSTS) {
      for (s = 0; s < NINE_MAX_TEXTURE_STAGES; ++s) {
         for (i = 0; i < NINED3DTSS_COUNT; ++i)
      if (mask->ff.changed.tex_stage[s][i / 32] & (1 << (i % 32)))
      /*
   * if (apply) {
   *    TODO: it's 32 exactly, just offset by 1 as 0 is unused
   *    dst->ff.changed.tex_stage[s][0] |=
   *        mask->ff.changed.tex_stage[s][0];
   *    dst->ff.changed.tex_stage[s][1] |=
   *        mask->ff.changed.tex_stage[s][1];
      }
   if (mask->changed.group & NINE_STATE_FF_LIGHTING) {
      unsigned num_lights = MAX2(dst->ff.num_lights, src->ff.num_lights);
   /* Can happen in Capture() if device state has created new lights after
      * the stateblock was created.
   * Can happen in Apply() if the stateblock had recorded the creation of
      if (dst->ff.num_lights < num_lights) {
         dst->ff.light = REALLOC(dst->ff.light,
               memset(&dst->ff.light[dst->ff.num_lights], 0, (num_lights - dst->ff.num_lights) * sizeof(D3DLIGHT9));
   /* if mask == dst, a Type of 0 will trigger
   * "dst->ff.light[i] = src->ff.light[i];" later,
   * which is what we want in that case. */
   if (mask != dst) {
      for (i = dst->ff.num_lights; i < num_lights; ++i)
      }
   }
   /* Can happen in Capture() if the stateblock had recorded the creation of
      * new lights.
   * Can happen in Apply() if device state has created new lights after
      if (src->ff.num_lights < num_lights) {
         src->ff.light = REALLOC(src->ff.light,
               memset(&src->ff.light[src->ff.num_lights], 0, (num_lights - src->ff.num_lights) * sizeof(D3DLIGHT9));
   for (i = src->ff.num_lights; i < num_lights; ++i)
         }
   /* Note: mask is either src or dst, so at this point src, dst and mask
         for (i = 0; i < num_lights; ++i)
                  memcpy(dst->ff.active_light, src->ff.active_light, sizeof(src->ff.active_light) );
      }
   if (mask->changed.group & NINE_STATE_FF_VSTRANSF) {
      for (i = 0; i < ARRAY_SIZE(mask->ff.changed.transform); ++i) {
         if (!mask->ff.changed.transform[i])
         for (s = i * 32; s < (i * 32 + 32); ++s) {
      if (!(mask->ff.changed.transform[i] & (1 << (s % 32))))
         *nine_state_access_transform(&dst->ff, s, true) =
      }
   /* if (apply)
         }
      static void
   nine_state_copy_common_all(struct NineDevice9 *device,
                              struct nine_state *dst,
      {
               /* if (apply)
   *     dst->changed.group |= src->changed.group;
            dst->viewport = src->viewport;
            nine_bind(&dst->vs, src->vs);
            /* Vertex constants.
   *
   * Various possibilities for optimization here, like creating a per-SB
   * constant buffer, or memcmp'ing for changes.
   * Will do that later depending on what works best for specific apps.
   */
   if (1) {
      memcpy(&dst->vs_const_f[0],
            memcpy(dst->vs_const_i, src->vs_const_i, VS_CONST_I_SIZE(device));
               /* Pixel constants. */
   if (1) {
      struct nine_range *r = help->changed.ps_const_f;
   memcpy(&dst->ps_const_f[0],
            memcpy(dst->ps_const_i, src->ps_const_i, sizeof(dst->ps_const_i));
               /* Render states. */
   memcpy(dst->rs_advertised, src->rs_advertised, sizeof(dst->rs_advertised));
   /* if (apply)
               /* Clip planes. */
   memcpy(&dst->clip, &src->clip, sizeof(dst->clip));
   /* if (apply)
            /* Sampler state. */
   memcpy(dst->samp_advertised, src->samp_advertised, sizeof(dst->samp_advertised));
   /* if (apply)
   *     memcpy(dst->changed.sampler,
            /* Index buffer. */
   NineStateBlock9_BindBuffer(device,
                        /* Vertex streams. */
   if (1) {
      for (i = 0; i < ARRAY_SIZE(dst->stream); ++i) {
         NineStateBlock9_BindBuffer(device,
                     if (src->stream[i]) {
      dst->vtxbuf[i].buffer_offset = src->vtxbuf[i].buffer_offset;
      }
   }
   /* if (apply) {
      *     dst->changed.vtxbuf = (1ULL << MaxStreams) - 1;
   *     dst->changed.stream_freq = (1ULL << MaxStreams) - 1;
            /* Textures */
   if (1) {
      for (i = 0; i < NINE_MAX_SAMPLERS; i++)
               /* keep this check in case we want to disable FF */
   if (!(help->changed.group & NINE_STATE_FF))
                  /* Fixed function state. */
            memcpy(dst->ff.tex_stage, src->ff.tex_stage, sizeof(dst->ff.tex_stage));
   /* if (apply) TODO: memset
   *     memcpy(dst->ff.changed.tex_stage,
            /* Lights. */
   if (1) {
      if (dst->ff.num_lights < src->ff.num_lights) {
         dst->ff.light = REALLOC(dst->ff.light,
               }
   memcpy(dst->ff.light,
            memcpy(dst->ff.active_light, src->ff.active_light, sizeof(src->ff.active_light) );
               /* Transforms. */
   if (1) {
      /* Increase dst size if required (to copy the new states).
      * Increase src size if required (to initialize missing transforms).
      if (dst->ff.num_transforms != src->ff.num_transforms) {
         int num_transforms = MAX2(src->ff.num_transforms, dst->ff.num_transforms);
   nine_state_resize_transform(&src->ff, num_transforms);
   }
   memcpy(dst->ff.transform,
         /* Apply is always used on device state.
      * src is then the D3DSBT_ALL stateblock which
   * ff.changed.transform indicates all matrices are dirty.
   *
   * if (apply)
   *     memcpy(dst->ff.changed.transform,
      }
      /* Capture those bits of current device state that have been changed between
   * BeginStateBlock and EndStateBlock.
   */
   HRESULT NINE_WINAPI
   NineStateBlock9_Capture( struct NineStateBlock9 *This )
   {
      struct NineDevice9 *device = This->base.device;
   struct nine_state *dst = &This->state;
   struct nine_state *src = &device->state;
                     if (This->type == NINESBT_ALL)
         else
            if (dst->changed.group & NINE_STATE_VDECL)
               }
      /* Set state managed by this StateBlock as current device state. */
   HRESULT NINE_WINAPI
   NineStateBlock9_Apply( struct NineStateBlock9 *This )
   {
      struct NineDevice9 *device = This->base.device;
   struct nine_state *dst = &device->state;
   struct nine_state *src = &This->state;
   struct nine_range_pool *pool = &device->range_pool;
                     if (This->type == NINESBT_ALL)
         else
                     if ((src->changed.group & NINE_STATE_VDECL) && src->vdecl)
               }
      IDirect3DStateBlock9Vtbl NineStateBlock9_vtable = {
      (void *)NineUnknown_QueryInterface,
   (void *)NineUnknown_AddRef,
   (void *)NineUnknown_Release,
   (void *)NineUnknown_GetDevice, /* actually part of StateBlock9 iface */
   (void *)NineStateBlock9_Capture,
      };
      static const GUID *NineStateBlock9_IIDs[] = {
      &IID_IDirect3DStateBlock9,
   &IID_IUnknown,
      };
      HRESULT
   NineStateBlock9_new( struct NineDevice9 *pDevice,
               {
         }
