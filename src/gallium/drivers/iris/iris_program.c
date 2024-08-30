   /*
   * Copyright © 2017 Intel Corporation
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      /**
   * @file iris_program.c
   *
   * This file contains the driver interface for compiling shaders.
   *
   * See iris_program_cache.c for the in-memory program cache where the
   * compiled shaders are stored.
   */
      #include <stdio.h>
   #include <errno.h>
   #include "pipe/p_defines.h"
   #include "pipe/p_state.h"
   #include "pipe/p_context.h"
   #include "pipe/p_screen.h"
   #include "util/u_atomic.h"
   #include "util/u_upload_mgr.h"
   #include "util/u_debug.h"
   #include "util/u_async_debug.h"
   #include "compiler/nir/nir.h"
   #include "compiler/nir/nir_builder.h"
   #include "compiler/nir/nir_serialize.h"
   #include "intel/compiler/brw_compiler.h"
   #include "intel/compiler/brw_nir.h"
   #include "intel/compiler/brw_prim.h"
   #include "iris_context.h"
   #include "nir/tgsi_to_nir.h"
      #define KEY_INIT(prefix)                                                   \
      .prefix.program_string_id = ish->program_id,                            \
      #define BRW_KEY_INIT(gen, prog_id, limit_trig_input)       \
      .base.program_string_id = prog_id,                      \
         struct iris_threaded_compile_job {
      struct iris_screen *screen;
   struct u_upload_mgr *uploader;
   struct util_debug_callback *dbg;
   struct iris_uncompiled_shader *ish;
      };
      static unsigned
   get_new_program_id(struct iris_screen *screen)
   {
         }
      void
   iris_finalize_program(struct iris_compiled_shader *shader,
                        struct brw_stage_prog_data *prog_data,
   uint32_t *streamout,
   enum brw_param_builtin *system_values,
      {
      shader->prog_data = prog_data;
   shader->streamout = streamout;
   shader->system_values = system_values;
   shader->num_system_values = num_system_values;
   shader->kernel_input_size = kernel_input_size;
   shader->num_cbufs = num_cbufs;
            ralloc_steal(shader, shader->prog_data);
   ralloc_steal(shader->prog_data, (void *)prog_data->relocs);
   ralloc_steal(shader->prog_data, prog_data->param);
   ralloc_steal(shader, shader->streamout);
      }
      static struct brw_vs_prog_key
   iris_to_brw_vs_key(const struct iris_screen *screen,
         {
      return (struct brw_vs_prog_key) {
      BRW_KEY_INIT(screen->devinfo->ver, key->vue.base.program_string_id,
            /* Don't tell the backend about our clip plane constants, we've
   * already lowered them in NIR and don't want it doing it again.
   */
         }
      static struct brw_tcs_prog_key
   iris_to_brw_tcs_key(const struct iris_screen *screen,
         {
      return (struct brw_tcs_prog_key) {
      BRW_KEY_INIT(screen->devinfo->ver, key->vue.base.program_string_id,
         ._tes_primitive_mode = key->_tes_primitive_mode,
   .input_vertices = key->input_vertices,
   .patch_outputs_written = key->patch_outputs_written,
   .outputs_written = key->outputs_written,
         }
      static struct brw_tes_prog_key
   iris_to_brw_tes_key(const struct iris_screen *screen,
         {
      return (struct brw_tes_prog_key) {
      BRW_KEY_INIT(screen->devinfo->ver, key->vue.base.program_string_id,
         .patch_inputs_read = key->patch_inputs_read,
         }
      static struct brw_gs_prog_key
   iris_to_brw_gs_key(const struct iris_screen *screen,
         {
      return (struct brw_gs_prog_key) {
      BRW_KEY_INIT(screen->devinfo->ver, key->vue.base.program_string_id,
         }
      static struct brw_wm_prog_key
   iris_to_brw_fs_key(const struct iris_screen *screen,
         {
      return (struct brw_wm_prog_key) {
      BRW_KEY_INIT(screen->devinfo->ver, key->base.program_string_id,
         .nr_color_regions = key->nr_color_regions,
   .flat_shade = key->flat_shade,
   .alpha_test_replicate_alpha = key->alpha_test_replicate_alpha,
   .alpha_to_coverage = key->alpha_to_coverage ? BRW_ALWAYS : BRW_NEVER,
   .clamp_fragment_color = key->clamp_fragment_color,
   .persample_interp = key->persample_interp ? BRW_ALWAYS : BRW_NEVER,
   .multisample_fbo = key->multisample_fbo ? BRW_ALWAYS : BRW_NEVER,
   .force_dual_color_blend = key->force_dual_color_blend,
   .coherent_fb_fetch = key->coherent_fb_fetch,
   .color_outputs_valid = key->color_outputs_valid,
   .input_slots_valid = key->input_slots_valid,
         }
      static struct brw_cs_prog_key
   iris_to_brw_cs_key(const struct iris_screen *screen,
         {
      return (struct brw_cs_prog_key) {
      BRW_KEY_INIT(screen->devinfo->ver, key->base.program_string_id,
         }
      static void *
   upload_state(struct u_upload_mgr *uploader,
               struct iris_state_ref *ref,
   {
      void *p = NULL;
   u_upload_alloc(uploader, 0, size, alignment, &ref->offset, &ref->res, &p);
      }
      void
   iris_upload_ubo_ssbo_surf_state(struct iris_context *ice,
                     {
      struct pipe_context *ctx = &ice->ctx;
   struct iris_screen *screen = (struct iris_screen *) ctx->screen;
            void *map =
      upload_state(ice->state.surface_uploader, surf_state,
      if (!unlikely(map)) {
      surf_state->res = NULL;
               struct iris_resource *res = (void *) buf->buffer;
   struct iris_bo *surf_bo = iris_resource_bo(surf_state->res);
                     isl_buffer_fill_state(&screen->isl_dev, map,
                        .address = res->bo->address + res->offset +
         .size_B = buf->buffer_size - res->offset,
   .format = dataport ? ISL_FORMAT_RAW
   }
      static nir_def *
   get_aoa_deref_offset(nir_builder *b,
               {
      unsigned array_size = elem_size;
            while (deref->deref_type != nir_deref_type_var) {
               /* This level's element size is the previous level's array size */
   nir_def *index = deref->arr.index.ssa;
   assert(deref->arr.index.ssa);
   offset = nir_iadd(b, offset,
            deref = nir_deref_instr_parent(deref);
   assert(glsl_type_is_array(deref->type));
               /* Accessing an invalid surface index with the dataport can result in a
   * hang.  According to the spec "if the index used to select an individual
   * element is negative or greater than or equal to the size of the array,
   * the results of the operation are undefined but may not lead to
   * termination" -- which is one of the possible outcomes of the hang.
   * Clamp the index to prevent access outside of the array bounds.
   */
      }
      static void
   iris_lower_storage_image_derefs(nir_shader *nir)
   {
                        nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
                     nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   switch (intrin->intrinsic) {
   case nir_intrinsic_image_deref_load:
   case nir_intrinsic_image_deref_store:
   case nir_intrinsic_image_deref_atomic:
   case nir_intrinsic_image_deref_atomic_swap:
   case nir_intrinsic_image_deref_size:
   case nir_intrinsic_image_deref_samples:
   case nir_intrinsic_image_deref_load_raw_intel:
   case nir_intrinsic_image_deref_store_raw_intel: {
                     b.cursor = nir_before_instr(&intrin->instr);
   nir_def *index =
      nir_iadd_imm(&b, get_aoa_deref_offset(&b, deref, 1),
      nir_rewrite_image_intrinsic(intrin, index, false);
               default:
                  }
      static bool
   iris_uses_image_atomic(const nir_shader *shader)
   {
      nir_foreach_function_impl(impl, shader) {
      nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
                     nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   switch (intrin->intrinsic) {
   case nir_intrinsic_image_deref_atomic:
   case nir_intrinsic_image_deref_atomic_swap:
                  case nir_intrinsic_image_atomic:
                  default:
                              }
      /**
   * Undo nir_lower_passthrough_edgeflags but keep the inputs_read flag.
   */
   static bool
   iris_fix_edge_flags(nir_shader *nir)
   {
      if (nir->info.stage != MESA_SHADER_VERTEX) {
      nir_shader_preserve_all_metadata(nir);
               nir_variable *var = nir_find_variable_with_location(nir, nir_var_shader_out,
         if (!var) {
      nir_shader_preserve_all_metadata(nir);
               var->data.mode = nir_var_shader_temp;
   nir->info.outputs_written &= ~VARYING_BIT_EDGE;
   nir->info.inputs_read &= ~VERT_BIT_EDGEFLAG;
            nir_foreach_function_impl(impl, nir) {
      nir_metadata_preserve(impl, nir_metadata_block_index |
                              }
      /**
   * Fix an uncompiled shader's stream output info.
   *
   * Core Gallium stores output->register_index as a "slot" number, where
   * slots are assigned consecutively to all outputs in info->outputs_written.
   * This naive packing of outputs doesn't work for us - we too have slots,
   * but the layout is defined by the VUE map, which we won't have until we
   * compile a specific shader variant.  So, we remap these and simply store
   * VARYING_SLOT_* in our copy's output->register_index fields.
   *
   * We also fix up VARYING_SLOT_{LAYER,VIEWPORT,PSIZ} to select the Y/Z/W
   * components of our VUE header.  See brw_vue_map.c for the layout.
   */
   static void
   update_so_info(struct pipe_stream_output_info *so_info,
         {
      uint8_t reverse_map[64] = {};
   unsigned slot = 0;
   while (outputs_written) {
                  for (unsigned i = 0; i < so_info->num_outputs; i++) {
               /* Map Gallium's condensed "slots" back to real VARYING_SLOT_* enums */
            /* The VUE header contains three scalar fields packed together:
   * - gl_PointSize is stored in VARYING_SLOT_PSIZ.w
   * - gl_Layer is stored in VARYING_SLOT_PSIZ.y
   * - gl_ViewportIndex is stored in VARYING_SLOT_PSIZ.z
   */
   switch (output->register_index) {
   case VARYING_SLOT_LAYER:
      assert(output->num_components == 1);
   output->register_index = VARYING_SLOT_PSIZ;
   output->start_component = 1;
      case VARYING_SLOT_VIEWPORT:
      assert(output->num_components == 1);
   output->register_index = VARYING_SLOT_PSIZ;
   output->start_component = 2;
      case VARYING_SLOT_PSIZ:
      assert(output->num_components == 1);
   output->start_component = 3;
                     }
      static void
   setup_vec4_image_sysval(uint32_t *sysvals, uint32_t idx,
         {
               for (unsigned i = 0; i < n; ++i)
            for (unsigned i = n; i < 4; ++i)
      }
      /**
   * Associate NIR uniform variables with the prog_data->param[] mechanism
   * used by the backend.  Also, decide which UBOs we'd like to push in an
   * ideal situation (though the backend can reduce this).
   */
   static void
   iris_setup_uniforms(ASSERTED const struct intel_device_info *devinfo,
                     void *mem_ctx,
   nir_shader *nir,
   struct brw_stage_prog_data *prog_data,
   unsigned kernel_input_size,
   {
               const unsigned IRIS_MAX_SYSTEM_VALUES =
         enum brw_param_builtin *system_values =
                  unsigned patch_vert_idx = -1;
   unsigned tess_outer_default_idx = -1;
   unsigned tess_inner_default_idx = -1;
   unsigned ucp_idx[IRIS_MAX_CLIP_PLANES];
   unsigned img_idx[PIPE_MAX_SHADER_IMAGES];
   unsigned variable_group_size_idx = -1;
   unsigned work_dim_idx = -1;
   memset(ucp_idx, -1, sizeof(ucp_idx));
                                       /* Turn system value intrinsics into uniforms */
   nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
                                    switch (intrin->intrinsic) {
   case nir_intrinsic_load_base_workgroup_id: {
      /* GL doesn't have a concept of base workgroup */
   b.cursor = nir_instr_remove(&intrin->instr);
   nir_def_rewrite_uses(&intrin->def,
            }
   case nir_intrinsic_load_constant: {
      unsigned load_size = intrin->def.num_components *
                  /* This one is special because it reads from the shader constant
   * data and not cbuf0 which gallium uploads for us.
                  nir_def *offset =
                  assert(load_size < b.shader->constant_data_size);
                  /* Constant data lives in buffers within IRIS_MEMZONE_SHADER
   * and cannot cross that 4GB boundary, so we can do the address
   * calculation with 32-bit adds.  Also, we can ignore the high
   * bits because IRIS_MEMZONE_SHADER is in the [0, 4GB) range.
                                 nir_def *data =
      nir_load_global_constant(&b, nir_u2u64(&b, const_data_addr),
                     nir_def_rewrite_uses(&intrin->def,
            }
                     if (ucp_idx[ucp] == -1) {
                        for (int i = 0; i < 4; i++) {
                        b.cursor = nir_before_instr(instr);
   offset = nir_imm_int(&b, system_values_start +
            }
   case nir_intrinsic_load_patch_vertices_in:
                                    b.cursor = nir_before_instr(instr);
   offset = nir_imm_int(&b, system_values_start +
            case nir_intrinsic_load_tess_level_outer_default:
      if (tess_outer_default_idx == -1) {
                        for (int i = 0; i < 4; i++) {
                        b.cursor = nir_before_instr(instr);
   offset = nir_imm_int(&b, system_values_start +
            case nir_intrinsic_load_tess_level_inner_default:
      if (tess_inner_default_idx == -1) {
                        for (int i = 0; i < 2; i++) {
                        b.cursor = nir_before_instr(instr);
   offset = nir_imm_int(&b, system_values_start +
            case nir_intrinsic_image_deref_load_param_intel: {
      assert(devinfo->ver < 9);
                  if (img_idx[var->data.binding] == -1) {
                                                                     setup_vec4_image_sysval(
      img_sv + BRW_IMAGE_PARAM_OFFSET_OFFSET, img,
      setup_vec4_image_sysval(
      img_sv + BRW_IMAGE_PARAM_SIZE_OFFSET, img,
      setup_vec4_image_sysval(
      img_sv + BRW_IMAGE_PARAM_STRIDE_OFFSET, img,
      setup_vec4_image_sysval(
      img_sv + BRW_IMAGE_PARAM_TILING_OFFSET, img,
      setup_vec4_image_sysval(
                        b.cursor = nir_before_instr(instr);
   offset = nir_iadd_imm(&b,
      get_aoa_deref_offset(&b, deref, BRW_IMAGE_PARAM_SIZE * 4),
   system_values_start +
   img_idx[var->data.binding] * 4 +
         }
   case nir_intrinsic_load_workgroup_size: {
      assert(nir->info.workgroup_size_variable);
   if (variable_group_size_idx == -1) {
      variable_group_size_idx = num_system_values;
   num_system_values += 3;
   for (int i = 0; i < 3; i++) {
      system_values[variable_group_size_idx + i] =
                  b.cursor = nir_before_instr(instr);
   offset = nir_imm_int(&b, system_values_start +
            }
   case nir_intrinsic_load_work_dim: {
      if (work_dim_idx == -1) {
      work_dim_idx = num_system_values++;
      }
   b.cursor = nir_before_instr(instr);
   offset = nir_imm_int(&b, system_values_start +
            }
   case nir_intrinsic_load_kernel_input: {
      assert(nir_intrinsic_base(intrin) +
         b.cursor = nir_before_instr(instr);
   offset = nir_iadd_imm(&b, intrin->src[0].ssa,
            }
   default:
                  nir_def *load =
      nir_load_ubo(&b, intrin->def.num_components, intrin->def.bit_size,
               temp_ubo_name, offset,
               nir_def_rewrite_uses(&intrin->def,
                                 /* Uniforms are stored in constant buffer 0, the
   * user-facing UBOs are indexed by one.  So if any constant buffer is
   * needed, the constant buffer 0 will be needed, so account for it.
   */
   unsigned num_cbufs = nir->info.num_ubos;
   if (num_cbufs || nir->num_uniforms)
            /* Place the new params in a new cbuf. */
   if (num_system_values > 0 || kernel_input_size > 0) {
      unsigned sysval_cbuf_index = num_cbufs;
            system_values = reralloc(mem_ctx, system_values, enum brw_param_builtin,
            nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
                                                      if (load->src[0].ssa == temp_ubo_name) {
      nir_def *imm = nir_imm_int(&b, sysval_cbuf_index);
                     /* We need to fold the new iadds for brw_nir_analyze_ubo_ranges */
      } else {
      ralloc_free(system_values);
               assert(num_cbufs < PIPE_MAX_CONSTANT_BUFFERS);
            /* We don't use params[] but gallium leaves num_uniforms set.  We use this
   * to detect when cbuf0 exists but we don't need it anymore when we get
   * here.  Instead, zero it out so that the back-end doesn't get confused
   * when nr_params * 4 != num_uniforms != nr_params * 4.
   */
            *out_system_values = system_values;
   *out_num_system_values = num_system_values;
      }
      static const char *surface_group_names[] = {
      [IRIS_SURFACE_GROUP_RENDER_TARGET]      = "render target",
   [IRIS_SURFACE_GROUP_RENDER_TARGET_READ] = "non-coherent render target read",
   [IRIS_SURFACE_GROUP_CS_WORK_GROUPS]     = "CS work groups",
   [IRIS_SURFACE_GROUP_TEXTURE_LOW64]      = "texture",
   [IRIS_SURFACE_GROUP_TEXTURE_HIGH64]     = "texture",
   [IRIS_SURFACE_GROUP_UBO]                = "ubo",
   [IRIS_SURFACE_GROUP_SSBO]               = "ssbo",
      };
      static void
   iris_print_binding_table(FILE *fp, const char *name,
         {
               uint32_t total = 0;
            for (int i = 0; i < IRIS_SURFACE_GROUP_COUNT; i++) {
      uint32_t size = bt->sizes[i];
   total += size;
   if (size)
               if (total == 0) {
      fprintf(fp, "Binding table for %s is empty\n\n", name);
               if (total != compacted) {
      fprintf(fp, "Binding table for %s "
            } else {
                  uint32_t entry = 0;
   for (int i = 0; i < IRIS_SURFACE_GROUP_COUNT; i++) {
      uint64_t mask = bt->used_mask[i];
   while (mask) {
      int index = u_bit_scan64(&mask);
         }
      }
      enum {
      /* Max elements in a surface group. */
      };
      /**
   * Map a <group, index> pair to a binding table index.
   *
   * For example: <UBO, 5> => binding table index 12
   */
   uint32_t
   iris_group_index_to_bti(const struct iris_binding_table *bt,
         {
      assert(index < bt->sizes[group]);
   uint64_t mask = bt->used_mask[group];
   uint64_t bit = 1ull << index;
   if (bit & mask) {
         } else {
            }
      /**
   * Map a binding table index back to a <group, index> pair.
   *
   * For example: binding table index 12 => <UBO, 5>
   */
   uint32_t
   iris_bti_to_group_index(const struct iris_binding_table *bt,
         {
      uint64_t used_mask = bt->used_mask[group];
            uint32_t c = bti - bt->offsets[group];
   while (used_mask) {
      int i = u_bit_scan64(&used_mask);
   if (c == 0)
                        }
      static void
   rewrite_src_with_bti(nir_builder *b, struct iris_binding_table *bt,
               {
               b->cursor = nir_before_instr(instr);
   nir_def *bti;
   if (nir_src_is_const(*src)) {
      uint32_t index = nir_src_as_uint(*src);
   bti = nir_imm_intN_t(b, iris_group_index_to_bti(bt, group, index),
      } else {
      /* Indirect usage makes all the surfaces of the group to be available,
   * so we can just add the base.
   */
   assert(bt->used_mask[group] == BITFIELD64_MASK(bt->sizes[group]));
      }
      }
      static void
   mark_used_with_src(struct iris_binding_table *bt, nir_src *src,
         {
               if (nir_src_is_const(*src)) {
      uint64_t index = nir_src_as_uint(*src);
   assert(index < bt->sizes[group]);
      } else {
      /* There's an indirect usage, we need all the surfaces. */
         }
      static bool
   skip_compacting_binding_tables(void)
   {
      static int skip = -1;
   if (skip < 0)
            }
      /**
   * Set up the binding table indices and apply to the shader.
   */
   static void
   iris_setup_binding_table(const struct intel_device_info *devinfo,
                           struct nir_shader *nir,
   {
                        /* Set the sizes for each surface group.  For some groups, we already know
   * upfront how many will be used, so mark them.
   */
   if (info->stage == MESA_SHADER_FRAGMENT) {
      bt->sizes[IRIS_SURFACE_GROUP_RENDER_TARGET] = num_render_targets;
   /* All render targets used. */
   bt->used_mask[IRIS_SURFACE_GROUP_RENDER_TARGET] =
            /* Setup render target read surface group in order to support non-coherent
   * framebuffer fetch on Gfx8
   */
   if (devinfo->ver == 8 && info->outputs_read) {
      bt->sizes[IRIS_SURFACE_GROUP_RENDER_TARGET_READ] = num_render_targets;
   bt->used_mask[IRIS_SURFACE_GROUP_RENDER_TARGET_READ] =
         } else if (info->stage == MESA_SHADER_COMPUTE) {
                  assert(ARRAY_SIZE(info->textures_used) >= 4);
   int max_tex = BITSET_LAST_BIT(info->textures_used);
   assert(max_tex <= 128);
   bt->sizes[IRIS_SURFACE_GROUP_TEXTURE_LOW64] = MIN2(64, max_tex);
   bt->sizes[IRIS_SURFACE_GROUP_TEXTURE_HIGH64] = MAX2(0, max_tex - 64);
   bt->used_mask[IRIS_SURFACE_GROUP_TEXTURE_LOW64] =
         bt->used_mask[IRIS_SURFACE_GROUP_TEXTURE_HIGH64] =
                           /* Allocate an extra slot in the UBO section for NIR constants.
   * Binding table compaction will remove it if unnecessary.
   *
   * We don't include them in iris_compiled_shader::num_cbufs because
   * they are uploaded separately from shs->constbuf[], but from a shader
   * point of view, they're another UBO (at the end of the section).
   */
                     for (int i = 0; i < IRIS_SURFACE_GROUP_COUNT; i++)
            /* Mark surfaces used for the cases we don't have the information available
   * upfront.
   */
   nir_function_impl *impl = nir_shader_get_entrypoint(nir);
   nir_foreach_block (block, impl) {
      nir_foreach_instr (instr, block) {
                     nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   switch (intrin->intrinsic) {
   case nir_intrinsic_load_num_workgroups:
                  case nir_intrinsic_load_output:
      if (devinfo->ver == 8) {
      mark_used_with_src(bt, &intrin->src[0],
                  case nir_intrinsic_image_size:
   case nir_intrinsic_image_load:
   case nir_intrinsic_image_store:
   case nir_intrinsic_image_atomic:
   case nir_intrinsic_image_atomic_swap:
   case nir_intrinsic_image_load_raw_intel:
   case nir_intrinsic_image_store_raw_intel:
                  case nir_intrinsic_load_ubo:
                  case nir_intrinsic_store_ssbo:
                  case nir_intrinsic_get_ssbo_size:
   case nir_intrinsic_ssbo_atomic:
   case nir_intrinsic_ssbo_atomic_swap:
   case nir_intrinsic_load_ssbo:
                  default:
                        /* When disable we just mark everything as used. */
   if (unlikely(skip_compacting_binding_tables())) {
      for (int i = 0; i < IRIS_SURFACE_GROUP_COUNT; i++)
               /* Calculate the offsets and the binding table size based on the used
   * surfaces.  After this point, the functions to go between "group indices"
   * and binding table indices can be used.
   */
   uint32_t next = 0;
   for (int i = 0; i < IRIS_SURFACE_GROUP_COUNT; i++) {
      if (bt->used_mask[i] != 0) {
      bt->offsets[i] = next;
         }
            if (INTEL_DEBUG(DEBUG_BT)) {
                  /* Apply the binding table indices.  The backend compiler is not expected
   * to change those, as we haven't set any of the *_start entries in brw
   * binding_table.
   */
            nir_foreach_block (block, impl) {
      nir_foreach_instr (instr, block) {
      if (instr->type == nir_instr_type_tex) {
      nir_tex_instr *tex = nir_instr_as_tex(instr);
   if (tex->texture_index < 64) {
      tex->texture_index =
      iris_group_index_to_bti(bt, IRIS_SURFACE_GROUP_TEXTURE_LOW64,
   } else {
      tex->texture_index =
      iris_group_index_to_bti(bt, IRIS_SURFACE_GROUP_TEXTURE_HIGH64,
   }
                              nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   switch (intrin->intrinsic) {
   case nir_intrinsic_image_size:
   case nir_intrinsic_image_load:
   case nir_intrinsic_image_store:
   case nir_intrinsic_image_atomic:
   case nir_intrinsic_image_atomic_swap:
   case nir_intrinsic_image_load_raw_intel:
   case nir_intrinsic_image_store_raw_intel:
      rewrite_src_with_bti(&b, bt, instr, &intrin->src[0],
               case nir_intrinsic_load_ubo:
      rewrite_src_with_bti(&b, bt, instr, &intrin->src[0],
               case nir_intrinsic_store_ssbo:
      rewrite_src_with_bti(&b, bt, instr, &intrin->src[1],
               case nir_intrinsic_load_output:
      if (devinfo->ver == 8) {
      rewrite_src_with_bti(&b, bt, instr, &intrin->src[0],
                  case nir_intrinsic_get_ssbo_size:
   case nir_intrinsic_ssbo_atomic:
   case nir_intrinsic_ssbo_atomic_swap:
   case nir_intrinsic_load_ssbo:
      rewrite_src_with_bti(&b, bt, instr, &intrin->src[0],
               default:
                  }
      static void
   iris_debug_recompile(struct iris_screen *screen,
                     {
      if (!ish || list_is_empty(&ish->variants)
                  const struct brw_compiler *c = screen->compiler;
            brw_shader_perf_log(c, dbg, "Recompiling %s shader for program %s: %s\n",
                        struct iris_compiled_shader *shader =
                           switch (info->stage) {
   case MESA_SHADER_VERTEX:
      old_key.vs = iris_to_brw_vs_key(screen, old_iris_key);
      case MESA_SHADER_TESS_CTRL:
      old_key.tcs = iris_to_brw_tcs_key(screen, old_iris_key);
      case MESA_SHADER_TESS_EVAL:
      old_key.tes = iris_to_brw_tes_key(screen, old_iris_key);
      case MESA_SHADER_GEOMETRY:
      old_key.gs = iris_to_brw_gs_key(screen, old_iris_key);
      case MESA_SHADER_FRAGMENT:
      old_key.wm = iris_to_brw_fs_key(screen, old_iris_key);
      case MESA_SHADER_COMPUTE:
      old_key.cs = iris_to_brw_cs_key(screen, old_iris_key);
      default:
                     }
      static void
   check_urb_size(struct iris_context *ice,
               {
               /* If the last URB allocation wasn't large enough for our needs,
   * flag it as needing to be reconfigured.  Otherwise, we can use
   * the existing config.  However, if the URB is constrained, and
   * we can shrink our size for this stage, we may be able to gain
   * extra concurrency by reconfiguring it to be smaller.  Do so.
   */
   if (last_allocated_size < needed_size ||
      (ice->shaders.urb.constrained && last_allocated_size > needed_size)) {
         }
      /**
   * Get the shader for the last enabled geometry stage.
   *
   * This stage is the one which will feed stream output and the rasterizer.
   */
   static gl_shader_stage
   last_vue_stage(struct iris_context *ice)
   {
      if (ice->shaders.uncompiled[MESA_SHADER_GEOMETRY])
            if (ice->shaders.uncompiled[MESA_SHADER_TESS_EVAL])
               }
      /**
   * \param added  Set to \c true if the variant was added to the list (i.e., a
   *               variant matching \c key was not found).  Set to \c false
   *               otherwise.
   */
   static inline struct iris_compiled_shader *
   find_or_add_variant(const struct iris_screen *screen,
                     struct iris_uncompiled_shader *ish,
   {
                        if (screen->precompile) {
      /* Check the first list entry.  There will always be at least one
   * variant in the list (most likely the precompile variant), and
   * other contexts only append new variants, so we can safely check
   * it without locking, saving that cost in the common case.
   */
   struct iris_compiled_shader *first =
            if (memcmp(&first->key, key, key_size) == 0) {
      util_queue_fence_wait(&first->ready);
               /* Skip this one in the loop below */
                        /* If it doesn't match, we have to walk the list; other contexts may be
   * concurrently appending shaders to it, so we need to lock here.
   */
            list_for_each_entry_from(struct iris_compiled_shader, v, start,
            if (memcmp(&v->key, key, key_size) == 0) {
      variant = v;
                  if (variant == NULL) {
      variant = iris_create_shader_variant(screen, NULL, cache_id,
            /* Append our new variant to the shader's variant list. */
   list_addtail(&variant->link, &ish->variants);
               } else {
                              }
      static void
   iris_threaded_compile_job_delete(void *_job, UNUSED void *_gdata,
         {
         }
      static void
   iris_schedule_compile(struct iris_screen *screen,
                              {
               if (dbg) {
      u_async_debug_init(&async_debug);
               util_queue_add_job(&screen->shader_compiler_queue, job, ready_fence, execute,
            if (screen->driconf.sync_compile || dbg)
            if (dbg) {
      u_async_debug_drain(&async_debug, dbg);
         }
      /**
   * Compile a vertex shader, and upload the assembly.
   */
   static void
   iris_compile_vs(struct iris_screen *screen,
                  struct u_upload_mgr *uploader,
      {
      const struct brw_compiler *compiler = screen->compiler;
   const struct intel_device_info *devinfo = screen->devinfo;
   void *mem_ctx = ralloc_context(NULL);
   struct brw_vs_prog_data *vs_prog_data =
         struct brw_vue_prog_data *vue_prog_data = &vs_prog_data->base;
   struct brw_stage_prog_data *prog_data = &vue_prog_data->base;
   enum brw_param_builtin *system_values;
   unsigned num_system_values;
            nir_shader *nir = nir_shader_clone(mem_ctx, ish->nir);
            if (key->vue.nr_userclip_plane_consts) {
      nir_function_impl *impl = nir_shader_get_entrypoint(nir);
   /* Check if variables were found. */
   if (nir_lower_clip_vs(nir, (1 << key->vue.nr_userclip_plane_consts) - 1,
            nir_lower_io_to_temporaries(nir, impl, true, false);
   nir_lower_global_vars_to_local(nir);
   nir_lower_vars_to_ssa(nir);
                           iris_setup_uniforms(devinfo, mem_ctx, nir, prog_data, 0, &system_values,
            struct iris_binding_table bt;
   iris_setup_binding_table(devinfo, nir, &bt, /* num_render_targets */ 0,
                     brw_compute_vue_map(devinfo,
                           struct brw_compile_vs_params params = {
      .base = {
      .mem_ctx = mem_ctx,
   .nir = nir,
   .log_data = dbg,
      },
   .key = &brw_key,
               const unsigned *program = brw_compile_vs(compiler, &params);
   if (program == NULL) {
      dbg_printf("Failed to compile vertex shader: %s\n", params.base.error_str);
            shader->compilation_failed = true;
                                          uint32_t *so_decls =
      screen->vtbl.create_so_decl_list(&ish->stream_output,
         iris_finalize_program(shader, prog_data, so_decls, system_values,
            iris_upload_shader(screen, ish, shader, NULL, uploader, IRIS_CACHE_VS,
                        }
      /**
   * Update the current vertex shader variant.
   *
   * Fill out the key, look in the cache, compile and bind if needed.
   */
   static void
   iris_update_compiled_vs(struct iris_context *ice)
   {
      struct iris_screen *screen = (struct iris_screen *)ice->ctx.screen;
   struct iris_shader_state *shs = &ice->state.shaders[MESA_SHADER_VERTEX];
   struct u_upload_mgr *uploader = ice->shaders.uploader_driver;
   struct iris_uncompiled_shader *ish =
            struct iris_vs_prog_key key = { KEY_INIT(vue.base) };
            struct iris_compiled_shader *old = ice->shaders.prog[IRIS_CACHE_VS];
   bool added;
   struct iris_compiled_shader *shader =
            if (added && !iris_disk_cache_retrieve(screen, uploader, ish, shader,
                        if (shader->compilation_failed)
            if (old != shader) {
      iris_shader_variant_reference(&ice->shaders.prog[MESA_SHADER_VERTEX],
         ice->state.dirty |= IRIS_DIRTY_VF_SGVS;
   ice->state.stage_dirty |= IRIS_STAGE_DIRTY_VS |
                        unsigned urb_entry_size = shader ?
               }
      /**
   * Get the shader_info for a given stage, or NULL if the stage is disabled.
   */
   const struct shader_info *
   iris_get_shader_info(const struct iris_context *ice, gl_shader_stage stage)
   {
               if (!ish)
            const nir_shader *nir = ish->nir;
      }
      /**
   * Get the union of TCS output and TES input slots.
   *
   * TCS and TES need to agree on a common URB entry layout.  In particular,
   * the data for all patch vertices is stored in a single URB entry (unlike
   * GS which has one entry per input vertex).  This means that per-vertex
   * array indexing needs a stride.
   *
   * SSO requires locations to match, but doesn't require the number of
   * outputs/inputs to match (in fact, the TCS often has extra outputs).
   * So, we need to take the extra step of unifying these on the fly.
   */
   static void
   get_unified_tess_slots(const struct iris_context *ice,
               {
      const struct shader_info *tcs =
         const struct shader_info *tes =
            *per_vertex_slots = tes->inputs_read;
            if (tcs) {
      *per_vertex_slots |= tcs->outputs_written;
         }
      /**
   * Compile a tessellation control shader, and upload the assembly.
   */
   static void
   iris_compile_tcs(struct iris_screen *screen,
                  struct hash_table *passthrough_ht,
   struct u_upload_mgr *uploader,
      {
      const struct brw_compiler *compiler = screen->compiler;
   void *mem_ctx = ralloc_context(NULL);
   struct brw_tcs_prog_data *tcs_prog_data =
         struct brw_vue_prog_data *vue_prog_data = &tcs_prog_data->base;
   struct brw_stage_prog_data *prog_data = &vue_prog_data->base;
   const struct intel_device_info *devinfo = screen->devinfo;
   enum brw_param_builtin *system_values = NULL;
   unsigned num_system_values = 0;
                              const struct iris_tcs_prog_key *const key = &shader->key.tcs;
   struct brw_tcs_prog_key brw_key = iris_to_brw_tcs_key(screen, key);
            if (ish) {
      nir = nir_shader_clone(mem_ctx, ish->nir);
      } else {
      nir = brw_nir_create_passthrough_tcs(mem_ctx, compiler, &brw_key);
               iris_setup_uniforms(devinfo, mem_ctx, nir, prog_data, 0, &system_values,
         iris_setup_binding_table(devinfo, nir, &bt, /* num_render_targets */ 0,
                  struct brw_compile_tcs_params params = {
      .base = {
      .mem_ctx = mem_ctx,
   .nir = nir,
   .log_data = dbg,
      },
   .key = &brw_key,
               const unsigned *program = brw_compile_tcs(compiler, &params);
   if (program == NULL) {
      dbg_printf("Failed to compile control shader: %s\n", params.base.error_str);
            shader->compilation_failed = true;
                                          iris_finalize_program(shader, prog_data, NULL, system_values,
            iris_upload_shader(screen, ish, shader, passthrough_ht, uploader,
            if (ish)
               }
      /**
   * Update the current tessellation control shader variant.
   *
   * Fill out the key, look in the cache, compile and bind if needed.
   */
   static void
   iris_update_compiled_tcs(struct iris_context *ice)
   {
      struct iris_shader_state *shs = &ice->state.shaders[MESA_SHADER_TESS_CTRL];
   struct iris_uncompiled_shader *tcs =
         struct iris_screen *screen = (struct iris_screen *)ice->ctx.screen;
   struct u_upload_mgr *uploader = ice->shaders.uploader_driver;
   const struct brw_compiler *compiler = screen->compiler;
            const struct shader_info *tes_info =
         struct iris_tcs_prog_key key = {
      .vue.base.program_string_id = tcs ? tcs->program_id : 0,
   ._tes_primitive_mode = tes_info->tess._primitive_mode,
   .input_vertices =
         .quads_workaround = devinfo->ver < 9 &&
            };
   get_unified_tess_slots(ice, &key.outputs_written,
                  struct iris_compiled_shader *old = ice->shaders.prog[IRIS_CACHE_TCS];
   struct iris_compiled_shader *shader;
            if (tcs != NULL) {
      shader = find_or_add_variant(screen, tcs, IRIS_CACHE_TCS, &key,
      } else {
      /* Look for and possibly create a passthrough TCS */
               if (shader == NULL) {
      shader = iris_create_shader_variant(screen, ice->shaders.cache,
                           /* If the shader was not found in (whichever cache), call iris_compile_tcs
   * if either ish is NULL or the shader could not be found in the disk
   * cache.
   */
   if (added &&
      (tcs == NULL || !iris_disk_cache_retrieve(screen, uploader, tcs, shader,
         iris_compile_tcs(screen, ice->shaders.cache, uploader, &ice->dbg, tcs,
               if (shader->compilation_failed)
            if (old != shader) {
      iris_shader_variant_reference(&ice->shaders.prog[MESA_SHADER_TESS_CTRL],
         ice->state.stage_dirty |= IRIS_STAGE_DIRTY_TCS |
                        unsigned urb_entry_size = shader ?
               }
      /**
   * Compile a tessellation evaluation shader, and upload the assembly.
   */
   static void
   iris_compile_tes(struct iris_screen *screen,
                  struct u_upload_mgr *uploader,
      {
      const struct brw_compiler *compiler = screen->compiler;
   void *mem_ctx = ralloc_context(NULL);
   struct brw_tes_prog_data *tes_prog_data =
         struct brw_vue_prog_data *vue_prog_data = &tes_prog_data->base;
   struct brw_stage_prog_data *prog_data = &vue_prog_data->base;
   enum brw_param_builtin *system_values;
   const struct intel_device_info *devinfo = screen->devinfo;
   unsigned num_system_values;
            nir_shader *nir = nir_shader_clone(mem_ctx, ish->nir);
            if (key->vue.nr_userclip_plane_consts) {
      nir_function_impl *impl = nir_shader_get_entrypoint(nir);
   nir_lower_clip_vs(nir, (1 << key->vue.nr_userclip_plane_consts) - 1,
         nir_lower_io_to_temporaries(nir, impl, true, false);
   nir_lower_global_vars_to_local(nir);
   nir_lower_vars_to_ssa(nir);
               iris_setup_uniforms(devinfo, mem_ctx, nir, prog_data, 0, &system_values,
            struct iris_binding_table bt;
   iris_setup_binding_table(devinfo, nir, &bt, /* num_render_targets */ 0,
                     struct brw_vue_map input_vue_map;
   brw_compute_tess_vue_map(&input_vue_map, key->inputs_read,
                     struct brw_compile_tes_params params = {
      .base = {
      .mem_ctx = mem_ctx,
   .nir = nir,
   .log_data = dbg,
      },
   .key = &brw_key,
   .prog_data = tes_prog_data,
               const unsigned *program = brw_compile_tes(compiler, &params);
   if (program == NULL) {
      dbg_printf("Failed to compile evaluation shader: %s\n", params.base.error_str);
            shader->compilation_failed = true;
                                          uint32_t *so_decls =
      screen->vtbl.create_so_decl_list(&ish->stream_output,
         iris_finalize_program(shader, prog_data, so_decls, system_values,
            iris_upload_shader(screen, ish, shader, NULL, uploader, IRIS_CACHE_TES,
                        }
      /**
   * Update the current tessellation evaluation shader variant.
   *
   * Fill out the key, look in the cache, compile and bind if needed.
   */
   static void
   iris_update_compiled_tes(struct iris_context *ice)
   {
      struct iris_screen *screen = (struct iris_screen *)ice->ctx.screen;
   struct u_upload_mgr *uploader = ice->shaders.uploader_driver;
   struct iris_shader_state *shs = &ice->state.shaders[MESA_SHADER_TESS_EVAL];
   struct iris_uncompiled_shader *ish =
            struct iris_tes_prog_key key = { KEY_INIT(vue.base) };
   get_unified_tess_slots(ice, &key.inputs_read, &key.patch_inputs_read);
            struct iris_compiled_shader *old = ice->shaders.prog[IRIS_CACHE_TES];
   bool added;
   struct iris_compiled_shader *shader =
            if (added && !iris_disk_cache_retrieve(screen, uploader, ish, shader,
                        if (shader->compilation_failed)
            if (old != shader) {
      iris_shader_variant_reference(&ice->shaders.prog[MESA_SHADER_TESS_EVAL],
         ice->state.stage_dirty |= IRIS_STAGE_DIRTY_TES |
                        unsigned urb_entry_size = shader ?
                     /* TODO: Could compare and avoid flagging this. */
   const struct shader_info *tes_info = &ish->nir->info;
   if (BITSET_TEST(tes_info->system_values_read, SYSTEM_VALUE_VERTICES_IN)) {
      ice->state.stage_dirty |= IRIS_STAGE_DIRTY_CONSTANTS_TES;
         }
      /**
   * Compile a geometry shader, and upload the assembly.
   */
   static void
   iris_compile_gs(struct iris_screen *screen,
                  struct u_upload_mgr *uploader,
      {
      const struct brw_compiler *compiler = screen->compiler;
   const struct intel_device_info *devinfo = screen->devinfo;
   void *mem_ctx = ralloc_context(NULL);
   struct brw_gs_prog_data *gs_prog_data =
         struct brw_vue_prog_data *vue_prog_data = &gs_prog_data->base;
   struct brw_stage_prog_data *prog_data = &vue_prog_data->base;
   enum brw_param_builtin *system_values;
   unsigned num_system_values;
            nir_shader *nir = nir_shader_clone(mem_ctx, ish->nir);
            if (key->vue.nr_userclip_plane_consts) {
      nir_function_impl *impl = nir_shader_get_entrypoint(nir);
   nir_lower_clip_gs(nir, (1 << key->vue.nr_userclip_plane_consts) - 1,
         nir_lower_io_to_temporaries(nir, impl, true, false);
   nir_lower_global_vars_to_local(nir);
   nir_lower_vars_to_ssa(nir);
               iris_setup_uniforms(devinfo, mem_ctx, nir, prog_data, 0, &system_values,
            struct iris_binding_table bt;
   iris_setup_binding_table(devinfo, nir, &bt, /* num_render_targets */ 0,
                     brw_compute_vue_map(devinfo,
                           struct brw_compile_gs_params params = {
      .base = {
      .mem_ctx = mem_ctx,
   .nir = nir,
   .log_data = dbg,
      },
   .key = &brw_key,
               const unsigned *program = brw_compile_gs(compiler, &params);
   if (program == NULL) {
      dbg_printf("Failed to compile geometry shader: %s\n", params.base.error_str);
            shader->compilation_failed = true;
                                          uint32_t *so_decls =
      screen->vtbl.create_so_decl_list(&ish->stream_output,
         iris_finalize_program(shader, prog_data, so_decls, system_values,
            iris_upload_shader(screen, ish, shader, NULL, uploader, IRIS_CACHE_GS,
                        }
      /**
   * Update the current geometry shader variant.
   *
   * Fill out the key, look in the cache, compile and bind if needed.
   */
   static void
   iris_update_compiled_gs(struct iris_context *ice)
   {
      struct iris_shader_state *shs = &ice->state.shaders[MESA_SHADER_GEOMETRY];
   struct u_upload_mgr *uploader = ice->shaders.uploader_driver;
   struct iris_uncompiled_shader *ish =
         struct iris_compiled_shader *old = ice->shaders.prog[IRIS_CACHE_GS];
   struct iris_compiled_shader *shader = NULL;
            if (ish) {
      struct iris_gs_prog_key key = { KEY_INIT(vue.base) };
                     shader = find_or_add_variant(screen, ish, IRIS_CACHE_GS, &key,
            if (added && !iris_disk_cache_retrieve(screen, uploader, ish, shader,
                        if (shader->compilation_failed)
               if (old != shader) {
      iris_shader_variant_reference(&ice->shaders.prog[MESA_SHADER_GEOMETRY],
         ice->state.stage_dirty |= IRIS_STAGE_DIRTY_GS |
                        unsigned urb_entry_size = shader ?
               }
      /**
   * Compile a fragment (pixel) shader, and upload the assembly.
   */
   static void
   iris_compile_fs(struct iris_screen *screen,
                  struct u_upload_mgr *uploader,
   struct util_debug_callback *dbg,
      {
      const struct brw_compiler *compiler = screen->compiler;
   void *mem_ctx = ralloc_context(NULL);
   struct brw_wm_prog_data *fs_prog_data =
         struct brw_stage_prog_data *prog_data = &fs_prog_data->base;
   enum brw_param_builtin *system_values;
   const struct intel_device_info *devinfo = screen->devinfo;
   unsigned num_system_values;
            nir_shader *nir = nir_shader_clone(mem_ctx, ish->nir);
                     iris_setup_uniforms(devinfo, mem_ctx, nir, prog_data, 0, &system_values,
            /* Lower output variables to load_output intrinsics before setting up
   * binding tables, so iris_setup_binding_table can map any load_output
   * intrinsics to IRIS_SURFACE_GROUP_RENDER_TARGET_READ on Gfx8 for
   * non-coherent framebuffer fetches.
   */
            /* On Gfx11+, shader RT write messages have a "Null Render Target" bit
   * and do not need a binding table entry with a null surface.  Earlier
   * generations need an entry for a null surface.
   */
            struct iris_binding_table bt;
   iris_setup_binding_table(devinfo, nir, &bt,
                                    struct brw_compile_fs_params params = {
      .base = {
      .mem_ctx = mem_ctx,
   .nir = nir,
   .log_data = dbg,
      },
   .key = &brw_key,
            .allow_spilling = true,
               const unsigned *program = brw_compile_fs(compiler, &params);
   if (program == NULL) {
      dbg_printf("Failed to compile fragment shader: %s\n", params.base.error_str);
            shader->compilation_failed = true;
                                          iris_finalize_program(shader, prog_data, NULL, system_values,
            iris_upload_shader(screen, ish, shader, NULL, uploader, IRIS_CACHE_FS,
                        }
      /**
   * Update the current fragment shader variant.
   *
   * Fill out the key, look in the cache, compile and bind if needed.
   */
   static void
   iris_update_compiled_fs(struct iris_context *ice)
   {
      struct iris_shader_state *shs = &ice->state.shaders[MESA_SHADER_FRAGMENT];
   struct u_upload_mgr *uploader = ice->shaders.uploader_driver;
   struct iris_uncompiled_shader *ish =
         struct iris_screen *screen = (struct iris_screen *)ice->ctx.screen;
   struct iris_fs_prog_key key = { KEY_INIT(base) };
            struct brw_vue_map *last_vue_map =
            if (ish->nos & (1ull << IRIS_NOS_LAST_VUE_MAP))
            struct iris_compiled_shader *old = ice->shaders.prog[IRIS_CACHE_FS];
   bool added;
   struct iris_compiled_shader *shader =
      find_or_add_variant(screen, ish, IRIS_CACHE_FS, &key,
         if (added && !iris_disk_cache_retrieve(screen, uploader, ish, shader,
                        if (shader->compilation_failed)
            if (old != shader) {
      // XXX: only need to flag CLIP if barycentric has NONPERSPECTIVE
   // toggles.  might be able to avoid flagging SBE too.
   iris_shader_variant_reference(&ice->shaders.prog[MESA_SHADER_FRAGMENT],
         ice->state.dirty |= IRIS_DIRTY_WM |
               ice->state.stage_dirty |= IRIS_STAGE_DIRTY_FS |
                     }
      /**
   * Update the last enabled stage's VUE map.
   *
   * When the shader feeding the rasterizer's output interface changes, we
   * need to re-emit various packets.
   */
   static void
   update_last_vue_map(struct iris_context *ice,
         {
      struct brw_vue_prog_data *vue_prog_data = (void *) shader->prog_data;
   struct brw_vue_map *vue_map = &vue_prog_data->vue_map;
   struct brw_vue_map *old_map = !ice->shaders.last_vue_shader ? NULL :
         const uint64_t changed_slots =
            if (changed_slots & VARYING_BIT_VIEWPORT) {
      ice->state.num_viewports =
         ice->state.dirty |= IRIS_DIRTY_CLIP |
                     ice->state.stage_dirty |= IRIS_STAGE_DIRTY_UNCOMPILED_FS |
               if (changed_slots || (old_map && old_map->separate != vue_map->separate)) {
                     }
      static void
   iris_update_pull_constant_descriptors(struct iris_context *ice,
         {
               if (!shader || !shader->prog_data->has_ubo_pull)
            struct iris_shader_state *shs = &ice->state.shaders[stage];
   bool any_new_descriptors =
                     while (bound_cbufs) {
      const int i = u_bit_scan(&bound_cbufs);
   struct pipe_shader_buffer *cbuf = &shs->constbuf[i];
   struct iris_state_ref *surf_state = &shs->constbuf_surf_state[i];
   if (!surf_state->res && cbuf->buffer) {
      iris_upload_ubo_ssbo_surf_state(ice, cbuf, surf_state,
                        if (any_new_descriptors)
      }
      /**
   * Update the current shader variants for the given state.
   *
   * This should be called on every draw call to ensure that the correct
   * shaders are bound.  It will also flag any dirty state triggered by
   * swapping out those shaders.
   */
   void
   iris_update_compiled_shaders(struct iris_context *ice)
   {
               if (stage_dirty & (IRIS_STAGE_DIRTY_UNCOMPILED_TCS |
            struct iris_uncompiled_shader *tes =
         if (tes) {
      iris_update_compiled_tcs(ice);
      } else {
      iris_shader_variant_reference(&ice->shaders.prog[MESA_SHADER_TESS_CTRL], NULL);
   iris_shader_variant_reference(&ice->shaders.prog[MESA_SHADER_TESS_EVAL], NULL);
   ice->state.stage_dirty |=
      IRIS_STAGE_DIRTY_TCS | IRIS_STAGE_DIRTY_TES |
               if (ice->shaders.urb.constrained)
                  if (stage_dirty & IRIS_STAGE_DIRTY_UNCOMPILED_VS)
         if (stage_dirty & IRIS_STAGE_DIRTY_UNCOMPILED_GS)
            if (stage_dirty & (IRIS_STAGE_DIRTY_UNCOMPILED_GS |
            const struct iris_compiled_shader *gs =
         const struct iris_compiled_shader *tes =
                     if (gs) {
      const struct brw_gs_prog_data *gs_prog_data = (void *) gs->prog_data;
   points_or_lines =
      gs_prog_data->output_topology == _3DPRIM_POINTLIST ||
   } else if (tes) {
      const struct brw_tes_prog_data *tes_data = (void *) tes->prog_data;
   points_or_lines =
      tes_data->output_topology == BRW_TESS_OUTPUT_TOPOLOGY_LINE ||
            if (ice->shaders.output_topology_is_points_or_lines != points_or_lines) {
      /* Outbound to XY Clip enables */
   ice->shaders.output_topology_is_points_or_lines = points_or_lines;
                  gl_shader_stage last_stage = last_vue_stage(ice);
   struct iris_compiled_shader *shader = ice->shaders.prog[last_stage];
   struct iris_uncompiled_shader *ish = ice->shaders.uncompiled[last_stage];
   update_last_vue_map(ice, shader);
   if (ice->state.streamout != shader->streamout) {
      ice->state.streamout = shader->streamout;
               if (ice->state.streamout_active) {
      for (int i = 0; i < PIPE_MAX_SO_BUFFERS; i++) {
      struct iris_stream_output_target *so =
         if (so)
                  if (stage_dirty & IRIS_STAGE_DIRTY_UNCOMPILED_FS)
            for (int i = MESA_SHADER_VERTEX; i <= MESA_SHADER_FRAGMENT; i++) {
      if (ice->state.stage_dirty & (IRIS_STAGE_DIRTY_CONSTANTS_VS << i))
         }
      static void
   iris_compile_cs(struct iris_screen *screen,
                  struct u_upload_mgr *uploader,
      {
      const struct brw_compiler *compiler = screen->compiler;
   void *mem_ctx = ralloc_context(NULL);
   struct brw_cs_prog_data *cs_prog_data =
         struct brw_stage_prog_data *prog_data = &cs_prog_data->base;
   enum brw_param_builtin *system_values;
   const struct intel_device_info *devinfo = screen->devinfo;
   unsigned num_system_values;
            nir_shader *nir = nir_shader_clone(mem_ctx, ish->nir);
                     iris_setup_uniforms(devinfo, mem_ctx, nir, prog_data,
                  struct iris_binding_table bt;
   iris_setup_binding_table(devinfo, nir, &bt, /* num_render_targets */ 0,
                     struct brw_compile_cs_params params = {
      .base = {
      .mem_ctx = mem_ctx,
   .nir = nir,
   .log_data = dbg,
      },
   .key = &brw_key,
               const unsigned *program = brw_compile_cs(compiler, &params);
   if (program == NULL) {
               shader->compilation_failed = true;
                                          iris_finalize_program(shader, prog_data, NULL, system_values,
                  iris_upload_shader(screen, ish, shader, NULL, uploader, IRIS_CACHE_CS,
                        }
      static void
   iris_update_compiled_cs(struct iris_context *ice)
   {
      struct iris_shader_state *shs = &ice->state.shaders[MESA_SHADER_COMPUTE];
   struct u_upload_mgr *uploader = ice->shaders.uploader_driver;
   struct iris_uncompiled_shader *ish =
         struct iris_screen *screen = (struct iris_screen *)ice->ctx.screen;
   struct iris_cs_prog_key key = { KEY_INIT(base) };
            struct iris_compiled_shader *old = ice->shaders.prog[IRIS_CACHE_CS];
   bool added;
   struct iris_compiled_shader *shader =
      find_or_add_variant(screen, ish, IRIS_CACHE_CS, &key,
         if (added && !iris_disk_cache_retrieve(screen, uploader, ish, shader,
                        if (shader->compilation_failed)
            if (old != shader) {
      iris_shader_variant_reference(&ice->shaders.prog[MESA_SHADER_COMPUTE],
         ice->state.stage_dirty |= IRIS_STAGE_DIRTY_CS |
                     }
      void
   iris_update_compiled_compute_shader(struct iris_context *ice)
   {
      if (ice->state.stage_dirty & IRIS_STAGE_DIRTY_UNCOMPILED_CS)
            if (ice->state.stage_dirty & IRIS_STAGE_DIRTY_CONSTANTS_CS)
      }
      void
   iris_fill_cs_push_const_buffer(struct brw_cs_prog_data *cs_prog_data,
               {
      assert(brw_cs_push_const_total_size(cs_prog_data, threads) > 0);
   assert(cs_prog_data->push.cross_thread.size == 0);
   assert(cs_prog_data->push.per_thread.dwords == 1);
   assert(cs_prog_data->base.param[0] == BRW_PARAM_BUILTIN_SUBGROUP_ID);
   for (unsigned t = 0; t < threads; t++)
      }
      /**
   * Allocate scratch BOs as needed for the given per-thread size and stage.
   */
   struct iris_bo *
   iris_get_scratch_space(struct iris_context *ice,
               {
      struct iris_screen *screen = (struct iris_screen *)ice->ctx.screen;
   struct iris_bufmgr *bufmgr = screen->bufmgr;
            unsigned encoded_size = ffs(per_thread_scratch) - 11;
   assert(encoded_size < ARRAY_SIZE(ice->shaders.scratch_bos));
            /* On GFX version 12.5, scratch access changed to a surface-based model.
   * Instead of each shader type having its own layout based on IDs passed
   * from the relevant fixed-function unit, all scratch access is based on
   * thread IDs like it always has been for compute.
   */
   if (devinfo->verx10 >= 125)
                     if (!*bop) {
      assert(stage < ARRAY_SIZE(devinfo->max_scratch_ids));
   uint32_t size = per_thread_scratch * devinfo->max_scratch_ids[stage];
   *bop = iris_bo_alloc(bufmgr, "scratch", size, 1024,
                  }
      const struct iris_state_ref *
   iris_get_scratch_surf(struct iris_context *ice,
         {
      struct iris_screen *screen = (struct iris_screen *)ice->ctx.screen;
                     unsigned encoded_size = ffs(per_thread_scratch) - 11;
   assert(encoded_size < ARRAY_SIZE(ice->shaders.scratch_surfs));
                     if (ref->res)
            struct iris_bo *scratch_bo =
            void *map = upload_state(ice->state.scratch_surface_uploader, ref,
            isl_buffer_fill_state(&screen->isl_dev, map,
                        .address = scratch_bo->address,
   .size_B = scratch_bo->size,
   .format = ISL_FORMAT_RAW,
            }
      /* ------------------------------------------------------------------- */
      /**
   * The pipe->create_[stage]_state() driver hooks.
   *
   * Performs basic NIR preprocessing, records any state dependencies, and
   * returns an iris_uncompiled_shader as the Gallium CSO.
   *
   * Actual shader compilation to assembly happens later, at first use.
   */
   static void *
   iris_create_uncompiled_shader(struct iris_screen *screen,
               {
      struct iris_uncompiled_shader *ish =
         if (!ish)
            pipe_reference_init(&ish->ref, 1);
   list_inithead(&ish->variants);
   simple_mtx_init(&ish->lock, mtx_plain);
                     ish->program_id = get_new_program_id(screen);
   ish->nir = nir;
   if (so_info) {
      memcpy(&ish->stream_output, so_info, sizeof(*so_info));
               /* Use lowest dword of source shader sha1 for shader hash. */
            if (screen->disk_cache) {
      /* Serialize the NIR to a binary blob that we can hash for the disk
   * cache.  Drop unnecessary information (like variable names)
   * so the serialized NIR is smaller, and also to let us detect more
   * isomorphic shaders when hashing, increasing cache hits.
   */
   struct blob blob;
   blob_init(&blob);
   nir_serialize(&blob, nir, true);
   _mesa_sha1_compute(blob.data, blob.size, ish->nir_sha1);
                  }
      static void *
   iris_create_compute_state(struct pipe_context *ctx,
         {
      struct iris_context *ice = (void *) ctx;
   struct iris_screen *screen = (void *) ctx->screen;
   struct u_upload_mgr *uploader = ice->shaders.uploader_unsync;
   const nir_shader_compiler_options *options =
            nir_shader *nir;
   switch (state->ir_type) {
   case PIPE_SHADER_IR_NIR:
      nir = (void *)state->prog;
         case PIPE_SHADER_IR_NIR_SERIALIZED: {
      struct blob_reader reader;
   const struct pipe_binary_program_header *hdr = state->prog;
   blob_reader_init(&reader, hdr->blob, hdr->num_bytes);
   nir = nir_deserialize(NULL, options, &reader);
               default:
                  /* Most of iris doesn't really care about the difference between compute
   * shaders and kernels.  We also tend to hard-code COMPUTE everywhere so
   * it's way easier if we just normalize to COMPUTE here.
   */
   assert(nir->info.stage == MESA_SHADER_COMPUTE ||
                  struct iris_uncompiled_shader *ish =
         ish->kernel_input_size = state->req_input_mem;
                     if (screen->precompile) {
               struct iris_compiled_shader *shader =
                  /* Append our new variant to the shader's variant list. */
            if (!iris_disk_cache_retrieve(screen, uploader, ish, shader,
                              }
      static void
   iris_get_compute_state_info(struct pipe_context *ctx, void *state,
         {
      struct iris_screen *screen = (void *) ctx->screen;
            info->max_threads = MIN2(1024, 32 * screen->devinfo->max_cs_workgroup_threads);
   info->private_memory = 0;
   info->preferred_simd_size = 32;
            list_for_each_entry_safe(struct iris_compiled_shader, shader,
            info->private_memory = MAX2(info->private_memory,
         }
      static uint32_t
   iris_get_compute_state_subgroup_size(struct pipe_context *ctx, void *state,
         {
      struct iris_context *ice = (void *) ctx;
   struct iris_screen *screen = (void *) ctx->screen;
   struct u_upload_mgr *uploader = ice->shaders.uploader_driver;
   const struct intel_device_info *devinfo = screen->devinfo;
            struct iris_cs_prog_key key = { KEY_INIT(base) };
            bool added;
   struct iris_compiled_shader *shader =
      find_or_add_variant(screen, ish, IRIS_CACHE_CS, &key,
         if (added && !iris_disk_cache_retrieve(screen, uploader, ish, shader,
                                    }
      static void
   iris_compile_shader(void *_job, UNUSED void *_gdata, UNUSED int thread_index)
   {
      const struct iris_threaded_compile_job *job =
            struct iris_screen *screen = job->screen;
   struct u_upload_mgr *uploader = job->uploader;
   struct util_debug_callback *dbg = job->dbg;
   struct iris_uncompiled_shader *ish = job->ish;
            switch (ish->nir->info.stage) {
   case MESA_SHADER_VERTEX:
      iris_compile_vs(screen, uploader, dbg, ish, shader);
      case MESA_SHADER_TESS_CTRL:
      iris_compile_tcs(screen, NULL, uploader, dbg, ish, shader);
      case MESA_SHADER_TESS_EVAL:
      iris_compile_tes(screen, uploader, dbg, ish, shader);
      case MESA_SHADER_GEOMETRY:
      iris_compile_gs(screen, uploader, dbg, ish, shader);
      case MESA_SHADER_FRAGMENT:
      iris_compile_fs(screen, uploader, dbg, ish, shader, NULL);
         default:
            }
      static void *
   iris_create_shader_state(struct pipe_context *ctx,
         {
      struct iris_context *ice = (void *) ctx;
   struct iris_screen *screen = (void *) ctx->screen;
            if (state->type == PIPE_SHADER_IR_TGSI)
         else
            const struct shader_info *const info = &nir->info;
   struct iris_uncompiled_shader *ish =
            union iris_any_prog_key key;
                     switch (info->stage) {
   case MESA_SHADER_VERTEX:
      /* User clip planes */
   if (info->clip_distance_array_size == 0)
            key.vs = (struct iris_vs_prog_key) { KEY_INIT(vue.base) };
   key_size = sizeof(key.vs);
         case MESA_SHADER_TESS_CTRL: {
      key.tcs = (struct iris_tcs_prog_key) {
      KEY_INIT(vue.base),
   // XXX: make sure the linker fills this out from the TES...
   ._tes_primitive_mode =
   info->tess._primitive_mode ? info->tess._primitive_mode
         .outputs_written = info->outputs_written,
               /* MULTI_PATCH mode needs the key to contain the input patch dimensionality.
   * We don't have that information, so we randomly guess that the input
   * and output patches are the same size.  This is a bad guess, but we
   * can't do much better.
   */
   if (screen->compiler->use_tcs_multi_patch)
            key_size = sizeof(key.tcs);
               case MESA_SHADER_TESS_EVAL:
      /* User clip planes */
   if (info->clip_distance_array_size == 0)
            key.tes = (struct iris_tes_prog_key) {
      KEY_INIT(vue.base),
   // XXX: not ideal, need TCS output/TES input unification
   .inputs_read = info->inputs_read,
               key_size = sizeof(key.tes);
         case MESA_SHADER_GEOMETRY:
      /* User clip planes */
   if (info->clip_distance_array_size == 0)
            key.gs = (struct iris_gs_prog_key) { KEY_INIT(vue.base) };
   key_size = sizeof(key.gs);
         case MESA_SHADER_FRAGMENT:
      ish->nos |= (1ull << IRIS_NOS_FRAMEBUFFER) |
                        /* The program key needs the VUE map if there are > 16 inputs */
   if (util_bitcount64(info->inputs_read & BRW_FS_VARYING_INPUT_MASK) > 16) {
                  const uint64_t color_outputs = info->outputs_written &
      ~(BITFIELD64_BIT(FRAG_RESULT_DEPTH) |
               bool can_rearrange_varyings =
                     key.fs = (struct iris_fs_prog_key) {
      KEY_INIT(base),
   .nr_color_regions = util_bitcount(color_outputs),
   .coherent_fb_fetch = devinfo->ver >= 9,
   .input_slots_valid =
               key_size = sizeof(key.fs);
         default:
                  if (screen->precompile) {
               struct iris_compiled_shader *shader =
      iris_create_shader_variant(screen, NULL,
               /* Append our new variant to the shader's variant list. */
            if (!iris_disk_cache_retrieve(screen, uploader, ish, shader,
                              job->screen = screen;
   job->uploader = uploader;
                  iris_schedule_compile(screen, &ish->ready, &ice->dbg, job,
                     }
      /**
   * Called when the refcount on the iris_uncompiled_shader reaches 0.
   *
   * Frees the iris_uncompiled_shader.
   *
   * \sa iris_delete_shader_state
   */
   void
   iris_destroy_shader_state(struct pipe_context *ctx, void *state)
   {
               /* No need to take ish->lock; we hold the last reference to ish */
   list_for_each_entry_safe(struct iris_compiled_shader, shader,
                                 simple_mtx_destroy(&ish->lock);
            ralloc_free(ish->nir);
      }
      /**
   * The pipe->delete_[stage]_state() driver hooks.
   *
   * \sa iris_destroy_shader_state
   */
   static void
   iris_delete_shader_state(struct pipe_context *ctx, void *state)
   {
      struct iris_uncompiled_shader *ish = state;
                     if (ice->shaders.uncompiled[stage] == ish) {
      ice->shaders.uncompiled[stage] = NULL;
               if (pipe_reference(&ish->ref, NULL))
      }
      /**
   * The pipe->bind_[stage]_state() driver hook.
   *
   * Binds an uncompiled shader as the current one for a particular stage.
   * Updates dirty tracking to account for the shader's NOS.
   */
   static void
   bind_shader_state(struct iris_context *ice,
               {
      uint64_t stage_dirty_bit = IRIS_STAGE_DIRTY_UNCOMPILED_VS << stage;
            const struct shader_info *old_info = iris_get_shader_info(ice, stage);
            if ((old_info ? BITSET_LAST_BIT(old_info->samplers_used) : 0) !=
      (new_info ? BITSET_LAST_BIT(new_info->samplers_used) : 0)) {
               ice->shaders.uncompiled[stage] = ish;
            /* Record that CSOs need to mark IRIS_DIRTY_UNCOMPILED_XS when they change
   * (or that they no longer need to do so).
   */
   for (int i = 0; i < IRIS_NOS_COUNT; i++) {
      if (nos & (1 << i))
         else
         }
      static void
   iris_bind_vs_state(struct pipe_context *ctx, void *state)
   {
      struct iris_context *ice = (struct iris_context *)ctx;
            if (ish) {
      const struct shader_info *info = &ish->nir->info;
   if (ice->state.window_space_position != info->vs.window_space_position) {
               ice->state.dirty |= IRIS_DIRTY_CLIP |
                     const bool uses_draw_params =
      BITSET_TEST(info->system_values_read, SYSTEM_VALUE_FIRST_VERTEX) ||
      const bool uses_derived_draw_params =
      BITSET_TEST(info->system_values_read, SYSTEM_VALUE_DRAW_ID) ||
      const bool needs_sgvs_element = uses_draw_params ||
      BITSET_TEST(info->system_values_read, SYSTEM_VALUE_INSTANCE_ID) ||
               if (ice->state.vs_uses_draw_params != uses_draw_params ||
      ice->state.vs_uses_derived_draw_params != uses_derived_draw_params ||
   ice->state.vs_needs_edge_flag != info->vs.needs_edge_flag ||
   ice->state.vs_needs_sgvs_element != needs_sgvs_element) {
   ice->state.dirty |= IRIS_DIRTY_VERTEX_BUFFERS |
               ice->state.vs_uses_draw_params = uses_draw_params;
   ice->state.vs_uses_derived_draw_params = uses_derived_draw_params;
   ice->state.vs_needs_sgvs_element = needs_sgvs_element;
                  }
      static void
   iris_bind_tcs_state(struct pipe_context *ctx, void *state)
   {
         }
      static void
   iris_bind_tes_state(struct pipe_context *ctx, void *state)
   {
      struct iris_context *ice = (struct iris_context *)ctx;
   struct iris_screen *screen = (struct iris_screen *) ctx->screen;
            /* Enabling/disabling optional stages requires a URB reconfiguration. */
   if (!!state != !!ice->shaders.uncompiled[MESA_SHADER_TESS_EVAL])
      ice->state.dirty |= IRIS_DIRTY_URB | (devinfo->verx10 >= 125 ?
            }
      static void
   iris_bind_gs_state(struct pipe_context *ctx, void *state)
   {
               /* Enabling/disabling optional stages requires a URB reconfiguration. */
   if (!!state != !!ice->shaders.uncompiled[MESA_SHADER_GEOMETRY])
               }
      static void
   iris_bind_fs_state(struct pipe_context *ctx, void *state)
   {
      struct iris_context *ice = (struct iris_context *) ctx;
   struct iris_screen *screen = (struct iris_screen *) ctx->screen;
   const struct intel_device_info *devinfo = screen->devinfo;
   struct iris_uncompiled_shader *old_ish =
                  const unsigned color_bits =
      BITFIELD64_BIT(FRAG_RESULT_COLOR) |
         /* Fragment shader outputs influence HasWriteableRT */
   if (!old_ish || !new_ish ||
      (old_ish->nir->info.outputs_written & color_bits) !=
   (new_ish->nir->info.outputs_written & color_bits))
         if (devinfo->ver == 8)
               }
      static void
   iris_bind_cs_state(struct pipe_context *ctx, void *state)
   {
         }
      static char *
   iris_finalize_nir(struct pipe_screen *_screen, void *nirptr)
   {
      struct iris_screen *screen = (struct iris_screen *)_screen;
   struct nir_shader *nir = (struct nir_shader *) nirptr;
                     struct brw_nir_compiler_opts opts = {};
            NIR_PASS_V(nir, brw_nir_lower_storage_image,
            &(struct brw_nir_lower_storage_image_opts) {
      .devinfo = devinfo,
   .lower_loads = true,
   .lower_stores = true,
   .lower_atomics = true,
                        }
      static void
   iris_set_max_shader_compiler_threads(struct pipe_screen *pscreen,
         {
      struct iris_screen *screen = (struct iris_screen *) pscreen;
   util_queue_adjust_num_threads(&screen->shader_compiler_queue, max_threads,
      }
      static bool
   iris_is_parallel_shader_compilation_finished(struct pipe_screen *pscreen,
               {
               /* Threaded compilation is only used for the precompile.  If precompile is
   * disabled, threaded compilation is "done."
   */
   if (!screen->precompile)
                     /* When precompile is enabled, the first entry is the precompile variant.
   * Check the ready fence of the precompile variant.
   */
   struct iris_compiled_shader *first =
               }
      void
   iris_init_screen_program_functions(struct pipe_screen *pscreen)
   {
      pscreen->is_parallel_shader_compilation_finished =
         pscreen->set_max_shader_compiler_threads =
            }
      void
   iris_init_program_functions(struct pipe_context *ctx)
   {
      ctx->create_vs_state  = iris_create_shader_state;
   ctx->create_tcs_state = iris_create_shader_state;
   ctx->create_tes_state = iris_create_shader_state;
   ctx->create_gs_state  = iris_create_shader_state;
   ctx->create_fs_state  = iris_create_shader_state;
            ctx->delete_vs_state  = iris_delete_shader_state;
   ctx->delete_tcs_state = iris_delete_shader_state;
   ctx->delete_tes_state = iris_delete_shader_state;
   ctx->delete_gs_state  = iris_delete_shader_state;
   ctx->delete_fs_state  = iris_delete_shader_state;
            ctx->bind_vs_state  = iris_bind_vs_state;
   ctx->bind_tcs_state = iris_bind_tcs_state;
   ctx->bind_tes_state = iris_bind_tes_state;
   ctx->bind_gs_state  = iris_bind_gs_state;
   ctx->bind_fs_state  = iris_bind_fs_state;
            ctx->get_compute_state_info = iris_get_compute_state_info;
      }
