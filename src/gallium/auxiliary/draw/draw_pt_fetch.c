   /**************************************************************************
   *
   * Copyright 2008 VMware, Inc.
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
      #include "util/u_memory.h"
   #include "util/u_math.h"
   #include "util/format/u_format.h"
   #include "draw/draw_context.h"
   #include "draw/draw_private.h"
   #include "draw/draw_pt.h"
   #include "translate/translate.h"
   #include "translate/translate_cache.h"
         struct pt_fetch {
                                    };
         /**
   * Perform the fetch from API vertex elements & vertex buffers, to a
   * contiguous set of float[4] attributes as required for the
   * vertex_shader->run_linear() method.
   */
   void
   draw_pt_fetch_prepare(struct pt_fetch *fetch,
                     {
      struct draw_context *draw = fetch->draw;
   unsigned nr_inputs;
   unsigned nr = 0, ei = 0;
   unsigned dst_offset = 0;
   unsigned num_extra_inputs = 0;
                     /* Leave the clipmask/edgeflags/pad/vertex_id,
   * clip[] and whatever else in the header untouched.
   */
            if (instance_id_index != ~0) {
                                    for (unsigned i = 0; i < nr_inputs; i++) {
      if (i == instance_id_index) {
      key.element[nr].type = TRANSLATE_ELEMENT_INSTANCE_ID;
   key.element[nr].input_format = PIPE_FORMAT_R32_USCALED;
                     } else if (util_format_is_pure_sint(draw->pt.vertex_element[i].src_format)) {
      key.element[nr].type = TRANSLATE_ELEMENT_NORMAL;
   key.element[nr].input_format = draw->pt.vertex_element[ei].src_format;
   key.element[nr].input_buffer = draw->pt.vertex_element[ei].vertex_buffer_index;
   key.element[nr].input_offset = draw->pt.vertex_element[ei].src_offset;
   key.element[nr].instance_divisor = draw->pt.vertex_element[ei].instance_divisor;
                  ei++;
      } else if (util_format_is_pure_uint(draw->pt.vertex_element[i].src_format)) {
      key.element[nr].type = TRANSLATE_ELEMENT_NORMAL;
   key.element[nr].input_format = draw->pt.vertex_element[ei].src_format;
   key.element[nr].input_buffer = draw->pt.vertex_element[ei].vertex_buffer_index;
   key.element[nr].input_offset = draw->pt.vertex_element[ei].src_offset;
   key.element[nr].instance_divisor = draw->pt.vertex_element[ei].instance_divisor;
                  ei++;
      } else {
      key.element[nr].type = TRANSLATE_ELEMENT_NORMAL;
   key.element[nr].input_format = draw->pt.vertex_element[ei].src_format;
   key.element[nr].input_buffer = draw->pt.vertex_element[ei].vertex_buffer_index;
   key.element[nr].input_offset = draw->pt.vertex_element[ei].src_offset;
   key.element[nr].instance_divisor = draw->pt.vertex_element[ei].instance_divisor;
                  ei++;
                                    key.nr_elements = nr;
            if (!fetch->translate ||
      translate_key_compare(&fetch->translate->key, &key) != 0) {
   translate_key_sanitize(&key);
         }
      void
   draw_pt_fetch_run(struct pt_fetch *fetch,
                     {
      struct draw_context *draw = fetch->draw;
            for (unsigned i = 0; i < draw->pt.nr_vertex_buffers; i++) {
      translate->set_buffer(translate,
                        i,
            translate->run_elts(translate,
                     elts,
      }
         void
   draw_pt_fetch_run_linear(struct pt_fetch *fetch,
                     {
      struct draw_context *draw = fetch->draw;
            for (unsigned i = 0; i < draw->pt.nr_vertex_buffers; i++) {
      translate->set_buffer(translate,
                        i,
            translate->run(translate,
                  start,
   count,
   }
         struct pt_fetch *
   draw_pt_fetch_create(struct draw_context *draw)
   {
      struct pt_fetch *fetch = CALLOC_STRUCT(pt_fetch);
   if (!fetch)
            fetch->draw = draw;
   fetch->cache = translate_cache_create();
   if (!fetch->cache) {
      FREE(fetch);
                  }
         void
   draw_pt_fetch_destroy(struct pt_fetch *fetch)
   {
      if (fetch->cache)
               }
