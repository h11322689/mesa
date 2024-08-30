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
   * @file crocus_program.c
   *
   * This file contains the driver interface for compiling shaders.
   *
   * See crocus_program_cache.c for the in-memory program cache where the
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
   #include "util/u_prim.h"
   #include "compiler/nir/nir.h"
   #include "compiler/nir/nir_builder.h"
   #include "compiler/nir/nir_serialize.h"
   #include "intel/compiler/brw_compiler.h"
   #include "intel/compiler/brw_nir.h"
   #include "intel/compiler/brw_prim.h"
   #include "crocus_context.h"
   #include "nir/tgsi_to_nir.h"
   #include "program/prog_instruction.h"
      #define KEY_INIT_NO_ID()                              \
         #define KEY_INIT()                                                        \
      .base.program_string_id = ish->program_id,                             \
   .base.limit_trig_input_range = screen->driconf.limit_trig_input_range, \
         static void
   crocus_sanitize_tex_key(struct brw_sampler_prog_key_data *key)
   {
      key->gather_channel_quirk_mask = 0;
   for (unsigned s = 0; s < BRW_MAX_SAMPLERS; s++) {
      key->swizzles[s] = SWIZZLE_NOOP;
         }
      static uint32_t
   crocus_get_texture_swizzle(const struct crocus_context *ice,
         {
               for (int i = 0; i < 4; i++) {
         }
      }
      static inline bool can_push_ubo(const struct intel_device_info *devinfo)
   {
      /* push works for everyone except SNB at the moment */
      }
      static uint8_t
   gfx6_gather_workaround(enum pipe_format pformat)
   {
      switch (pformat) {
   case PIPE_FORMAT_R8_SINT: return WA_SIGN | WA_8BIT;
   case PIPE_FORMAT_R8_UINT: return WA_8BIT;
   case PIPE_FORMAT_R16_SINT: return WA_SIGN | WA_16BIT;
   case PIPE_FORMAT_R16_UINT: return WA_16BIT;
   default:
      /* Note that even though PIPE_FORMAT_R32_SINT and
   * PIPE_FORMAT_R32_UINThave format overrides in
   * the surface state, there is no shader w/a required.
   */
         }
      static const unsigned crocus_gfx6_swizzle_for_offset[4] = {
      BRW_SWIZZLE4(0, 1, 2, 3),
   BRW_SWIZZLE4(1, 2, 3, 3),
   BRW_SWIZZLE4(2, 3, 3, 3),
      };
      static void
   gfx6_gs_xfb_setup(const struct pipe_stream_output_info *so_info,
         {
      /* Make sure that the VUE slots won't overflow the unsigned chars in
   * prog_data->transform_feedback_bindings[].
   */
            /* Make sure that we don't need more binding table entries than we've
   * set aside for use in transform feedback.  (We shouldn't, since we
   * set aside enough binding table entries to have one per component).
   */
            gs_prog_data->num_transform_feedback_bindings = so_info->num_outputs;
   for (unsigned i = 0; i < so_info->num_outputs; i++) {
      gs_prog_data->transform_feedback_bindings[i] =
         gs_prog_data->transform_feedback_swizzles[i] =
         }
      static void
   gfx6_ff_gs_xfb_setup(const struct pipe_stream_output_info *so_info,
         {
      key->num_transform_feedback_bindings = so_info->num_outputs;
   for (unsigned i = 0; i < so_info->num_outputs; i++) {
      key->transform_feedback_bindings[i] =
         key->transform_feedback_swizzles[i] =
         }
      static void
   crocus_populate_sampler_prog_key_data(struct crocus_context *ice,
                                 {
      struct crocus_screen *screen = (struct crocus_screen *)ice->ctx.screen;
            while (mask) {
               struct crocus_sampler_view *texture = ice->state.shaders[stage].textures[s];
            if (!texture)
         if (texture->base.target == PIPE_BUFFER)
         if (devinfo->verx10 < 75) {
                           /* gather4 for RG32* is broken in multiple ways on Gen7. */
   if (devinfo->ver == 7 && uses_texture_gather) {
      switch (texture->base.format) {
   case PIPE_FORMAT_R32G32_UINT:
   case PIPE_FORMAT_R32G32_SINT: {
      /* We have to override the format to R32G32_FLOAT_LD.
   * This means that SCS_ALPHA and SCS_ONE will return 0x3f8
   * (1.0) rather than integer 1.  This needs shader hacks.
   *
   * On Ivybridge, we whack W (alpha) to ONE in our key's
   * swizzle.  On Haswell, we look at the original texture
   * swizzle, and use XYZW with channels overridden to ONE,
   * leaving normal texture swizzling to SCS.
   */
   unsigned src_swizzle = key->swizzles[s];
   for (int i = 0; i < 4; i++) {
      unsigned src_comp = GET_SWZ(src_swizzle, i);
   if (src_comp == SWIZZLE_ONE || src_comp == SWIZZLE_W) {
      key->swizzles[i] &= ~(0x7 << (3 * i));
            }
   FALLTHROUGH;
   case PIPE_FORMAT_R32G32_FLOAT:
      /* The channel select for green doesn't work - we have to
   * request blue.  Haswell can use SCS for this, but Ivybridge
   * needs a shader workaround.
   */
   if (devinfo->verx10 < 75)
            default:
            }
   if (devinfo->ver == 6 && uses_texture_gather) {
               }
      static void
   crocus_lower_swizzles(struct nir_shader *nir,
         {
      struct nir_lower_tex_options tex_options = {
         };
            while (mask) {
               if (key_tex->swizzles[s] == SWIZZLE_NOOP)
            tex_options.swizzle_result |= (1 << s);
   for (unsigned c = 0; c < 4; c++)
      }
   if (tex_options.swizzle_result)
      }
      static unsigned
   get_new_program_id(struct crocus_screen *screen)
   {
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
   crocus_lower_storage_image_derefs(nir_shader *nir)
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
      // XXX: need unify_interfaces() at link time...
      /**
   * Undo nir_lower_passthrough_edgeflags but keep the inputs_read flag.
   */
   static bool
   crocus_fix_edge_flags(nir_shader *nir)
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
   crocus_setup_uniforms(ASSERTED const struct intel_device_info *devinfo,
                        void *mem_ctx,
   nir_shader *nir,
      {
      const unsigned CROCUS_MAX_SYSTEM_VALUES =
         enum brw_param_builtin *system_values =
                  unsigned patch_vert_idx = -1;
   unsigned tess_outer_default_idx = -1;
   unsigned tess_inner_default_idx = -1;
   unsigned ucp_idx[CROCUS_MAX_CLIP_PLANES];
   unsigned img_idx[PIPE_MAX_SHADER_IMAGES];
   unsigned variable_group_size_idx = -1;
   memset(ucp_idx, -1, sizeof(ucp_idx));
                              nir_def *temp_ubo_name = nir_undef(&b, 1, 32);
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
      /* This one is special because it reads from the shader constant
   * data and not cbuf0 which gallium uploads for us.
   */
   b.cursor = nir_before_instr(instr);
   nir_def *offset =
                                 nir_intrinsic_instr *load_ubo =
         load_ubo->num_components = intrin->num_components;
   load_ubo->src[0] = nir_src_for_ssa(temp_const_ubo_name);
   load_ubo->src[1] = nir_src_for_ssa(offset);
   nir_intrinsic_set_align(load_ubo, 4, 0);
   nir_intrinsic_set_range_base(load_ubo, 0);
   nir_intrinsic_set_range(load_ubo, ~0);
   nir_def_init(&load_ubo->instr, &load_ubo->def,
                        nir_def_rewrite_uses(&intrin->def,
         nir_instr_remove(&intrin->instr);
      }
                     if (ucp_idx[ucp] == -1) {
                        for (int i = 0; i < 4; i++) {
                        b.cursor = nir_before_instr(instr);
   offset = nir_imm_int(&b, ucp_idx[ucp] * sizeof(uint32_t));
      }
   case nir_intrinsic_load_patch_vertices_in:
                                    b.cursor = nir_before_instr(instr);
   offset = nir_imm_int(&b, patch_vert_idx * sizeof(uint32_t));
      case nir_intrinsic_load_tess_level_outer_default:
      if (tess_outer_default_idx == -1) {
                        for (int i = 0; i < 4; i++) {
                        b.cursor = nir_before_instr(instr);
   offset =
            case nir_intrinsic_load_tess_level_inner_default:
      if (tess_inner_default_idx == -1) {
                        for (int i = 0; i < 2; i++) {
                        b.cursor = nir_before_instr(instr);
   offset =
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
                        }
   case nir_intrinsic_load_workgroup_size: {
      assert(nir->info.workgroup_size_variable);
   if (variable_group_size_idx == -1) {
      variable_group_size_idx = num_system_values;
   num_system_values += 3;
   for (int i = 0; i < 3; i++) {
      system_values[variable_group_size_idx + i] =
                  b.cursor = nir_before_instr(instr);
   offset = nir_imm_int(&b,
            }
   default:
                           nir_intrinsic_instr *load =
         load->num_components = comps;
   load->src[0] = nir_src_for_ssa(temp_ubo_name);
   load->src[1] = nir_src_for_ssa(offset);
   nir_intrinsic_set_align(load, 4, 0);
   nir_intrinsic_set_range_base(load, 0);
   nir_intrinsic_set_range(load, ~0);
   nir_def_init(&load->instr, &load->def, comps, 32);
   nir_builder_instr_insert(&b, &load->instr);
   nir_def_rewrite_uses(&intrin->def,
                                 /* Uniforms are stored in constant buffer 0, the
   * user-facing UBOs are indexed by one.  So if any constant buffer is
   * needed, the constant buffer 0 will be needed, so account for it.
   */
   unsigned num_cbufs = nir->info.num_ubos;
   if (num_cbufs || nir->num_uniforms)
            /* Place the new params in a new cbuf. */
   if (num_system_values > 0) {
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
            /* Constant loads (if any) need to go at the end of the constant buffers so
   * we need to know num_cbufs before we can lower to them.
   */
   if (temp_const_ubo_name != NULL) {
      nir_load_const_instr *const_ubo_index =
         assert(const_ubo_index->def.bit_size == 32);
               *out_system_values = system_values;
   *out_num_system_values = num_system_values;
      }
      static const char *surface_group_names[] = {
      [CROCUS_SURFACE_GROUP_RENDER_TARGET]      = "render target",
   [CROCUS_SURFACE_GROUP_RENDER_TARGET_READ] = "non-coherent render target read",
   [CROCUS_SURFACE_GROUP_SOL]                = "streamout",
   [CROCUS_SURFACE_GROUP_CS_WORK_GROUPS]     = "CS work groups",
   [CROCUS_SURFACE_GROUP_TEXTURE]            = "texture",
   [CROCUS_SURFACE_GROUP_TEXTURE_GATHER]     = "texture gather",
   [CROCUS_SURFACE_GROUP_UBO]                = "ubo",
   [CROCUS_SURFACE_GROUP_SSBO]               = "ssbo",
      };
      static void
   crocus_print_binding_table(FILE *fp, const char *name,
         {
               uint32_t total = 0;
            for (int i = 0; i < CROCUS_SURFACE_GROUP_COUNT; i++) {
      uint32_t size = bt->sizes[i];
   total += size;
   if (size)
               if (total == 0) {
      fprintf(fp, "Binding table for %s is empty\n\n", name);
               if (total != compacted) {
      fprintf(fp, "Binding table for %s "
            } else {
                  uint32_t entry = 0;
   for (int i = 0; i < CROCUS_SURFACE_GROUP_COUNT; i++) {
      uint64_t mask = bt->used_mask[i];
   while (mask) {
      int index = u_bit_scan64(&mask);
         }
      }
      enum {
      /* Max elements in a surface group. */
      };
      static void
   rewrite_src_with_bti(nir_builder *b, struct crocus_binding_table *bt,
               {
               b->cursor = nir_before_instr(instr);
   nir_def *bti;
   if (nir_src_is_const(*src)) {
      uint32_t index = nir_src_as_uint(*src);
   bti = nir_imm_intN_t(b, crocus_group_index_to_bti(bt, group, index),
      } else {
      /* Indirect usage makes all the surfaces of the group to be available,
   * so we can just add the base.
   */
   assert(bt->used_mask[group] == BITFIELD64_MASK(bt->sizes[group]));
      }
      }
      static void
   mark_used_with_src(struct crocus_binding_table *bt, nir_src *src,
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
   crocus_setup_binding_table(const struct intel_device_info *devinfo,
                              struct nir_shader *nir,
      {
                        /* Set the sizes for each surface group.  For some groups, we already know
   * upfront how many will be used, so mark them.
   */
   if (info->stage == MESA_SHADER_FRAGMENT) {
      bt->sizes[CROCUS_SURFACE_GROUP_RENDER_TARGET] = num_render_targets;
   /* All render targets used. */
   bt->used_mask[CROCUS_SURFACE_GROUP_RENDER_TARGET] =
            /* Setup render target read surface group in order to support non-coherent
   * framebuffer fetch on Gfx7
   */
   if (devinfo->ver >= 6 && info->outputs_read) {
      bt->sizes[CROCUS_SURFACE_GROUP_RENDER_TARGET_READ] = num_render_targets;
   bt->used_mask[CROCUS_SURFACE_GROUP_RENDER_TARGET_READ] =
         } else if (info->stage == MESA_SHADER_COMPUTE) {
         } else if (info->stage == MESA_SHADER_GEOMETRY) {
      /* In gfx6 we reserve the first BRW_MAX_SOL_BINDINGS entries for transform
   * feedback surfaces.
   */
   if (devinfo->ver == 6) {
      bt->sizes[CROCUS_SURFACE_GROUP_SOL] = BRW_MAX_SOL_BINDINGS;
                  bt->sizes[CROCUS_SURFACE_GROUP_TEXTURE] = BITSET_LAST_BIT(info->textures_used);
            if (info->uses_texture_gather && devinfo->ver < 8) {
      bt->sizes[CROCUS_SURFACE_GROUP_TEXTURE_GATHER] = BITSET_LAST_BIT(info->textures_used);
                        /* Allocate an extra slot in the UBO section for NIR constants.
   * Binding table compaction will remove it if unnecessary.
   *
   * We don't include them in crocus_compiled_shader::num_cbufs because
   * they are uploaded separately from shs->constbufs[], but from a shader
   * point of view, they're another UBO (at the end of the section).
   */
                     for (int i = 0; i < CROCUS_SURFACE_GROUP_COUNT; i++)
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
      if (devinfo->ver >= 6) {
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
      for (int i = 0; i < CROCUS_SURFACE_GROUP_COUNT; i++)
               /* Calculate the offsets and the binding table size based on the used
   * surfaces.  After this point, the functions to go between "group indices"
   * and binding table indices can be used.
   */
   uint32_t next = 0;
   for (int i = 0; i < CROCUS_SURFACE_GROUP_COUNT; i++) {
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
                     /* rewrite the tg4 component from green to blue before replacing the
         if (devinfo->verx10 == 70) {
      if (tex->component == 1)
                     if (is_gather && devinfo->ver == 6 && key->gfx6_gather_wa[tex->texture_index]) {
                           nir_def *val = nir_fmul_imm(&b, &tex->def, (1 << width) - 1);
   val = nir_f2u32(&b, val);
   if (wa & WA_SIGN) {
      val = nir_ishl_imm(&b, val, 32 - width);
                        tex->texture_index =
      crocus_group_index_to_bti(bt, is_gather ? CROCUS_SURFACE_GROUP_TEXTURE_GATHER : CROCUS_SURFACE_GROUP_TEXTURE,
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
      if (devinfo->ver >= 6) {
      rewrite_src_with_bti(&b, bt, instr, &intrin->src[0],
                  case nir_intrinsic_get_ssbo_size:
   case nir_intrinsic_ssbo_atomic:
   case nir_intrinsic_ssbo_atomic_swap:
   case nir_intrinsic_load_ssbo:
      rewrite_src_with_bti(&b, bt, instr, &intrin->src[0],
               default:
                  }
      static void
   crocus_debug_recompile(struct crocus_context *ice,
               {
      struct crocus_screen *screen = (struct crocus_screen *) ice->ctx.screen;
            if (!info)
            brw_shader_perf_log(c, &ice->dbg, "Recompiling %s shader for program %s: %s\n",
                        const void *old_key =
               }
      /**
   * Get the shader for the last enabled geometry stage.
   *
   * This stage is the one which will feed stream output and the rasterizer.
   */
   static gl_shader_stage
   last_vue_stage(struct crocus_context *ice)
   {
      if (ice->shaders.uncompiled[MESA_SHADER_GEOMETRY])
            if (ice->shaders.uncompiled[MESA_SHADER_TESS_EVAL])
               }
      static GLbitfield64
   crocus_vs_outputs_written(struct crocus_context *ice,
               {
      struct crocus_screen *screen = (struct crocus_screen *)ice->ctx.screen;
   const struct intel_device_info *devinfo = &screen->devinfo;
                        if (key->copy_edgeflag)
            /* Put dummy slots into the VUE for the SF to put the replaced
   * point sprite coords in.  We shouldn't need these dummy slots,
   * which take up precious URB space, but it would mean that the SF
   * doesn't get nice aligned pairs of input coords into output
   * coords, which would be a pain to handle.
   */
   for (unsigned i = 0; i < 8; i++) {
      if (key->point_coord_replace & (1 << i))
               /* if back colors are written, allocate slots for front colors too */
   if (outputs_written & BITFIELD64_BIT(VARYING_SLOT_BFC0))
         if (outputs_written & BITFIELD64_BIT(VARYING_SLOT_BFC1))
               /* In order for legacy clipping to work, we need to populate the clip
   * distance varying slots whenever clipping is enabled, even if the vertex
   * shader doesn't write to gl_ClipDistance.
   */
   if (key->nr_userclip_plane_consts > 0) {
      outputs_written |= BITFIELD64_BIT(VARYING_SLOT_CLIP_DIST0);
                  }
      /*
   * If no edgeflags come from the user, gen4/5
   * require giving the clip shader a default edgeflag.
   *
   * This will always be 1.0.
   */
   static void
   crocus_lower_default_edgeflags(struct nir_shader *nir)
   {
                        nir_variable *var = nir_variable_create(nir, nir_var_shader_out,
               var->data.location = VARYING_SLOT_EDGE;
      }
      /**
   * Compile a vertex shader, and upload the assembly.
   */
   static struct crocus_compiled_shader *
   crocus_compile_vs(struct crocus_context *ice,
               {
      struct crocus_screen *screen = (struct crocus_screen *)ice->ctx.screen;
   const struct brw_compiler *compiler = screen->compiler;
   const struct intel_device_info *devinfo = &screen->devinfo;
   void *mem_ctx = ralloc_context(NULL);
   struct brw_vs_prog_data *vs_prog_data =
         struct brw_vue_prog_data *vue_prog_data = &vs_prog_data->base;
   struct brw_stage_prog_data *prog_data = &vue_prog_data->base;
   enum brw_param_builtin *system_values;
   unsigned num_system_values;
                     if (key->nr_userclip_plane_consts) {
      nir_function_impl *impl = nir_shader_get_entrypoint(nir);
   /* Check if variables were found. */
   if (nir_lower_clip_vs(nir, (1 << key->nr_userclip_plane_consts) - 1,
            nir_lower_io_to_temporaries(nir, impl, true, false);
   nir_lower_global_vars_to_local(nir);
   nir_lower_vars_to_ssa(nir);
                  if (key->clamp_pointsize)
                     crocus_setup_uniforms(devinfo, mem_ctx, nir, prog_data, &system_values,
                     if (devinfo->ver <= 5 &&
      !(nir->info.inputs_read & BITFIELD64_BIT(VERT_ATTRIB_EDGEFLAG)))
         struct crocus_binding_table bt;
   crocus_setup_binding_table(devinfo, nir, &bt, /* num_render_targets */ 0,
            if (can_push_ubo(devinfo))
            uint64_t outputs_written =
         brw_compute_vue_map(devinfo,
                  /* Don't tell the backend about our clip plane constants, we've already
   * lowered them in NIR and we don't want it doing it again.
   */
   struct brw_vs_prog_key key_no_ucp = *key;
   key_no_ucp.nr_userclip_plane_consts = 0;
   key_no_ucp.copy_edgeflag = false;
            struct brw_compile_vs_params params = {
      .base = {
      .mem_ctx = mem_ctx,
   .nir = nir,
      },
   .key = &key_no_ucp,
   .prog_data = vs_prog_data,
      };
   const unsigned *program =
         if (program == NULL) {
      dbg_printf("Failed to compile vertex shader: %s\n", params.base.error_str);
   ralloc_free(mem_ctx);
               if (ish->compiled_once) {
         } else {
                  uint32_t *so_decls = NULL;
   if (devinfo->ver > 6)
      so_decls = screen->vtbl.create_so_decl_list(&ish->stream_output,
         struct crocus_compiled_shader *shader =
      crocus_upload_shader(ice, CROCUS_CACHE_VS, sizeof(*key), key, program,
                           crocus_disk_cache_store(screen->disk_cache, ish, shader,
                  ralloc_free(mem_ctx);
      }
      /**
   * Update the current vertex shader variant.
   *
   * Fill out the key, look in the cache, compile and bind if needed.
   */
   static void
   crocus_update_compiled_vs(struct crocus_context *ice)
   {
      struct crocus_shader_state *shs = &ice->state.shaders[MESA_SHADER_VERTEX];
   struct crocus_uncompiled_shader *ish =
         struct crocus_screen *screen = (struct crocus_screen *)ice->ctx.screen;
   const struct intel_device_info *devinfo = &screen->devinfo;
            if (ish->nos & (1ull << CROCUS_NOS_TEXTURES))
      crocus_populate_sampler_prog_key_data(ice, devinfo, MESA_SHADER_VERTEX, ish,
               struct crocus_compiled_shader *old = ice->shaders.prog[CROCUS_CACHE_VS];
   struct crocus_compiled_shader *shader =
            if (!shader)
            if (!shader)
            if (old != shader) {
      ice->shaders.prog[CROCUS_CACHE_VS] = shader;
   if (devinfo->ver == 8)
         ice->state.stage_dirty |= CROCUS_STAGE_DIRTY_VS |
                        const struct brw_vs_prog_data *vs_prog_data =
         const bool uses_draw_params = vs_prog_data->uses_firstvertex ||
         const bool uses_derived_draw_params = vs_prog_data->uses_drawid ||
         const bool needs_sgvs_element = uses_draw_params ||
                  if (ice->state.vs_uses_draw_params != uses_draw_params ||
      ice->state.vs_uses_derived_draw_params != uses_derived_draw_params ||
   ice->state.vs_needs_edge_flag != ish->needs_edge_flag ||
   ice->state.vs_uses_vertexid != vs_prog_data->uses_vertexid ||
   ice->state.vs_uses_instanceid != vs_prog_data->uses_instanceid) {
   ice->state.dirty |= CROCUS_DIRTY_VERTEX_BUFFERS |
      }
   ice->state.vs_uses_draw_params = uses_draw_params;
   ice->state.vs_uses_derived_draw_params = uses_derived_draw_params;
   ice->state.vs_needs_sgvs_element = needs_sgvs_element;
   ice->state.vs_needs_edge_flag = ish->needs_edge_flag;
   ice->state.vs_uses_vertexid = vs_prog_data->uses_vertexid;
         }
      /**
   * Get the shader_info for a given stage, or NULL if the stage is disabled.
   */
   const struct shader_info *
   crocus_get_shader_info(const struct crocus_context *ice, gl_shader_stage stage)
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
   get_unified_tess_slots(const struct crocus_context *ice,
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
   static struct crocus_compiled_shader *
   crocus_compile_tcs(struct crocus_context *ice,
               {
      struct crocus_screen *screen = (struct crocus_screen *)ice->ctx.screen;
   const struct brw_compiler *compiler = screen->compiler;
   void *mem_ctx = ralloc_context(NULL);
   struct brw_tcs_prog_data *tcs_prog_data =
         struct brw_vue_prog_data *vue_prog_data = &tcs_prog_data->base;
   struct brw_stage_prog_data *prog_data = &vue_prog_data->base;
   const struct intel_device_info *devinfo = &screen->devinfo;
   enum brw_param_builtin *system_values = NULL;
   unsigned num_system_values = 0;
                              if (ish) {
         } else {
                  crocus_setup_uniforms(devinfo, mem_ctx, nir, prog_data, &system_values,
            crocus_lower_swizzles(nir, &key->base.tex);
   crocus_setup_binding_table(devinfo, nir, &bt, /* num_render_targets */ 0,
         if (can_push_ubo(devinfo))
            struct brw_tcs_prog_key key_clean = *key;
            struct brw_compile_tcs_params params = {
      .base = {
      .mem_ctx = mem_ctx,
   .nir = nir,
      },
   .key = &key_clean,
               const unsigned *program = brw_compile_tcs(compiler, &params);
   if (program == NULL) {
      dbg_printf("Failed to compile control shader: %s\n", params.base.error_str);
   ralloc_free(mem_ctx);
               if (ish) {
      if (ish->compiled_once) {
         } else {
                     struct crocus_compiled_shader *shader =
      crocus_upload_shader(ice, CROCUS_CACHE_TCS, sizeof(*key), key, program,
                           if (ish)
      crocus_disk_cache_store(screen->disk_cache, ish, shader,
               ralloc_free(mem_ctx);
      }
      /**
   * Update the current tessellation control shader variant.
   *
   * Fill out the key, look in the cache, compile and bind if needed.
   */
   static void
   crocus_update_compiled_tcs(struct crocus_context *ice)
   {
      struct crocus_shader_state *shs = &ice->state.shaders[MESA_SHADER_TESS_CTRL];
   struct crocus_uncompiled_shader *tcs =
         struct crocus_screen *screen = (struct crocus_screen *)ice->ctx.screen;
            const struct shader_info *tes_info =
         struct brw_tcs_prog_key key = {
      KEY_INIT_NO_ID(),
   .base.program_string_id = tcs ? tcs->program_id : 0,
   ._tes_primitive_mode = tes_info->tess._primitive_mode,
   .input_vertices = ice->state.vertices_per_patch,
   .quads_workaround = tes_info->tess._primitive_mode == TESS_PRIMITIVE_QUADS &&
               if (tcs && tcs->nos & (1ull << CROCUS_NOS_TEXTURES))
      crocus_populate_sampler_prog_key_data(ice, devinfo, MESA_SHADER_TESS_CTRL, tcs,
      get_unified_tess_slots(ice, &key.outputs_written,
                  struct crocus_compiled_shader *old = ice->shaders.prog[CROCUS_CACHE_TCS];
   struct crocus_compiled_shader *shader =
            if (tcs && !shader)
            if (!shader)
            if (old != shader) {
      ice->shaders.prog[CROCUS_CACHE_TCS] = shader;
   ice->state.stage_dirty |= CROCUS_STAGE_DIRTY_TCS |
                     }
      /**
   * Compile a tessellation evaluation shader, and upload the assembly.
   */
   static struct crocus_compiled_shader *
   crocus_compile_tes(struct crocus_context *ice,
               {
      struct crocus_screen *screen = (struct crocus_screen *)ice->ctx.screen;
   const struct brw_compiler *compiler = screen->compiler;
   void *mem_ctx = ralloc_context(NULL);
   struct brw_tes_prog_data *tes_prog_data =
         struct brw_vue_prog_data *vue_prog_data = &tes_prog_data->base;
   struct brw_stage_prog_data *prog_data = &vue_prog_data->base;
   enum brw_param_builtin *system_values;
   const struct intel_device_info *devinfo = &screen->devinfo;
   unsigned num_system_values;
                     if (key->nr_userclip_plane_consts) {
      nir_function_impl *impl = nir_shader_get_entrypoint(nir);
   nir_lower_clip_vs(nir, (1 << key->nr_userclip_plane_consts) - 1, true,
         nir_lower_io_to_temporaries(nir, impl, true, false);
   nir_lower_global_vars_to_local(nir);
   nir_lower_vars_to_ssa(nir);
               if (key->clamp_pointsize)
            crocus_setup_uniforms(devinfo, mem_ctx, nir, prog_data, &system_values,
         crocus_lower_swizzles(nir, &key->base.tex);
   struct crocus_binding_table bt;
   crocus_setup_binding_table(devinfo, nir, &bt, /* num_render_targets */ 0,
            if (can_push_ubo(devinfo))
            struct brw_vue_map input_vue_map;
   brw_compute_tess_vue_map(&input_vue_map, key->inputs_read,
            struct brw_tes_prog_key key_clean = *key;
            struct brw_compile_tes_params params = {
      .base = {
      .mem_ctx = mem_ctx,
   .nir = nir,
      },
   .key = &key_clean,
   .prog_data = tes_prog_data,
               const unsigned *program = brw_compile_tes(compiler, &params);
   if (program == NULL) {
      dbg_printf("Failed to compile evaluation shader: %s\n", params.base.error_str);
   ralloc_free(mem_ctx);
               if (ish->compiled_once) {
         } else {
                  uint32_t *so_decls = NULL;
   if (devinfo->ver > 6)
      so_decls = screen->vtbl.create_so_decl_list(&ish->stream_output,
         struct crocus_compiled_shader *shader =
      crocus_upload_shader(ice, CROCUS_CACHE_TES, sizeof(*key), key, program,
                           crocus_disk_cache_store(screen->disk_cache, ish, shader,
                  ralloc_free(mem_ctx);
      }
      /**
   * Update the current tessellation evaluation shader variant.
   *
   * Fill out the key, look in the cache, compile and bind if needed.
   */
   static void
   crocus_update_compiled_tes(struct crocus_context *ice)
   {
      struct crocus_shader_state *shs = &ice->state.shaders[MESA_SHADER_TESS_EVAL];
   struct crocus_uncompiled_shader *ish =
         struct crocus_screen *screen = (struct crocus_screen *)ice->ctx.screen;
   struct brw_tes_prog_key key = { KEY_INIT() };
            if (ish->nos & (1ull << CROCUS_NOS_TEXTURES))
      crocus_populate_sampler_prog_key_data(ice, devinfo, MESA_SHADER_TESS_EVAL, ish,
      get_unified_tess_slots(ice, &key.inputs_read, &key.patch_inputs_read);
            struct crocus_compiled_shader *old = ice->shaders.prog[CROCUS_CACHE_TES];
   struct crocus_compiled_shader *shader =
            if (!shader)
            if (!shader)
            if (old != shader) {
      ice->shaders.prog[CROCUS_CACHE_TES] = shader;
   ice->state.stage_dirty |= CROCUS_STAGE_DIRTY_TES |
                           /* TODO: Could compare and avoid flagging this. */
   const struct shader_info *tes_info = &ish->nir->info;
   if (BITSET_TEST(tes_info->system_values_read, SYSTEM_VALUE_VERTICES_IN)) {
      ice->state.stage_dirty |= CROCUS_STAGE_DIRTY_CONSTANTS_TES;
         }
      /**
   * Compile a geometry shader, and upload the assembly.
   */
   static struct crocus_compiled_shader *
   crocus_compile_gs(struct crocus_context *ice,
               {
      struct crocus_screen *screen = (struct crocus_screen *)ice->ctx.screen;
   const struct brw_compiler *compiler = screen->compiler;
   const struct intel_device_info *devinfo = &screen->devinfo;
   void *mem_ctx = ralloc_context(NULL);
   struct brw_gs_prog_data *gs_prog_data =
         struct brw_vue_prog_data *vue_prog_data = &gs_prog_data->base;
   struct brw_stage_prog_data *prog_data = &vue_prog_data->base;
   enum brw_param_builtin *system_values;
   unsigned num_system_values;
                     if (key->nr_userclip_plane_consts) {
      nir_function_impl *impl = nir_shader_get_entrypoint(nir);
   nir_lower_clip_gs(nir, (1 << key->nr_userclip_plane_consts) - 1, false,
         nir_lower_io_to_temporaries(nir, impl, true, false);
   nir_lower_global_vars_to_local(nir);
   nir_lower_vars_to_ssa(nir);
               if (key->clamp_pointsize)
            crocus_setup_uniforms(devinfo, mem_ctx, nir, prog_data, &system_values,
         crocus_lower_swizzles(nir, &key->base.tex);
   struct crocus_binding_table bt;
   crocus_setup_binding_table(devinfo, nir, &bt, /* num_render_targets */ 0,
            if (can_push_ubo(devinfo))
            brw_compute_vue_map(devinfo,
                  if (devinfo->ver == 6)
         struct brw_gs_prog_key key_clean = *key;
            struct brw_compile_gs_params params = {
      .base = {
      .mem_ctx = mem_ctx,
   .nir = nir,
      },
   .key = &key_clean,
               const unsigned *program = brw_compile_gs(compiler, &params);
   if (program == NULL) {
      dbg_printf("Failed to compile geometry shader: %s\n", params.base.error_str);
   ralloc_free(mem_ctx);
               if (ish->compiled_once) {
         } else {
                  uint32_t *so_decls = NULL;
   if (devinfo->ver > 6)
      so_decls = screen->vtbl.create_so_decl_list(&ish->stream_output,
         struct crocus_compiled_shader *shader =
      crocus_upload_shader(ice, CROCUS_CACHE_GS, sizeof(*key), key, program,
                           crocus_disk_cache_store(screen->disk_cache, ish, shader,
                  ralloc_free(mem_ctx);
      }
      /**
   * Update the current geometry shader variant.
   *
   * Fill out the key, look in the cache, compile and bind if needed.
   */
   static void
   crocus_update_compiled_gs(struct crocus_context *ice)
   {
      struct crocus_shader_state *shs = &ice->state.shaders[MESA_SHADER_GEOMETRY];
   struct crocus_uncompiled_shader *ish =
         struct crocus_compiled_shader *old = ice->shaders.prog[CROCUS_CACHE_GS];
            if (ish) {
      struct crocus_screen *screen = (struct crocus_screen *)ice->ctx.screen;
   const struct intel_device_info *devinfo = &screen->devinfo;
            if (ish->nos & (1ull << CROCUS_NOS_TEXTURES))
      crocus_populate_sampler_prog_key_data(ice, devinfo, MESA_SHADER_GEOMETRY, ish,
               shader =
            if (!shader)
            if (!shader)
               if (old != shader) {
      ice->shaders.prog[CROCUS_CACHE_GS] = shader;
   ice->state.stage_dirty |= CROCUS_STAGE_DIRTY_GS |
                     }
      /**
   * Compile a fragment (pixel) shader, and upload the assembly.
   */
   static struct crocus_compiled_shader *
   crocus_compile_fs(struct crocus_context *ice,
                     {
      struct crocus_screen *screen = (struct crocus_screen *)ice->ctx.screen;
   const struct brw_compiler *compiler = screen->compiler;
   void *mem_ctx = ralloc_context(NULL);
   struct brw_wm_prog_data *fs_prog_data =
         struct brw_stage_prog_data *prog_data = &fs_prog_data->base;
   enum brw_param_builtin *system_values;
   const struct intel_device_info *devinfo = &screen->devinfo;
   unsigned num_system_values;
                              crocus_setup_uniforms(devinfo, mem_ctx, nir, prog_data, &system_values,
            /* Lower output variables to load_output intrinsics before setting up
   * binding tables, so crocus_setup_binding_table can map any load_output
   * intrinsics to CROCUS_SURFACE_GROUP_RENDER_TARGET_READ on Gen8 for
   * non-coherent framebuffer fetches.
   */
            /* lower swizzles before binding table */
   crocus_lower_swizzles(nir, &key->base.tex);
            struct crocus_binding_table bt;
   crocus_setup_binding_table(devinfo, nir, &bt,
                        if (can_push_ubo(devinfo))
            struct brw_wm_prog_key key_clean = *key;
            struct brw_compile_fs_params params = {
      .base = {
      .mem_ctx = mem_ctx,
   .nir = nir,
      },
   .key = &key_clean,
            .allow_spilling = true,
      };
   const unsigned *program =
         if (program == NULL) {
      dbg_printf("Failed to compile fragment shader: %s\n", params.base.error_str);
   ralloc_free(mem_ctx);
               if (ish->compiled_once) {
         } else {
                  struct crocus_compiled_shader *shader =
      crocus_upload_shader(ice, CROCUS_CACHE_FS, sizeof(*key), key, program,
                           crocus_disk_cache_store(screen->disk_cache, ish, shader,
                  ralloc_free(mem_ctx);
      }
      /**
   * Update the current fragment shader variant.
   *
   * Fill out the key, look in the cache, compile and bind if needed.
   */
   static void
   crocus_update_compiled_fs(struct crocus_context *ice)
   {
      struct crocus_screen *screen = (struct crocus_screen *)ice->ctx.screen;
   const struct intel_device_info *devinfo = &screen->devinfo;
   struct crocus_shader_state *shs = &ice->state.shaders[MESA_SHADER_FRAGMENT];
   struct crocus_uncompiled_shader *ish =
                  if (ish->nos & (1ull << CROCUS_NOS_TEXTURES))
      crocus_populate_sampler_prog_key_data(ice, devinfo, MESA_SHADER_FRAGMENT, ish,
               if (ish->nos & (1ull << CROCUS_NOS_LAST_VUE_MAP))
            struct crocus_compiled_shader *old = ice->shaders.prog[CROCUS_CACHE_FS];
   struct crocus_compiled_shader *shader =
            if (!shader)
            if (!shader)
            if (old != shader) {
      // XXX: only need to flag CLIP if barycentric has NONPERSPECTIVE
   // toggles.  might be able to avoid flagging SBE too.
   ice->shaders.prog[CROCUS_CACHE_FS] = shader;
   ice->state.dirty |= CROCUS_DIRTY_WM;
   /* gen4 clip/sf rely on fs prog_data */
   if (devinfo->ver < 6)
         else
         if (devinfo->ver == 6)
         if (devinfo->ver >= 7)
         ice->state.stage_dirty |= CROCUS_STAGE_DIRTY_FS |
                     }
      /**
   * Update the last enabled stage's VUE map.
   *
   * When the shader feeding the rasterizer's output interface changes, we
   * need to re-emit various packets.
   */
   static void
   update_last_vue_map(struct crocus_context *ice,
         {
      struct crocus_screen *screen = (struct crocus_screen *)ice->ctx.screen;
   const struct intel_device_info *devinfo = &screen->devinfo;
   struct brw_vue_prog_data *vue_prog_data = (void *) prog_data;
   struct brw_vue_map *vue_map = &vue_prog_data->vue_map;
   struct brw_vue_map *old_map = ice->shaders.last_vue_map;
   const uint64_t changed_slots =
            if (changed_slots & VARYING_BIT_VIEWPORT) {
      ice->state.num_viewports =
         ice->state.dirty |= CROCUS_DIRTY_SF_CL_VIEWPORT |
         if (devinfo->ver < 6)
            if (devinfo->ver <= 6)
            if (devinfo->ver >= 6)
      ice->state.dirty |= CROCUS_DIRTY_CLIP |
      ice->state.stage_dirty |= CROCUS_STAGE_DIRTY_UNCOMPILED_FS |
               if (changed_slots || (old_map && old_map->separate != vue_map->separate)) {
      ice->state.dirty |= CROCUS_DIRTY_GEN7_SBE;
   if (devinfo->ver < 6)
                        }
      static void
   crocus_update_pull_constant_descriptors(struct crocus_context *ice,
         {
               if (!shader || !shader->prog_data->has_ubo_pull)
            struct crocus_shader_state *shs = &ice->state.shaders[stage];
   bool any_new_descriptors =
                     while (bound_cbufs) {
      const int i = u_bit_scan(&bound_cbufs);
   struct pipe_constant_buffer *cbuf = &shs->constbufs[i];
   if (cbuf->buffer) {
                     if (any_new_descriptors)
      }
      /**
   * Get the prog_data for a given stage, or NULL if the stage is disabled.
   */
   static struct brw_vue_prog_data *
   get_vue_prog_data(struct crocus_context *ice, gl_shader_stage stage)
   {
      if (!ice->shaders.prog[stage])
               }
      static struct crocus_compiled_shader *
   crocus_compile_clip(struct crocus_context *ice, struct brw_clip_prog_key *key)
   {
      struct crocus_screen *screen = (struct crocus_screen *)ice->ctx.screen;
   const struct brw_compiler *compiler = screen->compiler;
   void *mem_ctx;
   unsigned program_size;
            struct brw_clip_prog_data *clip_prog_data =
            const unsigned *program = brw_compile_clip(compiler, mem_ctx, key, clip_prog_data,
            if (program == NULL) {
      dbg_printf("failed to compile clip shader\n");
   ralloc_free(mem_ctx);
      }
   struct crocus_binding_table bt;
            struct crocus_compiled_shader *shader =
      crocus_upload_shader(ice, CROCUS_CACHE_CLIP, sizeof(*key), key, program,
                  ralloc_free(mem_ctx);
      }
   static void
   crocus_update_compiled_clip(struct crocus_context *ice)
   {
      struct crocus_screen *screen = (struct crocus_screen *)ice->ctx.screen;
   struct brw_clip_prog_key key;
   struct crocus_compiled_shader *old = ice->shaders.clip_prog;
            const struct brw_wm_prog_data *wm_prog_data = brw_wm_prog_data(ice->shaders.prog[MESA_SHADER_FRAGMENT]->prog_data);
   if (wm_prog_data) {
      key.contains_flat_varying = wm_prog_data->contains_flat_varying;
   key.contains_noperspective_varying =
                     key.primitive = ice->state.reduced_prim_mode;
            struct pipe_rasterizer_state *rs_state = crocus_get_rast_state(ice);
            if (rs_state->clip_plane_enable)
            if (screen->devinfo.ver == 5)
         else
            if (key.primitive == MESA_PRIM_TRIANGLES) {
      if (rs_state->cull_face == PIPE_FACE_FRONT_AND_BACK)
         else {
      uint32_t fill_front = BRW_CLIP_FILL_MODE_CULL;
   uint32_t fill_back = BRW_CLIP_FILL_MODE_CULL;
                  if (!(rs_state->cull_face & PIPE_FACE_FRONT)) {
      switch (rs_state->fill_front) {
   case PIPE_POLYGON_MODE_FILL:
      fill_front = BRW_CLIP_FILL_MODE_FILL;
   offset_front = 0;
      case PIPE_POLYGON_MODE_LINE:
      fill_front = BRW_CLIP_FILL_MODE_LINE;
   offset_front = rs_state->offset_line;
      case PIPE_POLYGON_MODE_POINT:
      fill_front = BRW_CLIP_FILL_MODE_POINT;
   offset_front = rs_state->offset_point;
                  if (!(rs_state->cull_face & PIPE_FACE_BACK)) {
      switch (rs_state->fill_back) {
   case PIPE_POLYGON_MODE_FILL:
      fill_back = BRW_CLIP_FILL_MODE_FILL;
   offset_back = 0;
      case PIPE_POLYGON_MODE_LINE:
      fill_back = BRW_CLIP_FILL_MODE_LINE;
   offset_back = rs_state->offset_line;
      case PIPE_POLYGON_MODE_POINT:
      fill_back = BRW_CLIP_FILL_MODE_POINT;
   offset_back = rs_state->offset_point;
                  if (rs_state->fill_back != PIPE_POLYGON_MODE_FILL ||
                     /* Most cases the fixed function units will handle.  Cases where
   * one or more polygon faces are unfilled will require help:
                  if (offset_back || offset_front) {
      double mrd = 0.0;
   if (ice->state.framebuffer.zsbuf)
         key.offset_units = rs_state->offset_units * mrd * 2;
                     if (!(rs_state->front_ccw ^ rs_state->bottom_edge_rule)) {
      key.fill_ccw = fill_front;
   key.fill_cw = fill_back;
   key.offset_ccw = offset_front;
   key.offset_cw = offset_back;
   if (rs_state->light_twoside &&
      key.fill_cw != BRW_CLIP_FILL_MODE_CULL)
   } else {
      key.fill_cw = fill_front;
   key.fill_ccw = fill_back;
   key.offset_cw = offset_front;
   key.offset_ccw = offset_back;
   if (rs_state->light_twoside &&
      key.fill_ccw != BRW_CLIP_FILL_MODE_CULL)
            }
   struct crocus_compiled_shader *shader =
            if (!shader)
            if (old != shader) {
      ice->state.dirty |= CROCUS_DIRTY_CLIP;
         }
      static struct crocus_compiled_shader *
   crocus_compile_sf(struct crocus_context *ice, struct brw_sf_prog_key *key)
   {
      struct crocus_screen *screen = (struct crocus_screen *)ice->ctx.screen;
   const struct brw_compiler *compiler = screen->compiler;
   void *mem_ctx;
   unsigned program_size;
            struct brw_sf_prog_data *sf_prog_data =
            const unsigned *program = brw_compile_sf(compiler, mem_ctx, key, sf_prog_data,
            if (program == NULL) {
      dbg_printf("failed to compile sf shader\n");
   ralloc_free(mem_ctx);
               struct crocus_binding_table bt;
   memset(&bt, 0, sizeof(bt));
   struct crocus_compiled_shader *shader =
      crocus_upload_shader(ice, CROCUS_CACHE_SF, sizeof(*key), key, program,
                  ralloc_free(mem_ctx);
      }
      static void
   crocus_update_compiled_sf(struct crocus_context *ice)
   {
      struct brw_sf_prog_key key;
   struct crocus_compiled_shader *old = ice->shaders.sf_prog;
                     switch (ice->state.reduced_prim_mode) {
   case MESA_PRIM_TRIANGLES:
   default:
      if (key.attrs & BITFIELD64_BIT(VARYING_SLOT_EDGE))
         else
            case MESA_PRIM_LINES:
      key.primitive = BRW_SF_PRIM_LINES;
      case MESA_PRIM_POINTS:
      key.primitive = BRW_SF_PRIM_POINTS;
               struct pipe_rasterizer_state *rs_state = crocus_get_rast_state(ice);
   key.userclip_active = rs_state->clip_plane_enable != 0;
   const struct brw_wm_prog_data *wm_prog_data = brw_wm_prog_data(ice->shaders.prog[MESA_SHADER_FRAGMENT]->prog_data);
   if (wm_prog_data) {
      key.contains_flat_varying = wm_prog_data->contains_flat_varying;
                        key.do_point_sprite = rs_state->point_quad_rasterization;
   if (key.do_point_sprite) {
      key.point_sprite_coord_replace = rs_state->sprite_coord_enable & 0xff;
   if (rs_state->sprite_coord_enable & (1 << 8))
         if (wm_prog_data && wm_prog_data->urb_setup[VARYING_SLOT_PNTC] != -1)
                        if (key.do_twoside_color) {
         }
   struct crocus_compiled_shader *shader =
            if (!shader)
            if (old != shader) {
      ice->state.dirty |= CROCUS_DIRTY_RASTER;
         }
      static struct crocus_compiled_shader *
   crocus_compile_ff_gs(struct crocus_context *ice, struct brw_ff_gs_prog_key *key)
   {
      struct crocus_screen *screen = (struct crocus_screen *)ice->ctx.screen;
   struct brw_compiler *compiler = screen->compiler;
   void *mem_ctx;
   unsigned program_size;
            struct brw_ff_gs_prog_data *ff_gs_prog_data =
            const unsigned *program = brw_compile_ff_gs_prog(compiler, mem_ctx, key, ff_gs_prog_data,
            if (program == NULL) {
      dbg_printf("failed to compile sf shader\n");
   ralloc_free(mem_ctx);
               struct crocus_binding_table bt;
            if (screen->devinfo.ver == 6) {
      bt.sizes[CROCUS_SURFACE_GROUP_SOL] = BRW_MAX_SOL_BINDINGS;
                        struct crocus_compiled_shader *shader =
      crocus_upload_shader(ice, CROCUS_CACHE_FF_GS, sizeof(*key), key, program,
                  ralloc_free(mem_ctx);
      }
      static void
   crocus_update_compiled_ff_gs(struct crocus_context *ice)
   {
      struct crocus_screen *screen = (struct crocus_screen *)ice->ctx.screen;
   const struct intel_device_info *devinfo = &screen->devinfo;
   struct brw_ff_gs_prog_key key;
   struct crocus_compiled_shader *old = ice->shaders.ff_gs_prog;
                                       struct pipe_rasterizer_state *rs_state = crocus_get_rast_state(ice);
            if (key.primitive == _3DPRIM_QUADLIST && !rs_state->flatshade) {
      /* Provide consistenbbbbbt primitive order with brw_set_prim's
   * optimization of single quads to trifans.
   */
               if (devinfo->ver >= 6) {
      key.need_gs_prog = ice->state.streamout_active;
   if (key.need_gs_prog) {
      struct crocus_uncompiled_shader *vs =
         gfx6_ff_gs_xfb_setup(&vs->stream_output,
         } else {
      key.need_gs_prog = (key.primitive == _3DPRIM_QUADLIST ||
                     struct crocus_compiled_shader *shader = NULL;
   if (key.need_gs_prog) {
      shader = crocus_find_cached_shader(ice, CROCUS_CACHE_FF_GS,
         if (!shader)
      }
   if (old != shader) {
      ice->state.stage_dirty |= CROCUS_STAGE_DIRTY_GS;
   if (!!old != !!shader)
         ice->shaders.ff_gs_prog = shader;
   if (shader) {
      const struct brw_ff_gs_prog_data *gs_prog_data = (struct brw_ff_gs_prog_data *)ice->shaders.ff_gs_prog->prog_data;
            }
      // XXX: crocus_compiled_shaders are space-leaking :(
   // XXX: do remember to unbind them if deleting them.
      /**
   * Update the current shader variants for the given state.
   *
   * This should be called on every draw call to ensure that the correct
   * shaders are bound.  It will also flag any dirty state triggered by
   * swapping out those shaders.
   */
   bool
   crocus_update_compiled_shaders(struct crocus_context *ice)
   {
      struct crocus_screen *screen = (void *) ice->ctx.screen;
            struct brw_vue_prog_data *old_prog_datas[4];
   if (!(ice->state.dirty & CROCUS_DIRTY_GEN6_URB)) {
      for (int i = MESA_SHADER_VERTEX; i <= MESA_SHADER_GEOMETRY; i++)
               if (stage_dirty & (CROCUS_STAGE_DIRTY_UNCOMPILED_TCS |
            struct crocus_uncompiled_shader *tes =
         if (tes) {
      crocus_update_compiled_tcs(ice);
      } else {
      ice->shaders.prog[CROCUS_CACHE_TCS] = NULL;
   ice->shaders.prog[CROCUS_CACHE_TES] = NULL;
   ice->state.stage_dirty |=
      CROCUS_STAGE_DIRTY_TCS | CROCUS_STAGE_DIRTY_TES |
   CROCUS_STAGE_DIRTY_BINDINGS_TCS | CROCUS_STAGE_DIRTY_BINDINGS_TES |
               if (stage_dirty & CROCUS_STAGE_DIRTY_UNCOMPILED_VS)
         if (stage_dirty & CROCUS_STAGE_DIRTY_UNCOMPILED_GS)
            if (stage_dirty & (CROCUS_STAGE_DIRTY_UNCOMPILED_GS |
            const struct crocus_compiled_shader *gs =
         const struct crocus_compiled_shader *tes =
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
                  if (!ice->shaders.prog[MESA_SHADER_VERTEX])
            gl_shader_stage last_stage = last_vue_stage(ice);
   struct crocus_compiled_shader *shader = ice->shaders.prog[last_stage];
   struct crocus_uncompiled_shader *ish = ice->shaders.uncompiled[last_stage];
   update_last_vue_map(ice, shader->prog_data);
   if (ice->state.streamout != shader->streamout) {
      ice->state.streamout = shader->streamout;
               if (ice->state.streamout_active) {
                  /* use ice->state version as last_vue_map can dirty this bit */
   if (ice->state.stage_dirty & CROCUS_STAGE_DIRTY_UNCOMPILED_FS)
            if (screen->devinfo.ver <= 6) {
      if (ice->state.dirty & CROCUS_DIRTY_GEN4_FF_GS_PROG &&
      !ice->shaders.prog[MESA_SHADER_GEOMETRY])
            if (screen->devinfo.ver < 6) {
      if (ice->state.dirty & CROCUS_DIRTY_GEN4_CLIP_PROG)
         if (ice->state.dirty & CROCUS_DIRTY_GEN4_SF_PROG)
                  /* Changing shader interfaces may require a URB configuration. */
   if (!(ice->state.dirty & CROCUS_DIRTY_GEN6_URB)) {
      for (int i = MESA_SHADER_VERTEX; i <= MESA_SHADER_GEOMETRY; i++) {
      struct brw_vue_prog_data *old = old_prog_datas[i];
   struct brw_vue_prog_data *new = get_vue_prog_data(ice, i);
   if (!!old != !!new ||
      (new && new->urb_entry_size != old->urb_entry_size)) {
   ice->state.dirty |= CROCUS_DIRTY_GEN6_URB;
                     if (ice->state.stage_dirty & CROCUS_RENDER_STAGE_DIRTY_CONSTANTS) {
      for (int i = MESA_SHADER_VERTEX; i <= MESA_SHADER_FRAGMENT; i++) {
      if (ice->state.stage_dirty & (CROCUS_STAGE_DIRTY_CONSTANTS_VS << i))
         }
      }
      static struct crocus_compiled_shader *
   crocus_compile_cs(struct crocus_context *ice,
               {
      struct crocus_screen *screen = (struct crocus_screen *)ice->ctx.screen;
   const struct brw_compiler *compiler = screen->compiler;
   void *mem_ctx = ralloc_context(NULL);
   struct brw_cs_prog_data *cs_prog_data =
         struct brw_stage_prog_data *prog_data = &cs_prog_data->base;
   enum brw_param_builtin *system_values;
   const struct intel_device_info *devinfo = &screen->devinfo;
   unsigned num_system_values;
                              crocus_setup_uniforms(devinfo, mem_ctx, nir, prog_data, &system_values,
         crocus_lower_swizzles(nir, &key->base.tex);
   struct crocus_binding_table bt;
   crocus_setup_binding_table(devinfo, nir, &bt, /* num_render_targets */ 0,
            struct brw_compile_cs_params params = {
      .base = {
      .mem_ctx = mem_ctx,
   .nir = nir,
      },
   .key = key,
               const unsigned *program =
         if (program == NULL) {
      dbg_printf("Failed to compile compute shader: %s\n", params.base.error_str);
   ralloc_free(mem_ctx);
               if (ish->compiled_once) {
         } else {
                  struct crocus_compiled_shader *shader =
      crocus_upload_shader(ice, CROCUS_CACHE_CS, sizeof(*key), key, program,
                           crocus_disk_cache_store(screen->disk_cache, ish, shader,
                  ralloc_free(mem_ctx);
      }
      static void
   crocus_update_compiled_cs(struct crocus_context *ice)
   {
      struct crocus_shader_state *shs = &ice->state.shaders[MESA_SHADER_COMPUTE];
   struct crocus_uncompiled_shader *ish =
         struct crocus_screen *screen = (struct crocus_screen *)ice->ctx.screen;
   const struct intel_device_info *devinfo = &screen->devinfo;
            if (ish->nos & (1ull << CROCUS_NOS_TEXTURES))
      crocus_populate_sampler_prog_key_data(ice, devinfo, MESA_SHADER_COMPUTE, ish,
               struct crocus_compiled_shader *old = ice->shaders.prog[CROCUS_CACHE_CS];
   struct crocus_compiled_shader *shader =
            if (!shader)
            if (!shader)
            if (old != shader) {
      ice->shaders.prog[CROCUS_CACHE_CS] = shader;
   ice->state.stage_dirty |= CROCUS_STAGE_DIRTY_CS |
                     }
      void
   crocus_update_compiled_compute_shader(struct crocus_context *ice)
   {
      if (ice->state.stage_dirty & CROCUS_STAGE_DIRTY_UNCOMPILED_CS)
            if (ice->state.stage_dirty & CROCUS_STAGE_DIRTY_CONSTANTS_CS)
      }
      void
   crocus_fill_cs_push_const_buffer(struct brw_cs_prog_data *cs_prog_data,
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
   struct crocus_bo *
   crocus_get_scratch_space(struct crocus_context *ice,
               {
      struct crocus_screen *screen = (struct crocus_screen *)ice->ctx.screen;
   struct crocus_bufmgr *bufmgr = screen->bufmgr;
            unsigned encoded_size = ffs(per_thread_scratch) - 11;
                     if (!*bop) {
      assert(stage < ARRAY_SIZE(devinfo->max_scratch_ids));
   uint32_t size = per_thread_scratch * devinfo->max_scratch_ids[stage];
                  }
      /* ------------------------------------------------------------------- */
      /**
   * The pipe->create_[stage]_state() driver hooks.
   *
   * Performs basic NIR preprocessing, records any state dependencies, and
   * returns an crocus_uncompiled_shader as the Gallium CSO.
   *
   * Actual shader compilation to assembly happens later, at first use.
   */
   static void *
   crocus_create_uncompiled_shader(struct pipe_context *ctx,
               {
      struct crocus_screen *screen = (struct crocus_screen *)ctx->screen;
   const struct intel_device_info *devinfo = &screen->devinfo;
   struct crocus_uncompiled_shader *ish =
         if (!ish)
            if (devinfo->ver >= 6)
   NIR_PASS(ish->needs_edge_flag, nir, crocus_fix_edge_flags);
   else
            struct brw_nir_compiler_opts opts = {};
            NIR_PASS_V(nir, brw_nir_lower_storage_image,
            &(struct brw_nir_lower_storage_image_opts) {
      .devinfo = devinfo,
   .lower_loads = true,
   .lower_stores = true,
   .lower_atomics = true,
                     ish->program_id = get_new_program_id(screen);
   ish->nir = nir;
   if (so_info) {
      memcpy(&ish->stream_output, so_info, sizeof(*so_info));
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
      static struct crocus_uncompiled_shader *
   crocus_create_shader_state(struct pipe_context *ctx,
         {
               if (state->type == PIPE_SHADER_IR_TGSI)
         else
               }
      static void *
   crocus_create_vs_state(struct pipe_context *ctx,
         {
      struct crocus_context *ice = (void *) ctx;
   struct crocus_screen *screen = (void *) ctx->screen;
            ish->nos |= (1ull << CROCUS_NOS_TEXTURES);
   /* User clip planes or gen5 sprite coord enable */
   if (ish->nir->info.clip_distance_array_size == 0 ||
      screen->devinfo.ver <= 5)
         if (screen->devinfo.verx10 < 75)
            if (screen->precompile) {
               if (!crocus_disk_cache_retrieve(ice, ish, &key, sizeof(key)))
                  }
      static void *
   crocus_create_tcs_state(struct pipe_context *ctx,
         {
      struct crocus_context *ice = (void *) ctx;
   struct crocus_screen *screen = (void *) ctx->screen;
   struct crocus_uncompiled_shader *ish = crocus_create_shader_state(ctx, state);
            ish->nos |= (1ull << CROCUS_NOS_TEXTURES);
   if (screen->precompile) {
      struct brw_tcs_prog_key key = {
      KEY_INIT(),
   // XXX: make sure the linker fills this out from the TES...
   ._tes_primitive_mode =
      info->tess._primitive_mode ? info->tess._primitive_mode
      .outputs_written = info->outputs_written,
                        if (!crocus_disk_cache_retrieve(ice, ish, &key, sizeof(key)))
                  }
      static void *
   crocus_create_tes_state(struct pipe_context *ctx,
         {
      struct crocus_context *ice = (void *) ctx;
   struct crocus_screen *screen = (void *) ctx->screen;
   struct crocus_uncompiled_shader *ish = crocus_create_shader_state(ctx, state);
            ish->nos |= (1ull << CROCUS_NOS_TEXTURES);
   /* User clip planes */
   if (ish->nir->info.clip_distance_array_size == 0)
            if (screen->precompile) {
      struct brw_tes_prog_key key = {
      KEY_INIT(),
   // XXX: not ideal, need TCS output/TES input unification
   .inputs_read = info->inputs_read,
               if (!crocus_disk_cache_retrieve(ice, ish, &key, sizeof(key)))
                  }
      static void *
   crocus_create_gs_state(struct pipe_context *ctx,
         {
      struct crocus_context *ice = (void *) ctx;
   struct crocus_screen *screen = (void *) ctx->screen;
            ish->nos |= (1ull << CROCUS_NOS_TEXTURES);
   /* User clip planes */
   if (ish->nir->info.clip_distance_array_size == 0)
            if (screen->precompile) {
               if (!crocus_disk_cache_retrieve(ice, ish, &key, sizeof(key)))
                  }
      static void *
   crocus_create_fs_state(struct pipe_context *ctx,
         {
      struct crocus_context *ice = (void *) ctx;
   struct crocus_screen *screen = (void *) ctx->screen;
   struct crocus_uncompiled_shader *ish = crocus_create_shader_state(ctx, state);
            ish->nos |= (1ull << CROCUS_NOS_FRAMEBUFFER) |
               (1ull << CROCUS_NOS_DEPTH_STENCIL_ALPHA) |
            /* The program key needs the VUE map if there are > 16 inputs or gen4/5 */
   if (screen->devinfo.ver < 6 || util_bitcount64(ish->nir->info.inputs_read &
                        if (screen->precompile) {
      const uint64_t color_outputs = info->outputs_written &
      ~(BITFIELD64_BIT(FRAG_RESULT_DEPTH) |
               bool can_rearrange_varyings =
            const struct intel_device_info *devinfo = &screen->devinfo;
   struct brw_wm_prog_key key = {
      KEY_INIT(),
   .nr_color_regions = util_bitcount(color_outputs),
   .coherent_fb_fetch = false,
   .ignore_sample_mask_out = screen->devinfo.ver < 6 ? 1 : 0,
   .input_slots_valid =
               struct brw_vue_map vue_map;
   if (devinfo->ver < 6) {
      brw_compute_vue_map(devinfo, &vue_map,
            }
   if (!crocus_disk_cache_retrieve(ice, ish, &key, sizeof(key)))
                  }
      static void *
   crocus_create_compute_state(struct pipe_context *ctx,
         {
               struct crocus_context *ice = (void *) ctx;
   struct crocus_screen *screen = (void *) ctx->screen;
   struct crocus_uncompiled_shader *ish =
            ish->nos |= (1ull << CROCUS_NOS_TEXTURES);
            if (screen->precompile) {
               if (!crocus_disk_cache_retrieve(ice, ish, &key, sizeof(key)))
                  }
      /**
   * The pipe->delete_[stage]_state() driver hooks.
   *
   * Frees the crocus_uncompiled_shader.
   */
   static void
   crocus_delete_shader_state(struct pipe_context *ctx, void *state, gl_shader_stage stage)
   {
      struct crocus_uncompiled_shader *ish = state;
            if (ice->shaders.uncompiled[stage] == ish) {
      ice->shaders.uncompiled[stage] = NULL;
               if (ish->const_data) {
      pipe_resource_reference(&ish->const_data, NULL);
               ralloc_free(ish->nir);
      }
      static void
   crocus_delete_vs_state(struct pipe_context *ctx, void *state)
   {
         }
      static void
   crocus_delete_tcs_state(struct pipe_context *ctx, void *state)
   {
         }
      static void
   crocus_delete_tes_state(struct pipe_context *ctx, void *state)
   {
         }
      static void
   crocus_delete_gs_state(struct pipe_context *ctx, void *state)
   {
         }
      static void
   crocus_delete_fs_state(struct pipe_context *ctx, void *state)
   {
         }
      static void
   crocus_delete_cs_state(struct pipe_context *ctx, void *state)
   {
         }
      /**
   * The pipe->bind_[stage]_state() driver hook.
   *
   * Binds an uncompiled shader as the current one for a particular stage.
   * Updates dirty tracking to account for the shader's NOS.
   */
   static void
   bind_shader_state(struct crocus_context *ice,
               {
      uint64_t dirty_bit = CROCUS_STAGE_DIRTY_UNCOMPILED_VS << stage;
            const struct shader_info *old_info = crocus_get_shader_info(ice, stage);
            if ((old_info ? BITSET_LAST_BIT(old_info->textures_used) : 0) !=
      (new_info ? BITSET_LAST_BIT(new_info->textures_used) : 0)) {
               ice->shaders.uncompiled[stage] = ish;
            /* Record that CSOs need to mark CROCUS_DIRTY_UNCOMPILED_XS when they change
   * (or that they no longer need to do so).
   */
   for (int i = 0; i < CROCUS_NOS_COUNT; i++) {
      if (nos & (1 << i))
         else
         }
      static void
   crocus_bind_vs_state(struct pipe_context *ctx, void *state)
   {
      struct crocus_context *ice = (struct crocus_context *)ctx;
   struct crocus_uncompiled_shader *new_ish = state;
   struct crocus_screen *screen = (struct crocus_screen *)ice->ctx.screen;
            if (new_ish &&
      ice->state.window_space_position !=
   new_ish->nir->info.vs.window_space_position) {
   ice->state.window_space_position =
            ice->state.dirty |= CROCUS_DIRTY_CLIP |
                     if (devinfo->ver == 6) {
                     }
      static void
   crocus_bind_tcs_state(struct pipe_context *ctx, void *state)
   {
         }
      static void
   crocus_bind_tes_state(struct pipe_context *ctx, void *state)
   {
               /* Enabling/disabling optional stages requires a URB reconfiguration. */
   if (!!state != !!ice->shaders.uncompiled[MESA_SHADER_TESS_EVAL])
               }
      static void
   crocus_bind_gs_state(struct pipe_context *ctx, void *state)
   {
               /* Enabling/disabling optional stages requires a URB reconfiguration. */
   if (!!state != !!ice->shaders.uncompiled[MESA_SHADER_GEOMETRY])
               }
      static void
   crocus_bind_fs_state(struct pipe_context *ctx, void *state)
   {
      struct crocus_context *ice = (struct crocus_context *) ctx;
   struct crocus_screen *screen = (struct crocus_screen *) ctx->screen;
   const struct intel_device_info *devinfo = &screen->devinfo;
   struct crocus_uncompiled_shader *old_ish =
                  const unsigned color_bits =
      BITFIELD64_BIT(FRAG_RESULT_COLOR) |
         /* Fragment shader outputs influence HasWriteableRT */
   if (!old_ish || !new_ish ||
      (old_ish->nir->info.outputs_written & color_bits) !=
   (new_ish->nir->info.outputs_written & color_bits)) {
   if (devinfo->ver == 8)
         else
               if (devinfo->ver == 8)
            }
      static void
   crocus_bind_cs_state(struct pipe_context *ctx, void *state)
   {
         }
      void
   crocus_init_program_functions(struct pipe_context *ctx)
   {
      ctx->create_vs_state  = crocus_create_vs_state;
   ctx->create_tcs_state = crocus_create_tcs_state;
   ctx->create_tes_state = crocus_create_tes_state;
   ctx->create_gs_state  = crocus_create_gs_state;
   ctx->create_fs_state  = crocus_create_fs_state;
            ctx->delete_vs_state  = crocus_delete_vs_state;
   ctx->delete_tcs_state = crocus_delete_tcs_state;
   ctx->delete_tes_state = crocus_delete_tes_state;
   ctx->delete_gs_state  = crocus_delete_gs_state;
   ctx->delete_fs_state  = crocus_delete_fs_state;
            ctx->bind_vs_state  = crocus_bind_vs_state;
   ctx->bind_tcs_state = crocus_bind_tcs_state;
   ctx->bind_tes_state = crocus_bind_tes_state;
   ctx->bind_gs_state  = crocus_bind_gs_state;
   ctx->bind_fs_state  = crocus_bind_fs_state;
      }
