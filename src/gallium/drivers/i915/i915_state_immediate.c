   /**************************************************************************
   *
   * Copyright 2007 VMware, Inc.
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
   /*
   * Authors:
   *   Keith Whitwell <keithw@vmware.com>
   */
      #include "util/u_memory.h"
   #include "i915_context.h"
   #include "i915_reg.h"
   #include "i915_state.h"
   #include "i915_state_inlines.h"
      /* Convinience function to check immediate state.
   */
      static inline void
   set_immediate(struct i915_context *i915, unsigned offset, const unsigned state)
   {
      if (i915->current.immediate[offset] == state)
            i915->current.immediate[offset] = state;
   i915->immediate_dirty |= 1 << offset;
      }
      /***********************************************************************
   * S0,S1: Vertex buffer state.
   */
   static void
   upload_S0S1(struct i915_context *i915)
   {
               /* I915_NEW_VBO
   */
            /* Need to force this */
   if (i915->dirty & I915_NEW_VBO) {
      i915->immediate_dirty |= 1 << I915_IMMEDIATE_S0;
               /* I915_NEW_VERTEX_SIZE
   */
   {
                           set_immediate(i915, I915_IMMEDIATE_S0, LIS0);
      }
      const struct i915_tracked_state i915_upload_S0S1 = {
            /***********************************************************************
   * S4: Vertex format, rasterization state
   */
   static void
   upload_S2S4(struct i915_context *i915)
   {
               /* I915_NEW_VERTEX_FORMAT
   */
   {
      LIS2 = i915->current.vertex_info.hwfmt[1];
   LIS4 = i915->current.vertex_info.hwfmt[0];
                        set_immediate(i915, I915_IMMEDIATE_S2, LIS2);
      }
      const struct i915_tracked_state i915_upload_S2S4 = {
            /***********************************************************************
   */
   static void
   upload_S5(struct i915_context *i915)
   {
      unsigned LIS5 = 0;
            /* I915_NEW_DEPTH_STENCIL
   */
   if (stencil_ccw)
         else
         /* hope it's safe to set stencil ref value even if stencil test is disabled?
   */
            /* I915_NEW_BLEND
   */
         #if 0
      /* I915_NEW_RASTERIZER
   */
   if (i915->rasterizer->LIS7) {
            #endif
            }
      const struct i915_tracked_state i915_upload_S5 = {
      "imm S5", upload_S5,
         /***********************************************************************
   */
   static void
   upload_S6(struct i915_context *i915)
   {
               /* I915_NEW_FRAMEBUFFER
   */
   if (i915->framebuffer.cbufs[0])
            /* I915_NEW_BLEND
   */
   if (i915->blend) {
      struct i915_surface *cbuf = i915_surface(i915->framebuffer.cbufs[0]);
   if (cbuf && cbuf->alpha_in_g)
         else if (cbuf && cbuf->alpha_is_x)
         else
               /* I915_NEW_DEPTH
   */
   if (i915->depth_stencil)
            if (i915->rasterizer)
               }
      const struct i915_tracked_state i915_upload_S6 = {
      "imm S6", upload_S6,
   I915_NEW_BLEND | I915_NEW_DEPTH_STENCIL | I915_NEW_FRAMEBUFFER |
         /***********************************************************************
   */
   static void
   upload_S7(struct i915_context *i915)
   {
   #if 0
               /* I915_NEW_RASTERIZER
   */
               #endif
   }
      const struct i915_tracked_state i915_upload_S7 = {"imm S7", upload_S7,
            /***********************************************************************
   */
   static const struct i915_tracked_state *atoms[] = {
      &i915_upload_S0S1, &i915_upload_S2S4, &i915_upload_S5, &i915_upload_S6,
         static void
   update_immediate(struct i915_context *i915)
   {
               for (i = 0; i < ARRAY_SIZE(atoms); i++)
      if (i915->dirty & atoms[i]->dirty)
   }
      struct i915_tracked_state i915_hw_immediate = {
      "immediate", update_immediate,
      };
