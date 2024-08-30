   /*
   * Copyright (C) 2018 Rob Clark <robclark@freedesktop.org>
   * Copyright © 2018-2019 Google, Inc.
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   * Authors:
   *    Rob Clark <robclark@freedesktop.org>
   */
      #include <stdio.h>
      #include "freedreno_layout.h"
      static bool
   is_r8g8(const struct fdl_layout *layout)
   {
      return layout->cpp == 2 &&
      }
      void
   fdl6_get_ubwc_blockwidth(const struct fdl_layout *layout,
         {
      static const struct {
      uint8_t width;
      } blocksize[] = {
      { 16, 4 }, /* cpp = 1 */
   { 16, 4 }, /* cpp = 2 */
   { 16, 4 }, /* cpp = 4 */
   {  8, 4 }, /* cpp = 8 */
   {  4, 4 }, /* cpp = 16 */
   {  4, 2 }, /* cpp = 32 */
               /* special case for r8g8: */
   if (is_r8g8(layout)) {
      *blockwidth = 16;
   *blockheight = 8;
               if (layout->format == PIPE_FORMAT_Y8_UNORM) {
      *blockwidth = 32;
   *blockheight = 8;
               /* special case for 2bpp + MSAA (not layout->cpp is already
   * pre-multiplied by nr_samples):
   */
   if ((layout->cpp / layout->nr_samples == 2) && (layout->nr_samples > 1)) {
      if (layout->nr_samples == 2) {
      *blockwidth = 8;
      } else if (layout->nr_samples == 4) {
      *blockwidth = 4;
      } else {
         }
               uint32_t cpp = fdl_cpp_shift(layout);
   assert(cpp < ARRAY_SIZE(blocksize));
   *blockwidth = blocksize[cpp].width;
      }
      static void
   fdl6_tile_alignment(struct fdl_layout *layout, uint32_t *heightalign)
   {
      layout->pitchalign = fdl_cpp_shift(layout);
            if (is_r8g8(layout) || layout->cpp == 1) {
      layout->pitchalign = 1;
      } else if (layout->cpp == 2) {
                  /* Empirical evidence suggests that images with UBWC could have much
   * looser alignment requirements, however the validity of alignment is
   * heavily undertested and the "officially" supported alignment is 4096b.
   */
   if (layout->ubwc)
         else if (layout->cpp == 1)
         else if (layout->cpp == 2)
         else
      }
      /* NOTE: good way to test this is:  (for example)
   *  piglit/bin/texelFetch fs sampler3D 100x100x8
   */
   bool
   fdl6_layout(struct fdl_layout *layout, enum pipe_format format,
               uint32_t nr_samples, uint32_t width0, uint32_t height0,
   {
      uint32_t offset = 0, heightalign;
            assert(nr_samples > 0);
   layout->width0 = width0;
   layout->height0 = height0;
   layout->depth0 = depth0;
            layout->cpp = util_format_get_blocksize(format);
   layout->cpp *= nr_samples;
            layout->format = format;
   layout->nr_samples = nr_samples;
                     if (depth0 > 1 || ubwc_blockwidth == 0)
            if (layout->ubwc || util_format_is_depth_or_stencil(format))
            /* in layer_first layout, the level (slice) contains just one
   * layer (since in fact the layer contains the slices)
   */
            /* note: for tiled+noubwc layouts, we can use a lower pitchalign
   * which will affect the linear levels only, (the hardware will still
   * expect the tiled alignment on the tiled levels)
   */
   if (layout->tile_mode) {
         } else {
      layout->base_align = 64;
   layout->pitchalign = 0;
   /* align pitch to at least 16 pixels:
   * both turnip and galium assume there is enough alignment for 16x4
   * aligned gmem store. turnip can use CP_BLIT to work without this
   * extra alignment, but gallium driver doesn't implement it yet
   */
   if (layout->cpp > 4)
            /* when possible, use a bit more alignment than necessary
   * presumably this is better for performance?
   */
   if (!explicit_layout)
            /* not used, avoid "may be used uninitialized" warning */
                        if (explicit_layout) {
      offset = explicit_layout->offset;
   layout->pitch0 = explicit_layout->pitch;
   if (align(layout->pitch0, 1 << layout->pitchalign) != layout->pitch0)
               uint32_t ubwc_width0 = width0;
   uint32_t ubwc_height0 = height0;
   uint32_t ubwc_tile_height_alignment = RGB_TILE_HEIGHT_ALIGNMENT;
   if (mip_levels > 1) {
      /* With mipmapping enabled, UBWC layout is power-of-two sized,
   * specified in log2 width/height in the descriptors.  The height
   * alignment is 64 for mipmapping, but for buffer sharing (always
   * single level) other participants expect 16.
   */
   ubwc_width0 = util_next_power_of_two(width0);
   ubwc_height0 = util_next_power_of_two(height0);
      }
   layout->ubwc_width0 = align(DIV_ROUND_UP(ubwc_width0, ubwc_blockwidth),
         ubwc_height0 = align(DIV_ROUND_UP(ubwc_height0, ubwc_blockheight),
                     for (uint32_t level = 0; level < mip_levels; level++) {
      uint32_t depth = u_minify(depth0, level);
   struct fdl_slice *slice = &layout->slices[level];
   struct fdl_slice *ubwc_slice = &layout->ubwc_slices[level];
   uint32_t tile_mode = fdl_tile_mode(layout, level);
   uint32_t pitch = fdl_pitch(layout, level);
            uint32_t nblocksy = util_format_get_nblocksy(format, height);
   if (tile_mode)
            /* The blits used for mem<->gmem work at a granularity of
   * 16x4, which can cause faults due to over-fetch on the
   * last level.  The simple solution is to over-allocate a
   * bit the last level to ensure any over-fetch is harmless.
   * The pitch is already sufficiently aligned, but height
   * may not be. note this only matters if last level is linear
   */
   if (level == mip_levels - 1)
                     /* 1d array and 2d array textures must all have the same layer size for
   * each miplevel on a6xx.  For 3D, the layer size automatically reduces
   * until the value we specify in TEX_CONST_3_MIN_LAYERSZ, which is used to
   * make sure that we follow alignment requirements after minification.
   */
   if (is_3d) {
      if (level == 0) {
         } else if (min_3d_layer_size) {
         } else {
                     /* If this level didn't reduce the pitch by half, then fix it up,
   * and this is the end of layer size reduction.
   */
   uint32_t pitch = fdl_pitch(layout, level);
                  /* If the height is now less than the alignment requirement, then
   * scale it up and let this be the minimum layer size.
   */
                  /* If the size would become un-page-aligned, stay aligned instead. */
   if (align(slice->size0, 4096) != slice->size0)
         } else {
                           if (layout->ubwc) {
                     uint32_t meta_pitch = fdl_ubwc_pitch(layout, level);
                  ubwc_slice->size0 =
         ubwc_slice->offset = offset + layout->ubwc_layer_size;
                  if (layout->layer_first) {
      layout->layer_size = align(layout->size, 4096);
               /* Place the UBWC slices before the uncompressed slices, because the
   * kernel expects UBWC to be at the start of the buffer.  In the HW, we
   * get to program the UBWC and non-UBWC offset/strides
   * independently.
   */
   if (layout->ubwc) {
      for (uint32_t level = 0; level < mip_levels; level++)
                     /* include explicit offset in size */
               }
