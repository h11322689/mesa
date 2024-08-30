   /**************************************************************************
   *
   * Copyright 2009 VMware, Inc.
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
      #include "draw_gs.h"
      #include "draw_private.h"
   #include "draw_context.h"
   #ifdef DRAW_LLVM_AVAILABLE
   #include "draw_llvm.h"
   #endif
      #include "tgsi/tgsi_parse.h"
   #include "tgsi/tgsi_exec.h"
   #include "nir/nir_to_tgsi_info.h"
   #include "compiler/nir/nir.h"
   #include "pipe/p_shader_tokens.h"
      #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/u_prim.h"
   #include "util/ralloc.h"
   /* fixme: move it from here */
   #define MAX_PRIMITIVES 64
         static inline int
   draw_gs_get_input_index(int semantic, int index,
         {
      const uint8_t *input_semantic_names = input_info->output_semantic_name;
   const uint8_t *input_semantic_indices = input_info->output_semantic_index;
   for (int i = 0; i < PIPE_MAX_SHADER_OUTPUTS; i++) {
      if (input_semantic_names[i] == semantic &&
      input_semantic_indices[i] == index)
   }
      }
         /**
   * We execute geometry shaders in the SOA mode, so ideally we want to
   * flush when the number of currently fetched primitives is equal to
   * the number of elements in the SOA vector. This ensures that the
   * throughput is optimized for the given vector instruction set.
   */
   static inline bool
   draw_gs_should_flush(struct draw_geometry_shader *shader)
   {
         }
         /*#define DEBUG_OUTPUTS 1*/
   static void
   tgsi_fetch_gs_outputs(struct draw_geometry_shader *shader,
                     {
      struct tgsi_exec_machine *machine = shader->machine;
                     /* Unswizzle all output results.
            for (unsigned prim_idx = 0; prim_idx < num_primitives; ++prim_idx) {
      unsigned num_verts_per_prim = machine->Primitives[stream][prim_idx];
   unsigned prim_offset = machine->PrimitiveOffsets[stream][prim_idx];
   shader->stream[stream].primitive_lengths[prim_idx + shader->stream[stream].emitted_primitives] =
                  for (unsigned j = 0; j < num_verts_per_prim; j++) {
   #ifdef DEBUG_OUTPUTS
         #endif
            for (unsigned slot = 0; slot < shader->info.num_outputs; slot++) {
      output[slot][0] = machine->Outputs[idx + slot].xyzw[0].f[0];
   output[slot][1] = machine->Outputs[idx + slot].xyzw[1].f[0];
   #ifdef DEBUG_OUTPUTS
               debug_printf("\t%d: %f %f %f %f\n", slot,
               #endif
            }
         }
   *p_output = output;
      }
         #define DEBUG_INPUTS 0
   static void
   tgsi_fetch_gs_input(struct draw_geometry_shader *shader,
                     {
      struct tgsi_exec_machine *machine = shader->machine;
            int primid_sv = machine->SysSemanticToIndex[TGSI_SEMANTIC_PRIMID];
   if (primid_sv != -1) {
      for (unsigned j = 0; j < TGSI_QUAD_SIZE; j++)
                           #if DEBUG_INPUTS
         debug_printf("%d) vertex index = %d (prim idx = %d)\n",
   #endif
         const float (*input)[4] = (const float (*)[4])
         for (unsigned slot = 0; slot < shader->info.num_inputs; ++slot) {
      unsigned idx = i * TGSI_EXEC_MAX_INPUT_ATTRIBS + slot;
   if (shader->info.input_semantic_name[slot] == TGSI_SEMANTIC_PRIMID) {
      machine->Inputs[idx].xyzw[0].u[prim_idx] = shader->in_prim_idx;
   machine->Inputs[idx].xyzw[1].u[prim_idx] = shader->in_prim_idx;
   machine->Inputs[idx].xyzw[2].u[prim_idx] = shader->in_prim_idx;
      } else {
      /* TODO: Move this call out of the for(i) loop */
   int vs_slot = draw_gs_get_input_index(
      shader->info.input_semantic_name[slot],
   shader->info.input_semantic_index[slot],
      if (vs_slot < 0) {
      debug_printf("VS/GS signature mismatch!\n");
   machine->Inputs[idx].xyzw[0].f[prim_idx] = 0;
   machine->Inputs[idx].xyzw[1].f[prim_idx] = 0;
      #if DEBUG_INPUTS
                  debug_printf("\tSlot = %d, vs_slot = %d, idx = %d:\n",
         assert(!util_is_inf_or_nan(input[vs_slot][0]));
      #endif
                  machine->Inputs[idx].xyzw[0].f[prim_idx] = input[vs_slot][0];
      #if DEBUG_INPUTS
                  debug_printf("\t\t%f %f %f %f\n",
            #endif
                           }
         static void
   tgsi_gs_prepare(struct draw_geometry_shader *shader,
         {
      struct tgsi_exec_machine *machine = shader->machine;
   tgsi_exec_set_constant_buffers(machine, PIPE_MAX_CONSTANT_BUFFERS,
      }
         static void
   tgsi_gs_run(struct draw_geometry_shader *shader,
               {
               if (shader->info.uses_invocationid) {
      unsigned i = machine->SysSemanticToIndex[TGSI_SEMANTIC_INVOCATIONID];
   for (int j = 0; j < TGSI_QUAD_SIZE; j++)
               /* run interpreter */
            for (int i = 0; i < 4; i++)
      }
         #ifdef DRAW_LLVM_AVAILABLE
      /*
   * Fetch the vertex attribute values for one primitive.
   * num_vertices is vertices/prim (1 for points, 2 for lines, 3 for tris)
   */
   static void
   llvm_fetch_gs_input(struct draw_geometry_shader *shader,
                     {
      const unsigned input_vertex_stride = shader->input_vertex_stride;
                                 #if DEBUG_INPUTS
         debug_printf("%d) vertex index = %d (prim idx = %d)\n",
   #endif
         const float (*input)[4] = (const float (*)[4])
         for (unsigned slot = 0; slot < shader->info.num_inputs; ++slot) {
      if (shader->info.input_semantic_name[slot] == TGSI_SEMANTIC_PRIMID) {
      /* skip. we handle system values through gallivm */
   /* NOTE: If we hit this case here it's an ordinary input not a sv,
   * even though it probably should be a sv.
   * Not sure how to set it up as regular input however if that even,
   * would make sense so hack around this later in gallivm.
      } else {
      int vs_slot = draw_gs_get_input_index(
      shader->info.input_semantic_name[slot],
   shader->info.input_semantic_index[slot],
      if (vs_slot < 0) {
      debug_printf("VS/GS signature mismatch!\n");
   (*input_data)[i][slot][0][prim_idx] = 0;
   (*input_data)[i][slot][1][prim_idx] = 0;
      #if DEBUG_INPUTS
                  debug_printf("\tSlot = %d, vs_slot = %d, i = %d:\n",
         assert(!util_is_inf_or_nan(input[vs_slot][0]));
      #endif
                  (*input_data)[i][slot][0][prim_idx] = input[vs_slot][0];
      #if DEBUG_INPUTS
                  debug_printf("\t\t%f %f %f %f\n",
            #endif
                           }
         static void
   llvm_fetch_gs_outputs(struct draw_geometry_shader *shader,
                     {
      int total_verts = 0;
   int vertex_count = 0;
   int total_prims = 0;
   int max_prims_per_invocation = 0;
   char *output_ptr = (char*)shader->gs_output[stream];
   int prim_idx;
            for (int i = 0; i < shader->vector_length; ++i) {
      int prims = shader->llvm_emitted_primitives[i + (stream * shader->vector_length)];
   total_prims += prims;
      }
   for (int i = 0; i < shader->vector_length; ++i) {
                  output_ptr += shader->stream[stream].emitted_vertices * shader->vertex_size;
   for (int i = 0; i < shader->vector_length - 1; ++i) {
      int current_verts = shader->llvm_emitted_vertices[i + (stream * shader->vector_length)];
   #if 0
         int j;
   for (j = 0; j < current_verts; ++j) {
      struct vertex_header *vh = (struct vertex_header *)
                     #endif
         assert(current_verts <= shader->max_output_vertices);
   assert(next_verts <= shader->max_output_vertices);
   if (next_verts) {
      memmove(output_ptr + (vertex_count + current_verts) * shader->vertex_size,
            }
            #if 0
      {
      for (int i = 0; i < total_verts; ++i) {
      struct vertex_header *vh = (struct vertex_header *)(output_ptr + shader->vertex_size * i);
   debug_printf("%d) Vertex:\n", i);
   for (j = 0; j < shader->info.num_outputs; ++j) {
      unsigned *udata = (unsigned*)vh->data[j];
   debug_printf("    %d) [%f, %f, %f, %f] [%d, %d, %d, %d]\n", j,
                        #endif
         prim_idx = 0;
   for (int i = 0; i < shader->vector_length; ++i) {
      int num_prims = shader->llvm_emitted_primitives[i + (stream * shader->vector_length)];
   for (int j = 0; j < num_prims; ++j) {
      int prim_length =
         shader->stream[stream].primitive_lengths[shader->stream[stream].emitted_primitives + prim_idx] =
                        shader->stream[stream].emitted_primitives += total_prims;
      }
         static void
   llvm_gs_prepare(struct draw_geometry_shader *shader,
         {
   }
         static void
   llvm_gs_run(struct draw_geometry_shader *shader,
         {
      struct vertex_header *input[PIPE_MAX_VERTEX_STREAMS];
   for (unsigned i = 0; i < shader->num_vertex_streams; i++) {
      char *tmp = (char *)shader->gs_output[i];
   tmp += shader->stream[i].emitted_vertices * shader->vertex_size;
               shader->current_variant->jit_func(shader->jit_context,
                                    shader->jit_resources,
   shader->gs_input->data,
         for (unsigned i = 0; i < shader->num_vertex_streams; i++) {
            }
      #endif
         static void
   gs_flush(struct draw_geometry_shader *shader)
   {
      unsigned out_prim_count[TGSI_MAX_VERTEX_STREAMS];
   unsigned i;
            if (shader->draw->collect_statistics) {
                  assert(input_primitives > 0 &&
            for (unsigned invocation = 0; invocation < shader->num_invocations; invocation++) {
      shader->invocation_id = invocation;
   shader->run(shader, input_primitives, out_prim_count);
   for (i = 0; i < shader->num_vertex_streams; i++) {
      shader->fetch_outputs(shader, i, out_prim_count[i],
               #if 0
      for (i = 0; i < shader->num_vertex_streams; i++) {
      debug_printf("stream %d: PRIM emitted prims = %d (verts=%d), cur prim count = %d\n",
                     #endif
            }
         static void
   gs_point(struct draw_geometry_shader *shader, int idx)
   {
                        shader->fetch_inputs(shader, indices, 1,
         ++shader->in_prim_idx;
            if (draw_gs_should_flush(shader))
      }
         static void
   gs_line(struct draw_geometry_shader *shader, int i0, int i1)
   {
               indices[0] = i0;
            shader->fetch_inputs(shader, indices, 2,
         ++shader->in_prim_idx;
            if (draw_gs_should_flush(shader))
      }
         static void
   gs_line_adj(struct draw_geometry_shader *shader,
         {
               indices[0] = i0;
   indices[1] = i1;
   indices[2] = i2;
            shader->fetch_inputs(shader, indices, 4,
         ++shader->in_prim_idx;
            if (draw_gs_should_flush(shader))
      }
         static void
   gs_tri(struct draw_geometry_shader *shader,
         {
               indices[0] = i0;
   indices[1] = i1;
            shader->fetch_inputs(shader, indices, 3,
         ++shader->in_prim_idx;
            if (draw_gs_should_flush(shader))
      }
         static void
   gs_tri_adj(struct draw_geometry_shader *shader,
               {
               indices[0] = i0;
   indices[1] = i1;
   indices[2] = i2;
   indices[3] = i3;
   indices[4] = i4;
            shader->fetch_inputs(shader, indices, 6,
         ++shader->in_prim_idx;
            if (draw_gs_should_flush(shader))
      }
      #define FUNC         gs_run
   #define GET_ELT(idx) (idx)
   #include "draw_gs_tmp.h"
         #define FUNC         gs_run_elts
   #define LOCAL_VARS   const uint16_t *elts = input_prims->elts;
   #define GET_ELT(idx) (elts[idx])
   #include "draw_gs_tmp.h"
         /**
   * Execute geometry shader.
   */
   void
   draw_geometry_shader_run(struct draw_geometry_shader *shader,
                           const struct draw_buffer_info *constants,
   const struct draw_vertex_info *input_verts,
   {
      const float (*input)[4] = (const float (*)[4])input_verts->verts->data;
   const unsigned input_stride = input_verts->vertex_size;
   const unsigned num_outputs = draw_total_gs_outputs(shader->draw);
   const unsigned vertex_size =
         const unsigned num_input_verts =
         const unsigned num_in_primitives =
      align(MAX2(u_decomposed_prims_for_vertices(input_prim->prim,
                  u_decomposed_prims_for_vertices(shader->input_primitive,
   //Assume at least one primitive
   const unsigned max_out_prims =
      MAX2(1, u_decomposed_prims_for_vertices(shader->output_primitive,
               /* we allocate exactly one extra vertex per primitive to allow the GS to
   * emit overflown vertices into some area where they won't harm anyone */
   const unsigned total_verts_per_buffer =
               for (int i = 0; i < shader->num_vertex_streams; i++) {
      /* write all the vertex data into all the streams */
   output_verts[i].vertex_size = vertex_size;
   output_verts[i].stride = output_verts[i].vertex_size;
   output_verts[i].verts =
      (struct vertex_header *) MALLOC(output_verts[i].vertex_size *
                           #if 0
      debug_printf("%s count = %d (in prims # = %d, invocs = %d, streams = %d)\n",
               debug_printf("\tlinear = %d, prim_info->count = %d\n",
         debug_printf("\tprim pipe = %s, shader in = %s, shader out = %s\n",
               u_prim_name(input_prim->prim),
   debug_printf("\tmaxv  = %d, maxp = %d, primitive_boundary = %d, "
               "vertex_size = %d, tverts = %d\n",
      #endif
         for (int i = 0; i < shader->num_vertex_streams; i++) {
      shader->stream[i].emitted_vertices = 0;
   shader->stream[i].emitted_primitives = 0;
   FREE(shader->stream[i].primitive_lengths);
   shader->stream[i].primitive_lengths =
            }
   shader->vertex_size = vertex_size;
   shader->fetched_prim_count = 0;
   shader->input_vertex_stride = input_stride;
   shader->input = input;
         #ifdef DRAW_LLVM_AVAILABLE
      if (shader->draw->llvm) {
      for (int i = 0; i < shader->num_vertex_streams; i++) {
         }
   if (max_out_prims > shader->max_out_prims) {
      if (shader->llvm_prim_lengths) {
      for (unsigned i = 0; i < shader->num_vertex_streams * shader->max_out_prims; ++i) {
         }
               shader->llvm_prim_lengths = MALLOC(shader->num_vertex_streams * max_out_prims * sizeof(unsigned*));
   for (unsigned i = 0; i < shader->num_vertex_streams * max_out_prims; ++i) {
      int vector_size = shader->vector_length * sizeof(unsigned);
   shader->llvm_prim_lengths[i] =
                  }
   shader->jit_context->prim_lengths = shader->llvm_prim_lengths;
   shader->jit_context->emitted_vertices = shader->llvm_emitted_vertices;
         #endif
                  if (input_prim->linear)
      gs_run(shader, input_prim, input_verts,
      else
      gs_run_elts(shader, input_prim, input_verts,
         /* Flush the remaining primitives. Will happen if
   * num_input_primitives % 4 != 0
   */
   if (shader->fetched_prim_count > 0) {
         }
            /* Update prim_info:
   */
   for (int i = 0; i < shader->num_vertex_streams; i++) {
      output_prims[i].linear = true;
   output_prims[i].elts = NULL;
   output_prims[i].start = 0;
   output_prims[i].count = shader->stream[i].emitted_vertices;
   output_prims[i].prim = shader->output_primitive;
   output_prims[i].flags = 0x0;
   output_prims[i].primitive_lengths = shader->stream[i].primitive_lengths;
   output_prims[i].primitive_count = shader->stream[i].emitted_primitives;
            if (shader->draw->collect_statistics) {
      for (unsigned j = 0; j < shader->stream[i].emitted_primitives; ++j) {
      shader->draw->statistics.gs_primitives +=
      u_decomposed_prims_for_vertices(shader->output_primitive,
               #if 0
      debug_printf("GS finished\n");
   for (int i = 0; i < 4; i++)
      debug_printf("stream %d: prims = %d verts = %d\n", i,
   #endif
   }
         void
   draw_geometry_shader_prepare(struct draw_geometry_shader *shader,
         {
      bool use_llvm = draw->llvm != NULL;
   if (!use_llvm &&
      shader && shader->machine->Tokens != shader->state.tokens) {
   tgsi_exec_machine_bind_shader(shader->machine,
                           }
         bool
   draw_gs_init(struct draw_context *draw)
   {
      if (!draw->llvm) {
               for (unsigned i = 0; i < TGSI_MAX_VERTEX_STREAMS; i++) {
      draw->gs.tgsi.machine->Primitives[i] = align_malloc(
         draw->gs.tgsi.machine->PrimitiveOffsets[i] = align_malloc(
         if (!draw->gs.tgsi.machine->Primitives[i] ||
      !draw->gs.tgsi.machine->PrimitiveOffsets[i])
      memset(draw->gs.tgsi.machine->Primitives[i], 0,
         memset(draw->gs.tgsi.machine->PrimitiveOffsets[i], 0,
                     }
         void
   draw_gs_destroy(struct draw_context *draw)
   {
      if (draw->gs.tgsi.machine) {
      for (int i = 0; i < TGSI_MAX_VERTEX_STREAMS; i++) {
      align_free(draw->gs.tgsi.machine->Primitives[i]);
      }
         }
         /*
   * num_vertices is vertices/prim (1 for points, 2 for lines, 3 for tris)
   */
   struct draw_geometry_shader *
   draw_create_geometry_shader(struct draw_context *draw,
         {
   #ifdef DRAW_LLVM_AVAILABLE
      bool use_llvm = draw->llvm != NULL;
      #endif
            #ifdef DRAW_LLVM_AVAILABLE
      if (use_llvm) {
               if (!llvm_gs)
                           #endif
      {
                  if (!gs)
            gs->draw = draw;
            if (state->type == PIPE_SHADER_IR_TGSI) {
      gs->state.tokens = tgsi_dup_tokens(state->tokens);
   if (!gs->state.tokens) {
      FREE(gs);
               tgsi_scan_shader(state->tokens, &gs->info);
   gs->num_vertex_streams = 1;
   for (unsigned i = 0; i < gs->state.stream_output.num_outputs; i++) {
      if (gs->state.stream_output.output[i].stream >= gs->num_vertex_streams)
         } else {
      nir_tgsi_scan_shader(state->ir.nir, &gs->info, true);
   nir_shader *nir = state->ir.nir;
               /* setup the defaults */
         #ifdef DRAW_LLVM_AVAILABLE
      if (use_llvm) {
      /* TODO: change the input array to handle the following
      vector length, instead of the currently hardcoded
      gs->vector_length = lp_native_vector_width / 32;*/
         #endif
      {
                  gs->input_primitive =
         gs->output_primitive =
         gs->max_output_vertices =
         gs->num_invocations =
         if (!gs->max_output_vertices)
            /* Primitive boundary is bigger than max_output_vertices by one, because
   * the specification says that the geometry shader should exit if the
   * number of emitted vertices is bigger or equal to max_output_vertices and
   * we can't do that because we're running in the SoA mode, which means that
   * our storing routines will keep getting called on channels that have
   * overflown.
   * So we need some scratch area where we can keep writing the overflown
   * vertices without overwriting anything important or crashing.
   */
            gs->position_output = -1;
   bool found_clipvertex = false;
   for (unsigned i = 0; i < gs->info.num_outputs; i++) {
      if (gs->info.output_semantic_name[i] == TGSI_SEMANTIC_POSITION &&
      gs->info.output_semantic_index[i] == 0)
      if (gs->info.output_semantic_name[i] == TGSI_SEMANTIC_VIEWPORT_INDEX)
         if (gs->info.output_semantic_name[i] == TGSI_SEMANTIC_CLIPVERTEX &&
      gs->info.output_semantic_index[i] == 0) {
   found_clipvertex = true;
      }
   if (gs->info.output_semantic_name[i] == TGSI_SEMANTIC_CLIPDIST) {
      assert(gs->info.output_semantic_index[i] <
                        if (!found_clipvertex)
                  #ifdef DRAW_LLVM_AVAILABLE
      if (use_llvm) {
      int vector_size = gs->vector_length * sizeof(float);
   gs->gs_input = align_malloc(sizeof(struct draw_gs_inputs), 16);
   memset(gs->gs_input, 0, sizeof(struct draw_gs_inputs));
            gs->llvm_emitted_primitives = align_malloc(vector_size * gs->num_vertex_streams, vector_size);
   gs->llvm_emitted_vertices = align_malloc(vector_size * gs->num_vertex_streams, vector_size);
            gs->fetch_outputs = llvm_fetch_gs_outputs;
   gs->fetch_inputs = llvm_fetch_gs_input;
   gs->prepare = llvm_gs_prepare;
            gs->jit_context = &draw->llvm->gs_jit_context;
            llvm_gs->variant_key_size =
      draw_gs_llvm_variant_key_size(
      gs->info.file_max[TGSI_FILE_SAMPLER]+1,
         #endif
      {
      gs->fetch_outputs = tgsi_fetch_gs_outputs;
   gs->fetch_inputs = tgsi_fetch_gs_input;
   gs->prepare = tgsi_gs_prepare;
                  }
         void
   draw_bind_geometry_shader(struct draw_context *draw,
         {
               if (dgs) {
      draw->gs.geometry_shader = dgs;
   draw->gs.num_gs_outputs = dgs->info.num_outputs;
   draw->gs.position_output = dgs->position_output;
   draw->gs.clipvertex_output = dgs->clipvertex_output;
      } else {
      draw->gs.geometry_shader = NULL;
         }
         void
   draw_delete_geometry_shader(struct draw_context *draw,
         {
      if (!dgs) {
            #ifdef DRAW_LLVM_AVAILABLE
      if (draw->llvm) {
      struct llvm_geometry_shader *shader = llvm_geometry_shader(dgs);
            LIST_FOR_EACH_ENTRY_SAFE(li, next, &shader->variants.list, list) {
                           if (dgs->llvm_prim_lengths) {
      for (unsigned i = 0; i < dgs->num_vertex_streams * dgs->max_out_prims; ++i) {
         }
      }
   align_free(dgs->llvm_emitted_primitives);
   align_free(dgs->llvm_emitted_vertices);
                  #endif
         if (draw->gs.tgsi.machine && draw->gs.tgsi.machine->Tokens == dgs->state.tokens)
            for (unsigned i = 0; i < TGSI_MAX_VERTEX_STREAMS; i++)
            if (dgs->state.type == PIPE_SHADER_IR_NIR && dgs->state.ir.nir)
         FREE((void*) dgs->state.tokens);
      }
         #ifdef DRAW_LLVM_AVAILABLE
   void
   draw_gs_set_current_variant(struct draw_geometry_shader *shader,
         {
         }
   #endif
      /*
   * Called at the very begin of the draw call with a new instance
   * Used to reset state that should persist between primitive restart.
   */
   void
   draw_geometry_shader_new_instance(struct draw_geometry_shader *gs)
   {
      if (!gs)
               }
