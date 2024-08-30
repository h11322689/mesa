   /**************************************************************************
   *
   * Copyright 2010 VMware, Inc.
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
      #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/u_prim.h"
   #include "draw/draw_context.h"
   #include "draw/draw_gs.h"
   #include "draw/draw_tess.h"
   #include "draw/draw_vbuf.h"
   #include "draw/draw_vertex.h"
   #include "draw/draw_pt.h"
   #include "draw/draw_prim_assembler.h"
   #include "draw/draw_vs.h"
   #include "draw/draw_llvm.h"
   #include "gallivm/lp_bld_init.h"
   #include "gallivm/lp_bld_debug.h"
         struct llvm_middle_end {
      struct draw_pt_middle_end base;
            struct pt_emit *emit;
   struct pt_so_emit *so_emit;
   struct pt_fetch *fetch;
               unsigned vertex_data_offset;
   unsigned vertex_size;
   enum mesa_prim input_prim;
            struct draw_llvm *llvm;
      };
         /** cast wrapper */
   static inline struct llvm_middle_end *
   llvm_middle_end(struct draw_pt_middle_end *middle)
   {
         }
         static void
   llvm_middle_end_prepare_gs(struct llvm_middle_end *fpme)
   {
      struct draw_context *draw = fpme->draw;
   struct draw_llvm *llvm = fpme->llvm;
   struct draw_geometry_shader *gs = draw->gs.geometry_shader;
   struct draw_gs_llvm_variant_list_item *li;
   struct llvm_geometry_shader *shader = llvm_geometry_shader(gs);
   char store[DRAW_GS_LLVM_MAX_VARIANT_KEY_SIZE];
            /* Search shader's list of variants for the key */
   struct draw_gs_llvm_variant *variant = NULL;
   LIST_FOR_EACH_ENTRY(li, &shader->variants.list, list) {
      if (memcmp(&li->base->key, key, shader->variant_key_size) == 0) {
      variant = li->base;
                  if (variant) {
      /* found the variant, move to head of global list (for LRU) */
      } else {
               /* First check if we've created too many variants.  If so, free
   * 3.125% of the LRU to avoid using too much memory.
   */
   if (llvm->nr_gs_variants >= DRAW_MAX_SHADER_VARIANTS) {
      if (gallivm_debug & GALLIVM_DEBUG_PERF) {
      debug_printf("Evicting GS: %u gs variants,\t%u total variants\n",
               /*
   * XXX: should we flush here ?
   */
   struct draw_gs_llvm_variant_list_item *item;
   for (unsigned i = 0; i < DRAW_MAX_SHADER_VARIANTS / 32; i++) {
      if (list_is_empty(&llvm->gs_variants_list.list)) {
         }
   item = list_last_entry(&llvm->gs_variants_list.list,
         assert(item);
   assert(item->base);
                           if (variant) {
      list_add(&variant->list_item_local.list, &shader->variants.list);
   list_add(&variant->list_item_global.list, &llvm->gs_variants_list.list);
   llvm->nr_gs_variants++;
                     }
         static void
   llvm_middle_end_prepare_tcs(struct llvm_middle_end *fpme)
   {
      struct draw_context *draw = fpme->draw;
   struct draw_llvm *llvm = fpme->llvm;
   struct draw_tess_ctrl_shader *tcs = draw->tcs.tess_ctrl_shader;
   struct draw_tcs_llvm_variant_list_item *li;
   struct llvm_tess_ctrl_shader *shader = llvm_tess_ctrl_shader(tcs);
   char store[DRAW_TCS_LLVM_MAX_VARIANT_KEY_SIZE];
   const struct draw_tcs_llvm_variant_key *key =
            /* Search shader's list of variants for the key */
   struct draw_tcs_llvm_variant *variant = NULL;
   LIST_FOR_EACH_ENTRY(li, &shader->variants.list, list) {
      if (memcmp(&li->base->key, key, shader->variant_key_size) == 0) {
      variant = li->base;
                  if (variant) {
      /* found the variant, move to head of global list (for LRU) */
   list_move_to(&variant->list_item_global.list,
      } else {
               /* First check if we've created too many variants.  If so, free
   * 3.125% of the LRU to avoid using too much memory.
   */
   if (llvm->nr_tcs_variants >= DRAW_MAX_SHADER_VARIANTS) {
      if (gallivm_debug & GALLIVM_DEBUG_PERF) {
      debug_printf("Evicting TCS: %u tcs variants,\t%u total variants\n",
               /*
   * XXX: should we flush here ?
   */
   for (unsigned i = 0; i < DRAW_MAX_SHADER_VARIANTS / 32; i++) {
      struct draw_tcs_llvm_variant_list_item *item;
   if (list_is_empty(&llvm->tcs_variants_list.list)) {
         }
   item = list_last_entry(&llvm->tcs_variants_list.list,
         assert(item);
   assert(item->base);
                           if (variant) {
      list_add(&variant->list_item_local.list, &shader->variants.list);
   list_add(&variant->list_item_global.list, &llvm->tcs_variants_list.list);
   llvm->nr_tcs_variants++;
                     }
         static void
   llvm_middle_end_prepare_tes(struct llvm_middle_end *fpme)
   {
      struct draw_context *draw = fpme->draw;
   struct draw_llvm *llvm = fpme->llvm;
   struct draw_tess_eval_shader *tes = draw->tes.tess_eval_shader;
   struct draw_tes_llvm_variant *variant = NULL;
   struct draw_tes_llvm_variant_list_item *li;
   struct llvm_tess_eval_shader *shader = llvm_tess_eval_shader(tes);
   char store[DRAW_TES_LLVM_MAX_VARIANT_KEY_SIZE];
   const struct draw_tes_llvm_variant_key *key =
            /* Search shader's list of variants for the key */
   LIST_FOR_EACH_ENTRY(li, &shader->variants.list, list) {
      if (memcmp(&li->base->key, key, shader->variant_key_size) == 0) {
      variant = li->base;
                  if (variant) {
      /* found the variant, move to head of global list (for LRU) */
   list_move_to(&variant->list_item_global.list,
      } else {
               /* First check if we've created too many variants.  If so, free
   * 3.125% of the LRU to avoid using too much memory.
   */
   if (llvm->nr_tes_variants >= DRAW_MAX_SHADER_VARIANTS) {
      if (gallivm_debug & GALLIVM_DEBUG_PERF) {
      debug_printf("Evicting TES: %u tes variants,\t%u total variants\n",
               /*
   * XXX: should we flush here ?
   */
   for (unsigned i = 0; i < DRAW_MAX_SHADER_VARIANTS / 32; i++) {
      struct draw_tes_llvm_variant_list_item *item;
   if (list_is_empty(&llvm->tes_variants_list.list)) {
         }
   item = list_last_entry(&llvm->tes_variants_list.list,
         assert(item);
   assert(item->base);
                           if (variant) {
      list_add(&variant->list_item_local.list, &shader->variants.list);
   list_add(&variant->list_item_global.list, &llvm->tes_variants_list.list);
   llvm->nr_tes_variants++;
                     }
         /**
   * Prepare/validate middle part of the vertex pipeline.
   * NOTE: if you change this function, also look at the non-LLVM
   * function fetch_pipeline_prepare() for similar changes.
   */
   static void
   llvm_middle_end_prepare(struct draw_pt_middle_end *middle,
                     {
      struct llvm_middle_end *fpme = llvm_middle_end(middle);
   struct draw_context *draw = fpme->draw;
   struct draw_llvm *llvm = fpme->llvm;
   struct draw_vertex_shader *vs = draw->vs.vertex_shader;
   struct draw_geometry_shader *gs = draw->gs.geometry_shader;
   struct draw_tess_ctrl_shader *tcs = draw->tcs.tess_ctrl_shader;
   struct draw_tess_eval_shader *tes = draw->tes.tess_eval_shader;
   const enum mesa_prim out_prim =
      gs ? gs->output_primitive : tes ? get_tes_output_prim(tes) :
      unsigned point_line_clip = draw->rasterizer->fill_front == PIPE_POLYGON_MODE_POINT ||
                        fpme->input_prim = in_prim;
            draw_pt_post_vs_prepare(fpme->post_vs,
                           draw->clip_xy,
   draw->clip_z,
   draw->clip_user,
                     if (!(opt & PT_PIPELINE)) {
                  } else {
      /* limit max fetches by limiting max_vertices */
               /* Get the number of float[4] attributes per vertex.
   * Note: this must be done after draw_pt_emit_prepare() since that
   * can effect the vertex size.
   */
            /* Always leave room for the vertex header whether we need it or
   * not.  It's hard to get rid of it in particular because of the
   * viewport code in draw_pt_post_vs.c.
   */
            /* return even number */
            /* Find/create the vertex shader variant */
   {
      struct draw_llvm_variant *variant = NULL;
   struct draw_llvm_variant_list_item *li;
   struct llvm_vertex_shader *shader = llvm_vertex_shader(vs);
   char store[DRAW_LLVM_MAX_VARIANT_KEY_SIZE];
            /* Search shader's list of variants for the key */
   LIST_FOR_EACH_ENTRY(li, &shader->variants.list, list) {
      if (memcmp(&li->base->key, key, shader->variant_key_size) == 0) {
      variant = li->base;
                  if (variant) {
      /* found the variant, move to head of global list (for LRU) */
      } else {
               /* First check if we've created too many variants.  If so, free
   * 3.125% of the LRU to avoid using too much memory.
   */
   if (llvm->nr_variants >= DRAW_MAX_SHADER_VARIANTS) {
      if (gallivm_debug & GALLIVM_DEBUG_PERF) {
                        /*
   * XXX: should we flush here ?
   */
   for (unsigned i = 0; i < DRAW_MAX_SHADER_VARIANTS / 32; i++) {
      struct draw_llvm_variant_list_item *item;
   if (list_is_empty(&llvm->vs_variants_list.list)) {
         }
   item = list_last_entry(&llvm->vs_variants_list.list,
         assert(item);
   assert(item->base);
                           if (variant) {
      list_add(&variant->list_item_local.list, &shader->variants.list);
   list_add(&variant->list_item_global.list, &llvm->vs_variants_list.list);
   llvm->nr_variants++;
                              if (gs) {
         }
   if (tcs) {
         }
   if (tes) {
            }
         static unsigned
   get_num_consts_robust(struct draw_context *draw, struct draw_buffer_info *bufs, unsigned idx)
   {
               if (const_bytes < sizeof(float))
               }
      /**
   * Bind/update constant buffer pointers, clip planes and viewport dims.
   * These are "light weight" parameters which aren't baked into the
   * generated code.  Updating these items is much cheaper than revalidating
   * and rebuilding the generated pipeline code.
   */
   static void
   llvm_middle_end_bind_parameters(struct draw_pt_middle_end *middle)
   {
      static const float fake_const_buf[4];
   struct llvm_middle_end *fpme = llvm_middle_end(middle);
   struct draw_context *draw = fpme->draw;
   struct draw_llvm *llvm = fpme->llvm;
            for (enum pipe_shader_type shader_type = PIPE_SHADER_VERTEX; shader_type <= PIPE_SHADER_GEOMETRY; shader_type++) {
      for (i = 0; i < ARRAY_SIZE(llvm->jit_resources[shader_type].constants); ++i) {
      /*
   * There could be a potential issue with rounding this up, as the
   * shader expects 16-byte allocations, the fix is likely to move
   * to LOAD intrinsic in the future and remove the vec4 constraint.
   */
   int num_consts = get_num_consts_robust(draw, draw->pt.user.constants[shader_type], i);
   llvm->jit_resources[shader_type].constants[i].f = draw->pt.user.constants[shader_type][i].ptr;
   llvm->jit_resources[shader_type].constants[i].num_elements = num_consts;
   if (num_consts == 0) {
            }
   for (i = 0; i < ARRAY_SIZE(llvm->jit_resources[shader_type].ssbos); ++i) {
      int num_ssbos = draw->pt.user.ssbos[shader_type][i].size;
   llvm->jit_resources[shader_type].ssbos[i].u = draw->pt.user.ssbos[shader_type][i].ptr;
   llvm->jit_resources[shader_type].ssbos[i].num_elements = num_ssbos;
   if (num_ssbos == 0) {
                                 llvm->vs_jit_context.planes =
         llvm->gs_jit_context.planes =
            llvm->vs_jit_context.viewports = draw->viewports;
      }
         static void
   pipeline(struct llvm_middle_end *llvm,
               {
      if (prim_info->linear)
         else
      }
         static void
   emit(struct pt_emit *emit,
      const struct draw_vertex_info *vert_info,
      {
      if (prim_info->linear)
         else
      }
         static void
   llvm_pipeline_generic(struct draw_pt_middle_end *middle,
               {
      struct llvm_middle_end *fpme = llvm_middle_end(middle);
   struct draw_context *draw = fpme->draw;
   struct draw_geometry_shader *gshader = draw->gs.geometry_shader;
   struct draw_tess_ctrl_shader *tcs_shader = draw->tcs.tess_ctrl_shader;
   struct draw_tess_eval_shader *tes_shader = draw->tes.tess_eval_shader;
   struct draw_prim_info tcs_prim_info;
   struct draw_prim_info tes_prim_info;
   struct draw_prim_info gs_prim_info[TGSI_MAX_VERTEX_STREAMS];
   struct draw_vertex_info llvm_vert_info;
   struct draw_vertex_info tcs_vert_info;
   struct draw_vertex_info tes_vert_info;
   struct draw_vertex_info *vert_info;
   struct draw_prim_info ia_prim_info;
   struct draw_vertex_info ia_vert_info;
   const struct draw_prim_info *prim_info = in_prim_info;
   bool free_prim_info = false;
   unsigned opt = fpme->opt;
   bool clipped = 0;
                     llvm_vert_info.count = fetch_info->count;
   llvm_vert_info.vertex_size = fpme->vertex_size;
   llvm_vert_info.stride = fpme->vertex_size;
   llvm_vert_info.verts = (struct vertex_header *)
      MALLOC(fpme->vertex_size *
            if (!llvm_vert_info.verts) {
      assert(0);
               if (draw->collect_statistics) {
      draw->statistics.ia_vertices += prim_info->count;
   if (prim_info->prim == MESA_PRIM_PATCHES)
      draw->statistics.ia_primitives +=
      else
      draw->statistics.ia_primitives +=
                  {
      unsigned start, vertex_id_offset;
            if (fetch_info->linear) {
      start = fetch_info->start;
   vertex_id_offset = draw->start_index;
      } else {
      start = draw->pt.user.eltMax;
   vertex_id_offset = draw->pt.user.eltBias;
      }
   /* Run vertex fetch shader */
   clipped = fpme->current_variant->jit_func(&fpme->llvm->vs_jit_context,
                                             &fpme->llvm->jit_resources[PIPE_SHADER_VERTEX],
   llvm_vert_info.verts,
   draw->pt.user.vbuffer,
   fetch_info->count,
   start,
            /* Finished with fetch and vs */
   fetch_info = NULL;
               if (opt & PT_SHADE) {
      struct draw_vertex_shader *vshader = draw->vs.vertex_shader;
   if (tcs_shader) {
      draw_tess_ctrl_shader_run(tcs_shader,
                           vert_info,
   FREE(vert_info->verts);
               } else if (tes_shader) {
      unsigned num_prims = prim_info->count / draw->pt.vertices_per_patch;
   tcs_prim_info = *prim_info;
   tcs_prim_info.primitive_count = num_prims;
               if (tes_shader) {
      draw_tess_eval_shader_run(tes_shader,
                                          FREE(vert_info->verts);
   vert_info = &tes_vert_info;
                  /*
   * pt emit can only handle ushort number of vertices (see
   * render->allocate_vertices).
   * vsplit guarantees there's never more than 4096, however GS can
   * easily blow this up (by a factor of 256 (or even 1024) max).
   */
   if (vert_info->count > 65535) {
                        struct draw_vertex_info gs_vert_info[TGSI_MAX_VERTEX_STREAMS];
            if ((opt & PT_SHADE) && gshader) {
      struct draw_vertex_shader *vshader = draw->vs.vertex_shader;
   draw_geometry_shader_run(gshader,
                           draw->pt.user.constants[PIPE_SHADER_GEOMETRY],
            FREE(vert_info->verts);
   if (free_prim_info) {
      FREE(prim_info->primitive_lengths);
      }
   vert_info = &gs_vert_info[0];
   prim_info = &gs_prim_info[0];
   free_prim_info = false;
   /*
   * pt emit can only handle ushort number of vertices (see
   * render->allocate_vertices).
   * vsplit guarantees there's never more than 4096, however GS can
   * easily blow this up (by a factor of 256 (or even 1024) max).
   */
   if (vert_info->count > 65535) {
            } else {
      if (!tes_shader &&
      draw_prim_assembler_is_required(draw, prim_info, vert_info)) {
                  if (ia_vert_info.count) {
      FREE(vert_info->verts);
   if (free_prim_info) {
      FREE(prim_info->primitive_lengths);
   FREE(tes_elts_out);
      }
   vert_info = &ia_vert_info;
   prim_info = &ia_prim_info;
                     /* stream output needs to be done before clipping */
   draw_pt_so_emit(fpme->so_emit,
                  if (prim_info->count == 0) {
         } else {
               /*
   * if there's no position, need to stop now, or the latter stages
   * will try to access non-existent position output.
   */
   if (draw_current_shader_position_output(draw) != -1) {
      if ((opt & PT_SHADE) &&
      (gshader || tes_shader ||
   draw->vs.vertex_shader->info.writes_viewport_index)) {
      }
   /* "clipped" also includes non-one edgeflag */
   if (clipped) {
                  /* Do we need to run the pipeline? Now will come here if clipped */
   if (opt & PT_PIPELINE) {
         } else {
                        FREE(vert_info->verts);
   if (gshader && gshader->num_vertex_streams > 1)
   for (unsigned i = 1; i < gshader->num_vertex_streams; i++)
            if (free_prim_info) {
      FREE(tes_elts_out);
         }
         static inline enum mesa_prim
   prim_type(enum mesa_prim prim, unsigned flags)
   {
      if (flags & DRAW_LINE_LOOP_AS_STRIP)
         else
      }
         static void
   llvm_middle_end_run(struct draw_pt_middle_end *middle,
                     const unsigned *fetch_elts,
   unsigned fetch_count,
   {
      struct llvm_middle_end *fpme = llvm_middle_end(middle);
   struct draw_fetch_info fetch_info;
            fetch_info.linear = false;
   fetch_info.start = 0;
   fetch_info.elts = fetch_elts;
            prim_info.linear = false;
   prim_info.start = 0;
   prim_info.count = draw_count;
   prim_info.elts = draw_elts;
   prim_info.prim = prim_type(fpme->input_prim, prim_flags);
   prim_info.flags = prim_flags;
   prim_info.primitive_count = 1;
               }
         static void
   llvm_middle_end_linear_run(struct draw_pt_middle_end *middle,
                     {
      struct llvm_middle_end *fpme = llvm_middle_end(middle);
   struct draw_fetch_info fetch_info;
            fetch_info.linear = true;
   fetch_info.start = start;
   fetch_info.count = count;
            prim_info.linear = true;
   prim_info.start = start;
   prim_info.count = count;
   prim_info.elts = NULL;
   prim_info.prim = prim_type(fpme->input_prim, prim_flags);
   prim_info.flags = prim_flags;
   prim_info.primitive_count = 1;
               }
         static bool
   llvm_middle_end_linear_run_elts(struct draw_pt_middle_end *middle,
                                 {
      struct llvm_middle_end *fpme = llvm_middle_end(middle);
   struct draw_fetch_info fetch_info;
            fetch_info.linear = true;
   fetch_info.start = start;
   fetch_info.count = count;
            prim_info.linear = false;
   prim_info.start = 0;
   prim_info.count = draw_count;
   prim_info.elts = draw_elts;
   prim_info.prim = prim_type(fpme->input_prim, prim_flags);
   prim_info.flags = prim_flags;
   prim_info.primitive_count = 1;
                        }
         static void
   llvm_middle_end_finish(struct draw_pt_middle_end *middle)
   {
         }
         static void
   llvm_middle_end_destroy(struct draw_pt_middle_end *middle)
   {
               if (fpme->fetch)
            if (fpme->emit)
            if (fpme->so_emit)
            if (fpme->post_vs)
               }
         struct draw_pt_middle_end *
   draw_pt_fetch_pipeline_or_emit_llvm(struct draw_context *draw)
   {
               if (!draw->llvm)
            fpme = CALLOC_STRUCT(llvm_middle_end);
   if (!fpme)
            fpme->base.prepare         = llvm_middle_end_prepare;
   fpme->base.bind_parameters = llvm_middle_end_bind_parameters;
   fpme->base.run             = llvm_middle_end_run;
   fpme->base.run_linear      = llvm_middle_end_linear_run;
   fpme->base.run_linear_elts = llvm_middle_end_linear_run_elts;
   fpme->base.finish          = llvm_middle_end_finish;
                     fpme->fetch = draw_pt_fetch_create(draw);
   if (!fpme->fetch)
            fpme->post_vs = draw_pt_post_vs_create(draw);
   if (!fpme->post_vs)
            fpme->emit = draw_pt_emit_create(draw);
   if (!fpme->emit)
            fpme->so_emit = draw_pt_so_emit_create(draw);
   if (!fpme->so_emit)
            fpme->llvm = draw->llvm;
   if (!fpme->llvm)
                           fail:
      if (fpme)
               }
