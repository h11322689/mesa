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
      #include "indices/u_indices.h"
   #include "util/u_inlines.h"
   #include "util/u_prim.h"
   #include "util/u_upload_mgr.h"
      #include "svga_cmd.h"
   #include "svga_draw.h"
   #include "svga_draw_private.h"
   #include "svga_resource_buffer.h"
   #include "svga_winsys.h"
   #include "svga_context.h"
   #include "svga_hw_reg.h"
         /**
   * Return a new index buffer which contains a translation of the original
   * index buffer.  An example of a translation is converting from QUAD
   * primitives to TRIANGLE primitives.  Each set of four indexes for a quad
   * will be converted to six indices for two triangles.
   *
   * Before generating the new index buffer we'll check if the incoming
   * buffer already has a translated buffer that can be re-used.
   * This benefits demos like Cinebench R15 which has many
   * glDrawElements(GL_QUADS) commands (we can't draw quads natively).
   *
   * \param offset  offset in bytes to first index to translate in src buffer
   * \param orig_prim  original primitive type (like MESA_PRIM_QUADS)
   * \param gen_prim  new/generated primitive type (like MESA_PRIM_TRIANGLES)
   * \param orig_nr  number of indexes to translate in source buffer
   * \param gen_nr  number of indexes to write into new/dest buffer
   * \param index_size  bytes per index (2 or 4)
   * \param translate  the translation function from the u_translate module
   * \param out_buf  returns the new/translated index buffer
   * \return error code to indicate success failure
   */
   static enum pipe_error
   translate_indices(struct svga_hwtnl *hwtnl,
                     const struct pipe_draw_info *info,
   const struct pipe_draw_start_count_bias *draw,
   enum mesa_prim gen_prim,
   unsigned orig_nr, unsigned gen_nr,
   unsigned gen_size,
   {
      struct pipe_context *pipe = &hwtnl->svga->pipe;
   struct svga_screen *screen = svga_screen(pipe->screen);
   struct svga_buffer *src_sbuf = NULL;
   struct pipe_transfer *src_transfer = NULL;
   struct pipe_transfer *dst_transfer = NULL;
   const unsigned size = gen_size * gen_nr;
   const unsigned offset = draw->start * info->index_size;
   const void *src_map = NULL;
   struct pipe_resource *dst = NULL;
            assert(gen_size == 2 || gen_size == 4);
   if (!info->has_user_indices)
            /* If the draw_info provides us with a buffer rather than a
   * user pointer, Check to see if we've already translated that buffer
   */
   if (src_sbuf && !screen->debug.no_cache_index_buffers) {
      /* Check if we already have a translated index buffer */
   if (src_sbuf->translated_indices.buffer &&
      src_sbuf->translated_indices.orig_prim == info->mode &&
   src_sbuf->translated_indices.new_prim == gen_prim &&
   src_sbuf->translated_indices.offset == offset &&
   src_sbuf->translated_indices.count == orig_nr &&
   src_sbuf->translated_indices.index_size == gen_size) {
   pipe_resource_reference(out_buf, src_sbuf->translated_indices.buffer);
                  /* Need to trim vertex count to make sure we don't write too much data
   * to the dst buffer in the translate() call.
   */
            if (src_sbuf) {
      /* If we have a source buffer, create a destination buffer in the
   * hope that we can reuse the translated data later. If not,
   * we'd probably be better off using the upload buffer.
   */
   dst = pipe_buffer_create(pipe->screen,
               if (!dst)
            dst_map = pipe_buffer_map(pipe, dst, PIPE_MAP_WRITE, &dst_transfer);
   if (!dst_map)
            *out_offset = 0;
   src_map = pipe_buffer_map(pipe, info->index.resource,
                     if (!src_map)
      } else {
      /* Allocate upload buffer space. Align to the index size. */
   u_upload_alloc(pipe->stream_uploader, 0, size, gen_size,
         if (!dst)
                                 if (src_transfer)
            if (dst_transfer)
         else
                     if (src_sbuf && !screen->debug.no_cache_index_buffers) {
      /* Save the new, translated index buffer in the hope we can use it
   * again in the future.
   */
   pipe_resource_reference(&src_sbuf->translated_indices.buffer, dst);
   src_sbuf->translated_indices.orig_prim = info->mode;
   src_sbuf->translated_indices.new_prim = gen_prim;
   src_sbuf->translated_indices.offset = offset;
   src_sbuf->translated_indices.count = orig_nr;
                     fail:
      if (src_transfer)
            if (dst_transfer)
         else if (dst_map)
            if (dst)
               }
         enum pipe_error
   svga_hwtnl_simple_draw_range_elements(struct svga_hwtnl *hwtnl,
                                       struct pipe_resource *index_buffer,
   unsigned index_size, int index_bias,
   {
      SVGA3dPrimitiveRange range;
   unsigned hw_prim;
   unsigned hw_count;
            hw_prim = svga_translate_prim(prim, count, &hw_count, vertices_per_patch);
   if (hw_count == 0)
            range.primType = hw_prim;
   range.primitiveCount = hw_count;
   range.indexArray.offset = index_offset;
   range.indexArray.stride = index_size;
   range.indexWidth = index_size;
            return svga_hwtnl_prim(hwtnl, &range, count,
                  }
         enum pipe_error
   svga_hwtnl_draw_range_elements(struct svga_hwtnl *hwtnl,
                     {
      struct pipe_context *pipe = &hwtnl->svga->pipe;
   enum mesa_prim gen_prim;
   unsigned gen_size, gen_nr;
   enum indices_mode gen_type;
   u_translate_func gen_func;
            SVGA_STATS_TIME_PUSH(svga_sws(hwtnl->svga),
            if (svga_need_unfilled_fallback(hwtnl, info->mode)) {
      gen_type = u_unfilled_translator(info->mode,
                              }
   else {
               /* There is no geometry ordering with PATCH, so no need to
   * consider provoking vertex mode for the translation.
   * So use the same api_pv as the hw_pv.
   */
   hw_pv = info->mode == MESA_PRIM_PATCHES ? hwtnl->api_pv :
         gen_type = u_index_translator(svga_hw_prims,
                                 info->mode,
               if ((gen_type == U_TRANSLATE_MEMCPY) && (info->index_size == gen_size)) {
      /* No need for translation, just pass through to hardware:
   */
   unsigned start_offset = draw->start * info->index_size;
   struct pipe_resource *index_buffer = NULL;
            if (info->has_user_indices) {
      u_upload_data(pipe->stream_uploader, 0, count * info->index_size,
               u_upload_unmap(pipe->stream_uploader);
      } else {
      pipe_resource_reference(&index_buffer, info->index.resource);
                        ret = svga_hwtnl_simple_draw_range_elements(hwtnl, index_buffer,
                                             info->index_size,
      }
   else {
      struct pipe_resource *gen_buf = NULL;
            /* Need to allocate a new index buffer and run the translate
   * func to populate it.  Could potentially cache this translated
   * index buffer with the original to avoid future
   * re-translations.  Not much point if we're just accelerating
   * GL though, as index buffers are typically used only once
   * there.
   */
   ret = translate_indices(hwtnl, info, draw, gen_prim,
               if (ret == PIPE_OK) {
      gen_offset /= gen_size;
   ret = svga_hwtnl_simple_draw_range_elements(hwtnl,
                                             gen_buf,
   gen_size,
               if (gen_buf) {
                     SVGA_STATS_TIME_POP(svga_sws(hwtnl->svga));
      }
