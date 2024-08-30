   /*
   * Copyright 2013 Ilia Mirkin
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
      #include "nv50/nv84_video.h"
      #include "util/u_sse.h"
      struct h264_iparm1 {
      uint8_t scaling_lists_4x4[6][16]; // 00
   uint8_t scaling_lists_8x8[2][64]; // 60
   uint32_t width; // e0
   uint32_t height; // e4
   uint64_t ref1_addrs[16]; // e8
   uint64_t ref2_addrs[16]; // 168
   uint32_t unk1e8;
   uint32_t unk1ec;
   uint32_t w1; // 1f0
   uint32_t w2; // 1f4
   uint32_t w3; // 1f8
   uint32_t h1; // 1fc
   uint32_t h2; // 200
   uint32_t h3; // 204
   uint32_t mb_adaptive_frame_field_flag; // 208
   uint32_t field_pic_flag; // 20c
   uint32_t format; // 210
      };
      struct h264_iparm2 {
      uint32_t width; // 00
   uint32_t height; // 04
   uint32_t mbs; // 08
   uint32_t w1; // 0c
   uint32_t w2; // 10
   uint32_t w3; // 14
   uint32_t h1; // 18
   uint32_t h2; // 1c
   uint32_t h3; // 20
   uint32_t unk24;
   uint32_t mb_adaptive_frame_field_flag; // 28
   uint32_t top; // 2c
   uint32_t bottom; // 30
      };
      void
   nv84_decoder_vp_h264(struct nv84_decoder *dec,
               {
      struct h264_iparm1 param1;
   struct h264_iparm2 param2;
   int i, width = align(dest->base.width, 16),
            struct nouveau_pushbuf *push = dec->vp_pushbuf;
   struct nouveau_pushbuf_refn bo_refs[] = {
      { dest->interlaced, NOUVEAU_BO_RDWR | NOUVEAU_BO_VRAM },
   { dest->full, NOUVEAU_BO_RDWR | NOUVEAU_BO_VRAM },
   { dec->vpring, NOUVEAU_BO_RDWR | NOUVEAU_BO_VRAM },
   { dec->mbring, NOUVEAU_BO_RDWR | NOUVEAU_BO_VRAM },
   { dec->vp_params, NOUVEAU_BO_RDWR | NOUVEAU_BO_GART },
      };
   int num_refs = ARRAY_SIZE(bo_refs);
            STATIC_ASSERT(sizeof(struct h264_iparm1) == 0x218);
            memset(&param1, 0, sizeof(param1));
            memcpy(&param1.scaling_lists_4x4, desc->pps->ScalingList4x4,
         memcpy(&param1.scaling_lists_8x8, desc->pps->ScalingList8x8,
            param1.width = width;
   param1.w1 = param1.w2 = param1.w3 = align(width, 64);
   param1.height = param1.h2 = height;
   param1.h1 = param1.h3 = align(height, 32);
   param1.format = 0x3231564e; /* 'NV12' */
   param1.mb_adaptive_frame_field_flag = desc->pps->sps->mb_adaptive_frame_field_flag;
            param2.width = width;
   param2.w1 = param2.w2 = param2.w3 = param1.w1;
   if (desc->field_pic_flag)
         else
         param2.h1 = param2.h2 = align(height, 32);
   param2.h3 = height;
   param2.mbs = width * height >> 8;
   if (desc->field_pic_flag) {
      param2.top = desc->bottom_field_flag ? 2 : 1;
      }
   param2.mb_adaptive_frame_field_flag = desc->pps->sps->mb_adaptive_frame_field_flag;
                              for (i = 0; i < 16; i++) {
      struct nv84_video_buffer *buf = (struct nv84_video_buffer *)desc->ref[i];
   struct nouveau_bo *bo1, *bo2;
   if (buf) {
      bo1 = buf->interlaced;
   bo2 = buf->full;
   if (i == 0)
      } else {
      bo1 = dest->interlaced;
      }
   param1.ref1_addrs[i] = bo1->offset;
   param1.ref2_addrs[i] = bo2->offset;
   struct nouveau_pushbuf_refn bo_refs[] = {
      { bo1, NOUVEAU_BO_RDWR | NOUVEAU_BO_VRAM },
      };
               memcpy(dec->vp_params->map, &param1, sizeof(param1));
                     /* Wait for BSP to have completed */
   BEGIN_NV04(push, SUBC_VP(0x10), 4);
   PUSH_DATAh(push, dec->fence->offset);
   PUSH_DATA (push, dec->fence->offset);
   PUSH_DATA (push, 2);
            /* VP step 1 */
   BEGIN_NV04(push, SUBC_VP(0x400), 15);
   PUSH_DATA (push, 1);
   PUSH_DATA (push, param2.mbs);
   PUSH_DATA (push, 0x3987654); /* each nibble probably a dma index */
   PUSH_DATA (push, 0x55001); /* constant */
   PUSH_DATA (push, dec->vp_params->offset >> 8);
   PUSH_DATA (push, (dec->vpring->offset + dec->vpring_residual) >> 8);
   PUSH_DATA (push, dec->vpring_ctrl);
   PUSH_DATA (push, dec->vpring->offset >> 8);
   PUSH_DATA (push, dec->bitstream->size / 2 - 0x700);
   PUSH_DATA (push, (dec->mbring->offset + dec->mbring->size - 0x2000) >> 8);
   PUSH_DATA (push, (dec->vpring->offset + dec->vpring_ctrl +
         PUSH_DATA (push, 0);
   PUSH_DATA (push, 0x100008);
   PUSH_DATA (push, dest->interlaced->offset >> 8);
            BEGIN_NV04(push, SUBC_VP(0x620), 2);
   PUSH_DATA (push, 0);
            BEGIN_NV04(push, SUBC_VP(0x300), 1);
            /* VP step 2 */
   BEGIN_NV04(push, SUBC_VP(0x400), 5);
   PUSH_DATA (push, 0x54530201);
   PUSH_DATA (push, (dec->vp_params->offset >> 8) + 0x4);
   PUSH_DATA (push, (dec->vpring->offset + dec->vpring_ctrl +
         PUSH_DATA (push, dest->interlaced->offset >> 8);
            if (is_ref) {
      BEGIN_NV04(push, SUBC_VP(0x414), 1);
               BEGIN_NV04(push, SUBC_VP(0x620), 2);
   PUSH_DATAh(push, dec->vp_fw2_offset);
            BEGIN_NV04(push, SUBC_VP(0x300), 1);
            /* Set the semaphore back to 1 */
   BEGIN_NV04(push, SUBC_VP(0x610), 3);
   PUSH_DATAh(push, dec->fence->offset);
   PUSH_DATA (push, dec->fence->offset);
            /* Write to the semaphore location, intr */
   BEGIN_NV04(push, SUBC_VP(0x304), 1);
            for (i = 0; i < 2; i++) {
      struct nv50_miptree *mt = nv50_miptree(dest->resources[i]);
                  }
      static inline int16_t inverse_quantize(int16_t val, uint8_t quant, int mpeg1) {
      int16_t ret = val * quant / 16;
   if (mpeg1 && ret) {
      if (ret > 0)
         else
      }
   if (ret < -2048)
         else if (ret > 2047)
            }
      struct mpeg12_mb_info {
      uint32_t index;
   uint8_t unk4;
   uint8_t unk5;
   uint16_t coded_block_pattern;
   uint8_t block_counts[6];
   uint16_t PMV[8];
      };
      void
   nv84_decoder_vp_mpeg12_mb(struct nv84_decoder *dec,
               {
               struct mpeg12_mb_info info = {0};
   int i, sum = 0, mask, block_index, count;
   const int16_t *blocks;
   int intra = macrob->macroblock_type & PIPE_MPEG12_MB_TYPE_INTRA;
   int motion = macrob->macroblock_type &
         const uint8_t *quant_matrix = intra ? dec->mpeg12_intra_matrix :
                  info.index = macrob->y * mb(dec->base.width) + macrob->x;
   info.unk4 = motion;
   if (intra)
         if (macrob->macroblock_modes.bits.dct_type)
         info.unk5 = (macrob->motion_vertical_field_select << 4) |
         info.coded_block_pattern = macrob->coded_block_pattern;
   if (motion) {
         }
   blocks = macrob->blocks;
   for (mask = 0x20, block_index = 0; mask > 0; mask >>= 1, block_index++) {
      if ((macrob->coded_block_pattern & mask) == 0)
                     /*
   * The observation here is that there are a lot of 0's, and things go
   * a lot faster if one skips over them.
      #if DETECT_ARCH_SSE && DETECT_ARCH_X86_64
   /* Note that the SSE implementation is much more tuned to X86_64. As it's not
   * benchmarked on X86_32, disable it there. I suspect that the code needs to
   * be reorganized in terms of 32-bit wide data in order to be more
   * efficient. NV84+ were released well into the 64-bit CPU era, so it should
   * be a minority case.
   */
      /* This returns a 16-bit bit-mask, each 2 bits are both 1 or both 0, depending
   * on whether the corresponding (16-bit) word in blocks is zero or non-zero. */
   #define wordmask(blocks, zero) \
         (uint64_t)(_mm_movemask_epi8( \
                           /* TODO: Look into doing the inverse quantization in terms of SSE
   * operations unconditionally, when necessary. */
   uint64_t bmask0 = wordmask(blocks, zero);
   bmask0 |= wordmask(blocks + 8, zero) << 16;
   bmask0 |= wordmask(blocks + 16, zero) << 32;
   bmask0 |= wordmask(blocks + 24, zero) << 48;
   uint64_t bmask1 = wordmask(blocks + 32, zero);
   bmask1 |= wordmask(blocks + 40, zero) << 16;
   bmask1 |= wordmask(blocks + 48, zero) << 32;
            /* The wordmask macro returns the inverse of what we want, since it
   * returns a 1 for equal-to-zero. Invert. */
   bmask0 = ~bmask0;
            /* Note that the bitmask is actually sequences of 2 bits for each block
   * index. This is because there is no movemask_epi16. That means that
   * (a) ffs will never return 64, since the prev bit will always be set
   * in that case, and (b) we need to do an extra bit shift. Or'ing the
   * bitmasks together is faster than having a loop that computes them one
   * at a time and processes them, on a Core i7-920. Trying to put bmask
   * into an array and then looping also slows things down.
            /* shift needs to be the same width as i, and unsigned so that / 2
   * becomes a rshift operation */
   uint32_t shift;
            if (dec->base.entrypoint == PIPE_VIDEO_ENTRYPOINT_BITSTREAM) {
      int16_t tmp;
   while ((shift = __builtin_ffsll(bmask0))) {
      i += (shift - 1) / 2;
   bmask0 >>= shift - 1;
   *dec->mpeg12_data++ = dec->zscan[i] * 2;
   tmp = inverse_quantize(blocks[i], quant_matrix[i], mpeg1);
   *dec->mpeg12_data++ = tmp;
   sum += tmp;
   count++;
   i++;
      }
   i = 32;
   while ((shift = __builtin_ffsll(bmask1))) {
      i += (shift - 1) / 2;
   bmask1 >>= shift - 1;
   *dec->mpeg12_data++ = dec->zscan[i] * 2;
   tmp = inverse_quantize(blocks[i], quant_matrix[i], mpeg1);
   *dec->mpeg12_data++ = tmp;
   sum += tmp;
   count++;
   i++;
         } else {
      while ((shift = __builtin_ffsll(bmask0))) {
      i += (shift - 1) / 2;
   bmask0 >>= shift - 1;
   *dec->mpeg12_data++ = i * 2;
   *dec->mpeg12_data++ = blocks[i];
   count++;
   i++;
      }
   i = 32;
   while ((shift = __builtin_ffsll(bmask1))) {
      i += (shift - 1) / 2;
   bmask1 >>= shift - 1;
   *dec->mpeg12_data++ = i * 2;
   *dec->mpeg12_data++ = blocks[i];
   count++;
   i++;
         #undef wordmask
   #else
            /*
   * This loop looks ridiculously written... and it is. I tried a lot of
   * different ways of achieving this scan, and this was the fastest, at
   * least on a Core i7-920. Note that it's not necessary to skip the 0's,
   * the firmware will deal with those just fine. But it's faster to skip
   * them. Note to people trying benchmarks: make sure to use realistic
   * mpeg data, which can often be a single data point first followed by
   * 63 0's, or <data> 7x <0> <data> 7x <0> etc.
   */
   i = 0;
   if (dec->base.entrypoint == PIPE_VIDEO_ENTRYPOINT_BITSTREAM) {
      while (true) {
      int16_t tmp;
   while (likely(i < 64 && !(tmp = blocks[i]))) i++;
   if (i >= 64) break;
   *dec->mpeg12_data++ = dec->zscan[i] * 2;
   tmp = inverse_quantize(tmp, quant_matrix[i], mpeg1);
   *dec->mpeg12_data++ = tmp;
   sum += tmp;
   count++;
         } else {
      while (true) {
      int16_t tmp;
   while (likely(i < 64 && !(tmp = blocks[i]))) i++;
   if (i >= 64) break;
   *dec->mpeg12_data++ = i * 2;
   *dec->mpeg12_data++ = tmp;
   count++;
            #endif
            if (dec->base.entrypoint == PIPE_VIDEO_ENTRYPOINT_BITSTREAM) {
      if (!mpeg1 && (sum & 1) == 0) {
      if (count && *(dec->mpeg12_data - 2) == 63 * 2) {
      uint16_t *val = dec->mpeg12_data - 1;
   if (*val & 1) *val -= 1;
      } else {
      *dec->mpeg12_data++ = 63 * 2;
   *dec->mpeg12_data++ = 1;
                     if (count) {
         } else {
      *dec->mpeg12_data++ = 1;
   *dec->mpeg12_data++ = 0;
      }
   info.block_counts[block_index] = count;
               memcpy(dec->mpeg12_mb_info, &info, sizeof(info));
            if (macrob->num_skipped_macroblocks) {
      info.index++;
   info.coded_block_pattern = 0;
   info.skipped = macrob->num_skipped_macroblocks - 1;
   memset(info.block_counts, 0, sizeof(info.block_counts));
   memcpy(dec->mpeg12_mb_info, &info, sizeof(info));
         }
      struct mpeg12_header {
      uint32_t luma_top_size; // 00
   uint32_t luma_bottom_size; // 04
   uint32_t chroma_top_size; // 08
   uint32_t mbs; // 0c
   uint32_t mb_info_size; // 10
   uint32_t mb_width_minus1; // 14
   uint32_t mb_height_minus1; // 18
   uint32_t width; // 1c
   uint32_t height; // 20
   uint8_t progressive; // 24
   uint8_t mocomp_only; // 25
   uint8_t frames; // 26
   uint8_t picture_structure; // 27
   uint32_t unk28; // 28 -- 0x50100
   uint32_t unk2c; // 2c
      };
      void
   nv84_decoder_vp_mpeg12(struct nv84_decoder *dec,
               {
      struct nouveau_pushbuf *push = dec->vp_pushbuf;
   struct nv84_video_buffer *ref1 = (struct nv84_video_buffer *)desc->ref[0];
   struct nv84_video_buffer *ref2 = (struct nv84_video_buffer *)desc->ref[1];
   struct nouveau_pushbuf_refn bo_refs[] = {
      { dest->interlaced, NOUVEAU_BO_RDWR | NOUVEAU_BO_VRAM },
   { NULL, NOUVEAU_BO_RDWR | NOUVEAU_BO_VRAM },
   { NULL, NOUVEAU_BO_RDWR | NOUVEAU_BO_VRAM },
      };
   int i, num_refs = ARRAY_SIZE(bo_refs);
   struct mpeg12_header header = {0};
   struct nv50_miptree *y = nv50_miptree(dest->resources[0]);
                     if (!ref1)
         if (!ref2)
         bo_refs[1].bo = ref1->interlaced;
            header.luma_top_size = y->layer_stride;
   header.luma_bottom_size = y->layer_stride;
   header.chroma_top_size = uv->layer_stride;
   header.mbs = mb(dec->base.width) * mb(dec->base.height);
   header.mb_info_size = dec->mpeg12_mb_info - dec->mpeg12_bo->map - 0x100;
   header.mb_width_minus1 = mb(dec->base.width) - 1;
   header.mb_height_minus1 = mb(dec->base.height) - 1;
   header.width = align(dec->base.width, 16);
   header.height = align(dec->base.height, 16);
   header.progressive = desc->frame_pred_frame_dct;
   header.frames = 1 + (desc->ref[0] != NULL) + (desc->ref[1] != NULL);
   header.picture_structure = desc->picture_structure;
                                       BEGIN_NV04(push, SUBC_VP(0x400), 9);
   PUSH_DATA (push, 0x543210); /* each nibble possibly a dma index */
   PUSH_DATA (push, 0x555001); /* constant */
   PUSH_DATA (push, dec->mpeg12_bo->offset >> 8);
   PUSH_DATA (push, (dec->mpeg12_bo->offset + 0x100) >> 8);
   PUSH_DATA (push, (dec->mpeg12_bo->offset + 0x100 +
               PUSH_DATA (push, dest->interlaced->offset >> 8);
   PUSH_DATA (push, ref1->interlaced->offset >> 8);
   PUSH_DATA (push, ref2->interlaced->offset >> 8);
            BEGIN_NV04(push, SUBC_VP(0x620), 2);
   PUSH_DATA (push, 0);
            BEGIN_NV04(push, SUBC_VP(0x300), 1);
            for (i = 0; i < 2; i++) {
      struct nv50_miptree *mt = nv50_miptree(dest->resources[i]);
      }
      }
