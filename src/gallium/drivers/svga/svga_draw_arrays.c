   /**********************************************************
   * Copyright 2008-2009 VMware, Inc.  All rights reserved.
   *
   * Permission is hereby granted, free of charge, to any person
   * obtaining a copy of this software and associated documentation
   * files (the "Software"), to deal in the Software without
   * restriction, including without limitation the rights to use, copy,
   * modify, merge, publish, distribute, sublicense, and/or sell copies
   * of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be
   * included in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   **********************************************************/
      #include "svga_cmd.h"
      #include "indices/u_indices.h"
   #include "util/u_inlines.h"
   #include "util/u_prim.h"
      #include "svga_hw_reg.h"
   #include "svga_draw.h"
   #include "svga_draw_private.h"
   #include "svga_context.h"
   #include "svga_shader.h"
         #define DBG 0
         static enum pipe_error
   generate_indices(struct svga_hwtnl *hwtnl,
                     {
      struct pipe_context *pipe = &hwtnl->svga->pipe;
   struct pipe_transfer *transfer;
   unsigned size = index_size * nr;
   struct pipe_resource *dst = NULL;
            dst = pipe_buffer_create(pipe->screen, PIPE_BIND_INDEX_BUFFER,
         if (!dst)
            dst_map = pipe_buffer_map(pipe, dst, PIPE_MAP_WRITE, &transfer);
   if (!dst_map)
                              *out_buf = dst;
         fail:
      if (dst_map)
            if (dst)
               }
         static bool
   compare(unsigned cached_nr, unsigned nr, unsigned type)
   {
      if (type == U_GENERATE_REUSABLE)
         else
      }
         static enum pipe_error
   retrieve_or_generate_indices(struct svga_hwtnl *hwtnl,
                              enum mesa_prim prim,
      {
      enum pipe_error ret = PIPE_OK;
                     for (i = 0; i < IDX_CACHE_MAX; i++) {
      if (hwtnl->index_cache[prim][i].buffer != NULL &&
      hwtnl->index_cache[prim][i].generate == generate) {
   if (compare(hwtnl->index_cache[prim][i].gen_nr, gen_nr, gen_type)) {
                                       }
   else if (gen_type == U_GENERATE_REUSABLE) {
                     if (DBG)
                                    if (i == IDX_CACHE_MAX) {
      unsigned smallest = 0;
            for (i = 0; i < IDX_CACHE_MAX && smallest_size; i++) {
      if (hwtnl->index_cache[prim][i].buffer == NULL) {
      smallest = i;
      }
   else if (hwtnl->index_cache[prim][i].gen_nr < smallest) {
      smallest = i;
                           pipe_resource_reference(&hwtnl->index_cache[prim][smallest].buffer,
            if (DBG)
                              ret = generate_indices(hwtnl, gen_nr, gen_size, generate, out_buf);
   if (ret != PIPE_OK)
            hwtnl->index_cache[prim][i].generate = generate;
   hwtnl->index_cache[prim][i].gen_nr = gen_nr;
            if (DBG)
      debug_printf("%s cache %d/%d\n", __func__,
      done:
      SVGA_STATS_TIME_POP(svga_sws(hwtnl->svga));
      }
         static enum pipe_error
   simple_draw_arrays(struct svga_hwtnl *hwtnl,
                     {
      SVGA3dPrimitiveRange range;
   unsigned hw_prim;
            hw_prim = svga_translate_prim(prim, count, &hw_count, vertices_per_patch);
   if (hw_count == 0)
            range.primType = hw_prim;
   range.primitiveCount = hw_count;
   range.indexArray.surfaceId = SVGA3D_INVALID_ID;
   range.indexArray.offset = 0;
   range.indexArray.stride = 0;
   range.indexWidth = 0;
            /* Min/max index should be calculated prior to applying bias, so we
   * end up with min_index = 0, max_index = count - 1 and everybody
   * looking at those numbers knows to adjust them by
   * range.indexBias.
   */
   return svga_hwtnl_prim(hwtnl, &range, count,
                  }
         enum pipe_error
   svga_hwtnl_draw_arrays(struct svga_hwtnl *hwtnl,
                     {
      enum mesa_prim gen_prim;
   unsigned gen_size, gen_nr;
   enum indices_mode gen_type;
   u_generate_func gen_func;
   enum pipe_error ret = PIPE_OK;
   unsigned api_pv = hwtnl->api_pv;
                     if (svga->curr.rast->templ.fill_front !=
      svga->curr.rast->templ.fill_back) {
               if (svga->curr.rast->templ.flatshade &&
            /* The fragment color is a constant, not per-vertex so the whole
   * primitive will be the same color (except for possible blending).
   * We can ignore the current provoking vertex state and use whatever
   * the hardware wants.
   */
            if (hwtnl->api_fillmode == PIPE_POLYGON_MODE_FILL) {
      /* Do some simple primitive conversions to avoid index buffer
   * generation below.  Note that polygons and quads are not directly
   * supported by the svga device.  Also note, we can only do this
   * for flat/constant-colored rendering because of provoking vertex.
   */
   if (prim == MESA_PRIM_POLYGON) {
         }
   else if (prim == MESA_PRIM_QUADS && count == 4) {
                        if (svga_need_unfilled_fallback(hwtnl, prim)) {
      /* Convert unfilled polygons into points, lines, triangles */
   gen_type = u_unfilled_generator(prim,
                              }
   else {
      /* Convert MESA_PRIM_LINE_LOOP to MESA_PRIM_LINESTRIP,
   * convert MESA_PRIM_POLYGON to MESA_PRIM_TRIANGLE_FAN,
   * etc, if needed (as determined by svga_hw_prims mask).
   */
   gen_type = u_index_generator(svga_hw_prims,
                              prim,
            if (gen_type == U_GENERATE_LINEAR) {
      ret = simple_draw_arrays(hwtnl, gen_prim, start, count,
            }
   else {
               /* Need to draw as indexed primitive.
   * Potentially need to run the gen func to build an index buffer.
   */
   ret = retrieve_or_generate_indices(hwtnl,
                           if (ret == PIPE_OK) {
      util_debug_message(&svga->debug.callback, PERF_INFO,
                  ret = svga_hwtnl_simple_draw_range_elements(hwtnl,
                                             gen_buf,
               if (gen_buf) {
                     SVGA_STATS_TIME_POP(svga_sws(svga));
      }
