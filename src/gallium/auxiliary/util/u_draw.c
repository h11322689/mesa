   /**************************************************************************
   *
   * Copyright 2011 VMware, Inc.
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
         #include "util/u_debug.h"
   #include "util/u_inlines.h"
   #include "util/u_math.h"
   #include "util/format/u_format.h"
   #include "util/u_draw.h"
         /**
   * Returns the largest legal index value plus one for the current set
   * of bound vertex buffers.  Regardless of any other consideration,
   * all vertex lookups need to be clamped to 0..max_index-1 to prevent
   * an out-of-bound access.
   *
   * Note that if zero is returned it means that one or more buffers is
   * too small to contain any valid vertex data.
   */
   unsigned
   util_draw_max_index(
         const struct pipe_vertex_buffer *vertex_buffers,
   const struct pipe_vertex_element *vertex_elements,
   unsigned nr_vertex_elements,
   {
      unsigned max_index;
            max_index = ~0U - 1;
   for (i = 0; i < nr_vertex_elements; i++) {
      const struct pipe_vertex_element *element =
         const struct pipe_vertex_buffer *buffer =
         unsigned buffer_size;
   const struct util_format_description *format_desc;
            if (buffer->is_user_buffer || !buffer->buffer.resource) {
                  assert(buffer->buffer.resource->height0 == 1);
   assert(buffer->buffer.resource->depth0 == 1);
            format_desc = util_format_description(element->src_format);
   assert(format_desc->block.width == 1);
   assert(format_desc->block.height == 1);
   assert(format_desc->block.bits % 8 == 0);
            if (buffer->buffer_offset >= buffer_size) {
      /* buffer is too small */
                        if (element->src_offset >= buffer_size) {
      /* buffer is too small */
                        if (format_size > buffer_size) {
      /* buffer is too small */
                        if (element->src_stride != 0) {
                        if (element->instance_divisor == 0) {
      /* Per-vertex data */
      }
   else {
      /* Per-instance data. Simply make sure gallium frontends didn't
   * request more instances than those that fit in the buffer */
   if ((info->start_instance + info->instance_count)/element->instance_divisor
      > (buffer_max_index + 1)) {
   /* FIXME: We really should stop thinking in terms of maximum
   * indices/instances and simply start clamping against buffer
   * size. */
   debug_printf("%s: too many instances for vertex buffer\n",
                                 }
      struct u_indirect_params *
   util_draw_indirect_read(struct pipe_context *pipe,
                     {
      struct pipe_transfer *transfer;
   uint32_t *params;
   struct u_indirect_params *draws;
            assert(indirect);
            uint32_t draw_count = indirect->draw_count;
   if (indirect->indirect_draw_count) {
      struct pipe_transfer *dc_transfer;
   uint32_t *dc_param = pipe_buffer_map_range(pipe,
                     if (!dc_transfer) {
      debug_printf("%s: failed to map indirect draw count buffer\n", __func__);
      }
   draw_count = dc_param[0];
      }
   if (!draw_count) {
      *num_draws = draw_count;
      }
   draws = malloc(sizeof(struct u_indirect_params) * draw_count);
   if (!draws)
            unsigned map_size = (draw_count - 1) * indirect->stride + (num_params * sizeof(uint32_t));
   params = pipe_buffer_map_range(pipe,
                                 if (!transfer) {
      debug_printf("%s: failed to map indirect buffer\n", __func__);
   free(draws);
               for (unsigned i = 0; i < draw_count; i++) {
      memcpy(&draws[i].info, info_in, sizeof(struct pipe_draw_info));
   draws[i].draw.count = params[0];
   draws[i].info.instance_count = params[1];
   draws[i].draw.start = params[2];
   draws[i].draw.index_bias = info_in->index_size ? params[3] : 0;
   draws[i].info.start_instance = info_in->index_size ? params[4] : params[3];
      }
   pipe_buffer_unmap(pipe, transfer);
   *num_draws = draw_count;
      }
      /* This extracts the draw arguments from the indirect resource,
   * puts them into a new instance of pipe_draw_info, and calls draw_vbo on it.
   */
   void
   util_draw_indirect(struct pipe_context *pipe,
               {
      struct pipe_draw_info info;
   struct pipe_transfer *transfer;
   uint32_t *params;
            assert(indirect);
                              if (indirect->indirect_draw_count) {
      struct pipe_transfer *dc_transfer;
   uint32_t *dc_param = pipe_buffer_map_range(pipe,
                     if (!dc_transfer) {
      debug_printf("%s: failed to map indirect draw count buffer\n", __func__);
      }
   if (dc_param[0] < draw_count)
                     if (!draw_count)
            if (indirect->stride)
         params = (uint32_t *)
      pipe_buffer_map_range(pipe,
                        indirect->buffer,
   if (!transfer) {
      debug_printf("%s: failed to map indirect buffer\n", __func__);
               for (unsigned i = 0; i < draw_count; i++) {
               draw.count = params[0];
   info.instance_count = params[1];
   draw.start = params[2];
   draw.index_bias = info_in->index_size ? params[3] : 0;
                        }
      }
      void
   util_draw_multi(struct pipe_context *pctx, const struct pipe_draw_info *info,
                  unsigned drawid_offset,
      {
      struct pipe_draw_info tmp_info = *info;
            /* If you call this with num_draws==1, that is probably going to be
   * an infinite loop
   */
            for (unsigned i = 0; i < num_draws; i++) {
      if (indirect || (draws[i].count && info->instance_count))
         if (tmp_info.increment_draw_id)
         }
