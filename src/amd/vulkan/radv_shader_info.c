   /*
   * Copyright © 2017 Red Hat
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
   #include "nir/nir.h"
   #include "nir/nir_xfb_info.h"
   #include "radv_private.h"
   #include "radv_shader.h"
      #include "ac_nir.h"
      static void
   mark_sampler_desc(const nir_variable *var, struct radv_shader_info *info)
   {
         }
      static void
   gather_intrinsic_load_input_info(const nir_shader *nir, const nir_intrinsic_instr *instr, struct radv_shader_info *info)
   {
      switch (nir->info.stage) {
   case MESA_SHADER_VERTEX: {
      unsigned idx = nir_intrinsic_io_semantics(instr).location;
   unsigned component = nir_intrinsic_component(instr);
   unsigned mask = nir_def_components_read(&instr->def);
            info->vs.input_usage_mask[idx] |= mask & 0xf;
   if (mask >> 4)
            }
   default:
            }
      static void
   gather_intrinsic_store_output_info(const nir_shader *nir, const nir_intrinsic_instr *instr,
         {
      unsigned idx = nir_intrinsic_base(instr);
   unsigned num_slots = nir_intrinsic_io_semantics(instr).num_slots;
   unsigned component = nir_intrinsic_component(instr);
   unsigned write_mask = nir_intrinsic_write_mask(instr);
            switch (nir->info.stage) {
   case MESA_SHADER_VERTEX:
      output_usage_mask = info->vs.output_usage_mask;
      case MESA_SHADER_TESS_EVAL:
      output_usage_mask = info->tes.output_usage_mask;
      case MESA_SHADER_GEOMETRY:
      output_usage_mask = info->gs.output_usage_mask;
      case MESA_SHADER_FRAGMENT:
      if (idx >= FRAG_RESULT_DATA0) {
               if (idx == FRAG_RESULT_DATA0)
      }
      default:
                  if (output_usage_mask) {
      for (unsigned i = 0; i < num_slots; i++) {
                     if (consider_force_vrs && idx == VARYING_SLOT_POS) {
               if (write_mask & BITFIELD_BIT(pos_w_chan)) {
      nir_scalar pos_w = nir_scalar_resolved(instr->src[0].ssa, pos_w_chan);
   /* Use coarse shading if the value of Pos.W can't be determined or if its value is != 1
   * (typical for non-GUI elements).
   */
   if (!nir_scalar_is_const(pos_w) || nir_scalar_as_uint(pos_w) != 0x3f800000u)
                  if (nir->info.stage == MESA_SHADER_GEOMETRY) {
      uint8_t gs_streams = nir_intrinsic_io_semantics(instr).gs_streams;
         }
      static void
   gather_push_constant_info(const nir_shader *nir, const nir_intrinsic_instr *instr, struct radv_shader_info *info)
   {
               if (nir_src_is_const(instr->src[0]) && instr->def.bit_size >= 32) {
      uint32_t start = (nir_intrinsic_base(instr) + nir_src_as_uint(instr->src[0])) / 4u;
            if (start + size <= (MAX_PUSH_CONSTANTS_SIZE / 4u)) {
      info->inline_push_constant_mask |= u_bit_consecutive64(start, size);
                     }
      static void
   gather_intrinsic_info(const nir_shader *nir, const nir_intrinsic_instr *instr, struct radv_shader_info *info,
         {
      switch (instr->intrinsic) {
   case nir_intrinsic_load_barycentric_sample:
   case nir_intrinsic_load_barycentric_pixel:
   case nir_intrinsic_load_barycentric_centroid:
   case nir_intrinsic_load_barycentric_at_sample:
   case nir_intrinsic_load_barycentric_at_offset: {
      enum glsl_interp_mode mode = nir_intrinsic_interp_mode(instr);
   switch (mode) {
   case INTERP_MODE_SMOOTH:
   case INTERP_MODE_NONE:
      if (instr->intrinsic == nir_intrinsic_load_barycentric_pixel ||
      instr->intrinsic == nir_intrinsic_load_barycentric_at_sample ||
   instr->intrinsic == nir_intrinsic_load_barycentric_at_offset)
      else if (instr->intrinsic == nir_intrinsic_load_barycentric_centroid)
         else if (instr->intrinsic == nir_intrinsic_load_barycentric_sample)
            case INTERP_MODE_NOPERSPECTIVE:
      if (instr->intrinsic == nir_intrinsic_load_barycentric_pixel ||
      instr->intrinsic == nir_intrinsic_load_barycentric_at_sample ||
   instr->intrinsic == nir_intrinsic_load_barycentric_at_offset)
      else if (instr->intrinsic == nir_intrinsic_load_barycentric_centroid)
         else if (instr->intrinsic == nir_intrinsic_load_barycentric_sample)
            default:
         }
   if (instr->intrinsic == nir_intrinsic_load_barycentric_at_sample)
            }
   case nir_intrinsic_load_provoking_vtx_amd:
      info->ps.load_provoking_vtx = true;
      case nir_intrinsic_load_sample_positions_amd:
      info->ps.needs_sample_positions = true;
      case nir_intrinsic_load_rasterization_primitive_amd:
      info->ps.load_rasterization_prim = true;
      case nir_intrinsic_load_local_invocation_id:
   case nir_intrinsic_load_workgroup_id: {
      unsigned mask = nir_def_components_read(&instr->def);
   while (mask) {
               if (instr->intrinsic == nir_intrinsic_load_workgroup_id)
         else
      }
      }
   case nir_intrinsic_load_frag_coord:
      info->ps.reads_frag_coord_mask |= nir_def_components_read(&instr->def);
      case nir_intrinsic_load_sample_pos:
      info->ps.reads_sample_pos_mask |= nir_def_components_read(&instr->def);
      case nir_intrinsic_load_push_constant:
      gather_push_constant_info(nir, instr, info);
      case nir_intrinsic_vulkan_resource_index:
      info->desc_set_used_mask |= (1u << nir_intrinsic_desc_set(instr));
      case nir_intrinsic_image_deref_load:
   case nir_intrinsic_image_deref_sparse_load:
   case nir_intrinsic_image_deref_store:
   case nir_intrinsic_image_deref_atomic:
   case nir_intrinsic_image_deref_atomic_swap:
   case nir_intrinsic_image_deref_size:
   case nir_intrinsic_image_deref_samples: {
      nir_variable *var = nir_deref_instr_get_variable(nir_instr_as_deref(instr->src[0].ssa->parent_instr));
   mark_sampler_desc(var, info);
      }
   case nir_intrinsic_load_input:
      gather_intrinsic_load_input_info(nir, instr, info);
      case nir_intrinsic_store_output:
      gather_intrinsic_store_output_info(nir, instr, info, consider_force_vrs);
      case nir_intrinsic_load_sbt_base_amd:
      info->cs.is_rt_shader = true;
      case nir_intrinsic_load_rt_dynamic_callable_stack_base_amd:
      info->cs.uses_dynamic_rt_callable_stack = true;
      case nir_intrinsic_bvh64_intersect_ray_amd:
      info->cs.uses_rt = true;
      case nir_intrinsic_load_poly_line_smooth_enabled:
      info->ps.needs_poly_line_smooth = true;
      case nir_intrinsic_begin_invocation_interlock:
      info->ps.pops = true;
      default:
            }
      static void
   gather_tex_info(const nir_shader *nir, const nir_tex_instr *instr, struct radv_shader_info *info)
   {
      for (unsigned i = 0; i < instr->num_srcs; i++) {
      switch (instr->src[i].src_type) {
   case nir_tex_src_texture_deref:
      mark_sampler_desc(nir_deref_instr_get_variable(nir_src_as_deref(instr->src[i].src)), info);
      case nir_tex_src_sampler_deref:
      mark_sampler_desc(nir_deref_instr_get_variable(nir_src_as_deref(instr->src[i].src)), info);
      default:
               }
      static void
   gather_info_block(const nir_shader *nir, const nir_block *block, struct radv_shader_info *info, bool consider_force_vrs)
   {
      nir_foreach_instr (instr, block) {
      switch (instr->type) {
   case nir_instr_type_intrinsic:
      gather_intrinsic_info(nir, nir_instr_as_intrinsic(instr), info, consider_force_vrs);
      case nir_instr_type_tex:
      gather_tex_info(nir, nir_instr_as_tex(instr), info);
      default:
               }
      static void
   mark_16bit_ps_input(struct radv_shader_info *info, const struct glsl_type *type, int location)
   {
      if (glsl_type_is_scalar(type) || glsl_type_is_vector(type) || glsl_type_is_matrix(type)) {
      unsigned attrib_count = glsl_count_attribute_slots(type, false);
   if (glsl_type_is_16bit(type)) {
            } else if (glsl_type_is_array(type)) {
      unsigned stride = glsl_count_attribute_slots(glsl_get_array_element(type), false);
   for (unsigned i = 0; i < glsl_get_length(type); ++i) {
            } else {
      assert(glsl_type_is_struct_or_ifc(type));
   for (unsigned i = 0; i < glsl_get_length(type); i++) {
      mark_16bit_ps_input(info, glsl_get_struct_field(type, i), location);
            }
      static void
   gather_xfb_info(const nir_shader *nir, struct radv_shader_info *info)
   {
               if (!nir->xfb_info)
            const nir_xfb_info *xfb = nir->xfb_info;
   assert(xfb->output_count <= MAX_SO_OUTPUTS);
            for (unsigned i = 0; i < xfb->output_count; i++) {
      unsigned output_buffer = xfb->outputs[i].buffer;
   unsigned stream = xfb->buffer_to_stream[xfb->outputs[i].buffer];
               for (unsigned i = 0; i < NIR_MAX_XFB_BUFFERS; i++) {
            }
      static void
   assign_outinfo_param(struct radv_vs_output_info *outinfo, gl_varying_slot idx, unsigned *total_param_exports,
         {
      if (outinfo->vs_output_param_offset[idx] == AC_EXP_PARAM_UNDEFINED)
      }
      static void
   assign_outinfo_params(struct radv_vs_output_info *outinfo, uint64_t mask, unsigned *total_param_exports,
         {
      u_foreach_bit64 (idx, mask) {
      if (idx >= VARYING_SLOT_VAR0 || idx == VARYING_SLOT_LAYER || idx == VARYING_SLOT_PRIMITIVE_ID ||
      idx == VARYING_SLOT_VIEWPORT)
      }
      static uint8_t
   radv_get_wave_size(struct radv_device *device, gl_shader_stage stage, const struct radv_shader_info *info,
         {
      if (stage_key->subgroup_required_size)
            if (stage == MESA_SHADER_GEOMETRY && !info->is_ngg)
         else if (stage == MESA_SHADER_COMPUTE)
         else if (stage == MESA_SHADER_FRAGMENT)
         else if (stage == MESA_SHADER_TASK)
         else if (gl_shader_stage_is_rt(stage))
         else
      }
      static uint8_t
   radv_get_ballot_bit_size(struct radv_device *device, gl_shader_stage stage, const struct radv_shader_info *info,
         {
      if (stage_key->subgroup_required_size)
               }
      static uint32_t
   radv_compute_esgs_itemsize(const struct radv_device *device, uint32_t num_varyings)
   {
                        /* For the ESGS ring in LDS, add 1 dword to reduce LDS bank
   * conflicts, i.e. each vertex will start on a different bank.
   */
   if (device->physical_device->rad_info.gfx_level >= GFX9 && esgs_itemsize)
               }
      static void
   gather_info_input_decl_vs(const nir_shader *nir, unsigned location, const struct glsl_type *type,
         {
      if (glsl_type_is_scalar(type) || glsl_type_is_vector(type)) {
      if (key->vs.instance_rate_inputs & BITFIELD_BIT(location)) {
      info->vs.needs_instance_id = true;
               if (info->vs.use_per_attribute_vb_descs)
         else
               } else if (glsl_type_is_matrix(type) || glsl_type_is_array(type)) {
      const struct glsl_type *elem = glsl_get_array_element(type);
            for (unsigned i = 0; i < glsl_get_length(type); ++i)
      } else {
               for (unsigned i = 0; i < glsl_get_length(type); i++) {
      const struct glsl_type *field = glsl_get_struct_field(type, i);
   gather_info_input_decl_vs(nir, location, field, key, info);
            }
      static void
   gather_shader_info_vs(struct radv_device *device, const nir_shader *nir, const struct radv_pipeline_key *pipeline_key,
         {
      if (pipeline_key->vs.has_prolog && nir->info.inputs_read) {
      info->vs.has_prolog = true;
               /* Use per-attribute vertex descriptors to prevent faults and for correct bounds checking. */
            /* We have to ensure consistent input register assignments between the main shader and the
   * prolog.
   */
   info->vs.needs_instance_id |= info->vs.has_prolog;
   info->vs.needs_base_instance |= info->vs.has_prolog;
            nir_foreach_shader_in_variable (var, nir)
            if (info->vs.dynamic_inputs)
            /* When the topology is unknown (with GPL), the number of vertices per primitive needs be passed
   * through a user SGPR for NGG streamout with VS. Otherwise, the XFB offset is incorrectly
   * computed because using the maximum number of vertices can't work.
   */
   info->vs.dynamic_num_verts_per_prim =
            if (!info->outputs_linked)
            if (info->next_stage == MESA_SHADER_TESS_CTRL) {
         } else if (info->next_stage == MESA_SHADER_GEOMETRY) {
      info->vs.as_es = true;
         }
      static void
   gather_shader_info_tcs(struct radv_device *device, const nir_shader *nir, const struct radv_pipeline_key *pipeline_key,
         {
      info->tcs.tcs_vertices_out = nir->info.tess.tcs_vertices_out;
   info->tcs.tes_inputs_read = ~0ULL;
            if (!info->inputs_linked)
         if (!info->outputs_linked) {
      info->tcs.num_linked_outputs = util_last_bit64(nir->info.outputs_written);
               if (!(pipeline_key->dynamic_patch_control_points)) {
      /* Number of tessellation patches per workgroup processed by the current pipeline. */
   info->num_tess_patches =
      get_tcs_num_patches(pipeline_key->tcs.tess_input_vertices, nir->info.tess.tcs_vertices_out,
                     /* LDS size used by VS+TCS for storing TCS inputs and outputs. */
   info->tcs.num_lds_blocks =
      calculate_tess_lds_size(device->physical_device->rad_info.gfx_level, pipeline_key->tcs.tess_input_vertices,
                  /* By default, assume a TCS needs an epilog unless it's linked with a TES. */
      }
      static void
   gather_shader_info_tes(struct radv_device *device, const nir_shader *nir, struct radv_shader_info *info)
   {
      info->tes._primitive_mode = nir->info.tess._primitive_mode;
   info->tes.spacing = nir->info.tess.spacing;
   info->tes.ccw = nir->info.tess.ccw;
   info->tes.point_mode = nir->info.tess.point_mode;
   info->tes.tcs_vertices_out = nir->info.tess.tcs_vertices_out;
   info->tes.reads_tess_factors =
            if (!info->inputs_linked)
         if (!info->outputs_linked)
            if (info->next_stage == MESA_SHADER_GEOMETRY) {
      info->tes.as_es = true;
         }
      static void
   radv_init_legacy_gs_ring_info(const struct radv_device *device, struct radv_shader_info *gs_info)
   {
      const struct radv_physical_device *pdevice = device->physical_device;
   struct radv_legacy_gs_info *gs_ring_info = &gs_info->gs_ring_info;
   unsigned num_se = pdevice->rad_info.max_se;
   unsigned wave_size = 64;
   unsigned max_gs_waves = 32 * num_se; /* max 32 per SE on GCN */
   /* On GFX6-GFX7, the value comes from VGT_GS_VERTEX_REUSE = 16.
   * On GFX8+, the value comes from VGT_VERTEX_REUSE_BLOCK_CNTL = 30 (+2).
   */
   unsigned gs_vertex_reuse = (pdevice->rad_info.gfx_level >= GFX8 ? 32 : 16) * num_se;
   unsigned alignment = 256 * num_se;
   /* The maximum size is 63.999 MB per SE. */
            /* Calculate the minimum size. */
   unsigned min_esgs_ring_size =
         /* These are recommended sizes, not minimum sizes. */
   unsigned esgs_ring_size =
                  min_esgs_ring_size = align(min_esgs_ring_size, alignment);
   esgs_ring_size = align(esgs_ring_size, alignment);
            if (pdevice->rad_info.gfx_level <= GFX8)
               }
      static void
   radv_get_legacy_gs_info(const struct radv_device *device, struct radv_shader_info *gs_info)
   {
      struct radv_legacy_gs_info *out = &gs_info->gs_ring_info;
   const unsigned gs_num_invocations = MAX2(gs_info->gs.invocations, 1);
   const bool uses_adjacency =
            /* All these are in dwords: */
   /* We can't allow using the whole LDS, because GS waves compete with
   * other shader stages for LDS space. */
   const unsigned max_lds_size = 8 * 1024;
   const unsigned esgs_itemsize = radv_compute_esgs_itemsize(device, gs_info->gs.num_linked_inputs) / 4;
            /* All these are per subgroup: */
   const unsigned max_out_prims = 32 * 1024;
   const unsigned max_es_verts = 255;
   const unsigned ideal_gs_prims = 64;
   unsigned max_gs_prims, gs_prims;
            if (uses_adjacency || gs_num_invocations > 1)
         else
            /* MAX_PRIMS_PER_SUBGROUP = gs_prims * max_vert_out * gs_invocations.
   * Make sure we don't go over the maximum value.
   */
   if (gs_info->gs.vertices_out > 0) {
         }
            /* If the primitive has adjacency, halve the number of vertices
   * that will be reused in multiple primitives.
   */
            gs_prims = MIN2(ideal_gs_prims, max_gs_prims);
            /* Compute ESGS LDS size based on the worst case number of ES vertices
   * needed to create the target number of GS prims per subgroup.
   */
            /* If total LDS usage is too big, refactor partitions based on ratio
   * of ESGS item sizes.
   */
   if (esgs_lds_size > max_lds_size) {
      /* Our target GS Prims Per Subgroup was too large. Calculate
   * the maximum number of GS Prims Per Subgroup that will fit
   * into LDS, capped by the maximum that the hardware can support.
   */
   gs_prims = MIN2((max_lds_size / (esgs_itemsize * min_es_verts)), max_gs_prims);
   assert(gs_prims > 0);
            esgs_lds_size = esgs_itemsize * worst_case_es_verts;
               /* Now calculate remaining ESGS information. */
   if (esgs_lds_size)
         else
            /* Vertices for adjacency primitives are not always reused, so restore
   * it for ES_VERTS_PER_SUBGRP.
   */
            /* For normal primitives, the VGT only checks if they are past the ES
   * verts per subgroup after allocating a full GS primitive and if they
   * are, kick off a new subgroup.  But if those additional ES verts are
   * unique (e.g. not reused) we need to make sure there is enough LDS
   * space to account for those ES verts beyond ES_VERTS_PER_SUBGRP.
   */
            const uint32_t es_verts_per_subgroup = es_verts;
   const uint32_t gs_prims_per_subgroup = gs_prims;
   const uint32_t gs_inst_prims_in_subgroup = gs_prims * gs_num_invocations;
   const uint32_t max_prims_per_subgroup = gs_inst_prims_in_subgroup * gs_info->gs.vertices_out;
   const uint32_t lds_granularity = device->physical_device->rad_info.lds_encode_granularity;
   const uint32_t total_lds_bytes = align(esgs_lds_size * 4, lds_granularity);
   out->lds_size = total_lds_bytes / lds_granularity;
   out->vgt_gs_onchip_cntl = S_028A44_ES_VERTS_PER_SUBGRP(es_verts_per_subgroup) |
               out->vgt_gs_max_prims_per_subgroup = S_028A94_MAX_PRIMS_PER_SUBGROUP(max_prims_per_subgroup);
   out->vgt_esgs_ring_itemsize = esgs_itemsize;
               }
      static void
   gather_shader_info_gs(struct radv_device *device, const nir_shader *nir, struct radv_shader_info *info)
   {
      unsigned add_clip = nir->info.clip_distance_array_size + nir->info.cull_distance_array_size > 4;
   info->gs.gsvs_vertex_size = (util_bitcount64(nir->info.outputs_written) + add_clip) * 16;
            info->gs.vertices_in = nir->info.gs.vertices_in;
   info->gs.vertices_out = nir->info.gs.vertices_out;
   info->gs.input_prim = nir->info.gs.input_primitive;
   info->gs.output_prim = nir->info.gs.output_primitive;
   info->gs.invocations = nir->info.gs.invocations;
            nir_foreach_shader_out_variable (var, nir) {
      unsigned num_components = glsl_get_component_slots(var->type);
                                          if (!info->inputs_linked)
            if (!info->is_ngg)
      }
      static void
   gather_shader_info_mesh(const nir_shader *nir, const struct radv_pipeline_key *pipeline_key,
         {
                        /* Special case for mesh shader workgroups.
   *
   * Mesh shaders don't have any real vertex input, but they can produce
   * an arbitrary number of vertices and primitives (up to 256).
   * We need to precisely control the number of mesh shader workgroups
   * that are launched from draw calls.
   *
   * To achieve that, we set:
   * - input primitive topology to point list
   * - input vertex and primitive count to 1
   * - max output vertex count and primitive amplification factor
   *   to the boundaries of the shader
   *
   * With that, in the draw call:
   * - drawing 1 input vertex ~ launching 1 mesh shader workgroup
   *
   * In the shader:
   * - input vertex id ~ workgroup id (in 1D - shader needs to calculate in 3D)
   *
   * Notes:
   * - without GS_EN=1 PRIM_AMP_FACTOR and MAX_VERTS_PER_SUBGROUP don't seem to work
   * - with GS_EN=1 we must also set VGT_GS_MAX_VERT_OUT (otherwise the GPU hangs)
   * - with GS_FAST_LAUNCH=1 every lane's VGPRs are initialized to the same input vertex index
   *
   */
   ngg_info->esgs_ring_size = 1;
   ngg_info->hw_max_esverts = 1;
   ngg_info->max_gsprims = 1;
   ngg_info->max_out_verts = nir->info.mesh.max_vertices_out;
   ngg_info->max_vert_out_per_gs_instance = false;
   ngg_info->ngg_emit_size = 0;
   ngg_info->prim_amp_factor = nir->info.mesh.max_primitives_out;
               }
      static void
   calc_mesh_workgroup_size(const struct radv_device *device, const nir_shader *nir, struct radv_shader_info *info)
   {
               if (device->mesh_fast_launch_2) {
      /* Use multi-row export. It is also necessary to use the API workgroup size for non-emulated queries. */
      } else {
      struct gfx10_ngg_info *ngg_info = &info->ngg_info;
   unsigned min_ngg_workgroup_size = ac_compute_ngg_workgroup_size(
                  }
      static void
   gather_shader_info_fs(const struct radv_device *device, const nir_shader *nir,
         {
      uint64_t per_primitive_input_mask = nir->info.inputs_read & nir->info.per_primitive_inputs;
   unsigned num_per_primitive_inputs = util_bitcount64(per_primitive_input_mask);
            info->ps.num_interp = nir->num_inputs;
            if (device->physical_device->rad_info.gfx_level == GFX10_3) {
      /* GFX10.3 distinguishes NUM_INTERP and NUM_PRIM_INTERP, but
   * these are counted together in NUM_INTERP on GFX11.
   */
   info->ps.num_interp = nir->num_inputs - num_per_primitive_inputs;
               info->ps.can_discard = nir->info.fs.uses_discard;
   info->ps.early_fragment_test =
      nir->info.fs.early_fragment_tests ||
   (nir->info.fs.early_and_late_fragment_tests && nir->info.fs.depth_layout == FRAG_DEPTH_LAYOUT_NONE &&
   nir->info.fs.stencil_front_layout == FRAG_STENCIL_LAYOUT_NONE &&
      info->ps.post_depth_coverage = nir->info.fs.post_depth_coverage;
   info->ps.depth_layout = nir->info.fs.depth_layout;
   info->ps.uses_sample_shading = nir->info.fs.uses_sample_shading;
   info->ps.writes_memory = nir->info.writes_memory;
   info->ps.has_pcoord = nir->info.inputs_read & VARYING_BIT_PNTC;
   info->ps.prim_id_input = nir->info.inputs_read & VARYING_BIT_PRIMITIVE_ID;
   info->ps.layer_input = nir->info.inputs_read & VARYING_BIT_LAYER;
   info->ps.viewport_index_input = nir->info.inputs_read & VARYING_BIT_VIEWPORT;
   info->ps.writes_z = nir->info.outputs_written & BITFIELD64_BIT(FRAG_RESULT_DEPTH);
   info->ps.writes_stencil = nir->info.outputs_written & BITFIELD64_BIT(FRAG_RESULT_STENCIL);
   info->ps.writes_sample_mask = nir->info.outputs_written & BITFIELD64_BIT(FRAG_RESULT_SAMPLE_MASK);
   info->ps.reads_sample_mask_in = BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_SAMPLE_MASK_IN);
   info->ps.reads_sample_id = BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_SAMPLE_ID);
   info->ps.reads_frag_shading_rate = BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_FRAG_SHADING_RATE);
   info->ps.reads_front_face = BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_FRONT_FACE);
   info->ps.reads_barycentric_model = BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_BARYCENTRIC_PULL_MODEL);
            bool uses_persp_or_linear_interp = info->ps.reads_persp_center || info->ps.reads_persp_centroid ||
                  info->ps.allow_flat_shading =
      !(uses_persp_or_linear_interp || info->ps.needs_sample_positions || info->ps.reads_frag_shading_rate ||
   info->ps.writes_memory || nir->info.fs.needs_quad_helper_invocations ||
   BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_FRAG_COORD) ||
   BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_POINT_COORD) ||
   BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_SAMPLE_ID) ||
   BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_SAMPLE_POS) ||
   BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_SAMPLE_MASK_IN) ||
         info->ps.pops_is_per_sample =
                              info->ps.writes_mrt0_alpha = (pipeline_key->ps.alpha_to_coverage_via_mrtz && (info->ps.color0_written & 0x8)) &&
                              nir_foreach_shader_in_variable (var, nir) {
      const struct glsl_type *type = var->data.per_vertex ? glsl_get_array_element(var->type) : var->type;
   unsigned attrib_count = glsl_count_attribute_slots(type, false);
            switch (idx) {
   case VARYING_SLOT_CLIP_DIST0:
   case VARYING_SLOT_CLIP_DIST1:
      info->ps.num_input_clips_culls += attrib_count;
      default:
                  if (var->data.compact) {
      unsigned component_count = var->data.location_frac + glsl_get_length(var->type);
      } else {
                           if (!var->data.per_primitive) {
      if (var->data.interpolation == INTERP_MODE_FLAT)
         else if (var->data.interpolation == INTERP_MODE_EXPLICIT)
         else if (var->data.per_vertex)
               if (var->data.location >= VARYING_SLOT_VAR0) {
      if (var->data.per_primitive)
         else
                  /* Disable VRS and use the rates from PS_ITER_SAMPLES if:
   *
   * - The fragment shader reads gl_SampleMaskIn because the 16-bit sample coverage mask isn't enough for MSAA8x and
   *   2x2 coarse shading.
   * - On GFX10.3, if the fragment shader requests a fragment interlock execution mode even if the ordered section was
   *   optimized out, to consistently implement fragmentShadingRateWithFragmentShaderInterlock = VK_FALSE.
   */
   info->ps.force_sample_iter_shading_rate =
      (info->ps.reads_sample_mask_in && !info->ps.needs_poly_line_smooth) ||
   (device->physical_device->rad_info.gfx_level == GFX10_3 &&
   (nir->info.fs.sample_interlock_ordered || nir->info.fs.sample_interlock_unordered ||
                  unsigned conservative_z_export = V_02880C_EXPORT_ANY_Z;
   if (info->ps.depth_layout == FRAG_DEPTH_LAYOUT_GREATER)
         else if (info->ps.depth_layout == FRAG_DEPTH_LAYOUT_LESS)
            unsigned z_order =
            /* It shouldn't be needed to export gl_SampleMask when MSAA is disabled, but this appears to break Project Cars
   * (DXVK). See https://bugs.freedesktop.org/show_bug.cgi?id=109401
   */
            const bool disable_rbplus =
            info->ps.db_shader_control =
      S_02880C_Z_EXPORT_ENABLE(info->ps.writes_z) | S_02880C_STENCIL_TEST_VAL_EXPORT_ENABLE(info->ps.writes_stencil) |
   S_02880C_KILL_ENABLE(info->ps.can_discard) | S_02880C_MASK_EXPORT_ENABLE(mask_export_enable) |
   S_02880C_CONSERVATIVE_Z_EXPORT(conservative_z_export) | S_02880C_Z_ORDER(z_order) |
   S_02880C_DEPTH_BEFORE_SHADER(info->ps.early_fragment_test) |
   S_02880C_PRE_SHADER_DEPTH_COVERAGE_ENABLE(info->ps.post_depth_coverage) |
   S_02880C_EXEC_ON_HIER_FAIL(info->ps.writes_memory) | S_02880C_EXEC_ON_NOOP(info->ps.writes_memory) |
   }
      static void
   gather_shader_info_rt(const nir_shader *nir, struct radv_shader_info *info)
   {
      // TODO: inline push_constants again
   info->loads_dynamic_offsets = true;
   info->loads_push_constants = true;
   info->can_inline_all_push_constants = false;
   info->inline_push_constant_mask = 0;
      }
      static void
   gather_shader_info_cs(struct radv_device *device, const nir_shader *nir, const struct radv_pipeline_key *pipeline_key,
         {
               unsigned default_wave_size = device->physical_device->cs_wave_size;
   if (info->cs.uses_rt)
                     /* Games don't always request full subgroups when they should, which can cause bugs if cswave32
   * is enabled. Furthermore, if cooperative matrices or subgroup info are used, we can't transparently change
   * the subgroup size.
   */
   const bool require_full_subgroups =
      pipeline_key->stage_info[MESA_SHADER_COMPUTE].subgroup_require_full || nir->info.cs.has_cooperative_matrix ||
                  if (required_subgroup_size) {
         } else if (require_full_subgroups) {
         } else if (device->physical_device->rad_info.gfx_level >= GFX10 && local_size <= 32) {
      /* Use wave32 for small workgroups. */
      } else {
                  if (device->physical_device->rad_info.has_cs_regalloc_hang_bug) {
            }
      static void
   gather_shader_info_task(const nir_shader *nir, const struct radv_pipeline_key *pipeline_key,
         {
      /* Task shaders always need these for the I/O lowering even if the API shader doesn't actually
   * use them.
            /* Needed to address the task draw/payload rings. */
   info->cs.uses_block_id[0] = true;
   info->cs.uses_block_id[1] = true;
   info->cs.uses_block_id[2] = true;
            /* Needed for storing draw ready only on the 1st thread. */
            /* Task->Mesh dispatch is linear when Y = Z = 1.
   * GFX11 CP can optimize this case with a field in its draw packets.
   */
   info->cs.linear_taskmesh_dispatch =
               }
      static uint32_t
   radv_get_user_data_0(const struct radv_device *device, struct radv_shader_info *info)
   {
               switch (info->stage) {
   case MESA_SHADER_VERTEX:
   case MESA_SHADER_TESS_EVAL:
   case MESA_SHADER_MESH:
      if (info->next_stage == MESA_SHADER_TESS_CTRL) {
               if (gfx_level >= GFX10) {
         } else if (gfx_level == GFX9) {
         } else {
                     if (info->next_stage == MESA_SHADER_GEOMETRY) {
               if (gfx_level >= GFX10) {
         } else {
                     if (info->is_ngg)
            assert(info->stage != MESA_SHADER_MESH);
      case MESA_SHADER_TESS_CTRL:
         case MESA_SHADER_GEOMETRY:
         case MESA_SHADER_FRAGMENT:
         case MESA_SHADER_COMPUTE:
   case MESA_SHADER_TASK:
   case MESA_SHADER_RAYGEN:
   case MESA_SHADER_CALLABLE:
   case MESA_SHADER_CLOSEST_HIT:
   case MESA_SHADER_MISS:
   case MESA_SHADER_INTERSECTION:
   case MESA_SHADER_ANY_HIT:
         default:
            }
      static bool
   radv_is_merged_shader_compiled_separately(const struct radv_device *device, const struct radv_shader_info *info)
   {
               if (gfx_level >= GFX9) {
      switch (info->stage) {
   case MESA_SHADER_VERTEX:
      if (info->next_stage == MESA_SHADER_TESS_CTRL || info->next_stage == MESA_SHADER_GEOMETRY)
            case MESA_SHADER_TESS_EVAL:
      if (info->next_stage == MESA_SHADER_GEOMETRY)
            case MESA_SHADER_TESS_CTRL:
   case MESA_SHADER_GEOMETRY:
         default:
                        }
      void
   radv_nir_shader_info_init(gl_shader_stage stage, gl_shader_stage next_stage, struct radv_shader_info *info)
   {
               /* Assume that shaders can inline all push constants by default. */
            info->stage = stage;
      }
      void
   radv_nir_shader_info_pass(struct radv_device *device, const struct nir_shader *nir,
                     {
               if (layout->use_dynamic_descriptors) {
      info->loads_push_constants = true;
               nir_foreach_block (block, func->impl) {
                  if (nir->info.stage == MESA_SHADER_VERTEX || nir->info.stage == MESA_SHADER_TESS_EVAL ||
      nir->info.stage == MESA_SHADER_GEOMETRY)
         if (nir->info.stage == MESA_SHADER_VERTEX || nir->info.stage == MESA_SHADER_TESS_EVAL ||
      nir->info.stage == MESA_SHADER_GEOMETRY || nir->info.stage == MESA_SHADER_MESH) {
            /* These are not compiled into neither output param nor position exports. */
   uint64_t special_mask = BITFIELD64_BIT(VARYING_SLOT_PRIMITIVE_COUNT) |
               uint64_t per_prim_mask = nir->info.outputs_written & nir->info.per_primitive_outputs & ~special_mask;
            /* Mesh multivew is only lowered in ac_nir_lower_ngg, so we have to fake it here. */
   if (nir->info.stage == MESA_SHADER_MESH && pipeline_key->has_multiview_view_index) {
      per_prim_mask |= VARYING_BIT_LAYER;
               /* Per vertex outputs. */
   outinfo->writes_pointsize = per_vtx_mask & VARYING_BIT_PSIZ;
   outinfo->writes_viewport_index = per_vtx_mask & VARYING_BIT_VIEWPORT;
   outinfo->writes_layer = per_vtx_mask & VARYING_BIT_LAYER;
   outinfo->writes_primitive_shading_rate =
            /* Per primitive outputs. */
   outinfo->writes_viewport_index_per_primitive = per_prim_mask & VARYING_BIT_VIEWPORT;
   outinfo->writes_layer_per_primitive = per_prim_mask & VARYING_BIT_LAYER;
            /* Clip/cull distances. */
   outinfo->clip_dist_mask = (1 << nir->info.clip_distance_array_size) - 1;
   outinfo->cull_dist_mask = (1 << nir->info.cull_distance_array_size) - 1;
                     if (outinfo->writes_pointsize || outinfo->writes_viewport_index || outinfo->writes_layer ||
                  unsigned num_clip_distances = util_bitcount(outinfo->clip_dist_mask);
            if (num_clip_distances + num_cull_distances > 0)
         if (num_clip_distances + num_cull_distances > 4)
                                       /* Per-vertex outputs */
                     /* The HW always assumes that there is at least 1 per-vertex param.
   * so if there aren't any, we have to offset per-primitive params by 1.
   */
   const unsigned extra_offset =
            /* Per-primitive outputs: the HW needs these to be last. */
                        info->vs.needs_draw_id |= BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_DRAW_ID);
   info->vs.needs_base_instance |= BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_BASE_INSTANCE);
   info->vs.needs_instance_id |= BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_INSTANCE_ID);
   info->uses_view_index |= BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_VIEW_INDEX);
   info->uses_invocation_id |= BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_INVOCATION_ID);
            /* Used by compute and mesh shaders. Mesh shaders must always declare this before GFX11. */
   info->cs.uses_grid_size =
      BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_NUM_WORKGROUPS) ||
      info->cs.uses_local_invocation_idx = BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_LOCAL_INVOCATION_INDEX) |
                  if (nir->info.stage == MESA_SHADER_COMPUTE || nir->info.stage == MESA_SHADER_TASK ||
      nir->info.stage == MESA_SHADER_MESH) {
   for (int i = 0; i < 3; ++i)
               info->user_data_0 = radv_get_user_data_0(device, info);
            switch (nir->info.stage) {
   case MESA_SHADER_COMPUTE:
      gather_shader_info_cs(device, nir, pipeline_key, info);
      case MESA_SHADER_TASK:
      gather_shader_info_task(nir, pipeline_key, info);
      case MESA_SHADER_FRAGMENT:
      gather_shader_info_fs(device, nir, pipeline_key, info);
      case MESA_SHADER_GEOMETRY:
      gather_shader_info_gs(device, nir, info);
      case MESA_SHADER_TESS_EVAL:
      gather_shader_info_tes(device, nir, info);
      case MESA_SHADER_TESS_CTRL:
      gather_shader_info_tcs(device, nir, pipeline_key, info);
      case MESA_SHADER_VERTEX:
      gather_shader_info_vs(device, nir, pipeline_key, info);
      case MESA_SHADER_MESH:
      gather_shader_info_mesh(nir, pipeline_key, info);
      default:
      if (gl_shader_stage_is_rt(nir->info.stage))
                     const struct radv_shader_stage_key *stage_key = &pipeline_key->stage_info[nir->info.stage];
   info->wave_size = radv_get_wave_size(device, nir->info.stage, info, stage_key);
            switch (nir->info.stage) {
   case MESA_SHADER_COMPUTE:
   case MESA_SHADER_TASK:
               /* Allow the compiler to assume that the shader always has full subgroups,
   * meaning that the initial EXEC mask is -1 in all waves (all lanes enabled).
   * This assumption is incorrect for ray tracing and internal (meta) shaders
   * because they can use unaligned dispatch.
   */
   info->cs.uses_full_subgroups = pipeline_type != RADV_PIPELINE_RAY_TRACING && !nir->info.internal &&
            case MESA_SHADER_MESH:
      calc_mesh_workgroup_size(device, nir, info);
      default:
      /* FS always operates without workgroups. Other stages are computed during linking but assume
   * no workgroups by default.
   */
   info->workgroup_size = info->wave_size;
         }
      static void
   clamp_gsprims_to_esverts(unsigned *max_gsprims, unsigned max_esverts, unsigned min_verts_per_prim, bool use_adjacency)
   {
      unsigned max_reuse = max_esverts - min_verts_per_prim;
   if (use_adjacency)
            }
      static unsigned
   radv_get_num_input_vertices(const struct radv_shader_stage *es_stage, const struct radv_shader_stage *gs_stage)
   {
      if (gs_stage) {
                  if (es_stage->stage == MESA_SHADER_TESS_EVAL) {
      if (es_stage->nir->info.tess.point_mode)
         if (es_stage->nir->info.tess._primitive_mode == TESS_PRIMITIVE_ISOLINES)
                        }
      static unsigned
   radv_get_pre_rast_input_topology(const struct radv_shader_stage *es_stage, const struct radv_shader_stage *gs_stage)
   {
      if (gs_stage) {
                  if (es_stage->stage == MESA_SHADER_TESS_EVAL) {
      if (es_stage->nir->info.tess.point_mode)
         if (es_stage->nir->info.tess._primitive_mode == TESS_PRIMITIVE_ISOLINES)
                        }
      static void
   gfx10_get_ngg_info(const struct radv_device *device, struct radv_shader_stage *es_stage,
         {
      const enum amd_gfx_level gfx_level = device->physical_device->rad_info.gfx_level;
   struct radv_shader_info *gs_info = gs_stage ? &gs_stage->info : NULL;
   struct radv_shader_info *es_info = &es_stage->info;
   const unsigned max_verts_per_prim = radv_get_num_input_vertices(es_stage, gs_stage);
   const unsigned min_verts_per_prim = gs_stage ? max_verts_per_prim : 1;
                     const unsigned input_prim = radv_get_pre_rast_input_topology(es_stage, gs_stage);
            /* All these are in dwords: */
   /* We can't allow using the whole LDS, because GS waves compete with
   * other shader stages for LDS space.
   *
   * TODO: We should really take the shader's internal LDS use into
   *       account. The linker will fail if the size is greater than
   *       8K dwords.
   */
   const unsigned max_lds_size = 8 * 1024 - 768;
   const unsigned target_lds_size = max_lds_size;
   unsigned esvert_lds_size = 0;
            /* All these are per subgroup: */
   const unsigned min_esverts = gfx_level >= GFX11 ? 3 : /* gfx11 requires at least 1 primitive per TG */
               bool max_vert_out_per_gs_instance = false;
   unsigned max_esverts_base = 128;
            /* Hardware has the following non-natural restrictions on the value
   * of GE_CNTL.VERT_GRP_SIZE based on based on the primitive type of
   * the draw:
   *  - at most 252 for any line input primitive type
   *  - at most 251 for any quad input primitive type
   *  - at most 251 for triangle strips with adjacency (this happens to
   *    be the natural limit for triangle *lists* with adjacency)
   */
            if (gs_stage) {
               if (max_out_verts_per_gsprim <= 256) {
      if (max_out_verts_per_gsprim) {
            } else {
      /* Use special multi-cycling mode in which each GS
   * instance gets its own subgroup. Does not work with
   * tessellation. */
   max_vert_out_per_gs_instance = true;
   max_gsprims_base = 1;
               esvert_lds_size = es_info->esgs_itemsize / 4;
      } else {
      /* VS and TES. */
   /* LDS size for passing data from GS to ES. */
            if (so_info->num_outputs) {
      /* Compute the same pervertex LDS size as the NGG streamout lowering pass which allocates
   * space for all outputs.
   * TODO: only alloc space for outputs that really need streamout.
   */
               /* GS stores Primitive IDs (one DWORD) into LDS at the address
   * corresponding to the ES thread of the provoking vertex. All
   * ES threads load and export PrimitiveID for their thread.
   */
   if (es_stage->stage == MESA_SHADER_VERTEX && es_stage->info.outinfo.export_prim_id)
               unsigned max_gsprims = max_gsprims_base;
            if (esvert_lds_size)
         if (gsprim_lds_size)
            max_esverts = MIN2(max_esverts, max_gsprims * max_verts_per_prim);
   clamp_gsprims_to_esverts(&max_gsprims, max_esverts, min_verts_per_prim, uses_adjacency);
            if (esvert_lds_size || gsprim_lds_size) {
      /* Now that we have a rough proportionality between esverts
   * and gsprims based on the primitive type, scale both of them
   * down simultaneously based on required LDS space.
   *
   * We could be smarter about this if we knew how much vertex
   * reuse to expect.
   */
   unsigned lds_total = max_esverts * esvert_lds_size + max_gsprims * gsprim_lds_size;
   if (lds_total > target_lds_size) {
                     max_esverts = MIN2(max_esverts, max_gsprims * max_verts_per_prim);
   clamp_gsprims_to_esverts(&max_gsprims, max_esverts, min_verts_per_prim, uses_adjacency);
                  /* Round up towards full wave sizes for better ALU utilization. */
   if (!max_vert_out_per_gs_instance) {
      unsigned orig_max_esverts;
   unsigned orig_max_gsprims;
            if (gs_stage) {
         } else {
                  do {
                     max_esverts = align(max_esverts, wavesize);
   max_esverts = MIN2(max_esverts, max_esverts_base);
   if (esvert_lds_size)
                  /* Hardware restriction: minimum value of max_esverts */
   if (gfx_level == GFX10)
                        max_gsprims = align(max_gsprims, wavesize);
   max_gsprims = MIN2(max_gsprims, max_gsprims_base);
   if (gsprim_lds_size) {
      /* Don't count unusable vertices to the LDS
   * size. Those are vertices above the maximum
   * number of vertices that can occur in the
   * workgroup, which is e.g. max_gsprims * 3
   * for triangles.
   */
   unsigned usable_esverts = MIN2(max_esverts, max_gsprims * max_verts_per_prim);
      }
   clamp_gsprims_to_esverts(&max_gsprims, max_esverts, min_verts_per_prim, uses_adjacency);
               /* Verify the restriction. */
   if (gfx_level == GFX10)
         else
      } else {
      /* Hardware restriction: minimum value of max_esverts */
   if (gfx_level == GFX10)
         else
               unsigned max_out_vertices = max_vert_out_per_gs_instance ? gs_info->gs.vertices_out
                        unsigned prim_amp_factor = 1;
   if (gs_stage) {
      /* Number of output primitives per GS input primitive after
   * GS instancing. */
               /* On Gfx10, the GE only checks against the maximum number of ES verts
   * after allocating a full GS primitive. So we need to ensure that
   * whenever this check passes, there is enough space for a full
   * primitive without vertex reuse.
   */
   if (gfx_level == GFX10)
         else
            out->max_gsprims = max_gsprims;
   out->max_out_verts = max_out_vertices;
   out->prim_amp_factor = prim_amp_factor;
   out->max_vert_out_per_gs_instance = max_vert_out_per_gs_instance;
            /* Don't count unusable vertices. */
            if (gs_stage) {
         } else {
                           unsigned workgroup_size =
         if (gs_stage) {
         }
      }
      static void
   gfx10_get_ngg_query_info(const struct radv_device *device, struct radv_shader_stage *es_stage,
         {
               info->gs.has_pipeline_stat_query = device->physical_device->emulate_ngg_gs_query_pipeline_stat && !!gs_stage;
   info->has_xfb_query = gs_stage ? !!gs_stage->nir->xfb_info : !!es_stage->nir->xfb_info;
      }
      static void
   radv_determine_ngg_settings(struct radv_device *device, struct radv_shader_stage *es_stage,
         {
      assert(es_stage->stage == MESA_SHADER_VERTEX || es_stage->stage == MESA_SHADER_TESS_EVAL);
                     unsigned num_vertices_per_prim = 0;
   if (es_stage->stage == MESA_SHADER_VERTEX) {
         } else if (es_stage->stage == MESA_SHADER_TESS_EVAL) {
      num_vertices_per_prim = es_stage->nir->info.tess.point_mode                                   ? 1
                     /* TODO: Enable culling for LLVM. */
   es_stage->info.has_ngg_culling = radv_consider_culling(device->physical_device, es_stage->nir, ps_inputs_read,
                  nir_function_impl *impl = nir_shader_get_entrypoint(es_stage->nir);
            /* NGG passthrough mode should be disabled when culling and when the vertex shader
   * exports the primitive ID.
   */
   es_stage->info.is_ngg_passthrough = !es_stage->info.has_ngg_culling && !(es_stage->stage == MESA_SHADER_VERTEX &&
      }
      static void
   radv_link_shaders_info(struct radv_device *device, struct radv_shader_stage *producer,
         {
      /* Export primitive ID and clip/cull distances if read by the FS, or export unconditionally when
   * the next stage is unknown (with graphics pipeline library).
   */
   if (producer->info.next_stage == MESA_SHADER_FRAGMENT ||
      !(pipeline_key->lib_flags & VK_GRAPHICS_PIPELINE_LIBRARY_FRAGMENT_SHADER_BIT_EXT)) {
   struct radv_vs_output_info *outinfo = &producer->info.outinfo;
   const bool ps_prim_id_in = !consumer || consumer->info.ps.prim_id_input;
            if (ps_prim_id_in && (producer->stage == MESA_SHADER_VERTEX || producer->stage == MESA_SHADER_TESS_EVAL)) {
      /* Mark the primitive ID as output when it's implicitly exported by VS or TES. */
                              if (ps_clip_dists_in) {
      if (producer->nir->info.outputs_written & VARYING_BIT_CLIP_DIST0)
         if (producer->nir->info.outputs_written & VARYING_BIT_CLIP_DIST1)
                  if (producer->stage == MESA_SHADER_VERTEX || producer->stage == MESA_SHADER_TESS_EVAL) {
      /* Compute NGG info (GFX10+) or GS info. */
   if (producer->info.is_ngg) {
                              /* Determine other NGG settings like culling for VS or TES without GS. */
   if (!gs_stage) {
            } else if (consumer && consumer->stage == MESA_SHADER_GEOMETRY) {
      struct radv_shader_info *gs_info = &consumer->info;
   struct radv_shader_info *es_info = &producer->info;
   unsigned es_verts_per_subgroup = G_028A44_ES_VERTS_PER_SUBGRP(gs_info->gs_ring_info.vgt_gs_onchip_cntl);
                  unsigned workgroup_size =
      ac_compute_esgs_workgroup_size(device->physical_device->rad_info.gfx_level, es_info->wave_size,
      es_info->workgroup_size = workgroup_size;
                  if (producer->stage == MESA_SHADER_VERTEX && consumer && consumer->stage == MESA_SHADER_TESS_CTRL) {
      struct radv_shader_stage *vs_stage = producer;
            if (pipeline_key->dynamic_patch_control_points) {
      /* Set the workgroup size to the maximum possible value to ensure that compilers don't
   * optimize barriers.
   */
   vs_stage->info.workgroup_size = 256;
      } else {
      vs_stage->info.workgroup_size = ac_compute_lshs_workgroup_size(
                  tcs_stage->info.workgroup_size = ac_compute_lshs_workgroup_size(
                  if (!radv_use_llvm_for_stage(device, MESA_SHADER_VERTEX)) {
      /* When the number of TCS input and output vertices are the same (typically 3):
   * - There is an equal amount of LS and HS invocations
   * - In case of merged LSHS shaders, the LS and HS halves of the shader always process
   *   the exact same vertex. We can use this knowledge to optimize them.
   *
   * We don't set tcs_in_out_eq if the float controls differ because that might involve
   * different float modes for the same block and our optimizer doesn't handle a
   * instruction dominating another with a different mode.
   */
   vs_stage->info.vs.tcs_in_out_eq =
                        if (vs_stage->info.vs.tcs_in_out_eq)
      vs_stage->info.vs.tcs_temp_only_input_mask =
      tcs_stage->nir->info.inputs_read & vs_stage->nir->info.outputs_written &
                     /* Copy shader info between TCS<->TES. */
   if (producer->stage == MESA_SHADER_TESS_CTRL && consumer && consumer->stage == MESA_SHADER_TESS_EVAL) {
      struct radv_shader_stage *tcs_stage = producer;
            tcs_stage->info.has_epilog = false;
   tcs_stage->info.tcs.tes_reads_tess_factors = tes_stage->info.tes.reads_tess_factors;
   tcs_stage->info.tcs.tes_inputs_read = tes_stage->nir->info.inputs_read;
            if (!pipeline_key->dynamic_patch_control_points)
               /* Task/mesh I/O uses the task ring buffers. */
   if (producer->stage == MESA_SHADER_TASK) {
            }
      static void
   radv_nir_shader_info_merge(const struct radv_shader_stage *src, struct radv_shader_stage *dst)
   {
      const struct radv_shader_info *src_info = &src->info;
            assert((src->stage == MESA_SHADER_VERTEX && dst->stage == MESA_SHADER_TESS_CTRL) ||
                  dst_info->loads_push_constants |= src_info->loads_push_constants;
   dst_info->loads_dynamic_offsets |= src_info->loads_dynamic_offsets;
   dst_info->desc_set_used_mask |= src_info->desc_set_used_mask;
   dst_info->uses_view_index |= src_info->uses_view_index;
   dst_info->uses_prim_id |= src_info->uses_prim_id;
            /* Only inline all push constants if both allows it. */
            if (src->stage == MESA_SHADER_VERTEX) {
         } else {
                  if (dst->stage == MESA_SHADER_GEOMETRY)
      }
      static const gl_shader_stage graphics_shader_order[] = {
                  };
      void
   radv_nir_shader_info_link(struct radv_device *device, const struct radv_pipeline_key *pipeline_key,
         {
      /* Walk backwards to link */
            for (int i = ARRAY_SIZE(graphics_shader_order) - 1; i >= 0; i--) {
      gl_shader_stage s = graphics_shader_order[i];
   if (!stages[s].nir)
            radv_link_shaders_info(device, &stages[s], next_stage, pipeline_key);
               if (device->physical_device->rad_info.gfx_level >= GFX9) {
      /* Merge shader info for VS+TCS. */
   if (stages[MESA_SHADER_TESS_CTRL].nir) {
                  /* Merge shader info for VS+GS or TES+GS. */
   if (stages[MESA_SHADER_GEOMETRY].nir) {
                        }
      enum ac_hw_stage
   radv_select_hw_stage(const struct radv_shader_info *const info, const enum amd_gfx_level gfx_level)
   {
      switch (info->stage) {
   case MESA_SHADER_VERTEX:
      if (info->is_ngg)
         else if (info->vs.as_es)
         else if (info->vs.as_ls)
         else
      case MESA_SHADER_TESS_EVAL:
      if (info->is_ngg)
         else if (info->tes.as_es)
         else
      case MESA_SHADER_TESS_CTRL:
         case MESA_SHADER_GEOMETRY:
      if (info->is_ngg)
         else
      case MESA_SHADER_MESH:
         case MESA_SHADER_FRAGMENT:
         case MESA_SHADER_COMPUTE:
   case MESA_SHADER_KERNEL:
   case MESA_SHADER_TASK:
   case MESA_SHADER_RAYGEN:
   case MESA_SHADER_ANY_HIT:
   case MESA_SHADER_CLOSEST_HIT:
   case MESA_SHADER_MISS:
   case MESA_SHADER_INTERSECTION:
   case MESA_SHADER_CALLABLE:
         default:
            }
