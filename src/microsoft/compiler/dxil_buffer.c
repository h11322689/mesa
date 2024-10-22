   /*
   * Copyright © Microsoft Corporation
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "dxil_buffer.h"
   #include <assert.h>
      void
   dxil_buffer_init(struct dxil_buffer *b, unsigned abbrev_width)
   {
      blob_init(&b->blob);
   b->buf = 0;
               }
      void
   dxil_buffer_finish(struct dxil_buffer *b)
   {
         }
      static bool
   flush_dword(struct dxil_buffer *b)
   {
               uint32_t lower_bits = b->buf & UINT32_MAX;
   if (!blob_write_bytes(&b->blob, &lower_bits, sizeof(lower_bits)))
            b->buf >>= 32;
               }
      bool
   dxil_buffer_emit_bits(struct dxil_buffer *b, uint32_t data, unsigned width)
   {
      assert(b->buf_bits < 32);
   assert(width > 0 && width <= 32);
            b->buf |= ((uint64_t)data) << b->buf_bits;
            if (b->buf_bits >= 32)
               }
      bool
   dxil_buffer_emit_vbr_bits(struct dxil_buffer *b, uint64_t data,
         {
               uint32_t tag = UINT32_C(1) << (width - 1);
   uint32_t max = tag - 1;
   while (data > max) {
      uint32_t value = (data & max) | tag;
            if (!dxil_buffer_emit_bits(b, value, width))
                  }
      bool
   dxil_buffer_align(struct dxil_buffer *b)
   {
               if (b->buf_bits) {
      b->buf_bits = 32;
                  }
