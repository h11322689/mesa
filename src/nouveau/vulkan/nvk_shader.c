   /*
   * Copyright © 2022 Collabora Ltd. and Red Hat Inc.
   * SPDX-License-Identifier: MIT
   */
   #include "nvk_shader.h"
      #include "nvk_cmd_buffer.h"
   #include "nvk_descriptor_set_layout.h"
   #include "nvk_device.h"
   #include "nvk_physical_device.h"
   #include "nvk_pipeline.h"
   #include "nvk_sampler.h"
      #include "vk_nir_convert_ycbcr.h"
   #include "vk_pipeline.h"
   #include "vk_pipeline_cache.h"
   #include "vk_pipeline_layout.h"
   #include "vk_shader_module.h"
   #include "vk_ycbcr_conversion.h"
      #include "nir.h"
   #include "nir_builder.h"
   #include "nir_xfb_info.h"
   #include "compiler/spirv/nir_spirv.h"
      #include "nv50_ir_driver.h"
      #include "util/mesa-sha1.h"
      #include "cla097.h"
   #include "clc397.h"
   #include "clc597.h"
   #include "nvk_cl9097.h"
      static void
   shared_var_info(const struct glsl_type *type, unsigned *size, unsigned *align)
   {
               uint32_t comp_size = glsl_type_is_boolean(type) ? 4 : glsl_get_bit_size(type) / 8;
   unsigned length = glsl_get_vector_elements(type);
      }
      static inline enum pipe_shader_type
   pipe_shader_type_from_mesa(gl_shader_stage stage)
   {
      switch (stage) {
   case MESA_SHADER_VERTEX:
         case MESA_SHADER_TESS_CTRL:
         case MESA_SHADER_TESS_EVAL:
         case MESA_SHADER_GEOMETRY:
         case MESA_SHADER_FRAGMENT:
         case MESA_SHADER_COMPUTE:
   case MESA_SHADER_KERNEL:
         default:
            }
      static uint64_t
   get_prog_debug(void)
   {
         }
      static uint64_t
   get_prog_optimize(void)
   {
         }
      uint64_t
   nvk_physical_device_compiler_flags(const struct nvk_physical_device *pdev)
   {
      uint64_t prog_debug = get_prog_debug();
            assert(prog_debug <= UINT8_MAX);
   assert(prog_optimize <= UINT8_MAX);
      }
      const nir_shader_compiler_options *
   nvk_physical_device_nir_options(const struct nvk_physical_device *pdev,
         {
      enum pipe_shader_type p_stage = pipe_shader_type_from_mesa(stage);
      }
      struct spirv_to_nir_options
   nvk_physical_device_spirv_options(const struct nvk_physical_device *pdev,
         {
      return (struct spirv_to_nir_options) {
      .caps = {
      .demote_to_helper_invocation = true,
   .descriptor_array_dynamic_indexing = true,
   .descriptor_array_non_uniform_indexing = true,
   .descriptor_indexing = true,
   .device_group = true,
   .draw_parameters = true,
   .geometry_streams = true,
   .image_read_without_format = true,
   .image_write_without_format = true,
   .min_lod = true,
   .multiview = true,
   .physical_storage_buffer_address = true,
   .runtime_descriptor_array = true,
   .shader_clock = true,
   .shader_viewport_index_layer = true,
   .tessellation = true,
   .transform_feedback = true,
   .variable_pointers = true,
      },
   .ssbo_addr_format = nvk_buffer_addr_format(rs->storage_buffers),
   .phys_ssbo_addr_format = nir_address_format_64bit_global,
   .ubo_addr_format = nvk_buffer_addr_format(rs->uniform_buffers),
   .shared_addr_format = nir_address_format_32bit_offset,
   .min_ssbo_alignment = NVK_MIN_SSBO_ALIGNMENT,
         }
      static bool
   lower_image_size_to_txs(nir_builder *b, nir_intrinsic_instr *intrin,
         {
      if (intrin->intrinsic != nir_intrinsic_image_deref_size)
                     nir_deref_instr *img = nir_src_as_deref(intrin->src[0]);
   nir_def *lod = nir_tex_type_has_lod(img->type) ?
                  if (glsl_get_sampler_dim(img->type) == GLSL_SAMPLER_DIM_CUBE) {
      /* Cube image descriptors are set up as simple arrays but SPIR-V wants
   * the number of cubes.
   */
   if (glsl_sampler_type_is_array(img->type)) {
      size = nir_vec3(b, nir_channel(b, size, 0),
            } else {
      size = nir_vec3(b, nir_channel(b, size, 0),
                                    }
      static bool
   lower_load_global_constant_offset_instr(nir_builder *b,
               {
      if (intrin->intrinsic != nir_intrinsic_load_global_constant_offset &&
      intrin->intrinsic != nir_intrinsic_load_global_constant_bounded)
                  nir_def *base_addr = intrin->src[0].ssa;
            nir_def *zero = NULL;
   if (intrin->intrinsic == nir_intrinsic_load_global_constant_bounded) {
               unsigned bit_size = intrin->def.bit_size;
   assert(bit_size >= 8 && bit_size % 8 == 0);
                              nir_def *sat_offset =
         nir_def *in_bounds =
                        nir_def *val =
      nir_build_load_global(b, intrin->def.num_components,
                                 if (intrin->intrinsic == nir_intrinsic_load_global_constant_bounded) {
      nir_pop_if(b, NULL);
                           }
      static nir_variable *
   find_or_create_input(nir_builder *b, const struct glsl_type *type,
         {
      nir_foreach_shader_in_variable(in, b->shader) {
      if (in->data.location == location)
      }
   nir_variable *in = nir_variable_create(b->shader, nir_var_shader_in,
         in->data.location = location;
   if (glsl_type_is_integer(type))
               }
      static bool
   lower_fragcoord_instr(nir_builder *b, nir_instr *instr, UNUSED void *_data)
   {
      assert(b->shader->info.stage == MESA_SHADER_FRAGMENT);
            if (instr->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
            nir_def *val;
   switch (intrin->intrinsic) {
   case nir_intrinsic_load_frag_coord:
      var = find_or_create_input(b, glsl_vec4_type(),
               val = nir_load_var(b, var);
      case nir_intrinsic_load_point_coord:
      var = find_or_create_input(b, glsl_vector_type(GLSL_TYPE_FLOAT, 2),
               val = nir_load_var(b, var);
      case nir_intrinsic_load_sample_pos:
      var = find_or_create_input(b, glsl_vec4_type(),
               val = nir_ffract(b, nir_trim_vector(b, nir_load_var(b, var), 2));
      case nir_intrinsic_load_layer_id:
      var = find_or_create_input(b, glsl_int_type(),
         val = nir_load_var(b, var);
         default:
                              }
      static bool
   lower_system_value_first_vertex(nir_builder *b, nir_instr *instr, UNUSED void *_data)
   {
               if (instr->type != nir_instr_type_intrinsic)
                     if (intrin->intrinsic != nir_intrinsic_load_first_vertex)
            b->cursor = nir_before_instr(&intrin->instr);
   nir_def *base_vertex = nir_load_base_vertex(b);
               }
      static int
   count_location_slots(const struct glsl_type *type, bool bindless)
   {
         }
      static void
   assign_io_locations(nir_shader *nir)
   {
      if (nir->info.stage != MESA_SHADER_VERTEX) {
      unsigned location = 0;
   nir_foreach_variable_with_modes(var, nir, nir_var_shader_in) {
      var->data.driver_location = location;
   if (nir_is_arrayed_io(var, nir->info.stage)) {
         } else {
            }
      } else {
      nir_foreach_shader_in_variable(var, nir) {
      assert(var->data.location >= VERT_ATTRIB_GENERIC0);
                  {
      unsigned location = 0;
   nir_foreach_variable_with_modes(var, nir, nir_var_shader_out) {
      var->data.driver_location = location;
   if (nir_is_arrayed_io(var, nir->info.stage)) {
         } else {
            }
         }
      static const struct vk_ycbcr_conversion_state *
   lookup_ycbcr_conversion(const void *_layout, uint32_t set,
         {
      const struct vk_pipeline_layout *pipeline_layout = _layout;
   assert(set < pipeline_layout->set_count);
   const struct nvk_descriptor_set_layout *set_layout =
                  const struct nvk_descriptor_set_binding_layout *bind_layout =
            if (bind_layout->immutable_samplers == NULL)
                     const struct nvk_sampler *sampler =
            return sampler && sampler->vk.ycbcr_conversion ?
      }
      static void
   nvk_optimize_nir(nir_shader *nir)
   {
               do {
               NIR_PASS(progress, nir, nir_split_array_vars, nir_var_function_temp);
            if (!nir->info.var_copies_lowered) {
      /* Only run this pass if nir_lower_var_copies was not called
   * yet. That would lower away any copy_deref instructions and we
   * don't want to introduce any more.
   */
      }
   NIR_PASS(progress, nir, nir_opt_copy_prop_vars);
   NIR_PASS(progress, nir, nir_opt_dead_write_vars);
   NIR_PASS(progress, nir, nir_lower_vars_to_ssa);
   NIR_PASS(progress, nir, nir_copy_prop);
   NIR_PASS(progress, nir, nir_opt_remove_phis);
   NIR_PASS(progress, nir, nir_opt_dce);
   if (nir_opt_trivial_continues(nir)) {
      progress = true;
   NIR_PASS(progress, nir, nir_copy_prop);
   NIR_PASS(progress, nir, nir_opt_remove_phis);
      }
   NIR_PASS(progress, nir, nir_opt_if,
         NIR_PASS(progress, nir, nir_opt_dead_cf);
   NIR_PASS(progress, nir, nir_opt_cse);
   /*
   * this should be fine, likely a backend problem,
   * but a bunch of tessellation shaders blow up.
   * we should revisit this when NAK is merged.
   */
   NIR_PASS(progress, nir, nir_opt_peephole_select, 2, true, true);
   NIR_PASS(progress, nir, nir_opt_constant_folding);
                     if (nir->options->max_unroll_iterations) {
                     NIR_PASS(progress, nir, nir_opt_shrink_vectors);
   NIR_PASS(progress, nir, nir_remove_dead_variables,
      }
      VkResult
   nvk_shader_stage_to_nir(struct nvk_device *dev,
                           {
      struct nvk_physical_device *pdev = nvk_device_physical(dev);
   const gl_shader_stage stage = vk_to_mesa_shader_stage(sinfo->stage);
   const nir_shader_compiler_options *nir_options =
            unsigned char stage_sha1[SHA1_DIGEST_LENGTH];
            if (cache == NULL)
            nir_shader *nir = vk_pipeline_cache_lookup_nir(cache, stage_sha1,
                     if (nir != NULL) {
      *nir_out = nir;
               const struct spirv_to_nir_options spirv_options =
            VkResult result = vk_pipeline_shader_stage_to_nir(&dev->vk, sinfo,
                     if (result != VK_SUCCESS)
                                 }
      void
   nvk_lower_nir(struct nvk_device *dev, nir_shader *nir,
               const struct vk_pipeline_robustness_state *rs,
   {
               NIR_PASS(_, nir, nir_split_struct_vars, nir_var_function_temp);
            NIR_PASS(_, nir, nir_split_var_copies);
            NIR_PASS(_, nir, nir_lower_global_vars_to_local);
            NIR_PASS(_, nir, nir_lower_system_values);
   if (nir->info.stage == MESA_SHADER_VERTEX) {
      // codegen does not support SYSTEM_VALUE_FIRST_VERTEX
   NIR_PASS(_, nir, nir_shader_instructions_pass,
                     if (nir->info.stage == MESA_SHADER_FRAGMENT) {
      NIR_PASS(_, nir, nir_shader_instructions_pass, lower_fragcoord_instr,
         NIR_PASS(_, nir, nir_lower_input_attachments,
            &(nir_input_attachment_options) {
                           nir_lower_compute_system_values_options csv_options = {
         };
            /* Vulkan uses the separate-shader linking model */
            if (nir->info.stage == MESA_SHADER_VERTEX ||
      nir->info.stage == MESA_SHADER_GEOMETRY ||
   nir->info.stage == MESA_SHADER_FRAGMENT) {
      } else if (nir->info.stage == MESA_SHADER_TESS_EVAL) {
         }
            NIR_PASS(_, nir, nir_lower_global_vars_to_local);
                                 /* Lower push constants before lower_descriptors */
   NIR_PASS(_, nir, nir_lower_explicit_io, nir_var_mem_push_const,
            NIR_PASS(_, nir, nir_shader_intrinsics_pass, lower_image_size_to_txs,
            /* Lower non-uniform access before lower_descriptors */
   enum nir_lower_non_uniform_access_type lower_non_uniform_access_types =
            if (pdev->info.cls_eng3d < TURING_A) {
      lower_non_uniform_access_types |= nir_lower_non_uniform_texture_access |
               /* In practice, most shaders do not have non-uniform-qualified accesses
   * thus a cheaper and likely to fail check is run first.
   */
   if (nir_has_non_uniform_access(nir, lower_non_uniform_access_types)) {
      struct nir_lower_non_uniform_access_options opts = {
      .types = lower_non_uniform_access_types,
      };
   NIR_PASS(_, nir, nir_opt_non_uniform_access);
               NIR_PASS(_, nir, nvk_nir_lower_descriptors, rs, layout);
   NIR_PASS(_, nir, nir_lower_explicit_io, nir_var_mem_global,
         NIR_PASS(_, nir, nir_lower_explicit_io, nir_var_mem_ssbo,
         NIR_PASS(_, nir, nir_lower_explicit_io, nir_var_mem_ubo,
         NIR_PASS(_, nir, nir_shader_intrinsics_pass,
                  if (!nir->info.shared_memory_explicit_layout) {
      NIR_PASS(_, nir, nir_lower_vars_to_explicit_types,
      }
   NIR_PASS(_, nir, nir_lower_explicit_io, nir_var_mem_shared,
                                 nvk_optimize_nir(nir);
   if (nir->info.stage != MESA_SHADER_COMPUTE)
                        }
      #ifndef NDEBUG
   static void
   nvk_shader_dump(struct nvk_shader *shader)
   {
               if (shader->stage != MESA_SHADER_COMPUTE) {
      _debug_printf("dumping HDR for %s shader\n",
         for (pos = 0; pos < ARRAY_SIZE(shader->hdr); ++pos)
      _debug_printf("HDR[%02"PRIxPTR"] = 0x%08x\n",
   }
   _debug_printf("shader binary code (0x%x bytes):", shader->code_size);
   for (pos = 0; pos < shader->code_size / 4; ++pos) {
      if ((pos % 8) == 0)
            }
      }
   #endif
      #include "tgsi/tgsi_ureg.h"
      /* NOTE: Using a[0x270] in FP may cause an error even if we're using less than
   * 124 scalar varying values.
   */
   static uint32_t
   nvc0_shader_input_address(unsigned sn, unsigned si)
   {
      switch (sn) {
   case TGSI_SEMANTIC_TESSOUTER:    return 0x000 + si * 0x4;
   case TGSI_SEMANTIC_TESSINNER:    return 0x010 + si * 0x4;
   case TGSI_SEMANTIC_PATCH:        return 0x020 + si * 0x10;
   case TGSI_SEMANTIC_PRIMID:       return 0x060;
   case TGSI_SEMANTIC_LAYER:        return 0x064;
   case TGSI_SEMANTIC_VIEWPORT_INDEX:return 0x068;
   case TGSI_SEMANTIC_PSIZE:        return 0x06c;
   case TGSI_SEMANTIC_POSITION:     return 0x070;
   case TGSI_SEMANTIC_GENERIC:      return 0x080 + si * 0x10;
   case TGSI_SEMANTIC_FOG:          return 0x2e8;
   case TGSI_SEMANTIC_COLOR:        return 0x280 + si * 0x10;
   case TGSI_SEMANTIC_BCOLOR:       return 0x2a0 + si * 0x10;
   case TGSI_SEMANTIC_CLIPDIST:     return 0x2c0 + si * 0x10;
   case TGSI_SEMANTIC_CLIPVERTEX:   return 0x270;
   case TGSI_SEMANTIC_PCOORD:       return 0x2e0;
   case TGSI_SEMANTIC_TESSCOORD:    return 0x2f0;
   case TGSI_SEMANTIC_INSTANCEID:   return 0x2f8;
   case TGSI_SEMANTIC_VERTEXID:     return 0x2fc;
   case TGSI_SEMANTIC_TEXCOORD:     return 0x300 + si * 0x10;
   default:
      assert(!"invalid TGSI input semantic");
         }
      static uint32_t
   nvc0_shader_output_address(unsigned sn, unsigned si)
   {
      switch (sn) {
   case TGSI_SEMANTIC_TESSOUTER:     return 0x000 + si * 0x4;
   case TGSI_SEMANTIC_TESSINNER:     return 0x010 + si * 0x4;
   case TGSI_SEMANTIC_PATCH:         return 0x020 + si * 0x10;
   case TGSI_SEMANTIC_PRIMID:        return 0x060;
   case TGSI_SEMANTIC_LAYER:         return 0x064;
   case TGSI_SEMANTIC_VIEWPORT_INDEX:return 0x068;
   case TGSI_SEMANTIC_PSIZE:         return 0x06c;
   case TGSI_SEMANTIC_POSITION:      return 0x070;
   case TGSI_SEMANTIC_GENERIC:       return 0x080 + si * 0x10;
   case TGSI_SEMANTIC_FOG:           return 0x2e8;
   case TGSI_SEMANTIC_COLOR:         return 0x280 + si * 0x10;
   case TGSI_SEMANTIC_BCOLOR:        return 0x2a0 + si * 0x10;
   case TGSI_SEMANTIC_CLIPDIST:      return 0x2c0 + si * 0x10;
   case TGSI_SEMANTIC_CLIPVERTEX:    return 0x270;
   case TGSI_SEMANTIC_TEXCOORD:      return 0x300 + si * 0x10;
   case TGSI_SEMANTIC_VIEWPORT_MASK: return 0x3a0;
   case TGSI_SEMANTIC_EDGEFLAG:      return ~0;
   default:
      assert(!"invalid TGSI output semantic");
         }
      static int
   nvc0_vp_assign_input_slots(struct nv50_ir_prog_info_out *info)
   {
               for (n = 0, i = 0; i < info->numInputs; ++i) {
      switch (info->in[i].sn) {
   case TGSI_SEMANTIC_INSTANCEID: /* for SM4 only, in TGSI they're SVs */
   case TGSI_SEMANTIC_VERTEXID:
      info->in[i].mask = 0x1;
   info->in[i].slot[0] =
            default:
         }
   for (c = 0; c < 4; ++c)
                        }
      static int
   nvc0_sp_assign_input_slots(struct nv50_ir_prog_info_out *info)
   {
      unsigned offset;
            for (i = 0; i < info->numInputs; ++i) {
               for (c = 0; c < 4; ++c)
                  }
      static int
   nvc0_fp_assign_output_slots(struct nv50_ir_prog_info_out *info)
   {
      unsigned count = info->prop.fp.numColourResults * 4;
            /* Compute the relative position of each color output, since skipped MRT
   * positions will not have registers allocated to them.
   */
   unsigned colors[8] = {0};
   for (i = 0; i < info->numOutputs; ++i)
      if (info->out[i].sn == TGSI_SEMANTIC_COLOR)
      for (i = 0, c = 0; i < 8; i++)
      if (colors[i])
      for (i = 0; i < info->numOutputs; ++i)
      if (info->out[i].sn == TGSI_SEMANTIC_COLOR)
               if (info->io.sampleMask < NV50_CODEGEN_MAX_VARYINGS)
         else
   if (info->target >= 0xe0)
            if (info->io.fragDepth < NV50_CODEGEN_MAX_VARYINGS)
               }
      static int
   nvc0_sp_assign_output_slots(struct nv50_ir_prog_info_out *info)
   {
      unsigned offset;
            for (i = 0; i < info->numOutputs; ++i) {
               for (c = 0; c < 4; ++c)
                  }
      static int
   nvc0_program_assign_varying_slots(struct nv50_ir_prog_info_out *info)
   {
               if (info->type == PIPE_SHADER_VERTEX)
         else
         if (ret)
            if (info->type == PIPE_SHADER_FRAGMENT)
         else
            }
      static inline void
   nvk_vtgs_hdr_update_oread(struct nvk_shader *vs, uint8_t slot)
   {
      uint8_t min = (vs->hdr[4] >> 12) & 0xff;
            min = MIN2(min, slot);
               }
      static int
   nvk_vtgp_gen_header(struct nvk_shader *vs, struct nv50_ir_prog_info_out *info)
   {
               for (i = 0; i < info->numInputs; ++i) {
      if (info->in[i].patch)
         for (c = 0; c < 4; ++c) {
      a = info->in[i].slot[c];
   if (info->in[i].mask & (1 << c))
                  for (i = 0; i < info->numOutputs; ++i) {
      if (info->out[i].patch)
         for (c = 0; c < 4; ++c) {
      if (!(info->out[i].mask & (1 << c)))
         assert(info->out[i].slot[c] >= 0x40 / 4);
   a = info->out[i].slot[c] - 0x40 / 4;
   vs->hdr[13 + a / 32] |= 1 << (a % 32);
   if (info->out[i].oread)
                  for (i = 0; i < info->numSysVals; ++i) {
      switch (info->sv[i].sn) {
   case SYSTEM_VALUE_PRIMITIVE_ID:
      vs->hdr[5] |= 1 << 24;
      case SYSTEM_VALUE_INSTANCE_ID:
      vs->hdr[10] |= 1 << 30;
      case SYSTEM_VALUE_VERTEX_ID:
      vs->hdr[10] |= 1 << 31;
      case SYSTEM_VALUE_TESS_COORD:
      /* We don't have the mask, nor the slots populated. While this could
   * be achieved, the vast majority of the time if either of the coords
   * are read, then both will be read.
   */
   nvk_vtgs_hdr_update_oread(vs, 0x2f0 / 4);
   nvk_vtgs_hdr_update_oread(vs, 0x2f4 / 4);
      default:
                     vs->vs.clip_enable = (1 << info->io.clipDistances) - 1;
   vs->vs.cull_enable =
         for (i = 0; i < info->io.cullDistances; ++i)
                        }
      static int
   nvk_vs_gen_header(struct nvk_shader *vs, struct nv50_ir_prog_info_out *info)
   {
      vs->hdr[0] = 0x20061 | (1 << 10);
               }
      static int
   nvk_gs_gen_header(struct nvk_shader *gs,
               {
                        switch (info->prop.gp.outputPrim) {
   case MESA_PRIM_POINTS:
      gs->hdr[3] = 0x01000000;
      case MESA_PRIM_LINE_STRIP:
      gs->hdr[3] = 0x06000000;
      case MESA_PRIM_TRIANGLE_STRIP:
      gs->hdr[3] = 0x07000000;
      default:
      assert(0);
                                    }
      static void
   nvk_generate_tessellation_parameters(const struct nv50_ir_prog_info_out *info,
         {
      // TODO: this is a little confusing because nouveau codegen uses
   // MESA_PRIM_POINTS for unspecified domain and
   // MESA_PRIM_POINTS = 0, the same as NV9097 ISOLINE enum
   uint32_t domain_type;
   switch (info->prop.tp.domain) {
   case MESA_PRIM_LINES:
      domain_type = NV9097_SET_TESSELLATION_PARAMETERS_DOMAIN_TYPE_ISOLINE;
      case MESA_PRIM_TRIANGLES:
      domain_type = NV9097_SET_TESSELLATION_PARAMETERS_DOMAIN_TYPE_TRIANGLE;
      case MESA_PRIM_QUADS:
      domain_type = NV9097_SET_TESSELLATION_PARAMETERS_DOMAIN_TYPE_QUAD;
      default:
      domain_type = ~0;
      }
   shader->tp.domain_type = domain_type;
   if (domain_type == ~0) {
                  uint32_t spacing;
   switch (info->prop.tp.partitioning) {
   case PIPE_TESS_SPACING_EQUAL:
      spacing = NV9097_SET_TESSELLATION_PARAMETERS_SPACING_INTEGER;
      case PIPE_TESS_SPACING_FRACTIONAL_ODD:
      spacing = NV9097_SET_TESSELLATION_PARAMETERS_SPACING_FRACTIONAL_ODD;
      case PIPE_TESS_SPACING_FRACTIONAL_EVEN:
      spacing = NV9097_SET_TESSELLATION_PARAMETERS_SPACING_FRACTIONAL_EVEN;
      default:
      assert(!"invalid tessellator partitioning");
      }
            uint32_t output_prims;
   if (info->prop.tp.outputPrim == MESA_PRIM_POINTS) { // point_mode
         } else if (info->prop.tp.domain == MESA_PRIM_LINES) { // isoline domain
         } else {  // triangle/quad domain
      if (info->prop.tp.winding > 0) {
         } else {
            }
      }
      static int
   nvk_tcs_gen_header(struct nvk_shader *tcs, struct nv50_ir_prog_info_out *info)
   {
               if (info->numPatchConstants)
                     tcs->hdr[1] = opcs << 24;
                              if (info->target >= NVISA_GM107_CHIPSET) {
      /* On GM107+, the number of output patch components has moved in the TCP
   * header, but it seems like blob still also uses the old position.
   * Also, the high 8-bits are located in between the min/max parallel
   * field and has to be set after updating the outputs. */
   tcs->hdr[3] = (opcs & 0x0f) << 28;
                           }
      static int
   nvk_tes_gen_header(struct nvk_shader *tes, struct nv50_ir_prog_info_out *info)
   {
      tes->hdr[0] = 0x20061 | (3 << 10);
                                          }
      #define NVC0_INTERP_FLAT          (1 << 0)
   #define NVC0_INTERP_PERSPECTIVE   (2 << 0)
   #define NVC0_INTERP_LINEAR        (3 << 0)
   #define NVC0_INTERP_CENTROID      (1 << 2)
      static uint8_t
   nvk_hdr_interp_mode(const struct nv50_ir_varying *var)
   {
      if (var->linear)
         if (var->flat)
            }
         static int
   nvk_fs_gen_header(struct nvk_shader *fs, const struct nvk_fs_key *key,
         {
               /* just 00062 on Kepler */
   fs->hdr[0] = 0x20062 | (5 << 10);
            if (info->prop.fp.usesDiscard || key->zs_self_dep)
         if (!info->prop.fp.separateFragData)
         if (info->io.sampleMask < 80 /* PIPE_MAX_SHADER_OUTPUTS */)
         if (info->prop.fp.writesDepth) {
      fs->hdr[19] |= 0x2;
               for (i = 0; i < info->numInputs; ++i) {
      m = nvk_hdr_interp_mode(&info->in[i]);
   if (info->in[i].sn == TGSI_SEMANTIC_COLOR) {
      fs->fs.colors |= 1 << info->in[i].si;
   if (info->in[i].sc)
      }
   for (c = 0; c < 4; ++c) {
      if (!(info->in[i].mask & (1 << c)))
         a = info->in[i].slot[c];
   if (info->in[i].slot[0] >= (0x060 / 4) &&
      info->in[i].slot[0] <= (0x07c / 4)) {
      } else
   if (info->in[i].slot[0] >= (0x2c0 / 4) &&
      info->in[i].slot[0] <= (0x2fc / 4)) {
      } else {
      if (info->in[i].slot[c] < (0x040 / 4) ||
      info->in[i].slot[c] > (0x380 / 4))
      a *= 2;
   if (info->in[i].slot[0] >= (0x300 / 4))
                  }
   /* GM20x+ needs TGSI_SEMANTIC_POSITION to access sample locations */
   if (info->prop.fp.readsSampleLocations && info->target >= NVISA_GM200_CHIPSET)
            for (i = 0; i < info->numOutputs; ++i) {
      if (info->out[i].sn == TGSI_SEMANTIC_COLOR)
               /* There are no "regular" attachments, but the shader still needs to be
   * executed. It seems like it wants to think that it has some color
   * outputs in order to actually run.
   */
   if (info->prop.fp.numColourResults == 0 &&
      !info->prop.fp.writesDepth &&
   info->io.sampleMask >= 80 /* PIPE_MAX_SHADER_OUTPUTS */)
         fs->fs.early_z = info->prop.fp.earlyFragTests;
   fs->fs.sample_mask_in = info->prop.fp.usesSampleMaskIn;
   fs->fs.reads_framebuffer = info->prop.fp.readsFramebuffer;
            /* Mark position xy and layer as read */
   if (fs->fs.reads_framebuffer)
               }
      static uint8_t find_register_index_for_xfb_output(const struct nir_shader *nir,
         {
      nir_foreach_shader_out_variable(var, nir) {
      uint32_t slots = glsl_count_vec4_slots(var->type, false, false);
   for (uint32_t i = 0; i < slots; ++i) {
      if (output.location == (var->data.location+i)) {
               }
   // should not be reached
      }
      static struct nvk_transform_feedback_state *
   nvk_fill_transform_feedback_state(struct nir_shader *nir,
         {
      const uint8_t max_buffers = 4;
   const uint8_t dw_bytes = 4;
   const struct nir_xfb_info *nx = nir->xfb_info;
            struct nvk_transform_feedback_state *xfb =
            if (!xfb)
            for (uint8_t b = 0; b < max_buffers; ++b) {
      xfb->stride[b] = b < nx->buffers_written ? nx->buffers[b].stride : 0;
   xfb->varying_count[b] = 0;
      }
            if (info->numOutputs == 0)
            for (uint32_t i = 0; i < nx->output_count; ++i) {
      const nir_xfb_output_info output = nx->outputs[i];
   const uint8_t b = output.buffer;
   const uint8_t r = find_register_index_for_xfb_output(nir, output);
                     u_foreach_bit(c, nx->outputs[i].component_mask)
                        /* zero unused indices */
   for (uint8_t b = 0; b < 4; ++b)
      for (uint32_t c = xfb->varying_count[b]; c & 3; ++c)
            }
      VkResult
   nvk_compile_nir(struct nvk_physical_device *pdev, nir_shader *nir,
               {
      struct nv50_ir_prog_info *info;
   struct nv50_ir_prog_info_out info_out = {};
            info = CALLOC_STRUCT(nv50_ir_prog_info);
   if (!info)
            info->type = pipe_shader_type_from_mesa(nir->info.stage);
   info->target = pdev->info.chipset;
            for (unsigned i = 0; i < 3; i++)
            info->bin.smemSize = shader->cp.smem_size;
   info->dbgFlags = get_prog_debug();
   info->optLevel = get_prog_optimize();
   info->io.auxCBSlot = 1;
   info->io.uboInfoBase = 0;
   info->io.drawInfoBase = nvk_root_descriptor_offset(draw.base_vertex);
   if (nir->info.stage == MESA_SHADER_COMPUTE) {
         } else {
         }
   ret = nv50_ir_generate_code(info, &info_out);
   if (ret)
            if (info_out.bin.fixupData) {
      nv50_ir_apply_fixups(info_out.bin.fixupData, info_out.bin.code,
                           shader->stage = nir->info.stage;
   shader->code_ptr = (uint8_t *)info_out.bin.code;
            if (info_out.target >= NVISA_GV100_CHIPSET)
         else
         shader->cp.smem_size = info_out.bin.smemSize;
            switch (info->type) {
   case PIPE_SHADER_VERTEX:
      ret = nvk_vs_gen_header(shader, &info_out);
      case PIPE_SHADER_FRAGMENT:
      ret = nvk_fs_gen_header(shader, fs_key, &info_out);
   shader->fs.uses_sample_shading = nir->info.fs.uses_sample_shading;
      case PIPE_SHADER_GEOMETRY:
      ret = nvk_gs_gen_header(shader, nir, &info_out);
      case PIPE_SHADER_TESS_CTRL:
      ret = nvk_tcs_gen_header(shader, &info_out);
      case PIPE_SHADER_TESS_EVAL:
      ret = nvk_tes_gen_header(shader, &info_out);
      case PIPE_SHADER_COMPUTE:
         default:
      unreachable("Invalid shader stage");
      }
            if (info_out.bin.tlsSpace) {
      assert(info_out.bin.tlsSpace < (1 << 24));
   shader->hdr[0] |= 1 << 26;
   shader->hdr[1] |= align(info_out.bin.tlsSpace, 0x10); /* l[] size */
               if (info_out.io.globalAccess)
         if (info_out.io.globalAccess & 0x2)
         if (info_out.io.fp64)
            if (nir->xfb_info) {
      shader->xfb = nvk_fill_transform_feedback_state(nir, &info_out);
   if (shader->xfb == NULL) {
                        }
      VkResult
   nvk_shader_upload(struct nvk_device *dev, struct nvk_shader *shader)
   {
      uint32_t hdr_size = 0;
   if (shader->stage != MESA_SHADER_COMPUTE) {
      if (dev->pdev->info.cls_eng3d >= TURING_A)
         else
               /* Fermi   needs 0x40 alignment
   * Kepler+ needs the first instruction to be 0x80 aligned, so we waste 0x30 bytes
   */
   int alignment = dev->pdev->info.cls_eng3d >= KEPLER_A ? 0x80 : 0x40;
            if (dev->pdev->info.cls_eng3d >= KEPLER_A &&
      dev->pdev->info.cls_eng3d < TURING_A &&
   hdr_size > 0) {
   /* offset will be 0x30 */
               uint32_t total_size = shader->code_size + hdr_size + offset;
   char *data = malloc(total_size);
   if (data == NULL)
            memcpy(data + offset, shader->hdr, hdr_size);
         #ifndef NDEBUG
      if (debug_get_bool_option("NV50_PROG_DEBUG", false))
      #endif
         VkResult result = nvk_heap_upload(dev, &dev->shader_heap, data,
         if (result == VK_SUCCESS) {
      shader->upload_size = total_size;
      }
               }
      void
   nvk_shader_finish(struct nvk_device *dev, struct nvk_shader *shader)
   {
      if (shader->upload_size > 0) {
      nvk_heap_free(dev, &dev->shader_heap,
            }
      }
