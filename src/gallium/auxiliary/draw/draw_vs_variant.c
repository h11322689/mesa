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
   #include "util/u_math.h"
   #include "draw/draw_context.h"
   #include "draw/draw_private.h"
   #include "draw/draw_vbuf.h"
   #include "draw/draw_vertex.h"
   #include "draw/draw_vs.h"
   #include "translate/translate.h"
         /* A first pass at incorporating vertex fetch/emit functionality into
   */
   struct draw_vs_variant_generic {
               struct draw_vertex_shader *shader;
            /* Basic plan is to run these two translate functions before/after
   * the vertex shader's existing run_linear() routine to simulate
   * the inclusion of this functionality into the shader...
   *
   * Next will look at actually including it.
   */
   struct translate *fetch;
               };
            static void
   vsvg_set_buffer(struct draw_vs_variant *variant,
                  unsigned buffer,
      {
                  }
         static const struct pipe_viewport_state *
   find_viewport(struct draw_context *draw,
               char *buffer,
   {
      int viewport_index_output =
         const char *ptr = buffer + vertex_idx * stride;
   const unsigned *data = (const unsigned *) ptr;
   int viewport_index =
      draw_current_shader_uses_viewport_index(draw) ?
                     }
         /* Mainly for debug at this stage:
   */
   static void
   do_rhw_viewport(struct draw_vs_variant_generic *vsvg,
               {
      char *ptr = (char *)output_buffer;
                     for (unsigned j = 0; j < count; j++, ptr += stride) {
      const struct pipe_viewport_state *viewport =
      find_viewport(vsvg->base.vs->draw, (char*)output_buffer,
      const float *scale = viewport->scale;
   const float *trans = viewport->translate;
   float *data = (float *)ptr;
            data[0] = data[0] * w * scale[0] + trans[0];
   data[1] = data[1] * w * scale[1] + trans[1];
   data[2] = data[2] * w * scale[2] + trans[2];
         }
         static void
   do_viewport(struct draw_vs_variant_generic *vsvg,
               {
      char *ptr = (char *)output_buffer;
                     for (unsigned j = 0; j < count; j++, ptr += stride) {
      const struct pipe_viewport_state *viewport =
      find_viewport(vsvg->base.vs->draw, (char*)output_buffer,
      const float *scale = viewport->scale;
   const float *trans = viewport->translate;
            data[0] = data[0] * scale[0] + trans[0];
   data[1] = data[1] * scale[1] + trans[1];
         }
         static void UTIL_CDECL
   vsvg_run_elts(struct draw_vs_variant *variant,
               const unsigned *elts,
   {
      struct draw_vs_variant_generic *vsvg = (struct draw_vs_variant_generic *)variant;
   unsigned temp_vertex_stride = vsvg->temp_vertex_stride;
   void *temp_buffer = MALLOC(align(count,4) * temp_vertex_stride +
                     /* Want to do this in small batches for cache locality?
            vsvg->fetch->run_elts(vsvg->fetch,
                        elts,
         vsvg->base.vs->run_linear(vsvg->base.vs,
                           temp_buffer,
                     if (vsvg->base.key.clip) {
      /* not really handling clipping, just do the rhw so we can
   * see the results...
   */
      } else if (vsvg->base.key.viewport) {
                  vsvg->emit->set_buffer(vsvg->emit,
                              vsvg->emit->set_buffer(vsvg->emit,
                              vsvg->emit->run(vsvg->emit,
                  0, count,
            }
         static void UTIL_CDECL
   vsvg_run_linear(struct draw_vs_variant *variant,
                     {
      struct draw_vs_variant_generic *vsvg = (struct draw_vs_variant_generic *)variant;
   unsigned temp_vertex_stride = vsvg->temp_vertex_stride;
   void *temp_buffer = MALLOC(align(count,4) * temp_vertex_stride +
            if (0) debug_printf("%s %d %d (sz %d, %d)\n", __func__, start, count,
                  vsvg->fetch->run(vsvg->fetch,
                  start,
   count,
         vsvg->base.vs->run_linear(vsvg->base.vs,
                           temp_buffer,
            if (vsvg->base.key.clip) {
      /* not really handling clipping, just do the rhw so we can
   * see the results...
   */
      } else if (vsvg->base.key.viewport) {
                  vsvg->emit->set_buffer(vsvg->emit,
                              vsvg->emit->set_buffer(vsvg->emit,
                              vsvg->emit->run(vsvg->emit,
                  0, count,
            }
         static void
   vsvg_destroy(struct draw_vs_variant *variant)
   {
         }
         struct draw_vs_variant *
   draw_vs_create_variant_generic(struct draw_vertex_shader *vs,
         {
      struct draw_vs_variant_generic *vsvg = CALLOC_STRUCT(draw_vs_variant_generic);
   if (!vsvg)
            vsvg->base.key = *key;
   vsvg->base.vs = vs;
   vsvg->base.set_buffer    = vsvg_set_buffer;
   vsvg->base.run_elts      = vsvg_run_elts;
   vsvg->base.run_linear    = vsvg_run_linear;
                     vsvg->temp_vertex_stride = MAX2(key->nr_inputs,
            /* Build free-standing fetch and emit functions:
   */
   struct translate_key fetch;
   fetch.nr_elements = key->nr_inputs;
   fetch.output_stride = vsvg->temp_vertex_stride;
   for (unsigned i = 0; i < key->nr_inputs; i++) {
      fetch.element[i].type = TRANSLATE_ELEMENT_NORMAL;
   fetch.element[i].input_format = key->element[i].in.format;
   fetch.element[i].input_buffer = key->element[i].in.buffer;
   fetch.element[i].input_offset = key->element[i].in.offset;
   fetch.element[i].instance_divisor = 0;
   fetch.element[i].output_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
   fetch.element[i].output_offset = i * 4 * sizeof(float);
               struct translate_key emit;
   emit.nr_elements = key->nr_outputs;
   emit.output_stride = key->output_stride;
   for (unsigned i = 0; i < key->nr_outputs; i++) {
      if (key->element[i].out.format != EMIT_1F_PSIZE) {
      emit.element[i].type = TRANSLATE_ELEMENT_NORMAL;
   emit.element[i].input_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
   emit.element[i].input_buffer = 0;
   emit.element[i].input_offset = key->element[i].out.vs_output * 4 * sizeof(float);
   emit.element[i].instance_divisor = 0;
   emit.element[i].output_format = draw_translate_vinfo_format(key->element[i].out.format);
   emit.element[i].output_offset = key->element[i].out.offset;
      } else {
      emit.element[i].type = TRANSLATE_ELEMENT_NORMAL;
   emit.element[i].input_format = PIPE_FORMAT_R32_FLOAT;
   emit.element[i].input_buffer = 1;
   emit.element[i].input_offset = 0;
   emit.element[i].instance_divisor = 0;
   emit.element[i].output_format = PIPE_FORMAT_R32_FLOAT;
                  vsvg->fetch = draw_vs_get_fetch(vs->draw, &fetch);
               }
