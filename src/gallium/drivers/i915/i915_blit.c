   /**************************************************************************
   *
   * Copyright 2003 VMware, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      #include "i915_blit.h"
   #include "i915_batch.h"
   #include "i915_debug.h"
   #include "i915_reg.h"
      void
   i915_fill_blit(struct i915_context *i915, unsigned cpp, unsigned rgba_mask,
                     {
               I915_DBG(DBG_BLIT, "%s dst:buf(%p)/%d+%d %d,%d sz:%dx%d\n", __func__,
            if (!i915_winsys_validate_buffers(i915->batch, &dst_buffer, 1)) {
      FLUSH_BATCH(NULL, I915_FLUSH_ASYNC);
               switch (cpp) {
   case 1:
   case 2:
   case 3:
      BR13 = (((int)dst_pitch) & 0xffff) | (0xF0 << 16) | (1 << 24);
   CMD = XY_COLOR_BLT_CMD;
      case 4:
      BR13 = (((int)dst_pitch) & 0xffff) | (0xF0 << 16) | (1 << 24) | (1 << 25);
   CMD = (XY_COLOR_BLT_CMD | rgba_mask);
      default:
                  if (!BEGIN_BATCH(6)) {
      FLUSH_BATCH(NULL, I915_FLUSH_ASYNC);
      }
   OUT_BATCH(CMD);
   OUT_BATCH(BR13);
   OUT_BATCH((y << 16) | x);
   OUT_BATCH(((y + h) << 16) | (x + w));
   OUT_RELOC_FENCED(dst_buffer, I915_USAGE_2D_TARGET, dst_offset);
               }
      void
   i915_copy_blit(struct i915_context *i915, unsigned cpp,
                  unsigned short src_pitch, struct i915_winsys_buffer *src_buffer,
   unsigned src_offset, unsigned short dst_pitch,
      {
      unsigned CMD, BR13;
   int dst_y2 = dst_y + h;
   int dst_x2 = dst_x + w;
            I915_DBG(DBG_BLIT,
            "%s src:buf(%p)/%d+%d %d,%d dst:buf(%p)/%d+%d %d,%d sz:%dx%d\n",
         if (!i915_winsys_validate_buffers(i915->batch, buffers, 2)) {
      FLUSH_BATCH(NULL, I915_FLUSH_ASYNC);
               switch (cpp) {
   case 1:
   case 2:
   case 3:
      BR13 = (((int)dst_pitch) & 0xffff) | (0xCC << 16) | (1 << 24);
   CMD = XY_SRC_COPY_BLT_CMD;
      case 4:
      BR13 = (((int)dst_pitch) & 0xffff) | (0xCC << 16) | (1 << 24) | (1 << 25);
   CMD = (XY_SRC_COPY_BLT_CMD | XY_SRC_COPY_BLT_WRITE_ALPHA |
            default:
                  if (dst_y2 < dst_y || dst_x2 < dst_x) {
                  /* Hardware can handle negative pitches but loses the ability to do
   * proper overlapping blits in that case.  We don't really have a
   * need for either at this stage.
   */
            if (!BEGIN_BATCH(8)) {
      FLUSH_BATCH(NULL, I915_FLUSH_ASYNC);
      }
   OUT_BATCH(CMD);
   OUT_BATCH(BR13);
   OUT_BATCH((dst_y << 16) | dst_x);
   OUT_BATCH((dst_y2 << 16) | dst_x2);
   OUT_RELOC_FENCED(dst_buffer, I915_USAGE_2D_TARGET, dst_offset);
   OUT_BATCH((src_y << 16) | src_x);
   OUT_BATCH(((int)src_pitch & 0xffff));
               }
