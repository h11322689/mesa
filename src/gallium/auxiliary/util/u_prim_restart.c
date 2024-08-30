   /*
   * Copyright 2014 VMware, Inc.
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
   */
            #include "u_inlines.h"
   #include "util/u_memory.h"
   #include "u_prim_restart.h"
   #include "u_prim.h"
      typedef struct {
   uint32_t count;
   uint32_t primCount;
   uint32_t firstIndex;
   int32_t  baseVertex;
   uint32_t reservedMustBeZero;
   } DrawElementsIndirectCommand;
      static DrawElementsIndirectCommand
   read_indirect_elements(struct pipe_context *context, const struct pipe_draw_indirect_info *indirect)
   {
      DrawElementsIndirectCommand ret;
   struct pipe_transfer *transfer = NULL;
   void *map = NULL;
   /* we only need the first 3 members */
   unsigned read_size = 3 * sizeof(uint32_t);
   assert(indirect->buffer->width0 > 3 * sizeof(uint32_t));
   map = pipe_buffer_map_range(context, indirect->buffer,
                           assert(map);
   memcpy(&ret, map, read_size);
   pipe_buffer_unmap(context, transfer);
      }
      void
   util_translate_prim_restart_data(unsigned index_size,
               {
      if (index_size == 1) {
      uint8_t *src = (uint8_t *) src_map;
   uint16_t *dst = (uint16_t *) dst_map;
   unsigned i;
   for (i = 0; i < count; i++) {
            }
   else if (index_size == 2) {
      uint16_t *src = (uint16_t *) src_map;
   uint16_t *dst = (uint16_t *) dst_map;
   unsigned i;
   for (i = 0; i < count; i++) {
            }
   else {
      uint32_t *src = (uint32_t *) src_map;
   uint32_t *dst = (uint32_t *) dst_map;
   unsigned i;
   assert(index_size == 4);
   for (i = 0; i < count; i++) {
               }
      /** Helper structs for util_draw_vbo_without_prim_restart() */
      struct range_info {
      struct pipe_draw_start_count_bias *draws;
   unsigned count, max;
   unsigned min_index, max_index;
      };
         /**
   * Helper function for util_draw_vbo_without_prim_restart()
   * \return true for success, false if out of memory
   */
   static bool
   add_range(enum mesa_prim mode, struct range_info *info, unsigned start, unsigned count, unsigned index_bias)
   {
      /* degenerate primitive: ignore */
   if (!u_trim_pipe_prim(mode, (unsigned*)&count))
            if (info->max == 0) {
      info->max = 10;
   info->draws = MALLOC(info->max * sizeof(struct pipe_draw_start_count_bias));
   if (!info->draws) {
            }
   else if (info->count == info->max) {
      /* grow the draws[] array */
   info->draws = REALLOC(info->draws,
               if (!info->draws) {
                     }
   info->min_index = MIN2(info->min_index, start);
            /* save the range */
   info->draws[info->count].start = start;
   info->draws[info->count].count = count;
   info->draws[info->count].index_bias = index_bias;
   info->count++;
               }
      struct pipe_draw_start_count_bias *
   util_prim_restart_convert_to_direct(const void *index_map,
                                       {
      struct range_info ranges = { .min_index = UINT32_MAX, 0 };
   unsigned i, start, count;
            assert(info->index_size);
         #define SCAN_INDEXES(TYPE) \
      for (i = 0; i <= draw->count; i++) { \
      if (i == draw->count || \
      ((const TYPE *) index_map)[i] == info->restart_index) { \
   /* cut / restart */ \
   if (count > 0) { \
      if (!add_range(info->mode, &ranges, draw->start + start, count, draw->index_bias)) { \
            } \
   start = i + 1; \
      } \
   else { \
                     start = 0;
   count = 0;
   switch (info->index_size) {
   case 1:
      SCAN_INDEXES(uint8_t);
      case 2:
      SCAN_INDEXES(uint16_t);
      case 4:
      SCAN_INDEXES(uint32_t);
      default:
      assert(!"Bad index size");
               *num_draws = ranges.count;
   *min_index = ranges.min_index;
   *max_index = ranges.max_index;
   *total_index_count = ranges.total_index_count;
      }
      /**
   * Implement primitive restart by breaking an indexed primitive into
   * pieces which do not contain restart indexes.  Each piece is then
   * drawn by calling pipe_context::draw_vbo().
   * \return PIPE_OK if no error, an error code otherwise.
   */
   enum pipe_error
   util_draw_vbo_without_prim_restart(struct pipe_context *context,
                           {
      const void *src_map;
   struct pipe_draw_info new_info = *info;
   struct pipe_draw_start_count_bias new_draw = *draw;
   struct pipe_transfer *src_transfer = NULL;
   DrawElementsIndirectCommand indirect;
   struct pipe_draw_start_count_bias *direct_draws;
            assert(info->index_size);
            switch (info->index_size) {
   case 1:
   case 2:
   case 4:
         default:
      assert(!"Bad index size");
               if (indirect_info && indirect_info->buffer) {
      indirect = read_indirect_elements(context, indirect_info);
   new_draw.count = indirect.count;
   new_draw.start = indirect.firstIndex;
               /* Get pointer to the index data */
   if (!info->has_user_indices) {
      /* map the index buffer (only the range we need to scan) */
   src_map = pipe_buffer_map_range(context, info->index.resource,
                           if (!src_map) {
            }
   else {
      if (!info->index.user) {
      debug_printf("User-space index buffer is null!");
      }
   src_map = (const uint8_t *) info->index.user
               unsigned total_index_count;
   direct_draws = util_prim_restart_convert_to_direct(src_map, &new_info, &new_draw, &num_draws,
               /* unmap index buffer */
   if (src_transfer)
            new_info.primitive_restart = false;
   new_info.index_bounds_valid = true;
   if (direct_draws)
                     }
