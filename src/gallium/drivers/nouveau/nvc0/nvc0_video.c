   /*
   * Copyright 2011-2013 Maarten Lankhorst
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included in
   * all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   * OTHER DEALINGS IN THE SOFTWARE.
   */
      #include "nvc0/nvc0_video.h"
      #include "util/u_sampler.h"
   #include "util/format/u_format.h"
      static void
   nvc0_decoder_begin_frame(struct pipe_video_codec *decoder,
               {
      struct nouveau_vp3_decoder *dec = (struct nouveau_vp3_decoder *)decoder;
   uint32_t comm_seq = ++dec->fence_seq;
            assert(dec);
   assert(target);
                        }
      static void
   nvc0_decoder_decode_bitstream(struct pipe_video_codec *decoder,
                                 {
      struct nouveau_vp3_decoder *dec = (struct nouveau_vp3_decoder *)decoder;
   uint32_t comm_seq = dec->fence_seq;
                                 }
      static void
   nvc0_decoder_end_frame(struct pipe_video_codec *decoder,
               {
      struct nouveau_vp3_decoder *dec = (struct nouveau_vp3_decoder *)decoder;
   struct nouveau_vp3_video_buffer *target = (struct nouveau_vp3_video_buffer *)video_target;
   uint32_t comm_seq = dec->fence_seq;
            unsigned vp_caps, is_ref;
   ASSERTED unsigned ret; /* used in debug checks */
                              /* did we decode bitstream correctly? */
            nvc0_decoder_vp(dec, desc, target, comm_seq, vp_caps, is_ref, refs);
      }
      struct pipe_video_codec *
   nvc0_create_decoder(struct pipe_context *context,
         {
      struct nvc0_context *nvc0 = nvc0_context(context);
   struct nouveau_screen *screen = &nvc0->screen->base;
   struct nouveau_vp3_decoder *dec;
   struct nouveau_pushbuf **push;
   union nouveau_bo_config cfg;
            cfg.nvc0.tile_mode = 0x10;
            int ret, i;
   uint32_t codec = 1, ppp_codec = 3;
   uint32_t timeout;
            if (templ->entrypoint != PIPE_VIDEO_ENTRYPOINT_BITSTREAM) {
      debug_printf("%x\n", templ->entrypoint);
               dec = CALLOC_STRUCT(nouveau_vp3_decoder);
   if (!dec)
         dec->client = nvc0->base.client;
   dec->base = *templ;
            if (!kepler) {
      dec->bsp_idx = 5;
   dec->vp_idx = 6;
      } else {
      dec->bsp_idx = 2;
   dec->vp_idx = 2;
               for (i = 0; i < 3; ++i)
      if (i && !kepler) {
      dec->channel[i] = dec->channel[0];
      } else {
      void *data;
   u32 size;
                  if (!kepler) {
      size = sizeof(nvc0_args);
      } else {
      unsigned engine[] = {
      NVE0_FIFO_ENGINE_BSP,
                     nve0_args.engine = engine[i];
   size = sizeof(nve0_args);
               ret = nouveau_object_new(&screen->device->object, 0,
                  if (!ret)
      ret = nouveau_pushbuf_create(screen, &nvc0->base, nvc0->base.client, dec->channel[i],
      if (ret)
                  if (!kepler) {
      if (!ret)
         if (!ret)
         if (!ret)
      } else {
      if (!ret)
         if (!ret)
         if (!ret)
      }
   if (ret)
            BEGIN_NVC0(push[0], SUBC_BSP(NV01_SUBCHAN_OBJECT), 1);
            BEGIN_NVC0(push[1], SUBC_VP(NV01_SUBCHAN_OBJECT), 1);
            BEGIN_NVC0(push[2], SUBC_PPP(NV01_SUBCHAN_OBJECT), 1);
            dec->base.context = context;
   dec->base.begin_frame = nvc0_decoder_begin_frame;
   dec->base.decode_bitstream = nvc0_decoder_decode_bitstream;
            for (i = 0; i < NOUVEAU_VP3_VIDEO_QDEPTH && !ret; ++i)
      ret = nouveau_bo_new(screen->device, NOUVEAU_BO_VRAM,
      if (!ret) {
      /* total fudge factor... just has to be bigger for higher bitrates? */
   unsigned inter_size = align(templ->width * templ->height * 2, 4 << 20);
   ret = nouveau_bo_new(screen->device, NOUVEAU_BO_VRAM,
      }
   if (!ret) {
      ret = nouveau_bo_new(screen->device, NOUVEAU_BO_VRAM,
            }
   if (ret)
         switch (u_reduce_video_profile(templ->profile)) {
   case PIPE_VIDEO_FORMAT_MPEG12: {
      codec = 1;
   assert(templ->max_references <= 2);
      }
   case PIPE_VIDEO_FORMAT_MPEG4: {
      codec = 4;
   tmp_size = mb(templ->height)*16 * mb(templ->width)*16;
   assert(templ->max_references <= 2);
      }
   case PIPE_VIDEO_FORMAT_VC1: {
      ppp_codec = codec = 2;
   tmp_size = mb(templ->height)*16 * mb(templ->width)*16;
   assert(templ->max_references <= 2);
      }
   case PIPE_VIDEO_FORMAT_MPEG4_AVC: {
      codec = 3;
   dec->tmp_stride = 16 * mb_half(templ->width) * nouveau_vp3_video_align(templ->height) * 3 / 2;
   tmp_size = dec->tmp_stride * (templ->max_references + 1);
   assert(templ->max_references <= 16);
      }
   default:
      fprintf(stderr, "invalid codec\n");
               if (screen->device->chipset < 0xd0) {
      ret = nouveau_bo_new(screen->device, NOUVEAU_BO_VRAM, 0,
         if (ret)
            ret = nouveau_vp3_load_firmware(dec, templ->profile, screen->device->chipset);
   if (ret)
               if (codec != 3) {
      ret = nouveau_bo_new(screen->device, NOUVEAU_BO_VRAM, 0,
         if (ret)
               dec->ref_stride = mb(templ->width)*16 * (mb_half(templ->height)*32 + nouveau_vp3_video_align(templ->height)/2);
   ret = nouveau_bo_new(screen->device, NOUVEAU_BO_VRAM, 0,
               if (ret)
                     BEGIN_NVC0(push[0], SUBC_BSP(0x200), 2);
   PUSH_DATA (push[0], codec);
            BEGIN_NVC0(push[1], SUBC_VP(0x200), 2);
   PUSH_DATA (push[1], codec);
            BEGIN_NVC0(push[2], SUBC_PPP(0x200), 2);
   PUSH_DATA (push[2], ppp_codec);
                  #if NOUVEAU_VP3_DEBUG_FENCE
      ret = nouveau_bo_new(screen->device, NOUVEAU_BO_GART|NOUVEAU_BO_MAP,
         if (ret)
            BO_MAP(screen, dec->fence_bo, NOUVEAU_BO_RDWR, screen->client);
   dec->fence_map = dec->fence_bo->map;
   dec->fence_map[0] = dec->fence_map[4] = dec->fence_map[8] = 0;
            /* So lets test if the fence is working? */
   PUSH_SPACE_EX(push[0], 16, 1, 0);
   PUSH_REF1 (push[0], dec->fence_bo, NOUVEAU_BO_GART|NOUVEAU_BO_RDWR);
   BEGIN_NVC0(push[0], SUBC_BSP(0x240), 3);
   PUSH_DATAh(push[0], dec->fence_bo->offset);
   PUSH_DATA (push[0], dec->fence_bo->offset);
            BEGIN_NVC0(push[0], SUBC_BSP(0x304), 1);
   PUSH_DATA (push[0], 0);
            PUSH_SPACE_EX(push[1], 16, 1, 0);
   PUSH_REF1 (push[1], dec->fence_bo, NOUVEAU_BO_GART|NOUVEAU_BO_RDWR);
   BEGIN_NVC0(push[1], SUBC_VP(0x240), 3);
   PUSH_DATAh(push[1], (dec->fence_bo->offset + 0x10));
   PUSH_DATA (push[1], (dec->fence_bo->offset + 0x10));
            BEGIN_NVC0(push[1], SUBC_VP(0x304), 1);
   PUSH_DATA (push[1], 0);
            PUSH_SPACE_EX(push[2], 16, 1, 0);
   PUSH_REF1 (push[2], dec->fence_bo, NOUVEAU_BO_GART|NOUVEAU_BO_RDWR);
   BEGIN_NVC0(push[2], SUBC_PPP(0x240), 3);
   PUSH_DATAh(push[2], (dec->fence_bo->offset + 0x20));
   PUSH_DATA (push[2], (dec->fence_bo->offset + 0x20));
            BEGIN_NVC0(push[2], SUBC_PPP(0x304), 1);
   PUSH_DATA (push[2], 0);
            usleep(100);
   while (dec->fence_seq > dec->fence_map[0] ||
         dec->fence_seq > dec->fence_map[4] ||
      debug_printf("%u: %u %u %u\n", dec->fence_seq, dec->fence_map[0], dec->fence_map[4], dec->fence_map[8]);
      }
      #endif
               fw_fail:
      debug_printf("Cannot create decoder without firmware..\n");
   dec->base.destroy(&dec->base);
         fail:
      debug_printf("Creation failed: %s (%i)\n", strerror(-ret), ret);
   dec->base.destroy(&dec->base);
      }
      struct pipe_video_buffer *
   nvc0_video_buffer_create(struct pipe_context *pipe,
         {
      return nouveau_vp3_video_buffer_create(
      }
