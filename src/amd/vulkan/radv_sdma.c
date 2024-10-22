   /*
   * Copyright 2010 Jerome Glisse <glisse@freedesktop.org>
   * Copyright 2015-2021 Advanced Micro Devices, Inc.
   * All Rights Reserved.
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
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   */
   #include "util/macros.h"
   #include "util/u_memory.h"
   #include "radv_cs.h"
   #include "radv_private.h"
      static bool
   radv_sdma_v4_v5_copy_image_to_buffer(struct radv_device *device, struct radeon_cmdbuf *cs, struct radv_image *image,
         {
      assert(image->plane_count == 1);
   unsigned bpp = image->planes[0].surface.bpe;
   uint64_t dst_address = buffer->bo->va;
   uint64_t src_address = image->bindings[0].bo->va + image->planes[0].surface.u.gfx9.surf_offset;
   unsigned src_pitch = image->planes[0].surface.u.gfx9.surf_pitch;
   unsigned copy_width = DIV_ROUND_UP(image->vk.extent.width, image->planes[0].surface.blk_w);
   unsigned copy_height = DIV_ROUND_UP(image->vk.extent.height, image->planes[0].surface.blk_h);
            /* Linear -> linear sub-window copy. */
   if (image->planes[0].surface.is_linear) {
      bool is_v5_2 = device->physical_device->rad_info.gfx_level >= GFX10_3;
   uint64_t bytes = (uint64_t)src_pitch * copy_height * bpp;
   uint32_t chunk_size = 1u << (is_v5_2 ? 30 : 22);
                              for (int i = 0; i < chunk_count; i++) {
      uint32_t size = MIN2(chunk_size, bytes);
   radeon_emit(cs, CIK_SDMA_PACKET(CIK_SDMA_OPCODE_COPY, CIK_SDMA_COPY_SUB_OPCODE_LINEAR, (tmz ? 4 : 0)));
   radeon_emit(cs, size - 1);
   radeon_emit(cs, 0);
   radeon_emit(cs, src_address);
   radeon_emit(cs, src_address >> 32);
                  src_address += size;
   dst_address += size;
                           }
   /* Tiled sub-window copy -> Linear */
   else {
      unsigned tiled_width = copy_width;
   unsigned tiled_height = copy_height;
   unsigned linear_pitch = region->bufferRowLength;
   uint64_t linear_slice_pitch = (uint64_t)region->bufferRowLength * copy_height;
   uint64_t tiled_address = src_address;
   uint64_t linear_address = dst_address;
   bool is_v5 = device->physical_device->rad_info.gfx_level >= GFX10;
   /* Only SDMA 5 supports DCC with SDMA */
            /* Check if everything fits into the bitfields */
   if (!(tiled_width < (1 << 14) && tiled_height < (1 << 14) && linear_pitch < (1 << 14) &&
                           radeon_emit(cs, CIK_SDMA_PACKET(CIK_SDMA_OPCODE_COPY, CIK_SDMA_COPY_SUB_OPCODE_TILED_SUB_WINDOW, (tmz ? 4 : 0)) |
         radeon_emit(cs, (uint32_t)tiled_address | (image->planes[0].surface.tile_swizzle << 8));
   radeon_emit(cs, (uint32_t)(tiled_address >> 32));
   radeon_emit(cs, 0);
   radeon_emit(cs, ((tiled_width - 1) << 16));
   radeon_emit(cs, (tiled_height - 1));
   radeon_emit(cs, util_logbase2(bpp) | image->planes[0].surface.u.gfx9.swizzle_mode << 3 |
               radeon_emit(cs, (uint32_t)linear_address);
   radeon_emit(cs, (uint32_t)(linear_address >> 32));
   radeon_emit(cs, 0);
   radeon_emit(cs, ((linear_pitch - 1) << 16));
   radeon_emit(cs, linear_slice_pitch - 1);
   radeon_emit(cs, (copy_width - 1) | ((copy_height - 1) << 16));
            if (dcc) {
      uint64_t md_address = tiled_address + image->planes[0].surface.meta_offset;
   const struct util_format_description *desc;
                  desc = vk_format_description(image->vk.format);
                  /* Add metadata */
   radeon_emit(cs, (uint32_t)md_address);
   radeon_emit(cs, (uint32_t)(md_address >> 32));
   radeon_emit(cs, hw_fmt | vi_alpha_is_on_msb(device, format) << 8 | hw_type << 9 |
                                                   }
      bool
   radv_sdma_copy_image(struct radv_device *device, struct radeon_cmdbuf *cs, struct radv_image *image,
         {
      assert(device->physical_device->rad_info.gfx_level >= GFX9);
      }
      void
   radv_sdma_copy_buffer(const struct radv_device *device, struct radeon_cmdbuf *cs, uint64_t src_va, uint64_t dst_va,
         {
      if (size == 0)
            enum amd_gfx_level gfx_level = device->physical_device->rad_info.gfx_level;
   unsigned max_size_per_packet = gfx_level >= GFX10_3 ? GFX103_SDMA_COPY_MAX_SIZE : CIK_SDMA_COPY_MAX_SIZE;
   unsigned align = ~0u;
   unsigned ncopy = DIV_ROUND_UP(size, max_size_per_packet);
                     /* SDMA FW automatically enables a faster dword copy mode when
   * source, destination and size are all dword-aligned.
   *
   * When source and destination are dword-aligned, round down the size to
   * take advantage of faster dword copy, and copy the remaining few bytes
   * with the last copy packet.
   */
   if ((src_va & 0x3) == 0 && (dst_va & 0x3) == 0 && size > 4 && (size & 0x3) != 0) {
      align = ~0x3u;
                        for (unsigned i = 0; i < ncopy; i++) {
      unsigned csize = size >= 4 ? MIN2(size & align, max_size_per_packet) : size;
   radeon_emit(cs, CIK_SDMA_PACKET(CIK_SDMA_OPCODE_COPY, CIK_SDMA_COPY_SUB_OPCODE_LINEAR, (tmz ? 1u : 0) << 2));
   radeon_emit(cs, gfx_level >= GFX9 ? csize - 1 : csize);
   radeon_emit(cs, 0); /* src/dst endian swap */
   radeon_emit(cs, src_va);
   radeon_emit(cs, src_va >> 32);
   radeon_emit(cs, dst_va);
   radeon_emit(cs, dst_va >> 32);
   dst_va += csize;
   src_va += csize;
         }
