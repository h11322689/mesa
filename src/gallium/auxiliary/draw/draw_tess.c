   /**************************************************************************
   *
   * Copyright 2020 Red Hat.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included
   * in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   **************************************************************************/
   #include "draw_tess.h"
   #ifdef DRAW_LLVM_AVAILABLE
   #include "draw_llvm.h"
   #endif
      #include "tessellator/p_tessellator.h"
   #include "nir/nir_to_tgsi_info.h"
   #include "util/u_prim.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/ralloc.h"
   #ifdef DRAW_LLVM_AVAILABLE
   static inline int
   draw_tes_get_input_index(int semantic, int index,
         {
      int i;
   const uint8_t *input_semantic_names = input_info->output_semantic_name;
   const uint8_t *input_semantic_indices = input_info->output_semantic_index;
   for (i = 0; i < PIPE_MAX_SHADER_OUTPUTS; i++) {
      if (input_semantic_names[i] == semantic &&
      input_semantic_indices[i] == index)
   }
      }
      #define DEBUG_INPUTS 0
   static void
   llvm_fetch_tcs_input(struct draw_tess_ctrl_shader *shader,
                     {
      const float (*input_ptr)[4];
   float (*input_data)[32][NUM_TCS_INPUTS][TGSI_NUM_CHANNELS] = &shader->tcs_input->data;
   unsigned slot, i;
   int vs_slot;
            input_ptr = shader->input;
   for (i = 0; i < num_vertices; i++) {
      const float (*input)[4];
   int vertex_idx = prim_id * num_vertices + i;
   if (input_prim_info->linear == false)
   #if DEBUG_INPUTS
         debug_printf("%d) tcs vertex index = %d (prim idx = %d)\n",
   #endif
         input = (const float (*)[4])((const char *)input_ptr + (vertex_idx * input_vertex_stride));
   for (slot = 0, vs_slot = 0; slot < shader->info.num_inputs; ++slot) {
      vs_slot = draw_tes_get_input_index(
                     if (vs_slot < 0) {
      debug_printf("VS/TCS signature mismatch!\n");
   (*input_data)[i][slot][0] = 0;
   (*input_data)[i][slot][1] = 0;
   (*input_data)[i][slot][2] = 0;
      } else {
      (*input_data)[i][slot][0] = input[vs_slot][0];
   (*input_data)[i][slot][1] = input[vs_slot][1];
   #if DEBUG_INPUTS
               debug_printf("\t\t%p = %f %f %f %f\n", &(*input_data)[i][slot][0],
               #endif
                           }
      #define DEBUG_OUTPUTS 0
   static void
   llvm_store_tcs_output(struct draw_tess_ctrl_shader *shader,
                     {
      float (*output_ptr)[4];
   float (*output_data)[32][PIPE_MAX_SHADER_INPUTS][TGSI_NUM_CHANNELS] = &shader->tcs_output->data;
   unsigned slot, i;
            char *output = (char *)output_verts->verts->data;
                  #if DEBUG_OUTPUTS
         debug_printf("%d) tcs store vertex index = %d (prim idx = %d)\n",
   #endif
                  for (slot = 0; slot < shader->info.num_outputs; ++slot) {
      output_ptr[slot][0] = (*output_data)[i][slot][0];
   output_ptr[slot][1] = (*output_data)[i][slot][1];
      #if DEBUG_OUTPUTS
            debug_printf("\t\t%p = %f %f %f %f\n",
               &output_ptr[slot][0],
      #endif
               }
      static void
   llvm_tcs_run(struct draw_tess_ctrl_shader *shader, uint32_t prim_id)
   {
      shader->current_variant->jit_func(shader->jit_resources,
            }
   #endif
      /**
   * Execute tess ctrl shader.
   */
   int draw_tess_ctrl_shader_run(struct draw_tess_ctrl_shader *shader,
                                 {
      const float (*input)[4] = (const float (*)[4])input_verts->verts->data;
   unsigned num_outputs = draw_total_tcs_outputs(shader->draw);
   unsigned input_stride = input_verts->vertex_size;
   unsigned vertex_size = sizeof(struct vertex_header) + num_outputs * 4 * sizeof(float);
            output_verts->vertex_size = vertex_size;
   output_verts->stride = output_verts->vertex_size;
   output_verts->verts = NULL;
   output_verts->count = 0;
   shader->input = input;
   shader->input_vertex_stride = input_stride;
            output_prims->linear = true;
   output_prims->start = 0;
   output_prims->elts = NULL;
   output_prims->count = 0;
   output_prims->prim = MESA_PRIM_PATCHES;
   output_prims->flags = 0;
   output_prims->primitive_lengths = NULL;
            if (shader->draw->collect_statistics) {
            #ifdef DRAW_LLVM_AVAILABLE
      unsigned first_patch = input_prim->start / shader->draw->pt.vertices_per_patch;
   for (unsigned i = 0; i < num_patches; i++) {
                                          uint32_t old_verts = util_align_npot(vert_start, 16);
   uint32_t new_verts = util_align_npot(output_verts->count, 16);
   uint32_t old_size = output_verts->vertex_size * old_verts;
   uint32_t new_size = output_verts->vertex_size * new_verts;
                  #endif
         output_prims->primitive_count = num_patches;
      }
      #ifdef DRAW_LLVM_AVAILABLE
   #define DEBUG_INPUTS 0
   static void
   llvm_fetch_tes_input(struct draw_tess_eval_shader *shader,
                     {
      const float (*input_ptr)[4];
   float (*input_data)[32][PIPE_MAX_SHADER_INPUTS][TGSI_NUM_CHANNELS] = &shader->tes_input->data;
   unsigned slot, i;
   int vs_slot;
            input_ptr = shader->input;
   for (i = 0; i < num_vertices; i++) {
      const float (*input)[4];
            if (input_prim_info->linear == false)
   #if DEBUG_INPUTS
         debug_printf("%d) tes vertex index = %d (prim idx = %d)\n",
   #endif
         input = (const float (*)[4])((const char *)input_ptr + (vertex_idx * input_vertex_stride));
   for (slot = 0, vs_slot = 0; slot < shader->info.num_inputs; ++slot) {
      vs_slot = draw_tes_get_input_index(
                     if (vs_slot < 0) {
      debug_printf("TCS/TES signature mismatch!\n");
   (*input_data)[i][slot][0] = 0;
   (*input_data)[i][slot][1] = 0;
   (*input_data)[i][slot][2] = 0;
      } else {
      (*input_data)[i][slot][0] = input[vs_slot][0];
   (*input_data)[i][slot][1] = input[vs_slot][1];
   #if DEBUG_INPUTS
               debug_printf("\t\t%p = %f %f %f %f\n",
               &input[vs_slot][0],
   #endif
                           }
      static void
   llvm_fetch_tess_factors(struct draw_tess_eval_shader *shader,
                     {
      int outer_slot = draw_tes_get_input_index(
         int inner_slot = draw_tes_get_input_index(
         const float (*input_ptr)[4];
   const float (*input)[4];
   input_ptr = shader->input;
            if (outer_slot != -1) {
      for (unsigned i = 0; i < 4; i++)
      } else {
      for (unsigned i = 0; i < 4; i++)
      }
   if (inner_slot != -1) {
      for (unsigned i = 0; i < 2; i++)
      } else {
      for (unsigned i = 0; i < 2; i++)
         }
      static void
   llvm_tes_run(struct draw_tess_eval_shader *shader,
               uint32_t prim_id,
   uint32_t patch_vertices_in,
   struct pipe_tessellator_data *tess_data,
   {
      shader->current_variant->jit_func(shader->jit_resources,
                        }
   #endif
      /**
   * Execute tess eval shader.
   */
   int draw_tess_eval_shader_run(struct draw_tess_eval_shader *shader,
                                 unsigned num_input_vertices_per_patch,
   const struct draw_vertex_info *input_verts,
   {
      const float (*input)[4] = (const float (*)[4])input_verts->verts->data;
   unsigned num_outputs = draw_total_tes_outputs(shader->draw);
   unsigned input_stride = input_verts->vertex_size;
   unsigned vertex_size = sizeof(struct vertex_header) + num_outputs * 4 * sizeof(float);
   uint16_t *elts = NULL;
   output_verts->vertex_size = vertex_size;
   output_verts->stride = output_verts->vertex_size;
   output_verts->count = 0;
            output_prims->linear = false;
   output_prims->start = 0;
   output_prims->elts = NULL;
   output_prims->count = 0;
   output_prims->prim = get_tes_output_prim(shader);
   output_prims->flags = 0;
   output_prims->primitive_lengths = NULL;
            shader->input = input;
   shader->input_vertex_stride = input_stride;
         #ifdef DRAW_LLVM_AVAILABLE
      struct pipe_tessellation_factors factors;
   struct pipe_tessellator_data data = { 0 };
   struct pipe_tessellator *ptess = p_tess_init(shader->prim_mode,
                     for (unsigned i = 0; i < input_prim->primitive_count; i++) {
      uint32_t vert_start = output_verts->count;
   uint32_t prim_start = output_prims->primitive_count;
                     /* tessellate with the factors for this primitive */
            if (data.num_domain_points == 0)
            uint32_t old_verts = vert_start;
   uint32_t new_verts = vert_start + util_align_npot(data.num_domain_points, 4);
   uint32_t old_size = output_verts->vertex_size * old_verts;
   uint32_t new_size = output_verts->vertex_size * new_verts;
                     output_prims->count += data.num_indices;
   elts = REALLOC(elts, elt_start * sizeof(uint16_t),
            for (unsigned i = 0; i < data.num_indices; i++)
            llvm_fetch_tes_input(shader, input_prim, i, num_input_vertices_per_patch);
   /* run once per primitive? */
   char *output = (char *)output_verts->verts;
   output += vert_start * vertex_size;
            if (shader->draw->collect_statistics) {
                  uint32_t prim_len = u_prim_vertex_count(output_prims->prim)->min;
   output_prims->primitive_count += data.num_indices / prim_len;
   output_prims->primitive_lengths = REALLOC(output_prims->primitive_lengths, prim_start * sizeof(uint32_t),
         for (unsigned i = prim_start; i < output_prims->primitive_count; i++) {
            }
      #endif
         *elts_out = elts;
   output_prims->elts = elts;
      }
      struct draw_tess_ctrl_shader *
   draw_create_tess_ctrl_shader(struct draw_context *draw,
         {
   #ifdef DRAW_LLVM_AVAILABLE
      bool use_llvm = draw->llvm != NULL;
      #endif
            #ifdef DRAW_LLVM_AVAILABLE
      if (use_llvm) {
               if (!llvm_tcs)
                           #endif
      {
                  if (!tcs)
            tcs->draw = draw;
                     tcs->vector_length = 4;
      #ifdef DRAW_LLVM_AVAILABLE
                  tcs->tcs_input = align_malloc(sizeof(struct draw_tcs_inputs), 16);
            tcs->tcs_output = align_malloc(sizeof(struct draw_tcs_outputs), 16);
            tcs->jit_resources = &draw->llvm->jit_resources[PIPE_SHADER_TESS_CTRL];
   llvm_tcs->variant_key_size =
      draw_tcs_llvm_variant_key_size(
                  #endif
         }
      void draw_bind_tess_ctrl_shader(struct draw_context *draw,
         {
      draw_do_flush(draw, DRAW_FLUSH_STATE_CHANGE);
   if (dtcs) {
         } else {
            }
      void draw_delete_tess_ctrl_shader(struct draw_context *draw,
         {
      if (!dtcs)
         #ifdef DRAW_LLVM_AVAILABLE
      if (draw->llvm) {
                        LIST_FOR_EACH_ENTRY_SAFE(li, next, &shader->variants.list, list) {
                  assert(shader->variants_cached == 0);
   align_free(dtcs->tcs_input);
         #endif
         if (dtcs->state.type == PIPE_SHADER_IR_NIR && dtcs->state.ir.nir)
            }
      #ifdef DRAW_LLVM_AVAILABLE
   void draw_tcs_set_current_variant(struct draw_tess_ctrl_shader *shader,
         {
         }
   #endif
      struct draw_tess_eval_shader *
   draw_create_tess_eval_shader(struct draw_context *draw,
         {
   #ifdef DRAW_LLVM_AVAILABLE
      bool use_llvm = draw->llvm != NULL;
      #endif
            #ifdef DRAW_LLVM_AVAILABLE
      if (use_llvm) {
               if (!llvm_tes)
            tes = &llvm_tes->base;
         #endif
      {
                  if (!tes)
            tes->draw = draw;
                     tes->prim_mode = tes->info.properties[TGSI_PROPERTY_TES_PRIM_MODE];
   tes->spacing = tes->info.properties[TGSI_PROPERTY_TES_SPACING];
   tes->vertex_order_cw = tes->info.properties[TGSI_PROPERTY_TES_VERTEX_ORDER_CW];
                     tes->position_output = -1;
   bool found_clipvertex = false;
   for (unsigned i = 0; i < tes->info.num_outputs; i++) {
      if (tes->info.output_semantic_name[i] == TGSI_SEMANTIC_POSITION &&
      tes->info.output_semantic_index[i] == 0)
      if (tes->info.output_semantic_name[i] == TGSI_SEMANTIC_VIEWPORT_INDEX)
         if (tes->info.output_semantic_name[i] == TGSI_SEMANTIC_CLIPVERTEX &&
      tes->info.output_semantic_index[i] == 0) {
   found_clipvertex = true;
      }
   if (tes->info.output_semantic_name[i] == TGSI_SEMANTIC_CLIPDIST) {
      assert(tes->info.output_semantic_index[i] <
               }
   if (!found_clipvertex)
         #ifdef DRAW_LLVM_AVAILABLE
                  tes->tes_input = align_malloc(sizeof(struct draw_tes_inputs), 16);
            tes->jit_resources = &draw->llvm->jit_resources[PIPE_SHADER_TESS_EVAL];
   llvm_tes->variant_key_size =
      draw_tes_llvm_variant_key_size(
                  #endif
         }
      void draw_bind_tess_eval_shader(struct draw_context *draw,
         {
      draw_do_flush(draw, DRAW_FLUSH_STATE_CHANGE);
   if (dtes) {
      draw->tes.tess_eval_shader = dtes;
   draw->tes.num_tes_outputs = dtes->info.num_outputs;
   draw->tes.position_output = dtes->position_output;
      } else {
            }
      void draw_delete_tess_eval_shader(struct draw_context *draw,
         {
      if (!dtes)
         #ifdef DRAW_LLVM_AVAILABLE
      if (draw->llvm) {
      struct llvm_tess_eval_shader *shader = llvm_tess_eval_shader(dtes);
            LIST_FOR_EACH_ENTRY_SAFE(li, next, &shader->variants.list, list) {
                  assert(shader->variants_cached == 0);
         #endif
      if (dtes->state.type == PIPE_SHADER_IR_NIR && dtes->state.ir.nir)
            }
      #ifdef DRAW_LLVM_AVAILABLE
   void draw_tes_set_current_variant(struct draw_tess_eval_shader *shader,
         {
         }
   #endif
      enum mesa_prim get_tes_output_prim(struct draw_tess_eval_shader *shader)
   {
      if (shader->point_mode)
         else if (shader->prim_mode == MESA_PRIM_LINES)
         else
      }
