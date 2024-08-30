   /*
   * Copyright © 2014 Broadcom
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
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "vc4_context.h"
      void
   vc4_emit_state(struct pipe_context *pctx)
   {
         struct vc4_context *vc4 = vc4_context(pctx);
            if (vc4->dirty & (VC4_DIRTY_SCISSOR | VC4_DIRTY_VIEWPORT |
                  float *vpscale = vc4->viewport.scale;
   float *vptranslate = vc4->viewport.translate;
   float vp_minx = -fabsf(vpscale[0]) + vptranslate[0];
                        /* Clip to the scissor if it's enabled, but still clip to the
   * drawable regardless since that controls where the binner
   * tries to put things.
   *
   * Additionally, always clip the rendering to the viewport,
   * since the hardware does guardband clipping, meaning
   * primitives would rasterize outside of the view volume.
   */
   uint32_t minx, miny, maxx, maxy;
   if (!vc4->rasterizer->base.scissor) {
            minx = MAX2(vp_minx, 0);
   miny = MAX2(vp_miny, 0);
      } else {
            minx = MAX2(vp_minx, vc4->scissor.minx);
                     cl_emit(&job->bcl, CLIP_WINDOW, clip) {
            clip.clip_window_left_pixel_coordinate = minx;
                     job->draw_min_x = MIN2(job->draw_min_x, minx);
   job->draw_min_y = MIN2(job->draw_min_y, miny);
               if (vc4->dirty & (VC4_DIRTY_RASTERIZER |
                                       struct vc4_cl_out *bcl = cl_start(&job->bcl);
   /* HW-2905: If the RCL ends up doing a full-res load when
   * multisampling, then early Z tracking may end up with values
   * from the previous tile due to a HW bug.  Disable it to
   * avoid that.
   *
   * We should be able to skip this when the Z is cleared, but I
   * was seeing bad rendering on glxgears -samples 4 even in
   * that case.
                        /* Don't set the rasterizer to oversample if we're doing our
   * binning and load/stores in single-sample mode.  This is for
   * the samples == 1 case, where vc4 doesn't do any
   * multisampling behavior.
   */
   if (!job->msaa) {
                        cl_u8(&bcl, VC4_PACKET_CONFIGURATION_BITS);
   cl_u8(&bcl,
         (vc4->rasterizer->config_bits[0] |
   cl_u8(&bcl,
         vc4->rasterizer->config_bits[1] |
   cl_u8(&bcl,
                     if (vc4->dirty & VC4_DIRTY_RASTERIZER) {
                  if (vc4->dirty & VC4_DIRTY_VIEWPORT) {
            cl_emit(&job->bcl, CLIPPER_XY_SCALING, clip) {
            clip.viewport_half_width_in_1_16th_of_pixel =
                     cl_emit(&job->bcl, CLIPPER_Z_SCALE_AND_OFFSET, clip) {
            clip.viewport_z_offset_zc_to_zs =
                     cl_emit(&job->bcl, VIEWPORT_OFFSET, vp) {
            vp.viewport_centre_x_coordinate =
                  if (vc4->dirty & VC4_DIRTY_FLAT_SHADE_FLAGS) {
            cl_emit(&job->bcl, FLAT_SHADE_FLAGS, flags) {
            if (vc4->rasterizer->base.flatshade)
   }
