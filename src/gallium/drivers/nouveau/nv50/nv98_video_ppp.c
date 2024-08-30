   /*
   * Copyright 2011-2013 Maarten Lankhorst, Ilia Mirkin
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
      #include "nv50/nv98_video.h"
      static void
   nv98_decoder_setup_ppp(struct nouveau_vp3_decoder *dec, struct nouveau_vp3_video_buffer *target, uint32_t low700) {
               uint32_t stride_in = mb(dec->base.width);
   uint32_t stride_out = mb(target->resources[0]->width0);
   uint32_t dec_h = mb(dec->base.height);
   uint32_t dec_w = mb(dec->base.width);
   uint64_t in_addr;
   uint32_t y2, cbcr, cbcr2, i;
   struct nouveau_pushbuf_refn bo_refs[] = {
      { NULL, NOUVEAU_BO_WR | NOUVEAU_BO_VRAM },
   { NULL, NOUVEAU_BO_WR | NOUVEAU_BO_VRAM },
   #if NOUVEAU_VP3_DEBUG_FENCE
         #endif
      };
            for (i = 0; i < 2; ++i) {
      struct nv50_miptree *mt = (struct nv50_miptree *)target->resources[i];
               PUSH_REFN(push, bo_refs, num_refs);
            BEGIN_NV04(push, SUBC_PPP(0x700), 10);
            PUSH_DATA (push, (stride_out << 24) | (stride_out << 16) | low700); // 700
   PUSH_DATA (push, (stride_in << 24) | (stride_in << 16) | (dec_h << 8) | dec_w); // 704
            /* Input: */
   PUSH_DATA (push, in_addr); // 708
   PUSH_DATA (push, in_addr + y2); // 70c
   PUSH_DATA (push, in_addr + cbcr); // 710
            for (i = 0; i < 2; ++i) {
               PUSH_DATA (push, mt->base.address >> 8);
   PUSH_DATA (push, (mt->base.address + mt->total_size/2) >> 8);
         }
      static uint32_t
   nv98_decoder_vc1_ppp(struct nouveau_vp3_decoder *dec, struct pipe_vc1_picture_desc *desc, struct nouveau_vp3_video_buffer *target) {
               nv98_decoder_setup_ppp(dec, target, 0x1412);
   assert(!desc->deblockEnable);
   assert(!(dec->base.width & 0xf));
            BEGIN_NV04(push, SUBC_PPP(0x400), 1);
            // 728 = wtf?
      }
      void
   nv98_decoder_ppp(struct nouveau_vp3_decoder *dec, union pipe_desc desc, struct nouveau_vp3_video_buffer *target, unsigned comm_seq) {
      enum pipe_video_format codec = u_reduce_video_profile(dec->base.profile);
   struct nouveau_pushbuf *push = dec->pushbuf[2];
                     switch (codec) {
   case PIPE_VIDEO_FORMAT_MPEG12: {
      unsigned mpeg2 = dec->base.profile != PIPE_VIDEO_PROFILE_MPEG1;
   nv98_decoder_setup_ppp(dec, target, 0x1410 | mpeg2);
      }
   case PIPE_VIDEO_FORMAT_MPEG4: nv98_decoder_setup_ppp(dec, target, 0x1414); break;
   case PIPE_VIDEO_FORMAT_VC1: ppp_caps = nv98_decoder_vc1_ppp(dec, desc.vc1, target); break;
   case PIPE_VIDEO_FORMAT_MPEG4_AVC: nv98_decoder_setup_ppp(dec, target, 0x1413); break;
   default: assert(0);
   }
   BEGIN_NV04(push, SUBC_PPP(0x734), 2);
   PUSH_DATA (push, comm_seq);
         #if NOUVEAU_VP3_DEBUG_FENCE
      BEGIN_NV04(push, SUBC_PPP(0x240), 3);
   PUSH_DATAh(push, (dec->fence_bo->offset + 0x20));
   PUSH_DATA (push, (dec->fence_bo->offset + 0x20));
            BEGIN_NV04(push, SUBC_PPP(0x300), 1);
   PUSH_DATA (push, 1);
            {
               do {
      usleep(100);
   if ((spin++ & 0xff) == 0xff)
            #else
      BEGIN_NV04(push, SUBC_PPP(0x300), 1);
   PUSH_DATA (push, 0);
      #endif
   }
