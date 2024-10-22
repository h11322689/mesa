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
      #if NOUVEAU_VP3_DEBUG_FENCE
   static void dump_comm_bsp(struct comm *comm)
   {
      unsigned idx = comm->bsp_cur_index & 0xf;
   debug_printf("Cur seq: %x, bsp byte ofs: %x\n", comm->bsp_cur_index, comm->byte_ofs);
      }
   #endif
      unsigned
   nvc0_decoder_bsp_begin(struct nouveau_vp3_decoder *dec, unsigned comm_seq)
   {
      struct nouveau_screen *screen = nouveau_screen(dec->base.context->screen);
   struct nouveau_bo *bsp_bo = dec->bsp_bo[comm_seq % NOUVEAU_VP3_VIDEO_QDEPTH];
            ret = BO_MAP(screen, bsp_bo, NOUVEAU_BO_WR, dec->client);
   if (ret) {
      debug_printf("map failed: %i %s\n", ret, strerror(-ret));
                           }
      unsigned
   nvc0_decoder_bsp_next(struct nouveau_vp3_decoder *dec,
               {
      struct nouveau_screen *screen = nouveau_screen(dec->base.context->screen);
   struct nouveau_bo *bsp_bo = dec->bsp_bo[comm_seq % NOUVEAU_VP3_VIDEO_QDEPTH];
   struct nouveau_bo *inter_bo = dec->inter_bo[comm_seq & 1];
   uint32_t bsp_size = 0;
   uint32_t i = 0;
            bsp_size = dec->bsp_ptr - (char *)bsp_bo->map;
   for (i = 0; i < num_buffers; i++)
                  if (bsp_size > bsp_bo->size) {
      union nouveau_bo_config cfg;
            cfg.nvc0.tile_mode = 0x10;
            /* round up to the nearest mb */
   bsp_size += (1 << 20) - 1;
            ret = nouveau_bo_new(dec->client->device, NOUVEAU_BO_VRAM, 0, bsp_size, &cfg, &tmp_bo);
   if (ret) {
      debug_printf("reallocating bsp %u -> %u failed with %i\n",
                     ret = BO_MAP(screen, tmp_bo, NOUVEAU_BO_WR, dec->client);
   if (ret) {
      debug_printf("map failed: %i %s\n", ret, strerror(-ret));
               /* Preserve previous buffer. */
   /* TODO: offload this copy to the GPU, as otherwise we're reading and
   * writing to VRAM. */
            /* update position to current chunk */
            nouveau_bo_ref(NULL, &bsp_bo);
               if (!inter_bo || bsp_bo->size * 4 > inter_bo->size) {
      union nouveau_bo_config cfg;
            cfg.nvc0.tile_mode = 0x10;
            ret = nouveau_bo_new(dec->client->device, NOUVEAU_BO_VRAM, 0, bsp_bo->size * 4, &cfg, &tmp_bo);
   if (ret) {
      debug_printf("reallocating inter %u -> %u failed with %i\n",
                     ret = BO_MAP(screen, tmp_bo, NOUVEAU_BO_WR, dec->client);
   if (ret) {
      debug_printf("map failed: %i %s\n", ret, strerror(-ret));
               nouveau_bo_ref(NULL, &inter_bo);
                           }
         unsigned
   nvc0_decoder_bsp_end(struct nouveau_vp3_decoder *dec, union pipe_desc desc,
                     {
      struct nouveau_pushbuf *push = dec->pushbuf[0];
   enum pipe_video_format codec = u_reduce_video_profile(dec->base.profile);
   uint32_t bsp_addr, comm_addr, inter_addr;
   uint32_t slice_size, bucket_size, ring_size;
   uint32_t caps;
   struct nouveau_bo *bsp_bo = dec->bsp_bo[comm_seq % NOUVEAU_VP3_VIDEO_QDEPTH];
   struct nouveau_bo *inter_bo = dec->inter_bo[comm_seq & 1];
   struct nouveau_pushbuf_refn bo_refs[] = {
      { bsp_bo, NOUVEAU_BO_RD | NOUVEAU_BO_VRAM },
   #if NOUVEAU_VP3_DEBUG_FENCE
         #endif
            };
            if (!dec->bitplane_bo)
                              PUSH_SPACE_EX(push, 32, num_refs, 0);
            bsp_addr = bsp_bo->offset >> 8;
         #if NOUVEAU_VP3_DEBUG_FENCE
      memset(dec->comm, 0, 0x200);
      #else
         #endif
         BEGIN_NVC0(push, SUBC_BSP(0x700), 5);
   PUSH_DATA (push, caps); // 700 cmd
   PUSH_DATA (push, bsp_addr + 1); // 704 strparm_bsp
   PUSH_DATA (push, bsp_addr + 7); // 708 str addr
   PUSH_DATA (push, comm_addr); // 70c comm
            if (codec != PIPE_VIDEO_FORMAT_MPEG4_AVC) {
                        nouveau_vp3_inter_sizes(dec, 1, &slice_size, &bucket_size, &ring_size);
   BEGIN_NVC0(push, SUBC_BSP(0x400), 6);
   PUSH_DATA (push, bsp_addr); // 400 picparm addr
   PUSH_DATA (push, inter_addr); // 404 interparm addr
   PUSH_DATA (push, inter_addr + slice_size + bucket_size); // 408 interdata addr
   PUSH_DATA (push, ring_size << 8); // 40c interdata_size
   PUSH_DATA (push, bitplane_addr); // 410 BITPLANE_DATA
      } else {
      nouveau_vp3_inter_sizes(dec, desc.h264->slice_count, &slice_size, &bucket_size, &ring_size);
   BEGIN_NVC0(push, SUBC_BSP(0x400), 8);
   PUSH_DATA (push, bsp_addr); // 400 picparm addr
   PUSH_DATA (push, inter_addr); // 404 interparm addr
   PUSH_DATA (push, slice_size << 8); // 408 interparm size?
   PUSH_DATA (push, inter_addr + slice_size + bucket_size); // 40c interdata addr
   PUSH_DATA (push, ring_size << 8); // 410 interdata size
   PUSH_DATA (push, inter_addr + slice_size); // 414 bucket?
   PUSH_DATA (push, bucket_size << 8); // 418 bucket size? unshifted..
   PUSH_DATA (push, 0); // 41c targets
            #if NOUVEAU_VP3_DEBUG_FENCE
      BEGIN_NVC0(push, SUBC_BSP(0x240), 3);
   PUSH_DATAh(push, dec->fence_bo->offset);
   PUSH_DATA (push, dec->fence_bo->offset);
            BEGIN_NVC0(push, SUBC_BSP(0x300), 1);
   PUSH_DATA (push, 1);
            {
      unsigned spin = 0;
   do {
      usleep(100);
   if ((spin++ & 0xff) == 0xff) {
      debug_printf("b%u: %u\n", dec->fence_seq, dec->fence_map[0]);
                     dump_comm_bsp(dec->comm);
      #else
      BEGIN_NVC0(push, SUBC_BSP(0x300), 1);
   PUSH_DATA (push, 0);
   PUSH_KICK (push);
      #endif
   }
