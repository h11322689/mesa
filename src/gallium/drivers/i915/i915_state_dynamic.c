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
      #include "i915_batch.h"
   #include "i915_context.h"
   #include "i915_reg.h"
   #include "i915_state.h"
   #include "i915_state_inlines.h"
      #include "util/u_memory.h"
   #include "util/u_pack_color.h"
      /* State that we have chosen to store in the DYNAMIC segment of the
   * i915 indirect state mechanism.
   *
   * Can't cache these in the way we do the static state, as there is no
   * start/size in the command packet, instead an 'end' value that gets
   * incremented.
   *
   * Additionally, there seems to be a requirement to re-issue the full
   * (active) state every time a 4kb boundary is crossed.
   */
      static inline void
   set_dynamic(struct i915_context *i915, unsigned offset, const unsigned state)
   {
      if (i915->current.dynamic[offset] == state)
            i915->current.dynamic[offset] = state;
   i915->dynamic_dirty |= 1 << offset;
      }
      static inline void
   set_dynamic_array(struct i915_context *i915, unsigned offset,
         {
               if (!memcmp(src, &i915->current.dynamic[offset], dwords * 4))
            for (i = 0; i < dwords; i++) {
      i915->current.dynamic[offset + i] = src[i];
                  }
      /***********************************************************************
   * Modes4: stencil masks and logicop
   */
   static void
   upload_MODES4(struct i915_context *i915)
   {
                        /* I915_NEW_STENCIL
   */
   if (stencil_ccw)
         else
            /* I915_NEW_BLEND
   */
               }
      const struct i915_tracked_state i915_upload_MODES4 = {
      "MODES4", upload_MODES4,
         /***********************************************************************
   */
   static void
   upload_BFO(struct i915_context *i915)
   {
               unsigned bfo[2];
   if (stencil_ccw) {
      bfo[0] = i915->depth_stencil->bfo_ccw[0];
      } else {
      bfo[0] = i915->depth_stencil->bfo_cw[0];
      }
   /* I don't get it only allowed to set a ref mask when the enable bit is set?
   */
   if (bfo[0] & BFO_ENABLE_STENCIL_REF) {
      bfo[0] |= i915->stencil_ref.ref_value[!stencil_ccw]
                  }
      const struct i915_tracked_state i915_upload_BFO = {
            /***********************************************************************
   */
   static void
   upload_BLENDCOLOR(struct i915_context *i915)
   {
                        /* I915_NEW_BLEND
   */
   {
               bc[0] = _3DSTATE_CONST_BLEND_COLOR_CMD;
   bc[1] = pack_ui32_float4(color[i915->current.color_swizzle[2]],
                              }
      const struct i915_tracked_state i915_upload_BLENDCOLOR = {
            /***********************************************************************
   */
   static void
   upload_IAB(struct i915_context *i915)
   {
               if (i915->blend) {
      struct i915_surface *cbuf = i915_surface(i915->framebuffer.cbufs[0]);
   if (cbuf && cbuf->alpha_in_g)
         else if (cbuf && cbuf->alpha_is_x)
         else
                  }
      const struct i915_tracked_state i915_upload_IAB = {
            /***********************************************************************
   */
   static void
   upload_DEPTHSCALE(struct i915_context *i915)
   {
      set_dynamic_array(i915, I915_DYNAMIC_DEPTHSCALE_0,
      }
      const struct i915_tracked_state i915_upload_DEPTHSCALE = {
            /***********************************************************************
   * Polygon stipple
   *
   * The i915 supports a 4x4 stipple natively, GL wants 32x32.
   * Fortunately stipple is usually a repeating pattern.
   *
   * XXX: does stipple pattern need to be adjusted according to
   * the window position?
   *
   * XXX: possibly need workaround for conform paths test.
   */
   static void
   upload_STIPPLE(struct i915_context *i915)
   {
               st[0] = _3DSTATE_STIPPLE;
            /* I915_NEW_RASTERIZER
   */
   if (i915->rasterizer)
            /* I915_NEW_STIPPLE
   */
   {
      const uint8_t *mask = (const uint8_t *)i915->poly_stipple.stipple;
            p[0] = mask[12] & 0xf;
   p[1] = mask[8] & 0xf;
   p[2] = mask[4] & 0xf;
            /* Not sure what to do about fallbacks, so for now just dont:
   */
                  }
      const struct i915_tracked_state i915_upload_STIPPLE = {
            /***********************************************************************
   * Scissor enable
   */
   static void
   upload_SCISSOR_ENABLE(struct i915_context *i915)
   {
         }
      const struct i915_tracked_state i915_upload_SCISSOR_ENABLE = {
            /***********************************************************************
   * Scissor rect
   */
   static void
   upload_SCISSOR_RECT(struct i915_context *i915)
   {
      unsigned x1 = i915->scissor.minx;
   unsigned y1 = i915->scissor.miny;
   unsigned x2 = i915->scissor.maxx - 1;
   unsigned y2 = i915->scissor.maxy - 1;
            sc[0] = _3DSTATE_SCISSOR_RECT_0_CMD;
   sc[1] = (y1 << 16) | (x1 & 0xffff);
               }
      const struct i915_tracked_state i915_upload_SCISSOR_RECT = {
            /***********************************************************************
   */
   static const struct i915_tracked_state *atoms[] = {
      &i915_upload_MODES4,         &i915_upload_BFO,
   &i915_upload_BLENDCOLOR,     &i915_upload_IAB,
   &i915_upload_DEPTHSCALE,     &i915_upload_STIPPLE,
         /* These will be dynamic indirect state commands, but for now just end
   * up on the batch buffer with everything else.
   */
   static void
   update_dynamic(struct i915_context *i915)
   {
               for (i = 0; i < ARRAY_SIZE(atoms); i++)
      if (i915->dirty & atoms[i]->dirty)
   }
      struct i915_tracked_state i915_hw_dynamic = {
      "dynamic", update_dynamic,
      };
